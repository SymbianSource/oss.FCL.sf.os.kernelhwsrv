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
//

#define __E32TEST_EXTENSION__
#include <f32file.h>
#include <e32test.h>
#include <e32std.h>
#include <e32std_private.h>
#include <e32def.h>
#include <e32def_private.h>
#include <e32svr.h>
#include "t_server.h"

GLDEF_D RTest test(_L("t_dcallcaps"));
GLDEF_D TTime gTimeNow;
LOCAL_D TInt gTheDriveNum;


const TInt KPathPosition = 2;
_LIT(KExpectedPrivatePath, "\\Private\\00000001\\");
_LIT(KResourcePath, "?:\\Resource\\");
_LIT(KSystemPath,	"?:\\Sys\\");
_LIT(KPrivatePath,	"?:\\Private\\");
_LIT(KPrivateFalseID,	"?:\\Private\\FFFFFFFF\\");
_LIT(KDriveName,	"Billy");
_LIT(KVolLable,		"Benny");
_LIT(KFileSys,	"systemfile.txt");
_LIT(KFileSys3, "sysfile.rna");
_LIT(KFilePri,	"privatefile.txt");
_LIT(KFilePri2,	"privatefile.tmp");
_LIT(KFilePri3,	"prifile.rna");
_LIT(KFileRes,	"resourcefile.txt");
_LIT(KFileRes3,	"resfile.rna");
_LIT(KMkDirSub,"Subdir\\");
_LIT(KOldFile,"?:\\Anyold.txt");
_LIT(KWildPath, "Z:\\SYS\\");
_LIT(KWildFile, "*");

TCapability TheCaps;
TBuf<4> driveBuf=_L("?:\\");
RFormat format;
TInt count;
RRawDisk rawdisk;
RFile file1;
RFile file2;
RDir	dir;

TInt r;
TBuf<40> fsname;
TBuf<40> systestname;
TBuf<40> pritestname;
TBuf<40> restestname;
TBuf<40> theprivatepath;
TBuf<40> pritestfalseidname;
TBuf<40> mkdirname;
TFileName fromTemp;

TBuf<25> sysfilename;
TBuf<30> realName;
TBuf<40> shortfilename;
TBuf<40> longfilename;

TRequestStatus aStat1;
TRequestStatus aStat2;
TRequestStatus aStat3;
TRequestStatus aStat4;

TBuf<40> systestfile;
TBuf<40> pritestfile;
TBuf<40> restestfile;
TBuf<40> systestfile1;
TBuf<40> pritestfile1;
TBuf<40> restestfile1;

TTime testtime;
TBuf<20> oldName;

TBuf<25> temp;

TEntry entry;

LOCAL_C void systemRFsTest()
//
//	RFs test on system Directory
//
	{
	systestname=KSystemPath;
	systestname[0]=(TText)('A' + gTheDriveNum);
	
	mkdirname.Zero();
	mkdirname.Append(systestname);
	mkdirname.Append(KMkDirSub);
	r=TheFs.MkDirAll(mkdirname);	
	test_KErrNone(r);

	TheFs.RmDir(mkdirname);
	test_KErrNone(r);

	r=TheFs.SetSubst(systestname,EDriveO);
	test_KErrNone(r);
	
	r=TheFs.RealName(_L("O:\\File.XXX"),realName);
	test_KErrNone(r);

	r=TheFs.SetSubst(_L(""),EDriveO); //to unsubst
	test_KErrNone(r);

	r=TheFs.SetSessionPath(systestname);
	test_KErrNone(r);
	
	TheFs.NotifyChange(ENotifyAll,aStat1,systestname);
	test(aStat1==KRequestPending);

	systestfile=KSystemPath;
	systestfile[0]=(TText)('A' + gTheDriveNum);
	systestfile1=systestfile;
	systestfile.Append(KFileSys);
	systestfile1.Append(KFileSys3);
	
	oldName=KOldFile;
	oldName[0]=(TText)gDriveToTest;

	r=TheFs.GetShortName(systestfile, shortfilename);
	test_Value(r, r == KErrNotFound || r==KErrNotSupported);

	r=TheFs.GetLongName(systestfile1, longfilename);
	test_Value(r, r == KErrNotFound || r==KErrNotSupported);

	r=file1.Create(TheFs,oldName,EFileWrite);
	test_Value(r, r == KErrNone || r==KErrAlreadyExists);
	file1.Close();

	r=TheFs.Replace(oldName,systestfile);
	test_KErrNone(r);
	
	r=TheFs.Rename(systestfile,systestfile1);
	test_KErrNone(r);
	
	r=TheFs.Entry(systestfile1,entry);
	test_KErrNone(r);

	r=TheFs.SetEntry(systestfile1,testtime,KEntryAttNormal,KEntryAttReadOnly);
	test_KErrNone(r);

	r=TheFs.Delete(systestfile1);
	test_KErrNone(r);
	}

