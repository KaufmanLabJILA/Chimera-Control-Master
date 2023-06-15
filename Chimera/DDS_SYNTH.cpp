#include "stdafx.h"
#include <windows.h>
#include "DDS_SYNTH.h"
#include <sstream>

using namespace std;

DDS_SYNTH::DDS_SYNTH(std::string portID, int baudrate) : dds(portID, DDS_SAFEMODE ? -1 : baudrate) {
	//
	if (!DDS_SAFEMODE) {
		Sleep(100);
		program_default();
	}
}

DDS_SYNTH::~DDS_SYNTH() {
	//
}

void DDS_SYNTH::program_default() {
	if (!DDS_SAFEMODE) {
		clear();
		Sleep(10);
		lockPLLs(10, 10);
		Sleep(10);
		writeamps(defaultAmps);
		Sleep(10);
		writefreqs(defaultFreqs);
		Sleep(10);
		done();
	}
}

std::vector<unsigned char> DDS_SYNTH::messageToVector(int message, int bytes) {
	std::vector<unsigned char> msg(bytes);
	for (int i = 0; i < bytes; i++) {
		msg[bytes - 1 - i] = (message >> i * 8);
	}
	return msg;
}

void DDS_SYNTH::clear() {
	for (int i = 0; i < 5; i++) {
		dds.write(messageToVector(0x00, 1));
		Sleep(10);
	}

}

void DDS_SYNTH::lockPLLs(UINT m1, UINT m2) {
	UINT8 m1_bytes = (1 << 7) + (m1 << 2);
	UINT8 m2_bytes = (1 << 7) + (m1 << 2);

	dds.write(messageToVector(0xC1, 1));
	Sleep(10);
	dds.write(messageToVector(m1_bytes, 1));
	Sleep(10);
	dds.write(messageToVector(0x0000, 2));
	Sleep(10);
	dds.write(messageToVector(m2_bytes, 1));
	Sleep(10);
	dds.write(messageToVector(0x0000, 2));
	Sleep(10);
	dds.write(messageToVector(0x0000000000000000000000000000000000000000, 18));

}

//Input frequency in MHz
UINT DDS_SYNTH::getFTW(double freq) {
	UINT FTW;
	if (freq > INTERNAL_CLOCK / 2) {
		thrower("DDS frequency out of range. Must be <250MHz. ");
	}
	FTW = (UINT)round((freq * (pow(2, 32) - 1)) / (INTERNAL_CLOCK));
	return FTW;
}

//Input amp 0-100
UINT DDS_SYNTH::getATW(double amp) {
	UINT ATW;
	if (amp > 100) {
		thrower("DDS amplitude out of range, should be <100%.");
	}
	ATW = (UINT)round(amp * (pow(2, 10) - 1) / 100.0);
	return ATW;
}

void DDS_SYNTH::writefreqs(double freqs[]) {

	UINT FTWs[8];

	for (int i = 0; i < 8; i++) {
		FTWs[i] = getFTW(freqs[i]);
	}

	dds.write(messageToVector(0xC4, 1));
	Sleep(10);

	for (int i = 0; i < 8; i++) {
		UINT FTW = FTWs[i];
		UINT8 byte4 = (FTW & 0x000000ffUL);
		UINT8 byte3 = (FTW & 0x0000ff00UL) >> 8;
		UINT8 byte2 = (FTW & 0x00ff0000UL) >> 16;
		UINT8 byte1 = (FTW & 0xff000000UL) >> 24;
		dds.write(messageToVector(byte1, 1));
		Sleep(10);
		dds.write(messageToVector(byte2, 1));
		Sleep(10);
		dds.write(messageToVector(byte3, 1));
		Sleep(10);
		dds.write(messageToVector(byte4, 1));
		Sleep(10);
	}

}

void DDS_SYNTH::writeamps(double amps[]) {

	UINT ATWs[8];

	for (int i = 0; i < 8; i++) {
		ATWs[i] = getATW(amps[i]);
	}

	dds.write(messageToVector(0xC6, 1));

	for (int i = 0; i < 8; i++) {
		UINT ATW = ATWs[i];
		UINT8 byte3 = (ATW & 0x0000ffUL);
		UINT8 byte2 = 16 + ((ATW & 0x00ff00UL) >> 8);
		UINT8 byte1 = 0x00;
		/*if ((i - 1) % 3 == 0) {

		}*/
		dds.write(messageToVector(byte1, 1));
		Sleep(10);
		dds.write(messageToVector(byte2, 1));
		Sleep(10);
		dds.write(messageToVector(byte3, 1));
		Sleep(10);
	}
}

void DDS_SYNTH::done() {
	dds.write(messageToVector(0x40, 1));
	Sleep(10);
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
	clear();
	Sleep(10);
	lockPLLs(10, 10);
	Sleep(10);

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
		if (word == "snapshot") {
			Expression index, freqExp, ampExp;
			UINT8 indexVal;
			double freqs[8], amps[8];
			std::string subWord;
			currentDDSScript >> index;
			currentDDSScript >> subWord;
			indexVal = index.evaluate(variables, variation);
			if (indexVal > 1000) {
				thrower("Maximum number of DDS snapshots is 1000.");
			}
			currentState.index = indexVal;
			if (subWord == "amps") {
				for (UINT8 channel = 0; channel < 8; channel++)
				{
					currentDDSScript >> ampExp;
					amps[channel] = ampExp.evaluate(variables, variation);
					if (amps[channel] > 100) {
						thrower("DDS amplitude too large, must be <100%.");
					}
					if (amps[channel] < 0) {
						thrower("DDS amplitude step too small, must be >0%.");
					}
					currentState.amps[channel] = amps[channel];
				}
				writeamps(amps);
				Sleep(10);
			}
			else {
				thrower("Incorrect snapshot sequence. Must set amplitudes, then frequencies");
			}
			currentDDSScript >> subWord;
			if (subWord == "freqs") {
				for (UINT8 channel = 0; channel < 8; channel++)
				{
					currentDDSScript >> freqExp;
					freqs[channel] = freqExp.evaluate(variables, variation);
					if (freqs[channel] > INTERNAL_CLOCK / 2) {
						thrower("DDS frequency too large, must be <250MHz");
					}
					currentState.freqs[channel] = freqs[channel];
				}
				writefreqs(freqs);
				Sleep(10);
			}
			else {
				thrower("Incorrect snapshot sequence. Must set amplitudes, then frequencies");
			}
		}
		else if (word == "end") {

		}
		else
		{
			thrower("ERROR: unrecognized DDS script command: \"" + word + "\"");
		}
		word = "";
		currentDDSScript >> word;
	}
	done();
}