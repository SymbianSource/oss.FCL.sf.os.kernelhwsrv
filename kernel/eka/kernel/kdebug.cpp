// Copyright (c) 2002-2010 Nokia Corporation and/or its subsidiary(-ies).
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
// e32\kernel\kdebug.cpp
//
//

#include <kernel/kdebug.h>
#include <kernel/kernel.h>
#include "memmodel.h"
#include "debug.h"
#include <kernel/cache.h>
#include "kdebug_priv.h"

/**
Stop-mode debugger offset tables
This tables give the stop-mode debugger a description of the important
kernel objects.  Enumeration for each entry is given in debug.h
*/
const TInt Debugger::ObjectOffsetTable[]=
	{
// DDebuggerInfo info
	_FOFF(DDebuggerInfo, iObjectOffsetTable),
	_FOFF(DDebuggerInfo, iObjectOffsetTableCount),
	_FOFF(DDebuggerInfo, iThreadContextTable),
	_FOFF(DDebuggerInfo, iVersion),
	_FOFF(DDebuggerInfo, iOSVersion),
	KDebuggerOffsetInvalid,
	_FOFF(DDebuggerInfo, iContainers),
	_FOFF(DDebuggerInfo, iScheduler),
	_FOFF(DDebuggerInfo, iCurrentThread),
	_FOFF(DDebuggerInfo, iCodeSegGlobalList),
	_FOFF(DDebuggerInfo, iCodeSegLock),
	_FOFF(DDebuggerInfo, iChange),

// DMutex info
	_FOFF(DMutex, iHoldCount),

// more DDebuggerInfo info
	_FOFF(DDebuggerInfo, iShadowPages),
	_FOFF(DDebuggerInfo, iShadowPageCount),
	_FOFF(DDebuggerInfo, iEventMask),

// DObjectCon info
	_FOFF(DObjectCon, iMutex),
	_FOFF(DObjectCon, iObjects),
	_FOFF(DObjectCon, iCount),

// more DDebuggerInfo info
	_FOFF(DDebuggerInfo, iEventHandlerBreakpoint),
	_FOFF(DDebuggerInfo, iMemoryModelType),
	_FOFF(DDebuggerInfo, iMemModelObjectOffsetTable),
	_FOFF(DDebuggerInfo, iMemModelObjectOffsetTableCount),

// thread info
	_FOFF(DThread, iName),
	_FOFF(DThread, iId),
	_FOFF(DThread, iOwningProcess),
	_FOFF(DThread, iNThread),
	_FOFF(DThread, iSupervisorStack),
	_FOFF(DThread, iSupervisorStackSize),
	_FOFF(DThread, iUserStackRunAddress),
	_FOFF(DThread, iUserStackSize),
	_FOFF(DThread, iNThread.iSpare3 /*iUserContextType*/),
	_FOFF(DThread, iNThread.iSavedSP),
	_FOFF(DThread, iNThread.iPriority),
	_FOFF(DThread, iThreadType),
	_FOFF(DThread, iFlags),

// process info
	_FOFF(DProcess, iName),
	_FOFF(DProcess, iId),
	_FOFF(DProcess, iAttributes),
	_FOFF(DProcess, iCodeSeg),
	_FOFF(DProcess, iDataBssRunAddress),
	_FOFF(DProcess, iDataBssStackChunk),
#ifdef __MEMMODEL_MOVING__
	_FOFF(DMemModelProcess, iNumChunks),	//ARMv5 specific
	_FOFF(DMemModelProcess, iChunks),		//ARMv5 specific
#else
	KDebuggerOffsetInvalid,
	KDebuggerOffsetInvalid,
#endif
	KDebuggerOffsetInvalid,
	KDebuggerOffsetInvalid,
	KDebuggerOffsetInvalid,
	KDebuggerOffsetInvalid,

// chunkinfo info
#ifdef __MEMMODEL_MOVING__
	_FOFF(DMemModelProcess::SChunkInfo, iDataSectionBase),	//ARMv5 specific
	_FOFF(DMemModelProcess::SChunkInfo, iChunk),			//ARMv5 specific
#else
	KDebuggerOffsetInvalid,
	KDebuggerOffsetInvalid,
#endif
	KDebuggerOffsetInvalid,
	KDebuggerOffsetInvalid,

// chunk info
#ifdef __MEMMODEL_MOVING__
	_FOFF(DMemModelChunk, iOwningProcess), //ARMv5 specific
#else
	KDebuggerOffsetInvalid,
#endif
	_FOFF(DMemModelChunk, iSize),
	_FOFF(DMemModelChunk, iAttributes),
	_FOFF(DMemModelChunk, iChunkType),
#ifdef __MEMMODEL_MOVING__
	_FOFF(DMemModelChunk, iChunkState),		//ARMv5 specific
	_FOFF(DMemModelChunk, iHomeBase),		//ARMv5 specific
#else
	KDebuggerOffsetInvalid,
	KDebuggerOffsetInvalid,
#endif
	KDebuggerOffsetInvalid,
	KDebuggerOffsetInvalid,
	KDebuggerOffsetInvalid,
	KDebuggerOffsetInvalid,

// library info
	_FOFF(DLibrary, iMapCount),
	_FOFF(DLibrary, iState),
	_FOFF(DLibrary, iCodeSeg),
	KDebuggerOffsetInvalid,
	KDebuggerOffsetInvalid,
	KDebuggerOffsetInvalid,
	KDebuggerOffsetInvalid,

// code seg info
	_FOFF(DCodeSeg, iLink.iNext),
	_FOFF(DCodeSeg, iLink.iPrev),
	_FOFF(DCodeSeg, iDeps),
	_FOFF(DCodeSeg, iDepCount),
	_FOFF(DCodeSeg, iFileName),
	_FOFF(DEpocCodeSeg, iXIP),
	_FOFF(DEpocCodeSeg, iInfo),
	KDebuggerOffsetInvalid,
	KDebuggerOffsetInvalid,
	KDebuggerOffsetInvalid,
	KDebuggerOffsetInvalid,

// scheduler info
	_FOFF(TScheduler, iKernCSLocked),
	_FOFF(TScheduler, iLock.iWaiting),
	_FOFF(TScheduler, iCurrentThread),
	_FOFF(TScheduler, iAddressSpace),
	KDebuggerOffsetInvalid,
	KDebuggerOffsetInvalid,
	KDebuggerOffsetInvalid,
	KDebuggerOffsetInvalid,

// code segment information non-XIP
	_FOFF(SRamCodeInfo, iCodeSize),
	_FOFF(SRamCodeInfo, iTextSize),
	_FOFF(SRamCodeInfo, iDataSize),
	_FOFF(SRamCodeInfo, iBssSize),
	_FOFF(SRamCodeInfo, iCodeRunAddr),
	_FOFF(SRamCodeInfo, iCodeLoadAddr),
	_FOFF(SRamCodeInfo, iDataRunAddr),
	_FOFF(SRamCodeInfo, iDataLoadAddr),
	_FOFF(SRamCodeInfo, iConstOffset),
	_FOFF(SRamCodeInfo, iExportDir),
	_FOFF(SRamCodeInfo, iExportDirCount),
	KDebuggerOffsetInvalid,
	KDebuggerOffsetInvalid,
	KDebuggerOffsetInvalid,
	KDebuggerOffsetInvalid,

// code segment information XIP
	_FOFF(TRomImageHeader, iCodeAddress),
	_FOFF(TRomImageHeader, iDataAddress),
	_FOFF(TRomImageHeader, iDataBssLinearBase),
	_FOFF(TRomImageHeader, iCodeSize),
	_FOFF(TRomImageHeader, iTextSize),
	_FOFF(TRomImageHeader, iDataSize),
	_FOFF(TRomImageHeader, iBssSize),
	_FOFF(TRomImageHeader, iExportDir),
	_FOFF(TRomImageHeader, iExportDirCount),
	KDebuggerOffsetInvalid,
	KDebuggerOffsetInvalid,
	KDebuggerOffsetInvalid,
	KDebuggerOffsetInvalid,

	_FOFF(DDebuggerInfo, iStopModeExtension),
	_FOFF(Debug::DStopModeExtension, iFunctionalityBlock),
	KDebuggerOffsetInvalid,
	KDebuggerOffsetInvalid,

// Event filtering information
	_FOFF(DDebuggerInfo, iFilterBuffer),
	_FOFF(DDebuggerInfo, iFilterBufferSize),
	_FOFF(DDebuggerInfo, iFilterBufferInUse),
	KDebuggerOffsetInvalid,
	KDebuggerOffsetInvalid,

// more thread info
	_FOFF(DThread, iExitType),
	_FOFF(DThread, iExitCategory),
	_FOFF(DThread, iExitReason),
	KDebuggerOffsetInvalid,
	KDebuggerOffsetInvalid
	};

