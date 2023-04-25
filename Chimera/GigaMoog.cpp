#include "stdafx.h"
#include "GigaMoog.h"
#include "cnpy.h"
#include <fstream>
#include <experimental/filesystem>
#include <windows.h>
#include <math.h>
namespace fs = std::experimental::filesystem;

//using namespace::boost::asio;
//using namespace::std;
//using namespace::std::placeholders;

//const UINT gigaMoog::freqstartoffset = 512;
//const UINT gigaMoog::freqstopoffset = 1024;
//const UINT gigaMoog::gainoffset = 1536;
//const UINT gigaMoog::loadoffset = 2048;
//const UINT gigaMoog::moveoffset = 2560;

std::vector<int> memoryController::getNextChannels(int channelsNeeded) {
	/// returns vector of channel IDs to use on gmoog given desired number of channels used in parallel, and how much memory has already been used by the move sequence

	std::stable_sort(memBlocks.begin(), memBlocks.end()); //Sort the blocks by used memory.
	std::vector<int> channelsOut(channelsNeeded);

	for (size_t i = 0; i < channelsNeeded; i++)
	{
		channelsOut[i] = 8*(memBlocks[0].blockID) + memBlocks[0].channelID;
		memBlocks[0].channelID += 1; //iterate the channel ID, so that if this block is selected again we use the next channel.
		memBlocks[0].usedMemory += 3; //each full move uses up 3 snapshots.
		
		if (memBlocks[0].usedMemory > 253) //if the next move will use up more than the available 256 memory slices, delete this memory block so that it is not used again.
		{
			memBlocks.erase(memBlocks.begin());
			if (memBlocks.size() < 1)
			{
				thrower("Maximum number of moves exceeded, gigamoog memory is full.");
			}
		}
		
		std::stable_sort(memBlocks.begin(), memBlocks.end()); //Sort the blocks by used memory.
	}

	for (size_t i = 0; i < memBlocks.size(); i++)
	{
		memBlocks[i].channelID = 0; //reset the channel IDs for the next round of moves.
	}

	return channelsOut;
}

gigaMoog::gigaMoog(std::string IPAddress, int port) : fpga(IPAddress, port) {
	if (!GIGAMOOG_SAFEMODE) {
		//writeOff();
	}
}

//gigaMoog::gigaMoog() {
//	if (!GIGAMOOG_SAFEMODE) {
//		//writeOff();
//	}
//}

gigaMoog::~gigaMoog(void){
}

void gigaMoog::refreshLUT()
{
	//Reload tweezer position LUT
	//load LUTs from .npy file
	cnpy::NpyArray arrAmpLUT = cnpy::npy_load(TWEEZER_AMPLITUDE_LUT_FILE_LOCATION);
	std::vector<double> ampLUT = arrAmpLUT.as_vec<double>(); //load LUT as a flattened list of floats (row major)
	cnpy::NpyArray arrFreqLUT = cnpy::npy_load(TWEEZER_FREQUENCY_LUT_FILE_LOCATION);
	std::vector<double> freqLUT = arrFreqLUT.as_vec<double>(); // (row major)
	cnpy::NpyArray arrPaintAmpLUT = cnpy::npy_load(TWEEZER_PAINT_AMPLITUDE_LUT_FILE_LOCATION);
	std::vector<double> paintAmpLUT = arrPaintAmpLUT.as_vec<double>(); //load LUT as a flattened list of floats (row major)
	cnpy::NpyArray arrPaintMaskLUT = cnpy::npy_load(TWEEZER_PAINT_MASK_LUT_FILE_LOCATION);
	std::vector<double> paintMaskLUT = arrPaintMaskLUT.as_vec<double>(); //load LUT as a flattened list of floats (row major)


	xDim = arrAmpLUT.shape[0];
	yDim = arrAmpLUT.shape[1]; //Get np array dimensions
	xDimPaint = arrPaintAmpLUT.shape[0];
	yDimPaint = arrPaintAmpLUT.shape[1];
	xDimPaintMask = arrPaintMaskLUT.shape[0];
	yDimPaintMask = arrPaintMaskLUT.shape[1];

	if (xDimPaint != xDimPaintMask || yDimPaint != yDimPaintMask)
	{
		thrower("Dimensions of painting mask and amplitude LUT do not match.");
	}

	ATW_LUT.clear();
	FTW_LUT.clear();
	PAINT_ATW_LUT.clear();
	PAINT_MASK_LUT.clear();
	UINT i = 0;
	for (auto& amp : ampLUT)
	{
		ATW_LUT.push_back(amp);
		//ATW_LUT.push_back(getATW(amp));
		i++;
	}

	i = 0;
	for (auto& freq : freqLUT)
	{
		FTW_LUT.push_back(freq);
		//FTW_LUT.push_back(getFTW(freq)); //TODO: switch LUTs back to tuning words for speed, after fixing the message builder nonsense.
		i++;
	}

	i = 0;
	for (auto& paintAmp : paintAmpLUT)
	{
		PAINT_ATW_LUT.push_back(paintAmp);
		i++;
	}

	i = 0;
	for (auto& paintMask : paintMaskLUT)
	{
		PAINT_MASK_LUT.push_back(paintMask);
		i++;
	}
	if ((!checkPaintMaskX())&(painterActive))
	{
		if (paintStatic) // allow more than 24 tweezers for painting static barrier but restrict amplitude LUT power
		{
			double amp;
			for (int x = 0; x < xDimPaint; x++)
			{
				for (int y = 0; y < yDimPaint; y++)
				{
					amp = PAINT_ATW_LUT[2 * yDimPaint * x + 2 * y + 0];
					if (amp > (X_AMP_LUT_MEAN / 1.414))
					{
						thrower("More than 24 tweezers enabled in painting LUT and too much power:" + str(amp) + "! Remake painting amplitude LUT!");
					}
				}
			}
		}
		else
		{
			thrower("More than 24 tweezers enabled in painting LUT! Only enabling the first 24.");
		}
	}
	if ((!checkPaintMaskY())&(painterActive))
	{
		if (paintStatic) // allow more than 24 tweezers for painting static barrier but restrict amplitude LUT power
		{
			double amp;
			for (int x = 0; x < xDimPaint; x++)
			{
				for (int y = 0; y < yDimPaint; y++)
				{
					amp = PAINT_ATW_LUT[2 * yDimPaint * x + 2 * y + 1];
					if (amp > (Y_AMP_LUT_MEAN / 1.414))
					{
						thrower("More than 24 tweezers enabled in painting LUT and too much power:" +  str(amp) + "! Remake painting amplitude LUT!");
					}
				}
			}
		}
		else
		{
			thrower("More than 24 tweezers enabled in painting LUT! Only enabling the first 24.");
		}
	}


	cnpy::NpyArray arrSubpixelLUT = cnpy::npy_load(SUBPIXELLUT_FILE_LOCATION);
	subpixelLUT = arrSubpixelLUT.as_vec<double>();
	nSubpixel = arrSubpixelLUT.shape[0];
	xPix2MHz = { subpixelLUT[sqrt(nSubpixel)-1]*(-2), subpixelLUT[sqrt(nSubpixel)] * (-2) }; //y then x for consistency. Note, indices doubled because flattened array of doublets.
	yPix2MHz = { subpixelLUT[nSubpixel - 1 + sqrt(nSubpixel) - 1] * 2 , subpixelLUT[nSubpixel - 1 + sqrt(nSubpixel) - 1 + 1] * 2 }; //y then x for consistency.
}

double gigaMoog::getFreqX(int xIndex, int yIndex) {
	yIndex = (yIndex < 0) ? 0 : yIndex;

	if (xIndex == -1 && yIndex >= 0) //special handling for atom removal
	{
		return 60 + xOffset;
	}
	else if (xIndex == -2 && yIndex >= 0)
	{
		return 300 + xOffset;
	}
	else if (xIndex >= 0 && yIndex >= 0) 
	{
		return FTW_LUT[2 * yDim * xIndex + 2 * yIndex + 0] + xOffset;
	}
	else
	{
		thrower("invalid LUT index");
	}
}

double gigaMoog::getAmpX(int xIndex, int yIndex) {
	yIndex = (yIndex < 0) ? 0 : yIndex;

	if ((xIndex == -1 || xIndex == -2) && yIndex >= 0) //special handling for atom removal
	{
		return ATW_LUT[2 * yDim * 0 + 2 * yIndex + 0];
	}
	else if (xIndex >= 0 && yIndex >= 0)
	{
		return ATW_LUT[2 * yDim * xIndex + 2 * yIndex + 0];
	}
	else
	{
		thrower("invalid LUT index");
	}
}

