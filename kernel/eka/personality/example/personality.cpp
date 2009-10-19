// Copyright (c) 2003-2009 Nokia Corporation and/or its subsidiary(-ies).
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
// e32\personality\example\personality.cpp
// Example RTOS personality.
// 
//

#include "personality_int.h"

/******************************************************************************
 * Memory pool management
 ******************************************************************************/

// Create a single memory pool consisting of a specified number of equal sized blocks.
TInt PMemPool::Create(const poolinfo* aInfo)
	{
	iBlockSize = aInfo->block_size;
	TUint bsize = iBlockSize + sizeof(PMemPool*);
	TInt n = (TInt)aInfo->block_count;
	__KTRACE_OPT(KBOOT, Kern::Printf("PMemPool::Create %08x iBlockSize=%04x bsize=%04x n=%04x", this, iBlockSize, bsize, n));
	if (bsize < sizeof(SMemBlock) || (bsize & 3))
		return KErrArgument;
	TInt total_size = n * bsize;
	iFirstFree = (SMemBlock*)Kern::Alloc(total_size);
	__KTRACE_OPT(KBOOT, Kern::Printf("PMemPool::Create %08x iFirstFree=%08x", this, iFirstFree));
	if (!iFirstFree)
		return KErrNoMemory;
	TInt i;
	for (i=0; i<n; ++i)
		{
		SMemBlock* p = (SMemBlock*)(TLinAddr(iFirstFree) + i*bsize);
		SMemBlock* q = (i<n-1) ? (SMemBlock*)(TLinAddr(p) + bsize) : NULL;
		p->iPool = this;
		p->iNext = q;
		}
	__KTRACE_OPT(KBOOT, Kern::Printf("PMemPool::Create OK"));
	return KErrNone;
	}

// Call with interrupts disabled
void* PMemPool::Alloc()
	{
	SMemBlock* p = iFirstFree;
	if (p)
		{
		iFirstFree = p->iNext;
		__KTRACE_OPT(KBOOT, Kern::Printf("AL:%08x->%08x", this, &p->iNext));
		return &p->iNext;
		}
	__KTRACE_OPT(KBOOT, Kern::Printf("AL:%08x->0", this));
	return NULL;
	}

// Call with interrupts disabled
void PMemPool::Free(void* aBlock)
	{
	__KTRACE_OPT(KBOOT, Kern::Printf("FR:%08x<-%08x", this, aBlock));
	SMemBlock* b = (SMemBlock*)aBlock;
	__NK_ASSERT_DEBUG(b->iPool==this);
	b->iNext = iFirstFree;
	iFirstFree = b;
	}

PMemMgr* PMemMgr::TheMgr;

// Create a 'size bucket' memory manager consisting of a number of memory pools
// each containing blocks of the same size. The block size increases from one
// pool to the next.
void PMemMgr::Create(const poolinfo* aInfo)
	{
	TInt n;
	for (n=0; aInfo[n].block_size; ++n) {}
	PMemMgr* m = (PMemMgr*)Kern::Alloc(sizeof(PMemMgr) + (n-1)*sizeof(PMemPool));
	__KTRACE_OPT(KBOOT, Kern::Printf("PMemMgr::Create %08x NumPools=%d", m, n));
	__NK_ASSERT_ALWAYS(m!=NULL);
	m->iPoolCount = n;
	TInt i;
	size_t prev_sz=0;
	for (i=0; i<n; ++i)
		{
		__NK_ASSERT_ALWAYS(aInfo[i].block_size > prev_sz);
		prev_sz = aInfo[i].block_size;
		TInt r = m->iPools[i].Create(aInfo+i);
		__NK_ASSERT_ALWAYS(r==KErrNone);
		}
	TheMgr = m;
	}

// Allocate a memory block of the requested size (or the next larger size if necessary).
void* PMemMgr::Alloc(size_t aSize)
	{
	__KTRACE_OPT(KBOOT, Kern::Printf("MA:%04x", aSize));
	void* b = NULL;
	PMemPool* p = &TheMgr->iPools[0];
	PMemPool* q = p + TheMgr->iPoolCount;
	for (; p<q && p->iBlockSize < aSize; ++p) {}
	if (p < q)
		{
		TInt irq = NKern::DisableAllInterrupts();
		b = p->Alloc();
		NKern::RestoreInterrupts(irq);
		}
	return b;
	}

