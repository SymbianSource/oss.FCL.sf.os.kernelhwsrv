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
// e32test\heap\t_heap2.cpp
// Overview:
// Tests RHeap class, including a stress test and a "grow in place"
// ReAlloc test.
// API Information:
// RHeap
// Details:
// - Test allocation on fixed length heaps in local, disconnected chunks for
// different heap sizes and alignments.  Assumes knowledge of heap
// implementation.
// - Test allocation, free, reallocation and compression on chunk heaps with
// different maximum and minimum lengths and alignments.  Assumes knowledge
// of heap implementation.      
// - Stress test heap implementation with a single thread that allocates, frees
// and reallocates cells, and checks the heap.
// - Stress test heap implementation with two threads that run concurrently.
// - Create a chunk heap, test growing in place by allocating a cell and 
// then reallocating additional space until failure, verify that the cell 
// did not move and the size was increased.
// - The heap is checked to verify that no cells remain allocated after the 
// tests are complete.
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

// Needed for KHeapShrinkHysRatio which is now ROM 'patchdata'
#include "TestRHeapShrink.h"

#define DECL_GET(T,x)		inline T x() const {return i##x;}
#define DECL_GET2(T,x,y)	inline T y() const {return i##x;}


#ifdef __EABI__
       IMPORT_D extern const TInt KHeapMinCellSize;
#else
       const TInt KHeapMinCellSize = 0;
#endif

RTest test(_L("T_HEAP2"));

#define	TEST_ALIGN(p,a)		test((TLinAddr(p)&((a)-1))==0)

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
	aLength -= RHeap::EAllocCellSize;
	if (aLength==0)
		return;
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
	aLength -= RHeap::EAllocCellSize;
	if (aLength==0)
		return;
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
	DECL_GET(TInt,AccessCount)
	DECL_GET(TInt,HandleCount)
	DECL_GET(TInt*,Handles)
	DECL_GET(TUint32,Flags)
	DECL_GET(TInt,CellCount)
	DECL_GET(TInt,TotalAllocSize)
	DECL_GET(TInt,MinLength)
	DECL_GET(TInt,Offset)
	DECL_GET(TInt,GrowBy)
	DECL_GET(TInt,ChunkHandle)
	DECL_GET2(const RFastLock&,Lock,LockRef)
	DECL_GET(TUint8*,Top)
	DECL_GET(TInt,Align)
	DECL_GET(TInt,MinCell)
	DECL_GET(TInt,PageSize)
	DECL_GET2(const SCell&,Free,FreeRef)
public:
	TInt CheckAllocatedCell(const TAny* aCell) const;
	void FullCheckAllocatedCell(const TAny* aCell) const;
	TAny* TestAlloc(TInt aSize);
	void TestFree(TAny* aPtr);
	TAny* TestReAlloc(TAny* aPtr, TInt aSize, TInt aMode=0);
	void FullCheck();
	static void WalkFullCheckCell(TAny* aPtr, TCellType aType, TAny* aCell, TInt aLen);
	TInt FreeCellLen(const TAny* aPtr) const;
	static RTestHeap* FixedHeap(TInt aMaxLength, TInt aAlign=0, TBool aSingleThread=ETrue);
	void TakeChunkOwnership(RChunk aChunk);
	TInt LastFreeCellLen(void) const;
	TInt CalcComp(TInt aCompSize);
	void ForceCompress(TInt aFreed);
	};

TInt RTestHeap::CheckAllocatedCell(const TAny* aCell) const
	{
	SCell* pC = GetAddress(aCell);
	TInt len = pC->len;
	TUint8* pEnd = (TUint8*)pC + len;
	TEST_ALIGN(aCell, iAlign);
	TEST_ALIGN(len, iAlign);
	test(len >= iMinCell);
	test((TUint8*)pC>=iBase && pEnd<=iTop);
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
		test((len-RHeap::EAllocCellSize)>=aSize);
		((STestCell*)p)->Set(len);
		}
	return p;
	}

void RTestHeap::TestFree(TAny* aPtr)
	{
	if (aPtr)
		FullCheckAllocatedCell(aPtr);
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
	test((new_len-RHeap::EAllocCellSize)>=aSize);
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
		((STestCell*)p)->Verify(aPtr, old_len, Min(old_len, new_len));
	if (new_len != old_len)
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
			test(aCell == info.iNextCell);
			TInt len = ((SCell*)aCell)->len;
			test(len == aLen);
			info.iNextCell += len;
			++info.iTotalAlloc;
			info.iTotalAllocSize += (aLen-EAllocCellSize);
			STestCell* pT = (STestCell*)((TUint8*)aCell + EAllocCellSize);
			pT->Verify(len);
			break;
			}
		case EGoodFreeCell:
			{
			test(aCell == info.iNextCell);
			TInt len = ((SCell*)aCell)->len;
			test(len == aLen);
			info.iNextCell += len;
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
	info.iNextCell = iBase;
	DebugFunction(EWalk, (TAny*)&WalkFullCheckCell, &info);
	test(info.iNextCell == iTop);
	test(info.iTotalAlloc == iCellCount);
	test(info.iTotalAllocSize == iTotalAllocSize);
	}

TInt RTestHeap::FreeCellLen(const TAny* aPtr) const
	{
	SCell* p = iFree.next;
	SCell* q = (SCell*)((TUint8*)aPtr - EAllocCellSize);
	for (; p && p!=q; p = p->next) {}
	if (p == q)
		return p->len - EAllocCellSize;
	return -1;
	}

TInt RTestHeap::LastFreeCellLen(void) const
	{
	SCell* p = iFree.next;
	if (p==NULL)
		return -1;	
	for (; p->next; p=p->next){}
	return p->len;
	}


/** Checks whether a call to Compress() will actually perform a reduction 
	of the heap.
	Relies on the free last cell on the heap being cell that has just been freed
	plus any extra.
	Intended for use by t_heap2.cpp - DoTest4().  
	@param aFreedSize The size in bytes of the cell that was freed
*/
TInt RTestHeap::CalcComp(TInt aFreedSize)
	{	
	TInt largestCell=0;
	largestCell = LastFreeCellLen();
	// if the largest cell is too small or it would have been compressed by the
	// free operation then return 0.
	if (largestCell < iPageSize || aFreedSize >= KHeapShrinkHysRatio*(iGrowBy>>8))
		{
		return 0;			
		}
		else
		{
		return _ALIGN_DOWN(aFreedSize,iPageSize);
		}	
	}
	
