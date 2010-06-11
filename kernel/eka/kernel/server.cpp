// Copyright (c) 1994-2009 Nokia Corporation and/or its subsidiary(-ies).
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
// e32\kernel\server.cpp
// 
//

#include <kernel/kern_priv.h>
#include "execs.h"


/* Macro for checking whether function is called from Kern::DfcQue0 or Kern::DfcQue1 thread context.
   Device drivers which access user memory should not run in shared DFC threads.
   See Base_How_To_Migrate_Media_Drivers_To_Support_Demand Paging.doc
*/

#if (!defined _DEBUG || ((!defined _CHECK_DFCQ_CONTEXT_WARNING_) && (!defined _CHECK_DFCQ_CONTEXT_PANIC_)))
#define _CHECK_DFCQ01_CONTEXT(function)
#else
#ifdef _CHECK_DFCQ_CONTEXT_PANIC_
#define _CHECK_DFCQ01_CONTEXT(func ) \
	{ \
	NThread* nt=NKern::CurrentThread(); \
	__ASSERT_DEBUG( (nt!=Kern::DfcQue0()->iThread && nt!=Kern::DfcQue1()->iThread), (\
	KPrintf("Function: %s\n called from DfcQ0 or DfcQ1 thread context",func), \
	NKFault(func, 0))); \
	}
#else //WARRNING
#define _CHECK_DFCQ01_CONTEXT(func ) \
	{ \
	NThread* nt=NKern::CurrentThread(); \
	__ASSERT_DEBUG( (nt!=Kern::DfcQue0()->iThread && nt!=Kern::DfcQue1()->iThread), (\
	KPrintf("Function: %s\n called from DfcQ0 or DfcQ1 thread context",func))); \
	}
#endif //  _CHECK_DFCQ_CONTEXT_PANIC_
#endif // !defined _DEBUG || ((!defined _CHECK_DFCQ_CONTEXT_WARNING_) && (!defined _CHECK_DFCQ_CONTEXT_PANIC_)))

/********************************************
 * Generic kernel message code
 ********************************************/


/**	Sends a kernel message without waiting for a reply.

	If the receiving queue is ready to receive its DFC will be queued; otherwise
	the message is simply placed on the end of the queue.

	@param	aQ	Message queue to which message should be sent.
	
	@pre    No fast mutex can be held.
	@pre	Call in a thread context.
	@pre	Kernel must be unlocked
	@pre	interrupts enabled
	
	@post	The message contains a reference counted pointer to the current thread.
 */
EXPORT_C void TMessageBase::Send(TMessageQue* aQ)
	{
	CHECK_PRECONDITIONS(MASK_THREAD_STANDARD,"TMessageBase::Send");			
	__KTRACE_OPT(KSERVER,Kern::Printf("MsgB::Send %08x to %08x",this,aQ));
	DThread* pC=TheCurrentThread;
	TMessageQue::Lock();
	__NK_ASSERT_ALWAYS(iState==EFree);
	iQueue=aQ;
	iSem.iCount=0;
	iSem.iOwningThread=&pC->iNThread;
	pC->Open();
	if (aQ->iReady)
		{
		iState=EAccepted;
		aQ->iMessage=this;
		aQ->iReady=EFalse;
		aQ->UnlockAndKick();
		}
	else
		{
		aQ->iQ.Add(this);
		iState=EDelivered;
		TMessageQue::Unlock();
		}
	}


/**	Sends a kernel message and wait for a reply.

	If the receiving queue is ready to receive its DFC will be queued; otherwise
	the message is simply placed on the end of the queue.
	When received the message will contain a reference counted pointer to the
	current thread.

	@param	aQ	Message queue to which message should be sent.
	
	@return	The result parameter passed to TMessageBase::Complete().
	
	@pre    No fast mutex can be held.
	@pre	Call in a thread context.
	@pre	Kernel must be unlocked
	@pre	interrupts enabled

	@see	TMessageBase::Complete()
 */
EXPORT_C TInt TMessageBase::SendReceive(TMessageQue* aQ)
	{
	CHECK_PRECONDITIONS(MASK_THREAD_STANDARD,"TMessageBase::SendReceive");			
	__KTRACE_OPT(KSERVER,Kern::Printf(">MsgB::SendRcv %08x to %08x",this,aQ));
	Send(aQ);
	NKern::FSWait(&iSem);
	__KTRACE_OPT(KSERVER,Kern::Printf("<MsgB::SendRcv ret %d",iValue));
	return iValue;
	}


/**	Completes a kernel message and optionally receive the next one.

	@param	aResult		 Completion code to pass back to message sender.
	@param	aReceiveNext TRUE means receive the next message on the same queue (if any) immediately.
						 FALSE means don't receive any more messages on that queue yet.
	
	@pre    No fast mutex can be held.					 
	@pre	Call in a thread context.
	@pre	Kernel must be unlocked
	@pre	interrupts enabled
	@pre	Calling thread must be in a critical section
	
	@post	The reference count on the sending thread is closed asynchronously.

	@see	TMessageBase::SendReceive()
 */
