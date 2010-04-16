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
// Implements a debug thread for testing.
// 
//

#include <e32base.h>
#include <e32base_private.h>
#include <e32cons.h>
#include "d_rmdebugthread.h"

EXPORT_C TBuf8<SYMBIAN_RMDBG_MEMORYSIZE> gMemoryAccessBytes;
extern void RMDebug_BranchTst1();

EXPORT_C TInt TestData;

CDebugServThread::CDebugServThread()
//
// Empty constructor
//
	{
	}

GLDEF_C TInt CDebugServThread::ThreadFunction(TAny*)
//
// Generic thread function for testing
//
	{
	CTrapCleanup* cleanup=CTrapCleanup::New();
	if (cleanup == NULL)
		{
		User::Leave(KErrNoMemory);
		}

	RThread::Rendezvous(KErrNone);

	TestData = 1;

	while(1)
		{
		RMDebug_BranchTst1();

		TestData++;                   

		// Wait half a second (suspends this thread)
		User::After(500000);

		if (TestData == 0xFFFFFFFF)
			{
			break;
			}
		}

	delete cleanup;

	return (KErrNone);
	}

EXPORT_C TInt StartDebugThread(RThread& aDebugThread)
//
// Starts the test thread
//
{
	TInt res=KErrNone;

	// Create the thread
	res = aDebugThread.Create(	KDebugThreadName,
								CDebugServThread::ThreadFunction,
								KDefaultStackSize,
								KDebugThreadDefaultHeapSize,
								KDebugThreadDefaultHeapSize,
								NULL
								);

	// Check that the creation worked
	if (res == KErrNone)
		{
		TRequestStatus rendezvousStatus;

		aDebugThread.SetPriority(EPriorityNormal);
		// Make a request for a rendezvous
		aDebugThread.Rendezvous(rendezvousStatus);
		// Set the thread as ready for execution
		aDebugThread.Resume();
		// Wait for the resumption
		User::WaitForRequest(rendezvousStatus);
		}                                 
	else
		{
		// Close the handle.
		aDebugThread.Close();
		}

	return res;
	}
