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
// Implementation of MS drive state watcher
// 
//

/**
 @file
 @internalTechnology
*/

#include "t_ms_main.h"
#include "cpropertywatch.h"
#include "cstatemachine.h"

LOCAL_D TInt testedDriveIndex = -1;
 
///////////////////////////////////////////////////////////////////////////
//	class CPropertyWatch
///////////////////////////////////////////////////////////////////////////

CPropertyWatch* CPropertyWatch::NewLC(TUsbMsDriveState_Subkey aSubkey, CPropertyHandler& aHandler)
	{
	CPropertyWatch* me=new(ELeave) CPropertyWatch(aHandler);
	CleanupStack::PushL(me);
	me->ConstructL(aSubkey);
	return me;
	}

CPropertyWatch::CPropertyWatch(CPropertyHandler& aHandler)
	: CActive(0), iHandler(aHandler)
	{}

void CPropertyWatch::ConstructL(TUsbMsDriveState_Subkey aSubkey)
	{
	User::LeaveIfError(iProperty.Attach(KUsbMsDriveState_Category, aSubkey));
	CActiveScheduler::Add(this);
	// initial subscription and process current property value
	RunL();
	}

CPropertyWatch::~CPropertyWatch()
	{
	Cancel();
	iProperty.Close();
	}

void CPropertyWatch::DoCancel()
	{
	iProperty.Cancel();
	}

void CPropertyWatch::RunL()
	{
	// resubscribe before processing new value to prevent missing updates
	iProperty.Subscribe(iStatus);
	iHandler.HandleStatusChange(iProperty);
	SetActive();
	}

///////////////////////////////////////////////////////////////////////////
//	class CPropertyHandler
///////////////////////////////////////////////////////////////////////////

CPropertyHandler::CPropertyHandler(TInt aDriveNo, CStateMachine& aSm)
 	: iDriveNo(aDriveNo)
 	, iStateMachine(aSm)
	{
	}
	
CPropertyHandler::~CPropertyHandler()
	{
	}
	
///////////////////////////////////////////////////////////////////////////
//	class CMsDriveStatusHandler
///////////////////////////////////////////////////////////////////////////

CMsDriveStatusHandler*
CMsDriveStatusHandler::NewLC(TInt aDriveNo, CStateMachine& aSm)
	{
	CMsDriveStatusHandler* self = new (ELeave) CMsDriveStatusHandler(aDriveNo, aSm);
	CleanupStack::PushL(self);
	return self;
	};

CMsDriveStatusHandler::CMsDriveStatusHandler(TInt aDriveNo, CStateMachine& aSm)
	: CPropertyHandler(aDriveNo, aSm)
	{
	}
	
void
CMsDriveStatusHandler::HandleStatusChange(RProperty& aProperty)
	{
	TInt driveStatus = -1;
    TInt currentStatus = iStateMachine.CurrentStateId();
	TBuf8<16> allDrivesStatus;	
	TInt ret = aProperty.Get(allDrivesStatus);
	test.Printf(_L("Property.Get() return value: %d\n"), ret); 
	test(ret == KErrNone);

	if (testedDriveIndex < 0)
		{
		for(TInt i=0; i<allDrivesStatus.Length()/2; i++)
			{
			if (allDrivesStatus[2*i] == iDriveNo)
				{
				testedDriveIndex = i;
				break;
				}
			}
		}
			
	if (testedDriveIndex < 0)
		{
		test.Printf(_L("Tested drive %d not found\n"), iDriveNo);
		test(EFalse);
		}

	driveStatus = allDrivesStatus[2*testedDriveIndex + 1]; 
		
    switch (currentStatus)
    	{
    	case EUsbMsDriveState_Disconnected:
    		if (iStateMachine.FromStateId() == EUsbMsDriveState_Disconnected)
    			{
    			iStateMachine.MoveTo(EUsbMsDriveState_Connected);
    			iStateMachine.SetFromStateId(EUsbMsDriveState_Disconnected);
    			}
    		else if (iStateMachine.FromStateId() == EUsbMsDriveState_Disconnecting)
    			{
    			test(driveStatus == currentStatus);
    			iStateMachine.MoveTo(EUsbMsDriveState_Connecting);
    			iStateMachine.SetFromStateId(EUsbMsDriveState_Disconnected);
    			}
    		else if (iStateMachine.FromStateId() == EUsbMsState_Read)
    			{
    			CActiveScheduler::Stop();
    			}
    		break;
    	case EUsbMsDriveState_Connecting:
    		if (iStateMachine.FromStateId() == EUsbMsDriveState_Disconnected)
    			{
    			iStateMachine.MoveTo(EUsbMsState_Written);
    			iStateMachine.SetFromStateId(EUsbMsDriveState_Connecting);
    			}
    		break;
    	case EUsbMsDriveState_Connected:
    		if (iStateMachine.FromStateId() == EUsbMsDriveState_Disconnected)
    			{
    			test(driveStatus == currentStatus);
    			iStateMachine.MoveTo(EUsbMsDriveState_Active);
    			iStateMachine.SetFromStateId(EUsbMsDriveState_Connected);
    			}
    		break;
    	case EUsbMsDriveState_Disconnecting:
    		if (iStateMachine.FromStateId() == EUsbMsDriveState_Active)
    			{
    			test(driveStatus == currentStatus);
    			iStateMachine.MoveTo(EUsbMsDriveState_Disconnected);
    			iStateMachine.SetFromStateId(EUsbMsDriveState_Disconnecting);
    			}
    		
    		break;
    	case EUsbMsDriveState_Active:
    		if (iStateMachine.FromStateId() == EUsbMsDriveState_Connected)
    			{
    			test(driveStatus == currentStatus);
    			iStateMachine.MoveTo(EUsbMsDriveState_Disconnecting);
    			iStateMachine.SetFromStateId(EUsbMsDriveState_Active);
    			}
    		break;
    	case EUsbMsDriveState_Locked:
    		iStateMachine.MoveTo(EUsbMsDriveState_Disconnecting);
    		break;
    	default:
    		test.Printf(_L("uninteresting drive status: %d\n"), currentStatus);
    		break;
    	}
	}
	
