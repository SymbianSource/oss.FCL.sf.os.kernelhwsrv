// Copyright (c) 2000-2010 Nokia Corporation and/or its subsidiary(-ies).
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
// e32test/usb/t_usb_device/src/activecontrol.cpp
// USB Test Program T_USB_DEVICE, functional part.
// Device-side part, to work against T_USB_HOST running on the host.
//
//


#include "general.h"
#include "usblib.h"											// Helpers
#include "config.h"
#include "activecontrol.h"
#include "apitests.h"
#include "activerw.h"
#include "d32otgdi.h"
#ifdef USB_SC
#include "tranhandleserver.h"
#endif

#include "OstTraceDefinitions.h"
#ifdef OST_TRACE_COMPILER_IN_USE
#include "activecontrolTraces.h"
#endif


void StartMassStorage(RDEVCLIENT* aPort);
void StopMassStorage(RDEVCLIENT* aPort);
void OpenStackIfOtg();
void CloseStackIfOtg();

_LIT(KOtgdiLddFilename, "otgdi");
static TBool gSupportsOtg;
static RUsbOtgDriver gOtgPort;

enum Ep0Requests
	{
	EStop = 0x1,
	EVersion = 0x10,
	ETestParam = 0x20,
	ETestResult = 0x30,
	ETestFail = 0x40,
	ETestConnect = 0x50,
	ETestDisconnect = 0x60,
	ETestMassStorage = 0x70,
	ETestIdleCounter = 0x80,
	};

extern RTest test;
#ifdef USB_SC
extern TBool gShareHandle;
#endif
extern TBool gVerbose;
extern TBool gSkip;
extern TBool gTempTest;
extern TBool gStopOnFail;
extern TBool gAltSettingOnNotify;
extern TInt gSoakCount;
extern CActiveRW* gRW[KMaxConcurrentTests];				// the USB read/write active object
extern IFConfigPtr gInterfaceConfig [128] [KMaxInterfaceSettings];
extern TInt gActiveTestCount;
#ifdef USB_SC
extern RChunk gChunk;
#endif

TInt firstBulkOutEndpoint = -1;

_LIT(KTestIdleCounterChunkName, "TestIdleCounter");
_LIT(KTestIdleCounterPanic, "IdleCounter");

enum TTestIdleCounterPanic
	{
	ETestIdleCounterWrongCommand
	};

enum TTestIdleCounterCommand
	{
	ETestIdleCounterDoNothing,
	ETestIdleCounterReset,
	ETestIdleCounterClose
	};

struct TTestIdleCounter
	{
	volatile TInt64 iCounter;
	volatile TTestIdleCounterCommand iCommand;
	};

TInt IdleCounterThread(TAny*)
	{
	TInt r;
	//
	RThread().SetPriority(EPriorityAbsoluteVeryLow);
	//
	RChunk chunk;
	r = chunk.CreateGlobal(KTestIdleCounterChunkName,
			sizeof(struct TTestIdleCounter),
			sizeof(struct TTestIdleCounter) + 1);
	if (r == KErrNone)
		{
		struct TTestIdleCounter* counter = (struct TTestIdleCounter*) chunk.Base();
		counter->iCounter = 0;
		counter->iCommand = ETestIdleCounterDoNothing;
		//
		FOREVER
			{
			TInt command = counter->iCommand;
			if (command == ETestIdleCounterReset)
				{
				counter->iCounter = 0;
				counter->iCommand = ETestIdleCounterDoNothing;
				}
			else if (command == ETestIdleCounterClose)
				{
				break;
				}
			else if (command != ETestIdleCounterDoNothing)
				{
				RThread().Panic(KTestIdleCounterPanic, ETestIdleCounterWrongCommand);
				}
			//
			counter->iCounter++;
			}
		//
		chunk.Close();
		}
	return r;
	}

//
// --- class CActiveControl ---------------------------------------------------------
//

CActiveControl::CActiveControl(CConsoleBase* aConsole, TDes * aConfigFile, TDes * aScriptFile)
	: CActive(EPriorityNormal),
	  iConsole(aConsole),
	  iSoftwareConnect(EFalse),
	  iSupportResourceAllocationV2(EFalse),
	  iHighSpeed(EFalse),
	  iConfigFileName(aConfigFile),
	  iScriptFileName(aScriptFile),
	  iEp0PacketSize(0)
	{}


CActiveControl* CActiveControl::NewLC(CConsoleBase* aConsole, TDes * aConfigFile, TDes * aScriptFile)
	{
	CActiveControl* self = new (ELeave) CActiveControl(aConsole, aConfigFile, aScriptFile);
	CleanupStack::PushL(self);
	self->ConstructL();
	return self;
	}


CActiveControl* CActiveControl::NewL(CConsoleBase* aConsole, TDes * aConfigFile, TDes * aScriptFile)
	{
	CActiveControl* self = NewLC(aConsole, aConfigFile, aScriptFile);
	CleanupStack::Pop();
	return self;
	}

void CActiveControl::ConstructL()
	{
	CActiveScheduler::Add(this);
#ifdef USB_SC
	if (gShareHandle)
		{
		// to do add call to server to transfer config file name
		iTranHandleServer = CTranHandleServer::NewL(*this);
		RTransferSrv aSrv;
		test.Next (_L("ConstructL"));
		User::LeaveIfError(aSrv.Connect());
		CleanupClosePushL(aSrv);	
		test.Next (_L("ConstructL1"));
		User::LeaveIfError(aSrv.SetConfigFileName(*iConfigFileName));
		test.Next (_L("ConstructL2"));
		CleanupStack::Pop();
		aSrv.Close();
		return;
		}
#endif
	TInt r;

	User::LeaveIfError(iFs.Connect());

	test.Start (_L("Configuration"));

	test_Compare(iConfigFileName->Length(),!=,0);

	iTimer.CreateLocal();
	iPending = EPendingNone;

	test.Next (_L("Open configuration file"));
	// set the session path to use the ROM if no drive specified
	r=iFs.SetSessionPath(_L("Z:\\test\\"));
	test_KErrNone(r);

	r = iConfigFile.Open(iFs, * iConfigFileName, EFileShareReadersOnly | EFileStreamText | EFileRead);
	test_KErrNone(r);
	TUSB_VERBOSE_PRINT1("Configuration file %s Opened successfully", iConfigFileName->PtrZ());
	if(gVerbose)
	    {
	    OstTraceExt1(TRACE_VERBOSE, CACTIVECONTROL_CONSTRUCTL, "Configuration file %S Opened successfully", *iConfigFileName);
	    }

	test.Next (_L("Process configuration file"));
	test(ProcessConfigFile (iConfigFile,iConsole,&iLddPtr));

	iConfigFile.Close();

	test.Next (_L("LDD in configuration file"));
	test_NotNull(iLddPtr);

	LDDConfigPtr lddPtr = iLddPtr;
	TInt nextPort = 0;
	while (lddPtr != NULL)
		{
		// Load logical driver (LDD)
		// (There's no physical driver (PDD) with USB: it's a kernel extension DLL which
		//  was already loaded at boot time.)
		test.Next (_L("Loading USB LDD"));
		TUSB_VERBOSE_PRINT1("Loading USB LDD ",lddPtr->iName.PtrZ());
		if(gVerbose)
		    {
		    OstTraceExt1(TRACE_VERBOSE, CACTIVECONTROL_CONSTRUCTL_DUP01, "Loading USB LDD:%S ", lddPtr->iName);
		    }
		r = User::LoadLogicalDevice(lddPtr->iName);
		test(r == KErrNone || r == KErrAlreadyExists);

		IFConfigPtr ifPtr = lddPtr->iIFPtr;

		test.Next (_L("Opening Channels"));
		for (TInt portNumber = nextPort; portNumber < nextPort+lddPtr->iNumChannels; portNumber++)
			{
			test_Compare(lddPtr->iNumChannels,>,0);

			// Open USB channel
			r = iPort[portNumber].Open(0);
			test_KErrNone(r);
			TUSB_VERBOSE_PRINT("Successfully opened USB port");
			if(gVerbose)
			    {
			    OstTrace0(TRACE_VERBOSE, CACTIVECONTROL_CONSTRUCTL_DUP02, "Successfully opened USB port");
			    }

			// Query the USB device/Setup the USB interface
			if (portNumber == nextPort)
				{
				// Change some descriptors to contain suitable values
				SetupDescriptors(lddPtr, &iPort[portNumber]);
				}

			if (portNumber == 0)
				{
				QueryUsbClientL(lddPtr, &iPort[portNumber]);
				}

			test_NotNull(ifPtr);

			if (iSupportResourceAllocationV2)
				{
				PopulateInterfaceResourceAllocation(ifPtr, portNumber);
				}

			IFConfigPtr defaultIfPtr = ifPtr;
			SetupInterface(&ifPtr,portNumber);

			#ifdef USB_SC
			RChunk *tChunk = &gChunk;
			test_KErrNone(iPort[portNumber].FinalizeInterface(tChunk));
			#endif

			if (!iSupportResourceAllocationV2)
				{
				// 	allocate endpoint DMA and double buffering for all endpoints on default interface when using resource allocation v1 api
				for (TUint8 i = 1; i <= defaultIfPtr->iInfoPtr->iTotalEndpointsUsed; i++)
					{
					defaultIfPtr->iEpDMA[i-1] ? AllocateEndpointDMA(&iPort[portNumber],(TENDPOINTNUMBER)i) : DeAllocateEndpointDMA(&iPort[portNumber],(TENDPOINTNUMBER)i);
					#ifndef USB_SC
					defaultIfPtr->iEpDoubleBuff[i-1] ? AllocateDoubleBuffering(&iPort[portNumber],(TENDPOINTNUMBER)i) : DeAllocateDoubleBuffering(&iPort[portNumber],(TENDPOINTNUMBER)i);
					#endif
					}
				}
			}

		// Check for OTG support
		TBuf8<KUsbDescSize_Otg> otg_desc;
		r = iPort[0].GetOtgDescriptor(otg_desc);
		if (!(r == KErrNotSupported || r == KErrNone))
			{
			OstTrace1(TRACE_NORMAL, CACTIVECONTROL_CONSTRUCTL_DUP08, "Error %d while fetching OTG descriptor", r);
			User::Leave(-1);
			return;
			}
		gSupportsOtg = (r != KErrNotSupported) ? ETrue : EFalse;

		OpenStackIfOtg();
		
		iTotalChannels += lddPtr->iNumChannels;
		nextPort += lddPtr->iNumChannels;
		lddPtr = lddPtr->iPtrNext;
		}

	TUSB_VERBOSE_PRINT("All Interfaces and Alternate Settings successfully set up");
	if(gVerbose)
	    {
	    OstTrace0(TRACE_VERBOSE, CACTIVECONTROL_CONSTRUCTL_DUP03, "All Interfaces and Alternate Settings successfully set up");
	    }

	test.Next (_L("Start Idle Counter Thread"));
	r = iIdleCounterThread.Create(_L("IdleCounter"), IdleCounterThread, KDefaultStackSize, KMinHeapSize, KMinHeapSize, NULL);
	test_KErrNone(r);
	iIdleCounterThread.Resume();
	// Allow some time for low-priority counter process
	User::After(100000); // 0.1 second
	r = iIdleCounterChunk.OpenGlobal(KTestIdleCounterChunkName, EFalse);
	test_KErrNone(r);
	iIdleCounter = (struct TTestIdleCounter*) iIdleCounterChunk.Base();
	test_NotNull(iIdleCounter);
	// Allow some time for low-priority counter process
	User::After(100000); // 0.1 second
	TInt64 val1 = iIdleCounter->iCounter;
	User::After(1000000); // 1 second
	TInt64 val2 = iIdleCounter->iCounter;
	TUSB_PRINT1("Idle Counter when test inactive: %Ldinc/ms", (val2 - val1) / 1000);
	OstTraceExt1(TRACE_NORMAL, CACTIVECONTROL_CONSTRUCTL_DUP04, "Idle Counter when test inactive: %Ldinc/ms", (val2 - val1) / 1000);

	test.Next (_L("Enumeration..."));
	r = ReEnumerate();
	test_KErrNone(r);

	TUSB_VERBOSE_PRINT("Device successfully re-enumerated\n");
	if(gVerbose)
	    {
	    OstTrace0(TRACE_VERBOSE, CACTIVECONTROL_CONSTRUCTL_DUP05, "Device successfully re-enumerated\n");
	    }


	if (iLddPtr->iHighSpeed && !gSkip)
		{
		test.Next (_L("High Speed"));
		test(iHighSpeed);
		}

	test.Next (_L("Create Notifiers"));
	for (TInt portNumber = 0; portNumber < iTotalChannels; portNumber++)
		{

		// Create device state active object
		iDeviceStateNotifier[portNumber] = CActiveDeviceStateNotifier::NewL(iConsole, &iPort[portNumber], portNumber);
		test_NotNull(iDeviceStateNotifier[portNumber]);
		iDeviceStateNotifier[portNumber]->Activate();
		TUSB_VERBOSE_PRINT("Created device state notifier");
		if(gVerbose)
		    {
		    OstTrace0(TRACE_VERBOSE, CACTIVECONTROL_CONSTRUCTL_DUP06, "Created device state notifier");
		    }

		// Create endpoint stall status active object
		iStallNotifier[portNumber] = CActiveStallNotifier::NewL(iConsole, &iPort[portNumber]);
		test_NotNull(iStallNotifier[portNumber]);
		iStallNotifier[portNumber]->Activate();
		TUSB_VERBOSE_PRINT("Created stall notifier");
		if(gVerbose)
		    {
		    OstTrace0(TRACE_VERBOSE, CACTIVECONTROL_CONSTRUCTL_DUP07, "Created stall notifier");
		    }

		TestInvalidSetInterface (&iPort[portNumber],iNumInterfaceSettings[portNumber]);
		TestInvalidReleaseInterface (&iPort[portNumber],iNumInterfaceSettings[portNumber]);

		}

	test.Next (_L("Endpoint Zero Max Packet Sizes"));
	TUint ep0Size = iPort[0].EndpointZeroMaxPacketSizes();
	switch (ep0Size)
		{
		case KUsbEpSize8 :
			iEp0PacketSize = 8;
			break;

		case KUsbEpSize16 :
			iEp0PacketSize = 16;
			break;

		case KUsbEpSize32 :
			iEp0PacketSize = 32;
			break;

		case KUsbEpSize64 :
			iEp0PacketSize = 64;
			break;

		default:
			iEp0PacketSize = 0;
			break;
		}
	test_Compare(iEp0PacketSize,>,0);

	test.Next (_L("Set Device Control"));
	r = iPort[0].SetDeviceControl();
	test_KErrNone(r);

	#ifdef USB_SC
	r = iPort[0].OpenEndpoint(iEp0Buf,0);
	test_KErrNone(r);
	#endif

	test.End();

	}

