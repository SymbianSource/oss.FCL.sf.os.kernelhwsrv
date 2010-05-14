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
// e32test\secure\t_sdrivers.cpp
// Overview:
// Test the security aspects of device drivers.
// API Information:
// N/A
// Details:
// - For a variety of capability sets, test loading and opening various 
// devices and check that the results are as expected.
// Platforms/Drives/Compatibility:
// All.
// Assumptions/Requirement/Pre-requisites:
// Failures and causes:
// Base Port information:
// 
//

#define __INCLUDE_CAPABILITY_NAMES__

#define __E32TEST_EXTENSION__
#include <e32test.h>
#include <e32svr.h>
// SYM_BRANCH: Delete old sound driver
// #include <mdasound.h>
#include <d32comm.h>
#include <d32usbc.h>
#include <d32ethernet.h>

LOCAL_D RTest test(_L("T_SDRIVERS"));

TCapabilitySet Capabilities;

LOCAL_C TBool CheckLoaded(TInt aResult)
	{
	switch(aResult)
		{
	case KErrAlreadyExists:
		RDebug::Print(_L("  Already Exists"));
		return ETrue;

	case KErrNone:
		RDebug::Print(_L("  No Error"));
		return ETrue;

	case KErrNotFound:
		RDebug::Print(_L("  Not found"));
		return EFalse;

	default:
		test(EFalse);
		return EFalse;
		}
	}


class TDriverCheck;
typedef void (*TDriverCheckTestFunction)(TDriverCheck&);

class TDriverCheck
	{
public:
	TBool Check(TInt aResult);
	void ShowResult();
public:
	TDriverCheckTestFunction iTestFunction;
	const char* iDeviceName;
	TCapability iCapability;
	TBool iTested;
	TBool iPolicingVerified;
	};

TBool TDriverCheck::Check(TInt aResult)
	{
	switch(aResult)
		{
	case KErrNotSupported:
		RDebug::Print(_L("  Not Supported"));
		return ETrue;
	case KErrInUse:
		RDebug::Print(_L("  In Use"));
		return ETrue;
	case KErrAccessDenied:
		RDebug::Print(_L("  Access Denied (In Use?)"));
		return ETrue;
	case KErrNone:
		RDebug::Print(_L("  No Error"));
		break;
	case KErrPermissionDenied:
		RDebug::Print(_L("  Permission Denied"));
		break;
	default:
		RDebug::Print(_L("  Error %d"),aResult);
		return ETrue;
		}

	if(Capabilities.HasCapability(iCapability))
		{
		if(aResult==KErrNone)
			iTested = 1;
		return aResult==KErrNone;
		}
	else if(PlatSec::IsCapabilityEnforced(iCapability))
		{
		if(aResult==KErrPermissionDenied)
			iPolicingVerified = 1;
		return aResult==KErrPermissionDenied;
		}
	else
		{
		return aResult==KErrNone;
		}
	}

void TDriverCheck::ShowResult()
	{
	TBuf8<32> nameBuf((const TUint8*)iDeviceName);
	TPtr name(nameBuf.Expand());
	if(iTested)
		{
		if(iPolicingVerified)
			test.Printf(_L("*  %S - Verified security checking\n"),&name);
		else
			test.Printf(_L("*  %S - Did NOT verify security checking (Capabilties may be disabled)\n"),&name);
		}
	else
		test.Printf(_L("*  %S - Not tested (Driver may be missing or in use)\n"),&name);
	}

void TestELOCD(TDriverCheck& aCheck)
	{
	test.Next(_L("ELOCD"));

	test.Start(_L("Trying RLocalDrive with all local drives"));
	TInt i;
	TInt r;
	for(i=0; i<KMaxLocalDrives; i++)
		{
		RLocalDrive localDrive;
		TInt changedFlag = 0;
		r = localDrive.Connect(i,changedFlag);
		test(aCheck.Check(r));
		localDrive.Close();
		}
	test.End();
	}

#if defined (__WINS__)
#define COMM_PDD_NAME _L("ECDRV.PDD")
const TInt KMaxCommPdds=0;
#else
#define COMM_PDD_NAME _L("EUART")
const TInt KMaxCommPdds=10;
#endif

