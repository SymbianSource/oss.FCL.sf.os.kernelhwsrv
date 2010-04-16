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
// e32\nkernsmp\x86\ncthrd.cpp
// 
//

// NThreadBase member data
#define __INCLUDE_NTHREADBASE_DEFINES__

#include <x86.h>
#include <apic.h>
#include <nk_irq.h>

// Called by a thread when it first runs
void __StartThread();

void NThreadBase::OnKill()
	{
	}

void NThreadBase::OnExit()
	{
	}

extern void __ltr(TInt /*aSelector*/);

extern "C" TUint __tr();
extern void InitTimestamp(TSubScheduler* aSS, SNThreadCreateInfo& aInfo);

TInt NThread::Create(SNThreadCreateInfo& aInfo, TBool aInitial)
	{
	if (!aInfo.iStackBase || aInfo.iStackSize<0x100)
		return KErrArgument;
	new (this) NThread;
	TInt cpu = -1;
	TSubScheduler* ss = 0;
	if (aInitial)
		{
		cpu = __e32_atomic_add_ord32(&TheScheduler.iNumCpus, 1);
		if (cpu==0)
			memset(SubSchedulerLookupTable, 0x9a, sizeof(SubSchedulerLookupTable));
		aInfo.iCpuAffinity = cpu;
		// OK since we can't migrate yet
		TUint32 apicid = *(volatile TUint32*)(X86_LOCAL_APIC_BASE + X86_LOCAL_APIC_OFFSET_ID) >> 24;
		ss = &TheSubSchedulers[cpu];
		ss->iSSX.iAPICID = apicid << 24;
		ss->iCurrentThread = this;
		ss->iDeferShutdown = 0;
		SubSchedulerLookupTable[apicid] = ss;
		iRunCount.i64 = UI64LIT(1);
		iActiveState = 1;
		__KTRACE_OPT(KBOOT,DEBUGPRINT("Init: cpu=%d APICID=%08x ss=%08x", cpu, apicid, ss));
		if (cpu)
			{
			__ltr(TSS_SELECTOR(cpu));
			NIrq::HwInit2AP();
			__e32_atomic_ior_ord32(&TheScheduler.iThreadAcceptCpus, 1<<cpu);
			__e32_atomic_ior_ord32(&TheScheduler.iIpiAcceptCpus, 1<<cpu);
			__e32_atomic_ior_ord32(&TheScheduler.iCpusNotIdle, 1<<cpu);
			__e32_atomic_add_ord32(&TheScheduler.iCCRequestLevel, 1);
			__KTRACE_OPT(KBOOT,DEBUGPRINT("AP TR=%x",__tr()));
			}
		}
	TInt r=NThreadBase::Create(aInfo,aInitial);
	if (r!=KErrNone)
		return r;
	if (!aInitial)
		{
		TLinAddr stack_top = (TLinAddr)iStackBase + (TLinAddr)iStackSize;
		TLinAddr sp = stack_top;
		TUint32 pb = (TUint32)aInfo.iParameterBlock;
		SThreadStackStub* tss = 0;
		if (aInfo.iParameterBlockSize)
			{
			tss = (SThreadStackStub*)stack_top;
			--tss;
			tss->iVector = SThreadStackStub::EVector;
			tss->iError = 0;
			tss->iEip = 0;
			tss->iCs = 0;
			tss->iEflags = 0;
			sp = (TLinAddr)tss;
			sp -= (TLinAddr)aInfo.iParameterBlockSize;
			wordmove((TAny*)sp, aInfo.iParameterBlock, aInfo.iParameterBlockSize);
			pb = (TUint32)sp;
			tss->iPBlock = sp;
			}
		SThreadInitStack* tis = (SThreadInitStack*)sp;
		--tis;
		tis->iR.iCR0 = X86::DefaultCR0 | KX86CR0_TS;
		tis->iR.iReschedFlag = 1;
		tis->iR.iEip = (TUint32)&__StartThread;
		tis->iR.iReason = 0;
		tis->iX.iEcx = 0;
		tis->iX.iEdx = 0;
		tis->iX.iEbx = pb;		// parameter block pointer
		tis->iX.iEsi = 0;
		tis->iX.iEdi = 0;
		tis->iX.iEbp = stack_top;
		tis->iX.iEax = (TUint32)aInfo.iFunction;
		tis->iX.iDs = KRing0DS;
		tis->iX.iEs = KRing0DS;
		tis->iX.iFs = 0;
		tis->iX.iGs = KRing0DS;
		tis->iX.iVector = SThreadInitStack::EVector;
		tis->iX.iError = 0;
		tis->iX.iEip = (TUint32)aInfo.iFunction;
		tis->iX.iCs = KRing0CS;
		tis->iX.iEflags = (TUint32)(EX86FlagIF|EX86FlagAC|0x1002);
		tis->iX.iEsp3 = 0xFFFFFFFFu;
		tis->iX.iSs3 = 0xFFFFFFFFu;
		wordmove(&iCoprocessorState, DefaultCoprocessorState, sizeof(iCoprocessorState));
		iSavedSP = (TLinAddr)tis;
		}
	else
		{
		NKern::EnableAllInterrupts();

		// Initialise timestamp
		InitTimestamp(ss, aInfo);
		}
	AddToEnumerateList();
	InitLbInfo();
#ifdef BTRACE_THREAD_IDENTIFICATION
	BTrace4(BTrace::EThreadIdentification,BTrace::ENanoThreadCreate,this);
#endif
	return KErrNone;
	}

