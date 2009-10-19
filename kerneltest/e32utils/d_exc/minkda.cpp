// Copyright (c) 2002-2009 Nokia Corporation and/or its subsidiary(-ies).
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
// e32utils\d_exc\minkda.cpp
// Example of LDD implementing a minimal kernel-side debug agent.
// 
//

#include <kernel/kern_priv.h>
#ifdef __MARM__
#include <arm.h>
#endif
#include "minkda.h"

// Uncomment following lines to enable traces in UREL builds
//#undef __KTRACE_OPT
//#define __KTRACE_OPT(c,b) b

#ifdef _DEBUG
// Panic category used for internal errors
static const char KFault[] = "MINKDA-ERROR, line:";
#endif

// Panic category and codes used when detecting programming error in
// user-side clients.
_LIT(KClientPanic, "MINKDA");
enum TPanic
	{
	EPanicTrapWhileRequestPending,
	EPanicNoCrashedThread,
	EPanicUnsupportedRequest,
	};

// As this LDD allows to bypass platform security, we need to restrict
// access to a few trusted clients.
const TUint32 KDexecSid = 0x101F7770;

//////////////////////////////////////////////////////////////////////////////

//
// Callback invoked on thread panic/exception and associated state.
//

class DCrashHandler : public DKernelEventHandler
	{
public:
	// construction & destruction
	inline DCrashHandler();
	TInt Create(DLogicalDevice* aDevice);
	~DCrashHandler();
public:
	void Trap(TRequestStatus* aRs, TAny* aCrashInfo);
	void CancelTrap();
	void KillCrashedThread();
private:
	static TUint EventHandler(TKernelEvent aEvent, TAny* a1, TAny* a2, TAny* aThis);
	void HandleCrash(TAny* aContext);
	void GetCpuExcInfo(const TAny* aContext, TDbgCpuExcInfo& aInfo);
private:
	DMutex* iHandlerMutex;		// serialise access to crash handler
	NFastSemaphore iSuspendSem; // for suspending crashed thread
	DMutex* iDataMutex;			// serialise access to following members
	DThread* iClient;			// client to signal on crash or NULL
	TRequestStatus* iTrapRq;	// signalled on crash (NULL if none)
	TAny* iCrashInfo;			// user-side buffer filled when crash trapped
	DThread* iCrashedThread;	// thread which took exception (NULL if none)
	DLogicalDevice* iDevice;	// open reference to LDD for avoiding lifetime issues
	};

inline DCrashHandler::DCrashHandler()
	: DKernelEventHandler(EventHandler, this)
	{
	}

//
// second-phase c'tor.  Called in thread critical section.
//

TInt DCrashHandler::Create(DLogicalDevice* aDevice)
	{
	TInt r;
	r = aDevice->Open();
	if (r != KErrNone)
		return r;
	iDevice = aDevice;
	_LIT(KHandlerMutexName, "CtHandlerMutex");
	r = Kern::MutexCreate(iHandlerMutex, KHandlerMutexName, KMutexOrdDebug);
	if (r != KErrNone)
		return r;
	_LIT(KDataMutexName, "CtDataMutex");
	r = Kern::MutexCreate(iDataMutex, KDataMutexName, KMutexOrdDebug-1);
	if (r != KErrNone)
		return r;
	return Add();
	}


//
// Called when reference count reaches zero.  At that point no threads
// are in the handler anymore and the handler has been removed from
// the queue.
//

DCrashHandler::~DCrashHandler()
	{
	__KTRACE_OPT(KDEBUGGER, Kern::Printf("DCrashHandler::~DCrashHandler"));
	if (iDataMutex)
		iDataMutex->Close(NULL);
	if (iHandlerMutex)
		iHandlerMutex->Close(NULL);
	if (iDevice)
		iDevice->Close(NULL);
	}

inline TBool TookException(const DThread* aThread)
	{
    return aThread->iExitType == EExitPanic && 
			aThread->iExitReason == ECausedException && 
			aThread->iExitCategory == KLitKernExec;
	}

//
// Called by kernel when various kinds of events occur.  In thread critical
// section.
//

TUint DCrashHandler::EventHandler(TKernelEvent aEvent, TAny* a1, TAny* /*a2*/, TAny* aThis)
	{
	DThread* pC = &Kern::CurrentThread();
	switch (aEvent)
		{
	case EEventHwExc:
		((DCrashHandler*)aThis)->HandleCrash(a1);
		break;
	case EEventKillThread:
		if (pC->iExitType == EExitPanic && ! TookException(pC))
			((DCrashHandler*)aThis)->HandleCrash(NULL);
		break;
	default:
		// ignore other events
		break;
		}
	return ERunNext;
	}

