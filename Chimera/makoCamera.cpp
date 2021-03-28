#include "stdafx.h"
#include "makoCamera.h"


makoCamera::makoCamera()
{
	if (!MAKO_SAFEMODE) {
		apiController.StartUp();
	}
}

makoCamera::~makoCamera()
{
	if (!MAKO_SAFEMODE) {
		apiController.ShutDown();
	}
}

void makoCamera::startMako(int cameraIndex) {

	if (!MAKO_SAFEMODE) {
		std::string strError = apiController.ErrorCodeToMessage(err);

		AVT::VmbAPI::CameraPtrVector cameras = apiController.GetCameraList();
		if (cameras.size() < cameraIndex + 1)
		{
			err = VmbErrorNotFound;
			std::string strError = apiController.ErrorCodeToMessage(err);
			thrower("An error occurred. Number cameras found = " + std::to_string(cameras.size()));
		}
		else
		{
			err = cameras[cameraIndex]->GetID(strCameraID);
		}

		VmbErrorType err = apiController.openCamera(strCameraID);
		if (VmbErrorSuccess == err)
		{
			// send status somehow
		}
	}
}

std::string makoCamera::getCameraStr() {
	return strCameraID;
}

void makoCamera::closeCamera() {
	if (!MAKO_SAFEMODE) {
		VmbErrorType err = apiController.closeCamera();
	}
}

void makoCamera::setupAcquisition() {
	if (!MAKO_SAFEMODE) {
		AVT::VmbAPI::FramePtr pFrame;
		pFrames.push_back(pFrame);
		VmbErrorType err = apiController.setupAcquisition(strCameraID, pFrames);
	}
}

void makoCamera::finishAcquisition() {
	if (!MAKO_SAFEMODE) {
		VmbErrorType err = apiController.finishAcquisition();

		VmbFrameStatusType status = VmbFrameStatusIncomplete;
		if (VmbErrorSuccess == err)
		{
			err = pFrames.back()->GetReceiveStatus(status);
			if (VmbErrorSuccess == err
				&& VmbFrameStatusComplete == status)
			{
				VmbPixelFormatType ePixelFormat = VmbPixelFormatMono8;
				err = pFrames.back()->GetPixelFormat(ePixelFormat);
				if (VmbErrorSuccess == err)
				{
					if ((VmbPixelFormatMono8 != ePixelFormat)
						&& (VmbPixelFormatRgb8 != ePixelFormat))
					{
						err = VmbErrorInvalidValue;
					}
					else
					{
						VmbUint32_t nImageSize = 0;
						err = pFrames.back()->GetImageSize(nImageSize);
						if (VmbErrorSuccess == err)
						{
							VmbUint32_t nWidth = 0;
							err = pFrames.back()->GetWidth(nWidth);
							if (VmbErrorSuccess == err)
							{
								VmbUint32_t nHeight = 0;
								err = pFrames.back()->GetHeight(nHeight);
								if (VmbErrorSuccess == err)
								{
									VmbUchar_t *pImage = NULL;
									err = pFrames.back()->GetImage(pImage);
									if (VmbErrorSuccess == err)
									{
										AVTBitmap bitmap;

										if (VmbPixelFormatRgb8 == ePixelFormat)
										{
											bitmap.colorCode = ColorCodeRGB24;
										}
										else
										{
											bitmap.colorCode = ColorCodeMono8;
										}

										bitmap.bufferSize = nImageSize;
										bitmap.width = nWidth;
										bitmap.height = nHeight;

										// Create the bitmap
										if (0 == AVTCreateBitmap(&bitmap, pImage))
										{
											std::cout << "Could not create bitmap.\n";
											err = VmbErrorResources;
										}
										else
										{
											// Save the bitmap
											if (0 == AVTWriteBitmapToFile(&bitmap, imageName.c_str()))
											{
												std::cout << "Could not write bitmap to file.\n";
												err = VmbErrorOther;
											}
											else
											{
												std::cout << "Bitmap successfully written to file \"" << imageName << "\"\n";
												// Release the bitmap's buffer
												if (0 == AVTReleaseBitmap(&bitmap))
												{
													std::cout << "Could not release the bitmap.\n";
													err = VmbErrorInternalFault;
												}
											}
										}
									}
								}
							}
						}
					}
				}
			}
		}


		if (VmbErrorSuccess != err)
		{
			std::string strError = apiController.ErrorCodeToMessage(err);
			std::cout << "\nAn error occurred: " << strError.c_str() << "\n";
		}
		if (VmbFrameStatusIncomplete == status)
		{
			std::cout << "received frame was not complete\n";
		}
	}

}
