// Copyright (c) 2004-2009 Nokia Corporation and/or its subsidiary(-ies).
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
//



/**
 @file
 @internalTechnology
 @released
*/

#ifndef __RM_DEBUG_DRIVER_H__
#define __RM_DEBUG_DRIVER_H__

#include "d_rmd_stepping.h"
#include "d_rmd_breakpoints.h"
#include "d_driver_event_info.h"

// From mmboot.h header
const TLinAddr	KDataSectionEnd			=0x40000000u;
const TLinAddr	KRomLinearBase			=0xF8000000u;


#define ROM_LINEAR_BASE KRomLinearBase

// Result checking
#define ReturnIfError(x) { TInt y = x; if (KErrNone != y) return y; }

//
// class DRM_DebugDriverFactory
//
class DRM_DebugDriverFactory : public DLogicalDevice
{
public:

	DRM_DebugDriverFactory();
	virtual TInt Install();
	virtual void GetCaps(TDes8& aDes) const;
	virtual TInt Create(DLogicalChannelBase*& aChannel);
};

class DRM_DebugEventHandler;
//
// DRM_DebugChannel
//
class DRM_DebugChannel : public DLogicalChannel
{
public:

	DRM_DebugChannel(DLogicalDevice* aLogicalDevice);
	~DRM_DebugChannel();

	virtual TInt DoCreate(TInt aUnit, const TDesC* anInfo, const TVersion& aVer);	
	virtual void HandleMsg(TMessageBase* aMsg);
	virtual TInt SendMsg(TMessageBase* aMsg);
	TInt SendRequest(TMessageBase* aMsg);
	
	//called from the event handler
	TBool RemoveProcess(TAny* a1, TAny* a2);
	TBool StartThread(TAny* a1, TAny* a2);
	TBool AddLibrary(TAny* a1, TAny* a2);
	TBool RemoveLibrary(TAny* a1, TAny* a2);
	TBool HandleEventKillThread(TAny* a1, TAny* a2);
	TBool HandleSwException(TAny* a1, TAny* a2);
	TBool HandleHwException(TAny* a1, TAny* a2);
	TBool HandleUserTrace(TAny* a1, TAny* a2);
	TBool HandleUnsupportedEvent(TAny* a1, TAny* a2) { return EFalse; }
	TBool HandleAddProcessEvent(TAny* a1, TAny* a2);
	TBool HandleRemoveProcessEvent(TAny* a1, TAny* a2);
	
	// Used to be able to signal events to the DSS
	DThread* ClientThread(void) {return iClientThread; };
	
protected:
	virtual void DoCancel(TInt aReqNo);
	virtual void DoRequest(TInt aReqNo, TRequestStatus* aStatus, TAny* a1, TAny* a2);
	virtual TInt DoControl(TInt aFunction, TAny *a1, TAny *a2);
	
private:
	TInt PreAsyncGetValue(Debug::TEventInfo* aValue, TRequestStatus* aStatus);
	TInt CreateDfcQ();
	void DestroyDfcQ();
	TBool HandleInvalidOpCodeException(TDriverEventInfo& aEventInfo, DThread* aCurrentThread);

	TInt SetBreak(TSetBreakInfo* aBreakInfo);
	TInt StepRange(DThread* aThread, TRM_DebugStepInfo* aStepInfo);
	TInt ReadMemory(DThread* aThread, TRM_DebugMemoryInfo* aMemoryInfo);
	TInt WriteMemory(DThread* aThread, TRM_DebugMemoryInfo* aMemoryInfo);
	TInt ReadRegistersLegacy(DThread* aThread, TRM_DebugRegisterInfo* aRegisterInfo);
	TInt WriteRegistersLegacy(DThread* aThread, const TRM_DebugRegisterInfo* aRegisterInfo);
	TInt ReadRegisters(DThread* aThread, TRM_DebugRegisterInformation* aRegisterInfo) const;
	TInt WriteRegisters(DThread* aThread, TRM_DebugRegisterInformation* aRegisterInfo) const;
	TInt GetProcessInfo(TInt aIndex, TRM_DebugTaskInfo* aTaskInfo);
	TInt GetThreadInfo(TInt aIndex, TRM_DebugTaskInfo* aTaskInfo);
	TInt GetList(TListInformation* aListInformation) const;
	
	TInt Step(const TUint32 aThreadId, const TUint32 aNumSteps);
	TInt KillProcess(const TUint32 aProcessId, const TInt aReason);

	//Crash Flash	
	TInt ReadCrashLog(TFlashInfo* aBuffer);
	TInt WriteCrashLog(TFlashInfo* aBuffer) const;
	TInt EraseCrashLog();

