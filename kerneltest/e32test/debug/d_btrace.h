// Copyright (c) 2006-2009 Nokia Corporation and/or its subsidiary(-ies).
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
// e32tset\group\d_btrace.h
// 
//

#ifndef D_BTRACE_H
#define D_BTRACE_H

#include <e32cmn.h>
#include <e32ver.h>
#include <e32btrace.h>

#ifndef __KERNEL_MODE__
#include <e32std.h>
#endif


/**
Interface to the fast-trace memory buffer.
*/
class RBTraceTest : public RBusLogicalChannel
	{
public:
	enum TTestTraceFlags
		{
		EContextIsr			= 1<<8,
		EContextIDFC		= 1<<9,
		EContextIntsOff		= 1<<10,
		EBigTrace			= 1<<11,
		EUserTrace			= 1<<12,
		EFilter2Trace		= 1<<13
		};
#ifndef __KERNEL_MODE__
	inline TInt Open()
		{
		return DoCreate(Name(),TVersion(0,1,1),KNullUnit,NULL,NULL,EOwnerThread);
		}

	inline TInt Trace(TUint aType, TAny* aData, TInt aSize, TUint aDelay=0)
		{
		TPtrC8 des((TUint8*)aData,aSize+4);
		if(!aType)
			return DoControl(ETestTrace,(TAny*)aDelay,(TAny*)&des);
		else
			return DoControl(ETestSpecialTrace,(TAny*)aType,(TAny*)&des);
		}

	inline TUint TestBenchmark(TInt aSize, TUint aDuration)
		{
		return DoControl(ETestBenchmark,(TAny*)aSize,(TAny*)aDuration);
		}

	inline TUint TestBenchmark2(TInt aSize, TUint aDuration)
		{
		return DoControl(ETestBenchmark2,(TAny*)aSize,(TAny*)aDuration);
		}

	inline TUint TestBenchmarkCheckFilter(TInt aSize, TUint aDuration)
		{
		return DoControl(ETestBenchmarkCheckFilter,(TAny*)aSize,(TAny*)aDuration);
		}
	inline TInt UTrace(TUint aType, TAny* aData, TInt aSize)
		{
		TPtrC8 des((TUint8*)aData,aSize+4);
		return DoControl(ETestUTrace,(TAny*)aType,(TAny*)&des);
		}

#endif

	inline static const TDesC& Name();

private:
	enum TControl
		{
		ETestTrace,
		ETestSpecialTrace,
		ETestBenchmark,
		ETestBenchmark2,
		ETestBenchmarkCheckFilter,
		ETestUTrace,
		ETestBenchmarkUtrace,
		};
	friend class DBTraceTestChannel;
	friend class DBTraceTestFactory;
	};

inline const TDesC& RBTraceTest::Name()
	{
	_LIT(KBTraceName,"d_btrace");
	return KBTraceName;
	}


// Test UIDs are 0x10282675 To 0x102826D8 Inclusive
const TUint32 KBTraceFilterTestUid = 0x10282675;
const TUint KNumBTraceFilterTestUids = 100;
const TUint32 KBTraceFilterTestUid1 = KBTraceFilterTestUid+KNumBTraceFilterTestUids/2-1;
const TUint32 KBTraceFilterTestUid2 = KBTraceFilterTestUid1+2;


#endif
