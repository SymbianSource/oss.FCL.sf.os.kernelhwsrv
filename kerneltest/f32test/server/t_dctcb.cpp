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

#include <f32file.h>
#include <e32test.h>
#include <e32std.h>
#include <e32std_private.h>
#include <e32svr.h>
#include <e32def.h>
#include <e32def_private.h>
#include "t_server.h"

GLDEF_D RTest test(_L("t_dctcb"));
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
//_LIT(KFileSys2,	"systemfile.tmp");
_LIT(KFileSys3, "sysfile.rna");

_LIT(KFilePri,	"privatefile.txt");
_LIT(KFilePri2,	"privatefile.tmp");
_LIT(KFilePri3,	"prifile.rna");

_LIT(KFileRes,	"resourcefile.txt");
//_LIT(KFileRes2,	"resourcefile.tmp");
_LIT(KFileRes3,	"resfile.rna");

_LIT(KMkDirSub,"Subdir\\");
_LIT(KOldFile,"?:\\Anyold.txt");
//_LIT(KNullPath, "");

//_LIT(KLFFSName, "Lffs");

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

TVolumeInfo aVolInfo;

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


LOCAL_C void TestPathCheck()
//
// This test case is brought in by INC054580
// (NTT Renaming sys ¨Cfolder on C -drive on H2 allows user to access sys -files)
//
    {
    TInt r = TheFs.Rename(_L("\\sys"), _L("\\sysbad"));
    test(r == KErrNone);
    r = TheFs.Rename(_L("\\resource"), _L("\\resourcebad"));
    test(r == KErrNone);
    r = TheFs.Rename(_L("\\sysbad"), _L("\\sys"));
    test(r == KErrNone);
    r = TheFs.Rename(_L("\\resourcebad"), _L("\\resource"));
    test(r == KErrNone);
    r = TheFs.Rename(_L("\\private"), _L("\\privatebad"));
    test(r == KErrPermissionDenied);
    }

LOCAL_C void systemRFsTest()
//
//	RFs test on system Directory
//
	{
	__UHEAP_MARK;
	systestname=KSystemPath;
	systestname[0]=(TText)('A' + gTheDriveNum);
	
	mkdirname.Zero();
	mkdirname.Append(systestname);
	mkdirname.Append(KMkDirSub);
	r=TheFs.MkDirAll(mkdirname);	
	test(r==KErrNone);

	TheFs.RmDir(mkdirname);
	test(r==KErrNone);

	r=TheFs.SetSubst(systestname,EDriveO);
	test(r==KErrPermissionDenied);
	
	r=TheFs.RealName(_L("O:\\File.XXX"),realName);
	test(r==KErrNone);

	r=TheFs.SetSessionPath(systestname);
	test(r==KErrPermissionDenied);
	
	TheFs.NotifyChange(ENotifyAll,aStat1,systestname);
	test(aStat1==KErrPermissionDenied);

	systestfile=KSystemPath;
	systestfile[0]=(TText)('A' + gTheDriveNum);
	systestfile1=systestfile;
	systestfile.Append(KFileSys);
	systestfile1.Append(KFileSys3);
	
	oldName=KOldFile;
	oldName[0]=(TText)gDriveToTest;

	r=TheFs.GetShortName(systestfile, shortfilename);
	test(r==KErrPermissionDenied || r==KErrNotSupported);

	r=TheFs.GetLongName(systestfile1, longfilename);
	test(r==KErrPermissionDenied || r==KErrNotSupported);

	r=file1.Create(TheFs,oldName,EFileWrite);
	test(r==KErrNone || r==KErrAlreadyExists);
	file1.Close();

	r=TheFs.Replace(oldName,systestfile);
	test(r==KErrNone);
	
	r=TheFs.Delete(systestfile1);
	test(r==KErrNone || r==KErrNotFound);

	r=TheFs.Rename(systestfile,systestfile1);
	test(r==KErrNone);
	
	r=TheFs.Entry(systestfile1,entry);
	test(r==KErrPermissionDenied);

	r=TheFs.SetEntry(systestfile1,testtime,KEntryAttNormal,KEntryAttReadOnly);
	test(r==KErrNone);

	r=TheFs.Delete(systestfile1);
	test(r==KErrNone);

	__UHEAP_MARKEND;

	}

