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
// f32test\server\t_locate.cpp
// 
//
#define __E32TEST_EXTENSION__
#include <f32file.h>
#include <e32test.h>
#include "t_server.h"



GLDEF_D RTest test(_L("T_LOCATE"));

LOCAL_D TFileName gPath1;
LOCAL_D TFileName gPath2;
LOCAL_D TFileName gPath3;
LOCAL_D TFileName gPath4;
LOCAL_D TFileName gPath5;


LOCAL_D TFileName gRemovableDriveFile;
LOCAL_D TFileName gInternalDriveFile;
LOCAL_D TFileName gInternalDriveFile2;


LOCAL_D TChar removableDriveLetter;  
LOCAL_D TChar internalDriveLetter; 


LOCAL_D TInt removableFlag=0;
LOCAL_D TInt internalFlag=0;



LOCAL_C void Md(const TDesC& aDirName)
//
// Make a dir
//
	{

	TInt r=TheFs.MkDirAll(aDirName);
	if (r == KErrCorrupt)
		test.Printf(_L("Media corruption; previous test may have aborted; else, check hardware\n"));
	else if (r == KErrNotReady)
		test.Printf(_L("No medium present / drive not ready, previous test may have hung; else, check hardware\n"));
	test_Value(r, r == KErrNone || r == KErrAlreadyExists);
	}

LOCAL_C void Mf(const TDesC& aFileName)
//
// Make a file
//
	{

	RFile file;
	TInt r = file.Replace(TheFs,aFileName,0);
	if (r == KErrPathNotFound)
		{
		test.Printf(_L("Mf: Path Not Found\n"));
		Md(aFileName);
		r=file.Replace(TheFs,aFileName,0);
		}

	if (r == KErrCorrupt)
		test.Printf(_L("Media corruption; previous test may have aborted; else, check hardware\n"));
	else if (r == KErrNotReady)
		test.Printf(_L("No medium present / drive not ready, previous test may have hung; else, check hardware\n"));

	test_Value(r, r == KErrNone || r == KErrAlreadyExists);
	file.Close();
	}

LOCAL_C void MakeLocateTestDirectoryStructure()
//
// Create files for test
//
	{
	test.Next(_L("Create LOCTEST directories"));
	Md(_L("\\F32-TST\\LOCTEST\\BIN1\\"));
	Md(_L("\\F32-TST\\LOCTEST\\BIN2\\"));
	Md(_L("\\F32-TST\\LOCTEST\\BIN3\\"));
	Md(_L("\\F32-TST\\LOCTEST\\BIN1\\BIN4\\"));
		
	
	
#if defined(_DEBUG)
	TheFs.SetErrorCondition(-47,5);
	TDriveInfo drive;
	for (TInt i=0;i<5;i++)
		{
		TInt r=TheFs.Drive(drive);
		test_KErrNone(r);
		}
	TInt r=TheFs.MkDirAll(_L("alskdjfl"));
	test_Value(r, r == -47);
	r=TheFs.MkDirAll(_L("alskdjfl"));
	test_Value(r, r == -47);
	TheFs.SetErrorCondition(KErrNone);
	r=TheFs.Drive(drive);
	test_KErrNone(r);
#endif
//
	test.Next(_L("Create LOCTEST files"));
	Mf(_L("\\F32-TST\\LOCTEST\\FILE1.AAA"));
	Mf(_L("\\F32-TST\\LOCTEST\\FILE2.BBB"));
	Mf(_L("\\F32-TST\\LOCTEST\\FILE3.CCC"));
	Mf(_L("\\F32-TST\\LOCTEST\\WORK.AAA"));
	Mf(_L("\\F32-TST\\LOCTEST\\HOME.CCC"));
	Mf(_L("\\F32-TST\\LOCTEST\\FILE.AAA"));
	Mf(_L("C:\\F32-TST\\LOCTEST\\BIN1\\FILE1.AAA"));
	Mf(_L("C:\\F32-TST\\LOCTEST\\BIN1\\WORK.AAA"));
	Mf(_L("C:\\F32-TST\\LOCTEST\\BIN1\\WORK.BBB"));
	Mf(_L("\\F32-TST\\LOCTEST\\BIN1\\FILE1.AAA"));
	Mf(_L("\\F32-TST\\LOCTEST\\BIN1\\WORK.AAA"));
	Mf(_L("\\F32-TST\\LOCTEST\\BIN1\\WORK.BBB"));
	Mf(_L("\\F32-TST\\LOCTEST\\BIN1\\CONFUSED.DOG"));
	Mf(_L("\\F32-TST\\LOCTEST\\BIN2\\FILE1.BBB"));
	Mf(_L("\\F32-TST\\LOCTEST\\BIN2\\WORK.BBB"));
	Mf(_L("\\F32-TST\\LOCTEST\\BIN2\\FILE2.BBB"));
	Mf(_L("\\F32-TST\\LOCTEST\\BIN2\\FILE3.BBB"));
	Mf(_L("\\F32-TST\\LOCTEST\\BIN3\\FILE3.CCC"));
	Mf(_L("\\F32-TST\\LOCTEST\\BIN3\\WORK.CCC"));
	Mf(_L("\\F32-TST\\LOCTEST\\BIN3\\PLAY.CCC"));
	Mf(_L("\\F32-TST\\LOCTEST\\BIN1\\BIN4\\FILE1.AAA"));
	Mf(_L("\\F32-TST\\LOCTEST\\BIN1\\BIN4\\FILE2.BBB"));
	Mf(_L("\\F32-TST\\LOCTEST\\BIN1\\BIN4\\FILE3.CCC"));
	Mf(_L("\\F32-TST\\LOCTEST\\BIN1\\BIN4\\FILE4.DDD"));
	
	
	
	}
	

