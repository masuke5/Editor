#include "stdafx.h"
#include "App.h"

using namespace D2D1;

App::App() :
	hwnd(nullptr),
	direct2dFactory(nullptr),
	renderTarget(nullptr),
	dwriteFactory(nullptr),
	editor(nullptr) {
}

App::~App() {
	SafeRelease(&direct2dFactory);
	SafeRelease(&dwriteFactory);
	SafeRelease(&renderTarget);
}

void App::RunMessageLoop() {
	MSG msg;
	while (GetMessage(&msg, nullptr, 0, 0)) {
		TranslateMessage(&msg);
		DispatchMessage(&msg);

		OnRender();
	}
}

HRESULT App::Initialize() {
	HRESULT result;
	result = CreateDeviceIndependentResources();

	if (SUCCEEDED(result)) {
		WNDCLASSEX wcex = { sizeof(WNDCLASSEX) };
		wcex.style = CS_HREDRAW | CS_VREDRAW;
		wcex.lpfnWndProc = App::WndProc;
		wcex.cbClsExtra = 0;
		wcex.cbWndExtra = sizeof(LONG_PTR);
		wcex.hInstance = HINST_THISCOMPONENT;
		wcex.hbrBackground = nullptr;
		wcex.lpszMenuName = nullptr;
		wcex.hCursor = LoadCursor(nullptr, IDI_APPLICATION);
		wcex.lpszClassName = L"App";

		RegisterClassEx(&wcex);

		FLOAT dpi_x, dpi_y;
		direct2dFactory->GetDesktopDpi(&dpi_x, &dpi_y);

		// ウィンドウを作成
		hwnd = CreateWindow(
			L"App",
			L"This is app",
			WS_OVERLAPPEDWINDOW,
			CW_USEDEFAULT,
			CW_USEDEFAULT,
			static_cast<UINT>(ceil(640.f * dpi_x / 96.f)),
			static_cast<UINT>(ceil(480.f * dpi_y / 96.f)),
			nullptr,
			nullptr,
			HINST_THISCOMPONENT,
			this);

		result = hwnd ? S_OK : E_FAIL;
		if (SUCCEEDED(result)) {
			ShowWindow(hwnd, SW_SHOWNORMAL);
			UpdateWindow(hwnd);
		}
	}

	return result;
}

HRESULT App::CreateDeviceIndependentResources() {
	HRESULT hr = S_OK;
	hr = D2D1CreateFactory(D2D1_FACTORY_TYPE_SINGLE_THREADED, &direct2dFactory);

	if (SUCCEEDED(hr)) {
		hr = DWriteCreateFactory(
			DWRITE_FACTORY_TYPE_SHARED, 
			__uuidof(IDWriteFactory), 
			reinterpret_cast<IUnknown**>(&dwriteFactory));
	}

	return hr;
}

HRESULT App::CreateDeviceResources() {
	HRESULT result = S_OK;

	if (!renderTarget) {
		RECT rc;
		GetClientRect(hwnd, &rc);

		D2D1_SIZE_U size = D2D1::SizeU(rc.right - rc.left, rc.bottom - rc.top);

		result = direct2dFactory->CreateHwndRenderTarget(
			RenderTargetProperties(),
			HwndRenderTargetProperties(hwnd, size),
			&renderTarget);
	}

	return result;
}

void App::DiscardDeviceResources() {
	SafeRelease(&renderTarget);
}

