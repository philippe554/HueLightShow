#include "Razer.h"
#include "GLib.h"

#ifdef _WIN64
#define CHROMASDKDLL        _T("RzChromaSDK64.dll")
#else
#define CHROMASDKDLL        _T("RzChromaSDK.dll")
#endif

using namespace ChromaSDK;
using namespace ChromaSDK::Keyboard;
using namespace ChromaSDK::Keypad;
using namespace ChromaSDK::Mouse;
using namespace ChromaSDK::Mousepad;
using namespace ChromaSDK::Headset;
using namespace std;

typedef RZRESULT(*INIT)(void);
typedef RZRESULT(*UNINIT)(void);
typedef RZRESULT(*CREATEEFFECT)(RZDEVICEID DeviceId, ChromaSDK::EFFECT_TYPE Effect, PRZPARAM pParam, RZEFFECTID *pEffectId);
typedef RZRESULT(*CREATEKEYBOARDEFFECT)(ChromaSDK::Keyboard::EFFECT_TYPE Effect, PRZPARAM pParam, RZEFFECTID *pEffectId);
typedef RZRESULT(*CREATEHEADSETEFFECT)(ChromaSDK::Headset::EFFECT_TYPE Effect, PRZPARAM pParam, RZEFFECTID *pEffectId);
typedef RZRESULT(*CREATEMOUSEPADEFFECT)(ChromaSDK::Mousepad::EFFECT_TYPE Effect, PRZPARAM pParam, RZEFFECTID *pEffectId);
typedef RZRESULT(*CREATEMOUSEEFFECT)(ChromaSDK::Mouse::EFFECT_TYPE Effect, PRZPARAM pParam, RZEFFECTID *pEffectId);
typedef RZRESULT(*CREATEKEYPADEFFECT)(ChromaSDK::Keypad::EFFECT_TYPE Effect, PRZPARAM pParam, RZEFFECTID *pEffectId);
typedef RZRESULT(*SETEFFECT)(RZEFFECTID EffectId);
typedef RZRESULT(*DELETEEFFECT)(RZEFFECTID EffectId);
typedef RZRESULT(*REGISTEREVENTNOTIFICATION)(HWND hWnd);
typedef RZRESULT(*UNREGISTEREVENTNOTIFICATION)(void);
typedef RZRESULT(*QUERYDEVICE)(RZDEVICEID DeviceId, ChromaSDK::DEVICE_INFO_TYPE &DeviceInfo);

CREATEEFFECT CreateEffect = nullptr;
CREATEKEYBOARDEFFECT CreateKeyboardEffect = nullptr;
CREATEMOUSEEFFECT CreateMouseEffect = nullptr;
CREATEHEADSETEFFECT CreateHeadsetEffect = nullptr;
CREATEMOUSEPADEFFECT CreateMousepadEffect = nullptr;
CREATEKEYPADEFFECT CreateKeypadEffect = nullptr;
SETEFFECT SetEffect = nullptr;
DELETEEFFECT DeleteEffect = nullptr;
QUERYDEVICE QueryDevice = nullptr;

Razer::Razer()
{
	workerThread = std::make_unique<std::thread>(&Razer::worker, this);
}
Razer::~Razer()
{
	stop = true;
	if (workerThread)
	{
		workerThread->join();
		workerThread.reset();
	}
}

void Razer::setColor(float r, float g, float b)
{
	std::lock_guard<std::mutex> lock(dataMutex);
	color = RGB(r * 255, g * 255, b * 255);
}

void Razer::worker()
{
	while (!stop)
	{
		if (!loaded)
		{
			init();
		}

		if (loaded)
		{
			update();
		}

		std::this_thread::sleep_for(std::chrono::milliseconds(30));
	}

	GLib::Out << "Razer sync stoped\n";
}