#ifdef __MEMMODEL_MOVING__
const TInt Debugger::VariantObjectOffsetTable[]=
	{
	_FOFF(DMemModelProcess, iNumChunks),
	_FOFF(DMemModelProcess, iChunks),

	_FOFF(DMemModelProcess::SChunkInfo, iDataSectionBase),
	_FOFF(DMemModelProcess::SChunkInfo, iChunk),

	_FOFF(DMemModelChunk, iOwningProcess),
	_FOFF(DMemModelChunk, iChunkState),
	_FOFF(DMemModelChunk, iHomeBase),
	KDebuggerOffsetInvalid,
	KDebuggerOffsetInvalid,
	KDebuggerOffsetInvalid,
	KDebuggerOffsetInvalid
	};
#elif __MEMMODEL_MULTIPLE__
const TInt Debugger::VariantObjectOffsetTable[]=
	{
	_FOFF(DMemModelProcess, iOsAsid),
	_FOFF(DMemModelProcess, iLocalPageDir),
	_FOFF(DMemModelProcess, iChunkCount),
	_FOFF(DMemModelProcess, iChunks),
	_FOFF(DMemModelProcess::SChunkInfo, iChunk),
	_FOFF(DMemModelChunk, iOwningProcess),
	KDebuggerOffsetInvalid,
	KDebuggerOffsetInvalid,
	KDebuggerOffsetInvalid,
	KDebuggerOffsetInvalid
	};
