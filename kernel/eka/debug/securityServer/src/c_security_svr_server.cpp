// Copyright (c) 2006-2010 Nokia Corporation and/or its subsidiary(-ies).
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
// Provides the debug security server server implementation.
// 
//

/**
 @file
 @internalTechnology
 @released
*/

#include <e32base.h>
#include <e32base_private.h>
#include <rm_debug_api.h>
#include "c_process_pair.h"
#include "c_security_svr_session.h"
#include "c_security_svr_server.h"
#include "rm_debug_logging.h"

using namespace Debug;

/**
Server constructor, sessions are created as ESharableSessions, meaning that
each session will be used by at most one debug agent
*/
CSecuritySvrServer::CSecuritySvrServer(CActive::TPriority aActiveObjectPriority)
	: CServer2(aActiveObjectPriority, ESharableSessions),
	  iSessionCount(0),
	  iShutdown()
	{
	LOG_MSG("CSecuritySvrServer::CSecuritySvrServer()\n");
	}

/**
Standard implementation

@return pointer to new CSecuritySvrServer object
*/
CSecuritySvrServer* CSecuritySvrServer::NewLC()
	{
	LOG_MSG("CSecuritySvrServer::NewLC()\n");

	CSecuritySvrServer* self=new(ELeave) CSecuritySvrServer(EPriorityStandard);
	CleanupStack::PushL(self);
	self->ConstructL();
	return self;
	}

/**
Server destructor, performs cleanup for the server
*/
CSecuritySvrServer::~CSecuritySvrServer()
	{
	LOG_MSG("CSecuritySvrServer::~CSecuritySvrServer()\n");

	// stop the kernel side driver
	iKernelDriver.Close();
	User::FreeLogicalDevice(KDebugDriverName);

	//deallocate both the debug maps
	iPassiveDebugMap.ResetAndDestroy();
	iActiveDebugMap.ResetAndDestroy();
	}

/**
Starts the server and constructs and starts the servers shutdown timer
*/
void CSecuritySvrServer::ConstructL()
	{
	LOG_MSG("CSecuritySvrServer::ConstructL()");

	StartL(KSecurityServerName);
	iShutdown.ConstructL();
	iShutdown.Start();

	//load the kernel driver
	TInt err = User::LoadLogicalDevice(KDebugDriverFileName);
	if(! ((KErrNone == err) || (KErrAlreadyExists == err)))
		{
		User::Leave(err);
		}
	//create an information object for initialising the driver
	TRM_DebugDriverInfo driverInfo;
	driverInfo.iUserLibraryEnd = 0;
	User::LeaveIfError(iKernelDriver.Open(driverInfo));
	}

/**
Creates a new session with the DSS. A version check is done to ensure that an
up to date version of the DSS is available (according to the DA's needs).
The device driver is loaded if necessary and a session with the server and a 
handle to the driver opened.

@param aRequiredVersion the minimal version of the DSS required by the DA

@return a pointer to the new sever session, or NULL if any of the 
        initialisation process failed
*/
CSession2* CSecuritySvrServer::NewSessionL(const TVersion& aRequiredVersion, const RMessage2& aMessage) const
//
// Session constructor
//
	{
	LOG_ARGS("version=%d.%d.%d", aRequiredVersion.iMajor, aRequiredVersion.iMinor, aRequiredVersion.iBuild);

	//assert compatible version
	TVersion currentVersion(KDebugServMajorVersionNumber, KDebugServMinorVersionNumber, KDebugServPatchVersionNumber);
	if(!User::QueryVersionSupported(currentVersion, aRequiredVersion))
		{
		LOG_MSG("Requested version not compatible with this version. Asked for %d.%d.%d but this is %d.%d.%d", aRequiredVersion.iMajor, aRequiredVersion.iMinor, aRequiredVersion.iBuild, KDebugServMajorVersionNumber, KDebugServMinorVersionNumber, KDebugServPatchVersionNumber);
		User::Leave(KErrNotSupported);
		}

	//create session
	LOG_MSG("About to call new(ELeave) CSecuritySvrSession()");
	CSecuritySvrSession* servSession = new(ELeave) CSecuritySvrSession();

	CleanupStack::PushL(servSession);
	servSession->ConstructL();
	CleanupStack::Pop(servSession);
	return servSession;
	}

