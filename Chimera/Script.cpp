#include "stdafx.h"

#include <fstream>
#include <sstream>
#include <algorithm>
#include <unordered_map>
#include "boost/lexical_cast.hpp"

#include "Script.h"

#include "cleanString.h"
#include "TextPromptDialog.h"
#include "Richedit.h"
#include "VariableSystem.h"
#include "ProfileSystem.h"
#include "AuxiliaryWindow.h"
#include "DioSystem.h"
#include "RunInfo.h"


void Script::initialize( int width, int height, POINT& startingLocation, cToolTips& toolTips, CWnd* parent, 
						 int& id, std::string deviceTypeInput, std::string scriptHeader,
						 std::array<UINT, 2> ids, COLORREF backgroundColor)
{
	AfxInitRichEdit();
	InitCommonControls();
	LoadLibrary( TEXT( "Msftedit.dll" ) );
	deviceType = deviceTypeInput;
	if (deviceTypeInput == "Horizontal NIAWG" || deviceTypeInput == "Vertical NIAWG")
	{
		extension = str( "." ) + NIAWG_SCRIPT_EXTENSION;
	}
	else if (deviceTypeInput == "Agilent")
	{
		extension = str( "." ) + AGILENT_SCRIPT_EXTENSION;
	}
	else if (deviceTypeInput == "Master")
	{
		extension = str( "." ) + MASTER_SCRIPT_EXTENSION;
	}
	//else if (deviceTypeInput == "Moog")
	//{
	//	extension = str(".") + MOOG_SCRIPT_EXTENSION;
	//}
	else if (deviceTypeInput == "AWG")
	{
		extension = str(".") + AWG_SCRIPT_EXTENSION;
	}
	else if (deviceTypeInput == "Gmoog")
	{
		extension = str(".") + GIGAMOOG_SCRIPT_EXTENSION;
	}
	else if (deviceTypeInput == "DDS")
	{
		extension = str(".") + DDS_SCRIPT_EXTENSION;
	}
	else
	{
		thrower( "ERROR: Device input type not recognized during construction of script control." );
	}
	CHARFORMAT myCharFormat;
	memset( &myCharFormat, 0, sizeof( CHARFORMAT ) );
	myCharFormat.cbSize = sizeof( CHARFORMAT );
	myCharFormat.dwMask = CFM_COLOR;
	myCharFormat.crTextColor = RGB( 255, 255, 255 );
	// title
	if (scriptHeader != "")
	{
		// user has option to not have a header if scriptheader is "".
		title.sPos = { startingLocation.x, startingLocation.y, startingLocation.x + width, startingLocation.y + 25 };
		title.Create( cstr( scriptHeader ), NORM_HEADER_OPTIONS, title.sPos, parent, id++ );
		title.fontType = HeadingFont;
		startingLocation.y += 25;
	}
	// saved indicator
	savedIndicator.sPos = { startingLocation.x, startingLocation.y, startingLocation.x + 80, startingLocation.y + 20 };
	savedIndicator.Create( "Saved?", NORM_CWND_OPTIONS | BS_CHECKBOX | BS_LEFTTEXT, savedIndicator.sPos, parent, id++ );
	savedIndicator.SetCheck( true );
	// filename
	fileNameText.sPos = { startingLocation.x + 80, startingLocation.y, startingLocation.x + width - 20, startingLocation.y + 20 };
	fileNameText.Create( NORM_STATIC_OPTIONS, fileNameText.sPos, parent, id++ );
	isSaved = true;
	// help
	help.sPos = { startingLocation.x + width - 20, startingLocation.y, startingLocation.x + width, startingLocation.y + 20 };
	help.Create( NORM_STATIC_OPTIONS, help.sPos, parent, id++ );
	help.SetWindowTextA( "?" );
	// don't want this for the scripting window, hence the extra check.
	//if (deviceType == "Agilent" && ids[0] != IDC_INTENSITY_FUNCTION_COMBO)
	//{
	//	help.setToolTip( AGILENT_INFO_TEXT, toolTips, parent );
	//}
	//help.setToolTip( "", toolTips, parent );
	startingLocation.y += 20;
	// available functions combo
	availableFunctionsCombo.sPos = { startingLocation.x, startingLocation.y, startingLocation.x + width, startingLocation.y + 800 };
	availableFunctionsCombo.Create( NORM_COMBO_OPTIONS, availableFunctionsCombo.sPos, parent, ids[0] );
	
	loadFunctions( ); 
	// select "parent script".
	availableFunctionsCombo.SetCurSel( 0 );

	startingLocation.y += 25;
	// Edit
	edit.sPos = { startingLocation.x, startingLocation.y, startingLocation.x + width, startingLocation.y += height };
	edit.Create( NORM_EDIT_OPTIONS | ES_AUTOVSCROLL | WS_VSCROLL | ES_AUTOHSCROLL | WS_HSCROLL, edit.sPos, parent, ids[1] );
	edit.fontType = CodeFont;
	edit.SetBackgroundColor( FALSE, backgroundColor );
	edit.SetEventMask( ENM_CHANGE );
	edit.SetDefaultCharFormat( myCharFormat );

	// timer
	syntaxTimer.Create( 0, NULL, 0, { 0,0,0,0 }, parent, 0 );
}


