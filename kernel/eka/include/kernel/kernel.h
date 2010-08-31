// Copyright (c) 1994-2010 Nokia Corporation and/or its subsidiary(-ies).
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
// e32\include\kernel\kernel.h
// Public header for device drivers
// 
// WARNING: This file contains some APIs which are internal and are subject
//          to change without notice. Such APIs should therefore not be used
//          outside the Kernel and Hardware Services package.
//


#ifndef __K32STD_H__
#define __K32STD_H__

#include <kernel/klib.h>
#include <e32kpan.h>
#include <u32std.h>
#include <e32ldr.h>
#include <e32ldr_private.h>
#include <e32event.h>
#include <e32event_private.h>
#include <d32locd.h>
#include <kernel/localise.h>
#include <nkern.h>
#include <kernel/sproperty.h>

class DObject;
class DObjectCon;
class DThread;
class DCodeSeg;
class DProcess;
class DLibrary;
class DMutex;
class DSemaphore;
class DChunk;
class DShPool;
class DShBuf;


/** Data type for physical addresses
@publishedPartner
@released
*/
typedef TUint32 TPhysAddr;


/**
A constant representing an invalid physical address.
@publishedPartner
@released
*/
const TPhysAddr KPhysAddrInvalid=0xFFFFFFFFu;


/** @internalComponent */
_LIT(KLitKernExec, "KERN-EXEC");

/********************************************
 * HAL entry array
 ********************************************/

/** @internalTechnology */
const TInt KMaxHalGroups=32;




/**
@publishedPartner
@released

Defines the signature for a HAL handler function.
*/
typedef TInt (*THalFunc)(TAny*,TInt,TAny*,TAny*);

/** @internalTechnology */
const TInt KMaxHalEntries=8;

/** @internalTechnology */
struct SHalEntry
	{
	THalFunc iFunction;
	TAny* iPtr;
	};

/** @internalTechnology */
struct SHalEntry2 : public SHalEntry
	{
	SHalEntry* iExtendedEntries;
	};

/********************************************
 * Exports from layer 1 of the kernel
 ********************************************/
class TThreadMessage;
class DObjectCon;
class DPowerModel;
class TSuperPage;
class TMachineConfig;

/********************************************
 * Thread creation info block
 ********************************************/

/**
@publishedPartner
@released

Defines a set of thread types.
*/
enum TThreadType
	{
	/** The thread is the initial thread
		@internalComponent
	*/
	EThreadInitial,

	/** The thread runs in supervisor mode */
	EThreadSupervisor,

	/** The thread runs in supervisor mode and has no handles array */
	EThreadMinimalSupervisor,

	/** The thread runs in user mode */
	EThreadUser,

	/** The thread is the initial thread on a non-boot processor (SMP only)
		@internalComponent
	*/
	EThreadAPInitial,
	};

/********************************************
 * Kernel Mutex Ordering
 ********************************************/
const TUint8 KMutexOrdNone				= 0xff; /**< @internalComponent */
const TUint8 KMutexOrdUser				= 0xfe; /**< @internalComponent */

/**
Mutex order value for general purpose use. This value is higher than any used internally
by the kernel, therefore there are no mutex ordering restrictions that limit which kernel
functions may be called whilst a mutex of this order is held.
@see Kern::MutexCreate()
@publishedPartner
@released
*/
const TUint8 KMutexOrdGeneral7			= 0xf7;

/**
Mutex order value for general purpose use. This value is higher than any used internally
by the kernel, therefore there are no mutex ordering restrictions that limit which kernel
functions may be called whilst a mutex of this order is held.
@see Kern::MutexCreate()
@publishedPartner
@released
*/
const TUint8 KMutexOrdGeneral6			= 0xf6;

/**
Mutex order value for general purpose use. This value is higher than any used internally
by the kernel, therefore there are no mutex ordering restrictions that limit which kernel
functions may be called whilst a mutex of this order is held.
@see Kern::MutexCreate()
@publishedPartner
@released
*/
const TUint8 KMutexOrdGeneral5			= 0xf5;

/**
Mutex order value for general purpose use. This value is higher than any used internally
by the kernel, therefore there are no mutex ordering restrictions that limit which kernel
functions may be called whilst a mutex of this order is held.
@see Kern::MutexCreate()
@publishedPartner
@released
*/
const TUint8 KMutexOrdGeneral4			= 0xf4;

/**
Mutex order value for general purpose use. This value is higher than any used internally
by the kernel, therefore there are no mutex ordering restrictions that limit which kernel
functions may be called whilst a mutex of this order is held.
@see Kern::MutexCreate()
@publishedPartner
@released
*/
const TUint8 KMutexOrdGeneral3			= 0xf3;

/**
Mutex order value for general purpose use. This value is higher than any used internally
by the kernel, therefore there are no mutex ordering restrictions that limit which kernel
functions may be called whilst a mutex of this order is held.
@see Kern::MutexCreate()
@publishedPartner
@released
*/
const TUint8 KMutexOrdGeneral2			= 0xf2;

/**
Mutex order value for general purpose use. This value is higher than any used internally
by the kernel, therefore there are no mutex ordering restrictions that limit which kernel
functions may be called whilst a mutex of this order is held.
@see Kern::MutexCreate()
@publishedPartner
@released
*/
const TUint8 KMutexOrdGeneral1			= 0xf1;

/**
Mutex order value for general purpose use. This value is higher than any used internally
by the kernel, therefore there are no mutex ordering restrictions that limit which kernel
functions may be called whilst a mutex of this order is held.
@see Kern::MutexCreate()
@publishedPartner
@released
*/
const TUint8 KMutexOrdGeneral0			= 0xf0;


const TUint8 KMutexOrdRamDrive			= KMutexOrdGeneral7; /**< @internalComponent */
const TUint8 KMutexOrdShPool			= 0x68; /**< @internalComponent */
const TUint8 KMutexOrdCodeSegLock		= 0x60; /**< @internalComponent */
const TUint8 KMutexOrdPubSub2			= 0x5e; /**< @internalComponent */

/**
@internalComponent
@prototype
*/
const TUint8 KMutexOrdPageIn			= 0x5c;

const TUint8 KMutexOrdRamDefrag			= 0x59; /**< @internalComponent */
const TUint8 KMutexOrdPowerMgr			= 0x58; /**< @internalComponent */
const TUint8 KMutexOrdPubSub			= 0x50; /**< @internalComponent */
const TUint8 KMutexOrdProcessLock		= 0x48; /**< @internalComponent */
const TUint8 KMutexOrdDebug				= 0x47; /**< @internalComponent */
const TUint8 KMutexOrdTimer				= 0x40; /**< @internalComponent */
const TUint8 KMutexOrdObjectCon2		= 0x38;	/**< @internalComponent */ // servers
const TUint8 KMutexOrdHandle			= 0x30; /**< @internalComponent */
const TUint8 KMutexOrdObjectCon			= 0x28; /**< @internalComponent */
const TUint8 KMutexOrdMachineConfig		= 0x20; /**< @internalComponent */
const TUint8 KMutexOrdEntropyPool		= 0x12; /**< @internalComponent */
const TUint8 KMutexOrdRandNumGeneration = 0x11; /**< @internalComponent */
const TUint8 KMutexOrdHwChunk			= 0x10; /**< @internalComponent */
const TUint8 KMutexOrdKernelHeap		= 0x08; /**< @internalComponent */
const TUint8 KMutexOrdRamAlloc			= 0x04; /**< @internalComponent */
#if defined(__MEMMODEL_FLEXIBLE__)
const TUint8 KMutexOrdSyncPhysMem		= 0x03; /**< @internalComponent */
const TUint8 KMutexOrdPageOut			= 0x02; /**< @internalComponent */
#endif
const TUint8 KMutexOrdResourceManager	= 0x01; /**< @internalComponent */


/********************************************
 * Kernel Extension Priority Ordering
 ********************************************/

/**
@internalComponent
@prototype 9.5
*/
const TUint8 KExtensionMaximumPriority = 0xff;

/**
@internalComponent
@prototype 9.5
*/
const TUint8 KExtensionStandardPriority = 0;


/**
Defines a function type that implements a polling operation.

A function of this type takes a single TAny* type argument, and returns
a TBool type, and is passed as an argument to Kern::PollingWait().

@see Kern::PollingWait()

@publishedPartner
@released
*/
typedef TBool (*TPollFunction)(TAny*);




/**
Structure used to specify parameters to Kern::ChunkCreate()
@see Kern::ChunkCreate()
@publishedPartner
@released
*/
class TChunkCreateInfo
    {
public:
    /**
    Enumeration representing the type of chunk to be created.
    */
    enum TType
        {
        /**
        A chunk which may only be opened by one user side process at a time.
        Chunks of this type are slightly more efficient than the 
        ESharedKernelMultiple type when used on the Moving Memory Model.
        */
        ESharedKernelSingle = 9,

        /**
        A chunk which may be opened by any number of user side processes.
        */
        ESharedKernelMultiple = 10,
        };

    /**
    The chunk type to be created.
    @see TChunkCreateInfo::TType
    */
    TType iType;

    /**
    The size of linear address space to reserve for this chunk.
    */
    TInt iMaxSize;

    /**
    Caching attributes for the chunks memory.

    This is a value constructed from the TMappingAttributes values.
    E.g. EMapAttrFullyBlocking (no caching) or EMapAttrCachedMax (full caching).

    Note that if the MMU doesn't support the requested attributes then a lesser
    cached attribute will be used. The actual value used is returned in aMapAttr of
    Kern::ChunkCreate()

    @see TMappingAttributes
    */
    TUint32 iMapAttr;

    /**
    Set to true if the chunk is to own its committed memory. In which case all
    memory committed to the chunk will come from the system's free pool and will be
    returned there when the chunk is destroyed.
    */
    TUint8 iOwnsMemory;

    /**
    @internalComponent
    Reserved for future expansion.
    */
    TInt8 iSpare8[3];

    /**
    Pointer to a DFC which will be queued on destruction of chunk.
    */
    TDfc* iDestroyedDfc;

    /**
    @internalComponent
    Reserved for future expansion.
    */
    TInt32 iSpare32[2];
public:
    /**
    Constructor which zeros all member data.
    */
    inline TChunkCreateInfo()
        { memset(this,0,sizeof(*this)); }
    };



/**
Generic kernel hook function.
@internalComponent
*/
typedef TInt (*TKernelHookFn)();

/**
Available kernel hooks.
@internalComponent
*/
enum TKernelHookType
	{
	EHookTrace,
	EHookNanoWait,
	EHookInitialTime,
	
	ENumKernelHooks
	};


/*
Specifies the origin of the log when calling trace handler hook.
@publishedPartner
@released
*/
enum TTraceSource
	{
	/**
	User side tracing
	@see User::DebugPrint
	*/
	EUserTrace,
	/**
	Kernel tracing
	@see Kern::Print
	*/
	EKernelTrace,
	/**
	Platform security tracing
	*/
	EPlatSecTrace,
	};

/*
Trace handler hook
@param aText      Debug log. The content of the descriptor resides in kernel memory.
@param aTraceType Identifies the origin of the debug log.
@see TTraceHandlerType
@return Specifies whether the log is processed or not.
        If ETrue,  the log is processed. Kernel will drop the log (it won't be passed to trace port - UART)
        If EFalse, the log is not processed. Kernel will pass the log to the trace port, as well.
@publishedPartner
@released
*/
typedef TBool (*TTraceHandler)(const TDesC8& /*aText*/, TTraceSource /*aTraceSource*/);


/**
Defines the prototype of the kernel hook for the Kern::NanoWait implementation.
@see Kern::NanoWait
@see Kern::SetNanoWaitHandler
@publishedPartner
@released
*/
typedef void (*TNanoWaitHandler)(TUint32 aInterval);


/**
Defines the prototype of the kernel hook to get the initial system time.

The hook is called during boot to get the inital system time.  It should attempt
to read the time from the hardware RTC.  If the contents of the RTC are invalid
it should return KErrCorrupt.
 
@return The time in seconds from 00:00:00 01-01-2000, or one of the
system-wide error codes.

@see Kern::SetInitialTimeHandler
@see P::InitSystemTime
@publishedPartner
@released
*/
typedef TInt (*TInitialTimeHandler)();


/**
A thread's realtime state.

Some non-realtime behaviour can be detected by the kernel. When it does so, action is taken
depending on the thread state:
   
-	ERealtimeStateOff - no action.
-	ERealtimeStateOn - the the thread will be panicked with KERN-EXEC 61 (EIllegalFunctionForRealtimeThread).
-	ERealtimeStateWarn - no action. However, if the kernel trace flag KREALTIME is enabled
	then tracing will be emitted as if the thread state was ERealtimeStateOn.

@publishedPartner
@released
*/
enum TThreadRealtimeState
	{
	ERealtimeStateOff,	/**< Thread is not realtime */
	ERealtimeStateOn,	/**< Thread is realtime */
	ERealtimeStateWarn	/**< Thread is realtime but doesn't want this enforced */
	};


