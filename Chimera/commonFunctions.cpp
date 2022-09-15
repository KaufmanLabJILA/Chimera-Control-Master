#pragma once

#include "stdafx.h"
#include "resource.h"
#include <array>
#include "commonFunctions.h"
#include "TextPromptDialog.h"
//#include "NiawgController.h"
#include "experimentThreadInputStructure.h"
//#include "scriptWriteHelpProc.h"
#include "beginningSettingsDialogProc.h"
#include "openWithExplorer.h"
#include "saveWithExplorer.h"
#include "MainWindow.h"
#include "CameraWindow.h"
#include "AuxiliaryWindow.h"
#include <boost/filesystem.hpp>

// Functions called by all windows to do the same thing, mostly things that happen on menu presses.
namespace commonFunctions
{

	bool masterAborted;

	// this function handles messages that all windows can recieve, e.g. accelerator keys and menu messages. It 
	// redirects everything to all of the other functions below, for the most part.
	void handleCommonMessage( int msgID, CWnd* parent, MainWindow* mainWin, ScriptingWindow* scriptWin, 
							  CameraWindow* camWin, AuxiliaryWindow* auxWin )
	{
		switch (msgID)
		{
			case ID_FILE_RUN_FOLDER:
			{
				CWinThread* multiExperimentThread;

				std::string filepath = scriptWin->openMasterScriptFolder(parent);
				if (filepath == "") {
					break;
				}
				std::string delimiter = "\\";
				std::vector<std::string> filepathparts;

				size_t pos = 0;
				std::string token;
				while ((pos = filepath.find(delimiter)) != std::string::npos) {
					token = filepath.substr(0, pos);
					filepathparts.push_back(token);
					filepath.erase(0, pos + delimiter.length());
				}
				std::string folderpath = "";
				for (int i = 0; i < filepathparts.size(); i++) {
					folderpath += filepathparts[i] + "\\";
				}
				mainWin->getComm()->sendStatus("going to run all scripts the selected folder:" + folderpath + "\r\n");

				std::vector<std::string> master_scripts;

				for (const auto & file : boost::filesystem::directory_iterator(folderpath)) {
					//mainWin->getComm()->sendStatus(file.path().string() + "\r\n");
					std::string filepathstr = file.path().string();
					if (filepathstr.substr(filepathstr.find_last_of(".") + 1) == "mScript") {
						master_scripts.push_back(filepathstr);
						mainWin->getComm()->sendStatus(filepathstr + "\r\n");
					}
				}
				MultiExperimentInput* multiInput = new MultiExperimentInput;
				multiInput->auxWin = auxWin;
				multiInput->camWin = camWin;
				multiInput->mainWin = mainWin;
				multiInput->scriptWin = scriptWin;
				multiInput->parent = parent;
				multiInput->msgID = msgID;
				multiInput->master_scripts = master_scripts;
				multiExperimentThread = AfxBeginThread(multipleExperimentThreadProcedure, multiInput, THREAD_PRIORITY_HIGHEST);
				Sleep(3);

				break;
			}
			case ID_FILE_RUN_EVERYTHING:
			case ID_ACCELERATOR_F5:
			case ID_FILE_MY_WRITE_WAVEFORMS:
			{
				ExperimentInput input;
				camWin->redrawPictures(false);

				try
				{
					prepareCamera( mainWin, camWin, input );
					prepareMasterThread( msgID, scriptWin, mainWin, camWin, auxWin, input, false, true, true );
					camWin->preparePlotter(input);
					camWin->prepareAtomCruncher(input);


					logParameters( input, camWin, true );

					if (!ATOMCRUNCHER_SAFEMODE) {
						camWin->startAtomCruncher(input);
					}
					camWin->startPlotterThread(input);
					camWin->startCamera();
					if (input.masterInput->settings.saveMakoImages) {
					}
					startMaster( mainWin, input );
				}
				catch (Error& err)
				{
					if (err.whatBare() == "CANCEL")
					{
						mainWin->getComm()->sendStatus("Canceled camera initialization.\r\n");
						mainWin->getComm()->sendColorBox( Niawg, 'B' );
						break;
					}
					mainWin->getComm()->sendError("EXITED WITH ERROR! " + err.whatStr());
					mainWin->getComm()->sendColorBox( Camera, 'R' );
					mainWin->getComm()->sendStatus("EXITED WITH ERROR!\r\nInitialized Default Waveform\r\n");
					mainWin->getComm()->sendTimer("ERROR!");
					camWin->assertOff();
					break;
				}
				break;
			}
			case WM_CLOSE:
			case ID_ACCELERATOR_ESC:
			case ID_FILE_ABORT_GENERATION:
			{
				std::string status;
				if ( mainWin->experimentIsPaused( ) )
				{
					mainWin->getComm( )->sendError( "Experiment is paused. Please unpause before aborting.\r\n" );
					break;
				}
				bool andorAborted = false;
				masterAborted = false;

				mainWin->stopRearranger( );
				camWin->wakeRearranger( );

				try
				{
					//
					if ( mainWin->masterThreadManager.runningStatus( ) )
					{
						status = "MASTER";
						commonFunctions::abortMaster( mainWin, auxWin );
						masterAborted = true;
					}
					mainWin->getComm( )->sendColorBox( Master, 'B' );
					camWin->assertOff( );
				}
				catch ( Error& err )
				{
					mainWin->getComm( )->sendError( "Abort Master thread exited with Error! Error Message: " + err.whatStr( ) );
					mainWin->getComm( )->sendColorBox( Master, 'R' );
					mainWin->getComm( )->sendStatus( "Abort Master thread exited with Error!\r\n" );
					mainWin->getComm( )->sendTimer( "ERROR!" );
				}

				try
				{
					if ( camWin->Andor.isRunning( ) )
					{
						status = "ANDOR";
						commonFunctions::abortCamera( camWin, mainWin );
						andorAborted = true;
					}
					mainWin->getComm( )->sendColorBox( Camera, 'B' );
				}
				catch ( Error& err )
				{
					mainWin->getComm( )->sendError( "Abort camera threw error! Error: " + err.whatStr( ) );
					mainWin->getComm( )->sendColorBox( Camera, 'R' );
					mainWin->getComm( )->sendStatus( "Abort camera threw error\r\n" );
					mainWin->getComm( )->sendTimer( "ERROR!" );
				}
				//
				mainWin->waitForRearranger( );

				if (!andorAborted && !masterAborted)
				{
					mainWin->getComm()->sendError("Camera and Master were not running. Can't Abort.\r\n");
				}
				break;
			}
			case ID_RUNMENU_RUNCAMERA:
			{
				ExperimentInput input;
				mainWin->getComm()->sendColorBox( Camera, 'Y' );
				mainWin->getComm()->sendStatus("Starting Camera...\r\n");
				try
				{
					commonFunctions::prepareCamera( mainWin, camWin, input );
					camWin->preparePlotter( input );
					camWin->prepareAtomCruncher( input );
					//
					commonFunctions::logParameters( input, camWin, true);
					//
					if (!ATOMCRUNCHER_SAFEMODE) {
						camWin->startAtomCruncher(input);
					}
					camWin->startPlotterThread( input );
					camWin->startCamera();
					mainWin->getComm()->sendColorBox( Camera, 'G' );
					mainWin->getComm()->sendStatus("Camera is Running.\r\n");
				}
				catch (Error& exception)
				{
					if (exception.whatBare() == "CANCEL")
					{
						mainWin->getComm()->sendColorBox( Camera, 'B' );
						mainWin->getComm()->sendStatus("Camera is Not Running, User Canceled.\r\n");
						break;
					}
					mainWin->getComm()->sendColorBox( Camera, 'R' );
					mainWin->getComm()->sendError("EXITED WITH ERROR! " + exception.whatStr());
					mainWin->getComm()->sendStatus("EXITED WITH ERROR!\r\nInitialized Default Waveform\r\n");
					mainWin->getComm()->sendTimer("ERROR!");
					camWin->assertOff();
					break;
				}
				try
				{
					commonFunctions::logParameters( input, camWin, true);
				}
				catch (Error& err)
				{
					errBox( "Data Logging failed to start up correctly! " + err.whatStr() );
					mainWin->getComm()->sendError( "EXITED WITH ERROR! " + err.whatStr() );
				}
				break;
			}
			case ID_RUNMENU_RUNMOOG:
			{
				ExperimentInput input;
				try
				{
					commonFunctions::prepareMasterThread(ID_RUNMENU_RUNMASTER, scriptWin, mainWin, camWin, auxWin,
						input, false, true, false);
					//
					//commonFunctions::logParameters(input, camWin, false);
					//
					commonFunctions::startMaster(mainWin, input);
				}
				catch (Error& except)
				{
					//mainWin->getComm()->sendColorBox(Moog, 'R');
					mainWin->getComm()->sendError("EXITED WITH ERROR! " + except.whatStr());
					mainWin->getComm()->sendStatus("EXITED WITH ERROR!\r\nInitialized Default Waveform\r\n");
				}
				break;
			}
			case ID_ACCELERATOR_SINGLESHOT:
			case ID_RUNMENU_RUNSINGLESHOT:
			{
				//auxWin->configVariables.dialog.close();
				mainWin->profile.saveConfigurationOnly(scriptWin, mainWin, auxWin, camWin);
				ExperimentInput input;
				try
				{
					commonFunctions::prepareMasterThread(ID_RUNMENU_RUNMASTER, scriptWin, mainWin, camWin, auxWin,
						input, true, true, true);
					commonFunctions::startMaster(mainWin, input);
				}
				catch (Error& err)
				{
					if (err.whatBare() == "Canceled!")
					{
						break;
					}
					mainWin->getComm()->sendColorBox(Master, 'R');
					mainWin->getComm()->sendError("EXITED WITH ERROR! " + err.whatStr());
					mainWin->getComm()->sendStatus("EXITED WITH ERROR!\r\n");
				}
				//try
				//{
				//	commonFunctions::logParameters(input, camWin, false);
				//}
				//catch (Error& err)
				//{
				//	errBox("Data Logging failed to start up correctly! " + err.whatStr());
				//	mainWin->getComm()->sendError("EXITED WITH ERROR! " + err.whatStr());
				//}
				break;
			}
			case ID_ACCELERATOR_F4:
			case ID_RUNMENU_RUNMASTER:
			{
				ExperimentInput input;  
				try
				{
					commonFunctions::prepareMasterThread( ID_RUNMENU_RUNMASTER, scriptWin, mainWin, camWin, auxWin, 
														  input, false, true, true );
					commonFunctions::startMaster( mainWin, input );
				}
				catch (Error& err)
				{
					if (err.whatBare() == "Canceled!")
					{
						break;
					}
					mainWin->getComm()->sendColorBox( Master, 'R' );
					mainWin->getComm()->sendError( "EXITED WITH ERROR! " + err.whatStr() );
					mainWin->getComm()->sendStatus( "EXITED WITH ERROR!\r\n" );
				}
				try
				{
					//commonFunctions::logParameters( input, camWin, false );
				}
				catch (Error& err)
				{
					errBox( "Data Logging failed to start up correctly! " + err.whatStr() );
					mainWin->getComm()->sendError( "EXITED WITH ERROR! " + err.whatStr() );
				}
				break;
			}
			case ID_RUNMENU_ABORTMASTER:
			{
				if ( mainWin->experimentIsPaused( ) )
				{
					mainWin->getComm( )->sendError( "Experiment is paused. Please unpause before aborting.\r\n" );
					break;
				}
				commonFunctions::abortMaster(mainWin, auxWin);
				break;
			}
			/// File Management 
			case ID_ACCELERATOR_CTRL_S:
			case ID_FILE_SAVEALL:
			{
				try
				{
					//scriptWin->saveMoogScript();
					scriptWin->saveAWGScript();
					scriptWin->saveGmoogScript();
					scriptWin->saveDdsScript();

					try {
						scriptWin->saveMasterFunction_nocatch();
					}
					catch (Error& err) {
						scriptWin->saveMasterScript();
					}

					mainWin->profile.saveEntireProfile( scriptWin, mainWin, auxWin, camWin );
					mainWin->masterConfig.save( mainWin, auxWin, camWin );
					
				}
				catch ( Error& err )
				{
					mainWin->getComm( )->sendError( err.what( ) );
				}
				break;
			}
			case ID_PROFILE_SAVE_PROFILE:
			{
				mainWin->profile.saveEntireProfile(scriptWin, mainWin, auxWin, camWin);
				break;
			}
			case ID_FILE_MY_EXIT:
			{
				try
				{
					commonFunctions::exitProgram(scriptWin, mainWin, camWin, auxWin);
				}
				catch (Error& err)
				{
					mainWin->getComm()->sendError("ERROR! " + err.whatStr());
				}
				break;
			}
			case ID_PLOTTING_STOPPLOTTER:
			{
				camWin->stopPlotter( );
				break;
			}
			case ID_FILE_MY_AWG_NEW:
			{
				scriptWin->newAWGScript();
				break;
			}
			case ID_FILE_MY_AWG_OPEN:
			{
				scriptWin->openAWGScript(parent);
				break;
			}
			case ID_FILE_MY_AWG_SAVE:
			{
				scriptWin->saveAWGScript();
				break;
			}
			case ID_FILE_MY_AWG_SAVEAS:
			{
				scriptWin->saveAWGScriptAs(parent);
				break;
			}
			case ID_FILE_MY_GIGAMOOG_NEW:
			{
				scriptWin->newGmoogScript();
				break;
			}
			case ID_FILE_MY_GIGAMOOG_OPEN:
			{
				scriptWin->openGmoogScript(parent);
				break;
			}
			case ID_FILE_MY_GIGAMOOG_SAVE:
			{
				scriptWin->saveGmoogScript();
				break;
			}
			case ID_FILE_MY_GIGAMOOG_SAVEAS:
			{
				scriptWin->saveGmoogScriptAs(parent);
				break;
			}
			case ID_FILE_MY_DDS_NEW:
			{
				scriptWin->newDdsScript();
				break;
			}
			case ID_FILE_MY_DDS_OPEN:
			{
				scriptWin->openDdsScript(parent);
				break;
			}
			case ID_FILE_MY_DDS_SAVE:
			{
				scriptWin->saveDdsScript();
				break;
			}
			case ID_FILE_MY_DDS_SAVEAS:
			{
				scriptWin->saveDdsScriptAs(parent);
				break;
			}
			case ID_MASTERSCRIPT_NEW:
			{
				scriptWin->newMasterScript();
				break;
			}
			case ID_MASTERSCRIPT_SAVE:
			{
				scriptWin->saveMasterScript();
				break;
			}
			case ID_MASTERSCRIPT_SAVEAS:
			{
				scriptWin->saveMasterScriptAs(parent);
				break;
			}
			case ID_MASTERSCRIPT_OPENSCRIPT:
			{
				scriptWin->openMasterScript(parent);
				break;
			}
			case ID_MASTERSCRIPT_NEWFUNCTION:
			{
				scriptWin->newMasterFunction();
				break;
			}
			case ID_MASTERSCRIPT_SAVEFUNCTION:
			{
				scriptWin->saveMasterFunction();
				break;
			}
			case ID_SEQUENCE_RENAMESEQUENCE:
			{
				mainWin->profile.renameSequence();
				break;
			}
			case ID_SEQUENCE_ADD_TO_SEQUENCE:
			{
				mainWin->profile.addToSequence(parent);
				break;
			}
			case ID_SEQUENCE_SAVE_SEQUENCE:
			{
				mainWin->profile.saveSequence();
				break;
			}
			case ID_SEQUENCE_NEW_SEQUENCE:
			{
				mainWin->profile.newSequence(parent);
				break;
			}
			case ID_SEQUENCE_RESET_SEQUENCE:
			{
				mainWin->profile.loadNullSequence();
				break;
			}
			case ID_SEQUENCE_DELETE_SEQUENCE:
			{
				mainWin->profile.deleteSequence();
				break;
			}
			case ID_HELP_GENERALINFORMATION:
			{
				break;
			}
			//case ID_NIAWG_RELOADDEFAULTWAVEFORMS:
			//{
			//	commonFunctions::reloadNIAWGDefaults(mainWin);
			//	break;
			//}
			/*
			case ID_EXPERIMENT_NEW_EXPERIMENT_TYPE:
			{
				mainWin->profile.newExperiment();
				break;
			}
			case ID_EXPERIMENT_SAVEEXPERIMENTSETTINGS:
			{
				mainWin->profile.saveExperimentOnly(mainWin);
				break;
			}
			case ID_EXPERIMENT_SAVEEXPERIMENTSETTINGSAS:
			{
				mainWin->profile.saveExperimentAs(mainWin);
				break;
			}

			case ID_EXPERIMENT_RENAME_CURRENT_EXPERIMENT:
			{
				mainWin->profile.renameExperiment(mainWin);
				break;
			}
			case ID_EXPERIMENT_DELETE_CURRENT_EXPERIMENT:
			{
				mainWin->profile.deleteExperiment();
				break;
			}
			case ID_CATEGORY_NEW_CATEGORY:
			{
				mainWin->profile.newCategory();
				break;
			}
			case ID_CATEGORY_RENAME_CURRENT_CATEGORY:
			{
				mainWin->profile.renameCategory();
				break;
			}
			case ID_CATEGORY_DELETE_CURRENT_CATEGORY:
			{
				mainWin->profile.deleteCategory();
				break;
			}
			case ID_CATEGORY_SAVECATEGORYSETTINGS:
			{
				mainWin->profile.saveCategoryOnly(mainWin);
				break;
			}
			case ID_CATEGORY_SAVECATEGORYSETTINGSAS:
			{
				mainWin->profile.saveCategoryAs(mainWin);
				break;
			}*/
			case ID_CONFIGURATION_NEW_CONFIGURATION:
			{
				mainWin->profile.newConfiguration(mainWin, auxWin, camWin, scriptWin);
				break;
			}
			case ID_CONFIGURATION_RENAME_CURRENT_CONFIGURATION:
			{
				mainWin->profile.renameConfiguration();
				break;
			}
			case ID_CONFIGURATION_DELETE_CURRENT_CONFIGURATION:
			{
				mainWin->profile.deleteConfiguration();
				break;
			}
			case ID_CONFIGURATION_SAVE_CONFIGURATION_AS:
			{
				mainWin->profile.saveConfigurationAs(scriptWin, mainWin, auxWin);
				break;
			}
			case ID_CONFIGURATION_SAVECONFIGURATIONSETTINGS:
			{
				mainWin->profile.saveConfigurationOnly(scriptWin, mainWin, auxWin, camWin);
				break;
			}
			//case ID_NIAWG_SENDSOFTWARETRIGGER:
			//{
			//	mainWin->niawg.fgenConduit.sendSoftwareTrigger();
			//	break;
			//}
			//case ID_NIAWG_STREAMWAVEFORM:
			//{
			//	mainWin->niawg.streamWaveform();
			//	break;
			//}
			//case ID_NIAWG_GETNIAWGERROR:
			//{
			//	errBox(mainWin->niawg.fgenConduit.getErrorMsg());
			//	break;
			//}
			case ID_PICTURES_AUTOSCALEPICTURES:
			{
				camWin->handleAutoscaleSelection();
				break;
			}
			case ID_PICTURES_GREATER_THAN_MAX_SPECIAL:
			{
				camWin->handleSpecialGreaterThanMaxSelection();
				break;
			}
			case ID_PICTURES_LESS_THAN_MIN_SPECIAL:
			{
				camWin->handleSpecialLessThanMinSelection();
				break;
			}
			case ID_PICTURES_ALWAYSSHOWGRID:
			{
				camWin->passAlwaysShowGrid();
				break;
			}
			//case ID_NIAWG_NIAWGISON:
			//{
			//	mainWin->passNiawgIsOnPress( );
			//	break;
			//}
			case ID_RUNMENU_ABORTCAMERA:
			{
				try
				{
					if (camWin->Andor.isRunning())
					{
						commonFunctions::abortCamera(camWin, mainWin);
					}
					else
					{
						mainWin->getComm()->sendError("Camera was not running. Can't Abort.\r\n");
					}
					mainWin->getComm()->sendColorBox( Camera, 'B' );
					camWin->assertOff();
				}
				catch (Error& except)
				{
					mainWin->getComm()->sendError("EXITED WITH ERROR! " + except.whatStr());
					mainWin->getComm()->sendColorBox( Camera, 'R' );
					mainWin->getComm()->sendStatus("EXITED WITH ERROR!\r\nInitialized Default Waveform\r\n");
					mainWin->getComm()->sendTimer("ERROR!");
				}
				break;
			}
			//case ID_RUNMENU_ABORTNIAWG:
			//{
			//	try
			//	{
			//		if (mainWin->niawg.niawgIsRunning())
			//		{
			//			abortRearrangement( mainWin, camWin );
			//			commonFunctions::abortNiawg(scriptWin, mainWin);
			//		}
			//		else
			//		{
			//			mainWin->getComm()->sendError("NIAWG was not running. Can't Abort.\r\n");
			//		}
			//		mainWin->getComm()->sendColorBox( Niawg, 'B' );
			//	}
			//	catch (Error& except)
			//	{
			//		mainWin->getComm()->sendError("EXITED WITH ERROR! " + except.whatStr());
			//		mainWin->getComm()->sendColorBox( Niawg, 'R' );
			//		mainWin->getComm()->sendStatus("EXITED WITH ERROR!\r\nInitialized Default Waveform\r\n");
			//		mainWin->getComm()->sendTimer("ERROR!");
			//	}
			//	break;
			//}
			case ID_MASTERCONFIG_SAVEMASTERCONFIGURATION:
			{

				mainWin->masterConfig.save(mainWin, auxWin, camWin);
				break;
			}
			case ID_MASTERCONFIGURATION_RELOAD_MASTER_CONFIG:
			{
				mainWin->masterConfig.load(mainWin, auxWin, camWin);
				break;
			}
			case ID_MASTER_VIEWORCHANGEINDIVIDUALDACSETTINGS:
			{
				auxWin->ViewOrChangeDACNames();
				break;
			}
			case ID_MASTER_VIEWORCHANGETTLNAMES:
			{
				auxWin->ViewOrChangeTTLNames();
				break;
			}
			case ID_ACCELERATOR_F2:
			case ID_RUNMENU_PAUSE:
			{
				mainWin->handlePause();
				break;
			}
			case ID_ACCELERATOR_F1:
			{
				MasterThreadInput* input = new MasterThreadInput;
				auxWin->loadMotSettings( input );
				mainWin->fillMotInput( input );
				mainWin->startMaster(input, true);
				break;
			}
			default:
				errBox("ERROR: Common message passed but not handled!");
		}
	}


