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
// Definitions for the run mode debug test thread.
// 
//

#ifndef RMDEBUGSVRTHRD_H
#define RMDEBUGSVRTHRD_H

#define SYMBIAN_RMDBG_MEMORYSIZE    1024*4

// Thread name
_LIT(KDebugThreadName,"DebugThread");

const TUint KDebugThreadDefaultHeapSize=0x10000;

// enumeration of functions which the target debug thread can call, the
// debugger can choose to switch the thread to a different function by
// writing the appropriate enumeration value into FunctionChooser, the
// target thread will finish executing the function it is currently running
// then execute the chosen function.
enum TTestFunction
	{
	EDefaultFunction = 0,
	EStepFunction = 1,
	EDemandPagingFunction = 2,
	EMultipleTraceCalls = 3,
	};

class CDebugServThread : public CBase
	{
	public:
		CDebugServThread();
		static TInt ThreadFunction(TAny* aStarted);    

	public:
	};

#endif // RMDEBUGSVRTHRD_H
