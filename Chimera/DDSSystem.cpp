#include "stdafx.h"
#include "DDSSystem.h"
#include "AuxiliaryWindow.h"
// for other ni stuff
#include "nidaqmx2.h"


DDSSystem::DDSSystem() : ddsResolution({ 500 / pow(2,32), 100 / pow(2,10) })
{
	try
	{
		//
		int tcp_connect;
		try
		{
			tcp_connect = zynq_tcp.connectTCP(ZYNQ_ADDRESS);
		}
		catch (Error& err)
		{
			tcp_connect = 1;
			errBox(err.what());
		}

		if (tcp_connect == 0)
		{
			zynq_tcp.writeCommand("disableSeq");
			zynq_tcp.writeCommand("lockPLL");
			zynq_tcp.disconnect();
		}
		else
		{
			throw("connection to zynq failed. can't write DDS data\n");
		}
	}
	// I catch here because it's the constructor, and catching elsewhere is weird.
	catch (Error& exception)
	{
		errBox(exception.what());
	}
}


std::array<std::array<double, 2>, 12> DDSSystem::getDDSStatus()
{
	return ddsValues;
}


void DDSSystem::handleOpenConfig(std::ifstream& openFile, int versionMajor, int versionMinor)
{
	ProfileSystem::checkDelimiterLine(openFile, "DDSS");
	prepareForce();
	std::vector<double> values(getNumberOfDDSs());
	UINT ddsInc = 0;
	for (auto& dds : values)
	{
		std::string ddsFreqString;
		std::string ddsAmpString;
		openFile >> ddsFreqString;
		openFile >> ddsAmpString;
		try
		{
			std::array<double, 2> ddsValue = { std::stod(ddsFreqString), std::stod(ddsAmpString) };
			if (ddsValue[0] > 10) {
				prepareDDSForceChange(ddsInc, ddsValue);
			}
		}
		catch (std::invalid_argument&)
		{
			thrower("ERROR: failed to convert dds values to freq. string is " + ddsFreqString);
		}
		ddsInc++;
	}
	ProfileSystem::checkDelimiterLine(openFile, "END_DDSS");
}


void DDSSystem::handleNewConfig(std::ofstream& newFile)
{
	newFile << "DDSS\n";
	for (UINT ddsInc = 0; ddsInc < getNumberOfDDSs(); ddsInc++)
	{
		newFile << 80 << " " << 0 << " ";
	}
	newFile << "\nEND_DDSS\n";
}


void DDSSystem::handleSaveConfig(std::ofstream& saveFile)
{
	saveFile << "DDSS\n";
	for (UINT ddsInc = 0; ddsInc < getNumberOfDDSs(); ddsInc++)
	{
		saveFile << getDDSValue(ddsInc)[0] << " " << getDDSValue(ddsInc)[1] << " ";
	}
	saveFile << "\nEND_DDSS\n";
}


void DDSSystem::abort()
{
	// TODO...?
}


std::string DDSSystem::getDDSSequenceMessage(UINT variation)
{
	std::string message;
	for (auto snap : ddsSnapshots[variation])
	{
		std::string time = str(snap.time, 12, true);
		message += time + ":\r\n";
		int ddsCount = 0;
		for (auto val : snap.ddsValues)
		{
			std::string freq = str(val[0], true);
			std::string amp = str(val[1], true);
			message += freq + ", " + amp + ", ";
			ddsCount++;
			if (ddsCount % 12 == 0)
			{
				message += "\r\n";
			}
		}
		message += "\r\n---\r\n";
	}
	return message;
}

void DDSSystem::handleDDSScriptCommand(DDSCommandForm command, std::string name, std::vector<UINT>& ddsShadeLocations,
	std::vector<variableType>& vars, DioSystem* ttls)
{
	if (command.commandName != "ddsfreq:" && command.commandName != "ddsamp:" && command.commandName != "ddsamplinspace:" && command.commandName != "ddsfreqlinspace:"
		&& command.commandName != "ddsrampamp:" && command.commandName != "ddsrampfreq:")
	{
		thrower("ERROR: dds commandName not recognized!");
	}
	if (!isValidDDSName(name))
	{
		thrower("ERROR: the name " + name + " is not the name of a dds!");
	}
	// convert name to corresponding dac line.
	command.line = getDDSIdentifier(name);
	if (command.line == -1)
	{
		thrower("ERROR: the name " + name + " is not the name of a dds!");
	}
	ddsShadeLocations.push_back(command.line);
	setDDSCommandForm(command);
}

void DDSSystem::handleEditChange(UINT ddsNumber)
{
	//if (ddsNumber >= breakoutBoardAmpEdits.size())
	//{
	//	thrower("ERROR: attempted to handle dds amp edit change, but the dds number reported doesn't exist!");
	//}
	//CString text;
	//breakoutBoardAmpEdits[ddsNumber].GetWindowTextA(text);
	//bool matches = false;
	//std::string textStr(text);
	//try
	//{
	//	if (roundToDDSPrecision)
	//	{
	//		double roundNum = roundToDDSResolution(ddsValues[ddsNumber][1]);
	//		if (fabs(roundToDacResolution(dacValues[dacNumber]) - std::stod(textStr)) < 1e-8)
	//		{
	//			matches = true;
	//		}
	//	}
	//	else
	//	{
	//		if (fabs(dacValues[dacNumber] - std::stod(str(text))) < 1e-8)
	//		{
	//			matches = true;
	//		}
	//	}
	//}
	//catch (std::invalid_argument&){ /* failed to convert to double. Effectively, doesn't match. */ }
	//if ( matches )
	//{
	//	// mark this to change color.
	//	breakoutBoardEdits[dacNumber].colorState = 0;
	//	breakoutBoardEdits[dacNumber].RedrawWindow();
	//}
	//else
	//{
	//	breakoutBoardEdits[dacNumber].colorState = 1;
	//	breakoutBoardEdits[dacNumber].RedrawWindow();
	//}
}


