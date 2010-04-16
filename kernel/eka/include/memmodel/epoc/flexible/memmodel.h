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
// e32/include/memmodel/epoc/flexible/memmodel.h
// Flexible Memory Model header file

/**
 @file
 @internalComponent
*/


#ifndef __MEMMODEL_H__
#define __MEMMODEL_H__

#include <plat_priv.h>
#include <memmodel/epoc/mmubase/kblockmap.h>
#include <mmtypes.h>
#include <mmboot.h>

#ifdef __SMP__
// SubScheduler fields for each processor
#define	i_AliasLinAddr			iSubSchedScratch[0]
#define	i_AliasPdePtr			iSubSchedScratch[1]
#endif

/********************************************
 * Deterministic Scheduler Implementation
 ********************************************/

/**
@internalComponent
*/
#define TheCurrentAddressSpace			((DMemModelProcess*&)TheScheduler.iAddressSpace)

class DMemoryObject;
class DMemoryMapping;

/********************************************
 * Thread Control Block
 ********************************************/

class DMemModelProcess;

/**
@internalComponent
*/
class DMemModelThread : public DThread
	{
public:
	TInt Alias(TLinAddr aAddr, DMemModelProcess* aProcess, TInt aSize, TLinAddr& aAliasAddr, TUint& aAliasSize);
	void RemoveAlias();
	void RefreshAlias();
	virtual void DoExit1();
	static void RestoreAddressSpace();
 	virtual void BTracePrime(TInt aCategory);
protected:
	virtual void SetPaging(TUint& aCreateFlags);
private:
	void DoRemoveAlias(TLinAddr aAddr);
public:
	TLinAddr iAliasLinAddr;	// linear address to access aliased memory (0 means no alias is present).
	TPde* iAliasPdePtr;		// Address of PDE which has been modified to make aliased memory accessible.
	TPde iAliasPde;			// PDE to store at iAliasPdePtr.
	DMemModelProcess* iAliasProcess;		// The process whose memory is aliased.
	SDblQueLink iAliasLink;	// link to make TheMmu.iAliasList.
	TLinAddr iAliasTarget;	// linear address of the memory which has been aliased
	DMemoryMapping* iKernelStackMapping;
	DMemoryMapping* iUserStackMapping;
#ifdef __SMP__
	TInt iCpuRestoreCookie;
#endif
	};


/********************************************
 * Process Control Block
 ********************************************/

class DMemModelChunk;
class DMemModelCodeSegMemory;
class RAddressedContainer;

#ifdef INCLUDED_FROM_ASM
#define PRIVATE_EXCEPT_ASM public
#else
#define PRIVATE_EXCEPT_ASM private
#endif


