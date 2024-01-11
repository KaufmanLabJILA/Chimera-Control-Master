#include "stdafx.h"
#include "CameraSettingsControl.h"
#include "CameraWindow.h"
#include "miscellaneousCommonFunctions.h"


CameraSettingsControl::CameraSettingsControl(qcmosCamera* friendInitializer) : picSettingsObj(this)
{
	qcmosFriend = friendInitializer;
	// initialize settings. Most of these have been picked to match initial settings set in the "initialize" 
	// function.

	picSettingsObj.picsPerRepManual = false;

	runSettings.exposureTimes = { 0.026 };
	runSettings.picsPerRepetition = 1;
	runSettings.kineticCycleTime = 0.1f;
	runSettings.repetitionsPerVariation = 10;
	runSettings.totalVariations = 3;
	runSettings.totalPicsInExperiment = 30;
	runSettings.totalPicsInVariation = 10;
	// the read mode never gets changed currently. we always want images.
	runSettings.readMode = 4;
	runSettings.acquisitionMode = 3;
	runSettings.emGainModeIsOn = false;
	runSettings.showPicsInRealTime = false;
	runSettings.triggerMode = "External Trigger";
}


void CameraSettingsControl::initialize(cameraPositions& pos, int& id, CWnd* parent, cToolTips& tooltips)
{
	/// Header
	header.seriesPos = { pos.seriesPos.x, pos.seriesPos.y, pos.seriesPos.x + 480, pos.seriesPos.y += 25 };
	header.amPos = { pos.amPos.x, pos.amPos.y, pos.amPos.x + 480, pos.amPos.y += 25 };
	header.videoPos = { pos.videoPos.x, pos.videoPos.y, pos.videoPos.x + 480, pos.videoPos.y += 25 };
	header.Create("CAMERA SETTINGS", NORM_HEADER_OPTIONS, header.seriesPos, parent, id++);
	header.fontType = HeadingFont;

	/// camera mode
	cameraModeCombo.seriesPos = { pos.seriesPos.x, pos.seriesPos.y, pos.seriesPos.x + 480, pos.seriesPos.y + 100 };
	cameraModeCombo.amPos = { pos.amPos.x, pos.amPos.y, pos.amPos.x + 480, pos.amPos.y + 100 };
	cameraModeCombo.videoPos = { pos.videoPos.x, pos.videoPos.y, pos.videoPos.x + 480, pos.videoPos.y + 100 };
	cameraModeCombo.Create(NORM_COMBO_OPTIONS, cameraModeCombo.seriesPos, parent, IDC_CAMERA_MODE_COMBO);
	cameraModeCombo.AddString("Kinetic Series Mode");
	// cameraModeCombo.AddString("Accumulation Mode");
	// cameraModeCombo.AddString("Video Mode");
	// cameraModeCombo.AddString("Fast Kinetics Mode");
	cameraModeCombo.SelectString(0, "Kinetic Series Mode");
	runSettings.cameraMode = "Kinetic Series Mode";
	pos.amPos.y += 25;
	pos.videoPos.y += 25;
	pos.seriesPos.y += 25;
	/// EM Gain
	emGainButton.seriesPos = { pos.seriesPos.x, pos.seriesPos.y, pos.seriesPos.x + 120, pos.seriesPos.y + 20 };
	emGainButton.videoPos = { pos.videoPos.x, pos.videoPos.y, pos.videoPos.x + 120, pos.videoPos.y + 20 };
	emGainButton.amPos = { pos.amPos.x, pos.amPos.y, pos.amPos.x + 120, pos.amPos.y + 20 };
	emGainButton.Create("Set Gain", NORM_PUSH_OPTIONS, emGainButton.seriesPos, parent, IDC_SET_EM_GAIN_BUTTON);
	emGainButton.setToolTip("Set the gain of the of the camera. The program will immediately change the state of the camera after pressing this button.",
		tooltips, parent);
	//
	emGainEdit.seriesPos = { pos.seriesPos.x + 120, pos.seriesPos.y, pos.seriesPos.x + 300, pos.seriesPos.y + 20 };
	emGainEdit.amPos = { pos.amPos.x + 120, pos.amPos.y, pos.amPos.x + 300, pos.amPos.y + 20 };
	emGainEdit.videoPos = { pos.videoPos.x + 120, pos.videoPos.y, pos.videoPos.x + 300, pos.videoPos.y + 20 };
	emGainEdit.Create(NORM_EDIT_OPTIONS, emGainEdit.seriesPos, parent, id++);
	//
	emGainDisplay.seriesPos = { pos.seriesPos.x + 300, pos.seriesPos.y, pos.seriesPos.x + 480, pos.seriesPos.y + 20 };
	emGainDisplay.videoPos = { pos.videoPos.x + 300, pos.videoPos.y, pos.videoPos.x + 480, pos.videoPos.y + 20 };
	emGainDisplay.amPos = { pos.amPos.x + 300, pos.amPos.y, pos.amPos.x + 480, pos.amPos.y + 20 };
	emGainDisplay.Create("OFF", NORM_STATIC_OPTIONS, emGainDisplay.seriesPos, parent, id++);
	// initialize settings.
	runSettings.emGainLevel = 0;
	runSettings.emGainModeIsOn = true;
	//
	pos.seriesPos.y += 20;
	pos.amPos.y += 20;
	pos.videoPos.y += 20;

	// trigger combo
	triggerCombo.seriesPos = { pos.seriesPos.x, pos.seriesPos.y, pos.seriesPos.x + 480, pos.seriesPos.y + 800 };
	triggerCombo.videoPos = { pos.videoPos.x, pos.videoPos.y,pos.videoPos.x + 480, pos.videoPos.y + 800 };
	triggerCombo.amPos = { pos.amPos.x, pos.amPos.y, pos.amPos.x + 480, pos.amPos.y + 800 };
	triggerCombo.Create(NORM_COMBO_OPTIONS, triggerCombo.seriesPos, parent, IDC_TRIGGER_COMBO);
	// set options for the combo
	triggerCombo.AddString("Internal Trigger");
	triggerCombo.AddString("External Trigger");
	triggerCombo.AddString("Start On Trigger");
	// Select default trigger
	triggerCombo.SelectString(0, "External Trigger");
	pos.seriesPos.y += 25;
	pos.amPos.y += 25;
	pos.videoPos.y += 25;
	runSettings.triggerMode = "External Trigger";
	// Set temperature Button
	setTemperatureButton.seriesPos = { pos.seriesPos.x, pos.seriesPos.y, pos.seriesPos.x + 270, pos.seriesPos.y + 25 };
	setTemperatureButton.videoPos = { pos.videoPos.x, pos.videoPos.y, pos.videoPos.x + 270, pos.videoPos.y + 25 };
	setTemperatureButton.amPos = { pos.amPos.x, pos.amPos.y, pos.amPos.x + 270, pos.amPos.y + 25 };
	setTemperatureButton.Create("Set Camera Temperature (C)", NORM_PUSH_OPTIONS, setTemperatureButton.seriesPos,
		parent, IDC_SET_TEMPERATURE_BUTTON);
	// Temperature Edit
	temperatureEdit.seriesPos = { pos.seriesPos.x + 270, pos.seriesPos.y, pos.seriesPos.x + 350, pos.seriesPos.y + 25 };
	temperatureEdit.videoPos = { pos.videoPos.x + 270, pos.videoPos.y, pos.videoPos.x + 350, pos.videoPos.y + 25 };
	temperatureEdit.amPos = { pos.amPos.x + 270, pos.amPos.y, pos.amPos.x + 350, pos.amPos.y + 25 };
	temperatureEdit.Create(NORM_EDIT_OPTIONS, temperatureEdit.seriesPos, parent, id++);
	temperatureEdit.SetWindowTextA("0");
	// Temperature Setting Display
	temperatureDisplay.seriesPos = { pos.seriesPos.x + 350, pos.seriesPos.y, pos.seriesPos.x + 430, pos.seriesPos.y + 25 };
	temperatureDisplay.videoPos = { pos.videoPos.x + 350, pos.videoPos.y, pos.videoPos.x + 430, pos.videoPos.y + 25 };
	temperatureDisplay.amPos = { pos.amPos.x + 350, pos.amPos.y, pos.amPos.x + 430, pos.amPos.y + 25 };
	temperatureDisplay.Create("", NORM_STATIC_OPTIONS, temperatureDisplay.seriesPos, parent, id++);
	// Temperature Control Off Button
	temperatureOffButton.seriesPos = { pos.seriesPos.x + 430, pos.seriesPos.y, pos.seriesPos.x + 480, pos.seriesPos.y + 25 };
	temperatureOffButton.videoPos = { pos.videoPos.x + 430, pos.videoPos.y, pos.videoPos.x + 480, pos.videoPos.y + 25 };
	temperatureOffButton.amPos = { pos.amPos.x + 430, pos.amPos.y, pos.amPos.x + 480, pos.amPos.y + 25 };
	temperatureOffButton.Create("OFF", NORM_PUSH_OPTIONS, temperatureOffButton.seriesPos, parent, IDC_SET_TEMPERATURE_OFF_BUTTON);
	pos.seriesPos.y += 25;
	pos.amPos.y += 25;
	pos.videoPos.y += 25;
	// Temperature Message Display
	temperatureMsg.seriesPos = { pos.seriesPos.x, pos.seriesPos.y, pos.seriesPos.x + 480, pos.seriesPos.y + 50 };
	temperatureMsg.videoPos = { pos.videoPos.x, pos.videoPos.y, pos.videoPos.x + 480, pos.videoPos.y + 50 };
	temperatureMsg.amPos = { pos.amPos.x, pos.amPos.y, pos.amPos.x + 480, pos.amPos.y + 50 };
	temperatureMsg.Create("Temperature control is disabled", NORM_STATIC_OPTIONS, temperatureMsg.seriesPos, parent,
		id++);
	pos.seriesPos.y += 50;
	pos.amPos.y += 50;
	pos.videoPos.y += 50;
	// INSERT ABOVE
	// cooler combo
	coolerCombo.seriesPos = { pos.seriesPos.x, pos.seriesPos.y, pos.seriesPos.x + 480, pos.seriesPos.y + 80};
	coolerCombo.videoPos = { pos.videoPos.x, pos.videoPos.y,pos.videoPos.x + 480, pos.videoPos.y + 80};
	coolerCombo.amPos = { pos.amPos.x, pos.amPos.y, pos.amPos.x + 480, pos.amPos.y + 80};
	coolerCombo.Create(NORM_COMBO_OPTIONS, coolerCombo.seriesPos, parent, IDC_COOLER_COMBO);
	// set options for the combo
	coolerCombo.AddString("Sensor Cooler On");
	coolerCombo.AddString("Sensor Cooler Off");
	coolerCombo.AddString("Sensor Cooler Max");
	// Select default trigger
	coolerCombo.SelectString(0, "Sensor Cooler On");
	pos.seriesPos.y += 25;
	pos.amPos.y += 25;
	pos.videoPos.y += 25;
	// fan combo
	fanCombo.seriesPos = { pos.seriesPos.x, pos.seriesPos.y, pos.seriesPos.x + 480, pos.seriesPos.y+80};
	fanCombo.videoPos = { pos.videoPos.x, pos.videoPos.y,pos.videoPos.x + 480, pos.videoPos.y+80};
	fanCombo.amPos = { pos.amPos.x, pos.amPos.y, pos.amPos.x + 480, pos.amPos.y+80};
	fanCombo.Create(NORM_COMBO_OPTIONS, coolerCombo.seriesPos, parent, IDC_FAN_COMBO);
	// set options for the combo
	fanCombo.AddString("Fan On");
	fanCombo.AddString("Fan Off");
	// Select default trigger
	fanCombo.SelectString(0, "Fan On");

	pos.seriesPos.y += 25;
	pos.amPos.y += 25;
	pos.videoPos.y += 25;

	

	//
	picSettingsObj.initialize(pos, parent, id);

	imageDimensionsObj.initialize(pos, parent, false, id);

	//After initialize
	pos.seriesPos.y += 25;
	pos.amPos.y += 25;
	pos.videoPos.y += 25;

	// /// REPETITIONS PER VARIATION

	// Accumulation Time
	accumulationCycleTimeLabel.seriesPos = { -1,-1,-1,-1 };
	accumulationCycleTimeLabel.videoPos = { -1,-1,-1,-1 };
	accumulationCycleTimeLabel.amPos = { pos.amPos.x,pos.amPos.y,pos.amPos.x + 240,pos.amPos.y + 25 };
	accumulationCycleTimeLabel.Create("Accumulation Cycle Time", NORM_STATIC_OPTIONS,
		accumulationCycleTimeLabel.seriesPos, parent, id++);

	accumulationCycleTimeEdit.seriesPos = { -1,-1,-1,-1 };
	accumulationCycleTimeEdit.videoPos = { -1,-1,-1,-1 };
	accumulationCycleTimeEdit.amPos = { pos.amPos.x + 240,pos.amPos.y,pos.amPos.x + 480, pos.amPos.y += 25 };
	accumulationCycleTimeEdit.Create(NORM_EDIT_OPTIONS, accumulationCycleTimeEdit.seriesPos, parent, id++);
	accumulationCycleTimeEdit.SetWindowTextA("0.1");

	// Accumulation Number
	accumulationNumberLabel.seriesPos = { -1,-1,-1,-1 };
	accumulationNumberLabel.videoPos = { -1,-1,-1,-1 };
	accumulationNumberLabel.amPos = { pos.amPos.x,pos.amPos.y,pos.amPos.x + 240,pos.amPos.y + 25 };
	accumulationNumberLabel.Create("Accumulation #", NORM_STATIC_OPTIONS, accumulationNumberLabel.seriesPos, parent,
		id++);
	//
	accumulationNumberEdit.seriesPos = { -1,-1,-1,-1 };
	accumulationNumberEdit.videoPos = { -1,-1,-1,-1 };
	accumulationNumberEdit.amPos = { pos.amPos.x + 240,pos.amPos.y,pos.amPos.x + 480,pos.amPos.y += 25 };
	accumulationNumberEdit.Create(NORM_EDIT_OPTIONS, accumulationNumberEdit.seriesPos, parent, id++);
	accumulationNumberEdit.SetWindowTextA("1");

	// // minimum trigger interval (determined by camera)
	// minKineticCycleTimeLabel.seriesPos = { pos.seriesPos.x, pos.seriesPos.y, pos.seriesPos.x + 240, pos.seriesPos.y + 25 };
	// minKineticCycleTimeLabel.videoPos = { -1,-1,-1,-1 };
	// minKineticCycleTimeLabel.amPos = { -1,-1,-1,-1 };
	// minKineticCycleTimeLabel.Create("Minimum trigger Interval (s)", NORM_STATIC_OPTIONS,
	// 	minKineticCycleTimeLabel.seriesPos, parent, id++);

	// minKineticCycleTimeDisp.seriesPos = { pos.seriesPos.x + 240, pos.seriesPos.y, pos.seriesPos.x + 480, pos.seriesPos.y += 25 };
	// minKineticCycleTimeDisp.videoPos = { -1,-1,-1,-1 };
	// minKineticCycleTimeDisp.amPos = { -1,-1,-1,-1 };
	// minKineticCycleTimeDisp.Create(NORM_STATIC_OPTIONS, minKineticCycleTimeDisp.seriesPos, parent, id++);
	// minKineticCycleTimeDisp.SetWindowTextA("");

	// /// Frame Rate
	// // Frame Rate Label
	// kineticCycleTimeLabel.seriesPos = { pos.seriesPos.x, pos.seriesPos.y, pos.seriesPos.x + 240, pos.seriesPos.y + 25 };
	// kineticCycleTimeLabel.videoPos = { -1,-1,-1,-1 };
	// kineticCycleTimeLabel.amPos = { -1,-1,-1,-1 };
	// kineticCycleTimeLabel.triggerModeSensitive = -1;
	// kineticCycleTimeLabel.Create("Maximum Internal Frame Rate", NORM_STATIC_OPTIONS, kineticCycleTimeLabel.seriesPos, parent,
	// 	id++);

	// // Kinetic Cycle Time Edit
	// kineticCycleTimeEdit.seriesPos = { pos.seriesPos.x + 240, pos.seriesPos.y, pos.seriesPos.x + 480, pos.seriesPos.y += 25 };
	// kineticCycleTimeEdit.videoPos = { -1,-1,-1,-1 };
	// kineticCycleTimeEdit.amPos = { -1,-1,-1,-1 };
	// kineticCycleTimeEdit.triggerModeSensitive = -1;
	// kineticCycleTimeEdit.Create(NORM_EDIT_OPTIONS, kineticCycleTimeEdit.seriesPos, parent, id++);
	// kineticCycleTimeEdit.SetWindowTextA("0.1");
}
	// INSERT ABOVE
	// cooler combo
	// coolerCombo.seriesPos = { pos.seriesPos.x, pos.seriesPos.y, pos.seriesPos.x + 480, pos.seriesPos.y + 80};
	// coolerCombo.videoPos = { pos.videoPos.x, pos.videoPos.y,pos.videoPos.x + 480, pos.videoPos.y + 80};
	// coolerCombo.amPos = { pos.amPos.x, pos.amPos.y, pos.amPos.x + 480, pos.amPos.y + 80};
	// coolerCombo.Create(NORM_COMBO_OPTIONS, coolerCombo.seriesPos, parent, IDC_COOLER_COMBO);
	// // set options for the combo
	// coolerCombo.AddString("Sensor Cooler On");
	// coolerCombo.AddString("Sensor Cooler Off");
	// coolerCombo.AddString("Sensor Cooler Max");
	// // Select default trigger
	// coolerCombo.SelectString(0, "Sensor Cooler On");
	// pos.seriesPos.y += 25;
	// pos.amPos.y += 25;
	// pos.videoPos.y += 25;
	// // fan combo
	// fanCombo.seriesPos = { pos.seriesPos.x, pos.seriesPos.y, pos.seriesPos.x + 480, pos.seriesPos.y+80};
	// .videoPos = { pos.videoPos.x, pos.videoPos.y,pos.videoPos.x + 480, pos.videoPos.y+80};
	// fanCombo.amPos = { pos.amPos.x, pos.amPos.y, pos.amPos.x + 480, pos.amPos.y+80};
	// fanCombo.Create(NORM_COMBO_OPTIONS, coolerCombo.seriesPos, parent, IDC_FAN_COMBO);
	// // set options for the combo
	// fanCombo.AddString("Fan On");
	// fanCombo.AddString("Fan Off");
	// // Select default trigger
	// fanCombo.SelectString(0, "Fan On");

	// pos.seriesPos.y += 25;
	// pos.amPos.y += 25;
	// pos.videoPos.y += 25;

	// After initialize
	// pos.seriesPos.y += 25;
	// pos.amPos.y += 25;
	// pos.videoPos.y += 25;



