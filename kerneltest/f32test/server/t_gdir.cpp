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
#include "t_server.h"

GLDEF_D RTest test(_L("T_GDIR"));

LOCAL_D const TInt KFilesMax=9;
LOCAL_D const TInt KUidFilesMax=7;
LOCAL_D const TInt KDirsMax=4;
LOCAL_D TBool gFirstRun=ETrue;
LOCAL_D TPtrC test_dir(_L("\\F32-TST\\GDIR\\"));
LOCAL_D TPtrC test_dir_1(_L("\\F32-TST\\GDIR\\*"));

class TUidFile
	{
public:
	TUidFile(const TText* aFileName,TUidType aUidType,const TText8* aContents);
public:
	const TText* iFileName;
	TUidType iUidType;
	const TText8* iContents;
	};

TUidFile::TUidFile(const TText* aFileName,TUidType aUidType,const TText8* aContents)
	: iFileName(aFileName), iUidType(aUidType), iContents(aContents)
	{}

LOCAL_D TUidFile uidFiles[] = 
	{
	TUidFile(_S("File1.TXT"), TUidType(TUid::Uid(1),TUid::Uid(2),TUid::Uid(731)),_S8("blarg blarg blarg")),
	TUidFile(_S("asdf.asdf"), TUidType(TUid::Uid(55),TUid::Uid(2),TUid::Uid(731)),_S8("blarg")),
	TUidFile(_S("another fiel"), TUidType(TUid::Uid(104),TUid::Uid(22),TUid::Uid(731)),_S8("blarg2")),
	TUidFile(_S("another fiel1"), TUidType(TUid::Uid(7),TUid::Uid(23),TUid::Uid(131)),_S8("")),
	TUidFile(_S("another fiel2"), TUidType(TUid::Uid(8),TUid::Uid(2),TUid::Uid(531)),_S8("asdf")),
	TUidFile(_S("another fiel3"), TUidType(TUid::Uid(9),TUid::Uid(22),TUid::Uid(531)),_S8("blar")),
	TUidFile(_S("another fiel4"), TUidType(TUid::Uid(10),TUid::Uid(23),TUid::Uid(231)),_S8("blarg blarg blarg asdlfjasdfasdfasdfasdfasdfadfafa"))
	};

LOCAL_D const TText* fileNames[] =
	{
	_S("B1.B3"),_S("B2.B2"),_S("B3.B1"),
	_S("A1.A3"),_S("A2.A2"),_S("A3.A1"),
	_S("Z1.Z3"),_S("Z2.Z2"),_S("Z3.Z1")
	};

LOCAL_D const TText* dirNames[] =
	{
	_S("DB1"),
	_S("DA1"),
	_S("DZ1"),
	_S("DD1")
	};

inline TName files(TInt anIndex)
	{return(TName(fileNames[anIndex]));}
inline TName dirs(TInt anIndex)
	{return(TName(dirNames[anIndex]));}

LOCAL_C void displayDir(const CDir& aDir,TInt& aDirCount,TInt& aFileCount)
//
// Display the contents of a directory list.
//
	{

	TInt count=aDir.Count();
	TInt i=0;
	TInt fCount=0;
	TInt dCount=0;
	while (i<count)
		{
		const TEntry& e=aDir[i++];
		if (e.IsDir())
			{
			dCount++;
			test.Printf(_L("%- 16S <DIR>\n"),&e.iName);
			}
		else
			{
			fCount++;
			test.Printf(_L("%- 16S %+ 8d\n"),&e.iName,e.iSize);
			}
		}
	test.Printf(_L("Dirs = %d Files = %d\n"),dCount,fCount);
	aFileCount=fCount;
	aDirCount=dCount;
	}

LOCAL_C void createFile(const TUidFile& aFileName)
//
// Create a file in the test directory.
//
	{

	TCheckedUid checkedUid(aFileName.iUidType);
	TPtrC fileName(aFileName.iFileName);
	TAutoClose<RFile> file;
	TInt r=file.iObj.Replace(TheFs,fileName,EFileWrite);
	test_KErrNone(r);
	TPtrC8 uidBuf((TUint8*)&checkedUid,sizeof(TCheckedUid));
	r=file.iObj.Write(uidBuf);
	test_KErrNone(r);
	TPtrC8 contents(aFileName.iContents);
	r=file.iObj.Write(contents);
	test_KErrNone(r);
	}

LOCAL_C void createFile(TInt anIndex)
//
// Create a file in the test directory.
//
	{

	TFileName fName;
	TName name=files(anIndex);
	fName.Format(_L("%S%S"),&test_dir,&name);
	TBuf<0x80> mes;
	mes.Format(_L("Create file %S"),&fName);
    test.Next(mes);
//
	TAutoClose<RFile> file;
	TInt r=file.iObj.Replace(TheFs,fName,EFileWrite);
	test_KErrNone(r);
	TBuf8<36> b((TUint8*)"0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ");
	b.SetLength(anIndex+1);
	r=file.iObj.Write(b);
	test_KErrNone(r);
	}

