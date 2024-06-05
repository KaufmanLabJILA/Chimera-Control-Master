#include "stdafx.h"
#include "DebugOptionsControl.h"
#include "commonFunctions.h"
#include "MainWindow.h"
#include "CameraWindow.h"
#include "AuxiliaryWindow.h"
#include <future>
#include "resource.h"
#include "idleSequence.h"

idleSequence::idleSequence()
{
	idleSequenceActive = false;
	killIdler = false;
	idleSequenceRunning = false;
}