#pragma once

#include "GLibMain.h"

using namespace GLib;

class SoundPlot : public View
{
public:
	using View::View;
	void init()
	{
		addMouseListener(WM_MOUSEMOVE, [&](int x, int y)
		{
			mouseOnView = false;

			if (place.left <= x && x <= place.right)
			{
				if (place.top <= y && y <= place.bottom)
				{
					mouseOnView = true;
					mouseXPlace = x;
				}
			}
		});
	}

	void render(RT* rt, Writer* w, Color* c, D2D1_RECT_F& visibleRect) override
	{
		dataMutex.lock();
		/*for (const auto& o : onsets)
		{
			float at = float(o) / amountOfFrames * (place.right - place.left);
			rt->DrawLine({ at, place.top }, { at, place.bottom }, c->get(C::Gray), 2.0f);
		}*/

		for (int i = 0; i < beat.size(); i++)
		{
			if (i + 1 < beat.size() && i < songPart.size())
			{
				float from = float(beat[i]) / amountOfFrames * (place.right - place.left) - 0.5;
				float to = float(beat[i + 1]) / amountOfFrames * (place.right - place.left) + 0.5;

				float middle = (place.bottom - place.top) * 0.5;

				D2D1_RECT_F e = { from, middle - energyPerBeat[i] * 200, to, middle + energyPerBeat[i] * 200 };

				if (songPart[i] == 0)
				{
					rt->FillRectangle(e, c->get(C::Orange));
				}
				if (songPart[i] == 1)
				{
					rt->FillRectangle(e, c->get(C::Green));
				}
				if (songPart[i] == 2)
				{
					rt->FillRectangle(e, c->get(C::Blue));
				}
				if (songPart[i] == 3)
				{
					rt->FillRectangle(e, c->get(C::Yellow));
				}
				if (songPart[i] == 4)
				{
					rt->FillRectangle(e, c->get(C::Purple));
				}
				if (songPart[i] == 5)
				{
					rt->FillRectangle(e, c->get(C::Brown));
				}
				if (songPart[i] == 6)
				{
					rt->FillRectangle(e, c->get(C::Cyan));
				}
				if (songPart[i] == 7)
				{
					rt->FillRectangle(e, c->get(C::Pink));
				}

				rt->DrawLine({ from, place.bottom - beatConfidence[i] * 100 }, { to, place.bottom - beatConfidence[i + 1] * 100 }, c->get(C::Black));
			}
		}

		rt->DrawLine({ 0, place.bottom - 100 }, { place.right, place.bottom - 100 }, c->get(C::Black));

		if ((place.right - place.left) / beat.size() > 10)
		{
			for (int i = 0; i < beat.size(); i++)
			{
				float at = float(beat[i]) / amountOfFrames * (place.right - place.left);
				rt->DrawLine({ at, place.top }, { at, place.bottom }, c->get(C::Black));
			}
		}

		D2D1_RECT_F songDone = { place.left, 50, place.left + (place.right - place.left) * position, 100 };
		rt->FillRectangle(songDone, c->get(C::Red));

		/*int middle = (place.bottom + place.top) * 0.5;
		for (int i = 0; i < energy.size(); i++)
		{
			float from = float(i - 0.1) / energy.size() * (place.right - place.left) - 0.2;
			float to = float(i + 1 + 0.1) / energy.size() * (place.right - place.left) + 0.2;
			D2D1_RECT_F e = { from, middle - energy[i] * 100, to, middle + energy[i] * 100 };

			if (i < energy.size() * position)
			{
				rt->FillRectangle(e, c->get(C::Gray));
			}
			else
			{
				rt->FillRectangle(e, c->get(C::LightGray));
			}
		}*/

		/*if (fft.size() > 0)
		{
			for (int i = 0; i < fft.size() - 1; i++)
			{
				float from = float(i - 0.1) / fft.size() * (place.right - place.left);
				float to = float(i + 1 + 0.1) / fft.size() * (place.right - place.left);
				rt->DrawLine({ from, place.bottom - (fft[i] * 0.1f) }, { to, place.bottom - (fft[i + 1] * 0.1f) }, c->get(C::Black), 2.0f);
			}
		}*/

		dataMutex.unlock();
	}

