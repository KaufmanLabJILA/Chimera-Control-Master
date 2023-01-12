#pragma once

#include "stdafx.h"
#include "ProfileSystem.h"
#include "DebugOptionsControl.h"
#include "MainOptionsControl.h"
#include "StatusControl.h"
#include "StatusIndicator.h"
#include "Communicator.h"
#include "SmsTextingControl.h"
#include "EmbeddedPythonHandler.h"
#include "MasterConfiguration.h"
#include "Repetitions.h"
#include "MasterManager.h"
#include "commonFunctions.h"
#include "DataLogger.h"
#include "NoteSystem.h"
#include "profileSettings.h"
//#include "NiawgController.h"
//#include "rerngControl.h"

//megamoog controller
#include "SerialSynth.h"
#include "DDS_SYNTH.h"
#include "GigaMoog.h"
#include "fpgaAWG.h"


class ScriptingWindow;
class CameraWindow;
class AuxiliaryWindow;

class MainWindow : public CDialog
{
	using CDialog::CDialog;
	DECLARE_DYNAMIC(MainWindow);
	public:
	    // overrides
		void passNiawgIsOnPress( );
		void passGmoogIsOnPress( );
		void passAutoAlignIsOnPress();
		void passPainterIsOnPress();
		void passExportArrayIsOnPress();
		void passExportVariableIsOnPress();
		bool checkGmoogState();
		bool checkAutoAlignState();
		bool checkPainterState();
		bool checkExportArrayState();
		MainWindow(UINT id, CDialog*);
		BOOL OnInitDialog() override;
		HBRUSH OnCtlColor(CDC* pDC, CWnd* pWnd, UINT nCtlColor);
		BOOL PreTranslateMessage(MSG* pMsg); 
		void OnSize(UINT nType, int cx, int cy);
		void OnClose();
		void catchEnter( );
		void OnCancel() override;
		// stuff directly called (or 1 simple step removed) by message map.
		LRESULT onRepProgress(WPARAM wParam, LPARAM lParam);
		LRESULT onStatusTextMessage(WPARAM wParam, LPARAM lParam);
		LRESULT onErrorMessage(WPARAM wParam, LPARAM lParam);
		LRESULT onFatalErrorMessage(WPARAM wParam, LPARAM lParam);
		LRESULT onNormalFinishMessage(WPARAM wParam, LPARAM lParam);
		LRESULT onColoredEditMessage(WPARAM wParam, LPARAM lParam);
		LRESULT onDebugMessage(WPARAM wParam, LPARAM lParam);		
		LRESULT onNoAtomsAlertMessage( WPARAM wp, LPARAM lp);
		//
		void stopRearranger( );
		void waitForRearranger( );
		void passCommonCommand( UINT id );
		void handlePause();
		void passDebugPress( UINT id );
		void passMainOptionsPress( UINT id );
		void handleDblClick( NMHDR * pNotifyStruct, LRESULT * result );
		void handleRClick( NMHDR * pNotifyStruct, LRESULT * result );
		void handleSequenceCombo();
		void passClear( UINT id );

		// auxiliary functions used by the window.
		void setNotes(std::string notes);
		void setNiawgDefaults();
		void fillMasterThreadInput( MasterThreadInput* input );
		void fillMotInput( MasterThreadInput* input);
		void startMaster( MasterThreadInput* input, bool isTurnOnMot , bool waitTillFinished = false);
		std::string getNotes( );
		brushMap getBrushes();
		rgbMap getRgbs();
		fontMap getFonts();
		profileSettings getProfileSettings();
		debugInfo getDebuggingOptions();
		mainOptions getMainOptions();

		void checkProfileReady();
		void checkSequenceReady();
		void checkProfileSave();
		bool checkConfigurationSave();
		void updateConfigurationSavedStatus(bool status);
		void saveConfiguration();

		void setDebuggingOptions(debugInfo options);
		void updateStatusText(std::string whichStatus, std::string text);
		void addTimebar(std::string whichStatus);
		void setShortStatus(std::string text);
		void changeShortStatusColor(std::string color);
		void restartNiawgDefaults();
		void stopNiawg();
		void changeBoxColor(systemInfo<char> colors);
		void handleNewConfig( std::ofstream& newFile );
		void handleSaveConfig(std::ofstream& saveFile);
		void handleOpeningConfig(std::ifstream& configFile, int versionMajor, int versionMinor );
		void abortMasterThread();
		Communicator* getComm();
		std::string getSystemStatusString();

		bool niawgIsRunning();
		bool masterIsRunning();
		void setNiawgRunningState( bool newRunningState );
		RunInfo getRunInfo();
		void handleFinish();
		UINT getRepNumber();
		void logParams( DataLogger* logger, MasterThreadInput* input );
		bool experimentIsPaused( );
		void notifyConfigUpdate( );
		void passConfigPress( );
		void passOpenSeqPress();
		void passAddConfigsToSeqPress();
		void passAddCurrentConfigToSeqPress();
		void changeConfig(std::string pathToConfig);
		bool sequenceIsRunning = false;

	private:		
		DECLARE_MESSAGE_MAP();
		ScriptingWindow* TheScriptingWindow;
		CameraWindow* TheCameraWindow;
		AuxiliaryWindow* TheAuxiliaryWindow;
		// members that have gui elements
		ProfileSystem profile;
		MasterConfiguration masterConfig;
		NoteSystem notes;
		DebugOptionsControl debugger;
		Repetitions repetitionControl;
		MainOptionsControl settings;
		StatusControl mainStatus;
		StatusControl debugStatus;
		StatusControl errorStatus;
		SmsTextingControl texter;
		StatusIndicator shortStatus;
		bool exportVariables = false;
		
		//rerngControl rearrangeControl;
		//NiawgController niawg;
		//SerialSynth moog;
		fpgaAWG awg;
		gigaMoog gmoog;
		DDS_SYNTH dds;

		ColorBox boxes;
		// auxiliary members
		Communicator comm;
		brushMap mainBrushes;
		rgbMap mainRGBs;
		fontMap mainFonts;
		MasterManager masterThreadManager;
		CMenu menu;
		RunInfo systemRunningInfo;
		cToolTips tooltips;
		EmbeddedPythonHandler python;
		//KeyHandler masterKey;

		static BOOL CALLBACK monitorHandlingProc( _In_ HMONITOR hMonitor, _In_ HDC      hdcMonitor,
										   _In_ LPRECT   lprcMonitor, _In_ LPARAM   dwData );

		// friends (try to minimize these)
		friend void commonFunctions::handleCommonMessage( int msgID, CWnd* parent, MainWindow* mainWin,
														  ScriptingWindow* scriptWin, CameraWindow* camWin,
														  AuxiliaryWindow* masterWin );
		CDialog* appSplash;
};

