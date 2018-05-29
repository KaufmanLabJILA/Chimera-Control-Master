#include "stdafx.h"
#include "SerialSynth.h"

using namespace::boost::asio;
using namespace::std;
using namespace::std::placeholders;

const UINT8 SerialSynth::freqstartoffset = 512;
const UINT8 SerialSynth::freqstopoffset = 1024;
const UINT8 SerialSynth::gainoffset = 1536;
const UINT8 SerialSynth::loadoffset = 2048;
const UINT8 SerialSynth::moveoffset = 2560;

SerialSynth::SerialSynth(void) {
	if (!MOOG_SAFEMODE) {
		start();
	}
}

SerialSynth::~SerialSynth(void){
	stop();
}

//Parses string and returns relevant function object.
//Remember: A non-static member function must be called with an object. That is, it always implicitly passes "this" pointer as its argument.
auto SerialSynth::parseFunction(string funcstr) {
	if (funcstr == "startfreq") {
		return std::bind(&SerialSynth::writeStartFreq, this, _1, _2);
	}
	else if (funcstr == "stopfreq") {
		return std::bind(&SerialSynth::writeStopFreq, this, _1, _2);
	}
	else if (funcstr == "gain") {
		return std::bind(&SerialSynth::writeGain, this, _1, _2);
	}
	else if (funcstr == "loadphase") {
		return std::bind(&SerialSynth::writeLoadPhase, this, _1, _2);
	}
	else if (funcstr == "movephase") {
		return std::bind(&SerialSynth::writeMovePhase, this, _1, _2);
	}
	else {
		thrower("ERROR: " + funcstr + " is not a recognized settings function that can be swept over");
	}
}

//Loop over moog channels for arbitrary settings function (input as string) with linear spacing in input parameter.
void SerialSynth::linLoop(string funcstr, UINT channels, double start, double step) {
	function<void(double, UINT8)> func = parseFunction(funcstr);
	for (UINT8 i = 0; i < channels; i++) {
		func(start + i*step, i);
	}
}

void SerialSynth::loadMoogScript(std::string scriptAddress)
{
	std::ifstream scriptFile;
	// check if file address is good.
	FILE *file;
	fopen_s(&file, cstr(scriptAddress), "r");
	if (!file)
	{
		thrower("ERROR: Moog Script File " + scriptAddress + " does not exist!");
	}
	else
	{
		fclose(file);
	}
	scriptFile.open(cstr(scriptAddress));
	// check opened correctly
	if (!scriptFile.is_open())
	{
		thrower("ERROR: Moog script file passed test making sure the file exists, but it still failed to open!");
	}
	// dump the file into the stringstream.
	std::stringstream buf(std::ios_base::app | std::ios_base::out | std::ios_base::in);
	buf << scriptFile.rdbuf();
	// This is used to more easily deal some of the analysis of the script.
	buf << "\r\n\r\n__END__";
	// for whatever reason, after loading rdbuf into a stringstream, the stream seems to not 
	// want to >> into a string. tried resetting too using seekg, but whatever, this works.
	currentMoogScript.str("");
	currentMoogScript.str(buf.str());
	currentMoogScript.clear();
	currentMoogScript.seekg(0);
	//std::string str(currentMoogScript.str());
	scriptFile.close();
}

