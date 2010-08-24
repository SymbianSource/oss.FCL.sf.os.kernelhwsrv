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
// @file PBASE-T_USBDI-0482.cpp
// @internalComponent
// 
//

#include "PBASE-T_USBDI-0482.h"
#include "tada22.h"
#include "OstTraceDefinitions.h"
#ifdef OST_TRACE_COMPILER_IN_USE
#include "PBASE-T_USBDI-0482Traces.h"
#endif
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
	OstTraceFunctionEntry1( CUT_PBASE_T_USBDI_0482_NEWL_ENTRY, aHostRole );
	CUT_PBASE_T_USBDI_0482* self = new (ELeave) CUT_PBASE_T_USBDI_0482(aHostRole);
	CleanupStack::PushL(self);
	self->ConstructL();
	CleanupStack::Pop(self);
	OstTraceFunctionExit1( CUT_PBASE_T_USBDI_0482_NEWL_EXIT, ( TUint )( self ) );
	return self;
	}
	

CUT_PBASE_T_USBDI_0482::CUT_PBASE_T_USBDI_0482(TBool aHostRole)
:	CBaseTestCase(KTestCaseId,aHostRole),
	iCaseStep(EInProgress)
	{
	OstTraceFunctionEntryExt( CUT_PBASE_T_USBDI_0482_CUT_PBASE_T_USBDI_0482_ENTRY, this );
	iEndpointAddressIn = 0x84;
	iEndpointAddressOut = 0x01;
	iOutTransferBuf.CreateL(KChunkSize);
	OstTraceFunctionExit1( CUT_PBASE_T_USBDI_0482_CUT_PBASE_T_USBDI_0482_EXIT, this );
	} 


void CUT_PBASE_T_USBDI_0482::ConstructL()
	{
	OstTraceFunctionEntry1( CUT_PBASE_T_USBDI_0482_CONSTRUCTL_ENTRY, this );
	BaseConstructL();
	OstTraceFunctionExit1( CUT_PBASE_T_USBDI_0482_CONSTRUCTL_EXIT, this );
	}


CUT_PBASE_T_USBDI_0482::~CUT_PBASE_T_USBDI_0482()
	{
	OstTraceFunctionEntry1( CUT_PBASE_T_USBDI_0482_CUT_PBASE_T_USBDI_0482_ENTRY_DUP01, this );
	
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
	OstTraceFunctionExit1( CUT_PBASE_T_USBDI_0482_CUT_PBASE_T_USBDI_0482_EXIT_DUP01, this );
	}
	
void CUT_PBASE_T_USBDI_0482::ExecuteHostTestCaseL()	
	{
	OstTraceFunctionEntry1( CUT_PBASE_T_USBDI_0482_EXECUTEHOSTTESTCASEL_ENTRY, this );
	
	iActorFDF = CActorFDF::NewL(*this);
	iControlEp0 = new (ELeave) CEp0Transfer(iUsbInterface0);
	iActorFDF->Monitor();
	
	// Wait for the usb headset to be connected	
	TimeoutIn(30);
	OstTraceFunctionExit1( CUT_PBASE_T_USBDI_0482_EXECUTEHOSTTESTCASEL_EXIT, this );
	}

		
void CUT_PBASE_T_USBDI_0482::HostDoCancel()
	{
	OstTraceFunctionEntry1( CUT_PBASE_T_USBDI_0482_HOSTDOCANCEL_ENTRY, this );
	OstTraceFunctionExit1( CUT_PBASE_T_USBDI_0482_HOSTDOCANCEL_EXIT, this );
	}	
	

void CUT_PBASE_T_USBDI_0482::ExecuteDeviceTestCaseL()	
	{
	OstTraceFunctionEntry1( CUT_PBASE_T_USBDI_0482_EXECUTEDEVICETESTCASEL_ENTRY, this );
	OstTraceFunctionExit1( CUT_PBASE_T_USBDI_0482_EXECUTEDEVICETESTCASEL_EXIT, this );
	}
		
	
