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

#define __E32TEST_EXTENSION__
#include <f32file.h>
#include <e32test.h>
#include <e32math.h>
#include <e32hal.h>
#include "t_server.h"
#include "t_chlffs.h"

#include "f32_test_utils.h"
using namespace F32_Test_Utils;

RTest test(_L("B_GEN"));
//
// File test - general test of local filing system routines
//             (finishes with formating current device).
// Modified from BB's epoc test code

#define MINIMUM_DATE (315532800L)
#define DateTests   10


TBuf<0x100> gNameBuf;
TBuf<0x100> gNameOut;
RFile gFile;
RFile gFileErr;
RFile gFile2;
RDir gDir;
RFormat gFormat;
TFileName fBuf;
TInt gDriveNum = -1;


static TBuf8<0x4000> gDataBuf;
static TEntry gFileEntry;
static TVolumeInfo volInfo;
static TFileName pathBuf;





static void doError(const TDesC &aMess, TInt anErr, TInt line)
	{ 
    test.Printf(_L("%S failed at line %d. Error %d\n"),&aMess, line, anErr);
    test(0);
	}
#define Error(aMess, anErr) doError(aMess, anErr, __LINE__)

static void doError2(const TDesC &aMess, TInt anErr, TInt line, TInt callLine)
	{ 
    test.Printf(_L("%S failed at line %d. Error %d. Called from line %d\n"),&aMess, line, anErr, callLine); \
    test(0);
	}
#define Error2(aMess, anErr, line) doError2(aMess, anErr, __LINE__, line)

static void testWrite(const TDesC& aName,TInt aLen,TInt32 aSize,TBool aShouldChange)
//
// Write to a file
//
    {

	TTime saveTime;
	TInt c;
	test.Printf(_L("Write %u bytes %u\n"),aLen,aShouldChange);
	if ((c=TheFs.Entry(aName,gFileEntry))!=KErrNone)
		Error(_L("File info 600"),c);
	saveTime=gFileEntry.iModified;
	User::After(3000000L); // 30 tenths of a sec = 30 00000 micro seconds
	if ((c=gFile.Open(TheFs,aName,EFileWrite))!=KErrNone)
		Error(_L("Open 50"),c);
	if ((c=gFile.Write(gDataBuf,aLen))!=KErrNone)
		Error(_L("Write"),c);
	gFile.Close();
	if ((c=TheFs.Entry(gNameOut,gFileEntry))!=KErrNone)
		Error(_L("File info 601"),c);
	if ((saveTime!=gFileEntry.iModified)!=aShouldChange)
		Error(_L("Change flag check 6"),0);
	if (gFileEntry.iSize!=aSize)
		Error(_L("Size check 602"),0);
    }

static void testSetEof(const TDesC& aName,TUint32 aPos,TBool aShouldChange)
//
// Set the end of a file
//
	{

	TTime saveTime;
	TInt c;
	test.Printf(_L("Set EOF to %u %u\n"),aPos,aShouldChange);
	if ((c=TheFs.Entry(aName,gFileEntry))!=KErrNone)
		Error(_L("File info 500"),c);
	saveTime=gFileEntry.iModified;
	User::After(3000000L);
	if ((c=gFile.Open(TheFs,aName,EFileWrite))!=KErrNone)
		Error(_L("Open 50"),c);
	if ((c=gFile.SetSize(aPos))!=KErrNone)
		Error(_L("Set EOF 50"),c);
	gFile.Close();
	if ((c=TheFs.Entry(gNameOut,gFileEntry))!=KErrNone)
		Error(_L("File info 501"),c);
	if ((saveTime!=gFileEntry.iModified)!=aShouldChange)
		Error(_L("Change flag check 5"),0);
	}


static void testDir(const TDesC& aDirName)
//
// Create a directory
//
    {

	TInt c;
	test.Printf(_L("Test dir %S\n"),&aDirName);
	c=gDir.Open(TheFs,aDirName,KEntryAttNormal);

	if (c!=KErrNone)
		Error(_L("Directory open 1000"),c);
	
	gDir.Close();
	
	TChar drive(aDirName[0]);
	TInt driveNo;
	c=RFs::CharToDrive(drive,driveNo);
	test_KErrNone(c);
	if ((c=TheFs.Volume(volInfo,driveNo))!=KErrNone)
		Error(_L("Device info 1000"),c);
    }


/*
static void testNodeInfo(const TDesC& aName,TInt type,TInt anErr)
//
// Test p_ninfo.
//
    {

//    TInt c;
	test.Printf(_L("Node info: %S\n"),&aName);
//	if ((c=p_ninfo(aDirName,&nInfo))!=anErr)
//		Error(_L("Device info"),c);
	if (anErr==0)
		{
//		if (!(nInfo.version==2 || nInfo.version==3))
//			Error(_L("Node version check"),0);
//		if (nInfo.type!=type)
//			Error(_L("Node type check"),0);
		}
    }
*/

static void testDeviceInfo(const TDesC& aDeviceName,TInt anErr)
//
// Test p_dinfo.
//
    {

	TInt c;
	test.Printf(_L("Device info: %S\n"),&aDeviceName);
	TInt drive=KDefaultDrive;
	if (aDeviceName.Length())
		{
		c=RFs::CharToDrive(aDeviceName[0],drive);
		test_KErrNone(c);
		}		
	if ((c=TheFs.Volume(volInfo,drive))!=anErr)
		Error(_L("Device info"),c);
	if (anErr==0)
		{
//		if (!(volInfo.version==2 || volInfo.version==3)) ********* version is not a member of TVolumeInfo
//			Error(_L("Device version check"),0);
		if (volInfo.iFree>volInfo.iSize)
			Error(_L("Free greater than size check"),0);
		}
    }

static void testFileInfo(const TDesC& aFileName,TInt anErr)
//
// Test entry info
//
	{

	TInt c;
	test.Printf(_L("File info: %S\n"),&aFileName);
	if ((c=TheFs.Entry(aFileName,gFileEntry))!=anErr)
		Error(_L("Get info 100"),c);
	if (anErr==0)
		{
		if (aFileName.Length()>=2 && aFileName[0]=='Z' && aFileName[1]==':')
			{
			if ((gFileEntry.iAtt&(KEntryAttReadOnly|EFileStream)) != (TUint32)(KEntryAttReadOnly|EFileStream) && gFileEntry.iAtt!=KEntryAttDir)
				Error(_L("Info status check Z:\\"),0);
			}
		else
			{
			if (gFileEntry.iAtt&KEntryAttDir)
				return; // Found directory entry
			if (gFileEntry.iAtt!=(TUint32)(EFileStream|KEntryAttArchive))
				Error(_L("Info status check"),0);
			if (gFileEntry.iSize!=0L)
				Error(_L("Info size check"),0);
			}
		}
	}

static void testRenameFromRoot(const TDesC& aRName,const TDesC& aDName)
//
//
//
	{

	TInt c;
	if ((c=TheFs.Rename(aRName,aDName))!=KErrNone)
		Error(_L("Rename 92"),c);
	if ((c=gFile.Open(TheFs,aDName,EFileStream))!=KErrNone)
		Error(_L("Open 92"),c);
	gFile.Close();
	if ((c=TheFs.Delete(aRName))!=KErrNotFound)
		Error(_L("Delete 92"),c);
	}

static void testRenameToRoot(const TDesC& pName,const TDesC& rName)
//
//
//
	{

	TInt c;
	if ((c=gFile.Replace(TheFs,pName,EFileStream))!=KErrNone)
		Error(_L("Create 91"),c);
	gFile.Close();
	if ((c=TheFs.Rename(pName,rName))!=KErrNone)
		Error(_L("Rename 91"),c);
	if ((c=gFile.Open(TheFs,rName,EFileStream))!=KErrNone)
		Error(_L("Open 91"),c);
	gFile.Close();
	if ((c=TheFs.Delete(pName))!=KErrNotFound)
		Error(_L("Delete 91"),c);
	}

static void verifyTestPat1()
//
//
//
	{

	TInt i;
	TInt c;
	if ((c=gFile.Open(TheFs,_L("\\TESTPAT1.DAT"),EFileStream))!=KErrNone)
		Error(_L("Create testpat1"),c);
	gDataBuf.SetLength(0);
	if ((c=gFile.Read(gDataBuf,512))!=KErrNone || gDataBuf.Length()!=512)
		Error(_L("Read 90"),c);
	gFile.Close();
	for (i=0;i<512;i++)
		{
		if (gDataBuf[i]!='X')
			Error(_L("Data check"),0);
		}
	}

void TestINC103141() // PDEF104017
	{
	test.Printf(_L("Test INC103141\n"));
	_LIT(KDir1, "\\INC103141\\TEST");
	_LIT(KDir2, "\\INC103141\\test");
	TBuf<32> dirname;
	dirname.Copy(KDir1);
	dirname.Append(KPathDelimiter);
	MakeDir(dirname);
	TInt err = TheFs.Rename(KDir1, KDir2);
	test_KErrNone(err);
	err = TheFs.RmDir(dirname);
	test_KErrNone(err);
	err = TheFs.RmDir(KDir1);
	test_KErrNone(err);
	}
	
