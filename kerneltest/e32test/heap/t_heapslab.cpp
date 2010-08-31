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
// e32test\heap\t_heapslab.cpp
// Overview:
// Tests RHybridHeap class: slab allocator
// API Information:
// RHybridHeap/RHeap
// Details:
//- Starts with empty allocator configured to use slab allocation
//  on all cell sizes less than slab threshold (49).
//- Allocate enough cells of the same size to fill 128 slabs.
//- Check the number of pages used corresponds to the number of slabs.
//- Check that a new slab is taken from a partially filled page if available.
//- Check that a partially filled slab is used if available.
//- Check that if all four slabs in a page are free, the page is freed.
//- Free cells to give empty slab.
//- Free cells to give partial slab.
//- Reallocate cells.
//- RAllocator::Check() is used to check internal consistency.
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
#include <e32def_private.h>
#include "dla.h"
#include "slab.h"
#include "page_alloc.h"
#include "heap_hybrid.h"

#define MAX_THREADS 4
#define MAX_ALLOCS 20000 // 16128, if slab count is 128 and alloc size is 8

//#define TSTSLAB_DBG(a) a
#define TSTSLAB_DBG(a)

struct TSlabTestThreadParm
    {
    RHeap*  iHeap;
    TInt    iAllocSize;
    TInt    iInitSlabCount;
    TBool   iUseRandomSize;
    TInt    iThreadCount;
    TInt    iThreadIndex;
    };

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

class TestHybridHeap
    {
public:
    static void GetHeapMetaData(RHeap& aHeap, TMetaData& aMeta);
    };

LOCAL_D RTest test(_L("T_HEAPSLAB"));

LOCAL_D TInt PageSize;

LOCAL_D TAny* PtrBuf[MAX_THREADS][MAX_ALLOCS];
LOCAL_D TSlabTestThreadParm ThreadParm[MAX_THREADS];

enum TTestWalkFunc {ETestWalk, ETestFindSlab};


static unsigned SlabHeaderPagemap(unsigned h) {return (h&0x00000f00)>>8;}

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

/*LOCAL_C void PrintMeta(const char* aText, TMetaData& aMeta)
{
    RDebug::Printf("=========== HeapMetaData (local) - begin: %s", aText);

    RDebug::Printf("iDLOnly: 0x%08x", aMeta.iDLOnly);
    RDebug::Printf("iChunkSize: 0x%08x", aMeta.iChunkSize);
    RDebug::Printf("iSlabThreshold: 0x%08x / %d", aMeta.iSlabThreshold, aMeta.iSlabThreshold);
    RDebug::Printf("iSlabInitThreshold: 0x%08x / %d", aMeta.iSlabInitThreshold, aMeta.iSlabInitThreshold);
    RDebug::Printf("iSlabConfigBits: 0x%08x", aMeta.iSlabConfigBits);
    RDebug::Printf("iPartialPage: 0x%08x", aMeta.iPartialPage);
    RDebug::Printf("iFullSlab: 0x%08x", aMeta.iFullSlab);
    RDebug::Printf("iSparePage: 0x%08x", aMeta.iSparePage);
    RDebug::Printf("iMemBase: 0x%08x", aMeta.iMemBase);

    TInt i;
    TInt count;
    count = sizeof(aMeta.iSizeMap)/sizeof(unsigned char);
    for (i=0; i<count; ++i)
        {
        RDebug::Printf("iSizeMap[%d]: %d", i, aMeta.iSizeMap[i]);
        }
    count = sizeof(aMeta.iSlabAlloc)/sizeof(slabset);
    for (i=0; i<count; ++i)
        {
        RDebug::Printf("iSlabAlloc[%d].iPartial: 0x%08x", i, aMeta.iSlabAlloc[i].iPartial);
        }
    for (i=0; i<count; ++i)
        {
        RDebug::Printf("iSlabAllocRealRootAddress[%d]: 0x%08x", i, aMeta.iSlabAllocRealRootAddress[i]);
        }
    RDebug::Printf("=========== HeapMetaData (local) - end");
}

LOCAL_C void GetAndPrintMeta(RHeap& aHeap, const char* aText, TMetaData& aMeta)
{
    (void)aText;
    GetMeta(aHeap, aMeta);
    TSTSLAB_DBG(PrintMeta(aText, aMeta));
}

#ifndef __KERNEL_MODE__
LOCAL_C void Lock(TMetaData& aMeta)
   {((RFastLock&)*aMeta.iLock).Wait();}

LOCAL_C void Unlock(TMetaData& aMeta)
   {((RFastLock&)*aMeta.iLock).Signal();}
#else
LOCAL_C void Lock(TMetaData& aMeta)
   {;}

LOCAL_C void Unlock(TMetaData& aMeta)
   {;}
#endif
*/

