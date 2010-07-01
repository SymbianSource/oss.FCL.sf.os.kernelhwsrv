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
// Based on t_main.cpp as used for other tests.
// 
//

//! @file f32test\concur\t_cfsmain.cpp

#include <f32file.h>
#include <e32test.h>
#include <e32hal.h>
#include <e32math.h>
#include <f32dbg.h>

#include "t_server.h"
#include "cfafsdlyif.h"

GLDEF_D	RFs TheFs;
GLDEF_D TFileName gSessionPath;
GLDEF_D TInt gAllocFailOff=KAllocFailureOff;
GLDEF_D TInt gAllocFailOn=KAllocFailureOff;
GLDEF_D TInt64 gSeed=51703;

GLDEF_D TChar   gDriveToTest;
GLDEF_D TUint32 gDebugFlags = 0;

GLDEF_D TPtrC gArgV[128];
GLDEF_D TInt  gArgC = 0;

LOCAL_D TTestType TheTestType=ENotifierNone;
LOCAL_D RSemaphore ControlIoSem;

TMediaPassword thePassword=_L8("abc");

enum TTestCode {ESimLockRemMed, EClearSimLockRemMed};

const TInt KControlIoRemMedLock=5;						//setflag
const TInt KControlClearMedLock=6;						//clears flag
const TInt KControIoRemMedRepeat=9;

_LIT(KNotifierHang,"Hang");
_LIT(KNotifierRepeat,"Repeat");
_LIT(KNotifierWithRepeat,"WithRepeat");	
_LIT(KPrivate, "\\Private\\");

GLDEF_C TBool IsTestTypeNotifyHang()
//
//
//
	{
	return(TheTestType==ENotifierHang);
	}

GLDEF_C TBool IsTestTypeNotifyRepeat()
//
//
//
	{
	return(TheTestType==ENotifierRepeat);
	}

GLDEF_C TBool IsTestTypeNotifyWithRepeat()
//
//
//
	{
	return(TheTestType==ENotifierWithRepeat);
	}


GLDEF_C TBool IsTestTypeStandard()
//
//
//
	{
	return(TheTestType==ENotifierNone);
	}

GLDEF_C TTestType TestType()
//
//
//
	{
	return(TheTestType);
	}

GLDEF_C void CreateShortName(TDes& aFileName,TInt64& aSeed)
//
// Create a random, dos legal 8.3 char name
//
	{

	TInt length=Math::Rand(aSeed)%11;
	if (length==0)
		length=1;
	else if (length==3)	// don't create three letter names like 'AUX' or 'PRN'
		length++;
	else if (length>8)	// end in '.' if no extension
		length++;

	aFileName.SetLength(length);
	for(TInt i=0;i<length;i++)
		{
		if (i==9)
			{
			aFileName[i]='.';
			continue;
			}
		TInt letter=Math::Rand(aSeed)%26;
		aFileName[i]=(TText)('A'+letter);
		}
	}

_LIT(KFatName,"Fat");
GLDEF_C TBool IsFileSystemFAT(RFs &aFsSession,TInt aDrive)
//
// return true if fat on aDrive
//
	{
	TFileName f;
	TInt r=aFsSession.FileSystemName(f,aDrive);
	test_Value(r, r == KErrNone || r==KErrNotFound);
	return (f.CompareF(KFatName)==0);
	}

GLDEF_C void CreateLongName(TDes& aFileName,TInt64& aSeed,TInt aLength)
//
// Create a random, dos legal 8.3 char name
//
	{

	TInt length;
	if (aLength>0)
		length=aLength;
	else
		{
		length=Math::Rand(aSeed)%128;
		length+=Math::Rand(aSeed)%128;
		length+=Math::Rand(aSeed)%128;
		length+=Math::Rand(aSeed)%128;
		length-=256;
			length=Abs(length);
		if (length==0)
			length=1;
		if (length>220)
			length=31;
		}
	if (length==3)	// don't create three letter names like 'AUX' or 'PRN'
		length++;

	aFileName.SetLength(length);
	TInt spaceChar=-1;
	TInt i;
	for(i=0;i<length;i++)
		{
StartAgain:
		TChar letter=0;
		TBool illegalChar=ETrue;

		while(illegalChar)
			{
#if defined(__WINS__)
			if (gSessionPath[0]=='C')
				letter=(TChar)('A'+Math::Rand(aSeed)%26);
			else
				letter=(TChar)Math::Rand(aSeed)%256;
#else
			letter=(TChar)Math::Rand(aSeed)%256;
#endif
			TBool space=letter.IsSpace();
			if (space && spaceChar==-1)
				spaceChar=i;
			else if (!space && spaceChar!=-1)
				spaceChar=-1;

			switch(letter)
				{
			case '<':
			case '>':
			case ':':
			case '"':
			case '/':
			case '|':
			case '*':
			case '?':
			case '\\':
			case '\0':
				break;
			default:
				illegalChar=EFalse;
				};
			}
		aFileName[i]=(TText)letter;
		}

	if (spaceChar!=-1)
		{
		i=spaceChar;
		goto StartAgain;
		}
	}

