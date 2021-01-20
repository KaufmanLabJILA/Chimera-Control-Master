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

private:
	std::string strCameraID;
	AVT::VmbAPI::Examples::ApiController apiController;
	VmbErrorType err;
};


