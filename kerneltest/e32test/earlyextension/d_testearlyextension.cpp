// Copyright (c) 2007-2009 Nokia Corporation and/or its subsidiary(-ies).
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
// e32test\earlyextension\d_testearlyextension.cpp
// 
//

#include <kernel/kernel.h>
#include "d_testearlyextension.h"
#include "earlyextension.h"

_LIT(KTestEarlyExtName, "D_TESTEARLYEXTENSION.LDD");

/** Factory class */
class DTestEarlyExtLddFactory : public DLogicalDevice
	{
public:
	DTestEarlyExtLddFactory();
	virtual TInt Install();
	virtual void GetCaps(TDes8 &aDes) const;
	virtual TInt Create(DLogicalChannelBase*& aChannel);
	static TTimeK* iTimeArray; //Pointer to store the time stamps.
	};

/** Logical channel */
class DTestEarlyExtension : public DLogicalChannelBase
	{
public:
	DTestEarlyExtension();
	~DTestEarlyExtension();
protected:
	virtual TInt DoCreate(TInt aUnit, const TDesC8* anInfo, const TVersion& aVer);
	virtual TInt Request(TInt aReqNo, TAny* a1, TAny* a2);
	virtual TInt RequestUserHandle(DThread* aThread, TOwnerType aType);
private:
	DThread* iClient;
	};

TTimeK* DTestEarlyExtLddFactory::iTimeArray = NULL;

/** Factory class */
DTestEarlyExtLddFactory::DTestEarlyExtLddFactory()
	{
    iParseMask=0;
    iUnitsMask=0;
	}

/** Entry point for this driver */
DECLARE_EXTENSION_WITH_PRIORITY(LATE_EXTENSION_PRIORITY)
	{
	// - In extension init1 create and installing ldd factory
	// - Allocating space for 2 array of time stamps
	// - Calling early extension export to get the time stamp stored during its entry point
	// - Taking another time stamp and storing.
	DTestEarlyExtLddFactory* device = new DTestEarlyExtLddFactory;
	if(!device)
		return KErrNoMemory;
	device->iTimeArray = (TTimeK*)new(TTimeK[2]);
	if(!device->iTimeArray)
		{
		Kern::Printf("Memory not allocated");
		delete device;
		return KErrNoMemory;
		}
	TInt r = Kern::InstallLogicalDevice(device);
	if(r == KErrNone)
		{
		TTimeK temp;
		TestEarlyExtension::GetTimeStamp(temp);
		device->iTimeArray[0] = temp;
		device->iTimeArray[1] = Kern::SystemTime();
		}
	return r;
	}

DECLARE_EXTENSION_LDD()
	{
	return new DTestEarlyExtLddFactory;
	}
/** Second stage constuctor */
TInt DTestEarlyExtLddFactory::Install()
	{
    return(SetName(&KTestEarlyExtName));
	}

/** Device capabilities */
void DTestEarlyExtLddFactory::GetCaps(TDes8& aDes)const
	{
	}

/** Create logical channel, only open of one channel is allowed at a time */
TInt DTestEarlyExtLddFactory::Create(DLogicalChannelBase*& aChannel)
	{
    if (iOpenChannels != 0) //A Channel is already open
		return KErrInUse;
    aChannel = new DTestEarlyExtension;
    if(!aChannel)
		return KErrNoMemory;
    return KErrNone;
	}

/** Create channel */
TInt DTestEarlyExtension::DoCreate(TInt /*aUnit*/, const TDesC8* /*aInfo*/, const TVersion& /*aVer*/)
	{
	return KErrNone;
	}

/** Constructor of Logical channel */
DTestEarlyExtension::DTestEarlyExtension()
	{
	iClient = &Kern::CurrentThread();
	((DObject*)iClient)->Open();;
	}

/** Destructor */
DTestEarlyExtension::~DTestEarlyExtension()
	{
	// Close our reference on the client thread
	Kern::SafeClose((DObject*&)iClient,NULL);
	}

/** Handle user side requests */
TInt DTestEarlyExtension::Request(TInt aReqNo, TAny* a1, TAny* a2)
	{
	switch(aReqNo)
		{
		case (RLddEarlyExtensionTest::EGET_SYSTEM_TIME_STAMPS):
			{ //Get time stamps
			kumemput(a1, &(((DTestEarlyExtLddFactory*)iDevice)->iTimeArray[0]), sizeof(Int64));
			kumemput(a2, &(((DTestEarlyExtLddFactory*)iDevice)->iTimeArray[1]), sizeof(Int64));
			return KErrNone;
			}
		}
	return KErrNotSupported;
	}

/** Restricting handle duplication */
TInt DTestEarlyExtension::RequestUserHandle(DThread* aThread, TOwnerType aType)
	{
	if (aType!=EOwnerThread || aThread!=iClient)
		return KErrAccessDenied;
	return KErrNone;
	}


