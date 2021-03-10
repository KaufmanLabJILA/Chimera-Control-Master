#include "stdafx.h" 
#include "PictureSettingsControl.h" 
#include "Andor.h" 
#include "CameraSettingsControl.h" 
#include "CameraWindow.h" 
#include "Commctrl.h" 
#include <boost/lexical_cast.hpp> 

void PictureSettingsControl::initialize(cameraPositions& pos, CWnd* parent, int& id)
{
	picsPerRepetitionUnofficial = 1;

	// introducing things row by row 
	/// Set Picture Options 
	UINT count = 0;

	/// IMAGES PER REP CONTROL OVERRIDE 

	picsPerRepLabel.seriesPos = { pos.seriesPos.x, pos.seriesPos.y, pos.seriesPos.x + 200, pos.seriesPos.y + 25 };
	picsPerRepLabel.videoPos = { -1,-1,-1,-1 };
	picsPerRepLabel.amPos = { -1,-1,-1,-1 };
	picsPerRepLabel.Create("Images Per Repetition:", NORM_STATIC_OPTIONS, picsPerRepLabel.seriesPos, parent, PICTURE_SETTINGS_ID_START + count++);

	picsPerRepToggle.seriesPos = { pos.seriesPos.x + 200, pos.seriesPos.y, pos.seriesPos.x + 220, pos.seriesPos.y + 25 };
	picsPerRepToggle.videoPos = { -1,-1,-1,-1 };
	picsPerRepToggle.amPos = { -1,-1,-1,-1 };
	picsPerRepToggle.Create("", NORM_CHECK_OPTIONS, picsPerRepLabel.seriesPos, parent, PICTURE_SETTINGS_ID_START + count++);
	picsPerRepToggle.SetCheck(0);

	picsPerRepEdit.seriesPos = { pos.seriesPos.x + 220, pos.seriesPos.y, pos.seriesPos.x + 480, pos.seriesPos.y + 25 };
	picsPerRepEdit.videoPos = { -1,-1,-1,-1 };
	picsPerRepEdit.amPos = { -1,-1,-1,-1 };
	picsPerRepEdit.Create(NORM_EDIT_OPTIONS, picsPerRepToggle.seriesPos, parent, PICTURE_SETTINGS_ID_START + count++);
	picsPerRepEdit.SetWindowTextA("");

	pos.seriesPos.y += 25;
	pos.amPos.y += 25;
	pos.videoPos.y += 25;

	/// Picture Numbers 
	pictureLabel.seriesPos = { pos.seriesPos.x, pos.seriesPos.y, pos.seriesPos.x + 100, pos.seriesPos.y + 20 };
	pictureLabel.amPos = { pos.amPos.x, pos.amPos.y, pos.amPos.x + 100,	pos.amPos.y + 20 };
	pictureLabel.videoPos = { pos.videoPos.x, pos.videoPos.y, pos.videoPos.x + 100,	pos.videoPos.y + 20 };
	pictureLabel.Create("Picture #:", NORM_STATIC_OPTIONS, pictureLabel.seriesPos, parent,
		PICTURE_SETTINGS_ID_START + count++);

	for (int picInc = 0; picInc < 4; picInc++)
	{
		pictureNumbers[picInc].seriesPos = { pos.seriesPos.x + 100 + 95 * picInc, pos.seriesPos.y,
			pos.seriesPos.x + 100 + 95 * (picInc + 1), pos.seriesPos.y + 20 };
		pictureNumbers[picInc].amPos = { pos.amPos.x + 100 + 95 * picInc, pos.amPos.y, pos.amPos.x + 100 + 95 * (picInc + 1),
			pos.amPos.y + 20 };
		pictureNumbers[picInc].videoPos = { pos.videoPos.x + 100 + 95 * picInc, pos.videoPos.y,
			pos.videoPos.x + 100 + 95 * (picInc + 1), pos.videoPos.y + 20 };
		pictureNumbers[picInc].Create(cstr(picInc + 1), NORM_STATIC_OPTIONS,
			pictureNumbers[picInc].seriesPos, parent, PICTURE_SETTINGS_ID_START + count++);
	}
	pos.seriesPos.y += 20;
	pos.amPos.y += 20;
	pos.videoPos.y += 20;

	/// Total picture number 
	totalPicNumberLabel.seriesPos = { pos.seriesPos.x, pos.seriesPos.y, pos.seriesPos.x + 100, pos.seriesPos.y + 20 };
	totalPicNumberLabel.amPos = { pos.amPos.x, pos.amPos.y, pos.amPos.x + 100, pos.amPos.y + 20 };
	totalPicNumberLabel.videoPos = { pos.videoPos.x, pos.videoPos.y, pos.videoPos.x + 100, pos.videoPos.y + 20 };
	totalPicNumberLabel.Create("Total Picture #", NORM_STATIC_OPTIONS, totalPicNumberLabel.seriesPos, parent,
		PICTURE_SETTINGS_ID_START + count++);
	totalPicNumberLabel.fontType = SmallFont;
	for (int picInc = 0; picInc < 4; picInc++)
	{
		totalNumberChoice[picInc].seriesPos = { pos.seriesPos.x + 100 + 95 * picInc, pos.seriesPos.y,
			pos.seriesPos.x + 100 + 95 * (picInc + 1), pos.seriesPos.y + 20 };
		totalNumberChoice[picInc].amPos = { pos.amPos.x + 100 + 95 * picInc, pos.amPos.y, pos.amPos.x + 100 + 95 * (picInc + 1),
			pos.amPos.y + 20 };
		totalNumberChoice[picInc].videoPos = { pos.videoPos.x + 100 + 95 * picInc, pos.videoPos.y, pos.videoPos.x + 100 + 95 * (picInc + 1),
			pos.videoPos.y + 20 };
		if (picInc == 0)
		{
			// first of group 
			totalNumberChoice[picInc].Create("", NORM_RADIO_OPTIONS | WS_GROUP, totalNumberChoice[picInc].seriesPos,
				parent, PICTURE_SETTINGS_ID_START + count++);
			totalNumberChoice[picInc].SetCheck(1);
		}
		else
		{
			// members of group. 
			totalNumberChoice[picInc].Create("", NORM_RADIO_OPTIONS, totalNumberChoice[picInc].seriesPos, parent,
				PICTURE_SETTINGS_ID_START + count++);
		}
	}
	pos.seriesPos.y += 20;
	pos.amPos.y += 20;
	pos.videoPos.y += 20;

	/// Exposure Times 
	exposureLabel.seriesPos = { pos.seriesPos.x, pos.seriesPos.y, pos.seriesPos.x + 100, pos.seriesPos.y + 20 };
	exposureLabel.amPos = { pos.amPos.x, pos.amPos.y, pos.amPos.x + 100, pos.amPos.y + 20 };
	exposureLabel.videoPos = { pos.videoPos.x, pos.videoPos.y, pos.videoPos.x + 100, pos.videoPos.y + 20 };
	exposureLabel.Create("Exposure (ms):", NORM_STATIC_OPTIONS, exposureLabel.seriesPos, parent,
		PICTURE_SETTINGS_ID_START + count++);
	exposureLabel.fontType = SmallFont;


	exposureEdit.seriesPos = { pos.seriesPos.x + 100, pos.seriesPos.y,
		pos.seriesPos.x + 100 + 95, pos.seriesPos.y + 20 };
	exposureEdit.amPos = { pos.amPos.x + 100, pos.amPos.y,
		pos.amPos.x + 100 + 95, pos.amPos.y + 20 };
	exposureEdit.videoPos = { pos.videoPos.x + 100, pos.videoPos.y,
		pos.videoPos.x + 100 + 95, pos.videoPos.y + 20 };
	// first of group 
	exposureEdit.Create(NORM_EDIT_OPTIONS, exposureEdit.seriesPos, parent,
		PICTURE_SETTINGS_ID_START + count++);
	exposureEdit.SetWindowTextA("26.0");
	exposureTimesUnofficial = 26 / 1000.0f;

	pos.seriesPos.y += 20;
	pos.amPos.y += 20;
	pos.videoPos.y += 20;

	/// Thresholds 
	thresholdLabel.seriesPos = { pos.seriesPos.x, pos.seriesPos.y, pos.seriesPos.x + 100, pos.seriesPos.y + 20 };
	thresholdLabel.amPos = { pos.amPos.x, pos.amPos.y, pos.amPos.x + 100, pos.amPos.y + 20 };
	thresholdLabel.videoPos = { pos.videoPos.x, pos.videoPos.y, pos.videoPos.x + 100, pos.videoPos.y + 20 };
	thresholdLabel.Create("Threshold (cts)", NORM_STATIC_OPTIONS, thresholdLabel.seriesPos, parent,
		PICTURE_SETTINGS_ID_START + count++);
	thresholdLabel.fontType = SmallFont;
	for (int picInc = 0; picInc < 4; picInc++)
	{
		thresholdEdits[picInc].seriesPos = { pos.seriesPos.x + 100 + 95 * picInc, pos.seriesPos.y,
			pos.seriesPos.x + 100 + 95 * (picInc + 1), pos.seriesPos.y + 20 };
		thresholdEdits[picInc].amPos = { pos.amPos.x + 100 + 95 * picInc, pos.amPos.y,
			pos.amPos.x + 100 + 95 * (picInc + 1), pos.amPos.y + 20 };
		thresholdEdits[picInc].videoPos = { pos.videoPos.x + 100 + 95 * picInc, pos.videoPos.y,
			pos.videoPos.x + 100 + 95 * (picInc + 1), pos.videoPos.y + 20 };
		// first of group 
		thresholdEdits[picInc].Create(NORM_EDIT_OPTIONS, thresholdEdits[picInc].seriesPos, parent,
			PICTURE_SETTINGS_ID_START + count++);
		thresholdEdits[picInc].SetWindowTextA("100");
		thresholds[picInc] = 100;
	}
	pos.seriesPos.y += 20;
	pos.amPos.y += 20;
	pos.videoPos.y += 20;

	/// Yellow --> Blue Color 
	colormapLabel.seriesPos = { pos.seriesPos.x, pos.seriesPos.y, pos.seriesPos.x + 100, pos.seriesPos.y + 20 };
	colormapLabel.amPos = { pos.amPos.x, pos.amPos.y, pos.amPos.x + 100, pos.amPos.y + 20 };
	colormapLabel.videoPos = { pos.videoPos.x, pos.videoPos.y, pos.videoPos.x + 100, pos.videoPos.y + 20 };
	colormapLabel.Create("Virida", NORM_STATIC_OPTIONS, colormapLabel.seriesPos, parent,
		PICTURE_SETTINGS_ID_START + count++);


	/// The radio buttons 
	for (int picInc = 0; picInc < 4; picInc++)
	{
		colormapCombos[picInc].seriesPos = { pos.seriesPos.x + 100 + 95 * picInc, pos.seriesPos.y,
			pos.seriesPos.x + 100 + 95 * (picInc + 1), pos.seriesPos.y + 100 };
		colormapCombos[picInc].amPos = { pos.amPos.x + 100 + 95 * picInc, pos.amPos.y,
			pos.amPos.x + 100 + 95 * (picInc + 1), pos.amPos.y + 100 };
		colormapCombos[picInc].videoPos = { pos.videoPos.x + 100 + 95 * picInc, pos.videoPos.y,
			pos.videoPos.x + 100 + 95 * (picInc + 1), pos.videoPos.y + 100 };
		colormapCombos[picInc].Create(NORM_COMBO_OPTIONS, colormapCombos[picInc].seriesPos, parent,
			PICTURE_SETTINGS_ID_START + count++);
		colormapCombos[picInc].AddString("Dark Verida");
		colormapCombos[picInc].AddString("Inferno");
		colormapCombos[picInc].AddString("Black & White");
		colormapCombos[picInc].SetCurSel(0);
		colors[picInc] = 2;
	}
	pos.seriesPos.y += 20;
	pos.amPos.y += 20;
	pos.videoPos.y += 20;
	// above, the total number was set to 1. 
	enablePictureControls(0);
	disablePictureControls(1);
	disablePictureControls(2);
	disablePictureControls(3);
}



