// Copyright (c) 2003-2010 Nokia Corporation and/or its subsidiary(-ies).
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
// e32test/device/t_usbapi.cpp
// Overview:
// USB API Test Program (a standalone USB test program).
// API Information:
// Details:
// - Query whether the platform is operating HS (or it is connected to a HS host) or not,
// and executes the appropiate tests in each case (see RunTests() for the actual code,
// state machine enclosed for clarity):
// - Load and open an EUSBC device driver (logical device)
// - Setup the USB interface: query device capabilities, setup interface.
// - Test allocating DMA and double buffering resources with
// AllocateEndpointResource results in their use being correctly reported by
// QueryEndpointResourceUse
// - Test descriptor manipulation: validate the device, configuration,
// interface, alternate interface, endpoint and string descriptor
// manipulation.
// HS: device_qualifier and other_speed_configuation descriptors.
// - Check and validate the EndpointZeroMaxPacketSizes.
// - Quick test that calling the following APIs doesn't generate errors: device
// control, AlternateDeviceStatusNotify, EndpointStatusNotify
// - Test HaltEndpoint and ClearHaltEndpoint correctly result in endpoint
// status being reported as stalled/not stalled.
// - Test OTG extensions: OTG descriptor manipulations; set/get OTG feature
// - Close and free the logical device.
// Platforms/Drives/Compatibility:
// All.
// Assumptions/Requirement/Pre-requisites:
// Failures and causes:
// Base Port information:
// 
//


#include <e32test.h>
#include <e32debug.h>
#include <hal.h>
#include <d32usbc.h>
#include <d32otgdi.h>

#include "t_usblib.h"
#include "OstTraceDefinitions.h"
#ifdef OST_TRACE_COMPILER_IN_USE
#include "t_usbapiTraces.h"
#endif


// --- Local Top Level Variables

static RTest test(_L("T_USBAPI"));
static RDevUsbcClient gPort;
static RUsbOtgDriver gOTG;
static TBool gSupportsOtg;
static TBool gSupportsHighSpeed;
static TBool gSupportResouceAllocationV2;
static TBool gUsingHighSpeed;
static TBool gSoak;
static TChar gKeychar = 'a';

// Store the actual endpoint number(s) of our alternate interface
static TInt INT_IN_ep = -1;

_LIT(KUsbLddFilename, "eusbc");
_LIT(KOtgdiLddFilename, "otgdi");
_LIT(KUsbDeviceName, "Usbc");


// --- Local Constants

static const TInt KUsbDesc_SizeOffset = 0;
static const TInt KUsbDesc_TypeOffset = 1;

static const TInt KDevDesc_SpecOffset = 2;
static const TInt KDevDesc_DevClassOffset = 4;
static const TInt KDevDesc_DevSubClassOffset = 5;
static const TInt KDevDesc_DevProtocolOffset = 6;
static const TInt KDevDesc_Ep0SizeOffset = 7;
static const TInt KDevDesc_VendorIdOffset = 8;
static const TInt KDevDesc_ProductIdOffset = 10;
static const TInt KDevDesc_DevReleaseOffset = 12;

static const TInt KConfDesc_AttribOffset = 7;
static const TInt KConfDesc_MaxPowerOffset = 8;

static const TInt KIfcDesc_SettingOffset = 2;
static const TInt KIfcDesc_ProtocolOffset = 7;

static const TInt KEpDesc_PacketSizeOffset = 4;
static const TInt KEpDesc_IntervalOffset = 6;
static const TInt KEpDesc_SynchAddressOffset = 8;


//
// Helper.
//
static TEndpointState QueryEndpointState(TEndpointNumber aEndpoint)
	{
	TEndpointState ep_state = EEndpointStateUnknown;
	TInt r = gPort.EndpointStatus(aEndpoint, ep_state);
	test(r == KErrNone);
	test.Printf(_L("Endpoint %d state: %s\n"), aEndpoint,
				(ep_state == EEndpointStateNotStalled) ? _S("Not stalled") :
				((ep_state == EEndpointStateStalled) ? _S("Stalled") :
				 _S("Unknown...")));
	OstTraceExt2(TRACE_NORMAL, QUERYENDPOINTSTATE_QUERYENDPOINTSTATE, "Endpoint %d state: %s\n", aEndpoint,
				(ep_state == EEndpointStateNotStalled) ? _L("Not stalled") :
				((ep_state == EEndpointStateStalled) ? _L("Stalled") :
				 _L("Unknown...")));
	return ep_state;
	}


// --- Class CActiveKeypressNotifier

class CActiveKeypressNotifier : public CActive
	{
public:
	static CActiveKeypressNotifier* NewL(CConsoleBase* aConsole);
	~CActiveKeypressNotifier();
	void RequestCharacter();
	void ProcessKeyPressL(TChar aChar);
private:
	virtual void DoCancel();
	virtual void RunL();
	CActiveKeypressNotifier(CConsoleBase* aConsole);
	void ConstructL() {};
private:
	CConsoleBase* iConsole;
	};


CActiveKeypressNotifier* CActiveKeypressNotifier::NewL(CConsoleBase* aConsole)
	{
	CActiveKeypressNotifier* self = new (ELeave) CActiveKeypressNotifier(aConsole);
	CleanupStack::PushL(self);
	self->ConstructL();
	CActiveScheduler::Add(self);
	CleanupStack::Pop();
	return self;
	}


CActiveKeypressNotifier::CActiveKeypressNotifier(CConsoleBase* aConsole)
	: CActive(EPriorityNormal), iConsole(aConsole)
	{}


CActiveKeypressNotifier::~CActiveKeypressNotifier()
	{
	Cancel();												// base class cancel -> calls our DoCancel
	}


void CActiveKeypressNotifier::RunL()
	{
	gKeychar = (static_cast<TChar>(iConsole->KeyCode()));
	RequestCharacter();
	}


void CActiveKeypressNotifier::DoCancel()
	{
	iConsole->ReadCancel();
	}


void CActiveKeypressNotifier::RequestCharacter()
	{
	// A request is issued to the CConsoleBase to accept a character from the keyboard.
	if (IsActive())
		{
		return;
		}
	iConsole->Read(iStatus);
	SetActive();
	}


// --- Actual Test Functions

// 2nd Thread helper function
static TInt TestThreadFunction(TAny* aPtr)
	{
	RThread* other = static_cast<RThread*>(aPtr);
	RDevUsbcClient port = gPort;
	// Now try to duplicate the USB channel handle
	TInt r = port.Duplicate(*other);
 	// Wait for 1 second
 	User::After(1000000);
	return r;
	}


static void OpenChannel()
	{
	test.Start(_L("Open Channel"));

	test.Next(_L("Load USB LDD"));
	TInt r = User::LoadLogicalDevice(KUsbLddFilename);
	test(r == KErrNone || r == KErrAlreadyExists);

	RDevUsbcClient port1;
	test.Next(_L("Open local USB channel 1"));
	r = port1.Open(0);
	test(r == KErrNone);

	test.Next(_L("Open global USB channel"));
	r = gPort.Open(0);
	test(r == KErrNone);

	RDevUsbcClient port2;
	test.Next(_L("Open local USB channel 2"));
	r = port2.Open(0);
	test(r == KErrNone);

	test.Next(_L("Close USB channel 1"));
	port1.Close();

	RDevUsbcClient port3;
	test.Next(_L("Open local USB channel 3"));
	r = port3.Open(0);
	test(r == KErrNone);

	test.Next(_L("Close USB channel 2"));
	port2.Close();

	test.Next(_L("Close USB channel 3"));
	port3.Close();

	// Check for OTG support
	TBuf8<KUsbDescSize_Otg> otg_desc;
	r = gPort.GetOtgDescriptor(otg_desc);
	test(r == KErrNotSupported || r == KErrNone);
	gSupportsOtg = (r != KErrNotSupported) ? ETrue : EFalse;

	// On an OTG device we have to start the OTG driver, otherwise the Client
	// stack will remain disabled forever.
	if (gSupportsOtg)
		{
		test.Printf(_L("Running on OTG device: loading OTG driver\n"));
		OstTrace0(TRACE_NORMAL, OPENCHANNEL_OPENCHANNEL, "Running on OTG device: loading OTG driver\n");
		test.Next(_L("Load OTG LDD"));
		r = User::LoadLogicalDevice(KOtgdiLddFilename);
		test((r == KErrNone) || (r == KErrAlreadyExists));

		test.Next(_L("Open OTG channel"));
		r = gOTG.Open();
		test(r == KErrNone);

		test.Next(_L("Start OTG stack"));
		r = gOTG.StartStacks();
		test(r == KErrNone);
		}

	// Try duplicating channel handle in a second thread
	// (which should not work because we don't support it)

	test.Next(_L("Create 2nd Thread"));
	RThread me;
 	TThreadId me_id = me.Id();
	// We need to open the RThread object, otherwise we'll only get the
	// 'special' handle 0xFFFF8001.
	test(me.Open(me_id) == KErrNone);
	RThread test_thread;
	TBuf<17> name = _L("tusbapitestthread");
	test(test_thread.Create(name, TestThreadFunction, 0x1000, NULL, &me) == KErrNone);
	test.Next(_L("Logon to 2nd Thread"));
	TRequestStatus stat;
	test_thread.Logon(stat);
	test(stat == KRequestPending);
	test_thread.Resume();
	test.Next(_L("Wait for 2nd Thread to exit"));
	User::WaitForRequest(stat);
	// Check correct return value of RDevUsbcClient::Duplicate()
	test(stat == KErrAccessDenied);
	test.Next(_L("Close 2nd Thread"));
	test_thread.Close();

	test.End();
	}


static void TestResourceAllocationV1()
	{
	test.Start(_L("Test Endpoint Resource Allocation"));

	test.Next(_L("Request DMA resource"));
	const TInt dma = gPort.AllocateEndpointResource(EEndpoint1, EUsbcEndpointResourceDMA);
	TBool res = gPort.QueryEndpointResourceUse(EEndpoint1, EUsbcEndpointResourceDMA);
	test.Printf(_L("DMA on endpoint 1 %s\n"),
				res ? _S("now allocated") : _S("not allocated"));
	OstTraceExt1(TRACE_NORMAL, TESTRESOURCEALLOCATIONV1_TESTRESOURCEALLOCATIONV1, "DMA on endpoint 1 %s\n",
				res ? _L("now allocated") : _L("not allocated"));
	if (dma == KErrNone)
		// Only if DMA resource was successfully allocated should we expect truth here:
		test(res);
	else
		test(!res);

	test.Next(_L("Request Double Buffering resource"));
	const TInt db = gPort.AllocateEndpointResource(EEndpoint1, EUsbcEndpointResourceDoubleBuffering);
	res = gPort.QueryEndpointResourceUse(EEndpoint1, EUsbcEndpointResourceDoubleBuffering);
	test.Printf(_L("Double Buffering on endpoint 1 %s\n"),
				res ? _S("now allocated") : _S("not allocated"));
	OstTraceExt1(TRACE_NORMAL, TESTRESOURCEALLOCATIONV1_TESTRESOURCEALLOCATIONV1_DUP01, "Double Buffering on endpoint 1 %s\n",
				res ? _L("now allocated") : _L("not allocated"));
	if (db == KErrNone)
		// Only if DB resource was successfully allocated should we expect truth here:
		test(res);
	else
		test(!res);

	test.Next(_L("Deallocate Double Buffering resource"));
	TInt r = gPort.DeAllocateEndpointResource(EEndpoint1, EUsbcEndpointResourceDoubleBuffering);
	// Whether DB is dynamic or permanent - deallocation (if supported) should always return success:
	if (db == KErrNone)
		test(r == KErrNone);
	else
		test(r != KErrNone);
	res = gPort.QueryEndpointResourceUse(EEndpoint1, EUsbcEndpointResourceDoubleBuffering);
	test.Printf(_L("Double Buffering on endpoint 1 %s\n"),
				res ? _S("still allocated") : _S("not (longer) allocated"));
	OstTraceExt1(TRACE_NORMAL, TESTRESOURCEALLOCATIONV1_TESTRESOURCEALLOCATIONV1_DUP02, "Double Buffering on endpoint 1 %s\n",
				res ? _L("still allocated") : _L("not (longer) allocated"));

	test.Next(_L("Deallocate DMA resource"));
	r = gPort.DeAllocateEndpointResource(EEndpoint1, EUsbcEndpointResourceDMA);
	// Whether DMA is dynamic or permanent - deallocation (if supported) should always return success:
	if (dma == KErrNone)
		test(r == KErrNone);
	else
		test(r != KErrNone);
	res = gPort.QueryEndpointResourceUse(EEndpoint1, EUsbcEndpointResourceDMA);
	test.Printf(_L("DMA on endpoint 1 %s\n"),
				res ? _S("still allocated") : _S("not (longer) allocated"));
	OstTraceExt1(TRACE_NORMAL, TESTRESOURCEALLOCATIONV1_TESTRESOURCEALLOCATIONV1_DUP03, "DMA on endpoint 1 %s\n",
				res ? _L("still allocated") : _L("not (longer) allocated"));

	test.End();
	}


