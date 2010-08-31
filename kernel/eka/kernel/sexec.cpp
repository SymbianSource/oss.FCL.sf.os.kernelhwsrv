// Copyright (c) 1994-2009 Nokia Corporation and/or its subsidiary(-ies).
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
// e32\kernel\sexec.cpp
// 
//

#include <kernel/kern_priv.h>
#include <e32uid.h>
#include <e32ver.h>
//#include <unicode.h>
#include "execs.h"

TInt ExecHandler::ObjectNext(TObjectType aType, TBuf8<KMaxFullName>& aName, TFindHandle& aFindHandle)
//
// Do the next find.
//
	{

	__KTRACE_OPT(KEXEC,Kern::Printf("Exec::ObjectNext type %d",aType));
	if (aType<0 || aType>=ENumObjectTypes)
		K::PanicKernExec(EBadObjectType);
	DObjectCon* pC=K::Containers[aType];
	TFullName fn;
	TFullName match;
	TFindHandle h;
	Kern::KUDesGet(match,aName);
	kumemget32(&h,&aFindHandle,sizeof(h));
	__KTRACE_OPT(KEXEC,Kern::Printf("ObjN: %S %08x", &match, h.Handle()));
	NKern::ThreadEnterCS();
	TInt r=pC->FindByFullName(h, match, fn);
	NKern::ThreadLeaveCS();
	Kern::KUDesPut(aName,fn);
	kumemput32(&aFindHandle,&h,sizeof(h));
	__KTRACE_OPT(KEXEC,Kern::Printf("Exec::ObjectNext ret %d",r));
	return r;
	}

TUint8 *ExecHandler::ChunkBase(DChunk* aChunk)
//
// Return the address of the base of the Chunk.
//
	{

	__KTRACE_OPT(KEXEC,Kern::Printf("Exec::ChunkBase"));
	return (TUint8*)aChunk->Base(&Kern::CurrentProcess());
	}

TInt ExecHandler::ChunkSize(DChunk* aChunk)
//
// Return the current size of the Chunk.
//
	{

	__KTRACE_OPT(KEXEC,Kern::Printf("Exec::ChunkSize"));
	return aChunk->Size();
	}

TInt ExecHandler::ChunkMaxSize(DChunk* aChunk)
//
// Return the maximum size of the Chunk.
//
	{

	__KTRACE_OPT(KEXEC,Kern::Printf("Exec::ChunkMaxSize")); 	
	return aChunk->MaxSize();
	}

TInt ExecHandler::ChunkBottom(DChunk* aChunk)
//
// Return the position of the bottom of the chunk
//
	{

	__KTRACE_OPT(KEXEC,Kern::Printf("Exec::ChunkBottom"));
	return aChunk->Bottom();
	}

TInt ExecHandler::ChunkTop(DChunk* aChunk)
//
// Return the position of the top of the chunk
//
	{

	__KTRACE_OPT(KEXEC,Kern::Printf("Exec::ChunkTop"));
	return aChunk->Top();
	}

TInt ExecHandler::MutexWait(DMutex* aMutex, TInt aTimeout)
//
// Wait for the mutex.
//
	{
	if (aTimeout)	// 0 means wait forever, -1 means poll
		{
		if (aTimeout<-1)
			{
			return KErrArgument;
			}
		if (aTimeout>0)
			{
			// Convert microseconds to NTimer ticks, rounding up
			TInt ntp = NKern::TickPeriod();
			aTimeout += ntp-1;
			aTimeout /= ntp;
			}
		}
	return aMutex->Wait(aTimeout);
	}

void ExecHandler::MutexSignal(DMutex* aMutex)
//
// Signal the mutex.
//
	{

//	__KTRACE_OPT(KEXEC,Kern::Printf("Exec::MutexSignal"));
	if (TheCurrentThread==aMutex->iCleanup.iThread)
		{
		aMutex->Signal();
		return;
		}
	K::PanicCurrentThread(EAccessDenied);
	}

/**
Test if mutex is held by the current thread.
@return True if the current thread has waited on the mutex, false otherwise.
*/
TBool ExecHandler::MutexIsHeld(DMutex* aMutex)
	{
	__KTRACE_OPT(KEXEC,Kern::Printf("Exec::MutexIsHeld"));
	return TheCurrentThread==aMutex->iCleanup.iThread;
	}

void ExecHandler::ProcessType(DProcess* aProcess, TUidType& aUids)
//
// Get process' UIDs.
//
	{

	__KTRACE_OPT(KEXEC,Kern::Printf("Exec::ProcessType"));
	TUidType uids(aProcess->iUids);
	NKern::UnlockSystem();
	kumemput32(&aUids,&uids,sizeof(TUidType));
	}

TInt ExecHandler::ProcessId(DProcess* aProcess)
//
// Get process ID.
//
	{

	__KTRACE_OPT(KEXEC,Kern::Printf("Exec::ProcessId")); 	
	return (TInt)aProcess->iId;
	}

void ExecHandler::ProcessSecurityInfo(DProcess* aProcess,SSecurityInfo& aInfo)
//
// Get process security info.
//
	{
	__KTRACE_OPT(KEXEC,Kern::Printf("Exec::ProcessSecurityInfo"));
	SSecurityInfo info(aProcess->iS);
	NKern::UnlockSystem();
	kumemput32(&aInfo,&info,sizeof(info));
	}

void ExecHandler::ThreadSecurityInfo(DThread* aThread,SSecurityInfo& aInfo)
//
// Get threads security info.
//
	{
	__KTRACE_OPT(KEXEC,Kern::Printf("Exec::ThreadSecurityInfo"));
	SSecurityInfo info(aThread->iOwningProcess->iS);
	NKern::UnlockSystem();
	kumemput32(&aInfo,&info,sizeof(info));
	}

void ExecHandler::MessageSecurityInfo(DThread* aClient,SSecurityInfo& aInfo)
//
// Get clients security info.
//
	{
	__KTRACE_OPT(KEXEC,Kern::Printf("Exec::MessageSecurityInfo"));
	SSecurityInfo info(aClient->iOwningProcess->iS);
	NKern::UnlockSystem();
	kumemput32(&aInfo,&info,sizeof(info));
	}

TInt ExecHandler::SessionSecurityInfo(TInt aSession,SSecurityInfo& aInfo)
//
// Get session security info.
//
	{
	__KTRACE_OPT(KEXEC,Kern::Printf("Exec::SessionSecurityInfo"));
	TInt r = KErrBadHandle;
	SSecurityInfo info;
	NKern::LockSystem();
	DSession* s = (DSession*)TheCurrentThread->ObjectFromHandle(aSession,ESession);
	if (s)
		{
		if (s->iServer && s->iServer->iOwningThread)
			{
			// session is connected, and server is alive
			info = s->iServer->iOwningThread->iOwningProcess->iS;
			r = KErrNone;
			}
		else
			{
			r = KErrServerTerminated;
			}
		}
	NKern::UnlockSystem();
	if (r==KErrNone)
		kumemput32(&aInfo,&info,sizeof(info));
	return r;
	}

void ExecHandler::CreatorSecurityInfo(SSecurityInfo& aInfo)
//
// Get creator's security info.
//
	{
	__KTRACE_OPT(KEXEC,Kern::Printf("Exec::CreatorSecurityInfo"));
	kumemput32(&aInfo,&TheCurrentThread->iOwningProcess->iCreatorInfo,sizeof(aInfo));
	}

void ExecHandler::DisabledCapabilities(SCapabilitySet& aCaps)
//
// Get the set of capabilities which are not to be checked (implemented by effectively
// setting them for all executables).
//
	{
	__KTRACE_OPT(KEXEC, Kern::Printf("Exec::DisabledCapabilities"));
#ifdef __PLATSEC_UNLOCKED__
	kumemput32(&aCaps, &TheSuperPage().iDisabledCapabilities, sizeof(aCaps));
#else
	kumemset(&aCaps, 0, sizeof(aCaps));
#endif	// __PLATSEC_UNLOCKED__
	}

void ExecHandler::ProcessResume(DProcess* aProcess)
//
// Resume the first thread in the process.
//
	{

	__KTRACE_OPT(KEXEC,Kern::Printf("Exec::ProcessResume"));
	if (aProcess->iCreatorId!=TheCurrentThread->iOwningProcess->iId) // Not creator...
		K::ProcessIsolationFailure(__PLATSEC_DIAGNOSTIC_STRING("Checked by RProcess::Resume"));
	aProcess->Resume();
	}

void ExecHandler::ProcessFileName(DProcess* aProcess, TDes8& aName)
//
// Return the process file name.
//
	{

	__KTRACE_OPT(KEXEC,Kern::Printf("Exec::ProcessFileName"));
	TFileName fn;
	DCodeSeg* pS=aProcess->iCodeSeg;
	if (pS)
		pS->AppendFullFileName(fn);
	NKern::UnlockSystem();
	if (pS)
		P::NormalizeExecutableFileName(fn);
	Kern::KUDesPut(aName, fn);
	}

TInt ExecHandler::ProcessCommandLineLength(DProcess* aProcess)
//
// Return the process command line length.
//
	{

	__KTRACE_OPT(KEXEC,Kern::Printf("Exec::ProcessCommandLineLength"));

	if (aProcess->iSecurityZone!=TheCurrentThread->iOwningProcess->iSecurityZone)
		K::ProcessIsolationFailure(__PLATSEC_DIAGNOSTIC_STRING("Checked by RProcess::CommandLineLength"));
	const TDesC* pC=aProcess->iCommandLine;
#ifdef _UNICODE
	return pC?(pC->Length()>>1):0;
#else
	return pC?(pC->Length()):0;
#endif
	}

void ExecHandler::ProcessCommandLine(DProcess* aProcess, TDes8& aCommandLine)
//
// Return the process command line.
//
	{

	__KTRACE_OPT(KEXEC,Kern::Printf("Exec::ProcessCommandLine"));
	if (aProcess->iSecurityZone!=TheCurrentThread->iOwningProcess->iSecurityZone)
		K::ProcessIsolationFailure(__PLATSEC_DIAGNOSTIC_STRING("Checked by RProcess::CommandLine"));
	aProcess->CheckedOpen();
	DThread& t=*TheCurrentThread;
	t.iTempObj=aProcess;
	NKern::UnlockSystem();
	const TDesC* pC=aProcess->iCommandLine;
	if (!pC)
		pC=&KNullDesC;
	Kern::KUDesPut(aCommandLine,*pC);
	NKern::ThreadEnterCS();
	t.iTempObj=NULL;
	aProcess->Close(NULL);
	NKern::ThreadLeaveCS();
	}

TExitType ExecHandler::ProcessExitType(DProcess* aProcess)
//
// Return the exit type.
//
	{

	__KTRACE_OPT(KEXEC,Kern::Printf("Exec::ProcessExitType")); 	
	return (TExitType)aProcess->iExitType;
	}

TInt ExecHandler::ProcessExitReason(DProcess* aProcess)
//
// Return the exit reason.
//
	{

	__KTRACE_OPT(KEXEC,Kern::Printf("Exec::ProcessExitReason"));
	return aProcess->iExitReason;
	}

void ExecHandler::ProcessExitCategory(DProcess* aProcess, TDes8& aName)
//
// Return the category of the exit type.
//
	{

	__KTRACE_OPT(KEXEC,Kern::Printf("Exec::ProcessExitCategory"));
	TBufC<KMaxExitCategoryName> exitCat(aProcess->iExitCategory);
	NKern::UnlockSystem();
	Kern::KUDesPut(aName,exitCat);
	}

LOCAL_D const TProcessPriority procPriorityConvertTable[8]=
	{
	EPriorityLow,			EPriorityBackground,	EPriorityForeground,	EPriorityHigh,
	EPriorityWindowServer,	EPriorityFileServer,	EPrioritySupervisor,	EPriorityRealTimeServer
	};

LOCAL_D const TThreadPriority thrdPriorityConvertTable[8]=
	{
	EPriorityMuchLess,	EPriorityMuchLess,	EPriorityLess,		EPriorityNormal,
	EPriorityMore,		EPriorityMuchMore,	EPriorityRealTime,	EPriorityRealTime
	};

LOCAL_C TThreadPriority ConvertThreadPriority(TInt p)
	{
	switch(p)
		{
		case EThrdPriorityAbsoluteVeryLow:
			return EPriorityAbsoluteVeryLow;
		case EThrdPriorityAbsoluteLowNormal:
			return EPriorityAbsoluteLowNormal;
		case EThrdPriorityAbsoluteLow:
			return EPriorityAbsoluteLow;
		case EThrdPriorityAbsoluteBackgroundNormal:
			return EPriorityAbsoluteBackgroundNormal;
		case EThrdPriorityAbsoluteBackground:
			return EPriorityAbsoluteBackground;
		case EThrdPriorityAbsoluteForegroundNormal:
			return EPriorityAbsoluteForegroundNormal;
		case EThrdPriorityAbsoluteForeground:
			return EPriorityAbsoluteForeground;
		case EThrdPriorityAbsoluteHighNormal:
			return EPriorityAbsoluteHighNormal;
		case EThrdPriorityAbsoluteHigh:
			return EPriorityAbsoluteHigh;
		case EThrdPriorityAbsoluteRealTime1:
			return EPriorityAbsoluteRealTime1;
		case EThrdPriorityAbsoluteRealTime2:
			return EPriorityAbsoluteRealTime2;
		case EThrdPriorityAbsoluteRealTime3:
			return EPriorityAbsoluteRealTime3;
		case EThrdPriorityAbsoluteRealTime4:
			return EPriorityAbsoluteRealTime4;
		case EThrdPriorityAbsoluteRealTime5:
			return EPriorityAbsoluteRealTime5;
		case EThrdPriorityAbsoluteRealTime6:
			return EPriorityAbsoluteRealTime6;
		case EThrdPriorityAbsoluteRealTime7:
			return EPriorityAbsoluteRealTime7;
		case EThrdPriorityAbsoluteRealTime8:
			return EPriorityAbsoluteRealTime8;
		default:
			if (p>=-8 && p<0)
				return thrdPriorityConvertTable[p+8];
		}
	return EPriorityNormal;
	}

