// Copyright (c) 1996-2009 Nokia Corporation and/or its subsidiary(-ies).
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
// e32\kernel\notifier.cpp
// DChangeNotifier & DUndertaker code
// Note that both change notifier and undertaker handles can be thread or process relative
// 
//

#include <kernel/kern_priv.h>
#include "execs.h"

#define iMState		iWaitLink.iSpare1

DChangeNotifier::DChangeNotifier()
//
// Constructor
//
	{

	/**************************************************************************
	**   CHANGE THE NEXT LINE IF YOU ADD A NEW ECHANGES... TO THE NOTIFIER   **
	**   But not for EChangesLowMemory!										 **
	**************************************************************************/
	iChanges=EChangesLocale|EChangesMidnightCrossover|EChangesThreadDeath|EChangesPowerStatus|EChangesSystemTime|EChangesFreeMemory|EChangesOutOfMemory|EChangesThrashLevel;
	//iRequest=NULL;
	//iThread=NULL;
	}

TInt DChangeNotifier::Create()
	{
	return Kern::CreateClientRequest(iRequest);
	}

DChangeNotifier::~DChangeNotifier()
//
// Destructor
// Enter and leave with system unlocked.
//
	{
	NKern::LockSystem();
	if (iThread)
		Complete(KErrGeneral);
	else
		NKern::UnlockSystem();
	Kern::DestroyClientRequest(iRequest);
	}


void DChangeNotifier::Complete(TInt aResult)
//
// Complete the client the result code specified
// Enter with system locked, leave with system unlocked.
//
	{
	DThread* t = iThread;
	iChanges = 0;
	iThread = NULL;
	NKern::ThreadEnterCS();
	Kern::QueueRequestComplete(t, iRequest,aResult);
	NKern::UnlockSystem();
	t->Close(NULL);
	NKern::ThreadLeaveCS();
	}


void DChangeNotifier::Notify(TUint aChanges)
//
// Complete logged on status, or save up change.
// Enter with system locked, leave with system unlocked.
//
	{
	__KTRACE_OPT(KEXEC,Kern::Printf("DChangeNotifier::Notify %08x %x",this,aChanges));
	iChanges|=aChanges;
	if (iRequest->IsReady())
		Complete(iChanges);
	else
		NKern::UnlockSystem();
	}


TInt DChangeNotifier::Logon(TRequestStatus& aStatus, DThread* aThread)
//
// Logon to notifier.
// Enter with system locked, return with system unlocked.
//
	{
	if (iRequest->SetStatus(&aStatus) != KErrNone)
		return KErrInUse;
	iThread=aThread;
	iThread->CheckedOpen();
	if (iChanges!=0)
		Complete(iChanges);
	else
		NKern::UnlockSystem();
	return KErrNone;
	}

TInt DChangeNotifier::LogonCancel(DThread* aThread)
//
// Logoff from notifier.
// Enter with system locked, return with system unlocked.
//
	{
	if (iRequest->IsReady() && iThread==aThread)
		{
		Complete(KErrCancel);
		return KErrNone;
		}
	NKern::UnlockSystem();
	return KErrGeneral;
	}

TInt ExecHandler::ChangeNotifierCreate(TOwnerType aType)
//
// Create a change notifier
//
	{
	__KTRACE_OPT(KEXEC,Kern::Printf("Exec::ChangeNotifierCreate"));
	NKern::ThreadEnterCS();
	DChangeNotifier* pN=new DChangeNotifier;
	TInt r=KErrNoMemory;
	if (pN)
		{
		r=pN->Create();
		if (r==KErrNone)
			r=K::AddObject(pN,EChangeNotifier);
		if (r==KErrNone)
			r=K::MakeHandle(aType,pN);
		if (r<KErrNone)
			pN->Close(NULL);
		}
	NKern::ThreadLeaveCS();
	return r;
	}

TInt ExecHandler::ChangeNotifierLogon(DChangeNotifier* aChangeNotifier, TRequestStatus& aStatus)
//
// Enter with system locked, return with system unlocked.
//
	{
	__KTRACE_OPT(KEXEC,Kern::Printf("Exec::ChangeNotifierLogon"));
	return aChangeNotifier->Logon(aStatus, TheCurrentThread);
	}

TInt ExecHandler::ChangeNotifierLogoff(DChangeNotifier* aChangeNotifier)
//
// Enter with system locked, return with system unlocked.
//
	{
	__KTRACE_OPT(KEXEC,Kern::Printf("Exec::ChangeNotifierLogoff"));
	return aChangeNotifier->LogonCancel(TheCurrentThread);
	}


/** Synchronously notifies all change notifiers of a set of events.

	This call causes all DChangeNotifier objects to record the events indicated
	and to signal their owning thread if it is currently logged on.

	@param	aChanges	The mask of events to be notified (TChanges enumeration)

	@pre    No fast mutex can be held
	@pre    Calling thread must be in a critical section
	@pre	Call in a thread context
	@pre	Kernel must be unlocked
	@pre	interrupts enabled
	
	@see Kern::AsyncNotifyChanges()
	@see TChanges
 */
EXPORT_C void Kern::NotifyChanges(TUint aChanges)
	{
	CHECK_PRECONDITIONS(MASK_THREAD_CRITICAL,"Kern::NotifyChanges");		
	__KTRACE_OPT(KTHREAD,Kern::Printf("Kern::NotifyChanges %x",aChanges));
	DObjectCon& chnot=*K::Containers[EChangeNotifier];
	chnot.Wait();
	TInt c=chnot.Count();
	TInt i;
	for(i=0; i<c; i++)
		{
		DChangeNotifier* pN=(DChangeNotifier*)chnot[i];
		if (pN->Open() == KErrNone)
			{
			NKern::LockSystem();
			pN->Notify(aChanges);
			pN->AsyncClose();
			}
		}
	chnot.Signal();
	}