LOCAL_C  page* PageFor(slab* s)
    {
        return reinterpret_cast<page*>(Floor(s, PAGESIZE));
    }

   
LOCAL_C slab* SlabFor(const void* p)
{
    return (slab*)(Floor(p, SLABSIZE));
}

LOCAL_C TInt TreeWalk(slab** aRealRootAddress, slab* const* aRoot, TTestWalkFunc aFunc, TAny* aParm, TInt& aOutParm)
{
    TInt count = 0;
    aOutParm = 0;
    
    slab* s = *aRoot;
    if (!s)
        return count;
    
    for (;;)
        {
        slab* c;
        while ((c = s->iChild1) != 0)
            s = c;      // walk down left side to end
        for (;;)
            {
            count++;
            TSTSLAB_DBG(RDebug::Printf("TreeWalk - slab: 0x%08x", s));
            (void)aParm;
            if (aFunc == ETestWalk)
                {
                ;
                }
            else if (aFunc == ETestFindSlab)
                {
                if ((slab*)aParm == s)
                    {
                    aOutParm = 1;
                    return 0;
                    }
                }
            
            c = s->iChild2;
            if (c)
                {   // one step down right side, now try and walk down left
                s = c;
                break;
                }
            for (;;)
                {   // loop to walk up right side
                slab** pp = s->iParent;
                if (pp == aRealRootAddress)
                    return count;
                s = SlabFor(pp);
                if (pp == &s->iChild1)
                    break;
                }
            }
        }
}

LOCAL_C TInt WalkSlabSet(TInt aSlabsetIndex, TMetaData& aMeta, TTestWalkFunc aFunc, TAny* aParm, TInt& aOutParm)
{
    if (aSlabsetIndex >= (MAXSLABSIZE>>2))
        {
        return 0;
        }
    return TreeWalk(aMeta.iSlabAllocRealRootAddress[aSlabsetIndex], &aMeta.iSlabAlloc[aSlabsetIndex].iPartial, aFunc, aParm, aOutParm);
}

/*LOCAL_C void DebugPrintSlabs(TInt aSlabsetIndex, TMetaData& aMeta)
    {
    //RDebug::Printf("=========== DebugPrintSlabs: %s", aText);
    RDebug::Printf("=========== DebugPrintSlabs");

    RDebug::Printf("iSparePage: 0x%08x", aMeta.iSparePage);
    
    slab* fullSlab = aMeta.iFullSlab;
    TInt fullSlabCount = 0;
    while (fullSlab)
        {
        RDebug::Printf("fullSlab: 0x%08x", fullSlab);
        fullSlabCount++;
        fullSlab = fullSlab->iChild1;
        }

    TInt outParm;
    TInt partialTreeSlabCount = 0;
    partialTreeSlabCount += WalkSlabSet(aSlabsetIndex, aMeta, ETestWalk, 0, outParm);

    slab* partialPageSlab = aMeta.iPartialPage;
    TInt partialPageSlabCount = 0;
    while (partialPageSlab)
        {
        RDebug::Printf("partialPageSlab (empty): 0x%08x", partialPageSlab);
        partialPageSlabCount++;
        partialPageSlab = partialPageSlab->iChild1;
        }
    }*/