void CActiveControl::ReConnect()
	{
	TInt r;

	test.Start (_L("Reconnecting USB"));
	LDDConfigPtr lddPtr = iLddPtr;
	TInt nextPort = 0;
	while (lddPtr != NULL)
		{
		IFConfigPtr ifPtr = lddPtr->iIFPtr;

		test.Next (_L("Opening Channels"));
		for (TInt portNumber = nextPort; portNumber < nextPort+lddPtr->iNumChannels; portNumber++)
			{
			// Open USB channel
			r = iPort[portNumber].Open(0);
			test_KErrNone(r);
			TUSB_VERBOSE_PRINT("Successfully opened USB port");
			if(gVerbose)
			    {
			    OstTrace0(TRACE_VERBOSE, CACTIVECONTROL_RECONNECT, "Successfully opened USB port");
			    }

			// Query the USB device/Setup the USB interface
			if (portNumber == nextPort)
				{
				// Change some descriptors to contain suitable values
				SetupDescriptors(lddPtr, &iPort[portNumber]);
				}

			IFConfigPtr defaultIfPtr = ifPtr;
			SetupInterface(&ifPtr,portNumber);

			#ifdef USB_SC
			RChunk *tChunk = &gChunk;
			test_KErrNone(iPort[portNumber].FinalizeInterface(tChunk));
			#endif

			if (!iSupportResourceAllocationV2)
				{
				// 	allocate endpoint DMA and double buffering for all endpoints on default interface with resource allocation v1 api
				for (TUint8 i = 1; i <= defaultIfPtr->iInfoPtr->iTotalEndpointsUsed; i++)
					{
					defaultIfPtr->iEpDMA[i-1] ? AllocateEndpointDMA(&iPort[portNumber],(TENDPOINTNUMBER)i) : DeAllocateEndpointDMA(&iPort[portNumber],(TENDPOINTNUMBER)i);
					#ifndef USB_SC
					defaultIfPtr->iEpDoubleBuff[i-1] ? AllocateDoubleBuffering(&iPort[portNumber],(TENDPOINTNUMBER)i) : DeAllocateDoubleBuffering(&iPort[portNumber],(TENDPOINTNUMBER)i);
					#endif
					}
				}
			}

		nextPort += lddPtr->iNumChannels;
		lddPtr = lddPtr->iPtrNext;
		}

	TUSB_VERBOSE_PRINT("All Interfaces and Alternate Settings successfully set up");
	if(gVerbose)
	    {
	    OstTrace0(TRACE_VERBOSE, CACTIVECONTROL_RECONNECT_DUP01, "All Interfaces and Alternate Settings successfully set up");
	    }

	test.Next (_L("Enumeration..."));
	r = ReEnumerate();
	test_KErrNone(r);

	TUSB_VERBOSE_PRINT("Device successfully re-enumerated\n");
	if(gVerbose)
	    {
	    OstTrace0(TRACE_VERBOSE, CACTIVECONTROL_RECONNECT_DUP02, "Device successfully re-enumerated\n");
	    }

	for (TInt portNumber = 0; portNumber < iTotalChannels; portNumber++)
		{
		// Create device state active object
		iDeviceStateNotifier[portNumber] = CActiveDeviceStateNotifier::NewL(iConsole, &iPort[portNumber], portNumber);
		test_NotNull(iDeviceStateNotifier[portNumber]);
		iDeviceStateNotifier[portNumber]->Activate();
		TUSB_VERBOSE_PRINT("Created device state notifier");
		if(gVerbose)
		    {
		    OstTrace0(TRACE_VERBOSE, CACTIVECONTROL_RECONNECT_DUP03, "Created device state notifier");
		    }

		// Create endpoint stall status active object
		iStallNotifier[portNumber] = CActiveStallNotifier::NewL(iConsole, &iPort[portNumber]);
		test_NotNull(iStallNotifier[portNumber]);
		iStallNotifier[portNumber]->Activate();
		TUSB_VERBOSE_PRINT("Created stall notifier");
		if(gVerbose)
		    {
		    OstTrace0(TRACE_VERBOSE, CACTIVECONTROL_RECONNECT_DUP04, "Created stall notifier");
		    }

		if (portNumber == 0)
			{
			test.Next (_L("Set Device Control"));
			r = iPort[portNumber].SetDeviceControl();
			test_KErrNone(r);

			#ifdef USB_SC
			r = iPort[portNumber].OpenEndpoint(iEp0Buf,0);
			test_KErrNone(r);
			#endif

			}
		}

	test.End();
	}

void CActiveControl::FillEndpointsResourceAllocation(IFConfigPtr aIfCfg)
	{

	#ifdef USB_SC
		TUsbcScInterfaceInfo* iInfoPtr = aIfCfg->iInfoPtr;
	#else
		TUsbcInterfaceInfo* iInfoPtr = aIfCfg->iInfoPtr;
	#endif

	// 	fill resource allocation info in the endpoint info with resource allocation v2
	for (TUint8 i = 1; i <= iInfoPtr->iTotalEndpointsUsed; i++)
		{
		if (aIfCfg->iEpDMA[i-1])
			{
			iInfoPtr->iEndpointData[i-1].iFeatureWord1 |= KUsbcEndpointInfoFeatureWord1_DMA;
			}
		else
			{
			iInfoPtr->iEndpointData[i-1].iFeatureWord1 &= (~KUsbcEndpointInfoFeatureWord1_DMA);
			}
		#ifndef USB_SC
		if (aIfCfg->iEpDoubleBuff[i-1])
			{
			iInfoPtr->iEndpointData[i-1].iFeatureWord1 |= KUsbcEndpointInfoFeatureWord1_DoubleBuffering;
			}
		else
			{
			iInfoPtr->iEndpointData[i-1].iFeatureWord1 &= (~KUsbcEndpointInfoFeatureWord1_DoubleBuffering);
			}
		#endif
		}
	}

// all alternative settings of the interface 'aFirstIfCfg' will be populated
void CActiveControl::PopulateInterfaceResourceAllocation(IFConfigPtr aFirstIfCfg, TInt aPortNumber)
	{
	FillEndpointsResourceAllocation(aFirstIfCfg);

	IFConfigPtr ifCfgPtr = aFirstIfCfg->iPtrNext;
	while (ifCfgPtr != NULL)
		{
		if (ifCfgPtr->iAlternateSetting)
			{
			FillEndpointsResourceAllocation(ifCfgPtr);
			ifCfgPtr = ifCfgPtr->iPtrNext;
			}
		else
			{
			ifCfgPtr = NULL;
			}
		}
	}

void CActiveControl::SetupInterface(IFConfigPtr* aIfPtr, TInt aPortNumber)
	{
	test.Start (_L("Setup Interface"));

	// first of all set the default interface
	TUSB_PRINT2 ("Set Default Interface with %d endpoints bandwidth 0x%x",(*aIfPtr)->iInfoPtr->iTotalEndpointsUsed,(*aIfPtr)->iBandwidthIn | (*aIfPtr)->iBandwidthOut);
	OstTraceExt2 (TRACE_NORMAL, CACTIVECONTROL_SETUPINTERFACE, "Set Default Interface with %d endpoints bandwidth 0x%x",(*aIfPtr)->iInfoPtr->iTotalEndpointsUsed,(*aIfPtr)->iBandwidthIn | (*aIfPtr)->iBandwidthOut);
	#ifdef USB_SC
	TUsbcScInterfaceInfoBuf ifc = *((*aIfPtr)->iInfoPtr);
	TInt r = iPort[aPortNumber].SetInterface(0, ifc);
	#else
	TUsbcInterfaceInfoBuf ifc = *((*aIfPtr)->iInfoPtr);
	TInt r = iPort[aPortNumber].SetInterface(0, ifc, (*aIfPtr)->iBandwidthIn | (*aIfPtr)->iBandwidthOut);
	#endif
	test_KErrNone(r);

	TBuf8<KUsbDescSize_Interface> ifDescriptor;
	r = iPort[aPortNumber].GetInterfaceDescriptor(0, ifDescriptor);
	test_KErrNone(r);

	// Check the interface descriptor
	test(ifDescriptor[KIfcDesc_SettingOffset] == 0 && ifDescriptor[KIfcDesc_NumEndpointsOffset] == (*aIfPtr)->iInfoPtr->iTotalEndpointsUsed &&
	    ifDescriptor[KIfcDesc_ClassOffset] == (*aIfPtr)->iInfoPtr->iClass.iClassNum &&
	    ifDescriptor[KIfcDesc_SubClassOffset] == (*aIfPtr)->iInfoPtr->iClass.iSubClassNum &&
	    ifDescriptor[KIfcDesc_ProtocolOffset] == (*aIfPtr)->iInfoPtr->iClass.iProtocolNum);

	if ((*aIfPtr)->iNumber != 0 && ifDescriptor[KIfcDesc_NumberOffset] != (*aIfPtr)->iNumber)
		{
		ifDescriptor[KIfcDesc_NumberOffset] = (*aIfPtr)->iNumber;
		r = iPort[aPortNumber].SetInterfaceDescriptor(0, ifDescriptor);
		test_KErrNone(r);
		}
	else
		{
		(*aIfPtr)->iNumber = ifDescriptor[KIfcDesc_NumberOffset];
		}
	TUint8 interfaceNumber = (*aIfPtr)->iNumber;
	TUSB_PRINT1 ("Interface Number %d",interfaceNumber);
	OstTrace1 (TRACE_NORMAL, CACTIVECONTROL_SETUPINTERFACE_DUP01, "Interface Number %d",interfaceNumber);

	// Check all endpoint descriptors
	TBuf8<KUsbDescSize_AudioEndpoint> epDescriptor;
	for (TUint i = 0; i < (*aIfPtr)->iInfoPtr->iTotalEndpointsUsed; i++)
		{
		if (!gSkip)
			{
			TestEndpointDescriptor (&iPort[aPortNumber],0,i+1,(*aIfPtr)->iInfoPtr->iEndpointData[i]);

			}

		if (firstBulkOutEndpoint < 0 && ((*aIfPtr)->iInfoPtr->iEndpointData[i].iDir & KUsbEpDirOut) &&
			(*aIfPtr)->iInfoPtr->iEndpointData[i].iType == KUsbEpTypeBulk)
			{
			firstBulkOutEndpoint = i+1;
			}
		}

	TUSB_PRINT1 ("Interface number is %d",interfaceNumber);
	OstTrace1 (TRACE_NORMAL, CACTIVECONTROL_SETUPINTERFACE_DUP02, "Interface number is %d",interfaceNumber);
	(*aIfPtr)->iPortNumber = aPortNumber;
	gInterfaceConfig [interfaceNumber] [0] = *aIfPtr;

	TInt alternateNumber = 1;
	// check for any alternatate interfaces and set any that are found
	* aIfPtr = (*aIfPtr)->iPtrNext;
	if (* aIfPtr != NULL)
		{
		test(SupportsAlternateInterfaces());

		IFConfigPtr ifPtr = *aIfPtr;
		while (ifPtr != NULL)
			{
			if (ifPtr->iAlternateSetting)
				{
				ifc = *(ifPtr->iInfoPtr);
				#ifdef USB_SC
				TUSB_PRINT2 ("Set Alternate Interface Setting %d with %d endpoints",alternateNumber,ifPtr->iInfoPtr->iTotalEndpointsUsed);
				OstTraceExt2 (TRACE_NORMAL, CACTIVECONTROL_SETUPINTERFACE_DUP03, "Set Alternate Interface Setting %d with %d endpoints",alternateNumber,ifPtr->iInfoPtr->iTotalEndpointsUsed);
				r = iPort[aPortNumber].SetInterface(alternateNumber, ifc);
				#else
				TUSB_PRINT3 ("Set Alternate Interface Setting %d with %d endpoints bandwidth 0x%x",alternateNumber,ifPtr->iInfoPtr->iTotalEndpointsUsed,ifPtr->iBandwidthIn | iLddPtr->iIFPtr->iBandwidthOut);
				OstTraceExt3 (TRACE_NORMAL, CACTIVECONTROL_SETUPINTERFACE_DUP04, "Set Alternate Interface Setting %d with %u endpoints bandwidth 0x%x",(TInt32)alternateNumber,(TUint32)ifPtr->iInfoPtr->iTotalEndpointsUsed,(TUint32)(ifPtr->iBandwidthIn | iLddPtr->iIFPtr->iBandwidthOut));
				r = iPort[aPortNumber].SetInterface(alternateNumber, ifc, ifPtr->iBandwidthIn | iLddPtr->iIFPtr->iBandwidthOut);
				#endif
				test_KErrNone(r);

				r = iPort[aPortNumber].GetInterfaceDescriptor(alternateNumber, ifDescriptor);
				test_KErrNone(r);

				// Check the interface descriptor
				test(ifDescriptor[KIfcDesc_SettingOffset] == alternateNumber && ifDescriptor[KIfcDesc_NumEndpointsOffset] == (*aIfPtr)->iInfoPtr->iTotalEndpointsUsed &&
				    ifDescriptor[KIfcDesc_ClassOffset] == (*aIfPtr)->iInfoPtr->iClass.iClassNum &&
				    ifDescriptor[KIfcDesc_SubClassOffset] == (*aIfPtr)->iInfoPtr->iClass.iSubClassNum &&
				    ifDescriptor[KIfcDesc_ProtocolOffset] == (*aIfPtr)->iInfoPtr->iClass.iProtocolNum);

				// Check all endpoint descriptors
				for (TUint i = 0; i < ifPtr->iInfoPtr->iTotalEndpointsUsed; i++)
					{
					TInt desc_size;
					r = iPort[aPortNumber].GetEndpointDescriptorSize(alternateNumber, i+1, desc_size);
					test_KErrNone(r);
					test_Equal(KUsbDescSize_Endpoint + (*aIfPtr)->iInfoPtr->iEndpointData[i].iExtra,static_cast<TUint>(desc_size));

					r = iPort[aPortNumber].GetEndpointDescriptor(alternateNumber, i+1, epDescriptor);
					test_KErrNone(r);

					test((((*aIfPtr)->iInfoPtr->iEndpointData[i].iDir & KUsbEpDirIn) && (epDescriptor[KEpDesc_AddressOffset] & 0x80) ||
						!((*aIfPtr)->iInfoPtr->iEndpointData[i].iDir & KUsbEpDirIn) && !(epDescriptor[KEpDesc_AddressOffset] & 0x80)) &&
						EpTypeMask2Value((*aIfPtr)->iInfoPtr->iEndpointData[i].iType) == (TUint)(epDescriptor[KEpDesc_AttributesOffset] & 0x03) &&
						(*aIfPtr)->iInfoPtr->iEndpointData[i].iInterval == epDescriptor[KEpDesc_IntervalOffset]);


					if (!gSkip && (*aIfPtr)->iInfoPtr->iEndpointData[i].iExtra)
						{
						test.Next(_L("Extended Endpoint Descriptor Manipulation"));
						TUint8 addr = 0x85;										// bogus address
						if (epDescriptor[KEpDesc_SynchAddressOffset] == addr)
							addr++;
						epDescriptor[KEpDesc_SynchAddressOffset] = addr;
						r = iPort[aPortNumber].SetEndpointDescriptor(alternateNumber, i+1, epDescriptor);
						test_KErrNone(r);

						TBuf8<KUsbDescSize_AudioEndpoint> descriptor2;
						r = iPort[aPortNumber].GetEndpointDescriptor(alternateNumber, i+1, descriptor2);
						test_KErrNone(r);

						test.Next(_L("Compare endpoint descriptor with value set"));
						r = descriptor2.Compare(epDescriptor);
						test_KErrNone(r);
						}
					}


				// if no error move on to the next interface
				ifPtr->iPortNumber = aPortNumber;
				ifPtr->iNumber = interfaceNumber;
				gInterfaceConfig [interfaceNumber] [alternateNumber] = ifPtr;

				alternateNumber++;
				ifPtr = ifPtr->iPtrNext;
				* aIfPtr = ifPtr;
				}
			else
				{
				ifPtr = NULL;
				}
			}
		}
	iNumInterfaceSettings[aPortNumber] = alternateNumber;
	if (!gSkip)
		{
		TestInvalidSetInterface (&iPort[aPortNumber],iNumInterfaceSettings[aPortNumber]);
		TestInvalidReleaseInterface (&iPort[aPortNumber],iNumInterfaceSettings[aPortNumber]);

		TestDescriptorManipulation(iLddPtr->iHighSpeed,&iPort[aPortNumber],alternateNumber);
		TestOtgExtensions(&iPort[aPortNumber]);
		TestEndpoint0MaxPacketSizes(&iPort[aPortNumber]);
		}

	test.End();
	}


