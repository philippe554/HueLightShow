#include "../include/GLib.h"

namespace GLib
{
	void Color::init(ID2D1HwndRenderTarget* rt)
	{
		renderTarget = rt;
	}

	ID2D1SolidColorBrush * Color::get(int i)
	{
		if (data.count(i) == 0)
		{
			ID2D1SolidColorBrush* newBrush;
			renderTarget->CreateSolidColorBrush(D2D1::ColorF(i), &newBrush);
			data[i] = newBrush;
		}

		return data.at(i);
	}

	ID2D1SolidColorBrush * Color::get(int r, int g, int b)
	{
		return get(256 * 256 * r + 256 * g + b);
	}
}