LOCAL_C void CreateFilesInRemovableDrive()	
	{
	

    TInt err;
    TDriveList driveList;
    TDriveInfo info;

	 err = TheFs.DriveList(driveList);
    test_KErrNone(err);
    
    for (TInt i = 0; i < KMaxDrives; i++) 
        {
        
        if (driveList[i]) 
            {
            err = TheFs.Drive(info, i);
            test_KErrNone(err); 
                        
            if(( info.iDriveAtt  & KDriveAttRemovable ) && !( info.iDriveAtt  & KDriveAttLogicallyRemovable ))  
            	{
								
				if ( ( info.iType != EMediaNotPresent) && (info.iType != EMediaUnknown) && (info.iType != EMediaCdRom) )
					{
					TheFs.DriveToChar(i,removableDriveLetter) ;
					gRemovableDriveFile.Append (removableDriveLetter);
					gRemovableDriveFile.Append (_L(":\\F32-TST\\LOCTEST\\BIN\\FINDFILE.AAA") );

					Mf(gRemovableDriveFile);
					removableFlag=1;
					break;
					}
				else 
					continue;
      				
 
            	}
           				
            
            }
    
        }

	}
	
	

LOCAL_C void CreateFilesInInternalDrive()	
	{
	
    TInt err;
    TDriveList driveList;
    TDriveInfo info;

	 err = TheFs.DriveList(driveList);
    test_KErrNone(err);
    
    for (TInt i = 0; i < KMaxDrives; i++) 
        {
        
        if (driveList[i]) 
            {
            err = TheFs.Drive(info, i);
            test_KErrNone(err); 
                        
            if( info.iDriveAtt  & KDriveAttInternal  ) 
            	{			
				
				TheFs.DriveToChar(i,internalDriveLetter) ;
				gInternalDriveFile.Append (internalDriveLetter);
				gInternalDriveFile.Append (_L(":\\F32-TST\\LOCTEST\\BIN\\INT\\FINDINTERNALFILE.AAA") );
				
				gInternalDriveFile2.Append (internalDriveLetter);
				gInternalDriveFile2.Append (_L(":\\F32-TST\\LOCTEST\\BIN\\INT\\FINDINTERNALFILE_B.AAA") );
				
				Mf(gInternalDriveFile);
				Mf(gInternalDriveFile2);
				internalFlag=1;	
 				
            	break;
            	}
            
            }
    
        }
	
	
	}
	



LOCAL_C void DeleteRemovableDirectory()
	{	

	//Delete the directory structure we created in the removalbe drive	
	if ( removableFlag == 1 )
		{
		CFileMan* fMan=CFileMan::NewL(TheFs);
		test(fMan!=NULL);
	
		TFileName gPathRem;
		gPathRem.Append (removableDriveLetter);
		gPathRem.Append (_L(":\\F32-TST\\") );
		TInt r=fMan->RmDir(gPathRem);
		test_KErrNone(r);
	
		delete fMan;
		}
	}

LOCAL_C void DeleteInternalDirectory()
	{	

	//Delete the directory structure we created in the internal drive		


	if( internalFlag == 1 )
		{
		CFileMan* fMan=CFileMan::NewL(TheFs);
		test(fMan!=NULL);
	
		TFileName gPathInt;
		gPathInt.Append (internalDriveLetter);
		gPathInt.Append (_L(":\\F32-TST\\") );
		TInt r=fMan->RmDir(gPathInt);
		test_KErrNone(r);
	
		delete fMan;
		}
	}

	
LOCAL_C void MountRemoteFilesystem()	
	{
		
  	test.Next(_L("Mount Remote Drive simulator on Q:\n"));
	
	
	TInt r=TheFs.AddFileSystem(_L("CFAFSDLY"));
	test.Printf(_L("Add remote file system\n"));
	test.Printf(_L("AddFileSystem returned %d\n"),r);
	test_Value(r, r == KErrNone || r==KErrAlreadyExists);


	r=TheFs.MountFileSystem(_L("DELAYFS"),EDriveQ);

	
	test.Printf(_L("Mount remote file system\n"));
	test.Printf(_L("MountFileSystem returned %d\n"),r);
	test_Value(r, r == KErrNone || r==KErrCorrupt || r==KErrNotReady || r==KErrAlreadyExists);

	
	Mf(_L("Q:\\F32-TST\\LOCTEST\\BIN\\FINDFILE.AAA"));

	}
	

	
LOCAL_C void DisMountRemoteFilesystem()	
	{

	test.Printf(_L("Dismounting the remote Drives \n"));
 	
 	TInt r=TheFs.DismountFileSystem(_L("DELAYFS"),EDriveQ);
 	 
 	test.Printf(_L("Dismounting the Remote Drive returned %d\n"),r);
 	
 	test_KErrNone(r);
	}


	