LOCAL_C void createDir(TInt anIndex)
//
// Create a dir in the test directory.
//
	{

	TFileName dName;
	TName name=dirs(anIndex);
	dName.Format(_L("%S%S\\"),&test_dir,&name);
	TBuf<0x80> mes;
	mes.Format(_L("Create dir %S"),&dName);
    test.Next(mes);
//
	TInt r=TheFs.MkDir(dName);
	test_KErrNone(r);
	}

LOCAL_C void testSetup()
//
// Setup the test environment.
//
	{

	test.Next(_L("Remove test directory"));
	CDir* pD;
	TInt r=TheFs.GetDir(test_dir_1,KEntryAttMaskSupported,EDirsLast,pD);
	test_Value(r, r == KErrNone || r==KErrNotFound || r==KErrPathNotFound);
	if (r==KErrNone)
		{
		TInt count=pD->Count();
		TInt i=0;
		while (i<count)
			{
			const TEntry& e=(*pD)[i++];
			if (e.IsDir())
				{
				TFileName name;
				name.Format(_L("%S%S\\"),&test_dir,&e.iName);
				r=TheFs.RmDir(name);
				test_KErrNone(r);
				}
			else
				{
				TFileName name;
				name.Format(_L("%S%S"),&test_dir,&e.iName);
				r=TheFs.Delete(name);
				test_KErrNone(r);
				}
			}
		}
//
	delete pD;
//
	test.Next(_L("Create test files"));
	TInt i=0;
	while (i<KFilesMax)
		createFile(i++);
//
	test.Next(_L("Create test directories"));
	i=0;
	while (i<KDirsMax)
		createDir(i++);
	}

LOCAL_C void testDir()
//
// Setup the test environment.
//
	{

	TInt dCount;
	TInt fCount;
	test.Next(_L("Test directory handling"));
	CDir* pD;
	TInt r=TheFs.GetDir(test_dir_1,KEntryAttMaskSupported,EDirsLast,pD);
	test_KErrNone(r);
	displayDir(*pD,dCount,fCount);
	test(dCount==4 && fCount==9);
	delete pD;
//
	test.Next(_L("Attributes: NULL"));
	r=TheFs.GetDir(test_dir_1,NULL,EDirsLast,pD);
	test_KErrNone(r);
	displayDir(*pD,dCount,fCount);
	test(dCount==0 && fCount==9);
	delete pD;
//
	test.Next(_L("Attributes: KEntryAttDir & EDescending sort"));
	r=TheFs.GetDir(test_dir_1,KEntryAttDir,ESortByName|EDescending,pD);
	test_KErrNone(r);
	displayDir(*pD,dCount,fCount);
	test(dCount==4 && fCount==9);
	delete pD;
//	
	test.Next(_L("Attributes: Excl,Dir"));
	r=TheFs.GetDir(test_dir_1,KEntryAttMatchExclusive|KEntryAttDir,ESortByName|EDescending,pD);
	test_KErrNone(r);
	displayDir(*pD,dCount,fCount);
	test(dCount==4 && fCount==0);
	delete pD;
//	
	CDir* pD2;
//
	test.Next(_L("Test split directories and files"));
	r=TheFs.GetDir(test_dir_1,KEntryAttMaskSupported,ESortByName,pD,pD2);
	test_KErrNone(r);
	test.Printf(_L("FileList:\n"));
	displayDir(*pD,dCount,fCount);
	test(dCount==4 && fCount==9);
	test.Printf(_L("DirList:\n"));
	displayDir(*pD2,dCount,fCount);
	test(dCount==4 && fCount==0);
	delete pD;
	delete pD2;
//
	test.Next(_L("Attributes: NULL"));
	r=TheFs.GetDir(test_dir_1,NULL,ESortByName,pD,pD2);
	test_KErrNone(r);
	test.Printf(_L("FileList:\n"));
	displayDir(*pD,dCount,fCount);
	test(dCount==0 && fCount==9);
	test.Printf(_L("DirList:\n"));
	displayDir(*pD2,dCount,fCount);
	test(dCount==4 && fCount==0);
	delete pD;
	delete pD2;
//
	test.Next(_L("Attributes: KEntryAttDir"));
	r=TheFs.GetDir(test_dir_1,KEntryAttDir,ESortByName,pD,pD2);
	test_KErrNone(r);
	test.Printf(_L("FileList:\n"));
	displayDir(*pD,dCount,fCount);
	test(dCount==4 && fCount==9);
	test.Printf(_L("DirList:\n"));
	displayDir(*pD2,dCount,fCount);
	test(dCount==4 && fCount==0);
	delete pD;
	delete pD2;
//
	test.Next(_L("Attributes: Excl,Dir"));
	r=TheFs.GetDir(test_dir_1,KEntryAttMatchExclusive|KEntryAttDir,ESortByName,pD,pD2);
	test_KErrNone(r);
	test.Printf(_L("FileList:\n"));
	displayDir(*pD,dCount,fCount);
	test(dCount==4 && fCount==0);
	test.Printf(_L("DirList:\n"));
	displayDir(*pD2,dCount,fCount);
	test(dCount==4 && fCount==0);
	delete pD;
	delete pD2;
	}