//
// Called when an exception or panic occurs in context of thread which
// took the exception/panicked.  In thread critical section.
//

void DCrashHandler::HandleCrash(TAny* aContext)
	{
	DThread* pC = &Kern::CurrentThread();
	__KTRACE_OPT(KDEBUGGER, Kern::Printf("HandleCrash context=0x%08X thread=%O", aContext, pC));

	// Quick exit if crashed thread is debugger (i.e. client thread
	// which issued trap request).
	if (pC == iClient)
		{
		__KTRACE_OPT(KDEBUGGER, Kern::Printf("ignoring debugger crash"));
		return;
		}

	// Set realtime state to off to allow us to write to possibly paged debugger thread.  This is
	// reasonable as this thread has already crashed.
	Kern::SetRealtimeState(ERealtimeStateOff);

	// Ensure that, at any time, at most one thread executes the following
	// code.  This simplifies user-side API.
	Kern::MutexWait(*iHandlerMutex);
	__ASSERT_DEBUG(iCrashedThread == NULL, Kern::Fault(KFault, __LINE__));

	// If there is a pending trap request, store basic information
	// about the panic/exception in user-supplied buffer and
	// freeze the crashed thread so it can be inspected.

	Kern::MutexWait(*iDataMutex);
	if (iTrapRq != NULL)
		{
		iCrashedThread = pC;
		iSuspendSem.iOwningThread = &(iCrashedThread->iNThread);

		TDbgCrashInfo info;
		info.iTid = iCrashedThread->iId;
		if (aContext)
			{
			GetCpuExcInfo(aContext, info.iCpu);
			info.iType = TDbgCrashInfo::EException;
			}
		else
			info.iType = TDbgCrashInfo::EPanic;
		TInt r = Kern::ThreadRawWrite(iClient, iCrashInfo, &info, sizeof(info));
		Kern::RequestComplete(iClient, iTrapRq, r);
		iClient = NULL;
		}
	Kern::MutexSignal(*iDataMutex);

	if (iCrashedThread)
		{
		__KTRACE_OPT(KDEBUGGER, Kern::Printf("freezing crashed thread"));
		NKern::FSWait(&(iSuspendSem));
		__KTRACE_OPT(KDEBUGGER, Kern::Printf("resuming crashed thread"));
		Kern::MutexWait(*iDataMutex);
		// Must protect in case a cancel executes concurrently.
		iCrashedThread = NULL;
		Kern::MutexSignal(*iDataMutex);
		}

	Kern::MutexSignal(*iHandlerMutex);
	}


void DCrashHandler::Trap(TRequestStatus* aRs, TAny* aCrashInfo)
	{
	if (iTrapRq != NULL)
		Kern::PanicCurrentThread(KClientPanic, EPanicTrapWhileRequestPending);
	NKern::ThreadEnterCS();
	Kern::MutexWait(*iDataMutex);
	iClient = &Kern::CurrentThread();
	iCrashInfo = aCrashInfo;
	iTrapRq = aRs;
	Kern::MutexSignal(*iDataMutex);
	NKern::ThreadLeaveCS();
	}


void DCrashHandler::CancelTrap()
	{
	__KTRACE_OPT(KDEBUGGER, Kern::Printf(">DCrashHandler::CancelTrap"));
	NKern::ThreadEnterCS();
	Kern::MutexWait(*iDataMutex);

	__KTRACE_OPT(KDEBUGGER, Kern::Printf("cancel request (0x%08X)", iTrapRq));
	Kern::RequestComplete(iClient, iTrapRq, KErrCancel);
	iClient = NULL;

	if (iCrashedThread != NULL)
		{
		__KTRACE_OPT(KDEBUGGER, Kern::Printf("resume crashed thread"));
		NKern::FSSignal(&(iSuspendSem));
		}

	Kern::MutexSignal(*iDataMutex);
	NKern::ThreadLeaveCS();
	__KTRACE_OPT(KDEBUGGER, Kern::Printf("<DCrashHandler::CancelTrap"));
	}


void DCrashHandler::KillCrashedThread()
	{
	if (iCrashedThread == NULL)
		Kern::PanicCurrentThread(KClientPanic, EPanicNoCrashedThread);
	NKern::FSSignal(&iSuspendSem);
	}


