#pragma once

#include "GLib.h"

#include "Pattern.h"

class PatternGlow : public Pattern
{
public:
	PatternGlow(float _begin, float _end, std::string _group, LightColor _c1, LightColor _c2, int _periods)
		:Pattern(_begin, _end, _group)
	{
		c1 = _c1;
		c2 = _c2;
		periods = _periods;
	}

	LightColor get(float time, int id)
	{
		float x = (time - begin) / (end - begin);
		float ratio = (std::cos(x * PI * 2.0 * periods) + 1.0f) / 2.0f;

		LightColor l = LightColor();

		l.red = c1.red * ratio + c2.red * (1 - ratio);
		l.green = c1.green * ratio + c2.green * (1 - ratio);
		l.blue = c1.blue * ratio + c2.blue * (1 - ratio);

		return l;
	}

private:
	LightColor c1;
	LightColor c2;

	int periods = 1;
};
