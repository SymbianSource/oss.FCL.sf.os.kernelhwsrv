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
// e32\ewsrv\co_cli.cpp
// 
//

#include "ws_std.h"

TInt RConsole::Connect()
	{
	return CreateSession(KE32WindowServer,Version(),KMessageSlots);
	}

EXPORT_C TInt RConsole::Create()
//
// Connect with the window server if connection not already made,
// then create a default console window without displaying it
//
	{

	if (Handle()==KNullHandle)
		{
		TInt r = Connect();
		if (r!=KErrNone)
			return r;
		}
	return SendReceive(CWsSession::EConsoleCreate, TIpcArgs());
	}

EXPORT_C TInt RConsole::Init(const TDesC &aName,const TSize &aSize)
//
// Connect with the window server if connection not already made,
// then open/display a console window on with the specified title.
//
	{

	if (Handle()==KNullHandle)
		{
		TInt r = Connect();
		if (r!=KErrNone)
			return r;
		}
	TPckgC<TSize> size(aSize);
	return SendReceive(CWsSession::EConsoleSet, TIpcArgs(&aName, &size));
	}

EXPORT_C TInt RConsole::SetTitle(const TDesC &aName)
//
// Change the title of the window
//
	{

	return SendReceive(CWsSession::EConsoleSetTitle, TIpcArgs(&aName));
	}

EXPORT_C TInt RConsole::SetSize(const TSize &aSize)
//
// Change the underlying size of the window
//
	{

	TPckgC<TSize> size(aSize);
	return SendReceive(CWsSession::EConsoleSetSize, TIpcArgs(&size));
	}

EXPORT_C TInt RConsole::Size(TSize &aSize) const
//
// Read the current window size
//
	{

	TPckg<TSize> size(aSize);
	return SendReceive(CWsSession::EConsoleSize, TIpcArgs( (TDes8*)&size ));
	}

EXPORT_C TInt RConsole::ScreenSize(TSize &aSize) const
//
// Read the screen size in characters
//
	{
	
	TPckg<TSize> size(aSize);
	return SendReceive(CWsSession::EConsoleScreenSize, TIpcArgs( (TDes8*)&size ));
	}

EXPORT_C TVersion RConsole::Version()
//
// Return the client side version number.
//
	{

	return TVersion(KW32MajorVersionNumber,KW32MinorVersionNumber,KE32BuildVersionNumber);
	}

EXPORT_C TInt RConsole::Write(const TDesC &aDes)
//
// Write to the console.
//
	{

	return SendReceive(CWsSession::EConsoleWrite, TIpcArgs(&aDes));
	}

EXPORT_C TInt RConsole::ClearScreen()
//
// Clear window
//
	{

	return SendReceive(CWsSession::EConsoleClearScreen, TIpcArgs());
	}

EXPORT_C TInt RConsole::ClearToEndOfLine()
//
// Clear window from current cursor position to the end of the line
//
	{

	return SendReceive(CWsSession::EConsoleClearToEndOfLine, TIpcArgs());
	}

EXPORT_C TInt RConsole::Destroy()
//
// Remove and close down the window
//
	{

	return SendReceive(CWsSession::EConsoleDestroy, TIpcArgs());
	}

EXPORT_C TInt RConsole::SetWindowPosAbs(const TPoint &aPosition)
//
// Position the window
//
	{
	
	TPckgC<TPoint> point(aPosition);
	return SendReceive(CWsSession::EConsoleSetWindowPosAbs, TIpcArgs(&point));
	}

EXPORT_C TInt RConsole::SetCursorHeight(TInt aPercentage)
//
// Set the percentage height of the cursor
//
	{

	return SendReceive(CWsSession::EConsoleSetCursorHeight, TIpcArgs(aPercentage));
	}
		
EXPORT_C TInt RConsole::SetCursorPosAbs(const TPoint &aPosition)
//
// Position the cursor in the window buffer
//
	{

	TPckgC<TPoint> point(aPosition);
	return SendReceive(CWsSession::EConsoleSetCursorPosAbs, TIpcArgs(&point));
	}

