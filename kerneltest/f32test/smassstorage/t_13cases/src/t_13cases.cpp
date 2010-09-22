// Copyright (c) 2004-2010 Nokia Corporation and/or its subsidiary(-ies).
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


/**
Test for the 13 cases as specified in the Bulk-Only Transport
specification secton 6.7.
*/

#include <e32std.h>
#include <e32test.h>
#include <testusbc.h>
#include "t_13cases_protocol.h"
#include "cusbmassstoragecontroller.h"
#include "cbulkonlytransport.h"

LOCAL_D RTest test(_L("T_13Cases"));

_LIT(KDriverFileName,"TESTUSBC.LDD");
_LIT(KLddName,"usbc");
_LIT(KTransportThreadName,"TransportThread");
_LIT(KTransportSemName,"TransportThreadSem");

// const TInt KCswLength = 13;

LOCAL_D const TEndpointNumber KInEp = EEndpoint2;
LOCAL_D const TEndpointNumber KOutEp = EEndpoint1;

LOCAL_D const TInt K3Seconds = 3000000;

LOCAL_C TInt TransportThreadEntry(TAny* aPtr)
	{
	TInt numDrives = 5;
	
	//Create and install cleanup trap and active scheduler
	CTrapCleanup* cleanup = CTrapCleanup::New();
	if (cleanup == NULL)
		{
		return KErrNoMemory;
		}

	CActiveScheduler* sched = new CActiveScheduler;
	if (sched == NULL)
		{
		delete cleanup;
		return KErrNoMemory;
		}
	CActiveScheduler::Install(sched);
		
	CUsbMassStorageController* controller = (CUsbMassStorageController*)aPtr;
	controller->CreateL(numDrives);
	
	TInt err = controller->Start();
	
	//Synchronize with test thread
	RSemaphore gSemThreadReady;
	gSemThreadReady.OpenGlobal(KTransportSemName);
	gSemThreadReady.Signal();
	gSemThreadReady.Close();
	
	//If start returned an error do not start the scheduler.
	if (err == KErrNone)
		{
		CActiveScheduler::Start();
		}
	
	delete controller;
	delete sched;
	delete cleanup;
	
	return KErrNone;
	}

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
	cbwData[8] = (TUint8)(aDataTransferLength & 0x000000FF);
	cbwData[9] = (TUint8)((aDataTransferLength & 0x0000FF00) >> 8);
	cbwData[10] = (TUint8)((aDataTransferLength & 0x00FF0000) >> 16);
	cbwData[11] = (TUint8)((aDataTransferLength & 0xFF000000) >> 24);
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
	
	RSemaphore gSemThreadReady;
	err = gSemThreadReady.CreateGlobal(KTransportSemName, 0);
	test(err == KErrNone);
	
	//Start transport thread.
	RThread transportThread;
	test.Next(_L("Creating transport thread"));
	CUsbMassStorageController* controller = new CUsbMassStorageController();
	test(controller != NULL);
	err = transportThread.Create(KTransportThreadName, TransportThreadEntry, KDefaultStackSize, NULL, (void*)controller);
	test(err == KErrNone);
	transportThread.Resume();
	
	//Synchronize with transport thread.
	gSemThreadReady.Wait();
	gSemThreadReady.Close();
	
	TBuf8<KCbwLength> cbwData(KCbwLength);
	TBuf8<KCswLength> cswData(KCswLength);
	TBuf8<KRequestHdrSize> controlData(KRequestHdrSize);
	TBuf8<8> data(8);
	TRequestStatus status;
	TRequestStatus notifyStatus;
	
	RTimer timer;
	timer.CreateLocal();
	
	// ***** Case 1: Hn = Dn *****
	test.Next(_L("Case 1"));
	//Send a command that requires no data transfer. dCBWDataTransferLength = 0.
	SetCBWHeader(cbwData, 0, EFalse, 0, 2);
	cbwData[15] = 0;
	cbwData[16] = 0; //No data transfer
	
	test.Printf(_L("Writing CBW\n"));
	ldd.HostWrite(status, KOutEp, cbwData, KCbwLength);
	
	User::WaitForRequest(status);
	test(status == KErrNone);
	
	//Receive and check CSW
	test.Printf(_L("Reading CSW\n"));
	ldd.HostRead(status, KInEp, cswData, KCswLength);
	
	User::WaitForRequest(status);
	test(status == KErrNone);
	
	//bCSWStatus == 0 or 1
	test(cswData[12] == 0 || cswData[12] == 1);
	
	// ***** Case 2: Hn < Di ***** /
	test.Next(_L("Case 2"));
	//Send a command that requires data transfer to the host.
	//dCBWDataTransferLength = 0.
	SetCBWHeader(cbwData, 0, EFalse, 0, 2);
	cbwData[15] = 0; //In
	cbwData[16] = 8;
	
	test.Printf(_L("Writing CBW\n"));
	ldd.HostWrite(status, KOutEp, cbwData, KCbwLength);
	
	User::WaitForRequest(status);
	test(status == KErrNone);
	
	//Receive and check CSW
	test.Printf(_L("Reading CSW\n"));
	ldd.HostRead(status, KInEp, cswData, KCswLength);
	
	User::WaitForRequest(status);
	test(status == KErrNone);
	
	//bCSWStatus == 2
	test(cswData[12] == 2);
	
	test.Printf(_L("Performing Reset Recovery\n"));
	//Perform Reset Recovery
	controlData.FillZ();
	controlData[0] = 0x21;
	controlData[1] = 0xFF;
	ldd.HostWrite(status, EEndpoint0, controlData, KRequestHdrSize);
	
	User::WaitForRequest(status);
	test(status == KErrNone);
	User::After(K3Seconds);
	
	// ***** Case 3: Hn < Do *****
	test.Next(_L("Case 3"));
	//Send a command that requires data transfer to the device.
	//dCBWDataTransferLength = 0.
	SetCBWHeader(cbwData, 0, EFalse, 0, 2);
	cbwData[15] = 1; //Out
	cbwData[16] = 8;
	
	test.Printf(_L("Writing CBW\n"));
	ldd.HostWrite(status, KOutEp, cbwData, KCbwLength);
	
	User::WaitForRequest(status);
	test(status == KErrNone);
	
	//Receive and check CSW
	test.Printf(_L("Reading CSW\n"));
	ldd.HostRead(status, KInEp, cswData, KCswLength);
	
	User::WaitForRequest(status);
	test(status == KErrNone);
	
	//bCSWStatus == 2
	test(cswData[12] == 2);
	
	test.Printf(_L("Performing Reset Recovery\n"));
	//Perform Reset Recovery
	controlData.FillZ();
	controlData[0] = 0x21;
	controlData[1] = 0xFF;
	ldd.HostWrite(status, EEndpoint0, controlData, KRequestHdrSize);
	
	User::WaitForRequest(status);
	test(status == KErrNone);
	User::After(K3Seconds);
	
	// ***** Case 4: Hi > Dn *****
	test.Next(_L("Case 4"));
	//Send a command that requires no data transfer.
	//dCBWDataTransferLength = 8, direction = in
	SetCBWHeader(cbwData, 8, ETrue, 0, 2);
	cbwData[15] = 0; //In
	cbwData[16] = 0; //no data
	
	ldd.HostEndpointStatusNotify(notifyStatus, KInEp);
	
	test.Printf(_L("Writing CBW\n"));
	ldd.HostWrite(status, KOutEp, cbwData, KCbwLength);
	
	User::WaitForRequest(status);
	test(status == KErrNone);
	
	test.Printf(_L("Reading data\n"));
	data.SetLength(8);
	ldd.HostRead(status, KInEp, data, data.Length());
	
	test.Printf(_L("Waiting for end of transfer or stall\n"));
	User::WaitForRequest(status, notifyStatus);
	test(status == KErrNone || notifyStatus == KErrNone);
	
	if (notifyStatus == KErrNone)
		{
		ldd.HostClearEndpoint(KInEp);
		}
	
	//Receive and check CSW
	test.Printf(_L("Reading CSW\n"));
	ldd.HostRead(status, KInEp, cswData, KCswLength);
	
	User::WaitForRequest(status);
	test(status == KErrNone);
	
	//bCSWStatus == 0 or 1
	test(cswData[12] == 0 || cswData[12] == 1);
	//dCSWDataResidue == 8
	test(cswData[8] == 8 || cswData[9] == 0 || cswData[10] == 0 || cswData[11] == 0);
	
	// ***** Case 5: Hi > Di *****
	test.Next(_L("Case 5"));
	//Send a command that requires 4 bytes of data transfer to the host.
	//dCBWDataTransferLength = 8, direction = in
	SetCBWHeader(cbwData, 8, ETrue, 0, 2);
	cbwData[15] = 0; //In
	cbwData[16] = 4; //4 bytes
	
	ldd.HostEndpointStatusNotify(notifyStatus, KInEp);
	
	test.Printf(_L("Writing CBW\n"));
	ldd.HostWrite(status, KOutEp, cbwData, KCbwLength);
	
	User::WaitForRequest(status);
	test(status == KErrNone);
	
	test.Printf(_L("Reading data\n"));
	data.SetLength(8);
	ldd.HostRead(status, KInEp, data, data.Length());
	
	test.Printf(_L("Waiting for end of transfer or stall\n"));
	User::WaitForRequest(status, notifyStatus);
	test(status == KErrNone || notifyStatus == KErrNone);
	
	if (notifyStatus == KErrNone)
		{
		ldd.HostClearEndpoint(KInEp);
		}
		
	//Receive and check CSW
	test.Printf(_L("Reading CSW\n"));
	ldd.HostRead(status, KInEp, cswData, KCswLength);
	
	User::WaitForRequest(status);
	test(status == KErrNone);
	
	//bCSWStatus == 0 or 1
	test(cswData[12] == 0 || cswData[12] == 1);
	//dCSWDataResidue == 8
	test(cswData[8] == 4 || cswData[9] == 0 || cswData[10] == 0 || cswData[11] == 0);
	
	// **** Case 6: Hi = Di *****
	test.Next(_L("Case 6"));
	//Send a command that requires 8 bytes of data transfer to the host.
	//dCBWDataTransferLength = 8, direction = in
	SetCBWHeader(cbwData, 8, ETrue, 0, 2);
	cbwData[15] = 0; //In
	cbwData[16] = 8; //8 bytes
	
	test.Printf(_L("Writing CBW\n"));
	ldd.HostWrite(status, KOutEp, cbwData, KCbwLength);
	
	User::WaitForRequest(status);
	test(status == KErrNone);
	
	test.Printf(_L("Reading data\n"));
	data.SetLength(8);
	ldd.HostRead(status, KInEp, data, data.Length());
	
	User::WaitForRequest(status);
	test(status == KErrNone);
	
	//Receive and check CSW
	test.Printf(_L("Reading CSW\n"));
	ldd.HostRead(status, KInEp, cswData, KCswLength);
	
	User::WaitForRequest(status);
	test(status == KErrNone);
	
	//bCSWStatus == 0 or 1
	test(cswData[12] == 0 || cswData[12] == 1);
	//dCSWDataResidue == 0
	test(cswData[8] == 0 || cswData[9] == 0 || cswData[10] == 0 || cswData[11] == 0);
	
	// **** Case 7: Hi < Di *****
	test.Next(_L("Case 7"));
	//Send a command that requires 8 bytes of data transfer to the host.
	//dCBWDataTransferLength = 4, direction = in
	SetCBWHeader(cbwData, 4, ETrue, 0, 2);
	cbwData[15] = 0; //In
	cbwData[16] = 8; //8 bytes
	
	ldd.HostEndpointStatusNotify(notifyStatus, KInEp);
	
	test.Printf(_L("Writing CBW\n"));
	ldd.HostWrite(status, KOutEp, cbwData, KCbwLength);
	
	User::WaitForRequest(status);
	test(status == KErrNone);
	
	test.Printf(_L("Reading data\n"));
	data.SetLength(4);
	ldd.HostRead(status, KInEp, data, data.Length());
	
	test.Printf(_L("Waiting for end of transfer or stall\n"));
	User::WaitForRequest(status, notifyStatus);
	test(status == KErrNone || notifyStatus == KErrNone);
	
	if (notifyStatus == KErrNone)
		{
		ldd.HostClearEndpoint(KInEp);
		}
	
	//Receive and check CSW
	test.Printf(_L("Reading CSW\n"));
	ldd.HostRead(status, KInEp, cswData, KCswLength);
	
	User::WaitForRequest(status);
	test(status == KErrNone);
	
	//bCSWStatus == 2
	test(cswData[12] == 2);
	
	test.Printf(_L("Performing Reset Recovery\n"));
	//Perform Reset Recovery
	controlData.FillZ();
	controlData[0] = 0x21;
	controlData[1] = 0xFF;
	ldd.HostWrite(status, EEndpoint0, controlData, KRequestHdrSize);
	
	User::WaitForRequest(status);
	test(status == KErrNone);
	User::After(K3Seconds);
	
	// **** Case 8: Hi <> Do *****
	test.Next(_L("Case 8"));
	//Send a command that requires 8 bytes of data transfer to the host.
	//dCBWDataTransferLength = 8, direction = in
	SetCBWHeader(cbwData, 8, ETrue, 0, 2);
	cbwData[15] = 1; //Out
	cbwData[16] = 8; //8 bytes
	
	ldd.HostEndpointStatusNotify(notifyStatus, KInEp);
	
	test.Printf(_L("Writing CBW\n"));
	ldd.HostWrite(status, KOutEp, cbwData, KCbwLength);
	
	User::WaitForRequest(status);
	test(status == KErrNone);
	
	test.Printf(_L("Reading data\n"));
	data.SetLength(8);
	ldd.HostRead(status, KInEp, data, data.Length());
	
	test.Printf(_L("Waiting for end of transfer or stall\n"));
	User::WaitForRequest(status, notifyStatus);
	test(status == KErrNone || notifyStatus == KErrNone);
		
	if (notifyStatus == KErrNone)
		{
		ldd.HostClearEndpoint(KInEp);
		}

	//Receive and check CSW
	test.Printf(_L("Reading CSW\n"));
	ldd.HostRead(status, KInEp, cswData, KCswLength);
	
	User::WaitForRequest(status);
	test(status == KErrNone);
	
	//bCSWStatus == 2
	test(cswData[12] == 2);
	
	test.Printf(_L("Performing Reset Recovery\n"));
	//Perform Reset Recovery
	controlData.FillZ();
	controlData[0] = 0x21;
	controlData[1] = 0xFF;
	ldd.HostWrite(status, EEndpoint0, controlData, KRequestHdrSize);
	
	User::WaitForRequest(status);
	test(status == KErrNone);
	User::After(K3Seconds);

	// ***** Case 9: Ho > Dn *****
	test.Next(_L("Case 9"));
	//Send a command that requires no data transfer.
	//dCBWDataTransferLength = 8, direction = out
	SetCBWHeader(cbwData, 8, EFalse, 0, 2);
	cbwData[15] = 1; //Out
	cbwData[16] = 0; //no data
	
	ldd.HostEndpointStatusNotify(notifyStatus, KOutEp);
	
	test.Printf(_L("Writing CBW\n"));
	ldd.HostWrite(status, KOutEp, cbwData, KCbwLength);
	
	User::WaitForRequest(status);
	test(status == KErrNone);
	
	test.Printf(_L("Writing data\n"));
	data.SetLength(8);
	ldd.HostWrite(status, KOutEp, data, data.Length());
	
	test.Printf(_L("Waiting for end of transfer or stall\n"));
	User::WaitForRequest(status, notifyStatus);
	test(status == KErrNone || notifyStatus == KErrNone);
	
	if (notifyStatus == KErrNone)
		{
		ldd.HostClearEndpoint(KOutEp);
		}
	
	//Receive and check CSW
	test.Printf(_L("Reading CSW\n"));
	ldd.HostRead(status, KInEp, cswData, KCswLength);
	
	User::WaitForRequest(status);
	test(status == KErrNone);
	
	//bCSWStatus == 0 or 1
	test(cswData[12] == 0 || cswData[12] == 1);
	//dCSWDataResidue == 8
	test(cswData[8] == 8 || cswData[9] == 0 || cswData[10] == 0 || cswData[11] == 0);
	
	// ***** Case 10: Ho <> Di *****
	test.Next(_L("Case 10"));
	//Send a command that requires 8 bytes of data transfer to the host.
	//dCBWDataTransferLength = 8, direction = out
	SetCBWHeader(cbwData, 8, EFalse, 0, 2);
	cbwData[15] = 0; //In
	cbwData[16] = 8; //8
	
	ldd.HostEndpointStatusNotify(notifyStatus, KOutEp);
	
	test.Printf(_L("Writing CBW\n"));
	ldd.HostWrite(status, KOutEp, cbwData, KCbwLength);
	
	User::WaitForRequest(status);
	test(status == KErrNone);
	
	test.Printf(_L("Writing data\n"));
	data.SetLength(8);
	ldd.HostWrite(status, KOutEp, data, data.Length());
	
	test.Printf(_L("Waiting for end of transfer or stall\n"));
	User::WaitForRequest(status, notifyStatus);
	test(status == KErrNone || notifyStatus == KErrNone);
	
	if (notifyStatus == KErrNone)
		{
		ldd.HostClearEndpoint(KOutEp);
		}
	
	//Receive and check CSW
	test.Printf(_L("Reading CSW\n"));
	ldd.HostRead(status, KInEp, cswData, KCswLength);
	
	User::WaitForRequest(status);
	test(status == KErrNone);
	
	//bCSWStatus == 2
	test(cswData[12] == 2);
	
	test.Printf(_L("Performing Reset Recovery\n"));
	//Perform Reset Recovery
	controlData.FillZ();
	controlData[0] = 0x21;
	controlData[1] = 0xFF;
	ldd.HostWrite(status, EEndpoint0, controlData, KRequestHdrSize);
	
	User::WaitForRequest(status);
	test(status == KErrNone);
	User::After(K3Seconds);
	
	// ***** Case 11: Ho > Do *****
	test.Next(_L("Case 11"));
	//Send a command that requires 4 bytes of data transfer to the device.
	//dCBWDataTransferLength = 8, direction = out
	SetCBWHeader(cbwData, 8, EFalse, 0, 2);
	cbwData[15] = 1; //Out
	cbwData[16] = 4; //4 bytes
	
	ldd.HostEndpointStatusNotify(notifyStatus, KOutEp);
	
	test.Printf(_L("Writing CBW\n"));
	ldd.HostWrite(status, KOutEp, cbwData, KCbwLength);
	
	User::WaitForRequest(status);
	test(status == KErrNone);
	
	test.Printf(_L("Writing data\n"));
	data.SetLength(8);
	ldd.HostWrite(status, KOutEp, data, data.Length());
	
	//Out endpoint could be stalled
	test.Printf(_L("Waiting for end of transfer or stall\n"));
	User::WaitForRequest(status, notifyStatus);
	test(status == KErrNone || notifyStatus == KErrNone);
	
	if (notifyStatus == KErrNone)
		{
		ldd.HostClearEndpoint(KOutEp);
		}
	
	//Receive and check CSW
	test.Printf(_L("Reading CSW\n"));
	ldd.HostRead(status, KInEp, cswData, KCswLength);
	
	User::WaitForRequest(status);
	test(status == KErrNone);
	
	//bCSWStatus == 0 or 1
	test(cswData[12] == 0 || cswData[12] == 1);
	//dCSWDataResidue == 4
	test(cswData[8] == 4 || cswData[9] == 0 || cswData[10] == 0 || cswData[11] == 0);
	
	// ***** Case 12: Ho = Do *****
	test.Next(_L("Case 12"));
	//Send a command that requires 8 bytes of data transfer to the device.
	//dCBWDataTransferLength = 8, direction = out
	SetCBWHeader(cbwData, 8, EFalse, 0, 2);
	cbwData[15] = 1; //Out
	cbwData[16] = 8; //8 bytes
	
	test.Printf(_L("Writing CBW\n"));
	ldd.HostWrite(status, KOutEp, cbwData, KCbwLength);
	
	User::WaitForRequest(status);
	test(status == KErrNone);
	
	test.Printf(_L("Writing data\n"));
	data.SetLength(8);
	ldd.HostWrite(status, KOutEp, data, data.Length());
	
	User::WaitForRequest(status);
	test(status == KErrNone);
	
	//Receive and check CSW
	test.Printf(_L("Reading CSW\n"));
	ldd.HostRead(status, KInEp, cswData, KCswLength);
	
	User::WaitForRequest(status);
	test(status == KErrNone);
	
	//bCSWStatus == 0 or 1
	test(cswData[12] == 0 || cswData[12] == 1);
	//dCSWDataResidue == 0
	test(cswData[8] == 0 || cswData[9] == 0 || cswData[10] == 0 || cswData[11] == 0);
	
	// ***** Case 13: Ho < Do *****
	test.Next(_L("Case 13"));
	//Send a command that requires 8 bytes of data transfer to the device.
	//dCBWDataTransferLength = 4, direction = out
	SetCBWHeader(cbwData, 4, EFalse, 0, 2);
	cbwData[15] = 1; //Out
	cbwData[16] = 8; //8 bytes
	
	ldd.HostEndpointStatusNotify(notifyStatus, KOutEp);
	
	test.Printf(_L("Writing CBW\n"));
	ldd.HostWrite(status, KOutEp, cbwData, KCbwLength);
	
	User::WaitForRequest(status);
	test(status == KErrNone);
	
	test.Printf(_L("Writing data\n"));
	data.SetLength(8);
	ldd.HostWrite(status, KOutEp, data, data.Length());
	
	test.Printf(_L("Waiting for end of transfer or stall\n"));
	User::WaitForRequest(status, notifyStatus);
	test(status == KErrNone || notifyStatus == KErrNone);
	
	if (notifyStatus == KErrNone)
		{
		ldd.HostClearEndpoint(KOutEp);
		}
	
	//Receive and check CSW
	test.Printf(_L("Reading CSW\n"));
	ldd.HostRead(status, KInEp, cswData, KCswLength);
	
	User::WaitForRequest(status);
	test(status == KErrNone);
	
	//bCSWStatus == 2
	test(cswData[12] == 2);
	
	test.Printf(_L("Performing Reset Recovery\n"));
	//Perform Reset Recovery
	controlData.FillZ();
	controlData[0] = 0x21;
	controlData[1] = 0xFF;
	ldd.HostWrite(status, EEndpoint0, controlData, KRequestHdrSize);
	
	// ***** End of 13 cases *****
	
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
	test(err == KErrNone);
	
	timer.Close();
	
    test.End();
    
	return 0;
	}
	
