#pragma once

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif

#include <windows.h>
#include <winsock2.h>
#include <ws2tcpip.h>

#include <string>
#include <thread>
#include <atomic>

#include "Communicator.h"

/*namespace socketClient
{
	SOCKET initialize();
	//bool connect(SOCKET sock, UINT port = 8000, const char* host = "127.0.0.1");
	int sendAll(SOCKET sock, const char* data, int data_len, DWORD timeout = 0.1 * 1000);
	int recvAll(SOCKET sock, char* buffer, int buffer_size, DWORD timeout = 10 * 1000);
	void close(SOCKET sock);

	bool set_non_blocking(SOCKET sock, bool non_blocking);
	bool connectWithTimeout(SOCKET sock, int timeout_ms, UINT port = 8000, const char* host = "127.0.0.1");
}*/

class socketClient
{
	public:
		socketClient(Communicator* communicator);

		std::string send_receive_message(std::string msg, bool wait_for_response);

	private:
		WSADATA wsaData;
		std::thread clientThread;
		SOCKET sock = INVALID_SOCKET;
		std::mutex mtx;
		std::condition_variable cv;

		const char* host = "127.0.0.1";
		UINT port = 12345;
		sockaddr_in serverAddr;

		DWORD send_timeout = 1000;
		DWORD recv_timeout = 60 * 1000;

		int wsaStatus = 1;
		int connStatus = SOCKET_ERROR;

		bool flag = false;
		std::string message_to_send = "";
		std::string response_received = "";

		int sendAll(const char* data);
		int recvAll(char* buffer);

		void runThread();

		Communicator* comm;

		std::string identifier = "chimera";
};