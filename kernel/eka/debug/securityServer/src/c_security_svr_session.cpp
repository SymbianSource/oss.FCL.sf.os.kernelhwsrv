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
// Provides the debug security server session implementation.
// 
//

/**
 @file
 @internalTechnology
 @released
*/

// Needed so we get the text strings for capabilities
#define __INCLUDE_CAPABILITY_NAMES__

#include <e32std.h>
#include <e32std_private.h>
#include <e32btrace.h>
#include <d32btrace.h>
#include <e32base.h>
#include <e32base_private.h>
#include <e32uid.h>
#include <f32file.h>
#include <e32capability.h>
#include <rm_debug_api.h>

// required for direct parsing of e32image/tromimage headers
#include <f32image.h>
#include <e32rom.h>

//added for direct access to media driver
#include <partitions.h>
#include <ftlcontrolio.h>

#include "c_security_svr_session.h"
#include "c_security_svr_server.h"
#include "c_security_svr_async.h"
#include "rm_debug_logging.h"
#ifdef _DEBUG
#include "low_mem_requests.h"
#endif

using namespace Debug;

CSecuritySvrSession::CSecuritySvrSession()
	: iDebugAgentProcessIdStored(EFalse),
	  iDebugAgentProcessId(0),
	  iServerNotified(EFalse),
	  iCrashConnected(EFalse)
	{
	// Ensure that this debug agent has no target capability override
	// by default
	iOEMDebugCapabilities.SetEmpty();
	}

void CSecuritySvrSession::ServiceError(const RMessage2 &aMessage, TInt aError)
	{
	LOG_MSG2("CSecuritySvrSession::ServiceError(), aError: %d\n", aError);

	//insert ending heap markers
	HeapWatcher(aMessage.Function(), EFalse);

	aMessage.Complete(aError);
	}

/**
Called by the client/server framework as part of session creation.

Notifies the server that a session is being created
*/
void CSecuritySvrSession::CreateL()
	{
	LOG_MSG("CSecuritySvrSession::CreateL()\n");

	//notify the server that the session has been opened
	Server().SessionOpened();
	iServerNotified = ETrue;
	}

/**
  Returns a reference to the DSS

  @return a reference to the DSS
  */
CSecuritySvrServer& CSecuritySvrSession::Server() const
    {
    return *static_cast<CSecuritySvrServer*>(const_cast<CServer2*>(CSession2::Server()));
    }

/**
Session destructor. Performs necessary cleanup and notifies the server that the
session is being closed
*/
CSecuritySvrSession::~CSecuritySvrSession()
	{
	LOG_MSG("CSecuritySvrSession::~CSecuritySvrSession!()\n");

	//forced detachment if attached to processes
	if(iDebugAgentProcessIdStored)
		{
		// Cancel any outstanding async objects.
		iAsyncHandlers.ResetAndDestroy();
		
		// Inform the device driver of the agent detach.
		Server().iKernelDriver.DetachAgent(iDebugAgentProcessId.Id());

		LOG_MSG( "CSecuritySvrSession::~CSecuritySvrSession() : -> securityServer.DetachAllProcesses()\n" );
		Server().DetachAllProcesses(iDebugAgentProcessId);
		}

	//notify the server that the session has closed
	if(iServerNotified)
		{
		Server().SessionClosed();
		}
	}

void CSecuritySvrSession::ConstructL()
	{
	// nothing to do
	}

/**
  Used to insert heap checking markers.

  @param aFunction The function that heap markers should be added for
  @param aEntry if ETrue indicates that heap checking is starting, if EFalse
  that heap checking is ending.
  */
void CSecuritySvrSession::HeapWatcher(const TUint32 aFunction, const TBool aEntry) const
	{
	switch(aFunction)
		{
		case EDebugServAttachExecutable:
			return;
		case EDebugServDetachExecutable:
			return;
		case EDebugServSuspendThread:
			return;
		case EDebugServResumeThread:
			return;
// used for out-of-memory testing in debug mode
#ifdef _DEBUG
		// start heap marking in on entry, do nothing on exit
		case EDebugServMarkHeap:
			{
			if(aEntry)
				{
				__UHEAP_MARK;
				}
			return;
			}
		// stop heap marking on exit, do nothing on entry
		case EDebugServMarkEnd:
			{
			if(!aEntry)
				{
				__UHEAP_MARKEND;
				}
			return;
			}
#endif
		default:
			if(aEntry)
				{
				__UHEAP_MARK;
				}
			else
				{
				__UHEAP_MARKEND;
				}
			return;
		}
	}

void CSecuritySvrSession::ServiceL(const RMessage2& aMessage)
//
// Session service handler
//
	{
	//insert starting heap markers
	HeapWatcher(aMessage.Function(), ETrue);

	switch(aMessage.Function())
		{
		case EDebugServResumeThread:
			LOG_MSG( "CSecuritySvrSession::ServiceL() EDebugServResumeThread\n" );
			ResumeThreadL(aMessage);
			break;

		case EDebugServSuspendThread:
			LOG_MSG( "CSecuritySvrSession::ServiceL() EDebugServSuspendThread\n" );
			SuspendThreadL(aMessage);
			break;

		case EDebugServReadMemory:
			LOG_MSG( "CSecuritySvrSession::ServiceL() EDebugServReadMemory\n" );
			ReadMemoryL(aMessage);
			break;

		case EDebugServWriteMemory:
			LOG_MSG( "CSecuritySvrSession::ServiceL() EDebugServWriteMemory\n" );
			WriteMemoryL(aMessage);
			break;

		case EDebugServSetBreak:
			LOG_MSG( "CSecuritySvrSession::ServiceL() EDebugServSetBreak\n" );
			SetBreakL(aMessage);
			break;

		case EDebugServClearBreak:
			LOG_MSG( "CSecuritySvrSession::ServiceL() EDebugServClearBreak\n" );
			ClearBreakL(aMessage);
			break;

		case EDebugServModifyBreak:
			LOG_MSG( "CSecuritySvrSession::ServiceL() EDebugServModifyBreak\n" );
			ModifyBreakL(aMessage);
			break;

		case EDebugServModifyProcessBreak:
			LOG_MSG( "CSecuritySvrSession::ServiceL() EDebugServModifyProcessBreak\n" );
			ModifyProcessBreakL(aMessage);
			break;

		case EDebugServBreakInfo:
			LOG_MSG( "CSecuritySvrSession::ServiceL() EDebugServBreakInfo\n" );
			BreakInfoL(aMessage);
			break;

		case EDebugServReadRegisters:
			LOG_MSG( "CSecuritySvrSession::ServiceL() EDebugServReadRegisters\n" );
			ReadRegistersL(aMessage);
			break;

		case EDebugServWriteRegisters:
			LOG_MSG( "CSecuritySvrSession::ServiceL() EDebugServWriteRegisters\n" );
			WriteRegistersL(aMessage);
			break;

		case EDebugServGetEvent:
			LOG_MSG( "CSecuritySvrSession::ServiceL() EDebugServGetEvent\n" );
			GetEventL(aMessage);
			break;

		case EDebugServCancelGetEvent:
			CancelGetEventL(aMessage);
			break;

		case EDebugServAttachExecutable:
			LOG_MSG( "CSecuritySvrSession::ServiceL() EDebugServAttachExecutable\n" );
			AttachProcessL(aMessage);
			break;

		case EDebugServDetachExecutable:
			LOG_MSG( "CSecuritySvrSession::ServiceL() EDebugServDetachExecutable\n" );
			DetachProcessL(aMessage);
			break;

		case EDebugServGetDebugFunctionalityBufSize:
			LOG_MSG( "CSecuritySvrSession::ServiceL() EDebugServGetDebugFunctionalityBufSize\n" );
			GetDebugFunctionalityBufSizeL(aMessage);
			break;

		case EDebugServGetDebugFunctionality:
			LOG_MSG( "CSecuritySvrSession::ServiceL() EDebugServGetDebugFunctionality\n" );
			GetDebugFunctionalityL(aMessage);
			break;

		case EDebugServSetEventAction:
			LOG_MSG( "CSecuritySvrSession::ServiceL() EDebugServSetEventAction\n" );
			SetEventActionL(aMessage);
			break;

		case EDebugServGetList:
			LOG_MSG( "CSecuritySvrSession::ServiceL() EDebugServGetList\n" );
			GetListL(aMessage);
			break;

		case EDebugServStep:
			LOG_MSG("CSecuritySvrSession::ServiceL() EDebugServStep\n");
			StepL(aMessage);
			break;

		case EDebugServSetProcessBreak:
			LOG_MSG("CSecuritySvrSession::ServiceL() EDebugServSetProcessBreak\n");
			SetProcessBreakL(aMessage);
			break;
		
		case EDebugServProcessBreakInfo:
			LOG_MSG("CSecuritySvrSession::ServiceL() EDebugServProcessBreakInfo\n");
			ProcessBreakInfoL(aMessage);
			break;

		case EDebugServKillProcess:
			LOG_MSG("CSecuritySvrSession::ServiceL() EDebugServKillProcess\n");
			KillProcessL(aMessage);
			break;

#ifdef _DEBUG
		case EDebugServMarkHeap:
			LOG_MSG("CSecuritySvrSession::ServiceL() EDebugServMarkHeap\n");
			// all taken care of in HeapWatcher
			aMessage.Complete(KErrNone);
			break;

		case EDebugServMarkEnd:
			LOG_MSG("CSecuritySvrSession::ServiceL() EDebugServMarkEnd\n");
			// all taken care of in HeapWatcher
			aMessage.Complete(KErrNone);
			break;

		case EDebugServFailAlloc:
			LOG_MSG("CSecuritySvrSession::ServiceL() EDebugServFailAlloc\n");
			DoFailAlloc(aMessage);
			break;
#endif
		case EDebugServReadCrashFlash:
			ReadCrashLogL(aMessage);
			break;		
		case EDebugServWriteCrashFlash:
			LOG_MSG("CSecuritySvrSession::ServiceL() EDebugServWriteCrashFlash\n");
			WriteCrashConfigL(aMessage);
			break;
		case EDebugServEraseCrashFlash:
			LOG_MSG("CSecuritySvrSession::ServiceL() EDebugServEraseCrashFlash\n");
			EraseCrashLogL(aMessage);
			break;
		case EDebugServEraseEntireCrashFlash:
			LOG_MSG("CSecuritySvrSession::ServiceL() EDebugServEraseEntireCrashFlash\n");
			EraseEntireCrashLogL(aMessage);
			break;
		default:
			LOG_MSG( "CSecuritySvrSession::ServiceL() Unknown request, calling User::Leave(KErrNotSupported);\n" );
			User::Leave(KErrNotSupported);
			break;
		}

	//insert ending heap markers
	HeapWatcher(aMessage.Function(), EFalse);
	}