bool DDSSystem::isValidDDSName(std::string name)
{
	for (UINT ddsInc = 0; ddsInc < getNumberOfDDSs(); ddsInc++)
	{
		if (name == "dds" + str(ddsInc))
		{
			return true;
		}
		else if (getDDSIdentifier(name) != -1)
		{
			return true;
		}
	}
	return false;
}

void DDSSystem::rearrange(UINT width, UINT height, fontMap fonts)
{
	/*dacTitle.rearrange( width, height, fonts);
	dacSetButton.rearrange( width, height, fonts);
	zeroDacs.rearrange( width, height, fonts);
	for (auto& control : dacLabels)
	{
		control.rearrange( width, height, fonts);
	}
	for (auto& control : breakoutBoardEdits)
	{
		control.rearrange( width, height, fonts);
	}*/
}


void DDSSystem::setDefaultValue(UINT ddsNum, std::array<double, 2> val)
{
	defaultVals[ddsNum] = val;
}


std::array<double, 2> DDSSystem::getDefaultValue(UINT ddsNum)
{
	return defaultVals[ddsNum];
}


// this function returns the end location of the set of controls. This can be used for the location for the next control beneath it.
void DDSSystem::initialize(POINT& pos, cToolTips& toolTips, AuxiliaryWindow* master, int& id)
{
	// title
	ddsTitle.sPos = { pos.x, pos.y, pos.x + 400, pos.y += 25 };
	ddsTitle.Create("DDSS", WS_CHILD | WS_VISIBLE | SS_SUNKEN | SS_CENTER, ddsTitle.sPos, master, id++);
	ddsTitle.fontType = HeadingFont;
	// 
	ddsSetButton.sPos = { pos.x, pos.y, pos.x + 400, pos.y += 25 };
	ddsSetButton.Create("Set New DDS Values", WS_TABSTOP | WS_VISIBLE | WS_CHILD | BS_DEFPUSHBUTTON,
		ddsSetButton.sPos, master, ID_DDS_SET_BUTTON);
	ddsSetButton.setToolTip("Press this button to attempt force all DAC values to the values currently recorded in the"
		" edits below.", toolTips, master);

	pos.y += 15;

	// DDS board labels
	dds0Title.sPos = { pos.x, pos.y, pos.x + 120, pos.y + 25 };
	dds0Title.Create("DDS 0", WS_CHILD | WS_VISIBLE | SS_SUNKEN | SS_CENTER, dds0Title.sPos, master, id++);
	dds0Title.fontType = HeadingFont;
	dds1Title.sPos = { pos.x + 140, pos.y, pos.x + 260, pos.y + 25 };
	dds1Title.Create("DDS 1", WS_CHILD | WS_VISIBLE | SS_SUNKEN | SS_CENTER, dds1Title.sPos, master, id++);
	dds1Title.fontType = HeadingFont;
	dds2Title.sPos = { pos.x + 280, pos.y, pos.x + 400, pos.y += 25 };
	dds2Title.Create("DDS 2", WS_CHILD | WS_VISIBLE | SS_SUNKEN | SS_CENTER, dds2Title.sPos, master, id++);
	dds2Title.fontType = HeadingFont;

	dds0FreqAmp.sPos = { pos.x + 30, pos.y, pos.x + 120, pos.y + 25 };
	dds0FreqAmp.Create("freq / amp", WS_CHILD | WS_VISIBLE | SS_SUNKEN | SS_CENTER, dds0FreqAmp.sPos, master, id++);
	dds0FreqAmp.fontType = HeadingFont;
	dds1FreqAmp.sPos = { pos.x + 170, pos.y, pos.x + 260, pos.y + 25 };
	dds1FreqAmp.Create("freq / amp", WS_CHILD | WS_VISIBLE | SS_SUNKEN | SS_CENTER, dds1FreqAmp.sPos, master, id++);
	dds1FreqAmp.fontType = HeadingFont;
	dds2FreqAmp.sPos = { pos.x + 310, pos.y, pos.x + 400, pos.y += 25 };
	dds2FreqAmp.Create("freq / amp", WS_CHILD | WS_VISIBLE | SS_SUNKEN | SS_CENTER, dds2FreqAmp.sPos, master, id++);
	dds2FreqAmp.fontType = HeadingFont;



	//
	int collumnInc = 0;

	// there's a single label first, hence the +1.
	for (UINT ddsInc = 0; ddsInc < breakoutBoardFreqEdits.size(); ddsInc++)
	{
		if (ddsInc == breakoutBoardFreqEdits.size() / 3 || ddsInc == 2 * breakoutBoardFreqEdits.size() / 3)
		{
			collumnInc++;
			// go to second or third collumn
			pos.y -= 25 * breakoutBoardFreqEdits.size() / 3;
		}

		breakoutBoardFreqEdits[ddsInc].sPos = { pos.x + 30 + collumnInc * 140, pos.y, pos.x + 75 + collumnInc * 140,
												pos.y + 25 };
		breakoutBoardFreqEdits[ddsInc].colorState = 0;
		breakoutBoardFreqEdits[ddsInc].Create(WS_CHILD | WS_VISIBLE | WS_BORDER, breakoutBoardFreqEdits[ddsInc].sPos,
			master, id++);
		breakoutBoardFreqEdits[ddsInc].SetWindowText("0");
		breakoutBoardFreqEdits[ddsInc].setToolTip(ddsNames[ddsInc], toolTips, master);

		breakoutBoardAmpEdits[ddsInc].sPos = { pos.x + 75 + collumnInc * 140, pos.y, pos.x + 120 + collumnInc * 140,
												pos.y += 25 };
		breakoutBoardAmpEdits[ddsInc].colorState = 0;
		breakoutBoardAmpEdits[ddsInc].Create(WS_CHILD | WS_VISIBLE | WS_BORDER, breakoutBoardAmpEdits[ddsInc].sPos,
			master, id++);
		breakoutBoardAmpEdits[ddsInc].SetWindowText("0");
		breakoutBoardAmpEdits[ddsInc].setToolTip(ddsNames[ddsInc], toolTips, master);
	}

	collumnInc = 0;
	pos.y -= 25 * breakoutBoardFreqEdits.size() / 3;

	for (UINT ddsInc = 0; ddsInc < ddsLabels.size(); ddsInc++)
	{
		if (ddsInc == ddsLabels.size() / 3 || ddsInc == 2 * ddsLabels.size() / 3)
		{
			collumnInc++;
			// go to second or third collumn
			pos.y -= 25 * ddsLabels.size() / 3;
		}
		// create label
		ddsLabels[ddsInc].sPos = { pos.x + collumnInc * 140, pos.y, pos.x + 20 + collumnInc * 140, pos.y += 25 };
		ddsLabels[ddsInc].Create(cstr(ddsInc), WS_CHILD | WS_VISIBLE | SS_CENTER,
			ddsLabels[ddsInc].sPos, master, ID_DAC_FIRST_EDIT + ddsInc);
		ddsLabels[ddsInc].setToolTip(ddsNames[ddsInc], toolTips, master);
	}

	pos.y += 25;

	ddsPLLButton.sPos = { pos.x, pos.y, pos.x + 400, pos.y += 25 };
	ddsPLLButton.Create("Lock DDS PLLs", WS_TABSTOP | WS_VISIBLE | WS_CHILD | BS_DEFPUSHBUTTON,
		ddsPLLButton.sPos, master, ID_DDS_PLL_BUTTON);
	ddsPLLButton.setToolTip("Press this button to lock the DDS PLLs", toolTips, master);
}


