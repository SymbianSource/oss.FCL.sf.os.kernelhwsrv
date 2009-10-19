// Copyright (c) 2006-2009 Nokia Corporation and/or its subsidiary(-ies).
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
// Purpose: The DProcessTracker object tracks which processes are being
// debugged. The DProcessTracker class uses a DTargetProcess object for
// each process being debugged.
// Note: Although TheDProcessTracker object is a global, it should be unique
// as only the Debug Security Server should load and use this driver.
// 
//

#include <e32def.h>
#include <e32def_private.h>
#include <e32cmn.h>
#include <e32cmn_private.h>
#include <kernel/kernel.h>
#include <kernel/kern_priv.h>
#include "nk_priv.h"
#include <rm_debug_api.h>

#include "d_target_process.h"
#include "debug_logging.h"
#include "debug_utils.h"

// ctor
DTargetProcess::DTargetProcess()
	:iProcessName(0,0,0)
	{
	}

// dtor
DTargetProcess::~DTargetProcess()
	{
	// Delete the space allocated for the name if any
	if (iProcessName.Ptr() != 0)
		{
		NKern::ThreadEnterCS();
		Kern::Free((TAny*)iProcessName.Ptr());
		NKern::ThreadLeaveCS();
		}
	//Reset the array and delete the objects that its members point to
	NKern::ThreadEnterCS();
	iAgentList.ResetAndDestroy();
	NKern::ThreadLeaveCS();
	}

// Compare two DTargetProcess items. They are the same if they have the same name.
TInt DTargetProcess::Compare(const DTargetProcess& aFirst, const DTargetProcess& aSecond)
	{
	return aFirst.iProcessName.Compare(aSecond.iProcessName);
	}

// Set the name of the process we are tracking
TInt DTargetProcess::SetProcessName(const TDesC8& aProcessName)
	{
	// Argument checking
	if (aProcessName.Length() < 1)
		{
		return KErrArgument;
		}

	// Allocate some memory to store the name
	TUint length = aProcessName.Length();

	NKern::ThreadEnterCS();
	TUint8* buffer = (TUint8*)Kern::AllocZ(length);
	NKern::ThreadLeaveCS();
	if (buffer==NULL)
		{
		// Out of memory
		return KErrNoMemory;
		}

	// Set iProcessName to use the alloc'd buffer
	iProcessName.Set(buffer,length,length);

	// Store aProcessName within this object
	iProcessName.Copy(aProcessName);

	return KErrNone;
	}

// Obtain the name of the process being tracked
const TPtr8& DTargetProcess::ProcessName(void)
	{
	return iProcessName;
	}

// Returns a pointer to the DDebugAgent with aAgentId.
// If the agent is not in the list, it returns NULL.
DDebugAgent* DTargetProcess::Agent(TUint64 aAgentId)
	{
	for(TInt i = 0; i < iAgentList.Count(); i++)
	{
		if (iAgentList[i]->Id() == aAgentId)
		{
			return iAgentList[i];
		}
	}

	// what do we return if we don't have any agents?
	return NULL;
	}

// Adds aAgentId as a tracking agent for this process.
TInt DTargetProcess::AddAgent(TUint64 aAgentId)
	{
	LOG_MSG("DTargetProcess::AddAgent()");
	DDebugAgent* agent = DDebugAgent::New(aAgentId);
	if(agent == NULL)
		{
		LOG_MSG("DTargetProcess::AddAgent() couldn't allocate memory for DDebugAgent");
		return KErrNoMemory;
		}
	return iAgentList.Insert(agent,0);
	}

// Stops tracking the process with this agent
TInt DTargetProcess::RemoveAgent(TUint64 aAgentId)
	{
	// We need to find and then remove the agent
	for(TUint i = 0; i < iAgentList.Count(); i++)
		{
		if (iAgentList[i]->Id() == aAgentId)
			{
			delete iAgentList[i];
			iAgentList.Remove(i);
			return KErrNone;
			}
		}

	return KErrNotFound;
	}

// Index through the agents by position
DDebugAgent* DTargetProcess::operator[](TInt aIndex)
	{
	return iAgentList[aIndex];
	}

// returns the number of agents tracking this process.
TInt DTargetProcess::AgentCount(void)
	{
	return iAgentList.Count();
	}

/**
  Resume the specified thread

  @param aThread thread to resume

  @return KErrNone if the thread has previously been suspended and is resumed,
  KErrNotFound if the thread has not previously been suspended
  */
