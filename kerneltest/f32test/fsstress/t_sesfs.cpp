// Copyright (c) 1998-2009 Nokia Corporation and/or its subsidiary(-ies).
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
// f32test\fsstress\t_sesfs.cpp
// 
//
#define __E32TEST_EXTENSION__
#include "t_sess.h"

GLDEF_D TFileName tPath;
GLREF_D TFileName gExeFileName;

LOCAL_C void printDriveAtt(TInt aDrive,TUint anAtt);
LOCAL_C void printDriveInfo(TInt aDrive,TDriveInfo& anInfo);
LOCAL_C void DriveInfo(TInt aDrive,TDriveInfo& anInfo);


void TSessionTest::Initialise(RFs& aFs)
//
//	Initialise iFs
//
	{
	iFs=aFs;
	}


void TSessionTest::RunTests()
//
//	Run tests on iFs file server session
//

	{
	
	testDriveList();
	testDriveInfo();
	testVolumeInfo();
	testSetVolume();
	testPath();
	CreateTestDirectory(_L("\\SESSION_TEST\\TFSRV\\"));
	testInitialisation();
	testSubst();
	CopyFileToTestDirectory();
	MakeAndDeleteFiles();
	}



void TSessionTest::testDriveList()
//
// Test the drive list.
//
	{

	test.Start(_L("The drive list"));
	TDriveList list;
	TDriveInfo info;
	TInt r=iFs.DriveList(list);
	test_KErrNone(r);
	for (TInt i=0;i<KMaxDrives;i++)
		{
		if (list[i])
			{
			r = iFs.Drive(info, i);
			test_KErrNone(r);
			printDriveAtt(i,info.iDriveAtt);
			}
		}

	test.End();
	}

void TSessionTest::testDriveInfo()
//
// Test the drive info.
//
	{

	test.Start(_L("The drive info"));
	TDriveList list;
	TInt r=iFs.DriveList(list);
	test_KErrNone(r);
	for (TInt i=0;i<KMaxDrives;i++)
		{
		TInt att=list[i];
		if (att)
			{
			TDriveInfo d;
			r=iFs.Drive(d,i);
			test_KErrNone(r);
			printDriveInfo(i,d);
			test.Printf(_L("\n"));
			DriveInfo(i,d);
			}
		}

	test.End();
	}

void TSessionTest::testVolumeInfo()
//
// Test volume info.
//
	{

	test.Start(_L("The volume info"));
	TDriveList list;
	TInt r=iFs.DriveList(list);
	test_KErrNone(r);
	for (TInt i=0;i<KMaxDrives;i++)
		{
		TVolumeInfo v;
		TDriveInfo d;
		switch (r=iFs.Volume(v,i))
			{
			case KErrNone:
				printDriveInfo(i,v.iDrive);
				test.Printf(_L("   VOL=\"%S\" ID=%08x\n"),&v.iName,v.iUniqueID);
				test.Printf(_L("   SIZE=%ldK FREE=%ldK\n"),v.iSize/1024,v.iFree/1024);
				break;
			case KErrNotReady:
				r=iFs.Drive(d, i);
				test_KErrNone(r);
				if (d.iType == EMediaNotPresent)
					test.Printf(_L("%c: Medium not present - cannot perform test.\n"), i + 'A');
				else
					test.Printf(_L("medium found (type %d) but drive %c: not ready\nPrevious test may have hung; else, check hardware.\n"), (TInt)d.iType, i + 'A');
				break;
			case KErrPathNotFound:
				test.Printf(_L("%c: Not Found\n"), i + 'A');
				break;
			case KErrCorrupt:
				test.Printf(_L("%c: Media corruption; previous test may have aborted; else, check hardware\n"), i + 'A');
			default:
				test.Printf(_L("%c: Error %d - aborting test.\n"),i + 'A', r);
				test(0);
			}
		test.Printf(_L("\n"));
		}

	test.End();
	}


