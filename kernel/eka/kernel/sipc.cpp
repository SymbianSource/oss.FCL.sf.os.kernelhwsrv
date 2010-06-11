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
// e32\kernel\sipc.cpp
// 
//

#include <kernel/kern_priv.h>
#include "execs.h"
#include "memmodel.h"

#define iMState		iWaitLink.iSpare1

const TInt KMaxMsgLimit=512;

extern "C" void __SendDiscMsg(DSession* aSession);

/********************************************
 * User side message formats
 ********************************************/

// User side message version
// This must match the layout of class RMessage2
class RMessageU2
	{
public:
	inline RMessageU2(const RMessageK& a);
public:
	// from RMessagePtr2
	TInt iHandle;
	// from RMessage2
	TInt iFunction;
	TInt iArgs[KMaxMessageArguments];
	TUint32 iSpare1;
	const TAny* iSessionPtr;
#if		0
	// not used or copied by IPC, so not required ...
	mutable TInt iFlags;
	TInt iSpare3;
#endif
	};

inline RMessageU2::RMessageU2(const RMessageK& a)
	{
	iHandle = (TInt)&a;
	iFunction = a.iFunction;
	if (iFunction == RMessage2::EDisConnect)
		{
		iArgs[0] = 0;
		iArgs[1] = 0;
		iArgs[2] = 0;
		iArgs[3] = 0;
		}
	else
		{
		iArgs[0] = a.Arg(0);
		iArgs[1] = a.Arg(1);
		iArgs[2] = a.Arg(2);
		iArgs[3] = a.Arg(3);
		}
	iSpare1 = 0;
	iSessionPtr = a.iSession->iSessionCookie;
#ifdef KIPC
	if (KDebugNum(KIPC))
		Kern::Printf("RMessageU2(%08X): %08X %08X; %08X->%08X",
			&a, iHandle, iFunction, a.iSession, iSessionPtr);
#endif //KIPC
	}

TServerMessage::TServerMessage() :
	TClientRequest(CallbackFunc)
	{
	}

#ifndef __CLIENT_REQUEST_MACHINE_CODED__

void TServerMessage::CallbackFunc(TAny* aData, TUserModeCallbackReason aReason)
	{
	TServerMessage* req = (TServerMessage*)aData;
	if (aReason == EUserModeCallbackRun && req->iResult == KErrNone)
		{
		RMessageU2 userData(*req->iMessageData);
		K::USafeWrite(req->iMessagePtr, &userData, sizeof(userData));
		}
	TClientRequest::CallbackFunc(aData, aReason);
	}

#endif

RMessageK::RMessageK() :
	TClientRequest(CallbackFunc),
   	iAccessCount(0)
	{
	}

void RMessageK::CallbackFunc(TAny* aData, TUserModeCallbackReason aReason)
	{
	RMessageK* msg = (RMessageK*)aData;

	TBool ok = ETrue;
	if (aReason == EUserModeCallbackRun)
		{
		// Write back updated descriptor lengths
		TInt flags = msg->iMsgArgs.AllDesWritten();
		for (TInt i = 0 ; flags != 0 ; ++i, flags >>= 1)
			{
			if (flags & 1)
				{
				const TDesHeader& des = msg->Descriptor(i);
				TAny* exc = K::USafeWrite(msg->Ptr(i), &des.TypeAndLength(), sizeof(TUint32));
				if (exc != NULL)
					ok = EFalse;	
				if (ok && des.Type() == EBufCPtr)
					{
					TInt len = des.Length();
					TUint8* pL = (TUint8*)(des.DataPtr() - sizeof(TDesC));
					exc = K::USafeWrite(pL, &len, sizeof(len));
					if (exc != NULL)
						ok = EFalse;	
					}
				}
			}
		}
	
	TClientRequest::CallbackFunc(aData, aReason);

	NKern::LockSystem();
	__NK_ASSERT_DEBUG(msg->IsCompleting());
	msg->CloseRef();
	NKern::UnlockSystem();

	if (!ok)
		{
		NKern::ThreadLeaveCS();
		K::PanicKernExec(EBadIpcDescriptor);
		}
	}

void RMessageK::OpenRef()
	{ 
	// iAccess count will probably never be more than 2?
	__NK_ASSERT_DEBUG(iAccessCount < 0xfe);
	__e32_atomic_add_ord8(&iAccessCount, 1);
	}

void RMessageK::CloseRef()
	{
	__ASSERT_SYSTEM_LOCK;
	if (__e32_atomic_add_ord8(&iAccessCount, (TUint8)-1) == 1)
	   	Free(); 
	}

#if		0
// Just to exercise the chunk-adjustment code and check that we
// manage the system lock and thread critical sections correctly.
// Enter and return with system unlocked and caller in a critical section
static void JiggleMsgChunk()
	{
	CHECK_PRECONDITIONS(MASK_THREAD_CRITICAL, "RMessageK::JiggleMsgChunk");
	DChunk* msgChunk = K::MsgInfo.iChunk;
	if (msgChunk)
		{
		__ASSERT_DEBUG((TUint)msgChunk->Size() == K::MsgInfo.iCurrSize, K::Fault(K::EMsgFreeBadPool));
		msgChunk->Adjust(K::MsgInfo.iCurrSize+Kern::RoundToPageSize(1));
		msgChunk->Adjust(K::MsgInfo.iCurrSize);
		__ASSERT_DEBUG((TUint)msgChunk->Size() == K::MsgInfo.iCurrSize, K::Fault(K::EMsgFreeBadPool));
		}
	}

	// We must drop the system lock before calling JiggleMsgChunk(),
	// but as our caller may be holding resources we have to enter a
	// critical section before doing so. Note that K::ThreadEnterCS()
	// (as opposed to NKern::ThreadEnterCS()) both enters a critical
	// section AND then releases the lock; likewise K::ThreadLeaveCS()
	// reclaims the system lock before leaving the critical section,
	// so there is no window for the thread to be killed while holding
	// allocated resources ...
	K::ThreadEnterCS();
	JiggleMsgChunk();
	K::ThreadLeaveCS();
#endif

#ifdef	_DEBUG
// Check consistency of the specified list
// Enter and return with system locked
static TBool MessagePoolIntact(RMessageK* p = K::MsgInfo.iNextMessage, TInt count = K::MsgInfo.iFreeMessageCount)
{
	CHECK_PRECONDITIONS(MASK_SYSTEM_LOCKED, "DSession::MessagePoolIntact");
	TInt k = 0;
	while (p)
		{
		k += 1;
		p = (RMessageK*)p->iSessionLink.iNext;
		}

	return (k == count);
}

// Ditto, but enter and return with system unlocked
static TBool MessagePoolIntactUnlocked()
	{
	NKern::LockSystem();
	TInt r = MessagePoolIntact();
	NKern::UnlockSystem();
	return r;
	}
#endif

#ifndef __MESSAGE_MACHINE_CODED_2__
EXPORT_C RMessageK* RMessageK::MessageK(TInt aMsgHandle)
//
// Validates a message handle and converts it into a pointer.
// Enter and leave with system locked.
//
	{
	RMessageK* m = MessageK(aMsgHandle, TheCurrentThread);
	if (m == NULL || m->iFunction == RMessage2::EDisConnect)
		K::PanicCurrentThread(EBadMessageHandle);
	return m;
	}

RMessageK* RMessageK::MessageK(TInt aMsgHandle, DThread* aThread)
//
// Validates a message handle and converts it into a pointer.
// Enter and leave with system locked.
//
	{
	RMessageK& m = *(RMessageK*)aMsgHandle;
	SDblQueLink lnk;

	// Handle must point into kernel message chunk, and be correctly aligned
	TInt offset = (TUint8*)&m - K::MsgInfo.iBase;
	if (offset < 0 || offset+sizeof(RMessageK) > (TUint)K::MsgInfo.iMaxSize)
		return NULL;
	if (offset & (KMessageSize-1))
		return NULL;

	// Message must be readable, in the correct state and owned by the right process
	if (Kern::SafeRead(&m.iServerLink, &lnk, sizeof(lnk)) != NULL
	||  TUint32(lnk.iNext) != ~TUint32(&m)
	||	TUint32(lnk.iPrev) != ~TUint32(aThread->iOwningProcess))
		return NULL;

	return &m;
	}
#endif

