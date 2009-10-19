// Copyright (c) 2003-2009 Nokia Corporation and/or its subsidiary(-ies).
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
// e32test\debug\d_eventtracker.cpp
// LDD-based debug agent used to track kernel events.  See
// t_eventtracker.cpp
// 
//

#include <kernel/kern_priv.h>
#include "reventtracker.h"
#include "d_eventtracker.h"
#include "nk_trace.h"

#ifdef __MARM__
#include <kernel/kdebug.h>
#endif //__MARM__

#ifdef _DEBUG
static const char KPanicCat[] = "D_EVENTTRACKER";
#endif // _DEBUG
_LIT(KClientPanicCat, "D_EVENTTRACKER");

DEventTracker* TheEventTracker;

//////////////////////////////////////////////////////////////////////////////

/** Data about objects being tracked.
	All tracked objects are kept in a tracking list.  The object address 
	is a key and so must be unique.
 */

TTrackedItem::TTrackedItem(const DBase* aObject)
	: iObject(aObject), iAccountedFor(EFalse)
	{
	}
	

/** Subclass for DObjects being tracked */

TTrackedObject::TTrackedObject(DObject* aObject, TObjectType aType)
	:	TTrackedItem(aObject),
		iType(aType)
	{
	aObject->FullName(iFullName);
	}

TBool TTrackedObject::CheckIntegrity(const TDesC& aName, TObjectType aType) const
	{
	TBool ok = EFalse;
	
	if (aType == iType)
		{
		if (aType == EThread || aType == EProcess)
			{
			ok = (iFullName == aName);
			}
		else
			{
			ok = ETrue;
			}
		}

	if (!ok)
		{
		Kern::Printf("EVENTTRACKER: container / tracking list mismatch (0x%08x)", iObject);
		Kern::Printf("EVENTTRACKER: \tcontainer: %S (type %d)", &aName, aType);
		Kern::Printf("EVENTTRACKER: \ttracking list: %S (type %d)", &iFullName, iType);
		}
		
	return ok;
	}

/** Subclass for DCodeSegs being tracked */

TTrackedCodeSeg::TTrackedCodeSeg(const DCodeSeg* aCodeSeg)
	:	TTrackedItem(aCodeSeg),
		iAccessCount(aCodeSeg ? aCodeSeg->iAccessCount : 0)
	{
	}

TBool TTrackedCodeSeg::CheckIntegrity(TInt aAccessCount) const
	{
	const TBool ok = (aAccessCount == iAccessCount);

	if (!ok)
		{
		Kern::Printf("EVENTTRACKER: code seg list / tracking list mismatch (0x%08x)", iObject);
		Kern::Printf("EVENTTRACKER: \tcode seg list: %d", aAccessCount);
		Kern::Printf("EVENTTRACKER: \ttracking list: %d", iAccessCount);
		}
		
	return ok;
	}


/** Event handler and container for all objects being tracked.  */

DEventTracker::DEventTracker()
	:	DKernelEventHandler(EventHandler, this)
	{
	__ASSERT_DEBUG(!TheEventTracker, Kern::Fault(KPanicCat, __LINE__));
	
	TheEventTracker = this;
	}


//
// If aUseHook is true, the event tracker hooks the stop-mode debugger 
// breakpoint in preference to adding itself to the kernel event handler
// queue.  In order to clean up on its destruction, it has to
// reset the breakpoint by installing a dummy nop breakpoint
// handler, which is cut-and-pasted from kdebug.dll in order to
// avoid a dependency on kdebug.dll.  In order to use the event
// tracker using the stop-mode debugger breakpoint rather than
// the kernel event handler queue, kdebug.dll must be present in
// the ROM
//
TInt DEventTracker::Create(DLogicalDevice* aDevice, TBool aUseHook)
	{
	TInt err = aDevice->Open();

	if (err)
		{
		return err;
		}
	
	iDevice = aDevice;

	err = Kern::MutexCreate(iLock, _L("EventHandlerLock"), KMutexOrdNone);

	if (!err)
		{
		if (aUseHook)
			{
			// Find debugger info, if any
			DDebuggerInfo* const debugInfo = Kern::SuperPage().iDebuggerInfo;

			// Test stop-mode breakpoint if available
			if (debugInfo)
				{
#ifdef __MARM__
				// Receive all events
				for (TInt i = 0; i < ((EEventLimit + 31) >> 5); ++i)
					{
					debugInfo->iEventMask[i] = 0xffffffffu;
					}
				
				__KTRACE_OPT(KDEBUGGER, Kern::Printf("EVENTTRACKER: Copying breakpoint (0x%x) into handler (0x%x), size %d", &BranchToEventHandler, debugInfo->iEventHandlerBreakpoint, BreakPointSize()));

				// Set up breakpoint to call handler
				memcpy((TAny*)debugInfo->iEventHandlerBreakpoint, (TAny*) &BranchToEventHandler, BreakPointSize());
#else // !__MARM__
				err = KErrNotFound;
#endif // __MARM__
				}
			else
				{
				err = KErrNotFound;
				}
			}
		else
			{
			err = Add();
			}
		}
	
	return err;
	}


