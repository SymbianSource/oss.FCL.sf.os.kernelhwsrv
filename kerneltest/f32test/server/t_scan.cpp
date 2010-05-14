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
//

#define	__E32TEST_EXTENSION__
#include <f32file.h>
#include <e32test.h>
#include "t_server.h"

GLDEF_D RTest test(_L("T_SCAN"));

LOCAL_C void BuildTestDir()
//
// Build up a test directory structure
//

/*
//               SCANTEST
//                  | 
//  File1 File2 Left File3 Right Empty(Directory)
//             /  \        /  \
//           Dir2  Dir3 File4 File5
//            |      |
//           File6 Dir4----Hidden---HiddenFile       
//                   |      /  \
//                 File7 File8 System
//                              |
//                             File9
*/
	{

	MakeDir(_L("\\F32-TST\\SCANTEST\\Empty\\"));

	MakeFile(_L("\\F32-TST\\SCANTEST\\File1"));
	MakeFile(_L("\\F32-TST\\SCANTEST\\File2"));
	MakeFile(_L("\\F32-TST\\SCANTEST\\File3"));
	
	MakeFile(_L("\\F32-TST\\SCANTEST\\Right\\File4"));
	MakeFile(_L("\\F32-TST\\SCANTEST\\Right\\File5"));

	MakeFile(_L("\\F32-TST\\SCANTEST\\Left\\Dir2\\File6"));
	MakeFile(_L("\\F32-TST\\SCANTEST\\Left\\Dir3\\Dir4\\File7"));

	MakeFile(_L("\\F32-TST\\SCANTEST\\Left\\Dir3\\Dir4\\Hidden\\File8"));
	MakeFile(_L("\\F32-TST\\SCANTEST\\Left\\Dir3\\Dir4\\Hidden\\HiddenFile"));
	MakeFile(_L("\\F32-TST\\SCANTEST\\Left\\Dir3\\Dir4\\Hidden\\System\\File9"));

	TInt r;
	r=TheFs.SetAtt(_L("\\F32-TST\\SCANTEST\\Left\\Dir3\\Dir4\\Hidden"), KEntryAttHidden, 0);
	test_KErrNone(r);
	r=TheFs.SetAtt(_L("\\F32-TST\\SCANTEST\\Left\\Dir3\\Dir4\\Hidden\\HiddenFile"), KEntryAttHidden, 0);
	test_KErrNone(r);
	r=TheFs.SetAtt(_L("\\F32-TST\\SCANTEST\\Left\\Dir3\\Dir4\\Hidden\\System"), KEntryAttSystem, 0);
	test_KErrNone(r);
	}

