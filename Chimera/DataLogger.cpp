#include "stdafx.h"
#include "DataLogger.h"
#include "externals.h"
#include "longnam.h"
#include "DataAnalysisHandler.h"
#include "CameraImageDimensions.h"
#include <experimental/filesystem>
namespace fs = std::experimental::filesystem;


DataLogger::DataLogger(std::string systemLocation)
{
	// initialize this to false.
	fileIsOpen = false;
	dataFilesBaseLocation = systemLocation;
}


// this file assumes that h5 is the data_#.h5 file. User should check if incDataSet is on before calling. ???
void DataLogger::deleteFile(Communicator* comm)
{
	if (fileIsOpen)
	{
		// I'm not actually sure if this should be a prob with h5.
		thrower("ERROR: Can't delete current h5 file, the h5 file is open!");
	}
	std::string fileAddress = dataFilesBaseLocation + currentSaveFolder + "\\Raw Data\\data_"
		+ str( currentDataFileNumber ) + ".h5";
	int success = DeleteFile(cstr(fileAddress));
	if (success == false)
	{
		thrower("Failed to delete h5 file! Error code: " + str(GetLastError()) + ".\r\n");
	}
	else
	{
		comm->sendStatus( "Deleted h5 file located at \"" + fileAddress + "\"\r\n" );
	}
}


