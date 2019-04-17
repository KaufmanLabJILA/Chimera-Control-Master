#pragma once
#include <memory>
#include <sstream>
#include <string>
#include <vector>
#include "Messagetypes.h"

class MessageSender
{
public:
	MessageSender();
	~MessageSender();
	void enqueue(Message &m);
	void getQueueElementCount();
	std::vector<int> getMessageBytes();
private:
	std::vector<Message> queue;
	KA007_MessageFactory KA007_Factory;
};