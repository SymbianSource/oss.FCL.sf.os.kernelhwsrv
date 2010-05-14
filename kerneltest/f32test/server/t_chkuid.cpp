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
// f32test\server\t_chkuid.cpp
//
//

#define __E32TEST_EXTENSION__
#include <f32file.h>
#include <e32test.h>
#include "t_server.h"
#include "t_chlffs.h"

#if defined(__WINS__)
#define WIN32_LEAN_AND_MEAN
#pragma warning (disable:4201) // warning C4201: nonstandard extension used : nameless struct/union
#pragma warning (default:4201) // warning C4201: nonstandard extension used : nameless struct/union
#endif

GLDEF_D RTest test(_L("T_CHKUID"));


LOCAL_C void CreateUidTestFiles()
//
// Create files with uids for testing
//
	{
    // Create \\gSessionPath\\UIDCHKNO.SHT - no uid, zero length
	RFile file;
	TInt r=file.Replace(TheFs,_L("UIDCHKNO.SHT"),EFileRead|EFileWrite);
	test_KErrNone(r);
	file.Close();

    // Create \\gSessionPath\\UIDCHKNO.LNG - no uid, long length
	r=file.Replace(TheFs,_L("UIDCHKNO.LNG"),EFileRead|EFileWrite);
	test_KErrNone(r);
	r=file.Write(_L8("Hello World needs to be over 16 bytes"));
	file.Close();

    // Create \\gSessionPath\\UIDCHK.BLG - with uid no data
	r=file.Replace(TheFs,_L("UIDCHK.BLG"),EFileRead|EFileWrite);
	test_KErrNone(r);
	TUidType uidType(TUid::Uid('U'),TUid::Uid('I'),TUid::Uid('D'));
	TCheckedUid checkedUid(uidType);
	TPtrC8 buf((TUint8*)&checkedUid,sizeof(TCheckedUid));
	r=file.Write(buf);
	test_KErrNone(r);
	file.Close();

    // Create \\gSessionPath\\UIDCHK.MSG - with uid and data
	r=file.Replace(TheFs,_L("UIDCHK.MSG"),EFileRead|EFileWrite);
	test_KErrNone(r);
	TUidType uidType2(TUid::Uid('X'),TUid::Uid('Y'),TUid::Uid('Z'));
	checkedUid.Set(uidType2);
	buf.Set((TUint8*)&checkedUid,sizeof(TCheckedUid));
	r=file.Write(buf);
	test_KErrNone(r);
	r=file.Write(_L8("More file data"));
	test_KErrNone(r);
	file.Close();

    // Create \\gSessionPath\\UIDCHK.DAT - uid stored only in the file
	r=file.Replace(TheFs,_L("UIDCHK.DAT"),EFileRead|EFileWrite);
	test_KErrNone(r);
	TUidType uidType3(TUid::Uid('D'),TUid::Uid('A'),TUid::Uid('T'));
	checkedUid.Set(uidType3);
	buf.Set((TUint8*)&checkedUid,sizeof(TCheckedUid));
	r=file.Write(buf);
	test_KErrNone(r);
	r=file.Write(_L8("More file data"));
	test_KErrNone(r);
	file.Close();

    // Create \\gSessionPath\\UIDCHK.PE - uid stored in WINS PE file header
	r=file.Replace(TheFs,_L("UIDWINS.PE"),EFileRead|EFileWrite);
	test_KErrNone(r);

#if defined(__WINS__)
    if (!IsTestingLFFS())
        {
	    RFile fileSource;
	    r=fileSource.Open(TheFs,_L("Z:\\TEST\\T_CHKUID.EXE"),EFileShareReadersOnly|EFileRead);
	    test_KErrNone(r);

	    TBuf8<0x100> buffer;
	    do
		    {
		    r=fileSource.Read(buffer);
		    test_KErrNone(r);
		    r=file.Write(buffer);
		    test_KErrNone(r);
		    }
	    while (buffer.Length()==buffer.MaxLength());

	    fileSource.Close();
        }
    else
        {
	    r=file.Write(_L8("Some zany stuff here!"));
	    test_KErrNone(r);
        }
#else
	r=file.Write(_L8("Some zany stuff here!"));
	test_KErrNone(r);
#endif
	file.Close();
	}