CActiveControl::~CActiveControl()
	{
	TUSB_PRINT("CActiveControl::~CActiveControl()");
	OstTrace0(TRACE_NORMAL, CACTIVECONTROL_DCACTIVECONTROL, "CActiveControl::~CActiveControl()");

	Cancel();

	iTimer.Close();

	// delete interfaces
	while (iLddPtr->iIFPtr)
		{
		IFConfigPtr* ifPtrPtr = & iLddPtr->iIFPtr;
		while ((*ifPtrPtr)->iPtrNext)
			{
			ifPtrPtr = &(*ifPtrPtr)->iPtrNext;
			}
		delete (*ifPtrPtr)->iInfoPtr->iString;
		delete (*ifPtrPtr)->iInfoPtr;
		delete (*ifPtrPtr);
		* ifPtrPtr = NULL;
		}

	while (iLddPtr)
		{
		LDDConfigPtr* lddPtrPtr = &iLddPtr;
		while ((*lddPtrPtr)->iPtrNext)
			{
			lddPtrPtr = &(*lddPtrPtr)->iPtrNext;
			}
		delete (*lddPtrPtr)->iManufacturer;
		delete (*lddPtrPtr)->iProduct;
		delete (*lddPtrPtr)->iSerialNumber;
		delete (*lddPtrPtr);
		* lddPtrPtr = NULL;
		}
#ifdef USB_SC
	delete iTranHandleServer;
	TUSB_PRINT("CActiveControl::delete iTranHandleServer");
	OstTrace0(TRACE_NORMAL, CACTIVECONTROL_DCACTIVECONTROL_DUP01, "CActiveControl::delete iTranHandleServer");
#endif
	iFs.Close();
	}

void CActiveControl::DoCancel()
	{
	TUSB_VERBOSE_PRINT("CActiveControl::DoCancel()");
	if(gVerbose)
	    {
	    OstTrace0(TRACE_VERBOSE, CACTIVECONTROL_DOCANCEL, "CActiveControl::DoCancel()");
	    }
	iConsole->ReadCancel();
	}

void CActiveControl::SetMSFinished(TBool aState)
	{
	if (aState)
		{
		if (iPending != EPendingEject)
			{
			iPending = EPendingEject;
			iTimer.After(iStatus,KMSFinishedDelay);
			if (!IsActive())
				{
				SetActive();
				}
			}
		}
	else
		{
		if (iPending == EPendingEject)
			{
			iPending = EPendingCancel;
			iTimer.Cancel();
			}
		}
	}

void CActiveControl::RequestEp0ControlPacket()
	{
	TUSB_VERBOSE_PRINT("CActiveControl::RequestEp0ControlPacket()");
	if(gVerbose)
	    {
	    OstTrace0(TRACE_VERBOSE, CACTIVECONTROL_REQUESTEP0CONTROLPACKET, "CActiveControl::RequestEp0ControlPacket()");
	    }
	// A request is issued to read a packet for endpoint 0
	__ASSERT_ALWAYS(!IsActive(), User::Panic(KActivePanic, 660));
	#ifdef	USB_SC
	TInt r = 0;
	do
		{
		r = iEp0Buf.GetBuffer (iEp0Packet,iEp0Size,iEp0Zlp,iStatus);
		TUSB_VERBOSE_PRINT4("Get Buffer Return code %d Status %d PacketPtr 0x%x Size %d", r, iStatus.Int(),(TInt)iEp0Packet,iEp0Size);
		if(gVerbose)
		    {
		    OstTraceExt4(TRACE_VERBOSE, CACTIVECONTROL_REQUESTEP0CONTROLPACKET_DUP01, "Get Buffer Return code %d Status %d PacketPtr 0x%x Size %d", r, iStatus.Int(),(TInt)iEp0Packet,(TInt)iEp0Size);
		    }
		test_Value(r, (r == KErrNone) || (r == KErrCompletion) || (r == TEndpointBuffer::KStateChange) || (r == KErrAlternateSettingChanged));
		if (r == KErrCompletion)
			{
			// ignore anything except a setup packet
			if ((TInt)iEp0Size == KSetupPacketSize)
				{
				iEp0SetUpPacket.Copy((TUint8 *)iEp0Packet,iEp0Size);
				r = ProcessEp0ControlPacket();
				}
			}
		else
			{
			if (r == KErrNone)
				{
				iPending = EPendingEp0Read;
				SetActive();
				}
			}
		}
	while ((r == KErrCompletion) || (r == TEndpointBuffer::KStateChange) || (r == KErrAlternateSettingChanged));
	#else
	iPort[0].ReadPacket(iStatus,EEndpoint0, iEp0SetUpPacket,KSetupPacketSize);
	iPending = EPendingEp0Read;
	SetActive();
	#endif
	}

void CActiveControl::RunL()
	{
	TInt r = KErrNone;

	TUSB_VERBOSE_PRINT("CActiveControl::RunL()");
	if(gVerbose)
	    {
	    OstTrace0(TRACE_VERBOSE, CACTIVECONTROL_RUNL, "CActiveControl::RunL()");
	    }

	switch (iPending)
		{
		case EPendingNone :
			break;

		case EPendingEp0Read :
			iPending = EPendingNone;
			if (iStatus != KErrNone)
				{
				TUSB_PRINT1("ActiveControl::Error %d in Ep0 Read Packet", iStatus.Int());
				OstTrace1(TRACE_NORMAL, CACTIVECONTROL_RUNL_DUP01, "ActiveControl::Error %d in Ep0 Read Packet", iStatus.Int());
				test(EFalse);
				}
			#ifdef USB_SC
			// for shared chunks this means that data is available in the buffer
			// but the data has yet to be transferred to a local buffer
			RequestEp0ControlPacket();
			#else
			if (ProcessEp0ControlPacket() == KErrCompletion)
				RequestEp0ControlPacket();
			#endif
			break;

		case EPendingTimer :
			iPending = EPendingNone;
			if (iStatus != KErrNone)
				{
				TUSB_PRINT1("ActiveControl::Error %d in Connection Timer Delay", iStatus.Int());
				OstTrace1(TRACE_NORMAL, CACTIVECONTROL_RUNL_DUP02, "ActiveControl::Error %d in Connection Timer Delay", iStatus.Int());
				test(EFalse);
				}
			r = iPort[0].DeviceConnectToHost();
			test_KErrNone (r);

			test.End();

			RequestEp0ControlPacket();
			break;

		case EPendingEject :
			iPending = EPendingNone;
			if (iStatus != KErrNone)
				{
				TUSB_PRINT1("ActiveControl::Error %d in Eject Timer Delay", iStatus.Int());
				OstTrace1(TRACE_NORMAL, CACTIVECONTROL_RUNL_DUP03, "ActiveControl::Error %d in Eject Timer Delay", iStatus.Int());
				test(EFalse);
				}
			StopMassStorage(&iPort[0]);
			#ifdef USB_SC
				iEp0Buf.Close();
			#endif
			ReConnect();

			RequestEp0ControlPacket();
			break;

		case EPendingCancel :
			iPending = EPendingNone;
			if (iStatus != KErrNone && iStatus != KErrCancel)
				{
				TUSB_PRINT1("ActiveControl::Error %d in Eject Timer Delay", iStatus.Int());
				OstTrace1(TRACE_NORMAL, CACTIVECONTROL_RUNL_DUP04, "ActiveControl::Error %d in Eject Timer Delay", iStatus.Int());
				test(EFalse);
				}
		}

	}

