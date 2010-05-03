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
// e32\include\memmodel\epoc\platform.h
// Public header file for device drivers
// 
// WARNING: This file contains some APIs which are internal and are subject
//          to change without notice. Such APIs should therefore not be used
//          outside the Kernel and Hardware Services package.
//

#ifndef __M32STD_H__
#define __M32STD_H__
#include <kernel/kernel.h>
#include <kernel/kernboot.h>
#ifdef __EPOC32__
#include <e32rom.h>
#else
class TRomHeader;
class TRomImageHeader;
class TRomEntry;
#endif
//

/********************************************
 * Hardware chunk abstraction
 ********************************************/

/**
The list of memory types (aka cache attributes) in Kernel on ARMv6K, ARMv7 and later platforms.
Types 0-3 can be used on all platforms. Types 4-7 can be used only on the platforms with memory type remapping.
@see TMappingAttributes2
@publishedPartner
@released
*/
enum TMemoryType
	{
	EMemAttStronglyOrdered 	= 0, /**< Strongly Ordered memory.*/
	EMemAttDevice 			= 1, /**< Device memory.*/
	EMemAttNormalUncached 	= 2, /**< Uncached Normal memory. Writes may combine.*/
	EMemAttNormalCached 	= 3, /**< Fully cached (Write-Back, Read/Write Allocate, Normal memory).*/
	EMemAttKernelInternal4 	= 4, /**< @internalComponent. Not to be used by device drivers.*/
	EMemAttPlatformSpecific5= 5, /**< Defined by Baseport - H/W independent.*/
	EMemAttPlatformSpecific6= 6, /**< Defined by Baseport - H/W specific - see ARM core's document for the details.*/
	EMemAttPlatformSpecific7= 7	 /**< Defined by Baseport - H/W independent.*/
};

const TUint KMemoryTypeShift = 3; /**< @internalComponent. The number of bits in a TMemoryType value.*/
const TUint KMemoryTypeMask = (1<<KMemoryTypeShift)-1;  /**< @internalComponent. Mask value for extracting a TMemoryType value from a bitfield.*/