void Script::rearrange(UINT width, UINT height, fontMap fonts)
{
	edit.rearrange( width, height, fonts);
	title.rearrange( width, height, fonts);
	savedIndicator.rearrange( width, height, fonts);
	fileNameText.rearrange( width, height, fonts);
	availableFunctionsCombo.rearrange( width, height, fonts);
	help.rearrange( width, height, fonts);
}


void Script::functionChangeHandler(std::string categoryPath)
{
	int selection = availableFunctionsCombo.GetCurSel( );
	if ( selection != -1 )
	{
		CString text;
		availableFunctionsCombo.GetLBText( selection, text );
		std::string textStr( text.GetBuffer( ) );
		textStr = textStr.substr( 0, textStr.find_first_of( '(' ) );
		changeView( textStr, true, categoryPath );
	}
}


Script::Script()
{
	isSaved = true;
	editChangeEnd = 0;
	editChangeBegin = ULONG_MAX;
}

std::string Script::getScriptPath()
{
	return scriptPath;
}

std::string Script::getScriptText()
{
	CString text;
	edit.GetWindowText(text);
	return str(text);
}


COLORREF Script::getSyntaxColor( std::string word, std::string editType, std::vector<variableType> variables, 
								 rgbMap rgbs, bool& colorLine, 
								 std::array<std::array<std::string, 16>, 4> ttlNames, 
								 std::array<std::string, 24> dacNames)
{
	// convert word to lower case.
	std::transform( word.begin(), word.end(), word.begin(), ::tolower );
	// check special cases
	if (word.size() == 0)
	{
		// nothing??
		return rgbs["theme red"];
	}
	else if (word[0] == '%')
	{
		// comments
		colorLine = true;
		if (word.size() > 1)
		{
			if (word[1] == '%')
			{
				return rgbs["Slate Green"];
			}
		}
		return rgbs["theme comment"];
	}
	// Check NIAWG-specific commands
	//if ( editType == "Horizontal NIAWG" || editType == "Vertical NIAWG" )
	//{
	//	for ( auto num : range( 10 ) )
	//	{
	//		if ( word == "gen" + str( num + 1 ) + "const" || word == "gen" + str( num + 1 ) + "ampramp"
	//			|| word == "gen" + str( num + 1 ) + "freqramp" || word == "gen" + str( num + 1 ) + "freq&ampramp"
	//			 || word == "flash" || word == "rearrange")
	//		{
	//			return rgbs["theme purple"];
	//		}
	//	}
	//	// check logic
	//	if ( word == "repeattiltrig" || word == "repeatSet#" || word == "repeattilsoftwaretrig" || word == "endrepeat" 
	//		 || word == "repeatforever" )
	//	{
	//		return rgbs["theme blue"];
	//	}
	//	// check options
	//	if (word == "lin" || word == "nr" || word == "tanh")
	//	{
	//		return rgbs["theme green"];
	//	}
	//	// check variable
	//	else if (word == "{" || word == "}")
	//	{
	//		return rgbs["theme aqua"];
	//	}
	//	if (word.size() > 8)
	//	{
	//		if (word.substr(word.size() - 8, 8) == ".nScript")
	//		{
	//			return rgbs["theme yellow"];
	//		}
	//	}
	//}
	// Check Moog-specific commands
	//if (editType == "Moog") {
	//	if (word == "startfreq"|| word == "stopfreq"|| word == "gain"|| word == "loadphase"|| word == "movephase"|| word == "onoff"|| word == "step")
	//	{
	//		return rgbs["theme purple"];
	//	}
	//	// check logic
	//	if (word == "linloop"|| word=="customsequence")
	//	{
	//		return rgbs["theme blue"];
	//	}
	//	// check software triggers
	//	if (word == "load" || word == "move")
	//	{
	//		return rgbs["theme green"];
	//	}
	//	// check variable
	//	else if (word == "{" || word == "}")
	//	{
	//		return rgbs["theme aqua"];
	//	}
	//	if (word.size() > 8)
	//	{
	//		if (word.substr(word.size() - 8, 8) == ".moogScript")
	//		{
	//			return rgbs["theme yellow"];
	//		}
	//	}
	//}
	if (editType == "AWG") {
		if (word == "setsteps" || word == "amplin" || word == "freqlin")
		{
			return rgbs["theme purple"];
		}
		// check logic
		if (word == "customsequence")
		{
			return rgbs["theme blue"];
		}
		// check software triggers
		if (word == "reset" || word == "trigger" || word == "program")
		{
			return rgbs["theme green"];
		}
		// check variable
		else if (word == "{" || word == "}")
		{
			return rgbs["theme aqua"];
		}
		if (word.size() > 8)
		{
			if (word.substr(word.size() - 8, 8) == ".awgScript")
			{
				return rgbs["theme yellow"];
			}
		}
	}
	if (editType == "Gmoog") {
		if (word == "set" || word == "setmove")
		{
			return rgbs["theme purple"];
		}
		// check variable
		else if (word == "{" || word == "}")
		{
			return rgbs["theme aqua"];
		}
		else if (word == "dac0" || word == "dac1" || word == "dac2" || word == "dac3")
		{
			return rgbs["theme green"];
		}
		else if (word == "hardreset") {
			return rgbs["theme orange"];
		}
		if (word.size() > 8)
		{
			if (word.substr(word.size() - 8, 8) == ".gmoogScript")
			{
				return rgbs["theme yellow"];
			}
		}
	}
	// Check DDS-specific commands
	if (editType == "DDS") {
		if (word == "set")
		{
			return rgbs["theme purple"];
		}
		else if (word == "snapshot" || word == "reps" || word == "reset" || word == "end") {
			return rgbs["theme green"];
		}
		else if (word == "freqs"|| word == "amps") {
			return rgbs["theme blue"];
		}
		// check variable
		else if (word == "{" || word == "}")
		{
			return rgbs["theme aqua"];
		}
		if (word.size() > 8)
		{
			if (word.substr(word.size() - 8, 8) == ".ddsScript")
			{
				return rgbs["theme yellow"];
			}
		}
	}
	// Check Agilent-specific commands
	else if (editType == "Agilent") {
		std::transform(word.begin(), word.end(), word.begin(), ::tolower);
		if (word == "ramp" || word == "hold" || word == "pulse" )
		{
			return rgbs["theme purple"];
		}
		else if ( word == "once" || word == "oncewaittrig" || word == "lin" || word == "tanh" 
				  || word == "repeatuntiltrig" || word == "sech" || word == "gaussian" || word == "lorentzian" )
		{
			return rgbs["theme yellow"];
		}
	}
	else if (editType == "Master")
	{
		if (word == "on:" || word == "off:" || word == "pulseon:" || word == "pulseoff:")
		{
			return rgbs["theme purple"];
		}
		if (word == "dac:" || word == "dacarange:" || word == "daclinspace:")
		{
			return rgbs["theme yellow"];
		}
		else if (word == "call")
		{
			colorLine = true;
			return rgbs["theme blue"];
		}
		else if (word == "def")
		{
			colorLine = true;
			return rgbs["theme blue"];
		}
		else if ( word == "rsg:" || word == "repeat:" || word == "end" || word == "callcppcode" 
				  || word == "loadskipentrypoint!")
		{
			return rgbs["theme green"];
		}
		else if (word == "t")
		{
			return rgbs["theme foreground"];
		}
		for (UINT rowInc = 0; rowInc < ttlNames.size(); rowInc++)
		{
			std::string row;
			switch (rowInc)
			{
				case 0: row = "a"; break;
				case 1: row = "b"; break;
				case 2: row = "c"; break;
				case 3: row = "d"; break;
			}
			for (UINT numberInc = 0; numberInc < ttlNames[rowInc].size(); numberInc++)
			{
				
				if (word == ttlNames[rowInc][numberInc])
				{
					return rgbs["theme green"];
				}
				if (word == row + str(numberInc))
				{
					return rgbs["theme green"];
				}
			}
		}
		for (UINT dacInc = 0; dacInc < dacNames.size(); dacInc++)
		{
			if (word == dacNames[dacInc])
			{
				return rgbs["theme green"];
			}
			if (word == "dac" + str(dacInc))
			{
				return rgbs["theme green"];
			}
		}
	}

	if ( word == "+" || word == "=" || word == "(" || word == ")" || word == "*" || word == "-" || word == "/" )
	{
		// all scripts now support math expressions.
		return rgbs["theme aqua"];
	}

	for (UINT varInc = 0; varInc < variables.size(); varInc++)
	{
		if (word == variables[varInc].name)
		{
			return rgbs["theme blue"];
		}
	}
	// check delimiter
	if (word == "#")
	{
		return rgbs["Light Grey"];
	}
	//// see if it's a double.
	//try {
	//	// not sure why not just using std::stod.
	//	boost::lexical_cast<double>(word);
	//	return rgbs["theme foreground"];
	//}
	//catch (boost::bad_lexical_cast &) {
	//	try {
	//		boost::lexical_cast<int>(word);
	//		return rgbs["theme foreground"];
	//	}
	//	catch (boost::bad_lexical_cast &)
	//	{
	//		return rgbs["theme red"];
	//	}
	//}	
	// see if it's an int or double.
	try {
		std::stoul(word, nullptr, 0);
		return rgbs["theme foreground"];
	}
	catch (...) {
		try {
			std::stod(word, nullptr);
			return rgbs["theme foreground"];
		}
		catch (...)
		{
			return rgbs["theme red"];
		}
	}
}


