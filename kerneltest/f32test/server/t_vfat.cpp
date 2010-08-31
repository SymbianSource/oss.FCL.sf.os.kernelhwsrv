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
// f32test\server\t_vfat.cpp
// 
//

#define __E32TEST_EXTENSION__
#include <f32file.h>
#include <e32test.h>
#include <e32math.h>
#include "t_server.h"

RTest test(_L("T_VFAT"));

static void Test1()
//
// Create 71 8.3 files
// Rename each of them to vfat filenames (% order)
// Chkdsk
// Check entries
//
	{

	test.Next(_L("Test rename"));
	TInt totalFiles=103;
	TInt orderMod=61;
	TFileName nameshort=_L("File");
	TFileName namelong=_L("File.+',;'=[]");
	TInt i;
	TBuf8<256> data;

	for (i=0;i<totalFiles;i++)
		{
		TBuf<32> tempName=nameshort;
		tempName.AppendNum(i);
		data.SetLength(i);
		MakeFile(tempName,data);
		}

	TInt count=totalFiles;
	while(count--)
		{
		TInt fileNum=(orderMod*count)%totalFiles;
		TBuf<32> shortName=nameshort;
		shortName.AppendNum(fileNum);
		TBuf<32> longName=namelong;
		longName.AppendNum(fileNum);
		TInt r=TheFs.Rename(shortName,longName);
		test_KErrNone(r);
		}

	TInt r=TheFs.CheckDisk(gSessionPath);
	test_Value(r, r == KErrNone || r==KErrNotSupported);

	CDir* dirList;
	r=TheFs.GetDir(_L("*.*"),KEntryAttMaskSupported,ESortBySize,dirList);
	test_KErrNone(r);
	test(dirList->Count()==totalFiles);
	for (i=0;i<totalFiles;i++)
		{
		TBuf<32> longName=namelong;
		longName.AppendNum(i);
		TEntry entry;
		entry=(*dirList)[i];
		test(entry.iName.MatchF(longName)!=KErrNotFound);
		}

	delete dirList;
	}


#ifdef __WINS__
const TInt gMaxIterations=1000;
#else
const TInt gMaxIterations=1000;	// Have pity on a poor 18MHz CPU
#endif

const TInt gMaxFiles=256;
TBuf<gMaxFiles> gDataBuf;
TBuf8<gMaxFiles> buf;
TFileName gFileName[gMaxFiles];

LOCAL_C void Test2()
//
// Random test
// Generate random numbers fileNum, fileOp.
// fileOp = 1, shortname
// fileOp = 2, longName
// fileOp = 3, delete file
// 
	{

	TInt i;
	test.Next(_L("Random test"));
	TInt64 seed=51703;
	TInt maxIterations=gMaxIterations;
	TInt maxFileOps=3;
	TInt checkFrequency=100; // 1 in xxxx
	
	for(i=0;i<gMaxFiles;i++)
		gFileName[i]=_L("");

	TFileName fileName;
	while(maxIterations--)
		{
		TInt fileNum=Math::Rand(seed)%gMaxFiles;
		TInt fileOp=Math::Rand(seed)%maxFileOps;
		switch(fileOp)
			{
		case 0:
			CreateShortName(fileName,seed);
			break;
		case 1:
			CreateLongName(fileName,seed);
			break;
		case 2:
			TInt r;
			fileName=gFileName[fileNum];
			if (fileName==_L(""))
				goto End;
			r=TheFs.Delete(fileName);
			test_KErrNone(r);
			gFileName[fileNum]=_L("");
			goto End;
		default:
			User::Panic(_L("IllegalVal"),KErrGeneral);
			};
		
		if (gFileName[fileNum]==_L(""))
			{
			/* Delete any existing file with the same name */
			TInt r;
			RFile thing;
			r=thing.Open(TheFs, fileName, EFileShareAny);
			test_Value(r, r == KErrNone || r==KErrNotFound);
			if (r==KErrNone)
				{
				TInt s;
				test (thing.Size(s) == KErrNone);
				thing.Close();
				r=TheFs.Delete(fileName);
				test_KErrNone(r);
				gFileName[s]=_L("");
				}
			else
				thing.Close();
				
			gDataBuf.SetLength(fileNum);
			/* the return from the following function was being checked and the next would only be
			carried out if the return showed no error.  But this function never returns anything so
			the code wasn't compiling */
			buf.Copy(gDataBuf);	// Unicode
			MakeFile(fileName,buf);
			gFileName[fileNum]=fileName;
			}
		else
			{
			TInt r=TheFs.Rename(gFileName[fileNum],fileName);
            if (r != KErrNone && r != KErrAlreadyExists)
                test.Printf(_L("Rename returned %d at line %d"), r, __LINE__);
			test_Value(r, r == KErrNone || r==KErrAlreadyExists);
			if (r==KErrNone)
				gFileName[fileNum]=fileName;
			}
End:
		if ((maxIterations%checkFrequency)==0)
			{
			test.Printf(_L("Iteration %d    \r"),gMaxIterations-maxIterations);
			TInt r=TheFs.CheckDisk(gSessionPath);
			test_Value(r, r == KErrNone || r==KErrNotSupported);
			TInt count=0;
			CDir* dirList;
			r=TheFs.GetDir(_L("*.*"),KEntryAttMaskSupported,ESortBySize,dirList);
			test_KErrNone(r);
			for(i=0;i<gMaxFiles;i++)
				{
				if (gFileName[i]==_L(""))
					continue;
				TEntry entry;
				entry=(*dirList)[count];
				TInt r=gFileName[i].MatchF(entry.iName);
				if (r==KErrNotFound)
					{
                    //-- tests a dodgy case when the name has multiple trailing dots. They must have been removed by FS implementation
                    TUint len=gFileName[i].Length();
                    test(gFileName[i][len-1]=='.');

                    //-- strip all trailing dots from the original name
                    while(len)
                    {
                        if(gFileName[i][len-1]=='.')
                            len--;
                        else
                            break;
                    }

                    TPtrC ptrFileName(gFileName[i].Ptr(), len);

                    test(ptrFileName.CompareF(entry.iName) == 0);


					}
				count++;
				}
			delete dirList;
			}
		}
	test.Printf(_L("\n"),i);
	}

