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


#include <e32std.h>
#include <f32fsys.h>
#include <f32ver.h>
#include <e32rom.h>
#include <rofs.h>
#include <f32dbg.h>

IMPORT_C TUint32 DebugRegister();
#if defined(_DEBUG) || defined(_DEBUG_RELEASE)
#define __LOCK_SR_STD__
#define __PRINT(t)			{if (DebugRegister()&KROFS) RDebug::Print(t);}
#define __PRINT1(t,d)		{if (DebugRegister()&KROFS) RDebug::Print(t,d);}
#define __PRINT2(t,d,d1)	{if (DebugRegister()&KROFS) RDebug::Print( t, d , d1);}
#else
#define __PRINT(t)
#define __PRINT1(t,d)
#define __PRINT2(t,d,d1)
#endif

//

#define _USE_TRUE_LRU_CACHE

const TUint KFileHidden = 0xFFFFFFFF;

class CRofs : public CFileSystem
	{
public:
	enum TPanic
		{
		EPanicEntryNotDir,
		EPanicNullSubDir,
		EPanicNullFileBlock,
		EPanicNullFileBlock2,
		EPanicEntryBeforeDirectory,
		EPanicEntryAfterDirectory,
		EPanicBadMatchName,
		EPanicRemountNotSupported,
		EPanicGetFileInfo,
		EPanicFileTooBig,
		EPanicReadUid,
		EPanicDirCacheNull,
		EPanicDriveInfo
		};
	static void Panic( TPanic aPanic );

public:
	CRofs();
	~CRofs();
	
	static CRofs* New();

	TInt Install();
	CMountCB* NewMountL() const;
	CFileCB* NewFileL() const;
	CDirCB* NewDirL() const;
	CFormatCB* NewFormatL() const;

	void DriveInfo(TDriveInfo& anInfo,TInt aDriveNumber) const;
	TInt DefaultPath(TDes& aPath) const;
	TInt TotalSupportedDrives() const;
	TBool IsExtensionSupported() const{	return(ETrue);};
private:
	TInt	iTotalSupportedDrives;
	};


class CRofsMountCB;
class CDirectoryCache : public CBase
	{
	public:
		CDirectoryCache( CRofsMountCB& aMount, CProxyDrive& aLocalDrive, const TRofsHeader& aHeader );
		~CDirectoryCache();
	
		void ConstructL();

		void FindLeafDirL(const TDesC& aName, const TRofsDir*& aDir) const;
		void FindFileEntryL(const TDesC& aName, const TRofsEntry*& aEntry) const;
		void FindDirectoryEntryL(const TDesC& aName, const TRofsDir*& aDir) const;
		void GetNextMatchingL(const TDesC& aName, TUint aAtt, const TRofsDir*& aDir, const TRofsEntry*& aEntry, TInt aError, TBool bUseBinarySearch) const;
		void FindGeneralEntryL(const TDesC& aName, TUint aAtt, const TRofsDir*& aDir, const TRofsEntry*& aEntry ) const;

		static inline const TText* NameAddress( const TRofsEntry* aEntry );
		TUint8 GetMountId(void);

	private:
		CDirectoryCache();
		static inline TUint32 AlignUp( TUint32 aValue );
		inline const TRofsDir* RootDirectory() const;
		inline const TRofsDir* RofsDirFromMediaOffset( TUint aMediaOffset ) const;
		inline const TRofsDir* RofsDirFromSubDirEntry( const TRofsEntry* aEntry ) const;
		static inline const TRofsEntry* FirstSubDirEntryFromDir( const TRofsDir* aDir );
		inline const TRofsEntry* FirstFileEntryFromDir( const TRofsDir* aDir ) const;
		inline const TAny* EndOfFileBlockPlusOne( const TRofsDir* aDir ) const;
		inline const TAny* EndOfDirPlusOne( const TRofsDir* aDir ) const;
		static inline const TRofsEntry* NextEntry( const TRofsEntry* aEntry );

		TInt DoFindSubDir(const TDesC& aName, TUint aAtt, const TRofsDir* aDir, const TRofsEntry*& aEntry) const;
		TInt DoFindFile(const TDesC& aName, TUint aAtt, const TRofsDir* aDir, const TRofsEntry*& aEntry) const;

		TInt DoBinaryFindSubDir(const TDesC& aName, TUint aAtt, const TRofsDir* aDir, const TRofsEntry*& aEntry) const;
		TInt DoBinaryFindFile(const TDesC& aName, TUint aAtt, const TRofsDir* aDir, const TRofsEntry*& aEntry) const;

		TInt GetDirCount(const TRofsDir* aDir) const;
		TInt GetFileCount(const TRofsDir* aDir) const;
		TInt Compare(const TDesC& aLeft, const TDesC& aRight) const;
		TInt ExtractMangleInfo(const TDesC& searchName, TUint8 &MountId, TUint8 &Reserved) const;
	private:
		CRofsMountCB&	iMount;
		CProxyDrive&	iLocalDrive;
		const TUint		iTreeMediaOffset;
		const TUint		iTreeSize;
		const TUint		iFilesMediaOffset;
		const TUint		iFilesSize;
		HBufC8*	iTreeBuffer;
		HBufC8*	iFilesBuffer;
	};

