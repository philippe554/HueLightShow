#pragma once

#include <string>
#include <vector>
#include <thread>
#include <mutex>

#include <cpprest/http_client.h>

using namespace utility;                    // Common utilities like string conversions
using namespace web;                        // Common features like URIs.
using namespace web::http;                  // Common HTTP functionality
using namespace web::http::client;          // HTTP client features
using namespace concurrency::streams;       // Asynchronous streams

#include "mbedtls/net_sockets.h"
#include "mbedtls/debug.h"
#include "mbedtls/ssl.h"
#include "mbedtls/entropy.h"
#include "mbedtls/ctr_drbg.h"
#include "mbedtls/error.h"
#include "mbedtls/certs.h"
#include "mbedtls/timing.h"

#include "LightEffect.h"

#define READ_TIMEOUT_MS 1000
#define MAX_RETRY       5
#define DEBUG_LEVEL 2000
#define SERVER_PORT "2100"
#define SERVER_NAME "Hue"

std::wstring strToWStr(const std::string& s)
{
	std::wstring temp(s.length(), L' ');
	std::copy(s.begin(), s.end(), temp.begin());
	return temp;
}


std::string wstrToStr(const std::wstring& s)
{
	std::string temp(s.length(), ' ');
	std::copy(s.begin(), s.end(), temp.begin());
	return temp;
}

std::vector<unsigned char> hexToBytes(const std::string& hex)
{
	std::vector<unsigned char> bytes;

	for (unsigned int i = 0; i < hex.length(); i += 2) 
	{
		std::string byteString = hex.substr(i, 2);
		unsigned char byte = (unsigned char)strtol(byteString.c_str(), NULL, 16);
		bytes.push_back(byte);
	}

	return bytes;
}

void mbedtlsErrorWrapper(int error)
{
	if (error != 0)
	{
		char buffer[100];
		mbedtls_strerror(error, buffer, 100);
		throw std::runtime_error("Mbedtls error " + std::to_string(error) + ": " + buffer);
	}
}

class HueLight
{
public:
	HueLight(int _id)
	{
		id = _id;
	}

	int id;

	float red = 0;
	float green = 0;
	float blue = 0;
};

class HueGroup
{
public:
	int id;
	std::string type;
	std::string name;
	std::vector<HueLight> lights;
};

class Hue
{
public:
	Hue(std::string _ip, std::string _username, std::string _key)
	{
		ip = _ip;
		username = _username;
		key = _key;

		paired = true;

		updateGroups();
	}

	~Hue()
	{
		stop = true;
		if (workerThread)
		{
			workerThread->join();
			workerThread.reset();
		}
	}

	void updateGroups()
	{
		try
		{
			http_client client(strToWStr("http://" + ip + "/api/" + username));

			http_response response = client.request(methods::GET, strToWStr("/groups/"))
				.then([&](http_response response)
			{
				if (response.status_code() == status_codes::OK)
				{
					web::json::value groups = response.extract_json().get();

					for (const auto group : groups.as_object())
					{
						HueGroup hueGroup;
						hueGroup.id = std::stoi(group.first);
						hueGroup.type = wstrToStr(group.second.at(L"type").as_string());
						hueGroup.name = wstrToStr(group.second.at(L"name").as_string());

						web::json::value lights = group.second.at(L"lights");


						GLib::Out << "Hue Group: " << hueGroup.name << " (" << hueGroup.id << ") " << hueGroup.type << ": ";
						for (const auto light : lights.as_array())
						{
							hueGroup.lights.push_back(std::stoi(light.as_string()));
							GLib::Out << hueGroup.lights.back().id << " ";
						}
						GLib::Out << "\n";

						hueGroups.push_back(hueGroup);
					}
				}
			}
			).wait();
		}
		catch (const std::exception &e)
		{
			GLib::Out << "Connection error: " << e.what() << "\n";
		}

	}

	const std::vector<HueGroup>& getHueGroups()
	{
		return hueGroups;
	}

	int getFirstEntertainmentGroup()
	{
		return std::find_if(hueGroups.begin(), hueGroups.end(), [](HueGroup& group) {return group.type == "Entertainment"; })->id;
	}