LOCAL_C void Test1()
//
// Do simple tests
//
	{
	test.Next(_L("Test FindByPath"));
	
	TAutoClose<RFs> fs;
	TInt r=fs.iObj.Connect();
	test_KErrNone(r);
	TFindFile finder(fs.iObj);
	TPtrC path=gPath1;
	r=finder.FindByPath(_L("file1.aaa"),&path);
	test_KErrNone(r);
	TParse fileParse;
	fileParse.Set(finder.File(),NULL,NULL);
	test(fileParse.Path()==_L("\\F32-TST\\LOCTEST\\BIN1\\"));
	test(fileParse.NameAndExt()==_L("file1.aaa"));
	r=finder.Find();
	test_Value(r, r == KErrNotFound);


	path.Set(gPath2);
	r=finder.FindByPath(_L("file1.aaa"),&path);
	test_KErrNone(r);
	fileParse.Set(finder.File(),NULL,NULL);
	test(fileParse.Path()==_L("\\F32-TST\\LOCTEST\\BIN1\\"));
	test(fileParse.NameAndExt()==_L("file1.aaa"));
	r=finder.Find();
	test_KErrNone(r);
	fileParse.Set(finder.File(),NULL,NULL);
	test(fileParse.Path()==_L("\\F32-TST\\LOCTEST\\BIN1\\BIN4\\"));
	test(fileParse.NameAndExt()==_L("file1.aaa"));
	r=finder.Find();
	test_Value(r, r == KErrNotFound);
//
	test.Next(_L("Test FindByDir"));
	TPtrC dir=_L("\\F32-TST\\LOCTEST\\BIN2\\");
	r=finder.FindByDir(_L("file2.bbb"),dir);
	test_KErrNone(r);
	TFileName defaultPath;
	r=TheFs.SessionPath(defaultPath);
	defaultPath.SetLength(2);
	test_KErrNone(r);
	fileParse.Set(finder.File(),NULL,NULL);
	test(fileParse.Drive()==defaultPath);
	test(fileParse.Path()==_L("\\F32-TST\\LOCTEST\\BIN2\\"));
	test(_L("file2.bbb").MatchF(fileParse.NameAndExt())!=KErrNotFound); // MatchF only sees wildcards in its argument
	r=finder.Find();
	if (r==KErrNone)
		{
		fileParse.Set(finder.File(),NULL,NULL);
		if (defaultPath==_L("C:"))
			test(fileParse.Drive()==_L("Y:"));
		else
			test(fileParse.Drive()==_L("C:"));
		test(fileParse.Path()==_L("\\F32-TST\\LOCTEST\\BIN2\\"));
		test(_L("file2.bbb").MatchF(fileParse.NameAndExt())!=KErrNotFound);
		r=finder.Find();
		}
	test_Value(r, r == KErrNotFound);
	}

LOCAL_C void Test2()
//
// Test extremes
//
	{

	test.Next(_L("Test extremes"));
	TAutoClose<RFs> fs;
	TInt r=fs.iObj.Connect();
	test_KErrNone(r);
	TBuf<4> temp=_L("");
	TFindFile finder(fs.iObj);
	r=finder.FindByPath(_L("file1.aaa"),&temp);
	test_Value(r, r == KErrNotFound);
	r=finder.Find();
	test_Value(r, r == KErrNotFound);
//
	TPtrC path=_L("blarg.7");
	r=finder.FindByPath(_L(""),&path);	
	test_Value(r, r == KErrArgument);
	r=finder.FindByPath(_L("*"),&path);
	test_Value(r, r == KErrNotFound);
	r=finder.FindByPath(_L("xmvid"),&path);
	test_Value(r, r == KErrNotFound);
	r=finder.Find();
	test_Value(r, r == KErrNotFound);
//
	path.Set(_L("C:\\F32-TST\\LOCTEST\\BIN1\\;\\F32-TST\\LOCTEST\\BIN2\\;Z:\\F32-TST\\LOCTEST\\BIN1\\BIN4\\;\\F32-TST\\LOCTEST\\BIN3\\;"));
	r=finder.FindByPath(_L(""),&path);
	test_Value(r, r == KErrArgument);
	r=finder.FindByPath(_L("xyz.abc"),&path);
	test_Value(r, r == KErrNotFound);
	r=finder.Find();
	test_Value(r, r == KErrNotFound);
	
	test.Next(_L("Test FindByDir with empty file spec"));
	TPtrC dir2=_L("\\F32-TST\\LOCTEST\\");
	r=finder.FindByDir(_L(""),dir2);
	test_Value(r, r == KErrArgument);		
	
	}

