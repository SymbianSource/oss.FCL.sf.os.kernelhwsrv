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
// e32test\nkern\d_implicit.cpp
// LDD for testing nanokernel implicit system lock
// 
//

#define __INCLUDE_NTHREADBASE_DEFINES__

#include "platform.h"
#include "nk_priv.h"
#include "d_implicit.h"

#ifndef __SMP__
#include "../misc/prbs.h"

const TInt KMajorVersionNumber=0;
const TInt KMinorVersionNumber=1;
const TInt KBuildVersionNumber=1;

const TInt KStackSize=1024;

inline NThreadBase::NThreadBase()
	{
	}

class DImpSysTestFactory : public DLogicalDevice
//
// Implicit system lock test LDD factory
//
	{
public:
	DImpSysTestFactory();
	virtual TInt Install();						//overriding pure virtual
	virtual void GetCaps(TDes8& aDes) const;	//overriding pure virtual
	virtual TInt Create(DLogicalChannelBase*& aChannel);	//overriding pure virtual
	};

class DImpSysTest : public DLogicalChannelBase
//
// Implicit system lock test LDD channel
//
	{
public:
	DImpSysTest();
protected:
	virtual TInt Request(TInt aReqNo, TAny* a1, TAny* a2);
	virtual TInt DoCreate(TInt aUnit, const TDesC8* anInfo, const TVersion& aVer);
public:
	TInt CreateThread(TInt);
	TInt Start(TInt aTest);
	TInt Stop(SStats& aStats);
	static void Thread1(TAny*);
	static void Thread2(TAny*);
	static void Thread3(TAny*);
public:
	TInt iFailCount;
	TInt iCount1;
	TInt iCount2;
	TInt iCount3;
	TInt iTestNum;
	NFastMutex iMutex;
	NThread iThread1;	// holds fast mutex, imp sys
	NThread iThread2;	// holds system lock, not imp sys
	NThread iThread3;	// random stuff
	TUint32 iStack1[KStackSize/sizeof(TUint32)];
	TUint32 iStack2[KStackSize/sizeof(TUint32)];
	TUint32 iStack3[KStackSize/sizeof(TUint32)];
	};

TInt DImpSysTest::CreateThread(TInt a)
	{
	SNThreadCreateInfo info;
	info.iStackSize=KStackSize;
	info.iPriority=(a==3)?16:12;
	info.iTimeslice=-1;
	info.iAttributes=(TUint8)((a==1)?KThreadAttImplicitSystemLock:0);
	info.iHandlers=NULL;
	info.iFastExecTable=NULL;
	info.iSlowExecTable=NULL;
	info.iParameterBlock=(const TUint32*)this;
	info.iParameterBlockSize=0;

	NThread* pT=NULL;
	switch (a)
		{
		case 1:
			pT=&iThread1;
			info.iFunction=&Thread1;
			info.iStackBase=iStack1;
			break;
		case 2:
			pT=&iThread2;
			info.iFunction=&Thread2;
			info.iStackBase=iStack2;
			break;
		case 3:
			pT=&iThread3;
			info.iFunction=&Thread3;
			info.iStackBase=iStack3;
			break;
		default:
			return KErrArgument;
		}

	return NKern::ThreadCreate(pT,info);
	}

void DImpSysTest::Thread1(TAny* aPtr)
	{
	DImpSysTest& d=*(DImpSysTest*)aPtr;
	TScheduler* pS=TScheduler::Ptr();
	NFastMutex& m=pS->iLock;
	TUint seed[2];
	seed[0]=1;
	seed[1]=0;
	FOREVER
		{
		NKern::FMWait(&d.iMutex);
		Kern::NanoWait(1300000);	// spin for 1.3ms
		TInt c = NKern::CurrentContext();
		__NK_ASSERT_ALWAYS(c == NKern::EThread);
		NKern::FMSignal(&d.iMutex);
		if (m.iHoldingThread)
			++d.iFailCount;
		TInt x=Random(seed)&3;
		if (x)
			NKern::Sleep(x);
		++d.iCount1;
		}
	}

void DImpSysTest::Thread2(TAny* aPtr)
	{
	DImpSysTest& d=*(DImpSysTest*)aPtr;
	TUint seed[2];
	seed[0]=2;
	seed[1]=0;
	FOREVER
		{
		TInt c = NKern::CurrentContext();
		__NK_ASSERT_ALWAYS(c == NKern::EThread);
		NKern::LockSystem();
		Kern::NanoWait(1100000);	// spin for 1.1ms
		NKern::UnlockSystem();
		TInt x=Random(seed)&3;
		if (x)
			NKern::Sleep(x);
		++d.iCount2;
		}
	}

