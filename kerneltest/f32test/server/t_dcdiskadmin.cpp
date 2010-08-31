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
// f32test\server\t_dcdiskadmin.cpp
// 
//

#define __E32TEST_EXTENSION__
#include <f32file.h>
#include <e32test.h>
#include <e32std.h>
#include <e32std_private.h>
#include <e32svr.h>
#include "t_server.h"

GLDEF_D RTest test(_L("T_DCDiskadmin"));
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

TInt theDrive=0;
TCapability TheCaps;
TBuf<4> driveBuf=_L("?:\\");
RFormat format;
TInt count;
RRawDisk rawdisk;
RFile file1;
RFile file2;
RDir	dir;
CDir*	dirEntries;
TInt r;
TBuf<40> fsname;
TBuf<40> systestname;
TBuf<40> pritestname;
TBuf<40> restestname;
TBuf<40> theprivatepath;
TBuf<40> pritestfalseidname;
TBuf<40> mkdirname;
TFileName fromTemp;

TBuf<40> shortfilename;
TBuf<40> longfilename;

TBuf<30> dirNameBuf;
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
TBuf<30> realName;
TTime testtime;
TBuf<20> oldName;

TBuf<25> temp;

TEntry entry;



LOCAL_C void systemRFstest()
//
//
//
	{

	//system		
	systestname=KSystemPath;
	systestname[0]=(TText)('A' + gTheDriveNum);

	mkdirname.Zero();
	mkdirname.Append(systestname);
	mkdirname.Append(KMkDirSub);
	r=TheFs.MkDirAll(mkdirname);	
	test_Value(r, r == KErrPermissionDenied);

	r=TheFs.RmDir(mkdirname);	
	test_Value(r, r == KErrPermissionDenied);

	r=TheFs.SetSubst(systestname,EDriveS);
	test_Value(r, r == KErrPermissionDenied);
	
	r=TheFs.SetSessionPath(systestname);
	test_Value(r, r == KErrPermissionDenied);

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
	test_Value(r, r == KErrPermissionDenied);

	r=TheFs.GetLongName(systestfile1, longfilename);
	test_Value(r, r == KErrPermissionDenied);

	r=file1.Create(TheFs,oldName,EFileWrite);
	test_Value(r, r == KErrNone || r==KErrAlreadyExists);
	file1.Close();

	r=TheFs.Replace(oldName,systestfile);
	test_Value(r, r == KErrPermissionDenied);
	
	r=TheFs.Rename(systestfile,systestfile1);
	test_Value(r, r == KErrPermissionDenied);
	
	r=TheFs.Entry(systestfile1,entry);
	test_Value(r, r == KErrPermissionDenied);

	r=TheFs.SetEntry(systestfile1,testtime,KEntryAttNormal,KEntryAttReadOnly);
	test_Value(r, r == KErrPermissionDenied);

	r=TheFs.Delete(systestfile1);
	test_Value(r, r == KErrPermissionDenied);


	// DEF141257: Security Issue in File Server 
	// Test that we can't access the system drive by preceding it with a dot character
	_LIT(KSystemPathDot, "?:\\.Sys\\");

	// try creating a file in "\.SYS"...
	TBuf<40> SysTestFileDot;
	SysTestFileDot=KSystemPathDot;
	SysTestFileDot[0]=(TText)('A' + gTheDriveNum);
	SysTestFileDot.Append(KFileSys);
	RFile file;
	r = file.Replace(TheFs, SysTestFileDot, EFileWrite);
	test_Value(r, r == KErrPathNotFound);

	// try creating a subst drive for "\.SYS"...
	TBuf<40> SysTestNameDot;
	SysTestNameDot=KSystemPathDot;
	SysTestNameDot[0]=(TText)('A' + gTheDriveNum);
	r=TheFs.SetSubst(SysTestNameDot,EDriveA);
	test_KErrNone(r);

	// try creating a file using substituted drive...
	TBuf<40> SubstTestFile = _L("A:\\");
	SubstTestFile.Append(KFileSys);

	r = file.Replace(TheFs, SubstTestFile, EFileWrite);
	test_Value(r, r == KErrPathNotFound);

	r = TheFs.SetSubst(_L(""),EDriveA);
	test_KErrNone(r);



	// try listing files in "\.SYS"
	_LIT(KWildPathDot, "Z:\\.SYS\\");
	TFindFile finder(TheFs);
	CDir* dir = NULL;
	r=finder.FindWildByDir(KWildFile, KWildPathDot, dir);
	test_Value(r, r == KErrNotFound);
	delete dir;

	// Deliberately create a directory called "\.SYS"
	// and verify shortname is NOT the same as "SYS"
	mkdirname.Zero();
	mkdirname.Append(KSystemPathDot);
	mkdirname[0]=(TText)('A' + gTheDriveNum);
	r=TheFs.MkDirAll(mkdirname);	
	test_KErrNone(r);

	r=TheFs.GetShortName(mkdirname, shortfilename);
	test_Value(r, r == KErrNone || r==KErrNotSupported);	// short names not supported on LFFS
//	r = shortfilename.Compare(_L("SYS~1"));
//	test_KErrNone(r);
	r = shortfilename.Compare(_L("SYS"));
	test (r != 0);

	r = TheFs.RmDir(mkdirname);
	test_KErrNone(r);

	// Deliberately create a directory called "\..SYS"
	// and verify shortname is NOT the same as "SYS"
	_LIT(KSystemPathDotDot, "?:\\..Sys\\");
	mkdirname.Zero();
	mkdirname.Append(KSystemPathDotDot);
	mkdirname[0]=(TText)('A' + gTheDriveNum);
	r=TheFs.MkDirAll(mkdirname);	
	test_KErrNone(r);

	r=TheFs.GetShortName(mkdirname, shortfilename);
	test_Value(r, r == KErrNone || r==KErrNotSupported);	// short names not supported on LFFS
//	r = shortfilename.Compare(_L("_.SYS"));
//	test_KErrNone(r);
	r = shortfilename.Compare(_L("SYS"));
	test (r != 0);

	r = TheFs.RmDir(mkdirname);
	test_KErrNone(r);
	}