// note that this object doesn't actually store the camera state, it just uses it in passing to figure out whether 
// buttons should be on or off.
void CameraSettingsControl::cameraIsOn(bool state)
{
	// Can't change em gain mode or camera settings once started.
	emGainButton.EnableWindow(!state);
	setTemperatureButton.EnableWindow(!state);
	temperatureOffButton.EnableWindow(!state);
}


std::array<int, 4> CameraSettingsControl::getThresholds()
{
	return picSettingsObj.getThresholds();
}

void CameraSettingsControl::setRunSettings(qcmosRunSettings inputSettings)
{
	if (inputSettings.emGainModeIsOn == false || inputSettings.emGainLevel < 0)
	{
		emGainEdit.SetWindowTextA("-1");
		emGainDisplay.SetWindowTextA("OFF");
	}
	else
	{
		emGainEdit.SetWindowTextA(cstr(inputSettings.emGainLevel));
		emGainDisplay.SetWindowTextA(cstr("X" + str(inputSettings.emGainLevel)));
	}
	qcmosFriend->setGainMode();
	// try to set this time.
	picSettingsObj.setExposureTimes(inputSettings.exposureTimes, qcmosFriend);
	// now check actual times.
	checkTimings(inputSettings.exposureTimes);
	///
	// kineticCycleTimeEdit.SetWindowTextA(cstr(inputSettings.kineticCycleTime));
	// accumulationCycleTimeEdit.SetWindowTextA(cstr(inputSettings.accumulationTime));
	cameraModeCombo.SelectString(0, cstr(inputSettings.cameraMode));
	if (inputSettings.cameraMode == "Video Mode")
	{
		inputSettings.acquisitionMode = 5;
		inputSettings.totalPicsInVariation = INT_MAX;
	}
	else if (inputSettings.cameraMode == "Kinetic Series Mode")
	{
		inputSettings.acquisitionMode = 3;
	}
	else if (inputSettings.cameraMode == "Fast Kinetics Mode")
	{
		inputSettings.acquisitionMode = 4;
	}
	else if (inputSettings.cameraMode == "Accumulate Mode")
	{
		inputSettings.acquisitionMode = 2;
		inputSettings.totalPicsInVariation = INT_MAX;
	}
	// kineticCycleTimeEdit.SetWindowTextA(cstr(inputSettings.kineticCycleTime));
	// accumulationCycleTimeEdit.SetWindowTextA(cstr(inputSettings.accumulationTime * 1000.0));
	accumulationNumberEdit.SetWindowTextA(cstr(inputSettings.accumulationNumber));
	temperatureEdit.SetWindowTextA(cstr(inputSettings.temperatureSetting));
}