/**
Memory mapping permissions and attributes.

@see TChunkCreateInfo
@see Kern::ChunkCreate
@see DSharedIoBuffer::New
@see DPlatChunkHw::New
@see Kern::ChunkPhysicalAddress
@see Cache::SyncMemoryBeforeDmaWrite
@see Cache::SyncMemoryBeforeDmaRead
@see Cache::SyncMemoryBeforeDmaWrite

@publishedPartner
@released
*/
enum TMappingAttributes
	{
	// access permissions for read
	EMapAttrReadNoone=0x0,		/**< Sets the memory as not readable in any mode */
	EMapAttrReadSup=0x1,		/**< Sets the memory as readable only from Kernel (Supervisor) mode */
	EMapAttrReadUser=0x4,		/**< Sets the memory as readable for user, hence it can be read in both user and supervisor mode*/
	EMapAttrReadMask=0xF,		/**< Used for masking read attributes*/

	// access permissions for write
	EMapAttrWriteNoone=0x00,	/**< Sets the memory as not writable in any mode */
	EMapAttrWriteSup=0x10,		/**< Sets the memory as writable only from Kernel (Supervisor) mode */
	EMapAttrWriteUser=0x40,		/**< Sets the memory as writable for user, hence it can be written in both user and supervisor mode*/
	EMapAttrWriteMask=0xF0,		/**< Used for masking write attributes*/

	// access permissions for execute
	EMapAttrExecNoone=0x000,	/**< Sets the memory as not executable in any mode */
	EMapAttrExecSup=0x100,		/**< Sets the memory as executable only from Kernel (Supervisor) mode */
	EMapAttrExecUser=0x400,		/**< Sets the memory as executable for user, hence it can be executed in both user and supervisor mode*/
	EMapAttrExecMask=0xF00,		/**< Used for masking execute attributes*/

	// access permissions - popular combinations
	EMapAttrSupRo=0x01,			/**< Supervisor has read only and user has no access permissions*/
	EMapAttrSupRw=0x11,			/**< Supervisor has read/write and user has no access permissions*/
	EMapAttrSupRwx=0x111,		/**< Supervisor has read/write/execute and user has no access permissions*/
	EMapAttrUserRo=0x14,		/**< Supervisor has read/write and user has read only permissions*/
	EMapAttrUserRw=0x44,		/**< Supervisor and user both have read/write permissions*/
	EMapAttrUserRwx=0x444,		/**< Supervisor and user both have read/write/execute permissions*/
	EMapAttrAccessMask=0xFFF,	/**< Used for masking access permissions attribute for popular combination */

	// Level 1 cache/buffer attributes
	EMapAttrFullyBlocking=0x0000,	/**< Level 1 cache/buffer attributes sets the memory as uncached, unbuffered (may not be L2 cached)*/
	EMapAttrBufferedNC=0x1000,		/**< Level 1 cache/buffer attributes sets the memory as uncached, buffered, writes do not coalesce (may not be L2 cached)*/
	EMapAttrBufferedC=0x2000,		/**< Level 1 cache/buffer attributes sets the memory as uncached, buffered, writes may coalesce (may not be L2 cached)*/
	EMapAttrL1Uncached=0x3000,		/**< Level 1 cache/buffer attributes sets the memory as uncached, buffered, writes may coalesce (may be L2 cached)*/
	EMapAttrCachedWTRA=0x4000,		/**< Level 1 cache/buffer attributes sets the memory as write-through cached, read allocate*/
	EMapAttrCachedWTWA=0x5000,		/**< Level 1 cache/buffer attributes sets the memory as write-through cached, read/write allocate*/
	EMapAttrCachedWBRA=0x6000,		/**< Level 1 cache/buffer attributes sets the memory as write-back cached, read allocate*/
	EMapAttrCachedWBWA=0x7000,		/**< Level 1 cache/buffer attributes sets memory as write-back cached, read/write allocate*/
	EMapAttrAltCacheWTRA=0x8000,	/**< Level 1 cache/buffer attributes sets memory as write-through cached, read allocate, use alternate cache*/
	EMapAttrAltCacheWTWA=0x9000,	/**< Level 1 cache/buffer attributes sets memory as write-through cached, read/write allocate, use alternate cache*/
	EMapAttrAltCacheWBRA=0xA000,	/**< Level 1 cache/buffer attributes sets memory as write-back cached, read allocate, use alternate cache*/
	EMapAttrAltCacheWBWA=0xB000,	/**< Level 1 cache/buffer attributes write-back cached, read/write allocate, use alternate cache*/
	EMapAttrL1CachedMax=0xF000,		/**< Used to make memory maximally cached in L1 cache*/
	EMapAttrL1CacheMask=0xF000,		/**< Used for masking L1 cache attributes*/

	// Level 2 cache attributes
	EMapAttrL2Uncached=0x00000,		/**< Level 2 cache attributes used to set memory as uncached at level 2 */
	EMapAttrL2CachedWTRA=0x40000,	/**< Level 2 cache attributes sets memory as write-through cached, read allocate*/
	EMapAttrL2CachedWTWA=0x50000,	/**< Level 2 cache attributes sets memory as write-through cached, read/write allocate*/
	EMapAttrL2CachedWBRA=0x60000,	/**< Level 2 cache attributes sets memory as write-back cached, read allocate*/
	EMapAttrL2CachedWBWA=0x70000,	/**< Level 2 cache attributes sets memory as write-back cached, read/write allocate*/
	EMapAttrL2CachedMax=0xF0000,	/**< Used to make memory maximally cached in L2 cache*/
	EMapAttrL2CacheMask=0xF0000,	/**< Used for masking L2 cache attributes*/

	// Others
	EMapAttrCachedMax=0xFF000,		/**< Used to set memory as maximally cached for system (fully cached in L1&L2 cache)*/
	EMapAttrShared=0x100000,		/**< Used to set the memory as shared with other processors*/
	EMapAttrUseECC=0x200000,		/**< Used for error correcting code*/
	};