LOCAL_C TBool ProcessPriorityValid(TProcessPriority p)
	{
	switch(p)
		{
		case EPriorityLow:
		case EPriorityBackground:
		case EPriorityForeground:
		case EPriorityHigh:
			return ETrue;
		default:
			return EFalse;
		}
	}

TProcessPriority ExecHandler::ProcessPriority(DProcess* aProcess)
//
// Get the process base priority.
//
	{

	__KTRACE_OPT(KEXEC,Kern::Printf("Exec::ProcessPriority"));
	return procPriorityConvertTable[aProcess->iPriority];
	}

TInt ExecHandler::ProcessSetPriority(DProcess* aProcess, TProcessPriority aPriority)
//
// Set the process base priority.
//
	{

	__KTRACE_OPT(KEXEC,Kern::Printf("Exec::ProcessSetPriority"));
	aProcess->CheckedOpen();
	DThread& t=*TheCurrentThread;
	t.iTempObj=aProcess;
	NKern::UnlockSystem();
	if (!ProcessPriorityValid(aPriority))
		K::PanicKernExec(EBadPriority);

	TBool allowed=ETrue;
	DProcess* currentProcess=TheCurrentThread->iOwningProcess;
	if (aProcess->iSecurityZone!=currentProcess->iSecurityZone)  // Not self...
		if (aProcess->iCreatorId!=currentProcess->iId)  // Not creator
			{
			TInt processPriority=aProcess->iPriority;
			if (!(aProcess->iFlags&KProcessFlagPriorityControl)  // No remote control...
 				|| (processPriority!=EProcPriorityBackground && processPriority!=EProcPriorityForeground && processPriority!=EProcPriorityHigh)
				|| (aPriority!=EPriorityBackground && aPriority!=EPriorityForeground && aPriority!=EPriorityHigh) )  // Or not foreground/background/high
					allowed=!(TheSuperPage().KernelConfigFlags() & EKernelConfigPlatSecProcessIsolation);
			}

	NKern::ThreadEnterCS();
	t.iTempObj=NULL;
	if(allowed)
		aProcess->SetPriority(aPriority);
	aProcess->Close(NULL);
	NKern::ThreadLeaveCS();
	if (allowed)
		{
		return KErrNone;
		}
	else
		{
		return KErrPermissionDenied;
		}

	}



const TUint32 KDefinedProcessFlags=
	KProcessFlagSystemCritical|
	KProcessFlagSystemPermanent|
	KProcessFlagJustInTime|
	KProcessFlagPriorityControl|
	KThreadFlagProcessCritical;

const TUint32 KRestrictedProcessFlags =
		~(KProcessFlagJustInTime|KProcessFlagPriorityControl|KThreadFlagProcessCritical); // All but these are restricted

const TUint32 KSelfOnlyProcessFlags =
		~0u; // (KProcessFlagJustInTime|KProcessFlagPriorityControl); // Only self can change these

const TUint32 KDefinedThreadFlags=
	KThreadFlagProcessCritical|
	KThreadFlagProcessPermanent|
	KThreadFlagSystemCritical|
	KThreadFlagSystemPermanent|
	KThreadFlagLastChance|
	KThreadFlagRealtime|
	KThreadFlagRealtimeTest;

const TUint32 KRestrictedThreadFlags =
		~(KThreadFlagProcessCritical|KThreadFlagProcessPermanent|KThreadFlagLastChance|
			KThreadFlagRealtime|KThreadFlagRealtimeTest); // All but these are restricted

const TUint32 KSelfOnlyThreadFlags =
		~0u; // (KThreadFlagProcessCritical|KThreadFlagProcessPermanent|KThreadFlagLastChance); // Only owning process can change these


TUint ExecHandler::ProcessFlags(DProcess* aProcess)
//
// Get the process flags
//
	{

	__KTRACE_OPT(KEXEC,Kern::Printf("Exec::ProcessFlags"));
	return aProcess->iFlags;
	}

TUint ExecHandler::ThreadProcessFlags(DThread* aThread)
//
// Get the process flags
//
	{

	__KTRACE_OPT(KEXEC,Kern::Printf("Exec::ThreadProcessFlags"));
	return aThread->iOwningProcess->iFlags;
	}

void ExecHandler::ProcessSetFlags(DProcess* aProcess, TUint aClearMask, TUint aSetMask)
//
// Set the process flags
//
	{
	__KTRACE_OPT(KEXEC,Kern::Printf("Exec::ProcessSetFlags"));

	TUint flags = aProcess->iFlags;
	TUint modified = ((flags&~aClearMask)|aSetMask);
	modified = (modified^flags)&KDefinedProcessFlags;

	DProcess* currentProcess=TheCurrentThread->iOwningProcess;

	if (modified&KSelfOnlyProcessFlags)
		if(aProcess->iSecurityZone!=currentProcess->iSecurityZone)
			if(aProcess->iCreatorId!=currentProcess->iId)
				K::ProcessIsolationFailure(__PLATSEC_DIAGNOSTIC_STRING("Attempt to modify the attributes of another Process. Possibly RProcess::SetSystem."));

	if (modified&KRestrictedProcessFlags)
		if(!currentProcess->HasCapability(ECapabilityProtServ,__PLATSEC_DIAGNOSTIC_STRING("Checked by User::SetProcessCritical (or RProcess::SetSystem)")))
			K::LockedPlatformSecurityPanic();

	aProcess->iFlags=flags^modified;

	// if flags altered before resume, original thread inherits system critical and
	// process critical from process.
	if (!(aProcess->iAttributes & DProcess::EResumed))
		{
		TUint32& tf = aProcess->FirstThread()->iFlags;
		tf = (tf &~ (KThreadFlagSystemCritical|KThreadFlagProcessCritical)) |
			(aProcess->iFlags & (KThreadFlagSystemCritical|KThreadFlagProcessCritical));
		}
	}

TInt ExecHandler::SemaphoreWait(DSemaphore* aSemaphore, TInt aTimeout)
//
// Wait for a signal.
//
	{
	__KTRACE_OPT(KEXEC,Kern::Printf("Exec::SemaphoreWait"));
	if (aTimeout)	// 0 means wait forever, -1 means poll
		{
		if (aTimeout<-1)
			{
			NKern::UnlockSystem();
			return KErrArgument;
			}
		if (aTimeout>0)
			{
			// Convert microseconds to NTimer ticks, rounding up
			TInt ntp = NKern::TickPeriod();
			aTimeout += ntp-1;
			aTimeout /= ntp;
			}
		}
	return aSemaphore->Wait(aTimeout);
	}

void ExecHandler::SemaphoreSignal1(DSemaphore* aSemaphore)
//
// Signal the semaphore once.
//
	{

	__KTRACE_OPT(KEXEC,Kern::Printf("Exec::SemaphoreSignal1"));
	aSemaphore->Signal();
	}

void ExecHandler::SemaphoreSignalN(DSemaphore* aSem, TInt aCount)
//
// Signal the semaphore aCount times.
//
	{

	__KTRACE_OPT(KEXEC,Kern::Printf("Exec::SemaphoreSignalN"));
	aSem->CheckedOpen();
	NKern::ThreadEnterCS();
	aSem->SignalN(aCount);
	NKern::UnlockSystem();
	aSem->Close(NULL);
	NKern::ThreadLeaveCS();
	}

TInt ExecHandler::ThreadId(DThread* aThread)
//
// Get thread ID.
//
	{

	__KTRACE_OPT(KEXEC,Kern::Printf("Exec::ThreadId"));
	return (TInt)aThread->iId;
	}
	
LOCAL_C TBool IsThreadPriorityAbsoluteRealTime(TThreadPriority p)
//
// Returns true if priority is an absolute "real time" thread priority.
//
	{
	switch(p)
		{
		case EPriorityAbsoluteRealTime1:
		case EPriorityAbsoluteRealTime2:
		case EPriorityAbsoluteRealTime3:
		case EPriorityAbsoluteRealTime4:
		case EPriorityAbsoluteRealTime5:
		case EPriorityAbsoluteRealTime6:
		case EPriorityAbsoluteRealTime7:
		case EPriorityAbsoluteRealTime8:
			return ETrue;
		default:
			return EFalse;
		}
	}

void ExecHandler::ThreadResume(DThread* aThread)
//
// Resume a thread.
//
	{

	__KTRACE_OPT(KEXEC,Kern::Printf("Exec::ThreadResume"));
	if (aThread->iOwningProcess->iSecurityZone!=TheCurrentThread->iOwningProcess->iSecurityZone)
		K::ProcessIsolationFailure(__PLATSEC_DIAGNOSTIC_STRING("Use of RThread::Resume on a thread in another process"));
	aThread->Resume();
	}

TThreadPriority ExecHandler::ThreadPriority(DThread* aThread)
//
// Get the threads priority.
//
	{

	__KTRACE_OPT(KEXEC,Kern::Printf("Exec::ThreadPriority"));
	return ConvertThreadPriority(aThread->iThreadPriority);
	}

void ExecHandler::ThreadSetPriority(DThread* aThread,TThreadPriority aPriority)
//
// Set the threads priority.
//
	{
	__KTRACE_OPT(KEXEC,Kern::Printf("Exec::ThreadSetPriority"));
	if (aThread->iOwningProcess->iSecurityZone!=TheCurrentThread->iOwningProcess->iSecurityZone)
		K::ProcessIsolationFailure(__PLATSEC_DIAGNOSTIC_STRING("Use of RThread::SetPriority on a thread in a different process"));
	if(IsThreadPriorityAbsoluteRealTime(aPriority) && 
	   !(TheCurrentThread->HasCapability(ECapabilityProtServ,__PLATSEC_DIAGNOSTIC_STRING("Checked by RThread::SetPriority"))))
		K::LockedPlatformSecurityPanic();
	else
		aThread->SetPriority(aPriority);
	}

TProcessPriority ExecHandler::ThreadProcessPriority(DThread* aThread)
//
// Get the owning process's priority.
//
	{

	__KTRACE_OPT(KEXEC,Kern::Printf("Exec::ThreadProcessPriority"));
	return procPriorityConvertTable[aThread->iOwningProcess->iPriority];
	}

void ExecHandler::ThreadSetProcessPriority(DThread* aThread, TProcessPriority aPriority)
//
// Set the owning process's priority.
//
	{
	ExecHandler::ProcessSetPriority(aThread->iOwningProcess,aPriority);
	}

TUint ExecHandler::ThreadFlags(DThread* aThread)
//
// Get the threads flag state.
//
	{

	__KTRACE_OPT(KEXEC,Kern::Printf("Exec::ThreadFlags"));
	return(aThread->iFlags);
	}

void ExecHandler::ThreadSetFlags(DThread* aThread,TUint aClearMask,TUint aSetMask)
//
// Set the thread flags
//
	{
	__KTRACE_OPT(KEXEC,Kern::Printf("Exec::ThreadSetFlags"));
	TUint flags = aThread->iFlags;
	TUint modified = ((flags&~aClearMask)|aSetMask);
	modified = (modified^flags)&KDefinedThreadFlags;

	DProcess* currentProcess=TheCurrentThread->iOwningProcess;

	if (modified&KSelfOnlyThreadFlags)
		if(aThread->iOwningProcess->iSecurityZone!=currentProcess->iSecurityZone)
			K::ProcessIsolationFailure(__PLATSEC_DIAGNOSTIC_STRING("Attempt to modify attributes of a thread in another process. Possibly RThread::SetSystem"));

	if (modified&KRestrictedThreadFlags)
		if(!currentProcess->HasCapability(ECapabilityProtServ,__PLATSEC_DIAGNOSTIC_STRING("Checked by User::SetCritical (or RThread::SetSystem)")))
			K::LockedPlatformSecurityPanic();

	aThread->iFlags=flags^modified;
	}

TExitType ExecHandler::ThreadExitType(DThread* aThread)
//
// Return the exit type.
//
	{

	__KTRACE_OPT(KEXEC,Kern::Printf("Exec::ThreadExitType"));
	return (TExitType)aThread->iExitType;
	}

TInt ExecHandler::ThreadExitReason(DThread* aThread)
//
// Return the exit reason.
//
	{

	__KTRACE_OPT(KEXEC,Kern::Printf("Exec::ThreadExitReason"));
	return aThread->iExitReason;
	}

void ExecHandler::ThreadExitCategory(DThread* aThread, TDes8& aName)
//
// Return the category of the exit type.
//
	{

	__KTRACE_OPT(KEXEC,Kern::Printf("Exec::ThreadExitCategory"));
	TBufC<KMaxExitCategoryName> exitCat(aThread->iExitCategory);
	NKern::UnlockSystem();
	Kern::KUDesPut(aName,exitCat);
	}

