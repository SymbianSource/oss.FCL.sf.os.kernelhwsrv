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
// @file controlendpointreader.cpp
// @internalComponent
// 
//

#include "controlendpointreader.h"
#include "testdebug.h"

namespace NUnitTesting_USBDI
	{	
	
CControlEndpointReader::CControlEndpointReader(RDevUsbcClient& aClientDriver,MRequestHandler& aRequestHandler)
:	CEndpointReader(aClientDriver,EEndpoint0),
	iDeviceToHost(EFalse),
	iDataPhase(EFalse),
	iRequestHandler(aRequestHandler)
	{
	}
	
	
CControlEndpointReader::~CControlEndpointReader()
	{
	LOG_FUNC
	}


void CControlEndpointReader::ReadRequestsL()
	{
	LOG_FUNC
	
	// Read a packet from endpoint 0 (this should incorporate a request)
	
	ReadPacketL(this);
	}
	

void CControlEndpointReader::EndpointReadError(TEndpointNumber aEndpointNumber,TInt aErrorCode)
	{
	LOG_FUNC
	
	RDebug::Printf("<Error %d> Asynchronous read on endpoint %d",aErrorCode,aEndpointNumber);
	}
	
	
void CControlEndpointReader::DataReceivedFromEndpointL(TEndpointNumber aEndpointNumber,const TDesC8& aData)
	{
	LOG_FUNC
	
	RDebug::Printf("ibRequestType = %d, ibRequest = %d, iwValue = %d, iwIndex = %d, iwLength = %d",ibRequestType, ibRequest, iwValue, iwIndex, iwLength);
	RDebug::Printf("iDeviceToHost = %d, iDataPhase = %d",iDeviceToHost,iDataPhase);
	if(iDeviceToHost && iDataPhase)
		{
		TInt err = iRequestHandler.ProcessRequestL(ibRequest,iwValue,iwIndex,iwLength,aData);
		RDebug::Printf("ProdessRequestL returned %d",err);
		
		if(err != KErrAbort)
			{
			iDeviceToHost = EFalse;
			iDataPhase = EFalse;
			
			// After processing the request keep reading for more requests
			// from the host
			ReadPacketL(this);
			}
		}
	else
		{
		// Understand the setup packet
		ibRequestType = aData[0];
		iDeviceToHost = (ibRequestType & 0x80) == 0x00;
			
		// The request
		ibRequest = aData[1];
				
		// The request parameter
		TUint8* p = reinterpret_cast<TUint8*>(&iwValue);
		*p = aData[2];
		*(p+1) = aData[3];
		
		// The index for the request
		p = reinterpret_cast<TUint8*>(&iwIndex);
		*p = aData[4];
		*(p+1) = aData[5];
		
		// The length of data transfer
		p = reinterpret_cast<TUint8*>(&iwLength);
		*p = aData[6];
		*(p+1) = aData[7];
		
		iDataPhase = (iwLength > 0);
		
		// Read all information about the request sent by the host
		// i.e. any DATA1 packets sent after the setup DATA0 packet
		RDebug::Printf("AFTER UPDATES");
		RDebug::Printf("ibRequestType = %d, ibRequest = %d, iwValue = %d, iwIndex = %d, iwLength = %d",ibRequestType, ibRequest, iwValue, iwIndex, iwLength);
		RDebug::Printf("iDeviceToHost = %d, iDataPhase = %d",iDeviceToHost,iDataPhase);
		if(iDeviceToHost && iDataPhase)
			{
			RDebug::Printf("Issuing another read of %d bytes",iwLength);
			ReadL(iwLength);
			}
		else
			{
			TInt err = iRequestHandler.ProcessRequestL(ibRequest,iwValue,iwIndex,iwLength,KNullDesC8);
			
			if(err != KErrAbort)
				{
				// After processing the request keep reading for more requests
				// from the host
				ReadPacketL(this);
				}
			}
		}
	}
	
	
	}