double gigaMoog::getFreqY(int xIndex, int yIndex) {
	xIndex = (xIndex < 0) ? 0 : xIndex;

	if (xIndex >= 0 && yIndex == -1) //special handling for atom removal
	{
		return 60 + yOffset;
	}
	else if (xIndex >= 0 && yIndex == -2)
	{
		return 300 + yOffset;
	}
	else if (xIndex >= 0 && yIndex >= 0)
	{
		return FTW_LUT[2 * yDim * xIndex + 2 * yIndex + 1] + yOffset;
	}
	else
	{
		thrower("invalid LUT index");
	}
}

double gigaMoog::getAmpY(int xIndex, int yIndex) {
	xIndex = (xIndex < 0) ? 0 : xIndex;

	if (xIndex >= 0 && (yIndex == -1 || yIndex == -2)) //special handling for atom removal
	{
		return ATW_LUT[2 * yDim * xIndex + 2 * 0 + 1];
	}
	else if (xIndex >= 0 && yIndex >= 0)
	{
		return ATW_LUT[2 * yDim * xIndex + 2 * yIndex + 1];
	}
	else
	{
		thrower("invalid LUT index");
	}
}

double gigaMoog::getPaintAmpX(unsigned int xIndex, unsigned int yIndex) {
	if (xIndex >= xDimPaint || yIndex >= yDimPaint)
	{
		thrower("invalid LUT index");
	}
	else
	{
		//New way with offset tracking linear interpolation
		int offsetSign = int((xOffset - xOffsetManual) > 0) - int((xOffset - xOffsetManual) < 0);
		double amp1, amp2, freq1, freq2, derivative, amplitude;
		amp1 = PAINT_ATW_LUT[2 * yDimPaint * xIndex + 2 * yIndex + 0];
		freq1 = getFreqX(xIndex,yIndex);

		//If frequency in the LUT range then interpolate, else extrapolate
		if ((xIndex + offsetSign >= 0) && (xIndex + offsetSign < xDimPaint))
		{
			amp2 = PAINT_ATW_LUT[2 * yDimPaint * (xIndex + offsetSign) + 2 * yIndex + 0];
			freq2 = getFreqX(xIndex + offsetSign, yIndex);
		}
		else
		{
			amp2 = PAINT_ATW_LUT[2 * yDimPaint * (xIndex - offsetSign) + 2 * yIndex + 0];
			freq2 = getFreqX(xIndex - offsetSign, yIndex);
		}

		if (freq2 - freq1 != 0)
		{
			derivative = (amp2 - amp1) / (freq2 - freq1);
		}
		else
		{
			derivative = 0.0;
		}
		amplitude = amp1 + (xOffset- xOffsetManual) * derivative;

		//Old way without interpolation for offset drift
		//double amplitude = PAINT_ATW_LUT[2 * yDimPaint * xIndex + 2 * yIndex + 0];

		if (amplitude <= 100 && amplitude >= 0) {
			return amplitude;
		}
		else if (amplitude > 100) {
			return 100.0;
		}
		else {
			return 0.0;
		}
	}
}

double gigaMoog::getPaintAmpY(unsigned int xIndex, unsigned int yIndex) {
	if (xIndex >= xDimPaint || yIndex >= yDimPaint)
	{
		thrower("invalid LUT index");
	}
	else
	{
		//New way with offset tracking linear interpolation
		int offsetSign = int((yOffset - yOffsetManual) > 0) - int((yOffset - yOffsetManual) < 0);
		double amp1, amp2, freq1, freq2, derivative, amplitude;
		amp1 = PAINT_ATW_LUT[2 * yDimPaint * xIndex + 2 * yIndex + 1];
		freq1 = getFreqY(xIndex, yIndex);

		if ((yIndex + offsetSign >= 0) && (yIndex + offsetSign < yDimPaint)) //interpolate
		{
			amp2 = PAINT_ATW_LUT[2 * yDimPaint * xIndex + 2 * (yIndex + offsetSign) + 1];
			freq2 = getFreqY(xIndex, yIndex + offsetSign);
		}
		else //extrapolate
		{
			amp2 = PAINT_ATW_LUT[2 * yDimPaint * xIndex + 2 * (yIndex - offsetSign) + 1];
			freq2 = getFreqY(xIndex, yIndex - offsetSign);
		}

		if (freq2 - freq1 != 0)
		{
			derivative = (amp2 - amp1) / (freq2 - freq1);
		}
		else
		{
			derivative = 0.0;
		}
		amplitude = amp1 + (yOffset - yOffsetManual) * derivative;

		//Old way without interpolation for offset drift
		//double amplitude = PAINT_ATW_LUT[2 * yDimPaint * xIndex + 2 * yIndex + 1];


		if (amplitude <= 100 && amplitude >= 0) {
			return amplitude;
		}
		else if (amplitude > 100) {
			return 100.0;
		}
		else {
			return 0.0;
		}
	}
}

bool gigaMoog::getPaintMaskX(unsigned int xIndex, unsigned int yIndex)
{
	if (xIndex >= xDimPaintMask || yIndex >= yDimPaintMask)
	{
		thrower("invalid LUT index");
	}
	else
	{
		return PAINT_MASK_LUT[2 * yDimPaintMask * xIndex + 2 * yIndex + 0] > 0.5;
	}
}

bool gigaMoog::getPaintMaskY(unsigned int xIndex, unsigned int yIndex)
{
	if (xIndex >= xDimPaintMask || yIndex >= yDimPaintMask)
	{
		thrower("invalid LUT index");
	}
	else
	{
		return PAINT_MASK_LUT[2 * yDimPaintMask * xIndex + 2 * yIndex + 1] > 0.5;
	}
}

bool gigaMoog::checkPaintMaskX()
{
	for (int yIndex = 0; yIndex < 48; yIndex++)
	{
		int numTweezerOnX = 0;
		for (int xIndex = 0; xIndex < 48; xIndex++)
		{
			if (PAINT_MASK_LUT[2 * yDimPaintMask * xIndex + 2 * yIndex + 0])
			{
				numTweezerOnX++;
			}
		}
		if (numTweezerOnX > 24)
		{
			return false;
		}
	}
	return true;
}

bool gigaMoog::checkPaintMaskY()
{
	for (int xIndex = 0; xIndex < 48; xIndex++)
	{
		int numTweezerOnY = 0;
		for (int yIndex = 0; yIndex < 48; yIndex++)
		{
			if (PAINT_MASK_LUT[2 * yDimPaintMask * xIndex + 2 * yIndex + 1])
			{
				numTweezerOnY++;
			}
		}
		if (numTweezerOnY > 24)
		{
			return false;
		}
	}
	return true;
}

