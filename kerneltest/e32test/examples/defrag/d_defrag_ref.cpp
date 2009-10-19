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
// e32test\examples\defrag\d_defrag_ref.cpp
// Reference LDD for invoking defrag APIs.
// 
//

#include <kernel/kern_priv.h>
#include "platform.h"
#include "nk_priv.h"
#include "d_defrag_ref.h"

const TInt KMajorVersionNumber=0;
const TInt KMinorVersionNumber=1;
const TInt KBuildVersionNumber=1;

#if 1  // Set true for tracing
#define TRACE(x) x
#else
#define TRACE(x)
#endif

const TInt KDefragCompleteThreadPriority = 27;
const TInt KDefragRamThreadPriority = 1;
_LIT(KDefragCompleteThread,"DefragCompleteThread");

class DDefragChannel;

/**
	Clean up item responsible for ensuring all memory commmited to a chunk is
	freed once the chunk is destroyed
*/
class TChunkCleanup : public TDfc
    {
public:
    TChunkCleanup(DDefragChannel* aDevice, TPhysAddr* aBufAddrs, TUint aBufPages);
	TChunkCleanup(DDefragChannel* aDevice, TPhysAddr aBufBase, TUint aBufBytes);
    static void ChunkDestroyed(TChunkCleanup* aSelf);
	void RemoveDevice();

private:
    void DoChunkDestroyed();

private:
	TPhysAddr* iBufAddrs;		/**< Pointer to an array of the addresses of discontiguous buffer pages*/
	TPhysAddr iBufBase;			/**< Physical base address of a physically contiguous the buffer*/
	TUint iBufSize;				/**< The number of pages or bytes in the buffer depending if this is 
								discontiguous or contiguous buffer, repsectively*/
	TBool iBufContiguous;		/**< ETrue when the memory to be freed is contiguous, EFalse otherwise*/
	DDefragChannel* iDevice; 	/**< The device to be informed when the chunk is destroyed */
    };


/**
	Reference defrag LDD factory.
*/
class DDefragChannelFactory : public DLogicalDevice
	{
public:
	DDefragChannelFactory();
	~DDefragChannelFactory();
	virtual TInt Install();								//overriding pure virtual
	virtual void GetCaps(TDes8& aDes) const;			//overriding pure virtual
	virtual TInt Create(DLogicalChannelBase*& aChannel);//overriding pure virtual

	TDynamicDfcQue* iDfcQ;
	};


/**
	Reference defrag logical channel.
*/
class DDefragChannel : public DLogicalChannelBase
	{
public:
	DDefragChannel(TDfcQue* aDfcQ);
	~DDefragChannel();
	void ChunkDestroyed();
protected:
	virtual TInt DoCreate(TInt aUnit, const TDesC8* anInfo, const TVersion& aVer);
	virtual TInt Request(TInt aFunction, TAny* a1, TAny* a2);

	TInt DoAllocLowestZone();
	TInt DoClaimLowestZone();
	TInt DoChunkClose();
	TInt FindLowestPrefZone();

	static void DefragCompleteDfc(TAny* aSelf);
	void DefragComplete();

private:
	TInt iPageShift;			/**< The system's page shift */
	DSemaphore* iDefragSemaphore;/**< Semaphore to ensure only one defrag operation is active per channel*/
	TClientRequest* iCompleteReq;/**< Pointer to a request status that will signal to the user side client once the defrag has completed*/
	DThread* iRequestThread;	/**< Pointer to the thread that made the defrag request*/
	TRamDefragRequest iDefragReq;/**< The defrag request used to queue defrag operations*/
	DChunk* iBufChunk;			/**< Pointer to a chunk that can be mapped to a physical RAM area*/
	TChunkCleanup* iChunkCleanup;/**< Pointer to iBufChunk's cleanup object */
	TDfcQue* iDfcQ;				/**< The DFC queue used for driver functions */
	TDfc iDefragCompleteDfc;	/**< DFC to be queued once a defrag operation has completed */
	TBool iDefragDfcFree;		/**< Set to fase whenever a dfc defrag operation is still pending*/
	TUint iLowestPrefZoneId;	/**< The ID of the least preferable RAM zone*/
	TUint iLowestPrefZonePages;	/**< The number of pages in the least preferable RAM zone*/
	TUint iLowestPrefZoneIndex; /**< The test HAL function index of the least preferable RAM zone*/
	};

