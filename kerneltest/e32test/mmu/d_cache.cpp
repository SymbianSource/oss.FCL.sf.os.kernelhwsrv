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
// e32test\debug\d_cache.cpp
// See e32test\mmu\t_cache.cpp for details
// 
//

#include "d_cache.h"
#include <kernel/kern_priv.h>
#include <kernel/cache.h>

extern TUint32 GetCacheType();
extern void TestCodeFunc();
extern TInt TestCodeFuncSize();
extern void DataSegmetTestFunct(void* aBase, TInt aSize);

#ifdef __XSCALE_L2_CACHE__
extern TUint32 L2CacheTypeReg();
#endif

#if defined(__CPU_MEMORY_TYPE_REMAPPING)
extern TUint32 CtrlRegister();
extern TUint32 PRRRRegister();
extern TUint32 NRRRRegister();
extern void SetPRRR(TUint32);
extern void SetNRRR(TUint32);
#endif


typedef void(CodeTest) ();

class DCacheTest : public DLogicalChannelBase
	{
public:
	DCacheTest();
	~DCacheTest();
protected:
	virtual TInt DoCreate(TInt aUnit, const TDesC8* anInfo, const TVersion& aVer);
	virtual TInt Request(TInt aFunction, TAny* a1, TAny* a2);
private:
	TInt GetCacheInfo(TAny* a1);
	TInt TestDataChunk(TAny* a1);
	TInt TestCodeChunk(TAny* a1);
	TInt TestWriteBackMode(TAny* a1, TBool aWriteAlloc);
	TInt TestL2Maintenance();
	TInt GetThreshold(TAny* a1);
	TInt SetThreshold(TAny* a1);
	TInt TestUseCase(TAny* a1);
	void LoopTestCodeFunc(CodeTest* f);


	void GetExternalCacheInfo(RCacheTestDevice::TCacheInfo& info);

	void CheckRemapping(RCacheTestDevice::TCacheInfo& info);
	void Remap(RCacheTestDevice::TCacheAttr aCacheAttr);

	TInt UseCase_ReadFromChunk(RCacheTestDevice::TChunkTest& info);
	TInt UseCase_ReadFromChunk_ReadFromHeap(RCacheTestDevice::TChunkTest& info);
	TInt UseCase_WriteToChunk(RCacheTestDevice::TChunkTest& info);
	TInt UseCase_WriteToChunk_ReadFromHeap(RCacheTestDevice::TChunkTest& info);


	//Phys. memory and shared chunk alloc/dealloc primitives
	TInt AllocPhysicalRam(TInt aSize);
	void FreePhysicalRam();
	TInt CreateSharedChunk(TInt aMapAttr, TUint32& aActualMapAttr);
	void CloseSharedChunk();

private:
	DChunk* 	iSharedChunk;	// Shared chunk used in the test
	TPhysAddr 	iPhysAddr;		// Physical address of the allocated memory assigned to the chunk
	TUint 		iSize;			// The size of the allocated memory.
	TLinAddr 	iChunkBase;		// Base linear address of the shared chunk.

	TInt* iHeap1;
	TInt* iHeap2;
	TUint32 iDummy;
	};

DCacheTest* CacheTestDriver;

DCacheTest::DCacheTest() 	{}

DCacheTest::~DCacheTest()	{CacheTestDriver = NULL;}

/**Creates the channel*/
TInt DCacheTest::DoCreate(TInt /*aUnit*/, const TDesC8* /*anInfo*/, const TVersion& /*aVer*/) {return KErrNone;}

/** Allocates physical memory and sets iPhysAddr & iSize accordingly.*/
TInt DCacheTest::AllocPhysicalRam(TInt aSize)
	{
	iSize = aSize;
	NKern::ThreadEnterCS();
	TInt r = Epoc::AllocPhysicalRam(aSize, iPhysAddr, 0); //Allocate physical RAM. This will set iPhysAddr
	NKern::ThreadLeaveCS();
	return r;
	}

/** Frees physical memory.*/
void DCacheTest::FreePhysicalRam()
	{
	NKern::ThreadEnterCS();
	Epoc::FreePhysicalRam(iPhysAddr, iSize);
	NKern::ThreadLeaveCS();
	}

/**
Creates shared chunks with allocated physical memory and sets iChunkBase accordingly.
@pre Physical memory is allocated (iPhysAddr & iSize are set accordingly).
*/
TInt DCacheTest::CreateSharedChunk(TInt aMapAttr, TUint32& aActualMapAttr)
	{
	TInt r;
    TChunkCreateInfo chunkInfo;
    chunkInfo.iType         = TChunkCreateInfo::ESharedKernelSingle;
    chunkInfo.iMaxSize      = iSize;
    chunkInfo.iMapAttr      = aMapAttr;
    chunkInfo.iOwnsMemory   = EFalse;
    chunkInfo.iDestroyedDfc = NULL;

	NKern::ThreadEnterCS();
    if (KErrNone != (r = Kern::ChunkCreate(chunkInfo, iSharedChunk, iChunkBase, aActualMapAttr)))
		{
		FreePhysicalRam();
		NKern::ThreadLeaveCS();
		return r;
		}
	r = Kern::ChunkCommitPhysical(iSharedChunk,0,iSize, iPhysAddr);
    if(r!=KErrNone)
        {
		CloseSharedChunk();
		FreePhysicalRam();
		NKern::ThreadLeaveCS();
		return r;
		}
	NKern::ThreadLeaveCS();
	return KErrNone;
	}

/** Closes shared chunk.*/
void DCacheTest::CloseSharedChunk()
	{
	NKern::ThreadEnterCS();
	Kern::ChunkClose(iSharedChunk);
	Kern::HalFunction(EHalGroupKernel, EKernelHalSupervisorBarrier, 0, 0);	// make sure async close has happened
	NKern::ThreadLeaveCS();
	}


#if defined(__CPU_ARMV7)
extern TUint32 CacheTypeRegister();
extern TUint32 CacheLevelIDRegister();
extern TUint32 CacheSizeIdRegister(TUint32 aType/*0-1*/, TUint32 aLevel/*0-7*/);

void ParseCacheLevelInfo(TInt aCacheSizeIDReg, RCacheTestDevice::TCacheSingle& aCS)
	{
	aCS.iSets = ((aCacheSizeIDReg>>13)& 0x7fff)+1;
	aCS.iWays =   ((aCacheSizeIDReg>>3)& 0x3ff)+1;
	aCS.iLineSize =1<<((aCacheSizeIDReg & 0x7)+4);//+2 (and +2 as we count in bytes)
	aCS.iSize = aCS.iSets * aCS.iWays * aCS.iLineSize;
	}
#endif


void AppendTo(TDes8& aDes, const char* aFmt, ...)
	{
	VA_LIST list;
	VA_START(list,aFmt);
	Kern::AppendFormat(aDes,aFmt,list);
	}