// Free a memory block
void PMemMgr::Free(void* aPtr)
	{
	__KTRACE_OPT(KBOOT, Kern::Printf("MF:%08x", aPtr));
	SMemBlock* b = _LOFF(aPtr, SMemBlock, iNext);
	TInt irq = NKern::DisableAllInterrupts();
	b->iPool->Free(b);
	NKern::RestoreInterrupts(irq);
	}


/* Memory management APIs */
extern "C" {
void* alloc_mem_block(size_t size)
	{
	return PMemMgr::Alloc(size);
	}

void free_mem_block(void* block)
	{
	PMemMgr::Free(block);
	}
}

/******************************************************************************
 * Task management
 ******************************************************************************/

TInt PThread::NumTasks;
TInt PThread::MaxTaskId;
PThread** PThread::TaskTable;

// RTOS priority to nanokernel priority mapping
const TUint8 PThread::NThreadPriorityTable[MAX_TASK_PRIORITY+1] =
	{
	0x00, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x02, 0x02, 0x02, 0x02, 0x03, 0x03, 0x03, 0x03,
	0x04, 0x04, 0x04, 0x04, 0x05, 0x05, 0x05, 0x05, 0x06, 0x06, 0x06, 0x06, 0x07, 0x07, 0x07, 0x07,
	0x08, 0x08, 0x08, 0x08, 0x09, 0x09, 0x09, 0x09, 0x0a, 0x0a, 0x0a, 0x0a, 0x0b, 0x0b, 0x0b, 0x0b,
	0x0c, 0x0c, 0x0c, 0x0c, 0x0d, 0x0d, 0x0d, 0x0d, 0x0e, 0x0e, 0x0e, 0x0e, 0x0f, 0x0f, 0x0f, 0x0f,
	0x10, 0x10, 0x10, 0x10, 0x11, 0x11, 0x11, 0x11, 0x12, 0x12, 0x12, 0x12, 0x13, 0x13, 0x13, 0x13,
	0x14, 0x14, 0x14, 0x14, 0x15, 0x15, 0x15, 0x15, 0x16, 0x16, 0x16, 0x16, 0x17, 0x17, 0x17, 0x17,
	0x18, 0x18, 0x18, 0x18, 0x19, 0x19, 0x19, 0x19, 0x1a, 0x1a, 0x1a, 0x1a, 0x1b, 0x1b, 0x1b, 0x1b,
	0x1c, 0x1c, 0x1c, 0x1c, 0x1d, 0x1d, 0x1d, 0x1d, 0x1e, 0x1e, 0x1e, 0x1e, 0x1f, 0x1f, 0x1f, 0x1f,
	0x20, 0x20, 0x20, 0x20, 0x21, 0x21, 0x21, 0x21, 0x22, 0x22, 0x22, 0x22, 0x23, 0x23, 0x23, 0x23,
	0x24, 0x24, 0x24, 0x24, 0x25, 0x25, 0x25, 0x25, 0x26, 0x26, 0x26, 0x26, 0x27, 0x27, 0x27, 0x27,
	0x28, 0x28, 0x28, 0x28, 0x29, 0x29, 0x29, 0x29, 0x2a, 0x2a, 0x2a, 0x2a, 0x2b, 0x2b, 0x2b, 0x2b,
	0x2c, 0x2c, 0x2c, 0x2c, 0x2d, 0x2d, 0x2d, 0x2d, 0x2e, 0x2e, 0x2e, 0x2e, 0x2f, 0x2f, 0x2f, 0x2f,
	0x30, 0x30, 0x30, 0x30, 0x31, 0x31, 0x31, 0x31, 0x32, 0x32, 0x32, 0x32, 0x33, 0x33, 0x33, 0x33,
	0x34, 0x34, 0x34, 0x34, 0x35, 0x35, 0x35, 0x35, 0x36, 0x36, 0x36, 0x36, 0x37, 0x37, 0x37, 0x37,
	0x38, 0x38, 0x38, 0x38, 0x39, 0x39, 0x39, 0x39, 0x3a, 0x3a, 0x3a, 0x3a, 0x3b, 0x3b, 0x3b, 0x3b,
	0x3c, 0x3c, 0x3c, 0x3c, 0x3d, 0x3d, 0x3d, 0x3d, 0x3e, 0x3e, 0x3e, 0x3e, 0x3f, 0x3f, 0x3f, 0x3f
	};

