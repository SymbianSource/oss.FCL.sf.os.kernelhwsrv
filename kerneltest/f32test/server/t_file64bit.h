/**
* Copyright (c) 1996-2009 Nokia Corporation and/or its subsidiary(-ies).
* All rights reserved.
* This component and the accompanying materials are made available
* under the terms of the License "Eclipse Public License v1.0"
* which accompanies this distribution, and is available
* at the URL "http://www.eclipse.org/legal/epl-v10.html".
*
* Initial Contributors:
* Nokia Corporation - initial contribution.
*
* Contributors:
*
* Description:
* File Name:		f32test/server/t_file64bit.h
* Include file for t_file64bit.cpp (PREQ 1725).
* 
*
*/





#if !defined(__T_FILE64BIT_H__)
#define __T_FILE64BIT_H__

const TInt64 KKB  					= 1 << 10;
const TInt64 K2KB  					= 2 * KKB;
const TInt64 K4KB 					= 4 * KKB;
const TInt64 K32KB					= 8 * K4KB;
const TInt64 KGB 					= 1 << 30;
const TInt64 K2GB 					= 2 * KGB;
const TInt64 K3GB 					= 3 * KGB;
const TInt64 K4GB 					= 4 * KGB;
const TInt64 K9GB					= 9 * KGB;
const TInt64 K12GB					= 12 * KGB;
const TInt64 K1GBMinusOne 			= KGB  - 1;
const TInt64 KSize1GMinus4K 		= KGB - K4KB;
const TInt64 K2GBMinusOne 			= K2GB - 1;
const TInt64 K2GBMinusTen   		= K2GB - 10;
const TInt64 K2GBMinus100			= K2GB - 100;
const TInt64 KSize2GMinus4K 		= K2GB - K4KB;
const TInt64 K2GBPlusTen			= K2GB + 10;
const TInt64 K2GBPlus16				= K2GB + 16;
const TInt64 K2GBPlus80 			= K2GB + 80;
const TInt64 K2GBPlus98				= K2GB + 98;
const TInt64 K2GBPlus100			= K2GB + 100;
const TInt64 K2GBPlus200			= K2GB + 100;
const TInt64 K2GBPlus600			= K2GB + 600;
const TInt64 K2GBPlus300			= K2GB + 300;
const TInt64 K2GBPlus500			= K2GB + 500;
const TInt64 K3GBMinusOne			= K3GB - 1;
const TInt64 KSize3GMinus4K 		= K3GB - K4KB;
const TInt64 K4GBMinusOne 			= K4GB - 1;
const TInt64 K4GBMinusTwo 			= K4GB - 2;
const TInt64 K4GBMinusTen   		= K4GB - 10;
const TInt64 K4GBMinus100   		= K4GB - 100;
const TInt64 KSize3GMinus1Minus4K 	= K3GB -1 - K4KB;
const TInt64 KSize4GMinus1Minus4K 	= K4GB - 1 - K4KB;
const TInt64 KSize2GMinus1Minus4K 	= K2GB - 2 - K4KB;
const TInt64 K4GBPlusOne			= K4GB + 1;
const TInt64 K4GBPlusTen 			= K4GB + 10;


const TInt	KBUFSIZE 	 	 		= 10;
const TInt	KMAXBUFSIZE 	 		= 2*KKB;
const TInt	KWRITEBUFSIZE 	 		= KKB;
LOCAL_D TBuf8<0x63> pattern;

// Maximum bytes per cluster recommended in FAT32
const TInt		KMaxClusterSize 	= K32KB;

//-------------------------------------------------------------------------------------------------------------------

_LIT(KFsName_FAT32, "Fat32");
_LIT(KFsName_NGFS, "NGFS");
_LIT(KFile2GBMinusOne, "File2GBMinusOne.txt");
_LIT(KFile2GB, "File2GB.txt");
_LIT(KFile3GB, "File3GB.txt");
_LIT(KFile4GBMinusOne, "File4GBMinusOne.txt");
_LIT( KClientFileName, "File4GBMinusOne.txt");
_LIT(KServerName, "FHServer64bit");
TInt gFilesInDirectory = 4;

TVolumeInfo 		gDriveVolumeInfo;
TInt 				gDrive;
// Static function prototypes.
LOCAL_C void TestOpenFiles();
LOCAL_C void TestAdoptFiles();

LOCAL_C void TestReadFile();
LOCAL_C void TestWriteFile();
LOCAL_C void TestFileAccess();
LOCAL_C void TestLockUnLock(); 
LOCAL_C void TestCopyMoveDirectory();


class CFileManObserver : public CBase, public MFileManObserver
	{
public:
	CFileManObserver(CFileMan* aFileMan);

	TControl NotifyFileManStarted();
	TControl NotifyFileManOperation();
	TControl NotifyFileManEnded();
private:
	CFileMan* iFileMan;
public:
	TInt iNotifyEndedSuccesses;
	TInt iNotifyEndedFailures;
	};


