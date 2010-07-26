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
// e32test\system\d_khal.cpp
// LDD for testing class Kern HAL APIs
// Kern::AddHalEntry(), Kern::RemoveHalEntry(), Kern::FindHalEntry()
// 
//

#include <kernel/kernel.h>

#include "d_khal.h"

#define KMaxDeviceNumber 8 // same as KMaxHalEntries

TUint 	gDeviceNumber=1;			// Device Number
TUint	gRegisteredDeviceNumber;	// Holds the device number which we managed to register for
TBool	gEntryForDevice0Registered; // indicator whether the device0 got registered
TBool	gEntryForDeviceXRegistered;	// States HAL Entry Successfully registered or not
TBool	gFirstCall=ETrue;			// for add function, this tells if this is first call or not

class DKHalLDDTestFactory : public DLogicalDevice
//
// Test LDD factory
//
	{
public:
	DKHalLDDTestFactory();
	virtual TInt Install(); 								//overriding pure virtual
	virtual void GetCaps(TDes8& aDes) const;				//overriding pure virtual
	virtual TInt Create(DLogicalChannelBase*& aChannel); 	//overriding pure virtual
	};

class DKHalLDDTestChannel : public DLogicalChannelBase
//
// Test logical channel
//
	{
public:
	virtual ~DKHalLDDTestChannel();
protected:
	virtual TInt DoCreate(TInt aUnit, const TDesC8* anInfo, const TVersion& aVer);
	virtual TInt Request(TInt aReqNo, TAny* a1, TAny* a2);
	};


LOCAL_C TInt halFunction0(TAny* /*aPtr*/, TInt aFunction, TAny* /*a1*/, TAny* /*a2*/)
	{
	TInt r=KErrNotSupported;

	switch(aFunction)
		{
		case RLddKHalTest::ETestHalFunc:
			Kern::Printf("HAL function0 called successfully !");
			r=KErrNone; // just return KErrNone
			break;
		default:
			break;
		}
	return r;
	}


LOCAL_C TInt halFunctionX(TAny* /*aPtr*/, TInt aFunction, TAny* /*a1*/, TAny* /*a2*/)
	{
	TInt r=KErrNotSupported;

	switch(aFunction)
		{
		case RLddKHalTest::ETestHalFunc:
			Kern::Printf("HAL functionX called successfully !");
			r=KErrNone; // just return KErrNone
			break;
		default:
			break;
		}
	return r;
	}

DECLARE_STANDARD_LDD()
	{
	return new DKHalLDDTestFactory;
	}

//
// Constructor
//
DKHalLDDTestFactory::DKHalLDDTestFactory()
	{
	}

TInt DKHalLDDTestFactory::Create(DLogicalChannelBase*& aChannel)
	{
//
// Create new channel
//  
	aChannel=new DKHalLDDTestChannel;
	return aChannel?KErrNone:KErrNoMemory;
	}

TInt DKHalLDDTestFactory::Install()
//
// Install the LDD - overriding pure virtual
	{
	return SetName(&KLddName);
	}

void DKHalLDDTestFactory::GetCaps(TDes8& /*aDes*/) const
//
// Get capabilities - overriding pure virtual
//
	{
	}

TInt DKHalLDDTestChannel::DoCreate(TInt /*aUnit*/, const TDesC8* /*aInfo*/, const TVersion& /*aVer*/)
//
// Create channel
//
	{
	return KErrNone;
	}

DKHalLDDTestChannel::~DKHalLDDTestChannel()
//
// Destructor
//
	{
	}