void CUT_PBASE_T_USBDI_0482::DeviceDoCancel()
	{
	OstTraceFunctionEntry1( CUT_PBASE_T_USBDI_0482_DEVICEDOCANCEL_ENTRY, this );
	OstTraceFunctionExit1( CUT_PBASE_T_USBDI_0482_DEVICEDOCANCEL_EXIT, this );
	}
	
	
void CUT_PBASE_T_USBDI_0482::DeviceStateChangeL(RUsbDevice::TDeviceState aPreviousState,RUsbDevice::TDeviceState aNewState,
		TInt aCompletionCode)
	{
	OstTraceFunctionEntryExt( CUT_PBASE_T_USBDI_0482_DEVICESTATECHANGEL_ENTRY, this );
	OstTraceFunctionExit1( CUT_PBASE_T_USBDI_0482_DEVICESTATECHANGEL_EXIT, this );
	}
	

TInt CUT_PBASE_T_USBDI_0482::FindOUTIsochronousEndpoint(TUsbGenericDescriptor*& aDescriptor)
	{
	OstTraceFunctionEntryExt( CUT_PBASE_T_USBDI_0482_FINDOUTISOCHRONOUSENDPOINT_ENTRY, this );
	aDescriptor = NULL;

	TUsbInterfaceDescriptor alternate;
	TInt err = iUsbInterface1.GetAlternateInterfaceDescriptor(1, alternate);
	if(err)
		{
		OstTrace1(TRACE_NORMAL, CUT_PBASE_T_USBDI_0482_FINDOUTISOCHRONOUSENDPOINT, "iUsbInterface1.GetAlternateInterfaceDescriptor error = %d",err);
		OstTraceFunctionExitExt( CUT_PBASE_T_USBDI_0482_FINDOUTISOCHRONOUSENDPOINT_EXIT, this, err );
		return err;
		} 

	TUsbGenericDescriptor* descriptor = alternate.iFirstChild;
	while(descriptor)
		{
		OstTrace1(TRACE_NORMAL, CUT_PBASE_T_USBDI_0482_FINDOUTISOCHRONOUSENDPOINT_DUP01, "ibDescriptorType = %d", descriptor->ibDescriptorType);
		if(descriptor->ibDescriptorType == EEndpoint)
			{
			aDescriptor = descriptor;
			OstTrace0(TRACE_NORMAL, CUT_PBASE_T_USBDI_0482_FINDOUTISOCHRONOUSENDPOINT_DUP02, "found descriptor return KErrNone");
			OstTraceFunctionExitExt( CUT_PBASE_T_USBDI_0482_FINDOUTISOCHRONOUSENDPOINT_EXIT_DUP01, this, KErrNone );
			return KErrNone;
			}	
		descriptor = descriptor->iNextPeer;
		}

	OstTraceFunctionExitExt( CUT_PBASE_T_USBDI_0482_FINDOUTISOCHRONOUSENDPOINT_EXIT_DUP02, this, KErrNotFound );
	return KErrNotFound;	
	}

	
TInt CUT_PBASE_T_USBDI_0482::FindINIsochronousEndpoint(TUsbGenericDescriptor*& aDescriptor)
	{
	OstTraceFunctionEntryExt( CUT_PBASE_T_USBDI_0482_FINDINISOCHRONOUSENDPOINT_ENTRY, this );
	aDescriptor = NULL;

	TUsbInterfaceDescriptor alternate;
	TInt err = iUsbInterface2.GetAlternateInterfaceDescriptor(1, alternate);
	if(err)
		{
		OstTrace1(TRACE_NORMAL, CUT_PBASE_T_USBDI_0482_FINDINISOCHRONOUSENDPOINT, "iUsbInterface2.GetAlternateInterfaceDescriptor error = %d",err);
		OstTraceFunctionExitExt( CUT_PBASE_T_USBDI_0482_FINDINISOCHRONOUSENDPOINT_EXIT, this, err );
		return err;
		} 

	TUsbGenericDescriptor* descriptor = alternate.iFirstChild;
	while(descriptor)
		{
		OstTrace1(TRACE_NORMAL, CUT_PBASE_T_USBDI_0482_FINDINISOCHRONOUSENDPOINT_DUP01, "ibDescriptorType = %d", descriptor->ibDescriptorType);
		if(descriptor->ibDescriptorType == EEndpoint)
			{
			aDescriptor = descriptor;
			OstTrace0(TRACE_NORMAL, CUT_PBASE_T_USBDI_0482_FINDINISOCHRONOUSENDPOINT_DUP02, "found descriptor return KErrNone");
			OstTraceFunctionExitExt( CUT_PBASE_T_USBDI_0482_FINDINISOCHRONOUSENDPOINT_EXIT_DUP01, this, KErrNone );
			return KErrNone;
			}
		descriptor = descriptor->iNextPeer;
		}

	OstTraceFunctionExitExt( CUT_PBASE_T_USBDI_0482_FINDINISOCHRONOUSENDPOINT_EXIT_DUP02, this, KErrNotFound );
	return KErrNotFound;	
	}	
	
		
	
