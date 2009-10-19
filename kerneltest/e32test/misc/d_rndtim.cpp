// Copyright (c) 1997-2009 Nokia Corporation and/or its subsidiary(-ies).
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
// e32test\misc\d_rndtim.cpp
// LDD for generating random interrupts
// 
//

#include "platform.h"
#include <kernel/kern_priv.h>
#include "d_rndtim.h"
#include "../misc/prbs.h"

#if defined(__MAWD__)
#include <windermere.h>
#elif defined(__MISA__)
#define INT_ID KIntIdOstMatchGeneral
#include <sa1100.h>
#elif defined(__MCOT__)
#define INT_ID KIntIdOstMatchGeneral
#include <cotulla.h>
#elif defined(__MI920__) || defined(__NI1136__)
#include <integratorap.h>
#elif defined(__EPOC32__) && defined(__CPU_X86)
#include <x86.h>
#endif

#ifndef INT_ID
#error Random timer ISR not supported on this platform
#endif


const TInt KMajorVersionNumber=0;
const TInt KMinorVersionNumber=1;
const TInt KBuildVersionNumber=1;

class DRndTimFactory : public DLogicalDevice
//
// IPC copy LDD factory
//
	{
public:
	DRndTimFactory();
	virtual TInt Install();						//overriding pure virtual
	virtual void GetCaps(TDes8& aDes) const;	//overriding pure virtual
	virtual TInt Create(DLogicalChannelBase*& aChannel);	//overriding pure virtual
	};

class DRndTim : public DLogicalChannelBase
//
// Millisecond timer LDD channel
//
	{
public:
	DRndTim();
	virtual ~DRndTim();
protected:
	virtual TInt DoCreate(TInt aUnit, const TDesC8* aInfo, const TVersion& aVer);
	virtual TInt Request(TInt aReqNo, TAny* a1, TAny* a2);
public:
	static void TimerIsr(TAny* aPtr);
	static void IDfcFn(TAny* aPtr);
	void StartTimer();
	void StopTimer();
	TInt SetPriority(TInt aHandle, TInt aPriority);
	TInt Calibrate(TInt aMilliseconds);
public:
	TInt iIntId;
	NFastSemaphore iSem;
	volatile TUint32 iSeed[2];
	volatile TUint32 iIsrCount;
	DThread* iThread;
	TDfc iIDfc;
	};

DECLARE_STANDARD_LDD()
	{
    return new DRndTimFactory;
    }

DRndTimFactory::DRndTimFactory()
//
// Constructor
//
    {
    iVersion=TVersion(KMajorVersionNumber,KMinorVersionNumber,KBuildVersionNumber);
    //iParseMask=0;//No units, no info, no PDD
    //iUnitsMask=0;//Only one thing
    }

TInt DRndTimFactory::Create(DLogicalChannelBase*& aChannel)
//
// Create a new DRndTim on this logical device
//
    {
	aChannel=new DRndTim;
    return aChannel?KErrNone:KErrNoMemory;
    }

TInt DRndTimFactory::Install()
//
// Install the LDD - overriding pure virtual
//
    {
    return SetName(&KRndTimLddName);
    }

void DRndTimFactory::GetCaps(TDes8& aDes) const
//
// Get capabilities - overriding pure virtual
//
    {
    TCapsRndTimV01 b;
    b.iVersion=TVersion(KMajorVersionNumber,KMinorVersionNumber,KBuildVersionNumber);
    Kern::InfoCopy(aDes,(TUint8*)&b,sizeof(b));
    }

DRndTim::DRndTim()
//
// Constructor
//
	:	iIntId(-1),
		iIDfc(&IDfcFn, this)
    {
	iThread=&Kern::CurrentThread();
	iThread->Open();
	iSem.iOwningThread = &iThread->iNThread;
	iSeed[0] = 0xb504f333u;
	iSeed[1] = 0xf9de6484u;
    }

DRndTim::~DRndTim()
	{
	StopTimer();
	if (iIntId >= 0)
		Interrupt::Unbind(iIntId);
	Kern::SafeClose((DObject*&)iThread, NULL);
	}

TInt DRndTim::DoCreate(TInt /*aUnit*/, const TDesC8* /*anInfo*/, const TVersion& aVer)
//
// Create channel
//
    {

    if (!Kern::QueryVersionSupported(TVersion(KMajorVersionNumber,KMinorVersionNumber,KBuildVersionNumber),aVer))
    	return KErrNotSupported;
	StopTimer();
	TInt r = Interrupt::Bind(INT_ID, &TimerIsr, this);
	if (r == KErrNone)
		iIntId = INT_ID;
	return r;
	}