static void testRename()
//
// Test TheFs.Rename function.
//
	{

	TBuf<32> xName;
	TInt i;
	TInt c;
	test.Printf(_L("Test TheFs.Rename\n"));

	test.Printf(_L("Test rename into root\n"));
	if ((c=gFile.Replace(TheFs,_L("\\TESTPAT1.DAT"),EFileStream|EFileWrite))!=KErrNone)
		Error(_L("Create testpat1"),c);
	gDataBuf.Fill('X',512);
	if ((c=gFile.Write(gDataBuf,512))!=KErrNone)
		Error(_L("Write 90"),c);
	gFile.Close();
	if ((c=TheFs.MkDir(_L("\\DIRX\\")))!=KErrNone)
		Error(_L("Make dir 90"),c);
	for (i=0;i<32;i++)
		{
		xName.Format(_L("\\DIRX\\FILEX%u"),i);
		TPtrC rootName(xName.Ptr()+5,xName.Length()-5);
		testRenameToRoot(xName,rootName);
		verifyTestPat1();
		}

	test.Printf(_L("Test rename from root\n"));
	for (i=0;i<32;i++)
		{
		xName.Format(_L("\\DIRX\\FILEX%u"),i);
		TPtrC rootName(xName.Ptr()+5,xName.Length()-5);
		testRenameFromRoot(rootName,xName);
		verifyTestPat1();
		}
	for (i=0;i<32;i++)
		{
		xName.Format(_L("\\DIRX\\FILEX%u"),i);
		if ((c=TheFs.Delete(xName))!=KErrNone)
			Error(_L("Delete 93"),c);
		}
	if ((c=TheFs.RmDir(_L("\\DIRX\\")))!=KErrNone)
		Error(_L("Delete DIRX"),c);
	verifyTestPat1();
	if ((c=TheFs.Delete(_L("\\TESTPAT1.DAT")))!=KErrNone)
		Error(_L("Delete 80"),c);

	test.Printf(_L("Test rename with wild cards\n"));
	if ((c=TheFs.Rename(_L("*.*"),_L("FRED")))!=KErrBadName)
		Error(_L("Rename 100"),c);
	if ((c=TheFs.Rename(_L("?"),_L("FRED")))!=KErrBadName)
		Error(_L("Rename 101"),c);
	if ((c=TheFs.Rename(_L(""),_L("FRED")))!=KErrBadName) // KErrBadName)
		Error(_L("Rename 101.11"),c);
	if ((c=TheFs.Rename(_L("."),_L("FRED")))!=KErrBadName)
		Error(_L("Rename 101.12"),c);
	if ((c=TheFs.Rename(_L("NOEXIST"),_L("*")))!=KErrBadName)
		Error(_L("Rename 101.1"),c);
	if ((c=gFile.Create(TheFs,_L("FILE1"),EFileStream))!=KErrNone)
		Error(_L("Create 101.2"),c);
	gFile.Close();
	if ((c=TheFs.Rename(_L("FILE1"),_L("AAA?")))!=KErrBadName)
		Error(_L("Rename 101.3"),c);
	if ((c=TheFs.Rename(_L("FILE1"),_L("")))!=KErrBadName) // KErrBadName)
		Error(_L("Rename 101.41"),c);
	if ((c=TheFs.Rename(_L(""),_L("")))!=KErrBadName) // KErrBadName)
		Error(_L("Rename 101.42"),c);
	if ((c=TheFs.Delete(_L("FILE1")))!=KErrNone)
		Error(_L("Delete 101.5"),c);
	if ((c=TheFs.Rename(_L("\\"),_L("FRED")))!=KErrBadName)
		Error(_L("Rename 101.6"),c);

	test.Printf(_L("Test rename of directories\n"));
	if ((c=TheFs.MkDir(_L("\\DIR1\\")))!=KErrNone)
		Error(_L("Make dir 102"),c);
	if ((c=TheFs.Rename(_L("\\DIR1"),_L("\\A2345678.123")))!=KErrNone)
		Error(_L("Rename 103.1"),c);
	if ((c=TheFs.Rename(_L("\\A2345678.123"),_L("\\DIR2")))!=KErrNone)
		Error(_L("Rename 103.2"),c);
	if ((c=TheFs.Rename(_L("\\DIR2"),_L("\\A234567.1234")))!=KErrNone) // ****** KErrBadName) Long filenames are supported
		Error(_L("Rename 103.3"),c);
	if ((c=TheFs.Rename(_L("\\A234567.1234"),_L("\\DIR2")))!=KErrNone)
		Error(_L("Rename 103.3"),c);
	if ((c=TheFs.MkDir(_L("\\DIR1\\")))!=KErrNone)
		Error(_L("Make dir 104"),c);

	test.Printf(_L("Test rename of open files\n"));
	if ((c=gFile.Create(TheFs,_L("\\DIR1\\FILE1"),EFileStreamText))!=KErrNone)
		Error(_L("Create 105"),c);
	if ((c=TheFs.Rename(_L("\\DIR1\\FILE1"),_L("\\DIR1\\FILE1")))!=KErrInUse)
		Error(_L("Rename 106"),c);
	if ((c=TheFs.Rename(_L("\\DIR1\\FILE1"),_L("\\DIR2\\FILE1")))!=KErrInUse)
		Error(_L("Rename 106.1"),c);
	if ((c=gFile2.Open(TheFs,_L("\\DIR2\\FILE1"),EFileStream))!=KErrNotFound)
		Error(_L("Create 105"),c);
	gFile.Close();

	test.Printf(_L("Test rename to same name\n"));
	if ((c=TheFs.Rename(_L("\\DIR1\\FILE1"),_L("\\DIR1\\FILE1")))!=KErrNone) // !=KErrAlreadyExists)
		Error(_L("Rename 105.1"),c);
	if ((c=TheFs.Rename(_L("\\DIR1"),_L("\\DIR1")))!=KErrNone) // !=KErrAlreadyExists)
		Error(_L("Rename 105.2"),c);

	test.Printf(_L("Test rename of read-only files\n"));     // IS ALLOWED //
	if ((c=TheFs.SetAtt(_L("\\DIR1\\FILE1"),KEntryAttReadOnly,0))!=KErrNone)
		Error(_L("Att 106"),c);
	if ((c=TheFs.Entry(_L("\\DIR1\\FILE1"),gFileEntry))!=KErrNone)
		Error(_L("File info 106.1"),c);
	test.Printf(_L("STATUS=%04x\n"),gFileEntry.iAtt);
	if (gFileEntry.iAtt!=(TUint32)(KEntryAttReadOnly|EFileStream|KEntryAttArchive))
		Error(_L("Status check 106.2"),0);
	if ((c=TheFs.Entry(_L("\\DIR1"),gFileEntry))!=KErrNone)
		Error(_L("File info 106.3"),c);
	test.Printf(_L("STATUS=%04x\n"),gFileEntry.iAtt);
	if (gFileEntry.iAtt!=(TUint32)(EFileStream|KEntryAttDir))
		Error(_L("Status check 106.4"),0);

	if ((c=TheFs.Rename(_L("\\DIR1\\FILE1"),_L("\\DIR1\\FILE1")))!=KErrNone) // !=KErrAlreadyExists)
		Error(_L("Rename 107"),c);
	if ((c=TheFs.Rename(_L("\\DIR1\\FILE1"),_L("\\DIR1\\FILE2")))!=KErrNone)
		Error(_L("Rename 108"),c);
	if ((c=TheFs.Rename(_L("\\DIR1\\FILE2"),_L("\\DIR2\\FILE2")))!=KErrNone)
		Error(_L("Rename 109"),c);
	if ((c=gFile2.Open(TheFs,_L("\\DIR2\\FILE2"),EFileStream))!=KErrNone)
		Error(_L("Create 110"),c);
	gFile2.Close();
	if ((c=TheFs.Rename(_L("\\DIR2\\FILE2"),_L("\\DIR1\\FILE1")))!=KErrNone)
		Error(_L("Rename 110.1"),c);
	if ((c=TheFs.SetAtt(_L("\\DIR1\\FILE1"),0,KEntryAttReadOnly))!=KErrNone)
		Error(_L("Att 111"),c);

	test.Printf(_L("Test rename of files across directories\n"));
	if ((c=TheFs.Rename(_L("\\DIR1\\FILE1"),_L("\\DIR2\\FILE1")))!=KErrNone)
		Error(_L("Rename 112"),c);
	if ((c=gFile.Open(TheFs,_L("\\DIR2\\FILE1"),EFileStream))!=KErrNone)
		Error(_L("Exist 113"),c);
	gFile.Close();

	test.Printf(_L("Test rename of directories across directories\n"));
	if ((c=TheFs.Rename(_L("\\DIR1"),_L("\\DIR2\\DIR1")))!=KErrNone) // ******** KErrAccessDenied)
		Error(_L("Rename 114"),c);
	if ((c=TheFs.Rename(_L("\\DIR1"),_L("\\")))!=KErrBadName)
		Error(_L("Rename 114.1"),c);

	if ((c=TheFs.Delete(_L("\\DIR2\\FILE1")))!=KErrNone)
		Error(_L("Delete 115"),c);
	if ((c=TheFs.RmDir(_L("\\DIR2\\DIR1\\")))!=KErrNone)
		Error(_L("Delete 115"),c);
	if ((c=TheFs.RmDir(_L("\\DIR2\\")))!=KErrNone)
		Error(_L("Delete 115"),c);
	
	TestINC103141();  // PDEF104017
	}    

