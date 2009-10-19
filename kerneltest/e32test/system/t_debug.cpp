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
// e32test\system\t_debug.cpp
// 
//

#include <e32test.h>

const TUint KDefaultHeapSize=1000;

GLDEF_C TInt TestThread(TAny* anArg)
//
//
//
	{

	((RSemaphore*)anArg)->Signal();
	User::WaitForAnyRequest();
	return 0;
	}

TInt E32Main()
//
//
//
    {

	__DEBUGGER();
	RThread thread;
	RSemaphore sem;
	sem.CreateLocal(0);

	thread.Create(_L("Test Thread"),TestThread,KDefaultStackSize,KDefaultHeapSize,KDefaultHeapSize,&sem);

	TRequestStatus stat;
	thread.Logon(stat);
   	thread.Resume();
    sem.Wait();

//	thread.Kill(0);
 	thread.Panic(_L("Test Panic"),0);
	User::WaitForRequest(stat);
	thread.Close();
   	return 0;
    
    }
