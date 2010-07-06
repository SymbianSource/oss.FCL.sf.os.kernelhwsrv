// Copyright (c) 2008-2009 Nokia Corporation and/or its subsidiary(-ies).
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
// e32\drivers\locmedia\dmasupport.cpp
// 
//

#include <kernel/kernel.h>
#include <kernel/cache.h>
#include "locmedia.h"
#include "dmasupport.h"
#include "dmasupport.inl"

#include "OstTraceDefinitions.h"
#ifdef OST_TRACE_COMPILER_IN_USE
#include "locmedia_ost.h"
#ifdef __VC32__
#pragma warning(disable: 4127) // disabling warning "conditional expression is constant"
#endif
#include "dmasupportTraces.h"
#endif

#define PHYSADDR_FAULT()	Kern::Fault("TLOCDRV-PHYS-ADDR",__LINE__)

//#define __DEBUG_DMASUP__
#ifdef __DEBUG_DMASUP__
#define __KTRACE_DMA(p) {p;}
#else
#define __KTRACE_DMA(p)
#endif

TInt DDmaHelper::iPageSize;
TInt DDmaHelper::iPageSizeLog2;
TInt DDmaHelper::iPageSizeMsk;

/******************************************************************************
 DDmaHelper
 ******************************************************************************/
const TPhysAddr KPhysMemFragmented = KPhysAddrInvalid;

TUint32 Log2(TUint32 aVal)
	{	
    __ASSERT_COMPILE(sizeof(TUint32) == 4);

    TUint32 bitPos=31;

    if(!(aVal >> 16)) {bitPos-=16; aVal<<=16;}
    if(!(aVal >> 24)) {bitPos-=8;  aVal<<=8 ;}
    if(!(aVal >> 28)) {bitPos-=4;  aVal<<=4 ;}
    if(!(aVal >> 30)) {bitPos-=2;  aVal<<=2 ;}
    if(!(aVal >> 31)) {bitPos-=1;}
    
    return bitPos;
	}

TBool IsPowerOfTwo(TInt aNum)
//
// Returns ETrue if aNum is a power of two
//
	{
	return (aNum != 0 && (aNum & -aNum) == aNum);
	}

void DDmaHelper::ResetPageLists()
	{
	iFragLen = 0;
	iFragLenRemaining = 0;
	}

DDmaHelper::DDmaHelper()
	{
	OstTraceFunctionEntry0( DDMAHELPER_DDMAHELPER_ENTRY );
	iPageSize = Kern::RoundToPageSize(1);
	__ASSERT_ALWAYS(IsPowerOfTwo(iPageSize), PHYSADDR_FAULT());
	iPageSizeLog2 = Log2(iPageSize);
	iPageSizeMsk = iPageSize-1;
	OstTraceFunctionExit0( DDMAHELPER_DDMAHELPER_EXIT );
	}

DDmaHelper::~DDmaHelper()
	{
	OstTraceFunctionEntry0( DESTRUCTOR_DDMAHELPER_ENTRY );
	delete [] iPageArray;
	delete [] iPageList;
	if (iPhysicalPinObject)
		{
		NKern::ThreadEnterCS();
		Kern::DestroyPhysicalPinObject(iPhysicalPinObject);
		NKern::ThreadLeaveCS();
		}
	OstTraceFunctionExit0( DESTRUCTOR_DDMAHELPER_EXIT );
	}

