// Copyright (c) 2008-2009 Nokia Corporation and/or its subsidiary(-ies).
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
// f32test\plugins\version_2\crypto_encryption\src\t_encrypt.cpp
// 
//

#include <f32file.h>
#include <e32test.h>
#include <f32dbg.h>
#include "t_server.h"
#include "t_chlffs.h"

#define ExpandMe(X)	 L ## X
#define Expand(X)	 ExpandMe(X)
_LIT8(KWriteData,"Testing");
_LIT(KFileName,"c:\\testfile.txt");
_LIT(KFileName2,"c:\\testfilereplace.txt");
_LIT(KSizeDir,"c:\\sizedir\\");
_LIT(KSizeFileName,"testsize.txt");
_LIT(KRenameFileName,"c:\\testfilerename.txt");
const TInt KHeaderSize=16;
const TUint file_start=0;
GLDEF_D RTest test(_L("T_ENCPLUGIN"));
TExitType EncManInvokeUp(TBool aLoad,TBool aEnable)
	{
	TRequestStatus status;
	TBuf<5> cmd_params;
	if (aLoad)
		cmd_params.Append(_L("l"));
	if (aEnable)
		cmd_params.Append(_L(" e"));
	RProcess proc;
	proc.Create(_L("encman.exe"),cmd_params);
	proc.Logon(status);
	proc.Resume();
	User::WaitForRequest(status);
	TExitType exit_type=proc.ExitType();
	proc.Close();
	return (exit_type);
	}
TExitType EncManInvokeDn(TBool aDisable,TBool aUnload)
	{
	TRequestStatus status;
	TBuf<5> cmd_params;
	if (aDisable)
		cmd_params.Append(_L("d"));
	if (aUnload)
		cmd_params.Append(_L(" u"));
	RProcess proc;
	proc.Create(_L("encman.exe"),cmd_params);
	proc.Logon(status);
	proc.Resume();
	User::WaitForRequest(status);
	TExitType exit_type=proc.ExitType();
	proc.Close();
	return (exit_type);
	}
TExitType EncManInvoke(TDesC& aCmdParams)
	{
	TRequestStatus status;
	RProcess proc;
	proc.Create(_L("encman.exe"),aCmdParams);
	proc.Logon(status);
	proc.Resume();
	User::WaitForRequest(status);
	TExitType exit_type=proc.ExitType();
	proc.Close();
	return (exit_type);
	}
inline void safe_test(TInt aError,TInt aLine,TText* aName,TInt aExpectedError=KErrNone)
	{
	if(aError!=aExpectedError)
		{
		test.Printf(_L("Error: %d received on line %d\n"),aError,aLine);
		EncManInvokeDn(ETrue,EFalse);//disable encryption, may or may not be required
		EncManInvokeDn(EFalse,ETrue);//dismount and unload plugin, may or may not be required		
		test.operator()(EFalse,aLine,(TText*)aName);
		}
	}
inline void safe_test(TInt aError,TInt aLine,TText* aName,TInt aExpectedError1,TInt aExpectedError2)
	{
	if((aError!=aExpectedError1)&&(aError!=aExpectedError2))
		{
		test.Printf(_L("Error: %d received on line %d\n"),aError,aLine);
		EncManInvokeDn(ETrue,EFalse);//disable encryption, may or may not be required
		EncManInvokeDn(EFalse,ETrue);//dismount and unload plugin, may or may not be required		
		test.operator()(EFalse,aLine,(TText*)aName);
		}
	}
inline void safe_test(TExitType aExitType,TInt aLine,TText* aName,TExitType aExpectedExitType=EExitKill)
	{
	if(aExitType!=aExpectedExitType)
		{
		test.Printf(_L("Exittype: %d receieved on line %d\n"),aExitType,aLine);
		EncManInvokeDn(ETrue,EFalse);//disable encryption, may or may not be required
		EncManInvokeDn(EFalse,ETrue);//dismount and unload plugin, may or may not be required		
		test.operator()(EFalse,aLine,(TText*)aName);
		}
	}
inline void safe_check(TInt aValue,TInt aLine,TText* aName,TInt aExpectedValue)
	{
	if(aValue!=aExpectedValue)
		{
		test.Printf(_L("value: %d received,expected value:%d on line %d\n"),aValue,aExpectedValue,aLine);
		EncManInvokeDn(ETrue,EFalse);//disable encryption, may or may not be required
		EncManInvokeDn(EFalse,ETrue);//dismount and unload plugin, may or may not be required		
		test.operator()(EFalse,aLine,(TText*)aName);
		}
	}

TInt CheckDrive(TInt aDrive)
    {
    TDriveInfo drvInfo;
    TInt ret = TheFs.Drive(drvInfo,aDrive);
    test (ret == KErrNone);

    if(drvInfo.iType == EMediaRam || drvInfo.iType == EMediaRom || drvInfo.iMediaAtt == KMediaAttWriteProtected || drvInfo.iMediaAtt == KMediaAttLocked)
        {
        test.Printf(_L("Local Buffers are not supported on RAM or ROM drives\n"));
        if(drvInfo.iMediaAtt == KMediaAttLocked)
            {
            test.Printf(_L("This media is locked. Drive %d\n"),(TInt)gDriveToTest);
            }
        test.Printf(_L("Skipping Test\n"));
        return KErrNotSupported;
        }
    return KErrNone;
    }

