// Copyright (c) 2008-2009 Nokia Corporation and/or its subsidiary(-ies).
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
// e32test\system\d_atomic.cpp
// LDD for testing atomic operations
// 
//

#include <kernel/kern_priv.h>
#include "t_atomic.h"

class DAtomicTest;
struct TPerThreadK : public TPerThread
	{
	TAny* iOldExtraContext;
	const SSlowExecEntry* iOldSlowExecTable;
	DAtomicTest* iCh;
	TInt iId;
	};


class DAtomicTestFactory : public DLogicalDevice
	{
public:
	DAtomicTestFactory();
	~DAtomicTestFactory();
	virtual TInt Install();
	virtual void GetCaps(TDes8& aDes) const;
	virtual TInt Create(DLogicalChannelBase*& aChannel);
public:
	};


class DAtomicTest : public DLogicalChannelBase
	{
public:
	virtual ~DAtomicTest();
protected:
	virtual TInt DoCreate(TInt aUnit, const TDesC8* anInfo, const TVersion& aVer);
	virtual TInt Request(TInt aFunction, TAny* a1, TAny* a2);
private:
	void SetupExecTable();
	static TInt XGetThreadInfo(TAny* aDest);
	static TInt XSetThreadInfo(const TAny* aSrc);
	static TInt XAtomicAction(TAny* aPtr);
	static TInt XRestore();
private:
	TUint64A	iReg;
	TPerThreadK	iPerThread[KMaxThreads];
	TUint32		iXT[19];
	};


DAtomicTestFactory::DAtomicTestFactory()
	{
	}

DAtomicTestFactory::~DAtomicTestFactory()
	{
	}
TInt DAtomicTestFactory::Create(DLogicalChannelBase*& aChannel)
	{
	aChannel = new DAtomicTest;
	return KErrNone;
	}

TInt DAtomicTestFactory::Install()
	{
	return SetName(&KAtomicTestLddName);
	}

void DAtomicTestFactory::GetCaps(TDes8& aDes) const
	{
	// Not used but required as DLogicalDevice::GetCaps is pure virtual
	}

DECLARE_STANDARD_LDD()
	{
	return new DAtomicTestFactory;
	}


TInt DAtomicTest::DoCreate(TInt /*aUnit*/, const TDesC8* /*aInfo*/, const TVersion& /*aVer*/)
	{
	SetupExecTable();
	return KErrNone;
	}

DAtomicTest::~DAtomicTest()
	{
	}

void DAtomicTest::SetupExecTable()
	{
	NThread* t = NKern::CurrentThread();
	SSlowExecTable& sxt = *(SSlowExecTable*)iXT;
	const SSlowExecTable& orig_sxt = *_LOFF(t->iSlowExecTable, SSlowExecTable, iEntries);
	sxt.iSlowExecCount = 4;		// get thread info, set thread info, atomic action, restore exec table
	sxt.iInvalidExecHandler = orig_sxt.iInvalidExecHandler;
	sxt.iPreprocessHandler = 0;	// not used
	sxt.iEntries[0].iFlags = 0;		// don't do anything special on entry or exit
	sxt.iEntries[0].iFunction = (TLinAddr)&XGetThreadInfo;
	sxt.iEntries[1].iFlags = 0;		// don't do anything special on entry or exit
	sxt.iEntries[1].iFunction = (TLinAddr)&XSetThreadInfo;
	sxt.iEntries[2].iFlags = 0;		// don't do anything special on entry or exit
	sxt.iEntries[2].iFunction = (TLinAddr)&XAtomicAction;
	sxt.iEntries[3].iFlags = 0;		// don't do anything special on entry or exit
	sxt.iEntries[3].iFunction = (TLinAddr)&XRestore;
	}

TInt DAtomicTest::Request(TInt aFunction, TAny* a1, TAny* a2)
	{
	TInt r = KErrNotSupported;
	switch (aFunction)
		{
		case RTestAtomic::ETDGExecuteK:
			{
			TDGBase tdg;
			kumemget32(&tdg, a1, sizeof(tdg));
			r = tdg.Execute();
			kumemput32(a1, &tdg, sizeof(tdg));
			break;
			}
		case RTestAtomic::EInitialise:
			{
			kumemget32(&iReg, a1, sizeof(TUint64));
			break;
			}
		case RTestAtomic::ERetrieve:
			{
			kumemput32(a1, &iReg, sizeof(TUint64));
			break;
			}
		case RTestAtomic::ESetCurrentThreadTimeslice:
			{
			NKern::ThreadSetTimeslice(NKern::CurrentThread(), NKern::TimesliceTicks((TInt)a1));
			r = KErrNone;
			break;
			}
		case RTestAtomic::ESwitchExecTables:
			{
			TUint tid = (TUint)a1;
			NThread* nt = NKern::CurrentThread();
			DThread& t = Kern::CurrentThread();
			if (Kern::NThreadToDThread(nt)==&t && tid<TUint(KMaxThreads))
				{
				TPerThreadK* p = iPerThread + tid;
				p->iOldExtraContext = nt->iExtraContext;
				p->iOldSlowExecTable = nt->iSlowExecTable;
				p->iId = tid;
				p->iCh = this;
				nt->iExtraContext = p;
				SSlowExecTable& sxt = *(SSlowExecTable*)iXT;
				nt->iSlowExecTable = sxt.iEntries;
				r = KErrNone;
				}
			break;
			}
		case RTestAtomic::EGetKernelMemoryAddress:
			{
			r = (TInt)NKern::CurrentThread();	// if we trash this the test should go bang
			break;
			}
		default:
			break;
		}
	return r;
	}

TInt DAtomicTest::XGetThreadInfo(TAny* aDest)
	{
	NThread* t = NKern::CurrentThread();
	TPerThreadK* p = (TPerThreadK*)t->iExtraContext;
	kumemput32(aDest, p, sizeof(TPerThread));
	return KErrNone;
	}

TInt DAtomicTest::XSetThreadInfo(const TAny* aSrc)
	{
	NThread* t = NKern::CurrentThread();
	TPerThreadK* p = (TPerThreadK*)t->iExtraContext;
	kumemget32(p, aSrc, sizeof(TPerThread));
	return KErrNone;
	}

TInt DAtomicTest::XAtomicAction(TAny* aPtr)
	{
	NThread* t = NKern::CurrentThread();
	TPerThreadK* p = (TPerThreadK*)t->iExtraContext;
	TAtomicAction action;
	kumemget32(&action, aPtr, sizeof(TAtomicAction));
	if (action.iThread!=p->iId || TUint(action.iIndex)>=TUint(TOTAL_INDEXES))
		return KErrNotSupported;
	return DoAtomicAction(&p->iCh->iReg, p, action);
	}

TInt DAtomicTest::XRestore()
	{
	NThread* t = NKern::CurrentThread();
	TPerThreadK* p = (TPerThreadK*)t->iExtraContext;
	t->iExtraContext = p->iOldExtraContext;
	t->iSlowExecTable = p->iOldSlowExecTable;
	return KErrNone;
	}
