#include "stdafx.h"
#include "fpgaAWG.h"
#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <sstream>
#include <cmath>

fpgaAWG::fpgaAWG(std::string portID, int baudrate, bool AWG_SAFEMODE) : fpga(portID, AWG_SAFEMODE ? -1 : baudrate) {
	//using ternary operators to change which fpga constructor is called.
}

fpgaAWG::~fpgaAWG(void) {
	//Exit
}

std::vector<unsigned char> fpgaAWG::messageToVector(int message, int bytes) {
	std::vector<unsigned char> msg(bytes);
	for (int i = 0; i < bytes; i++) {
		msg[bytes - 1 - i] = (message >> i * 8);
	}
	return msg;
}

void fpgaAWG::trigger() {
	fpga.write(messageToVector(0xA200, 2));	//the uart buffer is only 16 bytes deep, so we need to wait until all data is written after every word
	//fpga.flush();
}

void fpgaAWG::reset() {
	std::vector<unsigned char> msg(2);
	fpga.write(messageToVector(0xA300, 2));
}

void fpgaAWG::writeTimestamp(unsigned char channel, unsigned short address, float timeStampMicro, bool phase_update, float phaseDegrees, float ampPercent, float freqMHz) {
	//std::cout << "  New timestamp";
	unsigned short int phase = getPTW(phaseDegrees);
	if (phase_update) {
		phase += 0x1000; //set update bit
	};

	unsigned long int timestamp = getTTW(timeStampMicro);
	unsigned short int atw = getATW(ampPercent);
	unsigned long int ftw = getFTW(freqMHz);

	fpga.write(messageToVector(0xA1, 1));
	//std::cout << "\nfifo trans";
	//std::cout << std::hex << 0xA1;

	fpga.write(messageToVector(channel, 1));
	//std::cout << "\nchannel";
	//std::cout << std::hex << channel;

	fpga.write(messageToVector(address, 2));
	//std::cout << "\naddress";
	//std::cout << std::hex << address;

	fpga.write(messageToVector(timestamp, 4));
	//std::cout << "\ntimestamp";
	//std::cout << std::hex << timestamp;

	fpga.write(messageToVector(phase, 2));
	//std::cout << "\nphase";
	//std::cout << std::hex << phase;

	fpga.write(messageToVector(atw, 2));
	//std::cout << "\natw";
	//std::cout << std::hex << atw;

	fpga.write(messageToVector(ftw, 4));
	//std::cout << "\nftw";
	//std::cout << std::hex << ftw;

	fpga.write(messageToVector(0x00, 1));
	//std::cout << "\nterminator";
	//std::cout << std::hex << 0x00;

	//std::cout << "\n  End timestamp";

		//the uart buffer is only 16 bytes deep, so we need to
		//wait until all data is written after every word
		//self.uart.flush()

}

std::vector<awgCommand>& fpgaAWG::selectCommandList(unsigned long channel) {

	if (channel == 0) {
		return awgCommandListDAC0;
	}
	else if (channel == 1) {
		return awgCommandListDAC1;
	}
	else if (channel == 2) {
		return awgCommandListDAC2;
	}
	else if (channel == 3) {
		return awgCommandListDAC3;
	}
	else if (channel == 4) {
		return awgCommandListDAC4;
	}
	else if (channel == 5) {
		return awgCommandListDAC5;
	}
	else if (channel == 6) {
		return awgCommandListDAC6;
	}
	else if (channel == 7) {
		return awgCommandListDAC7;
	}
	else {
		thrower("Writing to multiple AWG channels at once is not currently supported in Chimera.");
	}
}

void fpgaAWG::setStep(unsigned long channel, float tStep, float tStart, float tEnd) {
	int ts = getTTW(tStep);
	if (ts <= 0) {
		thrower("step size below minimum of 6.5 ns.");
	};
	stepSize = ts;

	int numSteps = ceil((tEnd - tStart) / (stepSize * AWGMINSTEP));

	if (numSteps <= 1) {
		thrower("Steps duration shorter than step size.");
	};

	std::vector<awgCommand> & awgCommandList = selectCommandList(channel);

	nPreviousStepSetting = awgCommandList.size();
	startTimeStepSetting = tStart;

	for (int i = 0; i < numSteps; i++) {
		awgCommandList.push_back(awgCommand());
		//awgCommandList.back().channel = channel; //set channel values here.
		awgCommandList.back().address = nPreviousStepSetting + i;
		awgCommandList.back().timeStampMicro = tStart + i * stepSize * AWGMINSTEP; //must have initial time step at t=0.
	};

}

