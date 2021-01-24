#pragma once
#include <array>
#include <string>
#include <unordered_map>

#include "Control.h"
#include "VariableSystem.h"
#include "DioSystem.h"
#include "miscellaneousCommonFunctions.h"

#include "nidaqmx2.h"
#include "DDSStructures.h"

/**
 * The DDSSystem class is based on the DAC and DIOSystems.
 */
class DDSSystem
{
	public:
		DDSSystem();
		void handleNewConfig( std::ofstream& newFile );
		void handleSaveConfig(std::ofstream& saveFile);
		void handleOpenConfig(std::ifstream& openFile, int versionMajor, int versionMinor);
		void abort();
		void initialize( POINT& pos, cToolTips& toolTips, AuxiliaryWindow* master, int& id );
		std::string getDDSSequenceMessage(UINT variation );
		void handleButtonPress();
		void setDDSCommandForm( DDSCommandForm command );
		void setForceDDSEvent( int line, double val, UINT variation );
		void setDDSStatusNoForceOut(std::array< double, 12> status);
		void interpretKey( std::vector<variableType>& variables, std::string& warnings );
		void organizeDDSCommands(UINT variation);
		void makeFinalDataFormat(UINT variation );
		void writeDDSs( UINT variation, bool loadSkip );
		void configureClocks( UINT variation, bool loadSkip );
		void setDefaultValue(UINT ddsNum, double val);
		double getDefaultValue(UINT ddsNum);

		unsigned int getNumberSnapshots(UINT variation );
		void checkTimingsWork(UINT variation );
		void setName(int ddsNumber, std::string name, cToolTips& toolTips, AuxiliaryWindow* master);
		std::string getName(int ddsNumber);
		std::array<std::string, 12> getAllNames();
		std::string getErrorMessage(int errorCode);
		ULONG getNumberEvents(UINT variation );
		void handleDDSScriptCommand( DDSCommandForm command, std::string name, std::vector<UINT>& ddsShadeLocations, 
									 std::vector<variableType>& vars, DioSystem* ttls );
		int getDDSIdentifier(std::string name);
		double getDDSValue(int ddsNumber);
		std::array< double, 12> DDSSystem::getDDSStatus();
		unsigned int getNumberOfDDSs();
		std::pair<double, double> getDDSAmpRange(int ddsNumber);
		std::pair<double, double> getDDSFreqRange(int ddsNumber);
		void setAmpMinMax(int ddsNumber, double min, double max);
		void setFreqMinMax(int ddsNumber, double min, double max);
		void shadeDDSs(std::vector<unsigned int>& ddsShadeLocations);
		void unshadeDDSs();
		void rearrange(UINT width, UINT height, fontMap fonts);
		bool isValidDDSName(std::string name);
		HBRUSH handleColorMessage(CWnd* hwnd, brushMap brushes, rgbMap rgbs, CDC* cDC);
		void resetDDSEvents();
		std::array< std::array<double, 2>, 12> getFinalSnapshot();
		void checkValuesAgainstLimits(UINT variation );
		void prepareForce();
		void prepareDDSForceChange(int line, double val);
		double roundToDDSResolution(double num);
		void handleEditChange( UINT ddsNumber );
		void setDDSs();

	private:
		Control<CStatic> ddsTitle;
		Control<CStatic> dds0Title;
		Control<CStatic> dds1Title;
		Control<CStatic> dds2Title;
		std::array < Control<CStatic>, 3> ampLabels;
		std::array < Control<CStatic>, 3> freqLabels;
		Control<CButton> ddsSetButton;
		//Control<CButton> zeroDSSs;
		std::array<Control<CStatic>, 12> ddsLabels;
		std::array<Control<CEdit>, 12> breakoutBoardFreqEdits;
		std::array<double, 12> ddsValues;
		std::array<std::string, 12> ddsNames;
		std::array<double, 12> ddsMinAmp;
		std::array<double, 12> ddsMaxAmp;
		std::array<double, 12> ddsMinFreq;
		std::array<double, 12> ddsMaxFreq;
		std::array<double, 12> defaultVals;
		std::array <const double, 2> ddsResolution;
		std::vector<DDSCommandForm> ddsCommandFormList;
		// the first vector is for each variation.
		std::vector<std::vector<DDSCommand>> ddsCommandList;
		std::vector<std::vector<DDSSnapshot>> ddsSnapshots;
		std::vector<std::vector<DDSChannelSnapshot>> ddsChannelSnapshots;
		std::vector<std::pair<double, std::vector<DDSCommand>>> timeOrganizer;

		//Zynq tcp connection
		ZynqTCP zynq_tcp;

		bool roundToDDSPrecision;
	
};
