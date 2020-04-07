
#include "stdafx.h"
#include <stdio.h> 
#include <stdlib.h> 
#include <string.h> 
#include <winsock2.h>
#include <Ws2tcpip.h>
#include <io.h>
#define MAX 80 
#define PORT "8080"
void func(SOCKET SendingSocket)
{
	char buff[MAX];
	int n;
	int BytesSent;
	for (;;) {
		memset(buff, 0, sizeof(buff));
		printf("Enter the string : ");
		n = 0;
		while ((buff[n++] = getchar()) != '\n')
			;
		//write(sockfd, buff, sizeof(buff));
		BytesSent = send(SendingSocket, buff, sizeof(buff), 0);
		if (BytesSent == SOCKET_ERROR)
			printf("Client: send() error %ld.\n", WSAGetLastError());
		else
		{
			memset(buff, 0, sizeof(buff));
			recv(SendingSocket, buff, sizeof(buff), 0);
			printf("From Server : %s", buff);
			if ((strncmp(buff, "exit", 4)) == 0) {
				printf("Client Exit...\n");
				break;
			}
		}
	}


}

int main()
{
	WSADATA wsaData;

	int iResult;

	// Initialize Winsock
	iResult = WSAStartup(MAKEWORD(2, 2), &wsaData);
	if (iResult != 0) {
		printf("WSAStartup failed: %d\n", iResult);
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
	iResult = getaddrinfo("10.0.0.2", PORT, &hints, &result);
	if (iResult != 0) {
		printf("getaddrinfo failed: %d\n", iResult);
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
		printf("Error at socket(): %ld\n", WSAGetLastError());
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
		printf("Unable to connect to server!\n");
		WSACleanup();
		return 1;
	}
	else
	{
		printf("connected to server\n");
	}

	// function for chat 
	func(ConnectSocket);

	// When you are finished sending and receiving data on socket SendingSocket,
	 // you should close the socket using the closesocket API. We will
	 // describe socket closure later in the chapter.
	if (closesocket(ConnectSocket) != 0)
	{
		printf("Client: Cannot close \"SendingSocket\" socket. Error code: %ld\n", WSAGetLastError());
	}
	else
	{
		printf("Client: Closing \"SendingSocket\" socket...\n");
	}
}
