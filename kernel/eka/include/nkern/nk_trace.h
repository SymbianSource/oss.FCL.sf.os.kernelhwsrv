// Copyright (c) 1998-2009 Nokia Corporation and/or its subsidiary(-ies).
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
// e32\include\nkern\nk_trace.h
// 
// WARNING: This file contains some APIs which are internal and are subject
//          to change without notice. Such APIs should therefore not be used
//          outside the Kernel and Hardware Services package.
//

#ifndef NK_TRACE_H
#define NK_TRACE_H

/**
@internalComponent
*/
#define DEBUGPRINT	KPrintf

/**
@publishedPartner
@released
*/
#define DEBUGMASK (KDebugMask())
#define DEBUGNUM(x) (KDebugNum(x))

GLREF_C void KPrintf(const char*,...);

/**
@publishedPartner
@released
*/
IMPORT_C TInt  KDebugMask();
IMPORT_C TBool KDebugNum(TInt aBitNum);

#if defined(_DEBUG)
//#if (1)
/**
@publishedPartner
@released
*/
// __KTRACE_MASK only supports the first 32 debug trace bits
#define __KTRACE_MASK(a,p) {if((DEBUGMASK&(a)))p;}
#define __KTRACE_OPT(a,p) {if((DEBUGNUM(a)))p;}
#define __KTRACE_OPT2(a,b,p) {if( (DEBUGNUM(a)) || (DEBUGNUM(b)) )p;}

/**
@publishedPartner
@released
*/
// __KTRACE_ALL only supports the first 32 debug trace bits
#define __KTRACE_ALL(a,p) {if((DEBUGMASK&(a))==(a))p;}

/**
@publishedPartner
@released
*/
#define KHARDWARE	0 //0x00000001

/**
@publishedPartner
@released
*/
#define KBOOT		1 //0x00000002

/**
@publishedPartner
@released
*/
#define KSERVER		2 //0x00000004

/**
@publishedPartner
@released
*/
#define KMMU		3 //0x00000008

/**
@publishedPartner
@released
*/
#define KSEMAPHORE	4 //0x00000010

/**
@publishedPartner
@released
*/
#define KSCHED		5 //0x00000020

/**
@publishedPartner
@released
*/
#define KPROC		6 //0x00000040

/**
@publishedPartner
@released
*/
#define KEXEC		7 //0x00000080

/**
@publishedPartner
@released
*/
#define KDEBUGGER	8 //0x00000100  // for kernel-side debug agents

/**
@publishedPartner
@released
*/
#define KTHREAD		9 //0x00000200

/**
@publishedPartner
@released
*/
#define KDLL		10 //0x00000400

/**
@publishedPartner
@released
*/
#define KIPC		11 //0x00000800

/**
@publishedPartner
@released
*/
#define KPBUS1		12 //0x00001000

/**
@publishedPartner
@released
*/
#define KPBUS2		13 //0x00002000

/**
@publishedPartner
@released
*/
#define KPBUSDRV	14 //0x00004000

/**
@publishedPartner
@released
*/
#define KPOWER      15 //0x00008000

/**
@publishedPartner
@released
*/
#define KTIMING     16 //0x00010000

/**
@publishedPartner
@released
*/
#define KEVENT      17 //0x00020000

/**
@publishedPartner
@released
*/
#define KOBJECT		18 //0x00040000

/**
@publishedPartner
@released
*/
#define KDFC		19 //0x00080000

/**
@publishedPartner
@released
*/
#define KEXTENSION	20 //0x00100000

/**
@publishedPartner
@released
*/
#define KSCHED2		21 //0x00200000

/**
@publishedPartner
@released
*/
#define KLOCDRV		22 //0x00400000

/**
@publishedPartner
@released
*/
#define KFAIL		23 //0x00800000

/**
@publishedPartner
@released
*/
#define KTHREAD2	24 //0x01000000

/**
@publishedPartner
@released
*/
#define KDEVICE		25 //0x02000000

/**
@publishedPartner
@released
*/
#define KMEMTRACE	26 //0x04000000

/**
@publishedPartner
@released
*/
#define KDMA        27 //0x08000000

/**
@publishedPartner
@released
*/
#define KMMU2		28 //0x10000000

/**
@publishedPartner
@released
*/
#define KNKERN		29 //0x20000000

/**
@publishedPartner
@released
*/
#define KSCRATCH	30 //0x40000000	// reserved for temporary debugging

/**
@publishedPartner
@released
*/
#define KPANIC		31 //0x80000000

/**
@publishedPartner
@released
*/
#define KUSB		32 //0x00000001, index 1

/**
@publishedPartner
@released
*/
#define KUSBPSL		33 //0x00000002, index 1
 
/**
@internalComponent
@released	
*/
#define KNETWORK1	34 //0x00000004, index 1
 
/**
@internalComponent
@released
*/
#define KNETWORK2   35 //0x00000008, index 1

