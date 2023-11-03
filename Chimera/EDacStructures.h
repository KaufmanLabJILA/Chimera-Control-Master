#pragma once

#include "Expression.h"
#include "commonTypes.h"
#include <array>
#include <string>

struct EDacCommandForm
{
	// can be "edac
	std::string commandName;

	unsigned short line;
	//timeType time;

	Expression edacChannelName;
	Expression edacVoltageValue1;
	Expression edacVoltageValue2;
	Expression edacVoltageValue3;
	Expression edacVoltageValue4;
    Expression edacVoltageValue5;
	Expression edacVoltageValue6;
	Expression edacVoltageValue7;
	Expression edacVoltageValue8;
};

struct EDacCommand
{
	unsigned short line;
	std::string edacVoltageValue1;
    std::string edacVoltageValue2;
    std::string edacVoltageValue3;
    std::string edacVoltageValue4;
    std::string edacVoltageValue5;
    std::string edacVoltageValue6;
    std::string edacVoltageValue7;
    std::string edacVoltageValue8;
};

///Copied from the DAC Struct class


