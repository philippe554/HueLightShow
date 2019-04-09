#pragma once

#include <vector>
#include <memory>
#include <algorithm>
#include <map>

#include "Pattern.h"

/*class LightEffect
{
public:
	LightEffect(float t)
	{
		time = t;
	}

	virtual LightColor getState(float t) const 
	{
		return LightColor(0, 0, 0);
	}

protected:
	float time;
};*/

/*class LightEffectFlash : public LightEffect
{
public:
	LightEffectFlash(float t, LightColor c, float fi = 0.1, float h = 0.1, float fo = 0.5) : LightEffect(t), color(c)
	{
		fadeIn = fi;
		high = h;
		fadeOut = fo;
	}

	LightColor getState(float t) const
	{
		t -= time;

		if (t < -fadeIn)
		{
			return LightColor(0, 0, 0);
		}
		else if (t <= 0)
		{
			float a = (t + fadeIn) / fadeIn;
			float red = std::sqrt(a * color.red * color.red);
			float green = std::sqrt(a * color.green * color.green);
			float blue = std::sqrt(a * color.blue * color.blue);
			return LightColor(red, green, blue);
		}
		else if (t <= high)
		{
			return color;
		}
		else if (t <= high + fadeOut)
		{
			float a = 1 - ((t - high) / fadeOut);
			float red = std::sqrt(a * color.red * color.red);
			float green = std::sqrt(a * color.green * color.green);
			float blue = std::sqrt(a * color.blue * color.blue);
			return LightColor(red, green, blue);
		}
		else
		{
			return LightColor(0, 0, 0);
		}
	}

private:
	LightColor color;
	float fadeIn;
	float high;
	float fadeOut;
};*/

class LightShow
{
public:
	LightShow(std::vector<std::shared_ptr<Pattern>> _patterns)
	{
		patterns = _patterns;
	}
	/*void addEffect(int id, std::shared_ptr<LightEffect> e)
	{
		effects.emplace_back(id, e);
	}*/

	void setTime(float t)
	{
		dataMutex.lock();
		time = t;
		dataMutex.unlock();
	}

	LightColor getState(int id)
	{
		dataMutex.lock();
		float timeCopy = time;
		dataMutex.unlock();

		LightColor color = LightColor(1, 1, 1);

		for (auto p : patterns)
		{
			if (p->inside(timeCopy))
			{
				color = p->get(timeCopy);
			}
		}

		dataMutex.lock();
		lastColors[id] = color;
		dataMutex.unlock();

		return color;
	}

	const std::map<int, LightColor>& getLastColors()
	{
		return lastColors;
	}

	/*LightColor getState(int id)
	{
		dataMutex.lock();
		float timeCopy = time;
		dataMutex.unlock();

		LightColor color(0, 0, 0);
		for (const auto& e : effects)
		{
			if (e.first == id)
			{
				LightColor eColor = e.second->getState(timeCopy);
				color.red = std::max(color.red, eColor.red);
				color.green = std::max(color.green, eColor.green);
				color.blue = std::max(color.blue, eColor.blue);
			}
		}
		return color;
	}*/

	/*int getAmountOfEffects()
	{
		return effects.size();
	}*/

private:
	//std::vector<std::pair<int, std::shared_ptr<LightEffect>>> effects;
	std::vector<std::shared_ptr<Pattern>> patterns;
	float time;
	std::mutex dataMutex;

	std::map<int, LightColor> lastColors;
};