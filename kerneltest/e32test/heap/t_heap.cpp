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
// e32test\heap\t_heap.cpp
// Overview:
// Tests RHeap class.
// API Information:
// RHeap
// Details:
// - Test that the expected methods are in the DLL by calling each one.
// - Test heap auto expansion and compression by calling Alloc and Compress
// and verifying the results are as expected.
// - Verify the heap dump Base, Size, MinLength, Top and len values.
// - Test the RHeap AllocSize, Alloc, AllocLen, Count and Free methods. Verify
// results are as expected. Check heap object and confirm Invariant status.
// - For an RHeap object, test and verify the results of: allocate some cells, 
// free them with Reset, allocate some cells again, free them with Free, 
// allocate some cells again, free them backwards, allocate again, free the 
// odd cells then the even cells, allocate again, free one half then the other.
// Check heap object and confirm Invariant status.
// - For an RHeap object, test and verify the results of: attempt to resize a
// block above the space available, resize the block to 0, resize positively, 
// allocate a block, fill with data, allocate another block or two then resize
// the original block such that it has to be moved in memory, then check the 
// blocks' contents, test data was copied on reallocation, resize blocks and
// verify data integrity, expand and shrink, verify data.
// Check heap object and confirm Invariant status.
// - For an RHeap object, test and verify the results of: Alloc some cells,
// verify the Count, Check the object, Free some cells, verify the Count, 
// Check and Reset the object, corrupt the heap data and reset the object.
// - Test the leaving methods: AllocL and ReAllocL. Verify the results are as
// expected.
// - Test the RHeap methods: Alloc, Count, Size, Free and Close. Verify results
// are as expected.
// - Test sharing a chunk heap between two separate threads.  Each thread
// accesses the shared heap in a timed loop, to ensure that some true
// concurrency.
// - Test sharing a chunk heap between two separate threads. Run each thread in
// a timed loop, to ensure that some true concurrency. Each thread accesses
// the shared heap and results are verified.  The heap size is used to verify
// no leaks and that the largest available space is still available. The heap
// is checked to verify that no cells remain allocated after the tests are
// complete.
// - Test sharing a heap between two threads.  The thread whose heap it was is
// killed first.  Each thread accesses the shared heap and results are
// verified.
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

// Sets data for Test6
#define SetData(size)	pHeap->Reset();\
						Cell1=pHeap->Alloc(size);\
						Cell2=pHeap->Alloc(size);\
						Cell3=pHeap->Alloc(size);\
						for(pC=(TText8*)Cell1; pC<(TText8*)Cell1+pHeap->AllocLen(Cell1); *pC++='x');\
						for(pC=(TText8*)Cell2; pC<(TText8*)Cell2+pHeap->AllocLen(Cell2); *pC++='y');\
						for(pC=(TText8*)Cell3; pC<(TText8*)Cell3+pHeap->AllocLen(Cell3); *pC++='z');\
						OrigLen=pHeap->AllocLen(Cell2);			

// Tests cell contents for Test6
#define TestCells(Cell2Len)	for(pC=(TText8*)Cell1; pC<(TText8*)Cell1+pHeap->AllocLen(Cell1); test(*pC++=='x'));\
							for(pC=(TText8*)Cell2; pC<(TText8*)Cell2+Cell2Len; test(*pC++=='y'));\
							for(pC=(TText8*)Cell3; pC<(TText8*)Cell3+pHeap->AllocLen(Cell3); test(*pC++=='z'));\
							pHeap->Check();

#ifdef __EABI__
	IMPORT_D extern const TInt KHeapMinCellSize;
#else
    const TInt KHeapMinCellSize = 0;
#endif

const TInt KHeadSize = (TInt)RHeap::EAllocCellSize;

const TInt KAlign = RHeap::ECellAlignment;
const TInt KMinCellLength = _ALIGN_UP((KHeapMinCellSize + Max(TInt(RHeap::EFreeCellSize),TInt(RHeap::EAllocCellSize))),KAlign) - RHeap::EAllocCellSize;
const TInt KMinFreeSize = _ALIGN_UP((KHeapMinCellSize + Max(TInt(RHeap::EFreeCellSize),TInt(RHeap::EAllocCellSize))),KAlign);

TInt PageSize;

class RTestHeap : public RHeap
	{
public:
	void __DbgTest(void* pRHeapDump) const;
	};

struct RHeapDump
	{
	TUint				iMinLength;
	RChunk				iChunk;
	TUint8				*iBase;
	TUint8				*iTop;
	//RHeap::SCell		iFree;
	};

#pragma warning ( disable :4705 ) // statement has no effect
RHeapDump OrigDump;
#pragma warning ( default :4705 )

#if defined(_DEBUG)
void RTestHeap::__DbgTest(void* aPtr) const
	{
	(void) aPtr;
/*	
	RHeapDump& d = *(RHeapDump*)aPtr;
	d.iMinLength=iMinLength;
	d.iChunk.SetHandle(iChunkHandle);
	d.iBase=iBase;
	d.iTop=iTop;
	d.iFree=iFree;
*/	
	}
#endif


#if defined(_DEBUG)
TBool Invariant(RHeap* aHeap)
	{
	(void) aHeap;
/*	
	RHeapDump dump;
	((RTestHeap*)aHeap)->__DbgTest(&dump);
	if(dump.iMinLength!=OrigDump.iMinLength) 		return(EFalse);
	// Note: iChunk is a class
	if(dump.iBase!=OrigDump.iBase)			return(EFalse);
	if(*dump.iBase!=*OrigDump.iBase)		return(EFalse);
	if(dump.iTop!=OrigDump.iTop)			return(EFalse);
	if(dump.iTop[-1]!=OrigDump.iTop[-1])	return(EFalse);	
	if(dump.iFree.len!=OrigDump.iFree.len)	return(EFalse);
	// iFree.Next changes during allocation/freeing etc.
*/
	return(ETrue);
	}
#define INV(x) x;
#else
#define INV(x)
#endif

LOCAL_D RTest test(_L("T_HEAP"));
LOCAL_D TInt heapCount=1;
LOCAL_D RHeap *gHeapPtr;
LOCAL_D RHeap *gHeapPtr2;

/*
Friend class of RHeapHybrid to access to hybrid heap metadata
*/
class TestHybridHeap
{
public:
    static TBool IsHybrid(const RHybridHeap * aHybridHeap);
};