/** Checks Memory Remap settings (both memory type and access permission remapping).*/
void DCacheTest::CheckRemapping(RCacheTestDevice::TCacheInfo& info)
	{
#if defined(__CPU_MEMORY_TYPE_REMAPPING)
	TUint32 cr = CtrlRegister();
	TUint32 prrr =PRRRRegister();
	TUint32 nrrr =NRRRRegister();
	AppendTo(info.iDesc,"Memory Remapping: CtrlReg:%xH, PRRR:%xH NRRR:%xH\n", cr, prrr, nrrr);

	if ( (cr&0x30000000) == 0x30000000)
		info.iMemoryRemapping = 1;
	else
		AppendTo(info.iDesc,"Error:Memory Remapping is OFF \n");
#endif
	}

//Remaps aCacheAttr memory type into EMemAttKernelInternal4
void DCacheTest::Remap(RCacheTestDevice::TCacheAttr aCacheAttr)
	{
#if defined(__CPU_MEMORY_TYPE_REMAPPING)
	TInt inner, outer;
	switch(aCacheAttr)
		{
		case RCacheTestDevice::E_InnerWT_Remapped: 	inner=2;outer=0;break;
		case RCacheTestDevice::E_InnerWBRA_Remapped:inner=3;outer=0;break;
		case RCacheTestDevice::E_InnerWB_Remapped:	inner=1;outer=0;break;
		case RCacheTestDevice::E_OuterWT_Remapped:	inner=0;outer=2;break;
		case RCacheTestDevice::E_OuterWBRA_Remapped:inner=0;outer=3;break;
		case RCacheTestDevice::E_OuterWB_Remapped:	inner=0;outer=1;break;
		case RCacheTestDevice::E_InOutWT_Remapped:	inner=2;outer=2;break;
		case RCacheTestDevice::E_InOutWBRA_Remapped:inner=3;outer=3;break;
		case RCacheTestDevice::E_InOutWB_Remapped:	inner=1;outer=1;break;
		default:Kern::PanicCurrentThread(_L("d_cache driver error"),0);return;
		}

	TUint32 prrr =PRRRRegister();
	TUint32 nrrr =NRRRRegister();
	prrr &= ~(3<<8);	// Clear EMemAttKernelInternal4 setting for memory type
	nrrr &= ~(3<<8); 	// Clear EMemAttKernelInternal4 setting for normal memory type, inner cache
	nrrr &= ~(3<<24);	// Clear EMemAttKernelInternal4 setting for normal memory type, outer cache
	prrr |= 2 <<8; 		// Set EMemAttKernelInternal4 as normal memory
	nrrr |= inner <<8;	// Set inner cache for EMemAttKernelInternal4 
	nrrr |= outer << 24;// Set outer cache for EMemAttKernelInternal4 

	SetPRRR(prrr);
	SetNRRR(nrrr);
#endif
	}



/** Fills in info structure with external cache parameters. */
void DCacheTest::GetExternalCacheInfo(RCacheTestDevice::TCacheInfo& info)
	{
#if defined(__HAS_EXTERNAL_CACHE__)
	info.iOuterCache=1;

#if defined(__ARM_L210_CACHE__)
	AppendTo(info.iDesc,"Built as L210 Cache;\n");
#elif defined(__ARM_L220_CACHE__)
	AppendTo(info.iDesc,"Built as L220 Cache:\n");
#elif defined(__ARM_PL310_CACHE__)
	AppendTo(info.iDesc,"Built as PL310 Cache:\n");
#endif

	TInt cacheController = Kern::SuperPage().iArmL2CacheBase;
	if (!cacheController)
		{
		AppendTo(info.iDesc,"Warning:No CCB Address in Super Page?\n");
		return;
		}
		
	TInt rawData = *(TInt*)(cacheController);   //reg 0 in controller is Cache ID Register
	AppendTo(info.iDesc,"L2 ID Reg:%xH\n", rawData);

	rawData = *(TInt*)(cacheController+4); //reg 4 in controller is Cache Type Register
	AppendTo(info.iDesc,"L2 Type Reg:%xH\n", rawData);

	RCacheTestDevice::TCacheSingle& cs = info.iCache[info.iCacheCount];

	cs.iLineSize=32; //always
#if defined(__ARM_L210_CACHE__) || defined(__ARM_L220_CACHE__)
	cs.iWays = (rawData>>3)&0x0f;	if (cs.iWays > 8) cs.iWays = 8;
#elif defined(__ARM_PL310_CACHE__)
	cs.iWays = (rawData&0x40) ? 16:8;
#endif
	TInt waySize;
	switch((rawData>>8)&7)
		{
		case 0:		waySize = 0x4000;  break;
		case 1:		waySize = 0x4000;  break;
		case 2:		waySize = 0x8000;  break;
		case 3:		waySize = 0x10000; break;
		case 4:		waySize = 0x20000; break;
#if defined(__ARM_L210_CACHE__) || defined(__ARM_L220_CACHE__)
		default:	waySize = 0x40000; break;
#elif defined(__ARM_PL310_CACHE__)
		case 5:		waySize = 0x40000; break;
		default:	waySize = 0x80000; break;
#endif
		}
	cs.iSize = waySize * cs.iWays;
	cs.iSets = waySize >> 5; // = waySize / lineLen 


	cs.iLevel = 2;
	cs.iCode = 1;
	cs.iData = 1;
	cs.iDesc.SetLength(0);
	AppendTo(cs.iDesc,"Outer Unified PAPT");

	info.iMaxCacheSize = Max(info.iMaxCacheSize, cs.iSize);
	info.iCacheCount++;
#endif //defined(__HAS_EXTERNAL_CACHE__)
	}


