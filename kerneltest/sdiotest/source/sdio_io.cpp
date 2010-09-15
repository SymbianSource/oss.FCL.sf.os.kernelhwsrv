// Copyright (c) 2003-2009 Nokia Corporation and/or its subsidiary(-ies).
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
// LDD for testing SDIO functions
// 
//

#include "sdio_io.h"

/** Text to replace any overflow in logging */
_LIT(KEllipses, "...");

#if defined(_UNICODE)
void CIOBase::TIOOverflowHandler::Overflow(TDes16 &aDes)
	{
	// Replace the last three characters with ellipses
	aDes.RightTPtr(KEllipses().Length()).Copy(KEllipses);
	}
#else
void CIOBase::TIOOverflowHandler::Overflow(TDes8 &aDes)
	{
	// Replace the last three characters with ellipses
	aDes.RightTPtr(KEllipses().Length()).Copy(KEllipses);
	}
#endif

//
// CIOBase
//
void CIOBase::Heading(TRefByValue<const TDesC> aFmt,...)
	{
	FORMAT_TEXT(aFmt);
	DoHeading();
	}

void CIOBase::Instructions(TBool topLine, TRefByValue<const TDesC> aFmt,...)
	{
	FORMAT_TEXT(aFmt);
	DoInstructions(topLine);
	}

void CIOBase::Printf(TRefByValue<const TDesC> aFmt,...)
	{
	FORMAT_TEXT(aFmt);
	DoPrintf();
	}

void CIOBase::FormatText(TRefByValue<const TDesC> aFmt, VA_LIST aList)
	{
	iText.Zero();
	iText.AppendFormatList(aFmt, aList, &iOverflowHandler);
	}

//
// CIOConsole
//
CIOConsole::~CIOConsole()
	{
	iDisplay.Destroy();
	}
	
void CIOConsole::CreateL(TPtrC aName)
	{
	iDisplay.CreateL(aName);
	}

void CIOConsole::DoHeading()
	{
	iDisplay.Heading(iText);
	}

void CIOConsole::DoInstructions(TBool topLine)
	{
	iDisplay.Instructions(topLine, iText);
	}

void CIOConsole::DoPrintf()
	{
	iDisplay.Printf(iText);
	}

void CIOConsole::ReportError(TPtrC errText, TInt anErr)
	{
	iDisplay.ReportError(errText, anErr);
	}

void CIOConsole::CurserToDataStart()
	{
	iDisplay.CurserToDataStart();
	}

TKeyCode CIOConsole::Getch()
	{
	return iDisplay.Getch();
	}

void CIOConsole::ClearScreen()
	{
	iDisplay.ClearScreen();
	}

//
// CIORDebug
//
void CIORDebug::CreateL(TPtrC aName)
	{
	RDebug::RawPrint(aName);
	}

void CIORDebug::DoHeading()
	{
	RDebug::RawPrint(iText);
	}

void CIORDebug::DoPrintf()
	{
	RDebug::RawPrint(iText);
	}

void CIORDebug::ReportError(TPtrC errText, TInt anErr)
	{
	RDebug::RawPrint(errText);
	RDebug::Printf("Error Code: %d", anErr);
	}
	
void CIORDebug::ClearScreen()
	{
	RDebug::Printf("\n\n");
	}
