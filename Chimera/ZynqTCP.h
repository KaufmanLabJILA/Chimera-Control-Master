#pragma once

#include "stdafx.h"
#include <stdio.h> 
#include <stdlib.h> 
#include <string.h> 
#include <winsock2.h>
#include <Ws2tcpip.h>
#include <io.h>
#include "stdint.h"

class ZynqTCP
{
private:
	SOCKET ConnectSocket;
public:
	ZynqTCP();
	~ZynqTCP();
	void disconnect();
	int connectTCP(const char ip_address[]);

};