#include "stdafx.h"
#include "GigaMoog.h"

//using namespace::boost::asio;
//using namespace::std;
//using namespace::std::placeholders;

//const UINT gigaMoog::freqstartoffset = 512;
//const UINT gigaMoog::freqstopoffset = 1024;
//const UINT gigaMoog::gainoffset = 1536;
//const UINT gigaMoog::loadoffset = 2048;
//const UINT gigaMoog::moveoffset = 2560;

gigaMoog::gigaMoog(std::string portID, int baudrate) : fpga(portID, baudrate) {
	if (GIGAMOOG_SAFEMODE) {
		//writeOff();
	}
}

gigaMoog::~gigaMoog(void){
}

void gigaMoog::loadMoogScript(std::string scriptAddress)
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

void gigaMoog::analyzeMoogScript(gigaMoog* moog, std::vector<variableType>& vars)
{
	writeOff();

	MessageSender ms;

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
		if (word == "set") {
			std::string DAC;
			Expression channel, amplitude, frequency, phase;
			currentMoogScript >> DAC;
			currentMoogScript >> channel;
			currentMoogScript >> amplitude;
			currentMoogScript >> frequency;
			currentMoogScript >> phase;



			MessageDAC dacset;
			if (DAC == "dac0") {
				dacset = MessageDAC::DAC0;
			}
			else if (DAC == "dac1") {
				dacset = MessageDAC::DAC1;
			}
			else if (DAC == "dac2") {
				dacset = MessageDAC::DAC2;
			}
			else if (DAC == "dac3") {
				dacset = MessageDAC::DAC3;
			}
			else {
				thrower("ERROR: unrecognized moog DAC selection: \"" + DAC + "\"");
			}

			Message m = Message::make().destination(MessageDestination::KA007)
				.DAC(dacset).channel(channel.evaluate())
				.setting(MessageSetting::LOADFREQUENCY)
				.frequencyMHz(frequency.evaluate()).amplitudePercent(amplitude.evaluate()).phaseDegrees(phase.evaluate());;
			ms.enqueue(m);
		}
		else
		{
			thrower("ERROR: unrecognized moog script command: \"" + word + "\"");
		}
		word = "";
		currentMoogScript >> word;
	}

	{
		Message m = Message::make().destination(MessageDestination::KA007)
			.setting(MessageSetting::TERMINATE_SEQ);
		ms.enqueue(m);
	}

	ms.getQueueElementCount();
	//MessagePrinter rec;
	//fpga.setReadCallback(boost::bind(&MessagePrinter::callback, rec, _1));
	fpga.write(ms.getMessageBytes());
}

void gigaMoog::writeOff() {

	MessageSender ms;

	for (int channel = 0; channel < 64; channel++) {
		Message m = Message::make().destination(MessageDestination::KA007)
			.DAC(MessageDAC::DAC0).channel(channel)
			.setting(MessageSetting::LOADFREQUENCY)
			.frequencyMHz(0).amplitudePercent(0).phaseDegrees(0.0);;
		ms.enqueue(m);
	}

	for (int channel = 0; channel < 64; channel++) {
		Message m = Message::make().destination(MessageDestination::KA007)
			.DAC(MessageDAC::DAC1).channel(channel)
			.setting(MessageSetting::LOADFREQUENCY)
			.frequencyMHz(0).amplitudePercent(0).phaseDegrees(0.0);;
		ms.enqueue(m);
	}

	for (int channel = 0; channel < 64; channel++) {
		Message m = Message::make().destination(MessageDestination::KA007)
			.DAC(MessageDAC::DAC2).channel(channel)
			.setting(MessageSetting::LOADFREQUENCY)
			.frequencyMHz(0).amplitudePercent(0).phaseDegrees(0.0);;
		ms.enqueue(m);
	}

	for (int channel = 0; channel < 64; channel++) {
		Message m = Message::make().destination(MessageDestination::KA007)
			.DAC(MessageDAC::DAC3).channel(channel)
			.setting(MessageSetting::LOADFREQUENCY)
			.frequencyMHz(0).amplitudePercent(0).phaseDegrees(0.0);;
		ms.enqueue(m);
	}
	
	{
		Message m = Message::make().destination(MessageDestination::KA007)
			.setting(MessageSetting::TERMINATE_SEQ);
		ms.enqueue(m);
	}

	ms.getQueueElementCount();
	//MessagePrinter rec;
	//fpga.setReadCallback(boost::bind(&MessagePrinter::callback, rec, _1));
	fpga.write(ms.getMessageBytes());

}