TInt DKHalLDDTestChannel::Request(TInt aReqNo, TAny* /*a1*/, TAny* /*a2*/)
	{
	TInt r=KErrNone;
	switch(aReqNo)
		{
		case RLddKHalTest::EAddHalEntryDevice0:
			{
			// try to register the halfunction for PlatformSpecific category, device 0
            NKern::ThreadEnterCS();
			r=Kern::AddHalEntry(EHalGroupPlatformSpecific2,halFunction0,this);
			NKern::ThreadLeaveCS();
			//this function gets called twice, second time must not changed these values
			if (gFirstCall)
				{
				if (r==KErrNone)
					{
					gEntryForDevice0Registered=ETrue;
					}
				else
					{
					gEntryForDevice0Registered=EFalse;
					}
				gFirstCall=EFalse;
				}

			break;
			}
		case RLddKHalTest::EAddHalEntryDeviceX:
			{
			// try to register the halfunction for PlatformSpecific category, device x
            NKern::ThreadEnterCS();
			do
				{
				r=Kern::AddHalEntry(EHalGroupPlatformSpecific2,halFunctionX,this,gDeviceNumber);
				}
            while((r==KErrInUse) && (++gDeviceNumber < KMaxDeviceNumber));
			NKern::ThreadLeaveCS();

			if((gDeviceNumber < KMaxDeviceNumber) && (r==KErrNone))
				{
				gEntryForDeviceXRegistered=ETrue;
				gRegisteredDeviceNumber=gDeviceNumber;
				}
			else
				{
				gEntryForDeviceXRegistered=EFalse;
				r=KErrInUse;
				}

			break;
			}
		case RLddKHalTest::EAddHalEntryForExistingFixed:
			{
			// try to add HAL entry for Kernel, should fail
            NKern::ThreadEnterCS();
			r=Kern::AddHalEntry(EHalGroupKernel,halFunction0,this);
			NKern::ThreadLeaveCS();
			break;
			}
		case RLddKHalTest::ERemoveHalEntryDevice0:
			{
			// try to remove the registered halfunction for device 0
			if(gEntryForDevice0Registered)
				r=Kern::RemoveHalEntry(EHalGroupPlatformSpecific2);
		
			break;
			}
		case RLddKHalTest::ERemoveHalEntryDeviceX:
			{
			// try to remove the registered halfunction for device x
			if(gEntryForDeviceXRegistered)
				r=Kern::RemoveHalEntry(EHalGroupPlatformSpecific2,gRegisteredDeviceNumber);
			break;
			}
		case RLddKHalTest::ERemoveHalEntryExistingFixed:
			{
			// try to remove EGroupHalKernel. This operation should return an error
			r=Kern::RemoveHalEntry(EHalGroupKernel);
			break;
			}
		case RLddKHalTest::EGetRegisteredDeviceNumber:
			{
			// return the device number which we managed to register
			if(gEntryForDeviceXRegistered)
				{
				r=gRegisteredDeviceNumber;
				}
			else
				{
				r=KErrNotFound;
				}
			break;
			}
		case RLddKHalTest::EFindHalEntryDevice0:
			{
			SHalEntry* pEntry=Kern::FindHalEntry(EHalGroupPlatformSpecific2);
			// returns valid pEntry if EAddHalEntryForDevice0 managed to register
			// an entry earlier
			if (pEntry && pEntry->iFunction!=NULL)
				{
				r=KErrNone;
				}
			else
				{
				r=KErrNotFound;
				}
			break;
			}
		case RLddKHalTest::EFindHalEntryDevice0Other:
			{
			SHalEntry* pEntry=Kern::FindHalEntry(EHalGroupKernel);
			//try to find an existing HAL group (kernel must exist)
			if (pEntry && pEntry->iFunction!=NULL)
				{
				r=KErrNone;
				}
			else
				{
				r=KErrNotFound;
				}
			break;
			}
		case RLddKHalTest::EFindHalEntryDeviceX:
			{
			SHalEntry* pEntry=Kern::FindHalEntry(EHalGroupPlatformSpecific2,gRegisteredDeviceNumber);
			// Should return valid pEntry if EAddHalEntryForDeviceX managed to register
			// one earlier
			if (pEntry && pEntry->iFunction!=NULL)
				{
				r=KErrNone;
				}
			else
				{
				r=KErrNotFound; 
				}
			break;
			}

		default:
			r=KErrNotSupported;
			break;
		} 

	return r;
	}
