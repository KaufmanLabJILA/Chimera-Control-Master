#pragma once

#include "stdafx.h"
#include "ProfileSystem.h"

class idleSequence
{
public:
	idleSequence();

	bool idleSequenceActive = false;
	bool killIdler = true;
	bool idleSequenceRunning = false;
	int varInd = -1;
};