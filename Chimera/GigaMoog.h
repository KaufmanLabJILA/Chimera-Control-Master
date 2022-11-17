#pragma once
#include "stdafx.h"
#include <iostream>

#include "constants.h"
#include "ScriptStream.h"

#include <thread>
#include <chrono>

#include "BoostUDP.h"
#include <windows.h>

#include "MessagePrinter.h"
#include "MessageSender.h"

#include "moveSequence.h"

class memoryController {
	//Simple helper class to keep track of how much memory has been used, and what channels to use next.
public:

	memoryController() {
		memBlocks.reserve(6); //reminder - always a lot faster to assign vector size ahead of time.
		for (size_t i = 0; i < 6; i++)
		{
			memBlock mem(i);
			memBlocks.push_back(mem);
		}
	}

	std::vector<int> getNextChannels(int channelsNeeded);
	inline void moveChannel(int blockID, int nmoves = 1) { memBlocks[blockID].usedMemory += nmoves; }; //inline because this function is very simple, but could get called a lot.

private:
	struct memBlock
	{
		int blockID; //each block contains 8 channels. 0-7, 8-15, 16-23, 24-31, 32-39, 40-47;
		int channelID;
		size_t usedMemory;

		memBlock(int id) : blockID(id), channelID(0), usedMemory(0) {} //constructor just takes block ID, and assumes memory starts out empty.

		bool operator < (const memBlock& mem) const //define a sorting operation that sorts by available memory
		{
			return (usedMemory < mem.usedMemory);
		}
	};

	std::vector<memBlock> memBlocks;

};

class gigaMoog {

public:
	gigaMoog(std::string IPAddress, int port);
	gigaMoog(void);
	virtual ~gigaMoog(void);
	//gigaMoog(const gigaMoog&) = default;
	//gigaMoog(gigaMoog&&) = default;
	//gigaMoog& operator = (const gigaMoog&) = default;
	//gigaMoog& operator = (gigaMoog&&) = default;

	//Attempt to parse moog script
	void loadMoogScript(std::string scriptAddress);
	void analyzeMoogScript(gigaMoog* moog, std::vector<variableType>& variables, UINT variation);

	virtual void writeLoad(MessageSender& ms);
	virtual void writeOff(MessageSender& ms);
	virtual void writeMoveOff(MessageSender& ms);
	virtual void writeTerminator(MessageSender& ms);

	void send(MessageSender& ms);
	void refreshLUT();
	void updateXYOffsetAuto();

	void writeRearrangeMoves(moveSequence input, MessageSender& ms);

	bool rearrangerActive = false;
	bool autoTweezerOffsetActive = false;
	std::vector<bool> initialPositionsX; //initial and target positions saved in gigamoog object mostly because it already handles the script, but these only get used by the rearrangement thread.
	std::vector<bool> initialPositionsY;
	std::vector<bool> initialPositions;
	std::vector<UINT8> targetPositions;
	std::vector<bool> filterPositionsX;
	std::vector<bool> filterPositionsY;

	int xPixelOffsetAuto = 0;
	int yPixelOffsetAuto = 0;
	int subpixelIndexOffsetAuto = 12;
	double xOffsetAuto = NULL;
	double yOffsetAuto = NULL;

	int targetNumber;
	int nTweezerX, nTweezerY;
	int nFilterTweezerX, nFilterTweezerY;
	int scrunchSpacing;
	std::string rearrangeMode;

private:
	BoostUDP fpga;

	// the moog script file contents get dumped into this.
	std::string currentMoogScriptText;
	ScriptStream currentMoogScript;
	//std::vector<unsigned short int> ATW_LUT;
	//std::vector<unsigned long long int> FTW_LUT;
	std::vector<double> ATW_LUT;
	std::vector<double> FTW_LUT;
	std::vector<double> subpixelLUT; //This must be synchronized with atomCruncher subpixel masks.
	double xOffset, yOffset;
	double xOffsetManual, yOffsetManual;
	std::vector<double> xPix2MHz, yPix2MHz;
	int nSubpixel;
	UINT xDim, yDim; //x and y dimensions of atom positions from LUT. Must be coordinated with number of masks in image processing.
	int ampStepMag = 134217727;
	int freqStepMag = 511;

	double getFreqX(int xIndex, int yIndex);
	double getAmpX(int xIndex, int yIndex);
	double getFreqY(int xIndex, int yIndex);
	double getAmpY(int xIndex, int yIndex);

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

