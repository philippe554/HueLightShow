#pragma once

#include <vector>
#include <memory>
#include <algorithm>
#include <map>

#include "GLib.h"

#include "SongData.h"

#include "Pattern.h"

#include "PatternStatic.h"
#include "PatternFade.h"
#include "PatternGlow.h"
#include "PatternBeat.h"
#include "PatternEnergy.h"
#include "PatternStrobe.h"

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
	LightShow(std::string fileName, std::shared_ptr<const SongData> _songData)
	{
		songData = _songData;

		std::ifstream file;
		file.open(fileName);

		std::string line;
		while (std::getline(file, line))
		{
			parseLine(line);
		}

		GLib::Out << "Show loaded, " << patterns.size() << " patterns.\n";
	}

	void setTime(float t)
	{
		dataMutex.lock();
		time = t;
		dataMutex.unlock();
	}
	void setBoost(float _boost)
	{
		boost = _boost;
	}
	void setForce(bool _force)
	{
		force = _force;
	}

	LightColor getState(int id)
	{
		dataMutex.lock();
		float timeCopy = time;
		dataMutex.unlock();

		LightColor color = LightColor(0, 0, 0);

		for (auto p : patterns)
		{
			if (p->inside(timeCopy))
			{
				if (groups.count(p->getGroup()) > 0)
				{
					std::vector<int>& group = groups.at(p->getGroup());
					if (std::find(group.begin(), group.end(), id) != group.end())
					{
						color = p->get(timeCopy, id);
					}
				}
			}
		}

		if (color.red < 0) color.red = 0;
		if (color.red > 1) color.red = 1;
		color.red = color.red * (1 - boost) + boost;

		if (color.green < 0) color.green = 0;
		if (color.green > 1) color.green = 1;
		color.green = color.green * (1 - boost) + boost;

		if (color.blue < 0) color.blue = 0;
		if (color.blue > 1) color.blue = 1;
		color.blue = color.blue * (1 - boost) + boost;

		if (force && (id == 2 || id == 4))
		{
			color.red = 1;
			color.green = 1;
			color.blue = 1;
		}

		dataMutex.lock();
		lastColors[id] = color;
		dataMutex.unlock();

		return color;
	}
	const LightColor getLastState(int id)
	{
		return lastColors[id];
	}
	const std::vector<int> getDefaultLights()
	{
		return groups[defaultGroup];
	}

