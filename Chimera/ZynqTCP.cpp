#include "stdafx.h"
#include "ZynqTCP.h"

ZynqTCP::ZynqTCP()
{
	if (!ZYNQ_SAFEMODE) {
		ConnectSocket = INVALID_SOCKET;
	}

}

ZynqTCP::~ZynqTCP() {
	//disconnect();
}

int ZynqTCP::sendWithCatch(const SOCKET& socket, const char* byte_buf, int buffLen, int flags)
{
	int BytesSent;
	BytesSent = send(socket, byte_buf, buffLen, flags);
	if (BytesSent == SOCKET_ERROR)
	{
		thrower("Unable to send message:" + str(byte_buf) + ", with prescribed length:" + str(buffLen) + " to server!");
		return 1; /*bad*/
	}
	return 0; /*good*/
}

void ZynqTCP::disconnect() {
	if (ZYNQ_SAFEMODE) {
		return;
	}

	char buff[ZYNQ_MAX_BUFF];
	memset(buff, 0, sizeof(buff));
	std::string  end_command = "end_0";

	//int n = sprintf_s(buff, ZYNQ_MAX_BUFF, "%s", end_command);
	for (size_t i = 0; i < end_command.length(); i++)
	{
		buff[i] = end_command[i];
	}

	sendWithCatch(ConnectSocket, buff, sizeof(buff), 0);
	closesocket(ConnectSocket);
}

int ZynqTCP::connectTCP(const char ip_address[])
{

	if (ZYNQ_SAFEMODE) {
		return 0;
	}

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

int ZynqTCP::writeCommand(std::string command)
{
	char buff[ZYNQ_MAX_BUFF];
	memset(buff, 0, sizeof(buff));
	if (command.length() > ZYNQ_MAX_BUFF)
	{
		thrower("Command exceeding ZYNQ_MAX_BUFF: " + std::to_string(ZYNQ_MAX_BUFF));
		return 1;
	}
	for (size_t i = 0; i < command.length(); i++)
	{
		buff[i] = command[i];
	}

	return sendWithCatch(ConnectSocket, buff, sizeof(buff), 0);
}

int ZynqTCP::writeDIO(std::vector<std::array<char[DIO_LEN_BYTE_BUF], 1>> TtlSnapshots)
{

	char buff[ZYNQ_MAX_BUFF];
	memset(buff, 0, sizeof(buff));

	int BytesSent = 0;

	char command[ZYNQ_MAX_BUFF];
	sprintf_s(command, ZYNQ_MAX_BUFF, "DIOseq_%u", TtlSnapshots.size());
	//sprintf_s(buff, ZYNQ_MAX_BUFF, "DIOseq_%u", TtlSnapshots.size());
	for (size_t i = 0; i < strlen(command); i++)
	{
		buff[i] = command[i];
	}

	int err = sendWithCatch(ConnectSocket, buff, sizeof(buff), 0);
	if (err == 1 /*bad*/) {
		return 1;
	}
	for (auto TtlSnapshot : TtlSnapshots)
	{
		int err2 = sendWithCatch(ConnectSocket, TtlSnapshot[0], DIO_LEN_BYTE_BUF, 0);
		if (err2 == 1) {
			return 1;
		}
	}
	return 0;

	

}

int ZynqTCP::writeDACs(std::vector<DacChannelSnapshot> dacChannelSnapshots)
{
	char buff[ZYNQ_MAX_BUFF];
	char buffCommand[ZYNQ_MAX_BUFF];
	memset(buff, 0, sizeof(buff));

	int snapIndex = 0;

	char byte_buf[DAC_LEN_BYTE_BUF];
	//char command[DAC_LEN_BYTE_BUF];
	unsigned int time, duration;
	unsigned short channel;
	double start, end;

	int BytesSent = 0;

	sprintf_s(buffCommand, ZYNQ_MAX_BUFF, "DACseq_%u", dacChannelSnapshots.size());
	for (size_t i = 0; i < strlen(buffCommand); i++)
	{
		buff[i] = buffCommand[i];
	}

	int err = sendWithCatch(ConnectSocket, buff, sizeof(buff), 0);
	if (err == 1 /*bad*/) {
		return 1;
	}

	for (int i = 0; i < dacChannelSnapshots.size(); ++i)
	{
		DacChannelSnapshot snapshot = dacChannelSnapshots[i];

		time = (unsigned int)(snapshot.time * timeConv);
		channel = snapshot.channel;
		start = snapshot.dacValue;
		end = snapshot.dacEndValue;
		duration = (unsigned int)(snapshot.dacRampTime * timeConvDAC + 0.5);

		sprintf_s(byte_buf, DAC_LEN_BYTE_BUF, "t%08X_c%04X_s%07.4f_e%07.4f_d%08x", time, channel, start, end, duration);
		int err2 = sendWithCatch(ConnectSocket, byte_buf, DAC_LEN_BYTE_BUF, 0);
		if (err2 == 1) {
			return 1;
		}

	}

	return 0;


}

int ZynqTCP::writeDDSs(std::vector<DDSChannelSnapshot> ddsChannelSnapshots)
{
	char buff[ZYNQ_MAX_BUFF];
	char buffCommand[ZYNQ_MAX_BUFF];
	memset(buff, 0, sizeof(buff));

	int snapIndex = 0;

	char byte_buf[DDS_LEN_BYTE_BUF];
	char byte_bufCommand[DDS_LEN_BYTE_BUF];
	memset(byte_buf, 0, sizeof(byte_buf));
	unsigned int time, duration;
	unsigned short channel;
	char type;
	double start, end;

	sprintf_s(buffCommand, ZYNQ_MAX_BUFF, "DDSseq_%u", ddsChannelSnapshots.size());
	for (size_t i = 0; i < strlen(buffCommand); i++)
	{
		buff[i] = buffCommand[i];
	}

	int err = sendWithCatch(ConnectSocket, buff, sizeof(buff), 0);
	if (err == 1 /*bad*/) {
		return 1;
	}

	for (int i = 0; i < ddsChannelSnapshots.size(); ++i)
	{
		DDSChannelSnapshot snapshot = ddsChannelSnapshots[i];

		time = (unsigned int)(snapshot.time * timeConv);
		type = snapshot.ampOrFreq;
		channel = snapshot.channel;
		start = snapshot.val;
		end = snapshot.endVal;
		duration = (unsigned int)(snapshot.rampTime * timeConvDDS);

		sprintf_s(byte_bufCommand, DDS_LEN_BYTE_BUF, "t%08X_c%04X_%c_s%07.3f_e%07.3f_d%08x",
			time, channel, type, start, end, duration);
		for (size_t inc = 0; inc < strlen(byte_bufCommand); inc++)
		{
			byte_buf[inc] = byte_bufCommand[inc];
		}
		int err2 = sendWithCatch(ConnectSocket, byte_buf, DDS_LEN_BYTE_BUF, 0);
		if (err2 == 1) {
			return 1;
		}
	}

	return 0;

}