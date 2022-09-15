#pragma once
#include <windows.h>
#include "DdsState.h"
#include "BoostAsyncSerial.h"

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
	BoostAsyncSerial dds;
	// the script file contents get dumped into this.
	std::string currentDDSScriptText;
	ScriptStream currentDDSScript;
	DdsState currentState;
public:
	DDS_SYNTH(std::string portID, int baudrate);
	~DDS_SYNTH();
	void connectasync(const char devSerial[]);
	void disconnect();
	void clear();
	std::vector<unsigned char> messageToVector(int message, int bytes);
	void lockPLLs(UINT m1, UINT m2);
	UINT* getFTWs(double freqs[]);
	void writeamp(UINT8 device, UINT8 channel, double freq);
	void writefreq(double freqs[]);

	void loadDDSScript(std::string scriptAddress);
	void programDDS(DDS_SYNTH* dds, std::vector<variableType>& vars, UINT variation);
};