class RFsTest : public RFs
	{
public:
	RFsTest& Replace(const TDesC &anOldName, const TDesC &aNewName);
	RFsTest& ReadFileSection(const TDesC& aName, TInt64 aPos, TDes8& aBuffer, TInt aLen);		
	RFsTest& GetDir(const TDesC &aName, TUint anEntryAttMask, TUint anEntrySortKey, CDir *&anEntryList);
	RFsTest& GetDir(const TDesC& aName,TUint anEntryAttMask,TUint anEntrySortKey,CDir*& anEntryList,CDir*& aDirList);
	RFsTest& GetDir(const TDesC& aName,const TUidType& anEntryUid,TUint anEntrySortKey,CDir*& aFileList);
	};
LOCAL_D	RFsTest TestRFs;

class RFileTest : public RFile64
	{
public:
	RFileTest(const TDesC& aName);
	RFileTest& Create(const TDesC& aName,TUint aFileMode);
	RFileTest& Replace(const TDesC& aName);
	RFileTest& Replace(const TDesC& aName, TUint aFileMode);
	RFileTest& Open(const TDesC& aName);
	RFileTest& Open(const TDesC& aName, TUint aFileMode);
	RFileTest& Temp(const TDesC& aPath,TFileName& aName,TUint aFileMode);
	void Close();
	RFileTest& Lock(TInt64 aPos,TInt64 aLen=1);
	RFileTest& LockE(TInt64 aPos,TInt64 aLen=1);
	RFileTest& UnLock(TInt64 aPos,TInt64 aLen=1);
	RFileTest& UnLockE(TInt64 aPos,TInt64 aLen=1);
	
	RFileTest& Write(const TDesC8& aDes);
	RFileTest& Write(const TDesC8& aDes, TRequestStatus& aStatus);
	RFileTest& Write(const TDesC8& aDes, TInt aLength);
	RFileTest& Write(const TDesC8& aDes, TInt aLength, TRequestStatus &aStatus);
	RFileTest& Write(TInt64 aPos,const TDesC8& aDes,TInt aLen=1);
	RFileTest& WriteE(TInt64 aPos,const TDesC8& aDes,TInt aLen=1);
	RFileTest& Write(TInt64 aPos,const TDesC8& aDes,TInt aLen,TRequestStatus &aStatus);	
	RFileTest& WriteP(TInt64 aPos,const TDesC8& aDes);
	RFileTest& Write(TInt64 aPos,const TDesC8& aDes,TRequestStatus& aStatus);		
	RFileTest& WriteU(TUint aPos,const TDesC8& aDes);
	RFileTest& WriteU(TUint aPos,const TDesC8& aDes,TRequestStatus& aStatus);
	RFileTest& WriteU(TUint aPos,const TDesC8& aDes, TInt aLength);
	RFileTest& WriteU(TUint aPos,const TDesC8& aDes, TInt aLength,TRequestStatus& aStatus);
	
	
	RFileTest& Read(TDes8& aDes);
	RFileTest& Read(TDes8& aDes, TRequestStatus& aStatus);
	RFileTest& Read(TDes8 &aDes, TInt aLen);
	RFileTest& Read(TDes8 &aDes, TInt aLength, TRequestStatus &aStatus);
	RFileTest& Read(TInt64 aPos,TDes8& aDes,TInt aLen=1);
	RFileTest& ReadE(TInt64 aPos,TDes8& aDes,TInt aLen=1);
	RFileTest& Read(TInt64 aPos,TDes8& aDes);
	RFileTest& Read(TInt64 aPos,TDes8& aDes,TInt aLen, TRequestStatus& aStatus);
	RFileTest& ReadP(TInt64 aPos,TDes8& aDes);
	RFileTest& Read(TInt64 aPos,TDes8& aDes,TRequestStatus& aStatus);
	RFileTest& ReadU(TUint aPos,TDes8& aDes,TRequestStatus& aStatus);
	RFileTest& ReadU(TUint aPos,TDes8& aDes);
	RFileTest& ReadU(TUint aPos,TDes8& aDes, TInt aLength);
	RFileTest& ReadU(TUint aPos,TDes8& aDes, TInt aLength,TRequestStatus& aStatus);
	
	RFileTest& SetSize(TInt64 aSize);
	RFileTest& SetSizeE(TInt64 aSize);
	RFileTest& Size(TInt64& aSize);
	RFileTest& Seek(TSeek aMode,TInt64& aPos);
private:
	TName iName;
	};
LOCAL_D	RFileTest TestRFile1(_L("File 1"));
LOCAL_D	RFileTest TestRFile2(_L("File 2"));
#endif