EXPORT_C void TMessageBase::Complete(TInt aResult, TBool aReceiveNext)
	{
	CHECK_PRECONDITIONS(MASK_THREAD_CRITICAL,"TMessageBase::Complete");			
	__KTRACE_OPT(KSERVER,Kern::Printf("MsgB::Complete %08x, %d",this,aResult));
	TMessageQue::Lock();
	__NK_ASSERT_ALWAYS(iState==EAccepted);
	iValue=aResult;
	iState=EFree;
	DThread* pT=_LOFF(iSem.iOwningThread,DThread,iNThread);
	if (aReceiveNext)
		{
		__NK_ASSERT_ALWAYS(!iQueue->iReady);
		if (!iQueue->iQ.IsEmpty())
			{
			TMessageBase* pM=(TMessageBase*)iQueue->iQ.First()->Deque();
			__KTRACE_OPT(KSERVER,Kern::Printf("rxnext: got %08x",pM));
			pM->iState=EAccepted;
			iQueue->iMessage=pM;
			iQueue->Enque();
			}
		else
			{
			__KTRACE_OPT(KSERVER,Kern::Printf("rxnext"));
			iQueue->iReady=ETrue;
			iQueue->iMessage=NULL;
			}
		}
	iQueue=NULL;
	NKern::FSSignal(&iSem,&TMessageQue::MsgLock);
	pT->AsyncClose();
	}


/**	Forwards a kernel message to another queue, and optionally receives the next
	message on the original queue.

	@param	aQ			 The queue to which the message should be forwarded.
	@param	aReceiveNext TRUE means receive the next message on the original queue (if any) immediately
						 FALSE means don't receive any more messages on that queue yet.
	
	@pre    No fast mutex can be held.					 
	@pre	Call in a thread context.
	@pre	Kernel must be unlocked
	@pre	interrupts enabled
 */
EXPORT_C void TMessageBase::Forward(TMessageQue* aQ, TBool aReceiveNext)
	{
	CHECK_PRECONDITIONS(MASK_THREAD_STANDARD,"TMessageBase::Forward");			
	__KTRACE_OPT(KSERVER,Kern::Printf("MsgB::Forward %08x->%08x",this,aQ));
	TMessageQue::Lock();
	__NK_ASSERT_ALWAYS(iState==EAccepted);
	if (aReceiveNext)
		{
		__NK_ASSERT_ALWAYS(!iQueue->iReady);
		if (!iQueue->iQ.IsEmpty())
			{
			TMessageBase* pM=(TMessageBase*)iQueue->iQ.First()->Deque();
			__KTRACE_OPT(KSERVER,Kern::Printf("rxnext: got %08x",pM));
			pM->iState=EAccepted;
			iQueue->iMessage=pM;
			iQueue->Enque();
			}
		else
			{
			__KTRACE_OPT(KSERVER,Kern::Printf("rxnext"));
			iQueue->iReady=ETrue;
			iQueue->iMessage=NULL;
			}
		}
	iQueue=aQ;
	if (aQ->iReady)
		{
		iState=EAccepted;
		aQ->iMessage=this;
		aQ->iReady=EFalse;
		aQ->UnlockAndKick();
		}
	else
		{
		aQ->iQ.Add(this);
		iState=EDelivered;
		TMessageQue::Unlock();
		}
	}


/** Called by kernel server if client thread exits with a message outstanding.

	Only cancels the message if its state is DELIVERED - that is it is on a
	queue and not currently being processed.
	Other measures must be taken to avoid problems if threads exit while their
	synchronous kernel messages are ACCEPTED.

    @pre    No fast mutex can be held.
	@pre	Call in a thread context.
	@pre	Kernel must be unlocked
	@pre	interrupts enabled
	@pre	Calling thread must be in a critical section
	
	@post	If the original message state was DELIVERED the reference count on
			the sending thread is closed asynchronously.

	@internalTechnology
 */
EXPORT_C void TMessageBase::Cancel()
	{
	CHECK_PRECONDITIONS(MASK_THREAD_CRITICAL,"TMessageBase::Cancel");			
	__KTRACE_OPT(KSERVER,Kern::Printf("MsgB::Cancel %08x",this));
	DThread* pT=NULL;
	TMessageQue::Lock();
	switch(iState)
		{
		case EDelivered:
			Deque();
			pT=_LOFF(iSem.iOwningThread,DThread,iNThread);
			iState=EFree;
			iQueue=NULL;
		case EAccepted:
		case EFree:
			break;
		}
	TMessageQue::Unlock();
	if (pT)
		pT->AsyncClose();
	}


/** Panics the sender of a kernel message.

	Also completes the message with reason code KErrDied. This is done so that
	the open reference on the client can be closed.

	@param	aCategory	Category string for panic.
	@param	aReason		Reason code for panic.
	
	@pre    No fast mutex can be held.
	@pre	Call in a thread context.
	@pre	Kernel must be unlocked
	@pre	interrupts enabled
 */
EXPORT_C void TMessageBase::PanicClient(const TDesC& aCategory, TInt aReason)
	{
	CHECK_PRECONDITIONS(MASK_THREAD_STANDARD,"TMessageBase::PanicClient");			
	NKern::LockSystem();
	DThread* pT=_LOFF(iSem.iOwningThread,DThread,iNThread);
	pT->Die(EExitPanic,aReason,aCategory);
	Complete(KErrDied,ETrue);	// so reference on client is closed
	}


/** Gets a pointer to the sending thread.

	@return	Pointer to the thread which sent this message.
			This pointer is reference counted provided that the message has not
			yet been completed.
			
	@pre	Call in any context.
	@pre	Message must be in ACCEPTED state.
 */
EXPORT_C DThread* TMessageBase::Client()
	{
	return _LOFF(iSem.iOwningThread,DThread,iNThread);
	}


/** Gets the current thread's kernel message.

	@return Current thread's kernel message.

	@pre No fast mutex can be held.
	@pre Call in a thread context.
	@pre Kernel must be unlocked
	@pre interrupts enabled
*/
EXPORT_C TThreadMessage& Kern::Message()
	{
	CHECK_PRECONDITIONS(MASK_THREAD_STANDARD,"Kern::Message");
	return TheCurrentThread->iKernMsg;
	}


