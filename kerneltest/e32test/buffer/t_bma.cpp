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
// e32test\buffer\t_bma.cpp
// Overview:
// Test the bitmap allocation abilities of the CBitMapAllocator class.
// API Information:
// CBitMapAllocator.
// Details:
// - Create an instance of CBitMapAllocator class with positive size using New and NewL methods, 
// verify that object is created and deleted successfully, test the heap allocation failure. 
// - Verify that the heap has not been corrupted by the test.
// - Test Alloc, AllocFromTop, AllocAt, Free, and AllocFromTopFrom methods of 
// CBitMapAllocator class are as expected.
// - Allocate all available memory using Alloc, AllocFromTop, AllocFromTopFrom
// and check that available free space is zero.
// - Allocate more than available memory using Alloc, AllocFromTop,
// AllocFromTopFrom and check the return value is KErrorNoMemory.
// - Free the memory and check that available free space is equal to the size.
// - Allocate at specified blocks, check the allocation and available free block 
// is as expected.
// - Free the block and check the available space is as expected. 
// - Check the alignment of blocks after allocation is as expected.
// - Perform all of the above tests for CBitMapAllocator size of 1, 4, 32, 33, 68, 96, 64, 65 and 63 bits.
// - Allocate some contiguous pages of RAM from the kernel's free page pool with pattern of 
// increasingly large gaps and test that the pages are allocated as specified.
// - Check KErrorNoMemory is returned when extracting a page beyond the available space.
// - Perform a test specifically for defect EXT-5AMDKP, Alloc, Free and ExtractRamPages. Test for 
// expected results.
// - Test whether the heap has been corrupted by any of the tests.
// Platforms/Drives/Compatibility:
// All 
// Assumptions/Requirement/Pre-requisites:
// Failures and causes:
// Base Port information:
// 
//

#include <e32test.h>
#include <e32base.h>
#include <e32base_private.h>
#include <e32def.h>
#include <e32def_private.h>

const TInt KMaxAllocations=50;

LOCAL_D RTest test(_L("T_BMA"));

LOCAL_C void testNew(TInt aSize)
//
//	Test New
//
	{

	test.Start(_L("New"));
	__UHEAP_MARK;
	CBitMapAllocator* pBitMapAllocator=CBitMapAllocator::New(aSize);
	test(pBitMapAllocator!=NULL);
	test(pBitMapAllocator->Size()==pBitMapAllocator->Avail());
	delete pBitMapAllocator;
	__UHEAP_CHECK(0);
	for (TInt i=1;i<KMaxAllocations;i++)
		{
		test.Printf(_L("Try %d\n"),i);
		__UHEAP_SETFAIL(RHeap::EDeterministic,i);
		pBitMapAllocator=CBitMapAllocator::New(aSize);
		if (pBitMapAllocator!=NULL)
			break;
		__UHEAP_CHECK(0);
		}
	delete pBitMapAllocator;
	__UHEAP_MARKEND;
	__UHEAP_RESET;
	test.End();
	}

LOCAL_C void testNewL(TInt aSize)
//
//	Test NewL
//
	{

	test.Start(_L("NewL"));
	__UHEAP_MARK;
	CBitMapAllocator* pBitMapAllocator=CBitMapAllocator::NewL(aSize);
	test(pBitMapAllocator!=NULL);
	test(pBitMapAllocator->Size()==pBitMapAllocator->Avail());
	delete pBitMapAllocator;
	__UHEAP_CHECK(0);
	test.Next(_L("Repetitive NewL"));
	for (TInt i=1;i<KMaxAllocations;i++)
		{
		test.Printf(_L("Try %d\n"),i);
		__UHEAP_SETFAIL(RHeap::EDeterministic,i);
		TRAPD(r,pBitMapAllocator=CBitMapAllocator::NewL(aSize));
		if (r==KErrNone)
			break;
		__UHEAP_CHECK(0);
		}
	delete pBitMapAllocator;
	__UHEAP_MARKEND;
	__UHEAP_RESET;
  	test.End();	
	}