/*
 * get the text from every edit and prepare a change.
 */
void DDSSystem::handleButtonPress()
{
	//ddsCommandFormList.clear();
	//prepareForce();

	std::array<std::array<double, 2>, 12> vals;
	for (UINT ddsInc = 0; ddsInc < ddsLabels.size(); ddsInc++)
	{
		CString freqText;
		CString ampText;
		breakoutBoardFreqEdits[ddsInc].GetWindowTextA(freqText);
		breakoutBoardAmpEdits[ddsInc].GetWindowTextA(ampText);
		try
		{
			vals[ddsInc][0] = std::stod(str(freqText));
			vals[ddsInc][1] = std::stod(str(ampText));
			std::string freqValStr;
			std::string ampValStr;
			if (roundToDDSPrecision)
			{
				double valRoundFreq;
				double valRoundAmp;
				valRoundFreq = roundToDDSResolutionFreq(vals[ddsInc][0]);
				valRoundAmp = roundToDDSResolutionAmp(vals[ddsInc][1]);
				freqValStr = str(valRoundFreq, 13, true);
				ampValStr = str(valRoundAmp, 13, true);
			}
			else
			{
				freqValStr = str(vals[ddsInc][0]);
				ampValStr = str(vals[ddsInc][1]);
			}
			breakoutBoardFreqEdits[ddsInc].SetWindowTextA(cstr(freqValStr));
			breakoutBoardAmpEdits[ddsInc].SetWindowTextA(cstr(ampValStr));
		}
		catch (std::invalid_argument&)
		{
			thrower("ERROR: value entered in DDS #" + str(ddsInc) + " (" + freqText.GetString() + " ) failed to convert to a double!");
		}
	}
	// wait until after all this to actually do this to make sure things get through okay.
	ddsValues = vals;
	for (UINT ddsInc = 0; ddsInc < ddsLabels.size(); ddsInc++)
	{
		breakoutBoardFreqEdits[ddsInc].colorState = 0;
		breakoutBoardFreqEdits[ddsInc].RedrawWindow();
		breakoutBoardAmpEdits[ddsInc].colorState = 0;
		breakoutBoardAmpEdits[ddsInc].RedrawWindow();
	}
}


void DDSSystem::organizeDDSCommands(UINT variation)
{
	// each element of this is a different time (the double), and associated with each time is a vector which locates 
	// which commands were at this time, for
	// ease of retrieving all of the values in a moment.
	timeOrganizer.clear();

	std::vector<DDSCommand> tempEvents(ddsCommandList[variation]);
	// sort the events by time. using a lambda here.
	std::sort(tempEvents.begin(), tempEvents.end(),
		[](DDSCommand a, DDSCommand b) {return a.time < b.time; });
	for (UINT commandInc = 0; commandInc < tempEvents.size(); commandInc++)
	{
		// because the events are sorted by time, the time organizer will already be sorted by time, and therefore I 
		// just need to check the back value's time.
		if (commandInc == 0 || fabs(tempEvents[commandInc].time - timeOrganizer.back().first) > 2 * DBL_EPSILON)
		{
			// new time
			timeOrganizer.push_back({ tempEvents[commandInc].time,
									std::vector<DDSCommand>({ tempEvents[commandInc] }) });
		}
		else
		{
			// old time
			timeOrganizer.back().second.push_back(tempEvents[commandInc]);
		}
	}
	/// make the snapshots
	if (timeOrganizer.size() == 0)
	{
		// no commands, that's fine.
		return;
	}
}