GLDEF_C void CheckEntry(const TDesC& aName,TUint anAttributes,const TTime& aModified)
//
// Checks the values associated with an entry
//
	{

	TEntry entry;
	TInt r=TheFs.Entry(aName,entry);
	test_KErrNone(r);
	test(entry.iAtt==anAttributes);
	if (aModified!=TTime(0))
		test(entry.iModified==aModified);
	}

GLDEF_C void CheckDisk()
//
// Do a checkdisk and report failure
//
	{
	test.Next(_L("Check Disk"));
	TInt r=TheFs.CheckDisk(gSessionPath);
	if (r!=KErrNone && r!=KErrNotSupported && r!=KErrPermissionDenied)
		ReportCheckDiskFailure(r);
	}

GLDEF_C void ReportCheckDiskFailure(TInt aRet)
//
// Report the failure of checkdisk
//
	{

	test.Printf(_L("CHECKDISK FAILED: "));
	switch(aRet)
		{
	case 1:	test.Printf(_L("File cluster chain contains a bad value (<2 or >maxCluster)\n")); break;
	case 2:	test.Printf(_L("Two files are linked to the same cluster\n")); break;
	case 3:	test.Printf(_L("Unallocated cluster contains a value != 0\n"));	break;
	case 4:	test.Printf(_L("Size of file != number of clusters in chain\n")); break;
	default: test.Printf(_L("Undefined Error value %d\n"),aRet);
		}
	test(EFalse);
	}

GLDEF_C void TurnAllocFailureOff()
//
// Switch off all allocFailure
//
	{

	test.Printf(_L("Disable Alloc Failure\n"));
	TheFs.SetAllocFailure(gAllocFailOff);
	gAllocFailOn=KAllocFailureOff; // Disable gAllocFailOn
	}

GLDEF_C void TurnAllocFailureOn()
//
// Switch off all allocFailure
//
	{

	test.Printf(_L("Enable Alloc Failure\n"));
	gAllocFailOn=KAllocFailureOn; // Enable gAllocFailOn
	TheFs.SetAllocFailure(gAllocFailOn);
	}

GLDEF_C void MakeFile(const TDesC& aFileName,const TUidType& aUidType,const TDesC8& aFileContents)
//
// Make a file and write uid and data
//
	{

	RFile file;
	TInt r=file.Replace(TheFs,aFileName,0);
	test_Value(r, r == KErrNone || r==KErrPathNotFound);
	if (r==KErrPathNotFound)
		{
		r=TheFs.MkDirAll(aFileName);
		test_KErrNone(r);
		r=file.Replace(TheFs,aFileName,0);
		test_KErrNone(r);
		}
	TCheckedUid checkedUid(aUidType);
	TPtrC8 uidData((TUint8*)&checkedUid,sizeof(TCheckedUid));
	r=file.Write(uidData);
	test_KErrNone(r);
	r=file.Write(aFileContents);
	test_KErrNone(r);
	file.Close();
	}

GLDEF_C void MakeFile(const TDesC& aFileName,const TDesC8& aFileContents)
//
// Make a file and write something in it
//
	{

	RFile file;
	TInt r=file.Replace(TheFs,aFileName,0);
	if(r!=KErrNone && r!=KErrPathNotFound)
		{
		test.Printf(_L("ERROR: r=%d"),r);
		test(EFalse);
		}
	test_Value(r, r == KErrNone || r==KErrPathNotFound);
	if (r==KErrPathNotFound)
		{
		r=TheFs.MkDirAll(aFileName);
		test_KErrNone(r);
		r=file.Replace(TheFs,aFileName,0);
		test_KErrNone(r);
		}
	r=file.Write(aFileContents);
	test_KErrNone(r);
	file.Close();
	}

GLDEF_C void MakeFile(const TDesC& aFileName,TInt anAttributes)
//
// Make a file and write something in it
//
	{

	RFile file;
	TInt r=file.Replace(TheFs,aFileName,0);
	test_Value(r, r == KErrNone || r==KErrPathNotFound);
	if (r==KErrPathNotFound)
		{
		r=TheFs.MkDirAll(aFileName);
		test_KErrNone(r);
		r=file.Replace(TheFs,aFileName,0);
		test_KErrNone(r);
		}
	file.Close();
	r=TheFs.SetAtt(aFileName,anAttributes,0);
	test_KErrNone(r);
	}

GLDEF_C void SetSessionPath(const TDesC& aPathName)
//
// Set the session path and update gSessionPath
//
	{

	TInt r=TheFs.SetSessionPath(aPathName);
	test_KErrNone(r);
	r=TheFs.SessionPath(gSessionPath);
	test_KErrNone(r);
	}