DEventTracker::~DEventTracker()
	{
#ifdef __MARM__
	// Remove breakpoint, if any
	DDebuggerInfo* const debugInfo = Kern::SuperPage().iDebuggerInfo;
	if (debugInfo)
		{
		CopyDummyHandler(debugInfo->iEventHandlerBreakpoint);
		}
#endif //__MARM__

	// clean-up tracking list
	SDblQueLink* link = iItems.GetFirst();
	while (link)
		{
		delete _LOFF(link, TTrackedItem, iLink);
		link = iItems.GetFirst();
		}

	if (iLock)
		{
		iLock->Close(NULL);
		}

	if (iDevice)
		{
		iDevice->Close(NULL);
		}
		
	TheEventTracker = NULL;
	}


TInt DEventTracker::Start()
	{
	TInt err = AddExistingObjects();
	
	if (!err)
		{
		iTracking = ETrue;
		}
	
	return err;
	}


TInt DEventTracker::Stop()
	{
	NKern::ThreadEnterCS();
	Kern::MutexWait(*iLock);

	iTracking = EFalse;

	Kern::MutexSignal(*iLock);
	NKern::ThreadLeaveCS();

	DumpCounters();

	return CheckIntegrity();
	}


TUint DEventTracker::EventHandler(TKernelEvent aType, TAny* a1, TAny* a2, TAny* aThis)
	{
	return ((DEventTracker*)aThis)->HandleEvent(aType, a1, a2);
	}


TUint DEventTracker::HandleEvent(TKernelEvent aType, TAny* a1, TAny* a2)
	{ 
	__KTRACE_OPT(KDEBUGGER, Kern::Printf("EVENTTRACKER: Handling event type 0x%x", aType));

	Kern::MutexWait(*iLock);

	if (iTracking)
		{
		++iCounters[aType];

		switch (aType)
			{
		case EEventAddProcess:
			AddObject(EProcess, (DObject*)a1);
			break;
		case EEventUpdateProcess:
			// could be renaming or chunk addition/deletion
			UpdateObject(EProcess, (DObject*)a1, EFalse);
			break;
		case EEventRemoveProcess:
			RemoveObject(EProcess, (DObject*)a1);
			break;
		case EEventAddThread:
			AddObject(EThread, (DObject*)a1);
			break;
		case EEventUpdateThread:
			UpdateObject(EThread, (DObject*)a1, ETrue);
			break;
		case EEventRemoveThread:
			RemoveObject(EThread, (DObject*)a1);
			break;
		case EEventAddLibrary:
			{
			DLibrary* pL = (DLibrary*)a1;
			if (pL->iMapCount == 1)
				AddObject(ELibrary, pL);
			}
			break;
		case EEventRemoveLibrary:
			{
			DLibrary* pL = (DLibrary*)a1;
			if (pL->iMapCount == 0)
				RemoveObject(ELibrary, pL);
			}
			break;
		case EEventNewChunk:
			AddObject(EChunk, (DObject*)a1);
			break;
		case EEventDeleteChunk:
			RemoveObject(EChunk, (DObject*)a1);
			break;
		case EEventAddCodeSeg:
			{
			AddCodeSeg((DCodeSeg*)a1, (DProcess*)a2);
			}
			break;
		case EEventRemoveCodeSeg:
			{
			RemoveCodeSeg((DCodeSeg*)a1, (DProcess*)a2);
			}
			break;
		case EEventLoadedProcess:
			{
			__KTRACE_OPT(KDEBUGGER, Kern::Printf("EVENTTRACKER: Process %O loaded", a1));
			ProcessLoaded((DProcess*)a1);
			}
			break;
		case EEventUnloadingProcess:
			__KTRACE_OPT(KDEBUGGER, Kern::Printf("EVENTTRACKER: Process %O unloaded", a1));
			break;
		default:
			// no-op
			__KTRACE_OPT(KDEBUGGER, Kern::Printf("EVENTTRACKER: Handling default case"));
			break;
			}
		}

	Kern::MutexSignal(*iLock);

	// Allow other handlers to see this event
	return DKernelEventHandler::ERunNext;
	}