LOCAL_C void ResourceRFsTest()
//
//
//
	{
	__UHEAP_MARK;

	restestname=KResourcePath;
	restestname[0]=(TText)('A' + gTheDriveNum);
	
	mkdirname.Zero();
	mkdirname.Append(restestname);
	mkdirname.Append(KMkDirSub);
	r=TheFs.MkDirAll(mkdirname);	
	test(r==KErrNone);

	TheFs.RmDir(mkdirname);
	test(r==KErrNone);

	r=TheFs.SetSubst(restestname,EDriveO);
	test(r==KErrPermissionDenied || r==KErrGeneral);
	
	r=TheFs.RealName(_L("O:\\File.XXX"),realName);
	test(r==KErrNone);

	r=TheFs.SetSessionPath(restestname);
	test(r==KErrNone);
	
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
	test(r==KErrNone || KErrNotFound || r==KErrNotSupported);

	r=TheFs.GetLongName(restestfile1, longfilename);
	test(r==KErrNone || KErrNotFound || r==KErrNotSupported);

	r=file1.Create(TheFs,oldName,EFileWrite);
	test(r==KErrNone || r==KErrAlreadyExists);
	file1.Close();

	r=TheFs.Replace(oldName,restestfile);
	test(r==KErrNone);
	
	r=TheFs.Rename(restestfile,restestfile1);
	test(r==KErrNone);
	
	r=TheFs.Entry(restestfile1,entry);
	test(r==KErrNone);

	r=TheFs.SetEntry(restestfile1,testtime,KEntryAttNormal,KEntryAttReadOnly);
	test(r==KErrNone);

	r=TheFs.Delete(restestfile1);
	test(r==KErrNone);

	__UHEAP_MARK;

	}

LOCAL_C void privatefalseIDRFsTest()
//
//
//
	{
	__UHEAP_MARK;

	//private
	pritestfalseidname=KPrivateFalseID;
	pritestfalseidname[0]=(TText)('A' + gTheDriveNum);
	
	mkdirname.Zero();
	mkdirname.Append(pritestfalseidname);
	mkdirname.Append(KMkDirSub);

	r=TheFs.MkDirAll(mkdirname);	
	test(r==KErrPermissionDenied);

	r=TheFs.RmDir(mkdirname);	
	test(r==KErrPermissionDenied);

	r=TheFs.SetSubst(pritestfalseidname,EDriveO);
	test(r==KErrPermissionDenied);

	r=TheFs.RealName(_L("O:\\File.XXX"),realName);
	test(r==KErrNone);

	r=TheFs.SetSessionPath(pritestfalseidname);
	test(r==KErrPermissionDenied);

	TheFs.NotifyChange(ENotifyAll,aStat2,pritestfalseidname);
	test(r==KErrPermissionDenied);


	pritestfile=KPrivateFalseID;
	pritestfile[0]=(TText)('A' + gTheDriveNum);
	pritestfile1=pritestfile;
	pritestfile.Append(KFilePri2);
	pritestfile1.Append(KFilePri3);
	
	oldName=KOldFile;
	oldName[0]=(TText)gDriveToTest;

	r=TheFs.GetShortName(pritestfile, shortfilename);
	test(r==KErrPermissionDenied || r==KErrNotSupported);

	r=TheFs.GetLongName(pritestfile1, longfilename);
	test(r==KErrPermissionDenied || r==KErrNotSupported);

	r=file1.Create(TheFs,oldName,EFileWrite);
	test(r==KErrNone || r==KErrAlreadyExists);
	file1.Close();

	r=TheFs.Replace(oldName,pritestfile);
	test(r==KErrPermissionDenied);
	
	r=TheFs.Rename(pritestfile,pritestfile1);
	test(r==KErrPermissionDenied);
	
	r=TheFs.Entry(pritestfile1,entry);
	test(r==KErrPermissionDenied);

	r=TheFs.SetEntry(pritestfile1,testtime,KEntryAttNormal,KEntryAttReadOnly);
	test(r==KErrPermissionDenied);

	r=TheFs.Delete(pritestfile1);
	test(r==KErrPermissionDenied);

	__UHEAP_MARKEND;
	}



