#pragma once
#include "windows.h"
#include "DacSystem.h"
#include <unordered_map>

struct dacInputStruct
{
	DacSystem* dacs;
	cToolTips toolTips;
	AuxiliaryWindow* master;
};

class DacSettingsDialog : public CDialog
{
	public:
		DECLARE_DYNAMIC(DacSettingsDialog);
		DacSettingsDialog(dacInputStruct* inputPtr, UINT dialogResource) : CDialog(dialogResource)
		{
			input = inputPtr;
		}
		//using CDialog::CDialog;

		BOOL OnInitDialog();
		void handleOk();
		void handleCancel();
	private:
		DECLARE_MESSAGE_MAP()
		
		//dacInputStruct input;
		std::array<Control<CStatic>, 32> numberLabels;
		std::array<Control<CEdit>, 32> nameEdits;
		std::array<Control<CEdit>, 32> minValEdits;
		std::array<Control<CEdit>, 32> maxValEdits;
		Control<CStatic> dac0BoardHeader;
		Control<CStatic> dac1BoardHeader;
		std::array<Control<CStatic>, 2> dacNumberHeaders;
		std::array<Control<CStatic>, 2> dacNameHeaders;
		std::array<Control<CStatic>, 2> dacMinValHeaders;
		std::array<Control<CStatic>, 2> dacMaxValHeaders;
		int startx = 30, starty = 40;
		int width = 63;
		int height = 28;
		int id = 445;
		dacInputStruct* input;
};