static void SetupInterface()
	{
	test.Start(_L("Query USB device caps and set up interface"));

	// Device caps
	test.Next(_L("Query USB device caps"));
	TUsbDeviceCaps d_caps;
	TInt r = gPort.DeviceCaps(d_caps);
	test(r == KErrNone);
	TInt n = d_caps().iTotalEndpoints;

	// Global variable - we'll need this value later
	gSupportsHighSpeed = d_caps().iHighSpeed;
	gSupportResouceAllocationV2 = (d_caps().iFeatureWord1 & KUsbDevCapsFeatureWord1_EndpointResourceAllocV2);
	
	test.Printf(_L("### USB device capabilities:\n"));
	OstTrace0(TRACE_NORMAL, SETUPINTERFACE_SETUPINTERFACE, "### USB device capabilities:\n");
	test.Printf(_L("Number of endpoints:                        %d\n"), n);
	OstTrace1(TRACE_NORMAL, SETUPINTERFACE_SETUPINTERFACE_DUP01, "Number of endpoints:                        %d\n", n);
	test.Printf(_L("Supports Software-Connect:                  %s\n"),
				d_caps().iConnect ? _S("yes") : _S("no"));
	OstTraceExt1(TRACE_NORMAL, SETUPINTERFACE_SETUPINTERFACE_DUP02, "Supports Software-Connect:                  %s\n",
				d_caps().iConnect ? _L("yes") : _L("no"));
	test.Printf(_L("Device is Self-Powered:                     %s\n"),
				d_caps().iSelfPowered ? _S("yes") : _S("no"));
	OstTraceExt1(TRACE_NORMAL, SETUPINTERFACE_SETUPINTERFACE_DUP03, "Device is Self-Powered:                     %s\n",
				d_caps().iSelfPowered ? _L("yes") : _L("no"));
	test.Printf(_L("Supports Remote-Wakeup:                     %s\n"),
				d_caps().iRemoteWakeup ? _S("yes") : _S("no"));
	OstTraceExt1(TRACE_NORMAL, SETUPINTERFACE_SETUPINTERFACE_DUP04, "Supports Remote-Wakeup:                     %s\n",
				d_caps().iRemoteWakeup ? _L("yes") : _L("no"));
	test.Printf(_L("Supports High-speed:                        %s\n"),
				gSupportsHighSpeed ? _S("yes") : _S("no"));
	OstTraceExt1(TRACE_NORMAL, SETUPINTERFACE_SETUPINTERFACE_DUP05, "Supports High-speed:                        %s\n",
				gSupportsHighSpeed ? _L("yes") : _L("no"));
	test.Printf(_L("Supports OTG:                               %s\n"),
				gSupportsOtg ? _S("yes") : _S("no"));
	OstTraceExt1(TRACE_NORMAL, SETUPINTERFACE_SETUPINTERFACE_DUP06, "Supports OTG:                               %s\n",
				gSupportsOtg ? _L("yes") : _L("no"));
	test.Printf(_L("Supports unpowered cable detection:         %s\n"),
				(d_caps().iFeatureWord1 & KUsbDevCapsFeatureWord1_CableDetectWithoutPower) ?
				_S("yes") : _S("no"));
	OstTraceExt1(TRACE_NORMAL, SETUPINTERFACE_SETUPINTERFACE_DUP07, "Supports unpowered cable detection:         %s\n",
				(d_caps().iFeatureWord1 & KUsbDevCapsFeatureWord1_CableDetectWithoutPower) ?
				_L("yes") : _L("no"));
	test.Printf(_L("Supports endpoint resource alloc scheme V2: %s\n"),
				gSupportResouceAllocationV2 ? _S("yes") : _S("no"));
	OstTraceExt1(TRACE_NORMAL, SETUPINTERFACE_SETUPINTERFACE_DUP08, "Supports endpoint resource alloc scheme V2: %s\n",
				gSupportResouceAllocationV2 ? _L("yes") : _L("no"));

	test(n >= 2);
	test.Printf(_L("(Device has sufficient endpoints.)\n"));
	OstTrace0(TRACE_NORMAL, SETUPINTERFACE_SETUPINTERFACE_DUP09, "(Device has sufficient endpoints.)\n");

	// Endpoint caps
	test.Next(_L("Query USB endpoint caps"));
	TUsbcEndpointData data[KUsbcMaxEndpoints];
	TPtr8 dataptr(reinterpret_cast<TUint8*>(data), sizeof(data), sizeof(data));
	r = gPort.EndpointCaps(dataptr);
	test(r == KErrNone);

	test.Printf(_L("### USB device endpoint capabilities:\n"));
	OstTrace0(TRACE_NORMAL, SETUPINTERFACE_SETUPINTERFACE_DUP10, "### USB device endpoint capabilities:\n");
	for (TInt i = 0; i < n; i++)
		{
		const TUsbcEndpointCaps* caps = &data[i].iCaps;
		test.Printf(_L("Endpoint: SizeMask = 0x%08x TypeDirMask = 0x%08x\n"),
					caps->iSizes, caps->iTypesAndDir);
		OstTraceExt2(TRACE_NORMAL, SETUPINTERFACE_SETUPINTERFACE_DUP11, "Endpoint: SizeMask = 0x%08x TypeDirMask = 0x%08x\n",
					caps->iSizes, caps->iTypesAndDir);
		if (caps->iHighBandwidth)
			{
			test.Printf(_L("  (high-speed, high bandwidth endpoint)\n"));
			OstTrace0(TRACE_NORMAL, SETUPINTERFACE_SETUPINTERFACE_DUP12, "  (high-speed, high bandwidth endpoint)\n");
			// Must be HS Int or Iso ep
			test(gSupportsHighSpeed);
			test(caps->iTypesAndDir & (KUsbEpTypeIsochronous | KUsbEpTypeInterrupt));
			}
		}

	test.Next(_L("Looking for suitable endpoints"));
	// Set the active interface
	TUsbcInterfaceInfoBuf ifc;
	TInt ep_found = 0;
	TBool foundBulkIN = EFalse;
	TBool foundBulkOUT = EFalse;
	for (TInt i = 0; i < n; i++)
		{
		const TUsbcEndpointCaps* caps = &data[i].iCaps;
		const TInt mps = caps->MaxPacketSize();
		if (!foundBulkIN &&
			(caps->iTypesAndDir & (KUsbEpTypeBulk | KUsbEpDirIn)) ==
			(KUsbEpTypeBulk | KUsbEpDirIn))
			{
			// EEndpoint1 is going to be our TX (IN, write) endpoint
			ifc().iEndpointData[0].iType = KUsbEpTypeBulk;
			ifc().iEndpointData[0].iDir  = KUsbEpDirIn;
			ifc().iEndpointData[0].iSize = mps;
			foundBulkIN = ETrue;
			if (++ep_found == 2)
				break;
			}
		else if (!foundBulkOUT &&
			(caps->iTypesAndDir & (KUsbEpTypeBulk | KUsbEpDirOut)) ==
				 (KUsbEpTypeBulk | KUsbEpDirOut))
			{
			// EEndpoint2 is going to be our RX (OUT, read) endpoint
			ifc().iEndpointData[1].iType = KUsbEpTypeBulk;
			ifc().iEndpointData[1].iDir  = KUsbEpDirOut;
			ifc().iEndpointData[1].iSize = mps;
			foundBulkOUT = ETrue;
			if (++ep_found == 2)
				break;
			}
		}
	test(ep_found == 2);

	test.Next(_L("Setting up main interface"));
	_LIT16(string, "T_USBAPI Test Interface (Setting 0)");
	ifc().iString = const_cast<TDesC16*>(&string);
	ifc().iTotalEndpointsUsed = 2;
	ifc().iClass.iClassNum	  = 0xff;
	ifc().iClass.iSubClassNum = 0xff;
	ifc().iClass.iProtocolNum = 0xff;
	// Set up the interface.
	r = gPort.SetInterface(0, ifc);
	test(r == KErrNone);

	TInt ifc_no = -1;
	r = gPort.GetAlternateSetting(ifc_no);
	test(r == KErrUsbDeviceNotConfigured);

	// Some UDCs won't allow endpoint resource manipulation once the hardware has been
	// configured and turned on. So we do it here & now:
	if (!gSupportResouceAllocationV2)
		{
		TestResourceAllocationV1();
		}

	// On the other hand, since some UDCs won't let us test many features which require
	// register access until the USB hardware is powered up (and because it might start
	// out unpowered), we should turn it on here explicitly.
	// (It will be turned off automatically by the PIL after all tests have been run,
	// when the interface gets deleted.)
	test.Next(_L("Powering up UDC (1)"));
	r = gPort.PowerUpUdc();
	if (!gSupportsOtg)
		{
		test(r == KErrNone);
		}
	else
		{
		test((r == KErrNone) || (r == KErrNotReady));
		}
	if (gSupportsOtg && (r == KErrNotReady))
		{
		test.Printf(_L("OTG device but not connected to Host, stopping subtest here.\n"));
		OstTrace0(TRACE_NORMAL, SETUPINTERFACE_SETUPINTERFACE_DUP13, "OTG device but not connected to Host, stopping subtest here.\n");
		test.End();
		return;
		}
	// The board might be attached to a PC with HS controller, thus enabling us
	// to test some HS-specific features. For that to work we have to connect
	// the board to the PC. The "Found new device" box that may pop up on the PC
	// in response to this can be ignored (i.e. just closed).
	test.Next(_L("Connecting to Host (1)"));
	r = gPort.DeviceConnectToHost();
	test(r == KErrNone);
 	// Suspend thread to let things get stable on the bus.
	test.Printf(_L("Waiting a short moment..."));
	OstTrace0(TRACE_NORMAL, SETUPINTERFACE_SETUPINTERFACE_DUP14, "Waiting a short moment...");
	User::After(2000000);
	test.Printf(_L(" done.\n"));
	OstTrace0(TRACE_NORMAL, SETUPINTERFACE_SETUPINTERFACE_DUP15, " done.\n");

	// Check the speed of the physical connection (if any).
	gUsingHighSpeed = gPort.CurrentlyUsingHighSpeed();
	if (gUsingHighSpeed)
		{
		test(gSupportsHighSpeed);							// sane?
		test.Printf(_L("---> USB High-speed Testing\n"));
		OstTrace0(TRACE_NORMAL, SETUPINTERFACE_SETUPINTERFACE_DUP16, "---> USB High-speed Testing\n");
		}
	else
		{
		test.Printf(_L("---> USB Full-speed Testing\n"));
		OstTrace0(TRACE_NORMAL, SETUPINTERFACE_SETUPINTERFACE_DUP17, "---> USB Full-speed Testing\n");
		}

	// By pulling down the interface/connection and bringing them up again we
	// simulate a starting/stopping of the USB service by a control app.

	test.Next(_L("Disconnecting from Host"));
	r = gPort.DeviceDisconnectFromHost();
	test(r == KErrNone);

	test.Next(_L("Releasing interface"));
	r = gPort.ReleaseInterface(0);
	test(r == KErrNone);

	if (gSupportResouceAllocationV2)
		{
		test.Next(_L("setting resource allocation info on endpoint 1 with resource allocation scheme v2"));
		ifc().iEndpointData[0].iFeatureWord1 |= KUsbcEndpointInfoFeatureWord1_DMA;
		ifc().iEndpointData[0].iFeatureWord1 |= KUsbcEndpointInfoFeatureWord1_DoubleBuffering;
		}

	test.Next(_L("Setting interface"));
	r = gPort.SetInterface(0, ifc);
	test(r == KErrNone);

 	// Suspend thread before connecting again.
	test.Printf(_L("Waiting a short moment..."));
	OstTrace0(TRACE_NORMAL, SETUPINTERFACE_SETUPINTERFACE_DUP18, "Waiting a short moment...");
	User::After(1000000);
	test.Printf(_L(" done.\n"));
	OstTrace0(TRACE_NORMAL, SETUPINTERFACE_SETUPINTERFACE_DUP19, " done.\n");

	test.Next(_L("Powering up UDC (2)"));
	r = gPort.PowerUpUdc();
	if (!gSupportsOtg)
		{
		test(r == KErrNone);
		}
	else
		{
		test((r == KErrNone) || (r == KErrNotReady));
		}
	if (gSupportsOtg && (r == KErrNotReady))
		{
		test.Printf(_L("OTG device but not connected to Host, stopping subtest here.\n"));
		OstTrace0(TRACE_NORMAL, SETUPINTERFACE_SETUPINTERFACE_DUP20, "OTG device but not connected to Host, stopping subtest here.\n");
		test.End();
		return;
		}

	test.Next(_L("Connecting to Host (2)"));
	r = gPort.DeviceConnectToHost();
	test(r == KErrNone);
	// Suspend thread to let things get stable on the bus.
	User::After(2000000);

	if (gSupportResouceAllocationV2)
		{
			test.Next(_L("endpoint 1 resource allocation results(resource allocation V2)"));
			TBool res = gPort.QueryEndpointResourceUse(EEndpoint1, EUsbcEndpointResourceDoubleBuffering);
			test.Printf(_L("Double Buffering on endpoint 1 %s\n"),
						res ? _S("now allocated") : _S("not allocated"));
			OstTraceExt1(TRACE_NORMAL, SETUPINTERFACE_SETUPINTERFACE_DUP21, "Double Buffering on endpoint 1 %s\n",
						res ? _L("now allocated") : _L("not allocated"));

			res = gPort.QueryEndpointResourceUse(EEndpoint1, EUsbcEndpointResourceDMA);
			test.Printf(_L("DMA on endpoint 1 %s\n"),
						res ? _S("still allocated") : _S("not allocated"));										
			OstTraceExt1(TRACE_NORMAL, SETUPINTERFACE_SETUPINTERFACE_DUP22, "DMA on endpoint 1 %s\n",
						res ? _L("still allocated") : _L("not allocated"));										
		}
		
	test.End();
	}


