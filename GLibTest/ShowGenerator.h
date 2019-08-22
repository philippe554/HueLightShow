#pragma once

#include "GLib.h"

#include "SongData.h"
#include "MediaPlayer.h"
#include "ShowPlot.h"

#include "Pattern.h"
#include "PatternParser.h"

using namespace GLib;

std::vector<LightColor> niceColors =
{
	LightColor(1, 0, 0),
	LightColor(0, 1, 0),
	LightColor(0, 0, 1),
	LightColor(1, 1, 0),
	LightColor(1, 0, 1),
	LightColor(0, 1, 1)
};

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
		auto selectList = addView<MovingView>(0, 0, 510, -1, false, true);

		std::string path = "./";
		int i = 0;
		for (const auto & entry : fs::directory_iterator(path))
		{
			if (entry.path().extension().generic_string() == ".lsh")
			{
				selectList->addView<Button>(20, 20 + i * 60, 450, 40, [file = entry.path(), &nextFile = nextFile, &m = workerMutex, &scheduled = scheduled]()
				{
					std::unique_lock<std::mutex> lock(m);
					scheduled = true;
					nextFile = file.generic_string();
				}, "  " + entry.path().filename().generic_string());
				i++;
			}

		}

		boostSlider = addView<Slider>(650, 140, 250, 40, [&](float ratio) 
		{
			if (lightShow)
			{
				lightShow->setBoost(ratio);
			}
		}
		, 0.0);

		forceCheckbox = addView<CheckBox>(650, 200, 40, 40, [&](bool state)
		{
			if (lightShow)
			{
				lightShow->setForce(state);
			}
		}, false);

		//addView<Button>(400, 60, 20, 40, []() {})->setHorizontalDragable(400, 500, [&](float ratio) {amountOfClusters = (ratio * 5) + 1; });

		/*MovingView* movingView = addView<GLib::MovingView>(0, 200, -1, 120, true, false);
		movingView->setScrollZoom(true, false);

		View* movingSoundPlot = movingView->getMovingView();
		showPlot = movingSoundPlot->addView<ShowPlot>();*/

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
			w->print("Loading...", c->get(C::Black), w->get(20), { 530, 20, 700, 60 });
		}
		else if (scheduled)
		{
			w->print("Scheduled...", c->get(C::Black), w->get(20), { 530, 20, 700, 60 });
		}

		w->print("Settings", c->get(C::Black), w->get(20), { 530, 80, 700, 120 });

		w->print("Boost", c->get(C::Black), w->get(20), { 530, 140, 700, 180 });

		w->print("Kitchen", c->get(C::Black), w->get(20), { 530, 200, 700, 240 });
 
		//w->print(std::to_string(amountOfClusters), c->get(C::Black), w->get(20), { 330, 60, 500, 100 });
	}

