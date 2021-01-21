#include "stdafx.h"
#include "makoCamera.h"


makoCamera::makoCamera(int cameraIndex)
{
	err = apiController.StartUp();
	if (VmbErrorSuccess != err)
	{
		std::string strError = apiController.ErrorCodeToMessage(err);
		thrower("An error occurred: " + strError + "\n");
	}

	AVT::VmbAPI::CameraPtrVector cameras = apiController.GetCameraList();
	if (cameras.size() <= cameraIndex + 1)
	{
		err = VmbErrorNotFound;
		std::string strError = apiController.ErrorCodeToMessage(err);
		thrower("An error occurred: " + strError + "\n");
	}
	else
	{
		err = cameras[cameraIndex]->GetID(strCameraID);
	}
}

makoCamera::~makoCamera()
{
	apiController.ShutDown();
}

VmbErrorType makoCamera::saveFrame(const char * fileName) {

	VmbFrameStatusType status = VmbFrameStatusIncomplete;

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
											thrower("Could not create bitmap.\n");
											err = VmbErrorResources;
										}
										else
										{
											// Save the bitmap
											if (0 == AVTWriteBitmapToFile(&bitmap, fileName))
											{
												thrower("Could not write bitmap to file.\n");
												err = VmbErrorOther;
											}
											else
											{
												//std::cout << "Bitmap successfully written to file \"" << fileName << "\"\n";
												// Release the bitmap's buffer
												if (0 == AVTReleaseBitmap(&bitmap))
												{
													thrower("Could not release the bitmap.\n");
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

	if (VmbFrameStatusIncomplete == status)
	{
		thrower("received frame was not complete\n");
	}

	return err;
}