/** Constructs a kernel side message queue.

	A message queue contains a DFC which runs whenever a message is available
	and has been requested.

	@param	aFunction	Function to be called on reception of a message.
	@param	aPtr		Arbitrary parameter to be passed to above function.
	@param	aDfcQ		Pointer to DFC queue to be used for processing messages.
	@param	aPriority	Priority of this message queue within the DFC queue (0-7).
 */
EXPORT_C TMessageQue::TMessageQue(TDfcFn aFunction, TAny* aPtr, TDfcQue* aDfcQ, TInt aPriority)
	:	TDfc(aFunction,aPtr,aDfcQ,aPriority),
		iReady(EFalse),
		iMessage(NULL)
	{
	}


/** Requests the next message on this queue.

	If the queue is nonempty the next message is removed from the queue, marked
	as ACCEPTED and a pointer to it is placed in the iMessage member; the DFC is
	then activated.
	
	If the queue is empty it is marked as ready so that the next message to be
	sent will be accepted immediately.

    @pre    No fast mutex can be held.
	@pre	Call in a thread context.
	@pre	Kernel must be unlocked
	@pre	interrupts enabled
 */
EXPORT_C void TMessageQue::Receive()
	{
	CHECK_PRECONDITIONS(MASK_THREAD_STANDARD,"TMessageQue::Receive");			
	Lock();
	__NK_ASSERT_ALWAYS(!iReady);
	if (!iQ.IsEmpty())
		{
		iMessage=(TMessageBase*)iQ.First()->Deque();
		__KTRACE_OPT(KSERVER,Kern::Printf("MsgQ:Rx got %08x",iMessage));
		iMessage->iState=TMessageBase::EAccepted;
		UnlockAndKick();
		}
	else
		{
		__KTRACE_OPT(KSERVER,Kern::Printf("MsgQ:Rx"));
		iReady=ETrue;
		iMessage=NULL;
		Unlock();
		}
	}


/** Gets the next message synchronously if there is one.

	If the queue is nonempty the next message is removed from the queue, marked
	as ACCEPTED and a pointer to it returned.
	If the queue is empty NULL is returned.
	No asynchronous receive operation may be pending on the queue.

	@return	Pointer to next message or NULL if there is none.
	
	@pre    No fast mutex can be held.
	@pre	Call in a thread context.
	@pre	Queue must not be in asynchronous receive mode.
	@pre	Kernel must be unlocked
	@pre	interrupts enabled
 */
EXPORT_C TMessageBase* TMessageQue::Poll()
	{
	CHECK_PRECONDITIONS(MASK_THREAD_STANDARD,"TMessageQue::Poll");			
	TMessageBase* pM=NULL;
	Lock();
	__NK_ASSERT_ALWAYS(!iReady);
	if (!iQ.IsEmpty())
		{
		pM=(TMessageBase*)iQ.First()->Deque();
		__KTRACE_OPT(KSERVER,Kern::Printf("MsgQ:Poll got %08x",pM));
		pM->iState=TMessageBase::EAccepted;
		}
	Unlock();
	return pM;
	}


/** Finds the last outstanding message on this queue.

	If the queue is nonempty a pointer to the last currently outstanding message
	is returned. The message is NOT marked as ACCEPTED or removed from the queue.
	If the queue is empty NULL is returned.

	@return	Pointer to last outstanding message or NULL if there is none.
	
	@pre    No fast mutex can be held.
	@pre	Call in a thread context.
	@pre	Kernel must be unlocked
	@pre	interrupts enabled
 */
EXPORT_C TMessageBase* TMessageQue::Last()
	{
	CHECK_PRECONDITIONS(MASK_THREAD_STANDARD,"TMessageQue::Last");			
	TMessageBase* pM=NULL;
	Lock();
	if (!iQ.IsEmpty())
		{
		pM=(TMessageBase*)iQ.Last();
		__KTRACE_OPT(KSERVER,Kern::Printf("MsgQ(%08x):Last=%08x",this,pM));
		}
	Unlock();
	return pM;
	}


/**	Completes all outstanding messages on this queue.

	No request is made to receive further messages.

	@param	aResult		 Completion code to pass back to message sender(s).
	
	@pre    No fast mutex can be held.
	@pre	Call in a thread context.
	@pre	Kernel must be unlocked
	@pre	interrupts enabled
	@pre	Calling thread must be in a critical section
	
	@post	The reference counts on all sending threads are closed asynchronously.
	@post	The queue is not ready to receive further messages.
 */
EXPORT_C void TMessageQue::CompleteAll(TInt aResult)
	{
	CHECK_PRECONDITIONS(MASK_THREAD_CRITICAL,"TMessageQue::CompleteAll");			
	TMessageBase* pM;
	while ((pM=Poll())!=NULL)
		pM->Complete(aResult,EFalse);
	}


