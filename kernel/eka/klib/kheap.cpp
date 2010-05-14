// Copyright (c) 1994-2009 Nokia Corporation and/or its subsidiary(-ies).
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
// e32\klib\kheap.cpp
// 
//

#include <kernel/kern_priv.h>

_LIT(KLitKernHeap,"KernHeap");

RHeapK::RHeapK(TInt aInitialSize)
	: RHybridHeap(aInitialSize, 0, EFalse)			
	{
	}

TInt RHeapK::Compress()
	{
	return 0;
	}

void RHeapK::Reset()
	{
	Fault(KErrNotSupported);
	}

TInt RHeapK::AllocSize(TInt& aTotalAllocSize) const
	{
	(void)aTotalAllocSize;
	Fault(KErrNotSupported);
	return 0;
	}

TInt RHeapK::Available(TInt& aBiggestBlock) const
	{
	(void)aBiggestBlock;
	Fault(KErrNotSupported);
	return 0;
	}

TInt RHeapK::CreateMutex()
	{
	DMutex*& m = *(DMutex**)&iLock;
	return K::MutexCreate(m, KLitKernHeap, NULL, EFalse, KMutexOrdKernelHeap);
	}

RHeapK* RHeapK::FixedHeap(TAny* aBase, TInt aInitialSize)
//
// Create a kernel fixed heap.
//
	{
	__ASSERT_ALWAYS(aInitialSize>(TInt)sizeof(RHeapK), K::Fault(K::ETHeapMaxLengthNegative));
	return new(aBase) RHeapK(aInitialSize);
	}

void RHeapK::CheckThreadState()
//
// Check that the kernel is not locked and the thread is unkillable
//
	{
	if (K::Initialising)
		return;
	__NK_ASSERT_UNLOCKED;
	__ASSERT_NO_FAST_MUTEX;
	__ASSERT_CRITICAL;
	}

void RHybridHeap::Lock() const   
	{
	DMutex* m = *(DMutex**)&iLock;
	if (m)
		Kern::MutexWait(*m);
	}

void RHybridHeap::Unlock() const   
	{
	DMutex* m = *(DMutex**)&iLock;
	if (m)
		Kern::MutexSignal(*m);
	}

void RHeapK::Fault(TInt aFault)
	{
	Kern::Fault("KERN-HEAP", aFault);
	}

#if defined(__HEAP_MACHINE_CODED__) && !defined(_DEBUG)
GLDEF_C void RHeapK_PanicBadAllocatedCellSize()
	{
	K::Fault(K::EKHeapBadAllocatedCellSize);
	}

GLDEF_C void RHeapK_PanicBadNextCell()
	{
	K::Fault(K::EKHeapFreeBadNextCell);
	}

GLDEF_C void RHeapK_PanicBadPrevCell()
	{
	K::Fault(K::EKHeapFreeBadPrevCell);
	}

GLDEF_C void RHeapK_PanicBadCellAddress()
	{
	K::Fault(K::EKHeapBadCellAddress);
	}
#endif