void ExecHandler::ThreadRequestSignal(DThread* aThread)
//
// Signal a request completion.
// Enter with system locked, return with system unlocked.
//
	{

	__KTRACE_OPT(KEXEC,Kern::Printf("Exec::ThreadRequestSignal"));
	if(aThread->iOwningProcess!=TheCurrentThread->iOwningProcess)
		K::ProcessIsolationFailure(__PLATSEC_DIAGNOSTIC_STRING("Attempt to use RThread::RequestComplete or RThread::RequestSignal on a thread in another process"));
	NKern::ThreadRequestSignal(&aThread->iNThread, SYSTEM_LOCK);
	}

TInt ExecHandler::FindHandleOpen(TOwnerType aType, const TFindHandle& aFindHandle)
	{

	__KTRACE_OPT(KEXEC,Kern::Printf("Exec::FindHandleOpen"));
	TFindHandle fh;
	kumemget32(&fh,&aFindHandle,sizeof(fh));
	NKern::ThreadEnterCS();
	TInt h;
	TInt r=TheCurrentThread->OpenFindHandle(aType,fh,h);
	if(r==KErrNone)
		r = h;
	NKern::ThreadLeaveCS();
	if (r==KErrBadHandle)
		K::PanicKernExec(EBadHandle);
	__KTRACE_OPT(KEXEC,Kern::Printf("Exec::FindHandleOpen returns %d",r));
	return r;
	}

TInt ExecHandler::HandleClose(TInt aHandle)
	{
	__KTRACE_OPT(KEXEC,Kern::Printf("Exec::HandleClose %08x",aHandle));
	NKern::ThreadEnterCS();
	TInt r=K::HandleClose(aHandle);
	NKern::ThreadLeaveCS();
	if (r==KErrBadHandle)
		K::PanicKernExec(EBadHandle);
	else if (r==DObject::EObjectUnmapped)
		TheCurrentThread->iOwningProcess->WaitDllLock();
	return r;
	}

TInt ExecHandler::LastThreadHandle()
	{
	__KTRACE_OPT(KEXEC,Kern::Printf("Exec::LastThreadHandle"));
	DThread& t=*TheCurrentThread;
	NKern::ThreadEnterCS();
	RObjectIx::Wait();
	TInt r=t.iHandles.LastHandle();
	RObjectIx::Signal();
	NKern::ThreadLeaveCS();
	if (r)
		r|=KHandleFlagLocal;
	return r;
	}

TInt ExecHandler::ChunkCreate(TOwnerType aType, const TDesC8* aName, TChunkCreate& anInfo)
	{
	TKName n;
	if (aName)
		Kern::KUDesGet(n,*aName);
	__KTRACE_OPT(KEXEC,Kern::Printf("Exec::ChunkCreate %S",&n));
	TChunkCreate uinfo;
	SChunkCreateInfo info;
	kumemget32(&uinfo,&anInfo,sizeof(uinfo));
	info.iGlobal=uinfo.iAtt & TChunkCreate::EGlobal;
	info.iAtt = uinfo.iAtt&TChunkCreate::EChunkCreateAttMask;
	info.iForceFixed=uinfo.iForceFixed;
	info.iOperations=SChunkCreateInfo::EAdjust;	// adjust but don't add to process
	info.iRunAddress=0;
	info.iType=(uinfo.iAtt & TChunkCreate::ECode) ? EUserSelfModCode : EUserData;
	info.iMaxSize=uinfo.iMaxSize;
	info.iInitialBottom=uinfo.iInitialBottom;
	info.iInitialTop=uinfo.iInitialTop;
	info.iPreallocated=0;
	info.iClearByte = uinfo.iClearByte;
	DThread* pT=TheCurrentThread;
	DProcess* pP=pT->iOwningProcess;
	if (aName)
		info.iName.Set(n);
	else
		info.iName.Set(NULL,0);
	if (!info.iGlobal)
		info.iOwner=(aType==EOwnerThread)?(DObject*)pT:(DObject*)pP;
	else
		info.iOwner=NULL;
	NKern::ThreadEnterCS();
	DChunk* pC=NULL;
	TLinAddr addr;
	TInt r=pP->NewChunk(pC,info,addr);
	if (r==KErrNone)
		r=K::MakeHandle(aType,pC);	// this will add the chunk to the process
	if (r<KErrNone && pC)
		pC->Close(NULL);				// can't have been added so NULL
	NKern::ThreadLeaveCS();
	__KTRACE_OPT(KTHREAD,Kern::Printf("Exec::ChunkCreate returns %d",r));
	return r;
	}

TInt ExecHandler::ChunkSetRestrictions(DChunk* aChunk, TUint aRestrictions)
	{
	__KTRACE_OPT(KEXEC,Kern::Printf("Exec::ChunkSetRestrictions %O flags=%x",aChunk,aRestrictions));
	if(aChunk->iControllingOwner!=TheCurrentThread->iOwningProcess->iId)
		return KErrAccessDenied;
	aChunk->iRestrictions = aRestrictions;
	return KErrNone;
	}

TInt ExecHandler::ChunkAdjust(DChunk* aChunk, TInt aType, TInt a1, TInt a2)
	{
	__KTRACE_OPT(KPROC,Kern::Printf("Exec::ChunkAdjust %O type %d a1=%x a2=%x",aChunk,aType,a1,a2));
	aChunk->CheckedOpen();
	NKern::ThreadEnterCS();
	NKern::UnlockSystem();
	TInt r=KErrNone;
	TInt s=aChunk->CheckAccess();
	if (s!=KErrNone)
		goto done;
	if((aChunk->iRestrictions&EChunkPreventAdjust) && (aChunk->iControllingOwner!=TheCurrentThread->iOwningProcess->iId))
		{
		r=KErrAccessDenied;
		goto done;
		}
	switch (aType)
		{
		case EChunkAdjust:
			r=aChunk->Adjust(a1);
			break;
		case EChunkAdjustDoubleEnded:
			r=aChunk->AdjustDoubleEnded(a1,a2);
			break;
		case EChunkCommit:
			r=aChunk->Commit(a1,a2);
			break;
		case EChunkDecommit:
			r=aChunk->Decommit(a1,a2);
			break;
		case EChunkAllocate:
			r=aChunk->Allocate(a1);
			break;
		case EChunkLock:
			if(&Kern::CurrentProcess()!=aChunk->iOwningProcess)
				r = KErrAccessDenied;	
			else
				r=aChunk->Lock(a1,a2);
			break;
		case EChunkUnlock:
			if(&Kern::CurrentProcess()!=aChunk->iOwningProcess)
				r = KErrAccessDenied;	
			else
				r=aChunk->Unlock(a1,a2);
			break;
		default:
			r=KErrArgument;
			break;
		}
done:
	aChunk->Close(NULL);		// NULL because we didn't up the process access count
	NKern::ThreadLeaveCS();
	if (s!=KErrNone)
		K::PanicKernExec(EAccessDenied);
	__KTRACE_OPT(KPROC,Kern::Printf("Exec::ChunkAdjust returns %d",r));
	return r;
	}


/**
@return ETrue if the chunk is data paged, EFalse otherwise.
*/
TBool ExecHandler::ChunkIsPaged(DChunk* aChunk)
	{
	return (aChunk->iAttributes & DChunk::EDataPaged) != 0;
	}


/**
@return ETrue if the process is data paged, EFalse otherwise.
*/
TBool ExecHandler::ProcessDefaultDataPaged(DProcess* aProcess)
	{
	return (aProcess->iAttributes & DProcess::EDataPaged) != 0;
	}



TInt ExecHandler::OpenObject(TObjectType aObjType, const TDesC8& aName, TOwnerType aType)
	{
	TFullName n;
	Kern::KUDesGet(n,aName);
	__KTRACE_OPT(KTHREAD,Kern::Printf("Exec::OpenObject %S",&n));
	if (Kern::ValidateFullName(n)!=KErrNone)
		K::PanicKernExec(EBadName);
	if ((TUint)aObjType>=(TUint)ENumObjectTypes)
		K::PanicKernExec(EBadObjectType);
	TInt h=0;
	DObject* pO=NULL;
	NKern::ThreadEnterCS();
	TInt r=TheCurrentThread->OpenObject(aType,n,h,pO,aObjType);
	NKern::ThreadLeaveCS();
	if(r==KErrNone)
		r = h;
	__KTRACE_OPT(KTHREAD,Kern::Printf("Exec::OpenObject returns %d",r));
	return r;
	}


TInt ExecHandler::HandleDuplicate(TInt aThreadHandle, TOwnerType aType, TInt& aHandle)
	{
	TInt h;
	kumemget32(&h, &aHandle, sizeof(h));
	__KTRACE_OPT(KTHREAD,Kern::Printf("Exec::HandleDuplicate %08x", h));
	DThread* pC=TheCurrentThread;
	NKern::LockSystem();
	DThread* pT=(DThread*)K::ThreadEnterCS(aThreadHandle,EThread);
	__KTRACE_OPT(KTHREAD,Kern::Printf("Exec::HandleDuplicate source thread %O",pT));
	TInt r=KErrBadHandle;
	NKern::LockSystem();
	DObject* pO=pT->ObjectFromHandle(h);
	h = 0;	// now holds value to be returned
	if (pO)
		r=pO->Open();
	NKern::UnlockSystem();
	if (r==KErrNone)
		{
		if (pO->Protection()!=DObject::EGlobal
			&& pT->iOwningProcess->iSecurityZone!=pC->iOwningProcess->iSecurityZone)
			{
#ifndef __REMOVE_PLATSEC_DIAGNOSTICS__
			r = PlatSec::ProcessIsolationFail(__PLATSEC_DIAGNOSTIC_STRING("Checked by RHandleBase::Duplicate"));
#else //__REMOVE_PLATSEC_DIAGNOSTICS__
			r = PlatSec::EmitDiagnostic();
#endif // !__REMOVE_PLATSEC_DIAGNOSTICS__
			}
		if (r==KErrNone)
			r = pC->MakeHandle(aType, pO, h);	// this will add to process if necessary
		if (r<KErrNone)
			pO->Close(NULL);	// can't have been added to process so NULL
		}
	pT->Close(NULL);
	TInt s = KErrNone;
	XTRAP(s, XT_DEFAULT, kumemput32(&aHandle, &h, sizeof(h)));
	if (s!=KErrNone && r==KErrNone)
		pC->HandleClose(h);
	NKern::ThreadLeaveCS();
	if (r == KErrBadHandle)
		K::PanicKernExec(EBadHandle);
	if (s != KErrNone)
		K::PanicKernExec(ECausedException);
	__KTRACE_OPT(KTHREAD,Kern::Printf("Exec::HandleDuplicate returns %d",r));
	return r;
	}

TInt ExecHandler::MutexCreate(const TDesC8* aName, TOwnerType aType)
	{
	TKName n;
	DObject* pO=NULL;
	const TDesC* pN=NULL;
	if (aName)
		{
		Kern::KUDesGet(n,*aName);
		pN=&n;
		__KTRACE_OPT(KSEMAPHORE,Kern::Printf("Exec::MutexCreate %S",pN));
		}
	else if (aType==EOwnerThread)
		pO=TheCurrentThread;
	else
		pO=TheCurrentThread->iOwningProcess;
	NKern::ThreadEnterCS();
	DMutex* pM;
	TInt r=K::MutexCreate(pM, *pN, pO, ETrue, KMutexOrdUser);
	if (r==KErrNone)
		{
		if(aName)
			pM->SetProtection(n.Length()? DObject::EGlobal : DObject::EProtected);
		r=K::MakeHandle(aType,pM);
		if (r<KErrNone)
			pM->Close(NULL);
		}
	NKern::ThreadLeaveCS();
	__KTRACE_OPT(KSEMAPHORE,Kern::Printf("Exec::MutexCreate returns %d",r));
	return r;
	}

TInt ExecHandler::SemaphoreCreate(const TDesC8* aName, TInt aCount, TOwnerType aType)
	{
	TKName n;
	DObject* pO=NULL;
	const TDesC* pN=NULL;
	if (aName)
		{
		Kern::KUDesGet(n,*aName);
		pN=&n;
		__KTRACE_OPT(KSEMAPHORE,Kern::Printf("Exec::SemaphoreCreate %S",pN));
		}
	else if (aType==EOwnerThread)
		pO=TheCurrentThread;
	else
		pO=TheCurrentThread->iOwningProcess;
	NKern::ThreadEnterCS();
	TInt r=KErrNoMemory;
	DSemaphore* pS=new DSemaphore;
	if (pS)
		r=pS->Create(pO,pN,aCount);
	if (r==KErrNone)
		{
		if(aName)
			pS->SetProtection(n.Length()? DObject::EGlobal : DObject::EProtected);
		r=K::MakeHandle(aType,pS);
		}
	if (r<KErrNone && pS)
		pS->Close(NULL);
	NKern::ThreadLeaveCS();
	__KTRACE_OPT(KSEMAPHORE,Kern::Printf("Exec::SemaphoreCreate returns %d",r));
	return r;
	}

