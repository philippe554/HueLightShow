
#include "GLibMain.h"

#include "MediaPlayer.h"

void GLibMain(GLib::Frame* frame)
{
	frame->init("Test", 1000, 800);

	frame->addView<GLib::MainBar>(0, 0, -1, 50, "Test Page");

	auto tabs = frame->addView<GLib::TabView>(0, 50);

	auto playerTab = tabs->getNewTab(" Player");

	auto bottomView = playerTab->addView<GLib::MovingView>(0, 500, -1, -1, false, true)->getMovingView();
	bottomView->addView<GLib::OutputView>()->setDefault();

	playerTab->addView<MediaPlayer>(0, 0, -1, 500);

	//auto hueTab = tabs->getNewTab(" Hue");
}

