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
// e32test\mmu\d_shadow.cpp
// LDD for testing ROM shadowing
// 
//

#include <kernel/kern_priv.h>
#include "platform.h"
#include <kernel/cache.h>
#include "d_shadow.h"

const TInt KMajorVersionNumber=0;
const TInt KMinorVersionNumber=1;
const TInt KBuildVersionNumber=1;

_LIT(KLddName,"Shadow");

#ifdef __CPU_X86
const TUint KPageDirectorySize = 1024;
const TUint KMaxNumberOfPageDirectories = 1024;
const TUint KPsudoX86TTBCR = 512;
#else 
const TUint KPageDirectorySize = 4096;  // Full size (ttbr0+ttbr1)
const TUint KMaxNumberOfPageDirectories = 256;
#endif

class DShadow;

class DShadowFactory : public DLogicalDevice
//
// Shadow ROM LDD factory
//
	{
public:
	DShadowFactory();
	virtual TInt Install();						//overriding pure virtual
	virtual void GetCaps(TDes8& aDes) const;	//overriding pure virtual
	virtual TInt Create(DLogicalChannelBase*& aChannel);	//overriding pure virtual
	};

class DShadow : public DLogicalChannelBase
//
// Shadow ROM logical channel
//
	{
public:
	DShadow();
	~DShadow();
protected:
	virtual TInt DoCreate(TInt aUnit, const TDesC8* anInfo, const TVersion& aVer);
	virtual TInt Request(TInt aFunction, TAny* a1, TAny* a2);

	// Memory model specific values will be initialised when the channel is created.
	TUint iPageDirectoryBase;
	TUint iPageTableBase;
	TMemModel  iMemoryModel;
	};

DECLARE_STANDARD_LDD()
	{
    return new DShadowFactory;
    }

DShadowFactory::DShadowFactory()
//
// Constructor
//
    {
    iVersion=TVersion(KMajorVersionNumber,KMinorVersionNumber,KBuildVersionNumber);
    //iParseMask=0;//No units, no info, no PDD
    //iUnitsMask=0;//Only one thing
    }

TInt DShadowFactory::Create(DLogicalChannelBase*& aChannel)
//
// Create a new DShadow on this logical device
//
    {
	aChannel=new DShadow;
	return aChannel?KErrNone:KErrNoMemory;
    }

TInt DShadowFactory::Install()
//
// Install the LDD - overriding pure virtual
//
    {
    return SetName(&KLddName);
    }

void DShadowFactory::GetCaps(TDes8& aDes) const
//
// Get capabilities - overriding pure virtual
//
    {
    TCapsShadowV01 b;
    b.iVersion=TVersion(KMajorVersionNumber,KMinorVersionNumber,KBuildVersionNumber);
    Kern::InfoCopy(aDes,(TUint8*)&b,sizeof(b));
    }

DShadow::DShadow()
//
// Constructor
//
    {
    }

TInt DShadow::DoCreate(TInt /*aUnit*/, const TDesC8* /*anInfo*/, const TVersion& aVer)
//
// Create channel
//
    {

    if (!Kern::QueryVersionSupported(TVersion(KMajorVersionNumber,KMinorVersionNumber,KBuildVersionNumber),aVer))
    	return KErrNotSupported;
	// Set memory model specific values.
	TUint32 memoryModelAttrib = (TUint32)Kern::HalFunction(EHalGroupKernel,EKernelHalMemModelInfo,0,0);    
	switch (memoryModelAttrib & EMemModelTypeMask)
		{
		case EMemModelTypeMoving:
			iPageDirectoryBase = 0x61000000;
			iPageTableBase = 0x62000000;
			iMemoryModel = EMemModelMoving;
			break;
		case EMemModelTypeMultiple:
			iPageDirectoryBase = 0xC1000000;
			iPageTableBase = 0xC2000000;
			iMemoryModel = EMemModelMultiple;
			break;
		case EMemModelTypeFlexible:
			iPageDirectoryBase = 0xF4000000u;
			iPageTableBase = 0xF8000000u;
			iMemoryModel = EMemModelFlexible;
			break;
		default:
			iPageDirectoryBase = 0x00000000;
			iPageTableBase = 0x00000000;
			iMemoryModel = EMemModelOther;
		}
	return KErrNone;
	}