/**
Manages requests from debug agents to attach to target debug processes

Given the debug agent process ID and the target process name:
(1) checks whether the pair is already in either of the debug maps, if so
    then returns KErrAlreadyExists
(2) if aPassive == ETrue then just add the pair to the passive map and return
    whatever the return value of the array write was
(3) if aPassive == EFalse then check whether the target debug process is 
    already reserved by another debug agent. If it is then return KErrInUse,
    otherwise add the pair to the active debug map and return the status 
    value of the array write.

@param aTargetProcessName original FileName of the process to attach to
@param aDebugAgentProcessId process ID of the debug agent
@param aPassive ETrue if wish to attach passively, EFalse otherwise

@return KErrNone if successfully attached, otherwise another system wide error
        code as above
*/
TInt CSecuritySvrServer::AttachProcessL(const TDesC& aTargetProcessName, const TProcessId aDebugAgentProcessId, const TBool aPassive)
	{
	//store the pair of values
	LOG_MSG( "CSecuritySvrServer::AttachProcessL()\n" );

	CProcessPair *processPair = CProcessPair::NewL(aTargetProcessName, aDebugAgentProcessId);
	if(processPair == NULL)
		return KErrNoMemory;

	//check whether the pair already exists in the active debug map
	for(TInt i=0; i<iActiveDebugMap.Count(); i++)
		{
		if(*processPair == *(iActiveDebugMap[i]))
			{
			//process already exists
			LOG_MSG( "  AttachProcessL() error : KErrAlreadyExists in active map\n" );
			delete processPair;
			return KErrAlreadyExists;
			}
		}

	//check whether the pair already exists in the passive map
	for(TInt i=0; i<iPassiveDebugMap.Count(); i++)
		{
		if(*processPair == *(iPassiveDebugMap[i]))
			{
			//process already exists
			LOG_MSG( "  AttachProcessL() error : KErrAlreadyExists in passive map\n" );
			delete processPair;
			return KErrAlreadyExists;
			}
		}

	if(aPassive)
		{
		//just add the pair and return
		TInt err = iPassiveDebugMap.Append(processPair);
		if(err != KErrNone)
			{
			// couldn't add pair for some unknown reason, so delete the pair
			LOG_MSG2( "  AttachProcessL() error %d appending passive process pair \n", err );
			delete processPair;
			}
		return err;
		}
	else
		{
		//check whether the process Id has already been reserved
		for(TInt i=0; i<iActiveDebugMap.Count(); i++)
			{
			if(processPair->ProcessNameMatches(*(iActiveDebugMap[i])))
				{
				//process already being debugged
				LOG_MSG( "  AttachProcessL() error : process already being debugged\n" );
				delete processPair;
				return KErrInUse;
				}
			}
		//try to add the pair	
		TInt err = iActiveDebugMap.Append(processPair);
		if(err != KErrNone)
			{
			// couldn't add pair for some unknown reason, so delete the pair
			LOG_MSG2( "  AttachProcessL() error %d appending active process pair \n", err );
			delete processPair;
			}
		return err;
		}
	}

/*
Detach from debugging the specified process

@param aTargetProcessName name of the process to detach from
@param aDebugAgentProcessId process ID of the debug agent

@return KErrNone if successfully detached, KErrNotFound if an attempt is made
        to detach from a process which the debug agent hasn't previously attached to
*/
TInt CSecuritySvrServer::DetachProcess(const TDesC& aTargetProcessName, const TProcessId aDebugAgentProcessId)
	{
	//check whether the pair is in the active debug map
	for(TInt i=0; i<iActiveDebugMap.Count(); i++)
		{
		if(iActiveDebugMap[i]->Equals(aTargetProcessName, aDebugAgentProcessId))
			{
			//remove the process pair from the active debug map
			delete iActiveDebugMap[i];
			iActiveDebugMap.Remove(i);
			return KErrNone;
			}
		}

	//check whether the pair is in the passive debug map
	for(TInt i=0; i<iPassiveDebugMap.Count(); i++)
		{
		if(iPassiveDebugMap[i]->Equals(aTargetProcessName, aDebugAgentProcessId))
			{
			//remove the process pair from the active debug map
			delete iPassiveDebugMap[i];
			iPassiveDebugMap.Remove(i);
			return KErrNone;
			}
		}

	//process pair wasn't in either map
	return KErrNotFound;
	}