// Claim one page of memory, carve it up into message blocks,
// and add them to the global pool
// Enter and return with system lock, may be temporarily released though
TInt RMessageK::ExpandMessagePool()
	{
	// KMessageSize must have been defined as a power of two at least as big as an RMessageK
	__ASSERT_COMPILE((KMessageSize & (KMessageSize-1)) == 0);
	__ASSERT_COMPILE(KMessageSize >= sizeof(RMessageK));

	// This function can be called during the creation of the very first thread, which is
	// before the msg chunk has been created (the first thread will own the chunk, so must
	// be created first). In this situation we cannot expand the pool, and so just return.
	// This first thread is thus unique in not having a preallocated message for synchronous
	// use; but it's destined to become the idle thread and as such never makes use of IPC.
	// So that's alright then :)
	DChunk* msgChunk = K::MsgInfo.iChunk;
	if (!msgChunk)
		return KErrNotReady;

	CHECK_PRECONDITIONS(MASK_SYSTEM_LOCKED, "DSession::ExpandMessagePool");
	__ASSERT_DEBUG(MessagePoolIntact(), K::Fault(K::EMsgFreeBadPool));

	RMessageK* first = NULL;
	RMessageK* m = NULL;
	TInt r = KErrNone;
	TInt k = 0;

	// We must drop the system lock before calling DChunk::Adjust().
	// This DMutex ensures single-threading while adjusting the pool.
	// Note that the system lock may be dropped and reacquired here ...
	TUint oldSize = K::MsgInfo.iCurrSize;
	NKern::ThreadEnterCS();
	K::MsgInfo.iMsgChunkLock->Wait();
	NKern::UnlockSystem();

	// Someone may have got in first while we were waiting for the DMutex.
	// So we only expand the chunk if its size is unchanged from above;
	// otherwise we just do nothing and let the caller retry if necessary
	if (K::MsgInfo.iCurrSize == oldSize)
		{
		__ASSERT_DEBUG((TUint)msgChunk->Size() == K::MsgInfo.iCurrSize, K::Fault(K::EMsgFreeBadPool));
		__KTRACE_OPT(KSERVER,Kern::Printf("ExpandMessagePool(): MsgChunk %08X, base %08X, size %08X, max %08X",
			msgChunk, K::MsgInfo.iBase, K::MsgInfo.iCurrSize, K::MsgInfo.iMaxSize));

		r = msgChunk->Adjust(K::MsgInfo.iCurrSize+Kern::RoundToPageSize(KMessageSize));
		__KTRACE_OPT(KSERVER,Kern::Printf("ExpandMessagePool(): Adjust returns %d, size now %08X", r, msgChunk->Size()));

		if (r == KErrNone)
			{
			// Turn the allocated memory info a list of messages
			// This can (and should) be done without the system lock
			TUint8* p = K::MsgInfo.iBase + K::MsgInfo.iCurrSize;
			K::MsgInfo.iCurrSize = (TUint)msgChunk->Size();
			TUint8* top = K::MsgInfo.iBase + K::MsgInfo.iCurrSize;
			__ASSERT_DEBUG(p < top, K::Fault(K::EMsgFreeBadPool));
			memclr(p, top-p);

			for (first = (RMessageK*)p, k = 0; p < top; ++k)
				{
				m = (RMessageK*)p;
				p += KMessageSize;
				new (m) RMessageK;
				m->iMsgType = EGlobal;
				m->iSessionLink.iNext = (SDblQueLink*)p;
				}

			__ASSERT_DEBUG(p == top, K::Fault(K::EMsgFreeBadPool));
			__ASSERT_DEBUG((TAny*)(m+1) <= top, K::Fault(K::EMsgFreeBadPool));
			}
		}

	// Reacquire system lock before adding the new messages to the pool
	// Note that K::MsgInfo.iNextMessage may no longer be NULL
	NKern::LockSystem();

	if (first)
		{
		// We expanded the pool; m is the last allocated message
		m->iSessionLink.iNext = (SDblQueLink*)K::MsgInfo.iNextMessage;
		K::MsgInfo.iNextMessage = first;
		K::MsgInfo.iFreeMessageCount += k;
		}

	// Now we can release the pool mutex; unfortunately this also releases
	// the system lock, so we have to acquire it again before returning.
	// This makes it possible -- though very unlikely -- that after adding
	// messages to the pool, they *all* get claimed by other threads in the
	// window between dropping the system lock and reacquiring it, so that
	// our caller finds the pool still empty.  In such cases, it's up to
	// the caller to retry ...
	K::MsgInfo.iMsgChunkLock->Signal();
	NKern::LockSystem();
	NKern::ThreadLeaveCS();
	return r;
	}

// Claim a chain of aCount RMessageK objects from the global pool,
// and set their type to aType (e.g. ESession or ESync)
// Enter and return with system unlocked and caller in a critical section
RMessageK* RMessageK::ClaimMessagePool(enum TMsgType aType, TInt aCount, DSession *aSession)
	{
	CHECK_PRECONDITIONS(MASK_THREAD_CRITICAL, "RMessageK::ClaimMessagePool");

	RMessageK* chain = NULL;
	TInt wanted = aCount;

	// In order not to hold the system lock for too long at one time,
	// we release it between iterations of this loop.
#define	KMaxMessagesInOneGo		16
	for (NKern::LockSystem(); ; NKern::FlashSystem())
		{
		TInt n = Min(wanted, KMaxMessagesInOneGo);
		while (K::MsgInfo.iFreeMessageCount < n)
			if (ExpandMessagePool() != KErrNone)
				goto fail;

		__ASSERT_DEBUG(MessagePoolIntact(), K::Fault(K::EMsgFreeBadPool));
		RMessageK* first = K::MsgInfo.iNextMessage;
		RMessageK* last = first;
		RMessageK* p = first;

		// Count out n message blocks
		__ASSERT_DEBUG(K::MsgInfo.iFreeMessageCount >= n, K::Fault(K::EMsgFreeBadPool));
		for (TInt k = 0; k < n; ++k)
			{
			last = p;
			p->iSession = aSession;
			p->iMsgType = (TUint8)aType;
			p = (RMessageK*)p->iSessionLink.iNext;
			}

		// Add them to the chain we're building
		if (chain)
			last->iSessionLink.iNext = (SDblQueLink*)chain;
		else
			last->iSessionLink.iNext = NULL;
		chain = first;

		// Synchronise the global pool
		K::MsgInfo.iNextMessage = p;
		K::MsgInfo.iFreeMessageCount -= n;
		__ASSERT_DEBUG(MessagePoolIntact(), K::Fault(K::EMsgFreeBadPool));
		if ((wanted -= n) == 0)
			break;
		}

	__ASSERT_DEBUG(MessagePoolIntact(chain, aCount), K::Fault(K::EMsgFreeBadPool));
	NKern::UnlockSystem();

	// Return this chain of message blocks
	return chain;

fail:
	__ASSERT_DEBUG(MessagePoolIntact(), K::Fault(K::EMsgFreeBadPool));
	NKern::UnlockSystem();
	if (chain)
		chain->ReleaseMessagePool(aType, KMaxTInt);
	return NULL;
	}

// Release a chain of at most aMax RMessageK objects back to the
// global pool, resetting their type to EGlobal in the process
// Enter and return with system unlocked and caller in a critical section
void RMessageK::ReleaseMessagePool(enum TMsgType aType, TInt aMax)
	{
	CHECK_PRECONDITIONS(MASK_THREAD_CRITICAL, "RMessageK::ReleaseMessagePool");
	__ASSERT_DEBUG(MessagePoolIntactUnlocked(), K::Fault(K::EMsgFreeBadPool));

	RMessageK* next = this;
	RMessageK* p;
	TInt k = 0;

	do
		{
		p = next;
		next = (RMessageK*)p->iSessionLink.iNext;
		__ASSERT_DEBUG(p->iMsgType == aType, K::Fault(K::EMsgFreeBadPool));
		p->iMsgType = EGlobal;
		}
	while (++k < aMax && next != NULL);

	// The linked list may not be NULL-terminated -- in DEBUG builds, the link
	// may be deliberately set to an invalid value.  Here, we assert that if
	// exit was reached because the count reached zero, rather than because we
	// reached a NULL link, then the next link was in fact just such a value.
#ifndef	KILL_LINK_VALUE
#define KILL_LINK_VALUE ((SDblQueLink*)0xdfdfdfdf)
#endif
	__ASSERT_DEBUG((next == NULL || (k == aMax && next == (TAny*)KILL_LINK_VALUE)), K::Fault(K::EMsgFreeBadPool));
	__ASSERT_DEBUG((k == aMax || aMax == KMaxTInt), K::Fault(K::EMsgFreeBadPool));

	// At the end of the chain
	NKern::LockSystem();
	p->iSessionLink.iNext = (SDblQueLink*)K::MsgInfo.iNextMessage;
	K::MsgInfo.iNextMessage = this;
	K::MsgInfo.iFreeMessageCount += k;
	__ASSERT_DEBUG(MessagePoolIntact(), K::Fault(K::EMsgFreeBadPool));
	NKern::UnlockSystem();
	(void)aType;				// Suppress compiler whinge in non-DEBUG build
	}

// Get a free message from the global pool. Expand the global pool if
// it's empty; but return NULL if that fails to add more messages.
// 
// Enter and return with system lock, may be temporarily released though
RMessageK* RMessageK::GetNextFreeMessage(DSession* aSession)
	{
	CHECK_PRECONDITIONS(MASK_SYSTEM_LOCKED, "RMessageK::GetNextFreeMessage");

	__ASSERT_DEBUG(MessagePoolIntact(), K::Fault(K::EMsgFreeBadPool));
	RMessageK* volatile* p = &K::MsgInfo.iNextMessage;
	RMessageK* m;

	// If there are no free messages, expand the global pool. Keep trying
	// until we successfully get a message or fail with an error.
	while ((m = *p) == NULL)
		if (ExpandMessagePool() != KErrNone)
			return NULL;

	*p = (RMessageK*)m->iSessionLink.iNext;
	m->iSessionLink.iNext = NULL;
	m->iSession = aSession;
	K::MsgInfo.iFreeMessageCount -= 1;

	__ASSERT_DEBUG(MessagePoolIntact(), K::Fault(K::EMsgFreeBadPool));
	__KTRACE_OPT(KSERVER,Kern::Printf("RMessageK::GetNextFreeMessage returns %08x", m));
	return m;
	}

// Enter and return with system unlocked.
void RMessageK::TMsgArgs::ReadDesHeaders(const TInt aArgsPtr[KMaxMessageArguments+1])
	{
	// Copy the args into the structure; the extra word contains the IPC flags
	TInt i;
	for (i = 0; i < KMaxMessageArguments; ++i)
		iArgs[i] = *aArgsPtr++;
	iArgFlags = *aArgsPtr & KAllIpcFlagsMask;

	TInt descFlags = AllDescriptorFlags();
	for (i = 0; descFlags != 0; ++i, descFlags >>= TIpcArgs::KBitsPerType)
		if (descFlags & TIpcArgs::EFlagDes)
			{
			// Read descriptor header in current process.  May take page fault
			TInt r = K::USafeReadAndParseDesHeader((TAny*)iArgs[i], iDesInfo[i]);
			if (r != KErrNone)
				{
				// If the descriptor is bad (unreadable or unknown type), clear
				// the parameter type and mark the descriptor header as invalid
				SetArgUndefined(i);
				iDesInfo[i].Unset();
				}
			}
	}

