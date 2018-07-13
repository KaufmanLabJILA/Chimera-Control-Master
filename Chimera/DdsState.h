#pragma once

//Structure that stores the current state of all channels of a given DDS.

struct DdsState
{
	double freqs [2][4];
	double amps [2][4];
	UINT8 index;
};