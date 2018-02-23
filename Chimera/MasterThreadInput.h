﻿#pragma once

#include "DioSystem.h"
#include "DacSystem.h"
#include "VariableSystem.h"
#include "RhodeSchwarz.h"
#include "GpibFlume.h"
#include "DebugOptionsControl.h"
#include "Agilent.h"
#include "EmbeddedPythonHandler.h"
#include <chrono>
#include <vector>
#include <atomic>

//for moog test:
#include "serial_synth.h"

class MasterManager;

//Useful structure for containing relevant parameters for master thread.
struct MasterThreadInput
{
	//for moog test:
	serial_synth* moog;
	std::string moogScriptAddress;

	EmbeddedPythonHandler* python;
	DataLogger* logger;
	profileSettings profile;
	DioSystem* ttls;
	DacSystem* dacs;
	UINT repetitionNumber;
	std::vector<variableType> variables;
	std::vector<variableType> constants;
	MasterManager* thisObj;
	std::string masterScriptAddress;
	Communicator* comm;
	RhodeSchwarz* rsg;
	debugInfo debugOptions;
	std::vector<Agilent*> agilents;
	TektronicsControl* topBottomTek;
	TektronicsControl* eoAxialTek;
	VariableSystem* globalControl;
	NiawgController* niawg;
	UINT intensityAgilentNumber;
	bool quiet;
	mainOptions settings;
	bool runNiawg;
	bool runMaster;
	bool runMoog;
	// only for rearrangement.
	std::mutex* rearrangerLock;
	std::vector<std::vector<bool>>* atomQueueForRearrangement;
	chronoTimes* andorsImageTimes;
	chronoTimes* grabTimes;
	std::condition_variable* conditionVariableForRearrangement;
	rerngOptions rearrangeInfo;
	std::atomic<bool>* skipNext;
};


struct ExperimentInput
{
	ExperimentInput::ExperimentInput( ) :
		includesCameraRun( false ), masterInput( NULL ), plotterInput( NULL ), cruncherInput( NULL ) { }
	MasterThreadInput* masterInput;
	realTimePlotterInput* plotterInput;
	atomCruncherInput* cruncherInput;
	AndorRunSettings camSettings;
	bool includesCameraRun;
};