LOCAL_C void Test1()
//
// Test GetDir
//
	{

	test.Next(_L("Use GetDir to check files"));
	CDir* dum=NULL;
	TInt r=TheFs.GetDir(_L("UID*"),KEntryAttAllowUid,ESortByName,dum);
	CDir& dir=*dum;
	test_KErrNone(r);
	TInt count=dir.Count();
	test(count==6);

	TEntry entry=dir[0];
	test(entry.iName==_L("UIDCHK.BLG"));
	test(entry.IsTypeValid());
	test(entry.iType[0]==TUid::Uid('U') && entry.iType[1]==TUid::Uid('I') && entry.iType[2]==TUid::Uid('D'));

	entry=dir[1];
	test(entry.iName==_L("UIDCHK.DAT"));
	test(entry.IsTypeValid());
	test(entry.iType[0]==TUid::Uid('D') && entry.iType[1]==TUid::Uid('A') && entry.iType[2]==TUid::Uid('T'));

	entry=dir[2];
	test(entry.iName==_L("UIDCHK.MSG"));
	test(entry.IsTypeValid());
	test(entry.iType[0]==TUid::Uid('X') && entry.iType[1]==TUid::Uid('Y') && entry.iType[2]==TUid::Uid('Z'));

	entry=dir[3];
	test(entry.iName==_L("UIDCHKNO.LNG"));
	test(entry.IsTypeValid()==EFalse);

	entry=dir[4];
	test(entry.iName==_L("UIDCHKNO.SHT"));
	test(entry.IsTypeValid()==EFalse);

	entry=dir[5];
	test(entry.iName==_L("UIDWINS.PE"));
#if defined(__WINS__)
	TFileName sessionPath;
	TheFs.SessionPath(sessionPath);
	if (sessionPath[0]!='C')
		test(entry.IsTypeValid()==EFalse);
	else
		{
		test(entry.IsTypeValid());
		test(entry.iType[0]==TUid::Uid(0x1000007a) && entry.iType[1]==TUid::Uid(2) && entry.iType[2]==TUid::Uid(3));
		}
#else
	test(entry.IsTypeValid()==EFalse);
#endif
	delete dum;
	}

LOCAL_C void Test2()
//
// Test GetDir
//
	{

	test.Next(_L("Test KEntryAttAllowUid allows uids"));
	CDir* dum=NULL;
	TInt r=TheFs.GetDir(_L("UID*"),0,ESortByName,dum);
	CDir& dir=*dum;
	test_KErrNone(r);
	TInt count=dir.Count();
	test(count==6);

	TEntry entry=dir[0];
	test(entry.iName==_L("UIDCHK.BLG"));
	test(!entry.IsTypeValid());

	entry=dir[1];
	test(entry.iName==_L("UIDCHK.DAT"));
	test(!entry.IsTypeValid());

	entry=dir[2];
	test(entry.iName==_L("UIDCHK.MSG"));
	test(!entry.IsTypeValid());

	entry=dir[3];
	test(entry.iName==_L("UIDCHKNO.LNG"));
	test(entry.IsTypeValid()==EFalse);

	entry=dir[4];
	test(entry.iName==_L("UIDCHKNO.SHT"));
	test(entry.IsTypeValid()==EFalse);

	entry=dir[5];
	test(entry.iName==_L("UIDWINS.PE"));
	test(entry.IsTypeValid()==EFalse);
	delete dum;
	}