/**
@internalComponent
*/
class DMemModelProcess : public DEpocProcess
	{
public:
	~DMemModelProcess();
private:
	void Destruct();
protected:
	virtual TInt AttachExistingCodeSeg(TProcessCreateInfo& aInfo);
	virtual TInt SetPaging(const TProcessCreateInfo& aInfo);
public:
	virtual TInt DoCreate(TBool aKernelProcess, TProcessCreateInfo& aInfo);
	virtual TInt NewChunk(DChunk*& aChunk, SChunkCreateInfo& aInfo, TLinAddr& aRunAddr);
	virtual TInt AddChunk(DChunk* aChunk,TBool aIsReadOnly);
	virtual TInt NewShPool(DShPool*& aPool, TShPoolCreateInfo& aInfo);
	virtual TInt CreateDataBssStackArea(TProcessCreateInfo& aInfo);
	virtual TInt MapCodeSeg(DCodeSeg* aCodeSeg);
	virtual void UnmapCodeSeg(DCodeSeg* aCodeSeg);
	virtual void RemoveDllData();
	virtual void FinalRelease();
	virtual void BTracePrime(TInt aCategory);
public:
	TInt DoAddChunk(DMemModelChunk* aChunk, TBool aIsReadOnly);
	TInt AllocateDataSectionBase(DMemModelChunk& aChunk, TUint& aBase);
	TUint8* DataSectionBase(DMemModelChunk* aChunk);
	void RemoveChunk(DMemModelChunk *aChunk);
	void DoRemoveChunk(TInt aIndex);
	TInt ChunkIndex(DMemModelChunk* aChunk);
	TUint ChunkInsertIndex(DMemModelChunk* aChunk);
	TInt CommitDllData(TLinAddr aBase, TInt aSize, DCodeSeg* aCodeSeg);
	void DecommitDllData(TLinAddr aBase, TInt aSize);
	TInt MapUserRamCode(DMemModelCodeSegMemory* aMemory);
	void UnmapUserRamCode(DMemModelCodeSegMemory* aMemory);
	inline TInt OsAsid()
		{__NK_ASSERT_DEBUG(	TheCurrentThread->iOwningProcess == this || // current thread's process so asid can't be freed.
							iOsAsid == (TInt)KKernelOsAsid ||	// kernel process so asid can't be freed.
							iContainerID != EProcess ||	// process not fully created yet so asid can't be freed.
							iOsAsidRefCount > 1);		// if none of the others are true then should have a reference 
														// to prevent asid being freed (this check isn't very 
														// robust but best we can do).
		return iOsAsid;
		};

	TInt TryOpenOsAsid();
	void CloseOsAsid();
	void AsyncCloseOsAsid();
public:
	struct SChunkInfo
		{
		DMemModelChunk* iChunk;
		DMemoryMapping* iMapping;
		TInt16 iAccessCount;
		TInt16 iIsReadOnly;
		};

	TInt iChunkCount;
	TInt iChunkAlloc;
	SChunkInfo* iChunks;
	RAddressedContainer* iSharedChunks;
	TPhysAddr iPageDir;
	DMemoryMapping* iDataBssMapping;
	/**
	Size of virtual memory allocated in process for EXE code, but which hasn't yet been been
	adopted by the codeseg memory mapping.
	*/
	TUint iCodeVirtualAllocSize;
	/**
	Address of virtual memory allocated in process for EXE code.
	*/
	TLinAddr iCodeVirtualAllocAddress;

PRIVATE_EXCEPT_ASM:
	TInt iOsAsid;		// This should only be accessed directly by the scheduler, 
						// in all other cases use either OsAsid() or TryOpenOsAsid().
private:
	TUint iOsAsidRefCount;
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
	/**
	@see DChunk::TChunkAttributes for generic attribute flags
	*/
	enum TMemModelChunkAttributes
		{
		EPrivate			=0x00000000, // need to be iOwningProcess in order to map or change chunk
		EPublic				=0x80000000, // not EPrivate
		ECode				=0x40000000, // contents are executable
		EMMChunkAttributesMask = EPrivate | EPublic | ECode,
		};
	
public:
	DMemModelChunk();
	~DMemModelChunk();
public:
	virtual TInt Close(TAny* aPtr);
	virtual TInt DoCreate(SChunkCreateInfo& aInfo);
	virtual TInt Adjust(TInt aNewSize);
	virtual TInt AdjustDoubleEnded(TInt aBottom, TInt aTop);
	virtual TInt CheckAccess();
	virtual TInt Commit(TInt aOffset, TInt aSize, TCommitType aCommitType=DChunk::ECommitDiscontiguous, TUint32* aExtraArg=0);
	virtual TInt Allocate(TInt aSize, TInt aGuard=0, TInt aAlign=0);
	virtual TInt Decommit(TInt aOffset, TInt aSize);
	virtual TInt Lock(TInt aOffset, TInt aSize);
	virtual TInt Unlock(TInt aOffset, TInt aSize);
	virtual TInt Address(TInt aOffset, TInt aSize, TLinAddr& aKernelAddress);
	virtual TInt PhysicalAddress(TInt aOffset, TInt aSize, TLinAddr& aKernelAddress, TUint32& aPhysicalAddress, TUint32* aPhysicalPageList=NULL);
	virtual void BTracePrime(TInt aCategory);
	virtual void Substitute(TInt aOffset, TPhysAddr aOldAddr, TPhysAddr aNewAddr);
	virtual TUint8* Base(DProcess* aProcess);
protected:
	virtual void SetPaging(TUint aCreateAtt);
public:
	void SetFixedAddress(TLinAddr aAddr, TInt aInitialSize);
	TInt SetAttributes(SChunkCreateInfo& aInfo);
	TInt DoCommit(TInt aOffset, TInt aSize, TCommitType aCommitType=DChunk::ECommitDiscontiguous, TUint32* aExtraArg=0);
	void DoDecommit(TInt aOffset, TInt aSize);
	TInt CheckRegion(TInt& aOffset, TInt& aSize);
public:
	TBitMapAllocator* iPageBitMap;		// NULL if not disconnected chunk
	TBitMapAllocator* iPermanentPageBitMap;

	/**
	The memory object containing this chunk's memory.
	*/
	DMemoryObject* iMemoryObject;

	/**
	For shared chunks and shared i/o buffers this is the mapping
	which maps #iMemoryObject into the kernel's address space.
	*/
	DMemoryMapping* iKernelMapping;
public:
	friend class Monitor;
	};


