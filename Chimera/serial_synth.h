#pragma once
#include "constants.h"
#include "ScriptStream.h"
#include <boost/asio.hpp>
#include <stdio.h>

using namespace::boost::asio;

typedef boost::shared_ptr<boost::asio::serial_port> serial_port_ptr;

class serial_synth {

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
	serial_synth(void);
	virtual ~serial_synth(void);
	//simple way to set many channels at once.
	void linloop(std::function<void(double, int)> funcToLoop, unsigned channels, double start, double step);
	//Attempt to parse moog script
	void loadMoogScript(std::string scriptAddress);
	void analyzeMoogScript(serial_synth* moog, std::vector<variableType>& vars);
	virtual bool start();
	virtual void stop();
	virtual void load();
	virtual void move();

	virtual void writestartfreq(double freq, unsigned channel);
	virtual void writestopfreq(double freq, unsigned channel);
	virtual void writegain(double gain, unsigned channel);
	virtual void writeloadphase(double phase, unsigned channel);
	virtual void writemovephase(double phase, unsigned channel);
	virtual void writeonoff(unsigned onoff);
	virtual void writefreqstep(unsigned step);

	unsigned getFTW(double freq);

};

