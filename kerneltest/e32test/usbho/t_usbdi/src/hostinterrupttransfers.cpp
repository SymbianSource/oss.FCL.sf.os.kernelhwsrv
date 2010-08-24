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
// @file hostinterrupttransfers.cpp
// @internalComponent
// 
//

#include "hosttransfers.h"
#include "testdebug.h"
#include "OstTraceDefinitions.h"
#ifdef OST_TRACE_COMPILER_IN_USE
#include "hostinterrupttransfersTraces.h"
#endif

namespace NUnitTesting_USBDI
	{
	
	
CInterruptTransfer::CInterruptTransfer(RUsbPipe& aPipe,RUsbInterface& aInterface,TInt aMaxTransferSize,MTransferObserver& aObserver,TInt aTransferId)
:	CBaseTransfer(aPipe,aInterface,aObserver,aTransferId),
	iTransferDescriptor(aMaxTransferSize) // Allocate the buffer for interrupt transfers
	{
    OstTraceFunctionEntryExt( CINTERRUPTTRANSFER_CINTERRUPTTRANSFER_ENTRY, this );

	OstTraceFunctionExit1( CINTERRUPTTRANSFER_CINTERRUPTTRANSFER_EXIT, this );
	}
	
CInterruptTransfer::~CInterruptTransfer()
	{
	OstTraceFunctionEntry1( CINTERRUPTTRANSFER_CINTERRUPTTRANSFER_ENTRY_DUP01, this );
	
	// Cancel the transfer

	Cancel();
	OstTraceFunctionExit1( CINTERRUPTTRANSFER_CINTERRUPTTRANSFER_EXIT_DUP01, this );
	}
	
	
TPtrC8 CInterruptTransfer::DataPolled()
	{
	OstTraceFunctionEntry1( CINTERRUPTTRANSFER_DATAPOLLED_ENTRY, this );
	return iTransferDescriptor.Buffer();
	}
	
	
TInt CInterruptTransfer::TransferInL(TInt aSize)
	{
	OstTraceFunctionEntryExt( CINTERRUPTTRANSFER_TRANSFERINL_ENTRY, this );
	
	// Activate the asynchronous transfer 	
	OstTrace0(TRACE_NORMAL, CINTERRUPTTRANSFER_TRANSFERINL, "Activating interrupt in transfer");
	iTransferDescriptor.SaveData(aSize);
	Pipe().Transfer(iTransferDescriptor,iStatus);
	SetActive();
	OstTraceFunctionExitExt( CINTERRUPTTRANSFER_TRANSFERINL_EXIT, this, KErrNone );
	return KErrNone;
	}

TInt CInterruptTransfer::RegisterTransferDescriptor()
	{
	OstTraceFunctionEntry1( CINTERRUPTTRANSFER_REGISTERTRANSFERDESCRIPTOR_ENTRY, this );
	
	// Register the transfer descriptor with the interface	
	TInt err(Interface().RegisterTransferDescriptor(iTransferDescriptor));
	if(err != KErrNone)
		{
		OstTrace1(TRACE_NORMAL, CINTERRUPTTRANSFER_REGISTERTRANSFERDESCRIPTOR, "<Error %d> Unable to register transfer descriptor",err);
		}
	OstTraceFunctionExitExt( CINTERRUPTTRANSFER_REGISTERTRANSFERDESCRIPTOR_EXIT, this, err );
	return err;
	}
	

		
	}