// Handlers for personality layer threads
const SNThreadHandlers PThread::Handlers =
	{
	NULL,				// no exit handler
	&StateHandler,
	&ExceptionHandler,
	NULL				// no timeout handler
	};

// Create a personality layer thread
TInt PThread::Create(PThread*& aThread, const taskinfo* a)
	{
	if (!a->entry_pt)
		return BAD_ENTRY_POINT;
	if (a->priority < MIN_TASK_PRIORITY || a->priority > MAX_TASK_PRIORITY)
		return BAD_PRIORITY;
	if (a->stack_size & 3 || a->stack_size < MIN_STACK_SIZE)
		return BAD_STACK_SIZE;
	if (a->task_id < 0)
		return BAD_TASK_ID;
	TInt memsize = sizeof(PThread) + a->stack_size;
	PThread* t = (PThread*)Kern::Alloc(memsize);
	if (!t)
		return OUT_OF_MEMORY;
	t->iTaskId = a->task_id;
	t->iSetPriority = a->priority;
	t->iFirstMsg = NULL;
	t->iLastMsg = NULL;
	t->iISRFirstMsg = NULL;
	t->iISRLastMsg = NULL;
	new (&t->iMsgQIDfc) TDfc(&MsgQIDfcFn, t);
	TAny* stack = t + 1;
	memset(stack, 0xbb, a->stack_size);
	SNThreadCreateInfo info;
	info.iFunction = (NThreadFunction)a->entry_pt;
	info.iStackBase = stack;
	info.iStackSize = a->stack_size;
	info.iPriority = NThreadPriorityTable[a->priority];
	info.iTimeslice = -1;	// no timeslicing
	info.iAttributes = 0;
	info.iHandlers = &Handlers;
	info.iFastExecTable = NULL;
	info.iSlowExecTable = NULL;
	info.iParameterBlock = NULL;
	info.iParameterBlockSize = 0;
	TInt r = NKern::ThreadCreate(t, info);
	__NK_ASSERT_ALWAYS(r==KErrNone);
	aThread = t;
	return OK;
	}

// Create all required personality layer threads
void PThread::CreateAll(const taskinfo* a)
	{
	TInt n = 0;
	TInt maxid = -1;
	for (; a[n].entry_pt; ++n)
		{
		if (a[n].task_id > maxid)
			maxid = a[n].task_id;
		}
	NumTasks = n;
	MaxTaskId = maxid;
	TaskTable = (PThread**)Kern::AllocZ((maxid+1) * sizeof(PThread*));
	__NK_ASSERT_ALWAYS(TaskTable != NULL);
	TInt i;
	for (i=0; i<NumTasks; ++i)
		{
		TInt r = Create(TaskTable[a[i].task_id], a+i);
		__NK_ASSERT_ALWAYS(r == KErrNone);
		}
	// resume the tasks
	for (i=0; i<NumTasks; ++i)
		{
		if (a[i].auto_start)
			NKern::ThreadResume(TaskTable[i]);
		}
	}

// State handler
void PThread::StateHandler(NThread* aThread, TInt aOp, TInt aParam)
	{
	PThread* t = (PThread*)aThread;
	switch (aOp)
		{
		case NThreadBase::ESuspend:
			t->HandleSuspend();
			break;
		case NThreadBase::EResume:
		case NThreadBase::EForceResume:
			t->HandleResume();
			break;
		case NThreadBase::ERelease:
			t->HandleRelease(aParam);
			break;
		case NThreadBase::EChangePriority:
			t->HandlePriorityChange(aParam);
			break;
		case NThreadBase::ETimeout:
			t->HandleTimeout();
			break;
		case NThreadBase::ELeaveCS:
		default:
			__NK_ASSERT_ALWAYS(0);
		}
	}

// Exception handler - just fault
void PThread::ExceptionHandler(TAny* aContext, NThread* aThread)
	{
	(void)aThread;
	Exc::Fault(aContext);
	}