#ifdef _DEBUG
/**
  Used to control heap failure in debug mode.
  @param aMessage If aMessage.Int0 is non-zero then heap will be set to fail on that allocation.
  If aMessage.Int0 is zero then the heap failure count is reset
  */
void CSecuritySvrSession::DoFailAlloc(const RMessage2& aMessage)
	{
	TInt count = aMessage.Int0();
	if(count == 0)
		{
		__UHEAP_RESET;
		}
	else
		{
		__UHEAP_FAILNEXT(count);
		}
	aMessage.Complete(KErrNone);
	}
#endif

/**
Suspends execution of the specified thread.

@param aMessage contains an integer representation of the target thread's
       thread ID at offset 0.

@leave KErrPermissionDenied if security check fails or KErrArgument if the
       thread does not exist
*/
void CSecuritySvrSession::SuspendThreadL(const RMessage2& aMessage)
	{

	LOG_MSG( "CSecuritySvrSession::SuspendThreadL()\n" );

	//get thread ID
	TThreadId threadId = ReadTThreadIdL(aMessage, 0);
	//check attached
	CheckAttachedL(threadId, aMessage, EFalse);

	//security check passed so can perform actions
	User::LeaveIfError(Server().iKernelDriver.SuspendThread(threadId));

	aMessage.Complete(KErrNone);
	}

/**
Resumes execution of the specified thread.

@param aMessage contains an integer representation of the target thread's
       thread ID at offset 0.

@leave KErrPermissionDenied if security check fails or KErrArgument if the
       thread does not exist
*/
void CSecuritySvrSession::ResumeThreadL(const RMessage2& aMessage)
	{
	LOG_MSG( "CSecuritySvrSession::ResumeThreadL()\n" );

	//get thread ID
	TThreadId threadId = ReadTThreadIdL(aMessage, 0);

	//check attached
	CheckAttachedL(threadId, aMessage, EFalse);
	
	//security check passed so can perform actions
	TInt err = Server().iKernelDriver.ResumeThread(threadId);
	aMessage.Complete(err);
	}

void CSecuritySvrSession::GetDebugFunctionalityBufSizeL(const RMessage2& aMessage)
//
// Retrieve size of functionality data buffer in bytes which must be allocated
// by the client
//
	{
	LOG_MSG( "CSecuritySvrSession::GetDebugFunctionalityBufSizeL()\n" );

	TUint32 result = 0;
	// Get Buffer size from the kernel driver
	User::LeaveIfError(Server().iKernelDriver.GetDebugFunctionalityBufSize(result));

	TPtr8 stuff((TUint8*)&result,4, 4);

	aMessage.WriteL(0,stuff);

	aMessage.Complete(KErrNone);
	}

void CSecuritySvrSession::GetDebugFunctionalityL(const RMessage2& aMessage)
//
// Retrieve the functionality data and place it in a buffer
// allocated by the client.
//
	{
	LOG_MSG( "CSecuritySvrSession::GetDebugFunctionalityL()\n" );

	TUint32 dfsize = 0;

	// Get Buffer size from the kernel driver
	User::LeaveIfError(Server().iKernelDriver.GetDebugFunctionalityBufSize(dfsize));

	// Allocate space for the functionality data
	HBufC8* dftext = HBufC8::NewLC(dfsize);

	const TPtr8& dfPtr = dftext->Des();

	// Extract said data from the device driver
	User::LeaveIfError(Server().iKernelDriver.GetDebugFunctionality((TDes8&)dfPtr));

	// Return data to client
	aMessage.WriteL(0,dfPtr);

	// Free buffer
	CleanupStack::PopAndDestroy(dftext);

	aMessage.Complete(KErrNone);
	}

/**
Reads memory from a specified thread using the passed parameters. The user 
should ensure that the TPtr8 that is passed in has size greater than or equal
to the size of the memory that is trying to be read.

@param aMessage The RMessage2 object should be constructed as follows:
    * aMessage.Int0() is the thread ID of the target debug app
    * aMessage.Ptr1() is a TMemoryInfo object which contains the following:
        * the address of the memory to be read from the target debug thread
        * the size of the memory block to be read from the target debug thread
	* the access size to use
	* the endianess to interpret the data as
    * aMessage.Ptr2() is the address of the buffer in the debug agent thread 
      that the data from the target debug app should be written into 

@leave KErrPermissionDenied if client is not attached to the target
       thread's process,
       KErrNoMemory if memory could not be allocated,
       KErrArgument if there are problems with the aMessage object,
       KErrBadHandle if the thread represented by aMessage.Ptr0() is invalid,
       an error value from CSecuritySvrSession::ValidateMemoryInfo if checking
       the memory attributes failed,
       or another of the system wide error codes
*/
void CSecuritySvrSession::ReadMemoryL(const RMessage2& aMessage)
	{
	LOG_MSG( "CSecuritySvrSession::ReadMemoryL()\n" );

	//get debug app thread ID
	TThreadId threadId = ReadTThreadIdL(aMessage, 0);

	CheckAttachedL(threadId, aMessage, ETrue);

	//create and initialise the memory info object
	TMemoryInfo targetMemory;
	TPtr8 targetMemoryPtr( (TUint8 *)&targetMemory, sizeof(TMemoryInfo) );

	aMessage.ReadL(1,targetMemoryPtr);

	//check memory info is acceptable
	ValidateMemoryInfoL(threadId, targetMemory, ETrue);

	RBuf8 data;
	data.CreateL(targetMemory.iSize);
	data.CleanupClosePushL();

	//fill buffer with data from target debug thread
	User::LeaveIfError(Server().iKernelDriver.ReadMemory(threadId, targetMemory.iAddress, targetMemory.iSize, data));

	//attempt to write the data from the target debug thread back to the agent
	aMessage.WriteL(2, data);

	//delete temporary buffer
	CleanupStack::PopAndDestroy(&data);

	aMessage.Complete(KErrNone);
	}

/**
Writes memory to a specified thread using the passed parameters. 

@param aMessage The RMessage2 object should be constructed as follows:
    * aMessage.Ptr0() is the thread ID of the target debug app
    * aMessage.Ptr1() is a TMemoryInfo object which contains the following:
        * the address of the memory to be written to the target debug thread
        * the size of the memory block to be written to the target debug thread
	* the access size to use
	* the endianess to interpret the data as
    * aMessage.Ptr2() is the address of the buffer in the debug agent thread 
      that the data to write to the target debug app should be read from

@leave KErrPermissionDenied if client is not attached (actively) to the target
       thread's process,
       KErrNoMemory if memory could not be allocated,
       KErrArgument if there are problems with the aMessage object,
       KErrBadHandle if the thread represented by aMessage.Ptr0() is invalid,
       an error value from CSecuritySvrSession::ValidateMemoryInfo if checking
       the memory attributes failed,
       or another of the system wide error codes
*/
void CSecuritySvrSession::WriteMemoryL(const RMessage2& aMessage)
	{
	LOG_MSG( "CSecuritySvrSession::WriteMemoryL()\n" );

	//get debug app thread ID
	TThreadId threadId = ReadTThreadIdL(aMessage, 0);

	CheckAttachedL(threadId, aMessage, EFalse);

	//create and initialise the memory info object
	TMemoryInfo targetMemory;
	TPtr8 targetMemoryPtr( (TUint8 *)&targetMemory, sizeof(TMemoryInfo) );

	aMessage.ReadL(1,targetMemoryPtr);

	//check memory info is acceptable
	ValidateMemoryInfoL(threadId, targetMemory, EFalse);

	//create temporary buffer and read data from client
	RBuf8 data;
	data.CreateL(targetMemory.iSize);
	data.CleanupClosePushL();
	
	aMessage.ReadL(2, data);

	// what about telling the driver about endianess/access size?
	User::LeaveIfError(Server().iKernelDriver.WriteMemory(threadId, targetMemory.iAddress, targetMemory.iSize, data));

	//free temporary buffer
	CleanupStack::PopAndDestroy(&data);

	aMessage.Complete(KErrNone);
	}

/**
@internalTechnology

Notes: This call is used to set a thread specific breakpoint. Its input arguments
are the thread id, address and architecture type of the breakpoint. It returns success
or failure, and if successful, it sets the TBreakId in the Debug Agent to the 
breakpoint id by which it can be referenced in future calls to ModifyBreak,ClearBreak and
BreakInfo.

@param aMessage.Ptr0() - aThreadId is thread id of the target debug process
@param aMessage.Ptr1() - Address of a TBreakInfo in the Debug Agent
@param aMessage.Ptr2() - Address of a TBreakId in the Debug Agent
@leave KErrPermissionDenied if the security check fails.
 KErrAlreadyExists if there is a breakpoint overlapping the desired address.
 KErrNotSupported if the architecture type is unrecognised.
 KErrNoMemory if there is no more memory to complete the operation.
 KErrArgument if the breakpoint address alignment is unsuitable for the requested
 breakpoint.
 KErrOverflow if there are too many breakpoints set.
*/
void CSecuritySvrSession::SetBreakL(const RMessage2& aMessage)
	{
	LOG_MSG( "CSecuritySvrSession::SetBreakL!()\n" );

	//get debug app thread ID
	TThreadId threadId = ReadTThreadIdL(aMessage, 0);

	//check that the agent has attached to the target process
	CheckAttachedL(threadId, aMessage, EFalse);

	//create and initialise the memory info object
	TBreakInfo breakInfo;
	TPtr8 breakInfoPtr( (TUint8 *)&breakInfo, sizeof(TBreakInfo) );

	aMessage.ReadL(1,breakInfoPtr);

	//set break in target app
	TBreakId breakId = 0;
	User::LeaveIfError(Server().iKernelDriver.SetBreak(breakId, threadId, breakInfo.iAddress, breakInfo.iArchitectureMode));

	//attempt to write the break id back to the debug agent
	WriteDataL(aMessage, 2, &breakId, sizeof(breakId));

	aMessage.Complete(KErrNone);
	}

/**
Clears a breakpoint previously set by a SetBreak() call.

@param aMessage.Int0() - TBreakId of the breakpoint to be removed.
*/
void CSecuritySvrSession::ClearBreakL(const RMessage2& aMessage)
	{
	LOG_MSG( "CSecuritySvrSession::ClearBreakL()\n" );

	const TInt breakId = aMessage.Int0();

	// Check that the breakpoint exists
	TUint64 objectId;
	TUint32 address;
	TArchitectureMode mode;
	TBool threadSpecific = EFalse;

	User::LeaveIfError(Server().iKernelDriver.BreakInfo(breakId,objectId,address,mode,threadSpecific));

	if(threadSpecific)
		{
		// Check that the debug agent is attached to the thread for which the
		// breakpoint is currently set.
		CheckAttachedL(TThreadId(objectId), aMessage, EFalse);
		}
	else
		{
		// Check that the debug agent is attached to the process for which the
		// breakpoint is currently set.
		CheckAttachedL(TProcessId(objectId), aMessage, EFalse);
		}

	// Finally clear the breakpoint
	User::LeaveIfError(Server().iKernelDriver.ClearBreak(breakId));

	aMessage.Complete(KErrNone);
	}

