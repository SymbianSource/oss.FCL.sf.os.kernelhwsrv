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
// e32test\nkernsa\nkutils.h
// 
//

#ifndef __NKUTILS_H__
#define __NKUTILS_H__
#include <nk_priv.h>
#include <nktest/utils.h>

#ifndef __SMP__
class NThreadGroup;
#endif


enum TExitCallBackInstance
	{
	EInContext=0,
	EBeforeFree=1,
	EAfterFree=2,
	};

typedef TInt NRequestStatus;

typedef void (*TExitFunc)(TAny*, NThread*, TInt);

extern NThread* CreateThread(const char* aName, NThreadFunction aFunc, TInt aPri, const TAny* aParams, TInt aPSize, TBool aResume, TInt aTimeslice, TExitFunc aExitFunc=0, TAny* aExitParam=0, TUint32 aCpuAffinity=0, NThreadGroup* aGroup=0);
extern NThread* CreateThreadSignalOnExit(const char* aName, NThreadFunction aFunc, TInt aPri, const TAny* aParams, TInt aPSize, TInt aTimeslice, NFastSemaphore* aExitSem, TUint32 aCpuAffinity, NThreadGroup* aGroup=0);
extern NThread* CreateUnresumedThreadSignalOnExit(const char* aName, NThreadFunction aFunc, TInt aPri, const TAny* aParams, TInt aPSize, TInt aTimeslice, NFastSemaphore* aExitSem, TUint32 aCpuAffinity, NThreadGroup* aGroup=0);
extern void CreateThreadAndWaitForExit(const char* aName, NThreadFunction aFunc, TInt aPri, const TAny* aParams, TInt aPSize, TInt aTimeslice, TUint32 aCpuAffinity=0, NThreadGroup* aGroup=0);
extern TDfcQue* CreateDfcQ(const char* aName, TInt aPri, TUint32 aCpuAffinity=0, NThreadGroup* aGroup=0);
extern void DestroyDfcQ(TDfcQue* aQ);

extern void FMWaitFull(NFastMutex* aMutex);
extern void FMSignalFull(NFastMutex* aMutex);

extern TInt WaitWithTimeout(NFastSemaphore* aS, TUint32 aTimeout);

extern "C" TInt __timer_period();
extern "C" TInt __microseconds_to_timeslice_ticks(TInt us);
extern "C" TInt __fast_counter_to_timeslice_ticks(TUint64 aFCdelta);

const TInt KStackSize = 4096;
const TInt KTimeslice = 20000;	// microseconds

const TUint32 KMinTimeout = 1;
#ifdef __SMP__
const TInt KSmallTimeslice = 500;
#else
const TInt KSmallTimeslice = 2;
#endif

extern void WaitForRequest(NRequestStatus& aStatus);
extern void RequestComplete(NThread* aThread, NRequestStatus*& aStatus, TInt aValue);

extern void InitBTraceHandler();
extern void DumpBTraceBuffer();
extern void StartBTrace();
extern void StopBTrace();

extern "C" void CheckPoint();

#define __CHECKPOINT()	CheckPoint()


#endif


