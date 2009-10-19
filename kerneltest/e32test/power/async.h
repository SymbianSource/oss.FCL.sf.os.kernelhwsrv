// Copyright (c) 1998-2009 Nokia Corporation and/or its subsidiary(-ies).
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
// e32test\power\async.h
// 
//

class RAsyncSwitchOff : public RThread
	{
public:
	TInt Start(TTimeIntervalMicroSeconds32 aTime);
	TInt Wait();
	static TInt Thread(TAny* aPtr);
public:
	TTimeIntervalMicroSeconds32 iTime;
	TRequestStatus iStatus;
	};

_LIT(KAsyncSwitchOffThreadName, "AsyncSwitchOffThread");

TInt RAsyncSwitchOff::Thread(TAny* aPtr)
	{
	RAsyncSwitchOff& a=*(RAsyncSwitchOff*)aPtr;
	User::After(a.iTime);
	UserHal::SwitchOff();
	return KErrNone;
	}

TInt RAsyncSwitchOff::Start(TTimeIntervalMicroSeconds32 aTime)
	{
	iTime=aTime;
	TInt r=Create(KAsyncSwitchOffThreadName, Thread, 0x1000, NULL, this);
	if (r!=KErrNone)
		return r;
	SetPriority(EPriorityMuchMore);
	Logon(iStatus);
	Resume();
	return KErrNone;
	}

TInt RAsyncSwitchOff::Wait()
	{
	User::WaitForRequest(iStatus);
	TExitType exitType=ExitType();
	CLOSE_AND_WAIT(*this);
	if (exitType!=EExitKill)
		User::Panic(_L("AsyncSwitchOff"), iStatus.Int());
	return iStatus.Int();
	}
