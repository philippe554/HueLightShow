#pragma once

#include "GLib.h"

#include "SongData.h"
#include "MediaPlayer.h"
#include "ShowPlot.h"

using namespace GLib;

std::vector<float> getHann(int size)
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
		g[i] = 1.0 / (std::sqrt(2.0 * PI * std::pow(std, 2.0))) * std::pow(std::exp(1), -std::pow(i, 2.0) / (2.0 * std::pow(std, 2.0)));
	}
	return g;
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

	for (auto g : groups)
	{
		GLib::Out << "(" << g.first << "-" << g.second << ") ";
	}
	GLib::Out << "\n";

	return groups;
}

class ShowGenerator : public View
{
public:
	using View::View;
	void init(MediaPlayer* mp)
	{
		addView<Button>(20, 20, 250, 40, [&scheduled = scheduled, &m = workerMutex]()
		{
			std::unique_lock<std::mutex> lock(m);
			scheduled = true;
		}, "  Create");

		addView<Button>(400, 60, 20, 40, []() {})->setHorizontalDragable(400, 500, [&](float ratio) {amountOfClusters = (ratio * 5) + 1; });

		View* movingSoundPlot = addView<GLib::MovingView>(0, 200, -1, 120, true, false)->getMovingView();
		showPlot = movingSoundPlot->addView<ShowPlot>();

		mediaPlayer = mp;

		workerThread = std::make_unique<std::thread>(&ShowGenerator::worker, this);
	}

	~ShowGenerator()
	{
		stop = true;
		workerThread->join();
		workerThread.reset();
	}

	void render(RT* rt, Writer* w, Color* c, D2D1_RECT_F& visibleRect) override
	{
		if (inProgress)
		{
			w->print("Loading...", c->get(C::Black), w->get(20), { 330, 20, 500, 60 });
		}
		else if (scheduled)
		{
			w->print("Scheduled...", c->get(C::Black), w->get(20), { 330, 20, 500, 60 });
		}

		w->print(std::to_string(amountOfClusters), c->get(C::Black), w->get(20), { 330, 60, 500, 100 });
	}

private:
	std::vector<int> calculateSongPart(std::shared_ptr<const SongData> songData, int amountOfClusters)
	{
		const int amountOfGroups = 12;
		typedef dlib::matrix<float, 12, 1> SampleType;
		typedef dlib::radial_basis_kernel<SampleType> KernelType;

		std::vector<int> songPart;
		std::vector<SampleType> fftData;

		int inputSize = SAMPLE_RATE;
		int outputSize = inputSize / 2 + 1;

		std::vector<std::pair<int, int>> groups = getLogGroups(outputSize, amountOfGroups);
		std::vector<float> hann = getHann(inputSize);

		double* inputBuffer = fftw_alloc_real(inputSize * sizeof(double));
		fftw_complex* outputBuffer = fftw_alloc_complex(outputSize * sizeof(fftw_complex));
		fftw_plan plan = fftw_plan_dft_r2c_1d(inputSize, inputBuffer, outputBuffer, FFTW_ESTIMATE);

		//std::vector<float> e = calculateEnergyPerBeat(beat, song);

		for (int i = 0; i < songData->beat.size() - 1; i++)
		{
			int middle = songData->beat[i]; // beat[i + 1] - beat[i];
			//if (middle - inputSize / 2 >= 0 && middle + inputSize / 2 < song.size())
			{
				for (int j = 0; j < inputSize; j++)
				{
					inputBuffer[j] = (songData->data[middle - inputSize / 2 + j].first + songData->data[middle - inputSize / 2 + j].second) * 0.5 * hann[j];
				}

				fftw_execute(plan);

				SampleType sample;
				for (int j = 0; j < 12; j++)
				{
					float sum = 0;
					for (int k = groups[j].first; k <= groups[j].second; k++)
					{
						sum += sqrt(outputBuffer[k][0] * outputBuffer[k][0] + outputBuffer[k][1] * outputBuffer[k][1]);
					}
					sample(j) = (sum / (groups[j].second - groups[j].first + 1));
				}
				//sample(11) = e[i];

				fftData.push_back(sample);
			}
		}

		dlib::kcentroid<KernelType> kc(KernelType(0.1), 0.001, 8);
		dlib::kkmeans<KernelType> test(kc);
		std::vector<SampleType> initial_centers;
		test.set_number_of_centers(amountOfClusters);
		dlib::pick_initial_centers(amountOfClusters, initial_centers, fftData, test.get_kernel());
		test.train(fftData, initial_centers);

		for (int i = 0; i < songData->beat.size() - 1; i++)
		{
			songPart.push_back(test(fftData[i]));
		}

		fftw_free(inputBuffer);
		fftw_free(outputBuffer);
		fftw_destroy_plan(plan);

		return songPart;
	}

	void cleanSongPart(std::unique_ptr<SongPartData>& songPartData, int amountOfSongParts)
	{
		std::vector<int> newSet;
		std::map<int, float> g = getGaussian(20, 4);

		for (int i = 0; i < songPartData->data.size(); i++)
		{
			float max = 0;
			int maxIndex = 0;

			for (int j = 0; j < amountOfSongParts; j++)
			{
				float v = 0;
				for (int k = -10; k <= 10; k++)
				{
					if (i + k >= 0 && i + k < songPartData->data.size())
					{
						if (songPartData->data[i + k] == j)
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

		songPartData->data = newSet;
	}

	void worker()
	{
		std::shared_ptr<const SongData> songData;
		std::unique_ptr<SongPartData> songPartDataLocal;
		while (!stop)
		{
			if (scheduled && !inProgress)
			{
				std::unique_lock<std::mutex> lock(workerMutex);
				songData = mediaPlayer->getSongData();
				if (songData)
				{
					scheduled = false;
					inProgress = true;

					songPartDataLocal = std::make_unique<SongPartData>();
				}
			}
			if (inProgress)
			{
				int n = amountOfClusters;
				songPartDataLocal->data = calculateSongPart(songData, n);
				cleanSongPart(songPartDataLocal, n);

				songPartData = std::move(songPartDataLocal);
				showPlot->setSongPartData(songData, songPartData);
				inProgress = false;
			}
			else
			{
				std::this_thread::sleep_for(std::chrono::milliseconds(10));
			}
		}
	}

private:
	MediaPlayer* mediaPlayer;
	std::unique_ptr<std::thread> workerThread;
	std::mutex workerMutex;
	bool stop = false;
	bool inProgress = false;
	bool scheduled = false;
	int amountOfClusters = 4;
	std::shared_ptr<const SongPartData> songPartData;
	ShowPlot* showPlot;
};