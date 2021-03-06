
#include "stdafx.h"
#include "App.h"

#pragma comment(lib, "d2d1")
#pragma comment(lib, "dwrite")
#pragma comment(lib, "imm32")
#pragma comment(lib, "lib/icudt")
#pragma comment(lib, "lib/icuin")
#pragma comment(lib, "lib/icuio")
#pragma comment(lib, "lib/icutu")
#pragma comment(lib, "lib/icuuc")

int WINAPI WinMain(HINSTANCE instance, HINSTANCE prev_instance, LPSTR cmd_line, int cmd_show) {
	setlocale(LC_ALL, "");
	HeapSetInformation(nullptr, HeapEnableTerminationOnCorruption, nullptr, 0);

	// コンソール表示
	if (!AttachConsole(ATTACH_PARENT_PROCESS)) {
		AllocConsole();
	}

	FILE* fp;
	freopen_s(&fp, "CONIN$", "r", stdin);
	freopen_s(&fp, "CONOUT$", "w", stdout);
	freopen_s(&fp, "CONOUT$", "w", stderr);

	if (SUCCEEDED(CoInitialize(nullptr))) {
		{
			App app;
			if (SUCCEEDED(app.Initialize())) {
				app.RunMessageLoop();
			}
		}
		CoUninitialize();
	}

	fclose(fp);
	FreeConsole();

	return 0;
}