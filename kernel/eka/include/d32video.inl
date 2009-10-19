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
// e32\include\d32video.inl
// 
//

_LIT(KRDeviceVideo,"Video");
inline TInt RDeviceVideo::Open()
	{return(DoCreate(KRDeviceVideo,VersionRequired(),KNullUnit,NULL,NULL));}
inline void RDeviceVideo::Start()
	{DoControl(EControlStart);}
inline TVersion RDeviceVideo::VersionRequired() const
	{return(TVersion(EMajorVersionNumber,EMinorVersionNumber,EBuildVersionNumber));}
inline void RDeviceVideo::Caps(TDes8 &aCaps)
	{DoControl(EControlCaps,&aCaps);}
inline void RDeviceVideo::Blit(TDes8 &aBlitInfo)
	{DoControl(EControlBlit,&aBlitInfo);}
inline void RDeviceVideo::SetPixel(TDes8 &aPixelInfo)
	{DoControl(EControlSetPixel,&aPixelInfo);}
inline void RDeviceVideo::Mode(TVideoMode *aMode)
	{DoControl(EControlMode,(TAny*)aMode);}
inline TInt RDeviceVideo::SetMode(TVideoMode aMode)
	{return(DoControl(EControlSetMode,(TAny*)aMode));}
inline void RDeviceVideo::SetPaletteEntry(TInt aIndex,TUint aEntry)
	{DoControl(EControlSetPaletteEntry,(TAny*)aIndex,(TAny*)aEntry);}
inline void RDeviceVideo::ScrollUp(const TRect& aRect)
	{DoControl(EControlScrollUp,(TRect*)&aRect);}
inline void RDeviceVideo::Clear(const TRect& aRect)
	{DoControl(EControlClear,(TRect*)&aRect);}
inline TInt RDeviceVideo::GetPixel(const TPoint& aPoint)
	{return DoControl(EControlGetPixel,(TPoint*)&aPoint);} 
inline void RDeviceVideo::SetWord(const TPoint& aPoint,TInt& aWord)
	{DoControl(EControlSetWord,(TPoint*)&aPoint,&aWord);}
inline TInt RDeviceVideo::GetWord(const TPoint& aPoint)
	{return (DoControl(EControlGetWord,(TPoint*)&aPoint));}
inline void RDeviceVideo::SetLine(const TPoint& aPoint,const TPixelLine& aPixelLine)
	{DoControl(EControlSetLine,(TPoint*)&aPoint,(TPixelLine*)&aPixelLine);}
inline void RDeviceVideo::GetLine(const TPoint& aPoint,TPixelLine& aPixelLine)
	{DoControl(EControlGetLine,(TPoint*)&aPoint,&aPixelLine);}

inline TBlitInfo::TBlitInfo(const TText *aBuffer,const TColour aColour, const TInt aLength,const TInt aX,const TInt aY)
	:iBuffer(aBuffer),iColour(aColour),iLength(aLength),iX(aX),iY(aY)
	{
	} 
inline TBlitInfo::TBlitInfo()
	:iBuffer(0),iColour(EBlack),iLength(0),iX(0),iY(0)
	{
	} 
inline TPixelInfo::TPixelInfo(const TInt aX,const TInt aY,const TColour aColour)
	:iColour(aColour),iX(aX),iY(aY)
	{
	} 
inline TPixelInfo::TPixelInfo()
	:iColour(EBlack),iX(0),iY(0)
	{
	} 