class CRofsFileCB;
/**

*/
class CRofsLruCache;
class CRofsMountCB : public CLocDrvMountCB, public CMountCB::MFileAccessor
	{
public:
	CRofsMountCB();
	void MountL(TBool aForceMount);
	TInt ReMount();
	void Dismounted();
	void VolumeL(TVolumeInfo& aVolume) const;
	void SetVolumeL(TDes& aName);
	void MkDirL(const TDesC& aName);
	void RmDirL(const TDesC& aName);
	void DeleteL(const TDesC& aName);
	void RenameL(const TDesC& anOldName,const TDesC& anNewName);
	void ReplaceL(const TDesC& anOldName,const TDesC& anNewName);
	void EntryL(const TDesC& aName,TEntry& anEntry) const;
	void SetEntryL(const TDesC& aName,const TTime& aTime,TUint aMask,TUint aVal);
	void FileOpenL(const TDesC& aName,TUint aMode,TFileOpen anOpen,CFileCB* aFile);
	void DirOpenL(const TDesC& aName,CDirCB* aDir);
	void RawReadL(TInt64 aPos,TInt aLength,const TAny* aTrg,TInt anOffset,const RMessagePtr2& aMessage) const;
	void RawWriteL(TInt64 aPos,TInt aLength,const TAny* aSrc,TInt anOffset,const RMessagePtr2& aMessage);
	void GetShortNameL(const TDesC& aLongName,TDes& aShortName);
	void GetLongNameL(const TDesC& aShortName,TDes& aLongName);
	void ReadSectionL(const TDesC& aName,TInt aPos,TAny* aTrg,TInt aLength,const RMessagePtr2& aMessage);
	void ReadUidL( TUint aMediaOffset, TEntry& aEntry, TRofsEntry* aRofsEntry) const;
	inline CRofs& FileSystem() const;
	virtual TInt GetInterface(TInt aInterfaceId,TAny*& aInterface,TAny* aInput);
	virtual TInt GetFileUniqueId(const TDesC& aName, TInt64& aUniqueId);
	virtual TInt Spare3(TInt aVal, TAny* aPtr1, TAny* aPtr2);
	virtual TInt Spare2(TInt aVal, TAny* aPtr1, TAny* aPtr2);
	virtual TInt Spare1(TInt aVal, TAny* aPtr1, TAny* aPtr2);

#ifdef _USE_TRUE_LRU_CACHE
	void CacheReadL(TInt aPos, TInt aLength, const TAny* aDes, TInt anOffset, const RMessagePtr2& aMessage) const;
#endif
	TUint8 iMountId;

private:
	TInt CheckHeader() const;
	TInt CheckExtension();

private:
	TPckgBuf<TRofsHeader>	iHeader;
	CDirectoryCache*	iDirectoryCache;
	TInt64				iMediaSize;

#ifdef _USE_TRUE_LRU_CACHE
 	CRofsLruCache*		iDataCache;
#endif

	};

class CRofsFileCB : public CFileCB, public CFileCB::MBlockMapInterface, public CFileCB::MExtendedFileInterface
	{
public:
	CRofsFileCB();
	void RenameL(const TDesC& aNewName);
	void ReadL(TInt aPos,TInt& aLength,const TAny* aDes,const RMessagePtr2& aMessage);
	void WriteL(TInt aPos,TInt& aLength,const TAny* aDes,const RMessagePtr2& aMessage);
	void SetSizeL(TInt aSize);
	void SetEntryL(const TTime& aTime,TUint aMask,TUint aVal);
	void FlushDataL();
	void FlushAllL();
	inline void SetMediaBase(const TUint aBase);
	inline void SetAttExtra( TUint8 aAttExtra );
	TInt GetInterface(TInt aInterfaceId,TAny*& aInterface,TAny* aInput);

	// from CFileCB::MExtendedFileInterface
	virtual void ReadL(TInt64 aPos,TInt& aLength,TDes8* aDes,const RMessagePtr2& aMessage, TInt aOffset);
	virtual void WriteL(TInt64 aPos,TInt& aLength,const TDesC8* aDes,const RMessagePtr2& aMessage, TInt aOffset);
	virtual void SetSizeL(TInt64 aSize);

private:
	inline CRofsMountCB& RofsMount();
	TInt BlockMap(SBlockMapInfo& aInfo, TInt64& aStartPos, TInt64 aEndPos);

private:
	TUint iMediaBase;
	TUint8	iAttExtra;
	};

class CRofsDirCB : public CDirCB
	{
public:
	CRofsDirCB();
	~CRofsDirCB();
	void ReadL(TEntry& anEntry);
	void SetDir(const TRofsDir* aDir, const TDesC& aMatch, TInt64& aTimeStamp );
	inline void SetCache( CDirectoryCache* aCache );
private:
	TBool MatchUid();
private:
	CDirectoryCache* iCache;
	TEntry iEntry;
	TTime	iTimeStamp;
	const TRofsDir* iDir;
	const TRofsEntry* iNext;
	HBufC* iMatch;
	};

#ifdef _USE_TRUE_LRU_CACHE
const TInt KSizeOfCacheInPages = 5;	// 5K Cache
const TInt KSizeOfSegment = 1024;	//Two pages
const TInt KPageSize = 512;

class TCacheSegment
	{
public:
	TCacheSegment();
	void Set(TInt aPos);
	TUint8* Data() {return(((TUint8*)this)+sizeof(TCacheSegment));}
public:
	TInt iPos;	
	TDblQueLink iLink;
	};

//
//Data cache for Rofs, (read only cache)
//
class CRofsLruCache: public CBase
	{
public:
	~CRofsLruCache();
	static CRofsLruCache* New(TInt aSegmentSize, CRofsMountCB* aMount, TInt64 aMediaSize);
	TUint8* Find(TInt aPos , TInt aLength);
	TUint8* ReadL(TInt aPos , TInt aLength);
protected:
	CRofsLruCache(CRofsMountCB* aMount, TInt64 aMediaSize);
private:
	TDblQue<TCacheSegment> iQue;
	CRofsMountCB* iMount;
	TInt64 iMediaSize;
	};
#endif


#include "sr_rofs.inl"