void fpgaAWG::setSingle(unsigned long channel, float time, float amp, float freq, bool phase_update, float phase) {
	std::vector<awgCommand> & awgCommandList = selectCommandList(channel);

	nPreviousStepSetting = awgCommandList.size();
	awgCommandList.push_back(awgCommand());
	awgCommandList.back().address = nPreviousStepSetting;
	awgCommandList.back().timeStampMicro = time;
	awgCommandList.back().ampPercent = amp;
	awgCommandList.back().freqMHz = freq;
	awgCommandList.back().phase_update = phase_update;
	awgCommandList.back().phaseDegrees = phase;
}

void fpgaAWG::freqLinearRamp(unsigned long channel, float tStart, float tEnd, float fStart, float fEnd, bool phase_update, float phaseStart) {
	int iStart = nPreviousStepSetting + ceil((tStart - startTimeStepSetting) / (stepSize * AWGMINSTEP));
	int numSteps = ceil((tEnd - tStart) / (stepSize * AWGMINSTEP));
	float fStep = (fEnd - fStart) / (numSteps - 1);

	std::vector<awgCommand> & awgCommandList = selectCommandList(channel);

	if (tEnd > awgCommandList.back().timeStampMicro + stepSize * AWGMINSTEP) {
		thrower("Ramp too long for assigned steps.");
	};

	for (int i = 0; i < numSteps; i++) {
		awgCommandList[iStart + i].freqMHz = fStart + i * fStep;
		awgCommandList[iStart + i].phaseDegrees = phaseStart;
		awgCommandList[iStart + i].phase_update = false;
	};
	awgCommandList[iStart].phase_update = phase_update;
}

void fpgaAWG::ampLinearRamp(unsigned long channel, float tStart, float tEnd, float aStart, float aEnd) {
	int iStart = nPreviousStepSetting + ceil((tStart - startTimeStepSetting) / (stepSize * AWGMINSTEP));
	int numSteps = ceil((tEnd - tStart) / (stepSize * AWGMINSTEP));
	float aStep = (aEnd - aStart) / (numSteps - 1);

	std::vector<awgCommand> & awgCommandList = selectCommandList(channel);

	if (tEnd > awgCommandList.back().timeStampMicro + stepSize * AWGMINSTEP) {
		thrower("Ramp too long for assigned steps.");
	};

	for (int i = 0; i < numSteps; i++) {
		awgCommandList[iStart + i].ampPercent = aStart + i * aStep;
	};
}

void fpgaAWG::ampGaussianRamp(unsigned long channel, float tStart, float tEnd, float tSigma, int direction, float aStart, float aStop) {
	int iStart = nPreviousStepSetting + ceil((tStart - startTimeStepSetting) / (stepSize * AWGMINSTEP));
	int numSteps = ceil((tEnd - tStart) / (stepSize * AWGMINSTEP));

	std::vector<awgCommand> & awgCommandList = selectCommandList(channel);

	if (tEnd > awgCommandList.back().timeStampMicro + stepSize * AWGMINSTEP) {
		thrower("Ramp too long for assigned steps.");
	};

	float t, aVal;

	if (direction == 1) {
		for (int i = 0; i < numSteps; i++) {
			t = stepSize * AWGMINSTEP * i + tStart;
			aVal = (aStop - aStart) * exp(-1 * pow((t - tEnd), 2) / (2 * pow(tSigma, 2))) + aStart;
			awgCommandList[iStart + i].ampPercent = aVal;
		};
	}
	else if (direction == -1) {
		for (int i = 0; i < numSteps; i++) {
			t = stepSize * AWGMINSTEP * i + tStart;
			aVal = (aStart - aStop) * exp(-1 * pow((t - tStart), 2) / (2 * pow(tSigma, 2))) + aStop;
			awgCommandList[iStart + i].ampPercent = aVal;
		};
	}
	else {
		thrower("Direction must be +1 or -1.");
	}
}

//// FOR OPTIMAL GHZ ////

