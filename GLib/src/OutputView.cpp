#include "../include/GLib.h"

namespace GLib
{
	OutputForward Out;

	void OutputView::init()
	{
		text.push_back("");
	}

	void OutputView::render(RT * rt, Writer * w, Color * c, D2D1_RECT_F& visibleRect)
	{
		D2D1_RECT_F background = { 0.0, 0.0, place.right - place.left, place.bottom - place.top };
		rt->FillRectangle(background, c->get(C::Black));

		int start = int(visibleRect.top - yOffset) / fontSize;
		int stop = start + int(place.bottom - place.top) / fontSize + 1;

		int textWidth = place.right - place.left - xOffset;

		for (int i = start; i < min(text.size(), stop); i++)
		{
			D2D1_RECT_F textBox = { float(xOffset), float(i * fontSize + yOffset), float(textWidth), float(i * fontSize + fontSize + yOffset) };
			w->print(text[i], c->get(C::White), w->get(14, 500, "Courier New"), textBox);
		}
	}

	void OutputView::update()
	{
		/*int n = rand() % 1000;
		if (n <= 50)
		{
			std::string spaces = "";
			for (int i = 0; i < n * 2; i++)
			{
				spaces += " ";
			}
			write("Random #: " + std::to_string(n) + spaces + "abcdefghijklmnopqrstuvwxyz\n");
		}*/
	}

	void OutputView::winEvent(Frame * frame, HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
	{
	}

	void OutputView::setDefault()
	{
		Out.outputView = this;
	}

	void OutputView::write(std::string s)
	{
		for (auto c : s)
		{
			if (c == '\n')
			{
				text.push_back("");
			}
			else
			{
				text.back() += c;
			}
		}
		place.bottom = max(place.top + fontSize * text.size() + 2 * yOffset, place.bottom);
	}

	template<> OutputForward & operator<<<const char*>(OutputForward & out, const char* s)
	{
		if (out.outputView != nullptr)
		{
			out.outputView->write(s);
		}
		return out;
	}
	
	template<> OutputForward & operator<<<std::string>(OutputForward & out, std::string s)
	{
		if (out.outputView != nullptr)
		{
			out.outputView->write(s);
		}
		return out;
	}
}