void Script::updateSavedStatus(bool scriptIsSaved)
{
	isSaved = scriptIsSaved;
	if (scriptIsSaved)
	{
		savedIndicator.SetCheck( true );
	}
	else
	{
		savedIndicator.SetCheck( false );
	}
}


bool Script::coloringIsNeeded()
{
	return !syntaxColoringIsCurrent;
}


void Script::handleTimerCall(std::vector<variableType> vars,
							 rgbMap rgbs, std::array<std::array<std::string, 16>, 4> ttlNames,
							 std::array<std::string, 24> dacNames )
{
	if (!edit)
	{
		return;
	}
	if (!syntaxColoringIsCurrent)
	{
		// preserve saved state
		bool tempSaved = false;
		if (isSaved == true)
		{
			tempSaved = true;
		}
		DWORD x1 = 1, x2 = 1;
		int initScrollPos, finScrollPos;
		CHARRANGE charRange;
		edit.GetSel(charRange);
		initScrollPos = edit.GetScrollPos(SB_VERT);
		// color syntax
		colorScriptSection(editChangeBegin, editChangeEnd, vars, rgbs, ttlNames, dacNames);
		editChangeEnd = 0;
		editChangeBegin = ULONG_MAX;
		syntaxColoringIsCurrent = true;
		edit.SetSel(charRange);
		finScrollPos = edit.GetScrollPos(SB_VERT);
		edit.LineScroll(-(finScrollPos - initScrollPos));
		updateSavedStatus(tempSaved);
	}
}