static void TestDeviceDescriptor()
	{
	test.Start(_L("Device Descriptor Manipulation"));

	test.Next(_L("GetDeviceDescriptorSize()"));
	TInt desc_size = 0;
	gPort.GetDeviceDescriptorSize(desc_size);
	test(static_cast<TUint>(desc_size) == KUsbDescSize_Device);

	test.Next(_L("GetDeviceDescriptor()"));
	TBuf8<KUsbDescSize_Device> descriptor;
	TInt r = gPort.GetDeviceDescriptor(descriptor);
	test(r == KErrNone);

	test.Next(_L("SetDeviceDescriptor()"));
	// Change the USB spec number to 2.30
	descriptor[KDevDesc_SpecOffset]   = 0x30;
	descriptor[KDevDesc_SpecOffset+1] = 0x02;
	// Change the device vendor ID (VID) to 0x1234
	descriptor[KDevDesc_VendorIdOffset]   = 0x34;			// little endian
	descriptor[KDevDesc_VendorIdOffset+1] = 0x12;
	// Change the device product ID (PID) to 0x1111
	descriptor[KDevDesc_ProductIdOffset]   = 0x11;
	descriptor[KDevDesc_ProductIdOffset+1] = 0x11;
	// Change the device release number to 3.05
	descriptor[KDevDesc_DevReleaseOffset]   = 0x05;
	descriptor[KDevDesc_DevReleaseOffset+1] = 0x03;
	r = gPort.SetDeviceDescriptor(descriptor);
	test(r == KErrNone);

	test.Next(_L("GetDeviceDescriptor()"));
	TBuf8<KUsbDescSize_Device> descriptor2;
	r = gPort.GetDeviceDescriptor(descriptor2);
	test(r == KErrNone);

	test.Next(_L("Compare device descriptor with value set"));
	r = descriptor2.Compare(descriptor);
	test(r == KErrNone);

	if (gUsingHighSpeed)
		{
		// HS only allows one possible packet size.
		test(descriptor[KDevDesc_Ep0SizeOffset] == 64);
		}

	test.End();
	}


static void	TestDeviceQualifierDescriptor()
	{
	test.Start(_L("Device_Qualifier Descriptor Manipulation"));

	if (!gSupportsHighSpeed)
		{
		test.Printf(_L("*** Not supported - skipping Device_Qualifier descriptor tests\n"));
		OstTrace0(TRACE_NORMAL, TESTDEVICEQUALIFIERDESCRIPTOR_TESTDEVICEQUALIFIERDESCRIPTOR, "*** Not supported - skipping Device_Qualifier descriptor tests\n");
		test.End();
		return;
		}

	test.Next(_L("GetDeviceQualifierDescriptor()"));
	TBuf8<KUsbDescSize_DeviceQualifier> descriptor;
	TInt r = gPort.GetDeviceQualifierDescriptor(descriptor);
	test(r == KErrNone);

	test.Next(_L("SetDeviceQualifierDescriptor()"));
	// Change the USB spec number to 3.00
	descriptor[KDevDesc_SpecOffset]   = 0x00;
	descriptor[KDevDesc_SpecOffset+1] = 0x03;
	// Change the device class, subclass and protocol codes
	descriptor[KDevDesc_DevClassOffset]    = 0xA1;
	descriptor[KDevDesc_DevSubClassOffset] = 0xB2;
	descriptor[KDevDesc_DevProtocolOffset] = 0xC3;
	r = gPort.SetDeviceQualifierDescriptor(descriptor);
	test(r == KErrNone);

	test.Next(_L("GetDeviceQualifierDescriptor()"));
	TBuf8<KUsbDescSize_DeviceQualifier> descriptor2;
	r = gPort.GetDeviceQualifierDescriptor(descriptor2);
	test(r == KErrNone);

	test.Next(_L("Compare Device_Qualifier desc with value set"));
	r = descriptor2.Compare(descriptor);
	test(r == 0);

	if (!gUsingHighSpeed)
		{
		// HS only allows one possible packet size.
		test(descriptor[KDevDesc_Ep0SizeOffset] == 64);
		}

	test.End();
	}


static void TestConfigurationDescriptor()
	{
	test.Start(_L("Configuration Descriptor Manipulation"));

	test.Next(_L("GetConfigurationDescriptorSize()"));
	TInt desc_size = 0;
	gPort.GetConfigurationDescriptorSize(desc_size);
	test(static_cast<TUint>(desc_size) == KUsbDescSize_Config);

	test.Next(_L("GetConfigurationDescriptor()"));
	TBuf8<KUsbDescSize_Config> descriptor;
	TInt r = gPort.GetConfigurationDescriptor(descriptor);
	test(r == KErrNone);

	test.Next(_L("SetConfigurationDescriptor()"));
	// Invert Remote-Wakup support
	descriptor[KConfDesc_AttribOffset] = (descriptor[KConfDesc_AttribOffset] ^ KUsbDevAttr_RemoteWakeup);
	// Change the reported max power to 200mA (2 * 0x64)
	descriptor[KConfDesc_MaxPowerOffset] = 0x64;
	r = gPort.SetConfigurationDescriptor(descriptor);
	test(r == KErrNone);

	test.Next(_L("GetConfigurationDescriptor()"));
	TBuf8<KUsbDescSize_Config> descriptor2;
	r = gPort.GetConfigurationDescriptor(descriptor2);
	test(r == KErrNone);

	test.Next(_L("Compare configuration desc with value set"));
	r = descriptor2.Compare(descriptor);
	test(r == KErrNone);

	test.End();
	}


static void	TestOtherSpeedConfigurationDescriptor()
	{
	test.Start(_L("Other_Speed_Configuration Desc Manipulation"));

	if (!gSupportsHighSpeed)
		{
		test.Printf(_L("*** Not supported - skipping Other_Speed_Configuration desc tests\n"));
		OstTrace0(TRACE_NORMAL, TESTOTHERSPEEDCONFIGURATIONDESCRIPTOR_TESTOTHERSPEEDCONFIGURATIONDESCRIPTOR, "*** Not supported - skipping Other_Speed_Configuration desc tests\n");
		test.End();
		return;
		}

	test.Next(_L("GetOtherSpeedConfigurationDescriptor()"));
	TBuf8<KUsbDescSize_OtherSpeedConfig> descriptor;
	TInt r = gPort.GetOtherSpeedConfigurationDescriptor(descriptor);
	test(r == KErrNone);

	test.Next(_L("SetOtherSpeedConfigurationDescriptor()"));
	// Invert Remote-Wakup support
	descriptor[KConfDesc_AttribOffset] = (descriptor[KConfDesc_AttribOffset] ^ KUsbDevAttr_RemoteWakeup);
	// Change the reported max power to 330mA (2 * 0xA5)
	descriptor[KConfDesc_MaxPowerOffset] = 0xA5;
	r = gPort.SetOtherSpeedConfigurationDescriptor(descriptor);
	test(r == KErrNone);

	test.Next(_L("GetOtherSpeedConfigurationDescriptor()"));
	TBuf8<KUsbDescSize_OtherSpeedConfig> descriptor2;
	r = gPort.GetOtherSpeedConfigurationDescriptor(descriptor2);
	test(r == KErrNone);

	test.Next(_L("Compare O_S_Config desc with value set"));
	r = descriptor2.Compare(descriptor);
	test(r == KErrNone);

	test.End();
	}


static void TestInterfaceDescriptor()
	{
	test.Start(_L("Interface Descriptor Manipulation"));

	// First the standard Interface descriptor

	test.Next(_L("GetInterfaceDescriptorSize()"));
	TInt desc_size = 0;
	TInt r = gPort.GetInterfaceDescriptorSize(0, desc_size);
	test(r == KErrNone);
	test(static_cast<TUint>(desc_size) == KUsbDescSize_Interface);

	test.Next(_L("GetInterfaceDescriptor()"));
	TBuf8<KUsbDescSize_Interface> descriptor;
	r = gPort.GetInterfaceDescriptor(0, descriptor);
	test(r == KErrNone);

	test.Next(_L("SetInterfaceDescriptor()"));
	// Change the interface protocol to 0x78(+)
	TUint8 prot = 0x78;
	if (descriptor[KIfcDesc_ProtocolOffset] == prot)
		prot++;
	descriptor[KIfcDesc_ProtocolOffset] = prot;
	r = gPort.SetInterfaceDescriptor(0, descriptor);
	test(r == KErrNone);

	test.Next(_L("GetInterfaceDescriptor()"));
	TBuf8<KUsbDescSize_Interface> descriptor2;
	r = gPort.GetInterfaceDescriptor(0, descriptor2);
	test(r == KErrNone);

	test.Next(_L("Compare interface descriptor with value set"));
	r = descriptor2.Compare(descriptor);
	test(r == KErrNone);

	test.End();
	}


