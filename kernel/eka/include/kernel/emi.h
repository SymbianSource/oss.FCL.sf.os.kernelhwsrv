// Copyright (c) 2005-2009 Nokia Corporation and/or its subsidiary(-ies).
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
//

#ifndef __EMI_H__
#define __EMI_H__

#include <kernel/kernel.h>
#include <emievents.h>


/**
@publishedPartner
@released

Defines the type of function that is passed to EMI::TaskEventLogging(), and
which is called when a Symbian OS thread (DThread) starts.
*/
typedef TInt (*TThreadStartMonitor)(NThread*);




/**
@publishedPartner
@released

Defines the type of function that is passed to EMI::TaskEventLogging(), and 
which is called when a Symbian OS thread (DThread) exits.
*/
typedef void (*TThreadExitMonitor)(NThread*);



/**
@publishedPartner
@released

A collection of static functions, known as the Execution Monitoring
Interface (EMI), that can be used to implement energy management schemes (VEMS)
that aim to reduce the overall energy use of a device.

This interface is intended to be used by heavyweight schemes that need detailed knowledge
of thread scheduling within Symbian OS. The EMI is generic, and means that  
more than one energy management scheme could be developed to meet the needs of
a given device.

The interface maintains real time capability by keeping the necessary increase
in latency of Symbian OS to a minimum.
*/
class EMI 
	{
	
public:

	IMPORT_C static TInt     TaskEventLogging (TBool aLogging, TInt aLogSize, TThreadStartMonitor aStartMonitor, TThreadExitMonitor aExitMonitor);
	IMPORT_C static TInt     TaskEventLogging (TBool aLogging, TInt aLogSize);
	IMPORT_C static void     ThreadMonitors (TThreadStartMonitor aStartMonitor, TThreadExitMonitor aExitMonitor);
	IMPORT_C static TBool    GetTaskEvent(TTaskEventRecord& aRecord);
	IMPORT_C static TBool    AddTaskEvent(TTaskEventRecord& aRecord);
	IMPORT_C static NThread* GetIdleThread();
	IMPORT_C static NThread* GetSigmaThread();
	IMPORT_C static void     SetThreadVemsData(NThread* aThread, TAny* aVemsData);
	IMPORT_C static TAny*    GetThreadVemsData(NThread* aThread);
	IMPORT_C static void     SetThreadLoggable(NThread* aThread, TBool aLoggable);
	IMPORT_C static TBool    GetThreadLoggable(NThread* aThread);
	IMPORT_C static void     SetThreadTag(NThread* aThread, TUint32 aTag);
	IMPORT_C static TUint32  GetThreadTag(NThread* aThread);
	IMPORT_C static void     SetMask(TUint32 aMask);
	IMPORT_C static TUint32  GetMask();
	IMPORT_C static void     SetDfc(TDfc* aDFC, TUint32 aMask);
	IMPORT_C static TUint32  GetDfcTriggerTag();
	IMPORT_C static void     SetState(TUint32 aState);
	IMPORT_C static TUint32  GetState();
	IMPORT_C static void     AfterIdle(TInt aDelay);

	IMPORT_C static void     EnterIdle();
	IMPORT_C static void     LeaveIdle();

	static TInt Init();

private:

	static NFastSemaphore*     ExitSemaphore;
	static TThreadStartMonitor StartMonitor;
	static TThreadExitMonitor  ExitMonitor;
	static TThreadExitMonitor  OldExitMonitor;
	static TInt                Generation;

typedef enum {ENone,EWaiting,EHeld} TAfterIdleState;

	static NTimer*         AfterIdleTimer;
	static TAfterIdleState AfterIdleState;
	static TInt            AfterIdleDelay;

	static void CallStartHandler(DThread* aThread);
	static void CallExitHandler(DThread* aThread);

	friend class DThread;
	friend void AfterIdleCallback(TAny*);

	};

#endif
