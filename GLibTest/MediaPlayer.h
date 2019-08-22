#pragma once

#include <mutex>
#include <thread>
#include <iostream>
#include <vector>
#include <algorithm>
#include <numeric>
#include <memory>
#include <random>

#include "dlib/clustering.h"

#include "Razer.h"
#include "GLibMain.h"
#include "Hue.h"
#include "ShowPlot.h"
#include "SongData.h"

#include "portaudio.h"
#include "mpg123.h"
#include "fftw3.h"

#define PI 3.14159265359

namespace Aubio
{
	#include "aubio/aubio.h"
};

#define SAMPLE_RATE         (44100)
#define FRAMES_PER_BUFFER   (1024)

using namespace GLib;

void errorWrapper(PaError error)
{
	if (error != paNoError)
	{
		throw std::runtime_error("Port Audio error " + std::to_string(error) + ":" + Pa_GetErrorText(error));
	}
}

class MediaPlayer : public View
{
public:
	using View::View;
	void init()
	{
		workerThread = std::make_unique<std::thread>(&MediaPlayer::workerFunction, this);

		addView<Button>(10, 10, 100, 50, [this]()
		{
			this->play = !this->play;
		}, " Play");

		addView<Button>(10, int(place.bottom - place.top - 200 - 90), int((place.right - place.left) / 2 - 20), 80, [this]()
		{
			if (!hue)
			{
				hue = std::make_unique<Hue>("192.168.178.12", "Gl4AsrYSHlDx3mBxFDDfIvVxEFH22dVKxZ6xXfcr", "496C432C6171944976E29F6790408ADB");
				hue->startEntertainmentMode(hue->getFirstEntertainmentGroup());
				if (lightShow)
				{
					hue->setLightShow(lightShow);
				}
			}
			else
			{
				hue->setplayShow(!hue->isPlayShow());
			}
		}, " Hue Play/Pause");

		addView<Button>(int((place.right - place.left) / 2 + 10), int(place.bottom - place.top - 200 - 90), int((place.right - place.left) / 2 - 20), 80, [this]()
		{
			if (!razer)
			{
				razer = std::make_unique<Razer>();
			}
		}, " Sync Razer");

		addMouseListener(WM_LBUTTONDOWN, [&](int x, int y)
		{
			if (songScrubBar.left <= x && x <= songScrubBar.right && songScrubBar.top <= y && y <= songScrubBar.bottom)
			{
				if (songData)
				{
					location = float(x - songScrubBar.left) / float(songScrubBar.right - songScrubBar.left) * songData->data.size();
				}
			}
		});

		MovingView* movingView = addView<GLib::MovingView>(0, 200, -1, 100, true, false);
		movingView->setScrollZoom(true, false);

		View* movingSoundPlot = movingView->getMovingView();
		showPlot = movingSoundPlot->addView<ShowPlot>();
	}

	~MediaPlayer()
	{
		stop = true;
		workerThread->join();
		workerThread.reset();
	}

	void render(RT* rt, Writer* w, Color* c, D2D1_RECT_F& visibleRect) override
	{
		rt->FillRectangle(songScrubBar, c->get(C::DarkRed));

		if (songData)
		{
			float percentageDone = songData->data.size() != 0 ? float(location) / float(songData->data.size()) : 0;

			D2D1_RECT_F songDone = { songScrubBar.left, songScrubBar.top, songScrubBar.left + (songScrubBar.right - songScrubBar.left) * percentageDone, songScrubBar.bottom };
			rt->FillRectangle(songDone, c->get(C::Red));

			int seconds = location / songData->sampleRate;
			int minutes = seconds / 60;
			seconds -= minutes * 60;

			D2D1_RECT_F songLocation = { 10, 60, 100, 110};
			w->print(std::to_string(minutes) + ":" + std::to_string(seconds), c->get(C::Black), w->get(30), songLocation);
		}

		/*int middle = (songScrubBar.bottom + songScrubBar.top) * 0.5;
		for (int i = 0; i < energy.size(); i++)
		{
			float from = songScrubBar.left + (songScrubBar.right - songScrubBar.left) * float(i) / energy.size();
			float to = songScrubBar.left + (songScrubBar.right - songScrubBar.left) * float(i + 1) / energy.size();
			D2D1_RECT_F e = { from, middle - energy[i] * 100, to, middle + energy[i] * 100 };
			rt->FillRectangle(e, c->get(C::LightGray));
		}*/

		if (lightShow)
		{
			auto lights = lightShow->getDefaultLights();
			int amount = lights.size();
			float size = (place.right - place.left) / amount;

			for (int i = 0; i < lights.size(); i++)
			{
				LightColor color;

				if (hue && hue->isPlayShow())
				{
					color = lightShow->getLastState(lights[i]);
				}
				else
				{
					color = lightShow->getState(lights[i]);
				}

				D2D1_RECT_F box = { i * size - 1, place.bottom - place.top - 200, size + i * size + 1, place.bottom - place.top };
				rt->FillRectangle(box, c->get(C::Black));
				D2D1_RECT_F light = { i * size + 10, place.bottom - place.top - 200 + 10, size + i * size - 10, place.bottom - place.top - 10 };
				rt->FillRectangle(light, c->get(color.red * 255, color.green * 255, color.blue * 255));

				if(i == 0 && razer)
				{
					razer->setColor(color.red, color.green, color.blue);
				}
			}
		}
	}
	void update()
	{
	}

