// Copyright (c) 2005-2009 Nokia Corporation and/or its subsidiary(-ies).
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
// x86pc\nkern\tlsf.cpp
// 
//

#include <nktest/nkutils.h>

extern "C" {
extern TLinAddr RomHeaderAddress;
extern TLinAddr SuperPageAddress;
}

#define	BLOCK_STATUS_FREE	1u	// bit set if SBlock free
#define	BLOCK_STATUS_FINAL	2u	// bit set if this is last physical SBlock
#define BLOCK_SIZE_MASK		0xfffffffcu

struct SBlock
	{
	TUint32 size;					// bits 2-31 give size, bits 0,1 are status bits
	SBlock* predecessor;		// previous SBlock in physical address order
	};

struct SFreeBlock
	{
	SBlock b;
	SFreeBlock* next;			// next SBlock in same bucket, circular list
	SFreeBlock* prev;			// previous SBlock in same bucket, circular list
	};

struct STlsfAllocator
	{
public:
	static STlsfAllocator* Create(TAny* aBase, TUint32 aTotalSize);
	TAny* Alloc(TUint32 aSize);
	void Free(TAny* aCell);
	TAny* ReAlloc(TAny* aCell, TUint32 aSize);

	void Init(TUint32 aTotalSize);
	void InsertFree(SFreeBlock* aB);
	TUint32 MapSize(TUint32 aSize, TUint32* p_sli, TInt round_up);
	SFreeBlock* FindFree(TUint32 aSize);
	void RemoveFree(SFreeBlock* aB);
	TUint32 ConsistencyCheck();
public:
	TUint32	imin_size;			// minimum SBlock size
	TUint32 itotal_size;		// total size of allocated and free blocks
	TUint32 il2min;				// log2 min size
	TUint32 infl;				// number of first level lists
	TUint32 insl;				// number of second level lists
	TUint32 il2nsl;
	SFreeBlock** isll;			// pointer to first second level list pointer for minimum size
	TUint32 iflb;				// first level bitmap
	SBlock* iff;				// first free address
	TUint32 islb[1];			// second level bitmaps, one for each first level list
								// second level lists follow, nsl pointers for each first level list
	};

STlsfAllocator* TheAllocator;
NFastMutex AllocMutex;

STlsfAllocator* STlsfAllocator::Create(void* aBase, TUint32 aTotalSize)
	{
	STlsfAllocator* p = (STlsfAllocator*)aBase;
	p->Init(aTotalSize);
	return p;
	}

void STlsfAllocator::Init(TUint32 total_size)
	{
	TUint32 a;
	imin_size = 16;
	il2min = 4;
	insl = 16;
	il2nsl = 4;
	itotal_size = total_size;
	infl = __e32_find_ms1_32(total_size) - il2min + 1;
	iflb = 0;
	a = (TUint32)&islb[infl];
	a = (a+63)&~63;
	isll = (SFreeBlock**)a;
	a += insl * infl * sizeof(TUint32*);
	iff = (SBlock*)a;
	a -= (TUint32)this;
	memset(islb, 0, total_size - sizeof(STlsfAllocator) + sizeof(TUint32));
	iff->size = (total_size - a) | BLOCK_STATUS_FINAL;
	iff->predecessor = 0;
	Free(iff+1);
	}

// size already rounded up to multiple of min_size
TUint32 STlsfAllocator::MapSize(TUint32 size, TUint32* p_sli, TInt round_up)
	{
	TUint32 sli = size >> il2min;
	TUint32 fli = __e32_find_ms1_32(sli);
	sli -= (1u<<fli);
	if (fli > il2nsl)
		{
		TUint32 sls = (fli - il2nsl);
		TUint32 sz2 = sli;
		sli >>= sls;
		if (round_up && ((sli << sls) < sz2) )
			{
			if (++sli == insl)
				sli=0, ++fli;
			}
		}
	*p_sli = sli;
	return fli;
	}

