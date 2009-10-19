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
//

/**
 @file
 @internalComponent
*/

#ifndef MPDALLOC_H
#define MPDALLOC_H


class PageDirectoryAllocator
	{
public:
	void Init2();
	TInt Alloc(TUint aOsAsid, TPhysAddr& aPageDirectory);
	void Free(TUint aOsAsid);
	void GlobalPdeChanged(TPde* aPde);
private:
	void AssignPages(TUint aIndex, TUint aCount, TPhysAddr aPhysAddr);
private:
	TPhysAddr iKernelPageDirectory;
	DMemoryObject* iPageDirectoryMemory;
	TBitMapAllocator* iAllocator;
	};

extern PageDirectoryAllocator PageDirectories;


#endif
