// Copyright (c) 2004-2009 Nokia Corporation and/or its subsidiary(-ies).
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
// f32test\testusbcldd\src\dtestusblogdev.cpp
// 
//

#include "dtestusblogdev.h"
#include <d32usbc.h>

// global Dfc Que
TDynamicDfcQue* gDfcQ;

//
// DLL export number 1: Create a new LDD factory
//
DECLARE_STANDARD_LDD()
    {
	return(new DTestUsbcLogDevice);
	}

DTestUsbcLogDevice::DTestUsbcLogDevice() :
	iEndpoints(KMaxEndpointsPerClient+1)
	{
	iParseMask = KDeviceAllowUnit;
	iUnitsMask = 0xffffffff;
	iVersion = TVersion(KTestUsbcMajorVersion, KTestUsbcMinorVersion, KTestUsbcBuildVersion);
	}

DTestUsbcLogDevice::~DTestUsbcLogDevice()
	{
	for (TInt i = 0; i < iEndpoints.Count(); i++)
		{
		delete iEndpoints[i];
		}
	iEndpoints.Reset();
	iEndpoints.Close();

	if (gDfcQ)
		gDfcQ->Destroy();
	}
	
const TInt KDSTestUsbThreadPriority = 27;
_LIT(KDTestUsbThread,"DTestUsbThread");

TInt DTestUsbcLogDevice::Install()
	{
	// Allocate a kernel thread to run the DFC 
	TInt r = Kern::DynamicDfcQCreate(gDfcQ, KDSTestUsbThreadPriority, KDTestUsbThread);

	if (r != KErrNone)
		return r; 	

	_LIT(KName, "usbc");
	return SetName(&KName);
	}
	
void DTestUsbcLogDevice::GetCaps(TDes8& aDes) const
	{
	TPckgBuf<TCapsDevUsbc> b;
	b().version=iVersion;
	Kern::InfoCopy(aDes, b);
	}
	
TInt DTestUsbcLogDevice::Create(DLogicalChannelBase*& aChannel)
	{
	if (iEndpoints.Count() == 0)
		{
		for (TInt i = 0; i < KMaxEndpointsPerClient+1; i++)
			{
			DTestUsbcEndpoint* ep = new DTestUsbcEndpoint();
			if (!ep)
				{
				return KErrNoMemory;
				}
			TInt err = ep->Create(DLddTestUsbcChannel::iEndpointData[i].iCaps);
			if (err != KErrNone)
				{
				return err;
				}
			iEndpoints.Append(ep);
			}
		}
	aChannel = new DLddTestUsbcChannel(iEndpoints);
	return aChannel ? KErrNone : KErrNoMemory;
	}

