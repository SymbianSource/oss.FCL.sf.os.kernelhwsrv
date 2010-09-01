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
// Note: Although TheDProcessTracker object is a global, it will be unique
// as only the Debug Security Server can load and use rm_debug.ldd.
// 
//

#include <e32def.h>
#include <e32def_private.h>
#include <e32cmn.h>
#include <e32cmn_private.h>
#include <kernel/kernel.h>
#include <kernel/kern_priv.h>

#include <rm_debug_api.h>
#include "debug_logging.h"
#include "d_process_tracker.h"
#include "debug_utils.h"

// Global Run-mode debugged process tracking object
DProcessTracker TheDProcessTracker;

// ctor
DProcessTracker::DProcessTracker()
	{
	}

/**
 * dtor
 * @internalTechnology
 */
DProcessTracker::~DProcessTracker()
	{
	// Forget about all the iProcesses
	iProcesses.ResetAndDestroy();
	}

/**
 * @internalTechnology
 *
 * Creates and stores an internal mapping of debug agent to debugged process.
 * Note that an individual process may be mapped to a number of debug agents.
 *
 * @param aProcessName - The fullly qualified path of the debugged process. E.g. z:\sys\bin\hello_world.exe
 * @param aAgentId - The process id of the debug agent which is attaching to aProcessName, as returned by RProcess.Id()
 * @return KErrNone if there are no errors. KErrArgument if the processname is too long/short for a valid filepath.
 *  KErrNoMemory if there is insufficient memory.
 */
TInt DProcessTracker::AttachProcess(const TDesC8& aProcessName,TUint64 aAgentId)
	{
	LOG_MSG("DProcessTracker::AttachProcess()");

	// Valid ProcessName?
	if (aProcessName.Length() < 1 || aProcessName.Length() >= KMaxPath)
		{
		return KErrArgument;
		}

	// Create an DTargetProcess to store
	DTargetProcess* tmpProcess = new DTargetProcess;
	if (tmpProcess == 0)
		{
		return KErrNoMemory;
		}
	LOG_MSG2(" AttachProcess: < new DTargetProcess=0x%08x", tmpProcess );
	
	// Set the name
	TInt err = KErrNone;
	err = tmpProcess->SetProcessName(aProcessName);
	if (err != KErrNone)
		{
		LOG_MSG2(" AttachProcess: < SetProcessName returned %d", err );
		return err;
		}

	// Is this process being debugged (ie already attached?)
	TInt index;
	TBool found = EFalse;
	
	TInt numberOfProcesses = iProcesses.Count();
	for(index=0; index<numberOfProcesses; index++)
		{
		const TPtr8& tmpPtr8(iProcesses[index]->ProcessName() );

		if ( tmpPtr8.CompareF(aProcessName) == 0)
			{
			LOG_MSG3(" Proc count=%d, found proc in iProcesses at %d. Count=%d",
				index, iProcesses.Count() );
			found = ETrue;
			break;
			}
		}

	if (found)
		{
		// Yes, it is being debugged

		// Add the agent to the list of agents for this process
		LOG_MSG3(" > AddAgent(agent id %d) to existing iProcesses[%d]", I64LOW(aAgentId), index ); 

		iProcesses[index]->AddAgent(aAgentId);

		return KErrNone;
		}
	else
		{
		// No, it is not being debugged
			
		// Add the agent to the list of agents for this process
		LOG_MSG2(" > AddAgent(agent %d) to new proc at index 0", I64LOW(aAgentId) ); 

		tmpProcess->AddAgent(aAgentId);

		// Add the process to the list of processes being debugged
		return iProcesses.Insert(tmpProcess,0);
		}
	}

/**
 * @internalTechnology
 * 
 * Removes a previously created mapping between a debug agent and a debugged process,
 * as created by AttachProcess.
 *
 * @param aProcessName - The fully qualified path of the debugged process. E.g. z:\sys\bin\hello_world.exe
 * @param aAgentId - The process id of the debug agent which is attaching to aProcessName, as returned by RProcess.Id()
 * @return KErrNone if there are no problems. KErrArgument if the processname is too long/short for a valid filepath.
 * KErrNotFound if the mapping does not exist (and therefore cannot be removed).
 */
TInt DProcessTracker::DetachProcess(const TDesC8& aProcessName, TUint64 aAgentId)
	{
	// Valid ProcessName?
	if (aProcessName.Length() < 1 || aProcessName.Length() >= KMaxPath)
		{
		return KErrArgument;
		};

	// Are we debugging this process?
	TInt i;
	TBool found = EFalse;
	DTargetProcess* foundProcess = 0;

	TInt numberOfProcesses = iProcesses.Count();
	for(i=0; i<numberOfProcesses; i++)
		{
		foundProcess = iProcesses[i];

		const TPtr8& tmpPtr8( foundProcess->ProcessName() );

		if ( tmpPtr8.CompareF(aProcessName) == 0)
			{
			found = ETrue;
			break;
			}
		}

	if (found == EFalse)
		{
		return KErrNotFound;
		}

	// remove the agent from the process
	iProcesses[i]->RemoveAgent(aAgentId);

	// Found it, are there any more attached agents, or suspended threads in the process?
	if ((iProcesses[i]->AgentCount() == 0) && !iProcesses[i]->HasSuspendedThreads() )
		{
		// Delete the process as no more agents are still attached
		delete iProcesses[i];

		// Remove the now obsolete pointer from our array.
		iProcesses.Remove(i);
		}

	return KErrNone;
	}