void CameraSettingsControl::handleSetTemperatureOffPress()
{
	qcmosFriend->changeTemperatureSetting(true);
}

void CameraSettingsControl::handleSetTemperaturePress()
{
	if (qcmosFriend->isRunning())
	{
		thrower("ERROR: the camera (thinks that it?) is running. You can't change temperature settings during camera operation.");
	}

	//runSettings = qcmosFriend->getSettings();
	CString text;
	temperatureEdit.GetWindowTextA(text);
	int temp;
	try
	{
		temp = std::stoi(str(text));
	}
	catch (std::invalid_argument&)
	{
		thrower("Error: Couldn't convert temperature input to a double! Check for unusual characters.");
	}
	runSettings.temperatureSetting = temp;
	qcmosFriend->setSettings(runSettings);

	qcmosFriend->setTemperature();
	//eCameraFileSystem.updateSaveStatus(false);
}


void CameraSettingsControl::handleTriggerControl(CameraWindow* cameraWindow)
{
	CString triggerMode;
	int itemIndex = triggerCombo.GetCurSel();
	if (itemIndex == -1)
	{
		return;
	}
	triggerCombo.GetLBText(itemIndex, triggerMode);
	std::cout << itemIndex;
	runSettings.triggerMode = str(triggerMode);
	qcmosFriend->setSettings(runSettings);
	CRect rect;
	cameraWindow->GetWindowRect(&rect);
	cameraWindow->OnSize(0, rect.right - rect.left, rect.bottom - rect.top);
	qcmosFriend->setCameraTriggerMode();
}