TBool TestHybridHeap::IsHybrid(const RHybridHeap * aHybridHeap)
  {
  if (aHybridHeap->iDLOnly)
      return EFalse;
  else
      return ETrue;
  }

class TestRHeap
	{
public:
	void Test1(void);
	void Test2(void);
	void Test3(void);
	void Test4(void);
	void Test5(void);
	void Test7(void);
	void Test8(void);
	void TestCompressAll(void);
	void TestOffset(void);
private:
	TInt RHeapCalcReduce(TInt aCellSize, TInt aGrowBy);
	};

LOCAL_C RHeap* allocHeap(TInt aSize)
//
// Allocate a chunk heap with max size aSize
//
	{

	TName n;
	n.Format(_L("TESTHEAP%d"),heapCount++);
	return(User::ChunkHeap(&n,aSize,aSize));
	}

////////////////////////////////////////////////////////////////////////////////////////
// Test that methods are in the DLL
////////////////////////////////////////////////////////////////////////////////////////
void TestRHeap::Test1(void)
	{ 
	TAny* aCell;
	TInt aVar;
	RHeap* pHeap=allocHeap(3000); // tests first constructor indirectly 
	// constructor with Chunk not tested
	pHeap->Base();
	pHeap->Size();
	pHeap->Available(aVar);
	pHeap->Check();
	pHeap->Count();												 	
	pHeap->Count(aVar);
	aCell=pHeap->Alloc(50);
	pHeap->Free(aCell);
	aCell=pHeap->AllocL(50);
	pHeap->AllocLen(aCell);
	pHeap->ReAlloc(aCell, 100);
	pHeap->ReAllocL(aCell, 150);
	pHeap->Reset();
	pHeap->Close();
	}

///////////////////////////////////////////////////////////////////////////////
// Test Assorted Methods 1
//////////////////////////////////////////////////////////////////////////////
void TestRHeap::Test2(void)
	{
#if defined(_DEBUG)
	RHeapDump dump;
	RHeap* pHeap=allocHeap(3000);

	((RTestHeap*)pHeap)->__DbgTest(&OrigDump);
	((RTestHeap*)pHeap)->__DbgTest(&dump);
	
//	test(dump.iBase==pHeap->Base());
//	test((dump.iTop-dump.iBase)==pHeap->Size());
	pHeap->Check();
	test(Invariant(pHeap));
	pHeap->Close();
#endif
	}

///////////////////////////////////////////////////////////////////////////////
// Test Assorted Methods 2
//////////////////////////////////////////////////////////////////////////////		 
void TestRHeap::Test3(void)
	{  
	TInt CellLen;
	TInt OrigBiggestBlock, BiggestBlock;
	TAny* aCell;
	TInt FreeCount, AllocCount, AllocSize;
	RHeap* pHeap=allocHeap(5000);

#if defined(_DEBUG)
	((RTestHeap*)pHeap)->__DbgTest(&OrigDump);
#endif

	// test AllocSize
	AllocCount=pHeap->Count(FreeCount);
	test(pHeap->AllocSize(AllocSize)==pHeap->Count());
	test(AllocSize==0);
	test(AllocCount==pHeap->Count());
	test(AllocCount==0);
	test(FreeCount==1);

	TAny* p1=pHeap->Alloc(1);
	test(pHeap->AllocSize(AllocSize)==1);
	test(AllocSize==pHeap->AllocLen(p1));
	 
	TAny* p2=pHeap->Alloc(8);
	test(pHeap->AllocSize(AllocSize)==2);
	test(AllocSize==pHeap->AllocLen(p1)+pHeap->AllocLen(p2));

	TAny* p3=pHeap->Alloc(127);
	test(pHeap->AllocSize(AllocSize)==3);
	test(AllocSize==pHeap->AllocLen(p1)+pHeap->AllocLen(p2)+pHeap->AllocLen(p3));

	pHeap->Free(p2);
	test(pHeap->AllocSize(AllocSize)==2);
	test(AllocSize==pHeap->AllocLen(p1)+pHeap->AllocLen(p3));

	pHeap->Free(p1);
	test(pHeap->AllocSize(AllocSize)==1);
	test(AllocSize==pHeap->AllocLen(p3));

	pHeap->Free(p3);
	test(pHeap->AllocSize(AllocSize)==0);
	test(AllocSize==0);

	pHeap->Available(OrigBiggestBlock);

	// Request too large a block 
	test((aCell=pHeap->Alloc(OrigBiggestBlock+1))==NULL);
	AllocCount=pHeap->Count(FreeCount);
	test(AllocCount==0);
	test(FreeCount==1);


	// Request block same size as that available
	test((aCell=pHeap->Alloc(OrigBiggestBlock))!=NULL);
	test(pHeap->Available(BiggestBlock)==0);  
	test(BiggestBlock==0);
	test(pHeap->AllocLen(aCell)==OrigBiggestBlock);
	AllocCount=pHeap->Count(FreeCount);
	test(AllocCount==pHeap->Count());
	test(AllocCount==1);
	test(FreeCount==0);
	pHeap->Check();
	// Free the block
	pHeap->FreeZ(aCell);
	test(aCell==NULL);
	pHeap->Available(BiggestBlock);
	test(BiggestBlock==OrigBiggestBlock);
	AllocCount=pHeap->Count(FreeCount);
	test(AllocCount==0);
	test(FreeCount==1);


	// Request a block much smaller than that available
	test((aCell=pHeap->Alloc(1))!=NULL);
	CellLen=pHeap->AllocLen(aCell);
	pHeap->Available(BiggestBlock);
	test(pHeap->Available(BiggestBlock)==BiggestBlock);
	test((BiggestBlock+CellLen+KHeadSize)==OrigBiggestBlock);
	// NOTE: if a block of 1000 was initially available, getting a cell of length 100 DOES NOT
	// leave 900 available as some of the 1000(KHeadSize) is used up storing the length of the
	// allocated block
	AllocCount=pHeap->Count(FreeCount);
	test(AllocCount==1);
	test(FreeCount==1);
	pHeap->Check();
	// Free the block
	pHeap->Free(aCell);
	test(aCell!=NULL);
	pHeap->Available(BiggestBlock);
	test(BiggestBlock==OrigBiggestBlock);
	AllocCount=pHeap->Count(FreeCount);
	test(AllocCount==0);
	test(FreeCount==1);


	// Request a block only just smaller than that available
	test((aCell=pHeap->Alloc(OrigBiggestBlock-1))!=NULL);
	CellLen=pHeap->AllocLen(aCell);
	AllocCount=pHeap->Count(FreeCount);
	test(AllocCount==1);
	test(FreeCount==0);
	pHeap->Check();
	// Free the block
	pHeap->Free(aCell);
	pHeap->Available(BiggestBlock);
	test(BiggestBlock==OrigBiggestBlock);
	AllocCount=pHeap->Count(FreeCount);
	test(AllocCount==0);
	test(FreeCount==1);


	//Request a block of 0 size	   Note: 0 may not necessarily be allocated (probably will be 4)
	test((aCell=pHeap->Alloc(0))!=NULL);
	pHeap->Available(BiggestBlock);
	AllocCount=pHeap->Count(FreeCount);
	test(AllocCount==1);
	test(FreeCount==1);
	pHeap->Check();
	//Free the block
	pHeap->Free(aCell);
	pHeap->Available(BiggestBlock);
	test(BiggestBlock==OrigBiggestBlock);	 
	AllocCount=pHeap->Count(FreeCount);
	test(AllocCount==0);
	test(FreeCount==1);
	pHeap->Check();
	INV(test(Invariant(pHeap)));

	// close heap so we don't exceed chunk limit
	pHeap->Close();
	}