LOCAL_C TInt OpenById(TUint anId, TOwnerType aType, TBool aProcess)
	{
	NKern::ThreadEnterCS();
	TInt type=aProcess?EProcess:EThread;
	DObjectCon* pC=K::Containers[type];
	pC->Wait();
	DObject* pO=aProcess?(DObject*)Kern::ProcessFromId(anId):(DObject*)Kern::ThreadFromId(anId);
	TInt r=KErrNotFound;
	if (pO && (r=pO->Open())==KErrNone)
		{
		pC->Signal();	// must release this before opening handle
		DProcess* pP;
		if(aProcess)
			pP=(DProcess*)pO;
		else
			pP=((DThread*)pO)->iOwningProcess;

		if( pO->Protection()!=DObject::EGlobal
			&& pP->iSecurityZone!=TheCurrentThread->iOwningProcess->iSecurityZone)
			{
#ifndef __REMOVE_PLATSEC_DIAGNOSTICS__
			r = PlatSec::ProcessIsolationFail(__PLATSEC_DIAGNOSTIC_STRING("Checked by RThread::Open(TThreadId)"));
#else //__REMOVE_PLATSEC_DIAGNOSTICS__
			r = PlatSec::EmitDiagnostic();
#endif // !__REMOVE_PLATSEC_DIAGNOSTICS__
			}
		if (r==KErrNone)
			r=K::MakeHandle(aType,pO);
		if (r<KErrNone)
			pO->Close(NULL);
		}
	else
		pC->Signal();
	NKern::ThreadLeaveCS();
	return r;
	}

TInt ExecHandler::ThreadOpenById(TUint anId, TOwnerType aType)
	{
	__KTRACE_OPT(KTHREAD,Kern::Printf("Exec::ThreadOpenById %d",anId));
	TInt r=OpenById(anId,aType,EFalse);
	__KTRACE_OPT(KTHREAD,Kern::Printf("Exec::ThreadOpenById returns %d",r));
	return r;
	}

TInt ExecHandler::ProcessOpenById(TUint anId, TOwnerType aType)
	{
	__KTRACE_OPT(KPROC,Kern::Printf("Exec::ProcessOpenById %d",anId));
	TInt r=OpenById(anId,aType,ETrue);
	__KTRACE_OPT(KPROC,Kern::Printf("Exec::ProcessOpenById returns %d",r));
	return r;
	}

// Enter with system locked, return with system unlocked
void ExecHandler::ThreadLogon(DThread* aThread, TRequestStatus* aStatus, TBool aRendezvous)
	{
	__KTRACE_OPT(KEXEC,Kern::Printf("Exec::ThreadLogon"));
	aThread->CheckedOpen();
	NKern::ThreadEnterCS();
	NKern::UnlockSystem();
	TInt r=aThread->Logon(aStatus,aRendezvous);
	if (r!=KErrNone)
		{
		if (r==KErrDied)
			r=aThread->iExitReason;
		Kern::RequestComplete(aStatus,r);
		}
	aThread->Close(NULL);
	NKern::ThreadLeaveCS();
	}

// Enter with system locked, return with system unlocked
TInt ExecHandler::ThreadLogonCancel(DThread* aThread, TRequestStatus* aStatus, TBool aRendezvous)
	{
	__KTRACE_OPT(KEXEC,Kern::Printf("Exec::ThreadLogonCancel"));
	aThread->CheckedOpen();
	NKern::ThreadEnterCS();
	NKern::UnlockSystem();
	DThread& t = *TheCurrentThread;
	TUint32 type = TLogon::ETargetThread;
	if (aRendezvous)
		type |= TLogon::ERendezvous;
	TInt r = TLogon::Cancel(t.iOwnedLogons, aThread, aStatus, type);
	aThread->Close(NULL);
	NKern::ThreadLeaveCS();
	return r;
	}

// Enter with system locked, return with system unlocked
void ExecHandler::ProcessLogon(DProcess* aProcess, TRequestStatus* aStatus, TBool aRendezvous)
	{
	__KTRACE_OPT(KEXEC,Kern::Printf("Exec::ProcessLogon"));
	aProcess->CheckedOpen();
	NKern::ThreadEnterCS();
	NKern::UnlockSystem();
	TInt r=aProcess->Logon(aStatus,aRendezvous);
	if (r!=KErrNone)
		{
		if (r==KErrDied)
			r=aProcess->iExitReason;
		Kern::RequestComplete(aStatus,r);
		}
	aProcess->Close(NULL);
	NKern::ThreadLeaveCS();
	}

// Enter with system locked, return with system unlocked
TInt ExecHandler::ProcessLogonCancel(DProcess* aProcess, TRequestStatus* aStatus, TBool aRendezvous)
	{
	__KTRACE_OPT(KEXEC,Kern::Printf("Exec::ProcessLogonCancel"));
	aProcess->CheckedOpen();
	NKern::ThreadEnterCS();
	NKern::UnlockSystem();
	DThread& t = *TheCurrentThread;
	TUint32 type = TLogon::ETargetProcess;
	if (aRendezvous)
		type |= TLogon::ERendezvous;
	TInt r = TLogon::Cancel(t.iOwnedLogons, aProcess, aStatus, type);
	aProcess->Close(NULL);
	NKern::ThreadLeaveCS();
	return r;
	}

TAny* ExecHandler::DllTls(TInt aHandle, TInt aDllUid)
	{
	// no protection needed since only this thread can access the TLS array
	__KTRACE_OPT(KEXEC,Kern::Printf("Exec::DllTls"));
#ifndef __EPOC32__
	extern TInt LookupDllUid(TInt);

	if (aDllUid == KDllUid_Special)
		aDllUid = LookupDllUid(aHandle);
#endif
	return TheCurrentThread->Tls(aHandle,aDllUid);
	}

TInt ExecHandler::DllSetTls(TInt aHandle, TInt aDllUid, TAny* aPtr)
	{
	__KTRACE_OPT(KEXEC,Kern::Printf("Exec::DllSetTls %08x->%08x", aHandle, aPtr));
#ifndef __EPOC32__
	extern TInt LookupDllUid(TInt);

	if (aDllUid == KDllUid_Special)
		aDllUid = LookupDllUid(aHandle);
#endif
	NKern::ThreadEnterCS();
	TInt r=TheCurrentThread->SetTls(aHandle,aDllUid,aPtr);
	NKern::ThreadLeaveCS();
	return r;
	}

void ExecHandler::DllFreeTls(TInt aHandle)
	{
	__KTRACE_OPT(KEXEC,Kern::Printf("Exec::DllFreeTls"));
	NKern::ThreadEnterCS();
	TheCurrentThread->FreeTls(aHandle);
	NKern::ThreadLeaveCS();
	}

TInt ExecHandler::ThreadRename(TInt aHandle, const TDesC8& aName)
	{
	TKName n;
	Kern::KUDesGet(n,aName);
	__KTRACE_OPT(KEXEC,Kern::Printf("Exec::ThreadRename %S",&n));
	NKern::LockSystem();
	DThread* pT=(DThread*)K::ThreadEnterCS(aHandle,EThread);
	if (pT!=TheCurrentThread &&
		pT->iOwningProcess->iSecurityZone!=TheCurrentThread->iOwningProcess->iSecurityZone)
		{
		if(TheSuperPage().KernelConfigFlags() & EKernelConfigPlatSecEnforcement)
			{
			pT->Close(NULL);
			K::ThreadLeaveCS();
			}
		K::ProcessIsolationFailure(__PLATSEC_DIAGNOSTIC_STRING("Checked by RThread::Rename"));
		if(TheSuperPage().KernelConfigFlags() & EKernelConfigPlatSecEnforcement)
			Kern::Fault("ThreadRename",0);
		}
	TInt r=pT->Rename(n);
	pT->Close(NULL);
	NKern::ThreadLeaveCS();
	return r;
	}

TInt ExecHandler::ProcessRename(TInt aHandle, const TDesC8& aName)
	{
	TKName n;
	Kern::KUDesGet(n,aName);
	__KTRACE_OPT(KEXEC,Kern::Printf("Exec::ProcessRename %S",&n));
	NKern::LockSystem();
	DProcess* pP=(DProcess*)K::ThreadEnterCS(aHandle,EProcess);
	if (pP->iSecurityZone!=TheCurrentThread->iOwningProcess->iSecurityZone)
		{
		if(TheSuperPage().KernelConfigFlags() & EKernelConfigPlatSecEnforcement)
			{
			pP->Close(NULL);
			K::ThreadLeaveCS();
			}
		K::ProcessIsolationFailure(__PLATSEC_DIAGNOSTIC_STRING("Checked by RProcess::Rename"));
		if(TheSuperPage().KernelConfigFlags() & EKernelConfigPlatSecEnforcement)
			Kern::Fault("ProcessRename",0);
		}
	TInt r=pP->Rename(n);
	pP->Close(NULL);
	NKern::ThreadLeaveCS();
	return r;
	}

TInt ExecHandler::ThreadProcess(DThread* aThread)
	{
	__KTRACE_OPT(KEXEC,Kern::Printf("Exec::ThreadProcess"));
	DProcess* pP=aThread->iOwningProcess;
	pP->Open();	// can't get an error here
	NKern::ThreadEnterCS();
	NKern::UnlockSystem();
	TInt r=K::MakeHandle(EOwnerThread,pP);
	if (r<KErrNone)
		pP->Close(NULL);
	NKern::ThreadLeaveCS();
	return r;
	}

RAllocator* ExecHandler::ThreadGetHeap(DThread* aThread)
	{
	__KTRACE_OPT(KEXEC,Kern::Printf("Exec::ThreadGetHeap %O",aThread));
	if (aThread->iOwningProcess->iSecurityZone!=TheCurrentThread->iOwningProcess->iSecurityZone)
		K::ProcessIsolationFailure(__PLATSEC_DIAGNOSTIC_STRING("Checked by RThread::Heap"));
	return aThread->iAllocator;
	}

void ExecHandler::HandleName(TInt aHandle, TDes8& aName)
	{
	__KTRACE_OPT(KEXEC,Kern::Printf("Exec::HandleName"));
	DObject* pO=NULL;
	TInt r=K::OpenObjectFromHandle(aHandle,pO);
	if (r!=KErrNone)
		K::PanicKernExec(EBadHandle);
	TKName n;
	pO->Name(n);
	pO->Close(NULL);
	NKern::ThreadLeaveCS();
	Kern::KUDesPut(aName,n);
	}

void ExecHandler::HandleFullName(TInt aHandle, TDes8& aFullName)
	{
	__KTRACE_OPT(KEXEC,Kern::Printf("Exec::HandleFullName"));
	DObject* pO=NULL;
	TInt r=K::OpenObjectFromHandle(aHandle,pO);
	if (r!=KErrNone)
		K::PanicKernExec(EBadHandle);
	TFullName n;
	pO->FullName(n);
	pO->Close(NULL);
	NKern::ThreadLeaveCS();
	Kern::KUDesPut(aFullName,n);
	}

void ExecHandler::HandleCount(DThread* aThread, TInt& aProcessHandleCount, TInt& aThreadHandleCount)
	{
	__KTRACE_OPT(KEXEC,Kern::Printf("Exec::HandleCount"));
	TInt tCount=aThread->iHandles.ActiveCount();
	TInt pCount=aThread->iOwningProcess->iHandles.ActiveCount();
	NKern::UnlockSystem();
	kumemput32(&aProcessHandleCount,&pCount,sizeof(pCount));
	kumemput32(&aThreadHandleCount,&tCount,sizeof(tCount));
	}

TInt ExecHandler::GetBTraceId(DObject* aObj)
//
// Get the BTraceID of any given RHandleBase-derived object. In practice
// this ID is simply a pointer to the associated DObject.
//
	{
	return (TInt)aObj;
	}

void ExecHandler::HandleInfo(TInt aHandle, THandleInfo* anInfo)
//
// Find out how many threads and processes have an open handle on the object given by 
// a handle, and whether it's open in this process and thread.
//
	{
	__KTRACE_OPT(KEXEC,Kern::Printf("Exec::HandleInfo"));
	THandleInfo hinfo;
	memclr(&hinfo, sizeof(hinfo));
	DObject* pO=NULL;
	TInt r=K::OpenObjectFromHandle(aHandle,pO);
	if (r==KErrNone)
		{
		RObjectIx::Wait();
		DProcess* pCurrentProcess=TheCurrentThread->iOwningProcess;
		hinfo.iNumOpenInThread=TheCurrentThread->iHandles.Count(pO);
		hinfo.iNumOpenInProcess=pCurrentProcess->iHandles.Count(pO);

		DObjectCon& threads=*K::Containers[EThread];
		threads.Wait();
		TInt c=threads.Count();
		TInt i=0;
		for (;i<c;i++)
			{
			DThread *pT=(DThread *)threads[i];
			TInt r=pT->iHandles.At(pO);
			if (r!=KErrNotFound)
				{
				++hinfo.iNumThreads;
				if (pT->iOwningProcess==pCurrentProcess)
					++hinfo.iNumOpenInProcess;
				}
			}
		threads.Signal();
		DObjectCon& processes=*K::Containers[EProcess];
		processes.Wait();
		c=processes.Count();
		for (i=0;i<c;i++)
			{
			DProcess *pP=(DProcess *)processes[i];
			TInt r=pP->iHandles.At(pO);
			if (r!=KErrNotFound)
				++hinfo.iNumProcesses;
			}
		processes.Signal();
		RObjectIx::Signal();
		pO->Close(NULL);
		NKern::ThreadLeaveCS();
		}
	kumemput32(anInfo,&hinfo,sizeof(hinfo));
	}