///////////////////////////////////////////////////////////////////////////
//	class CMsReadStatusHandler
///////////////////////////////////////////////////////////////////////////

CMsReadStatusHandler*
CMsReadStatusHandler::NewLC(TInt aDriveNo, CStateMachine& aSm)
	{
	CMsReadStatusHandler* self = new (ELeave) CMsReadStatusHandler(aDriveNo, aSm);
	CleanupStack::PushL(self);
	return self;
	};
	
CMsReadStatusHandler::CMsReadStatusHandler(TInt aDriveNo, CStateMachine& aSm)
	: CPropertyHandler(aDriveNo, aSm)
	{
	}
	
void
CMsReadStatusHandler::HandleStatusChange(RProperty& aProperty)
	{
    TInt currentStatus = iStateMachine.CurrentStateId();
	TUsbMsBytesTransferred kBytes;
	
	TInt ret = aProperty.Get(kBytes);
	test.Printf(_L("Read Property.Get() return value: %d\n"), ret); 
	test(ret == KErrNone);
	
    switch (currentStatus)
    	{
    	case EUsbMsState_Read:
    	    if (iStateMachine.FromStateId() == EUsbMsState_Written
    	    	&& (1 == kBytes[testedDriveIndex]))
    			{
    			iStateMachine.MoveTo(EUsbMsDriveState_Disconnected);
    			iStateMachine.SetFromStateId(EUsbMsState_Read);
    			}
    		break;
		case EUsbMsDriveState_Active:
    		if (iStateMachine.FromStateId() == EUsbMsDriveState_Connected)
    			{
    			iStateMachine.MoveTo(EUsbMsDriveState_Disconnecting);
    			iStateMachine.SetFromStateId(EUsbMsDriveState_Active);
    			}
    		break;
    	default:
    		test.Printf(_L("uninteresting read status: %d\n"), currentStatus);
    		break;
    	}
	}

///////////////////////////////////////////////////////////////////////////
//	class CMsWrittenStatusHandler
///////////////////////////////////////////////////////////////////////////

CMsWrittenStatusHandler*
CMsWrittenStatusHandler::NewLC(TInt aDriveNo, CStateMachine& aSm)
	{
	CMsWrittenStatusHandler* self = new (ELeave) CMsWrittenStatusHandler(aDriveNo, aSm);
	CleanupStack::PushL(self);
	return self;
	};

CMsWrittenStatusHandler::CMsWrittenStatusHandler(TInt aDriveNo, CStateMachine& aSm)
	: CPropertyHandler(aDriveNo, aSm)
	{
	}
	
void
CMsWrittenStatusHandler::HandleStatusChange(RProperty& aProperty)
	{
    TInt currentStatus = iStateMachine.CurrentStateId();
	TUsbMsBytesTransferred kBytes;
	
	TInt ret = aProperty.Get(kBytes);
	test.Printf(_L("Written Property.Get() return value: %d\n"), ret); 
	test(ret == KErrNone);
    test.Printf(_L("Kilobytes written: %d\n"), kBytes[testedDriveIndex]);
	
    switch (currentStatus)
    	{
    	case EUsbMsState_Written:
    		if (iStateMachine.FromStateId() == EUsbMsDriveState_Connecting &&
				kBytes[testedDriveIndex] == 1)
				{				
    			iStateMachine.MoveTo(EUsbMsState_Read);
    			iStateMachine.SetFromStateId(EUsbMsState_Written);
    			}
    		break;
    	default:
    		test.Printf(_L("uninteresting write status: %d\n"), currentStatus);
    		break;
    	}
	}