LOCAL_C void testZDirectory()
//
// Display Z directory
//
	{

	test.Next(_L("Test Z:"));
	TInt dCount,fCount;
	CDir* pD;
	TInt r=TheFs.GetDir(_L("Z:\\*"),KEntryAttMaskSupported,EDirsFirst,pD);
	test_KErrNone(r);
	displayDir(*pD,dCount,fCount);
	delete pD;
	}

LOCAL_C void testDisplayFiles()
//
// Display some files
//
	{

	test.Next(_L("Display contents of current directory"));
	CDir* pD;
	TInt r=TheFs.GetDir(gSessionPath,KEntryAttMaskSupported,EDirsFirst,pD);
	test_KErrNone(r);
	TInt dCount,fCount;
	displayDir(*pD,dCount,fCount);
	delete pD;

	TParsePtrC session(gSessionPath);
	TParse parser;
	TBuf<16> noName=_L("asdf.idd");
	parser.Set(session.Drive(),&noName,NULL);
	r=TheFs.GetDir(parser.FullName(),KEntryAttMaskSupported,EDirsFirst,pD);
	test_KErrNone(r);
	test(pD->Count()==0);
	delete pD;
	}

LOCAL_C void MatchUidFile(TInt aUidFile,TInt anEntryNum,const CDir* aFileList)
//
// Check aUidFile matches anEntryNum
//
	{

	test(aUidFile<KUidFilesMax);
	TInt count=aFileList->Count();
	test(anEntryNum<count);
	TEntry entry=(*aFileList)[anEntryNum];

	TPtrC uidFileName(uidFiles[aUidFile].iFileName);
	test(entry.iName==uidFileName);
	test(entry.iType==uidFiles[aUidFile].iUidType);

	RFile f;
	TInt r=f.Open(TheFs,entry.iName,EFileRead);
	test_KErrNone(r);
	TBuf8<256> contents;
	r=f.Read(sizeof(TCheckedUid),contents);
	test_KErrNone(r);
	TPtrC8 uidFileContents(uidFiles[aUidFile].iContents);
	test(contents==uidFileContents);
	r=f.Read(contents);
	test_KErrNone(r);
	test(contents.Length()==0);
	f.Close();
	}

LOCAL_C TInt PrintUid(TInt anEntryNum,const CDir* aFileList)
//
// Check aUidFile matches anEntryNum
//
	{

	TInt count=aFileList->Count();
	test(anEntryNum<count);
	TEntry entry=(*aFileList)[anEntryNum];
	test.Printf(_L("Entry name = %S UID=%d\n"),&entry.iName,entry.iType[2]);
	return(entry.iType[2].iUid);
	}