/**
@publishedPartner
@released
*/
#define KSOUND1		36 //0x00000010, index 1
 
/**
@publishedPartner
@released
*/
#define KUSBHOST	37 //0x00000020, index 1

/**
@publishedPartner
@released
*/
#define KUSBOTG		38 //0x00000040, index 1

/**
@publishedPartner
@released
*/
#define KUSBJOURNAL	39 //0x00000080, index 1

/**
@publishedPartner
@released
*/
#define KUSBHO		40 //0x00000100, index 1

/**
@publishedPartner
@released
*/
#define KRESMANAGER 41 //0x00000200, index 1

/**
@publishedPartner
@prototype
*/
#define KIIC 42 //0x00000400, index 1

/**
@publishedPartner
@prototype
*/
#define KHCR 43 //0x00000800, index 1

/**
@internalComponent
@released
*/
#define KREALTIME	63 //0x80000000, index 1

/**
@internalComponent
@released
*/
#define KPAGING		62 //0x40000000, index 1

/**
@internalComponent
@released
*/
#define KLOCDPAGING		61 //0x20000000, index 1

/**
@internalComponent
@released
*/
#define KDATAPAGEWARN	60 //0x10000000, index 1

/**
@internalComponent
@prototype
*/
#define KPCI	59 //0x08000000, index 1

/**
@internalComponent
@prototype
*/
#define KPIPE	58 //0x04000000, index 1

/**
@internalComponent
*/
#define KSCHED3	48 //0x00010000, index 1

// RESERVED: Trace bits 192 - 255 are reserved for licensee partners


/**
@publishedPartner
@released
*/
#define KALWAYS		-1     //0xffffffff
#define KMAXTRACE   (KNumTraceMaskWords*32-1)    // the maximum debug trace bit
#else
#define __KTRACE_OPT(a,p)
#define __KTRACE_ALL(a,p)
#define __KTRACE_OPT2(a,b,p)
#define KALWAYS		-1
#define KMAXTRACE   (KNumTraceMaskWords*32-1)
#endif


/**
@publishedPartner
@released
*/
#define DEBUGMASKWORD2	2

/*
words 0 & 1 of debug mask should be used for kernel debugging
word  2     of debug mask should be used to configure the ways the kernel behaves
word  3     of debug mask should be used to configure the ways the user library behaves
words 4 & 5 of debug mask should be used for file system debugging
words 6 & 7 of debug mask are reserved for licensees
*/

/**
@publishedPartner
@released
*/
#define KALLTHREADSSYSTEM	64 //0x00000001, index 2

/**
Suppresses console output (in EWSRV) for faster automated tests.
@publishedPartner
@released
*/
#define KTESTFAST			65 //0x00000002, index 2

/**
Suppresses anything which might disturb latency testing,
for example platsec diagnostics emitted with the system lock held.
@publishedPartner
@released
*/
#define KTESTLATENCY		66 //0x00000004, index 2

/**
When a crash occurs this flag determines whether the debugger executes.
If set the crash debugger will NOT operate (even if it's in rom).
If clear the crash debugger will run.
@publishedPartner
@released
 */
#define KDEBUGMONITORDISABLE	67 //0x00000008, index 2

/**
When a crash occurs this flag determines whether the logger executes.
If set the crash logger will NOT operate (even if it's in rom).
If clear the crash logger will run.
@publishedPartner
@released
 */
#define KCRASHLOGGERDISABLE	68 //0x00000010, index 2

/**
Delay scheduling of newly unblocked threads until the next timer tick
occurs, to check for thread priority dependencies. Part of the crazy
scheduler functionality.
@publishedPartner
@released
 */
#define KCRAZYSCHEDDELAY 69 //0x00000020, index 2

/**
Force page faults on kernel access to paged user-side data in a remote thread.

This is designed to help show up problems with device drivers that are not paging safe.

@internalComponent
@prototype
*/
#define KFORCEKUPAGEFAULTS		70 //0x00000040, index 2

/* Word 3 of debug mask : configures user library behaviour */

/**
@publishedPartner
@released
*/
#define KUSERHEAPTRACE		96 //0x00000001, index 3



#ifdef KFAIL

/**
@publishedPartner
@released
*/
#define __KTRACE_FAIL(r,p) {if ((r)!=KErrNone && (DEBUGNUM(KFAIL))) p;}

#else
#define __KTRACE_FAIL(r,p)
#endif

#include <e32btrace.h>

#ifdef __KERNEL_MODE__


class DBTraceFilter2;

/**
@internalComponent
*/
struct SBTraceData
	{
	TUint8 iFilter[256];
	BTrace::THandler iHandler;
	BTrace::TControlFunction iControl;
	DBTraceFilter2*volatile iFilter2;

	TBool CheckFilter2(TUint32 aUid);
	};

/**
@internalComponent
*/
extern SBTraceData BTraceData;

#if defined(_DEBUG) || defined(BTRACE_KERNEL_ALL)