/**
Container class for memory region's attributes.
It is intended for ARM platforms with memory type and access permission remapping
(arm11mpcore, arm1176, cortex_a8 and later), but could be used on any previous platform as well.

The object of this type can replace TMappingAttributes bit mask whereever it is in use. For example:
@code
	TChunkCreateInfo chunkInfo;
	...
	new (&chunkInfo.iMapAttr) TMappingAttributes2(EMemAttStronglyOrdered,EFalse,ETrue);
	r = Kern::ChunkCreate(chunkInfo, ...);
@endcode

@see TMemoryType
@see TMappingAttributes
@see TChunkCreateInfo
@see Kern::ChunkCreate
@see DSharedIoBuffer::New
@see DPlatChunkHw::New
@see Kern::ChunkPhysicalAddress
@see Cache::SyncMemoryBeforeDmaWrite
@see Cache::SyncMemoryBeforeDmaRead
@see Cache::SyncMemoryBeforeDmaWrite

@publishedPartner
@released
*/
class TMappingAttributes2
	{
public:
/**
Constructor.
Memory is always readable by Kernel. Other attributes are defined by input parameters, as follows:
@param aType 		Type (aka cache attributes) of the memory.
@param aUserAccess 	True if memory is also accessed from user code, false if it is only accessible from kernel.
@param aWritable 	True if memory is writable, false if this is read only memory.
@param aExecutable 	True if memory contains code or data, false if it only contains data.
					Default argument value is false.
@param aShared 		Shared attribute of the mapping:
					<0	Default value for the platform, e.g. Shareable for SMP, Unshareable for uni-processor.
					==0	Unshareable memory
					>0	Shareable memory
					To ensure future compatibility, use the value <0 except when absolutely neccessary.
					Default argument value is -1.
@param aParity 		Parity error attribute of the mapping:
					<0	Default value for the platform (which is off on all platforms so far).
					==0	Parity error doesn't generate external abort.
					>0	Parity error generates external abort.
					To ensure future compatibility, use the value <0 except when absolutely neccessary.
					Default argument value is -1.

@see TMemoryType
*/
	IMPORT_C TMappingAttributes2(TMemoryType 	aType       ,
								TBool 			aUserAccess ,
								TBool 			aWritable   ,
								TBool 			aExecutable = EFalse,
								TInt 			aShared     = -1,
								TInt 			aParity     = -1);
	
	TMappingAttributes2(TUint aMapAttr);/**< @internalComponent*/
	TMemoryType Type();	/**< @internalComponent @return Type of the memory (aka cache attributes).*/
	TBool UserAccess();	/**< @internalComponent @return True if memory can be accessed from user code.*/
	TBool Writable();	/**< @internalComponent @return True if memory can be written into, false if this is reaad only memory.*/
	TBool Executable();	/**< @internalComponent @return True if memory can contain code and data, false if it can only contain data.*/
	TBool Shared();	   	/**< @internalComponent @return True if memory is shared, false if not.*/
	TBool Parity();		/**< @internalComponent @return True if parity error generates external abort, false if not.*/
	TBool ObjectType2();/**< @internalComponent @return True if the object is TMappingAttributes2, false if it is TMappingAttributes bitmask.*/
private:
	static void Panic(TInt aPanic); /**< @internalComponent*/
private:
	TUint32 iAttributes; /**< @internalComponent*/
	};

/**
@internalComponent
*/
inline TBool ComparePermissions(TInt aActual, TInt aRequired)
	{
	return	((aActual&EMapAttrReadMask)>=(aRequired&EMapAttrReadMask)) &&
			((aActual&EMapAttrWriteMask)>=(aRequired&EMapAttrWriteMask)) &&
			((aActual&EMapAttrExecMask)>=(aRequired&EMapAttrExecMask));
	}


