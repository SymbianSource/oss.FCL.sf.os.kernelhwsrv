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
// e32test/multimedia/t_camera_bitmap.h
// This is a basic Windows bitmap file writer, that can be used for converting YUV422 data into
// RGB format and dumping it to a Windows .bmp file, for manual examination.
// 
//

#if !defined(__T_CAMERA_BITMAP_H__)
#define __T_CAMERA_BITMAP_H__

#include <d32camerasc.h>
#include <f32file.h>

class RBitmap
	{
public:

	TInt WriteBMP(const TDesC& aFileName, TUint8* aBuffer, SDevCamPixelFormat aPixelFormat, TInt aWidth, TInt aHeight);
	TInt WriteBuffer(const TDesC& aFileName, TUint8* aBuffer, TInt aSize);

private:

	void RGBToRGB(TUint8* aDest, TUint8* aSource, TInt aWidth, TInt aHeight);
	void YUVToRGB(TUint8* aDest, TUint8* aSource, TInt aWidth, TInt aHeight);
	void PutLong(TUint8* aBuffer, TInt aLong);
	void PutShort(TUint8* aBuffer, TInt16 aShort);
	TInt WriteHeader(RFile& aFile, TInt aWidth, TInt aHeight);
	TInt WriteBitmapData(RFile& aFile, TUint8* aBuffer, SDevCamPixelFormat aPixelFormat, TInt aWidth, TInt aHeight);
	};

#endif // ! __T_CAMERA_BITMAP_H__
