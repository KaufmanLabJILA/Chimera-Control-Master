#include "stdafx.h"
#include "serial_synth.h"

using namespace::boost::asio;
using namespace::std;

const unsigned serial_synth::freqstartoffset = 512;
const unsigned serial_synth::freqstopoffset = 1024;
const unsigned serial_synth::gainoffset = 1536;
const unsigned serial_synth::loadoffset = 2048;
const unsigned serial_synth::moveoffset = 2560;

serial_synth::serial_synth(void) {
}

serial_synth::~serial_synth(void){
	stop();
}

bool serial_synth::start(const char *PORT, serial_port_base::baud_rate BAUD){

	// create the serial device, note it takes the io service and the port name
	port_ = serial_port_ptr(new serial_port(io_service_));
	port_->open(PORT);

	// go through and set all the options as we need them
	port_->set_option(BAUD);
	port_->set_option(boost::asio::serial_port_base::character_size(8));
	port_->set_option(boost::asio::serial_port_base::stop_bits(boost::asio::serial_port_base::stop_bits::one));
	port_->set_option(boost::asio::serial_port_base::parity(boost::asio::serial_port_base::parity::none));
	port_->set_option(boost::asio::serial_port_base::flow_control(boost::asio::serial_port_base::flow_control::none));

	return true;
}

void serial_synth::stop() {
	if (port_) {
		port_->cancel();
		port_->close();
		port_.reset();
	}
	io_service_.stop();
	io_service_.reset();
}

void serial_synth::load(){
	unsigned char command[7] = {161,0,1,0,0,0,1};
	write(*port_, buffer(command, 7));
}

void serial_synth::move(){
	unsigned char command[7] = {161,0,0,0,0,0,1};
	write(*port_, buffer(command, 7));
}

unsigned serial_synth::getFTW(float freq) {
	unsigned FTW = (unsigned) round(freq * pow(2,28) / (800.0 / 3.0));
	return(FTW);
}

void serial_synth::writestartfreq(float freq, unsigned channel) {

	if (channel > 31 || channel < 0) {
		throw string("Error: only channels 0 to 31 valid.");
	}

	unsigned addr = freqstartoffset + channel;
	unsigned FTW = getFTW(freq*2.0);

	cout << "FTW: " << FTW << endl;

	unsigned addr_hi = (unsigned)floor(addr / 256.0);
	unsigned addr_lo = addr - addr_hi * 256;

	unsigned FTW3 = (unsigned) floor(FTW / 256 / 256 / 256);
	FTW = FTW - FTW3 * 256 * 256 * 256;
	unsigned FTW2 = (unsigned) floor(FTW / 256 / 256);
	FTW = FTW - FTW2 * 256 * 256;
	unsigned FTW1 = (unsigned) floor(FTW / 256);
	FTW = FTW - FTW1 * 256;
	unsigned FTW0 = (unsigned) floor(FTW);

	cout << "hi " << addr_hi << " - lo " << addr_lo << " -- FTW " << FTW3 << FTW2 << FTW1 << FTW0 << endl;
	char command[7] = { 161, addr_hi, addr_lo, FTW3, FTW2, FTW1, FTW0 };
	write(*port_, buffer(command, 7));
}

void serial_synth::writestopfreq(float freq, unsigned channel) {

	if (channel > 31 || channel < 0) {
		throw string("Error: only channels 0 to 31 valid.");
	}

	unsigned addr = freqstopoffset + channel;
	unsigned FTW = getFTW(freq*2.0);

	cout << "FTW: " << FTW << endl;

	unsigned addr_hi = (unsigned)floor(addr / 256.0);
	unsigned addr_lo = addr - addr_hi * 256;

	unsigned FTW3 = (unsigned)floor(FTW / 256 / 256 / 256);
	FTW = FTW - FTW3 * 256 * 256 * 256;
	unsigned FTW2 = (unsigned)floor(FTW / 256 / 256);
	FTW = FTW - FTW2 * 256 * 256;
	unsigned FTW1 = (unsigned)floor(FTW / 256);
	FTW = FTW - FTW1 * 256;
	unsigned FTW0 = (unsigned)floor(FTW);

	cout << "hi " << addr_hi << " - lo " << addr_lo << " -- FTW " << FTW3 << FTW2 << FTW1 << FTW0 << endl;
	char command[7] = { 161, addr_hi, addr_lo, FTW3, FTW2, FTW1, FTW0 };
	write(*port_, buffer(command, 7));
}

