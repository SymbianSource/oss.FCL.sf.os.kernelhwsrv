// Copyright (c) 1995-2009 Nokia Corporation and/or its subsidiary(-ies).
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
//

#include <memmodel.h>
#include "mmu/mm.h"
#include "mmboot.h"
#include "mmu/mcodepaging.h"

#include "cache_maintenance.h"


DCodeSeg* M::NewCodeSeg(TCodeSegCreateInfo&)
	{
	__KTRACE_OPT(KDLL,Kern::Printf("M::NewCodeSeg"));
	return new DMemModelCodeSeg;
	}


//
// DMemModelCodeSegMemory
//

DEpocCodeSegMemory* DEpocCodeSegMemory::New(DEpocCodeSeg* aCodeSeg)
	{
	return new DMemModelCodeSegMemory(aCodeSeg);
	}


DMemModelCodeSegMemory::DMemModelCodeSegMemory(DEpocCodeSeg* aCodeSeg)
	: DEpocCodeSegMemory(aCodeSeg)
	{
	}


TInt DMemModelCodeSegMemory::Create(TCodeSegCreateInfo& aInfo, DMemModelProcess* aProcess)
	{
	TInt r;

	TUint codePageCount;
	TUint dataPageCount;
	TBool isDemandPaged;
	if(!aInfo.iUseCodePaging)
		{
		isDemandPaged = 0;
		codePageCount = MM::RoundToPageCount(iRamInfo.iCodeSize+iRamInfo.iDataSize);
		dataPageCount = 0;
		}
	else
		{
		isDemandPaged = 1;
		codePageCount = MM::RoundToPageCount(iRamInfo.iCodeSize);
		dataPageCount = MM::RoundToPageCount(iRamInfo.iDataSize);

		iDataSectionMemory = Kern::Alloc(iRamInfo.iDataSize);
		if(!iDataSectionMemory)
			return KErrNoMemory;
		}

	iCodeSeg->iSize = codePageCount<<KPageShift;

	// allocate virtual address for code to run at...
	const TUint codeSize = codePageCount<<KPageShift;
	if(iCodeSeg->IsExe())
		{// Get the os asid without opening a reference on it as aProcess isn't fully 
		// created yet so won't free its os asid.
		r = MM::VirtualAlloc(aProcess->OsAsid(),iRamInfo.iCodeRunAddr,codeSize,isDemandPaged);
		if(r!=KErrNone)
			return r;
		aProcess->iCodeVirtualAllocSize = codeSize;
		aProcess->iCodeVirtualAllocAddress = iRamInfo.iCodeRunAddr;
		iCodeSeg->iAttr |= ECodeSegAttAddrNotUnique;
		}
	else
		{
		r = MM::VirtualAllocCommon(iRamInfo.iCodeRunAddr,codeSize,isDemandPaged);
		if(r!=KErrNone)
			return r;
		iVirtualAllocCommonSize = codeSize;
		}

	// create memory object for codeseg...
	if(isDemandPaged)
		{
		// create memory object...
		r = MM::PagedCodeNew(iCodeMemoryObject, codePageCount, iPagedCodeInfo);
		if(r!=KErrNone)
			return r;

		// get file blockmap for codeseg contents...
		r = iPagedCodeInfo->ReadBlockMap(aInfo);
		if (r != KErrNone)
			return r;
		}
	else
		{
		// create memory object...
		TMemoryCreateFlags flags = (TMemoryCreateFlags)(EMemoryCreateNoWipe | EMemoryCreateAllowExecution);
		r = MM::MemoryNew(iCodeMemoryObject, EMemoryObjectMovable, codePageCount, flags);
		if(r!=KErrNone)
			return r;

		// commit memory...
		r = MM::MemoryAlloc(iCodeMemoryObject,0,codePageCount);
		if(r!=KErrNone)
			return r;
		}

	// create a mapping of the memory for the loader...
	// No need to open reference on os asid it is the current thread/process's.
	DMemModelProcess* pP = (DMemModelProcess*)TheCurrentThread->iOwningProcess;
	r = MM::MappingNew(iCodeLoadMapping,iCodeMemoryObject,EUserReadWrite,pP->OsAsid());
	if(r!=KErrNone)
		return r;

	iRamInfo.iCodeLoadAddr = MM::MappingBase(iCodeLoadMapping);

	// work out where the loader is to put the loaded data section...
	TInt loadSize = iRamInfo.iCodeSize; // size of memory filled by loader
	if(iRamInfo.iDataSize)
		{
		if(!dataPageCount)
			{
			// data loaded immediately after code...
			iRamInfo.iDataLoadAddr = iRamInfo.iCodeLoadAddr+iRamInfo.iCodeSize;
			loadSize += iRamInfo.iDataSize;
			}
		else
			{
			// create memory object for data...
			DMemoryObject* dataMemory;
			r = MM::MemoryNew(dataMemory, EMemoryObjectMovable, dataPageCount, EMemoryCreateNoWipe);
			if(r!=KErrNone)
				return r;

			// commit memory...
			r = MM::MemoryAlloc(dataMemory,0,dataPageCount);
			if(r==KErrNone)
				{
				// create a mapping of the memory for the loader...
				// No need to open reference on os asid it is the current thread/process's.
				r = MM::MappingNew(iDataLoadMapping,dataMemory,EUserReadWrite,pP->OsAsid());
				}

			if(r!=KErrNone)
				{
				MM::MemoryDestroy(dataMemory);
				return r;
				}

			iRamInfo.iDataLoadAddr = MM::MappingBase(iDataLoadMapping);
			}
		}

	if(!isDemandPaged)
		{
		// wipe memory that the loader wont fill...
		UNLOCK_USER_MEMORY();
		memset((TAny*)(iRamInfo.iCodeLoadAddr+loadSize), 0x03, codeSize-loadSize);
		LOCK_USER_MEMORY();
		}

	// done...
	iCreator = pP;
	
	return KErrNone;
	}


