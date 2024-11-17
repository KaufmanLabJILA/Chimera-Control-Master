#pragma once
#include <string>

namespace socketClient
{
	SOCKET initialize();
	//bool connect(SOCKET sock, UINT port = 8000, const char* host = "127.0.0.1");
	int sendAll(SOCKET sock, const char* data, int data_len, DWORD timeout = 0.1 * 1000);
	int recvAll(SOCKET sock, char* buffer, int buffer_size, DWORD timeout = 10 * 1000);
	void close(SOCKET sock);

	bool set_non_blocking(SOCKET sock, bool non_blocking);
	bool connectWithTimeout(SOCKET sock, int timeout_ms, UINT port = 8000, const char* host = "127.0.0.1");
}