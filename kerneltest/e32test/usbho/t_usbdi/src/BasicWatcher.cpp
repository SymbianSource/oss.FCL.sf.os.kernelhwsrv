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
// @file basicwatcher.cpp
// @internalComponent
// 
//

#include "BasicWatcher.h"
#include "testdebug.h"
#include "OstTraceDefinitions.h"
#ifdef OST_TRACE_COMPILER_IN_USE
#include "BasicWatcherTraces.h"
#endif

namespace NUnitTesting_USBDI
	{

CBasicWatcher::CBasicWatcher(const TCallBack& aCallBack,TInt aPriority)
:	CActive(aPriority),
	iCallBack(aCallBack),
	iCompletionCode(KErrNone)
	{
	OstTraceFunctionEntryExt( CBASICWATCHER_CBASICWATCHER_ENTRY, this );
	CActiveScheduler::Add(this);
	OstTraceFunctionExit1( CBASICWATCHER_CBASICWATCHER_EXIT, this );
	}
	
CBasicWatcher::~CBasicWatcher()
	{
    OstTraceFunctionEntry1( CBASICWATCHER_CBASICWATCHER_ENTRY_DUP01, this );

	Cancel();
	OstTraceFunctionExit1( CBASICWATCHER_CBASICWATCHER_EXIT_DUP01, this );
	}

void CBasicWatcher::DoCancel()
	{
    OstTraceFunctionEntry1( CBASICWATCHER_DOCANCEL_ENTRY, this );

	OstTrace0(TRACE_NORMAL, CBASICWATCHER_DOCANCEL, "Watch cancelled");
	iStatus = KErrCancel;
	OstTraceFunctionExit1( CBASICWATCHER_DOCANCEL_EXIT, this );
	}


void CBasicWatcher::StartWatching()
	{
    OstTraceFunctionEntry1( CBASICWATCHER_STARTWATCHING_ENTRY, this );

	if(iStatus != KRequestPending)
		{
		User::Panic(_L("iStatus has not been set to pending this will lead to E32USER-CBase Panic"),46);
		}
	SetActive();
	OstTraceFunctionExit1( CBASICWATCHER_STARTWATCHING_EXIT, this );
	}


void CBasicWatcher::RunL()
	{
    OstTraceFunctionEntry1( CBASICWATCHER_RUNL_ENTRY, this );

	iCompletionCode = iStatus.Int();
	User::LeaveIfError(iCallBack.CallBack());
	OstTraceFunctionExit1( CBASICWATCHER_RUNL_EXIT, this );
	}


TInt CBasicWatcher::RunError(TInt aError)
	{
    OstTraceFunctionEntryExt( CBASICWATCHER_RUNERROR_ENTRY, this );

	OstTrace1(TRACE_NORMAL, CBASICWATCHER_RUNERROR, "Watcher code Left with %d",aError);
	OstTraceFunctionExitExt( CBASICWATCHER_RUNERROR_EXIT, this, KErrNone );
	return KErrNone;
	}


CInterfaceWatcher::CInterfaceWatcher(RUsbInterface& aInterface,const TCallBack& aCallBack)
:	CActive(EPriorityUserInput),
	iUsbInterface(aInterface),
	iResumeCallBack(aCallBack),
	iCompletionCode(KErrNone)
	{
	CActiveScheduler::Add(this);
	}

CInterfaceWatcher::~CInterfaceWatcher()
	{
	Cancel();
	}

void CInterfaceWatcher::SuspendAndWatch()
	{
	iUsbInterface.PermitSuspendAndWaitForResume(iStatus);
	SetActive();
	}

TInt CInterfaceWatcher::CompletionCode() const
	{
	return iCompletionCode;
	}

void CInterfaceWatcher::DoCancel()
	{
	iUsbInterface.CancelPermitSuspend();
	}

void CInterfaceWatcher::RunL()
	{
	iCompletionCode = iStatus.Int();
	User::LeaveIfError(iResumeCallBack.CallBack());
	}
	
TInt CInterfaceWatcher::RunError()
	{
	return KErrNone;
	}		
									
	}
	
