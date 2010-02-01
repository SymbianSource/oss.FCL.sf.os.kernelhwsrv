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
// f32test\fsstress\t_remses.cpp
// 
//

#if !defined(__T_REMFSY_H__)
#include "t_remfsy.h"
#endif

GLDEF_D TFileName tPath;

LOCAL_C void printDriveAtt(TInt aDrive,TUint anAtt,RTest& aTest);
LOCAL_C void printDriveInfo(TInt aDrive,TDriveInfo& anInfo, RTest& aTest);
LOCAL_C void DriveInformation(TInt aDrive,TDriveInfo& anInfo,RTest& aTest);


void TMultipleSessionTest::Initialise(RFs& aFs)
//
//	Initialise iFs
//
	{
	iFs=aFs;
	}


void TMultipleSessionTest::SetSessionPath(TInt aDrive)
//
//	Set the session path for a RFs connection to aDrive
//
	{
	iSessionPath=(_L("?:\\MULTIPLE_SESSION_TEST\\"));
	TChar driveLetter;
	TInt r=iFs.DriveToChar(aDrive,driveLetter);
	test(r==KErrNone);
		
	iSessionPath[0]=(TText)driveLetter;
	r=iFs.SetSessionPath(iSessionPath);
	test(r==KErrNone);
	r=iFs.MkDirAll(iSessionPath);
	test(r==KErrNone || r==KErrAlreadyExists);
	}


void TMultipleSessionTest::RunTests(RTest& aTest)
//
//	Run tests on iFs file server session
//

	{
	
	testDriveList(aTest);
	testDriveInfo(aTest);
	testVolumeInfo(aTest);
//	testPowerDown(aTest);
	testSetVolume(aTest);
	testInitialisation(aTest);
//	testMediaChange(aTest);
	testSubst(aTest);
	MakeAndDeleteFiles(aTest);
	}



void TMultipleSessionTest::testDriveList(RTest& aTest)
//
//	Test the drive list.
//
	{

	aTest.Next(_L("The drive list"));
	TInt r=iFs.SetSessionPath(iSessionPath);
	TDriveList list;
	r=iFs.DriveList(list);
	aTest(r==KErrNone);

	for (TInt i=0;i<KMaxDrives;i++)
		{
		TInt att=list[i];
		if (att)
			printDriveAtt(i,att,aTest);
		}
	}

void TMultipleSessionTest::testDriveInfo(RTest& aTest)
//
//	Test the drive info.
//
	{
	
	aTest.Next(_L("The drive info"));
	TInt r=iFs.SetSessionPath(iSessionPath);
	TDriveList list;
	r=iFs.DriveList(list);
	aTest(r==KErrNone);
	for (TInt i=0;i<KMaxDrives;i++)
		{
		TInt att=list[i];
		if (att)
			{
			TDriveInfo d;
			r=iFs.Drive(d,i);
			//aTest(r==KErrNone);
			printDriveInfo(i,d,aTest);
			aTest.Printf(_L("\n"));
			if (r==KErrNone)
				DriveInformation(i,d, aTest);
			}
		}
	}
/*
void TMultipleSessionTest::testPowerDown(RTest& aTest)
//
//	Test the effect of multiple power downs
//
	{
	aTest.Next(_L("Power Down"));
	
	RTimer timer;
	test(timer.CreateLocal()==KErrNone);
	TTime time;
	TRequestStatus status;

	for (TInt index=0; index<5; index++)
		{
		aTest.Printf(_L("********** %d **********\n"),(5-index));
		time.HomeTime();
		time+=TTimeIntervalSeconds(8);
		timer.At(status,time);
		UserHal::SwitchOff();			// Switch off
		User::WaitForRequest(status);	// Switch back on
		aTest(status==KErrNone);
		}
	}

*/
/*
void TMultipleSessionTest::testMediaChange(RTest& aTest)
//
//	Test the effect of multiple media changes
//
	{
	aTest.Next(_L("Media Change"));
	TInt drive=CurrentDrive(aTest);
	
	if (drive>KMaxLocalDrives)
		return;

#if defined (__MARM__)
	if (drive==EDriveC)
		return;
#endif
	
	TLocalDrive theDrive;
	TLocalDriveCaps info;
	TBool changedFlag;
	for (TInt index=0; index<5; index++)
		{
		aTest.Printf(_L("********** %d **********\n"),(5-index));
		changedFlag=EFalse;
		TInt r=theDrive.Connect(drive,changedFlag);
		aTest(r==KErrNone);
		UserSvr::ForceRemountMedia(ERemovableMedia0); // Generate media change
		aTest(changedFlag);

		do
			{
			r=theDrive.Caps(info);
			} while (r==KErrNotReady);
//		Wait a second...
//		User::After(1000000);
		}
	}
*/