void PictureSettingsControl::handleNewConfig(std::ofstream& newFile)
{
	newFile << "PICTURE_SETTINGS\n";
	newFile << 1 << "\n";
	for (auto color : colors)
	{
		newFile << 0 << " ";
	}
	newFile << "\n";
	/*for (auto exposure : exposureTimesUnofficial)
	{*/
		// in seconds 
		newFile << 0.025 << " ";
	//}
	newFile << "\n";
	for (auto threshold : thresholds)
	{
		newFile << 200 << " ";
	}
	newFile << "\n";
	newFile << "END_PICTURE_SETTINGS\n";
}


void PictureSettingsControl::handleSaveConfig(std::ofstream& saveFile)
{
	saveFile << "PICTURE_SETTINGS\n";
	saveFile << picsPerRepManual << "\n";
	saveFile << picsPerRepetitionUnofficial << "\n";
	for (auto color : colors)
	{
		saveFile << color << " ";
	}
	saveFile << "\n";
	/*for (auto exposure : exposureTimesUnofficial)
	{*/
		saveFile << exposureTimesUnofficial << " ";
	//}
	saveFile << "\n";
	for (auto threshold : thresholds)
	{
		saveFile << threshold << " ";
	}
	saveFile << "\n";
	saveFile << "END_PICTURE_SETTINGS\n";
}


