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


namespace NUnitTesting_USBDI
	{

	
CInterfaceBase::CInterfaceBase(RUsbTestDevice& aTestDevice,const TDesC16& aName)
:	iDevice(aTestDevice),
	iInterfaceName(aName),
	iCurrentAlternateInterfaceSetting(0) // The default alternate interface setting will be zero when opened
	{
	}
	
CInterfaceBase::~CInterfaceBase()
	{
	LOG_FUNC
	
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
	}
	
	
void CInterfaceBase::BaseConstructL()
	{
	LOG_FUNC
	// Open channel to driver
	TInt err(iClientDriver.Open(0));
	if(err != KErrNone)
		{
		RDebug::Printf("<Error %d> Unable to open a channel to USB client driver",err);
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
	}


void CInterfaceBase::AddInterfaceSettingL(CInterfaceSettingBase* aInterfaceSetting)
	{
	LOG_FUNC
	
	// Append to the container
	TInt err(iAlternateSettings.Append(aInterfaceSetting));
	if(err != KErrNone)
		{
		RDebug::Printf("<Error %d> Unable to add interface setting",err);
		User::Leave(err);
		}
	
	// Get the current number of alternate interface settings	
	TInt alternateSettingNumber(iAlternateSettings.Count()-1);

	TUint endpointSettingCount(aInterfaceSetting->iInterfaceInfo().iTotalEndpointsUsed);
	
	if(endpointSettingCount > 0)
		{
		RDebug::Printf("%u endpoint(s) to configure for this interface setting",endpointSettingCount);
				
		// Device capabilities
		TUsbDeviceCaps devCaps;
		err = iClientDriver.DeviceCaps(devCaps);
		if(err != KErrNone)
			{
			RDebug::Printf("<Error %d> Unable to retrieve device capabilities",err);
			User::Leave(err);
			}

		// Endpoint capabilities
		TUsbcEndpointData endpointCaps[KUsbcMaxEndpoints];
		TPtr8 dataptr(reinterpret_cast<TUint8*>(endpointCaps), sizeof(endpointCaps), sizeof(endpointCaps));
		err = iClientDriver.EndpointCaps(dataptr);
		if(err != KErrNone)
			{
			RDebug::Printf("<Error %d> Unable to get endpoint capabilities",err);
			User::Leave(err);
			}		
		
		TInt totalEndpoints(devCaps().iTotalEndpoints);
		
		// Loop through available hardware endpoints to find suitable one
		// i.e. endpoints that can be configured
		
		TUint epIndex(0);
		TUint epCount(0);
		
		for(; epIndex<totalEndpoints; epIndex++)
			{
			RDebug::Printf("Examining hardware endpoint %u",epIndex);
			const TUsbcEndpointData ep = endpointCaps[epIndex];
			
			// Check the endpoint index to see if already claimed
			if(!ep.iInUse)
				{			
				RDebug::Printf("...its free");
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
						RDebug::Printf("Created endpoint writer for endpoint%d",epCount+1);
						}
					else if(endpointSpec.iDir == KUsbEpDirOut)
						{
						// Create an endpoint reader for this endpoint
												
						aInterfaceSetting->CreateEndpointReaderL(iClientDriver,epCount+1);
						RDebug::Printf("Created endpoint reader for endpoint%d",epCount+1);
						}					
					
					epCount++; // Increment to next endpoint spec
					RDebug::Printf("Endpoint %u configured",epCount);
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
				RDebug::Printf("...its busy");
				}
			}
		
		RDebug::Printf("Configure %u out of %u endpoints",epCount,endpointSettingCount);			
		
		if(epCount < endpointSettingCount)
			{
			RDebug::Printf("<Error %d> Only managed to configure %u out of %u endpoints",KErrNotFound,epCount,endpointSettingCount);
			User::Leave(KErrNotFound);
			}			
		}
	else
		{
		RDebug::Printf("No endpoints for this interface setting");
		}
	
	// Add the new setting to the device
	err = iClientDriver.SetInterface(alternateSettingNumber,aInterfaceSetting->iInterfaceInfo);
	if(err != KErrNone)
		{
		RDebug::Printf("<Error %d> Unable to set the alternate interface setting %d",err,alternateSettingNumber);
		User::Leave(err);
		}
	
	RDebug::Printf("Alternate interface setting %d set",alternateSettingNumber);
	}


TInt CInterfaceBase::StallEndpoint(TUint16 aEndpointNumber)
	{
	LOG_FUNC
	
	RDebug::Printf("Stalling endpoint%d",aEndpointNumber);
	return iClientDriver.HaltEndpoint(static_cast<TEndpointNumber>(aEndpointNumber));
	}

	
CInterfaceSettingBase& CInterfaceBase::AlternateSetting(TInt aSettingNumber) const
	{
	return *iAlternateSettings[aSettingNumber];
	}

	
TInt CInterfaceBase::InterfaceSettingCount() const
	{
	return iAlternateSettings.Count();
	}


TUint32 CInterfaceBase::ExtractNumberL(const TDesC8& aPayload)
	{
	LOG_FUNC

	// Read the number of repeats and the data supplied by the host, on the specified endpoint
	TLex8 lex(aPayload.Left(KNumberStringLength));
	TUint32 numBytes;
	User::LeaveIfError(lex.Val(numBytes, EDecimal));
	RDebug::Printf("Writing %d bytes using string pattern below to IN endpoint",numBytes);
	RDebug::RawPrint(aPayload.Mid(KNumberStringLength));
	RDebug::Printf(""); //new line
	return numBytes;
	}

void CInterfaceBase::ExtractTwoNumbersL(const TDesC8& aPayload, TUint32& aFirstNum, TUint32& aSecondNum)
	{
	LOG_FUNC

	// Read the number of repeats and the data supplied by the host, on the specified endpoint
	TLex8 lex1(aPayload.Left(KNumberStringLength));
	User::LeaveIfError(lex1.Val(aFirstNum, EDecimal));
	TLex8 lex2(aPayload.Mid(KNumberStringLength, KNumberStringLength));
	User::LeaveIfError(lex2.Val(aSecondNum, EDecimal));
	RDebug::Printf("Writing or Reading a total of %d bytes in repeats of %d bytes using string pattern below to IN endpoint",aFirstNum,aSecondNum);
	RDebug::RawPrint(aPayload.Mid(2*KNumberStringLength));
	RDebug::Printf(""); //new line
	return;
	}

void CInterfaceBase::AlternateInterfaceSelectedL(TInt aAlternateInterfaceSetting)
	{
	LOG_FUNC
	RDebug::Printf("Interface %S:",&iInterfaceName);	
	iCurrentAlternateInterfaceSetting = aAlternateInterfaceSetting;
	}


TInt CInterfaceBase::ProcessRequestL(TUint8 aRequest,TUint16 aValue,TUint16 aIndex,
	TUint16 aDataReqLength,const TDesC8& aPayload)
	{
	LOG_FUNC
	RDebug::Printf("Interface %S:",&iInterfaceName);
	
	switch(aRequest)
		{
		case KVendorEmptyRequest:
			// Acknowledge the request and do nothing
			iEp0Reader->Acknowledge();
			
			RDebug::Printf("Request: Empty");
			break;
			
		case KVendorPutPayloadRequest:
			// Acknowledge the request
			iEp0Reader->Acknowledge();
			
			RDebug::Printf("Put payload");
			if(aPayload.Compare(_L8("DEADBEEF")) != 0)
				{
				RDebug::Printf("<Error %d> Payload not as expected",KErrCorrupt);
				iDevice.ReportError(KErrCorrupt);
				}
			break;
			
		case KVendorGetPayloadRequest:
			{
			RDebug::Printf("Get payload");
			__ASSERT_DEBUG(iAuxBuffer, User::Panic(_L("Trying to write non-allocated buffer"), KErrGeneral));
			RDebug::Printf("iAuxBuffer = ....");
			RDebug::RawPrint(*iAuxBuffer);
			RDebug::Printf("\n");
			
			//Perform synchronous write to EP0
			//This allows the subsequent 'Read' request to
			//take place
			TInt ret = iEp0Writer->WriteSynchronous(*iAuxBuffer, ETrue);
			RDebug::Printf("Write (from interface callback) executed with error %d", ret);
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
			RDebug::Printf("Write (from interface callback) executed with error %d", ret);
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
			RDebug::Printf("Write (from interface callback) executed with error %d", ret);
			}
			break;
			
		case KVendorWriteToEndpointRequest:
			// Acknowledge the request
			iEp0Reader->Acknowledge();
			
			RDebug::Printf("Writing %d bytes to IN endpoint (index %d)",aPayload.Length(),aValue);
			
			// Write the data supplied by the host, back to the host though the specified endpoint
			
			AlternateSetting(iCurrentAlternateInterfaceSetting).WriteSpecifiedDataToEndpointL(aPayload,aValue);
			break;
			
		case KVendorCancelWriteToEndpointRequest:
			// Acknowledge the request
			iEp0Reader->Acknowledge();
			
			RDebug::Printf("CANCEL Writing to IN endpoint (index %d)",aValue);
			
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
			RDebug::Printf("Extracted: Number of Bytes per Read = %d, Total Number of Bytes = %d",numBytesPerRead,totalNumBytes);
			
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
			RDebug::Printf("Extracted: Number of Bytes per Read = %d, Total Number of Bytes = %d",numBytesPerWrite,totalNumBytes);
			
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
			
			RDebug::Printf("Writing data cached on OUT endpoint (index %d) to IN endpoint (index %d)",readEndpoint,writeEndpoint);
			
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
			
			RDebug::Printf("Writing data cached on OUT endpoint (index %d) to IN endpoint (index %d)",readEndpoint,writeEndpoint);
			
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
			RDebug::Printf("Writing data cached on OUT endpoint (index %d) to IN endpoint (index %d) in sections of....",readEndpoint,writeEndpoint);
			
			// Read the number of bytes to use for each Write
			TUint numBytes[KNumSplitWriteSections];
			TUint numBytesWritten = 0;
			for(TUint i=0; i<KNumSplitWriteSections; ++i)
				{
				TLex8 lex(aPayload.Mid(i*KNumberStringLength, KNumberStringLength));
				User::LeaveIfError(lex.Val(numBytes[i], EDecimal));
				RDebug::Printf("%d bytes", numBytes[i]);
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
			RDebug::Printf("Reading %d bytes on OUT endpoint (index %d)",numBytes,aValue);
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
			RDebug::Printf("Reading %d bytes on OUT endpoint (index %d) ... then halting endpoint",numBytes,aValue);
			AlternateSetting(iCurrentAlternateInterfaceSetting).ReadDataFromAndHaltEndpointL(numBytes,aValue);
			}
			break;
			
		case KVendorCancelAnyReadFromEndpointRequest:
			{
			// Acknowledge the request
			iEp0Reader->Acknowledge();
			
			RDebug::Printf("CANCEL Reading on OUT endpoint (index %d)",aValue);
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
			RDebug::Printf("Reading %d bytes on OUT endpoint (index %d)",numBytes,aValue);
			AlternateSetting(iCurrentAlternateInterfaceSetting).ReadDataUntilShortFromEndpointL(numBytes,aValue);
			}
			break;
			
		case KVendorStringValidationRequest:
			{
			// Acknowledge the request
			iEp0Reader->Acknowledge();
			
			// Read the number of repeats and the data supplied by the host, on the specified endpoint
			TLex8 lex(aPayload.Left(KNumberStringLength));
			RDebug::Printf("NUMBER STRING LENGTH CALCULATED AS %d",KNumberStringLength);
			TUint32 numBytes;
			User::LeaveIfError(lex.Val(numBytes, EDecimal));
			RDebug::Printf("Validation");
			RDebug::Printf("Checking %d bytes using string pattern below exist in the buffer for endpoint %d",numBytes,aValue);
			RDebug::RawPrint(aPayload.Mid(KNumberStringLength));
			
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
			
			RDebug::Printf("Unrespond request: continually NAK the host");
			break;
			
		case KVendorStallRequest:
			{
			// Stall the specified endpoint		
			iEp0Reader->Acknowledge();
			RDebug::Printf("Stalling endpoint%d",aValue);
			TInt err = StallEndpoint(aValue);
			if(err != KErrNone)
				{
				RDebug::Printf("<Error %d> unable to stall endpoint index %d",err,aValue);
				iDevice.ReportError(err);
				}
			}

		default:
			// Maybe forward to derived classes
			break;
		}

	return KErrNone;
	}
	

void CInterfaceBase::StartEp0Reading()
	{
	LOG_FUNC
	
	iEp0Reader->ReadRequestsL();
	}
	

void CInterfaceBase::StopEp0Reading()
	{
	LOG_FUNC
	
	iEp0Reader->Cancel();		
	}

	}