TInt CActiveControl::ProcessEp0ControlPacket()
	{
	TUint16 value = *reinterpret_cast<TUint16*>(&iEp0SetUpPacket[KUsb_Ep0wValueOffset]);
	TUint16 index = *reinterpret_cast<TUint16*>(&iEp0SetUpPacket[KUsb_Ep0wIndexOffset]);
	TUint16 length= *reinterpret_cast<TUint16*>(&iEp0SetUpPacket[KUsb_Ep0wLengthOffset]);
	TUSB_VERBOSE_PRINT3("ProcessEp0ControlPacket length 0x%x value 0x%x index 0x%x",length,value,index);
	if(gVerbose)
	    {
	    OstTraceExt3(TRACE_VERBOSE, CACTIVECONTROL_PROCESSEP0CONTROLPACKET, "ProcessEp0ControlPacket length 0x%x value 0x%x index 0x%x",length,value,index);
	    }
	TRequestStatus ep0Status;
	TUint8 host_ver_major;
	TUint8 host_ver_minor;
	TUint8 host_ver_micro;
	TUint8 usbio_ver_major;
	TUint8 usbio_ver_minor;
	#ifndef USB_SC
	TBuf8<KMaxControlPacketSize> ep0DataPacket;
	#endif
	TestParamPtr tpPtr;
	TBool firstSettingThread = (index & KFirstSettingThreadMask) ? ETrue : EFalse;
	TBool lastSettingThread = (index & KLastSettingThreadMask) ? ETrue : EFalse;
	index &= ~(KFirstSettingThreadMask | KLastSettingThreadMask);
    CActiveRW* pActiveRW;
	TInt r;
	TBool sendStatus;

	if (((iEp0SetUpPacket[KUsb_Ep0RequestTypeOffset] & KUsbRequestType_DestMask) == KUsbRequestType_DestDevice) &&
		((iEp0SetUpPacket[KUsb_Ep0RequestTypeOffset] & KUsbRequestType_TypeMask) == KUsbRequestType_TypeClass))
		{
		TUSB_VERBOSE_PRINT("Received Device Directed setup packet");
		if(gVerbose)
		    {
		    OstTrace0(TRACE_VERBOSE, CACTIVECONTROL_PROCESSEP0CONTROLPACKET_DUP01, "Received Device Directed setup packet");
		    }
		if ((iEp0SetUpPacket[KUsb_Ep0RequestTypeOffset] & KUsbRequestType_DirMask) == KUsbRequestType_DirToDev)
			{
			iEp0DataBuffer.SetLength(0);
			while (iEp0DataBuffer.Length() < length)
				{
				TUSB_VERBOSE_PRINT("Reading Ep0 data packet");
				if(gVerbose)
				    {
				    OstTrace0(TRACE_VERBOSE, CACTIVECONTROL_PROCESSEP0CONTROLPACKET_DUP02, "Reading Ep0 data packet");
				    }
				#ifdef USB_SC
				r = iEp0Buf.GetBuffer (iEp0Packet,iEp0Size,iEp0Zlp,ep0Status);
				test_Value(r, r == KErrNone || r == KErrCompletion || (r == KErrAlternateSettingChanged));
				while (r == KErrNone)
					{
					TUSB_VERBOSE_PRINT("Waiting for Ep0 data packet");
					if(gVerbose)
					    {
					    OstTrace0(TRACE_VERBOSE, CACTIVECONTROL_PROCESSEP0CONTROLPACKET_DUP03, "Waiting for Ep0 data packet");
					    }
					User::WaitForRequest(ep0Status);
					test_KErrNone(ep0Status.Int());
					r = iEp0Buf.GetBuffer (iEp0Packet,iEp0Size,iEp0Zlp,ep0Status);
					test_Value(r, r == KErrNone || r == KErrCompletion || (r == KErrAlternateSettingChanged));
					}
				TUSB_VERBOSE_PRINT1("Ep0 data packet - size %d",iEp0Size);
				if(gVerbose)
				    {
				    OstTrace1(TRACE_VERBOSE, CACTIVECONTROL_PROCESSEP0CONTROLPACKET_DUP04, "Ep0 data packet - size %d",iEp0Size);
				    }
				iEp0DataBuffer.Append((TUint8 *)iEp0Packet,iEp0Size);
				#else
				TUint16 packetLength = Min(length-iEp0DataBuffer.Length(),iEp0PacketSize);
				iPort[0].ReadPacket(ep0Status, EEndpoint0, ep0DataPacket, packetLength);
				User::WaitForRequest(ep0Status);
				if (ep0Status == KErrNone)
					{
					iEp0DataBuffer.Append(ep0DataPacket);
					}
				else
					{
					TUSB_PRINT1("ActiveControl::Error %d in Ep0 Read Data Packet", ep0Status.Int());
					OstTrace1(TRACE_NORMAL, CACTIVECONTROL_PROCESSEP0CONTROLPACKET_DUP05, "ActiveControl::Error %d in Ep0 Read Data Packet", ep0Status.Int());
					test(EFalse);
					return KErrNone;
					}
				#endif
				}
			TUSB_VERBOSE_PRINT4("Setup ToDevice Type %d length %d value %d index %d",iEp0SetUpPacket[KUsb_Ep0RequestOffset],length,value,index);
			if(gVerbose)
			    {
			    OstTraceExt4(TRACE_VERBOSE, CACTIVECONTROL_PROCESSEP0CONTROLPACKET_DUP06, "Setup ToDevice Type %d length %d value %d index %d",iEp0SetUpPacket[KUsb_Ep0RequestOffset],length,value,index);
			    }
			sendStatus = ETrue;
			switch (iEp0SetUpPacket[KUsb_Ep0RequestOffset])
				{
				case EStop :
					// send this now as the port will be disconnected
					sendStatus = EFalse;
					r = iPort[0].SendEp0StatusPacket();
					test_KErrNone(r);

					if (value && firstBulkOutEndpoint > 0)
						{
						PrintHostLog();
						}
						
					CloseStackIfOtg();
					
					for (TInt portNumber = 0; portNumber < iTotalChannels; portNumber++)
						{
						// base class cancel -> calls our DoCancel
						delete iDeviceStateNotifier[portNumber];
						delete iStallNotifier[portNumber];
						if (portNumber == 0)
							{
							r = iPort[portNumber].RemoveStringDescriptor(stridx1);
							if (r != KErrNone)
								{
								TUSB_PRINT1("Error %d on string removal", r);
								OstTrace1(TRACE_NORMAL, CACTIVECONTROL_PROCESSEP0CONTROLPACKET_DUP07, "Error %d on string removal", r);
								}
							r = iPort[portNumber].RemoveStringDescriptor(stridx2);
							if (r != KErrNone)
								{
								TUSB_PRINT1("Error %d on string removal", r);
								OstTrace1(TRACE_NORMAL, CACTIVECONTROL_PROCESSEP0CONTROLPACKET_DUP08, "Error %d on string removal", r);
								}
							}
						TUSB_VERBOSE_PRINT1 ("Closing USB channel number %d",portNumber);
						if(gVerbose)
						    {
						    OstTrace1 (TRACE_VERBOSE, CACTIVECONTROL_PROCESSEP0CONTROLPACKET_DUP09, "Closing USB channel number %d",portNumber);
						    }
#ifdef USB_SC
						if (0 == portNumber)
							{
							iEp0Buf.Close();
							}
						RChunk* commChunk;
						User::LeaveIfError(iPort[portNumber].GetDataTransferChunk(commChunk));
						commChunk->Close(); 
#endif
						iPort[portNumber].Close();											// close USB channel
						}
					TUSB_VERBOSE_PRINT("Closing Idle Counter Thread");
					if(gVerbose)
					    {
					    OstTrace0(TRACE_VERBOSE, CACTIVECONTROL_PROCESSEP0CONTROLPACKET_DUP10, "Closing Idle Counter Thread");
					    }
					iIdleCounter->iCommand = ETestIdleCounterClose;
					iIdleCounterChunk.Close();
					// Allow time for low-priority thread to close
					User::After(100000);
					iIdleCounterThread.Close();

					CActiveScheduler::Stop();
					break;

				case EVersion :
					TUSB_PRINT("Receiving t_usb_host version");
					OstTrace0(TRACE_NORMAL, CACTIVECONTROL_PROCESSEP0CONTROLPACKET_DUP11, "Receiving t_usb_host version");
					host_ver_major = iEp0DataBuffer[0];
					host_ver_minor = iEp0DataBuffer[1];
					host_ver_micro = iEp0DataBuffer[2];
					usbio_ver_major = iEp0DataBuffer[3];
					usbio_ver_minor = iEp0DataBuffer[4];
					TUSB_PRINT5("Host-side: t_usb_host v%d.%d.%d  USBIO v%d.%d\n",
						host_ver_major, host_ver_minor, host_ver_micro,
						usbio_ver_major, usbio_ver_minor);
					OstTraceExt5(TRACE_NORMAL, CACTIVECONTROL_PROCESSEP0CONTROLPACKET_DUP12, "Host-side: t_usb_host v%d.%d.%d  USBIO v%d.%d\n",
						host_ver_major, host_ver_minor, host_ver_micro,
						usbio_ver_major, usbio_ver_minor);
					if (host_ver_major < KHostVersionMajor)
						{
						TUSB_PRINT1("t_usb_host version not sufficient (need at least v%d.x.x)\n",KHostVersionMajor);
						OstTrace1(TRACE_NORMAL, CACTIVECONTROL_PROCESSEP0CONTROLPACKET_DUP13, "t_usb_host version not sufficient (need at least v%d.x.x)\n",KHostVersionMajor);
						User::Leave(-1);
						return KErrNone;
						}
					// Just using '<' instead of the seemingly absurd '<= && !==' doesn't work without
					// GCC compiler warning because Kxxx can also be zero (in which case there's no '<').
					else if ((host_ver_minor <= KHostVersionMinor) &&
			 				!(host_ver_minor == KHostVersionMinor))
						{
						TUSB_PRINT2("t_usb_host version not sufficient (need at least v%d.%d.x)\n",
							KHostVersionMajor, KHostVersionMinor);
						OstTraceExt2(TRACE_NORMAL, CACTIVECONTROL_PROCESSEP0CONTROLPACKET_DUP14, "t_usb_host version not sufficient (need at least v%d.%d.x)\n",
							KHostVersionMajor, KHostVersionMinor);
						test(EFalse);
						return KErrNone;
						}
					// Just using '<' instead of the seemingly absurd '<= && !==' doesn't work without
					// GCC compiler warning because Kxxx can also be zero (in which case there's no '<').
					else if ((host_ver_micro <= KHostVersionMicro) &&
			 				!(host_ver_micro == KHostVersionMicro))
						{
						TUSB_PRINT3("USBRFLCT version not sufficient (need at least v%d.%d.%d)\n",
							KHostVersionMajor, KHostVersionMinor, KHostVersionMicro);
						OstTraceExt3(TRACE_NORMAL, CACTIVECONTROL_PROCESSEP0CONTROLPACKET_DUP15, "USBRFLCT version not sufficient (need at least v%d.%d.%d)\n",
									KHostVersionMajor, KHostVersionMinor, KHostVersionMicro);
						test(EFalse);
						return KErrNone;
						}
					break;

				case ETestParam :
					tpPtr = (TestParamPtr)(&iEp0DataBuffer[0]);
					TUSB_VERBOSE_PRINT4("Test Params - interface %d repeat %d settingRepeat %d beforeIndex %d",tpPtr->interfaceNumber,tpPtr->repeat,tpPtr->settingRepeat,tpPtr->beforeIndex);
					if(gVerbose)
					    {
					    OstTraceExt4(TRACE_VERBOSE, CACTIVECONTROL_PROCESSEP0CONTROLPACKET_DUP16, "Test Params - interface %d repeat %d settingRepeat %d beforeIndex %d",tpPtr->interfaceNumber,tpPtr->repeat,tpPtr->settingRepeat,tpPtr->beforeIndex);
					    }
					if (index >= KMaxConcurrentTests)
						{
						TUSB_PRINT2("Test index %d is greater than maximum allowed (%d) concurrent tests",index,KMaxConcurrentTests);
						OstTraceExt2(TRACE_NORMAL, CACTIVECONTROL_PROCESSEP0CONTROLPACKET_DUP17, "Test index %d is greater than maximum allowed (%d) concurrent tests",index,KMaxConcurrentTests);
						test(EFalse);
						return KErrNone;
						}
					// Create Reader/Writer active object
					pActiveRW = CActiveRW::NewL(iConsole, &iPort[gInterfaceConfig[tpPtr->interfaceNumber][tpPtr->alternateSetting]->iPortNumber], iFs, index, lastSettingThread);
					if (!pActiveRW)
						{
						TUSB_PRINT("Failed to create reader/writer");
						OstTrace0(TRACE_NORMAL, CACTIVECONTROL_PROCESSEP0CONTROLPACKET_DUP18, "Failed to create reader/writer");
						test(EFalse);
						return KErrNone;
						}
					TUSB_VERBOSE_PRINT("Created reader/writer");
					if(gVerbose)
					    {
					    OstTrace0(TRACE_VERBOSE, CACTIVECONTROL_PROCESSEP0CONTROLPACKET_DUP19, "Created reader/writer");
					    }
					pActiveRW->SetTestParams(tpPtr);
					switch (value)
						{
					case 'X' :
						test.Start (_L("Xml"));
						break;

					case 'L' :
						test.Start (_L("Loop"));
						pActiveRW->SetTransferMode(ELoop);
						gAltSettingOnNotify = ETrue;
						if (tpPtr->settingRepeat && !firstSettingThread)
							{
							pActiveRW->Suspend(ESuspend);
							}
						else
							{
							pActiveRW->StartOrSuspend();
							}
						break;

					case 'C' :
						test.Start (_L("Compare"));
						pActiveRW->SetTransferMode(ELoopComp);
						gAltSettingOnNotify = ETrue;
						if (tpPtr->settingRepeat && !firstSettingThread)
							{
							pActiveRW->Suspend(ESuspend);
							}
						else
							{
							pActiveRW->StartOrSuspend();
							}
						break;

					case 'S' :
						test.Start (_L("Stream"));
						if (tpPtr->outPipe > KMaxEndpointsPerClient)
							{
							pActiveRW->SetTransferMode(ETransmitOnly);
							gAltSettingOnNotify = ETrue;
							if (tpPtr->settingRepeat && !firstSettingThread)
								{
								pActiveRW->Suspend(ESuspend);
								}
							else
								{
								pActiveRW->StartOrSuspend();
								}
							}
						else
							{
							pActiveRW->SetTransferMode(EReceiveOnly);
							gAltSettingOnNotify = ETrue;
							if (tpPtr->settingRepeat && !firstSettingThread)
								{
								pActiveRW->Suspend(ESuspend);
								}
							else
								{
								pActiveRW->StartOrSuspend();
								}
							}
						break;

					case 'F' :
						test.Start (_L("File"));
						// send this now as the file setup takes a long time
						sendStatus = EFalse;
						r = iPort[0].SendEp0StatusPacket();
						test_KErrNone(r);
						if (tpPtr->outPipe > KMaxEndpointsPerClient)
							{
							pActiveRW->SetTransferMode(ETransmitOnly);
							TInt maxFileSize = tpPtr->maxSize * tpPtr->repeat;
							pActiveRW->ReadFromDisk((TChar)tpPtr->minSize,maxFileSize);
							gAltSettingOnNotify = ETrue;
							if (tpPtr->settingRepeat && !firstSettingThread)
								{
								pActiveRW->Suspend(ESuspend);
								}
							else
								{
								pActiveRW->StartOrSuspend();
								}
							}
						else
							{
							pActiveRW->SetTransferMode(EReceiveOnly);
							pActiveRW->WriteToDisk((TChar)tpPtr->minSize);
							gAltSettingOnNotify = ETrue;
							if (tpPtr->settingRepeat && !firstSettingThread)
								{
								pActiveRW->Suspend(ESuspend);
								}
							else
								{
								pActiveRW->StartOrSuspend();
								}
							}
						break;

					default :
						TUSB_PRINT1("Invalid test value %X",value);
						OstTrace1(TRACE_NORMAL, CACTIVECONTROL_PROCESSEP0CONTROLPACKET_DUP20, "Invalid test value %X",value);
						test(EFalse);
						}

					gRW[index] = pActiveRW;
					break;

				case ETestResult :
					TUSB_VERBOSE_PRINT2 ("Test index %d complete - value %d",index,value);
					if(gVerbose)
					    {
					    OstTraceExt2 (TRACE_VERBOSE, CACTIVECONTROL_PROCESSEP0CONTROLPACKET_DUP21, "Test index %d complete - value %d",index,value);
					    }
					// if failure, send this first to prevent panic corrupting EP0
					if (!value)
						{
						sendStatus = EFalse;
						r = iPort[0].SendEp0StatusPacket();
						}
					if (index < KMaxConcurrentTests)
						{
						if (gRW[index] != NULL)
							{
							gRW[index]->TestComplete (value);
							break;
							}
						}
					if (index == KHostErrorIndex)
						{
						if (!value)
							{
							TUSB_PRINT("Host Test Fail");
							OstTrace0(TRACE_NORMAL, CACTIVECONTROL_PROCESSEP0CONTROLPACKET_DUP22, "Host Test Fail");
							}
						}
					else
						{
						TUSB_PRINT2("Invalid test index %d for result %d",index,value);
						OstTraceExt2(TRACE_NORMAL, CACTIVECONTROL_PROCESSEP0CONTROLPACKET_DUP23, "Invalid test index %d for result %d",index,value);
						}
					if (!value)
						{
						test(EFalse);
						}
					break;

				case ETestFail :
					User::Leave(-1);
					break;

				case ETestConnect :
					test.Start (_L("Connect"));
					sendStatus = EFalse;
					r = iPort[0].SendEp0StatusPacket();
					if (iSoftwareConnect)
						{
						r = iPort[0].DeviceDisconnectFromHost();
						test_KErrNone (r);

						TUint32 waitTime = (TUint32)value * 1000;
						if (waitTime == 0)
							{
							waitTime = 5000;		// default to 5 milliseconds
							}
						iTimer.After(iStatus,waitTime);
						iPending = EPendingTimer;

						SetActive();
						}
					else
						{
						iConsole->Printf(_L("This device does not support software\n"));
						OstTrace0(TRACE_NORMAL, CACTIVECONTROL_PROCESSEP0CONTROLPACKET_DUP24, "This device does not support software\n");
						iConsole->Printf(_L("disconnect/reconnect\n"));
						OstTrace0(TRACE_NORMAL, CACTIVECONTROL_PROCESSEP0CONTROLPACKET_DUP25, "disconnect/reconnect\n");
						iConsole->Printf(_L("Please physically unplug and replug\n"));
						OstTrace0(TRACE_NORMAL, CACTIVECONTROL_PROCESSEP0CONTROLPACKET_DUP26, "Please physically unplug and replug\n");
						iConsole->Printf(_L("the USB cable NOW... "));
						OstTrace0(TRACE_NORMAL, CACTIVECONTROL_PROCESSEP0CONTROLPACKET_DUP27, "the USB cable NOW... ");
						test.End ();
						}
					break;

				case ETestDisconnect :
					test.Start (_L("Disconnect"));
					// send this now as the port will be disconnected
					sendStatus = EFalse;
					r = iPort[0].SendEp0StatusPacket();
					if (iSoftwareConnect)
						{
						r = iPort[0].DeviceDisconnectFromHost();
						test_KErrNone (r);
						}
					else
						{
						iConsole->Printf(_L("This device does not support software\n"));
						OstTrace0(TRACE_NORMAL, CACTIVECONTROL_PROCESSEP0CONTROLPACKET_DUP28, "This device does not support software\n");
						iConsole->Printf(_L("disconnect/reconnect\n"));
						OstTrace0(TRACE_NORMAL, CACTIVECONTROL_PROCESSEP0CONTROLPACKET_DUP29, "disconnect/reconnect\n");
						iConsole->Printf(_L("Please physically unplug and replug\n"));
						OstTrace0(TRACE_NORMAL, CACTIVECONTROL_PROCESSEP0CONTROLPACKET_DUP30, "Please physically unplug and replug\n");
						iConsole->Printf(_L("the USB cable NOW... "));
						OstTrace0(TRACE_NORMAL, CACTIVECONTROL_PROCESSEP0CONTROLPACKET_DUP31, "the USB cable NOW... ");
						}

					test.End ();
					break;

				case ETestMassStorage :
					test.Start (_L("Select Mass Storage"));

					// send this now as the port will be disconnected
					sendStatus = EFalse;
					r = iPort[0].SendEp0StatusPacket();
					test_KErrNone(r);

					CloseStackIfOtg();
					
					for (TInt portNumber = 0; portNumber < iTotalChannels; portNumber++)
						{
						delete iDeviceStateNotifier[portNumber];
						delete iStallNotifier[portNumber];
						if (portNumber == 0)
							{
							r = iPort[portNumber].RemoveStringDescriptor(stridx1);
							if (r != KErrNone)
								{
								TUSB_PRINT1("Error %d on string removal", r);
								OstTrace1(TRACE_NORMAL, CACTIVECONTROL_PROCESSEP0CONTROLPACKET_DUP32, "Error %d on string removal", r);
								}
							r = iPort[portNumber].RemoveStringDescriptor(stridx2);
							if (r != KErrNone)
								{
								TUSB_PRINT1("Error %d on string removal", r);
								OstTrace1(TRACE_NORMAL, CACTIVECONTROL_PROCESSEP0CONTROLPACKET_DUP33, "Error %d on string removal", r);
								}
							}
						TUSB_VERBOSE_PRINT1 ("Closing USB channel number %d",portNumber);
						if(gVerbose)
						    {
						    OstTrace1 (TRACE_VERBOSE, CACTIVECONTROL_PROCESSEP0CONTROLPACKET_DUP34, "Closing USB channel number %d",portNumber);
						    }
#ifdef USB_SC
						if (0 == portNumber)
							{
							iEp0Buf.Close();
							}
						RChunk* commChunk;
						User::LeaveIfError(iPort[portNumber].GetDataTransferChunk(commChunk));
						commChunk->Close();	
#endif							
						iPort[portNumber].Close();											// close USB channel
						}

					r = iPort[0].Open(0);
					test_KErrNone(r);
					TUSB_VERBOSE_PRINT("Successfully opened USB port");
					if(gVerbose)
					    {
					    OstTrace0(TRACE_VERBOSE, CACTIVECONTROL_PROCESSEP0CONTROLPACKET_DUP35, "Successfully opened USB port");
					    }

					SetupDescriptors(iLddPtr, &iPort[0],value);
					StartMassStorage(&iPort[0]);

#ifdef USB_SC
					r = iPort[0].OpenEndpoint(iEp0Buf,0);
					test_KErrNone(r);
#endif

					OpenStackIfOtg();
					test.Next (_L("Enumeration..."));
					r = ReEnumerate();
					test_KErrNone(r);


					test.End ();
					break;
				}
			if (sendStatus)
				{
				r = iPort[0].SendEp0StatusPacket();
				if (r != KErrNone)
					{
					TUSB_PRINT1("ActiveControl::Error %d in Ep0 Send Status Packet", r);
					OstTrace1(TRACE_NORMAL, CACTIVECONTROL_PROCESSEP0CONTROLPACKET_DUP36, "ActiveControl::Error %d in Ep0 Send Status Packet", r);
					test(EFalse);
					return KErrNone;
					}
				}
			}
		else
			{
			if ((iEp0SetUpPacket[KUsb_Ep0RequestOffset] == EVersion) && length > 0)
				{
				TUSB_PRINT4("Sending t_usb_device version: %d.%d.%d length %d \n", KDeviceVersionMajor, KDeviceVersionMinor, KDeviceVersionMicro, length);
				OstTraceExt4(TRACE_NORMAL, CACTIVECONTROL_PROCESSEP0CONTROLPACKET_DUP37, "Sending t_usb_device version: %u.%u.%u length %u \n", KDeviceVersionMajor, KDeviceVersionMinor, KDeviceVersionMicro, (TUint32)length);
				#ifdef	USB_SC
				TUint8 *ep0Buffer;
				TUint8 *ep0BufPtr;
				TUint ep0Length;
				iEp0Buf.GetInBufferRange(((TAny*&)ep0Buffer),ep0Length);

				ep0BufPtr = ep0Buffer;
				*(ep0Buffer++) = KDeviceVersionMajor;
				*(ep0Buffer++) = KDeviceVersionMinor;
				*(ep0Buffer++) = KDeviceVersionMicro;
				TUint8 i=3;
				if (iConfigFileName->Length())
					{
					for(TUint8 j=0; j < iConfigFileName->Length() && i < length; j++)
						{
						i++;
						*(ep0Buffer++) = (*iConfigFileName)[j];
						}
					}
				if (iScriptFileName->Length())
					{
					for(TUint8 j=0; j < iScriptFileName->Length() && i < length; j++)
						{
						i++;
						*(ep0Buffer++) = (*iScriptFileName)[j];
						}
					}
				*(ep0Buffer++) = 0;
				r = iEp0Buf.WriteBuffer(ep0BufPtr,length,FALSE,ep0Status);
				test_KErrNone(r);
				#else
				iEp0DataBuffer.FillZ(length);
				iEp0DataBuffer[0] = KDeviceVersionMajor;
				iEp0DataBuffer[1] = KDeviceVersionMinor;
				iEp0DataBuffer[2] = KDeviceVersionMicro;
				iEp0DataBuffer.SetLength(3);
				iEp0DataBuffer.Append (*iConfigFileName);
				iEp0DataBuffer.Append (*iScriptFileName);
				iEp0DataBuffer.SetLength(length);
				iPort[0].Write(ep0Status, EEndpoint0, iEp0DataBuffer, length);
				#endif
				User::WaitForRequest(ep0Status);
				test_KErrNone(ep0Status.Int());
				}
			else if ((iEp0SetUpPacket[KUsb_Ep0RequestOffset] == ETestIdleCounter) && length >= sizeof(TInt64))
				{
				// for a non zero request value if any tests still active send zero otherwise the counter value
				TInt64 val = (value == 0 || gActiveTestCount == 0) ? iIdleCounter->iCounter : 0;

				TUSB_PRINT1("Sending counter value %Ld\n", val);
				OstTraceExt1(TRACE_NORMAL, CACTIVECONTROL_PROCESSEP0CONTROLPACKET_DUP38, "Sending counter value %Ld\n", val);
				#ifdef	USB_SC

				TUint8 *ep0Buffer;
				TUint ep0Length;
				iEp0Buf.GetInBufferRange(((TAny*&)ep0Buffer),ep0Length);

				*((TInt64*) ep0Buffer) = val;

				r = iEp0Buf.WriteBuffer(ep0Buffer,length,FALSE,ep0Status);
				test_KErrNone(r);
				#else

				iEp0DataBuffer.FillZ(length);
				*((TInt64*) iEp0DataBuffer.Ptr()) = val;
				iEp0DataBuffer.SetLength(sizeof(TInt64));
				iPort[0].Write(ep0Status, EEndpoint0, iEp0DataBuffer, length);
				#endif

				User::WaitForRequest(ep0Status);
				test_KErrNone(ep0Status.Int());
				}
			}
		if (iEp0SetUpPacket[KUsb_Ep0RequestOffset] != EStop && iEp0SetUpPacket[KUsb_Ep0RequestOffset] != ETestConnect &&
			iEp0SetUpPacket[KUsb_Ep0RequestOffset] != ETestMassStorage)
			{
			return KErrCompletion;
			}
		}
	else
		{
		TUSB_PRINT1("Error : Incorrect SetUp Packet Request Type %X", iEp0SetUpPacket[0]);
		OstTrace1(TRACE_NORMAL, CACTIVECONTROL_PROCESSEP0CONTROLPACKET_DUP39, "Error : Incorrect SetUp Packet Request Type %X", iEp0SetUpPacket[0]);
		test(EFalse);
		return KErrNone;
		}

	return KErrNone;
	}