/**
@param aMessage.Int0() - Breakpoint Id of interest
@param aMessage.Ptr1() - Address in Debug Agent to place threadId of the breakpoint
@param aMessage.Ptr2() - Address in Debug Agent to place address of the breakpoint
@param aMessage.Ptr3() - Address in Debug Agent to place the architecture mode of the breakpoint
@leave Any error which may be returned by RSessionBase::SendReceive()
*/
void CSecuritySvrSession::BreakInfoL(const RMessage2& aMessage)
	{
	const TBreakId breakId = (TBreakId)aMessage.Int0();

	TThreadId threadId;
	TUint32 address;
	TArchitectureMode mode;
	TBool threadSpecific = ETrue;

	TUint64 threadIdData;
	User::LeaveIfError(Server().iKernelDriver.BreakInfo(breakId,threadIdData,address,mode,threadSpecific));

	if(!threadSpecific)
		{
		User::Leave(KErrNotFound);
		}

	threadId = TThreadId(threadIdData);

	//check that the agent has attached to the target process
	CheckAttachedL(threadId, aMessage, EFalse);

	// return the threadId
	WriteDataL(aMessage, 1, &threadId, sizeof(threadId));

	// return the address
	WriteDataL(aMessage, 2, &address, sizeof(address));

	// return the mode
	WriteDataL(aMessage, 3, &mode, sizeof(mode));

	aMessage.Complete(KErrNone);
	}

/**
@internalTechnology

Modify a previously set breakpoint.

@param aMessage.Int0() - The breakpoint id of the breakpoint to modify
@param aMessage.Ptr1() - The new Thread Id for the breakpoint
@param aMessage.Int2() - The new virtual memory address for the breakpoint
@param aMessage.Int3() - The new architecture mode for the breakpoint
@return KErrNone if succesful. KErrPermissionDenied if the security check fails.
 KErrAlreadyExists if there is a breakpoint overlapping the desired address.
 KErrNotSupported if the architecture type is unrecognised.
 KErrNoMemory if there is no more memory to complete the operation.
 KErrArgument if the breakpoint address alignment is unsuitable for the requested
 breakpoint.
 KErrOverflow if there are too many breakpoints set.
*/
void CSecuritySvrSession::ModifyBreakL(const RMessage2& aMessage)
	{
	const TBreakId breakId = (TBreakId)aMessage.Int0();
	const TThreadId threadId = ReadTThreadIdL(aMessage, 1);
	const TUint32 address = aMessage.Int2();
	const TArchitectureMode mode = (TArchitectureMode)aMessage.Int3();

	// Get information on the breakpoint to check the security status
	TUint64 checkThreadId;
	TUint32 checkAddress;
	TArchitectureMode checkMode;
	TBool threadSpecific;

	User::LeaveIfError(Server().iKernelDriver.BreakInfo(breakId,checkThreadId,checkAddress,checkMode,threadSpecific));

	// Security check that the thread Id is associated with the debug agent

	//check that the agent has attached to the target process
	CheckAttachedL(TThreadId(checkThreadId), aMessage, EFalse);

	// now check that the thread Id which is being set is permitted
	//check that the agent has attached to the target process
	CheckAttachedL(threadId, aMessage, EFalse);

	User::LeaveIfError(Server().iKernelDriver.ModifyBreak(breakId,threadId,address,mode));

	aMessage.Complete(KErrNone);
	}

/**
@internalTechnology

Notes: This call is used to set a process wide breakpoint. Its input arguments
are the process id, address and architecture type of the breakpoint. It returns success
or failure, and if successful, it sets the TBreakId in the Debug Agent to the 
breakpoint id by which it can be referenced in future calls to ModifyBreak,ClearBreak and
BreakInfo.

@param aMessage.Ptr0() - aProcessId is process id of the target debug process
@param aMessage.Ptr1() - Address of a TBreakInfo in the Debug Agent
@param aMessage.Ptr2() - Address of a TBreakId in the Debug Agent
@leave KErrPermissionDenied if the security check fails.
 KErrAlreadyExists if there is a breakpoint overlapping the desired address.
 KErrNotSupported if the architecture type is unrecognised.
 KErrNoMemory if there is no more memory to complete the operation.
 KErrArgument if the breakpoint address alignment is unsuitable for the requested
 breakpoint.
 KErrOverflow if there are too many breakpoints set.
*/
void CSecuritySvrSession::SetProcessBreakL(const RMessage2& aMessage)
	{
	LOG_MSG( "CSecuritySvrSession::SetProcessBreakL()\n" );

	//get debug app thread ID
	TProcessId procId = ReadTProcessIdL(aMessage, 0);

	//check that the agent has attached to the target process
	CheckAttachedL(procId, aMessage, EFalse);

	//create and initialise the memory info object
	TBreakInfo breakInfo;
	TPtr8 breakInfoPtr( (TUint8 *)&breakInfo, sizeof(TBreakInfo) );

	aMessage.ReadL(1,breakInfoPtr);

	//set break in target app
	TBreakId breakId = 0;
	User::LeaveIfError(Server().iKernelDriver.SetProcessBreak(breakId, procId, breakInfo.iAddress, breakInfo.iArchitectureMode));

	//attempt to write the break id back to the debug agent
	WriteDataL(aMessage, 2, &breakId, sizeof(breakId));

	aMessage.Complete(KErrNone);
	}

/**
@internalTechnology

Modify a previously set process breakpoint.

@param aMessage.Int0() - The breakpoint id of the breakpoint to modify
@param aMessage.Ptr1() - The new Process Id for the breakpoint
@param aMessage.Int2() - The new virtual memory address for the breakpoint
@param aMessage.Int3() - The new architecture mode for the breakpoint
@return KErrNone if succesful. KErrPermissionDenied if the security check fails.
 KErrAlreadyExists if there is a breakpoint overlapping the desired address.
 KErrNotSupported if the architecture type is unrecognised.
 KErrNoMemory if there is no more memory to complete the operation.
 KErrArgument if the breakpoint address alignment is unsuitable for the requested
 breakpoint.
 KErrOverflow if there are too many breakpoints set.
*/
void CSecuritySvrSession::ModifyProcessBreakL(const RMessage2& aMessage)
	{
	const TBreakId breakId = (TBreakId)aMessage.Int0();
	const TProcessId processId = ReadTProcessIdL(aMessage, 1);
	const TUint32 address = aMessage.Int2();
	const TArchitectureMode mode = (TArchitectureMode)aMessage.Int3();

	// Get information on the breakpoint to check the security status
	TUint64 checkProcessId;
	TUint32 checkAddress;
	TArchitectureMode checkMode;
	TBool threadSpecific;

	User::LeaveIfError(Server().iKernelDriver.BreakInfo(breakId,checkProcessId,checkAddress,checkMode,threadSpecific));

	// Security check that the thread Id is associated with the debug agent

	//check that the agent has attached to the target process
	CheckAttachedL(TProcessId(checkProcessId), aMessage, EFalse);

	// now check that the thread Id which is being set is permitted
	//check that the agent has attached to the target process
	CheckAttachedL(processId, aMessage, EFalse);

	User::LeaveIfError(Server().iKernelDriver.ModifyProcessBreak(breakId,processId,address,mode));

	aMessage.Complete(KErrNone);
	}

/**
@param aMessage.Int0() - Breakpoint Id of interest
@param aMessage.Ptr1() - Address in Debug Agent to place process Id of the breakpoint
@param aMessage.Ptr2() - Address in Debug Agent to place address of the breakpoint
@param aMessage.Ptr3() - Address in Debug Agent to place the architecture mode of the breakpoint
@leave Any error which may be returned by RSessionBase::SendReceive()
*/
void CSecuritySvrSession::ProcessBreakInfoL(const RMessage2& aMessage)
	{
	const TBreakId breakId = (TBreakId)aMessage.Int0();

	TProcessId procId;
	TUint32 address;
	TArchitectureMode mode;
	TBool threadSpecific;

	TUint64 procIdData;
	User::LeaveIfError(Server().iKernelDriver.BreakInfo(breakId,procIdData,address,mode,threadSpecific));
	if(threadSpecific)
		{
		User::Leave(KErrNotFound);
		}
	procId = TProcessId(procIdData);

	//check that the agent has attached to the target process
	CheckAttachedL(procId, aMessage, EFalse);

	// return the processId
	WriteDataL(aMessage, 1, &procId, sizeof(procId));

	// return the address
	WriteDataL(aMessage, 2, &address, sizeof(address));

	// return the mode
	WriteDataL(aMessage, 3, &mode, sizeof(mode));

	aMessage.Complete(KErrNone);
	}

/**
Read register values.

@param aMessage should contain:
        * at offset 0 a pointer to the thread ID of the target thread
        * at offset 1 a descriptor representing an array of TRegisterInfo 
          register IDs
        * at offset 2 a descriptor representing an array into which TRegisterValue 
          register values will be written
        * at offset 3 a descriptor representing an array into which TUint8 
          register flags will be written

@leave KErrArgument if the max length of the array at offset 1 is not a 
       multiple of sizeof(TRegisterInfo), if the max length of the array 
       at offset 2 is not a multiple of sizeof(TRegisterValue), if the max 
       length of the array at offset 3 is not a multiple of sizeof(TUint8), if
       any of the descriptors have max length of 0, or if the three 
       descriptors do not represent the same number of registers,
       KErrNoMemory if there is insufficient memory,
       KErrDied, if the thread with thread ID aThreadId is dead
*/
void CSecuritySvrSession::ReadRegistersL(const RMessage2& aMessage)
	{
	LOG_MSG( "CSecuritySvrSession::ReadRegistersL()\n" );

	const TThreadId threadId = ReadTThreadIdL(aMessage, 0);

	//check the agent is attached to the thread
	CheckAttachedL(threadId, aMessage, ETrue);

	//number of registers being requested
	TUint32 numberOfRegisters;

	//check length of descriptors is acceptable
	ValidateRegisterBuffersL(aMessage, numberOfRegisters);

	// Passed data will be saved in this descriptor.
	RBuf8 ids;
	ids.CreateL(numberOfRegisters * sizeof(TRegisterInfo));
	// Do the right cleanup if anything subsequently goes wrong
	ids.CleanupClosePushL();
	
	//read the data from the client thread
	aMessage.ReadL(1, ids);

	//create buffer to fill with data from target debug thread
	HBufC8 *data = HBufC8::NewLC(aMessage.GetDesMaxLength(2));
	TPtr8 values(data->Des());   
	
	HBufC8 *flagsData = HBufC8::NewLC(numberOfRegisters * sizeof(TUint8));
	TPtr8 flags(flagsData->Des());   
	
	//get register info and return relevant parts back to agent
	User::LeaveIfError(Server().iKernelDriver.ReadRegisters(threadId, ids, values, flags));
	aMessage.WriteL(2, values);
	aMessage.WriteL(3, flags);
	
	//delete temporary buffers and return status
	CleanupStack::PopAndDestroy(flagsData);
	CleanupStack::PopAndDestroy(data);
	CleanupStack::PopAndDestroy(&ids);

	aMessage.Complete(KErrNone);
	}