/**
A DFC queue intended to be created and destroyed as needed.

This class extends the TDfcQue class with a destroy method.

@publishedPartner
@released
*/
class TDynamicDfcQue : public TDfcQue
	{
public:
	TDynamicDfcQue();
	IMPORT_C void Destroy();
	IMPORT_C void SetRealtimeState(TThreadRealtimeState aNewState);
private:	
	TDfc iKillDfc;
	};


/**
An object representing an asynchronous request from a user thread, containing a TRequestStatus
pointer.

It can be queued for completion when the thread next returns to user-mode, to move the impact of any
page faults from the thread posting the completion to the thread that made the request.

@publishedPartner
@released
*/
class TClientRequest : public TUserModeCallback
	{
public:
	IMPORT_C TInt SetStatus(TRequestStatus*);
	IMPORT_C void Reset();
	IMPORT_C TRequestStatus* StatusPtr();
	IMPORT_C TBool IsReady();
public:
	void Close();
protected:
	enum TState
		{
		EFree,
		EReady,
		EInUse,
		EClosing,
		EBad
		};
	TClientRequest(TUserModeCallbackFunc aCallback = CallbackFunc);
	~TClientRequest();
	TState State();
	static TState GetState(T_UintPtr aStatus);
	TBool StartComplete(DThread* aThread, TInt aReason);
	void EndComplete(DThread* aThread);
	static void CallbackFunc(TAny* aData, TUserModeCallbackReason aReason);
private:
	static TDfc DeadClientCleanupDfc;
	static void DoDeadClientCleanup(TAny*);
private:
	T_UintPtr MakeFree();
public:
	volatile T_UintPtr iStatus;		// holds TRequestStatus pointer and state flag bits
	TInt iResult;
	friend class Kern;
	friend class K;
	};


/**
Base classs for TClientDataRequest.

@internalTechnology
*/
class TClientDataRequestBase : public TClientRequest
	{
public:
	/**
	Set the destination in the client's address space where the data will be copied to when the
	request is completed.	
	@publishedPartner
	@released
	*/
	inline void SetDestPtr(TAny* aPtr) { iDestPtr = aPtr; } /**< */
	/**
	Get the destination address in the client's address space.
	@publishedPartner
	@released
	*/
	inline TAny* DestPtr() { return iDestPtr; }
	/**
	Get the local address of the buffer where the structure data will be held until it is copied to
	the client.
	@publishedPartner
	@released
	*/
	inline TUint8* Buffer() { return (TUint8*)this + _ALIGN_UP(sizeof(*this), 8); }
protected:
	TClientDataRequestBase(TInt aBufferSize);
	~TClientDataRequestBase() { }	// call Close(), don't delete
	static void CallbackFunc(TAny* aData, TUserModeCallbackReason aReason);
public:
	TInt iSize;
	TAny* iDestPtr;
	friend class Kern;
	};

/**
An object representing an asynchronous request from a user thread that involves copying a small
fixed-size structure to the client.  It contains a TRequestStatus pointer and a buffer to hold the
structure.

It can be queued for completion when the thread next returns to user-mode, to move the impact of any
page faults from the thread posting the completion to the thread that made the request.

@publishedPartner
@released
*/
template <class T>
class TClientDataRequest : public TClientDataRequestBase
	{
public:
	T& Data() { return *(T*)Buffer(); }
private:
	TClientDataRequest();
	~TClientDataRequest();	// call Close(), don't delete
	};

/**
Base classs for TClientDataRequest2.

@prototype
@internalTechnology
*/
class TClientDataRequestBase2 : public TClientRequest
	{
public:
	inline void SetDestPtr1(TAny* aPtr) { iDestPtr1 = aPtr; }
	inline TAny* DestPtr1() { return iDestPtr1; }
	inline TUint8* Buffer1() { return (TUint8*)this + _ALIGN_UP(sizeof(*this), 8); }
	inline void SetDestPtr2(TAny* aPtr) { iDestPtr2 = aPtr; }
	inline TAny* DestPtr2() { return iDestPtr2; }
	inline TUint8* Buffer2() { return Buffer1() + _ALIGN_UP(iSize1, 8); }
protected:
	TClientDataRequestBase2(TInt aBufferSize1, TInt aBufferSize2);
	~TClientDataRequestBase2() { }	// call Close(), don't delete
	static void CallbackFunc(TAny* aData, TUserModeCallbackReason aReason);
public:
	TInt iSize1;
	TAny* iDestPtr1;
	TInt iSize2;
	TAny* iDestPtr2;
	friend class Kern;
	};

/**
An object representing an asynchronous request from a user thread that involves copying two small
fixed-size structures to the client.  It contains a TRequestStatus pointer and two buffer to hold
the structures.

It can be queued for completion when the thread next returns to user-mode, to move the impact of any
page faults from the thread posting the completion to the thread that made the request.

@prototype
@internalTechnology
*/
template <class T1, class T2>
class TClientDataRequest2 : public TClientDataRequestBase2
	{
public:
	T1& Data1() { return *(T1*)Buffer1(); }
	T2& Data2() { return *(T2*)Buffer2(); }
private:
	TClientDataRequest2();
	~TClientDataRequest2();	// call Close(), don't delete
	};

/**
A parsed descriptor header.

This has the same internal structure as TRawDesHeader but the header data is stored in a parsed
representation.

@see TRawDesHeader.

@prototype
@internalTechnology
*/
class TDesHeader
	{
public:
	enum { KConstMaxLength = (TUint)KErrBadDescriptor };
	inline TDesHeader();
	/// Set the contents of this object
	inline void Set(TUint32 aTypeAndLength, TLinAddr aDataPtr, TUint aMaxLength = KConstMaxLength);
	/// Reset the object to its initial un-set state
	inline void Unset();
	/// Determine whether this object has been set to a valid descriptor header
	inline TBool IsSet() const;
	/// Set both type and length fields in one operation
	inline void SetTypeAndLength(TUint32 aTypeAndLength);
	// Accessors
	inline const TUint32& TypeAndLength() const;
	inline TDesType Type() const;
	inline TInt Length() const;
	inline TBool IsWriteable() const;
	inline TInt MaxLength() const;
	inline TLinAddr DataPtr() const;
private:
	enum { KUnsetFlag = 0xffffffff };
	TUint32 iData[3];
	};

inline TDesHeader::TDesHeader()
	{
	iData[0] = (TUint32)KUnsetFlag;
	}

inline void TDesHeader::Set(TUint32 aTypeAndLength, TLinAddr aDataPtr, TUint aMaxLength)
	{
	iData[0] = aTypeAndLength;
	iData[1] = aMaxLength;
	iData[2] = aDataPtr;
	}

inline void TDesHeader::Unset()
	{
	iData[0] = (TUint32)KUnsetFlag;
	}

inline TBool TDesHeader::IsSet() const
	{
	return iData[0] != KUnsetFlag;
	}

inline void TDesHeader::SetTypeAndLength(TUint32 aTypeAndLength)
	{
	iData[0] = aTypeAndLength;
	}

inline const TUint32& TDesHeader::TypeAndLength() const
	{
	return iData[0];
	}

inline TDesType TDesHeader::Type() const
	{
	return (TDesType)(iData[0] >> KShiftDesType);
	}

inline TInt TDesHeader::Length() const
	{
	return (TInt)(iData[0] & KMaskDesLength);
	}

inline TBool TDesHeader::IsWriteable() const
	{
	return iData[1] != KConstMaxLength;
	}

inline TInt TDesHeader::MaxLength() const
	{
	return (TInt)iData[1];
	}

inline TLinAddr TDesHeader::DataPtr() const
	{
	return (TLinAddr)iData[2];
	}

/**
An object representing a client buffer that resides in user-side memory.

TClientBuffer objects can be used to specify memory to pin using Kern::PinVirtualMemory
@see, and can be read and written using Kern::ThreadBufRead and Kern::ThreadBufWrite.

@see TClientBufferRequest
@see Kern::PinVirtualMemory
@see Kern::UnpinVirtualMemory
@see Kern::ThreadBufRead
@see Kern::ThreadBufWrite

@publishedPartner
@released
*/
class TClientBuffer
	{
public:
	IMPORT_C TClientBuffer();
	IMPORT_C TInt SetFromDescriptor(TAny* aDesPtr, DThread* aClientThread = NULL);
	IMPORT_C void SetFromBuffer(TLinAddr aStartAddr, TInt aLength, TBool aWriteable);
	IMPORT_C TBool IsSet() const;
	IMPORT_C void Reset();
	IMPORT_C TBool IsWriteable() const;
	IMPORT_C TInt Length() const;
	IMPORT_C TInt MaxLength() const;
	IMPORT_C TInt UpdateDescriptorLength(DThread* aClientThread = NULL);
public:
	TAny* DesPtr() const;
	TAny* DataPtr() const;
private:
	enum TFlags
		{
		EIsBuffer = 1
		};
private:
	TUint32 iPtr;
	TDesHeader iHeader;
	friend class Kern;
	};

class TVirtualPinObject;
class TPhysicalPinObject;
class TShPool;
class TShBuf;

/**
An object representing a device driver request that involves writing data to one or more user-side
buffers.

It handles pinning of the buffers when the request is set up, so that the memory can be accessed in
kernel thread context.

It can be queued for completion when the thread next returns to user-mode.  Any writeable
descriptors have their lengths written back to the client at the same time as the completion value.

The following operations can only be called from client thread context:

  StartSetup
  AddBuffer
  EndSetup

These operations can be called from any thread context:

  Kern::QueueRequestComplete

@publishedPartner
@released
*/
class TClientBufferRequest : private TClientRequest
	{
public:
	enum TFlags
		{
		/** A flag indicating that buffers should have their virtual memory pinned. */
		EPinVirtual = 1
		};
	IMPORT_C TInt StartSetup(TRequestStatus* aStatus);
	IMPORT_C TInt AddBuffer(TClientBuffer*& aBufOut, TAny* aDesPtr);
	IMPORT_C TInt AddBuffer(TClientBuffer*& aBufOut, TLinAddr aStartAddr, TInt aLength, TBool aWriteable = EFalse);
	IMPORT_C void EndSetup();
	IMPORT_C void Reset();
	inline TInt Setup(TClientBuffer*& aBufOut, TRequestStatus* aStatus, TAny* aDesPtr);
	inline TInt Setup(TClientBuffer*& aBufOut, TRequestStatus* aStatus, TLinAddr aStartAddr, TInt aLength, TBool aWriteable = EFalse);
	inline TBool IsReady();
private:
	struct SBufferData : public SDblQueLink
		{
		TClientBuffer iBuffer;
		TVirtualPinObject* iPinObject;
		};
	TClientBufferRequest(TUint aFlags);
	~TClientBufferRequest();	// call Close(), don't delete
	TInt Construct(TInt aMaxBuffers);
	void Close();
	TInt AllocateBufferData();
	SBufferData* StartAddBuffer();
	TInt EndAddBuffer(TClientBuffer*& aBufOut, SBufferData* aBuf);
	void QueueComplete(DThread* aThread, TInt aReason);
	static void CallbackFunc(TAny* aData, TUserModeCallbackReason aReason);
private:
	TUint iFlags;
	DThread* iSetupThread;
	SDblQue iBufferList;
	static NFastMutex Lock;
	friend class Kern;
	};

inline TInt TClientBufferRequest::Setup(TClientBuffer*& aBufOut, TRequestStatus* aStatus, TAny* aDesPtr)
	{
	TInt r = StartSetup(aStatus);
	if (r == KErrNone)
		r = AddBuffer(aBufOut, aDesPtr);
	if (r == KErrNone)
		EndSetup();
	return r;
	}

inline TInt TClientBufferRequest::Setup(TClientBuffer*& aBufOut, TRequestStatus* aStatus, TLinAddr aStartAddr, TInt aLength, TBool aWriteable)
	{
	TInt r = StartSetup(aStatus);
	if (r == KErrNone)
		r = AddBuffer(aBufOut, aStartAddr, aLength, aWriteable);
	if (r == KErrNone)
		EndSetup();
	return r;
	}

inline TBool TClientBufferRequest::IsReady()
	{
	return TClientRequest::IsReady();
	}

class DPagingDevice;
class DLogicalDevice;
class DPhysicalDevice;
class TShPoolCreateInfo;
class TKernelMapObject;