LOCAL_C void Test1()
//
// Test all methods
//
	{
	
	test.Next(_L("Create scanner"));
	CDirScan* scanner=CDirScan::NewL(TheFs);
	TParse dirName;
	TheFs.Parse(_L("\\F32-TST\\SCANTEST\\"),dirName);
	scanner->SetScanDataL(dirName.FullName(),KEntryAttDir,ESortByName);
	CDir* entryList;
//
	test.Next(_L("Scan top level directory"));
	scanner->NextL(entryList);
	TInt count=entryList->Count();
	test(count==6);
	TEntry entry=(*entryList)[0];
	test(entry.iName==_L("Empty"));
	entry=(*entryList)[1];
	test(entry.iName==_L("File1"));
	entry=(*entryList)[2];
	test(entry.iName==_L("File2"));
	entry=(*entryList)[3];
	test(entry.iName==_L("File3"));
	entry=(*entryList)[4];
	test(entry.iName==_L("Left"));
	entry=(*entryList)[5];
	test(entry.iName==_L("Right"));
	delete entryList;
//
	test.Next(_L("Reset scanner"));
	scanner->SetScanDataL(dirName.FullName(),KEntryAttDir,ESortByName);
//
	test.Next(_L("Scan ascending: ScanTest directory"));
	scanner->NextL(entryList);
	count=entryList->Count();
	test(count==6);
	entry=(*entryList)[0];
	test(entry.iName==_L("Empty"));
	entry=(*entryList)[1];
	test(entry.iName==_L("File1"));
	entry=(*entryList)[2];
	test(entry.iName==_L("File2"));
	entry=(*entryList)[3];
	test(entry.iName==_L("File3"));
	entry=(*entryList)[4];
	test(entry.iName==_L("Left"));
	entry=(*entryList)[5];
	test(entry.iName==_L("Right"));
	delete entryList;
//
	test.Next(_L("Check next directory: Empty"));
	scanner->NextL(entryList);
	count=entryList->Count();
	test(count==0);
	delete entryList;
//
	test.Next(_L("Check next directory: Left"));
	scanner->NextL(entryList);
	count=entryList->Count();
	test(count==2);
	entry=(*entryList)[0];
	test(entry.iName==_L("Dir2"));
	entry=(*entryList)[1];
	test(entry.iName==_L("Dir3"));
	delete entryList;
//
	test.Next(_L("Check next directory: Left\\Dir2"));
	scanner->NextL(entryList);
	count=entryList->Count();
	test(count==1);
	entry=(*entryList)[0];
	//test(entry.iName==_L("File6"));
	delete entryList;
//
	test.Next(_L("Check next directory: Left\\Dir3"));
	scanner->NextL(entryList);
	count=entryList->Count();
	test(count==1);
	entry=(*entryList)[0];
	test(entry.iName==_L("Dir4"));
	delete entryList;
//
	test.Next(_L("Check next directory: Left\\Dir3\\Dir4"));
	scanner->NextL(entryList);
	count=entryList->Count();
	test(count==1);
	entry=(*entryList)[0];
	test(entry.iName==_L("File7"));
	delete entryList;
//
	test.Next(_L("Check next directory: Right"));
	scanner->NextL(entryList);
	count=entryList->Count();
	test(count==2);
	entry=(*entryList)[0];
	test(entry.iName==_L("File4"));
	entry=(*entryList)[1];
	test(entry.iName==_L("File5"));
	delete entryList;
//
	test.Next(_L("End of scan"));
	scanner->NextL(entryList);
	test(entryList==NULL);
	delete entryList;
	delete scanner;
	}	

LOCAL_C void Test2()
//
// Scan subset of test directory structure and test abs/rel paths
//
	{

	test.Next(_L("Scan descending: ScanTest\\Left "));
	CDirScan* scanner=CDirScan::NewL(TheFs);
	TParse dirName;
	TheFs.Parse(_L("\\F32-TST\\SCANTEST\\LEFT\\"),dirName);
	scanner->SetScanDataL(dirName.FullName(),KEntryAttNormal,ESortByName|EDescending);
//
	test.Next(_L("Check next directory: Left"));
	CDir* entryList;
	scanner->NextL(entryList);
	TInt count=entryList->Count();
	test(count==0);
	test(scanner->AbbreviatedPath()==_L("\\"));
	TheFs.Parse(_L("\\F32-TST\\SCANTEST\\LEFT\\"),dirName);
	test(scanner->FullPath()==dirName.FullName());
	delete entryList;
//
	test.Next(_L("Check next directory: Left\\Dir3"));
	scanner->NextL(entryList);
	count=entryList->Count();
	test(count==0);
	test(scanner->AbbreviatedPath()==_L("\\Dir3\\"));
	TheFs.Parse(_L("\\F32-TST\\SCANTEST\\LEFT\\Dir3\\"),dirName);
	test(scanner->FullPath()==dirName.FullName());
	delete entryList;
//
	test.Next(_L("Check next directory: Left\\Dir3\\Dir4"));
	scanner->NextL(entryList);
	count=entryList->Count();
	test(count==1);
	test((*entryList)[0].iName==_L("File7"));
	test(scanner->AbbreviatedPath()==_L("\\Dir3\\Dir4\\"));
	TheFs.Parse(_L("\\F32-TST\\SCANTEST\\LEFT\\Dir3\\Dir4\\"),dirName);
	test(scanner->FullPath()==dirName.FullName());
	delete entryList;
//
	test.Next(_L("Check next directory: Left\\Dir2"));
	scanner->NextL(entryList);
	count=entryList->Count();
	test(count==1);
	test((*entryList)[0].iName==_L("File6"));
	test(scanner->AbbreviatedPath()==_L("\\Dir2\\"));
	TheFs.Parse(_L("\\F32-TST\\SCANTEST\\LEFT\\Dir2\\"),dirName);
	test(scanner->FullPath()==dirName.FullName());
	delete entryList;
	delete scanner;
	}