///////////////////////////////////////////////////////////////////////////////
// Test Assorted Methods 3 - Here we go loopy loo, here we go loopy li
//////////////////////////////////////////////////////////////////////////////				  
void TestRHeap::Test4(void)
	{ 
	TInt OrigBiggestBlock, BiggestBlock, FreeCount, AllocCount;
	RHeap* pHeap=allocHeap(5000);

	pHeap->Available(OrigBiggestBlock);
#if defined(_DEBUG)
	((RTestHeap*)pHeap)->__DbgTest(&OrigDump);
#endif
	
	for(TInt ArraySize=1; ArraySize<=100; ArraySize++)
		{	  
		TAny** ArrayOfCells;
		ArrayOfCells= new TAny*[ArraySize];
		TInt ArrayIndex;
	
		// Allocate some cells
		for(ArrayIndex=0; ArrayIndex<ArraySize;ArrayIndex++)
			ArrayOfCells[ArrayIndex]=pHeap->Alloc(OrigBiggestBlock/(ArraySize*3)); 
		pHeap->Available(BiggestBlock);
		test(BiggestBlock!=OrigBiggestBlock);
		AllocCount=pHeap->Count(FreeCount);
		test((TInt)AllocCount==ArraySize);
		test(FreeCount==1);
		pHeap->Check();
		// Now free them with Reset
		pHeap->Reset();
		pHeap->Available(BiggestBlock);
		test(BiggestBlock==OrigBiggestBlock);
		AllocCount=pHeap->Count(FreeCount);
		test(AllocCount==0);
		test(FreeCount==1);
	
	
		// Allocate some cells again	 
		for(ArrayIndex=0; ArrayIndex<ArraySize;ArrayIndex++)
			ArrayOfCells[ArrayIndex]=pHeap->Alloc(OrigBiggestBlock/(ArraySize*3)); 
		pHeap->Available(BiggestBlock);
		test(BiggestBlock!=OrigBiggestBlock);
		AllocCount=pHeap->Count(FreeCount);
		test((TInt)AllocCount==ArraySize);
		test(FreeCount==1);
		pHeap->Check();
		// Free them with Free
		for(ArrayIndex=0; ArrayIndex<ArraySize;ArrayIndex++)
			pHeap->Free(ArrayOfCells[ArrayIndex]);
		pHeap->Available(BiggestBlock);
		test(BiggestBlock==OrigBiggestBlock);
		AllocCount=pHeap->Count(FreeCount);
		test(AllocCount==0);
		test(FreeCount==1);
	
	
		// Allocate some cells again	 
		for(ArrayIndex=0; ArrayIndex<ArraySize;ArrayIndex++)
			ArrayOfCells[ArrayIndex]=pHeap->Alloc(OrigBiggestBlock/(ArraySize*3));
		pHeap->Available(BiggestBlock);
		test(BiggestBlock!=OrigBiggestBlock); 
		AllocCount=pHeap->Count(FreeCount);
		test((TInt)AllocCount==ArraySize);
		test(FreeCount==1);
		pHeap->Check();
		// Free them backwards
		for(ArrayIndex=ArraySize-1; ArrayIndex>=0; ArrayIndex--)
			pHeap->Free(ArrayOfCells[ArrayIndex]);
		pHeap->Available(BiggestBlock);
		test(BiggestBlock==OrigBiggestBlock);
		AllocCount=pHeap->Count(FreeCount);
		test(AllocCount==0);
		test(FreeCount==1);


		// Allocate some cells again	 
		for(ArrayIndex=0; ArrayIndex<ArraySize;ArrayIndex++)
			ArrayOfCells[ArrayIndex]=pHeap->Alloc(OrigBiggestBlock/(ArraySize*3));
		pHeap->Available(BiggestBlock);
		test(BiggestBlock!=OrigBiggestBlock);
		AllocCount=pHeap->Count(FreeCount);
		test((TInt)AllocCount==ArraySize);
		test(FreeCount==1);
		pHeap->Check();
		// Free the odd cells then the even cells
		for(ArrayIndex=0; ArrayIndex<ArraySize; ArrayIndex+=2)
			pHeap->Free(ArrayOfCells[ArrayIndex]);
		pHeap->Check();
		for(ArrayIndex=1; ArrayIndex<ArraySize; ArrayIndex+=2)
			pHeap->Free(ArrayOfCells[ArrayIndex]);
		pHeap->Check();
		pHeap->Available(BiggestBlock);
		test(BiggestBlock==OrigBiggestBlock);
		AllocCount=pHeap->Count(FreeCount);
		test(AllocCount==0);
		test(FreeCount==1);
	
	
		// Allocate some cells again	 
		for(ArrayIndex=0; ArrayIndex<ArraySize;ArrayIndex++)
			ArrayOfCells[ArrayIndex]=pHeap->Alloc(OrigBiggestBlock/(ArraySize*3));
		pHeap->Available(BiggestBlock);
		test(BiggestBlock!=OrigBiggestBlock);
		AllocCount=pHeap->Count(FreeCount);
		test((TInt)AllocCount==ArraySize);
		test(FreeCount==1);
		pHeap->Check();
		// Free one half then the other
		for(ArrayIndex=ArraySize-1; ArrayIndex>=ArraySize/2; ArrayIndex--)
			pHeap->Free(ArrayOfCells[ArrayIndex]);
		for(ArrayIndex=0; ArrayIndex<ArraySize/2; ArrayIndex++)
			pHeap->Free(ArrayOfCells[ArrayIndex]);
		AllocCount=pHeap->Count(FreeCount);
		test(AllocCount==0);
		test(FreeCount==1);
	
		delete [] ArrayOfCells;
		pHeap->Check();
		INV(test(Invariant(pHeap)))
		}

	// close heap so we don't exceed chunk limit
	pHeap->Close();
	}

	