SFreeBlock* STlsfAllocator::FindFree(TUint32 size)
	{
	TUint32 sli;
	TUint32 fli = MapSize(size, &sli, 1);
	TUint32 sli2;
	TUint32 fli2;
	SFreeBlock** sll;
	SFreeBlock* b;
	TUint32 act_sz;

	if ((iflb >> fli) & 1)
		{
		if ((islb[fli]>>sli)==0)
			++fli, sli=0;
		}
	else
		sli = 0;
	if (fli >= infl)
		return 0;
	fli2 = __e32_find_ls1_32(iflb >> fli);
	if ((TInt)fli2 < 0)
		return 0;
	fli2 += fli;
	sli2 = __e32_find_ls1_32(islb[fli2] >> sli) + sli;	// must find a 1
	sll = &isll[(fli2 << il2nsl) | sli2];
	b = *sll;
	if (b->next == b)
		{
		// list now empty
		*sll = 0;
		if ( (islb[fli2] &= ~(1u<<sli2)) == 0 )
			iflb &= ~(1u<<fli2);
		}
	else
		{
		*sll = b->next;
		b->next->prev = b->prev;
		b->prev->next = b->next;
		}
	act_sz = b->b.size & BLOCK_SIZE_MASK;
	if (act_sz > size)
		{
		// free the extra
		SBlock* nfb = (SBlock*)((TUint32)b + size);
		nfb->size = (act_sz - size) | (b->b.size & BLOCK_STATUS_FINAL);
		nfb->predecessor = &b->b;
		if (!(b->b.size & BLOCK_STATUS_FINAL))
			{
			// if this isn't final SBlock, update predecessor of following SBlock
			SBlock* nb = (SBlock*)((TUint32)b + act_sz);
			nb->predecessor = nfb;
			}
		InsertFree((SFreeBlock*)nfb);
		b->b.size = 0;	// allocated SBlock can't be final
		}
	b->b.size &= BLOCK_STATUS_FINAL;
	b->b.size |= size;
	return b;
	}

void STlsfAllocator::InsertFree(SFreeBlock* b)
	{
	TUint32 size = b->b.size & BLOCK_SIZE_MASK;
	TUint32 sli;
	TUint32 fli = MapSize(size, &sli, 0);
	SFreeBlock** sll = &isll[(fli << il2nsl) | sli];
	if (*sll)
		{
		SFreeBlock* first = *sll;
		b->next = first;
		b->prev = first->prev;
		first->prev->next = b;
		first->prev = b;
		}
	else
		{
		b->next = b;
		b->prev = b;
		islb[fli] |= (1u<<sli);
		iflb |= (1u<<fli);
		}
	*sll = b;
	b->b.size |= BLOCK_STATUS_FREE;
	}

void STlsfAllocator::RemoveFree(SFreeBlock* b)
	{
	TUint32 size = b->b.size & BLOCK_SIZE_MASK;
	TUint32 sli;
	TUint32 fli = MapSize(size, &sli, 0);
	SFreeBlock** sll = &isll[(fli << il2nsl) | sli];
	if (b->next != b)
		{
		if (*sll == b)
			*sll = b->next;
		b->next->prev = b->prev;
		b->prev->next = b->next;
		return;
		}
	*sll = 0;
	if ( (islb[fli] &= ~(1u<<sli)) == 0)
		iflb &= ~(1u<<fli);
	}

TAny* STlsfAllocator::Alloc(TUint32 size)
	{
	SFreeBlock* b;
	TUint32 msm = imin_size - 1;

	size = (size + sizeof(SBlock) + msm) &~ msm;

	b = FindFree(size);
	if (b)
		return &b->next;
	return 0;
	}

void STlsfAllocator::Free(TAny* cell)
	{
	SBlock* b = ((SBlock*)cell) - 1;
//	SFreeBlock* fb = (SFreeBlock*)b;
	TUint32 size;
	if (!cell)
		return;
	size = b->size & BLOCK_SIZE_MASK;
	if (!(b->size & BLOCK_STATUS_FINAL))
		{
		// not last SBlock
		SBlock* nb = (SBlock*)((TUint32)b + size);
		TUint32 nbs = nb->size;
		if (nbs & BLOCK_STATUS_FREE)
			{
			// following SBlock is free
			RemoveFree((SFreeBlock*)nb);
			b->size = size + (nbs &~ BLOCK_STATUS_FREE);	// keeps final flag from following SBlock
			size = b->size & BLOCK_SIZE_MASK;
			if (!(nbs & BLOCK_STATUS_FINAL))
				{
				nb = (SBlock*)((TUint32)b + size);
				nb->predecessor = b;
				}
			}
		}
	if (b->predecessor && b->predecessor->size & BLOCK_STATUS_FREE)
		{
		// predecessor SBlock is free
		TUint32 psz = b->predecessor->size & BLOCK_SIZE_MASK;
		RemoveFree((SFreeBlock*)b->predecessor);
		psz += b->size; // keeps final flag if necessary
		b->predecessor->size = psz;
		b = b->predecessor;
		if (!(psz & BLOCK_STATUS_FINAL))
			{
			// need to adjust prev pointer of following SBlock
			SBlock* nb = (SBlock*)((TUint32)b + psz);
			nb->predecessor = b;
			}
		}
	InsertFree((SFreeBlock*)b);
	}

