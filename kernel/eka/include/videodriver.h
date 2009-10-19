// Copyright (c) 2001-2009 Nokia Corporation and/or its subsidiary(-ies).
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
//

/**
 @file
 @publishedPartner
 @released
*/

#ifndef __VIDEODRIVER_H__
#define __VIDEODRIVER_H__

#include <e32cmn.h>

struct SRectOpInfo
	{
	TInt	iX;
	TInt	iY;
	TInt	iWidth;
	TInt	iHeight;
	TInt	iSrcX;
	TInt	iSrcY;
	TUint32	iColor;
	};


/**
Encapsulates information about the screen display mode.

An object of this type is passed, via a TPckgBuf, to the HAL handler
that deals with the HAL group,function-id pair:
- EHalGroupDisplay, EDisplayHalCurrentModeInfo;
- EHalGroupDisplay, EDisplayHalSpecifiedModeInfo;

@see EDisplayHalCurrentModeInfo
@see EDisplayHalSpecifiedModeInfo
@see TDisplayHalFunction
@see EHalGroupDisplay
@see TPckgBuf
*/
class TVideoInfoV01
	{
public:
	TSize	iSizeInPixels;				/**< The visible width/height of the display in pixels. */
	TSize	iSizeInTwips;				/**< The visible width/height of the display in twips. */
	TBool	iIsMono;					/**< True if display is monochrome; false otherwise. */
	TBool	iIsPalettized;				/**< True if display is palettized (in current display mode); false otherwise. */
	TInt	iBitsPerPixel;				/**< The number of bits in one pixel. */
	TInt	iVideoAddress;				/**< The virtual address of screen memory. */
	TInt	iOffsetToFirstPixel;		/**< Number of bytes from iVideoAddress for the first displayed pixel. */
	TInt	iOffsetBetweenLines;		/**< Number of bytes between start of one line and start of next. */
	TBool	iIsPixelOrderRGB;			/**< The orientation of sub pixels on the screen; True if RBG, False if BGR. */
	TBool	iIsPixelOrderLandscape;		/**< True if display pixels are landscape. */
	TInt	iDisplayMode;				/**< The current display mode. */
	};


/**
Package buffer for a TVideoInfoV01 object.

@see TVideoInfoV01
*/
typedef TPckgBuf<TVideoInfoV01> TVideoInfoV01Buf;


#endif