bool Razer::init()
{
	m_ChromaSDKModule = LoadLibrary(CHROMASDKDLL);
	if (m_ChromaSDKModule == nullptr)
	{
		GLib::Out << "Chroma DLL not found\n";
		stop = true;
		return false;
	}
	else
	{
		INIT Init = reinterpret_cast<INIT>(GetProcAddress(m_ChromaSDKModule, "Init"));
		if (Init)
		{
			auto Result = Init();
			if (Result == RZRESULT_SUCCESS)
			{
				CreateEffect = reinterpret_cast<CREATEEFFECT>(GetProcAddress(m_ChromaSDKModule, "CreateEffect"));
				CreateKeyboardEffect = reinterpret_cast<CREATEKEYBOARDEFFECT>(GetProcAddress(m_ChromaSDKModule, "CreateKeyboardEffect"));
				CreateMouseEffect = reinterpret_cast<CREATEMOUSEEFFECT>(GetProcAddress(m_ChromaSDKModule, "CreateMouseEffect"));
				CreateHeadsetEffect = reinterpret_cast<CREATEHEADSETEFFECT>(GetProcAddress(m_ChromaSDKModule, "CreateHeadsetEffect"));
				CreateMousepadEffect = reinterpret_cast<CREATEMOUSEPADEFFECT>(GetProcAddress(m_ChromaSDKModule, "CreateMousepadEffect"));
				CreateKeypadEffect = reinterpret_cast<CREATEKEYPADEFFECT>(GetProcAddress(m_ChromaSDKModule, "CreateKeypadEffect"));
				SetEffect = reinterpret_cast<SETEFFECT>(GetProcAddress(m_ChromaSDKModule, "SetEffect"));
				DeleteEffect = reinterpret_cast<DELETEEFFECT>(GetProcAddress(m_ChromaSDKModule, "DeleteEffect"));
				QueryDevice = reinterpret_cast<QUERYDEVICE>(GetProcAddress(m_ChromaSDKModule, "QueryDevice"));

				if (CreateEffect &&
					CreateKeyboardEffect &&
					CreateMouseEffect &&
					CreateHeadsetEffect &&
					CreateMousepadEffect &&
					CreateKeypadEffect &&
					SetEffect &&
					DeleteEffect &&
					QueryDevice)
				{
					loaded = true;
					GLib::Out << "Chroma DLL loaded\n";
					return true;
				}
				else
				{
					GLib::Out << "Failed loading Chroma DLL\n";
					return false;
				}
			}
		}
	}
}

bool Razer::update()
{
	std::lock_guard<std::mutex> lock(dataMutex);
	if (loaded)
	{
		ChromaSDK::Keyboard::CUSTOM_EFFECT_TYPE Example_keyboard_effect = {};

		for (size_t row = 0; row < ChromaSDK::Keyboard::MAX_ROW; row++)
		{
			for (size_t col = 0; col < ChromaSDK::Keyboard::MAX_COLUMN; col++)
			{
				Example_keyboard_effect.Color[row][col] = color;
			}
		}

		RZRESULT Result_Keyboard = CreateKeyboardEffect(ChromaSDK::Keyboard::CHROMA_CUSTOM, &Example_keyboard_effect, nullptr);

		if (Result_Keyboard != RZRESULT_SUCCESS)
		{
			GLib::Out << "Chroma connection lost\n";
			loaded = false;
			return false;
		}
	}

	if (loaded)
	{
		ChromaSDK::Mouse::CUSTOM_EFFECT_TYPE2 Example_mouse_effect = {};

		for (size_t row = 0; row < ChromaSDK::Mouse::MAX_ROW; row++)
		{
			for (size_t col = 0; col < ChromaSDK::Mouse::MAX_COLUMN; col++)
			{
				Example_mouse_effect.Color[row][col] = color;
			}
		}

		RZRESULT Result_Keyboard = CreateMouseEffect(ChromaSDK::Mouse::CHROMA_CUSTOM2, &Example_mouse_effect, nullptr);

		if (Result_Keyboard != RZRESULT_SUCCESS)
		{
			GLib::Out << "Chroma connection lost\n";
			loaded = false;
			return false;
		}
	}

	return true;
}