void gigaMoog::writeRearrangeMoves(moveSequence input, MessageSender& ms) {
	//Important: this function automatically writes the terminator and sends the data
	UINT nMoves = input.nMoves();

	memoryController memoryDAC0;
	memoryController memoryDAC1;

	if (nMoves>256/3)
	{
		thrower("ERROR: too many moves for gmoog buffer");
		return;
	}

	//First write all zeroes for load. Depreciated 220124
	//writeOff(ms);
	writeMoveOff(ms);

	size_t nx, ny;
	double phase, amp, freq, ampPrev, freqPrev;
	int ampstep, freqstep;

	//step 0: turn off all load tones.
	for (int channel = 0; channel < 16; channel++) {//TODO: 16 could be changed to 48 if using more tones for rearrangement
		if (channel < nTweezerX)
		{
			size_t hardwareChannel = (channel * 8) % 48 + (channel * 8) / 48;
			memoryDAC0.moveChannel(hardwareChannel / 8);
			Message m = Message::make().destination(MessageDestination::KA007)
				.DAC(MessageDAC::DAC0).channel(hardwareChannel)
				.setting(MessageSetting::MOVEFREQUENCY)
				.frequencyMHz(0).amplitudePercent(0.01).phaseDegrees(0).instantFTW(1).ATWIncr(-ampStepMag).stepSequenceID(0).FTWIncr(0).phaseJump(1);;
			ms.enqueue(m);
		}
	}
	for (int channel = 0; channel < 24; channel++) {
		if (channel < nTweezerY)
		{
			size_t hardwareChannel = (channel * 8) % 48 + (channel * 8) / 48;
			memoryDAC1.moveChannel(hardwareChannel / 8);
			Message m = Message::make().destination(MessageDestination::KA007)
				.DAC(MessageDAC::DAC1).channel(hardwareChannel)
				.setting(MessageSetting::MOVEFREQUENCY)
				.frequencyMHz(0).amplitudePercent(0.01).phaseDegrees(0).instantFTW(1).ATWIncr(-ampStepMag).stepSequenceID(0).FTWIncr(0).phaseJump(1);;
			ms.enqueue(m);
		}
	}

	for (size_t stepID = 0; stepID < nMoves; stepID++)
	{
		nx = input.moves[stepID].nx();
		ny = input.moves[stepID].ny();

		//Get most hardware efficient channels to use. Also handle tripling up of tones.
		std::vector<int> hardwareChannelsDAC0;
		std::vector<int> hardwareChannelsDAC1;
		if (ny == 1)
		{
			hardwareChannelsDAC0 = memoryDAC0.getNextChannels(nx);
			hardwareChannelsDAC1 = memoryDAC1.getNextChannels(3);
		}
		else if (nx == 1)
		{
			hardwareChannelsDAC0 = memoryDAC0.getNextChannels(3);
			hardwareChannelsDAC1 = memoryDAC1.getNextChannels(ny);
		}
		else
		{
			hardwareChannelsDAC0 = memoryDAC0.getNextChannels(nx);
			hardwareChannelsDAC1 = memoryDAC1.getNextChannels(ny);
		}

		//step 1: ramp up tones at initial locations and phases
		for (int channel = 0; channel < 16; channel++) {//TODO: 16 could be changed to 48 if using more tones for rearrangement
			if (ny > 1 && nx == 1 && channel < 3) //Triple up tones if only a single tone on, assuming y axis not already tripled.
			{
				size_t hardwareChannel = hardwareChannelsDAC0[channel];
				freq = getFreqX(input.moves[stepID].startAOX[0], input.moves[stepID].startAOY[0]);
				amp = getAmpX(input.moves[stepID].startAOX[0], input.moves[stepID].startAOY[0]);
				Message m = Message::make().destination(MessageDestination::KA007)
					.DAC(MessageDAC::DAC0).channel(hardwareChannel)
					.setting(MessageSetting::MOVEFREQUENCY)
					.frequencyMHz(freq).amplitudePercent(amp).phaseDegrees(0).instantFTW(1).ATWIncr(ampStepMag).stepSequenceID(3 * stepID + 1).FTWIncr(0).phaseJump(1);;
				ms.enqueue(m);
			}
			else if (ny != 0 && nx != 0 && channel < nx) {
				size_t hardwareChannel = hardwareChannelsDAC0[channel];
				freq = getFreqX(input.moves[stepID].startAOX[channel], input.moves[stepID].startAOY[0]);
				amp = getAmpX(input.moves[stepID].startAOX[channel], input.moves[stepID].startAOY[0]);
				phase = fmod(180 * pow(channel + 1, 2) / nx, 360); //this assumes comb of even tones, imperfect, but also short duration so not super critical, and fast.

				Message m = Message::make().destination(MessageDestination::KA007)
					.DAC(MessageDAC::DAC0).channel(hardwareChannel)
					.setting(MessageSetting::MOVEFREQUENCY)
					.frequencyMHz(freq).amplitudePercent(amp).phaseDegrees(phase).instantFTW(1).ATWIncr(ampStepMag).stepSequenceID(3 * stepID + 1).FTWIncr(0).phaseJump(1);;
				ms.enqueue(m);
			}
		}

		for (int channel = 0; channel < 24; channel++) {
			if (nx != 0 && ny == 1 && channel < 3) //Triple up tones if only a single tone on.
			{
				size_t hardwareChannel = hardwareChannelsDAC1[channel];
				freq = getFreqY(input.moves[stepID].startAOX[0], input.moves[stepID].startAOY[0]);
				amp = getAmpY(input.moves[stepID].startAOX[0], input.moves[stepID].startAOY[0]);

				Message m = Message::make().destination(MessageDestination::KA007)
					.DAC(MessageDAC::DAC1).channel(hardwareChannel)
					.setting(MessageSetting::MOVEFREQUENCY)
					.frequencyMHz(freq).amplitudePercent(amp).phaseDegrees(0).instantFTW(1).ATWIncr(ampStepMag).stepSequenceID(3 * stepID + 1).FTWIncr(0).phaseJump(1);;
				ms.enqueue(m);
			}
			else if (nx != 0 && ny != 0 && channel < ny) {
				size_t hardwareChannel = hardwareChannelsDAC1[channel];
				freq = getFreqY(input.moves[stepID].startAOX[0], input.moves[stepID].startAOY[channel]);
				amp = getAmpY(input.moves[stepID].startAOX[0], input.moves[stepID].startAOY[channel]);
				phase = fmod(180 * pow(channel + 1, 2) / ny, 360);

				Message m = Message::make().destination(MessageDestination::KA007)
					.DAC(MessageDAC::DAC1).channel(hardwareChannel)
					.setting(MessageSetting::MOVEFREQUENCY)
					.frequencyMHz(freq).amplitudePercent(amp).phaseDegrees(phase).instantFTW(1).ATWIncr(ampStepMag).stepSequenceID(3 * stepID + 1).FTWIncr(0).phaseJump(1);;
				ms.enqueue(m);
			}
		}

		//step 2: ramp to new locations
		for (int channel = 0; channel < 16; channel++) {
			if (ny > 1 && nx == 1 && channel < 3) //Triple up tones if only a single tone on.
			{
				size_t hardwareChannel = hardwareChannelsDAC0[channel];
				freqPrev = getFreqX(input.moves[stepID].startAOX[0], input.moves[stepID].startAOY[0]);
				ampPrev = getAmpX(input.moves[stepID].startAOX[0], input.moves[stepID].startAOY[0]);
				
				freq = getFreqX(input.moves[stepID].endAOX[0], input.moves[stepID].endAOY[0]);
				amp = getAmpX(input.moves[stepID].endAOX[0], input.moves[stepID].endAOY[0]);

				ampstep = (amp < ampPrev) ? -ampStepMag : ampStepMag; //Change sign of steps appropriately.
				freqstep = (freq < freqPrev) ? -freqStepMag : freqStepMag;

				Message m = Message::make().destination(MessageDestination::KA007)
					.DAC(MessageDAC::DAC0).channel(hardwareChannel)
					.setting(MessageSetting::MOVEFREQUENCY)
					.frequencyMHz(freq).amplitudePercent(amp).phaseDegrees(0).instantFTW(0).ATWIncr(ampstep).stepSequenceID(3 * stepID + 1 + 1).FTWIncr(freqstep).phaseJump(0);;
				ms.enqueue(m);
			}
			else if (ny != 0 && nx != 0 && channel < nx)
			{
				size_t hardwareChannel = hardwareChannelsDAC0[channel];
				freqPrev = getFreqX(input.moves[stepID].startAOX[channel], input.moves[stepID].startAOY[0]);
				ampPrev = getAmpX(input.moves[stepID].startAOX[channel], input.moves[stepID].startAOY[0]);

				freq = getFreqX(input.moves[stepID].endAOX[channel], input.moves[stepID].endAOY[0]);
				amp = getAmpX(input.moves[stepID].endAOX[channel], input.moves[stepID].endAOY[0]);

				ampstep = (amp < ampPrev) ? -ampStepMag : ampStepMag; //Change sign of steps appropriately.
				freqstep = (freq < freqPrev) ? -freqStepMag : freqStepMag;

				Message m = Message::make().destination(MessageDestination::KA007)
					.DAC(MessageDAC::DAC0).channel(hardwareChannel)
					.setting(MessageSetting::MOVEFREQUENCY)
					.frequencyMHz(freq).amplitudePercent(amp).phaseDegrees(0).instantFTW(0).ATWIncr(ampstep).stepSequenceID(3 * stepID + 1 + 1).FTWIncr(freqstep).phaseJump(0);;
				ms.enqueue(m);
			}
		}

		for (int channel = 0; channel < 24; channel++) {
			if (nx != 0 && ny == 1 && channel < 3) //Triple up tones if only a single tone on.
			{
				size_t hardwareChannel = hardwareChannelsDAC1[channel];

				freqPrev = getFreqY(input.moves[stepID].startAOX[0], input.moves[stepID].startAOY[0]);
				ampPrev = getAmpY(input.moves[stepID].startAOX[0], input.moves[stepID].startAOY[0]);

				freq = getFreqY(input.moves[stepID].endAOX[0], input.moves[stepID].endAOY[0]);
				amp = getAmpY(input.moves[stepID].endAOX[0], input.moves[stepID].endAOY[0]);

				ampstep = (amp < ampPrev) ? -ampStepMag : ampStepMag; //Change sign of steps appropriately.
				freqstep = (freq < freqPrev) ? -freqStepMag : freqStepMag;

				Message m = Message::make().destination(MessageDestination::KA007)
					.DAC(MessageDAC::DAC1).channel(hardwareChannel)
					.setting(MessageSetting::MOVEFREQUENCY)
					.frequencyMHz(freq).amplitudePercent(amp).phaseDegrees(0).instantFTW(0).ATWIncr(ampstep).stepSequenceID(3 * stepID + 1 + 1).FTWIncr(freqstep).phaseJump(0);;
				ms.enqueue(m);
			}
			else if (nx != 0 && ny != 0 && channel < ny)
			{
				size_t hardwareChannel = hardwareChannelsDAC1[channel];

				freqPrev = getFreqY(input.moves[stepID].startAOX[0], input.moves[stepID].startAOY[channel]);
				ampPrev = getAmpY(input.moves[stepID].startAOX[0], input.moves[stepID].startAOY[channel]);

				freq = getFreqY(input.moves[stepID].endAOX[0], input.moves[stepID].endAOY[channel]);
				amp = getAmpY(input.moves[stepID].endAOX[0], input.moves[stepID].endAOY[channel]);

				ampstep = (amp < ampPrev) ? -ampStepMag : ampStepMag; //Change sign of steps appropriately.
				freqstep = (freq < freqPrev) ? -freqStepMag : freqStepMag;

				Message m = Message::make().destination(MessageDestination::KA007)
					.DAC(MessageDAC::DAC1).channel(hardwareChannel)
					.setting(MessageSetting::MOVEFREQUENCY)
					.frequencyMHz(freq).amplitudePercent(amp).phaseDegrees(0).instantFTW(0).ATWIncr(ampstep).stepSequenceID(3 * stepID + 1 + 1).FTWIncr(freqstep).phaseJump(0);;
				ms.enqueue(m);
			}
		}

		//step 3: ramp all tones to 0
		for (int channel = 0; channel < 16; channel++) {
			if (ny > 1 && nx == 1 && channel < 3) //Triple up tones if only a single tone on.
			{
				size_t hardwareChannel = hardwareChannelsDAC0[channel];
				freq = getFreqX(input.moves[stepID].endAOX[0], input.moves[stepID].endAOY[0]);
				Message m = Message::make().destination(MessageDestination::KA007)
					.DAC(MessageDAC::DAC0).channel(hardwareChannel)
					.setting(MessageSetting::MOVEFREQUENCY)
					.frequencyMHz(freq).amplitudePercent(0.01).phaseDegrees(0).instantFTW(1).ATWIncr(-ampStepMag).stepSequenceID(3 * stepID + 2 + 1).FTWIncr(0).phaseJump(0);;
				ms.enqueue(m);
			}
			else if (ny != 0 && nx != 0 && channel < nx) {
				size_t hardwareChannel = hardwareChannelsDAC0[channel];
				freq = getFreqX(input.moves[stepID].endAOX[channel], input.moves[stepID].endAOY[0]);
				Message m = Message::make().destination(MessageDestination::KA007)
					.DAC(MessageDAC::DAC0).channel(hardwareChannel)
					.setting(MessageSetting::MOVEFREQUENCY)
					.frequencyMHz(freq).amplitudePercent(0.01).phaseDegrees(0).instantFTW(1).ATWIncr(-ampStepMag).stepSequenceID(3 * stepID + 2 + 1).FTWIncr(0).phaseJump(0);;
				ms.enqueue(m);
				//Has trouble with ramping to 0 amp for some reason - set to ~1 LSB = 100/65535.
			}
		}

		for (int channel = 0; channel < 24; channel++) {
			if (nx != 0 && ny == 1 && channel < 3) //Triple up tones if only a single tone on.
			{
				size_t hardwareChannel = hardwareChannelsDAC1[channel];
				freq = getFreqY(input.moves[stepID].endAOX[0], input.moves[stepID].endAOY[0]);
				Message m = Message::make().destination(MessageDestination::KA007)
					.DAC(MessageDAC::DAC1).channel(hardwareChannel)
					.setting(MessageSetting::MOVEFREQUENCY)
					.frequencyMHz(freq).amplitudePercent(0.01).phaseDegrees(0).instantFTW(1).ATWIncr(-ampStepMag).stepSequenceID(3 * stepID + 2 + 1).FTWIncr(0).phaseJump(0);;
				ms.enqueue(m);
			}
			else if (nx != 0 && ny != 0 && channel < ny) {
				size_t hardwareChannel = hardwareChannelsDAC1[channel];
				freq = getFreqY(input.moves[stepID].endAOX[0], input.moves[stepID].endAOY[channel]);
				Message m = Message::make().destination(MessageDestination::KA007)
					.DAC(MessageDAC::DAC1).channel(hardwareChannel)
					.setting(MessageSetting::MOVEFREQUENCY)
					.frequencyMHz(freq).amplitudePercent(0.01).phaseDegrees(0).instantFTW(1).ATWIncr(-ampStepMag).stepSequenceID(3 * stepID + 2 + 1).FTWIncr(0).phaseJump(0);;
				ms.enqueue(m);
			}
		}
	}

	//additional snapshot ramping down all channels - unclear why needed, but prevents extra trigger issues.

	for (int channel = 0; channel < 48; channel++) {//TODO: 16 could be changed to 48 if using more tones for rearrangement
		{
			Message m = Message::make().destination(MessageDestination::KA007)
				.DAC(MessageDAC::DAC0).channel(channel)
				.setting(MessageSetting::MOVEFREQUENCY)
				.frequencyMHz(0).amplitudePercent(0.01).phaseDegrees(0).instantFTW(1).ATWIncr(-ampStepMag).stepSequenceID(3 * (nMoves - 1) + 2 + 2).FTWIncr(0).phaseJump(1);;
			ms.enqueue(m);
		}
		{
			Message m = Message::make().destination(MessageDestination::KA007)
				.DAC(MessageDAC::DAC1).channel(channel)
				.setting(MessageSetting::MOVEFREQUENCY)
				.frequencyMHz(0).amplitudePercent(0.01).phaseDegrees(0).instantFTW(1).ATWIncr(-ampStepMag).stepSequenceID(3 * (nMoves - 1) + 2 + 2).FTWIncr(0).phaseJump(1);;
			ms.enqueue(m);
		}
	}
}