LOCAL_C void resourceRFstest()
//
//
//
	{
//resource		
	restestname=KResourcePath;
	restestname[0]=(TText)('A' + gTheDriveNum);
	
	mkdirname.Zero();
	mkdirname.Append(restestname);
	mkdirname.Append(KMkDirSub);
	r=TheFs.MkDirAll(mkdirname);	
	test_Value(r, r == KErrPermissionDenied);

	TheFs.RmDir(mkdirname);
	test_Value(r, r == KErrPermissionDenied);
	
	// Change due to defect DEF099546 fix
	// TCB capability is required for following operation.
	r=TheFs.SetSubst(restestname,EDriveS);
	//test_KErrNone(r);
	test_Value(r, r == KErrPermissionDenied);
	
	// SetSubst will fail.
	//r=TheFs.RealName(_L("S:\\File.XXX"),realName);
	//test_KErrNone(r);

	//r=TheFs.SetSubst(_L(""),EDriveS); 
	//test_KErrNone(r);

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
	test(r == KErrNone || KErrPathNotFound);
//	test_Value(r, r == KErrNone || r == KErrPathNotFound);

	r=TheFs.GetLongName(restestfile1, longfilename);
	test(r == KErrNone || KErrPathNotFound);
//	test_Value(r, r == KErrNone || r == KErrPathNotFound);

	r=file1.Create(TheFs,oldName,EFileWrite);
	test_Value(r, r == KErrNone || r==KErrAlreadyExists);
	file1.Close();

	r=TheFs.Replace(oldName,restestfile);
	test_Value(r, r == KErrPermissionDenied);
	
	r=TheFs.Rename(restestfile,restestfile1);
	test_Value(r, r == KErrPermissionDenied);
	
	r=TheFs.Entry(restestfile1,entry);
	test(r == KErrNone || KErrPathNotFound);
//	test_Value(r, r == KErrNone || r == KErrPathNotFound);

	r=TheFs.SetEntry(restestfile1,testtime,KEntryAttNormal,KEntryAttReadOnly);
	test_Value(r, r == KErrPermissionDenied);

	r=TheFs.Delete(restestfile1);
	test_Value(r, r == KErrPermissionDenied);
	}


