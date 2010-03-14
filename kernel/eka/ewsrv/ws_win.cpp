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
// e32\ewsrv\ws_win.cpp
// 
//

#include "ws_std.h"
#include <e32hal.h>
#include <hal.h>
#include <domainmanager.h>

#ifdef __VC32__
    #pragma setlocale("english")
#endif

//#define __CHARACTERPOINTER

GLREF_D CKeyTranslator *KeyTranslator;
GLREF_D CKeyRepeat *KeyRepeat;

const TInt KTabSize=4;

#if defined(_UNICODE)	// K.K
#define FONTWIDTH 8

TBool CWsWindow::IsHankaku(const TText aCode)
	{
	if (aCode >= 0xff61 && aCode <= 0xff9f) 
		return ETrue;   // HANKAKU KATAKANA code
	if (aCode >= 0x2550 && aCode <= 0x259f)
		return ETrue;	// HANKAKU Graphics code
	if (aCode < 0x100)
        return ETrue;	// Alphanumeric codes means HANKAKU
	return EFalse;
	}

TInt CWsWindow::FitInWidth(TText* aDest,TInt aWidth,TInt aAsciiCol,TText aCode)
	{
	TInt pixel=aAsciiCol*FONTWIDTH;
	TInt aJpnCol=0;
	TInt jw=0;
	TInt width;
	while (pixel>0)
		{
		TInt width=IsHankaku(aDest[aJpnCol++]) ? FONTWIDTH : FONTWIDTH*2;
		pixel-=width;
		jw+=width;
		}
	width=IsHankaku(aCode) ? FONTWIDTH : FONTWIDTH*2;
	TInt w=jw+width-aWidth*FONTWIDTH;
	while (w > 0)
		w-=IsHankaku(aDest[aJpnCol-- - 1]) ? FONTWIDTH : FONTWIDTH*2;
	aDest[aJpnCol]=aCode;
	return aJpnCol;
	}

TInt CWsWindow::OffsetHZa(const TText* aDest,const TPoint& aPosition,const TSize& aSize,TInt& aX)
	{
	TInt i=aPosition.iY*aSize.iWidth;
	TInt j=0;
	TInt x=aPosition.iX*FONTWIDTH;
	while (x>0)
		x-=IsHankaku(aDest[i+j++]) ? FONTWIDTH : FONTWIDTH * 2;
	if (x<0) --j;
	aX=j;
	return(i+j);
	}

TInt CWsWindow::OffsetHZwP(const TText* aDest,const TPoint& aPosition,const TSize& aSize,TPoint& aP)
	{
	aP=aPosition;
	TInt x;
	TInt offset=OffsetHZa(aDest,aPosition,aSize,x);
	aP.iX=x;
	return offset;
	}

TInt CWsWindow::OffsetHZ(const TText* aDest,const TPoint& aPosition,const TSize& aSize)
	{
	TInt x;
	return OffsetHZa(aDest, aPosition, aSize, x);
	}

TText CWsWindow::GetCharFromOffset(const TText* aDest,const TPoint& aPosition,const TSize& aSize)
	{
	return aDest[OffsetHZ(aDest, aPosition, aSize)];
	}

TText* CWsWindow::GetCpFromOffset(const TText* aDest,const TPoint& aPosition,const TSize& aSize)
	{
	return (TText * )(& aDest[OffsetHZ(aDest, aPosition, aSize)]);
	}
#endif

typedef CScreenDriver* (*TScreenDriverCreate)();

void CWsWindow::New()
//
// Acquire resources needed to run window system
//
	{
	// Load EDISP.DLL from Z:\SYSTEM\LIBS and invoke ordinal 1 to
	// create the screen driver. This is because EDISP.DLL is hardware dependent.
	RLibrary edisp;
	ScreenDriver=NULL;
	TInt r=edisp.Load(_L("EDISP.DLL"), KNullDesC);
	if (r!=KErrNone && r!=KErrAlreadyExists)
		Fault(EWindowsInitialisation);
	TScreenDriverCreate f=(TScreenDriverCreate)edisp.Lookup(1);
	if (f)
		ScreenDriver=(*f)();
	__ASSERT_ALWAYS(ScreenDriver!=NULL,Fault(EWindowsInitialisation));
	ScreenDriver->Init(ScreenSize,FontSize);
	ScreenDriver->SetMode(EMono);
	// Set up data
	ScreenColor=IndexOf[ETextAttributeNormal+1];
	BorderColor=IndexOf[ETextAttributeNormal];
	WindowBgColor=IndexOf[ETextAttributeNormal+1];
	ScreenDriver->GetAttributeColors(IndexOf);
	CursorPeriodic=CPeriodic::New(ECursorPeriodicPriority);
	__ASSERT_ALWAYS(CursorPeriodic!=NULL,Fault(EWindowsInitialisation));
	CursorPeriodic->Start(500000,500000,TCallBack(CWsWindow::FlashCursor,NULL));
	Numbers=CBitMapAllocator::New(EMaxOpenWindows);
	__ASSERT_ALWAYS(Numbers!=NULL,Fault(EWindowsInitialisation));
	Numbers->AllocAt(EBackgroundNumber);
	VisibilityMap=(TInt8 *)User::Alloc(ScreenSize.iWidth*ScreenSize.iHeight);
	__ASSERT_ALWAYS(VisibilityMap!=NULL,Fault(EWindowsInitialisation));
	Mem::Fill(VisibilityMap,ScreenSize.iWidth*ScreenSize.iHeight,0);
	BlankLineText=(TText *)User::Alloc(sizeof(TText)*ScreenSize.iWidth);
	__ASSERT_ALWAYS(BlankLineText!=NULL,Fault(EWindowsInitialisation));
	BlankLineAttributes=(ColorInformation *)User::Alloc(ScreenSize.iWidth*sizeof(ColorInformation));
	__ASSERT_ALWAYS(BlankLineAttributes!=NULL,Fault(EWindowsInitialisation));
	TextFill(BlankLineText,ScreenSize.iWidth,_S(" "));
	Mem::Fill(BlankLineAttributes,ScreenSize.iWidth*sizeof(ColorInformation),ScreenColor);
	TInt err=MouseMutex.CreateLocal();
	__ASSERT_ALWAYS(err==KErrNone,Fault(EWindowsInitialisation));
	err=ServiceMutex.CreateLocal();
	__ASSERT_ALWAYS(err==KErrNone,Fault(EWindowsInitialisation));
	SetMode(EMono);
	}

void CWsWindow::Delete()
//
// Release resources needed to run window system
//
	{

	delete ScreenDriver;
	delete CursorPeriodic;
	delete Numbers;
	User::Free(VisibilityMap);
	User::Free(BlankLineText);
	User::Free(BlankLineAttributes);
	MouseMutex.Close();
	ServiceMutex.Close();
	}

void CWsWindow::TextFill(TText *aBuffer,TInt aLength,const TText *aValue)
//
// This helper function provided because no UNICODE compatible fill function exists in User::
//
	{

	TPtr(aBuffer,aLength).Fill(*aValue,aLength);
	}

TInt CWsWindow::Offset(const TPoint &aPosition,const TSize &aSize)
//
// Finds the offset of aPosition within an area of aSize dimensions
//
	{

	return(aPosition.iY*aSize.iWidth+aPosition.iX);
	}

TInt8 CWsWindow::NewNumberL()
//
// Issue a unique window id
//
	{

	TInt8 n=(TInt8)Numbers->Alloc();
	if (n<0)
		User::Leave(ETooManyWindowsOpen);
	return(n);
	}

void CWsWindow::ReleaseNumber(TInt8 aNumber)
//
// Return unique window id back to the pool for future issuing
//
	{

	Numbers->Free((TUint)aNumber);
	}

TInt CWsWindow::FlashCursor(TAny* /*aParameter*/)
//
// Flash the cursor if it is on
//
	{

	CWsWindow *pT=TopWindow();
	if (pT)
		{
		BeginUpdateScreen();
		if (pT->iCursorIsOn)
			{
			TPoint p=pT->iCursorPos-pT->iCurrentOffset;
			if (pT->IsInClippedTextArea(p) && p+pT->iViewOrigin!=MousePos)
				{
//#if defined(_UNICODE)	// K.K
//				TPoint sp;	// K.K
//				TInt offset=pT->OffsetHZwP(pT->iTextBuffer, pT->iCursorPos, pT->iCurrentSize, sp);	//K.K
//				const TText c = pT->iTextBuffer[offset];	// K.K
//				sp= sp - pT->iCurrentOffset;	// K.K
//				ScreenDriver->Blit(&c, 1, sp+pT->iViewOrigin);	// K.K
//#else	// K.K
				TInt i=Offset(pT->iCursorPos,pT->iCurrentSize);
				const TText c=pT->iTextBuffer[i];
				ScreenDriver->SetForegroundColor(pT->iAttributeBuffer[i].iFg);
				ScreenDriver->SetBackgroundColor(pT->iAttributeBuffer[i].iBg);
				ScreenDriver->Blit(&c,1,p+pT->iViewOrigin);
//#endif	// K.K
				}
			pT->iCursorIsOn=EFalse;
			}
		else
			{
			if (pT->iCursorRequired && pT->iReadIsValid)
				{
				TPoint p=pT->iCursorPos-pT->iCurrentOffset;
				if (pT->IsInClippedTextArea(p))
		    		{
					if (p+pT->iViewOrigin!=MousePos)
						{
						ScreenDriver->Blit(&(pT->iCursor),1,p+pT->iViewOrigin);
//#if defined(_UNICODE)	// K.K
//						TPoint sp;	// K.K
//						pT->OffsetHZwP(pT->iTextBuffer, pT->iCursorPos, pT->iCurrentSize, sp);	//K.K
//						sp= sp - pT->iCurrentOffset;	// K.K
//						ScreenDriver->Blit(&(pT->iCursor),1,sp+pT->iViewOrigin);	// K.K
//#else	// K.K
						ScreenDriver->SetForegroundColor(pT->iFgColor);
						ScreenDriver->SetBackgroundColor(WindowBgColor);
						ScreenDriver->Blit(&(pT->iCursor),1,p+pT->iViewOrigin);
//#endif	// K.K
						}
					pT->iCursorIsOn=ETrue;
					}
				}
			}
		EndUpdateScreen();
		}
	return(KErrNone);
	}