void DEventTracker::AddObject(TObjectType aType, DObject* aObject)
	{
	TTrackedObject* trackedObject = (TTrackedObject*)LookupItem(aObject);

	if (trackedObject)
		{
		Kern::Printf("EVENTTRACKER: Found orphaned object %O in tracking list while adding new object", aObject);
		++iErrorCount;
		return;
		}

	NKern::ThreadEnterCS();
	trackedObject = new TTrackedObject(aObject, aType);
	NKern::ThreadLeaveCS();

	if (trackedObject)
		{
		__KTRACE_OPT(KDEBUGGER, Kern::Printf("EVENTTRACKER: Adding %O (type %d) to tracking list", aObject, aType));
		__KTRACE_OPT(KDEBUGGER, Kern::Printf("EVENTTRACKER: DBase ptr == 0x%x", trackedObject->iObject));
		iItems.Add(&trackedObject->iLink);
		}
	else
		{
		iOOM = ETrue;
		++iErrorCount;
		}
	}


void DEventTracker::RemoveObject(TObjectType aType, DObject* aObject)
	{
	TTrackedObject* const trackedObject = (TTrackedObject*)LookupItem(aObject);

	if (trackedObject)
		{
 		TFullName name;
		aObject->FullName(name);
		if (!trackedObject->CheckIntegrity(name, aType))
			{
			++iErrorCount;
			}
		__KTRACE_OPT(KDEBUGGER, Kern::Printf("EVENTTRACKER: Removing %S (type %d) from tracking list", &name, aType));
		trackedObject->iLink.Deque();

		NKern::ThreadEnterCS();
		delete trackedObject;
		NKern::ThreadLeaveCS();
		}
	else
		{
		Kern::Printf("EVENTTRACKER: %O (type %d) removed but not in tracking list", aObject, aType);
		++iErrorCount;
		}
	}


void DEventTracker::UpdateObject(TObjectType aType, DObject* aObject, TBool aMustBeRenamed)
	{
	TTrackedObject* const trackedObject = (TTrackedObject*)LookupItem(aObject);

	if (trackedObject)
		{
		TFullName newName;
		aObject->FullName(newName);
		if (newName != trackedObject->iFullName)
			{
			__KTRACE_OPT(KDEBUGGER, Kern::Printf("EVENTTRACKER: Renaming %S --> %S (type %d)", 
						&trackedObject->iFullName, &newName));
			trackedObject->iFullName = newName;
			}
		else if (aMustBeRenamed)
			{
			Kern::Printf("EVENTTRACKER: %O (type %d) renamed with same name", aObject, aType);
			++iErrorCount;
			}
		}
	else
		{
		Kern::Printf("EVENTTRACKER: %O (type %d) updated but not in tracking list", aObject, aType);
		Kern::Printf("EVENTTRACKER: DBase ptr == 0x%x", (DBase*)aObject);
		++iErrorCount;
		}
	}

void DEventTracker::AddCodeSeg(DCodeSeg* aCodeSeg, DProcess* aProcess)
	{
	TTrackedCodeSeg* trackedCodeSeg = (TTrackedCodeSeg*)LookupItem(aCodeSeg);

	if (trackedCodeSeg)
		{
		if (aProcess && (aProcess->iTempCodeSeg == aCodeSeg))
			{
			// This is the exe code seg for a loading process
			// and hence the access count is currently
			// incremented by one
			++trackedCodeSeg->iAccessCount;
			}

		if (trackedCodeSeg->iAccessCount != aCodeSeg->iAccessCount)
			{
			Kern::Printf(
				"EVENTTRACKER: Access count for %C (%d) does not match the tracking list (%d)",
				aCodeSeg,
				aCodeSeg->iAccessCount,
				trackedCodeSeg->iAccessCount
				);
			++iErrorCount;
			return;
			}
		}

	if (!trackedCodeSeg)
		{
		NKern::ThreadEnterCS();
		trackedCodeSeg = new TTrackedCodeSeg(aCodeSeg);
		NKern::ThreadLeaveCS();

		if (trackedCodeSeg)
			{
			__KTRACE_OPT(KDEBUGGER, Kern::Printf("EVENTTRACKER: Adding %C to tracking list", aCodeSeg));
			iItems.Add(&trackedCodeSeg->iLink);
			}
		}
	else // trackedCodeSeg
		{
		if (aProcess)
			{
			__KTRACE_OPT(KDEBUGGER, Kern::Printf("EVENTTRACKER: Updating access count for %C (%d), attaching to process %O", aCodeSeg, aCodeSeg->iAccessCount, aProcess));
			}
		else // !aProcess
			{
			Kern::Printf("EVENTTRACKER: Found orphaned code seg %C in tracking list while adding new code seg", aCodeSeg);
			++iErrorCount;
			}
		}

	if (!trackedCodeSeg)
		{
		iOOM = ETrue;
		++iErrorCount;
		}
	}

