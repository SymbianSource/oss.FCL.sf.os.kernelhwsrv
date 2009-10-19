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
//

//! @file f32test\concur\t_cfssoak.h

#ifndef __T_CFSSOAK_H__
#define __T_CFSSOAK_H__

#include <f32file.h>
#include <e32test.h>
#include "t_server.h"
#include "t_tdebug.h"

class TSoakStats
/// Collect and print statistics.
	{
public:
	TSoakStats() : iTotal(0), iFail(0), iThis(0), iThisF(0) {}
	static void Print();
	void Print(const TDesC& aTitle);
	void Inc();
	void Fail();

public:
	TInt iTotal;
	TInt iFail;
	TInt iThis;
	TInt iThisF;
	};

class TSoakReadOnly
/// Tests for reading and scanning drives and directories.
	{
public:
	TSoakReadOnly();
	TSoakReadOnly(TInt aDrive);
	~TSoakReadOnly();
	TInt ReadFile(const TDesC& aName, TInt aSize);
	TInt ScanDirs(TInt aDrive, TInt aReadInterval);
	TInt ScanDirFunc(CDirScan* aScanner, TInt aDrive, TInt aReadInterval);
	TInt ScanDrives(TBool aScanDirs, TInt aReadInterval);
	void ExcludeDrive(TInt aDrive);

public:
	TSoakStats iDrives;	///< Statistics of number of drives scanned.
	TSoakStats iDirs;	///< Statistics of number of directories scanned.
	TSoakStats iFiles;	///< Statistics of number of files found.
	TSoakStats iReads;	///< statistics of number of files read.
	
private:
	TInt64 iSeed;
	TInt   iDrive;
	RFs    iFs;
	};

class TSoakFill
/// Tests filling and cleaning a drive.
	{
public:
	TSoakFill();
	TInt SetDrive(TInt aDrive);
	TInt FillDrive();
	TInt CleanDrive();
	TInt Fill(TFileName& aName, TInt aNfiles=0);

private:
	TInt64      iSeed;
	TInt64      iFree;
	TInt        iDrive;
	TInt        iDrvCh;
	RFs         iFs;
	TFileName   iName;
	TVolumeInfo iInfo;
	};

const TInt KSoakNumBuf = 10;	///< Number of buffers to be written.
const TInt KSoakBufLen = 0x100;	///< Length of each buffer.

class TSoakRemote
/// Tests on a 'remote' (special delaying) filesystem.
	{
public:
	TSoakRemote(TInt aDrive);
	void Remount(TBool aSync);
	TInt TestSession();
	TInt TestSubSession();
	TInt TestMount();

private:
	void Setup();

private:
	TInt      iDrive;
	TInt      iDrvCh;
	TBool	  iSync;
	RFs		  iFs;
	RFile     iFile;
	RTimer    iTimer;
	TFileName iName;
	TBuf8<KSoakBufLen> iBuff[KSoakNumBuf];
	TRequestStatus     iStat[KSoakNumBuf];
	};

struct TExtension
	{
	TFullName iName;
	TBool iExists;
	};


#endif
