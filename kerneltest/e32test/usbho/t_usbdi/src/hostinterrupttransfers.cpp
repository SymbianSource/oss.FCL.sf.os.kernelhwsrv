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
// @file hostinterrupttransfers.cpp
// @internalComponent
// 
//

#include "hosttransfers.h"
#include "testdebug.h"

namespace NUnitTesting_USBDI
	{
	
	
CInterruptTransfer::CInterruptTransfer(RUsbPipe& aPipe,RUsbInterface& aInterface,TInt aMaxTransferSize,MTransferObserver& aObserver,TInt aTransferId)
:	CBaseTransfer(aPipe,aInterface,aObserver,aTransferId),
	iTransferDescriptor(aMaxTransferSize) // Allocate the buffer for interrupt transfers
	{

	}
	
CInterruptTransfer::~CInterruptTransfer()
	{
	LOG_FUNC 
	
	// Cancel the transfer

	Cancel();
	}
	
	
TPtrC8 CInterruptTransfer::DataPolled()
	{
	return iTransferDescriptor.Buffer();
	}
	
	
TInt CInterruptTransfer::TransferInL(TInt aSize)
	{
	LOG_FUNC
	
	// Activate the asynchronous transfer 	
	RDebug::Printf("Activating interrupt in transfer");
	iTransferDescriptor.SaveData(aSize);
	Pipe().Transfer(iTransferDescriptor,iStatus);
	SetActive();
	return KErrNone;
	}

TInt CInterruptTransfer::RegisterTransferDescriptor()
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
	

		
	}
