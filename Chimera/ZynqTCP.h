#pragma once

#include <stdio.h> 
#include <stdlib.h> 
#include <string.h> 
#include <winsock2.h>
#include <Ws2tcpip.h>
#include <io.h>
#include "stdint.h"
#include "DacStructures.h"
#include "DDSStructures.h"
#include "DioStructures.h"

class ZynqTCP
{
private:
	SOCKET ConnectSocket;

	const unsigned int timeConv = 100000; // SEQ time given in multiples of 10 ns, 1/DIO_TIME_RESOLUTION
	const unsigned int timeConvDAC = 100000;
	const unsigned int dacRes = 0xffff; //16 bit dac resolution
public:
	ZynqTCP();
	~ZynqTCP();
	void disconnect();
	int sendWithCatch(const SOCKET& socket, const char* byte_buf, int buffLen, int flags);
	int connectTCP(const char ip_address[]);
	int writeDIO(std::vector<std::array<char[DIO_LEN_BYTE_BUF], 1>> TtlSnapshots);
	int writeDACs(std::vector<DacChannelSnapshot> dacSnapshots);
	int writeDDSs(std::vector<DDSChannelSnapshot> ddsSnapshots);
	int writeCommand(std::string command);
};