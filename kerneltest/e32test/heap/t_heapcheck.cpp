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
// e32test\debug\t_heapcorruption.cpp
// This is a test application that will cause heap corruption 
// to generate BTrace events (EHeapCorruption).
// 
//

//  Include Files
#include <e32test.h>
#include <e32base.h>
#include <e32panic.h>
#include <e32cmn.h>
#include "dla.h"
#include "slab.h"
#include "page_alloc.h"
#include "heap_hybrid.h"

LOCAL_D RTest test(_L("T_HEAPCHECK"));

TUint32 gSeed = 0xb504f334;

_LIT(KLitHeapCheck,"Heap Check");


TUint32 Random()
{
	gSeed *= 69069;
	gSeed += 41;
	return gSeed;
}

TInt RandomNumber(TInt aMin, TInt aMax)
{
	TInt y = aMax - aMin;
	if ( y <= 0 )
		return aMax;
	TUint32 x = Random() & 0xff;
	TInt s = 0;
	while ( y > (0x100 << s) )
		{
		s++;
		}
	return (aMin + (x << s) % y);
}


/**
Friend class of RHeapHybrid to access to hybrid heap metadata
*/
class TestHybridHeap
{
	public:
		TBool Init();
		TBool Check();
		TUint8* Alloc(TInt aLth);
		TUint8* ReAlloc(TAny* aBfr, TInt aLth, TInt aMode);
		void   Free(TAny* aBfr);
		TInt  AllocLen(TAny* aBfr);
		TInt  AllocSize(TInt& aTotalAllocSize);						
		TBool SlabAllocatorExists();
		TBool PageAllocatorExists();			   
		TBool SlabsCreated();
		TBool CorruptSmallBin();
		TBool CorruptTreeBin();
		TBool ConfigurePageAllocator();
		TInt  CopyPageBitmap(TUint8* aBitmap, TInt aLth);
		TBool RestorePageBitmap(TUint8* aBitmap, TInt aLth);
		void AllocateSomeBuffers(TUint8** aBfrs, TInt aMinLth, TInt MaxLth, TInt aCount);
		TBool PrintHeapInitData();		

	private:
		RHybridHeap* iHybridHeap;
};



TBool TestHybridHeap::Init()
{
	RHybridHeap::STestCommand cmd;
	cmd.iCommand = RHybridHeap::EHeapMetaData;
	RAllocator& heap = User::Allocator();
	TInt ret = heap.DebugFunction(RHeap::EHybridHeap, &cmd, 0);
	if (ret != KErrNone)
		return EFalse;
	iHybridHeap = (RHybridHeap*) cmd.iData;
	
	return ETrue;
}

TBool TestHybridHeap::Check()
{
	if ( iHybridHeap )
		{
		iHybridHeap->Check();  
		}

	return EFalse;
}

TUint8* TestHybridHeap::Alloc(TInt aLth)
{
	if ( iHybridHeap )
		{
		return (TUint8*)iHybridHeap->Alloc(aLth);  
		}

	return NULL;
}

TUint8* TestHybridHeap::ReAlloc(TAny* aBfr, TInt aLth, TInt aMode)
{
	if ( iHybridHeap )
		{
		return (TUint8*)iHybridHeap->ReAlloc(aBfr, aLth, aMode);  
		}

	return NULL;
}

void TestHybridHeap::Free(TAny* aBfr)
{
	if ( iHybridHeap )
		{
		iHybridHeap->Free(aBfr);  
		}
}

TInt TestHybridHeap::AllocLen(TAny* aBfr)
{
	if ( iHybridHeap )
		{
		return iHybridHeap->AllocLen(aBfr);  
		}
	return 0;
}

TInt TestHybridHeap::AllocSize(TInt& aTotalAllocSize)
{
	aTotalAllocSize = 0;
	if ( iHybridHeap )
		{
		return iHybridHeap->AllocSize(aTotalAllocSize);  
		}
	return 0;
}

TBool TestHybridHeap::SlabAllocatorExists()
{
	TBool status = EFalse;
	if ( iHybridHeap )
		{
		status = !iHybridHeap->iDLOnly;
		}
	
	return status;
}

TBool TestHybridHeap::PageAllocatorExists()
{
	TBool status = EFalse;
	if ( iHybridHeap )
		{
		status = (!iHybridHeap->iDLOnly && (iHybridHeap->iPageThreshold < 31));
		}

	return status;
}

TBool TestHybridHeap::SlabsCreated()
{
	TBool status = EFalse;
	if ( iHybridHeap )
		{
		status = (iHybridHeap->iSlabThreshold != 0);
		}

	return status;
}