// Enter and return with system unlocked and caller in a critical section
void UnpinMessageArguments(RMessageK::TPinArray *aPinArray)
	{
	for (TInt i = 0; i < KMaxMessageArguments; ++i)
		{
		if (aPinArray->iPinPtrs[i])
			{
			// This will unpin the pinned memory and set iPinPtrs[i] to NULL.
			M::DestroyVirtualPinObject(aPinArray->iPinPtrs[i]);
			}
		__NK_ASSERT_DEBUG(!aPinArray->iPinPtrs[i]);
		}
	}

// Enter and return with system lock, may be temporarily released though
TInt RMessageK::PinDescriptors(DSession* aSession, TBool aPinningServer)
	{
	CHECK_PRECONDITIONS(MASK_SYSTEM_LOCKED, "RMessageK::PinDescriptors");
	__NK_ASSERT_DEBUG(!iPinArray);				// Free messages should always have this nulled.

	// If none of the args are descriptors, there's obviously nothing to do
	TUint descFlags = iMsgArgs.AllDescriptorFlags();
	if (!descFlags)
		return KErrNone;

	// If neither the server nor the client have requested pinning, there's nothing to do
	TUint argPinFlags = aPinningServer ? ~0u : iMsgArgs.AllPinFlags();
	if (!argPinFlags)
		return KErrNone;

	// The session can only ever belong to one server so ok to release system lock
	// during pinning as need to pin won't change unless the session is closed.
	// Need a reference on the session, though, in case it goes away.
	aSession->TotalAccessInc();
	NKern::ThreadEnterCS();
	NKern::UnlockSystem();
	TInt r = KErrNone;
	TBool anyPins = EFalse;
	TPinArray pinArray = { { 0, 0, 0, 0 } };			// local, copy to heap later if used

	for (TInt i = 0; descFlags != 0; ++i, argPinFlags >>= 1, descFlags >>= TIpcArgs::KBitsPerType)
 		{
		__NK_ASSERT_DEBUG(i < KMaxMessageArguments);	// Should stop after max args processed.

		// Is this arg a descriptor that should be pinned?
		if ((descFlags & TIpcArgs::EFlagDes) && (argPinFlags & 1))
			{
			TDesHeader& desInfo = Descriptor(i);

			// Pin the max length for modifiable descriptors, but only
			// the current length for non-modifiable descriptors.
			TUint pinLength = desInfo.IsWriteable() ? desInfo.MaxLength() : desInfo.Length();
			if (pinLength)
				{
				// This will only create and pin if the descriptor data is paged.
				// An out-of-memory error here means we fail the whole operation.
				r = Kern::CreateAndPinVirtualMemory(pinArray.iPinPtrs[i], desInfo.DataPtr(), pinLength);
				if (pinArray.iPinPtrs[i])
					anyPins = ETrue;
				if (r == KErrNoMemory)
					break;
				if (r != KErrNone)
					{
					// For any other error, clear the parameter type and mark the
					// descriptor header as invalid, so the server will see a problem
					// on access, but then suppress the error so we can continue ...
					iMsgArgs.SetArgUndefined(i);
					desInfo.Unset();
					r = KErrNone;
					}
				}
			}
		}

	if (anyPins && r != KErrNoMemory)
		{
		iPinArray = new TPinArray (pinArray);
		if (!iPinArray)
			r = KErrNoMemory;
		}

	if (r == KErrNoMemory)
		{
		// Failed to pin everything so clean up any pin objects created.
		// This will also unpin any pinned memory.
		UnpinMessageArguments(&pinArray);
		}

	NKern::LockSystem();

	// Remove the access on the session.
	if (aSession->TotalAccessDec() == DObject::EObjectDeleted)
		{
		// This was the last access on the session and it has been deleted
		// so don't access any of its members.
		r = KErrDisconnected;
		}
	NKern::ThreadLeaveCS();
	return r;
	}

// Free a single message back to the pool it belongs in
// Enter and leave with system lock, may be temporarily released though
void RMessageK::Free()
	{
	CHECK_PRECONDITIONS(MASK_SYSTEM_LOCKED, "RMessageK::Free");
	__KTRACE_OPT(KSERVER, Kern::Printf("RMessageK(%08x)::Free(), type %d", this, iMsgType));
	__ASSERT_SYSTEM_LOCK;
	__NK_ASSERT_DEBUG(!IsFree());
	__NK_ASSERT_DEBUG(State()==EFree); // TClientRequest should be free
	__NK_ASSERT_DEBUG(iAccessCount==0);

	DThread* closeThread = 0;
	if (iSession)
		{
		--iSession->iMsgCount;
		iSessionLink.Deque();
		if (--iClient->iIpcCount == 0x80000000u)
			closeThread = iClient;
		}

	// take ownership of any pin objects...
	TPinArray* pinArray = iPinArray;
	iPinArray = NULL;

	// free the message...
	SetFree();
	switch (iMsgType)
		{
		default:
		case EDisc:
			// The Disconnect message is owned by the session and should never be freed
			K::Fault(K::EMsgFreeBadPool);
			break;

		case ESync:
			// The Synchronous message is owned by the thread, so we don't actually free it
			break;

		case ESession:
			if (iSession)
				{
				// Put this message back on the session's freelist
				iSessionLink.iNext = (SDblQueLink*)iSession->iNextFreeMessage;
				iSession->iNextFreeMessage = this;
				break;
				}
			// Session has gone away; return message to global pool instead
			iMsgType = EGlobal;
			/*FALLTHRU*/
		case EGlobal:
			__ASSERT_DEBUG(MessagePoolIntact(), K::Fault(K::EMsgFreeBadPool));
			// Put this message back on the global freelist
			iSessionLink.iNext = (SDblQueLink*)K::MsgInfo.iNextMessage;
			K::MsgInfo.iNextMessage = this;
			K::MsgInfo.iFreeMessageCount += 1;
			__ASSERT_DEBUG(MessagePoolIntact(), K::Fault(K::EMsgFreeBadPool));
			break;
		}

	if (closeThread)
		{
		K::ThreadEnterCS();
		closeThread->AsyncClose();
		K::ThreadLeaveCS();
		}

	if (pinArray)
		{
		// unpin any pinned descriptors and free the pin objects...
		K::ThreadEnterCS();
		UnpinMessageArguments(pinArray);
		delete pinArray;
		K::ThreadLeaveCS();
		}
	}

EXPORT_C DThread* RMessageK::Thread() const
	{
	return iClient;
	}


/********************************************
 * Server control block
 ********************************************/
DServer::DServer()
	{
	}

TInt DServer::Create()
	{
	iMessage = new TServerMessage;
	return iMessage ? KErrNone : KErrNoMemory;
	}

DServer::~DServer()
//
// Destructor
// Note that no-one else will try to access this server in here, since
// the access count will now be zero.
//
	{
	__KTRACE_OPT(KSERVER,Kern::Printf("DServer %O Destruct", this));
	__ASSERT_ALWAYS(iSessionQ.IsEmpty(), K::Fault(K::EServerDestructSessionsRemain));
	__ASSERT_ALWAYS(iDeliveredQ.IsEmpty(), K::Fault(K::EServerDestructMessagesRemain));
	if (iMessage)
		iMessage->Close();
	Kern::SafeClose((DObject*&)iOwningThread,NULL);
	}

TInt DServer::Close(TAny*)
	{
	__KTRACE_OPT(KSERVER,Kern::Printf("DServer %O Close AC=%d", this, AccessCount()));
	TInt r = Dec();
	if (r==1)
		{
		if (iOwningThread)
			{
			Kern::QueueRequestComplete(iOwningThread, iMessage, KErrCancel);
			NKern::LockSystem();
			for (; !iSessionQ.IsEmpty(); NKern::FlashSystem())
				{
				DSession* s = _LOFF(iSessionQ.First(), DSession, iServerLink);
				s->Detach(KErrServerTerminated);
				}
			__ASSERT_ALWAYS(iDeliveredQ.IsEmpty(), K::Fault(K::EServerCloseLeftoverMsg));
#ifdef BTRACE_CLIENT_SERVER
			BTrace4(BTrace::EClientServer,BTrace::EServerDestroy,this);
#endif
			NKern::UnlockSystem();
			}
		K::ObjDelete(this);
		return EObjectDeleted;
		}
	return 0;
	}

#ifndef __MESSAGE_MACHINE_CODED__
void DServer::Receive(TRequestStatus& aStatus, TAny* aMessage)
//
// Receive a message asynchronously.
// Enter and leave with system locked
//
	{

	if (iMessage->SetStatus(&aStatus) != KErrNone)
		K::PanicCurrentThread(EMesAlreadyPending);
	iMessage->iMessagePtr = aMessage;
	if (!iDeliveredQ.IsEmpty())
		{
		RMessageK* m = _LOFF(iDeliveredQ.First()->Deque(), RMessageK, iServerLink);
		Accept(m);
		}
	}
#endif

void DServer::Cancel()
//
// Cancel a message receive request.
// Enter and leave with system locked
//
	{
	Kern::QueueRequestComplete(iOwningThread, iMessage, KErrCancel);
	}

#ifndef __MESSAGE_MACHINE_CODED__
void DServer::Accept(RMessageK* aMsg)
//
// Accepts a message, assumes one is pending.
// Enter and leave with system locked
//
	{
	// set the ACCEPTED state
	aMsg->SetAccepted(iOwningThread->iOwningProcess);
	if (iOwningThread->iMState==DThread::EDead)
		return;	// server has already died
#ifdef BTRACE_CLIENT_SERVER
	BTrace4(BTrace::EClientServer,BTrace::EMessageReceive,aMsg);
#endif
	iMessage->iMessageData = aMsg;
#ifdef KIPC
	if (KDebugNum(KIPC))
		{
		TInt f = aMsg->iFunction;
		if (f == RMessage2::EDisConnect)
			Kern::Printf("MsgAcD: %O->%O", aMsg->iSession, this);
		else
			Kern::Printf("MsgAc: M:%d %O->%O", f, aMsg->iClient, this);
		}
#endif //KIPC
	Kern::QueueRequestComplete(iOwningThread, iMessage, KErrNone);
	}

