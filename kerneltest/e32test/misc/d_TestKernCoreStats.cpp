// Copyright (c) 2008-2010 Nokia Corporation and/or its subsidiary(-ies).
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
// e32test\misc\d_kerncorestas.cpp
// 
//

#include "d_TestKernCoreStats.h"

#include <kernel/kernel.h>
#include <kernel/kerncorestats.h>


class DTestKernCoreStatsFactory : public DLogicalDevice
        {
public:
        DTestKernCoreStatsFactory();
        ~DTestKernCoreStatsFactory();
        virtual TInt Install();
        virtual void GetCaps(TDes8& aDes) const;
        virtual TInt Create(DLogicalChannelBase*& aChannel);
        };

class DTestKernCoreStatsChannel : public DLogicalChannelBase
        {
public:
        DTestKernCoreStatsChannel();
        virtual ~DTestKernCoreStatsChannel();
        TInt Request(TInt aFunction, TAny* a1, TAny* a2);
        virtual TInt DoCreate(TInt aUnit, const TDesC8* anInfo, const TVersion&
aVer);
private:
	TInt GetStats(TAny* aBuffer);
	TInt Configure(TInt aMode);
	TInt DumpInfo();
private:
	TInt iLength;
        };


DECLARE_STANDARD_LDD()
        {
        return new DTestKernCoreStatsFactory;
        }

DTestKernCoreStatsFactory::DTestKernCoreStatsFactory()
        {
        }

DTestKernCoreStatsFactory::~DTestKernCoreStatsFactory()
        {
        }

TInt DTestKernCoreStatsFactory::Install()
        {
        return SetName(&KTestKernCoreStatsName);
        }


void DTestKernCoreStatsFactory::GetCaps(TDes8&) const
        {
        }


TInt DTestKernCoreStatsFactory::Create(DLogicalChannelBase*& aChannel)
        {
        aChannel=new DTestKernCoreStatsChannel();
        if(!aChannel)
                return KErrNoMemory;

        return KErrNone;
        }

DTestKernCoreStatsChannel::DTestKernCoreStatsChannel()
	{
	}


DTestKernCoreStatsChannel::~DTestKernCoreStatsChannel()
	{
	}

TInt DTestKernCoreStatsChannel::DoCreate(TInt, const TDesC8*, const TVersion&)
        {
        return KErrNone;
        }

TInt DTestKernCoreStatsChannel::Request(TInt aFunction, TAny* a1, TAny*)
        {
        switch (aFunction)
                {
                case RTestKernCoreStats::ERequestGetStats:
                        return GetStats( (TAny*) a1);
                case RTestKernCoreStats::ERequestConfigure:
                        return Configure( (TInt) a1);
                case RTestKernCoreStats::ERequestDumpInfo:
                        return DumpInfo();
                default:
                        return KErrNotSupported;
                }
        }


TInt DTestKernCoreStatsChannel::GetStats(TAny* aBuffer)
	{
	NKern::ThreadEnterCS();
	
	if (iLength==0)
		return KErrNone;
	
	TAny* tempBuff = Kern::Alloc(iLength);
	if (!tempBuff)
		{
		NKern::ThreadLeaveCS();
		return KErrNoMemory;
		}

	TInt r = KernCoreStats::Stats(tempBuff);


	if (r==KErrNone)
		kumemput(aBuffer, tempBuff, iLength);

	Kern::Free(tempBuff);
	NKern::ThreadLeaveCS();

	return r;
	}

TInt DTestKernCoreStatsChannel::Configure(TInt aMode)
	{

	NKern::ThreadEnterCS();
	TInt cores = NKern::NumberOfCpus();

	TInt len = 0;
	len+= (aMode & KStatsCoreTotalTimeInIdle)?		sizeof(TUint)*cores :0;
	len+= (aMode & KStatsTimeCrossIdleAndActive)?		sizeof(TUint)*(cores+1) :0;
	len+= (aMode & KStatsCoreNumTimesInIdle)?		sizeof(TUint)*cores :0;
	len+= (aMode & KStatsNumEvents)?			sizeof(TUint) :0;
	len+= (aMode & KStatsReadyStateChanges)?		sizeof(TUint)*2:0;
	len+= (aMode & KStatsNumTimeSliceExpire)?		sizeof(TUint):0;

	iLength=len;
	Kern::Printf("KernCoreStats packet length = %d", len);
	TInt r =  KernCoreStats::Configure(aMode);
	NKern::ThreadLeaveCS();
	return r;
	}


TInt DTestKernCoreStatsChannel::DumpInfo()
	{

	//KernCoreStats::Engage(0);

	return iLength;
	}