void TMultipleSessionTest::testVolumeInfo(RTest& aTest)
//
//	Test volume info.
//
	{

	aTest.Next(_L("The volume info"));
	TInt r=iFs.SetSessionPath(iSessionPath);
	TDriveList list;
	r=iFs.DriveList(list);
	aTest(r==KErrNone);
	for (TInt i=0;i<KMaxDrives;i++)
		{
		TVolumeInfo v;
		if ((r=iFs.Volume(v,i))==KErrNone)
			{
			printDriveInfo(i,v.iDrive,aTest);
			aTest.Printf(_L("   VOL=\"%S\" ID=%08x\n"),&v.iName,v.iUniqueID);
			aTest.Printf(_L("   SIZE=%ldK FREE=%ldK\n"),v.iSize/1024,v.iFree/1024);
			}
		else if (r==KErrNotReady)
			aTest.Printf(_L("%c: Not Ready\n"),i+'A');
		else if (r==KErrPathNotFound)
			aTest.Printf(_L("%c: Not Found\n"),i+'A');
		else
			{
			aTest.Printf(_L("%c: Error %d\n"),i+'A',r);
			aTest.Getch();
			}
		aTest.Printf(_L("\n"));
		}
	}


void TMultipleSessionTest::testInitialisation(RTest& aTest)
//
//	Modified from T_SESSION.  Still tests that calls to CheckedClose() are
//	OK, ie, tests bug fix, but doesn't check returned error values since remote drive
//	doesn't necessarily return them.
//
	{
	RFile file;
	RDir dir;
		
	aTest.Next(_L("Test calls to CheckedClose are OK"));
	
	TInt r=iFs.SetSessionPath(iSessionPath);
	TInt count;
	RFormat format;
	r=format.Open(iFs,_L("Z:\\"),EFullFormat,count);

	aTest((r==KErrAccessDenied)||(r==KErrInUse));
	
	r=dir.Open(iFs,_L("\\MULTIPLE_SESSION_TEST\\ERRORTEST\\"),KEntryAttMatchMask);
//	aTest(r==KErrPathNotFound);		
	if (r==KErrNone)
		dir.Close();
	
	r=file.Open(iFs,_L("\\MULTIPLE_SESSION_TEST\\SessionTest1.txt"),EFileRead);
//	aTest(r==KErrNotFound);
	if (r==KErrNone)
		file.Close();

	r=dir.Open(iFs,_L("\\MULTIPLE_SESSION_TEST\\ERRORTEST2\\"),KEntryAttMatchMask);
//	aTest(r==KErrPathNotFound);		
	if (r==KErrNone)
		dir.Close();

	r=file.Open(iFs,_L("\\MULTIPLE_SESSION_TEST\\SessionTest2.txt"),EFileRead);
//	aTest(r==KErrNotFound);
	if (r==KErrNone)
		file.Close();

	r=dir.Open(iFs,_L("\\MULTIPLE_SESSION_TEST\\ERRORTEST3\\"),KEntryAttMatchMask);
//	aTest(r==KErrPathNotFound);		
	if (r==KErrNone)
		dir.Close();

	r=file.Open(iFs,_L("\\MULTIPLE_SESSION_TEST\\SessionTest3.txt"),EFileRead);
//	aTest(r==KErrNotFound);
	if (r==KErrNone)
		file.Close();

	r=file.Open(iFs,_L("\\MULTIPLE_SESSION_TEST\\SessionTest4.txt"),EFileRead);
//	aTest(r==KErrNotFound);
	if (r==KErrNone)
		file.Close();
	}


