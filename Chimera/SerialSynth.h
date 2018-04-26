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
	static const unsigned freqstartoffset;
	static const unsigned freqstopoffset;
	static const unsigned gainoffset;
	static const unsigned loadoffset;
	static const unsigned moveoffset;

	// the moog script file contents get dumped into this.
	std::string currentMoogScriptText;
	ScriptStream currentMoogScript;

public:
	SerialSynth(void);
	virtual ~SerialSynth(void);
	//simple way to set many channels at once.
	auto parseFunction(std::string funcstr);
	void linLoop(std::string funcstr, unsigned channels, double start, double step);
	//Attempt to parse moog script
	void loadMoogScript(std::string scriptAddress);
	void analyzeMoogScript(SerialSynth* moog, std::vector<variableType>& vars);
	virtual bool start();
	virtual void stop();
	virtual void load();
	virtual void move();

	virtual void writeOnOff(unsigned onoff);
	virtual void writeFreqStep(unsigned step);

	virtual void writeStartFreq(double freq, unsigned channel);
	virtual void writeStopFreq(double freq, unsigned channel);
	virtual void writeGain(double gain, unsigned channel);
	virtual void writeLoadPhase(double phase, unsigned channel);
	virtual void writeMovePhase(double phase, unsigned channel);

	unsigned getFTW(double freq);
};

