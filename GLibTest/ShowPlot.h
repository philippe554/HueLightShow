#pragma once

#include "GLib.h"

using namespace GLib;

class ShowPlot : public View
{
public:
	using View::View;
	void init()
	{

	}
	void render(RT* rt, Writer* w, Color* c, D2D1_RECT_F& visibleRect) override
	{
		if (songData && songPartData)
		{
			for (int i = 0; i < songData->beat.size(); i++)
			{
				if (i + 1 < songData->beat.size() && i < songPartData->data.size())
				{
					float from = float(songData->beat[i]) / songData->data.size() * (place.right - place.left) - 0.5;
					float to = float(songData->beat[i + 1]) / songData->data.size() * (place.right - place.left) + 0.5;

					float middle = (place.bottom - place.top) * 0.5;

					//D2D1_RECT_F e = { from, middle - energyPerBeat[i] * 200, to, middle + energyPerBeat[i] * 200 };
					D2D1_RECT_F e = { from, 0, to, place.bottom };

					if (songPartData->data[i] == 0)
					{
						rt->FillRectangle(e, c->get(C::Orange));
					}
					if (songPartData->data[i] == 1)
					{
						rt->FillRectangle(e, c->get(C::Green));
					}
					if (songPartData->data[i] == 2)
					{
						rt->FillRectangle(e, c->get(C::Blue));
					}
					if (songPartData->data[i] == 3)
					{
						rt->FillRectangle(e, c->get(C::Yellow));
					}
					if (songPartData->data[i] == 4)
					{
						rt->FillRectangle(e, c->get(C::Purple));
					}
					if (songPartData->data[i] == 5)
					{
						rt->FillRectangle(e, c->get(C::Brown));
					}
					if (songPartData->data[i] == 6)
					{
						rt->FillRectangle(e, c->get(C::Cyan));
					}
					if (songPartData->data[i] == 7)
					{
						rt->FillRectangle(e, c->get(C::Pink));
					}

					//rt->DrawLine({ from, place.bottom - beatConfidence[i] * 100 }, { to, place.bottom - beatConfidence[i + 1] * 100 }, c->get(C::Black));
				}
			}
		}
	}

	void setSongPartData(std::shared_ptr<const SongData> sd, std::shared_ptr<const SongPartData> spd)
	{
		songData = sd;
		songPartData = spd;
	}
private:
	std::shared_ptr<const SongData> songData;
	std::shared_ptr<const SongPartData> songPartData;
};