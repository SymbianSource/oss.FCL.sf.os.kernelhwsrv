// Copyright (c) 1998-2009 Nokia Corporation and/or its subsidiary(-ies).
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
// e32test\secure\d_ldd.cpp
// LDD for testing kernel platsec APIs
// 
//

/* Hack to make sure we get the true value of iKernelConfigFlags */
#include "u32std.h"
#undef __PLATSEC_FORCED_FLAGS__
#define __PLATSEC_FORCED_FLAGS__ 0
/* End hack */

#include <kernel/kern_priv.h>

#include "d_sldd.h"

_LIT(KLddName,"D_SLDD");

const TInt KMajorVersionNumber=0;
const TInt KMinorVersionNumber=1;
const TInt KBuildVersionNumber=1;

TInt AFunction()
	{
	return KErrNone;
	}

TInt data=0x100;
TAny* dataptr=(TAny*)&AFunction;
TInt TheBss;


TUint32 KernelTestData[16] = { 0x32345678, 0x12345678 };

class DTest;

class DTestFactory : public DLogicalDevice
//
// Test LDD factory
//
	{
public:
	DTestFactory();
	virtual TInt Install(); 					//overriding pure virtual
	virtual void GetCaps(TDes8& aDes) const;	//overriding pure virtual
	virtual TInt Create(DLogicalChannelBase*& aChannel); 	//overriding pure virtual
	};

class DTest : public DLogicalChannelBase
//
// Test logical channel
//
	{
public:
	virtual ~DTest();
protected:
	virtual TInt DoCreate(TInt aUnit, const TDesC8* anInfo, const TVersion& aVer);
	virtual TInt Request(TInt aFunction, TAny* a1, TAny* a2);
	};

DECLARE_STANDARD_LDD()
	{
	return new DTestFactory;
	}

DTestFactory::DTestFactory()
//
// Constructor
//
	{
	iVersion=TVersion(KMajorVersionNumber,KMinorVersionNumber,KBuildVersionNumber);
	//iParseMask=0;//No units, no info, no PDD
	//iUnitsMask=0;//Only one thing
	}

TInt DTestFactory::Create(DLogicalChannelBase*& aChannel)
//
// Create a new DTest on this logical device
//
	{
	aChannel=new DTest;
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
	// deliberately do nothing here for testing purpose
	}

TInt DTest::DoCreate(TInt /*aUnit*/, const TDesC8* /*aInfo*/, const TVersion& aVer)
//
// Create channel
//
	{

	if (!Kern::QueryVersionSupported(TVersion(KMajorVersionNumber,KMinorVersionNumber,KBuildVersionNumber),aVer))
		return KErrNotSupported;
	return KErrNone;
	}

DTest::~DTest()
//
// Destructor
//
	{
	}

TInt DTest::Request(TInt aFunction, TAny* a1, TAny* a2)
	{
	TInt r = KErrNone;
	switch (aFunction)
		{
		case RLddTest::EControlTest1:
			r = RLddTest::ETest1Value;
			break;

		case RLddTest::EGetIds:
			{
			RLddTest::TIds ids;
			memclr(&ids, sizeof(ids));
			DThread* thread = &Kern::CurrentThread();
			ids.iThreadVID = Kern::ThreadVendorId(thread);
			ids.iThreadSID = Kern::ThreadSecureId(thread);
			DProcess* process = &Kern::CurrentProcess();
			ids.iProcessVID = Kern::ProcessVendorId(process);
			ids.iProcessSID = Kern::ProcessSecureId(process);
			kumemput(a1, &ids, sizeof(ids));
			break;
			}
		
		case RLddTest::EGetKernelConfigFlags:
			{
			TSuperPage& superPage = Kern::SuperPage();
			r = superPage.KernelConfigFlags();
			break;
			}

		case RLddTest::ESetKernelConfigFlags:
			{
			TSuperPage& superPage = Kern::SuperPage();
			superPage.SetKernelConfigFlags((TUint32)a1);
			break;
			}

		case RLddTest::ESetDisabledCapabilities0:
			{
			TSuperPage& superPage = Kern::SuperPage();
			memcpy(superPage.iDisabledCapabilities, &a1, sizeof(TUint32));
			break;
			}

		case RLddTest::EKernelTestData:
			{
			TUint32* ptr = KernelTestData;
			TUint32 data = *ptr;
			kumemput32(a1,&ptr,sizeof(ptr));
			kumemput32(a2,&data,sizeof(data));
			}
			break;

		case RLddTest::EGetSecureInfos:
			{
			TSecurityInfo infoThread(&Kern::CurrentThread());
			TSecurityInfo infoProcess(&Kern::CurrentProcess());
			kumemput32(a1,&infoThread,sizeof(infoThread));			
			kumemput32(a2,&infoProcess,sizeof(infoProcess));			
			}
			break;

		default:
			r = KErrNotSupported;
			break;
		}
	return r;
	}
