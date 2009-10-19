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
// e32test\mmu\t_wbc.cpp
// Overview:
// Read and write RChunk data.
// API Information:
// RChunk
// Details:
// Create two local RChunk objects. Perform ReadModifyWrite, WriteOnly 
// and ReadOnly operations on the chunks. Verify that the results are
// as expected.
// Platforms/Drives/Compatibility:
// All.
// Assumptions/Requirement/Pre-requisites:
// Failures and causes:
// Base Port information:
// 
//

#include <e32test.h>
#include <e32hal.h>
#include <e32svr.h>

RTest test(_L("T_WBC"));

TUint32 ReadModifyWrite(volatile TUint32* p, TInt aNumWords)
	{
	TUint32 x=0;
	while(aNumWords--)
		{
		x=*p;
		*p++=0;
		}
	return x;
	}

void WriteOnly(volatile TUint32* p, TInt aNumWords)
	{
	while(aNumWords--)
		*p++=0xffffffff;
	}

TUint32 ReadOnly(volatile TUint32* p, TInt aNumWords)
	{
	TUint32 x=0;
	while(aNumWords--)
		x+=*p++;
	return x;
	}

void Dump(volatile TUint32* p, TInt aNumWords)
	{
	while(aNumWords>0)
		{
		RDebug::Print(_L("%08x: %08x %08x %08x %08x %08x %08x %08x %08x"),p,
			p[0],p[1],p[2],p[3],p[4],p[5],p[6],p[7]);
		p+=8;
		aNumWords-=8;
		}
	}

void Verify(volatile TUint32* p, TInt aNumWords)
	{
	while(aNumWords--)
		test(*p++==0xffffffff);
	}

GLDEF_C TInt E32Main()
	{
	
	TInt pageSize;
	UserHal::PageSizeInBytes(pageSize);
	test.Title();
	test.Start(_L("Create chunk 1"));
	RChunk c1;
	TInt r=c1.CreateLocal(4*pageSize,0x100000);	// initial size 4 pages
	test(r==KErrNone);
	test.Next(_L("Create chunk 2"));
	RChunk c2;
	r=c2.CreateLocal(pageSize,0x100000);		// initial size 1 page
	test(r==KErrNone);
	test.Next(_L("Fill DCache with modified lines from chunk 1"));
	volatile TUint32* p=(volatile TUint32*)(c1.Base()+2*pageSize);
	ReadModifyWrite(p,pageSize>>1);
	c1.Adjust(2*pageSize);		// lose the two modified pages
	c2.Adjust(3*pageSize);		// map them back into chunk 2
	p=(volatile TUint32*)(c2.Base()+pageSize);
	WriteOnly(p,pageSize>>1);	// write to the modified pages at new virtual address
	p=(volatile TUint32*)c1.Base();
	ReadOnly(p,pageSize>>1);	// read two other pages (this will evict the modified lines)
	p=(volatile TUint32*)(c2.Base()+pageSize);
	Dump(p,pageSize>>1);
	Verify(p,pageSize>>1);
	test.Printf(_L("\n"));
	test.End();
	return 0;
	}
