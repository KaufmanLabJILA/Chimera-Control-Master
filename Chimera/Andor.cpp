#include "stdafx.h"
#include "ATMCD32D.h"
#include "Andor.h"
#include "atcore.h"
#include "CameraWindow.h"
#include <chrono>
#include <process.h>
#include <algorithm>
#include <numeric>

std::string AndorCamera::getSystemInfo()
{
	std::string info;
	// can potentially get more info from this.
	//AndorCapabilities capabilities;
	//getCapabilities( capabilities );
	info += "Camera Model: " + getHeadModel() + "\n";
	int num;
	getSerialNumber(num);
	info += "Camera Serial Number: " + str(num) + "\n";
	return info;
}


AndorCamera::AndorCamera()
{
	runSettings.emGainModeIsOn = false;
	std::string errorMessage;
	if (ANDOR_SAFEMODE)
	{
		errorMessage = "Andor Camera is in SAFEMODE: Initialization Not attempted.";
	}
	// Initialize driver in current directory
	try
	{
		NumberOfAcqBuffers = 10;
		NumberOfImageBuffers = 10;
		initialize();
		//setBaselineClamp(1);
		//setShutter(0, 5, 30, 30); //Shutter open for any series, 30ms open/close time.
		//setDMAParameters(1, 0.0001f);
		// TODO: turn fan back off, fanmode 2
		//setFanMode(0); //Internal fan off.
		//SetFastExternalTrigger(0); //TODO: Be careful of this setting.
	}
	catch (Error& err)
	{
		errBox(err.what());
	}

}

AndorCamera::~AndorCamera()
{
	AT_Close(CameraHndl);
	AT_FinaliseLibrary();
}

void AndorCamera::initializeClass(Communicator* comm, chronoTimes* imageTimes)
{
	threadInput.comm = comm;
	threadInput.imageTimes = imageTimes;
	threadInput.Andor = this;
	threadInput.spuriousWakeupHandler = false;
	// begin the camera wait thread.
	_beginthreadex(NULL, 0, &AndorCamera::cameraThread, &threadInput, 0, &cameraThreadID);
}

void AndorCamera::updatePictureNumber(ULONGLONG newNumber)
{
	currentPictureNumber = newNumber;
}

/*
 * pause the camera thread which watches the camera for pictures
 */
void AndorCamera::pauseThread()
{
	// andor should not be taking images anymore at this point.
	threadInput.spuriousWakeupHandler = false;
}


/*
 * this should get called when the camera finishes running. right now this is very simple.
 */
void AndorCamera::onFinish()
{
	//Free the allocated buffer s
	for (int i = 0; i < NumberOfAcqBuffers; i++) {
		delete[] AcqBuffers[i];
	}

	AT_Command(CameraHndl, L"Acquisition Stop");
	AT_Flush(CameraHndl);

	threadInput.signaler.notify_all();
	cameraIsRunning = false;
}


/*
 * this thread watches the camera for pictures and when it sees a picture lets the main thread know via a message.
 * it gets initialized at the start of the program and is basically always running.
 */
unsigned __stdcall AndorCamera::cameraThread(void* voidPtr)
{
	cameraThreadInput* input = (cameraThreadInput*)voidPtr;
	//... I'm not sure what this lock is doing here... why not inside while loop?
	std::unique_lock<std::mutex> lock(input->runMutex);
	int safeModeCount = 0;
	long pictureNumber = 1;
	bool armed = false;
	while (!input->Andor->cameraThreadExitIndicator)
	{
		/*
		 * wait until unlocked. this happens when data is started.
		 * the first argument is the lock.  The when the lock is locked, this function just sits and doesn't use cpu,
		 * unlike a while(gGlobalCheck){} loop that waits for gGlobalCheck to be set. The second argument here is a
		 * lambda, more or less a quick inline function that doesn't in this case have a name. This handles something
		 * called spurious wakeups, which are weird and appear to relate to some optimization things from the quick
		 * search I did. Regardless, I don't fully understand why spurious wakeups occur, but this protects against
		 * them.
		 */
		 // Also, anytime this gets locked, the count should be reset.
		input->signaler.wait(lock, [input, &safeModeCount]() { return input->spuriousWakeupHandler; });
		if (!ANDOR_SAFEMODE)
		{
			try
			{
				int status;
				//input->Andor->queryStatus(status);
				//auto start = std::chrono::high_resolution_clock::now();
				input->Andor->waitForAcquisition(pictureNumber);

				if (pictureNumber % 2 == 0)
				{
					(*input->imageTimes).push_back(std::chrono::high_resolution_clock::now());
				}
				armed = true;

				/*input->Andor->updatePictureNumber(pictureNumber);
				std::vector<std::vector<long>> picData;
				picData = input->Andor->acquireImageData(pictureNumber);
				input->Andor->queueBuffers(pictureNumber);*/

				input->comm->sendCameraProgress(pictureNumber);

				if (pictureNumber == input->Andor->runSettings.totalPicsInExperiment && armed)
				{
					// signal the end to the main thread.
					//input->comm->sendCameraProgress(-1);
					input->comm->sendCameraFin();
					armed = false;
					pictureNumber = 1;
				}
				else {
					++pictureNumber;
					//requeue the buffers if taking more images
					/*input->Andor->queueBuffers(pictureNumber);*/
				}
				/*auto stop = std::chrono::high_resolution_clock::now();
				auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(stop - start);
				int a = 0;*/
			}
			catch (Error&)
			{
				//...? When does this happen? not sure why this is here...
			}
		}
		else
		{
			// simulate an actual wait.
			//Sleep( ULONG(input->Andor->runSettings.kineticCycleTime * 1000) );
			Sleep(100);
			if (pictureNumber % 2 == 0)
			{
				(*input->imageTimes).push_back(std::chrono::high_resolution_clock::now());
			}
			if (input->Andor->cameraIsRunning && safeModeCount < input->Andor->runSettings.totalPicsInExperiment)
			{
				if (input->Andor->runSettings.cameraMode == "Kinetic Series Mode"
					|| input->Andor->runSettings.cameraMode == "Accumulation Mode")
				{
					safeModeCount++;
					input->comm->sendCameraProgress(safeModeCount);
				}
				else
				{
					input->comm->sendCameraProgress(1);
				}
			}
			else
			{
				input->Andor->cameraIsRunning = false;
				safeModeCount = 0;
				input->comm->sendCameraFin();
				input->spuriousWakeupHandler = false;
			}
		}
	}
	return 0;
}