void DumpExcInfo(TX86ExcInfo& a)
	{
	DEBUGPRINT("Exc %02x EFLAGS=%08x FAR=%08x ErrCode=%08x",a.iExcId,a.iEflags,a.iFaultAddress,a.iExcErrorCode);
	DEBUGPRINT("EAX=%08x EBX=%08x ECX=%08x EDX=%08x",a.iEax,a.iEbx,a.iEcx,a.iEdx);
	DEBUGPRINT("ESP=%08x EBP=%08x ESI=%08x EDI=%08x",a.iEsp,a.iEbp,a.iEsi,a.iEdi);
	DEBUGPRINT(" CS=%08x EIP=%08x  DS=%08x  SS=%08x",a.iCs,a.iEip,a.iDs,a.iSs);
	DEBUGPRINT(" ES=%08x  FS=%08x  GS=%08x",a.iEs,a.iFs,a.iGs);
	if (a.iCs&3)
		{
		DEBUGPRINT("SS3=%08x ESP3=%08x",a.iSs3,a.iEsp3);
		}
	TScheduler& s = TheScheduler;
	TInt irq = NKern::DisableAllInterrupts();
	TSubScheduler& ss = SubScheduler();
	NThreadBase* ct = ss.iCurrentThread;
	TInt inc = TInt(ss.iSSX.iIrqNestCount);
	TInt cpu = ss.iCpuNum;
	NKern::RestoreInterrupts(irq);
	DEBUGPRINT("Thread %T, CPU %d, KLCount=%08x, IrqNest=%d",ct,cpu,ss.iKernLockCount,inc);
	}


void GetContextAfterExc(TX86RegSet& aContext, SThreadExcStack* txs, TUint32& aAvailRegistersMask, TBool aSystem)
	{
	TInt cpl = txs->iCs & 3;
	aAvailRegistersMask = 0xffffu;	// EAX,EBX,ECX,EDX,ESP,EBP,ESI,EDI,CS,DS,ES,FS,GS,SS,EFLAGS,EIP all valid
	aContext.iEax = txs->iEax;
	aContext.iEbx = txs->iEbx;
	aContext.iEcx = txs->iEcx;
	aContext.iEdx = txs->iEdx;
	if (aSystem)
		{
		aContext.iEsp = TUint32(txs+1);
		if (cpl==0)
			aContext.iEsp -= 8;		// two less words pushed if interrupt taken while CPL=0
		aContext.iSs = KRing0DS;
		aAvailRegistersMask &= ~0x2000u;	// SS assumed not read
		}
	else if (cpl==3)
		{
		aContext.iEsp = txs->iEsp3;
		aContext.iSs = txs->iSs3;
		}
	else
		{
		__crash();
		}
	aContext.iEbp = txs->iEbp;
	aContext.iEsi = txs->iEsi;
	aContext.iEdi = txs->iEdi;
	aContext.iCs = txs->iCs;
	aContext.iDs = txs->iDs;
	aContext.iEs = txs->iEs;
	aContext.iFs = txs->iFs;
	aContext.iGs = txs->iGs;
	aContext.iEflags = txs->iEflags;
	aContext.iEip = txs->iEip;
	}