LRESULT CALLBACK App::WndProc(HWND hwnd, UINT message, WPARAM wparam, LPARAM lparam) {
	if (message == WM_CREATE) {
		LPCREATESTRUCT pcs = (LPCREATESTRUCT)lparam;
		App* app = (App*)pcs->lpCreateParams;

		SetWindowLongPtr(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(app));

		app->editor = std::make_unique<Editor>(hwnd, app->dwriteFactory);
		try {
			app->editor->Initialize();
		} catch (const EditorException& e) {
			MessageBox(hwnd, char_to_wchar(e.what()), L"エディタの初期化に失敗しました", MB_OK | MB_ICONERROR);
			exit(1);
		}
		app->editor->SetText(L"H");

		// タイマーを設定
		for (auto& timer : app->editor->timers) {
			SetTimer(hwnd, timer->id, timer->elapse, nullptr);
		}

		return 0;
	}
	else {
		App* app = reinterpret_cast<App*>(static_cast<LONG_PTR>(
			GetWindowLongPtr(hwnd, GWLP_USERDATA)));

		if (app) {
			switch (message) {
			case WM_SETCURSOR:
				if (LOWORD(lparam) == HTCLIENT) {
					SetCursor(LoadCursor(nullptr, IDC_IBEAM));
					return TRUE;
				}
				break;
			case WM_CHAR:
			case WM_SYSCHAR:
			case WM_IME_CHAR:
				app->editor->OnChar(static_cast<wchar_t>(wparam));
				return 0;
			case WM_KEYDOWN:
				app->editor->OnKeyDown(static_cast<int>(wparam));
				return 0;
			case WM_KEYUP:
				app->editor->OnKeyUp(static_cast<int>(wparam));
				return 0;
			case WM_LBUTTONDOWN:
				app->editor->OnLButtonDown(static_cast<float>(GET_X_LPARAM(lparam)), static_cast<float>(GET_Y_LPARAM(lparam)));
				return 0;
			case WM_LBUTTONUP:
				app->editor->OnLButtonUp(static_cast<float>(GET_X_LPARAM(lparam)), static_cast<float>(GET_Y_LPARAM(lparam)));
				return 0;
			case WM_MOUSEWHEEL:
				app->editor->OnMouseWheel(GET_WHEEL_DELTA_WPARAM(wparam));
				return 0;
			case WM_IME_SETCONTEXT:
				lparam &= ~ISC_SHOWUICOMPOSITIONWINDOW;
				return 0;
			case WM_IME_NOTIFY:
				switch (wparam) {
				case IMN_OPENCANDIDATE:
					app->editor->OnOpenCandidate();
					return 0;
				}
				break;
			case WM_IME_REQUEST:
				switch (wparam) {
				case IMR_QUERYCHARPOSITION:
					app->editor->OnQueryCharPosition(reinterpret_cast<IMECHARPOSITION*>(lparam));
					return 1;
				}
				break;
			case WM_IME_STARTCOMPOSITION:
				app->editor->OnIMEStartComposition();
				break;
			case WM_IME_COMPOSITION:
				app->editor->OnIMEComposition(lparam);
				break;
			case WM_IME_ENDCOMPOSITION:
				app->editor->OnIMEEndComposition();
				break;
			case WM_SIZE:
			{
				UINT width = LOWORD(lparam);
				UINT height = HIWORD(lparam);
				app->OnResize(width, height);
				app->editor->OnResize(width, height);
			}
				return 0;
			case WM_TIMER:
				// タイマーの実行
				for (auto& timer : app->editor->timers) {
					if (wparam == timer->id && timer->enabled) {
						timer->func();
					}
				}
				return 0;
			case WM_DISPLAYCHANGE:
				InvalidateRect(hwnd, nullptr, false);
				return 0;
			case WM_DESTROY:
				PostQuitMessage(0);
				return 0;
			}
		}

		return DefWindowProc(hwnd, message, wparam, lparam);
	}
}

HRESULT App::OnRender() {
	HRESULT result = S_OK;

	result = CreateDeviceResources();
	if (SUCCEEDED(result)) {
		renderTarget->BeginDraw();
		renderTarget->SetTransform(Matrix3x2F::Identity());
		renderTarget->Clear(ColorF(ColorF::White));

		editor->Render(renderTarget);

		result = renderTarget->EndDraw();
		if (result == D2DERR_RECREATE_TARGET) {
			result = S_OK;
			DiscardDeviceResources();
		}
	}

	return result;
}

void App::OnResize(UINT width, UINT height) {
	if (renderTarget) {
		renderTarget->Resize(SizeU(width, height));
	}
}