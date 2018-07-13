#include "stdafx.h"
#include <windows.h>
#include "DDS_SYNTH.h"
#include <sstream>

using namespace std;

DDS_SYNTH::DDS_SYNTH(const char devSerial[]) {
	if (!DDS_SAFEMODE) {
		this->connType = NONE;
		connectasync(devSerial);
		lockPLLs();
	}
}

DDS_SYNTH::~DDS_SYNTH() {
	disconnect();
}

void DDS_SYNTH::connectasync(const char devSerial[])
{
	FT_STATUS ftStatus;
	DWORD numDevs;
	ftStatus = FT_ListDevices(&numDevs, NULL, FT_LIST_NUMBER_ONLY);
	if (numDevs > 0)
	{
		ftStatus = FT_OpenEx((PVOID)devSerial, FT_OPEN_BY_SERIAL_NUMBER, &this->ftHandle);
		if (ftStatus != FT_OK) {
			thrower("Error opening device");
		}
		ftStatus = FT_SetUSBParameters(this->ftHandle, 65536, 65536);
		if (ftStatus != FT_OK) {
			thrower("Error opening device");
		}
		this->connType = ASYNC;
	}
	else
	{
		thrower("No devices found.");
	}
}

void DDS_SYNTH::disconnect()
{
	if (!DDS_SAFEMODE) {
		if (this->connType == SERIAL)
		{
			if (this->m_hSerialComm != INVALID_HANDLE_VALUE)
			{
				CloseHandle(this->m_hSerialComm);
				m_hSerialComm = INVALID_HANDLE_VALUE;
				thrower("Serial connection closed...");
				this->connType = NONE;
			}
		}
		else if (this->connType == ASYNC)
		{
			FT_STATUS ftStatus;
			ftStatus = FT_Close(this->ftHandle);
			if (ftStatus == FT_OK) {
				// FT_Write OK
			}
			else {
				// FT_Write Failed
				thrower("Error closing async connection");
			}
		}
		else
		{
			thrower("No connection to close...");
		}
	}
	
}

void DDS_SYNTH::write(UINT8 DEVICE, UINT16 ADDRESS, UINT8 dat1, UINT8 dat2, UINT8 dat3, UINT8 dat4)
{
	if (this->connType == ASYNC)
	{
		FT_STATUS ftStatus;
		DWORD BytesWritten;
		UINT bytesToWrite = 0;
		bytesToWrite += MSGLENGTH;

		if (DEVICE > 255 || ADDRESS > 65535 || dat1 > 255 || dat2 > 255 || dat3 > 255 || dat4 > 255) {
			thrower("DDS write out of range.");
		}

		UINT8 ADDRESS_LO = ADDRESS & 0x00ffUL;
		UINT8 ADDRESS_HI = (ADDRESS & 0xff00UL) >> 8;

		unsigned char input[7] = { WBWRITE + DEVICE, ADDRESS_HI, ADDRESS_LO, dat1, dat2, dat3, dat4 };

		ftStatus = FT_Write(this->ftHandle, input, bytesToWrite, &BytesWritten);
		if (ftStatus == FT_OK) {
			// FT_Write OK
		}
		else {
			// FT_Write Failed
			thrower("error writing");
		}
	}
	else {
		thrower("Incorrect connection type, should be ASYNC");
	}
	
}

void DDS_SYNTH::longupdate() {
	write(0, 29, 0, 0, 0, 1);
	write(1, 29, 0, 0, 0, 1);
}

void DDS_SYNTH::update() {
	write(0, 25, 0, 0, 0, 1);
	write(1, 25, 0, 0, 0, 1);
}

void DDS_SYNTH::lockPLLs() {
	write(0, 1, 0, 0b10101000, 0, 0);
	write(1, 1, 0, 0b10101000, 0, 0);
	longupdate();
	Sleep(100); //This delay is critical, need to give the PLL time to lock.
}

//Input frequency in MHz
INT DDS_SYNTH::getFTW(double freq) {//Negative ints, Nyquist freq, works out.
	if (freq > INTERNAL_CLOCK/2) {
		thrower("DDS frequency out of range");
	}
	INT FTW = (INT)round(freq * pow(2, 32) / (INTERNAL_CLOCK));
	return(FTW);
}

void DDS_SYNTH::channelSelect(UINT8 device, UINT8 channel) {
	UINT8 CW = 0;
	CW |= 1 << (channel + 4);
	write(device, 0, 0, 0, 0, CW);
}

//Devices numbered 0-1, channels numbered 0-3
void DDS_SYNTH::writefreq(UINT8 device, UINT8 channel, double freq) {
	
	UINT FTW = getFTW(freq);

	UINT8 byte4 = FTW & 0x000000ffUL;
	UINT8 byte3 = (FTW & 0x0000ff00UL) >> 8;
	UINT8 byte2 = (FTW & 0x00ff0000UL) >> 16;
	UINT8 byte1 = (FTW & 0xff000000UL) >> 24;

	write(device, 4, 0, 0, 0, 0);//TODO: Fix this. Currently sets FTW to zero and then back to force a rewrite, solving issue with inactive channels.
	write(device, 4, byte1, byte2, byte3, byte4);
}

//Amplitude in %, note that this scale is non-linear, and ranges from ~-13dBm to -7dBm
UINT DDS_SYNTH::getATW(double amp) {
	if (amp > 100) {
		thrower("DDS amplitude out of range, should be <100%.");
	}
	UINT ATW = (UINT)round(amp * (pow(2,10)-1) / 100.0);
	return(ATW);
}

INT DDS_SYNTH::get32bitATW(double amp) {//SIGNED
	if (amp > 100) {
		thrower("DDS amplitude out of range, should be <100%.");
	}
	INT ATW = (INT)round(amp * (pow(2, 32) - 1) / 100.0);
	return(ATW);
}

