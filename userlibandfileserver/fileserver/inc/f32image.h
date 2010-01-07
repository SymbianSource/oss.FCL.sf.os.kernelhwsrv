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
// f32\inc\f32image.h
// 
// WARNING: This file contains some APIs which are internal and are subject
//          to change without notice. Such APIs should therefore not be used
//          outside the Kernel and Hardware Services package.
//

/**
 @file f32\inc\f32image.h
 @internalTechnology
*/

#ifndef __F32IMAGE_H__
#define __F32IMAGE_H__
#include <e32cmn.h>
#include <e32ldr.h>
#include <e32ldr_private.h>

/**
Value used for E32ImageHeader::iCpuIdentifier.
*/
enum TCpu
	{
	ECpuUnknown=0, ECpuX86=0x1000, ECpuArmV4=0x2000, ECpuArmV5=0x2001, ECpuArmV6=0x2002, ECpuMCore=0x4000
	};

/**
Ordinal value of the first entry in an executables export directory.
@see E32ImageHeader::iExportDirOffset.
*/
const TInt KOrdinalBase=1;

/**
Value used to initialise E32ImageHeader::iHeaderCrc prior to CRC generation.
*/
const TUint32 KImageCrcInitialiser	= 0xc90fdaa2u;


/**
Byte offset from an executable's entrypoint to the code segment ID storage location.
*/
const TUint KCodeSegIdOffset = 12;

//
// Flags fields for E32ImageHeader::iFlags
//

const TUint KImageDll				= 0x00000001u;	///< Flag set if executable is a DLL, clear if an EXE.

const TUint KImageNoCallEntryPoint	= 0x00000002u;	///< Obsolete flag ignored since Symbian OS version 8.1b.

const TUint KImageFixedAddressExe	= 0x00000004u;	///< Executable's data should not move when running on the moving memory model.

const TUint KImageABIMask			= 0x00000018u;	///< Bitmask for ABI value.
const TInt	KImageABIShift			= 3;			///< Bit shift count for ABI value.
const TUint	KImageABI_GCC98r2		= 0x00000000u;	///< Obsolete ABI for ARM targets.
const TUint	KImageABI_EABI			= 0x00000008u;	///< ARM EABI

const TUint KImageEptMask			= 0x000000e0u;	///< Bitmask for Entrypoint value.
const TInt	KImageEptShift			= 5;			///< Bit shift count for Entrypoint value
const TUint KImageEpt_Eka1			= 0x00000000u;	///< @removed Obsolete format not used since Symbian OS version 8.1b.
const TUint KImageEpt_Eka2			= 0x00000020u;	///< Standard entrypoint for ARM executable.

const TUint KImageCodeUnpaged			= 0x00000100u;	///< Executable image should not be demand paged. Exclusive with KImageCodePaged,
const TUint KImageCodePaged				= 0x00000200u;	///< Executable image should be demand paged. Exclusive with KImageCodeUnpaged,

const TUint KImageNmdExpData		= 0x00000400u;	///< Flag to indicate when named symbol export data present in image

const TUint KImageDebuggable		= 0x00000800u;	///< Flag to indicate image is debuggable

const TUint KImageDataUnpaged		= 0x00001000u;	///< Flag to indicate the image should not be data paged. Exclusive with KImageDataPaged.
const TUint KImageDataPaged			= 0x00002000u;	///< Flag to indicate the image should be data paged. Exclusive with KImageDataUnpaged.
const TUint KImageDataPagingMask 	= KImageDataUnpaged | KImageDataPaged;	///< Mask for data paging flags.

const TUint KImageSMPSafe			= 0x00004000u;	///< Flag to indicate image is SMP safe

const TUint KImageHWFloatMask		= 0x00f00000u;	///< Bitmask for Floating Point type.
const TInt	KImageHWFloatShift		= 20;			///< Bit shift count for Floating Point type.
const TUint	KImageHWFloat_None		= EFpTypeNone << KImageHWFloatShift;	///< No hardware floating point used.
const TUint KImageHWFloat_VFPv2		= EFpTypeVFPv2 << KImageHWFloatShift;	///< ARM VFPv2 floating point used.
const TUint KImageHWFloat_VFPv3		= EFpTypeVFPv3 << KImageHWFloatShift;	///< ARM VFPv3 floating point used. This includes Advanced SIMD (NEON).
const TUint KImageHWFloat_VFPv3D16	= EFpTypeVFPv3D16 << KImageHWFloatShift;	///< ARM VFPv3-D16 floating point used. This does not include Advanced SIMD (NEON).

const TUint KImageHdrFmtMask		= 0x0f000000u;	///< Bitmask for header format type.
const TInt	KImageHdrFmtShift		= 24;			///< Bit shift count for header format type.
const TUint KImageHdrFmt_Original	= 0x00000000u;	///< @removed Obsolete format not used since Symbian OS version 8.1b.
const TUint KImageHdrFmt_J			= 0x01000000u;	///< @removed Obsolete format not used since Symbian OS version 8.1b.
const TUint KImageHdrFmt_V			= 0x02000000u;	///< Header has format given by class E32ImageHeaderV.

const TUint KImageImpFmtMask		= 0xf0000000u;	///< Bitmask for import section format type.
const TInt	KImageImpFmtShift		= 28;			///< Bit shift count for import section format type.
const TUint KImageImpFmt_PE			= 0x00000000u;	///< PE-derived imports.
const TUint KImageImpFmt_ELF		= 0x10000000u;	///< ELF-derived imports.
const TUint KImageImpFmt_PE2		= 0x20000000u;	///< PE-derived imports without redundant copy of import ordinals.




// forward references...
class RFile;
class E32RelocSection;


