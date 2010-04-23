// Copyright (c) 1997-2009 Nokia Corporation and/or its subsidiary(-ies).
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
// e32test\heap\t_hcomp.cpp
// Causes lots of chunk compression without using much memory
// Won't work on WINS unless memory allocation limits are set.
// 
//


#include <e32std.h>
#include <e32std_private.h>
#include <e32hal.h>

const TUint size64k = 64*1024;

GLDEF_C TInt E32Main()
	{

	RProcess me;
	me.SetPriority(EPriorityHigh);

	TMemoryInfoV1Buf membuf;
	UserHal::MemoryInfo(membuf);
	TInt maxmem=membuf().iTotalRamInBytes;
	TInt heapsize=maxmem*2;

	RHeap* heap1=User::ChunkHeap(NULL,size64k,heapsize+size64k);
	if (heap1==NULL)
		return KErrNoMemory;

	// Need this horrible construct because RVCT4 complains that the return is
	// unreachable code.  Without it, though, other code analysers will complain
	// that there is a missing return value!	
	volatile TInt forever = 1;
	
	while(forever)
		{
		TUint8* ptr=(TUint8*)heap1->Alloc(heapsize);	// fail, compress, fail
		User::After(1000);	// quite soon
		heap1->Free(ptr);
		}
	return 0;
	}