std::array< std::array<double, 2>, 12> DDSSystem::getFinalSnapshot()
{
	if (ddsSnapshots.size() != 0)
	{
		if (ddsSnapshots.back().size() != 0)
		{
			return ddsSnapshots.back().back().ddsValues;
		}
		else
		{
			thrower("No DDS Events");
		}
	}
	else
	{
		thrower("No DDS Events");
	}
}


std::array<std::string, 12> DDSSystem::getAllNames()
{
	return ddsNames;
}


/*
 * IMPORTANT: this does not actually change any of the outputs of the board. It is meant to be called when things have
 * happened such that the control doesn't know what it's own status is, e.g. at the end of an experiment, since the
 * program doesn't change it's internal memory of all of the status of the dacs as the experiment runs. (it can't,
 * besides it would intensive to keep track of that in real time).
 */
void DDSSystem::setDDSStatusNoForceOut(std::array<std::array<double, 2>, 12> status)
{
	// set the internal values
	ddsValues = status;
	// change the edits
	for (UINT dacInc = 0; dacInc < ddsLabels.size(); dacInc++)
	{
		std::string freqStr;
		std::string ampStr;
		if (roundToDDSPrecision)
		{
			double valFreq = roundToDDSResolutionFreq(ddsValues[dacInc][0]);
			double valAmp = roundToDDSResolutionAmp(ddsValues[dacInc][1]);
			freqStr = str(valFreq, 13, true);
			ampStr = str(valAmp, 13, true);
		}
		else
		{
			freqStr = str(ddsValues[dacInc][0], 13, true);
			ampStr = str(ddsValues[dacInc][1], 13, true);
		}
		/*breakoutBoardEdits[dacInc].SetWindowText(cstr(valStr));
		breakoutBoardEdits[dacInc].colorState = 0;*/
	}
}


double DDSSystem::roundToDDSResolutionFreq(double num)
{
	return long((num + ddsResolution[0] / 2) / ddsResolution[0]) * ddsResolution[0];
}

double DDSSystem::roundToDDSResolutionAmp(double num)
{
	return long((num + ddsResolution[1] / 2) / ddsResolution[1]) * ddsResolution[1];
}


std::string DDSSystem::getErrorMessage(int errorCode)
{
	char errorChars[2048];
	// Get the actual error message. This is much surperior to getErrorString function.
	DAQmxGetExtendedErrorInfo(errorChars, 2048);
	std::string errorString(errorChars);
	return errorString;
}


void DDSSystem::prepareForce()
{
	ddsCommandList.resize(1);
	ddsSnapshots.resize(1);
	ddsChannelSnapshots = std::vector<std::vector<DDSChannelSnapshot>>(1);
}


