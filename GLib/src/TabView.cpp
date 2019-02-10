#include "../include/GLib.h"

namespace GLib
{
	View* TabView::getNewTab(std::string name)
	{
		View* e = addView<View>(0, 50);
		
		tabs.push_back(e);

		addView<Button>(int (10 + (tabs.size() - 1) * 110), 10, 100, 30, [e = e, &tabs = tabs, &currentTab = currentTab, index = tabs.size() - 1]()
		{
			for (auto tab : tabs)
			{
				tab->renderFlag = false;
			}
			e->renderFlag = true;

			currentTab = index;
		}, name);

		for (auto tab : tabs)
		{
			tab->renderFlag = false;
		}
		e->renderFlag = true;

		currentTab = tabs.size() - 1;

		return e;
	}

	void TabView::render(RT * rt, Writer * w, Color * c, D2D1_RECT_F& visibleRect)
	{
		D2D1_RECT_F horizontal = { place.left, 45, place.right, 50 };
		rt->FillRectangle(horizontal, c->get(C::DarkGray));

		D2D1_RECT_F selected = { 5 + currentTab * 110, 5, 115 + currentTab * 110, 45 };
		rt->FillRectangle(selected, c->get(C::DarkGray));
	}
}