LOCAL_C void privateFalseIDRFstest()
//
//
//
	{
	pritestfalseidname=KPrivateFalseID;
	pritestfalseidname[0]=(TText)('A' + gTheDriveNum);

	mkdirname.Zero();
	mkdirname.Append(pritestfalseidname);
	mkdirname.Append(KMkDirSub);

	r=TheFs.MkDirAll(mkdirname);	
	test_Value(r, r == KErrPermissionDenied);

	r=TheFs.RmDir(mkdirname);	
	test_Value(r, r == KErrPermissionDenied);

	r=TheFs.SetSubst(pritestfalseidname,EDriveS);
	test_Value(r, r == KErrPermissionDenied); 
	
	r=TheFs.SetSessionPath(pritestfalseidname);
	test_Value(r, r == KErrPermissionDenied);

	TheFs.NotifyChange(ENotifyAll,aStat2,pritestfalseidname);
	test(aStat2==KErrPermissionDenied);

	pritestfile=KPrivateFalseID;
	pritestfile[0]=(TText)('A' + gTheDriveNum);
	pritestfile1=pritestfile;
	pritestfile.Append(KFilePri2);
	pritestfile1.Append(KFilePri3);
	
	oldName=KOldFile;
	oldName[0]=(TText)gDriveToTest;

	r=TheFs.GetShortName(pritestfile, shortfilename);
	test_Value(r, r == KErrPermissionDenied);

	r=TheFs.GetLongName(pritestfile1, longfilename);
	test_Value(r, r == KErrPermissionDenied);

	r=file1.Create(TheFs,oldName,EFileWrite);
	test_Value(r, r == KErrNone || r==KErrAlreadyExists);
	file1.Close();

	r=TheFs.Replace(oldName,pritestfile);
	test_Value(r, r == KErrPermissionDenied);
	
	r=TheFs.Rename(pritestfile,pritestfile1);
	test_Value(r, r == KErrPermissionDenied);
	
	r=TheFs.Entry(pritestfile1,entry);
	test_Value(r, r == KErrPermissionDenied);

	r=TheFs.SetEntry(pritestfile1,testtime,KEntryAttNormal,KEntryAttReadOnly);
	test_Value(r, r == KErrPermissionDenied);

	r=TheFs.Delete(pritestfile1);
	test_Value(r, r == KErrPermissionDenied);

	}



LOCAL_C void privateRFstest()
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
	test_Value(r, r == KErrPermissionDenied);

	r=TheFs.RmDir(mkdirname);	
	test_Value(r, r == KErrPermissionDenied);

	r=TheFs.SetSubst(pritestname,EDriveS);
	test_Value(r, r == KErrPermissionDenied); 
	
	r=TheFs.SetSessionPath(pritestname);
	test_Value(r, r == KErrPermissionDenied);

	TheFs.NotifyChange(ENotifyAll,aStat2,pritestname);
	test(aStat2==KErrPermissionDenied);

	pritestfile=KPrivatePath;
	pritestfile[0]=(TText)('A' + gTheDriveNum);
	pritestfile1=pritestfile;
	pritestfile.Append(KFilePri2);
	pritestfile1.Append(KFilePri3);
	
	oldName=KOldFile;
	oldName[0]=(TText)gDriveToTest;

	r=TheFs.GetShortName(pritestfile, shortfilename);
	test_Value(r, r == KErrPermissionDenied);

	r=TheFs.GetLongName(pritestfile1, longfilename);
	test_Value(r, r == KErrPermissionDenied);

	r=file1.Create(TheFs,oldName,EFileWrite);
	test_Value(r, r == KErrNone || r==KErrAlreadyExists);
	file1.Close();

	r=TheFs.Replace(oldName,pritestfile);
	test_Value(r, r == KErrPermissionDenied);
	
	r=TheFs.Rename(pritestfile,pritestfile1);
	test_Value(r, r == KErrPermissionDenied);
	
	r=TheFs.Entry(pritestfile1,entry);
	test_Value(r, r == KErrPermissionDenied);

	r=TheFs.SetEntry(pritestfile1,testtime,KEntryAttNormal,KEntryAttReadOnly);
	test_Value(r, r == KErrPermissionDenied);

	r=TheFs.Delete(pritestfile1);
	test_Value(r, r == KErrPermissionDenied);

	}


