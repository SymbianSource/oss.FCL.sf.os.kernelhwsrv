// Copyright (c) 2004-2009 Nokia Corporation and/or its subsidiary(-ies).
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
// f32\estart\estart.h
// 
//

/**
 @file
 @internalComponent
*/

#ifndef __ESTART_H__
#define __ESTART_H__

#include <f32file.h>
#include <d32locd.h>

GLREF_D TBool gMountRofsAlone;
GLREF_D TBool gMountComposite;

//#define AUTO_PROFILE
//#define LOAD_PATCH_LDD

#ifdef _DEBUG
extern TBool DebugTraceEnabled();
#define ____IF_DEBUG(s)			if (DebugTraceEnabled()) {s;}
#else
#define ____IF_DEBUG(s)
#endif

#define	DEBUGPRINT(s)			____IF_DEBUG(RDebug::Print(_L(s)))
#define	DEBUGPRINT1(s,a)		____IF_DEBUG(RDebug::Print(_L(s),(a)))
#define	DEBUGPRINT2(s,a,b)		____IF_DEBUG(RDebug::Print(_L(s),(a),(b)))
#define	DEBUGPRINT3(s,a,b,c)	____IF_DEBUG(RDebug::Print(_L(s),(a),(b),(c)))
#define	DEBUGPRINT4(s,a,b,c,d)	____IF_DEBUG(RDebug::Print(_L(s),(a),(b),(c),(d)))
#define	DEBUGPRINT5(s,a,b,c,d,e)	____IF_DEBUG(RDebug::Print(_L(s),(a),(b),(c),(d),(e)))
#define	DEBUGPRINT6(s,a,b,c,d,e,f)	____IF_DEBUG(RDebug::Print(_L(s),(a),(b),(c),(d),(e),(f)))
#define	DEBUGPRINT7(s,a,b,c,d,e,f,g)	____IF_DEBUG(RDebug::Print(_L(s),(a),(b),(c),(d),(e),(f),(g)))

const TInt KMaxLineLen=256;

#define FS_FORMAT_ALWAYS		0x001	// Format the drive once mounted
#define FS_FORMAT_COLD			0x002	// Format the drive on a cold boot
#define FS_FORMAT_CORRUPT 		0x004  	// Format only if corrupt
#define	FS_DISMNT_CORRUPT		0x008	// Dismount if corrupt
#define FS_SWAP_CORRUPT			0x010	// Swap drive mappings with another (specified) drive if corrupt
#define FS_SYNC_DRIVE			0x020	// Mount as a synchronous drive 
#define FS_SCANDRIVE			0x040	// Run scandrive once mounted (if rugged file system enabled)
#define FS_COMPOSITE			0x080	// Mount the composite FSY on this drive
#define FS_NO_MOUNT				0x100	// Add the FSY, map the drive but don't actually mount the drive
#define FS_ALLOW_REM_ACC  		0x200	// Allow this drive to be accessed directly via a remote host
#define FS_NOT_RUGGED  			0x400	// The FAT mount is not rugged
#define FS_SYSTEM_DRIVE			0x800	// The drive is System Drive

const TInt KFsDetectMappingChangeReturnOffset=0x10;
typedef TInt (*TFsDetect)(RLocalDrive, TInt, TLocalDriveCapsV2&);
struct SFsMountInfo
	{
	const TText* iFsyName;		// Filename of .FSY without extension
	const TText* iObjName;		// Object name of file system (NULL = same as iFsyName)
	const TText* iExtName;     	// Name of primary extension if applicable
	TUint32 iFlags;				// Mount options
	};

struct SFileSystemInfo
	{
	TFsDetect iDetect;			// Detection function
	SFsMountInfo iFsInfo;
	};

struct SLocalDriveMappingInfo
	{
	TInt iDriveNumber;
	TInt iLocalDriveNumber;
	SFsMountInfo iFsInfo;
	TInt iSpare;
	TInt iCapsRetCode;
	TBool iRemovable;
	};
	
class TText8FileReader
	{
public:
	TText8FileReader();
	~TText8FileReader();
	TInt Set(RFile& aFile);
	TInt Read(TPtr& aPtr);	
public:
	RFile iFile;
	TBuf8<KMaxLineLen> iBuf;
	TInt iBufPos;
	TText* iFileDataBuf;
	TInt iFileSize;	
	};	