LOCAL_C void ResourceRFsTest()
//
//
//
	{
	restestname=KResourcePath;
	restestname[0]=(TText)('A' + gTheDriveNum);
	
	mkdirname.Zero();
	mkdirname.Append(restestname);
	mkdirname.Append(KMkDirSub);
	r=TheFs.MkDirAll(mkdirname);	
	test_KErrNone(r);

	TheFs.RmDir(mkdirname);
	test_KErrNone(r);

	r=TheFs.SetSubst(restestname,EDriveO);
	test_KErrNone(r);
	
	r=TheFs.RealName(_L("O:\\File.XXX"),realName);
	test_KErrNone(r);

	r=TheFs.SetSubst(_L(""),EDriveO); 
	test_KErrNone(r);

	r=TheFs.SetSessionPath(restestname);
	test_KErrNone(r);
	
	TheFs.NotifyChange(ENotifyAll,aStat4,restestname);
	test(aStat4==KRequestPending);

	restestfile=KResourcePath;
	restestfile[0]=(TText)('A' + gTheDriveNum);
	restestfile1=restestfile;
	restestfile.Append(KFileRes);
	restestfile1.Append(KFileRes3);
	
	oldName=KOldFile;
	oldName[0]=(TText)gDriveToTest;

	r=TheFs.GetShortName(restestfile, shortfilename);
	test_Value(r, r == KErrNone || r==KErrNotFound || r==KErrNotSupported);

	r=TheFs.GetLongName(restestfile1, longfilename);
	test_Value(r, r == KErrNone || r==KErrNotFound || r==KErrNotSupported);

	r=file1.Create(TheFs,oldName,EFileWrite);
	test_Value(r, r == KErrNone || r==KErrAlreadyExists);
	file1.Close();

	r=TheFs.Replace(oldName,restestfile);
	test_KErrNone(r);
	
	r=TheFs.Rename(restestfile,restestfile1);
	test_KErrNone(r);
	
	r=TheFs.Entry(restestfile1,entry);
	test_KErrNone(r);

	r=TheFs.SetEntry(restestfile1,testtime,KEntryAttNormal,KEntryAttReadOnly);
	test_KErrNone(r);

	r=TheFs.Delete(restestfile1);
	test_KErrNone(r);

	}