void Script::handleEditChange()
{
	// make sure the edit has been initialized.
	if (!edit)
	{
		return;
	}
	CHARRANGE charRange;
	edit.GetSel(charRange);
	if (charRange.cpMin < editChangeBegin)
	{
		editChangeBegin = charRange.cpMin;
	}
	if (charRange.cpMax > editChangeEnd)
	{
		editChangeEnd = charRange.cpMax;
	}
	syntaxColoringIsCurrent = false;
	updateSavedStatus(false);
}


void Script::colorEntireScript(std::vector<variableType> vars, rgbMap rgbs, std::array<std::array<std::string, 16>, 4> ttlNames,
							   std::array<std::string, 24> dacNames )
{
	colorScriptSection(0, ULONG_MAX, vars, rgbs, ttlNames, dacNames);
}


void Script::colorScriptSection( DWORD beginingOfChange, DWORD endOfChange, std::vector<variableType> vars, 
								 rgbMap rgbs, std::array<std::array<std::string, 16>, 4> ttlNames, 
								 std::array<std::string, 24> dacNames)
{
	if (!edit)
	{
		return;
	}
	//parent->SetRedraw( false );
	edit.SetRedraw( false );
	bool tempSaveStatus = false;
	if ( isSaved )
	{
		tempSaveStatus = true;
	}
	long long beginingSigned = beginingOfChange;
	long long endSigned = endOfChange;
	CString text;
	edit.GetWindowTextA(text);
	std::string script(text);
	std::vector<std::string> predefinedScripts;
	COLORREF coloring;
	std::string word;
	std::stringstream fileTextStream(script);
	std::string line;
	std::string currentTextToAdd;
	std::string tempColor;
	COLORREF syntaxColor;
	CHARFORMAT syntaxFormat;
	memset(&syntaxFormat, 0, sizeof(CHARFORMAT));
	syntaxFormat.cbSize = sizeof(CHARFORMAT);
	syntaxFormat.dwMask = CFM_COLOR;
	//int relevantID = GetDlgCtrlID(edit.hwnd);
	DWORD start = 0, end = 0;
	std::size_t prev, pos;

	while (std::getline(fileTextStream, line))
	{
		DWORD lineStartCoordingate = start;
		int endTest = end + line.size();
		if (endTest < beginingSigned - 5 || start > endSigned)
		{
			// then skip to next line.
			end = endTest;
			start = end;
			continue;
		}
		prev = 0;
		coloring = 0;
		bool colorLine = false;
		while ((pos = line.find_first_of(" \t\r\n+=()-*/", prev)) != std::string::npos)
		{
			if (pos == prev)
			{
				// then there was one of " \t\r\n+=" at the begging of the next string. check it.
				pos++;
			}
			end = lineStartCoordingate + pos;
			word = line.substr(prev, pos - prev);
			if (word == " " || word == "\t" || word == "\r" || word == "\n")
			{
				start = end;
				prev = pos;
				continue;
			}
			// if comment is found, the rest of the line is green.
			if (!colorLine)
			{
				// get all the variables
				// get 
				syntaxColor = getSyntaxColor(word, deviceType, vars, rgbs, colorLine, ttlNames, dacNames);
				if (syntaxColor != coloring)
				{
					coloring = syntaxColor;
					syntaxFormat.crTextColor = coloring;
					CHARRANGE charRange;
					charRange.cpMin = start;
					charRange.cpMax = end;
					edit.SetSel(charRange);
					edit.SetSelectionCharFormat(syntaxFormat);
					start = end;
				}
			}
			CHARRANGE charRange;
			charRange.cpMin = start;
			charRange.cpMax = end;
			edit.SetSel(charRange);
			edit.SetSelectionCharFormat(syntaxFormat);
			start = end;
			prev = pos;
		}
		// handle the end. above doesn't catch the end. There's probably a better way to do this.
		if (prev < std::string::npos)
		{
			bool colorLine = false;
			word = line.substr(prev, std::string::npos);
			end = lineStartCoordingate + line.length();
			// get all the variables together
			syntaxColor = getSyntaxColor( word, deviceType, vars, rgbs, colorLine, ttlNames, dacNames);
			if (!colorLine)
			{
				coloring = syntaxColor;
				syntaxFormat.crTextColor = coloring;
				CHARRANGE charRange;
				charRange.cpMin = start;
				charRange.cpMax = end;
				edit.SetSel(charRange);
				edit.SetSelectionCharFormat(syntaxFormat);
				start = end;
			}
			CHARRANGE charRange;
			charRange.cpMin = start;
			charRange.cpMax = end;
			edit.SetSel(charRange);
			edit.SetSelectionCharFormat(syntaxFormat);
			start = end;
		}
	}
	edit.SetRedraw( true );
	edit.RedrawWindow();
	updateSavedStatus( tempSaveStatus );
}


