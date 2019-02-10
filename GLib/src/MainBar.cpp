#include "../include/GLib.h"

namespace GLib
{
	void MainBar::init(std::string _title)
	{
		int xSize = place.right - place.left;
		int ySize = place.bottom - place.top;

		background = D2D1::RectF(0, 0, xSize, ySize);

		title = _title;
		titleBox = D2D1::RectF(ySize / 2, 0, xSize, ySize);

		int border = ySize*0.1;

		closeButton = addView<Button>(xSize - ySize + border, border, ySize - 2 * border, ySize - 2 * border, []()
		{
			Frame::closeWindow();
		});

		addMouseListener(WM_LBUTTONDOWN, [&](int x, int y)
		{
			if (background.left <= x && x <= background.right && background.top <= y && y <= background.bottom)
			{
				if (x < closeButton->place.left || x > closeButton->place.right || y < closeButton->place.top || y > closeButton->place.bottom)
				{
					move = true;
					movePoint = { x,y };
					SetCapture(Frame::getHWND());
				}
			}
		});

		addMouseListener(WM_LBUTTONUP, [&](int x, int y)
		{
			move = false;
			ReleaseCapture();
		});

		addMouseListener(WM_MOUSEMOVE, [&](int x, int y)
		{
			if (move)
			{
				POINT p;
				RECT  pr;
				GetCursorPos(&p);
				GetWindowRect(Frame::getHWND(), &pr);
				MoveWindow(Frame::getHWND(), p.x - movePoint.x, p.y - movePoint.y, pr.right - pr.left, pr.bottom - pr.top, false);
			}
		});

		addMouseListener(WM_MOUSELEAVE, [&](int x, int y)
		{
			move = false;
		});

		addMouseListener(WM_CAPTURECHANGED, [&](int x, int y)
		{
			move = false;
		});
	}

	void MainBar::render(RT* rt, Writer* w, Color* c, D2D1_RECT_F& visibleRect)
	{
		rt->FillRectangle(background, c->get(C::LightGray));

		w->print(title, c->get(C::Black), w->get(30), titleBox);
	}
}