/**
 * @internalTechnology
 *
 * Detachs a debug agent from every process being debugged. Used when a debug agent is being detached
 * from the debug security server and has not supplied a specific process name from which to detach.
 */
TInt DProcessTracker::DetachAgent(const TUint64 aAgentId)
	{
	// Remove this agent from all the processes being tracked.
	TInt numberOfProcesses = iProcesses.Count();
	for(TInt i=0; i<numberOfProcesses; i++)
		{
		// remove the agent from the process (we don't care about the return code)
		iProcesses[i]->RemoveAgent(aAgentId);
		}

	// Increment down through the array as we then don't have to worry about
	// missing entries which have been shifted after deletes.
	// The initial value of i correspnds to the index of the final element 
	// in the array.
	for(TInt i = iProcesses.Count()-1; i>=0; i--)
		{
		if (iProcesses[i]->AgentCount() == 0)
			{
			// No agents remain for this process. Delete the
			// process object and remove the pointer from the array
			delete iProcesses[i];
			iProcesses.Remove(i);
			}
		}
	return KErrNone;
	}

/**
 * @internalTechnology
 *
 * Returns a pointer to a DTargetProcess object representing the mapping of a debugged process
 * with all the relevant debug agents interested in that process, as determined
 * by AttachProcess.
 *
 * @param aProcessName - The fully qualified path of the debugged process. E.g. z:\sys\bin\hello_world.exe
 * @return DTargetProcess* pointer to an object representing the internal mapping of a process to all associated
 * debug agents. Returns 0 if the mapping cannot be found or the aProcessName is invalid.
 */
DTargetProcess* DProcessTracker::FindProcess(const TDesC8& aProcessName)
	{
	// Valid ProcessName?
	if (aProcessName.Length() < 1 || aProcessName.Length() >= KMaxPath)
		{
		return 0;	// not found
		}

	// Can we find this in the array?
	TInt i;
	TBool found = EFalse;
	DTargetProcess* foundProcess = 0;

	TInt numberOfProcesses = iProcesses.Count();
	for(i=0; i<numberOfProcesses; i++)
		{
		foundProcess = iProcesses[i];

		const TPtr8& tmpPtr8( foundProcess->ProcessName() );

		if ( tmpPtr8.CompareF(aProcessName) == 0)
			{
			found = ETrue;
			break;
			}
		}

	if (found == EFalse)
		{
		LOG_EVENT_MSG("DProcessTracker::FindProcess, not found" );
		return 0;	// not found
		}

	return foundProcess;
	}

/**
 * @internalTechnology
 *
 * Returns a pointer to a DTargetProcess object representing the mapping of a debugged process
 * with all the relevant debug agents interested in that process, as determined
 * by AttachProcess.
 *
 * Note: This does not attempt an exact match, because the AddProcess event does not provide
 * a fully-qualified path, it provides something like [t_rmdebug_security0.exe].
 *
 * So for the purposes of dealing with this event, we need a "fuzzier" match which does not use the complete
 * path.
 *
 * @param aProcessName - The fully qualified path of the debugged process. E.g. z:\sys\bin\hello_world.exe
 * @return DTargetProcess* pointer to an object representing the internal mapping of a process to all associated
 * debug agents. Returns 0 if the mapping cannot be found or the aProcessName is invalid.
 */
DTargetProcess*	DProcessTracker::FuzzyFindProcess(const TDesC8& aProcessName)
	{
	// Valid ProcessName?
	if (aProcessName.Length() < 1 || aProcessName.Length() >= KMaxPath)
		{
		return 0;	// not found
		}

	// Can we find this in the array?
	TBool found = EFalse;
	DTargetProcess* foundProcess = 0;
	const TChar KBackSlash('\\');

	TInt numberOfProcesses = iProcesses.Count();
	for(TInt i=0; i < numberOfProcesses; i++)
		{
		foundProcess = iProcesses[i];

		TInt procListBackSlash = foundProcess->ProcessName().LocateReverse( KBackSlash );
		if( procListBackSlash == KErrNotFound )
			{
			procListBackSlash = 0;
			}
		else
			{
			//Now move to the char after the backlash
			procListBackSlash++;
			}

		TInt eventBackSlash = aProcessName.LocateReverse( KBackSlash );
		if( eventBackSlash == KErrNotFound )
			{
			eventBackSlash = 0;
			}
		else
			{
			//Now move to the char after the backlash
			eventBackSlash++;
			}

		if( ( procListBackSlash == 0 ) && ( eventBackSlash == 0 ) )
			{
			//There were no backslashes on either name, so no point in continuing
			break;
			}

		TPtrC8 eventCleanName( aProcessName.Mid( eventBackSlash ) );		
		TPtrC8 procListCleanName( foundProcess->ProcessName().Mid( procListBackSlash ) );

		if ( eventCleanName.CompareF( procListCleanName ) == 0 )
			{
			LOG_MSG2("DProcessTracker::FuzzyFindProcess() found a match : process list[%d]", i );
			found = ETrue;
			break;
			}
		}

	if (found == EFalse)
		{
		return 0;	// not found
		}

	return foundProcess;
	}