void CActiveControl::PrintHostLog()
	{
#ifdef OST_TRACE_COMPILER_IN_USE
	TRequestStatus status = 0;
	wchar_t lineBuf[128];
	TUint j = 0;

	TUSB_VERBOSE_PRINT("Reading Host log file\n");
	if(gVerbose)
	    {
	    OstTrace0(TRACE_VERBOSE, CACTIVECONTROL_PRINTHOSTLOG, "Reading Host log file\n");
	    }

	#ifdef USB_SC
	TInt r = 0;
	TEndpointBuffer scReadBuf;
	TAny * scReadData;
	TUint8 * scCharPtr;
	TUint readSize;
	TBool readZlp = EFalse;

    r = iPort->OpenEndpoint(scReadBuf,firstBulkOutEndpoint);
    test_KErrNone(r);
    do
        {
        r = scReadBuf.GetBuffer (scReadData,readSize,readZlp,status);
        // The following line can be reinstated once the shared chunk failure is fixed
        // that prevents the readZlp flag from being set
        // test_Value(r, (r == KErrNone) || (r == KErrCompletion) || (r == KErrEof));
        if (r == KErrCompletion)
            {
            TUSB_VERBOSE_PRINT1("Host log file %d bytes read\n",readSize);
            if(gVerbose)
                {
                OstTrace1(TRACE_VERBOSE, CACTIVECONTROL_PRINTHOSTLOG_DUP01, "Host log file %d bytes read\n",readSize);
                }
            scCharPtr = (TUint8 *)scReadData;
            // Print the host log file
            for (TUint i = 0; i < readSize; i++)
                {
                if (* scCharPtr == '\r')
                    {
                    lineBuf[j++] = '\0';
                    OstTraceExt1(TRACE_NORMAL, CACTIVECONTROL_PRINTHOSTLOG_DUP02, "%S",*lineBuf);
                    j = 0;
                    }
                else
                    {
                    if (* scCharPtr != '\n')
                        {
                        lineBuf[j++] = * scCharPtr;
                        }
                    }
                scCharPtr++;
                }
            }
        if (r == KErrNone)
            {
            User::WaitForRequest(status);
            test_KErrNone(status.Int());
            }
        }
    while (r >= KErrNone && !readZlp);
    #else
    TPtr8 readBuf((TUint8 *)User::Alloc(KHostLogFileSize),KHostLogFileSize,KHostLogFileSize);
    iPort[0].ReadUntilShort(status, (TEndpointNumber)firstBulkOutEndpoint, readBuf);
    User::WaitForRequest(status);
    test_KErrNone(status.Int());
    TUSB_VERBOSE_PRINT1("Host log file %d bytes read\n",readBuf.Length());
    if(gVerbose)
        {
        OstTrace1(TRACE_VERBOSE, CACTIVECONTROL_PRINTHOSTLOG_DUP03, "Host log file %d bytes read\n",readBuf.Length());
        }
    for (TUint i = 0; i < readBuf.Length(); i++)
        {
        if (readBuf[i] == '\r')
            {
            lineBuf[j++] = '\0';
            OstTraceExt1(TRACE_NORMAL, CACTIVECONTROL_PRINTHOSTLOG_DUP04, "%s",*lineBuf);
            j = 0;
            }
        else
            {
            if (readBuf[i] != '\n')
                {
                lineBuf[j++] = readBuf[i];
                }
            }
        }
    User::Free ((TAny *)readBuf.Ptr());
    #endif
    
#endif // OST_TRACE_COMPILER_IN_USE
	}