void TSessionTest::testPath()
//
// Test the path handling.
//
	{

	test.Start(_L("Test path handling"));

	TInt r;
	
	TFileName p;
	r=iFs.SessionPath(p);
	test_KErrNone(r);
	test.Printf(_L("SESSION=\"%S\"\n"),&p);
	r=iFs.SetSessionPath(_L("A:\\TEST\\"));
	test_KErrNone(r);
	r=iFs.SessionPath(p);
	test_KErrNone(r);
	test(p==_L("A:\\TEST\\"));
	r=iFs.SetSessionPath(gTestSessionPath);
	test_KErrNone(r);

	test.End();
	}


void TSessionTest::testInitialisation()
//
//	Tests that calls to CheckedClose() are OK, ie, tests bug fix
//
	{
	test.Next(_L("Test calls to CheckedClose are OK"));
	
	RFile file;
	RDir dir;
		
	TInt count;
	RFormat format;
	TInt r=format.Open(iFs,_L("Z:\\"),EFullFormat,count);

	test_Value(r, r==KErrAccessDenied || r==KErrInUse);
	
	r=dir.Open(iFs,_L("\\SESSION_TEST\\ERRORTEST\\"),KEntryAttMatchMask);
	test_Value(r, r==KErrPathNotFound);
	
	r=file.Open(iFs,_L("\\SESSION_TEST\\SessionTest1.txt"),EFileRead);
	test_Value(r, r==KErrNotFound);

	r=dir.Open(iFs,_L("\\SESSION_TEST\\ERRORTEST2\\"),KEntryAttMatchMask);
	test_Value(r, r==KErrPathNotFound);		
	
	r=file.Open(iFs,_L("\\SESSION_TEST\\SessionTest2.txt"),EFileRead);
	test_Value(r, r==KErrNotFound);

	r=dir.Open(iFs,_L("\\SESSION_TEST\\ERRORTEST3\\"),KEntryAttMatchMask);
	test_Value(r, r==KErrPathNotFound);		
	
	r=file.Open(iFs,_L("\\SESSION_TEST\\SessionTest3.txt"),EFileRead);
	test_Value(r, r==KErrNotFound);

	r=file.Open(iFs,_L("\\SESSION_TEST\\SessionTest4.txt"),EFileRead);
	test_Value(r, r==KErrNotFound);
	
	format.Close();
	dir.Close();
	file.Close();
	}


