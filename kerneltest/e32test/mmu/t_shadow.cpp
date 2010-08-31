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
// e32test\mmu\t_shadow.cpp
// Overview:
// Test ROM shadowing
// API Information:
// RBusLogicalChannel
// Details:
// - Load and open the logical device driver ("D_SHADOW.LDD"). Verify
// results.
// - Allocate a shadow ROM page, verify data is copied into shadow page
// without altering the following page.
// - Move the shadow page and the original rom page.
// - Write and verify data in the shadow page.
// - Free the shadow page and verify original ROM data is restored.
// - Test pinning of shadow pages.
// Platforms/Drives/Compatibility:
// All.
// Assumptions/Requirement/Pre-requisites:
// Failures and causes:
// Base Port information:
// 
//

#define __E32TEST_EXTENSION__

#include <e32test.h>
#include <e32uid.h>
#include <e32hal.h>
#include "d_shadow.h"
#include "d_gobble.h"
#include "..\defrag\d_pagemove.h"
#include "d_memorytest.h"
#include <dptest.h>
#include "mmudetect.h"
#include "freeram.h"

enum TPaged
	{
	EUnpaged,
	EPaged
	};

_LIT(KLddFileName, "D_SHADOW.LDD");
_LIT(KMoveLddFileName, "D_PAGEMOVE.LDD");

LOCAL_D RTest test(_L("T_SHADOW"));

RPageMove PageMove;
RShadow Shadow;
RMemoryTestLdd MemoryTest;

TInt PageSize;
LOCAL_D TBool RomPagingSupported = EFalse;
#ifndef __X86__
LOCAL_D TBool PageMovingSupported = ETrue;
#else
LOCAL_D TBool PageMovingSupported = EFalse;
#endif

TLinAddr RomUnpagedStart = 0;
TLinAddr RomUnpagedEnd = 0;
TLinAddr RomPagedStart = 0;
TLinAddr RomPagedEnd = 0;

TUint8* PageBuffer1 = NULL;
TUint8* PageBuffer2 = NULL;
TUint8* PageBuffer3 = NULL;