/**
Utility functions to wait for chunk clean dfc to be queued by waiting for the 
idle thread to be queued.
*/
void signal_sem(TAny* aPtr)
	{
	NKern::FSSignal((NFastSemaphore*)aPtr);
	}

TInt WaitForIdle()
	{// Wait for chunk to be destroyed and then for the chunk cleanup dfc to run.
	for (TUint i = 0; i < 2; i++)
		{
		NFastSemaphore s(0);
		TDfc idler(&signal_sem, &s, Kern::SvMsgQue(), 0);	// supervisor thread, priority 0, so will run after destroyed DFC
		NTimer timer(&signal_sem, &s);
		idler.QueueOnIdle();
		timer.OneShot(NKern::TimerTicks(5000), ETrue);	// runs in DFCThread1
		NKern::FSWait(&s);	// wait for either idle DFC or timer
		TBool timeout = idler.Cancel();	// cancel idler, return TRUE if it hadn't run
		TBool tmc = timer.Cancel();	// cancel timer, return TRUE if it hadn't expired
		if (!timeout && !tmc)
			NKern::FSWait(&s);	// both the DFC and the timer went off - wait for the second one
		if (timeout)
			return KErrTimedOut;
		}
	return KErrNone;
	}

/** 
	Standard logical device driver entry point.  
	Called the first time this device driver is loaded.
*/
DECLARE_STANDARD_LDD()
	{
	DDefragChannelFactory* factory = new DDefragChannelFactory;
	if (factory)
	{
		// Allocate a kernel thread to run the DFC 
		TInt r = Kern::DynamicDfcQCreate(factory->iDfcQ, KDefragCompleteThreadPriority, KDefragCompleteThread);

		if (r != KErrNone)
			{ 
			// Must close rather than delete factory as it is a DObject object.
			factory->AsyncClose();
			return NULL; 	
			} 	
	}
    return factory;
    }


/**
	Constructor
*/
DDefragChannelFactory::DDefragChannelFactory()
    {
    iVersion=TVersion(KMajorVersionNumber,KMinorVersionNumber,KBuildVersionNumber);
    }


/**
	Destructor
*/
DDefragChannelFactory::~DDefragChannelFactory()
    {
	if (iDfcQ != NULL)
		{// Destroy the DFC queue created when this device drvier was loaded.
		iDfcQ->Destroy();
		}
    }


/**
	Create a new DDefragChannel on this logical device.

@param  aChannel On successful return this will point to the new channel.
@return KErrNone on success or KErrNoMemory if the channel couldn't be created.
*/
TInt DDefragChannelFactory::Create(DLogicalChannelBase*& aChannel)
    {
	aChannel = new DDefragChannel(iDfcQ);
	return (aChannel)? KErrNone : KErrNoMemory;
    }


/**
	Install the LDD - overriding pure virtual

@return KErrNone on success or one of the system wide error codes.
*/
TInt DDefragChannelFactory::Install()
    {
    return SetName(&KLddName);
    }


/**
	Get capabilities - overriding pure virtual

@param aDes A descriptor to be loaded with the capabilities.
*/
void DDefragChannelFactory::GetCaps(TDes8& aDes) const
    {
    TCapsDefragTestV01 b;
    b.iVersion=TVersion(KMajorVersionNumber,KMinorVersionNumber,KBuildVersionNumber);
    Kern::InfoCopy(aDes,(TUint8*)&b,sizeof(b));
    }


/**
	Constructor

@param aDfcQ The DFC queue to use for defrag completion DFCs.
*/
DDefragChannel::DDefragChannel(TDfcQue* aDfcQ) 
		:
		iDefragSemaphore(NULL),
		iCompleteReq(NULL),
		iBufChunk(NULL),
		iChunkCleanup(NULL),
		iDfcQ(aDfcQ),
		iDefragCompleteDfc(DefragCompleteDfc, (TAny*)this, 1)  // DFC is priority '1', it is the only type of dfc on this queue.
    {
    }


