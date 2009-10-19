// Copyright (c) 1997-2009 Nokia Corporation and/or its subsidiary(-ies).
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
// Contains all of the delay filesystem classes.
//

//! @file f32test\concur\cfafsdly.h

#ifndef __CFAFSDLY_H__
#define __CFAFSDLY_H__

#include <f32file.h>
#include "common.h"
#include <f32fsys.h>
#include <f32ver.h>
#include <f32dbg.h>
#include <e32svr.h>

const TInt KMaxFiles   = 10;    ///< Maximum number of files in the filesystem
const TInt KMaxFileLen = 8192;  ///< Maximum length of an individual file

class TTestFile
/// Describe a file, including the data.
	{
public:
	TTestFile();
	void Entry(TEntry& aEntry) const;
public:
	TFileName iName;				///< name of the file.
	TUint8    iData[KMaxFileLen];	///< data in the file (fixed maximum length).
	TInt      iSize;				///< current size of the file (length of data).
	TTime     iTime;				///< timestamp of last modification.
	};

class TTestDir
/// Holds the directory for the filesystem (there is only one!), including all
/// of the files with their data.  Only practical for a simple test system.
	{
public:
	TTestDir();
	TTestFile* Create(const TDesC& aName);
	TTestFile* Find(const TDesC& aName);
	const TTestFile* Find(const TDesC& aName) const;
	void       Delete(const TDesC& aName);
	TTestFile* Entry(TInt aIndex);
private:
	TTestFile  iFile[KMaxFiles];	///< All of the file data.
	};

class CTestMountCB : public CMountCB
/// Data and functions associated with a mounted filesystem.  It is allocated
/// by the file server when the filesystem is mounted and destroyed when it is
/// unmounted (hence losing all of the data in files in this case).
	{
public:
	CTestMountCB();
	~CTestMountCB();
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
	void SetEntryL(const TDesC& aName,const TTime& aTime,TUint aSetAttMask,TUint learAttMask);
	void FileOpenL(const TDesC& aName,TUint aMode,TFileOpen anOpen,CFileCB* aFile);
	void DirOpenL(const TDesC& aName,CDirCB* aDir);
	void RawReadL(TInt64 aPos,TInt aLength,const TAny* aDes,TInt anOffset,const RMessagePtr2& aMessage) const;
	void RawWriteL(TInt64 aPos,TInt aLength,const TAny* aDes,TInt anOffset,const RMessagePtr2& aMessage);
	void ReadUidL(const TDesC& aName,TEntry& anEntry) const;
	void GetShortNameL(const TDesC& aLongName,TDes& aShortName);
	void GetLongNameL(const TDesC& aShortName,TDes& aLongName);
	void IsFileInRom(const TDesC& aFileName,TUint8*& aFileStart);
	void ReadSectionL(const TDesC& aName,TInt aPos,TAny* aTrg,TInt aLength,const RMessagePtr2& aMessage);
private:
        TTestDir iDir;	///< The directory information, including all files and their data.
	};



class CTestFileCB : public CFileCB
/// Data about a specific open file.
	{
public:
	CTestFileCB();
	~CTestFileCB();
	void RenameL(const TDesC& aNewName);
	void ReadL(TInt aPos,TInt& aLength,const TAny* aDes,const RMessagePtr2& aMessage);
	void WriteL(TInt aPos,TInt& aLength,const TAny* aDes,const RMessagePtr2& aMessage);
	TInt Address(TInt& aPos) const;
	void SetSizeL(TInt aSize);
	void SetEntryL(const TTime& aTime,TUint aSetAttMask,TUint aClearAttMask);
	void FlushDataL();
	void FlushAllL();
	void CheckPos(TInt aPos);

public:
	TUint8	iPadding[128];	// in case we're passed a CFATFileCB
	TTestFile* iFile;	///< Points to the file structure of the open file.
	TUint8*    iData;	///< Points to the data area of the file.
	};

class CTestDirCB : public CDirCB
/// Data for accessing a directory for search etc.
	{
public:
	CTestDirCB();
	~CTestDirCB();
	void ReadL(TEntry& anEntry);
	
public:
	TFileName iName;	///< Name of the current file.
	TTestDir* iDir;		///< Pointer to the directory data.
	TInt      iIndex;	///< Current position in the directory.
	};

class CTestFormatCB : public CFormatCB
/// Functions for formatting the filesystem.  Not used.
	{
public:
	CTestFormatCB();
	~CTestFormatCB();
	void DoFormatStepL();
	};

class CTestFileSystem : public CFileSystem
/// Describes the filesysem, and creates a new one when it is mounted.
	{
public:
	CTestFileSystem();
	~CTestFileSystem();
	TInt Install();
	TInt DefaultPath(TDes& aPath) const;
	void DriveInfo(TDriveInfo& anInfo, TInt aDriveNumber) const;
	TBusLocalDrive& DriveNumberToLocalDrive(TInt aDriveNumber);
	TInt GetInterface(TInt aInterfaceId, TAny*& aInterface,TAny* aInput);
private:
	CMountCB*  NewMountL() const;
	CFileCB*   NewFileL() const;
	CDirCB*    NewDirL() const;
	CFormatCB* NewFormatL() const;
public:
	static CFileSystem* NewL();
	};

#endif