	void update()
	{

	}

	void winEvent(Frame * frame, HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
	{
		if (message == WM_MOUSEWHEEL)
		{
			if (mouseOnView)
			{
				/*float partLeft = float(mouseXPlace - place.left) / (place.right - place.left);
				float partRight = float(place.right - mouseXPlace) / (place.right - place.left);
				
				int zoom = GET_WHEEL_DELTA_WPARAM(wParam);

				place.right += partRight * zoom;
				place.left -= partLeft * zoom;*/

				place.right += GET_WHEEL_DELTA_WPARAM(wParam);
				resized();
			}
		}
	}

	void resized()
	{
		if ((place.right - place.left) / beat.size() > 25)
		{
			for (int i = 0; i < beat.size(); i++)
			{
				float l = (float(beat[i]) / amountOfFrames) * (place.right - place.left);
				moveBeatButtons[i]->place = { l - 5, 0.0f, l + 5, 20.0f };
				moveBeatButtons[i]->activateFlag = true;

				int dragFrom = (i == 0) ? 0 : (float(beat[i - 1]) / amountOfFrames) * (place.right - place.left) + 5;
				int dragTo = (i == (beat.size() - 1)) ? (place.right - place.left) : (float(beat[i + 1]) / amountOfFrames) * (place.right - place.left) - 5;
				float s = amountOfFrames / (place.right - place.left);
				moveBeatButtons[i]->setHorizontalDragable(dragFrom, dragTo, [dragFrom = dragFrom, dragTo = dragTo, &beat = beat, i = i, s=s](float ratio)
				{
					beat[i] = (dragFrom + 5 + (dragTo - dragFrom - 10) * ratio) * s;
				}
				);
			}
		}
		else
		{
			for (auto b : moveBeatButtons)
			{
				b->activateFlag = false;
			}
		}
	}

	void setPosition(float p)
	{
		position = p;
	}

	void setEnergy(std::vector<float>& e)
	{
		dataMutex.lock();
		energy = e;
		dataMutex.unlock();
	}

	void setFFT(std::vector<float>& f)
	{
		dataMutex.lock();
		fft = f;
		dataMutex.unlock();
	}

	void setOnset(std::vector<int>& o, int frames)
	{
		dataMutex.lock();
		onsets = o;
		amountOfFrames = frames;
		dataMutex.unlock();
	}
	
	void setBeat(std::vector<int>& b, std::vector<float>& bc, int frames)
	{
		dataMutex.lock();
		beat = b;
		beatConfidence = bc;
		amountOfFrames = frames;
		dataMutex.unlock();

		for (int i = 0; i < beat.size(); i++)
		{
			int l = (float(beat[i]) / amountOfFrames) * (place.right - place.left);
			auto button = addView<Button>(l - 5, 0, 10, 20, []() {});
			button->activateFlag = false;
			moveBeatButtons.push_back(button);
		}
	}

	void setSongPart(std::vector<int>& sp, std::vector<float>& e)
	{
		dataMutex.lock();
		songPart = sp;
		energyPerBeat = e;
		dataMutex.unlock();
	}

private:
	bool mouseOnView = false;
	int mouseXPlace = 0;

	float position = 0;
	std::mutex dataMutex;

	std::vector<float> energy;
	std::vector<float> fft;
	std::vector<int> onsets;
	std::vector<int> beat;
	std::vector<float> beatConfidence;
	std::vector<float> energyPerBeat;
	std::vector<int> songPart;

	std::vector<GLib::Button*> moveBeatButtons;
	
	int amountOfFrames = 0;
};