/** compress the heap if the KHeapShrinkRatio is too large for what we are
	expecting in DoTest4().
*/
void RTestHeap::ForceCompress(TInt aFreed)
	{	
	if (aFreed < KHeapShrinkHysRatio*(iGrowBy>>8))
		{
		Compress();
		}
	}
RTestHeap* RTestHeap::FixedHeap(TInt aMaxLength, TInt aAlign, TBool aSingleThread)
	{
	RChunk c;
	TInt bottom = 0x40000;
	TInt top = bottom + aMaxLength;
	TInt r = c.CreateDisconnectedLocal(bottom, top, top + bottom, EOwnerThread);
	if (r!=KErrNone)
		return NULL;
	TUint8* base = c.Base() + bottom;
	RTestHeap* h = (RTestHeap*)UserHeap::FixedHeap(base, aMaxLength, aAlign, aSingleThread);
	if (!aAlign)
		aAlign = RHeap::ECellAlignment;
	test((TUint8*)h == base);
	test(h->AccessCount() == 1);
	test(h->HandleCount() == (aSingleThread ? 0 : 1));
	test(h->Handles() == (aSingleThread ? NULL : (TInt*)&h->LockRef()));
	test(h->Flags() == TUint32(RAllocator::EFixedSize | (aSingleThread ? RAllocator::ESingleThreaded : 0)));
	test(h->CellCount() == 0);
	test(h->TotalAllocSize() == 0);
	test(h->MaxLength() == aMaxLength);
	test(h->MinLength() == h->Top() - (TUint8*)h);
	test(h->Offset() == 0);
	test(h->GrowBy() == 0);
	test(h->ChunkHandle() == 0);
	test(h->Align() == aAlign);
	TInt min_cell = _ALIGN_UP((KHeapMinCellSize + Max((TInt)RHeap::EAllocCellSize, (TInt)RHeap::EFreeCellSize)), aAlign);
	TInt hdr_len = _ALIGN_UP(sizeof(RHeap) + RHeap::EAllocCellSize, aAlign) - RHeap::EAllocCellSize;
	TInt user_len = _ALIGN_DOWN(aMaxLength - hdr_len, aAlign);
	test(h->Base() == base + hdr_len);
	test(h->MinCell() == min_cell);
	test(h->Top() - h->Base() == user_len);
	test(h->FreeRef().next == (RHeap::SCell*)h->Base());
	h->TakeChunkOwnership(c);
	return h;
	}

void RTestHeap::TakeChunkOwnership(RChunk aChunk)
	{
	iChunkHandle = aChunk.Handle();
	++iHandleCount;
	iHandles = &iChunkHandle;
	}


#define	ACCESS_COUNT(h)		(((RTestHeap*)h)->AccessCount())
#define	HANDLE_COUNT(h)		(((RTestHeap*)h)->HandleCount())
#define	HANDLES(h)			(((RTestHeap*)h)->Handles())
#define	FLAGS(h)			(((RTestHeap*)h)->Flags())
#define	CELL_COUNT(h)		(((RTestHeap*)h)->CellCount())
#define	TOTAL_ALLOC_SIZE(h)	(((RTestHeap*)h)->TotalAllocSize())
#define	MIN_LENGTH(h)		(((RTestHeap*)h)->MinLength())
#define	OFFSET(h)			(((RTestHeap*)h)->Offset())
#define	GROW_BY(h)			(((RTestHeap*)h)->GrowBy())
#define	CHUNK_HANDLE(h)		(((RTestHeap*)h)->ChunkHandle())
#define	LOCK_REF(h)			(((RTestHeap*)h)->LockRef())
#define	TOP(h)				(((RTestHeap*)h)->Top())
#define	ALIGN(h)			(((RTestHeap*)h)->Align())
#define	MIN_CELL(h)			(((RTestHeap*)h)->MinCell())
#define	PAGE_SIZE(h)		(((RTestHeap*)h)->PageSize())
#define	FREE_REF(h)			(((RTestHeap*)h)->FreeRef())

void DoTest1(RHeap* aH)
	{
	RTestHeap* h = (RTestHeap*)aH;
	test.Printf(_L("Test Alloc: min=%x max=%x align=%d growby=%d\n"),
						h->MinLength(), h->MaxLength(), h->Align(), h->GrowBy());
	TInt l;
	TAny* p = NULL;
	TUint8* next = h->Base();
	TUint8* top = h->Top();
	TUint8* limit = (TUint8*)h + h->MaxLength();
	TBool fixed = h->Flags() & RAllocator::EFixedSize;
	for (l=1; l<=1024; ++l)
		{
		TInt remain1 = top - next;
		TInt xl1 = _ALIGN_UP(Max((l+RHeap::EAllocCellSize), h->MinCell()), h->Align());
		p = h->TestAlloc(l);
		if ( (fixed && remain1 < xl1) || (next + xl1 > limit) )
			{
			test(p == NULL);
			test(top == h->Top());
			test.Printf(_L("Alloc failed at l=%d next=%08x\n"), l, next);
			break;
			}
		test(p == next + RHeap::EAllocCellSize);
		if (xl1 > remain1)
			{
			// no room for this cell
			TInt g = h->GrowBy();
			while (xl1 > remain1)
				{
				top += g;
				remain1 += g;
				}
			}
		test(top == h->Top());
		if (xl1 + h->MinCell() > remain1)
			{
			// this cell fits but remainder is too small or nonexistent
			xl1 = top - next;
			next = top;
			test(h->FreeRef().next == NULL);
			}
		else
			{
			// this cell fits and remainder can be reused
			next += xl1;
			}
		test(aH->AllocLen(p) == xl1 - RHeap::EAllocCellSize);
		}
	h->FullCheck();
	}