void SerialSynth::analyzeMoogScript(SerialSynth* moog, std::vector<variableType>& vars)
{
	currentMoogScriptText = currentMoogScript.str();
	if (currentMoogScript.str() == "")
	{
		thrower("ERROR: Moog script is empty!\r\n");
	}
	std::string word;
	currentMoogScript >> word;
	std::vector<UINT> totalRepeatNum, currentRepeatNum;
	std::vector<std::streamoff> repeatPos;
	// the analysis loop.
	while (!(currentMoogScript.peek() == EOF) || word != "__end__")
	{
		if (word == "testsequence") {
			start();
			writeFreqStep(10);
			writeOnOff(0xFF000000);

			for (UINT8 i = 0; i < 32; i++) {
				writeGain(0x0FFF, i);
				writeStartFreq(90.0 + 5 * i, i);
				writeStopFreq(10.0 + 5 * i, i);
				writeLoadPhase(5 * i, i);
				writeMovePhase(5 * i, i);
			}

			load();
			move();
			stop();
			break;
		}
		else if (word == "linloop") {
			std::string funcstr, channels;
			Expression start, step;
			currentMoogScript >> funcstr;
			currentMoogScript >> channels;
			currentMoogScript >> start;
			currentMoogScript >> step;

			linLoop(funcstr, stoul(channels, nullptr), start.evaluate(), step.evaluate());
		}
		else if (word == "onoff") {
			std::string onoffstr;
			currentMoogScript >> onoffstr;
			int tmp = stoul(onoffstr, nullptr, 0);
			writeOnOff(stoul(onoffstr, nullptr, 0));
		}
		else if (word == "step") {
			std::string step;
			currentMoogScript >> step;
			writeFreqStep(stoul(step, nullptr, 0));
		}
		else if (word == "startfreq") {
			std::string channel;
			Expression startfreq;
			currentMoogScript >> channel;
			currentMoogScript >> startfreq;
			writeStartFreq(startfreq.evaluate(), stoi(channel, nullptr));
		}
		else if (word == "stopfreq") {
			std::string channel;
			Expression stopfreq;
			currentMoogScript >> channel;
			currentMoogScript >> stopfreq;
			writeStopFreq(stopfreq.evaluate(), stoi(channel, nullptr));
		}
		else if (word == "gain") {
			std::string channel;
			Expression gain;
			currentMoogScript >> channel;
			currentMoogScript >> gain;
			writeGain(gain.evaluate(), stoi(channel, nullptr));
		}
		else if (word == "loadphase") {
			std::string channel;
			Expression lphase;
			currentMoogScript >> channel;
			currentMoogScript >> lphase;
			writeLoadPhase(lphase.evaluate(), stoi(channel, nullptr));
		}
		else if (word == "movephase") {
			std::string channel;
			Expression mphase;
			currentMoogScript >> channel;
			currentMoogScript >> mphase;
			writeLoadPhase(mphase.evaluate(), stoi(channel, nullptr));
		}
		else if (word == "move") {
			move();
		}
		else if (word == "load") {
			load();
		}
		///// deal with function calls.
		//else if (word == "call")
		//{
		//	// calling a user-defined function. Get the name and the arguments to pass to the function handler.
		//	std::string functionCall, functionName, functionArgs;
		//	functionCall = currentMasterScript.getline('\r');
		//	int pos = functionCall.find_first_of("(") + 1;
		//	int finalpos2 = functionCall.find_last_of(")");
		//	int finalpos = functionCall.find_last_of(")");

		//	functionName = functionCall.substr(0, pos - 1);
		//	functionArgs = functionCall.substr(pos, finalpos - pos);
		//	std::string arg;
		//	std::vector<std::string> args;
		//	while (true)
		//	{
		//		pos = functionArgs.find_first_of(',');
		//		if (pos == std::string::npos)
		//		{
		//			arg = functionArgs.substr(0, functionArgs.size());
		//			if (arg != "")
		//			{
		//				args.push_back(arg);
		//			}
		//			break;
		//		}
		//		arg = functionArgs.substr(0, pos);
		//		args.push_back(arg);
		//		// cut oinputut that argument off the string.
		//		functionArgs = functionArgs.substr(pos + 1, functionArgs.size());
		//	}
		//	try
		//	{
		//		analyzeFunction(functionName, args, ttls, dacs, ttlShades, dacShades, rsg, vars);
		//	}
		//	catch (Error& err)
		//	{
		//		thrower(err.whatStr() + "... In Function call to function " + functionName);
		//	}
		//}
		else if (word == "repeat:")
		{
			Expression repeatStr;
			currentMoogScript >> repeatStr;
			try
			{
				totalRepeatNum.push_back(repeatStr.evaluate());
			}
			catch (Error&)
			{
				thrower("ERROR: the repeat number failed to convert to an integer! Note that the repeat number can not"
					" currently be a variable.");
			}
			repeatPos.push_back(currentMoogScript.tellg());
			currentRepeatNum.push_back(1);
		}
		// (look for end of repeat)
		else if (word == "end")
		{
			if (currentRepeatNum.size() == 0)
			{
				thrower("ERROR! Tried to end repeat, but you weren't repeating!");
			}
			if (currentRepeatNum.back() < totalRepeatNum.back())
			{
				currentMoogScript.seekg(repeatPos.back());
				currentRepeatNum.back()++;
			}
			else
			{
				currentRepeatNum.pop_back();
				repeatPos.pop_back();
				totalRepeatNum.pop_back();
			}
		}
		else
		{
			thrower("ERROR: unrecognized moog script command: \"" + word + "\"");
		}
		word = "";
		currentMoogScript >> word;
	}
}

