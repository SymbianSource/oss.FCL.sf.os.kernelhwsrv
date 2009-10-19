// Copyright (c) 2009 Nokia Corporation and/or its subsidiary(-ies).
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
// e32/include/kernel/sshbuf.h
// Shareable Data Buffers


/**
	@file
	@internalComponent
	@prototype
*/

#ifndef SSHBUF_H
#define SSHBUF_H


// Forward declarations
class DShBuf;
class SMap;

#include <kernel/kern_priv.h>


class DShPoolClient : public DBase
	{
public:
	DShPoolClient() : iAccessCount(1), iFlags(0)
		{};
	TInt iAccessCount;
	TUint iFlags;
	};


class DShPool : public DObject
	{
public:
	DShPool();
	virtual ~DShPool();

	TInt Create(DObject* aOwner, TShPoolCreateInfo& aInfo);
	virtual TInt Close(TAny*);

	virtual TInt Alloc(DShBuf*&) = 0;
	void GetInfo(TShPoolInfo& aInfo);
	inline TUint BufSize() const {return iBufSize;};
	TUint FreeCount();
	TInt AddNotification(TShPoolNotifyType aType, TUint aThreshold, TRequestStatus& aStatus);
	TInt RemoveNotification(TShPoolNotifyType aType, TRequestStatus& aStatus);

	TInt RequestUserHandle(DThread *aThread, TOwnerType aType);
	TInt ModifyClientFlags(DProcess* aProcess, TUint aSetMask, TUint aClearMask);

	virtual TInt SetBufferWindow(DProcess* aProcess, TInt aWindowSize);

protected:
	virtual TInt DoCreate(TShPoolCreateInfo& aInfo);
	virtual TInt CreateInitialBuffers();
	virtual TInt DeleteInitialBuffers();
	virtual TInt GrowPool()=0;
	virtual TInt ShrinkPool()=0;
	TInt OpenClient(DProcess* aProcess, TUint& aFlags);
	void CloseClient(DProcess* aProcess);
	virtual void DestroyClientResources(DProcess* aProcess)=0;
	virtual void Free(DShBuf* aBuf) = 0;
	TInt UpdateReservedHandles(TInt aNoOfBuffers);
	virtual TInt UpdateFreeList() = 0;
	static void ManagementDfc(TAny* aPool);	// this DFC does the automatic allocations in the context of the driver thread, and completes any pending notification requests

	void CalculateGrowShrinkTriggers();
	static TUint mult_fx248(TUint n, TUint f);	// multiplies a TUint by an fx24.8 fixed-point ratio
	TBool HaveWorkToDo();
	void KickManagementDfc();

	void CompleteAllNotifications();
	void CheckAndCompleteNotifications(TBool aAll);
	void CheckLowSpaceNotificationQueue(TBool aAll);
	void CheckFreeSpaceNotificationQueue(TBool aAll);

	inline void LockPool()
		{
		NKern::FMWait(&iLock);
		}
	inline void UnlockPool()
		{
		NKern::FMSignal(&iLock);
		}

protected:
	SMap* iClientMap;

	static TDfcQue iSharedDfcQue;			// shared DFCQ thread for servicing asynch notification requests

	SDblQue iNotifLowReqQueue;		// queue of TShPoolNotificationRequests for low space (only accessed from our DFC thread, so access doesn't need to be protected by a mutex)
	SDblQue iNotifFreeReqQueue;		// queue of TShPoolNotificationRequests for free space (only accessed from our DFC thread, so access doesn't need to be protected by a mutex)
	TDfc iNotifDfc;				// DFC to auto alloc and/or complete notification requests (will run in DFC thread)
	TUint iLowSpaceThreshold;	// Value of largest threshold on list + 1
	TUint iFreeSpaceThreshold;	// Value of smallest threshold on list
	NFastMutex iLock;			// to protect allocations/deallocations of buffers on pool
	DMutex *iProcessLock;		// to protect access to the following

	SDblQue iFreeList;
	SDblQue iAltFreeList;
	TUint iFreeBuffers;			// number of free buffers
	TUint iTotalBuffers;	    // number of buffers in the pool
#ifdef _DEBUG
	TUint iAllocatedBuffers;	// number of allocated buffers
	SDblQue iAllocated;
#endif
	TUint iInitialBuffers;		// initial number of buffers
	TUint iMaxBuffers;			// total number of buffers there can ever be (if pool grown to max)
	TUint iAlignment;
	TUint iBufSize;				// size of buffers in this pool
	TUint iBufGap;				// bytes from one buffer to the next in this pool.	>= iBufSize.  For alignment.
	TUint iCommittedPages;		// current committed size of pool (chunk)
	TUint iMaxPages;			// max size of pool (chunk)
								// for growing and shrinking:
	TUint iGrowTriggerRatio;	// when to grow the pool (proportion free of total buffers, fixed-point fx24.8)
	TUint iGrowByRatio;			// how much bigger to make pool (proportion of total buffers, fx24.8)
	TUint iShrinkHysteresisRatio;	// to avoid shrinking immed after growing, also fx24.8
	TUint iShrinkByRatio;		// calculated from iGrowByRatio
	TUint iGrowTrigger;			// actual number of buffers to trigger grow, calculated from iGrowTriggerRatio
	TUint iShrinkTrigger;		// actual number of buffers to trigger shrink, (calc'd from iGrowShrinkRatio)

	TPhysAddr* iPhysAddr;
	TUint iPoolFlags;			// bitwise OR of values from TShPoolCreateFlags
	friend class ExecHandler;
	friend class DShBuf;
	friend class TShPoolNotificationCleanup;
	friend class DMemModelAlignedShBuf;
	friend class DMemModelNonAlignedShBuf;
	friend class DWin32ShBuf;
	friend class Kern;
	friend class K;
	};