LOCAL_C void Test3()
//
// Test FindByDrives in a path=_L("c:\xyz;z:\lmnop;\abc;\y:\help");
//
	{

	test.Next(_L("Test FindInDrivesByPath"));
	TPtrC path=_L("\\F32-TST\\LOCTEST\\BIN2\\");
	TFileName defaultPath;
	TInt r=TheFs.SessionPath(defaultPath);
	defaultPath.SetLength(2);
//
	TAutoClose<RFs> fs;
	r=fs.iObj.Connect();
	test_KErrNone(r);
	TFindFile finder(fs.iObj);
	r=finder.FindByPath(_L("file1.aaa"),&path);
	test_Value(r, r == KErrNotFound);
	r=finder.Find();
	test_Value(r, r == KErrNotFound);
//
	path.Set(_L("\\F32-TST\\LOCTEST\\BIN2\\"));
	r=finder.FindByPath(_L("file2.bbb"),&path);
	test_KErrNone(r);
	TParse fileParse;
	fileParse.Set(finder.File(),NULL,NULL);
	test(fileParse.Drive()==defaultPath);
	test(fileParse.Path()==_L("\\F32-TST\\LOCTEST\\BIN2\\"));
	test(fileParse.NameAndExt()==_L("file2.bbb"));
	r=finder.Find();
	test_Value(r, r == KErrNotFound || r==KErrNone);
	if (r==KErrNone)
		{
		fileParse.Set(finder.File(),NULL,NULL);
		test(fileParse.Drive()!=defaultPath);
		test(fileParse.Path()==_L("\\F32-TST\\LOCTEST\\BIN2\\"));
		test(fileParse.NameAndExt()==_L("file2.bbb"));
		r=finder.Find();
		test_Value(r, r == KErrNotFound);
		}
//
	path.Set(_L("C:\\F32-TST\\LOCTEST\\BIN1\\;;\\F32-TST\\LOCTEST\\BIN2\\;Z:\\F32-TST\\LOCTEST\\BIN1\\BIN4\\;\\F32-TST\\LOCTEST\\BIN3\\;"));
	r=finder.FindByPath(_L("xyz.abc"),&path);
	test_Value(r, r == KErrNotFound);
	r=finder.Find();
	test_Value(r, r == KErrNotFound);
//
	r=finder.FindByPath(_L("file2.bbb"),&path);
	test_KErrNone(r);
	fileParse.Set(finder.File(),NULL,NULL);
	test(fileParse.Drive()==defaultPath);
	test(fileParse.Path()==_L("\\F32-TST\\LOCTEST\\BIN2\\"));
	test(fileParse.NameAndExt()==_L("file2.bbb"));
	r=finder.Find();
	test_Value(r, r == KErrNotFound || r==KErrNone);
	if (r==KErrNone)
		{
		fileParse.Set(finder.File(),NULL,NULL);
		test(fileParse.Drive()!=defaultPath);
		test(fileParse.Path()==_L("\\F32-TST\\LOCTEST\\BIN2\\"));
		test(fileParse.NameAndExt()==_L("file2.bbb"));
		r=finder.Find();
		test_Value(r, r == KErrNotFound);
		}
	}

LOCAL_C void Test4()
//
// Test wildcard findbypath
//
	{

	test.Next(_L("FindByPath with wild filenames"));
	TFindFile finder(TheFs);
	CDir* dir;
	TInt count;
	TEntry entry;
	TFileName path;

	TInt r=finder.FindWildByPath(_L("*.aaa"),&gPath3,dir);
	test_KErrNone(r);
	count=dir->Count();
	test(count==3);
	entry=(*dir)[0];
	test(entry.iName.MatchF(_L("FILE.AAA"))!=KErrNotFound);
	entry=(*dir)[1];
	test(entry.iName.MatchF(_L("FILE1.AAA"))!=KErrNotFound);
	entry=(*dir)[2];
	test(entry.iName.MatchF(_L("WORK.AAA"))!=KErrNotFound);
	TParse fileParse;
	fileParse.Set(finder.File(),NULL,NULL);
	path=fileParse.FullName();
	test(path==_L("*.aaa"));
	delete dir;

	r=finder.FindWild(dir);
	test_KErrNone(r);
	count=dir->Count();
	test(count==2);
	entry=(*dir)[0];
	test(entry.iName.MatchF(_L("FILE1.AAA"))!=KErrNotFound);
	entry=(*dir)[1];
	test(entry.iName.MatchF(_L("WORK.AAA"))!=KErrNotFound);
	fileParse.Set(finder.File(),NULL,NULL);
	path=fileParse.FullName();
	test(path==_L("C:\\F32-TST\\LOCTEST\\BIN1\\*.aaa"));
	delete dir;

	r=finder.FindWild(dir);
	test_Value(r, r == KErrNotFound);
	r=finder.FindWild(dir);
	test_Value(r, r == KErrNotFound);

	r=finder.FindWildByPath(_L("*FILE.AAA*"), &gPath1, dir);
	test_KErrNone(r);
	test(dir->Count()==1);
	entry=(*dir)[0];
	test(entry.iName.MatchF(_L("FILE.AAA"))!=KErrNotFound);
	delete dir;
	
	r=finder.FindWildByPath(_L("*FILE.AAA"), &gPath1, dir);
	test_KErrNone(r);
	test(dir->Count()==1);
	entry=(*dir)[0];
	test(entry.iName.MatchF(_L("FILE.AAA"))!=KErrNotFound);
	delete dir;
	
	r=finder.FindWildByPath(_L("FILE.AAA*"), &gPath1, dir);
	test_KErrNone(r);
	test(dir->Count()==1);
	entry=(*dir)[0];
	test(entry.iName.MatchF(_L("FILE.AAA"))!=KErrNotFound);
	delete dir;

    
	r=finder.FindWildByPath(_L("CONFUSED.DOG"), &gPath1, dir);
	test_KErrNone(r);
	test(dir->Count()==1);
	entry=(*dir)[0];
	test(entry.iName.MatchF(_L("CONFUSED.DOG"))!=KErrNotFound);
	delete dir;

	r=finder.FindWildByPath(_L("*CONFUSED.DOG"), &gPath1, dir);
	test_KErrNone(r);
	test(dir->Count()==1);
	entry=(*dir)[0];
	test(entry.iName.MatchF(_L("CONFUSED.DOG"))!=KErrNotFound);
	delete dir;
	r=finder.FindWildByPath(_L("CONFUSED.DOG*"), &gPath1, dir);
	test_KErrNone(r);
	test(dir->Count()==1);
	entry=(*dir)[0];
	test(entry.iName.MatchF(_L("CONFUSED.DOG"))!=KErrNotFound);
	delete dir;
	r=finder.FindWildByPath(_L("*CONFUSED.DOG*"), &gPath1, dir);
	test_KErrNone(r);
	test(dir->Count()==1);
	entry=(*dir)[0];
	test(entry.iName.MatchF(_L("CONFUSED.DOG"))!=KErrNotFound);
	delete dir;
	}

