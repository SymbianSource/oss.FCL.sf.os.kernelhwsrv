// Copyright (c) 1998-2009 Nokia Corporation and/or its subsidiary(-ies).
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
// e32\include\memmodel\epoc\direct\memmodel.h
// 
// WARNING: This file contains some APIs which are internal and are subject
//          to change without notice. Such APIs should therefore not be used
//          outside the Kernel and Hardware Services package.
//

#ifndef __MEMMODEL_H__
#define __MEMMODEL_H__
#include <plat_priv.h>

/********************************************
 * Process Control Block
 ********************************************/

class DMemModelChunk;

/**
@internalComponent
*/
class DMemModelProcess : public DEpocProcess
	{
public:
	void Destruct();
public:
	virtual TInt DoCreate(TBool aKernelProcess, TProcessCreateInfo& aInfo);
	virtual TInt NewChunk(DChunk*& aChunk, SChunkCreateInfo& anInfo, TLinAddr& aRunAddr);
	virtual TInt AddChunk(DChunk* aChunk,TBool isReadOnly);
	virtual TInt NewShPool(DShPool*& aPool, TShPoolCreateInfo& aInfo);
	virtual TInt CreateDataBssStackArea(TProcessCreateInfo& aInfo);
	virtual TInt MapCodeSeg(DCodeSeg* aCodeSeg);
	virtual void UnmapCodeSeg(DCodeSeg* aCodeSeg);
	virtual void RemoveDllData();
	virtual void FinalRelease();
public:
	friend class Monitor;
	};

/********************************************
 * Chunk Control Block
 ********************************************/

/**
@internalComponent
*/
class DMemModelChunk : public DChunk
	{
public:
	enum TMemModelChunkAttributes
		{
		EPrivate=0x10000000,
	
		EMMChunkAttributesMask = EPrivate,
		};

public:
	~DMemModelChunk();
public:
//	virtual TInt Close(TAny* aPtr);
	virtual TInt DoCreate(SChunkCreateInfo& anInfo);
	virtual TInt Adjust(TInt aNewSize);
	virtual TInt AdjustDoubleEnded(TInt aBottom, TInt aTop);
	virtual TInt CheckAccess();
	virtual TInt Commit(TInt anOffset, TInt aSize, TCommitType aCommitType=DChunk::ECommitDiscontiguous, TUint32* aExtraArg=0);
	virtual TInt Allocate(TInt aSize, TInt aGuard=0, TInt aAlign=0);
	virtual TInt Decommit(TInt anOffset, TInt aSize);
	virtual TInt Lock(TInt anOffset, TInt aSize);
	virtual TInt Unlock(TInt anOffset, TInt aSize);
	virtual TInt Address(TInt aOffset, TInt aSize, TLinAddr& aKernelAddress);
	virtual TInt PhysicalAddress(TInt aOffset, TInt aSize, TLinAddr& aKernelAddress, TUint32& aPhysicalAddress, TUint32* aPhysicalPageList=NULL);
	virtual void BTracePrime(TInt aCategory);
	virtual void Substitute(TInt aOffset, TPhysAddr aOldAddr, TPhysAddr aNewAddr);
	virtual TUint8* Base(DProcess* aProcess);
	inline TUint8* Base() const { return DChunk::Base(); }
public:
	void SetFixedAddress(TLinAddr anAddr, TInt aSize);
public:
	TLinAddr iRegionBase;
	TInt iRegionSize;
public:
	friend class Monitor;
	};

/********************************************
 * Code segment
 ********************************************/

/**
@internalComponent
*/
class DMemModelCodeSegMemory : public DEpocCodeSegMemory
	{
public:
	DMemModelCodeSegMemory(DEpocCodeSeg* aCodeSeg);
	};

/**
@internalComponent
*/
class DMemModelCodeSeg: public DEpocCodeSeg
	{
public:
	DMemModelCodeSeg();
	virtual ~DMemModelCodeSeg();
	virtual TInt DoCreateRam(TCodeSegCreateInfo& aInfo, DProcess* aProcess);
	virtual TInt DoCreateXIP(DProcess* aProcess);
	virtual TInt Loaded(TCodeSegCreateInfo& aInfo);
	virtual void ReadExportDir(TUint32* aDest);
	virtual TBool FindCheck(DProcess* aProcess);
	virtual TBool OpenCheck(DProcess* aProcess);
public:
	TLinAddr iDataAlloc;
	TAny* iKernelData;			// only for kernel modules
	};

/********************************************
 * Functions/Data defined in memory model
 ********************************************/

/**
@internalComponent
*/
class MM
	{
public:
	enum TMemModelPanic
		{
		ERamAllocCreateFailed=0,
		ERamAllocMutexCreateFailed=1,
		EFreeInvalidRegion=2,
		ECodeSegCheckInconsistent=3,
		ESecAllocCreateFailed=4,
		EUnsupportedOperation=5,
		};

	static void Panic(TMemModelPanic aPanic);
public:
	static void Init1();
	static void StartCrashDebugger();
	static void WaitRamAlloc();
	static void SignalRamAlloc();
	static TUint32 RoundToBlockSize(TUint32 aSize);
	static void FreeRegion(TLinAddr aBase, TInt aSize);
	static TInt AllocRegion(TLinAddr& aBase, TInt aSize, TInt aAlign=0);
	static TInt ClaimRegion(TLinAddr aBase, TInt aSize);
	static TInt AllocContiguousRegion(TLinAddr& aBase, TInt aSize, TInt aAlign=0);
	static TInt BlockNumber(TPhysAddr aAddr);
public:
	static TLinAddr UserDataSectionBase;
	static TLinAddr UserRomDataSectionEnd;
	static TLinAddr UserDataSectionEnd;
	static TLinAddr RomLinearBase;
	static DMutex* RamAllocatorMutex;
	static TBitMapAllocator* RamAllocator;
	static TBitMapAllocator* SecondaryAllocator;
	static TInt RamBlockSize;
	static TInt RamBlockShift;
	static TInt InitialFreeMemory;
	static TBool AllocFailed;
	};

#endif
