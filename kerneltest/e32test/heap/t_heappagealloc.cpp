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
// e32test\heap\t_page_alloc.cpp
// Overview:
// Tests RHeap class.
// API Information:
// RHeap
// Details:
// - Tests that the page bitmap is consistent (i.e. encoded sizes are sensible and 
// encoded in the correct fashion.
// - Tests that pages which appear in the page bitmap are present in memory by 
// reading them.
// -Tests that other pages are not readable  
// - Tests page bitmap by creating an allocator where all allocations >= 4kB use
// paged allocator, allocating a large number of regions of various sizes (from
// 4 kB to b MB), checking that the walk function finds them all correctly, freeing
// some of them, checking the walk function again, and so on. 
// Platforms/Drives/Compatibility:
// All
// Assumptions/Requirement/Pre-requisites:
// Failures and causes:
// Base Port information:
// 
//

#include <e32test.h>
#include <e32hal.h>
#include <e32def.h>
#include <e32math.h>
#include <e32def_private.h>
#include "dla.h"
#include "slab.h"
#include "page_alloc.h"
#include "heap_hybrid.h"


struct TMetaData
    {
    TBool           iDLOnly;
    RFastLock*      iLock;
    TInt            iChunkSize;
    TInt            iSlabThreshold;
    unsigned        iSlabInitThreshold;
    unsigned        iSlabConfigBits;
    slab*           iPartialPage;
    slab*           iFullSlab;
    page*           iSparePage;
    TUint8*         iMemBase;
    unsigned char   iSizeMap[(MAXSLABSIZE>>2)+1];
    slabset         iSlabAlloc[MAXSLABSIZE>>2];
    slab**          iSlabAllocRealRootAddress[MAXSLABSIZE>>2];
    };

LOCAL_D RTest test(_L("T_HEAPPAGEALLOC"));

class TestHybridHeap
    {
public:
    static TUint8* MemBase(const RHybridHeap * aHybridHeap);
    static void GetHeapMetaData(RHeap& aHeap, TMetaData& aMeta);
    };

TUint8* TestHybridHeap::MemBase(const RHybridHeap * aHybridHeap)
	{
	return aHybridHeap->iMemBase;
	}

void TestHybridHeap::GetHeapMetaData(RHeap& aHeap, TMetaData& aMeta)
{
    RHybridHeap::STestCommand cmd;
    cmd.iCommand = RHybridHeap::EHeapMetaData;
    TInt ret = aHeap.DebugFunction(RHeap::EHybridHeap, &cmd, 0);
    test(ret == KErrNone);
    
    RHybridHeap* hybridHeap = (RHybridHeap*) cmd.iData;
    
    aMeta.iDLOnly              = hybridHeap->iDLOnly;
    aMeta.iLock                = &hybridHeap->iLock;
    aMeta.iChunkSize           = hybridHeap->iChunkSize;
    aMeta.iSlabThreshold       = hybridHeap->iSlabThreshold;
    aMeta.iSlabInitThreshold   = hybridHeap->iSlabInitThreshold;
    aMeta.iSlabConfigBits      = hybridHeap->iSlabConfigBits;
    aMeta.iPartialPage         = hybridHeap->iPartialPage;
    aMeta.iFullSlab            = hybridHeap->iFullSlab;
    aMeta.iSparePage           = hybridHeap->iSparePage;
    aMeta.iMemBase             = hybridHeap->iMemBase;

    TInt i;
    TInt count;
    count = sizeof(aMeta.iSizeMap)/sizeof(unsigned char);
    for (i=0; i<count; ++i)
        {
        aMeta.iSizeMap[i] = hybridHeap->iSizeMap[i];
        }
    count = sizeof(aMeta.iSlabAlloc)/sizeof(slabset);
    for (i=0; i<count; ++i)
        {
        aMeta.iSlabAlloc[i].iPartial = hybridHeap->iSlabAlloc[i].iPartial;
        aMeta.iSlabAllocRealRootAddress[i] = &hybridHeap->iSlabAlloc[i].iPartial;
        }
}

LOCAL_C void GetMeta(RHeap& aHeap, TMetaData& aMeta)
{
    TestHybridHeap::GetHeapMetaData(aHeap, aMeta);
}