/**
Constructs the DDmaHelper object 

@param aLength The maximum length of data mapped by this object.
			   Should be a multiple of the page size 
@param aMediaBlockSize The minimum amount data that the media can transfer in read / write operations
@param aDmaAlignment The memory alignment required by the media devices DMA controller. (i.e. word aligned = 2)

@return KErrNone,if successful;
		KErrNoMemory, if unable to create Page Array's.
*/
TInt DDmaHelper::Construct(TInt aLength, TInt aMediaBlockSize, TInt aDmaAlignment)
	{
	OstTraceFunctionEntry1( DDMAHELPER_CONSTRUCT_ENTRY, this );
	__ASSERT_ALWAYS(aMediaBlockSize > 0, PHYSADDR_FAULT());
	__ASSERT_ALWAYS(IsPowerOfTwo(aMediaBlockSize), PHYSADDR_FAULT());
	__ASSERT_ALWAYS(aLength > 0, PHYSADDR_FAULT());
	__ASSERT_ALWAYS(aLength > iPageSize, PHYSADDR_FAULT());

	// This code assumes that the media block size (normally 512) is >= the processor's 
	// cache-line size (typically 32 bytes). This may not be true for future processors.
	// If the cache-line size was 1024, for example,  reading 512 bytes into a client's 
	// buffer & then calling Cache::SyncMemoryAfterDmaRead would invalidate an entire 1024 
	// bytes in the user's address space.
	TUint cacheLineSize = Cache::DmaBufferAlignment();
	__ASSERT_ALWAYS(IsPowerOfTwo(cacheLineSize), PHYSADDR_FAULT());
	if (cacheLineSize > (TUint) aMediaBlockSize)
	    {
		OstTraceFunctionExitExt( DDMAHELPER_CONSTRUCT_EXIT1, this, KErrNotSupported );
		return KErrNotSupported;
	    }
	
	//Check whether Kernel supports physical memory pinning:
	TInt mm = Kern::HalFunction(EHalGroupKernel, EKernelHalMemModelInfo, 0, 0) & EMemModelTypeMask;
	if (mm >= EMemModelTypeFlexible)
		{
		// Flexible memory model supports physical pinning for user (and Kernel) memory that
		// is the subject of DMA transfer.
		// Physical memory pinning ensures that:
		// - physical memory is not moved by RAM defragmentation.
		// - it is safe to to DMA against it or do sync cache (using new interface) even if/when
		// the owner of the memory (e.g. untrusted user aplication) decomits memory or panics.
		// For details @see Kern::PinPhysicalMemory.
		// Cache Sync of physically pinned memory on flexible memory model is done by:
		//  - Cache::SyncPhysicalMemoryBeforeDmaWrite
		//  - Cache::SyncPhysicalMemoryBeforeDmaRead
		//  - Cache::SyncPhysicalMemoryAfterDmaRead
		iPhysPinningAvailable = ETrue;
		__KTRACE_DMA(Kern::Printf("Memory model (%d) supports physical pinning\n",mm));
		NKern::ThreadEnterCS();
		TInt r=Kern::CreatePhysicalPinObject(iPhysicalPinObject);
		OstTraceExt2(TRACE_DMASUPPORT, DDMAHELPER_CONSTRUCT1, "Memory model=%d supports physical pinning; created Physical Pin Object with return value=%d",mm, r);
		NKern::ThreadLeaveCS();
		if (r) return r;
		}
	else
		{
		// Memory models before flexible do not support memory pinning.
		// The driver has to use  PrepareMemoryForDMA/ReleaseMemoryFromDMA Kernel interface 
		// that ensures that physical memory won't be moved by RAM defragmentation module.
		// However, Kernel relies on assumption that the user memory won't dissapear (e.g. by
		// user client closing the chunk or panics), as it would lead to Kernel crash.
		// For that reason, the only use case for DMA transfer into user memory is File System's
		// read/write buffer - as it is assumed that File System is trusted component.
		// To mark its buffers(s) for DMA transfer, File Sytem must call UserSvr::RegisterTrustedChunk
		// before DMA transfer starts.
		// Cache sync. operations before/after DMA transfer must be done by using the old Cache interface:
		//  - Cache::SyncMemoryBeforeDmaWrite
		//  - Cache::SyncMemoryBeforeDmaRead
		//  - Cache::SyncMemoryAfterDmaRead
		// As they all require linear address as input, these methods also rely on File System buffers
		// to be in valid state during sync calls.
		iPhysPinningAvailable = EFalse;
		__KTRACE_DMA(Kern::Printf("Memory model (%d) doesn't support physical pining",mm));
		OstTrace1(TRACE_DMASUPPORT, DDMAHELPER_CONSTRUCT2, "Memory model=%d doesn't support physical pinning",mm);
		iPhysicalPinObject = NULL;
		}
	
	iMaxPages = (aLength >> iPageSizeLog2)-1;
	
	// 2 Additional pages for page straddling
	iPageArray = new TPhysAddr[iMaxPages+2];
	if (iPageArray != NULL)
		{
		iPageList = new TPageList[iMaxPages];
		if (iPageList != NULL)
			{
			iMediaBlockSize = aMediaBlockSize;
			iMediaBlockSizeMask = TInt64(iMediaBlockSize - 1);

			iDmaAlignment = aDmaAlignment;
			__KTRACE_DMA(Kern::Printf("-PHYSADDR: Construct iMaxPages(%d), MediaBlocks(%d), DMAalign(%d)",iMaxPages,iMediaBlockSize,iDmaAlignment));
			OstTraceExt3(TRACE_FLOW, DDMAHELPER_CONSTRUCT_EXIT2, "< KErrNone PHYSADDR: Construct iMaxPages %d MediaBlocks %d DMAalign %d", iMaxPages,iMediaBlockSize,iDmaAlignment );
			return KErrNone;
			}
		delete [] iPageArray; iPageArray = NULL;
		}
	
	iMaxPages = 0;
	OstTraceFunctionExitExt( DDMAHELPER_CONSTRUCT_EXIT3, this, KErrNoMemory );
	return KErrNoMemory;
	}

/**
 * Each Read/Write request is examined to determine if the descriptor that 
 * is referenced is mapped to a physical memory object; 
 * if so it prepares the memory, updates the request with physical memory information
 * and issues the request.
 * If a request does not make use of physical memory or is not configured correctly the
 * request is passed through without modification.
 */
