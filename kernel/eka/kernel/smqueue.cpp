// Copyright (c) 2008-2009 Nokia Corporation and/or its subsidiary(-ies).
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
// e32\kernel\smqueue.cpp
// 
//

#include <kernel/kern_priv.h>
#include "execs.h"
#include "msgqueue.h"

/********************************************
 * Asynchronous message queues
 ********************************************/

TInt ExecHandler::MsgQueueCreate(const TDesC8* aName, TInt aSize, TInt aLength, TOwnerType aType)
	{

	__KTRACE_OPT(KEXEC,Kern::Printf("Exec::MsgQueueCreate")); 	
	
	//validate params
	//length must be multiple of 4, greater than 0 and < kmaxlength
	if ((aLength & 3) || (aLength > DMsgQueue::KMaxLength) || (aLength < 4))
		K::PanicKernExec(EMsgQueueInvalidLength);

	//size is number of message slots in the queue, it must be > 0
	if (aSize <= 0)
		K::PanicKernExec(EMsgQueueInvalidSlots);

	TKName name;
	DObject* pOwner = NULL;
	const TDesC* pName = NULL;
	if (aName)
		{
		Kern::KUDesGet(name, *aName);
		pName = &name;
		}
	else if (aType == EOwnerThread)
		pOwner = TheCurrentThread;
	else
		pOwner = TheCurrentThread->iOwningProcess;


	NKern::ThreadEnterCS();
	
	TInt ret = KErrNoMemory;
	DMsgQueue* pMQ=new DMsgQueue;
	if (pMQ)
		{
		ret = pMQ->Create(pOwner, pName, aLength, aSize);

		if (KErrNone == ret)
			{
			if(aName)
				pMQ->SetProtection(name.Length() ? DObject::EGlobal : DObject::EProtected);
			ret = K::MakeHandle(aType, pMQ);
			}
		if (ret < KErrNone)
			pMQ->Close(NULL);

		}
	NKern::ThreadLeaveCS();
	return ret;
	}

TInt ExecHandler::MsgQueueSend(TInt aMsgQueueHandle, const TAny* aPtr, TInt aLength)
	{
	__KTRACE_OPT(KEXEC,Kern::Printf("Exec::MsgQueueSend")); 	

	TUint8 sbuf[DMsgQueue::KMaxLength];
	if(TUint(aLength)<=TUint(DMsgQueue::KMaxLength))
		kumemget(sbuf,aPtr,aLength);

	NKern::LockSystem();
	DMsgQueue* msgQueue = (DMsgQueue*)K::ObjectFromHandle(aMsgQueueHandle,EMsgQueue);
	return msgQueue->Send(sbuf, aLength);
	}


TInt ExecHandler::MsgQueueReceive(DMsgQueue* aMsgQueue,  TAny* aPtr, TInt aLength)
	{
	__KTRACE_OPT(KEXEC,Kern::Printf("Exec::MsgQueueReceive")); 	
	TUint8 sbuf[DMsgQueue::KMaxLength];
	TInt r = aMsgQueue->Receive(sbuf, aLength);
	if(r==KErrNone)
		kumemput(aPtr,sbuf,aLength);
	return r;
	}


void ExecHandler::MsgQueueNotifySpaceAvailable(DMsgQueue* aMsgQueue, TRequestStatus& aStatus)
	{
	__KTRACE_OPT(KEXEC,Kern::Printf("Exec::MsgQueueNotifySpaceAvailable")); 	
	aMsgQueue->NotifySpaceAvailable(&aStatus);
	}

void ExecHandler::MsgQueueCancelSpaceAvailable(DMsgQueue* aMsgQueue)
	{
	__KTRACE_OPT(KEXEC,Kern::Printf("Exec::MsgQueueCancelSpaceAvailable")); 	
	aMsgQueue->CancelSpaceAvailable();
	}

void ExecHandler::MsgQueueNotifyDataAvailable(DMsgQueue* aMsgQueue, TRequestStatus& aStatus)
	{
	__KTRACE_OPT(KEXEC,Kern::Printf("Exec::MsgQueueNotifyDataAvailable")); 	
	aMsgQueue->NotifyDataAvailable(&aStatus);
	}

