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
// e32test\digitiser\t_ldddigitisertest.cpp
// 
//

#include <e32test.h>
#include <e32hal.h>
#include "d_lddDigitisertest.h"
#include <hal.h>

_LIT(KLddFileName, "D_LDDDIGITISERTEST.LDD");

RLddDigitiserTest gLDD;
GLDEF_D RTest test(_L("Digitiser LDD tests"));

void DoTestOnUnregisteredDevice()
	{
	__UHEAP_MARK;
	test.Printf(_L("DoTestOnUnregisteredDevice(): should return KErrNotSupported coz NO DEVICE IS REGISTERED !!!"));
 	TInt r;
	TInt halApiVal;	
	TInt unregisteredDeviceNumber=99; // Its highly unlikely that this device number is registered 

    r=HAL::Get(unregisteredDeviceNumber,HALData::EPointer3D,halApiVal);
	test(r==KErrNotSupported);
	
	r=HAL::Get(unregisteredDeviceNumber,HALData::EPointer3DMaxProximity,halApiVal);
	test(r==KErrNotSupported);
	
    r=HAL::Get(unregisteredDeviceNumber,HALData::EPointer3DThetaSupported,halApiVal);
	test(r==KErrNotSupported);
	
    r=HAL::Get(unregisteredDeviceNumber,HALData::EPointer3DPhiSupported,halApiVal);
	test(r==KErrNotSupported);
	
    r=HAL::Get(unregisteredDeviceNumber,HALData::EPointer3DRotationSupported,halApiVal);
	test(r==KErrNotSupported);
	
    r=HAL::Get(unregisteredDeviceNumber,HALData::EPointer3DPressureSupported,halApiVal);
	test(r==KErrNotSupported);
	
    r=HAL::Get(unregisteredDeviceNumber,HALData::EPointer3DProximityStep,halApiVal);
	test(r==KErrNotSupported);
	
    r=HAL::Get(unregisteredDeviceNumber,HALData::EPointerMaxPointers,halApiVal);
	test(r==KErrNotSupported);
	
    r=HAL::Get(unregisteredDeviceNumber,HALData::EPointerNumberOfPointers,halApiVal);
	test(r==KErrNotSupported);
	
    r=HAL::Get(unregisteredDeviceNumber,HALData::EPointer3DMaxPressure,halApiVal);
	test(r==KErrNotSupported);
	
	r=HAL::Get(unregisteredDeviceNumber,HALData::EPointer3DPressureStep,halApiVal);
	test(r==KErrNotSupported);
    
    test.Printf(_L("Successully Finished Testing the DoTestOnUnregisteredDevice\n"));
	__UHEAP_MARKEND;
	}