//-----------------------------------------------------------------
_LIT(KName1, "\\file1");
_LIT(KName2, "\\file1.");
_LIT(KName3, "\\file1..");
_LIT(KName4, "\\file1...");
_LIT(KExpectedName, "file1");

void DoCheckTD_FN()
{
    TInt    nRes;
    TEntry  entry;

    nRes = TheFs.Entry(KName1, entry);
    test_KErrNone(nRes);
    test(entry.iName.CompareF(KExpectedName) == 0);

    nRes = TheFs.Entry(KName2, entry);
    test_KErrNone(nRes);
    test(entry.iName.CompareF(KExpectedName) == 0);

    nRes = TheFs.Entry(KName3, entry);
    test_KErrNone(nRes);
    test(entry.iName.CompareF(KExpectedName) == 0);

    nRes = TheFs.Entry(KName3, entry);
    test_KErrNone(nRes);
    test(entry.iName.CompareF(KExpectedName) == 0);
}

/**
    Test that ALL trailing dots are removed from the file names by filsystem implementation
*/
void TestTrailingDots()
{
    test.Next(_L("Test trailing dots"));

    //-- actually, all these APIs shall be tested:
    //-- CMountCB::MkDirL()
    //-- CMountCB::RmDirL()
    //-- CMountCB::DeleteL()
    //-- CMountCB::RenameL()
    //-- CMountCB::ReplaceL()
    //-- CMountCB::EntryL() const
    //-- CMountCB::SetEntryL()
    //-- CMountCB::FileOpenL()
    //-- CMountCB::DirOpenL()
    //-- CMountCB::ReadSectionL()
    //-- CFileCB::RenameL()


    TInt    nRes;
    RFile   file;

    //----- create and check "\\file1"
    nRes = file.Replace(TheFs, KName1, EFileWrite);
    test_KErrNone(nRes);
    file.Close();
    
    DoCheckTD_FN();

    nRes = TheFs.Delete(KName1);
    test_KErrNone(nRes);


    //----- create and check "\\file1."
    nRes = file.Replace(TheFs, KName2, EFileWrite);
    test_KErrNone(nRes);
    file.Close();
    
    DoCheckTD_FN();

    nRes = TheFs.Delete(KName2);
    test_KErrNone(nRes);


    //----- create and check "\\file1.."
    nRes = file.Replace(TheFs, KName3, EFileWrite);
    test_KErrNone(nRes);
    file.Close();
    
    DoCheckTD_FN();

    nRes = TheFs.Delete(KName3);
    test_KErrNone(nRes);


    //----- create and check "\\file1..."
    nRes = file.Replace(TheFs, KName4, EFileWrite);
    test_KErrNone(nRes);
    file.Close();
    
    DoCheckTD_FN();
    
    nRes = TheFs.Delete(KName4);
    test_KErrNone(nRes);


}

void CallTestsL()
//
// Do tests relative to session path
//
	{

	TurnAllocFailureOff();

	CreateTestDirectory(_L("\\F32-TST\\TVFAT\\"));

	Test1();
	DeleteTestDirectory();
	CreateTestDirectory(_L("\\F32-TST\\TVFAT\\"));
	Test2();

    TestTrailingDots();

	DeleteTestDirectory();
	}

