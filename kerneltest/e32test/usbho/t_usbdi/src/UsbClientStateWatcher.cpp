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
// @file usbclientstatewatcher.cpp
// @internalComponent
// 
//

#include "UsbClientStateWatcher.h"
#include <d32usbc.h>
#include <e32test.h>
#include <e32debug.h>

extern RTest gtest;

namespace NUnitTesting_USBDI
	{

CUsbClientStateWatcher* CUsbClientStateWatcher::NewL(RDevUsbcClient& aClientDriver,MUsbClientStateObserver& aStateObserver)
	{
	CUsbClientStateWatcher* self = new (ELeave) CUsbClientStateWatcher(aClientDriver,aStateObserver);
	CleanupStack::PushL(self);
	self->ConstructL();
	CleanupStack::Pop(self);
	return self;
	}
	
	
CUsbClientStateWatcher::CUsbClientStateWatcher(RDevUsbcClient& aClientDriver,MUsbClientStateObserver& aStateObserver)
:	CActive(EPriorityUserInput),
	iClientDriver(aClientDriver),
	iStateObserver(aStateObserver)
	{
	CActiveScheduler::Add(this);
	}


CUsbClientStateWatcher::~CUsbClientStateWatcher()
	{
	LOG_FUNC
	Cancel();
	}
	
	
void CUsbClientStateWatcher::ConstructL()
	{
	RDebug::Printf("<Client State Watcher> Watching state of device");
	iClientDriver.AlternateDeviceStatusNotify(iStatus,iState);
	SetActive();
	}


void CUsbClientStateWatcher::DoCancel()
	{
	// Cancel device status notification
	iClientDriver.AlternateDeviceStatusNotifyCancel();	
	}

void CUsbClientStateWatcher::RunL()
	{
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
					RDebug::Printf("<Client State> Not attached");
					break;
				
				case EUsbcDeviceStateAttached:
					RDebug::Printf("<Client State> Attached to host but not powered");
					break;
					
				case EUsbcDeviceStatePowered:
					RDebug::Printf("<Client State> Attached and powered but no reset");
					break;
					
				case EUsbcDeviceStateDefault:
					RDebug::Printf("<Client State> Reset but not addressed");
					break;
					
				case EUsbcDeviceStateAddress:
					RDebug::Printf("<Client State> Addressed but not configured");
					break;
	 
				case EUsbcDeviceStateConfigured:
					RDebug::Printf("<Client State> Fully configured");
					break;
	 
				case EUsbcDeviceStateSuspended:
					RDebug::Printf("<Client State> Suspended");
					break;
					
				case EUsbcNoState: //follow through
				default:
					RDebug::Printf("<Client State> Not specified");
					break;
				}
			}
		else
			{
			RDebug::Printf("<Client State> Notification error %d",completionCode);
			}
		
		// Device state change
		iStateObserver.StateChangeL(static_cast<TUsbcDeviceState>(iState),completionCode);		
		}
			
	// Keep asking to be informed for status notifications
	iClientDriver.AlternateDeviceStatusNotify(iStatus,iState);
	SetActive();	
	}
	
	
TInt CUsbClientStateWatcher::RunError(TInt aError)
	{
	aError = KErrNone;
	return aError;
	}






CAlternateInterfaceSelectionWatcher* CAlternateInterfaceSelectionWatcher::NewL(
	RDevUsbcClient& aClientDriver,MAlternateSettingObserver& aObserver)
	{
	CAlternateInterfaceSelectionWatcher* self = new (ELeave) CAlternateInterfaceSelectionWatcher(aClientDriver,aObserver);
	CleanupStack::PushL(self);
	self->ConstructL();
	CleanupStack::Pop(self);
	return self;
	}
	
	
CAlternateInterfaceSelectionWatcher::CAlternateInterfaceSelectionWatcher(
	RDevUsbcClient& aClientDriver,MAlternateSettingObserver& aObserver)
:	CActive(EPriorityUserInput),
	iClientDriver(aClientDriver),
	iObserver(aObserver)
	{
	CActiveScheduler::Add(this);
	}
	
	
CAlternateInterfaceSelectionWatcher::~CAlternateInterfaceSelectionWatcher()
	{
	LOG_FUNC

	Cancel();
	}
	
	
void CAlternateInterfaceSelectionWatcher::ConstructL()
	{
	LOG_FUNC

	iClientDriver.AlternateDeviceStatusNotify(iStatus,iState);
	SetActive();	
	}


void CAlternateInterfaceSelectionWatcher::DoCancel()
	{
	LOG_FUNC

	iClientDriver.AlternateDeviceStatusNotifyCancel();
	}
	
	
void CAlternateInterfaceSelectionWatcher::RunL()
	{
	LOG_FUNC

	TInt completionCode(iStatus.Int());
	
	if(iState & KUsbAlternateSetting)
		{
		iObserver.AlternateInterfaceSelectedL(iState & (~KUsbAlternateSetting));
		}	
	// Keep asking to be informed for status notifications
	iClientDriver.AlternateDeviceStatusNotify(iStatus,iState);
	SetActive();	
	}


TInt CAlternateInterfaceSelectionWatcher::RunError(TInt aError)
	{
	LOG_FUNC

	return KErrNone;
	}














	}
