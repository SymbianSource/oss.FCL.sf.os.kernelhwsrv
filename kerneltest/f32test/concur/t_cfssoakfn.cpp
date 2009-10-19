// Copyright (c) 2002-2009 Nokia Corporation and/or its subsidiary(-ies).
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
// f32test\server\t_csfsoakfn.cpp
// 
//

#include <f32file.h>
#include <e32test.h>
#include <e32math.h>
#include <f32dbg.h>
#include "t_server.h"
#include "t_tdebug.h"
#include "t_cfssoak.h"

/// Time value constants
LOCAL_D const TInt KSecond   = 1000000;
LOCAL_D const TInt KTenthSec = KSecond / 10;

/// Time to wait before session/subsession close, to allow some writes to
/// complete but not all (1 second per write with slow FS)
LOCAL_D const TInt KSessionWaitTime = 25 * KTenthSec;

/// Number of writes we expect to have completed
LOCAL_D const TInt KSessionNumEnded = KSessionWaitTime / KSecond;

GLREF_D TExtension gPrimaryExtensions[];
// -----------------------------------------------------------------

void TSoakStats::Print()
///
/// Print a statistics header.
///
	{
	TTest::Printf(_L("                    Total                Fail\n"));
	}

void TSoakStats::Print(const TDesC& aTitle)
///
/// Print the statistics for this item (title, total executed, number executed
/// this time, total failures, failures this time) then reset the "this time"
/// values.
///
	{
	TTest::Printf(_L("  %-8S %8d %+8d   %8d %+8d\n"),
				  &aTitle, iTotal, iThis, iFail, iThisF);
	iThis = 0;
	iThisF = 0;
	}

void TSoakStats::Inc()
///
/// Increment both the total and "this time" execution numbers.
///
	{
	iTotal++;
	iThis++;
	}

void TSoakStats::Fail()
///
/// Increment both the total and "this time" failure numbers.
///
	{
	iFail++;
	iThisF++;
	}


// -----------------------------------------------------------------

TSoakReadOnly::TSoakReadOnly() : iDrive(-1)
///
/// Initialise the "read only" tests, connect the file session.
///
	{
	TInt r = iFs.Connect();
	if (r != KErrNone)
		TTest::Fail(HERE, r, _L("iFs connect"));
	}

TSoakReadOnly::TSoakReadOnly(TInt aDriveCh) : iDrive(aDriveCh)
///
/// Initialise the "read only" tests, connect the file session, setting up
/// a drive to be excluded from the reading.
/// @param aDriveCh Drive letter to be excluded.
///
	{
	TInt r = iFs.Connect();
	if (r != KErrNone)
		TTest::Fail(HERE, r, _L("iFs connect"));
	}

TSoakReadOnly::~TSoakReadOnly()
///
/// Destructor -- close the file session.
///
	{
	iFs.Close();
	}

TInt TSoakReadOnly::ReadFile(const TDesC& aName, TInt aSize)
///
/// Read a file, expecting a certain size.  No check is done on the actual
/// data, just that it can be read.
/// @param aName Name of the file.
/// @param aSize Expected file size.
///
	{
	TInt r = KErrNone;
	RFile f;
	iReads.Inc();
	r = f.Open(iFs, aName, EFileStreamText);
	if (r == KErrNotFound || r == KErrPathNotFound)
		return KErrNone;
	TBuf<256> errbuf;
	if (r != KErrNone)
		{
		if (r != KErrInUse)
			{
			iFiles.Fail();
			TTest::Printf(_L("%S -- open failed %S\n"), &aName, &TTest::ErrStr(r, errbuf));
			}
		else
			TTest::Printf(_L("Warning: %S -- open failed %S\n"), &aName, &TTest::ErrStr(r, errbuf));
		return r;
		}
	TBuf8<256> buf;
	TInt len = 0;
	while ((r = f.Read(buf)) == KErrNone && buf.Length() > 0)
		len += buf.Length();
	f.Close();
	if (r != KErrNone && r != KErrEof)
		{
		iReads.Fail();
		TTest::Printf(_L("ERROR %S -- READ FAILED %S\n"), &aName, &TTest::ErrStr(r, errbuf));
		}
	else if (len < aSize)
		{
		iReads.Fail();
		TTest::Printf(_L("ERROR %S -- %d READ, %d EXPECTED\n"), &aName, len, aSize);
		}
	else if (len > aSize)
		TTest::Printf(_L("Warning: %S -- %d read, %d expected\n"), &aName, len, aSize);
	return r;
	}

