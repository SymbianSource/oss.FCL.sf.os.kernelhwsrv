// Copyright (c) 2006-2009 Nokia Corporation and/or its subsidiary(-ies).
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
// e32test\mmu\paging_info.h
// 
//

#ifndef __PAGING_INFO_H__
#define __PAGING_INFO_H__

class PagingInfo
	{
public:
	static TInt ResetConcurrency(TInt aLocDrvNo=-1, TMediaPagingStats aMediaStats=EMediaPagingStatsAll);
	static TInt PrintConcurrency(TInt aLocDrvNo=-1, TMediaPagingStats aMediaStats=EMediaPagingStatsAll);
	static TInt ResetEvents();
	static TInt PrintEvents();
	static TInt ResetBenchmarks(TInt aLocDrvNo=-1, TMediaPagingStats aMediaStats=EMediaPagingStatsAll);
	static TInt PrintBenchmarks(TInt aLocDrvNo=-1, TMediaPagingStats aMediaStats=EMediaPagingStatsAll);
	static TInt ResetAll(TInt aLocDrvNo=-1, TMediaPagingStats aMediaStats=EMediaPagingStatsAll);
	static TInt PrintAll(TInt aLocDrvNo=-1, TMediaPagingStats aMediaStats=EMediaPagingStatsAll);
	};

#endif