/** Passes cache configuration parameters to the user side*/
TInt DCacheTest::GetCacheInfo(TAny* a1)
	{
	TInt ret = KErrNone;
	RCacheTestDevice::TCacheInfo info;

	info.iDesc.SetLength(0);
	info.iCacheCount=0;
	info.iMaxCacheSize=0;
	info.iMemoryRemapping=0;
	info.iOuterCache=0;

////////////////////////
#if defined(__CPU_ARMV7)
////////////////////////
	info.iOuterCache=1;

	TUint32 ctr=CacheTypeRegister();
	TUint32 clr=CacheLevelIDRegister();
	TInt LoC = (clr>>24)&7;	//The number of levels to be purged/clean to Point-to-Coherency
	TInt LoU = (clr>>27)&7;	//The number of levels to be purged/clean to Point-to-Unification
	AppendTo(info.iDesc,"ARMv7 cache - CTR:%xH CLR:%xH LoC:%d LoU:%d\n", ctr, clr, LoC, LoU);
	
	RCacheTestDevice::TCacheSingle* cs = &info.iCache[info.iCacheCount];
	TInt level;
	for (level=0;level<LoC;level++)
		{
		TInt type = (clr >> (level*3)) & 7; //000:NoCache 001:ICache 010:DCache 011:Both 100:Unified
		
		if (type==0)		// No Cache. Also no cache below this level
			break;
		
		if(type & 1) 	// Instruction Cache
			{
			TInt csr = CacheSizeIdRegister(1,level);
			ParseCacheLevelInfo(csr, *cs);
			cs->iLevel = level+1;
			cs->iCode = 1;
			cs->iData = 0;
			AppendTo(cs->iDesc,"ICache CSR:%xH",csr);
			info.iMaxCacheSize = Max(info.iMaxCacheSize, cs->iSize);
			cs = &info.iCache[++info.iCacheCount];
			}
			
		if(type & 2) 	// Data Cache
			{
			TInt csr = CacheSizeIdRegister(0,level);
			ParseCacheLevelInfo(csr, *cs);
			cs->iLevel = level+1;
			cs->iCode = 0;
			cs->iData = 1;
			AppendTo(cs->iDesc,"DCache CSR:%xH",csr);
			info.iMaxCacheSize = Max(info.iMaxCacheSize, cs->iSize);
			cs = &info.iCache[++info.iCacheCount];
			}

		if(type & 4) 	// Unified Cache
			{
			TInt csr = CacheSizeIdRegister(0,level);
			ParseCacheLevelInfo(csr, *cs);
			cs->iLevel = level+1;
			cs->iCode = 1;
			cs->iData = 1;
			AppendTo(cs->iDesc,"Unified CSR:%xH",csr);
			info.iMaxCacheSize = Max(info.iMaxCacheSize, cs->iSize);
			cs = &info.iCache[++info.iCacheCount];
			}
		}

///////////////////////////////////
#elif defined(__CPU_HAS_CACHE_TYPE_REGISTER)
///////////////////////////////////

	TInt rawData=GetCacheType();
	TInt splitCache=rawData&0x01000000;
	AppendTo(info.iDesc,"L1 Cache TypeReg=%xH\n", rawData);

	//Cache #1	
	TUint32 s=(rawData>>12)&0xfff;  		//s = P[11]:0:size[9:5]:assoc[5:3]:M[2]:len[1:0] 
	info.iCache[info.iCacheCount].iLineSize = 1 << ((s&2) + 3); 							//1<<(len+3)
	info.iCache[info.iCacheCount].iWays = (2 + ((s>>2)&1)) << (((s>>3)&0x7) - 1);			//(2+M) << (assoc-1)
	info.iCache[info.iCacheCount].iSize = (2 + ((s>>2)&1)) << (((s>>6)&0xf) + 8);			//(2+M) << (size+8)
	info.iCache[info.iCacheCount].iSets = 1 << (((s>>6)&0xf) + 6 - ((s>>3)&0x7) - (s&2));	//(2+M) <<(size + 6 -assoc - len)
	info.iCache[info.iCacheCount].iData = 1;
	info.iCache[info.iCacheCount].iLevel = 1;

	if (splitCache)
		{
		info.iCache[info.iCacheCount].iCode = 0;
		info.iCache[info.iCacheCount].iDesc.SetLength(0);
		AppendTo(info.iCache[info.iCacheCount].iDesc,"Inner DCache");

		#if defined(__CPU_ARMV6)
		AppendTo(info.iCache[info.iCacheCount].iDesc," VAPT");
		#else
		AppendTo(info.iCache[info.iCacheCount].iDesc," VAVT");
		#endif		
		info.iMaxCacheSize = Max(info.iMaxCacheSize, info.iCache[info.iCacheCount].iSize);
		info.iCacheCount++;

		// Cache #2
		s=rawData&0xfff;  		//s = P[11]:0:size[9:5]:assoc[5:3]:M[2]:len[1:0] 
		info.iCache[info.iCacheCount].iLineSize = 1 << ((s&2) + 3); 							//1<<(len+3)
		info.iCache[info.iCacheCount].iWays = (2 + ((s>>2)&1)) << (((s>>3)&0x7) - 1);			//(2+M) << (assoc-1)
		info.iCache[info.iCacheCount].iSize = (2 + ((s>>2)&1)) << (((s>>6)&0xf) + 8);			//(2+M) << (size+8)
		info.iCache[info.iCacheCount].iSets = 1 << (((s>>6)&0xf) + 6 - ((s>>3)&0x7) - (s&2));	//(2+M) <<(size + 6 -assoc - len)
		info.iCache[info.iCacheCount].iLevel = 1;
		info.iCache[info.iCacheCount].iCode = 1;
		info.iCache[info.iCacheCount].iData = 0;
		info.iCache[info.iCacheCount].iDesc.SetLength(0);
		AppendTo(info.iCache[info.iCacheCount].iDesc,"Inner ICache");
		#if defined(__CPU_ARMV6)
		AppendTo(info.iCache[info.iCacheCount].iDesc," VAPT");
		#else
		AppendTo(info.iCache[info.iCacheCount].iDesc," VAVT");
		#endif		
		}
	else
	{
		info.iCache[info.iCacheCount].iCode = 1;
		info.iCache[info.iCacheCount].iDesc.SetLength(0);
		AppendTo(info.iCache[info.iCacheCount].iDesc,"Inner Unified");
		#if defined(__CPU_ARMV6)
		AppendTo(info.iCache[info.iCacheCount].iDesc," VAPT");
		#else
		AppendTo(info.iCache[info.iCacheCount].iDesc," VAVT");
		#endif		
	}		
	info.iMaxCacheSize = Max(info.iMaxCacheSize, info.iCache[info.iCacheCount].iSize);
	info.iCacheCount++;

/////
#else
/////

	ret = KErrNotSupported;

#endif

	GetExternalCacheInfo(info); // Get ARMl210/20 info
	CheckRemapping(info);		// Get memory remapping info

	info.iDmaBufferAlignment = Cache::DmaBufferAlignment();
	kumemput(a1,&info,sizeof(info));
	return ret;
	}

/** Get cache thresholds.*/
TInt DCacheTest::GetThreshold(TAny* a1)
	{
	RCacheTestDevice::TThresholdInfo info;
	kumemget(&info,a1,sizeof(info));

	TCacheThresholds thresholds;
	TInt r = Cache::GetThresholds(thresholds, info.iCacheType);
	if (r==KErrNone)
		{
		info.iPurge = thresholds.iPurge;	
		info.iClean = thresholds.iClean;	
		info.iFlush = thresholds.iFlush;	
		kumemput(a1,&info,sizeof(info));
		}
	return r;
	}

/** Set cache thresholds.*/
TInt DCacheTest::SetThreshold(TAny* a1)
	{
	RCacheTestDevice::TThresholdInfo info;
	kumemget(&info,a1,sizeof(info));

	TCacheThresholds thresholds;
	thresholds.iPurge = info.iPurge;
	thresholds.iClean = info.iClean;
	thresholds.iFlush = info.iFlush;
	return Cache::SetThresholds(thresholds, info.iCacheType);
	}

