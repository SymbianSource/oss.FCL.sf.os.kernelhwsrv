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
// @file usbclientstatewatcher.cpp
// @internalComponent
// 
//

#include "UsbClientStateWatcher.h"
#include "OstTraceDefinitions.h"
#ifdef OST_TRACE_COMPILER_IN_USE
#include "UsbClientStateWatcherTraces.h"
#endif
#include <d32usbc.h>
#include <e32test.h>
#include <e32debug.h>

extern RTest gtest;

namespace NUnitTesting_USBDI
	{

CUsbClientStateWatcher* CUsbClientStateWatcher::NewL(RDevUsbcClient& aClientDriver,MUsbClientStateObserver& aStateObserver)
	{
	OstTraceFunctionEntryExt( CUSBCLIENTSTATEWATCHER_NEWL_ENTRY, 0 );
	CUsbClientStateWatcher* self = new (ELeave) CUsbClientStateWatcher(aClientDriver,aStateObserver);
	CleanupStack::PushL(self);
	self->ConstructL();
	CleanupStack::Pop(self);
	OstTraceFunctionExit1( CUSBCLIENTSTATEWATCHER_NEWL_EXIT, ( TUint )( self ) );
	return self;
	}
	
	
CUsbClientStateWatcher::CUsbClientStateWatcher(RDevUsbcClient& aClientDriver,MUsbClientStateObserver& aStateObserver)
:	CActive(EPriorityUserInput),
	iClientDriver(aClientDriver),
	iStateObserver(aStateObserver)
	{
	OstTraceFunctionEntryExt( CUSBCLIENTSTATEWATCHER_CUSBCLIENTSTATEWATCHER_ENTRY, this );
	CActiveScheduler::Add(this);
	OstTraceFunctionExit1( CUSBCLIENTSTATEWATCHER_CUSBCLIENTSTATEWATCHER_EXIT, this );
	}


CUsbClientStateWatcher::~CUsbClientStateWatcher()
	{
	OstTraceFunctionEntry1( CUSBCLIENTSTATEWATCHER_CUSBCLIENTSTATEWATCHER_ENTRY_DUP01, this );
	Cancel();
	OstTraceFunctionExit1( CUSBCLIENTSTATEWATCHER_CUSBCLIENTSTATEWATCHER_EXIT_DUP01, this );
	}
	
	
void CUsbClientStateWatcher::ConstructL()
	{
	OstTraceFunctionEntry1( CUSBCLIENTSTATEWATCHER_CONSTRUCTL_ENTRY, this );
	OstTrace0(TRACE_NORMAL, CUSBCLIENTSTATEWATCHER_CONSTRUCTL, "<Client State Watcher> Watching state of device");
	iClientDriver.AlternateDeviceStatusNotify(iStatus,iState);
	SetActive();
	OstTraceFunctionExit1( CUSBCLIENTSTATEWATCHER_CONSTRUCTL_EXIT, this );
	}


void CUsbClientStateWatcher::DoCancel()
	{
	OstTraceFunctionEntry1( CUSBCLIENTSTATEWATCHER_DOCANCEL_ENTRY, this );
	// Cancel device status notification
	iClientDriver.AlternateDeviceStatusNotifyCancel();	
	OstTraceFunctionExit1( CUSBCLIENTSTATEWATCHER_DOCANCEL_EXIT, this );
	}

void CUsbClientStateWatcher::RunL()
	{
	OstTraceFunctionEntry1( CUSBCLIENTSTATEWATCHER_RUNL_ENTRY, this );
	// Retrieve the asynchronous completion code
	TInt completionCode(iStatus.Int());
	
	if(iState & KUsbAlternateSetting)
		{
		// This is notification for alternate interface setting selected by the host
		// so do nothing (Do not watch for these)
		}
	else
		{
		if(completionCode == KErrNone)
			{
			switch(iState)
				{
				case EUsbcDeviceStateUndefined:
					OstTrace0(TRACE_NORMAL, CUSBCLIENTSTATEWATCHER_RUNL, "<Client State> Not attached");
					break;
				
				case EUsbcDeviceStateAttached:
					OstTrace0(TRACE_NORMAL, CUSBCLIENTSTATEWATCHER_RUNL_DUP01, "<Client State> Attached to host but not powered");
					break;
					
				case EUsbcDeviceStatePowered:
					OstTrace0(TRACE_NORMAL, CUSBCLIENTSTATEWATCHER_RUNL_DUP02, "<Client State> Attached and powered but no reset");
					break;
					
				case EUsbcDeviceStateDefault:
					OstTrace0(TRACE_NORMAL, CUSBCLIENTSTATEWATCHER_RUNL_DUP03, "<Client State> Reset but not addressed");
					break;
					
				case EUsbcDeviceStateAddress:
					OstTrace0(TRACE_NORMAL, CUSBCLIENTSTATEWATCHER_RUNL_DUP04, "<Client State> Addressed but not configured");
					break;
	 
				case EUsbcDeviceStateConfigured:
					OstTrace0(TRACE_NORMAL, CUSBCLIENTSTATEWATCHER_RUNL_DUP05, "<Client State> Fully configured");
					break;
	 
				case EUsbcDeviceStateSuspended:
					OstTrace0(TRACE_NORMAL, CUSBCLIENTSTATEWATCHER_RUNL_DUP06, "<Client State> Suspended");
					break;
					
				case EUsbcNoState: //follow through
				default:
					OstTrace0(TRACE_NORMAL, CUSBCLIENTSTATEWATCHER_RUNL_DUP07, "<Client State> Not specified");
					break;
				}
			}
		else
			{
			OstTrace1(TRACE_NORMAL, CUSBCLIENTSTATEWATCHER_RUNL_DUP08, "<Client State> Notification error %d",completionCode);
			}
		
		// Device state change
		iStateObserver.StateChangeL(static_cast<TUsbcDeviceState>(iState),completionCode);		
		}
			
	// Keep asking to be informed for status notifications
	iClientDriver.AlternateDeviceStatusNotify(iStatus,iState);
	SetActive();	
	OstTraceFunctionExit1( CUSBCLIENTSTATEWATCHER_RUNL_EXIT, this );
	}
	
	
TInt CUsbClientStateWatcher::RunError(TInt aError)
	{
	OstTraceFunctionEntryExt( CUSBCLIENTSTATEWATCHER_RUNERROR_ENTRY, this );
	aError = KErrNone;
	OstTraceFunctionExitExt( CUSBCLIENTSTATEWATCHER_RUNERROR_EXIT, this, aError );
	return aError;
	}






CAlternateInterfaceSelectionWatcher* CAlternateInterfaceSelectionWatcher::NewL(
	RDevUsbcClient& aClientDriver,MAlternateSettingObserver& aObserver)
	{
	OstTraceFunctionEntryExt( CALTERNATEINTERFACESELECTIONWATCHER_NEWL_ENTRY, 0 );
	CAlternateInterfaceSelectionWatcher* self = new (ELeave) CAlternateInterfaceSelectionWatcher(aClientDriver,aObserver);
	CleanupStack::PushL(self);
	self->ConstructL();
	CleanupStack::Pop(self);
	OstTraceFunctionExit1( CALTERNATEINTERFACESELECTIONWATCHER_NEWL_EXIT, ( TUint )( self ) );
	return self;
	}
	
	
CAlternateInterfaceSelectionWatcher::CAlternateInterfaceSelectionWatcher(
	RDevUsbcClient& aClientDriver,MAlternateSettingObserver& aObserver)
:	CActive(EPriorityUserInput),
	iClientDriver(aClientDriver),
	iObserver(aObserver)
	{
	OstTraceFunctionEntryExt( CALTERNATEINTERFACESELECTIONWATCHER_CALTERNATEINTERFACESELECTIONWATCHER_ENTRY, this );
	CActiveScheduler::Add(this);
	OstTraceFunctionExit1( CALTERNATEINTERFACESELECTIONWATCHER_CALTERNATEINTERFACESELECTIONWATCHER_EXIT, this );
	}
	
	
CAlternateInterfaceSelectionWatcher::~CAlternateInterfaceSelectionWatcher()
	{
    OstTraceFunctionEntry1( CALTERNATEINTERFACESELECTIONWATCHER_CALTERNATEINTERFACESELECTIONWATCHER_ENTRY_DUP01, this );

	Cancel();
	OstTraceFunctionExit1( CALTERNATEINTERFACESELECTIONWATCHER_CALTERNATEINTERFACESELECTIONWATCHER_EXIT_DUP01, this );
	}
	
	
void CAlternateInterfaceSelectionWatcher::ConstructL()
	{
    OstTraceFunctionEntry1( CALTERNATEINTERFACESELECTIONWATCHER_CONSTRUCTL_ENTRY, this );

	iClientDriver.AlternateDeviceStatusNotify(iStatus,iState);
	SetActive();	
	OstTraceFunctionExit1( CALTERNATEINTERFACESELECTIONWATCHER_CONSTRUCTL_EXIT, this );
	}


void CAlternateInterfaceSelectionWatcher::DoCancel()
	{
    OstTraceFunctionEntry1( CALTERNATEINTERFACESELECTIONWATCHER_DOCANCEL_ENTRY, this );

	iClientDriver.AlternateDeviceStatusNotifyCancel();
	OstTraceFunctionExit1( CALTERNATEINTERFACESELECTIONWATCHER_DOCANCEL_EXIT, this );
	}
	
	
void CAlternateInterfaceSelectionWatcher::RunL()
	{
    OstTraceFunctionEntry1( CALTERNATEINTERFACESELECTIONWATCHER_RUNL_ENTRY, this );

	TInt completionCode(iStatus.Int());
	
	if(iState & KUsbAlternateSetting)
		{
		iObserver.AlternateInterfaceSelectedL(iState & (~KUsbAlternateSetting));
		}	
	// Keep asking to be informed for status notifications
	iClientDriver.AlternateDeviceStatusNotify(iStatus,iState);
	SetActive();	
	OstTraceFunctionExit1( CALTERNATEINTERFACESELECTIONWATCHER_RUNL_EXIT, this );
	}


TInt CAlternateInterfaceSelectionWatcher::RunError(TInt aError)
	{
    OstTraceFunctionEntryExt( CALTERNATEINTERFACESELECTIONWATCHER_RUNERROR_ENTRY, this );

	OstTraceFunctionExitExt( CALTERNATEINTERFACESELECTIONWATCHER_RUNERROR_EXIT, this, KErrNone );
	return KErrNone;
	}














	}
