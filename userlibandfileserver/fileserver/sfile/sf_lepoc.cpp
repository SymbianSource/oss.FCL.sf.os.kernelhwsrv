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
// f32\sfile\sf_lepoc.cpp
// 
//

#include "sf_std.h"

#include <e32std.h>
#include <e32std_private.h>
#include <e32base.h>
#include <e32base_private.h>
#include <e32math.h>
#include <e32svr.h>
#include <e32ver.h>
#include <e32hal.h>
#include <u32exec.h>
#define INCLUDE_E32IMAGEHEADER_IMPLEMENTATION
#include "sf_ldr.h"
#include <f32image.h>
#include "sf_image.h"
#include <e32uid.h>
#include <e32rom.h>
#include "sf_cache.h"

#include "sf_pgcompr.h"

_LIT(KLitFinderInconsistent, "LDR-FINDER-INC");
_LIT(KLitSysBinError, "LDR-SYS\\BIN ERR");
_LIT8(KSysBin,":\\sys\\bin\\");

#ifdef _DEBUG

enum TLdrEpocPanic
	{
	EFuaiNoFixupTable = 0x10,
	EBcbmNotCodePaged = 0x20,
	ELfiCodePagingNotSupported = 0x30,
	EFprUnexpectedFixup = 0x40,
	};

static void Panic(TLdrEpocPanic aPanic)
	{
	_LIT(KPanicCat, "LDR-PNC");
	User::Panic(KPanicCat, aPanic);
	}

extern TRequestStatus* ProcessDestructStatPtr;
extern TBool ProcessCreated;

#endif

extern void DumpImageHeader(const E32ImageHeader*);
extern TDriveCacheHeader* gDriveFileNamesCache[];

TBuf8<KMaxPath> gLoadeePath;
TUint NextCodeSegId;

const TInt KMaxHeaderSize = sizeof(E32ImageHeaderV) + 65536/8;


#ifdef __X86__
extern TInt UseFloppy;
#endif



// -------- demand paging --------

/** Page size as a power of two. */
const TUint32 KPageSizeShift = 12;
/** Page size, as defined for code relocations.  This same page size is used for demand paging. */
const TUint32 KPageSize = 1<<KPageSizeShift;
/** Apply this mask to an address to get the page offset. */
const TUint32 KPageOffsetMask = KPageSize - 1;

/**
Calculate the number of pages required to contain the supplied number of bytes.

@param	aSizeInBytes	Size of are which has to be contained in whole blocks.
@return					Number of KPageSize pages required to contain area.
*/
inline TInt SizeToPageCount(TInt aSizeInBytes)
	{
	return (aSizeInBytes + KPageOffsetMask) >> KPageSizeShift;
	}


/**
Allocate a block which indexes the reallocations by page.  This can be used for demand paging.

@param	aSection			Pointer to relocation section to process.
@param	aAreaSize			Size in bytes of area described by reloc section.
@param  aLoadAddress		Address of relocation section in memory
@param	aProcessedBlock		On success (return == KErrNone) this is set to the processed
							relocation section which is allocated on the current thread's heap.
							The caller takes ownership.  The contents are undefined on failure.
@return						KErrNoMemory if could not allocate memory for processed block
							and auxiliary structures; KErrNone otherwise.
 */
TInt E32Image::AllocateRelocationData(E32RelocSection* aSection, TUint32 aAreaSize, TUint32 aLoadAddress, TUint32*& aProcessedBlock)
	{
	__IF_DEBUG(Printf("AllocateRelocationData"));

	TUint32 sectionSize = aSection->iSize;
	TUint32 numRelocs = aSection->iNumberOfRelocs;
	TInt pageCount = SizeToPageCount(aAreaSize);

	// The file format documentation (SOSI ch10) does not guarantee that each page has
	// relocation information, or that the pages are listed in order, so store them in
	// page order here.
	
	TUint8** subBlocks = (TUint8**)User::AllocZ(sizeof(TUint8*)*pageCount);
	if(subBlocks == 0)
		return KErrNoMemory;

	const TUint8* subBlockPtr = (TUint8*)(aSection+1);
	while(sectionSize > 0)
		{
		TUint32 pageOffset = *(TUint32*)(subBlockPtr);
		TUint32 subBlockSize = *(TUint32*)(subBlockPtr+4);

		subBlocks[pageOffset >> KPageSizeShift] = (TUint8*)subBlockPtr;
		
		sectionSize -= subBlockSize;
		subBlockPtr += subBlockSize;	// move to next sub-block
		}

	// now have each relocation page in memory, build lookup table	
	TUint32 indexSize = (pageCount + 1) * sizeof(TUint32);	// include sentinel
	TUint32 totalRelocations = numRelocs;
	iCodeRelocTableSize = indexSize + totalRelocations * sizeof(TUint16);
	TUint8* table = (TUint8*) User::Alloc(iCodeRelocTableSize);

	if(table == 0)
		{
		User::Free(subBlocks);
		return KErrNoMemory;
		}

	// where sub-block positions are written to in the table
	TUint32* destSubBlock = (TUint32*)table;
	// where entries are written to in the table
	TUint16* destEntry = (TUint16*)(table + indexSize);

	TInt i;
	for(i = 0; i < pageCount; ++i)
		{
		*destSubBlock++ = TUint32(destEntry) - TUint32(table);
		
		// see if a relocation page was defined for this page
		const TUint8* subBlock = subBlocks[i];
		if(subBlock == 0)
			continue;
		
		// get number of entries in this sub-block, including padding
		TUint32 sbEntryCount;
		TUint32 pageOffset = *(TUint32*)subBlock;	// offset of page from start of section
		sbEntryCount = *(TUint32*)(subBlock + 4);	// sub-block size
		sbEntryCount -= 8;							// exclude sub-block header
		sbEntryCount /= 2;							// each entry is two bytes
		const TUint16* srcEntry = (TUint16*)(subBlock + 8);
		 
		while(sbEntryCount--)
			{
			TUint16 entry = *srcEntry++;
			if(entry==0)		// ignore null padding values
				continue;

			// Replace inferred fixup type with actual fixup type
			TUint type = entry & 0xf000;
			if(type==KInferredRelocType)
				{
				TUint32* ptr = (TUint32*)(aLoadAddress + pageOffset + (entry & 0x0fff));
				TUint32 word = *ptr;
				type = (TUint(word - iHeader->iCodeBase) < TUint(iHeader->iCodeSize)) ? KTextRelocType : KDataRelocType;
				entry = (entry & 0x0fff) | type;
				}
			
			*destEntry++ = entry;
			}
		}
	
	// sentinel entry marks the byte following last sub-block in table
	// This gives the size of the last processed sub-block.
	*destSubBlock = TUint32(destEntry) - TUint32(table);

	aProcessedBlock = (TUint32*) table;
	User::Free(subBlocks);

#ifdef _DEBUG
	__IF_DEBUG(Printf("processed reloc table (size=%d,pageCount=%d)", iCodeRelocTableSize, pageCount));

	// Dump the processed reloc table if loader tracing enabled. The dump is in
	// two parts; first, the page indexes (1 word per page), then the entries
	// describing the items to be relocated on each of these pages, formatted
	// with up to 8 entries per line but starting a new line for each page.
	// Each of these entries has the relocation type in the first nibble, and
	// the offset within the page in the remaining 3 nibbles.
	const TUint32* table32 = (const TUint32*)table;
	for (i = 0; i <= pageCount; ++i)
		__IF_DEBUG(Printf("%04x: %08x", i*4, table32[i]));

	for (i = 0; i < pageCount; ++i)
		{
		TUint start = table32[i];
		TInt nbytes = table32[i+1] - start;
		while (nbytes)
			{
			TBuf8<0x100> buf;
			buf.Format(_L8("%04x:"), start);

			const TUint16* p = (const TUint16*)(table+start);
			TInt n = nbytes <= 16 ? nbytes : 16;
			for (nbytes -= n, start += n; n > 0; n -= 2)
				buf.AppendFormat(_L8(" %04x"), *p++);

			buf.AppendFormat(_L8("\r\n"));
			__IF_DEBUG(RawPrint(buf));
			}
		}
#endif
	return KErrNone;
	}


/*******************************************************************************
 * These functions run in supervisor mode since they require access to the
 * chunks of the newly-created process or DLL while they are still in the
 * home section.
 ******************************************************************************/

/**
Vector which ::ExecuteInSupervisorMode invokes.
*/
TInt (*ExecuteInSupervisorModeVector)(TSupervisorFunction, TAny*);

/**
Executute aFunction in supervisor mode (if the memory model requires this.)
*/
TInt ExecuteInSupervisorMode(TSupervisorFunction aFunction, TAny* aParameter)
	{
	return(*ExecuteInSupervisorModeVector)(aFunction, aParameter);
	}

/**
Implementation of ::ExecuteInSupervisorMode which actually executes the
function in user mode.
*/
TInt UserModeExecuteInSupervisorMode(TSupervisorFunction aFunction, TAny* aParameter)
	{
	return (*aFunction)(aParameter);
	}

/**
Decide whether any Loader code actually needs to execute in supervisor mode
and set ::ExecuteInSupervisorModeVector so that invocations of ::ExecuteInSupervisorMode
call the appropriate function.
*/
void InitExecuteInSupervisorMode()
	{
	// work out if we need to really 'execute in supervisor mode'...
	TUint32 memModelAttrs = (TUint32)UserSvr::HalFunction(EHalGroupKernel, EKernelHalMemModelInfo, NULL, NULL);
	TUint32 memModel = memModelAttrs & EMemModelTypeMask;
	if(memModel==EMemModelTypeFlexible)
		{
		// we can do everything user side...
		ExecuteInSupervisorModeVector = UserModeExecuteInSupervisorMode;
		gExecutesInSupervisorMode = EFalse;
		}
	else
		{
		// we need to go kernel side...
		ExecuteInSupervisorModeVector = UserSvr::ExecuteInSupervisorMode;
		gExecutesInSupervisorMode = ETrue;
		}
	}

/**
It would be nice to be able to print debug information from the various functions
supervisor-mode functions below. Unfortunately, we can't call RDebug::Printf() or
any of its relatives in supervisor mode, and of course we can't call the equivalent
kernel functions even when we're already in supervisor mode, because the entry
points aren't visible.

So this function just wraps and guards the call to RDebug, so we won't call it
in SVC mode. The outcome is that trace messages are only generated if using the
flexible memory model, where the code doesn't actually run in SVC mode anyway.
*/
void svPrintf(const char* aFmt, ...)
	{
	if (gExecutesInSupervisorMode)
		return;

	VA_LIST list;
	VA_START(list, aFmt);
	TPtrC8 fmt((const TText8*)aFmt);
	TBuf8<0x100> buf;
	buf.AppendFormatList(fmt, list);
	buf.AppendFormat(_L8("\r\n"));
	RDebug::RawPrint(buf);
	VA_END(list);
	}

/**
Arguments for svRelocateSection.

The relocation information (at iRelocsBuf) has list sub blocks, each referring to a 4kB
page within the section. See E32RelocBlock.
*/
struct SRelocateSectionInfo
	{
	E32Image* iImage;		///< The executable being relocated.
	TUint8* iRelocsBuf;		///< Pointer to relocation info.
	TUint32 iNumRelocs;		///< Total number of relocations to apply.
	TUint32 iLoadAddress; 	///< Virtual address where section is currently located in memory.
	};

/**
Apply relocations to a code or data section.

@param aPtr Pointer to SRelocateSectionInfo.
*/
TInt svRelocateSection(TAny* aPtr)
	{
	SRelocateSectionInfo& info=*(SRelocateSectionInfo*)aPtr;

	E32Image& img = *(E32Image*)info.iImage;
	TUint8* relocs = info.iRelocsBuf;
	TUint32 numRelocs = info.iNumRelocs;
	TUint32 loadAddress = info.iLoadAddress;

	TUint32 codeStart = img.iHeader->iCodeBase;
	TUint32 codeFinish = codeStart+img.iHeader->iCodeSize;
	TUint32 codeDelta = img.iCodeDelta;
	TUint32 dataDelta = img.iDataDelta;

	while(numRelocs>0)
		{
		TUint32 pageAddress = ((TUint32*)relocs)[0];
		TUint32 pageSize = ((TUint32*)relocs)[1];
		TUint8* relocsEnd = relocs+pageSize;
		relocs += 8;

		while(relocs<relocsEnd)
			{
			TUint16 relocOffset = *(TUint16*)relocs;
			relocs += 2;
			if(!relocOffset)
				continue;

			TUint32 offset = pageAddress+(TUint32)(relocOffset&0x0fff);
			TUint32* destPtr = (TUint32*)(loadAddress+offset);
			TUint16 relocType = relocOffset&0xf000;

			TUint32 relocAddr = *destPtr;
			if(relocType==KTextRelocType)
				relocAddr += codeDelta; // points to text/rdata section
			else if(relocType==KDataRelocType)
				relocAddr += dataDelta; // points to data section
			else if (relocAddr>=codeStart && relocAddr<codeFinish)
				relocAddr += codeDelta; // points to text/rdata section
			else
				relocAddr += dataDelta; // points to data section
			*destPtr = relocAddr;

			--numRelocs;
			}
		}
	return 0;
	}


/**
Fix up the export directory
Only performed on PE images.  ELF image's exports are marked as relocatable
and therefore relocated by svRelocateSection along with the text section
*/
TInt svRelocateExports(TAny* aPtr)
	{
	E32Image& exporter = *(E32Image*)aPtr;

	// Dump everything potentially useful that we know about the exporter ...
	__LDRTRACE(svPrintf("RelocateExports: paged? %d, iRomImageHeader@%08x, iHeader@%08x",
						exporter.iUseCodePaging, exporter.iRomImageHeader, exporter.iHeader));
	__LDRTRACE(svPrintf("  iCodeLoadAddress %08x, iCodeRunAddress %08x, iCodeSize %x iTextSize %x",
						exporter.iCodeLoadAddress, exporter.iCodeRunAddress,
						exporter.iCodeSize, exporter.iTextSize))
	__LDRTRACE(svPrintf("  iDataLoadAddress %08x, iDataRunAddress %08x, iDataSize %x iBssSize %x iTotalDataSize %x",
						exporter.iDataLoadAddress, exporter.iDataRunAddress,
						exporter.iDataSize, exporter.iBssSize, exporter.iTotalDataSize));
	__LDRTRACE(svPrintf("  iCodeDelta, %x iDataDelta %x, iExportDirEntryDelta %x",
						exporter.iCodeDelta, exporter.iDataDelta, exporter.iExportDirEntryDelta));

	// It turns out that very little of the exporter info is useful! For
	// example, the required code and data deltas are NOT those provided
	// by the exporter, nor are the load addresses relevant ... :(
	//
	// In the case of a PE-derived image, the entries in the export table
	// are expressed in terms of offsets into the image file, rather than
	// locations in memory. Each therefore needs to be relocated by the
	// difference between its file offset and its run address.
	//
	// It is assumed that the code segment appears before the data segment
	// in the file; therefore, export table entries with values between 0
	// and (exporter.iCodeSize) refer to the text segment, while higher
	// values represent references to data addresses. Since the run addresses
	// of code and data segments may be different, each type of export must
	// be relocated with respect to the correct section.
	//
	// The following express the start and finish of each section in terms of
	// file offsets and then derive the required adjustments to the entries
	// in the export table ...
	TUint32 codeStart = 0;							// compiler whinges if this is 'const' :(
	const TUint32 codeFinish = codeStart + exporter.iCodeSize;
	const TUint32 dataStart = codeFinish;
	const TUint32 dataFinish = dataStart + exporter.iTotalDataSize;
	const TUint32 codeDelta = exporter.iCodeRunAddress - codeStart;
	const TUint32 dataDelta = exporter.iDataRunAddress - dataStart;

	TUint32* destExport = (TUint32*)exporter.iExportDirLoad;
	for (TInt i = exporter.iExportDirCount; --i >= 0; )
		{
		TUint32 relocAddr = *destExport;
		TUint32 newValue;
		if (relocAddr >= codeStart && relocAddr < codeFinish)
			newValue = relocAddr + codeDelta;		// points to text/rdata section
		else if (relocAddr >= dataStart && relocAddr < dataFinish)
			newValue = relocAddr + dataDelta;		// points to data/bss section
		else
			newValue = relocAddr;					// unknown - just leave it alone
		*destExport++ = newValue;

		__LDRTRACE(svPrintf("RelocateExports: export %d %08x => %08x %c",
							exporter.iExportDirCount-i, relocAddr, newValue,
							(relocAddr >= codeStart && relocAddr < codeFinish) ? 'C' :
							(relocAddr >= dataStart && relocAddr < dataFinish) ? 'D' : 'X'));
		}

	return 0;
	}


struct SFixupImportAddressesInfo
	{
	TUint32* iIat;					// Next part of IAT to be fixed up
	E32Image* iExporter;			// Module from which we're importing
	TInt iNumImports;				// Number of imports from this exporter

	/**
	For demand paging, this points to the buffer which is populated
	so each page can be fixed up as it is loaded in.
	*/
	TUint64* iFixup64;
	// For ElfDerived...
	TUint32 iCodeLoadAddress;
	TUint32* iImportOffsetList;
	};


