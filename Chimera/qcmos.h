#pragma once
#include <string>
#include "CameraImageDimensions.h"
#include "Communicator.h"
#include <process.h>
#include <mutex>
#include "dcamapi4.h"
#include "dcamprop.h"
#include "qcmosRunSettings.h"

/// /////////////////////////////////////////////////////
/// 
///			The QCMOS Class
///
/// /////////////////////////////////////////////////////

// This class is designed to facilitate interaction with the qcmos camera and
// is based around the andor SDK. It is largely based on what was previously written for the ANDOR camera by Alec.

class qcmosCamera;

struct cameraThreadInput
{
	bool spuriousWakeupHandler;
	std::mutex runMutex;
	std::condition_variable signaler;
	Communicator* comm;
	// Andor is set to this in the constructor of the andor camera.
	qcmosCamera* qcmos;
	std::vector<std::chrono::time_point<std::chrono::high_resolution_clock>>* imageTimes;
};

/// the all-important camera class.
class qcmosCamera
{
public:
	HDCAM hdcam;
	DCAMAPI_INIT	apiinit;
	DCAMDEV_OPEN	devopen;
	DCAMWAIT_OPEN	waitopen;
	HDCAMWAIT hwait;
	DCAMWAIT_START waitstart;
	qcmosCamera::qcmosCamera();
	int currentFrameIndex;
	/// Andor Wrappers, in alphabetical order. Versions that take no parameters just insert current settings into 
	// the versions that take parameters. Note that my wrapper names don't always match the andor SDK names. If 
	// looking for specific sdk functions, search in the cpp file.
	void abortAcquisition();
	void checkForNewImages();
	void getAcquisitionProgress(long& seriesNumber);
	void getAcquisitionProgress(long& accumulationNumber, long& seriesNumber);
	void getAcquisitionTimes(float& exposure, float& accumulation, float& kinetic);
	void getAdjustedRingExposureTimes(int size, double* timesArray);
	void getNumberOfPreAmpGains(int& number);
	void getOldestImage(std::vector<long>& dataArray);
	void getNewestImage(std::vector<long>& dataArray);
	void getPreAmpGain(int index, float& gain);
	void queryStatus();
	void queryStatus(int32& status);
	void getTemperatureRange(int& min, int& max);
	void getTemperature(double& temp);
	void getTemperatureStatus(double& sensorStatus);
	int getTemperatureCode();
	void setFlag(int pictureNumber);
	void setInitial(bool initial);

	void setShutter(int typ, int mode, int closingtime, int openingtime);
	void setAccumulationCycleTime();
	void setAccumulationCycleTime(float time);
	void setAccumulationNumber(int number);
	void setAcquisitionMode();
	void setAcquisitionMode(int mode);
	void setFastKineticsEx();
	void setFastKineticsEx(int exposedRows, int seriesLength, float time, int mode, int hbin, int vbin, int offset);
	void setFKVShiftSpeed(int index);
	void setADChannel(int channel);
	void setEmCcdGain(int gain);
	void SetFastExternalTrigger(int mode);
	void setEmGainSettingsAdvanced(int state);
	void setFanMode();
	void setCoolerMode();
	void setFrameTransferMode();
	void setFrameTransferMode(int mode);
	void setHSSpeed(int type, int index);
	void setVSSpeed(int index);
	void setVSAmplitude(int index);
	void getVSSpeed(int index, float * speed);
	void getHSSpeed(int channel, int typ, int index, float *speed);
	void getNumberVSSpeeds(int *speeds);
	void getNumberHSSpeeds(int channel, int typ, int *speeds);
	void getFKVShiftSpeedF(int index, float *speed);
	void getFKExposureTime(float* time);

	void setImage(int hBin, int vBin, int lBorder, int rBorder, int tBorder, int bBorder);
	void setKineticCycleTime();
	void setKineticCycleTime(float cycleTime);
	void setNumberKinetics(int number);
	void setOutputAmplifier(int type);
	void setPreAmpGain(int index);
	void setReadMode();
	void setReadMode(int mode);
	void setRingExposureTimes(int sizeOfTimesArray, double* arrayOfTimes);
	void setSingleExposure();
	void setTemperature(int temp);
	void setTriggerMode(int mode);
	void startAcquisition();

	void temperatureControlOn();
	void temperatureControlOff();

	void waitForAcquisition(DCAMERR& err);

	// void getCapabilities(An& caps);
	// void getSerialNumber(int& num);
	std::string getHeadModel();

	/// End Andor sdk wrappers.

	// all of the following do something more interesting.
	//qcmosCamera::qcmosCamera();
	qcmosRunSettings getSettings();
	void pauseThread();
	void setSettings(qcmosRunSettings settingsToSet);
	void armCamera(CameraWindow* camWin, double& minKineticCycleTime);
	std::vector<std::vector<long>> acquireImageData();
	void setTemperature();
	void setExposures();
	void setImageParametersToCamera();
	void setScanNumber();
	double getMinKineticCycleTime();
	void checkAcquisitionTimings(float& kinetic, float& accumulation, std::vector<double>& exposures);
	void setNumberAccumulations(bool isKinetic);
	void setCameraTriggerMode();
	void onFinish();
	bool isRunning();
	void setIsRunningState(bool state);
	void updatePictureNumber(ULONGLONG newNumber);
	void setGainMode();
	void changeTemperatureSetting(bool temperatureControlOff);
	void andorErrorChecker(int errorCode);

	void initialize();
	void setBaselineClamp(int clamp);
	void setBaselineOffset(int offset);
	void setDMAParameters(int maxImagesPerDMA, float secondsPerDMA);

	static UINT __stdcall cameraThread(void* voidPtr);

	//void initializeClass( Communicator* comm, std::vector<std::chrono::time_point<std::chrono::high_resolution_clock>>* imageTimes );
	void initializeClass(Communicator* comm, chronoTimes* imageTimes);
	std::string getSystemInfo();

private:
	/// These are official settings and are the final say on what the camera does. Some unofficial 
	/// settings are stored in smaller classes.
	// If the experiment is running, these settings hold the options that the experiment is using.
	//AndorBaseSettings baseSettings;
	qcmosRunSettings runSettings;
	// ??? 
	imageParameters readImageParameters;
	imageParameters runningImageParameters;
	// 
	bool cameraIsRunning;
	// set either of these to true in order to break corresponding threads out of their loops.
	bool plotThreadExitIndicator;
	bool cameraThreadExitIndicator = false;

	ULONGLONG currentPictureNumber;
	ULONGLONG currentRepetitionNumber;

	HANDLE plottingMutex;
	// ???
	HANDLE imagesMutex;
	//
	std::vector<std::vector<long> > imagesOfExperiment;
	std::vector<std::vector<long> > imageVecQueue;
	UINT cameraThreadID = 0;

	cameraThreadInput threadInput;

};