/**
Write register values.

@param aMessage should contain:
        * at offset 0 a pointer to the thread ID of the target thread
        * at offset 1 a descriptor representing an array of TRegisterInfo 
          register IDs
        * at offset 2 a descriptor representing an array of TRegisterValue register 
          values
        * at offset 3 a descriptor representing an array into which TUint8 
          register flags will be written

@leave KErrArgument if the max length of the array at offset 1 is not a 
       multiple of sizeof(TRegisterInfo), if the max length of the array 
       at offset 2 is not a multiple of sizeof(TRegisterValue), if the max 
       length of the array at offset 3 is not a multiple of sizeof(TUint8), if
       any of the descriptors have max length of 0, or if the three 
       descriptors do not represent the same number of registers,
       KErrNoMemory if there is insufficient memory,
       KErrDied, if the thread with thread ID aThreadId is dead
*/
void CSecuritySvrSession::WriteRegistersL(const RMessage2& aMessage)
	{
	LOG_MSG( "CSecuritySvrSession::WriteRegistersL()\n" );

	const TThreadId threadId = ReadTThreadIdL(aMessage, 0);

	CheckAttachedL(threadId, aMessage, EFalse);

	//number of registers attempting to set
	TUint32 numberOfRegisters;

	//check length of descriptors is acceptable
	ValidateRegisterBuffersL(aMessage, numberOfRegisters);

	// Passed register ids will be saved in this descriptor.
	RBuf8 ids;

	//allocate buffer
	ids.CreateL(numberOfRegisters * sizeof(TRegisterInfo));

	// Do the right cleanup if anything subsequently goes wrong
	ids.CleanupClosePushL();

	//read the data from the client thread
	aMessage.ReadL(1, ids);

	// Passed register values will be saved in this descriptor.
	RBuf8 values;

	//allocate buffer
	values.CreateL(aMessage.GetDesMaxLength(2));
	// Do the right cleanup if anything subsequently goes wrong
	values.CleanupClosePushL();
	//read the data from the client thread
	aMessage.ReadL(2,values);

	HBufC8 *flagsData = HBufC8::NewLC(numberOfRegisters*sizeof(TUint8));
	TPtr8 flags(flagsData->Des());

	//get register info and return relevant parts back to agent
	User::LeaveIfError(Server().iKernelDriver.WriteRegisters(threadId, ids, values, flags));

	//write flags data back
	aMessage.WriteL(3, flags);

	CleanupStack::PopAndDestroy(flagsData);
	CleanupStack::PopAndDestroy(&values);
	CleanupStack::PopAndDestroy(&ids);

	aMessage.Complete(KErrNone);
	}

/**
Processes an attach request from a debug agent. Gets the target debug
processes' original FileName as an argument. The method sets completion
status of the aMessage argument to KErrNone if successfully attached and to
another of the system wide error codes if there were problems.

@param aMessage contains:
       * a boolean at offset 0 which indicates whether the agent wishes to
       attach passively
       * a buffer at offset 1 which contains the FileName
       of the target debug process.
*/
void CSecuritySvrSession::AttachProcessL(const RMessage2& aMessage)
	{
	LOG_MSG( "CSecuritySvrSession::AttachProcessL()\n" );

	TBool aPassive = aMessage.Int0() ? ETrue : EFalse;

	TInt deslen = aMessage.GetDesLengthL(1);

	// Passed data will be saved in this descriptor.
	RBuf processName;

	// Max length set to the value of "deslen", but current length is zero
	processName.CreateL(deslen);

	// Do the right cleanup if anything subsequently goes wrong
	processName.CleanupClosePushL();

	// Copy the client's descriptor data into our buffer.
	aMessage.ReadL(1,processName);

	//
	// Security Check
	//
	// It is not permitted to debug the debug security server!
	//
	// get the secure id of the executable
	TUid secureId(TUid::Null());
	GetSecureIdL(processName, secureId);
	if (KUidDebugSecurityServer.iUid == secureId.iUid)
		{
		// The debug agent has requested to debug the Debug Security Server
		// This is either an error, or an attempt to breach security. We
		// therefore refuse to agree to this request, and return KErrPermissionDenied
		LOG_MSG("CSecuritySvrSession::AttachProcessL() - Debug Agent attempted to debug the Debug Security Server\n");

		User::Leave(KErrPermissionDenied);
		}

	//get the debug agent's thread and push handle onto clean up stack
	RThread clientThread;
	User::LeaveIfError(aMessage.Client(clientThread));
	CleanupClosePushL(clientThread);

	//get the debug agent's process
	RProcess clientProcess;
	User::LeaveIfError(clientThread.Process(clientProcess));

	//finished with thread so close handle and destroy
	CleanupStack::PopAndDestroy(&clientThread);
	
	//get the debug agent's process id
	TProcessId processId = clientProcess.Id();

	//store the debug agent's process id for forced detaching later if the
	//agent doesn't tidy up after itself
	StoreDebugAgentId(processId);

	// Read the OEM Debug token capabilities (if any)
	GetDebugAgentOEMTokenCapsL();

	//finished with process so close handle
	clientProcess.Close();
	
	// Get the Security info via rlibrary::getinfo
	RLibrary::TInfo info;
	TPckg<RLibrary::TInfo> infoBuf(info);

	TInt err = RLibrary::GetInfo(processName, infoBuf);
	if (err != KErrNone)
		{
		LOG_MSG("CSecuritySvrSession::AttachProcessL() - Cannot parse the target executable header\n");
		
		// Could not read the header for this executable :-(

		CleanupStack::PopAndDestroy(&processName);
		
		aMessage.Complete(KErrPermissionDenied);
 
		return;
		}
	
	// Special case for AllFiles - OEM Debug tokens MUST have
	// AllFiles, as this is what allows them to read contents
	// of other executables.
	TBool checkDebuggable = ETrue;

	// Does an OEM Debug Token permit debug where it would normally not be
	// permitted?
	if ( Server().OEMTokenPermitsDebugL(iOEMDebugCapabilities, info.iSecurityInfo.iCaps) )
		{
		// OEM Debug token is valid and has sufficient capabilities
		LOG_MSG("CSecuritySvrSession::AttachProcessL() - Debug Agent has sufficient capabilites based on OEM Debug Token");	
		
		checkDebuggable = EFalse;
		}

	if (checkDebuggable)
		{
		// OEM Debug token (if any), does not confer sufficient capabilities to
		// debug the specified target executable. Therefore debugging can only
		// be permitted if the target executable itself has been built as 'Debuggable'
		LOG_MSG("CSecuritySvrSession::AttachProcessL() - Debug Agent has insufficient capabilites based on OEM Debug Token");	

		IsDebuggableL(processName);
		}

	User::LeaveIfError(Server().AttachProcessL(processName, processId, aPassive));

	// Inform the kernel driver about the attachment, so that it
	// can track per-agent data about the process.
	RBuf8 processName8;

	processName8.CreateL(deslen);

	processName8.CleanupClosePushL();

	processName8.Copy(processName);

	User::LeaveIfError(Server().iKernelDriver.AttachProcess(processName8, processId.Id()));

	// Create an Active Object to handle asynchronous calls to GetEvent
	CSecuritySvrAsync* handler = CSecuritySvrAsync::NewL(this,processName8,processId);

	err = iAsyncHandlers.Insert(handler,0);
	if (err != KErrNone)
		{
		// If we don't have an asynchronous handler, we should detach
		// the driver as well.
		if( (KErrNone != Server().iKernelDriver.DetachProcess(processName8,processId.Id()))
			|| (KErrNone != Server().DetachProcess(processName, processId)) )
			{
			// this is a serious error, neither of these calls should fail so
			// we panic the server after closing the driver and printing a message

			LOG_MSG("CSecuritySvrSession::AttachProcessL(): critical error during cleanup\n");
			Server().iKernelDriver.Close();
			RProcess process;
			process.Panic(_L("AttachProcessL() failed"), KErrServerTerminated);
			}
		}

	User::LeaveIfError(err);

	CleanupStack::PopAndDestroy(&processName8);

	CleanupStack::PopAndDestroy(&processName);

	aMessage.Complete(KErrNone);
	}