void TestECOMM(TDriverCheck& aCheck)
	{
	test.Next(_L("ECOMM"));

	test.Start(_L("Load PDDs"));
	TInt i;
	TInt r;
    TBuf<10> pddName=COMM_PDD_NAME;
	for (i=-1; i<KMaxCommPdds; ++i)
		{
		if (i==0)
			pddName.Append(TChar('0'));
		else if (i>0)
			pddName[pddName.Length()-1] = (TText)('0'+i);
		r = User::LoadPhysicalDevice(pddName);
		CheckLoaded(r);
		}
	test.Next(_L("Load LDD"));
	r = User::LoadLogicalDevice(_L("ECOMM.LDD"));
	if(!CheckLoaded(r))
		goto done;

	test.Next(_L("Open Channels"));
	for(i=0; i<10; i++)
		{
		RBusDevComm commDevice;
		r = commDevice.Open(i);
		test(aCheck.Check(r));
		commDevice.Close();
		}
done:
	test.End();
	}

void TestEUSBC(TDriverCheck& aCheck)
	{
	test.Next(_L("EUSBC"));

	test.Start(_L("Load LDD"));
	TInt r = User::LoadLogicalDevice(_L("EUSBC.LDD"));
	if(!CheckLoaded(r))
		goto done;
	test.Next(_L("Open Channel"));
	{
	RDevUsbcClient usbDevice;
	r = usbDevice.Open(0);
	test(aCheck.Check(r));
	usbDevice.Close();
	}
done:
	test.End();
	}

void TestENET(TDriverCheck& aCheck)
	{
	test.Next(_L("ENET"));

	test.Start(_L("Load PDD"));
	TInt r = User::LoadPhysicalDevice(_L("ETHERNET.PDD"));
	if(!CheckLoaded(r))
		goto done;
	test.Start(_L("Load LDD"));
	r = User::LoadLogicalDevice(_L("ENET.LDD"));
	if(!CheckLoaded(r))
		goto done;
	test.Next(_L("Open Channel"));
	{
	RBusDevEthernet ethernetDevice;
	r = ethernetDevice.Open(0);
	test(aCheck.Check(r));
	ethernetDevice.Close();
	}
done:
	test.End();
	}

// SYM_BRANCH: Delete old sound driver
#if 0
void TestESOUND(TDriverCheck& aCheck)
	{
	test.Next(_L("ESOUND"));

	test.Start(_L("Load PDD"));
	TInt r = User::LoadPhysicalDevice(_L("ESDRV.PDD"));
	if(!CheckLoaded(r))
		goto done;
	test.Next(_L("Load LDD"));
	r = User::LoadLogicalDevice(_L("ESOUND.LDD"));
	if(!CheckLoaded(r))
		goto done;
	test.Next(_L("Open Channel"));
	{
	RMdaDevSound soundDevice;
	r = soundDevice.Open();
	test(aCheck.Check(r));
	soundDevice.Close();
	}
done:
	test.End();
	}
#endif

TDriverCheck DriverList[] =
	{
		{TestELOCD,"ELOCD",ECapabilityTCB},
		{TestECOMM,"ECOMM",ECapabilityCommDD},
		{TestEUSBC,"EUSBC",ECapabilityCommDD},
		{TestENET,"ENET",ECapabilityCommDD},
// SYM_BRANCH: Delete old sound driver 
//		{TestESOUND,"ESOUND",ECapabilityMultimediaDD},
		{0}
	};

LOCAL_C TInt DoTests()
	{
	TInt result=0;
	test.Start(_L("Testing all LDDs..."));
	TInt i=0;
	while(DriverList[i].iTestFunction)
		{
		(*DriverList[i].iTestFunction)(DriverList[i]);
		result |= DriverList[i].iTested<<(i*2);
		result |= DriverList[i].iPolicingVerified<<(i*2+1);
		++i;
		}
	test.End();
	return result^0x55555555;
	}


enum TTestProcessFunctions
	{
	ETestProcessDoTests,
	};


#include "d_sldd.h"
#include "u32std.h"

RDevice TestDevice;

TInt TestGetCapsThread(TAny* aDes)
	{
	RThread::Rendezvous(KErrNone);
	TestDevice.GetCaps(*(TDes8*)aDes);
	return KErrNone;
	}

