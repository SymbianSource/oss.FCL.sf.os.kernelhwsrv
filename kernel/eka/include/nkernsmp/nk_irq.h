// Copyright (c) 2007-2009 Nokia Corporation and/or its subsidiary(-ies).
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
// e32\include\nkernsmp\nk_irq.h
// 
// WARNING: This file contains some APIs which are internal and are subject
//          to change without notice. Such APIs should therefore not be used
//          outside the Kernel and Hardware Services package.
//

/**
 @file
 @internalTechnology
*/

#ifndef __NK_IRQ_H__
#define __NK_IRQ_H__

#ifndef NK_MAX_IRQS
#if defined(__CPU_ARM)
#define NK_MAX_IRQS			96	// 32-127 on GIC
#else
#define NK_MAX_IRQS			32
#endif
#endif

#ifndef NK_MAX_IRQ_HANDLERS
#if defined(__CPU_ARM)
#define NK_MAX_IRQ_HANDLERS	(1*NK_MAX_IRQS)
#else
#define NK_MAX_IRQ_HANDLERS	(2*NK_MAX_IRQS)
#endif
#endif

#include <nklib.h>
#include <nk_event.h>

class NSchedulable;
class NThreadBase;
class NThread;
class NFastSemaphore;
class NFastMutex;

/******************************************************************************
 * Class per peripheral interrupt
 ******************************************************************************/
class NIrqHandler;
class NIrqX;
class NIrq
	{
public:
	NIrq();
public:
	// client services
	TInt BindRaw(NIsr aIsr, TAny* aPtr);
	TInt UnbindRaw();
	TInt DisableRaw(TBool aUnbind);
	TInt EnableRaw();
	TInt Bind(NIrqHandler* aH);
	static TInt FromHandle(TInt& aHandle, NIrq*& aIrq, NIrqHandler*& aHandler);
public:
	// HW access
	void HwIsr();
	void HwEoi();
	void HwEnable();
	void HwDisable();
	void HwSetCpu(TInt aCpu);
	void HwSetCpuMask(TUint32 aMask);
	void HwInit();
	static void HwInit0();
	static void HwInit1();
	static void HwInit2AP();
	TBool HwPending();
	void HwWaitCpus();
public:
	// functions to manipulate iIState
	TUint32 EnterIsr();			// wait for EWait clear, increment run count, return original iIState
	TBool IsrDone();			// decrement run count, return TRUE if still not zero
	void Wait();				// wait until run count is zero and we can transition EWait from 0 to 1
	void Done();				// set EWait back to 0
public:
	enum	{	// iStaticFlags
			ELevel=0x01,		// set for level triggered, clear for edge
			EPolarity=0x02,		// set for active high, clear for active low
			EShared=0x10,		// set if interrupt can be shared
			};
	enum	{	// iIState bits 0-7
			EWait=0x01,
			ERaw=0x02,			// raw ISR with no processing, can't be shared
			ECount=0x04,		// if set count all interrupts else limit pending count to 1
			EUnbind=0x08,		// raw ISR being unbound
			};
public:
	TSpinLock			iNIrqLock;
	SDblQue				iHandlers;
	volatile TUint32	iIState;		// bits 0-7=flags, bits 8-15=CPU on which it is running, bits 16-31=run count
	TUint16				iStaticFlags;
	TUint16				iIndex;
	volatile TUint32	iEventsPending;
	volatile TUint32	iEnabledEvents;	// bits 1-31 = count of enabled handlers, bit 0 = 1 if temporarily disabled
	volatile TUint32	iGeneration;	// incremented on unbind raw or enable while bound as raw
	TUint32				iHwId;
	TUint32				iVector;
	NIrqX*				iX;
	TUint32				iNIrqSpare[16-10-sizeof(TSpinLock)/sizeof(TUint32)];
	};

__ASSERT_COMPILE(!(_FOFF(NIrq,iNIrqLock)&7));
__ASSERT_COMPILE(sizeof(NIrq)==64);

class NIrqX
	{
public:
	typedef void (*TEoiFn)(NIrq*);
	typedef void (*TEnableFn)(NIrq*);
	typedef void (*TDisableFn)(NIrq*);
	typedef void (*TSetCpuFn)(NIrq*, TUint32);
	typedef void (*TInitFn)(NIrq*);
	typedef TBool (*TPendingFn)(NIrq*);
	typedef void (*TWaitFn)(NIrq*);
public:
	TEoiFn				iEoiFn;
	TEnableFn			iEnableFn;
	TDisableFn			iDisableFn;
	TSetCpuFn			iSetCpuFn;
	TInitFn				iInitFn;
	TPendingFn			iPendingFn;
	TWaitFn				iWaitFn;
	};

/******************************************************************************
 * Class per interrupt handler
 ******************************************************************************/
typedef NEventFn NIsr;
class TSubScheduler;
class NIrqHandler : public NEventHandler
	{
public:
	NIrqHandler();
	static NIrqHandler* Alloc();
	void Free();
	void Activate(TInt aCount);
	TInt Enable(TInt aHandle);
	TInt Disable(TBool aUnbind, TInt aHandle);
	TInt Unbind(TInt aId, NSchedulable* aTied);
	void DoUnbind();
public:
	// functions to manipulate iHState
	TUint32 DoSetEnabled();			// if EUnbound clear, clear EDisable. Return original iHState
	TUint32 DoActivate(TInt);
	TUint32 EventBegin();
	TUint32 EventDone();
public:
	enum	{	// iHState bits 8-31
			EDisable		=0x00000100u,
			EUnbind			=0x00000200u,				// this handler is being unbound
			EBind			=0x00000400u,				// this handler has been bound but not enabled
			ENotReady		=0x00000800u,				// this handler is being bound
			ECount			=0x00001000u,				// if set count all interrupts else limit pending count to 1
			EActive			=0x00002000u,				// handler is running or about to run
			EExclusive		=0x00004000u,				// exclusive access to shared interrupt
			ERunCountMask	=0xffff0000u
			};
public:
	SDblQueLink				iIrqLink;		// link to NIrq
	NIrq* volatile			iIrq;
	volatile TUint32		iGeneration;	// incremented on enable or bind
	volatile TUint32		iHandle;		// bits 0-15 = array index, bits 16-30 = cookie, 1-32767
	TUint32					iNIrqHandlerSpare[3];		// round to power of 2
public:
	static NIrqHandler*		FirstFree;		// protected by NEventHandler::TiedLock
	};

__ASSERT_COMPILE(sizeof(NIrqHandler)==64);


#if 0
#include <e32btrace.h>
#define IRQ_TRACE_CAT			253			// FIXME
#define	TRACE_IRQ0(n)			BTraceContextPc0(IRQ_TRACE_CAT, n)
#define	TRACE_IRQ4(n,a)			BTraceContextPc4(IRQ_TRACE_CAT, n, a)
#define	TRACE_IRQ8(n,a,b)		BTraceContextPc8(IRQ_TRACE_CAT, n, a, b)
#define	TRACE_IRQ12(n,a,b,c)	BTraceContextPc12(IRQ_TRACE_CAT, n, a, b, c)
#else
#define	TRACE_IRQ0(n)
#define	TRACE_IRQ4(n,a)
#define	TRACE_IRQ8(n,a,b)
#define	TRACE_IRQ12(n,a,b,c)
#endif

#endif	// __NK_IRQ_H__