void CameraSettingsControl::handleCoolerControl(CameraWindow* cameraWindow)
{
	if (qcmosFriend->isRunning())
	{
		thrower("ERROR: the camera (thinks that it?) is running. You can't change temperature settings during camera operation.");
	}
	CString coolerMode;
	int itemIndex = coolerCombo.GetCurSel();
	if (itemIndex == -1)
	{
		return;
	}
	coolerCombo.GetLBText(itemIndex, coolerMode);
	runSettings.coolerMode = str(coolerMode);
	qcmosFriend->setSettings(runSettings);
	CRect rect;
	cameraWindow->GetWindowRect(&rect);
	cameraWindow->OnSize(0, rect.right - rect.left, rect.bottom - rect.top);
	qcmosFriend->setCoolerMode();
}

void CameraSettingsControl::handleFanControl(CameraWindow* cameraWindow)
{
	if (qcmosFriend->isRunning())
	{
		thrower("ERROR: the camera (thinks that it?) is running. You can't change temperature settings during camera operation.");
	}
	CString fanMode;
	int itemIndex = fanCombo.GetCurSel();
	if (itemIndex == -1)
	{
		return;
	}
	fanCombo.GetLBText(itemIndex, fanMode);
	runSettings.fanMode = str(fanMode);
	qcmosFriend->setSettings(runSettings);
	CRect rect;
	cameraWindow->GetWindowRect(&rect);
	cameraWindow->OnSize(0, rect.right - rect.left, rect.bottom - rect.top);
	qcmosFriend->setFanMode();
}