/** Hardware Chunk class
	Class representing a global mapping of I/O or global memory buffers

@publishedPartner
@released
*/
class DPlatChunkHw : public DObject
	{
public:
	IMPORT_C static TInt New(DPlatChunkHw*& aChunk, TPhysAddr anAddr, TInt aSize, TUint aAttribs);
	inline TLinAddr LinearAddress() {return iLinAddr;}
	inline TPhysAddr PhysicalAddress() {return iPhysAddr;}
public:
	/** @internalComponent */
	static TInt DoNew(DPlatChunkHw*& aChunk, TPhysAddr anAddr, TInt aSize, TUint aAttribs);
public:
	TPhysAddr iPhysAddr;			/**< @internalComponent */
	TLinAddr iLinAddr;				/**< @internalComponent */
	TInt iSize;						/**< @internalComponent */
	TUint iAttribs;					/**< @internalComponent */	// mapping attributes
	};

/********************************************
 * Exports from layer 2 or below of the kernel
 * which are not available to layer 1
 ********************************************/

/**
Specifies the operation performed by the TRamZoneCallback function.
@see TRamZoneCallback
@publishedPartner
@released 
*/
enum TRamZoneOp
	{
	/** Informs the variant that a specified RAM zone is not currently 
	being used and therefore it may be possible to save power by not refreshing 
	this zone or, if the rest of the its RAM IC's zones are also empty, powering 
	down the RAM IC.

	The TRamZoneCallback parameter aParam1 is the ID of the zone.
	The TRamZoneCallback parameter aParam2 is a pointer to const array of TUints
	that are the bit masks of the zones' power status.
	*/
	ERamZoneOp_PowerDown=0, 

	/** Informs the variant that a specified RAM zone is now required for use
	and therefore it must be ready.
	The variant should ensure the zone is refreshed, if required, and that the 
	RAM IC is powered and fully initialised.

	The TRamZoneCallback parameter aParam1 is the ID of the zone.
	The TRamZoneCallback parameter aParam2 is a pointer to const array of TUints
	that are the bit masks of the zones' power status.
	*/
	ERamZoneOp_PowerUp=1,

	/** Operation that informs the variant of the RAM zones that have been used
	during the initial stages of the boot process.  Any RAM zones that are not
	in use may be powered down or not refreshed to save power.
	This will be the first operation requested of the variant and it is only
	issued once.

	The TRamZoneCallback parameter aParam1 is unused by this operation.
	The TRamZoneCallback parameter aParam2 is a pointer to const array of TUints
	that are the bit masks of the zones' power status.
	*/
	ERamZoneOp_Init=2,
	};


/**
Call back function that is invoked by the kernel when its RAM allocator determines 
that an operation can be performed on a particular RAM zone.

@publishedPartner
@released

@param aOp Type of operation to perform; a value of TRamZoneOp
@param aParam1 A value whose use is defined by the TRamZoneOp to be performed
@param aParam2 A value whose use is defined by the TRamZoneOp to be performed 
The data pointed to by aParam2 is const and therefore should not be modified

@return KErrNone if successful, otherwise one of the system wide error codes

@see TRamZoneOp
*/
typedef TInt (*TRamZoneCallback) (TRamZoneOp aOp, TAny* aParam1, const TAny* aParam2);

/**
Holds the number of each page type within a RAM zone.

@see Epoc::GetRamZonePageCount()

@publishedPartner
@released
*/
struct SRamZonePageCount
	{
	TUint iFreePages;		/**< The number of free pages in the RAM zone*/
	TUint iUnknownPages;	/**< The number of unknown pages in the RAM zone*/
	TUint iFixedPages;		/**< The number of fixed pages in the RAM zone*/
	TUint iMovablePages;	/**< The number of movable pages in the RAM zone*/
	TUint iDiscardablePages;/**< The number of discardable pages in the RAM zone*/
	TUint iReserved[4];		/**<@internalComponent reserved for internal use only*/
	};

