// Copyright (c) 2002-2009 Nokia Corporation and/or its subsidiary(-ies).
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
//

#include "t_property.h"

class CPropBroadcastSlave : public CBase
	{
public:
	static TInt LoopThreadEntry(TAny*);

	CPropBroadcastSlave(CPropBroadcast* aProg, TUid aCategory, TUint aMasterKey, TUint aSlaveKey);
	void Create(TThreadPriority);
	void Delete();
	void Loop();

	CPropBroadcast*	iProg;
	TUid			iCategory;
	TUint			iMasterKey;
	TUint			iSlaveKey;

	RProperty				iProp;
	RThread					iThread;
	TRequestStatus			iStatus;

	CPropBroadcastSlave*	iNext;
	};



CPropBroadcastSlave::CPropBroadcastSlave(CPropBroadcast* aProg, TUid aCategory, TUint aMasterKey, TUint aSlaveKey) : 
		iProg(aProg), iCategory(aCategory), iMasterKey(aMasterKey), iSlaveKey(aSlaveKey)
	{
	}

void CPropBroadcastSlave::Create(TThreadPriority aPrio)
	{
	TInt r = iProp.Define(iCategory, iSlaveKey, RProperty::EInt, KPassPolicy, KPassPolicy);
	TF_ERROR_PROG(iProg, r, r == KErrNone);
	r = iProp.Attach(iCategory, iSlaveKey);
	TF_ERROR_PROG(iProg, r, r == KErrNone);

	r = iThread.Create(KNullDesC, LoopThreadEntry, 0x2000, NULL, this);
	TF_ERROR_PROG(iProg, r, r == KErrNone);
	iThread.Logon(iStatus);
	iThread.SetPriority(aPrio);
	iThread.Resume();
	}

void CPropBroadcastSlave::Delete()
	{
	iThread.Kill(EExitKill);
	iThread.Close();
	User::WaitForRequest(iStatus);
	TF_ERROR_PROG(iProg, iStatus.Int(), iStatus.Int() == EExitKill);
	
	TInt r = iProp.Delete(iCategory, iSlaveKey);
	TF_ERROR_PROG(iProg, r, r == KErrNone);
	iProp.Close();

	delete this;
	}

TInt CPropBroadcastSlave::LoopThreadEntry(TAny* ptr)
	{
	CPropBroadcastSlave* self = (CPropBroadcastSlave*) ptr;
	self->Loop();
	return KErrNone;
	}

void CPropBroadcastSlave::Loop()
	{
	RProperty mProp;
	TInt r = mProp.Attach(iCategory, iMasterKey, EOwnerThread);
	TF_ERROR_PROG(iProg, r, r == KErrNone);
	
	RProperty sProp;
	r = sProp.Attach(iCategory, iSlaveKey, EOwnerThread);
	TF_ERROR_PROG(iProg, r, r == KErrNone);

	TInt value = 1;
	for(;;)
		{
		TRequestStatus status;
		mProp.Subscribe(status);

		r = sProp.Set(value);
		TF_ERROR_PROG(iProg, r, r == KErrNone);

		User::WaitForRequest(status);

		r = mProp.Get(value);
		TF_ERROR_PROG(iProg, r, r == KErrNone);
		}
	}

_LIT(KPropBroadcastName, "RProperty Broadcast");
 
CPropBroadcast::CPropBroadcast(TUid aCategory, TUint aMasterKey, 
								TUint aSlaveKeySlot, TUint aSlaveCount, TUint aFirstHighPriority) : 
	  CTestProgram(KPropBroadcastName), iCategory(aCategory), iMasterKey(aMasterKey), 
		  iSlaveKeySlot(aSlaveKeySlot), iSlaveCount(aSlaveCount), iFirstHighPriority(aFirstHighPriority)
	{
	}


void CPropBroadcast::Run(TUint aCount)
	{
	RProperty mProp;
	TInt r = mProp.Define(iCategory, iMasterKey, RProperty::EInt, KPassPolicy, KPassPolicy);
	TF_ERROR(r, r == KErrNone);
	r = mProp.Attach(iCategory, iMasterKey);
	TF_ERROR(r, r == KErrNone);

	for (TUint j = 0; j < iSlaveCount; ++j)
		{
		CPropBroadcastSlave* slave = new CPropBroadcastSlave(this, iCategory, iMasterKey, iSlaveKeySlot + j);
		slave->Create((j < iFirstHighPriority) ? EPriorityLess : EPriorityMore);

		slave->iNext = iSlaveList;
		iSlaveList = slave;
		}

	for (TUint i = 1; i < aCount; ++i)
		{
		CPropBroadcastSlave* slave = iSlaveList;
		while (slave)
			{
			TRequestStatus status;
			slave->iProp.Subscribe(status);
			TInt value;
			r = slave->iProp.Get(value);
			TF_ERROR(r, r == KErrNone);
			if (value == (TInt) i)
				{
				slave->iProp.Cancel();
				}
			User::WaitForRequest(status);
			TF_ERROR(status.Int(), (status.Int() == KErrCancel) || (status.Int() == KErrNone));
			r = slave->iProp.Get(value);
			TF_ERROR(r, r == KErrNone);
			TF_ERROR(value, value == (TInt) i);
			slave = slave->iNext;
			}
		mProp.Set(i + 1);
		}

	for (;;)
		{
		CPropBroadcastSlave* slave = iSlaveList;
		if (slave == NULL) break;
		iSlaveList = slave->iNext;
		slave->Delete();
		}

	mProp.Delete(iCategory, iMasterKey);
	mProp.Close();
	}