TInt DDmaHelper::SendReceive(TLocDrvRequest& aReq, TLinAddr aLinAddress)
	{
	OstTraceFunctionEntry0( DDMAHELPER_SENDRECEIVE_ENTRY );
	DPrimaryMediaBase& primaryMedia = *aReq.Drive()->iPrimaryMedia;
	
	TInt reqId = aReq.Id();
	if (reqId != DLocalDrive::ERead && reqId != DLocalDrive::EWrite)
	    {
	    OstTrace0(TRACE_FLOW, DDMAHELPER_SENDRECEIVE_EXIT1, "< Request is not ERead or EWrite, cannot perform Direct Memory Access");
		return aReq.SendReceive(&primaryMedia.iMsgQ);
	    }
		
	if ((I64HIGH(aReq.Length()) > 0) || (aReq.Length() < iMediaBlockSize))
	    {
	    OstTrace0(TRACE_FLOW, DDMAHELPER_SENDRECEIVE_EXIT2, "< Invalid request length, cannot perform Direct Memory Access");
		return aReq.SendReceive(&primaryMedia.iMsgQ);
	    }
	
	// If more than one user thread tries to access the drive, then bail out as there is 
	// only one DDmaHelper object per TLocDrv. Normally this shouldn't ever happen unless
	// a client app accesses the drive directly using TBusLOcalDrive or the file system is 
	// asynchronous (i.e. there is a separate drive thread) but the file server message is 
	// flagged as synchronous - e.g. EFsDrive
	if (TInt(__e32_atomic_add_ord32(&iLockCount, 1)) > 0)	// busy ?
		{
		__KTRACE_DMA(Kern::Printf("-PHYSADDR: BUSY"));
		__e32_atomic_add_ord32(&iLockCount, TUint32(-1));
		OstTrace0(TRACE_FLOW, DDMAHELPER_SENDRECEIVE_EXIT3, "< DMA Busy");
		return aReq.SendReceive(&primaryMedia.iMsgQ);
		}

	// make a copy of the request 
	iMemoryType = EUnknown;
	iReq = &aReq;
	iReqId = reqId;
		
	iReqPosClient = iReq->Pos();

	iReqLenClient = I64LOW(iReq->Length());

	iReqRemoteDesOffset = iReq->RemoteDesOffset();
	iReqFlags = iReq->Flags();

	iRemoteThread = iReq->RemoteThread();
	iCurrentThread = &Kern::CurrentThread();
	iOwningThread = iRemoteThread ? iRemoteThread : iCurrentThread;

	iChunk = NULL;
	iChunkOffset = 0;
	iLinAddressUser = NULL;
	iLenConsumed = 0;

	// point to the start of the descriptor
	iLinAddressUser = aLinAddress - iReqRemoteDesOffset;
	
	// Need to check descriptors from both direct Clients (i.e. file cache, RemoteThread == NULL )
	// and Remote Server Clients (file server clients, RemoteThread != NULL)
	// Shared Memory can potentially be used by both remote server and direct clients
	NKern::ThreadEnterCS();
	iChunk = Kern::OpenSharedChunk(iOwningThread, (const TAny*) iLinAddressUser, ETrue, iChunkOffset);
	NKern::ThreadLeaveCS();
	
	TInt fragments = 0;
	TInt r;
	do
		{
		__KTRACE_DMA(Kern::Printf(">PHYSADDR:SendReceive() iReqLen %d; iLenConsumed %d; fragments %d",iReqLenClient, iLenConsumed, fragments));
		OstTraceExt2( TRACE_DMASUPPORT, DDMAHELPER_SENDRECEIVE1, "PHYSADDR:SendReceive() iLenConsumed=%d; fragments=%d", iLenConsumed, fragments);
		r = RequestStart();
		if (r != KErrNone)
			{
			if (iChunk)
				{
				NKern::ThreadEnterCS();
				Kern::ChunkClose(iChunk);
				iChunk = NULL;
				NKern::ThreadLeaveCS();
				}
			__KTRACE_DMA(Kern::Printf("<PHYSADDR:SendReceive()- r:%d",r));
			OstTrace1( TRACE_FLOW, DDMAHELPER_SENDRECEIVE_EXIT4, "< PHYSADDR:SendReceive() Return code %d",r);
			iMemoryType = EUnknown;
			__e32_atomic_add_ord32(&iLockCount, TUint32(-1));
			return fragments ? r : iReq->SendReceive(&primaryMedia.iMsgQ);
			}
		else
			{
			iReq->Flags() |= TLocDrvRequest::EPhysAddr;
			}

		__KTRACE_DMA(Kern::Printf("-PHYSADDR:SendReceive() rThread %08X pos %08lX, len %d addr %08X off %08X", 
				iRemoteThread, iReq->Pos(), I64LOW(iReq->Length()), iLinAddressUser, iReqRemoteDesOffset));
		OstTraceExt4(TRACE_DMASUPPORT, DDMAHELPER_SENDRECEIVE2, "PHYSADDR:SendReceive() position=%Ld; length=%d; address=0x%x; offset=0x%x", iReq->Pos(), (TInt) I64LOW(iReq->Length()), (TUint) iLinAddressUser, (TUint) iReqRemoteDesOffset );
		
		__ASSERT_DEBUG(iReq->Length() == FragLength(), PHYSADDR_FAULT());
		__ASSERT_DEBUG(iReq->Length() != 0, PHYSADDR_FAULT());

		// reinstate iValue in case overwritten by DMediaPagingDevice::CompleteRequest()
		iReq->iValue = iReqId;
		
		OstTrace1(TRACE_DMASUPPORT, DDMAHELPER_SENDRECEIVE3, "Dma SendReceive Start iReq=%d", iReq);
		r = iReq->SendReceive(&primaryMedia.iMsgQ);
		OstTrace1(TRACE_DMASUPPORT, DDMAHELPER_SENDRECEIVE4, "Dma SendReceive Return iReq=%d", iReq);
		
		// The media driver could potentially choose to deal with the request 
		// without accessing physical memory (e.g. if the data is already cached).
		iLenConsumed += iFragLenRemaining;
				
		RequestEnd();
		
		ResetPageLists();

		fragments++;
		
		}
	while(r == KErrNone && LengthRemaining() > 0);

	if (iChunk)		
		{
		NKern::ThreadEnterCS();
		Kern::ChunkClose(iChunk);
		iChunk = NULL;
		NKern::ThreadLeaveCS();
		}
	
	// Set remote descriptor length to iReqLenClient
	if (iReqId == DLocalDrive::ERead && r == KErrNone)
		r = UpdateRemoteDescriptorLength(iReqLenClient);

	__KTRACE_DMA(Kern::Printf("<PHYSADDR:SendReceive()"));

	iMemoryType = EUnknown;

	__e32_atomic_add_ord32(&iLockCount, TUint32(-1));
	OstTraceFunctionExit0( DDMAHELPER_SENDRECEIVE_EXIT5 );
	return r;
	}