void Script::handleToolTip( NMHDR * pNMHDR, LRESULT * pResult )
{
	TOOLTIPTEXT *pTTT = (TOOLTIPTEXT *)pNMHDR;
	UINT nID = pNMHDR->idFrom;
	if (pTTT->uFlags & TTF_IDISHWND)
	{
		nID = ::GetDlgCtrlID( (HWND)nID );
	}

	if (nID == help.GetDlgCtrlID())
	{
		// it's this window.
		// note that I don't think this is the hwnd of the help box, but rather the tooltip itself.
		::SendMessageA( pNMHDR->hwndFrom, TTM_SETMAXTIPWIDTH, 0, 2500 );
		if (deviceType == "Master")
		{
			pTTT->lpszText = (LPSTR)MASTER_HELP;
		}
		else if( deviceType == "Horizontal NIAWG" || deviceType == "Vertical NIAWG" )
		{
			pTTT->lpszText = (LPSTR)SCRIPT_INFO_TEXT;
		}
		else if (deviceType == "Agilent")
		{
			pTTT->lpszText = (LPSTR)AGILENT_INFO_TEXT;
		}
		//else if (deviceType == "Moog")
		//{
		//	pTTT->lpszText = (LPSTR)MOOG_INFO_TEXT;
		//}
		else if (deviceType == "AWG")
		{
			pTTT->lpszText = (LPSTR)AWG_INFO_TEXT;
		}
		else if (deviceType == "Gmoog")
		{
			pTTT->lpszText = (LPSTR)GIGAMOOG_INFO_TEXT;
		}
		else if (deviceType == "DDS")
		{
			pTTT->lpszText = (LPSTR)DDS_INFO_TEXT;
		}
		else
		{
			pTTT->lpszText = "No Help available";
		}
		//pTTT->lpszText = ;
		//_tcsncpy_s( , _T(  ), _TRUNCATE );
		*pResult = 0;
		thrower( "Worked." );
	}
	// else it's another window, just return and let them try.
}


INT_PTR Script::colorControl(LPARAM lParam, WPARAM wParam)
{
	int controlID = GetDlgCtrlID((HWND)lParam);
	HDC hdcStatic = (HDC)wParam;
	if (controlID == edit.GetDlgCtrlID())
	{
		SetTextColor(hdcStatic, RGB(255, 255, 255));
		SetBkColor(hdcStatic, RGB(50, 45, 45));
	}
	else if (controlID == title.GetDlgCtrlID())
	{
		SetTextColor(hdcStatic, RGB(255, 255, 255));
		SetBkMode(hdcStatic, TRANSPARENT);
		SetBkColor(hdcStatic, RGB(75, 0, 0));
	}
	else if (controlID == savedIndicator.GetDlgCtrlID())
	{
		SetTextColor(hdcStatic, RGB(255, 255, 255));
		SetBkMode(hdcStatic, TRANSPARENT);
		SetBkColor(hdcStatic, RGB(50, 45, 45));
	}
	else if (controlID == fileNameText.GetDlgCtrlID())
	{
		SetTextColor(hdcStatic, RGB(255, 255, 255));
		SetBkMode(hdcStatic, TRANSPARENT);
		SetBkColor(hdcStatic, RGB(25, 0, 0));
	}
	else
	{
		return false;
	}
	return true;
}



void Script::changeView(std::string viewName, bool isFunction, std::string categoryPath)
{
	if (viewName == "Parent Script")
	{
		// load parent
		loadFile(categoryPath + scriptName + extension);
	}
	else if (isFunction)
	{
		loadFile(FUNCTIONS_FOLDER_LOCATION + viewName + "." + FUNCTION_EXTENSION);
	}
	else
	{
		// load child
		loadFile(categoryPath + viewName);
	}

	// colorEntireScript(variables, rgbs, ttlNames, dacNames);
	// the view is fresh from a file, so it's saved.
	updateSavedStatus(true);
}

//
void Script::saveScript(std::string categoryPath, RunInfo info)
{
	if (categoryPath == "")
	{
		thrower("ERROR: Please select a category before trying to save a script!\r\n");
	}
	if (isSaved && scriptName != "")
	{
		// shouldn't need to do anything
		return;
	}
	int sel = availableFunctionsCombo.GetCurSel();
	CString text;
	if (sel != -1)
	{
		availableFunctionsCombo.GetLBText( sel, text );
	}
	else
	{
		text = "";
	}
	if (text != "Parent Script")
	{
		int answer = promptBox("WARNING: The current view is not the parent view. This will save the "
								"current text in the edit under the name \"" + scriptName 
								+ "\". Are you sure you wish to proceed?", MB_OKCANCEL );
		if (answer == IDCANCEL)
		{
			return;
		}
	}
	if (scriptName == "")
	{
		std::string newName;
		TextPromptDialog dialog(&newName, "Please enter new name for the " + deviceType + " script " + scriptName + ".");
		dialog.DoModal();
		if (newName == "")
		{
			// canceled
			return;
		}
		std::string path = categoryPath + newName + extension;
		saveScriptAs(path, info);
	}
	if (info.running)
	{
		for (UINT scriptInc = 0; scriptInc < info.currentlyRunningScripts.size(); scriptInc++)
		{
			if (scriptName == info.currentlyRunningScripts[scriptInc])
			{
				thrower("ERROR: System is currently running. You can't save over any files in use by the system while it runs, which includes the "
					"horizontal and vertical AOM scripts and the intensity script.");
			}
		}
	}
	edit.GetWindowTextA(text);
	std::fstream saveFile(categoryPath + scriptName + extension, std::fstream::out);
	if (!saveFile.is_open())
	{
		thrower("ERROR: Failed to open script file: " + categoryPath + scriptName 
				+ extension);
	}
	saveFile << text;
	saveFile.close();
	scriptFullAddress = categoryPath + scriptName + extension;
	scriptPath = categoryPath;
	updateSavedStatus(true);
}