/**
	Create channel.

@param aVer The version number required.
@return KErrNone on success, KErrNotSupported if the device doesn't support defragmentation.
*/
TInt DDefragChannel::DoCreate(TInt /*aUnit*/, const TDesC8* /*anInfo*/, const TVersion& aVer)
    {
	// Check the client has ECapabilityPowerMgmt capability.
	if(!Kern::CurrentThreadHasCapability(ECapabilityPowerMgmt, __PLATSEC_DIAGNOSTIC_STRING("Checked by DDefragChannel")))
		{
		return KErrPermissionDenied;
		}
	TInt pageSize;
	TInt r = Kern::HalFunction(EHalGroupKernel, EKernelHalPageSizeInBytes, &pageSize, 0);
	if (r != KErrNone)
		{
		TRACE(Kern::Printf("ERROR - Unable to determine page size"));
		return r;
		}
	TUint32 pageMask = pageSize;
	TUint i = 0;
	for (; i < 32; i++)
		{
		if (pageMask & 1)
			{
			if (pageMask & ~1u)
				{
				TRACE(Kern::Printf("ERROR - page size not a power of 2"));
				return KErrNotSupported;
				}
			iPageShift = i;
			break;
			}
		pageMask >>= 1;
		}

	// Check the client is a supported version.
    if (!Kern::QueryVersionSupported(TVersion(KMajorVersionNumber,KMinorVersionNumber,KBuildVersionNumber),aVer))
		{
    	return KErrNotSupported;
		}

	// Check this system has more than one RAM zone defined.
	// A real driver shouldn't need to do this as any driver that uses defrag should 
	// only be loaded on devices that support it.
	TInt ret = FindLowestPrefZone();
	if (ret != KErrNone)
		{// Only one zone so can't move pages anywhere or empty a zone
		return KErrNotSupported;
		}

	// Create a semaphore to protect defrag invocation.  OK to just use one name as
	// the semaphore is not global so it's name doesn't need to be unique.
	ret = Kern::SemaphoreCreate(iDefragSemaphore, _L("DefragRefSem"), 1);
	if (ret != KErrNone)
		{
		return ret;
		}

	// Create a client request for completing dfc defrag requests.
	ret = Kern::CreateClientRequest(iCompleteReq);
	if (ret != KErrNone)
		{
		iDefragSemaphore->Close(NULL);
		return ret;
		}

	// Setup a DFC to be invoked when a defrag operation completes.
	iDefragCompleteDfc.SetDfcQ(iDfcQ);
	iDefragDfcFree = ETrue;

	return KErrNone;
	}


/**
	Destructor
*/
DDefragChannel::~DDefragChannel()
    {
	// Clean up any heap objects.
	if (iDefragSemaphore != NULL)
		{
		iDefragSemaphore->Close(NULL);
		}

	// Unregister from any chunk cleanup object as we are to be deleted.
	if (iChunkCleanup != NULL)
		{
		iChunkCleanup->RemoveDevice();
		}
	// Clean up any client request object.
	if (iCompleteReq)
		{
		Kern::DestroyClientRequest(iCompleteReq);
		}
	// Free any existing chunk.
	DoChunkClose();
    }