class TestRHeap : public RHeap
	{
public:
	void InitTests();
	void Test1(void);
	void Test2(void);
	void Test3(void);
	void CloseTests();
	TUint GetRandomSize(TUint aMaxSize);
	TUint GetRandomIndex(TUint aMaxIndex);
	static void WalkCallback(TAny* aPtr, TCellType aType, TAny* aCell, TInt aLen);
	TBool CheckWalkArrayEmpty();
			
private:
	RHybridHeap* iHybridHeap;
	RHeap *iHeap;
	TUint8* iMemBase;			     // bottom of Paged/Slab memory (chunk base)
	static TUint iWalkArraySize;
	static TUint iWalkArrayIndex;
	static TAny** iWalkArrayOfCells;
	TUint iAllocatedArrayIndex;
	TAny** iAllocatedArrayOfCells;
	};

TUint TestRHeap::iWalkArraySize = 100;
TUint TestRHeap::iWalkArrayIndex = 0;
TAny** TestRHeap::iWalkArrayOfCells = new TAny*[iWalkArraySize];

void TestRHeap::InitTests()
{
    // Allocate a chunk heap
	TPtrC testHeap=_L("TESTHEAP");
	iHeap=User::ChunkHeap(&testHeap,0x1800,0x800000); 
	RHybridHeap::STestCommand cmd;
	cmd.iCommand = RHybridHeap::EHeapMetaData;
	iHeap->DebugFunction(RHeap::EHybridHeap, &cmd, 0);
	iHybridHeap = (RHybridHeap*) cmd.iData;
	iMemBase = TestHybridHeap::MemBase(iHybridHeap);
	
	// configure paged heap threshold 16 kB
	cmd.iCommand = RHybridHeap::ESetConfig;
	cmd.iConfig.iSlabBits = 0x0; //0xabe
	cmd.iConfig.iDelayedSlabThreshold = 0x40000000;
	cmd.iConfig.iPagePower = 14;
	test(iHeap->DebugFunction(RHeap::EHybridHeap, &cmd, 0) == KErrNone);
}


TUint TestRHeap::GetRandomSize(TUint aMaxSize)
{
	TUint size = 0;
	do
	{
		size = Math::Random() & aMaxSize;
	}
	while(size < 16384 ||  size > aMaxSize );
	// subtract debug header size	
	return size - 8;	
}


TUint TestRHeap::GetRandomIndex(TUint aMaxIndex)
{
	TUint index = 0;
	do
	    {
        index = Math::Random() & 0x7F;
	    }
    while(index >= aMaxIndex || iWalkArrayOfCells[index] == 0);

	return index;
}


void TestRHeap::WalkCallback(TAny* aPtr, TCellType aCellType, TAny* aBuffer, TInt aLen)
{
    if (aLen>16375 && aPtr>0)    // Don't test DL allocator
        test(aCellType == EGoodAllocatedCell);
    
	TUint i = 0;
	for(i=0; i<iWalkArrayIndex; i++)
	{
		if(iWalkArrayOfCells[i] == aBuffer)
		{
			iWalkArrayOfCells[i] = NULL;
			break;
		}
	}
}

TBool TestRHeap::CheckWalkArrayEmpty()
{
	TUint i = 0;
	for(i=0; i<iWalkArrayIndex; i++)
	{
		if(iWalkArrayOfCells[i])
		{
			return EFalse;
		}
	}
	return ETrue;
}


///////////////////////////////////////////////////////////
// Test page allocation with various sizes, 16 kB - 8 MB //
// Simple test with fixed sizes.                         //
///////////////////////////////////////////////////////////
void TestRHeap::Test1(void)
{
	// Allocate and free single paged buffers of different size
	// Small buffer
	TAny* p1 = NULL;
	p1=iHeap->Alloc(0x4000);
	test(p1 != NULL && p1 >= iMemBase && p1 < iHybridHeap);
	test(iHeap->Count() == 1);
	iHeap->Free(p1);
	p1 = NULL;
	test(iHeap->Count() == 0);

	// Medium buffer
	p1=iHeap->Alloc(0x20000);
	test(p1 != NULL && p1 >= iMemBase && p1 < iHybridHeap);
	test(iHeap->Count() == 1);
	iHeap->Free(p1);
	p1 = NULL;
	test(iHeap->Count() == 0);
	
	// Large buffer
	p1=iHeap->Alloc(0x700000);
	test(p1 != NULL && p1 >= iMemBase && p1 < iHybridHeap);
	test(iHeap->Count() == 1);
	iHeap->Free(p1);
	p1 = NULL;
	test(iHeap->Count() == 0);

	// Oversized buffer, not allocated
	p1=iHeap->Alloc(0x900000);
	test(p1 == NULL);
	test(iHeap->Count() == 0);
}