/**
	A kernel-side representation of a shared buffer.

	Buffers are created and held by the pool (DShPool). Device drivers get pointers to buffer
	objects when allocating or when translating from received RShBuf handles.

	DShBufs objects are created (on the kernel heap) as a result of calling Alloc() or FromRShBuf().

	User-side, buffers are represented by objects of the RShBuf class, which holds a handle to
	the DShBuf object.  Create()ing an RShBuf a mapping is added to the respective DShPool and a
	DShBuf is also created.

	@see RShBuf
*/
class DShBuf : public DObject
	{
	friend class DShPool;
	friend class DMemModelShPool;
	friend class DMemModelAlignedShPool;
	friend class DMemModelNonAlignedShPool;
	friend class DWin32ShPool;
	friend class DWin32AlignedShPool;
	friend class DWin32NonAlignedShPool;

public:

	TInt RequestUserHandle(DThread *aThread, TOwnerType aType, TUint aAttr);

	TInt Close(TAny*);

	virtual TInt Construct();

	/**
	@return The size, in bytes, of this buffer (and every other buffer in the pool).
	*/
	inline TUint Size() const
		{
		return iPool->BufSize();
		};

protected:
	DShBuf(DShPool* aPool, TLinAddr aRelAddr);
	DShBuf(DShPool* aPool);
	virtual ~DShBuf();

	virtual TUint8* Base(DProcess* aProcess) = 0;
	virtual TUint8* Base() = 0;
	virtual TInt Map(TUint, DProcess*, TLinAddr&) = 0;
	virtual TInt UnMap(DProcess*) = 0;
	virtual TInt Pin(TPhysicalPinObject* aPinObject, TBool aReadOnly, TPhysAddr& aAddress, TPhysAddr* aPages, TUint32& aMapAttr, TUint& aColour);

	friend class ExecHandler;
	friend class Kern;

	DShPool* iPool;		///< Kern code sees pool
	TLinAddr iRelAddress;	///< Address of this buffer w.r.t. pool's iBaseAddress
	SDblQueLink iObjLink;
	};

#endif // SSHBUF_H