void TSessionTest::testSubst()
//
// Test the substitute functions.
//
	{

	test.Printf(_L("Test subst"));
	TVolumeInfo v;
	TInt r=iFs.Volume(v);
	test_KErrNone(r);
	TDriveInfo origDI;
	r=iFs.Drive(origDI);
	test_KErrNone(r);
	
	TDriveInfo driveInfo;
	r=iFs.Drive(driveInfo,EDriveO);
	test_KErrNone(r);

	testSetVolume();
	
	if (driveInfo.iDriveAtt==KDriveAttLocal)
		{	
		return;	//	Subst local drives fails
		}
	
	TFileName n;
	r=iFs.Subst(n,EDriveO);
	test_KErrNone(r);
	test(n.Length()==0);
	r=iFs.SetSubst(gTestSessionPath,EDriveO);
	test_KErrNone(r);
	r=iFs.Subst(n,EDriveO);
	test_KErrNone(r);
	test(n==gTestSessionPath);
	TVolumeInfo w;
	r=iFs.Volume(w,EDriveO);
	test_KErrNone(r);
	test(w.iDrive.iType==v.iDrive.iType);
	test(w.iDrive.iConnectionBusType==v.iDrive.iConnectionBusType);
	test(w.iDrive.iDriveAtt==KDriveAttSubsted);
	test(w.iDrive.iMediaAtt==v.iDrive.iMediaAtt);
	test(w.iUniqueID==v.iUniqueID);

    if(v.iDrive.iType != EMediaRam) // We can't assume that RAM disk will be the same size since last recorded...
    {
	    test(w.iSize==v.iSize);

    // If this test is being run under windows using drive C then skip free space comparison
    // as it is likely to fail as the windows file system is unlike to have static freespace
#ifdef __WINS__
	    if(User::UpperCase(gTestSessionPath[0]) != 'C')
		    {
#endif
		    test(w.iFree==v.iFree);

#ifdef __WINS__
		    }
#endif
    }

	test(w.iName==v.iName);
	TDriveList driveList;
	r=iFs.DriveList(driveList);
	test_KErrNone(r);
	test(driveList[EDriveO]==KDriveAttSubsted);
	TDriveInfo d;
	r=iFs.Drive(d,EDriveO);
	test_KErrNone(r);
	test(d.iDriveAtt==KDriveAttSubsted);
	test(d.iMediaAtt==origDI.iMediaAtt);
	test(d.iType==origDI.iType);
	test(d.iConnectionBusType==origDI.iConnectionBusType);


	test.Next(_L("Test real name"));
	r=iFs.RealName(_L("O:\\FILE.XXX"),n);
	test_KErrNone(r);
	TFileName substedPath=gTestSessionPath;
	substedPath.Append(_L("FILE.XXX"));
	test(n.CompareF(substedPath)==KErrNone);
//
	test.Next(_L("Test MkDir, Rename and RmDir on Substed drive"));
	_LIT(KTurgid,"turgid\\");
	TFileName dir=gTestSessionPath;
	dir+=KTurgid;
	r=iFs.MkDirAll(dir);
	test_KErrNone(r);
	dir+=_L("subdir\\");
	r=iFs.MkDir(dir);
	test_KErrNone(r);
	r=iFs.RmDir(_L("O:\\turgid\\subdir\\"));
	test_KErrNone(r);
	r=iFs.Rename(_L("O:\\turgid"), _L("O:\\facile"));
	test_KErrNone(r);
	r=iFs.MkDir(_L("O:\\insipid\\"));
	test_KErrNone(r);
	r=iFs.Rename(_L("O:\\insipid"), _L("O:\\glib"));
	test_KErrNone(r);
	r=iFs.RmDir(_L("O:\\facile\\"));
	test_KErrNone(r);
	_LIT(KGlib,"glib\\");
	dir=gTestSessionPath;
	dir+=KGlib;
	r=iFs.RmDir(dir);
	test_KErrNone(r);
	test.Next(_L("Test file operations on Substed drive"));
	_LIT(File1,"File1.txt");
	_LIT(File2,"File2.txt");
	_LIT(SubstRoot,"O:\\");
	_LIT(Subdir,"subdir\\");
	TFileName name1,name2;
	name1=gTestSessionPath;
	name1+=File1;
	RFile f1;
	r=f1.Create(iFs,name1,EFileShareExclusive|EFileWrite);
	test_KErrNone(r);
	name2=SubstRoot;
	name2+=File2;
	TBool isValid=iFs.IsValidName(name2);
	test(isValid);
	r=f1.Rename(name2);
	test_KErrNone(r);
	f1.Close();
	r=f1.Create(iFs,name1,EFileShareExclusive|EFileWrite);
	test_KErrNone(r);
	f1.Close();
	r=iFs.Replace(name2,name1);
	test_KErrNone(r);
	r=iFs.Delete(name1);
	test_KErrNone(r);
	test.Next(_L("Test notifications on Substed drive"));
	name1=gTestSessionPath;
	name1+=Subdir;
	name2=SubstRoot;
	name2+=Subdir;
	// set up some extended notifications
	TRequestStatus status1;
	TRequestStatus status2;
	TRequestStatus status3;
	iFs.NotifyChange(ENotifyDir,status1,name1);
	test(status1==KRequestPending);
	iFs.NotifyChange(ENotifyDir,status2,name2);
	test(status2==KRequestPending);
	r=iFs.MkDirAll(name1);
	test_KErrNone(r);
	User::WaitForRequest(status1);
	User::WaitForRequest(status2);
	test(status1==KErrNone && status2==KErrNone);
	iFs.NotifyChange(ENotifyDir,status1,name1);
	test(status1==KRequestPending);
	iFs.NotifyChange(ENotifyDir,status2,name2);
	test(status2==KRequestPending);
	iFs.NotifyChange(ENotifyAll,status3,name2);
	test(status3==KRequestPending);
	r=f1.Temp(iFs,name2,n,EFileShareAny|EFileWrite);
	test_KErrNone(r);
	User::WaitForRequest(status3);
	test(status3==KErrNone && status1==KRequestPending && status2==KRequestPending);
	f1.Close();
	iFs.NotifyChangeCancel();
	test(status1==KErrCancel && status2==KErrCancel);
	User::WaitForRequest(status1);
	User::WaitForRequest(status2);
	r=iFs.Delete(n);
	test_KErrNone(r);
	r=iFs.RmDir(name1);
	test_KErrNone(r);
//
	test.Next(_L("Test file systems on Substed drive"));
	// test cannot mount file system on substituted drive
	TInt sessionDrv;
	r=iFs.CharToDrive(gTestSessionPath[0],sessionDrv);
	test_KErrNone(r);
	r=iFs.FileSystemName(n,sessionDrv);
	test_Value(r, r==KErrNone || r==KErrNotFound);
	r=iFs.MountFileSystem(n,EDriveO);
	test_Value(r, r==KErrAccessDenied);
	// test file system name on substitued drive is null
	r=iFs.FileSystemName(n,EDriveO);
	test_Value(r, r==KErrNotFound && n==KNullDesC);
	// test cannot format a substitued drive
	RFormat format;
	TInt count;
	r=format.Open(iFs,SubstRoot,EHighDensity,count);
	test_Value(r, r==KErrAccessDenied);
	
	r=iFs.SetSubst(_L(""),EDriveO);
	test_KErrNone(r);
	r=iFs.Subst(n,EDriveO);
	test_KErrNone(r);
	test(n==_L(""));
	r=iFs.Drive(d,EDriveO);
	test_KErrNone(r);
	test(d.iDriveAtt==0);
	}