/**
 * Each read/write request is split into one or more DMA "fragments".
 * The maximum size of each fragment depends on the size of iPageArray[].
 * Subsquent calls to RequestStart maybe required to complete a request.
 * 
 * The physical address is checked for DMA alignment or the possibility of 
 * eventually alignment due to mis-aligned start/end media blocks.
 * 
 * A DMA "fragment" can be split over a number of pages as follows :
 * ----------------------------------------------------------
 * |    4K    |    4K    |    4K    |    4K    |
 * ----------------------------------------------------------
 *      ********************************		: region to be read
 * <----------- iFragLen ----------->
 * 
 * The pages may not be physically contiguous; if they are not, 
 * then they are supplied to the media driver one contiguous 
 * sequent at a time by GetPhysicalAddress()
 **/
TInt DDmaHelper::RequestStart()
	{
	OstTraceFunctionEntry1( DDMAHELPER_REQUESTSTART_ENTRY, this );
	__KTRACE_DMA(Kern::Printf(">PHYSADDR:RequestStart()"));

	iIndex = 0;

	TLinAddr startAddr = LinAddress();
	TInt64 startPos = iReqPosClient + iLenConsumed;
	TInt mediaBlockOffset = BlockOffset(startPos);
	TInt addrBlockOffset = BlockOffset(startAddr);
	TInt length = Min(LengthRemaining(), MaxFragLength());

	iPageArrayCount = iPageListCount = 0;

	TLinAddr firstPageStart = PageAlign(startAddr);
	TLinAddr lastPageStart = PageAlign(startAddr + length + iPageSize - 1);
	iPageArrayCount = (lastPageStart - firstPageStart + 1) >> iPageSizeLog2;

	iMemoryType = EUnknown;
	iPhysAddr = KPhysMemFragmented; // Default - Mark memory as fragmented

	//*************************************
	// Check Physical Page Alignment!!	
	//*************************************
	if (!IsBlockAligned(startPos))
		{
		// Will DMA align at next block alignment? such that DMA can be used
		TInt ofset = I64LOW((startPos + iMediaBlockSize) & (iMediaBlockSize-1));
		ofset = iMediaBlockSize - ofset;

		if (!IsDmaAligned(startAddr))
			{			
			__KTRACE_DMA(Kern::Printf("<PHYSADDR:RequestStart() - not DMA Aligned pos 0x%x addr 0x%x)",I64LOW(startPos), startAddr));
			OstTraceExt2( TRACE_FLOW, DDMAHELPER_REQUESTSTART_EXIT1, "< KErrNotSupported Not DMA Aligned startPos %x startAddr %x", I64LOW(startPos), startAddr );
			return KErrNotSupported;
			}
		}
	else 
		{ //block aligned!
		if (!IsDmaAligned(startAddr))
			{
			__KTRACE_DMA(Kern::Printf("<PHYSADDR:RequestStart() - not DMA Aligned (0x%x)",startAddr));
			OstTrace1(TRACE_FLOW, DDMAHELPER_REQUESTSTART_EXIT2, "< KErrNotSupported Not DMA Aligned startAddr %x", startAddr);
			return KErrNotSupported;
			}
		}

	//************************************************
	// Check for possible striping of RAM pages vs Media blocks
	// i.e. Media blocks which may straddle 2 non contiguous pages. 
	//************************************************
	if (mediaBlockOffset != addrBlockOffset)
		{
		__KTRACE_DMA(Kern::Printf("<PHYSADDR:RequestStart() - Frag / not block aligned: pos 0x%x addr 0x%x", I64LOW(startPos), startAddr));
		OstTraceExt2(TRACE_FLOW, DDMAHELPER_REQUESTSTART_EXIT3, "< KErrNotSupported Frag / not block aligned: startPos 0x%x startAddr 0x%x", I64LOW(startPos), startAddr );
		return KErrNotSupported;
		}

	//************************************************
	// Is it File Server Cache request ?
	//************************************************
	if (iChunk == NULL &&				// Not Shared memory
		iRemoteThread == NULL &&		// Direct Client Request
		IsPageAligned(startAddr) &&
		IsBlockAligned(startPos) &&
		(iPageArrayCount > 0) )
		{
		TLinAddr firstPageAddr = PageAlign(startAddr); //ensure that it is page aligned.
		
		TInt r = KErrNone;
		if (iPhysPinningAvailable)
			{
			TBool readOnlyMem = (iReqId == DLocalDrive::EWrite); 
			r =  Kern::PinPhysicalMemory(iPhysicalPinObject,  firstPageAddr, iPageArrayCount << iPageSizeLog2,
					readOnlyMem, iPhysAddr, iPageArray, iMapAttr, iPageColour, iCurrentThread);
			}
		else
			{
			NKern::ThreadEnterCS();
			r = Kern::PrepareMemoryForDMA(iCurrentThread, (void*)firstPageAddr, iPageArrayCount << iPageSizeLog2, iPageArray);
			NKern::ThreadLeaveCS();
			}
		if (r != KErrNone) 
		    {
			OstTraceFunctionExitExt( DDMAHELPER_REQUESTSTART_EXIT4, this, r );
			return r;
		    }

		iMemoryType = EFileServerChunk;
		
		__KTRACE_DMA(Kern::Printf("-PHYSADDR:RequestStart() - EFileServerChunk"));
		OstTrace0( TRACE_DMASUPPORT, DDMAHELPER_REQUESTSTART1, "EFileServerChunk");
		}
	//****************************
	// Is it shared chunk ?
	//****************************
	else if (iChunk)
		{
		// calculate chunk offset of start of first page
		TInt offset = iChunkOffset + iReqRemoteDesOffset+ iLenConsumed;
				
		TInt r = Kern::ChunkPhysicalAddress(iChunk, offset, length, iLinAddressKernel, iMapAttr, iPhysAddr, iPageArray);
		
		if (r < KErrNone)
		    {
			OstTraceFunctionExitExt( DDMAHELPER_REQUESTSTART_EXIT5, this, r );
			return r;  // 0 = Contiguous Memory, 1 = Fragmented/Dis-Contiguous Memory
		    }
			
		iMemoryType = ESharedChunk;
		
		__KTRACE_DMA(Kern::Printf("-PHYSADDR:RequestStart() - ESharedChunk"));
		OstTrace0( TRACE_DMASUPPORT, DDMAHELPER_REQUESTSTART2, "ESharedChunk");
		}
	else
		{
		__KTRACE_DMA(Kern::Printf("<PHYSADDR:RequestStart() - EUnknown"));
		OstTraceFunctionExitExt( DDMAHELPER_REQUESTSTART_EXIT6, this, KErrNotFound );
		return KErrNotFound;
		}

	SetFragLength(length);
	
	//************************************************
	// Build Contiguous Page list
	//************************************************
	BuildPageList();
	
	//************************************************
	// Set up request parameters for this fragment 
	//************************************************
	iReq->Length() = MAKE_TINT64(0, length);
	iReq->Pos() = iReqPosClient + iLenConsumed;
	iReq->RemoteDesOffset() = iReqRemoteDesOffset + iLenConsumed;
	// restore EAdjusted flag to ensure iReq->Pos() is adjusted correctly
	iReq->Flags()&= ~TLocDrvRequest::EAdjusted;
	iReq->Flags()|= (iReqFlags & TLocDrvRequest::EAdjusted);

	//************************************************
	// Sync memory
	//************************************************
	__KTRACE_DMA(Kern::Printf(">SYNC-PHYSADDR:addr 0x%x len %d", startAddr, length));
	OstTraceExt2(TRACE_DMASUPPORT, DDMAHELPER_REQUESTSTART3, "startAddr=0x%x length=%d", (TUint) startAddr, length );

	// Only sync whole blocks: it is assumed that the media driver will transfer 
	// partial start and end blocks without DMA

	TInt startBlockPartialLen = IsBlockAligned(startPos) ? 0 : iMediaBlockSize - BlockOffset(startPos);
	TInt blockLen = (TInt) BlockAlign(length - startBlockPartialLen);

	if (iReqId == DLocalDrive::EWrite)
		{
		if (iMemoryType == ESharedChunk)
			{
			Cache::SyncMemoryBeforeDmaWrite(iLinAddressKernel+startBlockPartialLen, blockLen, iMapAttr);
			}
		else // (iMemoryType == EFileServerChunk)
			{
			if (iPhysPinningAvailable)
				Cache::SyncPhysicalMemoryBeforeDmaWrite(iPageArray, iPageColour, startBlockPartialLen, blockLen, iMapAttr);
			else
				Cache::SyncMemoryBeforeDmaWrite(startAddr+startBlockPartialLen, blockLen);
			}
		}
	else
		{
		if (iMemoryType == ESharedChunk)
			Cache::SyncMemoryBeforeDmaRead(iLinAddressKernel, length, iMapAttr);
		else // (iMemoryType == EFileServerChunk)
			{
			if (iPhysPinningAvailable)
				Cache::SyncPhysicalMemoryBeforeDmaRead(iPageArray, iPageColour, 0, length, iMapAttr);
			else
				Cache::SyncMemoryBeforeDmaRead(startAddr, length);
			}
		}

	__KTRACE_DMA(Kern::Printf("<PHYSADDR:RequestStart()"));

	OstTraceFunctionExitExt( DDMAHELPER_REQUESTSTART_EXIT7, this, KErrNone );
	return KErrNone;
	}

