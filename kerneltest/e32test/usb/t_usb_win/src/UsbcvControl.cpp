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
// UsbcvControl.cpp: implementation of the UsbcvControl class.
//

#include "stdafx.h"
#include "t_usb_win.h"
#include "UsbcvControl.h"

#include "usbio.h"											// USBIO Dev Kit

#include "global.h"

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif

extern void PrintOut(BOOL screenFlag, BOOL logFlag, BOOL timeFlag, const char *format, ...);

extern BOOL gAbort;

// define the CreateProcess strings for the Usbcv test app
#define APPNAME "C:\\Program Files\\USB-IF Test Suite\\USBCommandVerifier\\UsbCV13.exe"
#define APPTITLE "Automated USBCV"
#define CMDLINE " /title \"Automated USBCV\" /drv hcdriver"
#define CURDIR "C:\\Program Files\\USB-IF Test Suite\\USBCommandVerifier\\lib"

#define DIALOG_CLASS "#32770"
#define BUTTON_CLASS "Button"
#define STATIC_CLASS "Static"

#define ID_COMPLIANCE 0x3EC
#define ID_RUN 0x3F5
#define ID_EXIT 0x1

#define DEVLIST_TITLE ""
#define ID_OKBUTTON 0x3E8
#define ID_TESTLIST 0x3E9
#define ID_DEVLIST 0x3EA
#define ID_TEXTBOX 0x3EB
#define ID_CONFIRM 0x3EC
#define ID_ABORT 0x3ED

#define TESTRESULT_TITLE "Test Results"
#define ID_RESULT_TEXT 0xFFFF
#define ID_RESULT_OK 0x2

#define BUFFER_SIZE 256
#define VIDPID_SIZE	16
#define TEST_PASSED "Passed"
#define TEST_FAILED "Failed"

// Wait sleep in milliseconds, other waits as repetitions of the sleep time
#define WAIT_SLEEP 250					
#define CREATE_WAIT 40
#define SELECT_WAIT 20
#define CONTINUE_WAIT 120
#define RESULT_WAIT	1200
#define EXIT_WAIT 40

#define USBMS_DEVICE_REQUEST_CONTINUES	6

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

UsbcvControl::UsbcvControl()
{

    ZeroMemory( &si, sizeof(si) );
    si.cb = sizeof(si);
    ZeroMemory( &pi, sizeof(pi) );

}

UsbcvControl::~UsbcvControl()
{
	DWORD exitCode;
	
	if (GetExitCodeProcess(pi.hProcess,(LPDWORD)&exitCode))
		{
		if (exitCode == STILL_ACTIVE)
			{
			SendMessage (mainWindow,WM_COMMAND,MAKEWPARAM(ID_EXIT,BN_CLICKED),(LPARAM)exitButton);
			}
		int i = 0;
		while (exitCode == STILL_ACTIVE && i++ < EXIT_WAIT)
			{
			Sleep (WAIT_SLEEP);
			GetExitCodeProcess(pi.hProcess,(LPDWORD)&exitCode);
			}
		// Force an unclean process termination only if necessary
		if (exitCode == STILL_ACTIVE)
			{
			TerminateProcess(pi.hProcess,0);
			}
		}	
}

DWORD UsbcvControl::Create()
{
	mainWindow = NULL;
	exitButton = NULL;

	if (!CreateProcess (APPNAME,CMDLINE,NULL,NULL,FALSE,0,NULL,CURDIR,&si,&pi))
		return USBIO_ERR_FAILED;

	HWND complianceButton;

	for (int i=0; i < CREATE_WAIT && !mainWindow; i++)
		{
		mainWindow = FindWindow(DIALOG_CLASS,APPTITLE);
		if (mainWindow)
			{
			complianceButton = GetDlgItem(mainWindow,ID_COMPLIANCE);
			runButton = GetDlgItem(mainWindow,ID_RUN);
			exitButton = GetDlgItem(mainWindow,ID_EXIT);	
			if (!complianceButton || !runButton || !exitButton)
				{
				mainWindow = NULL;
				}
			}
		if (!mainWindow)
			Sleep(WAIT_SLEEP);
		}

	if (mainWindow)
		{
		SendMessage (mainWindow,WM_COMMAND,MAKEWPARAM(ID_COMPLIANCE,BN_CLICKED),(LPARAM)complianceButton);

		return USBIO_ERR_SUCCESS;
		}
	else
		{
		return USBIO_ERR_FAILED;
		}
}

