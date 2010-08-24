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
// e32test/usb/t_usb_device/src/activestatenotifier.cpp
// USB Test Program T_USB_DEVICE, functional part.
// Device-side part, to work against T_USB_HOST running on the host.
//
//

#include "general.h"
#include "activerw.h"									// CActiveRW
#include "config.h"
#include "activeControl.h"
#include "OstTraceDefinitions.h"
#ifdef OST_TRACE_COMPILER_IN_USE
#include "activedevicestatenotifierTraces.h"
#endif
#include "activedevicestatenotifier.h"

extern CActiveControl* gActiveControl;
extern RTest test;
extern TBool gVerbose;
extern TBool gSkip;
extern TBool gStopOnFail;
extern TBool gAltSettingOnNotify;
extern TInt gSoakCount;
extern CActiveRW* gRW[KMaxConcurrentTests];				// the USB read/write active object
extern IFConfigPtr gInterfaceConfig [128] [KMaxInterfaceSettings];

//
// --- class CActiveDeviceStateNotifier ---------------------------------------------------------
//

CActiveDeviceStateNotifier::CActiveDeviceStateNotifier(CConsoleBase* aConsole, RDEVCLIENT* aPort, TUint aPortNumber)
	: CActive(EPriorityNormal),
	  iConsole(aConsole),
	  iPort(aPort),
	  iDeviceState(0),
	  iPortNumber(aPortNumber)
	{
	CActiveScheduler::Add(this);
	}

CActiveDeviceStateNotifier* CActiveDeviceStateNotifier::NewL(CConsoleBase* aConsole, RDEVCLIENT* aPort, TUint aPortNumber)
	{
	CActiveDeviceStateNotifier* self = new (ELeave) CActiveDeviceStateNotifier(aConsole, aPort, aPortNumber);
	CleanupStack::PushL(self);
	self->ConstructL();
	CleanupStack::Pop();									// self
	return self;
	}


void CActiveDeviceStateNotifier::ConstructL()
	{}


CActiveDeviceStateNotifier::~CActiveDeviceStateNotifier()
	{
	TUSB_VERBOSE_PRINT("CActiveDeviceStateNotifier::~CActiveDeviceStateNotifier()");
	if(gVerbose)
	    {
	    OstTrace0(TRACE_VERBOSE, CACTIVEDEVICESTATENOTIFIER_DCACTIVEDEVICESTATENOTIFIER, "CActiveDeviceStateNotifier::~CActiveDeviceStateNotifier()");
	    }
	Cancel();												// base class
	}


void CActiveDeviceStateNotifier::DoCancel()
	{
	TUSB_VERBOSE_PRINT("CActiveDeviceStateNotifier::DoCancel()");
	if(gVerbose)
	    {
	    OstTrace0(TRACE_VERBOSE, CACTIVEDEVICESTATENOTIFIER_DOCANCEL, "CActiveDeviceStateNotifier::DoCancel()");
	    }
	iPort->AlternateDeviceStatusNotifyCancel();
	}


