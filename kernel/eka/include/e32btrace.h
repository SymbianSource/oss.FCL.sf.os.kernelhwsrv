// Copyright (c) 2005-2009 Nokia Corporation and/or its subsidiary(-ies).
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
// e32\include\e32btrace.h
// 
// WARNING: This file contains some APIs which are internal and are subject
//          to change without notice. Such APIs should therefore not be used
//          outside the Kernel and Hardware Services package.
//

#ifndef E32BTRACE_H
#define E32BTRACE_H

#ifdef __KERNEL_MODE__
class TSpinLock;
#endif

/**
Class for handling fast tracing.

A trace record consists of three parts: a header, header extensions,
and the trace data itself.

The header consists of four bytes containing:

-#	Size of the record in bytes. (Maximum value is KMaxBTraceRecordSize.)
-#	Flags. See enum TFlags.
-#	Category. Category value from enum BTrace::TCategory.
-#	Sub-category. The meaning of this is dependent on the value of Category.

When trace records are stored in memory they are stored word (32 bit) aligned.
Therefore the size must be rounded up to a multiple of four when calculating
the address of the next record. E.g.
@code
	TUint8* record; // pointer to trace record
	TInt size = record[BTrace::ESizeIndex];
	record += (size+3)&~3; // move record pointer on to next record.
@endcode
The NextRecord() method is provided to do this operation.

Following the header are optionally a number of 32 bit 'header extension' values.
These are present in the order shown below but only exist if the appropriate flag bit
is set in the Header.

-#	Header2.		Contains flag values from enum Flags2.
					This value is only present if the EHeader2Present flag is set.
-#	Timestamp.		A timestamp value indicating when the trace was generated.
					The format and resolution of this value are platform-dependent, but
					typically will contain the same values as would be returned by
					User::FastCounter() or NKern::FastCounter().
					This value is only present if the ETimestampPresent flag is set.
-#	Timestamp2. 	Additional timestamp information. E.g. the most significant
					half of a 64bit timestamp value. Note, it is valid for a Timestamp2 value
					to be present even if the previous Timestamp is absent.
					This value is only present if the ETimestamp2Present flag is set.
-#	Context ID. 	This value indicates the context in which the trace was generated.
					The meaning of the id is dependent on the contents of the two
					least significant bits:
					-	00	indicates the value is the address of the NThread object for
							the currently executing thread.
					-	01	indicates Fast Interrupt (FIQ) context.
							Other bits of the value are currently reserved for future use.
					-	10	indicates Interrupt (IRQ) context. Other bits of the value
							are currently reserved for future use.
					-	11	indicates Immediate Delayed Function Call (IDFC) context.
							Other bits of the value are currently reserved for future use.
					.
					This value is only present if the EContextIdPresent flag is set.
-#	Program Counter. This is the memory address of the instruction after the location
					the trace was output.
					This value is only present if the EPcPresent flag is set.
-#	Extra.			An extra value used for different purposes depending on the trace type.
					This value is only present if the EExtraPresent flag is set.

Following the header extensions are 0 or more bytes of trace data specified when the trace
was output.

To output a trace, the following macros can be used:
- BTrace0
- BTrace4
- BTrace8
- BTrace12
- BTraceN
- BTraceBig
- BTracePc0
- BTracePc4
- BTracePc8
- BTracePc12
- BTracePcN
- BTracePcBig
- BTraceContext0
- BTraceContext4
- BTraceContext8
- BTraceContext12
- BTraceContextN
- BTraceContextBig
- BTraceContextPc0
- BTraceContextPc4
- BTraceContextPc8
- BTraceContextPc12
- BTraceContextPcN
- BTraceContextPcBig

Whenever a trace is output, the trace handler is called with the arguments specified.
See typedef THandler and SetHandler().

Each tracing category has a filter bit, which if set to zero means that traces in that category
are discarded, see SetFilter(). This filtering is performed before the trace handler is
called. This filter may also be initialised from boot time by using the 'btrace' keyword in
an OBY file used to build a ROM image.

Traces may also be additionally sent through a second level of filtering. This examines the
first 32 bits of data in the trace and if this value isn't present in the list maintained
in the secondary filter, the trace is discarded. The contents of the secondary filter are
set using the SetFilter2 methods.

Values used for secondary filtering must be Symbian Unique Identifiers (UIDs) allocated
using the normal UID allocation process. Note, the following non-valid UID value ranges
are reserved.
- 0x00000000..0x007fffff	Reserved for platform specific use.
- 0x00800000..0x00ffffff	Reserved for use by Symbian.

To generate traces which are to be processed by the secondary filter, the following
macros can be used:

- BTraceFiltered4
- BTraceFiltered8
- BTraceFiltered12
- BTraceFilteredN
- BTraceFilteredBig
- BTraceFilteredPc4
- BTraceFilteredPc8
- BTraceFilteredPc12
- BTraceFilteredPcN
- BTraceFilteredPcBig
- BTraceFilteredContext4
- BTraceFilteredContext8
- BTraceFilteredContext12
- BTraceFilteredContextN
- BTraceFilteredContextBig
- BTraceFilteredContextPc4
- BTraceFilteredContextPc8
- BTraceFilteredContextPc12
- BTraceFilteredContextPcN
- BTraceFilteredContextPcBig

Traces generated using the above methods will be filtered twice; once using the primary
filter which checks the trace's category, and once using the secondary filter which checks
the 32 bit UID value at the start of the trace data. Therefore the trace must pass both filter
checks for it to be sent to the trace handler for output.

@publishedPartner
@released
*/
class BTrace
	{
public:
	/**
	Byte indices into the trace header for specific fields.
	*/
	enum THeaderStructure
		{
		/**
		Size of record in bytes.
		*/
		ESizeIndex = 0,

		/**
		Bitfield of flags from enum TFlags. E.g. to detect if a timestamp is present in
		the record, code like this could be used.
		@code
			TUint8* record; // pointer to trace record
			if(record[BTrace::EFlagsIndex]&BTrace::ETimestampPresent)
				TimestampPresent();
			else
				TimestampNotPresent();
		@endcode
		*/
		EFlagsIndex = 1,

		/**
		Category value from enum BTrace::TCategory.
		*/
		ECategoryIndex = 2,

		/**
		Sub-category value. The meaning of this is dependent on the Category.
		*/
		ESubCategoryIndex = 3,
		};

	/**
	Bit flags which indicate state of a trace record.
	*/
	enum TFlags
		{
		/**
		Header2 is present in the trace record.
		*/
		EHeader2Present 	= 1<<0,

		/**
		A timestamp value is present in the trace record.
		*/
		ETimestampPresent	= 1<<1,

		/**
		A second timestamp value is present in the trace record.
		*/
		ETimestamp2Present	= 1<<2,

		/**
		A context ID is present in the trace record.
		*/
		EContextIdPresent	= 1<<3,

		/**
		A CPU program counter (PC) value is present in the trace record.
		*/
		EPcPresent			= 1<<4,

		/**
		An 'extra' value is present in the trace record.
		*/
		EExtraPresent		= 1<<5,

		/**
		Indicates that the data in this trace record was truncated to keep the size
		within the maximum permissible.
		*/
		ERecordTruncated	= 1<<6,

		/**
		Indicates that trace record(s) before this one are missing.
		This can happen if the trace buffer was full when a trace output was attempted.
		*/
		EMissingRecord		= 1<<7
		};

	/**
	Bit flags present in the Flags2 value of the header extension.
	*/
	enum TFlags2
		{
		/**
		Masks out the bits for the multipart trace type. (See enum TMultiPart.)
		*/
		EMultipartFlagMask		= 3<<0,

		/**
		Masks out the bits for the CPU ID for SMP systems (zero if present on non SMP systems)
		*/
		ECpuIdMask			= 0xfffU<<20,
		};

	/**
	Values for multipart trace indicator. These values are stored in Flags2 an
	are obtained by ANDing with the value EMultipartFlagMask.

	If a 'Big' trace is generated which doesn't fit into a single trace record
	then its data is split into several separate trace records; a multipart trace.

	In multipart traces the 'extra' trace value is present in the header extension.
	(EExtraPresent is set.) This extra value contains a unique trace identifier
	which is the same is all parts of the trace.

	The structure of the data part of each trace record in a multipart trace is described
	below. In this description, the following labels are used.
	-	A is the initial 4 bytes of data; the a1 argument of BTraceBig.
	-	D is the array of bytes of additional data; the aData argument of BTraceBig.
	-	N is the size of D; the aDataSize argument of BTraceBig
	-	X is the maximum number of additional bytes which will fit into a trace record.
		This is usually KMaxBTraceDataArray but this should not be assumed, instead
		the size and other information present in each trace record should be examined.

	For the first part of a multipart trace, the data in a trace record has the following
	structure:

	-	4 bytes containing N.
	-	4 bytes containing A.
	-	X bytes containing D[0..X-1]

	If the parts are numbered 0 through to 'j', then a middle part of a multipart trace
	is numbered 'i' where 0<i<j. The data in these parts has the structure:

	-	4 bytes containing N.
	-	4 bytes containing X*i. I.e. the offset within D for the data in this trace record.
	-	X bytes containing D[X*i..X*i+X-1]

	For the last part of a multipart trace, the data has the structure:

	-	4 bytes containing N.
	-	4 bytes containing X*j. I.e. the offset within D for the data in this trace record.
	-	N modulo X bytes containing D[X*j..N-1]. I.e. the final bytes of the trace data.
	*/
	enum TMultiPart
		{
		/**
		Indicates that this trace is the first part of a multipart trace.
		*/
		EMultipartFirst 		= 1,

		/**
		Indicates that this trace is a middle part of a multipart trace.
		I.e. it is not the first or last part.
		*/
		EMultipartMiddle		= 2,

		/**
		Indicates that this trace is the last part of a multipart trace.
		*/
		EMultipartLast			= 3,
		};

	/**
	Enumeration of trace categories.
	*/
	enum TCategory
		{
		/**
		Trace generated by all calls to RDebug::Printf.

		The first 4 bytes of trace data contain the thread ID, RThread::Id(), for the
		thread which caused this trace to be emitted. If the trace wasn't generated in
		thread context, this id has the value KNullThreadId.

		Subsequent bytes of data contain the ASCII text for the formatted string
		generated by Kern::Printf.

		These traces also contain a context ID, i.e. the EContextIdPresent flag is
		set and a context ID value is present in the extended header.

		If the trace text doesn't fit completely into one trace record, then
		a multipart trace is generated. See enum TMultiPart.
		*/
		ERDebugPrintf = 0,

		/**
		Trace generated by all calls to Kern::Printf.
		Trace records in this category have the same structure as ERDebugPrintf.
		*/
		EKernPrintf = 1,

		/**
		Trace generated by platform security diagnostic messages.
		Trace records in this category have the same structure as ERDebugPrintf.
		*/
		EPlatsecPrintf = 2,

		/**
		Trace generated for the purpose of associating thread context ids with
		the textual names of threads. These traces are usually generated when a
		thread is created, renamed or destroyed.

		If #Prime is called with this category, traces will be generated for all
		threads currently extant.

		@see enum TThreadIdentification
		*/
		EThreadIdentification = 3,

		/**
		Trace generated when the CPU usage changes state, e.g. at thread context switch
		or during interrupt and DFC processing.

		The purpose of this trace category is to profile CPU usage.

		@see enum TCpuUsage
		*/
		ECpuUsage = 4,

		/**
		Category used for profiling device drivers, kernel extensions etc.
		Used by PERF_LOG macro.
		@prototype 9.3
		*/
		EKernPerfLog = 5,

		/**
		Trace generated when client-server activity takes place such as server creation,
		session management, message handling, etc.
		If #Prime is called with this category, traces will be generated for all
		servers currently running and their sessions.
		*/
		EClientServer = 6,

		/**
		Trace generated on thread request completion.
		*/
		ERequests = 7,

		/**
		Trace generated when chunks are created and destroyed, and when memory
		is committed and decommitted to and from chunks.

		If #Prime is called with this category, traces will be generated for all
		chunks currently extant.

		@see TChunks
		*/
		EChunks = 8,

		/**
		Trace generated when code segments are created and destroyed, mapped
		into out of processes, and when memory is committed and decommitted to
		and from them.

		If #Prime is called with this category, traces will be generated for all
		code segments currently extant.

		@see TCodeSegs
		*/
		ECodeSegs = 9,

		/**
		Trace generated by Demand Paging.
		@prototype 9.3
		*/
		EPaging = 10,

		/**
		Trace generated when thread and process priorities are modified, whether
		directly or through priority inheritance, aging or other mechanisms used
		by the kernel.

		The purpose of this category is to enable system-wide study of thread
		priority usage.

		If #Prime is called with this category, traces will be generated for all
		threads currently extant.

		@see enum TThreadPriority
		@internalTechnology
		@prototype 9.3
		*/
		EThreadPriority = 11,

		/**
		Trace generated by processing Paging requests in the Media subsystem and Media drivers.
		@prototype 9.3
		*/
		EPagingMedia = 12,

		/**
		Trace generated by the kernel for memory regions which don't belong to any chunk.
		@see enum TKernelMemory
		@prototype 9.4
		*/
		EKernelMemory = 13,

		/**
		Trace generated by user-mode heap usage.

		Unlike other trace categories, capturing heap trace involves an additional step
		depending on how much trace is required. To enable heap trace for a single process
		from the moment it starts, add the following line to the .exe's project (.mmp) file:

			firstlib eexe_instrumented_heap.lib

		This overrides the build tools default implicit link (for .exe projects) against eexe.lib.

		Alternatively, to enable heap trace for all processes at once you can enable the
		KUSERHEAPTRACE bit (#96) of the kernel trace flags. You can set this flag either at
		ROM-building time (look for the 'kerneltrace' line generally in \epoc32\rom\<platform>\header.iby)
		or at runtime by running the following at the Eshell command prompt:

			trace 0 0 1

		Note that changing this flag at runtime only affects processes created after the flag
		is set or unset. It will not affect running processes.

		@see enum THeap
		@prototype 9.4
		*/
		EHeap = 14,

		/**
		Meta trace. Trace that is only useful to programs which use or display BTrace-based data.
		@see enum TMetaTrace
		@prototype 9.4
		*/
		EMetaTrace = 15,

		/**
		Trace generated by the ram allocator to allow the physical layout of RAM
		to be tracked.
		@internalTechnology
		*/
		ERamAllocator = 16,

		/**
		Trace generated by the Fast Mutex in the Nkern.
		*/
		EFastMutex = 17,


		/**
		Trace generated by any sampling profiler.
		@see enum TProfiling
		*/
		EProfiling = 18,

		/**
		Trace generated by Power Resource Manager.
		@prototype 9.5
		*/
		EResourceManager = 19,


		/**
		Trace generated by Power Resource Manager User-Side API.
		@prototype 9.5
		*/
		EResourceManagerUs = 20,

		/**
		Trace generated by Raw Event subsystem APIs
		@see enum TRawEventTrace
		@prototype 9.5
		*/
		ERawEvent  =21,

		/**
		Trace generated by USB communications (Client, Host and OTG) where use
		of standard logging (conditional Kern::Printf() calls) is sufficiently
		time-consuming that the required device timings mandated by the core
		USB standards cannot be achieved
		@prototype 9.5
		*/
		EUsb = 22,

		/**
		Trace generated by Symbian OS kernel synchronization objects.
		@prototype 9.5
		*/
		ESymbianKernelSync = 23,

		/**
		Trace generated by the flexible memory model.
		*/
		EFlexibleMemModel = 24,

		/**
		Trace generated by IIC bus.
		@prototype 9.6
		*/
		EIic = 25,

		/**
		Trace generated by load balancing or higher-level scheduling
		@prototype 9.6
		*/
		EHSched = 26,

		/**
		Trace generated by the nanokernel
		@prototype 9.6
		*/
		ENKern = 27,

		/**
		First category value in the range reserved for platform specific use;
		the end of this range is #EPlatformSpecificLast.
		Symbian's code will not generate any traces with categories in this range.

		It is strongly recommended that platforms reserve the first half of this range
		(128..143) for definition and use by base-port (kernel-side) code. Any general
		trace framework built on top of BTrace APIs should use the second half of the range.
		This allows fast (primary filtered only) BTrace categories to be used in device drivers
		and other base-port code, without clashing with more general trace frameworks implemented
		for application layer code.
		*/
		EPlatformSpecificFirst = 128,

		/**
		Last category value in the range reserved for platform specific use.
		@see EPlatformSpecificFirst
		*/
		EPlatformSpecificLast = 191,

		/**
		First category value in the range reserved for Symbian tools and future trace framework
		implementations; the end of this range is #ESymbianExtentionsLast.
		*/
		ESymbianExtentionsFirst = 192,

		/**
		Last category value in the range reserved for Symbian tools and future trace framework
		implementations.
		@see ESymbianExtentionsFirst
		*/
		ESymbianExtentionsLast = 253,

		/**
		Used for testing purposes.

		This may be used for ad-hoc testing purposes, e.g. special builds of components
		with tracing enabled for diagnostic purposes.

		This category is also used by the E32 BTrace unit tests.
		@test
		*/
		ETest1 = 254,

		/**
		Used for testing purposes.

		This may be used for ad-hoc testing purposes, e.g. special builds of components
		with tracing enabled for diagnostic purposes.

		This category is also used by the E32 BTrace unit tests.
		@test
		*/
		ETest2 = 255
		};

	/**
	Enumeration of sub-category values for trace category EThreadIdentification.
	@see EThreadIdentification
	*/
	enum TThreadIdentification
		{
		/**
		A nano-kernel thread (NThread) has been created.

		Trace data format:
		- 4 bytes containing the context id (an NThread*) for this thread.
		*/
		ENanoThreadCreate,

		/**
		A nano-kernel thread (NThread) has been destroyed.

		Trace data format:
		- 4 bytes containing the context id (an NThread*) for this thread.
		*/
		ENanoThreadDestroy,

		/**
		A thread (DThread) has been created.

		Trace data format:
		- 4 bytes containing the context id (an NThread*) for this thread.
		- 4 bytes containing trace id (a DProcess*) for the process to which this thread belongs.
		- Remaining data is the ASCII name of the thread.
		*/
		EThreadCreate,

		/**
		A thread (DThread) has been destroyed.

		Trace data format:
		- 4 bytes containing the context id (an NThread*) for this thread.
		- 4 bytes containing trace id for the process to which this thread belongs.
		- 4 bytes containing thread ID, as returned by RThread::Id().
		*/
		EThreadDestroy,

		/**
		A thread (DThread) has been renamed.
		This trace may also be output by the tracing system at initialisation
		in order to identify threads already in existence.

		Trace data format:
		- 4 bytes containing the context id (an NThread*) for this thread.
		- 4 bytes containing trace id (a DProcess*) for the process to which this thread belongs.
		- Remaining data is the ASCII name of the thread.
		*/
		EThreadName,

		/**
		A process has been renamed.
		This trace may also be output together with EThreadCreate or EThreadName traces
		to help identify the name of the process to which the thread belongs.

		Trace data format:
		- 4 bytes containing zero, or if this trace is generated together with EThreadName
		  or EThreadCreate, this contains the context id (an NThread*) for the thread.
		- 4 bytes containing trace id (a DProcess*) for process.
		- Remaining data is the ASCII name of the process.
		*/
		EProcessName,

		/**
		Informational trace giving a threads ID, as returned by RThread::Id().
		Trace data format:
		- 4 bytes containing the context id (an NThread*) for this thread.
		- 4 bytes containing trace id (a DProcess*) for the process to which this thread belongs.
		- 4 bytes containing thread ID, as returned by RThread::Id().
		*/
		EThreadId,

		/**
		A process has been created.

		Trace data format:
		- 4 bytes containing trace id (a DProcess*) for the process.
		*/
		EProcessCreate,

		/**
		A process has been destroyed.

		Trace data format:
		- 4 bytes containing trace id (a DProcess*) for the process.
		*/
		EProcessDestroy

		};

	/**
	Enumeration of sub-category values for trace category ECpuUsage.
	@see ECpuUsage
	*/
	enum TCpuUsage
		{
		/**
		Trace output at start of Interrupt (IRQ) dispatch.

		On platforms which support nested interrupts, traces for these will also
		be nested.
		*/
		EIrqStart,

		/**
		Trace output at end of Interrupt (IRQ) dispatch.

		Note, this trace isn't generated if an Interrupt Service Routine queues
		a DFC or causes a thread to be scheduled. In these cases, the traces for
		these events (EIDFCStart or ENewThreadContext) should be taken to indicate
		that interrupt servicing has ended.
		*/
		EIrqEnd,

		/**
		Trace output at start of Fast Interrupt (FIQ) dispatch.

		On platforms which support nested interrupts, traces for these will also
		be nested.
		*/
		EFiqStart,

		/**
		Trace output at end of Fast Interrupt (FIQ) dispatch.

		Note, this trace isn't generated if an Interrupt Service Routine queues
		a DFC or causes a thread to be scheduled. In these cases, the traces for
		these events (EIDFCStart or ENewThreadContext) should be taken to indicate
		that interrupt servicing has ended.
		*/
		EFiqEnd,

		/**
		Trace output at start of Immediate Delayed Function Call (IDFC) processing.
		This processing also includes moving DFCs to their final queue, so the trace
		does not necessarily indicate that any IDFCs have been executed.
		*/
		EIDFCStart,

		/**
		Trace output at end of Immediate Delayed Function Call (IDFC) processing.
		*/
		EIDFCEnd,

		/**
		Trace output when a thread is scheduled to run.
		The context id (NThread*) in this trace is that of the thread being scheduled.
		*/
		ENewThreadContext
		};

	/**
	@internalTechnology
	@prototype 9.3
	*/
	enum TClientServer
		{
		/**
		Trace generated whenever a server is created and during prime.

		Trace data format:
		- 4 bytes containing the server id (a DServer*).
		- 4 bytes containing the owning thread pointer (a DThread*).
		- Remaining data is the ASCII name of the server.

		*/
		EServerCreate,

		/**
		Trace generated whenever a server is destroyed.

		Trace data format:
		- 4 bytes containing the server id (a DServer*).

		*/
		EServerDestroy,

		/**
		Trace generated whenever a new session is attached to a server and during prime.
		I.e. a new session has been created.

		Trace data format:
		- 4 bytes containing the session id (a DSession*).
		- 4 bytes containing the server id (a DServer*).
		- 4 bytes containing the owner id (a DObject*).

		The context id (NThread*) in this trace is that of the thread creating the session
		(apart from during prime when it is NULL).
		*/
		ESessionAttach,

		/**
		Trace generated whenever a server session is detached from a server.
		I.e. a session has been closed.

		Trace data format:
		- 4 bytes containing the session id (a DSession*).
		- 4 bytes containing the reasons (error code) for the session being closed.

		*/
		ESessionDetach,

		/**
		Trace generated whenever a new message is sent to a server.

		Trace data format:
		- 4 bytes containing the message handle.
		- 4 bytes containing the iFunction value for the message.
		- 4 bytes containing the session id (a DSession*).

		The context id (NThread*) in this trace is that of the thread which sent the message.
		*/
		EMessageSend,

		/**
		Trace generated when a server receives a new message.

		Trace data format:
		- 4 bytes containing the message handle.
		*/
		EMessageReceive,

		/**
		Trace generated whenever a message is completed using RMessagePtr2::Complete.

		Trace data format:
		- 4 bytes containing the message handle.
		- 4 bytes containing the completion reason, or object handle, value.
			(The object handle value is that which is delivered to the sender of the
			message, not that supplied by the server actually completing the request.)

		The context id (NThread*) in this trace is that of the thread which completed the message.
		*/
		EMessageComplete
		};


	/**
	@internalTechnology
	@prototype 9.3
	*/
	enum TRequests
		{
		/**
		Trace generated whenever a request status is completed.

		Trace data format:
		- 4 bytes containing the thread id (NThread*) of the thread being signalled.
		- 4 bytes containing the address of the TRequestStatus object.
		- 4 bytes containing the completion reason.

		The context id (NThread*) in this trace is that of the thread which completed the request.
		*/
		ERequestComplete
		};


	/**
	Enumeration of sub-category values for trace category EChunks.
	@see EChunks
	*/
	enum TChunks
		{
		/**
		Trace output when a chunk is created.

		Trace data format:
		- 4 bytes containing the chunk id (a DChunk*).
		- 4 bytes containing the maximum size of the chunk.
		- The ASCII name of the chunk.
		*/
		EChunkCreated,

		/**
		@internalTechnology

		Trace output when a chunk is created containing extra chunk information.

		Note that the meaning of the data in this trace is different between
		memory models, and may change without warning.

		Trace data format:
		- 4 bytes containing the chunk id (a DChunk*).
		- 4 bytes containing the chunk type.
		- 4 bytes containing the chunk's attributes.
		*/
		EChunkInfo,

		/**
		Trace output when a chunk is destroyed.

		Trace data format:
		- 4 bytes containing the chunk id (a DChunk*)
		*/
		EChunkDestroyed,

		/**
		Trace output when memory is allocated and committed to a chunk.

		Trace data format:
		- 4 bytes containing the chunk id (a DChunk*).
		- 4 bytes containing the offset into the chunk.
		- 4 bytes containing the size of the memory committed.
		*/
		EChunkMemoryAllocated,

		/**
		Trace output when memory is decommitted from a chunk and deallocated.

		Trace data format:
		- 4 bytes containing the chunk id (a DChunk*).
		- 4 bytes containing the offset into the chunk.
		- 4 bytes containing the size of the memory decommitted.
		*/
		EChunkMemoryDeallocated,

		/**
		Trace output when un-owned memory is committed to a chunk.

		Trace data format:
		- 4 bytes containing the chunk id (a DChunk*).
		- 4 bytes containing the offset into the chunk.
		- 4 bytes containing the size of the memory committed.
		*/
		EChunkMemoryAdded,

		/**
		Trace output when un-owned memory is decommitted to a chunk.

		Trace data format:
		- 4 bytes containing the chunk id (a DChunk*).
		- 4 bytes containing the offset into the chunk.
		- 4 bytes containing the size of the memory decommitted.
		*/
		EChunkMemoryRemoved,

		/**
		Trace to indicate the owning process of a chunk - only for local (private) chunks.

		Trace data format:
		- 4 bytes containing the chunk id (a DChunk*).
		- 4 bytes containing the process id of the owner (a DProcess*).
		*/
		EChunkOwner
		};

	/**
	Enumeration of sub-category values for trace category ECodeSegs.
	@see ECodeSegs
	*/
	enum TCodeSegs
		{
		/**
		Trace output when a code segment is created to associate a code segment
		id with a filename.

		Trace data format:
		- 4 bytes containing the code segment id (a DCodeSeg*).
		- The ASCII filename.
		*/
		ECodeSegCreated,

		/**
		Trace output when a code segment is created.

		Trace data format:
		- 4 bytes containing the code segment id (a DCodeSeg*).
		- 4 bytes containing the attributes.
		- 4 bytes containing the code base address (.text).
		- 4 bytes containing the size of the code section (.text).
		- 4 bytes containing the base address of the constant data section (.rodata).
		- 4 bytes containing the size of the constant data section (.rodata).
		- 4 bytes containing the base address of the initialised data section (.data).
		- 4 bytes containing the size of the initialised data section (.data).
		- 4 bytes containing the base address of the uninitialised data section (.bss).
		- 4 bytes containing the size of the uninitialised data section (.bss).
		*/
		ECodeSegInfo,

		/**
		Trace output when a code segment is destroyed.

		Trace data format:
		- 4 bytes containing the code segment id (a DCodeSeg*).
		*/
		ECodeSegDestroyed,

		/**
		Trace output when a code segment is mapped into a process.

		Trace data format:
		- 4 bytes containing the code segment id (a DCodeSeg*).
		- 4 bytes containing the process id (a DProcess*).
		*/
		ECodeSegMapped,

		/**
		Trace output when a code segment is unmapped from a process.

		Trace data format:
		- 4 bytes containing the code segment id (a DCodeSeg*).
		- 4 bytes containing the process id (a DProcess*).
		*/
		ECodeSegUnmapped,

		/**
		Trace output when memory is allocated to a code segment.

		Under the multiple memory model, code segments for RAM-loaded user code
		own the RAM pages the code occupies.  The pages are not owned by any
		chunk, but are mapped into the code chunk of each process that uses the
		code segment.

		Trace data format:
		- 4 bytes containing the code segment id (a DCodeSeg*).
		- 4 bytes containing the size of the memory allocated.
		*/
		ECodeSegMemoryAllocated,

		/**
		Trace output when memory is deallocated from a code segment.

		Under the multiple memory model, code segments for RAM-loaded user code
		own the RAM pages the code occupies.  The pages are not owned by any
		chunk, but are mapped into the code chunk of each process that uses the
		code segment.

		Trace data format:
		- 4 bytes containing the code segment id (a DCodeSeg*).
		- 4 bytes containing the size of the memory deallocated.
		*/
		ECodeSegMemoryDeallocated
		};


	/**
	Enumeration of sub-category values for trace category EPaging.
	@see EPaging
	*/
	enum TPaging
		{
		/**
		This event indicates the beginning of the 'page in' activity.
		The end of this activity is indicated by one of the following events:
		- EPagingPageInUnneeded
		- EPagingPageInROM
		- EPagingPageInCode
		- EPagingPageIn (flexible memory model)

		Trace data format:
		- 4 bytes containing the virtual address which was accessed, causing this paging event.
		- 4 bytes containing the virtual address of the instruction which caused this paging event.
		  (The PC value.)

		On the flexible memory model, the following addition trace data is also present:
		- 1 byte containing the required access permissions, as defined by TMappingPermissions.
		- 3 spare bytes, currently zero

		The context id (NThread*) in this trace is that of the thread caused this paging event.
		*/
		EPagingPageInBegin,

		/**
		Event which terminates the 'page in' activity when the required page was found to have been
		paged in by another thread while the current thread was processing the fault (see
		EPagingPageInBegin).

		Trace data format:
		- 0 bytes. (No extra data.)

		The context id (NThread*) in this trace is that of the thread caused this paging event.
		*/
		EPagingPageInUnneeded,

		/**
		A ROM page has been paged in.
		This event indicates the end of the 'page in' activity. (See EPagingPageInBegin.)

		Trace data format:
		- 4 bytes containing the physical address of the page 'paged in'.
		- 4 bytes containing the virtual address of the page 'paged in'.

		The context id (NThread*) in this trace is that of the thread caused this paging event.

		This trace is not emitted on the flexible memory model - EPagingPageIn is used instead.
		*/
		EPagingPageInROM,

		/**
		A ROM page has been 'paged out'. I.e. removed from the live list to be either
		reused or returned to free pool.

		Trace data format:
		- 4 bytes containing the physical address of the page being 'paged out'.
		- 4 bytes containing the virtual address of the page being 'paged out'.

		The context id (NThread*) in this trace is that of the thread caused this paging event.

		This trace is not emitted on the flexible memory model - EPagingPageOut is used instead.
		*/
		EPagingPageOutROM,

		/**
		A Free page has been 'paged in'. I.e. added to the live list.

		Trace data format:
		- 4 bytes containing the physical address of the page being 'paged in'.

		The context id (NThread*) in this trace is that of the thread caused this paging event.
		*/
		EPagingPageInFree,

		/**
		A Free page has been 'paged out'. I.e. removed from the live list to be either
		reused or returned to free pool.

		Trace data format:
		- 4 bytes containing the physical address of the page being 'paged out'.

		The context id (NThread*) in this trace is that of the thread caused this paging event.

		This trace is not emitted on the flexible memory model - EPagingPageOut is used instead.
		*/
		EPagingPageOutFree,

		/**
		A page has been made 'young' again because it was accessed.

		Trace data format:
		- 4 bytes containing the physical address of the page being rejuvenated, (made young).
		- 4 bytes containing the virtual address which was accessed, causing this paging event.
		- 4 bytes containing the virtual address of the instruction which caused this paging event.
		  (The PC value.)

		The context id (NThread*) in this trace is that of the thread caused this paging event.
		*/
		EPagingRejuvenate,

		/**
		A page fault was found to have already been previously serviced.

		Trace data format:
		- 4 bytes containing the physical address of the page accessed.
		- 4 bytes containing the virtual address which was accessed, causing this paging event.
		- 4 bytes containing the virtual address of the instruction which caused this paging event.
		  (The PC value.)

		The context id (NThread*) in this trace is that of the thread caused this paging event.

		This trace is not emitted on the flexible memory model.
		*/
		EPagingPageNop,

		/**
		A page has been locked.

		Trace data format:
		- 4 bytes containing the physical address of the page being locked.
		- 4 bytes containing the value of the lock count after the paged was locked.

		The context id (NThread*) in this trace is that of the thread caused this paging event.
		*/
		EPagingPageLock,

		/**
		A page has been unlocked.

		Trace data format:
		- 4 bytes containing the physical address of the page being unlocked.
		- 4 bytes containing the value of the lock count before the paged was unlocked.

		The context id (NThread*) in this trace is that of the thread caused this paging event.
		*/
		EPagingPageUnlock,

		/**
		A page containing RAM cache has been 'paged out'. I.e. removed from the live list to be
		either reused or returned to free pool.

		Trace data format:
		- 4 bytes containing the physical address of the page being 'paged out'.
		- 4 bytes containing the virtual address of the page being 'paged out'.

		The context id (NThread*) in this trace is that of the thread caused this paging event.

		This trace is not emitted on the flexible memory model - EPagingPageOut is used instead.
		*/
		EPagingPageOutCache,

		/**
		A page containing RAM-loaded code has been paged in.
		This event indicates the end of the 'page in' activity. (See EPagingPageInBegin.)

		Trace data format:
		- 4 bytes containing the physical address of the page 'paged in'.
		- 4 bytes containing the virtual address of the page 'paged in'.

		The context id (NThread*) in this trace is that of the thread caused this paging event.

		This trace is not emitted on the flexible memory model - EPagingPageIn is used instead.
		*/
		EPagingPageInCode,

		/**
		A page containing RAM-loaded code has been 'paged out'. I.e. removed from the live list to be
		either reused or returned to free pool.

		Trace data format:
		- 4 bytes containing the physical address of the page being 'paged out'.
		- 4 bytes containing the virtual address of the page being 'paged out'.

		The context id (NThread*) in this trace is that of the thread caused this paging event.

		This trace is not emitted on the flexible memory model - EPagingPageOut is used instead.
		*/
		EPagingPageOutCode,

		/**
		A page of RAM-loaded code was found to already be 'paged in' but not mapped in
		the faulting process.

		Trace data format:
		- 4 bytes containing the physical address of the page 'paged in'.
		- 4 bytes containing the virtual address which was accessed, causing this paging event.

		The context id (NThread*) in this trace is that of the thread caused this paging event.

		This trace is only emitted on the multiple memory model.
		*/
		EPagingMapCode,

		/**
		A page has been made 'old' because it was the last young page to be accessed.

		This trace is only produced when the kernel is compiled with the #BTRACE_PAGING_VERBOSE
		macro defined.

		Trace data format:
		- 4 bytes containing the physical address of the page being aged, (made old).

		The context id (NThread*) in this trace is that of the thread caused this paging event.
		*/
		EPagingAged,

		/**
		Trace emitted at the start of decompression of demand paged data.

		This trace is only produced when the kernel is compiled with the #BTRACE_PAGING_VERBOSE
		macro defined.

		Trace data format:
		- 4 bytes containing an integer which indicates the compression type being used:
			  0, no compression;
			  1, bytepair compression.

		The context id (NThread*) in this trace is that of the thread caused this paging event.
		*/
		EPagingDecompressStart,

		/**
		Trace emitted at the end of decompression of demand paged data.

		This trace is only produced when the kernel is compiled with the #BTRACE_PAGING_VERBOSE
		macro defined.

		The context id (NThread*) in this trace is that of the thread caused this paging event.
		*/
		EPagingDecompressEnd,

		/**
		Information about the kernel's memory model.

		Trace data format:
		- 4 bytes containing the memory model as defined by TMemModelAttributes.
		*/
		EPagingMemoryModel,

		/**
		A page has been donated to the paging cache via RChunk::Unlock().

		Trace data format:
		- 4 bytes containing the chunk id (a DChunk*).
		- 4 bytes containing the page index of the page within the chunk.

		This trace is not emitted on the flexible memory model.
		@see EPagingDonatePage
		*/
		EPagingChunkDonatePage,

		/**
		A page has been reclaimed from the paging cache via RChunk::Lock().

		Trace data format:
		- 4 bytes containing the chunk id (a DChunk*).
		- 4 bytes containing the page index of the page within the chunk.

		This trace is not emitted on the flexible memory model.
		@see EPagingReclaimPage.
		*/
		EPagingChunkReclaimPage,

		// Traces specific to the flexible memory model

		/**
		A page has been paged in.
		This event indicates the end of the 'page in' activity. (See EPagingPageInBegin.)

		Trace data format:
		- 4 bytes containing the physical address of the page 'paged in'.
		- 4 bytes containing the memory object id (DMemoryObject*).
		- 4 bytes containing the page index of the page within the memory object.

		The context id (NThread*) in this trace is that of the thread caused this paging event.

		This trace is only emitted on the flexible memory model.
		*/
		EPagingPageIn,

		/**
		A page has been 'paged out'. I.e. removed from the live list to be either
		reused or returned to free pool.

		Trace data format:
		- 4 bytes containing the physical address of the page being 'paged out'.

		The context id (NThread*) in this trace is that of the thread caused this paging event.

		This trace is only emitted on the flexible memory model.
		*/
		EPagingPageOut,

		/**
		Event which terminates the 'page in' activity when the required page was found to
		already be paged in but not mapped in the faulting process (see EPagingPageInBegin).

		Trace data format:
		- 4 bytes containing the physical address of the page 'paged in'.

		The context id (NThread*) in this trace is that of the thread caused this paging event.

		This trace is only emitted on the flexible memory model.
		*/
		EPagingMapPage,

		/**
		A page has been donated to the paging cache via RChunk::Unlock().

		Trace data format:
		- 4 bytes containing the physical address of the page.
		- 4 bytes containing the memory object id (DMemoryObject*).
		- 4 bytes containing the page index of the page within the memory object.

		This trace is only emitted on the flexible memory model.
		@see EPagingChunkDonatePage.
		*/
		EPagingDonatePage,

		/**
		A page has been reclaimed from the paging cache via RChunk::Lock().

		Trace data format:
		- 4 bytes containing the physical address of the page.

		This trace is only emitted on the flexible memory model.
		@see EPagingChunkReclaimPage.
		*/
		EPagingReclaimPage,

		/**
		A page has been moved to the oldest clean list because it was the last old page and
		it was clean.

		This trace is only produced when the kernel is compiled with the #BTRACE_PAGING_VERBOSE
		macro defined.

		Trace data format:
		- 4 bytes containing the physical address of the page being moved to the oldest clean list.

		The context id (NThread*) in this trace is that of the thread caused this paging event.
		*/
		EPagingAgedClean,

		/**
		A page has been moved to the oldest dirty list because it was the last old page and
		it was dirty.

		This trace is only produced when the kernel is compiled with the #BTRACE_PAGING_VERBOSE
		macro defined.

		Trace data format:
		- 4 bytes containing the physical address of the page being moved to the oldest dirty list.

		The context id (NThread*) in this trace is that of the thread caused this paging event.
		*/
		EPagingAgedDirty,

		/**
		A page has been allocated to hold the MMU page tables required to map demand paged memory.

		Trace data format:
		- 4 bytes containing the physical address of the page allocated.

		The context id (NThread*) in this trace is that of the thread caused this paging event.
		
		This trace is only emitted on the flexible memory model.
		*/
		EPagingPageTableAlloc,
		};

    /**
    Enumeration of sub-category values for trace category EResourceManager.
    @see EResourceManager
    @prototype 9.5
    */
    enum TResourceManager
        {
        /**
        Trace output for resource registration.

        Trace data format:
        - 4 bytes containing the Resource Id.
        - 4 bytes containing the Resource address.
        - 4 bytes containing the Resource Minimum Level
        - 4 bytes containing the Resource Maximum Level
        - 4 bytes containing the Resource Default Level
        - 4 bytes containing the length of resource name
        - N bytes containing the Resource name, where 0 < N < 32
        */
        ERegisterResource = 0,

        /**
        Trace output for client registration

        Trace data format:
        - 4 bytes containing clientId
        - 4 bytes containing client address
        - 4 bytes containing the length of client name
        - N bytes containing client name, where 0 < N < 32
        */
        ERegisterClient,

        /**
        Trace output for client deregistration

        Trace data format:
        - 4 bytes containing clientId
        - 4 bytes containing client address
        - 4 bytes containing the length of client name
        - N bytes containing client name, where 0 < N < 32
        */
        EDeRegisterClient,

        /**
        Trace output for resource state change start operation

        Trace data format:
        - 4 bytes containing clientId
        - 4 bytes containing the Resource Id.
        - 4 bytes containing the Resource state
        - 4 bytes containing the length of client name
        - N bytes containing client name, where 0 < N < 32
        - 4 bytes containing the length of resource name
        - N bytes containing the Resource name, where 0 < N < 32
        */
        ESetResourceStateStart,

        /**
        Trace output for resource state change end operation

        Trace data format:
        - 4 bytes containing clientId
        - 4 bytes containing the Resource Id.
        - 4 bytes containing return value.
        - 4 bytes containing the Resource state.
        - 4 bytes containing the length of client name
        - N bytes containing client name, where 0 < N < 32
        - 4 bytes containing the length of resource name        
        - N bytes containing the Resource name, where 0 < N < 32
        */
        ESetResourceStateEnd,

        /**
        Trace output for registration for post notification

        Trace data format:
        - 4 bytes containing clientId
        - 4 bytes containing the Resource Id.
        - 4 bytest containing the callback address
        - 4 bytes containing return value.
        */
        EPostNotificationRegister,

        /**
        Trace output for deregistration for post notification

        Trace data format:
        - 4 bytes containing clientId
        - 4 bytes containing the Resource Id.
        - 4 bytes containing the callback address
        - 4 bytes containing the return value.
        */
        EPostNotificationDeRegister,

        /**
        Trace output for post notification sent.

        Trace data format:
        - 4 bytes containing clientId
        - 4 bytes containing the Resource Id.
        */
        EPostNotificationSent,

        /**
        Trace output for Callback complete

        Trace data format:
        - 4 bytes containing clientId
        - 4 bytes containing the Resource Id.
        */
        ECallbackComplete,

        /**
        Trace output for resource manager memory usage

        Trace data format:
        - 4 bytes containing memory allocated in bytes.
        */
        EMemoryUsage,

        /**
        Trace output for get resource state start operation

        Trace data format:
        - 4 bytes containing clientId
        - 4 bytes containing the Resource Id.
        - 4 bytes containing the length of client name.
        - N bytes containing client name, where 0 < N < 32
        - 4 bytes containing the length of resource name.
        - N bytes containing the Resource name, where 0 < N < 32
        */
        EGetResourceStateStart,

        /**
        Trace output for get resource state end operation

        Trace data format:
        - 4 bytes containing clientId
        - 4 bytes containing the Resource Id.
        - 4 bytes containing the Resource state
        - 4 bytes containing return value.
        - 4 bytes containing the length of client name.
        - N bytes containing client name, where 0 < N < 32
        - 4 bytes containing the length of resource name.
        - N bytes containing the Resource name, where 0 < N < 32
        */
        EGetResourceStateEnd,

        /**
        Trace output for cancellation of long latency operation

        Trace data format:
        - 4 bytes containing clientId
        - 4 bytes containing the Resource Id.
        - 4 bytes containing return value
        - 4 bytes containing the length of client name.
        - N bytes containing client name, where 0 < N < 32
        - 4 bytes containing the length of resource name.
        - N bytes containing the Resource name, where 0 < N < 32
        */
        ECancelLongLatencyOperation,

        /**
        Trace output for booting of resource manager

        Trace data format:
        - 4 bytes containing entry point
        */
        EBooting,

        /**
        Trace output for PSL resource state change operation

        Trace data format:
        - 4 bytes containing clientId
        - 4 bytes containing the Resource Id.
        - 4 bytes containing the Resource current state
        - 4 bytes containing the resource requested state
        - 4 bytes containing the length of resource name.
        - N bytes containing the Resource name, where 0 < N < 32
        */
        EPslChangeResourceStateStart,

        /**
        Trace output for PSL resource state change operation

        Trace data format:
        - 4 bytes containing clientId
        - 4 bytes containing the Resource Id.
        - 4 bytes containing the Resource current state
        - 4 bytes containing the resource requested state
        - 4 bytes containing return value
        - 4 bytes containing the length of resource name.
        - N bytes containing the Resource name, where 0 < N < 32
        */
        EPslChangeResourceStateEnd,

        /**
        Trace output for get resource state start operation in PSL

        Trace data format:
        - 4 bytes containing clientId
        - 4 bytes containing the Resource Id.
        - 4 bytes containing the length of resource name.
        - N bytes containing the Resource name, where 0 < N < 32
        */
        EPslGetResourceStateStart,

        /**
        Trace output for get resource state end operation in PSL

        Trace data format:
        - 4 bytes containing clientId
        - 4 bytes containing the Resource Id.
        - 4 bytes containing the Resource state
        - 4 bytes containing return value.
        - 4 bytes containing the length of resource name.
        - N bytes containing the Resource name, where 0 < N < 32
        */
        EPslGetResourceStateEnd,

        /**
        Trace output for resource creation

        Trace data format:
        - 4 bytes containing minimum value of resource
        - 4 bytes containing maximum value of resource
        - 4 bytes containing the default value of resource
        - 4 bytes containing the properties of the resource
        - 4 bytes containing the length of resource name.
        - N bytes containing the Resource name, where 0 < N < 32
        */
        EPslResourceCreate,

        /**
        Trace output for static resource with dependency registration

        Trace data format:
        - 4 bytes containing the Resource Id
        - 4 bytes containing the Resource address
        - 4 bytes containing the minimum value of resource
        - 4 bytes containing the maximum value of resource
        - 4 bytes containing the default value of resource
        - 4 bytes containing the length of resource name
        - N bytes containing the Resource name, where 0 < N < 32
        */
        ERegisterStaticResourceWithDependency,

        /**
        Trace output for dynamic resource registration

        Trace data format:
        - 4 bytes containing clientId
        - 4 bytes containing the Resource Id
        - 4 bytes containing the resouce address
        - 4 bytes containing the length of client name
        - N bytes containing the client name, where 0 < N < 32
        - 4 bytes containing the length of resource name
        - N bytes containing the resource name, where 0 < N < 32
        */
        ERegisterDynamicResource,

        /**
        Trace output for dynamic resource deregistration

        Trace data format:
        - 4 bytes containing clientId
        - 4 bytes containing the Resource Id
        - 4 bytes containing the resource address
        - 4 bytes containing the resource level.
        - 4 bytes containing the length of client name
        - N bytes containing the client name, where 0 < N < 32
        - 4 bytes containing the length of resource name
        - N bytes containing the resource name, where 0 < N < 32
        */
        EDeRegisterDynamicResource,

        /**
        Trace output for resource dependency registration

        Trace data format:
        - 4 bytes containing clientId
        - 4 bytes containing the Resource Id of first dependent resource
        - 4 bytes containing the Resource Id of second dependent resource
        - 4 bytes containing the address of first dependent resource
        - 4 bytes containing the address of second dependent resource
        - 4 bytes containing the length of client name
        - N bytes containing the client name, where 0 < N < 32
        - 4 bytes containing the length of resource name of first dependent resource
        - N bytes containing the resource name of first dependent resource, where 0 < N < 32
        - 4 bytes containing the length of resource name of second dependent resource
        - N bytes containing the resource name of second dependent resource, where 0 < N < 32
        */
        ERegisterResourceDependency,

        /**
        Trace output for resource dependency deregistration

        Trace data format:
        - 4 bytes containing clientId
        - 4 bytes containing the Resource Id of first dependent resource
        - 4 bytes containing the resource id of second dependent resource
        - 4 bytes containing the address of first dependent resource
        - 4 bytes containing the address of second dependent resource
        - 4 bytes containing the length of client name
        - N bytes containing the client name, where 0 < N < 32
        - 4 bytes containing the length of resource name of first dependent resource
        - N bytes containing the resource name of first dependent resource, where 0 < N < 32
        - 4 bytes containing the length of resource name of second dependent resource
        - N bytes containing the resource name of second dependent resource, where 0 < N < 32
        */
        EDeRegisterResourceDependency
        };
    /**
    Enumeration of sub-category values for trace category EResourceManagerUs.
    @see EResourceManagerUs
    @prototype 9.5
    */
    enum TResourceManagerUs
        {
        /**
        Trace output for the start of opening a channel to the Resource Controller.

        Trace data format:
        - 4 bytes containing the client thread identifier.
        - 4 bytes containing the length of client name.
        - N bytes containing the client name, where 0 < N < 32
        */
        EOpenChannelUsStart = 0,
        /**
        Trace output for the end of opening a channel to the Resource Controller.

        Trace data format:
        - 4 bytes containing the client identifier provided by the Resource Controller
        - 4 bytes containing the length of client name.
        - N bytes containing the client name, where 0 < N < 32
        */
        EOpenChannelUsEnd,
        /**
        Trace output for the start of registering a client with the Resource Controller.

        Trace data format:
        - 4 bytes containing the client identifier provided by the Resource Controller
        - 1 bytes the number of concurrent change resource state operations to be supported
        - 1 bytes the number of concurrent notification requests to be supported        
        - 1 bytes the number of concurrent get resource state operations to be supported
        - 1 bytes unused        
        - 4 bytes containing the length of client name.
        - N bytes containing the client name, where 0 < N < 32
        */
        ERegisterClientUsStart,
        /**
        Trace output for the end of registering a client with the Resource Controller.

        Trace data format:
        - 4 bytes containing the client identifier provided by the Resource Controller.
        - 4 bytes specifying the value returned from the call to Resource Controller's AllocReserve method
        */
        ERegisterClientUsEnd,
        /**
        Trace output for the start of de-registering a client with the Resource Controller.

        Trace data format:
        - 4 bytes containing the client identifier provided by the Resource Controller.
        - 4 bytes containing the length of client name.
        - N bytes containing the client name, where 0 < N < 32
        */
        EDeRegisterClientUsStart,
        /**
        Trace output for the end of registering a client with the Resource Controller.

        Trace data format:
        - 4 bytes containing the client identifier provided by the Resource Controller.
        */
        EDeRegisterClientUsEnd,
        /**
        Trace output for the start of a GetResourceState request to the Resource Controller.

        Trace data format:
        - 4 bytes specifying the resource ID
        - 4 bytes containing the client identifier provided by the Resource Controller.
        - 4 bytes containing the length of client name.
        - N bytes containing the client name, where 0 < N < 32
        */
        EGetResourceStateUsStart,
        /**
        Trace output for the end of a GetResourceState request to the Resource Controller.

        Trace data format:
        - 4 bytes specifying the resource ID
        - 4 bytes specifying the resource level
        - 4 bytes containing the client identifier
        - 4 bytes specifying the success code returned by the Resource Controller.
        */
        EGetResourceStateUsEnd,
        /**
        Trace output for the start of a ChangeResourceState request to the Resource Controller.

        Trace data format:
        - 4 bytes specifying the resource ID
        - 4 bytes specifying the required state
        - 4 bytes containing the client identifier provided by the Resource Controller.
        - 4 bytes containing the length of client name.
        - N bytes containing the client name, where 0 < N < 32
        */
        ESetResourceStateUsStart,
        /**
        Trace output for the end of a ChangeResourceState request to the Resource Controller.

        Trace data format:
        - 4 bytes specifying the resource ID
        - 4 bytes specifying the requested state
        - 4 bytes containing the client identifier
        - 4 bytes specifying the success code returned by the Resource Controller.
        */
        ESetResourceStateUsEnd,
        /**
        Trace output for the start of a cancel GetResourceState request to the Resource Controller.

        Trace data format:
        - 4 bytes specifying the resource ID
        - 4 bytes containing the client identifier provided by the Resource Controller.
        - 4 bytes containing the length of client name.
        - N bytes containing the client name, where 0 < N < 32
        */
        ECancelGetResourceStateUsStart,
        /**
        Trace output for the end of a cancel GetResourceState request to the Resource Controller.

        Trace data format:
        - 4 bytes specifying the resource ID
        - 4 bytes containing the client identifier provided by the Resource Controller.
        - 4 bytes containing the length of client name.
        - N bytes containing the client name, where 0 < N < 32
        */
        ECancelGetResourceStateUsEnd,
        /**
        Trace output for the start of a cancel ChangeResourceState request to the Resource Controller.

        Trace data format:
        - 4 bytes specifying the resource ID
        - 4 bytes containing the client identifier provided by the Resource Controller.
        - 4 bytes containing the length of client name.
        - N bytes containing the client name, where 0 < N < 32
        */
        ECancelSetResourceStateUsStart,
        /**
        Trace output for the end of a cancel ChangeResourceState request to the Resource Controller.

        Trace data format:
        - 4 bytes specifying the resource ID
        - 4 bytes containing the client identifier provided by the Resource Controller.
        - 4 bytes containing the length of client name.
        - N bytes containing the client name, where 0 < N < 32
        */
        ECancelSetResourceStateUsEnd
        };

    /**
    Enumeration of sub-category values for trace category EThreadPriority.
    @see EThreadPriority
    @internalTechnology
    @prototype 9.3
    */
    enum TThreadPriority
        {
        /**
        Trace output when a nanothread priority is changed.

        Trace data format:
        - 4 bytes containing the context id (an NThread*) for the thread whose priority is changing.
        - 4 bytes containing the new absolute priority.
        */
        ENThreadPriority=0,

        /**
        Trace output when a DThread's default priority is set.

        Trace data format:
        - 4 bytes containing the context id (an NThread*) for the thread whose priority is changing.
        - 4 bytes containing the iThreadPriority member - a value from enum ::TThrdPriority.
        - 4 bytes containing the new default absolute priority.
        */
        EDThreadPriority=1,

        /**
        Trace output when a DProcess priority is changed.

        Trace data format:
        - 4 bytes containing trace id (a DProcess*) for process.
        - 4 bytes containing the new process priority, a value from enum ::TProcPriority
        */
        EProcessPriority=2
        };

	/**
	Enumeration of sub-category values for trace category EPagingMedia.
	@see EPagingMedia
	*/
	enum TPagingMedia
		{
		/**
		Event generated when a request to 'page in' data is received by the Local Media Subsystem.

		Trace data format:
		- 4 bytes containing the linear address of the buffer to where the data is to be written.
		- 4 bytes containing the offset from start of the partition to where the data to be paged in resides.
		- 4 bytes containing the number of bytes to be read off the media.
		- 4 bytes containing local drive number for the drive where the data to be paged in resides (-1 if ROM paging).
		- 4 bytes containing the linear address in memory where this request object resides.

		The context id (NThread*) in this trace is that of the thread that took the page fault that caused this event.
		*/
		EPagingMediaLocMedPageInBegin,

		/**
		Event generated by the Local Media Subsystem when a request to page data in or out has completed.

		Trace data format:
		- 4 bytes containing the linear address in memory where this request object resides.
		- 4 bytes containing the completion code to be returned.
		- 4 bytes containing a code qualifying this request as either a ROM or Code 'page in' (see TPagingRequestId) or a data page-in/out.

		The context id (NThread*) in this trace is that of the media driver thread that services this 'page in' request.
		*/
		EPagingMediaLocMedPageInPagedIn,

		/**
		Event generated by the Local Media Subsystem when a request to 'page in' data is deferred.

		Trace data format:
		- 4 bytes containing the linear address in memory where this request object resides.
		- 4 bytes containing a code qualifying this request as either a ROM or Code 'page in' (see TPagingRequestId) or a data page-in/out..

		The context id (NThread*) in this trace is that of the media driver thread that services this 'page in' request.
		*/
		EPagingMediaLocMedPageInDeferred,

		/**
		Event generated by the Local Media Subsystem when a request to 'page in' data that has been deferred is re-posted.

		Trace data format:
		- 4 bytes containing the linear address in memory where this request object resides.
		- 4 bytes containing a code qualifying this request as either a ROM or Code 'page in' (see TPagingRequestId) or a data page-in/out..

		The context id (NThread*) in this trace is that of the media driver thread that services this 'page in' request.
		*/
		EPagingMediaLocMedPageInDeferredReposted,

		/**
		Event generated by the Local Media Subsystem when a request to 'page in' data is re-deferred.

		Trace data format:
		- 4 bytes containing the linear address in memory where this request object resides.
		- 4 bytes containing a code qualifying this request as either a ROM or Code 'page in' (see TPagingRequestId) or a data page-in/out..

		The context id (NThread*) in this trace is that of the media driver thread that services this 'page in' request.
		*/
		EPagingMediaLocMedPageInReDeferred,

		/**
		Event generated by the Local Media Subsystem when a request to 'page in' data is issued when the media is not yet open.

		Trace data format:
		- 4 bytes containing the linear address in memory where this request object resides.
		- 4 bytes containing the state of the media (one of TMediaState).
		- 4 bytes containing a code qualifying this request as either a ROM or Code 'page in' (see TPagingRequestId) or a data page-in/out..

		The context id (NThread*) in this trace is that of the media driver thread that services this 'page in' request.
		*/
		EPagingMediaLocMedPageInQuietlyDeferred,

		/**
		Event generated by the Local Media Subsystem when a fragment of a Write request is created and is ready to be sent to the Media
		Driver thread .

		Trace data format:
		- 4 bytes containing the linear address in memory where this request object resides.
		- 4 bytes containing the ID of this fragment (middle or last).
		- 4 bytes containing the length of data in this request fragment.
		- 4 bytes containing the offset within the original request to the start of data in this fragment.
		- 4 bytes containing the offset from start of the partition to where the data in this fragment is to be written.
		- 4 bytes containing the address of the DThread object representing the thread that issued the original Write request.

		The context id (NThread*) in this trace is that of the File Server drive thread that issued the original Write request.
		*/
		EPagingMediaLocMedFragmentBegin,

		/**
		Event generated by the Local Media Subsystem when a Write fragment is completed .

		Trace data format:
		- 4 bytes containing the linear address in memory where this request object resides.
		- 4 bytes containing the completion code to be returned.

		The context id (NThread*) in this trace is that of the File Server drive thread that issued the original Write request.
		*/
		EPagingMediaLocMedFragmentEnd,

		/**
		Event generated when the Media Driver starts processing a request to 'page in' data in its specific Request(..) function.

		Trace data format:
		- 4 bytes containing a code describing the type of the media (one of TMediaDevice).
		- 4 bytes containing the linear address in memory where this request object resides.

		The context id (NThread*) in this trace is that of the media driver thread that services this 'page in' request.
		*/
		EPagingMediaPagingMedDrvBegin,

		/**
		Event generated by the Media Driver when the data read by a 'page in' request is written to the paging buffer.

		Trace data format:
		- 4 bytes containing a code describing the type of the media (one of TMediaDevice).
		- 4 bytes containing the linear address in memory where this request object resides.
		- 4 bytes containing the linear address of the buffer to where the data is to be written.
		- 4 bytes containing the offset within the buffer to where the data will be written.
		- 4 bytes containing the length of data to be written.

		The context id (NThread*) in this trace is that of the media driver thread that services this 'page in' request.
		*/
		EPagingMediaMedDrvWriteBack,

		/**
		Event generated when a request to 'page in' data is put on hold because the Media Driver is performing some background
		operation (not another request) and cannot service the request.

		Trace data format:
		- 4 bytes containing a code describing the type of the media (one of TMediaDevice).
		- 4 bytes containing the linear address in memory where this request object resides.

		The context id (NThread*) in this trace is that of the media driver thread that services this 'page in' request.
		*/
		EPagingMediaMedDrvOnHold,

		/**
		Event generated by the Media Driver when the data read by a 'page out' request is read from the paging buffer.

		Trace data format:
		- 4 bytes containing a code describing the type of the media (one of TMediaDevice).
		- 4 bytes containing the linear address in memory where this request object resides.
		- 4 bytes containing the linear address of the buffer to where the data is to be written.
		- 4 bytes containing the offset within the buffer to where the data will be written.
		- 4 bytes containing the length of data to be written.

		The context id (NThread*) in this trace is that of the media driver thread that services this 'page in' request.
		*/
		EPagingMediaMedDrvRead,

		/**
		Event generated when a request to 'page out' data is received by the Local Media Subsystem.

		Trace data format:
		- 4 bytes containing the linear address of the buffer from where the data is to be read.
		- 4 bytes containing the offset from start of the partition to where the data to be paged out resides.
		- 4 bytes containing the number of bytes to be written to the media.
		- 4 bytes containing the linear address in memory where this request object resides.

		The context id (NThread*) in this trace is that of the thread that took the page fault that caused this event.
		*/
		EPagingMediaLocMedPageOutBegin,

		/**
		Event generated when a request to mark an area of the swap file as deleted is received by the Local Media Subsystem.

		Trace data format:
		- 4 bytes containing NULL
		- 4 bytes containing the offset from start of the partition to where the data to be paged out resides.
		- 4 bytes containing the number of bytes to be marked as deleted
		- 4 bytes containing the linear address in memory where this request object resides.

		The context id (NThread*) in this trace is that of the thread that took the page fault that caused this event.
		*/
		EPagingMediaLocMedDeleteNotifyBegin,

		};

	/**
	Enumeration of sub-category values for trace category EKernelMemory.
	@see EKernelMemory
	*/
	enum TKernelMemory
		{
		/**
		Event recorded during startup and prime which details the initial amount of free RAM.

		Trace data format:
		- 4 bytes containing the number of bytes of RAM the system started with.
		*/
		EKernelMemoryInitialFree,

		/**
		Event recorded during prime which records the then-current amount of free RAM.

		Trace data format:
		- 4 bytes containing the number of bytes of free RAM.
		*/
		EKernelMemoryCurrentFree,

		/**
		Event recorded when a miscellaneous kernel allocation is made. These include:
		- Memory for MMU page table contents
		- Memory for MMU SPageTableInfos
		- Memory for shadow pages

		Trace data format:
		- 4 bytes containing the size, in bytes, of the allocation.
		*/
		EKernelMemoryMiscAlloc,

		/**
		Event recorded when a miscellaneous kernel allocation (see EKernelMemoryMiscAlloc
		above) is freed.

		Trace data format:
		- 4 bytes containing the size, in bytes, of the freed allocation.
		*/
		EKernelMemoryMiscFree,

		/**
		The amount of memory reserved for the minimum demand paging cache. The *actual* DP cache
		also uses free memory, only this minimum amount is permanently reserved for that purpose.
		This event is recorded during prime and when the amount changes.

		Trace data format:
		- 4 bytes containing the minimum size, in bytes, of the demand paging cache.
		*/
		EKernelMemoryDemandPagingCache,

		/**
		Physically contiguous memory allocated by device drivers via one of:
			Epoc::AllocPhysicalRam()
			Epoc::ZoneAllocPhysicalRam()
			Epoc::ClaimPhysicalRam()
			TRamDefragRequest::ClaimRamZone()

		Trace data format:
		- 4 bytes containing the size of the allocated memory.
		- 4 bytes containing the physical base address of allocated memory.

		NB: The prime function logs a EKernelMemoryDrvPhysAlloc record where the physical
		address is -1 and should be ignored.
		*/
		EKernelMemoryDrvPhysAlloc,

		/**
		Memory freed by device drivers via calls to all versions of
		Epoc::FreePhysicalRam().

		Trace data format:
		- 4 bytes containing the size of the freed memory.
		- 4 bytes containing the physical base address of freed memory.
		*/
		EKernelMemoryDrvPhysFree,
		};

	/**
	Enumeration of sub-category values for trace category EHeap.
	@see EHeap
	*/
	enum THeap
		{
		/**
		Event recorded during process startup which logs the point of heap creation.

		Trace data format:
		- 4 bytes containing the heap ID (The RAllocator*)
		- 2 bytes containing the size of header overhead, per allocation (0xFFFF indicates a variable size)
		- 2 bytes containing the size of footer overhead, per allocation (0xFFFF indicates a variable size)
		*/
		EHeapCreate,

		/**
		Event recorded during process startup which details the chunk being used as a heap.

		Trace data format:
		- 4 bytes containing the heap ID (The RAllocator*)
		- 4 bytes containing the chunk ID (The RHandleBase* of the chunk)
		*/
		EHeapChunkCreate,

		/**
		Event recorded when RHeap::Alloc() is called.

		Trace data format:
		- 4 bytes containing the heap ID (The RAllocator*)
		- 4 bytes containing the address of the allocation
		- 4 bytes containing the size of the allocation
		- 4 bytes containing the requested size of allocation
		*/
		EHeapAlloc,

		/**
		Event recorded when RHeap::ReAlloc() is called.

		Trace data format:
		- 4 bytes containing the heap ID (The RAllocator*)
		- 4 bytes containing the address of the new allocation
		- 4 bytes containing the size of the allocation
		- 4 bytes containing the requested size of allocation
		- 4 bytes containing the address of the old allocation
		*/
		EHeapReAlloc,

		/**
		Event recorded when RHeap::Free() is called.

		Trace data format:
		- 4 bytes containing the heap ID (The RAllocator*)
		- 4 bytes containing the address of the free'd allocation
		*/
		EHeapFree,

		/**
		Event recorded when RHeap::Alloc() fails.

		Trace data format:
		- 4 bytes containing the heap ID (The RAllocator*)
		- 4 bytes containing the requested size of allocation
		*/
		EHeapAllocFail,

		/**
		Event recorded when RHeap::ReAlloc() fails.

		Trace data format:
		- 4 bytes containing the heap ID (The RAllocator*)
		- 4 bytes containing the address of the old allocation
		- 4 bytes containing the requested size of allocation
		 */
		EHeapReAllocFail,

		/**
		Event recorded when heap memory corruption occurs.

		Trace data format:
		- 4 bytes containing the heap ID (The RAllocator*)
		- 4 bytes containing address of the corrupted memory block
		- 4 bytes containing length of the corrupted memory block
		*/
		EHeapCorruption,

		/**
		Trace to provide additional heap debugging information.

		This trace (if present) is generated by Symbian OS memory debug tools, and
		will follow one of the other THeap events (e.g. EHeapAlloc, EHeapFree, etc.).
		It is intended to provide a stack trace for the preceding heap event,
		to indicate how the previous heap event was generated.

		On many systems exact stack frames are not available, and the values
		will be extracted from the stack using heuristics, so there may be some
		spurious values and some missing values.

		Trace data format:
		- 4 bytes containing the heap ID (the RAllocator*)
		- sequence of 4-byte PC values representing the call stack, with the top-most
		  (most recent call) first.
		*/
		EHeapCallStack,
		};

	/**
	Enumeration of sub-category values for trace category EMetaTrace.
	@see EMetaTrace
	*/
	enum TMetaTrace
		{
		/**
		Information about timestamps used for tracing.

		Trace data format:
		- 4 bytes containing the period of the Timestamp value.
		- 4 bytes containing the period of the Timestamp2 value.
		- 4 bytes containing a set of bit flags with the following meaning:
			- Bit 0, if true, indicates that Timestamp and Timestamp2 are two halves
			  of a single 64bit timestamp value; Timestamp2 is the most significant part.
			- All other bits are presently undefined.

		The format of the timestamp period data is a period in seconds given using an exponent and mantissa
		format, where the most significant 8 bits are the signed power-of-two value for the exponent, and
		the least significant 24 bits are the integer value of the mantissa. The binary point is to the right
		of the least significant mantissa bit, and the mantissa may not be in normalised form.

		Example code for decoding the period:
		@code
			TInt32 period; // value from trace record
			int exponent = (signed char)(period>>24);
			int mantissa = period&0xffffff;
			double periodInSeconds = (double)mantissa*pow(2,exponent);
		@endcode
		*/
		EMetaTraceTimestampsInfo,

		/**
		Trace indicating the start of a test case being measured.

		Trace data format:
		- 4 bytes containing measurement specific value.
		- 4 bytes containing a further measurement specific value.
		- Remaining data is ASCII text providing human readable information.
		*/
		EMetaTraceMeasurementStart,

		/**
		Trace indicating the end of a test case being measured.

		Trace data format:
		- 4 bytes containing measurement specific identifying value.
		- 4 bytes containing a further measurement specific identifying value.

		The values contained in this trace must be identical to those in the corresponding
		ETraceInfoMeasurementStart trace.
		*/
		EMetaTraceMeasurementEnd,

		/**
		Trace indicating a change in state of the primary filter.

		Trace data format:
		- 1 byte containing a trace category number.
		- 1 byte containing the new filter state for the category. (0=off, 1=on).
		- 2 byte padding. (Should be output as zeros.)
		*/
		EMetaTraceFilterChange,
		};

	/**
	Enumeration of sub-category values for trace category ERamAllocator.
	@see BTrace::ERamAllocator
	*/
	enum TRamAllocator
		{
		/**
		The number of RAM zones.

		Trace data format:
		- 4 bytes containing the number of RAM zones.
		*/
		ERamAllocZoneCount,

		/**
		Information on the layout of a RAM zone.

		Trace data format:
		- 4 bytes containing the number of pages in the zone
		- 4 bytes containing the physical base address of the zone
		- 4 bytes containing the ID of the zone
		- 1 bytes containing the preference of the zone
		- 1 bytes containing the flags of the zone
		- 2 bytes reserved for future use
		*/
		ERamAllocZoneConfig,

		/**
		This trace is sent for every contiguous block of RAM that was allocated
		during the kernel boot process.

		Trace data format:
		- 4 bytes containing the number of contiguous pages allocated for the block
		- 4 bytes containing the physical base address of the block
		*/
		ERamAllocBootAllocation,


		/**
		This trace marks the end of the boot allocations

		Trace data format:
		- no extra bytes are sent
		*/
		ERamAllocBootAllocationEnd,

		/**
		Event generated when a RAM zone's flags have been modified
		This could occur when a RAM zone is blocked/unblocked from further
		allocations from all/certain page types.

		Trace data format:
		- 4 bytes containing the ID of the zone
		- 4 bytes containing the flags of the zone
		*/
		ERamAllocZoneFlagsModified,

		/**
		Event generated when DRamAllocator::ClaimPhysicalRam has successfully
		claimed the specified RAM pages.

		Trace data format:
		- 4 bytes containing the number of contiguous pages
		- 4 bytes containing the base address of the pages
		*/
		ERamAllocClaimRam,

		/**
		Event generated when DRamAllocator::MarkPageAllocated has successfully
		marked the specified page as allocated.

		Trace data format:
		- 4 bytes containing the TZonePageType type of the page
		- 4 bytes containing the address of the page
		*/
		ERamAllocMarkAllocated,

		/**
		Event generated when DRamAllocator::AllocContiguousRam successfully
		allocates the requested number of pages.

		Trace data format:
		- 4 bytes containing the TZonePageType type of the pages
		- 4 bytes containing the number of contiguous pages
		- 4 bytes containing the base address of the pages
		*/
		ERamAllocContiguousRam,

		/**
		Event generated when DRamAllocator::FreePage has successfully freed
		the specified RAM page.

		Trace data format:
		- 4 bytes containing the TZonePageType type of the page
		- 4 bytes containing the address of the page
		*/
		ERamAllocFreePage,

		/**
		Event generated when DRamAllocator::FreePhysical successfully freed
		the specified RAM page(s).

		Trace data format:
		- 4 bytes containing the number of contiguous pages
		- 4 bytes containing the base address of the pages
		*/
		ERamAllocFreePhysical,

		/**
		Event generated for each contiguous block of pages when
		DRamAllocator::AllocRamPages or DRamAllocator::ZoneAllocRamPages
		are attempting to fulfil a request.

		Trace data format:
		- 4 bytes containing the TZonePageType type of the pages
		- 4 bytes containing the number of contiguous pages
		- 4 bytes containing the base address of the pages
		*/
		ERamAllocRamPages,

		/**
		Event generated for contiguous block of pages when
		DRamAllocator::FreeRamPages is invoked.

		Trace data format:
		- 4 bytes containing the TZonePageType type of the pages
		- 4 bytes containing the number of contiguous pages
		- 4 bytes containing the base address of the pages
		*/
		ERamAllocFreePages,

		/**
		Event generated when DRamAllocator::AllocRamPages has successfully
		allocated all the requested number of RAM pages.  If DRamAllocator::AllocRamPages
		couldn't allocate all the requested pages then this event is not generated.

		Trace data format:
		- no extra bytes sent
		*/
		ERamAllocRamPagesEnd,

		/**
		Event generated when all ERamAllocFreePages events for a particular
		call of DRamAllocator::FreeRamPages have been generated.

		Trace data format:
		- no extra bytes sent
		*/
		ERamAllocFreePagesEnd,

		/**
		Event generated when DRamAllocator::ChangePageType is has successfully
		updated the type of the specified page.

		Trace data format:
		- 4 bytes containing the new TZonePageType type of the page
		- 4 bytes containing the physical address of the page
		*/
		ERamAllocChangePageType,

		/**
		Event generated when DRamAllocator::ZoneAllocContiguousRam has
		successfully allocated the required number of pages.

		Trace data format:
		- 4 bytes containing the TZonePageType type of the pages
		- 4 bytes containing the number of contiguous pages
		- 4 bytes containing the base address of the pages
		*/
		ERamAllocZoneContiguousRam,

		/**
		Event generated when DRamAllocator::ZoneAllocRamPages has successfully
		allocated all the requested RAM pages.	If DRamAllocator::ZoneAllocRamPages
		couldn't allocate all the requested pages then this event is not generated.

		Trace data format:
		- no extra bytes sent
		*/
		ERamAllocZoneRamPagesEnd,

		/**
		Event generated when Epoc::ClaimRamZone has successfully claimed
		the requested zone.

		Trace data format:
		- 4 bytes containing the ID of the zone that has been claimed.
		*/
		ERamAllocClaimZone,
		};

	enum TFastMutex
		{
		/**
		Event generated when a thread acquires a fast mutex, (waits on it).

		Trace data format:
		- 4 bytes containing the fast mutex id, (an NFastMutex*).
		*/
		EFastMutexWait,

		/**
		Event generated when a thread releases a fast mutex, (signals it).

		Trace data format:
		- 4 bytes containing the fast mutex id, (an NFastMutex*).
		*/
		EFastMutexSignal,

		/**
		Event generated when a fast mutex is 'flashed' (signalled then immediately
		waited on again). E.g the operation performed by NKern::FlashSystem.

		Trace data format:
		- 4 bytes containing the fast mutex id, (an NFastMutex*).
		*/
		EFastMutexFlash,

		/**
		Trace to associate a fast mutex with a textual name.

		Trace data format:
		- 4 bytes containing the fast mutex id, (an NFastMutex*).
		- 4 bytes containing unspecified data (should be output as zero).
		- Remaining data is the ASCII name for the fast mutex.
		*/
		EFastMutexName,

		/**
		Event generated when a thread blocks on a fast mutex.

		Trace data format:
		- 4 bytes containing the fast mutex id, (an NFastMutex*).
		*/
		EFastMutexBlock,
		};

	/**
	Enumeration of sub-category values for trace category EProfiling.
	@see BTrace::EProfiling
	*/
	enum TProfiling
		{
		/**
		CPU sample including program counter and thread context.

		Trace data format:
		- 4 bytes containing the program counter.
		- 4 bytes containing thread context (an NThread*).
		*/
		ECpuFullSample = 0,

		/**
		Optimised CPU sample including program counter.
		Doesn't include a thread context id as it hasn't changed since
		the last sample.

		Trace data format:
		- 4 bytes containing the program counter.
		*/
		ECpuOptimisedSample,

		/**
		CPU sample from iDFC including program counter.

		Trace data format:
		- 4 bytes containing the program counter.
		*/
		ECpuIdfcSample,

		/**
		CPU sample from non-Symbian thread.

		Trace data format:
		- no extra bytes are sent
		*/
		ECpuNonSymbianThreadSample

		};

	/**
	Enumeration of sub-category values for trace category ERawEvent.
	@see BTrace::ERawEvent
	*/
	enum TRawEventTrace
		{

		/**
		For all the set Functions in the TRawEvent class.
		Trace Data Format Varies depends which of the Overloaded Set Method from where its set .
		Trace data format:
		if there are only one 4 byte data

		--The Following oder is folloed for data.
		- 4 bytes containing the event type

		if there are 2*4 byte data
		- 4 bytes containing the event type
		- 4 bytes containing the scan code

		if there are  3*4 byte data
		- 4 bytes containing the event type
		--4 bytes containining the X co-ordinate
		--4 bytes containining the Y co-ordinate

		if there are  4*4 byte data
		- 4 bytes containing the event type
		--4 bytes containining the X co-ordinate
		--4 bytes containining the Y co-ordinate
		--4 bytes containining the Z co-ordinate

		if there are  5*4 byte data
		- 4 bytes containing the event type
		--4 bytes containining the X co-ordinate
		--4 bytes containining the Y co-ordinate
		--4 bytes containining the Z co-ordinate
		--4 bytes containining the PointerNumber

		if there are  7*4 byte data
		- 4 bytes containing the event type
		--4 bytes containining the X co-ordinate
		--4 bytes containining the Y co-ordinate
		--4 bytes containining the Z co-ordinate
		--4 bytes containining the Phi polar coordinate.
		--4 bytes containining the Theta polar coordinate.
		--4 bytes containining the rotation angle(alpha)
		*/

		ESetEvent = 1,

		/**
		For  user side SetTip Events
		Trace data format:
		- 4 bytes to state containing Tip Info.
		*/
		ESetTipEvent,

		/**
		For  SetTilt Events
		Trace data format:
		- 4 bytes containing the event type
		--4 bytes containining the Phi polar coordinate.
		--4 bytes containining the Theta polar coordinate.
		*/
		ESetTiltEvent,

		/**
		For  SetRotation Events
		Trace data format:
		- 4 bytes containing the event type
		--4 bytes containining the rotation angle (alpha)
		*/
		ESetRotationtEvent,

		/**
		For  SetPointerNumber Events
		Trace data format:
		- 4 bytes containing the Pointer Number
		*/
		ESetPointerNumberEvent,

		/**
		For  user side addevents (User::AddEvent)
		Trace data format:
		- 4 bytes containing the event type
		*/
		EUserAddEvent,

		/**
		For  kernal side addevents (Kern::AddEvent)
		Trace data format:
		- 4 bytes containing the event type
		*/
		EKernelAddEvent
		};

	enum TSymbianKernelSync
		{
		/**
		A semaphore (DSemaphore) has been created.

		Trace data format:
		- 4 bytes containing trace id (a DSemaphore*) for this semaphore.
		- 4 bytes containing the owning DThread* or DProcess*
		- Remaining data is the ASCII name of the semaphore.
		*/
		ESemaphoreCreate=0x00,

		/**
		A semaphore (DSemaphore) has been destroyed.

		Trace data format:
		- 4 bytes containing trace id (a DSemaphore*) for this semaphore.
		*/
		ESemaphoreDestroy=0x01,

		/**
		A semaphore (DSemaphore) has been acquired.

		Trace data format:
		- 4 bytes containing trace id (a DSemaphore*) for this semaphore.
		*/
		ESemaphoreAcquire=0x02,

		/**
		A semaphore (DSemaphore) has been released.

		Trace data format:
		- 4 bytes containing trace id (a DSemaphore*) for this semaphore.
		*/
		ESemaphoreRelease=0x03,

		/**
		A thread has blocked on a semaphore (DSemaphore)

		Trace data format:
		- 4 bytes containing trace id (a DSemaphore*) for this semaphore.
		*/
		ESemaphoreBlock=0x04,


		/**
		A mutex (DMutex) has been created.

		Trace data format:
		- 4 bytes containing trace id (a DMutex*) for this mutex.
		- 4 bytes containing the owning DThread* or DProcess*
		- Remaining data is the ASCII name of the mutex.
		*/
		EMutexCreate=0x10,

		/**
		A mutex (DMutex) has been destroyed.

		Trace data format:
		- 4 bytes containing trace id (a DMutex*) for this mutex.
		*/
		EMutexDestroy=0x11,

		/**
		A mutex (DMutex) has been acquired.

		Trace data format:
		- 4 bytes containing trace id (a DMutex*) for this mutex.
		*/
		EMutexAcquire=0x12,

		/**
		A mutex (DMutex) has been released.

		Trace data format:
		- 4 bytes containing trace id (a DMutex*) for this mutex.
		*/
		EMutexRelease=0x13,

		/**
		A thread has blocked on a mutex (DMutex)

		Trace data format:
		- 4 bytes containing trace id (a DMutex*) for this mutex.
		*/
		EMutexBlock=0x14,


		/**
		A condition variable (DCondVar) has been created.

		Trace data format:
		- 4 bytes containing trace id (a DCondVar*) for this condition variable.
		- 4 bytes containing the owning DThread* or DProcess*
		- Remaining data is the ASCII name of the condition variable.
		*/
		ECondVarCreate=0x20,

		/**
		A condition variable (DCondVar) has been destroyed.

		Trace data format:
		- 4 bytes containing trace id (a DCondVar*) for this condition variable.
		*/
		ECondVarDestroy=0x21,

		/**
		A thread has blocked on a condition variable (DCondVar)

		Trace data format:
		- 4 bytes containing trace id (a DCondVar*) for this condition variable.
		- 4 bytes containing trace id (DMutex*) for the associated mutex.
		*/
		ECondVarBlock=0x22,

		/**
		A thread has been released from a condition variable (DCondVar) wait

		Trace data format:
		- 4 bytes containing trace id (a DCondVar*) for this condition variable.
		- 4 bytes containing trace id (DMutex*) for the associated mutex.
		*/
		ECondVarWakeUp=0x23,

		/**
		A condition variable (DCondVar) has been signalled

		Trace data format:
		- 4 bytes containing trace id (a DCondVar*) for this condition variable.
		- 4 bytes containing trace id (DMutex*) for the associated mutex.
		*/
		ECondVarSignal=0x24,

		/**
		A condition variable (DCondVar) has been signalled in broadcast mode.

		Trace data format:
		- 4 bytes containing trace id (a DCondVar*) for this condition variable.
		- 4 bytes containing trace id (DMutex*) for the associated mutex.
		*/
		ECondVarBroadcast=0x25,

		};

	enum TFlexibleMemModel
		{
		/**
		A memory object has been created.

		Trace data format:
		- 4 bytes containing the memory object id (DMemoryObject*).
		- 4 bytes containing the size of the memory in pages.
		*/
		EMemoryObjectCreate,

		/**
		A memory object has been destroyed.

		Trace data format:
		- 4 bytes containing the memory object id (DMemoryObject*).
		*/
		EMemoryObjectDestroy,

		/**
		A memory mapping has been created.

		Trace data format:
		- 4 bytes containing the memory mapping id (DMemoryMapping*).
		- 4 bytes containing the memory object id (DMemoryObject*).
		- 4 bytes containing the offset of the mapping into the memory object, in pages
		- 4 bytes containing the size of the mapping in pages
		- 2 bytes containing the ASID
		- 2 spare bytes, currently zero
		- 4 bytes containing the virtual address the mapping
		*/
		EMemoryMappingCreate,

		/**
		A memory mapping has been destroyed.

		Trace data format:
		- 4 bytes containing the memory mapping id (DMemoryMapping*).
		*/
		EMemoryMappingDestroy,

		// The following traces associate memory model objects with the kernel objects that use them

		/**
		A memory object is being used for the contents of a chunk.

		Trace data format:
		- 4 bytes containing the memory object id (a DMemoryObject*).
		- 4 bytes containing the chunk id (a DChunk*)
		*/
		EMemoryObjectIsChunk,

		/**
		A memory object is being used for the contents of a code segment.

		Trace data format:
		- 4 bytes containing the memory object id (a DMemoryObject*).
		- 4 bytes containing the code segment id (a DCodeSeg*)
		*/
		EMemoryObjectIsCodeSeg,

		/**
		A memory object is being used for process static data.

		Trace data format:
		- 4 bytes containing the memory object id (a DMemoryObject*).
		- 4 bytes containing the process id (a DProcess*)
		*/
		EMemoryObjectIsProcessStaticData,

		/**
		A memory object is being used for DLL static data.

		Trace data format:
		- 4 bytes containing the memory object id (a DMemoryObject*).
		- 4 bytes containing the code segment id (a DCodeSeg*)
		- 4 bytes containing the process id (a DProcess*)
		*/
		EMemoryObjectIsDllStaticData,

		/**
		A memory object is being used for a thread's supervisor stack.

		Trace data format:
		- 4 bytes containing the memory object id (a DMemoryObject*).
		- 4 bytes containing the thread id (a DThread*)
		*/
		EMemoryObjectIsSupervisorStack,

		/**
		A memory object is being used for a thread's user stack.

		Trace data format:
		- 4 bytes containing the memory object id (a DMemoryObject*).
		- 4 bytes containing the thread id (a DThread*)
		*/
		EMemoryObjectIsUserStack,

		/**
		Identifies the Address Space ID (ASID) used for a process.

		Trace data format:
		- 4 bytes containing the process id (a DProcess*)
		- 2 bytes containing the ASID
		- 2 spare bytes, currently zero
		*/
		EAddressSpaceId
		};

	/**
	Enumeration of sub-category values for trace category EIic.
	@see EIic
	@prototype 9.6
	*/
	enum TIic
		{
		/**
		Trace output for the invocation by the PSL of registering an array of pointers to channels with the IIC bus controller.

		Trace data format:
		- 4 bytes containing the address of the array
		- 4 bytes containing the number of channels in the array
		*/
		ERegisterChansStartPsl = 0,

		/**
		Trace output for the start of the PIL registering an array of pointers to channels with the IIC bus controller.

		Trace data format:
		- 4 bytes containing the address of the array
		- 4 bytes containing the number of channels in the array
		- 4 bytes containing the number of channels registered with the controller prior to this point
		*/
		ERegisterChansStartPil = 1,

		/**
		Trace output for the end of the PIL registering an array of pointers to channels with the IIC bus controller.

		Trace data format:
		- 4 bytes containing the address of the array
		- 4 bytes containing the number of channels now registered with the controller
		*/
		ERegisterChansEndPil = 2,

		/**
		Trace output for the end of the PSL registering an array of pointers to channels with the IIC bus controller.

		Trace data format:
		- 4 bytes containing the address of the array
		- 4 bytes containing the number of channels in the array
		- 4 bytes containing the error code returned by the IIC bus controller
		*/
		ERegisterChansEndPsl = 3,

		/**
		Trace output for the start of the PSL de-registering a channel with the IIC bus controller.

		Trace data format:
		- 4 bytes containing the address of the channel
		*/
		EDeRegisterChanStartPsl = 4,

		/**
		Trace output for the start of the PIL de-registering a channel with the IIC bus controller.

		Trace data format:
		- 4 bytes containing the address of the channel
		- 4 bytes containing the number of channels registered with the controller prior to this point
		*/
		EDeRegisterChanStartPil = 5,

		/**
		Trace output for the end of the PSL de-registering a channel with the IIC bus controller.

		Trace data format:
		- 4 bytes containing the address of the channel
		- 4 bytes containing the number of channels now registered with the controller
		*/
		EDeRegisterChanEndPil = 6,

		/**
		Trace output for the end of the PSL de-registering a channel with the IIC bus controller.

		Trace data format:
		- 4 bytes containing the address of the channel
		- 4 bytes containing the error code returned by the IIC bus controller
		*/
		EDeRegisterChanEndPsl = 7,

		/**
		Trace output for the start of a synchronous queue transaction request in the PIL.

		Trace data format:
		- 4 bytes containing the bus realisation variability token
		- 4 bytes containing the pointer to the transaction object
		*/
		EMQTransSyncStartPil = 8,

		/**
		Trace output for the end of a synchronous queue transaction request in the PIL.

		Trace data format:
		- 4 bytes containing the bus realisation variability token
		- 4 bytes containing the error code returned by the controller
		*/
		EMQTransSyncEndPil = 9,

		/**
		Trace output for the start of a synchronous queue transaction request in the PIL.

		Trace data format:
		- 4 bytes containing the bus realisation variability token
		- 4 bytes containing the pointer to the transaction object
		- 4 bytes containing the pointer to the callback object
		*/
		EMQTransAsyncStartPil = 10,

		/**
		Trace output for the end of a synchronous queue transaction request in the PIL.

		Trace data format:
		- 4 bytes containing the bus realisation variability token
		- 4 bytes containing the error code returned by the controller
		*/
		EMQTransAsyncEndPil = 11,

		/**
		Trace output for the start of cancelling an asynchronous queue transaction request in the PIL.

		Trace data format:
		- 4 bytes containing the bus realisation variability token
		- 4 bytes containing the pointer to the transaction object
		*/
		EMCancelTransStartPil = 12,

		/**
		Trace output for the end of cancelling an asynchronous queue transaction request in the PIL.

		Trace data format:
		- 4 bytes containing the pointer to the transaction object
		- 4 bytes containing the error code returned by the controller
		*/
		EMCancelTransEndPil = 13,

		/**
		Trace output for the start of processing a transaction request in the PIL.

		Trace data format:
		- 4 bytes containing the address of the channel
		- 4 bytes containing the pointer to the transaction object
		*/
		EMProcessTransStartPil = 14,

		/**
		Trace output for the start of processing a transaction request in the PSL.

		Trace data format:
		- 4 bytes containing the pointer to the transaction object
		*/
		EMProcessTransStartPsl = 15,

		/**
		Trace output for the end of processing a transaction request in the PSL.

		Trace data format:
		- 4 bytes containing the result code
		*/
		EMProcessTransEndPsl = 16,

		/**
		Trace output for the end of processing a transaction request in the PIL.

		Trace data format:
		- 4 bytes containing the address of the channel
		- 4 bytes containing the pointer to the transaction object
		- 4 bytes containing the result code
		- 4 bytes containing the pointer to the client callback object
		*/
		EMProcessTransEndPil = 17,

		/**
		Trace output for the start of synchronously capturing a Slave channel in the PIL.

		Trace data format:
		- 4 bytes containing the bus realisation variability
		- 4 bytes containing a pointer to device specific configuration option applicable to all transactions
		*/
		ESCaptChanSyncStartPil = 18,

		/**
		Trace output for the start of synchronously capturing a Slave channel in the PSL.

		Trace data format:
		- 4 bytes containing the address of the channel
		- 4 bytes containing a pointer to device specific configuration option applicable to all transactions
		*/
		ESCaptChanSyncStartPsl = 19,

		/**
		Trace output for the end of synchronously capturing a Slave channel in the PSL.

		Trace data format:
		- 4 bytes containing the address of the channel
		- 4 bytes containing the result code
		*/
		ESCaptChanSyncEndPsl = 20,

		/**
		Trace output for the end of synchronously capturing a Slave channel in the PIL.

		Trace data format:
		- 4 bytes containing the bus realisation variability
		- 4 bytes containing the platform-specific cookie that uniquely identifies the channel
		- 4 bytes containing the result code
		*/
		ESCaptChanSyncEndPil = 21,

		/**
		Trace output for the start of asynchronously capturing a Slave channel in the PIL.

		Trace data format:
		- 4 bytes containing the bus realisation variability
		- 4 bytes containing a pointer to device specific configuration option applicable to all transactions
		- 4 bytes containing the pointer to the client callback object
		*/
		ESCaptChanASyncStartPil = 22,

		/**
		Trace output for the start of asynchronously capturing a Slave channel in the PSL.

		Trace data format:
		- 4 bytes containing a pointer to the channel object
		- 4 bytes containing a pointer to device specific configuration option applicable to all transactions
		*/
		ESCaptChanASyncStartPsl = 23,

		/**
		Trace output for the end of asynchronously capturing a Slave channel in the PSL.

		Trace data format:
		- 4 bytes containing a pointer to the channel object
		- 4 bytes containing the result code
		*/
		ESCaptChanASyncEndPsl = 24,

		/**
		Trace output for the end of asynchronously capturing a Slave channel in the PIL.

		Trace data format:
		- 4 bytes containing a pointer to the channel object
		- 4 bytes containing the result code
		*/
		ESCaptChanASyncEndPil = 25,

		/**
		Trace output for the start of releasing a Slave channel in the PIL.

		Trace data format:
		- 4 bytes containing the channel identifier cookie
		*/
		ESRelChanStartPil = 26,

		/**
		Trace output for the start of releasing a Slave channel in the PSL.

		Trace data format:
		- 4 bytes containing a pointer to the channel object
		*/
		ESRelChanStartPsl = 27,

		/**
		Trace output for the end of releasing a Slave channel in the PSL.

		Trace data format:
		- 4 bytes containing a pointer to the channel object
		- 4 bytes containing the result code
		*/
		ESRelChanEndPsl = 28,

		/**
		Trace output for the end of releasing a Slave channel in the PIL.

		Trace data format:
		- 4 bytes containing the channel identifier cookie
		- 4 bytes containing the result code
		*/
		ESRelChanEndPil = 29,

		/**
		Trace output for the start of registering an Rx buffer for a Slave channel in the PIL.

		Trace data format:
		- 4 bytes containing a pointer to the receive buffer
		- 4 bytes containing the number of buffer bytes used to store a word.
		- 4 bytes containing the number of words expected to be received.
		- 4 bytes containing offset from the start of the buffer where to store the received data.
		*/
		ESRegRxBufStartPil = 30,

		/**
		Trace output for the start of registering an Rx buffer for a Slave channel in the PSL.

		Trace data format:
		- 4 bytes containing a pointer to the channel object
		- 4 bytes containing a pointer to the receive buffer
		- 4 bytes containing the number of buffer bytes used to store a word.
		- 4 bytes containing the number of words expected to be received.
		- 4 bytes containing offset from the start of the buffer where to store the received data.
		*/
		ESRegRxBufStartPsl = 31,

		/**
		Trace output for the end of registering an Rx buffer for a Slave channel in the PSL.

		Trace data format:
		- 4 bytes containing a pointer to the channel object
		- 4 bytes containing the result code
		*/
		ESRegRxBufEndPsl = 32,

		/**
		Trace output for the end of registering an Rx buffer for a Slave channel in the PIL.

		Trace data format:
		- 4 bytes containing the result code
		*/
		ESRegRxBufEndPil = 33,

		/**
		Trace output for the start of registering an Tx buffer for a Slave channel in the PIL.

		Trace data format:
		- 4 bytes containing a pointer to the transmit buffer
		- 4 bytes containing the number of buffer bytes used to store a word.
		- 4 bytes containing the number of words expected to be transmitted.
		- 4 bytes containing offset from the start of the buffer from where to transmit data.
		*/
		ESRegTxBufStartPil = 34,

		/**
		Trace output for the start of registering an Tx buffer for a Slave channel in the PSL.

		Trace data format:
		- 4 bytes containing a pointer to the channel object
		- 4 bytes containing a pointer to the transmit buffer
		- 4 bytes containing the number of buffer bytes used to store a word.
		- 4 bytes containing the number of words expected to be transmitted.
		- 4 bytes containing offset from the start of the buffer from where to transmit data.
		*/
		ESRegTxBufStartPsl = 35,

		/**
		Trace output for the end of registering an Tx buffer for a Slave channel in the PSL.

		Trace data format:
		- 4 bytes containing a pointer to the channel object
		- 4 bytes containing the result code
		*/
		ESRegTxBufEndPsl = 36,

		/**
		Trace output for the start of setting a notification for a Slave channel in the PIL.

		Trace data format:
		- 4 bytes containing the result code
		*/
		ESRegTxBufEndPil = 37,

		/**
		Trace output for the start of setting a notification for a Slave channel in the PIL.

		Trace data format:
		- 4 bytes containing the channel identifier cookie
		- 4 bytes containing the trigger value
		*/
		ESNotifTrigStartPil = 38,

		/**
		Trace output for the start of setting a notification for a Slave channel in the PSL.
		
		Trace data format:
		- 4 bytes containing the trigger value
		*/
		ESNotifTrigStartPsl = 39,

		/**
		Trace output for the end of setting a notification for a Slave channel in the PSL.
		
		Trace data format:
		- 4 bytes containing the result code
		*/
		ESNotifTrigEndPsl = 40,

		/**
		Trace output for the end of setting a notification for a Slave channel in the PIL.

		Trace data format:
		- 4 bytes containing the channel identifier cookie
		- 4 bytes containing the result code
		*/
		ESNotifTrigEndPil = 41,

		/**
		Trace output for the start of a StaticExtension operaton for a MasterSlave channel in the PIL.

		Trace data format:
		- 4 bytes containing a token containing the bus realisation variability or the channel identifier cookie for this client.
		- 4 bytes containing a function identifier
		- 4 bytes containing an argument to be passed to the function
		- 4 bytes containing an argument to be passed to the function
		*/
		EMSStatExtStartPil = 42,

		/**
		Trace output for the end of a StaticExtension operation for a MasterSlave channel in the PIL.

		Trace data format:
		- 4 bytes containing a token containing the bus realisation variability or the channel identifier cookie for this client.
		- 4 bytes containing a function identifier
		- 4 bytes containing the result code
		*/
		EMSStatExtEndPil = 43,

		/**
		Trace output for the start of a StaticExtension operation for a Master channel in the PIL.

		Trace data format:
		- 4 bytes containing a token containing the bus realisation variability or the channel identifier cookie for this client.
		- 4 bytes containing a function identifier
		- 4 bytes containing an argument to be passed to the function
		- 4 bytes containing an argument to be passed to the function
		*/
		EMStatExtStartPil = 44,

		/**
		Trace output for the start of a StaticExtension operation for a Master channel in the PSL.

		Trace data format:
		- 4 bytes containing a pointer to the channel object
		- 4 bytes containing a function identifier
		- 4 bytes containing an argument to be passed to the function
		- 4 bytes containing an argument to be passed to the function
		*/
		EMStatExtStartPsl = 45,

		/**
		Trace output for the end of a StaticExtension operation for a Master channel in the PSL.

		Trace data format:
		- 4 bytes containing a pointer to the channel object
		- 4 bytes containing a function identifier
		- 4 bytes containing the result code
		*/
		EMStatExtEndPsl = 46,

		/**
		Trace output for the end of a StaticExtension operation for a Master channel in the PIL.

		Trace data format:
		- 4 bytes containing a token containing the bus realisation variability or the channel identifier cookie for this client.
		- 4 bytes containing a function identifier
		- 4 bytes containing the result code
		*/
		EMStatExtEndPil = 47,

		/**
		Trace output for the start of a StaticExtension operation for a Slave channel in the PIL.

		Trace data format:
		- 4 bytes containing a token containing the bus realisation variability or the channel identifier cookie for this client.
		- 4 bytes containing a function identifier
		- 4 bytes containing an argument to be passed to the function
		- 4 bytes containing an argument to be passed to the function
		*/
		ESStatExtStartPil = 48,

		/**
		Trace output for the start of a StaticExtension operation for a Slave channel in the PSL.

		Trace data format:
		- 4 bytes containing a pointer to the channel object
		- 4 bytes containing a function identifier
		- 4 bytes containing an argument to be passed to the function
		- 4 bytes containing an argument to be passed to the function
		*/
		ESStatExtStartPsl = 49,

		/**
		Trace output for the end of a StaticExtension operation for a Slave channel in the PSL.

		Trace data format:
		- 4 bytes containing a pointer to the channel object
		- 4 bytes containing a function identifier
		- 4 bytes containing the result code
		*/
		ESStatExtEndPsl = 50,
		/**
		Trace output for the end of a StaticExtension operation for a Slave channel in the PIL.

		Trace data format:
		- 4 bytes containing a token containing the bus realisation variability or the channel identifier cookie for this client.
		- 4 bytes containing a function identifier
		- 4 bytes containing the result code
		*/
		ESStatExtEndPil = 51
		};

	/**
	Enumeration of sub-category values for trace category EHSched.
	@see EHSched
	@prototype 9.6
	*/
	enum THSched
		{
		/**
		Trace output when a thread has been processed by the load balancer
		Trace data format:
		- 4 bytes containing pointer to thread
		- 4 bytes containing flags indicating result of balancing
		*/
		ELbDone = 0,
		};

	/**
	Calculate the address of the next trace record.
	@param aCurrentRecord A pointer to a trace record.
	@return The address of the trace record which follows aCurrentRecord.
	*/
	inline static TUint8* NextRecord(TAny* aCurrentRecord);

#ifdef __KERNEL_MODE__

	/**
	Create initial trace output required for the specified trace category.
	E.g. For the EThreadIdentification category, this will cause traces which identify
	all threads already in existence at this point.

	@aCategory The trace category, or -1 to indicate all trace categories.

	@publishedPartner
	@released
	*/
	IMPORT_C static void Prime(TInt aCategory=-1);

	/**
	Prototype for function which is called to output trace records.
	I.e. as set by SetHandler().

	The handler function should output all values which are appropriate for the
	trace as specified in aHeader.

	@param aHeader	The 4 bytes for the trace header.
	@param aHeader2 The second header word.
					(If EHeader2Present is set in the header flags.)
	@param aContext The context id.
					(If EContextIdPresent is set in the header flags.)
	@param a1		The first four bytes of trace data.
					(Only if the header size indicates that this is present.)
	@param a2		The second four bytes of trace data.
					(Only if the header size indicates that this is present.)
	@param a3		This is either the third four bytes of trace data, if
					the header size indicates there is between 9 and 12 bytes
					of trace data. If more than 12 of trace data are indicated, then
					a3 contains a pointer to the remaining trace data, bytes 8 trough to N.
					This trace data is word aligned.
	@param aExtra	The 'extra' value.
					(If EExtraPresent is set in the header flags.)
	@param aPc		The Program Counter value.
					(If EPcPresent is set in the header flags.)

	@return 		True, if the trace handler is enabled and outputting trace.
					False otherwise.

	Here is an example implementation of a trace handler:

	@code
	TInt size = (aHeader>>BTrace::ESizeIndex*8)&0xff;
	TInt flags = (aHeader>>BTrace::EFlagsIndex*8)&0xff;
	Output(aHeader), size -= 4;
	if(flags&BTrace::EHeader2Present)
		Output(aHeader2), size -= 4;
	if(flags&BTrace::EContextIdPresent)
		Output(aContext), size -= 4;
	if(flags&BTrace::EPcPresent)
		Output(aPc), size -= 4;
	if(flags&BTrace::EExtraPresent)
		Output(aExtra), size -= 4;
	if(size<=0)
		return ETrue;
	Output(a1), size -= 4;
	if(size<=0)
		return ETrue;
	Output(a2), size -= 4;
	if(size<=0)
		return ETrue;
	if(size<=4)
		Output(a3);
	else
		{
		TUint32* data = (TUint32*)a3;
		TUint32* end = (TUint32*)(a3+size);
		do Output(*data++); while(data<end);
		}
	return ETrue;
	@endcode

	The trace handler may add timestamp values to the trace it outputs, in which case
	it should modify the flags and size in aHeader accordingly, before outputting this value.
	The Timestamp and/or Timestamp2 should be output, in that order, between the Header2 and
	Context ID values.

	IMPORTANT NOTES.
	-	The handler must not make use of any kernel APIs, apart from TDfc::RawAdd(). (This is
		because kernel APIs may contain tracing and this would result in deadlock of the system.)
	-	The trace handler must not access or modify arguments which are not indicated as
		being present by their flag in aHeader or aHeader2.
		In particular, on ARM CPUs, the values a2, a3, aExtra and aPc are stored on the stack
		and the caller of this function may not have created these arguments before calling the
		trace handler.
	-	The handler may be called in any context and in a re-entrant manner. Implementations must
		be designed to cope with this.
	-	The interrupt disable status must not be reduced by the trace handler during its operation.
		E.g. if IRQs are disabled on entry, the handler must not cause them to become enabled
		at any point.

	@pre Call in any context.
	@post Interrupt enable status unchanged.

	@publishedPartner
	@released
	*/
	typedef TBool(*THandler)(TUint32 aHeader,TUint32 aHeader2,const TUint32 aContext,const TUint32 a1,const TUint32 a2,const TUint32 a3,const TUint32 aExtra,const TUint32 aPc);


	/**
	Set the function which will receive all trace records.
	@return The handler function which existed before this function was called.
	@publishedPartner
	@released
	*/
	IMPORT_C static BTrace::THandler SetHandler(BTrace::THandler aHandler);


	/**
	Set the trace filter bit for the specified category.

	@param aCategory A category value from enum BTrace::TCategory.
	@param aValue The new filter value for the category.
				  1 means traces of this category are output, 0 means they are suppressed.
				  Other values must not be used.

	@return The previous value of the filter for the category, 0 or 1.
			Or KErrNotSupported if this category is not supported by this build of the kernel.
	@publishedPartner
	@released
	*/
	IMPORT_C static TInt SetFilter(TUint aCategory, TBool aValue);


	/**
	Modify the secondary trace filter to add or remove the specified UID.

	This method can not be used to disable a UID key if SetFilter2(TInt aGlobalFilter)
	has been used to set the filter to pass all traces. Such attempts result in a return
	code of KErrNotSupported.

	@param aUid   The UID to filter.
	@param aValue The new filter value for the UID.
				  1 means traces with this UID are output, 0 means they are suppressed.
				  Other values must not be used.

	@return The previous value of the filter for the UID, 0 or 1, if operation is successful.
			Otherwise, a negative number representing a system wide error code.
			(E.g. KErrNoMemory.)

	@pre Call in a thread context.

	@publishedPartner
	@released
	*/
	IMPORT_C static TInt SetFilter2(TUint32 aUid, TBool aValue);


	/**
	Set the secondary trace filter to include only the specified UIDs.

	@param aUids	Pointer to array of UIDs.
	@param aNumUids Number of UID values pointer to by \a aUid.

	@return KErrNone on success.
			Otherwise, a negative number representing a system wide error code.
			(E.g. KErrNoMemory.)

	@pre Call in a thread context.

	@publishedPartner
	@released
	*/
	IMPORT_C static TInt SetFilter2(const TUint32* aUids, TInt aNumUids);


	/**
	Set the secondary trace filter to pass or reject every trace.

	@param aGlobalFilter If 0, the secondary filter will reject
						 all traces; if 1, all traces are passed
						 by the filter.
						 Other values have no effect.

	@return The previous value of the global filter, or -1 if the global filter
			was not previously set.

	@pre Call in a thread context.

	@publishedPartner
	@released
	*/
	IMPORT_C static TInt SetFilter2(TInt aGlobalFilter);


	/**
	Get the contents of the secondary trace filter.

	@param[out] aUids	Pointer to array of UIDs contained in the secondary filter.
						Ownership of this array is passed to the caller of this
						function, which is then responsible for deleting it.
						If filter is empty, \a aUid equals zero.
	@param[out] aGlobalFilter	Set to 1 if the secondary filter passes all traces.
								Set to 0 if the secondary filter rejects all traces.
								Set to -1 if the secondary filter operates by UIDs contained in traces.

	@return Number of UIDs in returned array, if operation is successful.
			Otherwise, a negative number representing a system wide error code.


	@pre Call in a thread context.
	@pre Calling thread must be in a critical section.

	@publishedPartner
	@released
	*/
	IMPORT_C static TInt Filter2(TUint32*& aUids, TInt& aGlobalFilter);


	/**
	Get the trace filter bit for the specified category.

	@param aCategory A category value from enum BTrace::TCategory,
	@return The value of the filter for the category, 0 or 1.
			Or KErrNotSupported if this category is not supported by this build of the kernel.

	@publishedPartner
	@released
	*/
	inline static TInt Filter(TUint aCategory);

	/**
	Get a pointer to the spinlock used to serialise BTrace output on SMP systems.

	@internalComponent
	*/
	IMPORT_C static TSpinLock* LockPtr();


	/**
	Enumeration of control functions which can be implemented by the BTrace handler.

	These values are passed to #Control to indicate the requested operation and are
	passed unaltered to the BTrace implementation's control function as specified by
	SetHandlers().

	@see #Control
	@see #TControlFunction
	*/
	enum TControl
		{
		/**
		Called to indicate that the system has crashed. Typical response to this call
		is to disable tracing so that debug monitor activity doesn't generate any additional
		trace.

		As this function is called after the system is crashed its implementation must not
		make use of any APIs which require a running system, it must also not re-enable
		interrupts.

		ControlFunction argument meaning: None, ignore.

		ControlFunction returns nothing, ignore.
		*/
		ECtrlSystemCrashed,

		/**
		Called by crash monitor to request first block of data from any memory resident
		trace buffer. A size of zero should be returned if the buffer is empty.

		This should be a non-destructive operation if possible. I.e. the contents of the
		buffer should be capable of being read multiple times by issuing repeated
		ECtrlCrashReadFirst/ECtrlCrashReadNext sequences.

		As this function is called after the system is crashed its implementation must not
		make use of any APIs which require a running system, it must also not re-enable
		interrupts.

		ControlFunction argument meaning:
		- aArg1 should be treated as a TUint8*& and set to the start address of the trace data.
		- aArg2 should be treated as a TUint& and set to the length of the trace data.

		ControlFunction returns KErrNone if successful, otherwise one of the other system wide
		error codes.
		*/
		ECtrlCrashReadFirst,

		/**
		Called by crash monitor after using ECrashReadFirst, to request subsequent
		blocks of data from the trace buffer. A size of zero should be returned if
		the end of the buffer has been reached.

		As this function is called after the system is crashed its implementation must not
		make use of any APIs which require a running system, it must also not re-enable
		interrupts.

		ControlFunction argument meaning:
		aArg1 should be treated as a TUint8** and set to the start address of the trace data.
		aArg2 should be treated as a TUint* and set to the length of the trace data.

		ControlFunction returns KErrNone if successful, otherwise one of the other system wide
		error codes.
		*/
		ECtrlCrashReadNext
		};

	/**
	Prototype for function callback called by #Control.
	I.e. as set by SetHandlers().
	*/
	typedef TInt(*TControlFunction)(TControl aFunction, TAny* aArg1, TAny* aArg2);

	/**
	Call the BTrace handlers control function to perform the function.

	@param aFunction	A value from TControl specifying the requested operation.
	@param aArg1		First argument for the operation. See enum TControl.
	@param aArg1		Second argument for the operation. See enum TControl.

	@return KErrNone if successful,
			KErrNotSupported if the function isn't supported.
			otherwise one of the other system wide error codes.

	@see TControl.
	*/
	IMPORT_C static TInt Control(TControl aFunction, TAny* aArg1=0, TAny* aArg2=0);

	/**
	Set both the function which will receive all trace records, and the
	control function which will be called by each use of #Control.

	@param aNewHandler The new handler to receive trace.
	@param aNewControl The new handler for control functions.
	@param aOldHandler The trace handler which existed prior to this function being called.
	@param aOldControl The control handler which existed prior to this function being called.
	*/
	IMPORT_C static void SetHandlers(BTrace::THandler aNewHandler, BTrace::TControlFunction aNewControl, BTrace::THandler& aOldHandler, BTrace::TControlFunction& aOldControl);

#endif //  __KERNEL_MODE__

	/**
	Check the trace filters to see if a trace with a given category
	would be output.

	@param aCategory  A category value from enum BTrace::TCategory.
					  Only the 8 least significant bits in this value are used;
					  other bits are ignored.

	@return True if a trace with this specification would be passed by the filters.
			False if a trace with this specification would be dropped by the filters.

	@publishedPartner
	@released
	*/
	IMPORT_C static TBool CheckFilter(TUint32 aCategory);

	/**
	Check the trace filters to see if a trace with a given category
	and filter UID would be output.

	@param aCategory  A category value from enum BTrace::TCategory.
					  Only the 8 least significant bits in this value are used;
					  other bits are ignored.
	@param aUid 	  A UID to filter on.

	@return True if a trace with this specification would be passed by the filters.
			False if a trace with this specification would be dropped by the filters.

	@publishedPartner
	@released
	*/
	IMPORT_C static TBool CheckFilter2(TUint32 aCategory,TUint32 aUid);

	/**
	Common function for BTrace macros.
	@internalComponent
	*/
	IMPORT_C static TBool Out(TUint32 a0,TUint32 a1,TUint32 a2,TUint32 a3);

	/**
	Common function for BTrace macros.
	@internalComponent
	*/
	IMPORT_C static TBool OutX(TUint32 a0,TUint32 a1,TUint32 a2,TUint32 a3);

	/**
	Common function for BTrace macros.
	@internalComponent
	*/
	IMPORT_C static TBool OutN(TUint32 a0, TUint32 a1, TUint32 a2, const TAny* aData, TInt aDataSize);

	/**
	Common function for BTrace macros.
	@internalComponent
	*/
	IMPORT_C static TBool OutNX(TUint32 a0, TUint32 a1, TUint32 a2, const TAny* aData, TInt aDataSize);

	/**
	Common function for BTrace macros.
	@internalComponent
	*/
	IMPORT_C static TBool OutBig(TUint32 a0, TUint32 a1, const TAny* aData, TInt aDataSize);

	/**
	Common function for BTrace macros.
	@internalComponent
	*/
	IMPORT_C static TBool OutFiltered(TUint32 a0,TUint32 a1,TUint32 a2,TUint32 a3);

	/**
	Common function for BTrace macros.
	@internalComponent
	*/
	IMPORT_C static TBool OutFilteredX(TUint32 a0,TUint32 a1,TUint32 a2,TUint32 a3);

	/**
	Common function for BTrace macros.
	@internalComponent
	*/
	IMPORT_C static TBool OutFilteredN(TUint32 a0, TUint32 a1, TUint32 a2, const TAny* aData, TInt aDataSize);

	/**
	Common function for BTrace macros.
	@internalComponent
	*/
	IMPORT_C static TBool OutFilteredNX(TUint32 a0, TUint32 a1, TUint32 a2, const TAny* aData, TInt aDataSize);

	/**
	Common function for BTrace macros.
	@internalComponent
	*/
	IMPORT_C static TBool OutFilteredBig(TUint32 a0, TUint32 a1, const TAny* aData, TInt aDataSize);

	/**
	@internalComponent
	*/
	static void Init0();

	/**
	@internalComponent
	*/
	struct SExecExtension
		{
		TUint32 iA2;
		TUint32 iA3;
		TUint32 iPc;
		};

	/**
	@internalComponent
	*/
	static TBool DoOutBig(TUint32 a0, TUint32 a1, const TAny* aData, TInt aDataSize, TUint32 aContext, TUint32 aPc);

	/**
	@internalComponent
	*/
	static TUint32 BigTraceId;

	/**
	@internalComponent
	*/
	static TBool IsSupported(TUint aCategory);

	/**
	Common function for UTrace calls, that need to set both program counter and format id as well as the normal parameters.

	@param aHeader			The header (a0) of the trace record.
	@param aModuleUid		A uid (a1) to filter on
	@param aPc				A program counter
	@param aFormatId		A format id
	@param aData			The data to output
	@param aDataSize		The size of the data

	@return ETrue if a trace was successfully sent and dealt with by the handler.

	@internalComponent
	@prototype
	*/
	IMPORT_C static TBool OutFilteredPcFormatBig(TUint32 aHeader, TUint32 aModuleUid, TUint32 aPc, TUint16 aFormatId, const TAny* aData, TInt aDataSize);

	};


