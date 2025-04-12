#pragma once
#include "stdafx.h"

#include <vector>

class moveSingle {
public:
	//std::vector<bool> boolAOX;
	//std::vector<bool> boolAOY;
	//inline UINT8 nx() { 
	//	return std::count(boolAOX.begin(), boolAOX.end(), true);
	//}; //number of active tweezers
	//inline UINT8 ny() {
	//	return std::count(boolAOY.begin(), boolAOY.end(), true);
	//}; //number of active tweezers

	std::vector<float> startAOX; //negative values allow for special handling, e.g. removing atoms.
	std::vector<float> startAOY; // a non-whole number linearly interpolates from the neighboring LUT values
	std::vector<float> endAOX;
	std::vector<float> endAOY;
	std::vector<float> powerMultsX; // multiply the normally set power (use especially to make tweezers shallower, you may run into problems with values >1.0)
	std::vector<float> powerMultsY; // these can be empty, but if you add to them they must be the same length as start/endAOX/Y


	bool manualPhaseX = false; // allow manually specifying phases so that we can stack tones "by hand"
	bool manualPhaseY = false;

	std::vector<double> phaseX; // specified phases, only accessed if corresponding flag above is true
	std::vector<double> phaseY;

	bool instantOff = false; // if true, turn off tweezer immediately instead of ramping
	bool rampOffOnMove = false; // if true, ramp off as you move
	bool slowMove = false; // if true, override normal moverate with slowMoveSpeed 

	int slowMoveSpeed = 255;

	

	inline UINT8 nx() {
		return startAOX.size();
	}; //number of active tweezers
	inline UINT8 ny() {
		return startAOY.size();
	}; //number of active tweezers
};

class moveSequence {
public:
	std::vector<moveSingle> moves;
	inline UINT8 nMoves() {
		return moves.size();
	}
};