/**
@publishedPartner
@released
*/
class Epoc
	{
public:
	/**
	The types of RAM defragmentation operations.
	@internalComponent
	*/
	enum TRamDefragOp
		{
		ERamDefrag_DefragRam,
		ERamDefrag_EmptyRamZone,
		ERamDefrag_ClaimRamZone,
		};

	/**
	The type of page to move with Epoc::MovePhysicalPage().
	@internalComponent
	*/
	enum TRamDefragPageToMove
		{
		/** 
		Move the physical page aOld.
		*/
		ERamDefragPage_Physical,
		/** 
		Move the page table page that maps the linear address in the 
		current thread at aOld.
		*/
		ERamDefragPage_PageTable,
		/** 
		Move the page table info page of the page table that maps the linear 
		address in the current thread at aOld.
		*/
		ERamDefragPage_PageTableInfo,
		};


	IMPORT_C static void SetMonitorEntryPoint(TDfcFn aFunction);			/**< @internalComponent */
	IMPORT_C static void SetMonitorExceptionHandler(TLinAddr aHandler);		/**< @internalComponent */
	IMPORT_C static TAny* ExceptionInfo();									/**< @internalComponent */
	IMPORT_C static const TRomHeader& RomHeader();
	IMPORT_C static TInt AllocShadowPage(TLinAddr aRomAddr);
	IMPORT_C static TInt CopyToShadowMemory(TLinAddr aDest, TLinAddr aSrc, TUint32 aLength);
	IMPORT_C static TInt FreeShadowPage(TLinAddr aRomAddr);
	IMPORT_C static TInt FreezeShadowPage(TLinAddr aRomAddr);
	IMPORT_C static TInt AllocPhysicalRam(TInt aSize, TPhysAddr& aPhysAddr, TInt aAlign=0);
	IMPORT_C static TInt ZoneAllocPhysicalRam(TUint aZoneId, TInt aSize, TPhysAddr& aPhysAddr, TInt aAlign=0);
	IMPORT_C static TInt ZoneAllocPhysicalRam(TUint* aZoneIdList, TUint aZoneIdCount, TInt aSize, TPhysAddr& aPhysAddr, TInt aAlign=0);
	IMPORT_C static TInt AllocPhysicalRam(TInt aNumPages, TPhysAddr* aPageList);
	IMPORT_C static TInt ZoneAllocPhysicalRam(TUint aZoneId, TInt aNumPages, TPhysAddr* aPageList);
	IMPORT_C static TInt ZoneAllocPhysicalRam(TUint* aZoneIdList, TUint aZoneIdCount, TInt aNumPages, TPhysAddr* aPageList);
	IMPORT_C static TInt FreePhysicalRam(TPhysAddr aPhysAddr, TInt aSize);
	IMPORT_C static TInt FreePhysicalRam(TInt aNumPages, TPhysAddr* aPageList);
	IMPORT_C static TInt FreeRamZone(TUint aZoneId);
	IMPORT_C static TInt ClaimPhysicalRam(TPhysAddr aPhysAddr, TInt aSize);
	IMPORT_C static TPhysAddr LinearToPhysical(TLinAddr aLinAddr);
	IMPORT_C static void RomProcessInfo(TProcessCreateInfo& aInfo, const TRomImageHeader& aRomImageHeader);	/**< @internalComponent */
#ifdef BTRACE_KERNEL_MEMORY
	static TInt DriverAllocdPhysRam; // the number of bytes allocated by Epoc::AllocPhysicalRam and Epoc::FreePhysicalRam
	static TInt KernelMiscPages; // the number of bytes of 'miscelaneous' kernel memory allocated
#endif
	IMPORT_C static TInt MovePhysicalPage(TPhysAddr aOld, TPhysAddr& aNew, TRamDefragPageToMove aPageToMove=ERamDefragPage_Physical);	/**< @internalComponent */
	IMPORT_C static TInt SetRamZoneConfig(const SRamZone* aZones, TRamZoneCallback aCallback);
	IMPORT_C static TInt GetRamZonePageCount(TUint aId, SRamZonePageCount& aPageData);
	IMPORT_C static TInt ModifyRamZoneFlags(TUint aId, TUint aClearMask, TUint aSetMask);
	};