void gigaMoog::writePaintMoves(MessageSender& ms)
{
	memoryController memoryDAC0;
	memoryController memoryDAC1;

	if (!paintStatic)
	{
		writeMoveOff(ms);
		//step 0: turn off all load tones.
		double phase;
		for (int channel = 0; channel < nTweezerX; channel++) {
			size_t hardwareChannel = (channel * 8) % 48 + (channel * 8) / 48;
			memoryDAC0.moveChannel(hardwareChannel / 8);
			phase = fmod(180 * pow(channel + 1, 2) / nTweezerX, 360); //this assumes comb of even tones, imperfect, but also short duration so not super critical, and fast.
			Message m = Message::make().destination(MessageDestination::KA007)
				.DAC(MessageDAC::DAC0).channel(hardwareChannel)
				.setting(MessageSetting::MOVEFREQUENCY)
				.frequencyMHz(0).amplitudePercent(0.01).phaseDegrees(phase).instantFTW(1).ATWIncr(-ampStepPaintMag).stepSequenceID(0).FTWIncr(0).phaseJump(1);;
			ms.enqueue(m);
		}

		//double phase;
		for (int channel = 0; channel < nTweezerY; channel++) {
			size_t hardwareChannel = (channel * 8) % 48 + (channel * 8) / 48;
			memoryDAC1.moveChannel(hardwareChannel / 8);
			phase = fmod(180 * pow(channel + 1, 2) / nTweezerY, 360); //this assumes comb of even tones, imperfect, but also short duration so not super critical, and fast.
			Message m = Message::make().destination(MessageDestination::KA007)
				.DAC(MessageDAC::DAC1).channel(hardwareChannel)
				.setting(MessageSetting::MOVEFREQUENCY)
				.frequencyMHz(0).amplitudePercent(0.01).phaseDegrees(phase).instantFTW(1).ATWIncr(-ampStepPaintMag).stepSequenceID(0).FTWIncr(0).phaseJump(1);;
			ms.enqueue(m);
		}

		// track all frequencies and amplitudes used for painting
		std::string freqXOut, freqYOut, ampXOut, ampYOut;
		double paintAmpX = 0.0;
		double ATWSign = 1;
		double lastPaintAmpX = 0;

		std::vector<double> paintAmpY;
		std::vector<double> lastPaintAmpY;

		for (unsigned int channel = 0; channel < nTweezerY; channel++) {
			paintAmpY.push_back(0);
			lastPaintAmpY.push_back(0.0);
		}

		// sweep along x
		int totalPaintStepsX = 0;
		for (unsigned int step = 0; step < xDimPaint; step++) {
			bool enableX = getPaintMaskX(step, 0);
			if (!enableX) {
				continue;
			}

			if (!paintStatic)
			{
				totalPaintStepsX++;
			}

			paintAmpX = getPaintAmpX(step, 0);
			ATWSign = lastPaintAmpX > paintAmpX ? -1.0 : 1.0;
			lastPaintAmpX = paintAmpX;

			memoryDAC0.moveChannel(0);

			double tmpFreqX = getFreqX(step, 0);
			Message m = Message::make().destination(MessageDestination::KA007)
				.DAC(MessageDAC::DAC0).channel(0)
				.setting(MessageSetting::MOVEFREQUENCY)
				.frequencyMHz(tmpFreqX).amplitudePercent(paintAmpX).phaseDegrees(0).instantFTW(1).ATWIncr(ATWSign * ampStepPaintMag).stepSequenceID(totalPaintStepsX).FTWIncr(511).phaseJump(0);;
			ms.enqueue(m);

			freqXOut += std::to_string(tmpFreqX) + ", ";
			ampXOut += std::to_string(paintAmpX) + ", ";

			int indexIncY = 0;
			for (unsigned int channel = 0; channel < (std::min)(nTweezerY, 24); channel++) {
				bool enableY = false; // getPaintMaskY(step, indexIncY);
				while (!enableY) {
					if (indexIncY >= yDimPaintMask) {
						goto endY;
					}

					enableY = getPaintMaskY(step, indexIncY);
					indexIncY++;
				}

				if (enableY) {
					double paintFreqY = getFreqY(step, indexIncY - 1);

					paintAmpY[channel] = getPaintAmpY(step, indexIncY - 1);
					ATWSign = lastPaintAmpY[channel] > paintAmpY[channel] ? -1.0 : 1.0;
					lastPaintAmpY[channel] = paintAmpY[channel];

					phase = fmod(180 * pow(channel + 1, 2) / nTweezerY, 360); //this assumes comb of even tones, imperfect, but also short duration so not super critical, and fast.

					size_t hardwareChannel = (channel * 8) % 48 + (channel * 8) / 48;
					memoryDAC1.moveChannel(hardwareChannel / 8);
					Message m = Message::make().destination(MessageDestination::KA007)
						.DAC(MessageDAC::DAC1).channel(hardwareChannel)
						.setting(MessageSetting::MOVEFREQUENCY)
						.frequencyMHz(paintFreqY).amplitudePercent(paintAmpY[channel]).phaseDegrees(phase).instantFTW(1).ATWIncr(ATWSign * ampStepPaintMag).stepSequenceID(totalPaintStepsX).FTWIncr(511).phaseJump(0);;
					ms.enqueue(m);

					freqYOut += std::to_string(paintFreqY) + ", ";
					ampYOut += std::to_string(paintAmpY[channel]) + ", ";
				}
			}
		endY:
			freqYOut += "\n";
			ampYOut += "\n";
		}

		if (exportPaint) {
			exportPaintParams(freqXOut, freqYOut, ampXOut, ampYOut);
		}

		// turn tweezers back to off
		for (unsigned int channel = 0; channel < nTweezerY; channel++) {
			size_t hardwareChannel = (channel * 8) % 48 + (channel * 8) / 48;
			memoryDAC1.moveChannel(hardwareChannel / 8);
			Message m = Message::make().destination(MessageDestination::KA007)
				.DAC(MessageDAC::DAC1).channel(hardwareChannel)
				.setting(MessageSetting::MOVEFREQUENCY)
				.frequencyMHz(0).amplitudePercent(0.01).phaseDegrees(0).instantFTW(1).ATWIncr(-ampStepPaintMag).stepSequenceID(1 + totalPaintStepsX).FTWIncr(511).phaseJump(0);;
			ms.enqueue(m);
		}
		for (unsigned int channel = 0; channel < nTweezerX; channel++) {
			size_t hardwareChannel = (channel * 8) % 48 + (channel * 8) / 48;
			memoryDAC0.moveChannel(hardwareChannel / 8);
			Message m = Message::make().destination(MessageDestination::KA007)
				.DAC(MessageDAC::DAC0).channel(hardwareChannel)
				.setting(MessageSetting::MOVEFREQUENCY)
				.frequencyMHz(0).amplitudePercent(0.01).phaseDegrees(0).instantFTW(1).ATWIncr(-ampStepPaintMag).stepSequenceID(1 + totalPaintStepsX).FTWIncr(511).phaseJump(0);;
			ms.enqueue(m);
		}
	}
	else if (paintStatic)
	{
		writeOff(ms);
		int totalPaintX = 0;
		for (unsigned int xind = 0; xind < xDimPaint; xind++) {
			bool enableX = getPaintMaskX(xind, 0);
			if (!enableX) {
				continue;
			}
			totalPaintX++;
		}

		int currentX = 0;
		for (unsigned int xind = 0; xind < xDimPaint; xind++) {
			bool enableX = getPaintMaskX(xind, 0);
			if (!enableX) {
				continue;
			}
			size_t hardwareChannel = xind;
			double phase = fmod(180 * pow(currentX + 1, 2) / totalPaintX, 360); //this assumes comb of even tones, imperfect, but also short duration so not super critical, and fast.
			double paintAmpX = getPaintAmpX(xind, 0);
			double paintFreqX = getFreqX(xind, 0);

			Message m = Message::make().destination(MessageDestination::KA007)
				.DAC(MessageDAC::DAC0).channel(hardwareChannel)
				.setting(MessageSetting::LOADFREQUENCY)
				.frequencyMHz(paintFreqX).amplitudePercent(paintAmpX).phaseDegrees(phase);
			ms.enqueue(m);
			currentX++;
		}

		int totalPaintY = 0;
		for (unsigned int yind = 0; yind < yDimPaint; yind++) {
			bool enableY = getPaintMaskY(0, yind);
			if (!enableY) {
				continue;
			}
			totalPaintY++;
		}

		int currentY = 0;
		for (unsigned int yind = 0; yind < yDimPaint; yind++) {
			bool enableY = getPaintMaskY(0, yind);
			if (!enableY) {
				continue;
			}
			size_t hardwareChannel = yind;
			double phase = fmod(180 * pow(currentY + 1, 2) / totalPaintY, 360); //this assumes comb of even tones, imperfect, but also short duration so not super critical, and fast.
			double paintAmpY = getPaintAmpY(0, yind);
			double paintFreqY = getFreqY(0, yind);

			Message m = Message::make().destination(MessageDestination::KA007)
				.DAC(MessageDAC::DAC1).channel(hardwareChannel)
				.setting(MessageSetting::LOADFREQUENCY)
				.frequencyMHz(paintFreqY).amplitudePercent(paintAmpY).phaseDegrees(phase);
			ms.enqueue(m);
			currentY++;
		}

	}
	else
	{
	thrower("Invalid tweezer painting setting.");
	}

}


