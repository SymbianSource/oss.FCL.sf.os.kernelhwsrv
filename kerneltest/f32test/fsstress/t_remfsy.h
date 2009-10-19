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
// f32test\fsstress\t_remfsy.h
// 
//

#include <e32twin.h>
#if !defined(__T_REMFSY_H__)
#define __T_REMFSY_H__
#if defined(_UNICODE)
#define _WTEXT(c) L ## c
#define _STRC LPCWSTR
#define _STR LPWSTR
#else
#define _WTEXT(c) c
#define _STRC LPCSTR
#define _STR LPSTR
#endif

#include <f32fsys.h>
#include <f32file.h>
#include <e32test.h>
#include <e32hal.h>
#include <e32math.h>
#if defined (__DEBUG__)|| defined(_DEBUG_RELEASE)
#include <f32dbg.h>
#endif


//
// Common constants used by both EFSRV and the filesystems
//

const TUint KEntryAttIllegal=(KEntryAttVolume|KEntryAttDir);
const TUint KEntryAttModified=0x20000000;
const TUint KEntryAttMustBeFile=0x80000000;
const TInt KCurrentPosition=KMinTInt;


GLDEF_D const TInt KMaxParses=7;
GLDEF_D const TInt KHeapSize=0x2000;
GLREF_C void TurnAllocFailureOff();
GLREF_C void TurnAllocFailureOn();
GLREF_C void ReportCheckDiskFailure(TInt aRet);
GLREF_D RTest test;
GLREF_D TFileName gSessionPath;
GLREF_D TInt gAllocFailOff;
GLREF_D TInt gAllocFailOn;

#if defined(_DEBUG)
#define SetAllocFailure(a) SetAllocFailure(a)
#else
#define SetAllocFailure(a) IsRomAddress(NULL)
#define KAllocFailureOn 0
#define KAllocFailureOff 0
#endif


enum TPanic
	{
	EFileTimeToSystemTime,
	EFileClose,
	EFileCloseSetAttributes,
	EDirClose,
	EMapCouldNotConnect
	};


class CSessionFs;

class CRemoteMountCB : public CMountCB
	{
public:
	CRemoteMountCB();
	~CRemoteMountCB();
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
	void RawReadL(TInt64 aPos,TInt aLength,const TAny* aTrg,TInt anOffset,const RMessagePtr2& aMessage) const;
	void RawWriteL(TInt64 aPos,TInt aLength,const TAny* aSrc,TInt anOffset,const RMessagePtr2& aMessage);
	void ReadUidL(const TDesC& aName,TEntry& anEntry) const;
	void GetShortNameL(const TDesC& aLongName,TDes& aShortName);
	void GetLongNameL(const TDesC& aShortName,TDes& aLongName);
	void IsFileInRom(const TDesC& aFileName,TUint8*& aFileStart);
	void ReadSectionL(const TDesC& aName,TInt aPos,TAny* aTrg,TInt aLength,const RMessagePtr2& aMessage);
private:
	TBool IsRomDrive() const;
	};


class RConsole;
class CRemoteFileCB : public CFileCB
	{
public:
	CRemoteFileCB();
	~CRemoteFileCB();
	void RenameL(const TDesC& aNewName);
	void ReadL(TInt aPos,TInt& aLength,const TAny* aDes,const RMessagePtr2& aMessage);
	void WriteL(TInt aPos,TInt& aLength,const TAny* aDes,const RMessagePtr2& aMessage);
	TInt Address(TInt& aPos) const;
	void SetSizeL(TInt aSize);
	void SetEntryL(const TTime& aTime,TUint aSetAttMask,TUint aClearAttMask);
	void FlushDataL();
	void FlushAllL();
//	inline void SetHandle(HANDLE aHandle) {iWinHandle=aHandle;}
	void CheckPos(TInt aPos);
private:
	TBool IsRomDrive() const;
private:
	TInt iCurrentPos;
	TUint8* iFilePtr;
	RConsole iConsole;
//	HANDLE iWinHandle;
	};

class CRemoteDirCB : public CDirCB
	{
public:
	CRemoteDirCB(/*CSessionFs* aSession*/);
	~CRemoteDirCB();
	void ReadL(TEntry& anEntry);
//	inline void SetHandle(HANDLE aHandle) {iWinHandle=aHandle;}
	inline void SetFullName(const TDesC& aName) {iFullName.Set(aName,NULL,NULL);}
private:
	TBool MatchUid();
public:
	TEntry iEntry;
private:
//	HANDLE iWinHandle;
	TParse iFullName;
	};

class CRemoteFormatCB : public CFormatCB
	{
public:
	CRemoteFormatCB(/*CSessionFs* aSession*/);
	~CRemoteFormatCB();
public:
	virtual void DoFormatStepL();
	};

class CRemote : public CFileSystem
	{
public:
	CRemote();
	TInt Install();
	TInt DefaultPath(TDes& aPath) const;
	void DriveInfo(TDriveInfo& anInfo,TInt aDriveNumber) const;
private:
	CMountCB* NewMountL() const;	//	These were not previously implemented
	CFileCB* NewFileL() const;		//	They are essential for a CRemote
	CDirCB* NewDirL() const;		//	object to be instantiated because they are
	CFormatCB* NewFormatL() const;	//	pure virtual in CFileSystem
public:
	static CFileSystem* NewL();
	};

struct SParse
	{
	const TText* src;
	const TText* rel;
	const TText* def;
	const TText* fullName;
	const TText* drive;
	const TText* path;
	const TText* name;
	const TText* ext;
	};

struct SParseServer
	{
	const TText* src;
	const TText* rel;
	const TText* fullName;
	const TText* drive;
	const TText* path;
	const TText* name;
	const TText* ext;
	};


class TMultipleSessionTest
	{
public:
	TMultipleSessionTest() {};
	TMultipleSessionTest(RFs& aFs):	iFs(aFs){};	
	void Initialise(RFs& aFs);
	void RunTests(RTest& aTest);
	void testDriveList(RTest& aTest);
	void testDriveInfo(RTest& aTest);
	void testVolumeInfo(RTest& aTest);
	void testPath(RTest& aTest);
	void testInitialisation(RTest& aTest);
	void testSubst(RTest& aTest);
	void CopyFileToTestDirectory(RTest& aTest);
	void MakeAndDeleteFiles(RTest& aTest);
	void FillUpDisk(RTest& aTest);
	void testSetVolume(RTest& aTest);
//	void testPowerDown(RTest& aTest);
//	void testMediaChange(RTest& aTest);
	void DeleteTestDirectory(RTest& aTest);
	TInt CurrentDrive(RTest& aTest);
	void SetSessionPath(TInt aDrive);
private:
	RFs iFs;
	TBuf<30> iSessionPath;
	};


//
// WINS - dependent functions
//

GLREF_D const TFileName ZPath;

GLREF_C TBool GetEnvValue(TInt aDrive,TDes& aDes);
GLREF_C TBool MapDrive(TDes& aFileName,TInt aDrive);
GLREF_C TBool MapDriveInfo(TDriveInfo& anInfo,TInt aDrive);
GLREF_C TBool MapDriveAttributes(TUint& aDriveAtt,TInt aDrive);
GLREF_C void CheckAppendL(TDes& aFileName,const TDesC& aName);
#endif


