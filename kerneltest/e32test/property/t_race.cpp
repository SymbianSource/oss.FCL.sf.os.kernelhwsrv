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

_LIT(KPropSetGetRaceName, "RProperty::Set/Get() Race");
 
CPropSetGetRace::CPropSetGetRace(TUid aCategory, TUint aKey) : 
	  CTestProgram(KPropSetGetRaceName), iCategory(aCategory), iKey(aKey), 
		  iBuf1(RProperty::KMaxPropertySize), iBuf2(RProperty::KMaxPropertySize)
	{
	}

void CPropSetGetRace::Trublemaker(TDes8& aBuf)
	{
	RProperty prop;	
	TInt r = prop.Attach(iCategory, iKey, EOwnerThread);
	TF_ERROR(r, r == KErrNone);
	TBool attached = ETrue;
	for(;;)
		{
		TBuf8<RProperty::KMaxPropertySize> buf;
		if (attached)
			{
			r = prop.Set(aBuf);
			TF_ERROR(r, r == KErrNone);
			r = prop.Get(buf);
			TF_ERROR(r, r == KErrNone);
			}
		else 
			{
			r = prop.Set(iCategory, iKey, aBuf);
			TF_ERROR(r, r == KErrNone);
			r = prop.Get(iCategory, iKey, buf);
			TF_ERROR(r, r == KErrNone);
			}
		TF_ERROR(KErrGeneral, !(buf.Compare(iBuf1) && buf.Compare(iBuf2)));
		User::After(1);
		attached = !attached;
		}
	}

TInt CPropSetGetRace::TrublemakerThreadEntry(TAny* ptr)
	{
	CPropSetGetRace* prog = (CPropSetGetRace*) ptr;
	prog->Trublemaker(prog->iBuf2);
	return KErrNone;
	}

void CPropSetGetRace::Run(TUint aCount)
	{
	RProperty prop;	

	TInt r = prop.Define(iCategory, iKey, RProperty::EByteArray, KPassPolicy, KPassPolicy);
	TF_ERROR(r, r == KErrNone);
	r = prop.Attach(iCategory,iKey);
	TF_ERROR(r, r == KErrNone);

	TUint i;
	for(i = 0; i < RProperty::KMaxPropertySize; ++i)
		{
		iBuf1[i] = '1';
		iBuf2[i] = '2';
		}

	TRequestStatus status;
	RThread thr;
	r = thr.Create(KNullDesC, TrublemakerThreadEntry, 0x2000, NULL, this);
	TF_ERROR(r, r == KErrNone);
	thr.Logon(status);
	thr.SetPriority(EPriorityMore);
	thr.Resume();

	TBool attached = ETrue;
	for (i = 0; i < aCount; ++i)
		{
		TBuf8<RProperty::KMaxPropertySize> buf;
		if (attached)
			{
			r = prop.Set(iBuf1);
			TF_ERROR(r, r == KErrNone);
			r = prop.Get(buf);
			TF_ERROR(r, r == KErrNone);
			}
		else 
			{
			r = prop.Set(iCategory, iKey, iBuf1);
			TF_ERROR(r, r == KErrNone);
			r = prop.Get(iCategory, iKey, buf);
			TF_ERROR(r, r == KErrNone);
			}
		TF_ERROR(KErrGeneral, !(buf.Compare(iBuf1) && buf.Compare(iBuf2)));
		attached = !attached;
		}

	thr.Kill(EExitKill);
	thr.Close();

	User::WaitForRequest(status);
	TF_ERROR(status.Int(), status.Int() == EExitKill);

	prop.Delete(iCategory, iKey);
	prop.Close();
	}


_LIT(KPropCancelRaceName, "RProperty::Cancel() Race");
 
CPropCancelRace::CPropCancelRace(TUid aCategory, TUint aKey) : 
	  CTestProgram(KPropCancelRaceName), iCategory(aCategory), iKey(aKey) 
	{
	}

void CPropCancelRace::Trublemaker()
	{
	for(;;)
		{
		iProp.Cancel();
		User::After(1);
		}
	}

TInt CPropCancelRace::TrublemakerThreadEntry(TAny* ptr)
	{
	CPropCancelRace* prog = (CPropCancelRace*) ptr;
	prog->Trublemaker();
	return KErrNone;
	}

void CPropCancelRace::Run(TUint aCount)
	{
	TInt r = iProp.Define(iCategory, iKey, RProperty::EInt, KPassPolicy, KPassPolicy);
	TF_ERROR(r, r == KErrNone);
	r = iProp.Attach(iCategory,iKey);
	TF_ERROR(r, r == KErrNone);

	TRequestStatus thrStatus;
	RThread thr;
	r = thr.Create(KNullDesC, TrublemakerThreadEntry, 0x2000, NULL, this);
	TF_ERROR(r, r == KErrNone);
	thr.Logon(thrStatus);
	thr.SetPriority(EPriorityMore);
	thr.Resume();

	for (TUint i = 0; i < aCount; ++i)
		{
		TRequestStatus status;
		iProp.Subscribe(status);
		TInt st = status.Int();
		TF_ERROR(st, (st == KRequestPending) || (st == KErrCancel));
		iProp.Set(1);
		User::WaitForRequest(status);
		st = status.Int();
		TF_ERROR(st, (st == KErrNone) || (st == KErrCancel));
		}

	thr.Kill(EExitKill);
	User::WaitForRequest(thrStatus);
	TF_ERROR(thrStatus.Int(), thrStatus.Int() == EExitKill);
	thr.Close();

	iProp.Delete(iCategory, iKey);
	iProp.Close();
	}