void DoTest2(RHeap* aH)
	{
	RTestHeap* h = (RTestHeap*)aH;
	test.Printf(_L("Test Free: min=%x max=%x align=%d growby=%d\n"),
						h->MinLength(), h->MaxLength(), h->Align(), h->GrowBy());
	TInt al;
	TInt min = h->MinCell();
	TBool pad = EFalse;
	for (al=1; al<256; (void)((pad=!pad)!=0 || (al+=al+1)) )
		{
		TAny* p[32];
		TInt last_len = 0;
		TAny* last = NULL;
		TInt i;
		test.Printf(_L("al=%d pad=%d\n"), al, pad);
		TUint8* top=0;
		TAny* spare=0;
		TBool heapReduced = EFalse;
		for (i=0; i<32; ++i)
			{
			// Check whether the cell created for the allocation of al would end up
			// including extra bytes from the last free cell that aren't enough
			// to create a new free cell.
			top = h->Top();
			TInt freeLen=h->LastFreeCellLen();
			TInt actualAllocBytes = Max(_ALIGN_UP(al + RHeap::EAllocCellSize, h->Align()), min);
			TInt remainingBytes = freeLen - actualAllocBytes;
			if (remainingBytes < min)
				{
				// Force the heap to grow so that once this allocation is freed
				// the free cell left will be large enough to include the al allocation
				// and to create a new free cell if necessary.
				actualAllocBytes = _ALIGN_UP(actualAllocBytes + min, h->Align());
				TAny* q = h->TestAlloc(actualAllocBytes);
				// Check heap has grown
				test(top < h->Top());
				top = h->Top();
				test(q!=NULL);
				// Have grown the heap so allocate a cell as a place holder to stop
				// the heap being shrunk and the actual cell we want to allocate from being the
				// wrong size
				spare=h->TestAlloc(8);
				h->TestFree(q);
				// Ensure heap wasn't shrunk after free
				test(top == h->Top());
				}
			top = h->Top();
			// Allocate the new 
			p[i] = h->TestAlloc(al);
			test(p[i]!=NULL);
			if (remainingBytes < min)
				{// now safe to free any padding as p[i] now allocated and its size can't change
				h->TestFree(spare);
				}
			TInt tmp1=h->AllocLen(p[i]);
			TInt tmp2=Max(_ALIGN_UP(al+RHeap::EAllocCellSize,h->Align()), min)-RHeap::EAllocCellSize;
			test(tmp1 == tmp2);
			}
		last = (TUint8*)p[31] + _ALIGN_UP(Max((al + RHeap::EAllocCellSize), min), h->Align());
		last_len = h->FreeCellLen(last);
		test(last_len > 0);
		if (pad)
			{
			test(h->TestAlloc(last_len) == last);
			test(h->FreeRef().next == NULL);
			}
		else
			last = NULL;
		top = h->Top();
		for (i=0,heapReduced=EFalse; i<32; ++i)
			{
			h->TestFree(p[i]);
			TInt fl = h->FreeCellLen(p[i]);
			TInt xfl = _ALIGN_UP(Max((al + RHeap::EAllocCellSize), h->MinCell()), h->Align()) - RHeap::EAllocCellSize;
			if (h->Top() < top) // heap was reduced due to small KHeapShrinkHysRatio and big KHeapMinCellSize
				{
				top = h->Top();
				heapReduced = ETrue;
				}

			if (i < 31 || pad)
				test(fl == xfl);
			else
				{
				if (!heapReduced)
					test(fl == xfl + RHeap::EAllocCellSize + last_len);
				else
					{
					heapReduced = EFalse;
					}
				}
			test(h->TestAlloc(al)==p[i]);
			}
		for (i=0,heapReduced=EFalse; i<31; ++i)
			{
			TInt j = i+1;
			TUint8* q;
			// Free to adjacent cells and check that the free cell left is the combined
			// size of the 2 adjacent cells just freed
			h->TestFree(p[i]);
			h->TestFree(p[j]);
			TInt fl = h->FreeCellLen(p[i]);
			if (h->Top() < top) // heap was reduced due to small KHeapShrinkHysRatio and big KHeapMinCellSize
				{
				top = h->Top();
				heapReduced = ETrue;
				}
			TInt xfl = 2 * _ALIGN_UP(Max((al + RHeap::EAllocCellSize), h->MinCell()), h->Align()) - RHeap::EAllocCellSize;
			if (j < 31 || pad)
				test(fl == xfl);
			else
				{
				if (!heapReduced)
					test(fl == xfl + RHeap::EAllocCellSize + last_len);
				else
					{
					heapReduced = EFalse;
					}
				}
			test(h->FreeCellLen(p[j]) < 0);
			test(h->TestAlloc(fl)==p[i]);
			test(h->Top() == top);
			h->TestFree(p[i]);
			test(h->FreeCellLen(p[i]) == fl);
			// test when you alloc a cell that is larger than cells just freed
			// that its position is not the same as the freed cells
			// will hold for all cells except top/last one
			if (j < 31 && !pad && fl < last_len)
				{
				q = (TUint8*)h->TestAlloc(fl+1);
				if (h->Top() > top)
					top = h->Top();
				test(h->Top() == top);
				test(q > p[i]);
				h->TestFree(q);
				if (h->Top() < top) // heap was reduced due to small KHeapShrinkHysRatio and big KHeapMinCellSize
					{
					top = h->Top();
					heapReduced = ETrue;
					}
				}
			// check cell that is just smaller than space but not small enough 
			// for a new free cell to be created, is the size of whole free cell
			test(h->TestAlloc(fl-min+1)==p[i]);
			test(h->Top() == top);
			test(h->AllocLen(p[i])==fl);
			h->TestFree(p[i]);
			// Check cell that is small enough for new free cell and alloc'd cell to be
			// created at p[i] cell is created at p[i]
			test(h->TestAlloc(fl-min)==p[i]);
			test(h->Top() == top);
			// check free cell is at expected position
			q = (TUint8*)p[i] + fl - min + RHeap::EAllocCellSize;
			test(h->FreeCellLen(q) == min - RHeap::EAllocCellSize);
			// alloc 0 length cell at q, will work as new cell of min length will be created
			test(h->TestAlloc(0) == q);
			test(h->Top() == top);
			h->TestFree(p[i]);
			test(h->FreeCellLen(p[i]) == fl - min);
			h->TestFree(q);
			// again check free cells are combined
			test(h->FreeCellLen(q) < 0);
			test(h->FreeCellLen(p[i]) == fl);
			// check reallocating the cells places them back to same positions
			test(h->TestAlloc(al)==p[i]);
			test(h->Top() == top);
			test(h->TestAlloc(al)==p[j]);
			test(h->Top() == top);
			if (pad)
				test(h->FreeRef().next == NULL);
			}
		for (i=0,heapReduced=EFalse; i<30; ++i)
			{
			TInt j = i+1;
			TInt k = i+2;
			TUint8* q;
			// Free 3 adjacent cells and check free cell created is combined size
			h->TestFree(p[i]);
			h->TestFree(p[k]);
			h->TestFree(p[j]);
			h->FullCheck();
			if (h->Top() < top) // heap was reduced due to small KHeapShrinkHysRatio and big KHeapMinCellSize
				{
				top = h->Top();
				heapReduced = ETrue;
				}
			TInt fl = h->FreeCellLen(p[i]);
			TInt xfl = 3 * _ALIGN_UP(Max((al + RHeap::EAllocCellSize), h->MinCell()), h->Align()) - RHeap::EAllocCellSize;
			if (k < 31 || pad)
				test(fl == xfl);
			else
				{
				if (!heapReduced)
					test(fl == xfl + RHeap::EAllocCellSize + last_len);
				else
					{
					heapReduced = EFalse;
					}
				}
			test(h->FreeCellLen(p[j]) < 0);
			test(h->FreeCellLen(p[k]) < 0);
			//ensure created free cell is allocated to new cell of free cell size
			test(h->TestAlloc(fl)==p[i]);
			test(h->Top() == top);
			h->TestFree(p[i]);
			test(h->FreeCellLen(p[i]) == fl);
			if (h->Top() < top) // heap was reduced due to small KHeapShrinkHysRatio and big KHeapMinCellSize
				top = h->Top();
			if (k < 31 && !pad && fl < last_len)
				{
				// Test new cell one larger than free cell size is allocated somewhere else
				q = (TUint8*)h->TestAlloc(fl+1);
				if (h->Top() > top)
					top = h->Top();
				test(h->Top() == top); 
				test(q > p[i]);
				h->TestFree(q);
				if (h->Top() < top) // heap was reduced due to small KHeapShrinkHysRatio and big KHeapMinCellSize
					{
					top = h->Top();
					heapReduced = ETrue;
					}
				}
			// check allocating cell just smaller than free cell size but
			// too large for neew free cell to be created, is size of whole free cell
			test(h->TestAlloc(fl-min+1)==p[i]);
			test(h->Top() == top);
			test(h->AllocLen(p[i])==fl);
			h->TestFree(p[i]);
			// ensure free cell is created this time as well as alloc'd cell
			test(h->TestAlloc(fl-min)==p[i]);
			test(h->Top() == top);
			q = (TUint8*)p[i] + fl - min + RHeap::EAllocCellSize;
			test(h->FreeCellLen(q) == min - RHeap::EAllocCellSize);
			test(h->TestAlloc(0) == q);
			test(h->Top() == top);
			h->TestFree(p[i]);
			test(h->FreeCellLen(p[i]) == fl - min);
			h->TestFree(q);
			test(h->FreeCellLen(q) < 0);
			test(h->FreeCellLen(p[i]) == fl);
			// realloc all cells and check heap not expanded
			test(h->TestAlloc(al)==p[i]);
			test(h->Top() == top);
			test(h->TestAlloc(al)==p[j]);
			test(h->Top() == top);
			test(h->TestAlloc(al)==p[k]);
			test(h->Top() == top);
			// If padding than no space should left on heap
			if (pad)
				test(h->FreeRef().next == NULL);
			}
		// when padding this will free padding from top of heap
		h->TestFree(last);
		}
	h->FullCheck();
	}

