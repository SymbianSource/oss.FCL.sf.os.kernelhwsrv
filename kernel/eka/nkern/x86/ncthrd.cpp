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
// e32\nkern\x86\ncthrd.cpp
// 
//

#include <x86.h>

// Called by a thread when it first runs
void __StartThread();
void __DoForcedExit();

void NThreadBase::OnKill()
	{
	}


void NThreadBase::OnExit()
	{
	}


void NThreadBase::SetEntry(NThreadFunction aFunc)
	{
	SThreadStack* stack=(SThreadStack*)iSavedSP;
	stack->iEdi=(TUint32)aFunc;
	}


TInt NThread::Create(SNThreadCreateInfo& anInfo, TBool aInitial)
	{
	if (!anInfo.iStackBase || anInfo.iStackSize<0x100)
		return KErrArgument;
	TInt r=NThreadBase::Create(anInfo,aInitial);
	if (r!=KErrNone)
		return r;
	if (!aInitial)
		{
		TUint32* sp=(TUint32*)(iStackBase+iStackSize-anInfo.iParameterBlockSize);
		TUint32 esi=(TUint32)anInfo.iParameterBlock;
		if (anInfo.iParameterBlockSize)
			{
			wordmove(sp,anInfo.iParameterBlock,anInfo.iParameterBlockSize);
			esi=(TUint32)sp;
			}
		SThreadStack* stack=((SThreadStack*)sp)-1;
		stack->iCR0=X86::DefaultCR0 | KX86CR0_TS;
		stack->iEbx=0;
		stack->iEsi=esi;					// parameter block pointer
		stack->iEdi=(TUint32)anInfo.iFunction;
		stack->iEbp=0;
		stack->iGs=KRing0DS;
		stack->iFs=0;
		stack->iReschedFlag=1;
		stack->iEip=(TUint32)__StartThread;
		iSavedSP=(TLinAddr)stack;
		wordmove(&iCoprocessorState, DefaultCoprocessorState, sizeof(iCoprocessorState));
		}
	else
		{
#ifdef MONITOR_THREAD_CPU_TIME
		iLastStartTime = NKern::FastCounter();
#endif
		NKern::EnableAllInterrupts();
		}
#ifdef BTRACE_THREAD_IDENTIFICATION
	BTrace4(BTrace::EThreadIdentification,BTrace::ENanoThreadCreate,this);
#endif
	return KErrNone;
	}


void NThreadBase::ForceExit()
	{
	SThreadStack* stack=(SThreadStack*)iSavedSP;
	stack->iEip=(TUint32)__DoForcedExit;
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
	DEBUGPRINT("Thread %T, KernCSLocked=%d, IrqNest=%d",TheScheduler.iCurrentThread,TheScheduler.iKernCSLocked,X86_IrqNestCount);
	}


