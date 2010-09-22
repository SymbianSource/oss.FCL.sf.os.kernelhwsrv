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
// e32\debug\crashMonitor\src\scmmulticrashinfo.cpp
// Class to store info about the crash flash to enable multiple crashes
// 
//

/**
 @file
 @internalTechnology
*/

#include <scmdatatypes.h>

namespace Debug
	{
	/**
	 * constructor
	 */
	SCMMultiCrashInfo::SCMMultiCrashInfo()
		: iFirstBlock(NULL)
		, iCurrentBlock(NULL)
		{	
		}
		
	/** 
	 * destructor 
	 */
	SCMMultiCrashInfo::~SCMMultiCrashInfo()
		{
		ClearList();
		}
		
	/** add a pointer to a block to the list - takes ownership of block
	 * @param SCMCrashBlockEntry* aBlockEntry block to add
	 */ 
	void SCMMultiCrashInfo::AddBlock(SCMCrashBlockEntry* aBlockEntry)
		{	
		if(aBlockEntry)
			{
			CLTRACE4("SCMMultiCrashInfo::AddBlock iBlockOffset = [%d] [0x%X] iBlockSize = [%d] [0x%X]"
				, aBlockEntry->iBlockOffset,aBlockEntry->iBlockOffset, aBlockEntry->iBlockSize, aBlockEntry->iBlockSize);
			if(!iFirstBlock)
				{
				// adding to empty list
				iFirstBlock = aBlockEntry;
				iCurrentBlock = iFirstBlock;
				}
			else
				{
				SCMCrashBlockEntry* p = iFirstBlock;	
				while(p->iNext)
					{
					p = p->iNext;
					}
				p->iNext = aBlockEntry;
				}
			}
		else
			{
			CLTRACE("SCMMultiCrashInfo::AddBlock Adding a NULL block !");	
			}
		}
		
	
	/** add a pointer to a block to the list - takes ownership of block 
	 * @return SCMCrashBlockEntry* returns NULL when no more blocks
	 * @param none
	 */ 
	SCMCrashBlockEntry* SCMMultiCrashInfo::GetNextBlock()
		{
		SCMCrashBlockEntry* p = iCurrentBlock;
		if(iCurrentBlock)
			{
			iCurrentBlock = iCurrentBlock->iNext;
			}
		return p; 
		}
	
	/** 
	 * sets current block to first in list 
	 */
	void SCMMultiCrashInfo::Reset()
		{
		iCurrentBlock = iFirstBlock;
		}
		
	/**
	 * Clears all entries in the list 
	 */
	void SCMMultiCrashInfo::ClearList()
		{
		SCMCrashBlockEntry* p = iFirstBlock;
			
		while(p)
			{
			SCMCrashBlockEntry* tmp = p;
			p = p->iNext;
			delete tmp;
			}
		
		iFirstBlock = iCurrentBlock = NULL;
		}
	}

//eof