/**
Reads the OEM Debug Token associated with the debug agent if any. The OEM Debug Token
allows the Debug Agent to debug certain executables which have not been built as
'Debuggable'.

This works as follows: The OEM Debug Token is an executable with a special name
of the form "OEMDebug_<DA_SID>.exe" where <DA_SID> is the Secure ID of the Debug Agent
in hexadecimal. For example: "OEMDebug_F123ABCD.exe" would be a valid name. This token executable
must be signed with 'AllFiles' + X, where X is the set of PlatSec capabilities that are
possessed by the target executable to be debugged.

This function reads the capabilities possessed by the token by creating a process based
on the executable, and reading the TSecurityInfo associated with the process. This ensures
that the loader has validated the token has not been tampered with and that the security
information is valid.

The security information is then stored for future use as member data in iOEMDebugCapabilities.

Leaves if there is an error, otherwise simply fills in the capabilities
in iOEMDebugCapabilities.

It is not an error for the OEM Debug token not to exist. In this case, the function simply returns.
*/
void CSecuritySvrSession::GetDebugAgentOEMTokenCapsL(void)
	{
	// Sanity check
	if (!iDebugAgentProcessIdStored)
		{
		LOG_MSG("CSecuritySvrSession::GetDebugAgentOEMTokenCapsL() - Debug Agent Process Id not stored");
		
		// We have not stored the debug agent process id!
		User::Leave(KErrNotReady);
		}

	// Obtain the security info about the debug agent process
	//get the debug agent's process
	RProcess debugAgentProcess;

	CleanupClosePushL(debugAgentProcess);

	debugAgentProcess.Open(iDebugAgentProcessId);

	// We have now obtained a process handle based on the token executable, so we can check its security properties.
	TSecurityInfo secInfo(debugAgentProcess);

// Compute the name of the OEM debug token based on the SID of the debug agent
_LIT(KDSSOEMDebugTokenPrefix,"OEMDebug_");
_LIT(KDSSOEMDebugTokenAppendFmt,"%08X.exe");

	RBuf agentTokenName;
	agentTokenName.CreateL(KDSSOEMDebugTokenPrefix().Size()+8+1);	// allow space for SID+null terminator
	agentTokenName.CleanupClosePushL();

	agentTokenName.SetLength(0);

	// Add OEMDebug_
	agentTokenName.Append(KDSSOEMDebugTokenPrefix());

	// Add debug agent Secure ID
	agentTokenName.AppendFormat(KDSSOEMDebugTokenAppendFmt,secInfo.iSecureId.iId);
	
	// just log the token name for the moment.
	RBuf8 agentTokenName8;

	agentTokenName8.CreateL(agentTokenName.Length()+1);

	agentTokenName8.CleanupClosePushL();

	agentTokenName8.Copy(agentTokenName);

	agentTokenName8.Append(TChar(0));

	//LOG_MSG2("CSecuritySvrSession::GetDebugAgentOEMTokenCapsL() - OEM Debug Token Name is %s",agentTokenName8.Ptr()); 

	// Cleanup
	CleanupStack::PopAndDestroy(&agentTokenName8);

	// Now locate and start the executable...
	RProcess agentToken;
	TInt err = agentToken.Create(agentTokenName, KNullDesC);
	if (KErrNone != err)
		{
		// Failed to create a process based on the token, just give up
		LOG_MSG2("CSecuritySvrSession::GetDebugAgentOEMTokenCapsL() - Could not create process based on token due to err 0x%8x\n",err);
		
		// Cleanup remaining items from the stack
		CleanupStack::PopAndDestroy(&agentTokenName);

		CleanupStack::PopAndDestroy(&debugAgentProcess);
		return;
		}

	// Synchronise with the process to make sure it hasn't died straight away
	TRequestStatus stat;
	agentToken.Rendezvous(stat);
	if (stat != KRequestPending)
		{
		// logon failed - agentToken is not yet running, so cannot have terminated
		agentToken.Kill(0);             // Abort startup
		}

	// store the OEM Debug Token security data 
	TSecurityInfo agentSecInfo(agentToken);

	// Note capabilities for future use
	iOEMDebugCapabilities=agentSecInfo.iCaps;
	
	// resume the token. It _should_ just exit, but we don't really care.
	agentToken.Resume();

	// Wait to synchronise with agentToken - if it dies in the meantime, it
	// also gets completed
	User::WaitForRequest(stat);

	// Just close the handle to it again.
	agentToken.Close();

	// Cleanup remaining items from the stack
	CleanupStack::PopAndDestroy(&agentTokenName);

	CleanupStack::PopAndDestroy(&debugAgentProcess);

	}

/**
  Checks whether the file passed in as aExecutable is XIP or not

  @param aExecutable file to check
  @return ETrue if the file is XIP, EFalse otherwise
  */
TBool CSecuritySvrSession::IsExecutableXipL(RFile& aExecutable)
	{
	TUint atts;
	User::LeaveIfError(aExecutable.Att(atts));

	return atts & KEntryAttXIP;
	}
/**
  Gets access to the symbian crash partition for crash access operation.
  */
void CSecuritySvrSession::ConnectCrashPartitionL (void)
	{
	LOG_MSG("CSecuritySvrSession::ConnectCrashPartitionL()");
	
	TBool changed;
	TInt error = KErrNone;
	TInt i=0;
	
	//Intialising to EFalse
	iCrashConnected = EFalse;
	
	TPckg<TLocalDriveCapsV2> capsBuf(iCaps);
	
	//check for the symbian crash partition
	for (i=0; i<KMaxLocalDrives; i++)
		{
		error = iLocalDrive.Connect (i, changed);
		if ( error == KErrNone)
			{
			error = iLocalDrive.Caps(capsBuf);
			if ( error != KErrNone)
				{
				//continue if not found
				continue;
				}
			if ( iCaps.iPartitionType == (TUint16)KPartitionTypeSymbianCrashLog)
				{				
				LOG_MSG2("Found Symbian crash log partition on drive: %d",i);
				iCrashConnected = ETrue;
				break;
				}			
			}
		}
	if ( i == KMaxLocalDrives)
		{
			LOG_MSG("No crash log partition found with valid crash log signature found.  Exiting...");
			User::Leave (KErrNotFound);
		}
	
	}
/** Checks that aHeaderData contains enough data to cast it to the
  appropriate header type.

  @param aHeaderData buffer containing header data read from a file
  @param aXip boolean indicating whether the header data is for an XIP image

  @return ETrue if enough data in buffer, EFalse otherwise
  */
TBool CSecuritySvrSession::CheckSufficientData(const TDesC8& aHeaderData, const TBool aXip) const
	{
	TUint minimumHeaderSize = aXip ? sizeof(TRomImageHeader) : sizeof(E32ImageHeaderV);
	return (aHeaderData.Length() >= minimumHeaderSize);
	}

/**
  Opens a file handle to aFileName using aFileHandle
  @param aFileName file to open handle to
  @param aFs file system to use to open the handle
  @param aFileHandle file handle to open

  @leave one of the system wide error codes
  */
void CSecuritySvrSession::OpenFileHandleL(const TDesC& aFileName, RFs& aFs, RFile& aFileHandle)
	{
	TInt err = aFileHandle.Open(aFs, aFileName, EFileRead | EFileShareReadersOnly);
	if (err != KErrNone)
		{
		// Could not open the file for reading
		LOG_MSG("CSecuritySvrSession::OpenFileHandleL - Failed to open executable\n");

		User::Leave(err);
		}
	}

/**
  Checks whether an executable has the debug bit set

  @param aHeaderData buffer containing the header of the executable
  @param aXip indication of whether the executable is XIP or not

  @return ETrue if debug bit is set, EFalse otherwise
  */
TBool CSecuritySvrSession::IsDebugBitSet(const TDesC8& aHeaderData, const TBool aXip)
	{
	if(!CheckSufficientData(aHeaderData, aXip))
		{
		return EFalse;
		}

	if (aXip)
		{
		TRomImageHeader* hdr = (TRomImageHeader*)aHeaderData.Ptr();
		return (hdr->iFlags & KRomImageDebuggable);
		}
	else
		{
		// it is an epoc32 image
		E32ImageHeaderV* hdr = (E32ImageHeaderV*)aHeaderData.Ptr();
		return (hdr->iFlags & KImageDebuggable);
		}
	}

/**
Determines whether a particular executable is marked as 'debuggable'

Notes:
This function is currently hard coded to understand the format of e32 and
TRomImage file headers. Ideally this will be replaced by a call to RLibrary::GetInfo
which can return the 'debuggable' information. Unfortunately, this call currently
does not provide the information for XIP executables :-(

@leave KErrPermissionDenied if the debug bit is not set, or one of the other
system wide error codes
*/
void CSecuritySvrSession::IsDebuggableL(const TDesC& aFileName)
	{
#ifndef IGNORE_DEBUGGABLE_BIT

	RFs fs;
	User::LeaveIfError(fs.Connect());
	CleanupClosePushL(fs);

	RFile targetExe;
	OpenFileHandleL(aFileName, fs, targetExe);
	CleanupClosePushL(targetExe);

	// Read in the entire header
	RBuf8 e32HdrBuf;
	e32HdrBuf.CreateL(RLibrary::KRequiredImageHeaderSize);
	e32HdrBuf.CleanupClosePushL();

	// Read the entire header as far as possible
	TInt err = targetExe.Read(e32HdrBuf);
	if (err != KErrNone)
		{
		// Could not read the file 
		LOG_MSG("CSecuritySvrSession::IsDebuggableL - Failed to read executable\n");

		User::Leave(err);
		}

	if(!CheckSufficientData(e32HdrBuf, IsExecutableXipL(targetExe)))
		{
		User::Leave(KErrGeneral);
		}

	if(! IsDebugBitSet(e32HdrBuf, IsExecutableXipL(targetExe)))
		{
		User::Leave(KErrPermissionDenied);
		}
	CleanupStack::PopAndDestroy(3, &fs);

#else
	LOG_MSG("CSecuritySvrSession::IsDebuggableL() Debuggable bit temporarily ignored!!!");
#endif
	}

/**
Processes a detach request from a debug agent. Gets the target debug
processes' original FileName as an argument. The method sets completion
status of the aMessage argument to KErrNone if successfully detached and to
another of the system wide error codes if there were problems.

@param aMessage contains:
       * a buffer at offset 0 which contains the FileName
       of the target debug process.
*/
void CSecuritySvrSession::DetachProcessL(const RMessage2& aMessage)
	{
	LOG_MSG( "CSecuritySvrSession::DetachProcessL()\n" );

	TInt deslen = aMessage.GetDesLengthL(0);
	// Passed data will be saved in this descriptor.
	RBuf processName;

	// Max length set to the value of "deslen", but current length is zero
	processName.CreateL(deslen);

	// Do the right cleanup if anything subsequently goes wrong
	processName.CleanupClosePushL();

	// Copy the client's descriptor data into our buffer.
	aMessage.ReadL(0,processName);

	User::LeaveIfError(Server().DetachProcess(processName, iDebugAgentProcessId));

	// Inform the kernel driver about the detachment, so that
	// it can stop tracking per-agent data for the debugged process.
	RBuf8 processName8;

	processName8.CreateL(deslen);

	processName8.CleanupClosePushL();

	processName8.Copy(processName);

	// Remove the Asynchronous Object associated with this process
	for(TInt i=0; i<iAsyncHandlers.Count(); i++)
		{
		if (processName8.Compare(iAsyncHandlers[i]->ProcessName()) == 0)
			{
			delete iAsyncHandlers[i];
			iAsyncHandlers.Remove(i);

			break;
			}
		}
	
	// Inform the driver that we are no longer attached to this process
	User::LeaveIfError(Server().iKernelDriver.DetachProcess(processName8,iDebugAgentProcessId.Id()));
	
	CleanupStack::PopAndDestroy(&processName8);
	CleanupStack::PopAndDestroy(&processName);

	aMessage.Complete(KErrNone);
	}

/**
@param aMessage The RMessage2 object is expected to contain:
  * aMessage.Int0() - TDes8 Containing the process name.
  * aMessage.Int1() - Address of TPtr8 containing TEventInfo

*/
void CSecuritySvrSession::GetEventL(const RMessage2& aMessage)
	{
	LOG_MSG( "CSecuritySvrSession::GetEventL()\n" );

	// Local descriptor to contain target process name
	TInt deslen = aMessage.GetDesLengthL(0);

	RBuf processName;

	processName.CreateL(deslen);
	
	processName.CleanupClosePushL();

	// Read the target process name into processName
	aMessage.ReadL(0,processName);

	// Check if debug agent is attached to process
	if(!Server().CheckAttachedProcess(processName, aMessage, EFalse))
		{
		LOG_MSG("CSecuritySvrSession::GetEventL() - Not attached to this process\n");

		// Debug Agent is not attached at all to the requested process
		User::Leave(KErrPermissionDenied);
		}

	// Identify which process is being debugged, so that
	// we can locate the appropriate active object handler.
	RBuf8 processName8;

	processName8.CreateL(processName.Length());

	processName8.CleanupClosePushL();

	processName8.Copy(processName);

	// Find the Asynchronous Object associated with this process,
	// as it is permissible to have an outstanding GetEvent call
	// for each attached process.
	TBool foundHandler = EFalse;
	for(TInt i=0; i<iAsyncHandlers.Count(); i++)
		{
		if (processName8.Compare(iAsyncHandlers[i]->ProcessName()) == 0)
			{
			iAsyncHandlers[i]->GetEvent(aMessage);
			foundHandler = ETrue;
			break;
			}
		}

	if (foundHandler == EFalse)
		{
		// could not find an async handler object. Report the problem.
		LOG_MSG("CSecuritySvrSessionL - Could not find a handler object\n");
		User::Leave(KErrNotFound);
		}

	// Actually make the driver call, passing in the agent Id
	// so that the driver knows which per-agent event queue
	// to interrogate to retrieve the latest event.
	CleanupStack::PopAndDestroy(&processName8);
	CleanupStack::PopAndDestroy(&processName);
	}

