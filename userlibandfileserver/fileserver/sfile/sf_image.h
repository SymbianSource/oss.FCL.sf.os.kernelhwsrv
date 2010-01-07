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
// f32\sfile\sf_image.h
// 
//

#ifndef __SF_IMAGE_H__
#define __SF_IMAGE_H__
#include <f32file.h>
#include <f32image.h>
#include <e32ldr.h>
#include <e32ldr_private.h>
#include <e32rom.h>
#include "sf_deflate.h"

_LIT(KLitLoader,"LOADER");
//__DATA_CAGING__
const TUint32	KCapabilityOffSet	=	0x18;
const TUint32	KCapabilityLength	=	0x04;
const TUint32	KCapabilityMask		=	0x00000001;

enum TPanic
	{
	ELoadLibraryWithoutDllLock=0,
	};

class E32Image;
class RImageArray : public RPointerArray<E32Image>
	{
public:
	RImageArray();
	TInt Add(E32Image* aImage);
	void Find(const TDesC8& aRootName, TInt& aFirst, TInt& aLast) const;
	E32Image* Find(const TRomImageHeader* aRomImgHdr) const;
	};

class TFileNameInfo
	{
public:
	enum	{
			EIncludeDrive=1,
			EIncludePath=2,
			EIncludeBase=4,
			EIncludeVer=8,
			EForceVer=16,
			EIncludeUid=32,
			EForceUid=64,
			EIncludeExt=128,
			EIncludeDrivePath=EIncludeDrive|EIncludePath,
			EIncludeBaseExt=EIncludeBase|EIncludeExt,
			EIncludeDrivePathBaseExt=EIncludeDrive|EIncludePath|EIncludeBase|EIncludeExt,
			};
	enum	{
			EAllowUid=1,
			EAllowPlaceholder=2,
			EAllowDecimalVersion=4,
			};
public:
	TFileNameInfo();
	TInt Set(const TDesC8& aFileName, TUint aFlags);
	void Dump() const;
public:
	inline TInt DriveLen() const {return iPathPos;}
	inline TInt PathLen() const {return iBasePos-iPathPos;}
	inline TInt BaseLen() const {return iVerPos-iBasePos;}
	inline TInt VerLen() const {return iUidPos-iVerPos;}
	inline TInt UidLen() const {return iExtPos-iUidPos;}
	inline TInt ExtLen() const {return iLen-iExtPos;}
	inline TPtrC8 Drive() const {return TPtrC8(iName, iPathPos);}
	inline TPtrC8 Path() const {return TPtrC8(iName+iPathPos, iBasePos-iPathPos);}
	inline TPtrC8 DriveAndPath() const {return TPtrC8(iName, iBasePos);}
	inline TPtrC8 Base() const {return TPtrC8(iName+iBasePos, iVerPos-iBasePos);}
	inline TPtrC8 VerStr() const {return TPtrC8(iName+iVerPos, iUidPos-iVerPos);}
	inline TPtrC8 UidStr() const {return TPtrC8(iName+iUidPos, iExtPos-iUidPos);}
	inline TPtrC8 Ext() const {return TPtrC8(iName+iExtPos, iLen-iExtPos);}
	inline TUint32 Version() const {return iVersion;}
	inline TUint32 Uid() const {return iUid;}
	void GetName(TDes8& aName, TUint aFlags) const;
public:
	const TText8* iName;
	TInt iPathPos;
	TInt iBasePos;
	TInt iVerPos;
	TInt iUidPos;
	TInt iExtPos;
	TInt iLen;
	TUint32 iVersion;
	TUint32 iUid;
	};

class RLoaderMsg;

// Information used to search for an image file
class RLdrReq : public TLdrInfo
	{
public:
	RLdrReq();
	void Close();
	void Panic(TInt aPanic);
	TInt CheckForSubstDriveInName();
	TInt CheckForSubstDrivesInPath();
	TInt AddFileExtension(const TDesC8& aExt);
	void Dump(const char* aTitle) const;
	TInt CheckSecInfo(const SSecurityInfo& aCandidate) const;
public:
	HBufC8* iFileName;
	HBufC8* iCmd;
	HBufC8* iPath;
	const RLoaderMsg* iMsg;
	SCapabilitySet iPlatSecCaps;
	RThread iClientThread;
	RProcess iClientProcess;
	TFileNameInfo iFileNameInfo;
	E32Image* iImporter;
	};

class RLoaderFile : public RFile
	{
public:
	inline TBool IsOpen()
		{ return Session().Handle() && SubSessionHandle(); }
	};

