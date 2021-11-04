#pragma once

struct awgCommand
{
	//unsigned long channel;
	unsigned short address;
	float timeStampMicro;
	bool phase_update;
	float phaseDegrees;
	float ampPercent;
	float freqMHz;
};