LOCAL_C void testGetDirByUid()
//
// Get directory contents by matching UIDs
//
	{

	test.Next(_L("Get directory contents by matching UIDs"));
	TInt i=KUidFilesMax;
	while(i--)
		createFile(uidFiles[i]);

	TBuf<16> matchName=_L("*.txt");
	TUidType matchUid(TUid::Null(),TUid::Uid(2),TUid::Null());
	CDir* fileList;
	TInt r=TheFs.GetDir(matchName,matchUid,EAscending,fileList);
	test_KErrNone(r);
	TInt count=fileList->Count();
	test(count==1);
	MatchUidFile(0,0,fileList);
	delete fileList;

	matchName=_L("*.*");
	matchUid=TUidType(TUid::Uid(1),TUid::Uid(2),TUid::Uid(731));
	r=TheFs.GetDir(matchName,matchUid,EAscending,fileList);
	test_KErrNone(r);
	count=fileList->Count();
	test(count==1);
	MatchUidFile(0,0,fileList);
	delete fileList;

	matchName=_L("*.*");
	matchUid=TUidType(TUid::Null(),TUid::Uid(2),TUid::Null());
	r=TheFs.GetDir(matchName,matchUid,ESortByName|EAscending,fileList);
	test_KErrNone(r);
	count=fileList->Count();
	test(count==3);
	MatchUidFile(0,2,fileList);
	MatchUidFile(1,1,fileList);
	MatchUidFile(4,0,fileList);
	delete fileList;

	matchName=_L("*.*");
	matchUid=TUidType(TUid::Null(),TUid::Null(),TUid::Uid(731));
	r=TheFs.GetDir(matchName,matchUid,ESortByName|EAscending,fileList);
	test_KErrNone(r);
	count=fileList->Count();
	test(count==3);
	MatchUidFile(2,0,fileList);
	MatchUidFile(1,1,fileList);
	MatchUidFile(0,2,fileList);
	delete fileList;

	matchName=_L("*.*");
	r=TheFs.GetDir(matchName,KEntryAttNormal,ESortByUid|EAscending,fileList);
	test_KErrNone(r);
	count=fileList->Count();
	MatchUidFile(4,0,fileList);
	MatchUidFile(1,1,fileList);
	MatchUidFile(0,2,fileList);
	MatchUidFile(5,3,fileList);
	MatchUidFile(2,4,fileList);
	MatchUidFile(3,5,fileList);
	MatchUidFile(6,6,fileList);
	for (i=7;i<count;i++)
		{
		TEntry entry;
		entry=(*fileList)[i];
		test(entry.iType[2].iUid==0);
		PrintUid(i,fileList);
		}			
	test(i==count);
	delete fileList;
	}

LOCAL_C void testZGetDirByUid()
//
// Get directory contents by matching UIDs from Z:
//
	{

	TUidType matchUid(TUid::Null(),TUid::Uid(0x1000008c),TUid::Null());
	CDir* fileList;
	TInt r=TheFs.GetDir(PlatSec::ConfigSetting(PlatSec::EPlatSecEnforceSysBin)?_L("Z:\\SYS\\BIN\\"):_L("Z:\\SYSTEM\\BIN\\"),matchUid,EAscending,fileList);
	test_KErrNone(r);
	TInt count=fileList->Count();
#if defined(__WINS__)
	test(count==0);
#else
	test.Printf(_L("Count=%d\n"),count);
	while(count--)
		PrintUid(count,fileList);
//	test(count==1);
//	TEntry entry;
//	entry=(*fileList)[0];
//	test(entry.iName.MatchF(_L("EFILE.EXE"))!=KErrNotFound);
#endif
	delete fileList;
	}

LOCAL_C void testGetFilesExcept()
//
// Get all files except read only ...
//
	{

	MakeFile(_L("\\F32-TST\\GDIR\\RONLY1.CCC"),KEntryAttReadOnly);
	MakeFile(_L("\\F32-TST\\GDIR\\RONLY2.CCC"),KEntryAttReadOnly);
	MakeFile(_L("\\F32-TST\\GDIR\\RW1.CCC"));
	MakeFile(_L("\\F32-TST\\GDIR\\RW2.CCC"));
	MakeFile(_L("\\F32-TST\\GDIR\\SYSTEM1.CCC"),KEntryAttSystem);
	MakeFile(_L("\\F32-TST\\GDIR\\SYSTEM2.CCC"),KEntryAttSystem);

	test.Next(_L("Can match only read only files"));
	TUint onlyRO=KEntryAttReadOnly|KEntryAttMatchExclusive;
	CDir* fileList;
	TInt r=TheFs.GetDir(_L("\\F32-TST\\GDIR\\*.CCC"),onlyRO,EAscending,fileList);
	test_KErrNone(r);
	TInt count=fileList->Count();
	test(count==2);

	TEntry entry;
	entry=(*fileList)[0];
	test(entry.iName.MatchF(_L("RONLY1.CCC"))!=KErrNotFound);
	entry=(*fileList)[1];
	test(entry.iName.MatchF(_L("RONLY2.CCC"))!=KErrNotFound);
	delete fileList;

	test.Next(_L("Can match everything except read only files"));
	TUint excludeRO=KEntryAttReadOnly|KEntryAttMatchExclude;
	r=TheFs.GetDir(_L("\\F32-TST\\GDIR\\*.CCC"),excludeRO,EAscending,fileList);
	test_KErrNone(r);
	count=fileList->Count();
	test(count==4);

	entry=(*fileList)[0];
	test(entry.iName.MatchF(_L("RW1.CCC"))!=KErrNotFound);
	entry=(*fileList)[1];
	test(entry.iName.MatchF(_L("RW2.CCC"))!=KErrNotFound);
	entry=(*fileList)[2];
	test(entry.iName.MatchF(_L("SYSTEM1.CCC"))!=KErrNotFound);
	entry=(*fileList)[3];
	test(entry.iName.MatchF(_L("SYSTEM2.CCC"))!=KErrNotFound);
	delete fileList;

	test.Next(_L("Can match everything except system and readonly files"));
	TUint excludeSystemAndRO=KEntryAttReadOnly|KEntryAttSystem|KEntryAttMatchExclude;
	r=TheFs.GetDir(_L("\\F32-TST\\GDIR\\*.CCC"),excludeSystemAndRO,EAscending,fileList);
	test_KErrNone(r);
	count=fileList->Count();
	test(count==2);

	entry=(*fileList)[0];
	test(entry.iName.MatchF(_L("RW1.CCC"))!=KErrNotFound);
	entry=(*fileList)[1];
	test(entry.iName.MatchF(_L("RW2.CCC"))!=KErrNotFound);
	delete fileList;

	r=TheFs.SetAtt(_L("\\F32-TST\\GDIR\\RONLY1.CCC"),0,KEntryAttReadOnly);
	test_KErrNone(r);
	r=TheFs.SetAtt(_L("\\F32-TST\\GDIR\\RONLY2.CCC"),0,KEntryAttReadOnly);
	test_KErrNone(r);
	}

