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
// f32test\manager\t_oom.cpp
// 
//

#define	__E32TEST_EXTENSION__
#include <hal.h>
#include <f32file.h>
#include <e32test.h>
#include "../server/t_server.h"

GLDEF_D RTest test(_L("T_OOM"));

LOCAL_C void FormatFat()
//
// Call all RFormat methods
//
	{

	test.Next(_L("Format the disk"));
	RFormat format;
	TFileName sessionPath;
	TInt r=TheFs.SessionPath(sessionPath);
	test_KErrNone(r);
	TInt count;
	r=format.Open(TheFs,sessionPath,EHighDensity,count);
	test_KErrNone(r);
//	test(count==100);
//	TRequestStatus status;
//	TPckgBuf<TInt> step;
//	do 	{
//		format.Next(step,status);
//		User::WaitForRequest(status);
//		test(status==KErrNone);
//	} while (step()<count);
//	test(step()==count);
	const TInt total(count);
	while (count && r == KErrNone)
		{
		test((r = format.Next(count)) == KErrNone);
		test.Printf(_L("\r%d/%d"), count, total);
		}
	test.Printf(_L("\n"));
	format.Close();
	}

LOCAL_C void Test1()
//
// Test openning a large file
//
	{

	test.Next(_L("Create a file GOBBLE.DAT"));
	TUint size=0x340000; // 3.25MB
	
	test.Printf(_L("FileSize = 0x%x\n"),size);
	RFile file;
	TInt r=file.Replace(TheFs,_L("\\F32-TST\\GOBBLE.DAT"),EFileRead);
	test_KErrNone(r);
	r=file.SetSize(size);
	test_Value(r, r == KErrNone || r==KErrDiskFull);
	if (r==KErrDiskFull)
		{
		TFileName sessionPath;
		r=TheFs.SessionPath(sessionPath);
		test_KErrNone(r);
		test.Printf(_L("Error %S diskfull\n"),&sessionPath);
// Reintroduce when we can detect that the test is being run manually
//		test.Getch();
		}
	file.Close();
	}

/** if internal RAM drive has a defined limit, test cannot be exceeded */

LOCAL_C void TestRAMDriveLimit()
	{
	test.Start(_L("TestRAMDriveLimit"));

	TInt r;										// error code

	for (TInt d = EDriveA; d <= EDriveZ; ++d)
		{
		TDriveInfo di;
		test((r = TheFs.Drive(di, d)) == KErrNone);
		TInt maxRam;

		if (di.iType == EMediaRam && HAL::Get(HAL::EMaxRAMDriveSize, maxRam) == KErrNone)
			{
#ifdef __WINS__									// c: not EMediaRam on WINS
			if (d != EDriveY)
				continue;
#endif

			test.Printf(_L("Testing RAM drive limit %08x on drive %x\n"), maxRam, d);

			// create lots of files and check KErrDiskFull after right number

			const TInt KFileSize = 16 * 1024;

			_LIT(KDrvTmp, "?:\\");				// set session path to limited drive
			TBuf<3> bfDrv(KDrvTmp);
			TChar ch;
			test(RFs::DriveToChar(d, ch) == KErrNone);
			bfDrv[0] = static_cast<TText>(ch);
			test.Printf(_L("Setting session path to \"%S\".\n"), &bfDrv);
			test(TheFs.SetSessionPath(bfDrv) == KErrNone);

			FormatFat();						// remove all current files from drive

			TBuf<3 + 2> bfDir;					// subdir to avoid root KErrDirFull
			bfDir.Append(bfDrv);
			_LIT(KTstDir, "t\\");
			bfDir.Append(KTstDir);
			test.Printf(_L("creating directory \"%S\".\n"), &bfDir);
			r = TheFs.MkDir(bfDir);
			test_KErrNone(r);

			TBuf<3 + 3 + 8 + 1 + 3> bfFlNm(bfDir);
			TInt ctr = 0;						// create files until KErrDiskFull
			do
				{
				bfFlNm.SetLength(bfDir.Length());
				bfFlNm.AppendFormat(_L("%08x.dat"), ctr);

				test.Printf(
					_L("\rcreating %S @ %08x (total %08x)"),
					&bfFlNm, KFileSize, ctr * KFileSize);

				RFile f;
				r = f.Create(TheFs, bfFlNm, EFileShareExclusive | EFileStream | EFileWrite);
				test_Value(r, r == KErrNone || r == KErrDiskFull);
				if (r == KErrNone)
					{
					r = f.SetSize(KFileSize);
					test_Value(r, r == KErrNone || r == KErrDiskFull);
					}
				f.Close();

				++ctr;
				} while (r != KErrDiskFull);
			test.Printf(_L("\n"));

			// new file takes KFileSize, any possibly metadata cluster

			TVolumeInfo vi;
			test(TheFs.Volume(vi, d) == KErrNone);
			test(vi.iSize < maxRam);			// vi.iSize does not include FAT
			test(vi.iFree < 2 * KFileSize);

			FormatFat();

			// create single file and set to > maxRam

			RFile fS;
			_LIT(bfFlNmS, "00000000.dat");
			test(fS.Create(TheFs, bfFlNmS, EFileShareExclusive | EFileStream | EFileWrite) == KErrNone);
			test(fS.SetSize(maxRam) == KErrDiskFull);
			fS.Close();
			}
		}	// for (TInt d = EDriveA; d <= EDriveZ; ++d)

	test.End();
	}

GLDEF_C void CallTestsL()
//
// Call all tests
//
	{
	test.Title();
	test.Start(_L("Starting T_OOM test"));

	TDriveInfo driveInfo;
	TInt r=TheFs.Drive(driveInfo);
	test_KErrNone(r);
	if (driveInfo.iType==EMediaNotPresent)
		{
		test.Printf(_L("ERROR: MEDIA NOT PRESENT\n"));
// Reintroduce when we can detect that the test is being run manually
//		test.Getch();
		return;
		}

	TFileName sessionPath;
	r=TheFs.SessionPath(sessionPath);
	test_KErrNone(r);
	r=TheFs.MkDirAll(sessionPath);
	test_Value(r, r == KErrCorrupt || r==KErrAlreadyExists || r==KErrNone);
	if (r==KErrCorrupt)
		FormatFat();
	if (r==KErrAlreadyExists)
		{
		test.Next(_L("Remove test directory"));
		CFileMan* fman=CFileMan::NewL(TheFs);
		TInt ret=fman->RmDir(sessionPath);
		test_KErrNone(ret);
		delete fman;
		}
	if (r!=KErrNone)
		{
		r=TheFs.MkDirAll(sessionPath);
		test_KErrNone(r);
		}

	Test1();

	TestRAMDriveLimit();
	test.End();
	test.Close();
	}
