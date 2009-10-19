// Copyright (c) 1997-2009 Nokia Corporation and/or its subsidiary(-ies).
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
// e32\drivers\edisp\epoc\vga\vgatext.cpp
// Brutus display driver
// 
//

#include "ws_std.h"

const TUint KCharAttrib=0x17;		// white on blue

#if 0
#define __DEBUG_PRINT(a) RDebug::Print(a)
#define __DEBUG_PRINT2(a,b) RDebug::Print(a,b)
#else
#define __DEBUG_PRINT(a)
#define __DEBUG_PRINT2(a,b)
#endif

class CScreenDriverVgaText : public CScreenDriver
	{
public:
	CScreenDriverVgaText();
	virtual void Init(TSize &aScreenSize,TSize &aFontSize);
	virtual void Blit(const TText *aBuffer,TInt aLength,const TPoint &aPosition);
	virtual TBool ScrollUp(const TRect& aRect);
	virtual void Clear(const TRect& aRect);

	virtual void SetPixel(const TPoint& aPoint,TUint8 aColour);
	virtual TInt GetPixel(const TPoint& aPoint);
	virtual void SetWord(const TPoint& aPoint,TInt aWord);
	virtual TInt GetWord(const TPoint& aPoint);
	virtual void SetLine(const TPoint& aPoint,const TPixelLine& aPixelLine);
	virtual void GetLine(const TPoint& aPoint,TPixelLine& aPixelLine);

	virtual void SetPaletteEntry(TColorIndex anIndex,TUint8 aRed,TUint8 aGreen,TUint8 aBlue);
	virtual void GetPaletteEntry(TColorIndex anIndex,TUint8 &aRed,TUint8 &aGreen,TUint8 &aBlue);
	virtual void SetForegroundColor(TColorIndex anIndex);
	virtual void SetBackgroundColor(TColorIndex anIndex);
	virtual void GetAttributeColors(TColorIndex* anArray);
	virtual TInt SetMode(TVideoMode aMode);
public:
	TInt iMode;
	TUint16* iScreen;
	TInt iSizeX;
	TInt iSizeY;
	friend class CScreenDriver;
	};

CScreenDriverVgaText::CScreenDriverVgaText()
//
// Constructor
//
	{
	__DEBUG_PRINT(_L("CSD::Ctor"));
	iMode=EMono;
	}

CScreenDriver* NewScreenDriverVgaText()
//
// Return the actual screen driver.
//
	{

	__DEBUG_PRINT(_L("CSD::New"));

	CScreenDriverVgaText* s=new CScreenDriverVgaText;
	if (!s)
		return NULL;
	TScreenInfoV01 info;
	TPckg<TScreenInfoV01> infoPckg(info);
	UserSvr::ScreenInfo(infoPckg);
	if (!info.iScreenAddressValid)
		{
		delete s;
		return NULL;
		}
	s->iScreen = (TUint16*)(TUint32(info.iScreenAddress)+0x18000);
	s->iSizeX=80;
	s->iSizeY=25;
	return s;
	}

void CScreenDriverVgaText::Init(TSize& aScreenSize, TSize& aFontSize)
//
// Report screen information
//
	{

	__DEBUG_PRINT(_L("CSD::Init"));
	aFontSize=TSize(8,10);
	aScreenSize.iWidth=iSizeX;
	aScreenSize.iHeight=iSizeY;

	TInt i;
	TUint16* p=iScreen;
	for (i=0; i<iSizeX*iSizeY; ++i)
		*p++=KCharAttrib<<8;
	}

TInt CScreenDriverVgaText::SetMode(TVideoMode /*aMode*/)
//
// Set the screen mode
//
	{
	__DEBUG_PRINT(_L("CSD::SetMode"));
	return(KErrNotSupported);
	}

