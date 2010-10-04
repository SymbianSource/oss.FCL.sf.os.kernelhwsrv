// Copyright (c) 2007-2010 Nokia Corporation and/or its subsidiary(-ies).
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
// e32\include\drivers\dmasupport.h
// 
//

#ifndef DMASUPPORT_H
#define DMASUPPORT_H

#include "plat_priv.h"
#include <d32locd.h>


class TLocDrvRequest;

/**
@internalTechnology

Class used for read / write requests to the local media subsystem to gain access
to physical memory address to make use of DMA without the need of an intermediate buffer.
*/
class DDmaHelper : public DBase
	{
public:

	enum TMemoryType
		{
		EUnknown,
		EFileServerChunk,
		ESharedChunk,
		};

	/**
	Class used to describe a number of contiguous physical pages
	*/
	class TPageList
		{
	public:
		TPhysAddr iAddress;	// address of page
		TInt 	  iLength;
		};

public:
	DDmaHelper();
	~DDmaHelper();
	
	TInt Construct(TInt aLength, TInt aMediaBlockSize, TInt aDmaAlignment);
	
	TInt SendReceive(TLocDrvRequest& aReq, TLinAddr aLinAddress);
	TInt GetPhysicalAddress(TPhysAddr& aAddr, TInt& aLen);

#ifdef __DEMAND_PAGING__
	static TInt GetPhysicalAddress(TLocDrvRequest& aReq, TPhysAddr& aAddr, TInt& aLen);
#endif
	inline TInt PageSize()	const { return iPageSize; }
	
private:

	static inline TBool IsPageAligned(TLinAddr aAddr);
	static inline TLinAddr PageAlign(TLinAddr aAddr);
	static inline TLinAddr PageOffset(TLinAddr aAddr);

	inline TInt MaxFragLength()	const;
	inline void SetFragLength(TInt aLength);
	inline TInt FragLength()	const;
	inline TInt LengthRemaining() const;

	inline TBool IsDmaAligned(TLinAddr aAddr);
	inline TBool IsBlockAligned(TInt64 aPos);
	inline TInt64 BlockAlign(TInt64 aPos);
	inline TInt BlockOffset(TInt64 aPos);

	inline TLinAddr LinAddress() const;

	void ResetPageLists();

	TInt UpdateRemoteDescriptorLength(TInt aLength);

	TInt RequestStart();
	void RequestEnd();
	void BuildPageList();
	void ReleasePages(TLinAddr aAddr);

private:
	TInt iMediaBlockSize;		// Minimum transfer size (bytes) for the media.
	TInt64 iMediaBlockSizeMask;	// iMediaBlockSize - 1
	TInt iDmaAlignment;			// DMA Alignment req for media's DMA controller i.e. word alignment

	static TInt iPageSize;		// Memory page size in bytes (e.g. 4096 Bytes)
	static TInt iPageSizeLog2;	// Log2 of page size (e.g. 4096 -> 12)
	static TInt iPageSizeMsk;	// Mask of page size (e.g. 4096-1 -> 4095)
	TInt iMaxPages;				// Maximum number of pages that can be stored by this object

	DThread* iRemoteThread;		// 
	DThread* iCurrentThread;	//  
	DThread* iOwningThread;		// Thread owning remote descriptor, either iRemoteThread or iCurrentThread

	TLocDrvRequest* iReq;		// Current TLocDrvRequest
	
	// The following attributes are copied from the current TLocDrvRequest
	TInt iReqId;
	TInt iReqRemoteDesOffset;
	TInt iReqFlags;				// 
	TInt iReqLenClient;			// length of data requested by client (unmodified)
	TInt64 iReqPosClient;		// position of data requested by client (unmodified)

	TLinAddr iLinAddressUser;	// linear address of client buffer in user process
	TLinAddr iLinAddressKernel;	// linear address of client buffer in kernel process
	TInt iFragLen;				// length of data to be read into physical pages (possibly < a multiple of the page-size)
	TInt iFragLenRemaining;		// length of data to be read left in this fragment

	TMemoryType iMemoryType;
	
	/** array of (possibly non-contiguous) pages */
	TPhysAddr* iPageArray;
	TInt iPageArrayCount;
	
	/** list of contiguous pages */
	TPageList* iPageList;
	TInt iPageListCount;

	TInt iIndex;					// Current memory fragment index
	
	/** Represents the current read/write position in terms of an offset 
	from the start of the caller's linear address */ 
	TInt iLenConsumed;				// Offset from start of client buffer

	DChunk* iChunk;					// Shared chunk object in use
	TInt    iChunkOffset;			// Offset within shared chunk
	TUint32 iMapAttr;				// mmu mapping attributes for the Shared chunk or pinned physical memory.
	TUint32 iPhysAddr;				// Physical Address of chunk (if contiguous)

	TInt iLockCount;				// Prevent 2+ threads accessing this object
	
	TBool iPhysPinningAvailable;    // True if physical memory pinning Kernel interface is available
	TUint  iPageColour;				// Mapping colour of the first page in iPageArray.
	TPhysicalPinObject* iPhysicalPinObject;	// Physical pinning object.
	};



#endif	// DMASUPPORT_H