#elif __MEMMODEL_FLEXIBLE__
const TInt Debugger::VariantObjectOffsetTable[]=
	{
	_FOFF(DMemModelProcess, iOsAsid),
	_FOFF(DMemModelProcess, iPageDir),
	_FOFF(DMemModelProcess, iChunkCount),
	_FOFF(DMemModelProcess, iChunks),
	_FOFF(DMemModelProcess::SChunkInfo, iChunk),
	_FOFF(DMemModelChunk, iOwningProcess),
	KDebuggerOffsetInvalid,
	KDebuggerOffsetInvalid,
	KDebuggerOffsetInvalid,
	KDebuggerOffsetInvalid
	};
#else
	//Debug API is not supported for other memory models apart from moving & multiple
	__ASSERT_COMPILE(EFalse);
#endif

//
// Reschedule hook to enable thread context type
//

DDebuggerInfo* TheDebuggerInfo = NULL;

inline NThread*& CurrentThread() { return TheDebuggerInfo->iCurrentThread; }
inline volatile TUint32& Change() { return TheDebuggerInfo->iChange; }

void RescheduleCallback(NThread* aNThread)
/**
 Main scheduler callback.
 Called with the Kernel Lock (Preemption Lock) held.
*/
	{
	// The 'CurrentThread' is now unscheduled and has become the 'previous thread'
	// Set this thread 'UserContextType'...
	CurrentThread()->SetUserContextType();

	// Make the newly scheduled thread the CurrentThread
	CurrentThread() = aNThread;
	}

void RescheduleCallbackFirst(NThread* aNThread)
/**
 Scheduler callback used once for initialisation.
 Called with the Kernel Lock (Preemption Lock) held.
*/
	{
	// Initialise CurrentThread
	CurrentThread() = aNThread;

	// Switch future callbacks to the main RescheduleCallback
	NKern::SetRescheduleCallback(RescheduleCallback);
	}