std::string DataLogger::initializeDataFiles()
{
	// if the function fails, the h5 file will not be open. If it succeeds, this will get set to true.
	fileIsOpen = false;
	/// First, create the folder for today's h5 data.
	// Get the date and use it to set the folder where this data run will be saved.
	// get time now
	
	time_t timeInt = time(0);
	struct tm timeStruct;
	localtime_s(&timeStruct, &timeInt);

	char buffer[3];

	std::string yearStr = str(timeStruct.tm_year - 100);

	sprintf_s(buffer, sizeof(buffer), "%02d", timeStruct.tm_mon + 1);
	std::string monthStr = str(buffer);

	sprintf_s(buffer, sizeof(buffer), "%02d", timeStruct.tm_mday);
	std::string dayStr = str(buffer);

	// Create the string of the date.
	std::string finalSaveFolder;
	std::string dateStr;
	finalSaveFolder = yearStr + monthStr + dayStr + "\\";
	dateStr = yearStr + monthStr + dayStr;

	// right now the save folder IS the date...
	currentSaveFolder = finalSaveFolder;
	// create date's folder.
	int result = 1;
	struct stat info;
	int resultStat = stat( cstr( dataFilesBaseLocation + finalSaveFolder ), &info );
	if (resultStat != 0)
	{
		result = CreateDirectory( cstr( dataFilesBaseLocation + finalSaveFolder ), 0 );
	}
	if (!result)
	{
		thrower( "ERROR: Failed to create save location for data at location " + dataFilesBaseLocation + finalSaveFolder +
				 ". Make sure you have access to the jilafile or change the save location. Error: " + str(GetLastError()) 
				 + "\r\n" );
	}
	resultStat = stat(cstr(DATA_SAVE_LOCATION + finalSaveFolder), &info);
	if (resultStat != 0)
	{
		result = CreateDirectory(cstr(DATA_SAVE_LOCATION + finalSaveFolder), 0);
	}
	if (!result)
	{
		thrower("ERROR: Failed to create save location for data at location " + dataFilesBaseLocation + finalSaveFolder +
			". Make sure you have access to the jilafile or change the save location. Error: " + str(GetLastError())
			+ "\r\n");
	}
	finalSaveFolder += "\\Raw Data";
	resultStat = stat( cstr( dataFilesBaseLocation + finalSaveFolder ), &info );
	if (resultStat != 0)
	{
		result = CreateDirectory( cstr( dataFilesBaseLocation + finalSaveFolder ), 0 );
	}
	if (!result)
	{
		thrower( "ERROR: Failed to create save location for data! Error: " + str( GetLastError() ) + "\r\n" );
	}
	resultStat = stat(cstr(DATA_SAVE_LOCATION + finalSaveFolder), &info);
	if (resultStat != 0)
	{
		result = CreateDirectory(cstr(DATA_SAVE_LOCATION + finalSaveFolder), 0);
	}
	if (!result)
	{
		thrower("ERROR: Failed to create save location for data! Error: " + str(GetLastError()) + "\r\n");
	}
	finalSaveFolder += "\\";
	/// Get a filename appropriate for the data
	std::string finalSaveFileName;

	// find the first data file that hasn't been already written, starting with data_1.h5
	int fileNum = 1;
	// The while condition here check if file exists. No idea how this actually works.
	struct stat statBuffer;
	// figure out the next file number
	while ((stat(cstr(DATA_SAVE_LOCATION + finalSaveFolder + "data_" + str(fileNum) + ".h5"),
		   &statBuffer) == 0))
	{
		fileNum++;
	}
	// at this point a valid filename has been found.
	finalSaveFileName = "data_" + str(fileNum) + ".h5";
	// update this, which is used later to move the key file.
	currentDataFileNumber = fileNum;
	
	try
	{
		// create the file. H5F_ACC_TRUNC means it will overwrite files with the same name.
		file = H5::H5File( cstr( dataFilesBaseLocation + finalSaveFolder + finalSaveFileName ), H5F_ACC_TRUNC );
		H5::Group ttlsGroup( file.createGroup( "/TTLs" ) );
		// initial settings
		// list of commands
		H5::Group dacsGroup( file.createGroup( "/DACs" ) );
		// initial settings
		// list of commands
		H5::Group tektronicsGroup( file.createGroup( "/Tektronics" ) );
		// mode, freq, power
		//H5::Group miscellaneousGroup( file.createGroup( "/Miscellaneous" ) );
		miscellaneousGroup = H5::Group(file.createGroup("/Miscellaneous"));

		time_t t = time( 0 );   // get time now
		struct tm now;
		localtime_s( &now, &t );
		// Keep this for backwards compatibility
		std::string dateString = str( now.tm_year + 1900 ) + "-" + str( now.tm_mon + 1 ) + "-" + str( now.tm_mday );
		writeDataSet( dateString, "Run-Date", miscellaneousGroup );
		std::string timeString = str( now.tm_hour) + ":" + str( now.tm_min) + ":" + str( now.tm_sec) + ":";
		writeDataSet( timeString, "Time-Of-Logging", miscellaneousGroup );
		// Save start date and time of the experiment
		std::string dateTimeString = str(now.tm_year + 1900) + "-" + str(now.tm_mon + 1) + "-" + str(now.tm_mday) + " " + str(now.tm_hour) + ":" + str(now.tm_min) + ":" + str(now.tm_sec);
		writeDataSet(dateTimeString, "Experiment-Start", miscellaneousGroup);

		fileIsOpen = true;
	}
	catch (H5::Exception err)
	{
		thrower( "ERROR: Failed to initialize HDF5 data file: " + err.getDetailMsg() );
	}

	return dataFilesBaseLocation + yearStr + monthStr + dayStr + "\\Raw Data\\" + finalSaveFileName;
}