LOCAL_C void privatefalseIDRFsTest()
//
//
//
	{
	//private//FalseID
	pritestfalseidname=KPrivateFalseID;
	pritestfalseidname[0]=(TText)('A' + gTheDriveNum);
	
	mkdirname.Zero();
	mkdirname.Append(pritestfalseidname);
	mkdirname.Append(KMkDirSub);

	r=TheFs.MkDirAll(mkdirname);	
	test_KErrNone(r);

	r=TheFs.RmDir(mkdirname);	
	test_KErrNone(r);

	r=TheFs.SetSubst(pritestfalseidname,EDriveO);
	test_KErrNone(r);

	r=TheFs.RealName(_L("O:\\File.XXX"),realName);
	test_KErrNone(r);

	r=TheFs.SetSubst(_L(""),EDriveO);
	test_KErrNone(r);

	r=TheFs.SetSessionPath(pritestfalseidname);
	test_KErrNone(r);

	TheFs.NotifyChange(ENotifyAll,aStat2,pritestfalseidname);
	test(aStat2==KRequestPending);


	pritestfile=KPrivateFalseID;
	pritestfile[0]=(TText)('A' + gTheDriveNum);
	pritestfile1=pritestfile;
	pritestfile.Append(KFilePri2);
	pritestfile1.Append(KFilePri3);
	
	oldName=KOldFile;
	oldName[0]=(TText)gDriveToTest;

	r=TheFs.GetShortName(pritestfile, shortfilename);
	test_Value(r, r == KErrNone || r==KErrNotFound || r==KErrNotSupported);

	r=TheFs.GetLongName(pritestfile1, longfilename);
	test_Value(r, r == KErrNone || r==KErrNotFound || r==KErrNotSupported);

	r=file1.Create(TheFs,oldName,EFileWrite);
	test_Value(r, r == KErrNone || r==KErrAlreadyExists);
	file1.Close();

	r=TheFs.Replace(oldName,pritestfile);
	test_KErrNone(r);
	
	r=TheFs.Rename(pritestfile,pritestfile1);
	test_KErrNone(r);
	
	r=TheFs.Entry(pritestfile1,entry);
	test_KErrNone(r);

	r=TheFs.SetEntry(pritestfile1,testtime,KEntryAttNormal,KEntryAttReadOnly);
	test_KErrNone(r);

	r=TheFs.Delete(pritestfile1);
	test_KErrNone(r);

	}



LOCAL_C void privateRFsTest()
//
//
//
	{
	//private
	pritestname=KPrivatePath;
	pritestname[0]=(TText)('A' + gTheDriveNum);
	
	mkdirname.Zero();
	mkdirname.Append(pritestname);
	mkdirname.Append(KMkDirSub);

	r=TheFs.MkDirAll(mkdirname);	
	test_KErrNone(r);

	r=TheFs.RmDir(mkdirname);	
	test_KErrNone(r);

	r=TheFs.SetSubst(pritestname,EDriveO);
	test_KErrNone(r);

	r=TheFs.RealName(_L("O:\\File.XXX"),realName);
	test_KErrNone(r);

	r=TheFs.SetSubst(_L(""),EDriveO);
	test_KErrNone(r);

	r=TheFs.SetSessionPath(pritestname);
	test_KErrNone(r);

	TheFs.NotifyChange(ENotifyAll,aStat2,pritestname);
	test(aStat2==KRequestPending);


	pritestfile=KPrivatePath;
	pritestfile[0]=(TText)('A' + gTheDriveNum);
	pritestfile1=pritestfile;
	pritestfile.Append(KFilePri2);
	pritestfile1.Append(KFilePri3);
	
	oldName=KOldFile;
	oldName[0]=(TText)gDriveToTest;

	r=TheFs.GetShortName(pritestfile, shortfilename);
	test_Value(r, r == KErrNone || r==KErrNotFound || r==KErrNotSupported);

	r=TheFs.GetLongName(pritestfile1, longfilename);
	test_Value(r, r == KErrNone || r==KErrNotFound || r==KErrNotSupported);

	r=file1.Create(TheFs,oldName,EFileWrite);
	test_Value(r, r == KErrNone || r==KErrAlreadyExists);
	file1.Close();

	r=TheFs.Replace(oldName,pritestfile);
	test_KErrNone(r);
	
	r=TheFs.Rename(pritestfile,pritestfile1);
	test_KErrNone(r);
	
	r=TheFs.Entry(pritestfile1,entry);
	test_KErrNone(r);

	r=TheFs.SetEntry(pritestfile1,testtime,KEntryAttNormal,KEntryAttReadOnly);
	test_KErrNone(r);

	r=TheFs.Delete(pritestfile1);
	test_KErrNone(r);

	}