/**
@internalComponent
*/
class DBTraceFilter2
	{
public:
	TUint iNumUids;
	TInt iAccessCount;
	TUint32 iUids[1];

	TBool Check(TUint32 aUid);

	static DBTraceFilter2* New(TInt aNumUids);
	static DBTraceFilter2* Open(DBTraceFilter2*volatile& aFilter2);
	void Close();

	DBTraceFilter2* iCleanupLink;
	static DBTraceFilter2* iCleanupHead;
	static void Cleanup();
	};




//
// BTraceX macros
//

/**
@internalComponent
*/
#define BTRACE_HEADER(aSize,aCategory,aSubCategory) 		\
	(((aSize)<<BTrace::ESizeIndex*8)							\
	+((aCategory)<<BTrace::ECategoryIndex*8)					\
	+((aSubCategory)<<BTrace::ESubCategoryIndex*8))

/**
@internalComponent
*/
#define BTRACE_HEADER_C(aSize,aCategory,aSubCategory)		\
	((((aSize)+4)<<BTrace::ESizeIndex*8)						\
	+((BTrace::EContextIdPresent)<<BTrace::EFlagsIndex*8)	\
	+((aCategory)<<BTrace::ECategoryIndex*8)					\
	+((aSubCategory)<<BTrace::ESubCategoryIndex*8))

