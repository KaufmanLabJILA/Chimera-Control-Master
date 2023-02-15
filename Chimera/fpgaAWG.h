#pragma once
#include "stdafx.h"

#include "constants.h"
#include "ScriptStream.h"
#include <vector>
#include <cmath>
#include "BoostAsyncSerial.h"
#include "awgCommand.h"


#define AWGMINSTEP    (double)0.0065104166666666666666666666666 //Minimum step size in us.

class fpgaAWG{

public:
	fpgaAWG(std::string portID, int baudrate);
	virtual ~fpgaAWG(void);
	void trigger();
	void reset();
	void writeTimestamp(unsigned char channel, unsigned short address, float timeStampMicro, bool phase_update, float phaseDegrees, float ampPercent, float freqMHz);

	void setStep(unsigned long channel, float tstep, float tStart, float tEnd);
	void setSingle(unsigned long channel, float time, float amp, float freq, bool phase_update, float phase);
	void freqLinearRamp(unsigned long channel, float tStart, float tEnd, float fStart, float fEnd, bool phase_update, float phaseStart);
	void freqGaussianRamp(unsigned long channel, float tStart, float tEnd, float tSigma, int direction, float fStart, float fStop, bool phase_update, float phaseStart);
	void writeCommandList(unsigned long channel, int AWGnum);
	void ampLinearRamp(unsigned long channel, float tStart, float tEnd, float aStart, float aEnd);
	void ampGaussianRamp(unsigned long channel, float tStart, float tEnd, float tSigma, int direction, float aStart, float aStop);

	static unsigned long int getFTW(double freqMHz) {
		//36-bit DDS
		//1 MHz = 223696213.33333333333333333333333 FTW
		double MHz = 223696213.33333333333333333333333;
		unsigned long long int ftw = round(freqMHz * MHz);
		return (ftw >> 4);
	};
	static unsigned short int getATW(double ampPercent) {
		if (ampPercent < 0 || ampPercent > 100) {
			thrower("AWG amplitude out of range, should be <100%.");
		}
		unsigned short int ATW = round(ampPercent * 65535 / 100.0);
		return(ATW);
	};
	static unsigned int getPTW(double phaseDegrees) {
		if (phaseDegrees < -3600 ) {
			thrower("AWG phase out of range, should be >-3600 degrees.");
		}

		unsigned int PTW = round(fmod(phaseDegrees+3600,360) * 4095 / 360.0);
		return(PTW);
	}
	static unsigned long int getTTW(double timeMicrosecs) {
		//Max update rate is 153.6 MHz - step size is 6.51041667 ns.
		if (timeMicrosecs < 0 || timeMicrosecs > 27962026.7) {
			thrower("AWG time out of range, should be <27.96 seconds.");
		}
		unsigned long int TTW = round(timeMicrosecs / AWGMINSTEP);
		return(TTW);
	}
	//Attempt to parse moog script
	void loadAWGScript(std::string scriptAddress);
	void analyzeAWGScript(fpgaAWG * fpgaawg, std::vector<variableType>& variables, UINT variation, int AWGnum);

private:
	BoostAsyncSerial fpga;
	std::vector<unsigned char> messageToVector(int message, int bytes);

	unsigned int stepSize = 1;
	unsigned int nPreviousStepSetting = 0;
	float startTimeStepSetting = 0.0;
	std::vector<awgCommand> awgCommandListDAC0;
	std::vector<awgCommand> awgCommandListDAC1;
	std::vector<awgCommand> awgCommandListDAC2;
	std::vector<awgCommand> awgCommandListDAC3;
	std::vector<awgCommand>& selectCommandList(unsigned long channel);

	// the script file contents get dumped into this.
	std::string currentAWGScriptText;
	ScriptStream currentAWGScript;
};
