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
// e32\include\memmodel\emul\win32\memmodel.h
// 
// WARNING: This file contains some APIs which are internal and are subject
//          to change without notice. Such APIs should therefore not be used
//          outside the Kernel and Hardware Services package.
//

/**
 @file
 @internalComponent
*/

#ifndef __WIN32_MEM_H__
#define __WIN32_MEM_H__
#include <win32.h>
#include <plat_priv.h>
#include <kernel/sshbuf.h>

const TInt KRamChunkSize=0x10000;
const TInt KRamChunkShift=16;
const TInt KRamPageSize=0x1000;
const TInt KRamPageShift=12;

/********************************************
 * Thread Control Block
 ********************************************/

class DWin32Thread : public DThread
	{
public:
	~DWin32Thread();
	virtual TInt Context(TDes8& aDes);
	virtual TInt SetupContext(SThreadCreateInfo& anInfo);
	virtual void DoExit2();
	};

/********************************************
 * Process Control Block
 ********************************************/
class DWin32CodeSeg;
class DWin32Chunk;
struct SProcessDllDataBlock;
class DWin32Process : public DProcess
	{
public:
	DWin32Process();
	~DWin32Process();
public:
	virtual TInt DoCreate(TBool aKernelProcess, TProcessCreateInfo& aInfo);
	virtual TInt NewChunk(DChunk*& aChunk, SChunkCreateInfo& aInfo, TLinAddr& aRunAddr);
	virtual TInt AddChunk(DChunk* aChunk,TBool isReadOnly);
	virtual TInt NewShPool(DShPool*& aPool, TShPoolCreateInfo& aInfo);
	virtual TInt CreateDataBssStackArea(TProcessCreateInfo& aInfo);
	virtual TInt GetNewThread(DThread*& aThread, SThreadCreateInfo& anInfo);
	virtual TInt AttachExistingCodeSeg(TProcessCreateInfo& aInfo);
	virtual TInt MapCodeSeg(DCodeSeg* aCodeSeg);
	virtual void UnmapCodeSeg(DCodeSeg* aCodeSeg);
	virtual void RemoveDllData();
	virtual void FinalRelease();
	virtual void Release();
	void CallRuntimeHook(TWin32RuntimeReason aReason);
public:
	TWin32RuntimeHook iWin32RuntimeHook;
	RArray<SProcessDllDataBlock> iDllData;
	};

/******************************************************************
 * structure to keep static data in code segments within a process
 ******************************************************************/
struct SProcessDllDataBlock
	{
	DWin32CodeSeg* iCodeSeg;
	TAny* iDataCopy;			// copy of .data
	TAny* iBssCopy;				// copy of .bss
	};

/********************************************
 * Chunk Control Block
 ********************************************/
class DWin32Chunk : public DChunk
	{
public:
	enum TMemModelChunkAttributes
		{
		EPrivate=0x10000000,

		EMMChunkAttributesMask = EPrivate,
		};

public:
	~DWin32Chunk();
public:
	virtual TInt DoCreate(SChunkCreateInfo& aInfo);
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
private:
	TInt DoCommit(TInt aOffset, TInt aSize);
	void DoDecommit(TInt aOffset, TInt aSize);
public:
	TBitMapAllocator* iPageBitMap;
	TBitMapAllocator* iUnlockedPageBitMap;
	TBitMapAllocator* iPermanentPageBitMap;
	};

/********************************************
 * Code segment
 ********************************************/
class DModuleList;
class DWin32CodeSeg: public DCodeSeg
	{
public:
	DWin32CodeSeg();
	virtual ~DWin32CodeSeg();
	TInt ProcessImports(DProcess* aProcess);
	TInt CreateAlreadyLoaded(HMODULE aModule, TInt aDepCount);
	TInt RegisterCodeSeg(HMODULE aModule);
	TInt CopyDataBss();
public:
	virtual TLibraryFunction Lookup(TInt aOrdinal);
	virtual TInt GetMemoryInfo(TModuleMemoryInfo& aInfo, DProcess* aProcess);
	virtual TInt DoCreate(TCodeSegCreateInfo& aInfo, DProcess* aProcess);
	virtual TInt Loaded(TCodeSegCreateInfo& aInfo);
	virtual void InitData();
	virtual void ReadExportDir(TUint32* aDest);
	virtual TBool FindCheck(DProcess* aProcess);
	virtual TBool OpenCheck(DProcess* aProcess);
	virtual void Info(TCodeSegCreateInfo& aInfo);
public:
	HINSTANCE iWinInstance;
	HMODULE iModuleHandle;
	wchar_t* iModuleFile;
	TBool iAlwaysLoaded;		// TRUE for variant or extension
	DModuleList* iModuleList;
	TAny* iDataCopy;			// copy of .data
	TInt iRealDataSize;
	TInt iRealBssSize;
	TLinAddr iDataDest;			// load address of .data
	TLinAddr iBssDest;			// load address of .bss
	TInt iCodeSegId;			// unique ID, incremented each time a code segment is loaded
	DWin32Process* iLiveProcess;// process who's static data is currently loaded in the codeseg
	};