DShadow::~DShadow()
//
// Destructor
//
    {
    }

#ifdef __MARM__
extern TInt DoRead(TAny*);
extern TInt GetMmuId();
extern TInt GetCacheType();
extern TUint GetTTBCR();
extern TUint GetControlRegister();
#endif

LOCAL_C TInt KernStackSize()
	{
	TRomEntry* pE=(TRomEntry*)Kern::SuperPage().iPrimaryEntry;
	TRomImageHeader* pI=(TRomImageHeader*)pE->iAddressLin;
	return pI->iStackSize;
	}

LOCAL_C TInt MeasureKernStackUse()
	{
	TLinAddr kstack=Kern::RoundToPageSize(Epoc::RomHeader().iKernDataAddress + Epoc::RomHeader().iTotalSvDataSize);
	TLinAddr kstackEnd=kstack+KernStackSize();
	TUint32 *pS=(TUint32*)kstack;
	while(*pS==0xffffffff) pS++;
	TUint used=kstackEnd-TLinAddr(pS);
	return (TInt)used;
	}

#if !defined(__WINS__)
TInt DShadow::Request(TInt aFunction, TAny* a1, TAny* a2)
	{
	TInt pageSize=Kern::RoundToPageSize(1);
	TInt r=KErrNone;
	switch (aFunction)
		{
		case RShadow::EControlAllocShadow:
			NKern::ThreadEnterCS();
			r=Epoc::AllocShadowPage(TLinAddr(a1));
			NKern::ThreadLeaveCS();
			break;
		case RShadow::EControlFreeShadow:
			NKern::ThreadEnterCS();
			r=Epoc::FreeShadowPage(TLinAddr(a1));
			NKern::ThreadLeaveCS();
			break;

		case RShadow::EControlWriteShadow: //copy 4KB(page size) data to shadow page
			{
			NKern::ThreadEnterCS();
			void* alloc = Kern::Alloc(pageSize); //CopyToShadowMemory assumes Kernel adress space. Copy here first
			if (alloc)
				{
				kumemget(alloc,a2,pageSize);//From user space to kernel heap
				for (TInt i=0;i<pageSize;i+=32)
					Epoc::CopyToShadowMemory((TLinAddr)((TInt)a1+i),(TLinAddr)((TInt)alloc+i),32);
				Cache::IMB_Range((TLinAddr)a1,pageSize);
				Kern::Free(alloc);
				}
			else
				r = KErrNoMemory;
			
			NKern::ThreadLeaveCS();
			}
			break;
		case RShadow::EControlFreezeShadow:
			NKern::ThreadEnterCS();
			r=Epoc::FreezeShadowPage(TLinAddr(a1));
			NKern::ThreadLeaveCS();
			break;
		case RShadow::EControlSetPriority:
			{
			TInt h=(TInt)a1;
			TInt p=(TInt)a2;
			NKern::LockSystem();
			DThread *pT=(DThread*)Kern::CurrentThread().ObjectFromHandle(h);
			pT->SetThreadPriority(p);
			NKern::UnlockSystem();
			break;
			}
#ifdef __MARM__
		case RShadow::EControlRead:
			r=DoRead(a1);
			break;
		case RShadow::EControlMmuId:
			r=GetMmuId();
			break;
		case RShadow::EControlCacheType:
			r=GetCacheType();
			break;
#endif
		case RShadow::EControlMeasureKernStackUse:
			r=MeasureKernStackUse();
			break;
		case RShadow::EControlMeasureKernHeapFree:
			r=KErrNotSupported;
			break;
		case RShadow::EControlCallFunction:
			{
			TThreadFunction f=(TThreadFunction)a1;
			r=(*f)(a2);
			break;
			}
		case RShadow::EControlAllocPhys:
			{
			
			TInt size=(TInt)a1;
			TInt align=(TInt)a2;
			TPhysAddr pa;
		
			NKern::ThreadEnterCS();
			r=Epoc::AllocPhysicalRam(size,pa,align);
			NKern::ThreadLeaveCS();
			
			if (r==KErrNone)
				{
				if (pa&0x0f)
					r=KErrCorrupt;
				else
					r=pa>>4;
				}
			
			break;
			}
		case RShadow::EControlFreePhys:
			{
			
			TPhysAddr pa=(TPhysAddr)a1;
			TInt size=(TInt)a2;
			NKern::ThreadEnterCS();
			r=Epoc::FreePhysicalRam(pa,size);
			NKern::ThreadLeaveCS();
			break;
			}
		case RShadow::EControlClaimPhys:
			{
			
			TPhysAddr pa=(TPhysAddr)a1;
			TInt size=(TInt)a2;
			NKern::ThreadEnterCS();
			r=Epoc::ClaimPhysicalRam(pa,size);
			NKern::ThreadLeaveCS();
			break;
			}
			
		// GetMemoryArchitecture
		case RShadow::EControlGetMemoryArchitecture:
			{
			TCpu* cpu = (TCpu*) a1;
			TUint* flags = (TUint*) a2;
			
#if defined(__CPU_ARM)
			*cpu=ECpuArm;
			*flags = GetControlRegister();
#elif defined(__CPU_X86)
			*cpu=ECpuX86;
			*flags =0;
#else
			*cpu=ECpuUnknown;
			*flags =0;
#endif
			
			break;
			}
			
			
		// GetMemModelInfo
		case RShadow::EControlGetMemModelInfo:
			{
			TUint pageTable;
			TUint numPds;
						 
#ifdef __EPOC32__

			if (iMemoryModel == EMemModelFlexible || iMemoryModel == EMemModelMultiple)
				numPds = KMaxNumberOfPageDirectories;
			else
				numPds = 0;	
			r = iMemoryModel;
#endif
			pageTable = iPageTableBase;			

			kumemput(a1, &pageTable, sizeof(TUint));
			kumemput(a2, &numPds, sizeof(TUint));
			break;
			}
			
			
		// GetPdInfo
		case RShadow::EControlGetPdInfo:
			{
			TUint pd;
			kumemget(&pd, a1, sizeof(TUint));

			TUint pdSize;
			TUint pdBase;

			r=KErrNoPageTable;

			if (iMemoryModel == EMemModelMoving)
				{
				if (pd==KGlobalPageDirectory)
					{
					pdSize=KPageDirectorySize;
					pdBase=iPageDirectoryBase;
					r = KErrNone;
					}
				}
			else if(iMemoryModel == EMemModelFlexible || iMemoryModel == EMemModelMultiple)
				{
#ifdef __CPU_X86
				TUint ttbcr = KPsudoX86TTBCR;
#else
				TUint ttbcr = KPageDirectorySize >> GetTTBCR();
#endif // __CPU_X86

				if (pd==KGlobalPageDirectory)
					{
					pdSize=KPageDirectorySize - ttbcr;
					pdBase=iPageDirectoryBase + ttbcr*4;
					r = ttbcr & KPageOffsetMask;
					}
				else
					{
					pdSize = ttbcr;
					pdBase=iPageDirectoryBase + pd * KPageDirectorySize * 4;

					TPhysAddr phys=Epoc::LinearToPhysical((TLinAddr)pdBase);				
					r = (phys==KPhysAddrInvalid) ? KErrNoPageTable : KErrNone;
					}
				}

			if ((r & KErrNoPageTable) == 0)
				{
				kumemput(a1, &pdSize, sizeof(TUint));
				kumemput(a2, &pdBase, sizeof(TUint));
				}
			
			break;	
			}
			
		default:
			r=KErrNotSupported;
			break;
		}
	return r;
	}
#else
TInt DShadow::Request(TInt /*aFunction*/, TAny* /*a1*/, TAny* /*a2*/)
	{

	return KErrNotSupported;
	}
#endif