TBool TestHybridHeap::ConfigurePageAllocator()
{
	TBool status = EFalse;
	if ( iHybridHeap )
		{
		RHybridHeap::STestCommand conf;
		conf.iCommand = RHybridHeap::ESetConfig;
		conf.iConfig.iPagePower = 14;  // 16 Kb		
		if ( iHybridHeap->DebugFunction(RHeap::EHybridHeap, (TAny*)&conf ) == KErrNone )
			status = ETrue;
		}

	return status;
}


TBool TestHybridHeap::CorruptTreeBin()
{
	TBool status = EFalse;
	if ( iHybridHeap )
		{
		TUint i;
		for (i = 0; i < NTREEBINS; ++i)
			{
			tbinptr* tb = TREEBIN_AT(&iHybridHeap->iGlobalMallocState, i);
			tchunkptr t = *tb;
			if ( t )
				{
				// Corrupt tree bin by writing erroneous index value
				t->iIndex ++;
				return ETrue;
				}
			}
		}

	return status;
}

TBool TestHybridHeap::CorruptSmallBin()
{
	TBool status = EFalse;
	if ( iHybridHeap )
		{
		TUint i;
		for (i = 0; i < NSMALLBINS; ++i)
			{
			sbinptr b = SMALLBIN_AT(&iHybridHeap->iGlobalMallocState, i);
			mchunkptr p = b->iBk;
			if ( p != b )
				{ 
				b->iBk = b;
				status = ETrue;
				}
			}
		}

	return status;
}

TInt TestHybridHeap::CopyPageBitmap(TUint8* aBitmap, TInt aLth)
{
	TInt lth = 0;
	if ( iHybridHeap && (aLth > (TInt) sizeof(iHybridHeap->iBitMapBuffer)) )
		{// Dirty version
		memcpy(aBitmap, &iHybridHeap->iBitMapBuffer[0], sizeof(iHybridHeap->iBitMapBuffer));
        lth = sizeof(iHybridHeap->iBitMapBuffer) << 3;
		}

	return lth;
}

TBool TestHybridHeap::RestorePageBitmap(TUint8* aBitmap, TInt aLth)
{
	TBool status = EFalse;
	if ( iHybridHeap && ((aLth >> 3) <= (TInt) sizeof(iHybridHeap->iBitMapBuffer)) )
		{// Dirty version
		memcpy(&iHybridHeap->iBitMapBuffer[0], aBitmap, (aLth >> 3));
		status = ETrue;
		}

	return status;
}

void TestHybridHeap::AllocateSomeBuffers(TUint8** aBfrs, TInt aMinLth, TInt MaxLth, TInt aCount )
{
	
	TInt loop = RandomNumber(2, 8);

	while ( loop )
		{
		// allocate all buffers
		TInt i;
		for (i=0; i<aCount; ++i)
			{
			if (!aBfrs[i])
				{
				aBfrs[i] = (TUint8*)Alloc(RandomNumber(aMinLth, MaxLth));
				}
			}

		// free some cells
		TInt n = RandomNumber(2, aCount);
		while (--n)
			{
			i = RandomNumber(2, aCount);
			if (aBfrs[i])
				{
				Free(aBfrs[i]);
				aBfrs[i] = NULL;
				}
			}

		// realloc some cells
		n = RandomNumber(2, aCount);
		while (--n)
			{
			TInt new_len = RandomNumber(aMinLth, MaxLth);
			if (aBfrs[i])
				{
				TUint8* p = (TUint8*)ReAlloc(aBfrs[i], new_len, Random());
				if (p)
					{
					aBfrs[i] = p;
					}
				}
			}

		loop --;
		}

}	

TBool TestHybridHeap::PrintHeapInitData()
{
	TInt total;
	TInt count = AllocSize(total);
	RDebug::Printf("Heap initialised for test, alloc count: %d , alloc size: %d\n", count, total);
	if ( iHybridHeap )
		RDebug::Printf("Heap initialised for test, iCellCount: %d , iTotalAllocSize: %d\n", iHybridHeap->iCellCount, iHybridHeap->iTotalAllocSize);	    	
	return (count != 0);
}