/**
Structure for an executable image's header.
This is extended by E32ImageHeaderComp and E32ImageHeaderV.
All executables since Symbian OS version 8.1b have an header given by class E32ImageHeaderV.

Summary of an executable image structure...

- Header,			0..iCodeOffset-1
- Code part,		iCodeOffset..iCodeOffset+iCodeSize-1
	- .text section,				0 + iTextSize
	- Import Address Table (IAT),	iText + ?
	- Export Directory,				iExportDirOffset + iExportDirCount*4 (in .text Section)
- Rest of data,		iCodeOffset+iCodeSize..EOF
	- .data section,				iDataOffset + iDataSize
	- Import section, 				iImportOffset + sizeof(E32ImportSection)+?
	- Code relocation section,		iCodeRelocOffset + sizeof(E32RelocSection)+?
	- Data relocation section,		iDataRelocOffset + sizeof(E32RelocSection)+?
*/
class E32ImageHeader
	{
public:
	static TInt New(E32ImageHeader*& aHdr, RFile& aFile);
	static TInt New(E32ImageHeader*& aHdr, TUint8* aFileData, TUint32 aFileSize);
	TInt ValidateHeader(TInt aFileSize, TUint32& aUncompressedSize) const;

	inline static TUint ABIFromFlags(TUint aFlags);
	inline static TUint EptFromFlags(TUint aFlags);
	inline static TUint HdrFmtFromFlags(TUint aFlags);
	inline static TUint ImpFmtFromFlags(TUint aFlags);

	inline TUint ABI() const;
	inline TUint EntryPointFormat() const;
	inline TUint HeaderFormat() const;
	inline TUint ImportFormat() const;

	inline TUint32 CompressionType() const;
	inline TUint32 ModuleVersion() const;
	inline TInt TotalSize() const;
	inline TInt UncompressedFileSize() const;
	inline void GetSecurityInfo(SSecurityInfo& aInfo) const;
	inline TCpu CpuIdentifier() const;
	inline TProcessPriority ProcessPriority() const;
	inline TUint32 ExceptionDescriptor() const;
public:
	TUint32	iUid1;				///< KDynamicLibraryUidValue or KExecutableImageUidValue
	TUint32	iUid2;				///< Second UID for executable.
	TUint32	iUid3;				///< Third UID for executable.
	TUint32 iUidChecksum;		///< Checksum for iUid1, iUid2 and iUid3.
	TUint iSignature;			///< Contains 'EPOC'.
	TUint32	iHeaderCrc;			///< CRC-32 of entire header. @see #KImageCrcInitialiser.
	TUint32 iModuleVersion;		///< Version number for this executable (used in link resolution).
	TUint32 iCompressionType;	///< Type of compression used for file contents located after the header. (UID or 0 for none).
	TVersion iToolsVersion;		///< Version number of tools which generated this file.
	TUint32 iTimeLo;			///< Least significant 32 bits of the time of image creation, in milliseconds since since midnight Jan 1st, 2000.
	TUint32 iTimeHi;			///< Most significant 32 bits of the time of image creation, in milliseconds since since midnight Jan 1st, 2000.
	TUint iFlags;				///< Contains various bit-fields of attributes for the image.
	TInt iCodeSize;				///< Size of executables code. Includes import address table, constant data and export directory.
	TInt iDataSize;				///< Size of executables initialised data.
	TInt iHeapSizeMin;			///< Minimum size for an EXEs runtime heap memory.
	TInt iHeapSizeMax;			///< Maximum size for an EXEs runtime heap memory.
	TInt iStackSize;			///< Size for stack required by an EXEs initial thread.
	TInt iBssSize;				///< Size of executables uninitialised data.
	TUint iEntryPoint;			///< Offset into code of the entry point.
	TUint iCodeBase;			///< Virtual address that the executables code is linked for.
	TUint iDataBase;			///< Virtual address that the executables data is linked for.
	TInt iDllRefTableCount;		///< Number of executable against which this executable is linked. The number of files mention in the import section at iImportOffset.
	TUint iExportDirOffset;		///< Byte offset into file of the export directory.
	TInt iExportDirCount;		///< Number of entries in the export directory.
	TInt iTextSize;				///< Size of just the text section, also doubles as the offset for the Import Address Table w.r.t. the code section.
	TUint iCodeOffset;			///< Offset into file of the code section. Also doubles the as header size.
	TUint iDataOffset;			///< Offset into file of the data section.
	TUint iImportOffset;		///< Offset into file of the import section (E32ImportSection).
	TUint iCodeRelocOffset;		///< Offset into file of the code relocation section (E32RelocSection).
	TUint iDataRelocOffset;		///< Offset into file of the data relocation section (E32RelocSection).
	TUint16 iProcessPriority;	///< Initial runtime process priorty for an EXE. (Value from enum TProcessPriority.)
	TUint16 iCpuIdentifier;		///< Value from enum TCpu which indicates the CPU architecture for which the image was created
	};


/**
Extends E32ImageHeader.
*/
class E32ImageHeaderComp : public E32ImageHeader
	{
public:
	TUint32 iUncompressedSize;	///< Uncompressed size of file data after the header, or zero if file not compressed.
	};


/**
Extends E32ImageHeaderComp.
All Symbian OS executable files have a header in this format since OS version 8.1b.
*/
class E32ImageHeaderV : public E32ImageHeaderComp
	{
public:
	SSecurityInfo iS;				///< Platform Security information of executable.
	TUint32 iExceptionDescriptor;   ///< Offset in bytes from start of code section to Exception Descriptor, bit 0 set if valid.
	TUint32 iSpare2;				///< Reserved for future use. Set to zero.
	TUint16	iExportDescSize;		///< Size of export description stored in iExportDesc.
	TUint8	iExportDescType;		///< Type of description of holes in export table
	TUint8	iExportDesc[1];			///< Description of holes in export table, size given by iExportDescSize..
public:
	TInt ValidateWholeImage(TAny* aBufferStart, TUint aBufferSize) const;
	TInt ValidateHeader(TInt aFileSize, TUint32& aUncompressedSize) const;
	TInt ValidateExportDescription() const;
	TInt ValidateRelocations(TAny* aBufferStart, TUint aBufferSize, TUint aRelocationInfoOffset, TUint aRelocatedSectionSize, E32RelocSection*& aRelocationSection) const;
	TInt ValidateImports(TAny* aBufferStart, TUint aBufferSize, TUint& aBiggestImportCount) const;
	TInt ValidateAndAdjust(TUint32 aFileSize);
	};

// export description type E32ImageHeaderV::iExportDescType
const TUint	KImageHdr_ExpD_NoHoles			=0x00;	///< No holes, all exports present.
const TUint	KImageHdr_ExpD_FullBitmap		=0x01;	///< Full bitmap present at E32ImageHeaderV::iExportDesc
const TUint	KImageHdr_ExpD_SparseBitmap8	=0x02;	///< Sparse bitmap present at E32ImageHeaderV::iExportDesc, granularity 8
const TUint	KImageHdr_ExpD_Xip				=0xff;	///< XIP file


//
// inline getters for E32ImageHeader
//