LOCAL_C void privateSIDRFstest()
	{
	theprivatepath[0]=(TText)gDriveToTest;	
	test.Printf(_L("the Private Path = %S"),&theprivatepath);

	mkdirname.Zero();
	mkdirname.Append(theprivatepath);
	mkdirname.Append(KMkDirSub);
	r=TheFs.MkDirAll(mkdirname);	
	test_KErrNone(r);

	r=TheFs.RmDir(mkdirname);	
	test_KErrNone(r);

	r=TheFs.SetSubst(theprivatepath,EDriveO);	
	test_KErrNone(r);

	r=TheFs.RealName(_L("O:\\File.XXX"),realName);
	test_KErrNone(r);

	r=TheFs.SetSubst(_L(""),EDriveO);	
	test_KErrNone(r);

	r=TheFs.SetSessionPath(theprivatepath);
	test_KErrNone(r);

	TheFs.NotifyChange(ENotifyAll,aStat3,theprivatepath);
	test(aStat3==KRequestPending);

	pritestfile=theprivatepath;
	pritestfile[0]=(TText)('A' + gTheDriveNum);
	pritestfile1=pritestfile;
	pritestfile.Append(KFilePri2);
	pritestfile1.Append(KFilePri3);
	
	oldName=KOldFile;
	oldName[0]=(TText)gDriveToTest;

	r=TheFs.GetShortName(pritestfile, shortfilename);
	test_Value(r, r == KErrNone || r==KErrNotFound || r==KErrNotSupported);

	r=TheFs.GetLongName(pritestfile1, longfilename);
	test_Value(r, r == KErrNone || r==KErrNotFound || r==KErrNotSupported);

	r=file1.Create(TheFs,oldName,EFileWrite);
	test_Value(r, r == KErrNone || r==KErrAlreadyExists);
	file1.Close();

	r=TheFs.Replace(oldName,pritestfile);
	test_KErrNone(r);
	
	r=TheFs.Rename(pritestfile,pritestfile1);
	test_Value(r, r == KErrNone || r==KErrAlreadyExists);
	
	r=TheFs.Entry(pritestfile1,entry);
	test_KErrNone(r);

	r=TheFs.SetEntry(pritestfile1,testtime,KEntryAttNormal,KEntryAttReadOnly);
	test_KErrNone(r);

	r=TheFs.Delete(pritestfile1);
	test_KErrNone(r);
	}


LOCAL_C void systemRFiletest()
//
//RFile testing with session path set to //system//
//
	{

	r=TheFs.SetSessionPath(systestname);
	test_KErrNone(r);

	r=file1.Temp(TheFs,systestname,fromTemp,EFileWrite);
	test_KErrNone(r);
	file1.Close();

	r=file1.Create(TheFs,KFileSys,EFileWrite);
	test_KErrNone(r);
	file1.Close();

	r=file1.Open(TheFs,KFileSys,EFileWrite);
	test_KErrNone(r);
	file1.Close();

	r=file1.Open(TheFs,KFileSys,EFileRead);
	test_KErrNone(r);
	file1.Close();
	
	r=file1.Replace(TheFs,KFileSys,EFileWrite);
	test_KErrNone(r);
	
	r=file1.Rename(KFileSys3);
	test_KErrNone(r);
	file1.Close();

    TFindFile finder(TheFs);
    CDir* dir = NULL;
    r=finder.FindWildByDir(KWildFile, KWildPath, dir);
	if (!(r==KErrNone))
        test.Printf(_L("T_DCALLCAPS: test find wildcards r = %d (expected KErrNone)\n"), r);
	test_KErrNone(r);
	delete dir;
	}

LOCAL_C void resourceRFiletest()
//
//RFile testing with session path set to //resource//
//
	{
	r=TheFs.SetSessionPath(restestname);
	test_KErrNone(r);

	r=file1.Temp(TheFs,restestname,fromTemp,EFileWrite);
	test_KErrNone(r);
	file1.Close();

	r=file1.Create(TheFs,KFileRes,EFileWrite);
	test_KErrNone(r);
	file1.Close();

	r=file1.Open(TheFs,KFileRes,EFileWrite|EFileShareExclusive );
	test_KErrNone(r);
	
	r=file1.Rename(KFileRes3);
	test_KErrNone(r);
	file1.Close();

	r=file1.Open(TheFs,KFileRes3,EFileShareReadersOrWriters|EFileRead);
	test_KErrNone(r);
	file1.Close();

	r=file1.Open(TheFs,KFileRes3,EFileShareReadersOrWriters|EFileWrite);
	test_KErrNone(r);
	file1.Close();

	r=file1.Open(TheFs,KFileRes3,EFileShareReadersOnly);
	test_KErrNone(r);

	r=file1.ChangeMode(EFileShareExclusive);
	test_KErrNone(r);
	file1.Close();

	r=file1.Replace(TheFs,KFileRes,EFileWrite);
	test_KErrNone(r);
	file1.Close();	
	}


