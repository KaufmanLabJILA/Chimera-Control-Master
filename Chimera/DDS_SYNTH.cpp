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

void DDS_SYNTH::write(UINT8 DEVICE, UINT8 ADDRESS, UINT8 dat1, UINT8 dat2, UINT8 dat3, UINT8 dat4)
{
	if (this->connType == ASYNC)
	{
		FT_STATUS ftStatus;
		DWORD BytesWritten;
		UINT bytesToWrite = 0;
		bytesToWrite += MSGLENGTH;

		if (DEVICE > 255 || ADDRESS > 255 || dat1 > 255 || dat2 > 255 || dat3 > 255 || dat4 > 255) {
			thrower("DDS write out of range.");
		}

		unsigned char input[7] = { WBWRITE + DEVICE, 0, ADDRESS, dat1, dat2, dat3, dat4 };

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
UINT DDS_SYNTH::getFTW(double freq) {
	if (freq > INTERNAL_CLOCK) {
		thrower("DDS frequency out of range");
	}
	UINT FTW = (UINT)round(freq * pow(2, 32) / (INTERNAL_CLOCK));
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

void DDS_SYNTH::writeamp(UINT8 device, UINT8 channel, double amp) {

	UINT ATW = getATW(amp);

	UINT8 byte2 = ATW & 0x000000ffUL;
	UINT8 byte1 = ATW >> 8;

	byte1 |= 1 << 4; //Necessary to turn on amplitude multiplier.

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
		if (word == "set") {
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