/**
Fix up the import address table, used for 'PE derived' executables.
@param aPtr Pointer to function arguments (SFixupImportAddressesInfo structure).
			SFixupImportAddressesInfo::iIat is updated by this function.

For a given importer, this function will be called once for each image from which
objects are imported, and each time it will update the relevant portion of the
importer's IAT, until all imports from all exporters have been processed.
*/
TInt svFixupImportAddresses(TAny* aPtr)
	{
	SFixupImportAddressesInfo& info = *(SFixupImportAddressesInfo*)aPtr;
	E32Image& exporter = *info.iExporter;

#ifdef _DEBUG
	__LDRTRACE(svPrintf(">svFixupImportAddresses %d imports, code@%08x, fixup@%08x exporter@%08x",
						info.iNumImports, info.iCodeLoadAddress, info.iFixup64, info.iExporter));

	// Dump everything potentially useful that we know about the exporter ...
	__LDRTRACE(svPrintf("%S: paged? %d, iRomImageHeader@%08x, iHeader@%08x",
						&exporter.iFileName, exporter.iUseCodePaging,
						exporter.iRomImageHeader, exporter.iHeader));
	__LDRTRACE(svPrintf("iCodeLoadAddress %08x, iCodeRunAddress %08x, iCodeSize %x iTextSize %x",
						exporter.iCodeLoadAddress, exporter.iCodeRunAddress,
						exporter.iCodeSize, exporter.iTextSize))
	__LDRTRACE(svPrintf("iDataLoadAddress %08x, iDataRunAddress %08x, iDataSize %x iBssSize %x iTotalDataSize %x",
						exporter.iDataLoadAddress, exporter.iDataRunAddress,
						exporter.iDataSize, exporter.iBssSize, exporter.iTotalDataSize));
	__LDRTRACE(svPrintf("iCodeDelta, %x iDataDelta %x, iExportDirEntryDelta %x",
						exporter.iCodeDelta, exporter.iDataDelta, exporter.iExportDirEntryDelta));

	if (exporter.iRomImageHeader)
		{
		const TRomImageHeader& rh = *exporter.iRomImageHeader;
		__LDRTRACE(svPrintf("ROM: iCodeAddress %08x, iCodeSize %x, iTextSize %x",
							rh.iCodeAddress, rh.iCodeSize, rh.iTextSize));
		__LDRTRACE(svPrintf("ROM: iDataAddress %08x, iDataSize %x, iBssSize %x",
							rh.iDataAddress, rh.iDataSize, rh.iBssSize));
		__LDRTRACE(svPrintf("ROM: iDataBssLinearBase %08x, iTotalDataSize %x",
							rh.iDataBssLinearBase, rh.iTotalDataSize));
		}

	if (exporter.iHeader)
		{
		const E32ImageHeader& ih = *exporter.iHeader;
		__LDRTRACE(svPrintf("HEAD: iCodeBase %08x, iCodeSize %x, iTextSize %x",
							ih.iCodeBase, ih.iCodeSize, ih.iTextSize));
		__LDRTRACE(svPrintf("HEAD: iDataBase %08x, iDataSize %x, iBssSize %x",
							ih.iDataBase, ih.iDataSize, ih.iBssSize));
		}
#endif // _DEBUG

	// 'exportDir' points to the address of the 0th ordinal (symbol name data);
	// ordinary exports start from ordinal 1
	const TUint32* const exportDir = (TUint32*)exporter.iExportDirLoad - KOrdinalBase;
	const TUint32 maxOrdinal = (TUint32)exporter.iExportDirCount;
	const TUint32 absentOrdinal = (TUint32)exporter.iFileEntryPoint;

	TUint32* iat = info.iIat;
	TUint32* const iatEnd = iat + info.iNumImports;
	for (; iat < iatEnd; ++iat)
		{
		// Each IAT slot contains the ordinal number of the export to be imported from
		// the exporter. We use that index to locate the address of the export itself.
		TUint32 ordinal = *iat;
		if (ordinal > maxOrdinal)
			return KErrNotSupported;

		// If the import number is 0 (symbol name data), and the exporter doesn't provide
		// this, we don't regard it as an error; we just skip this block, leaving the
		// address set to 0. For all other valid cases, we index the export directory to
		// find the exported object's address (which has already been relocated) ...
		TUint32 newValue = 0;
		if (ordinal > 0 || (exporter.iAttr & ECodeSegAttNmdExpData))
			{
			TUint32 expAddr = exportDir[ordinal];
			if (expAddr == 0 || expAddr == absentOrdinal)
				return KErrNotSupported;
			// The new value is just the address of the export, no adjustment needed
			newValue = expAddr;
			}

		__LDRTRACE(svPrintf("svFixupImportAddresses: import[%d]@%08x is export[%d] == %08x",
							iat - info.iIat, iat, ordinal, newValue));

		// In non-paged code, we can simply replace the ordinals in the IAT with the
		// object addresses to which they refer once and for all. However, in a code
		// paging system, the IAT may be thrown away and later reloaded from the code
		// image; therefore, we need to save the updates in the buffer pointed to by
		// 'iFixup64' so that they can be reapplied each time the code page(s)
		// containing (parts of the) IAT are reloaded. The fixup entries are in the
		// form of 64-bit words, with the 32-bit address-to-be-fixed-up in the upper
		// half and the value-to-be-stored-there in the lower half -- the multiple
		// casts are needed to stop some compilers whinging about converting a
		// pointer to a 64-bit integral type :(
		if (!info.iFixup64)
			*iat = newValue;
		else
			*info.iFixup64++ = ((TUint64)(TUintPtr)iat << 32) | newValue;
		}

	// Finally, update 'info.iIat' to show which imports have been processed
	info.iIat = iat;
	return KErrNone;
	}


/**
Fix up the import addresses, used for 'elf derived' executables.
@param aPtr Pointer to function arguments (SFixupImportAddressesInfo structure).
*/
TInt svElfDerivedFixupImportAddresses(TAny* aPtr)
	{
	SFixupImportAddressesInfo& info = *(SFixupImportAddressesInfo*)aPtr;
	E32Image& exporter = *info.iExporter;

#ifdef _DEBUG
	__LDRTRACE(svPrintf(">svElfDerivedFixupImportAddresses %d imports, code@%08x, fixup@%08x exporter@%08x",
						info.iNumImports, info.iCodeLoadAddress, info.iFixup64, info.iExporter));

	// Dump everything potentially useful that we know about the exporter ...
	__LDRTRACE(svPrintf("%S: paged? %d, iRomImageHeader@%08x, iHeader@%08x",
						&exporter.iFileName, exporter.iUseCodePaging,
						exporter.iRomImageHeader, exporter.iHeader));
	__LDRTRACE(svPrintf("iCodeLoadAddress %08x, iCodeRunAddress %08x, iCodeSize %x iTextSize %x",
						exporter.iCodeLoadAddress, exporter.iCodeRunAddress,
						exporter.iCodeSize, exporter.iTextSize))
	__LDRTRACE(svPrintf("iDataLoadAddress %08x, iDataRunAddress %08x, iDataSize %x iBssSize %x iTotalDataSize %x",
						exporter.iDataLoadAddress, exporter.iDataRunAddress,
						exporter.iDataSize, exporter.iBssSize, exporter.iTotalDataSize));
	__LDRTRACE(svPrintf("iCodeDelta, %x iDataDelta %x, iExportDirEntryDelta %x",
						exporter.iCodeDelta, exporter.iDataDelta, exporter.iExportDirEntryDelta));

	if (exporter.iRomImageHeader)
		{
		const TRomImageHeader& rh = *exporter.iRomImageHeader;
		__LDRTRACE(svPrintf("ROM: iCodeAddress %08x, iCodeSize %x, iTextSize %x",
							rh.iCodeAddress, rh.iCodeSize, rh.iTextSize));
		__LDRTRACE(svPrintf("ROM: iDataAddress %08x, iDataSize %x, iBssSize %x",
							rh.iDataAddress, rh.iDataSize, rh.iBssSize));
		__LDRTRACE(svPrintf("ROM: iDataBssLinearBase %08x, iTotalDataSize %x",
							rh.iDataBssLinearBase, rh.iTotalDataSize));
		}

	if (exporter.iHeader)
		{
		const E32ImageHeader& ih = *exporter.iHeader;
		__LDRTRACE(svPrintf("HEAD: iCodeBase %08x, iCodeSize %x, iTextSize %x",
							ih.iCodeBase, ih.iCodeSize, ih.iTextSize));
		__LDRTRACE(svPrintf("HEAD: iDataBase %08x, iDataSize %x, iBssSize %x",
							ih.iDataBase, ih.iDataSize, ih.iBssSize));
		}
#endif // _DEBUG

	// Here we calculate the bounds of each section of the exporter, as
	// code and data exports may have to be offset by different amounts.
	// Unfortunately, the required information seems to be in several
	// different places, depending on whether the code is ROM or RAM, etc
	TUint32 codeStart = exporter.iCodeRunAddress;
	TUint32 codeEnd = codeStart + exporter.iCodeSize;
	TUint32 dataStart = exporter.iDataRunAddress;
	TUint32 dataEnd = dataStart + exporter.iTotalDataSize;

	if (exporter.iRomImageHeader)
		{
		const TRomImageHeader& rh = *exporter.iRomImageHeader;
		codeStart = rh.iCodeAddress;
		codeEnd = codeStart + rh.iCodeSize;
		dataStart = rh.iDataBssLinearBase;
		dataEnd = dataStart + rh.iTotalDataSize;
		}

	if (exporter.iHeader)
		{
		const E32ImageHeader& ih = *exporter.iHeader;
		codeStart = ih.iCodeBase;
		codeEnd = codeStart + ih.iCodeSize;
		dataStart = ih.iDataBase;
		dataEnd = dataStart + ih.iDataSize + ih.iBssSize;
		}

	// 'exportDir' points to the address of the 0th ordinal (symbol name data);
	// ordinary exports start from ordinal 1
	const TUint32* const exportDir = (TUint32*)exporter.iExportDirLoad - KOrdinalBase;
	const TUint32 maxOrdinal = (TUint32)exporter.iExportDirCount;
	const TUint32 absentOrdinal = (TUint32)exporter.iFileEntryPoint;

	const TUint32 codeDelta = exporter.iCodeDelta;
	const TUint32 dataDelta = exporter.iDataDelta;
	const TUint32 dirDelta = exporter.iExportDirEntryDelta;
	TUint8* const codeBase = (TUint8*)info.iCodeLoadAddress;

	TUint32* iol = info.iImportOffsetList;
	TUint32* const iolEnd = iol + info.iNumImports;
	for(; iol < iolEnd; ++iol)
		{
		// Whereas the PE format's IAT contains ordinals to be imported, the ELF IOL
		// (Import Offset List) is a list of offsets (within the importer's code) of
		// the locations that contain references to imported objects.
		//
		// At the start of this process, each such location contains a composite value,
		// of which the low 16 bits indicate the ordinal to be imported from the
		// exporter's directory, and the upper 16 provide an optional adjustment to
		// be added to the imported value.
		//
		// This composite value has to be replaced by the actual address of the
		// object being imported (plus the adjustment factor, if any).
		TUint32 codeOffset = *iol;
		TUint32* codePtr = (TUint32*)(codeBase+codeOffset);
		TUint32 importInfo = *codePtr;
		TUint32 ordinal = importInfo & 0xffff;
		TUint32 adjustment = importInfo >> 16;
		if(ordinal > maxOrdinal)
			return KErrNotSupported;

		// If the import number is 0 (symbol name data), and the exporter doesn't provide
		// this, we don't regard it as an error; we just skip this block, leaving the
		// address set to 0. For all other valid cases, we index the export directory to find
		// the exported object's address (which may OR MAY NOT have already been relocated)
		TUint32 expAddr = 0;
		TUint32 newValue = 0;
		if (ordinal > 0 || (exporter.iAttr & ECodeSegAttNmdExpData))
			{
			expAddr = exportDir[ordinal];
			if(expAddr == 0 || expAddr == absentOrdinal)
				return KErrNotSupported;

			// If the exporter does not use code paging, then the entries in the export
			// table will already have been relocated along with its text section. In
			// the paged case, however, the relocation will have been deferred until the
			// relevant pages are (re)loaded; therefore, we have to deduce here whether
			// each export is code or data so that we can apply the correct delta ...
			TUint32 sectionDelta;
			if (!exporter.iUseCodePaging)
				sectionDelta = dirDelta;
			else if (expAddr >= codeStart && expAddr < codeEnd)
				sectionDelta = codeDelta;			// points to text/rdata section
			else if (expAddr >= dataStart && expAddr < dataEnd)
				sectionDelta = dataDelta;			// points to data/bss section
			else
				sectionDelta = dirDelta;			// unknown - assume nonpaged?
			newValue = expAddr + sectionDelta + adjustment;
			}

		__LDRTRACE(svPrintf("svElfDerivedFixupImportAddresses: import[%d] (%08x:%08x) is export[%d] %08x+%08x => %08x",
							iol - info.iImportOffsetList, codePtr, importInfo, ordinal, expAddr, adjustment, newValue));

		// In non-paged code, we can simply replace the ordinals in the IAT with the
		// object addresses to which they refer once and for all. However, in a code
		// paging system, the IAT may be thrown away and later reloaded from the code
		// image; therefore, we need to save the updates in the buffer pointed to by
		// 'iFixup64' so that they can be reapplied each time the code page(s)
		// containing (parts of the) IAT are reloaded. The fixup entries are in the
		// form of 64-bit words, with the 32-bit address-to-be-fixed-up in the upper
		// half and the value-to-be-stored-there in the lower half -- the multiple
		// casts are needed to stop some compilers whinging about converting a
		// pointer to a 64-bit integral type :(
		if (!info.iFixup64)
			*codePtr = newValue;
		else
			*info.iFixup64++ = ((TUint64)(TUintPtr)codePtr << 32) | newValue;
		}

	return KErrNone;
	}


/**
Wrapper for memory copy arguments.
*/
struct SCopyDataInfo
	{
	TAny* iDest;
	const TAny* iSource;
	TInt iNumberOfBytes;
	};


/**
Copies word aligned memory.
@param aPtr Pointer to function arguments (SCopyDataInfo structure).
*/
TInt svWordCopy(TAny* aPtr)
	{
	SCopyDataInfo& info=*(SCopyDataInfo*)aPtr;
	return (TInt) Mem::Move(info.iDest, info.iSource, info.iNumberOfBytes);
	}


/**
Copies memory.
@param aPtr Pointer to function arguments (SCopyDataInfo structure).
*/
TInt svMemCopy(TAny* aPtr)
	{
	SCopyDataInfo& info=*(SCopyDataInfo*)aPtr;
	return (TInt) Mem::Copy(info.iDest, info.iSource, info.iNumberOfBytes);
	}


/**
Argument for svElfDerivedGetImportInfo.
*/
struct SGetImportDataInfo
	{
	TInt iCount;					// number to extract
	TUint32* iDest;					// destination address for data
	TUint32 iCodeLoadAddress;		// address where code has been loaded
	TUint32* iImportOffsetList;		// pointer to list of import offsets in E32ImportBlock
	};

/**
Extract import ordinals/data
@param aPtr Pointer to function arguments (SGetImportDataInfo structure).
*/
TInt svElfDerivedGetImportInfo(TAny* aPtr)
	{
	SGetImportDataInfo& info = *(SGetImportDataInfo*)aPtr;
	TInt count = info.iCount;
	TUint32* dest = info.iDest;
	TUint32 code = info.iCodeLoadAddress;
	TUint32* iol = info.iImportOffsetList;

	TUint32* iolEnd = iol+count;
	while(iol<iolEnd)
		*dest++ = *(TUint32*)(code + *iol++);
		
	return 0;
	}

/*******************************************************************************
 * End of supervisor mode functions
 ******************************************************************************/


/*******************************************************************************
 * RImageInfo
 ******************************************************************************/
RImageInfo::RImageInfo()
	{
	memclr(this, sizeof(RImageInfo));
	}

void RImageInfo::Close()
	{
	iFile.Close();
	delete iHeader;
	iHeader=NULL;
	gFileDataAllocator.Free(iFileData);
	iFileData=NULL;
	}

void RImageInfo::Accept(RImageInfo& aInfo)
	{
	Close();
	wordmove(this, &aInfo, sizeof(RImageInfo));
	memclr(&aInfo.iFile, (_FOFF(RImageInfo,iFileSize) - _FOFF(RImageInfo,iFile)) );
	}

/*******************************************************************************
 * EPOC executable file finders
 ******************************************************************************/
RImageFinder::RImageFinder()
	:	iNameMatches(0), iUidFail(0), iCapFail(0), iMajorVersionFail(0), iImportFail(0),
		iCurrentVersion(KModuleVersionNull), iCurrentDrive(0), iFindExact(0), iNewValid(0),
		iReq(0), iExisting(0)
	{
	}

TInt RImageFinder::Set(const RLdrReq& aReq)
	{
	iReq = &aReq;
	TInt l = aReq.iFileNameInfo.BaseLen() + aReq.iFileNameInfo.ExtLen();
	if (l > KMaxProcessName)
		return KErrBadName;
	aReq.iFileNameInfo.GetName(iRootName, TFileNameInfo::EIncludeBaseExt);
	return KErrNone;
	}

void RImageFinder::Close()
	{
	iNew.Close();
	}

_LIT8(KDefaultPathSysBin, "sys\\bin");
_LIT8(KDefaultPathSysBin2, "?:\\sys\\bin");
_LIT8(KDefaultExePath, "sys\\bin;system\\bin;system\\programs;system\\libs");
_LIT8(KDefaultDllPath, "sys\\bin;system\\bin;system\\libs");
_LIT8(KDefaultExePath2, "?:\\sys\\bin;?:\\system\\bin;?:\\system\\programs;?:\\system\\libs");
_LIT8(KDefaultDllPath2, "?:\\sys\\bin;?:\\system\\bin;?:\\system\\libs");

