 #pragma once

#include <string>
#include "Control.h"
#include "Windows.h"
#include <vector>
// #include "Andor.h"
#include "qcmos.h"

struct cameraPositions;

class CameraConfigurationSystem
{
	public:
		CameraConfigurationSystem(std::string fileSystemPath);
		~CameraConfigurationSystem();
		qcmosRunSettings openConfiguration( std::string configurationNameToOpen, qcmosRunSettings baseSettings );
		void saveConfiguration(bool isFromSaveAs, qcmosRunSettings settings );
		void saveConfigurationAs(std::string newConfigurationName, qcmosRunSettings settings );
		void renameConfiguration(std::string newConfigurationName );
		void deleteConfiguration();
		int checkSave();
		void initialize( cameraPositions& positions, CWnd* parent, bool isTriggerModeSensitive, int& id );
		void reorganizeControls(RECT parentRectangle, std::string mode);

		std::vector<std::string> searchForFiles(std::string locationToSearch, std::string extensions);
		void reloadCombo(std::string nameToLoad);
		std::string getComboText();
		bool fileExists(std::string filePathway);
		void updateSaveStatus(bool saved);

	private:
		std::string configurationName;
		std::string FILE_SYSTEM_PATH;
		bool configurationSaved;
		Control<CStatic> configLabel;
		Control<CComboBox> configCombo;
};