void TMultipleSessionTest::testSubst(RTest& aTest)
//
//	Test the substitute functions
//
	{

	aTest.Next(_L("Test subst"));
	TInt r=iFs.SetSessionPath(iSessionPath);
	TVolumeInfo v;
	r=iFs.Volume(v);
	aTest(r==KErrNone);
	
	TDriveInfo driveInfo;
	r=iFs.Drive(driveInfo,EDriveO);
	aTest(r==KErrNone);

	testSetVolume(aTest);
	
	if (driveInfo.iDriveAtt==KDriveAttLocal)
		return;	//	Subst local drives fails

	TFileName n;
	r=iFs.Subst(n,EDriveO);
	aTest(r==KErrNone);
	aTest(n.Length()==0);
	r=iFs.SetSubst(iSessionPath,EDriveO);
	aTest(r==KErrNone);
	r=iFs.Subst(n,EDriveO);
	aTest(r==KErrNone);
	aTest(n==iSessionPath);
	TVolumeInfo w;
	r=iFs.Volume(w,EDriveO);
	aTest(r==KErrNone);
	aTest(w.iDrive.iType==v.iDrive.iType);
	aTest(w.iDrive.iConnectionBusType==v.iDrive.iConnectionBusType);
	aTest(w.iDrive.iDriveAtt==v.iDrive.iDriveAtt);
	aTest(w.iDrive.iMediaAtt==v.iDrive.iMediaAtt);
	aTest(w.iUniqueID==v.iUniqueID);
	aTest(w.iSize==v.iSize);
	aTest(w.iFree==v.iFree);
	aTest(w.iName==v.iName);
	TDriveInfo d;
	r=iFs.Drive(d,EDriveO);
	aTest(r==KErrNone);
	aTest(d.iDriveAtt==KDriveAttSubsted);

	aTest.Next(_L("Test real name"));
	r=iFs.RealName(_L("O:\\FILE.XXX"),n);
	aTest(r==KErrNone);
	TFileName substedPath=iSessionPath;
	substedPath.Append(_L("FILE.XXX"));
	aTest(n==substedPath);

	aTest.Next(_L("Test MkDir, Rename and RmDir on Substed drive"));
	r=iFs.MkDir(_L("C:\\MULTIPLE_SESSION_TEST\\TFSRV\\turgid\\"));
	aTest(r==KErrNone);
	r=iFs.Rename(_L("O:\\turgid"), _L("O:\\facile"));
	aTest(r==KErrNone);
	r=iFs.MkDir(_L("O:\\insipid\\"));
	aTest(r==KErrNone);
	r=iFs.Rename(_L("O:\\insipid"), _L("O:\\glib"));
	aTest(r==KErrNone);
	r=iFs.RmDir(_L("O:\\facile\\"));
	aTest(r==KErrNone);
	r=iFs.RmDir(_L("C:\\MULTIPLE_SESSION_TEST\\TFSRV\\glib\\"));
	aTest(r==KErrNone);

	r=iFs.SetSubst(_L(""),EDriveO);
	aTest(r==KErrNone);
	r=iFs.Subst(n,EDriveO);
	aTest(r==KErrNone);
	aTest(n==_L(""));
	r=iFs.Drive(d,EDriveO);
	aTest(r==KErrNone);
	aTest(d.iDriveAtt==0);

	}


LOCAL_C TInt CreateFileX(const TDesC& aBaseName,TInt aX, RFs iFs, RTest& aTest)
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
		aTest.Printf(_L("ERROR:: Replace returned %d\n"),r);
		aTest.Getch();
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
		aTest.Printf(_L("ERROR:: SetSize returned %d\n"),r);
		aTest.Getch();
		file.Close();
		return(KErrDiskFull);
		}
	file.Close();
	aTest.Printf(_L("Created file %d size 64k\n"),aX);
	return(KErrNone);
	}

LOCAL_C TInt DeleteFileX(TBuf<128>& aBaseName,TInt aX, RFs iFs, TInt aDrive,RTest& aTest)
//
// Delete a file.
//
	{

	TBuf<128> fileName=aBaseName;
	fileName.AppendNum(aX);

	TInt r=iFs.Delete(fileName);
	if (aDrive!=EDriveQ)	//	T_REMFSY may return incorrect value on WINS
		aTest(r==KErrNone);
	aTest.Printf(_L("Deleted File %d\n"),aX);
	return(KErrNone);
	}