//
void Script::saveScriptAs(std::string location, RunInfo info)
{
	if (location == "")
	{
		return;
	}
	if (info.running)
	{
		for (UINT scriptInc = 0; scriptInc < info.currentlyRunningScripts.size(); scriptInc++)
		{
			if (scriptName == info.currentlyRunningScripts[scriptInc])
			{
				thrower("ERROR: System is currently running. You can't save over any files in use by the system while it runs, which includes the "
					"horizontal and vertical AOM scripts and the intensity script.");
			}
		}
	}
	CString text;
	edit.GetWindowTextA(text);
	std::fstream saveFile(location, std::fstream::out);
	if (!saveFile.is_open())
	{
		thrower("ERROR: Failed to open script file: " + location);
	}
	saveFile << text;
	char fileChars[_MAX_FNAME];
	char dirChars[_MAX_FNAME];
	char pathChars[_MAX_FNAME];
	int myError = _splitpath_s(cstr(location), dirChars, _MAX_FNAME, pathChars, _MAX_FNAME, fileChars, _MAX_FNAME, NULL, 0);
	scriptName = str(fileChars);
	scriptPath = str(fileChars) + str(pathChars);
	saveFile.close();
	scriptFullAddress = location;
	updateScriptNameText(location);
	updateSavedStatus(true);
}


//
void Script::checkSave(std::string categoryPath, RunInfo info)
{
	if (isSaved)
	{
		// don't need to do anything
		return;
	}
	// test first non-commented word of text to see if this looks like a function or not.
	CString text;
	edit.GetWindowTextA(text);
	ScriptStream tempStream;
	tempStream << text;
	std::string word;
	tempStream >> word;
	if (word == "def")
	{
		int answer = promptBox("Current " + deviceType + " function file is unsaved. Save it?", MB_YESNOCANCEL );
		if (answer == IDCANCEL)
		{
			thrower("Cancel!");
		}
		else if (answer == IDNO)
		{
			return;
		}
		else if (answer == IDYES)
		{
			saveAsFunction();
			return;
		}
	}
	// else it's a normal script file.
	if (scriptName == "")
	{
		int answer = promptBox("Current " + deviceType + " script file is unsaved and unnamed. Save it with a with new name?", MB_YESNOCANCEL);
		if (answer == IDCANCEL)
		{
			thrower("Cancel!");
		}
		else if (answer == IDNO)
		{
			return;
		}
		else if (answer == IDYES)
		{
			std::string newName;
			TextPromptDialog dialog(&newName, "Please enter new name for the " + deviceType + " script " + scriptName + ".");
			dialog.DoModal();
			std::string path = categoryPath + newName + extension;
			saveScriptAs(path, info);
			return;
		}
	}
	else
	{
		int answer = promptBox("The " + deviceType + " script file is unsaved. Save it as " + scriptName 
								+ extension + "?", MB_YESNOCANCEL );
		if (answer == IDCANCEL)
		{
			thrower("Cancel!");
		}
		else if (answer == IDNO)
		{
		}
		else if (answer == IDYES)
		{
			saveScript(categoryPath, info);
		}
	}
}


//
void Script::renameScript(std::string categoryPath)
{
	if (scriptName == "")
	{
		// ??? don't know why I need this here.
		return;
	}
	std::string newName;
	TextPromptDialog dialog(&newName, "Please enter new name for the " + deviceType + " script " + scriptName + ".");
	dialog.DoModal();
	if (newName == "")
	{
		// canceled
		return;
	}
	int result = MoveFile(cstr(categoryPath + scriptName + extension),
						  cstr(categoryPath + newName + extension));

	if (result == 0)
	{
		thrower("ERROR: Failed to move file.");
	}
	scriptFullAddress = categoryPath + scriptName + extension;
	scriptPath = categoryPath;
}


//
void Script::deleteScript(std::string categoryPath)
{
	if (scriptName == "")
	{
		return;
	}
	// check to make sure:
	int answer = promptBox("Are you sure you want to delete the script file " + scriptName + "?", MB_YESNO);
	if (answer == IDNO)
	{
		return;
	}
	int result = DeleteFile(cstr(categoryPath + scriptName + extension));
	if (result == 0)
	{
		thrower("ERROR: Deleting script file failed!");
	}
	else
	{
		scriptName = "";
		scriptPath = "";
		scriptFullAddress = "";
	}
}


// the differences between this and new script are that this opens the default function instead of the default script
// and that this does not reset the script category, etc. 
void Script::newFunction()
{
	std::string tempName;
	tempName = DEFAULT_SCRIPT_FOLDER_PATH;
	if (deviceType == "Master")
	{
		tempName += "DEFAULT_FUNCTION.func";
	}
	else
	{
		thrower("ERROR: tried to load new function with non-master script???");
	}
	loadFile(tempName);
}