LOCAL_C void Test5()
//
// Test wildcard findbydir
//
	{

	test.Next(_L("FindByDir with wild filenames"));
	TFindFile finder(TheFs);
	CDir* dir;
	TInt count;
	TEntry entry;
	TFileName path;

	TInt r=finder.FindWildByDir(_L("FILE*"),_L("\\F32-TST\\LOCTEST\\BIN3\\"),dir);
	test_KErrNone(r);
	count=dir->Count();
	test(count==1);
	entry=(*dir)[0];
	test(entry.iName.MatchF(_L("FILE3.CCC"))!=KErrNotFound);
	TParse fileParse;
	fileParse.Set(finder.File(),NULL,NULL);
	path=fileParse.FullName();
	TFileName tpath=_L("?:\\F32-TST\\LOCTEST\\BIN3\\FILE*");
    tpath[0]=gSessionPath[0];
	test(path.CompareF(tpath)==0);
	delete dir;

	r=finder.FindWild(dir);
	if (r==KErrNotFound)
		return;
	test_KErrNone(r);
	entry=(*dir)[0];
	test(entry.iName.MatchF(_L("FILE3.CCC"))!=KErrNotFound);
	fileParse.Set(finder.File(),NULL,NULL);
	path=fileParse.FullName();
	test(path.CompareF(tpath)==0);
	delete dir;

	r=finder.FindWild(dir);
	test_Value(r, r == KErrNotFound);
	r=finder.FindWild(dir);
	test_Value(r, r == KErrNotFound);
	}

LOCAL_C void Test6()
//
// Test file not found
//
	{

	test.Next(_L("Test file not found"));
	TFindFile ff(TheFs);
	TInt r=ff.FindByDir(_L("NOEXIST.EXE"),_L("\\System\\Programs\\"));
	test_Value(r, r == KErrNotFound);
	}




//  The following test has the requirement that the only remote drive is the one we mount 
//  during the test(DELAYFS) and which doesn't have any other attributes set. If this is not the
//  case then test conditions must be changed, in order for the test to stop failing.
// 	Even more if a removable drive is not present in the target platform then findfile.aaa
//	only exists in the remote one and this is why we have a distinction in the test results.
//



  	//--------------------------------------------- 
	//! @SYMTestCaseID			PBASE-T_LOCATE-0553
	//! @SYMTestType			UT 
	//! @SYMREQ					CR909
	//! @SYMTestCaseDesc		When using the various Find functions of class TFindFile,by default remote drives are 
	//!							excluded from the list of drives that are searched. Using function 
	//!							SetFindMask(TUint aMask) it is possible to specify a combination of attributes that
	//!						    the drives to be searched must match.
	//! @SYMTestActions			Call function FindByPath/Find without specifying a mask. Check that remote drives are not 
	//!							included. Then call SetFindMask(TUint aMask) using various combinations and verify 
	//!							that FindByPath or Find return appopriate results.
	//! @SYMTestExpectedResults	Test that file findfile.aaa is found or not depending on the specified mask.
	//! @SYMTestPriority		High
	//! @SYMTestStatus			Implemented 
	//--------------------------------------------- 