/**
@internalComponent
*/
#define BTRACE_HEADER_P(aSize,aCategory,aSubCategory)		\
	((((aSize)+4)<<BTrace::ESizeIndex*8)						\
	+((BTrace::EPcPresent)<<BTrace::EFlagsIndex*8)		\
	+((aCategory)<<BTrace::ECategoryIndex*8)					\
	+((aSubCategory)<<BTrace::ESubCategoryIndex*8))

/**
@internalComponent
*/
#define BTRACE_HEADER_CP(aSize,aCategory,aSubCategory)								\
	((((aSize)+8)<<BTrace::ESizeIndex*8)												\
	+((BTrace::EContextIdPresent|BTrace::EPcPresent)<<BTrace::EFlagsIndex*8)	\
	+((aCategory)<<BTrace::ECategoryIndex*8)											\
	+((aSubCategory)<<BTrace::ESubCategoryIndex*8))



/**
Output a trace record of the specified category.

The trace record data is 0 bytes in size.

@param aCategory	A value from enum BTrace::TCategory,
@param aSubCategory Sub-category value between 0 and 255.
					The meaning of this is dependent on the Category.

@return True if trace is enabled for aCategory, false otherwise.
@publishedPartner
@released
*/
#define BTrace0(aCategory,aSubCategory) \
	BTrace::Out \
		(BTRACE_HEADER(4,(aCategory),(aSubCategory)),0,0,0)