LOCAL_C void TestSlabFixedSizeManyThreads(TSlabTestThreadParm& aParm)
    {
    RHeap* heap = aParm.iHeap;
    TInt allocSize = aParm.iAllocSize;
    TInt initSlabCount = aParm.iInitSlabCount;
    //TBool useRandomSize = aParm.iUseRandomSize;
    //TInt threadCount = aParm.iThreadCount;
    TInt threadIndex = aParm.iThreadIndex;

    TInt slabsPerPage = PageSize/SLABSIZE;

    test(initSlabCount % slabsPerPage == 0); // for this test
    
#ifdef _DEBUG
    TInt allocRealCellSize = allocSize + RHeap::EDebugHdrSize;
#else
    TInt allocRealCellSize = allocSize;
#endif

    TMetaData metaData;
    GetMeta(*heap, metaData);
    
    if (allocRealCellSize >= metaData.iSlabThreshold)
        {
        allocRealCellSize = metaData.iSlabThreshold - 1;
#ifdef _DEBUG
        allocSize = allocRealCellSize - RHeap::EDebugHdrSize;
#else
        allocSize = allocRealCellSize;
#endif
        }
    
    TAny** pBuf = &PtrBuf[threadIndex][0];
    TInt i;
    for (i=0; i<MAX_ALLOCS; ++i)
        {
        pBuf[i] = 0;
        }

    //Allocate enough cells of the same size to fill initSlabCount (128) slabs
    TInt slabsetIndex = metaData.iSizeMap[(allocRealCellSize+3)>>2];
    test(slabsetIndex != 0xff);
    TInt slabCellSize = 4 + (slabsetIndex * 4);
    
    TInt slabPayloadSize = SLABSIZE - sizeof(slabhdr);
    TInt cellCountPerSlab = slabPayloadSize / slabCellSize;
    TInt initCellCount = initSlabCount * cellCountPerSlab;

    TSTSLAB_DBG(RDebug::Printf("=========== Allocate enough cells of the same size to fill %d slabs", initSlabCount));
    TSTSLAB_DBG(RDebug::Printf("=========== counts: %d %d %d", cellCountPerSlab, initCellCount, slabCellSize));

    for (i=0; i<initCellCount; ++i)
        {
        pBuf[i] = heap->Alloc(allocSize);
        test(pBuf[i] != 0);
        }

    heap->Check();

    TInt maxI5 = initCellCount + (cellCountPerSlab*(slabsPerPage+1));
    for (i=initCellCount; i<maxI5; ++i)
        {
        pBuf[i] = heap->Alloc(allocSize);
        test(pBuf[i] != 0);
        }

    heap->Check();
    
    TAny* p2 = heap->Alloc(allocSize);
    test(p2 != 0);

    heap->Check();
    heap->Free(p2);

    heap->Check();
    

    TInt page2pBufIndexFirst = cellCountPerSlab * slabsPerPage;
    //TInt page2pBufIndexLast = page2pBufIndexFirst + (cellCountPerSlab * slabsPerPage);
    
    slab* partialTreeSlabX1 = SlabFor(pBuf[page2pBufIndexFirst]);
    page* partialTreeSlabPageX1 = PageFor(partialTreeSlabX1);
    
    heap->Free(pBuf[page2pBufIndexFirst]);
    pBuf[page2pBufIndexFirst] = 0;

    heap->Check();
    
    TAny* p3 = heap->Alloc(allocSize);
    test(p3 != 0);
    heap->Check();
    heap->Free(p3);
    heap->Check();
    
    TInt size2 = metaData.iChunkSize;
    TSTSLAB_DBG(RDebug::Printf("---- size2: 0x%08x", size2));
    if (metaData.iSparePage)
        {
        size2 -= PageSize;
        }

    for (i=0; i<MAX_ALLOCS; ++i)
        {
        if (pBuf[i]) {
            page* page1 = PageFor(SlabFor(pBuf[i]));
            if (partialTreeSlabPageX1 == page1)
                {
                heap->Free(pBuf[i]);
                pBuf[i] = 0;
                }
            }
        }

    heap->Check();

    TInt size3 = metaData.iChunkSize;
    if (metaData.iSparePage)
        {
        size3 -= PageSize;
        }

    TInt bufIndexFirst = cellCountPerSlab;
    TInt maxI = bufIndexFirst + cellCountPerSlab;
    for (i=bufIndexFirst; i<=maxI; ++i)
        {
        if (pBuf[i])
            {
            heap->Free(pBuf[i]);
            pBuf[i] = 0;
            }
        }

    heap->Check();

    TInt firstI = cellCountPerSlab * 3;
    maxI = firstI + cellCountPerSlab;
    for (i=firstI; i<=maxI; ++i)
        {
        if (i % 3 == 0)
            {
            if (pBuf[i])
                {
                heap->Free(pBuf[i]);
                pBuf[i] = 0;
                }
            }
        }
    
    heap->Check();
    
    //Reallocate cells.
    for (i=0; i<(MAX_ALLOCS); ++i)
        {
        if (pBuf[i] != 0)
            {
            pBuf[i] = heap->ReAlloc(pBuf[i], allocSize);
            test(pBuf[i] != 0);
            }
        }
    
    heap->Check();
    
    //Allocate cells.
    for (i=0; i<(MAX_ALLOCS/4); ++i)
        {
        if (pBuf[i] == 0)
            {
            pBuf[i] = heap->Alloc(allocSize);
            test(pBuf[i] != 0);
            }
        }

    heap->Check();
    
    for (i=0; i<MAX_ALLOCS; ++i)
        {
        if (pBuf[i])
            {
            heap->Free(pBuf[i]);
            pBuf[i] = 0;
            }
        }
    heap->Check();

    TSTSLAB_DBG(RDebug::Printf("=========== TestSlabFixedSizeManyThreads end"));
    }
    
    