void ExecHandler::MsgQueueCancelDataAvailable(DMsgQueue* aMsgQueue)
	{
	__KTRACE_OPT(KEXEC,Kern::Printf("Exec::MsgQueueCancelDataAvailable")); 	
	aMsgQueue->CancelDataAvailable();
	}

TInt ExecHandler::MsgQueueSize(DMsgQueue* aMsgQueue)
	{
	__KTRACE_OPT(KEXEC,Kern::Printf("Exec::MsgQueueSize")); 	
	return aMsgQueue->MessageSize();
	}


TInt DMsgQueue::Create(DObject* aOwner, const TDesC* aName, TInt aMsgLength, TInt aSlotCount, TBool aVisible)
// Enter and leave with system unlocked
	{

	SetOwner(aOwner);
	TInt ret = KErrNone;
	if (aName && (aName->Length() > 0))
		{
		ret = SetName(aName);
		if (ret != KErrNone)
			return ret;
		}

	ret = Kern::CreateClientRequest(iDataAvailRequest);
	if (ret != KErrNone)
		return ret;

	ret = Kern::CreateClientRequest(iSpaceAvailRequest);
	if (ret != KErrNone)
		return ret;

	//Kern::Alloc asserts if the size is > KMaxTint/2 so guard against this
	if (aSlotCount > (KMaxTInt/2) / aMsgLength)
		return KErrNoMemory;

	iMsgPool = static_cast<TUint8*>(Kern::Alloc(aSlotCount * aMsgLength));

	if (!iMsgPool)	
		return KErrNoMemory;

	iMaxMsgLength = (TUint16)aMsgLength;
	iFirstFreeSlot = iMsgPool;
	iFirstFullSlot = iMsgPool;
	iEndOfPool = iMsgPool + iMaxMsgLength * aSlotCount;

	if (aVisible)
		ret = K::AddObject(this,EMsgQueue);

	return ret;
	}

DMsgQueue::~DMsgQueue()
	{
	//no problem with race condition here, don't need temporary copy of thread ptrs
	if (iDataAvailRequest->IsReady())
		{
		Kern::QueueRequestComplete(iThreadWaitingOnDataAvail, iDataAvailRequest, KErrCancel);
		iThreadWaitingOnDataAvail->Close(NULL);
		}
	if (iSpaceAvailRequest->IsReady())
		{
		Kern::QueueRequestComplete(iThreadWaitingOnSpaceAvail, iSpaceAvailRequest, KErrCancel);
		iThreadWaitingOnSpaceAvail->Close(NULL);
		}
	Kern::Free(iMsgPool);
	Kern::DestroyClientRequest(iDataAvailRequest);
	Kern::DestroyClientRequest(iSpaceAvailRequest);
	}


TInt DMsgQueue::Send(const TAny* aPtr, TInt aLength)
// Enter with system locked, leave with system unlocked
	{
	if (aLength > iMaxMsgLength || aLength <= 0)
		K::PanicCurrentThread(EMsgQueueInvalidLength);

	if (iState == EFull)
		{
		NKern::UnlockSystem();
		return KErrOverflow;
		}

	memcpy(iFirstFreeSlot, aPtr, aLength);

	iFirstFreeSlot += iMaxMsgLength;
	if (iFirstFreeSlot == iEndOfPool)
		iFirstFreeSlot = iMsgPool;

	iState = static_cast<TUint8>((iFirstFreeSlot == iFirstFullSlot) ? EFull : EPartial);

	//see if anyone is waiting on data available
	CompleteRequestIfPending(iThreadWaitingOnDataAvail, iDataAvailRequest, KErrNone);

	return KErrNone;
	}