void TestShadowPage(TLinAddr aPageAddr, TPaged aPageType)
	{
	test.Start(_L("Test shadowing a page"));	
	test.Printf(_L("  addr == 0x%08x, type == %d\n"), aPageAddr, aPageType);

	test.Next(_L("Copy page to be shadowed and following page to local buffers"));
	TLinAddr secondPage = aPageAddr + PageSize;
	if (secondPage >= RomPagedEnd)
		secondPage = RomUnpagedStart;
	Mem::Move(PageBuffer1,(TAny*)aPageAddr,PageSize);
	Mem::Move(PageBuffer3,(TAny*)(secondPage),PageSize);
	
	TUint origPhysAddr = 0xffffffff;	// The physical address of the rom page to be shadowed
	
	if (PageMovingSupported && aPageType == EPaged)
		{
		test.Next(_L("Test page can be moved"));
		TUint dummy = *(volatile TUint32*)aPageAddr;  // ensure paged in
		test_KErrNone(PageMove.TryMovingUserPage((TAny*)aPageAddr));
		
		test.Next(_L("Get page's physical address"));
		dummy += *(volatile TUint32*)aPageAddr;  // ensure paged in
		test_KErrNone(PageMove.GetPhysAddr((TAny*)aPageAddr, (TAny*)&origPhysAddr));
		test.Printf(_L("  physical address: %08x\n"), origPhysAddr);
		}

	test.Next(_L("Allocate a shadow ROM page"));
	test_KErrNone(Shadow.Alloc(aPageAddr));

	test.Next(_L("Try to shadow the page again"));
	test_Equal(KErrAlreadyExists, Shadow.Alloc(aPageAddr));

	if (PageMovingSupported && aPageType == EPaged)
		{
		test.Next(_L("Check page's physical address has changed"));
		TUint newPhysAddr;
		test_KErrNone(PageMove.GetPhysAddr((TAny*)aPageAddr, (TAny*)&newPhysAddr));
		test(newPhysAddr != origPhysAddr);
		
		test.Next(_L("Test moving a shadowed page is not allowed"));
		test_Equal(KErrNotSupported, PageMove.TryMovingUserPage((TAny*)aPageAddr));
	
		test.Next(_L("Test moving the original page fails (it should be pinned)"));
		test_Equal(KErrInUse, PageMove.TryMovingPhysAddr((TAny*)origPhysAddr, (TAny*)&newPhysAddr));
		}

	test.Next(_L("Check data copied into shadow page"));
	test_Equal(0, Mem::Compare((TUint8*)aPageAddr,PageSize,PageBuffer1,PageSize));

	test.Next(_L("Check page following shadow page is unaltered"));
	test_Equal(0, Mem::Compare((TUint8*)(secondPage),PageSize,PageBuffer3,PageSize));

	test.Next(_L("Write data into shadow page"));
	for(TInt i=0; i<PageSize; i++)
		{
		TInt i2=i*i;
		PageBuffer2[i]=TUint8(i2^(i2>>8)^(i2>>16));
		}
	test_KErrNone(Shadow.Write(aPageAddr,PageBuffer2));

	test.Next(_L("Check data written into shadow page"));
	test_Equal(0, Mem::Compare((TUint8*)aPageAddr,PageSize,PageBuffer2,PageSize));
	
	test.Next(_L("Check page following shadow page is unaltered"));
	test_Equal(0, Mem::Compare((TUint8*)(secondPage),PageSize,PageBuffer3,PageSize));

	test.Next(_L("Allocate another shadow ROM page"));
	test_KErrNone(Shadow.Alloc(secondPage));

	test.Next(_L("Free the original shadow page"));
	test_KErrNone(Shadow.Free(aPageAddr));

	test.Next(_L("Check original ROM data restored"));
	test_Equal(0, Mem::Compare((TUint8*)aPageAddr,PageSize,PageBuffer1,PageSize));

	if (PageMovingSupported && aPageType == EPaged)
		{
		test.Next(_L("Test page can be moved again"));
		test_KErrNone(PageMove.TryMovingUserPage((TAny*)aPageAddr));
		}
	
	test.Next(_L("Free the second shadow page"));
	test_KErrNone(Shadow.Free(secondPage));

	test.Next(_L("Check original ROM data restored"));
	test_Equal(0, Mem::Compare((TUint8*)(secondPage),PageSize,PageBuffer3,PageSize));
	test.End();
	}

/*
    Reintroduce this when RTest can report whether the test is
    being run in automatic or manual mode
*/
void TestFreeze(TLinAddr aPageAddr)
	{
	test.Start(_L("Test freezing a shadow page"));	
	test.Printf(_L("Press 0 to test Freeze (causes kernel fault)\n"));
	test.Printf(_L("Press any other key to skip this test\n"));
	TKeyCode key=test.Getch();
	if (key==TKeyCode('0'))
		{
		test.Next(_L("Freeze first shadow page"));
		test_KErrNone(Shadow.Freeze(aPageAddr));

		test.Printf(_L("Press a key to attempt write after freezing\n"));
		test.Printf(_L("Should get Kernel Exception 9, Data Address 50000xxx\n"));
		test.Getch();
		Shadow.Write(aPageAddr,PageBuffer2);
		test(0);
		}
	test.End();
	}