LOCAL_C void privateRFsTest()
//
//
//
	{
	__UHEAP_MARK;
	//private
	pritestname=KPrivatePath;
	pritestname[0]=(TText)('A' + gTheDriveNum);
	
	mkdirname.Zero();
	mkdirname.Append(pritestname);
	mkdirname.Append(KMkDirSub);

	r=TheFs.MkDirAll(mkdirname);	
	test(r==KErrPermissionDenied);

	r=TheFs.RmDir(mkdirname);	
	test(r==KErrPermissionDenied);

	r=TheFs.SetSubst(pritestname,EDriveO);
	test(r==KErrPermissionDenied);

	r=TheFs.RealName(_L("O:\\File.XXX"),realName);
	test(r==KErrNone);

	r=TheFs.SetSessionPath(pritestname);
	test(r==KErrPermissionDenied);

	TheFs.NotifyChange(ENotifyAll,aStat2,pritestname);
	test(r==KErrPermissionDenied);


	pritestfile=KPrivatePath;
	pritestfile[0]=(TText)('A' + gTheDriveNum);
	pritestfile1=pritestfile;
	pritestfile.Append(KFilePri2);
	pritestfile1.Append(KFilePri3);
	
	oldName=KOldFile;
	oldName[0]=(TText)gDriveToTest;

	r=TheFs.GetShortName(pritestfile, shortfilename);
	test(r==KErrPermissionDenied || r==KErrNotSupported);

	r=TheFs.GetLongName(pritestfile1, longfilename);
	test(r==KErrPermissionDenied || r==KErrNotSupported);

	r=file1.Create(TheFs,oldName,EFileWrite);
	test(r==KErrNone || r==KErrAlreadyExists);
	file1.Close();

	r=TheFs.Replace(oldName,pritestfile);
	test(r==KErrPermissionDenied);
	
	r=TheFs.Rename(pritestfile,pritestfile1);
	test(r==KErrPermissionDenied);
	
	r=TheFs.Entry(pritestfile1,entry);
	test(r==KErrPermissionDenied);

	r=TheFs.SetEntry(pritestfile1,testtime,KEntryAttNormal,KEntryAttReadOnly);
	test(r==KErrPermissionDenied);

	r=TheFs.Delete(pritestfile1);
	test(r==KErrPermissionDenied);

	__UHEAP_MARKEND;
	}


LOCAL_C void privateSIDRFstest()
	{
	__UHEAP_MARK;

	theprivatepath[0]=(TText)gDriveToTest;	
	test.Printf(_L("the Private Path = %S"),&theprivatepath);

	mkdirname.Zero();
	mkdirname.Append(theprivatepath);
	mkdirname.Append(KMkDirSub);
	r=TheFs.MkDirAll(mkdirname);	
	test(r==KErrNone);

	r=TheFs.RmDir(mkdirname);	
	test(r==KErrNone);

	r=TheFs.SetSubst(theprivatepath,EDriveO);	
	test(r==KErrPermissionDenied || r==KErrGeneral); // Drive may already be substituted

	r=TheFs.RealName(_L("O:\\File.XXX"),realName);
	test(r==KErrNone);

	r=TheFs.SetSessionPath(theprivatepath);
	test(r==KErrNone);

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
	test(r==KErrNone || r==KErrNotFound || r==KErrNotSupported);

	r=TheFs.GetLongName(pritestfile1, longfilename);
	test(r==KErrNone || r==KErrNotFound || r==KErrNotSupported);

	r=file1.Create(TheFs,oldName,EFileWrite);
	test(r==KErrNone || r==KErrAlreadyExists);
	file1.Close();

	r=TheFs.Replace(oldName,pritestfile);
	test(r==KErrNone);
	
	r=TheFs.Rename(pritestfile,pritestfile1);
	test(r==KErrNone || r==KErrAlreadyExists);
	
	r=TheFs.Entry(pritestfile1,entry);
	test(r==KErrNone);

	r=TheFs.SetEntry(pritestfile1,testtime,KEntryAttNormal,KEntryAttReadOnly);
	test(r==KErrNone);

	r=TheFs.Delete(pritestfile1);
	test(r==KErrNone);

	__UHEAP_MARKEND;
	}


LOCAL_C void systemRFiletest()
//
//RFile testing with session path set to //sys//
//
	{
	__UHEAP_MARK;

	r=TheFs.SetSessionPath(systestname);
	test(r==KErrPermissionDenied);

	r=file1.Temp(TheFs,systestname,fromTemp,EFileWrite);
	test(r==KErrNone);
	file1.Close();

	TBuf<25> sysfilename;
	sysfilename.Append(systestname);
	sysfilename.Append(KFileSys);

	r=file1.Create(TheFs,sysfilename,EFileWrite);
	test(r==KErrNone);
	file1.Close();

	r=file1.Open(TheFs,sysfilename,EFileWrite);
	test(r==KErrNone);
	file1.Close();
	
	r=file1.Open(TheFs,sysfilename,EFileRead);
	test(r==KErrPermissionDenied);
	file1.Close();
	
	r=file1.Replace(TheFs,sysfilename,EFileWrite);
	test(r==KErrNone);

	TBuf<25> sysfilename2;
	sysfilename2.Append(systestname);
	sysfilename2.Append(KFileSys3);
		
	r=file1.Rename(sysfilename2);
	test(r==KErrNone);
	file1.Close();

    TFindFile finder(TheFs);
    CDir* dir = NULL;
    r=finder.FindWildByDir(KWildFile, KWildPath, dir);
	if (!(r==KErrPermissionDenied))
        test.Printf(_L("T_DCTCB: test find wildcards r = %d (expected KErrPermissionDenied)\n"), r);
	test(r==KErrPermissionDenied);
	delete dir;

	__UHEAP_MARKEND;
	}