TInt TSoakReadOnly::ScanDirs(TInt aDriveCh, TInt aReadInterval)
///
/// Scan directories on a drive, reading a selection of the files found.
/// @param aDriveCh Drive letter to be scanned.
/// @param aReadInterval Approximate number of files to be read (they are
///                      read at random, so the actual number read may be
///                      less than this number).
///
	{
	TInt r = KErrNone;
	CDirScan* scanner=NULL;
	TRAP(r, scanner=CDirScan::NewL(iFs));
	ScanDirFunc(scanner, aDriveCh, aReadInterval);
	delete scanner;
	return r;
	}

TInt TSoakReadOnly::ScanDirFunc(CDirScan* aScanner, TInt aDriveCh, TInt aReadInterval)
///
/// Scan directories on a drive, reading a selection of the files found.
/// @param aDriveCh Drive letter to be scanned.
/// @param aReadInterval Approximate number of files to be read (they are
///                      read at random, so the actual number read may be
///                      less than this number).
///
	{
	TInt r = KErrNone;
	TBuf<8> name;
	name.Format(_L("%c:\\"), aDriveCh);
	// TTest::Printf(_L("Scanning %S\n"), &name);
	TParse dirName;
	r=iFs.Parse(name, dirName);
	if (r != KErrNone)
		{
		iDrives.Fail();
		return r;
		}
	CDir* entryList = NULL;
	TInt nfiles = 0;
	TRAP(r, aScanner->SetScanDataL(dirName.FullName(),KEntryAttDir,ESortByName));
	if (r != KErrNone)
		{
		TTest::Fail(HERE, r, _L("SetScanData %S"), &dirName.FullName());
		}
	TRAP(r, aScanner->NextL(entryList));
	if (r == KErrPathNotFound)
		return KErrNone;
	if (r != KErrNone)
		{
		TTest::Fail(HERE, r, _L("Scan Next %S"), &dirName.FullName());
		}
	while (entryList)
		{
		for (TInt i = 0; i < entryList->Count(); i++)
			{
			TEntry e = (*entryList)[i];
			if (!e.IsDir())
				++nfiles;
			}
		delete entryList;
		TRAP(r, aScanner->NextL(entryList));
		if (r == KErrPathNotFound)
			break;
		if (r != KErrNone)
			{
			TTest::Fail(HERE, r, _L("Scan Next %S"), &dirName.FullName());
			}
		}
	TInt prob = (nfiles / aReadInterval) + 1;
	TRAP(r, aScanner->SetScanDataL(dirName.FullName(),KEntryAttDir,ESortByName));
	if (r == KErrPathNotFound)
		return KErrNone;
	if (r != KErrNone)
		{
		TTest::Fail(HERE, r, _L("Scan Next %S"), &dirName.FullName());
		}
	for (;;)
		{
		TRAP(r, aScanner->NextL(entryList));
		if (r == KErrPathNotFound)
			return KErrNone;
		if (r != KErrNone)
			{
			TTest::Fail(HERE, r, _L("Scan Next %S"), &dirName.FullName());
			}
		if (entryList==NULL)
			break;
		TInt count=entryList->Count();
		while (count--)
			{
			TEntry e = (*entryList)[count];
			TFileName path=aScanner->FullPath();
			path.Append(e.iName);
			TBuf<16> attr;
			for (TInt j = 0; j < 16; j++)
				{
				if ((1 << j) & e.iAtt)
					attr.Append(TText("RHSVDA X        "[j]));
				}
			if (e.IsDir())
				{
				iDirs.Inc();
				// TTest::Printf(_L("%c:%-50S    <DIR>  %S\n"), aDriveCh, &path, &attr);
				}
			else
				{
				iFiles.Inc();
				TBool read = (aReadInterval && Math::Rand(iSeed) % prob == 0);
				if (read)
					{
					// TTest::Printf(_L("%-50S %8d  %-8S  test\n"), &path, e.iSize, &attr);
					r = ReadFile(path, e.iSize);
					}
				else
					{
					// TTest::Printf(_L("%c:%-50S %8d  %S\n"), aDriveCh, &path, e.iSize, &attr);
					}
				}
			}
		delete entryList;
		entryList=NULL;
		}
	return r;
	}

