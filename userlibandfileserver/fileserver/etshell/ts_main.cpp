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
// f32\etshell\ts_main.cpp
// 
//

#include "ts_std.h"

GLDEF_C void Panic(TShellPanic anErrorCode)
//
// Report a fault in the shell.
//
	{

	User::Panic(_L("SHELL"),anErrorCode);
	}

TInt E32Main()
//
// Run Shell
//
	{
	__UHEAP_MARK;
	CTrapCleanup* trapHandler=CTrapCleanup::New();
	if (trapHandler==NULL)
		return(KErrNoMemory);

    TInt r=KErrNone;
	TRAP(r,ShellFunction::TheShell=CShell::NewL());
	if (r!=KErrNone)
        {
        delete trapHandler;
        __UHEAP_MARKEND;
		return(r);
        }
    TRAP(r,ShellFunction::TheShell->RunL());
	if (r!=KErrNone)
        {
        delete ShellFunction::TheShell;
        delete trapHandler;
        __UHEAP_MARKEND;
		return(r);
        }

	delete ShellFunction::TheShell;
	delete trapHandler;
	__UHEAP_MARKEND;
	return(KErrNone);
	}