LOCAL_C void Test3()
//
// Test RFs::Entry()
//
	{

	test.Next(_L("Use RFs::EntryL() to check files"));
	TEntry entry;
	TInt r=TheFs.Entry(_L("UIDCHKNO.SHT"),entry);
	test_KErrNone(r);
	test(entry.iName==_L("UIDCHKNO.SHT"));
	test(entry.IsTypeValid()==EFalse);

	r=TheFs.Entry(_L("UIDCHKNO.LNG"),entry);
	test_KErrNone(r);
	test(entry.iName==_L("UIDCHKNO.LNG"));
	test(entry.IsTypeValid()==EFalse);

	r=TheFs.Entry(_L("UIDCHK.MSG"),entry);
	test_KErrNone(r);
	test(entry.iName==_L("UIDCHK.MSG"));
	test(entry.IsTypeValid());
	test(entry.iType[0]==TUid::Uid('X') && entry.iType[1]==TUid::Uid('Y') && entry.iType[2]==TUid::Uid('Z'));

	r=TheFs.Entry(_L("UIDCHK.BLG"),entry);
	test_KErrNone(r);
	test(entry.iName==_L("UIDCHK.BLG"));
	test(entry.IsTypeValid());
	test(entry.iType[0]==TUid::Uid('U') && entry.iType[1]==TUid::Uid('I') && entry.iType[2]==TUid::Uid('D'));

	r=TheFs.Entry(_L("UIDCHK.DAT"),entry);
	test_KErrNone(r);
	test(entry.iName==_L("UIDCHK.DAT"));
	test(entry.IsTypeValid());
	test(entry.iType[0]==TUid::Uid('D') && entry.iType[1]==TUid::Uid('A') && entry.iType[2]==TUid::Uid('T'));

	r=TheFs.Entry(_L("UIDWINS.PE"),entry);
	test_KErrNone(r);
	test(entry.iName==_L("UIDWINS.PE"));
#if defined(__WINS__)
	TFileName sessionPath;
	TheFs.SessionPath(sessionPath);
	if (sessionPath[0]!='C')
		test(entry.IsTypeValid()==EFalse);
	else
		{
		test(entry.IsTypeValid());
		test(entry.iType[0]==TUid::Uid(0x1000007a) && entry.iType[1]==TUid::Uid(2) && entry.iType[2]==TUid::Uid(3));
		}
#else
	test(entry.IsTypeValid()==EFalse);
#endif
	}

LOCAL_C void Test4()
//
// Test uid's can be read when the file is open
//
//	EFileShareExclusive,EFileShareReadersOnly,EFileShareAny,
//	EFileStream=0,EFileStreamText=0x100,
//	EFileRead=0,EFileWrite=0x200
//
	{

	test.Next(_L("Uids can be read if the file is open"));
	RFile f;
	TEntry entry;
	TInt r=f.Open(TheFs,_L("UIDCHK.DAT"),EFileShareExclusive|EFileRead);
	test_KErrNone(r);
	r=TheFs.Entry(_L("UIDCHK.DAT"),entry);
	test_KErrNone(r);
	test(entry.iName==_L("UIDCHK.DAT"));
	test(entry.IsTypeValid());
	test(entry.iType[0]==TUid::Uid('D') && entry.iType[1]==TUid::Uid('A') && entry.iType[2]==TUid::Uid('T'));
	f.Close();

	r=f.Open(TheFs,_L("UIDCHK.DAT"),EFileShareExclusive|EFileWrite);
	test_KErrNone(r);
	r=TheFs.Entry(_L("UIDCHK.DAT"),entry);
	test_KErrNone(r);
	test(entry.iName==_L("UIDCHK.DAT"));
	test(entry.IsTypeValid());
	test(entry.iType[0]==TUid::Uid('D') && entry.iType[1]==TUid::Uid('A') && entry.iType[2]==TUid::Uid('T'));

	r=f.SetSize(256);
	test_KErrNone(r);
	TBuf8<16> des;
	r=TheFs.ReadFileSection(_L("UIDCHK.DAT"),0,des,16);
	test_KErrNone(r);

	f.Close();

	r=f.Open(TheFs,_L("UIDCHK.DAT"),EFileShareReadersOnly|EFileRead);
	test_KErrNone(r);
	r=TheFs.Entry(_L("UIDCHK.DAT"),entry);
	test_KErrNone(r);
	test(entry.iName==_L("UIDCHK.DAT"));
	test(entry.IsTypeValid());
	test(entry.iType[0]==TUid::Uid('D') && entry.iType[1]==TUid::Uid('A') && entry.iType[2]==TUid::Uid('T'));
	f.Close();

//	EFileShareReadersOnly|EFileWrite is illegal

	r=f.Open(TheFs,_L("UIDCHK.DAT"),EFileShareAny|EFileRead);
	test_KErrNone(r);
	r=TheFs.Entry(_L("UIDCHK.DAT"),entry);
	test_KErrNone(r);
	test(entry.iName==_L("UIDCHK.DAT"));
	test(entry.IsTypeValid());
	test(entry.iType[0]==TUid::Uid('D') && entry.iType[1]==TUid::Uid('A') && entry.iType[2]==TUid::Uid('T'));
	f.Close();

	r=f.Open(TheFs,_L("UIDCHK.DAT"),EFileShareAny|EFileWrite);
	test_KErrNone(r);

	RFile secondFile;
	r=secondFile.Open(TheFs,_L("UIDCHK.DAT"),EFileShareAny|EFileWrite);
	test_KErrNone(r);

	RFile thirdFile;
	r=thirdFile.Open(TheFs,_L("UIDCHK.DAT"),EFileShareAny|EFileRead);
	test_KErrNone(r);

	r=TheFs.Entry(_L("UIDCHK.DAT"),entry);
	test_KErrNone(r);
	test(entry.iName==_L("UIDCHK.DAT"));
	test(entry.IsTypeValid());
	test(entry.iType[0]==TUid::Uid('D') && entry.iType[1]==TUid::Uid('A') && entry.iType[2]==TUid::Uid('T'));
	f.Close();
	secondFile.Close();
	thirdFile.Close();

	r=f.Open(TheFs,_L("UIDWINS.PE"),EFileShareAny|EFileWrite);
	test_KErrNone(r);

	r=TheFs.Entry(_L("UIDWINS.PE"),entry);
	test_KErrNone(r);
	test(entry.iName==_L("UIDWINS.PE"));
#if defined(__WINS__)
	TFileName sessionPath;
	TheFs.SessionPath(sessionPath);
	if (sessionPath[0]!='C')
		test(entry.IsTypeValid()==EFalse);
	else
		{
		test(entry.IsTypeValid());
		test(entry.iType[0]==TUid::Uid(0x1000007a) && entry.iType[1]==TUid::Uid(2) && entry.iType[2]==TUid::Uid(3));
		}
#else
	test(entry.IsTypeValid()==EFalse);
#endif
	f.Close();
	}