TInt TSoakReadOnly::ScanDrives(TBool aScanDirs, TInt aReadInterval)
///
/// Scan all drives (except the one excluded) for directories and files.
/// @param aScanDirs If true, read subdirectories, if false don't.
/// @param aReadInterval approximate number of files to read.
///
	{
	TInt r = KErrNone;
	TInt i;
	for (i = EDriveA; i <= EDriveZ; i++)
		{
		TChar drv;
		r=iFs.DriveToChar(i, drv);
		if (r != KErrNone)
			return r;
		TDriveInfo info;
		r=iFs.Drive(info, i);
		if (r != KErrNone)
			return r;
		if (i != iDrive
			&& info.iDriveAtt != 0
			&& info.iType != EMediaUnknown
			&& info.iType != EMediaNotPresent)
			{
			iDrives.Inc();
			// TTest::Printf(_L("Drive %c:"), drv);
			if (aScanDirs)
				{
				iDirs.Inc();
				r = ScanDirs(drv, aReadInterval);
				if (r != KErrNone)
					return r;
				}
			}
		}
	return r;
	}

void
TSoakReadOnly::ExcludeDrive(TInt aDriveCh)
///
/// Set a drive to be excluded from scanning.
/// @param aDriveCh Drive letter to be excluded.
///
	{
	iDrive = aDriveCh;
	}


// --------------------------------------------------------------------------

TSoakFill::TSoakFill()
///
/// Initialise the fill/clean cycle.
///
	{
	iSeed = 186483;
	TInt r = iFs.Connect();
	if (r != KErrNone)
		TTest::Fail(HERE, r, _L("iFs connect"));
	}

TInt TSoakFill::SetDrive(TInt aDriveCh)
///
/// Set a drive to be stressed.  The test directory is created and then it is
/// cleaned (in case anything was left over from previous tests).  Note that
/// other directories on the drive are untouched.
/// @param aDriveCh Drive letter to be used.
/// @leave The drive scanning can cause the function to leave.
///
	{
	TInt  r = KErrNone;
	iDrive = aDriveCh;
	if (aDriveCh)
		{
		iDrvCh = aDriveCh;
		iFs.CharToDrive(iDrvCh, iDrive);
		r=iFs.Volume(iInfo, iDrive);
		if (r != KErrNone)
			TTest::Fail(HERE, r, _L("volume info for %C:"), aDriveCh);
		iFree = iInfo.iSize;
		iName.Format(_L("%c:\\SOAK\\"), iDrvCh);
		r = iFs.MkDir(iName);
		if (r != KErrNone && r != KErrAlreadyExists)
			TTest::Fail(HERE, r, _L("mkdir %S"), &iName);
		r = KErrNone;
		}
	return r;
	}

TInt TSoakFill::FillDrive()
///
/// Fill the drive with files and directories of random lengths.
/// @param aDriveCh Drive letter to be tested, or zero (absent) to use the one
///        set up with SetDrive().
/// @leave Setting up a new drive can cause this function to leave.
///
	{
	TInt  r = KErrNone;
	r=iFs.Volume(iInfo, iDrive);
	if (r != KErrNone)
		TTest::Fail(HERE, r, _L("volume info for %C:"), iDrvCh);
	iFree = iInfo.iFree;
	r = iFs.MkDir(iName);
	if (r != KErrNone && r != KErrAlreadyExists && r != KErrNotSupported)
		TTest::Fail(HERE, r, _L("mkdir %S"), &iName);
	do
		{
		r = Fill(iName);
		if (r == KErrDiskFull)
			{
			// TTest::Printf(_L("Disk full on %c:\n"), iDrvCh);
			break;
			}
		if (r != KErrNone)
			TTest::Fail(HERE, r, _L("Filling drive %c"), iDrvCh);
		r=iFs.Volume(iInfo, iDrive);
		if (r != KErrNone)
			TTest::Fail(HERE, r, _L("volume info for %c:"), iDrvCh);
		} while (iInfo.iFree > 0);
	r=iFs.Volume(iInfo, iDrive);
	if (r != KErrNone)
		TTest::Fail(HERE, r, _L("volume info for %c:"), iDrvCh);
	if (iInfo.iFree > 0)
		TTest::Printf(_L("Free space on %c: %S %ld KB (out of %ld KB)\n"),
					  iDrvCh,
					  &iInfo.iName,
					  I64LOW(iInfo.iFree / 1024),
					  I64LOW(iInfo.iSize / 1024));
	return r;
	}

