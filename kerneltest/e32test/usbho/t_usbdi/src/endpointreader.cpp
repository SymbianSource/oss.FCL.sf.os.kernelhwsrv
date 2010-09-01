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
// @file endpointreader.cpp
// @internalComponent
// 
//

#include "endpointreader.h"
#include "controltransferrequests.h"
#include "testdebug.h"

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
	CActiveScheduler::Add(this);
	}
	
CEndpointReader::~CEndpointReader()
	{
	LOG_FUNC
	Cancel();
	delete iDataBuffer;
	iDataBuffer = NULL;
	delete iValidationPatternBuffer;
	iValidationPatternBuffer = NULL;
	}
			
TPtr8 CEndpointReader::Buffer()
	{
	return iDataPtr;
	}

TBool CEndpointReader::IsValid()
	{
	return iIsValid;
	}

TUint CEndpointReader::NumBytesReadSoFar()
	{
	return iNumBytesReadSoFar;
	}

void CEndpointReader::ReadPacketL(MEndpointDataHandler* aHandler)
	{
	LOG_FUNC
	RDebug::Printf("Endpoint %d", iEndpoint);
	
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
	}


void CEndpointReader::ReadL(TInt aByteCount)
	{
	LOG_FUNC
	RDebug::Printf("Endpoint %d", iEndpoint);
	
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
	}

void CEndpointReader::ReadUntilShortL(TInt aByteCount)
	{
	LOG_FUNC
	RDebug::Printf("Endpoint %d", iEndpoint);
	
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
	}

void CEndpointReader::ReadAndHaltL(TInt aByteCount)
	{
	LOG_FUNC
	iCompletionAction = EHaltEndpoint;
	ReadL(aByteCount);
	}

void CEndpointReader::RepeatedReadAndValidateL(const TDesC8& aDataPattern, TUint aNumBytesPerRead, TUint aTotalNumBytes)
	{
	LOG_FUNC

	iCompletionAction = ERepeatedRead;
	iRepeatedReadTotalNumBytes = aTotalNumBytes;
	iRepeatedReadNumBytesPerRead = aNumBytesPerRead;
	iNumBytesReadSoFar = 0;
	iDataPatternLength = aDataPattern.Length();
	iIsValid = ETrue; //until proven guilty!
	RDebug::Printf("Total Bytes To Read: %d, Bytes Per Individual Read: %d", iRepeatedReadTotalNumBytes, iRepeatedReadNumBytesPerRead);
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
	}


TInt CEndpointReader::Acknowledge()
	{
	LOG_FUNC
	TInt err(iClientDriver.SendEp0StatusPacket());
	if(err != KErrNone)
		{
		RDebug::Printf("<Error %d> Sending acknowledge packet",err);
		}
	return err;
	}
			
			
void CEndpointReader::DoCancel()
	{
	LOG_FUNC
	
	// Cancel reading from the endpoint
	
	iClientDriver.ReadCancel(iEndpoint);
	}
	
	
void CEndpointReader::RunL()
	{
	LOG_FUNC
	RDebug::Printf("Endpoint %d", iEndpoint);
	RDebug::Printf("Completion Action %d", iCompletionAction);
	TCompletionAction completionAction = iCompletionAction;
	iCompletionAction = ENone; //reset here in case of 'early' returns
	
	// The operation completion code
	TInt completionCode(iStatus.Int());
	
	if(completionCode != KErrNone)
		{
		RDebug::Printf("<Error> void CEndpointReader::RunL()completed with ERROR %d", completionCode);
		
		// Nak the packet received
		iClientDriver.HaltEndpoint(iEndpoint);
		
		if(iHandler)
			{
			iHandler->EndpointReadError(iEndpoint,completionCode);
			}
		else
			{
			RDebug::Printf("No handler set");
			}
		}
	else
		{
		// Some data has arrived but 
		TInt ent = iDataBuffer->Length()/16;
		ent = ent==0?1:ent;
		for(TInt i=0; i<iDataBuffer->Length(); i+=ent)
			{
			RDebug::Printf("byte %d %02x %c",i,(*iDataBuffer)[i],(*iDataBuffer)[i]);
			}

		if(iHandler)
			{
			iHandler->DataReceivedFromEndpointL(iEndpoint,*iDataBuffer);
			}
		else
			{
			RDebug::Printf("No handler set");
			}
		
		if(completionAction==EHaltEndpoint)
			{
			RDebug::Printf("Halting Endpoint");
			iClientDriver.HaltEndpoint(iEndpoint);
			}

		if(completionAction==ERepeatedRead)
			{
			RDebug::Printf("Repeated Read");
			iCompletionAction = ERepeatedRead;
			
			//Prepare to validate
			TInt previousBytesToRead = iRepeatedReadTotalNumBytes - iNumBytesReadSoFar; //PRIOR TO THIS READ
			TUint expectedNumJustReadBytes = previousBytesToRead < iRepeatedReadNumBytesPerRead ? previousBytesToRead : iRepeatedReadNumBytesPerRead;
			TPtrC8 valDesc = iValidationPatternPtr.Mid(iNumBytesReadSoFar%iDataPatternLength, expectedNumJustReadBytes);
			TInt err = iDataPtr.Compare(valDesc);

			iNumBytesReadSoFar += iDataPtr.Length();
			RDebug::Printf("Bytes read so far %d, Total bytes to read %d", iNumBytesReadSoFar, iRepeatedReadTotalNumBytes);

			if(err!=0)
				{
				RDebug::Printf("Validation Result %d, Validation String Length %d, Bytes Actually Read %d", err, valDesc.Length(), iDataPtr.Length());
				RDebug::Printf("Expected string, followed by read string");
				RDebug::RawPrint(valDesc);
				RDebug::Printf("\n");
				RDebug::RawPrint(iDataPtr);
				RDebug::Printf("\n");
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
	}

TInt CEndpointReader::RunError(TInt aError)
	{
	LOG_FUNC
	
	RDebug::Printf("<Leaving Error> void CEndpointReader::RunError()called with ERROR %d", aError);

	// Nak the packet received
	iClientDriver.HaltEndpoint(iEndpoint);

	aError = KErrNone;	
	return aError;
	}
	
	}