void EnableRescheduleCallback()
/**
 Sets the Scheduler to call us back on every thread reschedule
*/
	{
	// Reset the User Context Type for all threads, because these values
	// will be out-of-date. (It is our Rescheduler callback which set's them.
	// and we're just about enable that.)

	NKern::ThreadEnterCS();  // Prevent us from dying or suspending whilst holding a DMutex
	DObjectCon& threads = *Kern::Containers()[EThread];  // Get container holding threads
	threads.Wait();  // Obtain the container mutex so the list does get changed under us

	// For each thread...
	TInt count = threads.Count();
	for (TInt i = 0; i < count; ++i)
		{
		((DThread*)threads[i])->iNThread.ResetUserContextType();
		}

	threads.Signal();  // Release the container mutex
	NKern::ThreadLeaveCS();  // End of critical section

	// Ask for callback
	NKern::Lock();
	NKern::SetRescheduleCallback(RescheduleCallbackFirst);
	NKern::Unlock();
	}

void DisableRescheduleCallback()
/**
 Stops the Scheduler calling us on every context switch
*/
	{
	// Prevent rescheduling whilst we disable the callback
	NKern::Lock();

	// Disable Callback
	NKern::SetRescheduleCallback(NULL);

	// Invalidate CurrentThread
	CurrentThread() = NULL;

	// Callback now disabled...
	NKern::Unlock();
	}

void RemoveSchedulerHooks()
/**
 Removes the patches added to the Scheduler code.
*/
	{
	// Make sure callback is disabled (required before removing hooks)
	DisableRescheduleCallback();

	// Get range of memory used by hooks
	TLinAddr start, end;
	NKern::SchedulerHooks(start, end);

	// Free shadow pages which cover hooks
	TUint32 pageSize=Kern::RoundToPageSize(1);

	// Enter CS as we're handling shadowing
	NKern::ThreadEnterCS();

	// Free all shadow pages, a page at a time
	for (TLinAddr addr = start; addr < end; addr += pageSize)
		{
		Epoc::FreeShadowPage(addr);   // Ignore errors because we're trying to clean up anyway
		}

	// And release
	NKern::ThreadLeaveCS();
	}

TInt InsertSchedulerHooks()
/**
 Enables use of the Scheduler callback by using shadow pages to patch the Scheduler code.
*/
	{
	// Get range of memory used by hooks
	TLinAddr start, end;
	NKern::Lock();
	NKern::SchedulerHooks(start, end);
	NKern::Unlock();

	// Create shadow pages for hooks
	TUint32 pageSize = Kern::RoundToPageSize(1);

	for (TLinAddr addr = start; addr < end; addr += pageSize)
		{
		// Enter CS whilst handling shadowing
		NKern::ThreadEnterCS();

		// Allocate the shadow page
		TInt r = Epoc::AllocShadowPage(addr);

		// And leave...
		NKern::ThreadLeaveCS();

		// Check for unrecoverable errors
		if (r != KErrNone && r != KErrAlreadyExists)
			{
			// Bail out
			NKern::Lock();
			RemoveSchedulerHooks();
			NKern::Unlock();
			return r;
			}
		}

	// Put hooks in
	NKern::Lock();
	NKern::InsertSchedulerHooks();
	NKern::Unlock();

	// Make I and D caches consistant for hook region
	Cache::IMB_Range(start, (end - start));

	return KErrNone;
	}

//
// Event handler for change notification
//
DEventHandler::DEventHandler()
	:	DKernelEventHandler(EventHandler, this)
	{
	}

DEventHandler* DEventHandler::New()
	{
	__KTRACE_OPT(KBOOT, Kern::Printf(">>DEventHandler::New"));

	// Create new object
	DEventHandler* handler = new DEventHandler();

	if (handler)
		{
		// Get sizes required for event handler fn.
		const TUint32 KPagesNotMapped = ~0u;
		TUint32 physAddr = KPagesNotMapped;
		const TUint handlerSize = DummyHandlerSize();

		// Allocate some RAM to copy the fn. into
		TInt err = Epoc::AllocPhysicalRam(handlerSize, physAddr);

		if (!err)
			{
			// Set up an executable HW chunk for the fn.
			err = DPlatChunkHw::New(handler->iHwChunk, physAddr, handlerSize, EMapAttrSupRwx | EMapAttrFullyBlocking);
			}

		if (!err)
			{
			const TLinAddr linAddr = handler->iHwChunk->iLinAddr;
			CopyDummyHandler(linAddr);

			// NB: flushing not required since chunk is uncacheable
//			Cache::IMBRange(linAddr, handlerSize);
			}

		if (err)
			{
			__KTRACE_OPT(KBOOT, Kern::Printf("DEventHandler::New() failing with err = %d", err));

			// Free handler (and hence chunk), if any
			delete handler;
			handler = NULL;

			// Free RAM, if allocated
			if (physAddr != KPagesNotMapped)
				{
				Epoc::FreePhysicalRam(physAddr, handlerSize);
				}
			}

		}

	__KTRACE_OPT(KBOOT, Kern::Printf("<<DEventHandler::New, returning 0x%08x", handler));
	return handler;
	}

