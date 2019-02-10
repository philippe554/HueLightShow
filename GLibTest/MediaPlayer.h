#pragma once

#include <mutex>
#include <thread>
#include <iostream>
#include <vector>
#include <algorithm>
#include <numeric>
#include <memory>
#include <random>

#include "dlib/clustering.h"

#include "GLibMain.h"
#include "Hue.h"
#include "SoundPlot.h"

#include "portaudio.h"
#include "mpg123.h"
#include "fftw3.h"

#define PI 3.14159265359

namespace Aubio
{
	#include "aubio/aubio.h"
};

#define SAMPLE_RATE         (44100)
#define FRAMES_PER_BUFFER   (1024)

std::vector<LightColor> niceColors =
{
	LightColor(1, 0, 0),
	LightColor(0, 1, 0),
	LightColor(0, 0, 1),
	LightColor(1, 1, 0),
	LightColor(1, 0, 1),
	LightColor(0, 1, 1)
};

using namespace GLib;

/*std::vector<float> getHann(int size)
{
	std::vector<float> data(size);

	std::generate(data.begin(), data.end(), [size, i = 0]() mutable {return 0.5 * (1 - cos((2 * PI * i++) / (size - 1))); });
	float sum = std::accumulate(data.begin(), data.end(), 0.0);
	std::transform(data.begin(), data.end(), data.begin(), [sum](float e) { return e / sum; });

	return data;
}

std::map<int, float> getGaussian(int size, float std)
{
	std::map<int, float> g;
	for (int i = -size; i <= size; i++)
	{
		g[i] = 1.0 / (std::sqrt(2.0 * PI * std::pow(std, 2.0))) * std::pow(std::exp(1), - std::pow(i, 2.0) / (2.0 * std::pow(std, 2.0)));
	}
	return g;
}

std::vector<std::pair<float, float>> getMusic()
{
	std::vector<std::pair<float, float>> data;

	int err;
	mpg123_init();
	mpg123_handle* mh = mpg123_new(NULL, &err);

	size_t buffer_size = mpg123_outblock(mh);
	size_t done;
	unsigned char* buffer = new unsigned char[buffer_size];

	//mpg123_open(mh, "C:\\dev\\data\\avicii.mp3");
	//mpg123_open(mh, "C:\\dev\\data\\waiting.mp3");
	//mpg123_open(mh, "C:\\dev\\data\\dondiablo.mp3");
	mpg123_open(mh, "C:\\dev\\data\\spons.mp3");
	//mpg123_open(mh, "C:\\dev\\data\\deepmix1.mp3");

	int channels;
	int encoding;
	long rate;
	mpg123_getformat(mh, &rate, &channels, &encoding);

	while (mpg123_read(mh, buffer, buffer_size, &done) == MPG123_OK)
	{
		short* tst = reinterpret_cast<short*>(buffer);

		for (auto i = 0; i < done / 2; i+=2) 
		{
			data.emplace_back(tst[i] / (float)SHRT_MAX, tst[i+1] / (float)SHRT_MAX);
		}
	}

	mpg123_close(mh);
	mpg123_delete(mh);
	mpg123_exit();

	delete[] buffer;

	return data;
}

std::vector<float> calculateEnergy(const std::vector<std::pair<float, float>>& data)
{
	int steps = 2000;
	int stepsSize = data.size() / steps;

	std::vector<float> energy;

	for (int i = 0; i < steps; i++)
	{
		double squareSum = 0;
		for (int j = 0; j < stepsSize; j++)
		{
			squareSum += 0.5 * (data[i * stepsSize + j].first * data[i * stepsSize + j].first 
				+ data[i * stepsSize + j].second * data[i * stepsSize + j].second);
		}
		energy.push_back(std::sqrt(squareSum / stepsSize));
	}

	return energy;
}

std::vector<int> calculateOnset(const std::vector<std::pair<float, float>>& data)
{
	std::vector<int> onsetData;
	int read = 0;
	int sampleRate = SAMPLE_RATE;
	int windowSize = 1024;
	int hopSize = windowSize / 4;

	Aubio::aubio_onset_t* o = Aubio::new_aubio_onset("specdiff", windowSize, hopSize, sampleRate);

	Aubio::fvec_t * in = Aubio::new_fvec(hopSize);
	Aubio::fvec_t * out = Aubio::new_fvec(2);

	for(int i = 0; i < data.size() - hopSize; i += hopSize)
	{
		for (int j = 0; j < hopSize; j++)
		{
			in->data[j] = data[i + j].first;
		}

		Aubio::aubio_onset_do(o, in, out);

		if (out->data[0] != 0) 
		{
			float offset = Aubio::aubio_onset_get_last(o);
			int onset = i + int(offset * hopSize) - Aubio::aubio_onset_get_delay(o);
			//GLib::Out << "onset at frame " << offset << "\n";
			onsetData.push_back(offset);
		}

	} while (read == hopSize);

	Aubio::del_aubio_onset(o);
	Aubio::del_fvec(in);
	Aubio::del_fvec(out);

	return onsetData;
}

std::pair<std::vector<int>, std::vector<float>> calculateBeat(const std::vector<std::pair<float, float>>& data)
{
	std::pair<std::vector<int>, std::vector<float>> beatData;
	int read = 0;
	int sampleRate = SAMPLE_RATE;
	int windowSize = 1024;
	int hopSize = windowSize / 4;

	Aubio::aubio_tempo_t* o = Aubio::new_aubio_tempo("default", windowSize, hopSize, sampleRate);

	Aubio::fvec_t * in = Aubio::new_fvec(hopSize);
	Aubio::fvec_t * out = Aubio::new_fvec(1);

	for (int i = 0; i < data.size() - hopSize; i += hopSize)
	{
		for (int j = 0; j < hopSize; j++)
		{
			in->data[j] = data[i + j].first;
		}

		Aubio::aubio_tempo_do(o, in, out);

		if (out->data[0] != 0)
		{
			beatData.first.push_back(Aubio::aubio_tempo_get_last(o));
			beatData.second.push_back(Aubio::aubio_tempo_get_confidence(o));
		}

	} while (read == hopSize);

	Aubio::del_aubio_tempo(o);
	Aubio::del_fvec(in);
	Aubio::del_fvec(out);

	return beatData;
}

std::vector<float> calculateEnergyPerBeat(const std::vector<int>& beat, const std::vector<std::pair<float, float>>& data)
{
	std::vector<float> energy;

	for (int j = 0; j < beat.size() - 1; j++)
	{
		int from = beat[j];
		int to = beat[j + 1];

		if (to < data.size())
		{
			double squareSum = 0;
			for (int i = from; i < to; i++)
			{
				squareSum += 0.5 * (data[i].first * data[i].first + data[i].second * data[i].second);
			}
			energy.push_back(std::sqrt(squareSum / (to - from)));
		}
	}

	return energy;
}

std::vector<std::pair<int, int>> getLogGroups(int size, int amountOfGroups)
{
	std::vector<std::pair<int, int>> groups;

	for (int i = 0; i < amountOfGroups; i++)
	{
		int to = size * std::pow(0.5, i) - 1;
		int from = size * std::pow(0.5, i + 1);
		if (from > to)
		{
			throw std::runtime_error("Too many groups");
		}
		groups.insert(groups.begin(), std::make_pair(from, to));
	}

	groups[0].first = 0;
	return groups;
}

std::vector<int> calculateSongPart(const std::vector<std::pair<float, float>>& song, const std::vector<int>& beat, int amountOfClusters)
{
	const int amountOfGroups = 12;
	typedef dlib::matrix<float, 12, 1> SampleType;
	typedef dlib::radial_basis_kernel<SampleType> KernelType;

	std::vector<int> songPart; // = { 0 };
	std::vector<SampleType> fftData;

	int inputSize = SAMPLE_RATE;
	int outputSize = inputSize / 2 + 1;

	std::vector<std::pair<int, int>> groups = getLogGroups(outputSize, amountOfGroups);
	for (auto g : groups)
	{
		GLib::Out << "(" << g.first << "-" << g.second << ") ";
	}
	GLib::Out << "\n";

	std::vector<float> hann = getHann(inputSize);

	double* inputBuffer = fftw_alloc_real(inputSize * sizeof(double));
	fftw_complex* outputBuffer = fftw_alloc_complex(outputSize * sizeof(fftw_complex));
	fftw_plan plan = fftw_plan_dft_r2c_1d(inputSize, inputBuffer, outputBuffer, FFTW_ESTIMATE);

	std::vector<float> e = calculateEnergyPerBeat(beat, song);

	for (int i = 0; i < beat.size() - 1; i++)
	{
		int middle = beat[i]; // beat[i + 1] - beat[i];
		//if (middle - inputSize / 2 >= 0 && middle + inputSize / 2 < song.size())
		{
			for (int j = 0; j < inputSize; j++)
			{
				inputBuffer[j] = (song[middle - inputSize / 2 + j].first + song[middle - inputSize / 2 + j].second) * 0.5 * hann[j];
			}

			fftw_execute(plan);

			SampleType sample;
			for (int j = 0; j < 11; j++)
			{
				float sum = 0;
				for (int k = groups[j + 1].first; k <= groups[j + 1].second; k++)
				{
					sum += sqrt(outputBuffer[k][0] * outputBuffer[k][0] + outputBuffer[k][1] * outputBuffer[k][1]);
				}
				sample(j) = (sum / (groups[j + 1].second - groups[j + 1].first + 1));
			}
			sample(11) = e[i];

			fftData.push_back(sample);
		}
	}

	dlib::kcentroid<KernelType> kc(KernelType(0.1), 0.001, 8);
	dlib::kkmeans<KernelType> test(kc);
	std::vector<SampleType> initial_centers;
	test.set_number_of_centers(amountOfClusters);
	dlib::pick_initial_centers(amountOfClusters, initial_centers, fftData, test.get_kernel());
	test.train(fftData, initial_centers);

	for (int i = 0; i < beat.size() - 1; i++)
	{
		songPart.push_back(test(fftData[i]));
	}

	fftw_free(inputBuffer);
	fftw_free(outputBuffer);
	fftw_destroy_plan(plan);

	return songPart;
}

std::vector<int> cleanSongPart(std::vector<int> data, int amountOfSongParts)
{
	std::vector<int> newSet;
	std::map<int, float> g = getGaussian(20, 4);

	for (int i = 0; i < data.size(); i++)
	{
		float max = 0;
		int maxIndex = 0;

		for (int j = 0; j < amountOfSongParts; j++)
		{
			float v = 0;
			for (int k = -10; k <= 10; k++)
			{
				if (i + k >= 0 && i + k < data.size())
				{
					if (data[i + k] == j)
					{
						v += g[k];
					}
				}
			}
			if (v > max)
			{
				max = v;
				maxIndex = j;
			}
		}

		newSet.push_back(maxIndex);
	}

	return newSet;
}

std::vector<float> calculateFFT(const std::vector<std::pair<float, float>>& data)
{
	int inputSize = 4000;
	int outputSize = inputSize / 2 + 1;

	double time = double(inputSize) / SAMPLE_RATE;

	double* inputBuffer = fftw_alloc_real(inputSize * sizeof(double));
	fftw_complex* outputBuffer = fftw_alloc_complex(outputSize * sizeof(fftw_complex));
	fftw_plan plan = fftw_plan_dft_r2c_1d(inputSize, inputBuffer, outputBuffer, FFTW_ESTIMATE);

	std::vector<float> fft;

	for (int i = 0; i < data.size() - inputSize; i += inputSize)
	{
		for (int j = 0; j < inputSize; j++)
		{
			inputBuffer[j] = 0.5 * (data[i + j].first + data[i + j].second);
		}
		fftw_execute(plan);
		
		float maxA = 0;
		for (int i = std::round(60 * time); i < std::round(100 * time); i++)
		{
			float a = sqrt(outputBuffer[i][0] * outputBuffer[i][0] + outputBuffer[i][1] * outputBuffer[i][1]);
			if (a > maxA)
			{
				maxA = a;
			}
		}
		
		fft.push_back(maxA);
	}

	fftw_free(inputBuffer);
	fftw_free(outputBuffer);
	fftw_destroy_plan(plan);

	return fft;
}

std::vector<float> calculateAvgEnergyPerSongPart(const std::vector<float>& energyPerBeat, const std::vector<int>& songPart, int amountOfClusters)
{
	std::vector<float> avg;
	std::vector<int> amount;
	for (int i = 0; i < amountOfClusters; i++)
	{
		avg.push_back(0);
		amount.push_back(0);
	}

	for (int i = 0; i < songPart.size(); i++)
	{
		avg[songPart[i]] += energyPerBeat[i];
		amount[songPart[i]]++;
	}

	for (int i = 0; i < amountOfClusters; i++)
	{
		avg[i] /= amount[i];
	}

	return avg;
}
*/
void errorWrapper(PaError error)
{
	if (error != paNoError)
	{
		throw std::runtime_error("Port Audio error " + std::to_string(error) + ":" + Pa_GetErrorText(error));
	}
}