TAny* STlsfAllocator::ReAlloc(TAny* cell, TUint32 newsize)
	{
	SBlock* b;
	TUint32 size;
	TUint32 msm;
	SBlock* nb;
	TAny* newcell;

	if (!cell)
		return (newsize>0) ? Alloc(newsize) : 0;
	if (newsize == 0)
		{
		Free(cell);
		return 0;
		}
	b = ((SBlock*)cell) - 1;
	size = b->size & BLOCK_SIZE_MASK;
	msm = imin_size - 1;
	newsize = (newsize + sizeof(SBlock) + msm) &~ msm;
	if (newsize > size)
		{
		nb = (SBlock*)((TUint32)b + size);
		if (nb->size & BLOCK_STATUS_FREE)
			{
			// following SBlock is free
			TUint32 nbs = nb->size;
			if (nbs + size >= newsize)
				{
				// we can expand in place - grab the entire free SBlock for now
				// it will be split if necessary in the next section of code
				RemoveFree((SFreeBlock*)nb);
				b->size = size + (nbs &~ BLOCK_STATUS_FREE);	// keeps final flag from following SBlock
				size = b->size & BLOCK_SIZE_MASK;
				if (!(nbs & BLOCK_STATUS_FINAL))
					{
					SBlock* nnb = (SBlock*)((TUint32)b + size);
					nnb->predecessor = b;
					}
				}
			}
		}
	if (newsize == size)
		return cell;
	if (newsize < size)
		{
		// shrinking - split SBlock
		TUint32 final = b->size & BLOCK_STATUS_FINAL;
		SBlock* nfb = (SBlock*)((TUint32)b + newsize);
		nfb->size = (size - newsize) | final;
		nfb->predecessor = b;
		if (!final)
			{
			// if this isn't final SBlock, update predecessor of following SBlock
			nb = (SBlock*)((TUint32)b + size);
			nb->predecessor = nfb;
			}
		b->size = newsize;	// original SBlock can't be final
		Free((SBlock*)nfb + 1);
		return cell;
		}

	// must move SBlock to expand
	newcell = Alloc(newsize);
	if (newcell)
		{
		memcpy(newcell, cell, size);
		Free(cell);
		}
	return newcell;
	}

TUint32 STlsfAllocator::ConsistencyCheck()
	{
	TUint32 a;
	TUint32 szs = 0;
	TUint32 size;
	TUint32 total_block_size = 0;
	TUint32 block_count = 0;
	TUint32 flb = 0;
	TUint32 slb[32];
	TUint32 sli;
	TUint32 fli;
	TUint32 total_free = 0;
	SBlock* b = iff;
	SBlock* pb = 0;
	TUint32 total_user_size;

	memset(slb, 0, sizeof(slb));
	__NK_ASSERT_ALWAYS(imin_size == 16);
	__NK_ASSERT_ALWAYS(insl == 16);
	__NK_ASSERT_ALWAYS(il2min == 4);
	__NK_ASSERT_ALWAYS(il2nsl == 4);
	__NK_ASSERT_ALWAYS(infl == __e32_find_ms1_32(itotal_size) - il2min + 1);
	a = (TUint32)&islb[infl];
	a = (a+63)&~63;
	__NK_ASSERT_ALWAYS(isll == (SFreeBlock**)a);
	a += insl * infl * sizeof(TUint32*);
	__NK_ASSERT_ALWAYS(iff == (SBlock*)a);
	total_user_size = itotal_size - (a - (TUint32)this);

	do	{
		szs = b->size;
		size = szs & BLOCK_SIZE_MASK;
		__NK_ASSERT_ALWAYS(b->predecessor == pb);
		__NK_ASSERT_ALWAYS(size > 0);
		__NK_ASSERT_ALWAYS(size <= total_user_size);
		__NK_ASSERT_ALWAYS(size == ((size >> il2min) << il2min));
		total_block_size += size;
		++block_count;
		pb = b;
		b = (SBlock*)((TUint32)b + size);
		} while(!(szs & BLOCK_STATUS_FINAL));
	__NK_ASSERT_ALWAYS((TUint32)b == (TUint32)this + itotal_size);
	__NK_ASSERT_ALWAYS(total_block_size == total_user_size);

	b = iff;
	do	{
		szs = b->size;
		size = szs & BLOCK_SIZE_MASK;
		if (szs & BLOCK_STATUS_FREE)
			{
			SFreeBlock* fb = (SFreeBlock*)b;
			TUint32 lhi;
			SFreeBlock* lh;
			SFreeBlock* pfb = fb;
			TInt lh_found = 0;
			TInt c = (TInt)block_count;
			TUint32 fli = __e32_find_ms1_32(size) - il2min;
			TUint32 sli = (size >> il2min) - (1u << fli);
			TUint32 sli2;
			TUint32 fli2 = MapSize(size, &sli2, 0);
			(void)sli2, (void)fli2;
			if (fli > il2nsl)
				sli >>= (fli - il2nsl);
			__NK_ASSERT_ALWAYS(fli == fli2);
			__NK_ASSERT_ALWAYS(sli == sli2);
			flb |= (1u << fli);
			slb[fli] |= (1u << sli);
			lhi = (fli << il2nsl) | sli;
			lh = isll[lhi];
			do	{
				if (fb == lh)
					lh_found = 1;
				pfb = fb;
				fb = fb->next;
				__NK_ASSERT_ALWAYS(fb->prev == pfb);
				__NK_ASSERT_ALWAYS(fb->b.size & BLOCK_STATUS_FREE);
				} while ((fb != (SFreeBlock*)b) && --c>=0);
			__NK_ASSERT_ALWAYS(fb == (SFreeBlock*)b);
			__NK_ASSERT_ALWAYS(lh_found);
			total_free += size;
			}
		b = (SBlock*)((TUint32)b + size);
		} while(!(szs & BLOCK_STATUS_FINAL));

	__NK_ASSERT_ALWAYS(flb == iflb);
	for (fli=0; fli<infl; ++fli)
		{
		__NK_ASSERT_ALWAYS(slb[fli] == islb[fli]);
		if (!(flb & (1u<<fli)))
			__NK_ASSERT_ALWAYS(slb[fli]==0);
		for (sli=0; sli<insl; ++sli)
			{
			TUint32 lhi = (fli << il2nsl) | sli;
			if (!(slb[fli] & (1u<<sli)))
				__NK_ASSERT_ALWAYS(!isll[lhi]);
			}
		}
	return total_free;
	}