// Post a message to this thread from an ISR
void PThread::ISRPost(msghdr* aM)
	{
	aM->next = NULL;
	aM->sending_task_id = TASK_ID_ISR;
	msghdr* prev = (msghdr*)__e32_atomic_swp_ord_ptr(&iISRLastMsg, aM);
	if (prev)
		prev->next = aM;
	else
		{
		iISRFirstMsg = aM;
		iMsgQIDfc.Add();
		}
	}

// IDFC used to post message from ISR
void PThread::MsgQIDfcFn(TAny* aPtr)
	{
	PThread* t = (PThread*)aPtr;
	TInt irq = NKern::DisableAllInterrupts();
	msghdr* m = t->iISRFirstMsg;
	msghdr* l = t->iISRLastMsg;
	t->iISRFirstMsg = NULL;
	t->iISRLastMsg = NULL;
	NKern::RestoreInterrupts(irq);
	t->Post(m, l);
	}

// Post a chain of messages to this thread from an IDFC or thread
// Enter and return with preemption disabled
void PThread::Post(msghdr* aFirst, msghdr* aLast)
	{
	msghdr* l = iLastMsg;
	iLastMsg = aLast;
	if (l)
		{
		l->next = aFirst;
		return;	// queue was not empty so thread can't be waiting
		}
	iFirstMsg = aFirst;
	if (iNState == EWaitMsgQ)
		Release(KErrNone);
	}

// Dequeue and return the first message if there is one
// Return NULL if no messages waiting
// Enter and return with preemption disabled
msghdr* PThread::GetMsg()
	{
	msghdr* m = iFirstMsg;
	if (m)
		{
		iFirstMsg = m->next;
		if (!iFirstMsg)
			iLastMsg = NULL;
		}
	return m;
	}

void PThread::HandleSuspend()
	{
	switch(iNState)
		{
		case EWaitMsgQ:
			break;
		case EWaitSemaphore:
			((PSemaphore*)iWaitObj)->SuspendWaitingThread(this);
			break;
		default:
			__NK_ASSERT_ALWAYS(0);
		}
	}

void PThread::HandleResume()
	{
	switch(iNState)
		{
		case EWaitMsgQ:
			break;
		case EWaitSemaphore:
			((PSemaphore*)iWaitObj)->ResumeWaitingThread(this);
			break;
		default:
			__NK_ASSERT_ALWAYS(0);
		}
	}

void PThread::HandleRelease(TInt aReturnCode)
	{
	(void)aReturnCode;
	switch(iNState)
		{
		case EWaitMsgQ:
			CheckSuspendThenReady();
			break;
		case EWaitSemaphore:
			if (aReturnCode<0)
				((PSemaphore*)iWaitObj)->WaitCancel(this);
			else
				CheckSuspendThenReady();
			break;
		default:
			__NK_ASSERT_ALWAYS(0);
		}
	}

void PThread::HandlePriorityChange(TInt aNewPriority)
	{
	(void)aNewPriority;
	switch(iNState)
		{
		case EWaitMsgQ:
			iPriority = (TUint8)aNewPriority;
			break;
		case EWaitSemaphore:
			((PSemaphore*)iWaitObj)->ChangeWaitingThreadPriority(this, aNewPriority);
			break;
		default:
			__NK_ASSERT_ALWAYS(0);
		}
	}

void PThread::HandleTimeout()
	{
	switch(iNState)
		{
		case EWaitMsgQ:
			CheckSuspendThenReady();
			break;
		case EWaitSemaphore:
			((PSemaphore*)iWaitObj)->WaitCancel(this);
			break;
		default:
			__NK_ASSERT_ALWAYS(0);
		}
	}