TInt DMemModelCodeSegMemory::Loaded(TCodeSegCreateInfo& aInfo)
	{
	if(iPagedCodeInfo)
		{
		// get information needed to fixup code for it's run address...
		TInt r = iPagedCodeInfo->ReadFixupTables(aInfo);
		if(r!=KErrNone)
			return r;
		MM::PagedCodeLoaded(iCodeMemoryObject, iRamInfo.iCodeLoadAddr);
		}
	else
		{
		// make code visible to instruction cache...
		UNLOCK_USER_MEMORY();
		CacheMaintenance::CodeChanged(iRamInfo.iCodeLoadAddr, iRamInfo.iCodeSize);
		LOCK_USER_MEMORY();
		}

	// adjust iDataLoadAddr to point to address contents for initial data section
	// in running process...
	if(iRamInfo.iDataLoadAddr)
		{
		TAny* dataSection = iDataSectionMemory;
		if(dataSection)
			{
			// contents for initial data section to be stored in iDataSectionMemory...
			UNLOCK_USER_MEMORY();
			memcpy(dataSection,(TAny*)iRamInfo.iDataLoadAddr,iRamInfo.iDataSize);
			LOCK_USER_MEMORY();
			iRamInfo.iDataLoadAddr = (TLinAddr)dataSection;
			}
		else
			{
			// contents for initial data section stored after code...
			__NK_ASSERT_DEBUG(iRamInfo.iDataLoadAddr==iRamInfo.iCodeLoadAddr+iRamInfo.iCodeSize); // check data loaded at end of code
			iRamInfo.iDataLoadAddr = iRamInfo.iCodeRunAddr+iRamInfo.iCodeSize;
			}
		}

	// copy export directory (this will now have fixups applied)...
	TInt exportDirSize = iRamInfo.iExportDirCount * sizeof(TLinAddr);
	if(exportDirSize > 0 || (exportDirSize==0 && (iCodeSeg->iAttr&ECodeSegAttNmdExpData)) )
		{
		TLinAddr expDirLoad = iRamInfo.iExportDir - iRamInfo.iCodeRunAddr + iRamInfo.iCodeLoadAddr;
		if (expDirLoad < iRamInfo.iCodeLoadAddr ||
			expDirLoad + exportDirSize > iRamInfo.iCodeLoadAddr + iRamInfo.iCodeSize)
			{// Invalid export section but the loader should have checked this.
			return KErrCorrupt;
			}
		exportDirSize += sizeof(TLinAddr);
		TLinAddr* expDir = (TLinAddr*)Kern::Alloc(exportDirSize);
		if(!expDir)
			return KErrNoMemory;
		iCopyOfExportDir = expDir;
		UNLOCK_USER_MEMORY();
		memcpy(expDir,(TAny*)(expDirLoad-sizeof(TLinAddr)),exportDirSize);
		LOCK_USER_MEMORY();
		}

	// unmap code from loading process...
	DMemModelProcess* pP=(DMemModelProcess*)TheCurrentThread->iOwningProcess;
	__ASSERT_ALWAYS(iCreator==pP, MM::Panic(MM::ECodeSegLoadedNotCreator));
	MM::MappingDestroy(iCodeLoadMapping);
	MM::MappingAndMemoryDestroy(iDataLoadMapping);
	iCreator=NULL;

	// Mark the code memory object read only to prevent malicious code modifying it.
	TInt r = MM::MemorySetReadOnly(iCodeMemoryObject);
	__ASSERT_ALWAYS(r == KErrNone, MM::Panic(MM::ECodeSegSetReadOnlyFailure));

	return KErrNone;
	}


