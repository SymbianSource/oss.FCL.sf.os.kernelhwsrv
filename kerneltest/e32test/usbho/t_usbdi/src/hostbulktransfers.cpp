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
// @file hostcontroltransfers.cpp
// @internalComponent
// 
//

#include "hosttransfers.h"
#include "OstTraceDefinitions.h"
#ifdef OST_TRACE_COMPILER_IN_USE
#include "hostbulktransfersTraces.h"
#endif
#include <e32debug.h>

namespace NUnitTesting_USBDI
	{
	
	
CBulkTransfer::CBulkTransfer(RUsbPipe& aPipe,RUsbInterface& aInterface,TInt aMaxTransferSize,MTransferObserver& aObserver,TInt aTransferId)
:	CBaseTransfer(aPipe,aInterface,aObserver,aTransferId),
	iTransferDescriptor(aMaxTransferSize) // Allocate the buffer for bulk transfers
	{
    OstTraceFunctionEntryExt( CBULKTRANSFER_CBULKTRANSFER_ENTRY, this );

	// Register the transfer descriptor with the interface
	
	TInt err(Interface().RegisterTransferDescriptor(iTransferDescriptor));
	if(err != KErrNone)
		{
		OstTrace1(TRACE_NORMAL, CBULKTRANSFER_CBULKTRANSFER, "<Error %d> Unable to register transfer descriptor",err);
		}
	OstTraceFunctionExit1( CBULKTRANSFER_CBULKTRANSFER_EXIT, this );
	}
	
	
CBulkTransfer::~CBulkTransfer()
	{
	OstTraceFunctionEntry1( CBULKTRANSFER_CBULKTRANSFER_ENTRY_DUP01, this );
	
	// Cancel the transfer

	Cancel();
	OstTraceFunctionExit1( CBULKTRANSFER_CBULKTRANSFER_EXIT_DUP01, this );
	}
	
	
TPtrC8 CBulkTransfer::DataPolled()
	{
	OstTraceFunctionEntry1( CBULKTRANSFER_DATAPOLLED_ENTRY, this );
	return iTransferDescriptor.Buffer();
	}


void CBulkTransfer::TransferIn(TInt aExpectedDataSize)
	{
	OstTraceFunctionEntryExt( CBULKTRANSFER_TRANSFERIN_ENTRY, this );
	
	// Activate the asynchronous transfer 
	
	OstTrace0(TRACE_NORMAL, CBULKTRANSFER_TRANSFERIN, "Activating bulk in transfer");
	iTransferDescriptor.SaveData(aExpectedDataSize);
	Pipe().Transfer(iTransferDescriptor,iStatus);
	SetActive();
	OstTraceFunctionExit1( CBULKTRANSFER_TRANSFERIN_EXIT, this );
	}

void CBulkTransfer::TransferOut(const TDesC8& aBulkData, TBool aUseZLPIfRequired)
	{
    OstTraceFunctionEntryExt( CBULKTRANSFER_TRANSFEROUT_ENTRY, this );

	// Copy the data across
	if(aBulkData.Length() > iTransferDescriptor.iMaxSize)
		{
		OstTrace0(TRACE_NORMAL, CBULKTRANSFER_TRANSFEROUT, "Too much data in bulk transfer. This test suite will now PANIC!");
		OstTraceExt2(TRACE_NORMAL, CBULKTRANSFER_TRANSFEROUT_DUP01, "Bytes requested %d, Max allowed %d", aBulkData.Length(), iTransferDescriptor.iMaxSize);
		ASSERT(EFalse);
		}
		
	TPtr8 buffer = iTransferDescriptor.WritableBuffer();
	buffer.Copy(aBulkData);
	OstTrace1(TRACE_NORMAL, CBULKTRANSFER_TRANSFEROUT_DUP02, "Transfer buffer now has %d bytes to write",buffer.Length());
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
	
	OstTrace0(TRACE_NORMAL, CBULKTRANSFER_TRANSFEROUT_DUP03, "Activating bulk out transfer");
	Pipe().Transfer(iTransferDescriptor,iStatus);
	SetActive();
	OstTraceFunctionExit1( CBULKTRANSFER_TRANSFEROUT_EXIT, this );
	}

void CBulkTransfer::TransferOut(const TDesC8& aBulkDataPattern, TUint aNumBytes, TBool aUseZLPIfRequired)
	{
	OstTraceFunctionEntryExt( CBULKTRANSFER_TRANSFEROUT_ENTRY_DUP01, this );
	
	TransferOut(aBulkDataPattern, 0, aNumBytes, aUseZLPIfRequired);
	OstTraceFunctionExit1( CBULKTRANSFER_TRANSFEROUT_EXIT_DUP01, this );
	}


void CBulkTransfer::TransferOut(const TDesC8& aBulkDataPattern, TUint aStartPoint, TUint aNumBytes, TBool aUseZLPIfRequired)
	{
    OstTraceFunctionEntryExt( CBULKTRANSFER_TRANSFEROUT_ENTRY_DUP02, this );

	// Copy the data across
	if(aNumBytes > iTransferDescriptor.iMaxSize)
		{
		OstTrace0(TRACE_NORMAL, CBULKTRANSFER_TRANSFEROUT_DUP10, "Too much data in bulk transfer. This test suite will now PANIC!");
		OstTraceExt2(TRACE_NORMAL, CBULKTRANSFER_TRANSFEROUT_DUP11, "Bytes requested %d, Max allowed %d", aNumBytes, iTransferDescriptor.iMaxSize);
		ASSERT(EFalse);
		}
	if(aBulkDataPattern.Length()<=0)
		{
		OstTrace0(TRACE_NORMAL, CBULKTRANSFER_TRANSFEROUT_DUP12, "ZERO LENGTH data pattern used in TransferOut. This test suite will now PANIC!");
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
	OstTrace1(TRACE_NORMAL, CBULKTRANSFER_TRANSFEROUT_DUP13, "Transfer buffer now has %d bytes to write",buffer.Length());
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
	
	OstTrace0(TRACE_NORMAL, CBULKTRANSFER_TRANSFEROUT_DUP14, "Activating bulk out transfer");
	Pipe().Transfer(iTransferDescriptor,iStatus);
	SetActive();
	OstTraceFunctionExit1( CBULKTRANSFER_TRANSFEROUT_EXIT_DUP02, this );
	}

	}