void DoTest3(RHeap* aH)
	{
	RTestHeap* h = (RTestHeap*)aH;
	test.Printf(_L("Test ReAlloc: min=%x max=%x align=%d growby=%d\n"),
						h->MinLength(), h->MaxLength(), h->Align(), h->GrowBy());
	// allocate continuous heap cell, then free them and reallocate again
	TInt al;
	for (al=1; al<256; al+=al+1)
		{
		TAny* p0 = h->TestAlloc(al);
		TInt al0 = h->AllocLen(p0);
		h->TestFree(p0);
		TAny* p1 = h->TestReAlloc(NULL, al, 0);
		TInt al1 = h->AllocLen(p1);
		test(p1 == p0);
		test(al1 == al0);
		h->TestFree(p1);
		TAny* p2 = h->TestAlloc(1);
		TAny* p3 = h->TestReAlloc(p2, al, 0);
		test(p3 == p0);
		TInt al3 = h->AllocLen(p3);
		test(al3 == al0);
		h->TestFree(p3);
		TAny* p4 = h->TestAlloc(1024);
		TAny* p5 = h->TestReAlloc(p4, al, 0);
		test(p5 == p0);
		TInt al5 = h->AllocLen(p5);
		test(al5 == al0);
		h->TestFree(p5);
		}
	TInt i;
	TInt j;
	for (j=0; j<30; j+=3)
		{
		TAny* p[30];
		TInt ala[30];
		TInt fla[30];
		h->Reset();
		for (i=0; i<30; ++i)
			{
			p[i] = h->TestAlloc(8*i*i);
			ala[i] = h->AllocLen(p[i]);
			fla[i] = 0;
			}
		for (i=1; i<30; i+=3)
			{
			h->TestFree(p[i]);
			fla[i] = h->FreeCellLen(p[i]);
			test(fla[i] == ala[i]);
			test(h->FreeCellLen(p[i-1]) < 0);
			test(h->FreeCellLen(p[i+1]) < 0);
			}
		h->FullCheck();
		TInt al1 = _ALIGN_UP(Max((RHeap::EAllocCellSize + 1), h->MinCell()), h->Align());
		// adjust al1 for some case when reallocated heap cell will not be shrinked because remainder will not big enough
		// to form a new free cell due to a big KHeapMinCellSize value
		TInt alaj = ala[j] + RHeap::EAllocCellSize;
		if (al1 < alaj && alaj - al1 < h->MinCell())
			al1 = alaj;
		TAny* p1 = h->TestReAlloc(p[j], 1, RHeap::ENeverMove);
		test(p1 == p[j]);
		test(h->AllocLen(p1) == al1 - RHeap::EAllocCellSize);
		TAny* p1b = (TUint8*)p1 + al1;
		test(h->FreeCellLen(p1b) == fla[j+1] + RHeap::EAllocCellSize + ala[j] - al1);
		TInt l2 = ala[j] + fla[j+1] + RHeap::EAllocCellSize; // max without moving
		TInt l3 = l2 - h->MinCell();
		TAny* p3 = h->TestReAlloc(p[j], l3, RHeap::ENeverMove);
		test(p3 == p[j]);
		TAny* p3b = (TUint8*)p3 + h->AllocLen(p3) + RHeap::EAllocCellSize;
		test(h->FreeCellLen(p3b) == h->MinCell() - RHeap::EAllocCellSize);
		TAny* p2 = h->TestReAlloc(p[j], l2, RHeap::ENeverMove);
		test(p2 == p[j]);
		test(h->AllocLen(p2) == l2);
		TAny* p4 = h->TestReAlloc(p[j], l2+1, RHeap::ENeverMove);
		test(p4 == NULL);
		test(h->AllocLen(p2) == l2);
		TAny* p5 = h->TestReAlloc(p[j], l2+1, 0);
		TInt k = 0;
		for (; k<30 && fla[k] <= l2; ++k) {}
		if (k < 30)
			test(p5 == p[k]);
		else
			test(p5 >= (TUint8*)p[29] + ala[29]);
		test(h->FreeCellLen(p2) == ala[j] + ala[j+1] + RHeap::EAllocCellSize);
		TInt ali = _ALIGN_UP(RHeap::EAllocCellSize,h->Align());
		TAny* p6b = (TUint8*)p[j+2] + ala[j+2] - ali + RHeap::EAllocCellSize;
		test(h->FreeCellLen(p6b) < 0);
		TAny* p6 = h->TestReAlloc(p[j+2], ala[j+2] - ali , 0);
		test(p6 == p[j+2]);
		if (h->AllocLen(p6) != ala[j+2]) // allocated heap cell size changed
			test(h->FreeCellLen(p6b) == h->MinCell() - RHeap::EAllocCellSize);
		TInt g = h->GrowBy();
		TAny* p7 = h->TestReAlloc(p5, 8*g, 0);
		test(p7 >= p5);
		TUint8* p8 = (TUint8*)p7 - RHeap::EAllocCellSize + al1;
		TUint8* p9 = (TUint8*)_ALIGN_UP(TLinAddr(p8), h->PageSize());
		if (p9-p8 < h->MinCell())
			p9 += h->PageSize();
		TAny* p7b = h->TestReAlloc(p7, 1, 0);
		test(p7b == p7);
		test(h->Top() + (RHeap::EAllocCellSize & (h->Align()-1)) == p9);

		h->FullCheck();
		}
	}