void DDS_SYNTH::writeamp(UINT8 device, UINT8 channel, double amp) {

	UINT ATW = getATW(amp);

	UINT8 byte2 = ATW & 0x000000ffUL;
	UINT8 byte1 = ATW >> 8;

	byte1 |= 1 << 4; //Necessary to turn on amplitude multiplier.

	write(device, 6, 0, 0, 0, byte2); //TODO: Fix this. Currently sets Mult to off and then back to force a rewrite, solving issue with inactive channels.
	write(device, 6, 0, 0, byte1, byte2);
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

void DDS_SYNTH::writeArrResetFreq(UINT8 device, UINT8 channel, double freq) {

	UINT16 address = 4 * device + 3 - channel;

	UINT FTW = getFTW(freq);

	UINT8 byte4 = FTW & 0x000000ffUL;
	UINT8 byte3 = (FTW & 0x0000ff00UL) >> 8;
	UINT8 byte2 = (FTW & 0x00ff0000UL) >> 16;
	UINT8 byte1 = (FTW & 0xff000000UL) >> 24;

	write(WBWRITE_ARRAY, address, byte1, byte2, byte3, byte4);
}

void DDS_SYNTH::writeArrResetAmp(UINT8 device, UINT8 channel, double amp) {

	UINT16 address = 0x100 + 4 * device + 3 - channel;

	UINT32 ATW = getATW(amp);
	ATW = ATW << 22;

	UINT8 byte4 = ATW & 0x000000ffUL;
	UINT8 byte3 = (ATW & 0x0000ff00UL) >> 8;
	UINT8 byte2 = (ATW & 0x00ff0000UL) >> 16;
	UINT8 byte1 = (ATW & 0xff000000UL) >> 24;

	write(WBWRITE_ARRAY, address, byte1, byte2, byte3, byte4);
}

void DDS_SYNTH::writeArrReps(UINT8 index, UINT16 reps) {

	UINT16 address = 0x200 + index;

	UINT8 byte4 = reps & 0x000000ffUL;
	UINT8 byte3 = (reps & 0x0000ff00UL) >> 8;
	UINT8 byte2 = 0;
	UINT8 byte1 = 0;

	write(WBWRITE_ARRAY, address, byte1, byte2, byte3, byte4);
}

void DDS_SYNTH::writeArrDeltaFreq(UINT8 device, UINT8 channel, UINT8 index, double deltafreq) {

	UINT16 address = 0x400 + 0x200 * (4 * device + 3 - channel) + index;

	UINT FTW = getFTW(deltafreq);

	UINT8 byte4 = FTW & 0x000000ffUL;
	UINT8 byte3 = (FTW & 0x0000ff00UL) >> 8;
	UINT8 byte2 = (FTW & 0x00ff0000UL) >> 16;
	UINT8 byte1 = (FTW & 0xff000000UL) >> 24;

	write(WBWRITE_ARRAY, address, byte1, byte2, byte3, byte4);
}

void DDS_SYNTH::writeArrDeltaAmp(UINT8 device, UINT8 channel, UINT8 index, double deltaamp) {

	UINT16 address = 0x1400 + 0x200 * (4 * device + 3 - channel) + index;

	UINT ATW = get32bitATW(deltaamp);

	UINT8 byte4 = ATW & 0x000000ffUL;
	UINT8 byte3 = (ATW & 0x0000ff00UL) >> 8;
	UINT8 byte2 = (ATW & 0x00ff0000UL) >> 16;
	UINT8 byte1 = (ATW & 0xff000000UL) >> 24;

	write(WBWRITE_ARRAY, address, byte1, byte2, byte3, byte4);
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

				write(device, 0, 0, 0, 0, 0xF0); //select all channels on device.
				UINT8 byte1 = 1 << 4; //Necessary to turn on amplitude multiplier.
				write(device, 6, 0, 0, 0, 0); //TODO: Fix this. Currently sets Mult to off and then back to force a rewrite, solving issue with inactive channels.
				write(device, 6, 0, 0, byte1, 0);
			}
			longupdate();

			if (subWord == "freqs") {
				for (UINT8 device = 0; device < 2; device++)
				{
					for (UINT8 channel = 0; channel < 4; channel++)
					{
						currentDDSScript >> freq;
						writeArrResetFreq(device, channel, freq.evaluate(variables, variation));
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
						writeArrResetAmp(device, channel, amp.evaluate(variables, variation));
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
			currentState.index = indexVal;
			if (subWord == "reps") {
				currentDDSScript >> rep;
				repVal = rep.evaluate(variables, variation);
				writeArrReps(indexVal, repVal);
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
						writeArrDeltaFreq(device, channel, indexVal, freqstep);
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
						writeArrDeltaAmp(device, channel, indexVal, ampstep);
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
			writeArrReps(indexVal, 0);
			for (UINT8 device = 0; device < 2; device++)
			{
				for (UINT8 channel = 0; channel < 4; channel++)
				{
					writeArrDeltaFreq(device, channel, indexVal, 0);
					writeArrDeltaAmp(device, channel, indexVal, 0);
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
			
			channelSelect(stoul(devicenum, nullptr), stoul(channelnum, nullptr));
			writefreq(stoul(devicenum, nullptr), stoul(channelnum, nullptr), freq.evaluate(variables, variation));
			writeamp(stoul(devicenum, nullptr), stoul(channelnum, nullptr), amp.evaluate(variables, variation));
		}
		else
		{
			thrower("ERROR: unrecognized DDS script command: \"" + word + "\"");
		}
		word = "";
		currentDDSScript >> word;
	}
	longupdate();
}