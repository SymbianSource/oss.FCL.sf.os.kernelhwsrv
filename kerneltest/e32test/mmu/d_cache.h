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
// e32test\mmu\d_cache.h
// 
//

#ifndef __D_CACHE_H__
#define __D_CACHE_H__

#include <e32cmn.h>
#ifndef __KERNEL_MODE__
#include <e32std.h>
#endif

_LIT(KCacheTestDriverName,"d_cache");
const TInt KMaxThresholdTypes = 5; // 1.Instruction/Unified, 2.Data, 3.AltD 4.Data_IMB  5.L210/L2XScale


const TInt KSingleCacheDescSize = 30;//Desc. of each particular cache. Must be *2 as t_cache converts TDes8 into TDes16.
const TInt KCacheDescSize = 200;//Desc. of the cache as a whole on the platforms. Must be *2 as t_cache converts TDes8 into TDes16.
const TInt KMaxCaches = 8; //Max number of caches. In theory, could be more but...
/**User side channel to Device Driver.*/
class RCacheTestDevice : public RBusLogicalChannel
	{
public:
	enum TControl
		{
		EGetCacheInfo,
		ETestDataChunk,
		ETestCodeChunk,
		ETestWriteBackReadAllocate,
		ETestWriteBackWriteAllocate,
		ETesL2Maintenance,
		EGetThreshold,
		ESetThreshold,
		ETestUseCase
		};

	struct TThresholdInfo
		{
		TInt iCacheType; 
		TUint32 iPurge; 
		TUint32 iClean; 
		TUint32 iFlush;
		};

	//Description of a single cache
	struct TCacheSingle
		{
		TInt8 iLevel;		// Cache level (1-8)
		TInt8 iData;		// 1 if it is data (or unified) cache, 0 otherwise
		TInt8 iCode;		// 1 if this is instruction (or unified) cache, 0 otherwise
		TInt iSize; 		// In bytes. Should be = iLineLength * iWays * iSets
		TInt iLineSize; 	// In bytes
		TInt iWays;			// Number of sets in cache
		TInt iSets;			// Number of ways in cache
		TBuf8<2*KSingleCacheDescSize> iDesc;	// Description, eg. L1_Instr_Inner L1_Data L2_Unified L210_Unified_Outer
		};

	struct TCacheInfo
		{
		TBuf8<2*KCacheDescSize> iDesc;	// General decription of the platform. E.g. '1176 with L210'.
		TInt iCacheCount;				// How many caches are there all together.
		TInt iMaxCacheSize;				// All caches considered. In bytes
		TInt iMemoryRemapping;			// 1 if memory remapping is ON.
		TInt iOuterCache;				// 1 if there is outer cache
		TInt iDmaBufferAlignment;		// Maximum size of any data cache line
		TCacheSingle iCache[KMaxCaches];// Info for each separate cache.
		};

	enum TCacheAttr//Specifies cache attributes 

		{   				//CacheAttr		BC(armv5)	TEX:BC(armv6)		memory_remapping(armv6K, armv7)
// Non-cached mapping
		E_FullyBlocking, 		//00 		00			SO/Shared			T0
		E_Buffered_NC,			//01		00			Device/Shared		T1
		E_Buffered_C, 			//02		01			Normal-Uncached		T1

// Inner cache
		E_InnerWT,				//05		10			Normal-InnerWT		T2
		E_InnerWBRA,			//06		11			Normal-InnerWB-RA	T3
		E_InnerWB,				//07		11			Normal-InnerWB-RAWA	T3
// Outer cache
		E_OuterWT,				//50		10			Normal/OuterWT		T2
		E_OuterWBRA,			//60		11			Normal/OuterBW-RA	T2
		E_OuterWB,				//70		11			Normal/OuterBW-RAWA	T2
// All cache
		E_InOutWT,				//55		10			Normal/FullyWT		T2
		E_InOutWBRA,			//66		11			Normal/FullyWB-RA	T3
		E_InOutWB,				//77		11			Normal/FullyWB-RAWA	T3

// Memory remapped attributes	
		E_StronglyOrder,		//--		T0/00		SO					T0
		E_Device,				//--		T0/00		Device				T1
		E_Normal_Uncached,		//--		T0/01		Normal/Uncached		T2
		E_Normal_Cached,		//--		T0/11		Normal/FullyBW		T3
		E_KernelInternal4,		//--		-----		---					T4
		E_PlatformSpecific5,	//--		-----		---					T5
		E_PlatformSpecific6,	//--		-----		---					T6
		E_PlatformSpecific7,	//--		-----		---					T7

// Inner cache
		E_InnerWT_Remapped,		//--		--			--					T4/Normal/InnerWT-OuterNC
		E_InnerWBRA_Remapped,	//--		--			--					T4/Normal/InnerWBRA-OuterNC
		E_InnerWB_Remapped,		//--		--			--					T4/Normal/InnerWB-OuterNC
// Outer cache
		E_OuterWT_Remapped,		//--		--			--					T4/Normal/InnerNC-OuterWT
		E_OuterWBRA_Remapped,	//--		--			--					T4/Normal/InnerNC-OuterWBRA
		E_OuterWB_Remapped,		//--		--			--					T4/Normal/InnerNC-OuterWBRA
// All cache
		E_InOutWT_Remapped,		//--		--			--					T4/Normal/InnerWT-OuterWT
		E_InOutWBRA_Remapped,	//--		--			--					T4/Normal/InnerWBRA-OuterWBRA
		E_InOutWB_Remapped,		//--		--			--					T4/Normal/InnerWB-OuterWB

// Kernel heap for data test / rom-image for code test
		E_Default,
		};
	
	struct TChunkTest
		{
		TChunkTest() {iShared=EFalse; iUseCase=0; iLoops=0;}
		TChunkTest(TInt aUseCase, TInt aSize, TInt aLoops) {iShared=EFalse;iUseCase=aUseCase;iLoops=aLoops;iSize=aSize;}
		
		TInt 		iSize;				//Defines the size of memory (either code or data) to test against.
		TCacheAttr	iCacheAttr;  		//Defines cache attributes of the memory to test.
		TBool		iShared;			//Defines the shared bit
		TInt		iUseCase;				//Specifies which test to execute.
		TInt		iLoops;				//The number of loops to execute.

		TUint32 	iActualMapAttr;		//The actual mapping attributes of the chunk. Will be set by the driver.
		TInt 		iTime; 				//The number of kernel ticks. Will be set by the driver. 
		};
public:
#ifndef __KERNEL_MODE__
	TInt Open()
		{return DoCreate(KCacheTestDriverName,TVersion(1,0,0),KNullUnit,NULL,NULL);}
	TInt GetCacheInfo (TCacheInfo& aCaches)
		{return DoControl(EGetCacheInfo, reinterpret_cast<TAny*>(&aCaches));}
	TInt TestDataChunk (TChunkTest& aChunkTest)
		{return DoControl(ETestDataChunk, reinterpret_cast<TAny*>(&aChunkTest));}
	TInt TestCodeChunk (TChunkTest& aChunkTest)
		{return DoControl(ETestCodeChunk, reinterpret_cast<TAny*>(&aChunkTest));}
	TInt TestWriteBackReadAllocate (TChunkTest& aChunkTest)
		{return DoControl(ETestWriteBackReadAllocate, reinterpret_cast<TAny*>(&aChunkTest));}
	TInt TestWriteBackWriteAllocate (TChunkTest& aChunkTest)
		{return DoControl(ETestWriteBackWriteAllocate, reinterpret_cast<TAny*>(&aChunkTest));}
	TInt TestL2Maintenance()
		{return DoControl(ETesL2Maintenance);}
	TInt GetThreshold(TThresholdInfo& aThreshold)
		{return DoControl(EGetThreshold, reinterpret_cast<TAny*>(&aThreshold));}
	TInt SetThreshold(TThresholdInfo& aThreshold)
		{return DoControl(ESetThreshold, reinterpret_cast<TAny*>(&aThreshold));}
	TInt TestUseCase (TChunkTest& aChunkTest)
		{return DoControl(ETestUseCase, reinterpret_cast<TAny*>(&aChunkTest));}
#endif //__KERNEL_MODE__
	};
#endif //__D_CACHE_H__
