#include "stdafx.h"
#include <windows.h>
#include "DDS_SYNTH.h"
#include <sstream>

using namespace std;

DDS_SYNTH::DDS_SYNTH(std::string portID, int baudrate) : dds(portID, DDS_SAFEMODE ? -1 : baudrate) {

}

DDS_SYNTH::~DDS_SYNTH() {
	//disconnect();
}

void DDS_SYNTH::connectasync(const char devSerial[])
{
	
}

void DDS_SYNTH::disconnect()
{
	
}

std::vector<unsigned char> DDS_SYNTH::messageToVector(int message, int bytes) {
	std::vector<unsigned char> msg(bytes);
	for (int i = 0; i < bytes; i++) {
		msg[bytes - 1 - i] = (message >> i * 8);
	}
	return msg;
}

void DDS_SYNTH::clear() 
{
		dds.write(messageToVector(0x0000000000, 5));
}

void DDS_SYNTH::lockPLLs(UINT m1, UINT m2) {
	UINT8 m1_bytes = (1 << 7) + (m1 << 2);
	UINT8 m2_bytes = (1 << 7) + (m1 << 2);

	dds.write(messageToVector(0xC1, 1));
	dds.write(messageToVector(m1_bytes, 1));
	dds.write(messageToVector(0x0000, 2));

	dds.write(messageToVector(m2_bytes, 1));
	dds.write(messageToVector(0x0000, 2));

	for (int ii = 0; ii < 9; ii++) {
		dds.write(messageToVector(0x0000, 2));
	}

	Sleep(100);
}

//Input frequency in MHz
UINT* DDS_SYNTH::getFTWs(double freqs[]) {//Negative ints, Nyquist freq, works out.
	UINT FTWs[8];
	for (int ii = 0; ii < 8; ii++) {
		if (freqs[ii] > INTERNAL_CLOCK / 2) {
			thrower("DDS frequency out of range. Must be <250MHz. ");
		}
		FTWs[ii] = (UINT)round((freqs[ii] * pow(2, 32)) / (INTERNAL_CLOCK));
	}
	return(FTWs);
}

void DDS_SYNTH::writefreq(double freqs[]) {
	
	UINT* FTWs = getFTWs(freqs);

	//UINT8 byte4 = FTWs & 0x000000ffUL;
	//UINT8 byte3 = (FTWs & 0x0000ff00UL) >> 8;
	//UINT8 byte2 = (FTWs & 0x00ff0000UL) >> 16;
	//UINT8 byte1 = (FTWs & 0xff000000UL) >> 24;

}

void DDS_SYNTH::writeamp(UINT8 device, UINT8 channel, double amp) {

	//UINT ATW = getATW(amp);

	//UINT8 byte2 = ATW & 0x000000ffUL;
	//UINT8 byte1 = ATW >> 8;

	//byte1 |= 1 << 4; //Necessary to turn on amplitude multiplier.

	//write(device, 6, 0, 0, 0, byte2); //TODO: Fix this. Currently sets Mult to off and then back to force a rewrite, solving issue with inactive channels.
	//write(device, 6, 0, 0, byte1, byte2);
}

void DDS_SYNTH::loadDDSScript(std::string scriptAddress)
{
	std::ifstream scriptFile;
	// check if file address is good.
	FILE *file;
	fopen_s(&file, cstr(scriptAddress), "r");
	if (!file)
	{
		thrower("ERROR: DDS Script File " + scriptAddress + " does not exist!");
	}
	else
	{
		fclose(file);
	}
	scriptFile.open(cstr(scriptAddress));
	// check opened correctly
	if (!scriptFile.is_open())
	{
		thrower("ERROR: DDS script file passed test making sure the file exists, but it still failed to open!");
	}
	// dump the file into the stringstream.
	std::stringstream buf(std::ios_base::app | std::ios_base::out | std::ios_base::in);
	buf << scriptFile.rdbuf();
	// This is used to more easily deal some of the analysis of the script.
	buf << "\r\n\r\n__END__";
	// for whatever reason, after loading rdbuf into a stringstream, the stream seems to not 
	// want to >> into a string. tried resetting too using seekg, but whatever, this works.
	currentDDSScript.str("");
	currentDDSScript.str(buf.str());
	currentDDSScript.clear();
	currentDDSScript.seekg(0);
	//std::string str(currentMoogScript.str());
	scriptFile.close();
}