LOCAL_C void testGetHidden()
//
// Match hidden files and directories
//
	{

	test.Next(_L("Match hidden files and directories"));
	MakeFile(_L("File.qqq"));
	MakeFile(_L("FileHidden.qqq"));
	MakeFile(_L("FileSystem.qqq"));
	MakeFile(_L("FileHiddenSystem.qqq"));
	MakeDir(_L("\\F32-TST\\GDIR\\Dir.qqq\\"));
	MakeDir(_L("\\F32-TST\\GDIR\\Dirhidden.qqq\\"));
	MakeDir(_L("\\F32-TST\\GDIR\\Dirsystem.qqq\\"));
	MakeDir(_L("\\F32-TST\\GDIR\\Dirhiddensystem.qqq\\"));

	TInt r=TheFs.SetAtt(_L("FileHidden.qqq"),KEntryAttHidden,0);
	test_KErrNone(r);
	r=TheFs.SetAtt(_L("Filesystem.qqq"),KEntryAttSystem,0);
	test_KErrNone(r);
	r=TheFs.SetAtt(_L("FilehiddenSystem.qqq"),KEntryAttSystem|KEntryAttHidden,0);
	test_KErrNone(r);
	r=TheFs.SetAtt(_L("dirhidden.qqq"),KEntryAttHidden,0);
	test_KErrNone(r);
	r=TheFs.SetAtt(_L("dirsystem.qqq"),KEntryAttSystem,0);
	test_KErrNone(r);
	r=TheFs.SetAtt(_L("dirhiddensystem.qqq"),KEntryAttSystem|KEntryAttHidden,0);
	test_KErrNone(r);

// Files and directories not hidden or system
	CDir* dir;
	r=TheFs.GetDir(_L("*.qqq"),KEntryAttDir,ESortByName,dir);
	test_KErrNone(r);
	TInt count=dir->Count();
	test(count==2);
	TEntry entry;
	entry=(*dir)[0];
	test(entry.iName.MatchF(_L("dir.qqq"))!=KErrNotFound);
	entry=(*dir)[1];
	test(entry.iName.MatchF(_L("file.qqq"))!=KErrNotFound);
	delete dir;
	
// Files only
	r=TheFs.GetDir(_L("*.qqq"),KEntryAttNormal,ESortByName,dir);
	test_KErrNone(r);
	count=dir->Count();
	test(count==1);
	entry=(*dir)[0];
	test(entry.iName.MatchF(_L("file.qqq"))!=KErrNotFound);
	delete dir;

// Directories only
	r=TheFs.GetDir(_L("*.qqq"),KEntryAttDir|KEntryAttMatchExclusive,ESortByName,dir);
	test_KErrNone(r);
	count=dir->Count();
	test(count==1);
	entry=(*dir)[0];
	test(entry.iName.MatchF(_L("dir.qqq"))!=KErrNotFound);
	delete dir;

// Files + hidden
	r=TheFs.GetDir(_L("*.qqq"),KEntryAttHidden,ESortByName,dir);
	test_KErrNone(r);
	count=dir->Count();
	test(count==2);
	entry=(*dir)[0];
	test(entry.iName.MatchF(_L("file.qqq"))!=KErrNotFound);
	entry=(*dir)[1];
	test(entry.iName.MatchF(_L("filehidden.qqq"))!=KErrNotFound);
	delete dir;

// Files + system
	r=TheFs.GetDir(_L("*.qqq"),KEntryAttHidden,ESortByName,dir);
	test_KErrNone(r);
	count=dir->Count();
	test(count==2);
	entry=(*dir)[0];
	test(entry.iName.MatchF(_L("file.qqq"))!=KErrNotFound);
	entry=(*dir)[1];
	test(entry.iName.MatchF(_L("filehidden.qqq"))!=KErrNotFound);
	delete dir;

// Files + hidden + system
	r=TheFs.GetDir(_L("*.qqq"),KEntryAttHidden|KEntryAttSystem,ESortByName,dir);
	test_KErrNone(r);
	count=dir->Count();
	test(count==4);
	entry=(*dir)[0];
	test(entry.iName.MatchF(_L("file.qqq"))!=KErrNotFound);
	entry=(*dir)[1];
	test(entry.iName.MatchF(_L("filehidden.qqq"))!=KErrNotFound);
	entry=(*dir)[2];
	test(entry.iName.MatchF(_L("filehiddensystem.qqq"))!=KErrNotFound);
	entry=(*dir)[3];
	test(entry.iName.MatchF(_L("filesystem.qqq"))!=KErrNotFound);
	delete dir;

// Dirs + hidden
	r=TheFs.GetDir(_L("*.qqq"),KEntryAttHidden|KEntryAttDir|KEntryAttMatchExclusive,ESortByName,dir);
	test_KErrNone(r);
	count=dir->Count();
	test(count==2);
	entry=(*dir)[0];
	test(entry.iName.MatchF(_L("dir.qqq"))!=KErrNotFound);
	entry=(*dir)[1];
	test(entry.iName.MatchF(_L("dirhidden.qqq"))!=KErrNotFound);
	delete dir;

// Dirs + system
	r=TheFs.GetDir(_L("*.qqq"),KEntryAttSystem|KEntryAttDir|KEntryAttMatchExclusive,ESortByName,dir);
	test_KErrNone(r);
	count=dir->Count();
	test(count==2);
	entry=(*dir)[0];
	test(entry.iName.MatchF(_L("dir.qqq"))!=KErrNotFound);
	entry=(*dir)[1];
	test(entry.iName.MatchF(_L("dirsystem.qqq"))!=KErrNotFound);
	delete dir;

// Dirs + hidden + system
	r=TheFs.GetDir(_L("*.qqq"),KEntryAttHidden|KEntryAttSystem|KEntryAttDir|KEntryAttMatchExclusive,ESortByName,dir);
	test_KErrNone(r);
	count=dir->Count();
	test(count==4);
	entry=(*dir)[0];
	test(entry.iName.MatchF(_L("dir.qqq"))!=KErrNotFound);
	entry=(*dir)[1];
	test(entry.iName.MatchF(_L("dirhidden.qqq"))!=KErrNotFound);
	entry=(*dir)[2];
	test(entry.iName.MatchF(_L("dirhiddensystem.qqq"))!=KErrNotFound);
	entry=(*dir)[3];
	test(entry.iName.MatchF(_L("dirsystem.qqq"))!=KErrNotFound);
	
	delete dir;

// Files + Dirs + hidden
	r=TheFs.GetDir(_L("*.qqq"),KEntryAttHidden|KEntryAttDir,ESortByName,dir);
	test_KErrNone(r);
	count=dir->Count();
	test(count==4);
	entry=(*dir)[0];
	test(entry.iName.MatchF(_L("dir.qqq"))!=KErrNotFound);
	entry=(*dir)[1];
	test(entry.iName.MatchF(_L("dirhidden.qqq"))!=KErrNotFound);	
	entry=(*dir)[2];
	test(entry.iName.MatchF(_L("file.qqq"))!=KErrNotFound);
	entry=(*dir)[3];
	test(entry.iName.MatchF(_L("filehidden.qqq"))!=KErrNotFound);
	delete dir;

// Files + Dirs + system
	r=TheFs.GetDir(_L("*.qqq"),KEntryAttSystem|KEntryAttDir,ESortByName,dir);
	test_KErrNone(r);
	count=dir->Count();
	test(count==4);
	entry=(*dir)[0];
	test(entry.iName.MatchF(_L("dir.qqq"))!=KErrNotFound);
	entry=(*dir)[1];
	test(entry.iName.MatchF(_L("dirsystem.qqq"))!=KErrNotFound);
	entry=(*dir)[2];
	test(entry.iName.MatchF(_L("file.qqq"))!=KErrNotFound);
	entry=(*dir)[3];
	test(entry.iName.MatchF(_L("filesystem.qqq"))!=KErrNotFound);
	delete dir;

// Files + Dirs + hidden + system
	r=TheFs.GetDir(_L("*.qqq"),KEntryAttHidden|KEntryAttSystem|KEntryAttDir,ESortByName,dir);
	test_KErrNone(r);
	count=dir->Count();
	test(count==8);
	entry=(*dir)[0];
	test(entry.iName.MatchF(_L("dir.qqq"))!=KErrNotFound);
	entry=(*dir)[1];
	test(entry.iName.MatchF(_L("dirhidden.qqq"))!=KErrNotFound);
	entry=(*dir)[2];
	test(entry.iName.MatchF(_L("dirhiddensystem.qqq"))!=KErrNotFound);
	entry=(*dir)[3];
	test(entry.iName.MatchF(_L("dirsystem.qqq"))!=KErrNotFound);
	entry=(*dir)[4];
	test(entry.iName.MatchF(_L("file.qqq"))!=KErrNotFound);
	entry=(*dir)[5];
	test(entry.iName.MatchF(_L("filehidden.qqq"))!=KErrNotFound);
	entry=(*dir)[6];
	test(entry.iName.MatchF(_L("filehiddensystem.qqq"))!=KErrNotFound);
	entry=(*dir)[7];
	test(entry.iName.MatchF(_L("filesystem.qqq"))!=KErrNotFound);
	delete dir;
	}

