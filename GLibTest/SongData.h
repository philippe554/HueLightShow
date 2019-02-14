#pragma once

#include <vector>

class SongData
{
public:
	std::vector<std::pair<float, float>> data;
	std::vector<int> beat;
	std::vector<float> beatConfidence;
	std::vector<float> beatEnergy;
	int sampleRate;
};

class SongPartData
{
public:
	std::vector<int> dataL1;
	std::vector<int> dataL2;
};