/** Reads a descriptor from a thread's process.
	
	It reads a descriptor from a thread's address space, enforcing checks on validity of source and destination if necessary.
	It is used especially by device drivers to transfer data from a user thread.
	aDest might be accessed with user permission validation.
	  
	@param aThread  Thread from which address space to read.
	@param aSrc     Pointer to a descriptor to read from. It will fail with KErrBadDescriptor if it's NULL.
	@param aDest    Descriptor to write into.
	@param aOffset	Offset in aPtr from where to start reading.
	@param aMode    Flags specifying how to read. KChunkShiftBy1 treats descriptors as 16bit while KChunkShiftBy0 treats descriptors as 8bit variants.
	
	@return KErrNone, if succesful;
	        KErrBadDescriptor, if aSrc or aDest is an invalid decriptor;
            KErrArgument, if aOffset is negative;
            KErrDied, if aThread is dead.

	@pre    No fast mutex can be held.
	@pre	Call in a thread context.
	@pre	Kernel must be unlocked
	@pre	interrupts enabled
*/
EXPORT_C TInt Kern::ThreadDesRead(DThread* aThread, const TAny* aSrc, TDes8& aDest, TInt aOffset, TInt aMode)
	{
	CHECK_PRECONDITIONS(MASK_THREAD_STANDARD,"Kern::ThreadDesRead");
	_CHECK_DFCQ01_CONTEXT("Kern::ThreadDesRead");
#ifndef __MEMMODEL_FLEXIBLE__
	NKern::LockSystem();
#endif
	TInt r=aThread->DesRead(aSrc,(TUint8*)aDest.Ptr(),aDest.MaxLength(),aOffset,aMode);
#ifndef __MEMMODEL_FLEXIBLE__
	NKern::UnlockSystem();
#endif
	if (r<0)
		return r;
	aDest.SetLength(r);
	return KErrNone;
	}


/** Reads a raw buffer from a thread's process.

	It reads a raw buffer from a thread's address space, enforcing checks on validity of source and destination if necessary.
	It is used especially by device drivers to transfer data from a user thread.
	aDest might be accessed with user permission validation.
	  
	@param aThread Thread from which address space to read.
	@param aSrc    Pointer to a buffer to read from. The buffer is in aThread's address space.
	@param aDest   Pointer to a buffer to write into. The buffer is in the current process's address space.
	@param aSize   Length in bytes to be read.
	
	@return KErrNone, if successful;
	        KErrDied, if aThread is dead.
	        KErrBadDescriptor, if the attempt to read aSrc buffer causes exception.

	@pre    No fast mutex can be held.
	@pre	Call in a thread context.
	@pre	Kernel must be unlocked
	@pre	interrupts enabled
*/
EXPORT_C TInt Kern::ThreadRawRead(DThread* aThread, const TAny* aSrc, TAny* aDest, TInt aSize)
	{
	CHECK_PRECONDITIONS(MASK_THREAD_STANDARD,"Kern::ThreadRawRead");
	_CHECK_DFCQ01_CONTEXT("Kern::ThreadRawRead");
#ifndef __MEMMODEL_FLEXIBLE__
	NKern::LockSystem();
#endif
	TIpcExcTrap xt;
	xt.iLocalBase=0;
	xt.iRemoteBase=(TLinAddr)aSrc;
	xt.iSize=aSize;
	xt.iDir=0;
	TInt r=xt.Trap(aThread);
	if (r==0)
		{
		//On some memory models(such as moving), RawRead may update the content of xt. It happens if home address
		// is accessed (instead of the provided run address) or if it reads/writes in chunks.
		r=aThread->RawRead(aSrc,aDest,aSize,0, &xt);
		xt.UnTrap();
		}
#ifndef __MEMMODEL_FLEXIBLE__
	NKern::UnlockSystem();
#endif
	return r;
	}


/**	Writes a descriptor to a thread's process.
	
	It writes a descriptor to a thread's address space, enforcing checks on validity of source and destination if necessary.
	It is used especially by device drivers to transfer data to a user thread.
	aDest might be accessed with user permission validation.
		  
	@param aThread Thread from which address space to read.
	@param aDest   Pointer to a descriptor to write into. It will fail with KErrBadDescriptor if it's NULL.
	@param aSrc    Descriptor to read from.
	@param aOffset Offset in aDest from where to start writing.
	@param aMode   Flags specifying how to write:
	               KChunkShiftBy1 treats descriptors as 16bit;
	               KChunkShiftBy0 treats descriptors as 8bit variants;
	               KTruncateToMaxLength ensures that data in the target descriptor is truncated, if necessary,
	               to ensure that it does not exceed the target descriptor's maximum length.
	@param aOrigThread The thread on behalf of which this operation is performed (eg client of device driver).
	
	@return KErrNone, if successful;
	        KErrBadDescriptor, if aSrc or aDest is an invalid decriptor;
	        KErrArgument, if aOffset is negative;
	        KErrDied, if aThread is dead.

	@pre    No fast mutex can be held.
	@pre	Call in a thread context.
	@pre	Kernel must be unlocked
	@pre	interrupts enabled
*/
EXPORT_C TInt Kern::ThreadDesWrite(DThread* aThread, TAny* aDest, const TDesC8& aSrc, TInt aOffset, TInt aMode, DThread* aOrigThread)
	{
	CHECK_PRECONDITIONS(MASK_THREAD_STANDARD,"Kern::ThreadDesWrite");
	_CHECK_DFCQ01_CONTEXT("Kern::ThreadDesWrite");
#ifndef __MEMMODEL_FLEXIBLE__
	NKern::LockSystem();
#endif
	TInt r=aThread->DesWrite(aDest,aSrc.Ptr(),aSrc.Length(),aOffset,aMode,aOrigThread);
#ifndef __MEMMODEL_FLEXIBLE__
	NKern::UnlockSystem();
#endif
	return r;
	}


