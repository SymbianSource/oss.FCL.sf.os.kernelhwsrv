// Copyright (c) 2004-2010 Nokia Corporation and/or its subsidiary(-ies).
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
// Refactored class containing breakpoint related code from rm_debug_kerneldriver.cpp
//



/**
 @file
 @internalComponent
 @released
*/

#ifndef D_RMD_BREAKPOINTS_H
#define D_RMD_BREAKPOINTS_H

#include <rm_debug_api.h>
#include <kernel/kern_priv.h>
#include "rm_debug_kerneldriver.h"

// fwd declaration of friend classes needed due to re-factoring
class DRM_DebugChannel;

class DRMDStepper;

//
// Macros
//
const TUint32 KArmBreakPoint = 0xE7F123F4;
const TUint16 KThumbBreakPoint = 0xDE56;
const TUint16 KT2EEBreakPoint = 0xC100;	// From ARM ARM DDI0406A, section A9.2.1 Undefined instruction encoding for Thumb2-EE.

#define NUMBER_OF_TEMP_BREAKPOINTS 10

#define NUMBER_OF_MAX_BREAKPOINTS 100

//
// class TBreakEntry
//
class TBreakEntry
{
public:

	inline TBreakEntry() { Reset(); };

	inline TBreakEntry(Debug::TBreakId aBreakId, TUint64 aId, TBool aThreadSpecific, TUint32 aAddress, Debug::TArchitectureMode aMode)
			: iBreakId(aBreakId),
			  iId(aId),
			  iAddress(aAddress),
			  iMode(aMode),
			  iThreadSpecific(aThreadSpecific)
	{
		 iInstruction.FillZ(4);
		 iPageAddress = 0;
		 iDisabledForStep = EFalse;
		 iObsoleteLibraryBreakpoint = EFalse;
		 iResumeOnceOutOfRange = EFalse;
		 iSteppingInto = EFalse;
		 iRangeStart = 0;
		 iRangeEnd = 0;
		 iStepTarget = EFalse;
		 iNumSteps = 0;
	};
	
	inline void Reset()
	{
		 iId = 0;
		 iAddress = 0;
		 iMode = Debug::EArmMode;
		 iInstruction.FillZ(4);
		 iPageAddress = 0;
		 iDisabledForStep = EFalse;
		 iObsoleteLibraryBreakpoint = EFalse;
		 iResumeOnceOutOfRange = EFalse;
		 iSteppingInto = EFalse;
		 iRangeStart = 0;
		 iRangeEnd = 0;
		 iStepTarget = EFalse;
		 iNumSteps = 0;
	};

public:
	// Unique Id for this breakpoint. Assigned by D_RMD_Breakpoints::DoSetBreak(). @see D_RMD_Breakpoints::DoSetBreak
	TInt32		iBreakId;
	// Consider making the iId into a union of TProcessId, TThreadId, global etc. to make things more obvious
	// Object Id in which this breakpoint should operate.
	TUint64		iId;
	// Address at which this breakpoint should operate
	TUint32		iAddress;
	// CPU ISA which this breakpoint uses, e.g. EArmMode/EThumbMode.
	Debug::TArchitectureMode iMode;
	// The original instruction which was stored at iAddress.
	TBuf8<4>	iInstruction;
	TUint32		iPageAddress;   //not used: BC if we remove it

	// Indicates whether this breakpoint has been temporarily replaced with original instruction to enable step-off this breakpoint
	TBool		iDisabledForStep;
	/* This is used when libraries and processes are removed, so that
	 * the driver can say 'ok' when requested to remove breakpoints
	 * that existed in these cases, rather than 'Not Found'.
	 *
	 * Its not logical, but its a BC break if we change it :-(
	 */
	TBool		iObsoleteLibraryBreakpoint;
	// Indicates whether this thread should be resumed after stepping off this breakpoint
	TBool		iResumeOnceOutOfRange;
	TBool		iSteppingInto;
	TUint32		iRangeStart;
	TUint32		iRangeEnd;
	TBool		iThreadSpecific;
	TBool		iStepTarget;

