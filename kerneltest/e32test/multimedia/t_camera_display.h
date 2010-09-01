// Copyright (c) 2006-2009 Nokia Corporation and/or its subsidiary(-ies).
// All rights reserved.
// This component and the accompanying materials are made available
// under the terms of the License "Eclipse Public License v1.0"
// which accompanies this distribution, and is available
// at the URL "http://www.eclipse.org/legal/epl-v10.html".
//
// Initial Contributors:
// Nokia Corporation - initial contribution.
//
// Contributors:
//
// Description:
// e32test\multimedia\t_camera_display.h
// 
//

#if !defined(__T_CAMERA_DISPLAY_H__)
#define __T_CAMERA_DISPLAY_H__

#include <e32test.h>
#include <d32camerasc.h>
#include <pixelformats.h>
#include "d_mmcsc.h"

_LIT(KCaptureModeCapsTitle, "Capture modes: ");
_LIT(KCaptureModeImage, "Image ");
_LIT(KCaptureModeVideo, "Video ");
_LIT(KCaptureModeViewFinder, "ViewFinder ");

_LIT(KPixelFormatCapsTitle, "Pixel formats: ");
_LIT(KPixelFormatConfTitle, "Current pixel format: ");
_LIT(KPixelFormatUnknown, "Pixel Format Unknown (Other) ");
_LIT(KPixelFormatRGB_565, "RGB565 ");
_LIT(KPixelFormatYUV_422Interleaved, "YUV422 Interleaved ");
_LIT(KPixelFormatSpeedTaggedJPEG, "Speed Tagged JPEG ");
_LIT(KPixelFormatJPEG, "JPEG ");

class TCamDisplayHandler
	{
public:
	TCamDisplayHandler();
	TInt Init();
	TInt SetConfig(const SDevCamFrameSize& aSize,const SDevCamPixelFormat& aPixelFormat);
	TInt Process(TUint8* aImageBaseAddress);
private:
	TInt Min(TInt aA, TInt aB);
	TInt ProcessRGB565(TUint8* aImageBaseAddress);
	TInt ProcessYUV422(TUint8* aImageBaseAddress);
private:
	/** Pixel format of the data to be converted and displayed */
	SDevCamPixelFormat iPixelFormat;
	/** Address of the top left hand corner of screen memory */
	TUint8* iVideoAddress;
	/** Width of the frame to be displayed, in pixels */
	TUint iWidth;
	/** Height of the frame to be displayed, in pixels */
	TUint iHeight;
	/** Width of the screen, in pixels */
	TUint iScreenWidth;
	/** Height of the screen, in pixels */
	TUint iScreenHeight;
	/** Number of bits per pixel */
	TInt iBitsPerPixel;
	};

void AppendPixelFormat(TDes& aBuffer, TUidPixelFormat aPixelFormat);
void PrintCamModes(TCameraCapsV02* aCaps,RTest& aTest);
void PrintCamConf(TCameraConfigV02& aConf,RTest& aTest);
void PrintBufferConf(TMmSharedChunkBufConfig& aBufConf,RTest& aTest);

#endif // __T_CAMERA_DISPLAY_H__
