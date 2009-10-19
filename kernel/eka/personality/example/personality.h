/*
* Copyright (c) 2003-2009 Nokia Corporation and/or its subsidiary(-ies).
* All rights reserved.
* This component and the accompanying materials are made available
* under the terms of the License "Eclipse Public License v1.0"
* which accompanies this distribution, and is available
* at the URL "http://www.eclipse.org/legal/epl-v10.html".
*
* Initial Contributors:
* Nokia Corporation - initial contribution.
*
* Contributors:
*
* Description:
* e32\personality\example\personality.h
* External interface header file for example RTOS personality.
* This will be included by the real time application source files.
* 
* WARNING: This file contains some APIs which are internal and are subject
*          to change without notice. Such APIs should therefore not be used
*          outside the Kernel and Hardware Services package.
*/



/**
 @file
 @internalComponent
*/

#ifndef __PERSONALITY_H__
#define __PERSONALITY_H__

// should be separate C function
#define kprintf		Kern::Printf
#define assert(x)	__NK_ASSERT_ALWAYS(x)

#if defined(__GCC32__)
typedef unsigned long size_t;
#elif defined(__CW32__)
typedef unsigned int size_t;
#elif defined(__ARMCC__)
typedef unsigned int size_t;
#endif

#ifdef __cplusplus
extern "C" {
#endif

/* Return codes */
#define	OK					0
#define BAD_TASK_ID			(-100)
#define BAD_PRIORITY		(-101)
#define TIMED_OUT			(-102)
#define TIMER_IN_USE		(-103)
#define BAD_ENTRY_POINT		(-104)
#define BAD_STACK_SIZE		(-105)
#define	OUT_OF_MEMORY		(-106)
#define BAD_TIMER_ID		(-107)
#define BAD_TIME_INTERVAL	(-108)
#define BAD_SEM_ID			(-109)


/* Task priority range */
#define	MIN_TASK_PRIORITY	0
#define	MAX_TASK_PRIORITY	255

/* Task stack size */
#define MIN_STACK_SIZE		256

/* Special task IDs */
#define TASK_ID_ISR			(-1)
#define TASK_ID_UNKNOWN		(-2)		/* non-personality layer thread */

/* Time values */
#define	NO_WAIT				0			/* Never block */
#define	WAIT_FOREVER		(-1)		/* Never time out */

/* Special message IDs */
#define	MSG_ID_TIMEOUT		0x80000000

typedef void (*task_entry)(void);

/* Information required to create a task */
typedef struct _taskinfo
	{
	task_entry	entry_pt;
	int			priority;
	size_t		stack_size;
	short		task_id;
	short		auto_start;
	} taskinfo;

/* Externally supplied table of tasks which will be created at boot time */
extern const taskinfo task_list[];

/* Memory pool creation info */
typedef struct _poolinfo
	{
	size_t		block_size;
	unsigned	block_count;
	} poolinfo;

/* Externally supplied list of memory pools which will be created at boot time */
extern const poolinfo pool_list[];

/* Message header */
typedef struct _msghdr
	{
	struct _msghdr*	next;
	int				msg_id;
	int				sending_task_id;
	} msghdr;


/* Timers */
extern const int timer_count;

typedef struct _timer_msg
	{
	msghdr			header;
	void*			cookie;
	unsigned		count;
	} timer_msg;

/* Semaphores */
extern const int semaphore_count;


/* Task APIs */
extern int suspend_task(int id);						/* call from any thread */
extern int resume_task(int id);							/* call from any thread */
extern int get_task_priority(int id);					/* call from any thread */
extern int set_task_priority(int id, int priority);		/* call from any thread */
extern int current_task_id(void);						/* call from any thread or ISR */
extern void disable_preemption(void);					/* call from any thread */
extern void enable_preemption(void);					/* call from any thread */
extern int disable_interrupts(void);					/* call from any thread */
extern void restore_interrupts(int level);				/* call from any thread */

/* Memory management */
extern void* alloc_mem_block(size_t size);				/* call from any thread or ISR */
extern void free_mem_block(void* block);				/* call from any thread or ISR */

/* Messages */
extern int send_msg(int task_id, msghdr* msg);			/* call from any thread or ISR */
extern int recv_msg(msghdr** msgptr, int time_ticks);	/* call only from personality layer thread */

/* Timer APIs */
extern unsigned tick_count(void);						/* call from any thread or ISR */
extern void delay(int time_interval);					/* call from any thread */
extern int start_one_shot_timer(int timer_id, int task_id, int time_ticks, void* cookie);	/* call from any thread or ISR */
extern int start_periodic_timer(int timer_id, int task_id, int initial_time_ticks, int period_ticks, void* cookie);	/* call from any thread or ISR */
extern int stop_timer(int timer_id);					/* call from any thread or ISR */

/* Semaphore APIs */
extern int semaphore_wait(int sem_id, int time_ticks);	/* call only from personality layer thread */
extern int semaphore_signal(int sem_id);				/* call from any thread or ISR */

/* Initialisation */
extern void init_personality(void);						/* call from extension entry point */

/* Communication with EPOC */
extern void send_to_epoc(msghdr* msgptr);

#ifdef __cplusplus
}
#endif
#endif
