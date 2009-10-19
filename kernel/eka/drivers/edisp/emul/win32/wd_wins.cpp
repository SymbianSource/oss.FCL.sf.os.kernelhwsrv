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
// e32\drivers\edisp\emul\win32\wd_wins.cpp
// 
//

#include <e32cmn.h>
#include <e32cmn_private.h>
#include <emulator.h>
#define WIN32_LEAN_AND_MEAN
#pragma warning( disable : 4201 ) // nonstandard extension used : nameless struct/union
#include <windows.h>
#pragma warning( default : 4201 ) // nonstandard extension used : nameless struct/union
#include "ws_std.h"

// Driver for WINS platform using windows stock font

#define FONTWIDTH 8
#define FONTHEIGHT 12

struct rgbValues{
	TUint8 iRed;
	TUint8 iGreen;
	TUint8 iBlue;
	};

const TUint8 KMonoRed0=50;
const TUint8 KMonoGreen0=100;
const TUint8 KMonoBlue0=200;
const TUint8 KMonoRed1=250;
const TUint8 KMonoGreen1=250;
const TUint8 KMonoBlue1=20;

const TColorIndex KMonoAttributes[8]=
    {
    1,      // Normal foreground
    0,      // Normal background
    1,      // Bold foreground
    0,      // Bold background
    0,      // Inverse foreground
    1,      // Inverse background
    0,      // Highlight foreground
    1       // Highlight background
    };

const TColorIndex KColor256Attributes[8]=
    {
    38,      // Normal foreground
    33,      // Normal background
    36,      // Bold foreground
    38,      // Bold background
    33,      // Inverse foreground
    38,      // Inverse background
    38,      // Highlight foreground
    36       // Highlight background
    };

class CScreenDriverWins : public CScreenDriver
	{
	friend class CScreenDriver;
public:
	CScreenDriverWins();
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

	static TBool IsHankaku(const TText aCode);
	virtual TPoint GraphicsPosition(const TPoint& aPosition);
	virtual void ScreenBufferScrollUp(const TRect& aRect);
	virtual void ScreenBufferClear(const TRect& aRect);
private:
    TText* iScreenBuffer;

private:
	HFONT iFont;
    HDC iDc;
	TSize iTextScreenSize;
	TScreenInfoV01 iScreenInfo;
	TVideoMode iMode;
	rgbValues* iPalette; 
	};


TPoint CScreenDriverWins::GraphicsPosition(const TPoint& aPosition)
	{
    TPoint pos(0,aPosition.iY*FONTHEIGHT);
    TText* pS=iScreenBuffer+(iTextScreenSize.iWidth*aPosition.iY);
    TText* pE=pS+aPosition.iX;
	for (TText* pT=pS;pT<pE;++pT)
		pos.iX+=IsHankaku(*pT) ? FONTWIDTH : FONTWIDTH*2;
    return(pos);
	}

TBool CScreenDriverWins::IsHankaku(const TText aCode)
	{
	if (aCode >= 0xff61 && aCode <= 0xff9f) 
		return ETrue;	// HANKAKU KATAKANA code
	if (aCode >= 0x2550 && aCode <= 0x259f)
		return ETrue;	// HANKAKU Graphics code
	if (aCode < 0x100)
        return ETrue;	// Alphanumeric codes means HANKAKU
	return EFalse;
	}

void CScreenDriverWins::ScreenBufferScrollUp(const TRect& aRect)
	{
	TText* src=&iScreenBuffer[(aRect.iTl.iY+1)*iTextScreenSize.iWidth+aRect.iTl.iX];
	TText* dest=&iScreenBuffer[aRect.iTl.iY*iTextScreenSize.iWidth+aRect.iTl.iX];
	for (TInt j=0;j<aRect.iBr.iY-aRect.iTl.iY;++j)
        {
		TInt r=j*iTextScreenSize.iWidth;
		for (TInt i=0;i<aRect.iBr.iX-aRect.iTl.iX+1;++i)
            {
			TInt p = r+i;
			dest[p] = src[p];
			}
		}
	}

