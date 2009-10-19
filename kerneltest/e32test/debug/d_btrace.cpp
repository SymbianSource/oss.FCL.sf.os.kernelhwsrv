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
// e32\drivers\trace\btrace.cpp
// 
//

#include <kernel/kern_priv.h>
#include "platform.h"
#include "d_btrace.h"


class DBTraceTestFactory : public DLogicalDevice
	{
public:
	virtual TInt Install();
	virtual void GetCaps(TDes8& aDes) const;
	virtual TInt Create(DLogicalChannelBase*& aChannel);
	};


class DBTraceTestChannel : public DLogicalChannelBase
	{
public:
	DBTraceTestChannel();
	virtual ~DBTraceTestChannel();
	//	Inherited from DObject
	virtual TInt RequestUserHandle(DThread* aThread, TOwnerType aType);
	// Inherited from DLogicalChannelBase
	virtual TInt DoCreate(TInt aUnit, const TDesC8* anInfo, const TVersion& aVer);
	virtual TInt Request(TInt aReqNo, TAny* a1, TAny* a2);
private:
	DThread* iClient;
public:
	static void TestTrace(DBTraceTestChannel* aSelf);
	static void TestUTrace(DBTraceTestChannel* aSelf);
private:
	TUint32 iTestType;
	TInt iTestDataSize;
	TUint32 iTestData[KMaxBTraceRecordSize*2/4];
	volatile TBool iTimerExpired;
	NTimer iTraceTimer;
	TDfc iTraceIDFC;
	};


//
// DBTraceTestFactory
//

TInt DBTraceTestFactory::Install()
	{
	return SetName(&RBTraceTest::Name());
	}

void DBTraceTestFactory::GetCaps(TDes8& aDes) const
	{
	Kern::InfoCopy(aDes,0,0);
	}

TInt DBTraceTestFactory::Create(DLogicalChannelBase*& aChannel)
	{
	aChannel=new DBTraceTestChannel();
	if(!aChannel)
		return KErrNoMemory;
	return KErrNone;
	}


//
// DBTraceTestChannel
//

DBTraceTestChannel::DBTraceTestChannel()
	: iTraceTimer((NTimerFn)TestTrace,this),
	iTraceIDFC((NTimerFn)TestTrace,this)
	{
	}

DBTraceTestChannel::~DBTraceTestChannel()
	{
	iTraceTimer.Cancel();
	}

TInt DBTraceTestChannel::DoCreate(TInt /*aUnit*/, const TDesC8* /*aInfo*/, const TVersion& /*aVer*/)
	{
	iClient = &Kern::CurrentThread();
	return KErrNone;
	}


TInt DBTraceTestChannel::RequestUserHandle(DThread* aThread, TOwnerType aType)
	{
	if (aType!=EOwnerThread || aThread!=iClient)
		return KErrAccessDenied;
	return KErrNone;
	}

