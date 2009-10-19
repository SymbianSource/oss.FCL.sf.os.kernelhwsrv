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
// e32/include/e32shbuf.h
// Shareable Data Buffers

/**
	@file
	@publishedPartner
	@prototype
*/

#ifndef E32SHBUF_H
#define E32SHBUF_H

#include <e32shbufcmn.h>
#include <e32cmn.h>

/**
	Specifies characteristics of the pool to be created.

	@see RShPool::Create()

	@publishedPartner
	@prototype
*/

class TShPoolCreateInfo
	{
public:
	/**
	Enumeration type to create a pool with page-aligned buffers.

	The buffers in the pool will be at least the size of an MMU page,
	aligned to an MMU page, and may be mapped in and out of a process's
	address space independently.
	*/
	enum TShPoolPageAlignedBuffers
		{
		EPageAlignedBuffer = EShPoolPageAlignedBuffer,
		};

	/**
	Enumeration type to create a pool with non-page-aligned buffers.

	The buffers in the pool do not have any size or alignment restrictions beyond
	the iAlignment specified in TShPoolInfo.

	The whole pool will always mapped into a process's address space.
	*/
	enum TShPoolNonPageAlignedBuffers
		{
		ENonPageAlignedBuffer = EShPoolNonPageAlignedBuffer,
		};

	/**
	Specifies the buffer size and initial number of committed buffers for a pool
	with page-aligned buffers.

	@param aFlag			Page-aligned buffers
	@param aBufSize			Size in bytes of each buffer within the pool
	@param aInitialBufs		Initial number of buffers allocated to the pool
	*/
	IMPORT_C TShPoolCreateInfo(TShPoolPageAlignedBuffers aFlag, TUint aBufSize, TUint aInitialBufs);

	/**
	Specifies the buffer size, initial number of committed buffers, and buffer
	alignment for a pool with non-page-aligned buffers.

	@param aFlag			Non-page aligned buffers
	@param aBufSize			Size in bytes of each buffer within the pool
	@param aInitialBufs		Initial number of buffers allocated to the pool
	@param aAlignment		Alignment of the start of each buffer in the pool (shift/log2 value)
	*/
	IMPORT_C TShPoolCreateInfo(TShPoolNonPageAlignedBuffers aFlag, TUint aBufSize, TUint aInitialBufs, TUint aAlignment);

	/**
	Sets the sizing attributes for the pool, allowing it to grow and
	shrink automatically.

	If either aGrowTriggerRatio or aGrowByRatio is 0, no automatic growing or
	shrinking will be performed.

	@param aMaxBufs					The maximum number of buffers that the pool
									can grow to.  This value must not be less than
									aInitialBufs.

	@param aGrowTriggerRatio		This specifies when to grow the pool.  When the
									proportion of free buffers in the pool drops below
									this value, the pool will be grown.  This value is
									expressed as a 32-bit fixed-point number, where
									the binary point is defined to be between bits
									7 and 8 (where the least-significant bit is defined
									as bit 0).  (This format is also known as a Q8, or
									fx24.8 number, or alternatively as the value * 256.)
									It must represent a value < 1 (i.e. must be < 256).

	@param aGrowByRatio				The proportion to grow the pool by each time,
									expressed as a 32-bit fx24.8 fixed-point number, in
									the same way as aGrowTriggerRatio.

	@param aShrinkHysteresisRatio	The hysteresis value to ensure that a pool is not
									automatically shrunk immediately after it is grown.
									Automatic shrinking will only happen when there are
									(aGrowTriggerRatio + aGrowByRatio) * aShrinkHysteresisRatio *
									(total buffers in the pool) free buffers left in the pool.
									This is a 32-bit fx24.8 fixed-point number in the
									same way as aGrowTriggerRatio and aGrowByRatio.
									It must represent a value > 1 (i.e. must be > 256).
	*/
	IMPORT_C TInt SetSizingAttributes(TUint aMaxBufs, TUint aGrowTriggerRatio,
										TUint aGrowByRatio, TUint aShrinkHysteresisRatio);

	/**
	Ensures that each buffer is mapped into at most one process address space
	at a time.

	If this is set for a non-page-aligned pool, the pool creation will fail.
	*/
	IMPORT_C TInt SetExclusive();

	/**
	Specifies that unmapped guard pages are inserted between each buffer in a process's
	address space.

	If this is set for a non-page-aligned pool, the pool creation will fail.
	*/
	IMPORT_C TInt SetGuardPages();

private:
	friend class RShPool;
	TShPoolCreateInfo();		// hide the default constructor

	TShPoolInfo iInfo;
};


