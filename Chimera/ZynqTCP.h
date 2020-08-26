#pragma once

#include <stdio.h> 
#include <stdlib.h> 
#include <string.h> 
#include <winsock2.h>
#include <Ws2tcpip.h>
#include <io.h>
#include "stdint.h"
#include "DacStructures.h"
#include "DioStructures.h"

class ZynqTCP
{
private:
	SOCKET ConnectSocket;
public:
	ZynqTCP();
	~ZynqTCP();
	void disconnect();
	int connectTCP(const char ip_address[]);
	int writeDIO(std::vector<std::array<char[DIO_LEN_BYTE_BUF], 1>> TtlSnapshots);
	int writeDACs(std::vector<DacChannelSnapshot> dacSnapshots);
};