LOCAL_C void resourceRFiletest()
//
//RFile testing with session path set to //resource//
//
	{
	__UHEAP_MARK;

	r=TheFs.SetSessionPath(restestname);
	test(r==KErrNone);

	r=file1.Temp(TheFs,restestname,fromTemp,EFileWrite);
	test(r==KErrNone);
	file1.Close();

	r=file1.Create(TheFs,KFileRes,EFileWrite);
	test(r==KErrNone);
	file1.Close();

	r=file1.Open(TheFs,KFileRes,EFileWrite|EFileShareExclusive );
	test(r==KErrNone);
	
	r=file1.Rename(KFileRes3);
	test(r==KErrNone);
	file1.Close();

	r=file1.Open(TheFs,KFileRes3,EFileShareReadersOrWriters|EFileRead);
	test(r==KErrNone);
	file1.Close();

	r=file1.Open(TheFs,KFileRes3,EFileShareReadersOrWriters|EFileWrite);
	test(r==KErrNone);
	file1.Close();

	r=file1.Open(TheFs,KFileRes3,EFileShareReadersOnly);
	test(r==KErrNone);

	r=file1.ChangeMode(EFileShareExclusive);
	test(r==KErrNone);
	file1.Close();

	r=file1.Replace(TheFs,KFileRes,EFileWrite);
	test(r==KErrNone);
	file1.Close();	
	
	__UHEAP_MARKEND;

	}


LOCAL_C void privatefalseIDRFiletest()
//
//RFile testing with session path set to //Private//false ID
//
	{
	__UHEAP_MARK;

	r=TheFs.SetSessionPath(pritestfalseidname);
	test(r==KErrPermissionDenied);

	r=file1.Temp(TheFs,pritestfalseidname,fromTemp,EFileWrite);
	test(r==KErrPermissionDenied);
	file1.Close();

	// Since can't set session path create explicit path
	pritestfile=KPrivateFalseID;
	pritestfile[0]=(TText)('A' + gTheDriveNum);
	pritestfile.Append(KFilePri);

	r=file1.Create(TheFs,pritestfile,EFileWrite);
	test(r==KErrPermissionDenied);
	file1.Close();

	r=file1.Open(TheFs,pritestfile,EFileWrite);
	test(r==KErrPermissionDenied);
	file1.Close();
	
	r=file1.Open(TheFs,pritestfile,EFileRead);
	test(r==KErrPermissionDenied);
	file1.Close();
		
	r=file1.Replace(TheFs,pritestfile,EFileWrite);
	test(r==KErrPermissionDenied);

	// File does not exist so can't rename it	
/*	r=file1.Rename(KFilePri3);
	test(r==KErrAlreadyExists || r==KErrNone);
	file1.Close();
*/	__UHEAP_MARKEND;
	}


LOCAL_C void privateRFiletest()
//
//RFile testing with session path set to //Private//
//
	{
	__UHEAP_MARK;

	r=TheFs.SetSessionPath(pritestname);
	test(r==KErrPermissionDenied);

	r=file1.Temp(TheFs,pritestname,fromTemp,EFileWrite);
	test(r==KErrPermissionDenied);
	file1.Close();

	// Since can't set session path create explicit path
	pritestfile=KPrivatePath;
	pritestfile[0]=(TText)('A' + gTheDriveNum);
	pritestfile.Append(KFilePri);

	r=file1.Create(TheFs,pritestfile,EFileWrite);
	test(r==KErrPermissionDenied);
	file1.Close();

	r=file1.Open(TheFs,pritestfile,EFileWrite);
	test(r==KErrPermissionDenied);
	file1.Close();
	
	r=file1.Open(TheFs,pritestfile,EFileRead);
	test(r==KErrPermissionDenied);
	file1.Close();
	
	r=file1.Replace(TheFs,pritestfile,EFileWrite);
	test(r==KErrPermissionDenied);

	// File does not exist so can't be renamed	
/*	r=file1.Rename(KFilePri3);
	test(r==KErrNone);
	file1.Close();
*/
	__UHEAP_MARKEND;
	}


LOCAL_C void privateSIDRFiletest()
//
//Rfile Testing with session path set to //Private//UID//
//
	{
	__UHEAP_MARK;
	r=TheFs.SetSessionToPrivate(gTheDriveNum);
	test(r==KErrNone);
	
	r=file1.Temp(TheFs,theprivatepath,fromTemp,EFileWrite);
	test(r==KErrNone);
	file1.Close();

	r=file1.Create(TheFs,KFilePri,EFileWrite);
	test(r==KErrNone);
	file1.Close();

	r=file1.Open(TheFs,KFilePri,EFileWrite);
	test(r==KErrNone);
	file1.Close();
	
	r=file1.Open(TheFs,KFilePri,EFileRead);
	test(r==KErrNone);
	file1.Close();
	
	r=file1.Replace(TheFs,KFilePri,EFileWrite);
	test(r==KErrNone);
	
	r=file1.Rename(KFilePri3);
	test(r==KErrNone);
	file1.Close();
	__UHEAP_MARKEND;
	}

