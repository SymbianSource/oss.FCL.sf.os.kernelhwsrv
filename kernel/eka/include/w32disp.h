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
// e32\include\w32disp.h
// 
//

#ifndef __W32DISP_H__
#define __W32DISP_H__
#include "d32video.h"

/**
 * Screen Driver container class
 */
class CScreenDriver : public CBase
	{
public:
	IMPORT_C static CScreenDriver *New();
	virtual void Init(TSize &aScreenSize,TSize &aFontSize) =0;
	virtual void Blit(const TText *aBuffer,TInt aLength,const TPoint &aPosition) =0;
	virtual TBool ScrollUp(const TRect& aRect) =0;
	virtual void Clear(const TRect& aRect) =0;
	
	virtual void SetPixel(const TPoint& aPoint,TUint8 aColour)=0;
	virtual TInt GetPixel(const TPoint& aPoint)=0;
	virtual void SetWord(const TPoint& aPoint,TInt aWord)=0;
	virtual TInt GetWord(const TPoint& aPoint)=0;
	virtual void SetLine(const TPoint& aPoint,const TPixelLine& aPixelLine)=0;
	virtual void GetLine(const TPoint& aPoint,TPixelLine& aPixelLine)=0;

	virtual TInt SetMode(TVideoMode aMode)=0;

	virtual void SetPaletteEntry(TColorIndex anIndex,TUint8 aRed,TUint8 aGreen,TUint8 aBlue)=0;
	virtual void GetPaletteEntry(TColorIndex anIndex,TUint8 &aRed,TUint8 &aGreen,TUint8 &aBlue)=0;
	virtual void SetForegroundColor(TColorIndex anIndex)=0;
	virtual void SetBackgroundColor(TColorIndex anIndex)=0;
	virtual void GetAttributeColors(TColorIndex* anArray)=0;
	};
#endif