// Test compression
// {1 free cell, >1 free cell} x {reduce cell, eliminate cell, reduce cell but too small}
//
void DoTest4(RHeap* aH)
	{
	RTestHeap* h = (RTestHeap*)aH;
	test.Printf(_L("Test Compress: min=%x max=%x align=%d growby=%d\n"),
						h->MinLength(), h->MaxLength(), h->Align(), h->GrowBy());
	TInt page_size;
	UserHal::PageSizeInBytes(page_size);
	test(page_size == h->PageSize());
	TInt g = h->GrowBy();
	TEST_ALIGN(g, page_size);
	test(g >= page_size);
	RChunk c;
	c.SetHandle(h->ChunkHandle());
	TInt align = h->Align();
	TInt minc = h->MinCell();

	TInt orig_size = c.Size();
	TUint8* orig_top = h->Top();

	// size in bytes that last free cell on the top of the heap must be 
	// before the heap will be shrunk, size must include the no of bytes to
	// store the cell data/header i.e RHeap::EAllocCellSize
	TInt shrinkThres = KHeapShrinkHysRatio*(g>>8);

	TInt pass;
	for (pass=0; pass<2; ++pass)
		{
		TUint8* p0 = (TUint8*)h->TestAlloc(4);
		test(p0 == h->Base() + RHeap::EAllocCellSize);
		TInt l1 = h->Top() - (TUint8*)h->FreeRef().next;
		TEST_ALIGN(l1, align);
		l1 -= RHeap::EAllocCellSize;
		TUint8* p1;
		// Grow heap by 2*iGrowBy bytes
		p1 = (TUint8*)h->TestAlloc(l1 + 2*g);
		test(p1 == p0 + h->AllocLen(p0) + RHeap::EAllocCellSize);
		test(h->Top() - orig_top == 2*g);
		test(c.Size() - orig_size == 2*g);
		// May compress heap, may not
		h->TestFree(p1);
		h->ForceCompress(2*g);
		test(h->Top() == orig_top);
		test(c.Size() == orig_size);
		test((TUint8*)h->FreeRef().next == p1 - RHeap::EAllocCellSize);
		h->FullCheck();
		//if KHeapShrinkHysRatio is > 2.0 then heap compression will occur here
		test(h->Compress() == 0);
		test(h->TestAlloc(l1) == p1);
		test(h->FreeRef().next == NULL);
		if (pass)
			h->TestFree(p0);	// leave another free cell on second pass
		TInt l2 = g - RHeap::EAllocCellSize;
		// Will grow heap by iGrowBy bytes
		TUint8* p2 = (TUint8*)h->TestAlloc(l2);
		test(p2 == orig_top + RHeap::EAllocCellSize);
		test(h->Top() - orig_top == g);
		test(c.Size() - orig_size == g);
		// may or may not compress heap
		h->TestFree(p2);
		if (l2+RHeap::EAllocCellSize >= shrinkThres)
			{
			// When KHeapShrinkRatio small enough heap will have been compressed
			test(h->Top() == orig_top);			
			if (pass)
				{
				test((TUint8*)h->FreeRef().next == p0 - RHeap::EAllocCellSize);
				test((TUint8*)h->FreeRef().next->next == NULL);
				}
			else
				test((TUint8*)h->FreeRef().next == NULL);
			}
		else
			{			
			test(h->Top() - orig_top == g);
			if (pass)
				{
				test((TUint8*)h->FreeRef().next == p0 - RHeap::EAllocCellSize);
				test((TUint8*)h->FreeRef().next->next == orig_top);
				}
			else
				test((TUint8*)h->FreeRef().next == orig_top);
			}
		// this compress will only do anything if the KHeapShrinkRatio is large 
		// enough to introduce hysteresis otherwise the heap would have been compressed 
		// by the free operation itself
		TInt tmp1,tmp2;
		tmp2=h->CalcComp(g);
		tmp1=h->Compress();
		test(tmp1 == tmp2);
		test(h->Top() == orig_top);
		test(c.Size() == orig_size);
		h->FullCheck();
		// shouldn't compress heap as already compressed
		test(h->Compress() == 0);
		//grow heap by iGrowBy bytes
		test(h->TestAlloc(l2) == p2);
		//grow heap by iGrowBy bytes
		TUint8* p3 = (TUint8*)h->TestAlloc(l2);
		test(p3 == p2 + g);
		test(h->Top() - orig_top == 2*g);
		test(c.Size() - orig_size == 2*g);
		// may or may not reduce heap
		h->TestFree(p2);
		// may or may not reduce heap
		h->TestFree(p3);
		h->ForceCompress(2*g);
		test(h->Top() == orig_top);
		test(c.Size() == orig_size);
		h->FullCheck();
		if (pass)
			{
			test((TUint8*)h->FreeRef().next == p0 - RHeap::EAllocCellSize);
			test((TUint8*)h->FreeRef().next->next == NULL);
			}
		else
			test((TUint8*)h->FreeRef().next == NULL);
		//grow heap by iGrowBy bytes
		test(h->TestAlloc(l2) == p2);
		//grow heap by iGrowBy*2 + page size bytes
		test(h->TestAlloc(l2 + g + page_size) == p3);
		test(h->Top() - orig_top == 4*g);
		test(c.Size() - orig_size == 4*g);
		// will compress heap if KHeapShrinkHysRatio <= KHeapShrinkRatioDflt
		test(h->TestReAlloc(p3, page_size - RHeap::EAllocCellSize, 0) == p3);
		h->ForceCompress(g+page_size);
		test(h->Top() - orig_top == g + page_size);
		test(c.Size() - orig_size == g + page_size);
		h->FullCheck();
		// will compress heap if KHeapShrinkHysRatio <= KHeapShrinkRatio1
		h->TestFree(p2);
		// will compress heap if KHeapShrinkHysRatio <= KHeapShrinkRatio1 && g<=page_size
		// or KHeapShrinkHysRatio >= 2.0 and g==page_size
		h->TestFree(p3);
		// may or may not perform further compression
		tmp1=h->CalcComp(g+page_size);
		tmp2=h->Compress();
		test(tmp1 == tmp2);
		test(h->Top() == orig_top);
		test(c.Size() == orig_size);
		h->FullCheck();
		test(h->TestAlloc(l2 - minc) == p2);
		test(h->TestAlloc(l2 + g + page_size + minc) == p3 - minc);
		test(h->Top() - orig_top == 4*g);
		test(c.Size() - orig_size == 4*g);
		h->TestFree(p3 - minc);
		h->ForceCompress(l2 + g + page_size + minc);
		test(h->Top() - orig_top == g);
		test(c.Size() - orig_size == g);
		h->FullCheck();
		if (pass)
			{
			test((TUint8*)h->FreeRef().next == p0 - RHeap::EAllocCellSize);
			test((TUint8*)h->FreeRef().next->next == p3 - minc - RHeap::EAllocCellSize);
			}
		else
			test((TUint8*)h->FreeRef().next == p3 - minc - RHeap::EAllocCellSize);
		h->TestFree(p2);
		if (l2+RHeap::EAllocCellSize >= shrinkThres)
			{
			// When KHeapShrinkRatio small enough heap will have been compressed
			test(h->Top() == orig_top);
			test(c.Size() - orig_size == 0);
			}
		else
			{
			test(h->Top() - orig_top == g);
			test(c.Size() - orig_size == g);
			}
		h->FullCheck();
		if ( ((TLinAddr)orig_top & (align-1)) == 0)
			{
			TAny* free;
			TEST_ALIGN(p2 - RHeap::EAllocCellSize, page_size);
			// will have free space of g-minc
			test(h->TestAlloc(l2 + minc) == p2);
			test(h->Top() - orig_top == 2*g);
			test(c.Size() - orig_size == 2*g);
			free = pass ? h->FreeRef().next->next : h->FreeRef().next;
			test(free != NULL);
			test(h->TestReAlloc(p2, l2 - 4, 0) == p2);
			TInt freeSp = g-minc + (l2+minc - (l2-4));
			TInt adjust = 0;
			if (freeSp >= shrinkThres && freeSp-page_size >= minc)
				{
				// if page_size is less than growBy (g) then heap will be shrunk
				// by less than a whole g.
				adjust = g-((page_size<g)?page_size:0);
				}
			test(h->Top() - orig_top == 2*g - adjust);
			test(c.Size() - orig_size == 2*g - adjust);
			free = pass ? h->FreeRef().next->next : h->FreeRef().next;
			test(free != NULL);
			TEST_ALIGN(TLinAddr(free)+4, page_size);
			test(h->TestAlloc(l2 + g + page_size + 4) == p3 - 4);
			test(h->Top() - orig_top == 4*g - adjust);
			test(c.Size() - orig_size == 4*g - adjust);
			h->TestFree(p3 - 4);
			h->ForceCompress(l2 + g + page_size + 4);
			test(h->Top() - orig_top == g + page_size);
			test(c.Size() - orig_size == g + page_size);
			h->FullCheck();
			h->TestFree(p2);
			h->ForceCompress(l2-4);
			test(h->Compress() == 0);
			// check heap is grown, will have free space of g-minc
			test(h->TestAlloc(l2 + minc) == p2);
			test(h->Top() - orig_top == 2*g);
			test(c.Size() - orig_size == 2*g);
			free = pass ? h->FreeRef().next->next : h->FreeRef().next;
			test(free != NULL);
			// may shrink heap as will now have g+minc free bytes
			test(h->TestReAlloc(p2, l2 - minc, 0) == p2);
			if (g+minc >= shrinkThres)
				{
				test(h->Top() - orig_top == g);
				test(c.Size() - orig_size == g);
				}
			else
				{
				test(h->Top() - orig_top == 2*g);
				test(c.Size() - orig_size == 2*g);
				}
			free = pass ? h->FreeRef().next->next : h->FreeRef().next;
			test(free != NULL);
			TEST_ALIGN(TLinAddr(free)+minc, page_size);
			test(h->TestAlloc(l2 + g + page_size + minc) == p3 - minc);
			test(h->Top() - orig_top == 4*g);
			test(c.Size() - orig_size == 4*g);
			h->TestFree(p3 - minc);
			h->ForceCompress(l2 + g + page_size + minc);
			test(h->Top() - orig_top == g);
			test(c.Size() - orig_size == g);
			h->FullCheck();
			h->TestFree(p2);
			}

		h->TestFree(p1);
		if (pass == 0)
			h->TestFree(p0);
		h->Compress();
		}
	h->FullCheck();
	}