GLDEF_C void MakeFile(const TDesC& aFileName)
//
// Make a file
//
	{
	
	MakeFile(aFileName,_L8(""));
	}

GLDEF_C void MakeDir(const TDesC& aDirName)
//
// Make a directory
//
	{

	TInt r=TheFs.MkDirAll(aDirName);
	test_Value(r, r == KErrNone || r==KErrAlreadyExists);
	}

GLDEF_C TInt CheckFileExists(const TDesC& aName,TInt aResult,TBool aCompRes/*=ETrue*/)
//
// Check aName exists
//
	{

	TEntry entry;
	TInt r=TheFs.Entry(aName,entry);
	test_Value(r, r == aResult);
	if (aResult!=KErrNone)
		return(0);
	TParsePtrC nameParse(aName);
	TParsePtrC entryParse(entry.iName);
	TBool nameMatch=(entryParse.Name()==nameParse.Name());
	TBool extMatch=(entryParse.Ext()==nameParse.Ext()) || (entryParse.Ext().Length()<=1 && nameParse.Ext().Length()<=1);
	test((nameMatch && extMatch)==aCompRes);
	return(entry.iSize);
	}

GLDEF_C void CheckFileContents(const TDesC& aName,const TDesC8& aContents)
//
// Check contents of file
//
	{

	RFile f;
	TInt r=f.Open(TheFs,aName,EFileRead);
	test_KErrNone(r);
	HBufC8* testBuf=HBufC8::NewL(aContents.Length());
	test(testBuf!=NULL);
	TPtr8 bufPtr(testBuf->Des());
	r=f.Read(bufPtr);
	test_KErrNone(r);
	test(bufPtr==aContents);
	r=f.Read(bufPtr);
	test_KErrNone(r);
	test(bufPtr.Length()==0);
	f.Close();
	User::Free(testBuf);
	}

GLDEF_C void DeleteTestDirectory()
//
// Delete the leaf session path directory
//
	{

	TheFs.SetAtt(_L("\\F32-TST\\SCANTEST\\Left\\Dir3\\Dir4\\Hidden"), 0, KEntryAttHidden);
	TheFs.SetAtt(_L("\\F32-TST\\SCANTEST\\Left\\Dir3\\Dir4\\Hidden\\HiddenFile"), 0, KEntryAttHidden);
	TheFs.SetAtt(_L("\\F32-TST\\SCANTEST\\Left\\Dir3\\Dir4\\Hidden\\System"), 0, KEntryAttSystem);
	test.Next(_L("Delete test directory"));
	CFileMan* fMan=CFileMan::NewL(TheFs);
	test(fMan!=NULL);
	TInt r=TheFs.SessionPath(gSessionPath);
	test_KErrNone(r);
	r=TheFs.CheckDisk(gSessionPath);
	if (r!=KErrNone && r!=KErrNotSupported)
		ReportCheckDiskFailure(r);
	r=fMan->RmDir(gSessionPath);
	test_KErrNone(r);
	delete fMan;
	}

GLDEF_C void CreateTestDirectory(const TDesC& aSessionPath)
//
// Create directory for test
//
	{
	TParsePtrC path(aSessionPath);
	test(path.DrivePresent()==EFalse);

	TInt r=TheFs.SetSessionPath(aSessionPath);
	test_KErrNone(r);
	r=TheFs.SessionPath(gSessionPath);
	test_KErrNone(r);
	r=TheFs.MkDirAll(gSessionPath);
	test_Value(r, r == KErrNone || r==KErrAlreadyExists);
	}

GLDEF_C TInt CurrentDrive()
//
// Return the current drive number
//
	{

	TInt driveNum;
	TInt r=TheFs.CharToDrive(gSessionPath[0],driveNum);
	test_KErrNone(r);
	return(driveNum);
	}

GLDEF_C void Format(TInt aDrive)
//
// Format current drive
//
	{

	test.Next(_L("Format"));
	TBuf<4> driveBuf=_L("?:\\");
	driveBuf[0]=(TText)(aDrive+'A');
	RFormat format;
	TInt count;
	TInt r=format.Open(TheFs,driveBuf,EHighDensity,count);
	test_KErrNone(r);
	while(count)
		{
		TInt r=format.Next(count);
		test_KErrNone(r);
		}
	format.Close();
	}

LOCAL_C void PushLotsL()
//
// Expand the cleanup stack
//
	{
	TInt i;
	for(i=0;i<1000;i++)
		CleanupStack::PushL((CBase*)NULL);
	CleanupStack::Pop(1000);
	}