EXPORT_C void NKern::ThreadGetUserContext(NThread* aThread, TAny* aContext, TUint32& aAvailMask)
	{
	CHECK_PRECONDITIONS(MASK_INTERRUPTS_ENABLED|MASK_NOT_ISR|MASK_NOT_IDFC, "NKern::ThreadGetUserContext");
	TUint32* sp;
	TUint32* stackTop;
	TX86RegSet* regSet = (TX86RegSet*)aContext;
	TInt delta;

	NKern::Lock();

	NThread* currentThread = NCurrentThread();
	DEBUGPRINT(" NCurrentThread()=0x%x, aThread=0x%x", currentThread, aThread);

	switch (NKern::CurrentContext())
		{
		case NKern::EThread: DEBUGPRINT(" CurrentContext=NKern::EThread"); break;
		case NKern::EIDFC: DEBUGPRINT(" CurrentContext=NKern::EIDFC"); break;
		case NKern::EInterrupt: DEBUGPRINT(" CurrentContext=NKern::EInterrupt"); break;
		default: DEBUGPRINT(" CurrentContext= Unknown"); break;
		}

	DEBUGPRINT(" Attributes (iSpare2)=0x%x", aThread->iSpare2);
	DEBUGPRINT(" iExtraContext=0x%x, iExtraContextSize=0x%x", aThread->iExtraContext, aThread->iExtraContextSize);

	DEBUGPRINT(" iSuspendCount=%d", aThread->iSuspendCount);

	DEBUGPRINT(" X86_IrqStack=%x", X86_IrqStack);

	TBool isCurrentThread = (currentThread == aThread);

	sp = (TUint32*)aThread->iSavedSP;
	stackTop = (TUint32*)((TUint32)aThread->iStackBase+(TUint32)aThread->iStackSize);
	delta = stackTop - sp;	// number of words on the supervisor stack

	DEBUGPRINT(" Stack Top=iStackBase+iStackSize=0x%x iSavedSP=0x%x, delta=0x%x", stackTop, sp, delta);
	DEBUGPRINT(" iUserContextType (iSpare3)=0x%x", aThread->iSpare3);

	DEBUGPRINT(" NThreadState:");
	switch(aThread->iSpare1)
		{
		case NThreadBase::EReady:
			DEBUGPRINT(" EReady");
			break;
		case NThreadBase::ESuspended:
			DEBUGPRINT(" ESuspended");
			break;
		case NThreadBase::EWaitFastSemaphore:
			DEBUGPRINT(" EWaitFastSemaphore");
			break;
		case NThreadBase::ESleep:
			DEBUGPRINT(" ESleep");
			break;
		case NThreadBase::EBlocked:
			DEBUGPRINT(" EBlocked");
			break;
		case NThreadBase::EDead:
			DEBUGPRINT(" EDead");
			break;
		case NThreadBase::EWaitDfc:
			DEBUGPRINT(" EWaitDfc");
			break;
		default:
			DEBUGPRINT(" *Unknown");
		}

	if (aAvailMask)
		{
		DEBUGPRINT(" Setting Stack-Saved Registers");
		// Hack while ThreadSetUserContext is not implemented

		if (0 == aThread->iSpare3)
			{
			// Context when interrupted by user
			if (aAvailMask & 1<<13)
				stackTop[-1] = regSet->iSs;

			if (aAvailMask & 1<<4)
				stackTop[-2] = regSet->iEsp;

			if (aAvailMask & 1<<14)
				{
				DEBUGPRINT(" Setting EFLAGS to %x", regSet->iEflags);
				stackTop[-3] = regSet->iEflags;
				}

			if (aAvailMask & 1<<8)
				stackTop[-4] = regSet->iCs;

			if (aAvailMask & 1<<15)
				stackTop[-5] = regSet->iEip;

			if (aAvailMask & 1<<9)
				stackTop[-8] = regSet->iDs;

			if (aAvailMask & 1<<0)
				stackTop[-9] = regSet->iEax;

			if (aAvailMask & 1<<2)
				stackTop[-10] = regSet->iEcx;

			if (aAvailMask & 1<<5)
				stackTop[-16] = regSet->iEbp;

			if (aAvailMask & 1<<7)
				stackTop[-17] = regSet->iEdi;

			if (aAvailMask & 1<<6)
				stackTop[-18] = regSet->iEsi;
			}
		else
			{
			if (aAvailMask & 1<<13)
				stackTop[-1] = regSet->iSs;

			if (aAvailMask & 1<<4)
				stackTop[-2] = regSet->iEsp;

			if (aAvailMask & 1<<14)
				{
				stackTop[-3] = regSet->iEflags;
				DEBUGPRINT(" Setting EFLAGS to %x", regSet->iEflags);
				}

			if (aAvailMask & 1<<8)
				stackTop[-4] = regSet->iCs;

			if (aAvailMask & 1<<15)
				stackTop[-5] = regSet->iEip;

			/* -6 and -7 are not used since they are the vector number and the error code,
			 * which are 3 and 0 resp. for breakpoints.
			 */

			/* The following are from the push instructions in __X86VectorExc */
			if (aAvailMask & 1<<9)
				stackTop[-8] = regSet->iDs;

			if (aAvailMask & 1<<10)
				stackTop[-9] = regSet->iEs;

			if (aAvailMask & 1<<11)
				stackTop[-10] = regSet->iFs;

			if (aAvailMask & 1<<12)
				stackTop[-11] = regSet->iGs;

			if (aAvailMask & 1<<5)
				stackTop[-12] = regSet->iEbp;

			if (aAvailMask & 1<<7)
				stackTop[-13] = regSet->iEdi;

			if (aAvailMask & 1<<6)
				stackTop[-14] = regSet->iEsi;

			if (aAvailMask & 1<<1)
				stackTop[-15] = regSet->iEbx;

			if (aAvailMask & 1<<2)
				stackTop[-16] = regSet->iEcx;

			if (aAvailMask & 1<<3)
				stackTop[-17] = regSet->iEdx;

			if (aAvailMask & 1<<0)
				stackTop[-18] = regSet->iEax;
			}

		DEBUGPRINT("stack from stack top, after changes " );
		if (delta < 128)
			{
			delta = -delta;
			}
		else
			{
			delta = -128;
			}

		for ( ; delta < 0; delta++)
			{
			DEBUGPRINT("stackTop[%d]=%x", delta, stackTop[delta]);
			}
		}
	else
		{
 		memclr(aContext, sizeof(TX86RegSet));

		if (isCurrentThread)
			{
			// Not yet supported
			DEBUGPRINT(" NThread::GetContext() : Don't know how to obtain context for current thread\n Use TSS?");
			}
		else
			{
			DEBUGPRINT("stack from stack top " );

			if (delta < 128)
				{
				delta = -delta;
				}
			else
				{
				delta = -128;
				}

			for( ; delta < 0; delta++)
				{
				DEBUGPRINT("stackTop[%d]=%x", delta, stackTop[delta]);
				}

			if (0 == aThread->iSpare3)
				{
				// Context when interrupted by user
				regSet->iSs		= stackTop[-1];
				aAvailMask |= 1<<13;

				regSet->iEsp	= stackTop[-2];
				aAvailMask |= 1<<4;

				regSet->iEflags = stackTop[-3];
				aAvailMask |= 1<<14;

				regSet->iCs		= stackTop[-4];
				aAvailMask |= 1<<8;

				regSet->iEip	= stackTop[-5];
				aAvailMask |= 1<<15;

				regSet->iDs		= stackTop[-8];
				aAvailMask |= 1<<9;

				regSet->iEax	= stackTop[-9];
				aAvailMask |= 1<<0;

				regSet->iEcx	= stackTop[-10];
				aAvailMask |= 1<<2;

				regSet->iEbp	= stackTop[-16];
				aAvailMask |= 1<<5;

				regSet->iEdi	= stackTop[-17];
				aAvailMask |= 1<<7;

				regSet->iEsi	= stackTop[-18];
				aAvailMask |= 1<<6;
				}
			else
				{
				// Now populate the TX86RegSet with the contents of the stack

				/*
				 * The first 5 are from the comments at the start of __X86VectorExc :
				 * [ESP+0] = vector number
				 * [ESP+4] = error code (filled with 0 for exceptions without error codes)
				 * [ESP+8] = return EIP
				 * [ESP+12] = return CS
				 * [ESP+16] = return EFLAGS
				 * [ESP+20] = return ESP if privilege change occurred
				 * [ESP+24] = return SS if privilege change occurred
				 */
				regSet->iSs		= stackTop[-1];
				aAvailMask |= 1<<13;

				regSet->iEsp	= stackTop[-2];
				aAvailMask |= 1<<4;

				regSet->iEflags = stackTop[-3];
				aAvailMask |= 1<<14;

				regSet->iCs		= stackTop[-4];
				aAvailMask |= 1<<8;

				regSet->iEip	= stackTop[-5];
				aAvailMask |= 1<<15;

				/* -6 and -7 are not used since they are the vector number and the error code,
				 * which for a breakpoint are 3 and 0 resp.
				 */

				/* The following are from the push instructions in __X86VectorExc */
				regSet->iDs		= stackTop[-8];
				aAvailMask |= 1<<9;

				regSet->iEs		= stackTop[-9];
				aAvailMask |= 1<<10;

				regSet->iFs		= stackTop[-10];
				aAvailMask |= 1<<11;

				regSet->iGs		= stackTop[-11];
				aAvailMask |= 1<<12;

				regSet->iEbp	= stackTop[-12];
				aAvailMask |= 1<<5;

				regSet->iEdi	= stackTop[-13];
				aAvailMask |= 1<<7;

				regSet->iEsi	= stackTop[-14];
				aAvailMask |= 1<<6;

				regSet->iEbx	= stackTop[-15];
				aAvailMask |= 1<<1;

				regSet->iEcx	= stackTop[-16];
				aAvailMask |= 1<<2;

				regSet->iEdx	= stackTop[-17];
				aAvailMask |= 1<<3;

				regSet->iEax = stackTop[-18];
				aAvailMask |= 1<<0;
				} // else if (0 == aThread->iSpare3)

			} // else if (isCurrentThread)

		} // else if (aAvailMask)

	NKern::Unlock();
	}


void NKern::ThreadModifyUsp(NThread* aThread, TLinAddr aUsp)
	{
	}