/**
Output a trace record of the specified category.

The trace record data is 4 bytes in size.

@param aCategory	A value from enum BTrace::TCategory,
@param aSubCategory Sub-category value between 0 and 255.
					The meaning of this is dependent on the Category.
@param a1			The 32bit quantity which forms the data of this trace record.

@return True if trace is enabled for aCategory, false otherwise.
@publishedPartner
@released
*/
#define BTrace4(aCategory,aSubCategory,a1) \
	BTrace::Out \
		(BTRACE_HEADER(8,(aCategory),(aSubCategory)),(TUint32)(a1),0,0)

/**
Output a trace record of the specified category.

The trace record data is 8 bytes in size.

@param aCategory	A value from enum BTrace::TCategory,
@param aSubCategory Sub-category value between 0 and 255.
					The meaning of this is dependent on the Category.
@param a1			The first 32bit quantity which forms the data of this trace record.
@param a2			The second 32bit quantity which forms the data of this trace record.

@return True if trace is enabled for aCategory, false otherwise.
@publishedPartner
@released
*/
#define BTrace8(aCategory,aSubCategory,a1,a2) \
	BTrace::Out \
		(BTRACE_HEADER(12,(aCategory),(aSubCategory)),(TUint32)(a1),(TUint32)(a2),0)