LOCAL_C void Test3()
//
// Print directory structure
//
	{

	test.Next(_L("List directory structure"));
	TheFs.SetAllocFailure(gAllocFailOff);
	TFileName sessionPath;
	TInt r=TheFs.SessionPath(sessionPath);
	test_KErrNone(r);
	r=TheFs.SetSessionPath(_L("N:\\"));
	test_KErrNone(r);
	TAutoClose<RFs> fs;
	r=fs.iObj.Connect();
	test_KErrNone(r);
	CDirScan* scanner=CDirScan::NewL(fs.iObj);
	TParse dirName;
	TheFs.Parse(sessionPath,dirName);
	scanner->SetScanDataL(dirName.FullName(),KEntryAttDir,ESortByName);
	CDir* entryList;
	FOREVER
		{
		scanner->NextL(entryList);
		if (entryList==NULL)
			break;
		TInt count=entryList->Count();
		while (count--)
			{
			TEntry data=(*entryList)[count];
			TBuf<KMaxFileName> path=scanner->AbbreviatedPath();
			test.Printf(_L("%S%S\n"),&path,&data.iName);
			}
		delete entryList;
		entryList=NULL;
		}
	delete scanner;
	r=TheFs.SetSessionPath(sessionPath);
	test_KErrNone(r);
	TheFs.SetAllocFailure(gAllocFailOn);
	}