void DoTestDerivedeAtributes()
	{
	__UHEAP_MARK;
	TInt r;
	TInt halApiVal;
	TInt testDriverVal;
	TInt testDeviceNumber;
    
	//new Values for HAL::Set
	TInt halPointer3DMaxProximity=45;
	TInt halPointerNumberOfPointers=5;
 
	// New attribute values for Driver APIs
	TInt newPointer3D=0;
	TInt newPointer3DMaxProximity=50;
	TInt newPointer3DThetaSupported=0;
	TInt newPointer3DPhiSupported=0;
	TInt newPointer3DRotationSupported=0;
	TInt newPointer3DPressureSupported=0;
	TInt newPointer3DProximityStep=6;
	TInt newPointerMaxPointers=7;
	TInt newPointerNumberOfPointers=2;
	TInt newPointer3DMaxPressure=501;
	TInt newPointer3DPressureStep=50;

/*
The following sequence should be followed

a)	Load the test Driver

b)	Open the channel

c)	Register Kernel Hal Handler

d)  Get the Registered Device Number

e)	Initialise HAL data

f)	Get/Set HAL Data  using HAL APIS (HAL::GET, HAL::SET)

g)	Get/Set the HAL data  throught the test driver APIS

h)	Compare the results and make sure that data recived are same and data set 
    using HAL library apis received with test driver APIS  and vice versa.

i)	Do it for all the new Digitiser Attributes (11 at  the moment)

j)	.....

k)	De-register the Kernel HAL handler

l)	Unload the Driver

m)	END...

*/
   	test.Printf(_L("Testing Derived Attributes using a test Driver for Digitiser\n"));
	r=User::LoadLogicalDevice(KLddFileName);
	test(r==KErrNone);
	r=gLDD.Open();
	test(r==KErrNone);

	r=gLDD.registerHalHandler();
    test(r==KErrNone || r==KErrNotSupported);

    testDeviceNumber=gLDD.getRegisteredDeviceNumber();
	test(testDeviceNumber != KErrNotFound);
	gLDD.initialiseHalData();

    r=HAL::Get(testDeviceNumber,HALData::EPointer3D,halApiVal);
	test(r==KErrNone);
	testDriverVal=gLDD.getPointer3D();
    test(halApiVal == testDriverVal);
	
    r=HAL::Get(testDeviceNumber,HALData::EPointer3DMaxProximity,halApiVal);
	test(r==KErrNone);
	testDriverVal=gLDD.getPointer3DMaxProximity();
    test(halApiVal == testDriverVal);

    r=HAL::Get(testDeviceNumber,HALData::EPointer3DThetaSupported,halApiVal);
    test(r==KErrNone);
    testDriverVal=gLDD.getPointer3DThetaSupported();
    test(halApiVal == testDriverVal);

    r=HAL::Get(testDeviceNumber,HALData::EPointer3DPhiSupported,halApiVal);
    test(r==KErrNone);
    testDriverVal=gLDD.getPointer3DPhiSupported();
    test(halApiVal == testDriverVal);

    r=HAL::Get(testDeviceNumber,HALData::EPointer3DRotationSupported,halApiVal);
    test(r==KErrNone);
    testDriverVal=gLDD.getPointer3DRotationSupported();
    test(halApiVal == testDriverVal);

    r=HAL::Get(testDeviceNumber,HALData::EPointer3DPressureSupported,halApiVal);
    test(r==KErrNone);
    testDriverVal=gLDD.getPointer3DPressureSupported();
    test(halApiVal == testDriverVal);

    r=HAL::Get(testDeviceNumber,HALData::EPointer3DProximityStep,halApiVal);
    test(r==KErrNone);
    testDriverVal=gLDD.getPointer3DProximityStep();
    test(halApiVal == testDriverVal);

    r=HAL::Get(testDeviceNumber,HALData::EPointerMaxPointers,halApiVal);
    test(r==KErrNone);
    testDriverVal=gLDD.getPointerMaxPointers();
    test(halApiVal == testDriverVal);

    r=HAL::Get(testDeviceNumber,HALData::EPointerNumberOfPointers,halApiVal);
    test(r==KErrNone);
    testDriverVal=gLDD.getPointerNumberOfPointers();
    test(halApiVal == testDriverVal);

    r=HAL::Get(testDeviceNumber,HALData::EPointer3DMaxPressure,halApiVal);
    test(r==KErrNone);
    testDriverVal=gLDD.getPointer3DMaxPressure();
    test(halApiVal == testDriverVal);

	r=HAL::Get(testDeviceNumber,HALData::EPointer3DPressureStep,halApiVal);
    test(r==KErrNone);
    testDriverVal=gLDD.getPointer3DPressureStep();
    test(halApiVal == testDriverVal);

	// There are just 2 settable HAL Values. Set them using HAL APIs and get the same using
	// Test Driver APIs.
	r=HAL::Set(testDeviceNumber,HALData::EPointer3DMaxProximity,halPointer3DMaxProximity);
    test(r==KErrNone);
    testDriverVal=gLDD.getPointer3DMaxProximity();
    test(halPointer3DMaxProximity == testDriverVal);
	r=HAL::Get(testDeviceNumber,HALData::EPointer3DMaxProximity,halApiVal);
	test(r==KErrNone);
	test(halApiVal == halPointer3DMaxProximity);

    r=HAL::Set(testDeviceNumber,HALData::EPointerNumberOfPointers,halPointerNumberOfPointers);
    test(r==KErrNone);
    testDriverVal=gLDD.getPointerNumberOfPointers();
    test(halPointerNumberOfPointers == testDriverVal);
	r=HAL::Get(testDeviceNumber,HALData::EPointerNumberOfPointers,halApiVal);
	test(r==KErrNone);
	test(halApiVal == halPointerNumberOfPointers);
   
	// Set the Attributes values using the Driver APis and make sure  that the same is
	// received with HAL::Get APis
    gLDD.setPointer3D(newPointer3D);
    r=HAL::Get(testDeviceNumber,HALData::EPointer3D,halApiVal);
	test(r==KErrNone);
	test(halApiVal == newPointer3D);
	
    gLDD.setPointer3DMaxProximity(newPointer3DMaxProximity);
	r=HAL::Get(testDeviceNumber,HALData::EPointer3DMaxProximity,halApiVal);
	test(r==KErrNone);
	test(halApiVal == newPointer3DMaxProximity);
	
    gLDD.setPointer3DThetaSupported(newPointer3DThetaSupported);
	r=HAL::Get(testDeviceNumber,HALData::EPointer3DThetaSupported,halApiVal);
	test(r==KErrNone);
	test(halApiVal == newPointer3DThetaSupported);

    gLDD.setPointer3DPhiSupported(newPointer3DPhiSupported);
	r=HAL::Get(testDeviceNumber,HALData::EPointer3DPhiSupported,halApiVal);
	test(r==KErrNone);
	test(halApiVal == newPointer3DPhiSupported);

    gLDD.setPointer3DRotationSupported(newPointer3DRotationSupported);
	r=HAL::Get(testDeviceNumber,HALData::EPointer3DRotationSupported,halApiVal);
	test(r==KErrNone);
	test(halApiVal == newPointer3DRotationSupported);

    gLDD.setPointer3DPressureSupported(newPointer3DPressureSupported);
	r=HAL::Get(testDeviceNumber,HALData::EPointer3DPressureSupported,halApiVal);
	test(r==KErrNone);
	test(halApiVal == newPointer3DPressureSupported);

    gLDD.setPointer3DProximityStep(newPointer3DProximityStep);
	r=HAL::Get(testDeviceNumber,HALData::EPointer3DProximityStep,halApiVal);
	test(r==KErrNone);
	test(halApiVal == newPointer3DProximityStep);

    gLDD.setPointerMaxPointers(newPointerMaxPointers);
	r=HAL::Get(testDeviceNumber,HALData::EPointerMaxPointers,halApiVal);
	test(r==KErrNone);
	test(halApiVal == newPointerMaxPointers);

    gLDD.setPointerNumberOfPointers(newPointerNumberOfPointers);
	r=HAL::Get(testDeviceNumber,HALData::EPointerNumberOfPointers,halApiVal);
	test(r==KErrNone);
	test(halApiVal == newPointerNumberOfPointers);

    gLDD.setPointer3DMaxPressure(newPointer3DMaxPressure);	
	r=HAL::Get(testDeviceNumber,HALData::EPointer3DMaxPressure,halApiVal);
	test(r==KErrNone);
	test(halApiVal == newPointer3DMaxPressure);

	gLDD.setPointer3DPressureStep(newPointer3DPressureStep);
	
	r=HAL::Get(testDeviceNumber,HALData::EPointer3DPressureStep,halApiVal);
	test(r==KErrNone);
	test(halApiVal == newPointer3DPressureStep);  	
	r=gLDD.removeHalHandler();
    test(r == KErrNone);

	gLDD.Close();

	r = User::FreeLogicalDevice(KLddFileName);;
	test(r==KErrNone);
	User::After(100000);

	// Finished Testing Derived attributes
   	test.Printf(_L("Successully Finished Testing the Derived attributes\n"));
   	__UHEAP_MARKEND;
	}