LOCAL_C void privateSIDRFstest()
//
//
//
	{
//private/UID
	theprivatepath[0]=(TText)gDriveToTest;	
	test.Printf(_L("the Private Path = %S"),&theprivatepath);

	mkdirname.Zero();
	mkdirname.Append(theprivatepath);
	mkdirname.Append(KMkDirSub);
	r=TheFs.MkDirAll(mkdirname);	
	test_KErrNone(r);

	r=TheFs.RmDir(mkdirname);	
	test_KErrNone(r);

	r=TheFs.SetSubst(theprivatepath,EDriveS);	
	test_KErrNone(r);

	r=TheFs.RealName(_L("S:\\File.XXX"),realName);
	test_KErrNone(r);

	r=TheFs.SetSubst(_L(""),EDriveS);	
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
	test(r == KErrNone || KErrPathNotFound);
//	test_Value(r, r == KErrNone || r == KErrPathNotFound);

	r=TheFs.GetLongName(pritestfile1, longfilename);
	test(r == KErrNone || KErrPathNotFound);
//	test_Value(r, r == KErrNone || r == KErrPathNotFound);

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
//
//
	{
//RFile testing with session path set to //system//

	r=TheFs.SetSessionPath(systestname);
	test_Value(r, r == KErrPermissionDenied);

	r=file1.Temp(TheFs,systestname,fromTemp,EFileWrite);
	test_Value(r, r == KErrPermissionDenied);

	TBuf<25> sysfilename;
	sysfilename.Append(systestname);
	sysfilename.Append(KFileSys);
	r=file1.Create(TheFs,sysfilename,EFileWrite);
	test_Value(r, r == KErrPermissionDenied);

	r=file1.Open(TheFs,sysfilename,EFileWrite);
	test_Value(r, r == KErrPermissionDenied);

	r=file1.Open(TheFs,sysfilename,EFileRead);
	test_Value(r, r == KErrPermissionDenied);

	r=file1.Replace(TheFs,sysfilename,EFileWrite);
	test_Value(r, r == KErrPermissionDenied);

    TFindFile finder(TheFs);
    CDir* dir = NULL;
    r=finder.FindWildByDir(KWildFile, KWildPath, dir);
	if (!(r==KErrPermissionDenied))
        test.Printf(_L("T_DCDISKADMIN: test find wildcards r = %d (expected KErrPermissionDenied)\n"), r);
	test_Value(r, r == KErrPermissionDenied);
	delete dir;
	}

LOCAL_C void resourceRFiletest()
//
//
//
	{
//RFile testing with session path set to //resource//
	r=TheFs.SetSessionPath(restestname);
	test_KErrNone(r);

	r=file1.Temp(TheFs,restestname,fromTemp,EFileWrite);
	test_Value(r, r == KErrPermissionDenied);
	file1.Close();

	r=file1.Create(TheFs,KFileRes,EFileWrite);
	test_Value(r, r == KErrPermissionDenied);
	file1.Close();

	r=file1.Open(TheFs,KFileRes,EFileWrite);
	test_Value(r, r == KErrPermissionDenied);
	file1.Close();

	r=file1.Open(TheFs,KFileRes,EFileRead|EFileShareReadersOnly);
	test_Value(r, r == KErrNone || r==KErrPathNotFound);
	file1.Close();

	r=file1.Open(TheFs,KFileRes,EFileShareReadersOrWriters|EFileRead);
	test_Value(r, r == KErrNone || r==KErrPathNotFound);
	file1.Close();

	r=file1.Open(TheFs,KFileRes,EFileShareReadersOrWriters|EFileWrite);
	test_Value(r, r == KErrPermissionDenied);
	file1.Close();

	r=file1.Open(TheFs,KFileRes,EFileShareReadersOnly);
	test_KErrNone(r);

	r=file1.ChangeMode(EFileShareExclusive);	
	test_KErrNone(r);

	//this operation is prevented as you can not open a file for write access in the resource directory
	r=file1.Rename(KFileRes3);
	test_Value(r, r == KErrPermissionDenied || r==KErrAccessDenied);
	file1.Close();

	r=file1.Replace(TheFs,KFileRes,EFileWrite);
	test_Value(r, r == KErrPermissionDenied);
	file1.Close();

	}


LOCAL_C void privatefalseIDRFiletest()
//
//RFile testing with session path set to //Private//falseID
//
	{
	r=TheFs.SetSessionPath(pritestfalseidname);
	test_Value(r, r == KErrPermissionDenied);

	r=file1.Temp(TheFs,pritestfalseidname,fromTemp,EFileWrite);
	test_Value(r, r == KErrPermissionDenied);

	TBuf<25> prifilename;
	prifilename.Append(pritestname);
	prifilename.Append(KFileSys);

	r=file1.Create(TheFs,prifilename,EFileWrite);
	test_Value(r, r == KErrPermissionDenied);

	r=file1.Open(TheFs,prifilename,EFileWrite);
	test_Value(r, r == KErrPermissionDenied);

	r=file1.Open(TheFs,prifilename,EFileRead);
	test_Value(r, r == KErrPermissionDenied);

	r=file1.Replace(TheFs,prifilename,EFileWrite);
	test_Value(r, r == KErrPermissionDenied);
	}


LOCAL_C void privateRFiletest()
//
//RFile testing with session path set to //Private//
//
	{
	r=TheFs.SetSessionPath(pritestname);
	test_Value(r, r == KErrPermissionDenied);

	r=file1.Temp(TheFs,pritestname,fromTemp,EFileWrite);
	test_Value(r, r == KErrPermissionDenied);

	TBuf<25> prifilename;
	prifilename.Append(pritestname);
	prifilename.Append(KFileSys);

	r=file1.Create(TheFs,prifilename,EFileWrite);
	test_Value(r, r == KErrPermissionDenied);

	r=file1.Open(TheFs,prifilename,EFileWrite);
	test_Value(r, r == KErrPermissionDenied);
	
	r=file1.Open(TheFs,prifilename,EFileRead);
	test_Value(r, r == KErrPermissionDenied);

	r=file1.Replace(TheFs,prifilename,EFileWrite);
	test_Value(r, r == KErrPermissionDenied);
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
	test_Value(r, r == KErrNone || r==KErrAlreadyExists);
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
	test_Value(r, r == KErrNone || r==KErrAlreadyExists);
	file1.Close();
	}


LOCAL_C void RDirtest()
//
//
//
	{
	//system
	dirNameBuf.Zero();
	dirNameBuf = KSystemPath;
	dirNameBuf[0]=(TText)gDriveToTest;
	r=dir.Open(TheFs,dirNameBuf,KEntryAttNormal);
	test_Value(r, r == KErrPermissionDenied);
	dir.Close();
	r=TheFs.GetDir(dirNameBuf,KEntryAttMatchMask,ESortByName,dirEntries);
	test_Value(r, r == KErrPermissionDenied);
	dirNameBuf.Zero();
	delete dirEntries;
	//Private//falseid
	dirNameBuf=KPrivateFalseID;
	dirNameBuf[0]=(TText)gDriveToTest;
	r=dir.Open(TheFs,dirNameBuf,KEntryAttNormal);
	test_Value(r, r == KErrPermissionDenied);
	dir.Close();
	r=TheFs.GetDir(dirNameBuf,KEntryAttMatchMask,ESortByName,dirEntries);
	test_Value(r, r == KErrPermissionDenied);
	dirNameBuf.Zero();
	delete dirEntries;
	//Private
	dirNameBuf=KPrivatePath;
	dirNameBuf[0]=(TText)gDriveToTest;
	r=dir.Open(TheFs,dirNameBuf,KEntryAttNormal);
	test_Value(r, r == KErrPermissionDenied);
	dir.Close();
	r=TheFs.GetDir(dirNameBuf,KEntryAttMatchMask,ESortByName,dirEntries);
	test_Value(r, r == KErrPermissionDenied);
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
	test_Value(r, r == KErrNone || r==KErrPathNotFound || r==KErrNotFound);
	r=TheFs.GetDir(dirNameBuf,KEntryAttMatchMask,ESortByName,dirEntries);
	test_Value(r, r == KErrNone || r==KErrPathNotFound || r==KErrNotFound);
	dir.Close();
	delete dirEntries;
	}


LOCAL_C void DiskAdminTest()
//
//	test diskadministration capabilitiy
//
	{
	r=TheFs.FileSystemName(fsname,gTheDriveNum);
	test_KErrNone(r);
	r = DismountFileSystem(TheFs, fsname, gTheDriveNum);
	test_KErrNone(r);
//	r=TheFs.RemoveFileSystem(fsname);	//can not test due to bug else where fix exists
//	test_Value(r, r == KErrPermissionDenied);
//	r=TheFs.AddFileSystem(fsname);
//	test_Value(r, r == KErrPermissionDenied);
	r = MountFileSystem(TheFs, fsname, gTheDriveNum);
	test_KErrNone(r);
	r=TheFs.SetDriveName(gTheDriveNum,KDriveName);
	test_KErrNone(r);
	r=TheFs.SetVolumeLabel(KVolLable, gTheDriveNum);
	test_Value(r, r == KErrNone || r==KErrNotSupported);

	systemRFstest();
	resourceRFstest();
	privateRFstest();
	privateSIDRFstest();
	privateFalseIDRFstest();

	systemRFiletest();
	resourceRFiletest();
	privateRFiletest();
	privateSIDRFiletest();
	privatefalseIDRFiletest();

//disk changes to sys and pri paths should have completed these
	test(aStat4 == KRequestPending);
	TheFs.NotifyChangeCancel(aStat4);
	test(aStat4==KErrCancel);
	
	User::WaitForRequest(aStat3);
	test(aStat1==KErrPermissionDenied);
	test(aStat2==KErrPermissionDenied);
	test(aStat3==KErrNone);
	
	r=TheFs.SetSessionPath(systestname);
	test_Value(r, r == KErrPermissionDenied);
	
//Test RRawDisk class
	r=rawdisk.Open(TheFs,gTheDriveNum);
	test_Value(r, r == KErrPermissionDenied);
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
	
	DiskAdminTest();

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
	test(r == KErrNone || KErrPathNotFound);
//	test_Value(r, r == KErrNone || r == KErrPathNotFound);
	delete fMan;
	}

GLDEF_C void CallTestsL(/*TChar aDriveLetter*/)
//
// Do all tests
//
	{
	if( !PlatSec::IsCapabilityEnforced(ECapabilityDiskAdmin))
		{
		test.Printf(_L("Capability ECapabilityDiskAdmin not enabled - leaving t_dcdiskadmin"));
		test.Printf(_L("\n")); // Prevent overwrite by next print
		return;
		}

	TurnAllocFailureOff();
	TheFs.CharToDrive(gDriveToTest,gTheDriveNum);
	
	TBuf<30> sesspath;
	sesspath=_L("?:\\");
	sesspath[0] = (TText)gDriveToTest;

	TInt r= TheFs.SetSessionPath(sesspath);
	test_KErrNone(r);

	//cleanup from previous run of this test
	TBuf<2> cmd;
	cmd.SetLength(1);
	cmd[0] = (TText)gDriveToTest;
	RProcess tp;
	r=tp.Create(_L("clean_prepdc.exe"),sesspath);
	test_KErrNone(r);
	{
	TRequestStatus ps;
	tp.Logon(ps);
	tp.Resume();
	tp.Close();
	User::WaitForRequest(ps);
	}

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