LOCAL_C void Test4()
//
// Scan for a matching filename
//
	{

	test.Start(_L("Scan for a matching filename"));
	MakeFile(_L("\\F32-TST\\SCANTEST\\MaTCHteST.EXTENSION"));
	MakeFile(_L("\\F32-TST\\SCANTEST\\MATCH.EXT"));
	MakeFile(_L("\\F32-TST\\SCANTEST\\LEFT\\DIR3\\ANOTherteST.EXTENSION"));
	MakeFile(_L("\\F32-TST\\SCANTEST\\LEFT\\DIR3\\APPDLL.DLL"));
	MakeFile(_L("\\F32-TST\\SCANTEST\\LEFT\\DIR3\\TEST.EXTENSION"));
	MakeFile(_L("\\F32-TST\\SCANTEST\\LEFT\\DIR3\\DIR4\\MaTCHteST.EXTENSION"));

	CDir* entryList;
	CDirScan* scanner=CDirScan::NewL(TheFs);
	TParse dirName;

	test.Next(_L("Scan for MAT??.EXT")); // can't test for MAT*.EXT because shortname of MaTCHteST.EXTENSION
	TheFs.Parse(_L("\\F32-TST\\SCANTEST\\MAT??.EXT"),dirName); // is MATCHT~1.EXT !!!
	scanner->SetScanDataL(dirName.FullName(),KEntryAttNormal,ESortByName);
	scanner->NextL(entryList);
	test((TInt)entryList);
	test(entryList->Count()==1);
	test((*entryList)[0].iName.MatchF(_L("MATCH.EXT"))!=KErrNotFound);
	test(scanner->AbbreviatedPath()==_L("\\"));
	TheFs.Parse(_L("\\F32-TST\\SCANTEST\\"),dirName);
	test(scanner->FullPath()==dirName.FullName());
	delete entryList;
	FOREVER
		{
		scanner->NextL(entryList);
		if (entryList==NULL)
			break;
		test(entryList->Count()==0);
		delete entryList;
		}

	test.Next(_L("Scan for *.EXTENSION"));
	TheFs.Parse(_L("\\F32-TST\\SCANTEST\\*.EXTENSION"),dirName);
	scanner->SetScanDataL(dirName.FullName(),KEntryAttNormal,ESortByName);
	scanner->NextL(entryList);
	test((TInt)entryList);
	test(entryList->Count()==1);
	test((*entryList)[0].iName.MatchF(_L("MATCHTEST.EXTENSION"))!=KErrNotFound);
	test(scanner->AbbreviatedPath()==_L("\\"));
	TheFs.Parse(_L("\\F32-TST\\SCANTEST\\"),dirName);
	test(scanner->FullPath()==dirName.FullName());
	do {
		delete entryList;
		scanner->NextL(entryList);
		test((TInt)entryList);
		} while (entryList->Count()==0);
	test(entryList->Count()==2);
	test((*entryList)[0].iName.MatchF(_L("ANOTHERTEST.EXTENSION"))!=KErrNotFound);
	test((*entryList)[1].iName.MatchF(_L("test.EXTENSION"))!=KErrNotFound);
	test(scanner->AbbreviatedPath().MatchF(_L("\\LEFT\\DIR3\\"))!=KErrNotFound);
	TheFs.Parse(_L("\\F32-TST\\SCANTEST\\LEFT\\DIR3\\"),dirName);
	test(scanner->FullPath().MatchF(dirName.FullName())!=KErrNotFound);
	do {
		delete entryList;
		scanner->NextL(entryList);
		test((TInt)entryList);
		} while (entryList->Count()==0);
	test(entryList->Count()==1);
	test((*entryList)[0].iName.MatchF(_L("MATCHTEST.EXTENSION"))!=KErrNotFound);
	test(scanner->AbbreviatedPath().MatchF(_L("\\LEFT\\DIR3\\DIR4\\"))!=KErrNotFound);
	TheFs.Parse(_L("\\F32-TST\\SCANTEST\\LEFT\\DIR3\\DIR4\\"),dirName);
	test(scanner->FullPath().MatchF(dirName.FullName())!=KErrNotFound);
	delete entryList;
	FOREVER
		{
		scanner->NextL(entryList);
		if (entryList==NULL)
			break;
		test(entryList->Count()==0);
		delete entryList;
		}

	test.Next(_L("Scan for APPDLL.DLL"));
	TheFs.Parse(_L("\\F32-TST\\SCANTEST\\APPDLL.DLL"),dirName);
	scanner->SetScanDataL(dirName.FullName(),KEntryAttNormal,ESortByName);
	FOREVER
		{
		scanner->NextL(entryList);
		if (entryList->Count()!=0)
			break;
		test((TInt)entryList);
		delete entryList;
		}
	test(entryList->Count()==1);
	test((*entryList)[0].iName==_L("APPDLL.DLL"));
	test(scanner->AbbreviatedPath().MatchF(_L("\\LEFT\\DIR3\\"))!=KErrNotFound);
	TheFs.Parse(_L("\\F32-TST\\SCANTEST\\LEFT\\DIR3\\"),dirName);
	test(scanner->FullPath().MatchF(dirName.FullName())!=KErrNotFound);
	delete entryList;
	FOREVER
		{
		scanner->NextL(entryList);
		if (entryList==NULL)
			break;
		test(entryList->Count()==0);
		delete entryList;
		}
	delete scanner;
	test.End();
	}		

LOCAL_C void Test5()
//
// Do a scan of Z:
//
	{

	test.Next(_L("List Z: directory structure"));
	CDirScan* scanner=CDirScan::NewLC(TheFs);
	TPtrC romPath(_L("Z:\\"));
	TParse dirName;
	TInt r=TheFs.Parse(romPath,dirName);
	test_KErrNone(r);
	scanner->SetScanDataL(dirName.FullName(),KEntryAttDir,ESortByName);
	CDir* entryList;
	FOREVER
		{
		scanner->NextL(entryList);
		if (entryList==NULL)
			break;
		TInt count=entryList->Count();
		while (count--)
			{
			TEntry data=(*entryList)[count];
			TBuf<KMaxFileName> path=scanner->AbbreviatedPath();
			test.Printf(_L("%S%S\n"),&path,&data.iName);
			}
		delete entryList;
		entryList=NULL;
		}
	CleanupStack::PopAndDestroy();
	}

