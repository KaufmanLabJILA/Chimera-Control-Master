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

int ZynqTCP::writeDIO(std::vector<std::array<char[DIO_LEN_BYTE_BUF], 1>> TtlSnapshots)
{

	char buff[ZYNQ_MAX_BUFF];
	memset(buff, 0, sizeof(buff));
	std::string TtlSnapshot_str;

	int BytesSent = 0;

	sprintf_s(buff, ZYNQ_MAX_BUFF, "DIOseq_%u", TtlSnapshots.size());

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
			BytesSent = send(ConnectSocket, TtlSnapshot[0], DIO_LEN_BYTE_BUF, 0);
			if (BytesSent == SOCKET_ERROR)
			{
				thrower("Unable to send message to server!");
				return 1;
			}
		}

		return 0;

	}

	

}

int ZynqTCP::writeDACs(std::array<std::vector<char[DAC_LEN_BYTE_BUF]>,2> dacSnapshots)
{
	char buff[ZYNQ_MAX_BUFF];
	memset(buff, 0, sizeof(buff));
	std::string TtlSnapshot_str;

	int BytesSent = 0;

	sprintf_s(buff, ZYNQ_MAX_BUFF, "DACseq_%u_%u", dacSnapshots[0].size(), dacSnapshots[1].size());

	BytesSent = send(ConnectSocket, buff, sizeof(buff), 0);
	if (BytesSent == SOCKET_ERROR)
	{
		thrower("Unable to send message to server!");
		return 1;
	}
	else
	{
		memset(buff, 0, sizeof(buff));

		for (auto dac0Snapshot : dacSnapshots[0])
		{
			BytesSent = send(ConnectSocket, dac0Snapshot, DAC_LEN_BYTE_BUF, 0);
			if (BytesSent == SOCKET_ERROR)
			{
				thrower("Unable to send message to server!");
				return 1;
			}
		}

		for (auto dac1Snapshot : dacSnapshots[1])
		{
			BytesSent = send(ConnectSocket, dac1Snapshot, DAC_LEN_BYTE_BUF, 0);
			if (BytesSent == SOCKET_ERROR)
			{
				thrower("Unable to send message to server!");
				return 1;
			}
		}

		return 0;
	}

}