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
// e32test\power\d_powermisctest.cpp
// 
//

#include <kernel/kernel.h>
#include <kernel/kpower.h>
#include "d_powermisctest.h"
#include <smppower/idlehelper.h>
#include "smpidlehandler.h"

#ifdef __SMP__
NONSHARABLE_CLASS(DTest_SMPIdleHandler) : public DSMPIdleHandler
    {
public:
    DTest_SMPIdleHandler() : DSMPIdleHandler() {};
    TInt SelfTest()
    	{
    	// call unused functions to ensure it is covered in tests
    	ResetSyncPoints();
    	return DoEnterIdle(0,0,NULL);
    	}
    TBool GetLowPowerMode(TInt aIdleTime, TInt &aLowPowerMode) {return ETrue;}
    TBool EnterLowPowerMode(TInt aMode, TInt aCpuMask, TBool aLastCpu) {return ETrue;}	
    };
#endif

class DTestFactory : public DLogicalDevice
//
// Test LDD factory
//
	{
public:
	DTestFactory();
	virtual TInt Install(); 
	virtual void GetCaps(TDes8& aDes) const;
	virtual TInt Create(DLogicalChannelBase*& aChannel);
	};

class DTest1 : public DLogicalChannelBase
//
// Test logical channel
//
	{
public:
	virtual ~DTest1();
protected:
	virtual TInt DoCreate(TInt aUnit, const TDesC8* anInfo, const TVersion& aVer);
	virtual TInt Request(TInt aReqNo, TAny* a1, TAny* a2);

private:
	TInt DoTest();
#ifdef __SMP__
private:    
    DTest_SMPIdleHandler* iH;    
#endif    
	};



DECLARE_STANDARD_LDD()
	{
	return new DTestFactory;
	}

//
// Constructor
//
DTestFactory::DTestFactory()
	{

	}

TInt DTestFactory::Create(DLogicalChannelBase*& aChannel)
	{
//
// Create new channel
//
	aChannel=new DTest1;
	return aChannel?KErrNone:KErrNoMemory;
	}

TInt DTestFactory::Install()
//
// Install the LDD - overriding pure virtual
//
	{
	return SetName(&KLddName);
	}

void DTestFactory::GetCaps(TDes8& /*aDes*/) const
//
// Get capabilities - overriding pure virtual
//
	{
	}

TInt DTest1::DoCreate(TInt /*aUnit*/, const TDesC8* /*aInfo*/, const TVersion& /*aVer*/)
//
// Create channel
//
	{
#ifdef __SMP__
	iH = new DTest_SMPIdleHandler();
	TIdleSupport();	
#endif	
	return KErrNone;
	}

DTest1::~DTest1()
//
// Destructor
//
	{
#ifdef __SMP__		
	delete iH;		
#endif	
	}

TInt DTest1::Request(TInt aReqNo, TAny* /*a1*/, TAny* /*a2*/)
	{

	// 'Control' functions...
	switch(aReqNo)
		{
		// DoControl
		case RLddTest1::ECONTROL_TEST:
			DoTest();
			break;
		}

	return KErrNone;
	}
	
TInt DTest1::DoTest()
	{
#ifdef __SMP__	
	TInt r = KErrNone;
	
	// smp idle handler self tests
	Kern::Printf("smp idle handler self test");
	iH->SelfTest();
	
	// Test GetTimerCount using supplied timer
	Kern::Printf("test GetTimerCount using supplied timer");
	TUint32 TimerCount = 10;
	TIdleSupport::SetupIdleSupport(0, 0, &TimerCount);
	if(TIdleSupport::GetTimerCount() != 10)
		{
		return KErrGeneral;
		}	
	// Test GetTimerCount using default timer
	Kern::Printf("test GetTimerCound using default timer");
	TIdleSupport::SetupIdleSupport(0, 0, 0);
	TimerCount = TIdleSupport::GetTimerCount();

	// Test MarkCoreRetired()
	Kern::Printf("test MarkCoreRetired");
	TUint32 engagedMask = *(TIdleSupport::EngagedCpusMaskAddr());
	TIdleSupport::MarkCoreRetired(0x1);
	if(*(TIdleSupport::EngagedCpusMaskAddr())!= (engagedMask&(~0x1)))
		{
		return KErrGeneral;
		}
	// Test MarkCoreEngaged()
	Kern::Printf("test MarkCoreEngaged");
	TIdleSupport::MarkCoreEngaged(0x1);
	if(*(TIdleSupport::EngagedCpusMaskAddr())!= (engagedMask|0x1))
		{
		return KErrGeneral;
		}
	return r;
#else
	Kern::Printf("DoTest no supported");
	return KErrNotSupported;	
#endif	

	}
