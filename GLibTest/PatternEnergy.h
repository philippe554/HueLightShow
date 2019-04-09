#pragma once

#include <algorithm>

#include "GLib.h"

#include "Pattern.h"

class PatternEnergy : public Pattern
{
public:
	PatternEnergy(float _begin, float _end, std::string _group, LightColor _color)
		:Pattern(_begin, _end, _group)
	{
		color = _color;
	}

	LightColor get(float time)
	{
		for (int i = 0; (i + 1) < energy.size(); i++)
		{
			if (energy[i].first <= time && time <= energy[i + 1].first)
			{
				float ratio = (time - energy[i].first) / (energy[i + 1].first - energy[i].first);
				float e = (energy[i + 1].second * ratio + energy[i].second * (1 - ratio)) / maxEnergy;

				LightColor copy = color;

				copy.red *= e;
				copy.green *= e;
				copy.blue *= e;

				return copy;
			}
		}
		return color;
	}

	void addEnergy(float time, float e)
	{
		energy.emplace_back(time, e);
		std::sort(energy.begin(), energy.end(), [](std::pair<float, float> p1, std::pair<float, float> p2) {return p1.first < p2.first; });

		if (e > maxEnergy)
		{
			maxEnergy = e;
		}
	}

private:
	LightColor color;
	std::vector<std::pair<float, float>> energy;
	float maxEnergy = 0;
};
