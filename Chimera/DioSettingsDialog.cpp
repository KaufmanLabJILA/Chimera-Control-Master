#pragma once

#include "stdafx.h"
#include "DioSettingsDialog.h"
#include "Control.h"
#include <array>
#include "DioSystem.h"


BEGIN_MESSAGE_MAP(TtlSettingsDialog, CDialog)
	ON_COMMAND(IDOK, &TtlSettingsDialog::handleOk)
	ON_COMMAND(IDCANCEL, &TtlSettingsDialog::handleCancel)
END_MESSAGE_MAP()


TtlSettingsDialog::TtlSettingsDialog(ttlInputStruct* inputPtr, UINT dialogResource) : CDialog(dialogResource)
{
	input = inputPtr;
}


BOOL TtlSettingsDialog::OnInitDialog()
{
	int id = 345;

	POINT pos = { 60, 20 };

	int ttlrow, ttlcol, xpos, ypos;

	for (UINT rowInc = 0; rowInc < edits.size(); rowInc++)
	{
		pos.y = 20;
		pos.x = 20;
		ttlrow = (rowInc + 1) % 3;
		ttlcol = (3 - floor((rowInc + 1) / 3) - 1);
		ypos = ttlrow;
		rowLabels[rowInc].sPos = { ttlcol * (25 + 70 * 8) + 7, pos.y + ttlrow * 40 + 10, 
			(ttlcol + 1) * (25 + 70 * 8) + 7, pos.y + (ttlrow + 1) * 40 + 10};
		rowLabels[rowInc].Create(cstr(rowInc + 1), WS_CHILD | WS_VISIBLE | SS_LEFT, rowLabels[rowInc].sPos, this, id++);
		for (UINT numberInc = 0; numberInc < edits[rowInc].size(); numberInc++)
		{
			xpos = ttlcol * 8 + numberInc;
			edits[rowInc][numberInc].sPos = { pos.x + xpos * 70 + 25 * ttlcol, pos.y + 40 * ttlrow + 15, 
				pos.x + (xpos + 1) * 70 + 25 * ttlcol - 10, pos.y + 40 * (ttlrow + 1) + 5 };
			edits[rowInc][numberInc].Create(WS_CHILD | WS_VISIBLE | SS_SUNKEN | WS_BORDER | ES_AUTOHSCROLL | WS_TABSTOP,
				edits[rowInc][numberInc].sPos, this, id++);
			edits[rowInc][numberInc].SetWindowTextA(cstr(input->ttls->getName(rowInc, numberInc)));
		}
	}
	return FALSE;
}


void TtlSettingsDialog::handleOk()
{
	for (UINT rowInc = 0; rowInc < edits.size(); rowInc++)
	{
		for (UINT numberInc = 0; numberInc < edits[rowInc].size(); numberInc++)
		{
			CString name;
			edits[rowInc][numberInc].GetWindowText(name);
			if (isdigit(name[0]))
			{
				errBox("ERROR: " + str(name) + " is an invalid name; names cannot start with numbers.");
				return;
			}
			input->ttls->setName(rowInc, numberInc, str(name), input->toolTips, input->master);
		}
	}
	EndDialog((WPARAM)&input);
}

void TtlSettingsDialog::handleCancel()
{
	EndDialog((WPARAM)&input);
}
