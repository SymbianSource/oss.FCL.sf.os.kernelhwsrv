// Copyright (c) 2002-2009 Nokia Corporation and/or its subsidiary(-ies).
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
// e32test\heap\t_heaphybridstress.cpp
// Overview:
// Tests RHybridHeap class: stress test
// API Information:
// RHybridHeap/RHeap
// Details:
// - Stress test heap implementation that allocates, frees
//   and reallocates cells in random patterns, and checks the heap.
// - Allocated/reallocated buffer content is verified, when buffer is freed/reallocated.
// - Stress test with a single thread
// - Stress test with two threads that run concurrently.
// - Tests configured for slab, doug lea, paged and hybrid allocators
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

// Needed for KHeapShrinkHysRatio which is now ROM 'patchdata'
#include "TestRHeapShrink.h"

//-------------------------------------------

//#define TSTDBG_PRINTMETA(a) a
#define TSTDBG_PRINTMETA(a)

//-------------------------------------------


#ifdef __EABI__
       IMPORT_D extern const TInt KHeapMinCellSize;
#else
       const TInt KHeapMinCellSize = 0;
#endif

RTest test(_L("T_HEAPSTRESS"));

#define TEST_ALIGN(p,a)     test((TLinAddr(p)&((a)-1))==0)


#define TST_HEAP_MAX_LTH 0x4000000 // 64M
#define MAX_CELL_COUNT 0x100000 // 0x100 0x1000 0x10000 0x100000
#define MAX_THREAD_COUNT 2
LOCAL_D TUint8* HeapStressCell[MAX_THREAD_COUNT][MAX_CELL_COUNT];
LOCAL_D TInt HeapStressLen[MAX_THREAD_COUNT][MAX_CELL_COUNT];

enum TTestHybridHeapFunc {ETstOnlySlab, ETstOnlyDl, ETstOnlyPaged, ETstHybrid};
enum TTestType {ETestE32Test, ETestForeverOne, ETestForeverAll};

LOCAL_D TTimeIntervalMicroSeconds32 TickPeriod;

//--------- config parameters - begin
LOCAL_D TTestType TestType;
LOCAL_D TInt TestTimeAsSeconds;
LOCAL_D TBool TestForeverMultiThreadTest;
LOCAL_D TTestHybridHeapFunc TestHybridHeapFunc;
LOCAL_D TInt CurrMaxCellCount;
LOCAL_D TInt HeapMaxLength;
//--------- config parameters - end

LOCAL_D TBool DlOnly;

LOCAL_D TInt SlabThreshold;
LOCAL_D TInt PageThreshold;


struct TMetaData
    {
    TBool           iDLOnly;
    RFastLock*      iLock;
    TInt            iChunkSize;
    TInt            iSlabThreshold;
    TInt            iPageThreshold;
    TInt            iSlabInitThreshold;
    TUint32         iSlabConfigBits;
    slab*           iPartialPage;
    slab*           iFullSlab;
    page*           iSparePage;
    TUint8*         iMemBase;
    TUint8          iSizeMap[(MAXSLABSIZE>>2)+1];
    slabset         iSlabAlloc[MAXSLABSIZE>>2];
    slab**          iSlabAllocRealRootAddress[MAXSLABSIZE>>2];
    };

class TestHybridHeap
    {
public:
    static void GetHeapMetaData(RHeap& aHeap, TMetaData& aMeta);
    };

