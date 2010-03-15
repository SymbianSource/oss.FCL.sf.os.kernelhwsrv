// Copyright (c) 2006-2009 Nokia Corporation and/or its subsidiary(-ies).
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
// Target application to be debugged by t_rmdebug.exe when testing
// security restrictions. This application is built with various
// capabilities by the t_rmdebug_securityX.mmp files. This allows
// the t_rmdebug2 program to ensure that security restrictions are
// properly enforced by the DSS/DDD subsystem.
// 
//

#include <e32base.h>
#include <e32base_private.h>
#include <e32cons.h>
#include <e32test.h>
#include <e32ldr.h>
#include <e32cmn.h>
#include <e32cmn_private.h>
#include "t_rmdebug_security.h"

CRunModeApp* CRunModeApp::NewL()
//
// CRunModeApp::NewL
//
	{
	CRunModeApp* self = new(ELeave) CRunModeApp();

  	self->ConstructL();
   
	return self;
	}

CRunModeApp::CRunModeApp()
//
// CRunModeApp constructor
//
	{
	}

CRunModeApp::~CRunModeApp()
//
// CRunModeApp destructor
//
	{
	}

void CRunModeApp::ConstructL()
//
// CRunModeApp::ConstructL
//
	{
	}

void CRunModeApp::TestWaitDebug()
//
// CRunModeApp::TestWaitDebug
//
	{
	RProcess::Rendezvous(KErrNone);

	// Wait a 3secs then quit (long enough to test, but not hang around forever)
	User::After(3000000);
	}

GLDEF_C TInt E32Main()
//
// Entry point for run mode debug app test program
//
	{
   TInt ret = KErrNone;
   
	// client
	CTrapCleanup* trap = CTrapCleanup::New();
	if (!trap)
		return KErrNoMemory;

   CRunModeApp* myApp = CRunModeApp::NewL();
   if (myApp != NULL)
       {
        __UHEAP_MARK;
	    TRAP(ret,myApp->TestWaitDebug());
	    __UHEAP_MARKEND;

	    delete myApp;
       }
       
	delete trap;

	return ret;
	}
