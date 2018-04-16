#pragma once
#include <windows.h>
#include "ftd2xx.h"

#define MSGLENGTH         7
#define WBWRITE           (unsigned char)161
#define WRITESPERDATAPT   3
#define BUFFERSIZESER     100
#define BUFFERSIZEASYNC   2048
#define INTERNAL_CLOCK    (double)500.0 //Internal clock in MHz

#define NONE            0
#define SERIAL          1
#define ASYNC           2

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
	void write(char device, char address, char dat1, char dat2, char dat3, char dat4);
	void longupdate();
	void update();
	void lockPLLs();
	unsigned getFTW(double freq);
	unsigned getATW(double amp);
	void writefreq(char device, char channel, double freq);
	void writeamp(char device, char channel, double freq);
	void loadDDSScript(std::string scriptAddress);
	void programDDS(DDS_SYNTH* dds, std::vector<variableType>& vars, UINT variation);
};