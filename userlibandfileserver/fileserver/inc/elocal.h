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
// f32\inc\elocal.h
// 
//

/**
 @file f32\inc\elocal.h
 @internalTechnology
*/

#if !defined(__ELOCAL_H__)
#define __ELOCAL_H__

#include "common.h"
#include <f32file.h>
#include <f32fsys.h>
#include <f32ver.h>

#include <e32wins.h>

#define WIN32_LEAN_AND_MEAN
#pragma warning( disable : 4201 ) // nonstandard extension used : nameless struct/union
#include <windows.h>
#pragma warning( default : 4201 ) // nonstandard extension used : nameless struct/union

/**
@internalTechnology
@released
*/
enum TPanic
	{
	EFileTimeToSystemTime,
	EFileClose,
	EFileCloseSetAttributes,
	EDirClose,
	EMapCouldNotConnect
	};


/**
@internalTechnology
@released
*/
class CLocalMountCB : public CMountCB, 
					  public CMountCB::MFileExtendedInterface
	{
public:
	CLocalMountCB();
	~CLocalMountCB();
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
	void SetEntryL(const TDesC& aName,const TTime& aTime,TUint aSetAttMask,TUint aClearAttMask);
	void FileOpenL(const TDesC& aName,TUint aMode,TFileOpen anOpen,CFileCB* aFile);
	void DirOpenL(const TDesC& aName,CDirCB* aDir);
	void RawReadL(TInt64 aPos,TInt aLength,const TAny* aDes,TInt anOffset,const RMessagePtr2& aMessage) const;
	void RawWriteL(TInt64 aPos,TInt aLength,const TAny* aDes,TInt anOffset,const RMessagePtr2& aMessage);
	void ReadUidL(const TDesC& aName,TEntry& anEntry) const;
	void GetShortNameL(const TDesC& aLongName,TDes& aShortName);
	void GetLongNameL(const TDesC& aShortName,TDes& aLongName);
	void IsFileInRom(const TDesC& aFileName,TUint8*& aFileStart);
	void ReadSectionL(const TDesC& aName,TInt aPos,TAny* aTrg,TInt aLength, const RMessagePtr2& aMessage);
	TInt LocalBufferSupport();
    TInt MountControl(TInt aLevel, TInt aOption, TAny* aParam);
    
    // interface extension implementation
	virtual void ReadSection64L(const TDesC& aName, TInt64 aPos, TAny* aTrg, TInt aLength, const RMessagePtr2& aMessage);


    inline TUint64 MaxFileSizeSupported() const {return iMaxFileSizeSupported;}

protected:
	virtual TInt GetInterface(TInt aInterfaceId,TAny*& aInterface,TAny* aInput);
private:
	TBool IsRomDrive() const;



private:

    TUint64 iMaxFileSizeSupported;  ///< Max. file size supported by HOST filesystem (4G-1 for FAT, 17,592,185,978,880 bytes for NTFS)

	};

/**
@internalTechnology
@released
*/
class CLocalFileCB : public CFileCB, public CFileCB::MExtendedFileInterface
	{
public:
	CLocalFileCB();
	~CLocalFileCB();
	void RenameL(const TDesC& aNewName);
	void ReadL(TInt aPos,TInt& aLength,const TAny* aDes,const RMessagePtr2& aMessage);
	void WriteL(TInt aPos,TInt& aLength,const TAny* aDes,const RMessagePtr2& aMessage);
	TInt Address(TInt& aPos) const;
	void SetSizeL(TInt aSize);
	void SetEntryL(const TTime& aTime,TUint aSetAttMask,TUint aClearAttMask);
	void FlushDataL();
	void FlushAllL();
	inline void SetHandle(HANDLE aHandle) {iWinHandle=aHandle;}
	void CheckPosL(TInt64 aPos);
	static TInt RomAddress(const TDesC& aName, HANDLE aFile, TUint8*& aAddr);
	
	// from CFileCB::MExtendedFileInterface
	virtual void ReadL(TInt64 aPos,TInt& aLength,TDes8* aDes,const RMessagePtr2& aMessage, TInt aOffset);
	virtual void WriteL(TInt64 aPos,TInt& aLength,const TDesC8* aDes,const RMessagePtr2& aMessage, TInt aOffset);
	virtual void SetSizeL(TInt64 aSize);

protected:
	// from CFileCB
	virtual TInt GetInterface(TInt aInterfaceId,TAny*& aInterface,TAny* aInput);

private:
	TBool IsRomDrive() const;
    inline CLocalMountCB& LocalMount() const {return((CLocalMountCB&)Mount());}

private:
	TInt64 iCurrentPos;
	TUint8* iFilePtr;
	HANDLE iWinHandle;
	};

/**
@internalTechnology
@released
*/
class CLocalDirCB : public CDirCB
	{
public:
	CLocalDirCB();
	~CLocalDirCB();
	void ReadL(TEntry& anEntry);
	inline void SetHandle(HANDLE aHandle) {iWinHandle=aHandle;}
	inline void SetFullName(const TDesC& aName) {iFullName.Set(aName,NULL,NULL);}
private:
	TBool MatchUid();
public:
	TEntry iEntry;
private:
	HANDLE iWinHandle;
	TParse iFullName;
	};

/**
@internalTechnology
@released
*/
class CLocalFormatCB : public CFormatCB
	{
public:
	CLocalFormatCB();
	~CLocalFormatCB();
public:
	virtual void DoFormatStepL();
	};

/**
@internalTechnology
@released
*/
class CLocal : public CFileSystem
	{
public:
	CLocal();
	TInt Install();
	CMountCB* NewMountL() const;
	CFileCB* NewFileL() const;
	CDirCB* NewDirL() const;
	CFormatCB* NewFormatL() const;
	TInt DefaultPath(TDes& aPath) const;
	void DriveInfo(TDriveInfo& anInfo,TInt aDriveNumber) const;
	};


#endif