///////////////////////////////////////////////////////////////////////////////
// Test ReAlloc
//////////////////////////////////////////////////////////////////////////////
void TestRHeap::Test5(void)
	{
	TInt BiggestBlock, CellSize;

	RHeap* pHeap=allocHeap(5000);
#if defined(_DEBUG)
	((RTestHeap*)pHeap)->__DbgTest(&OrigDump);
#endif
	pHeap->Available(BiggestBlock);
	TAny* aCell=pHeap->Alloc(BiggestBlock);

	// Attempt to resize the block above the space available
	test(pHeap->ReAlloc(aCell, BiggestBlock*2)==NULL);

	// Resize the block to 0
	aCell=pHeap->ReAlloc(aCell, 0);
	CellSize=pHeap->AllocLen(aCell);  // test?

	// Resize positively
	for(TInt aSize=0; aSize<=BiggestBlock; aSize++, pHeap->Available(BiggestBlock))
		{
		aCell = pHeap->ReAlloc(aCell, aSize); 
		test(aCell!=NULL); 
		CellSize=pHeap->AllocLen(aCell);
		test(CellSize>=aSize);
		}

	pHeap->Check();
	pHeap->Reset();
	// Allocate a block, fill with data, allocate another block or two then resize the original
	// block such that it has to be moved in memory, then check the blocks' contents 
	TAny* Cell1=pHeap->Alloc(16);
	TText8* pC;
	TInt Cell1Size=pHeap->AllocLen(Cell1);
	for(pC=(TText8*)Cell1; pC<(TText8*)Cell1+Cell1Size; *pC++='x')
		;
	TAny* Cell2=pHeap->Alloc(16);
	TInt Cell2Size=pHeap->AllocLen(Cell2);
	for(pC=(TText8*)Cell2; pC<(TText8*)Cell2+pHeap->AllocLen(Cell2); *pC++='y')
		;
	Cell1=pHeap->ReAlloc(Cell1, 128);
	// Test data was copied on reallocation
	for(pC=(TText8*)Cell1; pC<(TText8*)Cell1+Cell1Size; test(*pC++=='x'))
		; 
	// Test other data wasn't corrupted
	for(pC=(TText8*)Cell2; pC<(TText8*)Cell2+pHeap->AllocLen(Cell2); test(*pC++=='y'))
		;

	// Allocate another block
	TAny* Cell3=pHeap->Alloc(8);
	for(pC=(TText8*)Cell3; pC<(TText8*)Cell3+pHeap->AllocLen(Cell3); *pC++='z')
		;
	// test existing blocks to be safe
	for(pC=(TText8*)Cell1; pC<(TText8*)Cell1+Cell1Size; test(*pC++=='x'))
		;
	for(pC=(TText8*)Cell2; pC<(TText8*)Cell2+Cell2Size; test(*pC++=='y'))
		;
	// Resize previous blocks
	Cell1=pHeap->ReAlloc(Cell1, 16); // Shrink previously expanded block
	Cell2=pHeap->ReAlloc(Cell2, 64); 
	// Now test data
	for(pC=(TText8*)Cell1; pC<(TText8*)Cell1+Cell1Size; test(*pC++=='x'))
		;
	for(pC=(TText8*)Cell2; pC<(TText8*)Cell2+Cell2Size; test(*pC++=='y'))
		;
	for(pC=(TText8*)Cell3; pC<(TText8*)Cell3+pHeap->AllocLen(Cell3); test(*pC++=='z'))
		;

	// Re-expand Cell1
	Cell1=pHeap->ReAlloc(Cell1, 1028);
	for(pC=(TText8*)Cell1; pC<(TText8*)Cell1+Cell1Size; test(*pC++=='x'))
		;
	for(pC=(TText8*)Cell2; pC<(TText8*)Cell2+Cell2Size; test(*pC++=='y'))
		;
	for(pC=(TText8*)Cell3; pC<(TText8*)Cell3+pHeap->AllocLen(Cell3); test(*pC++=='z'))
		;

	// Shrink cells back to original size
	Cell1=pHeap->ReAlloc(Cell1, Cell1Size);
	Cell2=pHeap->ReAlloc(Cell2, Cell2Size);  
	for(pC=(TText8*)Cell1; pC<(TText8*)Cell1+Cell1Size; test(*pC++=='x'))
		;
	for(pC=(TText8*)Cell2; pC<(TText8*)Cell2+Cell2Size; test(*pC++=='y'))
		;
	for(pC=(TText8*)Cell3; pC<(TText8*)Cell3+pHeap->AllocLen(Cell3); test(*pC++=='z'))
		;

	pHeap->Check();
	INV(test(Invariant(pHeap)));

	// close heap so we don't exceed chunk limit
	pHeap->Close();
	}


