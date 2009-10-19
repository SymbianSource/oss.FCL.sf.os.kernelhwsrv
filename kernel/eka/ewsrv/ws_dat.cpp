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
// e32\ewsrv\ws_dat.cpp
// 
//

#include "ws_std.h"

GLDEF_D CCaptureKeys *CEvent::CaptureKeys;

GLDEF_D CKeyTranslator *KeyTranslator;
GLDEF_D CKeyRepeat *KeyRepeat;

GLDEF_D CScreenDriver *CWsWindow::ScreenDriver;
GLDEF_D CPeriodic *CWsWindow::CursorPeriodic;
GLDEF_D TSize CWsWindow::ScreenSize;
GLDEF_D TSize CWsWindow::FontSize;
GLDEF_D CBitMapAllocator *CWsWindow::Numbers;
GLDEF_D TDblQue<CWsWindow> CWsWindow::WQueue(_FOFF(CWsWindow,iLink));
GLDEF_D TInt8 *CWsWindow::VisibilityMap;
GLDEF_D TText *CWsWindow::BlankLineText;
GLDEF_D ColorInformation *CWsWindow::BlankLineAttributes;
GLDEF_D RMutex CWsWindow::MouseMutex;
GLDEF_D RMutex CWsWindow::ServiceMutex;
GLDEF_D RSemaphore CNotifierSession::NotifierSemaphore;
GLDEF_D TPoint CWsWindow::MousePos(0,0);
GLDEF_D TPoint CWsWindow::ScrollWithMouse(-1,-1);
GLDEF_D TPoint CWsWindow::MoveWithMouse(-1,-1);
GLDEF_D TPoint CWsWindow::ResizeWithMouse(-1,-1);
GLDEF_D TInt CWsWindow::ScrollSpeed=0;
GLDEF_D TBool CWsWindow::MouseIsCaptured=EFalse;
GLDEF_D TInt CWsWindow::Count=0;
GLDEF_D CWsWindow* CWsWindow::RawEventWindow=NULL;
GLDEF_D TColorIndex CWsWindow::ScreenColor;
GLDEF_D TColorIndex CWsWindow::BorderColor;
GLDEF_D TColorIndex CWsWindow::WindowBgColor;
GLDEF_D TColorIndex CWsWindow::IndexOf[8];
GLDEF_D const TText CWsWindow::Cursors[101] =
	{
    0,
	220, 220, 220, 220, 220, 220, 220, 220, 220, 220,
	220, 220, 220, 220, 220, 220, 220, 220, 220, 220,
	220, 220, 220, 220, 220, 220, 220, 220, 220, 220,
	220, 220, 220, 220, 220, 220, 220, 220, 220, 220,
	220, 220, 220, 220, 220, 220, 220, 220, 220, 220,
	219, 219, 219, 219, 219, 219, 219, 219, 219, 219,
	219, 219, 219, 219, 219, 219, 219, 219, 219, 219,
	219, 219, 219, 219, 219, 219, 219, 219, 219, 219,
	219, 219, 219, 219, 219, 219, 219, 219, 219, 219,
	219, 219, 219, 219, 219, 219, 219, 219, 219, 219
	};