//
void Script::newScript()
{
	std::string tempName;
	tempName = DEFAULT_SCRIPT_FOLDER_PATH;
/*	if (deviceType == "Horizontal NIAWG")
	{
		tempName += "DEFAULT_HORIZONTAL_SCRIPT.nScript";
	}
	else if (deviceType == "Vertical NIAWG")
	{
		tempName += "DEFAULT_VERTICAL_SCRIPT.nScript";
	}
	else if (deviceType == "Agilent")
	{
		tempName += "DEFAULT_INTENSITY_SCRIPT.aScript";
	}
	else*/ 
	if (deviceType == "Master")
	{
		tempName += "DEFAULT_MASTER_SCRIPT.mScript";
	}	
	//else if (deviceType == "Moog")
	//{
	//	tempName += "DEFAULT_MOOG_SCRIPT.moogScript";
	//}
	else if (deviceType == "AWG")
	{
		tempName += "DEFAULT_AWG_SCRIPT.awgScript";
	}
	else if (deviceType == "Gmoog")
	{
		tempName += "DEFAULT_GMOOG_SCRIPT.gmoogScript";
	}
	else if (deviceType == "DDS")
	{
		tempName += "DEFAULT_DDS_SCRIPT.ddsScript";
	}
	reset();
	loadFile(tempName);
}

//
void Script::openParentScript(std::string parentScriptFileAndPath, std::string categoryPath, RunInfo info)
{
	if (parentScriptFileAndPath == "" || parentScriptFileAndPath == "NONE")
	{
		return;
	}
	char fileChars[_MAX_FNAME];
	char extChars[_MAX_EXT];
	char dirChars[_MAX_FNAME];
	char pathChars[_MAX_FNAME];
	int myError = _splitpath_s(cstr(parentScriptFileAndPath), dirChars, _MAX_FNAME, pathChars, _MAX_FNAME, fileChars, 
								_MAX_FNAME, extChars, _MAX_EXT);
	std::string extStr(extChars);
	//if (deviceType == "Horizontal NIAWG" || deviceType == "Vertical NIAWG")
	//{
	//	if (extStr != str(".") + NIAWG_SCRIPT_EXTENSION)
	//	{
	//		thrower("ERROR: Attempted to open non-NIAWG script inside NIAWG script control.");
	//	}
	//}
	//else if (deviceType == "Agilent")
	//{
	//	if (extStr != str( "." ) + AGILENT_SCRIPT_EXTENSION)
	//	{
	//		thrower("ERROR: Attempted to open non-agilent script from agilent script control.");
	//	}
	//}
	if (deviceType == "Master")
	{
		if (extStr != str( "." ) + MASTER_SCRIPT_EXTENSION)
		{
			thrower("ERROR: Attempted to open non-master script from master script control!");
		}
	}
	//else if (deviceType == "Moog")
	//{
	//	if (extStr != str(".") + MOOG_SCRIPT_EXTENSION)
	//	{
	//		thrower("ERROR: Attempted to open non-moog script from moog script control!");
	//	}
	//}
	else if (deviceType == "AWG")
	{
		if (extStr != str(".") + AWG_SCRIPT_EXTENSION)
		{
			thrower("ERROR: Attempted to open non-awg script from awg script control!");
		}
	}
	else if (deviceType == "Gmoog")
	{
		if (extStr != str(".") + GIGAMOOG_SCRIPT_EXTENSION)
		{
			thrower("ERROR: Attempted to open non-gmoog script from gmoog script control!");
		}
	}
	else if (deviceType == "DDS")
	{
		if (extStr != str(".") + DDS_SCRIPT_EXTENSION)
		{
			thrower("ERROR: Attempted to open non-dds script from dds script control!");
		}
	}
	else
	{
		thrower("ERROR: Unrecognized device type inside script control!");
	}
	loadFile( parentScriptFileAndPath );
	scriptName = str(fileChars);
	scriptFullAddress = parentScriptFileAndPath;
	updateSavedStatus(true);
	// Check location of vertical script.
	int sPos = parentScriptFileAndPath.find_last_of('\\');
	std::string scriptLocation = parentScriptFileAndPath.substr(0, sPos);
	if (scriptLocation + "\\" != categoryPath && categoryPath != "")
	{
		int answer = promptBox("The requested " + deviceType + " script: " + parentScriptFileAndPath + " is not "
								"currently located in the current configuration folder. This is recommended so that "
								"scripts related to a particular configuration are reserved to that configuration "
								"folder. Copy script to current configuration folder?", MB_YESNO);
		if (answer == IDYES)
		{
			std::string scriptName = parentScriptFileAndPath.substr(sPos+1, parentScriptFileAndPath.size());
			std::string path = categoryPath + scriptName;
			saveScriptAs(path, info);
		}
	}
	updateScriptNameText( categoryPath );
	availableFunctionsCombo.SelectString(0, "Parent Script");
}


/*
]---	This function only puts the given file on the edit for this class, it doesn't change current settings parameters. It's used bare when just changing the
]-		view of the edit, while it's used with some surrounding changes for loading a new parent.
 */
void Script::loadFile(std::string pathToFile)
{
	std::fstream openFile;
	openFile.open(pathToFile, std::ios::in);
	if (!openFile.is_open())
	{
		reset();
		thrower("ERROR: Failed to open script file: " + pathToFile + ".");
	}
	std::string tempLine;
	std::string fileText;
	while (std::getline(openFile, tempLine))
	{
		cleanString(tempLine);
		fileText += tempLine;
		// Append the line to the edit control here.
	}
	// put the default into the new control.
	edit.SetWindowTextA(cstr(fileText));
	openFile.close();
}


