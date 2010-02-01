// Copyright (c) 2006-2009 Nokia Corporation and/or its subsidiary(-ies).
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
// e32test\defrag\d_pagemove.cpp
// LDD for testing defrag page moving
// 
//

#include <kernel/kern_priv.h>
#include "platform.h"
#include "d_pagemove.h"
#include "nk_priv.h"

// debug tracing for this test driver is very noisy - off by default
#undef DEBUG_PAGEMOVE
#ifdef DEBUG_PAGEMOVE
#define DBG(a) a
#else
#define DBG(a)
#endif

const TInt KArbitraryNumber = 4;

// This driver is ram loaded (see mmp file) so this function will do fine
// as a test of RAM-loaded code.
TInt RamLoadedFunction()
	{
	return KArbitraryNumber;
	}

const TInt KMajorVersionNumber=0;
const TInt KMinorVersionNumber=1;
const TInt KBuildVersionNumber=1;

class DPageMove;

class DPageMoveFactory : public DLogicalDevice
//
// Page move LDD factory
//
	{
public:
	DPageMoveFactory();
	virtual TInt Install();						//overriding pure virtual
	virtual void GetCaps(TDes8& aDes) const;	//overriding pure virtual
	virtual TInt Create(DLogicalChannelBase*& aChannel);	//overriding pure virtual
	};

class DPageMove : public DLogicalChannelBase
//
// Page move logical channel
//
	{
public:
	DPageMove();
	~DPageMove();
protected:
	virtual TInt DoCreate(TInt aUnit, const TDesC8* anInfo, const TVersion& aVer);
	virtual TInt Request(TInt aFunction, TAny* a1, TAny* a2);
	TInt DoPageMove(TLinAddr aAddr, TBool aEchoOff=EFalse);
	TInt KernelDataMovePerformance(void);
	TInt iPageSize;
	};

DECLARE_STANDARD_LDD()
	{
    return new DPageMoveFactory;
    }

DPageMoveFactory::DPageMoveFactory()
//
// Constructor
//
    {
    iVersion=TVersion(KMajorVersionNumber,KMinorVersionNumber,KBuildVersionNumber);
    //iParseMask=0;//No units, no info, no PDD
    //iUnitsMask=0;//Only one thing
    }

TInt DPageMoveFactory::Create(DLogicalChannelBase*& aChannel)
//
// Create a new DPageMove on this logical device
//
    {
	aChannel=new DPageMove;
	return aChannel?KErrNone:KErrNoMemory;
    }

TInt DPageMoveFactory::Install()
//
// Install the LDD - overriding pure virtual
//
    {
    return SetName(&KPageMoveLddName);
    }

void DPageMoveFactory::GetCaps(TDes8& aDes) const
//
// Get capabilities - overriding pure virtual
//
    {
    TCapsPageMoveV01 b;
    b.iVersion=TVersion(KMajorVersionNumber,KMinorVersionNumber,KBuildVersionNumber);
    Kern::InfoCopy(aDes,(TUint8*)&b,sizeof(b));
    }

DPageMove::DPageMove()
//
// Constructor
//
    {
    }

TInt DPageMove::DoCreate(TInt /*aUnit*/, const TDesC8* /*anInfo*/, const TVersion& aVer)
//
// Create channel
//
    {
    if (!Kern::QueryVersionSupported(TVersion(KMajorVersionNumber,KMinorVersionNumber,KBuildVersionNumber),aVer))
    	return KErrNotSupported;
	iPageSize=Kern::RoundToPageSize(1);
	return KErrNone;
	}

DPageMove::~DPageMove()
//
// Destructor
//
    {
    }