void DServer::Deliver(RMessageK* aMsg)
//
// Delivers a message to the server.
// Enter and leave with system locked
//
	{
	if (iMessage->IsReady())
		Accept(aMsg);
	else
		aMsg->SetDelivered(iDeliveredQ);
	}
#endif //__MESSAGE_MACHINE_CODED__

TInt DServer::RequestUserHandle(DThread* aThread, TOwnerType aType)
	{
	(void)aType;
	return (aThread->iOwningProcess==iOwningThread->iOwningProcess) ? KErrNone : KErrPermissionDenied;
	}

TInt ExecHandler::ServerCreateWithOptions(const TDesC8* aName, TInt aMode, TInt aRole, TInt aOpts)
//
// Create a server belonging to the current thread. UNPROTECTED exec call.
//
	{
	DThread* t = TheCurrentThread;
	TInt r = KErrNone;
	TKName n;
	if (aName)
		Kern::KUDesGet(n, *aName);
	__KTRACE_OPT(KEXEC, Kern::Printf("Exec::ServerCreate %S", &n));
	TInt nameLen = n.Length();
	if (nameLen && n[0] == KProtectedServerNamePrefix && !Kern::CurrentThreadHasCapability(ECapabilityProtServ, __PLATSEC_DIAGNOSTIC_STRING("Attempt to create a server with a '!' as the first character of the name")))
		K::UnlockedPlatformSecurityPanic();

	// Fully decode & validate the arguments before allocating the server object
	switch (aMode)
		{
		case EIpcSession_Unsharable:
		case EIpcSession_Sharable:
		case EIpcSession_GlobalSharable:
			break;
		default:
			r = KErrArgument;
			break;
		}

	switch (aRole)
		{
		case EServerRole_Default:
			aRole = EServerRole_Standalone;
			break;
		case EServerRole_Standalone:
		case EServerRole_Master:
		case EServerRole_Slave:
			break;
		default:
			r = KErrArgument;
			break;
		}

	TUint pinMode = (aOpts & EServerOpt_PinClientDescriptorsMask);
	aOpts &= ~EServerOpt_PinClientDescriptorsMask;
	switch (pinMode)
		{
		case EServerOpt_PinClientDescriptorsDisable:
			pinMode = EFalse;
			break;
		case EServerOpt_PinClientDescriptorsEnable:
			pinMode = ETrue;
			break;
		case EServerOpt_PinClientDescriptorsDefault:
			pinMode = EFalse;
			if (K::MemModelAttributes & EMemModelAttrDataPaging)
				if (!(t->iOwningProcess->iAttributes & DProcess::EDataPaged))
				{
				// If the platform supports data-paging, and the server process
				// *isn't* data-paged, then enable descriptor pinning by default
#if		0
				// Worried about the impact of this causing new failure modes in
				// existing code - for now we are not going to turn on pinning by
				// default for unpaged servers.
				pinMode = ETrue;
#endif
				}
			break;
		default:
			r = KErrArgument;
			break;
		}

	if (aOpts)
		r = KErrArgument;
	if (r != KErrNone)
		return r;	// An invalid server mode, role or option was specified.

	r = KErrNoMemory;
	NKern::ThreadEnterCS();
	DServer *pS = new DServer;
	if (pS)
		{
		r = pS->Create();
		if (r == KErrNone)
			{
			t->Open();
			pS->iOwningThread = t;
			pS->iSessionType = (TUint8)aMode;
			pS->iServerRole = (TUint8)aRole;
			pS->iPinClientDescriptors = (TUint8)pinMode;
			if (nameLen)
				r = pS->SetName(&n);
#ifdef BTRACE_CLIENT_SERVER
			BTraceN(BTrace::EClientServer, BTrace::EServerCreate, pS, pS->iOwningThread, n.Ptr(), n.Size());
#endif
			}
		if (r == KErrNone)
			{
			r = K::AddObject(pS, EServer);
			if (r == KErrNone)
				r = K::MakeHandle(nameLen ? EOwnerThread : EOwnerProcess, pS);
			}
		if (r < KErrNone)
			pS->Close(NULL);
		}
	NKern::ThreadLeaveCS();
	__KTRACE_OPT(KEXEC, Kern::Printf("Exec::ServerCreate returns %d", r));
	return r;
	}

TInt ExecHandler::ServerCreate(const TDesC8* aName, TInt aMode)
	{
	return ServerCreateWithOptions(aName, aMode, EServerRole_Default, 0);
	}

void DServer::BTracePrime(TInt aCategory)
	{
#ifdef BTRACE_CLIENT_SERVER
	if (aCategory == BTrace::EClientServer || aCategory == -1)
		{
		TKName nameBuf;
		Name(nameBuf);
		BTraceN(BTrace::EClientServer, BTrace::EServerCreate, this, iOwningThread, nameBuf.Ptr(), nameBuf.Size());
		}
#endif
	}


/********************************************
 * Session control block
 ********************************************/
DSession::DSession()
	: iTotalAccessCount(1), iMsgLimit(KMaxMsgLimit)
	{
	}

DSession::~DSession()
//
// Destroy
//
	{
	__KTRACE_OPT(KSERVER,Kern::Printf("DSession::Destruct"));

	// DObject access count and total access count should both be zero
	__ASSERT_ALWAYS(AccessCount()==0 && iTotalAccessCount==0, K::Fault(K::ESessionDestructStillRef));

	// session should already have been unlinked from server
	__ASSERT_ALWAYS(!iServer, K::Fault(K::ESessionDestruct));

	// there should be no messages outstanding on this session
	__ASSERT_ALWAYS(iMsgCount==0, K::Fault(K::ESessionDestructMsgCount));
	__ASSERT_ALWAYS(iMsgQ.IsEmpty(), K::Fault(K::ESessionDestructMsgQ));

	if (iDisconnectMsgPtr)
		{
		__KTRACE_OPT(KSERVER,Kern::Printf("DSession::~DSession(%08X) releasing disconnect message at %08X", this, iDisconnectMsgPtr));
		iDisconnectMsgPtr->ReleaseMessagePool(RMessageK::EDisc, 1);
		}
	iDisconnectMsgPtr = NULL;
	if (iNextFreeMessage)
		{
		__KTRACE_OPT(KSERVER,Kern::Printf("DSession::~DSession(%08X) releasing session pool at %08X", this, iNextFreeMessage));
		__ASSERT_DEBUG(iHasSessionPool, K::Fault(K::ESessionDestructMsgCount));
		__ASSERT_DEBUG(iPoolSize > 0, K::Fault(K::ESessionDestructMsgCount));
		iNextFreeMessage->ReleaseMessagePool(RMessageK::ESession, iPoolSize);
		}
	iNextFreeMessage = NULL;
	}


/**
Decrement the total access count, deleting the session object if this is the last
reference on the session.

Note - This may release the system lock temporarily.

@return 0 if the session still exists, EObjectDeleted if the session has been deleted.

@pre Enter with system locked
@post Leave with system locked
*/
TInt DSession::TotalAccessDec()
	{
	__ASSERT_SYSTEM_LOCK;
	TInt r = 0;
	if (--iTotalAccessCount == 0)
		{// The session must have been closed.
		__NK_ASSERT_DEBUG(AccessCount()==0);
		NKern::UnlockSystem();
		K::ObjDelete(this);
		r = EObjectDeleted;
		NKern::LockSystem();
		}
	return r;
	}

/**
As DSession::TotalAccessDec() except it returns with the system lock released.

@pre Enter with system locked
@post Leave with system unlocked
*/
TInt DSession::TotalAccessDecRel()
	{
	__ASSERT_SYSTEM_LOCK;
	TInt r = 0;
	TUint tac = --iTotalAccessCount;
	NKern::UnlockSystem();

	if (tac == 0)
		{// The session must have been closed.
		__NK_ASSERT_DEBUG(AccessCount()==0);
		K::ObjDelete(this);
		r = EObjectDeleted;
		}
	return r;
	}