static void TestClassSpecificDescriptors()
	{
	test.Start(_L("Class-specific Descriptor Manipulation"));

	// First a class-specific Interface descriptor

	test.Next(_L("SetCSInterfaceDescriptorBlock()"));
	// choose arbitrary new descriptor size
	const TInt KUsbDescSize_CS_Interface = KUsbDescSize_Interface + 10;
	TBuf8<KUsbDescSize_CS_Interface> cs_ifc_descriptor;
	cs_ifc_descriptor.FillZ(cs_ifc_descriptor.MaxLength());
	cs_ifc_descriptor[KUsbDesc_SizeOffset] = KUsbDescSize_CS_Interface;
	cs_ifc_descriptor[KUsbDesc_TypeOffset] = KUsbDescType_CS_Interface;
	TInt r = gPort.SetCSInterfaceDescriptorBlock(0, cs_ifc_descriptor);
	test(r == KErrNone);

	test.Next(_L("GetCSInterfaceDescriptorBlockSize()"));
	TInt desc_size = 0;
	r = gPort.GetCSInterfaceDescriptorBlockSize(0, desc_size);
	test(r == KErrNone);
	test(desc_size == KUsbDescSize_CS_Interface);

	test.Next(_L("GetCSInterfaceDescriptorBlock()"));
	TBuf8<KUsbDescSize_CS_Interface> descriptor;
	r = gPort.GetCSInterfaceDescriptorBlock(0, descriptor);
	test(r == KErrNone);

	test.Next(_L("Compare CS ifc descriptor with value set"));
	r = descriptor.Compare(cs_ifc_descriptor);
	test(r == KErrNone);

	// Next a class-specific Endpoint descriptor

	test.Next(_L("SetCSEndpointDescriptorBlock()"));
	// choose arbitrary new descriptor size
	const TInt KUsbDescSize_CS_Endpoint = KUsbDescSize_Endpoint + 5;
	TBuf8<KUsbDescSize_CS_Endpoint> cs_ep_descriptor;
	cs_ep_descriptor.FillZ(cs_ep_descriptor.MaxLength());
	cs_ep_descriptor[KUsbDesc_SizeOffset] = KUsbDescSize_CS_Endpoint;
	cs_ep_descriptor[KUsbDesc_TypeOffset] = KUsbDescType_CS_Endpoint;
	r = gPort.SetCSEndpointDescriptorBlock(0, 2, cs_ep_descriptor);
	test(r == KErrNone);

	test.Next(_L("GetCSEndpointDescriptorBlockSize()"));
	r = gPort.GetCSEndpointDescriptorBlockSize(0, 2, desc_size);
	test(r == KErrNone);
	test(desc_size == KUsbDescSize_CS_Endpoint);

	test.Next(_L("GetCSEndpointDescriptorBlock()"));
	TBuf8<KUsbDescSize_CS_Endpoint> descriptor2;
	r = gPort.GetCSEndpointDescriptorBlock(0, 2, descriptor2);
	test(r == KErrNone);

	test.Next(_L("Compare CS ep descriptor with value set"));
	r = descriptor2.Compare(cs_ep_descriptor);
	test(r == KErrNone);

	test.End();
	}


static void TestAlternateInterfaceManipulation()
	{
	test.Start(_L("Alternate Interface Setting Manipulation"));

	if (!SupportsAlternateInterfaces())
		{
		test.Printf(_L("*** Not supported - skipping alternate interface settings tests\n"));
		OstTrace0(TRACE_NORMAL, TESTALTERNATEINTERFACEMANIPULATION_TESTALTERNATEINTERFACEMANIPULATION, "*** Not supported - skipping alternate interface settings tests\n");
		test.End();
		return;
		}

	// Fetch endpoint data (again)
	test.Next(_L("Get endpoint capabilities"));
	TUsbDeviceCaps d_caps;
	TInt r = gPort.DeviceCaps(d_caps);
	test(r == KErrNone);
	const TInt n = d_caps().iTotalEndpoints;
	TUsbcEndpointData data[KUsbcMaxEndpoints];
	TPtr8 dataptr(reinterpret_cast<TUint8*>(data), sizeof(data), sizeof(data));
	r = gPort.EndpointCaps(dataptr);
	test(r == KErrNone);

	// Find ep's for alternate ifc setting
	test.Next(_L("Find suitable endpoints"));
	TInt ep_found = 0;
	TBool foundIsoIN  = EFalse;
	TBool foundIsoOUT = EFalse;
	TBool foundIntIN  = EFalse;
	TUsbcInterfaceInfoBuf ifc;

	// NB! We cannot assume that any specific device has any given set of
	// capabilities, so whilst we try and set an assortment of endpoint types
	// we may not get what we want.

	// Also, note that the endpoint[] array in the interface descriptor
	// must be filled from ep[0]...ep[n-1].

	for (TInt i = 0; i < n; i++)
		{
		const TUsbcEndpointCaps* const caps = &data[i].iCaps;
		const TInt mps = caps->MaxPacketSize();
		if (!foundIsoIN &&
			(caps->iTypesAndDir & (KUsbEpTypeIsochronous | KUsbEpDirIn)) ==
			(KUsbEpTypeIsochronous | KUsbEpDirIn))
			{
			// This is going to be our Iso TX (IN) endpoint
			ifc().iEndpointData[ep_found].iType = KUsbEpTypeIsochronous;
			ifc().iEndpointData[ep_found].iDir  = KUsbEpDirIn;
			ifc().iEndpointData[ep_found].iSize = mps;
			ifc().iEndpointData[ep_found].iInterval = 0x01;	// 2^(bInterval-1)ms, bInterval must be [1..16]
			ifc().iEndpointData[ep_found].iInterval_Hs = 0x01; // same as for FS
			test.Printf(_L("ISO  IN  size = %4d (ep %d)\n"), mps, ep_found + 1);
			OstTraceExt2(TRACE_NORMAL, TESTALTERNATEINTERFACEMANIPULATION_TESTALTERNATEINTERFACEMANIPULATION_DUP01, "ISO  IN  size = %4d (ep %d)\n", mps, ep_found + 1);
			foundIsoIN = ETrue;
			if (++ep_found == 3)
				break;
			}
		else if (!foundIsoOUT &&
				 (caps->iTypesAndDir & (KUsbEpTypeIsochronous | KUsbEpDirOut)) ==
				 (KUsbEpTypeIsochronous | KUsbEpDirOut))
			{
			// This is going to be our Iso RX (OUT) endpoint
			ifc().iEndpointData[ep_found].iType = KUsbEpTypeIsochronous;
			ifc().iEndpointData[ep_found].iDir  = KUsbEpDirOut;
			ifc().iEndpointData[ep_found].iSize = mps;
			ifc().iEndpointData[ep_found].iInterval = 0x01;	// 2^(bInterval-1)ms, bInterval must be [1..16]
			test.Printf(_L("ISO  OUT size = %4d (ep %d)\n"), mps, ep_found + 1);
			OstTraceExt2(TRACE_NORMAL, TESTALTERNATEINTERFACEMANIPULATION_TESTALTERNATEINTERFACEMANIPULATION_DUP02, "ISO  OUT size = %4d (ep %d)\n", mps, ep_found + 1);
			foundIsoOUT = ETrue;
			if (++ep_found == 3)
				break;
			}
		else if (!foundIntIN &&
				 (caps->iTypesAndDir & (KUsbEpTypeInterrupt | KUsbEpDirIn)) ==
				 (KUsbEpTypeInterrupt | KUsbEpDirIn))
			{
			// This is going to be our Interrupt TX (IN) endpoint
			ifc().iEndpointData[ep_found].iType  = KUsbEpTypeInterrupt;
			ifc().iEndpointData[ep_found].iDir   = KUsbEpDirIn;
			ifc().iEndpointData[ep_found].iSize  = mps;
			ifc().iEndpointData[ep_found].iInterval = 10;	// interval = 10ms, valid range [1..255]
			ifc().iEndpointData[ep_found].iInterval_Hs = 4;	// interval = 2^(bInterval-1)ms = 8ms
			ifc().iEndpointData[ep_found].iExtra    = 2;	// 2 extra bytes for Audio Class EP descriptor
			test.Printf(_L("INT  IN  size = %4d (ep %d)\n"), mps, ep_found + 1);
			OstTraceExt2(TRACE_NORMAL, TESTALTERNATEINTERFACEMANIPULATION_TESTALTERNATEINTERFACEMANIPULATION_DUP03, "INT  IN  size = %4d (ep %d)\n", mps, ep_found + 1);
			foundIntIN = ETrue;
			INT_IN_ep = ep_found + 1;
			if (++ep_found == 3)
				break;
			}
		}

	// Let's try to add some more Bulk endpoints up to the max # of 5.
	for (TInt i = 0; i < n; i++)
		{
		TUsbcEndpointCaps& caps = data[i].iCaps;
		const TUint mps = caps.MaxPacketSize();
		if (caps.iTypesAndDir & KUsbEpTypeBulk)
			{
			const TUint dir = (caps.iTypesAndDir & KUsbEpDirIn) ? KUsbEpDirIn : KUsbEpDirOut;
			ifc().iEndpointData[ep_found].iType = KUsbEpTypeBulk;
			ifc().iEndpointData[ep_found].iDir = dir;
			if (gUsingHighSpeed)
				{
				test.Printf(_L("Checking if correct Bulk packet size is reported in HS case\n"));
				OstTrace0(TRACE_NORMAL, TESTALTERNATEINTERFACEMANIPULATION_TESTALTERNATEINTERFACEMANIPULATION_DUP04, "Checking if correct Bulk packet size is reported in HS case\n");
				test(mps == KUsbEpSize512);					// sane?
				}
			// The PSL should in any case also offer the 'legacy' FS size:
			test(caps.iSizes & KUsbEpSize64);
			ifc().iEndpointData[ep_found].iSize = mps;
			test.Printf(_L("BULK %s size = %4d (ep %d)\n"),
						dir == KUsbEpDirIn ? _S("IN ") : _S("OUT"), mps, ep_found + 1);
			OstTraceExt3(TRACE_NORMAL, TESTALTERNATEINTERFACEMANIPULATION_TESTALTERNATEINTERFACEMANIPULATION_DUP05, "BULK %S size = %4d (ep %d)\n",
						dir == KUsbEpDirIn ? _L("IN ") : _L("OUT"), mps, ep_found + 1);
			if (++ep_found == 5)
				break;
			}
		}

	test.Printf(_L("Total: %d endpoints found for the alt. ifc setting\n"), ep_found);
	OstTrace1(TRACE_NORMAL, TESTALTERNATEINTERFACEMANIPULATION_TESTALTERNATEINTERFACEMANIPULATION_DUP06, "Total: %d endpoints found for the alt. ifc setting\n", ep_found);
	if (ep_found < 3)
		{
		test.Printf(_L("(3 endpoints are at least required. Skipping test...)\n"));
		OstTrace0(TRACE_NORMAL, TESTALTERNATEINTERFACEMANIPULATION_TESTALTERNATEINTERFACEMANIPULATION_DUP07, "(3 endpoints are at least required. Skipping test...)\n");
		test.End();
		return;
		}

	if (!foundIsoIN && !foundIsoOUT)
		{
		test.Printf(_L("(No Isochronous endpoints found)\n"));
		OstTrace0(TRACE_NORMAL, TESTALTERNATEINTERFACEMANIPULATION_TESTALTERNATEINTERFACEMANIPULATION_DUP08, "(No Isochronous endpoints found)\n");
		}

	if (!foundIntIN)
		{
		test.Printf(_L("(No Interrupt endpoint found)\n"));
		OstTrace0(TRACE_NORMAL, TESTALTERNATEINTERFACEMANIPULATION_TESTALTERNATEINTERFACEMANIPULATION_DUP09, "(No Interrupt endpoint found)\n");
		test.Printf(_L("Adjusting endpoint size for later test\n"));
		OstTrace0(TRACE_NORMAL, TESTALTERNATEINTERFACEMANIPULATION_TESTALTERNATEINTERFACEMANIPULATION_DUP10, "Adjusting endpoint size for later test\n");
		// We want to make sure that at least one descriptor has the 2 extra bytes.
		// It doesn't matter that this ep could be a Bulk one, or that the 2 Iso ep's might be missing -
		// we just want to test some functionality and we're not going to use this interface in earnest.
		ifc().iEndpointData[2].iExtra = 2;					// 2 extra bytes for Audio Class Ep descriptor
		INT_IN_ep = 3;										// pretend it's an INT ep
		}

	test.Next(_L("Create alternate interface setting"));
	_LIT16(string, "T_USBAPI Test Interface (Setting 1: Audio)");
	ifc().iString = const_cast<TDesC16*>(&string);
	ifc().iTotalEndpointsUsed = ep_found;
	ifc().iClass.iClassNum	  = KUsbAudioInterfaceClassCode;
	ifc().iClass.iSubClassNum = KUsbAudioInterfaceSubclassCode_Audiostreaming;
	ifc().iClass.iProtocolNum = KUsbAudioInterfaceProtocolCode_Pr_Protocol_Undefined;
	r = gPort.SetInterface(1, ifc);
	test(r == KErrNone);

	test.Next(_L("Set alternate setting number to 8"));
	TBuf8<KUsbDescSize_Interface> descriptor;
	r = gPort.GetInterfaceDescriptor(1, descriptor);
	test(r == KErrNone);
	descriptor[KIfcDesc_SettingOffset] = 8;
	r = gPort.SetInterfaceDescriptor(1, descriptor);
	test(r != KErrNone);

	test.Next(_L("Change ifc # in def setting whith alt ifcs"));
	r = gPort.GetInterfaceDescriptor(0, descriptor);
	test(r == KErrNone);
	descriptor[KIfcDesc_SettingOffset] = 8;
	r = gPort.SetInterfaceDescriptor(0, descriptor);
	test(r != KErrNone);

	test.Next(_L("Change the ifc # in default setting to 8"));
	r = gPort.ReleaseInterface(1);
	test(r == KErrNone);
	r = gPort.SetInterfaceDescriptor(0, descriptor);
	test(r == KErrNone);

	test.Next(_L("Create new setting - this should also get #8"));
	r = gPort.SetInterface(1, ifc);
	test(r == KErrNone);
	r = gPort.GetInterfaceDescriptor(1, descriptor);
	test(r == KErrNone);
	test(descriptor[KIfcDesc_SettingOffset] == 8);

	test.Next(_L("Change the ifc # in default setting to 0"));
	r = gPort.ReleaseInterface(1);
	test(r == KErrNone);
	r = gPort.GetInterfaceDescriptor(0, descriptor);
	test(r == KErrNone);
	descriptor[KIfcDesc_SettingOffset] = 0;
	r = gPort.SetInterfaceDescriptor(0, descriptor);
	test(r == KErrNone);

	test.Next(_L("Create new setting - this should also get #0"));
	r = gPort.SetInterface(1, ifc);
	test(r == KErrNone);
	r = gPort.GetInterfaceDescriptor(1, descriptor);
	test(r == KErrNone);
	test(descriptor[KIfcDesc_SettingOffset] == 0);

	test.End();
	}


