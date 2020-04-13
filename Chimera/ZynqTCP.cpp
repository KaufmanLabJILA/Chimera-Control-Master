#include "stdafx.h"
#include "ZynqTCP.h"

ZynqTCP::ZynqTCP()
{
	if (!ZNYQ_SAFEMODE) {
		//connectTCP(ip_address);
	}

}

ZynqTCP::~ZynqTCP() {
	//disconnect();
}

void ZynqTCP::disconnect() {
	if (!ZNYQ_SAFEMODE) {
		char buff[ZYNQ_MAX_BUFF];
		memset(buff, 0, sizeof(buff));
		char end_command[] = "end_0";

		int n = sprintf_s(buff, ZYNQ_MAX_BUFF, "%s", end_command);

		int BytesSent;
		BytesSent = send(ConnectSocket, buff, sizeof(buff), 0);
		if (BytesSent == SOCKET_ERROR)
		{
			thrower("Unable to send end message to server!");
		}
		else
		{
			memset(buff, 0, sizeof(buff));
		}
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

	ConnectSocket = INVALID_SOCKET;

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
		return 0;
	}

}

int ZynqTCP::writeDIO(std::vector<std::array<char[DIO_LEN_BYTE_BUF], 3>> TtlSnapshots)
{

	char buff[ZYNQ_MAX_BUFF];
	char buff_snapshot[DIO_LEN_BYTE_BUF * 3 + 1];
	memset(buff, 0, sizeof(buff));
	memset(buff_snapshot, 0, sizeof(buff_snapshot));
	std::string TtlSnapshot_str;

	int BytesSent = 0;

	sprintf_s(buff, ZYNQ_MAX_BUFF, "DIO_%u", TtlSnapshots.size());
	//errBox(buff);

	BytesSent = send(ConnectSocket, buff, sizeof(buff), 0);
	if (BytesSent == SOCKET_ERROR)
	{
		thrower("Unable to send message to server!");
		return 1;
	}
	else
	{
		memset(buff, 0, sizeof(buff));

		for (auto TtlSnapshot : TtlSnapshots)
		{
			/*strcpy(buff_snapshot, TtlSnapshot[0]);
			strcat(buff_snapshot, TtlSnapshot[1]);
			strcat(buff_snapshot, TtlSnapshot[2]);*/
			//TtlSnapshot_str = std::string(TtlSnapshot[0]) + std::string(TtlSnapshot[1]) + std::string(TtlSnapshot[2]);
			//sprintf_s(buff_snapshot, DIO_LEN_BYTE_BUF * 3, "%s", TtlSnapshot[0], TtlSnapshot[1], TtlSnapshot[2]);
			BytesSent = send(ConnectSocket, TtlSnapshot[0], sizeof(TtlSnapshot[0]), 0);
			BytesSent = send(ConnectSocket, TtlSnapshot[1], sizeof(TtlSnapshot[1]), 0);
			BytesSent = send(ConnectSocket, TtlSnapshot[2], sizeof(TtlSnapshot[2]), 0);
			if (BytesSent == SOCKET_ERROR)
			{
				thrower("Unable to send message to server!");
				return 1;
			}
		}

		return 0;

	}

	

}