void gigaMoog::exportPaintParams(std::string freqX, std::string freqY, std::string ampX, std::string ampY)
{
	std::string out;
	out += "Offset, Manual, Auto\n";
	out += std::to_string(xOffset) + ", " + std::to_string(xOffsetManual) + ", " + std::to_string(xOffsetAuto) + "\n";
	out += std::to_string(yOffset) + ", " + std::to_string(yOffsetManual) + ", " + std::to_string(yOffsetAuto) + "\n";
	
	out += "\nFrequencies X:\n";
	out += freqX + "\n";

	out += "\nAmplitudes X:\n";
	out += ampX + "\n";

	out += "\nFrequencies Y:\n";
	out += freqY;

	out += "\nAmplitudes Y:\n";
	out += ampY;
	
	// create temporary file
	std::ofstream tmpFile(EXPORT_PAINT_TMP_FILE_LOCATION);
	tmpFile << out;
	tmpFile.close();

	// move file (atomic operation)
	fs::rename(EXPORT_PAINT_TMP_FILE_LOCATION, EXPORT_PAINT_FILE_LOCATION);
}


void gigaMoog::writeTerminator(MessageSender& ms)
{
	Message m = Message::make().destination(MessageDestination::KA007)
		.setting(MessageSetting::TERMINATE_SEQ); //TERMINATE_SEQ swaps the written memory into active memory. gmload hardware trigger sets output based on active memory, but DOES NOT swap the memories. Swaps only occur with this TERMINATE_SEQ message.
	ms.enqueue(m);
}

