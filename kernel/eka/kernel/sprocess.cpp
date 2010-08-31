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
// e32\kernel\sprocess.cpp
// 
//

#include <kernel/kern_priv.h>
#include <e32uid.h>

#define iMState		iWaitLink.iSpare1

_LIT(KDollarLock,"$LOCK");
_LIT(KDllDollarLock,"DLL$LOCK");
_LIT(KLitMain,"Main");
_LIT(KLitKill,"Kill");
_LIT(KLitTerminate,"Terminate");

/********************************************
 * Process
 ********************************************/
DProcess::DProcess()
	: iPriority(EProcPriorityForeground),
	  iExitType((TUint8)EExitPending), iGeneration(1), iFlags(KProcessFlagJustInTime),
	  iDynamicCode(8, _FOFF(SCodeSegEntry, iSeg), 2*256)
	{
	//reserve slot 0 for later use for the command line
	iEnvironmentData[0] = EBinaryData;
	}

DProcess::~DProcess()
	{
	delete iCommandLine;
	//delete any process parameters given to this process when it was created
	TInt i;
	for (i = 0; i < KArgIndex; ++i)
		{
		//if the value is a pointer to a buffer, delete it
		//if its a pointer to an object, close it
		TInt data = iEnvironmentData[i]&~3;

		if (iEnvironmentData[i] & EBinaryData)
			{
			delete (HBuf8*)(data);
			}
		else if (iEnvironmentData[i] & EHandle)
			{
			((DObject*)data)->Close(NULL);
			}
		}

	}

void DProcess::Destruct()
	{
	}

void DProcess::DoAppendName(TDes& aName)
	{
	DObject::DoAppendName(aName);
	aName.Append('[');
	aName.AppendNumFixedWidth(iUids.iUid[2].iUid,EHex,8);
	aName.Append(']');
	aName.AppendNumFixedWidth(iGeneration,EDecimal,4);
	}

TInt NextGeneration(const TDesC& aName, TUid aUid)
//
// Return the next generation number for the process
//
	{

	const TInt KNameSuffixLen = 14;  // Number of chars after root name, see DoAppendName

	__KTRACE_OPT(KPROC,Kern::Printf("DProcess::NextGeneration %S 0x%08x", &aName, aUid.iUid));
	TInt gen=0;
	DObjectCon& processes=*K::Containers[EProcess];
	TKName name;
	processes.Wait();
	for (TInt i = 0 ; i < processes.Count() ; ++i)
		{
		DProcess* pP=(DProcess*)processes[i];
		if (pP->iAccessCount > 0 && aUid.iUid == pP->iUids.iUid[2].iUid && pP->NameBuf())
			{
			pP->Name(name);
			if (name.Length() > KNameSuffixLen)
				{
				TPtrC rootName = name.Left(name.Length() - KNameSuffixLen);
				if (aName.CompareF(rootName) == 0 && pP->iAccessCount > 0)
					{
					if (pP->iGeneration>gen)
						gen=pP->iGeneration;
					}
				}
			}
		}
	processes.Signal();
	__KTRACE_OPT(KPROC,Kern::Printf("DProcess generation %d",gen+1));
	return(gen+1);
	}

__ASSERT_COMPILE(ECapability_Limit<=32);  // Kernel's iCaps caps below assumes this


TInt DProcess::SetPaging(const TProcessCreateInfo& aInfo)
	{// Default implementation that only verifies flags, this virtual method 
	// is overridden in memory models that support paging.
	if (aInfo.iFlags & TProcessCreateInfo::EDataPagingMask == 
		TProcessCreateInfo::EDataPagingMask)
		{
		return KErrCorrupt;	
		}
	return KErrNone;
	}


