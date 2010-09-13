// Copyright (c) 2004-2009 Nokia Corporation and/or its subsidiary(-ies).
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

#define __E32TEST_EXTENSION__
#include <e32std.h>
#include <e32test.h>
#include <testusbc.h>
#include "t_gml_tur_protocol.h"
#include "t_gml_tur_controller.h"
#include "protocol.h"
#include "cbulkonlytransport.h"

LOCAL_D RTest test(_L("t_gml_tur"));

_LIT(KDriverFileName,"TESTUSBC.LDD");
_LIT(KLddName,"usbc");
_LIT(KTransportThreadName,"TransportThread");
_LIT(KTransportSemName,"TransportThreadSem");

LOCAL_D const TInt KFiveDrives = 5;
LOCAL_D const TEndpointNumber KOutEp = EEndpoint1;

GLDEF_D CUsbMassStorageController* gController = NULL;

LOCAL_C void SetCBWHeader(TDes8& cbwData, TInt32 aDataTransferLength, TBool aDataIn, TUint8 aLun, TUint8 aCBLength)
	{
	//dCBWSignature
	cbwData[0] = 0x55;
	cbwData[1] = 0x53;
	cbwData[2] = 0x42;
	cbwData[3] = 0x43;
	//dCBWTag
	cbwData[4] = 0x01;
	cbwData[5] = 0x00;
	cbwData[6] = 0x00;
	cbwData[7] = 0x00;
	//dCBWDataTransferLength
	cbwData[8] = TUint8((aDataTransferLength & 0x000000FF));
	cbwData[9] = TUint8((aDataTransferLength & 0x0000FF00) >> 8);
	cbwData[10] = TUint8((aDataTransferLength & 0x00FF0000) >> 16);
	cbwData[11] = TUint8((aDataTransferLength & 0xFF000000) >> 24);
	//bmCBWFlags
	if (aDataIn)
		{
		cbwData[12] = 0x80;
		}
	else
		{
		cbwData[12] = 0x00;
		}
	//bCBWLUN
	cbwData[13] = aLun;
	//bCBWCBLength
	cbwData[14] = aCBLength;
	}
	
LOCAL_C TInt TransportThreadEntry(TAny* aPtr)
	{
	TInt err = KErrNone;
	TInt numDrives = KFiveDrives;
	
	//Create and install cleanup trap and active scheduler
	CTrapCleanup* cleanup = CTrapCleanup::New();
	if (cleanup == NULL)
		{
		return KErrNoMemory;
		}

	CActiveScheduler* sched = new CActiveScheduler;
	if (sched == NULL)
		{
		return KErrNoMemory;
		}
	CActiveScheduler::Install(sched);
	
	//Start transport
	CScsiProtocol* protocol = NULL;
	TRAP(err, protocol = CScsiProtocol::NewL());
	if (err != KErrNone)
		{
		return err;
		}
		
	CUsbMassStorageController* controller = (CUsbMassStorageController*)aPtr;
	gController = controller;
	controller->CreateL(0);
	
	MTransportBase* transport = NULL;
	TRAP(err, transport = CBulkOnlyTransport::NewL(numDrives, *controller));
	if (err != KErrNone)
		{
		return err;
		}

	controller->SetTransport(transport);
	TRAP(err, transport->InitialiseTransportL(1));

	transport->RegisterProtocol(*protocol);
	transport->Start();
	
	//Synchronize with test thread
	RSemaphore gSemThreadReady;
	gSemThreadReady.OpenGlobal(KTransportSemName);
	gSemThreadReady.Signal();
	gSemThreadReady.Close();
	
	CActiveScheduler::Start();
	
	delete transport;
	delete controller;
	delete protocol;
	delete sched;
	delete cleanup;
	
	return KErrNone;
	}
	
GLDEF_C TInt E32Main()
	{
	test.Title();
	
	TInt err;
	
	test.Start(_L("Loading ldd"));
	err = User::LoadLogicalDevice(KDriverFileName);
	test(err == KErrNone || err == KErrAlreadyExists);
	
	RDevTestUsbcClient ldd;
	err = ldd.Open(0);
	test(err == KErrNone);
	
	//ldd.ResetEndpoints();
	
	RSemaphore gSemThreadReady;
	err = gSemThreadReady.CreateGlobal(KTransportSemName, 0);
	
	CUsbMassStorageController* controller = new CUsbMassStorageController();
	test(controller != NULL);
		
	//Start transport thread.
	RThread transportThread;
	test.Next(_L("Creating transport thread, Max Lun = 4"));
	err = transportThread.Create(KTransportThreadName, TransportThreadEntry, KDefaultStackSize, NULL, (void*)controller);
	test(err == KErrNone);
	transportThread.Resume();
	
	//Synchronize with transport thread.
	gSemThreadReady.Wait();
	gSemThreadReady.Close();
	test(gController != NULL);
		
	TRequestStatus status;
	
	test.Next(_L("Writing GetMaxLun request to endpoint 0"));
	//Write GetMaxLun request to endpoint 0
	TBuf8<KRequestHdrSize> controlData;
	controlData.SetLength(KRequestHdrSize);
	controlData.FillZ();
	controlData[0] = 0xA1;
	controlData[1] = 0xFE;
	controlData[6] = 0x01;
	ldd.HostWrite(status, EEndpoint0, controlData, KRequestHdrSize);
	
	User::WaitForRequest(status);
	test.Printf(_L("status = %d"), status.Int());
	test(status.Int() == KErrNone);
	
	//Read request response
	controlData.SetLength(1);
	ldd.HostRead(status, EEndpoint0, controlData, 1);
	
	User::WaitForRequest(status);
	test(status.Int() == KErrNone);
	
	test.Printf(_L("Max LUN: %d\n"), controlData[0]);
	test(controlData[0] == KFiveDrives - 1);
	
	//Send TEST UNIT READY command
	TBuf8<KCbwLength> cbwData;
	cbwData.SetLength(KCbwLength);
	cbwData.FillZ();
	SetCBWHeader(cbwData, 6, ETrue, 0, 6);
	ldd.HostWrite(status, KOutEp, cbwData, KCbwLength);
	User::WaitForRequest(status);
	test(status.Int() == KErrNone);
	
	test.Next(_L("Writing Reset request to endpoint 0"));
	//Write Reset request to endpoint 0
	controlData.SetLength(KRequestHdrSize);
	controlData.FillZ();
	controlData[0] = 0x21;
	controlData[1] = 0xFF;
	ldd.HostWrite(status, EEndpoint0, controlData, KRequestHdrSize);
	
	User::WaitForRequest(status);
	test(status.Int() == KErrNone);
	
	User::After(3000000); //3 seconds
	test(gController->IsReset());
	test.Printf(_L("Controller got reset request\n"));
	
	TRequestStatus logonStatus;
	transportThread.Logon(logonStatus);
	TRequestStatus* statusPtr = &(controller->iStatus);
	transportThread.RequestComplete(statusPtr, KErrNone);
	//Wait for thread to die
	test.Printf(_L("Waiting for controller thread to die\n"));
	User::WaitForRequest(logonStatus);
    transportThread.Close();

	test.Printf(_L("The thread is dead, long live the thread\n"));

	ldd.Close();
	
	test.Printf(_L("Unloading ldd"));
	err = User::FreeLogicalDevice(KLddName);
	test_KErrNone(err);
	
    test.End();
    
	return 0;
	}