void gigaMoog::loadMoogScript(std::string scriptAddress)
{
	std::ifstream scriptFile;
	// check if file address is good.
	FILE *file;
	fopen_s(&file, cstr(scriptAddress), "r");
	if (!file)
	{
		thrower("ERROR: Moog Script File " + scriptAddress + " does not exist!");
	}
	else
	{
		fclose(file);
	}
	scriptFile.open(cstr(scriptAddress));
	// check opened correctly
	if (!scriptFile.is_open())
	{
		thrower("ERROR: Moog script file passed test making sure the file exists, but it still failed to open!");
	}
	// dump the file into the stringstream.
	std::stringstream buf(std::ios_base::app | std::ios_base::out | std::ios_base::in);
	buf << scriptFile.rdbuf();
	// This is used to more easily deal some of the analysis of the script.
	buf << "\r\n\r\n__END__";
	// for whatever reason, after loading rdbuf into a stringstream, the stream seems to not 
	// want to >> into a string. tried resetting too using seekg, but whatever, this works.
	currentMoogScript.str("");
	currentMoogScript.str(buf.str());
	currentMoogScript.clear();
	currentMoogScript.seekg(0);
	//std::string str(currentMoogScript.str());
	scriptFile.close();
}

void gigaMoog::writeMoveOff(MessageSender& ms) {
	//REMINDER: gmoog memory is in blocks of 8 channels, not necessary to clear every channel.
	for (int stepID = 0; stepID < 256; stepID++) {
		for (int channel = 0; channel < 6; channel++) {
			Message m = Message::make().destination(MessageDestination::KA007)
				.DAC(MessageDAC::DAC0).channel(channel*8)
				.setting(MessageSetting::MOVEFREQUENCY)
				.frequencyMHz(0).amplitudePercent(0).phaseDegrees(0.0).instantFTW(1).ATWIncr(0).stepSequenceID(stepID).FTWIncr(0).phaseJump(0);;
			ms.enqueue(m);
		}

		for (int channel = 0; channel < 6; channel++) {
			Message m = Message::make().destination(MessageDestination::KA007)
				.DAC(MessageDAC::DAC1).channel(channel*8)
				.setting(MessageSetting::MOVEFREQUENCY)
				.frequencyMHz(0).amplitudePercent(0).phaseDegrees(0.0).instantFTW(1).ATWIncr(0).stepSequenceID(stepID).FTWIncr(0).phaseJump(0);;
			ms.enqueue(m);
		}

		//TODO: put back in after programming rate fixed, and when using both rails.
		//for (int channel = 0; channel < 48; channel++) {
		//	Message m = Message::make().destination(MessageDestination::KA007)
		//		.DAC(MessageDAC::DAC2).channel(channel)
		//		.setting(MessageSetting::MOVEFREQUENCY)
		//		.frequencyMHz(0).amplitudePercent(0).phaseDegrees(0.0).instantFTW(1).ATWIncr(0).stepSequenceID(stepID).FTWIncr(0).phaseJump(0);;
		//	ms.enqueue(m);
		//}

		//for (int channel = 0; channel < 48; channel++) {
		//	Message m = Message::make().destination(MessageDestination::KA007)
		//		.DAC(MessageDAC::DAC3).channel(channel)
		//		.setting(MessageSetting::MOVEFREQUENCY)
		//		.frequencyMHz(0).amplitudePercent(0).phaseDegrees(0.0).instantFTW(1).ATWIncr(0).stepSequenceID(stepID).FTWIncr(0).phaseJump(0);;
		//	ms.enqueue(m);
		//}
	}
}

void gigaMoog::writeOff(MessageSender& ms) {

	for (int channel = 0; channel < 48; channel++) {
		Message m = Message::make().destination(MessageDestination::KA007)
			.DAC(MessageDAC::DAC0).channel(channel)
			.setting(MessageSetting::LOADFREQUENCY)
			.frequencyMHz(0).amplitudePercent(0).phaseDegrees(0.0);;
		ms.enqueue(m);
	}

	for (int channel = 0; channel < 48; channel++) {
		Message m = Message::make().destination(MessageDestination::KA007)
			.DAC(MessageDAC::DAC1).channel(channel)
			.setting(MessageSetting::LOADFREQUENCY)
			.frequencyMHz(0).amplitudePercent(0).phaseDegrees(0.0);;
		ms.enqueue(m);
	}

	for (int channel = 0; channel < 48; channel++) {
		Message m = Message::make().destination(MessageDestination::KA007)
			.DAC(MessageDAC::DAC2).channel(channel)
			.setting(MessageSetting::LOADFREQUENCY)
			.frequencyMHz(0).amplitudePercent(0).phaseDegrees(0.0);;
		ms.enqueue(m);
	}

	for (int channel = 0; channel < 48; channel++) {
		Message m = Message::make().destination(MessageDestination::KA007)
			.DAC(MessageDAC::DAC3).channel(channel)
			.setting(MessageSetting::LOADFREQUENCY)
			.frequencyMHz(0).amplitudePercent(0).phaseDegrees(0.0);;
		ms.enqueue(m);
	}

	//{
	//	Message m = Message::make().destination(MessageDestination::KA007)
	//		.setting(MessageSetting::TERMINATE_SEQ);
	//	ms.enqueue(m);
	//}

	//ms.getQueueElementCount();
	//MessagePrinter rec;
	//fpga.setReadCallback(boost::bind(&MessagePrinter::callback, rec, _1));
	//fpga.write(ms.getMessageBytes());
}

void gigaMoog::writeLoad(MessageSender& ms)
{
	//Since writeLoad always called before rearrange, just do auto tweezer offset here.
	if (autoTweezerOffsetActive)
	{
		xOffset = xOffsetManual + xOffsetAuto;
		yOffset = yOffsetManual + yOffsetAuto;
	}
	else
	{
		xOffset = xOffsetManual;
		yOffset = yOffsetManual;
	}

	//Write load settings based on initXY

	{
		size_t iTweezer = 0;
		size_t iMask = 0;
		for (auto const& channelBool : initialPositionsX)
		{
			double phase, amp, freq;
			if (iTweezer > 16)
			{
				thrower("For safety, maximum number of x tones is limited to 16 in rearrangement mode");
			}
			if (channelBool)
			{
				//size_t hardwareChannel = iTweezer % 2 + 8 * (iTweezer / 2);
				size_t hardwareChannel = (iTweezer * 8) % 48 + (iTweezer * 8) / 48;
				phase = fmod(180 * pow(iTweezer + 1, 2) / nTweezerX, 360); //this assumes comb of even tones.
				Message m = Message::make().destination(MessageDestination::KA007)
					.DAC(MessageDAC::DAC0).channel(hardwareChannel)
					.setting(MessageSetting::LOADFREQUENCY)
					.frequencyMHz(FTW_LUT[0 + 2 * yDim * iMask] + xOffset).amplitudePercent(ATW_LUT[0 + 2 * yDim*iMask]).phaseDegrees(phase);;
				ms.enqueue(m);
				iTweezer++;
			}
			iMask++;
		}
	}
	{
		size_t iTweezer = 0;
		size_t iMask = 0;
		for (auto const& channelBool : initialPositionsY)
		{
			double phase, amp, freq;
			if (iTweezer > 24)
			{
				thrower("For safety, maximum number of y tones is limited to 24 in rearrangement mode");
			}
			if (channelBool)
			{
				//size_t hardwareChannel = iTweezer;
				size_t hardwareChannel = (iTweezer * 8) % 48 + (iTweezer * 8) / 48;
				phase = fmod(180 * pow(iTweezer + 1, 2) / nTweezerY, 360); //this assumes comb of even tones.
				Message m = Message::make().destination(MessageDestination::KA007)
					.DAC(MessageDAC::DAC1).channel(hardwareChannel)
					.setting(MessageSetting::LOADFREQUENCY)
					.frequencyMHz(FTW_LUT[1 + 2 * iMask] + yOffset).amplitudePercent(ATW_LUT[1 + 2 * iMask]).phaseDegrees(phase);;
				ms.enqueue(m);
				iTweezer++;
			}
			iMask++;
		}
	}
}

