#pragma once

#include <filesystem>

#include "GLib.h"

#include "SongData.h"
#include "MediaPlayer.h"

using namespace GLib;
namespace fs = std::filesystem;

class LoadSong : public View
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
			if (entry.path().extension().generic_string() == ".mp3")
			{
				selectList->addView<Button>(20, 20 + i * 60, 450, 40, [file = entry.path(), &nextFile = nextFile, &m = workerMutex]()
				{
					std::unique_lock<std::mutex> lock(m);
					nextFile = file.generic_string();
				}, "  " + entry.path().filename().generic_string());
				i++;
			}
	
		}

		mediaPlayer = mp;

		workerThread = std::make_unique<std::thread>(&LoadSong::worker, this);
	}

	~LoadSong()
	{
		stop = true;
		workerThread->join();
		workerThread.reset();
	}

	void render(RT* rt, Writer* w, Color* c, D2D1_RECT_F& visibleRect) override
	{
		if (inProgress)
		{
			w->print("Loading...", c->get(C::Black), w->get(20), {530, 20, 700, 60});
		}
	}

private:
	void worker()
	{
		int phase = 0;

		int err;
		mpg123_init();
		mpg123_handle* mh = mpg123_new(NULL, &err);

		size_t buffer_size = mpg123_outblock(mh);
		size_t done;
		unsigned char* buffer = new unsigned char[buffer_size];
		
		Aubio::aubio_tempo_t* o = nullptr;
		long pos = 0;
		int windowSize = 1024;
		int hopSize = windowSize / 4;
		Aubio::fvec_t * in = Aubio::new_fvec(hopSize);
		Aubio::fvec_t * out = Aubio::new_fvec(1);

		std::unique_ptr<SongData> songData;

		while (!stop)
		{
			workerMutex.lock();
			std::string nextFileCopy = nextFile;
			workerMutex.unlock();

			if (nextFileCopy.length() > 0)
			{
				workerMutex.lock();
				nextFile = "";
				workerMutex.unlock();

				mediaPlayer->setSongData(nullptr, nullptr);

				if (inProgress)
				{
					songData.reset();
					inProgress = false;
					Aubio::del_aubio_tempo(o);
					mpg123_close(mh);
				}

				inProgress = true;
				phase = 0;
				pos = 0;
				mpg123_open(mh, nextFileCopy.c_str());
				int channels;
				int encoding;
				long rate;
				mpg123_getformat(mh, &rate, &channels, &encoding);

				o = Aubio::new_aubio_tempo("default", windowSize, hopSize, rate);

				songData = std::make_unique<SongData>();
				songData->sampleRate = rate;
			}
			if (inProgress)
			{
				if (phase == 0)
				{
					if (mpg123_read(mh, buffer, buffer_size, &done) == MPG123_OK)
					{
						short* tst = reinterpret_cast<short*>(buffer);
						for (auto i = 0; i < done / 2; i += 2)
						{
							songData->data.emplace_back(tst[i] / (float)SHRT_MAX, tst[i + 1] / (float)SHRT_MAX);
						}
					}
					else
					{
						phase = 1;
					}
				}
				if(phase == 1)
				{
					if (pos < songData->data.size() - hopSize)
					{
						for (int j = 0; j < hopSize; j++)
						{
							in->data[j] = (songData->data[pos + j].first + songData->data[pos + j].second) * 0.5;
						}

						Aubio::aubio_tempo_do(o, in, out);

						if (out->data[0] != 0)
						{
							songData->beat.push_back(Aubio::aubio_tempo_get_last(o));
							songData->beatConfidence.push_back(Aubio::aubio_tempo_get_confidence(o));
						}

						pos += hopSize;
					}
					else
					{
						phase = 2;
					}
				}
				if (phase == 2)
				{
					for (int i = 0; i < songData->beat.size(); i++)
					{
						float sum = 0;
						int end = i + 1 == songData->beat.size() ? songData->data.size() - 1 : songData->beat[i + 1];
						for (int j = songData->beat[i]; j < end; j++)
						{
							sum += 0.5 * (songData->data[j].first * songData->data[j].first
								+ songData->data[j].second * songData->data[j].second);
						}
						songData->beatEnergy.push_back(std::sqrt(sum / (end - songData->beat[i])));
					}
					phase = 3;
				}
				if (phase == 3)
				{
					mediaPlayer->setSongData(std::move(songData), nullptr);
					inProgress = false;
					Aubio::del_aubio_tempo(o);
					mpg123_close(mh);
					phase = 0;
				}
			}
			else
			{
				std::this_thread::sleep_for(std::chrono::milliseconds(10));
			}
		}

		if (inProgress)
		{
			Aubio::del_aubio_tempo(o);
			mpg123_close(mh);
		}

		mpg123_delete(mh);
		mpg123_exit();

		Aubio::del_fvec(in);
		Aubio::del_fvec(out);
		Aubio::aubio_cleanup();

		delete[] buffer;
	}

private:
	std::string nextFile;
	std::unique_ptr<std::thread> workerThread;
	std::mutex workerMutex;
	bool stop = false;
	MediaPlayer* mediaPlayer;
	bool inProgress = false;
};