/**
Cancels a pre-issued GetEvent call for a specific debugged process.

@param aMessage.Int0() - TDes8 containing aProcessName
*/
void CSecuritySvrSession::CancelGetEventL(const RMessage2& aMessage)
	{
	LOG_MSG( "CSecuritySvrSession::CancelGetEventL()\n" );

	// Local descriptor to contain target process name
	TInt deslen = aMessage.GetDesLengthL(0);

	RBuf processName;

	processName.CreateL(deslen);

	processName.CleanupClosePushL();

	// Read the target process name into processName
	aMessage.ReadL(0,processName,0);

	// Debug Agent is not an active debugger. Check if the DA is passively attached
	if(!Server().CheckAttachedProcess(processName, aMessage, EFalse))
		{
		// Debug Agent is not attached at all to the requested process
		User::Leave(KErrPermissionDenied);
		}

	// Identify the appropriate active object associate
	// with this process.
	RBuf8 processName8;

	processName8.CreateL(processName.Length());

	processName8.CleanupClosePushL();

	processName8.Copy(processName);

	// Find the Asynchronous Object associated with this process
	TBool foundHandler = EFalse;
	for(TInt i=0; i<iAsyncHandlers.Count(); i++)
		{
		if (processName8.Compare(iAsyncHandlers[i]->ProcessName()) == 0)
			{

			// Found the AO handler, so cancel the outstanding getevent call.
			iAsyncHandlers[i]->Cancel();
			foundHandler = ETrue;
			break;
			}
		}

	if(!foundHandler)
		{
		// We could not found a handler, so report the problem to the debug agent
		User::Leave(KErrNotFound);
		}

	//do cleanup
	CleanupStack::PopAndDestroy(&processName8);
	CleanupStack::PopAndDestroy(&processName);

	aMessage.Complete(KErrNone);
	}

/*
 Purpose: Sets the required event action to be taken for a specific
 process and event combination

@param aMessage The RMessage2 object is expected to contain:
  * aMessage.Int0() - TDes8 Containing the process name.
  * aMessage.Int1() - TEventType
  * aMessage.Int2() - TKernelEventAction
  *
*/
void CSecuritySvrSession::SetEventActionL(const RMessage2& aMessage)
	{
	LOG_MSG( "CSecuritySvrSession::SetEventActionL()\n" );

	// Local descriptor to contain target process name
	TInt deslen = aMessage.GetDesLengthL(0);

	RBuf processName;

	processName.CreateL(deslen);

	processName.CleanupClosePushL();

	// Read the target process name into processName
	aMessage.ReadL(0,processName);

	//check that the agent has attached to the target process
	if(!Server().CheckAttachedProcess(processName, aMessage, EFalse))
		{
		// Debug Agent is not attached at all to the requested process
		User::Leave(KErrPermissionDenied);
		}

	// Extract and validate the arguments from aMessage
	TUint32 event  = aMessage.Int1();
	if (event >= EEventsLast)
		{
		// Supplied event Id was not recognised
		User::Leave(KErrArgument);
		}

	TUint32 action = aMessage.Int2();
	if(action >= EActionLast)
	{
		// Supplied event action was not recognised
		User::Leave(KErrArgument);
	}

	RBuf8 processName8;

	processName8.CreateL(processName.Length());

	processName8.CleanupClosePushL();

	processName8.Copy(processName);

	// Make the call to the device driver
	TInt err = Server().iKernelDriver.SetEventAction(processName8, \
		(TEventType)event,\
		(TKernelEventAction)action,\
		iDebugAgentProcessId.Id());

	User::LeaveIfError(err);

	CleanupStack::PopAndDestroy(&processName8);
	CleanupStack::PopAndDestroy(&processName);

	aMessage.Complete(KErrNone);
}

/**
Purpose: Single-step a thread for a specified number of instructions

@param aMessage.Ptr0() - Thread Id of the thread to be stepped
@param aMessage.Int1() - Number of instructions to step.

@leave one of the system wide error codes

*/
void CSecuritySvrSession::StepL(const RMessage2& aMessage)
	{
	LOG_MSG( "CSecuritySvrSession::StepL()\n" );

	const TThreadId threadId = ReadTThreadIdL(aMessage, 0);
	const TInt32 numSteps = aMessage.Int1();

	CheckAttachedL(threadId, aMessage, EFalse);

	User::LeaveIfError(Server().iKernelDriver.Step( threadId, numSteps ));

	aMessage.Complete(KErrNone);
	}

/**
 * This checks whether or not the agent is permitted access to the flash partition
 * @return KErrNone if allowed, otherwise one of the system wide error codes
 * @leave one of the system wide error codes
 */
TInt CSecuritySvrSession::CheckFlashAccessPermissionL(const RThread aClientThread)
	{
	//get the debug agent's process
	RProcess clientProcess;
	User::LeaveIfError(aClientThread.Process(clientProcess));
	
	//get the debug agent's process id
	TProcessId processId = clientProcess.Id();

	//store the debug agent's process id for forced detaching later if the
	//agent doesn't tidy up after itself
	StoreDebugAgentId(processId);
	
	// Read the OEM Debug token capabilities (if any)
	GetDebugAgentOEMTokenCapsL();
	
	if(Server().OEMTokenPermitsFlashAccessL((iOEMDebugCapabilities)))
		{
		return KErrNone;
		}

	return KErrPermissionDenied;
	}

/**
Purpose: Read the crash log from the crash flash partition
@param aMessage.Int0() - Position to read from.
@param aMessage.Ptr1() - Buffer to hold the data retrieved
@param aMessage.Int2() - Size of the data to read.

@leave one of the system wide error codes
*/
void CSecuritySvrSession::ReadCrashLogL (const RMessage2& aMessage)
	{	
	//get the debug agent's thread and push handle onto clean up stack
	RThread clientThread;
	User::LeaveIfError(aMessage.Client(clientThread));
	CleanupClosePushL(clientThread);
	
	TInt err = CheckFlashAccessPermissionL(clientThread);
	
	CleanupStack::PopAndDestroy(&clientThread);
	
	if(KErrNone != err)
		{
		LOG_MSG2( "CSecuritySvrSession::ReadCrashLogL()  Access Not Granted - [%d]\n", err );
		aMessage.Complete(err);
		return;
		}
	
	//Check whether drive connected.
	if(!iCrashConnected)
		ConnectCrashPartitionL();	

	TInt readPosition = aMessage.Int0(); //read position
	
	TInt readSize = aMessage.Int2(); //read size
	
	RBuf8 readBuf;
	readBuf.CreateL(readSize);
	readBuf.CleanupClosePushL();
	
	err = iLocalDrive.Read (readPosition, readSize, readBuf);
	
	//write the list data back
	aMessage.WriteL (1, readBuf);	
	
	CleanupStack::PopAndDestroy (&readBuf);	
	
	//Complete message
	aMessage.Complete(err);	
	}
/**
Purpose: Function to write the crash config to the crash flash partition

@param aMessage.Int0() - write position in bytes from start position in flash partition.
@param aMessage.Ptr1() - Buffer containing the data to be written onto the flash. 
                         The size could be 0 if only flash partition size is needed. 
@param aMessage.Int2() - returns the size of the flash partition.

@leave one of the system wide error codes
*/
void CSecuritySvrSession::WriteCrashConfigL(const RMessage2& aMessage)
	{
	LOG_MSG( "CSecuritySvrSession::WriteCrashConfigL()\n" );
	
	//get the debug agent's thread and push handle onto clean up stack
	RThread clientThread;
	User::LeaveIfError(aMessage.Client(clientThread));
	CleanupClosePushL(clientThread);
	
	TInt err = CheckFlashAccessPermissionL(clientThread);
	
	CleanupStack::PopAndDestroy(&clientThread);
	
	if(KErrNone != err)
		{
		LOG_MSG2( "CSecuritySvrSession::WriteCrashConfigL()  Access Not Granted - [%d]\n", err );
		aMessage.Complete(err);
		return;
		}
	
	//Check whether drive connected.
	if(!iCrashConnected)
		ConnectCrashPartitionL();	
	
	// Get the length of the buffer
	TInt deslen = aMessage.GetDesLengthL(1);

	RBuf8 dataBuf;
	dataBuf.CreateL(deslen);
	dataBuf.CleanupClosePushL();

	// data to be written to flash
	aMessage.ReadL(1,dataBuf);
	
	TUint32 position = aMessage.Int0(); //position to start from
		
	err = iLocalDrive.Write(position,(const TDesC8&)dataBuf);
	
	TPtr8 dataSize((TUint8*)&deslen,4, 4);
	
	//write the size of the data written back
	aMessage.WriteL(2,dataSize);
		
	//destroy buffer
	CleanupStack::PopAndDestroy(&dataBuf); 
	
	aMessage.Complete(err);	
	}
/**
Purpose: Method to erase the crash flash block

@param aMessage.Int0() - write position in bytes from start position in flash partition.
@param aMessage.Int2() - Number of blocks to erase.

@leave one of the system wide error codes
*/

void CSecuritySvrSession::EraseCrashLogL(const RMessage2& aMessage)
	{	
	LOG_MSG( "CSecuritySvrSession::EraseCrashLogL()\n" );
	
	//get the debug agent's thread and push handle onto clean up stack
	RThread clientThread;
	User::LeaveIfError(aMessage.Client(clientThread));
	CleanupClosePushL(clientThread);
	
	TInt err = CheckFlashAccessPermissionL(clientThread);
	
	CleanupStack::PopAndDestroy(&clientThread);
	
	if(KErrNone != err)
		{
		LOG_MSG2( "CSecuritySvrSession::EraseCrashLogL()  Access Not Granted - [%d]\n", err );
		aMessage.Complete(err);
		return;
		}
			
	//Check whether drive connected.
	if(!iCrashConnected)
		ConnectCrashPartitionL();	
	
	TInt64 position = aMessage.Int0();	
	TInt size = aMessage.Int1();
	
	//Format drive
	err = iLocalDrive.Format(position,size*iCaps.iEraseBlockSize);		
 
	aMessage.Complete(err);	
	}

