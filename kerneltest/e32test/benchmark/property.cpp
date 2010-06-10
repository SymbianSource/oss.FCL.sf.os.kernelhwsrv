// Copyright (c) 2002-2009 Nokia Corporation and/or its subsidiary(-ies).
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

#include <e32test.h>
#include <e32property.h>

#include "bm_suite.h"

static const TInt32 KUidPropBenchmarkCategoryValue = 0x101f75b7;
static const TUid KPropBenchmarkCategory = { KUidPropBenchmarkCategoryValue };
static _LIT_SECURITY_POLICY_PASS(KPassPolicy);

typedef void (*MeasurementFunc)(TBMResult*, TBMUInt64 aIter, struct Measurement*);
enum TSetGetType
	{
	EOneArg,
	EThreeArgsInt,
	EThreeArgsBuf8,
	EThreeArgsBuf16
	};

struct Measurement 
	{
	MeasurementFunc		iFunc;
	TPtrC				iName;
	TBool				iRemote;
	RProperty::TType	iType;
	TInt				iSize;
	TSetGetType			iSetGetType;

	Measurement(MeasurementFunc aFunc, const TDesC& aName,
				RProperty::TType aType, TInt aSize, TBool aRemote = EFalse, 
				TSetGetType aSetGetType = EOneArg) : 
			iFunc(aFunc), iName(aName), iRemote(aRemote), iType(aType), iSize(aSize), 
			iSetGetType(aSetGetType) {}
	};

class Property : public BMProgram
	{
public :
	Property() : BMProgram(_L("Properties"))
		{}
	virtual TBMResult* Run(TBMUInt64 aIter, TInt* aCount);


	static TBMResult	iResults[];
	static Measurement	iMeasurements[];

	static void NotificationLatencyParent(TBMResult* aResult, TBMUInt64 aIter, Measurement*);
	static TInt NotificationLatencyChild(TAny*);

	static void SetOverhead(TBMResult* aResult, TBMUInt64 aIter, struct Measurement* aM);
	static void GetOverhead(TBMResult* aResult, TBMUInt64 aIter, struct Measurement* aM);


private:
	static TBuf8<RProperty::KMaxPropertySize> iInBuf;
	static TBuf8<RProperty::KMaxPropertySize> iOutBuf;
	static TBuf16<RProperty::KMaxPropertySize> iInBuf16;
	static TBuf16<RProperty::KMaxPropertySize> iOutBuf16;
	};

