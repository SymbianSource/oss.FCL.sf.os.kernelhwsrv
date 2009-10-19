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
// e32\include\memmodel\epoc\mmubase\demand_paging.h
// 
//

#include <ramcache.h>
#include <memmodel.h>

#ifdef _DEBUG
#define __CONCURRENT_PAGING_INSTRUMENTATION__
#endif

class DDemandPagingLock;

/**
Maximum number of paging devices supported.
@internalComponent
*/
const TInt KMaxPagingDevices = 1 + KMaxLocalDrives;

/**
Multiplier for number of request objects in pool per drive that supports code paging.
@internalComponent
*/
const TInt KPagingRequestsPerDevice = 2;

/**
Maximum number of paging request objects supported.
@internalComponent
*/
const TInt KMaxPagingRequests = KMaxPagingDevices * KPagingRequestsPerDevice;

/**
Base class for the demand paging object.

The main functionality provided by this is:
- Management of the live page list.
- Interface to paging devices (media drivers).

The 'live page list' contains those pages which are currently present or 'paged in'.
This is a doubly linked list of SPageInfo objects which each represent a page of physical RAM.

The list is split into two parts; 'young' pages (iYoungList) which are marked as accessible
by the MMU and 'old' pages (iOldList) which are which are not marked as accessible.
The page at the head of iYoungList is the 'youngest' page, and that at the tail
of iOldList is the 'oldest' page.

This arrangement enables a pseudo Most Recently Used (MRU) algorithm to be implemented
where access to an old page will cause a data abort and the fault handler will then
move this page to the head of the 'young' list; the last young page will then be made
the first old page.

When a data abort occurs because of an access to a page which is not live, then
a new RAM page is obtained from the systems free pool, this is then filled with the
correct data and added to the start of the live page list. (It is made the 'youngest'
page.) If there are no RAM pages left in the systems free pool, or the live page list is
at its maximum size, (iMinimumPageCount), then the oldest page from the live page list
is recycled and used for the new page.

If the OS requests RAM pages from the systems free pool and there are not enough then
pages are removed from the live page list to satisfy this request - as long as this does
not make the live page list smaller than the limit specified by iMinimumPageCount.

@internalComponent
*/
class DemandPaging : public RamCacheBase
	{
public:
	/**
	Fault enumeration
	*/
	enum TFault
		{
		EInitialiseFailed = 0,		/**< Error occured during initialisation */
		EInitialiseBadArgs = 1,		/**< Arguments used during initialisation were bad */
		ERamPageLocked = 2,			/**< A page in the live page list was found to be locked */
		EUnexpectedPageType = 3,	/**< A page in the live page list had an unexpected type (SPageInfo::Attribs) */
		EPageInFailed = 4,			/**< An error occured whilst reading data for a 'page in' operation */
		ELockTwice = 5,				/**< DDemandPagingLock::Lock was used twice without an intervening DDemandPagingLock::Unlock. */
		ELockTooBig = 6,			/**< DDemandPagingLock::Lock was used with a size greater than that reserved with DDemandPagingLock::Alloc. */
		EInvalidPagingDevice = 7,	/**< Raised by InstallPagingDevice when the diven device is found to have invalid parameters */
		EDeviceAlreadyExists = 8,	/**< Atempt to install a paging device when one already exists for the ROM or the specified drive. */
		EDeviceMissing = 9,			/**< A Paging Fault occured and the device required to service it was found to be missing. */
		EPageFaultWhilstFMHeld = 10,/**< A Paging Fault occured whilst the current thread held a fast mutex. */
		EPageFreeContiguousPages = 11,/**< An error occured when finding pages to free to make contiguous memory blocks.*/
		};

	class DPagingRequest;

	//
	// Configuration and initialisation...
	//

	/**
	Tests whether rom paging has been requested.
	*/
	static TBool RomPagingRequested();

	/**
	Tests whether code paging has been requested.
	*/
	static TBool CodePagingRequested();

	static DemandPaging* New();
	
	DemandPaging();

	virtual ~DemandPaging();

	/**
	Intialisation called during MmuBase:Init2.
	*/
	virtual void Init2();

	/**
	Intialisation called from M::DemandPagingInit.
	*/
	virtual TInt Init3();

	//
	// Live list management...
	//

	/**
	Called by a thread whenever it takes an exception.

	If this function returns KErrNone then the thread will continue execution at the
	instruction which caused the exception. Any other return value will cause the normal
	thread exception handling to continue.

	The implementation of this function should determine if the exception was casued
	by access to pagable memory, if it wasn't then it should return KErrUnknown.

	Otherwise it should perform the actions necessary to make the memory accessible
	and return KErrNone. The implementation should also call the threads
	iPagingExcTrap->PagingException
	*/
	virtual TInt Fault(TAny* aExceptionInfo)=0;

	/**
	Make a page in the live page list not accessible. (By changing it's page table entries.)

	@pre System Lock held
	@post System Lock held (but may have been released by this function)
	*/
	virtual void SetOld(SPageInfo* aPageInfo)=0;

	/**
	Make a page in the live page list free to use for other purposes.
	I.e. Unmap it from all of the page tables which map it flush the cache.

	@pre RamAlloc mutex held
	@pre System Lock held
	@post System Lock held (but may have been released by this function)
	*/
	virtual void SetFree(SPageInfo* aPageInfo)=0;

	/**
	If the number of young pages excedes that specified by iYoungOldRatio then a
	single page is made 'old'. Call this after adding a new 'young' page.

	@pre System Lock held
	@post System Lock left unchanged.
	*/
	void BalanceAges();

	/**
	Add a page to the head of the live page list. I.e. make it the 'youngest' page.

	@pre System Lock held
	@post System Lock left unchanged.
	*/
	void AddAsYoungest(SPageInfo* aPageInfo);

	/**
	Mark a page as usused (EPagedFree) and add it to the end of the live page list.
	I.e. make it the 'oldest' page, so that it is the first page to be reused.

	@pre System Lock held
	@post System Lock left unchanged.
	*/
	void AddAsFreePage(SPageInfo* aPageInfo);

	/**
	Remove a page from live page list.
	It is set to the state EStatePagedDead.

	@pre System Lock held
	@post System Lock left unchanged.
	*/
	void RemovePage(SPageInfo* aPageInfo);

	/**
	Remove the oldest page from the live page list.
	The returned page is no longer mapped by any page table and is marked as unused.

	@pre System Lock held
	@post System Lock left unchanged.
	*/
	SPageInfo* GetOldestPage();

	/**
	Remove pages from the live page list and return them to the system's free pool. (Free them.)

	@param	aNumPages The number of pages to free up.
	@return	True if all pages could be freed, false otherwise
	@pre	RamAlloc mutex held.
	*/
	TBool GetFreePages(TInt aNumPages);

	/**
	Give a RAM cache page to the paging system for managing.
	This page of RAM may be reused for any purpose.
	If the page has already been donated then no action is taken.

	@param aPageInfo The page info for the donated page.

	@see ReclaimRamCachePage.

	@pre System Lock held
	@post System Lock left unchanged.
	*/
	void DonateRamCachePage(SPageInfo* aPageInfo);

	/**
	Attempt to reclaim a RAM cache page given to the paging system with #DonateRamCachePage.
	If the RAM page has not been reused for other purposes then the page is
	removed from the paging system's management.
	If the page has not previousely been donated then no action is taken.

	@param aPageInfo The page info for the page to reclaim.

	@return True if page successfully reclaimed, false otherwise.

	@pre System Lock held
	@post System Lock left unchanged.
	*/
	TBool ReclaimRamCachePage(SPageInfo* aPageInfo);


	/**
	Check whether the specified page can be discarded by the RAM cache.

	@param aPageInfo The page info of the page being queried.
	@return ETrue when the page can be discarded, EFalse otherwise.
	@pre System lock held.
	@post System lock held.
	*/
	TBool IsPageDiscardable(SPageInfo& aPageInfo);

	/**
	Discard the specified page.
	Should only be called on a page if a previous call to IsPageDiscardable()
	returned ETrue and the system lock hasn't been released between the calls.
	The cache will not be reduced beyond the minimum size as a new page will 
	be allocated if necessary.
	
	@param aPageInfo The page info of the page to be discarded
	@param aBlockZoneId The ID of the RAM zone that shouldn't be allocated into.
	@param aBlockRest Set to ETrue to stop allocation as soon as aBlockedZoneId is reached 
	in preference ordering.  EFalse otherwise.
	@return ETrue if the page could be discarded, EFalse otherwise.
	
	@pre System lock held.
	@post System lock held.
	*/
	TBool DoDiscardPage(SPageInfo& aPageInfo, TUint aBlockedZoneId, TBool aBlockRest);

	/**
	First stage in discarding a list of pages.

	Must ensure that the pages will still be discardable even if system lock 
	is released after this method has completed.
	To be used in conjunction with DemandPaging::DoDiscardPages1().

	@param aPageList A NULL terminated list of the pages to be discarded
	@return KErrNone on success.

	@pre System lock held
	@post System lock held
	*/
	TInt DoDiscardPages0(SPageInfo** aPageList);


	/**
	Final stage in discarding a list of page
	Finish discarding the pages previously removed by DemandPaging::DoDiscardPages0().

	@param aPageList A NULL terminated list of the pages to be discarded
	@return KErrNone on success.

	@pre System lock held
	@post System lock held
	*/
	TInt DoDiscardPages1(SPageInfo** aPageList);

	/**
	Get a RAM page for use by a new page to be added to the live page list.
	This tries to obtain a RAM page from the following places:
	1. An unused page in the live page list.
	2. The systems free pool.
	3. The oldest page from the live page list.

    @pre Calling thread must be in a critical section.
	@pre System Lock held
	@post System Lock held
	*/
	SPageInfo* AllocateNewPage();

	/**
	Move an old page the new youngest page. I.e. move it the the head of the live page list
	and use the MMU to mark it accessible.
	*/
	void Rejuvenate(SPageInfo* aPageInfo);

	/**
	Reserve one page for locking.
	Increments the reserved page count. May increse the size of the live list, and the minimum and
	maximum page counts.  To unreserve a page, simply decrement the reserved page count.
	@return Whether the operation was sucessful.
	*/
	TBool ReservePage();

	/**
	Ensure all pages in the given region are present and 'lock' them so that they will not
	be paged out.
	To enable the pages to be paged out again, call UnlockRegion.
	@param aProcess The process to which the linear addresses refer, or NULL for global memory
	@pre Paging mutex held
	*/
	TInt LockRegion(TLinAddr aStart,TInt aSize,DProcess* aProcess);

	/**
	Mark in all pages in the given region as no longer locked.
	@param aProcess The process to which the linear addresses refer, or NULL for global memory
	This reverses the action of LockRegion.
	*/
	TInt UnlockRegion(TLinAddr aStart,TInt aSize,DProcess* aProcess);

	/**
	Flush (unmap) all memory which is demand paged.
	This reduces the live page list to a minimum.
	*/
	void FlushAll();

	/**
	Page in the specified pages and 'lock' it so it will not be paged out.
	To enable the page to be paged out again, call UnlockPage.
	@param aPage The linear address of the page to be locked.
	@param aProcess The process which the page is mapped in.
	@param aPhysAddr The physical address of the page which was locked.
	@pre System Lock held
	@post System Lock held (but may have been released by this function)
	*/
	TInt LockPage(TLinAddr aPage, DProcess* aProcess, TPhysAddr& aPhysAddr);

	/**
	Mark the specified page as no longer locked.
	This reverses the action of LockPage.
	@param aPage The linear address of the page to be unlocked.
	@param aProcess The process which the page is mapped in.
	@param aPhysAddr The physical address of the page which was originally locked. (Or KPhysAddrInvalid.)
	@pre System Lock held
	@post System Lock held
	*/
	TInt UnlockPage(TLinAddr aPage, DProcess* aProcess, TPhysAddr aPhysAddr);

	/**
	Implementation of DDemandPagingLock::Alloc
	*/
	TInt ReserveAlloc(TInt aSize, DDemandPagingLock& aLock);

	/**
	Implementation of DDemandPagingLock::Free
	*/
	void ReserveFree(DDemandPagingLock& aLock);

	/**
	Implementation of DDemandPagingLock::Lock
	*/
	TBool ReserveLock(DThread* aThread, TLinAddr aStart, TInt aSize, DDemandPagingLock& aLock);

	/**
	Implementation of DDemandPagingLock::Unlock
	*/
	void ReserveUnlock(DDemandPagingLock& aLock);

	/**
	Ensure a page is present, paging it in if necessary.  Used in the implementation of LockPage.
	@param aPage    The linear address of the page.
	@param aProcess The process the page is mapped in.
	*/
	virtual TInt EnsurePagePresent(TLinAddr aPage, DProcess* aProcess)=0;

	/**
	Get the physical address of a page.  Used in the implementation of LockPage and UnlockPage.
	@param aPage    The linear address of the page.
	@param aProcess The process the page is mapped in.
	*/
	virtual TPhysAddr LinearToPhysical(TLinAddr aPage, DProcess* aProcess)=0;

	/**
	Install the specified paging device.
	@param aDevice The device.
	@return KErrNone or standard error code.
	@post The devices DPagingDevice::iDeviceId has been set.
	*/
	TInt InstallPagingDevice(DPagingDevice* aDevice);

	/**
	Pure virutal function to allocate the virtual address space for temporary page mapping, for a
	paging request object.  This is called by DoInstallPagingDevice after the object is created.
	@param aReq   The paging request object
	@param aReqId An small integer unique to the supplied paging request object
	*/
	virtual void AllocLoadAddress(DPagingRequest& aReq, TInt aReqId)=0;

	/**
	Notify the paging system that a page of physical RAM that was used for demand paging is no
	longer mapped into any processes and is about to be freed.  This is called on the multiple
	memory model when a code segment is unloaded.  It is not implemented on the moving memory model.
	*/
	virtual void NotifyPageFree(TPhysAddr aPage)=0;

	/**
	Called when a realtime thread takes a paging fault.
	Checks whether it's ok for the thread to take to fault.
	@return KErrNone if the paging fault should be further processed
	*/
	TInt CheckRealtimeThreadFault(DThread* aThread, TAny* aContext);

	/**
	Memory-model specific method to indicate if an address range might contain paged memory.

	Implementations may return false positives but not false negatives - in other words this method
	may say the range contains paged memory when it does not, but not the other way around.

	This is used when pinning to determine whether memory could actally be paged.
	*/
	virtual TBool MayBePaged(TLinAddr aStartAddr, TUint aLength);

public:
	TUint iMinimumPageCount;	/**< Minimum size for the live page list, including locked pages */
	TUint iMaximumPageCount;	/**< Maximum size for the live page list, including locked pages */
	TUint16 iYoungOldRatio;		/**< Ratio of young to old pages in the live page list */
	SDblQue iYoungList;			/**< Head of 'young' page list. */
	TUint iYoungCount;			/**< Number of young pages */
	SDblQue iOldList;			/**< Head of 'old' page list. */
	TUint iOldCount;			/**< Number of young pages */
	TUint iReservePageCount;	/**< Number of pages reserved for locking */
	TUint iMinimumPageLimit;	/**< Minimum size for iMinimumPageCount, not including locked pages.
								     iMinimumPageCount >= iMinimumPageLimit + iReservePageCount */
	
	TLinAddr iTempPages;		/**< Uncached memory location in kernel memory which may be used
									 to map RAM pages whilst they are being paged in. */

	static DemandPaging* ThePager;	/**< Pointer to the single instance of this class */

	TUint iInitMinimumPageCount;	/**< Initial value for iMinimumPageCount */
	TUint iInitMaximumPageCount;	/**< Initial value for iMaximumPageCount  */

	TLinAddr iRomLinearBase;		/**< Linear address of ROM start. */
	TUint iRomSize;					/**< ROM size in bytes. */
	TLinAddr iRomPagedLinearBase;	/**< Linear address for the start of pagable ROM. */
	TUint iRomPagedSize;			/**< The size of pagable ROM in bytes.
										 (Zero indicates ROM is not pagable.) */
	SRomPageInfo* iRomPageIndex;	/**< Pointer to ROM page index. */

	TLinAddr iCodeLinearBase;		/**< Linear adderss of start of user code area. */
	TUint iCodeSize;				/**< Size of user code area in bytes. */

public:
	//
	// Paging device management...
	//	
	
	/**
	Information for a paging device.
	*/
	struct SPagingDevice
		{
		TBool			iInstalled;	/**< True, if this device has been installed. */
		DPagingDevice*	iDevice;	/**< Pointer to device object */
		};

	TInt DoInstallPagingDevice(DPagingDevice* aDevice, TInt aId);
	TInt ReadRomPage(const DPagingRequest* aReq, TLinAddr aRomAddress);
	TInt ReadCodePage(const DPagingRequest* aReq, DMmuCodeSegMemory* aCodeSegMemory, TLinAddr aCodeAddress);
	TInt Decompress(TInt aCompressionType,TLinAddr aDst,TLinAddr aSrc,TUint aSrcSize);

	inline SPagingDevice& RomPagingDevice()
		{ return iPagingDevices[0]; }

	inline SPagingDevice& CodePagingDevice(TInt aLocalDriveNumber)
		{ return iPagingDevices[aLocalDriveNumber + 1]; }

public:
	SPagingDevice iPagingDevices[KMaxPagingDevices];	/**< Array of paging devices. The first device is used for ROM paging. */
	DChunk* iDeviceBuffersChunk;	/**< Shared Chunk used to contain buffers for paging devices */
	TLinAddr iDeviceBuffers;		/**< Address for start of iDeviceBuffersChunk */
	TUint iDeviceBufferSize;		/**< Size of each individual buffer within iDeviceBuffers */

public:
	//
	// Paging request management...
	//

	/**
	Resources needed to service a paging request.
	*/
	class DPagingRequest : public SDblQueLink
		{
	public:
		~DPagingRequest();
	public:
		TThreadMessage	iMessage;	/**< Used by the media driver to queue requests */
 		DMutex*			iMutex;		/**< A mutex for synchronisation and priority inheritance. */
		TInt			iUsageCount;/**< How many threads are using or waiting for this object. */
		TLinAddr		iBuffer;	/**< A two-page buffer to read compressed data into. */
		TLinAddr		iLoadAddr;	/**< Virtual address to map page at while it's being loaded. */
		TPte*			iLoadPte;	/**< PTE corresponding to iLoadAddr. */
		};

	/**
	Creates a new DPagingRequest object and adds it to the list and free pool.
	Called from DoInstallPagingDevice.
	*/
	TInt CreateRequestObject();

	/**
	Get a paging request object, waiting if necessary for one to become available.
	@pre The system lock must be held.
	*/
	DPagingRequest* AcquireRequestObject();

	/**
	Release a previously acquired paging request object.
	@pre The system lock must be held.
	*/
	void ReleaseRequestObject(DPagingRequest* aReq);

public:
	/** Count of number of paging requests created. */
	TUint iPagingRequestCount;

	/** Array of paging request objects. */
	DPagingRequest* iPagingRequests[KMaxPagingRequests];

	/** Pool of unused paging request objects. */
	SDblQue iFreeRequestPool;

	/**
	Count of number of paging requests created or currently being created.  Used to allocate request
	object IDs and communicate eventual paging request count to ResizeLiveList.
	*/ 
	TInt iNextPagingRequestCount;


public:
	//
	// Test and debug...
	//

	/**
	Resize the live page list.
	*/
	TInt ResizeLiveList(TUint aMinimumPageCount,TUint aMaximumPageCount);

	/**
	Return state information about a page of memory ad the given address in the current process.
	*/
	virtual TInt PageState(TLinAddr aAddr)=0;

	/**
	Debug check to see if current thread can safely acquire the PagingMutex.
	Use this check in locations where paged memory may be accessed. It will detect code which
	would fault if paged memory were accessed.
	@return The held mutex that prohibits acquiring the PagingMutex, or NULL.
	*/
	DMutex* CheckMutexOrder();

	/**
	Memory-model specific method to indicate if a read from an address range requires a mutex order
	check.  Used in the implementation of CheckMutexOrder.
	*/
	virtual TBool NeedsMutexOrderCheck(TLinAddr aStartAddr, TUint aLength)=0;

	/**
	Fault the system.
	*/
	static void Panic(TFault aFault);

private:

	/**
	Non-cryptographically secure linear congruential pseudo random number generator -
	developed for speed with the use of a single multiply accumulate instruction.
	Derived from section "7.8 RANDOM NUMBER GENERATION" in ARM System Developer's
	guide.
	*/
	static TUint32 FastPseudoRand()
		{
		// Make sure the seed has been lazily initialised.
		FastPseudoRandomise();

		TUint32 oldX;
		TUint32 newX;

		// Keep trying to generate the next value in the pseudo random sequence until we
		// are sure no race has been caused by another thread which has entered the same
		// code.
		do
			{
			oldX = PseudoRandSeed;
			newX = 69069 * oldX + 41; // should compile to a single multiply accumulate instruction under ARM
			}
		while(!__e32_atomic_cas_acq32(&PseudoRandSeed, &oldX, newX));

		return newX;
		}

	/**
	Initialises the seed value for the pseudo random number generator.
	*/
	static void FastPseudoRandomise()
		{
		// Create the initial seed value for the pseudo random number generator using
		// the current system time.
		if(!PseudoRandInitialised) // race-prone but harmless - worst that can happen is that the seed is initialised more than once until PseudoRandInitialised is set to true
			{
			Int64 t = Kern::SystemTime();
			PseudoRandSeed = (TUint32)t ^ (TUint32)(t >> 32); // combine the two words for maximum entropy
			
			PseudoRandInitialised = ETrue;
			}
		}

public:
#ifdef __SUPPORT_DEMAND_PAGING_EMULATION__
	TInt iOriginalRomPageCount;
	TPhysAddr* iOriginalRomPages;
#endif

	SVMEventInfo iEventInfo;

#ifdef __CONCURRENT_PAGING_INSTRUMENTATION__
	TInt iWaitingCount;		///< number of threads waiting to acquire request object
	TInt iPagingCount;		///< number of threads holding request object
	TInt iMaxWaitingCount;  ///< maximum historical value of iWaitingCount
	TInt iMaxPagingCount;	///< maximum historical value of iPagingCount
#endif
	
#ifdef __DEMAND_PAGING_BENCHMARKS__
	void RecordBenchmarkData(TPagingBenchmark aBm, TUint32 aStartTime, TUint32 aEndTime);
	void ResetBenchmarkData(TPagingBenchmark aBm);
	SPagingBenchmarkInfo iBenchmarkInfo[EMaxPagingBm];
#endif

private:
	static TBool PseudoRandInitialised;	// flag to check whether FastPseudoRand has been lazily initialised yet
	static volatile TUint32 PseudoRandSeed;			// current random seed for FastPseudoRand()
	};

#ifdef __DEMAND_PAGING_BENCHMARKS__
	
#define START_PAGING_BENCHMARK TUint32 _bmStart = NKern::FastCounter()
#define END_PAGING_BENCHMARK(pager, bm) pager->RecordBenchmarkData(bm, _bmStart, NKern::FastCounter())

#else
	
#define START_PAGING_BENCHMARK
#define END_PAGING_BENCHMARK(pager, bm)

#endif
