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
#include "d_wsdextension.h"

_LIT(KWsdExtName, "D_WSDEXTENSION.LDD");

/** Factory class */
class DWsdExtLddFactory : public DLogicalDevice
	{
public:
	DWsdExtLddFactory();
	virtual TInt Install();
	virtual void GetCaps(TDes8 &aDes) const;
	virtual TInt Create(DLogicalChannelBase*& aChannel);
	static TInt iData;
	};

/** Logical channel */
class DWsdExtension : public DLogicalChannelBase
	{
public:
	DWsdExtension();
	~DWsdExtension();
protected:
	virtual TInt DoCreate(TInt aUnit, const TDesC8* anInfo, const TVersion& aVer);
	virtual TInt Request(TInt aReqNo, TAny* a1, TAny* a2);
	virtual TInt RequestUserHandle(DThread* aThread, TOwnerType aType);
private:
	DThread* iClient;
	};

TInt DWsdExtLddFactory::iData = 0xEFEFEFEF;

/** Factory class */
DWsdExtLddFactory::DWsdExtLddFactory()
	{
    iParseMask=0;
    iUnitsMask=0;
	}

/** Entry point for this driver */
//DECLARE_EXTENSION_WITH_PRIORITY(LATE_EXTENSION_PRIORITY)
DECLARE_STANDARD_EXTENSION()
	{
	Kern::Printf("D_WSDEXTENSION.LDD Start");
	DWsdExtLddFactory* device = new DWsdExtLddFactory;
	if(!device)
		return KErrNoMemory;
	TInt r = Kern::InstallLogicalDevice(device);
	return r;
	}

DECLARE_EXTENSION_LDD()
	{
	Kern::Printf("D_WSDEXTENSION.LDD Start 2");
	return new DWsdExtLddFactory;
	}
/** Second stage constuctor */
TInt DWsdExtLddFactory::Install()
	{
	Kern::Printf("D_WSDEXTENSION.LDD Install");
    return(SetName(&KWsdExtName));
	}

/** Device capabilities */
void DWsdExtLddFactory::GetCaps(TDes8& aDes)const
	{
		aDes.FillZ();
	}

/** Create logical channel, only open of one channel is allowed at a time */
TInt DWsdExtLddFactory::Create(DLogicalChannelBase*& aChannel)
	{
    if (iOpenChannels != 0) //A Channel is already open
		return KErrInUse;
    aChannel = new DWsdExtension;
    if(!aChannel)
		return KErrNoMemory;
    return KErrNone;
	}

/** Create channel */
TInt DWsdExtension::DoCreate(TInt /*aUnit*/, const TDesC8* /*aInfo*/, const TVersion& /*aVer*/)
	{
	return KErrNone;
	}

/** Constructor of Logical channel */
DWsdExtension::DWsdExtension()
	{
	iClient = &Kern::CurrentThread();
	((DObject*)iClient)->Open();
	}

/** Destructor */
DWsdExtension::~DWsdExtension()
	{
	// Close our reference on the client thread
	Kern::SafeClose((DObject*&)iClient,NULL);
	}

/** Handle user side requests */
TInt DWsdExtension::Request(TInt aReqNo, TAny* a1, TAny*)
	{
	switch(aReqNo)
		{
		case (RLddWsdExtension::EGET_STATIC_DATA):
			{ 
				TInt i=0;
				i=DWsdExtLddFactory::iData;
				kumemput(a1, &DWsdExtLddFactory::iData, sizeof(TInt));
				return KErrNone;
			}
		}
	return KErrNotSupported;
	}

/** Restricting handle duplication */
TInt DWsdExtension::RequestUserHandle(DThread* aThread, TOwnerType aType)
	{
	if (aType!=EOwnerThread || aThread!=iClient)
		return KErrAccessDenied;
	return KErrNone;
	}