/*
 * Get whatever settings the camera is currently using in it's operation, assuming it's operating.
 */
AndorRunSettings AndorCamera::getSettings()
{
	return runSettings;
}

void AndorCamera::queueBuffers(int pictureNumber) {
	andorErrorChecker(AT_QueueBuffer(CameraHndl, AcqBuffers[(pictureNumber - 1) % NumberOfAcqBuffers], BufferSize));
}

void AndorCamera::setSettings(AndorRunSettings settingsToSet)
{
	runSettings = settingsToSet;
}

void AndorCamera::setAcquisitionMode()
{
	setAcquisitionMode(runSettings.acquisitionMode);
}

/*
	* Large function which initializes a given camera image run.
	*/
void AndorCamera::armCamera(CameraWindow* camWin, double& minKineticCycleTime)
{
	/// Set a bunch of parameters.

	if (!ANDOR_SAFEMODE) {
		if (runSettings.triggerMode != "External Exposure") {
			setExposures();
		}
		
		//andorErrorChecker(AT_SetBool(CameraHndl, L"VerticallyCentreAOI", true)); //note if setting this true, can't set "AOITop"
		setImageParametersToCamera();
		setCameraTriggerMode();

		// check plotting parameters
		/// TODO!
		// CAREFUL! I can only modify these guys here because I'm sure that I'm also not writing to them in the plotting 
		// thread since the plotting thread hasn't
		// started yet. If moving stuff around, be careful.
		// Initialize the thread accumulation number.
		// this->??? = 1;
		// //////////////////////////////
		//queryStatus();

		/// Do some plotting stuffs
		//eAlerts.setAlertThreshold();
		//ePicStats.reset();

		// the lock is released when the lock object function goes out of scope, which happens immediately after
		// the start acquisition call
		//std::lock_guard<std::mutex> lock( threadInput.runMutex );

		// get the min time after setting everything else.
		//minKineticCycleTime = getMinKineticCycleTime();


		cameraIsRunning = true;
		// remove the spurious wakeup check.
		threadInput.spuriousWakeupHandler = true;

		AT_SetEnumeratedString(CameraHndl, L"Pixel Encoding", L"Mono12");

		AT_64 ImageSizeBytes;
		AT_GetInt(CameraHndl, L"Image Size Bytes", &ImageSizeBytes);

		BufferSize = static_cast<int>(ImageSizeBytes);

		/*NumberOfAcqBuffers = runSettings.picsPerRepetition;
		NumberOfImageBuffers = runSettings.picsPerRepetition;*/

		//Allocate a number of memory buffers to store frames
		AcqBuffers.resize(NumberOfAcqBuffers);
		tempImageBuffers.resize(NumberOfImageBuffers);

		//for (int i = 0; i < NumberOfBuffers; i++) {
		//	AcqBuffers[i] = new unsigned char[BufferSize];
		//} //Pass these buffers to the SDK
		//for (int i = 0; i < NumberOfBuffers; i++) {
		//	andorErrorChecker(AT_QueueBuffer(CameraHndl, AcqBuffers[i], BufferSize));
		//}
		for (int i = 0; i < NumberOfAcqBuffers; i++) {
			AcqBuffers[i] = new unsigned char[BufferSize];
		}
		for (int i = 0; i < NumberOfAcqBuffers; i++) {
			andorErrorChecker(AT_QueueBuffer(CameraHndl, AcqBuffers[i], BufferSize));
		}

		//Set the camera to continuously acquires frames 
		andorErrorChecker(AT_SetEnumString(CameraHndl, L"CycleMode", L"Continuous"));
		//Set the camera AUX out to show if any pixel row is being exposed
		andorErrorChecker(AT_SetEnumString(CameraHndl, L"AuxiliaryOutSource", L"FireAny"));

	}
	startAcquisition();
	// notify the thread that the experiment has started..
	threadInput.signaler.notify_all();
}


/*
 * This function checks for new pictures, if they exist it gets them, and shapes them into the array which holds all of
 * the pictures for a given repetition.
 */
