#pragma once
#include "stdafx.h"
#include <iostream>

#include "constants.h"
#include "ScriptStream.h"

#include <thread>
#include <chrono>

#include "BoostAsyncSerial.h"
#include <windows.h>

#include "MessagePrinter.h"
#include "MessageSender.h"

class gigaMoog {

public:
	gigaMoog(std::string portID, int baudrate);
	virtual ~gigaMoog(void);

	//Attempt to parse moog script
	void loadMoogScript(std::string scriptAddress);
	void analyzeMoogScript(gigaMoog* moog, std::vector<variableType>& variables, UINT variation);

	virtual void writeOff(MessageSender& ms);
	void send(MessageSender& ms);

private:
	BoostAsyncSerial fpga;

	// the moog script file contents get dumped into this.
	std::string currentMoogScriptText;
	ScriptStream currentMoogScript;

};