TInt TSoakFill::CleanDrive()
///
/// Clean the test directory on the selected drive, removing all files and
/// subdirectories.  Other directories on the drive are left untouched.
/// @leave Scanning the drive can cause the function to leave.
///
	{
	TInt r = KErrNone;
	CDirScan* scanner=0;
	TRAP(r, scanner=CDirScan::NewL(iFs));
	if (r != KErrNone || !scanner)
		TTest::Fail(HERE, r, _L("creating scanner"));
	TParse dirName;
	r=iFs.Parse(iName, dirName);
	if (r != KErrNone)
		{
		if (r == KErrInUse || r == KErrNotFound || r == KErrPathNotFound)
			{
			// valid conditions, don't report error
			r = KErrNone;
			}
		else
			TTest::Fail(HERE, r, _L("Parse %S"), &iName);
		}
	else
		{
		// TTest::Printf(_L("scan %S\n"), &iName);
		CDir* entryList = NULL;
		TRAP(r, scanner->SetScanDataL(dirName.FullName(),KEntryAttDir,ESortByName,CDirScan::EScanUpTree));
		if (r != KErrNone)
			TTest::Fail(HERE, r, _L("SetScanDataL(%S)"), &dirName.FullName());
		TRAP(r, scanner->NextL(entryList));
		if (r != KErrNone && r != KErrPathNotFound)
			TTest::Fail(HERE, r, _L("Scan NextL()"));
		while (entryList)
			{
			for (TInt i = 0; i < entryList->Count(); i++)
				{
				TEntry e = (*entryList)[i];
				TFileName path = iName;
				if (path.Right(1) == _L("\\"))
					path.SetLength(path.Length()-1);
				path.Append(scanner->AbbreviatedPath());
				path.Append(e.iName);
				// TTest::Printf(_L("remove %S\n"), &path);
				TBuf<256> buf;
				TInt maxInUseCount = 120;
				do
					{
					if (e.IsDir())
						{
						if (path.Right(1) != _L("\\"))
							path.Append(_L("\\"));
						r = iFs.RmDir(path);
						}
					else
						r = iFs.Delete(path);
					if (r == KErrInUse)
						{
						TTest::Printf(_L("Warning: %S deleting %S\n"), &TTest::ErrStr(r, buf), &path);
						User::After(1*KSecond);
						}
					}
					while (r == KErrInUse && maxInUseCount-- > 0);
				if (r != KErrNone)
					{
					TTest::Printf(_L("ERROR %S deleting %S\n"), &TTest::ErrStr(r, buf), &path);
					}
				}
			delete entryList;
			TRAP(r, scanner->NextL(entryList));
			if (r != KErrNone)
				TTest::Fail(HERE, r, _L("Scan NextL()"));
			}
		}
	delete scanner;
	r = iFs.RmDir(iName);
	if (r != KErrNone && r != KErrNotFound && r != KErrPathNotFound)
		{
		TTest::Fail(HERE, r, _L("volume info for %C:"), iName[0]);
		}
	r=iFs.Volume(iInfo, iDrive);
	if (r != KErrNone)
		{
		TTest::Fail(HERE, r, _L("volume info for %C:"), iName[0]);
		}
	if (iInfo.iFree > iFree)
		{
		TTest::Printf(_L("Warning: %C: changed size -- was %ld now %ld\n"),
					  iName[0], iFree, iInfo.iFree);
		return r;
		}
	iFree = iInfo.iFree;
	return r;
	}