qcmosRunSettings CameraSettingsControl::getSettings()
{
	return runSettings;
}

void CameraSettingsControl::rearrange(std::string cameraMode, std::string triggerMode, int width, int height, fontMap fonts)
{
	imageDimensionsObj.rearrange(cameraMode, triggerMode, width, height, fonts);
	picSettingsObj.rearrange(cameraMode, triggerMode, width, height, fonts);
	header.rearrange(cameraMode, triggerMode, width, height, fonts);
	cameraModeCombo.rearrange(cameraMode, triggerMode, width, height, fonts);
	emGainButton.rearrange(cameraMode, triggerMode, width, height, fonts);
	emGainDisplay.rearrange(cameraMode, triggerMode, width, height, fonts);
	emGainEdit.rearrange(cameraMode, triggerMode, width, height, fonts);
	triggerCombo.rearrange(cameraMode, triggerMode, width, height, fonts);
	coolerCombo.rearrange(cameraMode, triggerMode, width, height, fonts);
	fanCombo.rearrange(cameraMode, triggerMode, width, height, fonts);
	setTemperatureButton.rearrange(cameraMode, triggerMode, width, height, fonts);
	temperatureOffButton.rearrange(cameraMode, triggerMode, width, height, fonts);
	temperatureEdit.rearrange(cameraMode, triggerMode, width, height, fonts);
	temperatureDisplay.rearrange(cameraMode, triggerMode, width, height, fonts);
	temperatureMsg.rearrange(cameraMode, triggerMode, width, height, fonts);
	kineticCycleTimeEdit.rearrange(cameraMode, triggerMode, width, height, fonts);
	kineticCycleTimeLabel.rearrange(cameraMode, triggerMode, width, height, fonts);
	accumulationCycleTimeEdit.rearrange(cameraMode, triggerMode, width, height, fonts);
	accumulationCycleTimeLabel.rearrange(cameraMode, triggerMode, width, height, fonts);
	accumulationNumberEdit.rearrange(cameraMode, triggerMode, width, height, fonts);
	accumulationNumberLabel.rearrange(cameraMode, triggerMode, width, height, fonts);
	minKineticCycleTimeLabel.rearrange(cameraMode, triggerMode, width, height, fonts);
	minKineticCycleTimeDisp.rearrange(cameraMode, triggerMode, width, height, fonts);
}