std::vector<std::vector<long>> AndorCamera::acquireImageData(int pictureNumber)
{
	//try
	//{
	//	checkForNewImages();
	//}
	//catch (Error& exception)
	//{
	//	if (exception.whatBare() == "DRV_NO_NEW_DATA")
	//	{
	//		throw;
	//		//TODO: put handling for this error back in.
	//		// just return this anyways.
	//		/*return imagesOfExperiment*/;
	//	}
	//	else
	//	{
	//		// it's an actual error, pass it up.
	//		throw;
	//	}
	//}


	/// ///
	// for only one image... (each image processed from the call from a separate windows message)
	int size;
	// If there is no data the acquisition must have been aborted
	// free all allocated memory
	int experimentPictureNumber;
	if (runSettings.showPicsInRealTime)
	{
		experimentPictureNumber = 0;
	}
	else
	{
		experimentPictureNumber = (((currentPictureNumber - 1) % runSettings.totalPicsInVariation)
			% runSettings.picsPerRepetition);
	}

	if (experimentPictureNumber == 0)
	{
		WaitForSingleObject(imagesMutex, INFINITE);
		imagesOfExperiment.clear();
		if (runSettings.showPicsInRealTime)
		{
			imagesOfExperiment.resize(1);
		}
		else
		{
			imagesOfExperiment.resize(runSettings.picsPerRepetition);
		}
		ReleaseMutex(imagesMutex);
	}


	size = runSettings.imageSettings.width * runSettings.imageSettings.height;
	std::vector<long> tempImage;
	tempImage.resize(size);
	WaitForSingleObject(imagesMutex, INFINITE);
	imagesOfExperiment[experimentPictureNumber].resize(size);
	if (!ANDOR_SAFEMODE)
	{
		AT_64 Stride, Width, Height;
		AT_GetInt(CameraHndl, L"AOIStride", &Stride);
		AT_GetInt(CameraHndl, L"AOIWidth", &Width);
		AT_GetInt(CameraHndl, L"AOIHeight", &Height);

		for (AT_64 Row = 0; Row < Height; Row++) {
			//Cast the raw image buffer to a 16-bit array. 
			//...Assumes the PixelEncoding is 16-bit. 
			unsigned short* ImagePixels = reinterpret_cast<unsigned short*>(tempImageBuffers[(pictureNumber - 1) % NumberOfImageBuffers]);
			//Process each pixel in a row as normal 
			for (AT_64 Pixel = 0; Pixel < Width; Pixel++) {
				tempImage[Row*Width + Pixel] = ImagePixels[Pixel];
			} //Use Stride to get the memory location of the next row. 
			tempImageBuffers[(pictureNumber - 1) % NumberOfImageBuffers] += Stride;
		}

		if (tempImage.size() == 0) {
			throw;
		}
		// immediately rotate
		for (UINT imageVecInc = 0; imageVecInc < imagesOfExperiment[experimentPictureNumber].size(); imageVecInc++)
		{
			imagesOfExperiment[experimentPictureNumber][imageVecInc] = tempImage[imageVecInc];
		}

	}
	else
	{
		// generate a fake image.
		std::vector<bool> atomSpots = { 0,	0,	0,	0,	0,	0,	0,	0,	0,	0,	0,	0,	0,	0,	0,
										0,	1,	0,	1,	0,	1,	0,	1,	0,	1,	0,	1,	0,	1,	0,
										0,	0,	0,	0,	0,	0,	0,	0,	0,	0,	0,	0,	0,	0,	0,
										0,	1,	0,	1,	0,	1,	0,	1,	0,	1,	0,	1,	0,	1,	0,
										0,	0,	0,	0,	0,	0,	0,	0,	0,	0,	0,	0,	0,	0,	0,
										0,	1,	0,	1,	0,	1,	0,	1,	0,	1,	0,	1,	0,	1,	0,
										0,	0,	0,	0,	0,	0,	0,	0,	0,	0,	0,	0,	0,	0,	0,
										0,	1,	0,	1,	0,	1,	0,	1,	0,	1,	0,	1,	0,	1,	0,
										0,	0,	0,	0,	0,	0,	0,	0,	0,	0,	0,	0,	0,	0,	0,
										0,	1,	0,	1,	0,	1,	0,	1,	0,	1,	0,	1,	0,	1,	0,
										0,	0,	0,	0,	0,	0,	0,	0,	0,	0,	0,	0,	0,	0,	0,
										0,	1,	0,	1,	0,	1,	0,	1,	0,	1,	0,	1,	0,	1,	0,
										0,	0,	0,	0,	0,	0,	0,	0,	0,	0,	0,	0,	0,	0,	0,
										0,	1,	0,	1,	0,	1,	0,	1,	0,	1,	0,	1,	0,	1,	0,
										0,	0,	0,	0,	0,	0,	0,	0,	0,	0,	0,	0,	0,	0,	0
		};
		atomSpots = { 0,0,0,
					  0,1,0,
					  0,0,0,
					  0,0,0,
					  0,0,0 };

		for (UINT imageVecInc = 0; imageVecInc < imagesOfExperiment[experimentPictureNumber].size(); imageVecInc++)
		{
			tempImage[imageVecInc] = rand() % 30 + 95;
			if (!(imageVecInc >= atomSpots.size()))
			{
				if (imageVecInc == 0)
				{
					tempImage[imageVecInc] = 3;
					continue;
				}
				if (atomSpots[imageVecInc])
				{
					// can have an atom here.
					if (UINT(rand()) % 300 > imageVecInc + 50)
					{
						tempImage[imageVecInc] += 400;
					}
				}
			}
		}
		WaitForSingleObject(imagesMutex, INFINITE);
		for (UINT imageVecInc = 0; imageVecInc < imagesOfExperiment[experimentPictureNumber].size(); imageVecInc++)
		{
			imagesOfExperiment[experimentPictureNumber][imageVecInc] = tempImage[((imageVecInc % runSettings.imageSettings.width)
				+ 1) * runSettings.imageSettings.height - imageVecInc / runSettings.imageSettings.width - 1];
		}
		ReleaseMutex(imagesMutex);
	}
	ReleaseMutex(imagesMutex);
	return imagesOfExperiment;
}


// sets this based on internal settings object.
void AndorCamera::setCameraTriggerMode()
{
	std::string errMsg;
	AT_WC* trigType;
	if (runSettings.triggerMode == "Internal Trigger")
	{
		trigType = L"Internal";
	}
	else if (runSettings.triggerMode == "Start On Trigger")
	{
		trigType = L"External Start";
	}
	else if (runSettings.triggerMode == "External Exposure")
	{
		trigType = L"External Exposure";
	}
	else {
		trigType = L"External";
	}
	setTriggerMode(trigType);
}


void AndorCamera::setTemperature()
{
	// Get the current temperature
	if (runSettings.temperatureSetting < -60 || runSettings.temperatureSetting > 25)
	{
		int answer = promptBox("Warning: The selected temperature is outside the normal temperature range of the "
			"camera (-60 through 25 C). Proceed anyways?", MB_OKCANCEL);
		if (answer == IDCANCEL)
		{
			return;
		}
	}
	// Proceedure to initiate cooling
	changeTemperatureSetting(false);
}