TInt DTargetProcess::ResumeThread(DThread* aThread)
	{
	LOG_MSG2("DTargetProcess::ResumeSuspendedThread(): thread=0x%08x", aThread);
	TInt err1 = ResumeSuspendedThread(aThread);
	LOG_MSG2("DTargetProcess::ResumeSuspendedThread(): ret=%d)", err1); 
	TInt err2 = ResumeFrozenThread(aThread->iNThread);
	LOG_MSG2("DTargetProcess::ResumeFrozenThread(): ret=%d)", err2);
	//if resuming the suspended thread failed for an obscure reason return it
	if((err1 != KErrNotFound) && (err1 != KErrNone))
		{
		LOG_MSG2("DTargetProcess::ResumeThread() unexpected exit, err1: %d", err1);
		return err1;
		}
	//if resuming the frozen thread failed for an obscure reason return it
	if((err2 != KErrNotFound) && (err2 != KErrNone))
		{
		LOG_MSG2("DTargetProcess::ResumeThread() unexpected exit, err2: %d", err2);
		return err2;
		}
	// if resuming the suspended thread succeeded in both cases, we have a consistency problem
	if ((err1 == KErrNone) && (err2 == KErrNone))
		{
		LOG_MSG("DTargetProcess::ResumeThread() unexpected exit, err1 == err2 == KErrNone");
		}

	//if the thread was in neither list return KErrNotFound, otherwise KErrNone
	return ((err1 == KErrNone) || (err2 == KErrNone)) ? KErrNone : KErrNotFound;
	}

/**
  Resume the specified frozen thread

  @param aThread thread to resume

  @return KErrNone if the thread has previously been suspended and is resumed,
  KErrNotFound if the thread has not previously been suspended
  */
TInt DTargetProcess::ResumeFrozenThread(NThread& aThread)
	{
	for(TInt i=0; i<iFrozenThreadSemaphores.Count(); i++)
		{
		if(iFrozenThreadSemaphores[i]->iOwningThread == &aThread)
			{
			NKern::FSSignal(iFrozenThreadSemaphores[i]);
			NKern::ThreadEnterCS();
			delete iFrozenThreadSemaphores[i];
			NKern::ThreadLeaveCS();
			iFrozenThreadSemaphores.Remove(i);
			return KErrNone;
			}
		}
	return KErrNotFound;
	}

/**
  Resume the specified suspended thread

  @param aThread thread to resume

  @return KErrNone if the thread has previously been suspended and is resumed,
  KErrNotFound if the thread has not previously been suspended
  */
TInt DTargetProcess::ResumeSuspendedThread(DThread* aThread)
	{
	TUint64 threadId = (TUint64)aThread->iId;
	for(TInt i=0; i<iSuspendedThreads.Count(); i++)
		{
		if(iSuspendedThreads[i] == threadId)
			{
			iSuspendedThreads.Remove(i);
			LOG_MSG2("DTargetProcess::ResumeSuspendedThread()> Kern::ThreadResume() 0x%x", aThread);
			Kern::ThreadResume(*aThread);
			return KErrNone;
			}
		}
	return KErrNotFound;
	}

/**
  Suspend the specified thread

  @param aThread thread to suspend

  @param aFreezeThread suspend the thread on a Fast Semaphore if
  ETrue. EFalse means suspend by calling Kern::Suspend.

  @return KErrNone if the thread is successfully suspended,
  KErrAlreadyExists if the agent has already suspended the thread,
  or one of the other system wide error codes
  
   This function suspends a thread by calling Kern::Thread Suspend.
                                                                                                       
  An alternative means of suspending the _current_ thread only
  is by call DTargetProcess::FreezeThread. This will ensure that
  the current thread is suspended when exception processing for this
  thread completes (see rm_debug_eventhandler.cpp)
  
  */
TInt DTargetProcess::SuspendThread(DThread* aThread, TBool aFreezeThread)
	{
	// should check if this thread is already suspended/frozen and return if so
	// but just warn for the moment.
	if (CheckSuspended(aThread))
		{
		// thread was already suspended, don't bother doing it again
		LOG_MSG2("DTargetProcess::SuspendThread - Thread Id 0x%08x already suspended\n",aThread->iId);
		//return KErrAlreadyExists;	
		}

	return aFreezeThread ? FreezeThread() : DoSuspendThread(aThread);
	}

