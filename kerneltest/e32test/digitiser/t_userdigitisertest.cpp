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
// e32test\digitiser\t_userdigitisertest.cpp
// Overview:
// Test the TRawEvent APIS and events associated with the Digitiser and also verify the BTRACEs (manually)
// Test HAL digitiser orientation attribute
// API Information:
// UserSvr
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
#include <hal.h>

#ifndef E32TEST_NOCAPS
LOCAL_D RTest test(_L("T_UserDigitiserTest"));
#else
LOCAL_D RTest test(_L("T_UserDigitiserNoCaps"));
#endif

class TestTRawDigitiserEvent
	{
public:
	TestTRawDigitiserEvent(TRawEvent::TType aType,TInt aX,TInt aY,TInt aZ,TInt aScanCode,TInt aPhi,TInt aTheta,TInt aAlpha,TUint8 aPointerNumber,TUint8 iTip);
	void TestEvents();	
private:	
	TRawEvent::TType iType;
	TInt iX;
    TInt iY;
	TInt iZ;
	TInt iScanCode;
	TInt iPhi;
	TInt iTheta;
	TInt iAlpha;
	TUint8 iPointerNumber;
	TUint8 iTip;
	TRawEvent iDigitiser3DEvent;
	};


TestTRawDigitiserEvent::TestTRawDigitiserEvent(TRawEvent::TType aType,TInt aX,TInt aY,TInt aZ,TInt aScanCode,TInt aPhi,TInt aTheta,TInt aAlpha,TUint8 aPointerNumber,TUint8 aTip):iType(aType),iX(aX),iY(aY),iZ(aZ),iScanCode(aScanCode),iPhi(aPhi),iTheta(aTheta),iAlpha(aAlpha),iPointerNumber(aPointerNumber),iTip(aTip)
	{}


void TestTRawDigitiserEvent::TestEvents()
	{
	static TInt count = 0;
	count++;
	test.Printf(_L("TestTRawDigitiserEvent test case %2d\n"), count);
	
	test(iDigitiser3DEvent.Type()==0);
	iDigitiser3DEvent.Set(iType);
	test(iDigitiser3DEvent.Type()==iType);
	iDigitiser3DEvent.SetPointerNumber(iPointerNumber);
	test(iPointerNumber == iDigitiser3DEvent.PointerNumber());
	iDigitiser3DEvent.Set(iType,iScanCode);
	//Set the Type temporarily to get through the assertion 
	iDigitiser3DEvent.Set(TRawEvent::EKeyDown);
    test(iScanCode==iDigitiser3DEvent.ScanCode());
	iDigitiser3DEvent.Set(iType,iX,iY);
	//Set the Type temporarily to get through the assertion
	iDigitiser3DEvent.Set(TRawEvent::EPointerMove);
	test(TPoint(iX,iY)==iDigitiser3DEvent.Pos());
	iDigitiser3DEvent.Set(iType,iX,iY,iZ);
	//Set the Type temporarily to get through the assertion
	iDigitiser3DEvent.Set(TRawEvent::EPointerMove);
	test(TPoint3D(iX,iY,iZ)==iDigitiser3DEvent.Pos3D());
	iDigitiser3DEvent.SetTip(iTip);
	test(TBool(iTip) == iDigitiser3DEvent.IsTip());
	iDigitiser3DEvent.SetTilt(iType,iPhi,iTheta);
	//Set the Type temporarily to get through the assertion
	iDigitiser3DEvent.Set(TRawEvent::EPointer3DTilt);
	TAngle3D rawEventAnge3D=iDigitiser3DEvent.Tilt();
	test((rawEventAnge3D.iPhi==iPhi) && (rawEventAnge3D.iTheta==iTheta)) ;
	

	iDigitiser3DEvent.SetRotation(iType,iAlpha);
	//Set the Type temporarily to get through the assertion
	iDigitiser3DEvent.Set(TRawEvent::EPointer3DRotation);
	test(iAlpha == iDigitiser3DEvent.Rotation());
	iDigitiser3DEvent.Set(iType,iX+1,iY+1,iZ+1,iPhi+1,iTheta+1,iAlpha+1);
	//Set the Type temporarily to get through the assertion
	iDigitiser3DEvent.Set(TRawEvent::EPointer3DTiltAndMove);
	test(TPoint3D(iX+1,iY+1,iZ+1)==iDigitiser3DEvent.Pos3D());
    rawEventAnge3D=iDigitiser3DEvent.Tilt();
	test((rawEventAnge3D.iPhi==iPhi+1) &&(rawEventAnge3D.iTheta==iTheta+1));	
	test((iAlpha+1) == iDigitiser3DEvent.Rotation());   
	iDigitiser3DEvent.Set(iType,iX+2,iY+2,iZ+2,static_cast<TUint8>(iPointerNumber+1));
	//Set the Type temporarily to get through the assertion
	iDigitiser3DEvent.Set(TRawEvent::EPointer3DTiltAndMove);
  	test(TPoint3D(iX+2,iY+2,iZ+2)==iDigitiser3DEvent.Pos3D());
	test((iPointerNumber+1) == iDigitiser3DEvent.PointerNumber());

	UserSvr::AddEvent(iDigitiser3DEvent);
	}
	
	
struct HalAttribute_TestCase
	{
	HALData::TAttribute iAttr;
	TInt				iValueIn;
	TInt				iSetRC;		// Set to KMaxTInt to skip set test case
	TInt				iGetRC;		// Set to KMaxTInt to skip get test case

	};
	