/**
Output a trace record of the specified category.

The trace record data is 12 bytes in size.

@param aCategory	A value from enum BTrace::TCategory,
@param aSubCategory Sub-category value between 0 and 255.
					The meaning of this is dependent on the Category.
@param a1			The first 32bit quantity which forms the data of this trace record.
@param a2			The second 32bit quantity which forms the data of this trace record.
@param a3			The third 32bit quantity which forms the data of this trace record.

@return True if trace is enabled for aCategory, false otherwise.
@publishedPartner
@released
*/
#define BTrace12(aCategory,aSubCategory,a1,a2,a3) \
	BTrace::Out \
		(BTRACE_HEADER(16,(aCategory),(aSubCategory)),(TUint32)(a1),(TUint32)(a2),(TUint32)(a3))

/**
Output a trace record of the specified category.

@param aCategory	A value from enum BTrace::TCategory,
@param aSubCategory Sub-category value between 0 and 255.
					The meaning of this is dependent on the Category.
@param a1			The first 32bit quantity which forms the data of this trace record.
@param a2			The second 32bit quantity which forms the data of this trace record.
@param aData		Address of addition data to add to trace.
					Must be word aligned, i.e. a multiple of 4.
@param aDataSize	Number of bytes of additional data. If this value is greater than
					KMaxBTraceDataArray then data is truncated to this size and the
					flag ERecordTruncated is set in the record.

@return True if trace is enabled for aCategory, false otherwise.
@publishedPartner
@released
*/
#define BTraceN(aCategory,aSubCategory,a1,a2,aData,aDataSize) \
	BTrace::OutN \
		(BTRACE_HEADER(12,(aCategory),(aSubCategory)),(TUint32)(a1),(TUint32)(a2),aData,aDataSize)