Measurement Property::iMeasurements[] =
	{
	Measurement(&Property::NotificationLatencyParent, _L("Local Int Notification Latency "), 
				RProperty::EInt, 0, EFalse),
	Measurement(&Property::NotificationLatencyParent, _L("Remote Int Notification Latency"),
				RProperty::EInt, 0,ETrue),
	Measurement(&Property::NotificationLatencyParent, _L("Local Byte(1) Notification Latency"),
				RProperty::EByteArray, 1, EFalse),
	Measurement(&Property::NotificationLatencyParent, _L("Remote Byte(1) Notification Latency"), 
				RProperty::EByteArray, 1, ETrue),
	Measurement(&Property::NotificationLatencyParent, _L("Local Byte(8) Notification Latency"),
				RProperty::EByteArray, 8, EFalse),
	Measurement(&Property::NotificationLatencyParent, _L("Remote Byte(8) Notification Latency"), 
				RProperty::EByteArray, 8,ETrue),
	Measurement(&Property::NotificationLatencyParent, _L("Local Byte(512) Notification Latency"), 
				RProperty::EByteArray, 512, EFalse),
	Measurement(&Property::NotificationLatencyParent, _L("Remote Byte(512) Notification Latency"), 
				RProperty::EByteArray, 512,ETrue),

	Measurement(&Property::NotificationLatencyParent, _L("Local Int Notification Latency ThreeArgsInt"), 
				RProperty::EInt, 0, EFalse, EThreeArgsInt),
	Measurement(&Property::NotificationLatencyParent, _L("Remote Int Notification Latency ThreeArgsInt"),
				RProperty::EInt, 0,ETrue, EThreeArgsInt),
	Measurement(&Property::NotificationLatencyParent, _L("Local Byte(1) Notification Latency ThreeArgsBuf8"),
				RProperty::EByteArray, 1, EFalse, EThreeArgsBuf8),
	Measurement(&Property::NotificationLatencyParent, _L("Remote Byte(1) Notification Latency ThreeArgsBuf8"), 
				RProperty::EByteArray, 1, ETrue, EThreeArgsBuf8),
	Measurement(&Property::NotificationLatencyParent, _L("Local TUint16(1) Notification Latency ThreeArgsBuf16"),
				RProperty::EByteArray, 1, EFalse, EThreeArgsBuf16),
	Measurement(&Property::NotificationLatencyParent, _L("Remote TUint16(1) Notification Latency ThreeArgsBuf16"), 
				RProperty::EByteArray, 1, ETrue, EThreeArgsBuf16),
	Measurement(&Property::NotificationLatencyParent, _L("Local Byte(8) Notification Latency ThreeArgsBuf8"),
				RProperty::EByteArray, 8, EFalse, EThreeArgsBuf8),
	Measurement(&Property::NotificationLatencyParent, _L("Remote Byte(8) Notification Latency ThreeArgsBuf8"), 
				RProperty::EByteArray, 8,ETrue, EThreeArgsBuf8),
	Measurement(&Property::NotificationLatencyParent, _L("Local TUint16(8) Notification Latency ThreeArgsBuf16"),
				RProperty::EByteArray, 8, EFalse, EThreeArgsBuf16),
	Measurement(&Property::NotificationLatencyParent, _L("Remote TUint16(8) Notification Latency ThreeArgsBuf16"), 
				RProperty::EByteArray, 8,ETrue, EThreeArgsBuf16),
	Measurement(&Property::NotificationLatencyParent, _L("Local Byte(512) Notification Latency ThreeArgsBuf8"), 
				RProperty::EByteArray, 512, EFalse, EThreeArgsBuf8),
	Measurement(&Property::NotificationLatencyParent, _L("Remote Byte(512) Notification Latency ThreeArgsBuf8"), 
				RProperty::EByteArray, 512,ETrue, EThreeArgsBuf8),
	Measurement(&Property::NotificationLatencyParent, _L("Local TUint16(512) Notification Latency ThreeArgsBuf16"), 
				RProperty::ELargeByteArray, 512, EFalse, EThreeArgsBuf16),
	Measurement(&Property::NotificationLatencyParent, _L("Remote TUint16(512) Notification Latency ThreeArgsBuf16"), 
				RProperty::ELargeByteArray, 512,ETrue, EThreeArgsBuf16),



	Measurement(&Property::SetOverhead, _L("Int Set Overhead"), 
				RProperty::EInt, 0),
	Measurement(&Property::SetOverhead, _L("Byte(1) Set Overhead"),
				RProperty::EByteArray, 1),
	Measurement(&Property::SetOverhead, _L("Byte(8) Set Overhead"),
				RProperty::EByteArray, 8),
	Measurement(&Property::SetOverhead, _L("Byte(512) Set Overhead"),
				RProperty::EByteArray, 512),

	Measurement(&Property::SetOverhead, _L("Int Set Overhead ThreeArgsInt"), 
				RProperty::EInt, 0, EFalse, EThreeArgsInt),
	Measurement(&Property::SetOverhead, _L("Byte(1) Set Overhead ThreeArgsBuf8"),
				RProperty::EByteArray, 1, EFalse, EThreeArgsBuf8),
	Measurement(&Property::SetOverhead, _L("TUint16(1) Set Overhead ThreeArgsBuf16"),
				RProperty::EByteArray, 1, EFalse, EThreeArgsBuf16),
	Measurement(&Property::SetOverhead, _L("Byte(8) Set Overhead ThreeArgsBuf8"),
				RProperty::EByteArray, 8, EFalse, EThreeArgsBuf8),
	Measurement(&Property::SetOverhead, _L("TUint16(8) Set Overhead ThreeArgsBuf16"),
				RProperty::EByteArray, 8, EFalse, EThreeArgsBuf16),
	Measurement(&Property::SetOverhead, _L("Byte(512) Set Overhead ThreeArgsBuf8"),
				RProperty::EByteArray, 512, EFalse, EThreeArgsBuf8),
	Measurement(&Property::SetOverhead, _L("TUint16(512) Set Overhead ThreeArgsBuf16"),
				RProperty::ELargeByteArray, 512, EFalse, EThreeArgsBuf16),



	Measurement(&Property::GetOverhead, _L("Int Get Overhead"), 
				RProperty::EInt, 0),
	Measurement(&Property::GetOverhead, _L("Byte(1) Get Overhead"),
				RProperty::EByteArray, 1),
	Measurement(&Property::GetOverhead, _L("Byte(8) Get Overhead"),
				RProperty::EByteArray, 8),
	Measurement(&Property::GetOverhead, _L("Byte(512) Get Overhead"),
				RProperty::EByteArray, 512),

	Measurement(&Property::GetOverhead, _L("Int Get Overhead ThreeArgsInt"), 
				RProperty::EInt, 0, EFalse, EThreeArgsInt),
	Measurement(&Property::GetOverhead, _L("Byte(1) Get Overhead ThreeArgsBuf8"),
				RProperty::EByteArray, 1, EFalse, EThreeArgsBuf8),
	Measurement(&Property::GetOverhead, _L("TUint16(1) Get Overhead ThreeArgsBuf16"),
				RProperty::EByteArray, 1, EFalse, EThreeArgsBuf16),
	Measurement(&Property::GetOverhead, _L("Byte(8) Get Overhead ThreeArgsBuf8"),
				RProperty::EByteArray, 8, EFalse, EThreeArgsBuf8),
	Measurement(&Property::GetOverhead, _L("TUint16(8) Get Overhead ThreeArgsBuf16"),
				RProperty::EByteArray, 8, EFalse, EThreeArgsBuf16),
	Measurement(&Property::GetOverhead, _L("Byte(512) Get Overhead ThreeArgsBuf8"),
				RProperty::EByteArray, 512, EFalse, EThreeArgsBuf8),
	Measurement(&Property::GetOverhead, _L("TUint16(512) Get Overhead ThreeArgsBuf16"),
				RProperty::ELargeByteArray, 512, EFalse, EThreeArgsBuf16),

	};