///////////////////////////////////////////////////////////////////////////
// Allocate and free multiple random sized buffers, sizes under 65 kB.   //
// Check that all are allocated succesfully with Count. Free every other //
// of them, check the Count. Allocate more buffers sized under 655 kB    //
// and free all buffers in reverse order. Check all are freed.           //
///////////////////////////////////////////////////////////////////////////
void TestRHeap::Test2(void)
{
    TInt ArraySize=10;
	TInt ArrayIndex;
	TAny** ArrayOfCells;
	ArrayOfCells = new TAny*[ArraySize];

	// Allocate set of buffers
	for(ArrayIndex=0; ArrayIndex<ArraySize; ArrayIndex++)
	{
		ArrayOfCells[ArrayIndex] = 0;
		ArrayOfCells[ArrayIndex] = iHeap->Alloc(GetRandomSize(0xFFFF));
		test(ArrayOfCells[ArrayIndex] != NULL);
	}
	test(iHeap->Count() == 10);

	// Free every other
	for(ArrayIndex=0; ArrayIndex<ArraySize; ArrayIndex=ArrayIndex+2 )
	{
		iHeap->Free(ArrayOfCells[ArrayIndex]);
		ArrayOfCells[ArrayIndex] = 0;
	}
	test(iHeap->Count() == 5);
	
	TInt ArraySize2=10;
	TInt ArrayIndex2;
	TAny** ArrayOfCells2;
	ArrayOfCells2 = new TAny*[ArraySize2];

	// Allocate larger buffers
	for(ArrayIndex2=0; ArrayIndex2<ArraySize; ArrayIndex2++)
	{
		ArrayOfCells2[ArrayIndex2] = 0;
		ArrayOfCells2[ArrayIndex2] = iHeap->Alloc(GetRandomSize(0x7FFFF));
		test(ArrayOfCells2[ArrayIndex2] != NULL);
	}
	test(iHeap->Count() == 15);

	// Free all buffers in reverse order
	for(ArrayIndex=9; ArrayIndex>=0; ArrayIndex-- )
	{
		if(ArrayOfCells[ArrayIndex] != 0)
		{
			iHeap->Free(ArrayOfCells[ArrayIndex]);
			ArrayOfCells[ArrayIndex] = 0;
		}
	}
	for(ArrayIndex2=9; ArrayIndex2>=0; ArrayIndex2-- )
	{
		if(ArrayOfCells2[ArrayIndex2] != 0)
		{
			iHeap->Free(ArrayOfCells2[ArrayIndex2]);
			ArrayOfCells2[ArrayIndex2] = 0;
		}
	}
	test(iHeap->Count() == 0);
}