BOOL CameraSettingsControl::getPicsPerRepManual() {
	return picSettingsObj.picsPerRepManual;
}

void CameraSettingsControl::setEmGain(qcmosCamera* qcmosObj)
{
	CString emGainText;
	emGainEdit.GetWindowTextA(emGainText);
	int emGain;
	try
	{
		emGain = std::stoi(str(emGainText));
	}
	catch (std::invalid_argument&)
	{
		thrower("ERROR: Couldn't convert EM Gain text to integer.");
	}
	// < 0 corresponds to NOT USING EM NIAWG_GAIN (using conventional gain).
	if (emGain < 0)
	{
		runSettings.emGainModeIsOn = false;
		runSettings.emGainLevel = 0;
		emGainDisplay.SetWindowTextA("OFF");
	}
	else
	{
		runSettings.emGainModeIsOn = true;
		runSettings.emGainLevel = emGain;
		emGainDisplay.SetWindowTextA(cstr("Gain: X" + str(runSettings.emGainLevel)));
	}
	// Change the andor settings.
	qcmosRunSettings settings = qcmosObj->getSettings();
	settings.emGainLevel = runSettings.emGainLevel;
	settings.emGainModeIsOn = runSettings.emGainModeIsOn;
	qcmosObj->setSettings(settings);
	// and immediately change the EM gain mode.
	qcmosObj->setGainMode();
	emGainEdit.RedrawWindow();
}


void CameraSettingsControl::setVariationNumber(UINT varNumber)
{
	runSettings.totalVariations = varNumber;
	if (runSettings.totalVariations * runSettings.totalPicsInVariation > INT_MAX)
	{
		thrower("ERROR: Trying to take too many pictures! Maximum picture number is " + str(INT_MAX));
	}
	runSettings.totalPicsInExperiment = int(runSettings.totalVariations * runSettings.totalPicsInVariation);
}


void CameraSettingsControl::setRepsPerVariation(UINT repsPerVar)
{
	runSettings.repetitionsPerVariation = repsPerVar;
	runSettings.totalPicsInVariation = runSettings.repetitionsPerVariation * runSettings.picsPerRepetition;
	if (runSettings.totalVariations * runSettings.totalPicsInVariation > INT_MAX)
	{
		thrower("ERROR: Trying to take too many pictures! Maximum picture number is " + str(INT_MAX));
	}
	runSettings.totalPicsInExperiment = int(runSettings.totalVariations * runSettings.totalPicsInVariation);
}


void CameraSettingsControl::handleTimer()
{
	// This case displays the current temperature in the main window. When the temp stabilizes at the desired 
	// level the appropriate message is displayed.
	// initial value is only relevant for safemode.
	double currentTemperature;
	double setTemperature;
	try
	{
		// in this case you expect it to throw.
		// setTemperature = qcmosFriend->getSettings().temperatureSetting;
		setTemperature = -10;
		qcmosFriend->getTemperature(currentTemperature);
		//if (HAM_SAFEMODE) { thrower("SAFEMODE"); }
		double sensorStatus;
		qcmosFriend->getTemperatureStatus(sensorStatus);

		// if not stable this won't get changed.
		if (sensorStatus == DCAMPROP_SENSORCOOLERSTATUS__READY)
		{
			currentControlColor = "Green";
			temperatureDisplay.SetWindowTextA(cstr(setTemperature));
			temperatureMsg.SetWindowTextA(cstr("Temperature has stabilized at " + str(currentTemperature)
				+ " (C)\r\n"));
		}

		else if (sensorStatus == DCAMPROP_SENSORCOOLERSTATUS__BUSY)
		{
			currentControlColor = "Red";
			temperatureDisplay.SetWindowTextA(cstr(setTemperature));
			temperatureMsg.SetWindowTextA(cstr("Set temperature not yet reached. Current temperature is "
				+ str(currentTemperature) + " (C)\r\n"));
		}
		// else if (exception.whatBare() == "DRV_TEMPERATURE_NOT_STABILIZED")
		// {
		// 	currentControlColor = "Red";
		// 	temperatureDisplay.SetWindowTextA(cstr(setTemperature));
		// 	temperatureMsg.SetWindowTextA(cstr("Temperature of " + str(currentTemperature)
		// 		+ " (C) reached but not stable."));
		// }
		// else if (exception.whatBare() == "DRV_TEMPERATURE_DRIFT")
		// {
		// 	currentControlColor = "Red";
		// 	temperatureDisplay.SetWindowTextA(cstr(setTemperature));
		// 	temperatureMsg.SetWindowTextA(cstr("Temperature had stabilized but has since drifted. Temperature: "
		// 		+ str(currentTemperature)));
		// }
		else if (sensorStatus == DCAMPROP_SENSORCOOLERSTATUS__OFF)
		{
			currentControlColor = "Red";
			temperatureDisplay.SetWindowTextA(cstr(setTemperature));
			temperatureMsg.SetWindowTextA(cstr("Temperature control is off. Temperature: " + str(currentTemperature)));
		}
		// else if (exception.whatBare() == "DRV_ACQUIRING")
		// {
		// 	// doesn't change color of temperature control. This way the color of the control represents the state of
		// 	// the temperature right before the acquisition started, so that you can tell if you remembered to let it
		// 	// completely stabilize or not.
		// 	temperatureDisplay.SetWindowTextA(cstr(setTemperature));
		// 	temperatureMsg.SetWindowTextA("Camera is Acquiring data. No Temperature updates are available.");
		// }
		else if (HAM_SAFEMODE)
		{
			currentControlColor = "Red";
			temperatureDisplay.SetWindowTextA(cstr(setTemperature));
			temperatureMsg.SetWindowTextA("Application is running in Safemode... No Real Temperature Data is available.");
		}
		else
		{
			currentControlColor = "Red";
			temperatureDisplay.SetWindowTextA(cstr(currentTemperature));
			temperatureMsg.SetWindowTextA(cstr("Invalid Code. Temperature: "+ str(currentTemperature)));
		}

		
	}
	catch (Error& exception)
	{
		errBox(exception.what());
	}
}


