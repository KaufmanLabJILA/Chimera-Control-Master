#include "stdafx.h"
#include "makoCamera.h"


makoCamera::makoCamera()
{

}

makoCamera::~makoCamera()
{
	apiController.ShutDown();
}

void makoCamera::startMako(int cameraIndex) {
	err = apiController.StartUp();
	if (VmbErrorSuccess != err)
	{
		std::string strError = apiController.ErrorCodeToMessage(err);
		thrower("An error occurred: " + strError);
	}

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
}

std::string makoCamera::getCameraStr() {
	return strCameraID;
}

VmbErrorType makoCamera::saveFrame(const char * fileName) {

	VmbFrameStatusType status = VmbFrameStatusIncomplete;

	err = apiController.StartUp();

	if (VmbErrorSuccess == err)
	{

		AVT::VmbAPI::FramePtr pFrame;
		err = apiController.AcquireSingleImage(strCameraID, pFrame);
		if (VmbErrorSuccess == err)
		{
			err = pFrame->GetReceiveStatus(status);
			if (VmbErrorSuccess == err
				&& VmbFrameStatusComplete == status)
			{
				VmbPixelFormatType ePixelFormat = VmbPixelFormatMono8;
				err = pFrame->GetPixelFormat(ePixelFormat);
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
						err = pFrame->GetImageSize(nImageSize);
						if (VmbErrorSuccess == err)
						{
							VmbUint32_t nWidth = 0;
							err = pFrame->GetWidth(nWidth);
							if (VmbErrorSuccess == err)
							{
								VmbUint32_t nHeight = 0;
								err = pFrame->GetHeight(nHeight);
								if (VmbErrorSuccess == err)
								{
									VmbUchar_t *pImage = NULL;
									err = pFrame->GetImage(pImage);
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
											if (0 == AVTWriteBitmapToFile(&bitmap, fileName))
											{
												std::cout << "Could not write bitmap to file.\n";
												err = VmbErrorOther;
											}
											else
											{
												std::cout << "Bitmap successfully written to file \"" << fileName << "\"\n";
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

	return err;
}