TInt DPageMove::Request(TInt aFunction, TAny* a1, TAny* a2)
	{
	TInt r=KErrNone;
	DBG(Kern::Printf("DPageMove::Request func=%d a1=%08x a2=%08x", aFunction, a1, a2));
	NKern::ThreadEnterCS();
	switch (aFunction)
		{
		case RPageMove::EControlTryMovingKHeap:
			// Allocate a large array on the kernel heap and try moving it around.
			{
			const TInt size=16384;
			TUint8* array = new TUint8[size];
			if (array == NULL)
				r=KErrNoMemory;
			else
				{
				for (TInt i=0; i<size; i++) array[i] = i*i;

				TUint8* firstpage=(TUint8*)_ALIGN_DOWN((TLinAddr)array, iPageSize);
				for (TUint8* page=firstpage; page<array+size; page+=iPageSize)
					{
					r=DoPageMove((TLinAddr)page);
					if (r!=KErrNone)
						{
						Kern::Printf("Move returned %d", r);
						break;
						}
					}

				if (r==KErrNone)
					{
					for (TInt i=0; i<size; i++)
						{
						if (array[i] != (TUint8)(i*i))
							{
							r=KErrGeneral;
							Kern::Printf("Data differs at index %d address %08x, expected %02x got %02x", i, &array[i], (TUint8)(i*i), array[i]);
							}
						}
					}

				Kern::ValidateHeap();

				delete [] array;
				}
			}
			break;

		case RPageMove::EControlTryMovingKStack:
			// Stick a not-too-large array on the current thread's kernel stack and try moving it around.
			{
			const TInt size=1024;
			TUint8 array[size];
			for (TInt i=0; i<size; i++) array[i] = i*i;

			TUint8* firstpage=(TUint8*)_ALIGN_DOWN((TLinAddr)array, iPageSize);
			for (TUint8* page=firstpage; page<array+size; page+=iPageSize)
				{
				r=DoPageMove((TLinAddr)page);
				if (r!=KErrNone)
					{
					Kern::Printf("Move returned %d", r);
					break;
					}
				}

			if (r==KErrNone)
				{
				for (TInt i=0; i<size; i++)
					{
					if (array[i] != (TUint8)(i*i))
						{
						r=KErrGeneral;
						Kern::Printf("Data differs at index %d address %08x, expected %02x got %02x", i, &array[i], (TUint8)(i*i), array[i]);
						}
					}
				}
			}
			break;

		case RPageMove::EControlTryMovingUserPage:
		case RPageMove::EControlTryMovingLocale:
			// Try moving the page that the user part of the test told us to.
			r=DoPageMove((TLinAddr)a1, (TBool)a2);
			if (r!=KErrNone && !a2)
				Kern::Printf("Move returned %d", r);
			break;

		case RPageMove::EControlTryMovingKCode:
			{
			r=DoPageMove((TLinAddr)&RamLoadedFunction);
			if (r==KErrNone)
				{
				if (RamLoadedFunction()!=KArbitraryNumber)
					r=KErrGeneral;
				}	
			else
				Kern::Printf("Move returned %d", r);
			}
			break;

		case RPageMove::EControlPerfMovingKData:
			r = KernelDataMovePerformance();
			break;

		case RPageMove::EControlGetPhysAddr:
			TPhysAddr addr;
			addr = (TUint)Epoc::LinearToPhysical((TLinAddr)a1);
			Kern::ThreadRawWrite(&Kern::CurrentThread(),a2, &addr, sizeof(TPhysAddr));
			break;

		case RPageMove::EControlTryMovingPhysAddr:
			{
			TPhysAddr newAddr;
			r = Epoc::MovePhysicalPage((TPhysAddr)a1, newAddr);
			Kern::ThreadRawWrite(&Kern::CurrentThread(),a2, &newAddr, sizeof(TPhysAddr));
			break;
			}

		case RPageMove::EControlTryMovingPageTable:
			{
			TPhysAddr newAddr;
			r = Epoc::MovePhysicalPage((TPhysAddr) a1, newAddr, Epoc::ERamDefragPage_PageTable);
			if (newAddr != KPhysAddrInvalid)
				r = KErrGeneral;
			break;
			}

		case RPageMove::EControlTryMovingPageTableInfo:
			{
			TPhysAddr newAddr;
			r = Epoc::MovePhysicalPage((TPhysAddr) a1, newAddr, Epoc::ERamDefragPage_PageTableInfo);
			if (newAddr != KPhysAddrInvalid)
				r = KErrGeneral;
			break;
			}

		case RPageMove::EControlNumberOfCpus:
			r = NKern::NumberOfCpus();
			break;
		default:
			r=KErrNotSupported;
			break;
		}
	NKern::ThreadLeaveCS();
	if (r!=KErrNone)
		DBG(Kern::Printf("DPageMove::Request returns %d", r));
	return r;
	}

TInt DPageMove::DoPageMove(TLinAddr aAddr, TBool aEchoOff)
	{
	DBG(Kern::Printf("DPageMove::DoPageMove() addr=%08x",aAddr));
	aAddr = _ALIGN_DOWN(aAddr, iPageSize);

	TPhysAddr aOld = Epoc::LinearToPhysical(aAddr);
	TInt r;
	if (aOld == KPhysAddrInvalid)
		r=KErrArgument;
	else
		{
		TPhysAddr aNew;
		r=Epoc::MovePhysicalPage(aOld, aNew);
		if (r==KErrNone)
			{
			TPhysAddr aNewCheck = Epoc::LinearToPhysical(aAddr);
			if (aNewCheck != aNew)
				{
				if (!aEchoOff)
					Kern::Printf("Address mismatch: expected %08x actual %08x\n",aNew,aNewCheck);
				if (aNew != KPhysAddrInvalid && aNewCheck != KPhysAddrInvalid)
					{// The page was not paged out by the moving so don't allow 
					// addresses to differ.  If is was paged out then it may have 
					// been paged back in again but to any free page so the addresses will differ.
					r=KErrGeneral;
					}
				}
			}
		}
	if (r!=KErrNone)
		DBG(Kern::Printf("DPageMove::DoPageMove() returns %d", r));
	return r;
	}


#ifndef __MSVC6__ 	// VC6 can't cope with variable arguments in macros.
#define KERN_PRINTF(x...) Kern::Printf(x)
#endif

//#define EXTRA_TRACE
#ifdef EXTRA_TRACE
#define PRINTF(x)	x
#else
#define PRINTF(x)
#endif