void DSession::Transfer(DServer* aServer, RMessageK* aMsg)
//
// Enter and leave with system locked
//
	{
	(void)aMsg;	// avoid non-DEBUG whinge about unused parameter
	__KTRACE_OPT(KSERVER, Kern::Printf("Session %O Transfer From %O To %O, msg %08x",
		this, iServer, aServer, aMsg));

	// The current server must be a master, and the new one a designated slave
	if (iServer->iServerRole != EServerRole_Master)
		K::PanicCurrentThread(ESessionInvalidCookieMsg);
	if (aServer->iServerRole != EServerRole_Slave)
		K::PanicCurrentThread(ESessionInvalidCookieMsg);

	// Both servers must be part of the same process
	if (iServer->iOwningThread->iOwningProcess != aServer->iOwningThread->iOwningProcess)
		K::PanicCurrentThread(ESessionInvalidCookieMsg);

	// Transfers must not involve moribund servers
	if (iServer->IsClosing() || aServer->IsClosing())
		K::PanicCurrentThread(ESessionInvalidCookieMsg);
	if (iServer->iOwningThread->iMState == DThread::EDead)
		K::PanicCurrentThread(ESessionInvalidCookieMsg);
	if (aServer->iOwningThread->iMState == DThread::EDead)
		K::PanicCurrentThread(ESessionInvalidCookieMsg);

	// Unlink from the current server, and link to the new one
	iServerLink.Deque();
	iServer = aServer;
	iServer->iSessionQ.Add(&iServerLink);

	// The message queue must contain at least the Connect message
	__ASSERT_DEBUG(!iMsgQ.IsEmpty(),
					K::Fault(K::EMsgFreeBadPool));

	RMessageK* connectMsg = _LOFF(iMsgQ.First(), RMessageK, iSessionLink);
	// RMessageK* lastMsg = _LOFF(iMsgQ.Last(), RMessageK, iSessionLink);

	__ASSERT_DEBUG(connectMsg == aMsg,
					K::Fault(K::EKernelMsgNotAccepted));
	__ASSERT_DEBUG(connectMsg->iFunction == RMessage2::EConnect,
					K::Fault(K::EKernelMsgNotAccepted));
	__ASSERT_DEBUG(connectMsg->IsAccepted(),
					K::Fault(K::EKernelMsgNotAccepted));

	// Transfer all pending messages except the first (which is the Connect)
	SDblQueLink* p;
	TInt msgCount = 0;
	for (RMessageK* m = connectMsg; (p = m->iSessionLink.iNext) != &iMsgQ.iA; )
		{
		msgCount += 1;
		m = _LOFF(p, RMessageK, iSessionLink);
		__ASSERT_DEBUG(!m->IsAccepted(),
						K::Fault(K::EMessageAlreadyPending));
		if (m->IsDelivered())
			m->iServerLink.Deque();
		iServer->Deliver(m);
		}

	// Transfer the disconnect message too?
	if (iDisconnectMsgPtr->IsDelivered())
		{
		__ASSERT_DEBUG(!iDisconnectMsgPtr->IsAccepted(),
						K::Fault(K::EMessageAlreadyPending));
		iDisconnectMsgPtr->iServerLink.Deque();
		__SendDiscMsg(this);
		}
	}

// Detach a session from the server
// Called either when server terminates or when server completes disconnect message
// Enter and leave with system locked
//
void DSession::Detach(TInt aReason)
	{
	__KTRACE_OPT(KSERVER,Kern::Printf("Session %O Detach From %O reason %d", this, iServer, aReason));
	iServer = NULL;
	iServerLink.Deque();

	if (iDisconnectMsgPtr->IsDelivered())
		iDisconnectMsgPtr->iServerLink.Deque();
	iDisconnectMsgPtr->SetFree();

	for (; !iMsgQ.IsEmpty(); NKern::LockSystem())
		{
		--iMsgCount;
		RMessageK* m = _LOFF(iMsgQ.First()->Deque(), RMessageK, iSessionLink);
		__ASSERT_ALWAYS(m->iSession == this, K::Fault(K::EInvalidSessionAccessCount));
		m->iSession = NULL;		// message now detached from session
		if (m->iMsgType == RMessageK::ESession)
			iPoolSize -= 1;		// one less message in the pool

		// Note whether session connect message has been discarded
		if (m->iFunction == RMessage2::EConnect)
			iConnectMsgPtr = NULL;
		if (m->IsDelivered())
			m->iServerLink.Deque();

		DThread* t = m->iClient;
		TUint32 c = --t->iIpcCount;

		if(m->IsDelivered() || m->IsAccepted())
			{
			if (!IsClosing() && t->iMState != DThread::EDead)
				{
				m->SetCompleting();
				Kern::QueueRequestComplete(t, m, aReason);
				}
			else
				{
				m->Reset();
				m->CloseRef();
				}
			}

		NKern::UnlockSystem();
		if (c == 0x80000000u)
			t->AsyncClose();
		}

	// Output total access count here before the session object may potentially 
	// be deleted.  Give the value after the detach has completed.
	__KTRACE_OPT(KSERVER,Kern::Printf("TAC: %d", iTotalAccessCount - 1));
#ifdef BTRACE_CLIENT_SERVER
	BTrace8(BTrace::EClientServer,BTrace::ESessionDetach,this,aReason);
#endif
	// This may temporarily release the system lock if it deletes the session.
	// Don't access any members after this call as the session may be deleted.
	TotalAccessDec();
	}

void DSession::CloseFromDisconnect()
//
// Called when server completes the disconnect message
// Enter and leave with system locked
//
	{
	__KTRACE_OPT(KIPC,Kern::Printf("MsgCoD: %O->%O",TheCurrentThread,this));
	__KTRACE_OPT(KTHREAD,Kern::Printf("DSession::CloseFromDisconnect AC:%d TAC:%d", AccessCount(), iTotalAccessCount));
	__ASSERT_ALWAYS(AccessCount()==0, K::Fault(K::EInvalidSessionAccessCount));

	// there should not be any DELIVERED messages outstanding
	// there may be outstanding ACCEPTED messages
	// no other thread can access the session here

	NKern::ThreadEnterCS();
	iDisconnectMsgPtr->SetFree();
	if (iServer && !iServer->IsClosing())
		Detach(KErrSessionClosed);
	// otherwise server is closing, so let that handle it
	NKern::ThreadLeaveCS();
	}

TInt DSession::Close(TAny*)
//
// Session close called from client side.
// Enter and leave with system unlocked.
//
	{
	__KTRACE_OPT(KTHREAD,Kern::Printf("DSession::Close %O AC:%d TAC:%d", this, AccessCount(), iTotalAccessCount));
	if (Dec() == 1)
		{
		// Last client access has been closed. Only the server may now influence this session.
		// Thus no more messages can be added to the session's queue.

		NKern::LockSystem();

		if (!iDisconnectMsgPtr)
			{
			// Allocation of disconnect message failed during session create; there's no cleanup to do
			}
		else if (!iDisconnectMsgPtr->IsFree())
			{
			// Disconnect message has already been sent and is pending completion by the server
			}
		else if (iSessionCookie)
			{
			// Deliver a disconnect message now if a user-side session has already been created
			__SendDiscMsg(this);
			}
		else if (!iConnectMsgPtr)
			{
			// Deliver a disconnect message now if there is no connect message outstanding
			__SendDiscMsg(this);
			}
		else if (iConnectMsgPtr->IsDelivered())
			{
			// We'll remove the pending message, to prevent it from being accepted and creating
			// an orphan session object. Then, completing the connect message will result in a
			// disconnect message being sent, as this is equivalent to the case where a connect
			// message was accepted but then subsequently rejected.
			iConnectMsgPtr->iServerLink.Deque();
			iConnectMsgPtr->SetCompleting();
			ExecHandler::MessageComplete(iConnectMsgPtr, KErrSessionClosed);
			}
		else
			{
			// There's an accepted connect message, but no user session (yet). However
			// one could still be created, so delay sending the disconnect message.
			// It will be sent from SetSessionPtr() or MessageComplete() instead.
			}

		// Remove client contribution to the total access count.  This may 
		// delete the object so don't access any of its members after this call.
		// This will release the system lock.
		return TotalAccessDecRel();
		}

	return 0;
	}

// Enter and return with system unlocked and caller in a critical section
TInt DSession::New(DSession*& aS, TInt aMsgSlots, TInt aMode)
	{
	__KTRACE_OPT(KEXEC,Kern::Printf("DSession::New MS:%d Mode %d", aMsgSlots, aMode));
	if (aMsgSlots<-1 || aMsgSlots>KMaxMsgLimit)
		return KErrArgument;
	if ( (TUint)aMode > (TUint)EIpcSession_GlobalSharable)
		return KErrArgument;
	aS = new DSession;
	if (!aS)
		return KErrNoMemory;

	RMessageK* mp = RMessageK::ClaimMessagePool(RMessageK::EDisc, 1, aS);
	__KTRACE_OPT(KSERVER,Kern::Printf("DSession::New(%08X) claimed disconnect message at %08X", aS, mp));
	if (!mp)
		return KErrNoMemory;
	mp->iFunction = RMessage2::EDisConnect;
	aS->iDisconnectMsgPtr = mp;

	aS->iSessionType = (TUint8)aMode;
	if (aMsgSlots == 0)
		{
		aS->iHasSessionPool = 1;			// pretend it has its own pool
		aS->iNextFreeMessage = NULL;		// but never any free messages
		}
	else if (aMsgSlots > 0)
		{
		aS->iHasSessionPool = 1;			// remember that it has its own pool
		aS->iNextFreeMessage = RMessageK::ClaimMessagePool(RMessageK::ESession, aMsgSlots, aS);
		__KTRACE_OPT(KSERVER,Kern::Printf("DSession::New(%08X) claimed %d messages at %08X", aS, aMsgSlots, aS->iNextFreeMessage));
		if (!aS->iNextFreeMessage)
			return KErrNoMemory;
		aS->iPoolSize = aMsgSlots;			// remember that it has its own pool
		}
	return KErrNone;
	}

// Add a new session to a server
// Enter with system locked, return with system unlocked
TInt DSession::Add(DServer* aSvr, const TSecurityPolicy* aSecurityPolicy)
	{
	__KTRACE_OPT(KSERVER,Kern::Printf("Session %O Add to server %O", this, aSvr));
	if (aSvr->IsClosing())
		{
		NKern::UnlockSystem();
		return KErrServerTerminated;
		}
	if (aSecurityPolicy && !aSecurityPolicy->CheckPolicy(aSvr->iOwningThread,__PLATSEC_DIAGNOSTIC_STRING("Checked during RSessionBase::CreateSession")))
		{
		NKern::UnlockSystem();
		return KErrPermissionDenied;
		}
	iSvrSessionType = aSvr->iSessionType;
	if (iSessionType > iSvrSessionType)
		{
		NKern::UnlockSystem();
		return iSvrSessionType ? KErrPermissionDenied : KErrAccessDenied;
		}
	iServer = aSvr;
	aSvr->iSessionQ.Add(&iServerLink);		// Add it to the server
	TotalAccessInc();						// give the server an access on this session

	DObject* owner = NULL;
	if (iSvrSessionType == EIpcSession_Unsharable)
		owner = TheCurrentThread;
	else if (iSvrSessionType == EIpcSession_Sharable)
		owner = TheCurrentThread->iOwningProcess;

#ifdef BTRACE_CLIENT_SERVER
	BTraceContext12(BTrace::EClientServer, BTrace::ESessionAttach, this, iServer, owner);
#endif
	NKern::UnlockSystem();					// server could be deleted after this line

	if (owner)
		SetOwner(owner);
	if (iSessionType == EIpcSession_GlobalSharable)
		SetProtection(EProtected);
	return KErrNone;
	}

