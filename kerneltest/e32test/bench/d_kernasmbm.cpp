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
// e32test\bench\d_kernasmbm.cpp
// Device driver providing benchmarking for assmblerised kernel routines
// 
//

#include <kernel/kern_priv.h>
#include <kernel/cache.h>
#include "d_kernasmbm.h"
#include "d_kernbm.h"

RPointerArray<TKernelBenchmark> KernelBenchmarks;

// TKernelBenchmark ////////////////////////////////////////////////////////////

TKernelBenchmark::TKernelBenchmark(const TDesC8& aName)
	{
	iInfo.iName = aName;
	iInfo.iCategories = KCategoryGeneral;
	iInfo.iAlignStep = 0;
	KernelBenchmarks.Append(this);  // Ignores rc
	}

TKernelBenchmark::TKernelBenchmark(const TDesC8& aName, TInt aAlignStep)
	{
	iInfo.iName = aName;
	iInfo.iCategories = KCategoryGeneral | KCategoryMemory;
	iInfo.iAlignStep = aAlignStep;
	KernelBenchmarks.Append(this);  // Ignores rc
	}

const TBmInfo& TKernelBenchmark::Info() const
	{
	return iInfo;
	}

TInt TKernelBenchmark::Run(const TBmParams& aParams, TInt& aResult)
	{
	TUint init, final;
	init = NKern::FastCounter();
	DoRun(aParams);
	final = NKern::FastCounter();
	aResult = final - init;
	return KErrNone;
	}

// TThreadedBenchmark //////////////////////////////////////////////////////////

TThreadedBenchmark::TThreadedBenchmark(const TDesC8& aName, TInt aRelPri) :
	TKernelBenchmark(aName), iRelPri(aRelPri)
	{
	}

TInt TThreadedBenchmark::Run(const TBmParams& aParams, TInt& aResult)
	{
	iIts = aParams.iIts;
	iThread1 = &Kern::CurrentThread();

	SThreadCreateInfo info;
	info.iType=EThreadSupervisor;
	info.iFunction=Thread2Func;
	info.iPtr=this;
	info.iSupervisorStack=NULL;
	info.iSupervisorStackSize=0;	// zero means use default value
	info.iInitialThreadPriority=iThread1->iNThread.iPriority + iRelPri;
	info.iName.Set(_L("bmthread"));
	info.iTotalSize = sizeof(info);
	
	NKern::ThreadEnterCS();
	TInt r = Kern::ThreadCreate(info);
	NKern::ThreadLeaveCS();
	if (r != KErrNone)
		return r;

	iThread2 = (DThread*)info.iHandle;

	return TKernelBenchmark::Run(aParams, aResult);
	}

TInt TThreadedBenchmark::Thread2Func(TAny* aPtr)
	{
	TThreadedBenchmark* self = (TThreadedBenchmark*)aPtr;
	self->DoRun2(self->iIts);
	return KErrNone;
	}

// Device driver implementation ////////////////////////////////////////////////

class DKernAsmBmFactory : public DLogicalDevice
	{
public:
	virtual TInt Install();						//overriding pure virtual
	virtual void GetCaps(TDes8& aDes) const;	//overriding pure virtual
	virtual TInt Create(DLogicalChannelBase*& aChannel);
	};

class DKernAsmBm : public DLogicalChannel
	{
public:
	virtual ~DKernAsmBm();
	virtual TInt DoCreate(TInt aUnit, const TDesC8* anInfo, const TVersion& aVer);
	virtual TInt Request(TInt aFunction, TAny* a1, TAny* a2);
	virtual void HandleMsg(TMessageBase *aMsg);
	};

TInt DKernAsmBmFactory::Install()
    {
    return(SetName(&KKernAsmBmLddName));
    }

void DKernAsmBmFactory::GetCaps(TDes8& /*aDes*/) const
    {
    }

TInt DKernAsmBmFactory::Create(DLogicalChannelBase*& aChannel)
	{
	aChannel = new DKernAsmBm;
	if(!aChannel)
		return KErrNoMemory;
	return KErrNone;
	}

DECLARE_STANDARD_LDD()
	{
    return new DKernAsmBmFactory;
    }

DKernAsmBm::~DKernAsmBm()
	{
	CloseData();
	}

TInt DKernAsmBm::DoCreate(TInt /*aUnit*/, const TDesC8* /*anInfo*/, const TVersion& /*aVer*/)
    {
	return InitData();
	}

TInt DKernAsmBm::Request(TInt aFunction, TAny* a1, TAny* a2)
    {
	TInt index = aFunction >> 8;
	aFunction &= 0xff;
	
	if (aFunction != RKernAsmBmLdd::ECount && (index < 0 || index >= KernelBenchmarks.Count()))
		return KErrArgument;

	switch (aFunction)
		{
		case RKernAsmBmLdd::ECount:
			return KernelBenchmarks.Count();
			
		case RKernAsmBmLdd::EInfo:
			{
			TPckgC<TBmInfo> info(KernelBenchmarks[index]->Info());
			return Kern::ThreadDesWrite(&Kern::CurrentThread(), a1, info, 0, 0, NULL);
			}
			
		case RKernAsmBmLdd::ERun:
			{
			TPckgBuf<TBmParams> params;
			Kern::ThreadDesRead(&Kern::CurrentThread(), a1, params, 0, 0);			
			UserPtr = (TUint8*)ALIGN_ADDR(a2);
			
			TInt delta;
			TInt r = KernelBenchmarks[index]->Run(params(), delta);
			if (r == KErrNone)
				r = Kern::ThreadRawWrite(&Kern::CurrentThread(), a2, &delta, sizeof(TInt));
			return r;
			}

		default:
			return KErrArgument;
		}
	}

void DKernAsmBm::HandleMsg(TMessageBase* /*aMsg*/)
	{
	}