void CameraSettingsControl::updateRunSettingsFromPicSettings()
{
	runSettings.showPicsInRealTime = picSettingsObj.picsPerRepManual;
	runSettings.exposureTimes = picSettingsObj.getUsedExposureTimes();
	runSettings.picsPerRepetition = picSettingsObj.getPicsPerRepetition();
	runSettings.totalPicsInVariation = runSettings.picsPerRepetition * runSettings.repetitionsPerVariation;
	if (runSettings.totalVariations * runSettings.totalPicsInVariation > INT_MAX)
	{
		thrower("ERROR: Trying to take too many pictures! Maximum picture number is " + str(INT_MAX));
	}
	runSettings.totalPicsInExperiment = runSettings.totalVariations * runSettings.totalPicsInVariation;
}


void CameraSettingsControl::handlePictureSettings(UINT id, qcmosCamera* qcmosObj)
{
	picSettingsObj.handleOptionChange(id, qcmosObj);
	updateRunSettingsFromPicSettings();
}


/*
 * This function checks things that don't have "Set" buttons. should be called to load the most recent values.
 */
void CameraSettingsControl::updatePassivelySetSettings()

//COMMENTED THIS OUT
{
	CString text;
	// kineticCycleTimeEdit.GetWindowTextA(text);
	// try
	// {
	// 	runSettings.kineticCycleTime = std::stof(str(text));
	// 	kineticCycleTimeEdit.SetWindowTextA(cstr(runSettings.kineticCycleTime));
	// }
	// catch (std::invalid_argument&)
	// {
	// 	runSettings.kineticCycleTime = 0.1f;
	// 	kineticCycleTimeEdit.SetWindowTextA(cstr(runSettings.kineticCycleTime));
	// 	thrower("Please enter a valid float for the kinetic cycle time.");
	// }

	// accumulationCycleTimeEdit.GetWindowTextA(text);
	// try
	// {
	// 	runSettings.accumulationTime = std::stof(str(text));
	// 	accumulationCycleTimeEdit.SetWindowTextA(cstr(runSettings.accumulationTime));
	// }
	// catch (std::invalid_argument&)
	// {
	// 	runSettings.accumulationTime = 0.1f;
	// 	accumulationCycleTimeEdit.SetWindowTextA(cstr(runSettings.accumulationTime));
	// 	thrower("Please enter a valid float for the accumulation cycle time.");
	// }

	accumulationNumberEdit.GetWindowTextA(text);
	try
	{
		runSettings.accumulationNumber = std::stol(str(text));
		accumulationNumberEdit.SetWindowTextA(cstr(runSettings.accumulationNumber));
	}
	catch (std::invalid_argument&)
	{
		runSettings.accumulationNumber = 1;
		accumulationNumberEdit.SetWindowTextA(cstr(runSettings.accumulationNumber));
		thrower("Please enter a valid float for the Accumulation number.");
	}
}


std::array<int, 4> CameraSettingsControl::getPaletteNumbers()
{
	return picSettingsObj.getPictureColors();
}




void CameraSettingsControl::handleOpenConfig(std::ifstream& configFile, int versionMajor, int versionMinor)
{
	ProfileSystem::checkDelimiterLine(configFile, "CAMERA_SETTINGS");
	qcmosRunSettings tempSettings;
	configFile.get();
	std::getline(configFile, tempSettings.triggerMode);
	configFile >> tempSettings.emGainModeIsOn;
	configFile >> tempSettings.emGainLevel;
	configFile.get();
	std::getline(configFile, tempSettings.cameraMode);
	if (tempSettings.cameraMode == "Video Mode")
	{
		tempSettings.acquisitionMode = 5;
		tempSettings.totalPicsInVariation = INT_MAX;
	}
	else if (tempSettings.cameraMode == "Kinetic Series Mode")
	{
		tempSettings.acquisitionMode = 3;
	}
	else if (tempSettings.cameraMode == "Fast Kinetics Mode")
	{
		tempSettings.acquisitionMode = 4;
	}
	else if (tempSettings.cameraMode == "Accumulate Mode")
	{
		tempSettings.acquisitionMode = 2;
		tempSettings.totalPicsInVariation = INT_MAX;
	}
	else
	{
		thrower("ERROR: Unrecognized camera mode!");
	}
	configFile >> tempSettings.kineticCycleTime;
	configFile >> tempSettings.accumulationTime;
	configFile >> tempSettings.accumulationNumber;
	configFile >> tempSettings.temperatureSetting;
	setRunSettings(tempSettings);
	ProfileSystem::checkDelimiterLine(configFile, "END_CAMERA_SETTINGS");
	picSettingsObj.handleOpenConfig(configFile, versionMajor, versionMinor, qcmosFriend);
	//setRunSettings(tempSettings);
	updateRunSettingsFromPicSettings();
	if ((versionMajor == 2 && versionMinor > 4) || versionMajor > 2)
	{
		imageDimensionsObj.handleOpen(configFile, versionMajor, versionMinor);
	}
}


