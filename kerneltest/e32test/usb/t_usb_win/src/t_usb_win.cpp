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
// t_usb_win.cpp : Defines the class behaviors for the application.
//

#include "stdafx.h"
#include "atlbase.h"
#include "t_usb_win.h"
#include "t_usb_winDlg.h"
#include "global.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif



// === Global Defines ===

char gScriptFileName[100];
char gManufacturerName[100];
char gProductName[100];
char gSerialNumber[100];
char gLogfileName[100];

BOOL gVerboseMode = FALSE;
BOOL gKeepOpen = FALSE;
BOOL gAbort = FALSE;

FILE *logStream = NULL;
int gTransferMode = 0;
CT_usb_winDlg* gMainWindow;
int gReadWriteTimeOut = READWRITE_TIMEOUT;

/////////////////////////////////////////////////////////////////////////////
// CT_usb_winApp

BEGIN_MESSAGE_MAP(CT_usb_winApp, CWinApp)
	//{{AFX_MSG_MAP(CT_usb_winApp)
		// NOTE - the ClassWizard will add and remove mapping macros here.
		//    DO NOT EDIT what you see in these blocks of generated code!
	//}}AFX_MSG
	ON_COMMAND(ID_HELP, CWinApp::OnHelp)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CT_usb_winApp construction

CT_usb_winApp::CT_usb_winApp()
{
	// TODO: add construction code here,
	// Place all significant initialization in InitInstance
}

/////////////////////////////////////////////////////////////////////////////
// The one and only CT_usb_winApp object

CT_usb_winApp theApp;


//
// Process the command line arguments, printing a helpful message
// if none are supplied.
//
static int ProcessCmdLine()
	{
	USES_CONVERSION;

	LPWSTR cmdLine = GetCommandLineW();

	gScriptFileName[0] = '\0';
	gManufacturerName[0] = '\0';
	gProductName[0] = '\0';
	gSerialNumber[0] = '\0';
	gLogfileName[0] = '\0';

	int argc;
	LPWSTR * argv = CommandLineToArgvW(cmdLine, & argc);

	for (int i = 1; i < argc; i++)
		{
		if ((argv[i][0] == '-') || (argv[i][0] == '/'))
			{
			switch (argv[i][1])
				{
			case 'A':
			case 'a':
			case 'L':
			case 'l':
				if (argv[i][2] != '=')
					return -1;
				strcpy (gLogfileName,W2A (&argv[i][3]));
				if (argv[i][1] == 'L' || argv[i][1] == 'l')
					logStream = fopen (gLogfileName, "w");
				else
					logStream = fopen (gLogfileName, "a+");
				if (logStream == NULL)
					return -1;
				break;

			case 'M':
			case 'm':
				if (argv[i][2] != '=')
					return -1;
				strcpy (gManufacturerName,W2A (&argv[i][3]));
				break;

			case 'P':
			case 'p':
				if (argv[i][2] != '=')
					return -1;
				strcpy (gProductName,W2A (&argv[i][3]));
				break;

			case 'S':
			case 's':
				if (argv[i][2] != '=')
					return -1;
				strcpy (gSerialNumber,W2A (&argv[i][3]));
				break;

			case 'T':
			case 't':
				if (argv[i][2] != '=')
					return -1;
				gReadWriteTimeOut = atoi(W2A (&argv[i][3]));
				if (gReadWriteTimeOut <= 0)
					return -1;
				break;

			case 'K':
			case 'k':
				gKeepOpen = true;
				break;

			case 'V':
			case 'v':
				gVerboseMode = true;
				break;

			default:
				return -1;
				}
			}
		else
			{
			strcpy (gScriptFileName,W2A (argv[i]));
			}
		}
	return 0;
	}

/////////////////////////////////////////////////////////////////////////////
// CT_usb_winApp initialization

BOOL CT_usb_winApp::InitInstance()
{
	BOOL retCode = FALSE; 

	// Standard initialization
	// If you are not using these features and wish to reduce the size
	//  of your final executable, you should remove from the following
	//  the specific initialization routines you do not need.

#ifdef _AFXDLL
	Enable3dControls();			// Call this when using MFC in a shared DLL
#else
	Enable3dControlsStatic();	// Call this when linking to MFC statically
#endif

	SetDialogBkColor( RGB(255,255,255),RGB(0, 0, 0));        // Set dialog background color to white

	if (ProcessCmdLine() < 0)
	{
		TRACE0("Invalid Command Line Parameter\n");
		return -1;

	}

	CT_usb_winDlg dlg;
	m_pMainWnd = &dlg;
	gMainWindow = &dlg;
	int nResponse = dlg.DoModal();

	return retCode;
}