void DMemModelCodeSegMemory::Destroy()
	{
	MM::MappingDestroy(iCodeLoadMapping);
	MM::MappingAndMemoryDestroy(iDataLoadMapping);
	}


DMemModelCodeSegMemory::~DMemModelCodeSegMemory()
	{
	__KTRACE_OPT(KDLL,Kern::Printf("DMemModelCodeSegMemory::~DMemModelCodeSegMemory %x", this));
	__NK_ASSERT_DEBUG(iAccessCount==0);

	MM::MappingDestroy(iCodeLoadMapping);
	MM::MappingAndMemoryDestroy(iDataLoadMapping);
	MM::MemoryDestroy(iCodeMemoryObject);

	if(iVirtualAllocCommonSize)
		MM::VirtualFreeCommon(iRamInfo.iCodeRunAddr, iVirtualAllocCommonSize);

	Kern::Free(iCopyOfExportDir);
	Kern::Free(iDataSectionMemory);
	}


//
// DMemModelCodeSeg
//

DMemModelCodeSeg::DMemModelCodeSeg()
	{
	}


DMemModelCodeSeg::~DMemModelCodeSeg()
	{
	__KTRACE_OPT(KDLL,Kern::Printf("DMemModelCodeSeg::Destruct %C", this));
	DCodeSeg::Wait();

	MM::MappingDestroy(iCodeLoadMapping);
	MM::MappingDestroy(iCodeGlobalMapping);
	MM::MemoryDestroy(iCodeMemoryObject);

	if(Memory())
		Memory()->Destroy();

	if(iDataAllocSize)
		MM::VirtualFreeCommon(iDataAllocBase,iDataAllocSize);

	DCodeSeg::Signal();

	Kern::Free(iKernelData);

	DEpocCodeSeg::Destruct();
	}