TInt DPageMove::KernelDataMovePerformance(void)
{
	const TInt KHeapPagesToMove = 2000;
	const TInt KMoveAttempts = 50;
	const TInt KStackSize=1024;
	enum TKMoveMode
		{
		EKMoveHeap,
		EKMoveStack,
		EKMoveModes,
		};

	TInt r = KErrNone;

	// Create some kernel stack pages
	TUint8 stackArray[KStackSize];

	/// Create some kernel heap pages
	TUint actualHeapPages = KHeapPagesToMove;
	TInt heapArraySize = iPageSize * KHeapPagesToMove;
	TUint8* heapArray = new TUint8[heapArraySize];
	if (heapArray == NULL)
		return KErrNoMemory;
	
	TInt i = 0;
	for (; i < heapArraySize; i++)
		{
		heapArray[i] = i;
		}

	PRINTF(KERN_PRINTF("Testing Performance of Moving Kernel Data Pages"));

	TInt moveMode = EKMoveStack;
	for (; moveMode < EKMoveModes; moveMode++)
		{
		TLinAddr pageAddr = NULL;
		TLinAddr endAddr = NULL;
		TLinAddr baseAddr = NULL;
		switch (moveMode)
			{
			case EKMoveHeap:
				pageAddr = _ALIGN_DOWN((TLinAddr)heapArray, iPageSize);
				baseAddr = pageAddr;
				endAddr = _ALIGN_UP((TLinAddr)heapArray + heapArraySize, iPageSize);
				actualHeapPages = (endAddr - baseAddr) / iPageSize;
				PRINTF(KERN_PRINTF("heap baseAddr %x endAddr %x", baseAddr, endAddr));
				break;

			case EKMoveStack:
				pageAddr = _ALIGN_DOWN((TLinAddr)stackArray, iPageSize);
				baseAddr = pageAddr;
				endAddr = _ALIGN_UP((TLinAddr)stackArray + KStackSize, iPageSize);
				PRINTF(KERN_PRINTF("stack baseAddr %x endAddr %x", baseAddr, endAddr));
				break;
			}

		TUint32 minTime = KMaxTUint32;
		TUint32 maxTime = 0; 
		TUint32 cummulative = 0;
		TInt iterations = KMoveAttempts;
		TInt tot = iterations;
		TUint pagesMoved = (endAddr - baseAddr) / iPageSize;
		while (iterations--)
			{
			TUint32 diff;
			TUint32 startTime=0;
			TUint32 endTime=0;
			switch (moveMode)
				{
				case EKMoveHeap:
					startTime = NKern::FastCounter(); 
				
					while (pageAddr < endAddr) 
						{
						r = DoPageMove(pageAddr);

						if (r != KErrNone)
							{
							goto exit;
							}
						pageAddr += iPageSize;
						}
					endTime = NKern::FastCounter();
					break;
			
				case EKMoveStack:
					// Normally will move same number of pages as heap to make comparison easier
					TUint i = actualHeapPages;
					startTime = NKern::FastCounter();
					for (; i > 0; i--)
						{
						while (pageAddr < endAddr) 
							{
							r = DoPageMove(pageAddr);

							if (r != KErrNone)
								{
								goto exit;
								}
							pageAddr += iPageSize;
							}
						pageAddr = baseAddr;
						}
					endTime = NKern::FastCounter();
					break;
				}
			diff = endTime - startTime;
			if (endTime < startTime) 
				{
				Kern::Printf("WARNING - fast counter overflowed. Assuming only once and continuing");
				diff = (KMaxTUint32 - startTime) + endTime;
				}

			if (diff == 0)
				{
				Kern::Printf("difference 0!");
				tot--;
				}

			if (diff > maxTime)
				maxTime = diff;
			if (diff < minTime)
				minTime = diff;

			if (cummulative + diff < cummulative)
				{
				Kern::Printf("OverFlow!!!");
				r = KErrOverflow;
				goto exit;
				}
			pageAddr = baseAddr;
			cummulative += diff;
			}
		switch (moveMode)
			{
			case EKMoveHeap:				
				if (tot != 0)
					{
					TUint average = (cummulative / tot);
					Kern::Printf("Fast counter ticks to move %d kernel heap pages: Av %d Min %d Max %d (non zero iterations = %d out of %d)", pagesMoved, average, minTime, maxTime, tot, KMoveAttempts);
					Kern::Printf("Average of %d ticks to move one page\n", average / pagesMoved);
					}
				else
					Kern::Printf("WARNING - all kernel heap page moves took 0 fast counter ticks");
				break;
			
			case EKMoveStack:
				if (tot != 0)
					{
					TUint average = (cummulative / tot);
					Kern::Printf("Fast counter ticks to move %d kernel stack pages %d times: Av %d Min %d Max %d (non zero iterations = %d out of %d)", pagesMoved, actualHeapPages, average, minTime, maxTime, tot, KMoveAttempts);
					Kern::Printf("Average of %d ticks to move one page\n", average / (pagesMoved * actualHeapPages));
					}
				else
					Kern::Printf("WARNING - all kernel stack page moves took 0 fast counter ticks");
				break;
			}
		}

	r = KErrNone;
exit:
	delete [] heapArray;
	return r;
	}
	