TInt RImageFinder::Search()
	{
	__LDRTRACE(iReq->Dump(">RImageFinder::Search"));
	TBool exe = (iReq->iRequestedUids[0] == KExecutableImageUid);
	const TFileNameInfo& fi = iReq->iFileNameInfo;
	TInt r = KErrNone;
	if (fi.PathLen())
		{
		// path specified, so only look there
		TPtrC8 drive_and_path(fi.DriveAndPath());
		r = Search(&drive_and_path, 0);
		}
	else
		{
		TInt drv = -1;
		if (fi.DriveLen())
			{
			// drive specified
			drv = (*iReq->iFileName)[0];
			}
		// if a search path is specified look there
		if (iReq->iPath)
			r = Search(iReq->iPath, drv);
		if (r == KErrNoMemory) // ignore other errors as they are a potential denial of service
			{
			__LDRTRACE(Dump("<RImageFinder::Search", r));
			return r;
			}
		const TDesC8* defpath;
		if(PlatSec::ConfigSetting(PlatSec::EPlatSecEnforceSysBin))
			defpath = (drv<0) ? &KDefaultPathSysBin() : &KDefaultPathSysBin2();
		else
			{
			if (drv<0)
				defpath = exe ? &KDefaultExePath() : &KDefaultDllPath();
			else
				defpath = exe ? &KDefaultExePath2() : &KDefaultDllPath2();
			}
		r = Search(defpath, drv);
		}
	if (r == KErrNoMemory)  // ignore other errors as they are a potential denial of service
		{
		__LDRTRACE(Dump("<RImageFinder::Search", r));
		return r;
		}
	if (iExisting || iNewValid)
		r = KErrNone;			// found something suitable
	else if (!iNameMatches)
		r = KErrNotFound;		// nothing matched requested name
	else if (iImportFail || iMajorVersionFail)
		r = KErrNotSupported;	// something failed only on missing imports or version
	else if (iCapFail)
		r = KErrPermissionDenied;	// something failed capability check
	else if (iUidFail)
		r = KErrNotSupported;	// something failed UID check
	else
		r = KErrCorrupt;		// a file had the correct name but was not a valid E32Image file
	__LDRTRACE(Dump("<RImageFinder::Search", r));
	return r;
	}

TInt RImageFinder::Search(const TDesC8* aPath, TInt aDrive)
	{
	__IF_DEBUG(Printf(">Path %S Drive %02x", aPath, aDrive));
	TInt ppos = 0;
	TInt plen = aPath->Length();
	while (ppos < plen)
		{
		TPtrC8 remain(aPath->Mid(ppos));
		TInt pel = remain.Locate(';');
		if (pel < 0)
			{
			pel = remain.Length();
			ppos += pel;
			}
		else
			{
			ppos += pel + 1;
			}
		if (pel == 0)
			continue;
		TBool alldrives = EFalse;
		if (pel<2 || remain[1]!=':')
			alldrives = ETrue;
		else if (remain[0]!='?')
			aDrive = remain[0];
		TInt drive = EDriveY;
		if (!alldrives && RFs::CharToDrive(TChar(aDrive), drive)!=KErrNone)
			continue;
		iCurrentDrive = (TUint8)drive;
		TInt startpos = alldrives ? 0 : 2;
		iCurrentPath.Set(remain.Mid(startpos, pel - startpos));
		do	{
			TInt r;
#ifdef __X86__
			if (alldrives && iCurrentDrive<=EDriveB && iCurrentDrive!=UseFloppy)
				goto bypass_drive;
#endif
			r = SearchSingleDir();
			if (r == KErrNoMemory) // ignore other errors as they are a potential denial of service
				{
				__IF_DEBUG(Printf("OOM!"));
				return r;
				}
#ifdef __X86__
bypass_drive:
#endif
			if (!iCurrentDrive--)
				iCurrentDrive = EDriveZ;
			} while(alldrives && iCurrentDrive != EDriveY);
		}
	__IF_DEBUG(Printf("<Path %S Drive %02x", aPath, aDrive));
	return KErrNone;
	}

// Can't be looking for main loadee here, so iReq->iImporter is never NULL
// Also gExeAttr must be set up
TInt RImageFinder::SearchExisting(const RImageArray& aArray)
	{
	__IF_DEBUG(Printf(">RImageFinder::SearchExisting"));
	TUint required_abi = gExeAttr & ECodeSegAttABIMask;
	TInt first, last, i;
	aArray.Find(iRootName, first, last);
	for (i=first; i<last; ++i)
		{
		E32Image* e = aArray[i];
		if (CheckUids(e->iUids, iReq->iRequestedUids) != KErrNone)
			continue;
		if (iReq->CheckSecInfo(e->iS) != KErrNone)
			continue;
		TInt action = DetailedCompareVersions(e->iModuleVersion, iReq->iRequestedVersion, iCurrentVersion, EFalse);
		if (action == EAction_Skip)
			continue;
		if (action == EAction_CheckImports || action == EAction_CheckLastImport)
			{
			// Never optimistically link to something with a different ABI
			if ((e->iAttr & ECodeSegAttABIMask) != required_abi)
				continue;
			TInt r = CheckRequiredImports(iReq->iImporter, e, action);
			if (r != KErrNone)
				{
				if (r != KErrNotSupported)
					return r;
				continue;
				}
			}
		iExisting = e;
		iCurrentVersion = e->iModuleVersion;
		}
	__IF_DEBUG(Printf("<RImageFinder::SearchExisting"));
	return KErrNone;
	}

// Called for each file found with matching root name but which is not a valid E32ImageFile
void RImageFinder::RecordCorruptFile()
	{
	__IF_DEBUG(Printf("RImageFinder::RecordCorruptFile"));
	++iNameMatches;
	}

// Called for each valid E32Image file found with matching root name
TInt RImageFinder::Try(RImageInfo& aInfo, const TDesC8& aRootName, const TDesC8& aDriveAndPath)
	{
	__IF_DEBUG(Printf(">RImageFinder::Try %S%S", &aDriveAndPath, &aRootName));
	__IF_DEBUG(Printf(">MA:%08x MV:%08x RV:%08x CV:%08x", aInfo.iAttr, aInfo.iModuleVersion, iReq->iRequestedVersion, iCurrentVersion));
	++iNameMatches;
	if (iFindExact)
		{
		if ( ((aInfo.iAttr & ECodeSegAttExpVer) && aInfo.iModuleVersion==iReq->iRequestedVersion)
			|| (!(aInfo.iAttr & ECodeSegAttExpVer) && iReq->iRequestedVersion==KModuleVersionWild)
			)
			{
			__IF_DEBUG(Printf("<RImageFinder::Try Exact Match Found"));
			iNewValid = 1;
			iNew.Accept(aInfo);
			SetName(aRootName, aDriveAndPath);
			return KErrCompletion;
			}
		return KErrNotFound;
		}
	TUint required_abi = gExeAttr & ECodeSegAttABIMask;
	TBool abi_mismatch = ((aInfo.iAttr & ECodeSegAttABIMask)!=required_abi);
	TInt32* uid = (TInt32*)&iReq->iRequestedUids;
	TBool dll_wanted = (uid[0] == KDynamicLibraryUidValue);
	if (CheckUids(*(TUidType*)aInfo.iUid, iReq->iRequestedUids) != KErrNone)
		{
		++iUidFail;
		__IF_DEBUG(Printf("<RImageFinder::Try UIDFAIL"));
		return KErrNotFound;
		}
	if (iReq->CheckSecInfo(aInfo.iS) != KErrNone)
		{
		++iCapFail;
		__IF_DEBUG(Printf("<RImageFinder::Try CAPFAIL"));
		return KErrNotFound;
		}
	TInt action = DetailedCompareVersions(aInfo.iModuleVersion, iReq->iRequestedVersion, iCurrentVersion, !iReq->iImporter);
	if (action == EAction_Skip)
		{
		if (DetailedCompareVersions(aInfo.iModuleVersion, iReq->iRequestedVersion) == EVersion_MajorSmaller)
			++iMajorVersionFail;
		__IF_DEBUG(Printf("<RImageFinder::Try VERFAIL"));
		return KErrNotFound;
		}
	if (action == EAction_CheckImports || action == EAction_CheckLastImport)
		{
		// If we get here, can't be main loadee so gExeAttr must be valid
		// Never optimistically link to something with a different ABI
		if (abi_mismatch || CheckRequiredImports(iReq->iImporter, aInfo, action)!=KErrNone)
			{
			__IF_DEBUG(Printf("<RImageFinder::Try IMPFAIL"));
			++iImportFail;
			return KErrNotFound;
			}
		}
	if (!iReq->iImporter && dll_wanted && abi_mismatch)
		{
		// Dynamically loading a DLL - ABI must match loading process
		__IF_DEBUG(Printf("<RImageFinder::Try ABIFAIL"));
		++iImportFail;
		return KErrNotFound;
		}
	if(PlatSec::ConfigSetting(PlatSec::EPlatSecEnforceSysBin))
		{
		TChar driveLetter;
		TInt driveNumber;
		TInt r;
		driveLetter=(TChar)aDriveAndPath[0];
		RFs::CharToDrive(driveLetter,driveNumber);
		TDriveCacheHeader* pDH=gDriveFileNamesCache[driveNumber];
		TUint driveAtt=0;
		if(pDH)
			driveAtt=pDH->iDriveAtt;
		else
			{
			TDriveInfo driveInfo;
			if ((r=gTheLoaderFs.Drive(driveInfo,driveNumber)) != KErrNone)
				{
				__IF_DEBUG(Printf("<RImageFinder::Try DINFFAIL"));
				++iImportFail;
				return r;
				}
			driveAtt=driveInfo.iDriveAtt;
			}

		if(driveAtt & KDriveAttRemovable)
			{
			__IF_DEBUG(Printf("** RImageFinder::Try %S%S is on a removable drive", &aDriveAndPath, &aRootName));
			// If the cache says we already checked the hash of this file, accept it without checking again
			// as any *legitimate* change to the file would've triggered the cache to be rebuilt.
			if (!(aInfo.iCacheStatus & TImageInfo::EHashChecked))
				{
				//We have to pass aDriveAndPath as aInfo may not contain Drive
				TRAP(r,CompareHashL(aInfo, aDriveAndPath));
				if (r == KErrNoMemory)
					return r;
				if(r!=KErrNone)
					{
					__IF_DEBUG(Printf("<RImageFinder::Try Compare Hash Failed"));
					iCapFail++;
					return r;
					}
				aInfo.iCacheStatus |= TImageInfo::EHashChecked;
				}
			else
				{
				// We've skipped hash checking as an optimisation, however someone could potentially have
				// used external hardware to switch the data on the card since the cached hash check. Setting
				// this mark means that if we actually load the file, we'll hash it then; but if it turns out
				// to be already loaded, we can save the effort.
				aInfo.iNeedHashCheck = 1;
				}
			}
		}
	iExisting = NULL;
	iNew.Accept(aInfo);
	iNewValid = 1;
	iCurrentVersion = aInfo.iModuleVersion;
	SetName(aRootName, aDriveAndPath);
	__IF_DEBUG(Printf("<MV:%08x RV:%08x CV:%08x", aInfo.iModuleVersion, iReq->iRequestedVersion, iCurrentVersion));
	__IF_DEBUG(Printf("<RImageFinder::Try OK"));
	return KErrNone;
	}

void RImageFinder::CompareHashL(RImageInfo& aInfo, const TDesC8& aDriveAndPath)
//
//	Calculate hash and compare after checking if one already exists in c:/system/caps
//
	{
	__IF_DEBUG(Printf(">RImageFinder::CompareHashL"));
	
	TInt extraFlag = 0;
	TBuf8<KMaxFileName*sizeof(TText)> fileName;
	TFileNameInfo fni = iReq->iFileNameInfo;
	if (aInfo.iAttr & ECodeSegAttExpVer)
		{
		fni.iVersion = aInfo.iModuleVersion;
		extraFlag = TFileNameInfo::EForceVer;				
		}
	
	TFileName hashname(KSysHash);
    hashname[0] = (TUint8) RFs::GetSystemDriveChar();
	fileName.SetLength(0);
	fni.GetName(fileName, TFileNameInfo::EIncludeBaseExt | extraFlag);
	hashname.Append(fileName.Expand());

	RFile fHash;
	CleanupClosePushL(fHash);

	__IF_DEBUG(Printf("RImageFinder::CompareHashL opening hash file %S ", &hashname));
	User::LeaveIfError(fHash.Open(gTheLoaderFs,hashname,EFileRead|EFileReadDirectIO));

	TBuf8<SHA1_HASH> installhash;
	User::LeaveIfError(fHash.Read(installhash));
	CleanupStack::PopAndDestroy(1);

	// if we get this far, we have loaded a valid hash, so calculate the file's hash

	CSHA1* hasher=CSHA1::NewL(); 
	CleanupStack::PushL(hasher);

	fileName.Copy(aDriveAndPath);
	fni.GetName(fileName, TFileNameInfo::EIncludeBaseExt | extraFlag);

	CleanupClosePushL(aInfo.iFile);
	TBool b = aInfo.FileOpened();
	if(!b)		
		{
		__IF_DEBUG(Printf("RImageFinder::CompareHashL opening the file %S", &fileName));
		User::LeaveIfError(aInfo.iFile.Open(gTheLoaderFs, fileName.Expand(), EFileRead|EFileReadDirectIO));
		}
	
	__IF_DEBUG(Printf("RImageFinder::CompareHashL calculate hash"));
	TInt size;
	User::LeaveIfError(aInfo.iFile.Size(size));
	aInfo.iFileData = (TUint8*)gFileDataAllocator.Alloc(size);
	if (aInfo.iFileData)
		aInfo.iFileSize = size;
	else
		User::Leave(KErrNoMemory);
	TPtr8 filedata(aInfo.iFileData, size);
	User::LeaveIfError(aInfo.iFile.Read(0, filedata, size));
	if (filedata.Length() != size)
		User::Leave(KErrCorrupt);
	CleanupStack::PopAndDestroy(1);	//the file handle only->aInfo.iFile.Close();
	hasher->Update(filedata);
		
	TBuf8<SHA1_HASH> hash;
	hash=hasher->Final(); 


	__IF_DEBUG(Printf("RImageFinder::CompareHashL comparing hashes..."));
	if(0 != hash.Compare(installhash))
		User::Leave(KErrPermissionDenied);
	CleanupStack::PopAndDestroy(1);
	
	// if we get this far the hash has passed and the file has been closed
	// but some of the RImageInfo parameters will've been initialised by the cache
	// and may be lies if we're being attacked, so compare them to be sure

	// if we already had the header, throw it away: it's from untrusted data
	if (aInfo.iHeader)
		{
		delete aInfo.iHeader;
		aInfo.iHeader = NULL;
		}

	// make the header and validate the cached parameters against it
	User::LeaveIfError(E32ImageHeader::New(aInfo.iHeader, aInfo.iFileData, aInfo.iFileSize));

	SSecurityInfo secinfo;
	aInfo.iHeader->GetSecurityInfo(secinfo);
	TUint32 attr = (aInfo.iHeader->iFlags & ECodeSegAttFixed) | aInfo.iHeader->ABI();
	if(aInfo.iHeader->iFlags&KImageNmdExpData)
		attr |= ECodeSegAttNmdExpData;
	if (Mem::Compare((TUint8*)aInfo.iUid, sizeof(aInfo.iUid), (TUint8*)&aInfo.iHeader->iUid1, sizeof(aInfo.iUid))
			|| aInfo.iModuleVersion != aInfo.iHeader->ModuleVersion()
			|| Mem::Compare((TUint8*)&aInfo.iS, sizeof(aInfo.iS), (TUint8*)&secinfo, sizeof(secinfo))
			|| (aInfo.iAttr & ~ECodeSegAttExpVer) != attr)
		User::Leave(KErrPermissionDenied);

	__IF_DEBUG(Printf("<RImageFinder::CompareHashL passed"));
	}

void RImageFinder::SetName(const TDesC8& aRootName, const TDesC8& aDriveAndPath)
	{
	iNewFileName = aDriveAndPath;
	iNewFileName.Append(aRootName);
	}

RImageArray::RImageArray()
	:	RPointerArray<E32Image>(8, 2*256)
	{
	}

TInt RImageArray::Add(E32Image* aImage)
	{
	return InsertInOrderAllowRepeats(aImage, &E32Image::Order);
	}

void RImageArray::Find(const TDesC8& aRootName, TInt& aFirst, TInt& aLast) const
	{
	TCodeSegCreateInfo name;
	name.iFileName.Copy(aRootName);
	name.iRootNameOffset = 0;
	name.iRootNameLength = aRootName.Length();
	aFirst = SpecificFindInOrder((const E32Image*)&name, &E32Image::Order, EArrayFindMode_First);
	aLast = aFirst;
	if (aFirst >= 0)
		aLast = SpecificFindInOrder((const E32Image*)&name, &E32Image::Order, EArrayFindMode_Last);
	}

E32Image* RImageArray::Find(const TRomImageHeader* a) const
	{
	TInt c = Count();
	if (!c)
		return NULL;
	E32Image* const * ee = &(*this)[0];
	E32Image* const * eE = ee + c;
	for (; ee<eE && (*ee)->iRomImageHeader != a; ++ee) {}
	return (ee<eE) ? *ee : NULL;
	}