void gigaMoog::send(MessageSender& ms)
{
	if (!GIGAMOOG_SAFEMODE)
	{
		ms.getQueueElementCount();
		//MessagePrinter rec;
		//fpga.setReadCallback(boost::bind(&MessagePrinter::callback, rec, _1));
		//fpga.writeVector(ms.getMessageVectorBytes());
		fpga.write(ms.getMessageBytes());

	}
}

void gigaMoog::analyzeMoogScript(gigaMoog* moog, std::vector<variableType>& variables, UINT variation)
{
	refreshLUT();

	MessageSender ms;
	bool test = false;
	bool hardreset = false;

	//rearrangerActive = false;

	writeOff(ms);

	currentMoogScriptText = currentMoogScript.str();
	if (currentMoogScript.str() == "")
	{
		thrower("ERROR: Moog script is empty!\r\n");
	}
	std::string word;
	currentMoogScript >> word;
	std::vector<UINT> totalRepeatNum, currentRepeatNum;
	std::vector<std::streamoff> repeatPos;
	// the analysis loop.

	if (word == "rearrange")
	{
		//rearrangerActive = true;
		Expression ampStepNew, freqStepNew, ampStepPaintNew, freqStepPaintNew, xoff, yoff, yPaintStartExpr, yPaintEndExpr, scrunchSpacingExpression;
		std::string tmp, initAOX, initAOY, filterAOX, filterAOY;
		currentMoogScript >> rearrangeMode;

		//TODO: put syntax checks back.

		//if (rearrangeMode != "scrunchx" && rearrangeMode != "scrunchy" && rearrangeMode != "scrunchxy" && rearrangeMode != "scrunchyx" && rearrangeMode != "centerscrunchyx" &&rearrangeMode != "tetris")
		//{
		//	thrower("Invalid rearrangement mode. Valid options are scrunchx, scrunchy, scrunchxy, scrunchyx, and tetris.");
		//}
		//if (rearrangeMode == "scrunchx" || rearrangeMode == "scrunchy" || rearrangeMode == "scrunchxy" || rearrangeMode == "scrunchyx" || rearrangeMode == "centerscrunchyx" || rearrangeMode == "tetris")
		{
			currentMoogScript >> scrunchSpacingExpression;
			scrunchSpacing = scrunchSpacingExpression.evaluate(variables, variation);
		}

		currentMoogScript >> ampStepNew;
		currentMoogScript >> freqStepNew;

		ampStepMag = round(ampStepNew.evaluate(variables, variation));
		if (ampStepMag > 134217727 || ampStepMag < 0) {
			thrower("Warning: gmoog amplitude step out of range [-134217728, 134217727]");
		}

		freqStepMag = round(freqStepNew.evaluate(variables, variation));
		if (freqStepMag > 511 || ampStepMag < 0) {
			thrower("Warning: gmoog frequency step out of range [-512, 511]");
		}

		currentMoogScript >> tmp;
		if (tmp == "painter")
		{
			currentMoogScript >> yPaintStartExpr;
			currentMoogScript >> yPaintEndExpr;
			yPaintStart = (int)round(yPaintStartExpr.evaluate(variables, variation));
			yPaintEnd = (int)round(yPaintEndExpr.evaluate(variables, variation));
			
			if (yPaintStart < 0 || yPaintStart >= 48)
			{
				thrower("Invalid starting paint position.");
			}
			if (yPaintEnd <= (yPaintStart + 1) || yPaintEnd >= 48)
			{
				thrower("Invalid ending paint position.");
			}

			currentMoogScript >> ampStepPaintNew;
			currentMoogScript >> freqStepPaintNew;

			ampStepPaintMag = round(ampStepPaintNew.evaluate(variables, variation));
			if (ampStepPaintMag > 134217727 || ampStepPaintMag < 0) {
				thrower("Warning: gmoog amplitude paint step out of range [-134217728, 134217727]");
			}

			freqStepPaintMag = round(freqStepPaintNew.evaluate(variables, variation));
			if (freqStepPaintMag > 511 || ampStepPaintMag < 0) {
				thrower("Warning: gmoog frequency paint step out of range [-512, 511]");
			}

		}
		else
		{
			thrower("Error: Must first specify paint parameters.");
		}

		currentMoogScript >> tmp;
		if (tmp == "xoffset")
		{
			currentMoogScript >> xoff;
			xOffsetManual = xoff.evaluate(variables, variation);
		}
		else
		{
			thrower("Error: must first specify x frequency offset.");
		}

		currentMoogScript >> tmp;
		if (tmp == "yoffset")
		{
			currentMoogScript >> yoff;
			yOffsetManual = yoff.evaluate(variables, variation);
		}
		else
		{
			thrower("Error: must first specify y frequency offset.");
		}

		currentMoogScript >> tmp;
		if (tmp == "initx")
		{
			currentMoogScript >> initAOX;
			initialPositionsX.clear();
			nTweezerX = 0;
			for (auto &ch : initAOX) { //convert string to boolean vector
				if (ch == '0') { initialPositionsX.push_back(0); }
				else if (ch == '1') {
					initialPositionsX.push_back(1);
					nTweezerX++;
				}
				else { thrower("Error: non-boolean target value."); }
			}
		}
		else
		{
			thrower("Error: must first specify initial x values.");
		}
		currentMoogScript >> tmp;
		if (tmp == "inity")
		{
			currentMoogScript >> initAOY;
			initialPositionsY.clear();
			nTweezerY = 0;
			for (auto &ch : initAOY) { //convert string to boolean vector
				if (ch == '0') { initialPositionsY.push_back(0); }
				else if (ch == '1') {
					initialPositionsY.push_back(1);
					nTweezerY++;
				}
				else { thrower("Error: non-boolean target value."); }
			}
		}
		else
		{
			thrower("Error: must first specify initial y values.");
		}

		initialPositions.clear();
		for (auto &tweezerBoolY : initialPositionsY)
		{
			for (auto &tweezerBoolX : initialPositionsX)
			{
				initialPositions.push_back(tweezerBoolX*tweezerBoolY);
			}
		}


		if (initAOX.length() != xDim || initAOY.length() != yDim)
		{
			thrower("Error: initial positions must match tweezer look up table size.");
		}

		currentMoogScript >> tmp;
		if (tmp == "filterx")
		{
			currentMoogScript >> filterAOX;
			filterPositionsX.clear();
			nFilterTweezerX = 0;
			for (auto &ch : filterAOX) { //convert string to boolean vector
				if (ch == '0') { filterPositionsX.push_back(0); }
				else if (ch == '1') {
					filterPositionsX.push_back(1);
					nFilterTweezerX++;
				}
				else { thrower("Error: non-boolean target value."); }
			}
		}
		else
		{
			thrower("Error: must first specify filter x values.");
		}

		currentMoogScript >> tmp;
		if (tmp == "filtery")
		{
			currentMoogScript >> filterAOY;
			filterPositionsY.clear();
			nFilterTweezerY = 0;
			for (auto &ch : filterAOY) { //convert string to boolean vector
				if (ch == '0') { filterPositionsY.push_back(0); }
				else if (ch == '1') {
					filterPositionsY.push_back(1);
					nFilterTweezerY++;
				}
				else { thrower("Error: non-boolean target value."); }
			}
		}
		else
		{
			thrower("Error: must first specify filter y values.");
		}

		currentMoogScript >> tmp;
		if (tmp == "targetstart")
		{
			targetPositions.clear();
			targetNumber = 0;

			for (size_t i = 0; i < yDim; i++)
			{
				//std::vector<bool> rowVect;
				//currentMoogScript >> tmp;
				//for (auto &ch : tmp) { //convert string to boolean vector
				//	if (ch == '0') { rowVect.push_back(0); }
				//	else if (ch == '1') {
				//		rowVect.push_back(1);
				//		targetNumber += 1; //count total desired atom number
				//	}
				//	else { thrower("Error: non-boolean target value."); }
				//}
				//if (rowVect.size() != xDim) { thrower("Error: invalid target dimensions"); }
				//targetPositions.push_back(rowVect);
				currentMoogScript >> tmp;
				for (auto &ch : tmp) { //convert string to boolean vector
					if (ch == '0') { targetPositions.push_back(0); }
					else if (ch == '1') {
						targetPositions.push_back(1);
						targetNumber += 1; //count total desired atom number
					}
					else { thrower("Error: non-boolean target value."); }
				}
				if (tmp.size() != xDim) { thrower("Error: invalid target dimensions"); }
			}

			currentMoogScript >> tmp;
			if (tmp != "targetend")
			{
				thrower("Error: invalid target dimensions");
			}
		}
		else
		{
			thrower("Error: must specify target locations.");
		}

		while (!(currentMoogScript.peek() == EOF) || word != "__end__")
		{
			if (word == "rearrange") {}
			else if (word == "hardreset") {
				hardreset = true;
			}
			else if (word == "test") {
				test = true;
			}
			//else if (word == "rearrangeroff")
			//{
			//	rearrangerActive = false;
			//}
			else
			{
				thrower("ERROR: unrecognized moog script command: \"" + word + "\"");
			}
			word = "";
			currentMoogScript >> word;
		}

		writeLoad(ms);
	}
	else { //Normal manual gmoog settings.
		while (!(currentMoogScript.peek() == EOF) || word != "__end__")
		{
			if (word == "set") {
				std::string DAC;
				Expression channel, amplitude, frequency, phase;
				currentMoogScript >> DAC;
				currentMoogScript >> channel;
				currentMoogScript >> amplitude;
				currentMoogScript >> frequency;
				currentMoogScript >> phase;

				MessageDAC dacset;
				if (DAC == "dac0") {
					dacset = MessageDAC::DAC0;
				}
				else if (DAC == "dac1") {
					dacset = MessageDAC::DAC1;
				}
				else if (DAC == "dac2") {
					dacset = MessageDAC::DAC2;
				}
				else if (DAC == "dac3") {
					dacset = MessageDAC::DAC3;
				}
				else {
					thrower("ERROR: unrecognized moog DAC selection: \"" + DAC + "\"");
				}

				Message m = Message::make().destination(MessageDestination::KA007)
					.DAC(dacset).channel(channel.evaluate(variables, variation))
					.setting(MessageSetting::LOADFREQUENCY)
					.frequencyMHz(frequency.evaluate(variables, variation)).amplitudePercent(amplitude.evaluate(variables, variation)).phaseDegrees(phase.evaluate(variables, variation));
				ms.enqueue(m);
			}
			else if (word == "setmove") {
				std::string DAC;
				Expression channel, amplitude, frequency, phase, ampIncr, freqIncr;
				std::string jumpFreq, jumpPhase, snapshotID;

				currentMoogScript >> snapshotID;
				currentMoogScript >> DAC;
				currentMoogScript >> channel;
				currentMoogScript >> jumpFreq;
				currentMoogScript >> jumpPhase;
				currentMoogScript >> amplitude;
				currentMoogScript >> ampIncr;
				currentMoogScript >> frequency;
				currentMoogScript >> freqIncr;
				currentMoogScript >> phase;

				MessageDAC dacset;
				if (DAC == "dac0") {
					dacset = MessageDAC::DAC0;
				}
				else if (DAC == "dac1") {
					dacset = MessageDAC::DAC1;
				}
				else if (DAC == "dac2") {
					dacset = MessageDAC::DAC2;
				}
				else if (DAC == "dac3") {
					dacset = MessageDAC::DAC3;
				}
				else {
					thrower("ERROR: unrecognized moog DAC selection: \"" + DAC + "\"");
				}
				Message m = Message::make().destination(MessageDestination::KA007)
					.DAC(dacset).channel(channel.evaluate(variables, variation))
					.setting(MessageSetting::MOVEFREQUENCY)
					.frequencyMHz(frequency.evaluate(variables, variation)).amplitudePercent(amplitude.evaluate(variables, variation)).phaseDegrees(phase.evaluate(variables, variation)).instantFTW(stoul(jumpFreq, nullptr)).ATWIncr(round(ampIncr.evaluate(variables, variation))).stepSequenceID(stoul(snapshotID, nullptr)).FTWIncr(round(freqIncr.evaluate(variables, variation))).phaseJump(stoul(jumpPhase, nullptr));
				ms.enqueue(m);
			}
			else if (word == "hardreset") {
				hardreset = true;
			}
			else if (word == "rearrange") {
				thrower("Rearrangement not allowed when setting tones manually.");
			}
			else if (word == "test") {
				test = true;
			}
			else
			{
				thrower("ERROR: unrecognized moog script command: \"" + word + "\"");
			}
			word = "";
			currentMoogScript >> word;
		}
	}

	writeTerminator(ms);
	send(ms);

	if (test) {
		moveSingle single;
		moveSequence testInput;
		MessageSender msTest;
		//size_t ypos = 0;
		//for (size_t i = 0; i < 16; i++)
		//{
		//	single.startAOX = { 0,8,15 };
		//	single.startAOY = { ypos };
		//	single.endAOX = { 0,14,15 };
		//	single.endAOY = { ypos };
		//	testInput.moves.push_back(single);
		//	if ( i % 2 == 0) {
		//		ypos += 1;
		//	}
		//	else {
		//		ypos -= 1;
		//	}
		//}
		//testInput.moves[0].endAOX = { 0,8,15 };
		//testInput.moves[15].endAOX = { 0,1,15 };

		single.startAOX = { 0,1,2,3,4,5,6,7 };
		single.startAOY = { 0 };
		single.endAOX = { 8,9,10,11,12,13,14,15 };
		single.endAOY = { 0 };
		testInput.moves.push_back(single);
		single.startAOX = { 8,9,10,11,12,13,14,15 };
		single.startAOY = { 0 };
		single.endAOX = { 0,1,2,3,4,5,6,7 };
		single.endAOY = { 0 };
		testInput.moves.push_back(single);
		
		writeOff(msTest);
		writeLoad(msTest);
		writeRearrangeMoves(testInput, msTest);
		writeTerminator(msTest);
		send(msTest);
	}

	if (hardreset)
	{
		for (size_t i = 0; i < 2; i++) //write twice to reset both sets of memory.
		{
			{
				MessageSender msReset;
				writeOff(msReset);
				writeMoveOff(msReset);
				writeTerminator(msReset);
				send(msReset);
			}
		}
	}
}