TInt DMemModelCodeSeg::DoCreateRam(TCodeSegCreateInfo& aInfo, DProcess* aProcess)
	{
	__KTRACE_OPT(KDLL,Kern::Printf("DMemModelCodeSeg::DoCreateRam %C", this));

	SRamCodeInfo& ri = RamInfo();
	iSize = MM::RoundToPageSize(ri.iCodeSize+ri.iDataSize);
	if (iSize==0)
		return KErrCorrupt;

	TBool kernel = ( (iAttr&(ECodeSegAttKernel|ECodeSegAttGlobal)) == ECodeSegAttKernel );
//	TBool user_global = ( (iAttr&(ECodeSegAttKernel|ECodeSegAttGlobal)) == ECodeSegAttGlobal );
	TBool user_local = ( (iAttr&(ECodeSegAttKernel|ECodeSegAttGlobal)) == 0 );

	TUint total_data_size = ri.iDataSize+ri.iBssSize;

	if(user_local)
		{
		// setup paging attribute for code...
		if(aInfo.iUseCodePaging)
			iAttr |= ECodeSegAttCodePaged;

		if(total_data_size && !IsExe())
			{
			// setup paging attribute for data section...
			if(aInfo.iUseCodePaging)
				if(K::MemModelAttributes & EMemModelAttrDataPaging)
					iAttr |= ECodeSegAttDataPaged;

			// allocate virtual address for data section...
			TInt r = MM::VirtualAllocCommon(iDataAllocBase,total_data_size,iAttr&ECodeSegAttDataPaged);
			if(r!=KErrNone)
				return r;
			iDataAllocSize = total_data_size;
			ri.iDataRunAddr = iDataAllocBase;
			}

		// create DCodeSegMemory for RAM loaded user local code...
		TInt r = Memory()->Create(aInfo,(DMemModelProcess*)aProcess);

#ifdef BTRACE_FLEXIBLE_MEM_MODEL
		if (r == KErrNone)
			{
			BTrace8(BTrace::EFlexibleMemModel,BTrace::EMemoryObjectIsCodeSeg,Memory()->iCodeMemoryObject,this);
			}
#endif
		
		return r;
		}

	// kernel or user-global code...

	// create memory object for codeseg...
	TMemoryCreateFlags flags = EMemoryCreateAllowExecution;
	if(kernel)
		{
		flags = (TMemoryCreateFlags)(flags|EMemoryCreateNoWipe);
		}
	TInt r = MM::MemoryNew(iCodeMemoryObject, EMemoryObjectMovable, MM::BytesToPages(iSize), flags);
	if(r!=KErrNone)
		return r;

	// commit memory...
	r = MM::MemoryAlloc(iCodeMemoryObject,0,MM::BytesToPages(iSize));
	if(r!=KErrNone)
		return r;

	// create a mapping of the memory for the loader...
	// No need to open reference on os asid it is the current thread/process's.
	DMemModelProcess* pP = (DMemModelProcess*)TheCurrentThread->iOwningProcess;
	r = MM::MappingNew(iCodeLoadMapping,iCodeMemoryObject,EUserReadWrite,pP->OsAsid());
	if(r!=KErrNone)
		return r;
	ri.iCodeLoadAddr = MM::MappingBase(iCodeLoadMapping);

	// create a global mapping of the memory for the codeseg to run at...
	r = MM::MappingNew(iCodeGlobalMapping,iCodeMemoryObject,kernel?ESupervisorExecute:EUserExecute,KKernelOsAsid);
	if(r!=KErrNone)
		return r;
	ri.iCodeRunAddr = MM::MappingBase(iCodeGlobalMapping);

	if(kernel)
		{
		// setup data section memory...
		if (ri.iDataSize)
			ri.iDataLoadAddr = ri.iCodeLoadAddr+ri.iCodeSize;
		if (total_data_size)
			{
			iKernelData = Kern::Alloc(total_data_size);
			if (!iKernelData)
				return KErrNoMemory;
			ri.iDataRunAddr = (TLinAddr)iKernelData;
			}
		}
	else
		{
		// we don't allow static data in global code...
		ri.iDataLoadAddr = 0;
		ri.iDataRunAddr = 0;
		}

#ifdef BTRACE_FLEXIBLE_MEM_MODEL
	BTrace8(BTrace::EFlexibleMemModel,BTrace::EMemoryObjectIsCodeSeg,iCodeMemoryObject,this);
#endif

	// done...
	return KErrNone;
	}


TInt DMemModelCodeSeg::DoCreateXIP(DProcess* aProcess)
	{
//	__KTRACE_OPT(KDLL,Kern::Printf("DMemModelCodeSeg::DoCreateXIP %C proc %O", this, aProcess));
	return KErrNone;
	}


TInt DMemModelCodeSeg::Loaded(TCodeSegCreateInfo& aInfo)
	{
	if(iXIP)
		return DEpocCodeSeg::Loaded(aInfo);

	TBool kernel = ( (iAttr&(ECodeSegAttKernel|ECodeSegAttGlobal)) == ECodeSegAttKernel );
	TBool user_global = ( (iAttr&(ECodeSegAttKernel|ECodeSegAttGlobal)) == ECodeSegAttGlobal );
	TBool user_local = ( (iAttr&(ECodeSegAttKernel|ECodeSegAttGlobal)) == 0 );
	if(user_local)
		{
		TInt r = Memory()->Loaded(aInfo);
		if(r!=KErrNone)
			return r;
		}
	else if((kernel && iExeCodeSeg!=this) || user_global)
		{
		// user-global or kernel code...
		SRamCodeInfo& ri = RamInfo();
		UNLOCK_USER_MEMORY();
		CacheMaintenance::CodeChanged(ri.iCodeLoadAddr, ri.iCodeSize);
		LOCK_USER_MEMORY();
		MM::MappingDestroy(iCodeLoadMapping);
		// adjust iDataLoadAddr to point to address contents for initial data section
		// in running process...
		if(ri.iDataLoadAddr)
			ri.iDataLoadAddr = ri.iCodeRunAddr+ri.iCodeSize;

		// Mark the code memory object read only to prevent malicious code modifying it.
		TInt r = MM::MemorySetReadOnly(iCodeMemoryObject);
		__ASSERT_ALWAYS(r == KErrNone, MM::Panic(MM::ECodeSegSetReadOnlyFailure));
		}
	return DEpocCodeSeg::Loaded(aInfo);
	}


