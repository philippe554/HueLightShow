#pragma once

#include "GLibMain.h"

#include "SongData.h"
#include "SoundPlot.h"

using namespace GLib;

std::vector<std::pair<std::string, std::string>> getPossibleFiles()
{
	std::vector<std::pair<std::string, std::string>> data = {
		{"Heaven", "C:\\dev\\data\\avicii.mp3"},
		{"Waiting for love", "C:\\dev\\data\\waiting.mp3"},
		{"Don Diablo", "C:\\dev\\data\\dondiablo.mp3"},
		{"Spons Sessions 51", "C:\\dev\\data\\spons.mp3"},
		{"House Mix", "C:\\dev\\data\\deepmix1.mp3"}
	};

	return data;
}

class LoadSong : public View
{
public:
	using View::View;
	void init(MediaPlayer* mp)
	{
		std::vector<std::pair<std::string, std::string>> files = getPossibleFiles();

		auto selectList = addView<MovingView>(0, 0, 310, -1, false, true);

		for (int i = 0; i < files.size(); i++)
		{
			selectList->addView<Button>(20, 20 + i * 60, 250, 40, [file = files[i].second, &nextFile = nextFile, &m = workerMutex]()
			{
				std::unique_lock<std::mutex> lock(m);
				nextFile = file;
			}, "  " + files[i].first);
		}

		mediaPlayer = mp;

		workerThread = std::make_unique<std::thread>(&LoadSong::worker, this);
	}

	~LoadSong()
	{
		stop = true;
		workerThread->join();
		workerThread.release();
	}

	void render(RT* rt, Writer* w, Color* c, D2D1_RECT_F& visibleRect) override
	{
		if (loadInProgress)
		{
			w->print("Loading...", c->get(C::Black), w->get(20), {330, 20, 500, 60});
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

		std::shared_ptr<SongData> songData;

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

				if (loadInProgress)
				{
					songData = nullptr;
					loadInProgress = false;
					Aubio::del_aubio_tempo(o);
					mpg123_close(mh);
				}

				loadInProgress = true;
				phase = 0;
				mpg123_open(mh, nextFileCopy.c_str());
				int channels;
				int encoding;
				long rate;
				mpg123_getformat(mh, &rate, &channels, &encoding);

				o = Aubio::new_aubio_tempo("default", windowSize, hopSize, rate);

				songData = std::make_shared<SongData>();
				songData->sampleRate = rate;
			}
			if (loadInProgress)
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
						mediaPlayer->sentSongData(songData);
						songData = nullptr;
						loadInProgress = false;
						Aubio::del_aubio_tempo(o);
						mpg123_close(mh);
						phase = 0;
					}
				}
			}
			else
			{
				std::this_thread::sleep_for(std::chrono::milliseconds(10));
			}
		}

		if (loadInProgress)
		{
			songData = nullptr;
			loadInProgress = false;
			Aubio::del_aubio_tempo(o);
			mpg123_close(mh);
		}

		mpg123_delete(mh);
		mpg123_exit();

		Aubio::del_fvec(in);
		Aubio::del_fvec(out);

		delete[] buffer;
	}

private:
	std::string nextFile;
	std::unique_ptr<std::thread> workerThread;
	std::mutex workerMutex;
	bool stop = false;
	MediaPlayer* mediaPlayer;
	bool loadInProgress = false;
};
