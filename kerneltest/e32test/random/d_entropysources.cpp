// Copyright (c) 2009 Nokia Corporation and/or its subsidiary(-ies).
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
// e32test\entropysources\d_entropysources.cpp
// 
//
/**
 @file
 @internalComponent
 @test
*/
#include <dfcs.h>
#include <kernel/kernel.h>
#include <kernel/kern_priv.h>
#include "kern_test.h"
#include "d_entropysources.h"

//---------------------------------------------------------------------------------------------------------------------
//! @SYMTestCaseID				KBASE-entropysources-2703
//! @SYMTestType				UT
//! @SYMTestCaseDesc			Verifies that entropy is contributed to the Secure RNG
//! @SYMPREQ					PREQ211
//! @SYMTestPriority			High
//! @SYMTestActions				
//! 	1. 	TestReseed: tests that the interval between RNG reseeds is less than KMaxReseedTime, unless the platform is 
//!         known not to have a viable entropy source.
//! 		
//! 
//! @SYMTestExpectedResults
//! 	1.	Properties checked:
//! 		1) checks that there is a valid entropy source contrbuting entropy data.
//!         2) checks that the entropy collection framework is functioning correctly..
//! 	
//---------------------------------------------------------------------------------------------------------------------

class DEntropySourcesFactory : public DLogicalDevice
    {
public:
    DEntropySourcesFactory();
    virtual TInt Install();    
    virtual void GetCaps(TDes8 &aDes) const;
    virtual TInt Create(DLogicalChannelBase*& aChannel); 
    };

class DEntropySources : public DLogicalChannelBase
    {
public:
 	DEntropySources();
   ~DEntropySources();    
    void ReseedHook();
    
protected:    
	virtual TInt DoCreate(TInt aUnit, const TDesC8* anInfo, const TVersion& aVer);
	virtual TInt Request(TInt aReqNo, TAny* a1, TAny* a2);

private:
    DThread*        iClient;
    TClientRequest* iRequest;    
    };

// Function to be called from the kernel side code.
void ReseedHook(TAny* aPtr)
    {
	((DEntropySources*)aPtr)->ReseedHook();
    }

DECLARE_STANDARD_LDD()
    {
     return new DEntropySourcesFactory;
    }

//
// DEntropySourcesFactory
//

DEntropySourcesFactory::DEntropySourcesFactory()
    {
    // Set version number for this device
    iVersion = TVersion(0,1,1);

    // Indicate we don't support units or a PDD
    iParseMask = 0;
    }

TInt DEntropySourcesFactory::Install()
    {
    return(SetName(&KEntropySourcesName));
    }

void DEntropySourcesFactory::GetCaps(TDes8& aDes) const
    {
    // Create a capabilities object
    TCapsEntropySources caps;
    caps.iVersion = iVersion;

    // Write it back to user memory
    Kern::InfoCopy(aDes,(TUint8*)&caps,sizeof(caps));
    }

TInt DEntropySourcesFactory::Create(DLogicalChannelBase*& aChannel)
    {
    aChannel = new DEntropySources;
    if(!aChannel)
        return KErrNoMemory;
    return KErrNone;
    }

//
// DEntropySources
//

DEntropySources::DEntropySources()
	{
    iClient = &Kern::CurrentThread();
    iClient->Open();
	}

DEntropySources::~DEntropySources()
    {
	KernTest::Test(KernTest::ERNGReseedHook, NULL, NULL);
    Kern::SafeClose((DObject*&)iClient, NULL);
	Kern::DestroyClientRequest(iRequest);
    }

TInt DEntropySources::DoCreate(TInt /*aUnit*/, const TDesC8* /*aInfo*/, const TVersion& /*aVer*/)
	{
	return Kern::CreateClientRequest(iRequest);
	}

TInt DEntropySources::Request(TInt aReqNo, TAny* a1, TAny* a2)
    {
    (void)a2;
    TInt r = KErrNotSupported;
    switch(aReqNo)
        {
        case ~REntropySources::EReseedTest:
            r = iRequest->SetStatus((TRequestStatus*)a1);
			if (r!=KErrNone)
				return r;
            KernTest::Test(KernTest::ERNGReseedHook, (TAny*)&::ReseedHook, this);            
            break;
        }
    return r;
    }

void DEntropySources::ReseedHook()
    {
	KernTest::Test(KernTest::ERNGReseedHook, NULL, NULL);
    Kern::QueueRequestComplete(iClient, iRequest, KErrNone);
    }