///Read Polynomaial from File ///
std::vector<double> fpgaAWG::readCoefficients(const std::string& filename){
	std::vector<double> coefficients;
	std::ifstream file(filename);
	std::string line;

	while (std::getline(file, line)){
		std::stringstream ss(line);
		double coeff;
		ss >> coeff;
		coefficients.push_back(coeff);
	}

	return coefficients;
}

double fpgaAWG::evaluatePolynomial(const std::vector<double>& coeffs, double x){
	double result = 0.0;
	for (int i = coeffs.size()-1; i>=0; --i){
		int n = coeffs.size() - 1;
		result = result + coeffs[i] * pow(x, n - i);
	}

	return result;
}

void fpgaAWG::freqGHZRamp(unsigned long channel, float tStart, float tEnd, float omega, bool phase_update, float phaseStart, int N, int omm) {
	int iStart = nPreviousStepSetting + ceil((tStart - startTimeStepSetting) / (stepSize * AWGMINSTEP));
	int numSteps = ceil((tEnd - tStart) / (stepSize * AWGMINSTEP));

	std::vector<awgCommand> & awgCommandList = selectCommandList(channel);

	char filePath[200];

	sprintf(filePath, "B:\Yb heap\Experiment_code_Yb\Chimera-Control-Master\Waveforms\ampl%i_N=%i.txt", N, omm);
	std::vector<double> coefficients = readCoefficients(filePath);

	if (tEnd > awgCommandList.back().timeStampMicro + stepSize * AWGMINSTEP) {
		thrower("Ramp too long for assigned steps.");
	};

	float t, aVal, fVal;

	for (int i = 0; i < numSteps; i++) {
		t = stepSize * AWGMINSTEP * i + tStart;
		fVal = omega*evaluatePolynomial(coefficients,t);
		awgCommandList[iStart + i].freqMHz = fVal;
		awgCommandList[iStart + i].phaseDegrees = phaseStart;
		awgCommandList[iStart + i].phase_update = false;
	};

	awgCommandList[iStart].phase_update = phase_update;
	
}

void fpgaAWG::ampGHZRamp(unsigned long channel, float tStart, float tEnd, float ampMax, float power, float omega, float norm, float offset, int N, int omm) {
	int iStart = nPreviousStepSetting + ceil((tStart - startTimeStepSetting) / (stepSize * AWGMINSTEP));
	int numSteps = ceil((tEnd - tStart) / (stepSize * AWGMINSTEP));

	char filePath[200];

	sprintf(filePath, "B:\Yb heap\Experiment_code_Yb\Chimera-Control-Master\Waveforms\ampl%i_N=%i.txt", N, omm);
	std::vector<double> coefficients = readCoefficients(filePath);

	std::vector<awgCommand> & awgCommandList = selectCommandList(channel);

	if (tEnd > awgCommandList.back().timeStampMicro + stepSize * AWGMINSTEP) {
		thrower("Ramp too long for assigned steps.");
	};

	float t, fVal, aVal;
	if (omm == 1){
		for (int i = 0; i < numSteps; i++) {
			t = stepSize * AWGMINSTEP * i + tStart;
			fVal = omega*evaluatePolynomial(coefficients,t);
			aVal = ampMax - ampMax*pow(fabs(cos(3.14156 *(t-tStart)/(tEnd-tStart))),2*power);
			awgCommandList[iStart + i].ampPercent = aVal;
		}
	}

	else {
		for (int i = 0; i < numSteps; i++) {
			t = stepSize * AWGMINSTEP * i + tStart;
			fVal = omega*evaluatePolynomial(coefficients,t);
			aVal = ampMax*pow(pow(sin(3.14156/2+sin(3.14156 *(t-tStart)/(tEnd-tStart))),2*power),2);
			awgCommandList[iStart + i].ampPercent = aVal;
		}
	}
	
}


//// END OPTIMAL GHZ STUFF ////


