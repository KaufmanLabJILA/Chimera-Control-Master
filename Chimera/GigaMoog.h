#pragma once
#include "stdafx.h"
#include <iostream>

#include "constants.h"
#include "ScriptStream.h"

#include <thread>
#include <chrono>

#include "BoostAsyncSerial.h"
#include <windows.h>

#include "MessagePrinter.h"
#include "MessageSender.h"

#include "moveSequence.h"

class gigaMoog {

public:
	gigaMoog(std::string portID, int baudrate);
	virtual ~gigaMoog(void);

	//Attempt to parse moog script
	void loadMoogScript(std::string scriptAddress);
	void analyzeMoogScript(gigaMoog* moog, std::vector<variableType>& variables, UINT variation);

	virtual void writeLoad(MessageSender& ms);
	virtual void writeOff(MessageSender& ms);
	virtual void writeMoveOff(MessageSender& ms);
	virtual void writeTerminator(MessageSender& ms);

	void send(MessageSender& ms);
	void refreshLUT();

	void writeRearrangeMoves(moveSequence input, MessageSender& ms);

	bool rearrangerActive = false;
	std::vector<bool> initialPositionsX; //initial and target positions saved in gigamoog object mostly because it already handles the script, but these only get used by the rearrangement thread.
	std::vector<bool> initialPositionsY;
	std::vector<bool> initialPositions;
	std::vector<std::vector<bool>> targetPositions;
	UINT targetNumber;
	UINT nTweezerX, nTweezerY;
	std::string rearrangeMode;

private:
	BoostAsyncSerial fpga;

	// the moog script file contents get dumped into this.
	std::string currentMoogScriptText;
	ScriptStream currentMoogScript;
	//std::vector<unsigned short int> ATW_LUT;
	//std::vector<unsigned long long int> FTW_LUT;
	std::vector<double> ATW_LUT;
	std::vector<double> FTW_LUT;
	double xOffset, yOffset;
	UINT xDim, yDim; //x and y dimensions of atom positions from LUT. Must be coordinated with number of masks in image processing.
	int ampStepMag = 134217727;
	int freqStepMag = 511;

	static unsigned long long int getFTW(double frequency) {
		//36-bit DDS
		//1 MHz = 223696213.33333333333333333333333 FTW
		double MHz = 223696213.33333333333333333333333;
		return (unsigned long long int)(frequency * MHz);
	}

	static unsigned short int getATW(double amplitude) {
		//16-bit ATW
		if (amplitude < 0.0 || amplitude > 100.0) thrower("invalid amplitude");
		amplitude *= 655.35;
		return (unsigned short int)(amplitude);
	}

	static unsigned short int getPTW(double phase) {
		//12-bit PTW
		if (phase < 0.0 || phase > 360.0) thrower("invalid phase");
		phase /= 360.0;
		phase *= 4096;
		return (unsigned short int)(phase);
	}

};