class MediaPlayer : public View
{
public:
	using View::View;
	void init()
	{
		workerThread = std::make_unique<std::thread>(&MediaPlayer::workerFunction, this);

		addView<Button>(10, 10, 100, 50, [this]()
		{
			this->play = !this->play;
		}, "Play");

		addMouseListener(WM_LBUTTONDOWN, [&](int x, int y)
		{
			if (songScrubBar.left <= x && x <= songScrubBar.right && songScrubBar.top <= y && y <= songScrubBar.bottom)
			{
				location = float(x - songScrubBar.left) / float(songScrubBar.right - songScrubBar.left) * data.size();
			}
		});

		View* movingSoundPlot = addView<GLib::MovingView>(0, 300, -1, 200, true, false)->getMovingView();
		soundPlot = movingSoundPlot->addView<SoundPlot>();

		hue = std::make_unique<Hue>("192.168.178.12", "Gl4AsrYSHlDx3mBxFDDfIvVxEFH22dVKxZ6xXfcr", "496C432C6171944976E29F6790408ADB");
		hue->startEntertainmentMode(hue->getFirstEntertainmentGroup());
	}

	~MediaPlayer()
	{
		stop = true;
		workerThread->join();
		workerThread.reset();
	}

	void render(RT* rt, Writer* w, Color* c, D2D1_RECT_F& visibleRect) override
	{
		rt->FillRectangle(songScrubBar, c->get(C::DarkRed));

		float percentageDone = data.size() != 0 ? float(location) / float(data.size()) : 0;

		D2D1_RECT_F songDone = { songScrubBar.left, songScrubBar.top, songScrubBar.left + (songScrubBar.right - songScrubBar.left) * percentageDone, songScrubBar.bottom };
		rt->FillRectangle(songDone, c->get(C::Red));

		int middle = (songScrubBar.bottom + songScrubBar.top) * 0.5;
		for (int i = 0; i < energy.size(); i++)
		{
			float from = songScrubBar.left + (songScrubBar.right - songScrubBar.left) * float(i) / energy.size();
			float to = songScrubBar.left + (songScrubBar.right - songScrubBar.left) * float(i + 1) / energy.size();
			D2D1_RECT_F e = { from, middle - energy[i] * 100, to, middle + energy[i] * 100 };
			rt->FillRectangle(e, c->get(C::LightGray));
		}
	}
	void update()
	{
		dataMutex.lock();

		if (dataReady)
		{
			/*float squareSum = 0;

			for (int i = 0; i < 4000; i++)
			{
				squareSum += 0.5 * (data[location + i + 2000].first * data[location + i + 2000].first
					+ data[location + i + 2000].second * data[location + i + 2000].second);
			}

			float energy = std::sqrt(squareSum / 4000) * 2;

			if (energy > 1)
			{
				energy = 1;
			}*/

			/*float intensity = fft[location / 4000] / 1000.0f;

			if (intensity > 1)
			{
				intensity = 1;
			}*/

			bool close = false;
			for (const auto& b : beat)
			{
				if (std::abs(b - (location - SAMPLE_RATE * 0.1)) < SAMPLE_RATE * 0.05)
				{
					close = true;
				}
			}

			if (close)
			{
				//hue->setLights(hue->getFirstEntertainmentGroup(), 1, 1, 1);
			}
			else
			{
				//hue->setLights(hue->getFirstEntertainmentGroup(), 0.5, 0.5, 0.5);
			}

			
		}

		dataMutex.unlock();

		soundPlot->setPosition(float(location) / data.size());
	}

