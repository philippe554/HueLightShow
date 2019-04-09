#pragma once

#include "GLib.h"

#include "Pattern.h"

class PatternBeat : public Pattern
{
public:
	PatternBeat(float _begin, float _end, std::string _group, LightColor _color, int _skip)
		:Pattern(_begin, _end, _group)
	{
		color = _color;
		skip = _skip;
	}

	LightColor get(float time)
	{
		float minimum = 100;

		for (int i = 0; i < beats.size(); i++)
		{
			if (i % skip == 0)
			{
				float diff = std::abs(time - beats[i]);
				if (diff < minimum)
				{
					minimum = diff;
				}
			}
		}

		float peak = 0.5 + 0.5 * std::exp(-1 * (minimum * minimum) / (2 * 0.1 * 0.1));

		LightColor copy = color;

		copy.red *= peak;
		copy.green *= peak;
		copy.blue *= peak;

		return copy;
	}

	void addBeat(float time)
	{
		beats.push_back(time);
	}

private:
	LightColor color;
	int skip;
	std::vector<float> beats;
};