void CUT_PBASE_T_USBDI_0482::DeviceInsertedL(TUint aDeviceHandle)
	{
	OstTraceFunctionEntryExt( CUT_PBASE_T_USBDI_0482_DEVICEINSERTEDL_ENTRY, this );
	
	Cancel();
	CUsbTestDevice& testDevice = iActorFDF->DeviceL(aDeviceHandle);
	
	CHECK(CheckTreeAfterDeviceInsertion(testDevice, _L("RLogitechHeadSet")) == KErrNone);
	
	TUint32 token0, token1, token2;
	CHECK(testDevice.Device().GetTokenForInterface(0,token0) == KErrNone);

	CHECK(iUsbInterface0.Open(token0) == KErrNone); // Default interface setting 0
	
	
	// open interface 1, alt. sett. 1
	CHECK(testDevice.Device().GetTokenForInterface(1,token1) == KErrNone);
	
	CHECK(iUsbInterface1.Open(token1) == KErrNone); // Default interface setting 0
	
	OstTrace0(TRACE_NORMAL, CUT_PBASE_T_USBDI_0482_DEVICEINSERTEDL, "Selecting alternate interface setting 1");
	CHECK(iUsbInterface1.SelectAlternateInterface(1) == KErrNone);

	
	// open interface 2, alt. sett. 1
	CHECK(testDevice.Device().GetTokenForInterface(2,token2) == KErrNone);

	CHECK(iUsbInterface2.Open(token2) == KErrNone); // Default interface setting 0

	
	OstTrace0(TRACE_NORMAL, CUT_PBASE_T_USBDI_0482_DEVICEINSERTEDL_DUP01, "Selecting alternate interface setting 1");
	CHECK(iUsbInterface2.SelectAlternateInterface(1) == KErrNone);

																
	iCaseStep = EWaitEndOfMusicTrack;
												
	// Open a pipe for endpoint (Isoch out)		
	OstTrace0(TRACE_NORMAL, CUT_PBASE_T_USBDI_0482_DEVICEINSERTEDL_DUP02, "Finding address for an out isoch endpoint on interface 1 setting 1");
	// Isochronous OUT	
	TUsbGenericDescriptor* isochEpDescriptorOut;
	CHECK(KErrNone == FindOUTIsochronousEndpoint(isochEpDescriptorOut));
	
	// Get the maximum packet size for this isochronous endpoint
	TUint16 wMaxPacketSizeOut = isochEpDescriptorOut->TUint16At(KMaxPacketSizeOffset);	
	OstTrace1(TRACE_NORMAL, CUT_PBASE_T_USBDI_0482_DEVICEINSERTEDL_DUP03, "wMaxPacketSizeOut = %d", wMaxPacketSizeOut);
	CHECK(wMaxPacketSizeOut == KMaxPacketSizeOutExpected); 
	
	// Isochronous IN		
	TUsbGenericDescriptor* isochEpDescriptorIn;
	CHECK(KErrNone == FindINIsochronousEndpoint(isochEpDescriptorIn));
	
	// Get the maximum packet size for this isochronous endpoint
	TUint16 wMaxPacketSizeIn = isochEpDescriptorIn->TUint16At(KMaxPacketSizeOffset);	
	OstTrace1(TRACE_NORMAL, CUT_PBASE_T_USBDI_0482_DEVICEINSERTEDL_DUP04, "wMaxPacketSizeIn = %d", wMaxPacketSizeIn);
	CHECK(wMaxPacketSizeIn == KMaxPacketSizeInExpected); 
		
	
	// open pipes now.
	// Out
	OstTrace1(TRACE_NORMAL, CUT_PBASE_T_USBDI_0482_DEVICEINSERTEDL_DUP05, "Opening a pipe to %08x",iEndpointAddressOut);
	CHECK(iUsbInterface1.OpenPipeForEndpoint(iPipeOut,iEndpointAddressOut,EFalse) == KErrNone);
		
	OstTrace1(TRACE_NORMAL, CUT_PBASE_T_USBDI_0482_DEVICEINSERTEDL_DUP06, "Opened pipe to endpoint address %08x for isochronous transfer to device",iEndpointAddressOut);
				
	// In
	OstTrace1(TRACE_NORMAL, CUT_PBASE_T_USBDI_0482_DEVICEINSERTEDL_DUP07, "Opening a pipe to %08x",iEndpointAddressIn);		
	CHECK(iUsbInterface2.OpenPipeForEndpoint(iPipeIn,iEndpointAddressIn,EFalse) == KErrNone);
		
	OstTrace1(TRACE_NORMAL, CUT_PBASE_T_USBDI_0482_DEVICEINSERTEDL_DUP08, "Opened pipe to endpoint address %08x for isochronous transfer to device",iEndpointAddressIn);
	
	// SET_CUR class specific command
	TSetCurRequest request(KHighRecordSamplingRate ,iEndpointAddressIn);
	iControlEp0->SendRequest(request,this);
	
	// Que transfers now		
	// Transfer IN	
	 iIsochInTransfer = new (ELeave) CIsochTransfer(iPipeIn,iUsbInterface2,wMaxPacketSizeIn,
					KIsochInMaxNumPackets ,*this, KRecordedDataId);
	
	CHECK(iIsochInTransfer->RegisterTransferDescriptor() == KErrNone);
			
	// Initialise the descriptors for transfer	
	OstTrace0(TRACE_NORMAL, CUT_PBASE_T_USBDI_0482_DEVICEINSERTEDL_DUP09, "Initialising the transfer descriptors interface 2");
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
		OstTrace1(TRACE_NORMAL, CUT_PBASE_T_USBDI_0482_DEVICEINSERTEDL_DUP10, "iChunk = %d", iChunk);
		
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
	OstTrace0(TRACE_NORMAL, CUT_PBASE_T_USBDI_0482_DEVICEINSERTEDL_DUP11, "Initialising the transfer descriptors");
	CHECK(iUsbInterface1.InitialiseTransferDescriptors() == KErrNone);
	
	// prepare & send transfers(TODO streaming algorithm with 3 Transfers only, filling 2 while the 3rd is transferring his data)
	size = KChunkSize;
	for(TInt iTransfers = 0; iTransfers < iOutTransfers.Count(); iTransfers++)
		{	
		OstTrace1(TRACE_NORMAL, CUT_PBASE_T_USBDI_0482_DEVICEINSERTEDL_DUP12, "iTransfers = %d", iTransfers);	
		// remainder(last loop)
		if(remainderSize != 0 && (iTransfers == iOutTransfers.Count()-1))
			{
			OstTrace1(TRACE_NORMAL, CUT_PBASE_T_USBDI_0482_DEVICEINSERTEDL_DUP13, "remainderSize = %d", remainderSize);
			size = remainderSize;
			}			
		CHECK(trackFile.Read(KChunkSize*iTransfers, iOutTransferBuf, size) == KErrNone);
		CHECK(iOutTransfers[iTransfers]->PrepareTransfer(iOutTransferBuf) == KErrNone);		
		CHECK(iOutTransfers[iTransfers]->TransferOut() == KErrNone);
		iOutTransferBuf.Zero();		
		}												
	OstTraceFunctionExit1( CUT_PBASE_T_USBDI_0482_DEVICEINSERTEDL_EXIT, this );
	}
	
