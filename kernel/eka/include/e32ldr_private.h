// Copyright (c) 1995-2010 Nokia Corporation and/or its subsidiary(-ies).
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
// e32\include\e32ldr_private.h
// 

/**
 @file
 @internalTechnology
*/

#ifndef __E32LDR_PRIVATE_H__
#define __E32LDR_PRIVATE_H__

#include <e32ldr.h>
#include <e32cmn.h>


const TInt KMaxLibraryEntryPoints=0x100;

//
// Loader version number.
//
const TInt KLoaderMajorVersionNumber=1;
const TInt KLoaderMinorVersionNumber=0;

//
// IPC messages to the loader
//
enum TLoaderMsg
	{
	ELoadProcess=1,
	ELoadLibrary=2,
	ELoadLogicalDevice=3,
	ELoadPhysicalDevice=4,
	ELoadLocale=5,
	ELoadFileSystem=6,
	EGetInfo=7,
	ELoaderDebugFunction=8,
	ELoadFSExtension=9,
	EGetInfoFromHeader=10,
	ELoadFSPlugin=11,
	ELoaderCancelLazyDllUnload=12,
	ELdrDelete=13,
	ECheckLibraryHash=14, 
	ELoadFSProxyDrive=15,
    ELoadCodePage=16,
	ELoaderRunReaper=17,
    EMaxLoaderMsg
	};
//
// Loader message arguments:
//		0 = TLdrInfo
//		1 = Filename
//		2 = Command line (process) or path (library)
//
class TLdrInfo
	{
public:
	IMPORT_C TLdrInfo();		// for BC
public:
	TUidType iRequestedUids;
	TOwnerType iOwnerType;
	TInt iHandle;
	TUint32 iSecureId;
	TUint32 iRequestedVersion;
	TInt iMinStackSize;			// Size of new process stack 
	};
	

#ifndef __KERNEL_MODE__
#include <e32std.h>
//
// Loader client class
//
class RLoader : public RSessionBase
	{
public:
	IMPORT_C TInt Connect();
	TVersion Version() const;
	TInt LoadProcess(TInt& aHandle, const TDesC& aFileName, const TDesC& aCommand, const TUidType& aUidType, TOwnerType aType);
	IMPORT_C TInt LoadLibrary(TInt& aHandle, const TDesC& aFileName, const TDesC& aPath, const TUidType& aType, TUint32 aModuleVersion);
	IMPORT_C TInt GetInfo(const TDesC& aFileName, TDes8& aInfoBuf);
	TInt LoadDeviceDriver(const TDesC& aFileName, TInt aDeviceType);
	IMPORT_C TInt DebugFunction(TInt aFunction, TInt a1, TInt a2, TInt a3);
	TInt LoadLocale(const TDesC& aLocaleDllName, TLibraryFunction* aExportList);
	TInt GetInfoFromHeader(const TDesC8& aHeader, TDes8& aInfoBuf);
	IMPORT_C TInt CancelLazyDllUnload();
	IMPORT_C TInt RunReaper();
	IMPORT_C TInt Delete(const TDesC& aFileName);
    IMPORT_C TInt CheckLibraryHash(const TDesC& aFileName, TBool aValidateHash=EFalse);
	TInt LoadProcess(TInt& aHandle, const TDesC& aFileName, const TDesC& aCommand, const TUidType& aUidType, TInt aMinStackSize, TOwnerType aType);
public:
#ifdef __ARMCC__
	// workaround for possible EDG bug (!!)
	inline TInt SendReceive(TInt aFunction,const TIpcArgs& aArgs) const
	  { return RSessionBase::SendReceive(aFunction, aArgs); }
#else
	using RSessionBase::SendReceive;
#endif
	};
#endif

//
// Information required to create a new code segment
//
enum TCodeSegAttributes
	{
	ECodeSegAttKernel			=0x00000001,
	ECodeSegAttGlobal			=0x00000002,
	ECodeSegAttFixed			=0x00000004,
	ECodeSegAttABIMask			=0x00000018,	  // same values as in image header
	ECodeSegAttCodePaged		=0x00000200,	  // the code seg is demand paged
	ECodeSegAttDataPaged		=0x00002000,	  // the code seg static data is demand paged
	ECodeSegAttHDll				=(TInt)0x80000000,// Emulator host file type: 1=DLL, 0=EXE
	ECodeSegAttExpVer			=0x40000000,	  // Filename is explicitly versioned
	ECodeSegAttNmdExpData		=0x20000000,	  // Named symbol export data in code seg
	ECodeSegAttSMPSafe			=0x10000000,	  // code seg and its static dependencies are SMP safe
	ECodeSegAttAddrNotUnique	=0x08000000,	  // run address not globally unique (may overlap other codesegs)
	};

// forward declarations from file server
class RFile;
class RFs;