///////////////////////////////////////////////////////////////////////////////
// Test walking methods (more thoroughly than previously)
//////////////////////////////////////////////////////////////////////////////				  
void TestRHeap::Test7(void)
	{ 
	TInt NumAllocated=0, NumFree=1, i;
	RHeap* pHeap=allocHeap(5000);

	TAny** ArrayOfCells;
	ArrayOfCells= new TAny*[100];

	for(i=0; i<100; i++)
		{
		ArrayOfCells[i]=pHeap->Alloc(8);
		NumAllocated++;
		test(NumAllocated==pHeap->Count(NumFree));
		test(NumFree==1);
		}
	pHeap->Check();

	for(i=0; i<100; i+=2)
		{
		TInt temp;
		pHeap->Free(ArrayOfCells[i]);
		NumAllocated--;
		NumFree++;
		test(NumAllocated==pHeap->Count(temp));
		test(NumFree==temp);
		}
	pHeap->Check();
	pHeap->Reset();


	///////////////////////////////////////////
	// Corrupt data and see what happens
	///////////////////////////////////////////
	// Corrupt  allocated cell header
	ArrayOfCells[0]=pHeap->Alloc(32);
	TUint32* pC=(TUint32*)ArrayOfCells[0]-KHeadSize; 
	*pC=0xa5a5a5a5u; 
	// pHeap->Check();

	// Corrupt free cell header 
	pHeap->Reset();
	ArrayOfCells[0]=pHeap->Alloc(32);
	pC=(TUint32*)ArrayOfCells[0]+(pHeap->AllocLen(ArrayOfCells[0])>>2);
	*pC=0xa1a1a1a1u;	
	//pHeap->Check();	 // Check doesn't pick it up but an access violation is generated

	// Write past end of heap
	pHeap->Reset();
	TInt Avail;
	ArrayOfCells[0]=pHeap->Alloc(pHeap->Available(Avail));
	pC=(TUint32*)ArrayOfCells[0]+(pHeap->AllocLen(ArrayOfCells[0])>>2);
	//*pC=0xa1a1a1a1u;	// This line isn't picked up by Check (wouldn't expect it to) but the call
	//pHeap->Check();	// to delete below consequently crashes 

	delete [] ArrayOfCells;

	// close heap so we don't exceed chunk limit
	pHeap->Close();
	}

//////////////////////////////////////
// Test the leave methods
//////////////////////////////////////
void TestRHeap::Test8(void)
	{ 

	TAny* aCell=NULL;
	RHeap* pHeap=allocHeap(1000); 
	TRAPD(ret,aCell=pHeap->AllocL(100))
	test(ret==KErrNone);
	TRAP(ret,aCell=pHeap->AllocL(PageSize))
   	test(ret==KErrNoMemory);
	TRAP(ret,aCell=pHeap->ReAllocL(aCell,32))
	test(ret==KErrNone);
	TRAP(ret,aCell=pHeap->ReAllocL(NULL,10000))
	test(ret==KErrNoMemory);

	// close heap so we don't exceed chunk limit
	pHeap->Close();
	}

class RMyHeap : public RHeap
	{
public:
	void MyCompressAll(){}
private:
	RMyHeap();
	};

#include "TestRHeapShrink.h"

/**
	Calculates whether or not the heap with iGrowBy=aGrowBy will be reduced if a 
	cell of size aCellSize bytes is the top free cell.
	It must be calculated as both the page size and min cell size could vary
	between different platforms/builds.  Also, KHeapMinCellSize is 'patchdata' and can be
	different for particular ROM builds
	ASSUMPTIONS:-
		1 - The cell of aCellSize starts past the RHeap's iMinLength (i.e. all of it can be 
		removed without the RHeap becoming smaller than iMinLength
		2 - The default value of aAlign was passed to RHeap contructor
	These should be safe as this is onl used by t_heap TestRHeap::CompressAll()
	@return The number of bytes the heap will be reduced by
*/
TInt TestRHeap::RHeapCalcReduce(TInt aCellSize, TInt aGrowBy)
	{
	TInt ret = 0;
	TInt pageSize = 0;
	test(UserHal::PageSizeInBytes(pageSize)==KErrNone);
	
	// adjust aGrowBy to match what RHeap would have aligned its iGrowBy to 
	// see RHeap::RHeap()
	aGrowBy = _ALIGN_UP(aGrowBy, pageSize);
	if (aCellSize >= KHeapShrinkHysRatio*(aGrowBy>>8))
		{
		//calc for amount to reduce heap from RHeap::Reduce()
		// assumes that cell of aCellSize starts past the RHeap's iMinLength
		ret=_ALIGN_DOWN(aCellSize, pageSize);
		}
	return ret;
	}

void TestRHeap::TestCompressAll()
	{

	TPtrC myHeapName=_L("MyHeap");
	// myHeap will have default GrowBy of KMinHeapGrowBy
	RMyHeap* myHeap=(RMyHeap*)User::ChunkHeap(&myHeapName,0x100,0x2000);
const TInt KnormHeapGrowBy = 0x2000;
	RHeap* normHeap=User::ChunkHeap(NULL,0x100,0x20000,KnormHeapGrowBy);
	//
	// Configure paged heap threshold 128 Kb (pagepower 17)
	//
	RHybridHeap::STestCommand conf;
	conf.iCommand = RHybridHeap::EGetConfig;
	if ( normHeap->DebugFunction(RHeap::EHybridHeap, (TAny*)&conf ) == KErrNone )
		{
		test.Printf(_L("New allocator detected, configuring paged threshold to 128 kb\r\n"));
		conf.iCommand = RHybridHeap::ESetConfig;
		conf.iConfig.iPagePower = 17;
		test( normHeap->DebugFunction(RHeap::EHybridHeap, (TAny*)&conf ) == KErrNone);
		}

	TAny* ptrMy1=myHeap->Alloc(0x102);
	test(ptrMy1!=NULL);
	TAny* ptrMy2=myHeap->Alloc(0x1001);
	test(ptrMy2!=NULL);
	TInt r=myHeap->Count();
	test(r==2);

	TAny* ptrNorm1=normHeap->Alloc(0x8002);
	test(ptrNorm1!=NULL);
	TAny* ptrNorm2=normHeap->Alloc(0x12fff);
	test(ptrNorm2!=NULL);
	TAny* ptrNorm3=normHeap->Alloc(0x334f);
	test(ptrNorm3!=NULL);
	r=normHeap->Count();
	test(r==3);

	TInt oldMyHeapSize=myHeap->Size(); 	
	TInt oldNormHeapSize=normHeap->Size();
	 	
	myHeap->MyCompressAll();

	r=myHeap->Count();
	test(r==2);
	r=myHeap->Size();
	test(r==oldMyHeapSize);
	r=normHeap->Count();
	test(r==3);
	r=normHeap->Size();
	test(r==oldNormHeapSize);

	// Remove the cell on the top of the normHeap
	normHeap->Free(ptrNorm3);
	// check myHeap unaffected
	r=myHeap->Count();
	test(r==2);
	r=myHeap->Size();
	test(r==oldMyHeapSize);
	//check normHeap updated after free of top cell
	r=normHeap->Count();
	test(r==2);
	r=normHeap->Size();

	// Calc the amount, if any, the overall size of normHeap will have been shrunk by
	// will depend on value of KHeapShrinkHysRatio.
	// 1st calc current total size of the allocated cells
	TInt normAllocdSize = 	normHeap->AllocLen(ptrNorm1)+KHeadSize +
							normHeap->AllocLen(ptrNorm2)+KHeadSize;
	TInt normReduce = RHeapCalcReduce(oldNormHeapSize-normAllocdSize,KnormHeapGrowBy);
	oldNormHeapSize -= normReduce;
	test(r==oldNormHeapSize);

	normHeap->Free(ptrNorm2);
	myHeap->Free(ptrMy2);
	r=myHeap->Count();
	test(r==1);
	r=myHeap->Size();

	// Calc the current total size of the allocated cells
	TInt myAllocdSize = myHeap->AllocLen(ptrMy1)+KHeadSize;
	TInt myReduce=RHeapCalcReduce(oldMyHeapSize-myAllocdSize,1);
	oldMyHeapSize -= myReduce;
	test(r==oldMyHeapSize);

	r=normHeap->Count();
	test(r==1);
	r=normHeap->Size();

	// cell represented by ptrNorm3 may have already caused the heap
	// size to be reduced so ensure normReduce is factored into calcs
	test(r==oldNormHeapSize-(0x16000-normReduce));

	myHeap->Close();
	normHeap->Close();
	}