void DEventTracker::ProcessLoaded(DProcess* aProcess)
	{
	if (aProcess->iCodeSeg)
		{
		TTrackedCodeSeg* trackedCodeSeg = (TTrackedCodeSeg*)LookupItem(aProcess->iCodeSeg);

		if (trackedCodeSeg)
			{
			// This is the exe code seg for a process that
			// has completed loading and hence the access
			// count has just been decremented by one
			--trackedCodeSeg->iAccessCount;
			}
		}
	}

void DEventTracker::RemoveCodeSeg(DCodeSeg* aCodeSeg, DProcess* aProcess)
	{
	TTrackedCodeSeg* const trackedCodeSeg = (TTrackedCodeSeg*)LookupItem(aCodeSeg);

	if (trackedCodeSeg)
		{
		if (!trackedCodeSeg->CheckIntegrity(aCodeSeg->iAccessCount))
			{
			++iErrorCount;
			}

		if (aCodeSeg->iAccessCount == 1)
			{
			__KTRACE_OPT(KDEBUGGER, Kern::Printf("EVENTTRACKER: Removing %C from tracking list, process %O", aCodeSeg, aProcess));
			trackedCodeSeg->iLink.Deque();

			NKern::ThreadEnterCS();
			delete trackedCodeSeg;
			NKern::ThreadLeaveCS();
			}
		}
	else
		{
		Kern::Printf("EVENTTRACKER: %C removed but not in tracking list.  Removing from process %O", aCodeSeg, aProcess);
		++iErrorCount;
		}
	}


/** Add all objects from relevant containers into the tracking list.  */

TInt DEventTracker::AddExistingObjects()
	{
	// Tracking can be started only after all containers read to avoid 
	// race conditions.
	__ASSERT_DEBUG(!iTracking, Kern::Fault(KPanicCat, __LINE__));

	TInt err = KErrNone;
	Kern::Printf("Adding processes");
	err = AddObjectsFromContainer(EProcess);
	if (err)
		{
		return err;
		}
	Kern::Printf("Adding threads");
	err = AddObjectsFromContainer(EThread);
	if (err)
		{
		return err;
		}
	Kern::Printf("Adding libraries");
	err = AddObjectsFromContainer(ELibrary);
	if (err)
		{
		return err;
		}
	Kern::Printf("Adding chunks");
	err = AddObjectsFromContainer(EChunk);
	if (err)
		{
		return err;
		}
	Kern::Printf("Adding LDDs");
	err = AddObjectsFromContainer(ELogicalDevice);
	if (err)
		{
		return err;
		}
	Kern::Printf("Adding PDDs");
	err = AddObjectsFromContainer(EPhysicalDevice);
	if (err)
		{
		return err;
		}
	Kern::Printf("Adding code segs");
	return AddCodeSegsFromList();
	}

/** Add all objects from specified container into tracking list.  */

TInt DEventTracker::AddObjectsFromContainer(TObjectType aType)
	{
	DObjectCon* const container = Kern::Containers()[aType];

	NKern::ThreadEnterCS();
	container->Wait();
	
	const TInt count = container->Count();
	TInt err = KErrNone;
	
	for (TInt i = 0; (i < count && err == KErrNone); ++i)
		{
		DObject* const object = (*container)[i];
		if (object->Open() == KErrNone)
			{
			AddObject(aType, object);
			if (iOOM)
				{
				err = KErrNoMemory;
				}
			object->Close(NULL);
			}
		}

	container->Signal();
	NKern::ThreadLeaveCS();

	return err;
	}