LOCAL_C void Test7()

	{
	
	TAutoClose<RFs> fs;
	TInt r=fs.iObj.Connect();
	test_KErrNone(r);
	TFindFile finder(fs.iObj);
	TPtrC path=gPath4;
	r=finder.FindByPath(_L("findfile.aaa"),&path); 	
	
	TParse fileParse;
	
	test.Next(_L("Test FindByPath without specifying any mask"));
	
	if (removableFlag == 1)
		{
			test_KErrNone(r); 
			fileParse.Set(finder.File(),NULL,NULL);
			test(fileParse.Path()==_L("\\F32-TST\\LOCTEST\\BIN\\"));
			test(fileParse.NameAndExt()==_L("findfile.aaa")); //The filename.aaa in the removable Drive
			r=finder.Find();
			test_Value(r, r == KErrNotFound);     //remote drives are excluded by default
		
		}
	else
		test_Value(r, r == KErrNotFound);

	

	test.Next(_L("Search for the specified file in all Drives, including remotes ones \n"));


	r=finder.SetFindMask(	KDriveAttAll) ;
	test_KErrNone(r);
	r=finder.FindByPath(_L("findfile.aaa"),&path);
	test_KErrNone(r);
	fileParse.Set(finder.File(),NULL,NULL);
	test(fileParse.Path()==_L("\\F32-TST\\LOCTEST\\BIN\\"));   
	test(fileParse.NameAndExt()==_L("findfile.aaa"));      //either the remote or removable one.
	r=finder.Find();
	
	
	if (removableFlag == 1)
		{	
		test_KErrNone(r);
	
		fileParse.Set(finder.File(),NULL,NULL);

		test(fileParse.Path()==_L("\\F32-TST\\LOCTEST\\BIN\\"));
		test(fileParse.NameAndExt()==_L("findfile.aaa"));         //either the remote or removable one.

		r=finder.Find();
		test_Value(r, r == KErrNotFound);
		}
	else 
		{
		test_Value(r, r == KErrNotFound);	
			
		}
		
		
	test.Next(_L("Search exclusively in remote drives \n"));

	r=finder.SetFindMask(	KDriveAttExclusive| KDriveAttRemote); 
	test_KErrNone(r);
	r=finder.FindByPath(_L("findfile.aaa"),&path);
	test_KErrNone(r);
	fileParse.Set(finder.File(),NULL,NULL);
	test(fileParse.Path()==_L("\\F32-TST\\LOCTEST\\BIN\\"));
	test(fileParse.NameAndExt()==_L("findfile.aaa"));
	r=finder.Find();
	test_Value(r, r == KErrNotFound);

	
	test.Next(_L("Search excluding removables and remote \n"));

	r=finder.SetFindMask(	KDriveAttExclude | KDriveAttRemovable |KDriveAttRemote ); 
	test_KErrNone(r);
	r=finder.FindByPath(_L("findfile.aaa"),&path);   
	test_Value(r, r == KErrNotFound);   //filename.aaa exists in the remote drive and if present to the removable one


	test.Next(_L("Search in Internal Drives \n"));

	r=finder.SetFindMask(KDriveAttInternal ) ;
	test_KErrNone(r);
	r=finder.FindByPath(_L("findfile.aaa"),&path);   
	test_Value(r, r == KErrNotFound);   //filename.aaa exists only in the Removable drive and the remote one.


	}

	
	 //--------------------------------------------- 
	//! @SYMTestCaseID			PBASE-T_LOCATE-0554
	//! @SYMTestType			UT 
	//! @SYMREQ					CR909
	//! @SYMTestCaseDesc		Test that SetFindMask(TUint aMask) returns the correct value for all combinations of matching masks.						
	//!							
	//! @SYMTestActions			Call SetFindMask for every combination of mask and check that the correct value is returned.   
	//!							A structure is used to store the expected value for each combination.
	//! @SYMTestExpectedResults	For every combination either KErrNone or KErrArgument must be returned.
	//! @SYMTestPriority		High.
	//! @SYMTestStatus			Implemented 
	//--------------------------------------------- 
	



LOCAL_C void Test8()

	{

	test.Next(_L("Test SetFindMask with all mask combinations \n"));	
	
	
	TAutoClose<RFs> fs;
	TInt r=fs.iObj.Connect();
	test_KErrNone(r);
	TFindFile finder(fs.iObj);
	TPtrC path=gPath4;	
	TParse fileParse;
		

	r=finder.SetFindMask(KDriveAttAll) ;
	test_KErrNone(r);
	r=finder.FindByPath(_L("findfile.aaa"),&path);
	test_KErrNone(r);
	fileParse.Set(finder.File(),NULL,NULL);
	test(fileParse.Path()==_L("\\F32-TST\\LOCTEST\\BIN\\"));   
	test(fileParse.NameAndExt()==_L("findfile.aaa"));


	struct TCombinations
		{
		TUint iMatchMask;			  // The Match Mask to be combined with drive attributes
		TInt  iExpectedResultNoAtts;	  // Expected result when flag used on it's own
		TInt  iExpectedResultWithAtts;  // Expected result when flag used in combination with drive flags
		};

	TCombinations testCombinations[] = {
		{ 0,														KErrNone,     KErrNone},
		{ KDriveAttAll,												KErrNone,     KErrArgument },
		{ KDriveAttExclude,											KErrArgument, KErrNone },
		{ KDriveAttExclusive,										KErrArgument, KErrNone },
		{ KDriveAttExclude | KDriveAttExclusive,					KErrArgument, KErrNone },
		{ KDriveAttAll	   | KDriveAttExclude,						KErrArgument, KErrArgument },
		{ KDriveAttAll     | KDriveAttExclusive,					KErrArgument, KErrArgument},
		{ KDriveAttAll     | KDriveAttExclude | KDriveAttExclusive, KErrArgument, KErrArgument}};


	
	for(TUint matchIdx = 0; matchIdx < sizeof(testCombinations) / sizeof(TCombinations); matchIdx++)
		{
		test.Printf(_L("\nTest mask : KDriveAttAll[%c] KDriveAttExclude[%c] KDriveAttExclusive[%c]\n"), testCombinations[matchIdx].iMatchMask & KDriveAttAll       ? 'X' : ' ',
																										 testCombinations[matchIdx].iMatchMask & KDriveAttExclude   ? 'X' : ' ',
																										 testCombinations[matchIdx].iMatchMask & KDriveAttExclusive ? 'X' : ' ');
		for(TUint testAtt = 0; testAtt <= KMaxTUint8; testAtt++)
			{
			r= finder.SetFindMask( testCombinations[matchIdx].iMatchMask | testAtt ) ;
	 		
		//	test.Printf(_L("            ATT : 0x%08x \n"), testAtt);
		//	test.Printf(_L("Expected Result : %d     \n"), testAtt == 0 ? testCombinations[matchIdx].iExpectedResultNoAtts : testCombinations[matchIdx].iExpectedResultWithAtts);
		//	test.Printf(_L("  Actual Result : 0x%08x \n"), err);
		
		//	test.Printf(_L("\nTest mask : %d \n"),testCombinations[matchIdx].iMatchMask | testAtt );
			
			test_Value(r, r == (testAtt == 0 ? testCombinations[matchIdx].iExpectedResultNoAtts : testCombinations[matchIdx].iExpectedResultWithAtts) );
			
			
			if (r== KErrNone)
				{
				r  = finder.FindByPath(_L("findfile.aaa"),&path);
				test_Value(r, r == KErrNone || r ==KErrNotFound);
				}
			
			}
		}

	}
	
	