static HalAttribute_TestCase gHalAttributeTests[] =
	{
#ifndef E32TEST_NOCAPS
	// Normal all pass tests
	{ HALData::EDigitiserOrientation, HALData::EDigitiserOrientation_000,		KErrNone, KErrNone},
	{ HALData::EDigitiserOrientation, HALData::EDigitiserOrientation_090,		KErrNone, KErrNone},	
	{ HALData::EDigitiserOrientation, HALData::EDigitiserOrientation_180,		KErrNone, KErrNone},
	{ HALData::EDigitiserOrientation, HALData::EDigitiserOrientation_270,		KErrNone, KErrNone},
	{ HALData::EDigitiserOrientation, HALData::EDigitiserOrientation_default,	KErrNone, KErrNone},
				
	// Negative tests
	{ HALData::EDigitiserOrientation, -1,		KErrArgument, KMaxTInt},
	{ HALData::EDigitiserOrientation, 100,		KErrArgument, KMaxTInt},
		
#else
	// Platsec tests for no capabilities executable.
	{ HALData::EDigitiserOrientation, HALData::EDigitiserOrientation_default,		KMaxTInt, KErrNone},			 // Get, No caps needed
	{ HALData::EDigitiserOrientation, HALData::EDigitiserOrientation_default,		KErrPermissionDenied, KMaxTInt}, // Set WDD cap needed
#endif
	};
	
static TInt gNumHalAttributeTests = sizeof(gHalAttributeTests)/sizeof(HalAttribute_TestCase);

void DoTestDigitiserHalAttributes()
	{
	__UHEAP_MARK;
#ifndef E32TEST_NOCAPS
	test.Start(_L("DoTestDigitiserHalAttributes tests"));
#else
	test.Start(_L("DoTestDigitiserHalAttributes NO CAPS tests"));
	
	// Skip No Caps testing for WDD caps when enforcement is not enabled on the
	// platform i.e. when a emulator epoc.ini is missing.
	if (!PlatSec::IsCapabilityEnforced(ECapabilityWriteDeviceData))
		{
		test.Printf(_L("Platform security enforcement off, skipping\n"));
		test.End();
		__UHEAP_MARKEND;
		return;
		}
#endif

	TInt i = 0;
	TInt origValue = -1;
	TInt r = HAL::Get(HALData::EDigitiserOrientation, origValue);
	if (r == KErrNotSupported)
		{
		test.Printf(_L("Platform doesn't support EDigitiserOrientation, skipping\n"));
		test.End();
		__UHEAP_MARKEND;
		return;
		}
	test_KErrNone(r);
	
	// Attribute supported on platform, proceed with test.
	TInt value = -1;
	for (i=0; i < gNumHalAttributeTests; i++)
		{
		test.Printf(_L("DoTestDigitiserHalAttributes - step/row %2d\n"), i+1);
		
		if (gHalAttributeTests[i].iSetRC != KMaxTInt) // Skip set test?
			{
			r = HAL::Set(gHalAttributeTests[i].iAttr,  gHalAttributeTests[i].iValueIn);
			test_Equal( gHalAttributeTests[i].iSetRC, r);	
			}
			
		if (gHalAttributeTests[i].iGetRC != KMaxTInt) // Skip get test?
			{
			r = HAL::Get(gHalAttributeTests[i].iAttr,  value);
			test_Equal(gHalAttributeTests[i].iGetRC, r);
			test_Equal(gHalAttributeTests[i].iValueIn, value);
			}
		}
		
#ifndef E32TEST_NOCAPS
	// Return system state back to before the test
	r = HAL::Set(HALData::EDigitiserOrientation, origValue);
	test_KErrNone(r);
#endif
		
	test.Printf(_L("DoTestDigitiserHalAttributes - complete\n"));
	test.End();
	__UHEAP_MARKEND;
	}

#ifndef E32TEST_NOCAPS
void DoTestRawDigitiserEvent()
	{
	__UHEAP_MARK;
	test.Start(_L("DoTestRawDigitiserEvent tests"));

    TestTRawDigitiserEvent digitiserEvent1(TRawEvent::EPointerMove, -890,-123, -823,455,2563,156,62,3,1);
	TestTRawDigitiserEvent digitiserEvent2(TRawEvent::EPointer3DInRange, 23,45,23,1,2,6,4,2,1);
	TestTRawDigitiserEvent digitiserEvent3(TRawEvent::EPointer3DOutOfRange, 23,45,23,1,2,6,4,2,0);
	TestTRawDigitiserEvent digitiserEvent4(TRawEvent::EPointer3DTilt, 23,45,23,1,2,6,4,2,1);
	TestTRawDigitiserEvent digitiserEvent5(TRawEvent::EPointer3DRotation, 23,45,23,1,2,6,4,2,1);
	TestTRawDigitiserEvent digitiserEvent6(TRawEvent::EPointer3DTiltAndMove, 23,45,23,1,2,6,4,2,0);

    digitiserEvent1.TestEvents();
	digitiserEvent2.TestEvents();
	digitiserEvent3.TestEvents();
	digitiserEvent4.TestEvents();
	digitiserEvent5.TestEvents();
	digitiserEvent6.TestEvents();    
	
	test.End();
	__UHEAP_MARKEND;
	}
#endif


GLDEF_C TInt E32Main()
//
//
    {
	__UHEAP_MARK;
	
 	test.Title();
	test.Start(_L("User-side Digitiser Testing Events/HAL"));
	
	DoTestDigitiserHalAttributes();
	
#ifndef E32TEST_NOCAPS
	DoTestRawDigitiserEvent();
#endif

	test.Printf(_L("\n"));
	test.End();
	test.Close();

	__UHEAP_MARKEND;
    return KErrNone;
    }