void AndorCamera::setReadMode()
{
	setReadMode(runSettings.readMode);
}


void AndorCamera::setExposures()
{
	//if (runSettings.exposureTimes.size() > 0 && runSettings.exposureTimes.size() <= 16)
	//{
	//	try {
	//		//setRingExposureTimes(runSettings.exposureTimes.size(), runSettings.exposureTimes.data());
	//	}
	//	catch (Error& err)
	//	{
	//		errBox("ERROR: " + err.whatStr());
	//	}
	//	//setRingExposureTimes(runSettings.exposureTimes.size(), runSettings.exposureTimes.data());
	//}
	//else
	//{
	//	thrower("ERROR: Invalid size for vector of exposure times, value of " + str(runSettings.exposureTimes.size()) + ".");
	//}
	AT_SetFloat(CameraHndl, L"ExposureTime", runSettings.exposureTime);
}


void AndorCamera::setImageParametersToCamera()
{
	setImage(runSettings.imageSettings.verticalBinning, runSettings.imageSettings.horizontalBinning,
		runSettings.imageSettings.bottom, runSettings.imageSettings.top,
		runSettings.imageSettings.left, runSettings.imageSettings.right);
}


void AndorCamera::setKineticCycleTime()
{
	setKineticCycleTime(runSettings.kineticCycleTime);
}


void AndorCamera::setScanNumber()
{
	if (runSettings.totalPicsInExperiment == 0 && runSettings.totalPicsInVariation != 0)
	{
		// all is good. The first variable has not been set yet.
	}
	else if (runSettings.totalPicsInVariation == 0)
	{
		thrower("ERROR: Scan Number Was Zero.\r\n");
	}
	else
	{
		setNumberKinetics(int(runSettings.totalPicsInExperiment));
	}
}


void AndorCamera::setFrameTransferMode()
{
	setFrameTransferMode(runSettings.frameTransferMode);
}


/*
 * exposures should be initialized to be the correct size. Nothing else matters for the inputs, they get
 * over-written.
 * throws exception if fails
 */
void AndorCamera::checkAcquisitionTimings(float& kinetic, float& accumulation, float& exposure)
{
	float tempExposure, tempAccumTime, tempKineticTime;
	float * timesArray = NULL;
	std::string errMsg;
	if (ANDOR_SAFEMODE)
	{
		// if in safemode initialize this stuff to the values to be outputted.
		tempExposure = exposure;
		tempAccumTime = accumulation;
		tempKineticTime = kinetic;
	}
	else
	{
		tempExposure = 0;
		tempAccumTime = 0;
		tempKineticTime = 0;
	}
	// It is necessary to get the actual times as the system will calculate the
	// nearest possible time. eg if you set exposure time to be 0, the system
	// will use the closest value (around 0.01s)
	int numpics = runSettings.picsPerRepetition;
	timesArray = new float[numpics];
	if (ANDOR_SAFEMODE)
	{
		getAcquisitionTimes(tempExposure, tempAccumTime, tempKineticTime);
		getAdjustedRingExposureTimes(numpics, timesArray);
	}
	else
	{
		for (UINT exposureInc = 0; exposureInc < numpics; exposureInc++)
		{
			timesArray[exposureInc] = exposure;
		}
	}
	// Set times

	for (UINT exposureInc = 0; exposureInc < numpics; exposureInc++)
	{
		exposure = timesArray[exposureInc];
	}
	delete[] timesArray;
	accumulation = tempAccumTime;
	kinetic = tempKineticTime;
}


/*
 (
 */
void AndorCamera::setAccumulationCycleTime()
{
	setAccumulationCycleTime(runSettings.accumulationTime);
}


void AndorCamera::setNumberAccumulations(bool isKinetic)
{
	std::string errMsg;
	if (isKinetic)
	{
		// right now, kinetic series mode always has one accumulation. could add this feature later if desired.
		//setNumberAccumulations(true); // ???
		setAccumulationNumber(1);
	}
	else
	{
		// ???
		// setNumberAccumulations(false); // ???
		setAccumulationNumber(runSettings.accumulationNumber);
	}
}


void AndorCamera::setGainMode()
{
	if (!runSettings.emGainModeIsOn)
	{
		// Set Gain
		int numGain;
		getNumberOfPreAmpGains(numGain);
		setPreAmpGain(2);
		float myGain;
		getPreAmpGain(2, myGain);
		// 1 is for conventional gain mode.
		setOutputAmplifier(1);
	}
	else
	{
		// 0 is for em gain mode.
		setOutputAmplifier(0);
		setPreAmpGain(2);
		if (runSettings.emGainLevel > 300)
		{
			setEmGainSettingsAdvanced(1);
		}
		else
		{
			setEmGainSettingsAdvanced(0);
		}
		setEmCcdGain(runSettings.emGainLevel);
	}
}


void AndorCamera::changeTemperatureSetting(bool turnTemperatureControlOff)
{
	char aBuffer[256];
	int minimumAllowedTemp, maximumAllowedTemp;
	// the default, in case the program is in safemode.
	minimumAllowedTemp = -60;
	maximumAllowedTemp = 25;
	// clear buffer
	wsprintf(aBuffer, "");
	// check if temp is in valid range
	//getTemperatureRange(minimumAllowedTemp, maximumAllowedTemp);
	if (runSettings.temperatureSetting < minimumAllowedTemp || runSettings.temperatureSetting > maximumAllowedTemp)
	{
		thrower("ERROR: Temperature is out of range\r\n");
	}
	else
	{
		// if it is in range, switch on cooler and set temp
		if (turnTemperatureControlOff == false)
		{
			temperatureControlOn();
		}
		else
		{
			temperatureControlOff();
		}
	}

	// ???
	/*
	eCooler = TRUE;
	SetTimer(eCameraWindowHandle, ID_TEMPERATURE_TIMER, 1000, NULL);
	*/
	if (turnTemperatureControlOff == false)
	{
		setTemperature(runSettings.temperatureSettingEnum);
	}
	else
	{
		thrower("Temperature Control has been turned off.\r\n");
	}
}