void fpgaAWG::freqGaussianRamp(unsigned long channel, float tStart, float tEnd, float tSigma, int direction, float fStart, float fStop, bool phase_update, float phaseStart) {
	int iStart = nPreviousStepSetting + ceil((tStart - startTimeStepSetting) / (stepSize * AWGMINSTEP));
	int numSteps = ceil((tEnd - tStart) / (stepSize * AWGMINSTEP));

	std::vector<awgCommand> & awgCommandList = selectCommandList(channel);

	if (tEnd > awgCommandList.back().timeStampMicro + stepSize * AWGMINSTEP) {
		thrower("Ramp too long for assigned steps.");
	};

	float t, fVal;

	if (direction == 1) {
		for (int i = 0; i < numSteps; i++) {
			t = stepSize * AWGMINSTEP * i + tStart;
			fVal = (fStop - fStart) * exp(-1 * (t - tEnd) * (t - tEnd) / (2 * tSigma * tSigma)) + fStart;
			awgCommandList[iStart + i].freqMHz = fVal;
			awgCommandList[iStart + i].phaseDegrees = phaseStart;
			awgCommandList[iStart + i].phase_update = false;
		};
	}
	else if (direction == -1) {
		for (int i = 0; i < numSteps; i++) {
			t = stepSize * AWGMINSTEP * i + tStart;
			fVal = (fStart - fStop) * exp(-1 * (t - tStart) * (t - tStart) / (2 * tSigma * tSigma)) + fStop;
			awgCommandList[iStart + i].freqMHz = fVal;
			awgCommandList[iStart + i].phaseDegrees = phaseStart;
			awgCommandList[iStart + i].phase_update = false;
		};
	}
	else {
		thrower("Direction must be +1 or -1.");
	};
	awgCommandList[iStart].phase_update = phase_update;
}

void fpgaAWG::freqtimeoptlukin(unsigned long channel, float tStart, float tEnd,  float freq, float A, float omega, float phi0, float delta0, bool phase_update, float phaseStart) {
	int iStart = nPreviousStepSetting + ceil((tStart - startTimeStepSetting) / (stepSize * AWGMINSTEP));
	int numSteps = ceil((tEnd - tStart) / (stepSize * AWGMINSTEP));

	std::vector<awgCommand> & awgCommandList = selectCommandList(channel);

	if (tEnd > awgCommandList.back().timeStampMicro + stepSize * AWGMINSTEP) {
		thrower("Ramp too long for assigned steps.");
	};

	float t, phaseVal;
	for (int i = 0; i < numSteps; i++) {
		t = stepSize * AWGMINSTEP * i;
		phaseVal = A * cos(omega * t - phi0);
		awgCommandList[iStart + i].freqMHz = freq + delta0;
		awgCommandList[iStart + i].phaseDegrees = phaseStart + phaseVal;
		awgCommandList[iStart + i].phase_update = false;
	};
	
	awgCommandList[iStart].phase_update = phase_update;
}

void fpgaAWG::writeCommandList(unsigned long channel, int AWGnum) {
	std::vector<awgCommand> & awgCommandList = selectCommandList(channel);
	int numSteps = awgCommandList.size();
	if (numSteps > 16384) {
		thrower("AWG out of memory, too many time steps.");
	}
	unsigned char channelWord = 0b0000;
	if (AWGnum == 0) {
		if (channel == 0) {
			channelWord = 0b0001;
		}
		else if (channel == 1) {
			channelWord = 0b0010;
		}
		else if (channel == 2) {
			channelWord = 0b0100;
		}
		else if (channel == 3) {
			channelWord = 0b1000;
		}
		for (int i = 0; i < numSteps; i++) {
			writeTimestamp(channelWord, awgCommandList[i].address, awgCommandList[i].timeStampMicro, awgCommandList[i].phase_update, awgCommandList[i].phaseDegrees, awgCommandList[i].ampPercent, awgCommandList[i].freqMHz);
		};
		writeTimestamp(channelWord, numSteps, 0.0, false, 0.0, 0.0, 0.0);
	}
	else if (AWGnum == 1) {
		if (channel == 4) {
			channelWord = 0b0001;
		}
		else if (channel == 5) {
			channelWord = 0b0010;
		}
		else if (channel == 6) {
			channelWord = 0b0100;
		}
		else if (channel == 7) {
			channelWord = 0b1000;
		}
		for (int i = 0; i < numSteps; i++) {
			writeTimestamp(channelWord, awgCommandList[i].address, awgCommandList[i].timeStampMicro, awgCommandList[i].phase_update, awgCommandList[i].phaseDegrees, awgCommandList[i].ampPercent, awgCommandList[i].freqMHz);
		};
		writeTimestamp(channelWord, numSteps, 0.0, false, 0.0, 0.0, 0.0);
	}



}