void CWsWindow::SetCursor()
//
// Place the text cursor for this window (if required)
//
    {

	BeginUpdateScreen();
	if (iCursorIsOn)
		{
		if (iCursorPos!=iLastCursorPos || !IsTop() || !iCursorRequired)
			{
			TPoint p=iLastCursorPos-iCurrentOffset;
			if (IsInClippedTextArea(p))
	    	    {
				TPoint q=p+iViewOrigin;
				if (q!=MousePos)
					{
					if(VisibilityMap[Offset(q,ScreenSize)]==iNumber)
						{
//#if defined(_UNICODE)	// K.K
//						TPoint sp;	// K.K
//						TInt offset = OffsetHZwP(iTextBuffer, iLastCursorPos, iCurrentSize, sp);	//K.K
//						sp= sp - iCurrentOffset;	// K.K
//						const TText c = iTextBuffer[offset];	// K.K
//						sp = sp + iViewOrigin;	// K.K
//						ScreenDriver->Blit(&c,1,sp);	// K.K
//#else	// K.K
						TInt i=Offset(iLastCursorPos,iCurrentSize);
						const TText c=iTextBuffer[i];
						ScreenDriver->SetForegroundColor(iAttributeBuffer[i].iFg);
						ScreenDriver->SetBackgroundColor(iAttributeBuffer[i].iBg);
						ScreenDriver->Blit(&c,1,q);
//#endif	// K.K
						}
					}
				iCursorIsOn=EFalse;
			    }
			}
		}
    if (IsTop() && iCursorRequired && iReadIsValid)
		{
		TPoint p=iCursorPos-iCurrentOffset;
		if (IsInClippedTextArea(p))
    	    {
			TPoint q=p+iViewOrigin;
			if (q!=MousePos)
				{
//#if defined(_UNICODE)	// K.K
//				TPoint sp;	// K.K
//				OffsetHZwP(iTextBuffer, iLastCursorPos, iCurrentSize, sp);	//K.K
//				sp= sp - iCurrentOffset;	// K.K
//				sp = sp + iViewOrigin;	// K.K
//				ScreenDriver->Blit(&iCursor,1,sp);	// K.K
//#else	// K.K
				ScreenDriver->SetForegroundColor(iFgColor);
				ScreenDriver->SetBackgroundColor(WindowBgColor);
				ScreenDriver->Blit(&iCursor,1,q);
//#endif	// K.K
				}
			iLastCursorPos=iCursorPos;
			iCursorIsOn=ETrue;
		    }
		}
	EndUpdateScreen();
	}

void CWsWindow::SetClip()
//
//	Set the clipped width and depth.
//
    {

	iClippedSize=ScreenSize-iViewOrigin;
	if (iClippedSize.iWidth>iViewSize.iWidth)
		iClippedSize.iWidth=iViewSize.iWidth;
	if (iClippedSize.iHeight>iViewSize.iHeight)
		iClippedSize.iHeight=iViewSize.iHeight;
    }

void CWsWindow::ResetVisibilityMap()
//
//	Recreate visibility map from window queue
//
	{

	TDblQueIter<CWsWindow> q(WQueue);
	CWsWindow *pW;
	Mem::Fill(VisibilityMap,ScreenSize.iWidth*ScreenSize.iHeight,0);
	q.SetToLast();
	while((pW=q--)!=NULL)
		{
		if (pW->iIsVisible)
			{
			TInt8 *pV=&VisibilityMap[Offset(pW->iViewOrigin,ScreenSize)];
			for(TInt i=0; i<pW->iClippedSize.iHeight; i++)
				{
				Mem::Fill(pV,pW->iClippedSize.iWidth,pW->iNumber);
				pV+=ScreenSize.iWidth;
				}
			}
		}
	}

void CWsWindow::BeginUpdateScreen()
//
// Prepare for a whole bunch of UpdateScreen() calls
//
	{

    MouseMutex.Wait();
	}

void CWsWindow::EndUpdateScreen()
//
// End of bunch of UpdateScreen() calls
//
	{

    MouseMutex.Signal();
	}

void CWsWindow::UpdateScreen(TPoint &aPosition,TInt aLength,TInt8 aNumber,TText *aTextBuffer,ColorInformation * anAttributeBuffer)
//
// This function purposefully made static,so that it can be used to update the background (aNumber=0) as well as
// window data. Line by line, it finds contiguous sections of visible window (using aNumber in the visibility map)
// and passes them to CScreenDriver::Blit().
//
	{

	TPoint q=aPosition;
	TInt8 *pV=&VisibilityMap[Offset(q,ScreenSize)];
	TInt w=aLength;
	while(w>0)
		{
		if (*pV==aNumber)
			{
			TPoint p=q;
			TInt l=0;
			TColorIndex fg=anAttributeBuffer[aLength-w].iFg;
			TColorIndex bg=anAttributeBuffer[aLength-w].iBg;
			while(w>=0)
				{
				if (*pV==aNumber && w!=0 && anAttributeBuffer[aLength-w].iFg==fg && anAttributeBuffer[aLength-w].iBg==bg)
					{
					l++;
					w--;
					q.iX++;
					pV++;
					}
				else
					{
					TText *pT=&aTextBuffer[p.iX-aPosition.iX];
					ScreenDriver->SetForegroundColor(fg);
					ScreenDriver->SetBackgroundColor(bg);
					ScreenDriver->Blit(pT,l,p);
					if (p.iY==MousePos.iY)
						{
						if (MousePos.iX>=p.iX && MousePos.iX<p.iX+l)
							TurnMouseOn();
						}
					break;
					}
				}
			}
		else
			{
			w--;
			q.iX++;
			pV++;
			}
		}
	}

TInt CWsWindow::SetMode(TVideoMode aMode)
	{
	
	TInt r=CWsWindow::ScreenDriver->SetMode(aMode);		
	if(r!=KErrNone)
		return(r);
	CWsWindow::ScreenDriver->GetAttributeColors(IndexOf);
	CWsWindow::ScreenColor=IndexOf[ETextAttributeNormal+1];
	CWsWindow::BorderColor=IndexOf[ETextAttributeNormal];
	CWsWindow::WindowBgColor=IndexOf[ETextAttributeNormal+1];
	CWsWindow::ChangeUIColors();
	CWsWindow* w;
	TDblQueIter<CWsWindow>q(WQueue);
	while((w=q++)!=NULL)
		{
		w->iFgColor=IndexOf[ETextAttributeNormal];
		w->iBgColor=IndexOf[ETextAttributeNormal+1];
		}
	Redraw();
	return KErrNone;
	}

void CWsWindow::Display()
//
//	Display a windows contents on the screen.
//
    {

    if (iIsVisible)
		{
		TPoint p=iViewOrigin;
		TSize s=iClippedSize;
		BeginUpdateScreen();
		TText *pT=&iTextBuffer[Offset(iCurrentOffset,iCurrentSize)];
		ColorInformation *pA=&iAttributeBuffer[Offset(iCurrentOffset,iCurrentSize)];
		while(s.iHeight>0)
			{
			UpdateScreen(p,s.iWidth,iNumber,pT,pA);
			s.iHeight--;
			p.iY++;
			pT=&pT[iCurrentSize.iWidth];
			pA=&pA[iCurrentSize.iWidth];
			}
		EndUpdateScreen();
		SetCursor();
		}
	}

void CWsWindow::Background()
//
//	Update wallpaper to physical screen
//
    {

	TPoint p(0,0);
	BeginUpdateScreen();
    while(p.iY<ScreenSize.iHeight)
    	{
    	UpdateScreen(p,ScreenSize.iWidth,0,BlankLineText,BlankLineAttributes);
		p.iY++;
		}
	EndUpdateScreen();
    }

void CWsWindow::Redraw()
//
// Redraw the whole screen including all the windows and the background
//
	{

	CWsWindow *pW=TopWindow();
	if (pW)
		pW->Refresh();
	else
		{
	    ResetVisibilityMap();
	    Background();
		}
	}

void CWsWindow::Refresh()
//
//	Refresh this window and those below it
//
	{

	CWsWindow* w;
	TDblQueIter<CWsWindow> q(WQueue);
	ResetVisibilityMap();
	while(q++!=this)
		{
		}
	Display();					// this window
	while((w=q++)!=NULL)
		w->Display();			// lower windows
	Background();
	}

void CWsWindow::ChangeUIColors()
//
// Change UI colours as set by the user
//
	{
	CWsWindow* w;

	TDblQueIter<CWsWindow>q(WQueue);
	while((w=q++)!=NULL)
		{
		TInt i=w->iCurrentSize.iWidth*w->iCurrentSize.iHeight;
		TColorIndex c=w->iAttributeBuffer[i-1].iBg;
		for(TInt t=0;t<i;t++)
			if (w->iAttributeBuffer[t].iBg==c)
				w->iAttributeBuffer[t].iBg=WindowBgColor;
		w->SetFrame();
		}
	Mem::Fill(BlankLineAttributes,ScreenSize.iWidth*sizeof(ColorInformation),ScreenColor);
	Redraw();
	}

void CWsWindow::SetTextAttribute(TTextAttribute anAttribute)
//
//
//
	{

	iFgColor=IndexOf[anAttribute*2];
	iBgColor=IndexOf[anAttribute*2+1];
	}

void CWsWindow::Clear()
//
// Clear the whole window and place cursor at top left.
//
	{

	TextFill(iTextBuffer,iCurrentSize.iWidth*iCurrentSize.iHeight,_S(" "));
	Mem::Fill(iAttributeBuffer,iCurrentSize.iWidth*iCurrentSize.iHeight*sizeof(ColorInformation),WindowBgColor);
	SetFrame();
	iCursorPos=TPoint(1,1);
	Display();
	}

void CWsWindow::Write(const TDesC &aBuffer)
//
//	Write characters to the window
//
	{

	const TText *b=aBuffer.Ptr();
	const TText *bE=b+aBuffer.Length();
	while(b<bE)
		{
		switch(*b)
			{
		case 0x00://Null
			break;
		case 0x07:
			User::Beep(440,100000);
			break;
		case 0x08://Backspace
		case 0x7f://Delete
			BackSpace();
			break;
		case 0x09:
			HorizontalTab();
			break;
		case 0x0a:
			LineFeed();
			break;
		case 0x0b://Vertical tab - do nothing
			break;
		case 0x0c:
			FormFeed();
			break;
		case 0x0d:
			CarriageReturn();
			break;
		default:
			WriteCharacter(b);
			}
		b++;
		}
	}