/**
	Handle the requests for this channel.

@param aFunction 	The operation the LDD should perform.
@param a1 			The first argument for the operation.
@param a2 			The second argument for the operation.
@return KErrNone on success or one of the system wide error codes.
*/
TInt DDefragChannel::Request(TInt aFunction, TAny* a1, TAny* a2)
	{
	TInt r = KErrNone;
	NKern::ThreadEnterCS();

	Kern::SemaphoreWait(*iDefragSemaphore);
	if (!iDefragDfcFree && aFunction != RDefragChannel::EControlGeneralDefragDfcComplete)
		{// Only allow a single defrag operation at a time.
		r = KErrInUse;
		goto exit;
		}

	switch (aFunction)
		{
		case RDefragChannel::EControlGeneralDefragDfc:
			// Queue a defrag operation so that on completion it queues a
			// DFC on this driver.
			iRequestThread = &Kern::CurrentThread();
			iRequestThread->Open();

			// Open a reference on this channel to stop the destructor running before
			// the defrag request has completed.
			Open();
			r = iCompleteReq->SetStatus((TRequestStatus*)a1);
			if (r == KErrNone)
				r = iDefragReq.DefragRam(&iDefragCompleteDfc, KDefragRamThreadPriority);
			if (r != KErrNone)
				{// defrag operation didn't start so close all openned handles
				AsyncClose();
				iRequestThread->AsyncClose();
				iRequestThread = NULL;
				}
			else
				iDefragDfcFree = EFalse;
			break;

		case RDefragChannel::EControlGeneralDefragDfcComplete:
			if (iRequestThread != NULL)
				{// The defrag dfc hasn't completed so this shouldn't have been invoked.
				r = KErrGeneral;
				}
			else
				{
				iDefragDfcFree = ETrue;
				}
			break;

		case RDefragChannel::EControlGeneralDefragSem:
			{// Queue a defrag operation so that it will signal a fast mutex once
			// it has completed.
			NFastSemaphore sem;
			NKern::FSSetOwner(&sem, 0);
			r = iDefragReq.DefragRam(&sem, KDefragRamThreadPriority);

			if (r != KErrNone)
				{// Error occurred attempting to queue the defrag operation.
				break;
				}

			// Defrag operation has now been queued so wait for it to finish.
			// Could do some extra kernel side work here before waiting on the 
			// semaphore.
			NKern::FSWait(&sem);
			r = iDefragReq.Result();
			}
			break;

		case RDefragChannel::EControlGeneralDefrag:
			// Synchronously perform a defrag.
			{
			r = iDefragReq.DefragRam(KDefragRamThreadPriority);
			}
			break;

		case RDefragChannel::EControlAllocLowestZone:
			// Allocate from the lowest preference zone
			r = DoAllocLowestZone();
			break;

		case RDefragChannel::EControlClaimLowestZone:
			// Claims the lowest preference zone
			r = DoClaimLowestZone();
			break;
			
		case RDefragChannel::EControlCloseChunk:
			// Have finished with the chunk so close it then free the RAM mapped by it
			r = DoChunkClose();
			TRACE( if (r != KErrNone) {Kern::Printf("ChunkClose returns %d", r);});
			break;

		default:
			r=KErrNotSupported;
			break;
		}
exit:
	Kern::SemaphoreSignal(*iDefragSemaphore);
	NKern::ThreadLeaveCS();
	TRACE(if (r!=KErrNone)	{Kern::Printf("DDefragChannel::Request returns %d", r);	});
	return r;
	}


/**
	Allocates RAM from the lowest preference zone and maps it to a shared chunk.

	Real drivers would not need to determine which zone to allocate from as they
	will know the zone's ID.

@return KErrNone on success, otherwise one of the system wide error codes.
*/
TInt DDefragChannel::DoAllocLowestZone()
	{
	TInt r = KErrNone;
	TLinAddr chunkAddr = NULL;
	TUint32 mapAttr = NULL;
	TChunkCreateInfo createInfo;
	TLinAddr bufBaseAddr;
	TUint bufPages;
	TPhysAddr* bufAddrs;

	if (iBufChunk != NULL)
		{// The buffer chunk is already mapped so can't use again until it is 
		// freed/closed. Wait a short while for it to be freed as it may be in the 
		// process of being destroyed.
		if (WaitForIdle() != KErrNone || iBufChunk != NULL)
			{// chunk still hasn't been freed so can't proceed.
			r = KErrInUse;
			goto exit;
			}
		}
	
	// Attempt to allocate all the pages it should be possible to allocate.
	// Real device drivers will now how much they need to allocate so they
	// wouldn't determine it here.
	SRamZoneUtilisation zoneUtil;
	Kern::HalFunction(EHalGroupRam, ERamHalGetZoneUtilisation, (TAny*)iLowestPrefZoneIndex, (TAny*)&zoneUtil);
	bufPages = iLowestPrefZonePages - (zoneUtil.iAllocFixed + zoneUtil.iAllocUnknown + zoneUtil.iAllocOther);
	bufAddrs = new TPhysAddr[bufPages];
	if (!bufAddrs)
		{
		TRACE(Kern::Printf("Failed to allocate an array for bufAddrs"));
		r = KErrNoMemory;
		goto exit;
		}

	// Update the page count as bufAddrs allocation may have caused the kernel 
	// heap to grow.
	Kern::HalFunction(EHalGroupRam, ERamHalGetZoneUtilisation, (TAny*)iLowestPrefZoneIndex, (TAny*)&zoneUtil);
	bufPages = iLowestPrefZonePages - (zoneUtil.iAllocFixed + zoneUtil.iAllocUnknown + zoneUtil.iAllocOther);

	// Allocate discontiguous pages from the zone
	r = Epoc::ZoneAllocPhysicalRam(iLowestPrefZoneId, bufPages, bufAddrs);
	if (r != KErrNone && r != KErrNoMemory)
		{
		TRACE(Kern::Printf("Zone Alloc returns %d bufPages %x", r, bufPages));
		goto exit;
		}
	// If we couldn't allocate all the required pages then empty the zone
	// and retry.
	if (r == KErrNoMemory)
		{
		r = iDefragReq.EmptyRamZone(iLowestPrefZoneId, TRamDefragRequest::KInheritPriority);
		if (r != KErrNone)
			{
			TRACE(Kern::Printf("Empty returns %d", r));
			goto exit;
			}
		r = Epoc::ZoneAllocPhysicalRam(iLowestPrefZoneId, bufPages, bufAddrs);
		if (r != KErrNone)
			{
			TRACE(Kern::Printf("ZoneAlloc1 returns %d bufPages %x", r, bufPages));
			goto exit;
			}
		}
	
	// Create a chunk cleanup object which will free the physical RAM when the 
	// chunk is detroyed
	iChunkCleanup = new TChunkCleanup(this, bufAddrs, bufPages);
	if (!iChunkCleanup)
		{
		TRACE(Kern::Printf("iChunkCleanup creation failed"));
		r = Epoc::FreePhysicalRam(bufPages, bufAddrs);
		if (r != KErrNone)
			{
			TRACE(Kern::Printf("ERROR - freeing physical memory when chunkCleanup create failed"));
			}
		else
			{
			r = KErrNoMemory;
			}
		goto exit;
		}

	// Map the allocated buffer pages to a chunk so we can use it.	
	createInfo.iType = TChunkCreateInfo::ESharedKernelSingle; // could also be ESharedKernelMultiple
	createInfo.iMaxSize = bufPages << iPageShift;
	createInfo.iMapAttr = EMapAttrFullyBlocking; // Non-cached - See TMappingAttributes for all options
	createInfo.iOwnsMemory = EFalse; // Must be false as the physical RAM has already been allocated
	createInfo.iDestroyedDfc = iChunkCleanup;
	r = Kern::ChunkCreate(createInfo, iBufChunk, chunkAddr, mapAttr);
	if (r != KErrNone)
		{
		TRACE(Kern::Printf("ChunkCreate returns %d size %x pages %x", r, createInfo.iMaxSize, bufPages));
		goto exit;
		}

	// Map the physical memory to the chunk
	r = Kern::ChunkCommitPhysical(iBufChunk, 0, createInfo.iMaxSize, bufAddrs);
	if (r != KErrNone)
		{
		TRACE(Kern::Printf("CommitPhys returns %d", r));
		goto exit;
		}

	// Now that the RAM is mapped into a chunk get the kernel-side virtual 
	// base address of the buffer.
	r = Kern::ChunkAddress(iBufChunk, 0, createInfo.iMaxSize, bufBaseAddr);

	// Using bufBaseAddr a real driver may now do something with the buffer.  We'll just return.

exit:
	return r;
	}