LOCAL_C void TestSlabFixedSizeOneThread(TSlabTestThreadParm& aParm)
    {
    RHeap* heap = aParm.iHeap;
    TInt allocSize = aParm.iAllocSize;
    TInt initSlabCount = aParm.iInitSlabCount;
    //TBool useRandomSize = aParm.iUseRandomSize;
    //TInt threadCount = aParm.iThreadCount;
    TInt threadIndex = aParm.iThreadIndex;

    TInt slabsPerPage = PageSize/SLABSIZE;

    test(initSlabCount % slabsPerPage == 0); // for this test
    
#ifdef _DEBUG
    TInt allocRealCellSize = allocSize + RHeap::EDebugHdrSize;
#else
    TInt allocRealCellSize = allocSize;
#endif

    TMetaData metaData;
    GetMeta(*heap, metaData);
    
    TSTSLAB_DBG(PrintMeta(" --- TestSlabFixedSizeOneThread start", metaData));

    if (allocRealCellSize >= metaData.iSlabThreshold)
        {
        allocRealCellSize = metaData.iSlabThreshold - 1;
#ifdef _DEBUG
        allocSize = allocRealCellSize - RHeap::EDebugHdrSize;
#else
        allocSize = allocRealCellSize;
#endif
        }

    TAny** pBuf = &PtrBuf[threadIndex][0];
    TInt i;
    for (i=0; i<MAX_ALLOCS; ++i)
        {
        pBuf[i] = 0;
        }

    //Allocate enough cells of the same size to fill initSlabCount (128) slabs
    TInt slabsetIndex = metaData.iSizeMap[(allocRealCellSize+3)>>2];
    test(slabsetIndex != 0xff);
    TInt slabCellSize = 4 + (slabsetIndex * 4);
    
    TInt slabPayloadSize = SLABSIZE - sizeof(slabhdr);
    TInt cellCountPerSlab = slabPayloadSize / slabCellSize;
    TInt initCellCount = initSlabCount * cellCountPerSlab;

    TSTSLAB_DBG(RDebug::Printf("=========== Allocate enough cells of the same size to fill %d slabs", initSlabCount));
    TSTSLAB_DBG(RDebug::Printf("=========== counts: %d %d %d", cellCountPerSlab, initCellCount, slabCellSize));

    for (i=0; i<initCellCount; ++i)
        {
        pBuf[i] = heap->Alloc(allocSize);
        test(pBuf[i] != 0);
        }

    heap->Check();
    GetMeta(*heap, metaData);
    
    TSTSLAB_DBG(PrintMeta("after init allocs", metaData));
    TSTSLAB_DBG(DebugPrintSlabs(slabsetIndex, metaData));

    //Check the number of pages used corresponds to the number of slabs.
    TSTSLAB_DBG(RDebug::Printf("=========== Check the number of pages used corresponds to the number of slabs"));
    
    TInt pageCountForSlabs1 = (metaData.iChunkSize / PageSize) - 1;
    TInt pageCountForSlabs2 = (initSlabCount+(slabsPerPage-1)) / slabsPerPage;
    TSTSLAB_DBG(RDebug::Printf("=========== page counts: %d %d", pageCountForSlabs1, pageCountForSlabs2));
    test(pageCountForSlabs1 == pageCountForSlabs2);

    //-----------------------------------------
    TSTSLAB_DBG(RDebug::Printf("=========== check slab counts in the lists"));
    
    slab* fullSlab = metaData.iFullSlab;
    TInt fullSlabCount = 0;
    while (fullSlab)
        {
        TSTSLAB_DBG(RDebug::Printf("fullSlab: 0x%08x", fullSlab));
        fullSlabCount++;
        fullSlab = fullSlab->iChild1;
        }

    TInt outParm;
    TInt partialTreeSlabCount = 0;
    partialTreeSlabCount = WalkSlabSet(slabsetIndex, metaData, ETestWalk, 0, outParm);

    slab* partialPageSlab = metaData.iPartialPage;
    TInt partialPageSlabCount = 0;
    while (partialPageSlab)
        {
        TSTSLAB_DBG(RDebug::Printf("partialPageSlab (empty): 0x%08x", partialPageSlab));
        partialPageSlabCount++;
        partialPageSlab = partialPageSlab->iChild1;
        }

    test(fullSlabCount == (initSlabCount-1));
    test(partialTreeSlabCount == 1);
    if (initSlabCount % slabsPerPage == 0)
        {
        test(partialPageSlabCount == 0);
        }
    else
        {
        test(partialPageSlabCount == 1);
        }
    //-----------------------------------------
    TSTSLAB_DBG(RDebug::Printf("=========== alloc one cell more -> one full slab more"));

    TAny* p = heap->Alloc(allocSize);
    test(p != 0);
    
    heap->Check();
    GetMeta(*heap, metaData);
    TSTSLAB_DBG(DebugPrintSlabs(slabsetIndex, metaData));
    
    fullSlab = metaData.iFullSlab;
    fullSlabCount = 0;
    while (fullSlab)
        {
        TSTSLAB_DBG(RDebug::Printf("fullSlab: 0x%08x", fullSlab));
        fullSlabCount++;
        fullSlab = fullSlab->iChild1;
        }
    test(fullSlabCount == initSlabCount);

    heap->Free(p);

    heap->Check();
    GetMeta(*heap, metaData);
    TSTSLAB_DBG(DebugPrintSlabs(slabsetIndex, metaData));
    
    //-----------------------------------------
    //Check that a new slab is taken from a partially filled page if available.
    TSTSLAB_DBG(RDebug::Printf("=========== Check that a new slab is taken from a partially filled page if available"));

    // fill the first slab in the page (after iSparePage)
    TInt maxI5 = initCellCount + (cellCountPerSlab*(slabsPerPage+1));
    for (i=initCellCount; i<maxI5; ++i)
        {
        pBuf[i] = heap->Alloc(allocSize);
        test(pBuf[i] != 0);
        }

    heap->Check();
    GetMeta(*heap, metaData);
    TSTSLAB_DBG(DebugPrintSlabs(slabsetIndex, metaData));
    
    partialPageSlab = metaData.iPartialPage;
    partialPageSlabCount = 0;
    while (partialPageSlab)
        {
        TSTSLAB_DBG(RDebug::Printf("partialPageSlab (empty): 0x%08x", partialPageSlab));
        partialPageSlabCount++;
        partialPageSlab = partialPageSlab->iChild1;
        }
    test(partialPageSlabCount == 1);

    page* page1 = PageFor(metaData.iPartialPage);
    unsigned header = page1->iSlabs[0].iHeader;
    unsigned pagemap = SlabHeaderPagemap(header);
    unsigned slabix = LOWBIT(pagemap);
    slab* partialPageSlab2 = &page1->iSlabs[slabix];
    
    TAny* p2 = heap->Alloc(allocSize);
    test(p2 != 0);

    heap->Check();
    TSTSLAB_DBG(RDebug::Printf("p2: 0x%08x; partialPageSlab2: 0x%08x", p2, partialPageSlab2));
    test(partialPageSlab2 == SlabFor(p2));
    heap->Free(p2);

    heap->Check();
    
    //-----------------------------
    // use the second page for the next test
    TInt page2pBufIndexFirst = cellCountPerSlab * slabsPerPage;
    //TInt page2pBufIndexLast = page2pBufIndexFirst + (cellCountPerSlab * slabsPerPage);
    
    //-----------------------------------------
    //Check that a partially filled slab is used if available.
    TSTSLAB_DBG(RDebug::Printf("=========== Check that a partially filled slab is used if available"));

    slab* partialTreeSlabX1 = SlabFor(pBuf[page2pBufIndexFirst]);
    page* partialTreeSlabPageX1 = PageFor(partialTreeSlabX1);
    
    heap->Free(pBuf[page2pBufIndexFirst]);
    pBuf[page2pBufIndexFirst] = 0;

    heap->Check();
    
    TAny* p3 = heap->Alloc(allocSize);
    test(p3 != 0);
    heap->Check();
    test(partialTreeSlabX1 == SlabFor(p3));
    heap->Free(p3);
    heap->Check();
    
    //-----------------------------------------
    //Check that if all four slabs in a page are free the page is freed.
    TSTSLAB_DBG(RDebug::Printf("=========== Check that if all four slabs in a page are free, the page is freed"));

    GetMeta(*heap, metaData);
    TSTSLAB_DBG(DebugPrintSlabs(slabsetIndex, metaData));

    TInt size2 = metaData.iChunkSize;
    TSTSLAB_DBG(RDebug::Printf("---- size2: 0x%08x", size2));
    if (metaData.iSparePage)
        {
        size2 -= PageSize;
        }

    for (i=0; i<MAX_ALLOCS; ++i)
        {
        if (pBuf[i]) {
            page* page1 = PageFor(SlabFor(pBuf[i]));
            if (partialTreeSlabPageX1 == page1)
                {
                heap->Free(pBuf[i]);
                pBuf[i] = 0;
                }
            }
        }

    heap->Check();
    GetMeta(*heap, metaData);
    TSTSLAB_DBG(DebugPrintSlabs(slabsetIndex, metaData));

    TInt size3 = metaData.iChunkSize;
    if (metaData.iSparePage)
        {
        size3 -= PageSize;
        }

    test(size3 == (size2-PageSize));

    //-----------------------------------------
    //Free cells to give empty slab (The second slab in the first page)
    TSTSLAB_DBG(RDebug::Printf("=========== Free cells to give empty slab (The second slab in the first page)"));
    slab* emptySlabAddr = (slab*)(metaData.iMemBase + SLABSIZE);

    //Check that emptySlabAddr is not already in iPartialPage list
    partialPageSlab = metaData.iPartialPage;
    while (partialPageSlab)
        {
        if (partialPageSlab == emptySlabAddr)
            {
            test(0);
            }
        partialPageSlab = partialPageSlab->iChild1;
        }

    // free cells to give empty slab  - emptySlabAddr
    TInt bufIndexFirst = cellCountPerSlab;
    TInt maxI = bufIndexFirst + cellCountPerSlab;
    for (i=bufIndexFirst; i<=maxI; ++i)
        {
        if (pBuf[i])
            {
            heap->Free(pBuf[i]);
            pBuf[i] = 0;
            }
        }

    heap->Check();
    GetMeta(*heap, metaData);
    TSTSLAB_DBG(DebugPrintSlabs(slabsetIndex, metaData));

    // Check that emptySlabAddr is not now in iPartialPage list
    partialPageSlab = metaData.iPartialPage;
    while (partialPageSlab)
        {
        if (partialPageSlab == emptySlabAddr)
            {
            break;
            }
        partialPageSlab = partialPageSlab->iChild1;
        }
    test(partialPageSlab != 0);

    //Free cells to give partial slab (The third slab in the first page)
    TSTSLAB_DBG(RDebug::Printf("=========== Free cells to give partial slab (The third slab in the first page)"));
    slab* partialSlabAddr = (slab*)(metaData.iMemBase + (3*SLABSIZE));

    // Check that partialSlabAddr is not now in iPartialSlab list
    WalkSlabSet(slabsetIndex, metaData, ETestFindSlab, partialSlabAddr, outParm);
    test(outParm == 0);

    TInt firstI = cellCountPerSlab * 3;
    maxI = firstI + cellCountPerSlab;
    for (i=firstI; i<=maxI; ++i)
        {
        if (i % 3 == 0)
            {
            if (pBuf[i])
                {
                heap->Free(pBuf[i]);
                pBuf[i] = 0;
                }
            }
        }
    
    heap->Check();
    GetMeta(*heap, metaData);
    TSTSLAB_DBG(DebugPrintSlabs(slabsetIndex, metaData));
    
    // Check that partialSlabAddr is now in iPartialSlab list
    WalkSlabSet(slabsetIndex, metaData, ETestFindSlab, partialSlabAddr, outParm);
    test(outParm == 1);
    
    //Reallocate cells.
    for (i=0; i<(MAX_ALLOCS); ++i)
        {
        if (pBuf[i] != 0)
            {
            pBuf[i] = heap->ReAlloc(pBuf[i], allocSize);
            test(pBuf[i] != 0);
            }
        }

    heap->Check();
    
    //Allocate cells.
    for (i=0; i<(MAX_ALLOCS/4); ++i)
        {
        if (pBuf[i] == 0)
            {
            pBuf[i] = heap->Alloc(allocSize);
            test(pBuf[i] != 0);
            }
        }

    heap->Check();
    
    for (i=0; i<MAX_ALLOCS; ++i)
        {
        if (pBuf[i])
            {
            heap->Free(pBuf[i]);
            pBuf[i] = 0;
            }
        }
    heap->Check();

    TSTSLAB_DBG(RDebug::Printf("=========== TestSlabFixedSizeOneThread end"));
    }

