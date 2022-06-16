﻿#pragma once

#include "DioSystem.h"
#include "DacSystem.h"
#include "DDSSystem.h"
#include "VariableSystem.h"
#include "GpibFlume.h"
#include "DebugOptionsControl.h"
#include "EmbeddedPythonHandler.h"
#include <chrono>
#include <vector>
#include <atomic>
#include <condition_variable>
#include "atomCruncherInput.h"
#include "SerialSynth.h"
#include "DDS_SYNTH.h"
#include "GigaMoog.h"
#include "fpgaAWG.h"

#include "rerngParams.h"

class DataLogger;
class MasterManager;

//Useful structure for containing relevant parameters for master thread.
struct MasterThreadInput
{
	//for moog test:
	//SerialSynth* moog;
	//std::string moogScriptAddress;
	fpgaAWG* awg;
	std::string awgScriptAddress;
	gigaMoog* gmoog;
	std::string gmoogScriptAddress;
	std::string ddsScriptAddress;

	EmbeddedPythonHandler* python;
	DataLogger* logger;
	profileSettings profile;
	DioSystem* ttls;
	DacSystem* dacs;
	DDSSystem* ddss;
	DDS_SYNTH* dds;
	UINT repetitionNumber;
	std::vector<variableType> variables;
	std::vector<variableType> constants;
	MasterManager* thisObj;
	std::string masterScriptAddress;
	Communicator* comm;
	debugInfo debugOptions;
	VariableSystem* globalControl;
	UINT intensityAgilentNumber;
	bool quiet;
	mainOptions settings;
	bool runSingle;
	bool runNiawg;
	bool runMaster;
	bool runAWG;
	// only for rearrangement.
	std::mutex* rearrangerLock;
	std::vector<std::vector<bool>>* atomQueueForRearrangement;
	chronoTimes* andorsImageTimes;
	chronoTimes* grabTimes;
	std::condition_variable* conditionVariableForRearrangement;
	rerngOptions rearrangeInfo;
	std::atomic<bool>* skipNext;
	bool multipleExperiments;
};


struct ExperimentInput
{
	ExperimentInput::ExperimentInput() :
		includesCameraRun(false), multipleExperiments(false), masterInput(NULL), plotterInput(NULL), cruncherInput(NULL) { };
	MasterThreadInput* masterInput;
	realTimePlotterInput* plotterInput;
	atomCruncherInput* cruncherInput;
	AndorRunSettings camSettings;
	bool includesCameraRun;
	bool multipleExperiments;
};