LOCAL_C void RDirtest()
//
//
//
	{
	__UHEAP_MARK;

	//system
	CDir*	dirEntries;
	TBuf<30> dirNameBuf(KSystemPath);
	dirNameBuf[0]=(TText)gDriveToTest;
	r=dir.Open(TheFs,dirNameBuf,KEntryAttNormal);
	test(r==KErrPermissionDenied);
	dir.Close();
	r=TheFs.GetDir(dirNameBuf,KEntryAttMatchMask,ESortByName,dirEntries);
	test(r==KErrPermissionDenied);
	dirNameBuf.Zero();
	delete dirEntries;

	//Private
	dirNameBuf=KPrivatePath;
	dirNameBuf[0]=(TText)gDriveToTest;
	r=dir.Open(TheFs,dirNameBuf,KEntryAttNormal);
	test(r==KErrPermissionDenied);
	dir.Close();
	r=TheFs.GetDir(dirNameBuf,KEntryAttMatchMask,ESortByName,dirEntries);
	test(r==KErrPermissionDenied);
	dirNameBuf.Zero();
	delete dirEntries;

	//Private//falseID
	dirNameBuf=KPrivateFalseID;
	dirNameBuf[0]=(TText)gDriveToTest;
	r=dir.Open(TheFs,dirNameBuf,KEntryAttNormal);
	test(r==KErrPermissionDenied);
	dir.Close();
	r=TheFs.GetDir(dirNameBuf,KEntryAttMatchMask,ESortByName,dirEntries);
	test(r==KErrPermissionDenied);
	dirNameBuf.Zero();
	delete dirEntries;

	//Private/uid
	TheFs.PrivatePath(dirNameBuf);
	dirNameBuf.Insert(0,_L("?:"));
	dirNameBuf[0]=(TText)gDriveToTest;
	r=dir.Open(TheFs,dirNameBuf,KEntryAttNormal);
	test(r==KErrNone);
	dir.Close();
	r=TheFs.GetDir(dirNameBuf,KEntryAttMatchMask,ESortByName,dirEntries);
	test(r==KErrNone);
	dirNameBuf.Zero();
	delete dirEntries;
	//Resource
	dirNameBuf=KResourcePath;
	dirNameBuf[0]=(TText)gDriveToTest;
	r=dir.Open(TheFs,dirNameBuf,KEntryAttNormal);
	test(r==KErrNone);
	r=TheFs.GetDir(dirNameBuf,KEntryAttMatchMask,ESortByName,dirEntries);
	test(r==KErrNone);
	dir.Close();
	delete dirEntries;
	__UHEAP_MARKEND;
	}


LOCAL_C void TestTcbCaps()
//
//	Test with tcb capabilities
//
	{
	__UHEAP_MARK;
	r=TheFs.FileSystemName(fsname,gTheDriveNum);
	test(r==KErrNone);	
	r = DismountFileSystem(TheFs, fsname, gTheDriveNum);
	test(r==KErrPermissionDenied);
	r = MountFileSystem(TheFs, fsname, gTheDriveNum);
	test(r==KErrPermissionDenied);
	r=TheFs.SetDriveName(gTheDriveNum,KDriveName);
	test(r==KErrPermissionDenied);
#ifndef __WINS__
 	r=TheFs.SetVolumeLabel(KVolLable, gTheDriveNum);
	test(r==KErrPermissionDenied);
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
	test(aStat1==KErrPermissionDenied);
	test(aStat2==KErrPermissionDenied);
	test(aStat3==KErrNone);
	test(aStat4==KErrNone);

	r=TheFs.SetSessionPath(systestname);
	test(r==KErrPermissionDenied);
	
//Test RRawDisk class
	r=rawdisk.Open(TheFs,gTheDriveNum);
	test(r==KErrNone);
    rawdisk.Close();

	RDirtest();

#ifdef __WINS__
	if (User::UpperCase(driveBuf[0]) != 'C')
#endif
		{
		//Test RFormat class
		r=format.Open(TheFs,driveBuf,EHighDensity,count);
		test(r==KErrPermissionDenied);

		while(count)	
			{
			TInt r=format.Next(count);
			test(r==KErrNone);
			}
		format.Close();
		}

	driveBuf[0]=(TText)gDriveToTest;
	r=TheFs.ScanDrive(driveBuf);
	test((r==KErrPermissionDenied)||(r==KErrNotSupported));
	r=TheFs.CheckDisk(driveBuf);
	test((r==KErrPermissionDenied)||(r==KErrNotSupported));
	__UHEAP_MARKEND;
	}

