#pragma once

#include "Expression.h"
#include "commonTypes.h"
#include <array>
#include "repeatManager.h"

struct DacCommandForm
{
	// can either be "dac", "dacarange", "daclinspace", "daccosspace", or "dacexpspace"
	std::string commandName;

	unsigned short line;
	timeType time;

	Expression initVal;
	Expression finalVal;
	Expression rampTime;
	Expression rampInc;
	Expression numSteps;

	// stores whether this command is subject to repeats and which repeat it correpsonds to in the tree if so
	repeatInfoId repeatId = { 0, {0,0}, false };
};

struct DacCommand
{
	unsigned short line;
	double time;
	double value;

	// same as DACCommandForm, this will be used for repeat generation, i.e. copy and extend the std::vector<AoCommand>
	repeatInfoId repeatId = { 0, {0,0}, false };
};


struct DacSnapshot
{
	double time;
	std::array<double, 24> dacValues;
};
