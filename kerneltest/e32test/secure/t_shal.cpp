// Copyright (c) 2002-2010 Nokia Corporation and/or its subsidiary(-ies).
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
// e32test\secure\t_shal.cpp
// Overview:
// Test the security aspects of the HAL class.
// API Information:
// HAL
// Details:
// - For a variety of capability sets, get and set HAL hardware attributes
// and check that the results are as expected.
// - Set and restore the XYInputCalibration.
// - Add an event using UserSvr::AddEvent() and verify the resulting capability
// status.
// Platforms/Drives/Compatibility:
// All.
// Assumptions/Requirement/Pre-requisites:
// Failures and causes:
// Base Port information:
// 
//

#define __E32TEST_EXTENSION__
#define __INCLUDE_CAPABILITY_NAMES__

#include <e32test.h>
#include <e32hal.h>
#include <hal.h>
#include <e32svr.h>

LOCAL_D RTest test(_L("T_SHAL"));

TCapabilitySet Capabilities;

TInt PolicingVerified = 0;

LOCAL_C TBool Check(TInt aResult,TCapability aCap)
	{
	switch(aResult)
		{
	case KErrNotSupported:
		RDebug::Print(_L("  Not Supported"));
		return ETrue;
	case KErrNone:
		RDebug::Print(_L("  No Error"));
		break;
	case KErrPermissionDenied:
		RDebug::Print(_L("  Permission Denied"));
		break;
	default:
		RDebug::Print(_L("  Error %d"),aResult);
		break;
		}

	if(Capabilities.HasCapability(aCap))
		return aResult==KErrNone;
	else if(PlatSec::IsCapabilityEnforced(aCap))
		return aResult==KErrPermissionDenied;
	else
		return aResult==KErrNone;
	}

LOCAL_C void GetSetCheck(const char* aText,HALData::TAttribute aAttribute,TCapability aCap)
	{
	TBuf8<256> text=(const TUint8*)"HAL::Set(";
	text.Append((const TUint8*)aText,User::StringLength((const TUint8*)aText));
	text.Append(')'); 
	test.Next(text.Expand());
	TInt x = 0;
	HAL::Get(aAttribute,x);
	TInt r = HAL::Set(aAttribute,x);
	test(Check(r,aCap));
	}

#define SET_CHECK(a,c) 	GetSetCheck(#a,a,c);

LOCAL_C void TestUnusedFunctions()
    {
    TAny * mem = 0;
    TInt ret=0;
    ret=UserSvr::HalGet(HALData::EMemoryRAMFree, mem);
    test_Equal(KErrNotSupported, ret);
    ret=UserSvr::HalSet(HALData::EMemoryRAMFree, mem);
    test_Equal(KErrNotSupported, ret);
    ret=UserSvr::ResetMachine(EStartupCold);
    test_Equal(KErrNotSupported, ret);
    UserSvr::WsSwitchOnScreen();
    ret=User::Beep(220,1000000);
	test_Equal(KErrNotSupported, ret);
    }