void TestRHeap::TestOffset()
	{
	TInt size = 0x100000;
	const TInt offset = 0x8;
	const TUint8 magic = 0x74; // arbitrary magic value
	RChunk chunk;
	RHeap* heap;
	
	chunk.CreateLocal(0, size);
	size = chunk.MaxSize();	// X86 has 4MB chunk size
	
	// try and create a heap with a large offset - no room to make RHeap, should fail
	heap = UserHeap::OffsetChunkHeap(chunk, 0, size);
	test(heap==NULL);
	
	// write some magic numbers into the offset-reserved area
	chunk.Adjust(offset);
	TUint8* reserved = chunk.Base();
	TUint8* limit = reserved + offset;
	for (; reserved<limit; reserved++)
		*reserved = magic;
	
	// make a heap with an offset
	heap = UserHeap::OffsetChunkHeap(chunk, 0, offset);
	test(heap!=NULL);
	test(chunk.Base() + offset == (TUint8*)heap);
	TInt origsize = heap->Size();

	// force the heap to grow to the maximum size by allocating 1kb blocks
	// and then allocating whatever is left. Check this really is the end
	// of the chunk.
	TUint8* temp = NULL;
	TUint8* last = NULL;
	do
		{
		last = temp;
		temp = (TUint8*)heap->Alloc(1024);
		}
	while (temp != NULL);
	TInt biggestblock, space;
	space = heap->Available(biggestblock);
	if (space>0)
		{
		last = (TUint8*)heap->Alloc(space);
		test(last!=NULL);
		// Check that the last allocation doesn't pass the end of the chunk
		test(last+space <= chunk.Base()+size);
		// but that it is within the alignment requirement, as less than this
		// would be short of the end
		}
	else
		{
		test(last+1024 == chunk.Base()+size);
		}
	
	// try writing at the top end of it to make sure it's backed
	*(chunk.Base()+size-1) = 1;
	
	// test resetting the heap
	heap->Reset();
	test(origsize == heap->Size());
	
	// check reducing the heap works
	last = (TUint8*)heap->Alloc(size>>2);
	TInt midsize = heap->Size();
	temp = (TUint8*)heap->Alloc(size>>2);
	heap->Free(temp);
	heap->Compress();
	test(midsize == heap->Size());
	heap->Free(last);
	heap->Compress();
	test(origsize == heap->Size());
	
	// check the magic numbers are still there
	for (reserved = chunk.Base(); reserved<limit; reserved++)
		test(*reserved==magic);
	
	heap->Close();
	}


RSemaphore sem;
LOCAL_C void syncThreads(TAny* anArg)
//
// get the threads both running at the same time
//
	{
	if ((TInt)anArg==1)
		sem.Wait();
	else
		sem.Signal();
	}

TInt comeInNumber=0;
LOCAL_C TInt sharedHeapTest1(TAny* anArg)
//
// Shared heap test thread.
//
	{

	RHeap* pH = (RHeap*)&User::Allocator();
	if (gHeapPtr && pH!=gHeapPtr)
		return(KErrGeneral);
	gHeapPtr2 = pH;

	syncThreads(anArg);

	TAny* a[0x100];
	TInt mod=((TInt)anArg)*3;

	// Run in a timed loop, to ensure that we get some true concurrency
	RTimer timer;
	TTime now;
	TRequestStatus done;
	test(timer.CreateLocal()==KErrNone);
	now.HomeTime();
	timer.At(done,now+TTimeIntervalSeconds(20));

	while (done==KRequestPending && comeInNumber!=(TInt)anArg)
		{
		TInt i=0;
		for (;i<0x100;i++)
			{
			a[i]=User::Alloc(0x10);
			test(a[i]!=NULL);
			Mem::Fill(a[i],0x10,(((TInt)anArg)<<4)|(i&0x0F));	// marker
			if ((i%mod)==0)
				pH->Check();
			}
		for (i=0;i<0x100;i++)
			{
			User::Free(a[i]);
			if ((i%mod)==0)
				pH->Check();
			}
		}
	timer.Cancel();
	return((TInt)anArg);
	}

LOCAL_C void bumpKernelGranularity()
//
// Push up the kernels granularities
//
	{

	RThread t[4];
	TInt r;
	TUint i=0;
	for (;i<4;i++)
		{
		TName n;
		n.Format(_L("Temp%d"),i);
		r=t[i].Create(n,sharedHeapTest1,KDefaultStackSize,NULL,NULL);
		test(r==KErrNone);
		}
	for (i=0;i<4;i++)
		{
		t[i].Kill(KErrNone);
		t[i].Close();
		}
	}