class TImageInfo
	{
public:
	TUint32 iUid[KMaxCheckedUid];
	TUint32 iModuleVersion;
	SSecurityInfo iS;
	TUint32 iAttr;
	TUint16 iExportDirCount;
	TUint8 iExportDescType;
	TUint8 iNameLength;
	enum TCacheStatusFlags
		{
		EHashChecked = 1,
		};
	TUint8 iCacheStatus;
	// 8-bit name follows (store base+ext only, not version)
	// export description follows name
	};

// Information returned by a search for an image file
class TFileCacheRecord;
class RImageInfo : public TImageInfo
	{
public:
	RImageInfo();
	RImageInfo& operator=(const TFileCacheRecord& aRecord);
	void Close();
	void Accept(RImageInfo& aInfo);
	inline TBool FileOpened() { return ((RLoaderFile*)&iFile)->IsOpen(); }
public:
	RFile iFile;
	E32ImageHeader* iHeader;				// header if available
	TUint8* iFileData;						// file data if it's been loaded
	TUint32 iFileSize;						// size of loaded data
	const TRomImageHeader* iRomImageHeader;	// pointer to ROM image header for XIP
	TUint16 iExportDescSize;
	const TUint8* iExportDesc;				// points into cache record so only valid during a single directory search
	TUint8 iNeedHashCheck;					// true if hash check was skipped and must be done at load time
	};

// Image finder - looks at candidates for a load and remembers the best one
class RImageFinder
	{
public:
	RImageFinder();
	TInt Set(const RLdrReq& aReq);
	void Close();
	TInt Search();
	TInt Search(const TDesC8* aPath, TInt aDrive);
	TInt SearchSingleDir();
	TInt SearchExisting(const RImageArray& aArray);
	TInt Try(RImageInfo& aInfo, const TDesC8& aRootName, const TDesC8& aDriveAndPath);
	void RecordCorruptFile();
	void SetName(const TDesC8& aRootName, const TDesC8& aDriveAndPath);
	void Dump(const char* aTitle, TInt aR);
	void CompareHashL(RImageInfo& aInfo, const TDesC8& aDriveAndPath);
public:
	TInt iNameMatches;				// number of files for which name matches
	TInt iUidFail;					// number of files for which UIDs are incompatible
	TInt iCapFail;					// number of files for which capabilities/SID are incompatible
	TInt iMajorVersionFail;			// number of files with lower major version than requested
	TInt iImportFail;				// number of files which failed import check
	TUint32 iCurrentVersion;		// version of current best match
	TPtrC8 iCurrentPath;			// current search directory
	TUint8 iCurrentDrive;			// current search drive
	TUint8 iFindExact;
	TUint8 iNewValid;				// a valid new image has been found
	const RLdrReq* iReq;
	E32Image* iExisting;			// pointer to existing image if that is currently the best
	RImageInfo iNew;				// new image info if that is currently the best
	TBuf8<KMaxFileName> iNewFileName;	// full path name for new image file
	TBuf8<KMaxKernelName> iRootName;
	};

extern RFs gTheLoaderFs;
extern TAny* gExeCodeSeg;
extern TUint32 gExeAttr;
extern TAny* gKernelCodeSeg;
extern TUint32 gKernelAttr;
extern TBool gExecutesInSupervisorMode;

TInt GetModuleInfo(RLdrReq& aReq);
TInt GetInfoFromHeader(const RLoaderMsg& aMsg);
TInt CheckSystemBin(const TDesC& aThePath);
TInt LoadProcess(RLdrReq& aReq);
TInt LoadLibrary(RLdrReq& aReq);
TInt LoadDeviceDriver(RLdrReq& aReq, TInt aDeviceType);
TInt LoadLocale(RLdrReq& aReq, TLibraryFunction* aExportsList);
TInt CheckUids(const TUidType& aUids, const TUidType& aRequestedUids);
TInt OpenFile8(RFile& aFile, const TDesC8& aName);
TInt CheckSubstDrive(TDes8& aDest, const TDesC8& aSrc);
TInt CompareVersions(TUint32 aL, TUint32 aR);
TInt CheckRequiredImports(E32Image* aImporter, E32Image* aExporter, TInt aAction);
TInt CheckRequiredImports(E32Image* aImporter, const RImageInfo& aExporter, TInt aAction);
TInt CheckedCollapse(TDes8& aDest, const TDesC16& aSrc);


enum TVersionCompareResult
	{
	EVersion_MinorBigger=0,
	EVersion_Exact,
	EVersion_MajorBigger,
	EVersion_MinorSmaller,
	EVersion_MajorSmaller,
	};

