
#include "stdafx.h"
#include "App.h"

#pragma comment(lib, "d2d1")

int WINAPI WinMain(HINSTANCE instance, HINSTANCE prev_instance, LPSTR cmd_line, int cmd_show) {
	HeapSetInformation(nullptr, HeapEnableTerminationOnCorruption, nullptr, 0);

	if (SUCCEEDED(CoInitialize(nullptr))) {
		{
			App app;
			if (SUCCEEDED(app.initialize())) {
				app.run_message_loop();
			}
		}
		CoUninitialize();
	}

	return 0;
}