	void setLights(int id, float red, float green, float blue)
	{
		std::lock_guard<std::mutex> lock(dataMutex);

		for (auto& group : hueGroups)
		{
			if (group.id == id)
			{
				for (auto& light : group.lights)
				{
					light.red = red;
					light.green = green;
					light.blue = blue;
				}
			}
		}
	}
	void setLightShow(std::shared_ptr<LightShow> lg)
	{
		lightShow = lg;
	}

	void setplayShow(bool _playShow)
	{
		playShow = _playShow;
	}
	bool isPlayShow()
	{
		return playShow;
	}

	void startEntertainmentMode(int id)
	{
		entertainmentGroup = id;

		try
		{
			http_client client(strToWStr("http://" + ip + "/api/" + username));

			web::json::value data;

			http_response response = client.request(methods::PUT, strToWStr("/groups/" + std::to_string(id) + "/"), L"{\"stream\":{\"active\":true}}")
				.then([&](http_response response)
			{
				if (response.status_code() == status_codes::OK)
				{
					GLib::Out << "Entertainment mode activated\n";
				}
			}
			).wait();
		}
		catch (const std::exception &e)
		{
			GLib::Out << "Connection error: " << e.what() << "\n";
		}

		workerThread = std::make_unique<std::thread>(&Hue::worker, this);
	}

private:
	std::vector<unsigned char> getEntertainmentData()
	{
		std::lock_guard<std::mutex> lock(dataMutex);

		std::vector<unsigned char> data{
			 'H', 'u', 'e', 'S', 't', 'r', 'e', 'a', 'm', //protocol
			0x01, 0x00, //version 1.0
			0x01, //sequence number 1
			0x00, 0x00, //Reserved write 0’s
			0x00,
			0x00, // Reserved, write 0’s
		};

		const HueGroup& group = *std::find_if(hueGroups.begin(), hueGroups.end(), [&](HueGroup& group) {return group.id == entertainmentGroup; });

		for (const auto& light : group.lights)
		{
			LightColor color = lightShow && playShow ? lightShow->getState(light.id) : LightColor(1, 1, 1);

			data.push_back(0x00);
			data.push_back((light.id >> 8) & 0xff);
			data.push_back(light.id & 0xff);
			
			int red = color.red * 0xffff;
			int green = color.green * 0xffff;
			int blue = color.blue * 0xffff;

			data.push_back((red >> 8) & 0xff);
			data.push_back(red & 0xff);

			data.push_back((green >> 8) & 0xff);
			data.push_back(green & 0xff);

			data.push_back((blue >> 8) & 0xff);
			data.push_back(blue & 0xff);
		}

		return data;
	}

