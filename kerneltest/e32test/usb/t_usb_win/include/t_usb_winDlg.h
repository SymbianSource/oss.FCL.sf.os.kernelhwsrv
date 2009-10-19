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
// t_usb_winDlg.h : header file
//

#if !defined(AFX_T_USB_WINDLG_H__D718CC3F_C865_4D1E_BAA3_704E34073BD4__INCLUDED_)
#define AFX_T_USB_WINDLG_H__D718CC3F_C865_4D1E_BAA3_704E34073BD4__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

//#include "mdlsmain.h"

/////////////////////////////////////////////////////////////////////////////
// CT_usb_winDlg dialog

class CT_usb_winDlg : public CDialog
{
// Construction
public:
	CT_usb_winDlg(CWnd* pParent = NULL);	// standard constructor

// Dialog Data
	//{{AFX_DATA(CT_usb_winDlg)
	enum { IDD = IDD_T_USB_WIN_DIALOG };
	CEdit	m_text1;
	//}}AFX_DATA

	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CT_usb_winDlg)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
	HICON m_hIcon;
	CFont m_font;
	HDEVNOTIFY m_DevNotify;

	void DisplayHello();
	void OutputString(const char *s);
	BOOLEAN CT_usb_winDlg::RegisterDevNotify(const GUID *InterfaceClassGuid,HDEVNOTIFY *hDevNotify);

	// Generated message map functions
	//{{AFX_MSG(CT_usb_winDlg)
	virtual BOOL OnInitDialog();
	afx_msg void OnSysCommand(UINT nID, LPARAM lParam);
	afx_msg void OnClose();
	afx_msg LRESULT OnUserMessage(WPARAM wParam, LPARAM lParam);
	afx_msg void OnDestroy();
	afx_msg BOOL OnDeviceChange( UINT nEventType, DWORD dwData );
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_T_USB_WINDLG_H__D718CC3F_C865_4D1E_BAA3_704E34073BD4__INCLUDED_)