TInt DBTraceTestChannel::Request(TInt aReqNo, TAny* a1, TAny* a2)
	{
	switch(aReqNo)
		{
	// test functions
	case RBTraceTest::ETestSpecialTrace:
	case RBTraceTest::ETestTrace:
		{
		TPtr8 data((TUint8*)&iTestData,sizeof(iTestData));
		Kern::KUDesGet(data,*(TDesC8*)a2);
		iTestDataSize = data.Size()-4;
		if(aReqNo==RBTraceTest::ETestSpecialTrace)
			{
			iTestType = (TUint)a1;
			iTimerExpired = EFalse;
			if(iTestType&RBTraceTest::EContextIsr)
				iTraceTimer.OneShot(1);
			else if(iTestType&RBTraceTest::EContextIDFC)
				{
				NKern::Lock();
				iTraceIDFC.Add();
				NKern::Unlock();
				}
			else if(iTestType&RBTraceTest::EContextIntsOff)
				{
				TInt irq = NKern::DisableAllInterrupts();
				TestTrace(this);
				NKern::RestoreInterrupts(irq);
				}
			else
				TestTrace(this);
			while(!__e32_atomic_load_acq32(&iTimerExpired)) {};
			return (TInt)NKern::CurrentThread();
			}
		else
			{
			TInt delay = (TInt)a1/NKern::TickPeriod();
			iTestType = 0;
			if(!delay)
				TestTrace(this);
			else
				{
				iTraceTimer.Cancel();
				iTraceTimer.OneShot(delay+1);
				}
			}
		}
		return KErrNone;

	case RBTraceTest::ETestBenchmark:
		{
		TInt delay = (TInt)a2/NKern::TickPeriod();
		TInt size = (TInt)a1;
		iTestDataSize = -1;

		// wait for next tick...
		iTraceTimer.Cancel();
		iTimerExpired = EFalse;
		iTraceTimer.OneShot(1);
		while(!__e32_atomic_load_acq32(&iTimerExpired)) {};

		// do benchmark...
		iTimerExpired = EFalse;
		iTraceTimer.OneShot(delay+1);
		TInt count = 0;
		if(size)
			for(;;)
				{
				TBool finished = __e32_atomic_load_acq32(&iTimerExpired);
				BTraceContextN(BTrace::ETest1,0,0,0,&iTestData,size);
				++count;
				if(!finished)
					continue;
				break;
				}
		else
			for(;;)
				{
				TBool finished = __e32_atomic_load_acq32(&iTimerExpired);
				BTrace0(BTrace::ETest1,0);
				++count;
				if(!finished)
					continue;
				break;
				}
		return count;
		}

	case RBTraceTest::ETestBenchmark2:
		{
		TInt delay = (TInt)a2/NKern::TickPeriod();
		TInt size = (TInt)a1;
		iTestDataSize = -1;

		// wait for next tick...
		iTraceTimer.Cancel();
		iTimerExpired = EFalse;
		iTraceTimer.OneShot(1);
		while(!__e32_atomic_load_acq32(&iTimerExpired)) {};

		// do benchmark...
		iTimerExpired = EFalse;
		iTraceTimer.OneShot(delay+1);
		TInt count = 0;
		if(size)
			for(;;)
				{
				TBool finished = __e32_atomic_load_acq32(&iTimerExpired);
				BTraceFilteredContextN(BTrace::ETest1,0,KBTraceFilterTestUid1,0,&iTestData,size);
				++count;
				if(!finished)
					continue;
				break;
				}
		else
			for(;;)
				{
				TBool finished = __e32_atomic_load_acq32(&iTimerExpired);
				BTraceFiltered4(BTrace::ETest1,0,KBTraceFilterTestUid1);
				++count;
				if(!finished)
					continue;
				break;
				}
		return count;
		}

	case RBTraceTest::ETestBenchmarkCheckFilter:
		{
		TInt delay = (TInt)a2/NKern::TickPeriod();

		// wait for next tick...
		iTraceTimer.Cancel();
		iTimerExpired = EFalse;
		iTraceTimer.OneShot(1);
		while(!__e32_atomic_load_acq32(&iTimerExpired)) {};

		// do benchmark...
		iTimerExpired = EFalse;
		iTraceTimer.OneShot(delay+1);
		TInt count = 0;
		if(a1)
			for(;;)
				{
				TBool finished = __e32_atomic_load_acq32(&iTimerExpired);
				BTrace::CheckFilter2(BTrace::ETest1,KBTraceFilterTestUid1);
				++count;
				if(!finished)
					continue;
				break;
				}
		else
			for(;;)
				{
				TBool finished = __e32_atomic_load_acq32(&iTimerExpired);
				BTrace::CheckFilter(BTrace::ETest1);
				++count;
				if(!finished)
					continue;
				break;
				}
		return count;
		}
	case RBTraceTest::ETestUTrace:
		{
		TPtr8 data((TUint8*)&iTestData,sizeof(iTestData));
		Kern::KUDesGet(data,*(TDesC8*)a2);
		iTestDataSize = data.Size()-4;
		TInt delay = (TInt)a1/NKern::TickPeriod();
		iTestType = 0;
		if(!delay)
			TestUTrace(this);
		else
			{
			iTraceTimer.Cancel();
			iTraceTimer.OneShot(delay+1);
			}
		return KErrNone;
		}

	default:
		break;
		}
	return KErrNotSupported;
	}


