#include "stdafx.h"
#include "dcamapi4.h"
#include "dcamprop.h"
#include "qcmos.h"
#include "CameraWindow.h"
#include <chrono>
#include <process.h>
#include <algorithm>
#include <numeric>



std::string qcmosCamera::getSystemInfo()
{
	DCAMERR err;
	
	char	model[ 256 ];
	char	cameraid[ 64 ];
	DCAMDEV_STRING	param;
	memset( &param, 0, sizeof(param) );
	param.size		= sizeof(param);
	param.text		= model;
	param.textbytes	= sizeof(model);
	param.iString	= DCAM_IDSTR_MODEL;
	//Model name
	err = dcamdev_getstring( hdcam, &param );
	if (failed(err))
	{
		throw(err);
	}

	param.text		= cameraid;
	param.textbytes	= sizeof(cameraid);
	param.iString	= DCAM_IDSTR_CAMERAID;
	//Serial Number
	err = dcamdev_getstring( hdcam, &param );
	if (failed(err))
	{
		throw(err);
	}
	std::string info;
	info += "Camera Model: " + str(model) + "\n";

	info += "Camera Serial Number: " + str(cameraid) + "\n";
	return info;
	
	
	
}

qcmosCamera::qcmosCamera()
{
	std::string errorMessage;
	if (HAM_SAFEMODE)
	{
		errorMessage = "QCMOS Camera is in SAFEMODE: Initialization Not attempted.";
	}
	// Initialize driver in current directory
	// Initialize DCAM-API ver 4.0

	try
	{
		memset( &apiinit, 0, sizeof(apiinit) );
		apiinit.size	= sizeof(apiinit);

		DCAMERR	err;
		err = dcamapi_init( &apiinit );

	
	}
	catch(Error& err)
	{
		errBox(err.what());
	}
	
	

	// open specified camera - Need to add exception handling here
	try
	{
		DCAMERR err;
		memset( &devopen, 0, sizeof(devopen) );
		devopen.size	= sizeof(devopen);
		devopen.index	= 0; //Always assume only one camera is connected
		err = dcamdev_open( &devopen );
		hdcam = devopen.hdcam;
		// open wait handle
		memset( &waitopen, 0, sizeof(waitopen) );
		waitopen.size	= sizeof(waitopen);
		waitopen.hdcam	= hdcam;
		err = dcamwait_open( &waitopen );
		hwait = waitopen.hwait;

		//probably not the best place to set the capture mode
		err = dcamprop_setvalue(hdcam, DCAM_IDPROP_CAPTUREMODE, DCAMPROP_CAPTUREMODE__NORMAL);
		std::cout << "Hello";
	}
	catch(Error& error)
	{
		throw(error);
	}
	
	
	
	

}

void qcmosCamera::initializeClass(Communicator* comm, chronoTimes* imageTimes)
{
	threadInput.comm = comm;
	threadInput.imageTimes = imageTimes;
	threadInput.qcmos = this;
	threadInput.spuriousWakeupHandler = false;
	// begin the camera wait thread.
	_beginthreadex(NULL, 0, &qcmosCamera::cameraThread, &threadInput, 0, &cameraThreadID);
}

void qcmosCamera::updatePictureNumber(ULONGLONG newNumber)
{
	currentPictureNumber = newNumber;
}

/*
 * pause the camera thread which watches the camera for pictures
 */
void qcmosCamera::pauseThread()
{
	// andor should not be taking images anymore at this point.
	threadInput.spuriousWakeupHandler = false;
}


/*
 * this should get called when the camera finishes running. right now this is very simple.
 */
void qcmosCamera::onFinish()
{
	threadInput.signaler.notify_all();
	cameraIsRunning = false;
}

// void qcmosCamera::setFlag(int pictureNumber)
// {
// 	flag = pictureNumber > 0;
// }

// void qcmosCamera::setInitial(bool initial)
// {
// 	initial = initial;
// }

/*
 * this thread watches the camera for pictures and when it sees a picture lets the main thread know via a message.
 * it gets initialized at the start of the program and is basically always running.
 */