void DDSSystem::interpretKey(std::vector<variableType>& variables, std::string& warnings)
{
	UINT variations;
	variations = variables.front().keyValues.size();
	if (variations == 0)
	{
		variations = 1;
	}
	/// imporantly, this sizes the relevant structures.
	ddsCommandList = std::vector<std::vector<DDSCommand>>(variations);
	ddsSnapshots = std::vector<std::vector<DDSSnapshot>>(variations);
	ddsChannelSnapshots = std::vector<std::vector<DDSChannelSnapshot>>(variations);

	bool resolutionWarningPosted = false;
	bool nonIntegerWarningPosted = false;

	for (UINT variationInc = 0; variationInc < variations; variationInc++)
	{
		for (UINT eventInc = 0; eventInc < ddsCommandFormList.size(); eventInc++)
		{
			DDSCommand tempEvent;
			tempEvent.line = ddsCommandFormList[eventInc].line;
			// Deal with time.
			if (ddsCommandFormList[eventInc].time.first.size() == 0)
			{
				// no variable portion of the time.
				tempEvent.time = ddsCommandFormList[eventInc].time.second;
			}
			else
			{
				double varTime = 0;
				for (auto variableTimeString : ddsCommandFormList[eventInc].time.first)
				{
					varTime += variableTimeString.evaluate(variables, variationInc);
				}
				tempEvent.time = varTime + ddsCommandFormList[eventInc].time.second;
			}

			if (ddsCommandFormList[eventInc].commandName == "ddsamp:")
			{
				/// single point.
				////////////////
				// deal with amp
				tempEvent.amp = ddsCommandFormList[eventInc].initVal.evaluate(variables, variationInc);
				tempEvent.endAmp = tempEvent.amp;
				tempEvent.rampTime = 0;
				tempEvent.freq = 0;
				ddsCommandList[variationInc].push_back(tempEvent);
			}
			else if (ddsCommandFormList[eventInc].commandName == "ddsfreq:")
			{
				/// single point.
				////////////////
				// deal with amp
				tempEvent.freq = ddsCommandFormList[eventInc].initVal.evaluate(variables, variationInc);
				tempEvent.endFreq = tempEvent.freq;
				tempEvent.rampTime = 0;
				tempEvent.amp = 0;
				ddsCommandList[variationInc].push_back(tempEvent);
			}
			else if (ddsCommandFormList[eventInc].commandName == "ddslinspaceamp:")
			{
				// interpret ramp time command. I need to know whether it's ramping or not.
				double rampTime = ddsCommandFormList[eventInc].rampTime.evaluate(variables, variationInc);
				/// many points to be made.
				// convert initValue and finalValue to doubles to be used 
				double initValue, finalValue, numSteps;
				initValue = ddsCommandFormList[eventInc].initVal.evaluate(variables, variationInc);
				// deal with final value;
				finalValue = ddsCommandFormList[eventInc].finalVal.evaluate(variables, variationInc);
				// deal with numPoints
				numSteps = ddsCommandFormList[eventInc].numSteps.evaluate(variables, variationInc);
				double rampInc = (finalValue - initValue) / numSteps;
				if ((fabs(rampInc) < DDS_MAX_AMP / pow(2, 10)) && !resolutionWarningPosted)
				{
					resolutionWarningPosted = true;
					warnings += "Warning: numPoints of " + str(numSteps) + " results in an amplitude ramp increment of "
						+ str(rampInc) + " is below the resolution of the ddss (which is " + str(DDS_MAX_AMP) + "/2^10 = "
						+ str(DDS_MAX_AMP / pow(2, 10)) + "). \r\n";
				}
				double timeInc = rampTime / numSteps;
				double initTime = tempEvent.time;
				double currentTime = tempEvent.time;
				double val = initValue;

				for (auto stepNum : range(numSteps))
				{
					tempEvent.amp = val;
					tempEvent.time = currentTime;
					tempEvent.endAmp = val;
					tempEvent.rampTime = 0;
					tempEvent.freq = 0;
					ddsCommandList[variationInc].push_back(tempEvent);
					currentTime += timeInc;
					val += rampInc;
				}
				// and get the final amp.
				tempEvent.amp = finalValue;
				tempEvent.time = initTime + rampTime;
				tempEvent.endAmp = finalValue;
				tempEvent.rampTime = 0;
				tempEvent.freq = 0;
				ddsCommandList[variationInc].push_back(tempEvent);
			}
			else if (ddsCommandFormList[eventInc].commandName == "ddslinspacefreq:")
			{
				// interpret ramp time command. I need to know whether it's ramping or not.
				double rampTime = ddsCommandFormList[eventInc].rampTime.evaluate(variables, variationInc);
				/// many points to be made.
				// convert initValue and finalValue to doubles to be used 
				double initValue, finalValue, numSteps;
				initValue = ddsCommandFormList[eventInc].initVal.evaluate(variables, variationInc);
				// deal with final value;
				finalValue = ddsCommandFormList[eventInc].finalVal.evaluate(variables, variationInc);
				// deal with numPoints
				numSteps = ddsCommandFormList[eventInc].numSteps.evaluate(variables, variationInc);
				double rampInc = (finalValue - initValue) / numSteps;
				if ((fabs(rampInc) < 500.0 / pow(2, 32)) && !resolutionWarningPosted)
				{
					resolutionWarningPosted = true;
					warnings += "Warning: numPoints of " + str(numSteps) + " results in a ramp increment of "
						+ str(rampInc) + " is below the frequency resolution of the ddss (which is 500/2^32 = "
						+ str(500.0 / pow(2, 32)) + "). \r\n";
				}
				double timeInc = rampTime / numSteps;
				double initTime = tempEvent.time;
				double currentTime = tempEvent.time;
				double val = initValue;

				for (auto stepNum : range(numSteps))
				{
					tempEvent.freq = val;
					tempEvent.time = currentTime;
					tempEvent.endFreq = val;
					tempEvent.rampTime = 0;
					tempEvent.amp = 0;
					ddsCommandList[variationInc].push_back(tempEvent);
					currentTime += timeInc;
					val += rampInc;
				}
				// and get the final amp.
				tempEvent.freq = finalValue;
				tempEvent.time = initTime + rampTime;
				tempEvent.endFreq = finalValue;
				tempEvent.rampTime = 0;
				tempEvent.amp = 0;
				ddsCommandList[variationInc].push_back(tempEvent);
			}
			else if (ddsCommandFormList[eventInc].commandName == "ddsrampamp:")
			{
				double rampTime = ddsCommandFormList[eventInc].rampTime.evaluate(variables, variationInc);
				// convert initValue and finalValue to doubles to be used 
				double initValue, finalValue, numSteps;
				initValue = ddsCommandFormList[eventInc].initVal.evaluate(variables, variationInc);
				// deal with final value;
				finalValue = ddsCommandFormList[eventInc].finalVal.evaluate(variables, variationInc);
				// set votlage resolution to be maximum allowed by the ramp range and time
				numSteps = rampTime / DDS_TIME_RESOLUTION;
				double rampInc = (finalValue - initValue) / numSteps;
				if ((fabs(rampInc) < DDS_MAX_AMP / pow(2, 10)) && !resolutionWarningPosted)
				{
					resolutionWarningPosted = true;
					warnings += "Warning: numPoints of " + str(numSteps) + " results in a ramp increment of "
						+ str(rampInc) + " is below the amplitude resolution of the dacs (which is " + str(DDS_MAX_AMP) + "/2^10 = "
						+ str(DDS_MAX_AMP / pow(2, 10)) + "). Ramp will not run.\r\n";
				}
				if (numSteps > 65535) {
					warnings += "Warning: numPoints of " + str(numSteps) + " is larger than the max time of the DDS ramps. Ramp will be truncated. \r\n";
				}

				double initTime = tempEvent.time;

				// for ddsrampamp, pass the ramp points and time directly to a single ddsCommandList element
				tempEvent.amp = initValue;
				tempEvent.endAmp = finalValue;
				tempEvent.time = initTime;
				tempEvent.rampTime = rampTime;
				tempEvent.freq = 0;
				ddsCommandList[variationInc].push_back(tempEvent);
			}
			else if (ddsCommandFormList[eventInc].commandName == "ddsrampfreq:")
			{
				double rampTime = ddsCommandFormList[eventInc].rampTime.evaluate(variables, variationInc);
				// convert initValue and finalValue to doubles to be used 
				double initValue, finalValue, numSteps;
				initValue = ddsCommandFormList[eventInc].initVal.evaluate(variables, variationInc);
				// deal with final value;
				finalValue = ddsCommandFormList[eventInc].finalVal.evaluate(variables, variationInc);
				// set votlage resolution to be maximum allowed by the ramp range and time
				numSteps = rampTime / DDS_TIME_RESOLUTION;
				double rampInc = (finalValue - initValue) / numSteps;
				if ((fabs(rampInc) < 500.0 / pow(2, 32)) && !resolutionWarningPosted)
				{
					resolutionWarningPosted = true;
					warnings += "Warning: numPoints of " + str(numSteps) + " results in a ramp increment of "
						+ str(rampInc) + " is below the frequency resolution of the ddss (which is 500/2^32 = "
						+ str(500.0 / pow(2, 32)) + "). Ramp will not run.\r\n";
				}
				if (numSteps > 65535) {
					warnings += "Warning: numPoints of " + str(numSteps) + " is larger than the max time of the DDS ramps. Ramp will be truncated. \r\n";
				}

				double initTime = tempEvent.time;

				// for ddsrampfreq, pass the ramp points and time directly to a single ddsCommandList element
				tempEvent.freq = initValue;
				tempEvent.endFreq = finalValue;
				tempEvent.time = initTime;
				tempEvent.rampTime = rampTime;
				tempEvent.amp = 0;
				ddsCommandList[variationInc].push_back(tempEvent);
			}
			else
			{
				thrower("ERROR: Unrecognized dds command name: " + ddsCommandFormList[eventInc].commandName);
			}
		}
	}
}