	void prepareCamera( MainWindow* mainWin, CameraWindow* camWin, ExperimentInput& input, bool prompt )
	{
		camWin->redrawPictures( false );
		mainWin->getComm()->sendTimer( "Starting..." );
		camWin->prepareCamera( input );
		std::string msg = camWin->getStartMessage();
		if (prompt) {
			int answer = promptBox( msg, MB_OKCANCEL );
			if (answer == IDCANCEL)
			{
				// user doesn't want to start the camera.
				thrower( "CANCEL" );
			}
		}
		input.includesCameraRun = true;
	}

	UINT __cdecl multipleExperimentThreadProcedure(void* voidInput) {

		MultiExperimentInput* multiExpInput = (MultiExperimentInput*)voidInput;
		ExperimentInput input;

		std::string runningMultiExperiment = "\r\n\r\nYou are going to run multiple experiments. I hope this dialog helps.";

		/*INT_PTR delayDialog;
		delayDialog = DialogBoxParam(NULL, MAKEINTRESOURCE(IDD_BEGINNING_SETTINGS), 0,
				beginningSettingsDialogProc, (LPARAM)cstr(runningMultiExperiment));*/

		for (int i = 0; i < multiExpInput->master_scripts.size(); i++) {
			if (masterAborted) {
				break;
			}
			multiExpInput->camWin->redrawPictures(false);
			bool opened_script = multiExpInput->scriptWin->openMasterScriptByPath(multiExpInput->master_scripts[i]);
			if (opened_script == 1) {
				multiExpInput->mainWin->getComm()->sendError("failed to load script: " + multiExpInput->master_scripts[i]);
				break;
			}
			try
			{
				Sleep(0.5);
				multiExpInput->mainWin->profile.saveConfigurationOnly(multiExpInput->scriptWin, multiExpInput->mainWin, multiExpInput->auxWin, multiExpInput->camWin);
				Sleep(0.5);
				prepareCamera(multiExpInput->mainWin, multiExpInput->camWin, input, false);
				Sleep(0.5);
				prepareMasterThread(multiExpInput->msgID, multiExpInput->scriptWin, multiExpInput->mainWin, multiExpInput->camWin, multiExpInput->auxWin, 
					input, false, true, true, false);
				Sleep(0.5);
				multiExpInput->camWin->preparePlotter(input);
				multiExpInput->camWin->prepareAtomCruncher(input);

				logParameters(input, multiExpInput->camWin, true);

				multiExpInput->camWin->startAtomCruncher(input);
				multiExpInput->camWin->startPlotterThread(input);
				multiExpInput->camWin->startCamera();
				startMaster(multiExpInput->mainWin, input);
			}
			catch (Error& err)
			{
				if (err.whatBare() == "CANCEL")
				{
					multiExpInput->mainWin->getComm()->sendStatus("Canceled camera initialization.\r\n");
					multiExpInput->mainWin->getComm()->sendColorBox(Niawg, 'B');
					break;
				}
				multiExpInput->mainWin->getComm()->sendError("EXITED WITH ERROR! " + err.whatStr());
				multiExpInput->mainWin->getComm()->sendColorBox(Camera, 'R');
				multiExpInput->mainWin->getComm()->sendStatus("EXITED WITH ERROR!\r\nInitialized Default Waveform\r\n");
				multiExpInput->mainWin->getComm()->sendTimer("ERROR!");
				multiExpInput->camWin->assertOff();
				break;
			}
			multiExpInput->mainWin->masterThreadManager.runningThread->m_bAutoDelete = FALSE;
			multiExpInput->mainWin->masterThreadManager.runningThread->ResumeThread();
			WaitForSingleObject(multiExpInput->mainWin->masterThreadManager.runningThread->m_hThread, INFINITE);
			Sleep(0.5);
			delete multiExpInput->mainWin->masterThreadManager.runningThread;
			multiExpInput->mainWin->masterThreadManager.runningThread = 0;
			Sleep(0.5);
		}
		delete multiExpInput;
		return false;
	}