/**
Given a debug agent process ID, removes all references to that debug agent
from the debug maps

@param aMessage message from the debug agent

@return returns KErrNone if successful, another system wide error code otherwise
*/
void CSecuritySvrServer::DetachAllProcesses(const TProcessId aDebugAgentProcessId)
	{
	//check whether the debug agent process ID is in the active debug map
	for(TInt i=iActiveDebugMap.Count()-1; i>=0; i--)
		{
		if(iActiveDebugMap[i]->ProcessIdMatches(aDebugAgentProcessId))
			{
			//remove the process pair from the active debug map
			delete iActiveDebugMap[i];
			iActiveDebugMap.Remove(i);
			}
		}

	//check whether the debug agent process ID is in the passive debug map
	for(TInt i=iPassiveDebugMap.Count()-1; i>=0; i--)
		{
		if(iPassiveDebugMap[i]->ProcessIdMatches(aDebugAgentProcessId))
			{
			//remove the process pair from the passive debug map
			delete iPassiveDebugMap[i];
			iPassiveDebugMap.Remove(i);
			}
		}
	}

/*
Check whether the specified debug agent is attaced to the specfied target
process.

@param aTargetThreadId thread ID of a thread in the target process
@param aMessage a message which originates with the debug agent
@param aPassive if EFalse then checks whether the debug agent is the active
       debugger of the target process. If ETrue then checks whether the debug
       agent is attached to the target process, irrespective of whether it is
       attached passively or actively

@return ETrue if attached, EFalse otherwise
*/
TBool CSecuritySvrServer::CheckAttached(const TThreadId aTargetThreadId, const RMessage2& aMessage, const TBool aPassive)
	{
	
	//get a handle to the target thread
	RThread targetThread;
	TInt err = targetThread.Open(aTargetThreadId);
	if(err != KErrNone)
		{
		return EFalse;
		}

	//get a handle to the target process
	RProcess targetProcess;
	err = targetThread.Process(targetProcess);
	//finshed with the thread handle so close it
	targetThread.Close();
	if(err != KErrNone)
		return EFalse;

	//get the target process' file name
	TFileName targetFileName = targetProcess.FileName();

	// Tamperproofing. Ensure that the debug agent really has a superset
	// of the target process PlatSec capabilities, as authorised
	// by an OEM Debug Token (if any)

	TSecurityInfo targetSecInfo(targetProcess);

	// Now compare the capabilities, to ensure the DebugAgent has been authorised with
	// sufficient capabilities from its OEM Debug token
	CSecuritySvrSession* session = (CSecuritySvrSession*)aMessage.Session();

	// Presume we need to check the target process is debuggable unless a valid OEM Debug token in effect?
	if (!OEMTokenPermitsDebugL(session->GetOEMDebugCapabilities(), targetSecInfo.iCaps) )
		{
		// No debug token therefore check if the process is debuggable
		err = iKernelDriver.IsDebuggable(targetProcess.Id());
		}

	//finished with the process handle so close it
	targetProcess.Close();

	if (err != KErrNone)
	{
		// The process was not marked as debuggable by the loader, and the OEM
		// debug token did not override the lack of a debuggable bit.
		// The process was not marked as debuggable by the loader
		return EFalse;
	}

	return CheckAttachedProcess(targetFileName, aMessage, aPassive);
	}

/*
Check whether the specified debug agent is attaced to the specfied target
process.

@param aTargetProcessId process ID of the target process
@param aMessage a message which originates with the debug agent
@param aPassive if EFalse then checks whether the debug agent is the active
       debugger of the target process. If ETrue then checks whether the debug
       agent is attached to the target process, irrespective of whether it is
       attached passively or actively

@return ETrue if attached, EFalse otherwise
*/
TBool CSecuritySvrServer::CheckAttached(const TProcessId aTargetProcessId, const RMessage2& aMessage, const TBool aPassive)
	{
	//get a handle to the target process
	RProcess targetProcess;
	TInt err =targetProcess.Open(aTargetProcessId);
	if(err != KErrNone)
		{
		return EFalse;
		}

	//get the target process' file name
	TFileName targetFileName = targetProcess.FileName();

	// Tamperproofing. Ensure that the debug agent really has a superset
	// of the target process PlatSec capabilities, as authorised
	// by an OEM Debug Token (if any)

	TSecurityInfo targetSecInfo(targetProcess);

	// Now compare the capabilities, to ensure the DebugAgent has been authorised with
	// sufficient capabilities from its OEM Debug token
	CSecuritySvrSession* session = (CSecuritySvrSession*)aMessage.Session();

	// Presume we need to check the target process is debuggable unless a valid OEM Debug token in effect?
	if ( !OEMTokenPermitsDebugL(session->GetOEMDebugCapabilities(), targetSecInfo.iCaps) )
		{
		// No debug token therefore check if the process is debuggable
		err = iKernelDriver.IsDebuggable(targetProcess.Id());
		}

	//finished with the process handle so close it
	targetProcess.Close();

	if (err != KErrNone)
	{
		return EFalse;
	}

	return CheckAttachedProcess(targetFileName, aMessage, aPassive);
	}