void DoTestNonDerivedAtributes()
	{
	__UHEAP_MARK;
	// The following 4 HAL attributes would be tested using HAL::GET,HAL:SET APIs
	// 1. EPointer3DEnterHighPressureThreshold
	// 2. EPointer3DExitHighPressureThreshold
	// 3. EPointer3DEnterCloseProximityThreshold
	// 4. EPointer3DExitCloseProximityThreshold
	//////////********TEST STEPS********////////////////////
	// Step1: Get  the existing/default  values using HAL:GET() and save them (for restoring it at Step4)
	// Step2: Set new valuee using HAL:SET()
	// Step3: Make sure that the new values are set  by camparing the new valuee with the values got by HAL:GET()
	// Step4: Restore the orginal values( saved at Step1)
   
	TInt r;
	TInt halGetVal;
	// Save the  Original Values
	TInt origPointer3DEnterHighPressureThreshold;
	TInt origPointer3DExitHighPressureThreshold;
	TInt origPointer3DEnterCloseProximityThreshold;
	TInt origPointer3DExitCloseProximityThreshold;

	// New values to be set
    TInt newPointer3DEnterHighPressureThreshold = 500;
	TInt newPointer3DExitHighPressureThreshold = 300;
	TInt newPointer3DEnterCloseProximityThreshold = 20;
	TInt newPointer3DExitCloseProximityThreshold = 35;

	test.Printf(_L("Testing Non-Derived attributes\n"));
    
	// Test EPointer3DEnterHighPressureThreshold
	r=HAL::Get(HALData::EPointer3DEnterHighPressureThreshold,origPointer3DEnterHighPressureThreshold);
	test(r==KErrNone);
	r=HAL::Set(HALData::EPointer3DEnterHighPressureThreshold,newPointer3DEnterHighPressureThreshold);
	test(r==KErrNone);
	r=HAL::Get(HALData::EPointer3DEnterHighPressureThreshold,halGetVal);
	test(r==KErrNone);
	test(halGetVal == newPointer3DEnterHighPressureThreshold); 
	r=HAL::Set(HALData::EPointer3DEnterHighPressureThreshold,origPointer3DEnterHighPressureThreshold);
	test(r==KErrNone);
	r=HAL::Get(HALData::EPointer3DEnterHighPressureThreshold,halGetVal);
	test(r==KErrNone);
	test(halGetVal == origPointer3DEnterHighPressureThreshold); 

	// Test EPointer3DExitHighPressureThreshold
	r=HAL::Get(HALData::EPointer3DExitHighPressureThreshold,origPointer3DExitHighPressureThreshold);
	test(r==KErrNone);
	r=HAL::Set(HALData::EPointer3DExitHighPressureThreshold,newPointer3DExitHighPressureThreshold);
	test(r==KErrNone);
	r=HAL::Get(HALData::EPointer3DExitHighPressureThreshold,halGetVal);
	test(r==KErrNone);
	test(halGetVal == newPointer3DExitHighPressureThreshold);
	r=HAL::Set(HALData::EPointer3DExitHighPressureThreshold,origPointer3DExitHighPressureThreshold);
	test(r==KErrNone);
	r=HAL::Get(HALData::EPointer3DExitHighPressureThreshold,halGetVal);
	test(r==KErrNone);
	test(halGetVal == origPointer3DExitHighPressureThreshold); 	
	
	// Test EPointer3DEnterCloseProximityThreshold
	r=HAL::Get(HALData::EPointer3DEnterCloseProximityThreshold,origPointer3DEnterCloseProximityThreshold);
	test(r==KErrNone);
	r=HAL::Set(HALData::EPointer3DEnterCloseProximityThreshold,newPointer3DEnterCloseProximityThreshold);
   	test(r==KErrNone);
   	r=HAL::Get(HALData::EPointer3DEnterCloseProximityThreshold,halGetVal);
	test(r==KErrNone);
	test(halGetVal == newPointer3DEnterCloseProximityThreshold); 
	r=HAL::Set(HALData::EPointer3DEnterCloseProximityThreshold,origPointer3DEnterCloseProximityThreshold);
	test(r==KErrNone);
	r=HAL::Get(HALData::EPointer3DEnterCloseProximityThreshold,halGetVal);
	test(r==KErrNone);
	test(halGetVal == origPointer3DEnterCloseProximityThreshold); 

	// Test EPointer3DExitCloseProximityThreshold
	r=HAL::Get(HALData::EPointer3DExitCloseProximityThreshold,origPointer3DExitCloseProximityThreshold);
	test(r==KErrNone);
	r=HAL::Set(HALData::EPointer3DExitCloseProximityThreshold,newPointer3DExitCloseProximityThreshold);
   	test(r==KErrNone);
   	r=HAL::Get(HALData::EPointer3DExitCloseProximityThreshold,halGetVal);
	test(r==KErrNone);
	test(halGetVal == newPointer3DExitCloseProximityThreshold); 
	r=HAL::Set(HALData::EPointer3DExitCloseProximityThreshold,origPointer3DExitCloseProximityThreshold);
	test(r==KErrNone);
	r=HAL::Get(HALData::EPointer3DExitCloseProximityThreshold,halGetVal);
	test(r==KErrNone);
	test(halGetVal == origPointer3DExitCloseProximityThreshold);
	
	// Finished Testing non-derived attributes
	test.Printf(_L("Successully Finished Testing the non-derived attributes\n"));
	__UHEAP_MARKEND;
	}