private:
	void parseLine(std::string line)
	{
		try
		{
			int comment = line.find("%");
			if (comment != -1)
			{
				line = line.substr(0, comment);
			}

			if (line.find("color") == 0)
			{
				std::vector<std::string> parts = split(line, ' ');
				colors[parts[1]] = hexToColor(parts[2].substr(1, 6));
			}
			else if (line.find("group") == 0)
			{
				std::vector<std::string> parts = split(line, ' ');
				std::string name = parts.at(1).substr(0, parts.at(1).find('('));
				auto lamps = split(getNested(parts.at(1)).at(0), ',');
				groups[name] = {};
				GLib::Out << "Light show group: " << name << ": ";
				for (auto l : lamps)
				{
					groups[name].push_back(std::stoi(l));
					GLib::Out << groups[name].back() << " ";
				}
				
				if (parts.size() >= 3 && parts.at(2) == "default")
				{
					defaultGroup = name;
					GLib::Out << "--> set to default";
				}
				GLib::Out << "\n";
			}
			else if (line.find("song") == 0)
			{
				//TODO
			}
			else
			{
				std::vector<std::string> parts = split(line, ' ');
				if (parts.size() >= 2)
				{
					float begin = strToTime(parts[0]);
					float end = strToTime(parts[1]);

					std::string group = defaultGroup;

					for (int i = 2; i < parts.size(); i++)
					{
						if (parts[i].find("group") == 0) group = getNested(parts[i])[0];

						if (parts[i].find("static") == 0) parseStatic(begin, end, group, getNested(parts[i])[0]);
						if (parts[i].find("fade") == 0) parseFade(begin, end, group, getNested(parts[i])[0]);
						if (parts[i].find("glow") == 0) parseGlow(begin, end, group, getNested(parts[i])[0]);
						if (parts[i].find("beat") == 0) parseBeat(begin, end, group, getNested(parts[i])[0]);
						if (parts[i].find("energy") == 0) parseEnergy(begin, end, group, getNested(parts[i])[0]);
						if (parts[i].find("strobe") == 0) parseStrobe(begin, end, group, getNested(parts[i])[0]);
					}
				}
			}
		}
		catch (std::runtime_error error)
		{
			GLib::Out << "Error parsing: " << line << " : " << error.what() << "\n";
		}
		catch (std::invalid_argument error)
		{
			GLib::Out << "Error parsing: " << line << " : " << error.what() << "\n";
		}
		catch (std::out_of_range error)
		{
			GLib::Out << "Error parsing: " << line << " : " << error.what() << "\n";
		}
	}

	void parseStatic(float begin, float end, std::string group, std::string data)
	{
		patterns.push_back(std::make_shared<PatternStatic>(begin, end, group, dataToColor(data)));
	}
	void parseFade(float begin, float end, std::string group, std::string data)
	{
		std::vector<std::string> parts = split(data, ',');

		if (parts.size() != 2)
		{
			throw std::runtime_error("2 parameters needed");
		}

		LightColor c1 = dataToColor(parts[0]);
		LightColor c2 = dataToColor(parts[1]);

		patterns.push_back(std::make_shared<PatternFade>(begin, end, group, c1, c2));
	}
	void parseGlow(float begin, float end, std::string group, std::string data)
	{
		std::vector<std::string> parts = split(data, ',');

		if (parts.size() != 3)
		{
			throw std::runtime_error("3 parameters needed");
		}

		LightColor c1 = dataToColor(parts[0]);
		LightColor c2 = dataToColor(parts[1]);

		int periods = std::round((end - begin) / std::stof(parts[2]));

		if (periods < 1)
		{
			periods = 1;
		}

		patterns.push_back(std::make_shared<PatternGlow>(begin, end, group, c1, c2, periods));
	}
	void parseBeat(float begin, float end, std::string group, std::string data)
	{
		std::vector<std::string> parts = split(data, ',');

		int skips = parts.size() >= 2 ? std::stoi(parts[1]) : 1;
		int offset = parts.size() >= 3 ? std::stoi(parts[2]) : 0;

		auto pattern = std::make_shared<PatternBeat>(begin, end, group, dataToColor(parts[0]), skips, offset);

		for (int i = 0; i < songData->beat.size(); i++)
		{
			float time = float(songData->beat[i]) / songData->sampleRate;

			if (begin <= time && time <= end)
			{
				pattern->addBeat(time);
			}
		}

		patterns.push_back(pattern);
	}
	void parseEnergy(float begin, float end, std::string group, std::string data)
	{
		auto pattern = std::make_shared<PatternEnergy>(begin, end, group, dataToColor(data));

		float length = float(songData->energyRate) / songData->sampleRate;

		GLib::Out << "offset: " << length << "\n";

		for (int i = 0; i < songData->energy.size(); i++)
		{
			float time = i * length;

			if ((begin - length) <= time && time <= (end + length))
			{
				pattern->addEnergy(time, songData->energy[i]);
			}
		}

		patterns.push_back(pattern);
	}
	void parseStrobe(float begin, float end, std::string group, std::string data)
	{
		patterns.push_back(std::make_shared<PatternStrobe>(begin, end, group, dataToColor(data)));
	}

	std::vector<std::string> split(std::string s, char d)
	{
		std::vector<std::string> data;

		int start = 0;
		int end = 0;
		while (end >= 0)
		{
			end = s.find(d, start);
			data.push_back(s.substr(start, end - start));

			start = end + 1;
		}

		return data;
	}
	float strToTime(std::string s)
	{
		std::vector<std::string> parts = split(s, '.');
		if (parts.size() == 2)
		{
			int min = std::stoi(parts[0]);
			int sec = std::stoi(parts[1]);

			return min * 60 + sec;
		}
		else if (parts.size() == 3)
		{
			int min = std::stoi(parts[0]);
			int sec = std::stoi(parts[1]);
			int ms = std::stoi(parts[2]);

			return (ms / 1000.0f) + min * 60 + sec;
		}
		else
		{
			throw std::runtime_error("Unable to parse time");
		}
	}
	std::vector<std::string> getNested(std::string line)
	{
		std::vector<std::string> nested;

		int begin = 0;
		int end = 0;
		int level = 0;

		for (int i = 0; i < line.size(); i++)
		{
			if (line[i] == '(')
			{
				if (level == 0)
				{
					begin = i;
				}
				level++;
			}
			if (line[i] == ')')
			{
				if (level == 1)
				{
					end = i;
					if (end - begin >= 2)
					{
						nested.push_back(line.substr(begin + 1, end - begin - 1));
					}
				}
				level--;
			}
		}

		if (level != 0)
		{
			throw std::runtime_error("Brackets mismatch");
		}

		return nested;
	}
	double hexToDec(std::string hex)
	{
		int x;
		std::stringstream ss;
		ss << std::hex << hex;
		ss >> x;
		return x;
	}
	LightColor hexToColor(std::string data)
	{
		float multiplier = 1;
		if (data.find("(") != -1)
		{
			std::vector<std::string> parts = getNested(data);
			if (parts.size() == 1)
			{
				multiplier = std::stoi(parts[0]) / 100.0f;
			}

			data = data.substr(0, data.find("("));
		}

		LightColor lightColor;

		lightColor.red = hexToDec(data.substr(0, 2)) * multiplier / 255.0f;
		lightColor.green = hexToDec(data.substr(2, 2)) * multiplier / 255.0f;
		lightColor.blue = hexToDec(data.substr(4, 2)) * multiplier / 255.0f;

		return lightColor;
	}
	LightColor nameToColor(std::string data)
	{
		float multiplier = 1;
		if (data.find("(") != -1)
		{
			std::vector<std::string> parts = getNested(data);
			if (parts.size() == 1)
			{
				multiplier = std::stoi(parts[0]) / 100.0f;
			}

			data = data.substr(0, data.find("("));
		}

		LightColor lightColor;

		if (colors.count(data) > 0)
		{
			lightColor = colors[data];
		}

		lightColor.red *= multiplier;
		lightColor.green *= multiplier;
		lightColor.blue *= multiplier;

		return lightColor;
	}
	LightColor dataToColor(std::string data)
	{
		if (data == "off")
		{
			return LightColor(0, 0, 0);
		}
		else if (data[0] == '#')
		{
			return hexToColor(data.substr(1, data.length() - 1));
		}
		else
		{
			return nameToColor(data);
		}
	}

private:
	std::shared_ptr<const SongData> songData;

	std::map<std::string, std::vector<int>> groups;
	std::string defaultGroup = "all";
	std::map<std::string, LightColor> colors;

	std::vector<std::shared_ptr<Pattern>> patterns;

	float time;
	std::mutex dataMutex;

	std::map<int, LightColor> lastColors;

	float boost = 0.0;
	bool force = false;
};