/*
Check whether the specified debug agent is attaced to the specfied target
process.

@param aTargetProcessName 
@param aMessage a message which originates with the debug agent

@return ETrue if attached, EFalse otherwise
*/
TBool CSecuritySvrServer::CheckAttachedProcess(const TDesC& aTargetProcessName, const RMessage2& aMessage, const TBool aPassive) const
	{
	//get the debug agent's process id
	TProcessId clientProcessId = 0;
	TInt err = GetProcessIdFromMessage(clientProcessId, aMessage);
	if(err != KErrNone)
		return EFalse;

	//check permissions
	if(aPassive)
		return IsDebugger(aTargetProcessName, clientProcessId);
	else
		return IsActiveDebugger(aTargetProcessName, clientProcessId);
	}

/**
Tests whether the debug agent is attached actively to the target debug process

@param aTargetProcessName target debug process' FileName
@param aDebugAgentProcessId process ID of a debug agent

@return ETrue if the specified debug agent is actively attached to the 
        specified target debug process, EFalse otherwise
*/
TBool CSecuritySvrServer::IsActiveDebugger(const TDesC& aTargetProcessName, const TProcessId aDebugAgentProcessId) const
	{
	//check whether the pair is in the active debug map
	for(TInt i=0; i<iActiveDebugMap.Count(); i++)
		{
		if(iActiveDebugMap[i]->Equals(aTargetProcessName, aDebugAgentProcessId))
			return ETrue;
		}
	//not found so return false
	return EFalse;
	}

/**
Tests whether the target process is being debugged

@param aTargetProcessName target process' FileName
@param aPassive indicates whether to check for the process being actively debugged,
or passively debugged

@return ETrue if the specified target process is being debugged,
        EFalse otherwise
*/
TBool CSecuritySvrServer::IsDebugged(const TDesC& aTargetProcessName, const TBool aPassive) const
	{
	//get a reference to the appropriate list
	const RPointerArray<CProcessPair>& map = (aPassive) ? iPassiveDebugMap : iActiveDebugMap;

	//iterate through the map trying to match the aTargetProcessName
	for(TInt i=0; i<map.Count(); i++)
		{
		if(map[i]->ProcessNameMatches(aTargetProcessName))
			{
			return ETrue;
			}
		}
	return EFalse;
	}

/**
Tests whether the debug agent is attached to the target debug process

@param aTargetProcessName target debug process' FileName
@param aDebugAgentProcessId process ID of a debug agent

@return ETrue if the specified debug agent is attached to the 
        specified target debug process (regardless of whether it is attached
	passively or actively), EFalse otherwise
*/
TBool CSecuritySvrServer::IsDebugger(const TDesC& aTargetProcessName, const TProcessId aDebugAgentProcessId) const
	{
	//check whether the pair is in the active debug map
	if(IsActiveDebugger(aTargetProcessName, aDebugAgentProcessId))
		return ETrue; 

	//check whether the pair is in the passive debug map
	for(TInt i=0; i<iPassiveDebugMap.Count(); i++)
		{
		if(iPassiveDebugMap[i]->Equals(aTargetProcessName, aDebugAgentProcessId))
			return ETrue;
		}
	//not found so return false
	return EFalse;
	}

/**
Decrements the server's count of how many sessions are connected to it and
starts the shutdown timer if there are no sessions connected
*/
void CSecuritySvrServer::SessionClosed()
	{
	if(--iSessionCount < 1)
		{
		iShutdown.Start();
		}
	}