TInt DProcess::Create(TBool aKernelProcess, TProcessCreateInfo& aInfo, HBuf* aCommandLine)
	{
	__KTRACE_OPT(KBOOT,Kern::Printf("DProcess::Create"));

	TInt r=KErrNone;
	iCommandLine=aCommandLine;	// do this first to prevent memory leak if error occurs
	TPtrC f=aInfo.iFileName.Mid(aInfo.iRootNameOffset,aInfo.iRootNameLength);
	r=SetName(&f);
	if (r!=KErrNone)
		return r;

	// Verify and save any data paging attributes.
	SetPaging(aInfo);

	iUids=aInfo.iUids;
	iS=aInfo.iS;
	iDebugAttributes = (TUint8)aInfo.iDebugAttributes;
	if (!aKernelProcess)
		{
		iPriority=0;
		TInt p=ConvertPriority(aInfo.iPriority);
		if (p<0)
			return KErrArgument;
		iPriority=p;
		iGeneration=NextGeneration(f,iUids.iUid[2]);
		}
	else
		{
		iPriority = EProcPrioritySystemServer3;
		iFlags |= KProcessFlagSystemPermanent|KProcessFlagSystemCritical;
		}
#ifdef BTRACE_THREAD_PRIORITY
	BTrace8(BTrace::EThreadPriority,BTrace::EProcessPriority,this,iPriority);
#endif
	iId = K::NewId();
	iCreatorId = iId;  // Initialise as self for safety because creator has special capabilities
	if(TheSuperPage().KernelConfigFlags() & EKernelConfigPlatSecProcessIsolation)
		{
		if(aInfo.iSecurityZone==KSecurityZoneUnique)
			iSecurityZone=iId;
		else
			iSecurityZone=aInfo.iSecurityZone;
		}
	else
		iSecurityZone=KSecurityZoneLegacyCode;

	r=K::MutexCreate((DMutex*&)iProcessLock, KDollarLock, this, EFalse, KMutexOrdProcessLock);
	__KTRACE_OPT(KPROC,Kern::Printf("Lock mutex created, %d",r));
	if (r!=KErrNone)
		return r;
	if (!aKernelProcess)
		{
		r=K::MutexCreate((DMutex*&)iDllLock, KDllDollarLock, this, EFalse, KMutexOrdUser);
		if (r!=KErrNone)
			return r;
		}
	r=DoCreate(aKernelProcess,aInfo);
	if (r!=KErrNone)
		return r;
	__KTRACE_OPT(KPROC,Kern::Printf("Process attributes %08x",iAttributes));
	iAttributes |= EBeingLoaded;

#ifdef __DEBUGGER_SUPPORT__
	// Send new process notification.  Note that the creator thread can not always 
	// be figured out.  It could have been killed atfer requesting the loader to
	// create the process or the process could be created by a kernel thread.
	NKern::LockSystem();

	DThread* creator = (DThread*)TheCurrentThread->ObjectFromHandle(aInfo.iClientHandle,EThread);

	if (creator && (creator->Open() != KErrNone))
		creator = NULL;

	NKern::UnlockSystem();

	__DEBUG_EVENT2(EEventAddProcess, this, creator);

	if (creator)
		creator->Close(NULL);
#endif

#ifdef BTRACE_THREAD_IDENTIFICATION
	BTrace4(BTrace::EThreadIdentification,BTrace::EProcessCreate,this);
#endif

	iTempCodeSeg=(DCodeSeg*)aInfo.iHandle;
	if (iTempCodeSeg)
		{
		if (iTempCodeSeg->iExeCodeSeg!=iTempCodeSeg)
			return KErrNotSupported;
		iTempCodeSeg->WaitCheckedOpen();
		r=AttachExistingCodeSeg(aInfo);		// doesn't map the code in - Loaded() does this
		if (r!=KErrNone)
			return r;
		}
	else
		{
		iTempCodeSeg=M::NewCodeSeg(aInfo);
		if (!iTempCodeSeg)
			return KErrNoMemory;
		iTempCodeSeg->iExeCodeSeg=iTempCodeSeg;
		r=iTempCodeSeg->Create(aInfo,this);
		if (r!=KErrNone)
			return r;
		iTempCodeSeg->WaitCheckedOpen();
		aInfo.iHandle=iTempCodeSeg;
		}
	if (!aKernelProcess)
		{
		DThread* pT=NULL;
		SStdEpocThreadCreateInfo t;
		t.iType=EThreadUser;
		t.iFunction=(TThreadFunction)aInfo.iFileEntryPoint;	// kernel will call veneer, veneer will call this
		t.iPtr=NULL;
		t.iSupervisorStack=NULL;
		t.iSupervisorStackSize=0;	// zero means use default value
		t.iUserStack=NULL;
		t.iUserStackSize=aInfo.iStackSize;
		t.iInitialThreadPriority=EThrdPriorityNormal;
		t.iName.Set(KLitMain);
		t.iAllocator=NULL;
		t.iHeapInitialSize=aInfo.iHeapSizeMin;
		t.iHeapMaxSize=aInfo.iHeapSizeMax;
		t.iTotalSize = sizeof(t);
		r=NewThread((DThread*&)pT, t, NULL, EOwnerProcess);
		if (r==KErrNone)
			pT->iFlags |= KThreadFlagProcessPermanent|KThreadFlagOriginal;
		iUserThreadsRunning=1;
		}
	if (r!=KErrNone)
		return r;
	if (!aKernelProcess)
		r=K::AddObject(this,EProcess);
	return r;
	}