LOCAL_C void createTestThreads(TThreadFunction aFunction,RHeap* aHeap)
//
// Create two test threads using the supplied entry point and heap
//
	{


	test.Next(_L("Create t1"));
	RThread t1;
	TInt r=t1.Create(_L("Shared1"),aFunction,KDefaultStackSize,aHeap,(TAny*)1);
	test(r==KErrNone);
	TRequestStatus tStat1;
	t1.Logon(tStat1);
	test(tStat1==KRequestPending);

	test.Next(_L("Create t2"));
	RThread t2;
	r=t2.Create(_L("Shared2"),aFunction,KDefaultStackSize,aHeap,(TAny*)2);
	test(r==KErrNone);
	TRequestStatus tStat2;
	t2.Logon(tStat2);
	test(tStat2==KRequestPending);

	test.Next(_L("Wait for t1 or t2 - approx 20 seconds"));
	t1.Resume();
	t2.Resume();
	User::WaitForRequest(tStat1,tStat2);
	User::WaitForRequest(tStat1==KRequestPending ? tStat1 : tStat2);
	test(tStat1==1);
	test(tStat2==2);
	CLOSE_AND_WAIT(t1);
	CLOSE_AND_WAIT(t2);
	}

LOCAL_C void SharedHeapTest1()
//
// Shared heap test using normal chunk heap
//
	{

	sem.CreateLocal(0);	// create synchronisation semaphore
	test.Start(_L("Create chunk to share"));
	TPtrC sharedHeap=_L("SharedHeap");
	TInt minsize = ((RHeap&)User::Allocator()).Size();
	gHeapPtr=User::ChunkHeap(&sharedHeap,minsize/*0x20000*/,0x40000);
	test(gHeapPtr!=NULL);
	TInt count=gHeapPtr->Count();
	createTestThreads(sharedHeapTest1,gHeapPtr);
	test(count==gHeapPtr->Count());
	gHeapPtr->Close();
	test.End();
	}

LOCAL_C void SharedHeapTest2()
//
// Shared heap test using the current threads heap. Can test kernel
// cleanup since granularity will have been handled by running
// SharedHeapTest2().
//
	{

	test.Start(_L("Current chunk to share"));
	test.Next(_L("Bump up granularities"));
//
// First create a number of threads to push up the kernels granularities
//
	bumpKernelGranularity();
//
	__KHEAP_MARK;
	gHeapPtr = (RHeap*)&User::Allocator();
	TInt biggest1;
	TInt avail1=gHeapPtr->Available(biggest1);
	TInt size1=gHeapPtr->Size();

	createTestThreads(sharedHeapTest1,NULL);
	
	TInt biggest2;
	TInt avail2=gHeapPtr->Available(biggest2);
	TInt size2=gHeapPtr->Size();
	test.Printf(_L("Before: size %d, %d available (biggest %d)\r\n"),size1,avail1,biggest1);
	test.Printf(_L("After:  size %d, %d available (biggest %d)\r\n"),size2,avail2,biggest2);
	test((size1-avail1)==(size2-avail2));	// no leaks
	if (avail1==biggest1)	// if it was a single block of free space before
		test(avail2==biggest2);	// then it should still be a single block
	__KHEAP_MARKEND;
	test.End();
	}

LOCAL_C void SharedHeapTest3()
//
// Shared heap test borrowing a thread's default heap and
// killing threads in different orders.
//
	{

	test.Start(_L("Create t1 whose heap will be shared"));
	gHeapPtr = NULL;
	RThread t1;
	TInt r=t1.Create(_L("Owner_T1"),sharedHeapTest1,KDefaultStackSize,0x20000,0x40000,(TAny*)1);
	test(r==KErrNone);
	TRequestStatus tStat1;
	t1.Logon(tStat1);
	test(tStat1==KRequestPending);
	t1.SetPriority(EPriorityMore); //t1 gets to wait on semaphore sem, before we start t2
	t1.Resume();
	test.Next(_L("Create t2 sharing t1's heap"));
	RThread t2;
	r=t2.Create(_L("Sharer_T2"),sharedHeapTest1,KDefaultStackSize,gHeapPtr2,(TAny*)2);
	test(r==KErrNone);
	TRequestStatus tStat2;
	t2.Logon(tStat2);
	test(tStat2==KRequestPending);

	test.Next(_L("Get t1 to exit while t2 continues running"));
	test(tStat1==KRequestPending);
	test(tStat2==KRequestPending);
	t1.SetPriority(EPriorityNormal); //back to the same priority as t2
	t2.Resume();
	test(tStat1==KRequestPending);
	test(tStat2==KRequestPending);
	comeInNumber=1;
	test.Next(_L("Wait for t1"));
	User::WaitForRequest(tStat1);
	test(tStat1==1);
	test(t1.ExitType()==EExitKill);
	test(t1.ExitReason()==1);
	test(tStat2==KRequestPending);
	test(t2.ExitType()==EExitPending);
	test.Next(_L("Wait for t2"));
	User::WaitForRequest(tStat2);
	test(tStat2==2);
	test(t2.ExitType()==EExitKill);
	test(t2.ExitReason()==2);
	CLOSE_AND_WAIT(t2);
	CLOSE_AND_WAIT(t1);
	test.End();
	}

LOCAL_C void TestAuto()
//
// Test heap auto expansion and compression
//
	{

	test.Start(_L("Create chunk to"));
	TPtrC autoHeap=_L("AutoHeap");
	gHeapPtr=User::ChunkHeap(&autoHeap,0x1800,0x6000);
	
	test(gHeapPtr!=NULL);
	TInt biggest;
	TInt avail=gHeapPtr->Available(biggest);
	test(avail==biggest);
	TAny *p1=gHeapPtr->Alloc(biggest);
	test(p1!=NULL);
	TAny *p2=gHeapPtr->Alloc(biggest);
	test(p2!=NULL);
	TAny *p3=gHeapPtr->Alloc(biggest);
	test(p3!=NULL);
	TAny *p4=gHeapPtr->Alloc(biggest);
	test(p4==NULL);
	TInt comp=gHeapPtr->Compress();
	test(comp==0);
	gHeapPtr->Free(p2);
	comp=gHeapPtr->Compress();
	test(comp==0);
	gHeapPtr->Free(p3);
	comp=gHeapPtr->Compress();
// stop wins compiler warning of constant expression as KHeapShrinkHysRatio
// isn't constant for non-emulator builds but ROM 'patchdata'
#pragma warning(disable : 4127)
	// When hysteresis value > 4.0*GrowBy then Free() calls
	// won't shrink heap but normally will shrink heap
	if (KHeapShrinkHysRatio <= 1024)
		test(comp==0);
	else
		test(comp==0x4000);
#pragma warning(default : 4127)
	gHeapPtr->Free(p1);
	comp=gHeapPtr->Compress();
	test(comp==0);
	TInt biggest1;
	TInt avail1=gHeapPtr->Available(biggest1);
	test(avail==avail1);
	test(biggest==biggest1);
	test(gHeapPtr->Count()==0);
	gHeapPtr->Close();
	test.End();
	}