TInt E32Image::LoadProcess(const RLdrReq& aReq)
	{
	__LDRTRACE(aReq.Dump("E32Image::LoadProcess"));

	RImageFinder finder;
	TInt r = finder.Set(aReq);
	if (r == KErrNone)
		r = finder.Search();
	if (r!=KErrNone)
		{
		finder.Close();
		return r;
		}
	r = Construct(finder);	// needs to find it if it's already loaded
	finder.Close();
	if (r!=KErrNone)
		{
		return r;
		}
	if (iIsDll)
		return KErrNotSupported;
	r = aReq.iMsg->Client((RThread&)aReq.iClientThread);
	if (r!=KErrNone)
		{
		return r;
		}
	iClientHandle=aReq.iClientThread.Handle();
	
	if(iStackSize < aReq.iMinStackSize)
		iStackSize=aReq.iMinStackSize;		// If the process required larger stack than the default.

	//initialise to zero
#ifdef _DEBUG
	iDestructStat = ProcessDestructStatPtr;
#endif
	iDebugAttributes = 0;
	if (iRomImageHeader)
		{
		if (iRomImageHeader->iFlags & KRomImageDebuggable)
			iDebugAttributes |= EDebugAllowed;
		}
	else if (iHeader)
		{
		if (iHeader->iFlags & KImageDebuggable)
			iDebugAttributes |= EDebugAllowed;
		}
	
	// Get the data paging flags and pass to the kernel.
	__ASSERT_COMPILE(EDataPagingUnspecified == 0);
	if (iRomImageHeader)
		{
		TUint dataPaging = iRomImageHeader->iFlags & KRomImageDataPagingMask;
		if (dataPaging == KRomImageDataPagingMask)
			RETURN_FAILURE(KErrCorrupt);
		if (dataPaging == KRomImageFlagDataPaged)
			iFlags |= EDataPaged;
		if (dataPaging == KRomImageFlagDataUnpaged)
			iFlags |= EDataUnpaged;
		}
	else if (iHeader)
		{
		TUint dataPaging = iHeader->iFlags & KImageDataPagingMask;
		if (dataPaging == KImageDataPagingMask)
			RETURN_FAILURE(KErrCorrupt);
		if (dataPaging == KImageDataPaged)
			iFlags |= EDataPaged;
		if (dataPaging == KImageDataUnpaged)
			iFlags |= EDataUnpaged;
		}
		
	r=E32Loader::ProcessCreate(*this, aReq.iCmd);
	__IF_DEBUG(Printf("Done E32Loader::ProcessCreate %d",r));
	if (r!=KErrNone)
		{
		return r;
		}
#ifdef _DEBUG
	ProcessCreated = ETrue;
#endif
	iClientProcessHandle=iProcessHandle;
	if (!iAlreadyLoaded)
		{
		gExeCodeSeg=iHandle;	// implicitly linked DLLs must load into the new process
		gExeAttr=iAttr;
		if (!iRomImageHeader)
			r=LoadToRam();
		if (r==KErrNone)
			r=ProcessImports();	// this sets up gLoadeePath
		}
	// transfers ownership of clamp handle to codeseg; nulls handle if successful
	if (r==KErrNone)
		{
		r=E32Loader::ProcessLoaded(*this);
		if ((r==KErrNone) && iUseCodePaging)
			{
			iFileClamp.iCookie[0]=0;// null handle to indicate 
			iFileClamp.iCookie[1]=0;// transfer of ownership of clamp handle to proc's codeseg
			}
		}
	__IF_DEBUG(Printf("Done E32Image::LoadProcess %d",r));
	return r;
	}

// Load a code segment, plus all imports if main loadee
TInt E32Image::LoadCodeSeg(const RLdrReq& aReq)
	{
	__LDRTRACE(aReq.Dump(">E32Image::LoadCodeSeg"));

#ifdef __X86__
	if (iMain==this && iClientProcessHandle)
		{
		RProcess p;
		p.SetHandle(iClientProcessHandle);
		TFileName f(p.FileName());
		if (f.Length()>=2 && f[1]==':')
			{
			TInt d = f[0];
			if (d=='a' || d=='A')
				UseFloppy = EDriveA;
			else if (d=='b' || d=='B')
				UseFloppy = EDriveB;
			}
		}
#endif

	RImageFinder finder;
	TInt r = finder.Set(aReq);
	if (r == KErrNone)
		r = finder.Search();
	if (r!=KErrNone)
		{
		finder.Close();
		return r;
		}
	return DoLoadCodeSeg(aReq, finder);
	}

// Load a code segment, plus all imports if main loadee
TInt E32Image::DoLoadCodeSeg(const RLdrReq& aReq, RImageFinder& aFinder)
	{
	__LDRTRACE(aReq.Dump(">E32Image::DoLoadCodeSeg"));

	TInt r = Construct(aFinder);	// needs to find it if it's already loaded
	aFinder.Close();
	if (r!=KErrNone)
		{
		return r;
		}
	__IF_DEBUG(Printf("epv=%x, fep=%x, codesize=%x, textsize=%x, uid3=%x",iEntryPtVeneer,iFileEntryPoint,iCodeSize,iTextSize,iUids[2]));
	__IF_DEBUG(Printf("attr=%08x, gExeAttr=%08x",iAttr,gExeAttr));

	// If EXE and not main loadee, EXE code segment must be the same as the client process or newly loaded process
	if (gExeCodeSeg && !iIsDll && iMain!=this && iHandle!=gExeCodeSeg)
		return KErrNotSupported;

	// If DLL and main loadee, ABI must match the process
	if (iIsDll && iMain==this && (iAttr & ECodeSegAttABIMask)!=(gExeAttr & ECodeSegAttABIMask) )
		return KErrNotSupported;

	// code segment already loaded
	if (iAlreadyLoaded || (iMain!=this && AlwaysLoaded()) )
		return KErrNone;

	__IF_DEBUG(Printf("CodeSeg create"));
	r=E32Loader::CodeSegCreate(*this);
	if (r!=KErrNone)
		return r;
	
	iCloseCodeSeg=iHandle;	// so new code segment is removed if the load fails
	if (!iRomImageHeader)
		r=LoadToRam();
	if (r==KErrNone)
		{
		iCloseCodeSeg=NULL;
		if (iMain==this)
			{
			r=ProcessImports();	// this sets up gLoadeePath
			// transfers ownership of clamp handle to codeseg; nulls handle if successful
			if (r==KErrNone)
				{
				r=E32Loader::CodeSegLoaded(*this);
				if ((r==KErrNone) && iUseCodePaging)
					{
					iFileClamp.iCookie[0]=0;// null handle to indicate 
					iFileClamp.iCookie[1]=0;// transfer of ownership of clamp handle to codeseg
					}
				}
			}
		}

	__IF_DEBUG(Printf("<DoLoadCodeSeg, r=%d, iIsDll=%d",r,iIsDll));
	return r;
	}

// Load a ROM XIP code segment as part of another load
TInt E32Image::DoLoadCodeSeg(const TRomImageHeader& a)
	{
	__IF_DEBUG(Printf("E32Image::DoLoadCodeSeg ROM XIP @%08x",&a));
	
	Construct(a);
	if (AlwaysLoaded())
		{
		GetRomFileName();
		return KErrNone;
		}
	TInt r=CheckRomXIPAlreadyLoaded();
	if (r!=KErrNone || iAlreadyLoaded)
		{
		return r;
		}
	GetRomFileName();
	r=E32Loader::CodeSegCreate(*this);

	__IF_DEBUG(Printf("<DoLoadCodeSeg, r=%d",r));
	return r;
	}

/******************************************************************************
 * EPOC specific E32Image functions
 ******************************************************************************/

/**
Construct an image object which represents an XIP ROM executable.
*/
void E32Image::Construct(const TRomImageHeader& a)
	{
	__IF_DEBUG(Printf("E32Image::Construct ROM %08x",&a));

	iRomImageHeader = &a;
	iUids = *(const TUidType*)&a.iUid1;
	iS = a.iS;
	iCodeSize = a.iCodeSize;
	iTextSize = a.iTextSize;
	iDataSize = a.iDataSize;
	iBssSize = a.iBssSize;
	iTotalDataSize = a.iTotalDataSize;
	iEntryPtVeneer = 0;
	iFileEntryPoint = a.iEntryPoint;
	iDepCount = a.iDllRefTable ? a.iDllRefTable->iNumberOfEntries : 0;
	iExportDir = a.iExportDir;
	iExportDirCount = a.iExportDirCount;
	iCodeLoadAddress = (TUint32)&a;
	iDataRunAddress = a.iDataBssLinearBase;	// for fixed processes
	iHeapSizeMin = a.iHeapSizeMin;
	iHeapSizeMax = a.iHeapSizeMax;
	iStackSize = a.iStackSize;
	iPriority = a.iPriority;
	iIsDll = (a.iFlags & KImageDll)!=0;
	if(iExportDirCount)
		iExportDirLoad = iExportDir;

	// setup attributes...
	iAttr &= ~(ECodeSegAttKernel|ECodeSegAttGlobal|ECodeSegAttFixed|ECodeSegAttABIMask|ECodeSegAttNmdExpData);
	if(a.iFlags&KRomImageFlagsKernelMask)
		iAttr |= ECodeSegAttKernel;
	else
		iAttr |= ECodeSegAttGlobal;
	if(a.iFlags&KRomImageFlagFixedAddressExe)
		iAttr |= ECodeSegAttFixed;
	iAttr |= (a.iFlags & KRomImageABIMask);
	if(a.iFlags&KRomImageNmdExpData)
		iAttr |= ECodeSegAttNmdExpData;
	if(a.iFlags&KRomImageSMPSafe)
		iAttr |= ECodeSegAttSMPSafe;
	
	iExceptionDescriptor = a.iExceptionDescriptor;
	}


TBool E32Image::AlwaysLoaded()
	{
	// If loaded from ROM and EXE or DLL with no static data or extension or variant, don't need code segment
	TBool r=EFalse;
	__IF_DEBUG(Printf(">E32Image::AlwaysLoaded %08x",iRomImageHeader));
	if (iRomImageHeader)
		{
		if (iIsDll && (iRomImageHeader->iFlags & KRomImageFlagDataPresent)==0)
			r=ETrue;
		}
	__IF_DEBUG(Printf("<E32Image::AlwaysLoaded %x",r));
	return r;
	}


void E32Image::GetRomFileName()
	{
	TBuf8<KMaxFileName> fn = _S8("z:\\");
	TFileNameInfo fni;
	TPtr8 path_and_name(((TText8*)fn.Ptr())+3, 0, KMaxFileName-3);
	const TRomDir& rootdir = *(const TRomDir*)UserSvr::RomRootDirectoryAddress();
	if (!TraverseDirs(rootdir, iRomImageHeader, path_and_name))
		*(const TAny**)1=iRomImageHeader;	// DIE!
	fn.SetLength(path_and_name.Length()+3);
	fni.Set(fn, 0);
	iFileName.Zero();
	fni.GetName(iFileName, TFileNameInfo::EIncludeDrivePathBaseExt);
	if (fni.VerLen())
		iAttr |= ECodeSegAttExpVer;
	iRootNameOffset = fni.iBasePos;
	iRootNameLength = fni.BaseLen() + fni.ExtLen();
	iExtOffset = iFileName.Length() - fni.ExtLen();
	__IF_DEBUG(Printf("GetRomFileName(%08x)->%S,%d,%d,%d Attr %08x",iRomImageHeader,&iFileName,iRootNameOffset,iRootNameLength,iExtOffset,iAttr));
	}


/**
Starting from aDir, search for XIP executable specified by aHdr.
If found, return true and set aName to file path and name, (will cause descriptor panics if max size of aName isn't big enough.)
If not found, return false.
*/
TBool E32Image::TraverseDirs(const TRomDir& aDir, const TRomImageHeader* aHdr, TDes8& aName)
	{
	const TRomEntry* pE=&aDir.iEntry;
	const TRomEntry* pEnd=(const TRomEntry*)((TUint8*)pE+aDir.iSize);
	while(pE<pEnd)
		{
		if ( (pE->iAtt & KEntryAttXIP) && (pE->iAddressLin==(TLinAddr)aHdr) )
			{
			// ROM XIP file found
			aName.Copy(TPtrC16((const TText*)pE->iName, pE->iNameLength));
			return ETrue;
			}
		if (pE->iAtt & KEntryAttDir)
			{
			// subdirectory found
			const TRomDir& subdir = *(const TRomDir*)pE->iAddressLin;
			TText8* p = (TText8*)aName.Ptr();
			TInt m = aName.MaxLength();
			TInt nl = pE->iNameLength;
			TPtr8 ptr(p+nl+1, 0, m-nl-1);
			if (TraverseDirs(subdir, aHdr, ptr))
				{
				// match found in subdirectory
				aName.SetLength(ptr.Length()+nl+1);
				const TText* s = (const TText*)pE->iName;
				p[nl]='\\';
				while (nl--)
					*p++ = (TText8)*s++;
				return ETrue;
				}
			}
		TInt entry_size = KRomEntrySize + pE->iNameLength*sizeof(TText);
		entry_size = (entry_size+sizeof(TInt)-1)&~(sizeof(TInt)-1);
		pE=(const TRomEntry*)((TUint8*)pE+entry_size);
		}
	return EFalse;
	}


/**
Read data from a file.
*/
TInt FileRead(RFile& aFile, TUint8* aDest, TInt aSize)
	{
	TPtr8 p(aDest,aSize,aSize);
	TInt r = aFile.Read(p,aSize);
	if(r==KErrNone && p.Size()!=aSize)
		RETURN_FAILURE(KErrCorrupt);
	return r;
	}


/**
Construct a new image header by reading a file. File must not be XIP.
*/
TInt E32ImageHeader::New(E32ImageHeader*& aHdr, RFile& aFile)
	{
	aHdr = NULL;

	TInt fileSize;
	TInt r = aFile.Size(fileSize);
	if(r!=KErrNone)
		return r;

	E32ImageHeaderV tempHeader;
	r = FileRead(aFile, (TUint8*)&tempHeader, sizeof(tempHeader));
	if(r!=KErrNone)
		return r;

	TUint headerSize = tempHeader.TotalSize();
	if(headerSize<sizeof(tempHeader) || headerSize>TUint(KMaxHeaderSize))
		RETURN_FAILURE(KErrCorrupt);

	E32ImageHeaderV* header = (E32ImageHeaderV*)User::Alloc(headerSize);
	if(!header)
		return KErrNoMemory;

	wordmove(header, &tempHeader, sizeof(tempHeader));
	if(headerSize>sizeof(tempHeader))
		r = FileRead(aFile, ((TUint8*)header)+sizeof(tempHeader), headerSize-sizeof(tempHeader));

	if(r==KErrNone)
		r = header->ValidateAndAdjust(fileSize);

	if(r==KErrNone)
		aHdr = header;
	else
		delete header;

	return r;
	}


/**
Construct a new image header using data from the supplied buffer.
*/
TInt E32ImageHeader::New(E32ImageHeader*& aHdr, TUint8* aFileData, TUint32 aFileSize)
	{
	aHdr = NULL;

	E32ImageHeaderV& tempHeader = *(E32ImageHeaderV*)aFileData;

	if(aFileSize<sizeof(tempHeader))
		RETURN_FAILURE(KErrCorrupt); // too small to contain a header

	TUint headerSize = tempHeader.TotalSize();
	if(headerSize<sizeof(tempHeader) || headerSize>TUint(KMaxHeaderSize))
		RETURN_FAILURE(KErrCorrupt);
	if(headerSize>aFileSize)
		RETURN_FAILURE(KErrCorrupt);

	E32ImageHeaderV* header = (E32ImageHeaderV*)User::Alloc(headerSize);
	if(!header)
		return KErrNoMemory;

	wordmove(header, &tempHeader, headerSize);

	TInt r = header->ValidateAndAdjust(aFileSize);
	if(r==KErrNone)
		aHdr = header;
	else
		delete header;

	return r;
	}


/**
Validate header, then adjust:
- iUncompressedSize to contain size of data even when file is not compressed.
- Platform security capability to include all disabled capabilities and exclude invalid ones.

@param aFileSize Total size of the file containing the image data.
*/
TInt E32ImageHeaderV::ValidateAndAdjust(TUint32 aFileSize)
	{
	// check header is valid...
	TUint32 uncompressedSize;
	TInt r = ValidateHeader(aFileSize,uncompressedSize);
	if(r!=KErrNone)
		return r;

	// set size of data when uncompressed...
	iUncompressedSize = uncompressedSize;

	// override capabilities in image to conform to system wide configuration...
	for(TInt i=0; i<SCapabilitySet::ENCapW; i++)
		{
		iS.iCaps[i] |= DisabledCapabilities[i];
		iS.iCaps[i] &= AllCapabilities[i];
		}

	return KErrNone;
	}


