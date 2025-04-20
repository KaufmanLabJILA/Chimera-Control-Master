#pragma once
#include "stdafx.h"

#define WIN32_LEAN_AND_MEAN 

#include <windows.h>
#include <winsock2.h>
#include <ws2tcpip.h>

#include <thread>
#include <atomic>
#include <mutex>
#include <future>
#include <nlohmann/json.hpp>
using json = nlohmann::json;
#include "Communicator.h"

class socketClient
{
public:
	socketClient(const char* host, const char* port, Communicator* communicator);
	~socketClient();

	void start(HWND hwnd);

	json request(std::string id, std::string type, json data = {}, int timeout_s = 0.1);

private:
	void runthread();
	void runheartbeat();

	int sendall(const char* buf, int len);
	int recvall(char* buf);

	std::string formatmessage(std::string type, json data = {});

	const char* HOST;
	const char* PORT;

	WSADATA wsaData;
	struct addrinfo *servinfo;
	SOCKET sock;
	DWORD timeout = 5000; // 1 second
	int heartbeat_interval = 1;
	std::atomic_bool connected = false;

	std::mutex m;
	std::unordered_map<std::string, std::promise<json>> pending_requests;

	std::thread t1, t2;

	Communicator* comm;

	HWND hwnd = nullptr;
};