void fpgaAWG::loadAWGScript(std::string scriptAddress)
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
	currentAWGScript.str("");
	currentAWGScript.str(buf.str());
	currentAWGScript.clear();
	currentAWGScript.seekg(0);
	//std::string str(currentMoogScript.str());
	scriptFile.close();
}

void fpgaAWG::analyzeAWGScript(fpgaAWG* fpgaawg, std::vector<variableType>& variables, UINT variation, int AWGnum)
{
	currentAWGScriptText = currentAWGScript.str();
	if (currentAWGScript.str() == "")
	{
		thrower("ERROR: Moog script is empty!\r\n");
	}
	std::string word;
	currentAWGScript >> word;
	std::vector<UINT> totalRepeatNum, currentRepeatNum;
	std::vector<std::streamoff> repeatPos;

	//Important: clear all old settings before new write.
	awgCommandListDAC0.clear();
	awgCommandListDAC1.clear();
	awgCommandListDAC2.clear();
	awgCommandListDAC3.clear();
	awgCommandListDAC4.clear();
	awgCommandListDAC5.clear();
	awgCommandListDAC6.clear();
	awgCommandListDAC7.clear();
	nPreviousStepSetting = 0;
	startTimeStepSetting = 0.0;
	stepSize = 1;

	// the analysis loop.

	while (!(currentAWGScript.peek() == EOF) || word != "__end__")
	{
		if (word == "customsequence") {
			break;
		}
		else if (word == "setsteps") {
			std::string channel;
			Expression step, start, stop;
			currentAWGScript >> channel;
			currentAWGScript >> step;
			currentAWGScript >> start;
			currentAWGScript >> stop;
			setStep(stoul(channel, nullptr, 0), step.evaluate(variables, variation), start.evaluate(variables, variation), stop.evaluate(variables, variation));
		}
		else if (word == "amplin") {
			std::string channel;
			Expression tstart, tstop, astart, astop;
			currentAWGScript >> channel;
			currentAWGScript >> tstart;
			currentAWGScript >> tstop;
			currentAWGScript >> astart;
			currentAWGScript >> astop;
			ampLinearRamp(stoul(channel, nullptr, 0), tstart.evaluate(variables, variation), tstop.evaluate(variables, variation), astart.evaluate(variables, variation), astop.evaluate(variables, variation));
		}
		else if (word == "ampgauss") {
			std::string channel;
			Expression tstart, tstop, tsigma, direction, azerolevel, apeak;
			currentAWGScript >> channel;
			currentAWGScript >> tstart;
			currentAWGScript >> tstop;
			currentAWGScript >> tsigma;
			currentAWGScript >> direction;
			currentAWGScript >> azerolevel;
			currentAWGScript >> apeak;
			ampGaussianRamp(stoul(channel, nullptr, 0), tstart.evaluate(variables, variation), tstop.evaluate(variables, variation), tsigma.evaluate(variables, variation), direction.evaluate(variables, variation), azerolevel.evaluate(variables, variation), apeak.evaluate(variables, variation));
		}
		else if (word == "freqlin") {
			std::string channel;
			Expression tstart, tstop, fstart, fstop, phase_update, phaseDegrees;
			currentAWGScript >> channel;
			currentAWGScript >> tstart;
			currentAWGScript >> tstop;
			currentAWGScript >> fstart;
			currentAWGScript >> fstop;
			currentAWGScript >> phase_update;
			currentAWGScript >> phaseDegrees;

			freqLinearRamp(stoul(channel, nullptr, 0), tstart.evaluate(variables, variation), tstop.evaluate(variables, variation), fstart.evaluate(variables, variation), fstop.evaluate(variables, variation), phase_update.evaluate(variables, variation), phaseDegrees.evaluate(variables, variation));
		}
		else if (word == "freqgauss") {
			std::string channel;
			Expression tstart, tstop, tSigma, direction, fstart, fstop, phase_update, phaseDegrees;
			currentAWGScript >> channel;
			currentAWGScript >> tstart;
			currentAWGScript >> tstop;
			currentAWGScript >> tSigma;
			currentAWGScript >> direction;
			currentAWGScript >> fstart;
			currentAWGScript >> fstop;
			currentAWGScript >> phase_update;
			currentAWGScript >> phaseDegrees;

			freqGaussianRamp(stoul(channel, nullptr, 0), tstart.evaluate(variables, variation), tstop.evaluate(variables, variation), tSigma.evaluate(variables, variation), direction.evaluate(variables, variation), fstart.evaluate(variables, variation), fstop.evaluate(variables, variation), phase_update.evaluate(variables, variation), phaseDegrees.evaluate(variables, variation));
		}
		else if (word == "freqtimeoptlukin") {

			std::string channel;
			Expression tstart, tstop, freq, A,  omega, phi0, delta0, phase_update, phaseDegrees;
			currentAWGScript >> channel;
			currentAWGScript >> tstart;
			currentAWGScript >> tstop;
			currentAWGScript >> freq;
			currentAWGScript >> A;
			currentAWGScript >> omega;
			currentAWGScript >> phi0;
			currentAWGScript >> delta0;
			currentAWGScript >> phase_update;
			currentAWGScript >> phaseDegrees;

			freqtimeoptlukin(stoul(channel, nullptr, 0), tstart.evaluate(variables, variation), tstop.evaluate(variables, variation), freq.evaluate(variables, variation), A.evaluate(variables, variation),omega.evaluate(variables, variation), phi0.evaluate(variables, variation), delta0.evaluate(variables, variation), phase_update.evaluate(variables, variation), phaseDegrees.evaluate(variables, variation));
		}
		else if (word == "setsingle") {
			std::string channel;
			Expression time, amp, freq, phase_update, phaseDegrees;
			currentAWGScript >> channel;
			currentAWGScript >> time;
			currentAWGScript >> amp;
			currentAWGScript >> freq;
			currentAWGScript >> phase_update;
			currentAWGScript >> phaseDegrees;

			setSingle(stoul(channel, nullptr, 0), time.evaluate(variables, variation), amp.evaluate(variables, variation), freq.evaluate(variables, variation), phase_update.evaluate(variables, variation), phaseDegrees.evaluate(variables, variation));
		}
		else if (word == "program") {
			std::string channel;
			currentAWGScript >> channel;
			writeCommandList(stoul(channel, nullptr, 0), AWGnum);

			//Important - after programming a single channel reset resources so another channel can be programmed.
			nPreviousStepSetting = 0;
			startTimeStepSetting = 0.0;
			std::vector<awgCommand> & awgCommandList = selectCommandList(stoul(channel, nullptr, 0));
			awgCommandList.clear();
		}
		else if (word == "reset") {
			reset();
		}
		else if (word == "trigger") {
			Sleep(100);
			trigger();
		}
		/// GHZ RAMPS ///

		else if (word == "freqghzramp") {

			std::string channel;
			Expression tstart, tstop, omega, phase_update, phaseDegrees, N, omm;
			currentAWGScript >> channel;
			currentAWGScript >> tstart;
			currentAWGScript >> tstop;
			currentAWGScript >> omega;
			currentAWGScript >> phase_update;
			currentAWGScript >> phaseDegrees;
			currentAWGScript >> N;
			currentAWGScript >> omm;

			freqGHZRamp(stoul(channel, nullptr, 0), tstart.evaluate(variables, variation), tstop.evaluate(variables, variation), omega.evaluate(variables, variation), phase_update.evaluate(variables, variation), phaseDegrees.evaluate(variables, variation), N.evaluate(variables, variation), omm.evaluate(variables, variation));
		}

		else if (word == "ampghzramp") {

			std::string channel;
			Expression tstart, tstop, ampMax, power, omega, norm, offset, N, omm;
			currentAWGScript >> channel;
			currentAWGScript >> tstart;
			currentAWGScript >> tstop;
			currentAWGScript >> ampMax;
			currentAWGScript >> power;
			currentAWGScript >> omega;
			currentAWGScript >> norm;
			currentAWGScript >> offset;
			currentAWGScript >> N;
			currentAWGScript >> omm;

			ampGHZRamp(stoul(channel, nullptr, 0), tstart.evaluate(variables, variation), tstop.evaluate(variables, variation), ampMax.evaluate(variables, variation), power.evaluate(variables, variation), omega.evaluate(variables, variation), norm.evaluate(variables, variation), offset.evaluate(variables, variation),N.evaluate(variables, variation), omm.evaluate(variables, variation));
		}

		/// END GHZ RAMPS ///
		else
		{
			thrower("ERROR: unrecognized AWG script command: \"" + word + "\"");
		}
		word = "";
		currentAWGScript >> word;
	}
}