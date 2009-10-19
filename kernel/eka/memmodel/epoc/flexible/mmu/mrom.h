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

#ifndef MROM_H
#define MROM_H


#ifdef __SUPPORT_DEMAND_PAGING_EMULATION__
extern void RomOriginalPages(TPhysAddr*& aPages, TUint& aPageCount);
#endif

extern TBool IsUnpagedRom(TLinAddr aBase, TUint aSize);

#endif