void  DoTestTEvntTypes()
	{
	TInt r;
	TRawEvent::TType firstEvent=TRawEvent::ENone;
	TRawEvent::TType lastEvent=TRawEvent::ERestartSystem;
    for(int i=firstEvent;i<=lastEvent;++i)
		{
		gLDD.setTEvntType(i);			
		r=gLDD.getTEvntType();
		test(r == i);
		}   	

	}


void  DoTestTEvntDNum()
	{
	TInt r;
	TInt aDvcNum1=2;
	TInt aDvcNum2=100;


	 
	gLDD.setTEvntDNum(aDvcNum1);
	r=gLDD.getTEvntDNum();
	test(r == aDvcNum1);

	gLDD.setTEvntDNum(aDvcNum2);
	r=gLDD.getTEvntDNum();
	test(r == aDvcNum2);
		
	}



void  DoTestTEvntScanCode()
	{

	TInt r;
    TInt aScanCode1=0;
	TInt aScanCode2=235;
	gLDD.setTEvntScanCode(TRawEvent::EPointer3DInRange, aScanCode1);
	r=gLDD.getTEvntScanCode();
	test(r == aScanCode1);


	gLDD.setTEvntScanCode(TRawEvent::EPointer3DRotation, aScanCode2);
	r=gLDD.getTEvntScanCode();
	test(r == aScanCode2);

	}
	