static void testDelete()
//
// Test RFs::Delete function.
//
	{

	TInt c;
	test.Printf(_L("Test RFs::Delete\n"));
	test.Printf(_L("Test delete non-empty directories\n"));
	if ((c=TheFs.MkDir(_L("\\TESTDIR\\")))!=KErrNone)
		Error(_L("Make dir 500"),c);
	if ((c=gFile.Create(TheFs,_L("\\TESTDIR\\NAME.EXT"),EFileStream))!=KErrNone)
		Error(_L("Create"),c);
	if ((c=TheFs.Delete(_L("\\TESTDIR\\")))!=KErrBadName) // ******* KErrAccessDenied)
		Error(_L("Delete 501"),c);

	test.Printf(_L("Test delete open file\n"));
	if ((c=TheFs.Delete(_L("\\TESTDIR\\NAME.EXT")))!=KErrInUse)
		Error(_L("Delete 502"),c);
	gFile.Close();
	if ((c=TheFs.Delete(_L("\\TESTDIR\\NAME.EXT")))!=KErrNone)
		Error(_L("Delete 503"),c);
	if ((c=TheFs.RmDir(_L("\\TESTDIR\\")))!=KErrNone)
		Error(_L("Delete 504"),c);
	}

static void testUnique(TUint fileFormat)
//
// Test RFile::Temp
//
	{

	TInt pos;
	TInt c;
	test.Printf(_L("Test RFile::Temp\n"));
	gDataBuf.SetLength(1);
	if ((c=gFile.Temp(TheFs,_L(""),gNameBuf,fileFormat))!=KErrNone)
		Error(_L("Directory open 2000"),c);
	if ((c=gFile.Write(gDataBuf,0))!=KErrNone)
		Error(_L("Write 2000"),c);
	if ((c=gFile.Write(gDataBuf,1))!=KErrNone)
		Error(_L("Write 2000"),c);
	pos=0L;
	if ((c=gFile.Seek(ESeekStart,pos))!=KErrNone) // !=E_FILE_INV) Temp file is random access
		Error(_L("Seek 2000"),c);
	gFile.Close();
	if ((c=TheFs.Delete(gNameBuf))!=KErrNone)
		Error(_L("Delete"),c);
	}

static void testFileName(const TDesC& aFileName,TInt res)
//
//
//
	{

	TInt c;
	if ((c=aFileName.Length())>20)
		test.Printf(_L("%u char name\n"),c);
	else
		test.Printf(_L("\"%S\"\n"),&aFileName);
	if ((c=gFile.Create(TheFs,aFileName,EFileStream))!=res)
		Error(_L("Create 200"),c);
	if (res==KErrNone)
		{
		gFile.Close();
		if ((c=gFile.Open(TheFs,aFileName,EFileStream))!=KErrNone)
			Error(_L("Open 200"),c);
		gFile.Close();
		if ((c=TheFs.Delete(aFileName))!=KErrNone)
			Error(_L("Delete 200"),c);
		}
	else
		{
		if ((c=gFile.Open(TheFs,aFileName,EFileStream))!=res)    // test eg *.* //
			Error(_L("Open 201"),c);
		if ((c=gFile.Replace(TheFs,aFileName,EFileStream))!=res)
			Error(_L("Replace 202"),c);
		}
	}

static void testFileName(const TDesC8& aFileName,TInt res)
//
// Defined to cope with all the instances of testFileName(gDataBuf,...)
//
	{
	TPtrC gDataBuf16((TText*)aFileName.Ptr(),gDataBuf.Size()/sizeof(TText8));
	testFileName(gDataBuf16,res);
	}

static void testVolumeName(const TDesC& aVolumeName,TInt aResultExpected)
//
//
//
	{

	TInt result;
	test.Printf(_L("\"%S\"\n"),&aVolumeName);
	TInt drive=KDefaultDrive;
	if (aVolumeName.Length()>=2 && aVolumeName[0]=='Z' && aVolumeName[1]==':')
		{
		drive=25;
		TPtr volName((TText*)&aVolumeName[2],(aVolumeName.Length()-2),(aVolumeName.Length()-2));
		result=TheFs.SetVolumeLabel(volName,drive);
		}
	else
		result=TheFs.SetVolumeLabel(aVolumeName,drive);
	
	if (result==KErrGeneral)
		{
		test.Printf(_L("KErrGeneral: Cannot set volume label on a substed drive!\n"));
		return;
		}
	if (result!=aResultExpected)
		Error(_L("Set volume name returned"),result);
	
/*	if (aResultExpected==FALSE)
		{
		if ((result=TheFs.Volume(volInfo))!=KErrNone)
			Error(_L("Volume Info failed"),result);
		TPtrC vol(aVolumeName);
		if (vol!=volInfo.iName)
			Error(_L("Check volume name failed"),0);
		}
*/
	}

#define testMakeDir(aDirName, res) TestMakeDirLine(aDirName, res, __LINE__)
static void TestMakeDirLine(const TDesC& aDirName,TInt res, TInt line)
//
//
//
	{

    TInt c;
    TUint l;
    TFileName buf;
    if ((c=aDirName.Length())>20)
        test.Printf(_L("%u char name\n"),c);
    else
        test.Printf(_L("\"%S\"\n"),&aDirName);
    if ((c=TheFs.MkDirAll(aDirName))!=res)
        Error2(_L("Make directory 1"),c, line);
    if (res==FALSE)
		{
        TParse parse;
        c=TheFs.Parse(aDirName,parse);
        test_KErrNone(c);
        buf=parse.Path();
        buf.Append(_L("*"));
        if (buf.Length()<=64)
			{
            test.Printf(_L("Open dir...\n"));
            if ((c=gDir.Open(TheFs,buf,KEntryAttMaskSupported))!=KErrNone)
                Error2(_L("Directory open 1"),c, line);
            gDataBuf[0]=0;
            if ((c=gDir.Read(gFileEntry))!=KErrEof)
            	{
                test.Printf(_L("buf=\"%S\"\n"),&gFileEntry.iName);
                Error2(_L("Directory read"),c, line);
            	}
            gDir.Close();
        	}
        buf=aDirName;
        l=buf.Length();
        FOREVER
			{
            test.Printf(_L("Delete \"%S\"\n"),&buf);
            if ((c=TheFs.RmDir(buf))!=KErrNone)
                Error2(_L("Delete directory"),c, line);
            while (--l)
				{
                if (buf[l]=='\\')
					{
                    buf.SetLength(l);
                    if (buf.LocateReverse('\\')==0)
                        return;
                    break;
                	}
            	}
            if (l == 0)
                break;
        	}
    	}
	}


static void TestMakeDirLine(const TDesC8& aDirName, TInt res, TInt line)
//
// Defined to cope with all the instances of testMakeDir(gDataBuf,...)
//
{
    
    TPtrC gDataBuf16((TText*)aDirName.Ptr(),gDataBuf.Size()/sizeof(TText8));
    //	Not sizeof(TText16) since gDataBuf is a TBuf*!	
    TestMakeDirLine(gDataBuf16, res, line);
}


#ifdef  TEST_MEDIA
static void testMedia(const TDesC& instructions,TInt anErr)
//
//
//
	{

	TBuf<0x40> errBuf;
	TInt openErr;
	TInt c;
//	p_errs(&errBuf[0],anErr);
	pathBuf=fBuf;
	pathBuf.SetLength(7);
	test.Printf(_L("Test %S\n"),&errBuf);
	test.Printf(_L("%S %S\n"),&instructions,&pathBuf);
	p_getch();
	openErr=anErr;
	if (anErr==WriteProtectErr)
		openErr=0;
	if ((c=gFile.Open(TheFs,fBuf,P_FDIR))!=openErr)
		Error(_L("Directory open 100"),c);
	if (openErr==KErrNone)
		{
		if (c=gFile.Close())
			Error(_L("Close 100"),c);
		}
	gNameBuf.SetLength(0);
	if ((c=gFile.Open(TheFs,gNameBuf,P_FUNIQUE|EFileStream))!=anErr)
		Error(_L("Create unique 100"),c);
	if (anErr==0)
		{
		if (c=gFile.Close())
			Error(_L("Close 100"),c);
		}
	}
#endif