/**
Extract ABI type from aFlags.
*/
inline TUint E32ImageHeader::ABIFromFlags(TUint aFlags)
	{
	return aFlags & KImageABIMask;
	}

/**
Extract ABI type from #iFlags.
*/
inline TUint E32ImageHeader::ABI() const
	{
	return ABIFromFlags(iFlags);
	}

/**
Extract entrypoint format from aFlags.
*/
inline TUint E32ImageHeader::EptFromFlags(TUint aFlags)
	{
	return aFlags & KImageEptMask;
	}

/**
Extract entrypoint format from #iFlags.
*/
inline TUint E32ImageHeader::EntryPointFormat() const
	{
	return EptFromFlags(iFlags);
	}

/**
Extract header format from aFlags.
*/
inline TUint E32ImageHeader::HdrFmtFromFlags(TUint aFlags)
	{
	return aFlags & KImageHdrFmtMask;
	}

/**
Extract header format from #iFlags.
*/
inline TUint E32ImageHeader::HeaderFormat() const
	{
	return HdrFmtFromFlags(iFlags);
	}

/**
Extract import format from aFlags.
*/
inline TUint E32ImageHeader::ImpFmtFromFlags(TUint aFlags)
	{
	return aFlags & KImageImpFmtMask;
	}

/**
Extract import format from #iFlags.
*/
inline TUint E32ImageHeader::ImportFormat() const
	{
	return ImpFmtFromFlags(iFlags);
	}

/**
Return #iCompressionType.
*/
inline TUint32 E32ImageHeader::CompressionType() const
	{
	return iCompressionType;
	}

/**
Return #iModuleVersion.
*/
inline TUint32 E32ImageHeader::ModuleVersion() const
	{
	return iModuleVersion;
	}

/**
Return size of this header.
*/
inline TInt E32ImageHeader::TotalSize() const
	{
	return iCodeOffset;
	}

/**
Return total size of file after decompression, or -1 if file not compressed.
*/
inline TInt E32ImageHeader::UncompressedFileSize() const
	{
	if(iCompressionType==0)
		return -1;			// not compressed
	else
		return ((E32ImageHeaderComp*)this)->iUncompressedSize + TotalSize();
	}

/**
Return copy of security info, #E32ImageHeaderV::iS.
*/
inline void E32ImageHeader::GetSecurityInfo(SSecurityInfo& aInfo) const
	{
	aInfo = ((E32ImageHeaderV*)this)->iS;
	}

/**
Return #iCpuIdentifier.
*/
inline TCpu E32ImageHeader::CpuIdentifier() const
	{
	return (TCpu)iCpuIdentifier;
	}

/**
Return #iProcessPriority.
*/
inline TProcessPriority E32ImageHeader::ProcessPriority() const
	{
	return (TProcessPriority)iProcessPriority;
	}

/**
Return fffset in bytes from start of code section for the Exception Descriptor.
Or zero if not present.
*/
inline TUint32 E32ImageHeader::ExceptionDescriptor() const
	{
	TUint32 xd = ((E32ImageHeaderV*)this)->iExceptionDescriptor;

	if((xd & 1) && (xd != 0xffffffffu))
		return (xd & ~1);

	return 0;
	}


/**
A block of imports from a single executable.
These structures are conatined in a images Import Section (E32ImportSection).
*/
class E32ImportBlock
	{
public:
	inline const E32ImportBlock* NextBlock(TUint aImpFmt) const;
	inline TInt Size(TUint aImpFmt) const;
	inline const TUint* Imports() const;	// import list if present
public:
	TUint32	iOffsetOfDllName;			///< Offset from start of import section for a NUL terminated executable (DLL or EXE) name.
	TInt	iNumberOfImports;			///< Number of imports from this executable.
//	TUint	iImport[iNumberOfImports];	///< For ELF-derived executes: list of code section offsets. For PE, list of imported ordinals. Omitted in PE2 import format
	};

/**
Return size of this import block.
@param aImpFmt Import format as obtained from image header.
*/
inline TInt E32ImportBlock::Size(TUint aImpFmt) const
	{
	TInt r = sizeof(E32ImportBlock);
	if(aImpFmt!=KImageImpFmt_PE2)
		r += iNumberOfImports * sizeof(TUint);
	return r;
	}

/**
Return pointer to import block which immediately follows this one.
@param aImpFmt Import format as obtained from image header.
*/
inline const E32ImportBlock* E32ImportBlock::NextBlock(TUint aImpFmt) const
	{
	const E32ImportBlock* next = this + 1;
	if(aImpFmt!=KImageImpFmt_PE2)
		next = (const E32ImportBlock*)( (TUint8*)next + iNumberOfImports * sizeof(TUint) );
	return next;
	}

/**
Return address of first import in this block.
For import format KImageImpFmt_ELF, imports are list of code section offsets.
For import format KImageImpFmt_PE, imports are a list of imported ordinals.
For import format KImageImpFmt_PE2, the import list is not present and should not be accessed.
*/
inline const TUint* E32ImportBlock::Imports() const
	{
	return (const TUint*)(this + 1);
	}


/**
Header for the Import Section in an image, as referenced by E32ImageHeader::iImportOffset.
Immediately following this structure are an array of E32ImportBlock structures.
The number of these is given by E32ImageHeader::iDllRefTableCount.
*/
class E32ImportSection
	{
public:
	TInt iSize;		///< Size of this section excluding 'this' structure
//	E32ImportBlock iImportBlock[iDllRefTableCount];
	};


/**
A block of relocations for a single page (4kB) of code/data.

Immediately following this structure are an array of TUint16 values
each representing a single value in the page which is to be relocated.
The lower 12 bits of each entry is the offset, in bytes, from start of this page.
The Upper 4 bits are the relocation type to be applied to the 32-bit value located
at that offset.
	- 1 means relocate relative to code section.
	- 2 means relocate relative to data section.
	- 3 means relocate relative to code or data section; calculate which.

A value of all zeros (0x0000) is ignored. (Used for padding structure to 4 byte alignment).
*/
class E32RelocBlock
	{
public:
	TUint32 iPageOffset;	///< Offset, in bytes, for the page being relocated; relative to the section start. Always a multiple of the page size: 4096 bytes.
	TUint32 iBlockSize;		///< Size, in bytes, for this block structure. Always a multiple of 4.
//	TUint16 iEntry[]
	};