DWORD UsbcvControl::TestAllDevices(USHORT aVendorId, USHORT aProductId1, USHORT aProductId2)
{
	DWORD dwRC = USBIO_ERR_SUCCESS;
	HWND testList;
	int chapter9_TestIndex = -1;
	int MSC_TestIndex = -1;
	LRESULT lResult;

	testList = GetDlgItem(mainWindow,ID_TESTLIST);

	LRESULT itemCount = SendMessage (testList,LB_GETCOUNT,0,0);
	if (!itemCount)
		return USBIO_ERR_FAILED;

	char strBuffer[BUFFER_SIZE];
	for (int i = 0; i < itemCount; i++)
		{
		LRESULT lResult = SendMessage (testList,LB_GETTEXT,(WPARAM)i,(LPARAM)&strBuffer);
		if (lResult > 0)
			{
			if (strstr (strBuffer,"Chapter 9 Tests"))
				chapter9_TestIndex = i;
			if (strstr (strBuffer,"MSC Tests"))
				MSC_TestIndex = i;
			}
		}

		
	if (chapter9_TestIndex < 0 || MSC_TestIndex < 0)
		return USBIO_ERR_FAILED;


	int ch9DeviceIndex = 0;
	int numCh9Devices = 1;
	int mscDeviceIndex = 0;
	int numMscDevices = 1;
	int testResult;
	bool msDevice = false;
	while (numCh9Devices > 0 && dwRC == USBIO_ERR_SUCCESS)
		{
		// select the chapter 9 tests
		lResult = SendMessage (testList,LB_SETCURSEL,(WPARAM)chapter9_TestIndex,0);
		lResult = SendMessage (mainWindow,WM_COMMAND,MAKEWPARAM(ID_TESTLIST,LBN_SELCHANGE),(LPARAM)testList);

		// then run it
		lResult = SendMessage (mainWindow,WM_COMMAND,MAKEWPARAM(ID_RUN,BN_CLICKED),(LPARAM)runButton);

		dwRC = SelectDevice (aVendorId,aProductId1,aProductId2,&ch9DeviceIndex,&numCh9Devices,&msDevice);
		if (ch9DeviceIndex >= 0 && dwRC == USBIO_ERR_SUCCESS)
			{

			dwRC = WaitForTestResult(&testResult);
			if (dwRC == USBIO_ERR_SUCCESS)
				{
				if (!testResult)
					{
					gAbort = TRUE;
					}
				PRINT_TIME "USBCV Chapter 9 Test Complete - %s  ",testResult ? "Passed" : "Failed");
				ch9DeviceIndex++;
				}
			else
				{
				PRINT_ALWAYS "Error in Running USBCV\n");
				return dwRC;
				}
			}

		if (msDevice && numMscDevices > 0 && dwRC == USBIO_ERR_SUCCESS)
			{
			// select the mass storage tests
			lResult = SendMessage (testList,LB_SETCURSEL,(WPARAM)MSC_TestIndex,0);
			lResult = SendMessage (mainWindow,WM_COMMAND,MAKEWPARAM(ID_TESTLIST,LBN_SELCHANGE),(LPARAM)testList);

			// then run it
			lResult = SendMessage (mainWindow,WM_COMMAND,MAKEWPARAM(ID_RUN,BN_CLICKED),(LPARAM)runButton);

			if (dwRC == USBIO_ERR_SUCCESS)
				dwRC = SelectDevice (aVendorId,aProductId1,aProductId2,&mscDeviceIndex,&numMscDevices,&msDevice);
			if (mscDeviceIndex >= 0 && dwRC == USBIO_ERR_SUCCESS)
				{
				dwRC = ContinueOk ("The following test might destroy",ID_CONFIRM);
				for (int i = 0; i < USBMS_DEVICE_REQUEST_CONTINUES; i++)
					{
					if (dwRC == USBIO_ERR_SUCCESS)
						dwRC = ContinueOk ("Processing device request",ID_OKBUTTON);
					}
				if (dwRC == USBIO_ERR_SUCCESS)
					dwRC = ContinueOk ("Disconnect and power off",ID_CONFIRM);

				if (dwRC == USBIO_ERR_SUCCESS)
					{
					dwRC = WaitForTestResult(&testResult);
					if (dwRC == USBIO_ERR_SUCCESS)
						{
						if (!testResult)
							{
							gAbort = TRUE;
							}
						PRINT_TIME "USBCV Mass Storage Class Test Complete - %s  ",testResult ? "Passed" : "Failed");
						mscDeviceIndex++;
						}
					else
						{
						PRINT_ALWAYS "Error in Running USBCV\n");
						return dwRC;
						}
					}
				}
			}
		}

	return dwRC;
}

