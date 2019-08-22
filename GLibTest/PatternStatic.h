#pragma once

#include "GLib.h"

#include "Pattern.h"

class PatternStatic : public Pattern
{
public:
	PatternStatic(float _begin, float _end, std::string _group, LightColor _color)
		:Pattern(_begin, _end, _group)
	{
		color = _color;
	}

	LightColor get(float time, int id)
	{
		return color;
	}

private:
	LightColor color;
};