/**
A Handle used to identify a file on storage media.
@internalTechnology
*/
class RFileClamp
	{
public:
	inline RFileClamp()
		{
		iCookie[0] = 0;
		iCookie[1] = 0;
		}
	IMPORT_C TInt Clamp(RFile& aFile);
	IMPORT_C TInt Close(RFs& aFs);

public:
	TInt64 iCookie[2];
	};


class TCodeSegCreateInfo
	{
public:
	TBuf8<KMaxFileName> iFileName;		// not including {MMMMmmmm} version info
	TUidType iUids;				// uid1 indicates EXE or DLL
	TUint32 iAttr;
	TInt iCodeSize;
	TInt iTextSize;
	TInt iDataSize;
	TInt iBssSize;
	TInt iTotalDataSize;
	TUint32 iEntryPtVeneer;		// address of first instruction to be called
	TUint32 iFileEntryPoint;	// address of entry point within this code segment
	TInt iDepCount;
	TUint32 iExportDir;
	TInt iExportDirCount;
	TUint32 iCodeLoadAddress;	// 0 for RAM loaded code, else pointer to TRomImageHeader
	TUint32 iCodeRunAddress;
	TUint32 iDataLoadAddress;
	TUint32 iDataRunAddress;
	TUint32 iExceptionDescriptor;
	TInt iRootNameOffset;
	TInt iRootNameLength;
	TInt iExtOffset;
	TUint32 iModuleVersion;
	SSecurityInfo iS;
	TAny* iHandle;				// pointer to kernel-side DCodeSeg object
	TInt iClientProcessHandle;	// handle to client process for user DLL loads
	/** Code relocation information stored on loader heap. */
	TUint32* iCodeRelocTable;
	/** Size of code relocation table in bytes. */
	TInt iCodeRelocTableSize;
	/** Import fixup information stored on loader heap. */
	TUint32* iImportFixupTable;
	/** Size of import fixup table in bytes. */
	TInt iImportFixupTableSize;
	/** Offset to apply to each code address in the image when it is fixed up. */
	TUint32 iCodeDelta;
	/** Offset to apply to each data address in the image when it is fixed up. */
	TUint32 iDataDelta;
	/**
		Whether the code is paged.  If this is set, then
		TCodeSegCreateInfo::iCodeRelocTable[Size] and
		TCodeSegCreateInfo::iImportFixupTable[Size] contain fixup information
		which the kernel uses to fix up each page.
		(They may be null if the binary has no imports or no code section.)
	 */
	TBool iUseCodePaging;
	/** The UID of the compression scheme in use. */
	TUint32 iCompressionType;
	/**
		Start of compressed pages within the file.  The kernel uses
		this to load compressed pages from byte-pair files when demand
		paging.
	 */
	TInt32* iCodePageOffsets;
	/** Where (possibly compressed) object code starts in iFile. */
	TInt iCodeStartInFile;
	/** Length of (possibly compressed) object code in iFile. */
	TInt iCodeLengthInFile;
	/** Information about block map entries in iCodeBlockMapEntries. */
	SBlockMapInfoBase iCodeBlockMapCommon;
	/** Where object code is located on the media. */
	TBlockMapEntryBase* iCodeBlockMapEntries;
	/** Size of block map entry array in bytes. */
	TInt iCodeBlockMapEntriesSize;
	/**
		File clamp cookie, used to delete the file when the
		codeseg is destroyed.
	 */
	RFileClamp iFileClamp;
public:
	IMPORT_C TPtrC8 RootName() const;
	};

//
// Information required to create a new process
//
class TProcessCreateInfo : public TCodeSegCreateInfo
	{
public:
	enum TDebugAttributes	// must be the same as RLibrary::TInfoV2::TDebugAttributes
		{
		EDebugAllowed = 1<<0, ///< Flags set if executable may be debugged.
		ETraceAllowed = 1<<1 ///< Flags set if executable may be traced.
		};
	/**
	The flags for process's creation.  Will be set by the loader from the images
	header flags ready for the kernel to use.
	*/
	enum TProcessCreateFlags
		{
		EDataPagingUnspecified	= 0x00000000,	///< Use the global data paging default.
		EDataPaged				= 0x00000001,	///< Page the process's data by default.
		EDataUnpaged			= 0x00000002,	///< Don't page the process's data by default.
		EDataPagingMask			= 0x00000003,	///< Bit mask ofr data paging flags.
		};

	/** Default constructor that ensures flags are clear. */
	TProcessCreateInfo() : iFlags(0) {};

	TInt iHeapSizeMin;
	TInt iHeapSizeMax;
	TInt iStackSize;
	TInt iClientHandle;			// handle to loader's client
	TInt iProcessHandle;		// handle to new DProcess
	TInt iFinalHandle;			// handle from loader client to new process
	TOwnerType iOwnerType;
	TProcessPriority iPriority;
	TUint iSecurityZone;
	TUint iDebugAttributes;	///< Set with values from TDebugAttributes.
	TRequestStatus* iDestructStat;
	TUint iFlags;	///< Flags for process creation, should set from TProcessCreateFlags.
	};