void Test1()
	{
	RHeap* h;
	h = RTestHeap::FixedHeap(0x1000, 0);
	test(h != NULL);
	DoTest1(h);
	h->Close();
	h = RTestHeap::FixedHeap(0x1000, 0, EFalse);
	test(h != NULL);
	DoTest1(h);
	h->Close();
	h = RTestHeap::FixedHeap(0x10000, 64);
	test(h != NULL);
	DoTest1(h);
	h->Close();
	h = RTestHeap::FixedHeap(0x100000, 4096);
	test(h != NULL);
	DoTest1(h);
	h->Close();
	h = RTestHeap::FixedHeap(0x100000, 8192);
	test(h != NULL);
	DoTest1(h);
	h->Close();
	h = UserHeap::ChunkHeap(&KNullDesC(), 0x1000, 0x1000, 0x1000, 4);
	test(h != NULL);
	DoTest1(h);
	h->Close();
	h = UserHeap::ChunkHeap(&KNullDesC(), 0x1000, 0x10000, 0x1000, 4);
	test(h != NULL);
	DoTest1(h);
	h->Close();
	h = UserHeap::ChunkHeap(&KNullDesC(), 0x1000, 0x100000, 0x1000, 4096);
	test(h != NULL);
	DoTest1(h);
	h->Close();
	h = UserHeap::ChunkHeap(&KNullDesC(), 0x1000, 0x100000, 0x1000, 4);
	test(h != NULL);
	DoTest1(h);
	h->Reset();
	DoTest2(h);
	h->Reset();
	DoTest3(h);
	h->Reset();
	DoTest4(h);
	h->Close();
	h = UserHeap::ChunkHeap(&KNullDesC(), 0x1000, 0x100000, 0x1000, 8);
	test(h != NULL);
	DoTest1(h);
	h->Reset();
	DoTest2(h);
	h->Reset();
	DoTest3(h);
	h->Reset();
	DoTest4(h);
	h->Close();
	h = UserHeap::ChunkHeap(&KNullDesC(), 0x1000, 0x100000, 0x1000, 16);
	test(h != NULL);
	DoTest1(h);
	h->Reset();
	DoTest2(h);
	h->Reset();
	DoTest3(h);
	h->Reset();
	DoTest4(h);
	h->Close();
	h = UserHeap::ChunkHeap(&KNullDesC(), 0x1000, 0x100000, 0x1000, 32);
	test(h != NULL);
	DoTest1(h);
	h->Reset();
	DoTest2(h);
	h->Reset();
	DoTest3(h);
	h->Reset();
	DoTest4(h);
	h->Close();
	h = UserHeap::ChunkHeap(&KNullDesC(), 0x3000, 0x100000, 0x3000, 4);
	test(h != NULL);
	DoTest1(h);
	h->Reset();
	DoTest2(h);
	h->Reset();
	DoTest3(h);
	h->Reset();
	DoTest4(h);
	h->Close();
	}

