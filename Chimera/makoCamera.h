#pragma once

#include <string>
#include <cstring>
#include <iostream>

#include "ApiController.h"
#include "Bitmap.h"

class makoCamera
{
public:
	makoCamera();
	~makoCamera();
	void startMako(int cameraIndex);
	void setupAcquisition();
	void finishAcquisition();
	void closeCamera();
	std::string getCameraStr();
	AVT::VmbAPI::Examples::ApiController apiController;
	VmbErrorType err;
	std::string strCameraID;
	std::string imageName;

private:
	std::vector<AVT::VmbAPI::FramePtr> pFrames;
};