void DMemModelCodeSeg::ReadExportDir(TUint32* aDest)
	{
	__KTRACE_OPT(KDLL,Kern::Printf("DMemModelCodeSeg::ReadExportDir %C %08x",this, aDest));

	if(!iXIP)
		{
		// This is not XIP code so the loader can't access the export directory. 
		if (Memory()->iCopyOfExportDir)
			{// This must be local user side code.
			__NK_ASSERT_DEBUG((iAttr & (ECodeSegAttKernel|ECodeSegAttGlobal)) == 0);
			// Copy the kernel's copy of the export directory for this code seg to the loader's buffer.
			SRamCodeInfo& ri = RamInfo();
			TInt size = (ri.iExportDirCount + 1) * sizeof(TLinAddr);
			kumemput(aDest, Memory()->iCopyOfExportDir, size);
			}
		else
			{// This must be kernel side code.
			__NK_ASSERT_DEBUG((iAttr & (ECodeSegAttKernel|ECodeSegAttGlobal)) == ECodeSegAttKernel);
			// Copy the export directory for this code seg to the loader's buffer.
			SRamCodeInfo& ri = RamInfo();
			TInt size = (ri.iExportDirCount + 1) * sizeof(TLinAddr);
			TAny* expDirLoad = (TAny*)(ri.iExportDir - sizeof(TLinAddr));
			kumemput(aDest, expDirLoad, size);
			}
		}
	}


TBool DMemModelCodeSeg::OpenCheck(DProcess* aProcess)
	{
	return FindCheck(aProcess);
	}


TBool DMemModelCodeSeg::FindCheck(DProcess* aProcess)
	{
	__KTRACE_OPT(KDLL,Kern::Printf("CSEG:%08x Compat? proc=%O",this,aProcess));
	if (aProcess)
		{
		DMemModelProcess& p=*(DMemModelProcess*)aProcess;
		DCodeSeg* pPSeg=p.CodeSeg();
		if (iAttachProcess && iAttachProcess!=aProcess)
			return EFalse;
		if (iExeCodeSeg && iExeCodeSeg!=pPSeg)
			return EFalse;
		}
	return ETrue;
	}


void DMemModelCodeSeg::BTracePrime(TInt aCategory)
	{
	DCodeSeg::BTracePrime(aCategory);

#ifdef BTRACE_FLEXIBLE_MEM_MODEL
	if (aCategory == BTrace::EFlexibleMemModel || aCategory == -1)
		{
		// code seg mutex is held here, so memory objects cannot be destroyed
		DMemModelCodeSegMemory* codeSegMemory = Memory();
		if (codeSegMemory)
			{
			if (codeSegMemory->iCodeMemoryObject)
				{
				BTrace8(BTrace::EFlexibleMemModel,BTrace::EMemoryObjectIsCodeSeg,Memory()->iCodeMemoryObject,this);
				}
			}
		else
			{
			if (iCodeMemoryObject)
				{
				BTrace8(BTrace::EFlexibleMemModel,BTrace::EMemoryObjectIsCodeSeg,iCodeMemoryObject,this);
				}
			}
		}
#endif	
	}


//
// TPagedCodeInfo
//

TPagedCodeInfo::~TPagedCodeInfo()
	{
	Kern::Free(iCodeRelocTable);
	Kern::Free(iCodePageOffsets);
	}