LOCAL_D TFileName gDirDescendingBaseName=_L("\\F32-TST\\GDIR\\TDIRDESCENDING\\");
LOCAL_D TFileName gDirDescendingEntryName[6]=
	{
	_L("\\F32-TST\\GDIR\\TDIRDESCENDING\\aaaa"),
	_L("\\F32-TST\\GDIR\\TDIRDESCENDING\\ssss"),
	_L("\\F32-TST\\GDIR\\TDIRDESCENDING\\ZZZZ"),
	_L("\\F32-TST\\GDIR\\TDIRDESCENDING\\aaaa.directory\\"),
	_L("\\F32-TST\\GDIR\\TDIRDESCENDING\\SSSS.dir\\"),
	_L("\\F32-TST\\GDIR\\TDIRDESCENDING\\ZZZZDirectory\\")
	};

LOCAL_C void TestDirDescendingOrder(const TDesC& aResult,const CDir& aDirList)
//
// Test aDirList against aResult
//
	{
	
	TLex lex(aResult);
	TInt count=0;
	while (!lex.Eos())
		{
		lex.Mark();
		while(lex.Get().IsDigit()) {};
		TLex temp(lex.MarkedToken());
		TInt result;
		temp.Val(result);
		TFileName base=gDirDescendingBaseName;
		TEntry entry=aDirList[count];
		base+=entry.iName;
		if (entry.IsDir())
			base+=_L("\\");
		test(base==gDirDescendingEntryName[result]);
		count++;
		}
	}

