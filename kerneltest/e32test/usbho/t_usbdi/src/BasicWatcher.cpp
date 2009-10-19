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
// @file basicwatcher.cpp
// @internalComponent
// 
//

#include "BasicWatcher.h"
#include "testdebug.h"

namespace NUnitTesting_USBDI
	{

CBasicWatcher::CBasicWatcher(const TCallBack& aCallBack,TInt aPriority)
:	CActive(aPriority),
	iCallBack(aCallBack),
	iCompletionCode(KErrNone)
	{
	CActiveScheduler::Add(this);
	}
	
CBasicWatcher::~CBasicWatcher()
	{
	LOG_FUNC

	Cancel();
	}

void CBasicWatcher::DoCancel()
	{
	LOG_FUNC

	RDebug::Printf("Watch cancelled");
	iStatus = KErrCancel;
	}


void CBasicWatcher::StartWatching()
	{
	LOG_FUNC

	if(iStatus != KRequestPending)
		{
		User::Panic(_L("iStatus has not been set to pending this will lead to E32USER-CBase Panic"),46);
		}
	SetActive();
	}


void CBasicWatcher::RunL()
	{
	LOG_FUNC

	iCompletionCode = iStatus.Int();
	User::LeaveIfError(iCallBack.CallBack());
	}


TInt CBasicWatcher::RunError(TInt aError)
	{
	LOG_FUNC

	RDebug::Printf("Watcher code Left with %d",aError);
	return KErrNone;
	}

	}
	
