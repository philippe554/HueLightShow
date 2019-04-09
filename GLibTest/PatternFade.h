#pragma once

#include "GLib.h"

#include "Pattern.h"

class PatternFade : public Pattern
{
public:
	PatternFade(float _begin, float _end, std::string _group, LightColor _c1, LightColor _c2)
		:Pattern(_begin, _end, _group)
	{
		c1 = _c1;
		c2 = _c2;
	}

	LightColor get(float time)
	{
		float ratio = (time - begin) / (end - begin);

		LightColor l = LightColor();

		l.red = c2.red * ratio + c1.red * (1 - ratio);
		l.green = c2.green * ratio + c1.green * (1 - ratio);
		l.blue = c2.blue * ratio + c1.blue * (1 - ratio);

		return l;
	}

private:
	LightColor c1;
	LightColor c2;
};