LOCAL_C void testDirDescending()
//
// Test EDirDescending
//
	{

	test.Next(_L("Test EDirDescending"));
	MakeDir(gDirDescendingBaseName);
	MakeFile(gDirDescendingEntryName[0]);
	MakeFile(gDirDescendingEntryName[1]);
	MakeFile(gDirDescendingEntryName[2]);
	MakeDir(gDirDescendingEntryName[3]);
	MakeDir(gDirDescendingEntryName[4]);
	MakeDir(gDirDescendingEntryName[5]);

// Test DirFirst - EDescending
	CDir* dir;
	TUint sortOrder=ESortByName|EDirsFirst|EDescending;
	TInt r=TheFs.GetDir(gDirDescendingBaseName,KEntryAttMaskSupported,sortOrder,dir);
	test_KErrNone(r);
//	TBuf8<16> result=_L("2,1,0,3,4,5");
	TBuf<16> result=_L("2,1,0,3,4,5");

	TestDirDescendingOrder(result,*dir);
	delete dir;
// Test DirFirst - EAscending
	sortOrder=ESortByName|EDirsFirst;
	r=TheFs.GetDir(gDirDescendingBaseName,KEntryAttMaskSupported,sortOrder,dir);
	test_KErrNone(r);
	result=_L("3,4,5,0,1,2");
	TestDirDescendingOrder(result,*dir);
	delete dir;

// Test DirLast - EDescending
	sortOrder=ESortByName|EDirsLast|EDescending;
	r=TheFs.GetDir(gDirDescendingBaseName,KEntryAttMaskSupported,sortOrder,dir);
	test_KErrNone(r);
	result=_L("3,4,5,2,1,0");
	TestDirDescendingOrder(result,*dir);
	delete dir;
// Test DirLast - EAscending
	sortOrder=ESortByName|EDirsLast;
	r=TheFs.GetDir(gDirDescendingBaseName,KEntryAttMaskSupported,sortOrder,dir);
	test_KErrNone(r);
	result=_L("0,1,2,3,4,5");
	TestDirDescendingOrder(result,*dir);
	delete dir;

// Test DirFirst - EDirDescending
	sortOrder=ESortByName|EDirsFirst|EDirDescending;
	r=TheFs.GetDir(gDirDescendingBaseName,KEntryAttMaskSupported,sortOrder,dir);
	test_KErrNone(r);
	result=_L("5,4,3,0,1,2");
	TestDirDescendingOrder(result,*dir);
	delete dir;
// Test DirLast - EDirDescending
	sortOrder=ESortByName|EDirsLast|EDirDescending;
	r=TheFs.GetDir(gDirDescendingBaseName,KEntryAttMaskSupported,sortOrder,dir);
	test_KErrNone(r);
	result=_L("0,1,2,5,4,3");
	TestDirDescendingOrder(result,*dir);
	delete dir;

// Test DirFirst - EDescending|EDirDescending
	sortOrder=ESortByName|EDirsFirst|EDescending|EDirDescending;
	r=TheFs.GetDir(gDirDescendingBaseName,KEntryAttMaskSupported,sortOrder,dir);
	test_KErrNone(r);
	result=_L("2,1,0,5,4,3");
	TestDirDescendingOrder(result,*dir);
	delete dir;
// Test DirLast - EDescending|EDirDescending
	sortOrder=ESortByName|EDirsLast|EDirDescending|EDescending;
	r=TheFs.GetDir(gDirDescendingBaseName,KEntryAttMaskSupported,sortOrder,dir);
	test_KErrNone(r);
	result=_L("5,4,3,2,1,0");
	TestDirDescendingOrder(result,*dir);
	delete dir;

// Test DirNoOrder - EDescending|EDirDescending
	sortOrder=ESortByName|EDescending|EDirDescending;
	r=TheFs.GetDir(gDirDescendingBaseName,KEntryAttMaskSupported,sortOrder,dir);
	test_KErrNone(r);
	result=_L("5,2,4,1,3,0");
	TestDirDescendingOrder(result,*dir);
	delete dir;
// Test DirNoOrder - EDescending
	sortOrder=ESortByName|EDescending;
	r=TheFs.GetDir(gDirDescendingBaseName,KEntryAttMaskSupported,sortOrder,dir);
	test_KErrNone(r);
	result=_L("5,2,4,1,3,0");
	TestDirDescendingOrder(result,*dir);
	delete dir;
// Test DirNoOrder - EAscending
	sortOrder=ESortByName;
	r=TheFs.GetDir(gDirDescendingBaseName,KEntryAttMaskSupported,sortOrder,dir);
	test_KErrNone(r);
	result=_L("0,3,1,4,2,5");
	TestDirDescendingOrder(result,*dir);
	delete dir;
	}

