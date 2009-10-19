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
// e32/include/e32shbufcmn.h
// Shareable Data Buffers

/**
	@file
	@publishedPartner
	@prototype
*/

#ifndef E32SHBUFCMN_H
#define E32SHBUFCMN_H

#include <e32shbuf_priv.h>
#include <e32def.h>

/**
	Defines values used to modify client behaviour when opening a pool or buffer in a new process.

	@publishedPartner
	@prototype
*/
enum TShPoolHandleFlags
	{
	/**
	The buffers will be writeable in the process where this handle is used.
	If not set, the buffers will be read-only.
	*/
	EShPoolWriteable		= 0x0001,

	/**
	The process will be able to allocate buffers from this pool.
	If not set, the process will not be able to allocate buffers from this pool.
	*/
	EShPoolAllocate			= 0x0002,
	};


/**
	Defines flags for RShBuf::Alloc() and DShPool::Alloc().

	@see RShBuf::Alloc()
	@see DShPool::Alloc()

	@publishedPartner
	@prototype
*/
enum TShPoolAllocFlags
	{
	/**
	The thread is willing to wait for a pool grow if no buffer is immediately available.
	If this is not set, the Alloc() will return immediately if no free buffer is available,
	but the pool might be able to grow to create new buffers.
	*/
	EShPoolAllocCanWait = 0x0001,

	/**
	Do not automatically map the newly-allocated buffer into this process, if the pool is page-aligned.
	(RShBuf::Alloc() only.)
	*/
	EShPoolAllocNoMap = 0x0002,
	};


/**
	Defines the attributes of a pool.

	@publishedPartner
	@prototype
*/
class TShPoolInfo
	{
public:
	IMPORT_C TShPoolInfo();

	/**
	Specifies the size of each buffer in the pool.
	*/
	TUint iBufSize;

	/**
	Specifies the initial number of buffers to be allocated to the pool.
	*/
	TUint iInitialBufs;

	/**
	Specifies the maximum number of buffers the pool can grow to.
	*/
	TUint iMaxBufs;

	/**
	This specifies when the pool grows.  If zero, the pool will not grow or shrink
	automatically.
  
	This is the proportion of free buffers left in the pool at which it should be grown.
	For example, if the ratio is 0.1, the pool will be grown when the number of free
	buffers drops to 10%.

	This value is expressed as a 32-bit fixed-point number, where the binary
	point is defined to be between bits 7 and 8 (where the least-significant
	bit is defined as bit 0).  (This format is also known as a Q8, or fx24.8
	number, or alternatively as the value * 256.)  For the example given of 10%,
	use the value 26.  It represents a value < 1 (i.e. must be < 256).  Calculations
	are rounded down towards zero, but if calculating how many buffers to trigger
	on gives 0, the grow will be triggered when 1 buffer is free.
	*/
	TUint iGrowTriggerRatio;

	/**
	This specifies the proportion by which to grow the pool each time it is grown.
	If zero, the pool will not grow or shrink automatically.

	It is expressed as a 32-bit fx24.8 fixed-point number, in the same way as
	iGrowTriggerRatio.  Calculations are rounded down towards zero, but if calculating
	how many buffers to grow by yields 0, then the pool will be grown by 1 buffer.
	*/
	TUint iGrowByRatio;

	/**
	The hysteresis value to ensure that the pool does not automatically shrink
	immediately after is grows.

	Automatic shrinking will only happen when there are (iGrowTriggerRatio +
	iGrowByRatio) * iShrinkHysteresisRatio * (total buffers in the pool) free
	buffers left in the pool.

	The amount by which the pool is shrunk depends on iGrowByRatio: it is the operational
	inverse, such that the pool would shrink down to the same number of buffers if shrunk
	immediately after growing (although hysteresis normally prevents this).

	For example, if iGrowByRatio is 10%, a pool of 100 buffers would grow to 110 buffers.
	To shrink back to 100, a shrink ratio of 10/110 = 9% is required.  That is, if the
	grow-by ration is G, the shrink-by ratio S is calculated as S = 1 - 1 / (1 + G).

	iShrinkHysteresisRatio is a 32-bit fx24.8 fixed-point number in the same way as
	iGrowTriggerRatio and iGrowByRatio.  It represents a value > 1 (i.e. must be > 256).

	@see iGrowByRatio
	*/
	TUint iShrinkHysteresisRatio;

	/**
	Specifies the alignment for each buffer, as a shift count (log2 bytes).

	For example, 9 means that each buffer is aligned on a 512-byte boundary.
	*/
	TUint iAlignment;

	/**
	Specifies flags for the pool, as bit values from TShPoolCreateFlags or-ed together.

	@see TShPoolCreateFlags
	*/
	TUint iFlags;
private:
	TInt iSpare1;			// Reserved for future use
	TInt iSpare2;
	TInt iSpare3;
	TInt iSpare4;
	};


#endif	// E32SHBUF_H