LOCAL_C void testAlloc(TInt aSize)
//
//	Test Alloc, AllocFromTop, AllocAt, and Free, and AllocFromTopFrom
//
	{

	CBitMapAllocator* pBitMapAllocator=CBitMapAllocator::New(aSize);
	test(pBitMapAllocator!=NULL);
	test.Start(_L("Alloc all available"));
	TInt available=pBitMapAllocator->Avail();
	TInt i=0;
	for (;i<available;i++)
		{
		TInt j=pBitMapAllocator->Alloc();
		test(j==i);
		}
	test(pBitMapAllocator->Avail()==0);
//
	test.Next(_L("Try to alloc more than available"));
	i=pBitMapAllocator->Alloc();
	test(i==KErrNoMemory);
//
	test.Next(_L("Free"));
	for (i=0;i<available;i++)
		pBitMapAllocator->Free(i);
	test(pBitMapAllocator->Avail()==pBitMapAllocator->Size());
//
	test.Next(_L("AllocFromTop"));	
	for (i=available-1;i>=0;i--)
		{
		TInt j=pBitMapAllocator->AllocFromTop();
		test(j==i);
		}
	test(pBitMapAllocator->Avail()==0);
//
	test.Next(_L("Try to AllocFromTop more than available"));
	i=pBitMapAllocator->AllocFromTop();
	test(i==KErrNoMemory);
//
	test.Next(_L("Free (again)"));
	for (i=0;i<available;i++)
		pBitMapAllocator->Free(i);
	test(pBitMapAllocator->Avail()==pBitMapAllocator->Size());
//
	test.Next(_L("AllocFrom"));
	i=0;
	for (;i<available;i++)
		{
		TInt j=pBitMapAllocator->AllocFrom(i);
		test(j==i);
		}
	test(pBitMapAllocator->Avail()==0);

	test.Next(_L("Try AllocFrom for already allocated pos")); //should return KErrNoMemory
	TInt j=pBitMapAllocator->AllocFrom(i-1);
	test(j==KErrNoMemory);

	test.Next(_L("Free (again)"));
	for (i=0;i<available;i++)
		{
		pBitMapAllocator->Free(i);
		}
	test(pBitMapAllocator->Avail()==pBitMapAllocator->Size());
//

	test.Next(_L("AllocAt"));
	pBitMapAllocator->AllocAt(aSize-1);
	test(pBitMapAllocator->Avail()==pBitMapAllocator->Size()-1);

	test.Next(_L("Free (again)"));
	pBitMapAllocator->Free(aSize-1);
	test(pBitMapAllocator->Avail()==pBitMapAllocator->Size());
//
	test.Next(_L("AllocFromTopFrom"));
	TInt x;
	for (x=available-1;x>0;x--)
		{
		for (i=x;i>=0;i--)
			{
			TInt j=pBitMapAllocator->AllocFromTopFrom(x);
			test(j==i);
			test(!pBitMapAllocator->IsFree(j));
			}
		test(pBitMapAllocator->Avail()==available-x-1);

		test.Next(_L("Try to AllocFromTopFrom more than available"));
		i=pBitMapAllocator->AllocFromTopFrom(x);
		test(i==KErrNoMemory);
//
		TInt y;
		for (y=0;y<=x;y++)
			{
			for (i=0;i<=x;i++)
				{
				if (pBitMapAllocator->Avail()<=available-x-1)
					pBitMapAllocator->Free(y);
				TInt j=pBitMapAllocator->AllocFromTopFrom(i);
				if (i<y)
					test(j==KErrNoMemory);
				else
					{
					test(j==y);
					test(!pBitMapAllocator->IsFree(j));
					}
				}
			}
	
//
		test.Next(_L("Free (again)"));
		for (i=0;i<=x;i++)
			pBitMapAllocator->Free(i);
		test(pBitMapAllocator->Avail()==pBitMapAllocator->Size());
		}
//
	for (x=available-1;x>0;x--)
		{
		for (i=x;i>=0;i--)
			{
			TInt j=pBitMapAllocator->AllocFromTopFrom(x);
			test(j==i);
			}
		test(pBitMapAllocator->Avail()==available-x-1);

		test.Next(_L("Try to AllocFromTopFrom more than available"));
		i=pBitMapAllocator->AllocFromTopFrom(x);
		test(i==KErrNoMemory);
	//
		test.Next(_L("Free (again)"));
		for (i=0;i<=x;i++)
			pBitMapAllocator->Free(i);
		test(pBitMapAllocator->Avail()==pBitMapAllocator->Size());
		}
	test.End();
	delete pBitMapAllocator;
	}

