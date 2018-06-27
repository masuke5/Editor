#include "stdafx.h"
#include "Editor.h"

using namespace D2D1;

EditorOptions default_editoroptions() {
	EditorOptions options;

	return options;
}

Editor::Editor(IDWriteFactory* factory, IDWriteTextFormat* text_format, const EditorOptions& options) :
	factory(factory),
	text_format(text_format),
	options(options) {
}

void Editor::initialize() {
	// TODO: 文字の高さを測定
	char_height = 20;
}

void Editor::set_text(const std::wstring& str) {
	chars.clear();

	for (auto& ch : str) {
		auto character = create_char(ch);
		chars.push_back(character);
	}
}

void Editor::append_char(wchar_t wchar) {
	auto character = create_char(wchar);
	chars.push_back(character);
}

Char Editor::create_char(wchar_t character) {
	Char ch;
	ch.wchar = character;
	
	// 空白文字でなかったら文字の幅を取得
	IDWriteTextLayout* layout;
	// IDWriteTextLayout を作成
	auto result = factory->CreateTextLayout(&character, wcslen(&character), text_format, 100, 100, &layout);
	if (SUCCEEDED(result)) {
		DWRITE_TEXT_METRICS metrics;
		layout->GetMetrics(&metrics);
		// widthIncludingTrailingWhitespace は空白文字の幅も返す
		ch.width = metrics.widthIncludingTrailingWhitespace;

		// IDWriteTextLayout を破棄
		layout->Release();
	}

	return ch;
}

void Editor::render(ID2D1HwndRenderTarget* rt) {
	ID2D1SolidColorBrush* brush;
	HRESULT hr = rt->CreateSolidColorBrush(ColorF(ColorF::Black), &brush);

	if (SUCCEEDED(hr)) {
		float x = 0;
		float y = 0;
		for (auto& character : chars) {
			// 文字の描画
			rt->DrawText(
				&character.wchar,
				1,
				text_format,
				&RectF(x, y, x + character.width, y + char_height),
				brush);

			x += character.width;
		}
	}
}

void Editor::on_char(wchar_t character) {
	append_char(character);
}