TBool DProcessTracker::CheckSuspended(DThread* aTargetThread) const
	{
	//get the file name and return if NULL
	HBuf* name = GetFileName(aTargetThread);
	if(!name)
		{
		return EFalse;
		}

	//iterate through the processes trying to match the name, and check suspended if found
	TInt numberOfProcesses = iProcesses.Count();
	for(TInt i=0; i < numberOfProcesses; i++)
		{
		if(iProcesses[i]->ProcessName().CompareF(*name) == 0)
			{
			return iProcesses[i]->CheckSuspended(aTargetThread);
			}
		}

	//couldn't find the process so return EFalse
	return EFalse;
	}

TBool DProcessTracker::CheckSuspended(const TUint64 aTargetThreadId) const
	{
	//get a handle to the thread and return false if it's NULL
	DThread* thread = DebugUtils::OpenThreadHandle(aTargetThreadId);
	if(!thread)
		{
		return EFalse;
		}

	//check if the thread's suspended and then close the thread handle and return
	TBool suspended = CheckSuspended(thread);
	thread->Close(NULL);
	return suspended;
	}

/**
  Attempts to suspend the specified thread

  @param aTargetThread thread to suspend

  @return KErrNone on success, KErrAlreadyExists if the thread is already suspended,
  or one of the other system wide error codes
  */
TInt DProcessTracker::SuspendThread(DThread* aTargetThread, TBool aFreezeThread)
	{
	LOG_MSG3("DProcessTracker::SuspendThread() Requesting suspend for: 0x%08x, freeze thread: %d", aTargetThread->iId, aFreezeThread?1:0);

	//get the file name and return if NULL
	HBuf* name = GetFileName(aTargetThread);
	if(!name)
		{
		return KErrNotFound;
		}

	//iterate through the processes trying to match the name, try to suspend the thread if found
	TInt numberOfProcesses = iProcesses.Count();
	for(TInt i=0; i < numberOfProcesses; i++)
		{
		if(iProcesses[i]->ProcessName().CompareF(*name) == 0)
			{
			return iProcesses[i]->SuspendThread(aTargetThread, aFreezeThread);
			}
		}

	//couldn't find process so return error
	return KErrPermissionDenied;
	}

void DProcessTracker::FSWait()
	{
	TInt numberOfProcesses = iProcesses.Count();
	for(TInt i=0; i < numberOfProcesses; i++)
		{
		iProcesses[i]->FSWait();
		}
	}

/**
  Attempts to resume the specified thread

  @param aTargetThread thread to resume

  @return KErrNone on success, KErrInUse if the thread is not suspended,
  or one of the other system wide error codes
  */
TInt DProcessTracker::ResumeThread(DThread* aTargetThread)
	{
	LOG_MSG2("DProcessTracker::ResumeThread() Requesting resume for: 0x%08x", aTargetThread->iId);

	//get the file name and return if NULL
	HBuf* name = GetFileName(aTargetThread);
	if(!name)
		{
		return KErrNotFound;
		}

	//iterate through the processes trying to match the name, try to resume the thread if found
	TInt numberOfProcesses = iProcesses.Count();
	for(TInt i=0; i < numberOfProcesses; i++)
		{
		if(iProcesses[i]->ProcessName().CompareF(*name) == 0)
			{
			return iProcesses[i]->ResumeThread(aTargetThread);
			}
		}

	//couldn't find process so return error
	return KErrPermissionDenied;
	}

/**
  Get a thread's originating file name

  @param aThread the thread to get the file name for

  @return a pointer to the thread's file name, if there are problems accessing
  the file name then NULL will be returned
  */
HBuf* DProcessTracker::GetFileName(DThread* aThread) const
	{
	//check if the thread is NULL and return if so
	if(!aThread)
		{
		return NULL;
		}

	//get the owning process and return if it is NULL
	DProcess* process = aThread->iOwningProcess;
	if(!process)
		{
		return NULL;
		}

	//get the process' code seg and return if it is NULL
	DCodeSeg* codeSeg = process->iCodeSeg;
	if(!codeSeg)
		{
		return NULL;
		}

	//return the code seg's stored file name (which could theoretically be NULL)
	return codeSeg->iFileName;
	}