void  DoTestTEvnTilt()
	{
	TInt aPhi1 =0, aTheta1=3;
	TInt aPhi2 =20, aTheta2=45;
	TInt r;
	TUsrEventBuf  eventBuf;
	

	r=gLDD.setTEvntTilt(TRawEvent::EPointerMove,aPhi1,aTheta1);
	test(r==KErrNone);
	r=gLDD.getTEvntTilt(eventBuf);
	test(r==KErrNone);
	test((eventBuf().iPhi == aPhi1) && (eventBuf().iTheta == aTheta1));


	r=gLDD.setTEvntTilt(TRawEvent::EPointer3DInRange,aPhi2,aTheta2);
	test(r==KErrNone);

	r=gLDD.getTEvntTilt(eventBuf);
	test(r==KErrNone);
	test((eventBuf().iPhi == aPhi2) && (eventBuf().iTheta == aTheta2));	
	
	}





void  DoTestTEvntRotation()
	{
	TInt r;
    TInt  aRotation1 =12, aRotation2=323;

	r=gLDD.setTEvntRotation(TRawEvent::EPointerMove,aRotation1);
	test(r==KErrNone);
	r=gLDD.getTEvntRotation();
	test(r == aRotation1);


	r=gLDD.setTEvntRotation(TRawEvent::EPointerMove,aRotation2);
	test(r==KErrNone);
	r=gLDD.getTEvntRotation();
	test(r == aRotation2);
	
	}