void DEventHandler::CopyDummyHandler(TLinAddr aLinAddr)
	{
	const TUint handlerSize = DummyHandlerSize();

	// Copy the breakpoint-able handler into RAM by copying from the one (possibly) in ROM
	memcpy((TAny*)aLinAddr, (TAny*) &DummyHandler, handlerSize);
	__KTRACE_OPT(KBOOT, Kern::Printf("Breakpoint-able handler copied from 0x%x to (va) 0x%x, size %d", &DummyHandler, aLinAddr, handlerSize));
	}

DEventHandler::~DEventHandler()
	{
	if (iHwChunk)
		{
		const TPhysAddr physAddr = iHwChunk->iPhysAddr;
		const TInt size = iHwChunk->iSize;

		iHwChunk->Close(NULL);

		Epoc::FreePhysicalRam(physAddr, size);
		}
	}

TInt DEventHandler::Add(TAddPolicy aPolicy)
	{
	__KTRACE_OPT(KBOOT, Kern::Printf("DEventHandler::Add"));

	// Check we have a debug info table to add the handler into
	if (!TheDebuggerInfo)
		{
		__KTRACE_OPT(KBOOT, Kern::Printf("No DDebuggerInfo created yet - cannot add breakpointable handler!"));
		return KErrNotReady;
		}

	// Install the breakpoint-able handler
	TheDebuggerInfo->iEventHandlerBreakpoint = iHwChunk->iLinAddr;

	// Add the handler to the queue
	return DKernelEventHandler::Add(aPolicy);
	}

