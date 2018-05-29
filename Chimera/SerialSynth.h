#pragma once
#include "constants.h"
#include "ScriptStream.h"
#include <boost/asio.hpp>
#include <stdio.h>

using namespace::boost::asio;

typedef boost::shared_ptr<boost::asio::serial_port> serial_port_ptr;

class SerialSynth {

private:
	io_service io_service_;
	serial_port_ptr port_;
	static const UINT8 freqstartoffset;
	static const UINT8 freqstopoffset;
	static const UINT8 gainoffset;
	static const UINT8 loadoffset;
	static const UINT8 moveoffset;

	// the moog script file contents get dumped into this.
	std::string currentMoogScriptText;
	ScriptStream currentMoogScript;

public:
	SerialSynth(void);
	virtual ~SerialSynth(void);
	//simple way to set many channels at once.
	auto parseFunction(std::string funcstr);
	void linLoop(std::string funcstr, UINT channels, double start, double step);
	//Attempt to parse moog script
	void loadMoogScript(std::string scriptAddress);
	void analyzeMoogScript(SerialSynth* moog, std::vector<variableType>& vars);
	virtual bool start();
	virtual void stop();
	virtual void load();
	virtual void move();

	virtual void writeOnOff(UINT8 onoff);
	virtual void writeFreqStep(UINT8 step);

	virtual void writeStartFreq(double freq, UINT channel);
	virtual void writeStopFreq(double freq, UINT channel);
	virtual void writeGain(double gain, UINT channel);
	virtual void writeLoadPhase(double phase, UINT channel);
	virtual void writeMovePhase(double phase, UINT channel);

	UINT8 getFTW(double freq);
};

