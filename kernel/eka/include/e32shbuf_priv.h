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
// e32/include/e32shbuf_priv.h
// Shareable Data Buffers

/**
	@file
	@internalComponent
	@prototype
*/

#ifndef E32SHBUF_PRIV_H
#define E32SHBUF_PRIV_H


#include <e32def.h>


/**
	Defines flags for TShPoolInfo.

	These values specify the behaviour of the pool and its buffers.

	@internalComponent
	@prototype
*/
enum TShPoolCreateFlags
	{
	/**
	The buffers in this pool are at least the size of an MMU page,
	are aligned to an MMU page, and may be mapped in and out of a process's
	address space independently.
	*/
	EShPoolPageAlignedBuffer	= 0x0001,

	/**
	The buffers in this pool do not have any size or alignment restrictions beyond
	that specified by the pool creator.

	The whole pool is always mapped into a process's address space.
	*/
	EShPoolNonPageAlignedBuffer = 0x0002,

	/**
	This pool maps onto device memory, not system RAM.
	*/
	EShPoolPhysicalMemoryPool	= 0x0004,

	/**
	The physical memory backing this pool is contiguous.
	*/
	EShPoolContiguous			= 0x0008,

	/**
	Each buffer will only ever be mapped into one process's address space at a time.

	Requires EShPoolPageAlignedBuffer.
	*/
	EShPoolExclusiveAccess		= 0x0010,

	/**
	An unmapped page will be placed between each buffer.

	Requires EShPoolPageAlignedBuffer.
	*/
	EShPoolGuardPages			= 0x0020,

	/**
	Set by automatic shrinking when it is unable to shrink the pool despite there
	being enough free buffers available (usually due to fragmentation).  This prevents
	the automatic shrinking code being continually called when it can't do anything.
	*/
	EShPoolSuppressShrink		= 0x80000000,
	};


/**
	Specifies client flags.

	@internalComponent
	@prototype
*/
enum TShPoolClientFlags
	{
	/**
	Buffers will be automatically mapped into this process's address space when a handle is
	created for them.  Having this flag clear can be useful for a process that will function
	as an intermediary, without requiring access to the data in the buffers.
	*/
	EShPoolAutoMapBuf		= 0x1000,

	/**
	Newly-allocated buffers will not be mapped into this process's address space.
	*/
	EShPoolNoMapBuf			= 0x2000,
	};


/**
	Specifies the type of notification.  (A real enumeration, not bit flags.)

	@internalComponent
	@prototype
*/
enum TShPoolNotifyType
	{
	EShPoolLowSpace = 1,		// notifies when free buffers drop to a certain number
	EShPoolFreeSpace = 2,		// notifies when free buffers rise to a certain number
	};

/**
	Structure for passing base address and pointer of buffer

	@internalComponent
	@prototype
*/
struct SShBufBaseAndSize
	{
	TLinAddr iBase;
	TUint    iSize;
	};


#endif	// E32SHBUF_PRIV_H
