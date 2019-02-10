#include "../include/GLib.h"

namespace GLib
{
	void Writer::init(RT * _rt)
	{
		DWriteCreateFactory(DWRITE_FACTORY_TYPE_SHARED, __uuidof(writeFactory), reinterpret_cast<IUnknown **>(&writeFactory));
		rt = _rt;
	}
	IDWriteTextFormat * Writer::get(int size, int weight, std::string font)
	{
		auto tuple = std::make_tuple(size, weight, font);

		if (data.count(tuple) == 0)
		{
			IDWriteTextFormat* newFont;
			auto name = std::wstring(font.begin(), font.end());
			writeFactory->CreateTextFormat(name.c_str(), nullptr, DWRITE_FONT_WEIGHT(weight), DWRITE_FONT_STYLE_NORMAL, DWRITE_FONT_STRETCH_NORMAL, size, L"", &newFont);

			newFont->SetWordWrapping(DWRITE_WORD_WRAPPING_NO_WRAP);
			newFont->SetParagraphAlignment(DWRITE_PARAGRAPH_ALIGNMENT_CENTER);

			data[tuple] = newFont;
		}

		return data.at(tuple);
	}
	void Writer::print(std::string text, ID2D1SolidColorBrush * color, IDWriteTextFormat * font, D2D1_RECT_F place)
	{
		ID2D1Layer *pLayerText = NULL;
		rt->CreateLayer(NULL, &pLayerText);
		rt->PushLayer(D2D1::LayerParameters(place), pLayerText);

		auto t = std::wstring(text.begin(), text.end());
		rt->DrawText(t.c_str(), text.size(), font, place, color);

		rt->PopLayer();
		pLayerText->Release();
	}
}