TInt DEventTracker::AddCodeSegsFromList()
	{
	Kern::AccessCode();

	const SDblQueLink* const anchor = &Kern::CodeSegList()->iA;
	for (SDblQueLink* link = Kern::CodeSegList()->First(); link != anchor; link = link->iNext)
		{
		DCodeSeg* const codeSeg = _LOFF(link, DCodeSeg, iLink);
		AddCodeSeg(codeSeg, NULL);
		}

	Kern::EndAccessCode();

	return KErrNone;
	}


/** Check that tracking list matches existing objects. 
	@return number of discrepancies found
 */

TInt DEventTracker::CheckIntegrity()
	{
	// Tracking must be stopped to avoid race conditions.
	__ASSERT_DEBUG(!iTracking, Kern::Fault(KPanicCat, __LINE__));

	if (iOOM)
		{
		Kern::Printf("EVENTTRACKER: OOM during tracking");
		}

	CheckContainerIntegrity(EProcess);
	CheckContainerIntegrity(EThread);
	CheckContainerIntegrity(ELibrary);
	CheckContainerIntegrity(EChunk);
	CheckContainerIntegrity(ELogicalDevice);
	CheckContainerIntegrity(EPhysicalDevice);
	CheckCodeSegListIntegrity();

	CheckAllAccountedFor();

	if (iErrorCount)
		{
		Kern::Printf("EVENTTRACKER: %d error(s) found", iErrorCount);
		return KErrGeneral;
		}

	return KErrNone;
	}

/** Check all objects in specified container are in tracking list.  */

void DEventTracker::CheckContainerIntegrity(TObjectType aType)
	{
	DObjectCon* const container = Kern::Containers()[aType];

	NKern::ThreadEnterCS();
	container->Wait();

	const TInt count = container->Count();
	
	for (TInt i = 0; i < count; ++i)
		{
		DObject* const object = (*container)[i];
		if (object->Open() == KErrNone)
			{
			TFullName name;
			object->FullName(name);

			TTrackedObject* const trackedObject = (TTrackedObject*)LookupItem(object);
			
			if (trackedObject)
				{
				trackedObject->iAccountedFor = ETrue;
				if (!trackedObject->CheckIntegrity(name, aType))
					{
					++iErrorCount;
					}
				}
			else
				{
				Kern::Printf("EVENTTRACKER: %S (type %d) is in container but not in tracking list", &name, aType);
				++iErrorCount;
				}

			object->Close(NULL);
			}
		}

	container->Signal();
	NKern::ThreadLeaveCS();
	}

void DEventTracker::CheckCodeSegListIntegrity()
	{
	Kern::AccessCode();

	const SDblQueLink* const anchor = &Kern::CodeSegList()->iA;
	for (SDblQueLink* link = Kern::CodeSegList()->First(); link != anchor; link = link->iNext)
		{
		DCodeSeg* const codeSeg = _LOFF(link, DCodeSeg, iLink);
		TTrackedCodeSeg* const trackedCodeSeg = (TTrackedCodeSeg*)LookupItem(codeSeg);

		if (trackedCodeSeg)
			{
			trackedCodeSeg->iAccountedFor = ETrue;
			if (!trackedCodeSeg->CheckIntegrity(codeSeg->iAccessCount))
				{
				++iErrorCount;
				}
			}
		else
			{
			Kern::Printf("EVENTTRACKER: %C is in global list but not in tracking list", codeSeg);
			++iErrorCount;
			}
		}

	Kern::EndAccessCode();
	}


/** Check that all objects in tracking list have been accounted for. */
void DEventTracker::CheckAllAccountedFor()
	{
	const SDblQueLink* link = iItems.GetFirst();
	while (link)
		{
		TTrackedItem* const item = _LOFF(link, TTrackedItem, iLink);
		if (!item->iAccountedFor)
			{
			Kern::Printf(
				"EVENTTRACKER: 0x%x is in tracking list but not in container / list", 
				&item->iObject
				);
			++iErrorCount;
			}
		link = iItems.GetFirst();
		}
	}

/** Look for specified object in the tracking list. 
	@pre iLock held
	@post iLock held
 */
