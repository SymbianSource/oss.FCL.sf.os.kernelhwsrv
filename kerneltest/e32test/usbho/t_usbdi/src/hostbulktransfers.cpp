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
// @file hostcontroltransfers.cpp
// @internalComponent
// 
//

#include "hosttransfers.h"
#include <e32debug.h>

namespace NUnitTesting_USBDI
	{
	
	
CBulkTransfer::CBulkTransfer(RUsbPipe& aPipe,RUsbInterface& aInterface,TInt aMaxTransferSize,MTransferObserver& aObserver,TInt aTransferId)
:	CBaseTransfer(aPipe,aInterface,aObserver,aTransferId),
	iTransferDescriptor(aMaxTransferSize) // Allocate the buffer for bulk transfers
	{

	// Register the transfer descriptor with the interface
	
	TInt err(Interface().RegisterTransferDescriptor(iTransferDescriptor));
	if(err != KErrNone)
		{
		RDebug::Printf("<Error %d> Unable to register transfer descriptor",err);
		}
	}
	
	
CBulkTransfer::~CBulkTransfer()
	{
	LOG_FUNC
	
	// Cancel the transfer

	Cancel();
	}
	
	
TPtrC8 CBulkTransfer::DataPolled()
	{
	return iTransferDescriptor.Buffer();
	}


void CBulkTransfer::TransferIn(TInt aExpectedDataSize)
	{
	LOG_FUNC
	
	// Activate the asynchronous transfer 
	
	RDebug::Printf("Activating bulk in transfer");
	iTransferDescriptor.SaveData(aExpectedDataSize);
	Pipe().Transfer(iTransferDescriptor,iStatus);
	SetActive();
	}

void CBulkTransfer::TransferOut(const TDesC8& aBulkData, TBool aUseZLPIfRequired)
	{
	LOG_FUNC

	// Copy the data across
	if(aBulkData.Length() > iTransferDescriptor.iMaxSize)
		{
		RDebug::Printf("Too much data in bulk transfer. This test suite will now PANIC!");
		RDebug::Printf("Bytes requested %d, Max allowed %d", aBulkData.Length(), iTransferDescriptor.iMaxSize);
		ASSERT(EFalse);
		}
		
	TPtr8 buffer = iTransferDescriptor.WritableBuffer();
	buffer.Copy(aBulkData);
	RDebug::Printf("Transfer buffer now has %d bytes to write",buffer.Length());
	iTransferDescriptor.SaveData(buffer.Length());
	if(aUseZLPIfRequired)
		{
		iTransferDescriptor.SetZlpStatus(RUsbTransferDescriptor::ESendZlpIfRequired); 
		}
	else
		{
		iTransferDescriptor.SetZlpStatus(RUsbTransferDescriptor::ESuppressZlp);
		}
	
	// Activate the asynchronous transfer 
	
	RDebug::Printf("Activating bulk out transfer");
	Pipe().Transfer(iTransferDescriptor,iStatus);
	SetActive();
	}

void CBulkTransfer::TransferOut(const TDesC8& aBulkDataPattern, TUint aNumBytes, TBool aUseZLPIfRequired)
	{
	LOG_FUNC
	
	TransferOut(aBulkDataPattern, 0, aNumBytes, aUseZLPIfRequired);
	}


void CBulkTransfer::TransferOut(const TDesC8& aBulkDataPattern, TUint aStartPoint, TUint aNumBytes, TBool aUseZLPIfRequired)
	{
	LOG_FUNC

	// Copy the data across
	if(aNumBytes > iTransferDescriptor.iMaxSize)
		{
		RDebug::Printf("Too much data in bulk transfer. This test suite will now PANIC!");
		RDebug::Printf("Bytes requested %d, Max allowed %d", aNumBytes, iTransferDescriptor.iMaxSize);
		ASSERT(EFalse);
		}
	if(aBulkDataPattern.Length()<=0)
		{
		RDebug::Printf("ZERO LENGTH data pattern used in TransferOut. This test suite will now PANIC!");
		ASSERT(EFalse);
		}
	TUint startPoint = aStartPoint%aBulkDataPattern.Length();
	TUint numStartBytes = (aBulkDataPattern.Length() - startPoint)%aBulkDataPattern.Length();
	TUint fullRepeats = (aNumBytes-numStartBytes)/aBulkDataPattern.Length();
	TUint numEndBytes = aNumBytes - fullRepeats*aBulkDataPattern.Length() - numStartBytes;
	TPtr8 buffer = iTransferDescriptor.WritableBuffer();
	buffer.Zero(); //set length to zero
	if(numStartBytes)
		{
		buffer.Append(aBulkDataPattern.Right(numStartBytes));
		}
	for(TUint i = 0; i<fullRepeats; i++)
		{
		buffer.Append(aBulkDataPattern);
		}
	if(numEndBytes)
		{
		buffer.Append(aBulkDataPattern.Left(numEndBytes));
		}
	RDebug::Printf("Transfer buffer now has %d bytes to write",buffer.Length());
	iTransferDescriptor.SaveData(buffer.Length());	
	if(aUseZLPIfRequired)
		{
		iTransferDescriptor.SetZlpStatus(RUsbTransferDescriptor::ESendZlpIfRequired); 
		}
	else
		{
		iTransferDescriptor.SetZlpStatus(RUsbTransferDescriptor::ESuppressZlp);
		}
	
	// Activate the asynchronous transfer 
	
	RDebug::Printf("Activating bulk out transfer");
	Pipe().Transfer(iTransferDescriptor,iStatus);
	SetActive();
	}

	}
