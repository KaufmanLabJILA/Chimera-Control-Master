#include "stdafx.h"
#include "EDacSystem.h"
#include "AuxiliaryWindow.h"
#include <string>
// for other ni stuff
#include "nidaqmx2.h"
#include <boost/filesystem.hpp>
#include <experimental/filesystem>
#include <fstream>
#include "nidaqmx2.h"
namespace fs = std::experimental::filesystem;
//#include <vector>

// for other ni stuff

EDacSystem::EDacSystem()
{
	
}

void EDacSystem::setEDacCommandForm( EDacCommandForm command )
{
	edacCommandFormList.push_back(command);
	// you need to set up a corresponding trigger to tell the dacs to change the output at the correct time. 
	// This is done later on interpretation of ramps etc.
}

void EDacSystem::handleEDacScriptCommand( EDacCommandForm command, std::vector<variableType>& vars)
{
	if ( command.commandName != "edac:")
	{
		thrower( "ERROR: edac commandName not recognized!" );
	}
	setEDacCommandForm(command);
    
}

void EDacSystem::interpretKey( std::vector<variableType>& variables, std::string& warnings )
{
	UINT variations;
	variations = variables.front( ).keyValues.size( );
	if (variations == 0)
	{
		variations = 1;
	}
	/// imporantly, this sizes the relevant structures.
	edacCommandList = std::vector<std::vector<EDacCommand>> (variations);
	edacSnapshots = std::vector<std::vector<EDacSnapshot>> (variations);
	finalEDacSnapshots = std::vector<std::vector<EDacChannelSnapshot>> (variations);
	loadSkipEDacSnapshots = std::vector<std::vector<EDacSnapshot>>( variations );
	//finalFormatEDacData = std::vector<std::array<std::vector<double>,8>>( variations );
    finalFormatEDacData = std::vector<std::array<double,8>> ( variations );
	loadSkipEDacFinalFormat = std::vector<std::array<std::vector<double>,8>>( variations );
	for (UINT variationInc = 0; variationInc < variations; variationInc++)
	{
		for (UINT eventInc = 0; eventInc < edacCommandFormList.size(); eventInc++)
		{
			EDacCommand tempEvent;
			tempEvent.line = edacCommandFormList[eventInc].line;
			// Deal with time.
			if (edacCommandFormList[eventInc].time.first.size() == 0)
			{
				// no variable portion of the time.
				tempEvent.time = edacCommandFormList[eventInc].time.second;
			}
			else
			{
				double varTime = 0;
				for (auto variableTimeString : edacCommandFormList[eventInc].time.first)
				{
					varTime += variableTimeString.evaluate( variables, variationInc );
				}
				tempEvent.time = varTime + edacCommandFormList[eventInc].time.second;
			}

			if ( edacCommandFormList[eventInc].commandName == "edac:")
			{
				/// single point.
				////////////////
				// deal with value
				tempEvent.edacVoltageValue1 = edacCommandFormList[eventInc].edacVoltageValue1.evaluate( variables, variationInc );
                tempEvent.edacVoltageValue2 = edacCommandFormList[eventInc].edacVoltageValue2.evaluate( variables, variationInc );
                tempEvent.edacVoltageValue3 = edacCommandFormList[eventInc].edacVoltageValue3.evaluate( variables, variationInc );
                tempEvent.edacVoltageValue4 = edacCommandFormList[eventInc].edacVoltageValue4.evaluate( variables, variationInc );
                tempEvent.edacVoltageValue5 = edacCommandFormList[eventInc].edacVoltageValue5.evaluate( variables, variationInc );
                tempEvent.edacVoltageValue6 = edacCommandFormList[eventInc].edacVoltageValue6.evaluate( variables, variationInc );
                tempEvent.edacVoltageValue7 = edacCommandFormList[eventInc].edacVoltageValue7.evaluate( variables, variationInc );
                tempEvent.edacVoltageValue8 = edacCommandFormList[eventInc].edacVoltageValue8.evaluate( variables, variationInc );
				edacCommandList[variationInc].push_back(tempEvent);
			}
			
			else
			{
				thrower( "ERROR: Unrecognized edac command name: " + edacCommandFormList[eventInc].commandName);
			}
		}
	}
}

// void EDacSystem::findLoadSkipSnapshots( double time, std::vector<variableType>& variables, UINT variation )
// {
// 	// find the splitting time and set the loadSkip snapshots to have everything after that time.
// 	//TODO: add error handling for no DAC commands here (if size dacsnapshots ==0 throw)
//     //Do not know if this function is needed here. If someone can figure out, great! :))) (Actually :()

// 	if (edacSnapshots[variation].size()==0)
// 	{
// 		thrower("DAC snapshots not initialized. Try adding a dummy DAC setting to the master script.");
// 	}
// 	for ( auto snapshotInc : range( edacSnapshots[variation].size( ) - 1 ) )
// 	{
// 		if (edacSnapshots[variation][snapshotInc].time < time && edacSnapshots[variation][snapshotInc + 1].time >= time )
// 		{
// 			loadSkipEDacSnapshots[variation] = std::vector<EDacSnapshot>( edacSnapshots[variation].begin( ) 
// 																		+ snapshotInc + 1,
// 																		edacSnapshots[variation].end( ) );
// 			break;
// 		}
// 	}
// }