TInt DetailedCompareVersions(TUint32 aCandidate, TUint32 aRequest);
TInt DetailedCompareVersions(TUint32 aCandidate, TUint32 aRequest, TUint32 aCurrent, TBool aStrict);

enum TVersionCompareAction
	{
	EAction_Skip=0,
	EAction_CheckLastImport,
	EAction_CheckImports,
	EAction_Replace,
	};



class E32Image : public TProcessCreateInfo
	{
public:
	E32Image();
	~E32Image();
	void Reset();

	TInt Construct(RImageFinder& aFinder);
	void Construct(const TRomImageHeader& a);
	TInt OpenFile();

	TBool AlwaysLoaded();

	TInt LoadProcess(const RLdrReq& aReq);
	TInt LoadCodeSeg(const RLdrReq& aReq);
	TInt DoLoadCodeSeg(const RLdrReq& aReq, RImageFinder& aFinder);
	TInt DoLoadCodeSeg(const TRomImageHeader& aRomImgHdr);
	TInt CheckAlreadyLoaded();
	TInt CheckRomXIPAlreadyLoaded();
	TInt ProcessFileName();
	void GetRomFileName();
	static TBool TraverseDirs(const TRomDir& aDir, const TRomImageHeader* aHdr, TDes8& aName);

	TInt LoadToRam();
	TInt LoadFile();
	TInt LoadFileNoCompress();
	TInt Read(TUint aPos,TUint8* aDest,TUint aSize,TBool aSvPerms=EFalse);
	void LoadFileInflateL();
	void LoadFileBytePairUnpakL();
	TInt RelocateCode();
	TInt RelocateExports();
	TInt LoadAndRelocateData();
	TInt RelocateSection(E32RelocSection* aSection, TUint32 aLoadAddress);
	static TUint8* WordCopy(TAny* aDestination, const TAny* aSource, TInt aNumberOfBytes);
	static TUint8* MemCopy(TAny* aDestination, const TAny* aSource, TInt aNumberOfBytes);
	TInt ReadImportData();

	TInt ProcessImports();

	TInt LoadDlls(RImageArray& aDllArray);
	TInt GetCurrentImportList(const E32ImportBlock* aBlock);
	static TInt FixupDlls(RImageArray& aDllArray);
	TUint64* ExpandFixups(TInt aNumFixups);
	TInt FinaliseDlls(RImageArray& aDllArray);
	void CleanupDlls(RImageArray& aDllArray);

	TInt LastCurrentImport();
	void SortCurrentImportList();
	static TInt Order(const E32Image& aL, const E32Image& aR);
	TInt ReadExportDirLoad();

	// for demand paging...
	TInt ShouldBeCodePaged(TBool& aPage);
	TInt LoadCompressionData();
	TInt LoadCompressionDataNoCompress();
	void LoadCompressionDataBytePairUnpakL();
	TInt BuildCodeBlockMap();
	TInt AllocateRelocationData(E32RelocSection* aSection, TUint32 aAreaSize, TUint32 aLoadAddress, TUint32*& aProcessedBlock);
	TInt BuildImportFixupTable();
public:
	const TRomImageHeader* iRomImageHeader;
	E32ImageHeader* iHeader;
	E32RelocSection* iCodeRelocSection;		// address within iRestOfFileData 
	E32RelocSection* iDataRelocSection;		// address within iRestOfFileData 
	TUint32* iImportData;
	TUint8* iRestOfFileData;				// buffer holding all file data after code section
	TUint32 iRestOfFileSize;				// size of data at iRestOfFileData
	TUint32 iConversionOffset;
	RFile iFile;
	TUint8* iFileData;						// file data if it's been loaded
	TUint32 iFileSize;						// size of loaded data
	TUint32* iCopyOfExportDir;
	TUint32 iExportDirLoad;
	TUint32 iExportDirEntryDelta; // value to add to all values read from iExportDirLoad
	E32Image* iMain;
	TAny* iCloseCodeSeg;
	TInt iCurrentImportCount;
	TInt iNextImportPos;
	TUint32* iCurrentImportList;
	TUint8 iCurrentImportListSorted;
	TUint8 iIsDll;
	TUint8 iAlreadyLoaded;
	TUint8 iPadding2;
	TInt iFixupCount;				// number of fixups in iFixups
	TUint64* iFixups;				// array of fixups to apply to demand paged code {addr,value} pairs
	};


inline TBool CheckUid(TUint32 aUid, TUint32 aRequestedUid)
	{
	return (aRequestedUid==0 || aRequestedUid==aUid);
	}


#endif