void DImpSysTest::Thread3(TAny* aPtr)
	{
	DImpSysTest& d=*(DImpSysTest*)aPtr;
	TUint seed[2];
	seed[0]=3;
	seed[1]=0;
	if (d.iTestNum==RImpSysTest::ETestPriority)
		{
		FOREVER
			{
			TInt c = NKern::CurrentContext();
			__NK_ASSERT_ALWAYS(c == NKern::EThread);
			TInt x=Random(seed)&15;
			NKern::Sleep(x+1);
			x=Random(seed)&1;
			TInt p=10+Random(seed)&3;
			if (x)
				NKern::ThreadSetPriority(&d.iThread1,p);
			else
				NKern::ThreadSetPriority(&d.iThread2,p);
			++d.iCount3;
			}
		}
	else if (d.iTestNum==RImpSysTest::ETestRoundRobin)
		{
		FOREVER
			{
			TInt c = NKern::CurrentContext();
			__NK_ASSERT_ALWAYS(c == NKern::EThread);
			TInt x=Random(seed)&15;
			NKern::Sleep(x+1);
			NKern::RotateReadyList(12);
			++d.iCount3;
			}
		}
	else if (d.iTestNum==RImpSysTest::ETestDummy)
		{
		FOREVER
			{
			TInt c = NKern::CurrentContext();
			__NK_ASSERT_ALWAYS(c == NKern::EThread);
			TInt x=Random(seed)&15;
			NKern::Sleep(x+1);
			x=Random(seed)&255;
			TInt p=10+Random(seed)&3;
			if (x<85)
				{
				NKern::LockSystem();
				NKern::ThreadSetPriority(&d.iThread1,p);
				NKern::UnlockSystem();
				}
			else if (x<170)
				{
				NKern::LockSystem();
				NKern::ThreadSetPriority(&d.iThread2,p);
				NKern::UnlockSystem();
				}
			else
				{
				NKern::FMWait(&d.iMutex);
				NKern::FMSignal(&d.iMutex);
				}
			++d.iCount3;
			}
		}
	}

TInt DImpSysTest::Start(TInt aTest)
	{
	if (iTestNum>=0)
		return KErrInUse;
	iTestNum=aTest;
	iFailCount=0;
	iCount1=0;
	iCount2=0;
	iCount3=0;
	new (&iMutex) NFastMutex;
	TInt r=CreateThread(1);
	if (r==KErrNone)
		r=CreateThread(2);
	if (r==KErrNone)
		r=CreateThread(3);
	if (r==KErrNone)
		{
		NKern::ThreadResume(&iThread3);
		NKern::ThreadResume(&iThread1);
		NKern::ThreadResume(&iThread2);
		}
	if (r!=KErrNone)
		iTestNum=-1;
	return r;
	}


TInt DImpSysTest::Stop(SStats& a)
	{
	NKern::ThreadKill(&iThread1);
	NKern::ThreadKill(&iThread2);
	NKern::ThreadKill(&iThread3);
	NKern::ThreadSetPriority(&iThread1,16);
	NKern::ThreadSetPriority(&iThread2,16);
	NKern::ThreadSetPriority(&iThread3,16);
	while (iThread1.iNState!=NThread::EDead || iThread2.iNState!=NThread::EDead || iThread3.iNState!=NThread::EDead)
		NKern::Sleep(10);
	TInt size=3*(sizeof(NThread)+KStackSize)+sizeof(NFastMutex);
	memset(&iMutex,0xbb,size);
	a.iFailCount=iFailCount;
	a.iCount1=iCount1;
	a.iCount2=iCount2;
	a.iCount3=iCount3;
	iTestNum=-1;
	return KErrNone;
	}

DImpSysTestFactory::DImpSysTestFactory()
//
// Constructor
//
    {
    iVersion=TVersion(KMajorVersionNumber,KMinorVersionNumber,KBuildVersionNumber);
    //iParseMask=0;//No units, no info, no PDD
    //iUnitsMask=0;//Only one thing
    }

TInt DImpSysTestFactory::Create(DLogicalChannelBase*& aChannel)
//
// Create a new DImpSysTest on this logical device
//
    {
	aChannel=new DImpSysTest;
    return aChannel?KErrNone:KErrNoMemory;
    }

TInt DImpSysTestFactory::Install()
//
// Install the LDD - overriding pure virtual
//
    {
    return SetName(&KLddName);
    }

void DImpSysTestFactory::GetCaps(TDes8& aDes) const
//
// Get capabilities - overriding pure virtual
//
    {
    TCapsImpSysTestV01 b;
    b.iVersion=TVersion(KMajorVersionNumber,KMinorVersionNumber,KBuildVersionNumber);
    Kern::InfoCopy(aDes,(TUint8*)&b,sizeof(b));
    }

DImpSysTest::DImpSysTest()
//
// Constructor
//
	: iTestNum(-1)
    {
    }

TInt DImpSysTest::DoCreate(TInt /*aUnit*/, const TDesC8* /*anInfo*/, const TVersion& aVer)
//
// Create channel
//
    {

    if (!Kern::QueryVersionSupported(TVersion(KMajorVersionNumber,KMinorVersionNumber,KBuildVersionNumber),aVer))
    	return KErrNotSupported;
	return KErrNone;
	}

TInt DImpSysTest::Request(TInt aReqNo, TAny* a1, TAny*)
	{
	SStats s;
	TInt r=KErrNotSupported;
	switch (aReqNo)
		{
		case RImpSysTest::EControlStart:
			r=Start((TInt)a1);
			break;
		case RImpSysTest::EControlStop:
			{
			r=Stop(s);
			kumemput32(a1,&s,sizeof(s));
			}
			break;
		default:
			break;
		}
	return r;
	}

#endif


DECLARE_STANDARD_LDD()
	{
#ifdef __SMP__
	return 0;	// not used on SMP
#else
    return new DImpSysTestFactory;
#endif
    }

