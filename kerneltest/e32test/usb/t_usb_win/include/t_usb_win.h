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
// t_usb_win.h : main header file for the T_USB_WIN application
//

#if !defined(AFX_T_USB_WIN_H__667290F3_79F3_44E5_9EDC_86D2E9D2E6B0__INCLUDED_)
#define AFX_T_USB_WIN_H__667290F3_79F3_44E5_9EDC_86D2E9D2E6B0__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#ifndef __AFXWIN_H__
	#error include 'stdafx.h' before including this file for PCH
#endif

#include "resource.h"		// main symbols

/////////////////////////////////////////////////////////////////////////////
// CT_usb_winApp:
// See t_usb_win.cpp for the implementation of this class
//

class CT_usb_winApp : public CWinApp
{
public:
	CT_usb_winApp();

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CT_usb_winApp)
	public:
	virtual BOOL InitInstance();
	//}}AFX_VIRTUAL

// Implementation

	//{{AFX_MSG(CT_usb_winApp)
		// NOTE - the ClassWizard will add and remove member functions here.
		//    DO NOT EDIT what you see in these blocks of generated code !
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};


/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_T_USB_WIN_H__667290F3_79F3_44E5_9EDC_86D2E9D2E6B0__INCLUDED_)