void CActiveControl::QueryUsbClientL(LDDConfigPtr aLddPtr, RDEVCLIENT* aPort)
	{
	// Get device/endpoint capabilities
	//
	// A TPckg, or TPckBuf was not used in the following, because
	//
	//	 TPckgBuf<TUsbcEndpointData[KUsbcMaxEndpoints]> databuf;
	//
	// doesn't work. Also,
	//
	//	 TUsbcEndpointData data[KUsbcMaxEndpoints];
	//	 TPckgBuf<TUsbcEndpointData[KUsbcMaxEndpoints]> databuf(data);
	//
	// doesn't work. Also,
	//
	//	 TUsbcEndpointData data[KUsbcMaxEndpoints];
	//	 TPckgBuf<TUsbcEndpointData[]> databuf(data);
	//
	// doesn't work.
	// So we seem to have to stick to the ugly cast below.
	//
	//	 TUsbcEndpointData data[KUsbcMaxEndpoints];
	//	 TPtr8 databuf(reinterpret_cast<TUint8*>(data), sizeof(data), sizeof(data));
	//

	// Device
	// === Device Descriptor

	test.Start(_L("Query device and Endpoint Capabilities"));


	TUsbDeviceCaps d_caps;
	TInt r = aPort->DeviceCaps(d_caps);
	test_KErrNone(r);

	const TInt n = d_caps().iTotalEndpoints;

	TUSB_PRINT("###  USB device capabilities:");
	OstTrace0(TRACE_NORMAL, CACTIVECONTROL_QUERYUSBCLIENTL, "###  USB device capabilities:");
	TUSB_PRINT1("Number of endpoints:                %d", n);
	OstTrace1(TRACE_NORMAL, CACTIVECONTROL_QUERYUSBCLIENTL_DUP01, "Number of endpoints:                %d", n);
	TUSB_PRINT1("Supports Software-Connect:          %s",
		d_caps().iConnect ? _S("yes") : _S("no"));
	OstTraceExt1(TRACE_NORMAL, CACTIVECONTROL_QUERYUSBCLIENTL_DUP02, "Supports Software-Connect:          %s",
				d_caps().iConnect ? _L("yes") : _S("no"));
	TUSB_PRINT1("Device is Self-Powered:             %s",
		d_caps().iSelfPowered ? _S("yes") : _S("no"));
	OstTraceExt1(TRACE_NORMAL, CACTIVECONTROL_QUERYUSBCLIENTL_DUP03, "Device is Self-Powered:             %s",
				d_caps().iSelfPowered ? _L("yes") : _L("no"));
	TUSB_PRINT1("Supports Remote-Wakeup:             %s",
		d_caps().iRemoteWakeup ? _S("yes") : _S("no"));
	OstTraceExt1(TRACE_NORMAL, CACTIVECONTROL_QUERYUSBCLIENTL_DUP04, "Supports Remote-Wakeup:             %s",
				d_caps().iRemoteWakeup ? _L("yes") : _L("no"));
	TUSB_PRINT1("Supports High-speed:                %s",
		d_caps().iHighSpeed ? _S("yes") : _S("no"));
	OstTraceExt1(TRACE_NORMAL, CACTIVECONTROL_QUERYUSBCLIENTL_DUP05, "Supports High-speed:                %s",
				d_caps().iHighSpeed ? _L("yes") : _L("no"));
	TUSB_PRINT1("Supports unpowered cable detection: %s\n",
		(d_caps().iFeatureWord1 & KUsbDevCapsFeatureWord1_CableDetectWithoutPower) ?
		_S("yes") : _S("no"));
	OstTraceExt1(TRACE_NORMAL, CACTIVECONTROL_QUERYUSBCLIENTL_DUP06, "Supports unpowered cable detection: %s\n",
				(d_caps().iFeatureWord1 & KUsbDevCapsFeatureWord1_CableDetectWithoutPower) ?
				_L("yes") : _L("no"));
	TUSB_PRINT1("Supports endpoint resource allocation v2 scheme: %s\n",
		(d_caps().iFeatureWord1 & KUsbDevCapsFeatureWord1_EndpointResourceAllocV2) ?
		_S("yes") : _S("no"));
	OstTraceExt1(TRACE_NORMAL, CACTIVECONTROL_QUERYUSBCLIENTL_DUP07, "Supports endpoint resource allocation v2 scheme: %s\n",
				(d_caps().iFeatureWord1 & KUsbDevCapsFeatureWord1_EndpointResourceAllocV2) ?
				_L("yes") : _L("no"));
	TUSB_PRINT("");
	OstTrace0(TRACE_NORMAL, CACTIVECONTROL_QUERYUSBCLIENTL_DUP08, "");

	iSoftwareConnect = d_caps().iConnect;					// we need to remember this
	test_Equal(aLddPtr->iSoftConnect,iSoftwareConnect);

	iSupportResourceAllocationV2 = ((d_caps().iFeatureWord1 & KUsbDevCapsFeatureWord1_EndpointResourceAllocV2) != 0);

	// only check capabilities if set; therefore allowing them to be disabled
	if (aLddPtr->iSelfPower)
		{
		test(d_caps().iSelfPowered);
		}

	// only check capabilities if set; therefore allowing them to be disabled
	if (aLddPtr->iRemoteWakeup)
		{
		test(d_caps().iRemoteWakeup);
		}

	test_Equal(d_caps().iFeatureWord1 & KUsbDevCapsFeatureWord1_CableDetectWithoutPower,aLddPtr->iFeatures);

	// only check capability if set; therefore allowing it to be disabled
	if (aLddPtr->iHighSpeed)
		{
		test(d_caps().iHighSpeed);
		}

	test_Equal(aLddPtr->iNumEndpoints,n);

	// Endpoints
	TUsbcEndpointData data[KUsbcMaxEndpoints];
	TPtr8 dataptr(reinterpret_cast<TUint8*>(data), sizeof(data), sizeof(data));
	r = aPort->EndpointCaps(dataptr);
	test_KErrNone(r);

	TUSB_PRINT("### USB device endpoint capabilities:");
	OstTrace0(TRACE_NORMAL, CACTIVECONTROL_QUERYUSBCLIENTL_DUP09, "### USB device endpoint capabilities:");
	for (TInt i = 0; i < n; i++)
		{
		const TUsbcEndpointCaps* caps = &data[i].iCaps;


		TBuf<40> sizeStr(_S("unknown"));
		if (caps->iSizes == KUsbEpNotAvailable)
			{
			sizeStr = _S("Not Available");
			}
		else
			{
			sizeStr.SetLength(0);
			if (caps->iSizes & KUsbEpSizeCont)
				sizeStr.Append(_S(" Continuous"),11);
			if (caps->iSizes & KUsbEpSize8)
				sizeStr.Append(_S(" 8"),2);
			if (caps->iSizes & KUsbEpSize16)
				sizeStr.Append(_S(" 16"),3);
			if (caps->iSizes & KUsbEpSize32)
				sizeStr.Append(_S(" 32"),3);
			if (caps->iSizes & KUsbEpSize64)
				sizeStr.Append(_S(" 64"),3);
			if (caps->iSizes & KUsbEpSize128)
				sizeStr.Append(_S(" 128"),4);
			if (caps->iSizes & KUsbEpSize256)
				sizeStr.Append(_S(" 256"),4);
			if (caps->iSizes & KUsbEpSize512)
				sizeStr.Append(_S(" 512"),4);
			if (caps->iSizes & KUsbEpSize1023)
				sizeStr.Append(_S(" 1023"),5);
			if (caps->iSizes & KUsbEpSize1024)
				sizeStr.Append(_S(" 1024"),5);
			}

		TBuf<40> typeStr(_S("unknown"));
		if (caps->iTypesAndDir == KUsbEpNotAvailable)
			typeStr = _S("Not Available");
		if (caps->iTypesAndDir & (KUsbEpTypeControl | KUsbEpTypeBulk | KUsbEpTypeInterrupt | KUsbEpTypeIsochronous))
			{
			typeStr.SetLength(0);
			if (caps->iTypesAndDir & KUsbEpTypeBulk)
				typeStr.Append(_S("Control "),8);
			if (caps->iTypesAndDir & KUsbEpTypeBulk)
				typeStr.Append(_S("Bulk "),5);
			if (caps->iTypesAndDir & KUsbEpTypeInterrupt)
				typeStr.Append(_S("Interrupt "),10);
			if (caps->iTypesAndDir & KUsbEpTypeIsochronous)
				typeStr.Append(_S("Isochronous"),11);
			}

		TBuf<20> directionStr(_S("unknown"));

		if (caps->iTypesAndDir & KUsbEpDirIn)
			directionStr = _S("In");
		if (caps->iTypesAndDir & KUsbEpDirOut)
			directionStr = _S("Out");
		if (caps->iTypesAndDir & KUsbEpDirBidirect)
			directionStr = _S("Both");

		TUSB_PRINT4("Endpoint:%d Sizes =%s Type = %s - %s",
					i+1,sizeStr.PtrZ(), typeStr.PtrZ(), directionStr.PtrZ());
		OstTraceExt4(TRACE_NORMAL, CACTIVECONTROL_QUERYUSBCLIENTL_DUP10, "Endpoint:%d Sizes =%S Type = %S - %S",
					i+1,sizeStr, typeStr, directionStr);
		}
	TUSB_PRINT("");
	OstTrace0(TRACE_NORMAL, CACTIVECONTROL_QUERYUSBCLIENTL_DUP11, "");

	test.End();

	}


void CActiveControl::AllocateEndpointDMA(RDEVCLIENT* aPort,TENDPOINTNUMBER aEndpoint)
	{
	TBool res = EFalse;

	TInt r = aPort->AllocateEndpointResource(aEndpoint, EUsbcEndpointResourceDMA);
	if (r == KErrNone)
		OstTrace1(TRACE_NORMAL, CACTIVECONTROL_ALLOCATEENDPOINTDMA, "DMA allocation on endpoint %d: KErrNone", aEndpoint);
	else if (r == KErrInUse)
		OstTrace1(TRACE_NORMAL, CACTIVECONTROL_ALLOCATEENDPOINTDMA_DUP01, "DMA allocation on endpoint %d: KErrInUse", aEndpoint);
	else if (r == KErrNotSupported)
		OstTrace1(TRACE_NORMAL, CACTIVECONTROL_ALLOCATEENDPOINTDMA_DUP02, "DMA allocation on endpoint %d: KErrNotSupported", aEndpoint);
	else
		OstTraceExt2(TRACE_NORMAL, CACTIVECONTROL_ALLOCATEENDPOINTDMA_DUP03, "DMA allocation on endpoint %d: unexpected return value %d", aEndpoint, r);
	#ifdef	USB_SC
	res = aPort->QueryEndpointResourceUse(aEndpoint, EUsbcEndpointResourceDMA);
	#else
	res = aPort->QueryEndpointResourceUse(aEndpoint, EUsbcEndpointResourceDMA);
	#endif

	TUSB_PRINT2("DMA on endpoint %d %s\n",
				aEndpoint, res ? _S("allocated") : _S("not allocated"));
	OstTraceExt2(TRACE_NORMAL, CACTIVECONTROL_ALLOCATEENDPOINTDMA_DUP04, "DMA on endpoint %d %S\n",
				aEndpoint, res ? _L("allocated") : _L("not allocated"));

	if ((r == KErrNone) && !res)
		OstTrace0(TRACE_NORMAL, CACTIVECONTROL_ALLOCATEENDPOINTDMA_DUP05, "(Allocation success but negative query result: contradiction!\n");
	else if ((r != KErrNone) && res)
		OstTrace0(TRACE_NORMAL, CACTIVECONTROL_ALLOCATEENDPOINTDMA_DUP06, "(Allocation failure but positive query result: contradiction!\n");
	}


void CActiveControl::DeAllocateEndpointDMA(RDEVCLIENT* aPort,TENDPOINTNUMBER aEndpoint)
	{
	TBool res = FALSE;
	TInt r = aPort->DeAllocateEndpointResource(aEndpoint, EUsbcEndpointResourceDMA);
	if (r == KErrNone)
		OstTrace1(TRACE_NORMAL, CACTIVECONTROL_DEALLOCATEENDPOINTDMA, "DMA deallocation on endpoint %d: KErrNone", aEndpoint);
	else if (r == KErrNotSupported)
		OstTrace1(TRACE_NORMAL, CACTIVECONTROL_DEALLOCATEENDPOINTDMA_DUP01, "DMA deallocation on endpoint %d: KErrNotSupported", aEndpoint);
	else
		OstTraceExt2(TRACE_NORMAL, CACTIVECONTROL_DEALLOCATEENDPOINTDMA_DUP02, "DMA deallocation on endpoint %d: unexpected return value %d",
					  aEndpoint, r);
	#ifdef	USB_SC
	res = aPort->QueryEndpointResourceUse(aEndpoint, EUsbcEndpointResourceDMA);
	#else
	res = aPort->QueryEndpointResourceUse(aEndpoint, EUsbcEndpointResourceDMA);
	#endif

	TUSB_PRINT2("DMA on endpoint %d %s\n",
				aEndpoint, res ? _S("allocated") : _S("not allocated"));
	OstTraceExt2(TRACE_NORMAL, CACTIVECONTROL_DEALLOCATEENDPOINTDMA_DUP03, "DMA on endpoint %d %s\n",
				aEndpoint, res ? _L("allocated") : _L("not allocated"));
	}