/**	Writes a raw buffer to a thread's process.
	
	It writes a raw buffer to a thread's address space, enforcing checks on validity of source and destination if necessary.
	It is used especially by device drivers to transfer data to a user thread.
	aDest might be accessed with user permission validation.
	  
	@param aThread     Thread to which address space to write.
	@param aDest       Pointer to a buffer to write into. It belongs to aThread's address space.
	@param aSrc        Pointer to a buffer to read from. It belongs to the current process's address space.
	@param aSize       Size in bytes of the copied data.
	@param aOrigThread The thread on behalf of which this operation is performed (eg client of device driver). If NULL, current thread is assumed.
	
	@return KErrNone, if successful;
            KErrDied, if aThread is dead.
	        KErrBadDescriptor, if the attempt to write to aDest buffer causes exception.

	@pre    No fast mutex can be held.
	@pre	Call in a thread context.
	@pre	Kernel must be unlocked
	@pre	interrupts enabled
*/
EXPORT_C TInt Kern::ThreadRawWrite(DThread* aThread, TAny* aDest, const TAny* aSrc, TInt aSize, DThread* aOrigThread)
	{
	CHECK_PRECONDITIONS(MASK_THREAD_STANDARD,"Kern::ThreadRawWrite");
	_CHECK_DFCQ01_CONTEXT("Kern::ThreadRawWrite");
#ifndef __MEMMODEL_FLEXIBLE__
	NKern::LockSystem();
#endif
	TIpcExcTrap xt;
	xt.iLocalBase=0;
	xt.iRemoteBase=(TLinAddr)aDest;
	xt.iSize=aSize;
	xt.iDir=1;
	TInt r=xt.Trap(aThread);
	if (r==0)
		{
		//On some memory models(such as moving), RawRead may update the content of xt. It happens if home address
		// is accessed (instead of the provided run address) or if it reads/writes in chunks.
		r=aThread->RawWrite(aDest,aSrc,aSize,0,aOrigThread, &xt);
		xt.UnTrap();
		}
#ifndef __MEMMODEL_FLEXIBLE__
	NKern::UnlockSystem();
#endif
	return r;
	}


/** Reads a descriptor from a thread's process.
	
	It reads a descriptor from a thread's address space, enforcing checks on validity of source and destination if necessary.
	It is used especially by device drivers to transfer data from a user thread.
	aDest might be accessed with user permission validation.
	  
	@param aThread  Thread from which address space to read.
	@param aSrc     A TClientBuffer object containing the descriptor information.
	@param aDest    Descriptor to write into.
	@param aOffset	Offset in aPtr from where to start reading.
	@param aMode    Flags specifying how to read. KChunkShiftBy1 treats descriptors as 16bit while KChunkShiftBy0 treats descriptors as 8bit variants.
	
	@return KErrNone, if succesful;
	        KErrBadDescriptor, if aSrc or aDest is an invalid decriptor;
            KErrArgument, if aOffset is negative;
            KErrDied, if aThread is dead.

	@pre    No fast mutex can be held.
	@pre	Call in a thread context.
	@pre	Kernel must be unlocked
	@pre	interrupts enabled
	
	@publishedPartner
	@released
*/
EXPORT_C TInt Kern::ThreadBufRead(DThread* aThread, const TClientBuffer* aSrc, TDes8& aDest, TInt aOffset, TInt aMode)
	{
	CHECK_PRECONDITIONS(MASK_THREAD_STANDARD,"Kern::ThreadBufRead");
	_CHECK_DFCQ01_CONTEXT("Kern::ThreadBufRead");
	__ASSERT_DEBUG(aSrc, K::Fault(K::EThreadBufReadWithNullPointer));
#ifndef __MEMMODEL_FLEXIBLE__
	NKern::LockSystem();
#endif
	TInt r=aThread->DoDesRead(aSrc->iHeader,(TUint8*)aDest.Ptr(),aDest.MaxLength(),aOffset,aMode);
#ifndef __MEMMODEL_FLEXIBLE__
	NKern::UnlockSystem();
#endif
	if (r<0)
		return r;
	aDest.SetLength(r);
	return KErrNone;
	}


/**	Writes a descriptor to a thread's process.
	
	It writes a descriptor to a thread's address space, enforcing checks on validity of source and destination if necessary.
	It is used especially by device drivers to transfer data to a user thread.
	aDest might be accessed with user permission validation.
		  
	@param aThread Thread from which address space to read.
	@param aDest   A TClientBuffer object containing information about the descriptorto write into.
	@param aSrc    Descriptor to read from.
	@param aOffset Offset in aDest from where to start writing.
	@param aMode   Flags specifying how to write:
	               KChunkShiftBy1 treats descriptors as 16bit;
	               KChunkShiftBy0 treats descriptors as 8bit variants;
	               KTruncateToMaxLength ensures that data in the target descriptor is truncated, if necessary,
	               to ensure that it does not exceed the target descriptor's maximum length.
	@param aOrigThread The thread on behalf of which this operation is performed (eg client of device driver).
	
	@return KErrNone, if successful;
	        KErrBadDescriptor, if aSrc or aDest is an invalid decriptor;
	        KErrArgument, if aOffset is negative;
	        KErrDied, if aThread is dead.

	@pre    No fast mutex can be held.
	@pre	Call in a thread context.
	@pre	Kernel must be unlocked
	@pre	interrupts enabled
	
	@publishedPartner
	@released
*/
EXPORT_C TInt Kern::ThreadBufWrite(DThread* aThread, TClientBuffer* aDest, const TDesC8& aSrc, TInt aOffset, TInt aMode, DThread* aOrigThread)
	{
	CHECK_PRECONDITIONS(MASK_THREAD_STANDARD,"Kern::ThreadBufWrite");
	_CHECK_DFCQ01_CONTEXT("Kern::ThreadBufesWrite");
	__ASSERT_DEBUG(aDest, K::Fault(K::EThreadBufWriteWithNullPointer));
	if (!aDest->IsWriteable())
		return KErrBadDescriptor;
#ifndef __MEMMODEL_FLEXIBLE__
	NKern::LockSystem();
#endif
	TInt r=aThread->DoDesWrite(aDest->DesPtr(),aDest->iHeader,aSrc.Ptr(),aSrc.Length(),aOffset,aMode|KDoNotUpdateDesLength,aOrigThread);
#ifndef __MEMMODEL_FLEXIBLE__
	NKern::UnlockSystem();
#endif
	if (r < KErrNone)
		return r;
	aDest->iHeader.SetTypeAndLength(r);
	return KErrNone;
	}