void DCrashHandler::GetCpuExcInfo(const TAny* aContext, TDbgCpuExcInfo& aInfo)
	{
#if defined(__MARM__)
	const TArmExcInfo* pE = (const TArmExcInfo*)aContext;
	aInfo.iFaultPc = pE->iR15;
	aInfo.iFaultAddress = pE->iFaultAddress;
	aInfo.iFaultStatus = pE->iFaultStatus;
	aInfo.iExcCode = (TDbgCpuExcInfo::TExcCode)pE->iExcCode;
	aInfo.iR13Svc = pE->iR13Svc;
	aInfo.iR14Svc = pE->iR14Svc;
	aInfo.iSpsrSvc = pE->iSpsrSvc;
#else
	(void) aContext; // silence warnings
	(void) aInfo;
#endif
	}

//////////////////////////////////////////////////////////////////////////////

//
// Channel initialisation and cleanup.  Dispatcher for user-side
// requests.  Crash-related requests are forwarded to DCrashHandler,
// others are implemented here.
//

class DKdaChannel : public DLogicalChannelBase
	{
public:
	~DKdaChannel();
protected:
	// from DLogicalChannelBase
	virtual TInt DoCreate(TInt aUnit, const TDesC8* anInfo, const TVersion &aVer);
	virtual TInt Request(TInt aFunction, TAny* a1, TAny* a2);
private:
	TInt ReadMem(RMinKda::TReadMemParams* aParams);
	TInt GetThreadInfo(TUint aTid, TAny* aInfo);
	void GetThreadCpuInfo(DThread* aThread, TDbgRegSet& aInfo);
	TInt GetCodeSegs(RMinKda::TCodeSnapshotParams* aParams);
	TInt GetCodeSegInfo(RMinKda::TCodeInfoParams* aParams);
	TInt OpenTempObject(TUint aId, TObjectType aType);
	void CloseTempObject();
private:
	DCrashHandler* iCrashHandler;
	DObject* iTempObj;			// automagically closed if abnormal termination
	};


//
// Called when user-side thread create new channel with LDD.  Called
// in context of that thread, in thread critical section.
//

TInt DKdaChannel::DoCreate(TInt /*aUnit*/, const TDesC8* /*aInfo*/, const TVersion &aVer)
	{
	if (Kern::CurrentThread().iOwningProcess->iS.iSecureId != KDexecSid)
		return KErrPermissionDenied;
	if (! Kern::QueryVersionSupported(KKdaLddVersion(), aVer))
		return KErrNotSupported;

	iCrashHandler = new DCrashHandler;
	if (iCrashHandler == NULL)
		return KErrNoMemory;
	return iCrashHandler->Create(iDevice);
	}


//
// Called when last reference to channel is closed, in context of
// closing thread, in thread critical section.
//

DKdaChannel::~DKdaChannel()
	{
	__KTRACE_OPT(KDEBUGGER, Kern::Printf("DKdaChannel::~DKdaChannel"));
	Kern::SafeClose(iTempObj, NULL);
	if (iCrashHandler)
		{
		iCrashHandler->CancelTrap();
		iCrashHandler->Close();
		}
	}


//
// Request dispatcher. Called in context of requesting thread.
//

TInt DKdaChannel::Request(TInt aFunction, TAny* a1, TAny* a2)
	{
	__KTRACE_OPT(KDEBUGGER, Kern::Printf(">DKdaChannel::Request function=%d a1=0x%08X a2=0x%08X", aFunction, a1, a2));
	
	TInt r = KErrNone;
	switch (aFunction)
		{
	case RMinKda::ETrap:
		iCrashHandler->Trap((TRequestStatus*)a1, a2);
		break;
	case RMinKda::ECancelTrap:
		iCrashHandler->CancelTrap();
		break;
	case RMinKda::EKillCrashedThread:
		iCrashHandler->KillCrashedThread();
		break;
	case RMinKda::EGetThreadInfo:
		r = GetThreadInfo((TUint)a1, a2);
		break;
	case RMinKda::EReadMem:
		r = ReadMem((RMinKda::TReadMemParams*)a1);
		break;
	case RMinKda::EGetCodeSegs:
		r = GetCodeSegs((RMinKda::TCodeSnapshotParams*)a1);
		break;
	case RMinKda::EGetCodeSegInfo:
		r = GetCodeSegInfo((RMinKda::TCodeInfoParams*)a1);
		break;
	default:
		Kern::PanicCurrentThread(KClientPanic, EPanicUnsupportedRequest);
		break;
		}

	__KTRACE_OPT(KDEBUGGER, Kern::Printf("<DKdaChannel::Request r=%d", r));
	return r;
	}


