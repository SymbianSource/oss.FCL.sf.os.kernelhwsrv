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
// Definitions for the security server server side session.
// 
//

#ifndef C_SECURITY_SVR_SESSION_H
#define C_SECURITY_SVR_SESSION_H

/**
@file
@internalTechnology
@released
*/

// forward declaration
class CSecuritySvrAsync;

#include "c_security_svr_async.h"
#include <f32file.h>
#include <d32locd.h>

#include <rm_debug_api.h>

#include "rm_debug_kerneldriver.h"

// Server name
_LIT(KDebugDriverName,"RunMode Debug Driver");

class CSecuritySvrServer;

/**
Debug Security Server session. Manages the session with one debug agent and
as many target executables as it has attached to.
*/
class CSecuritySvrSession : public CSession2
	{
public:
	CSecuritySvrSession();
	~CSecuritySvrSession();
	void ConstructL ();
	void CreateL();

	TInt OpenHandle(const TRM_DebugDriverInfo& aDriverInfo);
	void ServiceL(const RMessage2& aMessage);
	void ServiceError(const RMessage2 &aMessage, TInt aError);

	void ResumeThreadL(const RMessage2& aMessage);
	void SuspendThreadL(const RMessage2& aMessage);
	//break
	void SetBreakL(const RMessage2& aMessage);
	void ClearBreakL(const RMessage2& aMessage);
	void ModifyBreakL(const RMessage2& aMessage);
	void BreakInfoL(const RMessage2& aMessage);

	void StepRangeL(const RMessage2& aMessage);

	void GetEventL(const RMessage2& aMessage);
	void CancelGetEventL(const RMessage2& aMessage);

	void AttachProcessL(const RMessage2& aMessage);
	void DetachProcessL(const RMessage2& aMessage);
	//debug functionality
	void GetDebugFunctionalityBufSizeL(const RMessage2& aMessage);
	void GetDebugFunctionalityL(const RMessage2& aMessage);
	//memory
	void ReadMemoryL(const RMessage2& aMessage);
	void WriteMemoryL(const RMessage2& aMessage);
	//registers
	void ReadRegistersL(const RMessage2& aMessage);
	void WriteRegistersL(const RMessage2& aMessage);
	//event
	void SetEventActionL(const RMessage2& aMessage);

	void GetListL(const RMessage2& aMessage);
	void StepL(const RMessage2& aMessage);
	void TraceExecutableL(const RMessage2& aMessage);
	
	//crash log
	void ReadCrashLogL(const RMessage2& aMessage);
	void WriteCrashConfigL(const RMessage2& aMessage);
	void EraseCrashLogL(const RMessage2& aMessage);
	void EraseEntireCrashLogL(const RMessage2& aMessage);

	void SetProcessBreakL(const RMessage2& aMessage);
	void ModifyProcessBreakL(const RMessage2& aMessage);
	void ProcessBreakInfoL(const RMessage2& aMessage);

	void KillProcessL(const RMessage2& aMessage);

	TCapabilitySet GetOEMDebugCapabilities(void) const { return iOEMDebugCapabilities; };

#ifdef _DEBUG
	void DoFailAlloc(const RMessage2& aMessage);
#endif

private:
	CSecuritySvrServer& Server() const;
	void HeapWatcher(const TUint32 aFunction, const TBool aEntry) const;
	void WriteDataL(const RMessage2& aMessage, const TInt aIndex, const TAny* aPtr, const TUint32 aPtrSize) const;
	void StoreDebugAgentId(const TProcessId aDebugAgentProcessId);
	void CheckAttachedL(const TThreadId aThreadId, const RMessage2& aMessage, const TBool aPassive) const;
	void CheckAttachedL(const TProcessId aProcessId, const RMessage2& aMessage, const TBool aPassive) const;
	TBool PermitDebugL(const TProcessId aDebugAgentProcessId, const TDesC& aTargetProcessName) const;
	TBool IsDebugged(const TDesC& aFileName, const TBool aPassive) const;
	void OpenFileHandleL(const TDesC& aFileName, RFs& aFs, RFile& aFileHandle);
	TBool IsTraceBitSet(const TDesC8& aHeaderData, const TBool aXip);
	TBool IsDebugBitSet(const TDesC8& aHeaderData, const TBool aXip);
	TBool CheckSufficientData(const TDesC8& aHeaderData, const TBool aXip) const;

	void ValidateMemoryInfoL(const TThreadId aThreadId, const Debug::TMemoryInfo &aMemoryInfo, const TBool aReadOperation);
	void ValidateRegisterBuffersL(const RMessage2& aMessage, TUint32& aNumberOfRegisters);

	TInt GetExecutablesListL(TDes8& aBuffer, TUint32& aSize) const;
	void AppendExecutableData(TDes8& aBuffer, TUint32& aSize, const TDesC& aEntryName) const;
	void GetSecureIdL(const TDesC& aFileName, TUid& aSecureId);
	TUid GetSecureIdL(const TDesC8& aHeaderData, TBool aXip);

	void IsDebuggableL(const TDesC& aFileName);
	TThreadId ReadTThreadIdL(const RMessagePtr2& aMessage, const TInt aIndex) const;
	TProcessId ReadTProcessIdL(const RMessagePtr2& aMessage, const TInt aIndex) const;
	TBool IsExecutableXipL(RFile& aExecutable);
	
	void ConnectCrashPartitionL(void);

	void GetDebugAgentOEMTokenCapsL();
	TInt CheckFlashAccessPermissionL(const RThread aClientThread);

	// Declare the CSecuritySvrAsync as a friend so it can use the iKernelDriver too
	friend class CSecuritySvrAsync;

private:
	/**
	Flag to indicate whether we have stored the TProcessId associated with the Debug Agent.
	*/
	TBool iDebugAgentProcessIdStored;

	/**
	The TProcessId of the Debug Agent associated with this session. A convenience to
	save looking it up repeatedly.
	*/
	TProcessId iDebugAgentProcessId;
	/**
	Need an array of async completion objects, one for each target executable.
	*/
	RPointerArray<CSecuritySvrAsync> iAsyncHandlers;

	/**
	Used to track whether the Debug Agent has been notified when closing the session.
	*/
	TBool iServerNotified;

	/**
	OEM Debug token support. This is only used when the Debug Agent has OEM debug 
	authority provided by a specific authorisation token file. This token confers
	the ability to debug certain executables which have not been built as 'Debuggable'.
	
	The OEM Debug token executable must be marked with 'AllFiles', as this is analogous
	to looking 'inside' executables - with AllFiles, it could read all the data out of an
	executable in \sys\bin\. In addition, since debug control of an executable implies the
	ability to execute arbitrary code within the target process space, this would imply that
	a Debug Agent could use any PlatSec capability which that target process possessed.
	
	Therefore, we require that the OEM Debug Token must also be marked with a superset of
	the PlatSec capabilities of the executable which is to be debugged. This means the
	Debug Agent is not granted more access/PlatSec capabilities than its authorisation
	token allows, and cannot exploit a target executable to leverage greater access than
	should be permitted.

	iTargetCapabilities tracks which PlatSec capabilities the target executables may
	possess and still be debugged by this debug agent. The capabilities are NOT those
	of the debug agent process, they are the capabilites indicated in the OEM Debug Token
	which describe the capabilities the debug agent is authorised to debug. E.g. a Debug
	Agent might use CommsDD, but wish to debug a DRM capable executable. In that case, the
	Debug Agent exe must be signed with CommsDD, but the OEM Debug Token need only possess
	DRM and AllFiles (permission to look inside another executable).
	*/
	TCapabilitySet iOEMDebugCapabilities;
	
	//RLocalDrive to access the crash Flash
	RLocalDrive iLocalDrive;
	
	//For NOR flash 
	TLocalDriveCapsV2 iCaps;
	
	/**
	 * If true means the local drive connected to the crash partition else connect 
	 * when access required to crash flash partition for read operation
	*/	
	TBool iCrashConnected;
	};


#endif // C_SECURITY_SVR_SESSION_H