/*
 *
 */
void AndorCamera::andorErrorChecker(int errorCode)
{
	std::string errorMessage = "uninitialized";
	switch (errorCode)
	{
	case 0:
	{
		errorMessage = "AT_SUCCESS";
		break;
	}
	case 1:
	{
		errorMessage = "AT_ERR_NOTINITIALISED";
		break;
	}
	case 20001:
	{
		errorMessage = "DRV_ERROR_CODES";
		break;
	}
	case 20002:
	{
		errorMessage = "DRV_SUCCESS";
		break;
	}
	case 20003:
	{
		errorMessage = "DRV_VXDNOTINSTALLED";
		break;
	}
	case 20004:
	{
		errorMessage = "DRV_ERROR_SCAN";
		break;
	}
	case 20005:
	{
		errorMessage = "DRV_ERROR_CHECK_SUM";
		break;
	}
	case 20006:
	{
		errorMessage = "DRV_ERROR_FILELOAD";
		break;
	}
	case 20007:
	{
		errorMessage = "DRV_UNKNOWN_FUNCTION";
		break;
	}
	case 20008:
	{
		errorMessage = "DRV_ERROR_VXD_INIT";
		break;
	}
	case 20009:
	{
		errorMessage = "DRV_ERROR_ADDRESS";
		break;
	}
	case 20010:
	{
		errorMessage = "DRV_ERROR_PAGELOCK";
		break;
	}
	case 20011:
	{
		errorMessage = "DRV_ERROR_PAGE_UNLOCK";
		break;
	}
	case 20012:
	{
		errorMessage = "DRV_ERROR_BOARDTEST";
		break;
	}
	case 20013:
	{
		errorMessage = "DRV_ERROR_ACK";
		break;
	}
	case 20014:
	{
		errorMessage = "DRV_ERROR_UP_FIFO";
		break;
	}
	case 20015:
	{
		errorMessage = "DRV_ERROR_PATTERN";
		break;
	}
	case 20017:
	{
		errorMessage = "DRV_ACQUISITION_ERRORS";
		break;
	}
	case 20018:
	{
		errorMessage = "DRV_ACQ_BUFFER";
		break;
	}
	case 20019:
	{
		errorMessage = "DRV_ACQ_DOWNFIFO_FULL";
		break;
	}
	case 20020:
	{
		errorMessage = "DRV_PROC_UNKNOWN_INSTRUCTION";
		break;
	}
	case 20021:
	{
		errorMessage = "DRV_ILLEGAL_OP_CODE";
		break;
	}
	case 20022:
	{
		errorMessage = "DRV_KINETIC_TIME_NOT_MET";
		break;
	}
	case 20023:
	{
		errorMessage = "DRV_KINETIC_TIME_NOT_MET";
		break;
	}
	case 20024:
	{
		errorMessage = "DRV_NO_NEW_DATA";
		break;
	}
	case 20026:
	{
		errorMessage = "DRV_SPOOLERROR";
		break;
	}
	case 20033:
	{
		errorMessage = "DRV_TEMPERATURE_CODES";
		break;
	}
	case 20034:
	{
		errorMessage = "DRV_TEMPERATURE_OFF";
		break;
	}
	case 20035:
	{
		errorMessage = "DRV_TEMPERATURE_NOT_STABILIZED";
		break;
	}
	case 20036:
	{
		errorMessage = "DRV_TEMPERATURE_STABILIZED";
		break;
	}
	case 20037:
	{
		errorMessage = "DRV_TEMPERATURE_NOT_REACHED";
		break;
	}
	case 20038:
	{
		errorMessage = "DRV_TEMPERATURE_OUT_RANGE";
		break;
	}
	case 20039:
	{
		errorMessage = "DRV_TEMPERATURE_NOT_SUPPORTED";
		break;
	}
	case 20040:
	{
		errorMessage = "DRV_TEMPERATURE_DRIFT";
		break;
	}
	case 20049:
	{
		errorMessage = "DRV_GENERAL_ERRORS";
		break;
	}
	case 20050:
	{
		errorMessage = "DRV_INVALID_AUX";
		break;
	}
	case 20051:
	{
		errorMessage = "DRV_COF_NOTLOADED";
		break;
	}
	case 20052:
	{
		errorMessage = "DRV_FPGAPROG";
		break;
	}
	case 20053:
	{
		errorMessage = "DRV_FLEXERROR";
		break;
	}
	case 20054:
	{
		errorMessage = "DRV_GPIBERROR";
		break;
	}
	case 20064:
	{
		errorMessage = "DRV_DATATYPE";
		break;
	}
	case 20065:
	{
		errorMessage = "DRV_DRIVER_ERRORS";
		break;
	}
	case 20066:
	{
		errorMessage = "DRV_P1INVALID";
		break;
	}
	case 20067:
	{
		errorMessage = "DRV_P2INVALID";
		break;
	}
	case 20068:
	{
		errorMessage = "DRV_P3INVALID";
		break;
	}
	case 20069:
	{
		errorMessage = "DRV_P4INVALID";
		break;
	}
	case 20070:
	{
		errorMessage = "DRV_INIERROR";
		break;
	}
	case 20071:
	{
		errorMessage = "DRV_COFERROR";
		break;
	}
	case 20072:
	{
		errorMessage = "DRV_ACQUIRING";
		break;
	}
	case 20073:
	{
		errorMessage = "DRV_IDLE";
		break;
	}
	case 20074:
	{
		errorMessage = "DRV_TEMPCYCLE";
		break;
	}
	case 20075:
	{
		errorMessage = "DRV_NOT_INITIALIZED";
		break;
	}
	case 20076:
	{
		errorMessage = "DRV_P5INVALID";
		break;
	}
	case 20077:
	{
		errorMessage = "DRV_P6INVALID";
		break;
	}
	case 20078:
	{
		errorMessage = "DRV_INVALID_MODE";
		break;
	}
	case 20079:
	{
		errorMessage = "DRV_INVALID_FILTER";
		break;
	}
	case 20080:
	{
		errorMessage = "DRV_I2CERRORS";
		break;
	}
	case 20081:
	{
		errorMessage = "DRV_DRV_ICDEVNOTFOUND";
		break;
	}
	case 20082:
	{
		errorMessage = "DRV_I2CTIMEOUT";
		break;
	}
	case 20083:
	{
		errorMessage = "DRV_P7INVALID";
		break;
	}
	case 20089:
	{
		errorMessage = "DRV_USBERROR";
		break;
	}
	case 20090:
	{
		errorMessage = "DRV_IOCERROR";
		break;
	}
	case 20091:
	{
		errorMessage = "DRV_NOT_SUPPORTED";
		break;
	}
	case 20093:
	{
		errorMessage = "DRV_USB_INTERRUPT_ENDPOINT_ERROR";
		break;
	}
	case 20094:
	{
		errorMessage = "DRV_RANDOM_TRACK_ERROR";
		break;
	}
	case 20095:
	{
		errorMessage = "DRV_INVALID_tRIGGER_MODE";
		break;
	}
	case 20096:
	{
		errorMessage = "DRV_LOAD_FIRMWARE_ERROR";
		break;
	}
	case 20097:
	{
		errorMessage = "DRV_DIVIDE_BY_ZERO_ERROR";
		break;
	}
	case 20098:
	{
		errorMessage = "DRV_INVALID_RINGEXPOSURES";
		break;
	}
	case 20099:
	{
		errorMessage = "DRV_BINNING_ERROR";
		break;
	}
	case 20100:
	{
		errorMessage = "DRV_INVALID_AMPLIFIER";
		break;
	}
	case 20115:
	{
		errorMessage = "DRV_ERROR_MAP";
		break;
	}
	case 20116:
	{
		errorMessage = "DRV_ERROR_UNMAP";
		break;
	}
	case 20117:
	{
		errorMessage = "DRV_ERROR_MDL";
		break;
	}
	case 20118:
	{
		errorMessage = "DRV_ERROR_UNMDL";
		break;
	}
	case 20119:
	{
		errorMessage = "DRV_ERROR_BUFSIZE";
		break;
	}
	case 20121:
	{
		errorMessage = "DRV_ERROR_NOHANDLE";
		break;
	}
	case 20130:
	{
		errorMessage = "DRV_GATING_NOT_AVAILABLE";
		break;
	}
	case 20131:
	{
		errorMessage = "DRV_FPGA_VOLTAGE_ERROR";
		break;
	}
	case 20990:
	{
		errorMessage = "DRV_ERROR_NOCAMERA";
		break;
	}
	case 20991:
	{
		errorMessage = "DRV_NOT_SUPPORTED";
		break;
	}
	case 20992:
	{
		errorMessage = "DRV_NOT_AVAILABLE";
		break;
	}
	default:
	{
		std::string errorCodeStr = std::to_string(errorCode);
		errorMessage = "UNKNOWN ERROR MESSAGE RETURNED FROM CAMERA FUNCTION! errorCode = " + errorCodeStr;
		break;
	}
	}
	/// So no throw is considered success.
	if (errorMessage != "DRV_SUCCESS" && errorMessage != "AT_SUCCESS")
	{
		thrower(errorMessage);
	}
}