const TUint KSecurityZoneUnique = 0u;
const TUint KSecurityZoneLegacyCode = ~0u;

//
// Information required to attach a code segment to a process
// in the form of a library.
//
class TLibraryCreateInfo
	{
public:
	TAny* iCodeSegHandle;		// pointer to kernel-side DCodeSeg object
	TInt iClientHandle;			// handle to loader's client
	TInt iLibraryHandle;		// handle to new DLibrary
	TOwnerType iOwnerType;
	};

//
// Information required to find an existing code segment
//
class TFindCodeSeg
	{
public:
	TUidType iUids;				// required UIDs
	const TAny* iRomImgHdr;		// ROM image header if ROM code required, NULL otherwise
	TUint32 iAttrMask;			// mask for attributes
	TUint32 iAttrVal;			// required value for masked attributes
	TInt iProcess;				// handle to process in which code is required to operate
								// not used if kernel only specified
	SSecurityInfo iS;			// required capabilities/SID
	TUint32 iModuleVersion;		// required version
	TBuf8<KMaxLibraryName> iName;	// name to look for - zero length means any
	};

//
// Information required to by the reaper from the codeseg.
//
struct TCodeSegLoaderCookie
	{
	RFileClamp iFileClamp;
	TInt64 iStartAddress;
	TInt iDriveNumber;
	};

//
// Loader magic executive functions
//
class E32Loader
	{
public:
	// used by loader only
	IMPORT_C static TInt CodeSegCreate(TCodeSegCreateInfo& aInfo);
	IMPORT_C static TInt CodeSegLoaded(TCodeSegCreateInfo& aInfo);
	IMPORT_C static TInt LibraryCreate(TLibraryCreateInfo& aInfo);
	IMPORT_C static TInt CodeSegOpen(TAny* aHandle, TInt aClientProcessHandle);
	IMPORT_C static void CodeSegClose(TAny* aHandle);
	IMPORT_C static void CodeSegNext(TAny*& aHandle, const TFindCodeSeg& aFind);
	IMPORT_C static void CodeSegInfo(TAny* aHandle, TCodeSegCreateInfo& aInfo);
	IMPORT_C static TInt CodeSegAddDependency(TAny* aImporter, TAny* aExporter);
	IMPORT_C static void CodeSegDeferDeletes();
	IMPORT_C static void CodeSegEndDeferDeletes();
	IMPORT_C static TInt ProcessCreate(TProcessCreateInfo& aInfo, const TDesC8* aCommandLine);
	IMPORT_C static TInt ProcessLoaded(TProcessCreateInfo& aInfo);
	IMPORT_C static TInt CheckClientState(TInt aClientHandle);
	IMPORT_C static TInt DeviceLoad(TAny* aHandle, TInt aType);
	IMPORT_C static TAny* ThreadProcessCodeSeg(TInt aHandle);
	IMPORT_C static void ReadExportDir(TAny* aHandle, TUint32* aDest);
	IMPORT_C static TInt LocaleExports(TAny* aHandle, TLibraryFunction* aExportsList);

#ifdef __MARM__
	IMPORT_C static void GetV7StubAddresses(TLinAddr& aExe, TLinAddr& aDll);
	static TInt V7ExeEntryStub();
	static TInt V7DllEntryStub(TInt aReason);
#endif

	IMPORT_C static TUint32 PagingPolicy();
	
	IMPORT_C static TInt NotifyIfCodeSegDestroyed(TRequestStatus& aStatus);
	IMPORT_C static TInt GetDestroyedCodeSegInfo(TCodeSegLoaderCookie& aCookie);

public:
	// used by client side
	static TInt WaitDllLock();
	static TInt ReleaseDllLock();
	static TInt LibraryAttach(TInt aHandle, TInt& aNumEps, TLinAddr* aEpList);
	static TInt LibraryAttached(TInt aHandle);
	static TInt StaticCallList(TInt& aNumEps, TLinAddr* aEpList);
	static void StaticCallsDone();
	static TInt LibraryDetach(TInt& aNumEps, TLinAddr* aEpList);
	static TInt LibraryDetached();
	};

typedef TInt (*TSupervisorFunction)(TAny*);

// Relocation types
/**
@internalTechnology
@released
*/
const TUint16 KReservedRelocType        = (TUint16)0x0000;
/**
@internalTechnology
@released
*/
const TUint16 KTextRelocType            = (TUint16)0x1000;
/**
@internalTechnology
@released
*/
const TUint16 KDataRelocType            = (TUint16)0x2000;
/**
@internalTechnology
@released
*/
const TUint16 KInferredRelocType        = (TUint16)0x3000;

// Compression types

/**
@internalTechnology
@released
*/
const TUint KFormatNotCompressed=0;
/**
@internalTechnology
@released
*/
const TUint KUidCompressionDeflate=0x101F7AFC;


const TUint KUidCompressionBytePair=0x102822AA;


#endif // __E32LDR_PRIVATE_H__