void CScreenDriverWins::ScreenBufferClear(const TRect& aRect)
	{
	TText* dest=&iScreenBuffer[aRect.iTl.iY*iTextScreenSize.iWidth+aRect.iTl.iX];
	for (TInt j=0;j<aRect.iBr.iY-aRect.iTl.iY+1;++j)
        {
		TInt r=j*iTextScreenSize.iWidth;
		for (TInt i=0;i<aRect.iBr.iX-aRect.iTl.iX+1;++i)
			dest[r+i] = 0x0020;
		}
	}

CScreenDriverWins::CScreenDriverWins()
//
// Constructor
//
	{

	TPckg<TScreenInfoV01> sI(iScreenInfo);
	UserSvr::ScreenInfo(sI);
	iPalette=(rgbValues *)User::Alloc(256*sizeof(rgbValues));
	}

EXPORT_C CScreenDriver *CScreenDriver::New()
//
// Return the actual screen driver.
//
	{

	CScreenDriverWins *pS=new CScreenDriverWins;
    if (!pS)
    	return(NULL);
	if (!pS->iScreenInfo.iWindowHandleValid)
		{
		delete pS;
    	return(NULL);
		}
    pS->iTextScreenSize=pS->iScreenInfo.iScreenSize;
	pS->iTextScreenSize.iWidth/=FONTWIDTH; // Convert from GDI units to character positions
	pS->iTextScreenSize.iHeight/=FONTHEIGHT;
    pS->iScreenBuffer=new TText[pS->iTextScreenSize.iWidth*pS->iTextScreenSize.iHeight];
    if (!pS->iScreenBuffer)
		{
		delete pS;
        return(NULL);
		}
	
	__LOCK_HOST;
	HWND win=(HWND)pS->iScreenInfo.iWindowHandle;
	pS->iDc=GetDC(win);
	DWORD fontCharacterSet;
	LANGID language;
	fontCharacterSet=ANSI_CHARSET;
	language=(LANGID)PRIMARYLANGID(GetSystemDefaultLangID());
	if (language==LANG_JAPANESE) fontCharacterSet=SHIFTJIS_CHARSET;
	pS->iFont=CreateFontA(FONTHEIGHT+1,      // logical font height
						FONTWIDTH+1,        // logical average character width
						0,                  // angle of escapement
						0,                  // base-line orientation angle
						FW_DONTCARE,        // font weight
						0,                  // italic attribute
						0,                  // underline attribute
						0,                  // strikeout attribute
				        fontCharacterSet,	// character set identifier
						OUT_RASTER_PRECIS,  // output precision
						CLIP_CHARACTER_PRECIS, // clipping precision
						DEFAULT_QUALITY,    // output quality
						FIXED_PITCH,        // pitch and family
						"Terminal"          // typeface name string
				        );
	if (!pS->iFont)
		{
		delete pS->iScreenBuffer;
		delete pS;
		return(NULL);
		}

	// On some Windows9x PCs, the font we assume under NT isn't always present,
	// but it's important we get a font with height 12.  Therefore increase the
	// height setting and try again.  This seems always to do the trick.
	TEXTMETRIC textMetrics;
	HFONT oldFont=REINTERPRET_CAST(HFONT,SelectObject(pS->iDc,pS->iFont));
	TBool tryNewFont=FALSE;
	if (GetTextMetrics(pS->iDc, &textMetrics) && (textMetrics.tmHeight<12))
		tryNewFont=TRUE;
	SelectObject(pS->iDc, oldFont);
	if (tryNewFont)
		{
		HFONT newFont=CreateFontA(FONTHEIGHT+2,
							FONTWIDTH+1,
							0,
							0,
							FW_DONTCARE,
							0,
							0,
							0,
							fontCharacterSet,
							OUT_RASTER_PRECIS,
							CLIP_CHARACTER_PRECIS,
							DEFAULT_QUALITY,
							FIXED_PITCH,
							"Terminal"
							);
		if (newFont)
			pS->iFont=newFont;
		}

	return(pS);
	}

void CScreenDriverWins::Init(TSize &aScreenSize,TSize &aFontSize)
//
// Report screen information
//
	{

	aFontSize=TSize(FONTWIDTH,FONTHEIGHT);
	aScreenSize=iTextScreenSize;
	}