static void TestEndpointDescriptor()
	{
	test.Start(_L("Endpoint Descriptor Manipulation"));

	test.Next(_L("GetEndpointDescriptorSize(1)"));
	TInt epNumber = 1;
	TInt desc_size = 0;
	TInt r = gPort.GetEndpointDescriptorSize(0, epNumber, desc_size);
	test(r == KErrNone);
	test(static_cast<TUint>(desc_size) == KUsbDescSize_Endpoint);

	test.Next(_L("GetEndpointDescriptor(1)"));
	TBuf8<KUsbDescSize_Endpoint> descriptor;
	r = gPort.GetEndpointDescriptor(0, epNumber, descriptor);
	test(r == KErrNone);

	test.Next(_L("SetEndpointDescriptor(1)"));
	// Change the endpoint poll interval
	TUint8 ival = 0x66;
	if (descriptor[KEpDesc_IntervalOffset] == ival)
		ival++;
	descriptor[KEpDesc_IntervalOffset] = ival;
	r = gPort.SetEndpointDescriptor(0, epNumber, descriptor);
	test(r == KErrNone);

	test.Next(_L("GetEndpointDescriptor(1)"));
	TBuf8<KUsbDescSize_Endpoint> descriptor2;
	r = gPort.GetEndpointDescriptor(0, epNumber, descriptor2);
	test(r == KErrNone);

	test.Next(_L("Compare endpoint descriptor with value set"));
	r = descriptor2.Compare(descriptor);
	test(r == KErrNone);

	test.Next(_L("Check endpoint max packet size"));
	const TUint16 ep_size = EpSize(descriptor[KEpDesc_PacketSizeOffset],
								   descriptor[KEpDesc_PacketSizeOffset+1]);
	test.Printf(_L(" Size: %d\n"), ep_size);
	OstTrace1(TRACE_NORMAL, TESTENDPOINTDESCRIPTOR_TESTENDPOINTDESCRIPTOR, " Size: %d\n", ep_size);
	if (gUsingHighSpeed)
		{
		// HS Bulk ep can only have one possible packet size.
		test(ep_size == 512);
		}
	else
		{
		// FS Bulk ep cannot be larger than 64 bytes.
		test(ep_size <= 64);
		}

	test.End();
	}


static void TestExtendedEndpointDescriptor()
	{
	test.Start(_L("Extended Endpoint Descriptor Manipulation"));

	if (!SupportsAlternateInterfaces())
		{
		test.Printf(_L("*** Not supported - skipping Extended Endpoint descriptor tests\n"));
		OstTrace0(TRACE_NORMAL, TESTEXTENDEDENDPOINTDESCRIPTOR_TESTEXTENDEDENDPOINTDESCRIPTOR, "*** Not supported - skipping Extended Endpoint descriptor tests\n");
		test.End();
		return;
		}

	// Extended Endpoint Descriptor manipulation (Audio class endpoint)

	test.Next(_L("GetEndpointDescriptorSize()"));
	TInt epNumber = INT_IN_ep;
	TInt desc_size = 0;
	TInt r = gPort.GetEndpointDescriptorSize(1, epNumber, desc_size);
	test(r == KErrNone);
	test(static_cast<TUint>(desc_size) == KUsbDescSize_AudioEndpoint);

	test.Next(_L("GetEndpointDescriptor()"));
	TBuf8<KUsbDescSize_AudioEndpoint> descriptor;
	r = gPort.GetEndpointDescriptor(1, epNumber, descriptor);
	test(r == KErrNone);

	test.Next(_L("SetEndpointDescriptor()"));
	// Change the Audio Endpoint bSynchAddress field
	TUint8 addr = 0x85;										// bogus address
	if (descriptor[KEpDesc_SynchAddressOffset] == addr)
		addr++;
	descriptor[KEpDesc_SynchAddressOffset] = addr;
	r = gPort.SetEndpointDescriptor(1, epNumber, descriptor);
	test(r == KErrNone);

	test.Next(_L("GetEndpointDescriptor()"));
	TBuf8<KUsbDescSize_AudioEndpoint> descriptor2;
	r = gPort.GetEndpointDescriptor(1, epNumber, descriptor2);
	test(r == KErrNone);

	test.Next(_L("Compare endpoint descriptor with value set"));
	r = descriptor2.Compare(descriptor);
	test(r == KErrNone);

	test.Next(_L("Check endpoint max packet size"));
	const TUint16 ep_size = EpSize(descriptor[KEpDesc_PacketSizeOffset],
								   descriptor[KEpDesc_PacketSizeOffset+1]);
	if (gUsingHighSpeed)
		{
		// HS Interrupt ep.
		test(ep_size <= 1024);
		}
	else
		{
		// FS Interrupt ep cannot be larger than 64 bytes.
		test(ep_size <= 64);
		}

	test.End();
	}