//void DataLogger::logAgilentSettings( const std::vector<Agilent*>& agilents )
//{
//	H5::Group agilentsGroup( file.createGroup( "/Agilents" ) );
//	for ( auto& agilent : agilents )
//	{
//		H5::Group singleAgilent( agilentsGroup.createGroup( agilent->getName( ) ) );
//		// mode
//		deviceOutputInfo info = agilent->getOutputInfo( );
//		UINT channelCount = 1;
//		for ( auto& channel : info.channel )
//		{
//			H5::Group channelGroup( singleAgilent.createGroup( "Channel-" + str( channelCount ) ) );
//			std::string outputModeName;
//			switch ( channel.option )
//			{
//			case -2:
//				outputModeName = "No-Control";
//				break;
//			case -1:
//				outputModeName = "Output-Off";
//				break;
//			case 0:
//				outputModeName = "DC";
//				break;
//			case 1:
//				outputModeName = "Sine";
//				break;
//			case 2:
//				outputModeName = "Square";
//				break;
//			case 3:
//				outputModeName = "Preloaded-Arb";
//				break;
//			case 4:
//				outputModeName = "Scripted-Arb";
//				break;
//			}
//			writeDataSet( outputModeName, "Output-Mode", channelGroup );
//			H5::Group dcGroup( channelGroup.createGroup( "DC-Settings" ) );
//			writeDataSet( channel.dc.dcLevelInput.expressionStr, "DC-Level", dcGroup );
//			H5::Group sineGroup( channelGroup.createGroup( "Sine-Settings" ) );
//			writeDataSet( channel.sine.frequencyInput.expressionStr, "Frequency", sineGroup );
//			writeDataSet( channel.sine.amplitudeInput.expressionStr, "Amplitude", sineGroup );
//			H5::Group squareGroup( channelGroup.createGroup( "Square-Settings" ) );
//			writeDataSet( channel.square.amplitudeInput.expressionStr, "Amplitude", squareGroup );
//			writeDataSet( channel.square.frequencyInput.expressionStr, "Frequency", squareGroup );
//			writeDataSet( channel.square.offsetInput.expressionStr, "Offset", squareGroup );
//			H5::Group preloadedArbGroup( channelGroup.createGroup( "Preloaded-Arb-Settings" ) );
//			writeDataSet( channel.preloadedArb.address, "Address", preloadedArbGroup );
//			H5::Group scriptedArbSettings( channelGroup.createGroup( "Scripted-Arb-Settings" ) );
//			writeDataSet( channel.scriptedArb.fileAddress, "Script-File-Address", preloadedArbGroup );
//			// TODO: load script file itself
//			channelCount++;
//		}
//	}
//}

void DataLogger::logAndorSettings( AndorRunSettings settings, bool on, int nMask)
{
	try
	{
		if ( !on )
		{
			H5::Group andorGroup( file.createGroup( "/Andor:NA" ) );
			return;
		}
		// in principle there are some other low level settings or things that aren't used very often which I could include 
		// here. I'm gonna leave this for now though.

		H5::Group andorGroup( file.createGroup( "/Andor" ) );
		hsize_t rank1[] = { 1 };
		// pictures. These are permanent members of the class for speed during the writing process.	
		hsize_t setDims[] = { ULONGLONG( settings.totalPicsInExperiment ), settings.imageSettings.width, settings.imageSettings.height };
		hsize_t picDims[] = { 1, settings.imageSettings.width, settings.imageSettings.height }; 
		picureSetDataSpace = H5::DataSpace( 3, setDims ); //single repetition
		picDataSpace = H5::DataSpace( 3, picDims ); //single picture
		pictureDataset = andorGroup.createDataSet( "Pictures", H5::PredType::NATIVE_LONG, picureSetDataSpace ); //full experiment

		hsize_t atomArraySetDims[] = {ULONGLONG(settings.totalPicsInExperiment), nMask}; //single repetition
		hsize_t atomArrayDims[] = { 1,  nMask };
		atomArraySetDataSpace = H5::DataSpace(2, atomArraySetDims); //single repetition
		atomArrayDataSpace = H5::DataSpace(2, atomArrayDims); //single picture
		atomArrayDataset = andorGroup.createDataSet("Processed-Pictures", H5::PredType::NATIVE_LONG, atomArraySetDataSpace); //full experiment

		writeDataSet( settings.cameraMode, "Camera-Mode", andorGroup );
		writeDataSet( settings.exposureTimes, "Exposure-Times", andorGroup );
		writeDataSet( settings.triggerMode, "Trigger-Mode", andorGroup );
		writeDataSet( settings.emGainModeIsOn, "EM-Gain-Mode-On", andorGroup );
		if ( settings.emGainModeIsOn )
		{
			writeDataSet( settings.emGainLevel, "EM-Gain-Level", andorGroup );
		}
		else
		{
			writeDataSet( -1, "NA:EM-Gain-Level", andorGroup );
		}
		// image settings
		H5::Group imageDims = andorGroup.createGroup( "Image-Dimensions" );
		writeDataSet( settings.imageSettings.top, "Top", andorGroup );
		writeDataSet( settings.imageSettings.bottom, "Bottom", andorGroup );
		writeDataSet( settings.imageSettings.left, "Left", andorGroup );
		writeDataSet( settings.imageSettings.right, "Right", andorGroup );
		writeDataSet( settings.imageSettings.horizontalBinning, "Horizontal-Binning", andorGroup );
		writeDataSet( settings.imageSettings.verticalBinning, "Vertical-Binning", andorGroup );
		writeDataSet( settings.temperatureSetting, "Temperature-Setting", andorGroup );
		writeDataSet( settings.picsPerRepetition, "Pictures-Per-Repetition", andorGroup );
		writeDataSet( settings.repetitionsPerVariation, "Repetitions-Per-Variation", andorGroup );
		writeDataSet( settings.totalVariations, "Total-Variation-Number", andorGroup );
	}
	catch ( H5::Exception err )
	{
		thrower( "ERROR: Failed to log andor parameters in HDF5 file: " + err.getDetailMsg( ) );
	}
}