TUint ExecHandler::HandleAttributes(TInt aHandle)
	{
	__KTRACE_OPT(KEXEC,Kern::Printf("Exec::HandleAttributes"));
	TUint attributes = RHandleBase::EReadAccess | RHandleBase::EDirectReadAccess;
	NKern::LockSystem();
	DObject* pO = K::ObjectFromHandle(aHandle);
	if (pO->UniqueID() - 1 == EChunk)
		{
		DChunk* pChunk = (DChunk*) pO;
		if (!(pChunk->iAttributes & DChunk::EReadOnly) ||
			(TheCurrentThread->iOwningProcess->iId == pChunk->iControllingOwner))
			{
			attributes |= (RHandleBase::EWriteAccess | RHandleBase::EDirectWriteAccess);
			}
		}
	else
		{
		attributes |= (RHandleBase::EWriteAccess | RHandleBase::EDirectWriteAccess);
		}
	NKern::UnlockSystem();
	return attributes;
	}

TLibraryFunction ExecHandler::LibraryLookup(TInt aLibraryHandle, TInt aOrdinal)
	{
	TLibraryFunction f = NULL;
	NKern::LockSystem();

XTRAP_PAGING_RETRY(
	DLibrary* library = (DLibrary*)K::ObjectFromHandle(aLibraryHandle,ELibrary);
	__KTRACE_OPT(KEXEC,Kern::Printf("Exec::LibraryLookup %O %d",library,aOrdinal));
	DCodeSeg* pS = library->iCodeSeg;
	if(pS)
		f = pS->Lookup(aOrdinal);
)
	NKern::UnlockSystem();
	return f;
	}

/**
Retrieves pointer to the named symbol export data, if present.
	
@param aProcessHandle Handle to the process whose code seg to search
@return	Pointer to named symbol export data if its present, NULL otherwise
@internalComponent
*/
TAny* ExecHandler::ProcessExeExportData(void)
	{
	DProcess* pP = &Kern::CurrentProcess();
	__KTRACE_OPT(KEXEC,Kern::Printf("Exec::ProcessExeExportData %O",pP));
	DCodeSeg* pS = pP->CodeSeg();	// can't be null if process is running
	
	// Lookup() returns NULL if this is not a stdexe/stddll
	return (TAny*)pS->Lookup(0);
	}

void ExecHandler::LibraryType(DLibrary* aLibrary, TUidType& aUids)
	{
	__KTRACE_OPT(KEXEC,Kern::Printf("Exec::LibraryType"));
	TUidType uids;
	memclr(&uids, sizeof(uids));
	DCodeSeg* pS=aLibrary->iCodeSeg;
	if (pS)
		uids=pS->iUids;
	__KTRACE_OPT(KEXEC,Kern::Printf("UIDS: %08x,%08x,%08x",uids.iUid[0],uids.iUid[1],uids.iUid[2]));
	NKern::UnlockSystem();
	kumemput32(&aUids,&uids,sizeof(TUidType));
	}

void ExecHandler::LibraryFileName(DLibrary* aLibrary, TDes8& aFileName)
	{
	__KTRACE_OPT(KEXEC,Kern::Printf("Exec::LibraryFileName"));
	TFileName fn;
	DCodeSeg* pS=aLibrary->iCodeSeg;
	if (pS)
		pS->AppendFullFileName(fn);
	NKern::UnlockSystem();
	P::NormalizeExecutableFileName(fn);
	Kern::KUDesPut(aFileName, fn);
	}

TInt ExecHandler::HalFunction(TInt aGroup, TInt aFunction, TAny* a1, TAny* a2)
	{
	TInt aDeviceNumber = TUint(aGroup)>>16;
	aGroup=aGroup&0xffff;
	__KTRACE_OPT(KEXEC,Kern::Printf("Exec::HalFunction(%d,%d,%08x,%08x,%d)",aGroup,aFunction,a1,a2,aDeviceNumber));
	TInt r=KErrNotSupported;
	if(TUint(aDeviceNumber)>=TUint(KMaxHalEntries))
		return r;
	if (aGroup>=0 && aGroup<KMaxHalGroups)
		{
		SHalEntry2* pE=&K::HalEntryArray[aGroup];
		SHalEntry* pBase=(SHalEntry*)pE;
		THalFunc f=NULL;
		TAny* p=NULL;
		if(aDeviceNumber>0)
			{
			if(!pE->iExtendedEntries)
				return r;
			pBase=pE->iExtendedEntries + (aDeviceNumber-1);
			}
		NKern::LockSystem();
		f=pBase->iFunction;
		p=pBase->iPtr;
		NKern::UnlockSystem();
		if (f)
			r=(*f)(p,aFunction,a1,a2);
		}
	return r;
	}

TUint32 ExecHandler::DebugMask()
	{
	return TheSuperPage().iDebugMask[0];
	}

TUint32 ExecHandler::DebugMaskIndex(TUint aIndex)
 	{
	if (aIndex >= (TUint)KNumTraceMaskWords) 
		return 0;	
	else
		return TheSuperPage().iDebugMask[aIndex];
	}

void ExecHandler::SetDebugMask(TUint32 aVal)
	{
	TheSuperPage().iDebugMask[0]=(TInt)aVal;
	}

void ExecHandler::SetDebugMaskIndex(TUint32 aVal, TUint aIndex)
	{
	if (aIndex >= (TUint)KNumTraceMaskWords) return;
	
	// check that we have permission to set KALLTHREADSSYSTEM bit
	if (aIndex == DEBUGMASKWORD2 && (aVal & (1 << (KALLTHREADSSYSTEM%32))))
		{
		DProcess* currentProcess=TheCurrentThread->iOwningProcess;
		if(!currentProcess->HasCapability(ECapabilityProtServ,__PLATSEC_DIAGNOSTIC_STRING("Checked by User::SetDebugMask(TUint32, TUint)")))
			K::UnlockedPlatformSecurityPanic();
		}

	TheSuperPage().iDebugMask[aIndex]=(TInt)aVal;
	}

RAllocator* ExecHandler::HeapSwitch(RAllocator* aA)
	{
	DThread* pT = TheCurrentThread;
	RAllocator* pA = pT->iAllocator;
	pT->iAllocator = aA;
	if (!pT->iCreatedAllocator)
		pT->iCreatedAllocator = aA;
	return pA;
	}

void ExecHandler::KernelHeapDebug(TInt aFunction, TInt a1, TAny* a2)
	{
	(void)aFunction;
	(void)a1;
	(void)a2;
	__KTRACE_OPT(KEXEC,Kern::Printf("Exec::KernelHeapDebug %d,%08x,%08x",aFunction,a1,a2));
#ifdef _DEBUG
	TInt panic=KMinTInt;
	switch (aFunction)
		{
		case EDbgMarkStart:
			NKern::ThreadEnterCS();
			K::Allocator->__DbgMarkStart();
			NKern::ThreadLeaveCS();
			break;

		case EDbgMarkCheck:
			{
			TBuf8<KMaxFileName> name;
			TKernelHeapMarkCheckInfo info;
			kumemget32(&info,a2,sizeof(info));
			Kern::KUDesGet(name,*info.iFileName);
			NKern::ThreadEnterCS();
			TInt r = K::Allocator->__DbgMarkCheck(info.iCountAll, a1, name, info.iLineNum);
			if (r!=KErrNone)
				panic=EFailedKernelHeapCheck;
			NKern::ThreadLeaveCS();
			break;
			}

		case EDbgMarkEnd:
			{
			NKern::ThreadEnterCS();
			TInt r = K::Allocator->__DbgMarkEnd(a1);
			if (r!=KErrNone)
				panic=EFailedKernelHeapCheck;
			NKern::ThreadLeaveCS();
			break;
			}

		case EDbgSetAllocFail:
			NKern::ThreadEnterCS();
			K::Allocator->__DbgSetAllocFail((RAllocator::TAllocFail)a1,(TInt)a2);
			NKern::ThreadLeaveCS();
			break;

		case EDbgSetBurstAllocFail:
			{
			SRAllocatorBurstFail burstFail;
			kumemget32(&burstFail, a2, sizeof(SRAllocatorBurstFail));
			NKern::ThreadEnterCS();
			K::Allocator->__DbgSetBurstAllocFail((RAllocator::TAllocFail)a1, burstFail.iRate, burstFail.iBurst);
			NKern::ThreadLeaveCS();
			break;
			}

		case EDbgCheckFailure:
			{
			NKern::ThreadEnterCS();
			TUint failures = K::Allocator->__DbgCheckFailure();
			NKern::ThreadLeaveCS();
			kumemput32(a2, (TAny*)&failures, sizeof(TUint));
			break;
			}

		case EDbgGetAllocFail:
			{
			NKern::ThreadEnterCS();
			TInt allocFail = K::Allocator->__DbgGetAllocFail();
			NKern::ThreadLeaveCS();
			kumemput32(a2, (TAny*)&allocFail, sizeof(TInt));
			break;
			}

		default:
			panic=EBadKernelHeapDebugFunction;
			break;
		}
	if (panic>KMinTInt)
		K::PanicKernExec(panic);
#endif
	}

TExceptionHandler ExecHandler::ExceptionHandler(DThread* aThread)
	{
	__KTRACE_OPT(KEXEC,Kern::Printf("Exec::ExceptionHandler"));
	if(aThread!=TheCurrentThread)
		if(TheCurrentThread->iOwningProcess->iSecurityZone!=KSecurityZoneLegacyCode
			|| aThread->iOwningProcess->iSecurityZone!=KSecurityZoneLegacyCode
			)
			K::ProcessIsolationFailure(__PLATSEC_DIAGNOSTIC_STRING("Checked by RThread::ExceptionHandler"));
	return aThread->iExceptionHandler;
	}

TInt ExecHandler::SetExceptionHandler(DThread* aThread, TExceptionHandler aHandler, TUint32 aMask)
	{
	__KTRACE_OPT(KEXEC,Kern::Printf("Exec::SetExceptionHandler"));
	if(aThread!=TheCurrentThread)
		if(TheCurrentThread->iOwningProcess->iSecurityZone!=KSecurityZoneLegacyCode
			|| aThread->iOwningProcess->iSecurityZone!=KSecurityZoneLegacyCode
			)
			K::ProcessIsolationFailure(__PLATSEC_DIAGNOSTIC_STRING("Checked by RThread::SetExceptionHandler"));
	aThread->iExceptionHandler=aHandler;
	aThread->iExceptionMask=aMask;
	return KErrNone;
	}

void ExecHandler::ModifyExceptionMask(DThread* aThread, TUint32 aClearMask, TUint32 aSetMask)
	{
	__KTRACE_OPT(KEXEC,Kern::Printf("Exec::ModifyExceptionMask"));
	if(aThread!=TheCurrentThread)
		if(TheCurrentThread->iOwningProcess->iSecurityZone!=KSecurityZoneLegacyCode
			|| aThread->iOwningProcess->iSecurityZone!=KSecurityZoneLegacyCode
			)
			K::ProcessIsolationFailure(__PLATSEC_DIAGNOSTIC_STRING("Checked by RThread::ModifyExceptionMask"));
	TUint& m=aThread->iExceptionMask;
	m=(m&~aClearMask)|aSetMask;
	}

TInt ExecHandler::RaiseException(DThread* aThread, TExcType aType)
	{
	__KTRACE_OPT(KEXEC,Kern::Printf("Exec::RaiseException %d on %O",aType,aThread));
	if(aThread!=TheCurrentThread)
		if(TheCurrentThread->iOwningProcess->iSecurityZone!=KSecurityZoneLegacyCode
			|| aThread->iOwningProcess->iSecurityZone!=KSecurityZoneLegacyCode
			)
			K::ProcessIsolationFailure(__PLATSEC_DIAGNOSTIC_STRING("Checked by RThread::RaiseException"));
	return aThread->RaiseException(aType);
	}

TBool ExecHandler::IsExceptionHandled(DThread* aThread,TExcType aType, TBool aSwExcInProgress)
	{
	__KTRACE_OPT(KEXEC,Kern::Printf("Exec::IsExceptionHandled %d %d",aType,aSwExcInProgress));
	if(aThread!=TheCurrentThread)
		if(TheCurrentThread->iOwningProcess->iSecurityZone!=KSecurityZoneLegacyCode
			|| aThread->iOwningProcess->iSecurityZone!=KSecurityZoneLegacyCode
			)
			K::ProcessIsolationFailure(__PLATSEC_DIAGNOSTIC_STRING("Checked by RThread::IsExceptionHandled"));
	TBool isHandled=aThread->IsExceptionHandled(aType);
	NKern::UnlockSystem();
	if (aSwExcInProgress)
		DKernelEventHandler::Dispatch(EEventSwExc, (TAny*)aType, NULL);
	return isHandled;
	}

void ExecHandler::ThreadContext(DThread* aThread, TDes8& aDes)
	{
	__KTRACE_OPT(KEXEC,Kern::Printf("Exec::ThreadContext %O",aThread));
	TBuf8<KMaxThreadContext> c;
	aThread->Context(c);
	NKern::UnlockSystem();
	Kern::InfoCopy(aDes,c);
	}

