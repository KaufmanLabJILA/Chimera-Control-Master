#pragma once
#include <array>
#include <string>
#include <unordered_map>
#include <iomanip>
#include <vector>

#include <unordered_map>
#include <iomanip>

#include "Control.h"
#include "VariableSystem.h"

#include "Control.h"
#include "VariableSystem.h"
#include "DioSystem.h"
#include "miscellaneousCommonFunctions.h"

#include "nidaqmx2.h"
#include "EDacStructures.h"

class EDacSystem
{
	public:
	EDacSystem();
        void interpretKey( std::vector<variableType>& variables, std::string& warnings );
        //void findLoadSkipSnapshots( double time, std::vector<variableType>& variables, UINT variation );
        //void makeFinalDataFormat(UINT variation);
        void checkTimingsWork(UINT variation);
        //void writeEDacs(UINT variation, bool loadSkip);
        void updateEDACFile(UINT variation);
        void getEDacFinalData(UINT variation);
        void handleEDacScriptCommand( EDacCommandForm command, std::vector<variableType>& vars);
        void setEDacCommandForm( EDacCommandForm command );
        void writeEDACs(UINT variation);
	private:
        std::vector<std::vector<EDacCommand>> edacCommandList;
        std::vector<std::vector<EDacSnapshot>> edacSnapshots;
        std::vector<std::vector<EDacChannelSnapshot>> finalEDacSnapshots;
        std::vector<std::vector<EDacSnapshot>> loadSkipEDacSnapshots;
        std::vector<std::array<double,8>> finalFormatEDacData;
        std::vector<std::array<std::vector<double>, 8>> loadSkipEDacFinalFormat;
        std::vector<EDacCommandForm> edacCommandFormList;
        double edacVoltageValue;

};