void DBTraceTestChannel::TestTrace(DBTraceTestChannel* aSelf)
	{
	TInt size = aSelf->iTestDataSize;
	if(size<0)
		{
		__e32_atomic_store_rel32(&aSelf->iTimerExpired, 1);
		return;
		}
	TUint32* data = aSelf->iTestData;
	BTrace::TCategory category = (BTrace::TCategory)((TUint8*)data)[0];
	TUint subCategory = (BTrace::TCategory)((TUint8*)data)[1];
	TUint type = aSelf->iTestType&0xff;
	TBool bigTrace = aSelf->iTestType&RBTraceTest::EBigTrace;
	TBool filter2Trace = aSelf->iTestType&RBTraceTest::EFilter2Trace;

	if(!filter2Trace)
		{
		if(type==BTrace::EPcPresent)
			{
			if(bigTrace)
				{
				BTracePcBig(category,subCategory,data[1],data+2,size-4);
				BTracePcBig(category,subCategory,data[1],data+2,size-4);
				}
			else if(size==0)
				{
				BTracePc0(category,subCategory);
				BTracePc0(category,subCategory);
				}
			else if(size<=4)
				{
				BTracePc4(category,subCategory,data[1]);
				BTracePc4(category,subCategory,data[1]);
				}
			else if(size<=8)
				{
				BTracePc8(category,subCategory,data[1],data[2]);
				BTracePc8(category,subCategory,data[1],data[2]);
				}
			else
				{
				BTracePcN(category,subCategory,data[1],data[2],data+3,size-8);
				BTracePcN(category,subCategory,data[1],data[2],data+3,size-8);
				}
			}
		else if(type==BTrace::EContextIdPresent)
			{
			if(bigTrace)
				{
				BTraceContextBig(category,subCategory,data[1],data+2,size-4);
				BTraceContextBig(category,subCategory,data[1],data+2,size-4);
				}
			else if(size==0)
				{
				BTraceContext0(category,subCategory);
				BTraceContext0(category,subCategory);
				}
			else if(size<=4)
				{
				BTraceContext4(category,subCategory,data[1]);
				BTraceContext4(category,subCategory,data[1]);
				}
			else if(size<=8)
				{
				BTraceContext8(category,subCategory,data[1],data[2]);
				BTraceContext8(category,subCategory,data[1],data[2]);
				}
			else
				{
				BTraceContextN(category,subCategory,data[1],data[2],data+3,size-8);
				BTraceContextN(category,subCategory,data[1],data[2],data+3,size-8);
				}
			}
		else if(type==BTrace::EContextIdPresent+BTrace::EPcPresent)
			{
			if(bigTrace)
				{
				BTraceContextPcBig(category,subCategory,data[1],data+2,size-4);
				BTraceContextPcBig(category,subCategory,data[1],data+2,size-4);
				}
			else if(size==0)
				{
				BTraceContextPc0(category,subCategory);
				BTraceContextPc0(category,subCategory);
				}
			else if(size<=4)
				{
				BTraceContextPc4(category,subCategory,data[1]);
				BTraceContextPc4(category,subCategory,data[1]);
				}
			else if(size<=8)
				{
				BTraceContextPc8(category,subCategory,data[1],data[2]);
				BTraceContextPc8(category,subCategory,data[1],data[2]);
				}
			else
				{
				BTraceContextPcN(category,subCategory,data[1],data[2],data+3,size-8);
				BTraceContextPcN(category,subCategory,data[1],data[2],data+3,size-8);
				}
			}
		else
			{
			if(bigTrace)
				BTraceBig(category,subCategory,data[1],data+2,size-4);
			else if(size==0)
				BTrace0(category,subCategory);
			else if(size<=4)
				BTrace4(category,subCategory,data[1]);
			else if(size<8)
				BTrace8(category,subCategory,data[1],data[2]);
			else
				BTraceN(category,subCategory,data[1],data[2],data+3,size-8);
			}
		}
	else
		{
		if(type==BTrace::EPcPresent)
			{
			if(bigTrace)
				{
				BTraceFilteredPcBig(category,subCategory,data[1],data+2,size-4);
				BTraceFilteredPcBig(category,subCategory,data[1],data+2,size-4);
				}
			else if(size<4)
				{
				// invalid
				}
			else if(size==4)
				{
				BTraceFilteredPc4(category,subCategory,data[1]);
				BTraceFilteredPc4(category,subCategory,data[1]);
				}
			else if(size<=8)
				{
				BTraceFilteredPc8(category,subCategory,data[1],data[2]);
				BTraceFilteredPc8(category,subCategory,data[1],data[2]);
				}
			else
				{
				BTraceFilteredPcN(category,subCategory,data[1],data[2],data+3,size-8);
				BTraceFilteredPcN(category,subCategory,data[1],data[2],data+3,size-8);
				}
			}
		else if(type==BTrace::EContextIdPresent)
			{
			if(bigTrace)
				{
				BTraceFilteredContextBig(category,subCategory,data[1],data+2,size-4);
				BTraceFilteredContextBig(category,subCategory,data[1],data+2,size-4);
				}
			else if(size<4)
				{
				// invalid
				}
			else if(size==4)
				{
				BTraceFilteredContext4(category,subCategory,data[1]);
				BTraceFilteredContext4(category,subCategory,data[1]);
				}
			else if(size<=8)
				{
				BTraceFilteredContext8(category,subCategory,data[1],data[2]);
				BTraceFilteredContext8(category,subCategory,data[1],data[2]);
				}
			else
				{
				BTraceFilteredContextN(category,subCategory,data[1],data[2],data+3,size-8);
				BTraceFilteredContextN(category,subCategory,data[1],data[2],data+3,size-8);
				}
			}
		else if(type==BTrace::EContextIdPresent+BTrace::EPcPresent)
			{
			if(bigTrace)
				{
				BTraceFilteredContextPcBig(category,subCategory,data[1],data+2,size-4);
				BTraceFilteredContextPcBig(category,subCategory,data[1],data+2,size-4);
				}
			else if(size<4)
				{
				// invalid
				}
			else if(size==4)
				{
				BTraceFilteredContextPc4(category,subCategory,data[1]);
				BTraceFilteredContextPc4(category,subCategory,data[1]);
				}
			else if(size<=8)
				{
				BTraceFilteredContextPc8(category,subCategory,data[1],data[2]);
				BTraceFilteredContextPc8(category,subCategory,data[1],data[2]);
				}
			else
				{
				BTraceFilteredContextPcN(category,subCategory,data[1],data[2],data+3,size-8);
				BTraceFilteredContextPcN(category,subCategory,data[1],data[2],data+3,size-8);
				}
			}
		else
			{
			if(bigTrace)
				BTraceFilteredBig(category,subCategory,data[1],data+2,size-4);
			else if(size<4)
				{
				// invalid
				}
			else if(size==4)
				BTraceFiltered4(category,subCategory,data[1]);
			else if(size<8)
				BTraceFiltered8(category,subCategory,data[1],data[2]);
			else
				BTraceFilteredN(category,subCategory,data[1],data[2],data+3,size-8);
			}
		}
	__e32_atomic_store_rel32(&aSelf->iTimerExpired, 1);
	}

