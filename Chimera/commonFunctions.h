#pragma once
#include "Windows.h"

class MainWindow;
class CameraWindow;
class AuxiliaryWindow;

namespace commonFunctions
{
	/// Call to direct message to appropriate function in this namespace
	void handleCommonMessage( int msgID, CWnd* parent, MainWindow* comm, ScriptingWindow* scriptWin, 
							 CameraWindow* camWin, AuxiliaryWindow* auxWin );
	/// Run Menu
	void prepareCamera( MainWindow* mainWin, CameraWindow* camWin, ExperimentInput& input, bool isSequence );
	void prepareMasterThread( int msgID, ScriptingWindow* scriptWin, MainWindow* mainWin, CameraWindow* camWin,
							  AuxiliaryWindow* auxWin, ExperimentInput& input, bool single, bool runAWG, bool runTtls, bool isSequence);
	void abortRearrangement( MainWindow* mainWin, CameraWindow* camWin );
	void startMaster(MainWindow* mainWin, ExperimentInput& input, bool waitTillFinished = false);
	UINT __cdecl multipleExperimentThreadProcedure(void* voidInput);

	std::string logParameters( ExperimentInput& input, CameraWindow* camWin, bool takeAndorPictures );
	//void startFullMasterThread( MainWindow* mainWin, AuxiliaryWindow* auxWin, ScriptingWindow* scriptWin, 
	//							ExperimentInput& input, CameraWindow* camWin );
	void setMot(MainWindow* mainWin);
	//void abortNiawg( ScriptingWindow* scriptWin, MainWindow* mainWin );
	void abortCamera( CameraWindow* camWin, MainWindow* mainWin );
	void abortMaster(MainWindow* mainWin, AuxiliaryWindow* auxWin);
	void exitProgram( ScriptingWindow* scriptWindow, MainWindow* mainWin, CameraWindow* camWin, AuxiliaryWindow* auxWin );
	/// Scripting Menu
	int saveProfile( ScriptingWindow* scriptWindow, MainWindow* mainWin );
	//void reloadNIAWGDefaults( MainWindow* mainWin );
	int sendPythonInitializationFile(std::string runType, std::string dateStr);
	bool watchPythonUpdate2(MainWindow* mainWin);
	bool watchPythonUpdate(MainWindow* mainWin);
	void updateGlobalVars(MainWindow* mainWin, AuxiliaryWindow* auxWin);

	struct MultiExperimentInput
	{
		int msgID;
		CWnd* parent;
		MainWindow* mainWin;
		ScriptingWindow* scriptWin;
		CameraWindow* camWin;
		AuxiliaryWindow* auxWin;
	};
}