UINT DDSSystem::getNumberSnapshots(UINT variation)
{
	return ddsSnapshots[variation].size();
}


void DDSSystem::checkTimingsWork(UINT variation)
{

}

ULONG DDSSystem::getNumberEvents(UINT variation)
{
	return ddsSnapshots[variation].size();
}


// note that this is not directly tied to changing any "current" parameters in the DDSSystem object (it of course changes a list parameter). The 
// DacSystem object "current" parameters aren't updated to reflect an experiment, so if this is called for a force out, it should be called in conjuction
// with changing "currnet" parameters in the DacSystem object.
void DDSSystem::setDDSCommandForm(DDSCommandForm command)
{
	ddsCommandFormList.push_back(command);
	// you need to set up a corresponding trigger to tell the dacs to change the output at the correct time. 
	// This is done later on interpretation of ramps etc.
}

void DDSSystem::lockPLLs()
{
	try
	{
		//
		int tcp_connect;
		try
		{
			tcp_connect = zynq_tcp.connectTCP(ZYNQ_ADDRESS);
		}
		catch (Error& err)
		{
			tcp_connect = 1;
			errBox(err.what());
		}

		if (tcp_connect == 0)
		{
			zynq_tcp.writeCommand("lockPLL");
			zynq_tcp.disconnect();
		}
		else
		{
			throw("connection to zynq failed. can't lock DDS PLLs\n");
		}
	}
	// I catch here because it's the constructor, and catching elsewhere is weird.
	catch (Error& exception)
	{
		errBox(exception.what());
	}
}

void DDSSystem::setDDSsAmpFreq()
{

	prepareForce();

	DDSChannelSnapshot channelSnapshotAmp;
	DDSChannelSnapshot channelSnapshotFreq;

	int ddsCommandTime = 5;

	for (int line = 0; line < 4; ++line) {
		for (int dds = 0; dds < 3; ++dds) {
			channelSnapshotFreq.ampOrFreq = 'f'; //freq change
			channelSnapshotFreq.val = ddsValues[(4 * dds) + line][0];
			channelSnapshotFreq.endVal = 0;

			channelSnapshotAmp.ampOrFreq = 'a'; // amp change
			channelSnapshotAmp.val = ddsValues[(4 * dds) + line][1];
			channelSnapshotAmp.endVal = 0;

			channelSnapshotFreq.time = 1 + ddsCommandTime*line;
			channelSnapshotFreq.rampTime = 0;
			channelSnapshotFreq.channel = (4*dds) + line;

			channelSnapshotAmp.time = ddsCommandTime*(1 + line);
			channelSnapshotAmp.rampTime = 0;
			channelSnapshotAmp.channel = (4 * dds) + line;

			ddsChannelSnapshots[0].push_back(channelSnapshotFreq);
			ddsChannelSnapshots[0].push_back(channelSnapshotAmp);
		}
	}

	writeDDSs(0, false);
}