unsigned __stdcall qcmosCamera::cameraThread(void* voidPtr)
{
	cameraThreadInput* input = (cameraThreadInput*)voidPtr;
	//... I'm not sure what this lock is doing here... why not inside while loop?
	std::unique_lock<std::mutex> lock(input->runMutex);
	int safeModeCount = 0;
	long pictureNumber = 0;
	bool armed = false;
	
	while (!input->qcmos->cameraThreadExitIndicator)
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
		input->signaler.wait(lock, [input, &safeModeCount]() { return input->spuriousWakeupHandler;});
		if (!HAM_SAFEMODE)
		{
			try
			{
				
				int32 status;
				std::cout << "Hello";
				DCAMERR err;
				input->qcmos->queryStatus(status);
				if (status == DCAMCAP_STATUS_READY && armed && (input->qcmos->flagChecker) && input->qcmos->currentFrameAccessedIndex > -1)
				{
					// signal the end to the main thread.
					input->comm->sendCameraProgress(-1);
					input->comm->sendCameraFin();
					armed = false;
					input->qcmos->flagChecker = false;
				}
				else
				{
					DCAMERR err;
					input->qcmos->waitForAcquisition(err);
					//instead of waiting for acquisition, check if new frames have been added using currentFrameAccessedIndex

					
					if (pictureNumber % 2 == 0)
					{
						(*input->imageTimes).push_back(std::chrono::high_resolution_clock::now());
					}
					armed = true;
					
					if (true)
					{
						try
						{
							input->qcmos->getAcquisitionProgress(pictureNumber);
							
						}
						catch (Error& exception)
						{
							input->comm->sendError(exception.what());
						}
						if (input->qcmos->currentFrameAccessedIndex + 1 <= pictureNumber && input->qcmos->currentFrameAccessedIndex > -1)
						{
							// input->qcmos->currentFrameAccessedIndex += 1;
							if (pictureNumber == input->qcmos->runSettings.totalPicsInExperiment) 
							{ 
								pictureNumber = 0; 
								input->qcmos->flagChecker = true;
								input->qcmos->initialChecker = false;
							}
						
							if (pictureNumber != 0)
							{
								input->comm->sendCameraProgress(pictureNumber);
							
							}
							else if (pictureNumber == 0 && !input->qcmos->initialChecker) 
							{ 
								input->comm->sendCameraProgress(pictureNumber); 
							}
						}
						/*else
						{
							input->comm->sendCameraProgress(pictureNumber);
						}*/
					}	
				}
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
			if (input->qcmos->cameraIsRunning && safeModeCount < input->qcmos->runSettings.totalPicsInExperiment)
			{
				if (input->qcmos->runSettings.cameraMode == "Kinetic Series Mode"
					|| input->qcmos->runSettings.cameraMode == "Accumulation Mode")
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
				input->qcmos->cameraIsRunning = false;
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
qcmosRunSettings qcmosCamera::getSettings()
{
	return runSettings;
}

void qcmosCamera::setSettings(qcmosRunSettings settingsToSet)
{
	runSettings = settingsToSet;
}

// void qcmosCamera::setAcquisitionMode()
// //Support only for one kind of mode for the qcmos camera as of now
// {
// 	setAcquisitionMode(runSettings.acquisitionMode);
// }

/*
	* Large function which initializes a given camera image run.
	*/
void qcmosCamera::armCamera(CameraWindow* camWin, double& minKineticCycleTime)


{	
	/// Set a bunch of parameters.
	// Set to 1 MHz readout rate in both cases

	// deallocate any existing buffer because this can create problems
	
	DCAMERR err;
	try
	{
		err = dcambuf_release(hdcam);
		std::cout << "hello";
	}
	catch(Error& err)
	{
		errBox(err.what());
	}
	
	

	// setAcquisitionMode();

	//Not implementing setting read mode as of now
	//setReadMode();

	//set initialCheck and flagCheck variables
	flagChecker = false;
	initialChecker = true;
	currentFrameAccessedIndex = 0;

	int32 status;
	queryStatus(status);


	setImageParametersToCamera();
	//probably not the best place to set the capture mode
	
	// Set Mode-Specific Parameters - Currently only set to mode 3. So if any other mode is called, 
	// instead of crashing, we will just set the parameters for kinetic series mode
	if (runSettings.acquisitionMode == 3)
	{
		setExposures();
		setTemperature();
		//setFanMode(1); //Fan is on
		// setFrameTransferMode(0);
		// setKineticCycleTime();
		//setScanNumber();
		// set this to 1.
		//setNumberAccumulations(true);
	}
	// setGainMode();
	setCameraTriggerMode();
	// int32 status;
	// queryStatus(status);
	
	//For the qcmos here we get DCAM_IDPROP_TIMING_MINTRIGGERBLANKING
	//minKineticCycleTime = getMinKineticCycleTime();

	cameraIsRunning = true;
	// remove the spurious wakeup check.
	threadInput.spuriousWakeupHandler = true;
	// notify the thread that the experiment has started..
	threadInput.signaler.notify_all();
	currentFrameIndex = 0;


	//Allocate buffer space - since this is done every variation, see if there is buffer space first and release
	// from the older variation
	
	//Was initially set to totalpicsinvariation, and so was causing problems
	err = dcambuf_alloc( hdcam, runSettings.totalPicsInExperiment);
	if (failed(err))
	{
		throw(err);
	}
	
	



	startAcquisition();
}

/*
 * This function checks for new pictures, if they exist it gets them, and shapes them into the array which holds all of
 * the pictures for a given repetition.
 */
std::vector<std::vector<long>> qcmosCamera::acquireImageData()
{

	currentFrameAccessedIndex += 1;
	if(currentFrameAccessedIndex > runSettings.totalPicsInExperiment)
	{
		return imagesOfExperiment;
		currentFrameAccessedIndex = -1;
	}
	 // Set the wait parameters - wait for a new image - checks if the frame is ready in the buffer
	 DCAMWAIT_START waitstart;
	 memset( &waitstart, 0, sizeof(waitstart) );
	 waitstart.size		= sizeof(waitstart);
	 waitstart.eventmask	= DCAMWAIT_CAPEVENT_FRAMEREADY;
	 waitstart.timeout	= 100;

	 // wait for image
	 DCAMERR err;
	 err = dcamwait_start( hwait, &waitstart );
	 std::cout << "Hello!";

	 

	//// transferinfo param
	//DCAMCAP_TRANSFERINFO captransferinfo;
	//memset( &captransferinfo, 0, sizeof(captransferinfo));
	//captransferinfo.size	= sizeof(captransferinfo);

	//// get number of captured image
	//DCAMERR err;
	//err = dcamcap_transferinfo( hdcam, &captransferinfo);




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
	if (!HAM_SAFEMODE)
	{
		getOldestImage(tempImage);
		if (tempImage.size() == 0) {
			throw;
		}
		// immediately rotate
		for (UINT imageVecInc = 0; imageVecInc < imagesOfExperiment[experimentPictureNumber].size(); imageVecInc++)
		{
			float a = ((imageVecInc
				% runSettings.imageSettings.width) + 1) * runSettings.imageSettings.height
				- imageVecInc / runSettings.imageSettings.width - 1;
			imagesOfExperiment[experimentPictureNumber][imageVecInc] = tempImage[((imageVecInc
				% runSettings.imageSettings.width) + 1) * runSettings.imageSettings.height
				- imageVecInc / runSettings.imageSettings.width - 1];
		}

		std::cout << "Hello";
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

//////////////////////////////////////////////
///// START DCAM API WRAPPERS
//////////////////////////////////////////////

// sets this based on internal settings object.
void qcmosCamera::setCameraTriggerMode()
{
	std::string errMsg;
	int trigType;
	if (hdcam != NULL)
	
	{
		DCAMERR err;
		if (runSettings.triggerMode == "Internal Trigger")
		{
			err = dcamprop_setvalue( hdcam, DCAM_IDPROP_TRIGGERSOURCE, DCAMPROP_TRIGGERSOURCE__INTERNAL );
		}
		else if (runSettings.triggerMode == "External Trigger")
		{
			err = dcamprop_setvalue( hdcam, DCAM_IDPROP_TRIGGERSOURCE, DCAMPROP_TRIGGERSOURCE__EXTERNAL );
			std::cout << "Hello";
		}
		// Start on trigger is set to software trigger temporarily
		else if (runSettings.triggerMode == "Start On Trigger")
		{
			err = dcamprop_setvalue( hdcam, DCAM_IDPROP_TRIGGERSOURCE, DCAMPROP_TRIGGERSOURCE__SOFTWARE );
		}
		
	}
	else
	{
		throw;
	}
	
}

void qcmosCamera::setTemperature()
{
	// Get the current temperature
	if (runSettings.temperatureSetting < -35 || runSettings.temperatureSetting > 25)
	{
		int answer = promptBox("Warning: The selected temperature is outside the normal temperature range of the "
			"camera (-35 through 25 C). Proceed anyways?", MB_OKCANCEL);
		if (answer == IDCANCEL)
		{
			return;
		}
	}
	// Proceedure to initiate cooling
	changeTemperatureSetting(false);
}

void qcmosCamera::changeTemperatureSetting(bool turnTemperatureControlOff)
{
	char aBuffer[256];
	int minimumAllowedTemp, maximumAllowedTemp;
	// the default, in case the program is in safemode.
	minimumAllowedTemp = -35;
	maximumAllowedTemp = 25;
	// clear buffer
	wsprintf(aBuffer, "");
	// check if temp is in valid range
	// getTemperatureRange(minimumAllowedTemp, maximumAllowedTemp);
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

	if (turnTemperatureControlOff == false)
	{
		try
		{
			DCAMERR err;
			// err = dcamprop_setvalue( hdcam, DCAM_IDPROP_SENSORCOOLER, DCAMPROP_SENSORCOOLER__ON);
			err = dcamprop_setvalue( hdcam, DCAM_IDPROP_SENSORTEMPERATURETARGET, runSettings.temperatureSetting);
		}
		catch(Error& err)
		{
			errBox(err.what());
		}

	} 
	else
	{
		thrower("Temperature Control has been turned off.\r\n");
	}
}

void qcmosCamera::setExposures()
//This function was supposed to set a range of exposure times which was easy to do on Andor, but not so straightforward here.
//Skipping for now, but maybe useful later. Basically what happens now is, instead of changing the code, we just take the first
// value of the exposure time 
{
	if (runSettings.exposureTimes.size() > 0 && runSettings.exposureTimes.size() <= 16)
	{
		try 
		{
			//setRingExposureTimes(runSettings.exposureTimes.size(), runSettings.exposureTimes.data());
			setSingleExposure();
		}
		catch (Error& err)
		{
			errBox("ERROR: " + err.whatStr());
		}
	}
	else
	{
		thrower("ERROR: Invalid size for vector of exposure times, value of " + str(runSettings.exposureTimes.size()) + ".");
	}
}

void qcmosCamera::setRingExposureTimes(int sizeOfTimesArray, double* arrayOfTimes)
{
	if (!HAM_SAFEMODE)
	{
		DCAMERR err;
		err = dcamprop_setvalue( hdcam, DCAM_IDPROP_EXPOSURETIME_CONTROL, DCAMPROP_MODE__ON);
		if (failed(err))
		{
			throw(err);
		}
		err = dcamprop_setvalue( hdcam, DCAM_IDPROP_EXPOSURETIME, arrayOfTimes[0]);
		if (failed(err))
		{
			throw(err);
		}
	}
}

void qcmosCamera::setSingleExposure()
{
	if (!HAM_SAFEMODE)
	{
	
		DCAMERR err;
		try
		{
			err = dcamprop_setvalue(hdcam, DCAM_IDPROP_EXPOSURETIME_CONTROL, DCAMPROP_MODE__ON);
		}
		catch (Error& err)
		{
			errBox(err.what());
		}

		try
		{
			double internalFrameRate;
			double minTrigger;
			err = dcamprop_setvalue(hdcam, DCAM_IDPROP_EXPOSURETIME, runSettings.exposureTimes[0]);
			std::cout << "SET";
			err = dcamprop_getvalue( hdcam, DCAM_IDPROP_INTERNALFRAMERATE, &internalFrameRate);
			err = dcamprop_getvalue( hdcam, DCAM_IDPROP_TIMING_MINTRIGGERINTERVAL, &minTrigger);
			// csc.minKineticCycleTimeLabel.SetWindowTextA(cstr(minTrigger))
			
		}
		catch (Error& err)
		{
			errBox(err.what());
		}


	}
		

}

void qcmosCamera::setImageParametersToCamera()
{
	setImage(runSettings.imageSettings.verticalBinning, runSettings.imageSettings.horizontalBinning,
		runSettings.imageSettings.bottom, runSettings.imageSettings.top,
		runSettings.imageSettings.left, runSettings.imageSettings.right);
}

void qcmosCamera::setImage(int vBin, int hBin, int bBorder, int tBorder, int lBorder, int rBorder)
{
	int32 vSize = abs(tBorder-bBorder);
	int32 hSize = abs(lBorder-rBorder);
	int32 lB = lBorder;
	int32 vB = bBorder;

	// vSize = 200;
	// hSize = 200;
	// lB = 10;
	// vB = 10;
	if (!HAM_SAFEMODE)
	{
		DCAMERR err;
		
		err = dcamprop_setvalue( hdcam, DCAM_IDPROP_BINNING_INDEPENDENT, DCAMPROP_MODE__ON);
	
		try
		{
		err = dcamprop_setvalue(hdcam, DCAM_IDPROP_SUBARRAYMODE, DCAMPROP_MODE__OFF);
		}
		catch(Error& err)
		{
		errBox(err.what());
		}

		try
		{
		err = dcamprop_setvalue(hdcam, DCAM_IDPROP_SUBARRAYMODE, DCAMPROP_MODE__OFF);
		}
		catch(Error& err)
		{
		errBox(err.what());
		}
		 
		
		
		err = dcamprop_setvalue(hdcam, DCAM_IDPROP_SUBARRAYHSIZE, hSize);
		if (failed(err))
		{
			throw(err);
		}
		err = dcamprop_setvalue( hdcam, DCAM_IDPROP_SUBARRAYHPOS, lBorder);
		if (failed(err))
		{
			throw(err);
		}
		err = dcamprop_setvalue(hdcam, DCAM_IDPROP_SUBARRAYVSIZE, vSize);
		if (failed(err))
		{
			throw(err);
		}
		err = dcamprop_setvalue( hdcam, DCAM_IDPROP_SUBARRAYVPOS, tBorder);
		if (failed(err))
		{
			throw(err);
		}
		err = dcamprop_setvalue(hdcam, DCAM_IDPROP_SUBARRAYMODE, DCAMPROP_MODE__ON);
		if (failed(err))
		{
			throw(err);
		}
		// err = dcamprop_setvalue( hdcam, DCAM_IDPROP_BINNING_HORZ, hBin);
		// if (failed(err))
		// {
		// 	throw(err);
		// }
		// err = dcamprop_setvalue( hdcam, DCAM_IDPROP_BINNING_VERT, vBin);
		// if (failed(err))
		// {
		// 	throw(err);
		// }
	
		
	}
}

///KINETIC CYCLE IS NOT IMPLEMENTED AND THESE FUNCTIONS ARE DUMMY
//BEGIN DUMMY FUNCTIONS


void qcmosCamera::setKineticCycleTime()
{
	setKineticCycleTime(runSettings.kineticCycleTime);
}

void qcmosCamera::setKineticCycleTime(float cycleTime)
{
	if (!HAM_SAFEMODE)
	{
		
	}
}

void qcmosCamera::setScanNumber()
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

void qcmosCamera::setNumberKinetics(int number)
{
	if (!HAM_SAFEMODE)
	{
		
	}

}

void qcmosCamera::setFrameTransferMode()
{
	setFrameTransferMode(runSettings.frameTransferMode);
}

void qcmosCamera::setFrameTransferMode(int mode)
{
	if (!HAM_SAFEMODE)
	{
		
	}
}

void qcmosCamera::setFastKineticsEx()
{
	setFastKineticsEx(runSettings.imageSettings.height, runSettings.picsPerRepetition, runSettings.exposureTimes[0],
		4, runSettings.imageSettings.horizontalBinning, runSettings.imageSettings.verticalBinning, runSettings.imageSettings.bottom);
}

void qcmosCamera::setFastKineticsEx(int exposedRows, int seriesLength, float time, int mode, int hbin, int vbin, int offset)
{
	if (!HAM_SAFEMODE)
	{
		
	}
}

void qcmosCamera::checkAcquisitionTimings(float& kinetic, float& accumulation, std::vector<double>& exposures)
{
	//NOT IMPLEMENTED
}

///DUMMY FUNCTIONS END - AS I WAS WRITING THIS, I REALISED ITS PROBABLY BEST TO NOT INCLUDE FUNCTIONS THAT ARE USELESS

void qcmosCamera::setFanMode()
{

	if (!HAM_SAFEMODE)
	{
		try
		{
			DCAMERR err;
			if (runSettings.fanMode == "Fan On")
			{
				err = dcamprop_setvalue( hdcam, DCAM_IDPROP_SENSORCOOLERFAN, DCAMPROP_MODE__ON);
			}
			else
			{
				err = dcamprop_setvalue( hdcam, DCAM_IDPROP_SENSORCOOLERFAN, DCAMPROP_MODE__OFF);
				std::cout << "ehllo";

			}
		}
		catch(Error& err)
		{
			errBox(err.what());
		}

	}

	
}

void qcmosCamera::setCoolerMode()
{

	if (!HAM_SAFEMODE)
	{
		try
		{
			DCAMERR err;
			if (runSettings.coolerMode == "Sensor Cooler On")
			{
				err = dcamprop_setvalue( hdcam, DCAM_IDPROP_SENSORCOOLER, DCAMPROP_SENSORCOOLER__ON);
			}
			else if (runSettings.coolerMode == "Sensor Cooler Off")
			{
				err = dcamprop_setvalue( hdcam, DCAM_IDPROP_SENSORCOOLER, DCAMPROP_SENSORCOOLER__OFF);
				
			}
			else if (runSettings.coolerMode == "Sensor Cooler Max")
			{
				err = dcamprop_setvalue( hdcam, DCAM_IDPROP_SENSORCOOLER, DCAMPROP_SENSORCOOLER__MAX);
			}
		
		
		}
		catch(Error& err)
		{
			errBox(err.what());
		}
		
		
	}

	
}

void qcmosCamera::getTemperature(double& temp)
{
	if (!HAM_SAFEMODE)
	{
		DCAMERR err;
		err = dcamprop_getvalue( hdcam, DCAM_IDPROP_SENSORTEMPERATURE, &temp);
		if (failed(err))
		{
			throw(err);
		}

	}
}

void qcmosCamera::getTemperatureStatus(double& sensorStatus)
{
	if (!HAM_SAFEMODE)
	{
		DCAMERR err;
		err = dcamprop_getvalue( hdcam, DCAM_IDPROP_SENSORCOOLERSTATUS, &sensorStatus);
		if (failed(err))
		{
			throw(err);
		}

	}
}

int qcmosCamera::getTemperatureCode()
{
	double temp;
	int tempcode = MAXINT;
	if (!HAM_SAFEMODE)
	{
		
		DCAMERR err;
		err = dcamprop_getvalue( hdcam, DCAM_IDPROP_SENSORTEMPERATURE, &temp);
		if (failed(err))
		{
			throw(err);
		}
		tempcode = int(temp);
		
	}
	else 
	{
		return 0;
	}
}

void qcmosCamera::getAdjustedRingExposureTimes(int size, double* timesArray)
{
	DCAMERR err;
	err = dcamprop_getvalue( hdcam, DCAM_IDPROP_SENSORTEMPERATURE, &timesArray[0]);
	if (failed(err))
		{
			throw(err);
		}
		
}

//Apparently here temperature control cannot be turned on and off

void qcmosCamera::temperatureControlOn()
{
	if (!HAM_SAFEMODE)
	{
	}
}
void qcmosCamera::temperatureControlOff()
{
	if (!HAM_SAFEMODE)
	{
	}
}

///THE FUNCTIONS THAT ACTUALLY GET THE IMAGE
//You always want to get the oldest image and then make sure that it is deleted. DCAM SDK does not support this
// So we are going to have to do it ourselves
void qcmosCamera::getOldestImage(std::vector<long>& dataArray)
{
	//The function gets the oldest image - since with Andor, accessing an image would delete it from the buffer.
	//In this camera, that does not happen. So we have a variable called currentFrameIndex that is reset in the function
	//armCamera(). 
	if (!HAM_SAFEMODE)
	{
		// prepare the frame parameter - buff because we are at CU
		DCAMBUF_FRAME	bufframe;
		memset( &bufframe, 0, sizeof(bufframe) );
		bufframe.size		= sizeof(bufframe);
		bufframe.iFrame		= currentFrameIndex;				// currentFrame
		currentFrameIndex += 1;

		//access image
		DCAMERR err;
		try
		{
			// access image
			err = dcambuf_lockframe( hdcam, &bufframe );
		}
		catch(Error& err)
		{
			errBox(err.what());
		}

		////transfer image data to vector
		//int32 width = bufframe.width;
		//int32 height = bufframe.height;
		//char* pSrc = (char*) bufframe.buf;
		////create an empty pointer
		//char* buf = new char[width*height];
		//char* pDst = (char*) buf;

		//int idx;
		//for( idx = 0; idx < height; idx++ )
		//{
		//	memcpy_s( pDst, width, pSrc, width);

		//	pSrc += bufframe.rowbytes;
		//	pDst += width;
		//}

		//int size = sizeof(pDst[0]);

		//std::vector<long> vec(pDst, pDst + size);
		////now set dataArrat equal to this
		//dataArray = vec;

		int32 width = bufframe.width;
		int32 height = bufframe.height;
		uint8_t* pSrc = static_cast<uint8_t*>(bufframe.buf);

		// Create a vector to store long values
		std::vector<long> imageVector;


		for (int idx = 0; idx <= height; idx++)
		{
			// Convert each 8-bit pixel to a long and insert into the vector
			for (int i = 0; i <= width; i++)
			{
				imageVector.push_back(static_cast<long>(*pSrc));
				pSrc++;
			}
		}

		// Now, 'imageVector' contains the pixel data in a vector of long values

		dataArray = imageVector;
		std::cout << "Check";


		
	}
}

void qcmosCamera::setIsRunningState(bool state)
{
	cameraIsRunning = state;
}

void qcmosCamera::queryStatus(int32& status)
{
	if (!HAM_SAFEMODE)
	{
		
		DCAMERR err;
		//probably not the best place to set the capture mode
		err = dcamprop_setvalue(hdcam, DCAM_IDPROP_CAPTUREMODE, DCAMPROP_CAPTUREMODE__NORMAL);
		err = dcamcap_status(hdcam, &status);
		if (failed(err))
		{
			throw(err);
		}
		
	}
	else
	{
		status = DCAMCAP_STATUS_READY;
	}
}

void qcmosCamera::startAcquisition()
{
	if (!HAM_SAFEMODE)
	{
		DCAMERR err;
		err = dcamcap_start(hdcam, DCAMCAP_START_SNAP);
		if (failed(err))
		{
			throw(err);
		}
		
	}
}

void qcmosCamera::abortAcquisition()
{
	if (!HAM_SAFEMODE)
	{
		
		DCAMERR err;
		err = dcamcap_stop(hdcam);
		if (failed(err))
		{
			throw(err);
		}
		
		
	}
}

void qcmosCamera::getNumberOfPreAmpGains(int& number)
{
	if (!HAM_SAFEMODE)
	{
		number = 0;
	}
}



///
bool qcmosCamera::isRunning()
{
	return cameraIsRunning;
}

//This does nothing as of now, but needs to be there so that Chimera does not break
void qcmosCamera::setGainMode()
{
	
}

void qcmosCamera::setEmCcdGain(int gain)
{
	if (!HAM_SAFEMODE)
	{
		gain = 1;
	}
}

/*
* the input here will store how many whole pictures (not accumulations) have been taken.
*/
void qcmosCamera::getAcquisitionProgress(long& seriesNumber)
{
	if (!HAM_SAFEMODE)
	{
		// transferinfo param
		DCAMCAP_TRANSFERINFO captransferinfo;
		memset( &captransferinfo, 0, sizeof(captransferinfo) );
		captransferinfo.size	= sizeof(captransferinfo);
		DCAMERR err;
		
		// get number of captured image
		err = dcamcap_transferinfo( hdcam, &captransferinfo );
		if (failed(err))
		{
			throw(err);
		}
		seriesNumber = captransferinfo.nFrameCount;
		std::cout << "helloo";

	}
}

/*
* overload to get both the acccumulation progress and the whole picture progress.
*/
void qcmosCamera::getAcquisitionProgress(long& accumulationNumber, long& seriesNumber)
{
	if (!HAM_SAFEMODE)
	{
		// transferinfo param
		DCAMCAP_TRANSFERINFO captransferinfo;
		memset( &captransferinfo, 0, sizeof(captransferinfo) );
		captransferinfo.size	= sizeof(captransferinfo);
		DCAMERR err;

		// get number of captured image
		err = dcamcap_transferinfo( hdcam, &captransferinfo );
		if (failed(err))
		{
			throw(err);
		}
		seriesNumber = captransferinfo.nFrameCount;
		accumulationNumber = 1; //Always set this to 1 since we dont really do many accumulations per capture

	}
}

void qcmosCamera::waitForAcquisition(DCAMERR& err)
{
	if (!HAM_SAFEMODE)
	{

		// Set the wait parameters - wait for a new image - checks if the acqusition has finished
		DCAMWAIT_START waitstart;
		memset(&waitstart, 0, sizeof(waitstart));
		waitstart.size = sizeof(waitstart);
		waitstart.eventmask = DCAMWAIT_CAPEVENT_FRAMEREADY;
		waitstart.timeout = 100;

		// wait for image
		err = dcamwait_start(hwait, &waitstart);
		std::cout << "Hello";
	}
}




















