/**
 * After read requests this method synchronous the current physical memory in use.
 */
void DDmaHelper::RequestEnd()
	{
	OstTraceFunctionEntry0( DDMAHELPER_REQUESTEND_ENTRY );
	__KTRACE_DMA(Kern::Printf(">PHYSADDR:RequestEnd()"));


	__ASSERT_DEBUG(iReqId == DLocalDrive::ERead || iReqId == DLocalDrive::EWrite, PHYSADDR_FAULT());
	__ASSERT_DEBUG(iMemoryType == ESharedChunk || iMemoryType == EFileServerChunk, PHYSADDR_FAULT());

	TInt length = FragLength();	// len of data just transferred
	TLinAddr startAddr = LinAddress() - length;

	// Sync the memory : but not if the media driver has decided to transfer ALL the data using IPC rather than DMA.
	// It is assumed that the media driver will transfer partial start & end blocks using IPC, but it may also choose 
	// to use IPC for the ENTIRE fragment when read/writing at the end of the media (see medmmc.cpp)
	if (iFragLenRemaining < length && iReqId == DLocalDrive::ERead)
		{
		TInt64 startPos = iReq->Pos();
		TInt startBlockPartialLen = IsBlockAligned(startPos) ? 0 : iMediaBlockSize - BlockOffset(startPos);
		TInt blockLen = (TInt) BlockAlign(length - startBlockPartialLen);

		if (iMemoryType == ESharedChunk)
			{
			Cache::SyncMemoryAfterDmaRead(iLinAddressKernel + startBlockPartialLen, blockLen);
			}
		else // (iMemoryType == EFileServerChunk)
			{
			if (iPhysPinningAvailable)
				Cache::SyncPhysicalMemoryAfterDmaRead(iPageArray, iPageColour, startBlockPartialLen, blockLen, iMapAttr);
			else
				Cache::SyncMemoryAfterDmaRead(startAddr + startBlockPartialLen, blockLen);
			}

		}
	ReleasePages(PageAlign(startAddr));
	OstTraceFunctionExit0( DDMAHELPER_REQUESTEND_EXIT );
	}