void DDS_SYNTH::programDDS(DDS_SYNTH* dds, std::vector<variableType>& variables, UINT variation)
{
	currentDDSScriptText = currentDDSScript.str();
	if (currentDDSScript.str() == "")
	{
		thrower("ERROR: DDS script is empty!\r\n");
	}
	std::string word;
	currentDDSScript >> word;
	// the analysis loop.
	while (!(currentDDSScript.peek() == EOF) || word != "__end__")
	{
		if (word == "testsequence") {
			int snapshot_num = 0;
			/*writeArrFreq(WBWRITE_ARRAY, 0x400+snapshot_num, 0);
			write(WBWRITE_ARRAY, 0x0+snapshot_num, 0, 0, 0, 0);
			snapshot_num = 1;
			writeArrFreq(WBWRITE_ARRAY, 0x400 + snapshot_num, 0);
			write(WBWRITE_ARRAY, 0x0 + snapshot_num, 0, 0, 0, 0);
			snapshot_num = 2;
			writeArrFreq(WBWRITE_ARRAY, 0x400 + snapshot_num, 0);
			write(WBWRITE_ARRAY, 0x0 + snapshot_num, 0, 0, 0, 0);*/
		}
		else if (word == "reset") {
			Expression freq, amp;
			std::string subWord;
			currentDDSScript >> subWord;

			for (UINT8 device = 0; device < 2; device++)
			{

				//write(device, 0, 0, 0, 0, 0xF0); //select all channels on device.
				//UINT8 byte1 = 1 << 4; //Necessary to turn on amplitude multiplier.
				//write(device, 6, 0, 0, 0, 0); //TODO: Fix this. Currently sets Mult to off and then back to force a rewrite, solving issue with inactive channels.
				//write(device, 6, 0, 0, byte1, 0);
			}

			if (subWord == "freqs") {
				for (UINT8 device = 0; device < 2; device++)
				{
					for (UINT8 channel = 0; channel < 4; channel++)
					{
						currentDDSScript >> freq;
						//writeArrResetFreq(device, channel, freq.evaluate(variables, variation));
						currentState.freqs[device][channel] = freq.evaluate(variables, variation);
					}
				}
			}
			else {
				thrower("Incorrect snapshot sequence. Must set repetitions, then frequencies, then amplitudes");
			}
			currentDDSScript >> subWord;
			if (subWord == "amps") {
				for (UINT8 device = 0; device < 2; device++)
				{
					for (UINT8 channel = 0; channel < 4; channel++)
					{
						currentDDSScript >> amp;
						//writeArrResetAmp(device, channel, amp.evaluate(variables, variation));
						currentState.amps[device][channel] = amp.evaluate(variables, variation);
					}
				}
			}
			else {
				thrower("Incorrect snapshot sequence. Must set repetitions, then frequencies, then amplitudes");
			}
		}
		else if (word == "snapshot") {
			Expression index, freq, amp, rep;
			UINT8 indexVal;
			UINT16 repVal;
			double freqstep, ampstep;
			std::string subWord;
			currentDDSScript >> index;
			currentDDSScript >> subWord;
			indexVal = index.evaluate(variables, variation);
			if (indexVal > 255) {
				thrower("Maximum number of DDS snapshots is 256. ");
			}
			currentState.index = indexVal;
			if (subWord == "reps") {
				currentDDSScript >> rep;
				repVal = rep.evaluate(variables, variation);
				if (repVal > 65535) {
					thrower("Maximum number of steps in ramp is 65535. ");
				}
				//writeArrReps(indexVal, repVal);
			}
			else {
				thrower("Incorrect snapshot sequence. Must set repetitions, then frequencies, then amplitudes");
			}
			currentDDSScript >> subWord;
			if (subWord == "freqs") {
				for (UINT8 device = 0; device < 2; device++)
				{
					for (UINT8 channel = 0; channel < 4; channel++)
					{
						currentDDSScript >> freq;
						freqstep = (freq.evaluate(variables, variation) - currentState.freqs[device][channel]) / static_cast<double>(repVal);
						if (freqstep > INTERNAL_CLOCK / 4) {
							thrower("DDS frequency step too large, must be <125MHz. Try adding more reps to ramp.");
						}
						//writeArrDeltaFreq(device, channel, indexVal, freqstep);
						currentState.freqs[device][channel] = freq.evaluate(variables, variation);
					}
				}
			}
			else {
				thrower("Incorrect snapshot sequence. Must set repetitions, then frequencies, then amplitudes");
			}
			currentDDSScript >> subWord;
			if (subWord == "amps") {
				for (UINT8 device = 0; device < 2; device++)
				{
					for (UINT8 channel = 0; channel < 4; channel++)
					{
						currentDDSScript >> amp;
						ampstep = (amp.evaluate(variables, variation) - currentState.amps[device][channel]) / static_cast<double>(repVal);
						if (ampstep > 50) {
							thrower("DDS amplitude step too large, must be <50%. Try adding more reps to ramp.");
						}
						//writeArrDeltaAmp(device, channel, indexVal, ampstep);
						currentState.amps[device][channel] = amp.evaluate(variables, variation);
					}
				}
			}
			else {
				thrower("Incorrect snapshot sequence. Must set repetitions, then frequencies, then amplitudes");
			}
		}
		else if (word == "end") {
			UINT8 indexVal = currentState.index + 1;
			//writeArrReps(indexVal, 0);
			for (UINT8 device = 0; device < 2; device++)
			{
				for (UINT8 channel = 0; channel < 4; channel++)
				{
					//writeArrDeltaFreq(device, channel, indexVal, 0);
					//writeArrDeltaAmp(device, channel, indexVal, 0);
				}
			}
		}
		else if (word == "set") {
			std::string devicenum;
			std::string channelnum;
			Expression freq;
			Expression amp;
			currentDDSScript >> devicenum;
			currentDDSScript >> channelnum;
			currentDDSScript >> freq;
			currentDDSScript >> amp;
			
			//writefreq(stoul(devicenum, nullptr), stoul(channelnum, nullptr), freq.evaluate(variables, variation));
			//writeamp(stoul(devicenum, nullptr), stoul(channelnum, nullptr), amp.evaluate(variables, variation));
		}
		else
		{
			thrower("ERROR: unrecognized DDS script command: \"" + word + "\"");
		}
		word = "";
		currentDDSScript >> word;
	}
}