void CActiveDeviceStateNotifier::RunL()
	{
	// This displays the device state.
	// In a real world program, the user could take here appropriate action (cancel a
	// transfer request or whatever).
	if (!(iDeviceState & KUsbAlternateSetting) && gVerbose)
		{
		switch (iDeviceState)
			{
		case EUsbcDeviceStateUndefined:
#ifdef	USB_SC
			TUSB_PRINT("Device State notifier: Undefined0");
			OstTrace0(TRACE_NORMAL, CACTIVEDEVICESTATENOTIFIER_RUNL, "Device State notifier: Undefined0");
			for (TUint16 i =0; i < KMaxConcurrentTests; i++)
				{
				if (gRW[i])
					{
					TUSB_VERBOSE_PRINT2("ResetAltSetting index %d, LDD %x",i, gRW[i]->Ldd());
					if(gVerbose)
					    {
					    OstTraceExt2(TRACE_VERBOSE, CACTIVEDEVICESTATENOTIFIER_RUNL_DUP01, "ResetAltSetting index %u, LDD %x",(TUint32)i, (TUint32)gRW[i]->Ldd());
					    }
					gRW[i]->Ldd()->ResetAltSetting();
					}
				}
#endif
			TUSB_PRINT("Device State notifier: Undefined");
			OstTrace0(TRACE_NORMAL, CACTIVEDEVICESTATENOTIFIER_RUNL_DUP02, "Device State notifier: Undefined");
			break;
		case EUsbcDeviceStateAttached:
			TUSB_PRINT("Device State notifier: Attached");
			OstTrace0(TRACE_NORMAL, CACTIVEDEVICESTATENOTIFIER_RUNL_DUP03, "Device State notifier: Attached");
			break;
		case EUsbcDeviceStatePowered:
			TUSB_PRINT("Device State notifier: Powered");
			OstTrace0(TRACE_NORMAL, CACTIVEDEVICESTATENOTIFIER_RUNL_DUP04, "Device State notifier: Powered");
			break;
		case EUsbcDeviceStateDefault:
			TUSB_PRINT("Device State notifier: Default");
			OstTrace0(TRACE_NORMAL, CACTIVEDEVICESTATENOTIFIER_RUNL_DUP05, "Device State notifier: Default");
			break;
		case EUsbcDeviceStateAddress:
			TUSB_PRINT("Device State notifier: Address");
			OstTrace0(TRACE_NORMAL, CACTIVEDEVICESTATENOTIFIER_RUNL_DUP06, "Device State notifier: Address");
			break;
		case EUsbcDeviceStateConfigured:
			TUSB_PRINT("Device State notifier: Configured");
			OstTrace0(TRACE_NORMAL, CACTIVEDEVICESTATENOTIFIER_RUNL_DUP07, "Device State notifier: Configured");
			break;
		case EUsbcDeviceStateSuspended:
			TUSB_PRINT("Device State notifier: Suspended");
			OstTrace0(TRACE_NORMAL, CACTIVEDEVICESTATENOTIFIER_RUNL_DUP08, "Device State notifier: Suspended");
			break;
		default:
			TUSB_PRINT("Device State notifier: ***BAD***");
			OstTrace0(TRACE_NORMAL, CACTIVEDEVICESTATENOTIFIER_RUNL_DUP09, "Device State notifier: ***BAD***");
			}
		}
	else if (iDeviceState & KUsbAlternateSetting)
		{
		TUint8 altSetting = iDeviceState & ~KUsbAlternateSetting;
		TUSB_PRINT2("Device State notifier: Alternate interface %d setting has changed: now %d",
					iPortNumber, altSetting);
		OstTraceExt2(TRACE_NORMAL, CACTIVEDEVICESTATENOTIFIER_RUNL_DUP10, "Device State notifier: Alternate interface %u setting has changed: now %u",
					(TUint32)iPortNumber, (TUint32)altSetting);

		TUsbDeviceCaps dCaps;
		iPort->DeviceCaps(dCaps);
		TBool isResourceAllocationV2 = ((dCaps().iFeatureWord1 & KUsbDevCapsFeatureWord1_EndpointResourceAllocV2) != 0);
		if (!isResourceAllocationV2)
			{
			// allocate endpoint DMA and double buffering for all endpoints on interface for resource allocation v1
			// if resource allocation v2, refer to CActiveControl::ConstructL and CActiveControl::PopulateInterfaceResourceAllocation
			for (TUint8 ifNumber = 0; ifNumber < 128; ifNumber++)
				{
				IFConfigPtr newIfPtr = gInterfaceConfig[ifNumber][altSetting];
				if (newIfPtr)
					{
					if (newIfPtr->iPortNumber == iPortNumber)
						{
						// 	allocate endpoint DMA and double buffering for all endpoints on default interface
						for (TUint8 i = 1; i <= newIfPtr->iInfoPtr->iTotalEndpointsUsed; i++)
							{
							newIfPtr->iEpDMA[i-1] ? gActiveControl->AllocateEndpointDMA(iPort,(TENDPOINTNUMBER)i) : gActiveControl->DeAllocateEndpointDMA(iPort,(TENDPOINTNUMBER)i);
							#ifndef USB_SC
							newIfPtr->iEpDoubleBuff[i-1] ? gActiveControl->AllocateDoubleBuffering(iPort,(TENDPOINTNUMBER)i) : gActiveControl->DeAllocateDoubleBuffering(iPort,(TENDPOINTNUMBER)i);
							#endif
							}
						break;
						}
					}
				}
			}

		if (gAltSettingOnNotify)
			{
			for (TUint16 i =0; i < KMaxConcurrentTests; i++)
				{
				if (gRW[i])
					{
					TUSB_VERBOSE_PRINT1("Resuming alternate Setting - activeRW index %d",i);
					if(gVerbose)
					    {
					    OstTrace1(TRACE_VERBOSE, CACTIVEDEVICESTATENOTIFIER_RUNL_DUP11, "Resuming alternate Setting - activeRW index %d",i);
					    }
					gRW[i]->ResumeAltSetting(altSetting);
					}
				}
			}
		}
	Activate();
	}


void CActiveDeviceStateNotifier::Activate()
	{
	__ASSERT_ALWAYS(!IsActive(), User::Panic(KActivePanic, 661));
	iPort->AlternateDeviceStatusNotify(iStatus, iDeviceState);
	SetActive();
	}


// -eof-
