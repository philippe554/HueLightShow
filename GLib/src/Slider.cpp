#include "../include/GLib.h"

namespace GLib
{
	void Slider::init(std::function<void(float ratio)> _onClick, float default)
	{
		onClick = _onClick;

		int xSize = place.right - place.left;
		int ySize = place.bottom - place.top;

		box1 = D2D1::RectF(ySize + ySize/2, ySize / 2 - 5, xSize - ySize - ySize/2, ySize / 2 + 5);
		box2 = D2D1::RectF(ySize, ySize / 2 - 10, xSize - ySize, ySize / 2 + 10);
		
		float loc = box1.left + default * (box1.right - box1.left);
		bar = addView<Button>(int(loc - ySize / 4), 0, ySize / 2, ySize, [&]() {}, "");
		bar->setHorizontalDragable(box1.left - ySize / 4, box1.right + ySize / 4, [&](float ratio)
		{
			onClick(ratio);
		});
		
		left = addView<Button>(0, 0, ySize, ySize, [&]()
		{
			bar->moveHorizontalPlace(-10);
			onClick(bar->getHorizontalRatio());
		}, "");
		right = addView<Button>(xSize - ySize, 0, ySize, ySize, [&]()
		{
			bar->moveHorizontalPlace(10);
			onClick(bar->getHorizontalRatio());
		}, "");
	}

	void Slider::render(RT * rt, Writer * w, Color * c, D2D1_RECT_F& visibleRect)
	{
		rt->FillRectangle(box2, c->get(C::LightGray));
		rt->FillRectangle(box1, c->get(C::Gray));
	}

	float Slider::getRatio()
	{
		return bar->getHorizontalRatio();
	}
}