/**
	Claims the lowest preference zone and maps it to a shared chunk.

	Real drivers would not need to determine which zone to allocate from as they
	will know the zone's ID.

@return KErrNone on success, otherwise one of the system wide error codes.
*/
TInt DDefragChannel::DoClaimLowestZone()
	{
	TInt r = KErrNone;
	TChunkCreateInfo createInfo;
	TLinAddr bufBaseAddr;
	TLinAddr chunkAddr;
	TUint32 mapAttr = NULL;
	TPhysAddr bufBase;
	TUint bufBytes;

	if (iBufChunk != NULL)
		{// The buffer chunk is already mapped so can't use again until it is 
		// freed/closed. Wait a short while for it to be freed as it may be in the 
		// process of being destroyed.
		if (WaitForIdle() != KErrNone || iBufChunk != NULL)
			{// chunk still hasn't been freed so can't proceed.
			r = KErrInUse;
			goto exit;
			}
		}

	// Claim the zone the base address of which will be stored in iBufBase.
	r = iDefragReq.ClaimRamZone(iLowestPrefZoneId, bufBase, TRamDefragRequest::KInheritPriority);
	if (r != KErrNone)
		{
		TRACE(Kern::Printf("Claim returns %d", r));
		goto exit;
		}

	// Create a chunk cleanup object which will free the physical RAM when the 
	// chunk is detroyed
	bufBytes = iLowestPrefZonePages << iPageShift;
	iChunkCleanup = new TChunkCleanup(this, bufBase, bufBytes);
	if (!iChunkCleanup)
		{
		TRACE(Kern::Printf("chunkCleanup creation failed"));
		r = Epoc::FreePhysicalRam(bufBytes, bufBase);
		if (r != KErrNone)
			{
			TRACE(Kern::Printf("ERROR - freeing physical memory when chunkCleanup create failed"));
			}
		else
			{
			r = KErrNoMemory;
			}
		goto exit;
		}

	// Map the allocated buffer pages to a chunk so we can use it.	
	createInfo.iType = TChunkCreateInfo::ESharedKernelSingle; // could also be ESharedKernelMultiple
	createInfo.iMaxSize = bufBytes;
	createInfo.iMapAttr = EMapAttrFullyBlocking; // Non-cached - See TMappingAttributes for all options
	createInfo.iOwnsMemory = EFalse; // Must be false as the physical RAM has already been allocated
	createInfo.iDestroyedDfc = iChunkCleanup;
	r = Kern::ChunkCreate(createInfo, iBufChunk, chunkAddr, mapAttr);
	if (r != KErrNone)
		{
		TRACE(Kern::Printf("ChunkCreate returns %d size %x bytes %x", r, createInfo.iMaxSize, bufBytes));
		goto exit;
		}

	// Map the physically contiguous memory to the chunk
	r = Kern::ChunkCommitPhysical(iBufChunk, 0, createInfo.iMaxSize, bufBase);
	if (r != KErrNone)
		{
		TRACE(Kern::Printf("CommitPhys returns %d", r));
		goto exit;
		}

	// Now that the RAM is mapped into a chunk get the kernel-side virtual 
	// base address of the buffer.
	r = Kern::ChunkAddress(iBufChunk, 0, createInfo.iMaxSize, bufBaseAddr);

	// Using bufBaseAddr a real driver may now do something with the buffer.  We'll just return.

exit:
	return r;
	}


