#pragma once
#include "windows.h"
#include "DioSystem.h"
#include <unordered_map>

struct ttlInputStruct
{
	DioSystem* ttls;
	cToolTips toolTips;
	AuxiliaryWindow* master;
};

class TtlSettingsDialog : public CDialog
{
	public:
		TtlSettingsDialog(ttlInputStruct* inputPtr, UINT dialogResource);
		void handleOk();
		void handleCancel();
		BOOL OnInitDialog();

	private:
		DECLARE_MESSAGE_MAP();
		ttlInputStruct* input;
		std::array<Control<CStatic>, 24> numberlabels;
		std::array<Control<CStatic>, 3> rowLabels;
		std::array<std::array<Control<CEdit>, 24>, 3> edits;
};