bool SerialSynth::start(){

	// create the serial device, note it takes the io service and the port name
	port_ = serial_port_ptr(new serial_port(io_service_));
	port_->open(MOOG_COM_PORT);
	serial_port_base::baud_rate BAUD(MOOG_BAUD);
	// go through and set all the options as we need them
	port_->set_option(BAUD);
	port_->set_option(boost::asio::serial_port_base::character_size(8));
	port_->set_option(boost::asio::serial_port_base::stop_bits(boost::asio::serial_port_base::stop_bits::one));
	port_->set_option(boost::asio::serial_port_base::parity(boost::asio::serial_port_base::parity::none));
	port_->set_option(boost::asio::serial_port_base::flow_control(boost::asio::serial_port_base::flow_control::none));

	return true;
}

void SerialSynth::stop() {
	if (port_) {
		port_->cancel();
		port_->close();
		port_.reset();
	}
	io_service_.stop();
	io_service_.reset();
}

void SerialSynth::load(){
	unsigned char command[7] = {161,0,1,0,0,0,1};
	write(*port_, buffer(command, 7));
}

void SerialSynth::move(){
	unsigned char command[7] = {161,0,0,0,0,0,1};
	write(*port_, buffer(command, 7));
}

UINT8 SerialSynth::getFTW(double freq) {
	UINT8 FTW = (UINT8) round(freq * pow(2,28) / (800.0 / 3.0));
	return(FTW);
}

void SerialSynth::writeStartFreq(double freq, UINT channel) {

	if (channel > 31 || channel < 0) {
		thrower("Error: only channels 0 to 31 valid.");
	}

	UINT8 addr = freqstartoffset + channel;
	UINT8 FTW = getFTW(freq*2.0);

	cout << "FTW: " << FTW << endl;

	UINT8 addr_hi = (UINT8)floor(addr / 256.0);
	UINT8 addr_lo = addr - addr_hi * 256;

	UINT8 FTW3 = (UINT8) floor(FTW / 256 / 256 / 256);
	FTW = FTW - FTW3 * 256 * 256 * 256;
	UINT8 FTW2 = (UINT8) floor(FTW / 256 / 256);
	FTW = FTW - FTW2 * 256 * 256;
	UINT8 FTW1 = (UINT8) floor(FTW / 256);
	FTW = FTW - FTW1 * 256;
	UINT8 FTW0 = (UINT8) floor(FTW);

	cout << "hi " << addr_hi << " - lo " << addr_lo << " -- FTW " << FTW3 << FTW2 << FTW1 << FTW0 << endl;
	char command[7] = { 161, addr_hi, addr_lo, FTW3, FTW2, FTW1, FTW0 };
	write(*port_, buffer(command, 7));
}

void SerialSynth::writeStopFreq(double freq, UINT channel) {

	if (channel > 31 || channel < 0) {
		thrower("Error: only channels 0 to 31 valid.");
	}

	UINT8 addr = freqstopoffset + channel;
	UINT8 FTW = getFTW(freq*2.0);

	cout << "FTW: " << FTW << endl;

	UINT8 addr_hi = (UINT8)floor(addr / 256.0);
	UINT8 addr_lo = addr - addr_hi * 256;

	UINT8 FTW3 = (UINT8)floor(FTW / 256 / 256 / 256);
	FTW = FTW - FTW3 * 256 * 256 * 256;
	UINT8 FTW2 = (UINT8)floor(FTW / 256 / 256);
	FTW = FTW - FTW2 * 256 * 256;
	UINT8 FTW1 = (UINT8)floor(FTW / 256);
	FTW = FTW - FTW1 * 256;
	UINT8 FTW0 = (UINT8)floor(FTW);

	cout << "hi " << addr_hi << " - lo " << addr_lo << " -- FTW " << FTW3 << FTW2 << FTW1 << FTW0 << endl;
	char command[7] = { 161, addr_hi, addr_lo, FTW3, FTW2, FTW1, FTW0 };
	write(*port_, buffer(command, 7));
}

