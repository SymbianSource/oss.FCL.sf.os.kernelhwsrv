// Copyright (c) 2007-2010 Nokia Corporation and/or its subsidiary(-ies).
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
#include "OstTraceDefinitions.h"
#ifdef OST_TRACE_COMPILER_IN_USE
#include "controlendpointreaderTraces.h"
#endif

namespace NUnitTesting_USBDI
	{	
	
CControlEndpointReader::CControlEndpointReader(RDevUsbcClient& aClientDriver,MRequestHandler& aRequestHandler)
:	CEndpointReader(aClientDriver,EEndpoint0),
	iDeviceToHost(EFalse),
	iDataPhase(EFalse),
	iRequestHandler(aRequestHandler)
	{
	OstTraceFunctionEntryExt( CCONTROLENDPOINTREADER_CCONTROLENDPOINTREADER_ENTRY, this );
	OstTraceFunctionExit1( CCONTROLENDPOINTREADER_CCONTROLENDPOINTREADER_EXIT, this );
	}
	
	
CControlEndpointReader::~CControlEndpointReader()
	{
	OstTraceFunctionEntry1( CCONTROLENDPOINTREADER_CCONTROLENDPOINTREADER_ENTRY_DUP01, this );
	OstTraceFunctionExit1( CCONTROLENDPOINTREADER_CCONTROLENDPOINTREADER_EXIT_DUP01, this );
	}


void CControlEndpointReader::ReadRequestsL()
	{
	OstTraceFunctionEntry1( CCONTROLENDPOINTREADER_READREQUESTSL_ENTRY, this );
	
	// Read a packet from endpoint 0 (this should incorporate a request)
	
	ReadPacketL(this);
	OstTraceFunctionExit1( CCONTROLENDPOINTREADER_READREQUESTSL_EXIT, this );
	}
	

void CControlEndpointReader::EndpointReadError(TEndpointNumber aEndpointNumber,TInt aErrorCode)
	{
	OstTraceFunctionEntryExt( CCONTROLENDPOINTREADER_ENDPOINTREADERROR_ENTRY, this );
	
	OstTraceExt2(TRACE_NORMAL, CCONTROLENDPOINTREADER_ENDPOINTREADERROR, "<Error %d> Asynchronous read on endpoint %d",aErrorCode,aEndpointNumber);
	OstTraceFunctionExit1( CCONTROLENDPOINTREADER_ENDPOINTREADERROR_EXIT, this );
	}
	
	
void CControlEndpointReader::DataReceivedFromEndpointL(TEndpointNumber aEndpointNumber,const TDesC8& aData)
	{
	OstTraceFunctionEntryExt( CCONTROLENDPOINTREADER_DATARECEIVEDFROMENDPOINTL_ENTRY, this );
	
	OstTraceExt5(TRACE_NORMAL, CCONTROLENDPOINTREADER_DATARECEIVEDFROMENDPOINTL, "ibRequestType = %d, ibRequest = %d, iwValue = %d, iwIndex = %d, iwLength = %d",ibRequestType, ibRequest, iwValue, iwIndex, iwLength);
	OstTraceExt2(TRACE_NORMAL, CCONTROLENDPOINTREADER_DATARECEIVEDFROMENDPOINTL_DUP01, "iDeviceToHost = %d, iDataPhase = %d",iDeviceToHost,iDataPhase);
	if(iDeviceToHost && iDataPhase)
		{
		TInt err = iRequestHandler.ProcessRequestL(ibRequest,iwValue,iwIndex,iwLength,aData);
		OstTrace1(TRACE_NORMAL, CCONTROLENDPOINTREADER_DATARECEIVEDFROMENDPOINTL_DUP02, "ProdessRequestL returned %d",err);
		
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
		OstTrace0(TRACE_NORMAL, CCONTROLENDPOINTREADER_DATARECEIVEDFROMENDPOINTL_DUP03, "AFTER UPDATES");
		OstTraceExt5(TRACE_NORMAL, CCONTROLENDPOINTREADER_DATARECEIVEDFROMENDPOINTL_DUP04, "ibRequestType = %d, ibRequest = %d, iwValue = %d, iwIndex = %d, iwLength = %d",ibRequestType, ibRequest, iwValue, iwIndex, iwLength);
		OstTraceExt2(TRACE_NORMAL, CCONTROLENDPOINTREADER_DATARECEIVEDFROMENDPOINTL_DUP05, "iDeviceToHost = %d, iDataPhase = %d",iDeviceToHost,iDataPhase);
		if(iDeviceToHost && iDataPhase)
			{
			OstTrace1(TRACE_NORMAL, CCONTROLENDPOINTREADER_DATARECEIVEDFROMENDPOINTL_DUP06, "Issuing another read of %d bytes",iwLength);
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
	OstTraceFunctionExit1( CCONTROLENDPOINTREADER_DATARECEIVEDFROMENDPOINTL_EXIT, this );
	}
	
	
	}