class Kern
/**
Kernel utility functions

@publishedPartner
@released
*/
	{
public:
	/**
	Bit values to be used with the aMode parameter of Kern::SetSytemTime() to
	specify the mode of operation of Kern::SetSytemTime().
	*/
	enum TTimeSetMode
		{
		ETimeSet_SetHwRtc = 1,		/**< Set HW as well as SW RTC */
		ETimeSet_LocalTime = 2,		/**< The Kern::SetSytemTime() parameter aTime is specified in local time rather than UTC */
		ETimeSet_SyncNotify = 4,	/**< Synchronously trigger change notifiers*/
		ETimeSet_AsyncNotify = 8,	/**< Asynchronously trigger change notifiers*/
		ETimeSet_Secure = 16		/**< Set the secure clock (implies ETimeSet_SetHwRtc)*/
		};
	/**
	Attributes that can be set on new kernel mapping objects created via Kern::CreateKernelMapObject().

	@see Kern::CreateKernelMapObject()
	*/
	enum TKernelMapAttributes
		{
		/**
		Set this flag to create a read only kernel mapping object.  When set DMA 
		operations to memory mapped by the mapping object must not write to the 
		memory mapped, i.e. the only DMA operations must be DMA copied into H/W.
		Setting this flag may improve the performance if the memory the 
		kernel mapping object maps is paged out.
		*/
		EKernelMap_ReadOnly = 1,
		EKernelMap_ValidMask = EKernelMap_ReadOnly,	/**<@internalComponent*/
		};
public:
	IMPORT_C static void Printf(const char* aFmt, ...);
	IMPORT_C static TInt AddHalEntry(TInt aId, THalFunc aFunc, TAny* aPtr);
	IMPORT_C static TInt AddHalEntry(TInt aId, THalFunc aFunc, TAny* aPtr, TInt aDeviceNumber);
	IMPORT_C static TInt RemoveHalEntry(TInt aId);
	IMPORT_C static TInt RemoveHalEntry(TInt aId, TInt aDeviceNumber);
	IMPORT_C static SHalEntry* FindHalEntry(TInt aId);
	IMPORT_C static SHalEntry* FindHalEntry(TInt aId, TInt aDeviceNumber);
	IMPORT_C static void SafeClose(DObject*& aObj, TAny* aPtr);
	IMPORT_C static TInt ValidateName(const TDesC& aName);
	IMPORT_C static TInt ValidateFullName(const TDesC& aName);
	IMPORT_C static TBool PowerGood();
	IMPORT_C static void NotifyChanges(TUint aChangesMask);
	IMPORT_C static void NotifyThreadDeath(DThread* aDeadThread);	/**< @internalComponent */
	IMPORT_C static void AsyncNotifyChanges(TUint aChangesMask);
	IMPORT_C static void InfoCopy(TDes8& aDest, const TDesC8& aSrc);
	IMPORT_C static void InfoCopy(TDes8& aDest, const TUint8* aPtr, TInt aLength);
	IMPORT_C static void RequestComplete(DThread* aThread, TRequestStatus*& aStatus, TInt aReason);
	IMPORT_C static void RequestComplete(TRequestStatus*& aStatus, TInt aReason);	
	IMPORT_C static void QueueRequestComplete(DThread* aThread, TClientRequest* aRequest, TInt aReason);
	IMPORT_C static TInt CreateClientRequest(TClientRequest*& aRequestPtr);
	
	template <class T> inline static TInt CreateClientDataRequest(TClientDataRequest<T>*& aRequestPtr)
		{
		TInt r = Kern::CreateClientDataRequestBase((TClientDataRequestBase*&)aRequestPtr, sizeof(T));
		if (r == KErrNone)
			new (aRequestPtr->Buffer()) T;
		return r;
		}

	template <class T1, class T2> inline static TInt CreateClientDataRequest2(TClientDataRequest2<T1,T2>*& aRequestPtr)
		{
		TInt r = Kern::CreateClientDataRequestBase2((TClientDataRequestBase2*&)aRequestPtr, sizeof(T1), sizeof(T2));
		if (r == KErrNone)
			{
			new (aRequestPtr->Buffer1()) T1;
			new (aRequestPtr->Buffer2()) T2;
			}
		return r;
		}
	
	IMPORT_C static void DestroyClientRequest(TClientRequest*& aRequestPtr);
	template <class T> inline static void DestroyClientRequest(TClientDataRequest<T>*& aRequestPtr)
		{ DestroyClientRequest((TClientRequest*&)aRequestPtr); } /**< @prototype */
	template <class T, class T2> inline static void DestroyClientRequest(TClientDataRequest2<T,T2>*& aRequestPtr)
		{ DestroyClientRequest((TClientRequest*&)aRequestPtr); } /**< @prototype */
	IMPORT_C static TInt CreateClientBufferRequest(TClientBufferRequest*& aRequestPtr, TUint aInitialBuffers, TUint aFlags);
	IMPORT_C static void DestroyClientBufferRequest(TClientBufferRequest*& aRequestPtr);
	IMPORT_C static void QueueBufferRequestComplete(DThread* aThread, TClientBufferRequest* aRequest, TInt aReason);
	IMPORT_C static TDfcQue* DfcQue0();
	IMPORT_C static TDfcQue* DfcQue1();
	IMPORT_C static TDfcQue* TimerDfcQ();
	IMPORT_C static TDfcQue* SvMsgQue();
	IMPORT_C static DObjectCon* const *Containers();	/**< @internalComponent */
	IMPORT_C static SDblQue* CodeSegList();
	IMPORT_C static DMutex* CodeSegLock();
	IMPORT_C static DPowerModel* PowerModel();
	IMPORT_C static TInt SetThreadPriority(TInt aPriority, DThread* aThread=NULL);
	IMPORT_C static void SetRealtimeState(TThreadRealtimeState aNewState);
	IMPORT_C static DObject* ObjectFromHandle(DThread* aThread, TInt aHandle, TInt aType);
	IMPORT_C static DObject* ObjectFromHandle(DThread* aThread, TInt aHandle, TInt aType, TUint& aAttr);
	IMPORT_C static DThread* ThreadFromId(TUint aId);
	IMPORT_C static DProcess* ProcessFromId(TUint aId);
	IMPORT_C static void ThreadSuspend(DThread& aThread, TInt aCount);
	IMPORT_C static void ThreadResume(DThread& aThread);
	IMPORT_C static TInt MutexWait(DMutex& aMutex);
	IMPORT_C static void MutexSignal(DMutex& aMutex);
	IMPORT_C static TInt MutexCreate(DMutex*& aMutex, const TDesC& aName, TUint aOrder);
	IMPORT_C static TInt SemaphoreWait(DSemaphore& aSem, TInt aNTicks=0);
	IMPORT_C static void SemaphoreSignal(DSemaphore& aSem);
	IMPORT_C static TInt SemaphoreCreate(DSemaphore*& aSem, const TDesC& aName, TInt aInitialCount);
	IMPORT_C static TInt ThreadCreate(SThreadCreateInfo& aInfo);
	IMPORT_C static TInt DfcQInit(TDfcQue* aDfcQ, TInt aPriority, const TDesC* aName=NULL);
	IMPORT_C static TInt DfcQCreate(TDfcQue*& aDfcQ, TInt aPriority, const TDesC* aName=NULL);
	IMPORT_C static TInt DynamicDfcQCreate(TDynamicDfcQue*& aDfcQ, TInt aPriority, const TDesC& aBaseName);
	IMPORT_C static TInt ProcessCreate(DProcess*& aProcess, TProcessCreateInfo& aInfo, HBuf*& aCommand, TInt* aHandle);	/**< @internalComponent */
	IMPORT_C static TSupplyStatus MachinePowerStatus();
	IMPORT_C static void AppendFormat(TDes8& aDes, const char* aFmt, VA_LIST aList);
	IMPORT_C static TSuperPage& SuperPage();
	IMPORT_C static TMachineConfig& MachineConfig();
	IMPORT_C static TUint32 Random();
	IMPORT_C static void RandomSalt(TUint32 aEntropyData);
	IMPORT_C static void RandomSalt(TUint32 aEntropyData, TUint aBitsOfEntropy);
	IMPORT_C static void RandomSalt(TUint64 aEntropyData, TUint aBitsOfEntropy);
	IMPORT_C static void RandomSalt(const TUint8* aEntropyData, TUint aEntropyDataLength, TUint aBitsOfEntropy);
	IMPORT_C static TInt SecureRandom(TDes8& aRandomValue);
    IMPORT_C static void WaitForRequest(TRequestStatus& aStatus);	/**< @internalTechnology */
    IMPORT_C static TAny* Alloc(TInt aSize);
    IMPORT_C static TAny* AllocZ(TInt aSize);
    IMPORT_C static void Free(TAny* aPtr);
    IMPORT_C static void AsyncFree(TAny* aPtr);
    IMPORT_C static TAny* ReAlloc(TAny* aPtr, TInt aSize, TInt aMode=0); 
	IMPORT_C static TInt SafeReAlloc(TAny*& aPtr, TInt aOldSize, TInt aNewSize);
	IMPORT_C static void ValidateHeap();
	IMPORT_C static void PanicCurrentThread(const TDesC& aCategory, TInt aReason);
    IMPORT_C static TBool QueryVersionSupported(const TVersion& aCurrent,const TVersion& aRequested);
	IMPORT_C static TTimeK SystemTimeSecure();
	IMPORT_C static TTimeK SystemTime();
	IMPORT_C static	TInt SetSystemTime(const TTimeK& aTime, TUint aMode);
	IMPORT_C static TInt HalFunction(TInt aGroup, TInt aFunction, TAny* a1, TAny* a2);
	IMPORT_C static TInt HalFunction(TInt aGroup, TInt aFunction, TAny* a1, TAny* a2, TInt aDeviceNumber);
	IMPORT_C static TInt AddEvent(const TRawEvent& aEvent);
	IMPORT_C static TInt AddEvent(const TRawEvent& aEvent, TBool aResetUserActivity);
	IMPORT_C static void Exit(TInt aReason);
	IMPORT_C static TInt TickPeriod();
	IMPORT_C static void KUDesGet(TDes8& aDest, const TDesC8& aSrc);
	IMPORT_C static void KUDesPut(TDes8& aDest, const TDesC8& aSrc);
	IMPORT_C static const TUint8* KUDesInfo(const TDesC8& aSrc, TInt& aLength, TInt& aMaxLength);
	IMPORT_C static void KUDesSetLength(TDes8& aDes, TInt aLength);
	IMPORT_C static TInt ThreadDesRead(DThread* aThread, const TAny* aSrc, TDes8& aDest, TInt aOffset, TInt aMode);
	IMPORT_C static TInt ThreadRawRead(DThread* aThread, const TAny* aSrc, TAny* aDest, TInt aSize);
	IMPORT_C static TInt ThreadDesWrite(DThread* aThread, TAny* aDest, const TDesC8& aSrc, TInt aOffset, TInt aMode, DThread* aOrigThread);
	IMPORT_C static TInt ThreadRawWrite(DThread* aThread, TAny* aDest, const TAny* aSrc, TInt aSize, DThread* aOrigThread=NULL);
	inline static TInt ThreadDesRead(DThread* aThread, const TAny* aSrc, TDes8& aDest, TInt aOffset);
	inline static TInt ThreadDesWrite(DThread* aThread, TAny* aDest, const TDesC8& aSrc, TInt aOffset, DThread* aOrigThread=NULL);
	IMPORT_C static TInt ThreadBufRead(DThread* aThread, const TClientBuffer* aSrc, TDes8& aDest, TInt aOffset, TInt aMode);
	IMPORT_C static TInt ThreadBufWrite(DThread* aThread, TClientBuffer* aDest, const TDesC8& aSrc, TInt aOffset, TInt aMode, DThread* aOrigThread);
	IMPORT_C static TInt ThreadGetDesLength(DThread* aThread, const TAny* aDes);
	IMPORT_C static TInt ThreadGetDesMaxLength(DThread* aThread, const TAny* aDes);
	IMPORT_C static TInt ThreadGetDesInfo(DThread* aThread, const TAny* aDes, TInt& aLength, TInt& aMaxLength, TUint8*& aPtr, TBool aWriteable);
	IMPORT_C static void ThreadKill(DThread* aThread, TExitType aType, TInt aReason, const TDesC& aCategory);
	IMPORT_C static TAny* KUSafeRead(const TAny* aSrc, TAny* aDest, TInt aSize);	/**< @internalTechnology */
	IMPORT_C static TAny* SafeRead(const TAny* aSrc, TAny* aDest, TInt aSize);		/**< @internalTechnology */
	IMPORT_C static TAny* KUSafeWrite(TAny* aDest, const TAny* aSrc, TInt aSize);	/**< @internalTechnology */
	IMPORT_C static TAny* SafeWrite(TAny* aDest, const TAny* aSrc, TInt aSize);		/**< @internalTechnology */
	IMPORT_C static TInt KUSafeInc(TInt& aValue);		/**< @internalTechnology */
	IMPORT_C static TInt KUSafeDec(TInt& aValue);		/**< @internalTechnology */
	IMPORT_C static DThread& CurrentThread();
	IMPORT_C static DProcess& CurrentProcess();
	IMPORT_C static TThreadMessage& Message();
	IMPORT_C static TInt FreeRamInBytes();
	IMPORT_C static void Fault(const char* aCat, TInt aFault);
	IMPORT_C static TBool ColdStart();
	IMPORT_C static void NanoWait(TUint32 aInterval);
	IMPORT_C static TUint32 TickCount();
	IMPORT_C static TInt PollingWait(TPollFunction aFunction, TAny* aPtr, TInt aPollPeriodMs, TInt aMaxPoll);
	IMPORT_C static TUint32 RoundToPageSize(TUint32 aSize);
	IMPORT_C static TUint32 RoundToChunkSize(TUint32 aSize);
	IMPORT_C static void Restart(TInt aMode);			/**< @internalTechnology */
	IMPORT_C static void AccessCode();
	IMPORT_C static void EndAccessCode();
	IMPORT_C static	DThread* NThreadToDThread(NThread* aNThread);
	IMPORT_C static	TInt PrepareMemoryForDMA(DThread* aThread, TAny* aAddress, TInt aSize, TPhysAddr* aPageList); /**< @internalComponent */
	IMPORT_C static	TInt ReleaseMemoryFromDMA(DThread* aThread, TAny* aAddress, TInt aSize, TPhysAddr* aPageList);/**< @internalComponent */
	
#ifndef __REMOVE_PLATSEC_DIAGNOSTIC_STRINGS__
	/**
	Checks whether the process that owns the current thread has the specified capability.

	When a check fails the action taken is determined by the system wide Platform Security
	configuration. If PlatSecDiagnostics is ON, then a diagnostic message is emitted.
	If PlatSecEnforcement is OFF, then this function will return True even though the
	check failed.

	@param aCapability The capability to be tested.
	@param aDiagnosticText	A string that will be emitted along with any diagnostic message
	                   		that may be issued if the test finds the capability is not present.
					   		This string must be enclosed in the __PLATSEC_DIAGNOSTIC_STRING macro
					   		which enables it to be easily removed from the system.

	@return True if the current thread's process has the capability, False otherwise.
	*/
	inline static TBool CurrentThreadHasCapability(TCapability aCapability, const char* aDiagnosticText=0)
		{ return DoCurrentThreadHasCapability(aCapability, aDiagnosticText); }
#else //__REMOVE_PLATSEC_DIAGNOSTIC_STRINGS__
	// Only available to NULL arguments
	/**
	Checks whether the process that owns the current thread has the specified capability.

	When a check fails the action taken is determined by the system wide Platform Security
	configuration. If PlatSecDiagnostics is ON, then a diagnostic message is emitted.
	If PlatSecEnforcement is OFF, then this function will return True even though the
	check failed.

	@param aCapability The capability to be tested.

	@return True if the current thread's process has the capability, False otherwise.
	*/
	inline static TBool CurrentThreadHasCapability(TCapability aCapability, OnlyCreateWithNull /*aDiagnosticText*/=NULL)
		{ return DoCurrentThreadHasCapability(aCapability); }
#endif // !__REMOVE_PLATSEC_DIAGNOSTIC_STRINGS__

	IMPORT_C static TVendorId ThreadVendorId(DThread* aThread);
	IMPORT_C static TVendorId ProcessVendorId(DProcess* aProcess);
	IMPORT_C static TSecureId ThreadSecureId(DThread* aThread);
	IMPORT_C static TSecureId ProcessSecureId(DProcess* aProcess);

	IMPORT_C static DCodeSeg* CodeSegFromAddress(TLinAddr aAddr, DProcess* aProcess);
	IMPORT_C static void CodeSegGetMemoryInfo(DCodeSeg& aCodeSeg, TModuleMemoryInfo& aInfo, DProcess* aProcess);
	IMPORT_C static TInt MakeHandleAndOpen(DThread* aThread, DObject* aObject);
	IMPORT_C static TInt MakeHandleAndOpen(DThread* aThread, DObject* aObject, TOwnerType aType);
	IMPORT_C static TInt CloseHandle(DThread* aThread, TInt aHandle);
	IMPORT_C static TInt ChunkCreate(const TChunkCreateInfo& aInfo, DChunk*& aChunk, TLinAddr& aKernAddr, TUint32& iMapAttr);
	IMPORT_C static TInt ChunkCommit(DChunk* aChunk, TInt aOffset, TInt aSize);
	IMPORT_C static TInt ChunkCommitContiguous(DChunk* aChunk, TInt aOffset, TInt aSize, TUint32& aPhysicalAddress);
	IMPORT_C static TInt ChunkCommitPhysical(DChunk* aChunk, TInt aOffset, TInt aSize, TUint32 aPhysicalAddress);
	IMPORT_C static TInt ChunkCommitPhysical(DChunk* aChunk, TInt aOffset, TInt aSize, const TUint32* aPhysicalAddressList);
	IMPORT_C static TInt ChunkClose(DChunk* aChunk);
	IMPORT_C static DChunk* OpenSharedChunk(DThread* aThread, const TAny* aAddress, TBool aWrite, TInt& aOffset);
	IMPORT_C static DChunk* OpenSharedChunk(DThread* aThread, TInt aChunkHandle, TBool aWrite);
	IMPORT_C static TInt ChunkAddress(DChunk* aChunk, TInt aOffset, TInt aSize, TLinAddr& aKernelAddress);
	IMPORT_C static TInt ChunkPhysicalAddress(DChunk* aChunk, TInt aOffset, TInt aSize, TLinAddr& aKernelAddress, TUint32& aMapAttr, TUint32& aPhysicalAddress, TUint32* aPhysicalPageList=NULL);
	IMPORT_C static TUint8* ChunkUserBase(DChunk* aChunk, DThread* aThread);

	/**
	Enumeration indicating the behaviour of text trace messages.
	@publishedPartner
	@released
	*/
	enum TTextTraceMode
		{
		/** Traces are sent to the serial port if not already handled by other means. */
		ESerialOutDefault = 0, 
		/** Traces are never sent to the serial port. */
		ESerialOutNever = 1,
		/** Traces are always sent to the serial port. */
		ESerialOutAlways = 2,
		/** Mask for serial port mode values. */
		ESerialOutMask = 3
		};
	IMPORT_C static TUint SetTextTraceMode(TUint aMode, TUint aMask);
	IMPORT_C static TTraceHandler SetTraceHandler(TTraceHandler aHandler);
	inline static TNanoWaitHandler SetNanoWaitHandler(TNanoWaitHandler aHandler);
	inline static TInitialTimeHandler SetInitialTimeHandler(TInitialTimeHandler aHandler);
	
	/**
	Install the specified paging device.
	@param aDevice The device.
	@return KErrNone or standard error code.
	@post The devices DPagingDevice::iDeviceId has been set.
	@internalTechnology
	*/
	IMPORT_C static TInt InstallPagingDevice(DPagingDevice* aDevice);
	IMPORT_C static TInt InstallLogicalDevice(DLogicalDevice* aDevice);		/**< @internalTechnology */
	IMPORT_C static TInt InstallPhysicalDevice(DPhysicalDevice* aDevice);		/**< @internalTechnology */

	IMPORT_C static TInt CreateVirtualPinObject(TVirtualPinObject*& aPinObject); // prototype
	IMPORT_C static TInt PinVirtualMemory(TVirtualPinObject* aPinObject, TLinAddr aStart, TUint aSize, DThread* aThread=NULL); // prototype
	IMPORT_C static TInt PinVirtualMemory(TVirtualPinObject* aPinObject, const TClientBuffer& aDes, DThread* aThread=NULL); // prototype
	IMPORT_C static TInt CreateAndPinVirtualMemory(TVirtualPinObject*& aPinObject, TLinAddr aStart, TUint aSize); //prototype
	IMPORT_C static void UnpinVirtualMemory(TVirtualPinObject* aPinObject); // prototype
	IMPORT_C static void DestroyVirtualPinObject(TVirtualPinObject*& aPinObject); // prototype

	IMPORT_C static TInt CreatePhysicalPinObject(TPhysicalPinObject*& aPinObject); // prototype
	IMPORT_C static TInt PinPhysicalMemory(TPhysicalPinObject* aPinObject, TLinAddr aStart, TUint aSize, TBool aReadOnly, TPhysAddr& aAddress, TPhysAddr* aPages, TUint32& aMapAttr, TUint& aColour, DThread* aThread=NULL);
	IMPORT_C static TInt UnpinPhysicalMemory(TPhysicalPinObject* aPinObject); // prototype
	IMPORT_C static TInt DestroyPhysicalPinObject(TPhysicalPinObject*& aPinObject); // prototype

	IMPORT_C static TInt CreateKernelMapObject(TKernelMapObject*& aMapObject, TUint aMaxReserveSize=0);
	IMPORT_C static TInt MapAndPinMemory(TKernelMapObject* aMapObject, DThread* aThread, TLinAddr aStart, TUint aSize, TUint aMapAttributes, TLinAddr& aKernelAddr, TPhysAddr* aPages=NULL);
	IMPORT_C static void UnmapAndUnpinMemory(TKernelMapObject* aMapObject);
	IMPORT_C static void DestroyKernelMapObject(TKernelMapObject*& aMapObject);

	IMPORT_C static TInt ShPoolCreate(TShPool*& aPool, TShPoolCreateInfo& aInfo, TBool aMap, TUint aFlags);
	IMPORT_C static TInt ShPoolOpen(TShPool*& aPool, DThread* aThread, TInt aHandle, TBool aMap, TUint aFlags);
	IMPORT_C static TInt ShPoolClose(TShPool* aPool);
	IMPORT_C static TInt ShPoolMakeHandleAndOpen(TShPool* aPool, DThread* aThread, TUint aAttr);
	IMPORT_C static TInt ShPoolSetBufferWindow(TShPool* aPool, TInt aWindowSize);
	IMPORT_C static TInt ShPoolAlloc(TShPool* aPool, TShBuf*& aBuf, TUint aFlags);
	IMPORT_C static void ShPoolGetInfo(TShPool* aPool, TShPoolInfo& aInfo);
	IMPORT_C static TUint ShPoolBufSize(TShPool* aPool);
	IMPORT_C static TUint ShPoolFreeCount(TShPool* aPool);
	IMPORT_C static TInt ShBufOpen(TShBuf*& aBuf, DThread* aThread, TInt aHandle);
	IMPORT_C static TInt ShBufClose(TShBuf* aBuf);
	IMPORT_C static TInt ShBufMakeHandleAndOpen(TShBuf* aBuf, DThread* aThread);
	IMPORT_C static void ShBufCopyFrom(TShBuf* aBuf, const TDesC8& aSrc, TUint aOffset);
	IMPORT_C static TUint8* ShBufPtr(TShBuf* aBuf);
	IMPORT_C static TUint ShBufSize(TShBuf* aBuf);
	IMPORT_C static TInt ShBufPin(TShBuf* aBuf, TPhysicalPinObject* aPinObject, TBool aReadOnly, TPhysAddr& Address, TPhysAddr* aPages, TUint32& aMapAttr, TUint& aColour);


private:
	IMPORT_C static TBool DoCurrentThreadHasCapability(TCapability aCapability, const char* aDiagnosticText);
	IMPORT_C static TBool DoCurrentThreadHasCapability(TCapability aCapability);
	IMPORT_C static TKernelHookFn SetHook(TKernelHookType aType, TKernelHookFn aFunction, TBool aAllowOveride=EFalse);
	IMPORT_C static TInt CreateClientDataRequestBase(TClientDataRequestBase*& aRequestPtr, TInt aSize);					/**< @internalTechnology */
	IMPORT_C static TInt CreateClientDataRequestBase2(TClientDataRequestBase2*& aRequestPtr, TInt aSize1, TInt aSize2);	/**< @internalTechnology */
	friend class DThread;
	};