LOCAL_C void TestCaps()
//
//	test format etc that require certain capabilities
//
	{
	__UHEAP_MARK;
#ifndef __REMOVE_PLATSEC_DIAGNOSTIC_STRINGS__
	const char myDiagMsg[] = "Capability Check Failure";
#endif //!__REMOVE_PLATSEC_DIAGNOSTIC_STRINGS__
	r=RProcess().HasCapability(ECapabilityTCB, __PLATSEC_DIAGNOSTIC_STRING(myDiagMsg));
	test(r);

	driveBuf[0]=(TText)gDriveToTest;
	r=TheFs.SessionPath(temp);
	test(r==KErrNone);
	test.Printf(_L("Session path: %S"),&temp);
	r=TheFs.CreatePrivatePath(gTheDriveNum);
	test(r==KErrNone || r== KErrAlreadyExists);

	TBuf<18> tempPri;
	r=TheFs.PrivatePath(tempPri);
	test(r==KErrNone);
	theprivatepath = _L("?:");
	theprivatepath.Append(tempPri);

	TestTcbCaps();

	TFileName thesessionpath;
	r=TheFs.SetSessionToPrivate(gTheDriveNum);
	test(r==KErrNone);
	r=TheFs.SessionPath(thesessionpath);
	test(r==KErrNone);
	test(thesessionpath == theprivatepath);
	__UHEAP_MARKEND;
	}


TFileName dirName;

LOCAL_C void ScanDir(const TDesC& aName, CDirScan::TScanDirection aDirection, TInt aError)
	{
	CDirScan* scanner = NULL;
	TRAP(r, scanner = CDirScan::NewL(TheFs));
	test(r == KErrNone && scanner);

	TRAP(r, scanner->SetScanDataL(aName,KEntryAttDir,ESortByName|EAscending,aDirection));
	test(r == KErrNone);
	
	CDir *entryList=NULL;
	for (;;)
		{
		TRAP(r, scanner->NextL(entryList));
		test(r == aError);
		if (entryList==NULL)
			break;
		TInt count=entryList->Count();
		while (count--)
			{
			TEntry data=(*entryList)[count];
			TBuf<KMaxFileName> path=scanner->AbbreviatedPath();
			dirName = path;
			dirName.Append(data.iName);
			test.Printf(_L("    %S\n"),&dirName);
			
			}
		delete entryList;
		entryList=NULL;
		}
	delete scanner;

	}