/**
@publishedPartner
@released
*/
class DebugSupport
	{
public:

	/** Bitmask values representing different breakpoint types. */
	enum TType
		{
		EBreakpointGlobal = 1<<0, /**< Breakpoint appears in all processes */
		EBreakpointLocal  = 1<<1, /**< Breakpoint appears in the specified process only. */
		};

	IMPORT_C static TInt InitialiseCodeModifier(TUint& aCapabilities, TInt aMinBreakpoints);
	IMPORT_C static void CloseCodeModifier();
	IMPORT_C static TInt ModifyCode(DThread* aThread, TLinAddr aAddress, TInt aSize, TUint aValue, TUint aType);
	IMPORT_C static TInt RestoreCode(DThread* aThread, TLinAddr aAddress);

/**
@internalTechnology
@prototype
*/
	IMPORT_C static void TerminateProcess(DProcess* aProcess, const TInt aReason);

	};

#ifdef __DEBUGGER_SUPPORT__
/**
@internalComponent
*/
class CodeModifier : public DBase
	{
public:

	/** Values for panic values in category 'CodeModifier'. */
	enum TPanic
		{
		EPanicNotInitialised = 0,
		EPanicInvalidSizeOrAlignment = 1,
		};

	/** Defines the type/size of the breakpoint - see TBreakpoint::iSize*/
	enum TBrkType
		{
		EEmpty =0,		//The slot is unused
		EByte =1,		//Jazelle breakpoint
		EHalfword =2,	//Thumb breakpoint
		EWord =4		//ARM breakpoint
		};

	TInt static CreateAndInitialise(TInt aMinBreakpoints);
	~CodeModifier();
	void Close();
	TInt Modify(DThread* aThread, TLinAddr aAddress, TInt aSize, TUint aValue);
	TInt Restore(DThread* aThread, TLinAddr aAddress);
	static void CodeSegRemoved(DCodeSeg* aCodeSeg, DProcess* aProcess);
	static DMutex& Mutex() {return *Kern::CodeSegLock();}
	static void Fault(TPanic aPanic);

private:
	
	/**Desribes a breakpoint slot in the pool*/
	struct TBreakpoint
		{
		TUint   iProcessId;	//Id of the process associated to this breakpoint.
		TUint   iAddress;	//The virtual address of the breakpoint
		TUint32 iOldValue;	//Will hold the original content of iAddress
		TInt16  iSize; 		//Could be one of TBrkType. 0 means empty/unused, otherwise it indicates the size of the breakpoint in bytes.
		TInt16  iPageIndex;	//If iSize!=0 identifies corresponding shadowed page, or -1 if it is non-XIP page.
		};
	
	/** Desribes a page slot in the pool. Used for pages that are shadowed or need to be locked (for demand paging). */
	struct TPageInfo
		{
		TLinAddr iAddress;		//Base address of the page.
		TInt32 	 iCounter;	  	//The number of breakpoints associated with this page. 0 indicates empty slot.
		TBool 	 iWasShadowed;	//True if the page had been already shadowed before the first breakpoint was applied,
								//false otherwise. If true, it won't be un-shadowed after all breakpoints are removed.
#ifdef __DEMAND_PAGING__
		/// If set, points to the deamnd paging lock object used to lock this page.  Only applies to
		/// RAM-loaded code.
		DDemandPagingLock* iPagingLock;
#endif
		};
private:
	TBreakpoint* FindBreakpoint(DThread* aThread, TLinAddr aAddress, TInt aSize, TBool& aOverlap);
	TBreakpoint* FindEmptyBrk();
	TInt FindEmptyPageInfo();
	TInt FindPageInfo(TLinAddr aAddress);
	TInt IsRom(TLinAddr aAddress);
	TInt WriteCode(TLinAddr aAddress, TInt aSize, TUint aValue, void* aOldValue);
	DProcess* Process(TUint aProcessId);
	TInt SafeWriteCode(DProcess* aProcess, TLinAddr aAddress, TInt aSize, TUint aValue, void* aOldValue);
	void RestorePage(TInt aPageIndex);
	void DoCodeSegRemoved(DCodeSeg* aCodeSeg, DProcess* aProcess);

private:
	TInt iPoolSize;
	TBreakpoint* iBreakpoints;	//Breakpoint pool with iPoolSize slots
	TPageInfo* iPages;			//The pool of the shadowed/locked pages with iPoolSize slots
	TUint iPageSize;
	TUint iPageMask;
	};