void CWsWindow::WriteCharacter(const TText *aCharacter)
//
// Write a single character at the current cursor location - must check if it's being written to an
// position temporarily occupied by the frame. In this case, the character goes to the 'edge' of the
// buffer. It will be restored from here when necessary.
//
	{

	TPoint p=iCursorPos;
	TText *pT=&iTextBuffer[Offset(p,iCurrentSize)];
	ColorInformation *pA=&iAttributeBuffer[Offset(p,iCurrentSize)];
	if (p.iX>=iCurrentOffset.iX && p.iX<iCurrentOffset.iX+iViewSize.iWidth) // within view width
		{
		if (p.iY==iCurrentOffset.iY) // top edge
			{
			TPoint q=p;
			q.iY=0;
			pT=&iTextBuffer[Offset(q,iCurrentSize)];
			pA=&iAttributeBuffer[Offset(q,iCurrentSize)];
			}
		else if (p.iY==iCurrentOffset.iY+iViewSize.iHeight-1) // bottom edge
			{
			TPoint q=p;
			q.iY=iCurrentSize.iHeight-1;
			pT=&iTextBuffer[Offset(q,iCurrentSize)];
			pA=&iAttributeBuffer[Offset(q,iCurrentSize)];
			}
		}
	if (p.iY>=iCurrentOffset.iY && p.iY<iCurrentOffset.iY+iViewSize.iHeight) // within view depth
		{
		if (p.iX==iCurrentOffset.iX) // left edge
			{
			TPoint q=p;
			q.iX=0;
			pT=&iTextBuffer[Offset(q,iCurrentSize)];
			pA=&iAttributeBuffer[Offset(q,iCurrentSize)];
			}
		else if (p.iX==iCurrentOffset.iX+iViewSize.iWidth-1) // right edge
			{
			TPoint q=p;
			q.iX=iCurrentSize.iWidth-1;
			pT=&iTextBuffer[Offset(q,iCurrentSize)];
			pA=&iAttributeBuffer[Offset(q,iCurrentSize)];
			}
		}

	*pT=*aCharacter;
	pA->iFg=iFgColor;
	pA->iBg=iBgColor;

	p-=iCurrentOffset;

	if (IsInClippedTextArea(p))
		{
		p+=iViewOrigin;
		TInt8 *pV=&VisibilityMap[Offset(p,ScreenSize)];
		if (*pV==iNumber && p!=MousePos)
			{
			BeginUpdateScreen();
			ScreenDriver->SetForegroundColor(iFgColor);
			ScreenDriver->SetBackgroundColor(iBgColor);
			ScreenDriver->Blit(aCharacter,1,p);
			EndUpdateScreen();
			}
		if (iCursorIsOn && iCursorPos==((CWsWindow*)this)->iLastCursorPos)
			iCursorIsOn=EFalse; // Just overwrote the cursor character
		}
	Right();
	}

void CWsWindow::BackSpace()
//
// BS on this window
//
	{

	if (iCursorPos!=TPoint(1,1))
		{
		Left();
		TColorIndex col=iBgColor;
		iBgColor=WindowBgColor;
		WriteCharacter(_S(" "));
		iBgColor=col;
		Left();
		}
	}

void CWsWindow::HorizontalTab()
//
// Tab on this window
//
	{

	iCursorPos.iX=iCursorPos.iX-(iCursorPos.iX-1)%KTabSize+KTabSize;
	if (iCursorPos.iX>iCurrentSize.iWidth-1)
		{
		CarriageReturn();
		if (!iWrapLock)
			LineFeed();
		}
	}

void CWsWindow::FormFeed()
//
// FF on this window = clear the screen and place cursor at top left corner
//
	{

	Clear();
	Display();
	}

void CWsWindow::LineFeed()
//
// LF on this window (must do CR separately)
//
	{
	
	if (iNewLineMode)
		CarriageReturn();
	if (iCursorPos.iY<(iCurrentSize.iHeight-2))
		iCursorPos.iY++;
	else
		ScrollUp();
	}

void CWsWindow::CarriageReturn()
//
// CR on this wiondow
//
	{

	iCursorPos.iX=1;
	}

void CWsWindow::Right()
//
//	Move the cursor right one character.
//
	{

	if (++iCursorPos.iX>=iCurrentSize.iWidth-1)
		{
		if (!iWrapLock)
			{
			CarriageReturn();
			LineFeed();
			}
		else iCursorPos.iX--;
		}
	}


void CWsWindow::Left()
//
//	Move the cursor left one character
//
	{

	if (iCursorPos!=TPoint(1,1))
		{
		if (iCursorPos.iX==1)
			{
			iCursorPos.iX+=iCurrentSize.iWidth-2;
			iCursorPos.iY--;
			}
		iCursorPos.iX--;
		}
	}

void CWsWindow::RestoreEdges()
//
//	Restore saved charcters from the 'edges' of the buffer back into their proper place in the buffer
//
	{

	if (iCurrentOffset.iY!=0)	// need to restore top edge
		{
		TPoint t(iCurrentOffset.iX,0);						// Top left point of buffer 'edge'
		TPoint f(iCurrentOffset.iX,iCurrentOffset.iY); 		// Top left point of frame in buffer
		TText *pTT=&iTextBuffer[Offset(t,iCurrentSize)];
		TText *pTF=&iTextBuffer[Offset(f,iCurrentSize)];
		Mem::Copy(pTF,pTT,iViewSize.iWidth*sizeof(TText));
		ColorInformation *pAT=&iAttributeBuffer[Offset(t,iCurrentSize)];
		ColorInformation *pAF=&iAttributeBuffer[Offset(f,iCurrentSize)];
		Mem::Copy(pAF,pAT,iViewSize.iWidth*sizeof(ColorInformation));
		}
	if (iCurrentOffset.iY+iViewSize.iHeight!=iCurrentSize.iHeight)	// need to save bottom edge
		{
		TPoint b(iCurrentOffset.iX,iCurrentSize.iHeight-1);					// Bottom left point of buffer 'edge'
		TPoint f(iCurrentOffset.iX,iCurrentOffset.iY+iViewSize.iHeight-1);	// Bottom left point of frame in buffer
		TText *pTB=&iTextBuffer[Offset(b,iCurrentSize)];
		TText *pTF=&iTextBuffer[Offset(f,iCurrentSize)];
		Mem::Copy(pTF,pTB,iViewSize.iWidth*sizeof(TText));
		ColorInformation *pAB=&iAttributeBuffer[Offset(b,iCurrentSize)];
		ColorInformation *pAF=&iAttributeBuffer[Offset(f,iCurrentSize)];
		Mem::Copy(pAF,pAB,iViewSize.iWidth*sizeof(ColorInformation));
		}
	if (iCurrentOffset.iX!=0)	// need to save left hand edge
		{
		TPoint l(0,iCurrentOffset.iY);										// Top left point of buffer 'edge'
		TPoint f(iCurrentOffset.iX,iCurrentOffset.iY);						// Top left point of frame in buffer
		for(TInt y=0;y<iViewSize.iHeight;y++)
			{
			TText *pTL=&iTextBuffer[Offset(l,iCurrentSize)];
			TText *pTF=&iTextBuffer[Offset(f,iCurrentSize)];
			*pTF=*pTL;
			ColorInformation *pAL=&iAttributeBuffer[Offset(l,iCurrentSize)];
			ColorInformation *pAF=&iAttributeBuffer[Offset(f,iCurrentSize)];
			pAF->iFg=pAL->iFg;
			pAF->iBg=pAL->iBg;
			l.iY++;
			f.iY++;
			}
		}
	if (iCurrentOffset.iX+iViewSize.iWidth!=iCurrentSize.iWidth)	// need to save right hand edge
		{
		TPoint r(iCurrentSize.iWidth-1,iCurrentOffset.iY);					// Top right point of buffer 'edge'
		TPoint f(iCurrentOffset.iX+iViewSize.iWidth-1,iCurrentOffset.iY);	// Top right point of frame in buffer
		for(TInt y=0;y<iViewSize.iHeight;y++)
			{
			TText *pTL=&iTextBuffer[Offset(r,iCurrentSize)];
			TText *pTF=&iTextBuffer[Offset(f,iCurrentSize)];
			*pTF=*pTL;
			ColorInformation *pAL=&iAttributeBuffer[Offset(r,iCurrentSize)];
			ColorInformation *pAF=&iAttributeBuffer[Offset(f,iCurrentSize)];
			pAF->iFg=pAL->iFg;
			pAF->iBg=pAL->iBg;
			r.iY++;
			f.iY++;
			}
		}
	}