void TMultipleSessionTest::MakeAndDeleteFiles(RTest& aTest)
//
// Create and delete large files in a randomish order
//
	{

	aTest.Next(_L("Create and delete large files"));
	TInt r=iFs.SetSessionPath(iSessionPath);
	r=iFs.MkDirAll(_L("\\MULTIPLE_SESSION_TEST\\SMALLDIRECTORY\\"));

	if (CurrentDrive(aTest)!=EDriveQ)	//	T_REMFSY may return incorrect result on WINS
		aTest(r==KErrNone || r==KErrAlreadyExists);
	
	TBuf<128> fileName=_L("\\MULTIPLE_SESSION_TEST\\SMALLDIRECTORY\\FILE");
	r=CreateFileX(fileName,0,iFs,aTest);
	aTest(r==KErrNone);
	r=CreateFileX(fileName,1,iFs,aTest);
	aTest(r==KErrNone);
	r=DeleteFileX(fileName,0,iFs,CurrentDrive(aTest),aTest);	
	aTest(r==KErrNone);
	r=CreateFileX(fileName,2,iFs,aTest);
	aTest(r==KErrNone);
	r=CreateFileX(fileName,1,iFs,aTest);
	aTest(r==KErrNone);
	r=CreateFileX(fileName,3,iFs,aTest);
	aTest(r==KErrNone);
	r=DeleteFileX(fileName,1,iFs,CurrentDrive(aTest),aTest);	
	aTest(r==KErrNone);
	r=CreateFileX(fileName,4,iFs,aTest);
	aTest(r==KErrNone);
	r=DeleteFileX(fileName,2,iFs,CurrentDrive(aTest),aTest);	
	aTest(r==KErrNone);
	r=DeleteFileX(fileName,3,iFs,CurrentDrive(aTest),aTest);	
	aTest(r==KErrNone);
	r=DeleteFileX(fileName,4,iFs,CurrentDrive(aTest),aTest);	
	aTest(r==KErrNone);
	r=CreateFileX(fileName,1,iFs,aTest);
	aTest(r==KErrNone);
	r=DeleteFileX(fileName,1,iFs,CurrentDrive(aTest),aTest);	
	aTest(r==KErrNone);

	r=iFs.CheckDisk(fileName);
	if (r!=KErrNone && r!=KErrNotSupported)
		aTest.Printf(_L("ERROR:: CheckDisk returned %d\n"),r);
	}

void TMultipleSessionTest::FillUpDisk(RTest& aTest)
//
//	Test that a full disk is ok
//
	{

	aTest.Next(_L("Fill disk to capacity"));
	TInt r=iFs.SetSessionPath(iSessionPath);
	r=iFs.MkDirAll(_L("\\MULTIPLE_SESSION_TEST\\BIGDIRECTORY\\"));
	if	(CurrentDrive(aTest)!=EDriveQ)	//	T_REMFSY may return incorrect result on WINS
		aTest(r==KErrNone || r==KErrAlreadyExists);
	TInt count=0;
	TFileName sessionPath;
	r=iFs.SessionPath(sessionPath);
	test(sessionPath==iSessionPath);
	aTest(r==KErrNone);
	TBuf<128> fileName=_L("\\MULTIPLE_SESSION_TEST\\BIGDIRECTORY\\FILE");
	FOREVER
		{
		TInt r=CreateFileX(fileName,count,iFs, aTest);
		if (r==KErrDiskFull)
			break;
		aTest(r==KErrNone);
		count++;
 #if defined(__WINS__)
		if (count==32 && sessionPath[0]=='C') // Don't fill up disk on NT
			break;
 #endif
		}

	r=iFs.CheckDisk(fileName);
	if (r!=KErrNone && r!=KErrNotSupported)
		{
		aTest.Printf(_L("ERROR:: CheckDisk returned %d\n"),r);
		aTest.Getch();
		}

	while(count--)
		DeleteFileX(fileName,count,iFs,CurrentDrive(aTest),aTest);

	r=iFs.CheckDisk(fileName);
	if (r!=KErrNone && r!=KErrNotSupported)
		{
		aTest.Printf(_L("ERROR:: CheckDisk returned %d\n"),r);
		aTest.Getch();
		}
	}


