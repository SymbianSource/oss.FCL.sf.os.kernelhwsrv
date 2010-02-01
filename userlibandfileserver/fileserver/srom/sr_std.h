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
// f32\srom\sr_std.h
// 
//

#if defined(_UNICODE)
#if !defined(UNICODE)
#define UNICODE
#endif
#endif

#include "common.h"
#include <f32fsys.h>
#include <f32ver.h>
#include <e32rom.h>
#include <e32svr.h>

#if defined(__EPOC32__)
// #define __PRINT_DEBUG_INFO_SR_STD__ 1
#endif

#if defined(__PRINT_DEBUG_INFO_SR_STD__) && !defined(__LOCK_SR_STD__)
#define __LOCK_SR_STD__
#define __PRINT(t) RDebug::t;
#else
#define __PRINT(t)
#endif

//
enum TFault
	{
	ERomReMountNotSupported,
	ERomGetFileInfo,
	ERomFileTooBig,
	ERomCreateFileMapping,
	ERomCreateMappedView,
	ERomInvalidArgument,
	ERomFsCorrupt
	};
//
NONSHARABLE_CLASS(CRom) : public CFileSystem
	{
public:
	CRom();
	~CRom();
	TInt Install();
	CMountCB* NewMountL() const;
	CFileCB* NewFileL() const;
	CDirCB* NewDirL() const;
	CFormatCB* NewFormatL() const;
	void DriveInfo(TDriveInfo& anInfo,TInt aDriveNumber) const;
	const TRomHeader& RomHeader() const {return(*iRomHeaderAddress);}
	TLinAddr RootDirectory() const {return UserSvr::RomRootDirectoryAddress();}
private:
	static const TRomHeader* iRomHeaderAddress;
	};
//
NONSHARABLE_CLASS(CRomMountCB) : public CMountCB, public CMountCB::MFileAccessor
	{
public:
	CRomMountCB(const CRom* aRom);
	void MountL(TBool aForceMount);
	TInt ReMount();
	void Dismounted();
	static TInt Compare(const TDesC& aLeft, const TDesC& aRight);
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
	void FindLeafDirL(const TDesC& aName,TLinAddr& aDir) const;
	void FindL(const TDesC& aName,TUint anAtt,TLinAddr aDir,TLinAddr& anEntry,TInt anError) const;
	void FindBinaryL(const TDesC& aName,TUint aAtt,TBool aAttKnown,TLinAddr aDir,TLinAddr& aEntry,TInt aError) const;
	void FindEntryL(const TDesC& aName,TUint anAtt,TBool aAttKnown,TLinAddr& aDir,TLinAddr& anEntry) const;
	const TRomHeader& RomHeader() const {return (iRom->RomHeader());}
	TLinAddr RomRootDirectory() const {return iRom->RootDirectory(); }
	void RawReadL(TInt64 aPos,TInt aLength,const TAny* aDes,TInt anOffset,const RMessagePtr2& aMessage) const;
	void RawWriteL(TInt64 aPos,TInt aLength,const TAny* aDes,TInt anOffset,const RMessagePtr2& aMessage);
	void ReadUidL(TLinAddr anAddr,TEntry& anEntry) const;
	void GetShortNameL(const TDesC& aLongName,TDes& aShortName);
	void GetLongNameL(const TDesC& aShortName,TDes& aLongName);
	void IsFileInRom(const TDesC& aFileName,TUint8*& aFileStart);
	void ReadSectionL(const TDesC& aName,TInt aPos,TAny* aTrg,TInt aLength,const RMessagePtr2& aMessage);
	virtual TInt GetInterface(TInt aInterfaceId,TAny*& aInterface,TAny* aInput);
	virtual TInt GetFileUniqueId(const TDesC& aName, TInt64& aUniqueId);
	virtual TInt Spare3(TInt aVal, TAny* aPtr1, TAny* aPtr2);
	virtual TInt Spare2(TInt aVal, TAny* aPtr1, TAny* aPtr2);
	virtual TInt Spare1(TInt aVal, TAny* aPtr1, TAny* aPtr2);

private:
	const CRom* iRom;
	};
//
NONSHARABLE_CLASS(CRomFileCB) : public CFileCB
	{
public:
	CRomFileCB(const CRom* aRom);
	void RenameL(const TDesC& aNewName);
	void ReadL(TInt aPos,TInt& aLength,const TAny* aTrg,const RMessagePtr2& aMessage);
	void WriteL(TInt aPos,TInt& aLength,const TAny* aSrc,const RMessagePtr2& aMessage);
	void SetSizeL(TInt aSize);
	void SetEntryL(const TTime& aTime,TUint aMask,TUint aVal);
	void FlushDataL();
	void FlushAllL();
	TInt Address(TInt& aPos) const;
	const TRomHeader& RomHeader() const {return(iRom->RomHeader());}
	TLinAddr RomRootDirectory() const {return iRom->RootDirectory(); }			
	void SetBase(const TUint8* aBase) {iBase=aBase;}
private:
	const CRom* iRom;
	const TUint8* iBase;
	};
//
NONSHARABLE_CLASS(CRomDirCB) : public CDirCB
	{
public:
	CRomDirCB(const CRom* aRom);
	~CRomDirCB();
	void ReadL(TEntry& anEntry);
	const TRomHeader& RomHeader() const {return(iRom->RomHeader());}
	TLinAddr RomRootDirectory() const {return iRom->RootDirectory(); }
	void SetDir(TLinAddr aDir,TLinAddr anEntry,const TDesC& aMatch);
private:
	TBool MatchUid();
private:
	const CRom* iRom;
	TEntry iEntry;
	TLinAddr iDir;
	TLinAddr iNext;
	HBufC* iMatch;
	};
//
GLREF_C TInt InstallFileSystem(CFileSystem* aSys,RLibrary aLib);