TInt E32Image::Construct(RImageFinder& aFinder)
	{
	__IF_DEBUG(Printf("E32Image::iMain=%08x", iMain));
	__LDRTRACE(aFinder.Dump(">E32Image::Construct", 0));
	__ASSERT_ALWAYS(aFinder.iNewValid, User::Panic(KLitFinderInconsistent, 0));

	// fallback security check to ensure we don't try and load an executable from an insecure location...
	if(PlatSec::ConfigSetting(PlatSec::EPlatSecEnforceSysBin))
		{
		__ASSERT_ALWAYS(aFinder.iNewFileName.Length()>=11, User::Panic(KLitSysBinError, 0));
		__ASSERT_ALWAYS(KSysBin().CompareF(TPtrC8(aFinder.iNewFileName.Ptr()+1,10))==0, User::Panic(KLitSysBinError, 1));
		}

	TInt r = KErrNone;

	// setup file name info...
	iFileName.Copy(aFinder.iNewFileName);
	TFileNameInfo fi;
	fi.Set(iFileName, 0);
	iRootNameOffset = fi.iBasePos;
	iRootNameLength = fi.iLen - fi.iBasePos;
	iExtOffset = fi.iExtPos;

	// setup version...
	iAttr |= aFinder.iNew.iAttr & ECodeSegAttExpVer;
	iModuleVersion = aFinder.iNew.iModuleVersion;

	if(aFinder.iNew.iRomImageHeader)
		{
		// we're 'loading' an XIP executable from ROM...
		Construct(*aFinder.iNew.iRomImageHeader);
		if(!AlwaysLoaded() || iMain==this)
			r = CheckRomXIPAlreadyLoaded();
		return r;
		}

	// setup more image info...
	iAttr |= aFinder.iNew.iAttr & (ECodeSegAttFixed|ECodeSegAttABIMask|ECodeSegAttNmdExpData);
	iUids = *(const TUidType*)&aFinder.iNew.iUid;
	iIsDll = !(iUids[0].iUid == KExecutableImageUidValue);
	iS = aFinder.iNew.iS;

	// check if executable has already been loaded...
	r = CheckAlreadyLoaded();
	if(r!=KErrNone)
		return r;

	// if we are going to need to load it...
	if(!iAlreadyLoaded || !iIsDll)
		{
		if (aFinder.iNew.iNeedHashCheck)
			{
			// we need to check the file hash; the check in RImageFinder::Try
			// was skipped based on the cache. If it fails here, though, someone
			// is tampering with us and we can just fail the load.
			TRAP(r,aFinder.CompareHashL(aFinder.iNew, fi.DriveAndPath()));
			if (r != KErrNone)
				return r;
			}

		if(aFinder.iNew.iFileData)
			{
			// take ownership of the file data aFinder has already read in...
			iFileData = aFinder.iNew.iFileData;
			aFinder.iNew.iFileData = NULL;
			iFileSize = aFinder.iNew.iFileSize;
			}
		else if(aFinder.iNew.FileOpened())
			{
			// take ownership of the file handle that aFinder has already opened...
			iFile = aFinder.iNew.iFile;
			memclr(&aFinder.iNew.iFile, sizeof(RFile));
			}
		else
			{
			// no resource obtained from aFinder, so create a file handle for ourselves...
			r = OpenFile();
			if(r!=KErrNone)
				return r;
			}

		// take ownership of header...
		iHeader = aFinder.iNew.iHeader;
		aFinder.iNew.iHeader = NULL;

		// if there wast't a header, then create one now...
		if(!iHeader)
			{
			if(iFileData)
				r = E32ImageHeader::New(iHeader, iFileData, iFileSize);
			else
				r = E32ImageHeader::New(iHeader, iFile);
			if(r!=KErrNone)
				return r;
			}

		// setup info needed for process creation...
		iHeapSizeMin = iHeader->iHeapSizeMin;
		iHeapSizeMax = iHeader->iHeapSizeMax;
		iStackSize = iHeader->iStackSize;
		iPriority = iHeader->ProcessPriority();
		}

	// if already loaded...
	if(iAlreadyLoaded)
		return KErrNone; // nothing more to do

	// setup info needed to load an executable...
	iDepCount = iHeader->iDllRefTableCount;
	iExportDirCount = iHeader->iExportDirCount;
	iExportDir = iHeader->iExportDirOffset-iHeader->iCodeOffset;
	iTextSize = iHeader->iTextSize;
	iCodeSize = iHeader->iCodeSize;
	__IF_DEBUG(Printf("Code + const %x",iCodeSize));
	iDataSize = iHeader->iDataSize;
	__IF_DEBUG(Printf("Data %x",iDataSize));
	iBssSize = iHeader->iBssSize;
	__IF_DEBUG(Printf("Bss %x",iBssSize));
	iTotalDataSize = iDataSize+iBssSize;

	iFileEntryPoint = iHeader->iEntryPoint;	// just an offset at this stage
	iEntryPtVeneer = 0;
	iExceptionDescriptor = iHeader->ExceptionDescriptor();
	if(iHeader->iExportDirOffset)
		iExportDirLoad = iExportDir;		// only set this if not already loaded

	// initialise the SMP safe flag from the image header
	// this will get cleared during ProcessImports if any import is not SMP safe
	if(iHeader->iFlags & KImageSMPSafe)
		iAttr |= ECodeSegAttSMPSafe;
	else
		{
		__IF_DEBUG(Printf("%S is not marked SMP safe", &iFileName));
		iAttr &= ~ECodeSegAttSMPSafe;
		}

	// check if executable is to be demand paged...
	r = ShouldBeCodePaged(iUseCodePaging);
	__IF_DEBUG(Printf("ShouldBeCodePaged r=%d,iUseCodePaging=%d", r, iUseCodePaging));
	if(iUseCodePaging==EFalse || r!=KErrNone)
		return r;

	// image needs demand paging, create the additional information needed for this...

	// read compression info...
	iCompressionType = iHeader->iCompressionType;
	r = LoadCompressionData();
	if(r==KErrNotSupported)
		{
		// Compression type not supported, so just load executable as normal, (without paging)...
		iUseCodePaging = EFalse;
		return KErrNone;
		}
	else if (r!=KErrNone)
		return r;

	// clamp file so it doesn't get modified whilst it is being demand paged...
	r = iFileClamp.Clamp(iFile);
	// The clamp API will return KErrNotSupported if the media is removable: 
	// this implies that paging is not possible but the binary can still be loaded
	if (r != KErrNone)
		{
		iUseCodePaging = EFalse;
		return r == KErrNotSupported ? KErrNone : r;
		}

	// get blockmap data which indicates location of media where file contents are stored...
	r = BuildCodeBlockMap();
	__IF_DEBUG(Printf("BuildCodeBlockMap r=%d", r));
	if(r==KErrNotSupported)
		{
		// media doesn't support demand paging, so just load executable as normal, (without paging)...
		iUseCodePaging = EFalse;
		iFileClamp.Close(gTheLoaderFs);
		r = KErrNone;
		}

	return r;
	}


TInt E32Image::CheckRomXIPAlreadyLoaded()
	{
	__IF_DEBUG(Printf("ROM XIP %08x CheckAlreadyLoaded",iRomImageHeader));
	TFindCodeSeg find;
	find.iRomImgHdr=iRomImageHeader;
	E32Loader::CodeSegDeferDeletes();
	TAny* h=NULL;
	TInt r=KErrNone;
	E32Loader::CodeSegNext(h, find);
	if (h)
		{
		iHandle=h;
		r=E32Loader::CodeSegOpen(h, iClientProcessHandle);
		if (r==KErrNone)
			E32Loader::CodeSegInfo(iHandle, *this);
		}
	E32Loader::CodeSegEndDeferDeletes();
	if (iHandle && r==KErrNone)
		{
		iAlreadyLoaded=ETrue;
		__IF_DEBUG(Printf("ROM XIP %08x already loaded", iHandle));
		}
	__IF_DEBUG(Printf("ROM XIP CheckAlreadyLoaded returns %d",r));
	return r;
	}


/**
Read the E32Image file into its code and data chunks, relocating them
as necessary.
Create a dll reference table from the names of dlls referenced.
Fix up the import address table and the export table for real addresses.
*/
TInt E32Image::LoadToRam()
	{
	__IF_DEBUG(Printf("E32Image::LoadToRam %S",&iFileName));

	// offset of data after code which will be erad into iRestOfFileData...
	iConversionOffset = iHeader->iCodeOffset + iHeader->iCodeSize;

	// calculate sizes...
	TUint totalSize = ((E32ImageHeaderV*)iHeader)->iUncompressedSize;
	TUint remainder = totalSize-iConversionOffset;
	if(remainder>totalSize)
		RETURN_FAILURE(KErrCorrupt); // Fuzzer can't trigger this because header validation prevents it

	iRestOfFileData = (TUint8*)User::Alloc(remainder);
	if(!iRestOfFileData)
		return KErrNoMemory;
	iRestOfFileSize = remainder;

	TInt r = LoadFile(); // Read everything in
	if(r!=KErrNone)
		return r;

	__IF_DEBUG(Printf("iHeader->iCodeRelocOffset %d",iHeader->iCodeRelocOffset));
	r = ((E32ImageHeaderV*)iHeader)->ValidateRelocations(iRestOfFileData,iRestOfFileSize,iHeader->iCodeRelocOffset,iHeader->iCodeSize,iCodeRelocSection);
	if(r!=KErrNone)
		return r;

	__IF_DEBUG(Printf("iHeader->iDataRelocOffset %d",iHeader->iDataRelocOffset));
	r = ((E32ImageHeaderV*)iHeader)->ValidateRelocations(iRestOfFileData,iRestOfFileSize,iHeader->iDataRelocOffset,iHeader->iDataSize,iDataRelocSection);
	if(r!=KErrNone)
		return r;
		
	iCodeDelta = iCodeRunAddress-iHeader->iCodeBase;
	iDataDelta = iDataRunAddress-iHeader->iDataBase;
	
	if(r==KErrNone)
	   	r = RelocateCode();
	if(r==KErrNone)
		r = LoadAndRelocateData();
	if(r==KErrNone)
    	r = ReadImportData();

	return r;
	}


TInt E32Image::ShouldBeCodePaged(TBool& aPage)
/**
	Determine whether this binary should be paged.  Some of this
	function is unimplemented because it requires the media pageable
	attribute

	@param	aPage			On success, this variable is set to
							whether the binary should be paged.  Its
							value is undefined if the return code is
							not KErrNone.
	@return					Symbian OS error code.

	See S3.1.3.2 of PREQ1110 Design Sketch.
 */
	{
	aPage = EFalse;

	// kernel and global dlls can't be paged...
	if(iAttr&(ECodeSegAttKernel|ECodeSegAttGlobal))
		return KErrNone;

	// 1. if paging policy is NOPAGING then executable is unpaged
	TUint32 policy = E32Loader::PagingPolicy();

	__IF_DEBUG(Printf("sbcp,policy=0x%x", policy));
	if (policy == EKernelConfigCodePagingPolicyNoPaging)
		return KErrNone;

	// 2. if executable is on media without Pageable Media Attribute then unpaged
	// 3. if executable is on removable media then unpaged
	//	both superseded by the BlockMap API

	// 3a. if executable has already been loaded into RAM for tamperproofing then
	//     it can't be paged
	if (iFileData != NULL)
		return KErrNone;

	// 4. if not compressed with bytepair or uncompressed then unpaged
	__IF_DEBUG(Printf("sbcp,iHeader=0x%08x", iHeader));
	TUint32 comp = iHeader->CompressionType();
	__IF_DEBUG(Printf("sbcp,comp=0x%x", comp));
	if (comp != KUidCompressionBytePair && comp != KFormatNotCompressed)
		return KErrNone;

	aPage = ETrue;

	// 5. if policy is ALWAYSPAGE then page
	if (policy == EKernelConfigCodePagingPolicyAlwaysPage)
		return KErrNone;

	// 6. 
	TUint KPagedMask = (KImageCodePaged | KImageCodeUnpaged);
	TUint pagedFlags = iHeader->iFlags & KPagedMask;
	__IF_DEBUG(Printf("sbcp,iHeader->iFlags=0x%x,pagedFlags=0x%x", iHeader->iFlags, pagedFlags));

	// if KImageCodePaged and KImageCodeUnpaged flags present then corrupt
	if (pagedFlags == KPagedMask)
		RETURN_FAILURE(KErrCorrupt);

	// if KImageCodePaged set in executable then page
	if (pagedFlags == KImageCodePaged)
		return KErrNone;

	// if KImageCodeUnpaged set in executable then do not page
	if (pagedFlags == KImageCodeUnpaged)
		{
		aPage = EFalse;
		return KErrNone;
		}

	// 7. otherwise (neither paged nor unpaged set) use paging policy

	// policy must be EKernelConfigCodePagingPolicyDefaultUnpaged or EKernelConfigCodePagingPolicyDefaultPaged
	aPage = (policy == EKernelConfigCodePagingPolicyDefaultPaged);
	return KErrNone;
	}

TInt E32Image::BuildCodeBlockMap()
/**
	Use the block map API to build an array of TBlockMapInfo
	objects which the kernel can use to page in code as required.

	@return					Symbian OS error code.  KErrNotSupported means the
							Block Map functionality does not support paging from
							the binary's location.
 */
	{
	__IF_DEBUG(Printf("BuildCodeBlockMap,iCodeStartInFile=%d,iCodeLengthInFile=%d", iCodeStartInFile, iCodeLengthInFile));

	__ASSERT_DEBUG(iUseCodePaging, Panic(EBcbmNotCodePaged));

	// do nothing if no code section
	if (iCodeLengthInFile == 0)
		return KErrNone;

	// RFile::BlockMap populates an instance of this object.  Need to
	// retain information such as granularity which applies to all entries.
	SBlockMapInfo bmi;

	TInt curEntriesSize = 0;
	TUint8* entries8 = 0;		// points to heap cell containing TBlockMapEntryBase array

	TInt64 bmPos = 0;
	TInt64 bmEnd = iCodeStartInFile + iCodeLengthInFile;
	TInt r;
	do
		{
		__IF_DEBUG(Printf("lfbpu:BlockMap,in,bmPos=%ld,bmEnd=%ld", bmPos, bmEnd));
		r = iFile.BlockMap(bmi, bmPos, bmEnd, EBlockMapUsagePaging);	// updates bmPos to end of mapped range
		__IF_DEBUG(
			Printf("lfbpu:BlockMap,out,r=%d,bmPos=%ld,bmEnd=%ld,maplen=%d(%d)",
			r, bmPos, bmEnd, bmi.iMap.Length(), bmi.iMap.Length() / sizeof(TBlockMapEntryBase)));
		__IF_DEBUG(
			Printf("lfbpu:BlockMap,out,iBlockGranularity=%u,iBlockStartOffset=%u,iStartBlockAddress=%ld,iLocalDriveNumber=%d",
			bmi.iBlockGranularity, bmi.iBlockStartOffset, bmi.iStartBlockAddress, bmi.iLocalDriveNumber));
		if (r != KErrNone && r != KErrCompletion)
			break;

		// Copy info the first time round as this gets overwritten on subsequent passes
		if (curEntriesSize == 0)
			iCodeBlockMapCommon = bmi;	// slices the SBlockMapCommon subclass data
		
		// grow the buffer which contains the entries
		TInt newEntriesSize = bmi.iMap.Length();
		TInt newArraySize = curEntriesSize + newEntriesSize;
		TUint8* newEntries8 = (TUint8*) User::ReAlloc(entries8, newArraySize);
		if (newEntries8 == 0)
			{
			r = KErrNoMemory;
			break;
			}
		entries8 = newEntries8;

#ifdef _DEBUG
		// dump the newly-returned block entries
		for (TInt i = 0; i < newEntriesSize; i += sizeof(TBlockMapEntryBase))
			{
			const TBlockMapEntryBase& bme = *reinterpret_cast<const TBlockMapEntryBase*>(bmi.iMap.Ptr() + i);
			__IF_DEBUG(Printf("lfbpu:bme,iNumberOfBlocks=%d,iStartBlock=%d", bme.iNumberOfBlocks, bme.iStartBlock));
			}
#endif

		// append the new entries to the array.
		Mem::Copy(entries8 + curEntriesSize, bmi.iMap.Ptr(), newEntriesSize);
		curEntriesSize = newArraySize;
		} while (r != KErrCompletion);

	// r == KErrCompletion when mapped code section range
	if (r != KErrCompletion)
		{
		User::Free(entries8);
		return r;
		}
	
#ifdef _DEBUG
	// dump the block map table
	__IF_DEBUG(Printf("lfbpu:endbme,r=%d,curEntriesSize=%d", r, curEntriesSize));
	for (TInt i = 0; i < curEntriesSize; i += 8)
		{
		__IF_DEBUG(Printf(
			"entries[0x%08x], %02x %02x %02x %02x %02x %02x %02x %02x",
			entries8[i+0], entries8[i+1], entries8[i+2], entries8[i+3],
			entries8[i+4], entries8[i+5], entries8[i+6], entries8[i+7]));
		}
#endif

	iCodeBlockMapEntries = reinterpret_cast<TBlockMapEntryBase*>(entries8);
	iCodeBlockMapEntriesSize = curEntriesSize;

	return KErrNone;
	}


/**
Get the compression data relevant to demand paging
*/
TInt E32Image::LoadCompressionData()
	{
	__IF_DEBUG(Printf("E32Image::LoadCompressionData %S 0x%08x",&iFileName,iHeader->CompressionType()));

	TUint compression = iHeader->CompressionType();

	TInt r = KErrNone;
	if(compression==KFormatNotCompressed)
		{
		r = LoadCompressionDataNoCompress();
		}
	else if(compression==KUidCompressionBytePair)
		{
		TRAP(r,LoadCompressionDataBytePairUnpakL());
		}
	else
		{
		r = KErrNotSupported;
		}

	__IF_DEBUG(Printf("E32Image::LoadCompressionData exiting %S r=%d",&iFileName,r));
	return r;	
	}