void GetContextAfterSlowExec(TX86RegSet& aContext, SThreadSlowExecStack* tsxs, TUint32& aAvailRegistersMask)
	{
	TInt cpl = tsxs->iCs & 3;
	if (cpl!=3)
		{
		__crash();
		}
	aAvailRegistersMask = 0xffffu;	// EAX,EBX,ECX,EDX,ESP,EBP,ESI,EDI,CS,DS,ES,FS,GS,SS,EFLAGS,EIP all valid
	aContext.iEax = tsxs->iEax;
	aContext.iEbx = tsxs->iEbx;
	aContext.iEcx = tsxs->iEcx;
	aContext.iEdx = tsxs->iEdx;
	aContext.iEsp = tsxs->iEsp3;
	aContext.iSs = tsxs->iSs3;
	aContext.iEbp = tsxs->iEbp;
	aContext.iEsi = tsxs->iEsi;
	aContext.iEdi = tsxs->iEdi;
	aContext.iCs = tsxs->iCs;
	aContext.iDs = tsxs->iDs;
	aContext.iEs = tsxs->iEs;
	aContext.iFs = tsxs->iFs;
	aContext.iGs = tsxs->iGs;
	aContext.iEflags = tsxs->iEflags;
	aContext.iEip = tsxs->iEip;
	}


// Enter and return with kernel locked
void NThread::GetUserContext(TX86RegSet& aContext, TUint32& aAvailRegistersMask)
	{
	NThread* pC = NCurrentThreadL();
	TSubScheduler* ss = 0;
	if (pC != this)
		{
		AcqSLock();
		if (iWaitState.ThreadIsDead() || i_NThread_Initial)
			{
			RelSLock();
			aAvailRegistersMask = 0;
			return;
			}
		if (iReady && iParent->iReady)
			{
			ss = TheSubSchedulers + (iParent->iReady & EReadyCpuMask);
			ss->iReadyListLock.LockOnly();
			}
		if (iCurrent)
			{
			// thread is actually running on another CPU
			// interrupt that CPU and wait for it to enter interrupt mode
			// this allows a snapshot of the thread user state to be observed
			// and ensures the thread cannot return to user mode
			send_resched_ipi_and_wait(iLastCpu);
			}
		}
	TUint32* stack = (TUint32*)(TLinAddr(iStackBase) + TLinAddr(iStackSize));
	if (stack[-1]!=0xFFFFFFFFu && stack[-2]!=0xFFFFFFFFu && stack[-7]<0x100u)	// if not, thread never entered user mode
		{
		if (stack[-7] == 0x21)	// slow exec
			GetContextAfterSlowExec(aContext, ((SThreadSlowExecStack*)stack)-1, aAvailRegistersMask);
		else
			GetContextAfterExc(aContext, ((SThreadExcStack*)stack)-1, aAvailRegistersMask, FALSE);
		}
	if (pC != this)
		{
		if (ss)
			ss->iReadyListLock.UnlockOnly();
		RelSLock();
		}
	}

class TGetContextIPI : public TGenericIPI
	{
public:
	void Get(TInt aCpu, TX86RegSet& aContext, TUint32& aAvailRegistersMask);
	static void Isr(TGenericIPI*);
public:
	TX86RegSet* iContext;
	TUint32* iAvailRegsMask;
	};

void TGetContextIPI::Isr(TGenericIPI* aPtr)
	{
	TGetContextIPI& ipi = *(TGetContextIPI*)aPtr;
	TX86RegSet& a = *ipi.iContext;
	TSubScheduler& ss = SubScheduler();
	TUint32* irqstack = (TUint32*)ss.iSSX.iIrqStackTop;
	SThreadExcStack* txs = (SThreadExcStack*)irqstack[-1];	// first word pushed on IRQ stack points to thread supervisor stack
	GetContextAfterExc(a, txs, *ipi.iAvailRegsMask, TRUE);
	}

void TGetContextIPI::Get(TInt aCpu, TX86RegSet& aContext, TUint32& aAvailRegsMask)
	{
	iContext = &aContext;
	iAvailRegsMask = &aAvailRegsMask;
	Queue(&Isr, 1u<<aCpu);
	WaitCompletion();
	}