/**
Header for a Relocation Section in an image, as referenced by E32ImageHeader::iCodeRelocOffset
or E32ImageHeader::iDataRelocOffset.

Immediately following this structure are an array of E32RelocBlock structures.
*/
class E32RelocSection
	{
public:
	TInt iSize;				///< Size of this relocation section including 'this' structure. Always a multiple of 4.
	TInt iNumberOfRelocs;	///< Number of relocations in this section.
//	E32RelocBlock iRelockBlock[];
	};


/**
Structure contained in the export directory in text section of the stdexe/stddll.
It contains information on the names of symbols exported by this stdexe/stddll and
pointers to a E32EpocExpSymInfoHdr structure of any stddlls that are dependencies of
this stdexe/stddll.

This is not used for emulator images see E32EmulExpSymInfoHdr below.
@see E32EmulExpSymInfoHdr
*/
class E32EpocExpSymInfoHdr
	{
public:
	TInt	iSize;						// size of this Table
	TInt16	iFlags; 
	TInt16	iSymCount;					// number of symbols
	TInt	iSymbolTblOffset;			// start of the symbol table - offset from byte 0 of this header
	TInt	iStringTableSz;				// size of the string table
	TInt	iStringTableOffset;			// start of the string table having names of the symbols - offset from byte 0 of this header
	TInt	iDllCount;					// Number of dependent DLLs
	TInt	iDepDllZeroOrdTableOffset;	// offset of the DLL dependency table - offset from byte 0 of this header.
	};


/**
Header of the structure contained in the 'KWin32SectionName_NmdExpData' 
segment of emulator stdexe & stddll images.
The segment contains addresses of symbols and NULL 
terminated ASCII strings of the names of static dependencies.
For a stdexe, this segment contains the following:
	a) symbol count (iSymCount) and static dependency count (iDllCount)
	b) iSymCount * symbol addresses
	c) iSymCount * symbol names
	d) iDllCount * dependency names
	
For a stddll, this segment contains the following:
	a) symbol count (iSymCout) is always 0
	b) static dependency count (iDllCount)
	c) iDllCount * dependency names
The symbol addresses and names are not required for a stddll as the Windows API,
GetProcAddress may be used to get the addresses for symbol names.
Since this API works only on DLL handles, we explicitly list them for stdexes.
This is used for emulator images only.
*/
class E32EmulExpSymInfoHdr
	{
public:
	TInt32	iSymCount;		// Number of symbols
	TInt32	iDllCount;		// Number of static dependency DLLs
	};



#ifdef INCLUDE_E32IMAGEHEADER_IMPLEMENTATION

// include code which implements validation functions...

#ifndef RETURN_FAILURE
#define RETURN_FAILURE(_r) return (_r)
#endif

#ifndef E32IMAGEHEADER_TRACE
#define E32IMAGEHEADER_TRACE(_t) ((void)0)
#endif


#include <e32uid.h>


/**
Validate this image header.

After successful validation the following are true:
	- File size is big enough to contain the entire header.
	- Values #iUidChecksum, #iSignature and #iHeaderCrc are correct.
	- CPU type (#iCpuIdentifier), ABI type (#iFlags&#KImageABIMask) and
	  entrypoint type (#iFlags&#KImageEptMask) are valid for this system.
	- Code part of file as specified by #iCodeOffset and #iCodeSize is fully within the file.
	- Text section size (#iTextSize) is within code part.
	- Entrypoint value (#iEntryPoint) lies within the code part and is aligned correctly.
	- Export directory as specified by #iExportDirCount and #iExportDirOffset is fully
	  within code part and is aligned correctly.
	- Exception description (E32ImageHeaderV::iExceptionDescriptor), if present,
	  lies within the code part.
	- Data part of file as specified by #iDataOffset and #iDataSize is fully within the file.
	  Or data is not present (#iDataOffset==#iDataSize==0).
	- Import section (class E32ImportSection at #iImportOffset) is within 'rest of data'
	  and aligned correctly. Data following the E32ImportSection header is NOT validated or
	  checked if it is fully contained within the file.
	- Code relocations (class E32RelocSection at #iCodeRelocOffset) is within 'rest of data'
	  and aligned correctly. Data following the E32RelocSection header is NOT validated or
	  checked if it is fully contained within the file.
	- Data relocations (class E32RelocSection at #iDataRelocOffset) is within 'rest of data'
	  and aligned correctly. Data following the E32RelocSection header is NOT validated or
	  checked if it is fully contained within the file.
	- Export description is validated by E32ImageHeaderV::ValidateExportDescription().
	- #iUid1 is consistant with #iFlags&#KImageDll. I.e. if flaged as a DLL, #iUid1 is
	  KDynamicLibraryUidValue, otherwise it is KExecutableImageUidValue.
	- Version number (#iModuleVersion) is valid. (Major and minor versions are <32768).
	- File compression type (#iCompressionType) is supported.
	- #iHeapSizeMax>=#iHeapSizeMin
	- All signed values in header are not negative.

@param		aFileSize			Total size of the file from which this header was created.
@param[out] aUncompressedSize	Returns the total size that the file data would be once decompressed.

@return KErrNone if no errors detected;
		KErrCorrupt if errors found;
		KErrNotSupported if image format not supported on this platform.
*/
TInt E32ImageHeader::ValidateHeader(TInt aFileSize, TUint32& aUncompressedSize) const
	{
	// check file is big enough for any header...
	if(TUint(aFileSize)<sizeof(*this))
		return KErrCorrupt;

	TUint hdrfmt = HeaderFormat();
	if(hdrfmt==KImageHdrFmt_V)
		return ((E32ImageHeaderV*)this)->ValidateHeader(aFileSize,aUncompressedSize);

	return KErrNotSupported; // header format unrecognised
	}