LOCAL_C TInt DoTests()
	{
	TInt r;
//	TInt x = 0;

	//
	// ECapabilityReadDeviceData
	//

#if 0
	test.Start(_L("UserHal::MachineInfo()"));
    TMachineInfoV2Buf info;
	r = UserHal::MachineInfo(info);
	test(Check(r,ECapabilityReadDeviceData));
#endif

	//
	// ECapabilityWriteDeviceData
	//

	SET_CHECK(HAL::EKeyboardClickState,ECapabilityWriteDeviceData);
	SET_CHECK(HAL::EKeyboardClickVolume,ECapabilityWriteDeviceData);
	SET_CHECK(HAL::EPenClickState,ECapabilityWriteDeviceData);
	SET_CHECK(HAL::EPenClickVolume,ECapabilityWriteDeviceData);
	SET_CHECK(HAL::ELanguageIndex,ECapabilityWriteDeviceData);
	SET_CHECK(HAL::EKeyboardIndex,ECapabilityWriteDeviceData);
	SET_CHECK(HAL::ESystemDrive,ECapabilityWriteDeviceData);
	SET_CHECK(HAL::ECaseSwitchDisplayOn,ECapabilityWriteDeviceData);
	SET_CHECK(HAL::ECaseSwitchDisplayOff,ECapabilityWriteDeviceData);
	SET_CHECK(HAL::EDisplayContrast,ECapabilityWriteDeviceData);
	SET_CHECK(HAL::EDisplayBrightness,ECapabilityWriteDeviceData);
	SET_CHECK(HAL::EBacklightState,ECapabilityWriteDeviceData);
	SET_CHECK(HAL::EPenDisplayOn,ECapabilityWriteDeviceData);
	SET_CHECK(HAL::ELocaleLoaded,ECapabilityWriteDeviceData);
	SET_CHECK(HAL::ECustomResourceDrive,ECapabilityWriteDeviceData);

	test.Next(_L("UserHal::CalibrationPoints()"));
    TDigitizerCalibration xy;
	UserHal::CalibrationPoints(xy);
	r = UserHal::SetXYInputCalibration(xy);
	test(Check(r,ECapabilityWriteDeviceData));

	test.Next(_L("UserHal::RestoreXYInputCalibration()"));
	UserHal::SaveXYInputCalibration();
	r = UserHal::RestoreXYInputCalibration(ESaved);
	test(Check(r,ECapabilityWriteDeviceData));

	//
	// ECapabilityMultimediaDD
	//

	SET_CHECK(HAL::EMouseState,ECapabilityMultimediaDD);
	SET_CHECK(HAL::EMouseSpeed,ECapabilityMultimediaDD);
	SET_CHECK(HAL::EMouseAcceleration,ECapabilityMultimediaDD);
//	SET_CHECK(HAL::EDisplayMode,ECapabilityMultimediaDD);
//	SET_CHECK(HAL::EDisplayPaletteEntry,ECapabilityMultimediaDD);

	//
	// ECapabilityPowerMgmt
	//

	SET_CHECK(HAL::EKeyboardBacklightState,ECapabilityPowerMgmt);
	SET_CHECK(HAL::EAccessoryPower,ECapabilityPowerMgmt);
	SET_CHECK(HAL::EDisplayState,ECapabilityPowerMgmt);
	SET_CHECK(HAL::EKeyboardState,ECapabilityPowerMgmt);
	SET_CHECK(HAL::EPenState,ECapabilityPowerMgmt);
/*
	test.Next(_L("UserHal::SwitchOff()"));
	RTimer timer;
	TRequestStatus done;
	timer.CreateLocal();
	TTime wakeup;
	wakeup.HomeTime();
	wakeup+=TTimeIntervalSeconds(4);
	timer.At(done,wakeup);
	r = UserHal::SwitchOff(); // May not actually turn off due to imminent RTimer.At()
	test(Check(r,ECapabilityPowerMgmt));
	User::WaitForRequest(done);
*/
	//
	// ECapabilitySwEvent
	//

	test.Next(_L("UserSvr::AddEvent()"));
	TRawEvent event;
	r = UserSvr::AddEvent(event);
	test(Check(r,ECapabilitySwEvent));

	//

	test.End();

	return 0x55555555;
	}


enum TTestProcessFunctions
	{
	ETestProcessDoTests,
	};

#include "testprocess.h"



GLDEF_C TInt E32Main()
    {
	Capabilities = TSecurityInfo(RProcess()).iCaps;

	test.Title();

	TestUnusedFunctions();
	
	if(User::CommandLineLength())
		{
		TBuf<128> message;
		__ASSERT_COMPILE(ECapability_Limit<64);
		message.AppendFormat(_L("Tests with capabilities %08x%08x"),((TUint32*)&Capabilities)[1],((TUint32*)&Capabilities)[0]);
		test.Start(message);
		TInt result = DoTests();
		// Don't test.End() so we don't get lots of 'Success's in logs
		return(result);
		}

	test.Title();
	test.Start(_L("Start"));
	TInt c;
	for(c=0; c<1+ECapability_Limit; c++)
		{
		RTestProcess p;
		TRequestStatus s;
		TBuf<128> message;
		TCapabilitySet caps;
		caps.SetAllSupported();
		if(!caps.HasCapability((TCapability)c))
			continue;
		caps.RemoveCapability((TCapability)c);
		TBuf8<128> capNameBuf;
		capNameBuf.Copy((const TUint8*)CapabilityNames[c]);
		TPtr capName(capNameBuf.Expand());
		message.AppendFormat(_L("Tests with all capabilities except %S"),&capName);
		test.Next(message);
		p.Create(*(TUint32*)&caps,ETestProcessDoTests);
		p.Logon(s);
		p.Resume();
		User::WaitForRequest(s);
		test(p.ExitType()==EExitKill);
		TInt result=s.Int()^0x55555555;
		test(result==0);
		CLOSE_AND_WAIT(p);
		}

	// Show results requiring manual inspection
	_LIT(KSeperatorText,"----------------------------------------------------------------------------\n"); 
	test.Printf(_L("\n"));
	test.Printf(_L("RESULTS\n")); 
	test.Printf(KSeperatorText);
	TInt verified=1;
	for(c=0; c<ECapability_Limit; c++)
		if(!PlatSec::IsCapabilityEnforced((TCapability)c))
			verified = 0;

	if(!verified)
		test.Printf(_L("*  Did NOT verify security checking\n"));
	else
		test.Printf(_L("*  Verified security checking\n"));
	test.Printf(KSeperatorText);

	// Wait for a while, or for a key press
	test.Printf(_L("Waiting a short while for key press...\n"));
	TRequestStatus keyStat;
	test.Console()->Read(keyStat);
	RTimer timer;
	test(timer.CreateLocal()==KErrNone);
	TRequestStatus timerStat;
	timer.After(timerStat,20*1000000);
	User::WaitForRequest(timerStat,keyStat);
	if(keyStat!=KRequestPending)
		(void)test.Console()->KeyCode();
	timer.Cancel();
	test.Console()->ReadCancel();
	User::WaitForAnyRequest();

	test.End();
	return(0);
    }

