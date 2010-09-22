// Copyright (c) 2010 Nokia Corporation and/or its subsidiary(-ies).
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
// e32test\digitiser\t_kerneldigitisertest.cpp
// Overview:
// Test the TRawEvent APIS and events associated with the Digitiser and also verify the BTRACEs (manually)
// Tests kernel side functions
// API Information:
// TRawEvent
// Details:
// - Test the following 6  Events types 
// 1.	EPointerMove
// 2.	EPointer3DInRange,
// 3.	EPointer3DOutOfRange,
// 4.	EPointer3DTilt,
// 5.	EPointer3DRotation,
// 6.	EPointer3DTiltAndMove,
// Platforms/Drives/Compatibility:
// All.
// Assumptions/Requirement/Pre-requisites: 
// Failures and causes:
// 
//
#define __E32TEST_EXTENSION__

#include <e32test.h>
#include <e32svr.h>
#include <e32cmn.h>
#include <e32cmn_private.h>
#include "d_kerneldigitisertest.h"

LOCAL_D RTest test(_L("T_KernelDigitiserTest"));

RTestDigitiserLdd gLdd;

//
// class TestTRawDigitiserEvent user side constructor
//

TestTRawDigitiserEvent::TestTRawDigitiserEvent(TRawEvent::TType aType,TInt aX,TInt aY,TInt aZ,TInt aScanCode,TInt aPhi,TInt aTheta,TInt aAlpha,TUint8 aPointerNumber,TUint8 aTip)
:iType(aType),iX(aX),iY(aY),iZ(aZ),iScanCode(aScanCode),iPhi(aPhi),iTheta(aTheta),iAlpha(aAlpha),iPointerNumber(aPointerNumber),iTip(aTip)
	{}


//
// other functions
//
void LoadDeviceDriver()
	{
	test_KErrNone(User::LoadLogicalDevice(KLddName));
	test_KErrNone(gLdd.Open());
	}

void UnloadDeviceDriver()
	{
	gLdd.Close();
	test_KErrNone(User::FreeLogicalDevice(KLddName));
	}


GLDEF_C TInt E32Main()
//
//
    {
	__UHEAP_MARK;
 	test.Title();

	test.Start(_L("Testing kernel side digitiser events"));
	
	TestTRawDigitiserEvent kDigitiserEvent1(TRawEvent::EPointerMove, -890,-123, -823,455,2563,156,62,3,1);
	TestTRawDigitiserEvent kDigitiserEvent2(TRawEvent::EPointer3DInRange, 23,45,23,1,2,6,4,2,1);
	TestTRawDigitiserEvent kDigitiserEvent3(TRawEvent::EPointer3DOutOfRange, 23,45,23,1,2,6,4,2,0);
	TestTRawDigitiserEvent kDigitiserEvent4(TRawEvent::EPointer3DTilt, 23,45,23,1,2,6,4,2,1);
	TestTRawDigitiserEvent kDigitiserEvent5(TRawEvent::EPointer3DRotation, 23,45,23,1,2,6,4,2,1);
	TestTRawDigitiserEvent kDigitiserEvent6(TRawEvent::EPointer3DTiltAndMove, 23,45,23,1,2,6,4,2,0);

	LoadDeviceDriver();

	test.Printf(_L("kDigitiserEvent1"));
	test_KErrNone(gLdd.StartTest(kDigitiserEvent1));

	test.Printf(_L("kDigitiserEvent2"));
	test_KErrNone(gLdd.StartTest(kDigitiserEvent2));

	test.Printf(_L("kDigitiserEvent3"));
	test_KErrNone(gLdd.StartTest(kDigitiserEvent3));

	test.Printf(_L("kDigitiserEvent4"));
	test_KErrNone(gLdd.StartTest(kDigitiserEvent4));

	test.Printf(_L("kDigitiserEvent5"));
	test_KErrNone(gLdd.StartTest(kDigitiserEvent5));

	test.Printf(_L("kDigitiserEvent6"));
	test_KErrNone(gLdd.StartTest(kDigitiserEvent6));

	UnloadDeviceDriver();

	test.Printf(_L("T_KERNELDIGITISERTEST: Successfully Completed\n"));

	test.End();
	test.Close();

	__UHEAP_MARKEND;
    return KErrNone;
    }
