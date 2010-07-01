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
//   Handles pre-cleaning of dirty pages.
//
//   When the paging device is idle (as determined by it calling NotifyIdle/NotifyBusy), a thread
//   writes cleans dirty pages in the oldest section of the live list.
//

/**
 @file
 @internalComponent
*/

#ifndef MPAGECLEANER_H
#define MPAGECLEANER_H

#include <e32def.h>
#include <nkern.h>

class PageCleaner
	{
public:
	static void Start();
	static void NotifyPagesToClean();
	};

#endif