TBMResult	Property::iResults[sizeof(Property::iMeasurements)/sizeof(Property::iMeasurements[0])];

TBuf8<RProperty::KMaxPropertySize> Property::iInBuf(RProperty::KMaxPropertySize);
TBuf8<RProperty::KMaxPropertySize> Property::iOutBuf(RProperty::KMaxPropertySize);
TBuf16<RProperty::KMaxPropertySize> Property::iInBuf16(RProperty::KMaxPropertySize);
TBuf16<RProperty::KMaxPropertySize> Property::iOutBuf16(RProperty::KMaxPropertySize);


static Property property;

class NotificationLatencyArgs : public TBMSpawnArgs
	{
public:
	
	TBMUInt64		iIterationCount;
	RProperty::TType iType;
	TInt			iSize;
	TSetGetType		iSetGetType;

	NotificationLatencyArgs(RProperty::TType aType, TInt aSize, TInt aRemote, TBMUInt64 aIter, 
		TSetGetType aSetGetType);
	};

NotificationLatencyArgs::NotificationLatencyArgs(RProperty::TType aType, TInt aSize, TInt aRemote, 
		 TBMUInt64 aIter, TSetGetType aSetGetType) : 
	TBMSpawnArgs(Property::NotificationLatencyChild, KBMPriorityLow, aRemote, sizeof(*this)),
	iIterationCount(aIter), iType(aType), iSize(aSize), iSetGetType(aSetGetType)
	{
	}

void Property::NotificationLatencyParent(TBMResult* aResult, TBMUInt64 aIter, struct Measurement* aM)
	{
	TRequestStatus st1, st2;

	RProperty time;
	TInt r = time.Define(KPropBenchmarkCategory, 0, RProperty::EByteArray, KPassPolicy, KPassPolicy);
	BM_ERROR(r, r == KErrNone);	
	r = time.Attach(KPropBenchmarkCategory, 0);
	BM_ERROR(r, r == KErrNone);	
	time.Subscribe(st2);

	RProperty prop;	
	r = prop.Define(KPropBenchmarkCategory, 1, aM->iType, KPassPolicy, KPassPolicy);
	BM_ERROR(r, r == KErrNone);	
	r = prop.Attach(KPropBenchmarkCategory, 1);
	BM_ERROR(r, r == KErrNone);	
	prop.Subscribe(st1);

	RSemaphore sync;
	r = sync.CreateGlobal(_L("sync"), 0);
	BM_ERROR(r, r == KErrNone);	

	NotificationLatencyArgs sl(aM->iType, aM->iSize, aM->iRemote, aIter, aM->iSetGetType);
	MBMChild* child = property.SpawnChild(&sl);

	for (TBMUInt64 i = 0; i < aIter; ++i)
		{
		sync.Signal();
		User::WaitForRequest(st1);
		BM_ERROR(st1.Int(), st1.Int() == KErrNone);
		prop.Subscribe(st1);

		switch(aM->iSetGetType)
			{
		case EOneArg:
			if (aM->iType == RProperty::EInt)
				{
				TInt value;
				r = prop.Get(value);
				BM_ERROR(r, r == KErrNone);	
				}
			else 
				{
				r = prop.Get(iInBuf);
				BM_ERROR(r, r == KErrNone);	
				}
			break;
		case EThreeArgsInt:
			{
			TInt value;
			r = prop.Get(KPropBenchmarkCategory, 1, value);
			BM_ERROR(r, r == KErrNone);
			}
			break;
		case EThreeArgsBuf8:
			r = prop.Get(KPropBenchmarkCategory, 1, iInBuf);
			BM_ERROR(r, r == KErrNone);
			break;
		case EThreeArgsBuf16:
			r = prop.Get(KPropBenchmarkCategory, 1, iInBuf16);
			BM_ERROR(r, r == KErrNone);
			break;
			}

		TBMTicks now;
		::bmTimer.Stamp(&now);

		User::WaitForRequest(st2);
		BM_ERROR(st2.Int(), st2.Int() == KErrNone);
		time.Subscribe(st2);

		// get the time just before Set()
		TBMTicks propSetTime;
		TPtr8 ptr((TUint8*) &propSetTime, sizeof(propSetTime), sizeof(propSetTime));
		r = time.Get(KPropBenchmarkCategory, 0, ptr);
		BM_ERROR(r, r == KErrNone);	

		aResult->Cumulate(TBMTicksDelta(propSetTime, now));
		}

	prop.Cancel();
	User::WaitForRequest(st1);
	BM_ERROR(st1.Int(), st1.Int() == KErrCancel);
	time.Cancel();
	User::WaitForRequest(st2);
	BM_ERROR(st2.Int(), st2.Int() == KErrCancel);

	prop.Close();
	time.Close();
	sync.Close();
	child->WaitChildExit();

	r = prop.Delete(KPropBenchmarkCategory, 1);
	BM_ERROR(r, r == KErrNone);	
	r = time.Delete(KPropBenchmarkCategory, 0);
	BM_ERROR(r, r == KErrNone);	
	}