/**
Increments the servers count of how many sessions are connected to it and
cancels the shutdown timer if it is running
*/
void CSecuritySvrServer::SessionOpened()
	{
	iSessionCount++;
	iShutdown.Cancel();
	}

/** 
  Get the process id of the thread which sent aMessage
  @param aProcessId process id of the thread which sent aMessage
  @param aMessage message object sent by thread 

  @return KErrNone if aProcessId could be set, or one of the system wide error codes if not
  */
TInt CSecuritySvrServer::GetProcessIdFromMessage(TProcessId& aProcessId, const RMessage2& aMessage) const
	{
	//get the debug agent's thread
	RThread clientThread;
	TInt err = aMessage.Client(clientThread);
	if(err != KErrNone)
		{
		return err;
		}

	//get the debug agent's process
	RProcess clientProcess;
	err = clientThread.Process(clientProcess);

	//finished with the thread handle so close it
	clientThread.Close();

	//check if there was an error from getting the process
	if(err != KErrNone)
		{
		return err;
		}

	//get the debug agent's process id
	aProcessId = clientProcess.Id();

	//finished with the process handle so close it
	clientProcess.Close();

	return KErrNone;
	}

/**
  Helper function which determines whether the capabilities of the
  OEM Token are sufficient to permit debug of an application.

  Normally, we use the AllFiles capability as a proxy which
  means a Debug Agent can debug non-debuggable executables,
  provided it has a superset of the capabilities of the executable
  to be debugged.
 
  However, this causes the problem that all OEM Debug Tokens implicitly
  give the power to debug an AllFiles executable, even if all that
  is really needed is the power to debug an app with no capabilities,
  or capabilities other than AllFiles.
  
  To address this, we treat the AllFiles capability in a special way.
  The AllFiles capability in a token is taken to mean that an OEM has
  signed the token, and hence can debug other executables. But this does
  not inclue the ability to debug an AllFiles executable. To debug an AllFiles
  executable, the token must also have TCB.

  @param aTokenCaps - The PlatSec capabilities of a token
  @param aTargetCaps - The PlatSec capabilities of a target app to be debugged

  @return ETrue if authorised for debugging, EFalse otherwise.

  @leave Any system error code.
  */
TBool CSecuritySvrServer::OEMTokenPermitsDebugL(const TCapabilitySet aTokenCaps, const TCapabilitySet aTargetCaps)
	{	
	LOG_MSG("CSecuritySvrSession::OEMTokenPermitsDebugL\n");

	// Is the token valid - i.e. does it have AllFiles.
	if ( !aTokenCaps.HasCapability(ECapabilityAllFiles) )
		{
		// Token is not valid, as it does not have AllFiles.
		LOG_MSG("CSecuritySvrSession::OEMTokenPermitsDebugL - Token does not have AllFiles\n");
			
		return EFalse;
		}

	// Token MUST have a strict superset of capabilities
	if ( !aTokenCaps.HasCapabilities(aTargetCaps) )
		{
		// Token does not have at least all the capabilities of the target
		LOG_MSG("CSecuritySvrSession::OEMTokenPermitsDebugL - Token does not have superset of target capabilities\n");

		return EFalse;
		}

	// Special case: If the target has AllFiles, the Token must have TCB
	if ( aTargetCaps.HasCapability(ECapabilityAllFiles) )
		{
		// Target has AllFiles, so does the Token have TCB?
		if ( !aTokenCaps.HasCapability(ECapabilityTCB) )
			{
			// Token does not have TCB.
			LOG_MSG("CSecuritySvrSession::OEMTokenPermitsDebugL - Token does not have TCB when target has AllFiles\n");
	
			return EFalse;
			}
		}

	// If we have passed all the above tests, the token permits debug
	return ETrue;
	}

/**
 * This looks at a debug tokens capability and ensures it is sufficient 
 * to provide access to the flash partition
 * @param aTokenCaps Capabilties of the Token
 * @return TBool Whether or not flash access is permitted
 */
TBool CSecuritySvrServer::OEMTokenPermitsFlashAccessL(const TCapabilitySet aTokenCaps)
	{	
	//Must have TCB to access flash
	return aTokenCaps.HasCapability(ECapabilityTCB);
	}

//eof