void TestNoFreeRAM(TLinAddr aPageAddr)
	{
	test.Start(_L("Test allocating a shadow page when all free RAM is in 'chunk caches'"));
	
	// Remove limit on max size of live list
	TUint originalMin = 0;
	TUint originalMax = 0;
	TUint currentSize = 0;
	TInt r = DPTest::CacheSize(originalMin, originalMax, currentSize);
	test_Value(r, r == KErrNone || r == KErrNotSupported);
	TBool resizeCache = r == KErrNone;
	if (resizeCache)
		test_KErrNone(DPTest::SetCacheSize(originalMin, KMaxTUint));

	test.Next(_L("Load gobbler LDD"));
	r = User::LoadLogicalDevice(KGobblerLddFileName);
	test_Value(r, r==KErrNone || r==KErrAlreadyExists);
	RGobbler gobbler, gobbler2;
	test_KErrNone(gobbler.Open());
	TUint32 taken = gobbler.GobbleRAM(496*1024*1024);
	test.Printf(_L("  Gobbled: %dK\n"), taken/1024);
	test.Printf(_L("  Free RAM 0x%08X bytes\n"),FreeRam());
	//  Open 2nd globber here, while we still have some free pages.
	test_KErrNone(gobbler2.Open());


	// put all of free RAM in a chunk...
	TChunkCreateInfo createInfo;
	createInfo.SetCache(512*1024*1024);
	RChunk testChunk;
	test_KErrNone(testChunk.Create(createInfo));
	TInt commitEnd = 0;
	while(KErrNone==(r=testChunk.Commit(commitEnd,PageSize)))
		commitEnd += PageSize;
	test_Equal(KErrNoMemory,r);

	// Now we have some memory in a cache chunk ensure definitely no 
	// other free pages.
	taken = gobbler2.GobbleRAM(0);
	test.Printf(_L("  Gobbled: %dK\n"), taken/1024);
	test_Equal(0, FreeRam());


	// no memory to allocate shadow page...
	test_Equal(KErrNoMemory,Shadow.Alloc(aPageAddr));
	// unlock all of RAM in chunk...
	test_KErrNone(testChunk.Unlock(0,commitEnd));
	// should have memory now...
	test_KErrNone(Shadow.Alloc(aPageAddr));
	// tidy up...
	test_KErrNone(Shadow.Free(aPageAddr));
	testChunk.Close();

	// Restore original settings for live list size
	if (resizeCache)
		test_KErrNone(DPTest::SetCacheSize(originalMin, originalMax));

	gobbler.Close();
	gobbler2.Close();
	test.End();
	}

void TestShadowPageOOM(TLinAddr aPageAddr)
	{
	test.Start(_L("Test OOM while shadowing a page"));
	test.Printf(_L("  addr == 0x%08x\n"), aPageAddr);

	test.Next(_L("Copy page to be shadowed and following page to local buffers"));
	Mem::Move(PageBuffer1,(TAny*)aPageAddr,PageSize);

	__KHEAP_MARK;
	
	TInt r;
	TInt failCount = 0;
	test.Next(_L("Allocate a shadow ROM page"));
	do
		{
		__KHEAP_FAILNEXT(failCount);
		r = Shadow.Alloc(aPageAddr);
		if (r == KErrNoMemory)
			++failCount;
		}
	while (r == KErrNoMemory);
	__KHEAP_RESET;
	test.Printf(_L("  returned %d after %d allocations\n"), r, failCount);
	test_KErrNone(r);

	test.Next(_L("Try to shadow the page again"));
	__KHEAP_FAILNEXT(0);
	test_Equal(KErrAlreadyExists, Shadow.Alloc(aPageAddr));
	__KHEAP_RESET;

	test.Next(_L("Check data copied into shadow page"));
	test_Equal(0, Mem::Compare((TUint8*)aPageAddr,PageSize,PageBuffer1,PageSize));

	test.Next(_L("Write data into shadow page"));
	for(TInt i=0; i<PageSize; i++)
		{
		TInt i2=i*i;
		PageBuffer2[i]=TUint8(i2^(i2>>8)^(i2>>16));
		}
	test_KErrNone(Shadow.Write(aPageAddr,PageBuffer2));

	test.Next(_L("Check data written into shadow page"));
	test_Equal(0, Mem::Compare((TUint8*)aPageAddr,PageSize,PageBuffer2,PageSize));

	test.Next(_L("Free the original shadow page"));
	__KHEAP_FAILNEXT(0);
	test_KErrNone(Shadow.Free(aPageAddr));
	__KHEAP_RESET;
	
	test.Next(_L("Check original ROM data restored"));
	test_Equal(0, Mem::Compare((TUint8*)aPageAddr,PageSize,PageBuffer1,PageSize));

	test.Next(_L("Check kernel heap balance"));
	__KHEAP_MARKEND;

	test.End();
	}

