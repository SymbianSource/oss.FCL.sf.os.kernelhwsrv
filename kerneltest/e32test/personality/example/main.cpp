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
// e32test\personality\example\main.cpp
// Test code for example RTOS personality.
// 
//

#include <kernel/kern_priv.h>
#include <personality/example/personality.h>

#ifdef __cplusplus
extern "C" {
#endif

#define	OC_TASK				0
#define	L2_TASK				1
#define	RR_TASK				2
#define NONEXISTENT_TASK	3
#define TM_TASK				4
#define	TASK1				6
#define	TASK2				7
#define	TASK3				8
#define	TASK4				9
#define	L1_TASK				10

void oo_overall_control(void);
void l1_task_entry(void);
void l2_task_entry(void);
void rr_task_entry(void);
void tm_task_entry(void);
void task1_entry(void);
void task2_entry(void);
void task3_entry(void);
void task4_entry(void);

typedef void (*isr_entry)(unsigned);

extern int start_random_isr(isr_entry vector);
extern void stop_random_isr(void);

const taskinfo task_list[] =
	{

	/*		entry_pt,			priority, stack_size,	task_id, auto_start	*/

		{	&oo_overall_control,	120,	1024,		OC_TASK,	1	},
		{	&l2_task_entry,			236,	1024,		L2_TASK,	0	},
		{	&rr_task_entry,			224,	1024,		RR_TASK,	0	},
		{	&tm_task_entry,			240,	1024,		TM_TASK,	0	},
		{	&task1_entry,			112,	1024,		TASK1,		0	},
		{	&task2_entry,			112,	1024,		TASK2,		0	},
		{	&task3_entry,			112,	1024,		TASK3,		0	},
		{	&task4_entry,			112,	1024,		TASK4,		0	},
		{	&l1_task_entry,			244,	1024,		L1_TASK,	0	},
	/* terminator */
		{	0,						0,		0,			0,			0	}
	};

const poolinfo pool_list[] =
	{
	/*	block size,		block count	*/
		{	32,			256		},
		{	64,			256		},
		{	128,		128		},
		{	256,		64		},
		{	512,		32		},
	/* terminator */
		{	0,			0		}
	};

const int timer_count = 8;
const int semaphore_count = 2;

#define TM_TIMER		0

#define TM_INIT_DELAY	1000
#define TM_PERIOD		2

volatile unsigned next_random_id = 0;
volatile unsigned random_sem_signal_interval = 0;
volatile unsigned random_sem_signal_count = 0;
volatile unsigned random_send_interval = 0;
volatile unsigned random_send_count = 0;
volatile unsigned tmcount = 0;
volatile int t1func = 0;
volatile int t2func = 0;
volatile int t3func = 0;
volatile int t4func = 0;

#define TEST_SEM		0
#define	ISR_SEM			1

#define MSG_ID_INIT		1
#define MSG_ID_RUN		2
#define MSG_ID_RUN_P	3
#define	MSG_ID_RND_ISR	4
#define MSG_ID_DONE		5
#define	MSG_ID_DATA		6
#define	MSG_ID_FLUSH	7
#define MSG_ID_SEM_RPT	8
#define MSG_ID_RCV_RPT	9
#define MSG_ID_TM_RPT	10

typedef struct _run_msg
	{
	msghdr			header;
	int				task_id;
	unsigned		tmcount;
	int				parameter;
	} run_msg;

typedef struct _random_isr_msg
	{
	msghdr			header;
	unsigned		random_isr_number;
	unsigned		extra;
	} random_isr_msg;

typedef struct _data_msg
	{
	msghdr			header;
	int				length;
	unsigned char	checksum;
	unsigned char	data[1];
	} data_msg;

typedef struct _report_msg
	{
	msghdr			header;
	int				pad;
	unsigned		count;
	unsigned		ok_count;
	unsigned		bad_count;
	} report_msg;

void busy_wait(unsigned ticks)
	{
	unsigned t0 = tmcount;
	while ((tmcount - t0) < ticks)
		{}
	}

void send_run_signal()
	{
	run_msg* m = (run_msg*)alloc_mem_block(sizeof(run_msg));
	assert(m);
	m->header.msg_id = MSG_ID_RUN;
	m->task_id = current_task_id();
	m->tmcount = tmcount;
	int r = send_msg(OC_TASK, &m->header);
	assert(r == OK);
	}

void send_run_signal_p(int parameter)
	{
	run_msg* m = (run_msg*)alloc_mem_block(sizeof(run_msg));
	assert(m);
	m->header.msg_id = MSG_ID_RUN_P;
	m->task_id = current_task_id();
	m->tmcount = tmcount;
	m->parameter = parameter;
	int r = send_msg(OC_TASK, &m->header);
	assert(r == OK);
	}

void tsend_run_signal_p(int task_id, int parameter)
	{
	run_msg* m = (run_msg*)alloc_mem_block(sizeof(run_msg));
	assert(m);
	m->header.msg_id = MSG_ID_RUN_P;
	m->task_id = current_task_id();
	m->tmcount = tmcount;
	m->parameter = parameter;
	int r = send_msg(task_id, &m->header);
	assert(r == OK);
	}

void check_no_signal()
	{
	msghdr* m = NULL;
	int r = recv_msg(&m, NO_WAIT);
	assert(r == TIMED_OUT);
	}

unsigned check_for_signal(int task_id)
	{
	msghdr* m = NULL;
	int r = recv_msg(&m, NO_WAIT);
	assert(r == OK);
	assert(m->msg_id == MSG_ID_RUN);
	run_msg* rm = (run_msg*)m;
	assert(rm->task_id == task_id);
	unsigned tmc = rm->tmcount;
	free_mem_block(m);
	return tmc;
	}

int check_for_signal_p(int task_id, int task_id2, unsigned* pt)
	{
	msghdr* m = NULL;
	int r = recv_msg(&m, NO_WAIT);
	assert(r == OK);
	assert(m->msg_id == MSG_ID_RUN_P);
	run_msg* rm = (run_msg*)m;
	assert(rm->task_id == task_id);
	assert(m->sending_task_id == task_id2);
	r = rm->parameter;
	if (pt)
		*pt = rm->tmcount;
	free_mem_block(m);
	return r;
	}

int wait_for_signal_p(int task_id, unsigned* pt)
	{
	msghdr* m = NULL;
	int r = recv_msg(&m, WAIT_FOREVER);
	assert(r == OK);
	assert(m->msg_id == MSG_ID_RUN_P);
	run_msg* rm = (run_msg*)m;
	assert(rm->task_id == task_id);
	r = rm->parameter;
	if (pt)
		*pt = rm->tmcount;
	free_mem_block(m);
	return r;
	}

void resume_4(int t1, int t2, int t3, int t4)
	{
	if (t1>=0)
		assert(resume_task(t1)==OK);
	if (t2>=0)
		assert(resume_task(t2)==OK);
	if (t3>=0)
		assert(resume_task(t3)==OK);
	if (t4>=0)
		assert(resume_task(t4)==OK);
	}

void check_signal_4(int t1, int t2, int t3, int t4)
	{
	if (t1>=0)
		check_for_signal(t1);
	else
		check_no_signal();
	if (t2>=0)
		check_for_signal(t2);
	else
		check_no_signal();
	if (t3>=0)
		check_for_signal(t3);
	else
		check_no_signal();
	if (t4>=0)
		check_for_signal(t4);
	else
		check_no_signal();
	}

void check_for_multiple_signals(int task_id, int count)
	{
	unsigned t = check_for_signal(task_id);
	while (--count)
		{
		unsigned t2 = check_for_signal(task_id);
		assert(t2 - t >= 1);
		t = t2;
		}
	}

int flush_signals(void)
	{
	int c = 0;
	for (;;)
		{
		msghdr* m = NULL;
		int r = recv_msg(&m, NO_WAIT);
		if (r == TIMED_OUT)
			break;
		assert(r == OK);
		assert(m->msg_id == MSG_ID_RUN);
		free_mem_block(m);
		++c;
		}
	return c;
	}

void test_mem_pool(size_t size, int count, void** chain)
	{
	int i, fill;
	void *b, *bb, *c;
	c = *chain;
	for (i=0; i<count; ++i)
		{
		b = alloc_mem_block(size);
		assert(b != NULL);
		fill = (int)(size>>5);
		fill += 29;
		fill *= fill;
		fill &= 0xff;
		memset(b, fill, size);
		*(void**)b = c;
		((int*)b)[1] = (int)size;
		c = b;
		}
	bb = alloc_mem_block(size);
	assert(bb == NULL);
	*chain = c;
	}

void check_blocks(void* chain)
	{
	void* p = chain;
	while (p)
		{
		unsigned char *q, *qq;
		int size, fill, x;
		size = ((int*)p)[1];
		fill = (size>>5)+29;
		fill = (fill*fill)&0xff;
		q = (unsigned char*)p + sizeof(void*) + sizeof(int);
		qq = (unsigned char*)p + size;
		x = 0;
		while (q<qq)
			x |= (*q++ ^ fill);
		assert(x==0);
		p = *(void**)p;
		}
	}

int free_blocks(void* chain)
	{
	void* p = chain;
	int c = 0;
	while (p)
		{
		void* n = *(void**)p;
		free_mem_block(p);
		p = n;
		++c;
		}
	return c;
	}

void test_mem_mgr(void)
	{
	void* chain = NULL;
	const poolinfo* pi = pool_list;
	int nblocks = 0;
	int nfreed = 0;
	for (; pi->block_size; ++pi)
		{
		nblocks += pi->block_count;
		test_mem_pool(pi->block_size, pi->block_count, &chain);
		}
	check_blocks(chain);
	nfreed = free_blocks(chain);
	assert(nfreed == nblocks);
	chain = NULL;
	for (--pi; pi >= pool_list; --pi)
		test_mem_pool(pi->block_size, pi->block_count, &chain);
	check_blocks(chain);
	nfreed = free_blocks(chain);
	assert(nfreed == nblocks);
	chain = NULL;
	kprintf("Memory Manager Test OK");
	}

void test_suspend_1(void)
	{
	unsigned t1, t2, t3;
	int r;
	t1 = tmcount;
	delay(5*TM_PERIOD);
	t2 = tmcount;
	assert( ((int)t2)-((int)t1) >= 5 );
	r = suspend_task(TM_TASK);
	assert(r == OK);
	t1 = tmcount;
	delay(5*TM_PERIOD);
	t2 = tmcount;
	assert(t2==t1);
	r = resume_task(TM_TASK);
	assert(r == OK);
	t3 = tmcount;
	assert( ((int)t3)-((int)t2) >= 5 );

	r = suspend_task(TM_TASK);
	assert(r == OK);
	r = suspend_task(TM_TASK);
	assert(r == OK);
	t1 = tmcount;
	delay(5*TM_PERIOD);
	t2 = tmcount;
	assert(t2==t1);
	r = resume_task(TM_TASK);
	assert(r == OK);
	t3 = tmcount;
	assert(t3==t2);
	r = resume_task(TM_TASK);
	assert(r == OK);
	t3 = tmcount;
	assert( ((int)t3)-((int)t2) >= 5 );

	r = suspend_task(-1);
	assert(r == BAD_TASK_ID);
	r = suspend_task(300);
	assert(r == BAD_TASK_ID);
	r = suspend_task(NONEXISTENT_TASK);
	assert(r == BAD_TASK_ID);
	r = resume_task(-1);
	assert(r == BAD_TASK_ID);
	r = resume_task(300);
	assert(r == BAD_TASK_ID);
	r = resume_task(NONEXISTENT_TASK);
	assert(r == BAD_TASK_ID);

	kprintf("test_suspend_1 OK");
	}

void test_priority_scheduling(void)
	{
	int init_pri = get_task_priority(current_task_id());
	resume_4(TASK1, TASK2, TASK3, TASK4);
	delay(80*TM_PERIOD);
	check_for_multiple_signals(TASK1, 50);	// check no timeslicing
	assert(flush_signals()<=31);
	suspend_task(TASK1);
	delay(80*TM_PERIOD);
	check_for_multiple_signals(TASK2, 50);	// check no timeslicing
	assert(flush_signals()<=31);
	suspend_task(TASK2);
	delay(80*TM_PERIOD);
	check_for_multiple_signals(TASK3, 50);	// check no timeslicing
	assert(flush_signals()<=31);
	suspend_task(TASK3);
	delay(1);
	check_for_signal(TASK4);
	assert(flush_signals()<=1);

	t1func = 1;
	t2func = 1;
	t3func = 1;
	t4func = 1;

	resume_4(TASK1, TASK2, TASK3, TASK4);
	delay(10);
	flush_signals();

	resume_4(TASK3, TASK2, TASK4, TASK1);
	delay(10);
	check_signal_4(TASK3, TASK2, TASK4, TASK1);
	check_no_signal();
	resume_4(TASK1, TASK2, TASK3, TASK4);
	check_no_signal();	// all lower priority so don't run
	set_task_priority(TASK2, 255);		// higher than current task so run immediately
	check_for_signal(TASK2);
	set_task_priority(TASK4, 116);
	check_no_signal();	// all lower priority so don't run
	delay(10);
	check_for_signal(TASK4);
	check_for_signal(TASK1);
	check_for_signal(TASK3);
	set_task_priority(TASK1, 116);
	set_task_priority(TASK2, 116);
	set_task_priority(TASK3, 116);
	set_task_priority(TASK4, 116);
	resume_4(TASK1, TASK2, TASK3, TASK4);
	set_task_priority(current_task_id(), 112);	// drop current task priority
	assert(get_task_priority(current_task_id())==112);
	check_signal_4(TASK1, TASK2, TASK3, TASK4);
	set_task_priority(current_task_id(), init_pri);
	assert(get_task_priority(current_task_id())==init_pri);
	
	kprintf("test_priority_scheduling OK");
	}

unsigned sem_test(int task_id)
	{
	int r = semaphore_signal(TEST_SEM);
	assert(r==OK);
	return check_for_signal(task_id);
	}

unsigned sem_test_p(int task_id, int parameter)
	{
	unsigned t;
	int r = semaphore_signal(TEST_SEM);
	assert(r==OK);
	r = check_for_signal_p(task_id, task_id, &t);
	assert(r == parameter);
	return t;
	}

unsigned sem_test_pt(int task_id, int parameter)
	{
	unsigned t;
	int r = semaphore_signal(TEST_SEM);
	assert(r==OK);
	r = check_for_signal_p(task_id, task_id, &t);
	assert(r == parameter);
	return t;
	}

void test_semaphore(void)
	{
	unsigned t1, t2, t3;
	int r;
	int init_pri = get_task_priority(current_task_id());
	set_task_priority(TASK1, 128);
	set_task_priority(TASK2, 128);
	set_task_priority(TASK3, 128);
	set_task_priority(TASK4, 128);
	t1func = 2;
	t2func = 2;
	t3func = 2;
	t4func = 2;
	resume_4(TASK1, TASK2, TASK3, TASK4);
	delay(10);		// let tasks wait on semaphore
	check_no_signal();
	sem_test(TASK1);	// test they are released in same order
	sem_test(TASK2);
	sem_test(TASK3);
	sem_test(TASK4);
	check_no_signal();
	set_task_priority(TASK3, 132);	// test highest priority is released first
	sem_test(TASK3);
	sem_test(TASK3);
	suspend_task(TASK3);		// test suspended task doesn't contend for semaphore
	sem_test(TASK1);
	sem_test(TASK2);
	sem_test(TASK4);
	sem_test(TASK1);
	suspend_task(TASK2);
	sem_test(TASK4);
	sem_test(TASK1);
	sem_test(TASK4);
	set_task_priority(TASK2, 136);	// change priority while suspended
	sem_test(TASK1);
	sem_test(TASK4);
	sem_test(TASK1);
	resume_task(TASK2);
	sem_test(TASK2);
	sem_test(TASK2);	// test new highest priority task acquires semaphore first
	delay(100*TM_PERIOD);
	check_no_signal();	// check waits don't time out

	t2func = 3;			// switch over to timed waits for task 2
	t1 = sem_test(TASK2);			// get one last message of previous type
	delay(5*TM_PERIOD);
	t2 = sem_test_p(TASK2, OK);		// signal after half the timeout and check OK
	delay(11*TM_PERIOD);			// wait for > timeout
	r = check_for_signal_p(TASK2, TASK2, &t3);
	assert(r == TIMED_OUT);
	kprintf("t2-t1=%d t3-t2=%d", t2-t1, t3-t2);
	assert(t2-t1 >= 5);
	assert(t3-t2 >= 10);
	sem_test_p(TASK2, OK);
	resume_task(TASK3);

	set_task_priority(current_task_id(), 176);	// raise current task priority
	semaphore_signal(TEST_SEM);		// signal semaphore 4 times - should release all 4 waiting threads
	semaphore_signal(TEST_SEM);
	semaphore_signal(TEST_SEM);
	semaphore_signal(TEST_SEM);
	set_task_priority(current_task_id(), init_pri);	// let tasks run
	r = check_for_signal_p(TASK2, TASK2, NULL);
	assert(r == OK);
	check_for_signal(TASK3);
	check_for_signal(TASK4);
	check_for_signal(TASK1);
	set_task_priority(current_task_id(), 176);	// raise current task priority
	busy_wait(11);					// let semaphore wait time out
	t1func = 4;						// switch all threads over
	t2func = 4;						//
	t3func = 4;						//
	t4func = 4;						//
	semaphore_signal(TEST_SEM);		// signal semaphore 3 times - should release other 3 waiting threads
	semaphore_signal(TEST_SEM);
	semaphore_signal(TEST_SEM);
	set_task_priority(current_task_id(), init_pri);	// let tasks run
	r = check_for_signal_p(TASK2, TASK2, NULL);
	assert(r == TIMED_OUT);
	check_for_signal(TASK3);
	check_for_signal(TASK4);
	check_for_signal(TASK1);

	kprintf("test_semaphore OK");
	}

void test_message_queue(void)
	{
	unsigned t1, t2, t3, t4;
	int tid, p, r;
	int init_pri = get_task_priority(current_task_id());
	p = 0;
	t1 = 0;
	for (tid = TASK1; tid <= TASK4; ++tid)
		{
		for (p = 1; p; p<<=1)
			{
			tsend_run_signal_p(tid, p);
			r = check_for_signal_p(OC_TASK, tid, NULL);
			assert(r == p);
			}
		}
	check_no_signal();
	set_task_priority(current_task_id(), 176);	// raise current task priority
	set_task_priority(TASK4, 144);	// change task priorities while they are waiting
	set_task_priority(TASK3, 140);
	set_task_priority(TASK2, 136);
	set_task_priority(TASK1, 132);
	t1func = 5;	// switch task 1 to timed waits
	for (tid = TASK1; tid <= TASK4; ++tid)
		{
		for (p = 0; p<0x40000000; p+=(0x413b9cb+tid))
			{
			tsend_run_signal_p(tid, p);	// let multiple messages accumulate on the queues
			}
		}
	check_no_signal();
	set_task_priority(current_task_id(), init_pri);	// let tasks run
	kprintf("init_pri=%d",init_pri);
	for (tid = TASK4; tid >= TASK1; --tid)
		{
		for (p = 0; p<0x40000000; p+=(0x413b9cb+tid))
			{
			r = check_for_signal_p(OC_TASK, tid, &t1);
			assert(r == p);
			}
		}

	delay(5*TM_PERIOD);
	tsend_run_signal_p(TASK1, p);		// send after half timeout
	r = check_for_signal_p(OC_TASK, TASK1, &t2);
	assert(r == p);
	delay(11*TM_PERIOD);				// wait for > timeout
	tsend_run_signal_p(TASK1, ~p);		// send after timeout
	r = check_for_signal_p(TASK1, TASK1, &t3);
	assert(r == TIMED_OUT);
	kprintf("t2-t1=%d t3-t2=%d", t2-t1, t3-t2);
	assert(t2-t1 >= 5);
	assert(t3-t2 >= 10);
	r = check_for_signal_p(OC_TASK, TASK1, &t4);
	assert(r == ~p);
	assert(t4-t3 <= 1);
	t1func = 6;						// switch task 1 to timed semaphore wait
	t2func = 7;						// switch task 2 to timed queue wait
	t3func = 8;						//
	t4func = 8;						//
	for (tid = TASK1; tid <= TASK4; ++tid)
		{
		tsend_run_signal_p(tid, 0);
		r = check_for_signal_p(OC_TASK, tid, NULL);
		assert(r == 0);
		}
	check_no_signal();

	kprintf("test_message_queue OK");
	}

void random_isr(unsigned n)
	{
	random_isr_msg* m;
	unsigned extra = 1;
	unsigned count = 1;
	if (!(n%11))
		++count;
	if (!(n%13))
		++count;
	while (count--)
		{
		m = (random_isr_msg*)alloc_mem_block(sizeof(random_isr_msg));
		m->header.msg_id = MSG_ID_RND_ISR;
		m->random_isr_number = n;
		extra *= n;
		m->extra = extra;
		send_msg(L1_TASK, &m->header);
		}
	if (random_sem_signal_count && !--random_sem_signal_count)
		{
		random_sem_signal_count = random_sem_signal_interval;
		semaphore_signal(ISR_SEM);
		}
	}

void flush_queue(msghdr** f, msghdr** l, msghdr* tm)
	{
	msghdr* m = *f;
	*f = NULL;
	*l = NULL;
	send_to_epoc(tm);
	while (m)
		{
		msghdr* n = m->next;
		send_to_epoc(m);
		m = n;
		}
	}

void l1_task_entry(void)
	{
	msghdr* first = NULL;
	msghdr* last = NULL;
	unsigned state = 0;
	unsigned extra_count = 0;
	unsigned extra_value = 0;
	assert(current_task_id() == L1_TASK);
	kprintf("L1_TASK running");
	for (;;)
		{
		msghdr* m = NULL;
		int r = recv_msg(&m, WAIT_FOREVER);
		assert(r == OK);
		switch (m->msg_id)
			{
			case MSG_ID_RND_ISR:
				{
				random_isr_msg* rm = (random_isr_msg*)m;
				assert(m->sending_task_id == TASK_ID_ISR);
				assert(rm->random_isr_number == next_random_id);
				if (state == 0)
					{
					extra_count = 0;
					if (!(next_random_id % 11))
						++extra_count;
					if (!(next_random_id % 13))
						++extra_count;
					extra_value = next_random_id;
					}
				else if (state > 0)
					{
					extra_value *= next_random_id;
					}
				assert(rm->extra == extra_value);
				if (++state > extra_count)
					state = 0;
				if (state == 0)
					++next_random_id;
				if (rm->random_isr_number == 0)
					send_msg(OC_TASK, m), m=NULL;
				if (state == 1 && extra_count == 2 && m)
					{
					flush_queue(&first, &last, m);
					m = NULL;
					}
				if (random_send_count && !--random_send_count)
					{
					random_send_count = random_send_interval;
					if (m)
						send_msg(TASK2, m), m=NULL;
					}
				break;
				}
			case MSG_ID_DATA:
				m->next = NULL;
				if (last)
					last->next = m;
				else
					first = m;
				last = m;
				m = NULL;
				break;
			case MSG_ID_FLUSH:
				flush_queue(&first, &last, m);
				m = NULL;
				break;
			default:
				kprintf("L1<-%08x",m->msg_id);
				break;
			}
		if (m)
			free_mem_block(m);
		}
	}

void l2_task_entry(void)
	{
	assert(current_task_id() == L2_TASK);
	kprintf("L2_TASK running");
	for (;;)
		{
		msghdr* m = NULL;
		int r = recv_msg(&m, WAIT_FOREVER);
		assert(r == OK);
		switch (m->msg_id)
			{
			case MSG_ID_DATA:
				{
				data_msg* dm = (data_msg*)m;
				int i;
				unsigned char cs = 0;
				for (i=0; i<dm->length; ++i)
					cs = (unsigned char)(cs + dm->data[i]);
				dm->checksum = cs;
				send_msg(L1_TASK, m);
				m=NULL;
				break;
				}
			default:
				kprintf("L2<-%08x",m->msg_id);
				break;
			}
		if (m)
			free_mem_block(m);
		}
	}

void rr_task_entry(void)
	{
	assert(current_task_id() == RR_TASK);
	kprintf("RR_TASK running");
	for (;;)
		{
		msghdr* m = NULL;
		int r = recv_msg(&m, WAIT_FOREVER);
		assert(r == OK);
		switch (m->msg_id)
			{
			case MSG_ID_DATA:
				send_msg(L2_TASK, m);
				m=NULL;
				break;
			default:
				kprintf("RR<-%08x",m->msg_id);
				break;
			}
		if (m)
			free_mem_block(m);
		}
	}

void tm_task_entry(void)
	{
	assert(current_task_id() == TM_TASK);
	kprintf("TM_TASK running");
	for (;;)
		{
		msghdr* m = NULL;
		int r = recv_msg(&m, WAIT_FOREVER);
		assert(r == OK);
		switch (m->msg_id)
			{
			case MSG_ID_TIMEOUT:
				tmcount = ((timer_msg*)m)->count;
				assert(m->sending_task_id == TASK_ID_ISR);
				if (!(tmcount & 255))
					{
					report_msg* rpt = (report_msg*)alloc_mem_block(sizeof(report_msg));
					rpt->header.msg_id = MSG_ID_TM_RPT;
					rpt->count = tmcount;
					rpt->ok_count = 0;
					rpt->bad_count = 0;
					send_to_epoc(&rpt->header);
					}
				break;
			default:
				kprintf("TM<-%08x",m->msg_id);
				break;
			}
		free_mem_block(m);
		}
	}

void generic_task(volatile int* f)
	{
	int r;
	msghdr* m;
	unsigned t1, t2;
	unsigned count = 0;
	unsigned ok_count = 0;
	unsigned bad_count = 0;
	while (*f==0)
		{
		send_run_signal();
		busy_wait(1);
		}
	while (*f==1)
		{
		send_run_signal();
		suspend_task(current_task_id());
		}
	while (*f==2)
		{
		r = semaphore_wait(TEST_SEM, WAIT_FOREVER);
		assert(r == OK);
		send_run_signal();
		}
	while (*f==3)
		{
		r = semaphore_wait(TEST_SEM, 10*TM_PERIOD);
		assert(r==OK || r==TIMED_OUT);
		send_run_signal_p(r);
		}
	while (*f==4)
		{
		r = recv_msg(&m, WAIT_FOREVER);
		assert(r==OK);
		assert(m->sending_task_id == OC_TASK);
		r = send_msg(OC_TASK, m);
		assert(r == OK);
		}
	while (*f==5)
		{
		r = recv_msg(&m, 10*TM_PERIOD);
		assert(r==OK || r==TIMED_OUT);
		if (r == OK)
			{
			assert(m->sending_task_id == OC_TASK);
			r = send_msg(OC_TASK, m);
			assert(r == OK);
			}
		else
			send_run_signal_p(r);
		}
	while (*f==6)
		{
		t1 = tick_count();
		r = semaphore_wait(ISR_SEM, 5);
		t2 = tick_count() - t1;
		if (r == TIMED_OUT && t2<5)
			{
			kprintf("SEM timed out too soon: %d", t2);
			++bad_count;
			}
		if (r == OK)
			++ok_count;
		++count;
		if (!(count & 0xff))
			{
			report_msg* rpt = (report_msg*)alloc_mem_block(sizeof(report_msg));
			rpt->header.msg_id = MSG_ID_SEM_RPT;
			rpt->count = count;
			rpt->ok_count = ok_count;
			rpt->bad_count = bad_count;
			send_to_epoc(&rpt->header);
			}
		}
	while (*f==7)
		{
		t1 = tick_count();
		r = recv_msg(&m, 5);
		t2 = tick_count() - t1;
		if (r == TIMED_OUT && t2<5)
			{
			kprintf("RECV timed out too soon: %d", t2);
			++bad_count;
			}
		if (r==OK)
			++ok_count, free_mem_block(m);
		++count;
		if (!(count & 0xff))
			{
			report_msg* rpt = (report_msg*)alloc_mem_block(sizeof(report_msg));
			rpt->header.msg_id = MSG_ID_RCV_RPT;
			rpt->count = count;
			rpt->ok_count = ok_count;
			rpt->bad_count = bad_count;
			send_to_epoc(&rpt->header);
			}
		}
	kprintf("Task %d finished", current_task_id());
	for(;;)
		suspend_task(current_task_id());
	}

void task1_entry(void)
	{
	assert(current_task_id() == TASK1);
	generic_task(&t1func);
	}

void task2_entry(void)
	{
	assert(current_task_id() == TASK2);
	generic_task(&t2func);
	}

void task3_entry(void)
	{
	assert(current_task_id() == TASK3);
	generic_task(&t3func);
	}

void task4_entry(void)
	{
	assert(current_task_id() == TASK4);
	generic_task(&t4func);
	}



void oo_overall_control(void)
	{
	int r;
	msghdr* m;
	random_isr_msg* rm;
	unsigned t1, t2, rss_interval;
	kprintf("OC_TASK running");
	assert(current_task_id() == OC_TASK);
	resume_task(L2_TASK);
	resume_task(RR_TASK);
	resume_task(TM_TASK);
	test_mem_mgr();

	kprintf("Wait for init msg");
	r = recv_msg(&m, WAIT_FOREVER);
	assert(r == OK);
	assert(m->msg_id == MSG_ID_INIT);
	assert(m->sending_task_id == TASK_ID_UNKNOWN);
	free_mem_block(m);
	kprintf("Received init msg");

	r = start_periodic_timer(TM_TIMER, TM_TASK, TM_INIT_DELAY, TM_PERIOD, NULL);
	assert(r == OK);
	delay(TM_INIT_DELAY-10);
	assert(tmcount == 0);
	delay(10*TM_PERIOD+20);
	assert(tmcount > 0);
	test_suspend_1();
	test_priority_scheduling();
	test_semaphore();
	test_message_queue();

	resume_task(L1_TASK);
	r = start_random_isr(&random_isr);
	if (r != OK)
		goto no_random_isr;

	r = recv_msg(&m, WAIT_FOREVER);
	assert(r == OK);
	assert(m->msg_id == MSG_ID_RND_ISR);
	assert(m->sending_task_id == L1_TASK);
	rm = (random_isr_msg*)m;
	assert(rm->random_isr_number == 0);
	free_mem_block(m);
	t1 = next_random_id;
	delay(1024);
	t2 = next_random_id;
	kprintf("%d random ISRs in 1024 ticks", t2-t1);
	rss_interval = (5*(t2-t1)+512)/1024;
	set_task_priority(TASK1, 196);	// needs to be higher than DfcThread1
	set_task_priority(TASK2, 196);
	random_sem_signal_interval = rss_interval;
	random_sem_signal_count = rss_interval;
	random_send_interval = rss_interval;
	random_send_count = rss_interval;

no_random_isr:
	m = (msghdr*)alloc_mem_block(sizeof(msghdr));
	m->msg_id = MSG_ID_DONE;
	send_to_epoc(m);
	kprintf("All tests completed OK");
	for (;;)
		{
		int r = recv_msg(&m, WAIT_FOREVER);
		assert(r == OK);
		switch (m->msg_id)
			{
			case MSG_ID_DATA:
				send_msg(RR_TASK, m);
				m=NULL;
				break;
			case MSG_ID_FLUSH:
				send_msg(L1_TASK, m);
				m=NULL;
				break;
			case MSG_ID_DONE:
				stop_random_isr();
				stop_timer(TM_TIMER);
				suspend_task(L1_TASK);
				suspend_task(L2_TASK);
				suspend_task(RR_TASK);
				suspend_task(TM_TASK);
				suspend_task(TASK1);
				suspend_task(TASK2);
				suspend_task(TASK3);
				suspend_task(TASK4);
				break;
			default:
				kprintf("OC<-%08x",m->msg_id);
				break;
			}
		if (m)
			free_mem_block(m);
		}
	}

#ifdef __cplusplus
}
#endif