/**
@internalComponent
*/
class DMemModelChunkHw : public DPlatChunkHw
	{
public:
	virtual TInt Close(TAny* aPtr);
public:
	DMemoryObject* iMemoryObject;
	DMemoryMapping* iKernelMapping;
	};


	
/********************************************
 * Code segment
 ********************************************/

class TPagedCodeInfo;

/**
@internalComponent
*/
class DMemModelCodeSegMemory : public DEpocCodeSegMemory
	{
public:
	DMemModelCodeSegMemory(DEpocCodeSeg* aCodeSeg);
	~DMemModelCodeSegMemory();
	TInt Create(TCodeSegCreateInfo& aInfo, DMemModelProcess* aProcess);
	TInt Loaded(TCodeSegCreateInfo& aInfo);
	void Destroy();
public:
	/**
	The process loading this code segment.
	*/
	DMemModelProcess* iCreator;

	/**
	Kernel side copy of the codeseg's export directory or NULL.
	*/
	TLinAddr* iCopyOfExportDir;

	/**
	For demand paged codeseg's, pointer to saved copy of the data section.
	*/
	TAny* iDataSectionMemory;

	/**
	The memory object containing the memory for the codeseg.
	*/
	DMemoryObject* iCodeMemoryObject;

	/**
	A writable mapping which maps #iCodeMemoryObject into the loader
	process's memory so it can be accessed by the loader.

	This mapping is destroyed once #Loaded is called.
	*/
	DMemoryMapping* iCodeLoadMapping;

	/**
	For demand paged codeseg's, this is a writable mapping added to the
	loader process's address space which maps a memory object used to
	hold the initial contents of the codeseg's data section.

	This mapping, and it's memory object, is destroyed once #Loaded is called.
	*/
	DMemoryMapping* iDataLoadMapping;

	/**
	Size of any shared virtual memory allocated for the code.
	*/
	TUint iVirtualAllocCommonSize;

	/**
	For demand paged codeseg's, a pointer to the #TPagedCodeInfo
	used with #iCodeMemoryObject.
	*/
	TPagedCodeInfo* iPagedCodeInfo;
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
	inline DMemModelCodeSegMemory* Memory()
		{ return (DMemModelCodeSegMemory*)iMemory; }
	virtual void BTracePrime(TInt aCategory);
public:
	/**
	For kernel codesegs, the address of the memory allocated for its static data.
	*/
	TAny* iKernelData;

	/**
	The memory object containing the memory for this codeseg.
	*/
	DMemoryObject* iCodeMemoryObject;

	/**
	A writable mapping which maps #iCodeMemoryObject into the loader
	process's memory so it can be accessed by the loader.

	This mapping is destroyed once #Loaded is called.
	*/
	DMemoryMapping* iCodeLoadMapping;

	/**
	The read=only mapping used for kernel and global codesegs to map
	#iCodeMemoryObject into the global address region.
	*/
	DMemoryMapping* iCodeGlobalMapping;

	/**
	Base address of virtual memory allocated for the codeseg's static data.
	*/
	TLinAddr iDataAllocBase;	

	/**
	Size of virtual memory allocated for the codeseg's static data.
	*/
	TUint iDataAllocSize;
	};


/********************************************
 * Shared buffers and pools
 ********************************************/

#include <kernel/sshbuf.h>

class DShBufMapping;


class DMemModelNonAlignedShBuf : public DShBuf
	{
public:
	DMemModelNonAlignedShBuf(DShPool* aPool, TLinAddr aRelAddr);
	~DMemModelNonAlignedShBuf();

	TInt Close(TAny*);
	TInt AddToProcess(DProcess* aProcess, TUint aAttr);

protected:
	virtual TInt Pin(TPhysicalPinObject* aPinObject, TBool aReadOnly, TPhysAddr& aAddress, TPhysAddr* aPages, TUint32& aMapAttr, TUint& aColour);
	virtual TInt Map(TUint, DProcess*, TLinAddr&);
	virtual TInt UnMap(DProcess*);
	virtual TUint8* Base(DProcess* aProcess);
	virtual TUint8* Base();
	};