/**
the return codes are the exit type EExitKill, EExitPanic,etc
*/

GLDEF_C void CallTestsL()
	{
	//The plugin tries to write to the
	//SystemDrive:\private\12345...\encfiles.bin file
	//in the context of intercepting a request.
	//If the systemdrive is a ramdrive then it cannot be written to by a plugin
	//This results in the plugin simply not writing to encfiles.bin, thus if there was power loss the encrypted files would stay encrypted
	//unless they are added to encman manually. (somehow)
	//As I write this plugins do not have access to TDrive::DriveInfo or (via RFsPlugin) RFs::Drive
	//So rather than trying to determine an alternative drive for this situation i've simply decided to ignore it as this is only test code.

	TInt ret=KErrNone;
	RProcess proc;
	TRequestStatus status;
	TExitType exit_type;
	TInt theDrive;
	ret = TheFs.CharToDrive(gDriveToTest,theDrive);
	test(ret == KErrNone);
	TVolumeInfo volInfo;
	ret = TheFs.Volume(volInfo, theDrive);
	test (ret == KErrNone);
	test.Printf(_L("hi\n"));

	if(CheckDrive(theDrive) != KErrNone)
	    {
	    return;
	    }
	
	
	TBuf<256> theFSName;

	ret = TheFs.FileSystemName(theFSName, theDrive);

	TBool Win32Filesystem = (theFSName.CompareF(_L("Win32")) == 0);

    if(!Win32Filesystem)
        {
		// Drive is not mapped to local pc filesystem so can be formatted
		//test.Next(_L("Formating Drive......... "));
		//Format(theDrive);
		}

	// This should be changed to an actual mechanism of determining if the drives filesystem and extensions has local buffer support
	TBool LocalBufferSupported = ETrue;

	if (!LocalBufferSupported)
		{
		if((volInfo.iDrive.iType == EMediaRam) && !Win32Filesystem)
			{
			test.Printf(_L("Local Buffers are not supported on RAM drive\n"));
			test.Printf(_L("Skipping Test\n"));
			return;
			}
		else if(IsFileSystemLFFS(TheFs, theDrive))
			{
			test.Printf(_L("Local Buffers are not supported on LFFS drives\n"));
			test.Printf(_L("Skipping Test\n"));
			return;
			}
		else
			{
			test.Printf(_L("Error: Local Buffers are not supported on the selected drive\n"));
			test(EFalse);
			}
		}

		TBuf<10> extensionName;
		for(TInt i = 0; i < 2; i++)
			{
			ret = TheFs.ExtensionName(extensionName,CurrentDrive(),i);
			if(ret==KErrNone && extensionName.Compare(_L("Nandftl")) == 0)
				{
				test.Printf(_L("Local Buffers are not supported on the nandftl extension\n"));
				test.Printf(_L("Skipping Test\n"));
				return;
				}
			}
	
	test.Next(_L("System test\n"));
	TInt size=0;
	RFile file;
	TFileName filename;
	filename.Append(KFileName);
	filename[0]=(TUint16)gDriveToTest;
	ret=file.Replace(TheFs,filename,EFileWrite);
	test(KErrNone==ret);
	TBuf8<0x100> writebuf(KWriteData);
	TBuf8<0x100> readbuf;
	ret=file.Write(writebuf);
	test(KErrNone==ret);
	ret=file.Read(file_start,readbuf,writebuf.Length());
	test(KErrNone==ret);
	test(readbuf.Compare(writebuf)==0);
	ret=file.SetSize(600);
	test(KErrNone==ret);
	ret=file.Size(size);
	test(KErrNone==ret);
	test(size==600);
	file.Close();

/**	@SYMTestCaseID 		PBASE-T_ENCPLUGIN-1311
	@SYMTestCaseDesc 	Test EFsFileOpen intercept
	@SYMAPI			
	@SYMTestPriority 	Normal
	@SYMTestType		CIT
	@SYMTestPlatform	All
	@SYMTestActions			1)	Load and mount plug-in. 
							2)	Enable encryption. 
							3)	Add file to list of encrypted files
							4)	Disable encryption
							5)	Open file
							6)	Enable encryption. 
							7)	Open file
							8)	Check seek position
							9)	Close file
							10)	Disable encryption and unload plug-in

	@SYMTestExpectedResults	1)	Exit type== EExitKill
							2)	Exit type== EExitKill 
							3)	Exit type== EExitKill 
							4)	Exit type== EExitKill
							5)	KErrAccessDenied
							6)	Exit type== EExitKill
							7)	KErrNone
							8)	Seek position=0
							9)	File closed
							10)	Exit type== EExitKill
	 */

	TBuf<50> cmd_params;
	//test EFsFileOpen intercept
	test.Next(_L("Test EFsFileOpen\n"));
	cmd_params.Append(_L("l e a "));
	cmd_params.Append(filename);
	cmd_params.Append(_L(" d"));
	exit_type=EncManInvoke(cmd_params);
	safe_test(exit_type,__LINE__,(TText*)Expand("t_encrypt.cpp"),EExitKill);
	ret=file.Open(TheFs,filename,EFileRead);
	safe_test(ret,__LINE__,(TText*)Expand("t_encrypt.cpp"),KErrAccessDenied);
	file.Close();

	exit_type=EncManInvokeUp(EFalse,ETrue);//enable
	safe_test(exit_type,__LINE__,(TText*)Expand("t_encrypt.cpp"));

	TInt seekpos=file_start;
	ret=file.Open(TheFs,filename,EFileRead);
	safe_test(ret,__LINE__,(TText*)Expand("t_encrypt.cpp"));
	ret=file.Seek(ESeekCurrent,seekpos);
	safe_test(ret,__LINE__,(TText*)Expand("t_encrypt.cpp"));
	safe_check(seekpos,__LINE__,(TText*)Expand("t_encrypt.cpp"),(TInt)file_start);
	file.Close();

	exit_type=EncManInvokeDn(ETrue,ETrue);//disable,unload
	safe_test(exit_type,__LINE__,(TText*)Expand("t_encrypt.cpp"));
	
/**	@SYMTestCaseID 		PBASE-T_ENCPLUGIN-1325
	@SYMTestCaseDesc 	Basic tests to check loading and unloading of plug-in, enabling and disabling of encryption.
	@SYMAPI			
	@SYMTestPriority 	Normal
	@SYMTestType		CIT
	@SYMTestPlatform	All
	@SYMTestActions			1)	Encman L
							2)	Encman L
							3)	Encman E
							4)	Encman E
							5)	Encman D
							6)	Encman D
							7)	Encman x //some illegal input
							8)	Encman U
							9)	Encman U

	@SYMTestExpectedResults	1)	Exit type= EExitKill
							2)	Exit type= EExitPanic
							3)	Exit type= EExitKill
							4)	Exit type= EExitPanic
							5)	Exit type= EExitKill
							6)	Exit type= EExitPanic
							7)	Exit type= EExitPanic
							8)	Exit type= EExitKill
							9)	Exit type= EExitPanic
	 */

	exit_type=EncManInvokeUp(ETrue,EFalse);//load plug-in
	safe_test(exit_type,__LINE__,(TText*)Expand("t_encrypt.cpp"));
	
	exit_type=EncManInvokeUp(ETrue,EFalse);//load plug-in
	safe_test(exit_type,__LINE__,(TText*)Expand("t_encrypt.cpp"),EExitPanic);
	
	exit_type=EncManInvokeUp(EFalse,ETrue);//enable plug-in
	safe_test(exit_type,__LINE__,(TText*)Expand("t_encrypt.cpp"));
	
	exit_type=EncManInvokeUp(EFalse,ETrue);//enable plug-in
	safe_test(exit_type,__LINE__,(TText*)Expand("t_encrypt.cpp"),EExitPanic);

	exit_type=EncManInvokeDn(ETrue,EFalse);//disable
	safe_test(exit_type,__LINE__,(TText*)Expand("t_encrypt.cpp"));
	
	exit_type=EncManInvokeDn(ETrue,EFalse);//disable
	safe_test(exit_type,__LINE__,(TText*)Expand("t_encrypt.cpp"),EExitPanic);

	proc.Create(_L("encman.exe"),_L("x"));
	proc.Logon(status);
	proc.Resume();
	User::WaitForRequest(status);
	exit_type=proc.ExitType();
	safe_test(exit_type,__LINE__,(TText*)Expand("t_encrypt.cpp"),EExitPanic);
	proc.Close();
	
	exit_type=EncManInvokeDn(EFalse,ETrue);//unload
	safe_test(exit_type,__LINE__,(TText*)Expand("t_encrypt.cpp"));
	
	exit_type=EncManInvokeDn(EFalse,ETrue);//unload
	safe_test(exit_type,__LINE__,(TText*)Expand("t_encrypt.cpp"),EExitPanic);

	
	//Test EFsFileRead,EFsReadFileSection and EFsFileWrite intercept to save lines of code
	test.Next(_L("Test EFsFileRead and EFsFileWrite intercept and EFsReadFileSection intercept\n"));

	exit_type=EncManInvokeUp(ETrue,ETrue);//load , enable
	safe_test(exit_type,__LINE__,(TText*)Expand("t_encrypt.cpp"));
	ret=file.Open(TheFs,filename,EFileWrite);
	safe_test(ret,__LINE__,(TText*)Expand("t_encrypt.cpp"));

/**	@SYMTestCaseID 		PBASE-T_ENCPLUGIN-1312
	@SYMTestCaseDesc 	Test EFsFileRead and EFsFileWrite intercept
	@SYMAPI			
	@SYMTestPriority 	Normal
	@SYMTestType		CIT
	@SYMTestPlatform	All
	@SYMTestActions			1)	Write data completely within an encryption block. Read the same.
							2)	Write data across 2 blocks. Read the same.
							3)	Write data towards the end of the file

	@SYMTestExpectedResults	1)	KErrNone. Read data is the same as written data.
							2)	KErrNone. Read data is the same as written data.
							3)	KErrNone. Read data is the same as written data.
	 */
/**	@SYMTestCaseID 		PBASE-T_ENCPLUGIN-1313
	@SYMTestCaseDesc 	Test EFsReadFileSection intercept
	@SYMAPI			
	@SYMTestPriority 	Normal
	@SYMTestType		CIT
	@SYMTestPlatform	All
	@SYMTestActions			1)	Write data completely within an encryption block. Read the same.
							2)	Write data across 2 blocks. Read the same.
							3)	Write data towards the end of the file

	@SYMTestExpectedResults	1)	KErrNone. Read data is the same as written data.
							2)	KErrNone. Read data is the same as written data.
							3)	KErrNone. Read data is the same as written data.
	 */

	ret=file.Write(writebuf);
	safe_test(ret,__LINE__,(TText*)Expand("t_encrypt.cpp"));
	seekpos=file_start;
	ret=file.Seek(ESeekCurrent,seekpos);
	safe_test(ret,__LINE__,(TText*)Expand("t_encrypt.cpp"));
	safe_check(seekpos,__LINE__,(TText*)Expand("t_encrypt.cpp"),writebuf.Length());
	ret=file.Read(file_start,readbuf,writebuf.Length());
	safe_test(ret,__LINE__,(TText*)Expand("t_encrypt.cpp"));
	safe_check(readbuf.Compare(writebuf),__LINE__,(TText*)Expand("t_encrypt.cpp"),0);
	ret=TheFs.ReadFileSection(filename,file_start,readbuf,writebuf.Length());
	safe_test(ret,__LINE__,(TText*)Expand("t_encrypt.cpp"));
	safe_check(readbuf.Compare(writebuf),__LINE__,(TText*)Expand("t_encrypt.cpp"),0);
	//write across 2 blocks
	ret=file.Write(510,writebuf);
	safe_test(ret,__LINE__,(TText*)Expand("t_encrypt.cpp"));
	ret=file.Read(510,readbuf,writebuf.Length());
	safe_test(ret,__LINE__,(TText*)Expand("t_encrypt.cpp"));
	safe_check(readbuf.Compare(writebuf),__LINE__,(TText*)Expand("t_encrypt.cpp"),0);
	ret=TheFs.ReadFileSection(filename,510,readbuf,writebuf.Length());
	safe_test(ret,__LINE__,(TText*)Expand("t_encrypt.cpp"));
	safe_check(readbuf.Compare(writebuf),__LINE__,(TText*)Expand("t_encrypt.cpp"),0);
	//write towards the end of the file
	ret=file.Write(598,writebuf);
	safe_test(ret,__LINE__,(TText*)Expand("t_encrypt.cpp"));
	ret=file.Read(598,readbuf,writebuf.Length());
	safe_test(ret,__LINE__,(TText*)Expand("t_encrypt.cpp"));
	safe_check(readbuf.Compare(writebuf),__LINE__,(TText*)Expand("t_encrypt.cpp"),0);
	ret=TheFs.ReadFileSection(filename,598,readbuf,writebuf.Length());
	safe_test(ret,__LINE__,(TText*)Expand("t_encrypt.cpp"));
	safe_check(readbuf.Compare(writebuf),__LINE__,(TText*)Expand("t_encrypt.cpp"),0);
	file.Close();
//========================
/**	@SYMTestCaseID 		PBASE-T_ENCPLUGIN-1315
	@SYMTestCaseDesc 	Test EFsFileReplace
	@SYMAPI			
	@SYMTestPriority 	Normal
	@SYMTestType		CIT
	@SYMTestPlatform	All
	@SYMTestActions			1)	Disable encryption
							2)	Replace the file.
							3)	Enable encryption
							4)	Replace the file
							5)	Check the seek position
							6)	Add the replaced file to the list
							7)	Remove the file from the list

	@SYMTestExpectedResults	1)	Exit type= EExitKill
							2)	KErrAccessDenied
							3)	Exit type= EExitKill
							4)	KErrNone
							5)	Seek position is zero
							6)	Exit type= EExitPanic
							7)	Exit type= EExitKill
	 */

	//test EFsFileReplace
	test.Next(_L("Test EFsFileReplace\n"));
	exit_type=EncManInvokeDn(ETrue,EFalse);//disable
	safe_test(exit_type,__LINE__,(TText*)Expand("t_encrypt.cpp"));
	ret=file.Replace(TheFs,filename,EFileWrite);
	safe_test(ret,__LINE__,(TText*)Expand("t_encrypt.cpp"),KErrAccessDenied);
	file.Close();
	exit_type=EncManInvokeUp(EFalse,ETrue);//enable
	safe_test(exit_type,__LINE__,(TText*)Expand("t_encrypt.cpp"));
	
	ret=file.Replace(TheFs,filename,EFileWrite);
	safe_test(ret,__LINE__,(TText*)Expand("t_encrypt.cpp"));
	TInt pos=file_start;
	ret=file.Seek(ESeekCurrent,pos);
	safe_test(ret,__LINE__,(TText*)Expand("t_encrypt.cpp"));
	safe_check(pos,__LINE__,(TText*)Expand("t_encrypt.cpp"),0);
	file.Close();
	cmd_params.Zero();
	cmd_params.Append(_L("a "));
	cmd_params.Append(filename);
	exit_type=EncManInvoke(cmd_params);
	safe_test(exit_type,__LINE__,(TText*)Expand("t_encrypt.cpp"),EExitPanic);
	cmd_params.Zero();
	cmd_params.Append(_L("r n "));
	cmd_params.Append(filename);
	exit_type=EncManInvoke(cmd_params);
	safe_test(exit_type,__LINE__,(TText*)Expand("t_encrypt.cpp"));

	//test EFsReplace
	test.Next(_L("Test EFsReplace\n"));	
	TFileName filename2;
	filename2.Append(KFileName2);
	filename2[0]=(TUint16)gDriveToTest;
	cmd_params.Zero();
	cmd_params.Append(_L("a "));
	cmd_params.Append(filename);
	exit_type=EncManInvoke(cmd_params);
	safe_test(exit_type,__LINE__,(TText*)Expand("t_encrypt.cpp"));	
/**	@SYMTestCaseID 		PBASE-T_ENCPLUGIN-1314
	@SYMTestCaseDesc 	Test EFsReplace
	@SYMAPI			
	@SYMTestPriority 	Normal
	@SYMTestType		CIT
	@SYMTestPlatform	All
	@SYMTestActions			1)	Disable encryption
							2)	Replace the file.
							3)	Enable encryption
							4)	Replace the file
							5)	Remove the file from the list of encrypted files


	@SYMTestExpectedResults	1)	Exit type= EExitKill
							2)	KErrAccessDenied
							3)	Exit type= EExitKill
							4)	KErrNone
							5)	Exit type= EExitPanic
	 */

	exit_type=EncManInvokeDn(ETrue,EFalse);//disable
	safe_test(exit_type,__LINE__,(TText*)Expand("t_encrypt.cpp"));
	ret=TheFs.Replace(filename,filename2);
	safe_test(ret,__LINE__,(TText*)Expand("t_encrypt.cpp"),KErrAccessDenied);
	exit_type=EncManInvokeUp(EFalse,ETrue);//enable
	safe_test(exit_type,__LINE__,(TText*)Expand("t_encrypt.cpp"));

	ret=TheFs.Replace(filename,filename2);
	safe_test(ret,__LINE__,(TText*)Expand("t_encrypt.cpp"));
	cmd_params.Zero();
	cmd_params.Append(_L("r n "));
	cmd_params.Append(filename);
	exit_type=EncManInvoke(cmd_params);
	safe_test(exit_type,__LINE__,(TText*)Expand("t_encrypt.cpp"),EExitPanic);	

/**	@SYMTestCaseID 		PBASE-T_ENCPLUGIN-1319
	@SYMTestCaseDesc 	Test EFsFileSize
	@SYMAPI			
	@SYMTestPriority 	Normal
	@SYMTestType		CIT
	@SYMTestPlatform	All
	@SYMTestActions			1)	Create a file. Write x bytes. Encrypt the file. Get Size.
							2)	Disable encryption. Get Size.
							3)	Dismount and unload plug-in. Get Size.


	@SYMTestExpectedResults	1)	Size=x.
							2)	KErrAccessDenied.
							3)	Size=x + KHeaderSize.
	 */


	//test EFsFileSize
	test.Next(_L("Test EFsFileSize\n"));	
	exit_type=EncManInvokeDn(ETrue,ETrue);//disable unload
	safe_test(exit_type,__LINE__,(TText*)Expand("t_encrypt.cpp"));
	filename2.Zero();
	TFileName dir_name;
	dir_name.Append(KSizeDir);
	dir_name[0]=(TUint16)gDriveToTest;
	ret=TheFs.MkDir(dir_name);
	safe_test(ret,__LINE__,(TText*)Expand("t_encrypt.cpp"),KErrNone,KErrAlreadyExists);
	filename2.Append(dir_name);
	filename2.Append(KSizeFileName);
	ret=file.Replace(TheFs,filename2,EFileWrite);
	safe_test(ret,__LINE__,(TText*)Expand("t_encrypt.cpp"));
	ret=file.Write(KWriteData);
	safe_test(ret,__LINE__,(TText*)Expand("t_encrypt.cpp"));
	ret=file.Size(size);
	safe_test(ret,__LINE__,(TText*)Expand("t_encrypt.cpp"));
	safe_check(size,__LINE__,(TText*)Expand("t_encrypt.cpp"),KWriteData().Size());
	file.Close();
	cmd_params.Zero();
	cmd_params.Append(_L("l e a "));
	cmd_params.Append(filename2);
	exit_type=EncManInvoke(cmd_params);
	safe_test(exit_type,__LINE__,(TText*)Expand("t_encrypt.cpp"));	
	ret=file.Open(TheFs,filename2,EFileRead);
	safe_test(ret,__LINE__,(TText*)Expand("t_encrypt.cpp"));
	ret=file.Size(size);
	safe_test(ret,__LINE__,(TText*)Expand("t_encrypt.cpp"));
	safe_check(size,__LINE__,(TText*)Expand("t_encrypt.cpp"),KWriteData().Size());
	exit_type=EncManInvokeDn(ETrue,EFalse);//disable 
	safe_test(exit_type,__LINE__,(TText*)Expand("t_encrypt.cpp"));
	ret=file.Size(size);
	safe_test(ret,__LINE__,(TText*)Expand("t_encrypt.cpp"),KErrAccessDenied);
	exit_type=EncManInvokeDn(EFalse,ETrue);//unload
	safe_test(exit_type,__LINE__,(TText*)Expand("t_encrypt.cpp"));
	ret=file.Size(size);
	safe_test(ret,__LINE__,(TText*)Expand("t_encrypt.cpp"));
	safe_check(size,__LINE__,(TText*)Expand("t_encrypt.cpp"),KWriteData().Size()+KHeaderSize);
	file.Close();

/**	@SYMTestCaseID 		PBASE-T_ENCPLUGIN-1320
	@SYMTestCaseDesc 	Test EFsFileSetSize
	@SYMAPI			
	@SYMTestPriority 	Normal
	@SYMTestType		CIT
	@SYMTestPlatform	All
	@SYMTestActions			1)	Create a file. Encrypt. Set Size as x bytes.
							2)	Get Size.
							3)	Disable encryption. Set Size =y bytes.
							4)	Dismount and unload plug-in. Get size.



	@SYMTestExpectedResults	1)	KErrNone
							2)	Size=x bytes.
							3)	KErrAccessDenied.
							4)	Size=x + KHeaderSize

	 */

	test.Next(_L("Test EFsFileSetSize\n"));	
	//Test EFsFileSetSize 
	ret=file.Replace(TheFs,filename2,EFileWrite);
	safe_test(ret,__LINE__,(TText*)Expand("t_encrypt.cpp"));
	file.Close();
	cmd_params.Zero();
	cmd_params.Append(_L("l e a "));
	cmd_params.Append(filename2);
	exit_type=EncManInvoke(cmd_params);
	safe_test(exit_type,__LINE__,(TText*)Expand("t_encrypt.cpp"));	
	//file is encrypted now
	ret=file.Open(TheFs,filename2,EFileWrite);
	safe_test(ret,__LINE__,(TText*)Expand("t_encrypt.cpp"));
	const TInt fsize=30;
	ret=file.SetSize(fsize);
	safe_test(ret,__LINE__,(TText*)Expand("t_encrypt.cpp"));
	ret=file.Size(size);
	safe_test(ret,__LINE__,(TText*)Expand("t_encrypt.cpp"));
	safe_check(size,__LINE__,(TText*)Expand("t_encrypt.cpp"),fsize);
	exit_type=EncManInvokeDn(ETrue,EFalse);//disable
	safe_test(exit_type,__LINE__,(TText*)Expand("t_encrypt.cpp"));	
	ret=file.SetSize(fsize);
	safe_test(ret,__LINE__,(TText*)Expand("t_encrypt.cpp"),KErrAccessDenied);
	exit_type=EncManInvokeDn(EFalse,ETrue);//unload
	safe_test(exit_type,__LINE__,(TText*)Expand("t_encrypt.cpp"));	
	ret=file.Size(size);
	safe_check(size,__LINE__,(TText*)Expand("t_encrypt.cpp"),fsize+KHeaderSize);
	file.Close();

	//Test EFsEntry,EFsDirReadPacked,EFsDirReadOne=> being done together to save lines of code
/**	@SYMTestCaseID 		PBASE-T_ENCPLUGIN-1322
	@SYMTestCaseDesc 	Test EFsEntry intercept
	@SYMAPI			
	@SYMTestPriority 	Normal
	@SYMTestType		CIT
	@SYMTestPlatform	All
	@SYMTestActions			1)	Call RFs::Entry 
							2)	Disable encryption. Call RFs::Entry
							3)	Dismount and unload plug-in. Call RFs::Entry
	@SYMTestExpectedResults	1)	File Size from the entry details = x
							2)	KErrAccessDenied
							3)	File Size from the entry details= x + KHeaderSize.
	 */
/**	@SYMTestCaseID 		PBASE-T_ENCPLUGIN-1323
	@SYMTestCaseDesc 	Test EFsDirReadOne intercept
	@SYMAPI			
	@SYMTestPriority 	Normal
	@SYMTestType		CIT
	@SYMTestPlatform	All
	@SYMTestActions			1)	Create a directory. Create a file in the directory. Set file Size as x bytes.(The value of x is immaterial here). Call RDir::Read.
							2)	Disable encryption. Call RDir::Read.
							3)	Dismount and unload plug-in. Call RDir::Read.

	@SYMTestExpectedResults	1)	File Size from the entry details = x
							2)	KErrAccessDenied
							3)	File Size from the entry details= x + KHeaderSize.
	 */

/**	@SYMTestCaseID 		PBASE-T_ENCPLUGIN-1324
	@SYMTestCaseDesc 	Test EFsDirReadPacked intercept
	@SYMAPI			
	@SYMTestPriority 	Normal
	@SYMTestType		CIT
	@SYMTestPlatform	All
	@SYMTestActions			1)	Create a directory. Create a file in the directory. Set file Size as x bytes.(The size of the file is immaterial here). Call RDir::Read.
							2)	Disable encryption. Call RDir::Read(TEntryArray&) overload.
							3)	Dismount and unload plug-in. Call RDir::Read.
	@SYMTestExpectedResults	1)	File Size from the entry details = x
							2)	KErrAccessDenied
							3)	File Size from the entry details= x + KHeaderSize.
	 */
	
	test.Next(_L("Test EFsEntry,EFsDirReadPacked,EFsDirReadOne\n"));	
	TEntry entry;
	ret=TheFs.Entry(filename2,entry);
	safe_test(ret,__LINE__,(TText*)Expand("t_encrypt.cpp"));
	safe_check(entry.iSize,__LINE__,(TText*)Expand("t_encrypt.cpp"),fsize+KHeaderSize);

	
	RDir dir;
	ret=dir.Open(TheFs,dir_name,KEntryAttMaskSupported);
	safe_test(ret,__LINE__,(TText*)Expand("t_encrypt.cpp"));
	ret=dir.Read(entry);
	safe_test(ret,__LINE__,(TText*)Expand("t_encrypt.cpp"));
	safe_check(entry.iSize,__LINE__,(TText*)Expand("t_encrypt.cpp"),fsize+KHeaderSize);
	dir.Close();
	ret=dir.Open(TheFs,dir_name,KEntryAttMaskSupported);
	safe_test(ret,__LINE__,(TText*)Expand("t_encrypt.cpp"));
	TEntryArray entryarray;
	ret=dir.Read(entryarray);
	safe_test(ret,__LINE__,(TText*)Expand("t_encrypt.cpp"),KErrNone,KErrEof);
	dir.Close();
	safe_check(entryarray[0].iSize,__LINE__,(TText*)Expand("t_encrypt.cpp"),fsize+KHeaderSize);
	exit_type=EncManInvokeUp(ETrue,EFalse);//load
	safe_test(exit_type,__LINE__,(TText*)Expand("t_encrypt.cpp"));		
	ret=TheFs.Entry(filename2,entry);
	safe_test(ret,__LINE__,(TText*)Expand("t_encrypt.cpp"),KErrAccessDenied);
	exit_type=EncManInvokeUp(EFalse,ETrue);//enable
	safe_test(exit_type,__LINE__,(TText*)Expand("t_encrypt.cpp"));		
	ret=TheFs.Entry(filename2,entry);
	safe_test(ret,__LINE__,(TText*)Expand("t_encrypt.cpp"));
	safe_check(entry.iSize,__LINE__,(TText*)Expand("t_encrypt.cpp"),fsize);
	ret=dir.Open(TheFs,dir_name,KEntryAttMaskSupported);
	safe_test(ret,__LINE__,(TText*)Expand("t_encrypt.cpp"));
	ret=dir.Read(entry);
	safe_test(ret,__LINE__,(TText*)Expand("t_encrypt.cpp"));
	safe_check(entry.iSize,__LINE__,(TText*)Expand("t_encrypt.cpp"),fsize);
	dir.Close();
	ret=dir.Open(TheFs,dir_name,KEntryAttMaskSupported);
	safe_test(ret,__LINE__,(TText*)Expand("t_encrypt.cpp"));
	ret=dir.Read(entryarray);
	safe_test(ret,__LINE__,(TText*)Expand("t_encrypt.cpp"),KErrNone,KErrEof);
	dir.Close();
	safe_check(entryarray[0].iSize,__LINE__,(TText*)Expand("t_encrypt.cpp"),fsize);

	exit_type=EncManInvokeDn(ETrue,EFalse);//disable
	safe_test(exit_type,__LINE__,(TText*)Expand("t_encrypt.cpp"));		
	ret=dir.Open(TheFs,dir_name,KEntryAttMaskSupported);
	safe_test(ret,__LINE__,(TText*)Expand("t_encrypt.cpp"));
	ret=dir.Read(entry);
	safe_test(ret,__LINE__,(TText*)Expand("t_encrypt.cpp"),KErrAccessDenied);
	dir.Close();
	exit_type=EncManInvokeDn(EFalse,ETrue);//unload
	safe_test(exit_type,__LINE__,(TText*)Expand("t_encrypt.cpp"));		

	
	//test EFsFileRename
	TFileName filerename;
	filerename.Append(KRenameFileName);
	filerename[0]=(TUint16)gDriveToTest;
	//encrypt file
	exit_type=EncManInvokeUp(ETrue,ETrue);//load,enable
	safe_test(exit_type,__LINE__,(TText*)Expand("t_encrypt.cpp"));		
	TheFs.Delete(filerename);//need to delete the file before going to the next test
	ret=file.Open(TheFs,filename2,EFileWrite|EFileShareExclusive);
	safe_test(ret,__LINE__,(TText*)Expand("t_encrypt.cpp"));
/**	@SYMTestCaseID 		PBASE-T_ENCPLUGIN-1316
	@SYMTestCaseDesc 	Test EFsFileRename intercept
	@SYMAPI			
	@SYMTestPriority 	Normal
	@SYMTestType		CIT
	@SYMTestPlatform	All
	@SYMTestActions			1)	Rename the encrypted file.
							2)	Try adding the renamed file to the "list".
	@SYMTestExpectedResults	1)	KErrNone. 
							2)	Exit type==EExitPanic
	 */
	test.Next(_L("Test EFsFileRename\n"));	
	ret=file.Rename(filerename);
	file.Close();
	safe_test(ret,__LINE__,(TText*)Expand("t_encrypt.cpp"));
	//try adding the renamed file to the list
	cmd_params.Zero();
	cmd_params.Append(_L("a "));
	cmd_params.Append(filerename);
	exit_type=EncManInvoke(cmd_params);//add file to list of encrypted files
	safe_test(exit_type,__LINE__,(TText*)Expand("t_encrypt.cpp"),EExitPanic);		
	
/**	@SYMTestCaseID 		PBASE-T_ENCPLUGIN-1317
	@SYMTestCaseDesc 	Test EFsRename intercept
	@SYMAPI			
	@SYMTestPriority 	Normal
	@SYMTestType		CIT
	@SYMTestPlatform	All
	@SYMTestActions			1)	Rename the encrypted file.
							2)	Try adding the renamed file to the "list".
	@SYMTestExpectedResults	1)	KErrNone. 
							2)	Exit type==EExitPanic
	 */	
	test.Next(_L("Test EFsRename\n"));	
	//Test EFsRename
	ret=TheFs.Rename(filerename,filename);
	safe_test(ret,__LINE__,(TText*)Expand("t_encrypt.cpp"));
	//try adding the renamed file to the list
	cmd_params.Zero();
	cmd_params.Append(_L("a "));
	cmd_params.Append(filename);
	exit_type=EncManInvoke(cmd_params);//add file to list of encrypted files
	safe_test(exit_type,__LINE__,(TText*)Expand("t_encrypt.cpp"),EExitPanic);		
	
/**	@SYMTestCaseID 		PBASE-T_ENCPLUGIN-1318
	@SYMTestCaseDesc 	Test EFsDelete intercept
	@SYMAPI			
	@SYMTestPriority 	Normal
	@SYMTestType		CIT
	@SYMTestPlatform	All
	@SYMTestActions			1)	Disable encryption
							2)	Delete the encrypted file
							3)	Enable encryption
							4)	Delete the encrypted file
							5)	Try removing the deleted file from the list

	@SYMTestExpectedResults	1)	Exit type==EExitKill
							2)	KErrAccessDenied
							3)	Exit type==EExitKill
							4)	KErrNone
							5)	Exit type==EExitPanic
	 */	
	
	//test EFsDelete
	test.Next(_L("Test EFsDelete\n"));	
	exit_type=EncManInvokeDn(ETrue,EFalse);//disable
	safe_test(exit_type,__LINE__,(TText*)Expand("t_encrypt.cpp"));		
	ret=TheFs.Delete(filename);
	safe_test(ret,__LINE__,(TText*)Expand("t_encrypt.cpp"),KErrAccessDenied);
	exit_type=EncManInvokeUp(EFalse,ETrue);//enable
	safe_test(exit_type,__LINE__,(TText*)Expand("t_encrypt.cpp"));		
	ret=TheFs.Delete(filename);
	safe_test(ret,__LINE__,(TText*)Expand("t_encrypt.cpp"));
	cmd_params.Zero();
	cmd_params.Append(_L("r n "));
	cmd_params.Append(filename);
	exit_type=EncManInvoke(cmd_params);//remove the file from the list
	safe_test(exit_type,__LINE__,(TText*)Expand("t_encrypt.cpp"),EExitPanic);		
	
	//finish the test in same state
	exit_type=EncManInvokeDn(ETrue,ETrue);//disable,unload
	safe_test(exit_type,__LINE__,(TText*)Expand("t_encrypt.cpp"));		
	}

