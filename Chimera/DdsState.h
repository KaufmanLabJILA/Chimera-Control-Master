#pragma once

//Structure that stores the current state of all channels of a given DDS.

struct DdsState
{
	double freqs [8];
	double amps [8];
	UINT8 index;
};