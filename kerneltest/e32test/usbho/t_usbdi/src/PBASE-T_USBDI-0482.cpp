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
// @file PBASE-T_USBDI-0482.cpp
// @internalComponent
// 
//

#include "PBASE-T_USBDI-0482.h"
#include "tada22.h"
#include <d32usbdescriptors.h>


namespace NUnitTesting_USBDI
	{
	
_LIT(KTestCaseId,"PBASE-T_USBDI-0482");
const TFunctorTestCase<CUT_PBASE_T_USBDI_0482,TBool> CUT_PBASE_T_USBDI_0482::iFunctor(KTestCaseId);	
_LIT8(KHighRecordSamplingRate, "\x80\xBB\x00");	// sampling rate : 48,000 Hz
_LIT8(KLowSamplingRate		 , "\x22\x56\x00"); // sampling rate : 22,050 Hz

_LIT(KTrackFileName, "Z:\\scripts\\track.dat");

const TInt KRecordedDataId 			  = 0x12345;
const TUint KRecordedPacketsExpected  = 1000;
const TUint KAudioCDQualityFrequency  = 176; // 176 kB/s
const TUint KMaxPacketSizeOutExpected = 192;
const TUint KMaxPacketSizeInExpected  = 96;
const TUint KMaxPacketSizeOffset  	  = 4;
const TUint KIsochInMaxNumPackets  	  = 2000;
const TUint KChunkSize 				  = 500000; // 0.5 Mb


TInt CUT_PBASE_T_USBDI_0482::iExpectedTransferId = 0;

CUT_PBASE_T_USBDI_0482* CUT_PBASE_T_USBDI_0482::NewL(TBool aHostRole)
	{
	CUT_PBASE_T_USBDI_0482* self = new (ELeave) CUT_PBASE_T_USBDI_0482(aHostRole);
	CleanupStack::PushL(self);
	self->ConstructL();
	CleanupStack::Pop(self);
	return self;
	}
	

CUT_PBASE_T_USBDI_0482::CUT_PBASE_T_USBDI_0482(TBool aHostRole)
:	CBaseTestCase(KTestCaseId,aHostRole),
	iCaseStep(EInProgress)
	{
	iEndpointAddressIn = 0x84;
	iEndpointAddressOut = 0x01;
	iOutTransferBuf.CreateL(KChunkSize);
	} 


void CUT_PBASE_T_USBDI_0482::ConstructL()
	{
	BaseConstructL();
	}


CUT_PBASE_T_USBDI_0482::~CUT_PBASE_T_USBDI_0482()
	{
	LOG_FUNC
	
	Cancel();
		
	// Close pipes before interfaces	
	iPipeOut.Close();
	iPipeIn.Close();
	
	iUsbInterface0.Close();
	iUsbInterface1.Close();
	iUsbInterface2.Close();

	// Destroy transfers
	iOutTransfers.ResetAndDestroy();
	
	iOutTransferBuf.Close();
	iDataPolledBuf.Close();
	
	delete iIsochInTransfer;
	delete iControlEp0;
	delete iActorFDF;
	}
	
void CUT_PBASE_T_USBDI_0482::ExecuteHostTestCaseL()	
	{
	LOG_FUNC
	
	iActorFDF = CActorFDF::NewL(*this);
	iControlEp0 = new (ELeave) CEp0Transfer(iUsbInterface0);
	iActorFDF->Monitor();
	
	// Wait for the usb headset to be connected	
	TimeoutIn(30);
	}

		
void CUT_PBASE_T_USBDI_0482::HostDoCancel()
	{
	LOG_FUNC	
	}	
	

void CUT_PBASE_T_USBDI_0482::ExecuteDeviceTestCaseL()	
	{
	LOG_FUNC
	}
		
	
void CUT_PBASE_T_USBDI_0482::DeviceDoCancel()
	{
	LOG_FUNC
	}
	
	
void CUT_PBASE_T_USBDI_0482::DeviceStateChangeL(RUsbDevice::TDeviceState aPreviousState,RUsbDevice::TDeviceState aNewState,
		TInt aCompletionCode)
	{
	LOG_FUNC	
	}
	

TInt CUT_PBASE_T_USBDI_0482::FindOUTIsochronousEndpoint(TUsbGenericDescriptor*& aDescriptor)
	{
	LOG_FUNC	
	aDescriptor = NULL;

	TUsbInterfaceDescriptor alternate;
	TInt err = iUsbInterface1.GetAlternateInterfaceDescriptor(1, alternate);
	if(err)
		{
		RDebug::Printf("iUsbInterface1.GetAlternateInterfaceDescriptor error = %d",err);
		return err;
		} 

	TUsbGenericDescriptor* descriptor = alternate.iFirstChild;
	while(descriptor)
		{
		RDebug::Printf("ibDescriptorType = %d", descriptor->ibDescriptorType);
		if(descriptor->ibDescriptorType == EEndpoint)
			{
			aDescriptor = descriptor;
			RDebug::Printf("found descriptor return KErrNone");
			return KErrNone;
			}	
		descriptor = descriptor->iNextPeer;
		}

	return KErrNotFound;	
	}

	
TInt CUT_PBASE_T_USBDI_0482::FindINIsochronousEndpoint(TUsbGenericDescriptor*& aDescriptor)
	{
	LOG_FUNC	
	aDescriptor = NULL;

	TUsbInterfaceDescriptor alternate;
	TInt err = iUsbInterface2.GetAlternateInterfaceDescriptor(1, alternate);
	if(err)
		{
		RDebug::Printf("iUsbInterface2.GetAlternateInterfaceDescriptor error = %d",err);
		return err;
		} 

	TUsbGenericDescriptor* descriptor = alternate.iFirstChild;
	while(descriptor)
		{
		RDebug::Printf("ibDescriptorType = %d", descriptor->ibDescriptorType);
		if(descriptor->ibDescriptorType == EEndpoint)
			{
			aDescriptor = descriptor;
			RDebug::Printf("found descriptor return KErrNone");
			return KErrNone;
			}
		descriptor = descriptor->iNextPeer;
		}

	return KErrNotFound;	
	}	
	
		
	
void CUT_PBASE_T_USBDI_0482::DeviceInsertedL(TUint aDeviceHandle)
	{
	LOG_FUNC
	
	Cancel();
	CUsbTestDevice& testDevice = iActorFDF->DeviceL(aDeviceHandle);
	
	CHECK(CheckTreeAfterDeviceInsertion(testDevice, _L("RLogitechHeadSet")) == KErrNone);
	
	TUint32 token0, token1, token2;
	CHECK(testDevice.Device().GetTokenForInterface(0,token0) == KErrNone);

	CHECK(iUsbInterface0.Open(token0) == KErrNone); // Default interface setting 0
	
	
	// open interface 1, alt. sett. 1
	CHECK(testDevice.Device().GetTokenForInterface(1,token1) == KErrNone);
	
	CHECK(iUsbInterface1.Open(token1) == KErrNone); // Default interface setting 0
	
	RDebug::Printf("Selecting alternate interface setting 1");		
	CHECK(iUsbInterface1.SelectAlternateInterface(1) == KErrNone);

	
	// open interface 2, alt. sett. 1
	CHECK(testDevice.Device().GetTokenForInterface(2,token2) == KErrNone);

	CHECK(iUsbInterface2.Open(token2) == KErrNone); // Default interface setting 0

	
	RDebug::Printf("Selecting alternate interface setting 1");		
	CHECK(iUsbInterface2.SelectAlternateInterface(1) == KErrNone);

																
	iCaseStep = EWaitEndOfMusicTrack;
												
	// Open a pipe for endpoint (Isoch out)		
	RDebug::Printf("Finding address for an out isoch endpoint on interface 1 setting 1");
	// Isochronous OUT	
	TUsbGenericDescriptor* isochEpDescriptorOut;
	CHECK(KErrNone == FindOUTIsochronousEndpoint(isochEpDescriptorOut));
	
	// Get the maximum packet size for this isochronous endpoint
	TUint16 wMaxPacketSizeOut = isochEpDescriptorOut->TUint16At(KMaxPacketSizeOffset);	
	RDebug::Printf("wMaxPacketSizeOut = %d", wMaxPacketSizeOut);
	CHECK(wMaxPacketSizeOut == KMaxPacketSizeOutExpected); 
	
	// Isochronous IN		
	TUsbGenericDescriptor* isochEpDescriptorIn;
	CHECK(KErrNone == FindINIsochronousEndpoint(isochEpDescriptorIn));
	
	// Get the maximum packet size for this isochronous endpoint
	TUint16 wMaxPacketSizeIn = isochEpDescriptorIn->TUint16At(KMaxPacketSizeOffset);	
	RDebug::Printf("wMaxPacketSizeIn = %d", wMaxPacketSizeIn);
	CHECK(wMaxPacketSizeIn == KMaxPacketSizeInExpected); 
		
	
	// open pipes now.
	// Out
	RDebug::Printf("Opening a pipe to %08x",iEndpointAddressOut);
	CHECK(iUsbInterface1.OpenPipeForEndpoint(iPipeOut,iEndpointAddressOut,EFalse) == KErrNone);
		
	RDebug::Printf("Opened pipe to endpoint address %08x for isochronous transfer to device",iEndpointAddressOut);
				
	// In
	RDebug::Printf("Opening a pipe to %08x",iEndpointAddressIn);		
	CHECK(iUsbInterface2.OpenPipeForEndpoint(iPipeIn,iEndpointAddressIn,EFalse) == KErrNone);
		
	RDebug::Printf("Opened pipe to endpoint address %08x for isochronous transfer to device",iEndpointAddressIn);
	
	// SET_CUR class specific command
	TSetCurRequest request(KHighRecordSamplingRate ,iEndpointAddressIn);
	iControlEp0->SendRequest(request,this);
	
	// Que transfers now		
	// Transfer IN	
	 iIsochInTransfer = new (ELeave) CIsochTransfer(iPipeIn,iUsbInterface2,wMaxPacketSizeIn,
					KIsochInMaxNumPackets ,*this, KRecordedDataId);
	
	CHECK(iIsochInTransfer->RegisterTransferDescriptor() == KErrNone);
			
	// Initialise the descriptors for transfer	
	RDebug::Printf("Initialising the transfer descriptors interface 2"); 
	CHECK(iUsbInterface2.InitialiseTransferDescriptors() == KErrNone);

				
	// que interrupt transfer
	CHECK(iIsochInTransfer->TransferInL(KRecordedPacketsExpected) == KErrNone);
						
		
	// Transfer OUT
	RFs iFs;
	TInt ret = KErrNone; 
	ret = iFs.Connect();
	CHECK(ret==KErrNone || ret==KErrAlreadyExists);
	RFile trackFile;
	CHECK(trackFile.Open(iFs,KTrackFileName,EFileShareAny|EFileRead) == KErrNone);
		

	TInt trackFileSize;	
	trackFile.Size(trackFileSize); 
	
		
	wMaxPacketSizeOut = KAudioCDQualityFrequency;  	
	
	TInt nbChunks = trackFileSize/KChunkSize;
	TInt remainderSize = trackFileSize%KChunkSize; 
	
	if(remainderSize != 0)
		{
		nbChunks++;
		}
	
	TInt iChunk = 0;   
	TInt size = KChunkSize;
	              
	// create Transfers
	for(iChunk = 0; iChunk < nbChunks; iChunk++)
		{
		RDebug::Printf("iChunk = %d", iChunk);
		
		// remainder(last loop)
		if(remainderSize != 0 && (iChunk == nbChunks-1))
			{			
			size = remainderSize;
			}	
		CIsochTransfer* transfer = new (ELeave) CIsochTransfer(iPipeOut,iUsbInterface1,wMaxPacketSizeOut,
					(size/wMaxPacketSizeOut),*this, iChunk);
		CHECK(transfer->RegisterTransferDescriptor() == KErrNone);					
		iOutTransfers.AppendL(transfer);		
		iOutTransferBuf.Zero();    							
		}			
	
	// Initialise the descriptors for transfer	
	RDebug::Printf("Initialising the transfer descriptors"); 
	CHECK(iUsbInterface1.InitialiseTransferDescriptors() == KErrNone);
	
	// prepare & send transfers(TODO streaming algorithm with 3 Transfers only, filling 2 while the 3rd is transferring his data)
	size = KChunkSize;
	for(TInt iTransfers = 0; iTransfers < iOutTransfers.Count(); iTransfers++)
		{	
		RDebug::Printf("iTransfers = %d", iTransfers);	
		// remainder(last loop)
		if(remainderSize != 0 && (iTransfers == iOutTransfers.Count()-1))
			{
			RDebug::Printf("remainderSize = %d", remainderSize);
			size = remainderSize;
			}			
		CHECK(trackFile.Read(KChunkSize*iTransfers, iOutTransferBuf, size) == KErrNone);
		CHECK(iOutTransfers[iTransfers]->PrepareTransfer(iOutTransferBuf) == KErrNone);		
		CHECK(iOutTransfers[iTransfers]->TransferOut() == KErrNone);
		iOutTransferBuf.Zero();		
		}												
	}
	
TBool CUT_PBASE_T_USBDI_0482::ReplayRecordedData()
	{
	LOG_FUNC
		
	iOutTransferBuf.Zero();	
	
	// Transfer OUT						
	TInt nbChunks = iDataPolledBuf.Length()/KChunkSize;
	TInt remainderSize = iDataPolledBuf.Length()%KChunkSize; 
	RDebug::Printf("nbChunks = %d", nbChunks);
	RDebug::Printf("remainderSize = %d", remainderSize);
		
	if(remainderSize != 0) 
		{
		nbChunks++;
		}
	
	// prepare transfers	
	for(TInt iTransfers = 0; iTransfers < nbChunks; iTransfers++)
		{	
		RDebug::Printf("iTransfers = %d", iTransfers);	
		// remainder(last loop)
		if(remainderSize != 0 && (iTransfers == nbChunks-1))
			{
			RDebug::Printf("remainderSize = %d", remainderSize);
			}		
		CHECK_RET_BOOL(iOutTransfers[iTransfers]->PrepareTransfer(iDataPolledBuf) == KErrNone); // TODO retrieve relevant part of iDataPolledBuf if several chunks
		CHECK_RET_BOOL(iOutTransfers[iTransfers]->TransferOut() == KErrNone);
		iOutTransferBuf.Zero();	
		}
	return ETrue;			
	}
		
void CUT_PBASE_T_USBDI_0482::TransferCompleteL(TInt aTransferId,TInt aCompletionCode)
	{
	LOG_FUNC
	RDebug::Printf("Transfer completed (id=%d), aCompletionCode = %d",aTransferId, aCompletionCode);
	Cancel();

	if(aCompletionCode != KErrNone)
		{
		RDebug::Printf("<Error %d> Transfer %d not successful",aCompletionCode,aTransferId);
		TestFailed(aCompletionCode);
		return;
		}
	
	if(aTransferId == KRecordedDataId)
		{
		// data successfully recorded
		// 1. save data recorded
		CHECK(iIsochInTransfer->DataPolled(KRecordedPacketsExpected, iDataPolledBuf));
		// 2. waiting now for the end of the music track
		return;	
		}
				
	switch(iCaseStep)
		{
		case EWaitEndOfMusicTrack:
			{
			if(aTransferId != iExpectedTransferId)
				{
				RDebug::Printf("unexpected transfer!");
				TestFailed(KErrCorrupt);
				}
			iExpectedTransferId++;		
			// is it the last transfer?
			if(iExpectedTransferId == iOutTransfers.Count())
				{
				RDebug::Printf("last transfer successful! lets replay recorded data");
						
				iCaseStep = EReplayRecordedData; // assuming that recording is finished
				TSetCurRequest request(KLowSamplingRate ,iEndpointAddressOut);
				iControlEp0->SendRequest(request,this);		
				}			
			}		
			break;
								
		case EReplayRecordedData:
			{
			TestPassed();	// TODO only one transfer used in this case(few data recorded), cope with several ones	
			}			
			break;
				
		default:
			TestFailed(KErrDisconnected);
			break;
		}			
	}									   

void CUT_PBASE_T_USBDI_0482::Ep0TransferCompleteL(TInt aCompletionCode)
	{
	LOG_FUNC
	RDebug::Printf("Transfer EP0 completed aCompletionCode = %d", aCompletionCode);

	if(aCompletionCode != KErrNone)
		{
		TBuf<256> msg;
		msg.Format(_L("<Error %d> Transfer to control endpoint 0 was not successful"),aCompletionCode);
		RDebug::Print(msg);			
		return;			
		}	
	switch(iCaseStep)
		{	
		case EWaitEndOfMusicTrack:	
			break;	
			
		case EReplayRecordedData:	
			{
			CHECK(ReplayRecordedData());		
			}
			break;			
		
		default:
			RDebug::Printf("<Error> Unknown test step");
			TestFailed(KErrUnknown);
			break;
		}
	}	
	 	
	
void CUT_PBASE_T_USBDI_0482::DeviceRemovedL(TUint aDeviceHandle)
	{
	LOG_FUNC
	
	// Manual test over	
	switch(iCaseStep)
		{		
		case EFailed:
			TestFailed(KErrCompletion);
			break;
			
		case EInProgress:
		default:
			TestFailed(KErrDisconnected);
			break;
		}
	}
	
	
void CUT_PBASE_T_USBDI_0482::BusErrorL(TInt aError)
	{
	LOG_FUNC	
	// This test case handles no failiures on the bus	
	TestFailed(KErrCompletion);	
	}


void CUT_PBASE_T_USBDI_0482::HostRunL()
	{
	LOG_FUNC
	
	// Obtain the completion code
	TInt completionCode(iStatus.Int());
	
	if(completionCode == KErrNone)
		{
		// Action timeout
		RDebug::Printf("<Error> Action timeout");
		TestFailed(KErrTimedOut);
		}
	else
		{
		RDebug::Printf("<Error %d> Timeout timer could not complete",completionCode);
		TestFailed(completionCode);
		}
	}

void CUT_PBASE_T_USBDI_0482::DeviceRunL()
	{	
	}
		
	}