void TestGetCaps()
	{
	TUint memModelAttributes = UserSvr::HalFunction(EHalGroupKernel, EKernelHalMemModelInfo, NULL, NULL);
	if((memModelAttributes&EMemModelAttrKernProt)==false)
		return; // no kernel protection to test

	// open test device...
	test.Start(_L("Open test driver"));
	RLddTest ldd;
	_LIT(KTestDeviceName,"D_SLDD");
	TInt r = User::LoadLogicalDevice(KTestDeviceName);
	test(r == KErrNone || r == KErrAlreadyExists);
	r = TestDevice.Open(KTestDeviceName);
	test_KErrNone(r);
	r = ldd.OpenLocal();
	test_KErrNone(r);

	// get address of some kernel data...
	TUint32* kernelPtr;
	TUint32 kernelData;
	ldd.KernelTestData(kernelPtr,kernelData);

	// check device GetCaps works...
	test.Next(_L("Check GetCaps"));
	_LIT8(KDummyTestData,"Dummy Test Data");
	TBuf8<256> caps;
	caps.Copy(KDummyTestData);
	test(caps.Compare(KDummyTestData)==0);
	TestDevice.GetCaps(caps);
	test(caps.Compare(KDummyTestData)!=0);

	// get another thread to try and call device GetCaps to write to kernel data...
	test.Next(_L("Check GetCaps with bad descriptor"));
	TPtr8 badCaps((TUint8*)kernelPtr,sizeof(TUint32));
	RThread thread;
	r = thread.Create(_L("TestGetCapsThread"),TestGetCapsThread,KDefaultStackSize,0x2000,0x2000,(TAny*)&badCaps);
	test_KErrNone(r);
	TRequestStatus ls;
	thread.Logon(ls);
	TRequestStatus rs;
	thread.Rendezvous(rs);
	thread.Resume();
	User::WaitForRequest(rs);
	test_KErrNone(rs.Int());
	User::WaitForRequest(ls);
	test_Equal(EExitPanic,thread.ExitType());
	thread.Close();

	// check kernel data is unchanged...
	TUint32 kernelData2;
	ldd.KernelTestData(kernelPtr,kernelData2);
	test_Equal(kernelData,kernelData2);

	// get another thread to try and call device GetCaps with descriptor in kernel memory...
	test.Next(_L("Check GetCaps with bad descriptor 2"));
	r = thread.Create(_L("TestGetCapsThread2"),TestGetCapsThread,KDefaultStackSize,0x2000,0x2000,(TAny*)kernelPtr);
	test_KErrNone(r);
	thread.Logon(ls);
	thread.Rendezvous(rs);
	thread.Resume();
	User::WaitForRequest(rs);
	test_KErrNone(rs.Int());
	User::WaitForRequest(ls);
	test_Equal(EExitPanic,thread.ExitType());
	thread.Close();

	// check kernel data is unchanged...
	ldd.KernelTestData(kernelPtr,kernelData2);
	test_Equal(kernelData,kernelData2);

	// cleanup...
	ldd.Close();
	TestDevice.Close();

	test.End();
	}


#include "testprocess.h"


GLDEF_C TInt E32Main()
    {
	Capabilities = TSecurityInfo(RProcess()).iCaps;

	test.Title();

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

	test.Start(_L("Start"));

	test.Next(_L("Check driver GetCaps() vulnerability"));
	TestGetCaps();

	TInt i;
	TInt c;
	for(c=0; c<ECapability_Limit; c++)
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
		i=0;
		while(DriverList[i].iTestFunction)
			{
			if(result & (1<<(i*2)))
				DriverList[i].iTested = ETrue;
			if(result & (1<<(i*2+1)))
				DriverList[i].iPolicingVerified = ETrue;
			++i;
			}
		test((result>>(i*2))==0);
		CLOSE_AND_WAIT(p);
		}
	// Show results requiring manual inspection
	_LIT(KSeperatorText,"----------------------------------------------------------------------------\n"); 
	test.Printf(_L("\n"));
	test.Printf(_L("RESULTS\n")); 
	test.Printf(KSeperatorText);
	i=0;
	while(DriverList[i].iTestFunction)
		{
		DriverList[i].ShowResult();
		++i;
		}
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