#ifndef USB_SC
void CActiveControl::AllocateDoubleBuffering(RDEVCLIENT* aPort,TENDPOINTNUMBER aEndpoint)
	{
	TBool res = FALSE;
	TInt r = aPort->AllocateEndpointResource(aEndpoint, EUsbcEndpointResourceDoubleBuffering);
	if (r == KErrNone)
		OstTrace1(TRACE_NORMAL, CACTIVECONTROL_ALLOCATEDOUBLEBUFFERING, "Double Buffering allocation on endpoint %d: KErrNone", aEndpoint);
	else if (r == KErrInUse)
		OstTrace1(TRACE_NORMAL, CACTIVECONTROL_ALLOCATEDOUBLEBUFFERING_DUP01, "Double Buffering allocation on endpoint %d: KErrInUse", aEndpoint);
	else if (r == KErrNotSupported)
		OstTrace1(TRACE_NORMAL, CACTIVECONTROL_ALLOCATEDOUBLEBUFFERING_DUP02, "Double Buffering allocation on endpoint %d: KErrNotSupported", aEndpoint);
	else
		OstTraceExt2(TRACE_NORMAL, CACTIVECONTROL_ALLOCATEDOUBLEBUFFERING_DUP03, "Double Buffering allocation on endpoint %d: unexpected return value %d",
					  aEndpoint, r);
	res = aPort->QueryEndpointResourceUse(aEndpoint, EUsbcEndpointResourceDoubleBuffering);
	TUSB_PRINT2("Double Buffering on endpoint %d %s\n",
				aEndpoint, res ? _S("allocated") : _S("not allocated"));
	OstTraceExt2(TRACE_NORMAL, CACTIVECONTROL_ALLOCATEDOUBLEBUFFERING_DUP04, "Double Buffering on endpoint %d %s\n",
				aEndpoint, res ? _L("allocated") : _L("not allocated"));

	if ((r == KErrNone) && !res)
		OstTrace0(TRACE_NORMAL, CACTIVECONTROL_ALLOCATEDOUBLEBUFFERING_DUP05, "(Allocation success but negative query result: contradiction!\n");
	else if ((r != KErrNone) && res)
		OstTrace0(TRACE_NORMAL, CACTIVECONTROL_ALLOCATEDOUBLEBUFFERING_DUP06, "(Allocation failure but positive query result: contradiction!\n");
	}


void CActiveControl::DeAllocateDoubleBuffering(RDEVCLIENT* aPort,TENDPOINTNUMBER aEndpoint)
	{
	TInt r = aPort->DeAllocateEndpointResource(aEndpoint, EUsbcEndpointResourceDoubleBuffering);
	if (r == KErrNone)
		OstTrace1(TRACE_NORMAL, CACTIVECONTROL_DEALLOCATEDOUBLEBUFFERING, "Double Buffering deallocation on endpoint %d: KErrNone", aEndpoint);
	else if (r == KErrNotSupported)
		OstTrace1(TRACE_NORMAL, CACTIVECONTROL_DEALLOCATEDOUBLEBUFFERING_DUP01, "Double Buffering deallocation on endpoint %d: KErrNotSupported", aEndpoint);
	else
		OstTraceExt2(TRACE_NORMAL, CACTIVECONTROL_DEALLOCATEDOUBLEBUFFERING_DUP02, "Double Buffering deallocation on endpoint %d: unexpected return value %d",
					  aEndpoint, r);
	TBool res = aPort->QueryEndpointResourceUse(aEndpoint, EUsbcEndpointResourceDoubleBuffering);
	TUSB_PRINT2("Double Buffering on endpoint %d %s\n",
				aEndpoint, res ? _S("allocated") : _S("not allocated"));
	OstTraceExt2(TRACE_NORMAL, CACTIVECONTROL_DEALLOCATEDOUBLEBUFFERING_DUP03, "Double Buffering on endpoint %d %s\n",
				aEndpoint, res ? _L("allocated") : _L("not allocated"));
	}

#endif

TInt CActiveControl::ReEnumerate()
	{
	TRequestStatus enum_status;
	iPort[0].ReEnumerate(enum_status);
	if (!iSoftwareConnect)
		{
		iConsole->Printf(_L("This device does not support software\n"));
		OstTrace0(TRACE_NORMAL, CACTIVECONTROL_REENUMERATE, "This device does not support software\n");
		iConsole->Printf(_L("disconnect/reconnect\n"));
		OstTrace0(TRACE_NORMAL, CACTIVECONTROL_REENUMERATE_DUP01, "disconnect/reconnect\n");
		iConsole->Printf(_L("Please physically unplug and replug\n"));
		OstTrace0(TRACE_NORMAL, CACTIVECONTROL_REENUMERATE_DUP02, "Please physically unplug and replug\n");
		iConsole->Printf(_L("the USB cable NOW... "));
		OstTrace0(TRACE_NORMAL, CACTIVECONTROL_REENUMERATE_DUP03, "the USB cable NOW... ");
		}
	iConsole->Printf(_L("\n>>> Start the t_usb_win program on the host <<<\n"));
	OstTrace0(TRACE_NORMAL, CACTIVECONTROL_REENUMERATE_DUP04, "\n>>> Start the t_usb_win program on the host <<<\n");
	User::WaitForRequest(enum_status);
	if (enum_status != KErrNone)
		{
		TUSB_PRINT1("Error: Re-enumeration status = %d", enum_status.Int());
		OstTrace1(TRACE_NORMAL, CACTIVECONTROL_REENUMERATE_DUP05, "Error: Re-enumeration status = %d", enum_status.Int());
		return KErrGeneral;
		}
	TUsbcDeviceState device_state =	EUsbcDeviceStateUndefined;
	TInt r = iPort[0].DeviceStatus(device_state);
	if (r != KErrNone)
		{
		TUSB_PRINT1("Error %d on querying device state", r);
		OstTrace1(TRACE_NORMAL, CACTIVECONTROL_REENUMERATE_DUP06, "Error %d on querying device state", r);
		}
	else
		{
		TUSB_PRINT1("Current device state: %s",
					(device_state == EUsbcDeviceStateUndefined) ? _S("Undefined") :
					((device_state == EUsbcDeviceStateAttached) ? _S("Attached") :
					 ((device_state == EUsbcDeviceStatePowered) ? _S("Powered") :
					  ((device_state == EUsbcDeviceStateDefault) ? _S("Default") :
					   ((device_state == EUsbcDeviceStateAddress) ? _S("Address") :
						((device_state == EUsbcDeviceStateConfigured) ? _S("Configured") :
						 ((device_state == EUsbcDeviceStateSuspended) ? _S("Suspended") :
						  _S("Unknown"))))))));
		OstTraceExt1(TRACE_NORMAL, CACTIVECONTROL_REENUMERATE_DUP07, "Current device state: %s",
					(device_state == EUsbcDeviceStateUndefined) ? _L("Undefined") :
					((device_state == EUsbcDeviceStateAttached) ? _L("Attached") :
					 ((device_state == EUsbcDeviceStatePowered) ? _L("Powered") :
					  ((device_state == EUsbcDeviceStateDefault) ? _L("Default") :
					   ((device_state == EUsbcDeviceStateAddress) ? _L("Address") :
						((device_state == EUsbcDeviceStateConfigured) ? _L("Configured") :
						 ((device_state == EUsbcDeviceStateSuspended) ? _L("Suspended") :
						  _L("Unknown"))))))));
		}

	// Check the speed of the established physical USB connection
	iHighSpeed = iPort[0].CurrentlyUsingHighSpeed();
	if (iHighSpeed)
		{
		TUSB_PRINT("---> USB High-speed Testing\n");
		OstTrace0(TRACE_NORMAL, CACTIVECONTROL_REENUMERATE_DUP08, "---> USB High-speed Testing\n");
		}
	else
		{
		TUSB_PRINT("---> USB Full-speed Testing\n");
		OstTrace0(TRACE_NORMAL, CACTIVECONTROL_REENUMERATE_DUP09, "---> USB Full-speed Testing\n");
		}

	return KErrNone;
	}


#ifdef USB_SC

void CActiveControl::SetupTransferedInterface(IFConfigPtr* aIfPtr, TInt aPortNumber)
	{
	TInt r;
	TUSB_VERBOSE_PRINT1("SetupTransferedInterface %d", aPortNumber);
	if(gVerbose)
	    {
	    OstTrace1(TRACE_VERBOSE, CACTIVECONTROL_SETUPTRANSFEREDINTERFACE, "SetupTransferedInterface %d", aPortNumber);
	    }
	test.Start (_L("Setup Transfered Interface "));

	#ifdef USB_SC
	TUsbcScInterfaceInfoBuf ifc = *((*aIfPtr)->iInfoPtr);
	#else
	TUsbcInterfaceInfoBuf ifc = *((*aIfPtr)->iInfoPtr);
	#endif

	TBuf8<KUsbDescSize_Interface> ifDescriptor;
	r = iPort[aPortNumber].GetInterfaceDescriptor(0, ifDescriptor);
	test_KErrNone(r);

	// Check the interface descriptor
	test(ifDescriptor[KIfcDesc_SettingOffset] == 0 && ifDescriptor[KIfcDesc_NumEndpointsOffset] == (*aIfPtr)->iInfoPtr->iTotalEndpointsUsed &&
	    ifDescriptor[KIfcDesc_ClassOffset] == (*aIfPtr)->iInfoPtr->iClass.iClassNum &&
	    ifDescriptor[KIfcDesc_SubClassOffset] == (*aIfPtr)->iInfoPtr->iClass.iSubClassNum &&
	    ifDescriptor[KIfcDesc_ProtocolOffset] == (*aIfPtr)->iInfoPtr->iClass.iProtocolNum);

	if ((*aIfPtr)->iNumber != 0 && ifDescriptor[KIfcDesc_NumberOffset] != (*aIfPtr)->iNumber)
		{
		ifDescriptor[KIfcDesc_NumberOffset] = (*aIfPtr)->iNumber;
		r = iPort[aPortNumber].SetInterfaceDescriptor(0, ifDescriptor);
		test_KErrNone(r);
		}
	else
		{
		(*aIfPtr)->iNumber = ifDescriptor[KIfcDesc_NumberOffset];
		}
	TUint8 interfaceNumber = (*aIfPtr)->iNumber;
	TUSB_PRINT1 ("Interface Number %d",interfaceNumber);
	OstTrace1 (TRACE_NORMAL, CACTIVECONTROL_SETUPTRANSFEREDINTERFACE_DUP01, "Interface Number %d",interfaceNumber);

	// Check all endpoint descriptors
	TBuf8<KUsbDescSize_AudioEndpoint> epDescriptor;
	for (TUint i = 0; i < (*aIfPtr)->iInfoPtr->iTotalEndpointsUsed; i++)
		{
		if (!gSkip)
			{
			TestEndpointDescriptor (&iPort[aPortNumber],0,i+1,(*aIfPtr)->iInfoPtr->iEndpointData[i]);

			}

		if (firstBulkOutEndpoint < 0 && ((*aIfPtr)->iInfoPtr->iEndpointData[i].iDir & KUsbEpDirOut) &&
			(*aIfPtr)->iInfoPtr->iEndpointData[i].iType == KUsbEpTypeBulk)
			{
			firstBulkOutEndpoint = i+1;
			}
		}

	TUSB_PRINT1 ("Interface number is %d",interfaceNumber);
	OstTrace1 (TRACE_NORMAL, CACTIVECONTROL_SETUPTRANSFEREDINTERFACE_DUP02, "Interface number is %d",interfaceNumber);
	(*aIfPtr)->iPortNumber = aPortNumber;
	gInterfaceConfig [interfaceNumber] [0] = *aIfPtr;

	TInt alternateNumber = 1;
	// check for any alternatate interfaces and set any that are found
	* aIfPtr = (*aIfPtr)->iPtrNext;
	if (* aIfPtr != NULL)
		{
		test(SupportsAlternateInterfaces());

		IFConfigPtr ifPtr = *aIfPtr;
		while (ifPtr != NULL)
			{
			if (ifPtr->iAlternateSetting)
				{
				ifc = *(ifPtr->iInfoPtr);

				r = iPort[aPortNumber].GetInterfaceDescriptor(alternateNumber, ifDescriptor);
				test_KErrNone(r);

				// Check the interface descriptor
				test(ifDescriptor[KIfcDesc_SettingOffset] == alternateNumber && ifDescriptor[KIfcDesc_NumEndpointsOffset] == (*aIfPtr)->iInfoPtr->iTotalEndpointsUsed &&
				    ifDescriptor[KIfcDesc_ClassOffset] == (*aIfPtr)->iInfoPtr->iClass.iClassNum &&
				    ifDescriptor[KIfcDesc_SubClassOffset] == (*aIfPtr)->iInfoPtr->iClass.iSubClassNum &&
				    ifDescriptor[KIfcDesc_ProtocolOffset] == (*aIfPtr)->iInfoPtr->iClass.iProtocolNum);

				// Check all endpoint descriptors
				for (TUint i = 0; i < ifPtr->iInfoPtr->iTotalEndpointsUsed; i++)
					{
					TInt desc_size;
					r = iPort[aPortNumber].GetEndpointDescriptorSize(alternateNumber, i+1, desc_size);
					test_KErrNone(r);
					test_Equal(KUsbDescSize_Endpoint + (*aIfPtr)->iInfoPtr->iEndpointData[i].iExtra,static_cast<TUint>(desc_size));

					r = iPort[aPortNumber].GetEndpointDescriptor(alternateNumber, i+1, epDescriptor);
					test_KErrNone(r);

					test((((*aIfPtr)->iInfoPtr->iEndpointData[i].iDir & KUsbEpDirIn) && (epDescriptor[KEpDesc_AddressOffset] & 0x80) ||
						!((*aIfPtr)->iInfoPtr->iEndpointData[i].iDir & KUsbEpDirIn) && !(epDescriptor[KEpDesc_AddressOffset] & 0x80)) &&
						EpTypeMask2Value((*aIfPtr)->iInfoPtr->iEndpointData[i].iType) == (TUint)(epDescriptor[KEpDesc_AttributesOffset] & 0x03) &&
						(*aIfPtr)->iInfoPtr->iEndpointData[i].iInterval == epDescriptor[KEpDesc_IntervalOffset]);


					if (!gSkip && (*aIfPtr)->iInfoPtr->iEndpointData[i].iExtra)
						{
						test.Next(_L("Extended Endpoint Descriptor Manipulation"));
						TUint8 addr = 0x85;										// bogus address
						if (epDescriptor[KEpDesc_SynchAddressOffset] == addr)
							addr++;
						epDescriptor[KEpDesc_SynchAddressOffset] = addr;
						r = iPort[aPortNumber].SetEndpointDescriptor(alternateNumber, i+1, epDescriptor);
						test_KErrNone(r);

						TBuf8<KUsbDescSize_AudioEndpoint> descriptor2;
						r = iPort[aPortNumber].GetEndpointDescriptor(alternateNumber, i+1, descriptor2);
						test_KErrNone(r);

						test.Next(_L("Compare endpoint descriptor with value set"));
						r = descriptor2.Compare(epDescriptor);
						test_KErrNone(r);
						}
					}


				// if no error move on to the next interface
				ifPtr->iPortNumber = aPortNumber;
				ifPtr->iNumber = interfaceNumber;
				gInterfaceConfig [interfaceNumber] [alternateNumber] = ifPtr;

				alternateNumber++;
				ifPtr = ifPtr->iPtrNext;
				* aIfPtr = ifPtr;
				}
			else
				{
				ifPtr = NULL;
				}
			}
		}
	iNumInterfaceSettings[aPortNumber] = alternateNumber;
	if (!gSkip)
		{
		TestInvalidSetInterface (&iPort[aPortNumber],iNumInterfaceSettings[aPortNumber]);
		TestInvalidReleaseInterface (&iPort[aPortNumber],iNumInterfaceSettings[aPortNumber]);

		//TestDescriptorManipulation(iLddPtr->iHighSpeed,&iPort[aPortNumber],alternateNumber);
		TestOtgExtensions(&iPort[aPortNumber]);
		TestEndpoint0MaxPacketSizes(&iPort[aPortNumber]);
		}

	test.End();
	}