DCodeSeg* DProcess::CodeSeg()
	{
	return iCodeSeg ? iCodeSeg : iTempCodeSeg;
	}

TInt DProcess::SetPriority(TProcessPriority aPriority)
	{
	TInt p=ConvertPriority(aPriority);
	if (p<0)
		return KErrArgument;
	if (iExitType!=EExitPending)
		return KErrDied;

	__KTRACE_OPT(KPROC,Kern::Printf("Process %O SetPriority(%d)",this,p));
	TInt r=WaitProcessLock();
	__KTRACE_FAIL(r,Kern::Printf("PSP: %d",r));
	if (r!=KErrNone)
		return KErrDied;
#ifdef BTRACE_THREAD_PRIORITY
	BTrace8(BTrace::EThreadPriority,BTrace::EProcessPriority,this,p);
#endif
	NKern::LockSystem();
	iPriority=p;
	SDblQueLink* pLink=iThreadQ.First();
	while (pLink!=&iThreadQ.iA)
		{
		DThread* pT=_LOFF(pLink,DThread,iProcessLink);
		pT->SetThreadPriority(pT->iThreadPriority);
		pLink=pLink->iNext;
		NKern::FlashSystem();
		}
	NKern::UnlockSystem();
	SignalProcessLock();
	return KErrNone;
	}

TInt DProcess::ConvertPriority(TProcessPriority aPriority)
	{
	TInt p=-1;
	switch(aPriority)
		{
		case EPriorityLow: p=EProcPriorityLow; break;
		case EPriorityBackground: p=EProcPriorityBackground; break;
		case EPriorityForeground: p=EProcPriorityForeground; break;
		case EPriorityHigh: p=EProcPriorityHigh; break;
		case EPriorityWindowServer: p=EProcPrioritySystemServer1; break;
		case EPriorityFileServer: p=EProcPrioritySystemServer2; break;
		case EPriorityRealTimeServer: p=EProcPriorityRealTimeServer; break;
		case EPrioritySupervisor: p=EProcPrioritySystemServer3; break;
		}
	return p;
	}

TInt DProcess::Rename(const TDesC& aName)
	{
	if (aName.Length()>KMaxKernelName-KMaxUidName-4)
		return KErrBadName;
	TKName n;
	DObject::BaseName(n); // get current name, without UID and generation number
	if (n.MatchF(aName)==0)
		return KErrNone;		// new name is the same so nothing to do

	iGeneration = NextGeneration(aName, iUids.iUid[2]);

	__KTRACE_OPT(KTHREAD,Kern::Printf("DProcess::Rename %O to %S",this,&aName));
	TInt r = SetName(&aName);
#ifdef BTRACE_THREAD_IDENTIFICATION
	Name(n);
	BTraceN(BTrace::EThreadIdentification,BTrace::EProcessName,0,this,n.Ptr(),n.Size());
#endif

	__COND_DEBUG_EVENT(r==KErrNone, EEventUpdateProcess, this);
	return(r);
	}