/**	Gets the length of a descriptor in another thread's address space.
	
	@param aThread Thread whose address space contains the descriptor.
	@param aDes    Descriptor whose length is to be fetched.
	
	@return Length of descriptor or error code if it fails, specifically
	        KErrBadDescriptor if aDes is an invalid descriptor.

	@pre    No fast mutex can be held.
	@pre	Call in a thread context.
	@pre	Kernel must be unlocked
	@pre	interrupts enabled
*/
EXPORT_C TInt Kern::ThreadGetDesLength(DThread* aThread, const TAny* aDes)
	{
	CHECK_PRECONDITIONS(MASK_THREAD_STANDARD,"Kern::ThreadGetDesLength");
	_CHECK_DFCQ01_CONTEXT("Kern::ThreadGetDesLength");
#ifndef __MEMMODEL_FLEXIBLE__
	NKern::LockSystem();
#endif
	TInt r=aThread->GetDesLength(aDes);
#ifndef __MEMMODEL_FLEXIBLE__
	NKern::UnlockSystem();
#endif
	return r;
	}


/**	Gets the maximum length of a descriptor in another thread's address space.
 
	@param aThread Thread whose address space contains the descriptor.
	@param aDes    Descriptor whose maximum length is to be fetched.
	
	@return Maximum length of descriptor or error code if it fails,
	        specifically KErrBadDescriptor if aDes is an invalid descriptor.

	@pre    No fast mutex can be held.
	@pre	Call in a thread context.
	@pre	Kernel must be unlocked
	@pre	interrupts enabled
*/
EXPORT_C TInt Kern::ThreadGetDesMaxLength(DThread* aThread, const TAny* aDes)
	{
	CHECK_PRECONDITIONS(MASK_THREAD_STANDARD,"Kern::ThreadGetDesMaxLength");
	_CHECK_DFCQ01_CONTEXT("Kern::ThreadGetDesLMaxLength");
#ifndef __MEMMODEL_FLEXIBLE__
	NKern::LockSystem();
#endif
	TInt r=aThread->GetDesMaxLength(aDes);
#ifndef __MEMMODEL_FLEXIBLE__
	NKern::UnlockSystem();
#endif
	return r;
	}


/**	Gets information about a descriptor in another thread's address space.
	
	It returns length, maximum length and pointer to the descriptor's buffer in the parameters sent by reference.
	  
	@param aThread    Thread to read descriptor's maximum length from.
	@param aDes       Descriptor to get info about.
	@param aLength    Reference to a location where length of the descriptor will be copied.
	@param aMaxLength Reference to a location where maximum length of the descriptor will be copied.
	@param aPtr       Reference to a location where the pointer to descriptor's buffer will be copied.
	@param aWriteable ETrue if descriptor is writeable (eg. TBuf<>) and EFalse otherwise (eg. TBufC<>).
	
	@return KErrNone, if successful;
            KErrBadDescriptor, if aDes is an invalid descriptor, or if aWriteable is ETrue and aDes is a constant descriptor.	       

	@pre    No fast mutex can be held.
	@pre	Call in a thread context.
	@pre	Kernel must be unlocked
	@pre	interrupts enabled
*/
EXPORT_C TInt Kern::ThreadGetDesInfo(DThread* aThread, const TAny* aDes, TInt& aLength, TInt& aMaxLength, TUint8*& aPtr, TBool aWriteable)
	{
	CHECK_PRECONDITIONS(MASK_THREAD_STANDARD,"Kern::ThreadGetDesInfo");
	_CHECK_DFCQ01_CONTEXT("Kern::ThreadGetDesInfo");
#ifndef __MEMMODEL_FLEXIBLE__
	NKern::LockSystem();
#endif
	TInt r=aThread->GetDesInfo(aDes,aLength,aMaxLength,aPtr,aWriteable);
#ifndef __MEMMODEL_FLEXIBLE__
	NKern::UnlockSystem();
#endif
	return r;
	}


/********************************************
 * Kernel server client side
 ********************************************/

void GetCategory(TDes& aDest, const TDesC& aSrc)
	{
	TInt ulen, umax;
	TUint8* kptr=(TUint8*)aDest.Ptr();
	const TUint8* uptr=Kern::KUDesInfo(aSrc, ulen, umax);
	if (ulen>KMaxExitCategoryName)
		ulen=KMaxExitCategoryName;
	aDest.SetLength(ulen);
	kumemget(kptr,uptr,ulen);
	}

void ExecHandler::ThreadKill(TInt aHandle, TExitType aType, TInt aReason, const TDesC8* aCategory)
//
// Enter and leave with system unlocked
//
	{
	TBuf<KMaxExitCategoryName> cat;
	if (aType==EExitPanic && aCategory)
		GetCategory(cat,*aCategory);
	__KTRACE_OPT(KEXEC,Kern::Printf("Exec::ThreadKill %d,%d,%S",aType,aReason,&cat));
	K::CheckKernelUnlocked();
	NKern::LockSystem();
	DThread* pT=(DThread*)K::ObjectFromHandle(aHandle,EThread);
	if (!pT)
		K::PanicCurrentThread(EBadHandle);
	if (pT->iOwningProcess->iSecurityZone != TheCurrentThread->iOwningProcess->iSecurityZone)
		K::ProcessIsolationFailure(__PLATSEC_DIAGNOSTIC_STRING("Attempt to Kill, Terminate or Panic a thread in another process"));
	pT->Die(aType,aReason,cat);		// releases system lock
	}