///////////////////////////////////////////////////////////////////////
// Allocate and free multiple random sized buffers. Use              // 
// DebugFunction(EWalk) to check that all allocated cells are found. //
///////////////////////////////////////////////////////////////////////
void TestRHeap::Test3(void)
{
    TUint iAllocatedArraySize = 100;
    iAllocatedArrayOfCells = new TAny*[iAllocatedArraySize];
    
    // allocate 100 random cells and save them in iAllocatedArrayOfCells
    for(iAllocatedArrayIndex=0; iAllocatedArrayIndex<iAllocatedArraySize; iAllocatedArrayIndex++)
    {
        iAllocatedArrayOfCells[iAllocatedArrayIndex] = 0;
        iAllocatedArrayOfCells[iAllocatedArrayIndex] = iHeap->Alloc(GetRandomSize(0xFFFF));
        test(iAllocatedArrayOfCells[iAllocatedArrayIndex] != NULL);
    }
    test(iHeap->Count() == 100);    //check that all 100 allocations have succeedeed
	
    // copy iAllocatedArrayOfCells => iWalkArrayOfCells
    iWalkArrayOfCells = new TAny*[iWalkArrayIndex];
    for(iWalkArrayIndex=0; iWalkArrayIndex<iWalkArraySize; iWalkArrayIndex++)
        {
            iWalkArrayOfCells[iWalkArrayIndex] = 0;
            iWalkArrayOfCells[iWalkArrayIndex] = iAllocatedArrayOfCells[iWalkArrayIndex];
            test(iWalkArrayOfCells[iWalkArrayIndex] == iAllocatedArrayOfCells[iWalkArrayIndex]);
        }
    
    //check that walk finds all allocated cells...
        iHeap->DebugFunction(EWalk, (TAny*)&WalkCallback, (TAny*)this);  
        TBool ret = CheckWalkArrayEmpty();
        test(ret);     // ...and iWalkArrayOfCells is emptied
        
	// copy iAllocatedArrayOfCells => iWalkArrayOfCells
	    iWalkArrayOfCells = new TAny*[iWalkArrayIndex];
	    for(iWalkArrayIndex=0; iWalkArrayIndex<iWalkArraySize; iWalkArrayIndex++)
	        {
	            iWalkArrayOfCells[iWalkArrayIndex] = 0;
	            iWalkArrayOfCells[iWalkArrayIndex] = iAllocatedArrayOfCells[iWalkArrayIndex];
	            test(iWalkArrayOfCells[iWalkArrayIndex] == iAllocatedArrayOfCells[iWalkArrayIndex]);
	        }
	
	// free 40 random cells from iWalkArrayOfCells
	TUint i;
	for (i=0; i<40; i++)
	    {
        TUint RandomIndex = GetRandomIndex(99);
        iHeap->Free(iWalkArrayOfCells[RandomIndex]);
        iWalkArrayOfCells[RandomIndex] = 0;
        iAllocatedArrayOfCells[RandomIndex] = 0;
	    }
	test(iHeap->Count() == 60);
	
	//check that walk finds all the remaining allocated cells...
	iHeap->DebugFunction(EWalk, (TAny*)&WalkCallback, (TAny*)this);  
	ret = CheckWalkArrayEmpty();
	test(ret);     // ...and iWalkArrayOfCells is emptied
	
	// allocate 20 more random cells starting on the first available free cell
	iAllocatedArrayIndex = 0;
	for (i=0; i<20; i++)
	    {
        while (iAllocatedArrayOfCells[iAllocatedArrayIndex] != 0)
            {
            iAllocatedArrayIndex++;
            }
        iAllocatedArrayOfCells[iAllocatedArrayIndex] = iHeap->Alloc(GetRandomSize(0xFFFF));
	    }
	test(iHeap->Count() == 80);
	
	// copy iAllocatedArrayOfCells => iWalkArrayOfCells
	iWalkArrayOfCells = new TAny*[iWalkArrayIndex];
	    for(iWalkArrayIndex=0; iWalkArrayIndex<iWalkArraySize; iWalkArrayIndex++)
	        {
	            iWalkArrayOfCells[iWalkArrayIndex] = 0;
	            iWalkArrayOfCells[iWalkArrayIndex] = iAllocatedArrayOfCells[iWalkArrayIndex];
	            test(iWalkArrayOfCells[iWalkArrayIndex] == iAllocatedArrayOfCells[iWalkArrayIndex]);
	        }
	
	//check that walk finds all the earlier and newly allocated cells...
	iHeap->DebugFunction(EWalk, (TAny*)&WalkCallback, (TAny*)this);  
	ret = CheckWalkArrayEmpty();
	test(ret);     // ...and iWalkArrayOfCells is emptied
}

	
void TestRHeap::CloseTests()
	{
		// close heap so we don't exceed chunk limit
		iHeap->Close();  
	}

	
GLDEF_C TInt E32Main(void)
	{
	test.Title();
	__KHEAP_MARK;
	
	TestRHeap T;
	
	test.Start(_L("Page Allocator Test"));
		    
	TPtrC testHeapM=_L("TESTHEAP-MAIN");
    RHeap* iHeapM;

	iHeapM=User::ChunkHeap(&testHeapM,0x1800,0x800000); 
	
    TMetaData metaData;
    GetMeta(*iHeapM, metaData);

    iHeapM->Close();
    
    if (metaData.iDLOnly)
        {
        test.Printf(_L("Page allocator is not used, no tests to run.\n"));
        __KHEAP_MARKEND;
        test.End();
        return(0);
        }
    
    test.Next(_L("Init Paged allocator tests"));
    T.InitTests();
	test.Next(_L("Test Paged allocator 1"));
	T.Test1();
	test.Next(_L("Test Paged allocator 2"));
	T.Test2();
	test.Next(_L("Test Paged allocator 3"));
	T.Test3();
	T.CloseTests();

	__KHEAP_CHECK(0);
	__KHEAP_MARKEND;
	
	test.End();
	
	return (0);    
    }
