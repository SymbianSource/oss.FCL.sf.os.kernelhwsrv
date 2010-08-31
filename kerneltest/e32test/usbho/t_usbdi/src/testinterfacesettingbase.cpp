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
// @file testinterfacesettingbase.h
// @internalComponent
// 
//

#include "testinterfacesettingbase.h"
#include "testinterfacebase.h"
#include "controltransferrequests.h"
#include "OstTraceDefinitions.h"
#ifdef OST_TRACE_COMPILER_IN_USE
#include "testinterfacesettingbaseTraces.h"
#endif
 
 
namespace NUnitTesting_USBDI
	{	
	
CInterfaceSettingBase::CInterfaceSettingBase(const TDesC& aString)
:	iHashEndpointFunction(EndpointNumberHash),
	iIdRelEndpoint(EndpointIdentityRelationship),
	iEndpoints(iHashEndpointFunction,iIdRelEndpoint)
	{	
	OstTraceFunctionEntryExt( CINTERFACESETTINGBASE_CINTERFACESETTINGBASE_ENTRY, this );
	// Set the name of this interface setting
	iSettingString.Copy(aString);
	iInterfaceInfo().iString = &iSettingString;
	
	// Zero count the number of endpoints used for this setting
	iInterfaceInfo().iTotalEndpointsUsed = 0;
	OstTraceFunctionExit1( CINTERFACESETTINGBASE_CINTERFACESETTINGBASE_EXIT, this );
	}

	
CInterfaceSettingBase::~CInterfaceSettingBase()
	{
	OstTraceFunctionEntry1( CINTERFACESETTINGBASE_CINTERFACESETTINGBASE_ENTRY_DUP01, this );
	
	iEndpointReaders.ResetAndDestroy();
	iEndpointWriters.ResetAndDestroy();
	OstTraceFunctionExit1( CINTERFACESETTINGBASE_CINTERFACESETTINGBASE_EXIT_DUP01, this );
	}
	

const TDesC& CInterfaceSettingBase::Name() const
	{
	OstTraceFunctionEntry1( CINTERFACESETTINGBASE_NAME_ENTRY, this );
	OstTraceFunctionExitExt( CINTERFACESETTINGBASE_NAME_EXIT, this, ( TUint )&( iSettingString ) );
	return iSettingString;
	}


void CInterfaceSettingBase::SetClassCodeL(TUint8 aClassCode,TUint8 aSubClassCode,TUint8 aDeviceProtocol)
	{
	OstTraceFunctionEntryExt( CINTERFACESETTINGBASE_SETCLASSCODEL_ENTRY, this );
	// Set class specific information
	iInterfaceInfo().iClass = TUsbcClassInfo(aClassCode,aSubClassCode,aDeviceProtocol);
	OstTraceFunctionExit1( CINTERFACESETTINGBASE_SETCLASSCODEL_EXIT, this );
	}


TInt CInterfaceSettingBase::AddEndpoint(TEndpoint& aEndpoint)
	{
	OstTraceFunctionEntryExt( CINTERFACESETTINGBASE_ADDENDPOINT_ENTRY, this );
	
	if(aEndpoint.iEndpointNumber == EEndpoint0)
		{
		OstTraceFunctionExitExt( CINTERFACESETTINGBASE_ADDENDPOINT_EXIT, this, KErrInUse );
		return KErrInUse;
		}
	
	if(iEndpoints.Count() < KMaxEndpointsPerClient)
		{
		// Set EEndpoint1, EEndpoint2, EEndpoint3, EEndpoint4 or EEndpoint5
		iInterfaceInfo().iEndpointData[iEndpoints.Count()] = aEndpoint.iEndpointInfo;
		iEndpoints.Insert(aEndpoint.iEndpointNumber,aEndpoint);
		
		// Update the total endpoints used on this interface
		iInterfaceInfo().iTotalEndpointsUsed = iEndpoints.Count();
		
		OstTraceFunctionExitExt( CINTERFACESETTINGBASE_ADDENDPOINT_EXIT_DUP01, this, KErrNone );
		return KErrNone;
		}
	else
		{
		OstTraceFunctionExitExt( CINTERFACESETTINGBASE_ADDENDPOINT_EXIT_DUP02, this, KErrOverflow );
		return KErrOverflow;
		}
	}


void CInterfaceSettingBase::CreateEndpointReaderL(RDevUsbcClient& aClientDriver,TUint aEndpoint)
	{
	OstTraceFunctionEntryExt( CINTERFACESETTINGBASE_CREATEENDPOINTREADERL_ENTRY, this );
	// Create the reader for this endpoint and store in the container	
	CEndpointReader* epReader = new (ELeave) CEndpointReader(aClientDriver,
	iEndpoints.Find(static_cast<TEndpointNumber>(aEndpoint))->iEndpointNumber);
	CleanupStack::PushL(epReader);
	iEndpointReaders.AppendL(epReader);
	CleanupStack::Pop(epReader);
	OstTraceFunctionExit1( CINTERFACESETTINGBASE_CREATEENDPOINTREADERL_EXIT, this );
	}


void CInterfaceSettingBase::CreateEndpointWriterL(RDevUsbcClient& aClientDriver,TUint aEndpoint)
	{
	OstTraceFunctionEntryExt( CINTERFACESETTINGBASE_CREATEENDPOINTWRITERL_ENTRY, this );
	// Create the writer for this endpoint and store in the container	
	CEndpointWriter* epWriter = new (ELeave) CEndpointWriter(aClientDriver,
	iEndpoints.Find(static_cast<TEndpointNumber>(aEndpoint))->iEndpointNumber);
	CleanupStack::PushL(epWriter);
	iEndpointWriters.AppendL(epWriter);
	CleanupStack::Pop(epWriter);
	OstTraceFunctionExit1( CINTERFACESETTINGBASE_CREATEENDPOINTWRITERL_EXIT, this );
	}

void CInterfaceSettingBase::WriteSpecifiedDataToEndpointL(const TDesC8& aData,TUint16 aEndpointNumber)
	{
	OstTraceFunctionEntryExt( CINTERFACESETTINGBASE_WRITESPECIFIEDDATATOENDPOINTL_ENTRY, this );
	// Access the correct writer for the given endpoint
	// and write the data to the host			
	iEndpointWriters[aEndpointNumber-1]->Write(aData, ETrue);
	OstTraceFunctionExit1( CINTERFACESETTINGBASE_WRITESPECIFIEDDATATOENDPOINTL_EXIT, this );
	}

void CInterfaceSettingBase::WriteSpecifiedDataToEndpointL(const TDesC8& aDataPattern, const TUint aNumBytes, TUint16 aEndpointNumber)
	{
	OstTraceFunctionEntryExt( CINTERFACESETTINGBASE_WRITESPECIFIEDDATATOENDPOINTL_ENTRY_DUP01, this );
	// Access the correct writer for the given endpoint
	// and write the data to the host
	iEndpointWriters[aEndpointNumber-1]->WriteUsingPatternL(aDataPattern,aNumBytes,ETrue);
	OstTraceFunctionExit1( CINTERFACESETTINGBASE_WRITESPECIFIEDDATATOENDPOINTL_EXIT_DUP01, this );
	}

void CInterfaceSettingBase::RepeatedWriteSpecifiedDataToEndpointL(const TDesC8& aDataPattern, TUint aNumBytesPerWrite, TUint aTotalNumBytes, TUint16 aEndpointNumber)
	{
	OstTraceFunctionEntryExt( CINTERFACESETTINGBASE_REPEATEDWRITESPECIFIEDDATATOENDPOINTL_ENTRY, this );
	// Access the correct writer for the given endpoint
	// and write the data to the host
	iEndpointWriters[aEndpointNumber-1]->WriteInPartsUsingPatternL(aDataPattern,aNumBytesPerWrite,aTotalNumBytes,ETrue);
	OstTraceFunctionExit1( CINTERFACESETTINGBASE_REPEATEDWRITESPECIFIEDDATATOENDPOINTL_EXIT, this );
	}

void CInterfaceSettingBase::WriteCachedEndpointDataToEndpointL(const TUint16 aReadEndpointNumber,TUint16 aWriteEndpointNumber)
	{
	OstTraceFunctionEntryExt( CINTERFACESETTINGBASE_WRITECACHEDENDPOINTDATATOENDPOINTL_ENTRY, this );
	
	iEndpointWriters[aWriteEndpointNumber-1]->Write(iEndpointReaders[aReadEndpointNumber-1]->Buffer(), ETrue);
	OstTraceFunctionExit1( CINTERFACESETTINGBASE_WRITECACHEDENDPOINTDATATOENDPOINTL_EXIT, this );
	}

void CInterfaceSettingBase::CancelWriteDataToEndpointL(TUint16 aEndpointNumber)
	{
	OstTraceFunctionEntryExt( CINTERFACESETTINGBASE_CANCELWRITEDATATOENDPOINTL_ENTRY, this );
	// Access the correct writer for the given endpoint
	// and cancel any outstanding write. This will not of course work for 'synchronous' writes.			
	iEndpointWriters[aEndpointNumber-1]->Cancel();
	OstTraceFunctionExit1( CINTERFACESETTINGBASE_CANCELWRITEDATATOENDPOINTL_EXIT, this );
	}

void CInterfaceSettingBase::WriteSynchronousSpecifiedDataToEndpointL(const TDesC8& aDataPattern, const TUint aNumBytes, TUint16 aEndpointNumber)
	{
	OstTraceFunctionEntryExt( CINTERFACESETTINGBASE_WRITESYNCHRONOUSSPECIFIEDDATATOENDPOINTL_ENTRY, this );
	// Access the correct writer for the given endpoint
	// and write the data to the host
	iEndpointWriters[aEndpointNumber-1]->WriteSynchronousUsingPatternL(aDataPattern,aNumBytes,ETrue);
	OstTraceFunctionExit1( CINTERFACESETTINGBASE_WRITESYNCHRONOUSSPECIFIEDDATATOENDPOINTL_EXIT, this );
	}

void CInterfaceSettingBase::WriteSynchronousSpecifiedDataToAndHaltEndpointL(const TDesC8& aDataPattern, const TUint aNumBytes, TUint16 aEndpointNumber)
	{
	OstTraceFunctionEntryExt( CINTERFACESETTINGBASE_WRITESYNCHRONOUSSPECIFIEDDATATOANDHALTENDPOINTL_ENTRY, this );
	// Access the correct writer for the given endpoint
	// and write the data to the host
	iEndpointWriters[aEndpointNumber-1]->WriteSynchronousUsingPatternAndHaltL(aDataPattern,aNumBytes);
	OstTraceFunctionExit1( CINTERFACESETTINGBASE_WRITESYNCHRONOUSSPECIFIEDDATATOANDHALTENDPOINTL_EXIT, this );
	}

void CInterfaceSettingBase::WriteSynchronousCachedEndpointDataToEndpointL(const TUint16 aReadEndpointNumber,TUint16 aWriteEndpointNumber)
	{
	OstTraceFunctionEntryExt( CINTERFACESETTINGBASE_WRITESYNCHRONOUSCACHEDENDPOINTDATATOENDPOINTL_ENTRY, this );
	
	//Attempt to write the complete cached buffer by starting at zero and choosing the max length posible
	WriteSynchronousCachedEndpointDataToEndpointL(aReadEndpointNumber, aWriteEndpointNumber, 0, KMaxTInt);
	OstTraceFunctionExit1( CINTERFACESETTINGBASE_WRITESYNCHRONOUSCACHEDENDPOINTDATATOENDPOINTL_EXIT, this );
	}

void CInterfaceSettingBase::WriteSynchronousCachedEndpointDataToEndpointL(const TUint16 aReadEndpointNumber,TUint16 aWriteEndpointNumber, TUint aStartPoint, TUint aLength)
	{
	OstTraceFunctionEntryExt( CINTERFACESETTINGBASE_WRITESYNCHRONOUSCACHEDENDPOINTDATATOENDPOINTL_ENTRY_DUP01, this );
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
	OstTraceFunctionExit1( CINTERFACESETTINGBASE_WRITESYNCHRONOUSCACHEDENDPOINTDATATOENDPOINTL_EXIT_DUP01, this );
	}

TBool CInterfaceSettingBase::CachedEndpointResultL(const TUint16 aEndpointNumber)
	{
	OstTraceFunctionEntryExt( CINTERFACESETTINGBASE_CACHEDENDPOINTRESULTL_ENTRY, this );
	// Access endpoint buffer containing data from the endpoint's last read
	// and validate with the supplied data pattern

	return iEndpointReaders[aEndpointNumber-1]->IsValid();
	}

TInt CInterfaceSettingBase::NumBytesReadSoFarL(const TUint16 aEndpointNumber)
	{
	OstTraceFunctionEntryExt( CINTERFACESETTINGBASE_NUMBYTESREADSOFARL_ENTRY, this );
	// Access endpoint buffer containing data from the endpoint's last read
	// and validate with the supplied data pattern

	return iEndpointReaders[aEndpointNumber-1]->NumBytesReadSoFar();
	}

TInt CInterfaceSettingBase::NumBytesWrittenSoFarL(const TUint16 aEndpointNumber)
	{
	OstTraceFunctionEntryExt( CINTERFACESETTINGBASE_NUMBYTESWRITTENSOFARL_ENTRY, this );
	// Access endpoint buffer containing data from the endpoint's last read
	// and validate with the supplied data pattern

	return iEndpointWriters[aEndpointNumber-1]->NumBytesWrittenSoFar();
	}

TBool CInterfaceSettingBase::ValidateCachedEndpointDataL(const TDesC8& aDataPattern, const TUint aNumBytes, const TUint16 aEndpointNumber)
	{
	OstTraceFunctionEntryExt( CINTERFACESETTINGBASE_VALIDATECACHEDENDPOINTDATAL_ENTRY, this );
	// Access endpoint buffer containing data from the endpoint's last read
	// and validate with the supplied data pattern

	return ValidateCachedEndpointDataL(aDataPattern, 0, aNumBytes, aEndpointNumber);
	}

TBool CInterfaceSettingBase::ValidateCachedEndpointDataL(const TDesC8& aDataPattern, const TUint aStartPoint, const TUint aNumBytes, const TUint16 aEndpointNumber)
	{
	OstTraceFunctionEntryExt( CINTERFACESETTINGBASE_VALIDATECACHEDENDPOINTDATAL_ENTRY_DUP01, this );
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
			OstTrace0(TRACE_NORMAL, CINTERFACESETTINGBASE_VALIDATECACHEDENDPOINTDATAL, "ROUND TRIP VALIDATION: Start Bytes Match Failure");
			OstTrace1(TRACE_NORMAL, CINTERFACESETTINGBASE_VALIDATECACHEDENDPOINTDATAL_DUP01, "ROUND TRIP VALIDATION: numStartBytes = %d", numStartBytes);
			OstTrace0(TRACE_NORMAL, CINTERFACESETTINGBASE_VALIDATECACHEDENDPOINTDATAL_DUP02, "Start of sent data ...");
            const TPtrC8& midDataPattern = aDataPattern.Mid(startPoint, numStartBytes);
            OstTraceData(TRACE_NORMAL, CINTERFACESETTINGBASE_VALIDATECACHEDENDPOINTDATAL_DUP52, "", midDataPattern.Ptr(), midDataPattern.Length());
			OstTrace0(TRACE_NORMAL, CINTERFACESETTINGBASE_VALIDATECACHEDENDPOINTDATAL_DUP03, "\n");
			OstTrace0(TRACE_NORMAL, CINTERFACESETTINGBASE_VALIDATECACHEDENDPOINTDATAL_DUP04, "ROUND TRIP VALIDATION: Start of returned data ...");
            const TPtrC8& leftDataToValidate = dataToValidate.Left(numStartBytes);
            OstTraceData(TRACE_NORMAL, CINTERFACESETTINGBASE_VALIDATECACHEDENDPOINTDATAL_DUP54, "", leftDataToValidate.Ptr(), leftDataToValidate.Length());
			OstTrace0(TRACE_NORMAL, CINTERFACESETTINGBASE_VALIDATECACHEDENDPOINTDATAL_DUP05, "\n");
			OstTraceFunctionExitExt( CINTERFACESETTINGBASE_VALIDATECACHEDENDPOINTDATAL_EXIT, this, EFalse );
			return EFalse;
			}
		}
	if(numEndBytes)
		{
		if(dataToValidate.Mid(startEndPoint,numEndBytes).Compare(aDataPattern.Left(numEndBytes)) != 0)
			{
			OstTrace0(TRACE_NORMAL, CINTERFACESETTINGBASE_VALIDATECACHEDENDPOINTDATAL_DUP06, "ROUND TRIP VALIDATION: End Bytes Match Failure");
			OstTraceExt2(TRACE_NORMAL, CINTERFACESETTINGBASE_VALIDATECACHEDENDPOINTDATAL_DUP07, "ROUND TRIP VALIDATION: startEndPoint = %u, numEndBytes = %u", startEndPoint, numEndBytes);
			OstTrace0(TRACE_NORMAL, CINTERFACESETTINGBASE_VALIDATECACHEDENDPOINTDATAL_DUP08, "End of sent data ...");
            const TPtrC8& leftDataPattern = aDataPattern.Left(numEndBytes);
            OstTraceData(TRACE_NORMAL, CINTERFACESETTINGBASE_VALIDATECACHEDENDPOINTDATAL_DUP58, "", leftDataPattern.Ptr(), leftDataPattern.Length());
			OstTrace0(TRACE_NORMAL, CINTERFACESETTINGBASE_VALIDATECACHEDENDPOINTDATAL_DUP09, "\n");
			OstTrace0(TRACE_NORMAL, CINTERFACESETTINGBASE_VALIDATECACHEDENDPOINTDATAL_DUP10, "ROUND TRIP VALIDATION: End of returned data ...");
            const TPtrC8& midDataToValidate = dataToValidate.Mid(startEndPoint,numEndBytes);
            OstTraceData(TRACE_NORMAL, CINTERFACESETTINGBASE_VALIDATECACHEDENDPOINTDATAL_DUP60, "", midDataToValidate.Ptr(), midDataToValidate.Length());
			OstTrace0(TRACE_NORMAL, CINTERFACESETTINGBASE_VALIDATECACHEDENDPOINTDATAL_DUP11, "\n");
			OstTraceFunctionExitExt( CINTERFACESETTINGBASE_VALIDATECACHEDENDPOINTDATAL_EXIT_DUP01, this, EFalse );
			return EFalse;
			}
		}
	for(TInt i=0; i<fullRepeats; i++)
		{
		if(dataToValidate.Mid(numStartBytes + i*aDataPattern.Length(),aDataPattern.Length()).Compare(aDataPattern) != 0)
			{
			OstTrace1(TRACE_NORMAL, CINTERFACESETTINGBASE_VALIDATECACHEDENDPOINTDATAL_DUP12, "ROUND TRIP VALIDATION: Repeated Bytes Match Failure, Repeat %d",i);
			OstTrace0(TRACE_NORMAL, CINTERFACESETTINGBASE_VALIDATECACHEDENDPOINTDATAL_DUP13, "sent data middle block ...");
            OstTraceData(TRACE_NORMAL, CINTERFACESETTINGBASE_VALIDATECACHEDENDPOINTDATAL_DUP53, "", aDataPattern.Ptr(), aDataPattern.Length());
			OstTrace0(TRACE_NORMAL, CINTERFACESETTINGBASE_VALIDATECACHEDENDPOINTDATAL_DUP14, "\n");
			OstTrace0(TRACE_NORMAL, CINTERFACESETTINGBASE_VALIDATECACHEDENDPOINTDATAL_DUP15, "ROUND TRIP VALIDATION: Middle block of returned data ...");
            const TPtrC8& midDataToValidate = dataToValidate.Mid(numStartBytes + i*aDataPattern.Length(),aDataPattern.Length());
            OstTraceData(TRACE_NORMAL, CINTERFACESETTINGBASE_VALIDATECACHEDENDPOINTDATAL_DUP55, "", midDataToValidate.Ptr(), midDataToValidate.Length());
			OstTrace0(TRACE_NORMAL, CINTERFACESETTINGBASE_VALIDATECACHEDENDPOINTDATAL_DUP16, "\n");
			OstTraceFunctionExitExt( CINTERFACESETTINGBASE_VALIDATECACHEDENDPOINTDATAL_EXIT_DUP02, this, EFalse );
			return EFalse; //from 'for' loop
			}
		}
	OstTraceFunctionExitExt( CINTERFACESETTINGBASE_VALIDATECACHEDENDPOINTDATAL_EXIT_DUP03, this, ETrue );
	return ETrue;
	}