/**
Reads an 8-bit descriptor from the specified thread's address space, enforcing
checks on validity of source and destination if necessary.
	
It is used especially by device drivers to transfer data from a user thread.
aDes might be accessed with user permission validation.

@param aThread The thread from whose address space data is to be read.
@param aSrc Pointer to a source descriptor in the specified thread's address
            space. This must not be NULL.
@param aDest Target descriptor
@param aOffset Offset in aSrc from where to start reading data.

@return KErrNone, if sucessful; KErrBadDescriptor, if aPtr or aDes is an
        invalid decriptor; KErrArgument if anOffset is negative; KErrDied if aThread
        is dead.
*/
inline TInt Kern::ThreadDesRead(DThread* aThread, const TAny* aSrc, TDes8& aDest, TInt aOffset)
	{ return ThreadDesRead(aThread,aSrc,aDest,aOffset,KChunkShiftBy0); }




/**
Writes an 8-bit descriptor to the specified thread's address space, enforcing
checks on validity of source and destination if necessary.
	
It is used especially by device drivers to transfer data to a user thread.
aDes might be accessed with user permission validation.
	  
@param aThread The thread into whose address space data is to be written.
@param aDest Pointer to a target descriptor in the specified thread's address
            space. It must not be NULL
@param aSrc Source descriptor
@param aOffset Offset in aDest to start writing data to.
@param aOrigThread The thread on behalf of which this operation is performed (eg client of device driver).

@return KErrNone, if sucessful; KErrBadDescriptor, if aPtr or aDes is an
        invalid decriptor; KErrArgument if anOffset is negative; KErrDied if aThread
        is dead.
*/
inline TInt Kern::ThreadDesWrite(DThread* aThread, TAny* aDest, const TDesC8& aSrc, TInt aOffset, DThread* aOrigThread)
	{ return ThreadDesWrite(aThread,aDest,aSrc,aOffset,KChunkShiftBy0,aOrigThread); }


/**
Register the function used to implement Kern::NanoWait.

This should be called from the variant to supply an accurate implementation for Kern::NanoWait.

@return The previous implemention.

@see Kern::NanoWait

@publishedPartner
@released
*/
inline TNanoWaitHandler Kern::SetNanoWaitHandler(TNanoWaitHandler aHandler)
	{
	return (TNanoWaitHandler) SetHook(EHookNanoWait, (TKernelHookFn)aHandler, ETrue);
	}



