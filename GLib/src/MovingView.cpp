#include "../include/GLib.h"

namespace GLib
{
	void MovingView::init(bool _horizontal, bool _vertical)
	{
		int xSize = place.right - place.left;
		int ySize = place.bottom - place.top;

		horizontal = _horizontal;
		vertical = _vertical;

		if (vertical && !horizontal)
		{
			staticView = addView<View>(0, 0, xSize - buttonSize, ySize);
			movingView = staticView->addView<View>(0, 0, xSize - buttonSize, ySize);
		}
		else if (!vertical && horizontal)
		{
			staticView = addView<View>(0, 0, xSize, ySize - buttonSize);
			movingView = staticView->addView<View>(0, 0, xSize, ySize - buttonSize);
		}
		else
		{
			throw std::runtime_error("Not suported");
		}

		setup(xSize, ySize);
	}

	void MovingView::update()
	{
		float xMax = 0;
		float yMax = 0;
		for (auto v : movingView->subViews)
		{
			if (xMax < v->place.right)
			{
				xMax = v->place.right;
			}
			if (yMax < v->place.bottom)
			{
				yMax = v->place.bottom;
			}
		}

		float xSize = movingView->place.right - movingView->place.left;
		float ySize = movingView->place.bottom - movingView->place.top;

		if (abs(xMax - xSize) > 0.001
			|| abs(yMax - ySize) > 0.001)
		{
			movingView->place.right = movingView->place.left + xMax;
			movingView->place.bottom = movingView->place.top + yMax;
			resize(xMax, yMax);
		}
	}