TInt E32Image::LoadCompressionDataNoCompress()
	{
	__IF_DEBUG(Printf("E32Image::LoadCompressionDataNoCompress %S",&iFileName));
	if (iHeader->iCodeSize)
		{
		iCodeStartInFile = iHeader->iCodeOffset;
		iCodeLengthInFile = iCodeSize;
		}
	return KErrNone;
	}


void E32Image::LoadCompressionDataBytePairUnpakL()
	{
	__IF_DEBUG(Printf("E32Image::LoadCompressionDataBytePairUnpakL %S",&iFileName));

	if (iFileData)
		User::Leave(KErrNotSupported); // if the file data has been loaded into RAM we can't page it!

	TInt pos = iHeader->TotalSize();
	User::LeaveIfError(iFile.Seek(ESeekStart,pos)); // Start at beginning of compressed data

	CBytePairReader* reader = CBytePairFileReader::NewLC(iFile);

	if (iHeader->iCodeSize)
		{
		__IF_DEBUG(Printf("Code & const size %x",iCodeSize));
		__IF_DEBUG(Printf("Code & const offset %x",iHeader->iCodeOffset));
		__IF_DEBUG(Printf("Code & const dest %x",iCodeLoadAddress));
		
		TInt pageCount;
		reader->GetPageOffsetsL(pos, pageCount, iCodePageOffsets);

#ifdef _DEBUG
		for (TInt i = 0; i <= pageCount; ++i)
			{
			__IF_DEBUG(Printf("lfbpu:raw iCodePageOffsets[%d] = %d", i, iCodePageOffsets[i]));
			}
#endif

		// record the code start position in the file and its compressed length
		// so BuildCodeBlockMap can construct a block map for the kernel if this
		// file is demand paged.
		iCodeStartInFile = iCodePageOffsets[0];
		iCodeLengthInFile = iCodePageOffsets[pageCount] - iCodePageOffsets[0];
		}
		
	CleanupStack::PopAndDestroy(reader);
	}


/**
Read all image data into memory, decompressing it using the method indicated in the image header..
If code isn't being demand paged the code part is read into #iCodeLoadAddress.
The rest of the file data after the code part is read into #iRestOfFileData.
*/
TInt E32Image::LoadFile()
	{
	__IF_DEBUG(Printf("E32Image::LoadFile %S 0x%08x",&iFileName,iHeader->CompressionType()));

	TUint compression = iHeader->CompressionType();

	TInt r=KErrNone;
	if(compression==KFormatNotCompressed)
		{
		r = LoadFileNoCompress();
		CHECK_FAILURE(r); // Fuzzer can't trigger this because it only happens on file i/o error
		}
	else if(compression==KUidCompressionDeflate)
		{
		TRAP(r,LoadFileInflateL());
		CHECK_FAILURE(r);
		}
	else if(compression==KUidCompressionBytePair)
		{
		TRAP(r,LoadFileBytePairUnpakL());
		CHECK_FAILURE(r);
		}
	else
		{
		r = KErrNotSupported;
		CHECK_FAILURE(r); // Fuzzer can't trigger this because header validation ensures compression type is OK
		}

	// we're done with the file contents now, free up memory before resolving imports
	if(iFileData)
		{
		gFileDataAllocator.Free(iFileData);
		iFileData=NULL;
		}

	__IF_DEBUG(Printf("E32Image::LoadFile exiting %S r=%d",&iFileName,r));
	return r;
	}


/**
Read data from the image's file (or the preloaded data at #iFileData if present).
*/
TInt E32Image::Read(TUint aPos, TUint8* aDest, TUint aSize, TBool aSvPerms)
	{
	TPtr8 p(aDest,aSize,aSize);
	if(iFileData)
		{
		// get data from pre-loaded image data...
		if(aPos+aSize>iFileSize)
			RETURN_FAILURE(KErrCorrupt); // Fuzzer can't trigger this because earlier validation prevents sizes being wrong
		if (aSvPerms)
			WordCopy(aDest,iFileData+aPos,aSize);
		else
			p.Copy(iFileData+aPos,aSize);
		}
	else
		{
		// get data from file...
		TInt r = iFile.Read(aPos,p,aSize);
		if(r!=KErrNone)
			return r;
		}

	// check we got the amount of data requested...
	if(TUint(p.Length())!=aSize)
		{
		__IF_DEBUG(Printf("E32Image::Read() Expected:%d, read:%d", aSize, p.Length() ));
		RETURN_FAILURE(KErrCorrupt); // Fuzzer can't trigger this because requires file length to change during load
		}

	return KErrNone;
	}


/**
Read all image data into memory.
If code isn't being demand paged the code part is read into #iCodeLoadAddress.
The rest of the file data after the code part is read into #iRestOfFileData.
*/
TInt E32Image::LoadFileNoCompress()
	{
	__IF_DEBUG(Printf("E32Image::LoadFileNoCompress exiting %S",&iFileName));
	TInt r = KErrNone;
	
	if(iHeader->iCodeSize && !iUseCodePaging)
		{
		__IF_DEBUG(Printf("Code & const size %x",iCodeSize));
		__IF_DEBUG(Printf("Code & const offset %x",iHeader->iCodeOffset));
		__IF_DEBUG(Printf("Code & const dest %x",iCodeLoadAddress));
		r = Read(iHeader->iCodeOffset, (TText8*)iCodeLoadAddress, iCodeSize, ETrue);
		if(r!=KErrNone)
			return r;
		}

	if(iRestOfFileSize)
		r = Read(iConversionOffset, iRestOfFileData, iRestOfFileSize);
	
	return r;
	}


void FileCleanup(TAny* aPtr)
	{
	TFileInput* f=(TFileInput*)aPtr;
	f->Cancel();
	delete f;
	}

/**
Read all image data into memory, decompressing it using the Inflate method.
If code isn't being demand paged the code part is read into #iCodeLoadAddress.
The rest of the file data after the code part is read into #iRestOfFileData.
*/
void E32Image::LoadFileInflateL()
	{
	__IF_DEBUG(Printf("E32Image::LoadFileInflateL %S",&iFileName));
	__ASSERT_DEBUG(!iUseCodePaging, Panic(ELfiCodePagingNotSupported));
	
	TInt pos = iHeader->TotalSize();
	TBitInput* file;
	if(iFileData)
		{
		if(pos < 0)
			User::Leave(KErrArgument);
		file = new (ELeave) TBitInput(iFileData, iFileSize*8, pos*8);
		CleanupStack::PushL(file);
		}
	else
		{
		User::LeaveIfError(iFile.Seek(ESeekStart,pos)); // Start at beginning of compressed data
		file = new (ELeave) TFileInput(iFile);
		CleanupStack::PushL(TCleanupItem(&FileCleanup,file));
		}

	CInflater* inflater=CInflater::NewLC(*file);
	
	if(iHeader->iCodeSize)
		{
		__IF_DEBUG(Printf("Code & const size %x",iCodeSize));
		__IF_DEBUG(Printf("Code & const offset %x",iHeader->iCodeOffset));
		__IF_DEBUG(Printf("Code & const dest %x",iCodeLoadAddress));

		TInt count = inflater->ReadL((TUint8*)iCodeLoadAddress,iCodeSize,&WordCopy);
		if(count!=iCodeSize)
			User::Leave(KErrCorrupt);
		}

	if(iRestOfFileSize)
		{
		TUint32 count = inflater->ReadL(iRestOfFileData,iRestOfFileSize,&Mem::Copy);
		if(count!=iRestOfFileSize)
			User::Leave(KErrCorrupt);
		}

	CleanupStack::PopAndDestroy(2,file);
	}
	

/**
Read all image data into memory, decompressing it using the BytePair method.
If code isn't being demand paged the code part is read into #iCodeLoadAddress.
The rest of the file data after the code part is read into #iRestOfFileData.
*/
void E32Image::LoadFileBytePairUnpakL()
	{
	__IF_DEBUG(Printf("E32Image::LoadFileBytePairUnpak %S",&iFileName));

	// code starts after header
	TInt pos = iHeader->TotalSize();

	CBytePairReader* reader;
	if(iFileData)
		reader = CBytePairReader::NewLC(iFileData+pos, iFileSize-pos);
	else
		{
		iFile.Seek(ESeekStart, pos);
		reader = CBytePairFileReader::NewLC(iFile);
		}

	TBool codeLoaded = false;
	if(iHeader->iCodeSize && !iUseCodePaging)
		{
		__IF_DEBUG(Printf("Code & const size %x",iCodeSize));
		__IF_DEBUG(Printf("Code & const offset %x",iHeader->iCodeOffset));
		__IF_DEBUG(Printf("Code & const dest %x",iCodeLoadAddress));

		TUint32 bytes = reader->DecompressPagesL((TUint8*)iCodeLoadAddress,iCodeSize,&WordCopy);

		__IF_DEBUG(Printf("bytes:%x",bytes));
		if((TInt)bytes!=iCodeSize)
			User::Leave(KErrCorrupt);

		codeLoaded = true;
		}
	
	if(iRestOfFileSize)
		{
		if(!codeLoaded)
			{
			// skip past code part of file...
			TInt pageCount = (iCodeSize + KPageOffsetMask) >> KPageSizeShift;
		
			TInt pos = 	KIndexTableHeaderSize
					+	pageCount * sizeof(TUint16)
					+   iCodeLengthInFile;
	 
			__IF_DEBUG(Printf("lfpbu:pos=%x", pos));
			reader->SeekForwardL(pos);
			}
		
		__IF_DEBUG(Printf("  iRestOfFileSize==%x, iRestOfFileData==%x", iRestOfFileSize, iRestOfFileData));
		
		TUint32 bytes = reader->DecompressPagesL(iRestOfFileData,iRestOfFileSize,NULL);
		__IF_DEBUG(Printf("bytes:%x",bytes));
		if(bytes!=iRestOfFileSize)
			User::Leave(KErrCorrupt);
		}
		
	CleanupStack::PopAndDestroy(reader);
	}


/**
Relocate code.
*/
TInt E32Image::RelocateCode()
	{
	if(iHeader->iExportDirOffset)
		iExportDirLoad += iCodeLoadAddress;	// only for RAM modules which are not already loaded

	__IF_DEBUG(Printf("**EntryPointVeneer %08x FileEntryPoint %08x",iEntryPtVeneer,iFileEntryPoint));
	__IF_DEBUG(Printf("**ExportDir load@%08x run@%08x",iExportDirLoad,iExportDir));
	TInt r = KErrNone;	
	if(iHeader->iCodeRelocOffset)
		{
		__IF_DEBUG(Printf("Relocate code & const"));

		if(!iUseCodePaging)
			r = RelocateSection(iCodeRelocSection, iCodeLoadAddress);
		else
			{
			r = AllocateRelocationData(iCodeRelocSection, iHeader->iCodeSize, iCodeLoadAddress, iCodeRelocTable);
			iExportDirEntryDelta = iCodeDelta; // so exports get relocated
			}
		}

	if(r==KErrNone)
		r = RelocateExports();

	if(r==KErrNone)
		{
		// put a unique ID into the third word after the entry point

		// address for ID...
		TLinAddr csid_addr = iFileEntryPoint+KCodeSegIdOffset-iCodeRunAddress+iCodeLoadAddress;
		__IF_DEBUG(Printf("csid_addr %08x", csid_addr));

		// get existing ID...
		TUint x;
		WordCopy(&x, (const TAny*)csid_addr, sizeof(x));
		if(x==0)
			{
			// generate next ID...
			if(++NextCodeSegId == 0xffffffffu)
				Fault(ELdrCsIdWrap);
			__IF_DEBUG(Printf("NextCSID %08x", NextCodeSegId));
			// store ID...
			if(!iUseCodePaging)
				WordCopy((TAny*)csid_addr, &NextCodeSegId, sizeof(NextCodeSegId));
			else
				{
				// demand paged code needs modifying when paged in, so add ID as a new 'fixup'...
				TUint64* fixup = ExpandFixups(1);
				if(!fixup)
					r = KErrNoMemory;
				else
					*fixup = MAKE_TUINT64(csid_addr,NextCodeSegId);
				}
			}
		}

	return r;
	}


/**
Copy the data section from buffer #iRestOfFileData to the memory allocated at #iDataLoadAddress.
Then relocate this data ready for use at the executables run addresses.
*/
TInt E32Image::LoadAndRelocateData()
	{
	__IF_DEBUG(Printf("E32Image::LoadAndRelocateData %S",&iFileName));
	if(!iHeader->iDataOffset)
		return KErrNone; // do data section

	// copy data...
	__IF_DEBUG(Printf("Read Data: size %x->%08x",iDataSize,iDataLoadAddress));
	TUint32 bufferOffset=iHeader->iDataOffset-iConversionOffset;
	TUint8* source=iRestOfFileData+bufferOffset;
	MemCopy((TText8*)iDataLoadAddress,source,iDataSize);

	// relocate data...
	__IF_DEBUG(Printf("Relocate data section"));
	__IF_DEBUG(Printf("iDataRelocOffset %08x",iHeader->iDataRelocOffset));
	TInt r = KErrNone;	
	if(iHeader->iDataRelocOffset)
		r = RelocateSection(iDataRelocSection, iDataLoadAddress);

	return r;
	}


/**
Copies data from aDestination to aSource by running in supervisor mode.
aDest, aSource & aNumberOfBytes must be word aligned.
*/
TUint8* E32Image::WordCopy(TAny* aDestination, const TAny* aSource, TInt aNumberOfBytes)
	{
	aNumberOfBytes &= ~3; // Avoid panics for corrupt data which is not word size
	SCopyDataInfo info = {aDestination,aSource, aNumberOfBytes};
	return (TUint8*) ExecuteInSupervisorMode(&svWordCopy, &info);
	}


/**
Copies data from aDestination to aSource by running in supervisor mode.
*/
TUint8* E32Image::MemCopy(TAny* aDestination, const TAny* aSource, TInt aNumberOfBytes)
	{
	SCopyDataInfo info={aDestination,aSource, aNumberOfBytes};
	return (TUint8*) ExecuteInSupervisorMode(&svMemCopy, &info);
	}


/**
Relocate a section, applying relocations for run addresses to values currently at their load addresses.
*/
TInt E32Image::RelocateSection(E32RelocSection* aSection, TUint32 aLoadAddress)
	{
	if(!aSection)
		return KErrNone;

	__IF_DEBUG(Printf("Relocate: NRelocs:%08x LoadAddr:%08x", aSection->iNumberOfRelocs, aLoadAddress));

	SRelocateSectionInfo info={this, (TUint8*)(aSection+1), aSection->iNumberOfRelocs, aLoadAddress};

	// call function in supervisor mode to relocate the section
	TInt r = ExecuteInSupervisorMode(&svRelocateSection, &info);

	__IF_DEBUG(Printf("Relocate returning %d",r));
	return r;
	}


/**
Relocate the export directory for the code's run address
*/
TInt E32Image::RelocateExports()
	{
	// This only has to be done for PE-derived images, ELF marks all
	// export table entries as 'relocations' so this job has already been done.
	TUint impfmt = iHeader->ImportFormat();
	if (impfmt == KImageImpFmt_ELF)
		return KErrNone;

	__IF_DEBUG(Printf("E32Image::RelocateExports %S",&iFileName));

	if(iHeader->iExportDirOffset)
		{
		// call function in supervisor mode to fix up export directory
		ExecuteInSupervisorMode(&svRelocateExports, this);
		}
	return KErrNone;
	}


/**
Validate import section data structures in iRestOfFileData.
Set iImportData to point to point to start of this.
Allocate memory (iCurrentImportList) which is big enough to store imports for a single dependency.
*/
TInt E32Image::ReadImportData()
	{
	__IF_DEBUG(Printf("E32Image::ReadImportData %S",&iFileName));

	if(!iHeader->iImportOffset)
		return KErrNone;

	TUint biggestImportCount; 
	TInt r = ((E32ImageHeaderV*)iHeader)->ValidateImports(iRestOfFileData,iRestOfFileSize,biggestImportCount);
	if(r!=KErrNone)
		return r;

	iImportData = (TUint32*)(iRestOfFileData+iHeader->iImportOffset-iConversionOffset);
	iCurrentImportList = (TUint32*)User::Alloc(biggestImportCount * sizeof(TUint32));
	__IF_DEBUG(Printf("E32Image::ReadImportData - alloc %d current import slots at %08x", biggestImportCount, iCurrentImportList));
	if(!iCurrentImportList)
		return KErrNoMemory;

	return KErrNone;
	}


void E32Image::SortCurrentImportList()
	{
	if (!iCurrentImportListSorted)
		{
		RArray<TUint> array((TUint*)iCurrentImportList, iCurrentImportCount);
		array.Sort();
		iCurrentImportListSorted = (TUint8)ETrue;
		}
	}


TInt CheckRomExports(const TRomImageHeader* aR, const E32Image* aI)
	{
	__IF_DEBUG(Printf("CheckRomExports"));
	if (aR->iExportDirCount == 0)
		return aI->iCurrentImportCount ? KErrNotSupported : KErrNone;
	const TUint32* xd = (const TUint32*)aR->iExportDir;
	const TUint32* p = aI->iCurrentImportList;
	const TUint32* pE = p + aI->iCurrentImportCount;
	for (; p<pE; ++p)
		if (xd[*p] == 0)
			return KErrNotSupported;
	return KErrNone;
	}