void DDSSystem::checkValuesAgainstLimits(UINT variation)
{
	for (UINT line = 0; line < ddsNames.size(); line++)
	{
		for (auto snapshot : ddsSnapshots[variation])
		{
			if (snapshot.ddsValues[line][0] > ddsMaxFreq[line] || snapshot.ddsValues[line][0] < ddsMinFreq[line])
			{
				thrower("ERROR: Attempted to set DDS" + str(line) + " value outside min/max freq range for this line. The "
					"value was " + str(snapshot.ddsValues[line][0]) + ", while the minimum accepted value is " +
					str(ddsMinAmp[line]) + " and the maximum value is " + str(ddsMaxAmp[line]) + ". "
					"Change the min/max if you actually need to set this value.\r\n");
			}
			if (snapshot.ddsValues[line][1] > ddsMaxAmp[line] || snapshot.ddsValues[line][1] < ddsMinAmp[line])
			{
				thrower("ERROR: Attempted to set DDS" + str(line) + " value outside min/max amp range for this line. The "
					"value was " + str(snapshot.ddsValues[line][1]) + ", while the minimum accepted value is " +
					str(ddsMinAmp[line]) + " and the maximum value is " + str(ddsMaxAmp[line]) + ". "
					"Change the min/max if you actually need to set this value.\r\n");
			}
		}
	}
}

// this is a function called in preparation for forcing a dac change. Remember, you need to call ___ to actually change things.
void DDSSystem::prepareDDSForceChange(int line, std::array<double, 2> val)
{
	// change parameters in the DDSSystem object so that the object knows what the current settings are.
	//std::string volt = str(roundToDacResolution(voltage));
	std::string freqStr;
	std::string ampStr;
	if (roundToDDSPrecision)
	{
		double roundedValFreq = roundToDDSResolutionFreq(val[0]);
		double roundedValAmp = roundToDDSResolutionAmp(val[1]);
		freqStr = str(roundedValFreq, 13);
		ampStr = str(roundedValAmp, 13);
	}
	else
	{
		freqStr = str(val[0], 13);
		ampStr = str(val[1], 13);
	}
	if (freqStr.find(".") != std::string::npos)
	{
		// then it's a double. kill extra zeros on the end.
		freqStr.erase(freqStr.find_last_not_of('0') + 1, std::string::npos);
	}

	breakoutBoardFreqEdits[line].SetWindowText(cstr(freqStr));
	breakoutBoardAmpEdits[line].SetWindowText(cstr(ampStr));
	ddsValues[line] = val;

	setForceDDSEvent(line, val, 0);
}


void DDSSystem::setForceDDSEvent(int line, std::array<double, 2> val, UINT variation)
{
	if (val[0] > ddsMaxFreq[line] || val[0] < ddsMinFreq[line])
	{
		thrower("ERROR: Attempted to set DDS" + str(line) + " value outside min/max frequency range for this line. The "
			"value was " + str(val[0]) + ", while the minimum accepted value is " +
			str(ddsMinFreq[line]) + " and the maximum value is " + str(ddsMaxFreq[line]) + ". "
			"Change the min/max if you actually need to set this value.\r\n");
	}

	if (val[1] > ddsMaxAmp[line] || val[1] < ddsMinAmp[line])
	{
		thrower("ERROR: Attempted to set DDS" + str(line) + " value outside min/max amplitude range for this line. The "
			"value was " + str(val[1]) + ", while the minimum accepted value is " +
			str(ddsMinAmp[line]) + " and the maximum value is " + str(ddsMaxAmp[line]) + ". "
			"Change the min/max if you actually need to set this value.\r\n");
	}

	DDSCommand eventInfo;
	eventInfo.line = line;
	eventInfo.freq = val[0];
	eventInfo.amp = val[1];

}


void DDSSystem::resetDDSEvents()
{
	ddsCommandFormList.clear();
	ddsCommandList.clear();
	ddsSnapshots.clear();
}

void DDSSystem::configureClocks(UINT variation, bool loadSkip)
{

}


void DDSSystem::writeDDSs(UINT variation, bool loadSkip)
{

	//dioFPGA[variation].write();
	int tcp_connect;
	try
	{
		tcp_connect = zynq_tcp.connectTCP(ZYNQ_ADDRESS);
	}
	catch (Error& err)
	{
		tcp_connect = 1;
		errBox(err.what());
	}

	if (tcp_connect == 0)
	{
		//zynq_tcp.writeCommand("disableSeq");
		//zynq_tcp.writeCommand("lockPLL");
		zynq_tcp.writeDDSs(ddsChannelSnapshots[variation]);
		zynq_tcp.disconnect();
	}
	else
	{
		throw("connection to zynq failed. can't write DDS data\n");
	}
}


void DDSSystem::makeFinalDataFormat(UINT variation)
{
	DDSChannelSnapshot channelSnapshot;

	//for each channel with a changed amp or freq add a ddsSnapshot to the final list
	for (UINT commandInc = 0; commandInc < timeOrganizer.size(); commandInc++) {
		for (UINT zeroInc = 0; zeroInc < timeOrganizer[commandInc].second.size(); zeroInc++)
		{
			if (timeOrganizer[commandInc].second[zeroInc].freq == 0) {
				channelSnapshot.ampOrFreq = 'a'; // amp change
				channelSnapshot.val = timeOrganizer[commandInc].second[zeroInc].amp;
				channelSnapshot.endVal = timeOrganizer[commandInc].second[zeroInc].endAmp;
			}
			else
			{
				channelSnapshot.ampOrFreq = 'f'; //freq change
				channelSnapshot.val = timeOrganizer[commandInc].second[zeroInc].freq;
				channelSnapshot.endVal = timeOrganizer[commandInc].second[zeroInc].endFreq;
			}
			channelSnapshot.time = timeOrganizer[commandInc].first;
			channelSnapshot.channel = timeOrganizer[commandInc].second[zeroInc].line;

			channelSnapshot.rampTime = timeOrganizer[commandInc].second[zeroInc].rampTime;
			ddsChannelSnapshots[variation].push_back(channelSnapshot);
		}
	}
}