TInt CScreenDriverWins::SetMode(TVideoMode aMode)
//
// Set the screen mode
//
	{

//  Reset the palette

    if (aMode==EColor256)
        {
        TInt x, y, z, t;

    	for(t=0;t<16;t++)
    		{
    		iPalette[t].iRed=(TUint8)(t*8);
            iPalette[t].iGreen=(TUint8)(t*8);
            iPalette[t].iBlue=(TUint8)(t*8);
    		}
    	for(t=16;t<32;t++)
    		{
    		iPalette[t].iRed=(TUint8)(t*8+7);
            iPalette[t].iGreen=(TUint8)(t*8+7);
            iPalette[t].iBlue=(TUint8)(t*8+7);
    		}
    	for(x=0;x<2;x++)
    		for(y=0;y<2;y++)
    			for(z=0;z<2;z++)
    				{
    				iPalette[t].iRed=(TUint8)(x*255);
    				iPalette[t].iGreen=(TUint8)(y*255);
    				iPalette[t].iBlue=(TUint8)(z*255);
                    t++;
    				}
    	for(x=0;x<6;x++)
    		for(y=0;y<6;y++)
    			for(z=0;z<6;z++)
    				{
    				iPalette[t].iRed=(TUint8)(x*51);
    				iPalette[t].iGreen=(TUint8)(y*51);
    				iPalette[t].iBlue=(TUint8)(z*51);
                    t++;
    				}
        iMode=aMode;
        return(KErrNone);
        }
    if (aMode==EMono)
        {
        iPalette[0].iRed=KMonoRed0;
        iPalette[0].iGreen=KMonoGreen0;
        iPalette[0].iBlue=KMonoBlue0;
        iPalette[1].iRed=KMonoRed1;
        iPalette[1].iGreen=KMonoGreen1;
        iPalette[1].iBlue=KMonoBlue1;
        iMode=aMode;
        return(KErrNone);
        }
	return(KErrNotSupported);
    }

void CScreenDriverWins::Blit(const TText *aBuffer, TInt aLength, const TPoint &aPosition)
//
// Write contiguous block of characters to some segment of a line on the display
//
	{

	if (iDc)
	    {

        // Create a font of the appropriate metrics.  The character
        // set selection depends on the context.  The text console
        // was originally designed with the code page 850 line drawing
        // characters, so in the 8-bit version we continue to select
        // this, until we are in a position to supply a specific font
        // with the SDK.  In the 16-bit version, we can use an ANSI
        // font and the proper line drawing characters defined in Unicode.
        // For a Japanese environment, an attempt is made to select a
        // font that supports the shift-JIS character set.

		__LOCK_HOST;
		HFONT oldFont=REINTERPRET_CAST(HFONT,SelectObject(iDc,iFont));
		if (oldFont)
			{
			TPtrC buf(aBuffer,aLength);
			TBuf<0x100> b;
			b.Copy(buf);
			TText* code =(TText*)b.Ptr();
            Mem::Copy(iScreenBuffer+((aPosition.iY*iTextScreenSize.iWidth)+aPosition.iX),code,aLength*2);
			TPoint ap=GraphicsPosition(aPosition);
			for (TInt i=0;i<aLength;++i)
				{
				TInt fontWidth = IsHankaku(*code) ? FONTWIDTH : FONTWIDTH*2;
				RECT rect;
				SetRect(&rect, ap.iX, ap.iY, ap.iX + fontWidth, ap.iY + FONTHEIGHT);
				// The following function is better than TextOutW because it will fill in the gaps
				// if we get a font smaller than the appropriate size.
				ExtTextOutW(iDc, ap.iX, ap.iY, ETO_OPAQUE, &rect, (const unsigned short *)code, 1, NULL);
				ap.iX+=fontWidth;
				code++;
				}
			SelectObject(iDc,oldFont);
			}
		GdiFlush();
		}
	}