void CInterfaceSettingBase::ReadDataFromEndpointL(TUint aNumBytes, TUint16 aEndpointNumber)	
	{
	OstTraceFunctionEntryExt( CINTERFACESETTINGBASE_READDATAFROMENDPOINTL_ENTRY, this );
	// Access the correct readerer for the given endpoint
	// and prepare to read the data to be sent by the host			
	iEndpointReaders[aEndpointNumber-1]->ReadL(aNumBytes);
	OstTraceFunctionExit1( CINTERFACESETTINGBASE_READDATAFROMENDPOINTL_EXIT, this );
	}

void CInterfaceSettingBase::CancelAnyReadDataFromEndpointL(TUint16 aEndpointNumber)
	{
	OstTraceFunctionEntryExt( CINTERFACESETTINGBASE_CANCELANYREADDATAFROMENDPOINTL_ENTRY, this );
	// Access the correct writer for the given endpoint
	// and cancel any outstanding write. This will not of course work for 'synchronous' writes.			
	iEndpointReaders[aEndpointNumber-1]->Cancel();
	OstTraceFunctionExit1( CINTERFACESETTINGBASE_CANCELANYREADDATAFROMENDPOINTL_EXIT, this );
	}

void CInterfaceSettingBase::ReadDataFromAndHaltEndpointL(TUint aNumBytes, TUint16 aEndpointNumber)	
	{
	OstTraceFunctionEntryExt( CINTERFACESETTINGBASE_READDATAFROMANDHALTENDPOINTL_ENTRY, this );
	// Access the correct reader for the given endpoint
	// and prepare to read the data to be sent by the host			
	iEndpointReaders[aEndpointNumber-1]->ReadAndHaltL(aNumBytes);
	OstTraceFunctionExit1( CINTERFACESETTINGBASE_READDATAFROMANDHALTENDPOINTL_EXIT, this );
	}