// void EDacSystem::makeFinalDataFormat(UINT variation)
// {
// 	for ( auto& data : finalFormatEDacData[variation] )
// 	{
// 		data.clear( );
// 	}
// 	for ( auto& data : loadSkipEDacFinalFormat[variation] )
// 	{
// 		data.clear( );
// 	}
// 	for (EDacSnapshot snapshot : edacSnapshots[variation])
// 	{
//         finalFormatEDacData[variation][0].push_back(snapshot.edacVoltageValue1);
//         finalFormatEDacData[variation][1].push_back(snapshot.edacVoltageValue2);
//         finalFormatEDacData[variation][2].push_back(snapshot.edacVoltageValue3);
//         finalFormatEDacData[variation][3].push_back(snapshot.edacVoltageValue4);
//         finalFormatEDacData[variation][4].push_back(snapshot.edacVoltageValue5);
//         finalFormatEDacData[variation][5].push_back(snapshot.edacVoltageValue6);
//         finalFormatEDacData[variation][6].push_back(snapshot.edacVoltageValue7);
//         finalFormatEDacData[variation][7].push_back(snapshot.edacVoltageValue8);
// 	}
// 	// same loop for load skip
// 	for ( EDacSnapshot snapshot : loadSkipEDacSnapshots[variation] )
// 	{
// 		   loadSkipEDacFinalFormat[variation][0].push_back( snapshot.edacVoltageValue1);
//         loadSkipEDacFinalFormat[variation][1].push_back( snapshot.edacVoltageValue2);
//         loadSkipEDacFinalFormat[variation][2].push_back( snapshot.edacVoltageValue3);
//         loadSkipEDacFinalFormat[variation][3].push_back( snapshot.edacVoltageValue4);
//         loadSkipEDacFinalFormat[variation][4].push_back( snapshot.edacVoltageValue5);
//         loadSkipEDacFinalFormat[variation][5].push_back( snapshot.edacVoltageValue6);
//         loadSkipEDacFinalFormat[variation][6].push_back( snapshot.edacVoltageValue7);
//         loadSkipEDacFinalFormat[variation][7].push_back( snapshot.edacVoltageValue8);
// 	}

// 	;
// }

void EDacSystem::getEDacFinalData(UINT variation)
{
    finalFormatEDacData[variation][0] = edacCommandList[variation][0].edacVoltageValue1;
    finalFormatEDacData[variation][1] = edacCommandList[variation][0].edacVoltageValue2;
    finalFormatEDacData[variation][2] = edacCommandList[variation][0].edacVoltageValue3;
    finalFormatEDacData[variation][3] = edacCommandList[variation][0].edacVoltageValue4;
    finalFormatEDacData[variation][4] = edacCommandList[variation][0].edacVoltageValue5;
    finalFormatEDacData[variation][5] = edacCommandList[variation][0].edacVoltageValue6;
    finalFormatEDacData[variation][6] = edacCommandList[variation][0].edacVoltageValue7;
    finalFormatEDacData[variation][7] = edacCommandList[variation][0].edacVoltageValue8;
}

void EDacSystem::checkTimingsWork(UINT variation)
{
	std::vector<double> times;
	// grab all the times.
	for (auto snapshot : edacSnapshots[variation])
	{
		times.push_back(snapshot.time);
	}

	int count = 0;
	for (auto time : times)
	{
		int countInner = 0;
		for (auto secondTime : times)
		{
			// don't check against itself.
			if (count == countInner)
			{
				countInner++;
				continue;
			}
			// // can't trigger faster than the trigger time.
			// if (fabs(time - secondTime) < edacTriggerTime)
			// {
			// 	thrower("ERROR: timings are such that the dac system would have to get triggered too fast to follow the"
			// 			" programming! ");
			// }
			countInner++;
		}
		count++;
	}
}

void EDacSystem::updateEDACFile(UINT variation)
	{
		std::string tmpFile = EDAC_START_FILE_LOCATION + ".tmp";
		int pyStarted = 1;
		std::fstream pyStartFile(tmpFile, std::fstream::out);
		std::string edacVoltageValues;
        double edacVoltageValue;
		// edacVoltageValues = arrayToString(edacVoltageValue,8);
        
        for ( UINT edacInc = 0; edacInc < 8; edacInc++ )
	    {
            edacVoltageValue = finalFormatEDacData[variation][edacInc];
            std::string tempVal = edacVoltageValues + ',' + std::to_string(edacVoltageValue);
            edacVoltageValues = tempVal;
	    }
		if (pyStartFile.is_open())
		{
			pyStartFile << edacVoltageValues + "\r\n";
			pyStartFile.close();
			std::string newEDAC_START_FILE_LOCATION;
			fs::rename(tmpFile, EDAC_START_FILE_LOCATION);
			pyStarted = 0;
		}
	}

// void EDacSystem::writeEDacs(UINT variation, bool loadSkip)
// {

// 	if (getNumberEvents(variation) != 0) {
// 		try
// 		{
			
// 		}
// 		catch (Error& err)
// 		{
// 			errBox(err.what());
// 		}

// 		if (tcp_connect == 0)
// 		{
// 			zynq_tcp.writeDACs(finalEDacSnapshots[variation]);
// 			zynq_tcp.disconnect();
// 		}
// 		else
// 		{
// 			errBox("connection to zynq failed. can't write DAC data\n");
// 		}
// 	}
// }





