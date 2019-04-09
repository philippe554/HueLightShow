#pragma once

#include <algorithm>

#include "GLib.h"

#include "Pattern.h"

class PatternStrobe : public Pattern
{
public:
	PatternStrobe(float _begin, float _end, std::string _group, LightColor _color)
		:Pattern(_begin, _end, _group)
	{
		color = _color;
	}

	LightColor get(float time)
	{
		if (last)
		{
			last = false;
			return LightColor(0, 0, 0);
		}
		else
		{
			last = true;
			return color;
		}
	}

private:
	LightColor color;
	bool last = false;
};