void DProcess::Release()
	{
#ifdef __SMP__
	// delete thread group
	if (iSMPUnsafeGroup)
		{
		NKern::GroupDestroy(iSMPUnsafeGroup);
		}
#endif

	// delete handles with lock mutex free
	__KTRACE_OPT(KPROC,Kern::Printf("Process %O Release()",this));
	__KTRACE_OPT(KPROC,Kern::Printf("Deleting handles"));
	iHandles.Close(this);

	// Notify process unloading before code segs are released
	__DEBUG_EVENT(EEventUnloadingProcess, this);

	__NK_ASSERT_DEBUG(iGarbageList.IsEmpty());

	if (iCodeSeg || iTempCodeSeg)
		{
		DCodeSeg::Wait();
		if (iCodeSeg)
			RemoveCodeSeg(iCodeSeg, NULL);
		DCodeSeg* pS = (DCodeSeg*)__e32_atomic_swp_ord_ptr(&iTempCodeSeg, 0);
		if (pS)
			pS->CheckedClose();
		RemoveDllData();
		DCodeSeg::Signal();
		}

	__ASSERT_ALWAYS(iDynamicCode.Count()==0, K::Fault(K::ECodeStillMapped));
	iDynamicCode.Close();

	__KTRACE_OPT(KPROC,Kern::Printf("Closing DLL$LOCK mutex"));
	Kern::SafeClose((DObject*&)iDllLock,this);

	__KTRACE_OPT(KPROC,Kern::Printf("Closing owning process"));
	Kern::SafeClose((DObject*&)iOwningProcess,NULL);

	__KTRACE_OPT(KPROC,Kern::Printf("Closing DataBssStack chunk"));
	Kern::SafeClose((DObject*&)iDataBssStackChunk,this);

	NKern::LockSystem();
	iFinalReleaseFlag = ETrue;
	if (iProcessLock)
		iProcessLock->Wait();			// this will get released when the lock mutex is deleted
	NKern::UnlockSystem();

	__KTRACE_OPT(KPROC,Kern::Printf("Calling FinalRelease()"));
	// Call FinalRelease() with process $LOCK mutex held (if it exists)
	// and don't ever release the mutex afterwards.
	FinalRelease();

	__KTRACE_OPT(KPROC,Kern::Printf("Closing $LOCK mutex"));
	Kern::SafeClose((DObject*&)iProcessLock,this);

	// Notify process removal before closing process
	__DEBUG_EVENT(EEventRemoveProcess, this);

#ifdef BTRACE_THREAD_IDENTIFICATION
	BTrace4(BTrace::EThreadIdentification,BTrace::EProcessDestroy,this);
#endif

	// Kill any processes which we created but have not resumed
	NKern::LockSystem();	// In case we died in the middle of DProcess::Resume()
	NKern::UnlockSystem();
	for(;;)
		{
		DObjectCon& processes=*K::Containers[EProcess];
		processes.Wait();
		DProcess* zombie=NULL;
		TInt c=processes.Count();
		for (TInt i=0; i<c; i++)
			{
			DProcess* pP=(DProcess*)processes[i];
			if (pP->iCreatorId==iId && pP!=this && pP->Open()==KErrNone)
				{
				zombie = pP;
				break;
				}
			}
		processes.Signal();
		if(!zombie)
			break;
		__KTRACE_OPT(KPROC,Kern::Printf("Killing zombie process %O",zombie));
		zombie->iCreatorId=zombie->iId;
		zombie->Die(EExitPanic,EZombieProcessKilled,KLitKernExec());
		zombie->Close(0);
		}

	__KTRACE_OPT(KPROC,Kern::Printf("Closing process"));
	Close(this);
	}

TInt DProcess::WaitProcessLock()
	{
	TInt r=KErrGeneral;
	NKern::LockSystem();
	if (iProcessLock && !iFinalReleaseFlag)				// iProcessLock is deleted during process death
		{
		r=iProcessLock->Wait();							// mutex may be resetting just before deletion
		__KTRACE_FAIL(r,Kern::Printf("PLW: %d",r));		// will return KErrGeneral if mutex is reset
		}
	NKern::UnlockSystem();
	__KTRACE_FAIL(r,Kern::Printf("WPL: %d",r));
	return r;
	}