/// ANDOR SDK WRAPPERS
// the following functions are wrapped to throw errors if error are returned by the raw functions, as well as to only 
// excecute the raw functions if the camera is not in safemode.

void AndorCamera::initialize()
{
	//char aBuffer[256];
	// Look in current working directory for driver files
	//GetCurrentDirectory(256, aBuffer);
	if (!ANDOR_SAFEMODE)
	{
		//andorErrorChecker(Initialize(aBuffer));
		int i_retCode;
		i_retCode = AT_InitialiseLibrary();
		if (i_retCode != AT_SUCCESS) {
			thrower("library not initialized");
		}
		AT_64 iNumberDevices = 0;
		AT_GetInt(AT_HANDLE_SYSTEM, L"Device Count", &iNumberDevices);
		if (iNumberDevices <= 0) {
			thrower("No cameras detected");
		}
		andorErrorChecker(AT_Open(0, &CameraHndl));
	}
}

void AndorCamera::setShutter(int typ, int mode, int closingtime, int openingtime)
{
	if (!ANDOR_SAFEMODE)
	{
		andorErrorChecker(SetShutter(typ, mode, closingtime, openingtime));
	}
}

void AndorCamera::setFanMode(int mode)
{
	if (!ANDOR_SAFEMODE)
	{
		andorErrorChecker(SetFanMode(mode));
	}
}

void AndorCamera::setBaselineClamp(int clamp)
{
	if (!ANDOR_SAFEMODE)
	{
		andorErrorChecker(SetBaselineClamp(clamp));
	}
}

void AndorCamera::setBaselineOffset(int offset)
{
	if (!ANDOR_SAFEMODE)
	{
		andorErrorChecker(SetBaselineOffset(offset));
	}
}

void AndorCamera::setDMAParameters(int maxImagesPerDMA, float secondsPerDMA)
{
	if (!ANDOR_SAFEMODE)
	{
		andorErrorChecker(SetDMAParameters(maxImagesPerDMA, secondsPerDMA));
	}
}


