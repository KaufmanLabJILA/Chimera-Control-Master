#pragma once
#include <boost/asio.hpp>

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

public:
	serial_synth(void);
	virtual ~serial_synth(void);
	virtual bool start(const char *PORT, serial_port_base::baud_rate BAUD);
	virtual void stop();
	virtual void load();
	virtual void move();

	virtual void writestartfreq(float freq, unsigned channel);
	virtual void writestopfreq(float freq, unsigned channel);
	virtual void writegain(float gain, unsigned channel);
	virtual void writeloadphase(float phase, unsigned channel);
	virtual void writemovephase(float phase, unsigned channel);
	virtual void writeonoff(unsigned onoff);
	virtual void writefreqstep(unsigned step);

	unsigned getFTW(float freq);

};

