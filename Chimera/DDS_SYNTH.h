#pragma once
#include <windows.h>
#include "ftd2xx.h"

#define MSGLENGTH         7
#define WBWRITE           (unsigned char)161
#define WBWRITE_ARRAY     (unsigned char)2 //Add 2 to WBWRITE
#define WRITESPERDATAPT   3
#define BUFFERSIZESER     100
#define BUFFERSIZEASYNC   2048
#define INTERNAL_CLOCK    (double)500.0 //Internal clock in MHz

#define NONE            0
#define SERIAL          1
#define ASYNC           2

//Array begins with FTW, then increments by step size, and number of repetitions

class DDS_SYNTH
{
private:
	HANDLE m_hSerialComm;
	FT_HANDLE ftHandle;
	int connType;
	// the script file contents get dumped into this.
	std::string currentDDSScriptText;
	ScriptStream currentDDSScript;
public:
	DDS_SYNTH(const char devSerial[]);
	~DDS_SYNTH();
	void connectasync(const char devSerial[]);
	void disconnect();
	void write(UINT8 device, UINT16 address, UINT8 dat1, UINT8 dat2, UINT8 dat3, UINT8 dat4);
	void longupdate();
	void update();
	void lockPLLs();
	INT getFTW(double freq);
	UINT getATW(double amp);
	INT get32bitATW(double amp);
	void channelSelect(UINT8 device, UINT8 channel);
	void writefreq(UINT8 device, UINT8 channel, double freq);
	void writeamp(UINT8 device, UINT8 channel, double freq);

	void writeArrResetFreq(UINT8 device, UINT8 channel, double freq);
	void writeArrResetAmp(UINT8 device, UINT8 channel, double amp);
	void writeArrReps(UINT8 index, UINT16 reps);
	void writeArrDeltaFreq(UINT8 device, UINT8 channel, UINT8 index, double deltafreq);
	void writeArrDeltaAmp(UINT8 device, UINT8 channel, UINT8 index, double deltaamp);

	void loadDDSScript(std::string scriptAddress);
	void programDDS(DDS_SYNTH* dds, std::vector<variableType>& vars, UINT variation);
};