void CWsWindow::SaveEdges()
//
// Save charcters about to be stonked by the frame into the 'edges' of the buffer - the buffer is already 2 positions
// wider and 2 positions deeper than requested when the window was set.
//
	{

	if (iCurrentOffset.iY!=0)	// need to save top edge
		{
		TPoint t(iCurrentOffset.iX,0);						// Top left point of buffer 'edge'
		TPoint f(iCurrentOffset.iX,iCurrentOffset.iY); 		// Top left point of frame in buffer
		TText *pTT=&iTextBuffer[Offset(t,iCurrentSize)];
		TText *pTF=&iTextBuffer[Offset(f,iCurrentSize)];
		Mem::Copy(pTT,pTF,iViewSize.iWidth*sizeof(TText));
		ColorInformation *pAT=&iAttributeBuffer[Offset(t,iCurrentSize)];
		ColorInformation *pAF=&iAttributeBuffer[Offset(f,iCurrentSize)];
		Mem::Copy(pAT,pAF,iViewSize.iWidth*sizeof(ColorInformation));
		}
	if (iCurrentOffset.iY+iViewSize.iHeight!=iCurrentSize.iHeight)	// need to save bottom edge
		{
		TPoint b(iCurrentOffset.iX,iCurrentSize.iHeight-1);					// Bottom left point of buffer 'edge'
		TPoint f(iCurrentOffset.iX,iCurrentOffset.iY+iViewSize.iHeight-1);	// Bottom left point of frame in buffer
		TText *pTB=&iTextBuffer[Offset(b,iCurrentSize)];
		TText *pTF=&iTextBuffer[Offset(f,iCurrentSize)];
		Mem::Copy(pTB,pTF,iViewSize.iWidth*sizeof(TText));
		ColorInformation *pAB=&iAttributeBuffer[Offset(b,iCurrentSize)];
		ColorInformation *pAF=&iAttributeBuffer[Offset(f,iCurrentSize)];
		Mem::Copy(pAB,pAF,iViewSize.iWidth*sizeof(ColorInformation));
		}
	if (iCurrentOffset.iX!=0)	// need to save left hand edge
		{
		TPoint l(0,iCurrentOffset.iY);										// Top left point of buffer 'edge'
		TPoint f(iCurrentOffset.iX,iCurrentOffset.iY);						// Top left point of frame in buffer
		for(TInt y=0;y<iViewSize.iHeight;y++)
			{
			TText *pTL=&iTextBuffer[Offset(l,iCurrentSize)];
			TText *pTF=&iTextBuffer[Offset(f,iCurrentSize)];
			*pTL=*pTF;
			ColorInformation *pAL=&iAttributeBuffer[Offset(l,iCurrentSize)];
			ColorInformation *pAF=&iAttributeBuffer[Offset(f,iCurrentSize)];
			pAL->iFg=pAF->iFg;
			pAL->iBg=pAF->iBg;
			l.iY++;
			f.iY++;
			}
		}
	if (iCurrentOffset.iX+iViewSize.iWidth!=iCurrentSize.iWidth)	// need to save right hand edge
		{
		TPoint r(iCurrentSize.iWidth-1,iCurrentOffset.iY);					// Top right point of buffer 'edge'
		TPoint f(iCurrentOffset.iX+iViewSize.iWidth-1,iCurrentOffset.iY);	// Top right point of frame in buffer
		for(TInt y=0;y<iViewSize.iHeight;y++)
			{
			TText *pTL=&iTextBuffer[Offset(r,iCurrentSize)];
			TText *pTF=&iTextBuffer[Offset(f,iCurrentSize)];
			*pTL=*pTF;
			ColorInformation *pAL=&iAttributeBuffer[Offset(r,iCurrentSize)];
			ColorInformation *pAF=&iAttributeBuffer[Offset(f,iCurrentSize)];
			pAL->iFg=pAF->iFg;
			pAL->iBg=pAF->iBg;
			r.iY++;
			f.iY++;
			}
		}
	}

TBool CWsWindow::IsInClippedTextArea(const TPoint& aPoint) const
//
// Returns ETrue if aPoint (relative to window) is in the screen and the text part of the window
//
	{
	
	return (aPoint.iX>0 && aPoint.iX<iClippedSize.iWidth-(iClippedSize.iWidth==iViewSize.iWidth) && aPoint.iY>0 && aPoint.iY<iClippedSize.iHeight-(iClippedSize.iHeight==iViewSize.iHeight));
	}

TBool CWsWindow::IsRectVisible(TRect& aRect) const
//
// Returns ETrue if window is visible in aRect, possibly clips edges to make a visible rectangle
//
	{

	if (IsTop())
		return ETrue;
	
	//First clip off the left
	TInt top=-1;
	TInt bottom=aRect.iBr.iY;
	TInt j;
	TInt i;
	for (i=aRect.iTl.iX;i<aRect.iBr.iX && top==-1;i++)
		{
		for (j=aRect.iTl.iY;j<aRect.iBr.iY;j++)
			{
			if (VisibilityMap[Offset(TPoint(i,j),ScreenSize)]!=iNumber)
				{
				if (top!=-1 && bottom==aRect.iBr.iY)
					bottom=j;
				}
			else
				{
				if (bottom!=aRect.iBr.iY)
					return EFalse;
				if (top==-1)
					top=j;
				}
			}
		}

	if (top==-1) //Area completely covered
		{
		aRect.iTl=aRect.iBr;
		return ETrue;//It is a rectangle - the zero rectangle 
		}
	aRect.iTl.iX=i-1;
	
	TBool passedEdge=EFalse;//Right edge
	for (;i<aRect.iBr.iX && !passedEdge;i++)
		{
		for (j=aRect.iTl.iY;j<aRect.iBr.iY;j++)
			{
			if (VisibilityMap[Offset(TPoint(i,j),ScreenSize)]==iNumber)
				{
				if (passedEdge || j<top || j>=bottom)
					return EFalse;
				}
			else
				{
				if (j==top)
					passedEdge=ETrue;
				if (!passedEdge && j>top && j<bottom)
					return EFalse;
				}
			}
		}
	
	if (passedEdge)
		{
		TInt right=i-1;

		for (;i<aRect.iBr.iX;i++)
			for (j=aRect.iTl.iY;j<aRect.iBr.iY;j++)
				if (VisibilityMap[Offset(TPoint(i,j),ScreenSize)]==iNumber)
					return EFalse;
		aRect.iBr.iX=right;
		}
	aRect.iTl.iY=top;
	aRect.iBr.iY=bottom;
	return ETrue;
	}

void CWsWindow::ScrollUp()
//
//	Scroll the window up by one line.
//
	{

	if (!iScrollLock)
		{
		RestoreEdges();
		TurnMouseOff();
		TText *pT=&iTextBuffer[iCurrentSize.iWidth];
		Mem::Copy(pT,&pT[iCurrentSize.iWidth],iCurrentSize.iWidth*(iCurrentSize.iHeight-3)*sizeof(TText));
		TextFill(&pT[iCurrentSize.iWidth*(iCurrentSize.iHeight-3)],iCurrentSize.iWidth*sizeof(TText),_S(" "));
		ColorInformation *pA=&iAttributeBuffer[iCurrentSize.iWidth];
		Mem::Copy(pA,&pA[iCurrentSize.iWidth],iCurrentSize.iWidth*(iCurrentSize.iHeight-3)*sizeof(ColorInformation));
		Mem::Fill(&pA[iCurrentSize.iWidth*(iCurrentSize.iHeight-3)],iCurrentSize.iWidth*sizeof(ColorInformation),WindowBgColor);
		SaveEdges();
		SetFrame();
		
		TBool oldCursorRequired = iCursorRequired;
		iCursorRequired = EFalse;
		SetCursor();

		if (iIsVisible)
			{

			TRect updateRect(iViewOrigin.iX+1,iViewOrigin.iY+1,iViewOrigin.iX+iViewSize.iWidth-1,iViewOrigin.iY+iViewSize.iHeight-1); 
			updateRect.Intersection(TRect(TPoint(0,0),ScreenSize));
			if (IsRectVisible(updateRect))
				{
#ifndef __X86__
				if (ScreenDriver->ScrollUp(updateRect))
					{
					//Update bottom line
					updateRect.iTl.iY=updateRect.iBr.iY-1;
					pT=&iTextBuffer[Offset(updateRect.iTl-iViewOrigin,iCurrentSize)];
					pA=&iAttributeBuffer[Offset(updateRect.iTl-iViewOrigin,iCurrentSize)];
					TColorIndex fg=pA->iFg;
					TColorIndex bg=pA->iBg;
					TInt k=updateRect.Width();
					TInt l=0;
					for(TInt i=0;i<k;i++)
						{
						if(fg==pA->iFg && bg==pA->iBg && i!=k-1)
							l++;						
						else
							{
							if(i==k-1) l++;
							ScreenDriver->SetForegroundColor(fg);
							ScreenDriver->SetBackgroundColor(bg);
							ScreenDriver->Blit(pT,l,updateRect.iTl);
							pT+=l;
							pA+=l;
							fg=(pA+1)->iFg;
							bg=(pA+1)->iBg;
							l=1;
							}
						}
					}
                else
#endif
        			Display();
				}
			}
		
		iCursorRequired = oldCursorRequired;
		SetCursor();
		TurnMouseOn();
		}
	}

void CWsWindow::TurnMouseOff()
//
//	Take the mouse of the screen
//
	{
#ifdef __CHARACTERPOINTER
	CWsWindow *pW=MouseWindow();
#if defined(_UNICODE)	// K.K
	TPoint sp=MousePos;	// K.K
	if (pW)
		{
		TPoint p=MousePos-pW->iViewOrigin+pW->iCurrentOffset;
		TInt offset=pW->OffsetHZwP(pW->iTextBuffer,p,pW->iCurrentSize,sp);	// K.K
		sp=sp+pW->iViewOrigin-pW->iCurrentOffset;	// K.K
		TText c=pW->iTextBuffer[offset];	// K.K
		if (pW->iCursorIsOn && p==pW->iCursorPos-pW->iCurrentOffset)
			c=pW->iCursor;
		ScreenDriver->SetForegroundColor(pW->iAttributeBuffer[offset].iFg);
		ScreenDriver->SetBackgroundColor(pW->iAttributeBuffer[offset].iBg);
		ScreenDriver->Blit(&c, 1, sp);	// K.K
		}
	else
		{
        ScreenDriver->SetBackgroundColor(ScreenColor);
		ScreenDriver->Blit(BlankLineText, 1, sp);	// K.K
		}
#else	// K.K
	if (pW)
		{
		TPoint p=MousePos-pW->iViewOrigin+pW->iCurrentOffset;
		TText c=pW->iTextBuffer[Offset(p,pW->iCurrentSize)];
		if (pW->iCursorIsOn && p==pW->iCursorPos-pW->iCurrentOffset)
			c=pW->iCursor;
		ScreenDriver->SetForegroundColor(pW->iAttributeBuffer[Offset(p,pW->iCurrentSize)].iFg);
		ScreenDriver->SetBackgroundColor(pW->iAttributeBuffer[Offset(p,pW->iCurrentSize)].iBg);
		ScreenDriver->Blit(&c, 1, MousePos);
		}
	else
		{
		ScreenDriver->SetBackgroundColor(ScreenColor);
		ScreenDriver->Blit(BlankLineText, 1, MousePos);
		}
#endif	// K.K

#endif // __CHARACTERPOINTER
	}

void CWsWindow::TurnMouseOn()
//
//	Place the mouse on the screen
//
	{

#ifdef __CHARACTERPOINTER
	const TText c=EMouseCharacter;
	ScreenDriver->SetForegroundColor(BorderColor);
	ScreenDriver->Blit(&c, 1, MousePos);
#endif
	}