LOCAL_C RHeap* CreateSlabHeap(TInt aThreadCount)
{
    //TPtrC slabHeap=_L("SlabHeap");
    //RHeap* heap = User::ChunkHeap(&slabHeap, 0x1000, 0x10000);
    TInt maxLth = 0x60000 * aThreadCount;
    RHeap* heap = User::ChunkHeap(0, 0x1000, maxLth);
    test(heap!=NULL);

    // Configure heap for slab
    RHybridHeap::STestCommand cmd;
    cmd.iCommand = RHybridHeap::ESetConfig;
    cmd.iConfig.iSlabBits = 0xabe;
    cmd.iConfig.iDelayedSlabThreshold = 0;
    cmd.iConfig.iPagePower = 0;  // 16 // 0 -> no page allocator
    TInt ret = heap->DebugFunction(RHeap::EHybridHeap, &cmd, 0);
    test(ret == KErrNone);
    
    return heap;
}

LOCAL_C TInt SlabTestManyThreads(TAny* aThreadParm)
    {
    TSlabTestThreadParm* parm = (TSlabTestThreadParm*)aThreadParm;

    TInt i;
    TInt maxLoops = 30; //300;
    for (i=0; i<maxLoops; ++i)
        {
        TestSlabFixedSizeManyThreads(*parm);
        }
                                
    return KErrNone;
    }
    