// Runs DataSegmetTestFunct against data from a chunk.
// Chunk cache attributes and its size are specified in input arguments.
// Measures and returns the time spent.
TInt DCacheTest::TestDataChunk(TAny* a1)
	{
	TInt r = KErrNone;
	TInt time;
	
	RCacheTestDevice::TChunkTest info;
	kumemget(&info,a1,sizeof(info));


	TUint32 chunkAttr = EMapAttrSupRw;
	if (info.iShared) chunkAttr |= EMapAttrShared;
#ifdef __SMP__
	TUint32 force_shared = EMapAttrShared;
#else
	TUint32 force_shared = 0;
#endif

	switch (info.iCacheAttr)
		{

		case RCacheTestDevice::E_FullyBlocking:	chunkAttr |= EMapAttrFullyBlocking; break;
		case RCacheTestDevice::E_Buffered_NC:	chunkAttr |= EMapAttrBufferedNC; break;
		case RCacheTestDevice::E_Buffered_C:	chunkAttr |= EMapAttrBufferedC; break;

		case RCacheTestDevice::E_InnerWT:		chunkAttr |= EMapAttrCachedWTRA|force_shared; break;
		case RCacheTestDevice::E_InnerWBRA:		chunkAttr |= EMapAttrCachedWBRA|force_shared; break;
		case RCacheTestDevice::E_InnerWB:		chunkAttr |= EMapAttrCachedWBWA|force_shared; break;

		case RCacheTestDevice::E_OuterWT:		chunkAttr |= EMapAttrL2CachedWTRA; break;
		case RCacheTestDevice::E_OuterWBRA:		chunkAttr |= EMapAttrL2CachedWBRA; break;
		case RCacheTestDevice::E_OuterWB:		chunkAttr |= EMapAttrL2CachedWBWA; break;

		case RCacheTestDevice::E_InOutWT:		chunkAttr |= EMapAttrCachedWTRA|EMapAttrL2CachedWTRA|force_shared; break;
		case RCacheTestDevice::E_InOutWBRA:		chunkAttr |= EMapAttrCachedWBRA|EMapAttrL2CachedWBRA|force_shared; break;
		case RCacheTestDevice::E_InOutWB:		chunkAttr |= EMapAttrCachedWBWA|EMapAttrL2CachedWBWA|force_shared; break;

		case RCacheTestDevice::E_StronglyOrder:
			new (&chunkAttr) TMappingAttributes2(EMemAttStronglyOrdered,EFalse,ETrue,EFalse,info.iShared?ETrue:EFalse);
			break;
		case RCacheTestDevice::E_Device:
			new (&chunkAttr) TMappingAttributes2(EMemAttDevice,EFalse,ETrue,EFalse,info.iShared?ETrue:EFalse);
			break;
		case RCacheTestDevice::E_Normal_Uncached:
			new (&chunkAttr) TMappingAttributes2(EMemAttNormalUncached,EFalse,ETrue,EFalse,info.iShared?ETrue:EFalse);
			break;
		case RCacheTestDevice::E_Normal_Cached:
			new (&chunkAttr) TMappingAttributes2(EMemAttNormalCached,EFalse,ETrue,EFalse,(info.iShared|force_shared)?ETrue:EFalse);
			break;
		case RCacheTestDevice::E_KernelInternal4:
			new (&chunkAttr) TMappingAttributes2(EMemAttKernelInternal4,EFalse,ETrue,ETrue,(info.iShared|force_shared)?ETrue:EFalse);
			break;
		case RCacheTestDevice::E_PlatformSpecific5:
			new (&chunkAttr) TMappingAttributes2(EMemAttPlatformSpecific5,EFalse,ETrue,ETrue,(info.iShared|force_shared)?ETrue:EFalse);
			break;
		case RCacheTestDevice::E_PlatformSpecific6:
			new (&chunkAttr) TMappingAttributes2(EMemAttPlatformSpecific6,EFalse,ETrue,ETrue,(info.iShared|force_shared)?ETrue:EFalse);
			break;
		case RCacheTestDevice::E_PlatformSpecific7:
			new (&chunkAttr) TMappingAttributes2(EMemAttPlatformSpecific7,EFalse,ETrue,ETrue,(info.iShared|force_shared)?ETrue:EFalse);
			break;

#if defined(__CPU_MEMORY_TYPE_REMAPPING)
		case RCacheTestDevice::E_InnerWT_Remapped:
		case RCacheTestDevice::E_InnerWBRA_Remapped:
		case RCacheTestDevice::E_InnerWB_Remapped:
		case RCacheTestDevice::E_InOutWT_Remapped:
		case RCacheTestDevice::E_InOutWBRA_Remapped:
		case RCacheTestDevice::E_InOutWB_Remapped:
#ifdef __SMP__
			info.iShared = ETrue;
#endif
		case RCacheTestDevice::E_OuterWT_Remapped:
		case RCacheTestDevice::E_OuterWBRA_Remapped:
		case RCacheTestDevice::E_OuterWB_Remapped:
			Remap(info.iCacheAttr);
			new (&chunkAttr) TMappingAttributes2(EMemAttKernelInternal4,EFalse,ETrue,EFalse,info.iShared?ETrue:EFalse);
			break;
#endif
			
		case RCacheTestDevice::E_Default:
			{
			// Run the test against memory from kernel heap (no need for extra memory chunks)
			NKern::ThreadEnterCS();
			TLinAddr bufferBase = (TLinAddr)Kern::Alloc(info.iSize);
			NKern::ThreadLeaveCS();
			if (!bufferBase)
					return KErrNoMemory;
		
			//You can't purge  allocated heap memory as it will invalidate other data from the same cache line.
			//Cache::SyncMemoryAfterDmaRead((TLinAddr)bufferBase, info.iSize);

			// Execute the test
			time = NKern::TickCount();
			DataSegmetTestFunct((void*)bufferBase, info.iSize);
			info.iTime = NKern::TickCount() - time;
			info.iActualMapAttr = 0;
			kumemput(a1,&info,sizeof(info));

			NKern::ThreadEnterCS();
			Kern::Free((TAny*)bufferBase);
			NKern::ThreadLeaveCS();

			return KErrNone;
			}
		default:
			return KErrArgument;		
		}

	// Run the test against chunk with cache attributes as specified in info.iCacheState.
	if (KErrNone!=(r=AllocPhysicalRam(Kern::RoundToPageSize(info.iSize)))) return r;
	if (KErrNone!=(r=CreateSharedChunk(chunkAttr, info.iActualMapAttr))) return r;
	
	Cache::SyncMemoryAfterDmaRead(iChunkBase, info.iSize); // Invalidate (aka purge) cache.

	time = NKern::TickCount();
	DataSegmetTestFunct((void*)iChunkBase, info.iSize);
	info.iTime = NKern::TickCount() - time;

	CloseSharedChunk();
	FreePhysicalRam();

	kumemput(a1,&info,sizeof(info));
	return KErrNone;
	}

