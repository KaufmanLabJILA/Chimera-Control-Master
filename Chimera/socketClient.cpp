#define WIN32_LEAN_AND_MEAN 

#include <windows.h>
#include <winsock2.h>
#include <ws2tcpip.h>

#pragma comment(lib, "Ws2_32.lib")
#include "stdafx.h"
#include <thread>
#include <iostream>
#include <chrono>
#include <future>
#include <nlohmann/json.hpp>
using json = nlohmann::json;

#include "socketClient.h"

socketClient::socketClient(const char* host, const char* port, Communicator* communicator) : comm(communicator)
{
	HOST = host;
	PORT = port;
}

socketClient::~socketClient()
{
	WSACleanup();
	freeaddrinfo(servinfo);
	if (t1.joinable()) t1.join();
	if (t2.joinable()) t2.join();
}

void socketClient::start(HWND hwnd)
{
	this->hwnd = hwnd;

	int wsastatus = WSAStartup(MAKEWORD(2, 2), &wsaData);
	if (wsastatus != 0) {
		comm->sendStatus("WSA startup failed with code: " + std::to_string(wsastatus) + ". Socket client cannot be started.\n");
		return;
	}

	int status;
	struct addrinfo hints;

	memset(&hints, 0, sizeof hints);
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_STREAM;

	if ((status = getaddrinfo(HOST, PORT, &hints, &servinfo)) != 0) {
		std::string error = gai_strerror(status);
		comm->sendStatus("Error getting address info: " + error + ". Socket client cannot be started.\n");
		return;
	}

	t1 = std::thread(&socketClient::runthread, this);
	t2 = std::thread(&socketClient::runheartbeat, this);
}


void socketClient::runthread()
{
	while (true)
	{
		sock = socket(servinfo->ai_family, servinfo->ai_socktype, servinfo->ai_protocol);
		int status = connect(sock, servinfo->ai_addr, servinfo->ai_addrlen);
		if (status == 0)
		{
			setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, (const char*)&timeout, sizeof(timeout));
			setsockopt(sock, SOL_SOCKET, SO_SNDTIMEO, (const char*)&timeout, sizeof(timeout));
			connected = true;
			comm->sendStatus("Successfully connected to server.\n");
			while (connected.load())
			{
				char buf[2048];
				int received = recvall(buf);
				if (received <= 0)
				{
					connected = false;
					WSAGetLastError();
					break;
				}
				std::string msg_str(buf, received);
				json msg = json::parse(msg_str);
				if (msg["type"] == "heartbeat"){ continue; }
				comm->sendStatus("Message received: " + msg["type"].get<std::string>() + ".\n");
				{
					std::lock_guard<std::mutex> lock(m);
					auto it = pending_requests.find(msg["type"]);
					if (it != pending_requests.end())
					{
						it->second.set_value(msg["data"]);
					}
				}
			}
		}
		closesocket(sock);
		std::this_thread::sleep_for(std::chrono::seconds(heartbeat_interval));
	}
}

void socketClient::runheartbeat()
{
	while (true)
	{
		if (connected.load())
		{
			std::string msg = formatmessage("heartbeat");
			int sent = sendall(msg.data(), msg.size());
			if (sent == -1)
			{
				connected = false;
				WSAGetLastError();
			}
		}
		std::this_thread::sleep_for(std::chrono::seconds(heartbeat_interval));
	}
}

json socketClient::request(std::string id, std::string type, json data, int timeout_s)
{
	json response;
	if (!connected.load())
	{
		if (!t1.joinable())
		{
			comm->sendStatus("Socket client failed to start. Unable to send request: " + type + ".\n");
		}
		else
		{
			comm->sendStatus("No connection to server. Unable to send request: " + type + ".\n");
		}
		response = {};
		return response;
	}

	std::promise<json> prom;
	std::future<json> future = prom.get_future();
	{
		std::lock_guard<std::mutex> lock(m);
		pending_requests[id] = std::move(prom);
	}

	std::string msg = formatmessage(type, data);
	sendall(msg.data(), msg.size());

	if (future.wait_for(std::chrono::seconds(timeout_s)) == std::future_status::ready)
	{
		response = future.get();
		comm->sendStatus("Received response to request: " + response.dump() + ".\n");
	}
	else { 
		response = {};
		comm->sendStatus("Timeout waiting for response.\n");
	}
	{
		std::lock_guard<std::mutex> lock(m);
		pending_requests.erase(id);
	}
	return response;
}


std::string socketClient::formatmessage(std::string type, json data)
{
	json msg = { {"from", "chimera"},
		{"type", type},
		{"data", data}
	};
	return msg.dump();
}


int socketClient::sendall(const char* buf, int len)
{
	int total = 0;
	int n;

	uint32_t data_len = htonl(len);
	int result = send(sock, (const char*)&data_len, sizeof(data_len), 0);
	if (result != sizeof(data_len)) { return -1; }

	while (total < len) {
		n = send(sock, buf + total, len - total, 0);
		if (n == -1) { break; }
		total += n;
	}

	return n == -1 ? -1 : total;
}

int socketClient::recvall(char* buf)
{
	int size;
	int received = recv(sock, (char*)&size, sizeof(size), 0);
	if (received <= 0) { return received; }

	int bytes_received = 0;
	int bytes_remaining = ntohl(size);
	while (bytes_remaining > 0)
	{
		int received = recv(sock, buf + bytes_received, bytes_remaining, 0);
		if (received <= 0) { return received; }
		bytes_received += received;
		bytes_remaining -= received;
	}
	return bytes_received;
}