void DataLogger::logMasterParameters( MasterThreadInput* input )
{
	try
	{
		if ( input == NULL )
		{
			H5::Group runParametersGroup( file.createGroup( "/Master-Parameters:NA" ) );
			return;
		}
		H5::Group runParametersGroup( file.createGroup( "/Master-Parameters" ) );
		writeDataSet( input->runMaster, "Run-Master", runParametersGroup );
		writeDataSet(input->repetitionNumber, "Repetitions", runParametersGroup);
		logVariables(input->variables, runParametersGroup);

		if ( input->runMaster )
		{
			std::ifstream masterScript( input->masterScriptAddress );
			if ( !masterScript.is_open( ) )
			{
				thrower( "ERROR: Failed to load master script!" );
			}
			std::string scriptBuf( str( masterScript.rdbuf( ) ) );
			writeDataSet( scriptBuf, "Master-Script", runParametersGroup);
			writeDataSet( input->masterScriptAddress, "Master-Script-File-Address", runParametersGroup );
		}
		else
		{
			writeDataSet( "", "NA:Master-Script", runParametersGroup );
			writeDataSet( "", "NA:Master-Script-File-Address", runParametersGroup );
		}
		logFunctions(runParametersGroup);
		//logNiawgSettings( input );
		//logAgilentSettings( input->agilents );
	}
	catch ( H5::Exception& err )
	{
		thrower( "ERROR: Failed to log master parameters in HDF5 file: detail:" + err.getDetailMsg( ) );
	}
}

void DataLogger::logFunctions(H5::Group& group)
{
	H5::Group funcGroup(group.createGroup("Functions"));
	try
	{
		// Re-add the entries back in and figure out which one is the current one.
		std::vector<std::string> names;
		std::string search_path = FUNCTIONS_FOLDER_LOCATION + "\\" + "*." + FUNCTION_EXTENSION;
		WIN32_FIND_DATA fd;
		HANDLE hFind;
		hFind = FindFirstFile(cstr(search_path), &fd);
		if (hFind != INVALID_HANDLE_VALUE)
		{
			do
			{
				// if looking for folders
				if (!(fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY))
				{
					names.push_back(fd.cFileName);
				}
			} while (FindNextFile(hFind, &fd));
			FindClose(hFind);
		}

		for (auto name : names)
		{
			std::ifstream functionFile(FUNCTIONS_FOLDER_LOCATION + "\\" + name);
			std::stringstream stream;
			stream << functionFile.rdbuf();
			std::string tempStr(stream.str());
			H5::Group indvFunc(funcGroup.createGroup(name));
			writeDataSet(stream.str(), name, indvFunc);
		}
	}
	catch (H5::Exception& err)
	{
		thrower("ERROR: Failed to log functions in HDF5 file: " + err.getDetailMsg());
	}
}

void DataLogger::logDDSParameters(MasterThreadInput* input)
{
	try
	{
		if (input == NULL)
		{
			H5::Group runParametersGroup(file.createGroup("/DDS-Parameters:NA"));
			return;
		}
		H5::Group runParametersGroup(file.createGroup("/DDS-Parameters"));
		writeDataSet(input->runMaster, "Run-DDS", runParametersGroup);
		if (input->runMaster)
		{
			std::ifstream ddsScript(input->ddsScriptAddress);
			if (!ddsScript.is_open())
			{
				thrower("ERROR: Failed to load DDS script!");
			}
			std::string scriptBuf(str(ddsScript.rdbuf()));
			writeDataSet(scriptBuf, "DDS-Script", runParametersGroup);
			writeDataSet(input->ddsScriptAddress, "DDS-Script-File-Address", runParametersGroup);
		}
		else
		{
			writeDataSet("", "NA:DDS-Script", runParametersGroup);
			writeDataSet("", "NA:DDS-Script-File-Address", runParametersGroup);
		}
	}
	catch (H5::Exception& err)
	{
		thrower("ERROR: Failed to log DDS parameters in HDF5 file: detail:" + err.getDetailMsg());
	}
}