// Enter and return with kernel locked
void NThread::GetSystemContext(TX86RegSet& aContext, TUint32& aAvailRegsMask)
	{
	aAvailRegsMask = 0;
	NThread* pC = NCurrentThreadL();
	__NK_ASSERT_ALWAYS(pC!=this);
	TSubScheduler* ss = 0;
	AcqSLock();
	if (iWaitState.ThreadIsDead())
		{
		RelSLock();
		return;
		}
	if (iReady && iParent->iReady)
		{
		ss = TheSubSchedulers + (iParent->iReady & EReadyCpuMask);
		ss->iReadyListLock.LockOnly();
		}
	if (iCurrent)
		{
		// thread is actually running on another CPU
		// use an interprocessor interrupt to get a snapshot of the state
		TGetContextIPI ipi;
		ipi.Get(iLastCpu, aContext, aAvailRegsMask);
		}
	else
		{
		// thread is not running and can't start
		SThreadReschedStack* trs = (SThreadReschedStack*)iSavedSP;
		TUint32 kct = trs->iReason;
		TLinAddr sp = TLinAddr(trs+1);
		TUint32* stack = (TUint32*)sp;
		switch (kct)
			{
			case 0:	// thread not yet started
				{
				aContext.iEcx = stack[0];
				aContext.iEdx = stack[1];
				aContext.iEbx = stack[2];
				aContext.iEsi = stack[3];
				aContext.iEdi = stack[4];
				aContext.iEbp = stack[5];
				aContext.iEax = stack[6];
				aContext.iDs = stack[7];
				aContext.iEs = stack[8];
				aContext.iFs = stack[9];
				aContext.iGs = stack[10];
				aContext.iEsp = sp + 40 - 8;	// entry to initial function
				aContext.iEip = aContext.iEax;
				aContext.iEflags = 0x41202;		// guess
				aContext.iCs = KRing0CS;
				aContext.iSs = KRing0DS;
				aAvailRegsMask = 0x9effu;
				break;
				}
			case 1:	// unlock
				{
				aContext.iFs = stack[0];
				aContext.iGs = stack[1];
				aContext.iEbx = stack[2];
				aContext.iEbp = stack[3];
				aContext.iEdi = stack[4];
				aContext.iEsi = stack[5];
				aContext.iEip = stack[6];	// return address from NKern::Unlock()
				aContext.iCs = KRing0CS;
				aContext.iDs = KRing0DS;
				aContext.iEs = KRing0DS;
				aContext.iSs = KRing0DS;
				aContext.iEsp = sp + 28;	// ESP after return from NKern::Unlock()
				aContext.iEax = 0;	// unknown
				aContext.iEcx = 0;	// unknown
				aContext.iEdx = 0;	// unknown
				aContext.iEflags = 0x41202;	// guess
				aAvailRegsMask =0x98f2u;	// EIP,GS,FS,EDI,ESI,EBP,ESP,EBX available, others guessed or unavailable
				break;
				}
			case 2:	// IRQ
				{
				GetContextAfterExc(aContext, (SThreadExcStack*)sp, aAvailRegsMask, TRUE);
				break;
				}
			default:	// unknown reschedule reason
				__NK_ASSERT_ALWAYS(0);
			}
		}
	if (ss)
		ss->iReadyListLock.UnlockOnly();
	RelSLock();
	}

