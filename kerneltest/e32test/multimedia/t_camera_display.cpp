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
// e32test\multimedia\t_camera_display.cpp
// 
//

#include <e32test.h>
#include <e32svr.h>
#include <u32hal.h>
#include <videodriver.h>
#include "t_camera_display.h"

_LIT(KFrameSizeConfTitle,"Current frame size  :");
_LIT(KFrameSize, " %d x %d");

#define CLIP(a) if (a < 0) a = 0; else if (a > 255) a = 255;

/**
Constructor
*/
TCamDisplayHandler::TCamDisplayHandler()
	{}

/**
Initialise the display handler.
@return KErrNone if write was successful, otherwise one of the other system wide error codes.
*/
TInt TCamDisplayHandler::Init()
	{
	TScreenInfoV01 screenInfo;
	TPckg<TScreenInfoV01> screenInfoBuf(screenInfo);
	UserSvr::ScreenInfo(screenInfoBuf);
	iVideoAddress = (TUint8*) screenInfo.iScreenAddress;
	iScreenWidth = screenInfo.iScreenSize.iWidth;
	iScreenHeight = screenInfo.iScreenSize.iHeight;

	TPckgBuf<TVideoInfoV01> videoInfoBuf;
	UserSvr::HalFunction(EHalGroupDisplay, EDisplayHalCurrentModeInfo, &videoInfoBuf, NULL);
	iBitsPerPixel = videoInfoBuf().iBitsPerPixel;

	return(KErrNone);
	}

TInt TCamDisplayHandler::Min(TInt aA, TInt aB)
	{
	return (aA < aB) ? aA : aB;
	}

TInt TCamDisplayHandler::SetConfig(const SDevCamFrameSize& aSize,const SDevCamPixelFormat& aPixelFormat)
	{
	if (aPixelFormat.iPixelFormat==EUidPixelFormatYUV_422Interleaved || aPixelFormat.iPixelFormat==EUidPixelFormatRGB_565)
		iPixelFormat=aPixelFormat;
	else
		return(KErrArgument);

	iWidth = aSize.iWidth;
	iHeight = aSize.iHeight;

	return(KErrNone);
	}

/**
Post process a received image.
@return KErrNone if write was successful, otherwise one of the other system wide error codes.
*/
TInt TCamDisplayHandler::Process(TUint8* aImageBaseAddress)
	{
	switch (iPixelFormat.iPixelFormat)
		{
		case EUidPixelFormatYUV_422Interleaved:
			return(ProcessYUV422(aImageBaseAddress));
		case EUidPixelFormatRGB_565:
			return(ProcessRGB565(aImageBaseAddress));
		default:
			return(KErrNotSupported);
		}
	}

/**
Post process a received RGB565 image.
@return KErrNone if write was successful, otherwise one of the other system wide error codes.
*/
TInt TCamDisplayHandler::ProcessRGB565(TUint8* aImageBaseAddress)
	{
	TUint16* source = (TUint16*) aImageBaseAddress;
	TUint16 pixel;
	TInt sourceModulo, destModulo, width, height;
	TInt r = KErrNone;

	// Determine whether the screen or the picture to display is the widest, and calculate modulos
	// and clipping sizes appropriately
	if (iWidth < iScreenWidth)
		{
		width = iWidth;
		sourceModulo = 0;
		destModulo = (iScreenWidth - iWidth);
		}
	else
		{
		width = iScreenWidth;
		sourceModulo = (iWidth - iScreenWidth);
		destModulo = 0;
		}

	// Determine whether the screen or the picture to display is the highest
	height = (iHeight < iScreenHeight) ? iHeight : iScreenHeight;

	if (iBitsPerPixel == 16)
		{
		TUint16* dest = (TUint16*) iVideoAddress;

		// Loop around and copy the data directly onto the screen
		for (TInt line = 0; line < height; ++line)
			{
			for (TInt x = 0; x < width; ++x)
				{
				*dest++ = *source++;
				}

			source += sourceModulo;
			dest += destModulo;
			}
		}
	else if (iBitsPerPixel == 32)
		{
		TUint8* dest = iVideoAddress;

		destModulo *= 4;

		// Loop around and convert whatever part of the picture will fit onto the screen into BGRA,
		// writing it directly onto the screen
		for (TInt line = 0; line < height; ++line)
			{
			for (TInt x = 0; x < width; ++x)
				{
				pixel = *source++;
				*dest++= (TUint8) ((pixel & 0x001f) << 3);
				*dest++= (TUint8) ((pixel & 0x07e0) >> 3);
				*dest++= (TUint8) ((pixel & 0xf800) >> 8);
				*dest++ = 0xff;
				}

			source += sourceModulo;
			dest += destModulo;
			}
		}
	else
		{
		r = KErrNotSupported;
		}

	return r;
	}