void CWsWindow::MouseMove(TPoint aGraphicsPosition)
//
//	Move the mouse to a new position
//
	{

	TPoint p=aGraphicsPosition;
	p.iX/=FontSize.iWidth;
	p.iY/=FontSize.iHeight;
	if (p.iX>=ScreenSize.iWidth)
		p.iX=ScreenSize.iWidth-1;
	if (p.iY>=ScreenSize.iHeight)
		p.iY=ScreenSize.iHeight-1;
	if (p.iX<0)
		p.iX=0;
	if (p.iY<0)
		p.iY=0;
	if (MousePos!=p)
		{
		MouseMutex.Wait();
		TurnMouseOff();
		MousePos=p;
		TurnMouseOn();
		MouseMutex.Signal();
		CWsWindow* tw=TopWindow();
		if (ScrollWithMouse!=TPoint(-1,-1))
			{
			if (tw)
				tw->MouseSlide();
			}
		else if (MoveWithMouse!=TPoint(-1,-1))
			{
			if (tw)
				{
				if(MousePos.iX<tw->iViewOrigin.iX-1||MousePos.iX>tw->iViewOrigin.iX+tw->iCurrentSize.iWidth||MousePos.iY<tw->iViewOrigin.iY-1||MousePos.iY>tw->iViewOrigin.iY+tw->iCurrentSize.iHeight)
					{
					MoveWithMouse=TPoint(-1,-1);
					return;
					}
				CWsWindow::MoveTopWindowRelative(MousePos-MoveWithMouse);
				MoveWithMouse=MousePos;
				if(MousePos.iX==0 && tw->iViewOrigin.iX!=0)
					CWsWindow::MoveTopWindowRelative(TPoint(-tw->iViewOrigin.iX,0));
				if(MousePos.iY==0 && tw->iViewOrigin.iY!=0)
					CWsWindow::MoveTopWindowRelative(TPoint(0,-tw->iViewOrigin.iY));
				}
			}
		else if (ResizeWithMouse!=TPoint(-1,-1))
			{
			TInt r=CWsWindow::ChangeTopWindowSize(TSize(MousePos.iX-ResizeWithMouse.iX,MousePos.iY-ResizeWithMouse.iY));
			if(!r)
				ResizeWithMouse=MousePos;
			}
		}
	}

void CWsWindow::MouseSlide()
//
// Scroll the window
//
	{

	if(ScrollWithMouse.iX && MousePos.iX!=ScrollWithMouse.iX)
		{
		TInt r=SlideTopWindowRelative(TPoint((MousePos.iX-ScrollWithMouse.iX)*ScrollSpeed,0));
		if(!r)
			ScrollWithMouse.iX=MousePos.iX;
		else if(MousePos.iX<ScrollWithMouse.iX)
				{
				SlideTopWindowRelative(TPoint(-iCurrentOffset.iX,0));
				if(MousePos.iX<=iViewOrigin.iX)
					ScrollWithMouse.iX=iViewOrigin.iX+1;
				else
					ScrollWithMouse.iX=MousePos.iX;
				}
			else
				{
				SlideTopWindowRelative(TPoint(iCurrentSize.iWidth-iViewSize.iWidth-iCurrentOffset.iX,0));
				if(MousePos.iX>iViewOrigin.iX+iViewSize.iWidth-2)
					ScrollWithMouse.iX=iViewOrigin.iX+iViewSize.iWidth-2;
				else
					ScrollWithMouse.iX=MousePos.iX;
				}
		}
	else if(ScrollWithMouse.iY && MousePos.iY!=ScrollWithMouse.iY)
		{
		TInt r=SlideTopWindowRelative(TPoint(0,(MousePos.iY-ScrollWithMouse.iY)*ScrollSpeed));
		if(!r)
			ScrollWithMouse.iY=MousePos.iY;
		else if(MousePos.iY<ScrollWithMouse.iY)
				{
				SlideTopWindowRelative(TPoint(0,-iCurrentOffset.iY));
				if(MousePos.iY<=iViewOrigin.iY)
					ScrollWithMouse.iY=iViewOrigin.iY+1;
				else 
					ScrollWithMouse.iY=MousePos.iY;
				}
			else
				{
				SlideTopWindowRelative(TPoint(0,iCurrentSize.iHeight-iViewSize.iHeight-iCurrentOffset.iY));
				if(MousePos.iY>iViewOrigin.iY+iViewSize.iHeight-2)
					ScrollWithMouse.iY=iViewOrigin.iY+iViewSize.iHeight-2;
				else
					ScrollWithMouse.iY=MousePos.iY;
				}
		}

	}

void CWsWindow::InformTopMouse(TPoint aPos)
//
// Called if mouse has been captured
//
    {

    CWsWindow *pM=TopWindow();
    if(pM)
		pM->InformMouse(aPos);
    }

void CWsWindow::MouseLeftButton()
//
// Called when the left button is pressed
//
	{

	CWsWindow *pM=MouseWindow();
	CWsWindow *pT=TopWindow();
    if (pT && !MouseIsCaptured)
        {
	    if (pM)
		    {
		    if(pM!=pT)
			    pM->MakeTopWindow();
		    if(pM==TopWindow())
				pM->DoMouseLeftButton();
		    }
        else
            RotateWindowsForwards();           
        }
	}

void CWsWindow::MouseLeftButtonUp()
//
// Called when the left button is released
//
	{
	ScrollWithMouse=TPoint(-1,-1);
	MoveWithMouse=TPoint(-1,-1);
	ResizeWithMouse=TPoint(-1,-1);
	}

CWsWindow *CWsWindow::MouseWindow()
//
//	Return the window containing the mouse.
//
	{

	TInt8 n=VisibilityMap[Offset(MousePos,ScreenSize)];
	if (n!=0)
		{
		CWsWindow *pW;
		TDblQueIter<CWsWindow> q(WQueue);
		for(pW=q++;pW->iNumber!=n;pW=q++)
			;
		return(pW);
		}
	else
		return(NULL);
	}

TBool CWsWindow::IsTop() const
//
// Return TRUE if this window is the top window
//
	{

	return(WQueue.IsFirst(this));
	}

CWsWindow* CWsWindow::TopWindow()
//
// Return the top window
//
	{

	if (WQueue.IsEmpty())
		return(NULL);
	return(WQueue.First());
	}

CWsWindow *CWsWindow::BottomWindow()
//
// Return the bottom window
//
	{

	if (WQueue.IsEmpty())
		return(NULL);
	return(WQueue.Last());
	}

void CWsWindow::MakeTopWindow()
//
// Make this window the top window if possible
//
	{

	ResizeWithMouse=TPoint(-1,-1);
	ScrollWithMouse=TPoint(-1,-1);
	iLink.Deque();
	if(iOnTop||WQueue.IsEmpty()||!TopWindow()->iOnTop)
		{
		CWsWindow *pT=TopWindow();
		WQueue.AddFirst(*this);
		if(pT)
			pT->SetFrame();
		}
	else
		{
		TDblQueIter<CWsWindow> q(WQueue);
		q.SetToFirst();
		do
			q++;
		while(q!=NULL&&((CWsWindow*)q)->iOnTop);
		if (q==NULL)
			WQueue.AddLast(*this);
		else
			{
			q--;
			iLink.Enque(&(((CWsWindow*)q)->iLink));
			}
		}
	SetFrame();
	Refresh();
	}

void CWsWindow::SetWindowPosAbs(const TPoint &aPoint)
//
// Move the window to aPoint
//
	{
	
	ResizeWithMouse=TPoint(-1,-1);
	ScrollWithMouse=TPoint(-1,-1);
	iViewOrigin=aPoint;
	if (iViewOrigin.iX<0)
		iViewOrigin.iX=0;
	else if (iViewOrigin.iX>=ScreenSize.iWidth)
		iViewOrigin.iX=ScreenSize.iWidth-1;
	if (iViewOrigin.iY<0)
		iViewOrigin.iY=0;
	else if (iViewOrigin.iY>=ScreenSize.iHeight)
		iViewOrigin.iY=ScreenSize.iHeight-1;
	SetClip();
	Refresh();
	}

TInt CWsWindow::MoveTopWindowRelative(TPoint aDirection)
//
//	Move the top window relative to its current position
//
	{

	CWsWindow *pT=TopWindow();
	if (pT)
		{
		TPoint p=pT->iViewOrigin+aDirection;
		if (p.iX>=ScreenSize.iWidth || p.iX<0 || p.iY>=ScreenSize.iHeight || p.iY<0)
			return KErrArgument;
		pT->iViewOrigin=p;
		pT->SetClip();
		pT->Refresh();
		}
	return KErrNone;
	}

TInt CWsWindow::SlideTopWindowRelative(TPoint aDirection)
//
//	Slide the top window relative to its current position
//
	{

	CWsWindow *pT=TopWindow();
	if (pT && pT->iAllowSlide && aDirection!=TPoint(0,0))
		{
		TPoint p=pT->iCurrentOffset+aDirection;
		TSize s=pT->iCurrentSize-pT->iViewSize;
		if (p.iX>s.iWidth || p.iX<0 || p.iY>s.iHeight || p.iY<0)
			return KErrArgument;
		pT->RestoreEdges();
		pT->iCurrentOffset=p;
		pT->SaveEdges();
		pT->SetFrame();
		pT->Display();
		}
	return KErrNone;
	}

TInt CWsWindow::ChangeTopWindowSize(TSize aGrowth)
//
//	Increase the viewing size of the top window relative to its current size
//
	{

	CWsWindow *pT=TopWindow();
	if (pT && pT->iAllowResize)
		{
		TSize s=pT->iViewSize+aGrowth;
		if (s.iWidth>pT->iCurrentSize.iWidth || s.iWidth<3 || s.iHeight>pT->iCurrentSize.iHeight || s.iHeight<3)
			return KErrArgument;
		pT->RestoreEdges();
		pT->iViewSize=s;
		s=pT->iCurrentSize-pT->iViewSize;
		if (pT->iCurrentOffset.iX>s.iWidth ||pT-> iCurrentOffset.iY>s.iHeight)
			pT->iCurrentOffset-=aGrowth;
		pT->SaveEdges();
		pT->SetFrame();
		pT->SetClip();
		pT->Refresh();
		}
	return KErrNone;
	}