	void worker()
	{
		mbedtls_net_context server_fd;
		mbedtls_entropy_context entropy;
		mbedtls_ctr_drbg_context ctr_drbg;
		mbedtls_ssl_context ssl;
		mbedtls_ssl_config conf;
		mbedtls_x509_crt cacert;
		mbedtls_timing_delay_context timer;

		try
		{
			mbedtls_net_init(&server_fd);
			mbedtls_ssl_init(&ssl);
			mbedtls_ssl_config_init(&conf);
			mbedtls_x509_crt_init(&cacert);
			mbedtls_ctr_drbg_init(&ctr_drbg);
			mbedtls_entropy_init(&entropy);

			const char *pers = "dtls_client";
			mbedtlsErrorWrapper(mbedtls_ctr_drbg_seed(&ctr_drbg, mbedtls_entropy_func, &entropy, (const unsigned char *)pers, strlen(pers)));
			mbedtlsErrorWrapper(mbedtls_net_connect(&server_fd, ip.c_str(), SERVER_PORT, MBEDTLS_NET_PROTO_UDP));


			mbedtlsErrorWrapper(mbedtls_ssl_config_defaults(&conf, MBEDTLS_SSL_IS_CLIENT, MBEDTLS_SSL_TRANSPORT_DATAGRAM, MBEDTLS_SSL_PRESET_DEFAULT));

			mbedtls_ssl_conf_authmode(&conf, MBEDTLS_SSL_VERIFY_OPTIONAL);
			mbedtls_ssl_conf_ca_chain(&conf, &cacert, NULL);
			mbedtls_ssl_conf_rng(&conf, mbedtls_ctr_drbg_random, &ctr_drbg);

			mbedtlsErrorWrapper(mbedtls_ssl_setup(&ssl, &conf));

			std::vector<unsigned char> keyDecoded = hexToBytes(key);
			mbedtlsErrorWrapper(mbedtls_ssl_conf_psk(&conf, keyDecoded.data(), keyDecoded.size() * sizeof(char),
				(const unsigned char *)username.data(), username.length() * sizeof(char)));

			int ciphers[2];
			ciphers[0] = MBEDTLS_TLS_PSK_WITH_AES_128_GCM_SHA256;
			ciphers[1] = 0;
			mbedtls_ssl_conf_ciphersuites(&conf, ciphers);

			mbedtlsErrorWrapper(mbedtls_ssl_set_hostname(&ssl, SERVER_NAME));

			mbedtls_ssl_set_bio(&ssl, &server_fd, mbedtls_net_send, mbedtls_net_recv, mbedtls_net_recv_timeout);
			mbedtls_ssl_set_timer_cb(&ssl, &timer, mbedtls_timing_set_delay, mbedtls_timing_get_delay);

			for (int attempt = 0; attempt < 4; ++attempt)
			{
				mbedtls_ssl_conf_handshake_timeout(&conf, 400, 1000);
				int status;
				do status = mbedtls_ssl_handshake(&ssl);
				while (status == MBEDTLS_ERR_SSL_WANT_READ || status == MBEDTLS_ERR_SSL_WANT_WRITE);

				if (status == 0)
					break;

				std::this_thread::sleep_for(std::chrono::milliseconds(200));
			}

			GLib::Out << "Handshake complete\n";

			auto start = std::chrono::steady_clock::now();
			int updates = 0;
			while (!stop)
			{
				std::vector<unsigned char> msg = getEntertainmentData();

				int status;
				do status = mbedtls_ssl_write(&ssl, msg.data(), msg.size());
				while (status == MBEDTLS_ERR_SSL_WANT_READ || status == MBEDTLS_ERR_SSL_WANT_WRITE);

				updates++;

				std::this_thread::sleep_for(std::chrono::milliseconds(30));

				float millis = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - start).count();
				
				if (updates % 100 == 0)
				{
					//GLib::Out << int(float(updates) * 1000.0 / millis) << " light updates per second \n";
				}
			}

			/*if (ret < 0)
			{
				switch (ret)
				{
				case MBEDTLS_ERR_SSL_TIMEOUT:
					qWarning() << " timeout";
					if (retry_left-- > 0)
						goto send_request;
					goto exit;

				case MBEDTLS_ERR_SSL_PEER_CLOSE_NOTIFY:
					qWarning() << " connection was closed gracefully";
					ret = 0;
					goto close_notify;

				default:
					qWarning() << " mbedtls_ssl_read returned" << ret;
					goto exit;
				}
			}*/

			int status;
			do status = mbedtls_ssl_close_notify(&ssl);
			while (status == MBEDTLS_ERR_SSL_WANT_WRITE);
		}
		catch (std::runtime_error e)
		{
			GLib::Out << e.what() << "\n";
		}

		mbedtls_net_free(&server_fd);

		mbedtls_x509_crt_free(&cacert);
		mbedtls_ssl_free(&ssl);
		mbedtls_ssl_config_free(&conf);
		mbedtls_ctr_drbg_free(&ctr_drbg);
		mbedtls_entropy_free(&entropy);
	}

private:
	std::string ip;
	std::string username;
	std::string key;

	bool paired = false;

	std::vector<HueGroup> hueGroups;

	bool stop = false;
	std::mutex dataMutex;
	std::unique_ptr<std::thread> workerThread;

	int entertainmentGroup = -1;

	bool playShow = true;
	std::shared_ptr<LightShow> lightShow;
};