	// Stop/go
	TInt DoSuspendThread(DThread *aThread);
	TInt DoResumeThread(DThread *aThread);
	TInt DoStepRange(DThread *aThread, const TUint32 aStartAddress, const TUint32 aStopAddress, TBool aStepInto, TBool aResumeOnceOutOfRange, const TUint32 aNumSteps, TBool aUserRequest = EFalse);
	TInt DoReadMemory(const DThread *aThread, const TUint32 aAddress, const TUint32 aLength, TDes8 &aData) const;
	TInt DoWriteMemory(DThread *aThread, const TUint32 aAddress, const TUint32 aLength, TDes8 &aData);
	TInt DoReadRegisters(DThread *aThread, const TInt16 aFirstRegister, const TInt16 aLastRegister, TDes8 &aValues);
	TInt DoReadRegisters(DThread *aThread, const TDesC8 &aRegisterIds, TDes8 &aRegisterValues, TDes8 &aRegisterFlags) const;
	TInt DoWriteRegisters(DThread *aThread, const TInt16 aFirstRegister, const TInt16 aLastRegister, TDesC8 &aValues);
	TInt DoWriteRegisters(DThread *aThread, const TDesC8 &aRegisterIds, TDesC8 &aRegisterValues, TDes8 &aRegisterFlags) const;
	TInt DoGetProcessInfo(const TInt aIndex, TRM_DebugTaskInfo *aInfo);
	TInt DoGetThreadInfo(const TInt aIndex, TRM_DebugTaskInfo *aInfo);
	TBool DoSecurityCheck();

	TInt TryToReadMemory(const DThread *aThread, const TAny *aSrc, TAny *aDest, const TUint32 aLength) const;
	TInt TryToWriteMemory(const DThread *aThread, TAny *aDest, const TAny *aSrc, const TUint32 aLength);
	TInt32 ReadRegister(DThread *aThread, TInt aNum);
	TInt32 ReadDebugRegisterValue(DThread *aThread, const Debug::TRegisterInfo aDebugRegisterId, T4ByteRegisterValue &aValue) const;
	TInt32 ReadKernelRegisterValue(DThread *aThread, const TArmReg aKernelRegisterId, T4ByteRegisterValue &aValue) const;
	
	void NotifyEvent(const TDriverEventInfo& aEventInfo);

	TInt GetTRegisterInfo(const TDesC8 &aRegisterIds, const TUint aIndex, Debug::TRegisterInfo &aValue) const;
	TInt GetDebugRegisterId(const TArmReg aKernelRegister, Debug::TRegisterInfo& aDebugRegister) const;
	TInt GetKernelRegisterId(const Debug::TRegisterInfo aDebugRegister, TArmReg& aKernelRegister) const;
	TBool GetFlagAtOffset(const TUint32 aFlags, const TArmReg aIndex) const;

	TInt AllocAndReadDes(DThread *aThread, const TDesC8& aSrcDes, TPtr8& aDestDes, const TBool aReadFromClient=ETrue, const TUint aOffset=0) const;

	TInt AttachProcess(TAny* a1, TAny* a2);
	TInt DetachProcess(TAny* a1, TAny* a2);
	TInt DetachAgent(TAny* a1, TAny* a2);
	TInt SetEventAction(TAny* a1, TAny* a2);
	TBool CheckSuspended(const DThread *aThread) const;

	// Needed so moved functions can access iBreakpoint list and related functions
	friend class D_RMD_Breakpoints;
	// Needed so moved functions can access stepping functionality
	friend class DRMDStepping;

	// helper function was previously in rm_debug_kerneldriver.cpp
	inline TInt Bitcount(TUint32 val)
		{
			TInt nbits;

			for (nbits = 0; val != 0; nbits++)
			{
				val &= val - 1;		// delete rightmost 1-bit in val
			}
			
			return nbits;
		}

	// Security critical - this returns whether the specified process is debuggable or not
	TInt IsDebuggable(const TUint32 aProcessId);

private:
	DThread* iClientThread;
	DRM_DebugEventHandler* iEventHandler;
	
	TUint32 iExcludedROMAddressStart;
	TUint32 iExcludedROMAddressEnd;
	
	TUint32 iPageSize;
	
	RArray<Debug::TProcessInfo> iDebugProcessList; //processes that we are debugging

	D_RMD_Breakpoints* iBreakManager;	// new D_RMD_Breakpoints

	DRMDStepping* iStepper;				// new DRMDStepping

	DSemaphore* iStepLock;				// Synchronisation for stepping code.

	TDynamicDfcQue* iDfcQ;

	TBool	iInitialisedCodeModifier;	// Ensures we control its lifetime

	TClientDataRequest<Debug::TEventInfo>* iAsyncGetValueRequest;
};

#endif //__RM_DEBUG_DRIVER_H__