void gigaMoog::updateXYOffsetAuto() {
	///Updates the offsets in MHz
	std::vector<double> vx = xPix2MHz;	
	std::transform(vx.begin(), vx.end(), vx.begin(),
		std::bind(std::multiplies<double>(), std::placeholders::_1, xPixelOffsetAuto)); //convert offset in pixels to MHz

	std::vector<double> vy = yPix2MHz;
	std::transform(vy.begin(), vy.end(), vy.begin(),
		std::bind(std::multiplies<double>(), std::placeholders::_1, yPixelOffsetAuto)); //convert offset in pixels to MHz

	std::vector<double> vs(subpixelLUT.begin() + subpixelIndexOffsetAuto * 2 , subpixelLUT.begin() + subpixelIndexOffsetAuto * 2 + 2); //select appropriate offsets given subpixel masks.

	std::transform(vs.begin(), vs.end(), vx.begin(), vs.begin(), std::plus<double>()); // add pixel offsets to vs
	std::transform(vs.begin(), vs.end(), vy.begin(), vs.begin(), std::plus<double>());


	if (xOffsetAuto == NULL || yOffsetAuto == NULL)
	{
		xOffsetAuto = vs[1];
		yOffsetAuto = vs[0];
	}
	//else
	//{
	//	if (std::abs(xOffsetAuto - vs[1]) > 1.6) // if a large jump, immediately go to new value. 
	//	{
	//		xOffsetAuto = vs[1];
	//	}
	//	else
	//	{
	//		xOffsetAuto = (5 * xOffsetAuto + vs[1]) / 6;
	//	}
	//	if (std::abs(yOffsetAuto - vs[0]) > 1.6)
	//	{
	//		yOffsetAuto = vs[0];
	//	}
	//	else
	//	{
	//		yOffsetAuto = (5 * yOffsetAuto + vs[0]) / 6;
	//	}
	//}
	else
	{
		xOffsetAuto = (5 * xOffsetAuto + vs[1]) / 6;
		yOffsetAuto = (5 * yOffsetAuto + vs[0]) / 6;
	}

	if (xOffsetAuto > 1.3)
	{
		xOffsetAuto -= 1.92457761; //TODO: don't hard code this, just use LUT.
	}
	else if (xOffsetAuto < -1.3)
	{
		xOffsetAuto += 1.92457761;
	}
	if (yOffsetAuto > 1.3)
	{
		yOffsetAuto -= 1.98202151;
	}
	else if (yOffsetAuto < -1.3)
	{
		yOffsetAuto += 1.98202151;
	}

};