void CallTestsL()
    {

    //-- set up console output
    F32_Test_Utils::SetConsole(test.Console());

    TInt nRes=TheFs.CharToDrive(gDriveToTest, gDriveNum);
    test_KErrNone(nRes);
    
    PrintDrvInfo(TheFs, gDriveNum);

    //-- quick format the drive, if it isn't drive C: of the emulator or PlatSim
    if(!Is_SimulatedSystemDrive(TheFs, gDriveNum))
    	{
        nRes = FormatDrive(TheFs, gDriveNum, ETrue); 
        test_KErrNone(nRes);
    	}

    //-----------------------------------
	TInt c;
	TInt i,count;
	TInt pos;
	TInt64 seed;
	TInt attrib,mask;
	TTime saveTime;
	TInt testSize;

	CreateTestDirectory(_L("\\F32-TST\\BGEN\\"));
//	if (p_date()<MINIMUM_DATE)
//	p_sdate(MINIMUM_DATE);

	seed=(TInt64)1732;
	fBuf=gSessionPath;
	pathBuf=fBuf;
	pathBuf.Append('*');
	testDir(pathBuf);
//	testDir(_L("Z:")); // Session Path gets added -> KErrPathNotFound
//	testDir(_L("Z:*"));  // Session Path gets added -> KErrPathNotFound
//	testDir(_L("Z:*.*")); // Session Path gets added -> KErrPathNotFound
	testDir(_L("Z:\\"));
	testDir(_L("Z:\\*"));
	testDir(_L("Z:\\*.*"));

	test.Printf(_L("Test names containing '\\'\n"));
	if ((c=gFile.Create(TheFs,_L("Q\\ZZZ"),EFileWrite))!=KErrBadName)
		Error(_L("Create 1"),c);

	test.Printf(_L("Test create in non-exist directory\n"));
	if ((c=gFile.Create(TheFs,_L("\\Q1DDX\\ZZZ"),EFileWrite))!=KErrPathNotFound)
		Error(_L("Create 2"),c);

	test.Printf(_L("Test filenames starting with '.'\n"));
	if ((c=gFile.Create(TheFs,_L("\\.ZZZ"),EFileWrite))!=KErrNone) // ****** KErrBadName)
		Error(_L("Create 3"),c);
	gFile.Close();
	if ((c=TheFs.Delete(_L("\\.ZZZ")))!=KErrNone)
		Error(_L("Delete 3"),c);

	test.Printf(_L("Test filenames starting with 05/E5\n"));
	gNameBuf.SetLength(5);
	gNameBuf[0]=0xE5;
	gNameBuf[1]='X';
	gNameBuf[2]='X';
	gNameBuf[3]='X';
	gNameBuf[4]=0x00;
	gNameBuf.SetLength(4);
	if ((c=gFile.Replace(TheFs,gNameBuf,EFileWrite))!=KErrNone)
		Error(_L("Replace 4"),c);
	gFile.Close();
	if ((c=gFile.Open(TheFs,gNameBuf,EFileRead))!=KErrNone)
		Error(_L("Open 4"),c);
	gFile.Close();

// *************** Silly filename stuff rightly disallowed by NT
//	gNameBuf.SetLength(5);
//	gNameBuf[0]=0x05;
//	gNameBuf[1]='Y';
//	gNameBuf[2]='Y';
//	gNameBuf[3]='Y';
//	gNameBuf[4]=0x00; // Zero terminator gets lost by VFAT
//	if ((c=gFile.Replace(TheFs,gNameBuf,EFileWrite))!=KErrNone)
//		Error(_L("Create 5"),c);
//	gFile.Close();
//	gNameBuf[0]=0xE5;
//	if ((c=gFile.Open(TheFs,gNameBuf,EFileRead))!=KErrNone)
//		Error(_L("Open 5"),c);
//	gFile.Close();
//

//
//#ifdef  TEST_MEDIA
//	if (fBuf[5]!='B')
//		goto skipMediaTest;
//	testMedia(_L("OPEN DOOR ON"),NotReadyErr);
//	testMedia(_L("INSERT CORRUPT RAM PACK (WRITE ENABLED) INTO"),CorruptMediaErr);
//	testMedia(_L("INSERT CORRUPT RAM PACK (WRITE PROTECTED) INTO"),CorruptMediaErr);
//	testMedia(_L("INSERT CORRUPT FLASH PACK (WRITE ENABLED) INTO"),CorruptMediaErr);
//	testMedia(_L("INSERT CORRUPT FLASH PACK (WRITE PROTECTED) INTO"),CorruptMediaErr);
//#ifdef FULL_TEST
//	testMedia(_L("INSERT UNKNOWN MEDIA INTO"),UnknownErr);
//#endif
//	testMedia(_L("INSERT GOOD RAM PACK (WRITE PROTECTED) INTO"),WriteProtectErr);
//	if ((c=gFile.Open(TheFs,fBuf,P_FFORMAT))!=WriteProtectErr)
//		Error(_L("Format RAM write protect"),c);
//	testMedia(_L("INSERT GOOD FLASH PACK (WRITE PROTECTED) INTO"),WriteProtectErr);
//	if ((c=gFile.Open(TheFs,fBuf,P_FFORMAT))!=WriteProtectErr)
//		Error(_L("Format FLASH write protect"),c);
//	testMedia(_L("INSERT GOOD PACK (WRITE ENABLED) INTO"),0);
//
//skipMediaTest:
//#endif // TEST_MEDIA //
//

// Test update is assumed for unique //

	testUnique(EFileStream);
	testUnique(EFileStreamText);
	testDelete();
	testRename();
	test.Printf(_L("Test get file info\n"));
//	testFileInfo(_L("*.*"),KErrBadName); ********** Allowed (?)
	testFileInfo(_L(""),KErrNone); // KErrBadName);
	testFileInfo(_L("\\"),KErrBadName);
	testFileInfo(_L("."),KErrBadName);
	testFileInfo(_L(".."),KErrBadName);
	testFileInfo(_L("a.1234"),KErrNotFound); // ********* KErrBadName);
	testFileInfo(_L("a23456789"),KErrNotFound); // ********* KErrBadName);
	testFileInfo(_L(".a"),KErrNotFound); // ********** KErrBadName);
	testFileInfo(_L("?"),KErrBadName);
	testFileInfo(_L("NOEXIST"),KErrNotFound);
	testFileInfo(_L("\\NODIR\\NAME"),KErrPathNotFound);
	testFileInfo(_L("L:\\NAME"),KErrNotReady);
	gNameBuf.SetLength(0);
	if ((c=gFile.Temp(TheFs,gNameBuf,gNameOut,EFileStream))!=KErrNone)
		Error(_L("Open 1.1"),c);
	testFileInfo(gNameOut,FALSE);       // Not locked //
	gFile.Close();
	testFileInfo(gNameOut,FALSE);
	if ((c=TheFs.Delete(gNameOut))!=KErrNone)
		Error(_L("Delete"),c);

	test.Printf(_L("Test get device info\n"));
	testDeviceInfo(_L(""),FALSE);           // NULL is current device //
	testDeviceInfo(_L("L:"),KErrNotReady);
	testDeviceInfo(_L("Z:"),FALSE);
	testDeviceInfo(fBuf,FALSE);
	

//	test.Printf(_L("Test get node info\n"));
//	testNodeInfo(_L("LOC::"),P_FSYSTYPE_HIER,FALSE);
//	testNodeInfo(_L("Z:"),P_FSYSTYPE_FLAT,FALSE);
//	testNodeInfo(_L("LOC:"),P_FSYSTYPE_HIER,FALSE);
//	testNodeInfo(_L("LOC"),P_FSYSTYPE_HIER,FALSE);
//	testNodeInfo(_L("*"),P_FSYSTYPE_HIER,FALSE);
//	testNodeInfo(_L(""),P_FSYSTYPE_HIER,FALSE);
//	testNodeInfo(_L("?"),P_FSYSTYPE_HIER,FALSE);
//	testNodeInfo(_L("FRED"),P_FSYSTYPE_HIER,FALSE);
//	testNodeInfo(_L("FRED::"),P_FSYSTYPE_HIER,FALSE);
//	testNodeInfo(_L("....."),P_FSYSTYPE_HIER,FALSE);
//	testNodeInfo(_L("LOC::zzzzzzzzzzzzzzzzzzzzzzzzz"),P_FSYSTYPE_HIER,FALSE);
//	testNodeInfo(_L("LOC::\\"),P_FSYSTYPE_HIER,FALSE);
//	testNodeInfo(_L("XXX::"),0,E_GEN_FSYS);
//	testNodeInfo(_L("REM::"),0,E_GEN_FSYS);
//	testNodeInfo(_L("...::"),0,E_GEN_FSYS);

//	testFileInfo(_L("Z:SYS$WSRV.IMG"),FALSE);
//	testFileInfo(_L("Z:\\SYS$WSRV.IMG"),KErrBadName);    // \ not allowed  - no path //
//	testFileInfo(_L("Z:*"),FALSE);               // Z: allows *'s ! //
//	testFileInfo(_L("Z:SYS$WSRV."),KErrNotFound);
//	testFileInfo(_L("XXX::"),NoFileSystemErr);
	if(PlatSec::ConfigSetting(PlatSec::EPlatSecEnforceSysBin))
		testFileInfo(_L("Z:\\Sys\\Bin\\ESHELL.EXE"),FALSE);    // we now have paths //
	else
		testFileInfo(_L("Z:\\System\\Bin\\ESHELL.EXE"),FALSE);    // we now have paths //
//	testFileInfo(_L("Z:*"),KErrPathNotFound); // session path gets inserted ,FALSE);
//	testFileInfo(_L("Z:SYS$WSRV."),KErrNotFound);
//	testFileInfo(_L("H:"),KErrBadName); // ************** NoFileSystemErr);

	test.Printf(_L("Test weird filenames\n"));
// Test SPACES // 

	testFileName(_L("A B"),KErrNone); // ******* KErrBadName);
	testFileName(_L(" AB"),KErrNone); // ******* KErrBadName);
	testFileName(_L(" AB      "),KErrNone); // ******* KErrBadName);
	testFileName(_L("    AB"),KErrNone);
	testFileName(_L(" AB  . cdef"),KErrNone);
	testFileName(_L(" AB  .  cdef  "),KErrNone);
	testFileName(_L("A2345678 "),KErrNone); // ******* KErrBadName);
	testFileName(_L("A2345678.XY "),KErrNone); // ******* KErrBadName);
	testFileName(_L("A2345678.XYZ "),KErrNone); // ******* KErrBadName);
	testFileName(_L("A2345678 XYZ"),KErrNone); // ******* KErrBadName);
	testFileName(_L(" "),KErrBadName);
	testFileName(_L("\\A B\\NAME"),KErrPathNotFound); // ******* KErrBadName);
	testFileName(_L("\\ \\NAME"),KErrBadName);
	testFileName(_L("\\asdf\\qer\\   \\asdf\\NAME"),KErrBadName);
	testFileName(_L("     "),KErrBadName);
	testFileName(_L("C:\\asdf\\     "),KErrBadName);
// Test short names //
	testFileName(_L(""),KErrBadName);
	testFileName(_L("\\"),KErrBadName);
	testFileName(_L("1"),FALSE);
	testFileName(_L(".1"),KErrNone); // ******* KErrBadName);
	testFileName(_L(".1"),KErrNone); // ******* KErrBadName);
	testFileName(_L("\\.1"),KErrNone); // ******* KErrBadName);
	testFileName(_L("1.1"),FALSE);
// Test long names //
	testFileName(_L("12345678.123"),FALSE);
	testFileName(_L("123456789.123"),KErrNone); // ******* KErrBadName);
	testFileName(_L("12345678.1234"),KErrNone); // ******* KErrBadName);
	testFileName(_L("1.1234"),KErrNone); // ******* KErrBadName);
	testFileName(_L("123456789"),KErrNone); // ******* KErrBadName);
	gDataBuf.SetLength(256);
	gDataBuf.Fill('A',255);
	testFileName(gDataBuf,KErrBadName);
	gDataBuf.SetLength(257);
	gDataBuf.Fill('B',256);
	testFileName(gDataBuf,KErrBadName);
	gDataBuf.SetLength(258);
	gDataBuf.Fill('C',257);
	testFileName(gDataBuf,KErrBadName);
	gDataBuf.SetLength(4096);
	gDataBuf.Fill('D',4095);
	testFileName(gDataBuf,KErrBadName);
// Test DOTS //
	testFileName(_L("A.X"),FALSE);
	testFileName(_L("A..X"),KErrNone); // ******* KErrBadName);
	testFileName(_L("A.........X"),KErrNone); // ******* KErrBadName);
	testFileName(_L("A."),FALSE);
	testFileName(_L(".X"),KErrNone); // ******* KErrBadName);
	testFileName(_L("."),KErrBadName);
	testFileName(_L(".."),KErrBadName);
//	testFileName(_L("..."),KErrNone); // KErrBadName); // !!! ********* NT error KErrAccessDenied (?)
	testFileName(_L("\\a.x\\NAME"),KErrPathNotFound); // KErrPathNotFound == KErrPathNotFound
	testFileName(_L("\\a..x\\NAME"),KErrPathNotFound); // ******** KErrBadName);
	testFileName(_L("\\.\\NAME"),KErrBadName);
	testFileName(_L("\\..\\NAME"),KErrBadName);
//	testFileName(_L("\\...\\NAME"),KErrPathNotFound); // ******** KErrBadName); // !! NT treats ... as .. ??
// Test WILD CARDS //
	testFileName(_L("*.*"),KErrBadName);
	testFileName(_L("*"),KErrBadName);
	testFileName(_L("\\*"),KErrBadName);
	testFileName(_L("?"),KErrBadName);
	testFileName(_L("\\?"),KErrBadName);
	testFileName(_L("\\A?B\\NAME"),KErrBadName);
	testFileName(_L("\\A*B\\NAME"),KErrBadName);
	testFileName(_L("\\*\\NAME"),KErrBadName);
	testFileName(_L("\\********.***\\NAME"),KErrBadName);
	testFileName(_L("A?X"),KErrBadName);

	test.Printf(_L("Test set volume name\n"));
// New behaviour: SetVolumeName accepts any string < 12 chars
// No modifications are made on the string.
#if defined(__WINS__)
	TInt ret=TheFs.SetVolumeLabel(_L("TEST"),KDefaultDrive);
	if(ret==KErrNotSupported||ret==KErrAccessDenied)
		{
		test.Printf(_L("Error: Cannot set volume label on substed drive\n"));
		//test.Getch();
		}
	else
#endif
	{
	testVolumeName(_L("TESTNAME.VOL"),KErrBadName);	//	12 chars - too long for volume label
	testVolumeName(_L("TESTNAME"),FALSE);			//	OK for 8 bit - too long for UNICODE
	testVolumeName(_L("PQRSTUVWXYZ"),FALSE);		//	just uses the first 5 characters
	testVolumeName(_L("ABCDE"),FALSE);
	testVolumeName(_L("FGHIJK"),FALSE);
	testVolumeName(_L(""),FALSE);
	testVolumeName(_L(""),FALSE);
	testVolumeName(_L("\\"),FALSE);
	gNameBuf.SetLength(0);
	if ((c=gFile.Temp(TheFs,gNameBuf,gNameOut,EFileStream))!=KErrNone)
		Error(_L("Open 60"),c);
	testVolumeName(_L("TEST_NAME"),FALSE);	//	Check not locked 
	gFile.Close();
	if ((c=TheFs.Delete(gNameOut))!=KErrNone)
		Error(_L("Delete"),c);
	testVolumeName(_L("voL1"),FALSE);
	testVolumeName(_L("\\vol1"),FALSE);
	testVolumeName(_L("\\12345678.123"),KErrBadName);
	testVolumeName(_L("\\123456.123"),FALSE);
	testVolumeName(_L("\\vol1\\"),KErrNone); 
	testVolumeName(_L("."),KErrBadName);	//	Bug fix SW1-728 to prevent illegal characters
	testVolumeName(_L(".."),KErrBadName);	//	in the volume name
	testVolumeName(_L("A."),KErrBadName);
	if (!IsTestingLFFS())
		{ // ???
		testVolumeName(_L("!\"\x9C$%^&@.(){"),KErrBadName);
		testVolumeName(_L("!\"\x9C$%^&@("),KErrBadName);
		}
	testVolumeName(_L("*.*"),KErrBadName);	// Wild cards not allowed
	testVolumeName(_L("?.?"),KErrBadName); 
	testVolumeName(_L("????????.???"),KErrBadName);
	testVolumeName(_L("????????.??"),KErrBadName);
	testVolumeName(_L("ABC>DEF"),KErrBadName);
	testVolumeName(_L("ABC<DEF"),KErrBadName);
	testVolumeName(_L("ABC|DEF"),KErrBadName);
	testVolumeName(_L("ABC/DEF"),KErrBadName);
	testVolumeName(_L("ABC\"DEF"),KErrBadName);
	testVolumeName(_L("ABC*DEF"),KErrBadName);
	testVolumeName(_L("ABC:DEF"),KErrBadName);
	testVolumeName(_L("ABC?DEF"),KErrBadName);
	testVolumeName(_L("ABC\\DEF"),KErrBadName);
	testVolumeName(_L("ABCDEFGHIJKLMNOPQRSTUVWXYZ"),KErrBadName);	//	Too long
	testVolumeName(_L("VOLUME1"),FALSE);							//	Too long in UNICODE
	testVolumeName(_L("Z:VOLUME1"),KErrAccessDenied);
	}
		
	test.Printf(_L("Test make directory\n"));

// Test path 
	testMakeDir(_L("\\A2345678.A23\\NAME"),FALSE);
	testMakeDir(_L("\\A23456789.A23\\NAME"),KErrNone); // ******** KErrBadName);
	testMakeDir(_L("\\A2345678.A234\\NAME"),KErrNone); // ******** KErrBadName);
	testMakeDir(_L("\\A.1234\\NAME"),KErrNone); // ********* KErrBadName);
	testMakeDir(_L("\\A2345678\\NAME"),FALSE);
	testMakeDir(_L("\\A23456789\\NAME"),KErrNone); // ******** KErrBadName);
	testMakeDir(_L("\\A.X\\NAME"),FALSE);
	testMakeDir(_L("\\A..X\\NAME"),KErrNone); // ******** KErrBadName);
	testMakeDir(_L("\\A.\\NAME"),KErrBadName);
	testMakeDir(_L("\\.X\\NAME"),KErrNone); // ******** KErrBadName);
	testMakeDir(_L("\\.\\NAME"),KErrBadName);
	testMakeDir(_L("\\..\\NAME"),KErrBadName);
	testMakeDir(_L("\\\\NAME"),KErrBadName);
	testMakeDir(_L("\\\\"),KErrBadName);
	testMakeDir(_L("\\A\\A2\\A23\\a2345678\\a2345678.\\a2345678.1\\a2345678.123"),KErrBadName);
	testMakeDir(_L("\\A\\A2\\A23\\a2345678\\a2345678.\\a2345678.1\\a2345678..123"),KErrBadName); // ******* KErrBadName);
	testMakeDir(_L("\\A\\A2\\A23\\a2345678\\a2345678.\\a2345678.1\\a2345678.1234"),KErrBadName); // ******* KErrBadName);
	gDataBuf.SetLength(256);
	gDataBuf.Fill('V',255);
	testMakeDir(gDataBuf,KErrBadName);
	gDataBuf.SetLength(257);
	gDataBuf.Fill('W',256);
	testMakeDir(gDataBuf,KErrBadName);
	gDataBuf.SetLength(258);
	gDataBuf.Fill('X',257);
	testMakeDir(gDataBuf,KErrBadName);
	gDataBuf.SetLength(259);
	gDataBuf.Fill('Y',258);
	testMakeDir(gDataBuf,KErrBadName);
	gDataBuf.SetLength(4096);
	gDataBuf.Fill('Z',4095);
	testMakeDir(gDataBuf,KErrBadName);

// Test names 
	testMakeDir(_L("A..X"),KErrAlreadyExists); // ******* KErrBadName);
	testMakeDir(_L("\\A\\"),FALSE);
	testMakeDir(_L("\\12345678.123\\"),FALSE);
	testMakeDir(_L("\\.\\"),KErrBadName);
	testMakeDir(_L("\\..\\"),KErrBadName);
	testMakeDir(_L("\\X\\"),FALSE);
	testMakeDir(_L("\\12345678.1234\\"),KErrNone); // ******* KErrBadName);
	testMakeDir(_L("\\123456789\\"),KErrNone); // ******** KErrBadName);
// Test max levels
	testMakeDir(_L("\\A\\B\\C\\D\\E\\F\\G\\H\\I\\J\\K\\L\\M\\N\\O\\P\\Q\\R\\S\\T\\U\\V\\W\\X\\Y\\Z"),FALSE);
	testMakeDir(_L("\\00000000.000\\11111111.111\\22222222.222\\33333333.333\\45678901.3"),FALSE);
	testMakeDir(_L("\\00000000.000\\11111111.111\\22222222.222\\33333333.333\\45678901.34"),FALSE);
	testMakeDir(_L("\\00000000.000\\11111111.111\\22222222.222\\33333333.333\\45678901.345"),FALSE);
	testMakeDir(_L("\\00000000.000\\11111111.111\\22222222.222\\33333333.333\\45678901.3\\xxxxxxxx.xxx"),FALSE);
	testMakeDir(_L("\\00000000.000\\11111111.111\\22222222.222\\33333333.333\\45678901.34\\xxxxxxxx.xxx"),KErrNone); // ******* KErrBadName);
	testMakeDir(_L("\\00000000.000\\11111111.111\\22222222.222\\33333333.333\\45678901.345\\xxxxxxxx.xxx"),KErrNone); // ******* KErrBadName);
	testMakeDir(_L("\\2\\4\\6\\8\\0\\2\\4\\6\\8\\0\\2\\4\\6\\8\\0\\2\\4\\6\\8\\0\\2\\4\\6\\8\\0\\2\\4\\6\\8\\0\\23"),FALSE);
	testMakeDir(_L("\\2\\4\\6\\8\\0\\2\\4\\6\\8\\0\\2\\4\\6\\8\\0\\2\\4\\6\\8\\0\\2\\4\\6\\8\\0\\2\\4\\6\\8\\0\\23456789.123"),FALSE);
	testMakeDir(_L("\\2\\4\\6\\8\\0\\2\\4\\6\\8\\0\\2\\4\\6\\8\\0\\2\\4\\6\\8\\0\\2\\4\\6\\8\\0\\2\\4\\6\\8\\0\\23\\5"),FALSE);
	testMakeDir(_L("\\2\\4\\6\\8\\0\\2\\4\\6\\8\\0\\2\\4\\6\\8\\0\\2\\4\\6\\8\\0\\2\\4\\6\\8\\0\\2\\4\\6\\8\\0\\23\\56789012.456"),FALSE);
	testMakeDir(_L("\\2\\4\\6\\8\\0\\2\\4\\6\\8\\0\\2\\4\\6\\8\\0\\2\\4\\6\\8\\0\\2\\4\\6\\8\\0\\2\\4\\6\\8\\0\\234\\6"),KErrNone); // ******** KErrBadName);
	testMakeDir(_L("Z:\\ROMDIR\\"),KErrAccessDenied); //
	test.Printf(_L("Test setEof to same length\n"));
	gNameBuf.SetLength(0);
	if ((c=gFile.Temp(TheFs,gNameBuf,gNameOut,EFileStream|EFileWrite))!=KErrNone)
		Error(_L("Open 50"),c);
	gFile.Close();
	testSetEof(gNameOut,0L,FALSE);  // should be no change //
	testSetEof(gNameOut,1L,TRUE);   // should be change //
	testSetEof(gNameOut,1L,FALSE);  // should be no change //
	testSetEof(gNameOut,1L,FALSE);
	if (fBuf[5]=='M')
		testSize=650L;                  // No room on M: for 65536 ! //
	else
		testSize=65536L;
	testSetEof(gNameOut,testSize,TRUE);
	testSetEof(gNameOut,testSize,FALSE);
	testSetEof(gNameOut,testSize+1L,TRUE);
	testSetEof(gNameOut,testSize,TRUE);

	testSetEof(gNameOut,0L,TRUE);
	testSetEof(gNameOut,0L,FALSE);
	if ((c=TheFs.Delete(gNameOut))!=KErrNone)
		Error(_L("Delete"),c);

	test.Printf(_L("Test read of zero bytes\n"));
	gNameBuf.SetLength(0);

	if ((c=gFile.Temp(TheFs,gNameBuf,gNameOut,EFileStream|EFileWrite))!=KErrNone)
		Error(_L("Open 60"),c);
	if ((c=gFile.Read(gDataBuf,0))!=KErrNone) // ******** KErrEof)
		Error(_L("Read 61"),c);
	if ((c=gFile.Read(gDataBuf,0))!=KErrNone) // ******** KErrEof)
		Error(_L("Read 62"),c);
	if ((c=gFile.Write(gDataBuf,0))!=KErrNone)
		Error(_L("Write 63"),c);
	if ((c=gFile.Read(gDataBuf,0))!=KErrNone) // ******** KErrEof)
		Error(_L("Read 64"),c);
	gFile.Close();
	if ((c=gFile.Open(TheFs,gNameOut,EFileStream|EFileWrite))!=KErrNone)
		Error(_L("Open 70"),c);
	if ((c=gFile.Read(gDataBuf,0))!=KErrNone) // ******** KErrEof)
		Error(_L("Read 71"),c);
	gDataBuf.SetLength(1);
	gDataBuf[0]=0xf0;
	if ((c=gFile.Write(gDataBuf,1))!=KErrNone)
		Error(_L("Write 72"),c);
	if ((c=gFile.Read(gDataBuf,0))!=KErrNone) // ********* KErrEof)
		Error(_L("Read 73"),c);
	pos=0L;
	if ((c=gFile.Seek((TSeek)ESeekStart,pos))!=KErrNone)
		Error(_L("Seek 74"),c);
	gDataBuf.SetLength(1);
	gDataBuf[0]=0x83;
	if ((c=gFile.Read(gDataBuf,0))!=KErrNone)
		Error(_L("Read 75"),c);
//	if (gDataBuf[0]!=0x83) *********** Read zeros the length of a buffer after a zero length read
	if (gDataBuf.Length()!=0)
		Error(_L("buffer 1 check"),0);
	if ((c=gFile.Read(gDataBuf,0))!=KErrNone)
		Error(_L("Read 76"),c);
	if ((c=gFile.Read(gDataBuf,1))!=KErrNone || gDataBuf.Length()!=1)
		Error(_L("Read 77"),c);
	if (gDataBuf[0]!=0xf0)
		Error(_L("buffer 1 check"),0);
	if ((c=gFile.Read(gDataBuf,0))!=KErrNone) // ******** KErrEof)
		Error(_L("Read 78"),c);
	if ((c=gFile.Read(gDataBuf,16384))!=KErrNone) // ******* KErrEof)
		Error(_L("Read 79"),c);
	gFile.Close();
	if ((c=TheFs.Delete(gNameOut))!=KErrNone)
		Error(_L("Delete"),c);

    test.Printf(_L("Test write of zero bytes\n"));
    gNameBuf.SetLength(0);
    if ((c=gFile.Temp(TheFs,gNameBuf,gNameOut,EFileStream|EFileWrite))!=KErrNone)
        Error(_L("Open 50"),c);
    gFile.Close();
//	********** Error(_L("Close"),c); close has no return value
	gDataBuf.SetLength(16384);

    testWrite(gNameOut,0,0L,FALSE); // should be no change //
    testWrite(gNameOut,1,1L,TRUE);  // should be change //
    testWrite(gNameOut,0,1L,FALSE);
    testWrite(gNameOut,0,1L,FALSE);
    testWrite(gNameOut,16384,16384L,TRUE);
    testWrite(gNameOut,0,16384L,FALSE);
    testWrite(gNameOut,16383,16384L,TRUE);


    if ((c=TheFs.Delete(gNameOut))!=KErrNone)
        Error(_L("Delete"),c);

	test.Printf(_L("Test ReadOnly files\n"));
	gNameBuf.SetLength(0);
	if ((c=gFile.Create(TheFs,_L("TEST1.TMP"),EFileStream|EFileWrite))!=KErrNone)
		Error(_L("Create 40"),c);
	gFile.Close();
	if ((c=gFile.Temp(TheFs,gNameBuf,gNameOut,EFileStream|EFileWrite))!=KErrNone)
		Error(_L("Open 40"),c);
	mask=0;
	attrib=KEntryAttReadOnly;                       // Remove writable //
	if ((c=TheFs.SetAtt(gNameOut,attrib,mask))!=KErrInUse)
		Error(_L("TheFs.SetAtt not locked"),c);
	gFile.Close();
	if ((c=TheFs.SetAtt(gNameOut,attrib,mask))!=KErrNone)
		Error(_L("Att 41"),c);
	if ((c=gFile.Open(TheFs,gNameOut,EFileStream|EFileWrite))!=KErrAccessDenied)
		Error(_L("Open 41"),c);
	if ((c=gFile.Open(TheFs,gNameOut,EFileStream))!=KErrNone)
		Error(_L("Open 42"),c);
	if ((c=gFileErr.Open(TheFs,gNameOut,EFileStream))!=KErrInUse)
		Error(_L("Open 43"),c);
	if ((c=TheFs.Rename(_L("TEST1.TMP"),gNameOut))!=KErrAlreadyExists)
		Error(_L("Rename 43.1"),c);
	if ((c=gFileErr.Create(TheFs,gNameOut,EFileStream))!=KErrAlreadyExists) // KErrInUse)
		Error(_L("Open 44"),c);
	if ((c=gFileErr.Replace(TheFs,gNameOut,EFileStream))!=KErrInUse)
		Error(_L("Open 45"),c);
	gFile.Close();
	if ((c=gFile.Create(TheFs,gNameOut,EFileStream))!=KErrAlreadyExists)
		Error(_L("Create 46"),c);
	if ((c=gFile.Replace(TheFs,gNameOut,EFileStream))!=KErrAccessDenied)
		Error(_L("Replace 47"),c);
	if ((c=gFile.Create(TheFs,_L("FILE1.TMP"),EFileStream))!=KErrNone)
		Error(_L("Create 48"),c);
	if ((c=TheFs.Rename(_L("FILE1.TMP"),_L("FILE2.TMP")))!=KErrInUse)
		Error(_L("Rename 49"),c);
	gFile.Close();
	if ((c=TheFs.Rename(_L("FILE1.TMP"),_L("FILE2.TMP")))!=KErrNone)
		Error(_L("Rename 50"),c);
	if ((c=TheFs.Rename(_L("FILE2.TMP"),gNameOut))!=KErrAlreadyExists)
		Error(_L("Rename 51"),c);
	if ((c=TheFs.Delete(gNameOut))!=KErrAccessDenied)
		Error(_L("Delete"),c);
	mask=KEntryAttReadOnly;
	attrib=0;
	if ((c=TheFs.SetAtt(gNameOut,attrib,mask))!=KErrNone)
		Error(_L("Att 42"),c);
	if ((c=TheFs.Delete(gNameOut))!=KErrNone)
		Error(_L("Delete 1"),c);
	if ((c=TheFs.Delete(_L("TEST1.TMP")))!=KErrNone)
		Error(_L("Delete 2"),c);
	if ((c=TheFs.Delete(_L("FILE2.TMP")))!=KErrNone)
		Error(_L("Delete 3"),c);

	test.Printf(_L("Test write/setEof without UPDATE\n"));
	if ((c=gFile.Create(TheFs,_L("B_GEN.001"),EFileStream))!=KErrNone)
		Error(_L("Open 30"),c);
	gFile.Close();
	if ((c=gFile.Open(TheFs,_L("B_GEN.001"),EFileStream))!=KErrNone)
		Error(_L("Open 30"),c);
	pos=1L;
	if ((c=gFile.SetSize(pos))!=KErrAccessDenied)
		Error(_L("Set EOF 30"),c);
	if ((c=TheFs.Entry(_L("B_GEN.001"),gFileEntry))!=KErrNone)
		Error(_L("File info 30"),c);
	if (gFileEntry.iSize!=0L)
		Error(_L("Size check 30"),0);
	if ((c=gFile.Write(gDataBuf,1))!=KErrAccessDenied)
		Error(_L("Write 30"),c);
	if ((c=TheFs.Entry(_L("B_GEN.001"),gFileEntry))!=KErrNone)
		Error(_L("File info 31"),c);
	if (gFileEntry.iSize!=0L)
		Error(_L("Size check 31"),0);
	gFile.Close();
	if ((c=TheFs.Entry(_L("B_GEN.001"),gFileEntry))!=KErrNone)
		Error(_L("File info 32"),c);
	if (gFileEntry.iSize!=0L)
		Error(_L("Size check 32"),0);
	if ((c=TheFs.Delete(_L("B_GEN.001")))!=KErrNone)
		Error(_L("Delete"),c);


	test.Printf(_L("Test dir entries are written out\n"));
	gNameBuf.SetLength(0);
	
    if ((c=gFile.Temp(TheFs,gNameBuf,gNameOut,EFileStream))!=KErrNone)
		Error(_L("Open 20"),c); 
	
	if ((c=TheFs.Entry(gNameOut,gFileEntry))!=KErrNone)
		Error(_L("File info 0"),c);
	
    if ((gFileEntry.iAtt & KEntryAttArchive)==0)
		Error(_L("Status 20"),0);
	
	test.Printf(_L("Size=%u\n"),gFileEntry.iSize);
	
	if (gFileEntry.iSize!=0L)
		Error(_L("Size check 0"),0);
	
	saveTime=gFileEntry.iModified;
	test.Printf(_L("Wait 3 seconds...\n"));
	User::After(3000000L);
	gDataBuf.SetLength(1);
	
	if ((c=gFile.Write(gDataBuf,1))!=KErrNone)
		Error(_L("Write 1"),c);
	
         /* === pay attention to the code below if the "not updating file timestamp on Flush" mode is enabled (at least on FAT)
         //-- the timestamp in the "real" entry on the media (RFs::Entry()) and what we get by RFile::Modified()
         //-- can differ even after flushing file. The timestamp will be updated only on _closing_ the file.
         //-- This behaviour can be an optimisation to reduce number of media writes due to updating file timestamps.
         gFile.Close();
         nRes = gFile.Open(TheFs, gNameOut, EFileWrite);
         test_KErrNone(nRes);
         //-- restore the expected position in the file
         TInt pos1 = 0;
         nRes = gFile.Seek(ESeekEnd, pos1);
         test_KErrNone(nRes);
         //------------------------------------ 
         */


	if ((c=TheFs.Entry(gNameOut,gFileEntry))!=KErrNone)
		Error(_L("File info 1"),c);
	
	test.Printf(_L("Size=%u\n"),gFileEntry.iSize);
	if (gFileEntry.iSize!=1L)
		Error(_L("Size check 1"),0);
	
	if (gFileEntry.iModified==saveTime)
		Error(_L("Time update"),0);


	gDataBuf.SetLength(16384);
	if ((c=gFile.Write(gDataBuf,16384))!=KErrNone)
		Error(_L("Write 2"),c);
	if ((c=TheFs.Entry(gNameOut,gFileEntry))!=KErrNone)
		Error(_L("File info 2"),c);
	test.Printf(_L("Size=%u\n"),gFileEntry.iSize);
	if (gFileEntry.iSize!=16385L)
		Error(_L("Size check 2"),0);
	pos=0L;
	if ((c=gFile.Seek((TSeek)ESeekStart,pos))!=KErrNone)
		Error(_L("Seek 0"),c);
	if ((c=gFile.Write(gDataBuf,1))!=KErrNone)
		Error(_L("Write 3"),c);
	if ((c=TheFs.Entry(gNameOut,gFileEntry))!=KErrNone)
		Error(_L("File info 3"),c);
	test.Printf(_L("Size=%u\n"),gFileEntry.iSize);
	if (gFileEntry.iSize!=16385L)
		Error(_L("Size check 3"),0);
	pos=0L;
	if ((c=gFile.Seek((TSeek)ESeekStart,pos))!=KErrNone)
		Error(_L("Seek 1"),c);
	if ((c=gFile.Write(gDataBuf,16384))!=KErrNone)
		Error(_L("Write 4"),c);
	if ((c=gFile.Write(gDataBuf,1))!=KErrNone)
		Error(_L("Write 5"),c);
	if ((c=TheFs.Entry(gNameOut,gFileEntry))!=KErrNone)
		Error(_L("File info 4"),c);
	test.Printf(_L("Size=%u\n"),gFileEntry.iSize);
	if (gFileEntry.iSize!=16385L)
		Error(_L("Size check 4"),0);
	if ((c=gFile.Write(gDataBuf,1))!=KErrNone)
		Error(_L("Write 6"),c);
	if ((c=TheFs.Entry(gNameOut,gFileEntry))!=KErrNone)
		Error(_L("File info 5"),c);
	test.Printf(_L("Size=%u\n"),gFileEntry.iSize);
	if (gFileEntry.iSize!=16386L)
		Error(_L("Size check 5"),0);
	for (i=0;i<50;i++)
		{
		TInt r=(Math::Rand(seed) & 0x7fff);
		test.Printf(_L("%u) Set eof to %u\n"),i,r);
		pos=r;
		if ((c=gFile.SetSize(pos))!=KErrNone)
			Error(_L("Set EOF 1"),c);
		if ((c=TheFs.Entry(gNameOut,gFileEntry))!=KErrNone)
			Error(_L("File info 6"),c);
		if (gFileEntry.iSize!=r)
			Error(_L("Size check 6"),i);
		}
	pos=0L;
	if ((c=gFile.SetSize(pos))!=KErrNone)
		Error(_L("Set EOF 2"),c);
	if ((c=TheFs.Entry(gNameOut,gFileEntry))!=KErrNone)
		Error(_L("File info 7"),c);
	if (gFileEntry.iSize!=0L)
		Error(_L("Size check 7"),0);
	gFile.Close();
	if ((c=TheFs.Entry(gNameOut,gFileEntry))!=KErrNone)
		Error(_L("File info 8"),c);
	if (gFileEntry.iSize!=0L)
		Error(_L("Size check 7"),0);
	mask=KEntryAttArchive;
	attrib=0;
	if ((c=TheFs.SetAtt(gNameOut,attrib,mask))!=KErrNone)
		Error(_L("Att 20"),c);

//
	if ((c=gFile.Open(TheFs,gNameOut,EFileStream|EFileWrite))!=KErrNone)
		Error(_L("Open 21"),c);
	if ((c=TheFs.Entry(gNameOut,gFileEntry))!=KErrNone)
		Error(_L("File info 9"),c);
	if (gFileEntry.iAtt & KEntryAttArchive)
		Error(_L("Status 21"),0);
	if ((c=gFile.Write(gDataBuf,0))!=KErrNone)
		Error(_L("Write 21"),c);
	if ((c=TheFs.Entry(gNameOut,gFileEntry))!=KErrNone)
		Error(_L("File info 9"),c);
	if ((gFileEntry.iAtt & KEntryAttArchive))       // write 0 should not modify //
		Error(_L("Status 22"),0);
	gFile.Close();
	mask=KEntryAttArchive;
	attrib=0;
	if ((c=TheFs.SetAtt(gNameOut,attrib,mask))!=KErrNone)
		Error(_L("Att 20"),c);
	if ((c=gFile.Open(TheFs,gNameOut,EFileStream|EFileWrite))!=KErrNone)
		Error(_L("Open 22"),c);
	if ((c=TheFs.Entry(gNameOut,gFileEntry))!=KErrNone)
		Error(_L("File info 9"),c);
	if (gFileEntry.iAtt & KEntryAttArchive)
		Error(_L("Status 23"),0);
	pos=0L;
	if ((c=gFile.SetSize(pos))!=KErrNone)        // no change //
		Error(_L("Set EOF 21"),c);
	if ((c=TheFs.Entry(gNameOut,gFileEntry))!=KErrNone)
		Error(_L("File info 9"),c);
	if ((gFileEntry.iAtt & KEntryAttArchive))
		Error(_L("Status 24"),0);
	gFile.Close();
	if ((c=TheFs.Entry(gNameOut,gFileEntry))!=KErrNone)
		Error(_L("File info 0"),c);
	if ((c=gFile.Open(TheFs,gNameOut,EFileStream|EFileWrite))!=KErrNone)
		Error(_L("Open 23"),c);
	saveTime=gFileEntry.iModified;
	test.Printf(_L("Wait 3 seconds...\n"));
	User::After(3000000L);
	if ((c=gFile.Flush())!=KErrNone)        // Should not alter time //
		Error(_L("Flush 0"),c);
	if (gFileEntry.iModified!=saveTime)
		Error(_L("Flush new time"),0);
	gFile.Close();
	if ((c=TheFs.Entry(gNameOut,gFileEntry))!=KErrNone)
		Error(_L("File info 61"),c);
	if (gFileEntry.iModified!=saveTime)
		Error(_L("Close new time"),0);
	if ((c=gFile.Open(TheFs,gNameOut,EFileStream|EFileWrite))!=KErrNone)
		Error(_L("Open 24"),c);
	if ((c=gFile.Write(gDataBuf,1))!=KErrNone)
		Error(_L("Write 60"),c);
	if ((c=TheFs.Entry(gNameOut,gFileEntry))!=KErrNone)
		Error(_L("File info 62"),c);
	if (gFileEntry.iModified==saveTime)
		Error(_L("Write new time 1"),0);
	saveTime=gFileEntry.iModified;
	test.Printf(_L("Wait 3 seconds...\n"));
	User::After(3000000L);
	if ((c=gFile.Flush())!=KErrNone)        // Should alter time //
		Error(_L("Flush 1"),c);
	if ((c=TheFs.Entry(gNameOut,gFileEntry))!=KErrNone)
		Error(_L("File info 64"),c);
	if (gFileEntry.iModified!=saveTime) // ==saveTime) // !!! Flush doesn't alter the time unless the file is modified
		Error(_L("Flush new time 1"),0);
	gFile.Close();
	if ((c=TheFs.Delete(gNameOut))!=KErrNone)
		Error(_L("Delete"),c);

	test.Printf(_L("Test set file date\n"));
	gNameOut.SetLength(0);
	if ((c=gFile.Temp(TheFs,gNameBuf,gNameOut,EFileStream))!=KErrNone)
		Error(_L("Open 10"),c);
	gFile.Close();
	if ((c=gFile.Open(TheFs,gNameOut,EFileStream))!=KErrNone)
		Error(_L("Open 10"),c); // Temp file is created as writable. 
	TTime fileTime(0);
	if ((c=gFile.SetModified(fileTime))!=KErrAccessDenied) // KErrInUse)
		Error(_L("Set file date 10"),c);
	gFile.Close();


	for (i=0;i<DateTests;i++)
		{
//      TUint32 testSeconds[] = 
//          {
//          0L, 
//          315532799L, 315532800L,	315532801L,315532802L,
//          0xfffffffeL,0xffffffffL,0x7fffffffL,
//          0x80000000L,0x80000001L
//          };
//      TUint32 checkSeconds[] = 
//	        { 
//	        315532800L,	315532800L,	315532800L,	315532800L, 315532802L,
//	        0xfffffffeL,0xfffffffeL,0x7ffffffeL,
//	        0x80000000L,0x80000000L
//	        };
//		TInt64 num64((TReal)(testSeconds[i]*1000000)); // !!! NT fails on dates < 1601
//		TTime fileTime(num64);
//		if ((c=TheFs.SetModified(gNameBuf,fileTime))!=KErrNone)
//			Error(_L("Set file date 0"),c);
//		if ((c=TheFs.Entry(gNameBuf,gFileEntry))!=KErrNone)
//			Error(_L("File info 10"),c);
//		num64=(TReal)checkSeconds[i]*1000000;
//		if (gFileEntry.iModified.Int64()!=num64)
//			Error(_L("Date check 0"),i);
		}
	if ((c=TheFs.Delete(gNameOut))!=KErrNone)
		Error(_L("Delete 1"),c);
	test.Printf(_L("Test format\n"));
	gNameOut.SetLength(0);
	if ((c=gFile.Temp(TheFs,gNameBuf,gNameOut,EFileStream))!=KErrNone)
		Error(_L("Open 1"),c);
	if ((c=gFormat.Open(TheFs,fBuf,EFullFormat,count))!=KErrInUse)
		Error(_L("Format lock check 1"),c);
	if ((c=gFormat.Open(TheFs,fBuf,EQuickFormat,count))!=KErrInUse)
		Error(_L("Format lock check 2"),c);
	gFile.Close();

	if ((c=gFormat.Open(TheFs,fBuf,EFullFormat,count))!=KErrNone)
		{
		if (c==KErrInUse || c==KErrAccessDenied)
			{
			test.Printf(_L("Format: locked, no test\n"));
			}
		    else
            {
		Error(_L("Format lock check"),c);
		}
		}

	gFormat.Close();
	
	DeleteTestDirectory();
	
	}
