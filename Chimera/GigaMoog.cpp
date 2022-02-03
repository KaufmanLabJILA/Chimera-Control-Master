#include "stdafx.h"
#include "GigaMoog.h"
#include "cnpy.h"

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

gigaMoog::gigaMoog(std::string portID, int baudrate) : fpga(portID, baudrate) {
	if (!GIGAMOOG_SAFEMODE) {
		//writeOff();
	}
}

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

	xDim = arrAmpLUT.shape[0];
	yDim = arrAmpLUT.shape[1]; //Get np array dimensions

	ATW_LUT.clear();
	FTW_LUT.clear();
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

	cnpy::NpyArray arrSubpixelLUT = cnpy::npy_load(SUBPIXELLUT_FILE_LOCATION);
	subpixelLUT = arrSubpixelLUT.as_vec<double>();
	nSubpixel = arrSubpixelLUT.shape[0];
	xPix2MHz = { subpixelLUT[sqrt(nSubpixel)-1]*(-2), subpixelLUT[sqrt(nSubpixel)] * (-2) }; //y then x for consistency. Note, indices doubled because flattened array of doublets.
	yPix2MHz = { subpixelLUT[nSubpixel - 1 + sqrt(nSubpixel) - 1] * 2 , subpixelLUT[nSubpixel - 1 + sqrt(nSubpixel) - 1 + 1] * 2 }; //y then x for consistency.
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

	//MessageSender ms;
	//First write all zeroes for load. Depreciated 220124
	//writeOff(ms);
	writeMoveOff(ms);

	size_t nx, ny;
	double phase, amp, freq, ampPrev, freqPrev;
	int ampstep, freqstep;

	//step 0: turn off all load tones.
	for (int channel = 0; channel < 16; channel++) {//TODO: 16 could be changed to 64 if using more tones for rearrangement
		if (channel < nTweezerX)
		{
			//size_t hardwareChannel = channel % 2 + 8 * (channel / 2); //OLD
			size_t hardwareChannel = (channel * 8) % 64 + (channel * 8) / 64;
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
			size_t hardwareChannel = (channel * 8) % 64 + (channel * 8) / 64;
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
		for (int channel = 0; channel < 16; channel++) {//TODO: 16 could be changed to 64 if using more tones for rearrangement
			if (ny > 1 && nx == 1 && channel < 3) //Triple up tones if only a single tone on, assuming y axis not already tripled.
			{
				//size_t hardwareChannel = 8 * channel; //there are 256 memory locations for each group of 8
				//size_t hardwareChannel = (channel * 8) % 64 + (channel * 8) / 64;
				size_t hardwareChannel = hardwareChannelsDAC0[channel];
				freq = FTW_LUT[
					2 * yDim * input.moves[stepID].startAOX[0] + 2 * input.moves[stepID].startAOY[0] + 0
				] + xOffset;
				amp = ATW_LUT[
					2 * yDim * input.moves[stepID].startAOX[0] + 2 * input.moves[stepID].startAOY[0] + 0
				];
				Message m = Message::make().destination(MessageDestination::KA007)
					.DAC(MessageDAC::DAC0).channel(hardwareChannel)
					.setting(MessageSetting::MOVEFREQUENCY)
					.frequencyMHz(freq).amplitudePercent(amp).phaseDegrees(0).instantFTW(1).ATWIncr(ampStepMag).stepSequenceID(3 * stepID + 1).FTWIncr(0).phaseJump(1);;
				ms.enqueue(m);
			}
			else if (ny != 0 && nx != 0 && channel < nx) {
				//size_t hardwareChannel = (channel * 8) % 64 + (channel * 8) / 64; //there are 256 memory locations for each group of 8 channels, want to populate blocks of memory evenly.
				size_t hardwareChannel = hardwareChannelsDAC0[channel];
				freq = FTW_LUT[
					2 * yDim * input.moves[stepID].startAOX[channel] + 2 * input.moves[stepID].startAOY[0] + 0
				] + xOffset;
				amp = ATW_LUT[
					2 * yDim * input.moves[stepID].startAOX[channel] + 2 * input.moves[stepID].startAOY[0] + 0
				];
				phase = fmod(180 * pow(channel + 1, 2) / nx, 360); //this assumes comb of even tones, imperfect, but also short duration so not super critical, and fast.

				Message m = Message::make().destination(MessageDestination::KA007)
					.DAC(MessageDAC::DAC0).channel(hardwareChannel)
					.setting(MessageSetting::MOVEFREQUENCY)
					.frequencyMHz(freq).amplitudePercent(amp).phaseDegrees(phase).instantFTW(1).ATWIncr(ampStepMag).stepSequenceID(3 * stepID + 1).FTWIncr(0).phaseJump(1);;
				ms.enqueue(m);
			}
			//else //populate extra channels with null moves.
			//{
			//	//size_t hardwareChannel = (channel * 8) % 64 + (channel * 8) / 64;
			//	size_t hardwareChannel = hardwareChannelsDAC0[channel];
			//	Message m = Message::make().destination(MessageDestination::KA007)
			//		.DAC(MessageDAC::DAC0).channel(hardwareChannel)
			//		.setting(MessageSetting::MOVEFREQUENCY)
			//		.frequencyMHz(0).amplitudePercent(0.01).phaseDegrees(0).instantFTW(1).ATWIncr(-ampStepMag).stepSequenceID(3 * stepID + 1).FTWIncr(0).phaseJump(1);;
			//	ms.enqueue(m);
			//}

		}

		for (int channel = 0; channel < 16; channel++) {
			if (nx != 0 && ny == 1 && channel < 3) //Triple up tones if only a single tone on.
			{
				//size_t hardwareChannel = 8 * channel; //there are 256 memory locations for each group of 8
				//size_t hardwareChannel = (channel * 8) % 64 + (channel * 8) / 64;
				size_t hardwareChannel = hardwareChannelsDAC1[channel];
				freq = FTW_LUT[
					2 * yDim * input.moves[stepID].startAOX[0] + 2 * input.moves[stepID].startAOY[0] + 1
				] + yOffset;
				amp = ATW_LUT[
					2 * yDim * input.moves[stepID].startAOX[0] + 2 * input.moves[stepID].startAOY[0] + 1
				];

				Message m = Message::make().destination(MessageDestination::KA007)
					.DAC(MessageDAC::DAC1).channel(hardwareChannel)
					.setting(MessageSetting::MOVEFREQUENCY)
					.frequencyMHz(freq).amplitudePercent(amp).phaseDegrees(0).instantFTW(1).ATWIncr(ampStepMag).stepSequenceID(3 * stepID + 1).FTWIncr(0).phaseJump(1);;
				ms.enqueue(m);
			}
			else if (nx != 0 && ny != 0 && channel < ny) {
				//size_t hardwareChannel = channel % 2 + 8 * (channel / 2); //there are 256 memory locations for each group of 8
				//size_t hardwareChannel = (channel * 8) % 64 + (channel * 8) / 64;
				size_t hardwareChannel = hardwareChannelsDAC1[channel];
				freq = FTW_LUT[
					2 * yDim * input.moves[stepID].startAOX[0] + 2 * input.moves[stepID].startAOY[channel] + 1
				] + yOffset;
				amp = ATW_LUT[
					2 * yDim * input.moves[stepID].startAOX[0] + 2 * input.moves[stepID].startAOY[channel] + 1
				];
				phase = fmod(180 * pow(channel + 1, 2) / ny, 360);

				Message m = Message::make().destination(MessageDestination::KA007)
					.DAC(MessageDAC::DAC1).channel(hardwareChannel)
					.setting(MessageSetting::MOVEFREQUENCY)
					.frequencyMHz(freq).amplitudePercent(amp).phaseDegrees(phase).instantFTW(1).ATWIncr(ampStepMag).stepSequenceID(3 * stepID + 1).FTWIncr(0).phaseJump(1);;
				ms.enqueue(m);
			}
			//else //populate extra channels with null moves.
			//{
			//	//size_t hardwareChannel = (channel * 8) % 64 + (channel * 8) / 64;
			//	size_t hardwareChannel = hardwareChannelsDAC1[channel];
			//	Message m = Message::make().destination(MessageDestination::KA007)
			//		.DAC(MessageDAC::DAC1).channel(hardwareChannel)
			//		.setting(MessageSetting::MOVEFREQUENCY)
			//		.frequencyMHz(0).amplitudePercent(0.01).phaseDegrees(0).instantFTW(1).ATWIncr(-ampStepMag).stepSequenceID(3 * stepID + 1).FTWIncr(0).phaseJump(1);;
			//	ms.enqueue(m);
			//}
		}

		//step 2: ramp to new locations
		for (int channel = 0; channel < 16; channel++) {
			if (ny > 1 && nx == 1 && channel < 3) //Triple up tones if only a single tone on.
			{
				//size_t hardwareChannel = (channel * 8) % 64 + (channel * 8) / 64;
				size_t hardwareChannel = hardwareChannelsDAC0[channel];
				freqPrev = FTW_LUT[
					2 * yDim * input.moves[stepID].startAOX[0] + 2 * input.moves[stepID].startAOY[0] + 0
				] + xOffset;
				ampPrev = ATW_LUT[
					2 * yDim * input.moves[stepID].startAOX[0] + 2 * input.moves[stepID].startAOY[0] + 0
				];
				freq = FTW_LUT[
					2 * yDim * input.moves[stepID].endAOX[0] + 2 * input.moves[stepID].endAOY[0] + 0
				] + xOffset;
				amp = ATW_LUT[
					2 * yDim * input.moves[stepID].endAOX[0] + 2 * input.moves[stepID].endAOY[0] + 0
				];

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
				//size_t hardwareChannel = (channel * 8) % 64 + (channel * 8) / 64;
				size_t hardwareChannel = hardwareChannelsDAC0[channel];
				freqPrev = FTW_LUT[
					2 * yDim * input.moves[stepID].startAOX[channel] + 2 * input.moves[stepID].startAOY[0] + 0
				] + xOffset;
				ampPrev = ATW_LUT[
					2 * yDim * input.moves[stepID].startAOX[channel] + 2 * input.moves[stepID].startAOY[0] + 0
				];
				freq = FTW_LUT[
					2 * yDim * input.moves[stepID].endAOX[channel] + 2 * input.moves[stepID].endAOY[0] + 0
				] + xOffset;
				amp = ATW_LUT[
					2 * yDim * input.moves[stepID].endAOX[channel] + 2 * input.moves[stepID].endAOY[0] + 0
				];

				ampstep = (amp < ampPrev) ? -ampStepMag : ampStepMag; //Change sign of steps appropriately.
				freqstep = (freq < freqPrev) ? -freqStepMag : freqStepMag;

				Message m = Message::make().destination(MessageDestination::KA007)
					.DAC(MessageDAC::DAC0).channel(hardwareChannel)
					.setting(MessageSetting::MOVEFREQUENCY)
					.frequencyMHz(freq).amplitudePercent(amp).phaseDegrees(0).instantFTW(0).ATWIncr(ampstep).stepSequenceID(3 * stepID + 1 + 1).FTWIncr(freqstep).phaseJump(0);;
				ms.enqueue(m);
			}
			//else
			//{
			//	//size_t hardwareChannel = (channel * 8) % 64 + (channel * 8) / 64;
			//	size_t hardwareChannel = hardwareChannelsDAC0[channel];
			//	Message m = Message::make().destination(MessageDestination::KA007)
			//		.DAC(MessageDAC::DAC0).channel(hardwareChannel)
			//		.setting(MessageSetting::MOVEFREQUENCY)
			//		.frequencyMHz(0).amplitudePercent(0.01).phaseDegrees(0).instantFTW(1).ATWIncr(-ampStepMag).stepSequenceID(3 * stepID + 1 + 1).FTWIncr(0).phaseJump(1);;
			//	ms.enqueue(m);
			//}
		}

		for (int channel = 0; channel < 16; channel++) {
			if (nx != 0 && ny == 1 && channel < 3) //Triple up tones if only a single tone on.
			{
				//size_t hardwareChannel = (channel * 8) % 64 + (channel * 8) / 64;
				size_t hardwareChannel = hardwareChannelsDAC1[channel];
				freqPrev = FTW_LUT[
					2 * yDim * input.moves[stepID].startAOX[0] + 2 * input.moves[stepID].startAOY[0] + 1
				] + yOffset;
				ampPrev = ATW_LUT[
					2 * yDim * input.moves[stepID].startAOX[0] + 2 * input.moves[stepID].startAOY[0] + 1
				];
				freq = FTW_LUT[
					2 * yDim * input.moves[stepID].endAOX[0] + 2 * input.moves[stepID].endAOY[0] + 1
				] + yOffset;
				amp = ATW_LUT[
					2 * yDim * input.moves[stepID].endAOX[0] + 2 * input.moves[stepID].endAOY[0] + 1
				];

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
				//size_t hardwareChannel = (channel * 8) % 64 + (channel * 8) / 64;
				size_t hardwareChannel = hardwareChannelsDAC1[channel];
				freqPrev = FTW_LUT[
					2 * yDim * input.moves[stepID].startAOX[0] + 2 * input.moves[stepID].startAOY[channel] + 1
				] + yOffset;
				ampPrev = ATW_LUT[
					2 * yDim * input.moves[stepID].startAOX[0] + 2 * input.moves[stepID].startAOY[channel] + 1
				];
				freq = FTW_LUT[
					2 * yDim * input.moves[stepID].endAOX[0] + 2 * input.moves[stepID].endAOY[channel] + 1
				] + yOffset;
				amp = ATW_LUT[
					2 * yDim * input.moves[stepID].endAOX[0] + 2 * input.moves[stepID].endAOY[channel] + 1
				];

				ampstep = (amp < ampPrev) ? -ampStepMag : ampStepMag; //Change sign of steps appropriately.
				freqstep = (freq < freqPrev) ? -freqStepMag : freqStepMag;

				Message m = Message::make().destination(MessageDestination::KA007)
					.DAC(MessageDAC::DAC1).channel(hardwareChannel)
					.setting(MessageSetting::MOVEFREQUENCY)
					.frequencyMHz(freq).amplitudePercent(amp).phaseDegrees(0).instantFTW(0).ATWIncr(ampstep).stepSequenceID(3 * stepID + 1 + 1).FTWIncr(freqstep).phaseJump(0);;
				ms.enqueue(m);
			}
			//else
			//{
			//	//size_t hardwareChannel = (channel * 8) % 64 + (channel * 8) / 64;
			//	size_t hardwareChannel = hardwareChannelsDAC1[channel];
			//	Message m = Message::make().destination(MessageDestination::KA007)
			//		.DAC(MessageDAC::DAC1).channel(hardwareChannel)
			//		.setting(MessageSetting::MOVEFREQUENCY)
			//		.frequencyMHz(0).amplitudePercent(0.01).phaseDegrees(0).instantFTW(1).ATWIncr(-ampStepMag).stepSequenceID(3 * stepID + 1 + 1).FTWIncr(0).phaseJump(1);;
			//	ms.enqueue(m);
			//}
		}

		//step 3: ramp all tones to 0
		for (int channel = 0; channel < 16; channel++) {
			if (ny > 1 && nx == 1 && channel < 3) //Triple up tones if only a single tone on.
			{
				//size_t hardwareChannel = (channel * 8) % 64 + (channel * 8) / 64;
				size_t hardwareChannel = hardwareChannelsDAC0[channel];
				freq = FTW_LUT[
					2 * yDim * input.moves[stepID].endAOX[0] + 2 * input.moves[stepID].endAOY[0] + 0
				] + xOffset;
				Message m = Message::make().destination(MessageDestination::KA007)
					.DAC(MessageDAC::DAC0).channel(hardwareChannel)
					.setting(MessageSetting::MOVEFREQUENCY)
					.frequencyMHz(freq).amplitudePercent(0.01).phaseDegrees(0).instantFTW(1).ATWIncr(-ampStepMag).stepSequenceID(3 * stepID + 2 + 1).FTWIncr(0).phaseJump(0);;
				ms.enqueue(m);
			}
			else if (ny != 0 && nx != 0 && channel < nx) {
				//size_t hardwareChannel = (channel * 8) % 64 + (channel * 8) / 64;
				size_t hardwareChannel = hardwareChannelsDAC0[channel];
				freq = FTW_LUT[
					2 * yDim * input.moves[stepID].endAOX[channel] + 2 * input.moves[stepID].endAOY[0] + 0
				] + xOffset;
				Message m = Message::make().destination(MessageDestination::KA007)
					.DAC(MessageDAC::DAC0).channel(hardwareChannel)
					.setting(MessageSetting::MOVEFREQUENCY)
					.frequencyMHz(freq).amplitudePercent(0.01).phaseDegrees(0).instantFTW(1).ATWIncr(-ampStepMag).stepSequenceID(3 * stepID + 2 + 1).FTWIncr(0).phaseJump(0);;
				ms.enqueue(m);
				//Has trouble with ramping to 0 amp for some reason - set to ~1 LSB = 100/65535.
			}
			//else
			//{
			//	//size_t hardwareChannel = (channel * 8) % 64 + (channel * 8) / 64;
			//	size_t hardwareChannel = hardwareChannelsDAC0[channel];
			//	Message m = Message::make().destination(MessageDestination::KA007)
			//		.DAC(MessageDAC::DAC0).channel(hardwareChannel)
			//		.setting(MessageSetting::MOVEFREQUENCY)
			//		.frequencyMHz(0).amplitudePercent(0.01).phaseDegrees(0).instantFTW(1).ATWIncr(-ampStepMag).stepSequenceID(3 * stepID + 2 + 1).FTWIncr(0).phaseJump(1);;
			//	ms.enqueue(m);
			//}
		}

		for (int channel = 0; channel < 16; channel++) {
			if (nx != 0 && ny == 1 && channel < 3) //Triple up tones if only a single tone on.
			{
				//size_t hardwareChannel = (channel * 8) % 64 + (channel * 8) / 64;
				size_t hardwareChannel = hardwareChannelsDAC1[channel];
				freq = FTW_LUT[
					2 * yDim * input.moves[stepID].endAOX[0] + 2 * input.moves[stepID].endAOY[0] + 1
				] + yOffset;
				Message m = Message::make().destination(MessageDestination::KA007)
					.DAC(MessageDAC::DAC1).channel(hardwareChannel)
					.setting(MessageSetting::MOVEFREQUENCY)
					.frequencyMHz(freq).amplitudePercent(0.01).phaseDegrees(0).instantFTW(1).ATWIncr(-ampStepMag).stepSequenceID(3 * stepID + 2 + 1).FTWIncr(0).phaseJump(0);;
				ms.enqueue(m);
			}
			else if (nx != 0 && ny != 0 && channel < ny) {
				//size_t hardwareChannel = (channel * 8) % 64 + (channel * 8) / 64;
				size_t hardwareChannel = hardwareChannelsDAC1[channel];
				freq = FTW_LUT[
					2 * yDim * input.moves[stepID].endAOX[0] + 2 * input.moves[stepID].endAOY[channel] + 1
				] + yOffset;
				Message m = Message::make().destination(MessageDestination::KA007)
					.DAC(MessageDAC::DAC1).channel(hardwareChannel)
					.setting(MessageSetting::MOVEFREQUENCY)
					.frequencyMHz(freq).amplitudePercent(0.01).phaseDegrees(0).instantFTW(1).ATWIncr(-ampStepMag).stepSequenceID(3 * stepID + 2 + 1).FTWIncr(0).phaseJump(0);;
				ms.enqueue(m);
			}
			//else
			//{
			//	//size_t hardwareChannel = (channel * 8) % 64 + (channel * 8) / 64;
			//	size_t hardwareChannel = hardwareChannelsDAC1[channel];
			//	Message m = Message::make().destination(MessageDestination::KA007)
			//		.DAC(MessageDAC::DAC1).channel(hardwareChannel)
			//		.setting(MessageSetting::MOVEFREQUENCY)
			//		.frequencyMHz(0).amplitudePercent(0.01).phaseDegrees(0).instantFTW(1).ATWIncr(-ampStepMag).stepSequenceID(3 * stepID + 2 + 1).FTWIncr(0).phaseJump(1);;
			//	ms.enqueue(m);
			//}
		}
	}

	//additional snapshot ramping down all channels - unclear why needed, but prevents extra trigger issues.

	for (int channel = 0; channel < 64; channel++) {//TODO: 16 could be changed to 64 if using more tones for rearrangement
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

	//for (int channel = 0; channel < nx; channel++) {
	//	Message m = Message::make().destination(MessageDestination::KA007)
	//		.DAC(MessageDAC::DAC0).channel(channel)
	//		.setting(MessageSetting::MOVEFREQUENCY)
	//		.frequencyMHz(1).amplitudePercent(0).phaseDegrees(0).instantFTW(1).ATWIncr(-ampStepMag).stepSequenceID(3 * nMoves).FTWIncr(0).phaseJump(0);;
	//	ms.enqueue(m);
	//	//Always negative step back to 0 amp. 0 FTWIncr means do not need to recompute frequency values. But cannot be 0, since that looks like terminator.
	//}

	//for (int channel = 0; channel < ny; channel++) {
	//	Message m = Message::make().destination(MessageDestination::KA007)
	//		.DAC(MessageDAC::DAC1).channel(channel)
	//		.setting(MessageSetting::MOVEFREQUENCY)
	//		.frequencyMHz(1).amplitudePercent(0).phaseDegrees(0).instantFTW(1).ATWIncr(-ampStepMag).stepSequenceID(3 * nMoves).FTWIncr(0).phaseJump(0);;
	//	ms.enqueue(m);
	//}
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
	for (int stepID = 0; stepID < 256; stepID++) {
		for (int channel = 0; channel < 8; channel++) {
			Message m = Message::make().destination(MessageDestination::KA007)
				.DAC(MessageDAC::DAC0).channel(channel*8)
				.setting(MessageSetting::MOVEFREQUENCY)
				.frequencyMHz(0).amplitudePercent(0).phaseDegrees(0.0).instantFTW(1).ATWIncr(0).stepSequenceID(stepID).FTWIncr(0).phaseJump(0);;
			ms.enqueue(m);
		}

		for (int channel = 0; channel < 8; channel++) {
			Message m = Message::make().destination(MessageDestination::KA007)
				.DAC(MessageDAC::DAC1).channel(channel*8)
				.setting(MessageSetting::MOVEFREQUENCY)
				.frequencyMHz(0).amplitudePercent(0).phaseDegrees(0.0).instantFTW(1).ATWIncr(0).stepSequenceID(stepID).FTWIncr(0).phaseJump(0);;
			ms.enqueue(m);
		}
		//TODO: put back in after programming rate fixed, and when using both rails.
		//for (int channel = 0; channel < 64; channel++) {
		//	Message m = Message::make().destination(MessageDestination::KA007)
		//		.DAC(MessageDAC::DAC2).channel(channel)
		//		.setting(MessageSetting::MOVEFREQUENCY)
		//		.frequencyMHz(0).amplitudePercent(0).phaseDegrees(0.0).instantFTW(1).ATWIncr(0).stepSequenceID(stepID).FTWIncr(0).phaseJump(0);;
		//	ms.enqueue(m);
		//}

		//for (int channel = 0; channel < 64; channel++) {
		//	Message m = Message::make().destination(MessageDestination::KA007)
		//		.DAC(MessageDAC::DAC3).channel(channel)
		//		.setting(MessageSetting::MOVEFREQUENCY)
		//		.frequencyMHz(0).amplitudePercent(0).phaseDegrees(0.0).instantFTW(1).ATWIncr(0).stepSequenceID(stepID).FTWIncr(0).phaseJump(0);;
		//	ms.enqueue(m);
		//}
	}
}

void gigaMoog::writeOff(MessageSender& ms) {

	for (int channel = 0; channel < 64; channel++) {
		Message m = Message::make().destination(MessageDestination::KA007)
			.DAC(MessageDAC::DAC0).channel(channel)
			.setting(MessageSetting::LOADFREQUENCY)
			.frequencyMHz(0).amplitudePercent(0).phaseDegrees(0.0);;
		ms.enqueue(m);
	}

	for (int channel = 0; channel < 64; channel++) {
		Message m = Message::make().destination(MessageDestination::KA007)
			.DAC(MessageDAC::DAC1).channel(channel)
			.setting(MessageSetting::LOADFREQUENCY)
			.frequencyMHz(0).amplitudePercent(0).phaseDegrees(0.0);;
		ms.enqueue(m);
	}

	for (int channel = 0; channel < 64; channel++) {
		Message m = Message::make().destination(MessageDestination::KA007)
			.DAC(MessageDAC::DAC2).channel(channel)
			.setting(MessageSetting::LOADFREQUENCY)
			.frequencyMHz(0).amplitudePercent(0).phaseDegrees(0.0);;
		ms.enqueue(m);
	}

	for (int channel = 0; channel < 64; channel++) {
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
				size_t hardwareChannel = (iTweezer * 8) % 64 + (iTweezer * 8) / 64;
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
				size_t hardwareChannel = (iTweezer * 8) % 64 + (iTweezer * 8) / 64;
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
	ms.getQueueElementCount();
	//MessagePrinter rec;
	//fpga.setReadCallback(boost::bind(&MessagePrinter::callback, rec, _1));
	fpga.write(ms.getMessageBytes());
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
		Expression ampStepNew, freqStepNew, xoff, yoff, scrunchSpacingExpression;
		std::string tmp, initAOX, initAOY;
		currentMoogScript >> rearrangeMode;

		if (rearrangeMode != "scrunchx" && rearrangeMode != "scrunchy" && rearrangeMode != "scrunchxy" && rearrangeMode != "scrunchyx" && rearrangeMode != "tetris")
		{
			thrower("Invalid rearrangement mode. Valid options are scrunchx, scrunchy, scrunchxy, scrunchyx, and tetris.");
		}
		if (rearrangeMode == "scrunchx" || rearrangeMode == "scrunchy" || rearrangeMode == "scrunchxy" || rearrangeMode == "scrunchyx" || rearrangeMode == "tetris")
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
	std::vector<double> vx = xPix2MHz;	
	std::transform(vx.begin(), vx.end(), vx.begin(),
		std::bind(std::multiplies<double>(), std::placeholders::_1, xPixelOffsetAuto)); //convert offset in pixels to MHz

	std::vector<double> vy = yPix2MHz;
	std::transform(vy.begin(), vy.end(), vy.begin(),
		std::bind(std::multiplies<double>(), std::placeholders::_1, yPixelOffsetAuto)); //convert offset in pixels to MHz

	std::vector<double> vs(subpixelLUT.begin() + subpixelIndexOffsetAuto * 2 , subpixelLUT.begin() + subpixelIndexOffsetAuto * 2 + 2); //select appropriate offsets given subpixel masks.

	std::transform(vs.begin(), vs.end(), vx.begin(), vs.begin(), std::plus<double>()); // add pixel offsets to vs
	std::transform(vs.begin(), vs.end(), vy.begin(), vs.begin(), std::plus<double>());

	xOffsetAuto = vs[1];
	yOffsetAuto = vs[0];
};