void AndorCamera::waitForAcquisition(int pictureNumber)
{
	if (!ANDOR_SAFEMODE)
	{
		//andorErrorChecker(WaitForAcquisition());
		andorErrorChecker(AT_WaitBuffer(CameraHndl, &tempImageBuffers[(pictureNumber-1) % NumberOfImageBuffers], &BufferSize, AT_INFINITE));
	}
}

//
void AndorCamera::getAdjustedRingExposureTimes(int size, float* timesArray)
{
	if (!ANDOR_SAFEMODE)
	{
		andorErrorChecker(GetAdjustedRingExposureTimes(size, timesArray));
	}
}


void AndorCamera::setNumberKinetics(int number)
{
	if (!ANDOR_SAFEMODE)
	{
		andorErrorChecker(SetNumberKinetics(number));
	}

}


// Andor Wrappers
void AndorCamera::getTemperatureRange(int& min, int& max)
{
	if (!ANDOR_SAFEMODE)
	{
		andorErrorChecker(GetTemperatureRange(&min, &max));
	}
}


void AndorCamera::temperatureControlOn()
{
	if (!ANDOR_SAFEMODE)
	{
		andorErrorChecker(AT_SetBool(CameraHndl, L"SensorCooling", AT_TRUE));
	}
}


void AndorCamera::temperatureControlOff()
{
	if (!ANDOR_SAFEMODE)
	{
		andorErrorChecker(AT_SetBool(CameraHndl, L"SensorCooling", AT_FALSE));
	}
}

void AndorCamera::getTemperatureStatus(int& temperatureStatusIndex, AT_WC* temperatureStatus)
{
	if (!ANDOR_SAFEMODE)
	{
		AT_GetEnumIndex(CameraHndl, L"TemperatureStatus", &temperatureStatusIndex);
		AT_GetEnumStringByIndex(CameraHndl, L"TemperatureStatus", temperatureStatusIndex, temperatureStatus, 256);
	}
}

void AndorCamera::getTemperature(double& temp, int& temperatureIndex)
{
	if (!ANDOR_SAFEMODE)
	{
		andorErrorChecker(AT_GetEnumIndex(CameraHndl, L"TemperatureControl", &temperatureIndex));
		andorErrorChecker(AT_GetFloat(CameraHndl, L"SensorTemperature", &temp));
	}
}

void AndorCamera::setTemperature(int tempEnum)
{
	if (!ANDOR_SAFEMODE)
	{
		//andorErrorChecker(SetTemperature(temp));
		//int temperatureCount = 0;
		//andorErrorChecker(AT_GetEnumCount(CameraHndl, L"TemperatureControl", &temperatureCount));
		andorErrorChecker(AT_SetEnumIndex(CameraHndl, L"TemperatureControl", tempEnum));
	}
}


void AndorCamera::setADChannel(int channel)
{
	if (!ANDOR_SAFEMODE)
	{
		andorErrorChecker(SetADChannel(channel));
	}
}


void AndorCamera::setHSSpeed(int type, int index)
{
	if (!ANDOR_SAFEMODE)
	{
		andorErrorChecker(SetHSSpeed(type, index));
	}
}

void AndorCamera::setVSSpeed(int index)
{
	if (!ANDOR_SAFEMODE)
	{
		andorErrorChecker(SetVSSpeed(index));
	}
}

void AndorCamera::getVSSpeed(int index, float *speed)
{
	if (!ANDOR_SAFEMODE)
	{
		GetVSSpeed(index, speed);
	}
}

// note that the function used here could be used to get actual information about the number of images, I just only use
// it to check whether there are any new images or not. Not sure if this is the smartest way to do this.
void AndorCamera::checkForNewImages()
{
	long first, last;
	if (!ANDOR_SAFEMODE)
	{
		andorErrorChecker(GetNumberNewImages(&first, &last));
	}
	// don't do anything with the info.
}


void AndorCamera::getOldestImage(std::vector<long>& dataArray)
{
	if (!ANDOR_SAFEMODE)
	{
		andorErrorChecker(GetOldestImage(dataArray.data(), dataArray.size()));
	}
}


void AndorCamera::getNewestImage(std::vector<long>& dataArray)
{
	if (!ANDOR_SAFEMODE)
	{
		andorErrorChecker(GetMostRecentImage(dataArray.data(), dataArray.size()));
	}
}


void AndorCamera::setTriggerMode(AT_WC* mode)
{
	if (!ANDOR_SAFEMODE)
	{
		//andorErrorChecker(SetTriggerMode(mode));
		andorErrorChecker(AT_SetEnumString(CameraHndl, L"TriggerMode", mode));
	}
}


void AndorCamera::setAcquisitionMode(int mode)
{
	if (!ANDOR_SAFEMODE)
	{
		andorErrorChecker(SetAcquisitionMode(mode));
	}
}


void AndorCamera::setReadMode(int mode)
{
	if (!ANDOR_SAFEMODE)
	{
		andorErrorChecker(SetReadMode(mode));
	}
}


void AndorCamera::setRingExposureTimes(int sizeOfTimesArray, float* arrayOfTimes)
{
	if (!ANDOR_SAFEMODE)
	{
		andorErrorChecker(SetRingExposureTimes(sizeOfTimesArray, arrayOfTimes));
	}
}


void AndorCamera::setImage(int hBin, int vBin, int lBorder, int rBorder, int tBorder, int bBorder)
{
	if (!ANDOR_SAFEMODE)
	{
		//andorErrorChecker(SetImage(hBin, vBin, lBorder, rBorder, tBorder, bBorder));
		andorErrorChecker(AT_SetInt(CameraHndl, L"AOIHBin", hBin));
		andorErrorChecker(AT_SetInt(CameraHndl, L"AOIWidth", rBorder - lBorder + 1));
		andorErrorChecker(AT_SetInt(CameraHndl, L"AOILeft", lBorder));
		andorErrorChecker(AT_SetInt(CameraHndl, L"AOIVBin", vBin));
		andorErrorChecker(AT_SetInt(CameraHndl, L"AOIHeight", bBorder - tBorder + 1));
		andorErrorChecker(AT_SetInt(CameraHndl, L"AOITop", tBorder));
	}
}


