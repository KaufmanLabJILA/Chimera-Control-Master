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
#include "constants.h"
#include <fstream>
#include <cstdio>
#include <experimental/filesystem>
#include <windows.h>
#include <string>
using namespace boost::posix_time;
namespace fs = std::experimental::filesystem;

// Functions called by all windows to do the same thing, mostly things that happen on menu presses.
namespace commonFunctions
{
	// this function handles messages that all windows can recieve, e.g. accelerator keys and menu messages. It 
	// redirects everything to all of the other functions below, for the most part.
	void handleCommonMessage( int msgID, CWnd* parent, MainWindow* mainWin, ScriptingWindow* scriptWin, 
							  CameraWindow* camWin, AuxiliaryWindow* auxWin )
	{
		switch (msgID)
		{
			case ID_FILE_RUN_EVERYTHING:
			case ID_ACCELERATOR_F5:
			case ID_FILE_MY_WRITE_WAVEFORMS:
			{
				ExperimentInput input;
				camWin->redrawPictures(false);
				try
				{
					prepareCamera( mainWin, camWin, input, false );
					prepareMasterThread( msgID, scriptWin, mainWin, camWin, auxWin, input, false, true, true, false );
					camWin->preparePlotter(input);
					camWin->prepareAtomCruncher(input);

					logParameters( input, camWin, true );

					camWin->startAtomCruncher(input);
					camWin->startPlotterThread(input);
					camWin->startCamera();
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
			case ID_RUNMENU_RUNSEQUENCE:
			case ID_ACCELERATOR_F6:
			{
				mainWin->sequenceIsRunning = true;
				CWinThread* multiExperimentThread;
				MultiExperimentInput* input = new MultiExperimentInput;
				input->auxWin = auxWin;
				input->camWin = camWin;
				input->mainWin = mainWin;
				input->scriptWin = scriptWin;
				input->parent = parent;
				input->msgID = msgID;
				multiExperimentThread = AfxBeginThread(multipleExperimentThreadProcedure, input, THREAD_PRIORITY_HIGHEST);
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
				bool andorAborted = false, masterAborted = false;

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

				mainWin -> sequenceIsRunning = false;

				//try
				//{
				//	if ( mainWin->niawg.niawgIsRunning( ) )
				//	{
				//		status = "NIAWG";
				//		abortNiawg( scriptWin, mainWin );
				//		niawgAborted = true;
				//	}
				//	mainWin->getComm( )->sendColorBox( Niawg, 'B' );
				//}
				//catch ( Error& err )
				//{
				//	mainWin->getComm( )->sendError( "Abort NIAWG exited with Error! Error Message: " + err.whatStr( ) );
				//	if ( status == "NIAWG" )
				//	{
				//		mainWin->getComm( )->sendColorBox( Niawg, 'R' );
				//	}
				//	mainWin->getComm( )->sendStatus( "EXITED WITH ERROR!\r\nInitialized Default Waveform\r\n" );
				//	mainWin->getComm( )->sendTimer( "ERROR!" );
				//}
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
					commonFunctions::prepareCamera( mainWin, camWin, input, false );
					camWin->preparePlotter( input );
					camWin->prepareAtomCruncher( input );
					//
					commonFunctions::logParameters( input, camWin, true);
					//
					camWin->startAtomCruncher( input );
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
			//case ID_RUNMENU_RUNNIAWG:
			//{
			//	ExperimentInput input;
			//	try
			//	{
			//		commonFunctions::prepareMasterThread( ID_RUNMENU_RUNMASTER, scriptWin, mainWin, camWin, auxWin,
			//											  input, false, true, false );
			//		//
			//		commonFunctions::logParameters( input, camWin, false );
			//		//
			//		commonFunctions::startMaster( mainWin, input );
			//	}
			//	catch (Error& except)
			//	{
			//		mainWin->getComm()->sendColorBox( Niawg, 'R' );
			//		mainWin->getComm()->sendError("EXITED WITH ERROR! " + except.whatStr());
			//		mainWin->getComm()->sendStatus("EXITED WITH ERROR!\r\nInitialized Default Waveform\r\n");
			//	}
			//	try
			//	{
			//		//???
			//	}
			//	catch (Error& err)
			//	{
			//		errBox( "Data Logging failed to start up correctly! " + err.whatStr() );
			//		mainWin->getComm()->sendError( "EXITED WITH ERROR! " + err.whatStr() );
			//	}
			//	break;
			//}
			case ID_RUNMENU_RUNMOOG:
			{
				ExperimentInput input;
				try
				{
					commonFunctions::prepareMasterThread(ID_RUNMENU_RUNMASTER, scriptWin, mainWin, camWin, auxWin,
						input, false, true, false, false);
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
						input, true, true, true, false);
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
														  input, false, true, true, false );
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
					//scriptWin->saveHorizontalScript( );
					//scriptWin->saveVerticalScript( );
					//scriptWin->saveMoogScript();
					scriptWin->saveAWGScript();
					scriptWin->saveGmoogScript();
					//scriptWin->saveIntensityScript( );
					scriptWin->saveDdsScript();

					try {
						scriptWin->saveMasterFunction_nocatch();
					}
					catch (Error& err) {
						scriptWin->saveMasterScript();
					}

					//auxWin->updateAgilent( TopBottom );
					//auxWin->updateAgilent( Axial );
					//auxWin->updateAgilent( Flashing );
					//auxWin->updateAgilent( Microwave );
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
			/*case ID_FILE_MY_INTENSITY_NEW:
			{
				scriptWin->newIntensityScript();
				break;
			}
			case ID_FILE_MY_INTENSITY_OPEN:
			{
				scriptWin->openIntensityScript(parent);
				break;
			}
			case ID_FILE_MY_INTENSITY_SAVE:
			{
				scriptWin->saveIntensityScript();
				break;
			}
			case ID_FILE_MY_INTENSITY_SAVEAS:
			{
				scriptWin->saveIntensityScriptAs(parent);
				break;
			}*/
			/*case ID_TOP_BOTTOM_NEW_SCRIPT:
			{
				auxWin->newAgilentScript(TopBottom);
				break;
			}
			case ID_TOP_BOTTOM_OPEN_SCRIPT:
			{
				auxWin->openAgilentScript( TopBottom, parent );
				break;
			}
			case ID_TOP_BOTTOM_SAVE_SCRIPT:
			{
				auxWin->saveAgilentScript(TopBottom);
				break;
			}
			case ID_TOP_BOTTOM_SAVE_SCRIPT_AS:
			{
				auxWin->saveAgilentScriptAs( TopBottom, parent );
				break;
			}
			case ID_AXIAL_NEW_SCRIPT:
			{
				auxWin->newAgilentScript(Axial);
				break;
			}
			case ID_AXIAL_OPEN_SCRIPT:
			{
				auxWin->openAgilentScript( Axial, parent );
				break;
			}
			case ID_AXIAL_SAVE_SCRIPT:
			{
				auxWin->saveAgilentScript(Axial);
				break;
			}
			case ID_AXIAL_SAVE_SCRIPT_AS:
			{
				auxWin->saveAgilentScriptAs( Axial, parent );
				break;
			}
			case ID_FLASHING_NEW_SCRIPT:
			{
				auxWin->newAgilentScript( Flashing );
				break;
			}
			case ID_FLASHING_OPEN_SCRIPT:
			{
				auxWin->openAgilentScript( Flashing, parent );
				break;
			}
			case ID_FLASHING_SAVE_SCRIPT:
			{
				auxWin->saveAgilentScript( Flashing );
				break;
			}
			case ID_FLASHING_SAVE_SCRIPT_AS:
			{
				auxWin->saveAgilentScriptAs( Flashing, parent );
				break;
			}
			case ID_UWAVE_NEW_SCRIPT:
			{
				auxWin->newAgilentScript( Microwave );
				break;
			}
			case ID_UWAVE_OPEN_SCRIPT:
			{
				auxWin->openAgilentScript( Microwave, parent );
				break;
			}
			case ID_UWAVE_SAVE_SCRIPT:
			{
				auxWin->saveAgilentScript( Microwave );
				break;
			}
			case ID_UWAVE_SAVE_SCRIPT_AS:
			{
				auxWin->saveAgilentScriptAs( Microwave, parent );
				break; 
			}*/
			/*case ID_FILE_MY_VERTICAL_NEW:
			{
				scriptWin->newVerticalScript();
				break;
			}
			case ID_FILE_MY_VERTICAL_OPEN:
			{
				scriptWin->openVerticalScript(parent);
				break;
			}
			case ID_FILE_MY_VERTICAL_SAVE:
			{
				scriptWin->saveVerticalScript();
				break;
			}
			case ID_FILE_MY_VERTICAL_SAVEAS:
			{
				scriptWin->saveVerticalScriptAs(parent);
				break;
			}*/
			//case ID_FILE_MY_MOOG_NEW:
			//{
			//	scriptWin->newMoogScript();
			//	break;
			//}
			//case ID_FILE_MY_MOOG_OPEN:
			//{
			//	scriptWin->openMoogScript(parent);
			//	break;
			//}
			//case ID_FILE_MY_MOOG_SAVE:
			//{
			//	scriptWin->saveMoogScript();
			//	break;
			//}
			//case ID_FILE_MY_MOOG_SAVEAS:
			//{
			//	scriptWin->saveMoogScriptAs(parent);
			//	break;
			//}
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
				mainWin->profile.saveSequence(mainWin);
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
			case ID_SEQUENCE_SAVE_SEQUENCEAS:
			{
				mainWin->profile.saveSequenceAs(mainWin);
				break;
			}
			case ID_SEQUENCE_OPEN_SEQUENCE:
			{
				mainWin->profile.openSequenceFile(parent,mainWin);
				break;
			}
			case ID_SEQUENCE_REMOVE_CONFIGS:
			{
				mainWin->profile.removeConfigFromSequence();
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
			case ID_GIGAMOOG_REARRANGERACTIVE:
			{
				mainWin->passGmoogIsOnPress( );
				break;
			}
			case ID_GIGAMOOG_AUTOTWEEZERALIGNACTIVE:
			{
				mainWin->passAutoAlignIsOnPress();
				break;
			}
			case ID_GIGAMOOG_PAINTERACTIVE:
			{
				mainWin->passPainterIsOnPress();
				break;
			}
			case ID_GIGAMOOG_EXPORTARRAY:
			{
				mainWin->passExportArrayIsOnPress();
				break;
			}
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


	void prepareCamera( MainWindow* mainWin, CameraWindow* camWin, ExperimentInput& input, bool isSequence )
	{
		camWin->redrawPictures( false );
		mainWin->getComm()->sendTimer( "Starting..." );
		camWin->prepareCamera( input );
		std::string msg = camWin->getStartMessage();
		if (!isSequence)
		{
			int answer = promptBox(msg, MB_OKCANCEL);
			if (answer == IDCANCEL)
			{
				// user doesn't want to start the camera.
				thrower("CANCEL");
			}
		}
		input.includesCameraRun = true;
	}


	// Sequentially runs configurations in a sequence. At each iteration, the mainWindow opens the given configuration
	// of the sequence and a new thread is started to execute. The multiExperimentThread will wait for this thread to
	// terminate before loading and starting the next configuration
	UINT __cdecl multipleExperimentThreadProcedure(void* voidInput) {

		MultiExperimentInput* multiExpInput = (MultiExperimentInput*)voidInput;
		ExperimentInput input;
		profileSettings profileSeq = multiExpInput->mainWin->getProfileSettings();
		std::vector<std::string> configSequence = profileSeq.sequenceConfigNames;
		std::vector<std::string> pathSequence = profileSeq.sequenceConfigPaths;
		std::vector<bool> axPhaseFlags = profileSeq.axLatPhaseFlags;
		bool abortSequence = false;
		
		//Set default behavior of rearrangement and auto-align for non-calibration runs
		bool defaultAutoAlign = multiExpInput->mainWin->checkAutoAlignState();
		bool defaultGmoog = multiExpInput->mainWin->checkGmoogState();
		bool defaultPainter = multiExpInput->mainWin->checkPainterState();
		bool defaultExportArray = multiExpInput->mainWin->checkExportArrayState();
		std::string defaultAutoAlignStr, defaultGmoogStr, defaultPainterStr, defaultExportArrayStr;
		if (defaultAutoAlign)
		{defaultAutoAlignStr = "ON";}
		else
		{defaultAutoAlignStr = "OFF";}
		if (defaultGmoog)
		{defaultGmoogStr = "ON";}
		else
		{defaultGmoogStr = "OFF";}
		if (defaultPainter)
		{defaultPainterStr = "ON";}
		else
		{defaultPainterStr = "OFF";}
		if (defaultExportArray)
		{defaultExportArrayStr = "ON";}
		else
		{defaultExportArrayStr = "OFF";}

		// Confirm sequence running with user once at the start
		UINT_PTR areYouSure = 0;
		if (!areYouSure)
		{
			areYouSure = DialogBoxParam(NULL, MAKEINTRESOURCE(IDD_BEGINNING_SETTINGS), 0,
				beginningSettingsDialogProc, (LPARAM)cstr("\r\n\r\nBegin Sequence Execution?\n\r\n\rDefault Auto-align State: " + defaultAutoAlignStr +	
					"\n\rDefault Rearranger State: " + defaultGmoogStr));
		}
		
		if (areYouSure == 0) // Run sequence if user responds ok
		{
			multiExpInput->mainWin->sequenceIsRunning = true;
			for (UINT configInc = 0; configInc < configSequence.size(); configInc++)
			{
				// Update the configuration
				std::string dateStr;
				std::string configPath = pathSequence[configInc] + configSequence[configInc];
				multiExpInput->mainWin->changeConfig(configPath);

				// if experiment aborted, then abort sequence
				if (!multiExpInput->mainWin->sequenceIsRunning)
				{
					multiExpInput->mainWin->getComm()->sendStatus("EXITED SEQUENCE!\r\n");
					break;
				}

				if (axPhaseFlags[configInc])
				{
					// turn gigamoog totally off for axial phase
					if (multiExpInput->mainWin->checkAutoAlignState())
					{
						multiExpInput->mainWin->passAutoAlignIsOnPress();
					}
					if (multiExpInput->mainWin->checkGmoogState())
					{
						multiExpInput->mainWin->passGmoogIsOnPress();
					}
					if (multiExpInput->mainWin->checkPainterState())
					{
						multiExpInput->mainWin->passPainterIsOnPress();
					}
					if (multiExpInput->mainWin->checkExportArrayState())
					{
						multiExpInput->mainWin->passExportArrayIsOnPress();
					}

				}

				multiExpInput->camWin->redrawPictures(false);
				
				try // execute the experiment
				{
					prepareCamera(multiExpInput->mainWin, multiExpInput->camWin, input, true);
					prepareMasterThread(multiExpInput->msgID, multiExpInput->scriptWin, multiExpInput->mainWin, multiExpInput->camWin, multiExpInput->auxWin, input, false, true, true, true);
					multiExpInput->camWin->preparePlotter(input);
					multiExpInput->camWin->prepareAtomCruncher(input);

					dateStr = logParameters(input, multiExpInput->camWin, true);

					multiExpInput->camWin->startAtomCruncher(input);
					multiExpInput->camWin->startPlotterThread(input);
					multiExpInput->camWin->startCamera();
					startMaster(multiExpInput->mainWin, input, true); // true argument sets wait for experimentThread to finish
					if (multiExpInput->mainWin->sequenceIsRunning)
					{
						multiExpInput->mainWin->getComm()->sendStatus("Completed running sequence configuration " + str(configInc + 1) + ":\r\n" + configSequence[configInc] + "\r\n\r\n");
					}
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
					abortSequence = true;
					break;
				}

				// turn gigamoog to default state
				if (multiExpInput->mainWin->checkAutoAlignState() != defaultAutoAlign)
				{
					multiExpInput->mainWin->passAutoAlignIsOnPress();
				}
				if (multiExpInput->mainWin->checkGmoogState() !=  defaultGmoog)
				{
					multiExpInput->mainWin->passGmoogIsOnPress();
				}
				if (multiExpInput->mainWin->checkPainterState() != defaultPainter)
				{
					multiExpInput->mainWin->passPainterIsOnPress();
				}
				if (multiExpInput->mainWin->checkExportArrayState() != defaultExportArray)
				{
					multiExpInput->mainWin->passExportArrayIsOnPress();
				}

				// if experiment aborted, then abort sequence
				if (!multiExpInput->mainWin->sequenceIsRunning)
				{
					multiExpInput->mainWin->getComm()->sendStatus("EXITED SEQUENCE!\r\n");
					break;
				}

				// handling for certain configurations
				if (axPhaseFlags[configInc])
				{	
					std::string runType = "axial_phase";
					int pyStarted = sendPythonInitializationFile(runType, dateStr);
					if (pyStarted == 0)
					{
						multiExpInput->mainWin->getComm()->sendStatus("Python start file created. Waiting for response...\r\n");
						bool foundPythonUpdate = watchPythonUpdate(multiExpInput->mainWin);
						if (foundPythonUpdate) 
						{
							updateGlobalVars(multiExpInput->mainWin,multiExpInput->auxWin);
						}
					}
					else
					{
						multiExpInput->mainWin->getComm()->sendStatus("FAILED to create Python start file.\r\n");
					}
				}

				// if experiment aborted, then abort sequence
				if (!multiExpInput->mainWin->sequenceIsRunning)
				{
					multiExpInput->mainWin->getComm()->sendStatus("EXITED SEQUENCE!\r\n");
					break;
				}
			}
		}
		else
		{
			multiExpInput->mainWin->getComm()->sendStatus("Sequence Canceled!");
		}
		multiExpInput->mainWin->sequenceIsRunning = false;
		return 0;
	}


	void prepareMasterThread( int msgID, ScriptingWindow* scriptWin, MainWindow* mainWin, CameraWindow* camWin, 
		AuxiliaryWindow* auxWin, ExperimentInput& input,
		bool single, bool runAWG, bool runTtls, bool isSequence)
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
			//std::string moogNameString(scriptNames.moog);
			std::string awgNameString(scriptNames.awg);
			//std::string horizontalNameString( scriptNames.horizontalNIAWG );
			//std::string verticalNameString( scriptNames.verticalNIAWG );
			//std::string intensityNameString( scriptNames.intensityAgilent );
			std::string DDSNameString(scriptNames.DDS);
			std::string sequenceInfo = "";
			if (sequenceInfo != "")
			{
				beginInfo += sequenceInfo;
			}
			else
			{
				scriptInfo<bool> scriptSavedStatus = scriptWin->getScriptSavedStatuses();
				//beginInfo += "Vertical Script Name:........ " + str( verticalNameString );
				//if (scriptSavedStatus.verticalNIAWG)
				//{
				//	beginInfo += " SAVED\r\n";
				//}
				//beginInfo += "Moog Script Name:........ " + str(moogNameString);
				beginInfo += "AWG Script Name:........ " + str(awgNameString);
				//if (scriptSavedStatus.moog)
				if (scriptSavedStatus.awg)
				{
					beginInfo += " SAVED\r\n";
				}
				else
				{
					beginInfo += " NOT SAVED\r\n";
				}
				//beginInfo += "Horizontal Script Name:...... " + str( horizontalNameString );
				//if (scriptSavedStatus.horizontalNIAWG)
				//{
				//	beginInfo += " SAVED\r\n";
				//}
				//else
				//{
				//	beginInfo += " NOT SAVED\r\n";
				//}
				/*beginInfo += "Intensity Script Name:....... " + str( intensityNameString );
				if (scriptSavedStatus.intensityAgilent)
				{
					beginInfo += " SAVED\r\n";
				}
				else
				{
					beginInfo += " NOT SAVED\r\n";
				}*/
				beginInfo += "DDS Script Name:........ " + str(DDSNameString);
				
				if (scriptSavedStatus.DDS)
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
		if (!single && !isSequence)
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

	void startMaster(MainWindow* mainWin, ExperimentInput& input, bool waitTillFinished)
	{
		mainWin->addTimebar( "main" );
		mainWin->addTimebar( "error" );
		mainWin->addTimebar( "debug" );
		mainWin->startMaster( input.masterInput, false , waitTillFinished);
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

	std::string logParameters( ExperimentInput& input, CameraWindow* camWin, bool takeAndorPictures )
	{
		DataLogger* logger = camWin->getLogger();
		std::string dateStr = logger->initializeDataFiles();

		logger->logAndorSettings( input.camSettings, takeAndorPictures, input.cruncherInput->nMask);
		logger->logMasterParameters( input.masterInput );
		logger->logDDSParameters(input.masterInput);
		logger->logGmoogParameters(input.masterInput);
		logger->logAWGParameters(input.masterInput);
		logger->logMiscellaneous();
		//logger->closeFile(); //TODO: May have to remove this once andor is integrated.

		return dateStr;
	}

	void abortRearrangement( MainWindow* mainWin, CameraWindow* camWin )
	{

	}

	int sendPythonInitializationFile(std::string runType, std::string dateStr)
	{
		std::string tmpFile = PYTHON_ANALYSIS_START_FILE_LOCATION + ".tmp";
		//std::string finalFile = "C:\\Users\\alecj\\Kaufman_Lab\\dummy_data\\pyStart.txt";
		int pyStarted = 1;
		std::fstream pyStartFile(tmpFile, std::fstream::out);
		if (pyStartFile.is_open())
		{
			pyStartFile << runType + "\r\n";
			pyStartFile << dateStr + "\r\n";
			pyStartFile.close();
			fs::rename(tmpFile, PYTHON_ANALYSIS_START_FILE_LOCATION);
			pyStarted = 0;
		}
		return pyStarted;
	}

	bool watchPythonUpdate2(MainWindow* mainWin)
	{
		DWORD dwWaitStatus;
		HANDLE dwChangeHandles[1];

		dwChangeHandles[0] = FindFirstChangeNotification(PYTHON_ANALYSIS_LOCATION.c_str(),FALSE,FILE_NOTIFY_CHANGE_LAST_WRITE);

		if ((dwChangeHandles[0] == INVALID_HANDLE_VALUE)|| (dwChangeHandles[0] == INVALID_HANDLE_VALUE))
		{
			mainWin->getComm()->sendStatus("\r\nERROR watching for Python analysis update.\r\n");
			FindCloseChangeNotification(dwChangeHandles);
			//ExitProcess(GetLastError());
			return false;
		}
		else
		{
			while (TRUE)
			{
				dwWaitStatus = WaitForMultipleObjects(1,dwChangeHandles,FALSE,300000);
				FindCloseChangeNotification(dwChangeHandles);

				switch (dwWaitStatus)
				{
					case WAIT_OBJECT_0:
					{
						Sleep(100);
						mainWin->getComm()->sendStatus("\r\nFound update from Python.\r\n");
						return true;
					}
					case WAIT_TIMEOUT:
					{
						mainWin->getComm()->sendStatus("\r\nTIMEOUT. No Python analysis found after 5 minutes.\r\n");
						return false;
					}
				}
			}
		}
	}

	bool watchPythonUpdate(MainWindow* mainWin)
	{
		HANDLE file = CreateFile( PYTHON_ANALYSIS_LOCATION.c_str(), 
			FILE_LIST_DIRECTORY, FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE, 
			NULL, OPEN_EXISTING, FILE_FLAG_BACKUP_SEMANTICS | FILE_FLAG_OVERLAPPED, NULL);

		OVERLAPPED ovl { 0 };
		ovl.hEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
		//uint8_t change_buf[1024];
		//char change_buf[16384];
		std::vector<BYTE> buffer(1024 * 64);

		bool pyAnalysisFound = false;
		while (!pyAnalysisFound)
		{
			bool watchDir = ReadDirectoryChangesW(file, &buffer[0], buffer.size(), TRUE,
				FILE_NOTIFY_CHANGE_FILE_NAME, NULL, &ovl, NULL);

			DWORD result = WaitForSingleObject(ovl.hEvent, 15000);

			if (result == WAIT_OBJECT_0)
			{
				DWORD bytes_transferred;
				GetOverlappedResult(file, &ovl, &bytes_transferred, TRUE);

				FILE_NOTIFY_INFORMATION *fni = reinterpret_cast<FILE_NOTIFY_INFORMATION*>(&buffer[0]);
				while (true)
				{
					switch (fni->Action)
					{
					case FILE_ACTION_ADDED:
					{
						//mainWin->getComm()->sendStatus("\r\nFile add change detected!\r\n");
					}
					break;

					case FILE_ACTION_REMOVED:
					{
						//mainWin->getComm()->sendStatus("\r\nFile remove change detected!\r\n");
					}
					break;

					case FILE_ACTION_MODIFIED:
					{
						//mainWin->getComm()->sendStatus("\r\nFile modify change detected!\r\n");
					}
					break;

					case FILE_ACTION_RENAMED_OLD_NAME:
					{
						//mainWin->getComm()->sendStatus("\r\nRename old detected!\r\n");
					}
					break;

					case FILE_ACTION_RENAMED_NEW_NAME:
					{
						//mainWin->getComm()->sendStatus("\r\nRename new detected!\r\n");
						CString fileName(fni->FileName);
						if (fileName == "pyFinish.txt")
						{
							pyAnalysisFound = true;
							mainWin->getComm()->sendStatus("Python update detected!\r\n");
							Sleep(100);
						}
					}
					break;

					default:
					{
						//mainWin->getComm()->sendStatus("Unknown action detected!");
					}
					break;
					}

					if (fni->NextEntryOffset == 0)
					{
						break;
					}
					fni = reinterpret_cast<FILE_NOTIFY_INFORMATION*>(reinterpret_cast<BYTE*>(fni) + fni->NextEntryOffset);

					//if (event->NextEntryOffset) {
					//	*((uint8_t**)&event) += event->NextEntryOffset;
					//}
					//else
					//{
					//	break;
					//}

				}

			}
			else if (result == WAIT_TIMEOUT)
			{
				mainWin->getComm()->sendStatus("Did not find Python analysis.\r\n");
				break;
			}
		}
		return pyAnalysisFound;
	}

	void updateGlobalVars(MainWindow* mainWin, AuxiliaryWindow* auxWin)
	{
		std::fstream pyFinishFile(PYTHON_ANALYSIS_FINISH_FILE_LOCATION);
		if (pyFinishFile.is_open())
		{
			std::string variableToChange;
			std::string changeValueStr;
			std::getline(pyFinishFile, variableToChange);
			std::getline(pyFinishFile, changeValueStr);
			double changeValue = ::atof(changeValueStr.c_str());
			try
			{
				if (auxWin->globalVariables.checkVariableExists(variableToChange))
				{
					double currentValue = auxWin->globalVariables.getGlobalVariableValue(variableToChange);
					auxWin->globalVariables.changeVariableValue(variableToChange, changeValue);
					mainWin->getComm()->sendStatus("Updated " + variableToChange + " from " + std::to_string(currentValue) + " to " + changeValueStr +".\r\n");
				}
				else
				{
					throw (variableToChange);
				}
			}
			catch (std::string variableToChange)
			{
				mainWin->getComm()->sendStatus("Did not find " + variableToChange + " as a global variable.\r\n");
			}
		}
	}

};
