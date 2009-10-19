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
// e32test\mmu\t_ramall.cpp
// 
//

#define __E32TEST_EXTENSION__

#include <e32test.h>
#include <e32uid.h>
#include <e32hal.h>
#include "d_shadow.h"
#include "mmudetect.h"
#include "freeram.h"

LOCAL_D RTest test(_L("T_RAMALL"));

_LIT(KLddFileName,"D_SHADOW.LDD");

TInt PageSize;
TInt PageShift;
RShadow Shadow;
TInt InitFreeRam;

TInt AllocPhysicalRam(TUint32& aAddr, TInt aSize, TInt aAlign)
	{
	return Shadow.AllocPhysicalRam(aAddr,aSize,aAlign);
	}

TInt FreePhysicalRam(TUint32 aAddr, TInt aSize)
	{
	return Shadow.FreePhysicalRam(aAddr,aSize);
	}

TInt ClaimPhysicalRam(TUint32 aAddr, TInt aSize)
	{
	return Shadow.ClaimPhysicalRam(aAddr,aSize);
	}

void TestAlignedAllocs()
	{
	TInt align;
	TInt size;
	for (align=PageShift; align<=20; ++align)
		{
		for (size=PageSize; size<=0x100000; size+=PageSize)
			{
			TInt free=FreeRam();
			TUint32 pa=0;
			TInt r=AllocPhysicalRam(pa,size,align);
			test.Printf(_L("Size %08x Align %d r=%d pa=%08x\n"),size,align,r,pa);
			if (r==KErrNone)
				{
				TUint32 as=1u<<align;
				TUint32 am=as-1;
				test(FreeRam()==free-size);
				test((pa&am)==0);
				r=FreePhysicalRam(pa,size);
				test(r==KErrNone);
				}
			test(FreeRam()==free);
			}
		}
	}

void TestClaimPhys()
	{
	TInt free=FreeRam();
	TUint32 pa=0;
	TInt r=AllocPhysicalRam(pa,4*PageSize,0);
	test(r==KErrNone);
	test(FreeRam()==free-4*PageSize);
	r=FreePhysicalRam(pa,4*PageSize);
	test(r==KErrNone);
	test(FreeRam()==free);
	r=ClaimPhysicalRam(pa,4*PageSize);
	test(r==KErrNone);
	test(FreeRam()==free-4*PageSize);
	r=FreePhysicalRam(pa,3*PageSize);
	test(r==KErrNone);
	test(FreeRam()==free-PageSize);
	r=ClaimPhysicalRam(pa,4*PageSize);
	test(r==KErrInUse);
	test(FreeRam()==free-PageSize);
	if (HaveVirtMem())
		{
		r=FreePhysicalRam(pa,4*PageSize);
		test(r==KErrGeneral);
		test(FreeRam()==free-PageSize);
		}
	r=FreePhysicalRam(pa+3*PageSize,PageSize);
	test(r==KErrNone);
	test(FreeRam()==free);
	}

GLDEF_C TInt E32Main()
//
// Test RAM allocation
//
    {
	test.Title();
	test.Start(_L("Load test LDD"));
	TInt r=User::LoadLogicalDevice(KLddFileName);
	test(r==KErrNone || r==KErrAlreadyExists);

	r=UserHal::PageSizeInBytes(PageSize);
	test(r==KErrNone);

	TInt psz=PageSize;
	PageShift=-1;
	for (; psz; psz>>=1, ++PageShift);

	InitFreeRam=FreeRam();
	test.Printf(_L("Free RAM=%08x, Page size=%x, Page shift=%d\n"),InitFreeRam,PageSize,PageShift);

	test.Next(_L("Open test LDD"));
	r=Shadow.Open();
	test(r==KErrNone);

	TestAlignedAllocs();
	TestClaimPhys();

	Shadow.Close();
	test.End();
	return(KErrNone);
    }

