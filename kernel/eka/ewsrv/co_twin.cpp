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
// e32\ewsrv\co_twin.cpp
// 
//

#include "ws_std.h"

extern "C"
EXPORT_C TAny* NewConsole()
//
// Create a new console window.
//
	{
	return new CConsoleTextWin;
	}

CConsoleTextWin *CConsoleTextWin::NewL(const TDesC &aTitle,TSize aSize)
//
// Create a new console window. Leave on any error.
//
	{

	CConsoleTextWin *pC=new(ELeave) CConsoleTextWin;
	User::LeaveIfError(pC->iConsole.Init(aTitle,aSize));
	return(pC);
	}

CConsoleTextWin::CConsoleTextWin()
//
// Constrcutor
//
	{}

CConsoleTextWin::~CConsoleTextWin()
//
// Destructor
//
	{

	iConsole.Close();
	}

TInt CConsoleTextWin::Create(const TDesC &aTitle,TSize aSize)
//
// Create a new console window.
//
	{

	TInt r=iConsole.Init(aTitle,aSize);
	if (r==KErrNone)
		{
		r=iConsole.Control(_L("+Maximize +NewLine -Lock -Wrap"));
		}
	return(r);
	}

void CConsoleTextWin::Read(TRequestStatus &aStatus)
//
// Asynchronous get keystroke from window
//
	{

	iConsole.Read(iKey,aStatus);
	}


void ConsServerCheck(TInt aResult, TInt aLine)
	{
	if(aResult!=KErrNone)
		{
#ifdef _DEBUG
		RDebug::Printf("EConsServerFailed with %d at line %d",aResult,aLine);
#endif
		(void)aLine;
		Panic(EConsServerFailed);
		}
	}


void CConsoleTextWin::ReadCancel()
//
// Cancel asynchronous read request
//
	{

	TInt r=iConsole.ReadCancel();
	ConsServerCheck(r,__LINE__);
	}

void CConsoleTextWin::Write(const TDesC &aDes)
//
// Write to the console.
//
	{

	TInt r=iConsole.Write(aDes);
	ConsServerCheck(r,__LINE__);
	}

TPoint CConsoleTextWin::CursorPos() const
//
// Read current cursor position relative to the window
//
	{

	TPoint p;
	TInt r=iConsole.CursorPos(p);
	ConsServerCheck(r,__LINE__);
	return(p);
	}

void CConsoleTextWin::SetCursorPosAbs(const TPoint &aPosition)
//
// Position the cursor in the window buffer
//
	{

	TInt r=iConsole.SetCursorPosAbs(aPosition);
	ConsServerCheck(r,__LINE__);
	}

void CConsoleTextWin::SetCursorPosRel(const TPoint &aVector)
//
// Position the cursor in the window buffer
//
	{

	TInt r=iConsole.SetCursorPosRel(aVector);
	ConsServerCheck(r,__LINE__);
	}

void CConsoleTextWin::SetCursorHeight(TInt aPercentage)
//
// Set the percentage height of the cursor
//
	{

	TInt r=iConsole.SetCursorHeight(aPercentage);
	ConsServerCheck(r,__LINE__);
	}
		
void CConsoleTextWin::SetTitle(const TDesC &aTitle)
//
// Set the console window title
//
	{

	TInt r=iConsole.SetTitle(aTitle);
	ConsServerCheck(r,__LINE__);
	}

void CConsoleTextWin::ClearScreen()
//
// Clear screen
//
	{

	TInt r=iConsole.ClearScreen();
	ConsServerCheck(r,__LINE__);
	}

void CConsoleTextWin::ClearToEndOfLine()
//
// Clear window from current cursor position to the end of the line
//
	{

	TInt r=iConsole.ClearToEndOfLine();
	ConsServerCheck(r,__LINE__);
	}

TSize CConsoleTextWin::ScreenSize() const
//
// Return the current screen size
//
	{

	TSize s;
	TInt r=iConsole.Size(s);
	ConsServerCheck(r,__LINE__);
	return(s);
	}

TKeyCode CConsoleTextWin::KeyCode() const
//
// Return the current keycode
//
	{

	return(iKey.Code());
	}

TUint CConsoleTextWin::KeyModifiers() const
//
// Return the current key modifiers
//
	{

	return(iKey.Modifiers());
	}

void CConsoleTextWin::SetTextAttribute(TTextAttribute anAttribute)
//
// Set text attribute
//
	{

	iConsole.SetTextAttribute(anAttribute);
	}

RConsole &CConsoleTextWin::Console()
//
// Return the console object.
//
	{

	return(iConsole);
	}