LOCAL_C void privatefalseIDRFiletest()
//
//RFile testing with session path set to //Private//falseID
//
	{
	r=TheFs.SetSessionPath(pritestfalseidname);
	test_KErrNone(r);

	r=file1.Temp(TheFs,pritestfalseidname,fromTemp,EFileWrite);
	test_KErrNone(r);
	file1.Close();

	r=file1.Create(TheFs,KFilePri,EFileWrite);
	test_KErrNone(r);
	file1.Close();

	r=file1.Open(TheFs,KFilePri,EFileWrite);
	test_KErrNone(r);
	file1.Close();

	r=file1.Open(TheFs,KFilePri,EFileRead);
	test_KErrNone(r);
	file1.Close();
	
	r=file1.Replace(TheFs,KFilePri,EFileWrite);
	test_KErrNone(r);
	
	r=file1.Rename(KFilePri3);
	test_KErrNone(r);
	file1.Close();
	}

LOCAL_C void privateRFiletest()
//
//RFile testing with session path set to //Private//falseID
//
	{
	r=TheFs.SetSessionPath(pritestname);
	test_KErrNone(r);

	r=file1.Temp(TheFs,pritestname,fromTemp,EFileWrite);
	test_KErrNone(r);
	file1.Close();

	r=file1.Create(TheFs,KFilePri,EFileWrite);
	test_KErrNone(r);
	file1.Close();

	r=file1.Open(TheFs,KFilePri,EFileWrite);
	test_KErrNone(r);
	file1.Close();
	
	r=file1.Open(TheFs,KFilePri,EFileRead);
	test_KErrNone(r);
	file1.Close();

	r=file1.Replace(TheFs,KFilePri,EFileWrite);
	test_KErrNone(r);
	
	r=file1.Rename(KFilePri3);
	test_KErrNone(r);
	file1.Close();
	}



LOCAL_C void privateSIDRFiletest()
//
//Rfile Testing with session path set to //Private//UID//
//
	{
	r=TheFs.SetSessionToPrivate(gTheDriveNum);
	test_KErrNone(r);
	
	r=file1.Temp(TheFs,theprivatepath,fromTemp,EFileWrite);
	test_KErrNone(r);
	file1.Close();

	r=file1.Create(TheFs,KFilePri,EFileWrite);
	test_KErrNone(r);
	file1.Close();

	r=file1.Open(TheFs,KFilePri,EFileWrite);
	test_KErrNone(r);
	file1.Close();
	
	r=file1.Open(TheFs,KFilePri,EFileRead);
	test_KErrNone(r);
	file1.Close();

	r=file1.Replace(TheFs,KFilePri,EFileWrite);
	test_KErrNone(r);
	
	r=file1.Rename(KFilePri3);
	test_KErrNone(r);
	file1.Close();
	}