#undef BTRACE_THREAD_IDENTIFICATION
#undef BTRACE_CPU_USAGE
#undef BTRACE_CHUNKS
#undef BTRACE_CODESEGS
#undef BTRACE_PAGING
#undef BTRACE_PAGING_MEDIA
#undef BTRACE_KERNEL_MEMORY
#undef BTRACE_RAM_ALLOCATOR
#undef BTRACE_FAST_MUTEX
#undef BTRACE_RESOURCE_MANAGER
#undef BTRACE_RESMANUS
#undef BTRACE_TRAWEVENT
#undef BTRACE_SYMBIAN_KERNEL_SYNC
#undef BTRACE_FLEXIBLE_MEM_MODEL
#undef BTRACE_CLIENT_SERVER
#undef BTRACE_REQUESTS


/**
If defined, code for BTrace category BTrace::EThreadIdentification
is compiled into the kernel.
@publishedPartner
@released
*/
#define BTRACE_THREAD_IDENTIFICATION

/**
If defined, code for BTrace category BTrace::ECpuUsage
is compiled into the kernel.
@publishedPartner
@released
*/
#define BTRACE_CPU_USAGE

/**
If defined, code for BTrace category BTrace::EChunks is compiled into the
kernel.
@publishedPartner
@released
*/
#define BTRACE_CHUNKS

/**
If defined, code for BTrace category BTrace::ECodeSegs is compiled into the
kernel.
@publishedPartner
@released
*/
#define BTRACE_CODESEGS

/**
If defined, code for BTrace category BTrace::EPaging is compiled into the
kernel.
@publishedPartner
@released 9.3
*/
#define BTRACE_PAGING

/**
If defined, code for BTrace category BTrace::EPagingMedia is compiled into the
Local Media Subsystem and relevant paging media drivers.
@publishedPartner
@released 9.3
*/
#define BTRACE_PAGING_MEDIA

/**
If defined, code for BTrace category BTrace::EKernelMemory is compiled into the
kernel.
@publishedPartner
@released 9.4
*/
#define BTRACE_KERNEL_MEMORY

/**
If defined, code for BTrace category BTrace::ERamAllocator is compiled into the
kernel.

This BTrace category is only supported on the multiple and moving memory models.

@publishedPartner
@released 9.4
*/
#if !defined(__MEMMODEL_DIRECT__) && !defined (__MEMMODEL_EMUL_SINGLE_HOST_PROCESS__)
#define BTRACE_RAM_ALLOCATOR
#endif

/**
If defined, code for BTrace category BTrace::EFastMutex is compiled into the
kernel.
*/
#define BTRACE_FAST_MUTEX

/**
If defined, code for BTrace category BTrace::EResourceManager is compiled into the
Resource Manager Subsystem.
@publishedPartner
@released 9.5
*/
#define BTRACE_RESOURCE_MANAGER

/**
If defined, code for BTrace category BTrace::EResourceManagerUs is compiled into the
Resource Manager Subsystem's User-Side API.
@publishedPartner
@released 9.5
*/
#define BTRACE_RESMANUS

/**
If defined, code for BTrace category BTrace::ERawEvent is compiled into the
kernel.
@publishedPartner
@released 9.5
*/
#define BTRACE_TRAWEVENT

/**
If defined, code for BTrace category BTrace::ESymbianKernelSync is compiled into
the kernel.
*/
#define BTRACE_SYMBIAN_KERNEL_SYNC

/**
If defined, code for BTrace category BTrace::EFlexibleMemModel is compiled into
the kernel.

This BTrace category is only supported on the flexible memory model.
*/
#ifdef __MEMMODEL_FLEXIBLE__
#define BTRACE_FLEXIBLE_MEM_MODEL
#endif

/**
If defined, code for BTrace category BTrace::EIic is compiled into the
IIC Subsystem.
*/
#define BTRACE_IIC

/**
If defined, code for BTrace category BTrace::EClientServer is compiled into the
kernel.
*/
#define BTRACE_CLIENT_SERVER

/**
If defined, code for BTrace category BTrace::ERequest is compiled into the
kernel.
*/
#define BTRACE_REQUESTS

#endif // _DEBUG

#endif // __KERNEL_MODE__


#if defined(BTRACE_KERNEL_PROTOTYPE)
// Prototype trace categories...
#undef BTRACE_THREAD_PRIORITY
#define BTRACE_THREAD_PRIORITY
#endif

#if defined(BTRACE_KERNEL_VERBOSE)
// Verbose trace options

#ifdef BTRACE_PAGING
/**
If defined, verbose code for BTrace category BTrace::EPaging is compiled into the
kernel.
@publishedPartner
@released 9.3
*/
#define BTRACE_PAGING_VERBOSE
#endif

#endif //BTRACE_KERNEL_VERBOSE

#if defined(_DEBUG) && !defined(__SMP__)
/**
@internalComponent
*/
TInt KCrazySchedulerEnabled();
#endif

#endif // NK_TRACE_H
