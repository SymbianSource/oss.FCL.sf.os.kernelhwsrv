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
// 
//

#include "Ep0Reader.h"
#include "testdebug.h"
#include "OstTraceDefinitions.h"
#ifdef OST_TRACE_COMPILER_IN_USE
#include "Ep0ReaderTraces.h"
#endif

namespace NUnitTesting_USBDI
	{
	
CDeviceEndpoint0* CDeviceEndpoint0::NewL(MRequestHandler& aRequestHandler)
	{
	OstTraceFunctionEntry1( CDEVICEENDPOINT0_NEWL_ENTRY, ( TUint )&( aRequestHandler ) );
	CDeviceEndpoint0* self = new (ELeave) CDeviceEndpoint0();
	CleanupStack::PushL(self);
	self->ConstructL(aRequestHandler);
	CleanupStack::Pop(self);
	OstTraceFunctionExit1( CDEVICEENDPOINT0_NEWL_EXIT, ( TUint )( self ) );
	return self;
	}
	
	
CDeviceEndpoint0::CDeviceEndpoint0()
	{
	OstTraceFunctionEntry1( CDEVICEENDPOINT0_CDEVICEENDPOINT0_ENTRY, this );
	OstTraceFunctionExit1( CDEVICEENDPOINT0_CDEVICEENDPOINT0_EXIT, this );
	}	
	
	
CDeviceEndpoint0::~CDeviceEndpoint0()
	{
    OstTraceFunctionEntry1( CDEVICEENDPOINT0_CDEVICEENDPOINT0_ENTRY_DUP01, this );

	// Destroy the reader/writer
	delete iEndpoint0Writer; 
	delete iEndpoint0Reader; 
	
	// Close channel to the driver
	iClientDriver.Close();
	OstTraceFunctionExit1( CDEVICEENDPOINT0_CDEVICEENDPOINT0_EXIT_DUP01, this );
	}
	
	
void CDeviceEndpoint0::ConstructL(MRequestHandler& aRequestHandler)
	{
	OstTraceFunctionEntryExt( CDEVICEENDPOINT0_CONSTRUCTL_ENTRY, this );
	TInt err(iClientDriver.Open(0));
	if(err != KErrNone)
		{
		OstTrace1(TRACE_NORMAL, CDEVICEENDPOINT0_CONSTRUCTL, "<Error %d> Unable to open a channel to USB client driver",err);
		User::Leave(err);
		}
	
	// Create the reader of data on device endpoint 0
	iEndpoint0Reader = new (ELeave) CControlEndpointReader(iClientDriver,aRequestHandler);
	
	// Create the writer of data on device endpoint 0
	iEndpoint0Writer = new (ELeave) CEndpointWriter(iClientDriver,EEndpoint0);
	OstTraceFunctionExit1( CDEVICEENDPOINT0_CONSTRUCTL_EXIT, this );
	}

	
TInt CDeviceEndpoint0::Start()
	{
	OstTraceFunctionEntry1( CDEVICEENDPOINT0_START_ENTRY, this );
	
	// Make this channel to the driver able to get device directed ep0 requests
	TInt err(iClientDriver.SetDeviceControl());
	
	// Check operation success
	if(err != KErrNone)
		{
		OstTrace1(TRACE_NORMAL, CDEVICEENDPOINT0_START, "<Error %d> Unable to obtain device control",err);
		OstTraceFunctionExitExt( CDEVICEENDPOINT0_START_EXIT, this, err );
		return err;
		}
		
	// Start reading for requests from host
	TRAP(err,iEndpoint0Reader->ReadRequestsL());
	
	OstTraceFunctionExitExt( CDEVICEENDPOINT0_START_EXIT_DUP01, this, err );
	return err;
	}


TInt CDeviceEndpoint0::Stop()
	{
	OstTraceFunctionEntry1( CDEVICEENDPOINT0_STOP_ENTRY, this );
	// Cancel the data reader and writer
	iEndpoint0Writer->Cancel();
	iEndpoint0Reader->Cancel();
	
	// Give device control back
	TInt err(iClientDriver.ReleaseDeviceControl());
	if(err != KErrNone)
		{
		OstTrace1(TRACE_NORMAL, CDEVICEENDPOINT0_STOP, "<Error %d> Unable to release device control",err);
		}
	OstTraceFunctionExitExt( CDEVICEENDPOINT0_STOP_EXIT, this, err );
	return err;
	}


void CDeviceEndpoint0::SendData(const TDesC8& aData)
	{
	OstTraceFunctionEntryExt( CDEVICEENDPOINT0_SENDDATA_ENTRY, this );
	iEndpoint0Writer->Write(aData, ETrue);
	OstTraceFunctionExit1( CDEVICEENDPOINT0_SENDDATA_EXIT, this );
	}

TInt CDeviceEndpoint0::SendDataSynchronous(const TDesC8& aData)
	{
	OstTraceFunctionEntryExt( CDEVICEENDPOINT0_SENDDATASYNCHRONOUS_ENTRY, this );
	TInt ret = iEndpoint0Writer->WriteSynchronous(aData, ETrue);
	OstTraceFunctionExit1( CDEVICEENDPOINT0_SENDDATASYNCHRONOUS_EXIT, this );
	return ret;
	}

CControlEndpointReader& CDeviceEndpoint0::Reader()
	{
	OstTraceFunctionEntry1( CDEVICEENDPOINT0_READER_ENTRY, this );
	OstTraceFunctionExit1( CDEVICEENDPOINT0_READER_EXIT, this );
	return *iEndpoint0Reader;
	}

	}

