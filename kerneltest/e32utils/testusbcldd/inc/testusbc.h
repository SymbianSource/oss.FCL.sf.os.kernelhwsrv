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
// f32test\testusbcldd\inc\testusbc.h
// Test USB Client API.
// 
// WARNING: This file contains some APIs which are internal and are subject
//          to change without notice. Such APIs should therefore not be used
//          outside the Kernel and Hardware Services package.
//

/**
 @file
 @internalTechnology
*/

#ifndef __TESTUSBC_H__
#define __TESTUSBC_H__

#include <e32ver.h>
#include <d32usbc.h>


	
class RDevTestUsbcClient : public RDevUsbcClient
	{
	public:
	
	enum TControl
		{
		ETestControlReqEndpointStatusNotify = 100,
		ETestControlClearEndpoint
		};
	
	enum TRequest
		{
		ETestRequestNotifyEndpointStatus = 100
		};
		
	inline TInt Open(TInt aUnit)
		{
		_LIT(KUsbDevName, "Usbc");
		return (DoCreate(KUsbDevName, TVersion(0, 1, 1), aUnit, NULL, NULL, EOwnerThread));
		}
		
	inline void HostRead(TRequestStatus &aStatus, TEndpointNumber aEndpoint, TDes8 &aDes, TInt aLen)
		{
		TInt ep = (aEndpoint < 0 || aEndpoint>KMaxEndpointsPerClient) ? KInvalidEndpointNumber : aEndpoint;
		TEndpointTransferInfo info = { &aDes, ETransferTypeReadData, aLen };
		DoRequest(ep, aStatus, &info, (void*)ETrue);
		}
		
	inline void HostWrite(TRequestStatus &aStatus, TEndpointNumber aEndpoint, const TDesC8& aDes,
						  TInt aLen, TBool aZlpRequired = EFalse)
		{
		TInt ep = (aEndpoint < 0 || aEndpoint > KMaxEndpointsPerClient) ? KInvalidEndpointNumber : aEndpoint;
		TEndpointTransferInfo info = { const_cast<TDesC8*>(&aDes), ETransferTypeWrite, aLen, aZlpRequired };
		DoRequest(ep, aStatus, &info, (void*)ETrue);
		}
	
	inline void HostEndpointStatusNotify(TRequestStatus &aStatus, TEndpointNumber aEndpoint)
		{
		TInt ep = (aEndpoint < 0 || aEndpoint > KMaxEndpointsPerClient) ? KInvalidEndpointNumber : aEndpoint;
		DoRequest(ETestRequestNotifyEndpointStatus, aStatus, (void*)ep, (void*)ETrue);
		}
	
	inline void HostClearEndpoint(TEndpointNumber aEndpoint)
		{
		TInt ep = (aEndpoint < 0 || aEndpoint > KMaxEndpointsPerClient) ? KInvalidEndpointNumber : aEndpoint;
		DoControl(ETestControlClearEndpoint, (void*)ep, (void*)NULL);
		}
	};
	
#endif //__TESTUSBC_H__
