#include "stdafx.h"
#include "MessageSender.h"
#include <iostream>
#include "Messagetypes.h"

MessageSender::MessageSender()
{
}


MessageSender::~MessageSender()
{
}

void MessageSender::enqueue(Message &m)
{
	queue.push_back(m);
}

void MessageSender::getQueueElementCount()
{
	std::cout << "Queued messages: " << queue.size() << "\n";
}

std::vector<int> MessageSender::getMessageBytes()
{
	std::vector<int> bytevector;
	for (auto& message : queue) {
		for (auto& byte : KA007_Factory.getBytes(message.getParameters())){
			bytevector.push_back(byte);
		}
	}
	queue.clear();
	return bytevector;
}

std::vector<std::vector<int>> MessageSender::getMessageVectorBytes()
{
	std::vector<std::vector<int>> messagevector;
	for (auto& message : queue) {
		std::vector<int> bytevector;
		for (auto& byte : KA007_Factory.getBytes(message.getParameters())) {
			bytevector.push_back(byte);
		}
		messagevector.push_back(bytevector);
	}
	queue.clear();
	return messagevector;
}

MessageBuilder Message::make()
{
	return MessageBuilder();
}