/**
A handle to a pool of buffers.

Pools can be created by either user-side or kernel-side components.

Upon receiving a pool handle the recipient must map either whole or part of the pool
memory into its address space.

When finished with the pool, the recipient must Close() it: this invalidates the handle.
Any further attempt to use it will panic the thread.

All pools are reference-counted and will only disappear after all users have closed them.

These handles are process-relative.

@publishedPartner
@prototype
*/
class RShPool : public RHandleBase
	{
public:
	IMPORT_C RShPool();

	/**
	Creates a pool of buffers with the required attributes and maps the memory to this process.
	The RShPool object is changed by this call (iHandle is set).

	@param aBufInfo		A structure describing the parameters of the pool to be created.
						See TShPoolCreateInfo.

	@param aFlags		Flags to modify the behaviour of the handle.  This should be a bit-wise
						OR of values from TShPoolHandleFlags.

	@return the handle (>= 0) if successful, otherwise one of the system-wide error codes.

	@see TShPoolCreateInfo
	@see TShPoolClientFlags
	*/
	IMPORT_C TInt Create(const TShPoolCreateInfo& aBufInfo, TUint aFlags);

	/**
	Retrieves information about the pool.

	@param aInfo returns pool info.
	*/
	IMPORT_C void GetInfo(TShPoolInfo& aInfo) const;

	/**
	Specifies how many buffers of a page-aligned pool this process will require
	concurrent access to.

	This determines how much of the process's address space will be allocated for
	buffers of this pool.

	@param aCount		Specifies the number of buffers to map into the process's virtual address space
						(-1 specifies that all buffers will be mapped).

	@param aAutoMap		ETrue specifies that an allocated or received buffer
	                    will be automatically mapped into the process's address space.

	@pre the pool's buffers must be page-aligned

	@return KErrNone if successful, otherwise one of the system-wide error codes.
	*/
	IMPORT_C TInt SetBufferWindow(TInt aCount, TBool aAutoMap);

	/**
	@return the number of free buffers in the pool
	*/
	IMPORT_C TUint FreeCount() const;

	/**
	Requests a notification when the number of free buffers in the pool falls to the
	level specified.

	@param aFreeBuffers	Specifies the number of free buffers that will trigger completion of the notification.
	@param aStatus		Status request to be completed when the condition is met.
	*/
	IMPORT_C void RequestLowSpaceNotification(TUint aFreeBuffers, TRequestStatus& aStatus);

	/**
	Requests a notification when the number of free buffers in the pool rises to the
	level specified.

	@param aFreeBuffers	Specifies the number of free buffers that will trigger completion of the notification.
	@param aStatus		Status request to be completed when the condition is met.
	*/
	IMPORT_C void RequestFreeSpaceNotification(TUint aFreeBuffers, TRequestStatus& aStatus);

	/**
	Cancels a low space notification request.

	The status request completes with KErrCancel.

	@param aStatus		The status request whose notification is to be cancelled.
	*/
	IMPORT_C void CancelLowSpaceNotification(TRequestStatus& aStatus);

	/**
	Cancels a free space notification request.

	The status request completes with KErrCancel.

	@param aStatus		The status request whose notification is to be cancelled.
	*/
	IMPORT_C void CancelFreeSpaceNotification(TRequestStatus& aStatus);
	};


/**
A handle to a shared buffer within a pool.

A user-side or kernel-side component allocates the buffer and then passes the
handle to the recipient.

Upon receiving a buffer handle. the recipient may map the buffer memory into its address space if not already done so.

When finished with the buffer, the recipient must Close() the handle.  This
invalidates the handle; any further attempt to use it will panic the thread.

Buffers are reference-counted and will only be freed and returned to the pool
when all users have closed them.

These handles are process-relative.

@publishedPartner
@prototype
*/
class RShBuf  : public RHandleBase
	{
public:
	inline RShBuf() : iBase(0), iSize(0) {};

	IMPORT_C void Close();

	/**
	Allocates a shared data buffer.

	By default this method will return immediately with KErrNoMemory if no buffer is
	available on the pool's free list, even if the pool could grow automatically.

	By default it will also map the allocated buffer into the calling process's address space.

	Setting EShPoolAllocCanWait in the flags indicates that the caller is prepared to
	wait while the pool is grown if a buffer is not immediately available on the free list.

	Setting EShPoolAllocNoMap in the flags indicates that the caller does not want the
	buffer to be automatically mapped into its address space.  This can improve performance
	on buffers from page-aligned pools if the caller will not need to access the data in the
	buffer (e.g. if it will just be passing it on to another component).  This only prevents
	mapping if the pool is set to not automatically map buffers into processes' address space.

	@param aPool	Pool handle
	@param aFlags	Bitwise OR of values from TShPoolAllocFlags to specify non-default behaviour.

	@return KErrNone if successful, otherwise one of the system-wide error codes.

	@see TShPoolAllocFlags
	*/
	IMPORT_C TInt Alloc(RShPool& aPool, TUint aFlags = 0);

	/**
	Maps the buffer into the calling process's virtual address space if not already done.

	It is not necessary to call this method on buffers from non-page-aligned pools, although
	it will cause no harm to do so (and will result in KErrNone being returned).

	It is not necessary to call this method on buffers from page-aligned pools returned by
	Alloc() unless EShPoolAllocNoMap was specified.

	@param aReadOnly	Indicates whether the buffer should be mapped as read-only.  The default is
	                    that of pool in the clients address space.

	@return KErrNone if successful, otherwise one of the system-wide error codes.
	*/
	IMPORT_C TInt Map(TBool aReadOnly = EFalse);

	/**
	Unmaps the buffer from the calling process's virtual address space.

	@return KErrNone if successful, otherwise one of the system-wide error codes.
	*/
	IMPORT_C TInt UnMap();

	/**
	@return The size, in bytes, of this buffer (and every other buffer in the pool).
	*/
	IMPORT_C TUint Size();

	/**
	@return A pointer to the start of the buffer.
	*/
	IMPORT_C TUint8* Ptr();

private:
	friend class RShPool;
	TLinAddr iBase;
	TUint iSize;
	TUint iSpare1;
	TUint iSpare2;
	};


#endif	// E32SHBUF_H