LOCAL_C TInt CreateFileX(const TDesC& aBaseName,TInt aX, RFs iFs)
//
// Create a large file. Return KErrEof or KErrNone
//
	{

	TBuf<128> fileName=aBaseName;
	fileName.AppendNum(aX);
	RFile file;
	TInt r=file.Replace(iFs,fileName,EFileWrite);
	if (r==KErrDiskFull)
		return(r);
	if (r!=KErrNone)
		{
		test.Printf(_L("ERROR:: Replace returned %d\n"),r);
		return(KErrDiskFull);
		}
	r=file.SetSize(65536);
	if (r==KErrDiskFull)
		{
		file.Close();
		return(r);
		}
	if (r!=KErrNone)
		{
		test.Printf(_L("ERROR:: SetSize returned %d\n"),r);
		file.Close();
		return(KErrDiskFull);
		}
	file.Close();
//	r=iFs.CheckDisk(fileName);
//	if (r!=KErrNone && r!=KErrNotSupported)
//		{
//		test.Printf(_L("ERROR:: CheckDisk returned %d\n"),r);
//		test.Getch();
//		return(KErrDiskFull);
//		}
	test.Printf(_L("Created file %d size 64k\n"),aX);
	return(KErrNone);
	}

LOCAL_C TInt DeleteFileX(TBuf<128>& aBaseName,TInt aX, RFs iFs)
//
// Delete a file.
//
	{

	TBuf<128> fileName=aBaseName;
	fileName.AppendNum(aX);
	TInt r=iFs.Delete(fileName);
	test_KErrNone(r);
//	r=iFs.CheckDisk(fileName);
//	if (r!=KErrNone && r!=KErrNotSupported)
//		{
//		test.Printf(_L("ERROR:: CheckDisk returned %d\n"),r);
//		test_KErrNone(r);
//		}
	test.Printf(_L("Deleted File %d\n"),aX);
	return(KErrNone);
	}

