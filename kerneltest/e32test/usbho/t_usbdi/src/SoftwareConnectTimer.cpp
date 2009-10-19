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
// @file softwareconnecttimer.cpp
// @internalComponent
// 
//

#include "softwareconnecttimer.h"

namespace NUnitTesting_USBDI
	{
	
const TInt KOneSecond(1000000);
	
CSoftwareConnectTimer* CSoftwareConnectTimer::NewL(RUsbTestDevice& aTestDevice)
	{
	CSoftwareConnectTimer* self = new (ELeave) CSoftwareConnectTimer(aTestDevice);
	CleanupStack::PushL(self);
	self->ConstructL();
	CleanupStack::Pop(self);
	return self;
	}
	
	
CSoftwareConnectTimer::CSoftwareConnectTimer(RUsbTestDevice& aTestDevice)
:	CTimer(EPriorityStandard),
	iTestDevice(aTestDevice),
	iConnectType(EUnknown)
	{
	CActiveScheduler::Add(this);
	}
	
	
CSoftwareConnectTimer::~CSoftwareConnectTimer()
	{
	LOG_FUNC
	}
	
	
void CSoftwareConnectTimer::SoftwareConnect(TInt aInterval)
	{
	LOG_FUNC
	iConnectType = EConnect;
	After(aInterval*KOneSecond);
	}
	

void CSoftwareConnectTimer::SoftwareDisconnect(TInt aInterval)
	{
	LOG_FUNC
	iConnectType = EDisconnect;
	After(aInterval*KOneSecond);
	}
	
	
void CSoftwareConnectTimer::SoftwareReConnect(TInt aInterval)
	{
	LOG_FUNC
	iTestDevice.SoftwareDisconnect();
	SoftwareConnect(aInterval);
	}


void CSoftwareConnectTimer::RunL()
	{
	LOG_FUNC
	TInt completionCode(iStatus.Int());
	
	if(completionCode != KErrNone)
		{
		RDebug::Printf("<Error %d> software connect/disconnect timer error",completionCode);
		iTestDevice.ReportError(completionCode);
		}
	else
		{
		switch(iConnectType)
			{
			case EConnect:
				iTestDevice.SoftwareConnect();
				break;
				
			case EDisconnect:
				iTestDevice.SoftwareDisconnect();
				break;
				
			case EUnknown:
				RDebug::Printf("<Error> Unknown state for software connect timer");
				break;
			
			default:
				break;
			}			
		}	
	}


	}
	
