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
	timeType time;

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
    double time;
	unsigned short line;
	double edacVoltageValue1;
    double edacVoltageValue2;
    double edacVoltageValue3;
    double edacVoltageValue4;
    double edacVoltageValue5;
    double edacVoltageValue6;
    double edacVoltageValue7;
    double edacVoltageValue8;
};

struct EDacSnapshot
{
	double time;
	double edacVoltageValue1;
    double edacVoltageValue2;
    double edacVoltageValue3;
    double edacVoltageValue4;
    double edacVoltageValue5;
    double edacVoltageValue6;
    double edacVoltageValue7;
    double edacVoltageValue8;
};

struct EDacChannelSnapshot
{
	double time;
	double edacVoltageValue1;
    double edacVoltageValue2;
    double edacVoltageValue3;
    double edacVoltageValue4;
    double edacVoltageValue5;
    double edacVoltageValue6;
    double edacVoltageValue7;
    double edacVoltageValue8;
};

///Copied from the DAC Struct class