LOCAL_C void testBlock(TInt aSize)
//
// Test Alloc(TInt, TInt&), AllocAligned, AllocAlignedBlock, AllocAt(TInt, TInt),
// IsFree(TInt, TInt), Free(TInt, TInt)
//
	{
	CBitMapAllocator* pB=CBitMapAllocator::New(aSize);
	test(pB!=NULL);
	test.Start(_L("AllocAt block, Free block, IsFree block"));
	TInt available=pB->Avail();
	test(available==aSize);
	TInt start, len;
	for(start=0; start<available; start++)
		{
		for(len=1; len<=available-start; len++)
			{
			pB->AllocAt(start,len);
			test(pB->Avail()==available-len);
			for(TInt i=0; i<available; i++)
				{
				if (i>=start && i<start+len)
					{
					if(pB->IsFree(i))
						test(0);
					}
				else
					{
					if(!pB->IsFree(i))
						test(0);
					}
				}
			if (start)
				test(pB->IsFree(0,start));
			test(!pB->IsFree(0,start+1));
			if (start+len<available)
				{
				test(pB->IsFree(start+len,available-(start+len)));
				test(!pB->IsFree(start+len-1,available-(start+len-1)));
				}
			pB->Free(start,len);
			test(pB->Avail()==available);
			test(pB->IsFree(start,len));
			test(pB->IsFree(0,available));
			}
		}
	test.End();
	test.Start(_L("Alloc consecutive block"));
	TInt askfor, init, pos, consec;
	for(askfor=1; askfor<=available; askfor++)
		{
		test.Printf(_L("Ask for %d\n"),askfor);
		for(init=0; init<available; init++)
			{
			if (init)
				pB->AllocAt(0,init);
			for(pos=init+1; pos<available; pos++)
				{
				pB->AllocAt(pos);
				TInt firstfree=pB->Alloc(askfor, consec);
				if (firstfree!=init)
					test(0);
				TInt number=(pos-init>askfor)?askfor:pos-init;
				if (consec!=number)
					test(0);
				if (number<pos-init)
					{
					firstfree=pB->Alloc(pos-init-number,consec);
					if(firstfree!=init+number)
						test(0);
					if(consec!=pos-init-number)
						test(0);
					}
				test(pB->Avail()==available-pos-1);
				TInt freeto=available;
				if (pos<available-1)
					{
					firstfree=pB->Alloc(askfor,consec);
					number=(available-pos-1>askfor)?askfor:available-pos-1;
					if (firstfree!=pos+1)
						test(0);
					if (consec!=number)
						test(0);
					freeto=pos+1+number;
					}
				test(pB->Avail()==available-freeto);
				if (available==freeto)
					{
					firstfree=pB->Alloc(1,consec);
					if (firstfree!=KErrNoMemory)
						test(0);
					if (consec!=0)
						test(0);
					}
				pB->Free(init,freeto-init);
				}
			if (init)
				pB->Free(0,init);
			test(pB->Avail()==available);
			}
		}
	test.End();
	test.Start(_L("AllocAligned"));
	TInt alignment, alignstep;
	for(alignment=0, alignstep=1; alignstep<available; alignment++, alignstep<<=1 )
		{
		TInt numaligned=(available+alignstep-1)/alignstep;
		TInt next=0;
		TInt r;
		do	{
			r=pB->AllocAligned(alignment);
			if (r>=0)
				{
				if (r!=next)
					test(0);
				next+=alignstep;
				}
			else if (r!=KErrNoMemory)
				test(0);
			} while(r>=0);
		if (pB->Avail()!=available-numaligned)
			test(0);
		for(TInt i=0; i<available; i++)
			{
			if (i==((i>>alignment)<<alignment) )
				{
				if (pB->IsFree(i))
					test(0);
				pB->Free(i);
				}
			else
				{
				if (!pB->IsFree(i))
					test(0);
				}
			}
		test(pB->Avail()==available);
		}
	test.End();
	test.Start(_L("AllocAlignedBlock"));
	for(alignment=0, alignstep=1; alignstep<available; alignment++, alignstep<<=1 )
		{
		TInt numalignedblocks=available/alignstep;
		TInt next=0;
		TInt r;
		do	{
			r=pB->AllocAlignedBlock(alignment);
			if (r>=0)
				{
				if (r!=next)
					test(0);
				next+=alignstep;
				}
			else if (r!=KErrNoMemory)
				test(0);
			} while(r>=0);
		if (pB->Avail()!=available-numalignedblocks*alignstep)
			test(0);
		if (pB->Avail()!=0)
			{
			if ( !pB->IsFree(numalignedblocks*alignstep,pB->Avail()) )
				test(0);
			r=pB->Alloc();
			if (r!=numalignedblocks*alignstep)
				test(0);
			pB->Free(r);
			}
		pB->Free(0,numalignedblocks*alignstep);
		if (pB->Avail()!=available)
			test(0);
		TInt freepos, blockpos, c;
		for (freepos=0; freepos<available; freepos+=alignstep)
			{
			for (blockpos=0; blockpos<alignstep; blockpos++)
				{
				c=0;
				for(TInt i=blockpos; i<freepos; i+=alignstep)
					{
					pB->AllocAt(i);
					c++;
					}
				if (pB->Avail()!=available-c)
					test(0);
				r=pB->AllocAlignedBlock(alignment);
				if (available-freepos<alignstep)
					{
					if (r!=KErrNoMemory)
						test(0);
					if (pB->Avail()!=available-c)
						test(0);
					}
				else
					{
					if (r!=freepos)
						test(0);
					if (pB->Avail()!=available-c-alignstep)
						test(0);
					pB->Free(freepos,alignstep);
					if (pB->Avail()!=available-c)
						test(0);
					}
				for(TInt j=blockpos; j<freepos; j+=alignstep)
					pB->Free(j);
				if (pB->Avail()!=available)
					test(0);
				}
			}
		}
	delete pB;
	test.End();
	}