TInt CheckRamExports(TUint aEDT, const TUint8* aED, TUint aEDC, E32Image* aI)
	{
	__IF_DEBUG(Printf("CheckRamExports"));
	if (aEDC == 0)
		return aI->iCurrentImportCount ? KErrNotSupported : KErrNone;
	if (aEDT == KImageHdr_ExpD_NoHoles)
		return KErrNone;	// nothing missing

	const TUint32* p = aI->iCurrentImportList;
	const TUint32* pE = p + aI->iCurrentImportCount;

	if (aEDT == KImageHdr_ExpD_FullBitmap)
		{
		for (; p<pE; ++p)
			{
			TUint32 x = *p - 1;
			if ( !(aED[x>>3] & (1u<<(x&7))) )
				return KErrNotSupported;
			}
		return KErrNone;
		}

	if (aEDT != KImageHdr_ExpD_SparseBitmap8)
		return KErrNotSupported;		// don't know what this is
	aI->SortCurrentImportList();		// sort imports to increasing order
	TUint32 memsz = (aEDC + 7) >> 3;	// size of complete bitmap
	TUint32 mbs = (memsz + 7) >> 3;		// size of meta-bitmap
	const TUint8* mptr = aED;
	const TUint8* gptr = mptr + mbs;
	const TUint8* mptrE = mptr + mbs;
	TUint xlim = 64;
	for (; mptr<mptrE && p<pE; ++mptr, xlim+=64)
		{
		TUint m = *mptr;
		if (m==0)
			{
			// nothing missing in this block of 64 exports; step to next block
			for (; p<pE && *p<=xlim; ++p) {}
			continue;
			}
		// expand this block of 64
		TUint32 g32[2] = {0xffffffffu, 0xffffffffu};
		TUint8* g = (TUint8*)g32;
		for (; m; m>>=1, ++g)
			if (m&1)
				*g = *gptr++;
		g = (TUint8*)g32;
		for (; p<pE && *p<=xlim; ++p)
			{
			TUint ix = *p - (xlim - 64) - 1;
			if ( !(g[ix>>3] & (1u<<(ix&7))) )
				return KErrNotSupported;
			}
		}
	return KErrNone;
	}


TInt CheckRequiredImports(E32Image* aImporter, E32Image* aExporter, TInt aAction)
	{
	__IF_DEBUG(Printf("E32Image::CheckRequiredImports (existing) %d", aAction));
	TInt last = aImporter->LastCurrentImport();
	if (last > aExporter->iExportDirCount)
		return KErrNotSupported;
	if (aAction == EAction_CheckLastImport)
		return KErrNone;
	if (aExporter->iRomImageHeader)
		return CheckRomExports(aExporter->iRomImageHeader, aImporter);
	if (aExporter->iHeader)
		{
		E32ImageHeaderV* v = (E32ImageHeaderV*)aExporter->iHeader;
		return CheckRamExports(v->iExportDescType, v->iExportDesc, v->iExportDirCount, aImporter);
		}
	TInt r = aExporter->ReadExportDirLoad();
	if (r != KErrNone)
		return r;				// could fail with OOM
	TBool hasNmdExp = (aExporter->iAttr & ECodeSegAttNmdExpData);
	const TUint32* p = aImporter->iCurrentImportList;
	const TUint32* pE = p + aImporter->iCurrentImportCount;
	const TUint32* pX = (const TUint32*)aExporter->iExportDirLoad - 1;
	TUint32 xep = aExporter->iFileEntryPoint;
	for (; p<pE; ++p)
		{
		TUint32 x = *p;
		TUint32 xx = pX[x];
		if ((xx==0 && (x!=0 || (x==0&&hasNmdExp))) || xx==xep)
			return KErrNotSupported;
		}
	return KErrNone;
	}


TInt CheckRequiredImports(E32Image* aImporter, const RImageInfo& aExporter, TInt aAction)
	{
	__IF_DEBUG(Printf("E32Image::CheckRequiredImports (new) %d", aAction));
	TInt last = aImporter->LastCurrentImport();
	if (last > aExporter.iExportDirCount)
		return KErrNotSupported;
	if (aAction == EAction_CheckLastImport)
		return KErrNone;
	if (aExporter.iRomImageHeader)
		return CheckRomExports(aExporter.iRomImageHeader, aImporter);
	return CheckRamExports(aExporter.iExportDescType, aExporter.iExportDesc, aExporter.iExportDirCount, aImporter);
	}


TInt E32Image::GetCurrentImportList(const E32ImportBlock* a)
	{
	__IF_DEBUG(Printf("E32Image::GetCurrentImportList(E32ImportBlock* a:%08X)", a));
	TInt r;
	TInt n = a->iNumberOfImports;
	iCurrentImportCount = n;
	iCurrentImportListSorted = (TUint8)EFalse;
	__IF_DEBUG(Printf("iCurrentImportCount:%d, iCurrentImportListSorted:%d)", iCurrentImportCount, iCurrentImportListSorted));
	__IF_DEBUG(Printf("iHeader->ImportFormat() == KImageImpFmt_ELF:%d", (iHeader->ImportFormat() == KImageImpFmt_ELF) ));
	
	if (iHeader->ImportFormat() == KImageImpFmt_ELF)
		{
		SGetImportDataInfo info;
		info.iCount = n;
		info.iDest = iCurrentImportList;
		info.iCodeLoadAddress = iCodeLoadAddress;
		info.iImportOffsetList = (TUint32*)a->Imports();
		r = ExecuteInSupervisorMode(&svElfDerivedGetImportInfo, &info);
		}
	else
		{
		TUint32* iat = (TUint32*)(iCodeLoadAddress + iTextSize);
		WordCopy(iCurrentImportList, iat + iNextImportPos, n * sizeof(TUint32));
		r = KErrNone;
		}
	iNextImportPos += n;
	__IF_DEBUG(Printf("End of E32Image::GetCurrentImportList:%d)", r));
	return r;
	}


TInt E32Image::LastCurrentImport()
	{
	TUint32 last = 0;
	if (iCurrentImportListSorted)
		last = iCurrentImportList[iCurrentImportCount - 1];
	else
		{
		const TUint32* p = iCurrentImportList;
		const TUint32* pE = p + iCurrentImportCount;
		for (; p<pE; ++p)
			if (*p > last) last = *p;
		}
	__IF_DEBUG(Printf("E32Image::LastCurrentImport = %d", last));
	return last;
	}


TInt E32Image::ProcessImports()
//
//	This function is only ever called on the exe/dll which is loaded from 
//	the RProcess/RLibrary load.
//	It reads this DLL/EXE's imports section and builds up a table of dlls referenced.
//	It never goes recursive.
//
	{
	__IF_DEBUG(Printf("E32Image::ProcessImports %S",&iFileName));
	__IF_DEBUG(Printf("DepCount=%d",iDepCount));
	
	if (iDepCount==0 || AlwaysLoaded())
		return KErrNone;	// no imports

	TFileNameInfo fi;
	fi.Set(iFileName, 0);
	gLoadeePath.Zero();
	fi.GetName(gLoadeePath, TFileNameInfo::EIncludeDrivePath);
	if (PlatSec::ConfigSetting(PlatSec::EPlatSecEnforceSysBin)
			&& gLoadeePath.Length()==11
			&& KSysBin().CompareF(TPtrC8(gLoadeePath.Ptr()+1,10))==0)
		{
		// Main loadee is in the default path, so unset this in order to
		// search normally for dependents
		gLoadeePath.Zero();
		}
#ifdef __X86__
	if (gLoadeePath.Length()>=2 && gLoadeePath[1]==':')
		{
		TInt d = gLoadeePath[0];
		if (d=='a' || d=='A')
			UseFloppy = EDriveA;
		else if (d=='b' || d=='B')
			UseFloppy = EDriveB;
		}
#endif
	RImageArray array;
	TInt r = array.Add(this);
	if (r==KErrNone)
		r = LoadDlls(array);
	if (r==KErrNone)
		r = FixupDlls(array);
	if (r==KErrNone)
		r = FinaliseDlls(array);
	CleanupDlls(array);
	array.Close();

	__IF_DEBUG(Printf("E32Image::ProcessImports returns %d",r));
	return r;
	}

void E32Image::CleanupDlls(RImageArray& aArray)
//
// Free the space used in fixing up the dlls.
// Don't free the entry corresponding to the main loadee.
//
	{

	__IF_DEBUG(Printf("CleanupDlls"));
	TInt n = aArray.Count();
	TInt i;
	for (i=0; i<n; ++i)
		{
		E32Image* e = aArray[i];
		if (e != this)
			delete e;
		}
	}

TInt E32Image::FinaliseDlls(RImageArray& aArray)
	{
	__IF_DEBUG(Printf("E32Image::FinaliseDlls"));
	TInt i;
	TInt c = aArray.Count();
	TInt r = KErrNone;
	for(i=0; i<c && r==KErrNone; i++)
		{
		E32Image* e = aArray[i];
		if(e!=this && !e->iAlreadyLoaded)
			{
			// transfers ownership of clamp handle to codeseg; nulls handle if successful
			if(!e->AlwaysLoaded())
				r = E32Loader::CodeSegLoaded(*e);
			if(r==KErrNone && e->iUseCodePaging)
				{
				e->iFileClamp.iCookie[0]=0;// null handle to indicate 
				e->iFileClamp.iCookie[1]=0;// transfer of ownership of clamp handle to codeseg
				}
			}
		}
	__IF_DEBUG(Printf("E32Image::FinaliseDlls returns %d",r));
	return r;
	}


TInt E32Image::LoadDlls(RImageArray& aArray)
//
// Build a matrix of all DLLs referenced by the one we're loading, and
// ensure they're all loaded.
//
	{
	__IF_DEBUG(Printf("E32Image::LoadDlls"));
	TInt r=KErrNone;
	E32ImportSection* importSection=(E32ImportSection *)iImportData;
	E32ImportBlock* block;
	if(importSection)
		block=(E32ImportBlock*)(importSection+1);
	else
		block=NULL;
	const TRomImageHeader* const * pR=NULL;
	if (iRomImageHeader)
		pR=iRomImageHeader->iDllRefTable->iEntry;
	iNextImportPos = 0;

	// For each module referenced by this module
	for (TInt i=0; i<iDepCount; ++i)
		{
		RImageFinder finder;
		E32ImportBlock* thisBlock = block;
		E32Image* e = NULL;	// will represent referenced module
		const TRomImageHeader* rih = NULL;
		RLdrReq req;		// new loader request to load referenced module
		TBuf8<KMaxKernelName> rootname;
		req.iFileName = (HBufC8*)&rootname;

		if (pR)
			{
			// Processing imports for ROM XIP module
			rih = *pR++;
			__IF_DEBUG(Printf("Importing from ROM XIP %08x", rih));
			e = aArray.Find(rih);
			}
		else
			{
			// Processing imports for RAM module
			__IF_DEBUG(Printf("Import block address %08x",block));
			TPtrC8 dllname = (const TText8*)((TUint32)iImportData + block->iOffsetOfDllName);
			if (dllname.Length() > KMaxKernelName)
				{
				__IF_DEBUG(Printf("Import DLL name too big: %S",&dllname));
				RETURN_FAILURE(KErrNotSupported);
				}
			TFileNameInfo fni;
			r = fni.Set(dllname, TFileNameInfo::EAllowUid);
			if (r!=KErrNone)
				RETURN_FAILURE(KErrCorrupt);
			fni.GetName(rootname, TFileNameInfo::EIncludeBaseExt);
			TUint32* uid=(TUint32*)&req.iRequestedUids;
			uid[2] = fni.Uid();
			req.iRequestedVersion = fni.Version();
			if (gLoadeePath.Length() > 0)
				req.iPath = (HBufC8*)&gLoadeePath;
			req.iPlatSecCaps = iS.iCaps;
			req.iFileNameInfo.Set(rootname, 0);
			req.iImporter = this;
			r = GetCurrentImportList(block);	// get list of required exports from this exporter
			if (r!=KErrNone)
				{
				return r;
				}
			TUint impfmt = iHeader->ImportFormat();
			block = (E32ImportBlock*)block->NextBlock(impfmt);

			r = finder.Set(req);
			if (r == KErrNone)
				r = finder.SearchExisting(aArray);	// see what we've already got
			if (r == KErrNone)
				{
				TBool search = ETrue;
				if (finder.iExisting)
					{
					// Found an existing DLL - check for an exact version match
					if (DetailedCompareVersions(finder.iCurrentVersion, finder.iReq->iRequestedVersion) <= EVersion_Exact)
						search = EFalse;		// if exact match, don't need to continue search
					}
				if (search)
					r = finder.Search();		// see what else is available
				}
			if (r!=KErrNone)
				{
				finder.Close();
				return r;
				}
			if (finder.iExisting)
				e = finder.iExisting;			// already have the required module
			}

		// If it's already in the array, go on to the next module
		if (e)
		    {
			__IF_DEBUG(Printf("Already there"));
			}
		else
			{
			//	Not already in the array
			__IF_DEBUG(Printf("Not in array, add it"));
			e = new E32Image;
			if (!e)
				{
				finder.Close();
				return KErrNoMemory;
				}
			e->iMain = iMain;
			e->iClientProcessHandle = iMain->iClientProcessHandle;
			if (iMain->iAttr & ECodeSegAttKernel)
				e->iAttr |= ECodeSegAttKernel;
			if (rih)
				{
				// loading a specified ROM XIP DLL
				r = e->DoLoadCodeSeg(*rih);
				}
			else
				{
				// loading a DLL by name
				r = e->DoLoadCodeSeg(req, finder); // also closes 'finder'
				__IF_DEBUG(Printf("%S DoLoadCodeSeg returned %d",req.iFileName,r));
				}

			//	Add the new entry to the array
			if (r==KErrNone)
				{
				__IF_DEBUG(Printf("Add to the array"));
				r = aArray.Add(e);
				}
			if (r!=KErrNone)
				{
				delete e;
				return r;
				}
						
			//	Now go nice and recursive, and call LoadDlls on this latest dll, if it 
			//	imports anything
			//	This recursive horror *will* terminate because it is only called
			//	on "new" dlls
			if (e->iDepCount && !e->iAlreadyLoaded && e->iIsDll)
				{
				__IF_DEBUG(Printf("****Going recursive****"));
				r = e->LoadDlls(aArray);
				__IF_DEBUG(Printf("****Returned from recursion****"));
				if (r!=KErrNone)
					{
					return r;
					}
				}

			}

		// If we added an SMP unsafe dependent, this image is SMP unsafe.
		// This is done after recursing into LoadDlls, so a single unsafe
		// dependent anywhere down the tree will poison everything above it.
		// This isn't sufficient to deal with cycles, though, so the kernel
		// also has to update the flag in DCodeSeg::FinaliseRecursiveFlags.
		// It has to be done here first because the kernel doesn't know
		// about XIP DLLs that don't have a codeseg created.
		if (!(e->iAttr & ECodeSegAttSMPSafe))
			{
			__IF_DEBUG(Printf("%S is not SMP safe because it loads %S", &iFileName, &e->iFileName));
			iAttr &= ~ECodeSegAttSMPSafe;
			}

		// If exporter is an EXE it must be the same as the client process or newly created process
		__IF_DEBUG(Printf("Check EXE->EXE"));
		if (gExeCodeSeg && !e->iIsDll && e->iHandle!=gExeCodeSeg)
			return KErrNotSupported;

		// A globally-visible module may only link to other globally visible modules
		__IF_DEBUG(Printf("Check Global Attribute"));
		if ( (iAttr&ECodeSegAttGlobal) && !(e->iAttr&ECodeSegAttGlobal) )
			return KErrNotSupported;

		// A ram-loaded globally-visible module may only link to ROM XIP modules with no static data
		__IF_DEBUG(Printf("Check RAM Global"));
		if ( (iAttr&ECodeSegAttGlobal) && !iRomImageHeader && e->iHandle)
			return KErrNotSupported;

		if (thisBlock)
			thisBlock->iOffsetOfDllName=(TUint32)e;   // For easy access when fixing up imports
		if (e->iHandle)
			{
			//	Record the dependence of this on e
			r=E32Loader::CodeSegAddDependency(iHandle, e->iHandle);
			if (r!=KErrNone)
				{
				return r;
				}
			}
		}
	__IF_DEBUG(Printf("E32Image::LoadDlls OK"));
	return KErrNone;
	}


TInt E32Image::ReadExportDirLoad()
	{
	//	Get the exporter's export directory
	__IF_DEBUG(Printf("ReadExportDirLoad exp_dir=%08x", iExportDirLoad));
	if (!iExportDirLoad)
		{
		// already loaded nonglobal DLL - must read the export directory
		if (iExportDirCount==0 && !(iAttr&ECodeSegAttNmdExpData))
			return KErrGeneral; // DLL has no exports, something must be wrong
		iCopyOfExportDir = (TUint32*)User::Alloc((iExportDirCount+1) * sizeof(TUint32));
		if (!iCopyOfExportDir)
			return KErrNoMemory;
		__IF_DEBUG(Printf("Reading %d exports", iExportDirCount));
		E32Loader::ReadExportDir(iHandle, iCopyOfExportDir);
		iExportDirLoad = (TUint32)(iCopyOfExportDir+1);
		}
	return KErrNone;
	}