void  DoTestTEvntPtr()
	{
	TInt r;
	TUint8 aPointerNumber1=123;
	TUint8 aPointerNumber2=2;
	TUint8 aPointerNumber3=3;
	TUint8 aPointerNumber4=125;

	TPoint3D tPoint3D1(-23 ,-85 ,-93);
	TPoint3D tPoint3D2(-23 ,0 ,100);
	TUsrEventBuf  eventBuf;


    r=gLDD.setTEvnt3DnPntr(TRawEvent::EPointerMove,tPoint3D1.iX,tPoint3D1.iY,tPoint3D1.iZ,aPointerNumber1);
	test(r==KErrNone);			 
	r=gLDD.getTEvntPntr();
	test(r == aPointerNumber1);	 
	r=gLDD.getTEventPos3D(eventBuf);
	test(r==KErrNone);
	test((eventBuf().iX == tPoint3D1.iX) && (eventBuf().iY == tPoint3D1.iY)  && (eventBuf().iZ == tPoint3D1.iZ) );

 	
	gLDD.setTEvntPntr(aPointerNumber2);
	r=gLDD.getTEvntPntr();
	test(r == aPointerNumber2);

	r=gLDD.setTEvnt3DnPntr(TRawEvent::EPointerMove,tPoint3D2.iX,tPoint3D2.iY,tPoint3D2.iZ,aPointerNumber3);
	test(r==KErrNone);			 
	r=gLDD.getTEvntPntr();
	test(r == aPointerNumber3);	 
	r=gLDD.getTEventPos3D(eventBuf);
	test(r==KErrNone);
	test((eventBuf().iX == tPoint3D2.iX) && (eventBuf().iY == tPoint3D2.iY)  && (eventBuf().iZ == tPoint3D2.iZ) );	  

	gLDD.setTEvntPntr(aPointerNumber4);
	r=gLDD.getTEvntPntr();
	test(r == aPointerNumber4);
	
	}



void  DoTestTEvntTicksNTips()
	{
	TInt r;
	TBool aTip=true;

	gLDD.setTEvntTip(aTip);
	r=gLDD.TEvntTicksIsTip();
	test(r == aTip);
	
	aTip=false;
    gLDD.setTEvntTip(aTip);
	r=gLDD.TEvntTicksIsTip();
	test(r == aTip);

	r=gLDD.getTEvntTicks();
	test(r > 0             );
	
	}


	



void  DoTestTEvntPOS2D()
	{
	TInt r;		
	TPoint tPoint1(0,0);
	TPoint tPoint2(323232,45454);
	TPoint tPoint3(-85 ,-93);
	TPoint tPoint4 (0 ,100);
	TUsrEventBuf  eventBuf;


    r=gLDD.setTEvntPos(TRawEvent::EPointerMove,tPoint1.iX,tPoint1.iY);
	test(r==KErrNone);
	r=gLDD.getTEvntPos(eventBuf);
	test(r==KErrNone);
	test((eventBuf().iX == tPoint1.iX) && (eventBuf().iY == tPoint1.iY) );


	r=gLDD.setTEvntPos(TRawEvent::EPointerMove,tPoint2.iX,tPoint2.iY);
	test(r==KErrNone);
	r=gLDD.getTEvntPos(eventBuf);
	test(r==KErrNone);
	test((eventBuf().iX == tPoint2.iX) && (eventBuf().iY == tPoint2.iY) );



	r=gLDD.setTEvntPos(TRawEvent::EPointerMove,tPoint3.iX,tPoint3.iY);
	test(r==KErrNone);
	r=gLDD.getTEvntPos(eventBuf);
	test(r==KErrNone);
	test((eventBuf().iX == tPoint3.iX) && (eventBuf().iY == tPoint3.iY) );


	r=gLDD.setTEvntPos(TRawEvent::EPointerMove,tPoint4.iX,tPoint4.iY);
	test(r==KErrNone);
	r=gLDD.getTEvntPos(eventBuf);
	test(r==KErrNone);
	test((eventBuf().iX == tPoint4.iX) && (eventBuf().iY == tPoint4.iY) );

	}
		



