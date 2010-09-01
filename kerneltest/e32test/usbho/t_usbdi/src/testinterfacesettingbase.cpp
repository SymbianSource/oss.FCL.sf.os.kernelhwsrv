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
// @file testinterfacesettingbase.h
// @internalComponent
// 
//

#include "testinterfacesettingbase.h"
#include "testinterfacebase.h"
#include "controltransferrequests.h"
 
 
namespace NUnitTesting_USBDI
	{	
	
CInterfaceSettingBase::CInterfaceSettingBase(const TDesC& aString)
:	iHashEndpointFunction(EndpointNumberHash),
	iIdRelEndpoint(EndpointIdentityRelationship),
	iEndpoints(iHashEndpointFunction,iIdRelEndpoint)
	{	
	// Set the name of this interface setting
	iSettingString.Copy(aString);
	iInterfaceInfo().iString = &iSettingString;
	
	// Zero count the number of endpoints used for this setting
	iInterfaceInfo().iTotalEndpointsUsed = 0;
	}

	
CInterfaceSettingBase::~CInterfaceSettingBase()
	{
	LOG_FUNC
	
	iEndpointReaders.ResetAndDestroy();
	iEndpointWriters.ResetAndDestroy();
	}
	

const TDesC& CInterfaceSettingBase::Name() const
	{
	return iSettingString;
	}


void CInterfaceSettingBase::SetClassCodeL(TUint8 aClassCode,TUint8 aSubClassCode,TUint8 aDeviceProtocol)
	{
	// Set class specific information
	iInterfaceInfo().iClass = TUsbcClassInfo(aClassCode,aSubClassCode,aDeviceProtocol);
	}


TInt CInterfaceSettingBase::AddEndpoint(TEndpoint& aEndpoint)
	{
	LOG_FUNC
	
	if(aEndpoint.iEndpointNumber == EEndpoint0)
		{
		return KErrInUse;
		}
	
	if(iEndpoints.Count() < KMaxEndpointsPerClient)
		{
		// Set EEndpoint1, EEndpoint2, EEndpoint3, EEndpoint4 or EEndpoint5
		iInterfaceInfo().iEndpointData[iEndpoints.Count()] = aEndpoint.iEndpointInfo;
		iEndpoints.Insert(aEndpoint.iEndpointNumber,aEndpoint);
		
		// Update the total endpoints used on this interface
		iInterfaceInfo().iTotalEndpointsUsed = iEndpoints.Count();
		
		return KErrNone;
		}
	else
		{
		return KErrOverflow;
		}
	}


void CInterfaceSettingBase::CreateEndpointReaderL(RDevUsbcClient& aClientDriver,TUint aEndpoint)
	{
	LOG_FUNC	
	// Create the reader for this endpoint and store in the container	
	CEndpointReader* epReader = new (ELeave) CEndpointReader(aClientDriver,
	iEndpoints.Find(static_cast<TEndpointNumber>(aEndpoint))->iEndpointNumber);
	CleanupStack::PushL(epReader);
	iEndpointReaders.AppendL(epReader);
	CleanupStack::Pop(epReader);
	}


void CInterfaceSettingBase::CreateEndpointWriterL(RDevUsbcClient& aClientDriver,TUint aEndpoint)
	{
	LOG_FUNC	
	// Create the writer for this endpoint and store in the container	
	CEndpointWriter* epWriter = new (ELeave) CEndpointWriter(aClientDriver,
	iEndpoints.Find(static_cast<TEndpointNumber>(aEndpoint))->iEndpointNumber);
	CleanupStack::PushL(epWriter);
	iEndpointWriters.AppendL(epWriter);
	CleanupStack::Pop(epWriter);
	}

void CInterfaceSettingBase::WriteSpecifiedDataToEndpointL(const TDesC8& aData,TUint16 aEndpointNumber)
	{
	LOG_FUNC	
	// Access the correct writer for the given endpoint
	// and write the data to the host			
	iEndpointWriters[aEndpointNumber-1]->Write(aData, ETrue);
	}

void CInterfaceSettingBase::WriteSpecifiedDataToEndpointL(const TDesC8& aDataPattern, const TUint aNumBytes, TUint16 aEndpointNumber)
	{
	LOG_FUNC	
	// Access the correct writer for the given endpoint
	// and write the data to the host
	iEndpointWriters[aEndpointNumber-1]->WriteUsingPatternL(aDataPattern,aNumBytes,ETrue);
	}

void CInterfaceSettingBase::RepeatedWriteSpecifiedDataToEndpointL(const TDesC8& aDataPattern, TUint aNumBytesPerWrite, TUint aTotalNumBytes, TUint16 aEndpointNumber)
	{
	LOG_FUNC	
	// Access the correct writer for the given endpoint
	// and write the data to the host
	iEndpointWriters[aEndpointNumber-1]->WriteInPartsUsingPatternL(aDataPattern,aNumBytesPerWrite,aTotalNumBytes,ETrue);
	}

void CInterfaceSettingBase::WriteCachedEndpointDataToEndpointL(const TUint16 aReadEndpointNumber,TUint16 aWriteEndpointNumber)
	{
	LOG_FUNC
	
	iEndpointWriters[aWriteEndpointNumber-1]->Write(iEndpointReaders[aReadEndpointNumber-1]->Buffer(), ETrue);
	}

void CInterfaceSettingBase::CancelWriteDataToEndpointL(TUint16 aEndpointNumber)
	{
	LOG_FUNC	
	// Access the correct writer for the given endpoint
	// and cancel any outstanding write. This will not of course work for 'synchronous' writes.			
	iEndpointWriters[aEndpointNumber-1]->Cancel();
	}

void CInterfaceSettingBase::WriteSynchronousSpecifiedDataToEndpointL(const TDesC8& aDataPattern, const TUint aNumBytes, TUint16 aEndpointNumber)
	{
	LOG_FUNC	
	// Access the correct writer for the given endpoint
	// and write the data to the host
	iEndpointWriters[aEndpointNumber-1]->WriteSynchronousUsingPatternL(aDataPattern,aNumBytes,ETrue);
	}

void CInterfaceSettingBase::WriteSynchronousSpecifiedDataToAndHaltEndpointL(const TDesC8& aDataPattern, const TUint aNumBytes, TUint16 aEndpointNumber)
	{
	LOG_FUNC	
	// Access the correct writer for the given endpoint
	// and write the data to the host
	iEndpointWriters[aEndpointNumber-1]->WriteSynchronousUsingPatternAndHaltL(aDataPattern,aNumBytes);
	}

void CInterfaceSettingBase::WriteSynchronousCachedEndpointDataToEndpointL(const TUint16 aReadEndpointNumber,TUint16 aWriteEndpointNumber)
	{
	LOG_FUNC
	
	//Attempt to write the complete cached buffer by starting at zero and choosing the max length posible
	WriteSynchronousCachedEndpointDataToEndpointL(aReadEndpointNumber, aWriteEndpointNumber, 0, KMaxTInt);
	}

void CInterfaceSettingBase::WriteSynchronousCachedEndpointDataToEndpointL(const TUint16 aReadEndpointNumber,TUint16 aWriteEndpointNumber, TUint aStartPoint, TUint aLength)
	{
	LOG_FUNC	
	// Access the correct writer for the given endpoint
	// Access the 'source' endpoint buffer. This contains data from that endpoint's last read.
	// Write this data to the host using 'write' endpoint.
	
	// Check data
	TPtr8 dataPtr(iEndpointReaders[aReadEndpointNumber-1]->Buffer());
	User::LeaveIfError(dataPtr.MaxLength() == 0);
	if(aStartPoint+aLength>dataPtr.Length())
		//allow use of excessive length to imply a write of all the rest of the buffer...
		//..otherwise 'Mid' would panic!
		{
		aLength = dataPtr.Length() - aStartPoint;
		}
	
	// Do the 'Write'
	// Note we need a synchronous 'Write' here
	iEndpointWriters[aWriteEndpointNumber-1]->WriteSynchronous(iEndpointReaders[aReadEndpointNumber-1]->Buffer().Mid(aStartPoint, aLength), ETrue);
	}

TBool CInterfaceSettingBase::CachedEndpointResultL(const TUint16 aEndpointNumber)
	{
	LOG_FUNC	
	// Access endpoint buffer containing data from the endpoint's last read
	// and validate with the supplied data pattern

	return iEndpointReaders[aEndpointNumber-1]->IsValid();
	}

TInt CInterfaceSettingBase::NumBytesReadSoFarL(const TUint16 aEndpointNumber)
	{
	LOG_FUNC	
	// Access endpoint buffer containing data from the endpoint's last read
	// and validate with the supplied data pattern

	return iEndpointReaders[aEndpointNumber-1]->NumBytesReadSoFar();
	}

TInt CInterfaceSettingBase::NumBytesWrittenSoFarL(const TUint16 aEndpointNumber)
	{
	LOG_FUNC	
	// Access endpoint buffer containing data from the endpoint's last read
	// and validate with the supplied data pattern

	return iEndpointWriters[aEndpointNumber-1]->NumBytesWrittenSoFar();
	}

TBool CInterfaceSettingBase::ValidateCachedEndpointDataL(const TDesC8& aDataPattern, const TUint aNumBytes, const TUint16 aEndpointNumber)
	{
	LOG_FUNC	
	// Access endpoint buffer containing data from the endpoint's last read
	// and validate with the supplied data pattern

	return ValidateCachedEndpointDataL(aDataPattern, 0, aNumBytes, aEndpointNumber);
	}

TBool CInterfaceSettingBase::ValidateCachedEndpointDataL(const TDesC8& aDataPattern, const TUint aStartPoint, const TUint aNumBytes, const TUint16 aEndpointNumber)
	{
	LOG_FUNC	
	__ASSERT_DEBUG(aDataPattern.Length()!=0, User::Panic(_L("Trying to validate with ZERO LENGTH STRING"), KErrArgument));

	//Check data in endpoint buffer
	TPtr8 dataToValidate(iEndpointReaders[aEndpointNumber-1]->Buffer());
	User::LeaveIfError(dataToValidate.MaxLength() == 0);

	TUint startPoint = aStartPoint%aDataPattern.Length();
	TUint numStartBytes = (aDataPattern.Length() - startPoint)%aDataPattern.Length();
	numStartBytes = aNumBytes<numStartBytes?aNumBytes:numStartBytes; //never test for more than aNumBytes
	TUint fullRepeats = (aNumBytes-numStartBytes)/aDataPattern.Length();
	TUint startEndPoint = (fullRepeats*aDataPattern.Length()) + numStartBytes;
	TUint numEndBytes = aNumBytes - startEndPoint;//fullRepeats*aDataPattern.Length() - numStartBytes;
	if(numStartBytes)
		{
		if(dataToValidate.Left(numStartBytes).Compare(aDataPattern.Mid(startPoint, numStartBytes)) != 0)
			{
			RDebug::Printf("ROUND TRIP VALIDATION: Start Bytes Match Failure");
			RDebug::Printf("ROUND TRIP VALIDATION: numStartBytes = %d", numStartBytes);
			RDebug::Printf("Start of sent data ...");
			RDebug::RawPrint(aDataPattern.Mid(startPoint, numStartBytes));
			RDebug::Printf("\n");
			RDebug::Printf("ROUND TRIP VALIDATION: Start of returned data ...");				
			RDebug::RawPrint(dataToValidate.Left(numStartBytes));
			RDebug::Printf("\n");
			return EFalse;
			}
		}
	if(numEndBytes)
		{
		if(dataToValidate.Mid(startEndPoint,numEndBytes).Compare(aDataPattern.Left(numEndBytes)) != 0)
			{
			RDebug::Printf("ROUND TRIP VALIDATION: End Bytes Match Failure");
			RDebug::Printf("ROUND TRIP VALIDATION: startEndPoint = %d, numEndBytes = %d", startEndPoint, numEndBytes);
			RDebug::Printf("End of sent data ...");
			RDebug::RawPrint(aDataPattern.Left(numEndBytes));
			RDebug::Printf("\n");
			RDebug::Printf("ROUND TRIP VALIDATION: End of returned data ...");				
			RDebug::RawPrint(dataToValidate.Mid(startEndPoint,numEndBytes));
			RDebug::Printf("\n");
			return EFalse;
			}
		}
	for(TInt i=0; i<fullRepeats; i++)
		{
		if(dataToValidate.Mid(numStartBytes + i*aDataPattern.Length(),aDataPattern.Length()).Compare(aDataPattern) != 0)
			{
			RDebug::Printf("ROUND TRIP VALIDATION: Repeated Bytes Match Failure, Repeat %d",i);
			RDebug::Printf("sent data middle block ...");
			RDebug::RawPrint(aDataPattern);
			RDebug::Printf("\n");
			RDebug::Printf("ROUND TRIP VALIDATION: Middle block of returned data ...");
			RDebug::RawPrint(dataToValidate.Mid(numStartBytes + i*aDataPattern.Length(),aDataPattern.Length()));
			RDebug::Printf("\n");
			return EFalse; //from 'for' loop
			}
		}
	return ETrue;
	}


void CInterfaceSettingBase::ReadDataFromEndpointL(TUint aNumBytes, TUint16 aEndpointNumber)	
	{
	LOG_FUNC	
	// Access the correct readerer for the given endpoint
	// and prepare to read the data to be sent by the host			
	iEndpointReaders[aEndpointNumber-1]->ReadL(aNumBytes);
	}

void CInterfaceSettingBase::CancelAnyReadDataFromEndpointL(TUint16 aEndpointNumber)
	{
	LOG_FUNC	
	// Access the correct writer for the given endpoint
	// and cancel any outstanding write. This will not of course work for 'synchronous' writes.			
	iEndpointReaders[aEndpointNumber-1]->Cancel();
	}

void CInterfaceSettingBase::ReadDataFromAndHaltEndpointL(TUint aNumBytes, TUint16 aEndpointNumber)	
	{
	LOG_FUNC	
	// Access the correct reader for the given endpoint
	// and prepare to read the data to be sent by the host			
	iEndpointReaders[aEndpointNumber-1]->ReadAndHaltL(aNumBytes);
	}

void CInterfaceSettingBase::RepeatedReadAndValidateFromEndpointL(const TDesC8& aDataPattern, TUint aNumBytesPerRead, TUint aTotalNumBytes, TUint16 aEndpointNumber)
	{
	LOG_FUNC	
	// Access the correct reader for the given endpoint
	// and prepare to read the data to be sent by the host
	// using multiple 'Reads'
	iEndpointReaders[aEndpointNumber-1]->RepeatedReadAndValidateL(aDataPattern, aNumBytesPerRead, aTotalNumBytes);
	}

void CInterfaceSettingBase::ReadDataUntilShortFromEndpointL(TUint aNumBytes, TUint16 aEndpointNumber)	
	{
	LOG_FUNC	
	// Access the correct reader for the given endpoint
	// and prepare to read the data to be sent by the host			
	iEndpointReaders[aEndpointNumber-1]->ReadUntilShortL(aNumBytes);
	}
	}

	