void CWsWindow::RotateWindowsForwards()
//
// Put the next window on top
//
	{

	CWsWindow *pT=TopWindow();
	if(pT && pT->iOnTop==EFalse)
		{
		CWsWindow *pB=BottomWindow();
		if (pT!=pB)
			{
			MoveWithMouse=TPoint(-1,-1);
			ResizeWithMouse=TPoint(-1,-1);
			ScrollWithMouse=TPoint(-1,-1);
			do
				{
				pB->iLink.Deque();
				WQueue.AddFirst(*pB);
				pT->SetFrame();
				pB->SetFrame();
				pT=TopWindow();
				pB=BottomWindow();
				}
			while(!pT->iIsVisible);
			pT->Refresh();
			}
		}
	}

void CWsWindow::RotateWindowsBackwards()
//
// Put the previous window on top
//
	{

	CWsWindow *pT=TopWindow();
	if(pT && pT->iOnTop==EFalse)
		{
		CWsWindow *pB=BottomWindow();
		if (pT!=pB)
			{
			MoveWithMouse=TPoint(-1,-1);
			ResizeWithMouse=TPoint(-1,-1);
			ScrollWithMouse=TPoint(-1,-1);
			do
				{
				pT->iLink.Deque();
				WQueue.AddLast(*pT);
				pT->SetFrame();
				pT=TopWindow();
				}
			while(!pT->iIsVisible);
			pT->SetFrame();
			pT->Refresh();
			}
		}
	}

void CWsWindow::QueueTopWindowKey(TKeyData &aKeystroke)
//
// Place keystroke in top window's keyboard queue
//
	{
	CWsWindow *pT=TopWindow();
	if (pT)
		pT->QueueWindowKey(aKeystroke);
	}

void CWsWindow::KeyPress(TKeyData &aKeystroke)
//
// Called whenever a key is pressed
//
	{
	switch(aKeystroke.iKeyCode)
		{
	case EKeyIncContrast:
		{
		TInt max=0;
        if (HAL::Get(HAL::EDisplayContrastMax,max)==KErrNone)
			{
			TInt now=0;
			HAL::Get(HAL::EDisplayContrast,now);
			if (now++<max)
				HAL::Set(HAL::EDisplayContrast,now);
			}
		}
		break;
	case EKeyDecContrast:
         {
         TInt now=0;
         TInt r=HAL::Get(HAL::EDisplayContrast,now);
         if (r==KErrNone && now-->0)
             HAL::Set(HAL::EDisplayContrast,now);
         }
   		break;
	case EKeyIncBrightness:
		{
		TInt max=0;
        if (HAL::Get(HAL::EDisplayBrightnessMax,max)==KErrNone)
			{
			TInt now=0;
			HAL::Get(HAL::EDisplayBrightness,now);
			if (now++<max)
				HAL::Set(HAL::EDisplayBrightness,now);
			}
		}
		break;
	case EKeyDecBrightness:
         {
         TInt now=0;
         TInt r=HAL::Get(HAL::EDisplayBrightness,now);
         if (r==KErrNone && now-->0)
             HAL::Set(HAL::EDisplayBrightness,now);
         }
   		break;
	case EKeyOff:
		{
		RDmDomainManager mgr; 
		TInt r = mgr.Connect();
		if (r != KErrNone)
			User::Panic(_L("EWSRV KeyOff0"), r);
		TRequestStatus status;
		mgr.RequestSystemTransition(EPwStandby, status);
		User::WaitForRequest(status);
		if (status.Int() != KErrNone)
			User::Panic(_L("EWSRV KeyOff1"), status.Int());
		mgr.Close();
		}
		break;
    case EKeyBacklightOn:
		HAL::Set(HAL::EBacklightState,ETrue);
        break;
    case EKeyBacklightOff:
		HAL::Set(HAL::EBacklightState,EFalse);
        break;
    case EKeyBacklightToggle:
        {
        TBool state;
        if (HAL::Get(HAL::EBacklightState,state)==KErrNone)
            HAL::Set(HAL::EBacklightState,!state);
        }
        break;
    default:
      	if (aKeystroke.iModifiers&EModifierCtrl) // Trap all Crtl + keystrokes for window manipulation
      		{
      		if (aKeystroke.iModifiers&EModifierShift)
      			{														// Ctrl + Shift
      			switch(aKeystroke.iKeyCode)
      				{
         			case EKeyLeftArrow:
         				SlideTopWindowRelative(TPoint(-1,0));
         				break;
         			case EKeyRightArrow:
         				SlideTopWindowRelative(TPoint(1,0));
         				break;
         			case EKeyUpArrow:
         				SlideTopWindowRelative(TPoint(0,-1));
         				break;
         			case EKeyDownArrow:
         				SlideTopWindowRelative(TPoint(0,1));
         				break;
         			default:
         				QueueTopWindowKey(aKeystroke); // Buffer keystroke for app
         				break;
       				}
      			}
      		else
      			{														// Ctrl
      			switch(aKeystroke.iKeyCode)
      				{
         			case EKeyLeftArrow:
         				ChangeTopWindowSize(TSize(-1,0));
         				break;
         			case EKeyRightArrow:
         				ChangeTopWindowSize(TSize(1,0));
         				break;
         			case EKeyUpArrow:
         				ChangeTopWindowSize(TSize(0,-1));
         				break;
         			case EKeyDownArrow:
         				ChangeTopWindowSize(TSize(0,1));
         				break;
                    case '1':
                        ScreenDriver->SetMode(EMono);
                        break;
                    case '2':
                        ScreenDriver->SetMode(EGray4);
                        break;
                    case '4':
                        ScreenDriver->SetMode(EGray16);
                        break;
					case '0':
         					ControlTopWindowMaximised(ETrue);
						break;
         			case '9':
         				ControlTopWindowMaximised(EFalse);
         				break;
         			case '5':
						KeyRepeat->Cancel();
         				RotateWindowsBackwards();
         				break;
         			case '6':
						KeyRepeat->Cancel();
         				RotateWindowsForwards();
         				break;
      			    default:
      				    QueueTopWindowKey(aKeystroke); // Buffer keystroke for app
      				    break;
      				}
      			}
      		}
		else if (aKeystroke.iModifiers&EModifierShift)

      		{														// Shift
      		switch(aKeystroke.iKeyCode)
      			{
         		case EKeyLeftArrow:
         			MoveTopWindowRelative(TPoint(-1,0));
         			break;
         		case EKeyRightArrow:
         			MoveTopWindowRelative(TPoint(1,0));
         			break;
         		case EKeyUpArrow:
         			MoveTopWindowRelative(TPoint(0,-1));
         			break;
         		case EKeyDownArrow:
         			MoveTopWindowRelative(TPoint(0,1));
         			break;
         		default:
         			QueueTopWindowKey(aKeystroke); // Buffer keystroke for app
         			break;
            	}
      		}	
     	if (!(aKeystroke.iModifiers&EModifierShift||aKeystroke.iModifiers&EModifierCtrl))
			QueueTopWindowKey(aKeystroke);
        }
	DrainAllReadRequests();
	}

void CWsWindow::ControlInformAllMouse(TBool anIndicator)
//
// Turn reporting of pointer events on or off according to the value of anIndicator
//
	{

	MouseIsCaptured=anIndicator;
	}

void CWsWindow::ControlTopWindowMaximised(TBool anIndicator)
//
// Maximise or minimise the top window according to the value of anIndicator
//
	{

	CWsWindow *pT=TopWindow();
	if(pT)
		pT->ControlMaximised(anIndicator);
	}

void CWsWindow::DrainAllReadRequests()
//
// Drain all the satisfied read requests on all waiting windows
//
	{

	TDblQueIter<CWsWindow> q(WQueue);
	CWsWindow *pW;
	while((pW=q++)!=NULL)
		pW->DrainReadRequest();
	}

#pragma warning( disable : 4705 )	// statement has no effect
CWsWindow::CWsWindow()
	: iNumber(-1)
//
// Constructor.
//
	{
	}
#pragma warning( default : 4705 )

void CWsWindow::SetTitle(const TDesC &aName) 
//
//	Changes the window's title
//
	{

	iTitle=aName;
	SetFrame();
	Display();
    }

void CWsWindow::SetSize(const TSize &aSize) 
//
//	Changes the window's size
//
	{
	iCurrentSize=aSize;	// This does not get called. Proper implementation is obviously more complicated than this.
	}

TSize CWsWindow::Size()
//
// Return underlying window size
//
	{
	return(iCurrentSize);
	}

CWsWindow::~CWsWindow()
//
// Destructor
//
	{

	SWsKey *pS;
	while(!iKQueue.IsEmpty())
		{
		pS=iKQueue.First();
		iKQueue.Remove(*pS);
		delete pS;
		}
	User::Free(iTextBuffer);
	User::Free(iAttributeBuffer);
	if (iNumber >= 0)
		{
		ReleaseNumber(iNumber);
		}
	if (iLink.iNext!=NULL)
		iLink.Deque();
	if(!WQueue.IsEmpty())
		WQueue.First()->SetFrame();
		
	Redraw();
	}


void CWsWindow::SetView()
//
// Assign an initial wiewing region to a window (this maybe modified later by the user)
//
	{

	Count&=3;	// Increment count through the sequence 0, 1, 2, 3, 0, 1, 2, ....
	iViewSize.iWidth=ScreenSize.iWidth>>1; // View is half width and half depth of physical screen
	iViewSize.iHeight=ScreenSize.iHeight>>1; // View is half width and half depth of physical screen
	iViewOrigin=TPoint(0,0);
	if (iViewSize.iWidth<30 || iViewSize.iHeight<10)
		iViewSize=ScreenSize;
	else
		{
		if (Count&1)
			iViewOrigin.iX+=iViewSize.iWidth;
		if (Count&2)
			iViewOrigin.iY+=iViewSize.iHeight;
		}
	if (iViewSize.iWidth>iCurrentSize.iWidth)
		iViewSize.iWidth=iCurrentSize.iWidth;
	if (iViewSize.iHeight>iCurrentSize.iHeight)
		iViewSize.iHeight=iCurrentSize.iHeight;
	Count++;
	}