LOCAL_C void Test9()
//
// Test wildcard findbydir and FindByWildPath in Removable and Internal Drives
//

	//--------------------------------------------- 
	//! @SYMTestCaseID			PBASE-T_LOCATE-0555
	//! @SYMTestType			UT 
	//! @SYMREQ					CR909
	//! @SYMTestCaseDesc		Check that FindWildByDir and FindByDir functions behave correctly when a mask has been specified 
	//! 						through SetFindMask.
	//! @SYMTestActions			Call FindWildByDir with a filename containing wildchars and a specific path. Then call SetFindMask
	//!							to exclude Removable drives and call FindWildByDir again.Even more call FindByDir for the file in
	//!							the removable drive and for the same directory as before. 
	//! @SYMTestExpectedResults The number of files found when excluding the removable drive(if a removable drive exists in the
	//!							target platform) must differ by one. The FinByDir must find the same results.
	//! @SYMTestPriority		High
	//! @SYMTestStatus			Implemented 
	//--------------------------------------------- 


	{		

	TAutoClose<RFs> fs;
	TInt r=fs.iObj.Connect();
	test_KErrNone(r);
	
	TFindFile finder(fs.iObj);
	
	CDir* dir;
	CDir* dir3;
	
	TInt count;
	TEntry entry;	



	if ( removableFlag == 1 )
		{
		
		test.Next(_L("FindByDir with wild filenames when a find mask is specified"));
		
		TInt r=finder.SetFindMask(KDriveAttRemovable);
		test_KErrNone(r);
		r=finder.FindWildByDir(_L("FIND*"),_L("\\F32-TST\\LOCTEST\\BIN\\"),dir);
		test_KErrNone(r);
		count=dir->Count();
		test(count==1);
		entry=(*dir)[0];
		test(entry.iName.MatchF(_L("FINDFILE.AAA"))!=KErrNotFound); 
		delete dir;	

		r=finder.FindWild(dir);
		test_Value(r, r == KErrNotFound);	
		
		
		r=finder.SetFindMask(KDriveAttExclude| KDriveAttRemovable);
		test_KErrNone(r);
		r=finder.FindWildByDir(_L("FIND*"),_L("\\F32-TST\\LOCTEST\\BIN\\"),dir);
		test_Value(r, r == KErrNotFound);
		
		
		test.Next(_L("Test FindByDir when a find mask is specified"));
		
		
		TPtrC dir2=_L("\\F32-TST\\LOCTEST\\BIN\\");
		
		r=finder.SetFindMask(KDriveAttExclude | KDriveAttRemote );
		test_KErrNone(r);
		r=finder.FindByDir(_L("findfile.aaa"),dir2);
		test_KErrNone(r);
	
		r=finder.Find();
		test_Value(r, r == KErrNotFound);	
		

		}
	


	//--------------------------------------------- 
	//! @SYMTestCaseID			PBASE-T_LOCATE-0556
	//! @SYMTestType			UT 
	//! @SYMREQ					CR909
	//! @SYMTestCaseDesc		FindByWildPath and FindByPath functions when supplied with a path that also contains 
	//!							a Drive letter, they will not need to check other Drives. Therefore calling SetFindMask 
	//!							does not affect the drives returned. 
	//! @SYMTestActions			Call FindWildByPath with an appropriate path in the internal drive. Then call SetFindMask
	//!							to exclude Internal drives and call FindWildByPath again.
	//! @SYMTestExpectedResults The number of files found in both cases must be the same since no other drive is searched.
	//! @SYMTestPriority		High
	//! @SYMTestStatus			Implemented 
	//--------------------------------------------- 




	if( internalFlag == 1 )
		{
		
		
		test.Next(_L("Test that SetFindMask does not affect Find functions that have a drive letter specified"));
		

		gPath5.Append (internalDriveLetter);
		gPath5.Append (_L(":\\F32-TST\\LOCTEST\\BIN\\INT\\") );

		
		r=finder.FindWildByPath(_L("FIND*.AAA"), &gPath5, dir3);
 		test_KErrNone(r);
		test(dir3->Count()==2);
		
		entry=(*dir3)[0];		
		test(  (entry.iName.MatchF(_L("FINDINTERNALFILE_B.AAA"))!=KErrNotFound)  || (entry.iName.MatchF(_L("FINDINTERNALFILE.AAA"))!=KErrNotFound)  );
		
				
		entry=(*dir3)[1];
		test(  (entry.iName.MatchF(_L("FINDINTERNALFILE_B.AAA"))!=KErrNotFound )  || (entry.iName.MatchF(_L("FINDINTERNALFILE.AAA"))!=KErrNotFound)  );		

		
		delete dir3;
		
		
		
		r=finder.SetFindMask(KDriveAttExclude| KDriveAttInternal);
		test_KErrNone(r);
		r=finder.FindWildByPath(_L("FIND*.AAA"), &gPath5, dir3);	
 		test_KErrNone(r);
		test(dir3->Count()==2);
		
		delete dir3;
		
		
		r=finder.FindWild(dir3);
		test_Value(r, r == KErrNotFound);
				
				
		}


	}