/**
	Determine the lowest preference zone.

@return KErrNone on success or KErrNotFound if there is only one zone.
*/
TInt DDefragChannel::FindLowestPrefZone()
	{
	TUint zoneCount;
	TInt r = Kern::HalFunction(EHalGroupRam, ERamHalGetZoneCount, (TAny*)&zoneCount, NULL);
	if(r!=KErrNone)
		return r;

	if (zoneCount == 1)
		{// Only one zone so can't move pages anywhere or empty a zone
		return KErrNotFound;
		}

	SRamZoneConfig zoneConfig;
	SRamZoneUtilisation zoneUtil;
	Kern::HalFunction(EHalGroupRam, ERamHalGetZoneConfig, (TAny*)0, (TAny*)&zoneConfig);
	Kern::HalFunction(EHalGroupRam, ERamHalGetZoneUtilisation, (TAny*)0, (TAny*)&zoneUtil);
	TUint lowestPref = zoneConfig.iPref;
	TUint lowestFreePages = zoneUtil.iFreePages;
	iLowestPrefZoneIndex = 0;
	iLowestPrefZoneId = zoneConfig.iZoneId;
	TUint i = 1;
	for (; i < zoneCount; i++)
		{
		Kern::HalFunction(EHalGroupRam, ERamHalGetZoneConfig, (TAny*)i, (TAny*)&zoneConfig);
		Kern::HalFunction(EHalGroupRam, ERamHalGetZoneUtilisation, (TAny*)i, (TAny*)&zoneUtil);
		// When zones have the same preference the zone higher in the zone list is picked.
		if (zoneConfig.iPref > lowestPref || 
			(zoneConfig.iPref == lowestPref && zoneUtil.iFreePages >= lowestFreePages))
			{
			lowestPref = zoneConfig.iPref;
			lowestFreePages = zoneUtil.iFreePages;
			iLowestPrefZoneIndex = i;
			iLowestPrefZoneId = zoneConfig.iZoneId;
			}
		}
	// Now that we know the current least preferable zone store its size.
	Kern::HalFunction(EHalGroupRam, ERamHalGetZoneConfig, (TAny*)iLowestPrefZoneIndex, (TAny*)&zoneConfig);
	iLowestPrefZonePages = zoneConfig.iPhysPages;
	TRACE(Kern::Printf("LowestPrefZone %x size %x", iLowestPrefZoneId, iLowestPrefZonePages));
	return KErrNone;
	}


/**
	DFC callback called when a defrag operation has completed.

@param aSelf A pointer to the DDefragChannel that requested the defrag operation
*/
void DDefragChannel::DefragCompleteDfc(TAny* aSelf)
	{
	// Just call non-static method
	((DDefragChannel*)aSelf)->DefragComplete();
	}