/**
 * For File Server chunks this method releases the current physical memory in use.
 * 
 * @see Kern::ReleaseMemoryFromDMA()
 */
void DDmaHelper::ReleasePages(TLinAddr aAddr)
	{
	OstTraceFunctionEntry1( DDMAHELPER_RELEASEPAGES_ENTRY, this );
	if (iMemoryType == EFileServerChunk)
		{
		__KTRACE_DMA(Kern::Printf(">PHYSADDR():ReleasePages thread (0x%x) aAddr(0x%08x) size(%d) iPageArray(0x%x)",iCurrentThread, aAddr, (iPageArrayCount << iPageSizeLog2), iPageArray));
		OstTraceExt3( TRACE_DMASUPPORT, DDMAHELPER_RELEASEPAGES, "ReleasePages aAddr=0x%x; size=%d; iPageArray-0x%x", (TUint) aAddr, (iPageArrayCount << iPageSizeLog2), (TUint) iPageArray);

		TInt r;
		if (iPhysPinningAvailable)
			{
			r = Kern::UnpinPhysicalMemory(iPhysicalPinObject);
			}
		else
			{
			NKern::ThreadEnterCS();
			r = Kern::ReleaseMemoryFromDMA(iCurrentThread, (void*) aAddr, iPageArrayCount << iPageSizeLog2, iPageArray);
			NKern::ThreadLeaveCS();
			}
		__ASSERT_ALWAYS(r == KErrNone, PHYSADDR_FAULT());
		}		
	OstTraceFunctionExit1( DDMAHELPER_RELEASEPAGES_EXIT, this );
	}

/**
 * Utility method which examines the page array, compiling adjacent pages into contiguous fragments
 * and populating iPageList with said fragments.
 */
