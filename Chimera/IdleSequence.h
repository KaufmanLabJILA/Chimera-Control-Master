#pragma once

#include "stdafx.h"
#include "ProfileSystem.h"

class idleSequence
{
public:
	idleSequence();

	bool idleSequenceActive;
	std::atomic<bool> killIdler;
	std::atomic<bool> idleSequenceRunning;
	int varInd;
};