/**
The following test, tests CFileMan and CDirScan API on folders private and sys
to confirm that any operation on these folders for any app with incorrect capability
returns KErrPermissionDenied. This test step was added as a result of DEF051428
("PlatSec: Incorrect errors returned by f32")
*/
LOCAL_C void TestCaging()
	{
	CFileMan* fMan=CFileMan::NewL(TheFs);
	TInt r;
	if(fMan!=NULL)
		{		
		
		// Checking the private path
		TBuf<30> privatepath;
		r=TheFs.PrivatePath(privatepath);
		test.Printf(_L("Private Path is=%S"),&privatepath);
		
		r = TheFs.MkDir(_L("\\Caged\\"));
		test(r==KErrNone || r==KErrAlreadyExists);
		
		CDir* entryCount=NULL;
		r=TheFs.GetDir(_L("\\*.*"),KEntryAttNormal,ESortNone,entryCount);
		test(r==KErrNone);
		TInt rootCount= entryCount->Count();
		
		delete entryCount;
		entryCount=NULL;


		//Testing Copy
		CDir* entryCount2=NULL;
		r=fMan->Copy(_L("\\sys\\"),_L("\\Caged\\"));
		test(r == KErrPermissionDenied);
		r=fMan->Copy(_L("\\*"),_L("\\Caged\\"));
		test(r == KErrNone);
		
		r=TheFs.GetDir(_L("\\Caged\\*.*"),KEntryAttNormal,ESortNone,entryCount2);
		test(r==KErrNone);
		TInt cagedCount= entryCount2->Count();
		
		test(cagedCount==rootCount);
		
		delete entryCount2;
		entryCount2=NULL;
	
		r=fMan->Copy(_L("\\private\\two\\moo"),_L("\\private\\two\\mew")); 
		test(r == KErrPermissionDenied);
	
		// Create a test file
		RFile testFile;
		r = testFile.Replace(TheFs, _L("\\capTest"),EFileWrite);
		test(r==KErrNone || r==KErrAlreadyExists);
		testFile.Close();
		
		TFileName name;
		name = privatepath;
		name.Append(_L("privateFile.tst"));
		RFile privateFile;
		r = privateFile.Replace(TheFs, name,EFileWrite);
		test(r==KErrNone || r==KErrAlreadyExists);
		privateFile.Close();

	
		r=fMan->Copy(_L("\\capTest"),_L("\\private\\two\\moo")); 
		test(r == KErrPermissionDenied);
		r=fMan->Copy(_L("\\capTest"),_L("\\sys\\bin\\moo")); 
		test(r == KErrPathNotFound); 
		r=fMan->Copy(_L("\\sys\\bin\\capTest"),_L("\\sys\\bin\\moo")); 
		test(r == KErrPermissionDenied);
		r=fMan->Copy(_L("\\capTest"),_L("\\sys\\capTest")); 
		test(r == KErrNone || r==KErrAlreadyExists); 
		r=fMan->Copy(_L("\\sys\\*"),_L("\\"));
		test (r==KErrPermissionDenied);
		r=fMan->Copy(name,_L("\\sys\\"));
		test(r==KErrNone);

		// Testing Move
		r=fMan->Move(_L("\\private\\two\\moo"),_L("\\private\\one\\moo"));
		test(r == KErrPermissionDenied);
		r=fMan->Move(_L("\\private\\two\\moo.."),_L("\\private\\one\\moo"));
		test(r == KErrPermissionDenied);
		r=fMan->Move(_L("\\private\\two\\moo"),_L("\\private\\one\\moo.."));
		test(r == KErrPermissionDenied);
		r=fMan->Move(name,_L("\\privateFile.tst"));
		test(r == KErrNone);
		r=fMan->Move(_L("\\privateFile.tst"),name);
		test(r == KErrNone);


		// Testing Attribs
		r=fMan->Attribs(_L("\\private\\two\\moo"),KEntryAttReadOnly,0,TTime(0)); 
		test(r == KErrPermissionDenied);
		r=fMan->Attribs(_L("\\private\\moo"),KEntryAttReadOnly,0,TTime(0)); 
		test(r == KErrPermissionDenied);
		r=fMan->Attribs(name,KEntryAttReadOnly,0,TTime(0));
		test(r == KErrNone);
		r=fMan->Attribs(name,0,KEntryAttReadOnly,TTime(0));
		test(r == KErrNone);

		// Testing Move
		r=fMan->Move(_L("\\private\\two\\moo"),_L("\\private\\one\\moo"));
		test(r == KErrPermissionDenied);
		r=fMan->Move(_L("\\private\\two\\moo.."),_L("\\private\\one\\moo"));
		test(r == KErrPermissionDenied);
		r=fMan->Move(_L("\\private\\two\\moo"),_L("\\private\\one\\moo.."));
		test(r == KErrPermissionDenied);
		r=fMan->Move(name,_L("\\privateFile.tst"));
		test(r == KErrNone);
		r=fMan->Move(_L("\\privateFile.tst"),name);
		test(r == KErrNone);


		// Testing RmDir
		r=fMan->RmDir(_L("\\private\\"));
		test(r == KErrPermissionDenied);
		r=fMan->RmDir(_L("\\private\\two\\"));
		test(r == KErrPermissionDenied);
		r=fMan->RmDir(_L("\\private\\tw?\\"));
		test(r == KErrPermissionDenied);
		r=fMan->RmDir(_L("\\sys\\"));
		test(r == KErrPermissionDenied);
		
		
		// Testing Rename
		r=fMan->Rename(_L("\\private\\two\\moo"),_L("\\private\\two\\mew")); 
		test(r == KErrPermissionDenied);
		
		// Testing Delete
		r=fMan->Delete(_L("\\private\\two\\test")); 
		test(r == KErrPermissionDenied);
		r=fMan->Delete(_L("\\private\\moo")); 
		test(r == KErrPermissionDenied);
		r=fMan->Delete(_L("\\sys\\")); 
		test(r == KErrPermissionDenied);

		//Something that actually exists in Private
		r=fMan->Rename(name,_L("\\private\\00000001\\moo")); 
		test(r == KErrNone);
		r=fMan->Rename(_L("\\private\\00000001\\moo"),name); 
		test(r == KErrNone);
		r=fMan->Copy(name,_L("\\private\\00000001\\moo")); 
		test(r == KErrNone);
		r=fMan->Delete(_L("\\private\\00000001\\moo")); 
		test(r == KErrNone);

		// Clean up the test data
		r=fMan->RmDir(_L("\\Caged\\")); 
		test(r == KErrNone);
		r=fMan->Delete(_L("\\capTest")); 
		test(r == KErrNone);
		r=fMan->Delete(name); 
		test(r == KErrNone);
		delete(fMan);
		}
	
	// CDirScan tests
	ScanDir(_L("\\"), CDirScan::EScanUpTree, KErrNone);
	ScanDir(_L("\\"), CDirScan::EScanDownTree, KErrNone);
	ScanDir(_L("\\private\\"), CDirScan::EScanDownTree, KErrPermissionDenied);
	ScanDir(_L("\\private\\"), CDirScan::EScanUpTree, KErrPermissionDenied);
	ScanDir(_L("\\sys\\"), CDirScan::EScanDownTree, KErrPermissionDenied);
	ScanDir(_L("\\sys\\"), CDirScan::EScanUpTree, KErrPermissionDenied);
	}