TInt DProcess::SignalProcessLock()
	{
	TInt r=KErrDied;
	NKern::LockSystem();
	if (iProcessLock)
		{
		iProcessLock->Signal();
		r=KErrNone;
		}
	else
		NKern::UnlockSystem();
	__KTRACE_FAIL(r,Kern::Printf("SPL: %d",r));
	return r;
	}

TInt DProcess::Loaded(TProcessCreateInfo& aInfo)
//
// Confirm that the process has been loaded O.K.
//
	{
	__KTRACE_OPT(KPROC,Kern::Printf("DProcess::Loaded"));

	//
	// Update our code segment and dependents
	//
	DCodeSeg::Wait();

	TInt r = KErrNone;

	if (!(iTempCodeSeg->iMark & DCodeSeg::EMarkLoaded))
		{
		// newly loaded code segment, not another instance of same
		r = iTempCodeSeg->Loaded(aInfo);
		}

	if (r == KErrNone)
		r = OpenDeps();

	DCodeSeg::Signal();

	if (r != KErrNone)
		return r;

	if (iCodeSeg->iAttr & ECodeSegAttSMPSafe)
		iSMPUnsafeCount = 0;
	else
		{
		iSMPUnsafeCount = 1;
#ifdef __SMP__
		r = UpdateSMPSafe();
		if (r != KErrNone)
			return r;
#endif
		}
	FirstThread()->iMState=DThread::EReady;
	SetProtection(DObject::EGlobal);
	iAttributes &= ~EBeingLoaded;
	iReentryPoint = iCodeSeg->iEntryPtVeneer;

	// Send loaded process notification.
	__DEBUG_EVENT(EEventLoadedProcess, this);

	return KErrNone;
	}

void DProcess::Resume()
//
// Resume a newly created process. Idempotent. Enter and return with system locked.
//
	{
	if (iAttributes&EBeingLoaded)
		K::PanicCurrentThread(EProcessNotLoaded);
	if (!(iAttributes&EResumed))
		{
		iAttributes|=EResumed;
		iCreatorId = iId;	// Creator loses control over process once it has been resumed
		FirstThread()->Resume();
		}
	}

void DProcess::Die(TExitType aType, TInt aReason, const TDesC &aCategory)
//
// Kill a process. Enter and return with system unlocked and calling thread in critical section.
//
	{
	__KTRACE_OPT(KPROC,Kern::Printf("Process %O Die: %d %d %S",this,aType,aReason,&aCategory));

	TInt r=WaitProcessLock();
	if (r!=KErrNone)
		return;			// process already exiting
	NKern::LockSystem();
	if (iAttributes&EBeingLoaded)
		{
		// Load failed before UserSvr::ProcessLoaded() was called.
		// The thread is still runnable however, since UserSvr::ProcessCreate() must have succeeded.
		// We just need to set the thread state so it can be resumed.
		FirstThread()->iMState=DThread::EReady;
		}
	if (iExitType==EExitPending)
		{
		iExitType = (TUint8)aType;
		iExitReason=aReason;
		if (iExitType==EExitKill)
			iExitCategory=KLitKill;
		else if (iExitType==EExitTerminate)
			iExitCategory=KLitTerminate;
		else
			iExitCategory=aCategory;

		if (iExitType!=EExitKill && (iFlags & (KProcessFlagSystemPermanent|KProcessFlagSystemCritical)))
			K::Fault(K::ESystemProcessPanic);
		if (iFlags & KProcessFlagSystemPermanent)
			K::Fault(K::EPermanentProcessExit);

		// Kill all threads; when the last one exits the process will be cleaned up.
		TBool killMe = KillAllThreads(aType, aReason, aCategory);
		if (killMe)
			{
			DThread* pC=TheCurrentThread;
			pC->iExitType=(TUint8)iExitType;
			pC->iExitReason=iExitReason;
			pC->iExitCategory=iExitCategory;
			NKern::DeferredExit();
			}
		}
	NKern::UnlockSystem();
	SignalProcessLock();
	}