	void setSongData(std::shared_ptr<const SongData> sd, std::shared_ptr<const SongPartData> spd)
	{
		songData = sd;
		songPartData = spd;
		location = 0;
		showPlot->setSongPartData(songData, songPartData);
	}
	std::shared_ptr<const SongData> getSongData()
	{
		return songData;
	}

	void setLightShow(std::shared_ptr<LightShow> ls)
	{
		lightShow = ls;
		if (hue)
		{
			hue->setLightShow(lightShow);
		}
	}

private:
	void workerFunction()
	{
		try
		{
			PaStreamParameters outputParameters;
			PaStream *stream;

			float buffer[FRAMES_PER_BUFFER][2];

			errorWrapper(Pa_Initialize());
			outputParameters.device = Pa_GetDefaultOutputDevice();
			outputParameters.channelCount = 2;
			outputParameters.sampleFormat = paFloat32;
			outputParameters.suggestedLatency = 0.050;
			outputParameters.hostApiSpecificStreamInfo = NULL;

			errorWrapper(Pa_OpenStream(&stream, NULL, &outputParameters, SAMPLE_RATE, FRAMES_PER_BUFFER, paClipOff, NULL, NULL));

			errorWrapper(Pa_StartStream(stream));

			while (!stop)
			{
				if (play && songData && location < songData->data.size())
				{
					dataMutex.lock();

					for (int i = 0; i < FRAMES_PER_BUFFER; i++)
					{
						if (location + i < songData->data.size())
						{
							buffer[i][0] = songData->data[location + i].first;
							buffer[i][1] = songData->data[location + i].second;
						}
						else
						{
							buffer[i][0] = 0;
							buffer[i][1] = 0;
						}
					}

					location += FRAMES_PER_BUFFER;

					if (lightShow)
					{
						lightShow->setTime(float(location) / SAMPLE_RATE);
					}
					showPlot->setLocation(location);

					dataMutex.unlock();
					errorWrapper(Pa_WriteStream(stream, buffer, FRAMES_PER_BUFFER));
				}
				else
				{
					for (int i = 0; i < FRAMES_PER_BUFFER; i++)
					{
						buffer[i][0] = 0;
						buffer[i][1] = 0;
					}
					errorWrapper(Pa_WriteStream(stream, buffer, FRAMES_PER_BUFFER));
				}
			}

			errorWrapper(Pa_StopStream(stream));
			errorWrapper(Pa_CloseStream(stream));
		}
		catch (std::runtime_error e)
		{
			GLib::Out << e.what() << "\n";
		}

		Pa_Terminate();
	}

private:
	std::mutex dataMutex;
	int location = 0;

	std::shared_ptr<const SongData> songData;
	std::shared_ptr<const SongPartData> songPartData;
	std::shared_ptr<LightShow> lightShow;
	std::unique_ptr<Hue> hue;
	std::unique_ptr<Razer> razer;

	D2D1_RECT_F songScrubBar = { 120, 10, 120 + 800, 10 + 100 };
	ShowPlot* showPlot;

	std::unique_ptr<std::thread> workerThread;
	bool play = true;
	bool stop = false;
};