/**
Output a trace record of the specified category.

If the specified data is too big to find into a single trace record, then a
multipart trace is generated. See TMultiPart.

@param aCategory	A value from enum BTrace::TCategory,
@param aSubCategory Sub-category value between 0 and 255.
					The meaning of this is dependent on the Category.
@param a1			The first 32bit quantity which forms the data of this trace record.
@param aData		Address of addition data to add to trace.
					Must be word aligned, i.e. a multiple of 4.
@param aDataSize	Number of bytes of additional data.

@return True if trace is enabled for aCategory, false otherwise.
@publishedPartner
@released
*/
#define BTraceBig(aCategory,aSubCategory,a1,aData,aDataSize) \
	BTrace::OutBig \
		(BTRACE_HEADER(8,(aCategory),(aSubCategory)),(TUint32)(a1),aData,(TInt)(aDataSize))



/**
Output a trace record of the specified category.

The trace record data is 0 bytes in size.

@param aCategory	A value from enum BTrace::TCategory,
@param aSubCategory Sub-category value between 0 and 255.
					The meaning of this is dependent on the Category.

@return True if trace is enabled for aCategory, false otherwise.
@publishedPartner
@released
*/
#define BTraceContext0(aCategory,aSubCategory) \
	BTrace::OutX \
		(BTRACE_HEADER_C(4,(aCategory),(aSubCategory)),0,0,0)

