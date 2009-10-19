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
// @file hostisochronoustransfers.cpp
// @internalComponent
// 
//

#include "hosttransfers.h"
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
	RDebug::Printf("aMaxPacketSize = %d, aMaxNumPackets = %d",aMaxPacketSize, aMaxNumPackets);
	}
	
CIsochTransfer::~CIsochTransfer()
	{
	LOG_FUNC
	
	// Cancel the transfer

	Cancel();
	}	
	
TBool CIsochTransfer::DataPolled(TUint aPacketsToBeRead, RBuf8& aDataPolled) 
	{
	LOG_FUNC
	TInt numOfPacketsReturned = 0;	
	
	TInt firstPacketIndex = 0;
	TInt totalPacketsRead = 0;
	TInt packetsToBeRead = aPacketsToBeRead;

	TUint dataPolledBufSize = iMaxPacketSize*aPacketsToBeRead;
	aDataPolled.CreateL(dataPolledBufSize);
		
	do {						
		TPtrC8 ptrRet = iTransferDescriptor.Packets(firstPacketIndex, packetsToBeRead, numOfPacketsReturned); 
		RDebug::Printf("numOfPacketsReturned = %d", numOfPacketsReturned);
		RDebug::Printf("ptrRet.Length() = %d", ptrRet.Length());
		firstPacketIndex = numOfPacketsReturned;
		totalPacketsRead += numOfPacketsReturned;
		packetsToBeRead = packetsToBeRead - numOfPacketsReturned;
		RDebug::Printf("totalPacketsRead = %d", totalPacketsRead);	
		RDebug::Printf("packetsToBeRead = %d", packetsToBeRead);	
		aDataPolled.Append(ptrRet);		
		}	while(totalPacketsRead != aPacketsToBeRead); 	
		
	return ETrue; 
	}
	
	
TInt CIsochTransfer::TransferInL(TInt aPacketsExpected)
	{
	LOG_FUNC
	
	// Activate the asynchronous transfer 	
	RDebug::Printf("Activating isoch. in transfer");
	
	iTransferDescriptor.Reset();
	TPacketLengths fullLengths = iTransferDescriptor.Lengths();
	
	for(TInt packet = 0; packet < fullLengths.MaxNumPackets(); packet++)
		{
		fullLengths[packet] = iMaxPacketSize;
		}	

	RDebug::Printf("fullLengths.MaxNumPackets() == %d",fullLengths.MaxNumPackets());
	iTransferDescriptor.ReceivePackets(aPacketsExpected);
		
	Pipe().Transfer(iTransferDescriptor,iStatus);
	SetActive();
	return KErrNone;																
	}
		
TInt CIsochTransfer::RegisterTransferDescriptor()
	{
	LOG_FUNC
	
	// Register the transfer descriptor with the interface	
	TInt err(Interface().RegisterTransferDescriptor(iTransferDescriptor));
	if(err != KErrNone)
		{
		RDebug::Printf("<Error %d> Unable to register transfer descriptor",err);
		}
	return err;	
	}
	
TInt CIsochTransfer::PrepareTransfer(const TDesC8& aIsochData)
	{
	LOG_FUNC

	//	
	iTransferDescriptor.Reset();
	TPacketLengths fullLengths = iTransferDescriptor.Lengths();

	RDebug::Printf("fullLengths.MaxNumPackets() == %d",fullLengths.MaxNumPackets());
	
	//	
	TInt bytesRemaining(aIsochData.Size());
	TInt maxAvailablePacketSlots;
	TInt startOffset(0);
	TInt startPacket(0); 
	
	RDebug::Printf("Audio data is %d bytes",bytesRemaining);
	
	// Keep saving the isoch data to transfer in each packet buffer supplied 
	// by the transfer descriptor

	while(bytesRemaining)
		{
		// Request a modifiable buffer to write the isoch data to
		TPtr8 packetBuffer = iTransferDescriptor.WritablePackets(bytesRemaining/iMaxPacketSize,maxAvailablePacketSlots);
		TInt dataToWrite = Min(bytesRemaining,packetBuffer.MaxSize());		
		
		if(dataToWrite == 0)
			{
			RDebug::Printf("<Warning> dropping the rest of the isoch data");
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
	return KErrNone;
	}

TInt CIsochTransfer::TransferOut()
	{	
	// Transfer the iscohronous data	
	RDebug::Printf("Activating isochronous out transfer");
	Pipe().Transfer(iTransferDescriptor,iStatus);
	SetActive();
	return KErrNone;
	}
	
		
	}