TInt DMsgQueue::Receive(TAny* aPtr, TInt aLength)
// Enter with system locked, leave with system unlocked
	{
	if (aLength != iMaxMsgLength)
		K::PanicCurrentThread(EMsgQueueInvalidLength);

	if (iState == EEmpty)
		{
		NKern::UnlockSystem();
		return KErrUnderflow;
		}

	memcpy(aPtr, iFirstFullSlot, iMaxMsgLength);

	iFirstFullSlot += iMaxMsgLength;
	if (iFirstFullSlot == iEndOfPool)
		iFirstFullSlot = iMsgPool;

	iState = static_cast<TUint8>((iFirstFreeSlot == iFirstFullSlot) ? EEmpty : EPartial);

	//see if anyone is waiting on space available
	CompleteRequestIfPending(iThreadWaitingOnSpaceAvail, iSpaceAvailRequest, KErrNone);

	return KErrNone;
	}


void DMsgQueue::NotifySpaceAvailable(TRequestStatus* aStatus)
// Enter with system locked, leave with system unlocked
	{
	RequestNotification(ESpaceAvailable, aStatus, iThreadWaitingOnSpaceAvail, iSpaceAvailRequest);
	}

void DMsgQueue::NotifyDataAvailable(TRequestStatus* aStatus)
// Enter with system locked, leave with system unlocked
	{
	RequestNotification(EDataAvailable, aStatus, iThreadWaitingOnDataAvail, iDataAvailRequest);
	}

void DMsgQueue::RequestNotification(TNotification aNotification, TRequestStatus* aStatus, DThread*& aThread, TClientRequest* aRequest)
// Enter with system locked, leave with system unlocked
	{
	TInt r = KErrNone;
	DThread* previousThread = NULL;
	if (aRequest->IsReady())
		{
		//someone already waiting...
		if (aThread->iExitType == EExitPending) 
			K::PanicCurrentThread(EMsgQueueRequestPending); //...and is still alive. Panic the current thread.
		//The thread that was previously waitning on available data has died.
		//As iThreadWaitingOnDataAvail is not cleaned up automatically when the waiting
		//thread dies, we have to do it here.
		previousThread = aThread;
		NKern::ThreadEnterCS();
		aThread = NULL;
		aRequest->Reset();
		}
	if ((aNotification == ESpaceAvailable && iState != EFull) ||
		(aNotification == EDataAvailable  && iState != EEmpty))
		Kern::RequestComplete(aStatus, KErrNone);
	else
		{
		DThread* thread = TheCurrentThread;
		thread->CheckedOpen();
		aThread = thread;
		r = aRequest->SetStatus(aStatus);
		}
	NKern::UnlockSystem();

	if (previousThread)
		{
		//Complete the clean up of the dying thread which was previously waiting on data-available event.
		previousThread->AsyncClose();
		NKern::ThreadLeaveCS();
		}
	
	if (r != KErrNone)
		K::PanicCurrentThread(EMsgQueueRequestPending);
	}
 

void DMsgQueue::CancelSpaceAvailable()
// Enter with system locked, leave with system unlocked
	{
	CompleteRequestIfPending(iThreadWaitingOnSpaceAvail, iSpaceAvailRequest, KErrCancel);
	}

void DMsgQueue::CancelDataAvailable()
// Enter with system locked, leave with system unlocked
	{
	CompleteRequestIfPending(iThreadWaitingOnDataAvail, iDataAvailRequest, KErrCancel);
	}

void DMsgQueue::CompleteRequestIfPending(DThread*& aThread, TClientRequest* aRequest, TInt aCompletionVal)
// Enter with system locked, leave with system unlocked
	{
	DThread* thread = aThread;
	aThread = NULL;
	if (aRequest->IsReady())
		{
		TInt c = thread->Dec();
		if (c==1)
			{
			// todo: why do we need to do this?
			aRequest->Reset();
			NKern::ThreadEnterCS();
			NKern::UnlockSystem();
			thread->AsyncDelete();
			NKern::ThreadLeaveCS();
			return;
			}
		Kern::QueueRequestComplete(thread, aRequest, aCompletionVal);
		}
	NKern::UnlockSystem();
	}

TInt DMsgQueue::MessageSize() const
	{
// Enter with system locked, leave with system locked
	return iMaxMsgLength;
	}