void TSessionTest::MakeAndDeleteFiles()
//
// Create and delete large files in a randomish order
//
	{

	test.Start(_L("Create and delete large files"));
	TInt r=iFs.MkDirAll(_L("\\SESSION_TEST\\SMALLDIRECTORY\\"));
	test_Value(r, r==KErrNone || r==KErrAlreadyExists);
	TBuf<128> fileName=_L("\\SESSION_TEST\\SMALLDIRECTORY\\FILE");
	r=CreateFileX(fileName,0,iFs);
	test_KErrNone(r);
	r=CreateFileX(fileName,1,iFs);
	test_KErrNone(r);
	r=DeleteFileX(fileName,0,iFs);	
	test_KErrNone(r);
	r=CreateFileX(fileName,2,iFs);
	test_KErrNone(r);
	r=CreateFileX(fileName,1,iFs);
	test_KErrNone(r);
	r=CreateFileX(fileName,3,iFs);
	test_KErrNone(r);
	r=DeleteFileX(fileName,1,iFs);	
	test_KErrNone(r);
	r=CreateFileX(fileName,4,iFs);
	test_KErrNone(r);
	r=DeleteFileX(fileName,2,iFs);	
	test_KErrNone(r);
	r=DeleteFileX(fileName,3,iFs);	
	test_KErrNone(r);
	r=DeleteFileX(fileName,4,iFs);	
	test_KErrNone(r);
	r=CreateFileX(fileName,1,iFs);
	test_KErrNone(r);
	r=DeleteFileX(fileName,1,iFs);	
	test_KErrNone(r);

	r=iFs.CheckDisk(fileName);
	test_Value(r, r==KErrNone || r==KErrNotSupported);
	test.End();
	}

void TSessionTest::FillUpDisk()
//
// Test that a full disk is ok
//
	{

	test.Start(_L("Fill disk to capacity"));
	TInt r=iFs.MkDirAll(_L("\\SESSION_TEST\\BIGDIRECTORY\\"));
	test_Value(r, r==KErrNone || r==KErrAlreadyExists);
	TInt count=0;
	TFileName sessionPath;
	r=iFs.SessionPath(sessionPath);
	test_KErrNone(r);
	TBuf<128> fileName=_L("\\SESSION_TEST\\BIGDIRECTORY\\FILE");
	FOREVER
		{
		TInt r=CreateFileX(fileName,count,iFs);
		if (r==KErrDiskFull)
			break;
		test_KErrNone(r);
		count++;
 #if defined(__WINS__)
		if (count==32 && sessionPath[0]=='C') // Don't fill up disk on NT
			break;
 #endif
		}

	r=iFs.CheckDisk(fileName);
	test_Value(r, r==KErrNone || r==KErrNotSupported);

	while(count--)
		DeleteFileX(fileName,count,iFs);

	r=iFs.CheckDisk(fileName);
	test_Value(r, r==KErrNone || r==KErrNotSupported);

	test.End();
	}

void TSessionTest::CopyFileToTestDirectory()
//
// Make a copy of the file in ram
//
	{

	TFileName fn = _L("Z:\\TEST\\T_FSRV.CPP");
	fn[0] = gExeFileName[0];
	TParse f;
	TInt r=iFs.Parse(fn,f);
	test_KErrNone(r);
	test.Next(_L("Copying file to test directory"));
	TParse fCopy;
	r=iFs.Parse(f.NameAndExt(),fCopy);
	test_KErrNone(r);

	RFile f1;
	r=f1.Open(iFs,f.FullName(),EFileStreamText|EFileShareReadersOnly);
	test.Printf(_L("r=%d\n"),r);
	test_KErrNone(r);
	RFile f2;
	r=f2.Replace(iFs,fCopy.FullName(),EFileWrite);
	test_KErrNone(r);
	TBuf8<512> copyBuf;
	TInt rem;
	r=f1.Size(rem);
	test_KErrNone(r);
	TInt pos=0;
	while (rem)
		{
		TInt s=Min(rem,copyBuf.MaxSize());
		r=f1.Read(pos,copyBuf,s);
		test_KErrNone(r);
		test(copyBuf.Length()==s);
		r=f2.Write(pos,copyBuf,s);
		test_KErrNone(r);
		pos+=s;
		rem-=s;
		}
	f1.Close();
	f2.Close();
	}


