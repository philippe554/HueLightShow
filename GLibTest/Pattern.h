#pragma once

#include <string>
#include <sstream>

#define PI 3.14159265358979323846

struct LightColor
{
public:
	LightColor()
	{
		red = 1;
		green = 1;
		blue = 1;
	}

	LightColor(const LightColor& other)
	{
		red = other.red;
		green = other.green;
		blue = other.blue;
	}

	LightColor(float r, float g, float b)
	{
		red = r;
		green = g;
		blue = b;
	}

	float red;
	float green;
	float blue;
};

class Pattern
{
public:
	Pattern(float _begin, float _end, std::string _group)
	{
		begin = _begin;
		end = _end;
		group = _group;
	}
	virtual ~Pattern()
	{

	}

	bool inside(float time)
	{
		return begin <= time && time <= end;
	}

	std::string& getGroup()
	{
		return group;
	}

	virtual LightColor get(float time, int id) = 0;

protected:
	float begin;
	float end;
	std::string group;
};