void TestInteractionWithPinning(TLinAddr aPageAddr)
	{
	test.Start(_L("Test pinning of shadow pages"));
	
	// Create pin object to use for all pinnings
	test_KErrNone(MemoryTest.CreateVirtualPinObject());

	test.Next(_L("Test pin - shadow - unpin - unshadow"));
	test_Equal(KErrNone, MemoryTest.PinVirtualMemory((TLinAddr)aPageAddr, PageSize));
	test_KErrNone(Shadow.Alloc(aPageAddr));
	test_Equal(KErrNone, MemoryTest.UnpinVirtualMemory());
	test_KErrNone(Shadow.Free(aPageAddr));
		
	test.Next(_L("Test pin - shadow - unshadow - unpin"));
	test_Equal(KErrNone, MemoryTest.PinVirtualMemory((TLinAddr)aPageAddr, PageSize));
	test_KErrNone(Shadow.Alloc(aPageAddr));
	test_KErrNone(Shadow.Free(aPageAddr));
	test_Equal(KErrNone, MemoryTest.UnpinVirtualMemory());

	test.Next(_L("Test shadow - pin - unpin - unshadow"));
	test_KErrNone(Shadow.Alloc(aPageAddr));
	test_Equal(KErrNone, MemoryTest.PinVirtualMemory((TLinAddr)aPageAddr, PageSize));
	test_Equal(KErrNone, MemoryTest.UnpinVirtualMemory());
	test_KErrNone(Shadow.Free(aPageAddr));

	test.Next(_L("Test shadow - pin - unshadow - unpin"));
	test_KErrNone(Shadow.Alloc(aPageAddr));
	test_Equal(KErrNone, MemoryTest.PinVirtualMemory((TLinAddr)aPageAddr, PageSize));
	test_KErrNone(Shadow.Free(aPageAddr));
	test_Equal(KErrNone, MemoryTest.UnpinVirtualMemory());

	test_KErrNone(MemoryTest.DestroyVirtualPinObject());
	test.End();
	}

const TUint KChunkShift = 20;
const TUint KChunkSize = 1 << KChunkShift;

const TUint KRomSizeAlign = 16;  // This should match CFG_RomSizeAlign defined in bootcpu.inc
const TUint KRomSizeAlignMask = (1 << KRomSizeAlign) - 1;

void TestRomIsSectionMapped()
	{
	test.Start(_L("Test ROM is section mapped"));
	
	TUint pdSize;
	TUint pdBase;
	TUint offset;

#ifdef __MARM__
	test_KErrNone(Shadow.GetPdInfo(KGlobalPageDirectory, pdSize, pdBase, offset));
	test.Printf(_L("pd base == %08x, pd size == %08x, pd offset == %08x\n"), pdBase, pdSize, offset);

	TUint romSize = RomUnpagedEnd - RomUnpagedStart;	
	test.Printf(_L("rom size == %x\n"), romSize);
	if (RomPagedStart == RomPagedEnd)
		{
		// If rom is not paged then we must round the ROM size up to a mutiple of 64KB (or whatever
		// the CFG_RomSizeAlign settings is), because that's how the bootstrap maps it
		romSize = (romSize + KRomSizeAlignMask) & ~KRomSizeAlignMask;
		test.Printf(_L("rom size rounded up to %x\n"), romSize);
		}
	
	for (TUint pos = 0 ; pos < romSize ; pos += KChunkSize)
		{
		TLinAddr addr = RomUnpagedStart + pos;
		TUint i = (addr >> KChunkShift) - offset;
		TUint pde = Shadow.Read(pdBase + i*4);
		test.Printf(_L("  %08x: PDE %08x\n"), addr, pde);

		TUint expectedPdeType = (romSize - pos) >= KChunkSize ? 2 : 1;
		test_Equal(expectedPdeType, pde & 3);
		}
#else
	test.Printf(_L("Test not supported on this architecture\n"));
#endif

	test.End();
	}