// Enter and return with kernel locked
void NThread::SetUserContext(const TX86RegSet& aContext, TUint32& aRegMask)
	{
	NThread* pC = NCurrentThreadL();
	TSubScheduler* ss = 0;
	if (pC != this)
		{
		AcqSLock();
		if (iWaitState.ThreadIsDead() || i_NThread_Initial)
			{
			RelSLock();
			aRegMask = 0;
			return;
			}
		if (iReady && iParent->iReady)
			{
			ss = TheSubSchedulers + (iParent->iReady & EReadyCpuMask);
			ss->iReadyListLock.LockOnly();
			}
		if (iCurrent)
			{
			// thread is actually running on another CPU
			// interrupt that CPU and wait for it to enter interrupt mode
			// this allows a snapshot of the thread user state to be observed
			// and ensures the thread cannot return to user mode
			send_resched_ipi_and_wait(iLastCpu);
			}
		}
	TUint32* stack = (TUint32*)(TLinAddr(iStackBase) + TLinAddr(iStackSize));
	SThreadExcStack* txs = 0;
	SThreadSlowExecStack* tsxs = 0;
	aRegMask &= 0xffffu;
	if (stack[-1]!=0xFFFFFFFFu && stack[-2]!=0xFFFFFFFFu && stack[-7]<0x100u)	// if not, thread never entered user mode
		{
		if (stack[-7] == 0x21)	// slow exec
			tsxs = ((SThreadSlowExecStack*)stack)-1;
		else
			txs = ((SThreadExcStack*)stack)-1;

#define WRITE_REG(reg, value)	\
			{ if (tsxs) tsxs->reg=(value); else txs->reg=(value); }

		if (aRegMask & 0x0001u)
			WRITE_REG(iEax, aContext.iEax);
		if (aRegMask & 0x0002u)
			WRITE_REG(iEbx, aContext.iEbx);
		if (aRegMask & 0x0004u)
			{
			// don't allow write to iEcx if in slow exec since this may conflict
			// with handle preprocessing
			if (tsxs)
				aRegMask &= ~0x0004u;
			else
				txs->iEcx = aContext.iEcx;
			}
		if (aRegMask & 0x0008u)
			WRITE_REG(iEdx, aContext.iEdx);
		if (aRegMask & 0x0010u)
			WRITE_REG(iEsp3, aContext.iEsp);
		if (aRegMask & 0x0020u)
			WRITE_REG(iEbp, aContext.iEbp);
		if (aRegMask & 0x0040u)
			WRITE_REG(iEsi, aContext.iEsi);
		if (aRegMask & 0x0080u)
			WRITE_REG(iEdi, aContext.iEdi);
		if (aRegMask & 0x0100u)
			WRITE_REG(iCs, aContext.iCs|3);
		if (aRegMask & 0x0200u)
			WRITE_REG(iDs, aContext.iDs|3);
		if (aRegMask & 0x0400u)
			WRITE_REG(iEs, aContext.iEs|3);
		if (aRegMask & 0x0800u)
			WRITE_REG(iFs, aContext.iFs|3);
		if (aRegMask & 0x1000u)
			WRITE_REG(iGs, aContext.iGs|3);
		if (aRegMask & 0x2000u)
			WRITE_REG(iSs3, aContext.iSs|3);
		if (aRegMask & 0x4000u)
			WRITE_REG(iEflags, aContext.iEflags);
		if (aRegMask & 0x8000u)
			WRITE_REG(iEip, aContext.iEip);
		}
	else
		aRegMask = 0;
	if (pC != this)
		{
		if (ss)
			ss->iReadyListLock.UnlockOnly();
		RelSLock();
		}
	}

/** Get (subset of) user context of specified thread.

	The nanokernel does not systematically save all registers in the supervisor
	stack on entry into privileged mode and the exact subset depends on why the
	switch to privileged mode occured.  So in general only a subset of the
	register set is available.

	@param aThread	Thread to inspect.  It can be the current thread or a
	non-current one.

	@param aContext	Pointer to TX86RegSet structure where the context is
	copied.

	@param aAvailRegistersMask Bit mask telling which subset of the context is
	available and has been copied to aContext (1: register available / 0: not
	available). Bits represent fields in TX86RegSet, i.e.
	0:EAX	1:EBX	2:ECX	3:EDX	4:ESP	5:EBP	6:ESI	7:EDI
	8:CS	9:DS	10:ES	11:FS	12:GS	13:SS	14:EFLAGS 15:EIP

	@see TX86RegSet
	@see ThreadSetUserContext

	@pre Call in a thread context.
	@pre Interrupts must be enabled.
 */
EXPORT_C void NKern::ThreadGetUserContext(NThread* aThread, TAny* aContext, TUint32& aAvailRegistersMask)
	{
	CHECK_PRECONDITIONS(MASK_INTERRUPTS_ENABLED|MASK_NOT_ISR|MASK_NOT_IDFC,"NKern::ThreadGetUserContext");
	TX86RegSet& a = *(TX86RegSet*)aContext;
	memclr(aContext, sizeof(TX86RegSet));
	NKern::Lock();
	aThread->GetUserContext(a, aAvailRegistersMask);
	NKern::Unlock();
	}


/** Get (subset of) system context of specified thread.
  
	@param aThread	Thread to inspect.  It can be the current thread or a
	non-current one.

	@param aContext	Pointer to TX86RegSet structure where the context is
	copied.

	@param aAvailRegistersMask Bit mask telling which subset of the context is
	available and has been copied to aContext (1: register available / 0: not
	available). Bits represent fields in TX86RegSet, i.e.
	0:EAX	1:EBX	2:ECX	3:EDX	4:ESP	5:EBP	6:ESI	7:EDI
	8:CS	9:DS	10:ES	11:FS	12:GS	13:SS	14:EFLAGS 15:EIP

	@see TX86RegSet
	@see ThreadGetUserContext

	@pre Call in a thread context.
	@pre Interrupts must be enabled.
 */