LOCAL_C void RDirtest()
//
//
//
	{

	//system
	CDir*	dirEntries;
	TBuf<30> dirNameBuf(KSystemPath);
	dirNameBuf[0]=(TText)gDriveToTest;
	r=dir.Open(TheFs,dirNameBuf,KEntryAttNormal);
	test_KErrNone(r);
	dir.Close();
	r=TheFs.GetDir(dirNameBuf,KEntryAttMatchMask,ESortByName,dirEntries);
	test_KErrNone(r);
	dirNameBuf.Zero();
	delete dirEntries;
	//Private//falseID
	dirNameBuf=KPrivateFalseID;
	dirNameBuf[0]=(TText)gDriveToTest;
	r=dir.Open(TheFs,dirNameBuf,KEntryAttNormal);
	test_KErrNone(r);
	dir.Close();
	r=TheFs.GetDir(dirNameBuf,KEntryAttMatchMask,ESortByName,dirEntries);
	test_KErrNone(r);
	dirNameBuf.Zero();
	delete dirEntries;
	//Private
	dirNameBuf=KPrivatePath;
	dirNameBuf[0]=(TText)gDriveToTest;
	r=dir.Open(TheFs,dirNameBuf,KEntryAttNormal);
	test_KErrNone(r);
	dir.Close();
	r=TheFs.GetDir(dirNameBuf,KEntryAttMatchMask,ESortByName,dirEntries);
	test_KErrNone(r);
	dirNameBuf.Zero();
	delete dirEntries;
	//Private/uid
	TheFs.PrivatePath(dirNameBuf);
	dirNameBuf.Insert(0,_L("?:"));
	dirNameBuf[0]=(TText)gDriveToTest;
	r=dir.Open(TheFs,dirNameBuf,KEntryAttNormal);
	test_KErrNone(r);
	dir.Close();
	r=TheFs.GetDir(dirNameBuf,KEntryAttMatchMask,ESortByName,dirEntries);
	test_KErrNone(r);
	dirNameBuf.Zero();
	delete dirEntries;
	//Resource
	dirNameBuf=KResourcePath;
	dirNameBuf[0]=(TText)gDriveToTest;
	r=dir.Open(TheFs,dirNameBuf,KEntryAttNormal);
	test_KErrNone(r);
	r=TheFs.GetDir(dirNameBuf,KEntryAttMatchMask,ESortByName,dirEntries);
	test_KErrNone(r);
	dir.Close();
	delete dirEntries;
	}


LOCAL_C void TestAllCaps()
//
//	Test with root capabilities
//
	{
	r=TheFs.FileSystemName(fsname,gTheDriveNum);
	test_Value(r, r == KErrNone || r==KErrNotFound);
	r=DismountFileSystem(TheFs, fsname,gTheDriveNum);
	test_Value(r, r == KErrNone || r==KErrNotFound);
//	r=TheFs.RemoveFileSystem(fsname);	//can not test due to bug else where fix exists
//	test_KErrNone(r);
//	r=TheFs.AddFileSystem(fsname);
//	test_KErrNone(r);
	r=MountFileSystem(TheFs,fsname,gTheDriveNum);
	test_KErrNone(r);
	r=TheFs.SetDriveName(gTheDriveNum,KDriveName);
	test_KErrNone(r);
#ifndef __WINS__
 	r=TheFs.SetVolumeLabel(KVolLable, gTheDriveNum);
	test_KErrNone(r);
#endif

	systemRFsTest();
	ResourceRFsTest();
	privateRFsTest();	
	privateSIDRFstest();
	privatefalseIDRFsTest();

	systemRFiletest();
	resourceRFiletest();
	privateRFiletest();
	privateSIDRFiletest();
	privatefalseIDRFiletest();

	//check notifier return values set in rfs testing
	User::WaitForRequest(aStat1);
	User::WaitForRequest(aStat2);
	User::WaitForRequest(aStat3);
	User::WaitForRequest(aStat4);
	test(aStat1==KErrNone);
	test(aStat2==KErrNone);
	test(aStat3==KErrNone);
	test(aStat4==KErrNone);

	r=TheFs.SetSessionPath(systestname);
	test_KErrNone(r);
	
//Test RRawDisk class
	r=rawdisk.Open(TheFs,gTheDriveNum);
	test_KErrNone(r);
	rawdisk.Close();

	RDirtest();

#ifdef __WINS__
	if (User::UpperCase(driveBuf[0]) != 'C')
#endif
		{
		//Test RFormat class
		r=format.Open(TheFs,driveBuf,EHighDensity,count);
		test_KErrNone(r);

		while(count)	
			{
			TInt r=format.Next(count);
			test_KErrNone(r);
			}
		format.Close();
		}

	driveBuf[0]=(TText)gDriveToTest;
	r=TheFs.ScanDrive(driveBuf);
	test_Value(r, r == KErrNone || r==KErrNotSupported);
	r=TheFs.CheckDisk(driveBuf);
	test_Value(r, r == KErrNone || r==KErrNotSupported);
	}