LOCAL_C void testContiguousAllocation(TInt aSize)
	{//test RemoveRamPages()
	//set up bitmap with pattern of increasingly large gaps -
	//page 1      - in use,page  2        - free
	//pages 3,4   - in use,pages 5,6      - free
	//pages 7,8,9 - in use,pages 10,11,12 - free  ...etc
	test.Start(_L("Create swiss cheese effect..."));

 	CBitMapAllocator* pB=CBitMapAllocator::New(aSize);
	test(pB!=NULL);

	TInt available=pB->Avail();
	test(available==aSize);

	TInt i=0;
	TInt j=0;
	TInt k=1;
	while(k<46)
		{
		for(j=0;j<k;j++)
			{
			pB->AllocAt(i+j);
			test(!pB->IsFree(i+j));
			}
		i+=2*k;
		k++;
		}

	TInt ret=KErrNone;
	TInt pageNo=0;
	for(i=1;i<45;i++)
		{
		ret=pB->ExtractRamPages(i,pageNo);	//look for a gap of size i pages and allocate it
		test(pageNo==i*i);				//test the right page no is returned
		test.Printf(_L("OK  -pageNo is :%d\r\n"),pageNo);
		for(j=i*i;j<i*i + i;j++)		//test that the pages are actually allocated
			test(!pB->IsFree(j));
		}

	ret=pB->ExtractRamPages(45,pageNo);//there's not a big enough space in the bitmap for this to succeed
	test(ret==KErrNoMemory);
	delete pB;
	test.End();
	}

LOCAL_C void testAll(TInt aSize)
//
//	Test all BMA functions using a BMA of size aSize
//
	{

	TBuf<0x40> b;
	b.Format(_L("BitMapAllocator size = %d"),aSize);
	test.Start(b);
//
	testNew(aSize);
	testNewL(aSize);
	testAlloc(aSize);
	testBlock(aSize);
//
	test.End();
	}

GLDEF_C TInt E32Main()
//
// Test bitmap allocator
//
	{
	test.Title();
	__UHEAP_MARK;
//
	test.Start(_L("1 bit"));
	testAll(1);

	test.Next(_L("4 bit"));
	testAll(4);
//
	test.Next(_L("32 bit"));
	testAll(32);
//
	test.Next(_L("33 bit"));
	testAll(33);
//
	test.Next(_L("68 bit"));
	testAll(68);
//
	test.Next(_L("96 bit"));
	testAll(96);
//
	test.Next(_L("64 bit"));
	testAll(64);
//
	test.Next(_L("65 bit"));
	testAll(65);
//
	test.Next(_L("63 bit"));
	testAll(63);

	testContiguousAllocation(2048);

	test.Next(_L("test defect EXT-5AMDKP"));

	CBitMapAllocator* pB = CBitMapAllocator::New(64);
	test(pB != NULL);
	pB->AllocAt(0, 32);
	pB->Free(2, 2);
	pB->Free(5, 2);
	pB->Free(8, 2);
	pB->Free(12, 3);
	pB->Free(30, 2);
	TInt page = -1;
	pB->ExtractRamPages(3, page);
	test(page == 12);
	pB->ExtractRamPages(4, page);
	test(page == 30);
	pB->ExtractRamPages(1, page);
	test(page == 2);
	pB->ExtractRamPages(5, page);
	test(page == 34);
	pB->ExtractRamPages(2, page);
	test(page == 5);
	delete pB;	
		
	__UHEAP_MARKEND;
	test.End();
	return(KErrNone);
	}