void CWsWindow::SetFull()
//
// Calculate the view size and origin for this window if it were maximised and placed centrally
//
	{

	if (iCurrentSize.iWidth>ScreenSize.iWidth)
		{
		iMaximumOrigin.iX=0;
		iMaximumSize.iWidth=ScreenSize.iWidth;
		}
	else
		{
		iMaximumOrigin.iX=(ScreenSize.iWidth-iCurrentSize.iWidth)/2;
		iMaximumSize.iWidth=iCurrentSize.iWidth;
		}
	if (iCurrentSize.iHeight>ScreenSize.iHeight)
		{
		iMaximumOrigin.iY=0;
		iMaximumSize.iHeight=ScreenSize.iHeight;
		}
	else
		{
		iMaximumOrigin.iY=(ScreenSize.iHeight-iCurrentSize.iHeight)/2;
		iMaximumSize.iHeight=iCurrentSize.iHeight;
		}
	}

void CWsWindow::SetFrame()
//
//	Draw the frame into the buffer.
//
	{

// WINS text window server uses underlying Win32 graphics calls, so the Unicode
// WINS build has access to Unicode fonts, and needs to use the Unicode box-drawing
// character codes. 
// EPOC devices have a codepage 1252 font compiled into the simple display driver,
// so they need to use "Unicode" characters which are just the CP1252 characters
// expanded to 16-bits.

#if defined(_UNICODE) && defined(__WINS__)
	const TText bf[] = {0x2554,0x2557,0x255A,0x255D,0x2550,0x2551,0x2550,0x2550,0x2551,0x2551};
	const TText bf1[] = {0x2554,0x2557,0x255A,0x255D,0x2550,0x2551,0x2591,0x2592,0x2591,0x2592};
	const TText *frame[2] ={ bf, bf1 };
#else	// K.K
	const TText *frame[2] = {
		_S("\xDA\xBF\xC0\xD9\xC4\xB3\xC4\xC4\xB3\xB3"),
		_S("\xC9\xBB\xC8\xBC\xCD\xBA\xB1\xB2\xB1\xB1")
		};
#endif	// K.K
	const TText *pF=frame[IsTop() ? 1 : 0];
#if defined(_UNICODE)	// K.K
	TText *pLT=GetCpFromOffset(iTextBuffer, iCurrentOffset,iCurrentSize);	// K.K
#else	// K.K
	TText *pLT=&iTextBuffer[Offset(iCurrentOffset,iCurrentSize)];
#endif	// K.K
	TText *pRT=&pLT[iViewSize.iWidth-1];
	ColorInformation *pLA=&iAttributeBuffer[Offset(iCurrentOffset,iCurrentSize)];
	ColorInformation *pRA=&pLA[iViewSize.iWidth-1];
	TextFill(pLT,iViewSize.iWidth,&pF[4]);
	for(TInt x=0;x<iViewSize.iWidth;x++)
		{
		(pLA+x)->iFg=BorderColor;
		(pLA+x)->iBg=WindowBgColor;
		}

	TInt i=iViewSize.iWidth-2;
	TInt s=iTitle.Length();
	if (s>i)
		s=i;
// #if defined(_UNICODE)	// K.K
//	i=((i-s)>>2);
// #else	// K.K
	i=((i-s)>>1);
// #endif	// K.K
	Mem::Copy(pLT+i+1,iTitle.Ptr(),s*sizeof(TText));

	*pLT=pF[0];
#if defined(_UNICODE)	// K.K
//	if (s&1) pLT[i+1+s] = 0x0020;	// K.K
  	FitInWidth(pLT,iCurrentSize.iWidth,iViewSize.iWidth-1,pF[1]);	// K.K
#else	// K.K
	*pRT=pF[1];
#endif	// K.K

	i=iViewSize.iHeight-2;
	s=(i*iCurrentOffset.iY)/(iCurrentSize.iHeight-2);
	TInt l=((i*i)/(iCurrentSize.iHeight-2))+1;

	while(i-->0)
		{
		pLT=&pLT[iCurrentSize.iWidth];
		pRT=&pRT[iCurrentSize.iWidth];
		pLA=&pLA[iCurrentSize.iWidth];
		pRA=&pRA[iCurrentSize.iWidth];

		*pLT=pF[5];
		pLA->iFg=BorderColor;
		pLA->iBg=WindowBgColor;

		if (!iHasScrollBars)
			{
#if defined(_UNICODE)	// K.K
			FitInWidth(pLT, iCurrentSize.iWidth, iViewSize.iWidth-1, pF[5]);	// K.K
#else	// K.K
			*pRT=pF[5];
#endif	// K.K
			pRA->iFg=BorderColor;
			pRA->iBg=WindowBgColor;
			}
		else
			{
#if defined(_UNICODE)	// K.K
			FitInWidth(pLT, iCurrentSize.iWidth, iViewSize.iWidth-1, pF[8]);	// K.K
#else	// K.K
			*pRT=pF[8];
#endif	// K.K
			pRA->iFg=BorderColor;
			pRA->iBg=WindowBgColor;
			if (s)
				--s;
			else if (l)
				{
#if defined(_UNICODE)	// K.K
				FitInWidth(pLT, iCurrentSize.iWidth, iViewSize.iWidth-1, pF[9]);	// K.K
#else	// K.K
				*pRT=pF[9];
#endif	// K.K
				pRA->iFg=BorderColor;
				pRA->iBg=WindowBgColor;
				--l;
				}
			}
		}
    
	pLT=&pLT[iCurrentSize.iWidth];
	pRT=&pRT[iCurrentSize.iWidth];
	pLA=&pLA[iCurrentSize.iWidth];
	pRA=&pRA[iCurrentSize.iWidth];

	for(i=0;i<iViewSize.iWidth;i++)
		{
		pLA->iFg=BorderColor;
		pLA->iBg=WindowBgColor;
		pLA++;
		}

    if (!iHasScrollBars)
		{
		TextFill(pLT,iViewSize.iWidth,&pF[4]);
		}
	else
		{
		i=iViewSize.iWidth-2;
		s=(i*iCurrentOffset.iX)/(iCurrentSize.iWidth-2);
		l=((i*i)/(iCurrentSize.iWidth-2))+1;
#if defined(_UNICODE)	// K.K
//		s >>= 1;	// K.K
//		l >>= 1;	// K.K
#endif	// K.K
		TextFill(&pLT[1],i,&pF[6]);
		TextFill(&pLT[s+1],l,&pF[7]);
		}

	*pLT=pF[2];
#if defined(_UNICODE)	// K.K
	FitInWidth(pLT, iCurrentSize.iWidth, iViewSize.iWidth-1, pF[3]);	// K.K
#else	// K.K
	*pRT=pF[3];
#endif	// K.K
    }

void CWsWindow::SetCursorHeight(TInt aPercentage)
//
// Set percentage height of cursor
//
	{

	aPercentage=Min(aPercentage,100);
	if(aPercentage==0)
		iCursorRequired=EFalse;
	else
		iCursorRequired=ETrue;

#if defined(_UNICODE) // K.K
	iCursor=0x005F;	// K.K
#else	// K.K
	iCursor=Cursors[aPercentage];
#endif	// K.K

	SetCursor();
	}

void CWsWindow::ClearToEndOfLine()
//
// Clear the window from the current cursor position to the end of the line.
//
	{
	TInt w=iCurrentSize.iWidth-iCursorPos.iX-1;
	RestoreEdges();
	TextFill(&iTextBuffer[Offset(iCursorPos,iCurrentSize)],w,_S(" "));
	Mem::Fill(&iAttributeBuffer[Offset(iCursorPos,iCurrentSize)],w*sizeof(ColorInformation),WindowBgColor);
	SaveEdges();
	SetFrame();
	__ASSERT_DEBUG(IsTop(),User::Panic(_L("Not front window"),0));
	if (IsInClippedTextArea(iCursorPos))
		Display();
	}

void CWsWindow::WriteDone()
//
// Called after a bunch of Write() calls
//
	{

	SetCursor();
	}

void CWsWindow::NewLine()
//
//	Do CR/LF on this window
//
	{

	if (!iWrapLock)
    	{
		LineFeed();
		CarriageReturn();
		}
	}

void CWsWindow::InformMouse(TPoint aPos)
//
// Called if mouse has been captured
//
    {
    SWsKey *pS=new SWsKey;

    if (pS)
    	{
        pS->iMousePos=aPos;
        pS->iType=EMouseClick;
    	iKQueue.AddLast(*pS);
    	}
    DrainAllReadRequests();
    }

void CWsWindow::DoMouseLeftButton()
//
// Called when the left button is pressed
//
	{

	if(iAllowResize && MousePos==iViewOrigin+iViewSize-TPoint(1,1))
		ResizeWithMouse=MousePos;
	else
		{
		TInt i=iViewSize.iWidth-2;
		TInt s=(i*iCurrentOffset.iX)/(iCurrentSize.iWidth-2);
		TInt l=((i*i)/(iCurrentSize.iWidth-2))+1;
		if(iHasScrollBars && MousePos.iY==iViewOrigin.iY+iViewSize.iHeight-1 && MousePos.iX>iViewOrigin.iX+s && MousePos.iX<iViewOrigin.iX+s+l+1)
			{
			ScrollWithMouse=TPoint(MousePos.iX,0);
			ScrollSpeed=(iCurrentSize.iWidth+iViewSize.iWidth/2-3)/(iViewSize.iWidth-2);
			}
		else
			{
			i=iViewSize.iHeight-2;
			s=(i*iCurrentOffset.iY)/(iCurrentSize.iHeight-2);
			l=((i*i)/(iCurrentSize.iHeight-2))+1;
			if(iHasScrollBars && MousePos.iX==iViewOrigin.iX+iViewSize.iWidth-1 && MousePos.iY>iViewOrigin.iY+s && MousePos.iY<iViewOrigin.iY+s+l+1)
				{
				ScrollWithMouse=TPoint(0,MousePos.iY);
				ScrollSpeed=(iCurrentSize.iHeight+iViewSize.iHeight/2-3)/(iViewSize.iHeight-2);
				}
			else MoveWithMouse=MousePos;
			}
		}
	if(iPointerEvents)
 		{
   		SWsKey *pS=new SWsKey;
   		if (pS)
   			{
			pS->iMousePos=MousePos;
			pS->iType=EMouseClick;
 			iKQueue.AddLast(*pS);
  			}
		DrainAllReadRequests();
		}
	}