void ExecHandler::ProcessKill(TInt aHandle, TExitType aType, TInt aReason, const TDesC8* aCategory)
	{
	TBuf<KMaxExitCategoryName> cat;
	if (aType==EExitPanic && aCategory)
		GetCategory(cat,*aCategory);
	__KTRACE_OPT(KEXEC,Kern::Printf("Exec::ProcessKill %d,%d,%S",aType,aReason,&cat));
	K::CheckKernelUnlocked();
	NKern::LockSystem();
	DProcess* pP=(DProcess*)K::ThreadEnterCS(aHandle,EProcess);

	DProcess* currentProcess=TheCurrentThread->iOwningProcess;
	if (pP->iSecurityZone != currentProcess->iSecurityZone		// Not killing self...
		&&	currentProcess->iId!=pP->iCreatorId )			// and not creator
		{
		if(!currentProcess->HasCapability(ECapabilityPowerMgmt,__PLATSEC_DIAGNOSTIC_STRING("Checked when attempting to kill another process")))
			{
			// Doesn't have capability...
			// Action not allowed
			pP->Close(NULL);
			K::ThreadLeaveCS();
			K::LockedPlatformSecurityPanic();
			}
		}

	pP->Die(aType,aReason,cat);
	pP->Close(NULL);
	NKern::ThreadLeaveCS();
	}

void ExecHandler::ThreadSuspend(DThread* aThread)
//
// Suspend a thread. Enter and exit with system locked.
//
	{
	__KTRACE_OPT(KEXEC,Kern::Printf("Exec::ThreadSuspend %O",aThread));
	if(aThread->iOwningProcess->iSecurityZone!=TheCurrentThread->iOwningProcess->iSecurityZone)
		K::ProcessIsolationFailure(__PLATSEC_DIAGNOSTIC_STRING("Use of RThread::Suspend on a thread in another process"));
	aThread->Suspend(1);
	}

void K::PanicKernExec(TInt aReason)
	{
	// enter with system unlocked
	NKern::LockSystem();
	K::PanicCurrentThread(aReason);
	}

void K::PanicCurrentThread(TInt aReason)
	{
	// enter with system locked
	TheCurrentThread->Die(EExitPanic,aReason,KLitKernExec());	// doesn't return
	}


/**
Panics the current thread.

If the kernel is locked or the current thread holds a fast mutex this will fault
the kernel.
It can be used in a device driver to panic the client thread in case of errors.

@param aCategory A descriptor that specifies the exit category to be set
				in the current thread's exit information. It can't exceed
				KMaxExitCategoryName characters in length.
@param aReason	Reason for panic. Can be a standard kernel generated TKernelPanic
				or it can be a custom reason (eg. if it comes from a device driver).

@pre Kernel must be unlocked.
@pre No fast mutex can be held.
@pre Call in a thread context.
@pre interrupts enabled
@pre Can be used in a device driver.

@post It doesn't return.

@see TKernelPanic
*/
EXPORT_C void Kern::PanicCurrentThread(const TDesC& aCategory, TInt aReason)
	{
	CHECK_PRECONDITIONS(MASK_THREAD_STANDARD,"Kern::PanicCurrentThread");			
	// enter with system unlocked
	__KTRACE_OPT(KEXEC,Kern::Printf("Kern::PanicCurrentThread %S %d",&aCategory,aReason));
	K::CheckKernelUnlocked();
	NKern::LockSystem();
	TheCurrentThread->Die(EExitPanic,aReason,aCategory);	// doesn't return
	}


/**	Terminates the execution of a thread.

	It terminates the specified thread and it sets its exit info.
	This is an asynchronous operation.

	This method can only be used by a thread to kill itself, or to kill
	a user thread (iThreadType==EThreadUser). An attempt to kill a non-user
	thread which is not the currently running thread will cause the system to fault
	with KERN 94 (ENonUserThreadKilled).

	@param aThread Thread to be terminated. If it's NULL then the current thread will be terminated.
	@param aType Exit type. It can be one of EExitKill - if the thread was killed by another thread, EExitTerminate - usually for abnormal termination,EExitPanic - if the thread was terminated as the result of a panic.
	@param aReason Exit code.
	@param aCategory Exit category, this is relevant only if exit type is EExitPanic.

	@pre    No fast mutex can be held.
	@pre	Call in a thread context.
	@pre	Kernel must be unlocked
	@pre	interrupts enabled
*/
EXPORT_C void Kern::ThreadKill(DThread* aThread, TExitType aType, TInt aReason, const TDesC& aCategory)
	{
	CHECK_PRECONDITIONS(MASK_THREAD_STANDARD,"Kern::ThreadKill");
	if (!aThread)
		aThread=TheCurrentThread;
	NKern::LockSystem();
	aThread->Die(aType,aReason,aCategory);
	}

/**
Closes a kernel side reference-counted object without blocking
the current thread.

The function decrements the object's reference count by one, and invokes
asynchronous deletion of the object when the final reference to the object
is being closed.

The function assumes that Close() has not been re-implemented.

@return The object's reference count value before the call to this function.

@pre Kernel must be unlocked.
@pre Call in a thread context.
@pre interrupts enabled
@pre Thread must be in a critical section

@see DObject::Close()
*/
EXPORT_C TInt DObject::AsyncClose()
	{
	CHECK_PRECONDITIONS(MASK_KERNEL_UNLOCKED | MASK_INTERRUPTS_ENABLED | MASK_NOT_ISR | MASK_NOT_IDFC | MASK_CRITICAL, "DObject::AsyncClose");
	__KTRACE_OPT(KSERVER,Kern::Printf("DObject::AsyncClose() %O",this));

	TInt r=Dec();
	if (r==1)
		AsyncDelete();
	return r;
	}