void DataLogger::logGmoogParameters(MasterThreadInput* input)
{
	try
	{
		if (input == NULL)
		{
			H5::Group runParametersGroup(file.createGroup("/Gmoog-Parameters:NA"));
			return;
		}
		H5::Group runParametersGroup(file.createGroup("/Gmoog-Parameters"));
		//H5::Group tweezerAutoOffsetGroup(file.createGroup("/Tweezer-auto-offset"));

		writeDataSet(!MOOG_SAFEMODE, "Run-Gmoog", runParametersGroup);
		if (!MOOG_SAFEMODE)
		{
			std::ifstream gmoogScript(input->gmoogScriptAddress);
			if (!gmoogScript.is_open())
			{
				thrower("ERROR: Failed to load Gmoog script!");
			}
			std::string scriptBuf(str(gmoogScript.rdbuf()));
			writeDataSet(scriptBuf, "Gmoog-Script", runParametersGroup);
			writeDataSet(input->gmoogScriptAddress, "Gmoog-Script-File-Address", runParametersGroup);
			writeDataSet(input->gmoog->autoTweezerOffsetActive, "auto-offset-active", runParametersGroup);
			//writeDataSet(input->gmoog->xOffsetAuto, "x-auto-offset-MHz", tweezerAutoOffsetGroup);
			//writeDataSet(input->gmoog->yOffsetAuto, "y-auto-offset-MHz", tweezerAutoOffsetGroup);
		}
		else
		{
			writeDataSet("", "NA:Gmoog-Script", runParametersGroup);
			writeDataSet("", "NA:Gmoog-Script-File-Address", runParametersGroup);
		}
	}
	catch (H5::Exception& err)
	{
		thrower("ERROR: Failed to log Gmoog parameters in HDF5 file: detail:" + err.getDetailMsg());
	}
}

void DataLogger::logTweezerOffsets(std::vector<double> xOffsetAuto, std::vector<double> yOffsetAuto)
{
	if (fileIsOpen == false)
	{
		thrower("Tried to write to h5 file, but the file is closed!\r\n");
	}
	H5::Group tweezerAutoOffsetGroup(file.createGroup("/Tweezer-auto-offset"));
	try
	{
		writeDataSet(xOffsetAuto, "x-auto-offset-MHz", tweezerAutoOffsetGroup);
		writeDataSet(yOffsetAuto, "y-auto-offset-MHz", tweezerAutoOffsetGroup);
	}
	catch (H5::Exception& err)
	{
		thrower("Failed to write data to HDF5 file! Error: " + str(err.getDetailMsg()) + "\n");
	}
}

void DataLogger::logAWGParameters(MasterThreadInput* input)
{
	try
	{
		if (input == NULL)
		{
			H5::Group runParametersGroup(file.createGroup("/AWG-Parameters:NA"));
			return;
		}
		H5::Group runParametersGroup(file.createGroup("/AWG-Parameters"));
		writeDataSet(input->runAWG, "Run-AWG", runParametersGroup);
		if (input->runAWG)
		{
			std::ifstream awgScript(input->awgScriptAddress);
			if (!awgScript.is_open())
			{
				thrower("ERROR: Failed to load AWG script!");
			}
			std::string scriptBuf(str(awgScript.rdbuf()));
			writeDataSet(scriptBuf, "AWG-Script", runParametersGroup);
			writeDataSet(input->awgScriptAddress, "AWG-Script-File-Address", runParametersGroup);
		}
		else
		{
			writeDataSet("", "NA:AWG-Script", runParametersGroup);
			writeDataSet("", "NA:AWG-Script-File-Address", runParametersGroup);
		}
	}
	catch (H5::Exception& err)
	{
		thrower("ERROR: Failed to log AWG parameters in HDF5 file: detail:" + err.getDetailMsg());
	}
}

