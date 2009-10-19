/*
* Copyright (c) 2003-2009 Nokia Corporation and/or its subsidiary(-ies).
* All rights reserved.
* This component and the accompanying materials are made available
* under the terms of the License "Eclipse Public License v1.0"
* which accompanies this distribution, and is available
* at the URL "http://www.eclipse.org/legal/epl-v10.html".
*
* Initial Contributors:
* Nokia Corporation - initial contribution.
*
* Contributors:
*
* Description:
* e32\personality\example\personality_int.h
* Internal header file for example RTOS personality.
* This will be included by the personality layer source files.
* 
* WARNING: This file contains some APIs which are internal and are subject
*          to change without notice. Such APIs should therefore not be used
*          outside the Kernel and Hardware Services package.
*
*/



/**
 @file
 @internalComponent
*/

#ifndef __PERSONALITY_INT_H__
#define __PERSONALITY_INT_H__

// NThreadBase member data
#define __INCLUDE_NTHREADBASE_DEFINES__

#include <personality/example/personality.h>
#include <kernel/kern_priv.h>

// dummy constructor
inline NThreadBase::NThreadBase()
	{
	}

class PThread : public NThread
	{
public:
	enum PThreadState
		{
		EWaitMsgQ = NThreadBase::ENumNStates,
		EWaitSemaphore
		};
public:
	static TInt Create(PThread*& aThread, const taskinfo* aInfo);
	static void CreateAll(const taskinfo* aInfo);
	static void MsgQIDfcFn(TAny*);
	static void StateHandler(NThread* aThread, TInt aOp, TInt aParam);
	static void ExceptionHandler(TAny* aContext, NThread* aThread);
public:
	inline PThread() : iMsgQIDfc(0,0) {}	// dummy constructor
	void ISRPost(msghdr* aM);
	void Post(msghdr* aFirst, msghdr* aLast);
	msghdr* GetMsg();
	void HandleSuspend();
	void HandleResume();
	void HandleRelease(TInt aReturnCode);
	void HandlePriorityChange(TInt aNewPriority);
	void HandleTimeout();
public:
	TInt	iTaskId;
	TInt	iSetPriority;
	msghdr*	iFirstMsg;
	msghdr* iLastMsg;
	TDfc	iMsgQIDfc;
	msghdr* iISRFirstMsg;
	msghdr* iISRLastMsg;
public:
	static TInt NumTasks;
	static TInt MaxTaskId;
	static PThread** TaskTable;
	static const TUint8 NThreadPriorityTable[MAX_TASK_PRIORITY+1];
	static const SNThreadHandlers Handlers;
	};

class PMemPool;
struct SMemBlock
	{
	PMemPool*	iPool;
	SMemBlock*	iNext;		// only if free block
	};

class PMemPool
	{
public:
	TInt Create(const poolinfo* aInfo);
	void* Alloc();
	void Free(void* aBlock);
public:
	SMemBlock*	iFirstFree;
	size_t		iBlockSize;
	};

class PMemMgr
	{
public:
	static void Create(const poolinfo* aInfo);
	static void* Alloc(size_t aSize);
	static void Free(void* aBlock);
public:
	TInt iPoolCount;
	PMemPool iPools[1];		// extend
public:
	static PMemMgr* TheMgr;
	};


class PTimer : public NTimer
	{
public:
	PTimer();
	static void CreateAll();
	static void NTimerExpired(TAny*);
public:
	TInt		iPeriod;	// 0 if single shot
	TAny*		iCookie;
	PThread*	iThread;
	TUint		iExpiryCount;
public:
	static TInt NumTimers;
	static PTimer* TimerTable;
	};


class PSemaphore
	{
public:
	static void CreateAll();
public:
	PSemaphore();
	void WaitCancel(PThread* aThread);
	void SuspendWaitingThread(PThread* aThread);
	void ResumeWaitingThread(PThread* aThread);
	void ChangeWaitingThreadPriority(PThread* aThread, TInt aNewPriority);
	void Signal();
	void ISRSignal();
	static void IDfcFn(TAny*);
public:
	TInt iCount;
	TInt iISRCount;
	TDfc iIDfc;
	SDblQue iSuspendedQ;
	TPriList<PThread, KNumPriorities> iWaitQ;
public:
	static TInt NumSemaphores;
	static PSemaphore* SemaphoreTable;
	};

class TPMsgQ : public TDfc
	{
public:
	TPMsgQ(TDfcFn aFunction, TAny* aPtr, TDfcQue* aDfcQ, TInt aPriority);
	void Receive();
	msghdr* Get();
	void CancelReceive();
public:
	msghdr* iFirstMsg;
	msghdr* iLastMsg;
	TBool iReady;
public:
	static TPMsgQ* ThePMsgQ;
	};


#endif