EXPORT_C TInt RConsole::SetCursorPosRel(const TPoint &aVector)
//
// Position the cursor in the window buffer
//
	{

	TPckg<TPoint> point(aVector);
	return SendReceive(CWsSession::EConsoleSetCursorPosRel, TIpcArgs(&point));
	}

EXPORT_C TInt RConsole::CursorPos(TPoint &aPosition) const
//
// Read current cursor position relative to the window
//
	{

	TPckg<TPoint> point(aPosition);
	return SendReceive(CWsSession::EConsoleCursorPos, TIpcArgs( (TDes8*)&point ));
	}

EXPORT_C TInt RConsole::Control(const TDesC &aDes)
//
// Control window properties
//
	{

	return SendReceive(CWsSession::EConsoleControl, TIpcArgs(&aDes));
	}

EXPORT_C TInt RConsole::Read(TConsoleKey &aKeystroke)
//
// Synchronous get keystroke from window
//
	{

	return SendReceive(CWsSession::EConsoleRead, TIpcArgs( (TDes8*)&aKeystroke ));
	}

EXPORT_C void RConsole::Read(TConsoleKey &aKeystroke,TRequestStatus &aStatus)
//
// Asynchronous get keystroke from window
//
	{

	SendReceive(CWsSession::EConsoleRead, TIpcArgs( (TDes8*)&aKeystroke ), aStatus);
	}

EXPORT_C TInt RConsole::ReadCancel()
//
// Cancel asynchronous read request
//
	{

	return SendReceive(CWsSession::EConsoleReadCancel, TIpcArgs());
	}

EXPORT_C TInt RConsole::SetMode(TVideoMode aMode)
//
//
//
	{
	
	if (Handle()==KNullHandle)
		{
		TInt r = Connect();
		if (r!=KErrNone)
			return r;
		}
	return SendReceive(CWsSession::EConsoleSetMode, TIpcArgs(aMode));
	}

EXPORT_C void RConsole::SetPaletteEntry(TUint aIndex,TUint8 aRed,TUint8 aGreen,TUint8 aBlue)
//
//
//
	{
	
	if (Handle()==KNullHandle)
		{
		TInt r = Connect();
		if (r!=KErrNone)
			return;
		}
	SendReceive(CWsSession::EConsoleSetPaletteEntry, TIpcArgs(aIndex,aRed,aGreen,aBlue));
	}

EXPORT_C void RConsole::GetPaletteEntry(TUint aIndex,TUint8 &aRed,TUint8 &aGreen,TUint8 &aBlue)
//
//
//
	{
	
	if (Handle()==KNullHandle)
		{
		TInt r = Connect();
		if (r!=KErrNone)
			return;
		}
	TPckg<TUint8> r(aRed);
	TPckg<TUint8> g(aGreen);
	TPckg<TUint8> b(aBlue);
	SendReceive(CWsSession::EConsoleGetPaletteEntry, TIpcArgs(aIndex, &r, &g, &b));
	}

EXPORT_C void RConsole::SetTextColors(TUint aFgColor,TUint aBgColor)
//
//
//
	{
	
	if (Handle()==KNullHandle)
		{
		TInt r = Connect();
		if (r!=KErrNone)
			return;
		}
	SendReceive(CWsSession::EConsoleSetTextColors, TIpcArgs(aFgColor, aBgColor));
	}

EXPORT_C void RConsole::SetUIColors(TUint aWindowBgColor,TUint aBorderColor,TUint aScreenColor)
//
//
//
	{
	
	if (Handle()==KNullHandle)
		{
		TInt r = Connect();
		if (r!=KErrNone)
			return;
		}
	SendReceive(CWsSession::EConsoleSetUIColors, TIpcArgs(aWindowBgColor, aBorderColor, aScreenColor));
	}

EXPORT_C void RConsole::SetTextAttribute(TTextAttribute aAttr)
//
//
//
	{

	if (Handle()==KNullHandle)
		{
		TInt r = Connect();
		if (r!=KErrNone)
			return;
		}
	SendReceive(CWsSession::EConsoleSetTextAttribute, TIpcArgs(aAttr));
	}