void DCacheTest::LoopTestCodeFunc(CodeTest* f)
	{
	for (TInt x = 0;x<5000;x++)
		(*f)();
	}

// Runs TestCodeFunc (contains nops with ret at the end) from a chunk.
// Chunk cache attributes and the size of function are specified in input arguments
// Measures and returns the time spent.
TInt DCacheTest::TestCodeChunk(TAny* a1)
	{
	TInt r = KErrNone;
	TInt time;
	
	RCacheTestDevice::TChunkTest info;
	kumemget(&info,a1,sizeof(info));


	info.iActualMapAttr = EMapAttrSupRwx;
	if (info.iShared) info.iActualMapAttr |= EMapAttrShared;
#ifdef __SMP__
	TUint32 force_shared = EMapAttrShared;
#else
	TUint32 force_shared = 0;
#endif

	switch (info.iCacheAttr)
		{
		case RCacheTestDevice::E_FullyBlocking:	info.iActualMapAttr |= EMapAttrFullyBlocking; break;
		case RCacheTestDevice::E_Buffered_NC:	info.iActualMapAttr |= EMapAttrBufferedNC; break;
		case RCacheTestDevice::E_Buffered_C:	info.iActualMapAttr |= EMapAttrBufferedC; break;

		case RCacheTestDevice::E_InnerWT:		info.iActualMapAttr |= EMapAttrCachedWTRA|force_shared; break;
		case RCacheTestDevice::E_InnerWBRA:		info.iActualMapAttr |= EMapAttrCachedWBRA|force_shared; break;
		case RCacheTestDevice::E_InnerWB:		info.iActualMapAttr |= EMapAttrCachedWBWA|force_shared; break;

		case RCacheTestDevice::E_OuterWT:		info.iActualMapAttr |= EMapAttrL2CachedWTRA; break;
		case RCacheTestDevice::E_OuterWBRA:		info.iActualMapAttr |= EMapAttrL2CachedWBRA; break;
		case RCacheTestDevice::E_OuterWB:		info.iActualMapAttr |= EMapAttrL2CachedWBWA; break;

		case RCacheTestDevice::E_InOutWT:		info.iActualMapAttr |= EMapAttrCachedWTRA|EMapAttrL2CachedWTRA|force_shared; break;
		case RCacheTestDevice::E_InOutWBRA:		info.iActualMapAttr |= EMapAttrCachedWBRA|EMapAttrL2CachedWBRA|force_shared; break;
		case RCacheTestDevice::E_InOutWB:		info.iActualMapAttr |= EMapAttrCachedWBWA|EMapAttrL2CachedWBWA|force_shared; break;

		case RCacheTestDevice::E_StronglyOrder:
			new (&info.iActualMapAttr) TMappingAttributes2(EMemAttStronglyOrdered,EFalse,ETrue,ETrue,info.iShared?ETrue:EFalse);
			break;
		case RCacheTestDevice::E_Device:
			new (&info.iActualMapAttr) TMappingAttributes2(EMemAttDevice,EFalse,ETrue,ETrue,info.iShared?ETrue:EFalse);
			break;
		case RCacheTestDevice::E_Normal_Uncached:
			new (&info.iActualMapAttr) TMappingAttributes2(EMemAttNormalUncached,EFalse,ETrue,ETrue,info.iShared?ETrue:EFalse);
			break;
		case RCacheTestDevice::E_Normal_Cached:
			new (&info.iActualMapAttr) TMappingAttributes2(EMemAttNormalCached,EFalse,ETrue,ETrue,info.iShared?ETrue:EFalse);
			break;

#if defined(__CPU_MEMORY_TYPE_REMAPPING)
		case RCacheTestDevice::E_InnerWT_Remapped:
		case RCacheTestDevice::E_InnerWBRA_Remapped:
		case RCacheTestDevice::E_InnerWB_Remapped:
		case RCacheTestDevice::E_InOutWT_Remapped:
		case RCacheTestDevice::E_InOutWBRA_Remapped:
		case RCacheTestDevice::E_InOutWB_Remapped:
#ifdef __SMP__
			info.iShared = ETrue;
#endif
		case RCacheTestDevice::E_OuterWT_Remapped:
		case RCacheTestDevice::E_OuterWBRA_Remapped:
		case RCacheTestDevice::E_OuterWB_Remapped:
			Remap(info.iCacheAttr);
			new (&info.iActualMapAttr) TMappingAttributes2(EMemAttKernelInternal4,EFalse,ETrue,ETrue,info.iShared?ETrue:EFalse);
			break;
#endif
			
		case RCacheTestDevice::E_Default:
			{
			// Run the test against test function from rom image (no need for extra memory chunks)
			if (info.iSize > TestCodeFuncSize())
				return KErrNoMemory; // TestCodeFunc is not big enough to conduct the test.
			
			TInt startAddr = (TInt)TestCodeFunc + TestCodeFuncSize() - info.iSize;
			
			// This will invalidate (aka purge) test function from L2 cache.
			Cache::SyncMemoryAfterDmaRead((TLinAddr)startAddr, info.iSize); 

			// Execute the test
			time = NKern::TickCount();
			LoopTestCodeFunc((CodeTest*)startAddr);
			info.iTime = NKern::TickCount() - time;

			info.iActualMapAttr = 0; //Not relevant.
			kumemput(a1,&info,sizeof(info));
			return KErrNone;
			}
		default:
			return KErrArgument;		
		}

	// Run the test against test function from memory chunk with cache attributes as specified in info.iCacheState.
	// As we need a chunk with eXecutable permission attribute, can't use shared chunk. Take HwChunk instead.
	DPlatChunkHw* chunk;
	TPhysAddr physBase;		// This will be base physical address of the chunk
    TLinAddr linearBase;	// This will be base linear address of the chunk
	NKern::ThreadEnterCS();
	r = Epoc::AllocPhysicalRam(Kern::RoundToPageSize(info.iSize), physBase, 0);//Allocate RAM. This will set aPhysAddr
	if (r)
		{
		NKern::ThreadLeaveCS();
		return r;
		}
	r = DPlatChunkHw::New (chunk, physBase, Kern::RoundToPageSize(info.iSize), info.iActualMapAttr);//Create chunk
	if (r)
		{
		Epoc::FreePhysicalRam(physBase, Kern::RoundToPageSize(info.iSize));
		NKern::ThreadLeaveCS();
		return r;
		}
	NKern::ThreadLeaveCS();

	linearBase = chunk->LinearAddress();

	// Create nop,nop,...,nop,ret sequence at the start of the chunk with size = info.iSize
	TInt nopInstr = ((TInt*)TestCodeFunc)[0]; 						// NOP is the first instruction from TestCodeFunc
	TInt retInstr = ((TInt*)TestCodeFunc)[TestCodeFuncSize()/4-1];	// RET is the last instruction in TestCodeFunc 	
	for (TInt i = 0; i < (info.iSize/4-1) ; i++)  	// Put all NOPs...
		((TInt*)linearBase)[i] = nopInstr;			// ...
	((TInt*)linearBase)[info.iSize/4-1] = retInstr;	// ... and add RET at the end.

	Cache::IMB_Range((TLinAddr)linearBase, info.iSize); 			// Sync L1 Instruction & Data cache
	//Fluch the memory from which the test funcion executes. This will give fair chance to all test cases.
	Cache::SyncMemoryBeforeDmaWrite(linearBase, info.iSize);		// This will clean L1&L2 cache.
	Cache::SyncMemoryAfterDmaRead(linearBase, info.iSize);			// This will invalidate (aka purge) L1&L2 cache.

	// Execute the test
	time = NKern::TickCount();
	LoopTestCodeFunc((CodeTest*)linearBase);
	info.iTime = NKern::TickCount() - time;

	kumemput(a1,&info,sizeof(info));

	NKern::ThreadEnterCS();
	chunk->Close(NULL);
	Epoc::FreePhysicalRam(physBase, Kern::RoundToPageSize(info.iSize));
	NKern::ThreadLeaveCS();
	return KErrNone;
	}

