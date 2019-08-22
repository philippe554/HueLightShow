#pragma once

#include <iostream>
#include <tchar.h>
#include <assert.h>
#include <wtypes.h>
#include <thread>
#include <mutex>

#include "RzChromaSDKDefines.h"
#include "RzChromaSDKTypes.h"
#include "RzErrors.h"

class Razer
{
public:
	Razer();
	~Razer();

	void setColor(float r, float g, float b);

private:
	void worker();
	bool init();
	bool update();

private:
	HMODULE m_ChromaSDKModule;
	bool loaded = false;

	std::mutex dataMutex;
	std::unique_ptr<std::thread> workerThread;
	bool stop = false;

	COLORREF color;
};