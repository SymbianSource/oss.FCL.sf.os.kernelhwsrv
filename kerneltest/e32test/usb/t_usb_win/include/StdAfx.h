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
// stdafx.h : include file for standard system include files,
//  or project specific include files that are used frequently, but
//      are changed infrequently
//

#if !defined(AFX_STDAFX_H__56375944_26B5_4519_B26C_DC6157E6896F__INCLUDED_)
#define AFX_STDAFX_H__56375944_26B5_4519_B26C_DC6157E6896F__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

//
// Depending on what version of header files you are using,
// you may need to define WINVER to 5.00
// This will enable the declaration of RegisterDeviceNotification() and 
// UnregisterDeviceNotification() in WINUSER.H.
//
// Note:
// Consequently, the application will run on Win98, WinME, and Win2000 only.
// This should not be a problem because USBIO supports only these platforms.
//
#define WINVER 0x500
#define VC_EXTRALEAN		// Exclude rarely-used stuff from Windows headers

#include <afxwin.h>         // MFC core and standard components
#include <afxext.h>         // MFC extensions
#include <afxdtctl.h>		// MFC support for Internet Explorer 4 Common Controls
#ifndef _AFX_NO_AFXCMN_SUPPORT
#include <afxcmn.h>			// MFC support for Windows Common Controls
#endif // _AFX_NO_AFXCMN_SUPPORT


//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_STDAFX_H__56375944_26B5_4519_B26C_DC6157E6896F__INCLUDED_)