void serial_synth::writegain(float gain, unsigned channel) {

	if (channel > 31 || channel < 0) {
		throw string("Error: only channels 0 to 31 valid.");
	}

	if (gain > 0xFFFF) {
		throw string("Error: gain too high.");
	}

	if (gain < 0) {
		throw string("Error: gain cannot be negative. Maybe try changing phase instead?");
	}

	unsigned addr = gainoffset + channel;

	unsigned addr_hi = (unsigned)floor(addr / 256.0);
	unsigned addr_lo = addr - addr_hi * 256;

	unsigned GW3 = 0;
	unsigned GW2 = 0;
	unsigned GW1 = (unsigned)floor(gain / 256.0);
	gain = gain - GW1 * 256;
	unsigned GW0 = (unsigned)floor(gain);

	cout << "hi " << addr_hi << " - lo " << addr_lo << " -- gain " << GW3 << GW2 << GW1 << GW0 << endl;
	char command[7] = { 161, addr_hi, addr_lo, GW3, GW2, GW1, GW0 };
	write(*port_, buffer(command, 7));
}

void serial_synth::writeloadphase(float phase, unsigned channel) {

	if (channel > 31 || channel < 0) {
		throw string("Error: only channels 0 to 31 valid.");
	}

	unsigned addr = loadoffset + channel;

	unsigned addr_hi = (unsigned)floor(addr / 256.0);
	unsigned addr_lo = addr - addr_hi * 256;

	unsigned LPW3 = 0;
	unsigned LPW2 = 0;
	unsigned LPW1 = (unsigned)floor(phase / 256.0);
	phase = phase - LPW1 * 256;
	unsigned LPW0 = (unsigned)floor(phase);

	cout << "hi " << addr_hi << " - lo " << addr_lo << " -- load phase " << LPW3 << LPW2 << LPW1 << LPW0 << endl;
	char command[7] = { 161, addr_hi, addr_lo, LPW3, LPW2, LPW1, LPW0 };
	write(*port_, buffer(command, 7));
}

void serial_synth::writemovephase(float phase, unsigned channel) {

	if (channel > 31 || channel < 0) {
		throw string("Error: only channels 0 to 31 valid.");
	}

	unsigned addr = moveoffset + channel;

	unsigned addr_hi = (unsigned)floor(addr / 256.0);
	unsigned addr_lo = addr - addr_hi * 256;

	unsigned MPW3 = 0;
	unsigned MPW2 = 0;
	unsigned MPW1 = (unsigned)floor(phase / 256.0);
	phase = phase - MPW1 * 256;
	unsigned MPW0 = (unsigned)floor(phase);

	cout << "hi " << addr_hi << " - lo " << addr_lo << " -- load phase " << MPW3 << MPW2 << MPW1 << MPW0 << endl;
	char command[7] = { 161, addr_hi, addr_lo, MPW3, MPW2, MPW1, MPW0 };
	write(*port_, buffer(command, 7));
}

void serial_synth::writeonoff(unsigned onoff) {

	if (onoff > 0xFFFFFFFF || onoff < 0) {
		throw string("Error: on off tuning word wrong size.");
	}

	unsigned onoff3 = (unsigned) floor(onoff / 256 / 256 / 256);
	onoff = onoff - onoff3 * 256 * 256 * 256;
	unsigned onoff2 = (unsigned) floor(onoff / 256 / 256);
	onoff = onoff - onoff2 * 256 * 256;
	unsigned onoff1 = (unsigned) floor(onoff / 256);
	onoff = onoff - onoff1 * 256;
	unsigned onoff0 = (unsigned) floor(onoff);

	//Note that the onoff register is stored at address 3 and is 32 bits long.
	cout << "hi " << 0 << " - lo " << 3 << " -- onoff " << onoff3 << onoff2 << onoff1 << onoff0 << endl;
	char command[7] = { 161, 0, 3, onoff3, onoff2, onoff1, onoff0 };
	write(*port_, buffer(command, 7));
}

void serial_synth::writefreqstep(unsigned step) {
	//1 LSB is ~8MHz per sec(800 / 96 MHz per sec), register is 10 bits

	if (step > 0b1111111111 || step < 0) {
		throw string("Error: step tuning word wrong size.");
	}

	unsigned step3 = (unsigned)floor(step / 256 / 256 / 256);
	step = step - step3 * 256 * 256 * 256;
	unsigned step2 = (unsigned)floor(step / 256 / 256);
	step = step - step2 * 256 * 256;
	unsigned step1 = (unsigned)floor(step / 256);
	step = step - step1 * 256;
	unsigned step0 = (unsigned)floor(step);

	cout << "hi " << 0 << " - lo " << 3 << " -- step " << step3 << step2 << step1 << step0 << endl;
	char command[7] = { 161, 0, 2, step3, step2, step1, step0 };
	write(*port_, buffer(command, 7));
}