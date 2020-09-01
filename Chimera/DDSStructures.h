#pragma once

#include "Expression.h"
#include "commonTypes.h"
#include <array>

struct DDSCommandForm
{
	// can either be "ddsamp", "ddsfreq", "ddslinspaceamp", "ddslinspacefreq", "ddsrampamp", or "ddsrampfreq"
	std::string commandName;

	unsigned short line;
	timeType time;

	Expression initVal;
	Expression finalVal;
	Expression rampTime;
	Expression numSteps;
};

struct DDSCommand
{
	unsigned short line;
	double time;
	double amp;
	double freq;
	double endAmp;
	double endFreq;
	double rampTime;
};


struct DDSSnapshot
{
	double time;
	std::array<std::array<double, 2>, 12> ddsValues;
	std::array<std::array<double, 2>, 12> ddsEndValues;
	std::array<double, 12> ddsRampTimes;
};

struct DDSChannelSnapshot
{
	char ampOrFreq;
	unsigned short channel;
	double time;
	double val;
	double endVal;
	double rampTime;
};