void TSessionTest::testSetVolume()
//
// Test setting the volume info.
//
	{

	test.Start(_L("Test setting the volume label"));

#if defined(_UNICODE)
	test.Printf(_L("Unicode volume label set not implemented ****\n"));
	test.End();
	return;
#else
	TInt driveNum=CurrentDrive();
	TVolumeInfo v;
	TInt r=iFs.Volume(v,driveNum);
	test_KErrNone(r);
	TFileName n=v.iName;
	test.Printf(_L("VOL=\"%S\"\n"),&n);

	test.Next(_L("Set volume label to nothing"));
	r=iFs.SetVolumeLabel(_L(""),driveNum);
#if defined(__WINS__)
	if (r==KErrGeneral || r==KErrAccessDenied || r==KErrNotSupported)
		{
		test.Printf(_L("Error %d: Set volume label not testing on WINS\n"),r);
		test.End();
		return;
		}
#endif
	test_KErrNone(r);
	r=iFs.Volume(v,driveNum);
	test_KErrNone(r);
	test(v.iName==_L(""));
	test.Printf(_L("VOL=\"%S\"\n"),&v.iName);

	test.Next(_L("Set volume label to ABCDEFGHIJK"));
	r=iFs.SetVolumeLabel(_L("ABCDEFGHIJK"),driveNum);
	test_KErrNone(r);
	r=iFs.Volume(v,driveNum);
	test_KErrNone(r);
	test(v.iName==_L("ABCDEFGHIJK"));
	test.Printf(_L("VOL=\"%S\"\n"),&v.iName);

	test.Next(_L("Set volume label back to nothing"));
	r=iFs.SetVolumeLabel(_L(""),driveNum);
	test_KErrNone(r);
	r=iFs.Volume(v,driveNum);
	test_KErrNone(r);
	test(v.iName==_L(""));
	test.Printf(_L("VOL=\"%S\"\n"),&v.iName);

	test.Next(_L("Set volume label to original"));
	r=iFs.SetVolumeLabel(n,driveNum);
	test_KErrNone(r);
	r=iFs.Volume(v,driveNum);
	test_KErrNone(r);
	test(v.iName==n);
	test.Printf(_L("VOL=\"%S\"\n"),&v.iName);

	test.End();
#endif
	}

LOCAL_C void printDriveAtt(TInt aDrive,TUint anAtt)
//
// Print a drive attribute.
//
	{

	test.Printf(_L("%c: "),aDrive+'A');
	if (anAtt&KDriveAttLocal)
		test.Printf(_L("LOCAL "));
	if (anAtt&KDriveAttRom)
		test.Printf(_L("ROM "));
	if (anAtt&KDriveAttRedirected)
		test.Printf(_L("REDIR "));
	if (anAtt&KDriveAttSubsted)
		test.Printf(_L("SUBST "));
	if (anAtt&KDriveAttInternal)
		test.Printf(_L("INTERNAL "));
	if ((anAtt&KDriveAttRemovable) && !(anAtt&KDriveAttLogicallyRemovable))
		test.Printf(_L("PHYSICALLY-REMOVABLE "));
	if (anAtt&KDriveAttLogicallyRemovable)
		test.Printf(_L("LOGICALLY-REMOVABLE "));
	if (anAtt&KDriveAttHidden)
		test.Printf(_L("HIDDEN "));
	test.Printf(_L("\n"));
	}