TInt TSoakFill::Fill(TFileName& aName, TInt aNfiles)
///
/// Recursively create files and subdirectories at random.  If a subdirectory
/// is created, the function recurses to fill it.  This generates a mixture of
/// directories and files of varying lengths.
/// @param aName   Directory in which to create other stuff.
/// @param aNfiles Maximum number of files to create.
/// @return Status of last thing created, KErrDiskFull when out of space.
///
	{
	TInt oldlen = aName.Length();
	if (oldlen + 10 > aName.MaxLength())
		return KErrNone;
	TInt r=iFs.Volume(iInfo, iDrive);
	if (r != KErrNone)
		return r;
	TInt nfiles = I64LOW(iInfo.iFree/10000) + 5;
	if (aNfiles > 0 && nfiles > aNfiles)
		nfiles = aNfiles;
	nfiles = Math::Rand(iSeed) % nfiles + 5;
	TInt filesz = I64LOW(iInfo.iFree/nfiles/256) + 5;
	// TTest::Printf(_L("nfiles = %d\n"), nfiles);
	TInt i;
	for (i = 0; i < nfiles; i++)
		{
		aName.SetLength(oldlen);
		TBool dir = (Math::Rand(iSeed) % 5 == 0);
		if (dir && oldlen < 80)
			{
			aName.Append(_L("d"));
			aName.AppendNum(i);
			aName.Append(_L("\\"));
			r = iFs.MkDir(aName);
			TInt busycount = 120;
			while (busycount-- > 0)
				{
				r = iFs.MkDir(aName);
				if (r == KErrDiskFull)
					TTest::Printf(_L("Disk full creating %S\n"), &aName);
				if (r != KErrInUse)
					break;
				TTest::Printf(_L("Warning: KErrInUse creating %S\n"), &aName);
				User::After(1*KSecond);
				}
			// TTest::Printf(_L("mkdir %S = %d\n"), &aName, r);
			if (r == KErrNone || r == KErrAlreadyExists)
				{
				r = Fill(aName, nfiles - i - 2);
				}
			if (r != KErrNone)
				break;
			}
		else
			{
			aName.Append(_L("F"));
			aName.AppendNum(i);
			RFile f;
			TInt busycount = 120;
			while (busycount-- > 0)
				{
				r = f.Replace(iFs, aName, EFileStreamText | EFileWrite);
				if (r != KErrInUse)
					break;
				TTest::Printf(_L("Warning: KErrInUse creating %S\n"), &aName);
				User::After(1*KSecond);
				}
			if (r == KErrNone)
				{
				// iNames.AppendL(aName);
				TInt num = Math::Rand(iSeed) % filesz + 10;
				TBuf8<256> buf;
				buf.Fill(TText(' '), 256);
				TInt wr = 0;
				while (num-- > 0 && r == KErrNone)
					{
					r = f.Write(buf);
					if (r == KErrNone)
						wr += 256;
					else
						break;
					}
				f.Close();
				/*
				if (r == KErrDiskFull)
					TTest::Printf(_L("Disk full writing %S\n"), &aName);
				*/
				if (r != KErrNone)
					break;
				}
			else
				{
				// TTest::Printf(_L("creat %S = %d\n"), &aName, r);
				if (r == KErrDiskFull)
					TTest::Printf(_L("Disk full creating %S\n"), &aName);
				break;
				}
			}
		}
	aName.SetLength(oldlen);
	return r;
	}


TSoakRemote::TSoakRemote(TInt aDriveCh)
///
/// Initialise testing for 'remote' drive (special filesystem).  Connects to
/// file session, initialises the drive and timer, sets up the buffers and
/// file name.
/// @param aDriveCh Drive letter for the drive to test.
///
	{
	TInt r = iFs.Connect();
	if (r != KErrNone)
		TTest::Fail(HERE, r, _L("iFs connect"));
	iDrvCh = aDriveCh;
	iSync  = EFalse;
	iFs.CharToDrive(iDrvCh, iDrive);
	iTimer.CreateLocal();
	Setup();
	}

void TSoakRemote::Setup()
/// Set up the buffers and the filename to be used.
	{
	for (TInt i = 0; i < KSoakNumBuf; i++)
		{
		iBuff[i].Fill('_', KSoakBufLen);
		iStat[i] = KErrNone;
		}
	iName.Format(_L("%c:\\SOAKTEST.FILE"), iDrvCh);
	}

