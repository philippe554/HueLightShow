#include "../include/GLib.h"

namespace GLib
{
	View* TabView::getNewTab(std::string name)
	{
		View* e = addView<View>(0, 50);
		
		tabs.push_back(e);

		addView<Button>(int (10 + (tabs.size() - 1) * 130), 10, 120, 30, [e = e, &tabs = tabs, &currentTab = currentTab, index = tabs.size() - 1]()
		{
			for (auto tab : tabs)
			{
				tab->renderFlag = false;
				tab->mouseEventFlag = false;
				tab->winEventFlag = false;
			}
			e->renderFlag = true;
			e->mouseEventFlag = true;
			e->winEventFlag = true;

			currentTab = index;
		}, name);

		if (currentTab == -1)
		{
			currentTab = 0;
		}
		else
		{
			e->renderFlag = false;
			e->mouseEventFlag = false;
			e->winEventFlag = false;
		}

		return e;
	}

	void TabView::render(RT * rt, Writer * w, Color * c, D2D1_RECT_F& visibleRect)
	{
		D2D1_RECT_F horizontal = { place.left, 45, place.right, 50 };
		rt->FillRectangle(horizontal, c->get(C::DarkGray));

		D2D1_RECT_F selected = { 5 + currentTab * 130, 5, 135 + currentTab * 130, 45 };
		rt->FillRectangle(selected, c->get(C::DarkGray));
	}
}