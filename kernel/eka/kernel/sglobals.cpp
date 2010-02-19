// Copyright (c) 1995-2009 Nokia Corporation and/or its subsidiary(-ies).
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
// e32\kernel\sglobals.cpp
// 
//

#include <kernel/kern_priv.h>
#include <kernel/sshbuf.h>

extern "C" {
TLinAddr SuperPageAddress;
}

extern void DrainEntropyBuffers(TAny*);

TMachineConfig* K::MachineConfig;
RAllocator* K::Allocator;
K::SHeapInfo K::HeapInfo;
K::SMsgInfo K::MsgInfo;
DThread* K::TheKernelThread;
DProcess* K::TheKernelProcess;
DThread* K::TheNullThread;
DThread* K::SvThread;
TDfcQue* K::SvMsgQ;
TMessageQue K::SvBarrierQ(&K::DoSvBarrier, &K::SvBarrierQ, NULL, 0);
NFastMutex K::EventQueueMutex;
TRawEvent *K::EventHeadPtr;
TRawEvent *K::EventTailPtr;
TRawEvent *K::EventBufferStart;
TRawEvent *K::EventBufferEnd;
TClientDataRequest<TRawEvent>* K::EventRequest;
DThread *K::EventThread;
TDfcQue* K::DfcQ0;
TDfcQue* K::DfcQ1;
TDfcQue* K::TimerDfcQ;
TInt K::DfcQId=0;
volatile TUint K::DynamicDfcQId=0xdfc00001;
SHalEntry2* K::HalEntryArray;
DPowerModel* K::PowerModel=NULL;
TBool K::PowerGood=ETrue;
TInt K::MaxMemCopyInOneGo;
TTickQ* K::TickQ;
TSecondQ* K::SecondQ;
TInactivityQ* K::InactivityQ;
TInt K::HomeTimeOffsetSeconds;
TInt K::NonSecureOffsetSeconds;
TInt K::SecureClockStatus;
TInt64 K::Year2000InSeconds;
DMutex* K::MachineConfigMutex;
DObjectCon* K::Containers[ENumObjectTypes];
DMutex* DCodeSeg::CodeSegLock;
TInt DCodeSeg::DeleteLock;
TInt DCodeSeg::KernelCleanupLock;
SDblQue DCodeSeg::GlobalList;
SDblQue DCodeSeg::GarbageList;
SDblQue DCodeSeg::KernelGarbageList;
SDblQue DCodeSeg::DeferredKernelGarbageList;
TDfc DCodeSeg::KernelCleanupDfc(DCodeSeg::DoKernelCleanup, NULL, 1);
RPointerArray<DCodeSeg> DCodeSeg::CodeSegsByName(8, 2*256);
DCodeSeg::RCodeSegsByAddress DCodeSeg::CodeSegsByAddress(8, 2*256);
TClientRequest* DCodeSeg::DestructNotifyRequest=NULL;
DThread* DCodeSeg::DestructNotifyThread=NULL;
TCodeSegLoaderCookieList* DCodeSeg::DestructNotifyList=NULL;
TDfc TClientRequest::DeadClientCleanupDfc(TClientRequest::DoDeadClientCleanup, NULL, 2);

TInt K::MaxFreeRam;
//
TInt K::NextId=0;
//
NFastMutex TMessageQue::MsgLock;
NFastMutex DObject::Lock;
TUint64 DObject::NextObjectId = 1u<<18;
DMutex* RObjectIx::HandleMutex;
NFastMutex TLogon::LogonLock;
//
TAny* volatile K::AsyncFreeHead=NULL;
DBase* volatile K::AsyncDeleteHead=NULL;
TDfc K::AsyncFreeDfc(K::DoAsyncFree, NULL, 2);
TInt K::MemoryLowThreshold=0;
TInt K::MemoryGoodThreshold=KMaxTInt;
TUint K::AsyncChanges=0;
TDfc K::AsyncChangeNotifierDfc(K::DoAsyncNotify, NULL, 2);

//
TBool K::Initialising=ETrue;
TBool K::ColdStart;
TInt K::ExtensionCount = 0;
RArray<SExtInit1EntryPoint>* K::ExtensionArray=0;
DProcess* K::TheFileServerProcess;
DProcess* K::TheWindowServerProcess;
TInt K::PINestLevel=0;
SDblQue DKernelEventHandler::HandlersQ;
TInt K::SupervisorThreadStackSize;
TUint32 K::MemModelAttributes;

DMutex* TInternalRamDrive::Mutex=NULL;

TKernelHookFn K::KernelHooks[ENumKernelHooks] =
	{
	NULL,									// EHookTrace
	(TKernelHookFn)K::DoNanoWait,			// EHookNanoWait
	(TKernelHookFn)P::DefaultInitialTime	// EHookInitialTime
	};

TUint K::TextTraceMode=0;

TMiscNotifierMgr K::TheMiscNotifierMgr;
TAny* K::VariantData[31];

// Array of per CPU entropy buffer status values, each has the number of words used in the bottom 14 bits, 
// which may not exceed 2^13 words (32KB or 256Kbit). 
// The top 18 bits is the number of bits of entropy with maximum value 2^18 or 256Kbit which matches the buffer size.
TUint32 K::EntropyBufferStatus[KMaxCpus];

// Array of per CPU pointers to entropy buffers.
TUint32* K::EntropyBuffer[KMaxCpus];

// Temporary buffer to drain per cpu buffers to
TUint32 K::TempEntropyBuffer[KEntropyBufferSizeWords];

// DFC to drain entropy buffers
TDfc K::EntropyBufferDfc(DrainEntropyBuffers, NULL, 1);

TDfcQue DShPool::iSharedDfcQue;		// DFCQ thread for shareable data buffers growing/shrinking/notifications