/**
	Invoked by the DFC callback which is called when a defrag 
	operation has completed.
*/
void DDefragChannel::DefragComplete()
	{
	TRACE(Kern::Printf(">DDefragChannel::DefragComplete"));
	TInt result = iDefragReq.Result();
	TRACE(Kern::Printf("complete code %d", result));

	Kern::SemaphoreWait(*iDefragSemaphore);

	Kern::QueueRequestComplete(iRequestThread, iCompleteReq, result);
	iRequestThread->AsyncClose();
	iRequestThread = NULL;

	Kern::SemaphoreSignal(*iDefragSemaphore);

	TRACE(Kern::Printf("<DDefragChannel::DefragComplete"));
	// Close the handle on this channel - WARNING this channel may be 
	// deleted immmediately after this call so don't access any members
	AsyncClose();
	}


/**
	Close the chunk.

@return KErrNone on success or one of the system wide error codes.
*/
TInt DDefragChannel::DoChunkClose()
	{
	if (iBufChunk == NULL)
		{// Someone tried to close the chunk before using it
		return KErrNotFound;
		}

	// Rely on the chunk cleanup object being called as that
	// is what will actually free the physical RAM commited to the chunk.
	Kern::ChunkClose(iBufChunk);
	return KErrNone;
	}


/**
	The chunk has now been destroyed so reset the pointers to allow a new
	chunk to be created.
*/
void DDefragChannel::ChunkDestroyed()
	{
	__e32_atomic_store_ord_ptr(&iBufChunk, 0);
	__e32_atomic_store_ord_ptr(&iChunkCleanup, 0);
	}


/**
	Contruct a Shared Chunk cleanup object which will free the chunk's discontiguous
	physical memory when a chunk is destroyed.

@param aDevice The device to inform when the chunk is destroyed.
@param aBufBase The physical base addresses of each of the chunk's memory pages.
@param aBufPages The total number of the chunk's pages.
*/
TChunkCleanup::TChunkCleanup(DDefragChannel* aDevice, TPhysAddr* aBufAddrs, TUint aBufPages)
    : TDfc((TDfcFn)TChunkCleanup::ChunkDestroyed,this,Kern::SvMsgQue(),0),
    iBufAddrs(aBufAddrs),
	iBufSize(aBufPages),
	iBufContiguous(EFalse),
	iDevice(aDevice)
    {}


/**
	Contruct a Shared Chunk cleanup object which will free the chunk's contiguous 
	physical memory when a chunk is destroyed.

@param aDevice The device to inform when the chunk is destroyed.
@param aBufBase The physical base address of the chunk's memory.
@param aBufBytes The total number of the chunk's bytes.
*/
TChunkCleanup::TChunkCleanup(DDefragChannel* aDevice, TPhysAddr aBufBase, TUint aBufBytes)
    : TDfc((TDfcFn)TChunkCleanup::ChunkDestroyed,this,Kern::SvMsgQue(),0),
    iBufBase(aBufBase),
	iBufSize(aBufBytes),
	iBufContiguous(ETrue),
	iDevice(aDevice)
    {}

/**
	Callback function which is called the DFC runs, i.e. when a chunk is destroyed 
	and frees the physical memory allocated when the chunk was created.

@param aSelf Pointer to the cleanup object associated with the chunk that has 
been destroyed.
*/
void TChunkCleanup::ChunkDestroyed(TChunkCleanup* aSelf)
	{
	aSelf->DoChunkDestroyed();

    // We've finished so now delete ourself
    delete aSelf;
	}


/**
	The chunk has been destroyed so free the physical RAM that was allocated
	for its use and inform iDevice that it has been destroyed.
*/
void TChunkCleanup::DoChunkDestroyed()
    {
	if (iBufContiguous)
		{
		__NK_ASSERT_ALWAYS(Epoc::FreePhysicalRam(iBufBase, iBufSize) == KErrNone);
		}
	else
		{
		__NK_ASSERT_ALWAYS(Epoc::FreePhysicalRam(iBufSize, iBufAddrs) == KErrNone);
		}

	if (iDevice != NULL)
		{// Allow iDevice to perform any cleanup it requires for this chunk.
		iDevice->ChunkDestroyed();
		}
    }


/**
	Remove the device so its ChunkDestroyed() method isn't invoked  when the chunk is 
	destroyed.
*/
void TChunkCleanup::RemoveDevice()
	{
	__e32_atomic_store_ord_ptr(&iDevice, 0);
	}