static void TestStandardStringDescriptors()
	{
	test.Start(_L("String Descriptor Manipulation"));

	//
	// --- LANGID code
	//

	test.Next(_L("GetStringDescriptorLangId()"));
	TUint16 rd_langid_orig;
	TInt r = gPort.GetStringDescriptorLangId(rd_langid_orig);
	test(r == KErrNone);
	test.Printf(_L("Original LANGID code: 0x%04X\n"), rd_langid_orig);
	OstTrace1(TRACE_NORMAL, TESTSTANDARDSTRINGDESCRIPTORS_TESTSTANDARDSTRINGDESCRIPTORS, "Original LANGID code: 0x%04X\n", rd_langid_orig);

	test.Next(_L("SetStringDescriptorLangId()"));
	TUint16 wr_langid = 0x0809;								// English (UK) Language ID
	if (wr_langid == rd_langid_orig)
		wr_langid = 0x0444;									// Tatar Language ID
	r = gPort.SetStringDescriptorLangId(wr_langid);
	test(r == KErrNone);

	test.Next(_L("GetStringDescriptorLangId()"));
	TUint16 rd_langid;
	r = gPort.GetStringDescriptorLangId(rd_langid);
	test(r == KErrNone);
	test.Printf(_L("New LANGID code: 0x%04X\n"), rd_langid);
	OstTrace1(TRACE_NORMAL, TESTSTANDARDSTRINGDESCRIPTORS_TESTSTANDARDSTRINGDESCRIPTORS_DUP01, "New LANGID code: 0x%04X\n", rd_langid);

	test.Next(_L("Compare LANGID codes"));
	test(rd_langid == wr_langid);

	test.Next(_L("Restore original LANGID code"));
	r = gPort.SetStringDescriptorLangId(rd_langid_orig);
	test(r == KErrNone);
	r = gPort.GetStringDescriptorLangId(rd_langid);
	test(r == KErrNone);
	test(rd_langid == rd_langid_orig);

	//
	// --- Manufacturer string
	//

	test.Next(_L("GetManufacturerStringDescriptor()"));
	TBuf16<KUsbStringDescStringMaxSize / 2> rd_str_orig;
	r = gPort.GetManufacturerStringDescriptor(rd_str_orig);
	test(r == KErrNone || r == KErrNotFound);
	TBool restore_string;
	if (r == KErrNone)
		{
		test.Printf(_L("Original Manufacturer string: \"%lS\"\n"), &rd_str_orig);
		OstTraceExt1(TRACE_NORMAL, TESTSTANDARDSTRINGDESCRIPTORS_TESTSTANDARDSTRINGDESCRIPTORS_DUP02, "Original Manufacturer string: \"%lS\"\n", rd_str_orig);
		restore_string = ETrue;
		}
	else
		{
		test.Printf(_L("No Manufacturer string set\n"));
		OstTrace0(TRACE_NORMAL, TESTSTANDARDSTRINGDESCRIPTORS_TESTSTANDARDSTRINGDESCRIPTORS_DUP03, "No Manufacturer string set\n");
		restore_string = EFalse;
		}

	test.Next(_L("SetManufacturerStringDescriptor()"));
	_LIT16(manufacturer, "Manufacturer Which Manufactures Devices");
	TBuf16<KUsbStringDescStringMaxSize / 2> wr_str(manufacturer);
	r = gPort.SetManufacturerStringDescriptor(wr_str);
	test(r == KErrNone);

	test.Next(_L("GetManufacturerStringDescriptor()"));
	TBuf16<KUsbStringDescStringMaxSize / 2> rd_str;
	r = gPort.GetManufacturerStringDescriptor(rd_str);
	test(r == KErrNone);
	test.Printf(_L("New Manufacturer string: \"%lS\"\n"), &rd_str);
	OstTraceExt1(TRACE_NORMAL, TESTSTANDARDSTRINGDESCRIPTORS_TESTSTANDARDSTRINGDESCRIPTORS_DUP04, "New Manufacturer string: \"%lS\"\n", rd_str);

	test.Next(_L("Compare Manufacturer strings"));
	r = rd_str.Compare(wr_str);
	test(r == KErrNone);

	test.Next(_L("SetManufacturerStringDescriptor()"));
	_LIT16(manufacturer2, "Different Manufacturer Which Manufactures Different Devices");
	wr_str.FillZ(wr_str.MaxLength());
	wr_str = manufacturer2;
	r = gPort.SetManufacturerStringDescriptor(wr_str);
	test(r == KErrNone);

	test.Next(_L("GetManufacturerStringDescriptor()"));
	rd_str.FillZ(rd_str.MaxLength());
	r = gPort.GetManufacturerStringDescriptor(rd_str);
	test(r == KErrNone);
	test.Printf(_L("New Manufacturer string: \"%lS\"\n"), &rd_str);
	OstTraceExt1(TRACE_NORMAL, TESTSTANDARDSTRINGDESCRIPTORS_TESTSTANDARDSTRINGDESCRIPTORS_DUP05, "New Manufacturer string: \"%lS\"\n", rd_str);

	test.Next(_L("Compare Manufacturer strings"));
	r = rd_str.Compare(wr_str);
	test(r == KErrNone);

	test.Next(_L("RemoveManufacturerStringDescriptor()"));
	r = gPort.RemoveManufacturerStringDescriptor();
	test(r == KErrNone);
	r = gPort.GetManufacturerStringDescriptor(rd_str);
	test(r == KErrNotFound);

	if (restore_string)
		{
		test.Next(_L("Restore original string"));
		r = gPort.SetManufacturerStringDescriptor(rd_str_orig);
		test(r == KErrNone);
		r = gPort.GetManufacturerStringDescriptor(rd_str);
		test(r == KErrNone);
		r = rd_str.Compare(rd_str_orig);
		test(r == KErrNone);
		}

	//
	// --- Product string
	//

	test.Next(_L("GetProductStringDescriptor()"));
	rd_str_orig.FillZ(rd_str.MaxLength());
	r = gPort.GetProductStringDescriptor(rd_str_orig);
	test(r == KErrNone || r == KErrNotFound);
	if (r == KErrNone)
		{
		test.Printf(_L("Old Product string: \"%lS\"\n"), &rd_str_orig);
		OstTraceExt1(TRACE_NORMAL, TESTSTANDARDSTRINGDESCRIPTORS_TESTSTANDARDSTRINGDESCRIPTORS_DUP06, "Old Product string: \"%lS\"\n", rd_str_orig);
		restore_string = ETrue;
		}
	else
		restore_string = EFalse;

	test.Next(_L("SetProductStringDescriptor()"));
	_LIT16(product, "Product That Was Produced By A Manufacturer");
	wr_str.FillZ(wr_str.MaxLength());
	wr_str = product;
	r = gPort.SetProductStringDescriptor(wr_str);
	test(r == KErrNone);

	test.Next(_L("GetProductStringDescriptor()"));
	rd_str.FillZ(rd_str.MaxLength());
	r = gPort.GetProductStringDescriptor(rd_str);
	test(r == KErrNone);
	test.Printf(_L("New Product string: \"%lS\"\n"), &rd_str);
	OstTraceExt1(TRACE_NORMAL, TESTSTANDARDSTRINGDESCRIPTORS_TESTSTANDARDSTRINGDESCRIPTORS_DUP07, "New Product string: \"%lS\"\n", rd_str);

	test.Next(_L("Compare Product strings"));
	r = rd_str.Compare(wr_str);
	test(r == KErrNone);

	test.Next(_L("SetProductStringDescriptor()"));
	_LIT16(product2, "Different Product That Was Produced By A Different Manufacturer");
	wr_str.FillZ(wr_str.MaxLength());
	wr_str = product2;
	r = gPort.SetProductStringDescriptor(wr_str);
	test(r == KErrNone);

	test.Next(_L("GetProductStringDescriptor()"));
	rd_str.FillZ(rd_str.MaxLength());
	r = gPort.GetProductStringDescriptor(rd_str);
	test(r == KErrNone);
	test.Printf(_L("New Product string: \"%lS\"\n"), &rd_str);
	OstTraceExt1(TRACE_NORMAL, TESTSTANDARDSTRINGDESCRIPTORS_TESTSTANDARDSTRINGDESCRIPTORS_DUP08, "New Product string: \"%lS\"\n", rd_str);

	test.Next(_L("Compare Product strings"));
	r = rd_str.Compare(wr_str);
	test(r == KErrNone);

	test.Next(_L("RemoveProductStringDescriptor()"));
	r = gPort.RemoveProductStringDescriptor();
	test(r == KErrNone);
	r = gPort.GetProductStringDescriptor(rd_str);
	test(r == KErrNotFound);

	if (restore_string)
		{
		test.Next(_L("Restore original string"));
		r = gPort.SetProductStringDescriptor(rd_str_orig);
		test(r == KErrNone);
		r = gPort.GetProductStringDescriptor(rd_str);
		test(r == KErrNone);
		r = rd_str.Compare(rd_str_orig);
		test(r == KErrNone);
		}

	//
	// --- Serial Number string
	//

	test.Next(_L("GetSerialNumberStringDescriptor()"));
	rd_str_orig.FillZ(rd_str.MaxLength());
	r = gPort.GetSerialNumberStringDescriptor(rd_str_orig);
	test(r == KErrNone || r == KErrNotFound);
	if (r == KErrNone)
		{
		test.Printf(_L("Old Serial Number: \"%lS\"\n"), &rd_str_orig);
		OstTraceExt1(TRACE_NORMAL, TESTSTANDARDSTRINGDESCRIPTORS_TESTSTANDARDSTRINGDESCRIPTORS_DUP09, "Old Serial Number: \"%lS\"\n", rd_str_orig);
		restore_string = ETrue;
		}
	else
		restore_string = EFalse;

	test.Next(_L("SetSerialNumberStringDescriptor()"));
	_LIT16(serial, "000666000XYZ");
	wr_str.FillZ(wr_str.MaxLength());
	wr_str = serial;
	r = gPort.SetSerialNumberStringDescriptor(wr_str);
	test(r == KErrNone);

	test.Next(_L("GetSerialNumberStringDescriptor()"));
	rd_str.FillZ(rd_str.MaxLength());
	r = gPort.GetSerialNumberStringDescriptor(rd_str);
	test(r == KErrNone);
	test.Printf(_L("New Serial Number: \"%lS\"\n"), &rd_str);
	OstTraceExt1(TRACE_NORMAL, TESTSTANDARDSTRINGDESCRIPTORS_TESTSTANDARDSTRINGDESCRIPTORS_DUP10, "New Serial Number: \"%lS\"\n", rd_str);

	test.Next(_L("Compare Serial Number strings"));
	r = rd_str.Compare(wr_str);
	test(r == KErrNone);

	test.Next(_L("SetSerialNumberStringDescriptor()"));
	_LIT16(serial2, "Y11611193111711111Y");
	wr_str.FillZ(wr_str.MaxLength());
	wr_str = serial2;
	r = gPort.SetSerialNumberStringDescriptor(wr_str);
	test(r == KErrNone);

	test.Next(_L("GetSerialNumberStringDescriptor()"));
	rd_str.FillZ(rd_str.MaxLength());
	r = gPort.GetSerialNumberStringDescriptor(rd_str);
	test(r == KErrNone);
	test.Printf(_L("New Serial Number: \"%lS\"\n"), &rd_str);
	OstTraceExt1(TRACE_NORMAL, TESTSTANDARDSTRINGDESCRIPTORS_TESTSTANDARDSTRINGDESCRIPTORS_DUP11, "New Serial Number: \"%lS\"\n", rd_str);

	test.Next(_L("Compare Serial Number strings"));
	r = rd_str.Compare(wr_str);
	test(r == KErrNone);

	test.Next(_L("RemoveSerialNumberStringDescriptor()"));
	r = gPort.RemoveSerialNumberStringDescriptor();
	test(r == KErrNone);
	r = gPort.GetSerialNumberStringDescriptor(rd_str);
	test(r == KErrNotFound);

	if (restore_string)
		{
		test.Next(_L("Restore original string"));
		r = gPort.SetSerialNumberStringDescriptor(rd_str_orig);
		test(r == KErrNone);
		r = gPort.GetSerialNumberStringDescriptor(rd_str);
		test(r == KErrNone);
		r = rd_str.Compare(rd_str_orig);
		test(r == KErrNone);
		}

	//
	// --- Configuration string
	//

	test.Next(_L("GetConfigurationStringDescriptor()"));
	rd_str_orig.FillZ(rd_str.MaxLength());
	r = gPort.GetConfigurationStringDescriptor(rd_str_orig);
	test(r == KErrNone || r == KErrNotFound);
	if (r == KErrNone)
		{
		test.Printf(_L("Old Configuration string: \"%lS\"\n"), &rd_str_orig);
		OstTraceExt1(TRACE_NORMAL, TESTSTANDARDSTRINGDESCRIPTORS_TESTSTANDARDSTRINGDESCRIPTORS_DUP12, "Old Configuration string: \"%lS\"\n", rd_str_orig);
		restore_string = ETrue;
		}
	else
		restore_string = EFalse;

	test.Next(_L("SetConfigurationStringDescriptor()"));
	_LIT16(config, "Relatively Simple Configuration That Is Still Useful");
	wr_str.FillZ(wr_str.MaxLength());
	wr_str = config;
	r = gPort.SetConfigurationStringDescriptor(wr_str);
	test(r == KErrNone);

	test.Next(_L("GetConfigurationStringDescriptor()"));
	rd_str.FillZ(rd_str.MaxLength());
	r = gPort.GetConfigurationStringDescriptor(rd_str);
	test(r == KErrNone);
	test.Printf(_L("New Configuration string: \"%lS\"\n"), &rd_str);
	OstTraceExt1(TRACE_NORMAL, TESTSTANDARDSTRINGDESCRIPTORS_TESTSTANDARDSTRINGDESCRIPTORS_DUP13, "New Configuration string: \"%lS\"\n", rd_str);

	test.Next(_L("Compare Configuration strings"));
	r = rd_str.Compare(wr_str);
	test(r == KErrNone);

	test.Next(_L("SetConfigurationStringDescriptor()"));
	_LIT16(config2, "Convenient Configuration That Can Be Very Confusing");
	wr_str.FillZ(wr_str.MaxLength());
	wr_str = config2;
	r = gPort.SetConfigurationStringDescriptor(wr_str);
	test(r == KErrNone);

	test.Next(_L("GetConfigurationStringDescriptor()"));
	rd_str.FillZ(rd_str.MaxLength());
	r = gPort.GetConfigurationStringDescriptor(rd_str);
	test(r == KErrNone);
	test.Printf(_L("New Configuration string: \"%lS\"\n"), &rd_str);
	OstTraceExt1(TRACE_NORMAL, TESTSTANDARDSTRINGDESCRIPTORS_TESTSTANDARDSTRINGDESCRIPTORS_DUP14, "New Configuration string: \"%lS\"\n", rd_str);

	test.Next(_L("Compare Configuration strings"));
	r = rd_str.Compare(wr_str);
	test(r == KErrNone);

	test.Next(_L("RemoveConfigurationStringDescriptor()"));
	r = gPort.RemoveConfigurationStringDescriptor();
	test(r == KErrNone);
	r = gPort.GetConfigurationStringDescriptor(rd_str);
	test(r == KErrNotFound);

	if (restore_string)
		{
		test.Next(_L("Restore original string"));
		r = gPort.SetConfigurationStringDescriptor(rd_str_orig);
		test(r == KErrNone);
		r = gPort.GetConfigurationStringDescriptor(rd_str);
		test(r == KErrNone);
		r = rd_str.Compare(rd_str_orig);
		test(r == KErrNone);
		}

	test.End();
	}