	void prepareMasterThread( int msgID, ScriptingWindow* scriptWin, MainWindow* mainWin, CameraWindow* camWin, 
		AuxiliaryWindow* auxWin, ExperimentInput& input, bool single, bool runAWG, bool runTtls, bool prompt )
	{
		Communicator* comm = mainWin->getComm();
		profileSettings profile = mainWin->getProfileSettings();
		//if (mainWin->niawgIsRunning())
		//{
		//	mainWin->getComm( )->sendColorBox( Niawg, 'R' );
		//	thrower( "ERROR: Please Restart the niawg before running an experiment.\r\n" );
		//}

		if (profile.sequenceConfigNames.size() == 0)
		{
			//mainWin->getComm()->sendColorBox( Niawg, 'R' );
			thrower( "ERROR: No configurations in current sequence! Please set some configurations to run in this "
					 "sequence or set the null sequence.\r\n" );
		}
		// check config settings
		mainWin->checkProfileReady();
		scriptWin->checkScriptSaves( );
		std::string beginInfo = "Current Settings:\r\n=============================\r\n\r\n";
		if (runAWG)
		{
			scriptInfo<std::string> scriptNames = scriptWin->getScriptNames();
			// ordering matters here, make sure you get the correct script name.
			std::string awgNameString(scriptNames.awg);
	
			std::string sequenceInfo = "";
			if (sequenceInfo != "")
			{
				beginInfo += sequenceInfo;
			}
			else
			{
				scriptInfo<bool> scriptSavedStatus = scriptWin->getScriptSavedStatuses();
			
				beginInfo += "AWG Script Name:........ " + str(awgNameString);
		
				if (scriptSavedStatus.awg)
				{
					beginInfo += " SAVED\r\n";
				}
				else
				{
					beginInfo += " NOT SAVED\r\n";
				}
			
			}
			beginInfo += "\r\n";
		}


		std::vector<variableType> vars = auxWin->getAllVariables();
		if (vars.size() == 0)
		{
			beginInfo += "Variable Names:.............. NO VARIABLES\r\n";
		}
		else
		{
			beginInfo += "Variable Names:.............. ";
			for (UINT varInc = 0; varInc < vars.size(); varInc++)
			{
				beginInfo += vars[varInc].name + " ";
			}
			beginInfo += "\r\n";
		}
		
		mainOptions settings = mainWin->getMainOptions();
		std::string beginQuestion = "\r\n\r\nBegin Waveform Generation with these Settings?";

		INT_PTR areYouSure = 0;
		if (!single && prompt)
		{
			areYouSure = DialogBoxParam(NULL, MAKEINTRESOURCE(IDD_BEGINNING_SETTINGS), 0,
				beginningSettingsDialogProc, (LPARAM)cstr(beginInfo + beginQuestion));
		}
		if (areYouSure == 0)
		{
			if (runAWG)
			{
				mainWin->getComm()->sendStatus("Performing Initial Analysis and Writing and Loading Non-Varying Waveforms to Moog...\r\n");
				//mainWin->getComm()->sendColorBox(Moog, 'Y');
			}
			// Set the thread structure.
			input.masterInput = new MasterThreadInput();
			input.masterInput->runSingle = single;
			input.masterInput->runMaster = runTtls;
			input.masterInput->skipNext = camWin->getSkipNextAtomic( );
			// force accumulations to zero. This shouldn't affect anything, this should always get set by the master or be infinite.
			if (msgID == ID_FILE_MY_WRITE_WAVEFORMS)
			{
				input.masterInput->settings.dontActuallyGenerate = true;
			}
			else
			{
				input.masterInput->settings.dontActuallyGenerate = false;
			}
			input.masterInput->debugOptions = mainWin->getDebuggingOptions();
			input.masterInput->comm = mainWin->getComm();
			input.masterInput->profile = profile;

			input.masterInput->runAWG = runAWG;
			if (runAWG) {
				scriptInfo<std::string> addresses = scriptWin->getScriptAddresses();
				//mainWin->setMoogRunningState(true);
			}

			input.masterInput->multipleExperiments = input.multipleExperiments;

			// Start the programming thread.
			auxWin->fillMasterThreadInput( input.masterInput );
			mainWin->fillMasterThreadInput( input.masterInput );
			camWin->fillMasterThreadInput( input.masterInput );
			scriptWin->fillMasterThreadInput( input.masterInput );
			mainWin->updateStatusText( "debug", beginInfo );
		}
		else
		{
			thrower("Canceled!");
		}
	}