TBool CUT_PBASE_T_USBDI_0482::ReplayRecordedData()
	{
		OstTraceFunctionEntry1( CUT_PBASE_T_USBDI_0482_REPLAYRECORDEDDATA_ENTRY, this );
		
	iOutTransferBuf.Zero();	
	
	// Transfer OUT						
	TInt nbChunks = iDataPolledBuf.Length()/KChunkSize;
	TInt remainderSize = iDataPolledBuf.Length()%KChunkSize; 
	OstTrace1(TRACE_NORMAL, CUT_PBASE_T_USBDI_0482_REPLAYRECORDEDDATA, "nbChunks = %d", nbChunks);
	OstTrace1(TRACE_NORMAL, CUT_PBASE_T_USBDI_0482_REPLAYRECORDEDDATA_DUP01, "remainderSize = %d", remainderSize);
		
	if(remainderSize != 0) 
		{
		nbChunks++;
		}
	
	// prepare transfers	
	for(TInt iTransfers = 0; iTransfers < nbChunks; iTransfers++)
		{	
		OstTrace1(TRACE_NORMAL, CUT_PBASE_T_USBDI_0482_REPLAYRECORDEDDATA_DUP02, "iTransfers = %d", iTransfers);	
		// remainder(last loop)
		if(remainderSize != 0 && (iTransfers == nbChunks-1))
			{
			OstTrace1(TRACE_NORMAL, CUT_PBASE_T_USBDI_0482_REPLAYRECORDEDDATA_DUP03, "remainderSize = %d", remainderSize);
			}		
		CHECK_RET_BOOL(iOutTransfers[iTransfers]->PrepareTransfer(iDataPolledBuf) == KErrNone); // TODO retrieve relevant part of iDataPolledBuf if several chunks
		CHECK_RET_BOOL(iOutTransfers[iTransfers]->TransferOut() == KErrNone);
		iOutTransferBuf.Zero();	
		}
	OstTraceFunctionExitExt( CUT_PBASE_T_USBDI_0482_REPLAYRECORDEDDATA_EXIT, this, ETrue );
	return ETrue;			
	}
		