void AndorCamera::setKineticCycleTime(float cycleTime)
{
	if (!ANDOR_SAFEMODE)
	{
		andorErrorChecker(SetKineticCycleTime(cycleTime));
	}
}


void AndorCamera::setFrameTransferMode(int mode)
{
	if (!ANDOR_SAFEMODE)
	{
		andorErrorChecker(SetFrameTransferMode(mode));
	}
}

double AndorCamera::getMinKineticCycleTime()
{
	// get the currently set kinetic cycle time.
	float minKineticCycleTime, dummy1, dummy2;
	setKineticCycleTime(0);
	getAcquisitionTimes(dummy1, dummy2, minKineticCycleTime);
	// re-set whatever's currently in the settings.
	setKineticCycleTime();
	return minKineticCycleTime;
}


void AndorCamera::getAcquisitionTimes(float& exposure, float& accumulation, float& kinetic)
{
	if (!ANDOR_SAFEMODE)
	{
		andorErrorChecker(GetAcquisitionTimings(&exposure, &accumulation, &kinetic));
	}
}

/*
*/
void AndorCamera::queryStatus()
{
	int status;
	queryStatus(status);
	if (ANDOR_SAFEMODE)
	{
		status = DRV_IDLE;
	}
	if (status != DRV_IDLE)
	{
		thrower("ERROR: You tried to start the camera, but the camera was not idle! Camera was in state corresponding to "
			+ str(status) + "\r\n");
	}
}

void AndorCamera::setIsRunningState(bool state)
{
	cameraIsRunning = state;
}


void AndorCamera::queryStatus(int& status)
{
	if (!ANDOR_SAFEMODE)
	{
		andorErrorChecker(GetStatus(&status));
	}
}


void AndorCamera::startAcquisition()
{
	if (!ANDOR_SAFEMODE)
	{
		//andorErrorChecker(StartAcquisition());
		andorErrorChecker(AT_Command(CameraHndl, L"Acquisition Start"));
	}
}


void AndorCamera::abortAcquisition()
{
	if (!ANDOR_SAFEMODE)
	{
		//andorErrorChecker(AbortAcquisition());
		andorErrorChecker(AT_Command(CameraHndl, L"AcquisitionStop"));
		andorErrorChecker(AT_Flush(CameraHndl));
	}
}


void AndorCamera::setAccumulationCycleTime(float time)
{
	if (!ANDOR_SAFEMODE)
	{
		andorErrorChecker(SetAccumulationCycleTime(time));
	}
}


void AndorCamera::setAccumulationNumber(int number)
{
	if (!ANDOR_SAFEMODE)
	{
		andorErrorChecker(SetNumberAccumulations(number));
	}
}


void AndorCamera::getNumberOfPreAmpGains(int& number)
{
	if (!ANDOR_SAFEMODE)
	{
		andorErrorChecker(GetNumberPreAmpGains(&number));
	}
}


void AndorCamera::setPreAmpGain(int index)
{
	if (!ANDOR_SAFEMODE)
	{
		andorErrorChecker(SetPreAmpGain(index));
	}
}


void AndorCamera::getPreAmpGain(int index, float& gain)
{
	if (!ANDOR_SAFEMODE)
	{
		andorErrorChecker(GetPreAmpGain(index, &gain));
	}
}


void AndorCamera::setOutputAmplifier(int type)
{
	if (!ANDOR_SAFEMODE)
	{
		andorErrorChecker(SetOutputAmplifier(type));
	}
}


void AndorCamera::setEmGainSettingsAdvanced(int state)
{
	if (!ANDOR_SAFEMODE)
	{
		andorErrorChecker(SetEMAdvanced(state));
	}
}


void AndorCamera::setEmCcdGain(int gain)
{
	if (!ANDOR_SAFEMODE)
	{
		andorErrorChecker(SetEMCCDGain(gain));
	}
}

void AndorCamera::SetFastExternalTrigger(int mode)
{
	if (!ANDOR_SAFEMODE)
	{
		andorErrorChecker(SetFastExtTrigger(mode));
	}
}

///
bool AndorCamera::isRunning()
{
	return cameraIsRunning;
}


/*
* the input here will store how many whole pictures (not accumulations) have been taken.
*/
void AndorCamera::getAcquisitionProgress(long& seriesNumber)
{
	if (!ANDOR_SAFEMODE)
	{
		long dummyAccumulationNumber;
		andorErrorChecker(GetAcquisitionProgress(&dummyAccumulationNumber, &seriesNumber));
	}
}

/*
* overload to get both the acccumulation progress and the whole picture progress.
*/
void AndorCamera::getAcquisitionProgress(long& accumulationNumber, long& seriesNumber)
{
	if (!ANDOR_SAFEMODE)
	{
		andorErrorChecker(GetAcquisitionProgress(&accumulationNumber, &seriesNumber));
	}
}


void AndorCamera::getCapabilities(AndorCapabilities& caps)
{
	if (!ANDOR_SAFEMODE)
	{
		andorErrorChecker(GetCapabilities(&caps));
	}
}

void AndorCamera::getSerialNumber(int& num)
{
	if (!ANDOR_SAFEMODE)
	{
		andorErrorChecker(GetCameraSerialNumber(&num));
	}
}

std::string AndorCamera::getHeadModel()
{
	char nameChars[1024];
	if (!ANDOR_SAFEMODE)
	{
		andorErrorChecker(GetHeadModel(nameChars));
	}
	else
	{
		return "safemode";
	}
	return str(nameChars);
}


