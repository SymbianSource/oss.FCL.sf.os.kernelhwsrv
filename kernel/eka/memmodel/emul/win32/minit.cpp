// Copyright (c) 1996-2009 Nokia Corporation and/or its subsidiary(-ies).
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
// e32\memmodel\emul\win32\minit.cpp
// 
//

#include "memmodel.h"
#include <emulator.h>
#include <property.h>

_LIT8(KKernelFullPathName,"EKern.exe");
_LIT8(KLitRamAlloc,"RamAlloc");

void M::Init1()
	{
	__KTRACE_OPT2(KBOOT,KMMU,Kern::Printf("M::Init1"));
	
	SAddressInfo ainfo;
	Emul::TheAsic->AddressInfo(ainfo);

	MM::RamChunkSize = KRamChunkSize;
	MM::RamChunkShift = KRamChunkShift;
	MM::RamPageSize = KRamPageSize;
	MM::RamPageShift = KRamPageShift;
	MM::FreeMemory = ainfo.iTotalRamSize;
	TheSuperPage().iTotalRamSize = ainfo.iTotalRamSize;
	K::MaxFreeRam = ainfo.iTotalRamSize;
	PP::RamDriveMaxSize = ainfo.iRamDriveMaxSize;

	K::SupervisorThreadStackSize=0x1000;	// 4K

	K::MachineConfig=(TMachineConfig*)A::MachineConfiguration().Ptr();

	K::MaxMemCopyInOneGo=KDefaultMaxMemCopyInOneGo;
	__KTRACE_OPT(KBOOT,Kern::Printf("K::MaxMemCopyInOneGo=0x%x",K::MaxMemCopyInOneGo));

	LPVOID kheap = VirtualAlloc(NULL, KKernelHeapSizeMax, MEM_RESERVE, PAGE_READWRITE);
	__ASSERT_ALWAYS(kheap, MM::Panic(MM::EKernelHeapReserveFailed));
	kheap = VirtualAlloc(kheap, KKernelHeapSizeMin, MEM_COMMIT, PAGE_READWRITE);
	__ASSERT_ALWAYS(kheap, MM::Panic(MM::EKernelHeapCommitFailed));
	MM::KernelHeapAddress = kheap;
	K::MemModelAttributes = (TUint32)(EMemModelTypeEmul|EMemModelAttrNonExProt|EMemModelAttrWriteProt|EMemModelAttrVA);
	
	MM::Init1();
	}

void M::Init2()
	{
	__KTRACE_OPT2(KBOOT,KMMU,Kern::Printf("M::Init2"));
	TInt r=K::MutexCreate((DMutex*&)MM::RamAllocatorMutex, KLitRamAlloc, NULL, EFalse, KMutexOrdRamAlloc);
	__ASSERT_ALWAYS(r==KErrNone, MM::Panic(MM::ERamAllocMutexCreateFailed));
	}

void M::Init3()
	{
	__KTRACE_OPT2(KBOOT,KMMU,Kern::Printf("M::Init3"));
	}

void M::Init4()
    {
    // Fourth phase MMU initialisation - Not required on this memory model.
    }

// kernel heap construction

void P::KernelInfo(TProcessCreateInfo& aInfo, TAny*& aStack, TAny*& aHeap)
	{
	memclr(&aInfo.iUids, sizeof(TProcessCreateInfo)-sizeof(aInfo.iFileName));
	aInfo.iFileName=KKernelFullPathName;
	aInfo.iRootNameOffset=0;
	aInfo.iRootNameLength=9;
	aInfo.iExtOffset=5;
	aInfo.iAttr=ECodeSegAttKernel|ECodeSegAttFixed;

	aStack=NULL;
	aHeap=(TAny*)MM::KernelHeapAddress;

#ifdef _UNICODE
	TUint16 nameW[KMaxFileName+1];
	int index;
	for(index=0;index<aInfo.iFileName.Length();index++)
		nameW[index] = (TUint16)aInfo.iFileName[index];
	nameW[index] = 0;
	Emulator::RImageFile pefile;
	TInt r = pefile.Open((LPCTSTR)nameW);
#else
	TFileName name=aInfo.iFileName;
	name.Append('\0');
	Emulator::RImageFile pefile;
	TInt r = pefile.Open((LPCTSTR)name.Ptr());
#endif
	if (r == KErrNone)
		{
		pefile.GetInfo(aInfo);
		pefile.Close();
		}
	__ASSERT_ALWAYS(r==KErrNone, FAULT());
	aInfo.iHeapSizeMin = KKernelHeapSizeMin;
	aInfo.iHeapSizeMax = KKernelHeapSizeMax;
	aInfo.iDataRunAddress = (TLinAddr)MM::KernelHeapAddress;
	aInfo.iStackSize = 0;
	}

TInt M::InitSvHeapChunk(DChunk* aChunk, TInt aSize)
	{
	DWin32Chunk* pC=(DWin32Chunk*)aChunk;
	aSize = MM::RoundToPageSize(aSize);
	pC->iSize = aSize;
	pC->iBase = (TUint8*)K::Allocator;
// mark the kernel heap pages as allocated
	MM::FreeMemory -= aSize;
	__KTRACE_OPT(KBOOT,Kern::Printf("Created SvHeap chunk, addr %08X, init size %08X max size %08X",pC->Base(),aSize,pC->MaxSize()));
	K::HeapInfo.iChunk = aChunk;
	K::HeapInfo.iBase = pC->iBase;
	K::HeapInfo.iMaxSize = pC->MaxSize();
	return KErrNone;
	}

void RHeapK::Mutate(TInt aOffset, TInt aMaxLength)
//
// Used by the kernel to mutate a fixed heap into a chunk heap.
//
	{
	iMinLength += aOffset;
	iMaxLength = aMaxLength + aOffset;
	iOffset = aOffset;
	iChunkHandle = (TInt)K::HeapInfo.iChunk;
	iPageSize = M::PageSizeInBytes();
	iGrowBy = iPageSize;
	iFlags &= ~(RAllocator::ESingleThreaded|RAllocator::EFixedSize);
	}

//

TInt M::InitSvStackChunk()
	{
	return KErrNone;
	}