TInt DKdaChannel::ReadMem(RMinKda::TReadMemParams* aParams)
	{
	RMinKda::TReadMemParams params;
	umemget32(&params, aParams, sizeof(params));

	TInt destLen;
	TInt destMax;
	TUint8* destPtr = (TUint8*)Kern::KUDesInfo(*params.iDes, destLen, destMax);

	TInt r = OpenTempObject(params.iTid, EThread);
	if (r == KErrNone)
		{
		r = Kern::ThreadRawRead((DThread*)iTempObj, (TAny*)params.iAddr, destPtr, destMax);

		if (r == KErrNone)
			Kern::KUDesSetLength(*params.iDes, destMax);

		CloseTempObject();
		}

	return r;
	}


TInt DKdaChannel::GetThreadInfo(TUint aTid, TAny* aInfo)
	{
	TInt r = OpenTempObject(aTid, EThread);
	if (r == KErrNone)
		{
		DThread* pT = (DThread*)iTempObj;
		TDbgThreadInfo info;
		pT->FullName(info.iFullName);
		info.iPid = pT->iOwningProcess->iId;
		info.iStackBase = pT->iUserStackRunAddress;
		info.iStackSize = pT->iUserStackSize;
		info.iExitCategory = pT->iExitCategory;
		info.iExitReason = pT->iExitReason;
		GetThreadCpuInfo(pT, info.iCpu);
		umemput32(aInfo, &info, sizeof(info));
		CloseTempObject();
		}
	return r;
	}

// :FIXME: improve API
TInt DKdaChannel::GetCodeSegs(RMinKda::TCodeSnapshotParams* aParams)
	{
	RMinKda::TCodeSnapshotParams params;
	umemget32(&params, aParams, sizeof(params));

	TInt maxcount;
	umemget32(&maxcount, params.iCountPtr, sizeof(maxcount));

	__KTRACE_OPT(KDEBUGGER, Kern::Printf(">DKdaChannel::GetCodeSegs pid=%d maxcount=%d", params.iPid, maxcount));
	
	__ASSERT_DEBUG(! iTempObj, Kern::Fault(KFault, __LINE__));
	TInt r = OpenTempObject(params.iPid, EProcess);
	if (r != KErrNone)
		{
		__KTRACE_OPT(KDEBUGGER, Kern::Printf("<DKdaChannel::GetCodeSegs process not found"));
		return r;
		}
		
	DProcess* pP = (DProcess*)iTempObj;

	Kern::AccessCode();

	SDblQue q;
	TInt actcount = pP->TraverseCodeSegs(&q, NULL, DCodeSeg::EMarkDebug, DProcess::ETraverseFlagAdd);

	CloseTempObject();

	TInt n = Min(actcount, maxcount);
	SDblQueLink* pL = q.iA.iNext;
	r = KErrNone;
	for (TInt i=0; i<n; ++i, pL = pL->iNext)
		{
		DCodeSeg* pS = _LOFF(pL, DCodeSeg, iTempLink);
		XTRAP(r, XT_DEFAULT, umemput32(params.iHandles + i, &pS, sizeof(TAny*)));
		if (r != KErrNone)
			break;
		}

	DCodeSeg::EmptyQueue(q, DCodeSeg::EMarkDebug);

	Kern::EndAccessCode();

	if (r == KErrBadDescriptor)
		Kern::PanicCurrentThread(KLitKernExec, ECausedException);
	umemput32(params.iCountPtr, &actcount, sizeof(actcount));

	__KTRACE_OPT(KDEBUGGER, Kern::Printf("<DKdaChannel::GetCodeSegs actcount=%d", actcount));
	return r;
	}


// :FIXME: improve API
TInt DKdaChannel::GetCodeSegInfo(RMinKda::TCodeInfoParams* aParams)
	{
	RMinKda::TCodeInfoParams params;
	umemget32(&params, aParams, sizeof(params));

	// :FIXME: Currently code segments are always loaded at the same
	// location in every address space.  Consequently we can ignore
	// the PID provided by the client.
	DProcess* pP = NULL;

	TInt r = KErrNotFound;
	TFileName	nameBuffer;
	nameBuffer.Zero();
	Kern::AccessCode();
	DCodeSeg* pS = DCodeSeg::VerifyHandle(params.iHandle);
	if (pS)
		{
		TModuleMemoryInfo mmi;
		r = pS->GetMemoryInfo(mmi, pP);
		if (r == KErrNone)
			{
			params.iCodeBase = mmi.iCodeBase;
			params.iCodeSize = mmi.iCodeSize;
			XTRAP(r, XT_DEFAULT, nameBuffer.Append(*(pS->iFileName)));
			}
		}
	Kern::EndAccessCode();
	Kern::KUDesPut(*(params.iPathPtr), nameBuffer);
	if (r == KErrBadDescriptor)
		Kern::PanicCurrentThread(KLitKernExec, ECausedException);

	if (r == KErrNone)
		umemput32(aParams, &params, sizeof(params));

	return r;
	}

