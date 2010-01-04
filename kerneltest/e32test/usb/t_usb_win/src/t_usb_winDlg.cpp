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
// t_usb_winDlg.cpp : implementation file
//

#include "stdafx.h"
#include <dbt.h>
#include <afxpriv.h>
#include "t_usb_win.h"
#include "t_usb_winDlg.h"
#include "global.h"

#include "usbio.h"											// USBIO Dev Kit
#include "usbiopipe.h"										// ditto

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// external globals
extern CUsbIo gUsbDev;
extern GUID gUsbioID;
extern BOOL gVerboseMode;
extern HANDLE gDeviceConnectEvent; 
extern HANDLE gDeviceDisconnectEvent; 
extern FILE *logStream;

extern void PrintOut(BOOL screenFlag, BOOL logFlag, BOOL timeFlag, const char *format, ...);
extern UINT DoTests(LPVOID pParam);

/////////////////////////////////////////////////////////////////////////////
// CAboutDlg dialog used for App About

class CAboutDlg : public CDialog
{
public:
	CAboutDlg();

// Dialog Data
	//{{AFX_DATA(CAboutDlg)
	enum { IDD = IDD_ABOUTBOX };
	//}}AFX_DATA

	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CAboutDlg)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
	//{{AFX_MSG(CAboutDlg)
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

CAboutDlg::CAboutDlg() : CDialog(CAboutDlg::IDD)
{
	//{{AFX_DATA_INIT(CAboutDlg)
	//}}AFX_DATA_INIT
}

void CAboutDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CAboutDlg)
	//}}AFX_DATA_MAP
}

BEGIN_MESSAGE_MAP(CAboutDlg, CDialog)
	//{{AFX_MSG_MAP(CAboutDlg)
		// No message handlers
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CT_usb_winDlg dialog

CT_usb_winDlg::CT_usb_winDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CT_usb_winDlg::IDD, pParent)
{
	//{{AFX_DATA_INIT(CTempwinDlg)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT
	// Note that LoadIcon does not require a subsequent DestroyIcon in Win32
	m_hIcon = AfxGetApp()->LoadIcon(IDD_T_USB_WIN_DIALOG);
}


void CT_usb_winDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CT_usb_winDlg)
	DDX_Control(pDX, IDC_TEXT1, m_text1);
	//}}AFX_DATA_MAP
}

BEGIN_MESSAGE_MAP(CT_usb_winDlg, CDialog)
	//{{AFX_MSG_MAP(CT_usb_winDlg)
	ON_WM_SYSCOMMAND()
	ON_WM_CLOSE()
    ON_MESSAGE(WM_USER_PRINTOUT, OnUserMessage)
	ON_WM_DESTROY()
	//}}AFX_MSG_MAP
    ON_WM_DEVICECHANGE()
END_MESSAGE_MAP()


void CT_usb_winDlg::OutputString(const char *s)
{
  int len;
  int limit;
  
  // get current length
  len=m_text1.GetWindowTextLength(); 
  // get limit
  limit = m_text1.GetLimitText() - 2048;

  if ( len > limit ) {
    // delete 8K text
    m_text1.SetSel(0,8192);
    m_text1.Clear();
    len=m_text1.GetWindowTextLength(); 
  }
  // append string
  m_text1.SetSel(len,len);
  m_text1.ReplaceSel(s, FALSE);
}

void CT_usb_winDlg::DisplayHello()
	{
	PRINT_ALWAYS "*------------------------------------------------------------------------------"NL); 
 

	PRINT_ALWAYS "* T_USB_WIN v%d.%d.%d (for use with TUSBWIN.SYS v%d.%d)"NL,
		   VERSION_MAJOR, VERSION_MINOR, VERSION_MICRO,
		   USBIO_VERSION_MAJOR, USBIO_VERSION_MINOR);

	PRINT_ALWAYS "* USB Test Application, Host-side Part"NL);
	PRINT_ALWAYS "*   Copyright (c) 2007-2009 Nokia Corporation and/or its subsidiary(-ies)."NL);
	PRINT_ALWAYS "*------------------------------------------------------------------------------"NL); 
	}