/**
Validate this image header.

@param aFileSize				Total size of the file from which this header was created.
@param[out] aUncompressedSize	Returns the total size that the file data would be once decompressed.

@return KErrNone if no errors detected;
		KErrCorrupt if errors found;
		KErrNotSupported if image format not supported on this platform.
*/
TInt E32ImageHeaderV::ValidateHeader(TInt aFileSize, TUint32& aUncompressedSize) const
	{
	const TUint KMaxDesSize = 0x0fffffffu; // maximum size of descriptor
	if(aFileSize==-1)
		{
		// file size unknown, set to maximum valid so rest of validation works...
		aFileSize = KMaxDesSize;
		}
	if(TUint(aFileSize)>KMaxDesSize)
		RETURN_FAILURE(KErrCorrupt); // file size negative or too big

	aUncompressedSize = 0;

	// check file is big enough to contain this header...
	if(aFileSize<(TInt)sizeof(*this))
		RETURN_FAILURE(KErrCorrupt);

	// check header format version...
	if((iFlags&KImageHdrFmtMask)!=KImageHdrFmt_V)
		RETURN_FAILURE(KErrNotSupported);

	// check header size...
	TUint headerSize = iCodeOffset;
	if(headerSize>TUint(aFileSize))
		RETURN_FAILURE(KErrCorrupt); // Fuzzer can't trigger this because Loader will fail earlier when reading header from file

	// check iCpuIdentifier...
	TCpu cpu = (TCpu)iCpuIdentifier;
	TBool isARM = (cpu==ECpuArmV4 || cpu==ECpuArmV5 || cpu==ECpuArmV6);
#if defined(__CPU_ARM)
	if(!isARM)
		RETURN_FAILURE(KErrNotSupported);
#elif defined(__CPU_X86)
	if(cpu!=ECpuX86)
		RETURN_FAILURE(KErrNotSupported);
#endif
	TUint32 pointerAlignMask = isARM ? 3 : 0;	// mask of bits which must be zero for aligned pointers/offsets

	// check iUid1,iUid2,iUid3,iUidChecksum...
	TUidType uids = *(const TUidType*)&iUid1;
	TCheckedUid chkuid(uids);
	const TUint32* pChkUid = (const TUint32*)&chkuid; // need hackery to verify the UID checksum since everything is private
	if(pChkUid[3]!=iUidChecksum)
		RETURN_FAILURE(KErrCorrupt);

	// check iSignature...
	if(iSignature!=0x434f5045) // 'EPOC'
		RETURN_FAILURE(KErrCorrupt);

	// check iHeaderCrc...
	TUint32 supplied_crc = iHeaderCrc;
	((E32ImageHeaderV*)this)->iHeaderCrc = KImageCrcInitialiser;
	TUint32 crc = 0;
	Mem::Crc32(crc, this, headerSize);
	((E32ImageHeaderV*)this)->iHeaderCrc = supplied_crc;
	if(crc!=supplied_crc)
		RETURN_FAILURE(KErrCorrupt);

	// check iModuleVersion...
	TUint32 mv = iModuleVersion;
	if(mv>=0x80000000u || (mv&0x0000ffffu)>0x8000u)
		RETURN_FAILURE(KErrNotSupported);

	// check iCompressionType and get uncompressed size...
	TUint compression = iCompressionType;
	TUint uncompressedSize = aFileSize;
	if(compression!=KFormatNotCompressed)
		{
		if(compression!=KUidCompressionDeflate && compression!=KUidCompressionBytePair)
	        RETURN_FAILURE(KErrNotSupported);  // unknown compression method
		uncompressedSize = headerSize+iUncompressedSize;
		if(uncompressedSize<headerSize)
			RETURN_FAILURE(KErrCorrupt); // size overflowed 32 bits
		}

	// check sizes won't overflow the limit for a descriptor (many Loader uses won't like that).
	if(uncompressedSize>KMaxDesSize)
		RETURN_FAILURE(KErrCorrupt);

	// check KImageDll in iFlags...
	if(iFlags&KImageDll)
		{
		if(iUid1!=TUint32(KDynamicLibraryUidValue))
			RETURN_FAILURE(KErrNotSupported);
		}
	else if(iUid1!=TUint32(KExecutableImageUidValue))
		RETURN_FAILURE(KErrNotSupported);

	// check iFlags for ABI and entry point types...
	if(isARM)
		{
		if((iFlags&KImageEptMask)!=KImageEpt_Eka2)
			RETURN_FAILURE(KErrNotSupported);
		#if defined(__EABI__)
			if((iFlags&KImageABIMask)!=KImageABI_EABI)
				RETURN_FAILURE(KErrNotSupported);
		#elif defined(__GCC32__)
			if((iFlags&KImageABIMask)!=KImageABI_GCC98r2)
				RETURN_FAILURE(KErrNotSupported);
		#endif
		}
	else
		{
		if(iFlags&KImageEptMask)
			RETURN_FAILURE(KErrNotSupported); // no special entry point type allowed on non-ARM targets
		if(iFlags&KImageABIMask)
			RETURN_FAILURE(KErrNotSupported);
		}

	// check iFlags for import format...
	if((iFlags&KImageImpFmtMask)>KImageImpFmt_PE2)
		RETURN_FAILURE(KErrNotSupported);

	// check iHeapSizeMin...
	if(iHeapSizeMin<0)
		RETURN_FAILURE(KErrCorrupt);

	// check iHeapSizeMax...
	if(iHeapSizeMax<iHeapSizeMin)
		RETURN_FAILURE(KErrCorrupt);

	// check iStackSize...
	if(iStackSize<0)
		RETURN_FAILURE(KErrCorrupt);

	// check iBssSize...
	if(iBssSize<0)
		RETURN_FAILURE(KErrCorrupt);

	// check iEntryPoint...
	if(iEntryPoint>=TUint(iCodeSize))
		RETURN_FAILURE(KErrCorrupt);
	if(iEntryPoint+KCodeSegIdOffset+sizeof(TUint32)>TUint(iCodeSize))
		RETURN_FAILURE(KErrCorrupt);
	if(iEntryPoint&pointerAlignMask)
		RETURN_FAILURE(KErrCorrupt); // not aligned

	// check iCodeBase...
	if(iCodeBase&3)
		RETURN_FAILURE(KErrCorrupt); // not aligned

	// check iDataBase...
	if(iDataBase&3)
		RETURN_FAILURE(KErrCorrupt); // not aligned

	// check iDllRefTableCount...
	if(iDllRefTableCount<0)
		RETURN_FAILURE(KErrCorrupt);
	if(iDllRefTableCount)
		{
		if(!iImportOffset)
			RETURN_FAILURE(KErrCorrupt); // we link to DLLs but have no import data
		}

	// check iCodeOffset and iCodeSize specify region in file...
	TUint codeStart = iCodeOffset;
	TUint codeEnd = codeStart+iCodeSize;
	if(codeEnd<codeStart)
		RETURN_FAILURE(KErrCorrupt);
//	if(codeStart<headerSize)
//		RETURN_FAILURE(KErrCorrupt); // can't happen because headerSize is defined as iCodeOffset (codeStart)
	if(codeEnd>uncompressedSize)
		RETURN_FAILURE(KErrCorrupt);

	// check iDataOffset and iDataSize specify region in file...
	TUint dataStart = iDataOffset;
	TUint dataEnd = dataStart+iDataSize;
	if(dataEnd<dataStart)
		RETURN_FAILURE(KErrCorrupt);
	if(!dataStart)
		{
		// no data...
		if(dataEnd)
			RETURN_FAILURE(KErrCorrupt);
		}
	else
		{
		if(dataStart<codeEnd)
			RETURN_FAILURE(KErrCorrupt);
		if(dataEnd>uncompressedSize)
			RETURN_FAILURE(KErrCorrupt);
		if((dataStart-codeStart)&pointerAlignMask)
			RETURN_FAILURE(KErrCorrupt); // data not aligned with respect to code
		}


	// check total data size isn't too bit...
	TUint totalDataSize = iDataSize+iBssSize;
	if(totalDataSize>0x7fff0000)
		RETURN_FAILURE(KErrNoMemory);

	// check iExportDirOffset and iExportDirCount specify region in code part...
	if(TUint(iExportDirCount)>65535)
		RETURN_FAILURE(KErrCorrupt); // too many exports
	if(iExportDirCount)
		{
		TUint exportsStart = iExportDirOffset;
		TUint exportsEnd = exportsStart+iExportDirCount*sizeof(TUint32);
		if(iFlags&KImageNmdExpData)
			exportsStart -= sizeof(TUint32); // allow for 0th ordinal
		if(exportsEnd<exportsStart)
			RETURN_FAILURE(KErrCorrupt);
		if(exportsStart<codeStart)
			RETURN_FAILURE(KErrCorrupt);
		if(exportsEnd>codeEnd)
			RETURN_FAILURE(KErrCorrupt);
		if((exportsStart-codeStart)&pointerAlignMask)
			RETURN_FAILURE(KErrCorrupt); // not aligned within code section
		}

	// check iTextSize...
	if(TUint(iTextSize)>TUint(iCodeSize))
		RETURN_FAILURE(KErrCorrupt);

	// check iImportOffset...
	TUint start = iImportOffset;
	if(start)
		{
		TUint end = start+sizeof(E32ImportSection); // minimum valid size
		if(end<start)
			RETURN_FAILURE(KErrCorrupt);
		if(start<codeEnd)
			RETURN_FAILURE(KErrCorrupt);
		if(end>uncompressedSize)
			RETURN_FAILURE(KErrCorrupt);
		if((start-codeEnd)&pointerAlignMask)
			RETURN_FAILURE(KErrCorrupt); // not aligned within 'rest of data'
		}

	// check iCodeRelocOffset...
	start = iCodeRelocOffset;
	if(start)
		{
		TUint end = start+sizeof(E32RelocSection); // minimum valid size
		if(end<start)
			RETURN_FAILURE(KErrCorrupt);
		if(start<codeEnd)
			RETURN_FAILURE(KErrCorrupt);
		if(end>uncompressedSize)
			RETURN_FAILURE(KErrCorrupt);
		if((start-codeEnd)&pointerAlignMask)
			RETURN_FAILURE(KErrCorrupt); // not aligned within 'rest of data'
		}

	// check iDataRelocOffset...
	start = iDataRelocOffset;
	if(start)
		{
		TUint end = start+sizeof(E32RelocSection); // minimum valid size
		if(end<start)
			RETURN_FAILURE(KErrCorrupt);
		if(start<codeEnd)
			RETURN_FAILURE(KErrCorrupt);
		if(end>uncompressedSize)
			RETURN_FAILURE(KErrCorrupt);
		if((start-codeEnd)&pointerAlignMask)
			RETURN_FAILURE(KErrCorrupt); // not aligned within 'rest of data'
		}

	// check exception descriptor...
	if(iExceptionDescriptor&1) // if valid...
		if(iExceptionDescriptor>=TUint(iCodeSize))
			RETURN_FAILURE(KErrCorrupt);

	TInt r = ValidateExportDescription();
	if(r!=KErrNone)
		RETURN_FAILURE(r);

	// done...
	aUncompressedSize = uncompressedSize;
	return KErrNone;
	}