/**
Post process a received YUV422 image.
@return KErrNone if write was successful, otherwise one of the other system wide error codes.
*/
TInt TCamDisplayHandler::ProcessYUV422(TUint8* aImageBaseAddress)
	{
	TUint16* dest16 = (TUint16*) iVideoAddress;
	TUint32* dest32 = (TUint32*) iVideoAddress;
	TUint8* source = aImageBaseAddress;
	TInt y, u, v, r, g, b, sourceModulo, destModulo, width, height;
	TInt retVal = KErrNone;

	// Determine whether the screen or the picture to display is the widest, and calculate modulos
	// and clipping sizes appropriately
	if (iWidth < iScreenWidth)
		{
		width = (iWidth / 2);
		sourceModulo = 0;
		destModulo = (iScreenWidth - iWidth);
		}
	else
		{
		width = (iScreenWidth / 2);
		sourceModulo = ((iWidth - iScreenWidth) * 2);
		destModulo = 0;
		}

	// Determine whether the screen or the picture to display is the highest
	height = (iHeight < iScreenHeight) ? iHeight : iScreenHeight;

	// Only 16 and 32 bits per pixel are supported.  It is also assumed that 16 bit will be RGB565 and
	// 32 bit will be BGRA.  You will need to add support for new formats if required
	if ((iBitsPerPixel == 16) || (iBitsPerPixel == 32))
		{
		// Loop around and convert whatever part of the picture will fit onto the screen into RGB565 or BGRA,
		// writing it directly onto the screen
		for (TInt line = 0; line < height; ++line)
			{
			for (TInt x = 0; x < width; ++x)
				{
				u = (source[0] - 128);
				v = (source[2] - 128);
				y = (source[3] - 16);

				r = ((298 * y + 409 * u) / 256);
				g = ((298 * y - 100 * v - 208 * u) / 256);
				b = ((298 * y + 516 * v) / 256);

				CLIP(r);
				CLIP(g);
				CLIP(b);

				if (iBitsPerPixel == 16)
					{
					*dest16++ = (TUint16) (((b & 0xf8) << 8) | ((g & 0xfc) << 3) | ((r & 0xf8) >> 3));
					}
				else
					{
					*dest32++ = (0xff000000 | (r << 16) | (g << 8) | b);
					}

				y = (source[1] - 16);

				r = ((298 * y + 409 * u) / 256);
				g = ((298 * y - 100 * v - 208 * u) / 256);
				b = ((298 * y + 516 * v) / 256);

				CLIP(r);
				CLIP(g);
				CLIP(b);

				if (iBitsPerPixel == 16)
					{
					*dest16++ = (TUint16) (((b & 0xf8) << 8) | ((g & 0xfc) << 3) | ((r & 0xf8) >> 3));
					}
				else
					{
					*dest32++ = (0xff000000 | (r << 16) | (g << 8) | b);
					}

				source += 4;
				}

			source += sourceModulo;
			dest16 += destModulo;
			dest32 += destModulo;
			}
		}
	else
		{
		retVal = KErrNotSupported;
		}

	return retVal;
	}

/**
Appends a string representing a pixel format UID onto a descriptor.
@param aBuffer		Reference to the descriptor into which to append the string.  It is up to the
					caller to ensure that this is large enough.
@param aPixelFormat	UID of the pixel format to be converted into a string.
*/
void AppendPixelFormat(TDes& aBuffer, TUidPixelFormat aPixelFormat)
	{
	if (aPixelFormat == EUidPixelFormatRGB_565)
		aBuffer.Append(KPixelFormatRGB_565);
	else if (aPixelFormat == EUidPixelFormatYUV_422Interleaved)
		aBuffer.Append(KPixelFormatYUV_422Interleaved);
	else if (aPixelFormat == EUidPixelFormatSpeedTaggedJPEG)
		aBuffer.Append(KPixelFormatSpeedTaggedJPEG);
	else if (aPixelFormat == EUidPixelFormatJPEG)
		aBuffer.Append(KPixelFormatJPEG);
	else
		aBuffer.Append(KPixelFormatUnknown);
	}