/////////////////////////////////////////////////////////////////////////////
// CT_usb_winDlg message handlers

BOOL CT_usb_winDlg::OnInitDialog()
{
	CDialog::OnInitDialog();

	// TODO: Add extra initialization here
	// Add "About..." menu item to system menu.

	// IDM_ABOUTBOX must be in the system command range.
	ASSERT((IDM_ABOUTBOX & 0xFFF0) == IDM_ABOUTBOX);
	ASSERT(IDM_ABOUTBOX < 0xF000);

	CMenu* pSysMenu = GetSystemMenu(FALSE);
	if (pSysMenu != NULL)
	{
		CString strAboutMenu;
		strAboutMenu.LoadString(IDS_ABOUTBOX);
		if (!strAboutMenu.IsEmpty())
		{
			pSysMenu->AppendMenu(MF_SEPARATOR);
			pSysMenu->AppendMenu(MF_STRING, IDM_ABOUTBOX, strAboutMenu);
		}
	}

	// init font
	BOOL succ = m_font.CreatePointFont(
                80,  //int nPointSize, 
                "System",  //LPCTSTR lpszFaceName,
                NULL        // CDC* pDC = NULL
                );
	if ( succ )
	{
		m_text1.SetFont(&m_font);
	} else {
		AfxMessageBox("Unable to initialize font.");
	}

	// set text limit to 512K
	m_text1.SetLimitText(512*1024);

	DisplayHello();

	if (!RegisterDevNotify(&gUsbioID,&m_DevNotify)) {
		PRINT_ALWAYS "ERROR: Unable to register device notification."NL);
		} 

	AfxBeginThread (DoTests, NULL);

	return TRUE;  // return TRUE  unless you set the focus to a control
}

/////////////////////////////////////////////////////////////////////////////
// CT_usb_winDlg::OnClose
//      OnClose makes sure the log file stream is closed.

void CT_usb_winDlg::OnClose()
{
	if (logStream)
		fclose (logStream);

	CDialog::OnClose();
}

void CT_usb_winDlg::OnSysCommand(UINT nID, LPARAM lParam)
{
	if ((nID & 0xFFF0) == IDM_ABOUTBOX)
	{
		CAboutDlg dlgAbout;
		dlgAbout.DoModal();
	}
	else
	{
		CDialog::OnSysCommand(nID, lParam);
	}
}


//
// Message Handler for WM_USER_PRINTOUT
//
// get string buffer from message, print out, free buffer
//
LRESULT CT_usb_winDlg::OnUserMessage(WPARAM wParam, LPARAM lParam)
{
  const char* buffer = (const char*)lParam;

  if ( buffer!=NULL ) {
    // print to output window
    OutputString(buffer);
  }

  return 0;
}


BOOLEAN CT_usb_winDlg::RegisterDevNotify(
  const GUID *InterfaceClassGuid, 
  HDEVNOTIFY *hDevNotify
  )
/*
Routine Description:
    Registers for notification of changes in the device interfaces for
    the specified interface class GUID. 

Parameters:
    InterfaceClassGuid - The interface class GUID for the device 
        interfaces. 

    hDevNotify - Receives the device notification handle. On failure, 
        this value is NULL.

Return Value:
    If the function succeeds, the return value is TRUE.

    If the function fails, the return value is FALSE.
*/
{
    DEV_BROADCAST_DEVICEINTERFACE NotificationFilter;

    ZeroMemory(&NotificationFilter, sizeof(NotificationFilter) );
    NotificationFilter.dbcc_size = sizeof(DEV_BROADCAST_DEVICEINTERFACE);
    NotificationFilter.dbcc_devicetype = DBT_DEVTYP_DEVICEINTERFACE;
    NotificationFilter.dbcc_classguid = *InterfaceClassGuid;

    // device notifications should be send to the main dialog
    *hDevNotify = RegisterDeviceNotification(
                      m_hWnd, 
                      &NotificationFilter,
                      DEVICE_NOTIFY_WINDOW_HANDLE
                      );
    if ( !(*hDevNotify) ) {
      DWORD Err = GetLastError();
      PRINT_ALWAYS "RegisterDeviceNotification failed, errcode:%08X"NL,Err);
      return FALSE;
    }

    return TRUE;
}