TInt TPagedCodeInfo::ReadBlockMap(const TCodeSegCreateInfo& aInfo)
	{
	if(aInfo.iCodeBlockMapEntriesSize <= 0)
		return KErrArgument;  // no block map provided

	// get compression data...
	iCompressionType = aInfo.iCompressionType;
	switch(iCompressionType)
		{
	case KFormatNotCompressed:
		__ASSERT_COMPILE(KFormatNotCompressed==0); // Decompress() assumes this
		break;

	case KUidCompressionBytePair:
		{
		if(!aInfo.iCodePageOffsets)
			return KErrArgument;

		TInt pageCount = MM::RoundToPageCount(aInfo.iCodeSize);

		TInt size = sizeof(TInt32) * (pageCount + 1);
		iCodePageOffsets = (TInt32*)Kern::Alloc(size);
		if(!iCodePageOffsets)
			return KErrNoMemory;
		kumemget32(iCodePageOffsets, aInfo.iCodePageOffsets, size);

#ifdef __DUMP_BLOCKMAP_INFO
		Kern::Printf("CodePageOffsets:");
		for (TInt i = 0 ; i < pageCount + 1 ; ++i)
			Kern::Printf("  %08x", iCodePageOffsets[i]);
#endif

		TInt last = 0;
		for(TInt j=0; j<pageCount+1; ++j)
			{
			if(iCodePageOffsets[j] < last ||
				iCodePageOffsets[j] > (aInfo.iCodeLengthInFile + aInfo.iCodeStartInFile))
				{
				__NK_ASSERT_DEBUG(0);
				return KErrCorrupt;
				}
			last = iCodePageOffsets[j];
			}
		}
		break;

	default:
		return KErrNotSupported;
		}

	// Copy block map data itself...

#ifdef __DUMP_BLOCKMAP_INFO
	Kern::Printf("Original block map");
	Kern::Printf("  block granularity: %d", aInfo.iCodeBlockMapCommon.iBlockGranularity);
	Kern::Printf("  block start offset: %x", aInfo.iCodeBlockMapCommon.iBlockStartOffset);
	Kern::Printf("  start block address: %016lx", aInfo.iCodeBlockMapCommon.iStartBlockAddress);
	Kern::Printf("  local drive number: %d", aInfo.iCodeBlockMapCommon.iLocalDriveNumber);
	Kern::Printf("  entry size: %d", aInfo.iCodeBlockMapEntriesSize);
#endif

	// Find relevant paging device
	iCodeLocalDrive = aInfo.iCodeBlockMapCommon.iLocalDriveNumber;
	if(TUint(iCodeLocalDrive) >= (TUint)KMaxLocalDrives)
		{
		__KTRACE_OPT(KPAGING,Kern::Printf("Bad local drive number"));
		return KErrArgument;
		}

	DPagingDevice* device = CodePagingDevice(iCodeLocalDrive);
	if(!device)
		{
		__KTRACE_OPT(KPAGING,Kern::Printf("No paging device installed for drive"));
		return KErrNotSupported;
		}

	// Set code start offset
	iCodeStartInFile = aInfo.iCodeStartInFile;
	if(iCodeStartInFile < 0)
		{
		__KTRACE_OPT(KPAGING,Kern::Printf("Bad code start offset"));
		return KErrArgument;
		}

	// Allocate buffer for block map and copy from user-side
	TBlockMapEntryBase* buffer = (TBlockMapEntryBase*)Kern::Alloc(aInfo.iCodeBlockMapEntriesSize);
	if(!buffer)
		return KErrNoMemory;
	kumemget32(buffer, aInfo.iCodeBlockMapEntries, aInfo.iCodeBlockMapEntriesSize);

#ifdef __DUMP_BLOCKMAP_INFO
	Kern::Printf("  entries:");
	for (TInt k = 0 ; k < aInfo.iCodeBlockMapEntriesSize / sizeof(TBlockMapEntryBase) ; ++k)
		Kern::Printf("    %d: %d blocks at %08x", k, buffer[k].iNumberOfBlocks, buffer[k].iStartBlock);
#endif

	// Initialise block map
	TInt r = iBlockMap.Initialise(aInfo.iCodeBlockMapCommon,
								  buffer,
								  aInfo.iCodeBlockMapEntriesSize,
								  device->iReadUnitShift,
								  iCodeStartInFile + aInfo.iCodeLengthInFile);
	if(r!=KErrNone)
		{
		Kern::Free(buffer);
		return r;
		}

#if defined(__DUMP_BLOCKMAP_INFO) && defined(_DEBUG)
	iBlockMap.Dump();
#endif

	iCodeSize = aInfo.iCodeSize;
	return KErrNone;
	}