void TMultipleSessionTest::testSetVolume(RTest& aTest)
//
//	Test setting the volume info.
//
	{

	aTest.Next(_L("Test setting the volume label"));
	TInt r=iFs.SetSessionPath(iSessionPath);
	aTest(r==KErrNone);
#if defined(_UNICODE)
	aTest.Printf(_L("Unicode volume label set not implemented ****\n"));
	return;
#else
	TInt driveNum=CurrentDrive(aTest);
	TVolumeInfo v;
	r=iFs.Volume(v,driveNum);
	aTest(r==KErrNone);
	TFileName n=v.iName;
	aTest.Printf(_L("VOL=\"%S\"\n"),&n);

	aTest.Next(_L("Set volume label to nothing"));
	r=iFs.SetVolumeLabel(_L(""),driveNum);
	if (r==KErrGeneral)
		return;
	aTest(r==KErrNone);
	r=iFs.Volume(v,driveNum);
	aTest(r==KErrNone);
	aTest(v.iName==_L(""));
	aTest.Printf(_L("VOL=\"%S\"\n"),&v.iName);

	aTest.Next(_L("Set volume label to ABCDEFGHIJK"));
	r=iFs.SetVolumeLabel(_L("ABCDEFGHIJK"),driveNum);
	aTest(r==KErrNone);
	r=iFs.Volume(v,driveNum);
	aTest(r==KErrNone);
	aTest(v.iName==_L("ABCDEFGHIJK"));
	aTest.Printf(_L("VOL=\"%S\"\n"),&v.iName);

	aTest.Next(_L("Set volume label back to nothing"));
	r=iFs.SetVolumeLabel(_L(""),driveNum);
	aTest(r==KErrNone);
	r=iFs.Volume(v,driveNum);
	aTest(r==KErrNone);
	aTest(v.iName==_L(""));
	aTest.Printf(_L("VOL=\"%S\"\n"),&v.iName);

	aTest.Next(_L("Set volume label to original"));
	r=iFs.SetVolumeLabel(n,driveNum);
	aTest(r==KErrNone);
	r=iFs.Volume(v,driveNum);
	aTest(r==KErrNone);
	aTest(v.iName==n);
	aTest.Printf(_L("VOL=\"%S\"\n"),&v.iName);

#endif
	}


LOCAL_C void printDriveAtt(TInt aDrive,TUint anAtt,RTest& aTest)
//
// Print a drive attribute.
//
	{

	aTest.Printf(_L("%c: "),aDrive+'A');
	if (anAtt&KDriveAttRemote)
		aTest.Printf(_L("REMOTE "));
	else if (anAtt&KDriveAttLocal)
		aTest.Printf(_L("LOCAL "));
	if (anAtt&KDriveAttRom)
		aTest.Printf(_L("ROM "));
	if (anAtt&KDriveAttRedirected)
		aTest.Printf(_L("REDIRECTED "));
	if (anAtt&KDriveAttSubsted)
		aTest.Printf(_L("SUBST "));
	if (anAtt&KDriveAttInternal)
		aTest.Printf(_L("INTERNAL "));
	if (anAtt&KDriveAttRemovable)
		aTest.Printf(_L("REMOVABLE "));
	aTest.Printf(_L("\n"));
	}