void CWsWindow::QueueWindowKey(TKeyData &aKeystroke)
//
// Place keystroke in window's keyboard queue
//
	
	{
	SWsKey *pS=new SWsKey;
	if (pS)
		{
		pS->iKeyData=aKeystroke;
		pS->iType=EKeyPress;
		iKQueue.AddLast(*pS);
		}
	}

void CWsWindow::QueueRawEvent(TRawEvent& anEvent)
//
// Deliver a raw event to the raw event window
//
    {

    if (RawEventWindow)
		{
        RawEventWindow->QueueWindowRawEvent(anEvent);
		DrainAllReadRequests();
		}
    }

void CWsWindow::QueueWindowRawEvent(TRawEvent& anEvent)
//
// Place raw event in window's event queue
//
	{
	SWsKey *pS=new SWsKey;
	if (pS)
		{
		pS->iKeyData.iModifiers=0;
		pS->iKeyData.iApp=0;
		pS->iKeyData.iHandle=0;
		pS->iKeyData.iIsCaptureKey=EFalse;
		pS->iKeyData.iKeyCode=0;
		pS->iMousePos=TPoint(0,0);
        pS->iType=anEvent.Type();
		switch(anEvent.Type())
			{
		case TRawEvent::EPointerMove:
		case TRawEvent::EButton1Down:
		case TRawEvent::EButton1Up:
		case TRawEvent::EButton2Down:
		case TRawEvent::EButton2Up:
		case TRawEvent::EButton3Down:
		case TRawEvent::EButton3Up:
			pS->iMousePos=anEvent.Pos();
			pS->iPointerNumber = anEvent.PointerNumber();
			break;
		case TRawEvent::EKeyUp:
		case TRawEvent::EKeyDown:
			pS->iKeyData.iKeyCode=anEvent.ScanCode();
			break;
		default:
			break;
			}
		iKQueue.AddLast(*pS);
		}
	}

void CWsWindow::SetCursorPosAbs(const TPoint &aPosition)
//
// Place cursor at position specified
//
	{

	iCursorPos=aPosition;
	iCursorPos+=TPoint(1,1);
	if (iCursorPos.iX<1)
		iCursorPos.iX=1;
	if (iCursorPos.iX>=iCurrentSize.iWidth-1)
		iCursorPos.iX=iCurrentSize.iWidth-2;
	if (iCursorPos.iY<1)
		iCursorPos.iY=1;
	if (iCursorPos.iY>=iCurrentSize.iHeight-1)
		iCursorPos.iY=iCurrentSize.iHeight-2;
	SetCursor();
	}

void CWsWindow::SetCursorPosRel(const TPoint &aPosition)
//
// Place cursor at position relative to current position
//
	{

	TPoint p=iCursorPos+aPosition-TPoint(1,1);
	SetCursorPosAbs(p);
	}

TPoint CWsWindow::CursorPosition()
//
// Return current cursor position
//
	{

	return(iCursorPos-TPoint(1,1));
	}

void CWsWindow::ControlScrollBars(TBool anIndicator)
//
// Turn scroll bars on or off according to the value of anIndicator
//
	{

	iHasScrollBars=anIndicator;
	if (iTextBuffer)
		{
		SetFrame();
		Refresh();
		if (IsTop())
			ScrollWithMouse=TPoint(-1,-1);
		}
	}

void CWsWindow::ControlWrapLock(TBool anIndicator)
//
// Turn wrap lock on or off according to the value of anIndicator
//
	{

	iWrapLock=anIndicator;
	}

void CWsWindow::ControlPointerEvents(TBool anIndicator)
//
// Turn reporting of pointer events on or off according to the value of anIndicator
//
	{

	ResizeWithMouse=TPoint(-1,-1);
	ScrollWithMouse=TPoint(-1,-1);
	iPointerEvents=anIndicator;
	}

void CWsWindow::ControlScrollLock(TBool anIndicator)
//
// Turn scroll lock on or off according to the value of anIndicator
//
	{

	iScrollLock=anIndicator;
	}

void CWsWindow::ControlAllowResize(TBool anIndicator)
//
// Turn iAllowResize on or off
//
	{

	iAllowResize=anIndicator;
	ResizeWithMouse=TPoint(-1,-1);
	}

void CWsWindow::ControlOnTop(TBool anIndicator)
//
// Turn iOnTop on or off
//
	{

	iOnTop=anIndicator;
	if(iOnTop)
		iIsVisible=ETrue;
	if(iTextBuffer)
		MakeTopWindow();
	}

void CWsWindow::ControlVisibility(TBool anIndicator)
//
// Turn visibility on or off according to the value of anIndicator
//
	{
	if(!iOnTop)
		{
		iIsVisible=anIndicator;
		if (!iIsVisible && IsTop())
			RotateWindowsBackwards();		// make sure we have a visible window on top
		if (!iIsVisible&&iTextBuffer)
			Refresh();
		if (iTextBuffer&&iIsVisible)
			MakeTopWindow();
		}
	}

void CWsWindow::ControlCursorRequired(TBool anIndicator)
//
// Turn the text cursor on or off according to the value of anIndicator
//
	{

	iCursorRequired=anIndicator;
	SetCursor();
	}

void CWsWindow::ControlMaximised(TBool anIndicator)
//
// Maximise or minimise the window according to the value of anIndicator
//
	{
	ResizeWithMouse=TPoint(-1,-1);
	ScrollWithMouse=TPoint(-1,-1);
	if (anIndicator)
		{
		if (iViewSize==iMaximumSize && iViewOrigin==iMaximumOrigin)
			return;
		RestoreEdges();
		iMinimumSize=iViewSize;
		iMinimumOrigin=iViewOrigin;
		iViewSize=iMaximumSize;
		iViewOrigin=iMaximumOrigin;
		TSize s=iCurrentSize-iViewSize;
		if (iCurrentOffset.iX>s.iWidth)
			iCurrentOffset.iX=s.iWidth;
		if (iCurrentOffset.iY>s.iHeight)
			iCurrentOffset.iY=s.iHeight;
		SaveEdges();
		SetFrame();
		SetClip();
		Refresh();
		}
	else
		{
		if (iViewSize==iMaximumSize && iViewOrigin==iMaximumOrigin)
			{
			RestoreEdges();
			iViewSize=iMinimumSize;
			iViewOrigin=iMinimumOrigin;
			SaveEdges();
			SetFrame();
			SetClip();
			Refresh();
			}
		}
	}

void CWsWindow::ControlNewLineMode(TBool anIndicator)
//
// Set the newline mode
//
	{

	iNewLineMode=anIndicator;
	}

void CWsWindow::ControlRawEventMode(TBool anIndicator)
//
// Set the raw event mode
//
	{

    if (anIndicator)
        {
        if (!RawEventWindow)
            RawEventWindow=this;
        }
    else
        {
        if (RawEventWindow==this)
            RawEventWindow=NULL;
        }
	}

TBool CWsWindow::RawEventMode()
//
// Report whether in raw event mode
//
    {

    return(RawEventWindow!=NULL);
    }

TBool CWsWindow::EnqueReadRequest(const RMessage2 &aMessage)
//
// Accept read request on this window
//
	{

	if (!iReadIsValid)
		{
		iReadRequest=aMessage;
		iReadIsValid=ETrue;
		DrainAllReadRequests();
		SetCursor();
		return(ETrue);
		}
	return(EFalse);
	}

void CWsWindow::DequeReadRequest()
	{
	if (iReadIsValid)
		{
		iReadIsValid=EFalse;
		iReadRequest.Complete(KErrCancel);
		}
	}

void CWsWindow::DrainReadRequest()
//
// Drain satisfied read requests
//
	{

	if (iReadIsValid && !(iKQueue.IsEmpty()))
		{
		RMessage2& m=iReadRequest;
		SConsoleKey k;
		SWsKey *pS=iKQueue.First();
		k.iCode=(TKeyCode)pS->iKeyData.iKeyCode;
		
		k.iModifiers=KeyTranslator->GetModifierState();

		k.iPointerNumber = pS->iPointerNumber;
        k.iType=pS->iType;
        k.iMousePos=pS->iMousePos;

		TPckgC<SConsoleKey> keystroke(k);
		m.WriteL(0,keystroke);
		iKQueue.Remove(*pS);
		delete pS;
		iReadIsValid=EFalse;	// Do this before message completion to prevent message flow control problems
		m.Complete(KErrNone);
		}
	}

void CWsWindow::CreateL(const TSize &aSize)
//
//	Default the new control block and add to queue.
//
	{
	iNumber=NewNumberL();
 	iCurrentOffset=TPoint(0,0);
	iCurrentSize=aSize+TSize(2,2); // Allow for window border
	if (iCurrentSize.iWidth==KConsFullScreen+2)
		iCurrentSize.iWidth=ScreenSize.iWidth;
	if (iCurrentSize.iHeight==KConsFullScreen+2)
		iCurrentSize.iHeight=ScreenSize.iHeight;
	if (iCurrentSize.iWidth>ScreenSize.iWidth)
		User::Leave(EWindowTooWide);
	if (iCurrentSize.iWidth<3)
		User::Leave(EWindowTooThin);
	if (iCurrentSize.iHeight>ScreenSize.iHeight)
		User::Leave(EWindowTooHigh);
	if (iCurrentSize.iHeight<3)
		User::Leave(EWindowTooShort);
	iTextBuffer=(TText *)User::Alloc(sizeof(TText)*iCurrentSize.iWidth*iCurrentSize.iHeight);
	if (!iTextBuffer)
		User::Leave(EWindowOutOfMemory);
	iAttributeBuffer=(ColorInformation *)User::Alloc(iCurrentSize.iWidth*iCurrentSize.iHeight*sizeof(ColorInformation));
	if (!iAttributeBuffer)
		User::Leave(EWindowOutOfMemory);
	iFgColor=IndexOf[ETextAttributeNormal];
	iBgColor=IndexOf[ETextAttributeNormal+1];
	WQueue.AddLast(*this);
	SetView();
	SetFull();
	SetClip();
	Clear();
	if (iIsVisible)
		MakeTopWindow();
	}