/**
Register the function used to read the initial system time.

This may be called from the variant to override the default behaviour.  If so it
should be called in phase 1 of initialisation.

@return The previous implemention.

@publishedPartner
@released
*/
inline TInitialTimeHandler Kern::SetInitialTimeHandler(TInitialTimeHandler aHandler)
	{
	return (TInitialTimeHandler) SetHook(EHookInitialTime, (TKernelHookFn)aHandler, ETrue);
	}



struct SMiscNotifierQ;
class TMiscNotifierMgr;
class DObject : public DBase
/**
Base class for reference-counted kernel side objects.

@publishedPartner
@released
*/
// NOTE: Any changes to the structure of this class should be duplicated in class DMonObject
	{
public:

    /**
    Defines a set of bit values that are returned from calls
    to DObject::Close(), or by an overriding implementation of Close() provided
    by a derived class.
    
    The values describe the state of the object on return from Close().
    */
	enum TCloseReturn
		{
		
		/**
		If set, indicates that the object has been deleted.  
		*/
		EObjectDeleted=1,
		
		/**
		If set, indicates that the last reference
		from a user process has been removed.
		
		Note that this only applies to DLibrary objects.
		*/
		EObjectUnmapped=2,
		};
		
	/**	@internalComponent */
	enum TObjectProtection
		{
		ELocal=0,		// Private
		EProtected,
		EGlobal,		// Public
		};
		
	/**	@internalComponent */
	enum TObjectFlags
		{
		EObjectExtraReference = (1<<0),
		EObjectDeferKernelCodesegCleanup = (1<<1)
		};
		
public:
	/**	@internalComponent */
	inline TInt Inc() { return __e32_atomic_tas_ord32(&iAccessCount, 1, 1, 0); }

	/**	@internalComponent */
	inline TInt Dec() { return __e32_atomic_tas_ord32(&iAccessCount, 1, -1, 0); }

	IMPORT_C DObject();
	IMPORT_C ~DObject();
	
	/**
	Opens this kernel side reference-counted object.
	
	Opening a reference-counted object increments its reference count
	by one. The increment operation is done atomically.
	
	@return KErrNone, if the increment operation succeeds;
	        KErrGeneral, if the reference count value is either zero
	        or negative before performing the increment operation;
	        the reference count is initialised to 1 on construction
	        of the object, and can never be less than 1.
	*/
	inline TInt Open() { return(Inc()?KErrNone:KErrGeneral); }
	IMPORT_C void CheckedOpen();
	IMPORT_C virtual TInt Close(TAny* aPtr);
	IMPORT_C virtual TInt RequestUserHandle(DThread* aThread, TOwnerType aType);
	IMPORT_C virtual TInt RequestUserHandle(DThread* aThread, TOwnerType aType, TUint aAttr);
	IMPORT_C virtual TInt AddToProcess(DProcess* aProcess);
	IMPORT_C virtual TInt AddToProcess(DProcess* aProcess, TUint aAttr);
	IMPORT_C TInt AsyncClose();
	IMPORT_C virtual void DoAppendName(TDes& aName);
	IMPORT_C void DoAppendFullName(TDes& aFullName);
	IMPORT_C void Name(TDes& aName);
	IMPORT_C void AppendName(TDes& aName);
	IMPORT_C void FullName(TDes& aFullName);
	IMPORT_C void AppendFullName(TDes& aFullName);
	IMPORT_C TInt SetName(const TDesC* aName);
	IMPORT_C TInt SetOwner(DObject* aOwner);
	IMPORT_C void TraceAppendName(TDes8& aName, TBool aLock);
	IMPORT_C void TraceAppendFullName(TDes8& aFullName, TBool aLock);
	inline DObject* Owner();
	inline TInt AccessCount();
	inline TInt UniqueID();
	inline HBuf* NameBuf();
	inline void SetProtection(TObjectProtection aProtection);
	inline TUint Protection();
	void BaseName(TDes& aName);
	inline TUint64 ObjectId() const;		/**< @internalComponent */
public:
	/**	@internalComponent */
	TInt iAccessCount;			// must be first

	/**	@internalComponent */
	DObject* iOwner;

	/** A TObjectType value with one added
		@internalComponent */
	TUint8 iContainerID;

	/** A TObjectProtection value
		@internalComponent */
	TUint8 iProtection;

	/**	Bit field made of values from TObjectFlags
		@internalComponent */
	TUint8 iObjectFlags;

	/**	@internalComponent */
	TUint8 iNotQLow;

	/**	@internalComponent */
	HBuf* iName;

	/** A unique ID.
	    @internalComponent */
	TUint64 iObjectId;

	/**	@internalComponent */
//	SMiscNotifierQ* iNotifierQ;

public:
	static NFastMutex Lock;
private:
	inline SMiscNotifierQ* NotifierQ() const;	/**< @internalComponent */
	inline void SetNotifierQ(SMiscNotifierQ*);	/**< @internalComponent */
	inline TBool HasNotifierQ() const;			/**< @internalComponent */
private:
	static TUint64 NextObjectId;

	friend class DObjectCon;
	friend class RObjectIx;
	friend class Monitor;
	friend class Debugger;
	friend void Kern::AppendFormat(TDes8&, const char*, VA_LIST);
	friend class K;
	friend struct SMiscNotifierQ;
	friend class TMiscNotifierMgr;
	};

/**
Gets the number of open references to
this reference-counted kernel side object.

@return The number of open references.
*/
inline TInt DObject::AccessCount()
	{ return iAccessCount; }
	
/**
Gets a pointer to the kernel-side reference counted object
that owns this kernel side reference-counted object.

@return A pointer to the owning reference-counted object.
        This is NULL, if there is no owner.
*/	
inline DObject* DObject::Owner()
	{ return iOwner; }

/** @internalComponent */
inline HBuf* DObject::NameBuf()
	{ return iName; }

/** @internalComponent */
inline TInt DObject::UniqueID()
	{return(iContainerID);}

/** @internalComponent */
inline TUint DObject::Protection()
	{return (TUint8)iProtection;}

/** @internalComponent */
inline void DObject::SetProtection(TObjectProtection aProtection)
	{iProtection=(TUint8)aProtection;}

/********************************************
 * Device driver base classes
 ********************************************/

/**
@publishedPartner
@released

If set, it indicates that the use of units is valid.

@see DLogicalDevice::iParsemask
*/
const TUint KDeviceAllowUnit=0x01;




/**
@publishedPartner
@released

If set, then an LDD requires a PDD.

@see DLogicalDevice::iParsemask
*/
const TUint KDeviceAllowPhysicalDevice=0x02;




/**
@publishedPartner
@released

If set, it indicates that the use of additional information is allowed.

@see DLogicalDevice::iParsemask
*/
const TUint KDeviceAllowInfo=0x04;




/**
@publishedPartner
@released

Combines all of: KDeviceAllowUnit, KDeviceAllowPhysicalDevice and KDeviceAllowInfo.

@see DLogicalDevice::iParsemask
*/
const TUint KDeviceAllowAll=(KDeviceAllowUnit|KDeviceAllowPhysicalDevice|KDeviceAllowInfo);




/**
@publishedPartner
@released

A flag bit that can set in the aMode parameter passed to the
Kern::ThreadDesWrite(DThread* ,TAny*, const TDesC8&, TInt, TInt aMode, DThread*);
variant.

It ensures that the length of data copied to the target descriptor is
truncated, if necesary, to prevent the maximum length of that target descriptor
from being exceeded.

@see Kern::ThreadDesWrite()
*/
const TInt KTruncateToMaxLength=(TInt)0x40000000;




/**
@publishedPartner
@released

Used internally by Symbian OS.
*/
const TInt KCheckLocalAddress=(TInt)0x20000000;




/**
@internalTechnology
@prototype

A flag bit that can set in the aMode parameter passed to the
Kern::ThreadDesWrite(DThread* ,TAny*, const TDesC8&, TInt, TInt aMode, DThread*);
variant.

It indicates that the descriptor header should not be updated to reflect the new length.

@see Kern::ThreadDesWrite()
*/
const TInt KDoNotUpdateDesLength=(TInt)0x10000000;

class DLogicalDevice;
class DPhysicalDevice;




/**
@publishedPartner
@released

The abstract base class for a logical channel.
*/
class DLogicalChannelBase : public DObject
	{
public:
	IMPORT_C virtual ~DLogicalChannelBase();
public:
    /**
    Handles a client request in the client context.
    
    The function is called from within the kernel, but the implementation
    must be provided by external code.
    */
	virtual TInt Request(TInt aReqNo, TAny* a1, TAny* a2)=0;
	IMPORT_C virtual TInt DoCreate(TInt aUnit, const TDesC8* aInfo, const TVersion& aVer);
public:
	DLogicalDevice* iDevice;
	DPhysicalDevice* iPhysicalDevice;
	DBase* iPdd;
	};




/**
@publishedPartner
@released

The abstract base class for an LDD factory object.
*/
class DLogicalDevice : public DObject
	{
public:
	IMPORT_C virtual ~DLogicalDevice();
	IMPORT_C virtual TBool QueryVersionSupported(const TVersion& aVer) const;
	IMPORT_C virtual TBool IsAvailable(TInt aUnit, const TDesC* aDriver, const TDesC8* aInfo) const;
	TInt ChannelCreate(DLogicalChannelBase*& pC, TChannelCreateInfo& aInfo);
	TInt FindPhysicalDevice(DLogicalChannelBase* aChannel, TChannelCreateInfo& aInfo);


	/**
	Second stage constructor for derived objects.
	This must at least set a name for the driver object.

	@return KErrNone or standard error code.
	*/
	virtual TInt Install()=0;


	/**
	Gets the driver's capabilities.
	
	This is called in the response to an RDevice::GetCaps() request.

	@param aDes A user-side descriptor into which capabilities information is to be wriiten.
	*/
	virtual void GetCaps(TDes8& aDes) const =0;


	/**
	Called by the kernel's device driver framework to create a Logical Channel.
	This is called in the context of the user thread (client) which requested the creation of a Logical Channel
	(e.g. through a call to RBusLogicalChannel::DoCreate).
	The thread is in a critical section.

	@param aChannel Set to point to the created Logical Channel

	@return KErrNone or standard error code.
	*/
	virtual TInt Create(DLogicalChannelBase*& aChannel)=0;


public:
    /**
    The version of this factory object.

    This is used to check that an LDD and PDD are compatible.
    Typically, this is set by the constructor of a derived class.
    */
	TVersion iVersion;
	
	
	/**
	A bitmask that indicates device properties.

    This can take the following values:
    KDeviceAllowUnit
    KDeviceAllowPhysicalDevice
    KDeviceAllowInfo
    KDeviceAllowAll
    
    Typically, this is set by the constructor of a derived class.

    @see RBusLogicalChannel::DoCreate()
    @see KDeviceAllowUnit
    @see KDeviceAllowPhysicalDevice
    @see KDeviceAllowInfo
    @see KDeviceAllowAll
	*/
	TUint iParseMask;
	
	
	/**
	Indicates which units are valid.

    If units are allowed, i.e. the KDeviceAllowUnit bit is set in iParseMask,
    then this mask indicates which of the units (from 0 to 31) are valid.

    The DPhysicalDevice object associated with the PDD has a similar mask,
    and both masks are used to indicate which units the LDD-PDD pair
    can handle.

    Typically, this is set by the constructor of a derived class.
    
    @see KDeviceAllowUnit
	*/
	TUint iUnitsMask;
	
	
	/**
	Pointer to the DCodeSeg object which contains the executable code
	for this LDD.
	*/
	DCodeSeg* iCodeSeg;
	
	
	/**
	Number of DLogicalChannelBase objects currently in existence which
	have been created from this LDD.
	*/
	TInt iOpenChannels;
	};

/** @internalComponent */
typedef DLogicalDevice* (*TLogicalDeviceNew)();




/**
@publishedPartner
@released

The abstract base class for a PDD factory object.
*/
class DPhysicalDevice : public DObject
	{
public:
	enum TInfoFunction
		{
		EPriority=0,
		
		/** if implemented (i.e. Info(EMediaDriverPersistent) returns KErrNone) implies the media driver 
		created by this DPhysicalDevice will not be unloaded when the peripheral bus is powered down */
		EMediaDriverPersistent=1,
		};
public:
	IMPORT_C virtual ~DPhysicalDevice();
	IMPORT_C virtual TBool QueryVersionSupported(const TVersion& aVer) const;
	IMPORT_C virtual TBool IsAvailable(TInt aUnit, const TDesC8* aInfo) const;

	/**
	Second stage constructor for derived objects.
	This must at least set a name for the driver object.

	@return KErrNone or standard error code.
	*/
	virtual TInt Install() =0;

	/**
	Returns the drivers capabilities. This is not used by the Symbian OS device driver framework
	but may be useful for the LDD to use.

	@param aDes Descriptor to write capabilities information into
	*/
	virtual void GetCaps(TDes8& aDes) const =0;

	/**
	Called by the kernel's device driver framework to create a Physical Channel.
	This is called in the context of the user thread (client) which requested the creation of a Logical Channel
	(E.g. through a call to RBusLogicalChannel::DoCreate)
	The thread is in a critical section.

	@param aChannel Set to point to the created Physical Channel
	@param aUnit The unit argument supplied by the client to RBusLogicalChannel::DoCreate
	@param aInfo The info argument supplied by the client to RBusLogicalChannel::DoCreate
	@param aVer The version number of the Logical Channel which will use this Physical Channel 

	@return KErrNone or standard error code.
	*/
	virtual TInt Create(DBase*& aChannel, TInt aUnit, const TDesC8* aInfo, const TVersion& aVer) =0;

	/**
	Called by the kernel's device driver framework to check if this PDD is suitable for use with a Logical Channel.
	This is called in the context of the user thread (client) which requested the creation of a Logical Channel
	(E.g. through a call to RBusLogicalChannel::DoCreate)
	The thread is in a critical section.

	@param aUnit The unit argument supplied by the client to RBusLogicalChannel::DoCreate
	@param aInfo The info argument supplied by the client to RBusLogicalChannel::DoCreate
	@param aVer The version number of the Logical Channel which will use this Physical Channel 

	@return KErrNone or standard error code.
	*/
	virtual TInt Validate(TInt aUnit, const TDesC8* aInfo, const TVersion& aVer) =0;
	IMPORT_C virtual TInt Info(TInt aFunction, TAny* a1);
public:
	TVersion iVersion;
	TUint iUnitsMask;
	DCodeSeg* iCodeSeg;
	};