/**
  Freeze the current thread

  @return KErrNone if the thread is successfully suspended,
  KErrAlreadyExists if the agent has already suspended the thread,
  or one of the other system wide error codes

  This marks the current thread for waiting on a Fast Semaphore
  when exception handling for this thread has completed - see
  rm_debug_eventhandler.cpp for details.
  */
TInt DTargetProcess::FreezeThread()
	{
	// create and store a fast semaphore to stop the thread on
	NKern::ThreadEnterCS();
	NFastSemaphore* sem = new NFastSemaphore();
	NKern::ThreadLeaveCS();
	sem->iOwningThread = &(Kern::CurrentThread().iNThread);
	LOG_EVENT_MSG2("DTargetProcess::FreezeThread(): new NFastSemaphore() curr thread=0x%x", sem->iOwningThread);
	return iFrozenThreadSemaphores.Append(sem);
	}

/**
  Suspend the specified thread

  @param aThread thread to suspend

  @return KErrNone if the thread is successfully suspended,
  KErrAlreadyExists if the agent has already suspended the thread,
  or one of the other system wide error codes
  */
TInt DTargetProcess::DoSuspendThread(DThread* aThread)
	{
	TUint64 threadId = (TUint64)aThread->iId;
	
	// Don't suspend if this thread is already suspended (by FSWait or
	// Kern::ThreadSuspend
	if (CheckSuspended(aThread))
		{
		// thread was already suspended, don't bother doing it again
		LOG_MSG2("DTargetProcess::SuspendThread - Thread Id 0x%08x already suspended\n",threadId);
		return KErrAlreadyExists;	
		}

	// Add thread to the suspend list
	TInt err = iSuspendedThreads.Append(threadId);
	if(err == KErrNone)
		{
		Kern::ThreadSuspend(*aThread, 1);
		}
	return err;
	}

/**
 Waits the current thread on a Fast Semaphore.

 This is useful for situations where the current thread
 has hit a breakpoint within a critical section, and
 otherwise could not be suspended at this point.

 Note that the Fast Semaphore structure on which the thread
 waits must be a member data item of this class instance,
 as it needs to be FSSignal()'d by another thread to resume
 again.
 */
void DTargetProcess::FSWait()
	{
	LOG_MSG2("DTargetProcess::NotifyEvent(): number of attached agents: %d", AgentCount());
	NThread* currentThread = &(Kern::CurrentThread().iNThread);
	for(TInt i=0; i<iFrozenThreadSemaphores.Count(); i++)
		{
		if(iFrozenThreadSemaphores[i]->iOwningThread == currentThread)
			{
			LOG_MSG3("DTargetProcess::FSWait(): > FSWait frozen sem %d, curr thread=0x%x", i, currentThread);
			NKern::FSWait(iFrozenThreadSemaphores[i]);
			return;
			}
		}
	}

/**
  Checks that the thread has been suspended

  @param aThread thread to check suspended

  @return ETrue if the thread has been suspended,
  EFalse if the thread has not been suspended
  */
TBool DTargetProcess::CheckSuspended(DThread* aThread) const
	{
	if(!aThread)
		{
		return EFalse;
		}
	//check if the thread is in the suspended threads list
	for(TInt i=0; i<iSuspendedThreads.Count(); i++)
		{
		if(iSuspendedThreads[i] == (TUint64)aThread->iId)
			{
			return ETrue;
			}
		}
	// not in the suspended threads list so check in the frozen threads list
	NThread* nThread = &(aThread->iNThread);
	for(TInt i=0; i<iFrozenThreadSemaphores.Count(); i++)
		{
		if(iFrozenThreadSemaphores[i]->iOwningThread == nThread)
			{
			return ETrue;
			}
		}
	return EFalse;
	}

/*
@return ETrue if the debug driver has suspended any of the process' threads, EFalse otherwise
*/
TBool DTargetProcess::HasSuspendedThreads() const
	{
	return (iSuspendedThreads.Count() > 0) || (iFrozenThreadSemaphores.Count() > 0);
	}

void DTargetProcess::NotifyEvent(const TDriverEventInfo& aEventInfo)
	{
	// Stuff the event info into all the tracking agents event queues
	LOG_MSG2("DTargetProcess::NotifyEvent(): number of attached agents: %d", AgentCount());

	for(TInt i = 0; i < AgentCount(); i++)
		{
		// Index through all the relevant debug agents
		DDebugAgent* debugAgent = iAgentList[i];
		if(debugAgent != NULL)
			{
			debugAgent->NotifyEvent(aEventInfo);
			}
		}
	}

