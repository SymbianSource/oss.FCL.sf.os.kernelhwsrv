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
// 
//

#include "Ep0Reader.h"
#include "testdebug.h"

namespace NUnitTesting_USBDI
	{
	
CDeviceEndpoint0* CDeviceEndpoint0::NewL(MRequestHandler& aRequestHandler)
	{
	CDeviceEndpoint0* self = new (ELeave) CDeviceEndpoint0();
	CleanupStack::PushL(self);
	self->ConstructL(aRequestHandler);
	CleanupStack::Pop(self);
	return self;
	}
	
	
CDeviceEndpoint0::CDeviceEndpoint0()
	{
	}	
	
	
CDeviceEndpoint0::~CDeviceEndpoint0()
	{
	LOG_FUNC

	// Destroy the reader/writer
	delete iEndpoint0Writer; 
	delete iEndpoint0Reader; 
	
	// Close channel to the driver
	iClientDriver.Close();
	}
	
	
void CDeviceEndpoint0::ConstructL(MRequestHandler& aRequestHandler)
	{
	LOG_FUNC
	TInt err(iClientDriver.Open(0));
	if(err != KErrNone)
		{
		RDebug::Printf("<Error %d> Unable to open a channel to USB client driver",err);
		User::Leave(err);
		}
	
	// Create the reader of data on device endpoint 0
	iEndpoint0Reader = new (ELeave) CControlEndpointReader(iClientDriver,aRequestHandler);
	
	// Create the writer of data on device endpoint 0
	iEndpoint0Writer = new (ELeave) CEndpointWriter(iClientDriver,EEndpoint0);
	}

	
TInt CDeviceEndpoint0::Start()
	{
	LOG_FUNC
	
	// Make this channel to the driver able to get device directed ep0 requests
	TInt err(iClientDriver.SetDeviceControl());
	
	// Check operation success
	if(err != KErrNone)
		{
		RDebug::Printf("<Error %d> Unable to obtain device control",err);
		return err;
		}
		
	// Start reading for requests from host
	TRAP(err,iEndpoint0Reader->ReadRequestsL());
	
	return err;
	}


TInt CDeviceEndpoint0::Stop()
	{
	LOG_FUNC
	// Cancel the data reader and writer
	iEndpoint0Writer->Cancel();
	iEndpoint0Reader->Cancel();
	
	// Give device control back
	TInt err(iClientDriver.ReleaseDeviceControl());
	if(err != KErrNone)
		{
		RDebug::Printf("<Error %d> Unable to release device control",err);
		}
	return err;
	}


void CDeviceEndpoint0::SendData(const TDesC8& aData)
	{
	LOG_FUNC
	iEndpoint0Writer->Write(aData, ETrue);
	}

TInt CDeviceEndpoint0::SendDataSynchronous(const TDesC8& aData)
	{
	LOG_FUNC
	return iEndpoint0Writer->WriteSynchronous(aData, ETrue);
	}

CControlEndpointReader& CDeviceEndpoint0::Reader()
	{
	return *iEndpoint0Reader;
	}

	}