void PrintCamModes(TCameraCapsV02* aCaps,RTest& aTest)
	{
	TBuf<80> buf;

	// Display the supported capture modes
	buf.Zero();
	buf.Append(KCaptureModeCapsTitle);
	if (aCaps->iNumImagePixelFormats)
		buf.Append(KCaptureModeImage);
	if (aCaps->iNumVideoPixelFormats)
		buf.Append(KCaptureModeVideo);
	if (aCaps->iNumViewFinderPixelFormats)
		buf.Append(KCaptureModeViewFinder);
	buf.Append(_L("\r\n"));
	aTest.Printf(buf);

	// Display the supported video pixel formats
	TUint i;
	SDevCamPixelFormat* pixelFormat;
	if (aCaps->iNumImagePixelFormats)
		{
		buf.Zero();
		buf.Append(KPixelFormatCapsTitle);
		buf.Append(KCaptureModeImage);
		pixelFormat = (SDevCamPixelFormat*) (aCaps + 1);
		for (i = 0; i < aCaps->iNumImagePixelFormats; i++)
			{
			AppendPixelFormat(buf, pixelFormat->iPixelFormat);
			pixelFormat++;
			}
		buf.Append(_L("\r\n"));
		aTest.Printf(buf);
		}

	if (aCaps->iNumVideoPixelFormats)
		{
		buf.Zero();
		buf.Append(KPixelFormatCapsTitle);
		buf.Append(KCaptureModeVideo);
		pixelFormat = (SDevCamPixelFormat*) (aCaps + 1);
		for (i = aCaps->iNumImagePixelFormats; i < (aCaps->iNumImagePixelFormats + aCaps->iNumVideoPixelFormats); i++)
			{
			AppendPixelFormat(buf, pixelFormat->iPixelFormat);
			pixelFormat++;
			}
		buf.Append(_L("\r\n"));
		aTest.Printf(buf);
		}

	if (aCaps->iNumViewFinderPixelFormats)
		{
		buf.Zero();
		buf.Append(KPixelFormatCapsTitle);
		buf.Append(KCaptureModeViewFinder);
		pixelFormat = (SDevCamPixelFormat*) (aCaps + 1);
		i = aCaps->iNumImagePixelFormats + aCaps->iNumImagePixelFormats + 1;
		for (i = aCaps->iNumImagePixelFormats + aCaps->iNumVideoPixelFormats; i < (aCaps->iNumImagePixelFormats + aCaps->iNumVideoPixelFormats + aCaps->iNumViewFinderPixelFormats); i++)
			{
			AppendPixelFormat(buf, pixelFormat->iPixelFormat);
			pixelFormat++;
			}
		buf.Append(_L("\r\n"));
		aTest.Printf(buf);
		}
	}

void PrintCamConf(TCameraConfigV02& aConf,RTest& aTest)
	{
	TBuf<80> buf;

	// Display the current frame size
	buf.Zero();
	buf.Append(KFrameSizeConfTitle);
	buf.AppendFormat(KFrameSize, aConf.iFrameSize.iWidth, aConf.iFrameSize.iHeight);
	buf.Append(_L("\r\n"));
	aTest.Printf(buf);

	// Display the current pixel format
	buf.Zero();
	buf.Append(KPixelFormatConfTitle);
	AppendPixelFormat(buf, aConf.iPixelFormat.iPixelFormat);
	buf.Append(_L("\r\n"));
	aTest.Printf(buf);

	// Display the current frame rate
	buf.Zero();
	buf.Format(_L("Current frame rate  : %d fps\r\n"),aConf.iFrameRate);
	aTest.Printf(buf);
	}

void PrintBufferConf(TMmSharedChunkBufConfig& aBufConf,RTest& aTest)
	{
	TBuf<80> buf(0);

	SBufSpecList* tempSpec = aBufConf.iSpec;

	// Display the buffer configuration
	buf.Format(_L("Buffer Config       : NumBufs:%d Size:%xH\r\n"),aBufConf.iNumBuffers,aBufConf.iBufferSizeInBytes);
	aTest.Printf(buf);
	if (aBufConf.iFlags & KScFlagBufOffsetListInUse)
		{
		buf.Format(_L(" Offsets[%08xH,%08xH,%08xH,%08xH]\r\n"),tempSpec[0].iBufferOffset,tempSpec[1].iBufferOffset,tempSpec[2].iBufferOffset,tempSpec[3].iBufferOffset);
		aTest.Printf(buf);
		buf.Format(_L(" Offsets[%08xH,%08xH,%08xH,%08xH]\r\n"),tempSpec[4].iBufferOffset,tempSpec[5].iBufferOffset,tempSpec[6].iBufferOffset,tempSpec[7].iBufferOffset);
		aTest.Printf(buf);
		}
	}