struct SWin32Module
	{
	TLinAddr iWin32ModuleHandle;
	DWin32CodeSeg* iCodeSeg;
	};

	
/********************************************
 * Functions/Data defined in memory model
 ********************************************/

class MM
	{
public:
	enum TMemModelPanic
		{
		EKernelHeapReserveFailed = 0,
		EKernelHeapCommitFailed = 1,
		ERamAllocMutexCreateFailed = 2,
		EInvalidChunkCreate = 3,
		EInvalidSharedModule = 4,
		ECompileDepLists=5,
		EWin32RuntimeError=6,
		ENotSupportedOnEmulator=7,
		EWsdBadReserve=8,
		EWsdDllNotInProcess=9,
    	};

	static void Panic(TMemModelPanic aPanic);
public:
	static void Init1();
	static void Wait();
	static void Signal();
	static TUint32 RoundToPageSize(TUint32 aSize);
	static TUint32 RoundToChunkSize(TUint32 aSize);
	static TInt RegisterModule(HMODULE aModule);
	static TInt Commit(TLinAddr aBase, TInt aSize, TInt aClearByte, TBool aExecute);
	static TInt Decommit(TLinAddr aBase, TInt aSize);
	static void CheckMemoryCounters();
	static void DoProcessSwitch(TAny* aAddressSpace);
	static TAny* CurrentAddress(DThread* aThread, const TAny* aPtr, TInt aSize, TBool aWrite, TBool& aLocked);
public:
	static TAny* KernelHeapAddress;
	static DMutex* RamAllocatorMutex;
	static TInt RamChunkSize;
	static TInt RamChunkShift;
	static TInt RamPageSize;
	static TInt RamPageShift;
	static TInt FreeMemory;	// number of bytes in the system free memory 
	static TInt CacheMemory; // number of bytes of memory being used for cache chunks (RChunk::Unlock)
	static TInt ReclaimedCacheMemory; // number of bytes of memory removed from CacheMemory in order to satisfy memory allocation
	static TInt InitialFreeMemory;
	static TBool AllocFailed;
	static RArray<SWin32Module> Win32Modules;
	static TInt NextCodeSegId;
	};


/********************************************
 * Shared buffers and pools
 ********************************************/

class DWin32ShBuf : public DShBuf
	{
public:
	DWin32ShBuf(DShPool* aPool, TLinAddr aRelAddr);
	~DWin32ShBuf();

	TInt AddToProcess(DProcess* aProcess, TUint aAttr);

	TInt Close(TAny* aPtr);

protected:
	virtual TInt Map(TUint, DProcess*, TLinAddr&);
	virtual TInt UnMap(DProcess*);
	virtual TUint8* Base(DProcess* aProcess);
	virtual TUint8* Base();

private:
	TBool iMapped;
	};


class DWin32ShPool : public DShPool
	{
public:
	DWin32ShPool();
	virtual ~DWin32ShPool();

	TInt Close(TAny* aPtr);
	TInt AddToProcess(DProcess* aProcess, TUint aAttr);
	TInt Alloc(DShBuf*&);

protected:
	TInt DoCreate(TShPoolCreateInfo& aInfo);
	TInt AddToFreeList(TInt aOffset);
	TInt CreateInitialBuffers();
	TInt DeleteInitialBuffers();
	TInt DestroyHandles(DProcess* aProcess);
	void DestroyClientResources(DProcess* aProcess);

	void Free(DShBuf* aBuf);
	TInt UpdateFreeList();

	TUint8* Base();
	TUint8* Base(DProcess* aProcess);
	TBool IsOpen(DProcess* aProcess);

	TBitMapAllocator* iBufMap;
	DWin32ShBuf* iInitialBuffersArray;
	TUint8* iWin32MemoryBase;
	TInt iWin32MemorySize;
	friend class DWin32ShBuf;
	};


class DWin32AlignedShPool : public DWin32ShPool
	{
public:
	DWin32AlignedShPool();
	virtual ~DWin32AlignedShPool();
	TInt SetBufferWindow(DProcess* aProcess, TInt aWindowSize);

private:
	TInt DoCreate(TShPoolCreateInfo& aInfo);
	TInt GrowPool();
	TInt ShrinkPool();
	};


class DWin32NonAlignedShPool : public DWin32ShPool
	{
public:
	DWin32NonAlignedShPool();
	virtual ~DWin32NonAlignedShPool();

private:
	TInt DoCreate(TShPoolCreateInfo& aInfo);
	void FreeBufferPages(TUint aOffset);
	TInt GrowPool();
	TInt ShrinkPool();

	TBitMapAllocator* iPagesMap;
	};

#endif	// __WIN32_MEM_H__