void DataLogger::logVariables( const std::vector<variableType>& variables, H5::Group& group )
{
	H5::Group variableGroup = group.createGroup( "Variables" );
	for ( auto& variable : variables )
	{
		H5::DataSet varSet;
		if ( variable.constant )
		{
			varSet = writeDataSet( variable.ranges.front( ).initialValue, variable.name, variableGroup );
		}
		else
		{
			varSet = writeDataSet( variable.keyValues, variable.name, variableGroup );
		}
		writeAttribute( variable.constant, "Constant", varSet );
	}
}

void DataLogger::writePic(UINT currentPictureNumber, std::vector<long> image, imageParameters dims)
{
	if (fileIsOpen == false)
	{
		thrower("Tried to write to h5 file, but the file is closed!\r\n");
	}
	// MUST initialize status
	// starting coordinates of write area in the h5 file of the array of picture data points.
	hsize_t offset[] = { currentPictureNumber-1, 0, 0 };
	hsize_t slabdim[3] = { 1, dims.width, dims.height };
	std::vector<long> im( image.size(), 0 );
	try
	{
		picureSetDataSpace.selectHyperslab( H5S_SELECT_SET, slabdim, offset );
		pictureDataset.write( image.data(), H5::PredType::NATIVE_LONG, picDataSpace, picureSetDataSpace );
	}
	catch (H5::Exception& err)
	{
		thrower("Failed to write data to HDF5 file! Error: " + str(err.getDetailMsg()) + "\n");
	}
}

void DataLogger::writeAtomArray(UINT currentPictureNumber, std::vector<UINT8> atomArray) //std::vector<bool> has different handling I don't want to deal with.
{
	if (fileIsOpen == false)
	{
		thrower("Tried to write to h5 file, but the file is closed!\r\n");
	}
	// MUST initialize status
	// starting coordinates of write area in the h5 file of the array of picture data points.
	hsize_t offset[] = { currentPictureNumber - 1, 0 };
	hsize_t slabdim[2] = { 1, atomArray.size() };
	std::vector<long> im(atomArray.size(), 0);
	try
	{
		atomArraySetDataSpace.selectHyperslab(H5S_SELECT_SET, slabdim, offset);
		atomArrayDataset.write(atomArray.data(), H5::PredType::NATIVE_UINT8, atomArrayDataSpace, atomArraySetDataSpace);
	}
	catch (H5::Exception& err)
	{
		thrower("Failed to write data to HDF5 file! Error: " + str(err.getDetailMsg()) + "\n");
	}
}


void DataLogger::logMiscellaneous()
{
	// time
}


void DataLogger::closeFile()
{
	if (!fileIsOpen)
	{
		// wasn't open.
		return;
	}

	time_t t = time(0);  // get time now
	struct tm now;
	localtime_s(&now, &t);
	// Save stop date and time of the experiment
	std::string dateTimeString = str(now.tm_year + 1900) + "-" + str(now.tm_mon + 1) + "-" + str(now.tm_mday) + " " + str(now.tm_hour) + ":" + str(now.tm_min) + ":" + str(now.tm_sec);
	writeDataSet(dateTimeString, "Experiment-Stop", miscellaneousGroup);

	miscellaneousGroup.close();
	picureSetDataSpace.close();
	pictureDataset.close();	
	atomArraySetDataSpace.close();
	atomArrayDataset.close();
	file.close();
	fileIsOpen = false;
}


UINT DataLogger::getNextFileNumber()
{
	return currentDataFileNumber+1;
}


int DataLogger::getDataFileNumber()
{
	return currentDataFileNumber;
}