TTrackedItem* DEventTracker::LookupItem(DBase* aItem) const
	{
	const SDblQueLink* const anchor = &iItems.iA;
	
	for (SDblQueLink* link = iItems.First(); link != anchor; link = link->iNext)
		{
		TTrackedItem* const item = _LOFF(link, TTrackedItem, iLink);

		if (item->iObject == aItem)
			{
			return item;
			}
		}

	return NULL;
	}


void DEventTracker::DumpCounters() const
	{
	static const char* const KEventName[] = 
		{
		"SwExc           ",
		"HwExc           ",
		"AddProcess      ",
		"UpdateProcess   ",
		"RemoveProcess   ",
		"AddThread       ",
		"StartThread     ",
		"UpdateThread    ",
		"KillThread      ",
		"RemoveThread    ",
		"NewChunk        ",
		"UpdateChunk     ",
		"DeleteChunk     ",
		"AddLibrary      ",
		"RemoveLibrary   ",
		"LoadLdd         ",
		"UnloadLdd       ",
		"LoadPdd         ",
		"UnloadPdd       ",
		"UserTrace       ",
		"AddCodeSeg      ",
		"RemoveCodeSeg   ",
		"LoadedProcess   ",
		"UnloadingProcess"
		};

	Kern::Printf("EVENT USAGE STATISTICS:");

	for (TInt i = 0; i < EEventLimit; ++i)
		{
		Kern::Printf("\t%s\t\t %d times", KEventName[i], iCounters[i]);
		}
	}

#ifdef __MARM__
void DEventTracker::CopyDummyHandler(TLinAddr aLinAddr)
	{
	const TUint handlerSize = DummyHandlerSize();

	// Copy the breakpoint-able handler into RAM by copying from the one (possibly) in ROM
	memcpy((TAny*)aLinAddr, (TAny*) &DummyHandler, handlerSize);
	__KTRACE_OPT(KBOOT, Kern::Printf("Breakpoint-able handler copied from 0x%x to (va) 0x%x, size %d", &DummyHandler, aLinAddr, handlerSize));
	}
#endif
	
//////////////////////////////////////////////////////////////////////////////

class DTestChannel : public DLogicalChannelBase
	{
public:
	virtual ~DTestChannel();
protected:
	// from DLogicalChannelBase
	virtual TInt DoCreate(TInt aUnit, const TDesC8* aInfo, const TVersion& aVer);
	virtual TInt Request(TInt aFunction, TAny* a1, TAny* a2);
private:
	DEventTracker* iHandler;
	};


// called in thread critical section
TInt DTestChannel::DoCreate(TInt aUnit, const TDesC8* /*aInfo*/, const TVersion& /*aVer*/)
	{
	if((TUint)aUnit>=2)
		return KErrNotSupported;

	TBool useHook = aUnit;

	iHandler = new DEventTracker;

	if (!iHandler)
		{
		return KErrNoMemory;
		}

	return iHandler->Create(iDevice, useHook);
	}

// called in thread critical section
DTestChannel::~DTestChannel()
	{
	if (iHandler)
		{
		iHandler->Close();
		}
	}


TInt DTestChannel::Request(TInt aFunction, TAny* /*a1*/, TAny* /*a2*/)
	{
	TInt r = KErrNone;
	switch (aFunction)
		{
	case REventTracker::EStart:
		iHandler->Start();
		break;
	case REventTracker::EStop:
		iHandler->Stop();
		break;
	default:
		Kern::PanicCurrentThread(KClientPanicCat, __LINE__);
		break;
		}
	return r;
	}


//////////////////////////////////////////////////////////////////////////////

class DTestFactory : public DLogicalDevice
	{
public:
	DTestFactory();
	// from DLogicalDevice
	virtual TInt Install();
	virtual void GetCaps(TDes8& aDes) const;
	virtual TInt Create(DLogicalChannelBase*& aChannel);
	};

DTestFactory::DTestFactory()
    {
    iVersion = REventTracker::Version();
    iParseMask = KDeviceAllowUnit;
    iUnitsMask = 0x3;
    }

TInt DTestFactory::Create(DLogicalChannelBase*& aChannel)
    {
	aChannel = new DTestChannel;
	return (aChannel ? KErrNone : KErrNoMemory);
    }

TInt DTestFactory::Install()
    {
    return SetName(&KTestLddName);
    }

void DTestFactory::GetCaps(TDes8& /*aDes*/) const
    {
    }

//////////////////////////////////////////////////////////////////////////////

DECLARE_STANDARD_LDD()
	{
    return new DTestFactory;
	}