TInt Property::NotificationLatencyChild(TAny* cookie)
	{
	NotificationLatencyArgs* sl = (NotificationLatencyArgs*) cookie;
	TInt prio = BMProgram::SetAbsPriority(RThread(), sl->iChildOrigPriority);

	for (TInt j = 0; j < RProperty::KMaxPropertySize; ++j)
		{
		iOutBuf[j] = (TUint8)(j + 1);
		}

	RProperty time;
	TInt r = time.Attach(KPropBenchmarkCategory, 0);
	BM_ERROR(r, r == KErrNone);	

	RProperty prop;
	r = prop.Attach(KPropBenchmarkCategory, 1);
	BM_ERROR(r, r == KErrNone);

	RSemaphore sync;
	r = sync.OpenGlobal(_L("sync"));
	BM_ERROR(r, r == KErrNone);	

	for (TBMUInt64 i = 0; i < sl->iIterationCount; ++i)
		{
		sync.Wait();
		TBMTicks propSetTime;
		::bmTimer.Stamp(&propSetTime);

		switch(sl->iSetGetType)
			{
		case EOneArg:
			if (sl->iType == RProperty::EInt)
				{
				TInt value = 0xdeadbeef;
				r = prop.Set(value);
				BM_ERROR(r, r == KErrNone);
				}
			else 
				{
				TPtrC8 ptr(iOutBuf.Ptr(), sl->iSize);
				r = prop.Set(ptr);
				BM_ERROR(r, r == KErrNone);
				}
			break;
		case EThreeArgsInt:
			{
			TInt value = 0xdeadbeef;
			r = prop.Set(KPropBenchmarkCategory, 1, value);
			BM_ERROR(r, r == KErrNone);
			}
			break;
		case EThreeArgsBuf8:
			{
			TPtrC8 ptr(iOutBuf.Ptr(), sl->iSize);
			r = prop.Set(KPropBenchmarkCategory, 1, ptr);
			BM_ERROR(r, r == KErrNone);
			}
			break;
		case EThreeArgsBuf16:
			{
			TPtrC16 ptr(iOutBuf16.Ptr(), sl->iSize);
			r = prop.Set(KPropBenchmarkCategory, 1, ptr);
			BM_ERROR(r, r == KErrNone);
			}
			break;
			}

		// publish the time just before Set()
		TPtr8 ptr((TUint8*) &propSetTime, sizeof(propSetTime), sizeof(propSetTime));
		r = time.Set(ptr);
		BM_ERROR(r, r == KErrNone);
		}

	prop.Close();
	time.Close();
	sync.Close();

	BMProgram::SetAbsPriority(RThread(), prio);
	return KErrNone;
	}

						
