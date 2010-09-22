// Copyright (c) 2001-2010 Nokia Corporation and/or its subsidiary(-ies).
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
// Perl.cpp: allows running of a perl script and waiting for its completion.
//

#include "stdafx.h"

#include "usbio.h"											// USBIO Dev Kit

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif

// define the CreateProcess strings for Perl
#define APPNAME "C:\\Apps\\actperl\\bin\\perl.exe"
#define APPTITLE "Perl Script"

#define WAIT_SLEEP 1000			// checks for for perl script completion every second		
#define EXIT_WAIT 900			// exits if not complete within 15 minutes


DWORD PerlScript(char * scriptName)
{
    STARTUPINFO si;
    PROCESS_INFORMATION pi;
	DWORD exitCode = STILL_ACTIVE;

    ZeroMemory( &si, sizeof(si) );
    si.cb = sizeof(si);
    ZeroMemory( &pi, sizeof(pi) );

	if (!CreateProcess (APPNAME,scriptName,NULL,NULL,FALSE,0,NULL,NULL,&si,&pi))
	return USBIO_ERR_FAILED;
	for (int i = 0; i < EXIT_WAIT && exitCode == STILL_ACTIVE; i++)
		{
		Sleep (WAIT_SLEEP);
		GetExitCodeProcess(pi.hProcess,(LPDWORD)&exitCode);
		}
	
	// Force an unclean process termination only if necessary
	if (exitCode == STILL_ACTIVE)
		{
		TerminateProcess(pi.hProcess,0);
		return USBIO_ERR_TIMEOUT;
		}

	return exitCode;
}