/**
 Event handler that monitors changes for processes, threads, chunks, libraries and code segments
 Changes are flagged in the iChange member of DDebuggerInfo
*/
TUint DEventHandler::EventHandler(TKernelEvent aType, TAny* a1, TAny* a2, TAny* aThis)
	{
	TUint ret = DKernelEventHandler::ERunNext;

	TheDebuggerInfo->LockFilterBuffer();

	TFilterHeader* hdr = (TFilterHeader*)TheDebuggerInfo->iFilterBuffer;
	TBool interested = EFalse;
	TBool hdrPresent = hdr && (hdr->iSignature == KFilterBufferSignature);

	// Update the appropriate bit in the change bitmask
	switch (aType)
		{
	case EEventAddCodeSeg:
	case EEventRemoveCodeSeg:
		{
		DCodeSeg* codeSeg = (DCodeSeg*)a1;
		interested = hdrPresent && DEventHandler::InterestedIn(aType, codeSeg->iRootName);
		Change() |= KDebuggerChangeCode;

		break;
		}
	case EEventAddLibrary:
	case EEventRemoveLibrary:
		{
		DLibrary* lib = (DLibrary*)a1;
		interested = hdrPresent && DEventHandler::InterestedIn(aType, lib->iCodeSeg->iRootName);
		Change() |= KDebuggerChangeLibrary;

		break;
		}
	case EEventNewChunk:
	case EEventUpdateChunk:
	case EEventDeleteChunk:
		{
		DChunk* chunk = (DChunk*)a1;

		//The chunk will not neccesarily have an owning process.
		interested = hdrPresent && ( !chunk->iOwningProcess && hdr->iFlags & TFilterHeader::EGlobalEvents ||
				(chunk->iOwningProcess && DEventHandler::InterestedIn(aType, *(chunk->iOwningProcess->iName))));

		Change() |= KDebuggerChangeChunk;

		break;
		}
	case EEventAddThread:
	case EEventUpdateThread:
	case EEventRemoveThread:
		{
		DThread* thread = (DThread*)a1;
		interested = hdrPresent && DEventHandler::InterestedIn(aType, *(thread->iOwningProcess->iName));
		Change() |= KDebuggerChangeThread;

		break;
		}
	case EEventAddProcess:
	case EEventUpdateProcess:
	case EEventRemoveProcess:
		{
		DProcess* proc = (DProcess*)a1;
		interested = hdrPresent && DEventHandler::InterestedIn(aType, *(proc->iName));
		Change() |= KDebuggerChangeProcess;

		break;
		}
	case EEventLoadLdd:
	case EEventUnloadLdd:
		{
		DCodeSeg* codeSeg = (DCodeSeg*)a1;
		interested = hdrPresent && DEventHandler::InterestedIn(aType, codeSeg->iRootName);
		Change() |= KDebuggerChangeLdd;

		break;
		}
	case EEventLoadPdd:
	case EEventUnloadPdd:
		{
		DCodeSeg* codeSeg = (DCodeSeg*)a1;
		interested = hdrPresent && DEventHandler::InterestedIn(aType, codeSeg->iRootName);
		Change() |= KDebuggerChangePdd;

		break;
		}
	case EEventSwExc:
	case EEventHwExc:
		{
		DThread* thread = &Kern::CurrentThread();
		interested = hdrPresent && DEventHandler::InterestedIn(aType, *(thread->iOwningProcess->iName));

		// iChange variable not updated for compatibilty
		break;
		}
	case EEventKillThread:
		{
		DThread* thread = (DThread*)a1;
		interested = hdrPresent && DEventHandler::InterestedIn(aType, *(thread->iOwningProcess->iName));
		// iChange variable not updated for compatibilty
		break;
		}
	default:
		break;
		}

	// If this event matches the mask, call the event handler
	if (((1 << (aType & 31)) & TheDebuggerInfo->iEventMask[aType >> 5]) || interested)
		{
		__KTRACE_OPT(KDEBUGGER, Kern::Printf("KDebug: Calling debugger breakpoint-able handler for event type = 0x%x", aType));

		DEventHandler* handler = (DEventHandler*)aThis;
		ret = (*((TDebuggerEventHandler)handler->iHwChunk->iLinAddr))(aType, a1, a2);
		}

	TheDebuggerInfo->ReleaseFilterBuffer();

	// Allow other handlers to see this event
	return ret;
	}

/**
This looks at the filter list and event type
event is of interest to the client.
@param aKernelEvent Event Type for notification
@return true if user is interested in this event

@pre Assumes there is a filter present and valid
*/
TBool DEventHandler::InterestedIn(const TKernelEvent& aKernelEvent, TDesC& aName)
	{
	TFilterHeader* hdr = (TFilterHeader*)TheDebuggerInfo->iFilterBuffer;
	TUint8* pos = TheDebuggerInfo->iFilterBuffer + sizeof(TFilterHeader);

	TUint i;
	for(i = hdr->iNumItems; i != 0; --i)
		{
		TFilterObject* obj = (TFilterObject*)pos;

		//Get the filter object name eg. helloworld.exe
		TPtrC ptr(obj->iName, obj->iLength);

		//Each bit in iEventMask maps onto each member of enum TKernelEvent. ie the Nth bit of iEvent indicates
		//if we are interested in the Nth enum in TKernelEvent
		TUint64 eventOfInterest = ((1ULL << aKernelEvent) & obj->iEventMask);
		if(eventOfInterest && (aName.CompareF(ptr) == 0))
			{
			return ETrue;
			}

		pos += Align4(sizeof(TFilterObject) + obj->iLength -1);
		}

	//no matching filter object found and so not interested
	return EFalse;
	}

/**
Create the stop-mode debug API
*/
DDebuggerInfo::DDebuggerInfo()
	:	// initialise the debug tables
		iObjectOffsetTable(&Debugger::ObjectOffsetTable[0]),
		iObjectOffsetTableCount(EOffsetTableEntryMax),
		iThreadContextTable(NThread::UserContextTables()),

		// publish the version number
		iVersion(Debugger::Version()),
		iOSVersion(TVersion(KE32MajorVersionNumber, KE32MinorVersionNumber, KE32BuildVersionNumber)),
		iStopModeExtension(NULL),

		// initialise our other mebers