LOCAL_C void TestZ()
//
// Test Rom filesystem
//
	{

	CDir* dum=NULL;

	TInt r=TheFs.GetDir(PlatSec::ConfigSetting(PlatSec::EPlatSecEnforceSysBin)?_L("Z:\\Sys\\Bin\\*"):_L("Z:\\System\\Bin\\*"),KEntryAttAllowUid,0,dum);

	if (r==KErrNotReady)
		{
		test.Printf(_L("Error: Unable to open Z:\n"));
		return;
		}
	test_KErrNone(r);
	CDir& dir=*dum;
	TInt count=dir.Count();
	if (count==0)
		test.Printf(_L("No files present on Z:\\*\n"));
	while (count--)
		{
		TBuf<32> UID;
		if (dir[count].IsTypeValid()==EFalse)
			UID=_L("INVALID");
		else
			{
			UID=dir[count].iType[0].Name();
			UID.Append(dir[count].iType[1].Name());
			UID.Append(dir[count].iType[2].Name());
			}
		test.Printf(_L("FILE: %S UID %S\n"),&dir[count].iName,&UID);
		}
	delete &dir;
	TEntry entry;
	r=TheFs.Entry(PlatSec::ConfigSetting(PlatSec::EPlatSecEnforceSysBin)?_L("Z:\\Sys\\Bin\\ELOCAL.FSY"):_L("Z:\\System\\Bin\\ELOCAL.FSY"),entry);
	}

GLDEF_C void CallTestsL(void)
//
// Do all tests
//
	{

	TBuf<64> b;

	TFileName sessionPath;

	TInt r=TheFs.SessionPath(sessionPath);
	test_KErrNone(r);
	TChar driveLetter=sessionPath[0];
	b.Format(_L("Testing filesystem on %c:"),(TText)driveLetter);
	test.Next(b);

	CreateUidTestFiles();
	Test1();
	Test2();
	Test3();
	Test4();

	test.Next(_L("Testing filesystem on Z:"));
	TRAP(r,TestZ());
	if (r!=KErrNone)
		test.Printf(_L("Error: %d\n"),r);
	}