//Gain ranges from 0 to 1.
void SerialSynth::writeGain(double gainin, UINT channel) {
	UINT16 gain = round(gainin * 65535);
	if (channel > 31 || channel < 0) {
		thrower("Error: only channels 0 to 31 valid.");
	}
	if (gain > 0xFFFF) {
		thrower("Error: gain too high.");
	}
	if (gain < 0) {
		thrower("Error: gain cannot be negative. Maybe try changing phase instead?");
	}

	UINT8 addr = gainoffset + channel;

	UINT8 addr_hi = (UINT8)floor(addr / 256.0);
	UINT8 addr_lo = addr - addr_hi * 256;

	UINT8 GW3 = 0;
	UINT8 GW2 = 0;
	UINT8 GW1 = (UINT8)floor(gain / 256.0);
	gain = gain - GW1 * 256;
	UINT8 GW0 = (UINT8)floor(gain);

	cout << "hi " << addr_hi << " - lo " << addr_lo << " -- gain " << GW3 << GW2 << GW1 << GW0 << endl;
	char command[7] = { 161, addr_hi, addr_lo, GW3, GW2, GW1, GW0 };
	write(*port_, buffer(command, 7));
}

void SerialSynth::writeLoadPhase(double phase, UINT channel) {

	if (channel > 31 || channel < 0) {
		thrower("Error: only channels 0 to 31 valid.");
	}

	UINT8 addr = loadoffset + channel;

	UINT8 addr_hi = (UINT8)floor(addr / 256.0);
	UINT8 addr_lo = addr - addr_hi * 256;

	UINT8 LPW3 = 0;
	UINT8 LPW2 = 0;
	UINT8 LPW1 = (UINT8)floor(phase / 256.0);
	phase = phase - LPW1 * 256;
	UINT8 LPW0 = (UINT8)floor(phase);

	cout << "hi " << addr_hi << " - lo " << addr_lo << " -- load phase " << LPW3 << LPW2 << LPW1 << LPW0 << endl;
	char command[7] = { 161, addr_hi, addr_lo, LPW3, LPW2, LPW1, LPW0 };
	write(*port_, buffer(command, 7));
}

void SerialSynth::writeMovePhase(double phase, UINT channel) {

	if (channel > 31 || channel < 0) {
		thrower("Error: only channels 0 to 31 valid.");
	}

	UINT8 addr = moveoffset + channel;

	UINT8 addr_hi = (UINT8)floor(addr / 256.0);
	UINT8 addr_lo = addr - addr_hi * 256;

	UINT8 MPW3 = 0;
	UINT8 MPW2 = 0;
	UINT8 MPW1 = (UINT8)floor(phase / 256.0);
	phase = phase - MPW1 * 256;
	UINT8 MPW0 = (UINT8)floor(phase);

	cout << "hi " << addr_hi << " - lo " << addr_lo << " -- load phase " << MPW3 << MPW2 << MPW1 << MPW0 << endl;
	char command[7] = { 161, addr_hi, addr_lo, MPW3, MPW2, MPW1, MPW0 };
	write(*port_, buffer(command, 7));
}

void SerialSynth::writeOnOff(UINT8 onoff) {

	if (onoff > 0xFFFFFFFF || onoff < 0) {
		thrower("Error: on off tuning word wrong size.");
	}

	UINT8 onoff3 = (UINT8) floor(onoff / 256 / 256 / 256);
	onoff = onoff - onoff3 * 256 * 256 * 256;
	UINT8 onoff2 = (UINT8) floor(onoff / 256 / 256);
	onoff = onoff - onoff2 * 256 * 256;
	UINT8 onoff1 = (UINT8) floor(onoff / 256);
	onoff = onoff - onoff1 * 256;
	UINT8 onoff0 = (UINT8) floor(onoff);

	//Note that the onoff register is stored at address 3 and is 32 bits long.
	cout << "hi " << 0 << " - lo " << 3 << " -- onoff " << onoff3 << onoff2 << onoff1 << onoff0 << endl;
	char command[7] = { 161, 0, 3, onoff3, onoff2, onoff1, onoff0 };
	write(*port_, buffer(command, 7));
}

void SerialSynth::writeFreqStep(UINT8 step) {
	//1 LSB is ~8MHz per sec(800 / 96 MHz per sec), register is 10 bits

	if (step > 0b1111111111 || step < 0) {
		thrower("Error: step tuning word wrong size.");
	}

	UINT8 step3 = (UINT8)floor(step / 256 / 256 / 256);
	step = step - step3 * 256 * 256 * 256;
	UINT8 step2 = (UINT8)floor(step / 256 / 256);
	step = step - step2 * 256 * 256;
	UINT8 step1 = (UINT8)floor(step / 256);
	step = step - step1 * 256;
	UINT8 step0 = (UINT8)floor(step);

	cout << "hi " << 0 << " - lo " << 3 << " -- step " << step3 << step2 << step1 << step0 << endl;
	char command[7] = { 161, 0, 2, step3, step2, step1, step0 };
	write(*port_, buffer(command, 7));
}