/** @internalComponent */
typedef DPhysicalDevice* (*TPhysicalDeviceNew)();

// Utility stuff for PDD finding

/** @internalTechnology */
struct SPhysicalDeviceEntry
	{
	TInt iPriority;
	DPhysicalDevice* iPhysicalDevice;
	};

/** @internalTechnology */
class RPhysicalDeviceArray : public RArray<SPhysicalDeviceEntry>
	{
public:
	IMPORT_C RPhysicalDeviceArray();
	IMPORT_C void Close();
	IMPORT_C TInt GetDriverList(const TDesC& aMatch, TInt aUnit, const TDesC8* aInfo, const TVersion& aVersion);
	};

/********************************************
 * Kernel-side messages and message queues.
 ********************************************/
class TMessageQue;




/**
@publishedPartner
@released

The base class for a kernel side message.

This is a means of communication between Symbian OS threads
executing kernel-side code.
*/
class TMessageBase : public SDblQueLink
	{
public:
	/** @internalComponent */
	enum TState {EFree,EDelivered,EAccepted};
public:
	TMessageBase() : iState(EFree), iQueue(NULL) {}
	IMPORT_C void Send(TMessageQue* aQ);
	IMPORT_C TInt SendReceive(TMessageQue* aQ);
	IMPORT_C void Forward(TMessageQue* aQ, TBool aReceiveNext);
	IMPORT_C void Complete(TInt aResult, TBool aReceiveNext);
	IMPORT_C void Cancel();			/**< @internalComponent */
	IMPORT_C void PanicClient(const TDesC& aCategory, TInt aReason);
public:
	IMPORT_C DThread* Client();
public:
	TUint8 iState;					/**< @internalComponent */
	TMessageQue* iQueue;			/**< @internalComponent */
	NFastSemaphore iSem;			/**< @internalComponent */
	TInt iValue;					/**< @internalComponent */ // msg id/return code
	};




/**
@publishedPartner
@released

A queue for kernel-side messages.

Objects of this type consist of a DFC plus a doubly-linked list
of received messages.
*/
class TMessageQue : private TDfc
	{
public:
	IMPORT_C TMessageQue(TDfcFn aFunction, TAny* aPtr, TDfcQue* aDfcQ, TInt aPriority);
	IMPORT_C void Receive();
	IMPORT_C TMessageBase* Poll();
	IMPORT_C TMessageBase* Last();
	IMPORT_C void CompleteAll(TInt aResult);
	using TDfc::SetDfcQ;
public:
	inline static void Lock() {NKern::FMWait(&MsgLock);}			/**< @internalComponent */
	inline static void Unlock() {NKern::FMSignal(&MsgLock);}		/**< @internalComponent */
	inline void UnlockAndKick() {Enque(&MsgLock);}					/**< @internalComponent */
public:
	SDblQue iQ;						/**< @internalComponent */
	TBool iReady;					/**< @internalComponent */
	TMessageBase* iMessage;			/**< @internalComponent */
	static NFastMutex MsgLock;		/**< @internalComponent */
	friend class TMessageBase;
	};




/**
@publishedPartner
@released
	
Synchronous kernel-side messages.
	
There is one per thread, and the thread always blocks while the message is outstanding.
The iPtr field of a DFC points to the message.
*/
class TThreadMessage : public TMessageBase
	{
public:
    /**
    Returns argument 0 as an integer.
    */
	inline TInt Int0() const {return (TInt)iArg[0];}
	
    /**
    Returns argument 1 as an integer.
    */
	inline TInt Int1() const {return (TInt)iArg[1];}
	
    /**
    Returns argument 2 as an integer.
    */
	inline TInt Int2() const {return (TInt)iArg[2];}
	
    /**
    Returns argument 3 as an integer.
    */
	inline TInt Int3() const {return (TInt)iArg[3];}
	
    /**
    Returns argument 0 as a pointer type.
    */
	inline TAny* Ptr0() const {return iArg[0];}
	
    /**
    Returns argument 1 as a pointer type.
    */
	inline TAny* Ptr1() const {return iArg[1];}
	
    /**
    Returns argument 2 as a pointer type.
    */
	inline TAny* Ptr2() const {return iArg[2];}
	
    /**
    Returns argument 3 as a pointer type.
    */
	inline TAny* Ptr3() const {return iArg[3];}
public:
    /**
    Message arguments.
    */
	TAny* iArg[10];				// message arguments
	};




/**
@publishedPartner
@released
	
An abstract class for a logical channel that provides a framework in which
user-side client requests are executed in the context of
a single kernel-side thread.
*/
class DLogicalChannel : public DLogicalChannelBase
	{
public:
	enum {
	      /**
	      Defines the smallest request number that Request() accepts.
	      Smaller request numbers will raise a panic.
	      */
	      EMinRequestId=(TInt)0xc0000000,
	      
	      /**
	      The value of the close message sent to the device driver thread
	      by Close().
	      */
	      ECloseMsg=(TInt)0x80000000};
public:
	IMPORT_C DLogicalChannel();
	IMPORT_C virtual ~DLogicalChannel();
	IMPORT_C virtual TInt Close(TAny*);
	IMPORT_C virtual TInt Request(TInt aReqNo, TAny* a1, TAny* a2);
 	IMPORT_C virtual TInt SendMsg(TMessageBase* aMsg);

	/**
	Processes a message for this logical channel.
	This function is called in the context of a DFC thread.

	@param aMsg     The message to process.
	                The iValue member of this distinguishes the message type:
	                iValue==ECloseMsg, channel close message
	                iValue==KMaxTInt, a 'DoCancel' message
	                iValue>=0, a 'DoControl' message with function number equal to iValue
	                iValue<0, a 'DoRequest' message with function number equal to ~iValue
	*/
	IMPORT_C virtual void HandleMsg(TMessageBase* aMsg)=0;
	IMPORT_C void SetDfcQ(TDfcQue* aDfcQ);
public:
	static void MsgQFunc(TAny* aPtr);
public:
	TDfcQue* iDfcQ;
	TMessageQue iMsgQ;
	};




/********************************************
 * Timers based on the system tick
 ********************************************/

/**
@publishedPartner
@released

Defines the signature for a tick timer callback function.
*/
typedef void (*TTickCallBack)(TAny* aPtr);




/**
@publishedPartner
@released

Relative timer with the resolution of one Symbian OS tick (generally 1/64 second).

Tick timers are general purpose interval timers which are used where there
is no need for high resolution or great accuracy. They correspond to the
timing functions available to user side code on EKA1 (User::After,
RTimer::After, RTimer::Lock).

@see User::After()
@see RTimer::After()
@see RTimer::Lock()
*/
class TTickLink : public SDeltaQueLink
	{
public:
	IMPORT_C TTickLink();
	IMPORT_C void Periodic(TInt aPeriod, TTickCallBack aCallBack, TAny* aPtr);
	IMPORT_C void OneShot(TInt aTime, TTickCallBack aCallBack, TAny* aPtr);
	IMPORT_C void Lock(TInt aTicks, TTickCallBack aCallBack, TAny* aPtr);
	IMPORT_C void Cancel();
	void DoCancel();
	TInt GetNextLock(TTimerLockSpec aMark, TInt &aTickCount) const;
public:
	TInt iPeriod;				/**< @internalComponent */
	TAny *iPtr;					/**< @internalComponent */
	TTickCallBack iCallBack;	/**< @internalComponent */
	Int64 iLastLock;			/**< @internalComponent */
	};




/**
@publishedPartner
@released
	
Defines the signature for a second timer callback function.

@see TSecondLink
*/
typedef void (*TSecondCallBack)(TAny* aPtr);




/**
@publishedPartner
@released

Absolute timer with a resolution of 1 second.

Second timers are used when an event needs to occur at a specific date and
time of day, rather than after a specified interval. They have a resolution of
1 second.

They are typically used for user alarms, and, if necessary, will power up
the system at the expiry time.
*/
class TSecondLink : public SDblQueLink
	{
public:
	IMPORT_C TSecondLink();
	IMPORT_C TInt At(const TTimeK& aUTCTime, TSecondCallBack aCallBack, TAny* aPtr);
	IMPORT_C void Cancel();
public:
	Int64 iTime;				/**< @internalComponent */
	TAny* iPtr;					/**< @internalComponent */
	TSecondCallBack iCallBack;	/**< @internalComponent */
	};




/**
@publishedPartner
@released

Defines the signature for an inactivity timer callback function.

@see TInactivityLink
*/
typedef void (*TInactivityCallBack)(TAny*);




/**
@publishedPartner
@released

Inactivity timer.

Inactivity timers are used to detect a specified period of user inactivity,
for example to enable device auto power down or screen saver functionality.
*/
class TInactivityLink : public SDblQueLink
	{
public:
	IMPORT_C TInactivityLink();
	IMPORT_C TInt Start(TInt aSeconds, TInactivityCallBack aCallBack, TAny* aPtr);
	IMPORT_C void Cancel();
public:
	TUint32 iTime;					/**< @internalComponent */ // expiry time in ticks
	TAny* iPtr;						/**< @internalComponent */
	TInactivityCallBack iCallBack;	/**< @internalComponent */
	};

/********************************************
 * Internal RAM drive
 ********************************************/

/**
	@internalTechnology
*/
class TInternalRamDrive
	{
public:
	static TInt Create();
	IMPORT_C static TLinAddr Base();
	IMPORT_C static TInt Size();
	IMPORT_C static TInt MaxSize();
	IMPORT_C static TInt Adjust(TInt aNewSize);
	IMPORT_C static void Lock();
	IMPORT_C static void Unlock();
	IMPORT_C static void Wait();
	IMPORT_C static void Signal();
public:
	static DMutex* Mutex;
	friend class Monitor;
	};

/********************************************/