void DDmaHelper::BuildPageList()
	{
	OstTraceFunctionEntry1( DDMAHELPER_BUILDPAGELIST_ENTRY, this );
	iPageListCount = 0;
	
	if (iPhysAddr != KPhysMemFragmented)
		{
		__KTRACE_DMA(Kern::Printf(">PHYSADDR:BuildPageList() - Contiguous Memory"));
		OstTrace0( TRACE_DMASUPPORT, DDMAHELPER_BUILDPAGELIST1, "Contiguous Memory");
		// Only one entry required.
		iPageList[0].iAddress = iPhysAddr;
		iPageList[0].iLength = FragLength();
		iPageListCount = 1;
		}
	else
		{
		__KTRACE_DMA(Kern::Printf(">PHYSADDR:BuildPageList() - Dis-Contiguous Memory"));
		OstTrace0( TRACE_DMASUPPORT, DDMAHELPER_BUILDPAGELIST2, "Dis-Contiguous Memory");
		TInt offset;
		
		offset = PageOffset(iChunkOffset + iReqRemoteDesOffset+ iLenConsumed);
		iPageList[0].iAddress = iPageArray[0]+offset;
		iPageList[0].iLength  = iPageSize-offset;
		
		TInt lengthRemaining = FragLength() - iPageList[0].iLength;
		
		TInt i =1;
        for( ; i < iPageArrayCount; i++)
            {
            //Check if RAM pages are physically adjacent
            if ((iPageArray[i-1] + PageSize()) == iPageArray[i])
                {
                // Adjacent pages - just add length
                iPageList[iPageListCount].iLength += PageSize();             
                }
            else     	
                {
                // Not Adjacent, start new Memory fragment
                iPageListCount++;
                iPageList[iPageListCount].iAddress = iPageArray[i];
                iPageList[iPageListCount].iLength  = iPageSize;
                }
            
            lengthRemaining -= PageSize();
            if (lengthRemaining < 0)
            	{
            	// Last page, re-adjust length for odd remainder.            	
            	iPageList[iPageListCount].iLength += lengthRemaining;
            	break;
            	}
            }
        
        iPageListCount++;
		}

//#ifdef __DEBUG_DMASUP__
//	for (TInt m=0; m<iPageListCount; m++)
//		__KTRACE_DMA(Kern::Printf("-PHYSADDR:BuildPageList() [%d]: %08X l:%d", m, iPageList[m].iAddress, iPageList[m].iLength));
//#endif
	OstTraceFunctionExit1( DDMAHELPER_BUILDPAGELIST_EXIT, this );
	}


/**
 * Returns Address and Length of next contiguous Physical memory fragment
 * 
 * @param aAddr On success, populated with the Physical Address of the next fragment.
 * @param aLen  On success, populated with the length in bytes of the next fragment.
 * 
 * @return KErrNone, if successful;
 * 		   KErrNoMemory, if no more memory fragments left.
 */
TInt DDmaHelper::GetPhysicalAddress(TPhysAddr& aAddr, TInt& aLen)
	{
	OstTraceFunctionEntry1( DUP1_DDMAHELPER_GETPHYSICALADDRESS_ENTRY, this );
	if (iIndex >= iPageListCount)
		{
		__KTRACE_DMA(Kern::Printf(">PHYSADDR:GetPhysD() [%d], PageListCount:%d", iIndex, iPageListCount));
		OstTraceExt2(TRACE_DMASUPPORT, DDMAHELPER_GETPHYSICALADDRESS1, "GetPhysD() [%d]; iPageCountList=%d", iIndex, iPageListCount );
		aAddr = 0;
		aLen = 0;
		OstTraceFunctionExitExt( DUP1_DDMAHELPER_GETPHYSICALADDRESS_EXIT1, this, KErrGeneral );
		return KErrGeneral;
		}
	
	aAddr = iPageList[iIndex].iAddress;
	aLen = iPageList[iIndex].iLength;
	iLenConsumed+= aLen;
	iFragLenRemaining-= aLen;
	
	__KTRACE_DMA(Kern::Printf(">PHYSADDR:GetPhysD() [%d] addr:0x%08X, l:%d; Used:%d, Left:%d", iIndex, aAddr, aLen, iLenConsumed, iFragLenRemaining));
	OstTraceExt5(TRACE_DMASUPPORT, DDMAHELPER_GETPHYSICALADDRESS2, "GetPhysD() [%d]; address=0x%x; length=%d; iLenConsumed=%d; iFragLenRemaining=%d", iIndex, (TUint) aAddr, aLen, iLenConsumed, iFragLenRemaining);
	__ASSERT_DEBUG(aLen >= 0, PHYSADDR_FAULT());

	iIndex++;  //Move index to next page

	OstTraceFunctionExitExt( DDMAHELPER_GETPHYSICALADDRESS_EXIT2, this, KErrNone );
	return KErrNone;
	}


#ifdef __DEMAND_PAGING__
/**
 * Returns Address and Length of next contiguous Physical memory. 
 * Static function specifically for Demand Paging support
 * 
 * @param aReq  TLocDrvRequest from which physical 
 * @param aAddr Populated with the Physical Address of the Request aReq.
 * @param aLen  Populated with the length in bytes of the memory.
 * 
 * @return KErrNone 
 */
