#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif

#include <windows.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <iostream>
#include "stdafx.h"
#include <thread>
#include <chrono>
#include <atomic>

#include "socketClient.h"
#include "Communicator.h"

#pragma comment(lib, "Ws2_32.lib")

using namespace std::chrono_literals;

socketClient::socketClient(Communicator* communicator) : comm(communicator) {
	serverAddr.sin_family = AF_INET;
	serverAddr.sin_port = htons(port);
	serverAddr.sin_addr.s_addr = inet_addr(host);

	clientThread = std::thread(&socketClient::runThread, this);
}

void socketClient::runThread() {
	wsaStatus = WSAStartup(MAKEWORD(2, 2), &wsaData);
	if (wsaStatus != NO_ERROR)
	{
		DWORD dw_error = (DWORD)wsaStatus;
		fprintf(stderr, "WSAStartup failed with code %d.\n", dw_error);
		WSACleanup();
		return;
	}

	/*int iStatus = getaddrinfo(host, port, &s_hints, &servinfo);
	if ((iStatus != 0) || (servinfo == NULL))
	{
		dw_error = (DWORD)WSAGetLastError();
		fprintf(stderr, "getaddrinfo failed with code %d.\n", dw_error);
		WSACleanup();
		return;
	}*/
	
	sock = socket(AF_INET, SOCK_STREAM, 0);
	if (sock == INVALID_SOCKET)
	{
		DWORD dw_error = (DWORD)WSAGetLastError();
		fprintf(stderr, "socket failed with code %d.\n", dw_error);
		WSACleanup();
		return;
	}

	//setsockopt(sock, SOL_SOCKET, SO_SNDTIMEO, (char*)&send_timeout, sizeof(send_timeout));
	//setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, (char*)&recv_timeout, sizeof(recv_timeout));
	connStatus = connect(sock, (SOCKADDR *)&serverAddr, sizeof(serverAddr));
	if (connStatus == NO_ERROR)
	{
		sendAll(identifier.c_str());
	}

	while (true)
	{
		if (connStatus == SOCKET_ERROR)
		{
			closesocket(sock);
			std::this_thread::sleep_for(1s);
			sock = socket(AF_INET, SOCK_STREAM, 0);
			if (sock == INVALID_SOCKET)
			{
				DWORD dw_error = (DWORD)WSAGetLastError();
				fprintf(stderr, "socket failed with code %d.\n", dw_error);
				WSACleanup();
				return;
			}
			connStatus = connect(sock, (SOCKADDR *)&serverAddr, sizeof(serverAddr));
			if (connStatus == NO_ERROR)
			{
				sendAll(identifier.c_str());
			}
			continue;
		}

		std::unique_lock<std::mutex> lock(mtx);
		cv.wait(lock, [this] {return flag;});
		int bytesSent = sendAll(message_to_send.c_str());
		int bytesReceived = 0;
		char buffer[1024];
		if (bytesSent > 0)
		{
			//bytesReceived += recvAll(buffer);
			int buffer_size = sizeof(buffer);
			int size;
			int received = recv(sock, (char*)&size, sizeof(size), 0);
			if (received != sizeof(size)) {
				bytesReceived = -1;  // Failed to receive size
				break;
			}
			size = ntohl(size);  // Convert size back to host byte order

			// Check if the size fits within the buffer
			if (size > buffer_size) {
				bytesReceived = -1;  // Buffer is too small to hold the incoming data
				break;
			}

			// Now receive the actual data
			int total = 0;
			while (total < size) {
				int bytesReceived = recv(sock, buffer + total, size - total, 0);
				if (bytesReceived == SOCKET_ERROR || bytesReceived == 0) {
					bytesReceived = -1; // Error or connection closed
					break;
				}
				total += bytesReceived;
			}
			bytesReceived = total;
		}
		if (bytesReceived > 0)
		{
			{
				//std::lock_guard<std::mutex> lg(mtx);
				response_received = std::string(buffer, bytesReceived);
				flag = false;
			}
		}
		else 
		{
			connStatus = SOCKET_ERROR;
			DWORD dw_error = (DWORD)WSAGetLastError();
			{
				//std::lock_guard<std::mutex> lg(mtx);
				response_received = "";
				flag = false;
			}
		}
		cv.notify_all();
	}

	closesocket(sock);
	WSACleanup();
}

