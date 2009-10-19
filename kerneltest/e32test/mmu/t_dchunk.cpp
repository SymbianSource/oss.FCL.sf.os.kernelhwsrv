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
// e32test\mmu\t_dchunk.cpp
// 
//

#include <e32test.h>
#include <e32hal.h>
#include <e32panic.h>
#include <e32def.h>
#include <e32def_private.h>

LOCAL_D RTest test(_L("T_DCHUNK"));

TInt E32Main()
//
//	Test RChunk class
//
	{	
 
	test.Title();
	test.Start(_L("Test double-ended chunks"));
	__KHEAP_MARK;
	TMemoryInfoV1Buf meminfo1,meminfo2;
	TInt r=UserHal::MemoryInfo(meminfo1);
	test(r==KErrNone);
	test.Printf(_L("Free RAM %08X bytes\n"),meminfo1().iFreeRamInBytes);

	RChunk chunk1;
	r=chunk1.CreateDoubleEndedLocal(0x1000,0x3000,2*meminfo1().iFreeRamInBytes);
	test(r==KErrNone);
	TInt size = chunk1.Size();
	TInt bottom = chunk1.Bottom();
	TInt top = chunk1.Top();
	Mem::FillZ((TAny*)(chunk1.Base()+chunk1.Bottom()),chunk1.Size());

	r=UserHal::MemoryInfo(meminfo1);
	test (r==KErrNone);
	test.Printf(_L("Free RAM %08X bytes\n"),meminfo1().iFreeRamInBytes);

	r=chunk1.AdjustDoubleEnded(0,2*meminfo1().iFreeRamInBytes);
	test(r==KErrNoMemory);
	test(size==chunk1.Size());
	test(bottom==chunk1.Bottom());
	test(top==chunk1.Top());
	
	r=UserHal::MemoryInfo(meminfo2);
	test(r==KErrNone);
	test.Printf(_L("Free RAM %08X bytes\n"),meminfo2().iFreeRamInBytes);
	test(meminfo1().iFreeRamInBytes==meminfo2().iFreeRamInBytes);

	r=chunk1.AdjustDoubleEnded(0,0x6000);
	test(r==KErrNone);
	Mem::FillZ((TAny*)(chunk1.Base()+chunk1.Bottom()),chunk1.Size());
	
	chunk1.Close();
	
	test.End();
	test.Close();
	__KHEAP_MARKEND;
	return(KErrNone);
	}