/**
Output a trace record of the specified category which also includes a Context ID.

The trace record data is 4 bytes in size.

@param aCategory	A value from enum BTrace::TCategory,
@param aSubCategory Sub-category value between 0 and 255.
					The meaning of this is dependent on the Category.
@param a1			The 32bit quantity which forms the data of this trace record.

@return True if trace is enabled for aCategory, false otherwise.
@publishedPartner
@released
*/
#define BTraceContext4(aCategory,aSubCategory,a1) \
	BTrace::OutX \
		(BTRACE_HEADER_C(8,(aCategory),(aSubCategory)),(TUint32)(a1),0,0)

/**
Output a trace record of the specified category which also includes a Context ID.

The trace record data is 8 bytes in size.

@param aCategory	A value from enum BTrace::TCategory,
@param aSubCategory Sub-category value between 0 and 255.
					The meaning of this is dependent on the Category.
@param a1			The first 32bit quantity which forms the data of this trace record.
@param a2			The second 32bit quantity which forms the data of this trace record.

@return True if trace is enabled for aCategory, false otherwise.
@publishedPartner
@released
*/
#define BTraceContext8(aCategory,aSubCategory,a1,a2) \
	BTrace::OutX \
		(BTRACE_HEADER_C(12,(aCategory),(aSubCategory)),(TUint32)(a1),(TUint32)(a2),0)

/**
Output a trace record of the specified category which also includes a Context ID.

The trace record data is 12 bytes in size.

@param aCategory	A value from enum BTrace::TCategory,
@param aSubCategory Sub-category value between 0 and 255.
					The meaning of this is dependent on the Category.
@param a1			The first 32bit quantity which forms the data of this trace record.
@param a2			The second 32bit quantity which forms the data of this trace record.
@param a3			The third 32bit quantity which forms the data of this trace record.

@return True if trace is enabled for aCategory, false otherwise.
@publishedPartner
@released
*/
#define BTraceContext12(aCategory,aSubCategory,a1,a2,a3) \
	BTrace::OutX \
		(BTRACE_HEADER_C(16,(aCategory),(aSubCategory)),(TUint32)(a1),(TUint32)(a2),(TUint32)(a3))

/**
Output a trace record of the specified category which also includes a Context ID.

@param aCategory	A value from enum BTrace::TCategory,
@param aSubCategory Sub-category value between 0 and 255.
					The meaning of this is dependent on the Category.
@param a1			The first 32bit quantity which forms the data of this trace record.
@param a2			The second 32bit quantity which forms the data of this trace record.
@param aData		Address of addition data to add to trace.
					Must be word aligned, i.e. a multiple of 4.
@param aDataSize	Number of bytes of additional data. If this value is greater than
					KMaxBTraceDataArray then data is truncated to this size and the
					flag ERecordTruncated is set in the record.

@return True if trace is enabled for aCategory, false otherwise.
@publishedPartner
@released
*/
#define BTraceContextN(aCategory,aSubCategory,a1,a2,aData,aDataSize) \
	BTrace::OutNX \
		(BTRACE_HEADER_C(12,(aCategory),(aSubCategory)),(TUint32)(a1),(TUint32)(a2),aData,aDataSize)

/**
Output a trace record of the specified category which also includes a Context ID.

If the specified data is too big to find into a single trace record, then a
multipart trace is generated. See TMultiPart.

@param aCategory	A value from enum BTrace::TCategory,
@param aSubCategory Sub-category value between 0 and 255.
					The meaning of this is dependent on the Category.
@param a1			The first 32bit quantity which forms the data of this trace record.
@param aData		Address of addition data to add to trace.
					Must be word aligned, i.e. a multiple of 4.
@param aDataSize	Number of bytes of additional data.

@return True if trace is enabled for aCategory, false otherwise.
@publishedPartner
@released
*/
#define BTraceContextBig(aCategory,aSubCategory,a1,aData,aDataSize) \
	BTrace::OutBig \
		(BTRACE_HEADER_C(8,(aCategory),(aSubCategory)),(TUint32)(a1),aData,(TInt)(aDataSize))



/**
Output a trace record of the specified category which also includes a Program Counter value.

The trace record data is 0 bytes in size.

@param aCategory	A value from enum BTrace::TCategory,
@param aSubCategory Sub-category value between 0 and 255.
					The meaning of this is dependent on the Category.

@return True if trace is enabled for aCategory, false otherwise.
@publishedPartner
@released
*/
#define BTracePc0(aCategory,aSubCategory) \
	BTrace::Out \
		(BTRACE_HEADER_P(4,(aCategory),(aSubCategory)),0,0,0)

/**
Output a trace record of the specified category which also includes a Program Counter value.

The trace record data is 4 bytes in size.

@param aCategory	A value from enum BTrace::TCategory,
@param aSubCategory Sub-category value between 0 and 255.
					The meaning of this is dependent on the Category.
@param a1			The 32bit quantity which forms the data of this trace record.

@return True if trace is enabled for aCategory, false otherwise.
@publishedPartner
@released
*/
#define BTracePc4(aCategory,aSubCategory,a1)	\
	BTrace::Out \
		(BTRACE_HEADER_P(8,(aCategory),(aSubCategory)),(TUint32)(a1),0,0)

/**
Output a trace record of the specified category which also includes a Program Counter value.

The trace record data is 8 bytes in size.

@param aCategory	A value from enum BTrace::TCategory,
@param aSubCategory Sub-category value between 0 and 255.
					The meaning of this is dependent on the Category.
@param a1			The first 32bit quantity which forms the data of this trace record.
@param a2			The second 32bit quantity which forms the data of this trace record.

@return True if trace is enabled for aCategory, false otherwise.
@publishedPartner
@released
*/
#define BTracePc8(aCategory,aSubCategory,a1,a2) \
	BTrace::Out \
		(BTRACE_HEADER_P(12,(aCategory),(aSubCategory)),(TUint32)(a1),(TUint32)(a2),0)

/**
Output a trace record of the specified category which also includes a Program Counter value.

The trace record data is 12 bytes in size.

@param aCategory	A value from enum BTrace::TCategory,
@param aSubCategory Sub-category value between 0 and 255.
					The meaning of this is dependent on the Category.
@param a1			The first 32bit quantity which forms the data of this trace record.
@param a2			The second 32bit quantity which forms the data of this trace record.
@param a3			The third 32bit quantity which forms the data of this trace record.

@return True if trace is enabled for aCategory, false otherwise.
@publishedPartner
@released
*/
#define BTracePc12(aCategory,aSubCategory,a1,a2,a3) \
	BTrace::Out \
		(BTRACE_HEADER_P(16,(aCategory),(aSubCategory)),(TUint32)(a1),(TUint32)(a2),(TUint32)(a3))

/**
Output a trace record of the specified category which also includes a Program Counter value.

@param aCategory	A value from enum BTrace::TCategory,
@param aSubCategory Sub-category value between 0 and 255.
					The meaning of this is dependent on the Category.
@param a1			The first 32bit quantity which forms the data of this trace record.
@param a2			The second 32bit quantity which forms the data of this trace record.
@param aData		Address of addition data to add to trace.
					Must be word aligned, i.e. a multiple of 4.
@param aDataSize	Number of bytes of additional data. If this value is greater than
					KMaxBTraceDataArray then data is truncated to this size and the
					flag ERecordTruncated is set in the record.

@return True if trace is enabled for aCategory, false otherwise.
@publishedPartner
@released
*/
#define BTracePcN(aCategory,aSubCategory,a1,a2,aData,aDataSize) \
	BTrace::OutN \
		(BTRACE_HEADER_P(12,(aCategory),(aSubCategory)),(TUint32)(a1),(TUint32)(a2),aData,aDataSize)

/**
Output a trace record of the specified category which also includes a Program Counter value.

If the specified data is too big to find into a single trace record, then a
multipart trace is generated. See TMultiPart.

@param aCategory	A value from enum BTrace::TCategory,
@param aSubCategory Sub-category value between 0 and 255.
					The meaning of this is dependent on the Category.
@param a1			The first 32bit quantity which forms the data of this trace record.
@param aData		Address of addition data to add to trace.
					Must be word aligned, i.e. a multiple of 4.
@param aDataSize	Number of bytes of additional data.

@return True if trace is enabled for aCategory, false otherwise.
@publishedPartner
@released
*/
#define BTracePcBig(aCategory,aSubCategory,a1,aData,aDataSize) \
	BTrace::OutBig \
		(BTRACE_HEADER_P(8,(aCategory),(aSubCategory)),(TUint32)(a1),aData,(TInt)(aDataSize))




/**
Output a trace record of the specified category which also includes
Context ID and Program Counter values.

The trace record data is 0 bytes in size.

@param aCategory	A value from enum BTrace::TCategory,
@param aSubCategory Sub-category value between 0 and 255.
					The meaning of this is dependent on the Category.

@return True if trace is enabled for aCategory, false otherwise.
@publishedPartner
@released
*/
#define BTraceContextPc0(aCategory,aSubCategory)	\
	BTrace::OutX \
		(BTRACE_HEADER_CP(4,(aCategory),(aSubCategory)),0,0,0)

/**
Output a trace record of the specified category which also includes
Context ID and Program Counter values.

The trace record data is 4 bytes in size.

@param aCategory	A value from enum BTrace::TCategory,
@param aSubCategory Sub-category value between 0 and 255.
					The meaning of this is dependent on the Category.
@param a1			The 32bit quantity which forms the data of this trace record.

@return True if trace is enabled for aCategory, false otherwise.
@publishedPartner
@released
*/
#define BTraceContextPc4(aCategory,aSubCategory,a1) \
	BTrace::OutX \
		(BTRACE_HEADER_CP(8,(aCategory),(aSubCategory)),(TUint32)(a1),0,0)

/**
Output a trace record of the specified category which also includes
Context ID and Program Counter values.

The trace record data is 8 bytes in size.

@param aCategory	A value from enum BTrace::TCategory,
@param aSubCategory Sub-category value between 0 and 255.
					The meaning of this is dependent on the Category.
@param a1			The first 32bit quantity which forms the data of this trace record.
@param a2			The second 32bit quantity which forms the data of this trace record.

@return True if trace is enabled for aCategory, false otherwise.
@publishedPartner
@released
*/
#define BTraceContextPc8(aCategory,aSubCategory,a1,a2) \
	BTrace::OutX \
		(BTRACE_HEADER_CP(12,(aCategory),(aSubCategory)),(TUint32)(a1),(TUint32)(a2),0)

/**
Output a trace record of the specified category which also includes
Context ID and Program Counter values.

The trace record data is 12 bytes in size.

@param aCategory	A value from enum BTrace::TCategory,
@param aSubCategory Sub-category value between 0 and 255.
					The meaning of this is dependent on the Category.
@param a1			The first 32bit quantity which forms the data of this trace record.
@param a2			The second 32bit quantity which forms the data of this trace record.
@param a3			The third 32bit quantity which forms the data of this trace record.

@return True if trace is enabled for aCategory, false otherwise.
@publishedPartner
@released
*/
#define BTraceContextPc12(aCategory,aSubCategory,a1,a2,a3) \
	BTrace::OutX \
		(BTRACE_HEADER_CP(16,(aCategory),(aSubCategory)),(TUint32)(a1),(TUint32)(a2),(TUint32)(a3))

/**
Output a trace record of the specified category which also includes
Context ID and Program Counter values.

@param aCategory	A value from enum BTrace::TCategory,
@param aSubCategory Sub-category value between 0 and 255.
					The meaning of this is dependent on the Category.
@param a1			The first 32bit quantity which forms the data of this trace record.
@param a2			The second 32bit quantity which forms the data of this trace record.
@param aData		Address of addition data to add to trace.
					Must be word aligned, i.e. a multiple of 4.
@param aDataSize	Number of bytes of additional data. If this value is greater than
					KMaxBTraceDataArray then data is truncated to this size and the
					flag ERecordTruncated is set in the record.

@return True if trace is enabled for aCategory, false otherwise.
@publishedPartner
@released
*/
#define BTraceContextPcN(aCategory,aSubCategory,a1,a2,aData,aDataSize) \
	BTrace::OutNX \
		(BTRACE_HEADER_CP(12,(aCategory),(aSubCategory)),(TUint32)(a1),(TUint32)(a2),aData,aDataSize)

/**
Output a trace record of the specified category which also includes
Context ID and Program Counter values.

If the specified data is too big to find into a single trace record, then a
multipart trace is generated. See TMultiPart.

@param aCategory	A value from enum BTrace::TCategory,
@param aSubCategory Sub-category value between 0 and 255.
					The meaning of this is dependent on the Category.
@param a1			The first 32bit quantity which forms the data of this trace record.
@param aData		Address of addition data to add to trace.
					Must be word aligned, i.e. a multiple of 4.
@param aDataSize	Number of bytes of additional data.

@return True if trace is enabled for aCategory, false otherwise.
@publishedPartner
@released
*/
#define BTraceContextPcBig(aCategory,aSubCategory,a1,aData,aDataSize) \
	BTrace::OutBig \
		(BTRACE_HEADER_CP(8,(aCategory),(aSubCategory)),(TUint32)(a1),aData,(TInt)(aDataSize))