TBool DProcess::KillAllThreads(TExitType aType, TInt aReason, const TDesC &aCategory)
//
// Kill all threads in a process.  Enter and return with system locked, the process lock held and
// calling thread in a critical section.  Returns whether the current thread needs to exit too.
//
	{
	TBool killMe=EFalse;
	DThread* pC=TheCurrentThread;
	SDblQueLink* pLink=iThreadQ.First();
	while (pLink!=&iThreadQ.iA)
		{
		DThread* pT=_LOFF(pLink,DThread,iProcessLink);
		pLink=pLink->iNext;
		if (pT!=pC)
			{
			// If killing pT will cause a system crash then force that to happen in the context of the
			// current thread
			if(pT->iFlags & KThreadFlagSystemPermanent)
				{
				K::Fault(K::EPermanentThreadExit);
				}
			if (aType != EExitKill && (pT->iFlags & KThreadFlagSystemCritical))
				{
				K::Fault(K::ESystemThreadPanic);
				}
			// Need to stop the current thread being killed as a consequence of killing pT
			pT->iFlags &= ~(KThreadFlagProcessPermanent|KThreadFlagProcessCritical);
			pT->Die(aType, aReason, aCategory);
			}
		else
			{
			killMe=ETrue;
			NKern::UnlockSystem();
			}
		NKern::LockSystem();
		}
	return killMe;
	}

void DProcess::AddThread(DThread &aThread)
	{
	__KTRACE_OPT(KPROC,Kern::Printf("AddThread %O to %O",&aThread,this));
	iThreadQ.Add(&aThread.iProcessLink);
	}

TInt DProcess::NewThread(DThread*& aThread, SThreadCreateInfo& anInfo, TInt* aHandle, TOwnerType aType)
	{
	__KTRACE_OPT(KTHREAD,Kern::Printf("NewThread proc %O, func %08x ptr %08x",this,anInfo.iFunction,anInfo.iPtr));
	__KTRACE_OPT(KTHREAD,Kern::Printf("type %d name %S pri %d",anInfo.iType,&anInfo.iName,anInfo.iInitialThreadPriority));
	if (aHandle)
		*aHandle=0;
	TInt r=GetNewThread(aThread,anInfo);
	__KTRACE_FAIL(r,Kern::Printf("GNT: %d",r));
	DThread* pT=aThread;
	if (r==KErrNone)
		{
		r=pT->Create(anInfo);
		__KTRACE_FAIL(r,Kern::Printf("NTC: %d",r));
		if (r==KErrNone && aHandle)
			{
			r=K::MakeHandleAndOpen(aType,pT,*aHandle);
			__KTRACE_FAIL(r,Kern::Printf("NT MHO: %d",r));
			}
		if (r==KErrNone)
			{
			r=WaitProcessLock();
			if (r==KErrNone)
				{
				NKern::LockSystem();
				if (iExitType==EExitPending)
					{
					AddThread(*pT);
					pT->iMState=DThread::EReady;
					// set fully constructed flag here
					}
				else
					r=KErrDied;
				NKern::UnlockSystem();
				SignalProcessLock();
				__KTRACE_FAIL(r,Kern::Printf("NT ADD: %d",r));
				}
			else
				r=KErrDied;
			}
		}
	if (r==KErrNone)
		{
		if (anInfo.iType == EThreadUser)
			{
			pT->iUserThreadState = DThread::EUserThreadCreated;
			__e32_atomic_tas_ord32(&iUserThreadsRunning, 1, 1, 0);
			}	
#ifdef BTRACE_THREAD_IDENTIFICATION
		if(BTrace::Filter(BTrace::EThreadIdentification))
			{
			DObjectCon* threads=Kern::Containers()[EThread];
			threads->Wait(); // hold mutex so traces below don't get mixed up with other thread creation traces
			TKName nameBuf;
			Name(nameBuf);
			BTraceN(BTrace::EThreadIdentification,BTrace::EProcessName,&pT->iNThread,this,nameBuf.Ptr(),nameBuf.Size());
			pT->Name(nameBuf);
			BTraceN(BTrace::EThreadIdentification,BTrace::EThreadCreate,&pT->iNThread,this,nameBuf.Ptr(),nameBuf.Size());
			BTrace12(BTrace::EThreadIdentification,BTrace::EThreadId,&pT->iNThread,this,pT->iId);
			threads->Signal();
			}
#endif
		__DEBUG_EVENT2(EEventAddThread, pT, TheCurrentThread);
		}
	else if (pT)
		{
		if (aHandle && *aHandle)
			{
			K::HandleClose(*aHandle);
			*aHandle=0;
			}
		pT->Stillborn();
		aThread=NULL;
		}
	__KTRACE_FAIL(r,Kern::Printf("NT: %d",r));
	return r;
	}