TInt DRndTim::Request(TInt aFunction, TAny* a1, TAny* a2)
	{
	TInt r = KErrNotSupported;
	switch (aFunction)
		{
		case RRndTim::EControlWait:
			NKern::FSWait(&iSem);
			r = KErrNone;
			break;
		case RRndTim::EControlSetPriority:
			r = SetPriority(TInt(a1), TInt(a2));
			break;
		case RRndTim::EControlStartTimer:
			NKern::ThreadEnterCS();
			StartTimer();
			NKern::ThreadLeaveCS();
			break;
		case RRndTim::EControlStopTimer:
			NKern::ThreadEnterCS();
			StopTimer();
			NKern::ThreadLeaveCS();
			break;
		case RRndTim::EControlCalibrate:
			NKern::ThreadEnterCS();
			r = Calibrate(TInt(a1));
			NKern::ThreadLeaveCS();
			break;
		default:
			break;
		}
	return r;
	}

TInt DRndTim::SetPriority(TInt aHandle, TInt aPriority)
	{
	TInt r = KErrBadHandle;
	DThread& c = Kern::CurrentThread();
	NKern::ThreadEnterCS();
	NKern::LockSystem();
	DThread* t = (DThread*)Kern::ObjectFromHandle(&c, aHandle, EThread);
	if (t && !t->Open())
		{
		NKern::UnlockSystem();
		r = Kern::SetThreadPriority(aPriority, t);
		t->Close(NULL);
		}
	else
		NKern::UnlockSystem();
	NKern::ThreadLeaveCS();
	return r;
	}

TInt DRndTim::Calibrate(TInt aMilliseconds)
	{
	TUint32 n1, n2;
	TInt ticks = NKern::TimerTicks(aMilliseconds);
	n1 = iIsrCount;
	NKern::Sleep(ticks);
	n2 = iIsrCount;
	return (TInt)(n2-n1);
	}

void DRndTim::StartTimer()
	{
#if defined(__MISA__) 
	// for SA11x0 use OST match 0
	TSa1100::ModifyIntLevels(0,KHtIntsOstMatchGeneral);	// route new timer interrupt to FIQ
	TSa1100::SetOstMatchEOI(KHwOstMatchGeneral);
	TUint oscr=TSa1100::OstData();
	TSa1100::SetOstMatch(KHwOstMatchGeneral, oscr + 5000);
	TSa1100::EnableOstInterrupt(KHwOstMatchGeneral);
#elif defined(__MCOT__)
	// for SA11x0 use OST match 0
	TCotulla::ModifyIntLevels(0,KHtIntsOstMatchGeneral);	// route new timer interrupt to FIQ
	TCotulla::SetOstMatchEOI(KHwOstMatchGeneral);
	TUint oscr=TCotulla::OstData();
	TCotulla::SetOstMatch(KHwOstMatchGeneral, oscr + 5000);
	TCotulla::EnableOstInterrupt(KHwOstMatchGeneral);
#endif
	Interrupt::Enable(INT_ID);
	}

void DRndTim::StopTimer()
	{
#if defined(__MISA__) 
	Interrupt::Disable(KIntIdOstMatchGeneral);
	TSa1100::DisableOstInterrupt(KHwOstMatchGeneral);
	TSa1100::SetOstMatchEOI(KHwOstMatchGeneral);
#elif defined(__MCOT__)
	Interrupt::Disable(KIntIdOstMatchGeneral);
	TCotulla::DisableOstInterrupt(KHwOstMatchGeneral);
	TCotulla::SetOstMatchEOI(KHwOstMatchGeneral);
#endif
	}

void DRndTim::TimerIsr(TAny* aPtr)
	{
	DRndTim* d = (DRndTim*)aPtr;
	++d->iIsrCount;
#if defined(__MISA__) 
	TUint interval = Random((TUint*)d->iSeed);
	interval &= 0x3ff;
	interval += 256;	// 256-1279 ticks = approx 69 to 347 microseconds
	TUint oscr=TSa1100::OstData();
	TSa1100::SetOstMatch(KHwOstMatchGeneral, oscr + interval);
	TSa1100::SetOstMatchEOI(KHwOstMatchGeneral);
#elif defined(__MCOT__)
	TUint interval = Random((TUint*)d->iSeed);
	interval &= 0x3ff;
	interval += 256;	// 256-1279 ticks = approx 69 to 347 microseconds
	TUint oscr=TCotulla::OstData();
	TCotulla::SetOstMatch(KHwOstMatchGeneral, oscr + interval);
	TCotulla::SetOstMatchEOI(KHwOstMatchGeneral);
#endif
	d->iIDfc.Add();
	}

void DRndTim::IDfcFn(TAny* aPtr)
	{
	DRndTim* d = (DRndTim*)aPtr;
	d->iSem.Signal();
	}


