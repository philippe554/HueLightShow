
#include "GLibMain.h"

#include "MediaPlayer.h"
#include "LoadSong.h"
#include "ShowGenerator.h"
#include "TextBoxTest.h"

void GLibMain(GLib::Frame* frame)
{
	frame->init("Test", 1000, 700);

	frame->addView<GLib::MainBar>(0, 0, -1, 50, "Hue Music Player");

	auto tabs = frame->addView<GLib::TabView>(0, 50);

	auto playerTab = tabs->getNewTab(" Player");
	auto loadTab = tabs->getNewTab(" Song");
	auto generateTab = tabs->getNewTab(" Show");
	auto consoleTab = tabs->getNewTab(" Console");
	auto testTab = tabs->getNewTab(" Test");

	consoleTab->addView<GLib::MovingView>(0, 0, -1, -1, false, true)->getMovingView()->addView<GLib::OutputView>()->setDefault();

	auto player = playerTab->addView<MediaPlayer>(0, 0, -1, -1);

	loadTab->addView<LoadSong>(player);
	generateTab->addView<ShowGenerator>(player);

	testTab->addView<TextBoxTest>();

	//auto hueTab = tabs->getNewTab(" Hue");
}