/**
Valdate that the export description is valid.
*/
TInt E32ImageHeaderV::ValidateExportDescription() const
	{
	TUint headerSize = iCodeOffset;

	// check export description...
	TUint edSize = iExportDescSize + sizeof(iExportDescSize) + sizeof(iExportDescType);
	edSize = (edSize+3)&~3;
	TUint edEnd = _FOFF(E32ImageHeaderV,iExportDescSize)+edSize;
	if(edEnd!=headerSize)
		RETURN_FAILURE(KErrCorrupt);

	// size of bitmap of exports...
	TUint bitmapSize = (iExportDirCount+7) >> 3;

	// check export description bitmap...
	switch(iExportDescType)
		{
	case KImageHdr_ExpD_NoHoles:
		// no bitmap to check...
		E32IMAGEHEADER_TRACE(("ValidateExportDescription NoHoles"));
		return KErrNone;

	case KImageHdr_ExpD_FullBitmap:
		// full bitmap present...
		E32IMAGEHEADER_TRACE(("ValidateExportDescription FullBitmap"));
		if(bitmapSize!=iExportDescSize)
			RETURN_FAILURE(KErrCorrupt);
		return KErrNone;

	case KImageHdr_ExpD_SparseBitmap8:
		{
		// sparse bitmap present...
		E32IMAGEHEADER_TRACE(("ValidateExportDescription SparseBitmap8"));

		// get size of meta-bitmap...
		TUint metaBitmapSize = (bitmapSize+7) >> 3;
		if(metaBitmapSize>iExportDescSize)
			RETURN_FAILURE(KErrCorrupt); // doesn't fit

		TUint totalSize = metaBitmapSize;

		// scan meta-bitmap counting extra bytes which should be present...
		const TUint8* metaBitmap = iExportDesc;
		const TUint8* metaBitmapEnd = metaBitmap + metaBitmapSize;
		while(metaBitmap<metaBitmapEnd)
			{
			TUint bits = *metaBitmap++;
			do
				{
				if(bits&1)
					++totalSize; // another byte is present in bitmap
				}
			while(bits>>=1);
			}

		if(totalSize!=iExportDescSize)
			RETURN_FAILURE(KErrCorrupt);
		}
		return KErrNone;

	default:
		E32IMAGEHEADER_TRACE(("ValidateExportDescription ?"));
		RETURN_FAILURE(KErrNotSupported);
		}
	}