LOCAL_C void TestCaps()
//
//	test format etc that require certain capabilities
//
	{
#ifndef __REMOVE_PLATSEC_DIAGNOSTIC_STRINGS__
	const char myDiagMsg[] = __PLATSEC_DIAGNOSTIC_STRING("Capability Check Failure");
#endif //!__REMOVE_PLATSEC_DIAGNOSTIC_STRINGS__

	r=RProcess().HasCapability(ECapabilityAllFiles, __PLATSEC_DIAGNOSTIC_STRING(myDiagMsg));
	test(r);
	r=RProcess().HasCapability(ECapabilityTCB, __PLATSEC_DIAGNOSTIC_STRING(myDiagMsg));
	test(r);
	r=RProcess().HasCapability(ECapabilityDiskAdmin, __PLATSEC_DIAGNOSTIC_STRING(myDiagMsg));
	test(r);

	driveBuf[0]=(TText)gDriveToTest;
	r=TheFs.SessionPath(temp);
	test_KErrNone(r);

	test.Printf(_L("Session path: %S"),&temp);

	r=TheFs.CreatePrivatePath(gTheDriveNum);
	test_Value(r, r == KErrNone || r== KErrAlreadyExists);

	TBuf<18> tempPri;
	r=TheFs.PrivatePath(tempPri);
	test_KErrNone(r);
	theprivatepath = _L("?:");
	theprivatepath.Append(tempPri);

	TestAllCaps();
	
	TFileName thesessionpath;

	r=TheFs.SetSessionToPrivate(gTheDriveNum);
	test_KErrNone(r);
	r=TheFs.SessionPath(thesessionpath);
	test_KErrNone(r);
	
	test(thesessionpath == theprivatepath);
	}

LOCAL_C void CleanupL()
//
// Clean up tests
//
	{
	test.Next(_L("Delete test directory"));
	CFileMan* fMan=CFileMan::NewL(TheFs);
	TInt r=fMan->RmDir(gSessionPath);
	test_Value(r, r == KErrNone || r == KErrPathNotFound);
	delete fMan;
	}

GLDEF_C void CallTestsL(/*TChar aDriveLetter*/)
//
// Do all tests
//
	{
	TurnAllocFailureOff();
	TheFs.CharToDrive(gDriveToTest,gTheDriveNum);

	TBuf<30> sesspath;
	sesspath=_L("?:\\");
	sesspath[0] = (TText)gDriveToTest;

	TInt r= TheFs.SetSessionPath(sesspath);
	test_KErrNone(r);

	//cleanup from previous run of this test
	TBuf<20> delDir;
	CFileMan* fMan=CFileMan::NewL(TheFs);
	delDir=KResourcePath;
	delDir[0]=(TText)gDriveToTest;
	r=fMan->RmDir(delDir);
	test_Value(r, r == KErrNone || r == KErrNotFound);
	delDir=KSystemPath;
	delDir[0]=(TText)gDriveToTest;
	r=fMan->RmDir(delDir);
	test_Value(r, r == KErrNone || r == KErrNotFound);
	delDir=KPrivatePath;
	delDir[0]=(TText)gDriveToTest;
	r=fMan->RmDir(delDir);
	test_Value(r, r == KErrNone || r == KErrNotFound);
	delete fMan;

	//check double mode ie that Defpath still works	
	RFs fs1;
	RFs fs2;
	
	r=fs1.Connect();
	test_KErrNone(r);
	r=fs1.SessionPath(sesspath);
	test_KErrNone(r);
	test.Printf(_L("session1 Path=%S"),&sesspath);

	TBuf<30> privatepath;
	r=fs1.SetSessionToPrivate(gTheDriveNum);
	test_KErrNone(r);
	r=fs1.PrivatePath(privatepath);
	test_KErrNone(r);
	r=privatepath.Compare(KExpectedPrivatePath());
	test_Value(r, r == 0);
	r=fs1.SessionPath(sesspath);
	test_KErrNone(r);
	r=privatepath.Compare(sesspath.Mid(KPathPosition));
	test_Value(r, r == 0);
	r=fs1.CreatePrivatePath(gTheDriveNum);
	test_KErrNone(r);
	fs1.Close();

	r=fs2.Connect();
	test_KErrNone(r);
	r=fs2.SessionPath(sesspath);
	test_KErrNone(r);
	test.Printf(_L("session2 Path=%S"),&sesspath);
	fs2.Close();

	TestCaps();

	test.Printf(_L("No of files open=%d"), TheFs.ResourceCount());

	CleanupL();
	}
