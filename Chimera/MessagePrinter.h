#pragma once
#include <iostream>
#include <vector>
#include <array>

class MessagePrinter
{
public:
	void callback(int byte);
private:
	std::vector<int> received;
	bool isReceiving;
	int expectedBytes;
	int byteCount;

	std::array<int, 4> header;
	int messagetype;
	int messagesource;
	int messagelength;
	int messagedata_h;
	int messagedata_l;
	int headeridx;
};