void CActiveControl::ConstructLOnSharedLdd(const RMessagePtr2& aMsg)
	{
// currently only support one interface with one alternate settings
	test.Start (_L("ConstructLOnSharedLdd Configuration"));

	User::LeaveIfError(iPort[0].Open(aMsg, 0, EOwnerProcess));
	CleanupClosePushL(iPort[0]);

	RChunk* chunk;
	//Get the Ldd's RChunk, but don't own it.
	User::LeaveIfError(iPort[0].GetDataTransferChunk(chunk));
	User::LeaveIfError(chunk->Open(aMsg, 1, FALSE, EOwnerProcess));
	CleanupStack::Pop();


	TInt r;

	User::LeaveIfError(iFs.Connect());

	test_Compare(iConfigFileName->Length(),!=,0);

	iTimer.CreateLocal();
	iPending = EPendingNone;

	test.Next (_L("Open configuration file"));
	// set the session path to use the ROM if no drive specified
	r=iFs.SetSessionPath(_L("Z:\\test\\"));
	test_KErrNone(r);

	r = iConfigFile.Open(iFs, * iConfigFileName, EFileShareReadersOnly | EFileStreamText | EFileRead);
	test_KErrNone(r);
	TUSB_VERBOSE_PRINT1("Configuration file %s Opened successfully", iConfigFileName->PtrZ());
	if(gVerbose)
	    {
	    OstTraceExt1(TRACE_VERBOSE, CACTIVECONTROL_CONSTRUCTLONSHAREDLDD, "Configuration file %s Opened successfully", *iConfigFileName);
	    }

	test.Next (_L("Process configuration file"));
	test(ProcessConfigFile (iConfigFile,iConsole,&iLddPtr));

	iConfigFile.Close();

	test.Next (_L("LDD in configuration file"));
	test_NotNull(iLddPtr);

	LDDConfigPtr lddPtr = iLddPtr;
	TInt nextPort = 0;
	while (lddPtr != NULL)
		{
		// Load logical driver (LDD)
		// (There's no physical driver (PDD) with USB: it's a kernel extension DLL which
		//  was already loaded at boot time.)
		test.Next (_L("Loading USB LDD"));
		TUSB_VERBOSE_PRINT1("Loading USB LDD ",lddPtr->iName.PtrZ());
		if(gVerbose)
		    {
		    OstTraceExt1(TRACE_VERBOSE, CACTIVECONTROL_CONSTRUCTLONSHAREDLDD_DUP01, "Loading USB LDD:%S ",lddPtr->iName);
		    }
		r = User::LoadLogicalDevice(lddPtr->iName);
		test(r == KErrNone || r == KErrAlreadyExists);

		IFConfigPtr ifPtr = lddPtr->iIFPtr;

		test.Next (_L("Opening Channels"));
		TUSB_VERBOSE_PRINT1("Successfully opened USB port %d", lddPtr->iNumChannels);
		if(gVerbose)
		    {
		    OstTrace1(TRACE_VERBOSE, CACTIVECONTROL_CONSTRUCTLONSHAREDLDD_DUP02, "Successfully opened USB port %d", lddPtr->iNumChannels);
		    }
		for (TInt portNumber = nextPort; portNumber < nextPort+lddPtr->iNumChannels; portNumber++)
			{
			test_Compare(lddPtr->iNumChannels,>,0);

			// Open USB channel

			TUSB_VERBOSE_PRINT("Successfully opened USB port");
			if(gVerbose)
			    {
			    OstTrace0(TRACE_VERBOSE, CACTIVECONTROL_CONSTRUCTLONSHAREDLDD_DUP03, "Successfully opened USB port");
			    }

			// Query the USB device/Setup the USB interface
			if (portNumber == nextPort)
				{
				// Change some descriptors to contain suitable values
				SetupDescriptors(lddPtr, &iPort[portNumber]);
				}

			if (portNumber == 0)
				{
				QueryUsbClientL(lddPtr, &iPort[portNumber]);
				}

			test_NotNull(ifPtr);

			if (iSupportResourceAllocationV2)
				{
				PopulateInterfaceResourceAllocation(ifPtr, portNumber);
				}

			IFConfigPtr defaultIfPtr = ifPtr;
			SetupTransferedInterface(&ifPtr,portNumber);


			if (!iSupportResourceAllocationV2)
				{
				// 	allocate endpoint DMA and double buffering for all endpoints on default interface when using resource allocation v1 api
				for (TUint8 i = 1; i <= defaultIfPtr->iInfoPtr->iTotalEndpointsUsed; i++)
					{
					defaultIfPtr->iEpDMA[i-1] ? AllocateEndpointDMA(&iPort[portNumber],(TENDPOINTNUMBER)i) : DeAllocateEndpointDMA(&iPort[portNumber],(TENDPOINTNUMBER)i);
					#ifndef USB_SC
					defaultIfPtr->iEpDoubleBuff[i-1] ? AllocateDoubleBuffering(&iPort[portNumber],(TENDPOINTNUMBER)i) : DeAllocateDoubleBuffering(&iPort[portNumber],(TENDPOINTNUMBER)i);
					#endif
					}
				}
			}

		iTotalChannels += lddPtr->iNumChannels;
		nextPort += lddPtr->iNumChannels;
		lddPtr = lddPtr->iPtrNext;
		}

	TUSB_VERBOSE_PRINT("All Interfaces and Alternate Settings successfully set up");
	if(gVerbose)
	    {
	    OstTrace0(TRACE_VERBOSE, CACTIVECONTROL_CONSTRUCTLONSHAREDLDD_DUP04, "All Interfaces and Alternate Settings successfully set up");
	    }

	test.Next (_L("Start Idle Counter Thread"));
	r = iIdleCounterThread.Create(_L("IdleCounter"), IdleCounterThread, KDefaultStackSize, KMinHeapSize, KMinHeapSize, NULL);
	test_KErrNone(r);
	iIdleCounterThread.Resume();
	// Allow some time for low-priority counter process
	User::After(100000); // 0.1 second
	r = iIdleCounterChunk.OpenGlobal(KTestIdleCounterChunkName, EFalse);
	test_KErrNone(r);
	iIdleCounter = (struct TTestIdleCounter*) iIdleCounterChunk.Base();
	test_NotNull(iIdleCounter);
	// Allow some time for low-priority counter process
	User::After(100000); // 0.1 second
	TInt64 val1 = iIdleCounter->iCounter;
	User::After(1000000); // 1 second
	TInt64 val2 = iIdleCounter->iCounter;
	TUSB_PRINT1("Idle Counter when test inactive: %Ldinc/ms", (val2 - val1) / 1000);
	OstTraceExt1(TRACE_NORMAL, CACTIVECONTROL_CONSTRUCTLONSHAREDLDD_DUP05, "Idle Counter when test inactive: %Ldinc/ms", (val2 - val1) / 1000);

	test.Next (_L("Enumeration..."));
	r = ReEnumerate();
	test_KErrNone(r);

	TUSB_VERBOSE_PRINT("Device successfully re-enumerated\n");
	if(gVerbose)
	    {
	    OstTrace0(TRACE_VERBOSE, CACTIVECONTROL_CONSTRUCTLONSHAREDLDD_DUP06, "Device successfully re-enumerated\n");
	    }


	if (iLddPtr->iHighSpeed && !gSkip)
		{
		test.Next (_L("High Speed"));
		test(iHighSpeed);
		}

	test.Next (_L("Create Notifiers"));
	for (TInt portNumber = 0; portNumber < iTotalChannels; portNumber++)
		{

		// Create device state active object
		iDeviceStateNotifier[portNumber] = CActiveDeviceStateNotifier::NewL(iConsole, &iPort[portNumber], portNumber);
		test_NotNull(iDeviceStateNotifier[portNumber]);
		iDeviceStateNotifier[portNumber]->Activate();
		TUSB_VERBOSE_PRINT("Created device state notifier");
		if(gVerbose)
		    {
		    OstTrace0(TRACE_VERBOSE, CACTIVECONTROL_CONSTRUCTLONSHAREDLDD_DUP07, "Created device state notifier");
		    }

		// Create endpoint stall status active object
		iStallNotifier[portNumber] = CActiveStallNotifier::NewL(iConsole, &iPort[portNumber]);
		test_NotNull(iStallNotifier[portNumber]);
		iStallNotifier[portNumber]->Activate();
		TUSB_VERBOSE_PRINT("Created stall notifier");
		if(gVerbose)
		    {
		    OstTrace0(TRACE_VERBOSE, CACTIVECONTROL_CONSTRUCTLONSHAREDLDD_DUP08, "Created stall notifier");
		    }

		TestInvalidSetInterface (&iPort[portNumber],iNumInterfaceSettings[portNumber]);
		TestInvalidReleaseInterface (&iPort[portNumber],iNumInterfaceSettings[portNumber]);

		}

	test.Next (_L("Endpoint Zero Max Packet Sizes"));
	TUint ep0Size = iPort[0].EndpointZeroMaxPacketSizes();
	switch (ep0Size)
		{
		case KUsbEpSize8 :
			iEp0PacketSize = 8;
			break;

		case KUsbEpSize16 :
			iEp0PacketSize = 16;
			break;

		case KUsbEpSize32 :
			iEp0PacketSize = 32;
			break;

		case KUsbEpSize64 :
			iEp0PacketSize = 64;
			break;

		default:
			iEp0PacketSize = 0;
			break;
		}
	test_Compare(iEp0PacketSize,>,0);

	test.Next (_L("Set Device Control"));
	r = iPort[0].SetDeviceControl();
	test_KErrNone(r);

	#ifdef USB_SC
	r = iPort[0].OpenEndpoint(iEp0Buf,0);
	test_KErrNone(r);
	#endif

	test.End();
	RequestEp0ControlPacket();
	}

#endif

void OpenStackIfOtg()
	{
	// On an OTG device we have to start the OTG driver, otherwise the Client
	// stack will remain disabled forever.
	if (gSupportsOtg)
		{
		test.Start(_L("Running on OTG device: loading OTG driver\n"));
		test.Next(_L("Load OTG LDD"));
		TInt r = User::LoadLogicalDevice(KOtgdiLddFilename);
		test((r == KErrNone) || (r == KErrAlreadyExists));

		test.Next(_L("Open OTG channel"));
		r = gOtgPort.Open();
		test(r == KErrNone);

		test.Next(_L("Start OTG stack"));
		r = gOtgPort.StartStacks();
		test(r == KErrNone);
		test.End();
		}
	}

void CloseStackIfOtg()
	{
	if (gSupportsOtg)
		{
		test.Start(_L("Close OTG stack\n"));
		test.Next(_L("Stop OTG stack"));
		gOtgPort.StopStacks();
		test.Next(_L("Close OTG Channel"));
		gOtgPort.Close();
		test.Next(_L("Free OTG LDD"));
		TInt r = User::FreeLogicalDevice(RUsbOtgDriver::Name());
		test(r == KErrNone);
		test.End();
		}
	}
	
// -eof-