//  Local Functions
LOCAL_D TInt HeapCheckTestThread(TAny* param)
{
	TInt t = *((TInt*)param);
	TUint8* bfrs[256];
	Mem::FillZ(bfrs, sizeof(bfrs));
	TestHybridHeap heap;
	test(heap.Init());

	switch( t )
		{
		case 1:
			{
			// Overwrite Doug Lea buffer and check()
			heap.AllocateSomeBuffers(bfrs, 0x40, 0xfff0, 256);
			test(heap.PrintHeapInitData());
			TUint8 *p = heap.Alloc(64);
			test( p != NULL );
			Mem::FillZ(p, 80);  // Heap corrupted
			heap.Check();    // This should cause panic
			break;
			}

		case 2:
			// Corrupt a smallbin and check
			{
			TInt i = 0;
			TBool smallbin_corrupted = EFalse;
			while ( !smallbin_corrupted )
				{
				heap.AllocateSomeBuffers(bfrs, 0x4, 0xff, 256);
				smallbin_corrupted = heap.CorruptSmallBin();
				i ++;
				if ( i > 9 )
					break;
				}
			test(smallbin_corrupted);
    		test(heap.PrintHeapInitData());
			heap.Check();    // This should cause panic
			}
			break;

		case 3:
			// Corrupt a treebin and check
			{
			TInt i = 0;
			TBool treebin_corrupted = EFalse;
			while ( !treebin_corrupted )
				{
				heap.AllocateSomeBuffers(bfrs, 0x100, 0x4000, 256);
				treebin_corrupted = heap.CorruptTreeBin();
				i ++;
				if ( i > 9 )
					break;
				}
			test(treebin_corrupted);
			test(heap.PrintHeapInitData());						
			heap.Check();    // This should cause panic
			break;
			}

		case 10:
			// Overwrite slab buffer and check
			{
			TInt i = 0;
			TBool slabs_created = EFalse;
			if ( !heap.SlabAllocatorExists() )
				{
				User::Panic(KLitHeapCheck, ETHeapDebugUnmatchedCallToCheckHeap);
				}

			while ( !slabs_created )
				{
				// Allocate enough buffers to cause slab allocator to be
				// initialised  		
				heap.AllocateSomeBuffers(bfrs, 0x4, 0x2000, 256);
				slabs_created = heap.SlabsCreated();
				i ++;
				if ( i > 9 )
					break;
				}
			test(slabs_created);
			test(heap.PrintHeapInitData());						
			i = 0;
			TUint8* p[10];
			while ( i < 10 )
				{
				p[i] = heap.Alloc(24);
				test( p[i] != NULL );
				i ++;
				}
			i = 0;
			while ( i < 10 )
				{
				heap.Free(p[i]);	
				i +=2;
				}
			p[0] = heap.Alloc(24);
			test( p[0] != NULL );
			memset((TUint8*)(Floor(p[0], SLABSIZE) + sizeof(slabhdr)), 0xee, KMaxSlabPayload);  // Heap corrupted
			heap.Check();         // This should cause panic
			break;
			}

		case 11:
			// Corrupt slab header
			{
			TInt i = 0;
			TBool slabs_created = EFalse;
			if ( !heap.SlabAllocatorExists() )
				{
				User::Panic(KLitHeapCheck, ETHeapDebugUnmatchedCallToCheckHeap);
				}

			while ( !slabs_created )
				{
				// Allocate enough buffers to cause slab allocator to be
				// initialised  		
				heap.AllocateSomeBuffers(bfrs, 0x4, 0x2000, 256);
				slabs_created = heap.SlabsCreated();
				i ++;
				if ( i > 9 )
					break;
				}
			test(slabs_created);
			test(heap.PrintHeapInitData());						
			TUint8* p = heap.Alloc(28);
			test(p != NULL);
			p = Floor(p, SLABSIZE);
			*(TUint32*)p = 0xffeeddcc;
			heap.Check();      // This should cause panic
			break;
			}

		case 20:
			// Corrupt page bitmap data and check
			{
			if ( !heap.PageAllocatorExists() )
				{
				User::Panic(KLitHeapCheck, ETHeapDebugUnmatchedCallToCheckHeap);
				}
			test(heap.ConfigurePageAllocator());
			// Allocate some buffers to cause slab allocator to be
			// initialised  		
			heap.AllocateSomeBuffers(bfrs, 0x4000, 0x10000, 16);
			test(heap.PrintHeapInitData());						
			TUint8* bitmap = heap.Alloc(128);  // For saved bitmap
			test(bitmap != NULL);
			TInt bit_lth = heap.CopyPageBitmap(bitmap, 128);
			test(bit_lth != 0);
			memset(bitmap, 0xee, (bit_lth>>3));  //  corrupt bitmap data
			heap.RestorePageBitmap(bitmap, bit_lth);
			heap.Check();      // This should cause panic
			break;
			}

		case 21:
			// Corrupt page bitmap with a earlier freed "ghost" buffer info
			{
			if ( !heap.PageAllocatorExists() )
				{
				User::Panic(KLitHeapCheck, ETHeapDebugUnmatchedCallToCheckHeap);
				}
			test(heap.ConfigurePageAllocator());
			// Allocate some buffers to cause slab allocator to be
			// initialised  		
			heap.AllocateSomeBuffers(bfrs, 0x4000, 0x10000, 16);
			test(heap.PrintHeapInitData());						
			TUint8* bitmap = heap.Alloc(128);  // For saved bitmap
			test(bitmap != NULL);
			TUint8* p = heap.Alloc(0x8000);     // One more page buffer
			TInt bit_lth = heap.CopyPageBitmap(bitmap, 128);
			test(bit_lth != 0);
			heap.Free(p);
			heap.RestorePageBitmap(bitmap, bit_lth);
			heap.Check();      // This should cause panic
			break;
			}

		default:
			break;
		}

	User::Invariant();	// Should not reach here 
	return 0;
}


