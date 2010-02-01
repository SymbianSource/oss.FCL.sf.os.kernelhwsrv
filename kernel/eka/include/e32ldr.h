// Copyright (c) 1995-2009 Nokia Corporation and/or its subsidiary(-ies).
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
// e32\include\e32ldr.h
// 
// WARNING: This file contains some APIs which are internal and are subject
//          to change without notice. Such APIs should therefore not be used
//          outside the Kernel and Hardware Services package.
//

/**
 @file
 @internalTechnology
*/

#ifndef __E32LDR_H__
#define __E32LDR_H__
#include <e32cmn.h>


/**
	@internalTechnology

	Where sections of a file are located on the media.
	The kernel uses this to load in parts of a demand paged file.
 */
class TBlockMapEntryBase
	{
public:
	TUint iNumberOfBlocks;  // Number of contiguous blocks in map.
	TUint iStartBlock;		// Number for first block in the map.
	};


/**
	@internalTechnology

	Describes context for TBlockMapEntryBase objects.
 */
struct SBlockMapInfoBase
	{
	TUint iBlockGranularity;	// Size of a block in bytes.
	TUint iBlockStartOffset;	// Offset to start of the file or requested file position within a block.
	TInt64 iStartBlockAddress;	// Address of the first block of the partition.
	TInt iLocalDriveNumber;		// Local drive number of where the file lies on.
	};


#ifndef SYMBIAN_ENABLE_SPLIT_HEADERS
#include <e32ldr_private.h>
#endif

#endif // __E32LDR_H__