LOCAL_C void printDriveInfo(TInt aDrive,TDriveInfo& anInfo,RTest& aTest)
//
// Print a drive info.
//
	{

	printDriveAtt(aDrive,anInfo.iDriveAtt,aTest);
	aTest.Printf(_L("   MEDIA-ATT="));
	if (anInfo.iMediaAtt==0)
		aTest.Printf(_L("<none>"));
	if (anInfo.iMediaAtt&KMediaAttVariableSize)
		aTest.Printf(_L("VariableSize "));
	if (anInfo.iMediaAtt&KMediaAttDualDensity)
		aTest.Printf(_L("DualDensity "));
	if (anInfo.iMediaAtt&KMediaAttFormattable)
		aTest.Printf(_L("Formattable "));
	if (anInfo.iMediaAtt&KMediaAttWriteProtected)
		aTest.Printf(_L("WProtected "));
	aTest.Printf(_L("\n   CONNECTION BUS TYPE="));
	switch(anInfo.iConnectionBusType)
		{
	case EConnectionBusInternal: aTest.Printf(_L("Internal\n")); break;
	case EConnectionBusUsb: aTest.Printf(_L("USB\n")); break;
	default:
		aTest.Printf(_L("Unknown value\n"));
		}
	aTest.Printf(_L("   MEDIA="));
	switch(anInfo.iType)
		{
	case EMediaNotPresent: aTest.Printf(_L("Not present\n")); break;
	case EMediaUnknown: aTest.Printf(_L("Unknown\n")); break;
	case EMediaFloppy: aTest.Printf(_L("Floppy\n")); break;
	case EMediaHardDisk: aTest.Printf(_L("Hard disk\n")); break;
	case EMediaCdRom: aTest.Printf(_L("CD Rom\n")); break;
	case EMediaRam: aTest.Printf(_L("Ram\n")); break;
	case EMediaFlash: aTest.Printf(_L("Flash\n")); break;
	case EMediaRom: aTest.Printf(_L("Rom\n")); break;
	case EMediaRemote: aTest.Printf(_L("Remote\n")); break;
	default:
		aTest.Printf(_L("Unknown value\n"));
		}
	}

LOCAL_C void DriveInformation(TInt aDrive,TDriveInfo& anInfo,RTest& aTest)
//
//	Test the drive info is reasonable
//
	{

	aTest(anInfo.iConnectionBusType==EConnectionBusInternal || anInfo.iConnectionBusType==EConnectionBusUsb);
	
	if (aDrive==EDriveZ)
		{
		if (anInfo.iType==EMediaNotPresent)
			return;
		
		aTest(anInfo.iMediaAtt==KMediaAttWriteProtected);
		aTest(anInfo.iDriveAtt==(KDriveAttRom|KDriveAttInternal));
		aTest(anInfo.iType==EMediaRom);
		}
	
	else if (aDrive==EDriveC || aDrive==EDriveY)
		{
		if (anInfo.iType==EMediaNotPresent)
			return;

		aTest(anInfo.iDriveAtt==(KDriveAttLocal|KDriveAttInternal));
		aTest(anInfo.iType==EMediaHardDisk);
		aTest(anInfo.iMediaAtt==(KMediaAttVariableSize|KMediaAttFormattable));
		}
	else if (aDrive==EDriveD || aDrive==EDriveX)
		{
		if (anInfo.iType==EMediaNotPresent)
			return;

		aTest(anInfo.iDriveAtt==(KDriveAttLocal|KDriveAttRemovable));
		aTest(anInfo.iType==EMediaHardDisk);
		aTest(anInfo.iMediaAtt==KMediaAttFormattable);
		}
	}



GLDEF_C void ReportCheckDiskFailure(TInt aRet,RTest& aTest)
//
// Report the failure of checkdisk
//
	{

	aTest.Printf(_L("CHECKDISK FAILED: "));
	switch(aRet)
		{
	case 1:	aTest.Printf(_L("File cluster chain contains a bad value (<2 or >maxCluster)\n")); break;
	case 2:	aTest.Printf(_L("Two files are linked to the same cluster\n")); break;
	case 3:	aTest.Printf(_L("Unallocated cluster contains a value != 0\n"));	break;
	case 4:	aTest.Printf(_L("Size of file != number of clusters in chain\n")); break;
	default: aTest.Printf(_L("Undefined Error value %d\n"),aRet);
		}
	aTest.Printf(_L("Press any key to continue\n"));
	aTest.Getch();
	}



TInt TMultipleSessionTest::CurrentDrive(RTest& aTest)
//
// Return the current drive number
//
	{
	TInt r=iFs.SetSessionPath(iSessionPath);
	aTest(r==KErrNone);
	TInt driveNum;
	r=iFs.CharToDrive(iSessionPath[0],driveNum);
	aTest(r==KErrNone);
	return(driveNum);
	}


	
	
