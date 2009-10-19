// Copyright (c) 1998-2009 Nokia Corporation and/or its subsidiary(-ies).
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
// e32test\misc\t_mem.cpp
// 
//

#include <e32test.h>
#include "u32std.h"
#include "../misc/prbs.h"

RTest test(_L("T_MEM"));

TInt E32Main()
	{
	test.Title();
	test.Start(_L("Create chunk"));
	TMemoryInfoV1 minfo;
	TPckg<TMemoryInfoV1> infoPckg(minfo);
	TInt r = UserHal::MemoryInfo(infoPckg);
	test(r==KErrNone);
	test.Printf(_L("MaxFree = %08x\n"), minfo.iMaxFreeRamInBytes);
	test.Printf(_L("Free = %08x\n"), minfo.iFreeRamInBytes);
	TInt overhead = ((minfo.iFreeRamInBytes + 0x003FFFFF)>>22)<<12;
	test.Printf(_L("Overhead = %08x\n"), overhead);
	TInt size = Min(minfo.iFreeRamInBytes-0x00100000, minfo.iFreeRamInBytes-overhead);
	test.Printf(_L("Initial = %08x\n"), size);
	RChunk c;
	r=c.CreateLocal(size, minfo.iFreeRamInBytes);
	test(r==KErrNone);
	while (r==KErrNone)
		{
		r = c.Adjust(size + 0x1000);
		if (r==KErrNone)
			size += 0x1000;
		};
	test.Printf(_L("Final = %08x\n"), size);

	TUint seed[2];
	seed[0]=User::NTickCount();
	seed[1]=0;
	TUint8* p=c.Base();
	test.Printf(_L("Chunk Base %08x\n"),p);
	Mem::FillZ(p,size);
	TInt sizepg = size>>12;
	TUint iterations = 0;
	FOREVER
		{
		TUint x=Random(seed);
		x%=TUint(sizepg);
		TInt offset=x<<12;
		TUint init=Random(seed);
		TUint seed2[2];
		seed2[0]=init;
		seed2[1]=0;
		TUint* pW=(TUint*)(p+offset);
		*pW++=init;
		TInt i;
		for (i=1; i<1024; i++)
			*pW++=Random(seed2);
		TInt j;
		for (j=0; j<19; j++)
			{
			x=Random(seed);
			x%=TUint(sizepg);
			offset=x<<12;
			pW=(TUint*)(p+offset);
			seed2[0]=*pW++;
			seed2[1]=0;
			for (i=1; i<1024; i++)
				{
				TUint actual=*pW++;
				TUint expected=Random(seed2);
				TUint diff = actual ^ expected;
				if (diff)
					{
					test.Printf(_L("Address %08x: Got %08x Expected %08x Diff %08x\n"),pW-1,actual,expected,diff);
					pW[-1] = expected;
//					test.Getch();
//					test(0);
					}
				}
			}
		++iterations;
		if ((iterations & 0x3ff)==0)
			test.Printf(_L("%u\n"),iterations);
		}
	}