TInt E32Image::FixupDlls(RImageArray& aArray)
//
// Go through the array, fixing up the files
//
	{
	__IF_DEBUG(Printf("E32Image::FixupDlls"));

	// For each E32Image file in the array
	TInt i;
	TInt c = aArray.Count();

	for (i=0; i<c; ++i)
		{
		TInt r;

		E32Image* imp = aArray[i];
		__IF_DEBUG(Printf("Dll number %d %S",i,&imp->iFileName));

		const E32ImportSection* importSection = (const E32ImportSection*)imp->iImportData;
		if (!importSection)
			{
			__IF_DEBUG(Printf("Has no imports to fixup"));
			continue;	//	No imports, skip this dll (true of ALL ROM dlls)
			}

		const E32ImportBlock* block = (const E32ImportBlock*)(importSection + 1);

		SFixupImportAddressesInfo info;
		info.iIat = (TUint32*)(imp->iCodeLoadAddress + imp->iTextSize);
		info.iCodeLoadAddress = imp->iCodeLoadAddress;

		// fix up imports from each dependent DLL, building a table of all the imports for the binary
		TInt depCount = imp->iDepCount;
		while (depCount--)
			{
			// declare variables at start of loop body to prevent 'crosses initialization' errors
			TUint impfmt;

			// E32Image::LoadDlls() will have set iOffsetOfDllName of the 
			// import block to point to the E32Image object of the exporter
			// it's importing
			E32Image* exp = (E32Image*)(block->iOffsetOfDllName);   // LoadDlls() set this to exporter

			//	Get the exporter's export directory
			r = exp->ReadExportDirLoad();
			if (r != KErrNone)
				return r;
			info.iNumImports = block->iNumberOfImports;
			info.iExporter = exp;

			// if demand paging, expand the import fixup buffer for this next exporting DLL
			if (! imp->iUseCodePaging)
				info.iFixup64 = 0;
			else
				{
				info.iFixup64 = imp->ExpandFixups(block->iNumberOfImports);
				if (!info.iFixup64)
					return KErrNoMemory;
				}

			// call function in supervisor mode to fix up the import addresses.
			impfmt = imp->iHeader->ImportFormat();
			if (impfmt == KImageImpFmt_ELF)
				{
				info.iImportOffsetList = (TUint32*)(block+1);
				__IF_DEBUG(Printf("Import format ELF (%08x); info@%08x", impfmt, &info));
				r = ExecuteInSupervisorMode(&svElfDerivedFixupImportAddresses, &info);
				}
			else
				{
				__IF_DEBUG(Printf("Import format PE (%08x); info@%08x", impfmt, &info));
				r = ExecuteInSupervisorMode(&svFixupImportAddresses, &info);
				}

			if (r != KErrNone)
				{
				__IF_DEBUG(Printf("svFixupImportAddresses returns %d", r));
				return r;
				}

			// Next import block...
			block = block->NextBlock(impfmt);
			}	// while (depCount--)

		if (imp->iUseCodePaging && imp->iFixupCount > 0)
			{
			// convert the <addr,val> pairs to an import fixup tab which can be used when
			// the code is paged.
			r = imp->BuildImportFixupTable();
			if (r != KErrNone)
				return r;
			}
		}

	__IF_DEBUG(Printf("E32Image::FixupDlls OK"));
	return KErrNone;
	}


TUint64* E32Image::ExpandFixups(TInt aNumFixups)
	{
	__IF_DEBUG(Printf("ExpandFixups,%d+%d", iFixupCount,aNumFixups));
	TInt newCount = iFixupCount+aNumFixups;
	TUint64* fixups = (TUint64*) User::ReAlloc(iFixups, sizeof(TUint64) * newCount);
	if(!fixups)
		return 0;
	TUint64* newFixups = fixups+iFixupCount;
	iFixupCount = newCount;
	iFixups = fixups;
	return newFixups;
	}


/**
Helper function for FixupImports.  Takes the set of
64-bit <addr,val> fixups, and organizes them into pages.

Each page is stored as fXXX YYYY ZZZZ where YYYY ZZZZ is written
to the word at offset XXX.  (See PREQ1110 Design Sketch v1.0 S3.1.1.2.3.2.)

On success iImportFixupTableSize is set to the table size in bytes,
and iImportFixupTable is a cell containing the table.

@return					Symbian OS error code.
*/
TInt E32Image::BuildImportFixupTable()
	{
	__IF_DEBUG(Printf(">BuildImportFixupTable,%d@%08x,%08x", iFixupCount, iFixups, iCodeLoadAddress));

#ifdef _DEBUG
	// Dump the incoming fixup table if loader tracing enabled. Each item is an
	// (address, value) pair, where the address and the value are 32 bits each.
	TInt i;
	for (i = 0; i < iFixupCount; ++i)
		{
		TUint64 x = iFixups[i];
		__IF_DEBUG(Printf("%04x: %08x %08x", i*sizeof(TUint64), I64HIGH(x), I64LOW(x)));
		}
#endif	// DEBUG

	// sort the array in address order, to organize by page
	RArray<TUint64> fixup64ToSort(sizeof(TUint64), iFixups, iFixupCount);

	// address is in high word of entry, offset 4
	fixup64ToSort.SetKeyOffset(4);
	fixup64ToSort.SortUnsigned();

	// now have <address | new-value> pairs, organize into pages.
	// Each page is stored as fXXX YYYY ZZZZ where YYYY ZZZZ is written
	// to the word at offset XXX.  (See PREQ1110 Design Sketch v1.0 S3.1.1.2.3.2.)

	TUint32 pageCount = SizeToPageCount(iCodeSize);
	iImportFixupTableSize = (pageCount+1)*sizeof(TUint32) + 3*iFixupCount*sizeof(TUint16);
	iImportFixupTable = (TUint32*) User::Alloc(iImportFixupTableSize);
	__IF_DEBUG(Printf("iImportFixupTable=0x%08x", iImportFixupTable));
	if (iImportFixupTable == 0)
		return KErrNoMemory;

	// byte offsets of pages into the table are written as 32-bit words at
	// the start of the table

	TUint32 lastPage = 0;
	// byte index of first 48-bit entry in the table, after sentinel index
	iImportFixupTable[0] = (pageCount + 1) * sizeof(TUint32);;

	// location to which 48-bit imports are written
	TUint16* importOffset = (TUint16*)(iImportFixupTable + pageCount + 1);

	// location from where 64-bit <addr,val> pairs are read
	const TUint64* avEnd = iFixups + iFixupCount;

	for (const TUint64* avPtr = iFixups; avPtr < avEnd; ++avPtr)
		{
		TUint64 addr_val = *avPtr;
		TUint32 addr = I64HIGH(addr_val) - iCodeLoadAddress;
		TUint32 page = addr >> 12;
		if (page > lastPage)
			{
			// calculate new start index for current page
			TUint32 newStart = TUint32(importOffset) - TUint32(iImportFixupTable);

			__IF_DEBUG(Printf("page=%d, lastPage=%d, newStart=0x%08x", page, lastPage, newStart));

			// mark intermediate pages as zero-length, starting and ending at
			// current offset
			while (++lastPage <= page)
				iImportFixupTable[lastPage] = newStart;
			--lastPage;
			}

		TUint16 offsetIntoPage;
		offsetIntoPage = (addr & KPageOffsetMask);
		*importOffset++ = offsetIntoPage;

		TUint32 val = I64LOW(addr_val);
		*importOffset++ = val;				// low halfword stored first (YYYY)
		*importOffset++ = val >> 16;		// high halfword stored second (ZZZZ)
		}

	// sentinel value marks end of table
	while (++lastPage <= pageCount)
		iImportFixupTable[lastPage] = iImportFixupTableSize;

#ifdef _DEBUG
	__IF_DEBUG(Printf("processed fixup table (size=%d,pageCount=%d)", iImportFixupTableSize, pageCount));

	// Dump the processed fixup table if loader tracing enabled. The dump is in two
	// parts; first, the page indexes (1 word per page), then the entries describing
	// the items to be relocated, each of which is a 16-bit offset-within-page and a
	// 32-bit value to be stored there.
	for (i = 0; i <= (TInt)pageCount; ++i)
		__IF_DEBUG(Printf("%04x: %08x", i*4, iImportFixupTable[i]));

	const TUint16* table16 = (const TUint16*)iImportFixupTable;
	const TInt halfWordsInTable = iImportFixupTableSize / 2;
	for (i *= 2; i < halfWordsInTable; i += 3)
		__IF_DEBUG(Printf("%04x: %04x %04x%04x", i*2, table16[i+0], table16[i+2], table16[i+1]));
#endif

	User::Free(iFixups);
	iFixups = 0;
	return KErrNone;
	}


TInt GetModuleInfo(RLdrReq& aReq)
//
//	Read capabilities from file found
//
	{
	__IF_DEBUG(Printf("ReadModuleInfo %S",aReq.iFileName));
	TFileNameInfo& fi = aReq.iFileNameInfo;
	RImageFinder finder;
	TInt r = finder.Set(aReq);
	if (r == KErrNone)
		{
		finder.iFindExact = ETrue;

		r = KErrNotSupported;

		// must specify a fully qualified name
		if (fi.DriveLen() && fi.PathLen())
			{
			if (fi.VerLen())
				aReq.iRequestedVersion = fi.iVersion;
			else
				aReq.iRequestedVersion = KModuleVersionWild;
			r = finder.Search();
			if (r == KErrNone)
				{
				RLibrary::TInfo ret_info;
				memclr(&ret_info,sizeof(ret_info));
				ret_info.iModuleVersion = finder.iNew.iModuleVersion;
				ret_info.iUids = *(const TUidType*)finder.iNew.iUid;
				*(SSecurityInfo*)&ret_info.iSecurityInfo = finder.iNew.iS;
				TPckgC<RLibrary::TInfo> ret_pckg(ret_info);
				r = aReq.iMsg->Write(2, ret_pckg);
				}
			}
		}
	finder.Close();
	return r;
	}

TInt GetInfoFromHeader(const RLoaderMsg& aMsg)
	{
	TInt r;

	// Get size of header supplied by client
	TInt size;
	size = aMsg.GetDesLength(0);
	if(size<0)
		return size;
	if(size>RLibrary::KRequiredImageHeaderSize)
		size = RLibrary::KRequiredImageHeaderSize;
	if((TUint)size<sizeof(E32ImageHeaderV))
		return KErrUnderflow;

	// Get header data
	TUint8* data = new TUint8[size];
	if(!data)
		return KErrNoMemory;
	TPtr8 ptr(data,size);
	r = aMsg.Read(0,ptr);
	if(r==KErrNone)
		{
		// Check header is valid
		E32ImageHeaderV* header=(E32ImageHeaderV*)data;
		if(header->TotalSize()>size)
			r = KErrUnderflow;
		else
			{
			TUint32 uncompressedSize;
			r = header->ValidateHeader(-1,uncompressedSize);
			}
		if(r==KErrNone)
			{
			// Get info
			RLibrary::TInfoV2 ret_info;
			memclr(&ret_info,sizeof(ret_info));
			ret_info.iModuleVersion = header->ModuleVersion();
			ret_info.iUids = (TUidType&)header->iUid1;
			header->GetSecurityInfo((SSecurityInfo&)ret_info.iSecurityInfo);
			ret_info.iHardwareFloatingPoint = (header->iFlags & KImageHWFloatMask) >> KImageHWFloatShift;

			ret_info.iDebugAttributes = 0;	// default
			if (header->iFlags & KImageDebuggable)
				ret_info.iDebugAttributes |= RLibrary::TInfoV2::EDebugAllowed;

			TPckg<RLibrary::TInfoV2> ret_pckg(ret_info);
			TInt max = aMsg.GetDesMaxLength(1);
			if (ret_pckg.Length() > max)
				ret_pckg.SetLength(max);
			r = aMsg.Write(1, ret_pckg);
			}
		}

	delete[] data;
	return r;
	}

#if defined(_DEBUG) || defined(_DEBUG_RELEASE)
void memory_dump(const TAny* a, TUint l)
	{
	TBuf8<80> buf;
	const TUint8* s = (const TUint8*)a;
	TInt n=0;
	while (l)
		{
		buf.Append(' ');
		buf.AppendNumFixedWidth(*s++, EHex, 2);
		--l;
		++n;
		if (l==0 || n==16)
			{
			RDebug::Printf((const char*)buf.PtrZ());
			buf.Zero();
			n=0;
			}
		}
	}

void RImageFinder::Dump(const char* aTitle, TInt aR)
	{
	RDebug::Printf(aTitle);
	RDebug::Printf("r=%d",aR);
	if (iExisting)
		{
		RDebug::Printf("Existing image found");
		RDebug::Printf("Filename=%S Attr=%08x", &iExisting->iFileName, iExisting->iAttr);
		RDebug::Printf("SID %08x Caps %08x %08x", iExisting->iS.iSecureId, iExisting->iS.iCaps[1], iExisting->iS.iCaps[0]);
		const TUint32* uid = (const TUint32*)&iExisting->iUids;
		RDebug::Printf("UIDs %08x %08x %08x VER %08x", uid[0], uid[1], uid[2], iExisting->iModuleVersion);
		RDebug::Printf("Rom %08x", iExisting->iRomImageHeader);
		}
	else if (iNewValid)
		{
		RDebug::Printf("New image found");
		RDebug::Printf("Filename=%S Attr=%08x", &iNewFileName, iNew.iAttr);
		RDebug::Printf("SID %08x Caps %08x %08x", iNew.iS.iSecureId, iNew.iS.iCaps[1], iNew.iS.iCaps[0]);
		const TUint32* uid = (const TUint32*)iNew.iUid;
		RDebug::Printf("UIDs %08x %08x %08x VER %08x", uid[0], uid[1], uid[2], iNew.iModuleVersion);
		RDebug::Printf("Rom %08x", iNew.iRomImageHeader);
		}
	else
		{
		RDebug::Printf("No suitable image found");
		RDebug::Printf("#NM=%d #UidFail=%d #CapFail=%d #MajVFail=%d #ImpFail=%d", iNameMatches, iUidFail, iCapFail, iMajorVersionFail, iImportFail);
		}
	}

void DumpImageHeader(const E32ImageHeader* a)
	{
	RDebug::Printf("E32ImageHeader at %08x :", a);
	TUint abi = a->ABI();
	TUint hdrfmt = a->HeaderFormat();
	TUint impfmt = a->ImportFormat();
	TUint eptfmt = a->EntryPointFormat();
	RDebug::Printf("Header format %d", hdrfmt>>KImageHdrFmtShift);
	RDebug::Printf("Import format %d", impfmt>>KImageImpFmtShift);
	RDebug::Printf("EntryPoint format %d", eptfmt>>KImageEptShift);
	RDebug::Printf("ABI %d", abi>>KImageABIShift);
	RDebug::Printf("UIDs %08x %08x %08x (%08x)", a->iUid1, a->iUid2, a->iUid3, a->iUidChecksum);
	RDebug::Printf("Header CRC %08x", a->iHeaderCrc);
	RDebug::Printf("Signature %08x", a->iSignature);
	RDebug::Printf("CPU %08x", (TUint)a->CpuIdentifier());
	RDebug::Printf("ModuleVersion %08x", a->ModuleVersion());
	RDebug::Printf("Compression Type %08x", a->CompressionType());
	RDebug::Printf("Tools Version %d.%02d(%d)", a->iToolsVersion.iMajor, a->iToolsVersion.iMinor, a->iToolsVersion.iBuild);
	RDebug::Printf("Flags %08x", a->iFlags);
	RDebug::Printf("Code Size %08x", a->iCodeSize);
	RDebug::Printf("Text Size %08x", a->iTextSize);
	RDebug::Printf("Data Size %08x", a->iDataSize);
	RDebug::Printf("BSS Size %08x", a->iBssSize);
	RDebug::Printf("Stack Size %08x", a->iStackSize);
	RDebug::Printf("HeapSizeMin %08x", a->iHeapSizeMin);
	RDebug::Printf("HeapSizeMax %08x", a->iHeapSizeMax);
	RDebug::Printf("iEntryPoint %08x", a->iEntryPoint);
	RDebug::Printf("iCodeBase %08x", a->iCodeBase);
	RDebug::Printf("iDataBase %08x", a->iDataBase);
	RDebug::Printf("DLL Ref Table Count %d", a->iDllRefTableCount);
	RDebug::Printf("Export Dir Count %d", a->iExportDirCount);
	RDebug::Printf("Code Offset %08x", a->iCodeOffset);
	RDebug::Printf("Data Offset %08x", a->iDataOffset);
	RDebug::Printf("Code Reloc Offset %08x", a->iCodeRelocOffset);
	RDebug::Printf("Data Reloc Offset %08x", a->iDataRelocOffset);
	RDebug::Printf("Import Offset %08x", a->iImportOffset);
	RDebug::Printf("Export Dir Offset %08x", a->iExportDirOffset);
	RDebug::Printf("Priority %d", (TUint)a->ProcessPriority());
	// KImageHdrFmt_J
	RDebug::Printf("iUncompressedSize %08x", ((E32ImageHeaderComp*)a)->iUncompressedSize);
	// KImageHdrFmt_V
	E32ImageHeaderV* v = (E32ImageHeaderV*)a;
	RDebug::Printf("SID %08x VID %08x CAP %08x %08x", v->iS.iSecureId, v->iS.iVendorId, v->iS.iCaps[1], v->iS.iCaps[0]);
	RDebug::Printf("iExportDescType %02x", v->iExportDescType);
	RDebug::Printf("iExportDescSize %04x", v->iExportDescSize);
	if (v->iExportDescSize)
		memory_dump(v->iExportDesc, v->iExportDescSize);
	}
#endif

