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
// e32test\demandpaging\t_dpcmn.h
// 
//

extern TInt gPageSize;
extern TBool gPagingSupported;
extern TBool gRomPagingSupported;
extern TBool gCodePagingSupported;
extern TBool gDataPagingSupported;
extern TInt gDataPagingPolicy;
extern TBool gProcessPaged;

extern TBool gGlobalNoPaging;		
extern TBool gGlobalAlwaysPage;
extern TBool gGlobalDefaultUnpaged;	
extern TBool gGlobalDefaultPage;

// Size of paging cache in pages
extern TUint gMinCacheSize;
extern TUint gMaxCacheSize;
extern TUint gCurrentCacheSize;

extern RChunk gChunk;
extern RTest test;

TInt GetGlobalPolicies();
TBool IsDataPagingSupported();
void UpdatePaged(TBool& aPaged);
TInt TestThreadExit(RThread& aThread, TExitType aExitType, TInt aExitReason);

class TestHybridHeap : public RHeap
	{
public: TInt ChunkHandle() { return iChunkHandle; };
	};
