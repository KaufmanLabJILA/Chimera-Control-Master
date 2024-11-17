#define WIN32_LEAN_AND_MEAN

#include <winsock2.h>
#include <iostream>
#include "stdafx.h"

#include "socketClient.h"

#pragma comment(lib, "Ws2_32.lib")

namespace socketClient
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

	/*bool connect(SOCKET sock, UINT port, const char* host)
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
	}*/

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
}