/**
Purpose: Method to erase the entire crash flash block
@leave one of the system wide error codes
*/

void CSecuritySvrSession::EraseEntireCrashLogL(const RMessage2& aMessage)
	{
	LOG_MSG( "CSecuritySvrSession::EraseEntireCrashLogL()\n" );
	
	//get the debug agent's thread and push handle onto clean up stack
	RThread clientThread;
	User::LeaveIfError(aMessage.Client(clientThread));
	CleanupClosePushL(clientThread);
	
	TInt err = CheckFlashAccessPermissionL(clientThread);
	
	CleanupStack::PopAndDestroy(&clientThread);
	
	if(KErrNone != err)
		{
		LOG_MSG2( "CSecuritySvrSession::EraseEntireCrashLogL()  Access Not Granted - [%d]\n", err );
		aMessage.Complete(err);
		return;
		}	
	
	//Check whether drive connected.
	if(!iCrashConnected)
		ConnectCrashPartitionL();
	
	TUint numberBlocks = iCaps.iSize /iCaps.iEraseBlockSize;
	
	//Format drive
	for(TInt i = 0; i < numberBlocks; i++)
		{
		err = iLocalDrive.Format(i*iCaps.iEraseBlockSize,iCaps.iEraseBlockSize);
		if(KErrNone != err)
			{
			RDebug::Printf("err = %d", err);
			aMessage.Complete(err);
			return;
			}
		}
	
	
	aMessage.Complete(err);
	}


/**
Purpose: Kill a specified process

@param aMessage.Ptr0() - Process Id of the thread to be stepped
@param aMessage.Int1() - Reason code to supply when killing the process.

@leave one of the system wide error codes

*/
void CSecuritySvrSession::KillProcessL(const RMessage2& aMessage)
	{
	LOG_MSG( "CSecuritySvrSession::KillProcessL()\n" );

	const TProcessId processId = ReadTProcessIdL(aMessage, 0);
	const TInt32 reason = aMessage.Int1();

	CheckAttachedL(processId, aMessage, EFalse);

	User::LeaveIfError(Server().iKernelDriver.KillProcess( processId, reason ));

	aMessage.Complete(KErrNone);
	}

/** Gets the secure id of aFileName
  @param aFileName file name of executable to get SID for
  @param aSecureId on return will contain the SID of aFileName

  @leave one of the system wide error codes
  */
void CSecuritySvrSession::GetSecureIdL(const TDesC& aFileName, TUid& aSecureId)
	{
	RFs fs;
	User::LeaveIfError(fs.Connect());
	CleanupClosePushL(fs);

	RFile targetExe;
	OpenFileHandleL(aFileName, fs, targetExe);
	CleanupClosePushL(targetExe);

	// Read in the entire header
	RBuf8 e32HdrBuf;
	e32HdrBuf.CreateL(RLibrary::KRequiredImageHeaderSize);
	e32HdrBuf.CleanupClosePushL();

	// Read the entire header as far as possible
	TInt err = targetExe.Read(e32HdrBuf);
	if (err != KErrNone)
		{
		// Could not read the file 
		LOG_MSG("CSecuritySvrSession::GetSecureIdL - Failed to read executable\n");

		User::Leave(err);
		}

	if(!CheckSufficientData(e32HdrBuf, IsExecutableXipL(targetExe)))
		{
		User::Leave(KErrGeneral);
		}

	aSecureId = GetSecureIdL(e32HdrBuf, IsExecutableXipL(targetExe));

	CleanupStack::PopAndDestroy(3, &fs);
	}

/** Get the secure id from aHeaderData
  @param aHeaderData an executable's header data to read SID from
  @param aXip indication of whether the header data is from an XIP file

  @return secure ID from aHeaderData
  */
TUid CSecuritySvrSession::GetSecureIdL(const TDesC8& aHeaderData, TBool aXip)
	{
	if(!CheckSufficientData(aHeaderData, aXip))
		{
		User::Leave(KErrGeneral);
		}

	if (aXip)
		{
		TRomImageHeader* hdr = (TRomImageHeader*)aHeaderData.Ptr();
		return TUid::Uid(hdr->iS.iSecureId);
		}
	else
		{
		// it is an epoc32 image
		E32ImageHeaderV* hdr = (E32ImageHeaderV*)aHeaderData.Ptr();
		return TUid::Uid(hdr->iS.iSecureId);
		}
	}

/**
@param aMessage contains:
 * aMessage.Ptr0() a TListDetails object
 * aMessage.Ptr1() a client supplied TDes8 for the driver to return data in
 * aMessage.Ptr2() a TUint32 for the driver to return the size of the requested listing's data in

@leave KErrTooBig if the buffer passed as argument 1 of aMessage is too
       small to contain the requested data,
       KErrNoMemory if a temporary buffer could not be allocated,
       or one of the other system wide error codes
*/
void CSecuritySvrSession::GetListL(const RMessage2& aMessage)
	{
	LOG_MSG("CSecuritySvrSession::GetListL()");

	// buffer to write list data into before copying back to agent
	RBuf8 listDetailsBuf;

	//allocate buffer
	listDetailsBuf.CreateL(sizeof(TListDetails));

	// Do the right cleanup if anything subsequently goes wrong
	listDetailsBuf.CleanupClosePushL();

	//read the data from the client thread
	aMessage.ReadL(0, listDetailsBuf);
	TListDetails* listDetails = (TListDetails*)listDetailsBuf.Ptr();

	//get the type of list requested
	TListId type = (TListId)aMessage.Int0();

	//create a buffer to store the data in
	RBuf8 buffer;
	buffer.CreateL(aMessage.GetDesMaxLength(1));
	buffer.CleanupClosePushL();

	//create a temporary variable to potentially store data length in
	TUint32 size = 0;

	TInt err = KErrNone;

	// the executables list is generated in the DSS rather than in the driver
	// so is treated separately
	if(listDetails->iListId == EExecutables)
		{
		if(listDetails->iListScope != EScopeGlobal)
			{
			User::Leave(KErrArgument);
			}
		if(listDetails->iTargetId != 0)
			{
			User::Leave(KErrArgument);
			}
		err = GetExecutablesListL(buffer, size);
		}
	else
		{
		err = Server().iKernelDriver.GetList(listDetails->iListId, listDetails->iListScope, listDetails->iTargetId, iDebugAgentProcessId, buffer, size);
		}

	if(err == KErrNone)
		{
		//write the list data back
		aMessage.WriteL(1, buffer);
		}

	TPtr8 sizePtr((TUint8*)&size, sizeof(TUint32), sizeof(TUint32));
	//write size back to agent
	aMessage.WriteL(2, sizePtr);

	CleanupStack::PopAndDestroy(&buffer);
	CleanupStack::PopAndDestroy(&listDetailsBuf);

	aMessage.Complete(err);
	}

/**
Gets the executables list and returns it in aBuffer if it's big enough

@param aBuffer caller supplied buffer to write data into
@param aSize on return contains the size of the data in the buffer, or the
       size that the buffer would need to be to contain the data

@return KErrNone on success, or KErrTooBig if the requested data will not fit in aBuffer

@leave one of the system wide error codes
*/
TInt CSecuritySvrSession::GetExecutablesListL(TDes8& aBuffer, TUint32& aSize) const
	{
	LOG_MSG("CSecuritySvrSession::GetExecutablesList()");

	//initialise values and connect to file system
	aSize = 0;
	aBuffer.SetLength(0);
	RFs fs;
	User::LeaveIfError(fs.Connect());
	CleanupClosePushL(fs);

	// uids corresponding to executable image
	TUidType uids(KExecutableImageUid, KNullUid, KNullUid);

	//create a string containing the directory name. The drive letter is represented
	//by X but will be replaced by the appropriate drive letter for each drive
	_LIT(KColonSysBin,":\\sys\\bin\\");

	//create a modifiable copy of KColonSysBin, preceeded by an empty space for the drive letter
	RBuf dirName;
	dirName.CreateL(1 + KColonSysBin().Length());
	dirName.CleanupClosePushL();

	//set the length to 1 (to later fill with the drive letter) and then append KColonSysBin
	dirName.SetLength(1);
	dirName.Append(KColonSysBin());

	//get the list of valid drives for the device
	TDriveList driveList;
	User::LeaveIfError(fs.DriveList(driveList));

	//check each valid sys/bin directory for executables
	for(TInt i=0; i<KMaxDrives; i++)
		{
		//if the drive is not valid then skip this drive
		if(!driveList[i])
			{
			//skip processing this drive
			continue;
			}

		//get the drive letter and insert it as the drive letter for dirName
		TChar driveLetter;
	       	User::LeaveIfError(fs.DriveToChar(i, driveLetter));
		dirName[0] = (TUint)driveLetter;

		//get a list of the exes in this drive's sys/bin directory
		CDir* localDir = NULL;
		TInt err = fs.GetDir(dirName, uids, ESortByName, localDir);
		if(KErrNoMemory == err)
			{
			User::Leave(err);
			}
		if(!localDir)
			{
			//skip processing this drive
			continue;
			}

		//push onto cleanup stack in case we leave
		CleanupStack::PushL(localDir);

		//iterate through the files
		for(TInt j=0; j<localDir->Count(); j++)
			{
			//will store x:\sys\bin\<file-name> type string
			RBuf fullPathName;

			TUint16 nameLength = dirName.Length() + (*localDir)[j].iName.Length();
			fullPathName.CreateL(nameLength);
			fullPathName.CleanupClosePushL();
			fullPathName.Copy(dirName);
			fullPathName.Append((*localDir)[j].iName);

			//add the data to the buffer
			AppendExecutableData(aBuffer, aSize, fullPathName);

			//do cleanup
			CleanupStack::PopAndDestroy(&fullPathName);
			}

		//do cleanup
		CleanupStack::PopAndDestroy(localDir);
		}

	//do cleanup
	CleanupStack::PopAndDestroy(2, &fs);

	//return appropriate value as to whether the kernel's data was too big
	return (aSize <= aBuffer.MaxLength()) ? KErrNone : KErrTooBig;
	}


/**
  Append data to aBuffer and update size of aSize if the data will fit. If it will
  not fit then just puts the nee size in aSize.

  @param aBuffer buffer to append the data to
  @param aSize on return contains the new size of the buffer if the data could be
  appended, otherwise aSize is updated to reflect the size the buffer would have if
  the data had fitted.
  @param aEntryName file name of the entry to add to the buffer
  */