void TestHybridHeap::GetHeapMetaData(RHeap& aHeap, TMetaData& aMeta)
{
    RHybridHeap::STestCommand cmd;
    cmd.iCommand = RHybridHeap::EHeapMetaData;
    TInt ret = aHeap.DebugFunction(RHeap::EHybridHeap, &cmd, 0);
    test(ret == KErrNone);
    
    RHybridHeap* hybridHeap = (RHybridHeap*)cmd.iData;
    
    aMeta.iDLOnly              = hybridHeap->iDLOnly;
    aMeta.iLock                = &hybridHeap->iLock;
    aMeta.iChunkSize           = hybridHeap->iChunkSize;
    aMeta.iSlabThreshold       = hybridHeap->iSlabThreshold;
    aMeta.iPageThreshold       = hybridHeap->iPageThreshold;
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


TSTDBG_PRINTMETA(
LOCAL_C void PrintMeta(const char* aText, TMetaData& aMeta)
{
    RDebug::Printf("=========== HeapMetaData (local) - begin: %s", aText);

    RDebug::Printf("iDLOnly: 0x%08x", aMeta.iDLOnly);
    RDebug::Printf("iChunkSize: 0x%08x", aMeta.iChunkSize);
    RDebug::Printf("iSlabThreshold: 0x%08x / %d", aMeta.iSlabThreshold, aMeta.iSlabThreshold);
    RDebug::Printf("iPageThreshold: 0x%08x / %d", aMeta.iPageThreshold, aMeta.iPageThreshold);
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
)

LOCAL_C void ConfHeap(RHeap* aHeap)
{
    RHybridHeap::STestCommand cmd;

    if (TestHybridHeapFunc == ETstOnlySlab)
        {
        cmd.iCommand = RHybridHeap::ESetConfig;
        cmd.iConfig.iSlabBits = 0xabe;
        cmd.iConfig.iDelayedSlabThreshold = 0;            // 0 -> use slab at once from the beginning
        cmd.iConfig.iPagePower = 0;                       // 0 -> no page allocator
        }
    else if (TestHybridHeapFunc == ETstOnlyDl)
        {
        cmd.iCommand = RHybridHeap::ESetConfig;
        cmd.iConfig.iSlabBits = 0xabe;
        cmd.iConfig.iDelayedSlabThreshold = 0x40000000;   // 1G -> slab never used
        cmd.iConfig.iPagePower = 0;                       // 0 -> no page allocator
        }
    else if (TestHybridHeapFunc == ETstOnlyPaged)
        {
        cmd.iCommand = RHybridHeap::ESetConfig;
        cmd.iConfig.iSlabBits = 0xabe;
        cmd.iConfig.iDelayedSlabThreshold = 0x40000000;   // 1G -> slab never used
        cmd.iConfig.iPagePower = 14;                      // min page 14 -> 16K
        }
    else if (TestHybridHeapFunc == ETstHybrid)
        {
        cmd.iCommand = RHybridHeap::ESetConfig;
        cmd.iConfig.iSlabBits = 0xabe;
        cmd.iConfig.iDelayedSlabThreshold = 0;            // 0 -> use slab at once from the beginning
        cmd.iConfig.iPagePower = 14;                      // min page 14 -> 16K
        }
    else
        {
        test(0);
        }
    
    TInt ret = aHeap->DebugFunction(RHeap::EHybridHeap, &cmd, 0);
    test(ret == KErrNone);
}

LOCAL_C TInt MinPagedAllocLength(void)
{
    return (1 << PageThreshold);
}

LOCAL_C TUint32 RandomPagedLength(TUint32 aRandom)
{
    TUint32 ret;
    ret = aRandom;
    ret <<= PageThreshold;
    if (TestHybridHeapFunc == ETstOnlyPaged)
        {
        //ret &= 0xfffff; // below 1M
        ret &= 0x7ffff; // below 512K
        }
    else
        {
        ret &= 0x1ffff; // below 128K
        }
    if (ret == 0)
        {
        ret = MinPagedAllocLength();
        }
    return ret;
}

#if 0            
LOCAL_C TUint TicksAsMilliSeconds(TUint aTicks)
{
    TUint time = TUint((TUint64)aTicks*(TUint64)TickPeriod.Int()/(TUint64)1000);
    return time;
}
#endif

LOCAL_C TBool IsDlOnly(void)
{
    TestHybridHeapFunc = ETstHybrid;
    
    RHeap* heap;
    heap = UserHeap::ChunkHeap(&KNullDesC(), 0x1000, 0x4000, 0x1000, 4);
    test(heap != NULL);

    ConfHeap(heap);
    
    TMetaData metaData;
    GetMeta(*heap, metaData);
    
    heap->Close();
    return metaData.iDLOnly;
}

LOCAL_C RHeap* CreateTestHeap(TInt aAlign)
{
    if (HeapMaxLength > TST_HEAP_MAX_LTH)
        {
        HeapMaxLength = TST_HEAP_MAX_LTH;
        }
    
    if (CurrMaxCellCount > MAX_CELL_COUNT)
        {
        CurrMaxCellCount = MAX_CELL_COUNT;
        }
    
    RHeap* heap;
    heap = UserHeap::ChunkHeap(&KNullDesC(), 0x1000, HeapMaxLength, 0x1000, aAlign);
    test(heap != NULL);

    ConfHeap(heap);
    
    TMetaData metaData;
    GetMeta(*heap, metaData);
    
    if (TestHybridHeapFunc == ETstOnlySlab)
        {
        SlabThreshold = metaData.iSlabThreshold;
        test(SlabThreshold != 0);
        }
    else if (TestHybridHeapFunc == ETstOnlyDl)
        {
        }
    else if (TestHybridHeapFunc == ETstOnlyPaged)
        {
        PageThreshold = metaData.iPageThreshold;
        test(PageThreshold >= 14);
        }
    else if (TestHybridHeapFunc == ETstHybrid)
        {
        }
    else
        {
        test(0);
        }

    return heap;
}

//-------------------------------------------------------------------

struct STestCell
    {
    enum {EMagic = 0xb8aa3b29};

    TUint32 iLength;
    TUint32 iData[1];

    void Set(TInt aLength);
    void Verify(TInt aLength);
    void Verify(const TAny* aInitPtr, TInt aInitLength, TInt aLength);
    };

void STestCell::Set(TInt aLength)
    {
    TInt i;
    TUint32 x = (TUint32)this ^ (TUint32)aLength ^ (TUint32)EMagic;
    if (aLength < (TInt) sizeof(iLength))
        {
        return;
        }
    iLength = x;
    aLength /= sizeof(TUint32);
    for (i=0; i<aLength-1; ++i)
        {
        x *= 69069;
        x += 41;
        iData[i] = x;
        }
    }

void STestCell::Verify(TInt aLength)
    {
    Verify(this, aLength, aLength);
    }

void STestCell::Verify(const TAny* aInitPtr, TInt aInitLength, TInt aLength)
    {
    TInt i;
    TUint32 x = (TUint32)aInitPtr ^ (TUint32)aInitLength ^ (TUint32)EMagic;
    if (aLength < (TInt) sizeof(iLength))
        {
        return;
        }
    test(iLength == x);
    aLength /= sizeof(TUint32);
    for (i=0; i<aLength-1; ++i)
        {
        x *= 69069;
        x += 41;
        test(iData[i] == x);
        }
    }

class RTestHeap : public RHeap
    {
public:
    TInt CheckAllocatedCell(const TAny* aCell) const;
    void FullCheckAllocatedCell(const TAny* aCell) const;
    TAny* TestAlloc(TInt aSize);
    void TestFree(TAny* aPtr);
    TAny* TestReAlloc(TAny* aPtr, TInt aSize, TInt aMode=0);
    void FullCheck();
    static void WalkFullCheckCell(TAny* aPtr, TCellType aType, TAny* aCell, TInt aLen);
    };

TInt RTestHeap::CheckAllocatedCell(const TAny* aCell) const
    {
    TInt len = AllocLen(aCell);
    return len;
    }

void RTestHeap::FullCheckAllocatedCell(const TAny* aCell) const
    {
    ((STestCell*)aCell)->Verify(CheckAllocatedCell(aCell));
    }

TAny* RTestHeap::TestAlloc(TInt aSize)
    {
    TAny* p = Alloc(aSize);
    if (p)
        {
        TInt len = CheckAllocatedCell(p);
        test(len>=aSize);       
        ((STestCell*)p)->Set(len);
        }
    return p;
    }

void RTestHeap::TestFree(TAny* aPtr)
    {
    if (aPtr)
        {
        FullCheckAllocatedCell(aPtr);
        }
    Free(aPtr);
    }

TAny* RTestHeap::TestReAlloc(TAny* aPtr, TInt aSize, TInt aMode)
    {
    TInt old_len = aPtr ? CheckAllocatedCell(aPtr) : 0;
    if (aPtr)
        ((STestCell*)aPtr)->Verify(old_len);
    TAny* p = ReAlloc(aPtr, aSize, aMode);
    if (!p)
        {
        ((STestCell*)aPtr)->Verify(old_len);
        return p;
        }
    TInt new_len = CheckAllocatedCell(p);
    test(new_len>=aSize);       
    if (p == aPtr)
        {
        ((STestCell*)p)->Verify(p, old_len, Min(old_len, new_len));
        if (new_len != old_len)
            ((STestCell*)p)->Set(new_len);
        return p;
        }
    test(!(aMode & ENeverMove));
    test((new_len > old_len) || (aMode & EAllowMoveOnShrink));
    if (old_len)
        ((STestCell*)p)->Verify(aPtr, old_len, Min(old_len, aSize));
    ((STestCell*)p)->Set(new_len);
    return p;
    }

struct SHeapCellInfo
    {
    RTestHeap* iHeap;
    TInt iTotalAlloc;
    TInt iTotalAllocSize;
    TInt iTotalFree;
    TUint8* iNextCell;
    };

void RTestHeap::WalkFullCheckCell(TAny* aPtr, TCellType aType, TAny* aCell, TInt aLen)
    {
    (void)aCell;
    ::SHeapCellInfo& info = *(::SHeapCellInfo*)aPtr;
    switch(aType)
        {
        case EGoodAllocatedCell:
            {
            TInt len = aLen;
            info.iTotalAllocSize += len;
            STestCell* pT = (STestCell*)aCell;
            ++info.iTotalAlloc;
            pT->Verify(len);
            break;
            }
        case EGoodFreeCell:
            {
            ++info.iTotalFree;
            break;
            }
        default:
            test.Printf(_L("TYPE=%d ??\n"),aType);
            test(0);
            break;
        }
    }

void RTestHeap::FullCheck()
    {
    ::SHeapCellInfo info;
    Mem::FillZ(&info, sizeof(info));
    info.iHeap = this;
    DebugFunction(EWalk, (TAny*)&WalkFullCheckCell, &info);
    TInt count = AllocSize(iTotalAllocSize);
    test(info.iTotalAlloc == count);
    test(info.iTotalAllocSize == iTotalAllocSize);
    }


struct STestStress
    {
    RThread iThread;
    volatile TBool iStop;
    TInt iAllocs;
    TInt iFailedAllocs;
    TInt iFrees;
    TInt iReAllocs;
    TInt iFailedReAllocs;
    TInt iChecks;
    TUint32 iSeed;
    RAllocator* iAllocator;
    TInt iThreadIndex;

    TUint32 Random();
    };

TUint32 FirstSeed(TInt aThreadIndex)
    {
    static TUint32 seed0 = 0xb504f334;
    static TUint32 seed1 = 0xddb3d743;
    static TBool first = ETrue;

    TUint32 ret;

    if (aThreadIndex == 0)
        {
        ret = seed0;
        }
    else
        {
        ret = seed1;
        }
    
    if (first)
        {
        first = EFalse;
        }
    
    if (aThreadIndex == 0)
        {
        seed0 *= 69069;
        seed0 += 41;
        }
    else
        {
        seed1 *= 69069;
        seed1 += 41;
        }

    test.Printf(_L("FirstSeed: 0x%08x\n"), ret);
    return ret;
    }

TUint32 STestStress::Random()
    {
    iSeed *= 69069;
    iSeed += 41;
    return iSeed;
    }

TInt RandomLength(TUint32 aRandom)
    {
    TUint32 ret = 0;
    
    if (TestHybridHeapFunc == ETstOnlySlab)
        {
        test(SlabThreshold != 0);
        ret = aRandom;
        TInt realSlabThreshold = SlabThreshold;
#ifdef _DEBUG
        realSlabThreshold -= RHeap::EDebugHdrSize;
#endif
        ret %= realSlabThreshold;
        }
    else if (TestHybridHeapFunc == ETstOnlyDl)
        {
        TUint8 x = (TUint8)aRandom;
        if (x & 0x80)
            {
            ret = x & 0x7f;
            }
        else
            {
            ret = (x & 0x7f) << 7;
            }
        }
    else if (TestHybridHeapFunc == ETstOnlyPaged)
        {
        ret = RandomPagedLength(aRandom);
        }
    else if (TestHybridHeapFunc == ETstHybrid)
        {
        TUint8 x = (TUint8)aRandom;
        if (x & 0x80)
            {
            ret = x & 0x7f;
            }
        else
            {
            if (x & 0x10)
                {
                ret = (x & 0x7f) << 7;
                }
            else
                {
                ret = RandomPagedLength(aRandom);
                }
            }
        }
    else
        {
        test(0);
        }

    return (TInt)ret;
    }

TInt HeapStress(TAny* aPtr)
    {
    STestStress& stress = *(STestStress*)aPtr;
    RTestHeap* heap = (RTestHeap*)&User::Allocator();
    TUint8** cell;
    TInt* len;

    if (stress.iThreadIndex >= MAX_THREAD_COUNT)
        {
        test(0);
        }
    cell = &HeapStressCell[stress.iThreadIndex][0];
    len = &HeapStressLen[stress.iThreadIndex][0];

    Mem::FillZ(cell, sizeof(*cell)*CurrMaxCellCount);
    Mem::FillZ(len, sizeof(*len)*CurrMaxCellCount);

    RThread::Rendezvous(KErrNone);
    while (!stress.iStop)
        {
        // allocate all cells
        TInt i;
        for (i=0; i<CurrMaxCellCount; ++i)
            {
            if (!cell[i])
                {
                ++stress.iAllocs;
                cell[i] = (TUint8*)heap->TestAlloc(RandomLength(stress.Random()));
                if (cell[i])
                    len[i] = heap->AllocLen(cell[i]);
                else
                    ++stress.iFailedAllocs;
                }
            }

        // free some cells
        TInt n = (CurrMaxCellCount/4) + (stress.Random() & (CurrMaxCellCount/2-1));
        while (--n)
            {
            i = stress.Random() & (CurrMaxCellCount-1);
            if (cell[i])
                {
                test(heap->AllocLen(cell[i]) == len[i]);
                heap->TestFree(cell[i]);
                cell[i] = NULL;
                len[i] = 0;
                ++stress.iFrees;
                }
            }

        // realloc some cells
        n = (CurrMaxCellCount/4) + (stress.Random() & (CurrMaxCellCount/2-1));
        while (--n)
            {
            TUint32 rn = stress.Random();
            i = (rn >> 8) & (CurrMaxCellCount-1);
            TInt new_len = RandomLength(rn);
            if (cell[i])
                {
                test(heap->AllocLen(cell[i]) == len[i]);
                ++stress.iReAllocs;
                TUint8* p = (TUint8*)heap->TestReAlloc(cell[i], new_len, rn >> 16);
                if (p)
                    {
                    cell[i] = p;
                    len[i] = heap->AllocLen(p);
                    }
                else
                    {
                    ++stress.iFailedReAllocs;
                    }
                }
            }
                
        // check the heap
        heap->Check();
        ++stress.iChecks;
        }
    
    return 0;
    }

void PrintSummary(STestStress& aStress)
    {
    test.Printf(_L("Total Allocs    : %11d\n"), aStress.iAllocs);
    test.Printf(_L("Failed Allocs   : %11d\n"), aStress.iFailedAllocs);
    test.Printf(_L("Total Frees     : %11d\n"), aStress.iFrees);
    test.Printf(_L("Total ReAllocs  : %11d\n"), aStress.iReAllocs);
    test.Printf(_L("Failed ReAllocs : %11d\n"), aStress.iFailedReAllocs);
    test.Printf(_L("Heap checks     : %11d\n"), aStress.iChecks);
    }
    
void CreateStressThread(STestStress& aStress)
    {
    RThread& thread = aStress.iThread;
    TInt err = thread.Create(KNullDesC(), &HeapStress, 0x2000, aStress.iAllocator, &aStress);
    test(err==KErrNone);
    thread.SetPriority(EPriorityLess);
    TRequestStatus status;
    thread.Rendezvous(status);
    test(status == KRequestPending);
    thread.Resume();
    User::WaitForRequest(status);
    test(status == KErrNone);
    test(thread.ExitType() == EExitPending);
    thread.SetPriority(EPriorityMuchLess);
    }

void StopStressThread(STestStress& aStress)
    {
    RThread& thread = aStress.iThread;
    TRequestStatus status;
    thread.Logon(status);
    aStress.iStop = ETrue;
    User::WaitForRequest(status);
    const TDesC& exitCat = thread.ExitCategory();
    TInt exitReason = thread.ExitReason();
    TInt exitType = thread.ExitType();
    test.Printf(_L("Exit type %d,%d,%S\n"), exitType, exitReason, &exitCat);
    test(exitType == EExitKill);
    test(exitReason == KErrNone);
    test(status == KErrNone);
    PrintSummary(aStress);
    }

void WaitForKey(STestStress* aStress1, STestStress* aStress2)
{
    TRequestStatus keyStatus;
    CConsoleBase* console = test.Console();
    console->Read(keyStatus);
    
    for (;;)
        {
        User::WaitForRequest(keyStatus);
        if (keyStatus != KRequestPending)
            {
            test(keyStatus == KErrNone);
            if (console->KeyCode() == EKeyEscape)
                {
                test.Printf(_L("Forever test aborted by user\n"));
                break;
                }
            else if (console->KeyCode() == EKeySpace)
                {
                if (aStress1 != NULL)
                    {
                    PrintSummary(*aStress1);
                    }
                if (aStress2 != NULL)
                    {
                    PrintSummary(*aStress2);
                    }
                }
            }
        console->Read(keyStatus);
        }
}

TBool WaitForTimeoutOrKey(STestStress* aStress1, STestStress* aStress2)
{
    TBool abortedByUser = EFalse;
    RTimer timer;
    TRequestStatus timerStatus;
    TInt err = timer.CreateLocal();
    test(err == KErrNone);
    timer.After(timerStatus, TestTimeAsSeconds*1000000);
    
    TRequestStatus keyStatus;
    CConsoleBase* console = test.Console();
    console->Read(keyStatus);
    
    for (;;)
        {
        User::WaitForRequest(keyStatus, timerStatus);
        if (keyStatus != KRequestPending)
            {
            test(keyStatus == KErrNone);
            if (console->KeyCode() == EKeyEscape)
                {
                abortedByUser = ETrue;
                timer.Cancel();
                test.Printf(_L("Forever test aborted by user\n"));
                break;
                }
            else if (console->KeyCode() == EKeySpace)
                {
                if (aStress1 != NULL)
                    {
                    PrintSummary(*aStress1);
                    }
                if (aStress2 != NULL)
                    {
                    PrintSummary(*aStress2);
                    }
                }
            console->Read(keyStatus);
            }
        if (timerStatus != KRequestPending)
            {
            if (timerStatus != KErrNone)
                {
                test(0);
                }
            console->ReadCancel();
            break;
            }
        }
    timer.Close();
    return abortedByUser;
}
    
TBool DoStressTest1(RAllocator* aAllocator)
    {
    TBool abortedByUser = EFalse;
    
    RTestHeap* heap = (RTestHeap*)aAllocator;
    //test.Printf(_L("Test Stress 1: max=0x%x\n"),  heap->MaxLength());
    
    STestStress stress;
    Mem::FillZ(&stress, sizeof(STestStress));
    stress.iAllocator = aAllocator;
    stress.iThreadIndex = 0;
    if (TestType == ETestForeverAll)
        {
        stress.iSeed = FirstSeed(stress.iThreadIndex);
        }
    else
        {
        stress.iSeed = 0xb504f334;;
        }
    
    CreateStressThread(stress);
    
    if (TestType == ETestE32Test)
        {
        User::After(TestTimeAsSeconds*1000000);
        }
    else if (TestType == ETestForeverAll)
        {
        abortedByUser = WaitForTimeoutOrKey(&stress, NULL);
        }
    else if (TestType == ETestForeverOne)
        {
        WaitForKey(&stress, NULL);
        abortedByUser = ETrue;
        }
    else
        {
        test(0);
        }
    
    StopStressThread(stress);
    CLOSE_AND_WAIT(stress.iThread);
    heap->FullCheck();
    return abortedByUser;
    }

TBool DoStressTest2(RAllocator* aAllocator)
    {
    TBool abortedByUser = EFalse;
    
    RTestHeap* heap = (RTestHeap*)aAllocator;
    //test.Printf(_L("Test Stress 2: max=0x%x\n"),  heap->MaxLength());    
    
    STestStress stress1;
    Mem::FillZ(&stress1, sizeof(STestStress));
    stress1.iAllocator = aAllocator;
    stress1.iThreadIndex = 0;
    
    STestStress stress2;
    Mem::FillZ(&stress2, sizeof(STestStress));
    stress2.iAllocator = aAllocator;
    stress2.iThreadIndex = 1;
    
    if (TestType == ETestForeverAll)
        {
        stress1.iSeed = FirstSeed(stress1.iThreadIndex);
        stress2.iSeed = FirstSeed(stress2.iThreadIndex);
        }
    else
        {
        stress1.iSeed = 0xb504f334;
        stress2.iSeed = 0xddb3d743;
        }
    CreateStressThread(stress1);
    CreateStressThread(stress2);

    if (TestType == ETestE32Test)
        {
        User::After(2*TestTimeAsSeconds*1000000);
        }
    else if (TestType == ETestForeverAll)
        {
        abortedByUser = WaitForTimeoutOrKey(&stress1, &stress2);
        }
    else if (TestType == ETestForeverOne)
        {
        WaitForKey(&stress1, &stress2);
        abortedByUser = ETrue;
        }
    else
        {
        test(0);
        }
    
    StopStressThread(stress1);
    StopStressThread(stress2);
    CLOSE_AND_WAIT(stress1.iThread);
    CLOSE_AND_WAIT(stress2.iThread);
    heap->FullCheck();
    return abortedByUser;
    }

TBool StressTests(void)
    {
    TBool abortedByUser = EFalse;
    RHeap* heap = 0;
    
    for (;;)
        {
        if (TestType == ETestE32Test ||
            TestType == ETestForeverAll)
            {
            heap = CreateTestHeap(4);
            test(heap != NULL);
            test.Next(_L("one thread, align 4"));
            abortedByUser = DoStressTest1(heap);
            if (abortedByUser)
                {
                break;
                }
            heap->Close();
            
            heap = CreateTestHeap(4);
            test.Next(_L("two threads, align 4"));
            abortedByUser = DoStressTest2(heap);
            if (abortedByUser)
                {
                break;
                }
            heap->Close();
                
            heap = CreateTestHeap(8);
            test(heap != NULL);
            test.Next(_L("one thread, align 8"));
            abortedByUser = DoStressTest1(heap);
            if (abortedByUser)
                {
                break;
                }
            heap->Close();

            heap = CreateTestHeap(8);
            test.Next(_L("two threads, align 8"));
            abortedByUser = DoStressTest2(heap);
            }
        else if (TestType == ETestForeverOne)
            {
            heap = CreateTestHeap(4);
            test(heap != NULL);
            if (TestForeverMultiThreadTest)
                {
                test.Next(_L("two threads, align 4"));
                abortedByUser = DoStressTest2(heap);
                }
            else
                {
                test.Next(_L("one thread, align 4"));
                abortedByUser = DoStressTest1(heap);
                }
            }
        else
            {
            test(0);
            }
        break;
        }
    heap->Close();
    
    return abortedByUser;
    }           

        
void ForeverOneTest(void)
    {
    //--------- config parameters - begin
    TestForeverMultiThreadTest = ETrue; // EFalse
    TestHybridHeapFunc = ETstOnlySlab; // ETstOnlySlab // ETstOnlyDl // ETstOnlyPaged // ETstHybrid
    //--------- config parameters - end

    if (TestHybridHeapFunc == ETstOnlySlab && !DlOnly)
        {
        // slab tests
#ifdef __WINS__
        test.Next(_L("slab test 48M"));
        CurrMaxCellCount = 0x100000; //0x10000; 0x100000
        HeapMaxLength = 0x3000000; // 48M
#else
        test.Next(_L("slab test 3M"));
        CurrMaxCellCount = 0x10000; //0x10000; 0x100000
        HeapMaxLength = 0x300000; // 3M
#endif          
        StressTests();
        }
    else if (TestHybridHeapFunc == ETstOnlyDl)
        {
        // DL tests
        test.Next(_L("DL test 32M"));
        CurrMaxCellCount = 0x1000; //0x10000;
        HeapMaxLength = 0x2000000; // 32M
        StressTests();
        }
    else if (TestHybridHeapFunc == ETstOnlyPaged && !DlOnly)
        {
        // paged tests
        test.Next(_L("paged test 64M"));
        CurrMaxCellCount = 0x100; //0x10000;
        HeapMaxLength = 0x4000000; // 64M
        StressTests();
        }
    else if (TestHybridHeapFunc == ETstHybrid && !DlOnly)
        {
        // hybrid tests
        test.Next(_L("hybrid test 64M"));
        CurrMaxCellCount = 0x1000; //0x10000;
        HeapMaxLength = 0x4000000; // 64M
        StressTests();
        }
    else
        {
        test(0);
        }
    }
    
void ForeverAllTests(void)
    {
    //--------- config parameters - begin
    TInt basicTimeAsSeconds = 30; //10;
    //--------- config parameters - end

    for (;;)
        {
        if (!DlOnly)
            {
            // slab tests
            TestHybridHeapFunc = ETstOnlySlab;
            TestTimeAsSeconds = basicTimeAsSeconds * 3;
#ifdef __WINS__
            test.Next(_L("slab test 48M"));
            CurrMaxCellCount = 0x100000; //0x10000; 0x100000
            HeapMaxLength = 0x3000000; // 48M
#else
            test.Next(_L("slab test 3M"));
            CurrMaxCellCount = 0x10000; //0x10000; 0x100000
            HeapMaxLength = 0x300000; // 3M
#endif          
            if (StressTests())
                {
                break;
                }
            }
        
        // DL tests
        TestHybridHeapFunc = ETstOnlyDl;
        TestTimeAsSeconds = basicTimeAsSeconds;
        
        test.Next(_L("DL test 32M"));
        CurrMaxCellCount = 0x1000; //0x10000;
        HeapMaxLength = 0x2000000; // 32M
        if (StressTests())
            {
            break;
            }
        
        test.Next(_L("DL test 16M"));
        CurrMaxCellCount = 0x1000; //0x10000;
        HeapMaxLength = 0x1000000; // 16M
        if (StressTests())
            {
            break;
            }
        
        if (!DlOnly)
            {
            // paged tests
            TestHybridHeapFunc = ETstOnlyPaged;
            TestTimeAsSeconds = basicTimeAsSeconds;
            
            test.Next(_L("paged test 64M"));
            CurrMaxCellCount = 0x100; //0x10000;
            HeapMaxLength = 0x4000000; // 64M
            if (StressTests())
                {
                break;
                }
            }
        
        if (!DlOnly)
            {
            // hybrid tests
            TestHybridHeapFunc = ETstHybrid;
            TestTimeAsSeconds = basicTimeAsSeconds * 2;
            
            test.Next(_L("hybrid test 64M"));
            CurrMaxCellCount = 0x1000; //0x10000;
            HeapMaxLength = 0x4000000; // 64M
            if (StressTests())
                {
                break;
                }
            }
        }
    }
    
void TestUsedInE32Tests(void)
    {
    //--------- config parameters - begin
    TInt basicTimeAsSeconds = 10;
    //--------- config parameters - end

    if (!DlOnly)
        {
        // slab tests
        TestHybridHeapFunc = ETstOnlySlab;
        TestTimeAsSeconds = basicTimeAsSeconds * 3;
#ifdef __WINS__
        test.Next(_L("slab test 48M"));
        CurrMaxCellCount = 0x100000; //0x10000; 0x100000
        HeapMaxLength = 0x3000000; // 48M
#else
        test.Next(_L("slab test 3M"));
        CurrMaxCellCount = 0x10000; //0x10000; 0x100000
        HeapMaxLength = 0x300000; // 3M
#endif          
        StressTests();
        }
    
    // DL tests
    TestHybridHeapFunc = ETstOnlyDl;
    TestTimeAsSeconds = basicTimeAsSeconds;
    
    test.Next(_L("DL test 32M"));
    CurrMaxCellCount = 0x1000; //0x10000;
    HeapMaxLength = 0x2000000; // 32M
    StressTests();
    
    test.Next(_L("DL test 16M"));
    CurrMaxCellCount = 0x1000; //0x10000;
    HeapMaxLength = 0x1000000; // 16M
    StressTests();
    
    if (!DlOnly)
        {
        // paged tests
        TestHybridHeapFunc = ETstOnlyPaged;
        TestTimeAsSeconds = basicTimeAsSeconds;
        
        test.Next(_L("paged test 64M"));
        CurrMaxCellCount = 0x100; //0x10000;
        HeapMaxLength = 0x4000000; // 64M
        StressTests();
        }

    if (!DlOnly)
        {
        // hybrid tests
        TestHybridHeapFunc = ETstHybrid;
        TestTimeAsSeconds = basicTimeAsSeconds * 2;
        
        test.Next(_L("hybrid test 64M"));
        CurrMaxCellCount = 0x1000; //0x10000;
        HeapMaxLength = 0x4000000; // 64M
        StressTests();
        }
    }
    
TInt E32Main()
    {
    test.Title();
    __KHEAP_MARK;
    test.Start(_L("Testing heaps"));

    TInt err = UserHal::TickPeriod(TickPeriod);
    test(err == KErrNone);

    DlOnly = IsDlOnly();
    
    TestType = ETestE32Test; //ETestE32Test // ETestForeverOne // ETestForeverAll
    // see other config parameters: TestUsedInE32Tests()/ForeverOneTest()/ForeverAllTests()
   
    if (TestType == ETestE32Test)
        {
        TestUsedInE32Tests();
        }
    else if (TestType == ETestForeverOne)
        {
        ForeverOneTest();
        }
    else if (TestType == ETestForeverAll)
        {
        ForeverAllTests();
        }
    else
        {
        test(0);
        }
    
    test.End();
    __KHEAP_MARKEND;
    return 0;
    }
