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
// f32test\server\t_lock.cpp
// 
//

#define __E32TEST_EXTENSION__
#include <f32file.h>
#include <e32test.h>
#include "t_server.h"


class RFileTest : public RFile
	{
public:
	RFileTest(const TDesC& aName);
	RFileTest& Replace(const TDesC& aName);
	RFileTest& Open(const TDesC& aName);
	RFileTest& Lock(TInt aPos,TInt aLen=1);
	RFileTest& LockE(TInt aPos,TInt aLen=1);
	RFileTest& UnLock(TInt aPos,TInt aLen=1);
	RFileTest& UnLockE(TInt aPos,TInt aLen=1);
	RFileTest& LockEA(TInt aPos,TInt aLen=1);
	RFileTest& UnLockEA(TInt aPos,TInt aLen=1);
	RFileTest& Write(TInt aPos,TInt aLen=1);
	RFileTest& WriteE(TInt aPos,TInt aLen=1);
	RFileTest& Read(TInt aPos,TInt aLen=1);
	RFileTest& ReadE(TInt aPos,TInt aLen=1);
	RFileTest& Size(TInt aSize);
	RFileTest& SizeE(TInt aSize);
private:
	TName iName;
	};

GLDEF_D RTest test(_L("T_LOCK"));
LOCAL_D	RFileTest Test1(_L("File 1"));
LOCAL_D	RFileTest Test2(_L("File 2"));
LOCAL_D TBuf8<0x100> Pattern;
LOCAL_D TBuf8<0x100> Buffer;

LOCAL_C void DoFormat()
//
// Format the ramdisk
//
	{

	TInt count;
	RFormat format;
#if defined(__WINS__)
	TInt r=format.Open(TheFs,_L("Y:\\"),EHighDensity,count);
#else
	TInt r=format.Open(TheFs,_L("C:\\"),EHighDensity,count);
#endif
	test_KErrNone(r);
	while(count)
		{
		r=format.Next(count);
		test_KErrNone(r);
		}
	format.Close();
	}

LOCAL_C void MakeTestDirectory()
//
// Format the ramdisk if it is corrupt
//
	{

	TPtrC testDir=_L("\\F32-TST\\");
	TInt r=TheFs.MkDir(testDir);
	if (r==KErrNone || r==KErrAlreadyExists)
		return;
	test.Next(_L("Formatting disk"));
	DoFormat();
	r=TheFs.MkDir(testDir);
	test_KErrNone(r);
	}

RFileTest::RFileTest(const TDesC& aName)
//
// Constructor
//
	: iName(aName)
	{}

RFileTest& RFileTest::Replace(const TDesC& aName)
//
// Replace a file.
//
	{

	test.Printf(_L("%S replace %S\n"),&iName,&aName);
	TInt r=RFile::Replace(TheFs,aName,EFileStream|EFileWrite|EFileShareAny);
	test_KErrNone(r);
	return(*this);
	}

RFileTest& RFileTest::Open(const TDesC& aName)
//
// Open a file.
//
	{

	test.Printf(_L("%S open %S\n"),&iName,&aName);
	TInt r=RFile::Open(TheFs,aName,EFileStream|EFileWrite|EFileShareAny);
	test_KErrNone(r);
	return(*this);
	}

RFileTest& RFileTest::Lock(TInt aPos,TInt aLen)
//
// Set a lock on the file. Expected not to fail.
//
	{

	test.Printf(_L("%S lock   %08x-%08x\n"),&iName,aPos,aPos+aLen-1);
	TInt r=RFile::Lock(aPos,aLen);
	test_KErrNone(r);
	return(*this);
	}

RFileTest& RFileTest::LockE(TInt aPos,TInt aLen)
//
// Set a lock on the file. Expected to fail.
//
	{

	test.Printf(_L("%S lockE  %08x-%08x\n"),&iName,aPos,aPos+aLen-1);
	TInt r=RFile::Lock(aPos,aLen);
	test_Value(r, r == KErrLocked);
	return(*this);
	}

RFileTest& RFileTest::UnLock(TInt aPos,TInt aLen)
//
// Unlock the file. Expected not to fail.
//
	{

	test.Printf(_L("%S ulock  %08x-%08x\n"),&iName,aPos,aPos+aLen-1);
	TInt r=RFile::UnLock(aPos,aLen);
	test_KErrNone(r);
	return(*this);
	}

RFileTest& RFileTest::UnLockE(TInt aPos,TInt aLen)
//
// Unlock the file. Expected to fail.
//
	{

	test.Printf(_L("%S ulockE %08x-%08x\n"),&iName,aPos,aPos+aLen-1);
	TInt r=RFile::UnLock(aPos,aLen);
	test_Value(r, r == KErrNotFound);
	return(*this);
	}
 

RFileTest& RFileTest::LockEA(TInt aPos,TInt aLen)
//
// Set a lock on the file. Expected to fail with KErrArgument.
//
	{

	test.Printf(_L("%S lock   %08x-%08x\n"),&iName,aPos,aPos+aLen-1);
	TInt r=RFile::Lock(aPos,aLen);
	test_Value(r, r == KErrArgument);
	return(*this);
	}