	void MovingView::winEvent(Frame * frame, HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
	{
		if (message == WM_MOUSEWHEEL && vertical && verticalBar->activated)
		{
			verticalBar->moveVerticalPlace(GET_WHEEL_DELTA_WPARAM(wParam)*-0.2);
		}
	}

	View * MovingView::getMovingView()
	{
		return movingView;
	}

	void MovingView::setup(int xSize, int ySize)
	{
		if (horizontal)
		{
			horizontalBar = addView<Button>(buttonSize + spaceSize, ySize - buttonSize, xSize - 2 * buttonSize - 2 * spaceSize, buttonSize, [&]() {});
			horizontalBar->setHorizontalDragable(buttonSize + spaceSize, xSize - buttonSize - spaceSize, [&](float pos)
			{
				float overshoot = (movingView->place.right - movingView->place.left) - (staticView->place.right - staticView->place.left);
				float viewSize = (movingView->place.right - movingView->place.left);

				movingView->place.left = -pos * overshoot;
				movingView->place.right = -pos * overshoot + viewSize;
			});

			left = addView<Button>(0, ySize - buttonSize, buttonSize, buttonSize, [&]()
			{
				horizontalBar->moveHorizontalPlace(-10);
			});

			right = addView<Button>(xSize - buttonSize, ySize - buttonSize, buttonSize, buttonSize, [&]()
			{
				horizontalBar->moveHorizontalPlace(10);
			});

			horizontalBar->activated = false;
			left->activated = false;
			right->activated = false;
		}

		if (vertical)
		{
			verticalBar = addView<Button>(xSize - buttonSize, buttonSize + spaceSize, buttonSize, ySize - 2 * buttonSize - 2 * spaceSize, [&]() {});
			verticalBar->setVerticalDragable(buttonSize + spaceSize, ySize - 2 * buttonSize - 2 * spaceSize, [&](float pos)
			{
				float overshoot = (movingView->place.bottom - movingView->place.top) - (staticView->place.bottom - staticView->place.top);
				float viewSize = (movingView->place.bottom - movingView->place.top);

				movingView->place.top = -pos * overshoot;
				movingView->place.bottom = -pos * overshoot + viewSize;
			});

			up = addView<Button>(xSize - buttonSize, 0, buttonSize, buttonSize, [&]()
			{
				verticalBar->moveVerticalPlace(-10);
			});

			down = addView<Button>(xSize - buttonSize, ySize - buttonSize, buttonSize, buttonSize, [&]()
			{
				verticalBar->moveVerticalPlace(10);
			});

			verticalBar->activated = false;
			up->activated = false;
			down->activated = false;
		}
	}

	void MovingView::resize(int horizontalSize, int verticalSize)
	{
		int xSize = place.right - place.left;
		int ySize = place.bottom - place.top;

		if (!horizontal && vertical)
		{
			if (verticalSize > ySize)
			{
				float verticalViewRatio = float(ySize) / float(verticalSize);
				int verticalTravelLength = ySize - 2 * buttonSize - 2 * spaceSize;

				int offset = -movingView->place.top / verticalSize * verticalTravelLength;

				verticalBar->place.top = buttonSize + spaceSize + offset;
				verticalBar->place.bottom = buttonSize + spaceSize + verticalViewRatio * verticalTravelLength + offset;
				verticalBar->box.bottom = verticalViewRatio * verticalTravelLength;

				verticalBar->activated = true;
				up->activated = true;
				down->activated = true;
			}
			else
			{
				verticalBar->place.top = buttonSize + spaceSize;
				verticalBar->place.bottom = ySize - buttonSize - spaceSize;
				verticalBar->box.bottom = ySize - 2 * buttonSize - 2 * spaceSize;

				verticalBar->activated = false;
				up->activated = false;
				down->activated = false;
			}
		}
		else if (horizontal && !vertical)
		{
			if (horizontalSize > xSize)
			{
				float horizontalViewRatio = float(xSize) / float(horizontalSize);
				int horizontalTravelLength = xSize - 2 * buttonSize - 2 * spaceSize;

				int offset = -movingView->place.left / horizontalSize * horizontalTravelLength;

				horizontalBar->place.left = buttonSize + spaceSize + offset;
				horizontalBar->place.right = buttonSize + spaceSize + horizontalViewRatio * horizontalTravelLength + offset;
				horizontalBar->box.right = horizontalViewRatio * horizontalTravelLength;

				horizontalBar->activated = true;
				left->activated = true;
				right->activated = true;
			}
			else
			{
				horizontalBar->place.left = buttonSize + spaceSize;
				horizontalBar->place.right = xSize - buttonSize - spaceSize;
				horizontalBar->box.right = xSize - 2 * buttonSize - 2 * spaceSize;

				horizontalBar->activated = false;
				left->activated = false;
				right->activated = false;
			}
		}

		/*int xSize = place.right - place.left;
		int ySize = place.bottom - place.top;

		if (verticalSize > ySize)
		{
			if (horizontalSize > xSize - buttonSize)
			{
				staticView->place.right = xSize - buttonSize;
				staticView->place.bottom = ySize - buttonSize;

				right->place.left = xSize - 2*buttonSize;
				right->place.right = xSize - buttonSize;

				float horizontalViewRatio = float(xSize) / float(horizontalSize);
				int horizonalTravelLength = xSize - 3 * buttonSize - 2 * spaceSize;

				horizontalBar->place.left = buttonSize + spaceSize;
				horizontalBar->place.right = buttonSize + spaceSize + horizontalViewRatio * horizonalTravelLength;
				horizontalBar->box.right = horizontalViewRatio * horizonalTravelLength;
				horizontalBar->place.bottom = buttonSize + spaceSize + horizontalViewRatio * horizonalTravelLength;
				horizontalBar->maxLeft = buttonSize + spaceSize;
				horizontalBar->maxRight = xSize - (2*buttonSize + spaceSize);

				horizontalBar->activateFlag = true;
				left->activateFlag = true;
				right->activateFlag = true;

				float verticalViewRatio = float(ySize) / float(verticalSize);
				int verticalTravelLength = ySize - 2 * buttonSize - 2 * spaceSize;

				verticalBar->place.top = buttonSize + spaceSize;
				verticalBar->place.bottom = buttonSize + spaceSize + verticalViewRatio * verticalTravelLength;
				verticalBar->box.bottom = verticalViewRatio * verticalTravelLength;
				verticalBar->place.bottom = buttonSize + spaceSize + verticalViewRatio * verticalTravelLength;
				verticalBar->maxTop = buttonSize + spaceSize;
				verticalBar->maxBottom = ySize - (buttonSize + spaceSize);

				verticalBar->activateFlag = true;
				up->activateFlag = true;
				down->activateFlag = true;
			}
			else
			{
				staticView->place.right = xSize - buttonSize;
				staticView->place.bottom = ySize;

				horizontalBar->activateFlag = false;
				left->activateFlag = false;
				right->activateFlag = false;

				float verticalViewRatio = float(ySize) / float(verticalSize);
				int verticalTravelLength = ySize - 2 * buttonSize - 2 * spaceSize;

				int offset = -movingView->place.top / verticalSize * verticalTravelLength;

				verticalBar->place.top = buttonSize + spaceSize + offset;
				verticalBar->place.bottom = buttonSize + spaceSize + verticalViewRatio * verticalTravelLength + offset;
				verticalBar->box.bottom = verticalViewRatio * verticalTravelLength;
				verticalBar->maxTop = buttonSize + spaceSize;
				verticalBar->maxBottom = ySize - (buttonSize + spaceSize);

				verticalBar->activateFlag = true;
				up->activateFlag = true;
				down->activateFlag = true;
			}
		}
		else
		{
			if (horizontalSize > xSize)
			{
				staticView->place.right = xSize;
				staticView->place.bottom = ySize - buttonSize;

				right->place.left = xSize - buttonSize;
				right->place.right = xSize;

				float horizontalViewRatio = float(xSize) / float(horizontalSize);
				int horizonalTravelLength = xSize - 2 * buttonSize - 2 * spaceSize;

				horizontalBar->place.left = buttonSize + spaceSize;
				horizontalBar->place.right = buttonSize + spaceSize + horizontalViewRatio * horizonalTravelLength;
				horizontalBar->box.right = horizontalViewRatio * horizonalTravelLength;
				horizontalBar->place.bottom = buttonSize + spaceSize + horizontalViewRatio * horizonalTravelLength;
				horizontalBar->maxLeft = buttonSize + spaceSize;
				horizontalBar->maxRight = xSize - (buttonSize + spaceSize);

				horizontalBar->activateFlag = true;
				left->activateFlag = true;
				right->activateFlag = true;

				verticalBar->activateFlag = false;
				up->activateFlag = false;
				down->activateFlag = false;
			}
			else
			{
				staticView->place.right = xSize;
				staticView->place.bottom = ySize;

				horizontalBar->activateFlag = false;
				left->activateFlag = false;
				right->activateFlag = false;

				verticalBar->activateFlag = false;
				up->activateFlag = false;
				down->activateFlag = false;
			}
		}*/
	}
}