/**
Validate a relocation section.

@param aBufferStart				Start of buffer containing the data after the code part in the image file.
@param aBufferSize				Size of data at aBufferStart.
@param aRelocationInfoOffset	File offset for relocation section. (#iCodeRelocOffset or #iDataRelocOffset.)
@param aRelocatedSectionSize	Size of section being relocated. (#iCodeSize or #iDataSize.)
@param[out] aRelocationSection	Set to the start of the relocation section in the given buffer.

@return KErrNone if relocation section is valid, else KErrCorrupt.
*/
TInt E32ImageHeaderV::ValidateRelocations(TAny* aBufferStart, TUint aBufferSize, TUint aRelocationInfoOffset, TUint aRelocatedSectionSize, E32RelocSection*& aRelocationSection) const
	{
	aRelocationSection = 0;
	if(!aRelocationInfoOffset)
		return KErrNone; // no relocations

	// get alignment requirements...
	TCpu cpu = (TCpu)iCpuIdentifier;
	TBool isARM = (cpu==ECpuArmV4 || cpu==ECpuArmV5 || cpu==ECpuArmV6);
	TUint32 pointerAlignMask = isARM ? 3 : 0;	// mask of bits which must be zero for aligned pointers/offsets

	// buffer pointer to read relocation from...
	TUint8* bufferStart = (TUint8*)aBufferStart;
	TUint8* bufferEnd = bufferStart+aBufferSize;
	TUint baseOffset = iCodeOffset+iCodeSize; // file offset for aBufferStart
	TUint8* sectionStart = (bufferStart+aRelocationInfoOffset-baseOffset);
	TUint8* p = sectionStart;

	// read section header (ValidateHeader has alread checked this is OK)...
	E32RelocSection* sectionHeader = (E32RelocSection*)p;
	TUint size = sectionHeader->iSize;
	TUint relocsRemaining = sectionHeader->iNumberOfRelocs;
	E32IMAGEHEADER_TRACE(("E32RelocSection 0x%x %d",size,relocsRemaining));
	if(size&3)
		RETURN_FAILURE(KErrCorrupt); // not multiple of word size

	// calculate buffer range for block data...
	p = (TUint8*)(sectionHeader+1);  // start of first block
	TUint8* sectionEnd = p+size;
	if(sectionEnd<p)
		RETURN_FAILURE(KErrCorrupt); // math overflow
	if(sectionEnd>bufferEnd)
		RETURN_FAILURE(KErrCorrupt); // overflows buffer

	// process each block...
	while(p!=sectionEnd)
		{
		E32RelocBlock* block = (E32RelocBlock*)p;

		// get address of first entry in this block...
		TUint16* entryPtr = (TUint16*)(block+1);
		if((TUint8*)entryPtr<(TUint8*)block || (TUint8*)entryPtr>sectionEnd)
			RETURN_FAILURE(KErrCorrupt);  // overflows relocation section

		// read block header...
		TUint pageOffset = block->iPageOffset;
		TUint blockSize = block->iBlockSize;
		E32IMAGEHEADER_TRACE(("E32RelocSection block 0x%x 0x%x",pageOffset,blockSize));
		if(pageOffset&0xfff)
			RETURN_FAILURE(KErrCorrupt); // not page aligned
		if(blockSize<sizeof(E32RelocBlock))
			RETURN_FAILURE(KErrCorrupt); // blockSize must be at least that of the header just read
		if(blockSize&3)
			RETURN_FAILURE(KErrCorrupt); // not word aligned

		// caculate end of entries in this block...
		TUint16* entryEnd = (TUint16*)(p+blockSize);
		if(entryEnd<entryPtr)
			RETURN_FAILURE(KErrCorrupt); // math overflow
		if(entryEnd>(TUint16*)sectionEnd)
			RETURN_FAILURE(KErrCorrupt); // overflows relocation section

		// process each entry in this block...
		while(entryPtr<entryEnd)
			{
			TUint entry = *entryPtr++;
			E32IMAGEHEADER_TRACE(("E32RelocSection entry 0x%04x",entry));
			if(!entry)
				continue;

			// check relocation type...
			TUint entryType = entry&0xf000;
			if(entryType!=KTextRelocType && entryType!=KDataRelocType && entryType!=KInferredRelocType)
				RETURN_FAILURE(KErrCorrupt);

			// check relocation is within section being relocated...
			TUint offset = pageOffset+(entry&0x0fff);
			if(offset>=aRelocatedSectionSize || offset+4>aRelocatedSectionSize)
				RETURN_FAILURE(KErrCorrupt); // not within section
			if(offset&pointerAlignMask)
				RETURN_FAILURE(KErrCorrupt); // not aligned correctly

			// count each relocation processed...
			--relocsRemaining;
			}

		// next sub block...
		p = (TUint8*)entryEnd;
		}

	// check number of relocations in section header is correct...
	E32IMAGEHEADER_TRACE(("E32RelocSection relocsRemaining=%d",relocsRemaining));
	if(relocsRemaining)
		RETURN_FAILURE(KErrCorrupt); // incorrect number of entries

	aRelocationSection = sectionHeader;
	return KErrNone;
	}