class DMemModelAlignedShBuf : public DShBuf
	{
public:
	DMemModelAlignedShBuf(DShPool* aPool);
	virtual ~DMemModelAlignedShBuf();

	TInt Create();
	virtual TInt Construct();

	TInt Close(TAny*);
	TInt AddToProcess(DProcess* aProcess, TUint aAttr);

protected:
	TUint8* Base(DProcess* aProcess);
	TUint8* Base();
	TInt Map(TUint, DProcess*, TLinAddr&);
	TInt UnMap(DProcess*);
	TInt Pin(TPhysicalPinObject* aPinObject, TBool aReadOnly, TPhysAddr& Address, TPhysAddr* aPages, TUint32& aMapAttr, TUint& aColour);

private:
	TInt FindMapping(DShBufMapping*&, DMemModelProcess*);
	DMemoryObject* iMemoryObject;
	DMemoryMapping* iKernelMapping;
	SDblQue iMappings;
	friend class DMemModelAlignedShPool;
	};

class DMemModelShPool : public DShPool
	{
public:
	DMemModelShPool();
	virtual ~DMemModelShPool();
protected:
	void DestroyClientResources(DProcess* aProcess);
	virtual TInt DestroyAllMappingsAndReservedHandles(DProcess* aProcess) = 0;
	};

class DMemModelAlignedShPoolClient : public DShPoolClient
	{
public:
	SDblQue iMappingFreeList;
	TInt iWindowSize;
	};

class DMemModelNonAlignedShPoolClient : public DShPoolClient
	{
public:
	DMemoryMapping* iMapping;
	};

class DShBufMapping;

class DMemModelAlignedShPool : public DMemModelShPool
	{
public:
	DMemModelAlignedShPool();
	virtual ~DMemModelAlignedShPool();

	TInt Close(TAny* aPtr);
	TInt CreateInitialBuffers();
	TInt SetBufferWindow(DProcess* aProcess, TInt aWindowSize);
	TInt AddToProcess(DProcess* aProcess, TUint aAttr);
	TInt Alloc(DShBuf*&);

private:
	TInt DoCreate(TShPoolCreateInfo& aInfo);
	TInt GrowPool();
	TInt ShrinkPool();
	TInt MappingNew(DShBufMapping*& aMapping, DMemModelProcess* aProcess);
	TInt GetFreeMapping(DShBufMapping*& aMapping, DMemModelAlignedShPoolClient* aClient);
	TInt ReleaseMapping(DShBufMapping*& aMapping, DMemModelAlignedShPoolClient* aClient);
	TInt UpdateMappingsAndReservedHandles(TInt aNoOfBuffers);
	TInt UpdateFreeList();
	void Free(DShBuf* aBuf);
	TInt CreateMappings(DMemModelAlignedShPoolClient* aClient, TInt aNoOfMappings, DMemModelProcess* aProcess);
	TInt DestroyAllMappingsAndReservedHandles(DProcess* aProcess);
	TInt DestroyMappings(DMemModelAlignedShPoolClient* aClient, TInt aNoOfMappings);

	TInt DeleteInitialBuffers();

	SDblQue iPendingList;
	DMemModelAlignedShBuf* iInitialBuffersArray;

	friend class DMemModelAlignedShBuf;
	};

class DMemModelNonAlignedShPool : public DMemModelShPool
	{
public:
	DMemModelNonAlignedShPool();
	virtual ~DMemModelNonAlignedShPool();

	TInt Close(TAny* aPtr);

	TInt DoInitFreeList();
	TUint8* Base(DProcess* aProcess);

	inline TUint8* Base()
		{
		return reinterpret_cast<TUint8*>(iBaseAddress);
		};
	TInt AddToProcess(DProcess* aProcess, TUint aAttr);
	TInt CreateInitialBuffers();

	TInt Alloc(DShBuf*&);

private:
	void Free(DShBuf* aBuf);
	TInt UpdateFreeList();

	TInt DoCreate(TShPoolCreateInfo& aInfo);
	void FreeBufferPages(TUint aOffset);
	TInt GrowPool();
	TInt ShrinkPool();
	TInt DeleteInitialBuffers();
	TInt DestroyAllMappingsAndReservedHandles(DProcess* aProcess);

private:
	TLinAddr iBaseAddress;
	DMemoryObject* iMemoryObject;		// the 'real' memory pool (in the kernel)
	DMemModelNonAlignedShBuf* iInitialBuffersArray;
	TBitMapAllocator* iBufMap;
	TBitMapAllocator* iPagesMap;

	friend class DMemModelNonAlignedShBuf;
	};

#endif