/* Task APIs */
extern "C" {
int suspend_task(int id)
	{
	if (TUint(id) > TUint(PThread::MaxTaskId))
		return BAD_TASK_ID;
	PThread* t = PThread::TaskTable[id];
	if (!t)
		return BAD_TASK_ID;
	NKern::ThreadSuspend(t, 1);
	return OK;
	}

int resume_task(int id)
	{
	if (TUint(id) > TUint(PThread::MaxTaskId))
		return BAD_TASK_ID;
	PThread* t = PThread::TaskTable[id];
	if (!t)
		return BAD_TASK_ID;
	NKern::ThreadResume(t);
	return OK;
	}

int get_task_priority(int id)
	{
	if (TUint(id) > TUint(PThread::MaxTaskId))
		return BAD_TASK_ID;
	PThread* t = PThread::TaskTable[id];
	if (!t)
		return BAD_TASK_ID;
	return t->iSetPriority;
	}

int set_task_priority(int id, int priority)
	{
	if (TUint(id) > TUint(PThread::MaxTaskId))
		return BAD_TASK_ID;
	PThread* t = PThread::TaskTable[id];
	if (!t)
		return BAD_TASK_ID;
	if (priority < MIN_TASK_PRIORITY || priority > MAX_TASK_PRIORITY)
		return BAD_PRIORITY;
	NKern::Lock();
	t->iSetPriority = priority;
	t->SetPriority(PThread::NThreadPriorityTable[priority]);
	NKern::Unlock();
	return OK;
	}

int current_task_id(void)
	{
	TInt c = NKern::CurrentContext();
	if (c == NKern::EInterrupt)
		return TASK_ID_ISR;
	PThread* t = (PThread*)NKern::CurrentThread();
	if (t->iHandlers == &PThread::Handlers)
		return t->iTaskId;
	return TASK_ID_UNKNOWN;
	}

void disable_preemption(void)
	{
	NKern::Lock();
	}

void enable_preemption(void)
	{
	NKern::Unlock();
	}

int disable_interrupts(void)
	{
	return NKern::DisableAllInterrupts();
	}

void restore_interrupts(int level)
	{
	NKern::RestoreInterrupts(level);
	}


/* Message APIs */
int send_msg(int task_id, msghdr* msg)
	{
	if (TUint(task_id) > TUint(PThread::MaxTaskId))
		return BAD_TASK_ID;
	PThread* t = PThread::TaskTable[task_id];
	if (!t)
		return BAD_TASK_ID;
	TInt c = NKern::CurrentContext();
	if (c == NKern::EInterrupt)
		{
		t->ISRPost(msg);
		return OK;
		}
	msg->next = NULL;
	PThread* st = (PThread*)NKern::CurrentThread();
	msg->sending_task_id = (st->iHandlers == &PThread::Handlers) ? st->iTaskId : TASK_ID_UNKNOWN;
	NKern::Lock();
	t->Post(msg, msg);
	NKern::Unlock();
	return OK;
	}

int recv_msg(msghdr** msgptr, int time_ticks)
	{
	if (time_ticks < WAIT_FOREVER)
		return BAD_TIME_INTERVAL;
	PThread* t = (PThread*)NKern::CurrentThread();
	NKern::Lock();
	msghdr* m = t->GetMsg();
	if (!m && time_ticks != NO_WAIT)
		{
		NKern::NanoBlock(time_ticks>0 ? time_ticks : 0, PThread::EWaitMsgQ, NULL);
		NKern::PreemptionPoint();
		m = t->GetMsg();
		}
	NKern::Unlock();
	*msgptr = m;
	return m ? OK : TIMED_OUT;
	}
}

/******************************************************************************
 * Timer management
 ******************************************************************************/

TInt PTimer::NumTimers;
PTimer* PTimer::TimerTable;

// Create all required timers
void PTimer::CreateAll()
	{
	NumTimers = timer_count;
	TimerTable = new PTimer[timer_count];
	__NK_ASSERT_ALWAYS(TimerTable != NULL);
	}

PTimer::PTimer()
	:	NTimer(NTimerExpired, this),
		iPeriod(0),
		iCookie(0),
		iThread(0),
		iExpiryCount(0)
	{
	}

void PTimer::NTimerExpired(TAny* aPtr)
	{
	timer_msg* m = (timer_msg*)alloc_mem_block(sizeof(timer_msg));
	m->header.next = 0;
	m->header.msg_id = MSG_ID_TIMEOUT;
	PTimer* p = (PTimer*)aPtr;
	TInt irq = NKern::DisableAllInterrupts();
	PThread* t = p->iThread;
	m->count = ++p->iExpiryCount;
	m->cookie = p->iCookie;
	if (p->iPeriod > 0)
		p->Again(p->iPeriod);
	NKern::RestoreInterrupts(irq);
	t->ISRPost(&m->header);
	}

