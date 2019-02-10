#pragma once

#include <vector>

class SongData
{
public:
	std::vector<std::pair<float, float>> data;
	std::vector<int> beat;
	std::vector<float> beatConfidence;
	int sampleRate;
};

class SongPartData
{
public:
	std::vector<int> data;
};