LOCAL_C void printDriveInfo(TInt aDrive,TDriveInfo& anInfo)
//
// Print a drive info.
//
	{

	printDriveAtt(aDrive,anInfo.iDriveAtt);
	test.Printf(_L("   MEDIA-ATT="));
	if (anInfo.iMediaAtt==0)
		test.Printf(_L("<none>"));
	if (anInfo.iMediaAtt&KMediaAttVariableSize)
		test.Printf(_L("VariableSize "));
	if (anInfo.iMediaAtt&KMediaAttDualDensity)
		test.Printf(_L("DualDensity "));
	if (anInfo.iMediaAtt&KMediaAttFormattable)
		test.Printf(_L("Formattable "));
	if (anInfo.iMediaAtt&KMediaAttWriteProtected)
		test.Printf(_L("WProtected "));
	test.Printf(_L("\n   CONNECTION BUS="));
	switch(anInfo.iConnectionBusType)
		{
	case EConnectionBusInternal: test.Printf(_L("Internal\n")); break;
	case EConnectionBusUsb: test.Printf(_L("USB\n")); break;
	default:
		test.Printf(_L("Unknown value\n"));
		}
	test.Printf(_L("   MEDIA="));
	switch(anInfo.iType)
		{
	case EMediaNotPresent: test.Printf(_L("Not present\n")); break;
	case EMediaUnknown: test.Printf(_L("Unknown\n")); break;
	case EMediaFloppy: test.Printf(_L("Floppy\n")); break;
	case EMediaHardDisk: test.Printf(_L("Hard disk\n")); break;
	case EMediaCdRom: test.Printf(_L("CD Rom\n")); break;
	case EMediaRam: test.Printf(_L("Ram\n")); break;
	case EMediaFlash: test.Printf(_L("Flash\n")); break;
	case EMediaRom: test.Printf(_L("Rom\n")); break;
	case EMediaRemote: test.Printf(_L("Remote\n")); break;
	default:
		test.Printf(_L("Unknown value\n"));
		}
	}

LOCAL_C void DriveInfo(TInt aDrive,TDriveInfo& anInfo)
//
// Test the drive info is reasonable
//
	{

	test(anInfo.iConnectionBusType==EConnectionBusInternal || anInfo.iConnectionBusType==EConnectionBusUsb);
	
	if (aDrive==EDriveZ)
		{
		if (anInfo.iType==EMediaNotPresent)
			return;
		
		test(anInfo.iMediaAtt==KMediaAttWriteProtected);
		test(anInfo.iDriveAtt==(KDriveAttRom|KDriveAttInternal));
		test(anInfo.iType==EMediaRom);
		}

/*
Why assume certain drive letters can only refer to certain drive types?
	else if (aDrive==EDriveC || aDrive==EDriveY)
		{
		if (anInfo.iType==EMediaNotPresent)
			return;

//		test(anInfo.iDriveAtt==(KDriveAttLocal|KDriveAttInternal));

		test(anInfo.iDriveAtt&(KDriveAttLocal|KDriveAttInternal)==KDriveAttLocal|KDriveAttInternal);	// LFFS sets KDriveAttTransaction as well
		test(anInfo.iType==EMediaRam || anInfo.iType==EMediaFlash);
		if(anInfo.iType==EMediaRam)	test(anInfo.iMediaAtt==(KMediaAttVariableSize|KMediaAttFormattable));
		else if(anInfo.iType==EMediaFlash) test(anInfo.iMediaAtt==KMediaAttFormattable);
		}
	
	else if (aDrive==EDriveD || aDrive==EDriveX)
		{
		if (anInfo.iType==EMediaNotPresent)
			return;

		test(anInfo.iDriveAtt==(KDriveAttLocal|KDriveAttRemovable));
		test(anInfo.iType==EMediaHardDisk);
		test(anInfo.iMediaAtt&KMediaAttFormattable);
		}
*/
	}

void TSessionTest::CreateTestDirectory(const TDesC& aSessionPath)
//
// Create directory for test
//
	{
	TParsePtrC path(aSessionPath);
	test(path.DrivePresent()==EFalse);

	TInt r=iFs.SetSessionPath(aSessionPath);
	test_KErrNone(r);
	r=iFs.SessionPath(gTestSessionPath);
	test_KErrNone(r);
	r=iFs.MkDirAll(gTestSessionPath);
	test_Value(r, r==KErrNone || r==KErrAlreadyExists);
	}

TInt TSessionTest::CurrentDrive()
//
// Return the current drive number
//
	{

	TInt driveNum;
	TInt r=iFs.CharToDrive(gTestSessionPath[0],driveNum);
	test_KErrNone(r);
	return(driveNum);
	}