std::string socketClient::send_receive_message(std::string msg, bool wait_for_response)
{
	{
		std::lock_guard<std::mutex> lg(mtx);
		message_to_send = msg;
		flag = true;
	}
	cv.notify_all();

	if (wait_for_response)
	{
		std::unique_lock<std::mutex> lock(mtx);
		cv.wait(lock, [this] {return !flag; });
		return response_received;
	}
	else { return ""; }
}

// Helper function to send all data
int socketClient::sendAll(const char* data) {
	// Send the size of the data (4 bytes for an integer)
	int data_len = strlen(data);
	int total = 0;
	int size = htonl(data_len);  // Convert size to network byte order
	int sent = send(sock, (char*)&size, sizeof(size), 0);
	if (sent != sizeof(size)) {
		return -1;  // Failed to send the size
	}

	// Send the actual data
	while (total < data_len) {
		int bytes_sent = send(sock, data + total, data_len - total, 0);
		if (bytes_sent == SOCKET_ERROR) {
			return -1;  // Error during sending
		}
		total += bytes_sent;
	}
	return total;  // Total bytes sent
}

// Helper function to receive all data
int socketClient::recvAll(char* buffer) {
	// First, receive the size of the data (4 bytes)
	int buffer_size = sizeof(buffer);
	int size;
	int received = recv(sock, (char*)&size, sizeof(size), 0);
	if (received != sizeof(size)) {
		return -1;  // Failed to receive size
	}
	size = ntohl(size);  // Convert size back to host byte order

	// Check if the size fits within the buffer
	if (size > buffer_size) {
		return -1;  // Buffer is too small to hold the incoming data
	}

	// Now receive the actual data
	int total = 0;
	while (total < size) {
		int bytes_received = recv(sock, buffer + total, size - total, 0);
		if (bytes_received == SOCKET_ERROR || bytes_received == 0) {
			return -1;  // Error or connection closed
		}
		total += bytes_received;
	}
	return total;  // Total bytes received
}