LOCAL_C TInt SlabTestOneThread(TAny* aThreadParm)
    {
    TSlabTestThreadParm* parm = (TSlabTestThreadParm*)aThreadParm;
    TestSlabFixedSizeOneThread(*parm);
    return KErrNone;
    }
    
TInt StartThreads(TInt aThreadCount, TSlabTestThreadParm& aThreadParm)
    {
    const TInt KSlabTestThreadStackSize=0x4000; //0x10000; //0x2000;

    TRequestStatus  theStatus[MAX_THREADS];
    RThread         theThreads[MAX_THREADS];
    TBool           threadInUse[MAX_THREADS];

    TInt index;
    TInt ret;

    if (aThreadCount <= 0)
        {
        return KErrNone;
        }

    RHeap* heap = CreateSlabHeap(aThreadCount);
    aThreadParm.iHeap = heap;
    
    for (index = 0; index < aThreadCount; index++)
        {
        ThreadParm[index].iHeap             = aThreadParm.iHeap;
        ThreadParm[index].iAllocSize        = aThreadParm.iAllocSize;
        ThreadParm[index].iInitSlabCount    = aThreadParm.iInitSlabCount;
        ThreadParm[index].iUseRandomSize    = aThreadParm.iUseRandomSize;
        ThreadParm[index].iThreadCount      = aThreadParm.iThreadCount;
        
        ThreadParm[index].iThreadIndex = index;
        
        TBuf<32> threadName;
        threadName.Format(_L("SlabTest%d"), index);
        if (aThreadCount == 1)
            {
            ret = theThreads[index].Create(threadName, SlabTestOneThread, KSlabTestThreadStackSize, NULL, (TAny*)&ThreadParm[index]);
            }
        else
            {
            ret = theThreads[index].Create(threadName, SlabTestManyThreads, KSlabTestThreadStackSize, NULL, (TAny*)&ThreadParm[index]);
            }
        test(ret == KErrNone);
        theThreads[index].Logon(theStatus[index]);
        test(theStatus[index] == KRequestPending);
        threadInUse[index] = ETrue;
        theThreads[index].Resume();
        }

    User::WaitForAnyRequest();
    
    TBool anyUsed = ETrue;
    while (anyUsed)
        {
        User::After(1001000);
        anyUsed = EFalse;
        for (index = 0; index < aThreadCount; index++)
            {
            if (threadInUse[index])
                {
                if (theThreads[index].ExitType() != EExitPending)
                    {
                    threadInUse[index] = EFalse;
                    }
                else
                    {
                    anyUsed = ETrue;
                    }
                }
            }
        }
    
    for (index = 0; index < aThreadCount; index++)
        {
        theThreads[index].Close();
        }
    TSTSLAB_DBG(RDebug::Printf("=========== StartThreads end"));
    heap->Close();

    return KErrNone;
    }