void PictureSettingsControl::handleOpenConfig(std::ifstream& openFile, int versionMajor, int versionMinor, AndorCamera* andor)
{
	ProfileSystem::checkDelimiterLine(openFile, "PICTURE_SETTINGS");
	UINT picsPerRep;
	if (versionMajor > 2) {
		openFile >> picsPerRepManual;
		openFile >> picsPerRep;
		picsPerRepEdit.SetWindowTextA(cstr(picsPerRep));
		picsPerRepToggle.SetCheck(picsPerRepManual);
		setUnofficialPicsPerRep(picsPerRep, andor);
	}
	else {
		openFile >> picsPerRep;
		setUnofficialPicsPerRep(picsPerRep, andor);
	}

	std::array<int, 4> fileThresholds;
	for (auto& color : colors)
	{
		openFile >> color;
	}
	/*for (auto& exposure : exposureTimesUnofficial)
	{*/
		openFile >> exposureTimesUnofficial;
	//}
	for (auto& threshold : fileThresholds)
	{
		openFile >> threshold;
	}
	setExposureTimes(andor);
	setThresholds(fileThresholds);
	ProfileSystem::checkDelimiterLine(openFile, "END_PICTURE_SETTINGS");
}


void PictureSettingsControl::disablePictureControls(int pic)
{
	if (pic > 3)
	{
		return;
	}
	//exposureEdit.EnableWindow(0);
	thresholdEdits[pic].EnableWindow(0);
	colormapCombos[pic].EnableWindow(0);
}


