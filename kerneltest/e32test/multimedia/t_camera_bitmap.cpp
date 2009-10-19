// Copyright (c) 2008-2009 Nokia Corporation and/or its subsidiary(-ies).
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
// e32test/multimedia/t_camera_bitmap.cpp
// This is a basic Windows bitmap file writer, that can be used for converting YUV422 data into
// RGB format and dumping it to a Windows .bmp file, for manual examination.
// 
//

#include <e32test.h>
#include <f32file.h>
#include "t_camera_bitmap.h"

#define CLIP(a) if (a < 0) a = 0; else if (a > 255) a = 255;

/**
Converts a RGB565 buffer into 24 bit RGB format in a second buffer.
@param	aDest		Pointer to the buffer into which to place the 24 bit RGB data
@param	aSource		Pointer to the buffer containing the RGB565 data to be converted
@param	aWidth		The width of the data in the buffer in pixels
@param	aHeight		The height of the data in the buffer in pixels
*/
void RBitmap::RGBToRGB(TUint8* aDest, TUint8* aSource, TInt aWidth, TInt aHeight)
	{
	TUint16* source = (TUint16*) aSource;
	TUint16 pixel;

	for (TInt y = 0; y < aHeight; ++y)
		{
		for (TInt x = 0; x < aWidth; ++x)
			{
			pixel = *source++;
			*aDest++ = (TUint8) ((pixel & 0xf800) >> 8);
			*aDest++ = (TUint8) ((pixel & 0x07e0) >> 3);
			*aDest++ = (TUint8) ((pixel & 0x001f) << 3);
			}
		}
	}

/**
Converts a YUV422 buffer into 24 bit RGB format in a second buffer.
@param	aDest		Pointer to the buffer into which to place the 24 bit RGB data
@param	aSource		Pointer to the buffer containing the YUV422 data to be converted
@param	aWidth		The width of the data in the buffer in pixels
@param	aHeight		The height of the data in the buffer in pixels
*/
void RBitmap::YUVToRGB(TUint8* aDest, TUint8* aSource, TInt aWidth, TInt aHeight)
	{
	TInt y, u, v, r, g, b;

	aDest += ((aWidth * 3) * (aHeight - 1));

	for (TInt l = 0; l < aHeight; ++l)
		{
		for (TInt x = 0; x < (aWidth / 2); ++x)
			{
			u = (aSource[0] - 128);
			v = (aSource[2] - 128);
			y = (aSource[3] - 16);

			r = ((298 * y + 409 * u) / 256);
			g = ((298 * y - 100 * v - 208 * u) / 256);
			b = ((298 * y + 516 * v) / 256);

			CLIP(r);
			CLIP(g);
			CLIP(b);

			*aDest++ = (TUint8) r;
			*aDest++ = (TUint8) g;
			*aDest++ = (TUint8) b;

			y = (aSource[1] - 16);

			r = ((298 * y + 409 * u) / 256);
			g = ((298 * y - 100 * v - 208 * u) / 256);
			b = ((298 * y + 516 * v) / 256);

			CLIP(r);
			CLIP(g);
			CLIP(b);

			*aDest++ = (TUint8) r;
			*aDest++ = (TUint8) g;
			*aDest++ = (TUint8) b;
			aSource += 4;
			}

		aDest -= (aWidth * 3 * 2);
		}
	}

/**
Converts a 32 bit long from whatever the host format is into little endian format in a user supplied buffer.
@param	aBuffer		Pointer to the buffer into which to write the value
@param	aLong		The value to be written
*/
void RBitmap::PutLong(TUint8* aBuffer, TInt aLong)
	{
	*aBuffer++ = (TUint8) (aLong & 0xff);
	*aBuffer++ = (TUint8) ((aLong >> 8) & 0xff);
	*aBuffer++ = (TUint8) ((aLong >> 16) & 0xff);
	*aBuffer++ = (TUint8) ((aLong >> 24) & 0xff);
	}

/**
Converts a 16 bit short from whatever the host format is into little endian format in a user supplied buffer.
@param	aBuffer		Pointer to the buffer into which to write the value
@param	aShort		The value to be written
*/
void RBitmap::PutShort(TUint8* aBuffer, TInt16 aShort)
	{
	*aBuffer++ = (TUint8) (aShort & 0xff);
	*aBuffer++ = (TUint8) ((aShort >> 8) & 0xff);
	}