#define _ENUMSTR(e) (x==(e)) ? #e

static const char* DeviceChangeMsgStr(UINT x)
{
  return (
    _ENUMSTR(DBT_DEVICEARRIVAL) :
    _ENUMSTR(DBT_DEVICEQUERYREMOVE) :
    _ENUMSTR(DBT_DEVICEQUERYREMOVEFAILED) :
    _ENUMSTR(DBT_DEVICEREMOVEPENDING) :
    _ENUMSTR(DBT_DEVICEREMOVECOMPLETE) :
    _ENUMSTR(DBT_DEVICETYPESPECIFIC) :
    _ENUMSTR(DBT_CUSTOMEVENT) :
    _ENUMSTR(DBT_USERDEFINED) :
    _ENUMSTR(DBT_DEVNODES_CHANGED) :
    "unknown"
  );
}

BOOL CT_usb_winDlg::OnDeviceChange( UINT nEventType, DWORD dwData )
{
	DEV_BROADCAST_DEVICEINTERFACE *data=(DEV_BROADCAST_DEVICEINTERFACE*)dwData;

	PRINT_IF_VERBOSE "OnDeviceChange message: %08X (%s)"NL,nEventType,DeviceChangeMsgStr(nEventType));

	// check if data is valid
	if (data == NULL || data->dbcc_name == NULL || strlen(data->dbcc_name)==0) {
		return TRUE;
	}

	// convert interface name to CString
	CString Name(data->dbcc_name);

	// there is some strange behavior in Win98
	// there are notifications with dbcc_name = "."
	// we ignore this
	if (Name.GetLength() < 5) {
		return TRUE;
	}

	switch (nEventType) 
		{
		case DBT_DEVICEREMOVECOMPLETE:
			// a device with our interface has been removed or is stopped
			PRINT_NOLOG "Device disconnected."NL);
			PRINT_IF_VERBOSE "Device path: %s."NL,data->dbcc_name); 
			// close the global  handle
			if ( gUsbDev.GetDevicePathName() && (0==Name.CompareNoCase(gUsbDev.GetDevicePathName())) )
				{
				PRINT_IF_VERBOSE "Closing driver interface"NL NL);
				gUsbDev.Close();
				}
			SetEvent (gDeviceDisconnectEvent);
			break;

		case DBT_DEVICEARRIVAL:
			// a device with our interface has been activated (started)
			PRINT_NOLOG "Device connected."NL); 
			PRINT_IF_VERBOSE "Device path: %s."NL,data->dbcc_name); 
 			SetEvent (gDeviceConnectEvent);
			break;
		
		case DBT_DEVICEQUERYREMOVE:
			// windows asked, if our device can be removed, we answer with TRUE (yes)
			PRINT_IF_VERBOSE "MSG: DBT_DEVICEQUERYREMOVE -- Application returned success."NL);
			break;
		
		case DBT_DEVICEREMOVEPENDING:
			// device remove is pending
			PRINT_IF_VERBOSE "MSG: DBT_DEVICEREMOVEPENDING."NL);
			break;
		
		default:
			break;
		} 

  return TRUE;
}

void CT_usb_winDlg::OnDestroy() 
{
  // close device
  gUsbDev.Close();

  // The following is a work-around for a bug in Win98,
  // the system becomes unstable if UnregisterDeviceNotification is called
  // Therefore we don't call it on Win98.
  if ( _winver >= 0x500 ) {
    if ( m_DevNotify!=NULL ) {
      UnregisterDeviceNotification(m_DevNotify);
      m_DevNotify = NULL;
    } 
  }

  CDialog::OnDestroy();

}