/**
Tests WriteBack mode:
	(1)Writes down data into BW cached memory.
	(2)Purge the cache.
	(3)Counts the bytes that reach the main memory.
@param aWriteAlloc True if WriteAllocate to test, EFalse if ReadAllocate
*/
TInt DCacheTest::TestWriteBackMode(TAny* a1, TBool aWriteAlloc)
	{
	TInt r, cacheAttr = EMapAttrSupRw;
	TUint i, counter = 0;
	const TInt pattern = 0xabcdef12;

	RCacheTestDevice::TChunkTest info;
	kumemget(&info,a1,sizeof(info));
#ifdef __SMP__
	TUint32 force_shared = EMapAttrShared;
#else
	TUint32 force_shared = 0;
#endif

	switch (info.iCacheAttr)
		{
#if defined(__CPU_MEMORY_TYPE_REMAPPING)
		case RCacheTestDevice::E_InnerWBRA_Remapped:
		case RCacheTestDevice::E_InnerWB_Remapped:
		case RCacheTestDevice::E_OuterWBRA_Remapped:
		case RCacheTestDevice::E_OuterWB_Remapped:
			Remap(info.iCacheAttr);
			new (&cacheAttr) TMappingAttributes2(EMemAttKernelInternal4,EFalse,ETrue,ETrue,force_shared);
			break;
#endif
		case RCacheTestDevice::E_InnerWBRA:	cacheAttr |= EMapAttrCachedWBRA|force_shared; 	break;
		case RCacheTestDevice::E_InnerWB:	cacheAttr |= EMapAttrCachedWBWA|force_shared; 	break;
		case RCacheTestDevice::E_OuterWBRA:	cacheAttr |= EMapAttrL2CachedWBRA|force_shared;	break;
		case RCacheTestDevice::E_OuterWB:	cacheAttr |= EMapAttrL2CachedWBWA|force_shared;	break;
		default: return KErrArgument;
		}
	// Create chunk
	if (KErrNone!=(r=AllocPhysicalRam(info.iSize))) return r;
	if (KErrNone!=(r=CreateSharedChunk(cacheAttr, info.iActualMapAttr))) return r;
	
	for (i=0; i<(iSize>>2) ; i++) ((TInt*)iChunkBase)[i] = 0;   //Zero-fill cache and...
	Cache::SyncMemoryBeforeDmaWrite(iChunkBase, iSize);			//... clean the cache down to memory

	Cache::SyncMemoryAfterDmaRead(iChunkBase, iSize);			//Invalidate (aka purge).

	// Fill in cached region with the pattern.
	for (i=0; i<(iSize>>2); i++)
	 	{
	 	if (!aWriteAlloc) iDummy = ((TInt*)iChunkBase)[i]; 		// Don't read if WriteAllocate is tested
	 	((TInt*)iChunkBase)[i] = pattern;
	 	}
		
	Cache::SyncMemoryAfterDmaRead(iChunkBase, iSize);	//Invalidate (aka purge) cache. Data in cache should be destroyed
	CloseSharedChunk();									// Close cached chunk.
	
	//Create non-cached chunk over the same physical memory
	if (KErrNone!=(r=CreateSharedChunk(EMapAttrSupRw , iDummy))) return r;

	// Counts out how many bytes have reached RAM
	for (i=0; i<(iSize>>2); i++) if (((TInt*)iChunkBase)[i] == pattern) counter++;

	info.iSize = counter<<2; //Return the number of bytes that reached the main memory
	CloseSharedChunk();
	FreePhysicalRam();
	
	kumemput(a1,&info,sizeof(info));
	return r;
	}

