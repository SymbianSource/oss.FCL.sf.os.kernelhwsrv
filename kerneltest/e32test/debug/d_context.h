// Copyright (c) 2003-2009 Nokia Corporation and/or its subsidiary(-ies).
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
// e32test/debug/d_context.h
// 
//

#ifndef __D_CONTEXT_H__
#define __D_CONTEXT_H__

#include <e32cmn.h>
#ifndef __KLIB_H__
#include <e32std.h>
#endif
#include <kernel/arm/arm_types.h>

_LIT(KTestLddName, "TestContext");

enum TUserCallbackState
	{
	ENoCallback,		// thread does nothing special
	ESpinningCallback,	// thread will be given a callback that spins forever
	ESleepingCallback,	// thread will be given a callback that sleeps for a long time
	};

struct TArmFullContext
	{
	TArmRegSet iUserContext;
	TUint32 iUserAvail;
	TArmRegSet iSystemContext;
	TUint32 iSystemAvail;
	};

class RContextLdd : public RBusLogicalChannel
	{
public:
	enum 
		{
		EHook,
		EGetLastExc,
		ETrapNextHwExc,
		ETrapNextSwExc,
		ETrapNextDeath,
		ESetGetContext,
		EGetContext,
		EGetKernelContext,
		ESpinInKernel,
		ESetGetFullContext,
		EAddUserCallback,
		EResumeTrappedThread,
		};

	struct TTrapInfo
		{
		TUint iThreadId;
		TAny* iContextPtr;
		TRequestStatus* iStatusPtr;
		TBool iKillThread;
		};

public:
	static inline TVersion Version() { return TVersion(1, 0, 1); }
#ifndef __KERNEL_MODE__
public:
	inline RContextLdd();
	inline TInt Open();
	inline TInt Hook(TInt* aCounterPtr);
	inline TExcType LastException();
	inline TBool IsHooked() const;
	inline void TrapNextHwExc(TThreadId aId, TAny* aContext, TRequestStatus& aStatus, TBool aKillThread);
	inline void TrapNextSwExc(TThreadId aId, TAny* aContext, TRequestStatus& aStatus, TBool aKillThread);
	inline void TrapNextDeath(TThreadId aId, TAny* aContext, TRequestStatus& aStatus);
	inline TInt SetAndGetBackContext(TThreadId aId, TAny* aContext);
	inline TInt SetAndGetFullContext(TThreadId aId, TArmFullContext* aContextData);
	inline void GetContext(TThreadId aId, TAny* aContext);
	inline void GetKernelContext(TThreadId aId, TAny* aContext);
	inline TUint32 SpinInKernel(TBool aReallySpin);
	inline void AddUserCallback(TThreadId aId, TUserCallbackState aCallback);
	inline void ResumeTrappedThread(TThreadId aId);
private:
	TBool iHooked;
#endif
	};


#ifndef __KERNEL_MODE__

inline RContextLdd::RContextLdd() 
	: iHooked(EFalse) 
	{
	}

inline TBool RContextLdd::IsHooked() const 
	{ 
	return iHooked; 
	}

inline TInt RContextLdd::Open()
	{
	return DoCreate(KTestLddName, Version(), KNullUnit, NULL, NULL, EOwnerProcess);
	}

inline TInt RContextLdd::Hook(TInt* aCounterPtr)
	{
	TInt r = DoControl(EHook, aCounterPtr);
	iHooked = (r == KErrNone);
	return r;
	}

inline TExcType RContextLdd::LastException()
	{
	return static_cast<TExcType>(DoControl(EGetLastExc));
	}

inline void RContextLdd::TrapNextHwExc(TThreadId aId, TAny* aExcContext, TRequestStatus& aStatus, TBool aKillThread)
	{
	aStatus = KRequestPending;
	TTrapInfo info;
	info.iThreadId = aId;
	info.iContextPtr = aExcContext;
	info.iStatusPtr = &aStatus;
	info.iKillThread = aKillThread;
	DoControl(ETrapNextHwExc, &info);
	}

inline void RContextLdd::TrapNextSwExc(TThreadId aId, TAny* aContext, TRequestStatus& aStatus, TBool aKillThread)
	{
	aStatus = KRequestPending;
	TTrapInfo info;
	info.iThreadId = aId;
	info.iContextPtr = aContext;
	info.iStatusPtr = &aStatus;
	info.iKillThread = aKillThread;
	DoControl(ETrapNextSwExc, &info);
	}

inline void RContextLdd::TrapNextDeath(TThreadId aId, TAny* aContext, TRequestStatus& aStatus)
	{
	aStatus = KRequestPending;
	TTrapInfo info;
	info.iThreadId = aId;
	info.iContextPtr = aContext;
	info.iStatusPtr = &aStatus;
	DoControl(ETrapNextDeath, &info);
	}

inline TInt RContextLdd::SetAndGetBackContext(TThreadId aId, TAny* aContext)
	{
	return DoControl(ESetGetContext, (TAny*)(TUint)aId, aContext);
	}

inline TInt RContextLdd::SetAndGetFullContext(TThreadId aId, TArmFullContext* aContextData)
	{
	return DoControl(ESetGetFullContext, (TAny*)(TUint)aId, (TAny*)aContextData);
	}

inline void RContextLdd::GetContext(TThreadId aId, TAny* aContext)
	{
	DoControl(EGetContext, (TAny*)(TUint)aId, aContext);
	}

inline void RContextLdd::GetKernelContext(TThreadId aId, TAny* aContext)
	{
	DoControl(EGetKernelContext, (TAny*)(TUint)aId, aContext);
	}

inline TUint32 RContextLdd::SpinInKernel(TBool aReallySpin)
	{
	return DoControl(ESpinInKernel, (TAny*)aReallySpin);
	}

inline void RContextLdd::AddUserCallback(TThreadId aId, TUserCallbackState aCallback)
	{
	DoControl(EAddUserCallback, (TAny*)(TUint)aId, (TAny*)aCallback);
	}

inline void RContextLdd::ResumeTrappedThread(TThreadId aId)
	{
	DoControl(EResumeTrappedThread, (TAny*)(TUint)aId);
	}

#endif // __KERNEL_MODE__

#endif
