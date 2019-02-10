#pragma once

#include "GLib.h"

void GLibMain(GLib::Frame* frame);

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, PSTR lpCmdLine, int CmdShow)
{
	HeapSetInformation(NULL, HeapEnableTerminationOnCorruption, NULL, 0);

	if (SUCCEEDED(CoInitialize(NULL)))
	{
		GLib::Frame frame(nullptr, 0, 0, 0, 0);

		GLibMain(&frame);

		frame.showWindow(CmdShow);

		frame.runMessageLoop();

		CoUninitialize();
	}
}