TInt DDmaHelper::GetPhysicalAddress(TLocDrvRequest& aReq, TPhysAddr& aAddr, TInt& aLen)
	{
	OstTraceFunctionEntry0( DDMAHELPER_GETPHYSICALADDRESS_ENTRY );
	__ASSERT_DEBUG( (aReq.Flags() & TLocDrvRequest::ETClientBuffer) == 0,  PHYSADDR_FAULT());

	TInt reqLen = I64LOW(aReq.Length());
    __ASSERT_DEBUG(I64HIGH(aReq.Length()) == 0,  PHYSADDR_FAULT());
    
    TInt& offset = aReq.RemoteDesOffset();    
	if (aReq.Flags() & TLocDrvRequest::EPhysAddrOnly)
        {        
        __KTRACE_DMA(Kern::Printf("-Physical Address Only"));
        __ASSERT_DEBUG(IsPageAligned((TLinAddr)reqLen), PHYSADDR_FAULT());
        
        TPhysAddr* physArray = (TPhysAddr*)aReq.RemoteDes();        
        
        // Adjust start to the next element to be issued                 
        TUint pageNo = offset>>iPageSizeLog2;
        TPhysAddr currPhysPageAddr = aAddr = physArray[pageNo];
        __ASSERT_DEBUG(IsPageAligned((TLinAddr)currPhysPageAddr), PHYSADDR_FAULT());
        offset+=iPageSize; 
        aLen=iPageSize;

        //Determine how many pages are contiguous
        TPhysAddr nextPhysPageAddr;        
        while (offset < reqLen)
            {
            pageNo++;
            nextPhysPageAddr = physArray[pageNo];            
            __ASSERT_DEBUG(PageOffset((TLinAddr) nextPhysPageAddr) == 0,  PHYSADDR_FAULT());

            if (nextPhysPageAddr != currPhysPageAddr + iPageSize)
                break;
            
            currPhysPageAddr = nextPhysPageAddr;            
            offset+= iPageSize;
            aLen+= iPageSize;
            }
        
        __KTRACE_DMA(Kern::Printf(">PHYSADDR:DP:GetPhysS(PhysAddrOnly), physAddr %08X, len %x reqLen %x", aAddr, aLen, reqLen));
        }
	else
	    {	
        TLinAddr linAddr = (TLinAddr) aReq.RemoteDes();
        TLinAddr currLinAddr = linAddr + offset;	
    
        aAddr = Epoc::LinearToPhysical(currLinAddr);
    
        // Set the initial length to be the length remaining in this page or the request length (whichever is shorter).
        // If there are subsequent pages, we then need to determine whether they are contiguous
        aLen = Min( (TInt) (PageAlign(currLinAddr+iPageSize) - currLinAddr), reqLen - offset);
    
        __ASSERT_DEBUG(aLen > 0,  PHYSADDR_FAULT());
        
        TPhysAddr currPhysPageAddr = PageAlign((TLinAddr) aAddr);
    
        offset+= aLen;
    
        while (offset < reqLen)
            {
            TPhysAddr nextPhysPageAddr = Epoc::LinearToPhysical(linAddr + offset);
            __ASSERT_DEBUG(PageOffset((TLinAddr) nextPhysPageAddr) == 0,  PHYSADDR_FAULT());
    
            if (nextPhysPageAddr != currPhysPageAddr + iPageSize)
                break;
            
            currPhysPageAddr = nextPhysPageAddr;
    
            TInt len = Min(iPageSize, reqLen - offset);
            offset+= len;
            aLen+= len;
            }

        __KTRACE_DMA(Kern::Printf(">PHYSADDR:DP:GetPhysS(), linAddr %08X, physAddr %08X, len %x reqLen %x", linAddr + offset, aAddr, aLen, reqLen));
        OstTraceExt4(TRACE_DEMANDPAGING, DDMAHELPER_GETPHYSICALADDRESS_DP, "linAddr=0x%x; physAddr=0x%x; length=0x%x; reqLen=0x%x", linAddr + offset, aAddr, aLen, reqLen);
	    }
	
	OstTraceFunctionExit0( DDMAHELPER_GETPHYSICALADDRESS_EXIT );
	return KErrNone;
	}
#endif	// (__DEMAND_PAGING__)


/**
 * Modifies the current requests remote descriptor length
 * 
 * @param aLength Length in bytes to which the descriptor is to be set.
 * 
 * @return KErrNone, if successful;
 * 		   KErrBadDescriptor, if descriptor is corrupted;
 * 		   otherwise one of the other system wide error codes.
 */

TInt DDmaHelper::UpdateRemoteDescriptorLength(TInt aLength)
	{
	OstTraceFunctionEntryExt( DDMAHELPER_UPDATEREMOTEDESCRIPTORLENGTH_ENTRY, this );
	__KTRACE_DMA(Kern::Printf(">PHYSADDR:UpDesLen(%d)",aLength));

	// Restore request Id (overwritten by KErrNone return code) to stop ASSERT in WriteRemote
	iReq->Id() = DLocalDrive::ERead;

	// restore caller's descriptor offset
	iReq->RemoteDesOffset() = iReqRemoteDesOffset;

	// Write a zero length descriptor at the end such that the descriptors length is correctly updated.
	TPtrC8 zeroDes(NULL, 0);
	TInt r = iReq->WriteRemote(&zeroDes, aLength);

	// restore return code	
	iReq->iValue = KErrNone;

	OstTraceFunctionExitExt( DDMAHELPER_UPDATEREMOTEDESCRIPTORLENGTH_EXIT, this, r );
	return r;
	}