	void startMaster(MainWindow* mainWin, ExperimentInput& input)
	{
		mainWin->addTimebar( "main" );
		mainWin->addTimebar( "error" );
		mainWin->addTimebar( "debug" );
		mainWin->startMaster( input.masterInput, false );
	}

	void abortCamera( CameraWindow* camWin, MainWindow* mainWin )
	{
		if (!camWin->cameraIsRunning())
		{
			//mainWin->getComm()->sendColorBox( Niawg, 'B' );
			mainWin->getComm()->sendError( "System was not running. Can't Abort.\r\n" );
			return;
		}
		std::string errorMessage;
		// abort acquisition if in progress
		camWin->abortCameraRun();
		mainWin->getComm()->sendStatus( "Aborted Camera Operation.\r\n" );
		// todo: here handle data closing as well....? 
	}


	//void abortNiawg( ScriptingWindow* scriptWin, MainWindow* mainWin )
	//{
	//	Communicator* comm = mainWin->getComm();
	//	// set reset flag
	//	eAbortNiawgFlag = true;
	//	if (!mainWin->niawgIsRunning())
	//	{
	//		std::string msgString = "Passively Outputting Default Waveform.";
	//		comm->sendColorBox( Niawg, 'B' );
	//		comm->sendError( "System was not running. Can't Abort.\r\n" );
	//		return;
	//	}
	//	// wait for reset to occur
	//	int result = 1;
	//	result = WaitForSingleObject( eNIAWGWaitThreadHandle, 0 );
	//	if (result == WAIT_TIMEOUT)
	//	{
	//		// try again. Hopefully gives the main thread to handle other messages first if this happens.
	//		mainWin->PostMessageA( WM_COMMAND, MAKEWPARAM( ID_FILE_ABORT_GENERATION, 0 ) );
	//		return;
	//	}
	//	eAbortNiawgFlag = false;
	//	// abort the generation on the NIAWG.
	//	//scriptWin->setIntensityDefault();
	//	comm->sendStatus( "Aborted NIAWG Operation. Passively Outputting Default Waveform.\r\n" );
	//	comm->sendColorBox( Niawg, 'B' );
	//	mainWin->restartNiawgDefaults();
	//	mainWin->setNiawgRunningState( false );
	//}

