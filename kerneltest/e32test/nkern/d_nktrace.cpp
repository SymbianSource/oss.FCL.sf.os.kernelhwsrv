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
// e32test\nkern\d_nktrace.cpp
// LDD for testing nanokernel debug trace bits
// 
//

#define __INCLUDE_NTHREADBASE_DEFINES__

#include "platform.h"
#include "nk_priv.h"
#include "d_nktrace.h"

const TInt KMajorVersionNumber=0;
const TInt KMinorVersionNumber=1;
const TInt KBuildVersionNumber=1;

class DNKTraceTestFactory : public DLogicalDevice
//
// NK Trace test LDD factory
//
	{
public:
	DNKTraceTestFactory();
	virtual TInt Install();						//overriding pure virtual
	virtual void GetCaps(TDes8& aDes) const;	//overriding pure virtual
	virtual TInt Create(DLogicalChannelBase*& aChannel);	//overriding pure virtual
	};

class DNKTraceTest : public DLogicalChannelBase
//
// NK Trace test LDD channel
//
	{
public:
	DNKTraceTest();
protected:
	virtual TInt Request(TInt aReqNo, TAny* a1, TAny* a2);
	virtual TInt DoCreate(TInt aUnit, const TDesC8* anInfo, const TVersion& aVer);
public:
	TInt iTestNum;
	};


DECLARE_STANDARD_LDD()
	{
	//=== load
    return new DNKTraceTestFactory;
    }

DNKTraceTestFactory::DNKTraceTestFactory()
//
// Constructor
//
    {
	//=== load
    iVersion=TVersion(KMajorVersionNumber,KMinorVersionNumber,KBuildVersionNumber);
    }

TInt DNKTraceTestFactory::Create(DLogicalChannelBase*& aChannel)
//
// Create a new DNKTraceTest on this logical device
//
    {
	//=== open
	aChannel=new DNKTraceTest;
    return aChannel?KErrNone:KErrNoMemory;
    }

TInt DNKTraceTestFactory::Install()
//
// Install the LDD - overriding pure virtual
//
    {
	//=== load
    return SetName(&KLddName);
    }


void DNKTraceTestFactory::GetCaps(TDes8& aDes) const
//
// Get capabilities - overriding pure virtual
//
    {
    TCapsNKTraceTestV01 b;
    b.iVersion=TVersion(KMajorVersionNumber,KMinorVersionNumber,KBuildVersionNumber);
    Kern::InfoCopy(aDes,(TUint8*)&b,sizeof(b));
    }

DNKTraceTest::DNKTraceTest()
//
// Constructor
//
	//=== open
	: iTestNum(-1)
    {
    }

TInt DNKTraceTest::DoCreate(TInt /*aUnit*/, const TDesC8* /*anInfo*/, const TVersion& aVer)
//
// Create channel
//
    {

	//=== open
    if (!Kern::QueryVersionSupported(TVersion(KMajorVersionNumber,KMinorVersionNumber,KBuildVersionNumber),aVer))
    	return KErrNotSupported;
	return KErrNone;
	}

TInt DNKTraceTest::Request(TInt aReqNo, TAny* a1, TAny*)
	{
	TInt r=KErrNotSupported;
	switch (aReqNo)
		{
		case RNKTraceTest::EControlKTrace:
			{
			r=KErrNone;
			Kern::Printf("D_NKTRACE : Test the __KTRACE_OPT macros");
			__KTRACE_OPT(KALWAYS,Kern::Printf("D_NKTRACE : KALWAYS"));
			__KTRACE_OPT(KPANIC,Kern::Printf("D_NKTRACE : KPANIC"));
			__KTRACE_OPT(KSCRATCH,Kern::Printf("D_NKTRACE : KSCRATCH"));
			__KTRACE_OPT(KPROC,Kern::Printf("D_NKTRACE : KPROC -- THIS SHOULD NOT PRINT!"));
			__KTRACE_OPT(35,Kern::Printf("D_NKTRACE : Debug bit 35 is set"));
			__KTRACE_OPT(36,Kern::Printf("D_NKTRACE : Debug bit 36 -- THIS SHOULD NOT PRINT!"));
			__KTRACE_OPT(68,Kern::Printf("D_NKTRACE : Debug bit 68 is set"));
			__KTRACE_OPT(101,Kern::Printf("D_NKTRACE : Debug bit 101 is set"));
			__KTRACE_OPT(136,Kern::Printf("D_NKTRACE : Debug bit 136 is set"));
			__KTRACE_OPT(172,Kern::Printf("D_NKTRACE : Debug bit 172 is set"));
			__KTRACE_OPT(192,Kern::Printf("D_NKTRACE : Debug bit 192 is set"));
			__KTRACE_OPT(230,Kern::Printf("D_NKTRACE : Debug bit 230 is set"));
			__KTRACE_ALL(0xC0000000,Kern::Printf("D_NKTRACE : KTRACE_ALL returned true"));
			__KTRACE_ALL(0xC0000001,Kern::Printf("D_NKTRACE : KTRACE_ALL -- THIS SHOULD NOT PRINT!"));
			}
			break;
		case RNKTraceTest::EControlKDebug:
			{
			TInt arg =(TInt)a1;

			Kern::Printf("");
			Kern::Printf("D_NKTRACE : KDebug tests (%d)", arg);
			r=KErrNone;
			TUint32 m = KDebugMask();
			Kern::Printf("D_NKTRACE : KDebugMask() = 0x%08x", m);
			TBool b = KDebugNum(30);
			Kern::Printf("D_NKTRACE : KDebugNum(30) = %d", b);
			b = KDebugNum(31);
			Kern::Printf("D_NKTRACE : KDebugNum(31) = %d", b);
			b = KDebugNum(3);
			Kern::Printf("D_NKTRACE : KDebugNum(3) = %d", b);
			b = KDebugNum(9);
			Kern::Printf("D_NKTRACE : KDebugNum(9) = %d", b);
			b = KDebugNum(10000);
			Kern::Printf("D_NKTRACE : KDebugNum(10000) = %d", b);
			b = KDebugNum(-1);
			Kern::Printf("D_NKTRACE : KDebugNum(-1) = %d", b);
			b = KDebugNum(-2);
			Kern::Printf("D_NKTRACE : KDebugNum(-2) = %d", b);
			b = KDebugNum(35);
			Kern::Printf("D_NKTRACE : KDebugNum(35) = %d", b);
			b = KDebugNum(36);
			Kern::Printf("D_NKTRACE : KDebugNum(36) = %d", b);
			b = KDebugNum(101);
			Kern::Printf("D_NKTRACE : KDebugNum(101) = %d", b);
			b = KDebugNum(192);
			Kern::Printf("D_NKTRACE : KDebugNum(192) = %d", b);
			b = KDebugNum(230);
			Kern::Printf("D_NKTRACE : KDebugNum(230) = %d", b);
			}
			break;
		default:
			break;
		}
	return r;
	}

