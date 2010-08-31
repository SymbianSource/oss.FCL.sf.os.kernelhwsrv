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
// @file testinterfacebase.cpp
// @internalComponent
// 
//

#include "testdevicebase.h"
#include "testinterfacebase.h"
#include "testinterfacesettingbase.h"
#include "testdebug.h"
#include "controltransferrequests.h"
#include "endpointwriter.h"
#include "OstTraceDefinitions.h"
#ifdef OST_TRACE_COMPILER_IN_USE
#include "TestInterfaceBaseTraces.h"
#endif


namespace NUnitTesting_USBDI
	{

	
CInterfaceBase::CInterfaceBase(RUsbTestDevice& aTestDevice,const TDesC16& aName)
:	iDevice(aTestDevice),
	iInterfaceName(aName),
	iCurrentAlternateInterfaceSetting(0) // The default alternate interface setting will be zero when opened
	{
	OstTraceFunctionEntryExt( CINTERFACEBASE_CINTERFACEBASE_ENTRY, this );
	OstTraceFunctionExit1( CINTERFACEBASE_CINTERFACEBASE_EXIT, this );
	}
	
CInterfaceBase::~CInterfaceBase()
	{
	OstTraceFunctionEntry1( CINTERFACEBASE_CINTERFACEBASE_ENTRY_DUP01, this );
	
	delete iAuxBuffer;
	delete iStallWatcher;
	delete iSelectionWatcher;
		
	// Release all interfaces
	delete iEp0Writer;
	
	// Destroy the endpoint 0 reader
	delete iEp0Reader;
		
	// Destroy interface settings
	iAlternateSettings.ResetAndDestroy();
	
	// Close the channel to the driver
	iClientDriver.Close();
	OstTraceFunctionExit1( CINTERFACEBASE_CINTERFACEBASE_EXIT_DUP01, this );
	}
	
	
void CInterfaceBase::BaseConstructL()
	{
	OstTraceFunctionEntry1( CINTERFACEBASE_BASECONSTRUCTL_ENTRY, this );
	// Open channel to driver
	TInt err(iClientDriver.Open(0));
	if(err != KErrNone)
		{
		OstTrace1(TRACE_NORMAL, CINTERFACEBASE_BASECONSTRUCTL, "<Error %d> Unable to open a channel to USB client driver",err);
		User::Leave(err);
		}

	// Start the watcher for host selecting alternate interface settings
	iSelectionWatcher = CAlternateInterfaceSelectionWatcher::NewL(iClientDriver,*this);
	
	//
	iStallWatcher = new (ELeave) CEndpointStallWatcher(iClientDriver);
	
	// Create the Ep0 reader/writer
	iEp0Reader = new (ELeave) CControlEndpointReader(iClientDriver,*this);
	iEp0Writer = new (ELeave) CEndpointWriter(iClientDriver,EEndpoint0);

	// Hide bus from host while interfaces are being set up
	iClientDriver.DeviceDisconnectFromHost();
	OstTraceFunctionExit1( CINTERFACEBASE_BASECONSTRUCTL_EXIT, this );
	}


void CInterfaceBase::AddInterfaceSettingL(CInterfaceSettingBase* aInterfaceSetting)
	{
	OstTraceFunctionEntryExt( CINTERFACEBASE_ADDINTERFACESETTINGL_ENTRY, this );
	
	// Append to the container
	TInt err(iAlternateSettings.Append(aInterfaceSetting));
	if(err != KErrNone)
		{
		OstTrace1(TRACE_NORMAL, CINTERFACEBASE_ADDINTERFACESETTINGL, "<Error %d> Unable to add interface setting",err);
		User::Leave(err);
		}
	
	// Get the current number of alternate interface settings	
	TInt alternateSettingNumber(iAlternateSettings.Count()-1);

	TUint endpointSettingCount(aInterfaceSetting->iInterfaceInfo().iTotalEndpointsUsed);
	
	if(endpointSettingCount > 0)
		{
		OstTrace1(TRACE_NORMAL, CINTERFACEBASE_ADDINTERFACESETTINGL_DUP01, "%u endpoint(s) to configure for this interface setting",endpointSettingCount);
				
		// Device capabilities
		TUsbDeviceCaps devCaps;
		err = iClientDriver.DeviceCaps(devCaps);
		if(err != KErrNone)
			{
			OstTrace1(TRACE_NORMAL, CINTERFACEBASE_ADDINTERFACESETTINGL_DUP02, "<Error %d> Unable to retrieve device capabilities",err);
			User::Leave(err);
			}

		// Endpoint capabilities
		TUsbcEndpointData endpointCaps[KUsbcMaxEndpoints];
		TPtr8 dataptr(reinterpret_cast<TUint8*>(endpointCaps), sizeof(endpointCaps), sizeof(endpointCaps));
		err = iClientDriver.EndpointCaps(dataptr);
		if(err != KErrNone)
			{
			OstTrace1(TRACE_NORMAL, CINTERFACEBASE_ADDINTERFACESETTINGL_DUP03, "<Error %d> Unable to get endpoint capabilities",err);
			User::Leave(err);
			}		
		
		TInt totalEndpoints(devCaps().iTotalEndpoints);
		
		// Loop through available hardware endpoints to find suitable one
		// i.e. endpoints that can be configured
		
		TUint epIndex(0);
		TUint epCount(0);
		
		for(; epIndex<totalEndpoints; epIndex++)
			{
			OstTrace1(TRACE_NORMAL, CINTERFACEBASE_ADDINTERFACESETTINGL_DUP04, "Examining hardware endpoint %u",epIndex);
			const TUsbcEndpointData ep = endpointCaps[epIndex];
			
			// Check the endpoint index to see if already claimed
			if(!ep.iInUse)
				{			
				OstTrace0(TRACE_NORMAL, CINTERFACEBASE_ADDINTERFACESETTINGL_DUP05, "...its free");
				const TUsbcEndpointCaps caps(ep.iCaps);
				
				// Information about the endpoint we are looking for	
				TUsbcEndpointInfo& endpointSpec = aInterfaceSetting->iInterfaceInfo().iEndpointData[epCount];
						
				if( (caps.iTypesAndDir & (endpointSpec.iDir | endpointSpec.iType)) == 
					(endpointSpec.iDir | endpointSpec.iType) )
					{
					// Found suitable endpoint
					
					// Create the reader/writer for this endpoint					
					
					if(endpointSpec.iDir == KUsbEpDirIn)
						{
						// Create an endpoint writer for this endpoint
						
						aInterfaceSetting->CreateEndpointWriterL(iClientDriver,(epCount+1));
						OstTrace1(TRACE_NORMAL, CINTERFACEBASE_ADDINTERFACESETTINGL_DUP06, "Created endpoint writer for endpoint%d",epCount+1);
						}
					else if(endpointSpec.iDir == KUsbEpDirOut)
						{
						// Create an endpoint reader for this endpoint
												
						aInterfaceSetting->CreateEndpointReaderL(iClientDriver,epCount+1);
						OstTrace1(TRACE_NORMAL, CINTERFACEBASE_ADDINTERFACESETTINGL_DUP07, "Created endpoint reader for endpoint%d",epCount+1);
						}					
					
					epCount++; // Increment to next endpoint spec
					OstTrace1(TRACE_NORMAL, CINTERFACEBASE_ADDINTERFACESETTINGL_DUP08, "Endpoint %u configured",epCount);
					endpointSpec.iSize = caps.MaxPacketSize();
					
					if(epCount >= endpointSettingCount)
						{
						// Found all desired endpoints
						break;
						}
					}
				}
			else
				{
				OstTrace0(TRACE_NORMAL, CINTERFACEBASE_ADDINTERFACESETTINGL_DUP09, "...its busy");
				}
			}
		
		OstTraceExt2(TRACE_NORMAL, CINTERFACEBASE_ADDINTERFACESETTINGL_DUP10, "Configure %u out of %u endpoints",epCount,endpointSettingCount);			
		
		if(epCount < endpointSettingCount)
			{
			OstTraceExt3(TRACE_NORMAL, CINTERFACEBASE_ADDINTERFACESETTINGL_DUP11, "<Error %d> Only managed to configure %u out of %u endpoints",KErrNotFound,epCount,endpointSettingCount);
			User::Leave(KErrNotFound);
			}			
		}
	else
		{
		OstTrace0(TRACE_NORMAL, CINTERFACEBASE_ADDINTERFACESETTINGL_DUP12, "No endpoints for this interface setting");
		}
	
	// Add the new setting to the device
	err = iClientDriver.SetInterface(alternateSettingNumber,aInterfaceSetting->iInterfaceInfo);
	if(err != KErrNone)
		{
		OstTraceExt2(TRACE_NORMAL, CINTERFACEBASE_ADDINTERFACESETTINGL_DUP13, "<Error %d> Unable to set the alternate interface setting %d",err,alternateSettingNumber);
		User::Leave(err);
		}
	
	OstTrace1(TRACE_NORMAL, CINTERFACEBASE_ADDINTERFACESETTINGL_DUP14, "Alternate interface setting %d set",alternateSettingNumber);
	OstTraceFunctionExit1( CINTERFACEBASE_ADDINTERFACESETTINGL_EXIT, this );
	}


TInt CInterfaceBase::StallEndpoint(TUint16 aEndpointNumber)
	{
	OstTraceFunctionEntryExt( CINTERFACEBASE_STALLENDPOINT_ENTRY, this );
	
	OstTrace1(TRACE_NORMAL, CINTERFACEBASE_STALLENDPOINT, "Stalling endpoint%d",aEndpointNumber);
	return iClientDriver.HaltEndpoint(static_cast<TEndpointNumber>(aEndpointNumber));
	}

	
CInterfaceSettingBase& CInterfaceBase::AlternateSetting(TInt aSettingNumber) const
	{
	OstTraceFunctionEntryExt( CINTERFACEBASE_ALTERNATESETTING_ENTRY, this );
	OstTraceFunctionExit1( CINTERFACEBASE_ALTERNATESETTING_EXIT, this );
	return *iAlternateSettings[aSettingNumber];
	}

	
TInt CInterfaceBase::InterfaceSettingCount() const
	{
	OstTraceFunctionEntry1( CINTERFACEBASE_INTERFACESETTINGCOUNT_ENTRY, this );
	return iAlternateSettings.Count();
	}


TUint32 CInterfaceBase::ExtractNumberL(const TDesC8& aPayload)
	{
OstTraceFunctionEntryExt( CINTERFACEBASE_EXTRACTNUMBERL_ENTRY, this );

	// Read the number of repeats and the data supplied by the host, on the specified endpoint
	TLex8 lex(aPayload.Left(KNumberStringLength));
	TUint32 numBytes;
	User::LeaveIfError(lex.Val(numBytes, EDecimal));
	OstTrace1(TRACE_NORMAL, CINTERFACEBASE_EXTRACTNUMBERL, "Writing %d bytes using string pattern below to IN endpoint",numBytes);
	const TPtrC8& midPayload = aPayload.Mid(KNumberStringLength);
    OstTraceData(TRACE_NORMAL, CINTERFACEBASE_EXTRACTNUMBERL_DUP50, "", midPayload.Ptr(), midPayload.Length());
	OstTrace0(TRACE_NORMAL, CINTERFACEBASE_EXTRACTNUMBERL_DUP01, "");
	OstTraceFunctionExitExt( CINTERFACEBASE_EXTRACTNUMBERL_EXIT, this, ( TUint )( numBytes ) );
	return numBytes;
	}

void CInterfaceBase::ExtractTwoNumbersL(const TDesC8& aPayload, TUint32& aFirstNum, TUint32& aSecondNum)
	{
OstTraceFunctionEntryExt( CINTERFACEBASE_EXTRACTTWONUMBERSL_ENTRY, this );

	// Read the number of repeats and the data supplied by the host, on the specified endpoint
	TLex8 lex1(aPayload.Left(KNumberStringLength));
	User::LeaveIfError(lex1.Val(aFirstNum, EDecimal));
	TLex8 lex2(aPayload.Mid(KNumberStringLength, KNumberStringLength));
	User::LeaveIfError(lex2.Val(aSecondNum, EDecimal));
	OstTraceExt2(TRACE_NORMAL, CINTERFACEBASE_EXTRACTTWONUMBERSL, "Writing or Reading a total of %d bytes in repeats of %d bytes using string pattern below to IN endpoint",aFirstNum,aSecondNum);
	const TPtrC8& midPayload = aPayload.Mid(2*KNumberStringLength);
    OstTraceData(TRACE_NORMAL, CINTERFACEBASE_EXTRACTTWONUMBERSL_DUP50, "", midPayload.Ptr(), midPayload.Length());
	OstTrace0(TRACE_NORMAL, CINTERFACEBASE_EXTRACTTWONUMBERSL_DUP01, "");
	OstTraceFunctionExit1( CINTERFACEBASE_EXTRACTTWONUMBERSL_EXIT, this );
	return;
	}

void CInterfaceBase::AlternateInterfaceSelectedL(TInt aAlternateInterfaceSetting)
	{
	OstTraceFunctionEntryExt( CINTERFACEBASE_ALTERNATEINTERFACESELECTEDL_ENTRY, this );
	OstTraceExt1(TRACE_NORMAL, CINTERFACEBASE_ALTERNATEINTERFACESELECTEDL, "Interface %S:",iInterfaceName);	
	iCurrentAlternateInterfaceSetting = aAlternateInterfaceSetting;
	OstTraceFunctionExit1( CINTERFACEBASE_ALTERNATEINTERFACESELECTEDL_EXIT, this );
	}


TInt CInterfaceBase::ProcessRequestL(TUint8 aRequest,TUint16 aValue,TUint16 aIndex,
	TUint16 aDataReqLength,const TDesC8& aPayload)
	{
	OstTraceFunctionEntryExt( CINTERFACEBASE_PROCESSREQUESTL_ENTRY, this );
	OstTraceExt1(TRACE_NORMAL, CINTERFACEBASE_PROCESSREQUESTL, "Interface %S:",iInterfaceName);
	
	switch(aRequest)
		{
		case KVendorEmptyRequest:
			// Acknowledge the request and do nothing
			iEp0Reader->Acknowledge();
			
			OstTrace0(TRACE_NORMAL, CINTERFACEBASE_PROCESSREQUESTL_DUP01, "Request: Empty");
			break;
			
		case KVendorPutPayloadRequest:
			// Acknowledge the request
			iEp0Reader->Acknowledge();
			
			OstTrace0(TRACE_NORMAL, CINTERFACEBASE_PROCESSREQUESTL_DUP02, "Put payload");
			if(aPayload.Compare(_L8("DEADBEEF")) != 0)
				{
				OstTrace1(TRACE_NORMAL, CINTERFACEBASE_PROCESSREQUESTL_DUP03, "<Error %d> Payload not as expected",KErrCorrupt);
				iDevice.ReportError(KErrCorrupt);
				}
			break;
			
		case KVendorGetPayloadRequest:
			{
			OstTrace0(TRACE_NORMAL, CINTERFACEBASE_PROCESSREQUESTL_DUP04, "Get payload");
			__ASSERT_DEBUG(iAuxBuffer, User::Panic(_L("Trying to write non-allocated buffer"), KErrGeneral));
			OstTrace0(TRACE_NORMAL, CINTERFACEBASE_PROCESSREQUESTL_DUP05, "iAuxBuffer = ....");
            OstTraceData(TRACE_NORMAL, CINTERFACEBASE_PROCESSREQUESTL_DUP55, "", iAuxBuffer->Ptr(), iAuxBuffer->Length());
			OstTrace0(TRACE_NORMAL, CINTERFACEBASE_PROCESSREQUESTL_DUP06, "\n");
			
			//Perform synchronous write to EP0
			//This allows the subsequent 'Read' request to
			//take place
			TInt ret = iEp0Writer->WriteSynchronous(*iAuxBuffer, ETrue);
			OstTrace1(TRACE_NORMAL, CINTERFACEBASE_PROCESSREQUESTL_DUP07, "Write (from interface callback) executed with error %d", ret);
			}
			break;
			
		case KVendorGetRecordedNumBytesReadInPayloadRequest:
			{
			delete iAuxBuffer;
			iAuxBuffer = HBufC8::NewL(KNumberStringLength);
			TPtr8 ptr(iAuxBuffer->Des());
			TInt retValue = 0;
			retValue = AlternateSetting(iCurrentAlternateInterfaceSetting).NumBytesReadSoFarL(aValue);
			ptr.Zero();
			ptr.Format(KNumberFormatString, retValue);
	
			//Perform synchronous write to EP0
			//This allows the subsequent 'Read' request to
			//take place
			TInt ret = iEp0Writer->WriteSynchronous(*iAuxBuffer, ETrue);
			OstTrace1(TRACE_NORMAL, CINTERFACEBASE_PROCESSREQUESTL_DUP08, "Write (from interface callback) executed with error %d", ret);
			}
			break;
			
		case KVendorGetRecordedNumBytesWrittenInPayloadRequest:
			{
			delete iAuxBuffer;
			iAuxBuffer = HBufC8::NewL(KNumberStringLength);
			TPtr8 ptr(iAuxBuffer->Des());
			TInt retValue = 0;
			retValue = AlternateSetting(iCurrentAlternateInterfaceSetting).NumBytesWrittenSoFarL(aValue);
			ptr.Zero();
			ptr.Format(KNumberFormatString, retValue);
	
			//Perform synchronous write to EP0
			//This allows the subsequent 'Read' request to
			//take place
			TInt ret = iEp0Writer->WriteSynchronous(*iAuxBuffer, ETrue);
			OstTrace1(TRACE_NORMAL, CINTERFACEBASE_PROCESSREQUESTL_DUP09, "Write (from interface callback) executed with error %d", ret);
			}
			break;
			
		case KVendorWriteToEndpointRequest:
			// Acknowledge the request
			iEp0Reader->Acknowledge();
			
			OstTraceExt2(TRACE_NORMAL, CINTERFACEBASE_PROCESSREQUESTL_DUP10, "Writing %d bytes to IN endpoint (index %d)",aPayload.Length(),aValue);
			
			// Write the data supplied by the host, back to the host though the specified endpoint
			
			AlternateSetting(iCurrentAlternateInterfaceSetting).WriteSpecifiedDataToEndpointL(aPayload,aValue);
			break;
			
		case KVendorCancelWriteToEndpointRequest:
			// Acknowledge the request
			iEp0Reader->Acknowledge();
			
			OstTrace1(TRACE_NORMAL, CINTERFACEBASE_PROCESSREQUESTL_DUP11, "CANCEL Writing to IN endpoint (index %d)",aValue);
			
			// CANCEL writing the data supplied by the host, back to the host though the specified endpoint
			
			AlternateSetting(iCurrentAlternateInterfaceSetting).CancelWriteDataToEndpointL(aValue);
			break;
			
		case KVendorPatternWriteToEndpointRequest:
			{
			// Acknowledge the request
			iEp0Reader->Acknowledge();
			
			// Read the number of repeats and the data supplied by the host, on the specified endpoint
			TUint32 numBytes = ExtractNumberL(aPayload);
			
			// Write the data supplied by the host, back to the host though the specified endpoint
			AlternateSetting(iCurrentAlternateInterfaceSetting).WriteSpecifiedDataToEndpointL(aPayload.Mid(KNumberStringLength),numBytes,aValue);
			}
			break;
			
		case KVendorPatternWriteSynchronousToEndpointRequest:
			{
			// Acknowledge the request
			iEp0Reader->Acknowledge();
			
			// Read the number of repeats and the data supplied by the host, on the specified endpoint
			TUint32 numBytes = ExtractNumberL(aPayload);
			
			// Write the data supplied by the host, back to the host though the specified endpoint
			AlternateSetting(iCurrentAlternateInterfaceSetting).WriteSynchronousSpecifiedDataToEndpointL(aPayload.Mid(KNumberStringLength),numBytes,aValue);
			}
			break;
			
		case KVendorPatternWriteSynchronousToAndHaltEndpointRequest:
			{
			// Acknowledge the request
			iEp0Reader->Acknowledge();
			
			// Read the number of repeats and the data supplied by the host, on the specified endpoint
			TUint32 numBytes = ExtractNumberL(aPayload);
			
			// Write the data supplied by the host, back to the host though the specified endpoint
			AlternateSetting(iCurrentAlternateInterfaceSetting).WriteSynchronousSpecifiedDataToAndHaltEndpointL(aPayload.Mid(KNumberStringLength),numBytes,aValue);
			}
			break;
			
		case KVendorRepeatedReadAndValidateDataRequest:
			{
			// Acknowledge the request
			iEp0Reader->Acknowledge();
			
			// Read the number of bytes to read in total and per individual 'Read' together with the data supplied by the host, on the specified endpoint
			TUint32 numBytesPerRead = 0;
			TUint32 totalNumBytes = 0;
			ExtractTwoNumbersL(aPayload, numBytesPerRead, totalNumBytes);
			OstTraceExt2(TRACE_NORMAL, CINTERFACEBASE_PROCESSREQUESTL_DUP12, "Extracted: Number of Bytes per Read = %d, Total Number of Bytes = %d",numBytesPerRead,totalNumBytes);
			
			// Write the data supplied by the host, back to the host though the specified endpoint
			AlternateSetting(iCurrentAlternateInterfaceSetting).RepeatedReadAndValidateFromEndpointL(aPayload.Mid(KTwoNumberStringLength),numBytesPerRead,totalNumBytes,aValue);
			}
			break;
			
		case KVendorRepeatedPatternWriteDataRequest:
			{
			// Acknowledge the request
			iEp0Reader->Acknowledge();
			
			// Read the number of bytes to write in total and per individual 'Write' together with the data supplied by the host, on the specified endpoint
			TUint32 numBytesPerWrite = 0;
			TUint32 totalNumBytes = 0;
			ExtractTwoNumbersL(aPayload, numBytesPerWrite, totalNumBytes);
			OstTraceExt2(TRACE_NORMAL, CINTERFACEBASE_PROCESSREQUESTL_DUP13, "Extracted: Number of Bytes per Read = %d, Total Number of Bytes = %d",numBytesPerWrite,totalNumBytes);
			
			// Write the data supplied by the host, back to the host though the specified endpoint
			AlternateSetting(iCurrentAlternateInterfaceSetting).RepeatedWriteSpecifiedDataToEndpointL(aPayload.Mid(KTwoNumberStringLength),numBytesPerWrite,totalNumBytes,aValue);
			}
			break;
			
		case KVendorWriteCachedReadRequest:
			{
			// Acknowledge the request
			iEp0Reader->Acknowledge();
			
			TUint16 readEndpoint = aValue >> 8; //HI 8 buts
			TUint16 writeEndpoint = aValue & 0x00ff; //LO 8 bits
			
			OstTraceExt2(TRACE_NORMAL, CINTERFACEBASE_PROCESSREQUESTL_DUP14, "Writing data cached on OUT endpoint (index %d) to IN endpoint (index %d)",readEndpoint,writeEndpoint);
			
			// Write the data supplied by the host, back to the host though the specified endpoint
			
			AlternateSetting(iCurrentAlternateInterfaceSetting).WriteCachedEndpointDataToEndpointL(readEndpoint,writeEndpoint);
			}
			break;
			
		case KVendorWriteSynchronousCachedReadRequest:
			{
			// Acknowledge the request
			iEp0Reader->Acknowledge();
			
			TUint16 readEndpoint = aValue >> 8; //HI 8 buts
			TUint16 writeEndpoint = aValue & 0x00ff; //LO 8 bits
			
			OstTraceExt2(TRACE_NORMAL, CINTERFACEBASE_PROCESSREQUESTL_DUP15, "Writing data cached on OUT endpoint (index %d) to IN endpoint (index %d)",readEndpoint,writeEndpoint);
			
			// Write the data supplied by the host, back to the host though the specified endpoint
			
			AlternateSetting(iCurrentAlternateInterfaceSetting).WriteSynchronousCachedEndpointDataToEndpointL(readEndpoint,writeEndpoint);
			}
			break;
			
		case KVendorSplitWriteSynchronousCachedReadRequest:
			{
			// Acknowledge the request
			iEp0Reader->Acknowledge();
			
			TUint16 readEndpoint = aValue >> 8; //HI 8 buts
			TUint16 writeEndpoint = aValue & 0x00ff; //LO 8 bits
			OstTraceExt2(TRACE_NORMAL, CINTERFACEBASE_PROCESSREQUESTL_DUP16, "Writing data cached on OUT endpoint (index %d) to IN endpoint (index %d) in sections of....",readEndpoint,writeEndpoint);
			
			// Read the number of bytes to use for each Write
			TUint numBytes[KNumSplitWriteSections];
			TUint numBytesWritten = 0;
			for(TUint i=0; i<KNumSplitWriteSections; ++i)
				{
				TLex8 lex(aPayload.Mid(i*KNumberStringLength, KNumberStringLength));
				User::LeaveIfError(lex.Val(numBytes[i], EDecimal));
				OstTrace1(TRACE_NORMAL, CINTERFACEBASE_PROCESSREQUESTL_DUP17, "%d bytes", numBytes[i]);
				// Write the data supplied by the host, back to the host though the specified endpoint
				AlternateSetting(iCurrentAlternateInterfaceSetting).WriteSynchronousCachedEndpointDataToEndpointL(readEndpoint,writeEndpoint,numBytesWritten,numBytes[i]);
				// Updates bytes written for next round of 'for'loop
				numBytesWritten += numBytes[i];
				}
			}
			break;
			
		case KVendorReadFromEndpointRequest:
			{
			// Acknowledge the request
			iEp0Reader->Acknowledge();
			
			// Read the amount of data supplied by the host, on the specified endpoint
			TLex8 lex(aPayload);
			TUint32 numBytes;
			User::LeaveIfError(lex.Val(numBytes, EDecimal));
			OstTraceExt2(TRACE_NORMAL, CINTERFACEBASE_PROCESSREQUESTL_DUP18, "Reading %u bytes on OUT endpoint (index %u)",numBytes,(TUint32)aValue);
			AlternateSetting(iCurrentAlternateInterfaceSetting).ReadDataFromEndpointL(numBytes,aValue);
			}
			break;
			
		case KVendorReadFromAndHaltEndpointRequest:
			{
			// Acknowledge the request
			iEp0Reader->Acknowledge();
			
			// Read the amount of data supplied by the host, on the specified endpoint
			TLex8 lex(aPayload);
			TUint32 numBytes;
			User::LeaveIfError(lex.Val(numBytes, EDecimal));
			OstTraceExt2(TRACE_NORMAL, CINTERFACEBASE_PROCESSREQUESTL_DUP19, "Reading %u bytes on OUT endpoint (index %u) ... then halting endpoint",numBytes,(TUint32)aValue);
			AlternateSetting(iCurrentAlternateInterfaceSetting).ReadDataFromAndHaltEndpointL(numBytes,aValue);
			}
			break;
			
		case KVendorCancelAnyReadFromEndpointRequest:
			{
			// Acknowledge the request
			iEp0Reader->Acknowledge();
			
			OstTrace1(TRACE_NORMAL, CINTERFACEBASE_PROCESSREQUESTL_DUP20, "CANCEL Reading on OUT endpoint (index %u)",(TUint32)aValue);
			AlternateSetting(iCurrentAlternateInterfaceSetting).CancelAnyReadDataFromEndpointL(aValue);
			}
			break;
			
		case KVendorReadUntilShortFromEndpointRequest:
			{
			// Acknowledge the request
			iEp0Reader->Acknowledge();
			
			// Read the amount of data supplied by the host, on the specified endpoint
			TLex8 lex(aPayload);
			TUint32 numBytes;
			User::LeaveIfError(lex.Val(numBytes, EDecimal));
			OstTraceExt2(TRACE_NORMAL, CINTERFACEBASE_PROCESSREQUESTL_DUP21, "Reading %u bytes on OUT endpoint (index %u)",numBytes,(TUint32)aValue);
			AlternateSetting(iCurrentAlternateInterfaceSetting).ReadDataUntilShortFromEndpointL(numBytes,aValue);
			}
			break;
			
		case KVendorStringValidationRequest:
			{
			// Acknowledge the request
			iEp0Reader->Acknowledge();
			
			// Read the number of repeats and the data supplied by the host, on the specified endpoint
			TLex8 lex(aPayload.Left(KNumberStringLength));
			OstTrace1(TRACE_NORMAL, CINTERFACEBASE_PROCESSREQUESTL_DUP22, "NUMBER STRING LENGTH CALCULATED AS %d",KNumberStringLength);
			TUint32 numBytes;
			User::LeaveIfError(lex.Val(numBytes, EDecimal));
			OstTrace0(TRACE_NORMAL, CINTERFACEBASE_PROCESSREQUESTL_DUP23, "Validation");
			OstTraceExt2(TRACE_NORMAL, CINTERFACEBASE_PROCESSREQUESTL_DUP24, "Checking %u bytes using string pattern below exist in the buffer for endpoint %u",numBytes,(TUint32)aValue);
            const TPtrC8& midPayload = aPayload.Mid(KNumberStringLength);
            OstTraceData(TRACE_NORMAL, CINTERFACEBASE_PROCESSREQUESTL_DUP54, "", midPayload.Ptr(), midPayload.Length());
			
			delete iAuxBuffer;
			iAuxBuffer = HBufC8::NewL(KPassFailStringLength);
			TPtr8 ptr(iAuxBuffer->Des());
			if(AlternateSetting(iCurrentAlternateInterfaceSetting).ValidateCachedEndpointDataL(aPayload.Mid(KNumberStringLength),numBytes,aValue))
				{
				ptr.Copy(KClientPassString);
				}
			else
				{
				ptr.Copy(KClientFailString);
				}
			}
			break;
			
		case KVendorRecordedValidationResultRequest:
			{
			// Acknowledge the request
			iEp0Reader->Acknowledge();
			
			delete iAuxBuffer;
			iAuxBuffer = HBufC8::NewL(KPassFailStringLength);
			TPtr8 ptr(iAuxBuffer->Des());
			if(AlternateSetting(iCurrentAlternateInterfaceSetting).CachedEndpointResultL(aValue))
				{
				ptr.Copy(KClientPassString);
				}
			else
				{
				ptr.Copy(KClientFailString);
				}
			}
			break;
			
		case KVendorUnrespondRequest:
			// Do not acknowledge this request
			
			OstTrace0(TRACE_NORMAL, CINTERFACEBASE_PROCESSREQUESTL_DUP25, "Unrespond request: continually NAK the host");
			break;
			
		case KVendorStallRequest:
			{
			// Stall the specified endpoint		
			iEp0Reader->Acknowledge();
			OstTrace1(TRACE_NORMAL, CINTERFACEBASE_PROCESSREQUESTL_DUP26, "Stalling endpoint%d",aValue);
			TInt err = StallEndpoint(aValue);
			if(err != KErrNone)
				{
				OstTraceExt2(TRACE_NORMAL, CINTERFACEBASE_PROCESSREQUESTL_DUP27, "<Error %d> unable to stall endpoint index %d",err,aValue);
				iDevice.ReportError(err);
				}
			}

		default:
			// Maybe forward to derived classes
			break;
		}

	OstTraceFunctionExitExt( CINTERFACEBASE_PROCESSREQUESTL_EXIT, this, KErrNone );
	return KErrNone;
	}
	

void CInterfaceBase::StartEp0Reading()
	{
	OstTraceFunctionEntry1( CINTERFACEBASE_STARTEP0READING_ENTRY, this );
	
	iEp0Reader->ReadRequestsL();
	OstTraceFunctionExit1( CINTERFACEBASE_STARTEP0READING_EXIT, this );
	}
	

void CInterfaceBase::StopEp0Reading()
	{
	OstTraceFunctionEntry1( CINTERFACEBASE_STOPEP0READING_ENTRY, this );
	
	iEp0Reader->Cancel();		
	OstTraceFunctionExit1( CINTERFACEBASE_STOPEP0READING_EXIT, this );
	}

	}


