// Copyright (c) 2008-2009 Nokia Corporation and/or its subsidiary(-ies).
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
// e32test\digitiser\t_traweventdigitiser.cpp
// Overview:
// Test the TRawEvent APIS and events associated with the Digitiser and also verify the BTRACEs (manually)
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

#include <e32test.h>
#include <e32svr.h>
#include <e32cmn.h>
#include <e32cmn_private.h>

LOCAL_D RTest test(_L("t_TRawEventDigitiser"));

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


GLDEF_C TInt E32Main()
//
//
    {

 	test.Title();
	test.Start(_L("Testing Digitiser Events"));
	
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
	test.Printf(_L("T_TRAWEVENTDIGITISER: TEST Successfully Completed\n"));
	test.End();
	test.Close();

    return KErrNone;

    }