struct SHeapStress
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

	TUint32 Random();
	};

TUint32 SHeapStress::Random()
	{
	iSeed *= 69069;
	iSeed += 41;
	return iSeed;
	}

TInt RandomLength(TUint32 aRandom)
	{
	TUint8 x = (TUint8)aRandom;
	if (x & 0x80)
		return (x & 0x7f) << 7;
	return x & 0x7f;
	}

TInt HeapStress(TAny* aPtr)
	{
	SHeapStress& hs = *(SHeapStress*)aPtr;
	RTestHeap* h = (RTestHeap*)&User::Allocator();
	TUint8* cell[256];
	TInt len[256];

	Mem::FillZ(cell, sizeof(cell));
	Mem::FillZ(len, sizeof(len));

	RThread::Rendezvous(KErrNone);
	while (!hs.iStop)
		{
		// allocate all cells
		TInt i;
		for (i=0; i<256; ++i)
			{
			if (!cell[i])
				{
				++hs.iAllocs;
				cell[i] = (TUint8*)h->TestAlloc(RandomLength(hs.Random()));
				if (cell[i])
					len[i] = h->AllocLen(cell[i]);
				else
					++hs.iFailedAllocs;
				}
			}

		// free some cells
		TInt n = 64 + (hs.Random() & 127);
		while (--n)
			{
			i = hs.Random() & 0xff;
			if (cell[i])
				{
				test(h->AllocLen(cell[i]) == len[i]);
				h->TestFree(cell[i]);
				cell[i] = NULL;
				len[i] = 0;
				++hs.iFrees;
				}
			}

		// realloc some cells
		n = 64 + (hs.Random() & 127);
		while (--n)
			{
			TUint32 rn = hs.Random();
			i = (rn >> 8) & 0xff;
			TInt new_len = RandomLength(rn);
			if (cell[i])
				{
				test(h->AllocLen(cell[i]) == len[i]);
				++hs.iReAllocs;
				TUint8* p = (TUint8*)h->TestReAlloc(cell[i], new_len, rn >> 16);
				if (p)
					{
					cell[i] = p;
					len[i] = h->AllocLen(p);
					}
				else
					++hs.iFailedReAllocs;
				}
			}

		// check the heap
		h->Check();
		++hs.iChecks;
		}
	return 0;
	}

void CreateStressThread(SHeapStress& aInfo)
	{
	Mem::FillZ(&aInfo, _FOFF(SHeapStress, iSeed));
	RThread& t = aInfo.iThread;
	TInt r = t.Create(KNullDesC(), &HeapStress, 0x2000, aInfo.iAllocator, &aInfo);
	test(r==KErrNone);
	t.SetPriority(EPriorityLess);
	TRequestStatus s;
	t.Rendezvous(s);
	test(s == KRequestPending);
	t.Resume();
	User::WaitForRequest(s);
	test(s == KErrNone);
	test(t.ExitType() == EExitPending);
	t.SetPriority(EPriorityMuchLess);
	}

void StopStressThread(SHeapStress& aInfo)
	{
	RThread& t = aInfo.iThread;
	TRequestStatus s;
	t.Logon(s);
	aInfo.iStop = ETrue;
	User::WaitForRequest(s);
	const TDesC& exitCat = t.ExitCategory();
	TInt exitReason = t.ExitReason();
	TInt exitType = t.ExitType();
	test.Printf(_L("Exit type %d,%d,%S\n"), exitType, exitReason, &exitCat);
	test(exitType == EExitKill);
	test(exitReason == KErrNone);
	test(s == KErrNone);
	test.Printf(_L("Total Allocs    : %d\n"), aInfo.iAllocs);
	test.Printf(_L("Failed Allocs   : %d\n"), aInfo.iFailedAllocs);
	test.Printf(_L("Total Frees		: %d\n"), aInfo.iFrees);
	test.Printf(_L("Total ReAllocs  : %d\n"), aInfo.iReAllocs);
	test.Printf(_L("Failed ReAllocs : %d\n"), aInfo.iFailedReAllocs);
	test.Printf(_L("Heap checks     : %d\n"), aInfo.iChecks);
	}