GLDEF_C TInt E32Main(void)
    {
    TInt ret;
    
    test.Title();

    __KHEAP_MARK;
    
    test.Start(_L("TestSlab"));
    UserHal::PageSizeInBytes(PageSize);

    RHeap* heap = CreateSlabHeap(1);
    
    TMetaData metaData;
    GetMeta(*heap, metaData);

    heap->Close();

    if (metaData.iDLOnly)
        {
        test.Printf(_L("Slab allocator is not used, no tests to run\n"));
        __KHEAP_MARKEND;
        test.End();
        return(0);
        }
    
    TSlabTestThreadParm threadParm;
    threadParm.iHeap = heap;
    threadParm.iAllocSize = 17;
    threadParm.iInitSlabCount = 128; // 12
    threadParm.iUseRandomSize = EFalse;

    test.Next(_L("TestSlab - one thread"));

    TInt threadCount;
    threadCount = 1;
    if (threadCount > MAX_THREADS)
        {
        threadCount = MAX_THREADS;
        }
    threadParm.iThreadCount = threadCount;
    
#if 0
    ret = StartThreads(threadCount, threadParm);
    test(ret==KErrNone);
    
#else   
    
    TInt i;
    for (i=1; i<metaData.iSlabThreshold; ++i)
        {
#ifdef _DEBUG
        if ((i + RHeap::EDebugHdrSize) >= metaData.iSlabThreshold)
            {
            break;
            }
#endif // _DEBUG
        TSTSLAB_DBG(RDebug::Printf("=========== StartThreads size: %d", i));
        threadParm.iAllocSize = i;
        test.Printf(_L("AllocSize: %d\n"), i);
        ret = StartThreads(threadCount, threadParm);
        test(ret==KErrNone);
        }
#endif

    
    test.Next(_L("TestSlab - many threads"));

    threadParm.iAllocSize = 17;
    
    threadCount = 3;
    if (threadCount > MAX_THREADS)
        {
        threadCount = MAX_THREADS;
        }
    threadParm.iThreadCount = threadCount;
    
#if 1
    ret = StartThreads(threadCount, threadParm);
    test(ret==KErrNone);
    
#else   
    
    TInt i;
    for (i=1; i<metaData.iSlabThreshold; ++i)
        {
#ifdef _DEBUG
        if ((i + RHeap::EDebugHdrSize) >= metaData.iSlabThreshold)
            {
            break;
            }
#endif // _DEBUG
        TSTSLAB_DBG(RDebug::Printf("=========== StartThreads size: %d", i));
        threadParm.iAllocSize = i;
        test.Printf(_L("AllocSize: %d\n"), i);
        ret = StartThreads(threadCount, threadParm);
        test(ret==KErrNone);
        }
#endif

    __KHEAP_MARKEND;
    
    test.End();
    return(0);
    }