/** List of all supported events.

	By default, all events, except the explicitly mentioned ones, are sent only
	if the kernel is built with __DEBUGGER_SUPPORT__ defined.

	This event set may be extended in the future, so clients should handle
	unknown events gracefully.

	@see DKernelEventHandler
	@publishedPartner
	@released
*/
enum TKernelEvent
	{
	/**
	Event delivered when a user-side software exception occurs.
	This event is always sent.
	The following conditions are true:
		- a1 contains a TExcType instance.
		- The current thread is the thread taking the exception.
	*/
	EEventSwExc,

	/**
	Event delivered when a hardware exception occurs.
	This event is always sent.
	The following conditions are true:
		- a1 points to the CPU-specific structure stored on the 
		  stack where the processor state has been saved.  For ARM, 
		  the structure is TArmExcInfo.
		- The current thread is the thread taking the exception.
	On exit, use DKernelEventHandler::EExcHandled to indicate event
	handled.
	*/
	EEventHwExc,

	/**
	Event delivered when a process is created (i.e. during a call to 
	RProcess::Create or Kern::ProcessCreate()).
	The following conditions are true:
		- a1 points to the process being created.
		- a2 points to the creator thread (which may not be the current thread).
		  In some case the creator thread can not be reliably determined and a2
		  will be set to NULL.
		- The process being created is partly constructed (no threads and no chunks yet).
	*/
	EEventAddProcess,

	/**
	Event delivered after a process attribute change.  Currently this applies
	only to process renaming and address space change (chunk addition/removal)
	but may be extended in the future.
	The following conditions are true:
		- a1 points to the process being modified.
		- The process lock may be held
	*/
	EEventUpdateProcess,

	/**
	Event delivered when a process terminates.
	The following conditions are true:
		- a1 points to the process being terminated.
		- The current thread is the kernel server thread.
		- The process is partly destructed so its resources should be accessed
		  only after checking they still exist.
	*/
	EEventRemoveProcess,

	/**
	Event delivered when a user or kernel thread is created (i.e. during a call
	to RProcess::Create(), RThread::Create() or Kern::ThreadCreate()).  
	The following conditions are true:
		- a1 points to the thread being created.
		- a2 points to the creator thread (which may not be the current thread).
		- The thread being created is fully constructed but has not executed any code yet.
	*/
	EEventAddThread,

	/**
	Event delivered when a user or kernel thread is scheduled for the first time.
	The following conditions are true:
		- a1 points to the thread being scheduled.
		- The current thread is the thread being scheduled.
		- The thread has not executed any code yet.
	*/
	EEventStartThread,

	/**
	Event delivered after a thread attribute change.  Currently this applies
	only to thread renaming but may be extended in the future.
	a1 points to the thread being modified.
	*/
	EEventUpdateThread,

	/**
	Event delivered when a user or kernel thread terminates.
	This event is always sent.
	The following conditions are true:
		- a1 points to the thread being terminated.
		- The current thread is the thread being terminated.
		- The current thread is in the ECSExitInProgress state, 
		  and so can not be suspended.
		- The thread's address space can be inspected.
	*/
	EEventKillThread,

	/**
	Event delivered when a user or kernel thread is about to be closed.
	The following conditions are true:
		- a1 points to the thread being terminated.
		- The current thread is the supervisor thread.
		- The thread is partly destructed so its resources should be accessed
		  only after checking if they still exist.
	 */
	EEventRemoveThread,

	/**
	Event delivered when a chunk is created.
	The following conditions are true:
		- a1 points to the chunk being created.
	*/
	EEventNewChunk,

	/**
	Event delivered when physical memory is committed to/decommitted from a
	chunk.
	The following conditions are true:
		- a1 points to the chunk being modified.
	*/
	EEventUpdateChunk,

	/**
	Event delivered when a chunk is deleted.
	The following conditions are true:
		- a1 points to the chunk being deleted.
	*/
	EEventDeleteChunk,

	/**
	Event delivered when a user-side DLL is explicitly loaded.
	The following conditions are true:
		- a1 points to the DLibrary instance being loaded.
		- a2 points to the creator thread.
		- DLibrary::iMapCount is equals to 1 if the DLL is loaded for the first 
		  time into the creator thread's address space.
		- If the DLL is loaded for the first time, its global c'tors (if any) 
		  haven't been called yet.
		- The DLL and all its dependencies have been mapped.
		- The system-wide mutex DCodeSeg::CodeSegLock is held.
	*/
	EEventAddLibrary,

	/**
	Event delivered when a previously explicitly loaded user-side DLL is 
	closed or unloaded (i.e. call to RLibrary::Close()).
	The following conditions are true:
		- a1 points to the DLibrary instance being unloaded.
		- DLibrary::iMapCount is equals to 0 if the DLL is about to be unloaded.
		- If the DLL is about to be unloaded, its global d'tors have been called
		  but it is still mapped (and so are its dependencies).
		- The current thread may be the kernel server.
		- The system-wide mutex DCodeSeg::CodeSegLock is held.
	*/
	EEventRemoveLibrary,

	/**
	Event delivered when a LDD is loaded.  
	The following conditions are true:
		- a1 points to the LDD's code segment (DCodeSeg instance).
		- The current thread is the loader thread.
		- The LDD factory function has not been called yet.
	*/
	EEventLoadLdd,

	/** 
	Event delivered when a LDD is unloaded.
	The following conditions are true:
		- a1 points to the LDD's code segment (DCodeSeg instance).
		- The current thread is the loader thread.
	*/
	EEventUnloadLdd,

	/**
	Event delivered when a PDD is loaded.
	The following conditions are true:
		- a1 points to the PDD's code segment (DCodeSeg instance).
		- The current thread is the loader thread.
		- The PDD factory function has not been called yet.
	*/
	EEventLoadPdd,

	/** 
	Event delivered when a PDD is unloaded.
	The following conditions are true:
		- a1 points to the PDD's code segment (DCodeSeg instance).
		- The current thread is the loader thread.
	*/
	EEventUnloadPdd,

	/** 
	Event delivered when RDebug::Print is called in user-side code.
	The following conditions are true:
		- a1 points to the user-side buffer containing the unicode string 
		  for printing.  The characters can not be accessed directly. The
		  string must copied instead using kumemget() and, as the event
		  is delivered in thread critical section, the call to kumemget()
		  must be protected with XTRAP.
		- a2 is a TInt which holds the length in characters of the string 
		  for printing.  As this is a unicode string, the size of the 
		  user-side buffer is twice the length.
		- The current thread is the user-side client.
	On exit use DKernelEventHandler::ETraceHandled to indicate event handled.
	*/
	EEventUserTrace,
	
	/**
	Event delivered when a code segment is mapped into a process.
	The following conditions are true:
		- a1 points to the code segment.
		- a2 points to the process.
		- The system-wide mutex DCodeSeg::CodeSegLock is held.
	*/
	EEventAddCodeSeg,

	/**
	Event delivered when a code segment is unmapped from a process.
	The following conditions are true:
		- a1 points to the code segment.
		- a2 points to the process.
		- The system-wide mutex DCodeSeg::CodeSegLock is held.
	*/
	EEventRemoveCodeSeg,
	
	/**
	Event delivered when a process is created (i.e. during a call to 
	RProcess::Create or Kern::ProcessCreate()).
	The following conditions are true:
		- a1 points to the process.
		- The process being created is fully constructed.
	*/
	EEventLoadedProcess,

	/**
	Event delivered when a process is being released, before its code segment,
	stack chunk, etc. are unmapped.
	The following conditions are true:
		- a1 points to the process.
		- The process being released is fully constructed.
	*/
	EEventUnloadingProcess,

	/**
	Must be last
	*/
	EEventLimit
	};

/** Access-counted event handler callback.

	The kernel maintains a queue of event handlers which are called in turn 
	when various events (thread creation, hardware exception, ...) occur.

	Typical clients would be drivers or kernel extensions implementing kernel
	debug agents or profilers.  This class could also be used to extend the
	instruction set (by trapping undefined instructions).

	An access count needs to be maintained because handlers can not be dequeued
	and destructed while one or more threads are executing some code inside the
	callback.  The access count, initially set to 1, is incremented every time
	a thread enters the callback and decremented every time a thread exits the
	callback.  
	
	Consequently: 

	- Client code should use Close() instead the operator delete to destroy the
	  event handler.  

	- If the client is a LDD, the lifetime of the event handler may exceed that
	of the logical channel and its user-side client thread.  Therefore, the
	callback should not refer to data stored in the channel and any
	asynchronous request to be completed in the callback should be cancelled by
	the channel destructor.

	If the client is a RAM-loaded LDD (or PDD), it is possible for the DLL to
	be unloaded while the handler is still in use.  This would result in an
	exception.  To avoid this, the handler must open a reference to the
	DLogicalDevice (or DPhysicalDevice) and close it in its d'tor.

	@publishedPartner
	@released
*/
class DKernelEventHandler : public DBase
	{
public:
	/** Values used to select where to insert the handler in the queue */
	enum TAddPolicy
		{
		EAppend, /**< Append at end of queue */
		};

	/** Bitmask returned by callback function */
	enum TReturnCode
		{
		/** Run next handler if set, ignore remaining handlers if cleared */
		ERunNext = 1,
		/** Available for EEventUserTrace only.  Ignore trace statement if set. */
		ETraceHandled = 0x40000000,
		/** Available for hardware exceptions only.  Do not panic thread if set. */
		EExcHandled = (TInt)0x80000000,
		};

	/** Pointer to C callback function called when an event occurs.
		aEvent designates what event is dispatched.  a1 and a2 are event-specific.  
		aPrivateData is specified when the handler is created, typically a pointer
		to the event handler.  
		The function is always called in thread critical section. 
		@see TReturnCode
	 */
	typedef TUint (*TCallback)(TKernelEvent aEvent, TAny* a1, TAny* a2, TAny* aPrivateData);
public:
	// external interface
	IMPORT_C static TBool DebugSupportEnabled();
	IMPORT_C DKernelEventHandler(TCallback aCb, TAny* aPrivateData);
	IMPORT_C TInt Add(TAddPolicy aPolicy = EAppend);
	IMPORT_C TInt Close();
	inline TBool IsQueued() const;
public:
	// kernel internal interface
	static TUint Dispatch(TKernelEvent aEvent, TAny* a1, TAny* a2);
private:
	static SDblQue HandlersQ;
private:
	TInt iAccessCount;
	SDblQueLink iLink;
	TCallback iCb;
	TAny* iPrivateData;
	};

/** Returns whether or not the handler has been queued. */
inline TBool DKernelEventHandler::IsQueued() const
	{
	return iLink.iNext != NULL;
	}


/********************************************
 * Persistent Machine Configuration
 ********************************************/

/**
	@internalTechnology
*/
class TMachineConfig
	{
public:
	TInt iLogSize;
	TInt iLogMaxSize;
	SLocaleData iLocale;
	TInt iXtalError;

	/** The last time set by the user */
	TTimeK LastSetTime;

	/** The accumulated RTC correction due to crystal errors */
	TInt TheRTCCorrection;
	};

/******************************************************************************
 * Macros for typical extensions/device drivers/ASSPs
 ******************************************************************************/
#ifndef __WINS__
/**
@internalComponent
@prototype 9.5
	
Defines the entry point for an extension that specifies a priority
Priorities are used to sequence calls to the entry point (KModuleEntryReasonExtensionInit1)
and may be used to ensure that extensions with higher priority are started before
ones with lower priority. When the priority of 2 extensions is the same, the order
they are started is the order they appear in the ROM header extension list.
The maximum priority is 255. Priority 0 is the lowest priority (standard extension).
@note This feature is not available for the emulator.
*/
#define	DECLARE_EXTENSION_WITH_PRIORITY(priority)							\
	GLREF_C TInt InitExtension();											\
	TInt KernelModuleEntry(TInt aReason)									\
		{																	\
		if (aReason==KModuleEntryReasonExtensionInit0)						\
			return priority;												\
		if (aReason!=KModuleEntryReasonExtensionInit1)						\
			return KErrArgument;											\
		return InitExtension();												\
		}																	\
	GLDEF_C TInt InitExtension()


/**
@publishedPartner
@released
	
Defines the entry point for a standard extension	
*/
#define	DECLARE_STANDARD_EXTENSION() DECLARE_EXTENSION_WITH_PRIORITY(KExtensionStandardPriority)

#else 
/**
@publishedPartner
@released
	
Defines the entry point for a standard extension	
*/
#define	DECLARE_STANDARD_EXTENSION()										\
	GLREF_C TInt InitExtension();											\
	TInt KernelModuleEntry(TInt aReason)									\
		{																	\
		if (aReason==KModuleEntryReasonExtensionInit0)						\
			return KErrNone;												\
		if (aReason!=KModuleEntryReasonExtensionInit1)						\
			return KErrArgument;											\
		return InitExtension();												\
		}																	\
	GLDEF_C TInt InitExtension()

#endif

/**
@publishedPartner
@released

Defines the entry point for a standard logical device driver (LDD),
and declares the ordinal 1 export function for creating the LDD factory object	
*/
#define	DECLARE_STANDARD_LDD()												\
	TInt KernelModuleEntry(TInt)											\
		{ return KErrNone; }												\
	EXPORT_C DLogicalDevice* CreateLogicalDevice()




/**
@publishedPartner
@released

Defines the entry point for a standard physical device driver (PDD),
and declares the ordinal 1 export function for creating the PDD factory object.
*/
#define	DECLARE_STANDARD_PDD()												\
	TInt KernelModuleEntry(TInt)											\
		{ return KErrNone; }												\
	EXPORT_C DPhysicalDevice* CreatePhysicalDevice()




/**
@publishedPartner
@released

Defines the entry point for a standard logical device driver (LDD)
that is also an extension, and declares the ordinal 1 export function
for creating the LDD factory object	
*/
#define	DECLARE_EXTENSION_LDD()												\
	EXPORT_C DLogicalDevice* CreateLogicalDevice()




/**
@publishedPartner
@released
	
Defines the entry point for a standard physical device driver (PDD)
that is also an extension, and declares the ordinal 1 export function
for creating the PDD factory object	
*/
#define	DECLARE_EXTENSION_PDD()												\
	EXPORT_C DPhysicalDevice* CreatePhysicalDevice()




/**
@publishedPartner
@released

Generates a stub entry point function and the AsicInitialise() function that 
causes static constructors to be called.

ASSP DLLs require static constructors to be called at the same time as
the variant is initialised.
*/
#define	DECLARE_STANDARD_ASSP()												\
	extern "C" { GLREF_C TInt _E32Dll(TInt); }								\
	GLDEF_C TInt KernelModuleEntry(TInt aReason)							\
		{ return (aReason==KModuleEntryReasonExtensionInit1)				\
						?KErrNone:KErrGeneral; }							\
	EXPORT_C void AsicInitialise()											\
		{ _E32Dll(KModuleEntryReasonExtensionInit1); }


/**
@internalComponent
*/
#define	__VARIANT_SUPPORTS_ADDITIONAL_INTERFACE_BLOCK__(x)					\
	__Variant_Flags__ |= (x)

/**
@internalTechnology
@prototype
Used to indicate that a variant supports the nanokernel interface block
*/
#define	__VARIANT_SUPPORTS_NANOKERNEL_INTERFACE_BLOCK__()					\
			__VARIANT_SUPPORTS_ADDITIONAL_INTERFACE_BLOCK__(1)