//void DataLogger::logNiawgSettings(MasterThreadInput* input)
//{
//	H5::Group niawgGroup( file.createGroup( "/NIAWG" ) );
//	writeDataSet( input->runNiawg, "Run-NIAWG", niawgGroup );
//	if ( input->runNiawg )
//	{
//		niawgPair<std::vector<std::fstream>> niawgFiles;
//		std::vector<std::fstream> intensityScriptFiles;
//		ProfileSystem::openNiawgFiles( niawgFiles, input->profile, input->runNiawg );
//		std::stringstream stream;
//		stream << niawgFiles[Horizontal][0].rdbuf( );
//		writeDataSet( stream.str( ), "Horizontal-NIAWG-Script", niawgGroup );
//		stream = std::stringstream( );
//		stream << niawgFiles[Vertical][0].rdbuf( );
//		writeDataSet( stream.str( ), "Vertical-NIAWG-Script", niawgGroup );
//		writeDataSet( NIAWG_SAMPLE_RATE, "NIAWG-Sample-Rate", niawgGroup );
//		writeDataSet( NIAWG_GAIN, "NIAWG-Gain", niawgGroup );
//	}
//	else
//	{
//		writeDataSet( "", "NA:Horizontal-NIAWG-Script", niawgGroup );
//		writeDataSet( "", "NA:Vertical-NIAWG-Script", niawgGroup );
//		writeDataSet( -1, "NA:NIAWG-Sample-Rate", niawgGroup );
//		writeDataSet( -1, "NA:NIAWG-Gain", niawgGroup );
//	}
//}


H5::DataSet DataLogger::writeDataSet( bool data, std::string name, H5::Group& group )
{
	try
	{
		hsize_t rank1[] = { 1 };
		H5::DataSet dset = group.createDataSet( cstr( name ), H5::PredType::NATIVE_HBOOL, H5::DataSpace( 1, rank1 ) );
		dset.write( &data, H5::PredType::NATIVE_HBOOL );
		return dset;
	}
	catch ( H5::Exception& err )
	{
		thrower( "ERROR: error while writing bool data set to H5 File. bool was " + str( data ) 
				 + ". Dataset name was " + name + ". Error was :\r\n" + err.getDetailMsg( ) );
	}
}


H5::DataSet DataLogger::writeDataSet( ULONGLONG data, std::string name, H5::Group& group )
{
	try
	{
		hsize_t rank1[] = { 1 };
		H5::DataSet dset = group.createDataSet( cstr( name ), H5::PredType::NATIVE_ULLONG, H5::DataSpace( 1, rank1 ) );
		dset.write( &data, H5::PredType::NATIVE_ULLONG );
		return dset;
	}
	catch ( H5::Exception& err )
	{
		thrower( "ERROR: error while writing uint data set to H5 File. uint was " + str( data )
				 + ". Dataset name was " + name + ". Error was :\r\n" + err.getDetailMsg( ) );
	}
}


H5::DataSet DataLogger::writeDataSet( UINT data, std::string name, H5::Group& group )
{
	try
	{
		hsize_t rank1[] = { 1 };
		H5::DataSet dset = group.createDataSet( cstr( name ), H5::PredType::NATIVE_UINT, H5::DataSpace( 1, rank1 ) );
		dset.write( &data, H5::PredType::NATIVE_UINT );
		return dset;
	}
	catch ( H5::Exception& err )
	{
		thrower( "ERROR: error while writing uint data set to H5 File. uint was " + str( data ) 
				 + ". Dataset name was " + name + ". Error was :\r\n" + err.getDetailMsg( ) );
	}
}


H5::DataSet DataLogger::writeDataSet( int data, std::string name, H5::Group& group )
{
	try
	{
		hsize_t rank1[] = { 1 };
		H5::DataSet dset = group.createDataSet( cstr( name ), H5::PredType::NATIVE_INT, H5::DataSpace( 1, rank1 ) );
		dset.write( &data, H5::PredType::NATIVE_INT );
		return dset;
	}
	catch ( H5::Exception& err )
	{
		thrower( "ERROR: error while writing int data set to H5 File. int was " + str( data ) 
				 + ". Dataset name was " + name + ". Error was :\r\n" + err.getDetailMsg( ) );
	}
}


H5::DataSet DataLogger::writeDataSet( double data, std::string name, H5::Group& group )
{
	try
	{
		hsize_t rank1[] = { 1 };
		H5::DataSet dset = group.createDataSet( cstr( name ), H5::PredType::NATIVE_DOUBLE, H5::DataSpace( 1, rank1 ) );
		dset.write( &data, H5::PredType::NATIVE_DOUBLE );
		return dset;
	}
	catch ( H5::Exception& err )
	{
		thrower( "ERROR: error while writing double data set to H5 File. double was " + str(data) + ". Dataset name was " + name + ". Error was :\r\n" + err.getDetailMsg( ) );
	}
}