/*namespace socketClient
{
	SOCKET initialize()
	{
		int wsaStatus; //check errors
		WSADATA WSAData;
		wsaStatus = WSAStartup(MAKEWORD(2, 0), &WSAData);
		if (wsaStatus != NO_ERROR) {
			std::cout << "WSA Startup failed with error : " << wsaStatus;
			return INVALID_SOCKET;
		}

		// Create socket
		SOCKET sock = socket(AF_INET, SOCK_STREAM, 0);
		if (sock == INVALID_SOCKET) {
			std::cerr << "Socket creation failed!" << std::endl;
			WSACleanup();
		}
		return sock;
	}

	bool connect(SOCKET sock, UINT port, const char* host)
	{
		struct sockaddr_in serverAddr;
		serverAddr.sin_family = AF_INET;
		serverAddr.sin_port = htons(port);
		serverAddr.sin_addr.s_addr = inet_addr(host);

		if (connect(sock, (struct sockaddr*)&serverAddr, sizeof(serverAddr)) < 0) {
			//std::cerr << "Connection to server failed!" << std::endl;
			closesocket(sock);
			return false;
		}

		return true;
	}

	// Helper function to send all data
	int sendAll(SOCKET sock, const char* data, int data_len, DWORD timeout) {
		// set timeout
		setsockopt(sock, SOL_SOCKET, SO_SNDTIMEO, (char*)&timeout, sizeof(timeout));

		// Send the size of the data (4 bytes for an integer)
		int total = 0;
		int size = htonl(data_len);  // Convert size to network byte order
		int sent = send(sock, (char*)&size, sizeof(size), 0);
		if (sent != sizeof(size)) {
			return -1;  // Failed to send the size
		}

		// Send the actual data
		while (total < data_len) {
			int bytes_sent = send(sock, data + total, data_len - total, 0);
			if (bytes_sent == SOCKET_ERROR) {
				return -1;  // Error during sending
			}
			total += bytes_sent;
		}
		return total;  // Total bytes sent
	}

	// Helper function to receive all data
	int recvAll(SOCKET sock, char* buffer, int buffer_size, DWORD timeout) {
		// set timeout
		setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, (char*)&timeout, sizeof(timeout));

		// First, receive the size of the data (4 bytes)
		int size;
		int received = recv(sock, (char*)&size, sizeof(size), 0);
		if (received != sizeof(size)) {
			return -1;  // Failed to receive size
		}
		size = ntohl(size);  // Convert size back to host byte order

		// Check if the size fits within the buffer
		if (size > buffer_size) {
			return -1;  // Buffer is too small to hold the incoming data
		}

		// Now receive the actual data
		int total = 0;
		while (total < size) {
			int bytes_received = recv(sock, buffer + total, size - total, 0);
			if (bytes_received == SOCKET_ERROR || bytes_received == 0) {
				return -1;  // Error or connection closed
			}
			total += bytes_received;
		}
		return total;  // Total bytes received
	}

	void close(SOCKET sock)
	{
		closesocket(sock);
		WSACleanup();
	}


	// Helper function to set a socket to non-blocking mode
	bool set_non_blocking(SOCKET sock, bool non_blocking) {
		u_long mode = non_blocking ? 1 : 0;
		return ioctlsocket(sock, FIONBIO, &mode) == 0;
	}

	// Function to connect with a timeout
	bool connectWithTimeout(SOCKET sock, int timeout_ms, UINT port, const char* host) {
		// Set the socket to non-blocking mode
		if (!set_non_blocking(sock, true)) {
			std::cerr << "Failed to set non-blocking mode" << std::endl;
			return false;
		}

		struct sockaddr_in serverAddr;
		serverAddr.sin_family = AF_INET;
		serverAddr.sin_port = htons(port);
		serverAddr.sin_addr.s_addr = inet_addr(host);

		// Attempt to connect (this will return immediately because the socket is non-blocking)
		int result = connect(sock, (struct sockaddr*)&serverAddr, sizeof(serverAddr));

		// Check if connect is in progress (non-blocking sockets return SOCKET_ERROR with WSAEWOULDBLOCK)
		if (result == SOCKET_ERROR && WSAGetLastError() == WSAEWOULDBLOCK) {
			// Set up the timeout
			fd_set writefds;
			FD_ZERO(&writefds);
			FD_SET(sock, &writefds);

			timeval timeout;
			timeout.tv_sec = timeout_ms / 1000;
			timeout.tv_usec = (timeout_ms % 1000) * 1000;

			// Wait for the socket to become writable or for the timeout
			result = select(0, nullptr, &writefds, nullptr, &timeout);
			if (result == 0) {
				std::cerr << "Connect timeout!" << std::endl;
				set_non_blocking(sock, false);  // Always reset to blocking
				return false;  // Timeout occurred
			}
			else if (result > 0) {
				// Check if the socket is now writable (i.e., connection successful)
				int optval;
				int optlen = sizeof(optval);
				if (getsockopt(sock, SOL_SOCKET, SO_ERROR, (char*)&optval, &optlen) == 0 && optval == 0) {
					std::cout << "Connected successfully!" << std::endl;
					set_non_blocking(sock, false);  // Always reset to blocking
					return true;
				}
				else {
					std::cerr << "Connection failed after select: " << optval << std::endl;
					set_non_blocking(sock, false);  // Always reset to blocking
					return false;
				}
			}
			else {
				std::cerr << "Select error!" << std::endl;
				set_non_blocking(sock, false);  // Always reset to blocking
				return false;  // Some error occurred
			}
		}
		else if (result == 0) {
			std::cout << "Connected immediately!" << std::endl;
			set_non_blocking(sock, false);  // Always reset to blocking
			return true;  // Connected immediately
		}
		else {
			std::cerr << "Connection error: " << WSAGetLastError() << std::endl;
			set_non_blocking(sock, false);  // Always reset to blocking
			return false;  // Some other error occurred
		}

		// Restore the socket to blocking mode
		set_non_blocking(sock, false);
	}
}*/