	void abortMaster( MainWindow* mainWin, AuxiliaryWindow* auxWin )
	{
		mainWin->abortMasterThread();
		auxWin->handleAbort();
	}


	void exitProgram( ScriptingWindow* scriptWindow, MainWindow* mainWin, CameraWindow* camWin, AuxiliaryWindow* auxWin )
	{
		if (camWin->cameraIsRunning())
		{
			thrower( "The Camera is Currently Running. Please stop the system before exiting so that devices devices "
					 "can stop normally." );
		}
		if (mainWin->masterIsRunning())
		{
			thrower( "The Master system (ttls & dacs) is currently running. Please stop the system before exiting so "
					 "that devices can stop normally." );
		}
		scriptWindow->checkScriptSaves( );
		mainWin->checkProfileSave();
		std::string exitQuestion = "Are you sure you want to exit?\n\nThis will stop all output of the arbitrary waveform generator.";
		int areYouSure = promptBox(exitQuestion, MB_OKCANCEL);
		if (areYouSure == IDOK)
		{
			/// Exiting		
			auxWin->EndDialog( 0 );
			camWin->EndDialog( 0 );
			scriptWindow->EndDialog( 0 );
			mainWin->EndDialog( 0 );
			PostQuitMessage( 1 );
		}
	}


	//void reloadNIAWGDefaults( MainWindow* mainWin )
	//{
	//	profileSettings profile = mainWin->getProfileSettings();
	//	if (mainWin->niawgIsRunning())
	//	{
	//		thrower( "The system is currently running. You cannot reload the default waveforms while the system is "
	//				 "running. Please restart the system before attempting to reload default waveforms." );
	//	}
	//	int choice = promptBox("Reload the default waveforms from (presumably) updated files? Please make sure that "
	//							"the updated files are syntactically correct, or else the program will crash.",
	//							MB_OKCANCEL );
	//	if (choice == IDCANCEL)
	//	{
	//		return;
	//	}
	//	try
	//	{
	//		mainWin->setNiawgDefaults();
	//		mainWin->restartNiawgDefaults();
	//	}
	//	catch (Error& exception)
	//	{
	//		mainWin->restartNiawgDefaults();
	//		thrower( "ERROR: failed to reload the niawg default waveforms! Error message: " + exception.whatStr() );
	//	}
	//	mainWin->getComm()->sendStatus( "Reloaded Default Waveforms.\r\nInitialized Default Waveform.\r\n" );
	//}

	void setMot(MainWindow* mainWin)
	{
		MasterThreadInput* input = new MasterThreadInput;
		input->quiet = true;
		
	}

	void logParameters( ExperimentInput& input, CameraWindow* camWin, bool takeAndorPictures )
	{
		DataLogger* logger = camWin->getLogger();
		logger->initializeDataFiles();
		if (!ANDOR_SAFEMODE) {
			logger->logAndorSettings(input.camSettings, takeAndorPictures, input.cruncherInput->nMask);
		}
		logger->logMasterParameters( input.masterInput );
		logger->logAWGParameters(input.masterInput);
		logger->logMiscellaneous();
		//logger->closeFile(); //TODO: May have to remove this once andor is integrated.
	}

	void abortRearrangement( MainWindow* mainWin, CameraWindow* camWin )
	{

	}
};
