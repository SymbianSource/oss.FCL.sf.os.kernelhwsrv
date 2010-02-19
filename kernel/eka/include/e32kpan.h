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
// e32\include\e32kpan.h
// Kernel-generated panic codes
// 
//

#ifndef __E32KPAN_H__
#define __E32KPAN_H__
#include <e32def.h>

/**
@publishedAll
@released

These panics represent program errors detected by the Kernel.
Typically, they are caused by passing bad or contradictory parameters
to functions. Threads that cause exceptions also raise a KERN-EXEC type panic.
*/
enum TKernelPanic
    {
    
    /**
    This panic is raised when the Kernel cannot find an object in
    the object index for the current process, or current thread, using
    a specified object index number (the raw handle number).
    */
	EBadHandle=0,
	
	/**
	This is a general panic raised as a result of attempting 
	some unauthorised activity such as trying to suspend a thread,
	or trying to set the priority of a thread, when not authorised to do so.
	*/
	EAccessDenied=1,
	
	/**
	This panic is raised by the kernel when opening a kernel side object,
	a DObject type, and the fullname is invalid.
	*/
	EBadName=2,
	
	/**
	This panic is raised when an unhandled exception occurs.
	Exceptions have many causes, but the most common are access violations
	caused, for example, by dreferencing NULL.
	
	Among other possible causes are:
	general protection faults, executing an invalid instruction,
	alignment checks, etc.
	*/
	ECausedException=3,
	
	/**
	Not used.
	*/
	ECompletion=4,
	
	/**
	Not used.
	*/
	ELeaveWithoutTrap=5,
	
	/**
	This panic is raised by the kernel when a handle to a code segment
	is invalid.
	*/
	EBadCodeSegHandle=6,
	
	/**
	Not used.
	*/
	ESegmentWriteOutOfRange=7,
	
	/**
	Not used.
	*/
	EChunkSizeTooBig=8,
	
	/**
	This is a general panic raised by the kernel when an attempt
	is made to issue a request when one is already outstanding, and only one
    outstanding request is permitted at any one time.
	*/
	ERequestAlreadyPending=9,
	
	/**
	This panic is raised by the Request() member function of
	the DLogicalChannel() kernel object when the request number passed
	to the function is smaller than the permitted minimum.
	
	@see DLogicalChannel::EMinRequestId
	*/
	ERequestNoInvalid=10,
	
	/**
	This panic is raised when creating a logical channel, and the unit number
	is outside the permitted range.
	
	If unit numbers are not permmitted, the unit number value
	must be KNullUnit.
	If unit numbers are permitted, the unit number value must
	be less than KMaxUnits.
	*/
	EBadUnitNumber=11,
	
	/**
	This panic is raised by the kernel if an event capture
	hook has already been designated.
	*/
	EEventAlreadyCaptured=12,
	
	/**
	This panic is raised by the kernel if the current thread is not
	the designated event capture hook.
	*/
	EEventNotCaptured=13,
	
	/**
	This panic is raised when an attempt is made to set the priority of
	a thread or process to an illegal value.
	*/
	EBadPriority=14,
	
	/**
	This panic is raised when a timer event is requested from an asynchronous
	timer service, an RTimer, and a timer event is already outstanding.
	It is caused by calling either the At(), After() or Lock() member functions
	after a previous call to any of these functions but before the timer event
	requested by those functions has completed.
	*/
	ETimerAlreadyPending=15,
	
	/**
	Not used.
	*/
	EAlreadyLoggedOn=16,
	
	/**
	The panic is raised if kernel heap checking has failed.
	*/
	EFailedKernelHeapCheck=17,
	
	/**
	Not used.
	*/
	ERequestFromWrongThread=18,

    /**
    This panic is raised by the Kernel when a server program issues a request
    to receive a message, i.e. when it calls the Receive() member function
    of RServer, the handle to the Kernel side server object.
    
    The panic occurs when a receive message request has previously been made
    and is still outstanding.
    */
	EMesAlreadyPending=19,
	
	/**
	This panic is raised by the Kernel when a request for an event
	(as originated by a call to UserSvr::RequestEvent()) is made while
	a previously made request is still outstanding.
	*/
	EEventRequestPending=20,
	
	/**
	Not used.
	*/
	EIllegalWsProcess=21,
	
	/**
	Not used.
	*/
	EHardwareNotAvailable=22,
	
	/**
	This panic is raised when attempting to create a session with a server,
	and access is been denied.
	*/
	EUnsharableSession=23,
	
	/**
	This panic is raised when writing global DLL data, and the length of
	data to be written is greater than the space available.
	*/
	EDesOverflow=24,
	
	/**
	This panic is raised when searching for objects, using the internal
	function TFindHandleBase::NextObject(), and an invalid object type
	is specified.
	*/
	EBadObjectType=25,
	
	/**
	This panic is raised by kernel side code that implements heap debugging
	behaviour, when a specific debug request is not recognised.
	*/
	EBadKernelHeapDebugFunction=26,
	
	/**
	This panic is raised when an executive call is made with an invalid call
	number.
	*/
	EInvalidSystemCall=27,
	
	/**
	Not used.
	*/
	ESetSessionPtrInvalidMessage=28,
	
	/**
	Not used.
	*/
	ECompleteDisconnectInvalidMessage=29,
	
	/**
	This panic is raised when an attempt is being made to send 
	a synchronous message to a server more than once, using
	the current thread's dedicated synchronous message.
	*/
	ESyncMsgSentTwice=30,
	
	/**
	Not used.
	*/
	EAutoAttachFailed=31,
	
	/**
	This panic is called by DProcess::Resume() when trying to resume
	a process that is still being loaded.
	*/
	EProcessNotLoaded=32,
	
	/**
    This panic is raised in a call to Kern::KUDesInfo(), Kern::KUDesPut() etc
    when an invalid descriptor is passed.
	*/
	EKUDesInfoInvalidType=33,
	
	/**
	This panic is raised in a call to Kern::KUDesSetLength() & Kern::KUDesPut()
	when the descriptor passed to it is not a modifiable type.
	*/
	EKUDesSetLengthInvalidType=34,
	
	/**
	This panic is raised in a call to Kern::KUDesSetLength() & Kern::KUDesPut()
	when the length of the source descriptor is longer than the length of
	the target descriptor.
	*/
	EKUDesSetLengthOverflow=35,
	
	/**
	This panic is raised by the kernel side code that implements the setting
	of the currency symbol when the length of the currency symbol is 
    greater than KMaxCurrencySymbol. 
	*/
	EBadParameter=36,
	
	/**
	This panic is raised by kernel code when it tries to acquire the process
	DLL lock just before a load, and the wait DLL lock is invalid.
	*/
	EWaitDllLockInvalid=37,
	
	/**
	This panic is raised by internal kernel code when an illegal attempt
	is made to attatch to a library.
	*/
	ELibraryAttachInvalid=38,
	
	/**
	This panic is raised when extracting a list of DLL entry points
	and the number exceeds the maximum permitted.
	*/
	ETooManyEntryPoints=39,
	
	/**
	This panic is raised by internal kernel code when an illegal attempt
	is made to detach a library.
	*/
	ELibraryDetachInvalid=40,
	
	/**
	This panic is raised by internal kernel code when an illegal attempt
	is made to attach to a library.
	*/
	ELibraryAttachedInvalid=41,

	/**
	This panic is raised by internal kernel code when an illegal attempt
	is made to detach a library.
	*/
	ELibraryDetachedInvalid=42,

	/**
	This panic is raised by kernel code when it tries to release the process DLL lock
	when a load fails, and the release DLL lock is invalid.
	*/
	EReleaseDllLockInvalid=43,
	
	/**
	This panic is raised when a bad message handle is passed to the kernel.
	This usually occurs when using methods on the RMessagePtr2 or RMessage2 classes
	after the message has been completed; or when the iHandle data member has become
	corrupted.
	*/
	EBadMessageHandle=44,
	
	/**
	Not used.
	*/
	EInvalidMessageParameter=45,
	
	/**
	This panic can be raised as a result of a call to one of a large
	number of functions. In general, the panic indicates an attempt to perform an
	operation on a thread or process by code running in another process - violating
	the security principle of process isolation. 
		
	There are exceptions to this general rule, for example, where the
	panic is raised because the calling process has insufficient capability. The
	precise reason is stated with the function(s). 
	
	-# The panic is raised on a call to the following function if the
	   process owning the thread performing the call is not the creator of the target
	   process or, if a handle is specified, the handle is not local.
	   - RProcess::SetParameter()
	   .
	-# The panic is raised on a call to the following functions if the
	   process owning the thread performing the call is not the same as the target
	   process.\n
	   - RProcess::Kill()
	   - RProcess::Terminate()
	   - RProcess::Panic()
	   - RProcess::SetJustInTime()
	   - RProcess::Resume()
	   - RThread::Kill()
	   - RThread::Terminate()
	   - RThread::Panic()
	   - RThread::Suspend()
	   - RThread::Resume()
	   - RThread::SetPriority()		 
	   - RThread::RequestComplete()
	   - RThread::RequestSignal()		 
	   .  
	   NOTE: the creator of a new process can kill or panic the new
	   process, change the new process priority and set the new process startup
	   parameters until the process is resumed (which the creator can also do). After
	   the new process has been resumed, then it becomes totally independent of its
	   creator, and any attempt to panic it, kill it etc will raise the KERN-EXEC 46
	   panic.
	-# The panic is raised on call to the following (Symbian partner
	   only) functions if the calling process does not have the PowerMgmt
	   capability (TCapability::ECapabilityPowerMgmt): 
	   - Power::PowerDown()
	   - Power::EnableWakeupEvents()
	   - Power::DisableWakeupEvents()
	   - Power::RequestWakeupEventNotification()
	   - Power::CancelWakeupEventNotification()
	   .
	-# The panic is raised on call to the following functions if the
	   calling process does not have the WriteDeviceData capability
       (TCapability::ECapabilityWriteDeviceData): 
	   - User::SetMachineConfiguration()
	   - User::SetHomeTime()
	   - User::SetUTCTime()
	   - User::SetUTCOffset()
	   - User::SetUTCTimeAndOffset()
	   .
	-# The panic is raised on call to the following function if the
	   calling process does not have the ReadDeviceData capability
	   (TCapability::ECapabilityReadDeviceData): 		  
	   - User::MachineConfiguration()
	   .
	*/
	EPlatformSecurityTrap=46,
	
	/**
	This panic is raised when the user issues a request to be notified of
	messages or the availability of space, when a request has already been
	issued and is still outstanding.
	
	@see RMsgQueue
	*/
	EMsgQueueRequestPending=47,
	
	/**
	This panic is raised when creating a message queue and the size of
	the template parameter is invalid.
	
	@see RMsgQueue
	*/
	EMsgQueueInvalidLength=48,
	
	/**
	This panic is raised when creating a message queue and the specified number
	of slots is not positive.
	
	@see RMsgQueue
	*/
	EMsgQueueInvalidSlots=49,
	
	/**
	This panic is raised if an attempt is made to cancel an outstanding request 
	to be notified of messages or the availability of space, and the cancel is
	being made by a thread in a different process.
	*/
	EMsgQueueIllegalCancel=50,
	
	/**
	This panic is raised by RProcess::Setparameter()
	if a slot value is invalid.
	*/
	EParameterSlotRange=51,
	
	/**
	This panic is raised by RProcess::Setparameter()
	if a slot is in use.
	*/
	EParameterSlotInUse=52,
	
	/**
	This panic is raised by RProcess::Setparameter()
	if the length of the data passed is negative.
	*/
	EParameterSlotDataLength=53,
	
	/**
	This panic is raised by RCondVar::Wait() when the current thread does
	not hold the specified mutex.
	*/
	ECondVarWaitMutexNotLocked=54,
	
	/**
	This panic is raised when a call is made to RThread::GetDesMaxLength(),
	which is obsolete.
	*/
	EObsoleteFunctionality=55,

	/**
	This panic is raised on a process which has not yet been resumed and whoes
	creator has died.
	*/
	EZombieProcessKilled=56,

	/**
	A connect message was sent to a session that has already been successfully
	connected to the server (cookie is non-NULL).
	*/
	ESessionAlreadyConnected=57,

	/**
	A session tried to set the kernel session cookie to a null value
	*/
	ESessionNullCookie=58,

	/**
	A session tried to set the kernel session cookie twice
	*/
	ESessionCookieAlreadySet=59,

	/**
	A session tried to set the kernel session cookie with a message
	that wasn't the connect message
	*/
	ESessionInvalidCookieMsg=60,

	/**
	A realtime thread executed a non-realtime function.
	*/
	EIllegalFunctionForRealtimeThread=61,

	/**
	This panic is raised by Kern::SetThreadRealtimeState when the state argument is invalid.
	*/
	EInvalidRealtimeState=62,

	/**
	A bad descriptor was passed to a server over IPC, causing an exception when the kernel tried to
	update its length field.
	*/
	EBadIpcDescriptor=63,

	/**
	An invalid notification type was passed to a shared buffers exec call handler.
	*/
	EShBufExecBadNotification=64,

	/**
	An invalid parameter was passed to a shared buffers exec call handler.
	*/
	EShBufExecBadParameter=65,

    /**
	An entropy estimate passed to an entropy buffer was outside the allowed range.
	*/
	EEntropyEstimateOutOfRange=66,
	};

#endif