void CScreenDriverVgaText::Blit(const TText* aBuf, TInt aLen, const TPoint& aPos)
//
// Write contiguous block of characters to some segment of a line on the display
//
	{
	switch(iMode)
		{
		case EMono:
			{
			TUint16* pS=iScreen+aPos.iY*iSizeX+aPos.iX;
			TUint8* pSB=(TUint8*)pS;
			while(aLen--)
				{
				*pSB=(*aBuf++)&0xff;
				pSB+=2;
				}
			break;
			}
		default:
			break;
		}
	}

TBool CScreenDriverVgaText::ScrollUp(const TRect& aRect)
//
// Scroll a rectangle of the screen up one character, don't update bottom line
//
	{

	__DEBUG_PRINT(_L("CSD::ScrollUp"));
	TUint16* pT=iScreen+aRect.iTl.iY*iSizeX+aRect.iTl.iX;
	TUint16* pS=pT+iSizeX;
	TUint16* pE=iScreen+aRect.iBr.iY*iSizeX;
	TInt w=aRect.iBr.iX-aRect.iTl.iX;
	if (w>0)
		{
		for (; pS<pE; pT+=iSizeX, pS+=iSizeX)
			{
			Mem::Copy(pT,pS,w*2);
			}
		}
	return ETrue;
	}

void CScreenDriverVgaText::Clear(const TRect& aRect)
//
// Clear a rectangle of the screen, don't clear bottom line
//
	{

__DEBUG_PRINT(_L("CSD::Clear"));
	TUint16* pT=iScreen+aRect.iTl.iY*iSizeX+aRect.iTl.iX;
	TUint16* pE=iScreen+aRect.iBr.iY*iSizeX;
	TInt w=aRect.iBr.iX-aRect.iTl.iX;
	if (w>0)
		{
		for (; pT<pE; pT+=iSizeX)
			{
			TInt i;
			TUint16* pT2=pT;
			for (i=w; i; --i)
				*pT2++=KCharAttrib<<8;
			}
		}
	}

void CScreenDriverVgaText::SetPixel(const TPoint& /*aPoint*/, TUint8 /*aColour*/)
	{
	__DEBUG_PRINT(_L("CSD::SetPix"));
	}

TInt CScreenDriverVgaText::GetPixel(const TPoint& /*aPoint*/)
	{
	__DEBUG_PRINT(_L("CSD::GetPix"));
	return 0;
	}

void CScreenDriverVgaText::SetWord(const TPoint& /*aPoint*/, TInt /*aWord*/)
	{
	__DEBUG_PRINT(_L("CSD::SetWord"));
	}

TInt CScreenDriverVgaText::GetWord(const TPoint& /*aPoint*/)
	{
	__DEBUG_PRINT(_L("CSD::GetWord"));
	return 0;
	}

void CScreenDriverVgaText::SetLine(const TPoint& /*aPoint*/, const TPixelLine& /*aPixelLine*/)
	{
	__DEBUG_PRINT(_L("CSD::SetLine"));
	}

void CScreenDriverVgaText::GetLine(const TPoint& /*aPoint*/, TPixelLine& /*aPixelLine*/)
	{
	__DEBUG_PRINT(_L("CSD::GetLine"));
	}

void CScreenDriverVgaText::SetPaletteEntry(TColorIndex /*anIndex*/, TUint8 /*aRed*/, TUint8 /*aGreen*/, TUint8 /*aBlue*/)
	{
	}

void CScreenDriverVgaText::GetPaletteEntry(TColorIndex /*anIndex*/, TUint8& /*aRed*/, TUint8& /*aGreen*/, TUint8& /*aBlue*/)
	{
	}

void CScreenDriverVgaText::SetForegroundColor(TColorIndex /*anIndex*/)
	{
	}

void CScreenDriverVgaText::SetBackgroundColor(TColorIndex /*anIndex*/)
	{
	}

void CScreenDriverVgaText::GetAttributeColors(TColorIndex* /*anArray*/)
	{
	}