//
// Lookup a thread or process id and open the corresponding object.
//
// The object is stored in DKdaChannel::iTempObj to ensure it will be
// closed even if the client thread terminates unexpectedly.  The
// caller must call CloseTempObject() when it is finished with it.
//

TInt DKdaChannel::OpenTempObject(TUint aId, TObjectType aType)
	{
	__ASSERT_DEBUG(aType == EProcess || aType == EThread, Kern::Fault(KFault, __LINE__));
	__ASSERT_DEBUG(! iTempObj, Kern::Fault(KFault, __LINE__));

	DObjectCon* pC = Kern::Containers()[aType];
	NKern::ThreadEnterCS();
	pC->Wait();
	DObject* tempObj = (aType == EProcess) ? (DObject*)Kern::ProcessFromId(aId) : (DObject*)Kern::ThreadFromId(aId);
	NKern::LockSystem();
	iTempObj = tempObj;
	TInt r = KErrNotFound;
	if (iTempObj)
		r = iTempObj->Open();

	NKern::UnlockSystem();
	pC->Signal();
	NKern::ThreadLeaveCS();
	return r;
	}

void DKdaChannel::CloseTempObject()
	{
	__ASSERT_DEBUG(iTempObj, Kern::Fault(KFault, __LINE__));
	NKern::ThreadEnterCS();
	iTempObj->Close(NULL);
	iTempObj = NULL;
	NKern::ThreadLeaveCS();
	}

#ifdef __MARM__

void DKdaChannel::GetThreadCpuInfo(DThread* aThread, TDbgRegSet& aInfo)
	{
	__ASSERT_DEBUG(aThread != &Kern::CurrentThread(), Kern::Fault(KFault, __LINE__));

	TArmRegSet regSet;
	TUint32 unused;
	NKern::ThreadGetUserContext(&(aThread->iNThread), &regSet, unused);
	aInfo.iRn[0] = regSet.iR0;
	aInfo.iRn[1] = regSet.iR1;
	aInfo.iRn[2] = regSet.iR2;
	aInfo.iRn[3] = regSet.iR3;
	aInfo.iRn[4] = regSet.iR4;
	aInfo.iRn[5] = regSet.iR5;
	aInfo.iRn[6] = regSet.iR6;
	aInfo.iRn[7] = regSet.iR7;
	aInfo.iRn[8] = regSet.iR8;
	aInfo.iRn[9] = regSet.iR9;
	aInfo.iRn[10] = regSet.iR10;
	aInfo.iRn[11] = regSet.iR11;
	aInfo.iRn[12] = regSet.iR12;
	aInfo.iRn[13] = regSet.iR13;
	aInfo.iRn[14] = regSet.iR14;
	aInfo.iRn[15] = regSet.iR15;
	aInfo.iCpsr = regSet.iFlags;
	}

#else

void DKdaChannel::GetThreadCpuInfo(DThread* /*aThread*/, TDbgRegSet& /*aInfo*/)
	{
	}

#endif


//////////////////////////////////////////////////////////////////////////////

class DCtDevice : public DLogicalDevice
	{
public:
	DCtDevice();
	// from DLogicalDevice
	virtual TInt Install();
	virtual void GetCaps(TDes8& aDes) const;
	virtual TInt Create(DLogicalChannelBase*& aChannel);
	};

DCtDevice::DCtDevice()
	{
	// iParseMask = 0;
	// iUnitsMask = 0;
	iVersion = KKdaLddVersion();
	}

TInt DCtDevice::Install()
	{
	return SetName(&KKdaLddName);
	}

void DCtDevice::GetCaps(TDes8& /*aDes*/) const
	{
	}

TInt DCtDevice::Create(DLogicalChannelBase*& aChannel)
	{
	aChannel = new DKdaChannel;
	return (aChannel != NULL) ? KErrNone : KErrNoMemory;
	}

//////////////////////////////////////////////////////////////////////////////

DECLARE_STANDARD_LDD()
	{
	return new DCtDevice;
	}