TInt ExecHandler::ThreadStackInfo(DThread* aThread, TThreadStackInfo& aInfo)
	{
	__KTRACE_OPT(KEXEC,Kern::Printf("Exec::ThreadStackInfo %O",aThread));
	TThreadStackInfo info;
	memclr(&info, sizeof(info));
	TInt r=KErrGeneral;
	if(aThread->iUserStackRunAddress)
		{
		info.iBase = aThread->iUserStackRunAddress+aThread->iUserStackSize;
		info.iLimit = aThread->iUserStackRunAddress;
		info.iExpandLimit = aThread->iUserStackRunAddress;
		r = KErrNone;
		}
	NKern::UnlockSystem();
	kumemput32(&aInfo,&info,sizeof(info));
	return r;
	}

TInt ExecHandler::ProcessGetMemoryInfo(TInt aProcessHandle, TModuleMemoryInfo& aInfo)
	{
	TModuleMemoryInfo info;
	memclr(&info, sizeof(info));
	TInt r = KErrGeneral;
	NKern::LockSystem();

XTRAP_PAGING_RETRY(
	DProcess* process = (DProcess*)K::ObjectFromHandle(aProcessHandle,EProcess);
	__KTRACE_OPT(KEXEC,Kern::Printf("Exec::ProcessGetMemoryInfo %O",process));
	DCodeSeg* seg=process->iCodeSeg;
	if(seg)
		r = seg->GetMemoryInfo(info,TheCurrentThread->iOwningProcess);
)
	NKern::UnlockSystem();
	kumemput32(&aInfo, &info, sizeof(info));
	return r;
	}

TInt ExecHandler::LibraryGetMemoryInfo(TInt aLibraryHandle, TModuleMemoryInfo& aInfo)
	{
	TModuleMemoryInfo info;
	memclr(&info, sizeof(info));
	TInt r = KErrGeneral;
	NKern::LockSystem();

XTRAP_PAGING_RETRY(
	DLibrary* library = (DLibrary*)K::ObjectFromHandle(aLibraryHandle,ELibrary);
	__KTRACE_OPT(KEXEC,Kern::Printf("Exec::LibraryGetMemoryInfo %O",library));
	DCodeSeg* seg=library->iCodeSeg;
	if(seg)
		r = seg->GetMemoryInfo(info,TheCurrentThread->iOwningProcess);
)
	NKern::UnlockSystem();
	kumemput32(&aInfo, &info, sizeof(info));
	return r;
	}

void AccessMachineConfig()
	{
	NKern::ThreadEnterCS();
	Kern::MutexWait(*K::MachineConfigMutex);
	}

void EndAccessMachineConfig()
	{
	Kern::MutexSignal(*K::MachineConfigMutex);
	NKern::ThreadLeaveCS();
	}

TInt ExecHandler::MachineConfiguration(TDes8& aConfig, TInt& aSize)
//
// Get the machine configuration
// Enter and leave with system unlocked
//
	{
	__KTRACE_OPT(KEXEC,Kern::Printf("Exec::MachineConfiguration"));
	if(!Kern::CurrentThreadHasCapability(ECapabilityReadDeviceData,__PLATSEC_DIAGNOSTIC_STRING("Checked by User::MachineConfiguration")))
		K::UnlockedPlatformSecurityPanic();
    const TPtrC8 platConfig(A::MachineConfiguration());
    TInt platSize=platConfig.Length();
	TInt usize=sizeof(TUid)+sizeof(TVersion)+sizeof(platSize)+platSize;

	TInt ulen, umax;
	Kern::KUDesInfo(aConfig,ulen,umax);
	TInt r = KErrArgument;
	if(umax<usize)
		goto done; // user buffer not big enough

	// create tempory  buffer for config data
	{
	DThread& t=*TheCurrentThread;
	NKern::ThreadEnterCS();
	TUint8* buf=(TUint8*)Kern::Alloc(usize);
	t.iTempAlloc=buf;			// if we are killed, buf will be deleted
	NKern::ThreadLeaveCS();
	r = KErrNoMemory;
	if (!buf)
		goto done; // no memory

	// get config data
	{
	TUint8* p=buf;
	*((TUid*&)p)++ = KMachineConfigurationUid;
	*((TVersion*&)p)++ = TVersion(KMachineConfigurationMajorVersionNumber,KMachineConfigurationMinorVersionNumber,KMachineConfigurationBuildVersionNumber);
	*((TInt*&)p)++ = platSize;
	AccessMachineConfig();
	NKern::LockSystem();
	memcpy(p,platConfig.Ptr(),platSize);
	NKern::UnlockSystem();
	EndAccessMachineConfig();

	Kern::KUDesPut(aConfig,TPtrC8(buf,usize));

	NKern::ThreadEnterCS();
	t.iTempAlloc=NULL;
	delete buf;
	NKern::ThreadLeaveCS();

	r = KErrNone;
	}
	}
done:
	kumemput32(&aSize,&usize,sizeof(usize));
	return r;
	}

TInt ExecHandler::SetMachineConfiguration(const TDesC8& aConfig)
//
// Set the machine configuration
// Enter and leave with system unlocked
//
	{
	__KTRACE_OPT(KEXEC,Kern::Printf("Exec::SetMachineConfiguration"));
	if(!Kern::CurrentThreadHasCapability(ECapabilityWriteDeviceData,__PLATSEC_DIAGNOSTIC_STRING("Checked by User::SetMachineConfiguration")))
		K::UnlockedPlatformSecurityPanic();
	TInt len;
	TInt maxLen;
	const TUint8* pC=Kern::KUDesInfo(aConfig,len,maxLen);
	TInt r=KErrNoMemory;
	TUint8* pB=NULL;
    TPckgBuf<TUid> uid;
    TPckgBuf<TVersion> version;
    TPckgBuf<TInt> platSizeBuf;
	TInt platSize;
	TInt i=0;
	TPtrC8 platConfig;
	DThread& t=*TheCurrentThread;

	// first allocate a kernel-side buffer big enough to hold the new configuration
	NKern::ThreadEnterCS();
	pB=(TUint8*)Kern::Alloc(len);
	if (!pB)
		goto endSetMachineConfig2;
	t.iTempAlloc=pB;			// if we are killed, pB will be deleted
	NKern::ThreadLeaveCS();

	// copy the configuration
	kumemget(pB,pC,len);
	r=KErrArgument;
	if (len<(TInt)(sizeof(TUid)+sizeof(TVersion)+sizeof(TInt)))
		goto endSetMachineConfig;

	// extract and check the UID
	uid.Copy(pB+i,(TInt)sizeof(TUid));
    if (uid().iUid!=KMachineConfigurationUidValue)
		goto endSetMachineConfig;
	i+=(TInt)sizeof(TUid);

	// extract and check the version
	version.Copy(pB+i,(TInt)sizeof(TVersion));
	r=KErrNotSupported;
    if(!Kern::QueryVersionSupported(TVersion(KMachineConfigurationMajorVersionNumber,KMachineConfigurationMinorVersionNumber,KMachineConfigurationBuildVersionNumber),version()))
		goto endSetMachineConfig;
	i+=(TInt)sizeof(TVersion);

	// extract and check the super page size
	platSizeBuf.Copy(pB+i,(TInt)sizeof(TInt));
	i+=(TInt)sizeof(TInt);
	platSize=platSizeBuf();
	r=KErrArgument;
    if (platSize>(len-i))
		goto endSetMachineConfig;

	platConfig.Set(pB+i,platSize);

	// restore the super page config
	AccessMachineConfig();
	K::SetMachineConfiguration(platConfig);
	r=KErrNone;
	EndAccessMachineConfig();

endSetMachineConfig:
	NKern::ThreadEnterCS();
	t.iTempAlloc=NULL;
	delete pB;
endSetMachineConfig2:
	NKern::ThreadLeaveCS();
	return r;
	}

TInt ExecHandler::ThreadCreate(const TDesC8& aName, TOwnerType aType, SThreadCreateInfo& aInfo)
	{
	TKName n;
	Kern::KUDesGet(n,aName);
	__KTRACE_OPT(KEXEC,Kern::Printf("Exec::ThreadCreate %S",&n));
	TUint32 infoBuf[KMaxThreadCreateInfo/sizeof(TUint32)];
	SThreadCreateInfo& info = *(SThreadCreateInfo*)infoBuf;
	kumemget32(&info, &aInfo, sizeof(SThreadCreateInfo));
	TInt r = ( (info.iTotalSize < (TInt)sizeof(SThreadCreateInfo)) || (info.iTotalSize > KMaxThreadCreateInfo) || (info.iTotalSize & 7) )
		? KErrArgument : KErrNone;
	if (info.iUserStackSize < KMaxThreadCreateInfo*2)
		r=KErrArgument;
	if (r==KErrNone)
		{
		if (info.iTotalSize > (TInt)sizeof(SThreadCreateInfo))
			kumemget32( (&info)+1, (&aInfo)+1, info.iTotalSize-(TInt)sizeof(SThreadCreateInfo) );
		info.iType=EThreadUser;
		info.iSupervisorStackSize=0;	// zero means use default value
		info.iSupervisorStack=NULL;
		info.iInitialThreadPriority=EThrdPriorityNormal;
		info.iName.Set(n);
		NKern::ThreadEnterCS();
		DThread* pT=NULL;
		TInt h;
		r=TheCurrentThread->iOwningProcess->NewThread(pT, info, &h, aType);
		if(r==KErrNone)
			r = h;
		NKern::ThreadLeaveCS();
		}
	__KTRACE_OPT(KEXEC,Kern::Printf("Exec::ThreadCreate returns %d",r));
	return r;
	}

/********************************************
 * Kernel-side executive calls
 ********************************************/

TInt K::MutexCreate(DMutex*& aMutex, const TDesC& aName, DObject* anOwner, TBool aVisible, TUint aOrder)
	{
	__KTRACE_OPT(KSEMAPHORE,Kern::Printf("K::MutexCreate %S owner %O visible=%d order=%02x",&aName,anOwner,aVisible,aOrder));
	DMutex* pM=new DMutex;
	TInt r=KErrNoMemory;
	if (pM)
		{
		r=pM->Create(anOwner, &aName, aVisible, aOrder);
		if (r==KErrNone)
			aMutex=pM;
		else
			pM->Close(NULL);
		}
	__KTRACE_OPT(KSEMAPHORE,Kern::Printf("K::MutexCreate returns %d %08x",r,pM));
	return r;
	}

/**	Creates a new thread.

	It receives a parameter of type SThreadCreateInfo. The members of this structure have the following meaning for this function:

	iHandle	[out]	Heap allocated DThread pointer to the new created thread.
					This member is valid only if the return code is KErrNone.
					Do not assume it's NULL if the operation failed.
	iType	[in]	Specifies if the thread will run in User mode or in Kernel mode.
					It can be one of:
						EThreadInitial - this is the initial thread
						EThreadSupervisor - this runs in supervisor mode
						EThreadMinimalSupervisor - this runs in supervisor mode and has no handles array
						EThreadUser - this runs in User Mode
						EThreadAPInitial - this is the initial thread on a non-boot processor (SMP only)
	iFunction [in]	This is the function that will be run in the new thread.
	iPtr	[in]	Extra custom parameters passed to iFunction when it starts running.
	iSupervisorStack	Ignored.
	iSupervisorStackSize [in]	If the thread is a Kernel thread, this parameter will specify the desired supervisor stack size.
								If the size is 0, the size will be specified by K::SupervisorThreadStackSize, which is 4K.
								The size will be rounded up to Page or Block size.
	iUserStack			Ignored.
	iUserStackSize [in]	If the thread is a User Mode thread, this parameter will specify the desired user stack size.
						The size will be rounded up to Page or Block size.
						It will fail with KErrTooBig if the size is greater than PP::MaxUserThreadStack which is usually set to 0x14000 (80K).
	iInitialThreadPriority [in] Initial priority for this thread. Must be in [0,63] interval.
	iName	[in]	Name of the thread. In case you do not specify a name for this thread, it will be created EProtected
					and any attempts to open it will fail with KErrPermissionDenied. Otherwise, the object will be EGlobal.
	iTotalSize [in]	Total size in bytes of the SThreadCreateInfo, including the extras. Fails with KErrArgument if it's less than sizeof(SThreadCreateInfo) or greater than KMaxThreadCreateInfo.

	It can fail with KErrArgument if aInfo.iTotalSize is not set correctly or aInfo.iPriority is not in [0,63] interval
	In x86 port it can fail with KErrArgument if anInfo.iStackBase is NULL or anInfo.iStackSize is less than 0x100
	It can fail with KErrTooBig if aInfo.iUserStackSize is bigger than maximum user stack size
	It can fail in Emulator with a Win32 error code returned by GetLastError if the Win32 thread or the scheduler Win32 event cannot be created
	It can fail with KErrDied if the thread dies during the creation process
	It can fail with KErrNoMemory in OOM scenarios.
	It can fail with KErrGeneral indicating a general malfunction or data corruption.
	
	If it succeeds it will return the heap allocated DThread pointer to the new created thread in iHandle member of aInfo
	  
	@param aInfo Information passed by the caller to specify how to create the thread.
	
	@pre Calling thread must be in a critical section.
	@pre Call in a thread context.
	@pre Kernel must be unlocked.
	@pre interrupts enabled
	@pre No fast mutex can be held
	
	@post Calling thread is in a critical section.
*/
EXPORT_C TInt Kern::ThreadCreate(SThreadCreateInfo& aInfo)
	{
	CHECK_PRECONDITIONS(MASK_THREAD_CRITICAL,"Kern::ThreadCreate");		
	__KTRACE_OPT(KEXEC,Kern::Printf("Kern::ThreadCreate %S",&aInfo.iName));
	aInfo.iHandle=NULL;
	DThread* pT=NULL;
	TBool svc = aInfo.iType!=EThreadUser;
	DProcess* pP = svc ? K::TheKernelProcess : TheCurrentThread->iOwningProcess;
	aInfo.iSupervisorStack = NULL;
	aInfo.iUserStack = NULL;
	TInt r = pP->NewThread(pT, aInfo, NULL, EOwnerProcess);
	if (r==KErrNone)
		aInfo.iHandle = pT;
	__KTRACE_OPT(KEXEC,Kern::Printf("Kern::ThreadCreate returns %d",r));
	return r;
	}