/**
Validate an import section.

For PE format imports, this also verifies that the Import Address Table fits within the code
part of the image.

@param aBufferStart				Start of buffer containing the data after the code part in the image file.
@param aBufferSize				Size of data at aBufferStart.
@param[out] aBiggestImportCount	Largest number of imports the image has from any single dependency.

@return KErrNone if section is valid (or absent), else KErrCorrupt.
*/
TInt E32ImageHeaderV::ValidateImports(TAny* aBufferStart, TUint aBufferSize, TUint& aBiggestImportCount) const
	{
	if(!iImportOffset)
		{
		aBiggestImportCount = 0;
		return KErrNone; // no imports
		}

	// get alignment requirements...
	TCpu cpu = (TCpu)iCpuIdentifier;
	TBool isARM = (cpu==ECpuArmV4 || cpu==ECpuArmV5 || cpu==ECpuArmV6);
	TUint32 pointerAlignMask = isARM ? 3 : 0;	// mask of bits which must be zero for aligned pointers/offsets

	// buffer pointer to read imports from...
	TUint8* bufferStart = (TUint8*)aBufferStart;
	TUint8* bufferEnd = bufferStart+aBufferSize;
	TUint baseOffset = iCodeOffset+iCodeSize; // file offset for aBufferStart
	TUint8* sectionStart = (bufferStart+iImportOffset-baseOffset);
	TUint8* p = sectionStart;

	// read section header (ValidateHeader has alread checked this is OK)...
	E32ImportSection* sectionHeader = (E32ImportSection*)p;
	TUint size = sectionHeader->iSize;
	E32IMAGEHEADER_TRACE(("E32ImportSection 0x%x",size));

	// check section lies within buffer...
	p = (TUint8*)(sectionHeader+1);  // start of first import block
	TUint8* sectionEnd = sectionStart+size;
	if(sectionEnd<p)
		RETURN_FAILURE(KErrCorrupt); // math overflow or not big enough to contain header
	if(sectionEnd>bufferEnd)
		RETURN_FAILURE(KErrCorrupt); // overflows buffer

	// process each import block...
	TUint numDeps = iDllRefTableCount;
	TUint biggestImportCount = 0;
	TUint totalImports = 0;
	TUint importFormat = iFlags&KImageImpFmtMask;
	while(numDeps--)
		{
		// get block header...
		E32ImportBlock* block = (E32ImportBlock*)p;
		p = (TUint8*)(block+1);
		if(p<(TUint8*)block || p>sectionEnd)
			RETURN_FAILURE(KErrCorrupt); // overflows buffer

		E32IMAGEHEADER_TRACE(("E32ImportBlock 0x%x %d",block->iOffsetOfDllName,block->iNumberOfImports));

		// check import dll name is within section...
		TUint8* name = sectionStart+block->iOffsetOfDllName;
		if(name<sectionStart || name>=sectionEnd)
			RETURN_FAILURE(KErrCorrupt); // not within import section
		while(*name++ && name<sectionEnd)
			{}
		if(name[-1])
			RETURN_FAILURE(KErrCorrupt); // name overflows section
		E32IMAGEHEADER_TRACE(("E32ImportBlock %s",sectionStart+block->iOffsetOfDllName));

		// process import count...
		TUint numberOfImports = block->iNumberOfImports;
		if(numberOfImports>=0x80000000u/sizeof(TUint32))
			RETURN_FAILURE(KErrCorrupt); // size doesn't fit into a signed integer
		if(numberOfImports>biggestImportCount)
			biggestImportCount = numberOfImports;
		totalImports += numberOfImports;

		// process import data...

		// PE2 doesn't have any more data...
		if(importFormat==KImageImpFmt_PE2)
			continue;

		// get import data range...
		TUint32* imports = (TUint32*)p;
		TUint32* importsEnd = imports+numberOfImports;
		if(importsEnd<imports)
			RETURN_FAILURE(KErrCorrupt); // math overflow. Fuzzer can't trigger this because needs aBufferStart to be in to be >0x80000000
		if(importsEnd>(TUint32*)sectionEnd)
			RETURN_FAILURE(KErrCorrupt); // overflows buffer

		// move pointer on to next block...
		p = (TUint8*)importsEnd;

		if(importFormat==KImageImpFmt_ELF)
			{
			// check imports are in code section...
			TUint32 limit = iCodeSize-sizeof(TUint32);
			while(imports<importsEnd)
				{
				TUint32 i = *imports++;
				if(i>limit)
					RETURN_FAILURE(KErrCorrupt);
				if(i&pointerAlignMask)
					RETURN_FAILURE(KErrCorrupt); // not word aligned
				}
			}
		else if(importFormat==KImageImpFmt_PE)
			{
			// import data is not used, so don't bother checking it
			}
		else
			{
			RETURN_FAILURE(KErrCorrupt); // bad import format, Fuzzer can't trigger this because import format checked by header validation
			}

		// next block...
		p = (TUint8*)block->NextBlock(importFormat);
		}

	// done processing imports; for PE derived files now check import address table (IAT)...
	if(importFormat==KImageImpFmt_PE || importFormat==KImageImpFmt_PE2)
		{
		if(totalImports>=0x80000000u/sizeof(TUint32))
			RETURN_FAILURE(KErrCorrupt); // size doesn't fit into a signed integer
		TUint importAddressTable = iTextSize; // offset for IAT
		if(importAddressTable&pointerAlignMask)
			RETURN_FAILURE(KErrCorrupt); // Fuzzer can't trigger this because PE imports are for X86 which doesn't have alignment restrictions
		TUint importAddressTableEnd = importAddressTable+sizeof(TUint32)*totalImports;
		if(importAddressTableEnd<importAddressTable || importAddressTableEnd>TUint(iCodeSize))
			RETURN_FAILURE(KErrCorrupt); // import address table overflows code part of file
		E32IMAGEHEADER_TRACE(("E32ImportSection IAT offsets 0x%x..0x%x",importAddressTable,importAddressTableEnd));
		}

	aBiggestImportCount = biggestImportCount;
	return KErrNone;
	}




/**
Validate a whole executable image.

This runs all of the other validation methods in turn.

@param aBufferStart	Start of buffer containing the data after the header part of an image file.
@param aBufferSize	Size of data at aBufferStart.

@return KErrNone if image is valid, else KErrCorrupt or KErrNotSupported.
*/
TInt E32ImageHeaderV::ValidateWholeImage(TAny* aBufferStart, TUint aBufferSize) const
	{
	TUint32 dummyUncompressedSize;
	TInt r = ValidateHeader(TotalSize()+aBufferSize,dummyUncompressedSize);
	if(r!=KErrNone)
		return r;

	TInt endOfCodeOffset = iCodeSize;
	void* restOfFileData = ((TUint8*)aBufferStart)+endOfCodeOffset;
	TInt restOfFileSize = aBufferSize-endOfCodeOffset;

	E32RelocSection* dummy;
	r = ValidateRelocations(restOfFileData,restOfFileSize,iCodeRelocOffset,iCodeSize,dummy);
	if(r!=KErrNone)
		return r;
	r = ValidateRelocations(restOfFileData,restOfFileSize,iDataRelocOffset,iDataSize,dummy);
	if(r!=KErrNone)
		return r;

	TUint biggestImportCount; 
	r = ValidateImports(restOfFileData,restOfFileSize,biggestImportCount);
	if(r!=KErrNone)
		return r;

	return r;
	}


#endif	// INCLUDE_E32IMAGEHEADER_IMPLEMENTATION


#endif	// __F32IMAGE_H__