void PictureSettingsControl::enablePictureControls(int pic)
{
	if (pic > 3)
	{
		return;
	}
	exposureEdit.EnableWindow();
	thresholdEdits[pic].EnableWindow();
	colormapCombos[pic].EnableWindow();
}


CBrush* PictureSettingsControl::colorControls(int id, CDC* colorer, brushMap brushes, rgbMap rgbs)
{
	/// Exposures 
	if (id == exposureEdit.GetDlgCtrlID())
	{
		//int picNum = id - exposureEdit.GetDlgCtrlID();
		if (!exposureEdit.IsWindowEnabled())
		{
			return NULL;
		}
		colorer->SetTextColor(rgbs["theme foreground"]);
		//TCHAR textEdit[256]; 
		CString text;
		exposureEdit.GetWindowTextA(text);
		double exposure;
		try
		{
			exposure = std::stof(str(text));// / 1000.0f; 
			double dif = std::fabs(exposure / 1000.0 - exposureTimesUnofficial);
			if (dif < 0.000000001)
			{
				// good. 
				colorer->SetBkColor(rgbs["theme green"]);
				// catch change of color and redraw window. 
				if (exposureEdit.colorState != 0)
				{
					exposureEdit.colorState = 0;
					exposureEdit.RedrawWindow();
				}
				return brushes["theme green"];
			}
		}
		catch (std::exception&)
		{
			// don't do anything with it. 
		}
		colorer->SetBkColor(rgbs["Red"]);
		// catch change of color and redraw window. 
		if (exposureEdit.colorState != 1)
		{
			exposureEdit.colorState = 1;
			exposureEdit.RedrawWindow();
		}
		return brushes["Red"];
	}
	/// Thresholds 
	else if (id >= thresholdEdits.front().GetDlgCtrlID() && id <= thresholdEdits.back().GetDlgCtrlID())
	{
		int picNum = id - thresholdEdits.front().GetDlgCtrlID();
		if (!thresholdEdits[picNum].IsWindowEnabled())
		{
			return NULL;
		}
		colorer->SetTextColor(RGB(255, 255, 255));
		CString text;
		thresholdEdits[picNum].GetWindowTextA(text);
		int threshold;
		try
		{
			threshold = std::stoi(str(text));
			double dif = std::fabs(threshold - thresholds[picNum]);
			if (dif < 0.000000001)
			{
				// good. 
				colorer->SetBkColor(rgbs["theme green"]);
				// catch change of color and redraw window. 
				if (thresholdEdits[picNum].colorState != 0)
				{
					thresholdEdits[picNum].colorState = 0;
					thresholdEdits[picNum].RedrawWindow();
				}
				return brushes["theme green"];
			}
		}
		catch (std::exception&)
		{
			// don't do anything with it. 
		}
		colorer->SetBkColor(rgbs["Red"]);
		// catch change of color and redraw window. 
		if (exposureEdit.colorState != 1)
		{
			exposureEdit.colorState = 1;
			exposureEdit.RedrawWindow();
		}
		return brushes["Red"];
	}
	else
	{
		return NULL;
	}
}


