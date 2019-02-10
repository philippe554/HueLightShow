#include "../include/GLib.h"

namespace GLib
{
	void Button::init(std::function<void()> _onClick, std::string _title)
	{
		int xSize = place.right - place.left;
		int ySize = place.bottom - place.top;

		box = D2D1::RectF(0, 0, xSize, ySize);

		title = _title;
		titleBox = box;
		onClick = _onClick;

		addMouseListener(WM_LBUTTONDOWN, [&](int x, int y)
		{
			if (activated)
			{
				if (box.left <= x && x <= box.right && box.top <= y && y <= box.bottom)
				{
					state = 2;
					horizontalStart = x;
					verticalStart = y;

					if (horizontalDrag || verticalDrag)
					{
						SetCapture(Frame::getHWND());
						isDragging = true;
					}
				}
				else
				{
					state = 0;
					isDragging = false;
				}
			}
		});

		addMouseListener(WM_LBUTTONUP, [&](int x, int y)
		{
			if (activated)
			{
				if (box.left <= x && x <= box.right && box.top <= y && y <= box.bottom)
				{
					onClick();
				}
				state = 0;

				if (isDragging)
				{
					isDragging = false;
					ReleaseCapture();
				}
			}
		});

		addMouseListener(WM_MOUSEMOVE, [&](int x, int y)
		{
			if (activated)
			{
				if (box.left <= x && x <= box.right && box.top <= y && y <= box.bottom)
				{
					if (state == 0)
					{
						state = 1;
					}
				}
				else
				{
					state = 0;
				}
				if (horizontalDrag && isDragging)
				{
					moveHorizontalPlace(x - horizontalStart);
				}
				if (verticalDrag && isDragging)
				{
					moveVerticalPlace(y - verticalStart);
				}
			}
		});

		addMouseListener(WM_MOUSELEAVE, [&](int x, int y)
		{
			state = 0;
			isDragging = false;
		});

		addMouseListener(WM_CAPTURECHANGED, [&](int x, int y)
		{
			isDragging = false;
		});
	}

	void Button::render(RT * rt, Writer * w, Color * c, D2D1_RECT_F& visibleRect)
	{
		if (activated)
		{
			if (state == 0)
			{
				rt->FillRectangle(box, c->get(C::Gray));
			}
			else if (state == 1)
			{
				rt->FillRectangle(box, c->get(C::DarkGray));
			}
			else
			{
				rt->FillRectangle(box, c->get(C::LightSlateGray));
			}
		}
		else
		{
			rt->FillRectangle(box, c->get(C::LightGray));
		}

		w->print(title, c->get(C::Black), w->get(20), titleBox);
	}

	void Button::setHorizontalDragable(int _maxLeft, int _maxRight, std::function<void(float)> _onHorizontalDrag)
	{
		horizontalDrag = true;
		onHorizontalDrag = _onHorizontalDrag;
		maxLeft = _maxLeft;
		maxRight = _maxRight;
	}
	void Button::moveHorizontalPlace(int i)
	{
		if (horizontalDrag)
		{
			place.left += i;
			place.right += i;

			if (place.left < maxLeft)
			{
				int overshoot = maxLeft - place.left;
				place.left += overshoot;
				place.right += overshoot;
			}
			if (maxRight < place.right)
			{
				int overshoot = place.right - maxRight;
				place.left -= overshoot;
				place.right -= overshoot;
			}

			onHorizontalDrag(getHorizontalRatio());
		}
	}
	float Button::getHorizontalRatio()
	{
		return float(place.left - maxLeft) / float(maxRight - maxLeft - (place.right - place.left));
	}

	void Button::setVerticalDragable(int _maxTop, int _maxButton, std::function<void(float)> _onVerticalDrag)
	{
		verticalDrag = true;
		onVerticalDrag = _onVerticalDrag;
		maxTop = _maxTop;
		maxBottom = _maxButton;
	}
	void Button::moveVerticalPlace(int i)
	{
		if (verticalDrag)
		{
			place.top += i;
			place.bottom += i;

			if (place.top < maxTop)
			{
				int overshoot = maxTop - place.top;
				place.top += overshoot;
				place.bottom += overshoot;
			}
			if (maxBottom < place.bottom)
			{
				int overshoot = place.bottom - maxBottom;
				place.top -= overshoot;
				place.bottom -= overshoot;
			}

			onVerticalDrag(getVerticalRatio());
		}
	}
	float Button::getVerticalRatio()
	{
		return float(place.top - maxTop) / float(maxBottom - maxTop - (place.bottom - place.top));
	}
}