private:
	std::vector<std::vector<float>> calculateFft(std::shared_ptr<const SongData> songData, int inputSize, const int amountOfGroups)
	{
		int outputSize = inputSize / 2 + 1;

		double* inputBuffer = fftw_alloc_real(inputSize * sizeof(double));
		fftw_complex* outputBuffer = fftw_alloc_complex(outputSize * sizeof(fftw_complex));
		fftw_plan plan = fftw_plan_dft_r2c_1d(inputSize, inputBuffer, outputBuffer, FFTW_ESTIMATE);

		std::vector<float> hann = getHann(inputSize);
		std::vector<std::pair<int, int>> groups = getLogGroups(outputSize, amountOfGroups);
		std::vector<std::vector<float>> fft;

		for (int i = 0; i < songData->beat.size(); i++)
		{
			int middle = songData->beat[i];

			for (int j = 0; j < inputSize; j++)
			{
				if (0 <= middle - inputSize / 2 + j && middle - inputSize / 2 + j < songData->data.size())
				{
					inputBuffer[j] = (songData->data[middle - inputSize / 2 + j].first + songData->data[middle - inputSize / 2 + j].second) * 0.5 * hann[j];
				}
				else
				{
					inputBuffer[j] = 0;
				}
			}

			fftw_execute(plan);

			fft.emplace_back();

			for (int j = 0; j < amountOfGroups; j++)
			{
				float sum = 0;
				for (int k = groups[j].first; k <= groups[j].second; k++)
				{
					sum += sqrt(outputBuffer[k][0] * outputBuffer[k][0] + outputBuffer[k][1] * outputBuffer[k][1]);
				}
				fft.back().emplace_back(sum / (groups[j].second - groups[j].first + 1));
			}
		}

		fftw_free(inputBuffer);
		fftw_free(outputBuffer);
		fftw_destroy_plan(plan);

		return fft;
	}
	std::vector<int> cluster(std::vector<std::vector<float>>& fft, int from, int to, int amountOfClusters, int inputSize, const int amountOfGroups)
	{
		typedef dlib::matrix<float> SampleType;
		typedef dlib::radial_basis_kernel<SampleType> KernelType;

		std::vector<int> songPart;
		std::vector<SampleType> fftMatrix;

		for (int i = from; i < to; i++)
		{
			SampleType sample(amountOfGroups, 1);
			for (int j = 0; j < amountOfGroups; j++)
			{
				sample(j) = fft[i][j];
			}
			fftMatrix.push_back(sample);
		}

		dlib::kcentroid<KernelType> kc(KernelType(0.1), 0.001, 8);
		dlib::kkmeans<KernelType> test(kc);
		std::vector<SampleType> initial_centers;
		test.set_number_of_centers(amountOfClusters);
		dlib::pick_initial_centers(amountOfClusters, initial_centers, fftMatrix, test.get_kernel());
		test.train(fftMatrix, initial_centers);

		for (int i = 0; i < fftMatrix.size(); i++)
		{
			songPart.push_back(test(fftMatrix[i]));
		}

		return songPart;
	}
	void cleanSongPart(std::unique_ptr<SongPartData>& songPartData, int amountOfSongParts)
	{
		std::vector<int> newSet;
		std::map<int, float> g = getGaussian(20, 4);

		for (int i = 0; i < songPartData->dataL1.size(); i++)
		{
			float max = 0;
			int maxIndex = 0;

			for (int j = 0; j < amountOfSongParts; j++)
			{
				float v = 0;
				for (int k = -10; k <= 10; k++)
				{
					if (i + k >= 0 && i + k < songPartData->dataL1.size())
					{
						if (songPartData->dataL1[i + k] == j)
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

		songPartData->dataL1 = newSet;
	}
	std::unique_ptr<SongPartData> calculateSongPart(std::shared_ptr<const SongData> songData, int amountOfClusters, int inputSize, const int amountOfGroups)
	{
		std::vector<std::vector<float>> fft = calculateFft(songData, inputSize, amountOfGroups);

		std::unique_ptr<SongPartData> songPartDataLocal = std::make_unique<SongPartData>();

		songPartDataLocal->dataL1 = cluster(fft, 0, fft.size(), amountOfClusters, inputSize, amountOfGroups);

		cleanSongPart(songPartDataLocal, amountOfClusters);

		int last = songPartDataLocal->dataL1[0];
		int lastIndex = 0;
		for (int i = 1; i < songPartDataLocal->dataL1.size(); i++)
		{
			if (songPartDataLocal->dataL1[i] != last)
			{
				std::vector<int> c = cluster(fft, lastIndex, i, std::min(amountOfClusters, i - lastIndex), inputSize, amountOfGroups);
				songPartDataLocal->dataL2.insert(songPartDataLocal->dataL2.end(), c.begin(), c.end());
				last = songPartDataLocal->dataL1[i];
				lastIndex = i;
			}
		}
		std::vector<int> c = cluster(fft, lastIndex, songPartDataLocal->dataL1.size(), std::min(amountOfClusters, int(songPartDataLocal->dataL1.size() - lastIndex)), inputSize, amountOfGroups);
		songPartDataLocal->dataL2.insert(songPartDataLocal->dataL2.end(), c.begin(), c.end());

		return std::move(songPartDataLocal);
	}

	std::unique_ptr<LightShow> calculateLightShow(std::shared_ptr<const SongData> songData, std::shared_ptr<const SongPartData> songPartData)
	{
		//std::unique_ptr<LightShow> lightShow = std::make_unique<LightShow>();

		int lastLight = 0;
		for (int i = 0; i < songPartData->dataL2.size(); i++)
		{
			float c = (0.5 - songData->beatConfidence[i]) / 4.0;
			c = c < 0 ? 0 : c;

			for (int j = 0; j < 5; j++)
			{
				//lightShow->addEffect(j, std::make_shared<LightEffectFlash>(float(songData->beat[i]) / SAMPLE_RATE, niceColors[songPartData->dataL2[i]], c, 0.05, 0.1 + c));
			}
			/*if (i % 2 == 0)
			{
				lightShow->addEffect(0, std::make_shared<LightEffectFlash>(float(songData->beat[i]) / SAMPLE_RATE, LightColor(1, 1, 1), 0.5 + c, 10.0, 0.5 + c));
			}
			else
			{
				lightShow->addEffect(3, std::make_shared<LightEffectFlash>(float(songData->beat[i]) / SAMPLE_RATE, LightColor(1, 1, 1), 0.5 + c, 10.0, 0.5 + c));
			}

			std::vector<int> effectLights{ 1, 2, 4 };

			int same = 1;
			for (int j = 1; j <= 4; j++)
			{
				if (i + j >= 0 && songPartData->dataL2[i] == songPartData->dataL2[i - j])
				{
					same++;
				}
				else
				{
					break;
				}
			}
			for (int j = 1; j <= 4; j++)
			{
				if (i + j < songPartData->dataL2.size() && songPartData->dataL2[i] == songPartData->dataL2[i + j])
				{
					same++;
				}
				else
				{
					break;
				}
			}

			if (same < 5)
			{
				std::random_device rd;
				std::mt19937 eng(rd());
				std::uniform_int_distribution<> distr1(0, effectLights.size() - 1);
				std::uniform_int_distribution<> distr2(0, 1);

				int light = lastLight;
				while (light == lastLight)
				{
					light = effectLights[distr1(eng)];
				}
				lastLight = light;

				//if (distr2(eng) == 0)
				{
					lightShow->addEffect(light, std::make_shared<LightEffectFlash>(float(songData->beat[i]) / SAMPLE_RATE, niceColors[songPartData->dataL2[i]], c, 0.05, 0.1 + c));
				}
			}*/
		}

		//return std::move(lightShow);
	}

	void worker()
	{
		std::shared_ptr<const SongData> songData;
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
				}
			}
			if (inProgress)
			{
				/*int n = amountOfClusters;
				std::shared_ptr<const SongPartData> songPartData = calculateSongPart(songData, n, SAMPLE_RATE, 12);
				showPlot->setSongPartData(songData, songPartData);
				mediaPlayer->setSongData(songData, songPartData);

				std::shared_ptr<LightShow> lightShow = calculateLightShow(songData, songPartData);
				mediaPlayer->setLightShow(lightShow);*/	

				lightShow = std::make_shared<LightShow>(nextFile, songData);
				lightShow->setBoost(boostSlider->getRatio());
				lightShow->setForce(forceCheckbox->getState());

				mediaPlayer->setLightShow(lightShow);

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
	std::string nextFile;

	std::shared_ptr<LightShow> lightShow;

	Slider* boostSlider;
	CheckBox* forceCheckbox;
};