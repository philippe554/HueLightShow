#include "../include/GLib.h"

namespace GLib
{
	View::View(View* parent, int x, int y, int width, int height)
	{
		if (parent != nullptr)
		{
			if (width == -1)
			{
				width = parent->place.right - parent->place.left - x;
			}
			if (height == -1)
			{
				height = parent->place.bottom - parent->place.top - y;
			}
		}
		place = D2D1::RectF(x, y, x + width, y + height);
	}

	View::~View()
	{
		for (auto v : subViews)
		{
			delete v;
		}
	}

	void View::addMouseListener(int type, std::function<void(int, int)> f)
	{
		mouseFunctions[type] = f;
	}

	std::pair<int, int> View::getMousePosition()
	{
		return std::pair<int, int>(mouseX, mouseY);
	}

	void View::renderControl(RT * rt, Writer* w, Color* c, int x, int y, D2D1_RECT_F& visibleRect)
	{
		if (activateFlag && renderFlag)
		{
			D2D1_RECT_F layerRect = { 0, 0, place.right - place.left , place.bottom - place.top };

			D2D1_RECT_F overlap = { max(layerRect.left, visibleRect.left), max(layerRect.top, visibleRect.top),
				min(layerRect.right, visibleRect.right), min(layerRect.bottom, visibleRect.bottom) };

			if (overlap.left < overlap.right && overlap.top < overlap.bottom)
			{
				rt->SetTransform(D2D1::Matrix3x2F::Translation(x, y));
				rt->PushLayer(D2D1::LayerParameters(overlap), nullptr);

				render(rt, w, c, overlap);
				for (auto v : subViews)
				{
					D2D1_RECT_F newVisible = { overlap.left - v->place.left, overlap.top - v->place.top, overlap.right - v->place.left, overlap.bottom - v->place.top };
					v->renderControl(rt, w, c, x + v->place.left, y + v->place.top, newVisible);
				}

				rt->PopLayer();
			}
		}
	}

	void View::updateControl()
	{
		if (activateFlag && updateFlag)
		{
			update();

			for (auto v : subViews)
			{
				v->updateControl();
			}
		}
	}

	void View::winEventControl(Frame * frame, HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
	{
		if (activateFlag && winEventFlag)
		{
			winEvent(frame, hwnd, message, wParam, lParam);

			for (auto v : subViews)
			{
				v->winEventControl(frame, hwnd, message, wParam, lParam);
			}
		}
	}

	void View::mouseEventControl(Frame * frame, HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
	{
		if (message >= WM_MOUSEFIRST && message <= WM_MOUSELAST)
		{
			//Warning: Not all messages in this interval might store cursor position like this

			forwardMouseEvent(message, GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));
		}
		else if (message == WM_MOUSELEAVE || message == WM_CAPTURECHANGED)
		{
			forwardMouseEvent(message, 0, 0);
		}
	}

	View * View::getParentView()
	{
		return parentView;
	}

	void View::forwardMouseEvent(int type, int x, int y)
	{
		if (type == WM_MOUSEMOVE)
		{
			mouseX = x;
			mouseY = y;
		}
		if (activateFlag && mouseEventFlag)
		{
			if (mouseFunctions.count(type) > 0)
			{
				mouseFunctions[type](x, y);
			}

			for (auto v : subViews)
			{
				v->forwardMouseEvent(type, x - v->place.left, y - v->place.top);
			}
		}
	}
}