#ifdef _UNICODE
void ccopy (TUint16* aDest, const TAny* aSrc)
{
	TUint16* pSrc = (TUint16*)aSrc;
	if(aSrc) {
		TUint16* p = aDest + 1;
		for (;*pSrc;)
			*p++ = *pSrc++;
		*aDest = (TUint16)(p-aDest-1);
	}
	else
		*aDest = 0;	
}
#else
void ccopy (TUint8* aDest, const TAny* aSrc)
{
	TUint8* pSrc = (TUint8*)aSrc;
	if(aSrc) {
		TUint8* p = aDest + 1;
		for (;*pSrc;)
			*p++ = *pSrc++;
		*aDest = (TUint8)(p-aDest-1);
	}
	else
		*aDest = 0;	
}
#endif

TInt ExecHandler::GetModuleNameFromAddress(TAny* aPtr, TDes8& aModuleName)
	{
	__KTRACE_OPT(KEXEC,Kern::Printf("Exec::GetModuleNameFromAddress 0x%X", aPtr));
	TFileName fn;
	Kern::AccessCode();
	DCodeSeg* pSeg = Kern::CodeSegFromAddress( (TLinAddr)aPtr, TheCurrentThread->iOwningProcess );
	if (pSeg)
		pSeg->AppendFullFileName(fn);
	Kern::EndAccessCode();
	if (!pSeg)
		{
 		return KErrNotFound;
		}
	Kern::KUDesPut(aModuleName, fn);
	return KErrNone;
	}

TInt ExecHandler::LocaleExports(TAny* aHandle, TLibraryFunction* aExportList)
//
// Change Locale setups
//
// NOTES
//
// 1. A mutex is NOT used to protect this, so if it is called by more than one thread
//    simultaneousely, the locale info may be garbled.
// 2. Locale libraries are never closed once they have been used. This prevents them from
//    being unloaded whilst other threads are referencing the data contained in them.
// 3. This function doesn't duplicate the EKA1 behaviour of saving the default data for
//    restoring later.
//
// The current use case for this function is that it is called once by WSERV after boot,
// so the above points should never be a problem. ( A generic 'change locale' will cause
// all sorts of problems throught the OS, so current practice won't change before a
// comprehensive rearchitecture occurs anyway.)
//
	{
	__KTRACE_OPT(KEXEC,Kern::Printf("Exec::LocaleExports"));

	DCodeSeg& cs=*DCodeSeg::VerifyCallerAndHandle(aHandle);

  	if(cs.iUids.iUid[1].iUid !=	KLocaleDllUid.iUid)
  		return KErrNotSupported;

	// Increment the code segment's access count
	// This will increment every time the same locale is reloaded, but
	// this doesn't matter since we don't unload locale DLLs.
	NKern::ThreadEnterCS();
	DCodeSeg::Wait();
	cs.CheckedOpen();
	DCodeSeg::Signal();
	NKern::ThreadLeaveCS();
	
	TLibraryFunction data[KNumLocaleExports];
	TInt n;
	for(n=0; n<KNumLocaleExports; n++)
		data[n] = (TLibraryFunction)cs.Lookup(n);
  
	kumemput32(aExportList, &data[0], KNumLocaleExports * sizeof(TLibraryFunction));

	return KErrNone;
	}

TInt ExecHandler::ResetMachine(TMachineStartupType /*aType*/)
	{
	// Don't implement without considering Platform Security!
	return KErrNotSupported;
	}

TInt ExecHandler::ExecuteInSupervisorMode(TSupervisorFunction aFunction, TAny* aParameter)
//
// Execute a function in supervisor mode. Only available to F32 - panic anyone else
//
	{
#ifndef __MEMMODEL_FLEXIBLE__
	if (TheCurrentThread->iOwningProcess == K::TheFileServerProcess)
		{
		UNLOCK_USER_MEMORY();
		TInt r = A::CallSupervisorFunction(aFunction,aParameter);
		LOCK_USER_MEMORY();
		return r;
		}
#endif
	K::PanicKernExec(EAccessDenied);
	return 0;
	}

_LIT(KDriveZed, "Z:");
void ExecHandler::DllFileName(TInt aHandle, TDes8& aFileName)
	{
	__KTRACE_OPT(KEXEC,Kern::Printf("Exec::DllFileName %08x",aHandle));
	TFileName n;
	Kern::AccessCode();
	DCodeSeg* pS=DCodeSeg::CodeSegFromEntryPoint(aHandle);
	if (pS)
		pS->AppendFullFileName(n);
	else
		n=KDriveZed;
	Kern::EndAccessCode();
	if (pS)
		P::NormalizeExecutableFileName(n);
	Kern::KUDesPut(aFileName,n);
	}

#ifdef MONITOR_THREAD_CPU_TIME
TInt ExecHandler::ThreadGetCpuTime(TInt aThreadHandle, Int64& aTime)
	{
	TUint64 t = 0;
	if (aThreadHandle == KCurrentThreadHandle)
		{
		t = NKern::ThreadCpuTime(NKern::CurrentThread());
		}
	else
		{
		NKern::LockSystem();
		DThread* pT = (DThread*)K::ObjectFromHandle(aThreadHandle, EThread);
		t = NKern::ThreadCpuTime(&pT->iNThread);
		NKern::UnlockSystem();
		}
	TUint32 f = NKern::CpuTimeMeasFreq();
	TUint64 t2 = t>>32;
	t = (t & KMaxTUint32)*1000000;
	t2 *= 1000000;
	t2 += (t>>32);
	t &= TUint64(KMaxTUint32);
	TUint64 q2 = t2/f;
	t2 -= q2*f;
	t += (t2<<32);
	TUint64 q = t/f;
	q += (q2<<32);
	kumemput32(&aTime, &q, sizeof(TInt64));
	return KErrNone;
	}
#else
TInt ExecHandler::ThreadGetCpuTime(TInt /*aThreadHandle*/, Int64& /*aTime*/)
	{
	return KErrNotSupported;
	}
#endif

TInt ExecHandler::SetMemoryThresholds(TInt aLowThreshold, TInt aGoodThreshold)
	{
	if(!Kern::CurrentThreadHasCapability(ECapabilityWriteDeviceData,__PLATSEC_DIAGNOSTIC_STRING("Checked by UserSvr::SetMemoryThresholds")))
		K::LockedPlatformSecurityPanic();
	if (aLowThreshold<0 || aGoodThreshold<aLowThreshold)
		return KErrArgument;
	K::MemoryLowThreshold=aLowThreshold;
	K::MemoryGoodThreshold=aGoodThreshold;
	return KErrNone;
	}

void ExecHandler::FsRegisterThread()
//
// Register the file server thread
//
	{

	__KTRACE_OPT(KBOOT,Kern::Printf("File server thread registered"));
	DThread* pT = TheCurrentThread;
	DProcess* pP = pT->iOwningProcess;
	if (K::TheFileServerProcess && K::TheFileServerProcess!=pP)
		K::PanicCurrentThread(EAccessDenied);
	pP->iFlags |= (KThreadFlagProcessCritical | KProcessFlagSystemPermanent);
	pT->iFlags |= KThreadFlagSystemPermanent;
	K::TheFileServerProcess=pP;
	K::ThreadEnterCS();
	pP->SetPriority(EPriorityFileServer);
	M::FsRegisterThread();
	K::ThreadLeaveCS();
	}


void ExecHandler::RegisterTrustedChunk(DChunk* aChunk)
//
// Register file server's chunk intended for DMA transfer
//
	{
	__KTRACE_OPT(KEXEC,Kern::Printf("Exec::RegisterTrustedChunk %x",aChunk));

	DProcess* pP=TheCurrentThread->iOwningProcess;
	if (K::TheFileServerProcess && K::TheFileServerProcess!=pP)
		{
		K::PanicCurrentThread(EAccessDenied);
		}
	aChunk->iAttributes |= DChunk::ETrustedChunk;
	}
	
void ExecHandler::WsRegisterThread()
//
// Register the window server thread
//
	{

	__KTRACE_OPT(KBOOT,Kern::Printf("Window server thread registered"));
	DProcess* pP=TheCurrentThread->iOwningProcess;
	if (K::TheWindowServerProcess && K::TheWindowServerProcess!=pP)
		K::PanicCurrentThread(EAccessDenied);
	K::TheWindowServerProcess=pP;
	K::ThreadEnterCS();
	pP->SetPriority(EPriorityWindowServer);
	K::ThreadLeaveCS();
	}

void ExecHandler::RequestSignal(TInt aCount)
//
// Signal the request semaphore.
//
	{

	__KTRACE_OPT(KEXEC,Kern::Printf("Exec::RequestSignal %d",aCount));
	NKern::ThreadRequestSignal(NULL,aCount);
	}

TInt ExecHandler::ThreadRequestCount(DThread* aThread)
//
// Get the request semaphores count.
//
	{

	__KTRACE_OPT(KEXEC,Kern::Printf("Exec::ThreadRequestCount"));
	return aThread->iNThread.iRequestSemaphore.iCount;
	}

void CompleteUserAfter(TAny* aPtr)
	{
	DThread* pT=DThread::FromTimer(aPtr);
	if (pT->iTimer.iState==TTimer::EWaiting)
		{
		Kern::QueueRequestComplete(pT,pT->iTimer.iRequest,KErrNone);
		pT->iTimer.iState=TTimer::EIdle;
		}
	}

void CompleteUserAt(TAny* aPtr)
	{
	DThread* pT=DThread::FromTimer(aPtr);
	if (pT->iTimer.iState==TTimer::EWaiting)
		{
		Kern::QueueRequestComplete(pT,pT->iTimer.iRequest,KErrNone);
		pT->iTimer.iState=TTimer::EIdle;
		}
	}

void CompleteUserAfterHighRes(TAny* aPtr)
	{
	DThread* pT=DThread::FromTimer(aPtr);
	NKern::LockSystem();
	pT->iTimer.iState=TTimer::EIdle;
	Kern::QueueRequestComplete(pT,pT->iTimer.iRequest,KErrNone);
	NKern::UnlockSystem();
	}

void ExecHandler::After(TInt anInterval, TRequestStatus& aStatus)
	{
	__KTRACE_OPT(KEXEC,Kern::Printf("Exec::After %d",anInterval));
	TInt iv=anInterval;
	if (iv<=0)
		{
		// just rotate the ready queue for this thread
		NKern::RotateReadyList(-1);
		TRequestStatus* s=&aStatus;
		Kern::RequestComplete(s,KErrNone);
		return;
		}
	NKern::ThreadEnterCS();
	TInt r=TheCurrentThread->iTimer.After(iv,CompleteUserAfter,aStatus);
	NKern::ThreadLeaveCS();
	if (r==KErrInUse)
		K::PanicKernExec(ETimerAlreadyPending);
	else if (r!=KErrNone)
		{
		TRequestStatus* s=&aStatus;
		Kern::RequestComplete(s,r);
		}
	}

void ExecHandler::AfterHighRes(TInt anInterval, TRequestStatus& aStatus)
	{
	__KTRACE_OPT(KEXEC,Kern::Printf("Exec::AfterHighRes %d",anInterval));
	TInt r=TheCurrentThread->iTimer.AfterHighRes(anInterval,CompleteUserAfterHighRes,aStatus);
	if (r==KErrInUse)
		K::PanicCurrentThread(ETimerAlreadyPending);
	}

void ExecHandler::At(const TTimeK& aTime, TRequestStatus& aStatus)
	{
	__KTRACE_OPT(KEXEC,Kern::Printf("Exec::At"));
	TTimeK time;
	kumemget32(&time,&aTime,sizeof(time));
	NKern::ThreadEnterCS();
	TInt r=TheCurrentThread->iTimer.At(time,CompleteUserAt,aStatus);
	NKern::ThreadLeaveCS();
	if (r==KErrInUse)
		K::PanicKernExec(ETimerAlreadyPending);
	else if (r!=KErrNone)
		{
		TRequestStatus* s=&aStatus;
		Kern::RequestComplete(s,r);
		}
	}

#ifndef __FASTEXEC_MACHINE_CODED__
RAllocator* ExecHandler::Heap()
	{
	return TheCurrentThread->iAllocator;
	}

extern void InvalidFastExec();

TTrapHandler* ExecHandler::PushTrapFrame(TTrap* aFrame)
//
// Push a new trap frame.
//
	{
#ifdef __LEAVE_EQUALS_THROW__
#ifdef __WINS__
	// On WINS overload this function to remember when a TWin32SEHTrap is installed
	// over another one
	DThread& t=*TheCurrentThread;
	t.iFrame=aFrame;
	return 0;
#else
	InvalidFastExec();
	return (TTrapHandler*)aFrame; // Prevents compiler warnings
#endif
#else
	DThread& t=*TheCurrentThread;
	aFrame->iHandler=t.iTrapHandler;
	aFrame->iNext=t.iFrame;
	t.iFrame=aFrame;
	return t.iTrapHandler;
#endif
	}