EXPORT_C void NKern::ThreadGetSystemContext(NThread* aThread, TAny* aContext, TUint32& aAvailRegistersMask)
	{
	CHECK_PRECONDITIONS(MASK_INTERRUPTS_ENABLED|MASK_NOT_ISR|MASK_NOT_IDFC,"NKern::ThreadGetSystemContext");
	TX86RegSet& a = *(TX86RegSet*)aContext;
	memclr(aContext, sizeof(TX86RegSet));
	NKern::Lock();
	aThread->GetSystemContext(a, aAvailRegistersMask);
	NKern::Unlock();
	}


/** Set (subset of) user context of specified thread.

	@param aThread	Thread to modify.  It can be the current thread or a
	non-current one.

	@param aContext	Pointer to TX86RegSet structure containing the context
	to set.  The values of registers which aren't part of the context saved
	on the supervisor stack are ignored.

	@see TX86RegSet
	@see ThreadGetUserContext

  	@pre Call in a thread context.
	@pre Interrupts must be enabled.
 */
EXPORT_C void NKern::ThreadSetUserContext(NThread* aThread, TAny* aContext)
	{
	CHECK_PRECONDITIONS(MASK_INTERRUPTS_ENABLED|MASK_NOT_ISR|MASK_NOT_IDFC,"NKern::ThreadSetUserContext");
	TX86RegSet& a = *(TX86RegSet*)aContext;
	TUint32 mask = 0xffffu;
	NKern::Lock();
	aThread->SetUserContext(a, mask);
	NKern::Unlock();
	}


extern "C" void __fastcall add_dfc(TDfc* aDfc)
	{
	aDfc->Add();
	}


TInt NKern::QueueUserModeCallback(NThreadBase* aThread, TUserModeCallback* aCallback)
	{
	__e32_memory_barrier();
	if (aCallback->iNext != KUserModeCallbackUnqueued)
		return KErrInUse;
	if (aThread->i_NThread_Initial)
		return KErrArgument;
	TInt result = KErrDied;
	NKern::Lock();
	TUserModeCallback* listHead = aThread->iUserModeCallbacks;
	do	{
		if (TLinAddr(listHead) & 3)
			goto done;	// thread exiting
		aCallback->iNext = listHead;
		} while (!__e32_atomic_cas_ord_ptr(&aThread->iUserModeCallbacks, &listHead, aCallback));
	result = KErrNone;

	if (!listHead)	// if this isn't first callback someone else will have done this bit
		{
		/*
		 * If aThread is currently running on another CPU we need to send an IPI so
		 * that it will enter kernel mode and run the callback.
		 * The synchronization is tricky here. We want to check if the thread is
		 * running and if so on which core. We need to avoid any possibility of
		 * the thread entering user mode without having seen the callback,
		 * either because we thought it wasn't running so didn't send an IPI or
		 * because the thread migrated after we looked and we sent the IPI to
		 * the wrong processor. Sending a redundant IPI is not a problem (e.g.
		 * because the thread is running in kernel mode - which we can't tell -
		 * or because the thread stopped running after we looked)
		 * The following events are significant:
		 * Event A:	Target thread writes to iCurrent when it starts running
		 * Event B: Target thread reads iUserModeCallbacks before entering user
		 *			mode
		 * Event C: This thread writes to iUserModeCallbacks
		 * Event D: This thread reads iCurrent to check if aThread is running
		 * There is a barrier between A and B since A occurs with the ready
		 * list lock for the CPU involved or the thread lock for aThread held
		 * and this lock is released before B occurs.
		 * There is a barrier between C and D (__e32_atomic_cas_ord_ptr).
		 * Any observer which observes B must also have observed A.
		 * Any observer which observes D must also have observed C.
		 * If aThread observes B before C (i.e. enters user mode without running
		 * the callback) it must observe A before C and so it must also observe
		 * A before D (i.e. D reads the correct value for iCurrent).
		 */
		TInt current = aThread->iCurrent;
		if (current)
			{
			TInt cpu = current & NSchedulable::EReadyCpuMask;
			if (cpu != NKern::CurrentCpu())
				send_resched_ipi(cpu);
			}
		}
done:
	NKern::Unlock();
	return result;
	}