void CInterfaceSettingBase::RepeatedReadAndValidateFromEndpointL(const TDesC8& aDataPattern, TUint aNumBytesPerRead, TUint aTotalNumBytes, TUint16 aEndpointNumber)
	{
	OstTraceFunctionEntryExt( CINTERFACESETTINGBASE_REPEATEDREADANDVALIDATEFROMENDPOINTL_ENTRY, this );
	// Access the correct reader for the given endpoint
	// and prepare to read the data to be sent by the host
	// using multiple 'Reads'
	iEndpointReaders[aEndpointNumber-1]->RepeatedReadAndValidateL(aDataPattern, aNumBytesPerRead, aTotalNumBytes);
	OstTraceFunctionExit1( CINTERFACESETTINGBASE_REPEATEDREADANDVALIDATEFROMENDPOINTL_EXIT, this );
	}

void CInterfaceSettingBase::ReadDataUntilShortFromEndpointL(TUint aNumBytes, TUint16 aEndpointNumber)	
	{
	OstTraceFunctionEntryExt( CINTERFACESETTINGBASE_READDATAUNTILSHORTFROMENDPOINTL_ENTRY, this );
	// Access the correct reader for the given endpoint
	// and prepare to read the data to be sent by the host			
	iEndpointReaders[aEndpointNumber-1]->ReadUntilShortL(aNumBytes);
	OstTraceFunctionExit1( CINTERFACESETTINGBASE_READDATAUNTILSHORTFROMENDPOINTL_EXIT, this );
	}
	}

	