void TSoakRemote::Remount(TBool aSync)
///
/// Remount the test drive as (a)synchronous.
/// @param aSync If true, the drive is set as synchronous access, otherwise
///              as asynchronous.
///
	{
	TFileName fsname;
	iSync = aSync;
	TInt r = iFs.FileSystemName(fsname, iDrive);
	TEST(r == KErrNone || r == KErrNotFound);

	if (fsname.Length() > 0)
		{
		r = iFs.ExtensionName(gPrimaryExtensions[iDrive].iName, iDrive, 0);
		if (r == KErrNone)
			gPrimaryExtensions[iDrive].iExists = ETrue;

		r = iFs.DismountFileSystem(fsname, iDrive);
		if(r != KErrNone)
			{
			TTest::Fail(HERE, r, _L("Dismounting file system %S"), &fsname);
			}
		}

	TBufC<16> type = _L("asynchronous");
	if (iSync)
		type = _L("synchronous");
	TTest::Printf(_L("Remount filesystem %c: %S as %S\n"), iDrvCh, &fsname, &type);

#ifdef __CONCURRENT_FILE_ACCESS__
	if (gPrimaryExtensions[iDrive].iExists == EFalse)
		r=iFs.MountFileSystem(fsname,iDrive,iSync);
	else
		r=iFs.MountFileSystem(fsname,gPrimaryExtensions[iDrive].iName,iDrive,iSync);
#else
	if (gPrimaryExtensions[aDrive].iExists == EFalse)
		r=iFs.MountFileSystem(fsname,iDrive);
	else
		r=iFs.MountFileSystem(fsname,gPrimaryExtensions[iDrive].iName,iDrive);
#endif
	TEST(r==KErrNone);
	}

TInt TSoakRemote::TestSubSession()
///
/// Test what happens when a file is closed in the middle of writing it.  Note
/// that this assumes that the filesystem under test takes a second for each
/// write operation (i.e. is the special test filesystem).
///
	{
	TInt r = iFile.Replace(iFs, iName, EFileStreamText | EFileWrite);
	if (r != KErrNone)
		TTest::Fail(HERE, r, _L("opening %S for writing"), iName.Ptr());
	TInt i;
	for (i = 0; i < KSoakNumBuf; i++)
		{
		iFile.Write(iBuff[i], iStat[i]);
		}
	// wait for a couple of writes to complete, then close the file before the
	// others finish
	User::After(KSessionWaitTime);
	TTest::Printf(_L("Wait ended"));
	iFile.Close();
	TTest::Printf(_L("Close ended"));
	// test what has happened
	TBool bad = EFalse;
	for (i = 0; i < KSoakNumBuf; i++)
		{
		User::WaitForRequest(iStat[i]);
		r = iStat[i].Int();
		switch (r)
			{
			case KErrNone:
				if (i >= KSessionNumEnded)
					{
					TTest::Printf(_L("Write %d not cancelled"), i);
					bad = ETrue;
					}
				break;
			case KErrCancel:
				if (i <= KSessionNumEnded)
					{
					TTest::Printf(_L("Write %d incorrectly cancelled"), i);
					bad = ETrue;
					}
				TTest::Printf(_L("write %d cancelled\n"), i);
				break;
			default:
				TTest::Fail(HERE, r, _L("incorrect status for write %d"), i);
			}
		}
	iFs.Delete(iName);
	TPtrC sbuf(iSync ? _L("sync") : _L("async"));
	if (bad)
		{
		TTest::Printf(_L("TestSubSession %c: %S FAILED\n"), iName[0], &sbuf);
		// Commented out for the moment as result is undefined
		// TTest::Fail(HERE, _L("TestSubSession %c: %S FAILED\n"), iName[0], &sbuf);
		}
	TTest::Printf(_L("TestSubSession %c: %S OK\n"), iName[0], &sbuf);
	return KErrNone;
	}