UINT PictureSettingsControl::getPicsPerRepetition()
{
	CString txt;
	picsPerRepEdit.GetWindowTextA(txt);
	if (picsPerRepManual)
	{
		try
		{
			picsPerRepetitionUnofficial = boost::lexical_cast<UINT>(txt);
		}
		catch (boost::bad_lexical_cast& err)
		{
			thrower("Failed to convert pics per repetition to unsigned int!");
		}
		picsPerRepManual = true;
	}
	else
	{
		picsPerRepManual = false;
	}
	return picsPerRepetitionUnofficial;
}


void PictureSettingsControl::setUnofficialPicsPerRep(UINT picNum, AndorCamera* andorObj)
{
	if (picsPerRepManual) {
		picsPerRepetitionUnofficial = getPicsPerRepetition();
		picNum = 1;
	}
	else {
		picsPerRepetitionUnofficial = picNum;
		picsPerRepEdit.SetWindowTextA(cstr(picNum));
	}
	// not all settings are changed here, and some are used to recalculate totals. 
	AndorRunSettings settings = andorObj->getSettings();
	settings.picsPerRepetition = picsPerRepetitionUnofficial;
	settings.totalPicsInVariation = settings.picsPerRepetition  * settings.repetitionsPerVariation;
	if (settings.totalVariations * settings.totalPicsInVariation > INT_MAX)
	{
		thrower("ERROR: too many pictures to take! Maximum number of pictures possible is " + str(INT_MAX));
	}
	settings.totalPicsInExperiment = int(settings.totalVariations * settings.totalPicsInVariation);
	andorObj->setSettings(settings);
	for (UINT picInc = 0; picInc < 4; picInc++)
	{
		if (picInc < picNum)
		{
			enablePictureControls(picInc);
		}
		else
		{
			disablePictureControls(picInc);
		}
		if (picInc == picNum - 1)
		{
			totalNumberChoice[picInc].SetCheck(1);
		}
		else
		{
			totalNumberChoice[picInc].SetCheck(0);
		}
	}
}