LOCAL_C TInt NormalChunk(RChunk& aChunk, TInt aInitialSize, TInt aMaxSize)
    {
    TChunkCreateInfo createInfo;
    createInfo.SetNormal(aInitialSize, aMaxSize);
    TInt r=aChunk.Create(createInfo);
    return r;
    }    

LOCAL_C TInt DisconnectedChunk(RChunk& aChunk, TInt aInitialBottom, TInt aInitialTop, TInt aMaxSize)
    {
    TChunkCreateInfo createInfo;
    createInfo.SetDisconnected(aInitialBottom, aInitialTop, aMaxSize);
    TInt r=aChunk.Create(createInfo);
    return r;  
    }

LOCAL_C TBool TestIsHybridHeap(RHeap* aHeap)
    {
    RHybridHeap::STestCommand cmd;
    cmd.iCommand = RHybridHeap::EHeapMetaData;
    aHeap->DebugFunction(RHeap::EHybridHeap, (TAny*)&cmd, 0);
               
    RHybridHeap* hybridHeap = (RHybridHeap*) cmd.iData;
    return (TestHybridHeap::IsHybrid(hybridHeap));  
    }

LOCAL_C void TestHeapType()
    {
    TBool onlyDL = EFalse;
    _LIT(KHeap, "NamedHeap");
    // 1: Create a heap in a local chunk
    RHeap* heap;
    heap = UserHeap::ChunkHeap(NULL,0x100,0x2000);
    TBool hybrid = TestIsHybridHeap(heap);
    if (hybrid==0)
        {
        test.Printf(_L("Only DL allocator is in use \n"));
        onlyDL = ETrue;;
        }
    else
        test(hybrid==1); 
    heap->Close();
    
    // 2: Create a heap in a global chunk
    heap = UserHeap::ChunkHeap(&KHeap,0,0x1800,0x6000);
    hybrid = TestIsHybridHeap(heap);
    if(!onlyDL)
        test(hybrid==1); 
    heap->Close();
    
    // 3: Create a heap in an existing normal chunk
    RChunk chunk;
    TInt r = NormalChunk(chunk,0,0x1000);
    heap = UserHeap::ChunkHeap(chunk,0);
    hybrid = TestIsHybridHeap(heap);
    test(hybrid==0);
    heap->Close();
                   
    // 4: Create a heap in an existing disconnected chunk
    // when offset = 0.  Minimum heap size for a hybrid heap is 12KB
    r = DisconnectedChunk(chunk,0,0,0x3000);
    heap = UserHeap::ChunkHeap(chunk,0);
    hybrid = TestIsHybridHeap(heap);
    if(!onlyDL)
        test(hybrid==1);
    heap->Close();
        
    // 5: Create a heap in an existing disconnected chunk
    // when offset > 0
    r = DisconnectedChunk(chunk,0,0x1800,0x6000); 
    heap = UserHeap::OffsetChunkHeap(chunk,0,0x2800);
    hybrid = TestIsHybridHeap(heap);
    test(hybrid==0);
    heap->Close();
    
    // 6: Create a fixed length heap at a normal chunk's base address 
    r = NormalChunk(chunk,0x1000,0x1000);
    heap = UserHeap::FixedHeap(chunk.Base(), 0x1000);
    hybrid = TestIsHybridHeap(heap);
    test(hybrid==0);
    heap->Close();
    chunk.Close();
    
    // 7: Create a fixed length heap at a disconnected chunk's base address
    // when bottom = 0
    r = DisconnectedChunk(chunk,0,0x2000,0x2000); 
    heap = UserHeap::FixedHeap(chunk.Base(), 0x2000);
    hybrid = TestIsHybridHeap(heap);
    test(hybrid==0);
    heap->Close();
    chunk.Close();
        
    // 8: Create a fixed length heap at a disconnected chunk's base address
    // when bottom > 0
    r = DisconnectedChunk(chunk,0x6000,0x7000,0x13000); 
    heap = UserHeap::FixedHeap(chunk.Base()+ 0x6000, 0x1000); 
    hybrid = TestIsHybridHeap(heap);
    test(hybrid==0);
    heap->Close();
    chunk.Close();
    
    // 9: Create a fixed length heap for allocated buffer
    heap = UserHeap::ChunkHeap(&KNullDesC(), 4096, (4096 * 1024));
    test(heap != NULL);
    TAny* buffer = heap->Alloc(1024 * 1024);    
    test(buffer != NULL);
    TInt lth = heap->AllocLen(buffer);
    test.Printf(_L("Fixed heap buffer: %x, length: %x \n"), buffer, lth);  

    RHeap* heapf = UserHeap::FixedHeap(buffer, (1024 * 1024));
    test(heapf != NULL);
    test.Printf(_L("Fixed heap: %x \n"), heapf);
    hybrid = TestIsHybridHeap(heapf);
    test(hybrid==0);

    heapf->Close();
    heap->Free(buffer);

    heap->Close();
    }

GLDEF_C TInt E32Main(void)
	{

	test.Title();

	__KHEAP_MARK;

	test.Start(_L("Test 1"));
	UserHal::PageSizeInBytes(PageSize);
        TestRHeap T;

	T.Test1();
	test.Next(_L("Test auto expand and compress"));
	TestAuto();
	test.Next(_L("Test 2"));
	T.Test2();
	test.Next(_L("Test 3"));
	T.Test3();
	test.Next(_L("Test 4"));
	T.Test4();
	test.Next(_L("Test 5"));
	T.Test5();
	test.Next(_L("Test 7"));
	T.Test7();
	test.Next(_L("Test 8"));
	T.Test8();
	test.Next(_L("Test CompressAll()"));
	T.TestCompressAll();
	test.Next(_L("Test offset heap"));
	T.TestOffset();
	test.Next(_L("Shared heap test 1"));
	SharedHeapTest1();
	test.Next(_L("Shared heap test 2"));
	SharedHeapTest2();
	test.Next(_L("Shared heap test 3"));
	SharedHeapTest3();
	sem.Close();
	test.Next(_L("Test HeapType()"));    
	TestHeapType();

	__KHEAP_CHECK(0);
	__KHEAP_MARKEND;

	test.End();
	return(0);
    }