void DBTraceTestChannel::TestUTrace(DBTraceTestChannel* aSelf)
	{
	aSelf->iTimerExpired = ETrue;
	TInt size = aSelf->iTestDataSize;
	if(size<0)
		return;
	TUint32* data = aSelf->iTestData;
	BTrace::TCategory category = (BTrace::TCategory)((TUint8*)data)[0];
	TUint subCategory = (BTrace::TCategory)((TUint8*)data)[1];

	#define T_UTRACE_HEADER(aSize,aClassification,aContext,aPc)																\
		((((aSize) + (aContext?4:0) + (aPc?4:0)) << BTrace::ESizeIndex*8)										\
		+(((aContext?BTrace::EContextIdPresent:0) | (aPc?BTrace::EPcPresent:0)) << BTrace::EFlagsIndex*8)			\
		+((aClassification) << BTrace::ECategoryIndex*8)																				\
		+((subCategory) << BTrace::ESubCategoryIndex*8))

	#define UTRACE_SECONDARY(aClassification,aModuleUid,aThreadIdPresent,aPcPresent,aPc,aFormatId)	\
		BTrace::OutFilteredPcFormatBig(T_UTRACE_HEADER(8,aClassification,aThreadIdPresent,aPcPresent),(TUint32)(aModuleUid),aPc,aFormatId,0,0)

	#define UTRACE_SECONDARY_4(aClassification,aModuleUid,aThreadIdPresent,aPcPresent,aPc,aFormatId, aData1) \
		BTrace::OutFilteredPcFormatBig(T_UTRACE_HEADER(8,aClassification,aThreadIdPresent,aPcPresent),(TUint32)(aModuleUid),aPc,aFormatId,&aData1,4)

	#define UTRACE_SECONDARY_ANY(aClassification, aModuleUid, aThreadIdPresent, aPcPresent, aPc, aFormatId, aData, aDataSize) \
		BTrace::OutFilteredPcFormatBig(T_UTRACE_HEADER(8,aClassification,aThreadIdPresent,aPcPresent),(TUint32)(aModuleUid),aPc,aFormatId,aData,(TInt)(aDataSize))


	TUint32 KUtracePcValues[3]={0, 0x123456, 0x987654};
	TUint16 formatId = (TUint16)data[2];
	TUint type = aSelf->iTestType&0xff;
	if(type == BTrace::EPcPresent)
		{
		if(size <= 0)
			{
			UTRACE_SECONDARY(category, data[1], EFalse, ETrue, KUtracePcValues[1], formatId);
			UTRACE_SECONDARY(category, data[1], EFalse, ETrue, KUtracePcValues[2], formatId);
			}
		else if(size <= 4)
			{
			UTRACE_SECONDARY_4(category, data[1], EFalse, ETrue, KUtracePcValues[1], formatId, data[3]);
			UTRACE_SECONDARY_4(category, data[1], EFalse, ETrue, KUtracePcValues[2], formatId, data[3]);
			}
		else //size > 8
			{
			UTRACE_SECONDARY_ANY(category, data[1], EFalse, ETrue, KUtracePcValues[1], formatId, data+3, size);
			UTRACE_SECONDARY_ANY(category, data[1], EFalse, ETrue, KUtracePcValues[2], formatId, data+3, size);
			}
		}
	else if(type==BTrace::EContextIdPresent)
		{
		if(size <= 0)
			{
			UTRACE_SECONDARY(category, data[1], ETrue, EFalse, KUtracePcValues[1], formatId);
			UTRACE_SECONDARY(category, data[1], ETrue, EFalse, KUtracePcValues[2], formatId);
			}
		else if(size <= 4)
			{
			UTRACE_SECONDARY_4(category, data[1], ETrue, EFalse, KUtracePcValues[1], formatId, data[3]);
			UTRACE_SECONDARY_4(category, data[1], ETrue, EFalse, KUtracePcValues[2], formatId, data[3]);
			}
		else //size > 8
			{
			UTRACE_SECONDARY_ANY(category, data[1], ETrue, EFalse, KUtracePcValues[1], formatId, data+3, size);
			UTRACE_SECONDARY_ANY(category, data[1], ETrue, EFalse, KUtracePcValues[2], formatId, data+3, size);
			}
		}
	else if(type==BTrace::EContextIdPresent+BTrace::EPcPresent)
		{
		if(size <= 0)
			{
			UTRACE_SECONDARY(category, data[1], ETrue, ETrue, KUtracePcValues[1], formatId);
			UTRACE_SECONDARY(category, data[1], ETrue, ETrue, KUtracePcValues[2], formatId);
			}
		else if(size <= 4)
			{
			UTRACE_SECONDARY_4(category, data[1], ETrue, ETrue, KUtracePcValues[1], formatId, data[3]);
			UTRACE_SECONDARY_4(category, data[1], ETrue, ETrue, KUtracePcValues[2], formatId, data[3]);
			}
		else //size > 8
			{
			UTRACE_SECONDARY_ANY(category, data[1], ETrue, ETrue, KUtracePcValues[1], formatId, data+3, size);
			UTRACE_SECONDARY_ANY(category, data[1], ETrue, ETrue, KUtracePcValues[2], formatId, data+3, size);
			}
		}
	else
		{
		if(size <= 0)
			{
			UTRACE_SECONDARY(category, data[1], EFalse, EFalse, KUtracePcValues[1], formatId);
			UTRACE_SECONDARY(category, data[1], EFalse, EFalse, KUtracePcValues[2], formatId);
			}
		else if(size <= 4)
			{
			UTRACE_SECONDARY_4(category, data[1], EFalse, EFalse, KUtracePcValues[1], formatId, data[3]);
			UTRACE_SECONDARY_4(category, data[1], EFalse, EFalse, KUtracePcValues[2], formatId, data[3]);
			}
		else //size > 8
			{
			UTRACE_SECONDARY_ANY(category, data[1], EFalse, EFalse, KUtracePcValues[1], formatId, data+3, size);
			UTRACE_SECONDARY_ANY(category, data[1], EFalse, EFalse, KUtracePcValues[2], formatId, data+3, size);
			}
		}

	}

DECLARE_STANDARD_LDD()
	{
	return new DBTraceTestFactory;
	}