void PictureSettingsControl::handleOptionChange(int id, AndorCamera* andorObj)
{
	setPicsPerRepManual();
	if (id == picsPerRepToggle.GetDlgCtrlID()) {
		setUnofficialPicsPerRep(1, andorObj);
	}
	else if (id >= totalNumberChoice.front().GetDlgCtrlID() && id <= totalNumberChoice.back().GetDlgCtrlID())
	{
		int picNum = id - totalNumberChoice.front().GetDlgCtrlID();
		if (totalNumberChoice[picNum].GetCheck())
		{
			setUnofficialPicsPerRep(picNum + 1, andorObj);
		}
		// this message can weirdly get set after a configuration opens as well, it only means to set the number if the  
		// relevant button is now checked. 
	}
	else if (id >= colormapCombos[0].GetDlgCtrlID() && id <= colormapCombos[3].GetDlgCtrlID())
	{
		id -= colormapCombos[0].GetDlgCtrlID();
		int color = colormapCombos[id].GetCurSel();
		colors[id] = color;
	}
}


void PictureSettingsControl::setExposureTimes(AndorCamera* andorObj)
{
	setExposureTimes(exposureTimesUnofficial, andorObj);
}


void PictureSettingsControl::setExposureTimes(float& time, AndorCamera* andorObj)
{
	float exposuresToSet;
	exposuresToSet = time;
	/*if (picsPerRepManual) {
		exposuresToSet.resize(1);
	}
	else {
		exposuresToSet.resize(picsPerRepetitionUnofficial);
	}*/
	AndorRunSettings settings = andorObj->getSettings();
	settings.exposureTime = exposuresToSet;
	andorObj->setSettings(settings);
	// try to set this time. 
	andorObj->setExposures();
	// now check actual times. 
	try { parentSettingsControl->checkTimings(exposuresToSet); }
	catch (std::runtime_error&) { throw; }

	exposureTimesUnofficial = exposuresToSet;

	// now output things. 
	exposureEdit.SetWindowTextA(cstr(this->exposureTimesUnofficial * 1000));
}



float PictureSettingsControl::getUsedExposureTimes()
{
	updateSettings();
	float usedTimes;
	usedTimes = exposureTimesUnofficial;

	return usedTimes;
}

/*
 * modifies values for exposures, accumlation time, kinetic cycle time as the andor camera reports them.
 */
void PictureSettingsControl::confirmAcquisitionTimings()
{
	float usedExposures;
	usedExposures = exposureTimesUnofficial;

	try
	{
		parentSettingsControl->checkTimings(usedExposures);
	}
	catch (std::runtime_error)
	{
		throw;
	}

	exposureTimesUnofficial = usedExposures;

}

/**/
std::array<int, 4> PictureSettingsControl::getThresholds()
{
	updateSettings();
	return thresholds;
}

void PictureSettingsControl::setThresholds(std::array<int, 4> newThresholds)
{
	thresholds = newThresholds;
	for (UINT thresholdInc = 0; thresholdInc < thresholds.size(); thresholdInc++)
	{
		thresholdEdits[thresholdInc].SetWindowTextA(cstr(thresholds[thresholdInc]));
	}
}

void PictureSettingsControl::setPicsPerRepManual() {
	picsPerRepManual = picsPerRepToggle.GetCheck();
}