void Initialise()
	{
	test.Start(_L("Load test LDDs"));
	
	TInt r=User::LoadLogicalDevice(KLddFileName);
	test_Value(r, r==KErrNone || r==KErrAlreadyExists);
	if (PageMovingSupported)
		{
		r=User::LoadLogicalDevice(KMoveLddFileName);
		test_Value(r, r==KErrNone || r==KErrAlreadyExists);
		}

	test_KErrNone(UserHal::PageSizeInBytes(PageSize));

	test.Next(_L("Open test LDDs"));
	test_KErrNone(Shadow.Open());
	test_KErrNone(MemoryTest.Open());
	if (PageMovingSupported)
		test_KErrNone(PageMove.Open());

	test.Next(_L("Allocate some RAM"));
	PageBuffer1=(TUint8*)User::Alloc(PageSize);
	test_NotNull(PageBuffer1);
	PageBuffer2=(TUint8*)User::Alloc(PageSize);
	test_NotNull(PageBuffer2);
	PageBuffer3=(TUint8*)User::Alloc(PageSize);
	test_NotNull(PageBuffer3);

	test.Next(_L("Discover ROM addresses"));
	TRomHeader* romHeader = (TRomHeader*)UserSvr::RomHeaderAddress();
	RomUnpagedStart = (TLinAddr)romHeader;
	test_Equal(0, RomUnpagedStart & (PageSize - 1));

	//Round up to page size (May already be rounded, but doesnt hurt)
	TUint romSize = (romHeader->iUncompressedSize + PageSize - 1) & ~(PageSize - 1);
	
	if (DPTest::Attributes() & DPTest::ERomPaging)
		{
		// Use paged part of rom for testing
		test_NotNull(romHeader->iPageableRomStart);
		RomUnpagedEnd = RomUnpagedStart + romHeader->iPageableRomStart;
		test_Equal(0, RomUnpagedEnd & (PageSize - 1));
		RomPagedStart = RomUnpagedEnd;
		RomPagedEnd = RomUnpagedStart + romSize;
		RomPagingSupported = ETrue;
		}
	else
		{
		RomUnpagedEnd = RomUnpagedStart + romSize;
		RomPagedStart = RomUnpagedEnd;
		RomPagedEnd = RomPagedStart;
		}

	test.Printf(_L("Unpaged ROM: %08x -> %08x\n"), RomUnpagedStart, RomUnpagedEnd);
	test.Printf(_L("Paged ROM:   %08x -> %08x\n"), RomPagedStart, RomPagedEnd);

	test.End();
	}

void Finalise()
	{
	PageMove.Close();
	MemoryTest.Close();

	User::Free(PageBuffer1);
	User::Free(PageBuffer2);
	User::Free(PageBuffer3);
	}

GLDEF_C TInt E32Main()
//
// Test ROM shadowing
//
    {
	test.Title();
	
	if (!HaveMMU())
		{
		test.Printf(_L("This test requires an MMU\n"));
		return KErrNone;
		}
#ifdef __WINS__
	test.Printf(_L("Test not valid in WINS\n"));
#else
	// Turn off lazy dll unloading
	RLoader l;
	test_KErrNone(l.Connect());
	test_KErrNone(l.CancelLazyDllUnload());
	l.Close();

	test.Start(_L("Testing ROM shadowing"));
	Initialise();

	TestRomIsSectionMapped(); 

	TestShadowPage(RomUnpagedStart, EUnpaged);
	TestShadowPage(RomUnpagedStart + PageSize, EUnpaged);
	TestShadowPage(RomUnpagedEnd - PageSize, EUnpaged);
	TestNoFreeRAM(RomUnpagedStart);
	TestShadowPageOOM(RomUnpagedStart);
	
	if (RomPagingSupported)
		{
		TestShadowPage(RomPagedStart, EPaged);
		TestShadowPage(RomPagedStart + PageSize, EPaged);
		TestShadowPage(RomPagedEnd - PageSize, EPaged);
		TestNoFreeRAM(RomPagedEnd - PageSize);
		TestShadowPageOOM(RomPagedStart);
		TestInteractionWithPinning(RomPagedStart);
		}

	// todo: add test when reforming section mappings is implemented
	// TestRomIsSectionMapped();
	
	Finalise();
	test.End();
	
#endif
	return(KErrNone);
    }