TInt DSession::MakeHandle()
	{
	TInt r = K::AddObject(this, ESession);
	if (r==KErrNone)
		{
		TOwnerType htype = iSessionType == EIpcSession_Unsharable ? EOwnerThread : EOwnerProcess;
		r = K::MakeHandle(htype, this);
		}
	return r;
	}

TInt DSession::RequestUserHandle(DThread* aThread, TOwnerType aType)
	{
	if (iSvrSessionType == EIpcSession_Unsharable)
		return (aThread==Owner() && aType==EOwnerThread) ? KErrNone : KErrPermissionDenied;
	if (iSvrSessionType == EIpcSession_Sharable)
		return (aThread->iOwningProcess==Owner()) ? KErrNone : KErrPermissionDenied;
	return KErrNone;
	}

TInt ExecHandler::SessionCreate(const TDesC& aServer, TInt aMsgSlots, const TSecurityPolicy* aSecurityPolicy, TInt aMode)
//
// Create a new session. UNPROTECTED exec call.
//
	{
	TKName n;
	TKName svrName;
	DServer* svr = NULL;
	DObjectCon& servers = *K::Containers[EServer];
	Kern::KUDesGet(n, aServer);

	TSecurityPolicy policy;
	if(aSecurityPolicy)
		{
		kumemget32(&policy,aSecurityPolicy,sizeof(policy));
		if(!policy.Validate())
			return KErrArgument;
		aSecurityPolicy=&policy;
		}

	__KTRACE_OPT(KEXEC,Kern::Printf("Exec::SessionCreate %S MS:%d Mode %d", &n, aMsgSlots, aMode));
	NKern::ThreadEnterCS();
	DSession* s=NULL;
	TInt r = DSession::New(s, aMsgSlots, aMode);
	if (r==KErrNone)
		{
		servers.Wait();
		TFindHandle fh;
		r = servers.FindByName(fh, n, svrName);
		if (r==KErrNone)
			{
			svr = (DServer*)servers.At(fh);	// can't return NULL since we just found the server
			NKern::LockSystem();
			r = s->Add(svr, aSecurityPolicy);
			}
		servers.Signal();
		}
	if (r==KErrNone)
		r = s->MakeHandle();
	if (r<KErrNone && s)
		s->Close(NULL);
	NKern::ThreadLeaveCS();
	__KTRACE_OPT(KEXEC,Kern::Printf("Exec::SessionCreate returns %d",r));
	if (r==KErrAccessDenied)
		K::PanicKernExec(EUnsharableSession);
	return r;
	}

TInt ExecHandler::SessionCreateFromHandle(TInt aSvrHandle, TInt aMsgSlots, const TSecurityPolicy* aSecurityPolicy, TInt aMode)
//
// Create a new session to the given server.
// Enter and return with system unlocked.
//
	{
	TSecurityPolicy policy;
	if(aSecurityPolicy)
		{
		kumemget32(&policy,aSecurityPolicy,sizeof(policy));
		if(!policy.Validate())
			return KErrArgument;
		aSecurityPolicy=&policy;
		}

	__KTRACE_OPT(KEXEC,Kern::Printf("Exec::SessionCreate %08x MS:%d Mode %d", aSvrHandle, aMsgSlots, aMode));
	NKern::ThreadEnterCS();
	DSession* s;
	DThread* t = TheCurrentThread;
	TInt r = DSession::New(s, aMsgSlots, aMode);
	if (r==KErrNone)
		{
		NKern::LockSystem();
		DServer* svr = (DServer*)t->ObjectFromHandle(aSvrHandle, EServer);
		if (svr)
			r = s->Add(svr, aSecurityPolicy);
		else
			{
			r = KErrBadHandle;
			NKern::UnlockSystem();
			}
		}
	if (r==KErrNone)
		r = s->MakeHandle();
	if (r<KErrNone && s)
		s->Close(NULL);
	NKern::ThreadLeaveCS();
	__KTRACE_OPT(KEXEC,Kern::Printf("Exec::SessionCreate returns %d",r));
	if (r==KErrAccessDenied)
		K::PanicKernExec(EUnsharableSession);
	if (r==KErrBadHandle)
		K::PanicKernExec(EBadHandle);
	return r;
	}

// If this session doesn't have its own pool, get a free message from the
// global pool. This will attempt to expand the pool if necessary, and may
// temporarily release the system lock.
//
// If this session does have its own message pool, get a free message from
// it; if there are none, just return NULL, without attempting to expand it
// or releasing the system lock.
// 
// Enter and return with system lock, may be temporarily released though
RMessageK* DSession::GetNextFreeMessage()
	{
	CHECK_PRECONDITIONS(MASK_SYSTEM_LOCKED, "DSession::GetNextFreeMessage");

	if (!iHasSessionPool)
		return RMessageK::GetNextFreeMessage(this);

	RMessageK* volatile* p = &iNextFreeMessage;
	RMessageK* m = *p;
	if (m)
		{
		*p = (RMessageK*)m->iSessionLink.iNext;
		m->iSessionLink.iNext = NULL;
		m->iSession = this;
		}
	__KTRACE_OPT(KSERVER,Kern::Printf("DSession::GetNextFreeMessage returns %08x", m));
	return m;
	}

TInt DSession::Send(TInt aHandle, TInt aFunction, const TInt* aPtr, TRequestStatus* aStatus)
//
// Send a message to a server. Assumes aStatus
// has already been set to KRequestPending.
// Enter and return with system unlocked.
//
	{
	if (aFunction == RMessage2::EDisConnect)
		return KErrArgument;

	RMessageK::TMsgArgs msgArgs;
	if (aPtr)
		msgArgs.ReadDesHeaders(aPtr);

	NKern::LockSystem();
	DSession* session = (DSession*)K::ObjectFromHandle(aHandle, ESession);
	RMessageK* m = session->GetNextFreeMessage();
	if (!m)
		{
		NKern::UnlockSystem();
		return KErrServerBusy;
		}
	__ASSERT_DEBUG(m->IsFree(), K::Fault(K::EMessageNotFree));
	return session->Send(m, aFunction, aPtr ? &msgArgs : NULL, aStatus);
	}

TInt DSession::SendSync(TInt aHandle, TInt aFunction, const TInt* aPtr, TRequestStatus* aStatus)
//
// Send a SYNCHRONOUS message to a server using current thread's
// dedicated synchronous message. Assumes aStatus has already been set to KRequestPending.
// Enter and return with system unlocked.
//
	{
	if (aFunction == RMessage2::EDisConnect)
		return KErrArgument;

	RMessageK::TMsgArgs msgArgs;
	if (aPtr)
		msgArgs.ReadDesHeaders(aPtr);

	NKern::LockSystem();
	DSession* session = (DSession*)K::ObjectFromHandle(aHandle, ESession);
	RMessageK* m = TheCurrentThread->iSyncMsgPtr;
	__ASSERT_ALWAYS(m->IsFree(), K::PanicCurrentThread(ESyncMsgSentTwice));
	TInt r = session->Send(m, aFunction, aPtr ? &msgArgs : NULL, aStatus);
	NKern::YieldTimeslice();
	return r;
	}

TInt DSession::Send(RMessageK* aMsg, TInt aFunction, const RMessageK::TMsgArgs* aArgs, TRequestStatus* aStatus)
//
// Send a message to a server.
// Enter with system locked, return with system unlocked.
//
	{
	TInt panicReason = KErrNone;
	TInt r = KErrNone;

	__NK_ASSERT_DEBUG(aMsg->IsFree());

	aMsg->OpenRef();
	aMsg->SetInitialising();
	aMsg->iSession = this;
	aMsg->iClient = TheCurrentThread;
	TheCurrentThread->iIpcCount += 1;
	iMsgQ.Add(&aMsg->iSessionLink);
	if (++iMsgCount > iMsgLimit)
		{
		r = KErrOverflow;
		goto error;
		}

	// Check server is still alive ...
	if (!iServer || iServer->IsClosing())
		{
		r = KErrServerTerminated;
		goto error;
		}

	if (aFunction == RMessage2::EConnect)
		{
		// Only one connect message is allowed at once
		if (iConnectMsgPtr)
			{
			panicReason = ERequestAlreadyPending;
			goto error;
			}

		// Further connect messages not allowed after successful session creation
		if (iSessionCookie)
			{
			panicReason = ESessionAlreadyConnected;
			goto error;
			}

		// Keep track of connect message
		iConnectMsgPtr = aMsg;
		}

	// Set message contents
	aMsg->iFunction = aFunction;
	if (aArgs == NULL)
		{
		aMsg->iMsgArgs.iArgFlags = 0;
		aMsg->iMsgArgs.iArgs[0] = 0;
		aMsg->iMsgArgs.iArgs[1] = 0;
		aMsg->iMsgArgs.iArgs[2] = 0;
		aMsg->iMsgArgs.iArgs[3] = 0;
		}
	else
		{
		aMsg->iMsgArgs = *aArgs;

		// Attempt to pin the descriptors if required.  This may release the system lock temporarily
		r = aMsg->PinDescriptors(this, iServer->iPinClientDescriptors);
		if (r != KErrNone)
			goto error;

		if (!iServer || iServer->IsClosing())
			{
			// Server went away while we weren't holding the system lock
			r = KErrServerTerminated;
			goto error;
			}
		}

	// don't release system lock again after here until message is deliver/accepted
	// else session may have become detached

#ifdef BTRACE_CLIENT_SERVER
	BTraceContext12(BTrace::EClientServer,BTrace::EMessageSend,aMsg,aFunction,this);
#endif

	// NB: aStatus is NULL for blind messages
	r = aMsg->SetStatus(aStatus);
	__ASSERT_DEBUG(r == KErrNone, K::Fault(K::EMessageInUse));
	iServer->Deliver(aMsg);
	NKern::UnlockSystem();
	return r;

error:
	aMsg->CloseRef();
	if (panicReason)
		K::PanicCurrentThread(panicReason);		// doesn't return!
	NKern::UnlockSystem();
	return r;
	}