void DProcess::Rendezvous(TInt aReason)
//
// Enter and return with system unlocked and calling thread in critical section.
//
	{
	TLogon::CompleteAll(iTargetLogons, TLogon::ETargetRendezvous, aReason);
	}

TInt DProcess::Logon(TRequestStatus* aStatus, TBool aRendezvous)
	{
	TInt r = KErrNoMemory;
	DThread* pC = TheCurrentThread;
	__KTRACE_OPT(KTHREAD, Kern::Printf("Thread %O Logon to process %O, status at %08x rdv=%x",
		     pC, this, aStatus, aRendezvous));

	TLogon* pL = new TLogon;
	if (pL)
		{
		TUint32 type = TLogon::ETargetProcess;
		if (aRendezvous)
			type |= TLogon::ERendezvous;
		r = pL->Attach(iTargetLogons, pC, this, aStatus, type);
		if (r != KErrNone)
			pL->Close();
		}

	__KTRACE_OPT(KTHREAD, Kern::Printf("DProcess::Logon ret %d", r));
	return r;
	}

void DProcess::BTracePrime(TInt aCategory)
	{
#ifdef BTRACE_THREAD_IDENTIFICATION
	if(aCategory==BTrace::EThreadIdentification || aCategory==-1)
		BTrace4(BTrace::EThreadIdentification,BTrace::EProcessCreate,this);
#endif
	}

#ifdef __SMP__
void SMPUnsafeGroupDestroyFn(TAny* aGroup)
	{
	Kern::Free(aGroup);
	}

TInt DProcess::UpdateSMPSafe()
	{
	TUint32 config = TheSuperPage().KernelConfigFlags();
	if (!(config & (EKernelConfigSMPUnsafeCompat|EKernelConfigSMPUnsafeCPU0)) || this == K::TheKernelProcess)
		return KErrNone;
	__KTRACE_OPT(KPROC,Kern::Printf("Process %O UpdateSMPSafe count=%d",this,iSMPUnsafeCount));
	TInt r=WaitProcessLock();
	(void)r;
	__NK_ASSERT_DEBUG(r==KErrNone);
	if ((config & EKernelConfigSMPUnsafeCompat) && !iSMPUnsafeGroup)
		{
		SNThreadGroupCreateInfo info;
		info.iCpuAffinity = KCpuAffinityAny;
		NThreadGroup* g = (NThreadGroup*)Kern::Alloc(sizeof(NThreadGroup));
		r = KErrNoMemory;
		if (g)
			{
			info.iDestructionDfc = new TDfc(&SMPUnsafeGroupDestroyFn, g, K::SvMsgQ, 2);
			if (info.iDestructionDfc)
				r = NKern::GroupCreate(g, info);
			if (r != KErrNone)
				{
				delete info.iDestructionDfc;
				Kern::Free(g);
				g = 0;
				}
			iSMPUnsafeGroup = g;
			}
		}
	if (r==KErrNone)
		{
		SDblQueLink* pLink=iThreadQ.First();
		while (pLink!=&iThreadQ.iA)
			{
			DThread* pT=_LOFF(pLink,DThread,iProcessLink);
			NKern::QueueUserModeCallback(&pT->iNThread, &pT->iSMPSafeCallback);
			pLink=pLink->iNext;
			}
		}
	SignalProcessLock();
	return r;
	}
#endif

