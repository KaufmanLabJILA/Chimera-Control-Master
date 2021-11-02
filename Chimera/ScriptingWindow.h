#pragma once 
 
#include "stdafx.h" 
#include "Script.h" 
#include "ColorBox.h" 
#include "ProfileIndicator.h" 
 
class MainWindow; 
class CameraWindow; 
class AuxiliaryWindow; 
 
 
// a convenient structure for containing one object for each script. For example, the address of each script. 
template <typename type> struct scriptInfo 
{ 
	//type moog; 
	type awg;
	type gmoog; 
	type master; 
}; 
 
 
class ScriptingWindow : public CDialog 
{ 
	using CDialog::CDialog; 
	DECLARE_DYNAMIC(ScriptingWindow); 
 
	public: 
		ScriptingWindow(); 
 
		HBRUSH OnCtlColor(CDC* pDC, CWnd* pWnd, UINT nCtlColor); 
		 
		BOOL OnInitDialog() override; 
		void OnTimer(UINT_PTR eventID); 
 
		void passCommonCommand(UINT id); 
 
		void checkScriptSaves(); 
		void loadFriends(MainWindow* mainWin, CameraWindow* camWin, AuxiliaryWindow* masterWin); 
		void fillMasterThreadInput(MasterThreadInput* input); 
		BOOL OnToolTipText( UINT, NMHDR* pNMHDR, LRESULT* pResult ); 
		scriptInfo<std::string> getScriptNames(); 
		scriptInfo<bool> getScriptSavedStatuses(); 
		scriptInfo<std::string> getScriptAddresses(); 
		profileSettings getProfileSettings(); 
		std::string getSystemStatusString(); 
		BOOL PreTranslateMessage(MSG* pMsg); 
 
		void checkMasterSave(); 
 
		void handleNewConfig( std::ofstream& saveFile ); 
		void handleSavingConfig(std::ofstream& saveFile); 
 
		void updateScriptNamesOnScreen(); 
		void updateProfile(std::string text); 
		void considerScriptLocations(); 
		void recolorScripts(); 

 
		//void newMoogScript(); 
		//void openMoogScript(CWnd* parent); 
		//void openMoogScript(std::string name); 
		//void saveMoogScript(); 
		//void saveMoogScriptAs(CWnd* parent); 
		//void moogEditChange(); 

		void newAWGScript();
		void openAWGScript(CWnd* parent);
		void openAWGScript(std::string name);
		void saveAWGScript();
		void saveAWGScriptAs(CWnd* parent);
		void awgEditChange();
 
		void newGmoogScript(); 
		void openGmoogScript(CWnd* parent); 
		void openGmoogScript(std::string name); 
		void saveGmoogScript(); 
		void saveGmoogScriptAs(CWnd* parent); 
		void gmoogEditChange(); 
 
		void newMasterScript(); 
		std::string openMasterScriptFolder(CWnd* parent);
		int openMasterScriptByPath(std::string filepath);
		void openMasterScript(CWnd* parent); 
		void openMasterScript(std::string name); 
		void saveMasterScript(); 
		void saveMasterScriptAs(CWnd* parent); 
		void newMasterFunction(); 
		void saveMasterFunction(); 
		void saveMasterFunction_nocatch(); 
		void deleteMasterFunction(); 
		void masterEditChange(); 
		Communicator* comm(); 
 
		void changeBoxColor( systemInfo<char> colors ); 
		void updateConfigurationSavedStatus(bool status); 
		void OnCancel() override; 
		void OnSize(UINT nType, int cx, int cy); 
 
		//void handleMoogScriptComboChange(); 
		void handleAWGScriptComboChange();
		void handleGmoogScriptComboChange(); 
 
		void handleAgilentScriptComboChange(); 
		void handleMasterFunctionChange( ); 

		void handleOpenConfig(std::ifstream& configFile, int versionMajor, int versionMinor ); 
		void catchEnter(); 
		profileSettings getProfile(); 

	private: 
		DECLARE_MESSAGE_MAP(); 
		 
		MainWindow* mainWindowFriend; 
		CameraWindow* cameraWindowFriend; 
		AuxiliaryWindow* auxWindowFriend; 
		// 
		cToolTips tooltips; 
 
		Script masterScript, awgScript, gmoogScript; /*moogScript, */
		ColorBox statusBox; 
		ProfileIndicator profileDisplay; 
 
}; 