void CUT_PBASE_T_USBDI_0482::TransferCompleteL(TInt aTransferId,TInt aCompletionCode)
	{
	OstTraceFunctionEntryExt( CUT_PBASE_T_USBDI_0482_TRANSFERCOMPLETEL_ENTRY, this );
	OstTraceExt2(TRACE_NORMAL, CUT_PBASE_T_USBDI_0482_TRANSFERCOMPLETEL, "Transfer completed (id=%d), aCompletionCode = %d",aTransferId, aCompletionCode);
	Cancel();

	if(aCompletionCode != KErrNone)
		{
		OstTraceExt2(TRACE_NORMAL, CUT_PBASE_T_USBDI_0482_TRANSFERCOMPLETEL_DUP01, "<Error %d> Transfer %d not successful",aCompletionCode,aTransferId);
		TestFailed(aCompletionCode);
		OstTraceFunctionExit1( CUT_PBASE_T_USBDI_0482_TRANSFERCOMPLETEL_EXIT, this );
		return;
		}
	
	if(aTransferId == KRecordedDataId)
		{
		// data successfully recorded
		// 1. save data recorded
		CHECK(iIsochInTransfer->DataPolled(KRecordedPacketsExpected, iDataPolledBuf));
		// 2. waiting now for the end of the music track
		OstTraceFunctionExit1( CUT_PBASE_T_USBDI_0482_TRANSFERCOMPLETEL_EXIT_DUP01, this );
		return;	
		}
				
	switch(iCaseStep)
		{
		case EWaitEndOfMusicTrack:
			{
			if(aTransferId != iExpectedTransferId)
				{
				OstTrace0(TRACE_NORMAL, CUT_PBASE_T_USBDI_0482_TRANSFERCOMPLETEL_DUP02, "unexpected transfer!");
				TestFailed(KErrCorrupt);
				}
			iExpectedTransferId++;		
			// is it the last transfer?
			if(iExpectedTransferId == iOutTransfers.Count())
				{
				OstTrace0(TRACE_NORMAL, CUT_PBASE_T_USBDI_0482_TRANSFERCOMPLETEL_DUP03, "last transfer successful! lets replay recorded data");
						
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
	OstTraceFunctionExit1( CUT_PBASE_T_USBDI_0482_TRANSFERCOMPLETEL_EXIT_DUP02, this );
	}									   

void CUT_PBASE_T_USBDI_0482::Ep0TransferCompleteL(TInt aCompletionCode)
	{
	OstTraceFunctionEntryExt( CUT_PBASE_T_USBDI_0482_EP0TRANSFERCOMPLETEL_ENTRY, this );
	OstTrace1(TRACE_NORMAL, CUT_PBASE_T_USBDI_0482_EP0TRANSFERCOMPLETEL, "Transfer EP0 completed aCompletionCode = %d", aCompletionCode);

	if(aCompletionCode != KErrNone)
		{
		TBuf<256> msg;
		msg.Format(_L("<Error %d> Transfer to control endpoint 0 was not successful"),aCompletionCode);
		OstTrace0(TRACE_NORMAL, CUT_PBASE_T_USBDI_0482_EP0TRANSFERCOMPLETEL_DUP01, msg);
		OstTraceFunctionExit1( CUT_PBASE_T_USBDI_0482_EP0TRANSFERCOMPLETEL_EXIT, this );
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
			OstTrace0(TRACE_NORMAL, CUT_PBASE_T_USBDI_0482_EP0TRANSFERCOMPLETEL_DUP02, "<Error> Unknown test step");
			TestFailed(KErrUnknown);
			break;
		}
	OstTraceFunctionExit1( CUT_PBASE_T_USBDI_0482_EP0TRANSFERCOMPLETEL_EXIT_DUP01, this );
	}	
	 	
	
void CUT_PBASE_T_USBDI_0482::DeviceRemovedL(TUint aDeviceHandle)
	{
	OstTraceFunctionEntryExt( CUT_PBASE_T_USBDI_0482_DEVICEREMOVEDL_ENTRY, this );
	
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
	OstTraceFunctionExit1( CUT_PBASE_T_USBDI_0482_DEVICEREMOVEDL_EXIT, this );
	}
	
	
void CUT_PBASE_T_USBDI_0482::BusErrorL(TInt aError)
	{
	OstTraceFunctionEntryExt( CUT_PBASE_T_USBDI_0482_BUSERRORL_ENTRY, this );
	// This test case handles no failiures on the bus	
	TestFailed(KErrCompletion);	
	OstTraceFunctionExit1( CUT_PBASE_T_USBDI_0482_BUSERRORL_EXIT, this );
	}


void CUT_PBASE_T_USBDI_0482::HostRunL()
	{
	OstTraceFunctionEntry1( CUT_PBASE_T_USBDI_0482_HOSTRUNL_ENTRY, this );
	
	// Obtain the completion code
	TInt completionCode(iStatus.Int());
	
	if(completionCode == KErrNone)
		{
		// Action timeout
		OstTrace0(TRACE_NORMAL, CUT_PBASE_T_USBDI_0482_HOSTRUNL, "<Error> Action timeout");
		TestFailed(KErrTimedOut);
		}
	else
		{
		OstTrace1(TRACE_NORMAL, CUT_PBASE_T_USBDI_0482_HOSTRUNL_DUP01, "<Error %d> Timeout timer could not complete",completionCode);
		TestFailed(completionCode);
		}
	OstTraceFunctionExit1( CUT_PBASE_T_USBDI_0482_HOSTRUNL_EXIT, this );
	}

void CUT_PBASE_T_USBDI_0482::DeviceRunL()
	{	
	OstTraceFunctionEntry1( CUT_PBASE_T_USBDI_0482_DEVICERUNL_ENTRY, this );
	OstTraceFunctionExit1( CUT_PBASE_T_USBDI_0482_DEVICERUNL_EXIT, this );
	}
		
	}