//---------------------------------------------------------------------------------------
/**
    Test that callinng TFindFile methods that allocate CDir objects doesn't lead to memory leaks if some error occurs.
*/
void TestFailures()
    {

	test.Next(_L("Test TFindFile failures\n"));	

#ifndef _DEBUG
    test.Printf(_L("This test can't be performed in UREL mode, skipping\n"));
    return;	
#else

	TFindFile finder(TheFs);
	CDir* pDir;
	TInt nRes;
    TInt cnt=0;

    _LIT(KPath, "\\F32-TST\\LOCTEST\\");

    const TInt KMyError = -756; //-- specific error code we will simulate
    
    //------------------------------------
    test.Printf(_L("Test FindWildByPath failures\n"));	
    
    __UHEAP_MARK;
    nRes = finder.FindWildByPath(_L("*"), &gPath1, pDir);
    test_KErrNone(nRes);
    test(pDir && pDir->Count() > 1);
    delete pDir;

 
    for(cnt = 0; ;cnt++)
        {
        nRes =TheFs.SetErrorCondition(KMyError, cnt);
        test_KErrNone(nRes);

        pDir = (CDir*)0xaabbccdd;
        nRes = finder.FindWildByPath(_L("*"), &gPath1, pDir);
        
        //-- on error the memory allocated internally for CDir shall be freed and the pointer CDir* shall be set to NULL 
        if(nRes == KErrNone)
            {
            test.Printf(_L("Test FindWildByPath->FindWild() failures\n"));	
            test(pDir && pDir->Count() > 1);
            delete pDir;
            pDir = (CDir*)0xaabbccdd;

            TheFs.SetErrorCondition(KMyError);
            nRes = finder.FindWild(pDir);
            test(nRes != KErrNone);
            test(pDir == NULL); 
            
            break;
            }
        else
            {
            test(pDir == NULL);
            }

        }

   __UHEAP_MARKEND;
   TheFs.SetErrorCondition(KErrNone);


   //------------------------------------
   test.Printf(_L("Test FindWildByDir failures\n"));	
    
   __UHEAP_MARK;
   nRes = finder.FindWildByDir(_L("*"), KPath, pDir);
   test_KErrNone(nRes);
   test(pDir && pDir->Count() > 1);
   delete pDir;
   
   for(cnt = 0; ;cnt++)
        {
        nRes =TheFs.SetErrorCondition(KMyError, cnt);
        test_KErrNone(nRes);

        pDir = (CDir*)0xaabbccdd;
        nRes = finder.FindWildByDir(_L("*"), KPath, pDir);
        
        //-- on error the memory allocated internally for CDir shall be freed and the pointer CDir* shall be set to NULL  
        if(nRes == KErrNone)
            {
            test.Printf(_L("Test FindWildByDir->FindWild() failures\n"));	
            test(pDir && pDir->Count() > 1);
            delete pDir;
            pDir = (CDir*)0xaabbccdd;

            TheFs.SetErrorCondition(KMyError);
            nRes = finder.FindWild(pDir);
            test(nRes != KErrNone);
            test(pDir == NULL);
            
            break;
            }
        else
            {
            test(pDir == NULL);
            }

        }

   __UHEAP_MARKEND;
   TheFs.SetErrorCondition(KErrNone);
#endif
}

//---------------------------------------------------------------------------------------
void CallTestsL()
//
// Do all tests
//
	{

		
		gPath3=_L("C:\\F32-TST\\LOCTEST\\BIN1\\;C:\\F32-TST\\LOCTEST\\BIN2\\");

		gPath1=_L("");
		gPath1.Append(gSessionPath[0]);
		gPath1.Append(_L(":\\F32-TST\\LOCTEST\\BIN1\\;"));
		gPath1.Append(gSessionPath[0]);
		gPath1.Append(_L(":\\F32-TST\\LOCTEST\\BIN2\\"));
	
		gPath2=gPath1;
		gPath2.Append(';');
		gPath2.Append(gSessionPath[0]);
		gPath2.Append(_L(":\\F32-TST\\LOCTEST\\BIN1\\BIN4\\;"));
		if (gSessionPath[0]!='C')
			gPath2.Append(gSessionPath.Left(2));
		gPath2.Append(_L("\\F32-TST\\LOCTEST\\BIN3\\;"));
		
		gPath4=_L("");
		gPath4.Append(_L("\\F32-TST\\LOCTEST\\BIN\\"));
		
	

		CreateTestDirectory(_L("\\F32-TST\\LOCTEST\\"));
		MakeLocateTestDirectoryStructure();
		
        TestFailures();
		Test1();
		Test2();
		Test3();
		Test4();
		Test5();
		Test6();
	
		MountRemoteFilesystem();      
		CreateFilesInRemovableDrive();  //used in Test7/8/9   	
	
		Test7();
		Test8();
	
		CreateFilesInInternalDrive();  //used in Test9
		Test9();
			
		DisMountRemoteFilesystem();	
		
		DeleteTestDirectory();
		
		//Explicity delete the directories created
		DeleteRemovableDirectory();
		DeleteInternalDirectory();


	
	}
