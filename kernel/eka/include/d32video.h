// Copyright (c) 1995-2009 Nokia Corporation and/or its subsidiary(-ies).
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
// e32\include\d32video.h
// 
//

#ifndef __D32VIDEO_H__
#define __D32VIDEO_H__
#include <e32std.h>
#include <e32ver.h>
#include <e32twin.h>
//
enum TColour {EWhite=0xf, EWhite1=0xe, EWhite2=0xd, EWhite3=0xc, EWhite4=0xb,
			  ELightGrey=0xa, ELightGrey1=9, ELightGrey2=8, ELightGrey3=7, ELightGrey4=6,
			  EDarkGrey=5, EDarkGrey1=4, EDarkGrey2=3, EDarkGrey3=2, EDarkGrey4=1,
			  EBlack=0
			  };
//
const TUint KCapsHues2=0x00000001;
const TUint KCapsHues4=0x00000002;
const TUint KCapsHues16=0x00000004;
const TUint KCapsWidth640=0x01;
const TUint KCapsHeight240=0x01;
//
class TVideoCapsV01
	{
public:
	TInt iNumHues;
	TInt iWidth;
	TInt iHeight;
	};
typedef TPckgBuf<TVideoCapsV01> TVideoCaps;
class TCapsDevVideoV01
	{
public:
	TVersion version;
	};
//
class TBlitInfo
	{
public:
	inline TBlitInfo(const TText* aBuffer,const TColour aColour, const TInt aLength,const TInt aX, const TInt aY);
	inline TBlitInfo();
public:
	const TText* iBuffer;
	const TColour iColour;
	TInt iLength;
	const TInt iX;
	const TInt iY;
private:
	TBlitInfo& operator =(TBlitInfo & aBlitInfo);
	};

class TPixelInfo
	{
public:
	inline TPixelInfo(const TInt aX, const TInt aY, const TColour aColour);
	inline TPixelInfo();
public:
	const TColour iColour;
	const TInt iX;
	const TInt iY;
private:
	TPixelInfo& operator =(TPixelInfo & aPixelInfo);
	};

struct TPixelLine
	{
	TDes8* iPixels;
	TInt iNoOfPixels;
	};

class RDeviceVideo : public RBusLogicalChannel
	{
public:
	enum TVer {EMajorVersionNumber=1,EMinorVersionNumber=0,EBuildVersionNumber=KE32BuildVersionNumber};
	enum TControl {EControlStart,EControlMode,EControlSetMode,EControlCaps,EControlSetPaletteEntry,
		EControlBlit,EControlSetPixel,EControlGetPixel,EControlSetWord,EControlGetWord,
		EControlSetLine,EControlGetLine,EControlScrollUp,EControlClear};
public:
	inline TInt Open();
	inline void Start();
	inline TVersion VersionRequired() const;
	inline void Caps(TDes8 &aCaps);
	inline void SetPaletteEntry(TInt aIndex,TUint aEntry);
	inline void Blit(TDes8 &aBlitInfo);
	inline void SetPixel(TDes8 &aPixelInfo);
	inline TInt SetMode(TVideoMode aMode);
	inline void Mode(TVideoMode *aMode);
	inline void ScrollUp(const TRect& aRect);
	inline void Clear(const TRect& aRect);
	inline TInt GetPixel(const TPoint& aPoint);
	inline void SetWord(const TPoint& aPoint,TInt& aWord);
	inline TInt GetWord(const TPoint &aPoint);
	inline void SetLine(const TPoint& aPoint,const TPixelLine& aPixelLine);
	inline void GetLine(const TPoint& aPoint,TPixelLine& aPixelLine);
	};
#include "d32video.inl"
#endif