TInt TSoakRemote::TestSession()
///
/// Test what happens when a session is closed in the middle of writing a file.
/// Note that this assumes that the filesystem under test takes a second for
/// each write operation (i.e. is the special test filesystem).
///
	{
	TInt r;
	TInt i;
	TPtrC sbuf(iSync ? _L("sync") : _L("async"));

	r = iFile.Replace(iFs, iName, EFileStreamText | EFileWrite);
	if (r != KErrNone)
		TTest::Fail(HERE, r, _L("opening %S for writing"), iName.Ptr());
	for (i = 0; i < KSoakNumBuf; i++)
		{
		iFile.Write(iBuff[i], iStat[i]);
		}
	// wait for a couple of them to complete, then close the session
	User::After(KSessionWaitTime);
	TTest::Printf(_L("Close FS %S"), &sbuf);
	// iFs.SetDebugRegister(KFSYS | KFSERV | KTHRD);
	iFs.Close();
	TTest::Printf(_L("FS closed %S"), &sbuf);
	// see what has happened to the writes (wait for at most another 10 seconds).
	TRequestStatus tstat;
	iTimer.After(tstat, 10*KSecond);
	TBool busy = ETrue;
	TInt  file = 0;
	while (tstat == KRequestPending && busy)
		{
		User::WaitForAnyRequest();
		busy = EFalse;
		for (i = file; i < KSoakNumBuf; i++)
			{
			r = iStat[i].Int();
			switch (r)
				{
				case KRequestPending:
					busy = ETrue;
					TTest::Printf(_L("write %d pending\n"), i);
					break;
				case KErrNone:
					file = i + 1;
					TTest::Printf(_L("write %d finished\n"), i);
					break;
				case KErrCancel:
					file = i + 1;
					TTest::Printf(_L("write %d cancelled\n"), i);
					break;
				default:
					file = i + 1;
					TTest::Fail(HERE, r, _L("incorrect status for write %d"), i);
				}
			}
		}
	TBool bad = EFalse;
	if (busy)
		{
		for (i = 0; i < KSoakNumBuf; i++)
			{
			TBuf<64> buf;
			r = iStat[i].Int();
			if (r != KErrNone)
				{
				// We expect that the third and subsequent requests will either
				// have failed with cancel or be still outstanding.  If either
				// of the first two have failed or are outstanding, that's an
				// error.
				if (i < KSessionNumEnded || (r != KErrCancel && r != KRequestPending))
					{
					TTest::Fail(HERE, _L("write %d: %S\n"), i, &TTest::ErrStr(r, buf));
					bad = ETrue;
					}
				}
			}
		}
	iTimer.Cancel();
	// if it's got this far, re-connect the file session
	// User::After(100000);
	// TTest::Printf(_L("FS closed (%S) yet?"), &sbuf);
	r = iFs.Connect();
	iFs.SetDebugRegister(0);
	// TTest::Printf(_L("FS opened"));
	if (r != KErrNone)
		TTest::Fail(HERE, r, _L("iFs connect"));
	r = iFs.Delete(iName);
	for (TInt nr = 0; r == KErrInUse && nr < 100; nr++)
		{
		TTest::Printf(_L("Wait for previous session to close"));
		User::After(10*KSecond);
		r = iFs.Delete(iName);
		}
	if (r == KErrNotFound)
		r = KErrNone;
	if (r != KErrNone)
		bad = ETrue;
	TPtrC obuf(bad ? _L("FAIL") : _L("OK"));
	TTest::Printf(_L("TestSession %c: %S %S\n"), iName[0], &sbuf, &obuf);
	if (bad)
		{
		TTest::Printf(_L("Test session close %c: %S FAILED\n"), iName[0], &sbuf);
		// Commented out for the moment as result is undefined
		// TTest::Fail(HERE, _L("Test session close failed"));
		}
	return r;
	}

TInt TSoakRemote::TestMount()
///
/// Test dismounting with a file open (should fail)
///
	{
	TFileName fsname;
	TInt r = iFs.FileSystemName(fsname, iDrive);
	TEST(r == KErrNone || r == KErrNotFound);

	TPtrC sbuf(iSync ? _L("sync") : _L("async"));

	r = iFile.Replace(iFs, iName, EFileStreamText | EFileWrite);
	for (TInt nr = 0; r == KErrInUse && nr < 100; nr++)
		{
		TTest::Printf(_L("Warning: KErrInUse opening %S for writing\n"), &iName);
		User::After(10*KSecond);
		r = iFile.Replace(iFs, iName, EFileStreamText | EFileWrite);
		}
	if (r != KErrNone)
		TTest::Fail(HERE, r, _L("opening %S for writing"), &iName);

	if (fsname.Length() > 0)
		{
		r = iFs.DismountFileSystem(fsname, iDrive);
		if (r == KErrNone)
			{
#ifdef __CONCURRENT_FILE_ACCESS__
			r = iFs.MountFileSystem(fsname, iDrive, iSync);
#else
			r = iFs.MountFileSystem(fsname, iDrive);
#endif
			if (r != KErrNone)
				TTest::Fail(HERE, r, _L("MountFileSystem(%S, %C:)"), &fsname, iDrvCh);
			}
		else if (r != KErrInUse)
			{
			TTest::Fail(HERE, r, _L("DismountFileSystem(%S, %C:)"), &fsname, iDrvCh);
			}
		}

	iFile.Close();
	iFs.Delete(iName);

	TTest::Printf(_L("TestMount %c: %S OK\n"), iName[0], &sbuf);

	return KErrNone;
	}