LOCAL_C void DoTests(TInt aDrive)
//
// Do testing on aDrive
//
	{

	gSessionPath=_L("?:\\F32-TST\\");
	TChar driveLetter;
	TInt r=TheFs.DriveToChar(aDrive,driveLetter);
	test_KErrNone(r);
	gSessionPath[0]=(TText)driveLetter;
	r=TheFs.SetSessionPath(gSessionPath);
	test_KErrNone(r);

// !!! Disable platform security tests until we get the new APIs
//	if(User::Capability() & KCapabilityRoot)
//		CheckMountLFFS(TheFs,driveLetter);

	r=TheFs.MkDirAll(gSessionPath);
	TheFs.ResourceCountMarkStart();

	TRAP(r,CallTestsL());
	
	if (r==KErrNone)
		TheFs.ResourceCountMarkEnd();
	else
		{
		test.Printf(_L("Error: Leave %d\n"),r);
		test(EFalse);
		}

	TestingLFFS(EFalse);
	}

void ParseCommandArguments()
//
// Parse command line.  Put the parameters into global array gArgv for
// use by the tests, strip out flags starting with / or - and interpret
// them to set debug flags.
//
	{
	LOCAL_D TBuf<0x100> cmd;
	User::CommandLine(cmd);
	TLex lex(cmd);
	TPtrC token=lex.NextToken();
	TFileName thisfile=RProcess().FileName();
	if (token.MatchF(thisfile)==0)
		{
		token.Set(lex.NextToken());
		}
	// set up parameter list (offset zero is the filename)
	gArgC = 0;
	gArgV[gArgC++].Set(thisfile);
	while (token.Length() != 0)
		{
		TChar ch = token[0];
		// strip out (and interpret) flags starting with - or /
		if (ch == '-' || ch == '/')
			{
			for (TInt i = 1; i < token.Length(); i++)
				{
				switch (User::UpperCase(token[i]))
					{
					case 'D':
						gDebugFlags |= KDLYFAST;
						break;
					case 'F':
						gDebugFlags |= KFSYS;
						break;
					case 'I':
						gDebugFlags |= KISO9660;
						break;
					case 'L':
						gDebugFlags |= KFLDR;
						break;
#ifdef __CONCURRENT_FILE_ACCESS__
					case 'M':
						gDebugFlags |= KTHRD;
						break;
#endif
					case 'N':
						gDebugFlags |= KNTFS;
						break;
					case 'S':
						gDebugFlags |= KFSERV;
						break;
					case 'T':
						gDebugFlags |= KLFFS;
						break;
					case 'Y':
						gDebugFlags |= KDLYTRC;
						break;
					}
				}
			}
		else
			gArgV[gArgC++].Set(token);
		token.Set(lex.NextToken());
		}

	// set up drive to test
	gDriveToTest = 'C';
	if (gArgC > 1)
		{
		gDriveToTest = gArgV[1][0];
		gDriveToTest.UpperCase();
		}
	}


GLDEF_C TInt E32Main()
    {
	CTrapCleanup* cleanup;
	cleanup=CTrapCleanup::New();
	TRAPD(r,PushLotsL());
	__UHEAP_MARK;

	test.Title();
	test.Start(_L("Starting tests..."));

	r=TheFs.Connect();
	test_KErrNone(r);

	ParseCommandArguments(); //need this for drive letter to test

	TheFs.SetAllocFailure(gAllocFailOn);
	TTime timerC;
	timerC.HomeTime();
	TFileName sessionp;
	TheFs.SessionPath(sessionp);

	TBuf<30> privatedir;
	privatedir = KPrivate;

	TUid thisUID = RProcess().Identity();
	privatedir.AppendFormat(_L("%08x"),thisUID.iUid);
	privatedir.Append(_L("\\"));

	test(privatedir == sessionp.Mid(2,sessionp.Length()-2));

	test.Printf(_L("sp=%S"),&sessionp);
	sessionp[0]=(TText)gDriveToTest;
	test.Printf(_L("sp1=%S"),&sessionp);

	TInt theDrive;
	r=TheFs.CharToDrive(gDriveToTest,theDrive);
	test_KErrNone(r);
	
	// set up debug register
	test.Printf(_L("debug register = 0x%X"), gDebugFlags);
	TheFs.SetDebugRegister(gDebugFlags);

	// actually do the tests!
	DoTests(theDrive);
	
	// reset the debug register
	TheFs.SetDebugRegister(0);

	TTime endTimeC;
	endTimeC.HomeTime();
	TTimeIntervalSeconds timeTakenC;
	r=endTimeC.SecondsFrom(timerC,timeTakenC);
	test_KErrNone(r);
	test.Printf(_L("Time taken for test = %d seconds\n"),timeTakenC);
	TheFs.SetAllocFailure(gAllocFailOff);
	TheFs.Close();
	test.End();
	test.Close();
	__UHEAP_MARKEND;
	delete cleanup;
	return(KErrNone);
    }