	void setSongData(std::shared_ptr<const SongData> sd)
	{
		songData = sd;
	}

	std::shared_ptr<const SongData> getSongData()
	{
		return songData;
	}

private:
	void workerFunction()
	{
		/*try
		{
			GLib::Out << "Worker started\n";

			data = getMusic();
			GLib::Out << "Data loaded, size: " << data.size() << "\n";

			energy = calculateEnergy(data);
			soundPlot->setEnergy(energy);
			GLib::Out << "Energy calculated\n";

			fft = calculateFFT(data);
			soundPlot->setFFT(fft);
			GLib::Out << "FFT calculated\n";

			//onset = calculateOnset(data);
			//soundPlot->setOnset(onset, data.size());
			//GLib::Out << "onset calculated\n";

			auto beatData = calculateBeat(data);
			beat = beatData.first;
			beatConfidence = beatData.second;
			soundPlot->setBeat(beat, beatConfidence, data.size());
			GLib::Out << "beat calculated\n";

			energyPerBeat = calculateEnergyPerBeat(beat, data);
			songPart = calculateSongPart(data, beat, 4);
			songPart = cleanSongPart(songPart, 4);
			avgEnergyPerSongPart = calculateAvgEnergyPerSongPart(energyPerBeat, songPart, 4);

			for (auto e : avgEnergyPerSongPart)
			{
				GLib::Out << e << " ";
			}
			GLib::Out << "\n";
			soundPlot->setSongPart(songPart, energyPerBeat);
			GLib::Out << "song part calculated\n";


			lightShow = std::make_shared<LightShow>();
			std::random_device rd;
			std::mt19937 gen(rd());
			std::uniform_int_distribution<> dis1(0, niceColors.size() - 1);
			std::uniform_int_distribution<> dis2(2, 4);

			std::vector<int> lastColor = { 0, 0, 0, 0, 0 };

			for (int i = 0; i < beat.size(); i++)
			{
				int amountOfFlashes = (songPart[i] + 1);

				std::vector<int> ids;
				while (ids.size() < std::min(3, amountOfFlashes))
				{
					int l = dis2(gen);
					auto it = std::find(ids.begin(), ids.end(), l);
					if(it == ids.end())
					{
						ids.push_back(l);
					}
				}

				float confidenceBias = (1 - beatConfidence[i]) * 0.1;

				lightShow->addEffect(0, std::make_shared<LightEffectFlash>(float(beat[i]) / SAMPLE_RATE, LightColor(0.5, 0.5, 0.5), 0.1 + confidenceBias, 0.1, 0.2 + confidenceBias));
				lightShow->addEffect(3, std::make_shared<LightEffectFlash>(float(beat[i]) / SAMPLE_RATE, LightColor(0.5, 0.5, 0.5), 0.1 + confidenceBias, 0.1, 0.2 + confidenceBias));

				for (auto id : ids)
				{
					if (id == 3)
					{
						id = 1;
					}

					int c = dis1(gen);
					while (c == lastColor[id])
					{
						c = dis1(gen);
					}
					lastColor[id] = c;

					lightShow->addEffect(id, std::make_shared<LightEffectFlash>(float(beat[i]) / SAMPLE_RATE, niceColors[c], confidenceBias, 0.1, 0.2 + confidenceBias));
				}
			}
			GLib::Out << lightShow->getAmountOfEffects() << " Effects added\n";
			hue->setLightShow(lightShow);

			dataReady = true;

			PaStreamParameters outputParameters;
			PaStream *stream;

			float buffer[FRAMES_PER_BUFFER][2];

			errorWrapper(Pa_Initialize());
			outputParameters.device = Pa_GetDefaultOutputDevice();
			outputParameters.channelCount = 2;
			outputParameters.sampleFormat = paFloat32;
			outputParameters.suggestedLatency = 0.050;
			outputParameters.hostApiSpecificStreamInfo = NULL;

			errorWrapper(Pa_OpenStream(&stream, NULL, &outputParameters, SAMPLE_RATE, FRAMES_PER_BUFFER, paClipOff, NULL, NULL));

			errorWrapper(Pa_StartStream(stream));

			while (!stop && location < data.size())
			{
				if (play)
				{
					dataMutex.lock();

					for (int i = 0; i < FRAMES_PER_BUFFER; i++)
					{
						if (location + i < data.size())
						{
							buffer[i][0] = data[location + i].first;
							buffer[i][1] = data[location + i].second;
						}
						else
						{
							buffer[i][0] = 0;
							buffer[i][1] = 0;
						}
					}

					location += FRAMES_PER_BUFFER;

					lightShow->setTime(float(location) / SAMPLE_RATE);

					dataMutex.unlock();
					errorWrapper(Pa_WriteStream(stream, buffer, FRAMES_PER_BUFFER));
				}
				else
				{
					for (int i = 0; i < FRAMES_PER_BUFFER; i++)
					{
						buffer[i][0] = 0;
						buffer[i][1] = 0;
					}
					errorWrapper(Pa_WriteStream(stream, buffer, FRAMES_PER_BUFFER));
				}
			}

			errorWrapper(Pa_StopStream(stream));
			errorWrapper(Pa_CloseStream(stream));
		}
		catch (std::runtime_error e)
		{
			GLib::Out << e.what() << "\n";
		}

		Pa_Terminate();*/
	}

private:
	std::mutex dataMutex;
	int location = 0;


	std::shared_ptr<const SongData> songData;

	std::vector<std::pair<float,float>> data;
	std::unique_ptr<std::thread> workerThread;
	bool play = true;
	bool stop = false;

	D2D1_RECT_F songScrubBar = { 120, 10, 120 + 800, 10 + 100 };
	std::vector<float> energy;
	std::vector<float> fft;
	std::vector<int> onset;
	std::vector<int> beat;
	std::vector<float> beatConfidence;
	std::vector<int> songPart;
	std::vector<float> energyPerBeat;
	std::vector<float> avgEnergyPerSongPart;

	std::unique_ptr<Hue> hue;

	bool dataReady = false;

	SoundPlot* soundPlot;

	std::shared_ptr<LightShow> lightShow;
};