void  DoTestTEvntRepeat()
	{
	TInt r;

	TInt aType=TRawEvent::EPointerMove;
	 TInt aScanCode=20;
	 TInt aRepeats1=1;
	 TInt aRepeats2=13430;
	 TInt aRepeats3=0;

	 r=gLDD.setTEvntRepeat(aType,aScanCode,aRepeats1);
	 test(r==KErrNone);
	 r=gLDD.getTEvntRepeat();
	 test( r== aRepeats1);

	 r=gLDD.setTEvntRepeat(aType,aScanCode,aRepeats2);
	 test(r==KErrNone);
	 r=gLDD.getTEvntRepeat();
	 test( r== aRepeats2);


	 r=gLDD.setTEvntRepeat(aType,aScanCode,aRepeats3);
	 test(r==KErrNone);
	 r=gLDD.getTEvntRepeat();
	 test( r== aRepeats3);
	
	}
		



void  DoTestTEvntPos3D()
	{
	
	TPoint3D tPoint3D1(0,0,0);
	TPoint3D tPoint3D2(23232,323232,45454);
	TPoint3D tPoint3D3(-23 ,-85 ,-93);
	TPoint3D tPoint3D4(-23 ,0 ,100);
	TUsrEventBuf  eventBuf;
	TInt r;

     r=gLDD.setTEvntPos3D(TRawEvent::EPointerMove,tPoint3D1.iX,tPoint3D1.iY,tPoint3D1.iZ);
	test(r==KErrNone);
	r=gLDD.getTEventPos3D(eventBuf);
	test(r==KErrNone);
	test((eventBuf().iX == tPoint3D1.iX) && (eventBuf().iY == tPoint3D1.iY)  && (eventBuf().iZ == tPoint3D1.iZ) );


	r=gLDD.setTEvntPos3D(TRawEvent::EPointer3DInRange,tPoint3D2.iX,tPoint3D2.iY,tPoint3D2.iZ);
	test(r==KErrNone);
	r=gLDD.getTEventPos3D(eventBuf);
	test(r==KErrNone);
	test((eventBuf().iX == tPoint3D2.iX) && (eventBuf().iY == tPoint3D2.iY)  && (eventBuf().iZ == tPoint3D2.iZ) );


	r=gLDD.setTEvntPos3D(TRawEvent::EPointer3DRotation,tPoint3D3.iX,tPoint3D3.iY,tPoint3D3.iZ);
	test(r==KErrNone);
	r=gLDD.getTEventPos3D(eventBuf);
	test(r==KErrNone);
	test((eventBuf().iX == tPoint3D3.iX) && (eventBuf().iY == tPoint3D3.iY)  && (eventBuf().iZ == tPoint3D3.iZ) );


	r=gLDD.setTEvntPos3D(TRawEvent::EPointer3DTiltAndMove,tPoint3D4.iX,tPoint3D4.iY,tPoint3D4.iZ);
	test(r==KErrNone);
	r=gLDD.getTEventPos3D(eventBuf);
	test(r==KErrNone);
	test((eventBuf().iX == tPoint3D4.iX) && (eventBuf().iY == tPoint3D4.iY)  && (eventBuf().iZ == tPoint3D4.iZ) );
	
	}