H5::DataSet DataLogger::writeDataSet( std::vector<float> dataVec, std::string name, H5::Group& group )
{
	try
	{
		hsize_t rank1[] = { 1 };
		rank1[0] = dataVec.size( );
		H5::DataSet dset = group.createDataSet( cstr( name ), H5::PredType::NATIVE_FLOAT, H5::DataSpace( 1, rank1 ) );
		// get from the key file
		dset.write( dataVec.data( ), H5::PredType::NATIVE_FLOAT );
		return dset;
	}
	catch ( H5::Exception& err )
	{
		thrower( "ERROR: error while writing double float data set to H5 File. Dataset name was " + name
				 + ". Error was :\r\n" + err.getDetailMsg( ) );
	}
}


H5::DataSet DataLogger::writeDataSet( std::vector<double> dataVec, std::string name, H5::Group& group )
{
	try
	{
		hsize_t rank1[] = { 1 };
		rank1[0] = dataVec.size( );
		H5::DataSet dset = group.createDataSet( cstr( name ), H5::PredType::NATIVE_DOUBLE, H5::DataSpace( 1, rank1 ) );
		// get from the key file
		dset.write( dataVec.data( ), H5::PredType::NATIVE_DOUBLE );
		return dset;
	}
	catch ( H5::Exception& err )
	{
		thrower( "ERROR: error while writing double vector data set to H5 File. Dataset name was " + name
				 + ". Error was :\r\n" + err.getDetailMsg( ) );
	}
}


H5::DataSet DataLogger::writeDataSet( std::string data, std::string name, H5::Group& group )
{
	try
	{
		hsize_t rank1[] = { data.length( ) };
		H5::DataSet dset = group.createDataSet( cstr( name ), H5::PredType::C_S1, H5::DataSpace( 1, rank1 ) );
		dset.write( cstr( data ), H5::PredType::C_S1 );
		return dset;
	}
	catch ( H5::Exception& err )
	{
		thrower( "ERROR: error while writing string data set to H5 File. String was " + data 
				 + ". Dataset name was " + name + ". Error was :\r\n" + err.getDetailMsg( ) );
	}
}

void DataLogger::writeAttribute( double data, std::string name, H5::DataSet& dset )
{
	try
	{
		hsize_t rank1[] = { 1 };
		H5::Attribute attr = dset.createAttribute( cstr( name ), H5::PredType::NATIVE_DOUBLE, H5::DataSpace( 1, rank1 ) );
		attr.write( H5::PredType::NATIVE_DOUBLE, &data );
	}
	catch ( H5::Exception& err )
	{
		thrower( "ERROR: error while writing bool attribute to H5 File. bool was " + str( data )
				 + ". Dataset name was " + name + ". Error was :\r\n" + err.getDetailMsg( ) );
	}
}

void DataLogger::writeAttribute( bool data, std::string name, H5::DataSet& dset )
{
	try
	{
		hsize_t rank1[] = { 1 };
		H5::Attribute attr = dset.createAttribute( cstr(name), H5::PredType::NATIVE_HBOOL, H5::DataSpace( 1, rank1 ) );
		attr.write( H5::PredType::NATIVE_HBOOL, &data);
	}
	catch ( H5::Exception& err )
	{
		thrower( "ERROR: error while writing bool attribute to H5 File. bool was " + str(data)
				 + ". Dataset name was " + name + ". Error was :\r\n" + err.getDetailMsg( ) );
	}
}

void DataLogger::moveFileToHeap()
{
	std::string currentFileLocation = dataFilesBaseLocation + currentSaveFolder + "\\Raw Data\\data_"
		+ str(currentDataFileNumber) + ".h5";
	std::string finalFileLocaation = DATA_SAVE_LOCATION + currentSaveFolder + "\\Raw Data\\data_"
		+ str(currentDataFileNumber) + ".h5";
	if (std::experimental::filesystem::exists(currentFileLocation))
	{
		fs::rename(currentFileLocation, finalFileLocaation);
	}
}