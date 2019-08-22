#include "../include/GLib.h"

namespace GLib
{
	void CheckBox::init(std::function<void(bool)> _onClick, bool default)
	{
		state = default;
		onClick = _onClick;

		int xSize = place.right - place.left;
		int ySize = place.bottom - place.top;

		box1 = D2D1::RectF(0, 0, xSize, ySize);
		box2 = D2D1::RectF(5, 5, xSize - 5, ySize - 5);
		box3 = D2D1::RectF(10, 10, xSize - 10, ySize - 10);

		addMouseListener(WM_LBUTTONUP, [&](int x, int y)
		{
			if (activated)
			{
				if (box2.left <= x && x <= box2.right && box2.top <= y && y <= box2.bottom)
				{
					state = !state;
					onClick(state);
				}
			}
		});
	}

	void CheckBox::render(RT * rt, Writer * w, Color * c, D2D1_RECT_F& visibleRect)
	{
		if (activated)
		{
			rt->FillRectangle(box1, c->get(C::Gray));
			rt->FillRectangle(box2, c->get(C::White));
			if (state)
			{
				rt->FillRectangle(box3, c->get(C::DarkGray));
			}
		}
		else
		{
			rt->FillRectangle(box1, c->get(C::LightGray));
			rt->FillRectangle(box2, c->get(C::White));
		}
	}

	bool CheckBox::getState()
	{
		return state;
	}
}