//---------------------------------------------
//! @SYMTestCaseID KBASE-T_USBAPI-0041
//! @SYMTestType UT
//! @SYMREQ REQ5662
//! @SYMTestCaseDesc USB Device Driver API extension to support setting of a string descriptor at a specific index
//! @SYMTestActions Tests GetStringDescriptor(), SetStringDescriptor() and RemoveStringDescriptor() to verify
//! the right values are retrieved, set and deleted at specific positions
//! @SYMTestExpectedResults KErrNone in positive testing and KErrNotFound in negative one
//! @SYMTestPriority High
//! @SYMTestStatus Implemented
//---------------------------------------------
static void TestArbitraryStringDescriptors()
	{
	test.Start(_L("Arbitrary String Descriptor Manipulation"));

	const TUint8 stridx1 = 0xEE;
	const TUint8 stridx2 = 0xCC;
	const TUint8 stridx3 = 0xDD;
	const TUint8 stridx4 = 0xFF;

	// First test string

	test.Next(_L("GetStringDescriptor() 1"));
	TBuf16<KUsbStringDescStringMaxSize / 2> rd_str;
	TInt r = gPort.GetStringDescriptor(stridx1, rd_str);
	test(r == KErrNotFound);

	test.Next(_L("SetStringDescriptor() 1"));
	_LIT16(string_one, "Arbitrary String Descriptor Test String 1");
	TBuf16<KUsbStringDescStringMaxSize / 2> wr_str(string_one);
	r = gPort.SetStringDescriptor(stridx1, wr_str);
	test(r == KErrNone);

	test.Next(_L("GetStringDescriptor() 1"));
	r = gPort.GetStringDescriptor(stridx1, rd_str);
	test(r == KErrNone);
	test.Printf(_L("New test string @ idx %d: \"%lS\"\n"), stridx1, &rd_str);
	OstTraceExt2(TRACE_NORMAL, TESTARBITRARYSTRINGDESCRIPTORS_TESTARBITRARYSTRINGDESCRIPTORS, "New test string @ idx %d: \"%lS\"\n", stridx1, rd_str);

	test.Next(_L("Compare test strings 1"));
	r = rd_str.Compare(wr_str);
	test(r == KErrNone);

	// Second test string

	test.Next(_L("GetStringDescriptor() 2"));
	rd_str.FillZ(rd_str.MaxLength());
	r = gPort.GetStringDescriptor(stridx2, rd_str);
	test(r == KErrNotFound);

	test.Next(_L("SetStringDescriptor() 2"));
	_LIT16(string_two, "Arbitrary String Descriptor Test String 2");
	wr_str.FillZ(wr_str.MaxLength());
	wr_str = string_two;
	r = gPort.SetStringDescriptor(stridx2, wr_str);
	test(r == KErrNone);

	// In between we create another interface setting to see what happens
	// to the existing string descriptor indices.
	// (We don't have to test this on every platform -
	// besides, those that don't support alt settings
	// are by now very rare.)
	if (SupportsAlternateInterfaces())
		{
		TUsbcInterfaceInfoBuf ifc;
		_LIT16(string, "T_USBAPI Bogus Test Interface (Setting 2)");
		ifc().iString = const_cast<TDesC16*>(&string);
		ifc().iTotalEndpointsUsed = 0;
		TInt r = gPort.SetInterface(2, ifc);
		test(r == KErrNone);
		}

	test.Next(_L("GetStringDescriptor() 2"));
	r = gPort.GetStringDescriptor(stridx2, rd_str);
	test(r == KErrNone);
	test.Printf(_L("New test string @ idx %d: \"%lS\"\n"), stridx2, &rd_str);
	OstTraceExt2(TRACE_NORMAL, TESTARBITRARYSTRINGDESCRIPTORS_TESTARBITRARYSTRINGDESCRIPTORS_DUP01, "New test string @ idx %d: \"%lS\"\n", stridx2, rd_str);

	test.Next(_L("Compare test strings 2"));
	r = rd_str.Compare(wr_str);
	test(r == KErrNone);

	// Third test string

	test.Next(_L("GetStringDescriptor() 3"));
	rd_str.FillZ(rd_str.MaxLength());
	r = gPort.GetStringDescriptor(stridx3, rd_str);
	test(r == KErrNotFound);

	test.Next(_L("SetStringDescriptor() 3"));
	_LIT16(string_three, "Arbitrary String Descriptor Test String 3");
	wr_str.FillZ(wr_str.MaxLength());
	wr_str = string_three;
	r = gPort.SetStringDescriptor(stridx3, wr_str);
	test(r == KErrNone);

	test.Next(_L("GetStringDescriptor() 3"));
	r = gPort.GetStringDescriptor(stridx3, rd_str);
	test(r == KErrNone);
	test.Printf(_L("New test string @ idx %d: \"%lS\"\n"), stridx3, &rd_str);
	OstTraceExt2(TRACE_NORMAL, TESTARBITRARYSTRINGDESCRIPTORS_TESTARBITRARYSTRINGDESCRIPTORS_DUP02, "New test string @ idx %d: \"%lS\"\n", stridx3, rd_str);

	test.Next(_L("Compare test strings 3"));
	r = rd_str.Compare(wr_str);
	test(r == KErrNone);

	// Remove string descriptors

	test.Next(_L("RemoveStringDescriptor() 4"));
	r = gPort.RemoveStringDescriptor(stridx4);
	test(r == KErrNotFound);

	test.Next(_L("RemoveStringDescriptor() 3"));
	r = gPort.RemoveStringDescriptor(stridx3);
	test(r == KErrNone);
	r = gPort.GetStringDescriptor(stridx3, rd_str);
	test(r == KErrNotFound);

	test.Next(_L("RemoveStringDescriptor() 2"));
	r = gPort.RemoveStringDescriptor(stridx2);
	test(r == KErrNone);
	r = gPort.GetStringDescriptor(stridx2, rd_str);
	test(r == KErrNotFound);

	test.Next(_L("RemoveStringDescriptor() 1"));
	r = gPort.RemoveStringDescriptor(stridx1);
	test(r == KErrNone);
	r = gPort.GetStringDescriptor(stridx1, rd_str);
	test(r == KErrNotFound);

	test.End();
	}


static void TestDescriptorManipulation()
	{
	test.Start(_L("Test USB Descriptor Manipulation"));

	TestDeviceDescriptor();

	TestDeviceQualifierDescriptor();

	TestConfigurationDescriptor();

	TestOtherSpeedConfigurationDescriptor();

	TestInterfaceDescriptor();

	TestClassSpecificDescriptors();

	TestAlternateInterfaceManipulation();

	TestEndpointDescriptor();

	TestExtendedEndpointDescriptor();

	TestStandardStringDescriptors();

	TestArbitraryStringDescriptors();

	test.End();
	}


//---------------------------------------------
//! @SYMTestCaseID KBASE-T_USBAPI-0040
//! @SYMTestType UT
//! @SYMTestCaseDesc Test OTG extensions
//! @SYMTestExpectedResults All APIs behave as expected
//! @SYMTestPriority Medium
//! @SYMTestStatus Implemented
//---------------------------------------------
static void TestOtgExtensions()
	{
	test.Start(_L("Test Some OTG API Extensions"));

	// Test OTG descriptor manipulation
	test.Next(_L("Get OTG Descriptor Size"));
	TInt size;
	gPort.GetOtgDescriptorSize(size);
	test(static_cast<TUint>(size) == KUsbDescSize_Otg);

	test.Next(_L("Get OTG Descriptor"));
	TBuf8<KUsbDescSize_Otg> otgDesc;
	TInt r = gPort.GetOtgDescriptor(otgDesc);
	test(r == KErrNotSupported || r == KErrNone);

	test.Next(_L("Set OTG Descriptor"));
	if (r == KErrNotSupported)
		{
		r = gPort.SetOtgDescriptor(otgDesc);
		test(r == KErrNotSupported);
		}
	else
		{
		otgDesc[0] = KUsbDescSize_Otg;
		otgDesc[1] = KUsbDescType_Otg;
		// The next step is likely to reset KUsbOtgAttr_HnpSupp
		otgDesc[2] = KUsbOtgAttr_SrpSupp;
		r = gPort.SetOtgDescriptor(otgDesc);
		test(r == KErrNone);
		TBuf8<KUsbDescSize_Otg> desc;
		r = gPort.GetOtgDescriptor(desc);
		test(r == KErrNone);
		test(desc.Compare(otgDesc) == 0);
		}

	// Test get OTG features
	test.Next(_L("Get OTG Features"));
	TUint8 features;
	r = gPort.GetOtgFeatures(features);
	if (gSupportsOtg)
		{
		test(r == KErrNone);
		TBool b_HnpEnable = (features & KUsbOtgAttr_B_HnpEnable) ? ETrue : EFalse;
		TBool a_HnpSupport = (features & KUsbOtgAttr_A_HnpSupport) ? ETrue : EFalse;
		TBool a_AltHnpSupport = (features & KUsbOtgAttr_A_AltHnpSupport) ? ETrue : EFalse;
		test.Printf(_L("### OTG Features:\nB_HnpEnable(%d)\nA_HnpSupport(%d)\nA_Alt_HnpSupport(%d)\n"),
					b_HnpEnable, a_HnpSupport, a_AltHnpSupport);
		OstTraceExt3(TRACE_NORMAL, TESTOTGEXTENSIONS_TESTOTGEXTENSIONS, "### OTG Features:\nB_HnpEnable(%d)\nA_HnpSupport(%d)\nA_Alt_HnpSupport(%d)\n",
					b_HnpEnable, a_HnpSupport, a_AltHnpSupport);
		}
	else
		{
		test(r == KErrNotSupported);
		test.Printf(_L("GetOtgFeatures() not supported\n"));
		OstTrace0(TRACE_NORMAL, TESTOTGEXTENSIONS_TESTOTGEXTENSIONS_DUP01, "GetOtgFeatures() not supported\n");
		}

	test.End();
}


static void TestEndpoint0MaxPacketSizes()
	{
	test.Start(_L("Test Endpoint0 MaxPacketSizes"));

	TUint32 sizes = gPort.EndpointZeroMaxPacketSizes();
	TInt r = KErrNone;
	TBool good;
	TInt mpsize = 0;
	for (TInt i = 0; i < 32; i++)
		{
		TUint bit = sizes & (1 << i);
		if (bit != 0)
			{
			switch (bit)
				{
			case KUsbEpSizeCont:
				good = EFalse;
				break;
			case KUsbEpSize8:
				mpsize = 8;
				good = ETrue;
				break;
			case KUsbEpSize16:
				mpsize = 16;
				good = ETrue;
				break;
			case KUsbEpSize32:
				mpsize = 32;
				good = ETrue;
				break;
			case KUsbEpSize64:
				mpsize = 64;
				good = ETrue;
				break;
			case KUsbEpSize128:
			case KUsbEpSize256:
			case KUsbEpSize512:
			case KUsbEpSize1023:
			default:
				good = EFalse;
				break;
				}
			if (good)
				{
				test.Printf(_L("Ep0 supports %d bytes MaxPacketSize\n"), mpsize);
				OstTrace1(TRACE_NORMAL, TESTENDPOINT0MAXPACKETSIZES_TESTENDPOINT0MAXPACKETSIZES, "Ep0 supports %d bytes MaxPacketSize\n", mpsize);
				}
			else
				{
				test.Printf(_L("Bad Ep0 size: 0x%08x, failure will occur\n"), bit);
				OstTrace1(TRACE_NORMAL, TESTENDPOINT0MAXPACKETSIZES_TESTENDPOINT0MAXPACKETSIZES_DUP01, "Bad Ep0 size: 0x%08x, failure will occur\n", bit);
				r = KErrGeneral;
				}
			}
		}
	test(r == KErrNone);

    test.End();
	}


static void TestDeviceControl()
	{
	test.Start(_L("Test Device Control"));

	// This is a quick and crude test, to make sure that we don't get a steaming heap
	// as a result of calling the device control API's.
	test.Next(_L("SetDeviceControl()"));
	TInt r = gPort.SetDeviceControl();
	test(r == KErrNone);
	test.Next(_L("ReleaseDeviceControl()"));
	r = gPort.ReleaseDeviceControl();
	test(r == KErrNone);

	test.End();
	}