void DSession::BTracePrime(TInt aCategory)
	{
#ifdef BTRACE_CLIENT_SERVER
	if (aCategory == BTrace::EClientServer || aCategory == -1)
		{
		BTrace12(BTrace::EClientServer, BTrace::ESessionAttach, this, iServer, iOwner);
		}
#endif
	}


void ExecHandler::SetSessionPtr(RMessageK* aMsg, const TAny* aSessionCookie)
//
// Enter with system locked, return with system unlocked
//
	{
	__KTRACE_OPT(KEXEC,Kern::Printf("Exec::SetSessionPtr"));

	// Session cookie must not be set to NULL
	if (!aSessionCookie)
		K::PanicCurrentThread(ESessionNullCookie);

	// Session cookie must be set from the connect message
	if (aMsg->iFunction != RMessage2::EConnect)
		K::PanicCurrentThread(ESessionInvalidCookieMsg);

	DSession* pS = aMsg->iSession;

	// Must not attempt to set the cookie more than once
	if (pS->iSessionCookie)
		K::PanicCurrentThread(ESessionCookieAlreadySet);

	pS->iSessionCookie = aSessionCookie;

	if (pS->IsClosing() && pS->iDisconnectMsgPtr->IsFree())
		{
		// NB: pS->iServer is set to NULL during CloseFromDisconnect(), called
		// when a disconnect message has been completed (and hence is free again),
		// so this cannot send the disconnect message again.
		__SendDiscMsg(pS);
		}
	NKern::UnlockSystem();
	}

void ExecHandler::TransferSession(RMessageK* aMsg, TInt aHandle)
//
// Enter and return with system locked
//
	{
	__KTRACE_OPT(KEXEC, Kern::Printf("Exec::TransferSession"));

	// Session transfer must done using the Connect message
	if (aMsg->iFunction != RMessage2::EConnect)
		K::PanicCurrentThread(ESessionInvalidCookieMsg);

	// Session cookie must already have been set
	DSession* pSession = aMsg->iSession;
	if (pSession->iSessionCookie == NULL)
		K::PanicCurrentThread(ESessionNullCookie);

	// Find the new server and transfer the session to it
	DServer* pServer = (DServer*)K::ObjectFromHandle(aHandle, EServer);
	pSession->Transfer(pServer, aMsg);

	// This call also completes the original message with no error
	ExecHandler::MessageComplete(aMsg, KErrNone);
	}

void ExecHandler::ServerReceive(DServer* aServer, TRequestStatus& aStatus, TAny* aMsg)
//
// Enter and leave with system locked
//
	{
	__KTRACE_OPT(KEXEC,Kern::Printf("Exec::ServerReceive"));
	aServer->Receive(aStatus, aMsg);
	}

void ExecHandler::ServerCancel(DServer* aServer)
//
// Enter and leave with system locked
//
	{
	__KTRACE_OPT(KEXEC,Kern::Printf("Exec::ServerCancel"));
	aServer->Cancel();
	}

TInt ExecHandler::SessionSend(TInt aHandle, TInt aFunction, TAny* aPtr, TRequestStatus* aStatus)
//
// Enter with system locked, return with system unlocked.
//
	{
	__KTRACE_OPT(KEXEC,Kern::Printf("Exec::SessionSend"));
	return DSession::Send(aHandle, aFunction, (const TInt*)aPtr, aStatus);
	}

TInt ExecHandler::SessionSendSync(TInt aHandle, TInt aFunction, TAny* aPtr, TRequestStatus* aStatus)
//
// Enter with system locked, return with system unlocked.
//
	{
	__KTRACE_OPT(KEXEC,Kern::Printf("Exec::SessionSendSync"));
	return DSession::SendSync(aHandle, aFunction, (const TInt*)aPtr, aStatus);
	}

#ifndef __MESSAGE_MACHINE_CODED__
void ExecHandler::MessageComplete(RMessageK* aMsg, TInt aReason)
//
// Enter and leave with system locked.
//
	{
	__KTRACE_OPT(KEXEC,Kern::Printf("Exec::MessageComplete"));
	RMessageK& m = *aMsg;

#ifdef BTRACE_CLIENT_SERVER
	BTraceContext8(BTrace::EClientServer,BTrace::EMessageComplete,aMsg,aReason);
#endif

	DSession* s = m.iSession;

	// First check for disconnect message
	if (m.iFunction == RMessage2::EDisConnect)
		{
		s->CloseFromDisconnect();
		return;
		}

	// Note whether session connect message has completed
	if (m.iFunction == RMessage2::EConnect)
		s->iConnectMsgPtr = NULL;

	__KTRACE_OPT(KIPC,Kern::Printf("MsgCo: M:%d r:%d %O->%O", m.iFunction, aReason, TheCurrentThread, m.iClient));
	if (!s->IsClosing() && m.iClient->iMState != DThread::EDead)
		{
		m.SetCompleting();
		Kern::QueueRequestComplete(m.iClient, &m, aReason);
		}
	else
		{
		// Pending disconnect, is it a connect message?
		if(m.iFunction == RMessage2::EConnect)
			{
			// If a session has been created, then a disconnect message should already have been sent
			__ASSERT_DEBUG(!s->iSessionCookie || !s->iServer || !s->iDisconnectMsgPtr->IsFree(),
							K::Fault(K::EMsgCompleteDiscNotSent));

			// if no server-side session object to clean up, send disconnect message
			// anyway to prevent message lifetime issues for the server for any other
			// messages on this unconnected session the server may have accepted
			if (!s->iSessionCookie)
				__SendDiscMsg(s);
			}
		m.Reset();
		m.CloseRef();   // Return message to appropriate pool
		}
	}
#else
#ifdef _DEBUG
extern "C" void __FaultBadMsgPool()
	{
	K::Fault(K::EMsgFreeBadPool);
	}

extern "C" void __FaultMsgNotFree()
	{
	K::Fault(K::EMessageNotFree);
	}

extern "C" void __FaultMsgInUse()
	{
	K::Fault(K::EMessageInUse);
	}

extern "C" void __FaultMsgCompleteDiscNotSent()
	{
	K::Fault(K::EMsgCompleteDiscNotSent);
	}
#endif

extern "C" void __PanicSyncMsgSentTwice()
	{
	K::PanicCurrentThread(ESyncMsgSentTwice);
	}

extern "C" void __PanicMesAlreadyPending()
	{
	K::PanicCurrentThread(EMesAlreadyPending);
	}

#endif

//
// Enter and leave with system locked
//
extern "C" void __SendDiscMsg(DSession* aSession)
	{
	DServer* server = aSession->iServer;
	if (server && !server->IsClosing())
		{
		// Send the preallocated disconnect message, as the session is still attached
		// and the server isn't closing so it won't detach the session itself.
		RMessageK* disc = aSession->iDisconnectMsgPtr;
		__ASSERT_DEBUG(disc->iSession == aSession && disc->IsFree(),
						K::Fault(K::EMsgCompleteDiscNotSent));

#ifdef BTRACE_CLIENT_SERVER
		BTraceContext12(BTrace::EClientServer,BTrace::EMessageSend,disc,disc->iFunction,aSession);
#endif
		disc->iSession = aSession;
		server->Deliver(disc);
		}
	}

TInt ExecHandler::SessionShare(TInt& aHandle, TInt aMode)
//
// Mutate a single-threaded session into a multi-threaded session
//
	{
	if ( (TUint)aMode > (TUint)EIpcSession_GlobalSharable || aMode == EIpcSession_Unsharable)
		return KErrArgument;
	TInt oldHandle;
	TInt newHandle = 0;
	kumemget32(&oldHandle, &aHandle, sizeof(TInt));
	NKern::LockSystem();
	DSession* s = (DSession*)K::ObjectFromHandle(oldHandle, ESession);
	s->CheckedOpen();
	__KTRACE_OPT(KEXEC,Kern::Printf("Exec::SessionShare %O server %O mode %08x", s, s->iServer, aMode));
	TInt r = KErrNone;
	TUint ct = s->iSessionType;
	if (s->iSvrSessionType == EIpcSession_Unsharable)
		{
		r = EUnsharableSession;
		goto end;
		}
	if ((TUint)s->iSvrSessionType < (TUint)aMode)
		{
		r = KErrPermissionDenied;
		goto end;
		}
	if (aMode == EIpcSession_GlobalSharable)
		s->SetProtection(DObject::EProtected);
	if (ct >= (TUint)aMode)
		goto end;	// nothing to do
	s->iSessionType = (TUint8)aMode;
	if (ct >= EIpcSession_Sharable)
		goto end;	// nothing more to do
	K::ThreadEnterCS();
	r = K::MakeHandle(EOwnerProcess, s);
	if (r>=0)
		{
		newHandle = r;
		r = KErrNone;
		K::HandleClose(oldHandle);
		if (Kern::KUSafeWrite(&aHandle, &newHandle, sizeof(newHandle)))	// don't let thread die before updating handle
			r = ECausedException;
		}
	if (r==KErrNone)
		{
		NKern::ThreadLeaveCS();
		__KTRACE_OPT(KEXEC,Kern::Printf("Exec::SessionShare returns %d",r));
		return r;
		}
	goto end2;
end:
	K::ThreadEnterCS();
end2:
	s->Close(NULL);
	NKern::ThreadLeaveCS();
	if (r>0)
		K::PanicKernExec(r);
	__KTRACE_OPT(KEXEC,Kern::Printf("Exec::SessionShare returns %d",r));
	return r;
	}