class TestHeapCheck
{
	public:
		void TestCheck(void);
		TInt TestThreadExit(RThread& aThread, TExitType aExitType, TInt aExitReason);
};


TInt TestHeapCheck::TestThreadExit(RThread& aThread, TExitType aExitType, TInt aExitReason)
{
	// Disable JIT debugging.
	TBool justInTime=User::JustInTime();
	User::SetJustInTime(EFalse);

	TRequestStatus status;
	aThread.Logon(status); 
	aThread.Resume();
	User::WaitForRequest(status);
	if (aExitType != aThread.ExitType())
		return KErrGeneral;

	if ( (status.Int() == ETHeapDebugUnmatchedCallToCheckHeap) && (aThread.ExitReason() == ETHeapDebugUnmatchedCallToCheckHeap))
		{
		CLOSE_AND_WAIT(aThread);
		// Put JIT debugging back to previous status.
		User::SetJustInTime(justInTime);
		return KErrNotSupported;
		}

	if ( status.Int() == ERTestFailed )
		return KErrGeneral;

	if ( aExitReason > 0 )
		{
		if (aExitReason != status.Int())
			return KErrGeneral;

		if (aExitReason != aThread.ExitReason())
			return KErrGeneral;		
		}
	
	CLOSE_AND_WAIT(aThread);

	// Put JIT debugging back to previous status.
	User::SetJustInTime(justInTime);
	return KErrNone;

}

void TestHeapCheck::TestCheck()
{
    TInt type;
	TInt r;
	
	test.Next(_L("Testing Doug Lea allocator check"));
	{
	type = 1;
	RThread thread;
	test(thread.Create(_L("Check UserHeap"),HeapCheckTestThread, KDefaultStackSize, 0x1000, 0x400000,  (TAny*) &type)== KErrNone);
	test(TestThreadExit(thread, EExitPanic, ETHeapBadCellAddress)==KErrNone);
	
    type = 2;
    test(thread.Create(_L("Check UserHeap"),HeapCheckTestThread, KDefaultStackSize, 0x1000, 0x400000,  (TAny*) &type)==KErrNone);
    test(TestThreadExit(thread, EExitPanic, ETHeapBadCellAddress)==KErrNone);

	type = 3;
	test(thread.Create(_L("Check UserHeap"),HeapCheckTestThread, KDefaultStackSize, 0x1000, 0x400000,  (TAny*) &type)==KErrNone);
	test(TestThreadExit(thread, EExitPanic, ETHeapBadCellAddress)==KErrNone);
	
	}

	test.Next(_L("Testing Slab allocator check"));	
	{
	type = 10;
	RThread thread;
	test(thread.Create(_L("Check UserHeap"),HeapCheckTestThread, KDefaultStackSize, 0x1000, 0x400000,  (TAny*) &type)==KErrNone);
	r = TestThreadExit(thread, EExitPanic, ETHeapBadCellAddress);
	if ( r != KErrNotSupported )
		{
		test(r==KErrNone);
		
		type = 11;
		RThread thread;
		test(thread.Create(_L("Check UserHeap"),HeapCheckTestThread, KDefaultStackSize, 0x1000, 0x400000,  (TAny*) &type)==KErrNone);
		test(TestThreadExit(thread, EExitPanic, ETHeapBadCellAddress)==KErrNone);
		}
	else test.Printf(_L("Slab allocator does not exist, testes bypassed\n"));	
	}

	test.Next(_L("Testing Page allocator check"));	
	{
	type = 20;
	RThread thread;
	test(thread.Create(_L("Check UserHeap"),HeapCheckTestThread, KDefaultStackSize, 0x1000, 0x800000,  (TAny*) &type)==KErrNone);	
	r = TestThreadExit(thread, EExitPanic, KErrNone);   // Accept any panic reason here
	if ( r != KErrNotSupported )
		{
		test(r==KErrNone);

		type = 21;
		RThread thread;
		test(thread.Create(_L("Check UserHeap"),HeapCheckTestThread, KDefaultStackSize, 0x1000, 0x800000,  (TAny*) &type)==KErrNone);
		test(TestThreadExit(thread, EExitPanic,	KErrNone)==KErrNone);  // Accept any panic reason here
		}
    else test.Printf(_L("Page allocator does not exist, testes bypassed\n"));
	}

}




//  Global Functions

GLDEF_C TInt E32Main(void)
	{

	test.Title();
	
	test.Start(_L("Testing Heap Check function"));
	
	TestHeapCheck T;
	
	T.TestCheck();

	test.End();
	
	return(0);
	}