void Script::reset()
{
	availableFunctionsCombo.SelectString(0,"Parent Script");
	scriptName = "";
	scriptPath = "";
	scriptFullAddress = "";
	updateSavedStatus(false);
	fileNameText.SetWindowTextA("");
	edit.SetWindowTextA("");
}


bool Script::savedStatus()
{
	return isSaved;
}

std::string Script::getScriptPathAndName()
{
	return scriptFullAddress;
}

std::string Script::getScriptName()
{
	return scriptName;
}

void Script::considerCurrentLocation(std::string categoryPath, RunInfo info)
{
	if (scriptFullAddress.size() > 0)
	{
		// Check location of vertical script.
		int sPos = scriptFullAddress.find_last_of('\\');
		std::string scriptLocation = scriptFullAddress.substr(0, sPos);
		if (scriptLocation + "\\" != categoryPath)
		{
			int answer = promptBox( "The requested " + deviceType + " script location: \"" + scriptLocation + "\" "
									"is not currently located in the current configuration folder. This is recommended"
									" so that scripts related to a particular configuration are reserved to that "
									"configuration folder. Copy script to current configuration folder?", MB_YESNO );
			if (answer == IDYES)
			{
				std::string scriptName = scriptFullAddress.substr(sPos, scriptFullAddress.size());
				scriptFullAddress = categoryPath + scriptName;
				scriptPath = categoryPath;
				saveScriptAs(scriptFullAddress, info);
			}
		}
		// else nothing
	}
}

std::string Script::getExtension()
{
	return extension;
}

void Script::updateScriptNameText(std::string categoryPath)
{
	// there are some \\ on the end of the path by default.
	categoryPath = categoryPath.substr(0, categoryPath.size() - 1);
	int sPos = categoryPath.find_last_of('\\');
	if (sPos != -1)
	{
		std::string category = categoryPath.substr(sPos + 1, categoryPath.size());
		std::string text = category + "->" + scriptName;
		fileNameText.SetWindowTextA(cstr(text));
	}
	else
	{
		std::string text = "??? -> ";
		if (scriptName == "")
		{
			text += "???";
		}
		else
		{
			text += scriptName;
		}
		fileNameText.SetWindowTextA(cstr(text));
	}
}



void Script::setScriptText(std::string text)
{
	edit.SetWindowText( cstr( text ) );
}

INT_PTR Script::handleColorMessage(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam, brushMap brushes)
{
	DWORD controlID = GetDlgCtrlID((HWND)lParam);
	HDC hdcStatic = (HDC)wParam;
	if (controlID == edit.GetDlgCtrlID())
	{
		SetTextColor(hdcStatic, RGB(255, 255, 255));
		SetBkColor(hdcStatic, RGB(15, 15, 15));
		return (LRESULT)brushes["theme BG1"];
	}
	else if (controlID == title.GetDlgCtrlID() || controlID == savedIndicator.GetDlgCtrlID() 
			 || controlID == fileNameText.GetDlgCtrlID() || controlID == availableFunctionsCombo.GetDlgCtrlID())
	{
		SetTextColor(hdcStatic, RGB(218, 165, 32));
		SetBkColor(hdcStatic, RGB(25, 25, 25));
		return (LRESULT)brushes["theme BG0"];
	}
	else
	{
		return NULL;
	}
}


void Script::saveAsFunction()
{
	// check to make sure that the current script is defined like a function
	CString text;
	edit.GetWindowTextA(text);
	ScriptStream stream;
	stream << text.GetBuffer();
	std::string word;
	stream >> word;
	if (word != "def")
	{
		thrower("ERROR: Function declarations must begin with \"def\".");
	}
	std::string line;
	line = stream.getline( '\r' );
	int pos = line.find_first_of("(");
	if (pos == std::string::npos)
	{
		thrower("No \"(\" found in function name. If there are no arguments, use empty parenthesis \"()\"");
	}
	int initNamePos = line.find_first_not_of(" \t");
	std::string functionName = line.substr(initNamePos, line.find_first_of("("));
	if (functionName.find_first_of(" ") != std::string::npos)
	{
		thrower("ERROR: Function name included a space! Name was" + functionName);
	}
	std::string path = FUNCTIONS_FOLDER_LOCATION + functionName + "." + FUNCTION_EXTENSION;
	FILE *file;
	fopen_s( &file, cstr(path), "r" );
	if ( !file )
	{
		//
	}
	else
	{
		fclose( file );
		int answer = promptBox("The function \"" + functionName + "\" already exists! Overwrite it?", MB_YESNO );
		if ( answer == IDNO )
		{
			return;
		}
	}
	std::fstream functionFile(path, std::ios::out);
	if (!functionFile.is_open())
	{
		thrower("ERROR: the function file failed to open!");
	}
	functionFile << text.GetBuffer();
	functionFile.close();
	// refresh this.
	loadFunctions();
	availableFunctionsCombo.SelectString( 0, cstr(functionName) );
	// test if script exists in nearby folder.
	updateSavedStatus(true);
}


void Script::loadFunctions()
{
	ProfileSystem::reloadCombo( availableFunctionsCombo.GetSafeHwnd( ), functionLocation, str("*.") 
								+ FUNCTION_EXTENSION, "__NONE__" );
	availableFunctionsCombo.InsertString( 0, "Parent Script" );
}