#ifndef __MESSAGE_MACHINE_CODED_2__
void ExecHandler::MessageConstructFromPtr(RMessageK* aMsgK, TAny* aMsgU)
	{
	__KTRACE_OPT(KEXEC,Kern::Printf("Exec::MessageConstructFromPtr"));
	RMessageU2 umsg(*aMsgK);
	NKern::UnlockSystem();
	kumemput32(aMsgU, &umsg, sizeof(umsg));
	}
#endif

TInt ExecHandler::MessageGetDesLength(RMessageK* aMsg, TInt aParam)
	{
	__KTRACE_OPT(KEXEC, Kern::Printf("Exec::MessageGetDesLength"));
	if (TUint(aParam) >= TUint(KMaxMessageArguments))
		return KErrArgument;
	if (!aMsg->IsDescriptor(aParam))
		return KErrBadDescriptor;
	return aMsg->DesLength(aParam);
	}

TInt ExecHandler::MessageGetDesMaxLength(RMessageK* aMsg, TInt aParam)
	{
	__KTRACE_OPT(KEXEC,Kern::Printf("Exec::MessageGetDesMaxLength"));
	if (TUint(aParam) >= TUint(KMaxMessageArguments))
		return KErrArgument;
	if (!aMsg->IsDescriptor(aParam))
		return KErrBadDescriptor;
	return aMsg->DesMaxLength(aParam);
	}

const TRequestStatus* ExecHandler::MessageClientStatus(RMessageK* aMsg)
	{
	__KTRACE_OPT(KEXEC,Kern::Printf("Exec::MessageClientStatus"));
	if (aMsg->iMsgType == RMessageK::ESync)
		return NULL;
	return aMsg->StatusPtr();
	}

#ifndef __REMOVE_PLATSEC_DIAGNOSTICS__
#define __PSIF(msg, diag)	PlatSec::ProcessIsolationIPCFail(msg, diag)
#else //__REMOVE_PLATSEC_DIAGNOSTICS__
#define __PSIF(msg, diag)	PlatSec::EmitDiagnostic()
#endif // !__REMOVE_PLATSEC_DIAGNOSTICS__

TInt ExecHandler::MessageIpcCopy(RMessageK* aMsg, TInt aParam, SIpcCopyInfo& aInfo, TInt aOffset)
	{
	__KTRACE_OPT(KEXEC,Kern::Printf("Exec::MessageIpcCopy flags=%08x",aInfo.iFlags));
	if (TUint(aParam) >= TUint(KMaxMessageArguments))
		return KErrArgument;
	if (!aMsg->IsDescriptor(aParam) || !aMsg->Descriptor(aParam).IsSet())
		{
		__PSIF(aMsg, __PLATSEC_DIAGNOSTIC_STRING("Server attempted to use RMessagePtr2::Read/Write on a non descriptor message argument"));
		return KErrBadDescriptor;
		}

	TInt argType = aMsg->ArgType(aParam);
	if ((aInfo.iFlags & KIpcDirWrite) && (argType & TIpcArgs::EFlagConst))
		if (__PSIF(aMsg, __PLATSEC_DIAGNOSTIC_STRING("Server attempted to use RMessagePtr2::Write on a const client descriptor argument")))
			return KErrBadDescriptor;
	if ((aInfo.iFlags & KChunkShiftBy1) && !(argType & TIpcArgs::EFlag16Bit))
		if (__PSIF(aMsg, __PLATSEC_DIAGNOSTIC_STRING("Server attempted to use 16bit RMessagePtr2::Read/Write on a 8 bit client descriptor argument")))
			return KErrBadDescriptor;
	if ((argType & TIpcArgs::EFlag16Bit) && !(aInfo.iFlags & KChunkShiftBy1))
		if (__PSIF(aMsg, __PLATSEC_DIAGNOSTIC_STRING("Server attempted to use 8bit RMessagePtr2::Read/Write on a 16 bit client descriptor argument")))
			return KErrBadDescriptor;

	DThread& t = *TheCurrentThread;
	t.iTempMsg = aMsg;
	aMsg->OpenRef();

#ifndef __MEMMODEL_FLEXIBLE__
	NKern::FlashSystem();
#else
	NKern::UnlockSystem();
#endif

	TInt r = KErrNone;
	DThread* pT = aMsg->iClient;
	TInt mode = (aInfo.iFlags & KChunkShiftBy1) | KCheckLocalAddress | KDoNotUpdateDesLength;	// assume called from user mode

	if ((aInfo.iFlags & KIpcDirWrite) == 0)
		{
		r = pT->DoDesRead(aMsg->Descriptor(aParam), aInfo.iLocalPtr, aInfo.iLocalLen, aOffset, mode);
		}
	else
		{
		TAny* ptr = aMsg->Ptr(aParam);
		r = pT->DoDesWrite(ptr, aMsg->Descriptor(aParam), aInfo.iLocalPtr, aInfo.iLocalLen, aOffset, mode, NULL);
		if (r >= 0)
			{
			// Update cached flags + length word for all matching descriptors and set written bit
			TInt descFlags = aMsg->iMsgArgs.AllDescriptorFlags();
			for (TInt i = 0; descFlags != 0; ++i, descFlags >>= TIpcArgs::KBitsPerType)
				if ((descFlags & TIpcArgs::EFlagDes) && aMsg->Ptr(i) == ptr)
					{
					aMsg->Descriptor(i).SetTypeAndLength(r);
					aMsg->iMsgArgs.SetDesWritten(i);
					}
			r = KErrNone;
			}
		}

#ifndef __MEMMODEL_FLEXIBLE__
#else
	NKern::LockSystem();
#endif

	__NK_ASSERT_DEBUG(aMsg->IsAccepted() || aMsg->IsCompleting());
	t.iTempMsg = NULL;
	aMsg->CloseRef();
	return r;
	}

TInt ExecHandler::MessageClient(DThread* aClient, TOwnerType aType)
	{
	__KTRACE_OPT(KEXEC,Kern::Printf("Exec::MessageClient"));
	NKern::ThreadEnterCS();
	TInt r=aClient->Open();
	NKern::UnlockSystem();
	if (r==KErrNone)
		{
		r=K::MakeHandle(aType,aClient);
		if (r<KErrNone)
			aClient->Close(NULL);
		}
	NKern::ThreadLeaveCS();
	return r;
	}

TInt ExecHandler::MessageSetProcessPriority(DThread* aClient, TProcessPriority aPriority)
	{
	__KTRACE_OPT(KEXEC,Kern::Printf("Exec::MessageSetProcessPriority"));
	DProcess* pP = aClient->iOwningProcess;
	if (pP->iFlags & KProcessFlagPriorityControl)
		if (aPriority==EPriorityBackground || aPriority==EPriorityForeground)
			{
			ExecHandler::ProcessSetPriority(pP,aPriority);
			return KErrNone;
			}
	NKern::UnlockSystem();
	return KErrPermissionDenied;
	}

void GetCategory(TDes& aDest, const TDesC& aSrc); // in server.cpp

void ExecHandler::MessageKill(TInt aHandle, TExitType aType, TInt aReason, const TDesC* aCategory)
	{
	TBuf<KMaxExitCategoryName> cat;
	if (aType==EExitPanic && aCategory)
		GetCategory(cat,*aCategory);
	__KTRACE_OPT(KEXEC,Kern::Printf("Exec::MessageKill %d,%d,%S",aType,aReason,&cat));
	K::CheckKernelUnlocked();
	NKern::LockSystem();
	RMessageK* pM = RMessageK::MessageK(aHandle);
	DThread* pT = pM->iClient;
	pT->Die(aType,aReason,cat);		// releases system lock
	}

TInt ExecHandler::MessageOpenObject(RMessageK* aMsg, TObjectType aObjType, TInt aParam, TOwnerType aOwnerType)
	{
	__KTRACE_OPT(KEXEC,Kern::Printf("Exec::MessageOpenObject"));
	NKern::ThreadEnterCS();
	DObject* pO; 
	pO=0;   // To shut up the compiler
	TInt r;
	if ( TUint(aParam)>=TUint(KMaxMessageArguments) )
		{
		r = KErrArgument;
		goto done;
		}
	r = KErrBadHandle;
	if ( aMsg->ArgType(aParam)!=TIpcArgs::EHandle )
		goto done;
	pO=aMsg->iClient->ObjectFromHandle(aMsg->Arg(aParam),aObjType);
	if (pO)
		if(pO->Protection()!=DObject::ELocal)
			r=pO->Open();
done:
	NKern::UnlockSystem();
	if (r==KErrNone)
		{
		r=K::MakeHandle(aOwnerType,pO);	// this will add to process if necessary
		if (r<KErrNone)
			pO->Close(NULL);	// can't have been added to process so NULL
		}
	NKern::ThreadLeaveCS();
	return r;
	}

void ExecHandler::MessageCompleteWithHandle(RMessageK* aMsg, TInt aHandle)
//
// Enter and leave with system locked.
//
	{
	DObject* pO = K::ObjectFromHandle(aHandle);
	pO->CheckedOpen();
	DThread* pT=aMsg->iClient;
	TInt r = pT->Open();
	K::ThreadEnterCS();
	TInt h;
	if(r==KErrNone)
		{
		// always EOwnerThreads so it doesn't leak if client thread dies
		r = pT->MakeHandleAndOpen(EOwnerThread,pO,h);
		if (r==KErrNone)
			r = h;
		pT->Close(NULL);
		}
	pO->Close(NULL);
	K::ThreadLeaveCS();
	RMessageK::MessageK((TInt)aMsg);
	ExecHandler::MessageComplete(aMsg,r);
	}