//--------------------------------------------- 
//! @SYMTestCaseID			PBASE-T_GDIR-0815
//! @SYMTestType			UT
//! @SYMREQ					DEF122894
//! @SYMTestCaseDesc		This testcase tests the boundary condition of file name collation (8 characters).
//! @SYMTestActions			Creates file "xxxxxxxx2.dat" and "Xxxxxxxx1.dat" under same directory,
//! 						 retrieves dir list via GetDir(), sort list by name, check order of the file listed.  
//! @SYMTestExpectedResults File "Xxxxxxxx1.dat" should be listed before "xxxxxxxx2.dat".
//! @SYMTestPriority		High
//! @SYMTestStatus			Implemented
//--------------------------------------------- 	
void TestDEF122894()
	{
	test.Next(_L("Test \"DEF122894: Defect in RFs GetDir() API\""));
	MakeFile(_L("\\F32-TST\\GDIR\\DEF122894\\xxxxxxxx2.dat"));
	MakeFile(_L("\\F32-TST\\GDIR\\DEF122894\\Xxxxxxxx1.dat"));
	CDir* dir;
	TInt r=TheFs.GetDir(_L("\\F32-TST\\GDIR\\DEF122894\\"),KEntryAttMaskSupported,ESortByName|EAscending,dir);
	test_KErrNone(r);
	test(dir->Count() == 2);
	TEntry entry1, entry2;
	entry1 = (*dir)[0];
	entry2 = (*dir)[1];
	test(entry1.iName.Compare(_L("Xxxxxxxx1.dat")) == 0);
	test(entry2.iName.Compare(_L("xxxxxxxx2.dat")) == 0);
	delete dir;
	}

GLDEF_C void CallTestsL()
//
// Test directory handling.
//
    {

	CreateTestDirectory(_L("\\F32-TST\\GDIR\\"));
	if (gFirstRun)
		{
		gFirstRun=EFalse;
		testZDirectory();
		testZGetDirByUid();
		}

	testSetup();
	testDir();
	testDisplayFiles();
	testGetDirByUid();
	testGetFilesExcept();
	testGetHidden();
	testDirDescending();
	TestDEF122894();
	DeleteTestDirectory();
    }