void CameraSettingsControl::handleNewConfig(std::ofstream& newFile)
{
	newFile << "CAMERA_SETTINGS\n";
	newFile << "External Trigger" << "\n";
	newFile << 0 << "\n";
	newFile << 0 << "\n";
	newFile << "Kinetic Series Mode" << "\n";
	newFile << 1000 << "\n";
	newFile << 1000 << "\n";
	newFile << 2 << "\n";
	newFile << 25 << "\n";
	newFile << "END_CAMERA_SETTINGS\n";
	picSettingsObj.handleNewConfig(newFile);
	imageDimensionsObj.handleNew(newFile);
}


void CameraSettingsControl::handleSaveConfig(std::ofstream& saveFile)
{
	saveFile << "CAMERA_SETTINGS\n";
	saveFile << runSettings.triggerMode << "\n";
	saveFile << runSettings.emGainModeIsOn << "\n";
	saveFile << runSettings.emGainLevel << "\n";
	saveFile << runSettings.cameraMode << "\n";
	saveFile << runSettings.kineticCycleTime << "\n";
	saveFile << runSettings.accumulationTime << "\n";
	saveFile << runSettings.accumulationNumber << "\n";
	saveFile << runSettings.temperatureSetting << "\n";
	saveFile << "END_CAMERA_SETTINGS\n";

	picSettingsObj.handleSaveConfig(saveFile);
	imageDimensionsObj.handleSave(saveFile);
}


void CameraSettingsControl::handleModeChange(CameraWindow* cameraWindow)
{
	int sel = cameraModeCombo.GetCurSel();
	if (sel == -1)
	{
		return;
	}
	CString mode;
	cameraModeCombo.GetLBText(sel, mode);
	runSettings.cameraMode = mode;


	//						
	if (runSettings.cameraMode == "Video Mode")
	{
		runSettings.acquisitionMode = 5;
		runSettings.totalPicsInVariation = INT_MAX;
		runSettings.repetitionsPerVariation = runSettings.totalPicsInVariation / runSettings.picsPerRepetition;

	}
	else if (runSettings.cameraMode == "Kinetic Series Mode")
	{
		runSettings.acquisitionMode = 3;
	}
	else if (runSettings.cameraMode == "Fast Kinetics Mode")
	{
		runSettings.acquisitionMode = 4;
	}
	else if (runSettings.cameraMode == "Accumulate Mode")
	{
		runSettings.acquisitionMode = 2;
	}


	CRect rect;
	cameraWindow->GetWindowRect(&rect);
	cameraWindow->OnSize(0, rect.right - rect.left, rect.bottom - rect.top);
}


void CameraSettingsControl::checkTimings(std::vector<double>& exposureTimes)
{
	checkTimings(runSettings.kineticCycleTime, runSettings.accumulationTime, exposureTimes);
}


void CameraSettingsControl::checkTimings(float& kineticCycleTime, float& accumulationTime, std::vector<double>& exposureTimes)
{
	qcmosFriend->checkAcquisitionTimings(kineticCycleTime, accumulationTime, exposureTimes);
}


void CameraSettingsControl::updateMinKineticCycleTime(double time)
{
	minKineticCycleTimeDisp.SetWindowTextA(cstr(time));
}


imageParameters CameraSettingsControl::readImageParameters(CameraWindow* camWin)
{
	imageParameters parameters = imageDimensionsObj.readImageParameters(camWin);
	runSettings.imageSettings = parameters;
	return parameters;
}

CBrush* CameraSettingsControl::handleColor(int idNumber, CDC* colorer, brushMap brushes, rgbMap rgbs)
{
	return picSettingsObj.colorControls(idNumber, colorer, brushes, rgbs);
}


void CameraSettingsControl::setImageParameters(imageParameters newSettings, CameraWindow* camWin)
{
	imageDimensionsObj.setImageParametersFromInput(newSettings, camWin);
}


void CameraSettingsControl::checkIfReady()
{
	if (picSettingsObj.getUsedExposureTimes().size() == 0)
	{
		thrower("Please Set at least one exposure time.");
	}
	if (!imageDimensionsObj.checkReady())
	{
		thrower("Please set the image parameters.");
	}
	if (runSettings.picsPerRepetition <= 0)
	{
		thrower("ERROR: Please set the number of pictures per repetition to a positive non-zero value.");
	}
	if (runSettings.cameraMode == "Kinetic Series Mode")
	{
		if (runSettings.kineticCycleTime == 0 && runSettings.triggerMode == "Internal Trigger")
		{
			thrower("ERROR: Since you are running in internal trigger mode, please Set a kinetic cycle time.");
		}
		if (runSettings.repetitionsPerVariation <= 0)
		{
			thrower("ERROR: Please set the \"Repetitions Per Variation\" variable to a positive non-zero value.");
		}
		if (runSettings.totalVariations <= 0)
		{
			thrower("ERROR: Please set the number of variations to a positive non-zero value.");
		}
	}
	if (runSettings.cameraMode == "Accumulate Mode")
	{
		if (runSettings.accumulationNumber <= 0)
		{
			thrower("ERROR: Please set the current Accumulation Number to a positive non-zero value.");
		}
		if (runSettings.accumulationTime <= 0)
		{
			thrower("ERROR: Please set the current Accumulation Time to a positive non-zero value.");
		}
	}
}