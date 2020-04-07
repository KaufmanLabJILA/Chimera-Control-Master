#include "stdafx.h"
#include "ZynqTCP.h"

ZynqTCP::ZynqTCP()
{
	if (!ZNYQ_SAFEMODE) {
		//connectTCP(ip_address);
	}

}

ZynqTCP::~ZynqTCP() {
	disconnect();
}

void ZynqTCP::disconnect() {
	if (!ZNYQ_SAFEMODE) {
		closesocket(ConnectSocket);
	}
}

int ZynqTCP::connectTCP(const char ip_address[])
{

	WSADATA wsaData;

	int iResult;

	// Initialize Winsock
	iResult = WSAStartup(MAKEWORD(2, 2), &wsaData);
	if (iResult != 0) {
		thrower("WSAStartup failed");
		return 1;
	}

	struct addrinfo *result = NULL,
		*ptr = NULL,
		hints;

	ZeroMemory(&hints, sizeof(hints));
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_protocol = IPPROTO_TCP;

	// Resolve the server address and port
	iResult = getaddrinfo(ip_address, ZYNQ_PORT, &hints, &result);
	if (iResult != 0) {
		thrower("getaddrinfo failed");
		WSACleanup();
		return 1;
	}

	SOCKET ConnectSocket = INVALID_SOCKET;

	// Attempt to connect to the first address returned by
// the call to getaddrinfo
	ptr = result;

	// Create a SOCKET for connecting to server
	ConnectSocket = socket(ptr->ai_family, ptr->ai_socktype,
		ptr->ai_protocol);

	if (ConnectSocket == INVALID_SOCKET) {
		thrower("Error at socket()");
		freeaddrinfo(result);
		WSACleanup();
		return 1;
	}


	// Connect to server.
	iResult = connect(ConnectSocket, ptr->ai_addr, (int)ptr->ai_addrlen);
	if (iResult == SOCKET_ERROR) {
		closesocket(ConnectSocket);
		ConnectSocket = INVALID_SOCKET;
	}

	freeaddrinfo(result);

	if (ConnectSocket == INVALID_SOCKET) {
		thrower("Unable to connect to server!");
		WSACleanup();
		return 1;
	}
	else
	{
		char command[] = "a test command";
		char buff[ZYNQ_MAX_BUFF];
		memset(buff, 0, sizeof(buff));

		int n;
		n = sprintf_s(buff, ZYNQ_MAX_BUFF, "4000_%s", command);

		int BytesSent;
		BytesSent = send(ConnectSocket, buff, sizeof(buff), 0);
		if (BytesSent == SOCKET_ERROR)
		{
			thrower("Unable to send message to server!");
			return 0;
		}
		else
		{
			memset(buff, 0, sizeof(buff));
			return sizeof(buff);
		}
		
	}

}