/**
Read code relocation table and import fixup table from user side.
*/
TInt TPagedCodeInfo::ReadFixupTables(const TCodeSegCreateInfo& aInfo)
	{
	iCodeRelocTableSize = aInfo.iCodeRelocTableSize;
	iImportFixupTableSize = aInfo.iImportFixupTableSize;
	iCodeDelta = aInfo.iCodeDelta;
	iDataDelta = aInfo.iDataDelta;

	// round sizes up to four-byte boundaries...
	TUint relocSize = (iCodeRelocTableSize + 3) & ~3;
	TUint fixupSize = (iImportFixupTableSize + 3) & ~3;

	// copy relocs and fixups...
	iCodeRelocTable = (TUint8*)Kern::Alloc(relocSize+fixupSize);
	if (!iCodeRelocTable)
		return KErrNoMemory;
	iImportFixupTable = iCodeRelocTable + relocSize;
	kumemget32(iCodeRelocTable, aInfo.iCodeRelocTable, relocSize);
	kumemget32(iImportFixupTable, aInfo.iImportFixupTable, fixupSize);

	return KErrNone;
	}


void TPagedCodeInfo::ApplyFixups(TLinAddr aBuffer, TUint iIndex)
	{
//	START_PAGING_BENCHMARK;
	
	// relocate code...
	if(iCodeRelocTableSize)
		{
		TUint8* codeRelocTable = iCodeRelocTable;
		TUint startOffset = ((TUint32*)codeRelocTable)[iIndex];
		TUint endOffset = ((TUint32*)codeRelocTable)[iIndex+1];

		__KTRACE_OPT(KPAGING, Kern::Printf("Performing code relocation: start == %x, end == %x", startOffset, endOffset));
		__ASSERT_ALWAYS(startOffset<=endOffset && endOffset<=iCodeRelocTableSize, K::Fault(K::ECodeSegBadFixupTables));

		const TUint32 codeDelta = iCodeDelta;
		const TUint32 dataDelta = iDataDelta;

		const TUint16* ptr = (const TUint16*)(codeRelocTable + startOffset);
		const TUint16* end = (const TUint16*)(codeRelocTable + endOffset);
		while(ptr<end)
			{
			TUint16 entry = *ptr++;
			TUint32* addr = (TUint32*)(aBuffer+(entry&0x0fff));
			TUint32 word = *addr;
#ifdef _DEBUG
			TInt type = entry&0xf000;
			__NK_ASSERT_DEBUG(type==KTextRelocType || type==KDataRelocType);
#endif
			if(entry<KDataRelocType)
				word += codeDelta;
			else
				word += dataDelta;
			*addr = word;
			}
		}

	// fixup imports...
	if(iImportFixupTableSize)
		{
		TUint8* importFixupTable = iImportFixupTable;
		TUint startOffset = ((TUint32*)importFixupTable)[iIndex];
		TUint endOffset = ((TUint32*)importFixupTable)[iIndex+1];

		__KTRACE_OPT(KPAGING, Kern::Printf("Performing import fixup: start == %x, end == %x", startOffset, endOffset));
		__ASSERT_ALWAYS(startOffset<=endOffset && endOffset<=iImportFixupTableSize, K::Fault(K::ECodeSegBadFixupTables));

		const TUint16* ptr = (const TUint16*)(importFixupTable + startOffset);
		const TUint16* end = (const TUint16*)(importFixupTable + endOffset);

		while(ptr<end)
			{
			TUint16 offset = *ptr++;
			TUint32 wordLow = *ptr++;
			TUint32 wordHigh = *ptr++;
			TUint32 word = (wordHigh << 16) | wordLow;
//			__KTRACE_OPT(KPAGING, Kern::Printf("DP: Fixup %08x=%08x", iRamInfo.iCodeRunAddr+(page<<KPageShift)+offset, word));
			*(TUint32*)(aBuffer+offset) = word;
			}
		}
	
//	END_PAGING_BENCHMARK(DemandPaging::ThePager, EPagingBmFixupCodePage);
	}


