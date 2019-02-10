
#include "GLibMain.h"

#include "MediaPlayer.h"
#include "LoadSong.h"

void GLibMain(GLib::Frame* frame)
{
	frame->init("Test", 1000, 800);

	frame->addView<GLib::MainBar>(0, 0, -1, 50, "Test Page");

	auto tabs = frame->addView<GLib::TabView>(0, 50);

	auto playerTab = tabs->getNewTab(" Player");
	auto loadTab = tabs->getNewTab(" Load");
	auto consoleTab = tabs->getNewTab(" Console");

	consoleTab->addView<GLib::MovingView>(0, 0, -1, -1, false, true)->getMovingView()->addView<GLib::OutputView>()->setDefault();

	auto player = playerTab->addView<MediaPlayer>(0, 0, -1, 500);
	loadTab->addView<LoadSong>(player);

	//auto hueTab = tabs->getNewTab(" Hue");
}

