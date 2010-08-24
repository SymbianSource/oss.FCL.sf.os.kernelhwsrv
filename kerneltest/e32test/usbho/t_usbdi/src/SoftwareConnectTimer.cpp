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
// @file softwareconnecttimer.cpp
// @internalComponent
// 
//

#include "softwareconnecttimer.h"
#include "OstTraceDefinitions.h"
#ifdef OST_TRACE_COMPILER_IN_USE
#include "SoftwareConnectTimerTraces.h"
#endif

namespace NUnitTesting_USBDI
	{
	
const TInt KOneSecond(1000000);
	
CSoftwareConnectTimer* CSoftwareConnectTimer::NewL(RUsbTestDevice& aTestDevice)
	{
	OstTraceFunctionEntry1( CSOFTWARECONNECTTIMER_NEWL_ENTRY, ( TUint )&( aTestDevice ) );
	CSoftwareConnectTimer* self = new (ELeave) CSoftwareConnectTimer(aTestDevice);
	CleanupStack::PushL(self);
	self->ConstructL();
	CleanupStack::Pop(self);
	OstTraceFunctionExit1( CSOFTWARECONNECTTIMER_NEWL_EXIT, ( TUint )( self ) );
	return self;
	}
	
	
CSoftwareConnectTimer::CSoftwareConnectTimer(RUsbTestDevice& aTestDevice)
:	CTimer(EPriorityStandard),
	iTestDevice(aTestDevice),
	iConnectType(EUnknown)
	{
	OstTraceFunctionEntryExt( CSOFTWARECONNECTTIMER_CSOFTWARECONNECTTIMER_ENTRY, this );
	CActiveScheduler::Add(this);
	OstTraceFunctionExit1( CSOFTWARECONNECTTIMER_CSOFTWARECONNECTTIMER_EXIT, this );
	}
	
	
CSoftwareConnectTimer::~CSoftwareConnectTimer()
	{
	OstTraceFunctionEntry1( CSOFTWARECONNECTTIMER_CSOFTWARECONNECTTIMER_ENTRY_DUP01, this );
	OstTraceFunctionExit1( CSOFTWARECONNECTTIMER_CSOFTWARECONNECTTIMER_EXIT_DUP01, this );
	}
	
	
void CSoftwareConnectTimer::SoftwareConnect(TInt aInterval)
	{
	OstTraceFunctionEntryExt( CSOFTWARECONNECTTIMER_SOFTWARECONNECT_ENTRY, this );
	iConnectType = EConnect;
	After(aInterval*KOneSecond);
	OstTraceFunctionExit1( CSOFTWARECONNECTTIMER_SOFTWARECONNECT_EXIT, this );
	}
	

void CSoftwareConnectTimer::SoftwareDisconnect(TInt aInterval)
	{
	OstTraceFunctionEntryExt( CSOFTWARECONNECTTIMER_SOFTWAREDISCONNECT_ENTRY, this );
	iConnectType = EDisconnect;
	After(aInterval*KOneSecond);
	OstTraceFunctionExit1( CSOFTWARECONNECTTIMER_SOFTWAREDISCONNECT_EXIT, this );
	}
	
	
void CSoftwareConnectTimer::SoftwareReConnect(TInt aInterval)
	{
	OstTraceFunctionEntryExt( CSOFTWARECONNECTTIMER_SOFTWARERECONNECT_ENTRY, this );
	iTestDevice.SoftwareDisconnect();
	SoftwareConnect(aInterval);
	OstTraceFunctionExit1( CSOFTWARECONNECTTIMER_SOFTWARERECONNECT_EXIT, this );
	}


void CSoftwareConnectTimer::RunL()
	{
	OstTraceFunctionEntry1( CSOFTWARECONNECTTIMER_RUNL_ENTRY, this );
	TInt completionCode(iStatus.Int());
	
	if(completionCode != KErrNone)
		{
		OstTrace1(TRACE_NORMAL, CSOFTWARECONNECTTIMER_RUNL, "<Error %d> software connect/disconnect timer error",completionCode);
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
				OstTrace0(TRACE_NORMAL, CSOFTWARECONNECTTIMER_RUNL_DUP01, "<Error> Unknown state for software connect timer");
				break;
			
			default:
				break;
			}			
		}	
	OstTraceFunctionExit1( CSOFTWARECONNECTTIMER_RUNL_EXIT, this );
	}


	}
	