/* Timer APIs */
extern "C" {
unsigned tick_count(void)
	{
	return NKern::TickCount();
	}

void delay(int time_interval)
	{
	__NK_ASSERT_ALWAYS(time_interval > 0);
	NKern::Sleep(time_interval);
	}

int start_one_shot_timer(int timer_id, int task_id, int time_ticks, void* cookie)
	{
	if (time_ticks <= 0)
		return BAD_TIME_INTERVAL;
	if (TUint(timer_id) >= TUint(PTimer::NumTimers))
		return BAD_TIMER_ID;
	PTimer* tmr = PTimer::TimerTable + timer_id;
	if (TUint(task_id) > TUint(PThread::MaxTaskId))
		return BAD_TASK_ID;
	PThread* t = PThread::TaskTable[task_id];
	if (!t)
		return BAD_TASK_ID;
	TInt r = OK;
	TInt irq = NKern::DisableAllInterrupts();
	if (tmr->iThread)
		r = TIMER_IN_USE;
	else
		{
		tmr->iPeriod = 0;
		tmr->iCookie = cookie;
		tmr->iThread = t;
		tmr->iExpiryCount = 0;
		tmr->OneShot(time_ticks, EFalse);
		}
	NKern::RestoreInterrupts(irq);
	return r;
	}

int start_periodic_timer(int timer_id, int task_id, int initial_time_ticks, int period_ticks, void* cookie)
	{
	if (initial_time_ticks <= 0 || period_ticks <= 0)
		return BAD_TIME_INTERVAL;
	if (TUint(timer_id) >= TUint(PTimer::NumTimers))
		return BAD_TIMER_ID;
	PTimer* tmr = PTimer::TimerTable + timer_id;
	if (TUint(task_id) > TUint(PThread::MaxTaskId))
		return BAD_TASK_ID;
	PThread* t = PThread::TaskTable[task_id];
	if (!t)
		return BAD_TASK_ID;
	TInt r = OK;
	TInt irq = NKern::DisableAllInterrupts();
	if (tmr->iThread)
		r = TIMER_IN_USE;
	else
		{
		tmr->iPeriod = period_ticks;
		tmr->iCookie = cookie;
		tmr->iThread = t;
		tmr->iExpiryCount = 0;
		tmr->OneShot(initial_time_ticks, EFalse);
		}
	NKern::RestoreInterrupts(irq);
	return r;
	}

int stop_timer(int timer_id)
	{
	if (TUint(timer_id) >= TUint(PTimer::NumTimers))
		return BAD_TIMER_ID;
	PTimer* tmr = PTimer::TimerTable + timer_id;
	TInt irq = NKern::DisableAllInterrupts();
	tmr->Cancel();
	tmr->iThread = NULL;
	NKern::RestoreInterrupts(irq);
	return OK;
	}
}


/******************************************************************************
 * Semaphore management
 ******************************************************************************/

TInt PSemaphore::NumSemaphores;
PSemaphore* PSemaphore::SemaphoreTable;

void PSemaphore::CreateAll()
	{
	NumSemaphores = semaphore_count;
	SemaphoreTable = new PSemaphore[semaphore_count];
	__NK_ASSERT_ALWAYS(SemaphoreTable != NULL);
	}

PSemaphore::PSemaphore()
	:	iCount(0),
		iISRCount(0),
		iIDfc(IDfcFn, this)
	{
	}

void PSemaphore::WaitCancel(PThread* aThread)
	{
	if (aThread->iSuspendCount == 0)
		{
		iWaitQ.Remove(aThread);
		++iCount;
		}
	else
		aThread->Deque();
	aThread->CheckSuspendThenReady();
	}

void PSemaphore::SuspendWaitingThread(PThread* aThread)
	{
	// do nothing if already suspended
	if (aThread->iSuspendCount == 0)
		{
		iWaitQ.Remove(aThread);
		++iCount;
		iSuspendedQ.Add(aThread);
		}
	}

void PSemaphore::ResumeWaitingThread(PThread* aThread)
	{
	aThread->Deque();
	if (--iCount<0)
		{
		iWaitQ.Add(aThread);
		}
	else
		{
		aThread->iWaitObj=NULL;
		aThread->Ready();
		}
	}

void PSemaphore::ChangeWaitingThreadPriority(PThread* aThread, TInt aNewPriority)
	{
	if (aThread->iSuspendCount == 0)
		iWaitQ.ChangePriority(aThread, aNewPriority);
	else
		aThread->iPriority = (TUint8)aNewPriority;
	}