//
// Thread Watchers
//

TInt DUndertaker::Create()
	{
	return Kern::CreateClientDataRequest(iRequest);
	}

DUndertaker::~DUndertaker()
//
// Destructor
// Enter and return with system unlocked
//
	{
	if (iRequest)
		{
		NKern::LockSystem();
		Complete(KErrGeneral,NULL);
		Kern::DestroyClientRequest(iRequest);
		}
	}

void DUndertaker::Complete(TInt aValue, DThread* aDeadThread)
//
// Complete and delete from queue
// Enter with system locked and, if aDeadThread!=NULL, calling thread in critical section
// Return with system unlocked.
// Must be careful to ensure only one completion can occur
//
	{
	if (!iRequest->IsReady())
		{
		NKern::UnlockSystem();
		return;
		}	
	if (iOwningThread->iMState==DThread::EDead)
		{
		iOwningThread=NULL;
		iRequest->Reset();
		NKern::UnlockSystem();
		return;
		}
	iRequest->Data()=0;
	if (aDeadThread)
		{
		NKern::UnlockSystem();
		// At this point, the undertaker could be logged on again here by another thread in the same
		// process, but this will result in KErrInUse, as that thread can't know that the undertaker
		// has been completed
		iOwningThread->MakeHandleAndOpen(EOwnerThread,aDeadThread,iRequest->Data());
		NKern::LockSystem();
		}
	DThread* t = iOwningThread;
	iOwningThread = NULL;
	Kern::QueueRequestComplete(t, iRequest,aValue);
	NKern::UnlockSystem();
	}

void DUndertaker::Notify(DThread* aDeadThread)
//
// Complete the watcher if someone's logged on to it
// Enter and return with system unlocked
//
	{
	__KTRACE_OPT(KEXEC,Kern::Printf("DUndertaker::Notify %08x",this));
	NKern::LockSystem();
	Complete(KErrDied,aDeadThread);
	}

TInt DUndertaker::Logon(TRequestStatus* aStatus, TInt* aDeadThreadHandle)
//
// Logon to thread watcher
// Enter and return with system locked
//
	{
	if (iRequest->SetStatus(aStatus) != KErrNone)
		return KErrInUse;
	iRequest->SetDestPtr(aDeadThreadHandle);
	iOwningThread=TheCurrentThread;
	return KErrNone;
	}

TInt DUndertaker::LogonCancel()
//
// Logoff from thread watcher
// Enter with system locked, return with system unlocked.
//
	{
	if (iOwningThread==TheCurrentThread)
		{
		Complete(KErrCancel,NULL);
		return KErrNone;
		}
	NKern::UnlockSystem();
	return KErrGeneral;
	}

TInt ExecHandler::UndertakerCreate(TOwnerType aType)
//
// Create a thread watcher
//
	{
	__KTRACE_OPT(KEXEC,Kern::Printf("Exec::UndertakerCreate"));
	NKern::ThreadEnterCS();
	DUndertaker* pU=new DUndertaker;
	TInt r=KErrNoMemory;
	if (pU)
		{
		r=pU->Create();
		if (r==KErrNone)
			r=K::AddObject(pU,EUndertaker);
		if (r==KErrNone)
			r=K::MakeHandle(aType,pU);
		if (r<KErrNone)
			pU->Close(NULL);
		}
	NKern::ThreadLeaveCS();
	return r;
	}

TInt ExecHandler::UndertakerLogon(DUndertaker *anUndertaker, TRequestStatus& aStatus, TInt* aDeadThreadHandle)
//
// Enter and return with system locked.
//
	{
	__KTRACE_OPT(KEXEC,Kern::Printf("Exec::UndertakerLogon"));
	return anUndertaker->Logon(&aStatus,aDeadThreadHandle);
	}

TInt ExecHandler::UndertakerLogonCancel(DUndertaker* anUndertaker)
//
// Enter with system locked, return with system unlocked.
//
	{
	__KTRACE_OPT(KEXEC,Kern::Printf("Exec::UndertakerLogonCancel"));
	return anUndertaker->LogonCancel();
	}


/** Notifies all undertakers of a thread death.

	This call causes all DUndertaker objects to signal their owning thread
	if it is currently logged on. A handle to the dead thread will be created
	for each owning thread signalled.

	@param	aDeadThread Pointer to the deceased thread.
	
	@pre    No fast mutex can be held.
    @pre    Calling thread must be in a critical section.
	@pre	Call in a thread context.
	@pre	Kernel must be unlocked
	@pre	interrupts enabled

	@internalTechnology
 */
EXPORT_C void Kern::NotifyThreadDeath(DThread* aDeadThread)
	{
	CHECK_PRECONDITIONS(MASK_THREAD_CRITICAL,"Kern::NotifyThreadDeath");		
	__KTRACE_OPT(KTHREAD,Kern::Printf("Kern::NotifyThreadDeath %O",aDeadThread));
	DObjectCon& und=*K::Containers[EUndertaker];
	RObjectIx::Wait();
	und.Wait();
	TInt c=und.Count();
	TInt i;
	for (i=0; i<c; i++)
		{
		DUndertaker* pU=(DUndertaker*)und[i];
		if (pU->Open() == KErrNone)
			{
			pU->Notify(aDeadThread);
			pU->AsyncClose();
			}
		}
	und.Signal();
	RObjectIx::Signal();
	}