/**
Exercises SyncMemoryBeforeDmaWrite & SyncMemoryAfterDmaRead (that call L1/L2 Cache Clean & Purge methods)
This just ensures that they do not panic (doesn't do any functional test).
*/
TInt DCacheTest::TestL2Maintenance()
	{
	// Create 20000h big chunk with the the memory commited as:
	// |0| NotCommited |64K| Commited |128K| NotCommited |192K| Commited |256K| 
#ifdef __SMP__
	TUint32 force_shared = EMapAttrShared;
#else
	TUint32 force_shared = 0;
#endif
	TInt r;
	TChunkCreateInfo info;
    info.iType         = TChunkCreateInfo::ESharedKernelSingle;
	info.iMaxSize      = 0x40000;
	info.iMapAttr      = EMapAttrSupRw | EMapAttrCachedWBWA | EMapAttrL2CachedWBWA | force_shared;
	info.iOwnsMemory   = ETrue; // Use memory from system's free pool
	info.iDestroyedDfc = NULL;

    TLinAddr chunkAddr;
    TUint32 mapAttr;
    DChunk* chunk;
	TInt pageSize = 0x1000; //4K

	NKern::ThreadEnterCS();
    if (KErrNone != (r = Kern::ChunkCreate(info, chunk, chunkAddr, mapAttr)))
		{
		NKern::ThreadLeaveCS();
		return r;
		}
	r = Kern::ChunkCommit(chunk,0x10000,0x10000);
    if(r!=KErrNone)
        {
		Kern::ChunkClose(chunk);
		NKern::ThreadLeaveCS();
		return r;
		}
	r = Kern::ChunkCommit(chunk,0x30000,0x10000);
    if(r!=KErrNone)
        {
		Kern::ChunkClose(chunk);
		NKern::ThreadLeaveCS();
		return r;
		}

	NKern::ThreadLeaveCS();

	TInt valid = chunkAddr+0x10000;

	#if defined(__ARM_L220_CACHE__) || defined(__ARM_L210_CACHE__)
	// Check L2 cache maintenance for invalid addresses.
	// On ARMv6, clean/purge L1 cache of the region with invalid addresses panics.
	// However, cleaning/purging a large region above the threshold will clean/purge entire L1 cache(which doesn't panic).
	// That is why the following calls run against 256KB. 
	//We cannot do that on XScale L2 cache as it would generate page walk data abort.
	TInt invalid = chunkAddr;
	Cache::SyncMemoryBeforeDmaWrite(invalid+20, 0x40000-20);
	Cache::SyncMemoryAfterDmaRead(invalid+100,0x40000-101);
	#endif
	
	
	// The following calls operate against valid memory regions.
	Cache::SyncMemoryAfterDmaRead(valid+1, 0);
	Cache::SyncMemoryAfterDmaRead(valid+32, 12);
	Cache::SyncMemoryAfterDmaRead(valid+1, 0);
	Cache::SyncMemoryBeforeDmaWrite(valid+2, 1);
	Cache::SyncMemoryAfterDmaRead(valid+3, 2);
	Cache::SyncMemoryBeforeDmaWrite(valid+4, 3);
	Cache::SyncMemoryAfterDmaRead(valid+5, 4);
	Cache::SyncMemoryBeforeDmaWrite(valid+6, 5);
	Cache::SyncMemoryAfterDmaRead(valid+7, 6);
	Cache::SyncMemoryBeforeDmaWrite(valid+8, 7);
	Cache::SyncMemoryAfterDmaRead(valid+9, 8);
	Cache::SyncMemoryBeforeDmaWrite(valid+10, 9);
	Cache::SyncMemoryAfterDmaRead(valid+11, 10);
	Cache::SyncMemoryBeforeDmaWrite(valid+12, 11);
	Cache::SyncMemoryAfterDmaRead(valid+13, 12);
	Cache::SyncMemoryBeforeDmaWrite(valid+14, 13);
	Cache::SyncMemoryAfterDmaRead(valid+15, 14);

	TLinAddr page = (valid+2*pageSize);
	Cache::SyncMemoryBeforeDmaWrite(page, 0);
	Cache::SyncMemoryAfterDmaRead(page, 0);
	Cache::SyncMemoryBeforeDmaWrite(page-1, 2);
	Cache::SyncMemoryAfterDmaRead(page-2, 4);
	Cache::SyncMemoryBeforeDmaWrite(page-3, 6);
	Cache::SyncMemoryAfterDmaRead(page-4, 8);
	Cache::SyncMemoryBeforeDmaWrite(page-5, 10);
	Cache::SyncMemoryAfterDmaRead(page-6, 12);

	Cache::SyncMemoryBeforeDmaWrite(page, 2*pageSize);
	Cache::SyncMemoryAfterDmaRead(page-1, 2*pageSize);
	Cache::SyncMemoryBeforeDmaWrite(page+1, 2*pageSize);
	Cache::SyncMemoryAfterDmaRead(page+3, 2*pageSize);
	Cache::SyncMemoryBeforeDmaWrite(page-3, 2*pageSize);

	Cache::SyncMemoryBeforeDmaWrite(valid, 64, EMapAttrCachedMax);
	Cache::SyncMemoryBeforeDmaRead(valid, 64, EMapAttrCachedMax);
	Cache::SyncMemoryAfterDmaRead(valid, 64, EMapAttrCachedMax);

	
	Cache::IMB_Range(0, 0xffffffff);//will cause: Clean all DCache & Purge all ICache
	// Close the chunk
	NKern::ThreadEnterCS();
	Kern::ChunkClose(chunk);
	NKern::ThreadLeaveCS();


	//Check maintenance functions against entire cache (we need memory region >=8*cache size)
    info.iType         = TChunkCreateInfo::ESharedKernelSingle;
	info.iMaxSize      = 0x100000; //1MB will do
	info.iMapAttr      = EMapAttrSupRw | EMapAttrCachedWBWA | EMapAttrL2CachedWBWA | force_shared;
	info.iOwnsMemory   = ETrue; // Use memory from system's free pool
	info.iDestroyedDfc = NULL;

	NKern::ThreadEnterCS();
    if (KErrNone != (r = Kern::ChunkCreate(info, chunk, chunkAddr, mapAttr)))
		{
		NKern::ThreadLeaveCS();
		return r;
		}
	r = Kern::ChunkCommit(chunk,0x0,0x100000);
    if(r!=KErrNone)
        {
		Kern::ChunkClose(chunk);
		NKern::ThreadLeaveCS();
		return r;
		}
	NKern::ThreadLeaveCS();

	Cache::SyncMemoryBeforeDmaWrite(chunkAddr, 0x100000);
	Cache::SyncMemoryAfterDmaRead(chunkAddr, 0x100000);

	// Close the chunk
	NKern::ThreadEnterCS();
	Kern::ChunkClose(chunk);
	NKern::ThreadLeaveCS();

	return KErrNone;
	}


TInt DCacheTest::TestUseCase(TAny* a1)
	{
	TInt r = KErrNone;
	TInt time;
	
	RCacheTestDevice::TChunkTest info;
	kumemget(&info,a1,sizeof(info));

	TUint32 chunkAttr = EMapAttrSupRw;
#ifdef __SMP__
	TUint32 force_shared = EMapAttrShared;
#else
	TUint32 force_shared = 0;
#endif
	if (info.iShared) chunkAttr |= EMapAttrShared;

	switch (info.iCacheAttr)
		{
		case RCacheTestDevice::E_StronglyOrder:
			new (&chunkAttr) TMappingAttributes2(EMemAttStronglyOrdered,EFalse,ETrue,EFalse,info.iShared?ETrue:EFalse);
			break;
		case RCacheTestDevice::E_Device:
			new (&chunkAttr) TMappingAttributes2(EMemAttDevice,EFalse,ETrue,EFalse,info.iShared?ETrue:EFalse);
			break;
		case RCacheTestDevice::E_Normal_Uncached:
			new (&chunkAttr) TMappingAttributes2(EMemAttNormalUncached,EFalse,ETrue,EFalse,info.iShared?ETrue:EFalse);
			break;
		case RCacheTestDevice::E_Normal_Cached:
			new (&chunkAttr) TMappingAttributes2(EMemAttNormalCached,EFalse,ETrue,EFalse,info.iShared?ETrue:EFalse);
			break;
		#if defined(__CPU_MEMORY_TYPE_REMAPPING)
		case RCacheTestDevice::E_InOutWT_Remapped:
			Remap(info.iCacheAttr);
			new (&chunkAttr) TMappingAttributes2(EMemAttKernelInternal4,EFalse,ETrue,EFalse,(info.iShared|force_shared)?ETrue:EFalse);
		#else
		case RCacheTestDevice::E_InOutWT:		chunkAttr |= EMapAttrCachedWTRA|EMapAttrL2CachedWTRA|force_shared;
		#endif
			break;
		default:
			return KErrArgument;		
		}

	// Create chunk
	if (KErrNone!=(r=AllocPhysicalRam(Kern::RoundToPageSize(info.iSize)))) return r;
	if (KErrNone!=(r=CreateSharedChunk(chunkAttr, info.iActualMapAttr))) return r;
	
	//Alloc from the heap
	NKern::ThreadEnterCS();
	iHeap1 = (TInt*)Kern::Alloc(Max(info.iSize,0x8000));
	if (iHeap1==NULL) {NKern::ThreadLeaveCS();return KErrNoMemory;}
	iHeap2 = (TInt*)Kern::Alloc(0x8000);
	if (iHeap2==NULL) {Kern::Free((TAny*)iHeap1);NKern::ThreadLeaveCS();return KErrNoMemory;}
	NKern::ThreadLeaveCS();
	
	Cache::SyncMemoryAfterDmaRead(iChunkBase, info.iSize); // Invalidate (aka purge) cache.
	time = NKern::TickCount();
	switch(info.iUseCase)
		{
		case 0:  r = UseCase_ReadFromChunk(info);break;
		case 1:  r = UseCase_ReadFromChunk_ReadFromHeap(info);break;
		case 2:  r = UseCase_WriteToChunk(info);break;
		case 3:  r = UseCase_WriteToChunk_ReadFromHeap(info);break;
		default: r = KErrArgument;
		}
	info.iTime = NKern::TickCount() - time;

	NKern::ThreadEnterCS();
	Kern::Free((TAny*)iHeap1);
	Kern::Free((TAny*)iHeap2);
	NKern::ThreadLeaveCS();
	
	CloseSharedChunk();
	FreePhysicalRam();

	kumemput(a1,&info,sizeof(info));
	return r;
	}