//void PictureSettingsControl::setPicturesPerExperiment(UINT pics, AndorCamera* andorObj) 
//{ 
//	if (pics > 4) 
//	{ 
//		return; 
//	} 
//	picsPerRepetitionUnofficial = pics; 
//	AndorRunSettings settings = andorObj->getSettings(); 
//	settings.picsPerRepetition = picsPerRepetitionUnofficial; 
//	settings.totalPicsInVariation = settings.picsPerRepetition  * settings.repetitionsPerVariation; 
//	if (settings.totalVariations * settings.totalPicsInVariation > INT_MAX) 
//	{ 
//		thrower( "ERROR: Trying to take too many pictures! Maximum picture number is " + str( INT_MAX ) ); 
//	} 
//	settings.totalPicsInExperiment = int(settings.totalVariations * settings.totalPicsInVariation); 
//	for (UINT picInc = 0; picInc < 4; picInc++) 
//	{ 
//		if (picInc == pics - 1) 
//		{ 
//			totalNumberChoice[picInc].SetCheck(1); 
//		} 
//		else 
//		{ 
//			totalNumberChoice[picInc].SetCheck(0); 
//		} 
// 
//		if (picInc < picsPerRepetitionUnofficial) 
//		{ 
//			enablePictureControls(picInc); 
//		} 
//		else 
//		{ 
//			disablePictureControls(picInc); 
//		} 
//	} 
//} 


/*
*/
std::array<int, 4> PictureSettingsControl::getPictureColors()
{
	return colors;
}


void PictureSettingsControl::updateSettings()
{
	// grab the thresholds 
	for (int thresholdInc = 0; thresholdInc < 4; thresholdInc++)
	{
		CString textEdit;
		thresholdEdits[thresholdInc].GetWindowTextA(textEdit);
		int threshold;
		try
		{
			threshold = std::stoi(str(textEdit));
			thresholds[thresholdInc] = threshold;
		}
		catch (std::invalid_argument)
		{
			errBox("ERROR: failed to convert threshold number " + str(thresholdInc + 1) + " to an integer.");
		}
		thresholdEdits[thresholdInc].RedrawWindow();
	}
	// grab the exposure. 

	CString textEdit;
	exposureEdit.GetWindowTextA(textEdit);
	float exposure;
	try
	{
		exposure = std::stof(str(textEdit));
		exposureTimesUnofficial = exposure / 1000.0f;
	}
	catch (std::invalid_argument)
	{
		errBox("ERROR: failed to convert exposure number to a float.");
	}
	// refresh for new color 
	exposureEdit.RedrawWindow();

}


void PictureSettingsControl::rearrange(std::string cameraMode, std::string triggerMode, int width, int height,
	fontMap fonts)
{
	totalPicNumberLabel.rearrange(cameraMode, triggerMode, width, height, fonts);
	pictureLabel.rearrange(cameraMode, triggerMode, width, height, fonts);
	exposureLabel.rearrange(cameraMode, triggerMode, width, height, fonts);
	thresholdLabel.rearrange(cameraMode, triggerMode, width, height, fonts);
	colormapLabel.rearrange(cameraMode, triggerMode, width, height, fonts);
	picsPerRepLabel.rearrange(cameraMode, triggerMode, width, height, fonts);
	picsPerRepToggle.rearrange(cameraMode, triggerMode, width, height, fonts);
	picsPerRepEdit.rearrange(cameraMode, triggerMode, width, height, fonts);

	for (auto& control : pictureNumbers)
	{
		control.rearrange(cameraMode, triggerMode, width, height, fonts);
	}
	for (auto& control : totalNumberChoice)
	{
		control.rearrange(cameraMode, triggerMode, width, height, fonts);
	}

	exposureEdit.rearrange(cameraMode, triggerMode, width, height, fonts);

	for (auto& control : thresholdEdits)
	{
		control.rearrange(cameraMode, triggerMode, width, height, fonts);
	}
	for (auto& control : colormapCombos)
	{
		control.rearrange(cameraMode, triggerMode, width, height, fonts);
	}
}