class DemandPaging;

/**
Memory locking on demand paging systems.

This class is not thread safe. I.e. Only one method should be executed at any time
for a given instance of an object.

@internalTechnology
@prototype
*/
class DDemandPagingLock : public DBase
	{
public:
	IMPORT_C DDemandPagingLock();

	/**
	Unlock and Free the memory associated with this object
	*/
	inline ~DDemandPagingLock() { Free(); }

	/**
	Reserve memory so that this object can be used for locking up to
	the specified number of bytes.

	@param aSize Maximum number of bytes to be locked.

	@return KErrNone or standard error code.
	*/
	IMPORT_C TInt Alloc(TInt aSize);

	/**
	Unlock any memory locked by this object, then free the memory reserved with 
	Alloc(). This returns the object to the same state it was at immediately after 
	construction.
	*/
	IMPORT_C void Free();

	/**
	Ensure all pages in the given region are present and lock them so that they will 
	not be paged out. If the region contained no demand paged memory, then no action 
	is performed.

	This function may not be called again until the previous memory has been 
	unlocked.

	@param aThread The thread in whoes process the memory lies.
	@param aStart The address of the first byte of memory to be locked.
	@param aSize The number of bytes to lock.

	@return 1 (one) if any memory was locked. 0 (KErrNone) if memory
			did not need locking because it is not demand paged. Otherwise
			KErrBadDescriptor to indicate that the memory region was not
			valid.

	@pre Alloc must have been used to reserve sufficient RAM.
	*/
	IMPORT_C TInt Lock(DThread* aThread, TLinAddr aStart, TInt aSize);

	/**
	Unlock the memory region which was previousely locked with Lock.
	This may be called even if memory wasn't previousely locked.
	*/
	inline void Unlock() { if(iLockedPageCount) DoUnlock(); }

private:
	IMPORT_C void DoUnlock();
private:
	DemandPaging* iThePager;
	TInt iReservedPageCount;
	TInt iLockedPageCount;
	TLinAddr iLockedStart;
	DProcess* iProcess;
	TPhysAddr* iPages;
	TAny* iPinMapping;
	TInt iMaxPageCount;

	friend class DemandPaging;
	};



class DPagingRequestPool;

/**
Base class for Paging Devices.
A Paging Device provides the paging system with access to storage media which holds
data being demand paged.
@internalTechnology
@prototype
*/
class DPagingDevice : public DBase
	{
public:
	/** The type of device this represents. */
	enum TType
		{
		ERom  = 1<<0,			/**< Paged ROM device type. */
		ECode = 1<<1,			/**< Code paging device type. */
		EData = 1<<2,			/**< Data paging device type. */
		EMediaExtension = 1<<3	/**< Media extension device type. */
		};

	enum TSpecialDrives
		{
		EDriveRomPaging = -1,	/**< Special drive number to indicate rom paging. */
		EDriveDataPaging = -2,	/**< Special drive number to indicate data paging. */
		};

	/** Additional flags. */
	enum TFlags
		{
		ESupportsPhysicalAccess = 1<<0,  /**< Supports ReadPhysical and WritePhysical methods. */
		};

	/**
	Called by the paging system to read data from the media represented by this
	device.

	@param aBuffer The location where the read data should be stored.
	@param aOffset The offset from the media start, in read units, from where
				   data should be read.
	@param aSize   The number of read units to be read.
	
	@param aDrvNumber The drive number for code paging or a member of TSpecialDrives for rom or data
	paging.

	@return KErrNone or standard error code.
	*/
	virtual TInt Read(TThreadMessage* aReq,TLinAddr aBuffer,TUint aOffset,TUint aSize, TInt aDrvNumber) = 0;

	/**
	Called by the paging system to write data to the media represented by this device.

	This is only used in the implementation of data paging to write to the swap partition.

	@param aBuffer The location of the data to write.
	@param aOffset The offset from the media start, in read units, to where data should be written.
	@param aSize   The number of read units to write.
	@param aBackground  If set, this indicates that the request should not count as making the device busy.

	@return KErrNone or standard error code.
	*/
	inline virtual TInt Write(TThreadMessage* aReq,TLinAddr aBuffer,TUint aOffset,TUint aSize,TBool aBackground);

	/**
	Called by the paging system to notify the media driver that data that was previously written is
	now no longer needed.

	This is only used in the implementation of data paging when data in the swap partition is no
	longer needed.

	@param aOffset The offset from the media start, in read units, to where data was written.
	@param aSize   The size in read units.

	@return KErrNone or standard error code.
	*/
	inline virtual TInt DeleteNotify(TThreadMessage* aReq,TUint aOffset,TUint aSize);

	/**
	Return the lock that should be used to synchronise calculation of the idle/busy state and
	subsequent calls to #NotifyIdle and #NotifyBusy.
	*/
	IMPORT_C NFastMutex* NotificationLock();
	
	/**
	Called by the paging system to read data from the media represented by this device using
	physical addresses, if the device supports it.
	
	This is intended to allow reading pages from swap without needing to map them first, and should
	be implemented by media drivers that support DMA access.

	If this method is implemented, the ESupportsPhysicalAccess flag in iFlags must be set as well.

	The default implementation of this method just returns KErrNotSupported.

	@param aPageArray   Pointer to array of physical address to write to.
	@param aPageCount   Number of physical pages to read.
	@param aOffset      The offset from the media start, in read units, from where data should be read.
	
	@param aDrvNumber The drive number for code paging or a member of TSpecialDrives for rom or data
	paging.

	@return KErrNone or standard error code.
	*/
	inline virtual TInt ReadPhysical(TThreadMessage* aReq, TPhysAddr* aPageArray, TUint aPageCount, TUint aOffset, TInt aDrvNumber);

	/**
	Called by the paging system to write data to the media represented by this device using physical
	addresses, if the device supports it.

	This is only used in the implementation of data paging to write to the swap partition.

	This method takes a pointer to an array of physical addresses and the number of pages to write,
	in contrast to #Write which takes a logical address and a number of read units.

	This is intended to allow writing pages to swap without needing to map them first, and should be
	implelemented by media drivers that support DMA access.

	If this method is implemented, the ESupportsPhysicalAccess flag in iFlags must be set as well.

	The default implementation of this method just returns KErrNotSupported.

	@param aPageArray   Pointer to array of physical address to read from.
	@param aPageCount   Number of physical pages to write.
	@param aOffset      The offset from the media start, in read units, to where data should be
		   			    written.
	@param aBackground  If set, this indicates that the request should not count as making the 
		   				device busy.

	@return KErrNone or standard error code.
	*/
	inline virtual TInt WritePhysical(TThreadMessage* aReq, TPhysAddr* aPageArray, TUint aPageCount, TUint aOffset, TBool aBackground);
	
	/**
	Called by the paging device to notify the kernel that the device has just become idle and is not
	currently processing any requests.

	This is used by the kernel to work out when to clean pages in the background.

	Note that write requests made by calling the DPagingDevice::Write() with the aBackground flag
	set do not count towards the device being busy.
	*/
	IMPORT_C void NotifyIdle();

	/**
	Called by the paging device to notify the kernel that the device has just become busy and is
	currently processing requests.

	This is used by the kernel to work out when to clean pages in the background.

	Note that write requests made by calling the DPagingDevice::Write() with the aBackground flag
	set do not count towards the device being busy.
	*/
	IMPORT_C void NotifyBusy();
	
public:
	/** The type of device this represents. */
	TUint16 iType;

	/** Flags bitfield made up of members of TFlags. */
	TUint16 iFlags;

	/** The local drives supported for code paging.
	    This is a bitmask containing one bit set for each local drive supported, where the bit set
	    is 1 << the local drive number.  If this device does not support code paging, this should be
	    zero. */
	TUint32 iDrivesSupported;

	/** Zero terminated string representing the name of the device.
		(Only used for debug tracing purposes.) */
	const char* iName;

	/** Log2 of the read unit size. A read unit is the number of bytes which
		the device can optimally read from the underlying media.
		E.g. for small block NAND, a read unit would be equal to the page size,
		512 bytes, therefore iReadUnitShift would be set to 9. */
	TInt iReadUnitShift;

	/** The value the device should use to identify itself.
		This value is set by Kern::InstallPagingDevice().
		The purpose of this Id is to distinguish multiple Code Paging devices. */
	TInt iDeviceId;

	/** For data paging only, the size of the swap partition supported by this device, in read units.
		
		If data paging is not supported, or has been disabled by the media driver, this should be
		zero.
	*/
	TInt iSwapSize;

	/** The pool of DPagingRequest objects used to issue requests for this device.
		This is setup and used internally by the kernel.
	*/
	DPagingRequestPool* iRequestPool;

	/** Log2 of the media's preferred write size in bytes.

		E.g. if the preferred write size is 16KB, this should be set 14.
		
		Some media may exhibit poor performance unless writes happen in multiples of the superpage
		size.  In this case the media driver would set this field to indicate the write size, and
		the kernel will attempt to:
		
		 - write in multiples of the preferred size
		 - align writes to multiples of the preferred size

		Note that this value cannot be less the size of a page (eg 12) and there may be a maximum
		limit to what the kernel will write as well.
	*/
	TUint32 iPreferredWriteShift;

	/** Reserved for future use.
	*/
	TInt iSpare[3];
	};

inline TInt DPagingDevice::Write(TThreadMessage*, TLinAddr, TUint, TUint, TBool)
	{
	// Default implementation, may be overriden by derived classes
	return KErrNotSupported;
	}

inline TInt DPagingDevice::ReadPhysical(TThreadMessage*, TPhysAddr*, TUint, TUint, TInt)
	{
	// Default implementation, may be overriden by derived classes
	return KErrNotSupported;
	}
	
inline TInt DPagingDevice::WritePhysical(TThreadMessage*, TPhysAddr*, TUint, TUint, TBool)
	{
	// Default implementation, may be overriden by derived classes
	return KErrNotSupported;
	}

inline TInt DPagingDevice::DeleteNotify(TThreadMessage*,TUint,TUint)
	{
	// Default implementation, may be overriden by derived classes
	return KErrNotSupported;
	}

extern "C" { extern TInt __Variant_Flags__; }

/********************************************
 * Shareable Data Buffers
 ********************************************/
/**
   Pool create info

   @publishedPartner
   @prototype
*/
class TMappingAttributes2;

#include <e32shbufcmn.h>

class TShPoolCreateInfo
	{
public:
	/**
	Enumeration type to say that the shared pool is to be backed with memory supplied by the kernel.
	*/
	enum TShPoolPageAlignedBuffers
		{
		EPageAlignedBuffer = EShPoolPageAlignedBuffer,
		};

	/**
	Enumeration type to say that the shared pool is to be backed with memory supplied by the kernel.
	*/
	enum TShPoolNonPageAlignedBuffers
		{
		ENonPageAlignedBuffer = EShPoolNonPageAlignedBuffer,
		};

	/**
	Enumeration type to say that the shared pool is to be mapped onto device memory.
	*/
	enum TShPoolMemoryDevice
		{
		EDevice = EShPoolPhysicalMemoryPool,
		};

	IMPORT_C TShPoolCreateInfo(TShPoolPageAlignedBuffers aFlag, TUint aBufSize, TUint aInitialBufs);
	IMPORT_C TShPoolCreateInfo(TShPoolNonPageAlignedBuffers aFlag, TUint aBufSize, TUint aInitialBufs, TUint aAlignment);

	IMPORT_C TShPoolCreateInfo(TShPoolMemoryDevice aFlag, TUint aBufSize, TUint aInitialBufs, TUint aAlignment, TUint aPages, TPhysAddr* aPhysicalAddressList);

	IMPORT_C TShPoolCreateInfo(TShPoolMemoryDevice aFlag, TUint aBufSize, TUint aInitialBufs, TUint aAlignment, TUint aPages, TPhysAddr aPhysicalAddress);

	IMPORT_C TInt SetSizingAttributes(TUint aMaxBufs, TUint aMinFreeBufs, TUint aGrowByBufs, TUint aShrinkByBufs);
	IMPORT_C TInt SetExclusive();
	IMPORT_C TInt SetContiguous();
	IMPORT_C TInt SetGuardPages();

private:
	friend class ExecHandler;
	friend class DShPool;
	friend class DMemModelShPool;
	friend class DMemModelAlignedShPool;
	friend class DMemModelNonAlignedShPool;
	friend class DMemModelProcess;
	friend class DWin32ShPool;
	friend class DWin32AlignedShPool;
	friend class DWin32NonAlignedShPool;
	friend class DWin32Process;
	friend class DShBufTestDrvChannel;

	inline TShPoolCreateInfo()	///< default constructor
		{
		memclr(this, sizeof(TShPoolCreateInfo));
		};

	TShPoolInfo iInfo;

	union PhysAddr
		{
		TPhysAddr* iPhysAddrList;	///< physical addresses for hardware
		TPhysAddr  iPhysAddr;		///< physical address for hardware
		} iPhysAddr;

	TUint iPages;				///< number of pages to commit
};

#endif //__K32STD_H__
