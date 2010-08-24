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
// @file hostisochronoustransfers.cpp
// @internalComponent
// 
//

#include "hosttransfers.h"
#include "OstTraceDefinitions.h"
#ifdef OST_TRACE_COMPILER_IN_USE
#include "hostisochronoustransfersTraces.h"
#endif
#include <e32debug.h>
#include <e32test.h>

extern RTest gtest;

namespace NUnitTesting_USBDI
	{

CIsochTransfer::CIsochTransfer(RUsbPipe& aPipe,RUsbInterface& aInterface,TUint16 aMaxPacketSize,
							   TInt aMaxNumPackets,MTransferObserver& aObserver,TInt aTransferId)
:	CBaseTransfer(aPipe,aInterface,aObserver,aTransferId),
	iTransferDescriptor(aMaxPacketSize,aMaxNumPackets),
	iMaxPacketSize(aMaxPacketSize)
	{
	OstTraceFunctionEntryExt( CISOCHTRANSFER_CISOCHTRANSFER_ENTRY, this );
	OstTraceExt2(TRACE_NORMAL, CISOCHTRANSFER_CISOCHTRANSFER, "aMaxPacketSize = %d, aMaxNumPackets = %d",aMaxPacketSize, aMaxNumPackets);
	OstTraceFunctionExit1( CISOCHTRANSFER_CISOCHTRANSFER_EXIT, this );
	}
	
CIsochTransfer::~CIsochTransfer()
	{
	OstTraceFunctionEntry1( CISOCHTRANSFER_CISOCHTRANSFER_ENTRY_DUP01, this );
	
	// Cancel the transfer

	Cancel();
	OstTraceFunctionExit1( CISOCHTRANSFER_CISOCHTRANSFER_EXIT_DUP01, this );
	}	
	
TBool CIsochTransfer::DataPolled(TUint aPacketsToBeRead, RBuf8& aDataPolled) 
	{
	OstTraceFunctionEntryExt( CISOCHTRANSFER_DATAPOLLED_ENTRY, this );
	TInt numOfPacketsReturned = 0;	
	
	TInt firstPacketIndex = 0;
	TInt totalPacketsRead = 0;
	TInt packetsToBeRead = aPacketsToBeRead;

	TUint dataPolledBufSize = iMaxPacketSize*aPacketsToBeRead;
	aDataPolled.CreateL(dataPolledBufSize);
		
	do {						
		TPtrC8 ptrRet = iTransferDescriptor.Packets(firstPacketIndex, packetsToBeRead, numOfPacketsReturned); 
		OstTrace1(TRACE_NORMAL, CISOCHTRANSFER_DATAPOLLED, "numOfPacketsReturned = %d", numOfPacketsReturned);
		OstTrace1(TRACE_NORMAL, CISOCHTRANSFER_DATAPOLLED_DUP01, "ptrRet.Length() = %d", ptrRet.Length());
		firstPacketIndex = numOfPacketsReturned;
		totalPacketsRead += numOfPacketsReturned;
		packetsToBeRead = packetsToBeRead - numOfPacketsReturned;
		OstTrace1(TRACE_NORMAL, CISOCHTRANSFER_DATAPOLLED_DUP02, "totalPacketsRead = %d", totalPacketsRead);	
		OstTrace1(TRACE_NORMAL, CISOCHTRANSFER_DATAPOLLED_DUP03, "packetsToBeRead = %d", packetsToBeRead);	
		aDataPolled.Append(ptrRet);		
		}	while(totalPacketsRead != aPacketsToBeRead); 	
		
	OstTraceFunctionExitExt( CISOCHTRANSFER_DATAPOLLED_EXIT, this, ETrue );
	return ETrue; 
	}
	
	
TInt CIsochTransfer::TransferInL(TInt aPacketsExpected)
	{
	OstTraceFunctionEntryExt( CISOCHTRANSFER_TRANSFERINL_ENTRY, this );
	
	// Activate the asynchronous transfer 	
	OstTrace0(TRACE_NORMAL, CISOCHTRANSFER_TRANSFERINL, "Activating isoch. in transfer");
	
	iTransferDescriptor.Reset();
	TPacketLengths fullLengths = iTransferDescriptor.Lengths();
	
	for(TInt packet = 0; packet < fullLengths.MaxNumPackets(); packet++)
		{
		fullLengths[packet] = iMaxPacketSize;
		}	

	OstTrace1(TRACE_NORMAL, CISOCHTRANSFER_TRANSFERINL_DUP01, "fullLengths.MaxNumPackets() == %d",fullLengths.MaxNumPackets());
	iTransferDescriptor.ReceivePackets(aPacketsExpected);
		
	Pipe().Transfer(iTransferDescriptor,iStatus);
	SetActive();
	OstTraceFunctionExitExt( CISOCHTRANSFER_TRANSFERINL_EXIT, this, KErrNone );
	return KErrNone;																
	}
		
TInt CIsochTransfer::RegisterTransferDescriptor()
	{
	OstTraceFunctionEntry1( CISOCHTRANSFER_REGISTERTRANSFERDESCRIPTOR_ENTRY, this );
	
	// Register the transfer descriptor with the interface	
	TInt err(Interface().RegisterTransferDescriptor(iTransferDescriptor));
	if(err != KErrNone)
		{
		OstTrace1(TRACE_NORMAL, CISOCHTRANSFER_REGISTERTRANSFERDESCRIPTOR, "<Error %d> Unable to register transfer descriptor",err);
		}
	OstTraceFunctionExitExt( CISOCHTRANSFER_REGISTERTRANSFERDESCRIPTOR_EXIT, this, err );
	return err;	
	}
	
TInt CIsochTransfer::PrepareTransfer(const TDesC8& aIsochData)
	{
OstTraceFunctionEntryExt( CISOCHTRANSFER_PREPARETRANSFER_ENTRY, this );

	//	
	iTransferDescriptor.Reset();
	TPacketLengths fullLengths = iTransferDescriptor.Lengths();

	OstTrace1(TRACE_NORMAL, CISOCHTRANSFER_PREPARETRANSFER, "fullLengths.MaxNumPackets() == %d",fullLengths.MaxNumPackets());
	
	//	
	TInt bytesRemaining(aIsochData.Size());
	TInt maxAvailablePacketSlots;
	TInt startOffset(0);
	TInt startPacket(0); 
	
	OstTrace1(TRACE_NORMAL, CISOCHTRANSFER_PREPARETRANSFER_DUP01, "Audio data is %d bytes",bytesRemaining);
	
	// Keep saving the isoch data to transfer in each packet buffer supplied 
	// by the transfer descriptor

	while(bytesRemaining)
		{
		// Request a modifiable buffer to write the isoch data to
		TPtr8 packetBuffer = iTransferDescriptor.WritablePackets(bytesRemaining/iMaxPacketSize,maxAvailablePacketSlots);
		TInt dataToWrite = Min(bytesRemaining,packetBuffer.MaxSize());		
		
		if(dataToWrite == 0)
			{
			OstTrace0(TRACE_NORMAL, CISOCHTRANSFER_PREPARETRANSFER_DUP02, "<Warning> dropping the rest of the isoch data");
			break;
			}
		
		// Validate entire buffer as it is going to be filled
		
		packetBuffer.SetMax();
		// Calculate the number of packets to write in this buffer		
		TInt maxPacket = dataToWrite / iMaxPacketSize;
	
		for(TInt packet = 0; packet < maxPacket; packet++)
			{
			fullLengths[startPacket + packet] = iMaxPacketSize;
			}
		
		packetBuffer.Copy(aIsochData.Mid(startOffset, maxPacket * iMaxPacketSize));
		iTransferDescriptor.SaveMultiple(maxPacket);
		bytesRemaining -= maxPacket * iMaxPacketSize;
		startOffset += maxPacket * iMaxPacketSize;
		startPacket += maxPacket;
		}	
	OstTraceFunctionExitExt( CISOCHTRANSFER_PREPARETRANSFER_EXIT, this, KErrNone );
	return KErrNone;
	}

TInt CIsochTransfer::TransferOut()
	{	
	OstTraceFunctionEntry1( CISOCHTRANSFER_TRANSFEROUT_ENTRY, this );
	// Transfer the iscohronous data	
	OstTrace0(TRACE_NORMAL, CISOCHTRANSFER_TRANSFEROUT, "Activating isochronous out transfer");
	Pipe().Transfer(iTransferDescriptor,iStatus);
	SetActive();
	OstTraceFunctionExitExt( CISOCHTRANSFER_TRANSFEROUT_EXIT, this, KErrNone );
	return KErrNone;
	}
	
		
	}