LOCAL_C void Test6()
//
// Scan up a directory tree
//
	{

	test.Next(_L("Test ascending scan"));
	MakeFile(_L("\\F32-TST\\SCANTEST\\MaTCHteST.EXTENSION"));
	MakeFile(_L("\\F32-TST\\SCANTEST\\MATCH.EXT"));
	MakeFile(_L("\\F32-TST\\SCANTEST\\LEFT\\DIR3\\ANOTherteST.EXTENSION"));
	MakeFile(_L("\\F32-TST\\SCANTEST\\LEFT\\DIR3\\APPDLL.DLL"));
	MakeFile(_L("\\F32-TST\\SCANTEST\\LEFT\\DIR3\\TEST.EXTENSION"));
	MakeFile(_L("\\F32-TST\\SCANTEST\\LEFT\\DIR3\\DIR4\\MaTCHteST.EXTENSION"));
	
	CDir* entryList;
	CDirScan* scanner=CDirScan::NewL(TheFs);
	TParse dirName;

	test.Next(_L("Scan for MAT*.EXT"));
	TheFs.Parse(_L("\\F32-TST\\SCANTEST\\"),dirName);
//	TheFs.Parse(_L("\\F32-TST\\SCANTEST\\LEFT\\DIR3\\DIR4\\MaTCHteST.EXTENSION"),dirName);
//	scanner->SetScanDataL(dirName.FullName(),KEntryAttNormal,ESortByName,CDirScan::EScanUpTree);
	scanner->SetScanDataL(dirName.FullName(),KEntryAttDir,ESortByName,CDirScan::EScanUpTree);
	
	FOREVER
		{
		scanner->NextL(entryList);
		if (entryList==NULL)
			break;
		TInt count=entryList->Count();
		while(count--)
			{
			TEntry entry=(*entryList)[count];
			test.Printf(_L("entry.iName = %S\n"),&entry.iName);
		//	test(entry.iName==_L("MATCH.EXT"));
			}
		delete entryList;
		}
	
	delete scanner;
	}


LOCAL_C void Test7()
//
// Test scanning hidden directories
//
	{

	CDir* entryList;
	CDirScan* scanner=CDirScan::NewL(TheFs);
	TParse dirName;

	test.Next(_L("Scan in hidden directories for files"));
	TheFs.Parse(_L("\\F32-TST\\SCANTEST\\LEFT\\DIR3\\*"),dirName);
	scanner->SetScanDataL(dirName.FullName(),KEntryAttHidden,ESortByName,CDirScan::EScanDownTree);
	
	TInt hiddenCount=0;
	FOREVER
		{
		scanner->NextL(entryList);
		if (entryList==NULL)
			break;
		TInt count=entryList->Count();
		while(count--)
			{
			TEntry entry=(*entryList)[count];
			test.Printf(_L("entry.iName = %S\n"),&entry.iName);
			if (entry.IsHidden())
				hiddenCount++;
			if (entry.iName==_L("File8"))
				hiddenCount++;
			}
		delete entryList;
		}
	test(hiddenCount==2);

	test.Next(_L("Scan for system and hidden directories and files"));
	TheFs.Parse(_L("\\F32-TST\\SCANTEST\\LEFT\\DIR3\\*"),dirName);
	scanner->SetScanDataL(dirName.FullName(),KEntryAttSystem|KEntryAttHidden|KEntryAttDir,ESortByName,CDirScan::EScanDownTree);

	hiddenCount=0;
	FOREVER
		{
		scanner->NextL(entryList);
		if (entryList==NULL)
			break;
		TInt count=entryList->Count();
		while(count--)
			{
			TEntry entry=(*entryList)[count];
			test.Printf(_L("entry.iName = %S\n"),&entry.iName);
			if (entry.IsHidden())
				hiddenCount++;
			if (entry.iName==_L("File8"))
				hiddenCount++;
			if (entry.IsSystem())
				hiddenCount++;
			if (entry.iName==_L("File9"))
				hiddenCount++;
			}
		delete entryList;
		}
	test(hiddenCount==5);
	
	delete scanner;
	}

GLDEF_C void CallTestsL()
//
// Call all tests
//
	{

	CreateTestDirectory(_L("\\F32-TST\\SCANTEST\\"));
	DeleteTestDirectory();
	BuildTestDir();
	Test1();
	Test2();
	Test3();
	Test4();
	Test5();
	DeleteTestDirectory();
	Test6();
	DeleteTestDirectory();

	BuildTestDir();
	Test7();
	DeleteTestDirectory();
	}