GLREF_D CodeModifier* TheCodeModifier;
#endif //__DEBUGGER_SUPPORT__


/**
@internalComponent
*/
inline const TRomEntry &RomEntry(TLinAddr anAddr)
	{return(*((const TRomEntry *)anAddr));}

/**
@internalComponent
*/
inline const TRomImageHeader& RomImageHeader(TLinAddr anAddr)
	{return(*((const TRomImageHeader*)anAddr));}

/**
TRamDefragRequest is intended to be used by device drivers to request that RAM defragmentation
operations are performed.

All RAM defragmentation operations can be invoked synchronously or asynchronously.
The asynchronous RAM defragmentation operations can use either a TDfc or a NFastSemaphore
to signal to the caller that the operation has completed.

@see TDfc
@see NFastSemaphore
@publishedPartner
@released
*/
class TRamDefragRequest : protected TAsyncRequest
	{
public:
	IMPORT_C TRamDefragRequest();
	IMPORT_C TInt DefragRam(TInt aPriority, TInt aMaxPages=0);
	IMPORT_C TInt DefragRam(NFastSemaphore* aSem, TInt aPriority, TInt aMaxPages=0);
	IMPORT_C TInt DefragRam(TDfc* aDfc, TInt aPriority, TInt aMaxPages=0);
	IMPORT_C TInt EmptyRamZone(TUint aId, TInt aPriority);
	IMPORT_C TInt EmptyRamZone(TUint aId, NFastSemaphore* aSem, TInt aPriority);
	IMPORT_C TInt EmptyRamZone(TUint aId, TDfc* aDfc, TInt aPriority);
	IMPORT_C TInt ClaimRamZone(TUint aId, TPhysAddr& aPhysAddr, TInt aPriority);
	IMPORT_C TInt ClaimRamZone(TUint aId, TPhysAddr& aPhysAddr, NFastSemaphore* aSem, TInt aPriority);
	IMPORT_C TInt ClaimRamZone(TUint aId, TPhysAddr& aPhysAddr, TDfc* aDfc, TInt aPriority);
	IMPORT_C TInt Result();
	IMPORT_C void Cancel();

	/** 
	Values that can be specified to control which thread priority 
	the RAM defragmentation operations are run with.
	*/
	enum TPrioritySpecial
		{
		/** 
		The RAM defragmentation operation will use the same thread priority as 
		that of the caller.
		*/
		KInheritPriority = -1,	
		};

private:
	void SetupPriority(TInt aPriority);

private:
	Epoc::TRamDefragOp iOp;
	TUint iId;
	TUint iMaxPages;
	TInt iThreadPriority;
	TPhysAddr* iPhysAddr;
	TInt iSpare[6];

public:
	friend class Defrag;
	};


#endif

