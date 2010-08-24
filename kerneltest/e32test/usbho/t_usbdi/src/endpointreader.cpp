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
// @file endpointreader.cpp
// @internalComponent
// 
//

#include "endpointreader.h"
#include "controltransferrequests.h"
#include "testdebug.h"
#include "OstTraceDefinitions.h"
#ifdef OST_TRACE_COMPILER_IN_USE
#include "endpointreaderTraces.h"
#endif

namespace NUnitTesting_USBDI
	{
const TUint8 KNumPatternRepeatsNeededForValidation = 4;

CEndpointReader::CEndpointReader(RDevUsbcClient& aClientDriver,TEndpointNumber aEndpoint)
:	CActive(EPriorityStandard),
	iClientDriver(aClientDriver),
	iEndpoint(aEndpoint),
	iDataPtr(NULL,0),
	iValidationPatternPtr(NULL,0)
	{
	OstTraceFunctionEntryExt( CENDPOINTREADER_CENDPOINTREADER_ENTRY, this );
	CActiveScheduler::Add(this);
	OstTraceFunctionExit1( CENDPOINTREADER_CENDPOINTREADER_EXIT, this );
	}
	
CEndpointReader::~CEndpointReader()
	{
	OstTraceFunctionEntry1( CENDPOINTREADER_CENDPOINTREADER_ENTRY_DUP01, this );
	Cancel();
	delete iDataBuffer;
	iDataBuffer = NULL;
	delete iValidationPatternBuffer;
	iValidationPatternBuffer = NULL;
	OstTraceFunctionExit1( CENDPOINTREADER_CENDPOINTREADER_EXIT_DUP01, this );
	}
			
TPtr8 CEndpointReader::Buffer()
	{
	OstTraceFunctionEntry1( CENDPOINTREADER_BUFFER_ENTRY, this );
	OstTraceFunctionExitExt( CENDPOINTREADER_BUFFER_EXIT, this, ( TUint )&( iDataPtr ) );
	return iDataPtr;
	}

TBool CEndpointReader::IsValid()
	{
	OstTraceFunctionEntry1( CENDPOINTREADER_ISVALID_ENTRY, this );
	OstTraceFunctionExitExt( CENDPOINTREADER_ISVALID_EXIT, this, iIsValid );
	return iIsValid;
	}

TUint CEndpointReader::NumBytesReadSoFar()
	{
	OstTraceFunctionEntry1( CENDPOINTREADER_NUMBYTESREADSOFAR_ENTRY, this );
	OstTraceFunctionExitExt( CENDPOINTREADER_NUMBYTESREADSOFAR_EXIT, this, iNumBytesReadSoFar );
	return iNumBytesReadSoFar;
	}

void CEndpointReader::ReadPacketL(MEndpointDataHandler* aHandler)
	{
	OstTraceFunctionEntryExt( CENDPOINTREADER_READPACKETL_ENTRY, this );
	OstTrace1(TRACE_NORMAL, CENDPOINTREADER_READPACKETL, "Endpoint %d", iEndpoint);
	
	iHandler = aHandler;

	// Allocate buffer for reading a data packet
	if(iDataBuffer)
		{
		delete iDataBuffer;
		iDataBuffer = NULL;
		}
	iDataBuffer = HBufC8::NewL(KFullSpeedPacketSize); 
	iDataPtr.Set(iDataBuffer->Des());

	// Read from the endpoint
	// Read from the endpoint
	iClientDriver.ReadPacket(iStatus,iEndpoint,iDataPtr,KFullSpeedPacketSize);
	SetActive();
	OstTraceFunctionExit1( CENDPOINTREADER_READPACKETL_EXIT, this );
	}


void CEndpointReader::ReadL(TInt aByteCount)
	{
	OstTraceFunctionEntryExt( CENDPOINTREADER_READL_ENTRY, this );
	OstTrace1(TRACE_NORMAL, CENDPOINTREADER_READL, "Endpoint %d", iEndpoint);
	
	// Allocate buffer for reading a data packet
	if(iDataBuffer)
		{
		delete iDataBuffer;
		iDataBuffer = NULL;
		}
	iDataBuffer = HBufC8::NewL(aByteCount);
	iDataPtr.Set(iDataBuffer->Des());

	// Read from the endpoint
	iClientDriver.Read(iStatus,iEndpoint,iDataPtr,aByteCount);
	SetActive();
	OstTraceFunctionExit1( CENDPOINTREADER_READL_EXIT, this );
	}

void CEndpointReader::ReadUntilShortL(TInt aByteCount)
	{
	OstTraceFunctionEntryExt( CENDPOINTREADER_READUNTILSHORTL_ENTRY, this );
	OstTrace1(TRACE_NORMAL, CENDPOINTREADER_READUNTILSHORTL, "Endpoint %d", iEndpoint);
	
	// Allocate buffer for reading a data packet
	if(iDataBuffer)
		{
		delete iDataBuffer;
		iDataBuffer = NULL;
		}
	iDataBuffer = HBufC8::NewL(aByteCount);
	iDataPtr.Set(iDataBuffer->Des());

	// Read from the endpoint
	iClientDriver.ReadUntilShort(iStatus,iEndpoint,iDataPtr,aByteCount);
	SetActive();
	OstTraceFunctionExit1( CENDPOINTREADER_READUNTILSHORTL_EXIT, this );
	}

void CEndpointReader::ReadAndHaltL(TInt aByteCount)
	{
	OstTraceFunctionEntryExt( CENDPOINTREADER_READANDHALTL_ENTRY, this );
	iCompletionAction = EHaltEndpoint;
	ReadL(aByteCount);
	OstTraceFunctionExit1( CENDPOINTREADER_READANDHALTL_EXIT, this );
	}

void CEndpointReader::RepeatedReadAndValidateL(const TDesC8& aDataPattern, TUint aNumBytesPerRead, TUint aTotalNumBytes)
	{
OstTraceFunctionEntryExt( CENDPOINTREADER_REPEATEDREADANDVALIDATEL_ENTRY, this );

	iCompletionAction = ERepeatedRead;
	iRepeatedReadTotalNumBytes = aTotalNumBytes;
	iRepeatedReadNumBytesPerRead = aNumBytesPerRead;
	iNumBytesReadSoFar = 0;
	iDataPatternLength = aDataPattern.Length();
	iIsValid = ETrue; //until proven guilty!
	OstTraceExt2(TRACE_NORMAL, CENDPOINTREADER_REPEATEDREADANDVALIDATEL, "Total Bytes To Read: %u, Bytes Per Individual Read: %u", iRepeatedReadTotalNumBytes, iRepeatedReadNumBytesPerRead);
	//Create buffer to contain two lots of the payload pattern
	//..so that we may grab cyclic chunks of said payload pattern
	delete iValidationPatternBuffer;
	iValidationPatternBuffer = NULL;
	iValidationPatternBuffer = HBufC8::New(KNumPatternRepeatsNeededForValidation*aDataPattern.Length()); 
	iValidationPatternPtr.Set(iValidationPatternBuffer->Des());
	iValidationPatternPtr.Zero();
	for(TUint i=0;i<KNumPatternRepeatsNeededForValidation;i++)
		{
		iValidationPatternPtr.Append(aDataPattern);
		}

	ReadUntilShortL(iRepeatedReadNumBytesPerRead);
	OstTraceFunctionExit1( CENDPOINTREADER_REPEATEDREADANDVALIDATEL_EXIT, this );
	}


TInt CEndpointReader::Acknowledge()
	{
	OstTraceFunctionEntry1( CENDPOINTREADER_ACKNOWLEDGE_ENTRY, this );
	TInt err(iClientDriver.SendEp0StatusPacket());
	if(err != KErrNone)
		{
		OstTrace1(TRACE_NORMAL, CENDPOINTREADER_ACKNOWLEDGE, "<Error %d> Sending acknowledge packet",err);
		}
	OstTraceFunctionExitExt( CENDPOINTREADER_ACKNOWLEDGE_EXIT, this, err );
	return err;
	}
			
			
void CEndpointReader::DoCancel()
	{
	OstTraceFunctionEntry1( CENDPOINTREADER_DOCANCEL_ENTRY, this );
	
	// Cancel reading from the endpoint
	
	iClientDriver.ReadCancel(iEndpoint);
	OstTraceFunctionExit1( CENDPOINTREADER_DOCANCEL_EXIT, this );
	}
	
	
void CEndpointReader::RunL()
	{
	OstTraceFunctionEntry1( CENDPOINTREADER_RUNL_ENTRY, this );
	OstTrace1(TRACE_NORMAL, CENDPOINTREADER_RUNL, "Endpoint %d", iEndpoint);
	OstTrace1(TRACE_NORMAL, CENDPOINTREADER_RUNL_DUP01, "Completion Action %d", iCompletionAction);
	TCompletionAction completionAction = iCompletionAction;
	iCompletionAction = ENone; //reset here in case of 'early' returns
	
	// The operation completion code
	TInt completionCode(iStatus.Int());
	
	if(completionCode != KErrNone)
		{
		OstTrace1(TRACE_NORMAL, CENDPOINTREADER_RUNL_DUP02, "<Error> void CEndpointReader::RunL()completed with ERROR %d", completionCode);
		
		// Nak the packet received
		iClientDriver.HaltEndpoint(iEndpoint);
		
		if(iHandler)
			{
			iHandler->EndpointReadError(iEndpoint,completionCode);
			}
		else
			{
			OstTrace0(TRACE_NORMAL, CENDPOINTREADER_RUNL_DUP03, "No handler set");
			}
		}
	else
		{
		// Some data has arrived but 
		TInt ent = iDataBuffer->Length()/16;
		ent = ent==0?1:ent;
		for(TInt i=0; i<iDataBuffer->Length(); i+=ent)
			{
			OstTraceExt3(TRACE_NORMAL, CENDPOINTREADER_RUNL_DUP04, "byte %d %02x %c",i,(*iDataBuffer)[i],(*iDataBuffer)[i]);
			}

		if(iHandler)
			{
			iHandler->DataReceivedFromEndpointL(iEndpoint,*iDataBuffer);
			}
		else
			{
			OstTrace0(TRACE_NORMAL, CENDPOINTREADER_RUNL_DUP05, "No handler set");
			}
		
		if(completionAction==EHaltEndpoint)
			{
			OstTrace0(TRACE_NORMAL, CENDPOINTREADER_RUNL_DUP06, "Halting Endpoint");
			iClientDriver.HaltEndpoint(iEndpoint);
			}

		if(completionAction==ERepeatedRead)
			{
			OstTrace0(TRACE_NORMAL, CENDPOINTREADER_RUNL_DUP07, "Repeated Read");
			iCompletionAction = ERepeatedRead;
			
			//Prepare to validate
			TInt previousBytesToRead = iRepeatedReadTotalNumBytes - iNumBytesReadSoFar; //PRIOR TO THIS READ
			TUint expectedNumJustReadBytes = previousBytesToRead < iRepeatedReadNumBytesPerRead ? previousBytesToRead : iRepeatedReadNumBytesPerRead;
			TPtrC8 valDesc = iValidationPatternPtr.Mid(iNumBytesReadSoFar%iDataPatternLength, expectedNumJustReadBytes);
			TInt err = iDataPtr.Compare(valDesc);

			iNumBytesReadSoFar += iDataPtr.Length();
			OstTraceExt2(TRACE_NORMAL, CENDPOINTREADER_RUNL_DUP08, "Bytes read so far %u, Total bytes to read %u", iNumBytesReadSoFar, iRepeatedReadTotalNumBytes);

			if(err!=0)
				{
				OstTraceExt3(TRACE_NORMAL, CENDPOINTREADER_RUNL_DUP09, "Validation Result %d, Validation String Length %d, Bytes Actually Read %d", err, valDesc.Length(), iDataPtr.Length());
				OstTrace0(TRACE_NORMAL, CENDPOINTREADER_RUNL_DUP10, "Expected string, followed by read string");
                OstTraceData(TRACE_NORMAL, CENDPOINTREADER_RUNL_DUP50, "", valDesc.Ptr(), valDesc.Length());
				OstTrace0(TRACE_NORMAL, CENDPOINTREADER_RUNL_DUP11, "\n");
                OstTraceData(TRACE_NORMAL, CENDPOINTREADER_RUNL_DUP51, "", iDataPtr.Ptr(), iDataPtr.Length());
				OstTrace0(TRACE_NORMAL, CENDPOINTREADER_RUNL_DUP12, "\n");
				iIsValid = EFalse; //record validation error
				}
			
			if(iNumBytesReadSoFar < iRepeatedReadTotalNumBytes)
				{
				ReadUntilShortL(iRepeatedReadNumBytesPerRead);
				}
			else
				{
				//reset
				iRepeatedReadTotalNumBytes = 0;
				iRepeatedReadNumBytesPerRead = 0;
				iNumBytesReadSoFar = 0;
				iDataPatternLength = 0;
				//do not reset iIsValid - this is required later
				}
			}
		}
	OstTraceFunctionExit1( CENDPOINTREADER_RUNL_EXIT, this );
	}

TInt CEndpointReader::RunError(TInt aError)
	{
	OstTraceFunctionEntryExt( CENDPOINTREADER_RUNERROR_ENTRY, this );
	
	OstTrace1(TRACE_NORMAL, CENDPOINTREADER_RUNERROR, "<Leaving Error> void CEndpointReader::RunError()called with ERROR %d", aError);

	// Nak the packet received
	iClientDriver.HaltEndpoint(iEndpoint);

	aError = KErrNone;	
	OstTraceFunctionExitExt( CENDPOINTREADER_RUNERROR_EXIT, this, aError );
	return aError;
	}
	
	}