RFileTest& RFileTest::UnLockEA(TInt aPos,TInt aLen)
//
// Unlock the file. Expected to fail with KErrArgument.
//
	{

	test.Printf(_L("%S ulock  %08x-%08x\n"),&iName,aPos,aPos+aLen-1);
	TInt r=RFile::UnLock(aPos,aLen);
	test_Value(r, r == KErrArgument);
	return(*this);
	}

RFileTest& RFileTest::Write(TInt aPos,TInt aLen)
//
// Write to the file. Expected not to fail.
//
	{

	test.Printf(_L("%S write  %08x-%08x\n"),&iName,aPos,aPos+aLen-1);
	TInt r=RFile::Write(aPos,Pattern,aLen);
	test_KErrNone(r);
	return(*this);
	}

RFileTest& RFileTest::WriteE(TInt aPos,TInt aLen)
//
// Write to the file. Expected to fail.
//
	{

	test.Printf(_L("%S writeE %08x-%08x\n"),&iName,aPos,aPos+aLen-1);
	TInt r=RFile::Write(aPos,Pattern,aLen);
	test_Value(r, r == KErrLocked);
	return(*this);
	}

RFileTest& RFileTest::Read(TInt aPos,TInt aLen)
//
// Read from the file. Expected not to fail.
//
	{

	test.Printf(_L("%S read   %08x-%08x\n"),&iName,aPos,aPos+aLen-1);
	TInt r=RFile::Read(aPos,Buffer,aLen);
	test_KErrNone(r);
	return(*this);
	}

RFileTest& RFileTest::ReadE(TInt aPos,TInt aLen)
//
// Read from the file. Expected to fail.
//
	{

	test.Printf(_L("%S readE  %08x-%08x\n"),&iName,aPos,aPos+aLen-1);
	TInt r=RFile::Read(aPos,Buffer,aLen);
	test_Value(r, r == KErrLocked);
	return(*this);
	}

RFileTest& RFileTest::Size(TInt aSize)
//
// Set the size of the file. Expected not to fail.
//
	{

	test.Printf(_L("%S size   %08x\n"),&iName,aSize);
	TInt r=RFile::SetSize(aSize);
	test_KErrNone(r);
	return(*this);
	}

RFileTest& RFileTest::SizeE(TInt aSize)
//
// Set the size of the file. Expected to fail.
//
	{

	test.Printf(_L("%S sizeE  %08x\n"),&iName,aSize);
	TInt r=RFile::SetSize(aSize);
	test_Value(r, r == KErrLocked);
	return(*this);
	}

LOCAL_C void setup()
//
// Create the test files.
//
	{

	test.Start(_L("Settting test environment"));
//
	Test1.Replace(_L("\\F32-TST\\LOCK.TST"));
	Test2.Open(_L("\\F32-TST\\LOCK.TST"));
//
	test.Next(_L("Creating test pattern"));
	Pattern.SetLength(Pattern.MaxLength());
	for (TInt i=0;i<Pattern.MaxLength();i++)
		Pattern[i]=(TText8)i;
//
	test.End();
	}

LOCAL_C void testLock1()
//
// Test file sharing.
//
	{

	test.Start(_L("Test locking 1"));
//
	test.Next(_L("Single file tests"));
	Test1.UnLockE(0).Lock(0).LockE(0).UnLock(0);
	Test1.Lock(0,0x100).Lock(0x100,0x100).Lock(0x200,0x100);
	Test1.UnLock(0x100,0x100).UnLock(0x200,0x100).UnLock(0,0x100);
	Test1.Lock(0,0x100).Lock(0x200,0x100).Lock(0x100,0x100);
	Test1.UnLock(0x200,0x100).UnLock(0x100,0x100).UnLock(0,0x100);
	Test1.Lock(0,0x100).Lock(0x200,0x100);
	Test1.LockE(0x100,0x101).LockE(0x180,0x100).LockE(0x80,0x100);
	Test1.Lock(0x400,0x100).LockE(0x180,0x400).LockE(0,0x400);
	Test1.UnLock(0x0,0x100).UnLock(0x200,0x100).UnLock(0x400,0x100);
	Test1.LockEA(0x40000000,0x40000002).LockEA(0x7FFFFFFC,0x10).LockEA(0x7FFFFFFF,0x100);
 	Test1.UnLockEA(0x40000000,0x40000001).UnLockEA(0x7FFFFFFF,0x100).UnLockEA(0x7FFFFFFE,0x05);
//
	test.Next(_L("Multi file tests"));
	Test1.Lock(0);
	Test2.LockE(0);
	Test1.UnLock(0);
	Test2.Lock(0);
	Test1.LockE(0);
	Test2.UnLock(0);
//
	test.End();
	}

GLDEF_C void CallTestsL()
//
// Do all tests
//
	{

	MakeTestDirectory();
	setup();
	testLock1();
	
	Test1.Close();
	Test2.Close();
	}