static void TestAlternateDeviceStatusNotify()
	{
	test.Start(_L("Test Alternate Device Status Notification"));

	TRequestStatus dev_status;
	TUint deviceState = 0xffffffff;						// put in a nonsense value
	test.Next(_L("AlternateDeviceStatusNotify()"));
	gPort.AlternateDeviceStatusNotify(dev_status, deviceState);
	test.Next(_L("AlternateDeviceStatusNotifyCancel()"));
	gPort.AlternateDeviceStatusNotifyCancel();
	User::WaitForRequest(dev_status);
	test(dev_status == KErrCancel || dev_status == KErrNone);
	if (deviceState & KUsbAlternateSetting)
		{
		TUint setting = (deviceState & ~KUsbAlternateSetting);
		test.Printf(_L("Alternate setting change to setting %d - unexpected"), setting);
		OstTrace1(TRACE_NORMAL, TESTALTERNATEDEVICESTATUSNOTIFY_TESTALTERNATEDEVICESTATUSNOTIFY, "Alternate setting change to setting %d - unexpected", setting);
		test(EFalse);
		}
	else
		{
		switch (deviceState)
			{
		case EUsbcDeviceStateUndefined:
			test.Printf(_L("TestAlternateDeviceStatusNotify: Undefined state\n"));
			OstTrace0(TRACE_NORMAL, TESTALTERNATEDEVICESTATUSNOTIFY_TESTALTERNATEDEVICESTATUSNOTIFY_DUP01, "TestAlternateDeviceStatusNotify: Undefined state\n");
			break;
		case EUsbcDeviceStateAttached:
			test.Printf(_L("TestAlternateDeviceStatusNotify: Attached state\n"));
			OstTrace0(TRACE_NORMAL, TESTALTERNATEDEVICESTATUSNOTIFY_TESTALTERNATEDEVICESTATUSNOTIFY_DUP02, "TestAlternateDeviceStatusNotify: Attached state\n");
			break;
		case EUsbcDeviceStatePowered:
			test.Printf(_L("TestAlternateDeviceStatusNotify: Powered state\n"));
			OstTrace0(TRACE_NORMAL, TESTALTERNATEDEVICESTATUSNOTIFY_TESTALTERNATEDEVICESTATUSNOTIFY_DUP03, "TestAlternateDeviceStatusNotify: Powered state\n");
			break;
		case EUsbcDeviceStateDefault:
			test.Printf(_L("TestAlternateDeviceStatusNotify: Default state\n"));
			OstTrace0(TRACE_NORMAL, TESTALTERNATEDEVICESTATUSNOTIFY_TESTALTERNATEDEVICESTATUSNOTIFY_DUP04, "TestAlternateDeviceStatusNotify: Default state\n");
			break;
		case EUsbcDeviceStateAddress:
			test.Printf(_L("TestAlternateDeviceStatusNotify: Address state\n"));
			OstTrace0(TRACE_NORMAL, TESTALTERNATEDEVICESTATUSNOTIFY_TESTALTERNATEDEVICESTATUSNOTIFY_DUP05, "TestAlternateDeviceStatusNotify: Address state\n");
			break;
		case EUsbcDeviceStateConfigured:
			test.Printf(_L("TestAlternateDeviceStatusNotify: Configured state\n"));
			OstTrace0(TRACE_NORMAL, TESTALTERNATEDEVICESTATUSNOTIFY_TESTALTERNATEDEVICESTATUSNOTIFY_DUP06, "TestAlternateDeviceStatusNotify: Configured state\n");
			break;
		case EUsbcDeviceStateSuspended:
			test.Printf(_L("TestAlternateDeviceStatusNotify: Suspended state\n"));
			OstTrace0(TRACE_NORMAL, TESTALTERNATEDEVICESTATUSNOTIFY_TESTALTERNATEDEVICESTATUSNOTIFY_DUP07, "TestAlternateDeviceStatusNotify: Suspended state\n");
			break;
		case EUsbcNoState:
			test.Printf(_L("TestAlternateDeviceStatusNotify: State buffering error\n"));
			OstTrace0(TRACE_NORMAL, TESTALTERNATEDEVICESTATUSNOTIFY_TESTALTERNATEDEVICESTATUSNOTIFY_DUP08, "TestAlternateDeviceStatusNotify: State buffering error\n");
			test(EFalse);
			break;
		default:
			test.Printf(_L("TestAlternateDeviceStatusNotify: Unknown state\n"));
			OstTrace0(TRACE_NORMAL, TESTALTERNATEDEVICESTATUSNOTIFY_TESTALTERNATEDEVICESTATUSNOTIFY_DUP09, "TestAlternateDeviceStatusNotify: Unknown state\n");
			test(EFalse);
			}
		}

	test.End();
	}


static void TestEndpointStatusNotify()
	{
	test.Start(_L("Test Endpoint Status Notification"));

	TRequestStatus ep_status;
	TUint epStateBitmap = 0xffffffff;						// put in a nonsense value
	test.Next(_L("EndpointStatusNotify()"));
	gPort.EndpointStatusNotify(ep_status, epStateBitmap);
	test.Next(_L("EndpointStatusNotifyCancel()"));
	gPort.EndpointStatusNotifyCancel();
	User::WaitForRequest(ep_status);
	test(ep_status.Int() == KErrCancel);
	test.Next(_L("Check endpoint state bitmap returned"));
	// Our ifc only uses 2 eps + ep0 is automatically granted:
	const TUint usedEpBitmap = (1 << EEndpoint0 | 1 << EEndpoint1 | 1 << EEndpoint2);
	// Must not return info about non existent Eps:
	test((epStateBitmap & ~usedEpBitmap) == 0);
	for (TInt i = 0; i <= 2; i++)
		{
		if ((epStateBitmap & (1 << i)) == EEndpointStateNotStalled)
			{
			test.Printf(_L("EndpointStatusNotify: Ep %d NOT STALLED\n"), i);
			OstTrace1(TRACE_NORMAL, TESTENDPOINTSTATUSNOTIFY_TESTENDPOINTSTATUSNOTIFY, "EndpointStatusNotify: Ep %d NOT STALLED\n", i);
			}
		else
			{
			test.Printf(_L("EndpointStatusNotify: Ep %d STALLED\n"), i);
			OstTrace1(TRACE_NORMAL, TESTENDPOINTSTATUSNOTIFY_TESTENDPOINTSTATUSNOTIFY_DUP01, "EndpointStatusNotify: Ep %d STALLED\n", i);
			}
		}

	test.End();
	}


static void TestEndpointStallStatus()
	{
	test.Start(_L("Test Endpoint Stall Status"));

#ifdef BSW_USB_DRC
	// The MACRO comes from ncp adaptation to indicate that otg is built in.
	// Newly added code for defect ou1cimx1#267421. When otg is built in and the device is not 
	// in peripheral role, the ncp adaptation will return a dummy endpoint for the stall operation.
	// The solution is to check if the device is in peripheral mode, if not, skip the stall
	// operation. A problem is now we can't find a good solution to check the current role of the device.
	// For the test environement, it's ok to use the USB state to confirm and from the test result,
	// it works fine. Later when we find accurate method, we will change the confirmation logic.
	TInt ret = KErrNone;

	TUsbcDeviceState devstate = EUsbcDeviceStateUndefined;
	ret = gPort.DeviceStatus(devstate);
	test(ret == KErrNone);

	if( EUsbcDeviceStateUndefined==devstate )
		{
		test.Printf( _L("Device not connected, state EUsbcDeviceStateUndefined.\n")  );
		OstTrace0(TRACE_NORMAL, TESTENDPOINTSTALLSTATUS_TESTENDPOINTSTALLSTATUS, "Device not connected, state EUsbcDeviceStateUndefined.\n");
		test.Printf( _L("Skipping endpoint stall status tests.\n") );
		OstTrace0(TRACE_NORMAL, TESTENDPOINTSTALLSTATUS_TESTENDPOINTSTALLSTATUS_DUP01, "Skipping endpoint stall status tests.\n");
		test.End();
		return;
		}
#endif

	if (!SupportsEndpointStall())
		{
		test.Printf(_L("*** Not supported - skipping endpoint stall status tests\n"));
		OstTrace0(TRACE_NORMAL, TESTENDPOINTSTALLSTATUS_TESTENDPOINTSTALLSTATUS_DUP02, "*** Not supported - skipping endpoint stall status tests\n");
		test.End();
		return;
		}

	test.Next(_L("Endpoint stall status"));
	TEndpointState epState = EEndpointStateUnknown;
	QueryEndpointState(EEndpoint1);
	QueryEndpointState(EEndpoint2);

	test.Next(_L("Stall Ep1"));
	gPort.HaltEndpoint(EEndpoint1);
	epState = QueryEndpointState(EEndpoint1);
	test(epState == EEndpointStateStalled);

	test.Next(_L("Clear Stall Ep1"));
	gPort.ClearHaltEndpoint(EEndpoint1);
	epState = QueryEndpointState(EEndpoint1);
	test(epState == EEndpointStateNotStalled);

	test.Next(_L("Stall Ep2"));
	gPort.HaltEndpoint(EEndpoint2);
	epState = QueryEndpointState(EEndpoint2);
	test(epState == EEndpointStateStalled);

	test.Next(_L("Clear Stall Ep2"));
	gPort.ClearHaltEndpoint(EEndpoint2);
	epState = QueryEndpointState(EEndpoint2);
	test(epState == EEndpointStateNotStalled);

	test.End();
	}


static void CloseChannel()
	{
	test.Start(_L("Close Channel"));

	test.Next(_L("Disconnect Device from Host"));
	TInt r = gPort.DeviceDisconnectFromHost();
	test(r != KErrGeneral);

	if (gSupportsOtg)
		{
		test.Next(_L("Stop OTG stack"));
		gOTG.StopStacks();
		test.Next(_L("Close OTG Channel"));
		gOTG.Close();
		test.Next(_L("Free OTG LDD"));
		r = User::FreeLogicalDevice(RUsbOtgDriver::Name());
		test(r == KErrNone);
		}

	test.Next(_L("Close USB Channel"));
	gPort.Close();
	test.Next(_L("Free USB LDD"));
	r = User::FreeLogicalDevice(KUsbDeviceName);
	test(r == KErrNone);

	test.End();
	}


static const TInt KPrologue = 0;
static const TInt KMain     = 1;
static const TInt KEpilogue = 2;

static TInt RunTests(void* /*aArg*/)
	{
	static TInt step = KPrologue;
	static TReal loops = 0;

	switch (step)
		{
	case KPrologue:
		test.Title();
		// outermost test begin
		test.Start(_L("Test of USB APIs not requiring a host connection\n"));
		if (SupportsUsb())
			{
			step = KMain;
			}
		else
			{
			step = KEpilogue;
			test.Printf(_L("*** Test platform does not support USB - skipping all tests\n"));
			OstTrace0(TRACE_NORMAL, RUNTESTS_RUNTESTS, "*** Test platform does not support USB - skipping all tests\n");
			}
		return ETrue;
	case KMain:
		OpenChannel();
		SetupInterface();
		TestDescriptorManipulation();
		TestOtgExtensions();
		TestEndpoint0MaxPacketSizes();
		TestDeviceControl();
		TestAlternateDeviceStatusNotify();
		TestEndpointStatusNotify();
		TestEndpointStallStatus();
		CloseChannel();
		loops++;
		if (gSoak && (gKeychar != EKeyEscape))
			{
			step = KMain;
			}
		else
			{
			step = KEpilogue;
			}
		return ETrue;
	case KEpilogue:
		test.Printf(_L("USBAPI tests were run %.0f time(s)\n"), loops);
		OstTraceExt1(TRACE_NORMAL, RUNTESTS_RUNTESTS_DUP01, "USBAPI tests were run %.0f time(s)\n", loops);
		// outermost test end
		test.End();
		CActiveScheduler::Stop();
		return EFalse;
		}
	return EFalse;
	}


static void RunAppL()
	{
	// Create the active scheduler
	CActiveScheduler* scheduler = new (ELeave) CActiveScheduler();
	// Push active scheduler onto the cleanup stack
	CleanupStack::PushL(scheduler);
	// Install as the active scheduler
	CActiveScheduler::Install(scheduler);

	// Create console handler
	CConsoleBase* console =
		Console::NewL(_L("T_USBAPI - USB Client Test Program"), TSize(KConsFullScreen, KConsFullScreen));
	CleanupStack::PushL(console);
	// Make this one also RTest's console
	test.SetConsole(console);

	// Create keypress notifier active object
	CActiveKeypressNotifier* keypress_notifier = CActiveKeypressNotifier::NewL(console);
	test(keypress_notifier != NULL);
	CleanupStack::PushL(keypress_notifier);
	keypress_notifier->RequestCharacter();

	// Create long-running test task active object
	CIdle* active_test = CIdle::NewL(CActive::EPriorityIdle);
	test(active_test != NULL);
	CleanupStack::PushL(active_test);
	active_test->Start(TCallBack(RunTests));

	// Start active scheduler
	CActiveScheduler::Start();

	// Suspend thread for a short while
	User::After(1000000);

	// active_test, keypress_notifier, console, scheduler
	CleanupStack::PopAndDestroy(4);

	return;
	}


GLDEF_C TInt E32Main()
	{

	CTrapCleanup* cleanup = CTrapCleanup::New();			// get clean-up stack

	__UHEAP_MARK;

	_LIT(KArg, "soak");
	TBuf<64> c;
	User::CommandLine(c);
	if (c.CompareF(KArg) == 0)
		gSoak = ETrue;
	else
		gSoak = EFalse;
	TRAPD(r, RunAppL());
	__ASSERT_ALWAYS(!r, User::Panic(_L("E32EX"), r));

	__UHEAP_MARKEND;

	delete cleanup;											// destroy clean-up stack
	return KErrNone;
	}