void CSecuritySvrSession::AppendExecutableData(TDes8& aBuffer, TUint32& aSize, const TDesC& aEntryName) const
	{
	//update aSize to include the size of the data for this entry
	aSize = Align4(aSize + sizeof(TExecutablesListEntry) + (2*aEntryName.Length()) - sizeof(TUint16));

	//if the data will fit, and we haven't already stopped putting data in, then append the data,
	//if we've stopped putting data in then aSize will be bigger than aBuffer.MaxLength()
	if(aSize <= aBuffer.MaxLength())
		{
		TExecutablesListEntry& entry = *(TExecutablesListEntry*)(aBuffer.Ptr() + aBuffer.Length());
		//check whether an agent has registered to actively debug fullPathName
		TBool activelyDebugged = IsDebugged(aEntryName, EFalse);
		entry.iIsActivelyDebugged = activelyDebugged ? 1 : 0;

		//check whether any agents have registered to passively debug fullPathName
		TBool passivelyDebugged = IsDebugged(aEntryName, ETrue);
		entry.iIsPassivelyDebugged = passivelyDebugged ? 1 : 0;

		entry.iNameLength = aEntryName.Length();
		TPtr name(&(entry.iName[0]), aEntryName.Length(), aEntryName.Length());
		name = aEntryName;
		//pad the buffer to a four byte boundary
		aBuffer.SetLength(aSize);
		}
	}
/**
Helper function

Write data back to the thread that owns aMessage

@param aMessage the message which is passed between processes
@param aIndex the message slot which the data will be passed back in
@param aPtr pointer to data in this thread to be written into aMessage
@param aPtrSize size in bytes of the data to be written

@leave one of the system wide error codes
*/
void CSecuritySvrSession::WriteDataL(const RMessage2& aMessage, const TInt aIndex, const TAny* aPtr, const TUint32 aPtrSize) const
	{
	TPtr8 dataPtr((TUint8*)aPtr, aPtrSize, aPtrSize);

	aMessage.WriteL(aIndex, dataPtr);
	}

/**
Stores the PID of the debug agent, if it is not already stored.

@param aDebugAgentProcessId PID of the debug agent
*/
void CSecuritySvrSession::StoreDebugAgentId(const TProcessId aDebugAgentProcessId)
	{
	if(! iDebugAgentProcessIdStored)
		{
		iDebugAgentProcessIdStored = ETrue;
		iDebugAgentProcessId = aDebugAgentProcessId;
		}
	}

/**
Helper function.

Checks whether the debug agent (the owner of the aMessage) is attached to the
thread with thread id of aThreadId.

@param aThreadId thread ID of target debug thread
@param aMessage message owned by the debug agent
@param aPassive indicates whether to check if attached passively or actively

@leave KErrPermissionDenied if the agent is not attached to the process,
       KErrNoMemory if the security server could not be accessed
*/
void CSecuritySvrSession::CheckAttachedL(const TThreadId aThreadId, const RMessage2& aMessage, const TBool aPassive) const
	{
	//check that the agent has attached to the target process
	if(! Server().CheckAttached(aThreadId, aMessage, aPassive))
		{
		LOG_MSG("CSecuritySvrSession::CheckAttachedL() failed");
		User::Leave(KErrPermissionDenied);
		}
	}

/**
Helper function.

Checks whether the debug agent (the owner of the aMessage) is attached to the
process with process id of aProcessId.

@param aProcessId process ID of target debug thread
@param aMessage message owned by the debug agent
@param aPassive indicates whether to check if attached passively or actively

@leave KErrPermissionDenied if the agent is not attached to the process,
       KErrNoMemory if the security server could not be accessed
*/
void CSecuritySvrSession::CheckAttachedL(const TProcessId aProcessId, const RMessage2& aMessage, const TBool aPassive) const
	{
	
	//check that the agent has attached to the target process
	if(! Server().CheckAttached(aProcessId, aMessage, aPassive))
		{
		LOG_MSG("CSecuritySvrSession::CheckAttachedL() (process) failed");
		User::Leave(KErrPermissionDenied);
		}
	}

/**
Check whether the debug agent is permitted to attach to the target process.
Note that this function does not actually attach the agent to the process, it
simply tests whether an attach call would potentially be successful.

Currently this method returns ETrue in all cases but will be updated once
the security checking framework is in place.

@param aDebugAgentProcessId process id of the debug agent
@param aTargetProcessName original file name of the target process

@return ETrue if the debug agent would be allowed to attch to the target process,
        EFalse otherwise
*/
TBool CSecuritySvrSession::PermitDebugL(const TProcessId aDebugAgentProcessId, const TDesC& aTargetProcessName) const
	{
	return ETrue;
	}

/**
Helper function

Validates that the memory info passed in meets the debug driver's requirements

@param aMemoryInfo memory info passed in from client

@leave KErrArgument if:
	  * size is zero
	  * size is greater than the max block size
	  * size + address > 0xffffffff
	  * address is not access size aligned
	  * size is not a multiple of the access size
	KErrNotSupported if:
	  * iAccess is not TAccess::EAccess32
	  * iEndianess is not TEndianess::EEndLE8
	KErrUnknown if:
	  * the max memory block size cannot be determined
        or one of the other system wide error codes
*/
void CSecuritySvrSession::ValidateMemoryInfoL(const TThreadId aThreadId, const TMemoryInfo &aMemoryInfo, const TBool aReadOperation)
	{
	//check size is not 0
	if(aMemoryInfo.iSize == 0)
		User::Leave(KErrArgument);

	//get the max block size supported
	TUint32 maxSize = 0;
	User::LeaveIfError(Server().iKernelDriver.GetMemoryOperationMaxBlockSize(maxSize));

	//check that the block size given is less than the max block size
	if(aMemoryInfo.iSize > maxSize)
		User::Leave(KErrArgument);

	//must ensure that address + size <= 0xffffffff as will attempt to
	//read past 0xffffffff, which wouldn't be good
	TUint32 maxAddress = (~aMemoryInfo.iSize) + 1;
	if(aMemoryInfo.iAddress > maxAddress)
		User::Leave(KErrArgument);

	//check that arguments are supported
	if(aMemoryInfo.iAccess != EAccess32)
		User::Leave(KErrNotSupported);

	if(aMemoryInfo.iEndianess != EEndLE8)
		User::Leave(KErrNotSupported);

	//check that address is multiple of access size
	TInt addressIndicator = aMemoryInfo.iAddress % aMemoryInfo.iAccess;
	if(addressIndicator != 0)
		{
		User::Leave(KErrArgument);
		}

	//check that size is multiple of access size
	TInt sizeIndicator = aMemoryInfo.iSize % aMemoryInfo.iAccess;
	if(sizeIndicator != 0)
		User::Leave(KErrArgument);
	}

/**
Helper function

Validates that the three buffers relating to reading register data are of
appropriate sizes, and calculates the number of registers being requested.

@param aMessage message which in offsets 1, 2 and 3 contains descriptors
@param aNumberOfRegisters if the function returns with KErrNone this will
       contain the number of registers being requested, guaranteed to be non-zero

@leave KErrArgument if descriptors do not represent the same number of
       registers, if any of the descriptors have max length of 0, if any of
       the descriptors have max lengths which are not multiples of their data
       type's size or if any of the descriptors have max lengths greater than
       the max block size for memory operations
       or one of the other system wide error codes if there were problems 
       in getting the descriptors' lengths.
*/
void CSecuritySvrSession::ValidateRegisterBuffersL(const RMessage2& aMessage, TUint32& aNumberOfRegisters)
	{
	//get lengths of buffers, if error occurs returned value will be less then zero
	TInt idsBufferLength = aMessage.GetDesMaxLength(1);
	if(idsBufferLength < 0)
		{
		User::Leave(idsBufferLength);
		}
	TInt valuesBufferLength = aMessage.GetDesMaxLength(2);
	if(valuesBufferLength < 0)
		{
		User::Leave(valuesBufferLength);
		}
	TInt flagsBufferLength = aMessage.GetDesMaxLength(3);
	if(flagsBufferLength < 0)
		{
		User::Leave(flagsBufferLength);
		}

	//get the max block size supported
	TUint32 maxSize = 0;
	User::LeaveIfError(Server().iKernelDriver.GetMemoryOperationMaxBlockSize(maxSize));

	//check none of the descriptors have size greater than the max block size
	if((idsBufferLength > maxSize) || (valuesBufferLength > maxSize) || (flagsBufferLength > maxSize))
		User::Leave(KErrArgument);

	//get sizes of the three types of data the buffers represent arrays of
	//and validate that the buffer lengths are multiples of the data sizes
	TUint idSize = sizeof(TRegisterInfo);
	if(idsBufferLength % idSize != 0)
		User::Leave(KErrArgument);

	TUint flagSize = sizeof(TUint8);
	if(flagsBufferLength % flagSize != 0)
		User::Leave(KErrArgument);

	//perform check on id buffer length
	if(idsBufferLength == 0)
		User::Leave(KErrArgument);

	//calculate number of registers being requested
	aNumberOfRegisters = idsBufferLength / idSize;

	//check flags buffer is of appropriate size
	if(flagsBufferLength != (aNumberOfRegisters * flagSize))
		User::Leave(KErrArgument);
	}

/**
Establish whether any agents have registered to debug the specified aFileName

@param aFileName originating file name of the target process
@param aPassive indicates whether to check if there has been active attachment,
or passive attachment.

@return ETrue if aFileName is being debugged, EFalse otherwise

*/
TBool CSecuritySvrSession::IsDebugged(const TDesC& aFileName, const TBool aPassive) const
	{
	//check whether the target process is being debugged
	return Server().IsDebugged(aFileName, aPassive);
	}

/**
  Helper function which reads a TThreadId object from a client

  @param aMessage the message object containing the reference to the TThreadId
  @param aIndex the message argument containing the reference

  @return the TThreadId passed in by the client
  @leave KErrArgument if aIndex is outside of the valid range
  */
TThreadId CSecuritySvrSession::ReadTThreadIdL(const RMessagePtr2& aMessage, const TInt aIndex) const
	{
	//create a temporary TThreadId to read the data into
	TThreadId tempThreadId;
	TPtr8 threadIdPtr((TUint8*)&tempThreadId, sizeof(TThreadId));

	// read the data in from the client
	aMessage.ReadL(aIndex, threadIdPtr);

	return tempThreadId;
	}

/**
  Helper function which reads a TProcessId object from a client

  @param aMessage the message object containing the reference to the TProcessId
  @param aIndex the message argument containing the reference

  @return the TProcessId passed in by the client
  @leave KErrArgument if aIndex is outside of the valid range
  */
TProcessId CSecuritySvrSession::ReadTProcessIdL(const RMessagePtr2& aMessage, const TInt aIndex) const
	{
	//create a temporary TProcessId to read the data into
	TProcessId tempProcessId;
	TPtr8 processIdPtr((TUint8*)&tempProcessId, sizeof(TProcessId));

	// read the data in from the client
	aMessage.ReadL(aIndex, processIdPtr);

	return tempProcessId;
	}

// End of file - c_security_svr_session.cpp