void TestSystemDrive()
	{
	test.Next(_L("TestSystemDrive"));
	TDriveNumber drive = RFs::GetSystemDrive();
	test.Printf(_L("System Drive is %c:\n"), 'A'+drive);
	for(TInt i=EDriveA; i<=EDriveZ; i++)
		{
		TDriveInfo info;
		test(TheFs.Drive(info, i) == KErrNone);
		if(info.iType==EMediaHardDisk || info.iType==EMediaFlash || info.iType==EMediaNANDFlash)
			{
			if(info.iDriveAtt & (KDriveAttLocal|KDriveAttInternal))
				{
				if(!(info.iDriveAtt & (KDriveAttRom|KDriveAttRedirected|KDriveAttSubsted|KDriveAttRemovable)))
					{
					test.Printf(_L("Setting %c:"), 'A'+i);
					TInt ret = TheFs.SetSystemDrive((TDriveNumber)i);
					test.Printf(_L("%d\n"), ret);
					test(ret == KErrNone || ret == KErrAlreadyExists);
					if(ret == KErrNone)
						{
						drive = (TDriveNumber)i;
						test.Printf(_L("Re-setting %c:"), 'A'+i);
						ret = TheFs.SetSystemDrive(drive);
						test.Printf(_L("%d\n"), ret);
						test(ret == KErrAlreadyExists);
						}
					}
				}
			}
		}
	TChar drvchar = RFs::GetSystemDriveChar();
	test(drvchar == (TChar)('A' + drive)); 
	test(drive == RFs::GetSystemDrive());
	}
	
LOCAL_C void CleanupL()
//
// Clean up tests
//
	{
	test.Next(_L("Delete test directory"));
	CFileMan* fMan=CFileMan::NewL(TheFs);
	TInt r=fMan->RmDir(gSessionPath);
	test(r==KErrNone || KErrPathNotFound);
	delete fMan;
	}

GLDEF_C void CallTestsL(/*TChar aDriveLetter*/)
//
// Do all tests
//
	{
	if( !PlatSec::IsCapabilityEnforced(ECapabilityTCB))
		{
		test.Printf(_L("Capability ECapabilityTCB not enabled - leaving t_dctcb"));
		test.Printf(_L("\n")); // Prevent overwrite by next print
		return;
		}

	TurnAllocFailureOff();
	TheFs.CharToDrive(gDriveToTest,gTheDriveNum);
	
	TBuf<30> sesspath;
	sesspath=_L("?:\\");
	sesspath[0] = (TText)gDriveToTest;

	TInt r= TheFs.SetSessionPath(sesspath);
	test(r==KErrNone);

	//cleanup from previous run of this test
	TBuf<20> delDir;
	CFileMan* fMan=CFileMan::NewL(TheFs);
	delDir=KResourcePath;
	delDir[0]=(TText)gDriveToTest;
	r=fMan->RmDir(delDir);
	test(r==KErrNone || KErrNotFound);
	delDir=KSystemPath;
	delDir[0]=(TText)gDriveToTest;
	r=fMan->RmDir(delDir);
	test(r==KErrNone || KErrNotFound);
	delDir=KPrivatePath;
	delDir[0]=(TText)gDriveToTest;
	r=fMan->RmDir(delDir);
	test(r==KErrNone || KErrNotFound);
	delete fMan;

	//check double mode ie that Defpath still works	
	RFs fs1;
	RFs fs2;
	
	r=fs1.Connect();
	test(r==KErrNone);
	r=fs1.SessionPath(sesspath);
	test(r==KErrNone);
	test.Printf(_L("session1 Path=%S"),&sesspath);

	TBuf<30> privatepath;
	r=fs1.SetSessionToPrivate(gTheDriveNum);
	test(r==KErrNone);
	r=fs1.PrivatePath(privatepath);
	test(r==KErrNone);
	r=privatepath.Compare(KExpectedPrivatePath());
	test(r==0);
	r=fs1.SessionPath(sesspath);
	test(r==KErrNone);
	r=privatepath.Compare(sesspath.Mid(KPathPosition));
	test(r==0);
	r=fs1.CreatePrivatePath(gTheDriveNum);
	test(r==KErrNone);
	fs1.Close();

	r=fs2.Connect();
	test(r==KErrNone);
	r=fs2.SessionPath(sesspath);
	test(r==KErrNone);
	test.Printf(_L("session2 Path=%S"),&sesspath);
	fs2.Close();

	TestCaps();
	TestCaging();
	TestPathCheck();
	TestSystemDrive();

	test.Printf(_L("No of files open=%d"), TheFs.ResourceCount());

	CleanupL();
	}