/**
A class that implements the behaviour provided by ESTART.

The class defines a number of virtual functions, which may be overridden
by a customised versions of ESTART.  
*/
class TFSStartup
	{
public:	
	TFSStartup();
	virtual void Init();
	virtual TInt Run();
	virtual TInt LocalDriveInit();
	virtual TInt InitialiseHAL();
	virtual TInt LoadLocale();
	virtual TInt StartSystem();
	virtual TInt ParseCustomMountFlags(TPtrC* aFlagPtr,TUint32& aFlags,TInt& aSpare);
	virtual TInt HandleCustomMountFlags(TInt& aMountRet,TInt& aFlags,TInt& aSpare,TInt aDrive);
	virtual TInt DefaultLocalDrive(TInt aLocalDrive);
	virtual TPtrC LocalDriveMappingFileName();
    virtual void ShowFormatProgress(TInt aPercent, TInt aDrive);
	virtual void Close();
protected:
	virtual TInt GetStartupMode();
    virtual TInt GetStartupModeFromFile();
	virtual TInt SysFileNames(RArray<TPtrC>& aFileNames);
public:
	TInt ProcessLocalDriveMappingFile();
	TInt ParseMappingFileFlags(const TPtrC& aFlagDesc,TUint32& aFlagVar,TInt& aSpare);
	TInt ParseMappingRecord(TPtr& aTextLine,SLocalDriveMappingInfo& anInfo);
	void CheckAndReOrderArrayForSwapping(TInt aTotalEntries);
	void SwapDriveMappings(TInt aCurrentEntry,TInt aTotalEntries);
	TInt MountFileSystem(SLocalDriveMappingInfo& anInfo);
	void SetFServLocalDriveMapping();
	void SwapFServLocalDriveMapping(TInt aFirstDrive,TInt aSecondDrive);
	TInt InitCompositeFileSystem();
	void LoadCompositeFileSystem(TInt aDrive);
	void LoadCompositeFileSystem(SLocalDriveMappingInfo& anInfo);
	TBool CreateServer(const TDriveList& aDrives, const TDesC& aRootName);
	TInt FormatDrive(TInt aDrive);
    TInt MountRemovableDrives();
    TInt ParseMappingFileLocalDrive(const TPtrC& aDriveDesc,TUint32 (&aDrives)[KMaxLocalDrives],TInt& aCount);
	void LocalFSInitialisation();
	TInt SearchForUnusedDriveNumber(TInt& aDrvNum);
#if !defined(AUTODETECT_DISABLE)
	virtual TInt GetNextStandardFSInfoEntry(const SFileSystemInfo** anEntry,TInt aPos);
	TInt DetectFileSystem(TInt aLocalDriveNumber,SLocalDriveMappingInfo& anInfo);
	TInt DetectAndMountFileSystems();
#endif		
#if defined(_LOCKABLE_MEDIA)
	TInt WriteLocalPwStore(RFile& aFile);
	TInt InitializeLocalPwStore();	
#endif
#ifdef LOAD_PATCH_LDD	
	void LoadPatchLDDs();
#endif	
#ifdef AUTO_PROFILE
	void StartProfiler();
#endif
private:
	void SetSystemDrive();
public:
    TInt iStartupMode;                                              
	TInt iMuid;                                                     
	RFs iFs;                                                        
	TInt iTotalSupportedDrives;                                     
	TInt iRuggedFileSystem;                                         
	TUint iRegisteredDriveBitmask;                                  
	TInt iLocalDriveList[KMaxLocalDrives];                          
	SLocalDriveMappingInfo iDriveMappingInfo[KMaxLocalDrives];		
	TInt iDriveSwapCount;                                           
	TBool iColdStart;                                               
	TUint iUnmountedDriveBitmask;                                   
    TInt iMapCount;                                                 
    TText8FileReader* iMapFile;                                     
	};

TInt StartSysAgt2(); // launch system agent 2 server


#if !defined(AUTODETECT_DISABLE)	
GLREF_C TInt DetectELocal(RLocalDrive ld, TInt cr, TLocalDriveCapsV2& caps);
GLREF_C TInt DetectIRam(RLocalDrive ld, TInt cr, TLocalDriveCapsV2& caps);
GLREF_C TInt DetectRofs(RLocalDrive ld, TInt cr, TLocalDriveCapsV2& caps);
GLREF_C TInt DetectComposite(RLocalDrive ld, TInt cr, TLocalDriveCapsV2& caps);
GLREF_C TInt DetectEneaLFFS(RLocalDrive ld, TInt cr, TLocalDriveCapsV2& caps);
GLREF_C TInt DetectIso9660(RLocalDrive ld, TInt cr, TLocalDriveCapsV2& caps);
GLREF_C TInt DetectNtfs(RLocalDrive ld, TInt cr, TLocalDriveCapsV2& caps);
GLREF_C TInt DetectFtl(RLocalDrive ld, TInt cr, TLocalDriveCapsV2& caps);
#endif

#endif