	// Indicates how many more instruction steps should occur after hitting this breakpoint
	TInt		iNumSteps;
};
/**
@internalTechnology

This class encapsulates all the data concerning run-mode and stop mode breakpoints
as understood by the run-mode and stop-mode debug system.

Note:                                                                        
	The internal list of breakpoints is currently divided into two sections. The range from
	0...NUMBER_OF_TEMP_BREAKPOINTS is used internally by the debug driver for implementing
	stepping. The range from NUMBER_OF_TEMP_BREAKPOINTS to NUMBER_OF_MAX_BREAKPOINTS is used
	to store information about breakpoints set by the client debug agents.
                                                                                                                                                            
	In future, this should change, so that each breakpoint knows what kind of breakpoint it
	is (user/temp etc).


*/
class D_RMD_Breakpoints : public DBase
{
public:
	D_RMD_Breakpoints(DRM_DebugChannel* aChannel);
	~D_RMD_Breakpoints();

	TInt Init();

	// from rm_debug_driver.h
	TInt DoSetBreak(TInt32 &aBreakId, const TUint64 aId, const TBool aThreadSpecific, const TUint32 aAddress, const Debug::TArchitectureMode aMode );
	TInt DoEnableBreak(TBreakEntry &aEntry, TBool aSaveOldInstruction);
	TInt DoClearBreak(const TInt32 aBreakId, TBool aIgnoreTerminatedThreads=EFalse);
	TInt DoModifyBreak(TModifyBreakInfo* aBreakInfo);
	TInt DoModifyProcessBreak(TModifyProcessBreakInfo* aBreakInfo);
	TInt DoBreakInfo(TGetBreakInfo* aBreakInfo);
	void ClearAllBreakPoints();
	TInt DisableBreakAtAddress(TUint32 aAddress);
	TInt DoEnableDisabledBreak(TUint64 aThreadId);

	void DoRemoveThreadBreaks(TUint64 aThreadId);
	void RemoveBreaksForProcess(TUint64 aProcessId, TUint32 aCodeAddress, TUint32 aCodeSize);
	void InvalidateLibraryBreakPoints(TUint32 aCodeAddress, TUint32 aCodeSize);
	TInt BreakPointCount() const;
	TBreakEntry* GetNextBreak(const TBreakEntry* aBreakEntry) const;
	TBool IsTemporaryBreak(const TBreakEntry& aBreakEntry) const;

	TInt DoGetBreakList(TUint32* aBuffer, const TUint32 aBufSize, const TUint32 aElement, TUint32& aLastElement);

	// Useful helper functions for debugging breakpoint issues
	inline void print_BreakpointsDisabledForStep();
	inline void print_BreakpointsList();

private:
	// Locked versions of public functions
	TInt priv_DoSetBreak(TInt32 &aBreakId, const TUint64 aId,  const TBool aThreadSpecific, const TUint32 aAddress, const Debug::TArchitectureMode aMode );
	TInt priv_DoEnableBreak(TBreakEntry &aEntry, TBool aSaveOldInstruction);
	TInt priv_DoClearBreak(const TInt32 aBreakId, TBool aIgnoreTerminatedThreads);
	TInt priv_DoModifyBreak(TModifyBreakInfo* aBreakInfo);
	TInt priv_DoModifyProcessBreak(TModifyProcessBreakInfo* aBreakInfo);
	TInt priv_DoBreakInfo(TGetBreakInfo* aBreakInfo);	
	TInt priv_DisableBreakAtAddress(TUint32 aAddress);
	TInt priv_DoEnableDisabledBreak(TUint64 aThreadId);
	void priv_DoRemoveThreadBreaks(TUint64 aThreadId);
	void priv_ClearAllBreakPoints();
	TBool priv_IsTemporaryBreak(const TBreakEntry& aBreakEntry) const;

	// helper functions
	TBool Aligned(TUint32 aAddress, Debug::TArchitectureMode aMode);
	TInt BreakSize(Debug::TArchitectureMode aMode);
	TBool BreakpointsOverlap(TBreakEntry& aFirst, TBreakEntry& aSecond);
	TUint32 BreakInst(Debug::TArchitectureMode aMode);

private:
	RArray<TBreakEntry> iBreakPointList;
	TInt iNextBreakId;

	DRM_DebugChannel* iChannel;	// temporary reference back to DRM_DebugChannel to help with refactoring

	/* Protect access to the breakpoint list with a DSemaphore
	 *
	 * This means that stop-mode debuggers know when the list is being updated by the run-mode debug subsystem.
	 */
	DSemaphore* iLock;

	TBool iInitialised;
};

#include "d_rmd_breakpoints_debug.inl" 

#endif