int DDSSystem::getDDSIdentifier(std::string name)
{
	for (UINT ddsInc = 0; ddsInc < ddsValues.size(); ddsInc++)
	{
		// check names set by user.
		std::transform(ddsNames[ddsInc].begin(), ddsNames[ddsInc].end(), ddsNames[ddsInc].begin(), ::tolower);
		if (name == ddsNames[ddsInc])
		{
			return ddsInc;
		}
		// check standard names which are always acceptable.
		if (name == "dac" + str(ddsInc))
		{
			return ddsInc;
		}
	}
	// not an identifier.
	return -1;
}

void DDSSystem::setAmpMinMax(int ddsNumber, double min, double max)
{
	if (!(min <= max))
	{
		thrower("ERROR: Min dss amp value must be less than max dss amp value.");
	}
	if (min < 0 || min > 100 || max < 0 || max > 100)
	{
		thrower("ERROR: Min and max dss amp values must be within [0, 100].");
	}
	ddsMinAmp[ddsNumber] = min;
	ddsMaxAmp[ddsNumber] = max;
}

void DDSSystem::setFreqMinMax(int ddsNumber, double min, double max)
{
	if (!(min <= max))
	{
		thrower("ERROR: Min dss freq value must be less than max dss freq value.");
	}
	if (min < 0 || min > 500 || max < 0 || max > 500)
	{
		thrower("ERROR: Min and max dss freq values must be within [0, 500].");
	}
	ddsMinFreq[ddsNumber] = min;
	ddsMaxFreq[ddsNumber] = max;
}


std::pair<double, double> DDSSystem::getDDSAmpRange(int dacNumber)
{
	return { ddsMinAmp[dacNumber], ddsMaxAmp[dacNumber] };
}

std::pair<double, double> DDSSystem::getDDSFreqRange(int dacNumber)
{
	return { ddsMinFreq[dacNumber], ddsMaxFreq[dacNumber] };
}


void DDSSystem::setName(int dacNumber, std::string name, cToolTips& toolTips, AuxiliaryWindow* master)
{
	if (name == "")
	{
		// no empty names allowed.
		return;
	}
	std::transform(name.begin(), name.end(), name.begin(), ::tolower);
	ddsNames[dacNumber] = name;
	//breakoutBoardEdits[dacNumber].setToolTip(name, toolTips, master);
}


std::string DDSSystem::getName(int ddsNumber)
{
	return ddsNames[ddsNumber];
}


HBRUSH DDSSystem::handleColorMessage(CWnd* window, brushMap brushes, rgbMap rgbs, CDC* cDC)
{
	//int controlID = GetDlgCtrlID(*window);
	//if (controlID >= ddsLabels[0].GetDlgCtrlID() && controlID <= ddsLabels.back().GetDlgCtrlID() )
	//{
	//	cDC->SetBkColor(rgbs["theme BG1"]);
	//	cDC->SetTextColor(rgbs["theme foreground"]);
	//	return *brushes["theme BG1"];
	//}
	//else if (controlID >= breakoutBoardEdits[0].GetDlgCtrlID() && controlID <= breakoutBoardEdits.back().GetDlgCtrlID())
	//{
	//	int editNum = (controlID - breakoutBoardEdits[0].GetDlgCtrlID());
	//	if (breakoutBoardEdits[editNum].colorState == 0)
	//	{
	//		// default.
	//		cDC->SetTextColor(rgbs["theme foreground"]);
	//		cDC->SetBkColor(rgbs["theme input"]);
	//		return *brushes["theme input"];
	//	}
	//	else if (breakoutBoardEdits[editNum].colorState == 1)
	//	{
	//		// in this case, the actuall setting hasn't been changed despite the edit being updated.
	//		cDC->SetTextColor(rgbs["theme foreground"]);
	//		cDC->SetBkColor(rgbs["Red"]);
	//		return *brushes["Red"];
	//	}
	//	else if (breakoutBoardEdits[editNum].colorState == -1)
	//	{
	//		// in use during experiment.
	//		cDC->SetTextColor(rgbs["Black"]);
	//		cDC->SetBkColor(rgbs["theme foreground"]);
	//		return *brushes["theme foreground"];
	//	}
	//}
	return NULL;
}


UINT DDSSystem::getNumberOfDDSs()
{
	return ddsValues.size();
}


std::array<double, 2> DDSSystem::getDDSValue(int ddsNumber)
{
	return ddsValues[ddsNumber];
}


void DDSSystem::shadeDDSs(std::vector<UINT>& ddsShadeLocations)
{
	/*for (UINT shadeInc = 0; shadeInc < ddsShadeLocations.size(); shadeInc++)
	{
		breakoutBoardEdits[ddsShadeLocations[shadeInc]].colorState = -1;
		breakoutBoardEdits[ddsShadeLocations[shadeInc]].SetReadOnly(true);
		breakoutBoardEdits[ddsShadeLocations[shadeInc]].RedrawWindow();
	}
	for (auto& ctrl : breakoutBoardEdits)
	{
		ctrl.EnableWindow(0);
	}*/
}


void DDSSystem::unshadeDDSs()
{
	/*for (UINT shadeInc = 0; shadeInc < breakoutBoardEdits.size(); shadeInc++)
	{
		breakoutBoardEdits[shadeInc].EnableWindow();
		if (breakoutBoardEdits[shadeInc].colorState == -1)
		{
			breakoutBoardEdits[shadeInc].colorState = 0;
			breakoutBoardEdits[shadeInc].SetReadOnly(false);
			breakoutBoardEdits[shadeInc].RedrawWindow();
		}
	}*/
}