//		iChange(0),
		iContainers(Kern::Containers()),
		iCodeSegLock(Kern::CodeSegLock()),
		iCodeSegGlobalList(Kern::CodeSegList()),
		iScheduler(TScheduler::Ptr()),
		#ifdef __MEMMODEL_MOVING__
		iMemoryModelType(EARMv5MMU),
		#elif defined(__MEMMODEL_MULTIPLE__) || defined(__MEMMODEL_FLEXIBLE__)
		iMemoryModelType(EARMv6MMU),
		#endif
		iMemModelObjectOffsetTable(&Debugger::VariantObjectOffsetTable[0]),
		iMemModelObjectOffsetTableCount(sizeof(Debugger::VariantObjectOffsetTable)/sizeof(TInt)),
		iFilterBuffer(NULL),
		iFilterBufferSize(0),
		iFilterBufferInUse(EFalse)
//		iShadowPages(NULL),
//		iShadowPageCount(0),
//		iCurrentThread(NULL),
//		iEventMask(),
//		iEventHandlerBreakpoint(0)
	{
	}

/**
Locks access to the filter buffer by means of a boolean.
The debugger should read the iFilterBufferInUse variable to
see if it can write to the buffer
*/
inline void DDebuggerInfo::LockFilterBuffer()
	{
	iFilterBufferInUse = ETrue;
	}

/**
Releases access to the filter buffer. If the debugger wishes to write to
the filter buffer and finds it to be currently in use, it should run to
this function to wait for access.
*/
void DDebuggerInfo::ReleaseFilterBuffer()
	{
	iFilterBufferInUse = EFalse;
	}

/**
Install a stop-mode debugger
Make the stop-mode API visible to a JTAG debugger, by publishing its
existance in the superpage
*/
EXPORT_C TInt Debugger::Install(DDebuggerInfo* aDebugger)
	{
	Kern::SuperPage().iDebuggerInfo = aDebugger;
	return KErrNone;
	}

/**
Return the currently installed stop-mode API
*/
EXPORT_C DDebuggerInfo* Debugger::DebuggerInfo()
	{
	return Kern::SuperPage().iDebuggerInfo;
	}

EXPORT_C TVersion Debugger::Version()
/**
Return the version of the debug API
*/
	{
	return TVersion(KDebuggerMajorVersionNumber, KDebuggerMinorVersionNumber, KDebuggerBuildVersionNumber);
	}

/**
Extension entry point
Install a stop-mode debugger API
*/
DECLARE_STANDARD_EXTENSION()
	{
	__KTRACE_OPT(KBOOT, Kern::Printf("Starting debugger API"));

	// Create debugger info
	DDebuggerInfo* debugInfo = new DDebuggerInfo();

	if (!debugInfo)
		{
		__KTRACE_OPT(KBOOT, Kern::Printf("Debugger API failed to start"));
		return KErrNone;
		}

	TheDebuggerInfo = debugInfo;

	// Install scheduler hooks
	TInt err = InsertSchedulerHooks();
	if (!err)
		{
		EnableRescheduleCallback();
		}
	else
		{
		__KTRACE_OPT(KBOOT, Kern::Printf("Debugger rescheduler hooks failed to start %d", err));
		}

	// Add event handler for updating change flags
	DEventHandler* eventHandler = DEventHandler::New();
	if (eventHandler)
		{
		err = eventHandler->Add();
		}
	else
		{
		err = KErrNoMemory;
		}

	if (err)
		{
		__KTRACE_OPT(KBOOT, Kern::Printf("Debugger change notifier failed to start %d", err));
		}

	//Allocate buffer for filter
	TAny* req = Kern::AllocZ(KFilterBufferSize);
	if(!req)
		{
		//This is not fatal, just means we can't filter data. Buffer is NULL
		__KTRACE_OPT(KBOOT, Kern::Printf("Debugger failed to allocate filter buffer. Filtering is not enabled"));
		}
	else
		{
		debugInfo->iFilterBuffer = (TUint8*)req;
		debugInfo->iFilterBufferSize = KFilterBufferSize;
		}

	// Publish debug info in the super page
	err = Debugger::Install(debugInfo);

	return err;
	}