void Property::SetOverhead(TBMResult* aResult, TBMUInt64 aIter, struct Measurement* aM)
	{
	RProperty prop;	
	TInt r = prop.Define(KPropBenchmarkCategory, 1, aM->iType, KPassPolicy, KPassPolicy);
	BM_ERROR(r, r == KErrNone);	
	r = prop.Attach(KPropBenchmarkCategory, 1);
	BM_ERROR(r, r == KErrNone);	

	if (aM->iType == RProperty::EByteArray)
		{
		TPtrC8 ptr(iOutBuf.Ptr(), aM->iSize);
		r = prop.Set(ptr);
		BM_ERROR(r, r == KErrNone);
		}

	TBMTimeInterval ti;
	ti.Begin();
	for (TBMUInt64 i = 0; i < aIter; ++i)
		{
		switch(aM->iSetGetType)
			{
		case EOneArg:
			if (aM->iType == RProperty::EInt)
				{
				TInt value = 1;
				r = prop.Set(value);
				BM_ERROR(r, r == KErrNone);	
				}
			else 
				{
				TPtrC8 ptr(iOutBuf.Ptr(), aM->iSize);
				r = prop.Set(ptr);
				BM_ERROR(r, r == KErrNone);	
				}
			break;
		case EThreeArgsInt:
			{
			TInt value = 1;
			r = prop.Set(KPropBenchmarkCategory, 1, value);
			BM_ERROR(r, r == KErrNone);
			}
			break;
		case EThreeArgsBuf8:
			{
			TPtrC8 ptr(iOutBuf.Ptr(), aM->iSize);
			r = prop.Set(KPropBenchmarkCategory, 1, ptr);
			BM_ERROR(r, r == KErrNone);
			}
			break;
		case EThreeArgsBuf16:
			{
			TPtrC16 ptr(iOutBuf16.Ptr(), aM->iSize);
			r = prop.Set(KPropBenchmarkCategory, 1, ptr);
			BM_ERROR(r, r == KErrNone);
			}
			break;
			}
		}
	TBMTicks t = ti.End();

	prop.Close();
	r = prop.Delete(KPropBenchmarkCategory, 1);
	BM_ERROR(r, r == KErrNone);	

	aResult->Cumulate(t, aIter);
	}

void Property::GetOverhead(TBMResult* aResult, TBMUInt64 aIter, struct Measurement* aM)
	{
	RProperty prop;	
	TInt r = prop.Define(KPropBenchmarkCategory, 1, aM->iType, KPassPolicy, KPassPolicy);
	BM_ERROR(r, r == KErrNone);	
	r = prop.Attach(KPropBenchmarkCategory, 1);
	BM_ERROR(r, r == KErrNone);	

	if (aM->iType == RProperty::EByteArray)
		{
		TPtrC8 ptr(iOutBuf.Ptr(), aM->iSize);
		r = prop.Set(ptr);
		BM_ERROR(r, r == KErrNone);
		}

	TBMTimeInterval ti;
	ti.Begin();
	for (TBMUInt64 i = 0; i < aIter; ++i)
		{
		switch(aM->iSetGetType)
			{
		case EOneArg:
			if (aM->iType == RProperty::EInt)
				{
				TInt value = 1;
				r = prop.Get(value);
				BM_ERROR(r, r == KErrNone);	
				}
			else 
				{
				r = prop.Get(iInBuf);
				BM_ERROR(r, r == KErrNone);	
				}
			break;
		case EThreeArgsInt:
			{
			TInt value = 1;
			r = prop.Get(KPropBenchmarkCategory, 1, value);
			BM_ERROR(r, r == KErrNone);
			}
			break;
		case EThreeArgsBuf8:
			r = prop.Get(KPropBenchmarkCategory, 1, iInBuf);
			BM_ERROR(r, r == KErrNone);
			break;
		case EThreeArgsBuf16:
			r = prop.Get(KPropBenchmarkCategory, 1, iInBuf16);
			BM_ERROR(r, r == KErrNone);
			break;
			}
		}
	TBMTicks t = ti.End();

	prop.Close();
	r = prop.Delete(KPropBenchmarkCategory, 1);
	BM_ERROR(r, r == KErrNone);	

	aResult->Cumulate(t, aIter);
	}



TBMResult* Property::Run(TBMUInt64 aIter, TInt* aCount)
	{
	TInt count = sizeof(iResults)/sizeof(iResults[0]);

	for (TInt i = 0; i < count; ++i)
		{
		iResults[i].Reset(iMeasurements[i].iName);
		iMeasurements[i].iFunc(&iResults[i], aIter, &iMeasurements[i]);
		iResults[i].Update();
		}
	
	*aCount = count;
	return iResults;
	}

void AddProperty()
	{
	BMProgram* next = bmSuite;
	bmSuite=(BMProgram*)&property;
	bmSuite->Next()=next;
	}