/**
Asynchronously delete a DBase-derived object.

@pre Kernel must be unlocked.
@pre Call in a thread context.
@pre interrupts enabled
@pre Thread must be in a critical section
*/
EXPORT_C void DBase::AsyncDelete()
	{
	CHECK_PRECONDITIONS(MASK_NO_KILL_OR_SUSPEND | MASK_INTERRUPTS_ENABLED | MASK_NOT_ISR | MASK_NOT_IDFC, "DObject::AsyncDelete");
	__KTRACE_OPT(KSERVER,Kern::Printf("DBase::AsyncDelete() %08x",this));
	
	DBase* oldHead = K::AsyncDeleteHead;
	do	{
		iAsyncDeleteNext = oldHead;
		} while (!__e32_atomic_cas_rel_ptr(&K::AsyncDeleteHead, &oldHead, this));
	if (!oldHead)
		K::AsyncFreeDfc.Enque();
	}


/**	Frees a heap buffer asynchronously.

	aPtr must point to a currently allocated cell on the kernel heap.

	@param aPtr Pointer to the kernel heap cell to be freed.

	@pre Calling thread must be in a critical section.
	@pre Kernel must be unlocked.
	@pre Call in a thread context.
	@pre interrupts enabled

	@post Calling thread is in a critical section.
*/
EXPORT_C void Kern::AsyncFree(TAny* aPtr)
//
// Asynchronously free a kernel heap cell (must be >=4 bytes in length)
//
	{
	CHECK_PRECONDITIONS(MASK_NO_KILL_OR_SUSPEND | MASK_INTERRUPTS_ENABLED | MASK_NOT_ISR | MASK_NOT_IDFC, "Kern::AsyncFree");
	__KTRACE_OPT(KSERVER,Kern::Printf("Kern::AsyncFree(%08x)",aPtr));
	TAny* oldHead = K::AsyncFreeHead;
	do	{
		*(TAny**)aPtr = oldHead;	// use first word of cell as link field
		} while (!__e32_atomic_cas_rel_ptr(&K::AsyncFreeHead, &oldHead, aPtr));
	if (!oldHead)
		K::AsyncFreeDfc.Enque();
	}


/** Asynchronously notifies all change notifiers of a set of events.

	This call queues a DFC executed by the supervisor thread.  The DFC causes
	all DChangeNotifier objects to record the events indicated and to signal
	their owning thread if it is currently logged on.

	@param	aChanges	The mask of events to be notified (TChanges enumeration).
	
	@pre	Call in a thread context.
	@pre	Do not call from an ISR.
	@pre	Calling thread must be in a critical section.
	@pre	No fast mutex can be held.
	@pre	Kernel must be unlocked.
	@pre	interrupts enabled
	
	@see Kern::NotifyChanges()
	@see TChanges
 */
EXPORT_C void Kern::AsyncNotifyChanges(TUint aChanges)
	{
	CHECK_PRECONDITIONS(MASK_THREAD_CRITICAL,"Kern::AsyncNotifyChanges");			
	__KTRACE_OPT(KSERVER,Kern::Printf("Kern::AsyncNotifyChanges %08x",aChanges));
	TUint old = __e32_atomic_ior_ord32(&K::AsyncChanges, aChanges);
	if (old==0 && aChanges)
		K::AsyncChangeNotifierDfc.Enque();
	}


/********************************************
 * Kernel server functions
 ********************************************/
void K::DoAsyncFree(TAny*)
//
// Free any kernel heap cells queued for asynchronous freeing
// Delete any DBase-derived objects queued for asynchronous deletion
//
	{
	__KTRACE_OPT(KSERVER,Kern::Printf("K::DoAsyncFree"));
	while (K::AsyncFreeHead || K::AsyncDeleteHead)
		{
		TAny* p = __e32_atomic_swp_acq_ptr(&K::AsyncFreeHead, 0);
		DBase* pD = (DBase*)__e32_atomic_swp_acq_ptr(&K::AsyncDeleteHead, 0);
		while (p)
			{
			TAny* next = *(TAny**)p;
			__KTRACE_OPT(KSERVER,Kern::Printf("AsyncFree %08x",p));
			Kern::Free(p);
			p = next;
			}
		if (pD)
			{
			NKern::LockSystem();
			NKern::UnlockSystem();
			do	{
				DBase* next = pD->iAsyncDeleteNext;
				__KTRACE_OPT(KSERVER,Kern::Printf("AsyncDelete %08x",pD));
				DBase::Delete(pD);
				pD = next;
				} while(pD);
			}
		}
	}

void K::DoAsyncNotify(TAny*)
//
// Asynchronously signal change notifiers
//
	{
	TUint changes = __e32_atomic_swp_rel32(&K::AsyncChanges, 0);
	__KTRACE_OPT(KSERVER,Kern::Printf("K::DoAsyncNotify %08x",changes));
	if (changes)
		Kern::NotifyChanges(changes);
	}

/********************************************
 * Kernel server entry point
 ********************************************/
void K::StartKernelServer()
	{
	__KTRACE_OPT(KSERVER,Kern::Printf("K::StartKernelServer"));
	Kern::SetThreadPriority(KKernelServerDefaultPriority);
	K::SvBarrierQ.Receive();
	__KTRACE_OPT(KSERVER,Kern::Printf("Server waiting"));
	TDfcQue::ThreadFunction(K::SvMsgQ);
	}

void K::DoSvBarrier(TAny* a)
	{
	TMessageQue& q = *(TMessageQue*)a;
	q.iMessage->Complete(KErrNone, ETrue);
	}