#define __E32CMN_H__

#undef EXPORT_C
#define EXPORT_C /* */

class TVersion
	{
public:
	TInt8 iMajor;
	TInt8 iMinor;
	TInt16 iBuild;
	};

class TDesC;

#include <e32rom.h>
#include "kernboot.h"

extern "C" {
void SetupMemoryAllocator()
	{
	const SSuperPageBase& sp = *(const SSuperPageBase*)SuperPageAddress;
	const TRomHeader& romHdr = *(const TRomHeader*)RomHeaderAddress;
	const TRomEntry* primaryEntry = (const TRomEntry*)sp.iPrimaryEntry;
	const TRomImageHeader* primaryImageHeader = (const TRomImageHeader*)primaryEntry->iAddressLin;
	TLinAddr stack = romHdr.iKernDataAddress + round_to_page(romHdr.iTotalSvDataSize);
	TLinAddr heapbase = stack + round_to_page(primaryImageHeader->iStackSize);
	TLinAddr heapsize = sp.iInitialHeapSize;

	KPrintf("Heap base %08x size %08x", heapbase, heapsize);
	TheAllocator = STlsfAllocator::Create((TAny*)heapbase, heapsize);
	}

extern TBool InitialThreadDefined;

TAny* malloc(TUint32 aSize)
	{
//	__KTRACE_OPT(KMMU,DEBUGPRINT("malloc(%d)",aSize));
	if (InitialThreadDefined)
		NKern::FMWait(&AllocMutex);
	TAny* p = TheAllocator->Alloc(aSize);
	if (InitialThreadDefined)
		NKern::FMSignal(&AllocMutex);
	__KTRACE_OPT(KMMU,DEBUGPRINT("malloc(%d)->%08x",aSize,p));
	return p;
	}

void free(TAny* aCell)
	{
	__KTRACE_OPT(KMMU,DEBUGPRINT("free(%08x)",aCell));
#ifdef _DEBUG
	if (aCell)
		{
		SBlock* b = (SBlock*)aCell;
		TUint32 size = b[-1].size & BLOCK_SIZE_MASK;
		memset(aCell, 0xde, size - sizeof(SBlock));
		}
#endif
	NKern::FMWait(&AllocMutex);
	TheAllocator->Free(aCell);
	NKern::FMSignal(&AllocMutex);
	}

TAny* realloc(TAny* aCell, TUint32 aSize)
	{
	NKern::FMWait(&AllocMutex);
	TAny* p = TheAllocator->ReAlloc(aCell, aSize);
	NKern::FMSignal(&AllocMutex);
	return p;
	}
}