TTrap* ExecHandler::PopTrapFrame()
//
// Pop the current frame.
//
	{
#ifdef __LEAVE_EQUALS_THROW__
#ifdef __WINS__
	// On WINS overload this function to recall when a TWin32SEHTrap is installed
	// over another one
	DThread& t=*TheCurrentThread;
	return t.iFrame;
#else
	InvalidFastExec();
	return 0;
#endif
#else
	DThread& t=*TheCurrentThread;
	TTrap* pF=t.iFrame;
	if (pF)
		t.iFrame=pF->iNext;
	return pF;
#endif
	}

CActiveScheduler* ExecHandler::ActiveScheduler()
//
// Return the address of the current active scheduler
//
	{
	DThread& t=*TheCurrentThread;
	return t.iScheduler;
	}

void ExecHandler::SetActiveScheduler(CActiveScheduler* aScheduler)
//
// Set the address of the current active scheduler
//
	{
	DThread& t=*TheCurrentThread;
	t.iScheduler=aScheduler;
	}

TTrapHandler* ExecHandler::TrapHandler()
//
// Return the current trap handler.
//
	{
	DThread& t=*TheCurrentThread;
	return t.iTrapHandler;
	}

TTrapHandler* ExecHandler::SetTrapHandler(TTrapHandler* aHandler)
//
// Set the current trap handler.
//
	{
	DThread& t=*TheCurrentThread;
	TTrapHandler* pH=t.iTrapHandler;
	t.iTrapHandler=aHandler;
	return pH;
	}

void ExecHandler::SetReentryPoint(TLinAddr a)
	{
	DThread& t = *TheCurrentThread;
	t.iOwningProcess->iReentryPoint = a;
	}
#endif

#ifndef __REMOVE_PLATSEC_DIAGNOSTIC_STRINGS__
void K::DoProcessIsolationFailure(const char* aContextText)
	{
	// enter with system locked
	if(TheSuperPage().KernelConfigFlags() & EKernelConfigPlatSecProcessIsolation)
		{
		if(PlatSec::ProcessIsolationFail(aContextText)==KErrNone)
			return;
		LockedPlatformSecurityPanic();
		}
	}
#endif // !__REMOVE_PLATSEC_DIAGNOSTIC_STRINGS__

void K::DoProcessIsolationFailure()
	{
	// enter with system locked
#ifndef __REMOVE_PLATSEC_DIAGNOSTIC_STRINGS__
	DoProcessIsolationFailure(NULL);
#else //__REMOVE_PLATSEC_DIAGNOSTIC_STRINGS__
	if (TheSuperPage().KernelConfigFlags() & EKernelConfigPlatSecProcessIsolation)
		{
		if (PlatSec::EmitDiagnostic() == KErrNone)
			return;
		LockedPlatformSecurityPanic();
		}
#endif // !__REMOVE_PLATSEC_DIAGNOSTIC_STRINGS__
	}


void K::UnlockedPlatformSecurityPanic()
	{
	// enter with system unlocked
	NKern::LockSystem();
	K::LockedPlatformSecurityPanic();
	}

void K::LockedPlatformSecurityPanic()
	{
	// enter with system locked
	K::PanicCurrentThread(EPlatformSecurityTrap);
	}

void ExecHandler::ThreadRendezvous(TInt aReason)
	{
	NKern::ThreadEnterCS();
	TheCurrentThread->Rendezvous(aReason);
	NKern::ThreadLeaveCS();
	}

void ExecHandler::ProcessRendezvous(TInt aReason)
	{
	NKern::ThreadEnterCS();
	TheCurrentThread->iOwningProcess->Rendezvous(aReason);
	NKern::ThreadLeaveCS();
	}

void ExecHandler::DebugPrint(TAny* aDes, TInt aMode)
	{
	TInt l, m;
	const TText* p = Kern::KUDesInfo(*(const TDesC*)aDes, l, m);

#ifdef __DEBUGGER_SUPPORT__
	TUint r = DKernelEventHandler::Dispatch(EEventUserTrace, (TAny*)p, (TAny*)l);
	if (r & DKernelEventHandler::ETraceHandled)
		l = 0;
#endif

	TBuf8<256> buffer;
	l = Min(l,256);
	buffer.SetLength(l);
	kumemget((TUint8*)buffer.Ptr(), p, l); //Copy user-side data into kernel memory
	K::TextTrace(buffer,EUserTrace,!aMode);
	}


TInt ExecHandler::ProcessSetHandleParameter(DProcess* aProcess, TInt aSlot, TInt aHandle)
	{
	if (aProcess->iCreatorId != TheCurrentThread->iOwningProcess->iId) //check called by creator
		K::LockedPlatformSecurityPanic();

	if ((aSlot < 0) || (aSlot >= KArgIndex))
		K::PanicCurrentThread(EParameterSlotRange);

	if (aProcess->iEnvironmentData[aSlot] != 0)
		K::PanicCurrentThread(EParameterSlotInUse);

	DObject* pObject = K::ObjectFromHandle(aHandle);

	if (pObject->Protection() == DObject::ELocal) 
		K::LockedPlatformSecurityPanic();
	pObject->CheckedOpen();

	aProcess->iEnvironmentData[aSlot] = (TInt)pObject | EHandle;
	return KErrNone;
	}

//no locks held on entry
TInt ExecHandler::ProcessSetDataParameter(TInt aProcess, TInt aSlot, const TUint8* aData, TInt aLen)
	{

	if ((aSlot < 0) || (aSlot >= KArgIndex))
		K::PanicKernExec(EParameterSlotRange);

	if (aLen < 0)
		K::PanicKernExec(EParameterSlotDataLength);

	NKern::ThreadEnterCS();
	HBuf8* pBuf = NULL;
	if (aLen)
		pBuf = HBuf8::New(aLen);

	DThread* currentThread = TheCurrentThread;
	currentThread->iTempAlloc = pBuf;
	NKern::ThreadLeaveCS();				

	if (aLen)
		{
		if (!pBuf)
			return KErrNoMemory;
		kumemget((void*)pBuf->Ptr(), aData, aLen);
		pBuf->SetLength(aLen);
		}


	NKern::LockSystem();
	DProcess* pProc = (DProcess*)K::ObjectFromHandle(aProcess, EProcess); 

	if (pProc->iCreatorId != currentThread->iOwningProcess->iId) //check called by creator
		K::LockedPlatformSecurityPanic();

	if (pProc->iEnvironmentData[aSlot] != 0)
		K::PanicCurrentThread(EParameterSlotInUse);

	pProc->iEnvironmentData[aSlot] = (TInt)pBuf | EBinaryData;
	currentThread->iTempAlloc = NULL;
	NKern::UnlockSystem();

	return KErrNone;
	}



//need to have sys locked on way in
TInt ExecHandler::ProcessGetHandleParameter(TInt aSlot, TObjectType aObjectType, TOwnerType aOwnerType)
	{

	DThread * currentThread = TheCurrentThread;
	DProcess * currentProcess = currentThread->iOwningProcess;

	if ((aSlot < 0) || (aSlot >= KArgIndex))
		K::PanicCurrentThread(EParameterSlotRange);

	TInt data = currentProcess->iEnvironmentData[aSlot];

	if (!data)
		return KErrNotFound;

	TProcessParameterType type = (TProcessParameterType)(data&3);

	if (type != EHandle)
		return KErrArgument;

	DObject* pObject = (DObject*)(data&~3);

	if ((TInt)aObjectType+1 != pObject->UniqueID())	//check it's the correct type
		return KErrArgument;

	// zero parameter to prevent race conditions while retrieving the object
	currentProcess->iEnvironmentData[aSlot]=0;
	
	NKern::ThreadEnterCS();
	NKern::UnlockSystem();
	TInt handle = 0;

	TInt ret = currentThread->MakeHandle(aOwnerType, pObject, handle);

	NKern::LockSystem();
	NKern::ThreadLeaveCS();

	if (ret != KErrNone)
		{
		// restore parameter data as 'nothing happened'
		currentProcess->iEnvironmentData[aSlot]=data;
		return ret;
		}
	return handle;
	}


//enter with the system lock held, auto release on exit
TInt ExecHandler::ProcessGetDataParameter(TInt aSlot,  TUint8* aData, TInt aLen)
	{

	DThread * currentThread = TheCurrentThread;
	DProcess * currentProcess = currentThread->iOwningProcess;

	if ((aSlot < 0) || (aSlot >= KArgIndex))
		K::PanicCurrentThread(EParameterSlotRange);

	TInt data = currentProcess->iEnvironmentData[aSlot];
	if (!data)
		return KErrNotFound;

	TProcessParameterType type = (TProcessParameterType)(data&3);
	if (type != EBinaryData)
		return KErrArgument;

	HBuf8* p = (HBuf8*)(data&~3);
	if (!p)	//we've passed zero length binary data so nothing to copy
		return 0;

	if (aLen < p->Length())
		return KErrArgument;

	currentThread->iTempAlloc = p;
	currentProcess->iEnvironmentData[aSlot] = 0;
	NKern::UnlockSystem();

	TInt len = p->Length();
	kumemput((void*)aData, (void*)p->Ptr(), len);

	NKern::ThreadEnterCS();
	currentThread->iTempAlloc = NULL;
	delete p;
	NKern::ThreadLeaveCS();

	NKern::LockSystem();
	return len;
	}

TInt ExecHandler::ProcessDataParameterLength(TInt aSlot)
	{
	__KTRACE_OPT(KEXEC,Kern::Printf("Exec::ProcessDesParameterLength"));

	DProcess * currentProcess = TheCurrentThread->iOwningProcess;

	if ((aSlot < 0) || (aSlot >= KArgIndex))
		K::PanicCurrentThread(EParameterSlotRange);

	TInt data = currentProcess->iEnvironmentData[aSlot];

	if (!data)
		return KErrNotFound;

	TProcessParameterType type = (TProcessParameterType)(data&3);

	if (type != EBinaryData)
		return KErrArgument;

	const HBuf8* p = (const HBuf8*)(data&~3);

	return p ? p->Length() : 0;
	}



void ExecHandler::NotifyChanges(TUint aChanges)
//
// Check that the caller has permission to notify the requested changes,
// and pass it on to Kern::NotifyChanges if it's ok
//
	{
	__KTRACE_OPT(KEXEC,Kern::Printf("Exec::NotifyChanges"));
	
	if (aChanges & ~EChangesLocale)
		K::UnlockedPlatformSecurityPanic();
	
	NKern::ThreadEnterCS();
	Kern::NotifyChanges(aChanges);
	NKern::ThreadLeaveCS();
	}



static TInt GlobalUserData[EMaxGlobalUserData] = {0};

TInt ExecHandler::GetGlobalUserData(TInt aIndex)
	{
	if(TUint(aIndex)<TUint(EMaxGlobalUserData))
		return GlobalUserData[aIndex];
	return 0;
	}

TInt ExecHandler::SetGlobalUserData(TInt aIndex,TInt aValue)
	{
	__KTRACE_OPT(KEXEC,Kern::Printf("Exec::SetGlobalUserData %d 0x%8x",aIndex,aValue));
	if(TUint(aIndex)<TUint(EMaxGlobalUserData))
		{
		if(!Kern::CurrentThreadHasCapability(ECapabilityWriteDeviceData,__PLATSEC_DIAGNOSTIC_STRING("Checked by SetGlobalUserData")))
			return KErrPermissionDenied;
		else
			{
			GlobalUserData[aIndex] = aValue;
			return KErrNone;
			}
		}
	return KErrArgument;
	}

TBool ExecHandler::UserThreadExiting(TInt aReason)
	{
	// Mark this thread as exiting and check whether there are any other threads in the process that
	// are not already exiting
	
	DThread* thread = TheCurrentThread;
	DProcess* process = thread->iOwningProcess;
	
	NKern::ThreadEnterCS();

	// If the thread is process permanent then all other threads in the process will be killed -
	// make sure this happens now, so that this thread has a chance to run global object destructors.
	if (thread->iFlags & KThreadFlagProcessPermanent)
		{
		__NK_ASSERT_ALWAYS(process->WaitProcessLock() == KErrNone);
		NKern::LockSystem();
		process->KillAllThreads(EExitKill, aReason, KNullDesC);
		NKern::UnlockSystem();
		process->SignalProcessLock();
		}

	TBool lastThread = EFalse;
	__NK_ASSERT_DEBUG(thread->iUserThreadState >= DThread::EUserThreadRunning);
	if (thread->iUserThreadState == DThread::EUserThreadRunning)
		{
		thread->iUserThreadState = DThread::EUserThreadExiting;
		lastThread = (__e32_atomic_tas_ord32(&process->iUserThreadsRunning, 1, -1, 0) == 1);
		}
	
	NKern::ThreadLeaveCS();
	
	return lastThread;
	}


#include <kernel/cache.h>

void ExecHandler::IMBRange(TAny* aStart, TUint aSize)
	{
	UNLOCK_USER_MEMORY();
	Cache::IMB_Range((TLinAddr)aStart,aSize);
	LOCK_USER_MEMORY();
	}
