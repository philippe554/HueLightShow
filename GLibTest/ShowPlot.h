#pragma once

#include "GLib.h"

using namespace GLib;

#include "SongData.h"

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
			std::vector<ID2D1SolidColorBrush *> colors = 
			{
				c->get(C::Red),
				c->get(C::Green),
				c->get(C::Blue),
				c->get(C::Yellow),
				c->get(C::Magenta),
				c->get(C::Cyan),
			};

			for (int i = 0; i < songData->beat.size(); i++)
			{
				if (i + 1 < songData->beat.size() && i < songPartData->dataL1.size())
				{
					float from = float(songData->beat[i]) / songData->data.size() * (place.right - place.left);
					float to = float(songData->beat[i + 1]) / songData->data.size() * (place.right - place.left) + 0.5;

					float middle = (place.bottom - place.top) * 0.5;

					D2D1_RECT_F e = { from, 0, to, middle };
					rt->FillRectangle(e, colors[songPartData->dataL1[i]]);

					e = { from, middle, to, place.bottom - place.top };
					rt->FillRectangle(e, colors[songPartData->dataL2[i]]);

					e = { from, middle - songData->beatEnergy[i] * 50, to, middle + songData->beatEnergy[i] * 50 };
					rt->FillRectangle(e, c->get(C::Gray));

					rt->DrawLine({ from, place.bottom - songData->beatConfidence[i] * 100 }, { to, place.bottom - songData->beatConfidence[i + 1] * 100 }, c->get(C::Black));
				}
			}

			float at = float(location) / songData->data.size() * (place.right - place.left);
			rt->DrawLine({ at, 0}, { at, place.bottom - place.top}, c->get(C::Black));
		}
	}

	void setSongPartData(std::shared_ptr<const SongData> sd, std::shared_ptr<const SongPartData> spd)
	{
		songData = sd;
		songPartData = spd;
	}

	void parentResized(D2D1_RECT_F p) override
	{
		place.right = place.left + (p.right - p.left);
	}

	void setLocation(int l)
	{
		location = l;
	}

private:
	std::shared_ptr<const SongData> songData;
	std::shared_ptr<const SongPartData> songPartData;
	int location = 0;
};