TBool CScreenDriverWins::ScrollUp(const TRect& aRect)
//
// Scroll a rectangle of the screen up one line, the bottom line is not updated, return ETrue on sucess
//
	{

	ScreenBufferScrollUp(aRect);
	// Create MSVC RECT from E32 TRect
	__LOCK_HOST;
	RECT rect;
	rect.left=(aRect.iTl.iX)*FONTWIDTH;
	rect.top=(aRect.iTl.iY+1)*FONTHEIGHT;
	rect.right=(aRect.iBr.iX)*FONTWIDTH;
	rect.bottom=(aRect.iBr.iY)*FONTHEIGHT;
	RECT updateRect;
	ScrollWindowEx((HWND)iScreenInfo.iWindowHandle,0,-FONTHEIGHT,&rect,NULL,NULL,&updateRect,0);
	GdiFlush();
	if (updateRect.bottom==0 || (rect.bottom==updateRect.bottom && updateRect.bottom==updateRect.top+FONTHEIGHT))
		return ETrue;
	return EFalse;
	}

void CScreenDriverWins::Clear(const TRect& aRect)
//
// Clear a rectangle of the screen
//
	{

	ScreenBufferClear(aRect);
	// Create MSVC RECT from E32 TRect
	__LOCK_HOST;
	RECT rect;
	rect.left=(aRect.iTl.iX)*FONTWIDTH;
	rect.top=(aRect.iTl.iY)*FONTHEIGHT;
	rect.right=(aRect.iBr.iX)*FONTWIDTH;
	rect.bottom=(aRect.iBr.iY)*FONTHEIGHT;
	FillRect(GetDC((HWND)iScreenInfo.iWindowHandle),&rect,NULL); 
	GdiFlush();
	}

void CScreenDriverWins::SetPaletteEntry(TColorIndex anIndex,TUint8 aRed,TUint8 aGreen,TUint8 aBlue)
	{

	if(iMode==EColor256)
		{
		iPalette[anIndex].iRed=aRed;iPalette[anIndex].iGreen=aGreen;iPalette[anIndex].iBlue=aBlue;
		}
	}

void CScreenDriverWins::GetPaletteEntry(TColorIndex anIndex,TUint8 &aRed,TUint8 &aGreen,TUint8 &aBlue)
	{
	
	aRed=iPalette[anIndex].iRed;aGreen=iPalette[anIndex].iGreen;aBlue=iPalette[anIndex].iBlue;
	}

void CScreenDriverWins::SetForegroundColor(TColorIndex anIndex)
	{

	__LOCK_HOST;
    TInt index=anIndex;
	index=Min(index,(1<<iMode)-1);
	SetTextColor(iDc,RGB(iPalette[index].iRed,iPalette[index].iGreen,iPalette[index].iBlue));
	GdiFlush();
	}

void CScreenDriverWins::SetBackgroundColor(TColorIndex anIndex)
	{
	
	__LOCK_HOST;
    TInt index=anIndex;
	index=Min(index,(1<<iMode)-1);
	SetBkColor(iDc,RGB(iPalette[index].iRed,iPalette[index].iGreen,iPalette[index].iBlue));
	GdiFlush();
	}

void CScreenDriverWins::GetAttributeColors(TColorIndex* anArray)
//
//
//
	{
	if(iMode==EMono)
        Mem::Copy(anArray,KMonoAttributes,sizeof(KMonoAttributes));
	else if(iMode==EColor256)
        Mem::Copy(anArray,KColor256Attributes,sizeof(KColor256Attributes));
	}

void CScreenDriverWins::SetPixel(const TPoint& /*aPoint*/,TUint8 /*aColour*/)
	{
	}

TInt CScreenDriverWins::GetPixel(const TPoint& /*aPoint*/)
	{
	return 0;
	}

void CScreenDriverWins::SetWord(const TPoint& /*aPoint*/,TInt /*aWord*/)
	{
	}

TInt CScreenDriverWins::GetWord(const TPoint& /*aPoint*/)
	{
	return 0;
	}

void CScreenDriverWins::SetLine(const TPoint& /*aPoint*/,const TPixelLine& /*aPixelLine*/)
	{
	}

void CScreenDriverWins::GetLine(const TPoint& /*aPoint*/,TPixelLine& /*aPixelLine*/)
	{
	}

