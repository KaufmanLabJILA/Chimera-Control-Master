#pragma once

#include <string>
#include <cstring>
#include <iostream>

#include "ApiController.h"
#include "Bitmap.h"

class makoCamera
{
public:
	makoCamera(int cameraIndex);
	~makoCamera();
	VmbErrorType saveFrame(const char * fileName);
	std::string getCameraStr();
	AVT::VmbAPI::Examples::ApiController apiController;
	VmbErrorType err;
	std::string strCameraID;

private:
	
};