void  DoTestTEvntPos3DAll()
	{
	TInt r;
	TPoint3D tPoint3D1(0,322,45454);
	TPoint3D tPoint3D2(-23 ,0 ,-93);

    TInt aPhi1 =0, aTheta1=3, alpha1=10;
	TInt aPhi2 =20, aTheta2=45,alpha2=126;

	TRawEvent::TType event1 =TRawEvent::EPointerMove;
	TRawEvent::TType event2= TRawEvent::EPointer3DRotation;

	TUsrEventBuf  eventBuf;
		
    r=gLDD.setTEvntAll(event1,tPoint3D1.iX,tPoint3D1.iY,tPoint3D1.iZ,aPhi1,aTheta1,alpha1);
	test(r==KErrNone);

	r=gLDD.getTEvntType();
    test(r == event1);

	r=gLDD.getTEventPos3D(eventBuf);
	test(r==KErrNone);
	test((eventBuf().iX == tPoint3D1.iX) && (eventBuf().iY == tPoint3D1.iY)  && (eventBuf().iZ == tPoint3D1.iZ) );
    

	r=gLDD.getTEvntRotation();
	test(r == alpha1);
    r=gLDD.getTEvntTilt(eventBuf);
	test(r==KErrNone);
	test((eventBuf().iPhi == aPhi1) && (eventBuf().iTheta == aTheta1));


  

	r=gLDD.setTEvntAll(event2,tPoint3D2.iX,tPoint3D2.iY,tPoint3D2.iZ,aPhi2,aTheta2,alpha2);
	test(r==KErrNone);

	r=gLDD.getTEvntType();
    test(r == event2);

	r=gLDD.getTEventPos3D(eventBuf);
	test(r==KErrNone);
	test((eventBuf().iX == tPoint3D2.iX) && (eventBuf().iY == tPoint3D2.iY)  && (eventBuf().iZ == tPoint3D2.iZ) );
    

	r=gLDD.getTEvntRotation();
	test(r == alpha2);
    r=gLDD.getTEvntTilt(eventBuf);
	test(r==KErrNone);
	test((eventBuf().iPhi == aPhi2) && (eventBuf().iTheta == aTheta2));

	}



void DoTestKernelTrawEvents()
	{
	__UHEAP_MARK;
	TInt r;
	test.Printf(_L("Testing KernelSide TrawEvents \n"));
	r=User::LoadLogicalDevice(KLddFileName);	
	test(r==KErrNone);
	r=gLDD.Open();
	test(r==KErrNone);

	//Test Kernel Side TRawevents

	DoTestTEvntTypes();
	DoTestTEvntDNum();	
	DoTestTEvntScanCode();
	DoTestTEvnTilt();
	DoTestTEvntRotation();
	DoTestTEvntPtr();
	DoTestTEvntTicksNTips();
	DoTestTEvntPOS2D();
	DoTestTEvntRepeat();
	DoTestTEvntPos3D();
	DoTestTEvntPos3DAll();
	
	gLDD.Close();
	r = User::FreeLogicalDevice(KLddFileName);;
	test(r==KErrNone);
	User::After(100000);

	// Finished Testing Derived attributes
   	test.Printf(_L("Successully Finished Testing KernelSide TrawEvents \n"));
   	__UHEAP_MARKEND;

	}


GLDEF_C TInt E32Main()
//
// Test Digitiser LDD
//
	{
	TInt machineUid=0;
	// This Testing is performed only on H4,Wins and WinEmulator, so need to get the machinUID
    const TInt r = HAL::Get(HAL::EMachineUid, machineUid);
	if(r==KErrNone) 
		{
		test.Start(_L("Test Start : LDDDigitiserTest"));
		// Non-Derived attributes are tested using a Test Driver for Digitiser which is supported only on H4,Wins and WinEmulator
		if((machineUid == HAL::EMachineUid_OmapH4) || (machineUid == HAL::EMachineUid_Win32Emulator))
			{
			DoTestNonDerivedAtributes();
			}		
		// Derived attributes are tested using a Test Driver for Digitiser which is supported only on H4
        if(machineUid == HAL::EMachineUid_OmapH4) 
			{
			 DoTestOnUnregisteredDevice(); 
			 DoTestDerivedeAtributes();	
			 DoTestKernelTrawEvents();			
			}
		}
	test.End();
	test.Close();
 	return(KErrNone);
    }