void DoStressTest1(RAllocator* aAllocator)
	{
	RTestHeap* h = (RTestHeap*)aAllocator;
	test.Printf(_L("Test Stress 1: min=%x max=%x align=%d growby=%d\n"),
						h->MinLength(), h->MaxLength(), h->Align(), h->GrowBy());
	SHeapStress hs;
	hs.iSeed = 0xb504f334;
	hs.iAllocator = aAllocator;
	CreateStressThread(hs);
	User::After(10*1000000);
	StopStressThread(hs);
	CLOSE_AND_WAIT(hs.iThread);
	h->FullCheck();
	}

void DoStressTest2(RAllocator* aAllocator)
	{
	RTestHeap* h = (RTestHeap*)aAllocator;
	test.Printf(_L("Test Stress 2: min=%x max=%x align=%d growby=%d\n"),
						h->MinLength(), h->MaxLength(), h->Align(), h->GrowBy());
	SHeapStress hs1;
	SHeapStress hs2;
	hs1.iSeed = 0xb504f334;
	hs1.iAllocator = aAllocator;
	hs2.iSeed = 0xddb3d743;
	hs2.iAllocator = aAllocator;
	CreateStressThread(hs1);
	CreateStressThread(hs2);
	User::After(20*1000000);
	StopStressThread(hs1);
	StopStressThread(hs2);
	CLOSE_AND_WAIT(hs1.iThread);
	CLOSE_AND_WAIT(hs2.iThread);
	h->FullCheck();
	}

void StressTests()
	{
	RHeap* h;
	h = UserHeap::ChunkHeap(&KNullDesC(), 0x1000, 0x100000, 0x1000, 4);
	test(h != NULL);
	DoStressTest1(h);
	h->Reset();
	DoStressTest2(h);
	h->Close();
	h = UserHeap::ChunkHeap(&KNullDesC(), 0x1000, 0x100000, 0x1000, 8);
	test(h != NULL);
	DoStressTest1(h);
	h->Reset();
	DoStressTest2(h);
	h->Close();
	}
		
TInt TestHeapGrowInPlace(TInt aMode)
    {
    TBool reAllocs=EFalse;
    TBool heapGrew=EFalse;
    
    RHeap* myHeap;
    
    myHeap = UserHeap::ChunkHeap(NULL,0x1000,0x4000,0x1000);
    
    TAny *testBuffer,*testBuffer2;
    // Start size chosen so that 1st realloc will use up exactly all the heap.
    // Later iterations wont, and there will be a free cell at the end of the heap.
    TInt currentSize = ((0x800) - sizeof(RHeap)) - RHeap::EAllocCellSize;
    TInt growBy = 0x800;
    TInt newSpace, space;
    
    testBuffer2 = myHeap->Alloc(currentSize);

    newSpace = myHeap->Size();
    do 
    {
    	space = newSpace;
		testBuffer = testBuffer2;
	    currentSize+=growBy;
		testBuffer2 = myHeap->ReAlloc(testBuffer,currentSize,aMode);	
		
		newSpace = myHeap->Size();
		
		if (testBuffer2) 
			{
				
			if (testBuffer!=testBuffer2)
					reAllocs = ETrue;
				
			if (newSpace>space)
					heapGrew = ETrue;
			}
		growBy-=16;
 	} while (testBuffer2);
    currentSize-=growBy;	
    
    myHeap->Free(testBuffer);
    myHeap->Close();
    
    // How did we do?
    if (reAllocs) 
    	{
    	test.Printf(_L("Failure - Memory was moved!\n"));
    	return -100;
    	}
    if (!heapGrew) 
    	{
    	test.Printf(_L("Failure - Heap Never Grew!\n"));
    	return -200;
    	}
    if (currentSize<= 0x3000) 
    	{
    	test.Printf(_L("Failed to grow by a reasonable amount!\n"));
    	return -300;
    	}
        
    return KErrNone;
    }
    
void ReAllocTests()
	{
	test.Next(_L("Testing Grow In Place"));
	test(TestHeapGrowInPlace(0)==KErrNone);
    test(TestHeapGrowInPlace(RHeap::ENeverMove)==KErrNone);
	}

RHeap* TestDEF078391Heap = 0;

TInt TestDEF078391ThreadFunction(TAny*)
	{
    TestDEF078391Heap = UserHeap::ChunkHeap(NULL,0x1000,0x100000,KMinHeapGrowBy,0,EFalse);
	return TestDEF078391Heap ? KErrNone : KErrGeneral;
	}

void TestDEF078391()
	{
	// Test that creating a multithreaded heap with UserHeap::ChunkHeap
	// doesn't create any reference counts on the creating thread.
	// This is done by creating a heap in a named thread, then exiting
	// the thread and re-creating it with the same name.
	// This will fail with KErrAlreadyExists if the orinal thread has
	// not died because of an unclosed reference count.
	test.Next(_L("Test that creating a multithreaded heap doesn't open references of creator"));
	_LIT(KThreadName,"ThreadName");
	RThread t;
	TInt r=t.Create(KThreadName,TestDEF078391ThreadFunction,0x1000,0x1000,0x100000,NULL);
	test(r==KErrNone);
	TRequestStatus status;
	t.Logon(status);
	t.Resume();
	User::WaitForRequest(status);
	test(status==KErrNone);
	test(t.ExitType()==EExitKill);
	test(t.ExitReason()==KErrNone);
	CLOSE_AND_WAIT(t);
	test(TestDEF078391Heap!=0);
	User::After(1000000); // give more opportunity for thread cleanup to happen

	// create thread a second time
	r=t.Create(KThreadName,TestDEF078391ThreadFunction,0x1000,0x1000,0x100000,NULL);
	test(r==KErrNone);
	t.Kill(0);
	CLOSE_AND_WAIT(t);

	// close the heap that got created earlier
	TestDEF078391Heap->Close();
	}

TInt E32Main()
	{
	test.Title();
	__KHEAP_MARK;
	test.Start(_L("Testing heaps"));
	TestDEF078391();
	Test1();
	StressTests();
	ReAllocTests();
	test.End();
	__KHEAP_MARKEND;
	return 0;
	}