void PSemaphore::Signal()
	{
	if (++iCount <= 0)
		{
		// must wake up next thread
		PThread* t = iWaitQ.First();
		iWaitQ.Remove(t);
		t->Release(KErrNone);
		}
	}

void PSemaphore::ISRSignal()
	{
	if (__e32_atomic_add_ord32(&iISRCount, 1) == 0)
		iIDfc.Add();
	}

void PSemaphore::IDfcFn(TAny* aPtr)
	{
	PSemaphore* s = (PSemaphore*)aPtr;
	TInt count = (TInt)__e32_atomic_swp_ord32(&s->iISRCount, 0);
	while (count--)
		s->Signal();
	}

/* Semaphore APIs */
extern "C" {
int semaphore_wait(int sem_id, int time_ticks)
	{
	if (time_ticks < WAIT_FOREVER)
		return BAD_TIME_INTERVAL;
	if (TUint(sem_id) >= TUint(PSemaphore::NumSemaphores))
		return BAD_SEM_ID;
	PSemaphore* s = PSemaphore::SemaphoreTable + sem_id;
	PThread* t = (PThread*)NKern::CurrentThread();
	TInt r = OK;
	NKern::Lock();
	if (time_ticks == NO_WAIT)
		{
		if (s->iCount <= 0)
			r = TIMED_OUT;
		else
			--s->iCount;
		NKern::Unlock();
		return r;
		}
	if (--s->iCount < 0)
		{
		NKern::NanoBlock(time_ticks>0 ? time_ticks : 0, PThread::EWaitSemaphore, s);
		s->iWaitQ.Add(t);
		NKern::PreemptionPoint();
		if (t->iReturnValue == KErrTimedOut)
			r = TIMED_OUT;
		}
	NKern::Unlock();
	return r;
	}

int semaphore_signal(int sem_id)
	{
	if (TUint(sem_id) >= TUint(PSemaphore::NumSemaphores))
		return BAD_SEM_ID;
	PSemaphore* s = PSemaphore::SemaphoreTable + sem_id;
	TInt c = NKern::CurrentContext();
	if (c == NKern::EInterrupt)
		{
		s->ISRSignal();
		return OK;
		}
	NKern::Lock();
	s->Signal();
	NKern::Unlock();
	return OK;
	}

void init_personality(void)
	{
	__KTRACE_OPT(KBOOT,Kern::Printf("Starting example personality"));

	PMemMgr::Create(pool_list);
	PTimer::CreateAll();
	PSemaphore::CreateAll();
	PThread::CreateAll(task_list);
	}
}

/******************************************************************************
 * Communication with EPOC
 ******************************************************************************/
TPMsgQ* TPMsgQ::ThePMsgQ;

TPMsgQ::TPMsgQ(TDfcFn aFunction, TAny* aPtr, TDfcQue* aDfcQ, TInt aPriority)
	:	TDfc(aFunction, aPtr, aDfcQ, aPriority),
		iFirstMsg(NULL),
		iLastMsg(NULL),
		iReady(EFalse)
	{
	}

extern "C" void send_to_epoc(msghdr* m)
	{
	TPMsgQ* q = TPMsgQ::ThePMsgQ;
	m->next = NULL;
	m->sending_task_id = current_task_id();
	NKern::Lock();
	msghdr* l = q->iLastMsg;
	q->iLastMsg = m;
	if (l)
		{
		l->next = m;
		NKern::Unlock();
		return;	// queue was not empty so thread can't be waiting
		}
	q->iFirstMsg = m;
	if (q->iReady)
		{
		q->iReady = EFalse;
		q->DoEnque();
		}
	NKern::Unlock();
	}

void TPMsgQ::Receive()
	{
	NKern::Lock();
	if (iFirstMsg)
		DoEnque();
	else
		iReady = ETrue;
	NKern::Unlock();
	}

msghdr* TPMsgQ::Get()
	{
	NKern::Lock();
	msghdr* m = iFirstMsg;
	if (m)
		{
		iFirstMsg = m->next;
		if (!iFirstMsg)
			iLastMsg = NULL;
		}
	NKern::Unlock();
	return m;
	}

void TPMsgQ::CancelReceive()
	{
	iReady = EFalse;
	Cancel();
	}