/**
Output a trace record of the specified category.

The trace record data is 4 bytes in size.

If the value of \a aUid is not contained in the secondary filter then the trace is discarded.

@param aCategory	A value from enum BTrace::TCategory,
@param aSubCategory Sub-category value between 0 and 255.
					The meaning of this is dependent on the Category.
@param aUid 		The 32bit quantity which forms the data of this trace record.

@return True if trace is enabled for aCategory and aUid, false otherwise.
@publishedPartner
@released
*/
#define BTraceFiltered4(aCategory,aSubCategory,aUid) \
	BTrace::OutFiltered \
		(BTRACE_HEADER(8,(aCategory),(aSubCategory)),(TUint32)(aUid),0,0)

/**
Output a trace record of the specified category.

The trace record data is 8 bytes in size.

If the value of \a aUid is not contained in the secondary filter then the trace is discarded.

@param aCategory	A value from enum BTrace::TCategory,
@param aSubCategory Sub-category value between 0 and 255.
					The meaning of this is dependent on the Category.
@param aUid 		The first 32bit quantity which forms the data of this trace record.
@param a1			The second 32bit quantity which forms the data of this trace record.

@return True if trace is enabled for aCategory and aUid, false otherwise.
@publishedPartner
@released
*/
#define BTraceFiltered8(aCategory,aSubCategory,aUid,a1) \
	BTrace::OutFiltered \
		(BTRACE_HEADER(12,(aCategory),(aSubCategory)),(TUint32)(aUid),(TUint32)(a1),0)

/**
Output a trace record of the specified category.

The trace record data is 12 bytes in size.

If the value of \a aUid is not contained in the secondary filter then the trace is discarded.

@param aCategory	A value from enum BTrace::TCategory,
@param aSubCategory Sub-category value between 0 and 255.
					The meaning of this is dependent on the Category.
@param aUid 		The first 32bit quantity which forms the data of this trace record.
@param a1			The second 32bit quantity which forms the data of this trace record.
@param a2			The third 32bit quantity which forms the data of this trace record.

@return True if trace is enabled for aCategory and aUid, false otherwise.
@publishedPartner
@released
*/
#define BTraceFiltered12(aCategory,aSubCategory,aUid,a1,a2) \
	BTrace::OutFiltered \
		(BTRACE_HEADER(16,(aCategory),(aSubCategory)),(TUint32)(aUid),(TUint32)(a1),(TUint32)(a2))

/**
Output a trace record of the specified category.

If the value of \a aUid is not contained in the secondary filter then the trace is discarded.

@param aCategory	A value from enum BTrace::TCategory,
@param aSubCategory Sub-category value between 0 and 255.
					The meaning of this is dependent on the Category.
@param aUid 		The first 32bit quantity which forms the data of this trace record.
@param a1			The second 32bit quantity which forms the data of this trace record.
@param aData		Address of addition data to add to trace.
					Must be word aligned, i.e. a multiple of 4.
@param aDataSize	Number of bytes of additional data. If this value is greater than
					KMaxBTraceDataArray then data is truncated to this size and the
					flag ERecordTruncated is set in the record.

@return True if trace is enabled for aCategory and aUid, false otherwise.
@publishedPartner
@released
*/
#define BTraceFilteredN(aCategory,aSubCategory,aUid,a1,aData,aDataSize) \
	BTrace::OutFilteredN \
		(BTRACE_HEADER(12,(aCategory),(aSubCategory)),(TUint32)(aUid),(TUint32)(a1),aData,aDataSize)

/**
Output a trace record of the specified category.

If the specified data is too big to find into a single trace record, then a
multipart trace is generated. See TMultiPart.

If the value of \a aUid is not contained in the secondary filter then the trace is discarded.

@param aCategory	A value from enum BTrace::TCategory,
@param aSubCategory Sub-category value between 0 and 255.
					The meaning of this is dependent on the Category.
@param aUid 		The first 32bit quantity which forms the data of this trace record.
@param aData		Address of addition data to add to trace.
					Must be word aligned, i.e. a multiple of 4.
@param aDataSize	Number of bytes of additional data.

@return True if trace is enabled for aCategory and aUid, false otherwise.
@publishedPartner
@released
*/
#define BTraceFilteredBig(aCategory,aSubCategory,aUid,aData,aDataSize) \
	BTrace::OutFilteredBig \
		(BTRACE_HEADER(8,(aCategory),(aSubCategory)),(TUint32)(aUid),aData,(TInt)(aDataSize))



/**
Output a trace record of the specified category which also includes a Context ID.

The trace record data is 4 bytes in size.

If the value of \a aUid is not contained in the secondary filter then the trace is discarded.

@param aCategory	A value from enum BTrace::TCategory,
@param aSubCategory Sub-category value between 0 and 255.
					The meaning of this is dependent on the Category.
@param aUid 		The 32bit quantity which forms the data of this trace record.

@return True if trace is enabled for aCategory and aUid, false otherwise.
@publishedPartner
@released
*/
#define BTraceFilteredContext4(aCategory,aSubCategory,aUid) \
	BTrace::OutFilteredX \
		(BTRACE_HEADER_C(8,(aCategory),(aSubCategory)),(TUint32)(aUid),0,0)

/**
Output a trace record of the specified category which also includes a Context ID.

The trace record data is 8 bytes in size.

If the value of \a aUid is not contained in the secondary filter then the trace is discarded.

@param aCategory	A value from enum BTrace::TCategory,
@param aSubCategory Sub-category value between 0 and 255.
					The meaning of this is dependent on the Category.
@param aUid 		The first 32bit quantity which forms the data of this trace record.
@param a1			The second 32bit quantity which forms the data of this trace record.

@return True if trace is enabled for aCategory and aUid, false otherwise.
@publishedPartner
@released
*/
#define BTraceFilteredContext8(aCategory,aSubCategory,aUid,a1) \
	BTrace::OutFilteredX \
		(BTRACE_HEADER_C(12,(aCategory),(aSubCategory)),(TUint32)(aUid),(TUint32)(a1),0)

/**
Output a trace record of the specified category which also includes a Context ID.

The trace record data is 12 bytes in size.

If the value of \a aUid is not contained in the secondary filter then the trace is discarded.

@param aCategory	A value from enum BTrace::TCategory,
@param aSubCategory Sub-category value between 0 and 255.
					The meaning of this is dependent on the Category.
@param aUid 		The first 32bit quantity which forms the data of this trace record.
@param a1			The second 32bit quantity which forms the data of this trace record.
@param a2			The third 32bit quantity which forms the data of this trace record.

@return True if trace is enabled for aCategory and aUid, false otherwise.
@publishedPartner
@released
*/
#define BTraceFilteredContext12(aCategory,aSubCategory,aUid,a1,a2) \
	BTrace::OutFilteredX \
		(BTRACE_HEADER_C(16,(aCategory),(aSubCategory)),(TUint32)(aUid),(TUint32)(a1),(TUint32)(a2))

/**
Output a trace record of the specified category which also includes a Context ID.

If the value of \a aUid is not contained in the secondary filter then the trace is discarded.

@param aCategory	A value from enum BTrace::TCategory,
@param aSubCategory Sub-category value between 0 and 255.
					The meaning of this is dependent on the Category.
@param aUid 		The first 32bit quantity which forms the data of this trace record.
@param a1			The second 32bit quantity which forms the data of this trace record.
@param aData		Address of addition data to add to trace.
					Must be word aligned, i.e. a multiple of 4.
@param aDataSize	Number of bytes of additional data. If this value is greater than
					KMaxBTraceDataArray then data is truncated to this size and the
					flag ERecordTruncated is set in the record.

@return True if trace is enabled for aCategory and aUid, false otherwise.
@publishedPartner
@released
*/
#define BTraceFilteredContextN(aCategory,aSubCategory,aUid,a1,aData,aDataSize) \
	BTrace::OutFilteredNX \
		(BTRACE_HEADER_C(12,(aCategory),(aSubCategory)),(TUint32)(aUid),(TUint32)(a1),aData,aDataSize)

/**
Output a trace record of the specified category which also includes a Context ID.

If the specified data is too big to find into a single trace record, then a
multipart trace is generated. See TMultiPart.

If the value of \a aUid is not contained in the secondary filter then the trace is discarded.

@param aCategory	A value from enum BTrace::TCategory,
@param aSubCategory Sub-category value between 0 and 255.
					The meaning of this is dependent on the Category.
@param aUid 		The first 32bit quantity which forms the data of this trace record.
@param aData		Address of addition data to add to trace.
					Must be word aligned, i.e. a multiple of 4.
@param aDataSize	Number of bytes of additional data.

@return True if trace is enabled for aCategory and aUid, false otherwise.
@publishedPartner
@released
*/
#define BTraceFilteredContextBig(aCategory,aSubCategory,aUid,aData,aDataSize) \
	BTrace::OutFilteredBig \
		(BTRACE_HEADER_C(8,(aCategory),(aSubCategory)),(TUint32)(aUid),aData,(TInt)(aDataSize))



/**
Output a trace record of the specified category which also includes a Program Counter value.

The trace record data is 4 bytes in size.

If the value of \a aUid is not contained in the secondary filter then the trace is discarded.

@param aCategory	A value from enum BTrace::TCategory,
@param aSubCategory Sub-category value between 0 and 255.
					The meaning of this is dependent on the Category.
@param aUid 		The 32bit quantity which forms the data of this trace record.

@return True if trace is enabled for aCategory and aUid, false otherwise.
@publishedPartner
@released
*/
#define BTraceFilteredPc4(aCategory,aSubCategory,aUid)	\
	BTrace::OutFiltered \
		(BTRACE_HEADER_P(8,(aCategory),(aSubCategory)),(TUint32)(aUid),0,0)

/**
Output a trace record of the specified category which also includes a Program Counter value.

The trace record data is 8 bytes in size.

If the value of \a aUid is not contained in the secondary filter then the trace is discarded.

@param aCategory	A value from enum BTrace::TCategory,
@param aSubCategory Sub-category value between 0 and 255.
					The meaning of this is dependent on the Category.
@param aUid 		The first 32bit quantity which forms the data of this trace record.
@param a1			The second 32bit quantity which forms the data of this trace record.

@return True if trace is enabled for aCategory and aUid, false otherwise.
@publishedPartner
@released
*/
#define BTraceFilteredPc8(aCategory,aSubCategory,aUid,a1) \
	BTrace::OutFiltered \
		(BTRACE_HEADER_P(12,(aCategory),(aSubCategory)),(TUint32)(aUid),(TUint32)(a1),0)

/**
Output a trace record of the specified category which also includes a Program Counter value.

The trace record data is 12 bytes in size.

If the value of \a aUid is not contained in the secondary filter then the trace is discarded.

@param aCategory	A value from enum BTrace::TCategory,
@param aSubCategory Sub-category value between 0 and 255.
					The meaning of this is dependent on the Category.
@param aUid 		The first 32bit quantity which forms the data of this trace record.
@param a1			The second 32bit quantity which forms the data of this trace record.
@param a2			The third 32bit quantity which forms the data of this trace record.

@return True if trace is enabled for aCategory and aUid, false otherwise.
@publishedPartner
@released
*/
#define BTraceFilteredPc12(aCategory,aSubCategory,aUid,a1,a2) \
	BTrace::OutFiltered \
		(BTRACE_HEADER_P(16,(aCategory),(aSubCategory)),(TUint32)(aUid),(TUint32)(a1),(TUint32)(a2))

/**
Output a trace record of the specified category which also includes a Program Counter value.

If the value of \a aUid is not contained in the secondary filter then the trace is discarded.

@param aCategory	A value from enum BTrace::TCategory,
@param aSubCategory Sub-category value between 0 and 255.
					The meaning of this is dependent on the Category.
@param aUid 		The first 32bit quantity which forms the data of this trace record.
@param a1			The second 32bit quantity which forms the data of this trace record.
@param aData		Address of addition data to add to trace.
					Must be word aligned, i.e. a multiple of 4.
@param aDataSize	Number of bytes of additional data. If this value is greater than
					KMaxBTraceDataArray then data is truncated to this size and the
					flag ERecordTruncated is set in the record.

@return True if trace is enabled for aCategory and aUid, false otherwise.
@publishedPartner
@released
*/
#define BTraceFilteredPcN(aCategory,aSubCategory,aUid,a1,aData,aDataSize) \
	BTrace::OutFilteredN \
		(BTRACE_HEADER_P(12,(aCategory),(aSubCategory)),(TUint32)(aUid),(TUint32)(a1),aData,aDataSize)

/**
Output a trace record of the specified category which also includes a Program Counter value.

If the specified data is too big to find into a single trace record, then a
multipart trace is generated. See TMultiPart.

If the value of \a aUid is not contained in the secondary filter then the trace is discarded.

@param aCategory	A value from enum BTrace::TCategory,
@param aSubCategory Sub-category value between 0 and 255.
					The meaning of this is dependent on the Category.
@param aUid 		The first 32bit quantity which forms the data of this trace record.
@param aData		Address of addition data to add to trace.
					Must be word aligned, i.e. a multiple of 4.
@param aDataSize	Number of bytes of additional data.

@return True if trace is enabled for aCategory and aUid, false otherwise.
@publishedPartner
@released
*/
#define BTraceFilteredPcBig(aCategory,aSubCategory,aUid,aData,aDataSize) \
	BTrace::OutFilteredBig \
		(BTRACE_HEADER_P(8,(aCategory),(aSubCategory)),(TUint32)(aUid),aData,(TInt)(aDataSize))




/**
Output a trace record of the specified category which also includes
Context ID and Program Counter values.

The trace record data is 4 bytes in size.

If the value of \a aUid is not contained in the secondary filter then the trace is discarded.

@param aCategory	A value from enum BTrace::TCategory,
@param aSubCategory Sub-category value between 0 and 255.
					The meaning of this is dependent on the Category.
@param aUid 		The 32bit quantity which forms the data of this trace record.

@return True if trace is enabled for aCategory and aUid, false otherwise.
@publishedPartner
@released
*/
#define BTraceFilteredContextPc4(aCategory,aSubCategory,aUid)	\
	BTrace::OutFilteredX \
		(BTRACE_HEADER_CP(8,(aCategory),(aSubCategory)),(TUint32)(aUid),0,0)

/**
Output a trace record of the specified category which also includes
Context ID and Program Counter values.

The trace record data is 8 bytes in size.

If the value of \a aUid is not contained in the secondary filter then the trace is discarded.

@param aCategory	A value from enum BTrace::TCategory,
@param aSubCategory Sub-category value between 0 and 255.
					The meaning of this is dependent on the Category.
@param aUid 		The first 32bit quantity which forms the data of this trace record.
@param a1			The second 32bit quantity which forms the data of this trace record.

@return True if trace is enabled for aCategory and aUid, false otherwise.
@publishedPartner
@released
*/
#define BTraceFilteredContextPc8(aCategory,aSubCategory,aUid,a1) \
	BTrace::OutFilteredX \
		(BTRACE_HEADER_CP(12,(aCategory),(aSubCategory)),(TUint32)(aUid),(TUint32)(a1),0)

/**
Output a trace record of the specified category which also includes
Context ID and Program Counter values.

The trace record data is 12 bytes in size.

If the value of \a aUid is not contained in the secondary filter then the trace is discarded.

@param aCategory	A value from enum BTrace::TCategory,
@param aSubCategory Sub-category value between 0 and 255.
					The meaning of this is dependent on the Category.
@param aUid 		The first 32bit quantity which forms the data of this trace record.
@param a1			The second 32bit quantity which forms the data of this trace record.
@param a2			The third 32bit quantity which forms the data of this trace record.

@return True if trace is enabled for aCategory and aUid, false otherwise.
@publishedPartner
@released
*/
#define BTraceFilteredContextPc12(aCategory,aSubCategory,aUid,a1,a2) \
	BTrace::OutFilteredX \
		(BTRACE_HEADER_CP(16,(aCategory),(aSubCategory)),(TUint32)(aUid),(TUint32)(a1),(TUint32)(a2))

/**
Output a trace record of the specified category which also includes
Context ID and Program Counter values.

If the value of \a aUid is not contained in the secondary filter then the trace is discarded.

@param aCategory	A value from enum BTrace::TCategory,
@param aSubCategory Sub-category value between 0 and 255.
					The meaning of this is dependent on the Category.
@param aUid 		The first 32bit quantity which forms the data of this trace record.
@param a1			The second 32bit quantity which forms the data of this trace record.
@param aData		Address of addition data to add to trace.
					Must be word aligned, i.e. a multiple of 4.
@param aDataSize	Number of bytes of additional data. If this value is greater than
					KMaxBTraceDataArray then data is truncated to this size and the
					flag ERecordTruncated is set in the record.

@return True if trace is enabled for aCategory and aUid, false otherwise.
@publishedPartner
@released
*/
#define BTraceFilteredContextPcN(aCategory,aSubCategory,aUid,a1,aData,aDataSize) \
	BTrace::OutFilteredNX \
		(BTRACE_HEADER_CP(12,(aCategory),(aSubCategory)),(TUint32)(aUid),(TUint32)(a1),aData,aDataSize)

/**
Output a trace record of the specified category which also includes
Context ID and Program Counter values.

If the specified data is too big to find into a single trace record, then a
multipart trace is generated. See TMultiPart.

If the value of \a aUid is not contained in the secondary filter then the trace is discarded.

@param aCategory	A value from enum BTrace::TCategory,
@param aSubCategory Sub-category value between 0 and 255.
					The meaning of this is dependent on the Category.
@param aUid 		The first 32bit quantity which forms the data of this trace record.
@param aData		Address of addition data to add to trace.
					Must be word aligned, i.e. a multiple of 4.
@param aDataSize	Number of bytes of additional data.

@return True if trace is enabled for aCategory and aUid, false otherwise.
@publishedPartner
@released
*/
#define BTraceFilteredContextPcBig(aCategory,aSubCategory,aUid,aData,aDataSize) \
	BTrace::OutFilteredBig \
		(BTRACE_HEADER_CP(8,(aCategory),(aSubCategory)),(TUint32)(aUid),aData,(TInt)(aDataSize))



//
// Inline methods
//

inline TUint8* BTrace::NextRecord(TAny* aCurrentRecord)
	{
	TUint size = ((TUint8*)aCurrentRecord)[ESizeIndex];
	*(TUint*)&aCurrentRecord += 3;
	*(TUint*)&aCurrentRecord += size;
	*(TUint*)&aCurrentRecord &= ~3;
	return (TUint8*)aCurrentRecord;
	}

#ifdef __KERNEL_MODE__

inline TInt BTrace::Filter(TUint aCategory)
	{
	return SetFilter(aCategory,-1);
	}

#endif

/**
The maximum permissible value for aDataSize in trace outputs.
@see BTraceN BTracePcN BTraceContextN BTraceContextPcN
@publishedPartner
@released
*/
const TUint KMaxBTraceDataArray = 80;



/**
The maximum total number of bytes in a trace record.
@publishedPartner
@released
*/
const TInt KMaxBTraceRecordSize = 7*4+8+KMaxBTraceDataArray;


#ifdef __MARM__
#define BTRACE_MACHINE_CODED
#endif

#endif