/**
Writes a standard Windows .bmp header to a file, including the standard .bmp file header, followed
by a V3 DIB header.
@param	aFile		A reference to the file to which to write the header
@param	aWidth		The width of the bitmap in pixels
@param	aHeight		The height of the bitmap in pixels
@return	KErrNone if write was successful, otherwise one of the other system wide error codes
*/
TInt RBitmap::WriteHeader(RFile& aFile, TInt aWidth, TInt aHeight)
	{
	TBuf8<14> header(14);
	TUint8* buffer = (TUint8*) header.Ptr();

	header.Fill(0);

	header[0] = 'B';
	header[1] = 'M';
	PutLong((buffer + 2), (14 + 40 + (aWidth * aHeight * 3)));
	PutLong((buffer + 10), (14 + 40));

	TInt r = aFile.Write(header);

	if (r == KErrNone)
		{
		TBuf8<40> bitmapInfoHeader(40);
		TUint8* buffer = (TUint8*) bitmapInfoHeader.Ptr();

		bitmapInfoHeader.Fill(0);

		PutLong(buffer, 40);
		PutLong((buffer + 4), aWidth);
		PutLong((buffer + 8), aHeight);
		PutShort((buffer + 12), 1);
		PutShort((buffer + 14), 24);
		PutLong((buffer + 20), (aWidth * aHeight * 3));

		r = aFile.Write(bitmapInfoHeader);
		}

	return r;
	}

/**
Converts a YUV422 or RGB565 buffer into 24 bit RGB format and writes it to a file.
@param	aFile			A reference to the file to which to write the data
@param	aBuffer			A pointer to the buffer containing the data to be converted and written
@param	aPixelFormat	UID specifying the format of the source data
@param	aWidth			The width of the data in the buffer in pixels
@param	aHeight			The height of the data in the buffer in pixels
@return	KErrNone if write was successful, otherwise one of the other system wide error codes
*/
TInt RBitmap::WriteBitmapData(RFile& aFile, TUint8* aBuffer, SDevCamPixelFormat aPixelFormat, TInt aWidth, TInt aHeight)
	{
	TInt length = (aWidth * aHeight * 3);
	TUint8* rgbBuffer = new TUint8[length];

	TInt r = KErrNone;

	if (rgbBuffer)
		{
		if (aPixelFormat.iPixelFormat == EUidPixelFormatYUV_422Interleaved)
			{
			YUVToRGB(rgbBuffer, aBuffer, aWidth, aHeight);
			}
		else if (aPixelFormat.iPixelFormat == EUidPixelFormatRGB_565)
			{
			RGBToRGB(rgbBuffer, aBuffer, aWidth, aHeight);
			}
		else
			{
			r = KErrNotSupported;
			}

		if (r == KErrNone)
			{
			TPtr8 buffer(rgbBuffer, length, length);
			r = aFile.Write(buffer);
			}

		delete [] rgbBuffer;
		}
	else
		{
		r = KErrNoMemory;
		}

	return r;
	}

/**
Converts a YUV422 or RGB565 buffer into 24 bit RGB format and writes it to a Windows .bmp file.
@param	aFileName		A reference to the fully qualified name of the file to write the .bmp file to
@param	aBuffer			A pointer to the buffer containing the data to be converted and written
@param	aPixelFormat	UID specifying the format of the source data
@param	aWidth			The width of the data in the buffer in pixels
@param	aHeight			The height of the data in the buffer in pixels
@return	KErrNone if write was successful, otherwise one of the other system wide error codes
*/
TInt RBitmap::WriteBMP(const TDesC& aFileName, TUint8* aBuffer, SDevCamPixelFormat aPixelFormat, TInt aWidth, TInt aHeight)
	{
	TInt r;
	RFile file;
	RFs fs;

	if ((r = fs.Connect()) == KErrNone)
		{
		if ((r = file.Replace(fs, aFileName, EFileWrite)) == KErrNone)
			{
			if ((r = WriteHeader(file, aWidth, aHeight)) == KErrNone)
				{
				r = WriteBitmapData(file, aBuffer, aPixelFormat, aWidth, aHeight);
				}

			file.Close();

			// If anything went wrong, delete the file so that we do not leave partial files that
			// might cause confusion
			if (r != KErrNone)
				{
				fs.Delete(aFileName);
				}
			}

		fs.Close();
		}

	return r;
	}

/**
Dumps a buffer straight to disk, without any kind of processing.
@param	aFileName		A reference to the fully qualified name of the file to write the file to
@param	aBuffer			A pointer to the buffer containing the data to be converted written
@param	aSize			The size of the buffer to be written, in bytes
@return	KErrNone if write was successful, otherwise one of the other system wide error codes
*/
TInt RBitmap::WriteBuffer(const TDesC& aFileName, TUint8* aBuffer, TInt aSize)
	{
	TInt r;
	RFile file;
	RFs fs;

	if ((r = fs.Connect()) == KErrNone)
		{
		if ((r = file.Replace(fs, aFileName, EFileWrite)) == KErrNone)
			{
			TPtrC8 buffer(aBuffer, aSize);

			r = file.Write(buffer);

			file.Close();

			// If anything went wrong, delete the file so that we do not leave partial files that
			// might cause confusion
			if (r != KErrNone)
				{
				fs.Delete(aFileName);
				}
			}

		fs.Close();
		}

	return r;
	}