TInt DCacheTest::UseCase_ReadFromChunk(RCacheTestDevice::TChunkTest& info)
	{
	TInt i;
	for (i=0; i< info.iLoops; i++)
		{
		//Simulate - evict the chunk from the cache)
		Cache::SyncMemoryBeforeDmaRead(iChunkBase, info.iSize, info.iActualMapAttr); // Invalidate (aka purge) cache.

		//Read DMA data
		memcpy((TAny*)iHeap1, (const TAny*)iChunkBase, info.iSize);
		//for (j=0; j < info.iSize>>2; j++) iDummy = *((TInt*)iChunkBase+j);
		}
	return KErrNone;
	}

TInt DCacheTest::UseCase_ReadFromChunk_ReadFromHeap(RCacheTestDevice::TChunkTest& info)
	{
	TInt i;
	for (i=0; i< info.iLoops; i++)
		{
		//Simulate - evict the chunk memory from the cache
		Cache::SyncMemoryBeforeDmaRead(iChunkBase, info.iSize, info.iActualMapAttr); // Invalidate (aka purge) cache.

		//Read DMA memory
		memcpy((TAny*)iHeap1, (const TAny*)iChunkBase, info.iSize);

		//Simulate Kernel activities - reading heap2
		memcpy((TAny*)iHeap1, (const TAny*)iHeap2, 0x8000);
		}
	return KErrNone;
	}

TInt DCacheTest::UseCase_WriteToChunk(RCacheTestDevice::TChunkTest& info)
	{
	TInt i;
	for (i=0; i< info.iLoops; i++)
		{
		//Simulate - evict the chunk memory from the cache
		Cache::SyncMemoryBeforeDmaRead(iChunkBase, info.iSize, info.iActualMapAttr); // Invalidate (aka purge) cache.

		//Write DMA memory
		memcpy((TAny*)iChunkBase, (const TAny*)iHeap1, info.iSize);
		Cache::SyncMemoryBeforeDmaWrite(iChunkBase, info.iSize, info.iActualMapAttr); // Clean cache.

		}
	return KErrNone;
	}

TInt DCacheTest::UseCase_WriteToChunk_ReadFromHeap(RCacheTestDevice::TChunkTest& info)
	{
	TInt i;
	for (i=0; i< info.iLoops; i++)
		{
		//Simulate - evict the chunk memory from the cache
		Cache::SyncMemoryBeforeDmaRead(iChunkBase, info.iSize, info.iActualMapAttr); // Invalidate (aka purge) cache.

		//Write DMA memory
		memcpy((TAny*)iChunkBase, (const TAny*)iHeap1, info.iSize);
		Cache::SyncMemoryBeforeDmaWrite(iChunkBase, info.iSize, info.iActualMapAttr); // Clean cache.
		
		//Simulate Kernel activities - reading heap2
		memcpy((TAny*)iHeap1, (const TAny*)iHeap2, 0x8000);
		}
	return KErrNone;
	}


// Entry point
TInt DCacheTest::Request(TInt aFunction, TAny* a1, TAny* a2)
	{
	TInt r = KErrNone;
#ifdef __SMP__
	TUint32 affinity = NKern::ThreadSetCpuAffinity(NKern::CurrentThread(), 0);
#endif
	switch (aFunction)
	{
		case RCacheTestDevice::EGetCacheInfo:				r = GetCacheInfo(a1);		break;
		case RCacheTestDevice::ETestDataChunk:				r = TestDataChunk(a1);		break;
		case RCacheTestDevice::ETestCodeChunk:				r = TestCodeChunk(a1);		break;
		case RCacheTestDevice::ETestWriteBackReadAllocate:	r = TestWriteBackMode(a1, EFalse);	break;
		case RCacheTestDevice::ETestWriteBackWriteAllocate:	r = TestWriteBackMode(a1, ETrue);	break;
		case RCacheTestDevice::ETesL2Maintenance:			r = TestL2Maintenance();	break;
		case RCacheTestDevice::EGetThreshold:				r = GetThreshold(a1);		break;
		case RCacheTestDevice::ESetThreshold:				r = SetThreshold(a1);		break;
		case RCacheTestDevice::ETestUseCase:				r = TestUseCase(a1);		break;
		default:											r=KErrNotSupported;
		}
#ifdef __SMP__
	NKern::ThreadSetCpuAffinity(NKern::CurrentThread(), affinity);
#endif
	return r;
	}

//////////////////////////////////////////
class DTestFactory : public DLogicalDevice
	{
public:
	DTestFactory();
	// from DLogicalDevice
	virtual TInt Install();
	virtual void GetCaps(TDes8& aDes) const;
	virtual TInt Create(DLogicalChannelBase*& aChannel);
	};

DTestFactory::DTestFactory()
    {
    iParseMask = KDeviceAllowUnit;
    iUnitsMask = 0x3;
    }

TInt DTestFactory::Create(DLogicalChannelBase*& aChannel)
    {
	CacheTestDriver = new DCacheTest;
	aChannel = CacheTestDriver;
	return (aChannel ? KErrNone : KErrNoMemory);
    }

TInt DTestFactory::Install()
    {
    return SetName(&KCacheTestDriverName);
    }

void DTestFactory::GetCaps(TDes8& /*aDes*/) const
    {
    }

DECLARE_STANDARD_LDD()
	{
    return new DTestFactory;
	}
