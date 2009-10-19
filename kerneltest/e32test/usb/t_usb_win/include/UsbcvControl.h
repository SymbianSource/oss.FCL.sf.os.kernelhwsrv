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
// UsbcvControl.h: interface for the UsbcvControl class.
//


#if !defined(AFX_USBCVCONTROL_H__D42D5ED4_0817_4BEE_879B_636CCFB051E8__INCLUDED_)
#define AFX_USBCVCONTROL_H__D42D5ED4_0817_4BEE_879B_636CCFB051E8__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

class UsbcvControl  
{
private:
    STARTUPINFO si;
    PROCESS_INFORMATION pi;
	HWND mainWindow;
	HWND exitButton;
	HWND runButton;
	DWORD SelectDevice(USHORT aVendorId, USHORT aProductId1, USHORT aProductId2, int * deviceIndex, int * numDevices, bool * msDevice);
	DWORD WaitForTestResult(BOOL * passFail);	
	DWORD ContinueOk (char * choiceText, int buttonId);

public:
	UsbcvControl();
	virtual ~UsbcvControl();

	DWORD TestAllDevices (USHORT aVendorId, USHORT aProductId1, USHORT aProductId2);
	DWORD Create ();
};

#endif // !defined(AFX_USBCVCONTROL_H__D42D5ED4_0817_4BEE_879B_636CCFB051E8__INCLUDED_)