DWORD UsbcvControl::ContinueOk (char * choiceText, int buttonId)
{
	HWND choiceDialog = NULL;
	HWND okButton;
	char strBuffer[BUFFER_SIZE];

	for (int i=0; i < CONTINUE_WAIT && !choiceDialog; i++)
		{
		choiceDialog = FindWindow(DIALOG_CLASS,"");
		if (choiceDialog)
			{
			UINT textSize = GetDlgItemText (choiceDialog,ID_TEXTBOX,strBuffer,BUFFER_SIZE);
			okButton = GetDlgItem(choiceDialog,buttonId);
			if (!textSize || !okButton || !strstr (strBuffer,choiceText))
				{
				choiceDialog = NULL;
				}
			}
		if (!choiceDialog)
			Sleep(WAIT_SLEEP);
		}

	if (!choiceDialog)
		return USBIO_ERR_FAILED;

	SendMessage (choiceDialog,WM_COMMAND,MAKEWPARAM(buttonId,BN_CLICKED),(LPARAM)okButton);

	return USBIO_ERR_SUCCESS;

}

DWORD UsbcvControl::SelectDevice(USHORT aVendorId, USHORT aProductId1, USHORT aProductId2, int * deviceIndex, int * numDevices, bool * msDevice)
{
	HWND devListDialog = NULL;
	HWND devList;
	HWND okButton;
	LRESULT lResult;
	int i;

	for (i=0; i < SELECT_WAIT && !devListDialog; i++)
		{
		devListDialog = FindWindow(DIALOG_CLASS,DEVLIST_TITLE);
		if (devListDialog)
			{
			devList = GetDlgItem(devListDialog,ID_DEVLIST);
			okButton = GetDlgItem(devListDialog,ID_OKBUTTON);
			if (!devList || !okButton)
				{
				devListDialog = NULL;
				}
			}
		if (!devListDialog)
			Sleep(WAIT_SLEEP);
		}

	if (!devListDialog)
		return USBIO_ERR_FAILED;

	LRESULT itemCount = SendMessage (devList,LB_GETCOUNT,0,0);
	if (!itemCount)
		return USBIO_ERR_FAILED;

	* numDevices = 0;
	if (* deviceIndex >= itemCount)
		return USBIO_ERR_SUCCESS;

	char strBuffer[BUFFER_SIZE];
	char strVID [VIDPID_SIZE];
	char strPID1 [VIDPID_SIZE];
	char strPID2 [VIDPID_SIZE];
	bool deviceFound = false;

	for (i = * deviceIndex; i < itemCount; i++)
		{
		lResult = SendMessage (devList,LB_GETTEXT,(WPARAM)i,(LPARAM)&strBuffer);
		if (lResult > 0)
			{
			sprintf (strVID,"VID=%04x",aVendorId);
			sprintf (strPID1,"PID=%04x",aProductId1);
			sprintf (strPID2,"PID=%04x",aProductId2);
			if (strstr (strBuffer,strVID) && (strstr (strBuffer,strPID1) || strstr (strBuffer,strPID2)))
				{
				if (deviceFound)
					{
					(* numDevices)++;
					}
				else
					{
					deviceFound = true;
					* msDevice = strstr (strBuffer,strPID2) != NULL;
					* deviceIndex = i;
					lResult = SendMessage (devList,LB_SETCURSEL,(WPARAM)i,0);
					}
				}
			}
		}


	if (deviceFound)
		{
		if (lResult == LB_ERR)
			{
			return USBIO_ERR_FAILED;
			}
		else
			{
			lResult = SendMessage (devListDialog,WM_COMMAND,MAKEWPARAM(ID_OKBUTTON,BN_CLICKED),(LPARAM)okButton);

			return USBIO_ERR_SUCCESS;
			}
		}
	else
		{
		* deviceIndex = -1;

		return USBIO_ERR_FAILED;
		}

}


DWORD UsbcvControl::WaitForTestResult(BOOL * passFail)
{
	HWND testResultDialog = NULL;
	HWND okButton;
	char strBuffer[BUFFER_SIZE];

	for (int i=0; i < RESULT_WAIT && !testResultDialog; i++)
		{
		testResultDialog = FindWindow(DIALOG_CLASS,TESTRESULT_TITLE);
		if (testResultDialog)
			{
			UINT textSize = GetDlgItemText(testResultDialog,ID_RESULT_TEXT,(LPTSTR)&strBuffer,BUFFER_SIZE);
			okButton = GetDlgItem(testResultDialog,ID_RESULT_OK);
			if (!textSize || !okButton)
				{
				testResultDialog = NULL;
				}
			}
		if (!testResultDialog)
			Sleep(WAIT_SLEEP);
		}

	if (!testResultDialog)
		return USBIO_ERR_FAILED;

	if (strstr (strBuffer,TEST_PASSED))
		{
		* passFail = TRUE;
		}
	else
		{
		if (strstr (strBuffer,TEST_FAILED))
			{
			* passFail = FALSE;
			}
		else
			{
			return USBIO_ERR_FAILED;
			}
		}

	SendMessage (testResultDialog,WM_COMMAND,MAKEWPARAM(ID_RESULT_OK,BN_CLICKED),(LPARAM)okButton);

	return USBIO_ERR_SUCCESS;

}
