// Copyright (c) 1996-2010 Nokia Corporation and/or its subsidiary(-ies).
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
// //File Name:	f32test/server/t_file64bit.cpp
// //Description:This file contains implementation for checking the 64bit file
// //			server functionality. All the affected APIs are tested.
// //While generating a file for reading, the contents are generated such that 
// //every four bytes of the file contains its location. So the file would be
// //generated as:
// // 0000: 00 00 00 00 
// // 0004: 04 00 00 00
// // 0008: 08 00 00 00
// // .. etc
// 
//


#define __E32TEST_EXTENSION__
#include <f32file.h>
#include <e32test.h>
#include <e32svr.h>
#include "t_server.h"
#include "t_file64bit.h"
#include "../fileshare/handshare64bit.h"
#include <f32pluginutils.h>
#include <massstorage.h>
#include <e32math.h>
#include "f32_test_utils.h"

using namespace F32_Test_Utils;

RTest test(_L("T_FILE64BIT Tests"));

_LIT(KTestPath,  ":\\F32-TST\\TFILE64BIT\\");

// to test any file system that supports file sizes of greater than 4GB -1, 
// this value shall be set.
TBool KFileSizeMaxLargerThan4GBMinusOne = EFalse;



TInt GenerateBigFileContents()
	{
	test.Printf(_L("GenerateBigFileContents()\n"));

	TInt r;
    const TUint KBufSize = 256*K1KiloByte;
    RBuf8 buf;
    
    r = buf.CreateMax(KBufSize);
    test_KErrNone(r);

    RFile64 file;
	TFileName fileName;
	fileName.Append(gDriveToTest);
	fileName.Append(KTestPath);
	fileName.Append(_L("File4GBMinusOne.txt"));
	r = file.Replace(TheFs,fileName, EFileWrite);
	test_KErrNone(r);
	
    r = file.SetSize(K4GBMinusOne);
	test_KErrNone(r);
	
	TInt64 nNumberOfBytesToWrite = 0;
	TInt64 nNumberOfBytesWritten = 0;
	for (TInt64 pos = 0; pos < K4GBMinusOne; pos += nNumberOfBytesWritten)
		{
		// Prepare the write buffer
		for (TUint n = 0; n<KBufSize; n += 4)
			{
            *((TUint*) &buf[n]) = I64LOW(pos + n);
			}
		
		nNumberOfBytesToWrite = Min(MAKE_TINT64(0,KBufSize), K4GBMinusOne - pos);
        TPtrC8 pText(buf.Ptr(), KBufSize);

		file.Write(pText, (TInt)nNumberOfBytesToWrite);
		
        nNumberOfBytesWritten = nNumberOfBytesToWrite; 
		}
	
	r = file.Flush();
	test_KErrNone(r);
	test.Printf(_L("\nFile writing is completed!!"));
	
	
	file.Close();
	
    buf.Close();

    return KErrNone;
	}

TInt RFileHandleSharer64Bit::Connect()
	{
	return CreateSession(KServerName, TVersion(1,0,0));
	}


TInt RFileHandleSharer64Bit::Exit()
	{
	return SendReceive(EMsgExit, TIpcArgs(NULL));
	}
			
TInt RFileHandleSharer64Bit::SetTestDrive(TInt aDrive)
	{
	return SendReceive(EMsgDrive, TIpcArgs(aDrive));
	}

TInt RFileHandleSharer64Bit::PassFileHandleProcessLargeFileClient(TIpcArgs& aIpcArgs)
	{
	return SendReceive(EMsgPassFileHandleProcessLargeFileClient, aIpcArgs);
	}

TInt RFileHandleSharer64Bit::PassFileHandleProcessLargeFileCreator()
	{
	return SendReceive(EMsgPassFileHandleProcessLargeFileCreator);
	}

TInt RFileHandleSharer64Bit::GetFileHandleLargeFile2(TInt &aHandle, TFileMode aFileMode)
	{
	TPckgBuf<TInt> fh;
	TInt fsh = SendReceive(EMsgGetFileHandleLargeFile, TIpcArgs(&fh, aFileMode));
	aHandle = fh();
	return fsh;
	}
	
void RFileHandleSharer64Bit::Sync()
	{
	SendReceive(EMsgSync, TIpcArgs());
	}	


CFileManObserver::CFileManObserver(CFileMan* aFileMan)
	{
	__DECLARE_NAME(_S("CFileManObserver"));
	iFileMan = aFileMan;
	}

MFileManObserver::TControl CFileManObserver::NotifyFileManStarted()
	{
	return(MFileManObserver::EContinue);
	}

MFileManObserver::TControl CFileManObserver::NotifyFileManOperation()
	{
	return(MFileManObserver::EContinue);
	}

MFileManObserver::TControl CFileManObserver::NotifyFileManEnded()
	{
	TInt lastError = iFileMan->GetLastError();
	TFileName fileName = iFileMan->CurrentEntry().iName;
	test.Printf(_L("NotifyFileManEnded(): Error %d File %S\n"),lastError, &fileName);
	if (lastError == KErrNone)
		iNotifyEndedSuccesses++;
	else
		iNotifyEndedFailures++;
	return(MFileManObserver::EContinue);
	}



RFsTest& RFsTest::Replace(const TDesC &anOldName, const TDesC &aNewName)
//
// Replaces a single file with another
//
	{
	test.Printf(_L("%S File Replaced with %S\n"),&anOldName,&aNewName);\
	TInt r = TheFs.Replace(anOldName,aNewName);
	test_KErrNone(r);
	return(*this);
	}
	
RFsTest& RFsTest::ReadFileSection(const TDesC& aName, TInt64 aPos,TDes8& aBuffer,TInt aLen)
//
// Reads data from the file without opening it. Expected not to fail.
//
	{
	test.Printf(_L("Read File Section %S\n"),&aName);
	TInt r = TheFs.ReadFileSection(aName,aPos,aBuffer,aLen);
	TInt len = aBuffer.Length();
	test_KErrNone(r);
	if(KFileSizeMaxLargerThan4GBMinusOne == EFalse && aPos >= K4GB)
		{
		test_Equal(0, len);
		}
	return(*this);
	}
	
	
RFsTest& RFsTest::GetDir(const TDesC &aName, TUint anEntryAttMask, TUint anEntrySortKey, CDir *&anEntryList)
//
// Gets a filtered list of a directory's contents.
//
	{
	test.Printf(_L("Name of the directory for which listing is required %S\n"),&aName);	
	TInt r = TheFs.GetDir(aName,anEntryAttMask,anEntrySortKey,anEntryList);
	test_KErrNone(r);
	return(*this);
	}

RFsTest& RFsTest::GetDir(const TDesC& aName,TUint anEntryAttMask,TUint anEntrySortKey,CDir*& anEntryList,CDir*& aDirList)
//
// Gets a filtered list of the directory and the file entries contained in a directory and a
// list of the directory entries only.
	{
	test.Printf(_L("Name of the directory for which directory and file listing is required %S\n"),&aName);	
	TInt r = TheFs.GetDir(aName,anEntryAttMask,anEntrySortKey,anEntryList,aDirList);
	test_KErrNone(r);
	return(*this);
	}

RFsTest& RFsTest::GetDir(const TDesC& aName,const TUidType& anEntryUid,TUint anEntrySortKey,CDir*& aFileList)
//
// Gets a filtered list of directory contents by UID type.
//
	{
	test.Printf(_L("Name of the directory for which listing is required %S\n"),&aName);	
	TInt r = TheFs.GetDir(aName,anEntryUid,anEntrySortKey,aFileList);
	test_KErrNone(r);
	return(*this);	
	}
		
	
RFileTest::RFileTest(const TDesC& aName)
//
// Constructor
//
	: iName(aName)
	{}

RFileTest& RFileTest::Create(const TDesC& aName, TUint aFileMode)
//
// Creates and opens a new file for writing, if the file already exists an error is returned
//
	{
	test.Printf(_L("%S create %S in %d Mode\n"),&iName,&aName,aFileMode);
	TInt r = RFile64::Create(TheFs,aName,aFileMode);
	test_Value(r, r == KErrNone || r == KErrAlreadyExists);
	return(*this);		
	}
	
RFileTest& RFileTest::Replace(const TDesC& aName)
//
// Opens a file for writing, replacing the content of any existing file of the same name
// if it exists or cretaing a new file if it does not exist.
//
	{
	test.Printf(_L("%S replace %S\n"),&iName,&aName);
	TInt r = RFile64::Replace(TheFs,aName,EFileStream|EFileWrite);
	test_KErrNone(r);
	return(*this);
	}

RFileTest& RFileTest::Replace(const TDesC& aName, TUint aFileMode)
//
// Opens a file in aFileMode, replacing the content of any existing file of the same name
// if it exists or cretaing a new file if it does not exist.
//
	{
	test.Printf(_L("%S replace %S in %d Mode\n"),&iName,&aName, aFileMode);
	TInt r = RFile64::Replace(TheFs,aName,aFileMode);
	test_Value(r, r == KErrNone || r == KErrBadName);
	return(*this);		
	}
	
RFileTest& RFileTest::Open(const TDesC& aName)
//
// Open a existing file for reading and writing in shared access mode.
//
	{
	test.Printf(_L("%S open %S\n"),&iName,&aName);
	TInt r = RFile64::Open(TheFs,aName,EFileWrite|EFileShareAny);
	test_KErrNone(r);
	return(*this);
	}

RFileTest& RFileTest::Open(const TDesC& aName, TUint aFileMode)
//
// Opens an existing file using aFileMode.
//
	{
	test.Printf(_L("%S open %S in %d Mode\n"),&iName,&aName, aFileMode);
	TInt r = RFile64::Open(TheFs,aName,aFileMode);
	test_KErrNone(r);
	return(*this);
	}

RFileTest& RFileTest::Temp(const TDesC& aPath,TFileName& aName,TUint aFileMode)
//
// Creates and opens a temporary file with a unique name for writing and reading.
//
	{
	test.Printf(_L("%S Temp file %S in %d Mode\n"),&iName,&aName, aFileMode);
	TInt r = RFile64::Temp(TheFs,aPath,aName,aFileMode);	
	test_KErrNone(r);
	return(*this);
	}
	
void RFileTest::Close()
//
// Closes the file.
//
	{
	RFile::Close();
	}
	
RFileTest& RFileTest::Lock(TInt64 aPos, TInt64 aLen)
//
// Set a lock on the file. Expected not to fail.
//
	{
	test.Printf(_L("%S lock   0x%lx-0x%lx\n"),&iName,aPos,aPos+aLen-1);
	TInt r = RFile64::Lock(aPos,aLen);
	test_KErrNone(r);
	return(*this);
	}

RFileTest& RFileTest::LockE(TInt64 aPos, TInt64 aLen)
//
// Set a lock on the file. Expected to fail.
//
	{
	test.Printf(_L("%S lockE  0x%lx-0x%lx\n"),&iName,aPos,aPos+aLen-1);
	TInt r = RFile64::Lock(aPos,aLen);
	test_Value(r, r == KErrLocked);
	return(*this);
	}

RFileTest& RFileTest::UnLock(TInt64 aPos, TInt64 aLen)
//
// Unlock the file. Expected not to fail.
//
	{
	test.Printf(_L("%S ulock  0x%lx-0x%lx\n"),&iName,aPos,aPos+aLen-1);
	TInt r = RFile64::UnLock(aPos,aLen);
	test_KErrNone(r);
	return(*this);
	}

RFileTest& RFileTest::UnLockE(TInt64 aPos, TInt64 aLen)
//
// Unlock the file. Expected to fail.
//
	{
	test.Printf(_L("%S ulockE 0x%lx-0x%lx\n"),&iName,aPos,aPos+aLen-1);
	TInt r = RFile64::UnLock(aPos,aLen);
	test_Value(r, r == KErrNotFound);
	return(*this);
	}

RFileTest& RFileTest::Write(const TDesC8& aDes)
//
// Write to the file.
//
	{
	test.Printf(_L("%S write\n"),&iName);
	
	TInt64 seekPos = 0;
	TInt r = RFile64::Seek(ESeekCurrent,seekPos);
	test_KErrNone(r);
		
	r = RFile64::Write(aDes);
	if( KErrNone == r)	// this is to ensure that the written data is committed and not cached.
		r = RFile64::Flush(); 		
		
	
	if(KFileSizeMaxLargerThan4GBMinusOne == EFalse)
		{
		if((seekPos + aDes.Length()) < K4GB)
			{
			test_KErrNone(r);
			}
		else
			{
			test_Value(r, r == KErrNotSupported);
			}
		}
	else
		{
		test_KErrNone(r);	
		}
	return(*this);
	}

RFileTest& RFileTest::Write(const TDesC8 &aDes, TRequestStatus &aStatus)
//
// Write to the file.
//
	{
	test.Printf(_L("%S write\n"),&iName);
	
	TInt64 seekPos = 0;
	TInt r = RFile64::Seek(ESeekCurrent,seekPos);
	test_KErrNone(r);
		
	RFile64::Write(aDes, aStatus);
	User::WaitForRequest(aStatus);
	if( KErrNone == aStatus.Int())	// this is to ensure that the written data is committed and not cached.
		{
		RFile64::Flush(aStatus); 
		User::WaitForRequest(aStatus);
		}
	if(KFileSizeMaxLargerThan4GBMinusOne == EFalse)
		{
		if((seekPos + aDes.Length()) < K4GB)
			{
			test_KErrNone(aStatus.Int());
			}
		else
			{
			test_Equal(KErrNotSupported, aStatus.Int());
			}
		}
	else
		{
		test_KErrNone(aStatus.Int());
		}
	return(*this);
	}
	
RFileTest& RFileTest::Write(const TDesC8& aDes, TInt aLength)
//
// Write aLength specified number of bytes to the file.
//
	{
	test.Printf(_L("%S write\n"),&iName);
	
	TInt64 seekPos = 0;
	TInt r = RFile64::Seek(ESeekCurrent,seekPos);
	test_KErrNone(r);
	
	r = RFile64::Write(aDes, aLength);
	if( KErrNone == r)	// this is to ensure that the written data is committed and not cached.
		r = RFile64::Flush(); 		
	if(KFileSizeMaxLargerThan4GBMinusOne == EFalse)
		{
		if((seekPos + aLength) < K4GB)
			{
			test_KErrNone(r);
			}
		else
			{
			test_Value(r, r == KErrNotSupported);
			}
		}
	else
		{
		test_KErrNone(r);
		}
	return(*this);	
	}	

RFileTest& RFileTest::Write(const TDesC8& aDes, TInt aLength, TRequestStatus &aStatus)	
//
// Write aLength specified number of bytes to the file. Expected not to fail (Asynchronous).
//
	{
	test.Printf(_L("%S write\n"),&iName);
	
	TInt64 seekPos = 0;
	TInt r = RFile64::Seek(ESeekCurrent,seekPos);
	test_KErrNone(r);
	
	RFile64::Write(aDes,aLength,aStatus);
	User::WaitForRequest(aStatus);
	if( KErrNone == aStatus.Int())	// this is to ensure that the written data is committed and not cached.
		{
		RFile64::Flush(aStatus);
		User::WaitForRequest(aStatus);	
		}
		
	if(KFileSizeMaxLargerThan4GBMinusOne == EFalse)
		{
		if((seekPos + aLength) < K4GB)
			{
			test_KErrNone(aStatus.Int());
			}
		else
			{
			test_Equal(KErrNotSupported, aStatus.Int());
			}
		}
	else
		{
		test(aStatus.Int() == KErrNone);
		}
	return(*this);
	}
	
RFileTest& RFileTest::WriteP(TInt64 aPos, const TDesC8& aDes)
//
// Write to the file. Expected not to fail.
//
	{
	test.Printf(_L("%S write  0x%lx\n"),&iName,aPos);
	TInt r = RFile64::Write(aPos,aDes);
	if( KErrNone == r)	// this is to ensure that the written data is committed and not cached.
		r = RFile64::Flush();
	if(KFileSizeMaxLargerThan4GBMinusOne == EFalse)
		{
		if ((aPos + aDes.Length()) < K4GB)
			{
			test_KErrNone(r);
			}
		else
			{
			test_Value(r, r == KErrNotSupported);
			}
		}
	else
		{
		test_KErrNone(r);
		}
	return(*this);
	}

RFileTest& RFileTest::WriteU(TUint aPos, const TDesC8& aDes)
//
// Write to the file. Expected not to fail.
// Position is a TUint value
//
	{
	test.Printf(_L("%S write  %08x\n"),&iName,aPos);
	TInt r = RFile64::Write(aPos,aDes);
	if( KErrNone == r)	// this is to ensure that the written data is committed and not cached.
		r = RFile64::Flush();
	test_KErrNone(r);
	return(*this);
	}


RFileTest& RFileTest::Write(TInt64 aPos, const TDesC8& aDes, TInt aLen)
//
// Write to the file. Synchronous Expected not to fail.
//
	{
	test.Printf(_L("%S write  0x%lx-0x%lx\n"),&iName,aPos,aPos+aLen-1);
	TInt r = RFile64::Write(aPos,aDes,aLen);
	if( KErrNone == r)	// this is to ensure that the written data is committed and not cached.
		r = RFile64::Flush();
	if(KFileSizeMaxLargerThan4GBMinusOne == EFalse)
		{
		if ((aPos + aLen) < K4GB)
			{
			if (aLen < 0)
				{
				test_Value(r, r == KErrArgument);
				}
			else
				{
				test_KErrNone(r);
				}
			}
		else
			{
			test_Value(r, r == KErrNotSupported);
			}
		}
	else
		{
		if (aLen < 0)
			{
			test_Value(r, r == KErrArgument);
			}
		else
			{
			test_KErrNone(r);
			}
		}
	return(*this);
	}

RFileTest& RFileTest::WriteU(TUint aPos, const TDesC8& aDes, TInt aLen)
//
// Write to the file. Synchronous Expected not to fail.
// Position is a TUint value
//
	{
	test.Printf(_L("%S write  %08x-%08x\n"),&iName,aPos,aPos+aLen-1);
	TInt r = RFile64::Write(aPos,aDes,aLen);
	if( KErrNone == r)	// this is to ensure that the written data is committed and not cached.
		r = RFile64::Flush();
	test_KErrNone(r);
	return(*this);
	}


RFileTest& RFileTest::WriteE(TInt64 aPos, const TDesC8& aDes, TInt aLen)
//
// Write to the file. Expected to fail.
//
	{
	test.Printf(_L("%S writeE 0x%lx-0x%lx\n"),&iName,aPos,aPos+aLen-1);
	TInt r = RFile64::Write(aPos,aDes,aLen);
	if (aLen < 0)
		{
		test_Equal(KErrArgument, r);
		}
	else
		{
		test_Equal(KErrLocked, r);
		}
	return(*this);
	}

RFileTest& RFileTest::Write(TInt64 aPos, const TDesC8& aDes, TInt aLen, TRequestStatus &aStatus)
// 
// Write to the file. Asynchronous
	{
	test.Printf(_L("%S write  0x%lx-0x%lx\n"),&iName,aPos,aPos+aLen-1);
	RFile64::Write(aPos,aDes,aLen,aStatus);
	User::WaitForRequest(aStatus);
	if( KErrNone == aStatus.Int())	// this is to ensure that the written data is committed and not cached.
		{
		RFile64::Flush(aStatus);
		User::WaitForRequest(aStatus);	
		}

	if(aLen < 0)
		{
		test_Equal(KErrArgument, aStatus.Int());
		}
	else
		{
		if(KFileSizeMaxLargerThan4GBMinusOne == EFalse)
			{
			if((aPos + aLen) < K4GB)
				{
				test_KErrNone(aStatus.Int());
				}
			else
				{
				test_Equal(KErrNotSupported, aStatus.Int());
				}
			}
		else
			test_KErrNone(aStatus.Int());
		}
	return(*this);	
	}
	
RFileTest& RFileTest::Write(TInt64 aPos, const TDesC8& aDes, TRequestStatus &aStatus)
//
// Write to the file (Asynchronous).
	{
	test.Printf(_L("%S write  0x%lx\n"),&iName,aPos);
	RFile64::Write(aPos,aDes,aStatus);
	User::WaitForRequest(aStatus);
	if( KErrNone == aStatus.Int())	// this is to ensure that the written data is committed and not cached.
		{
		RFile64::Flush(aStatus);
		User::WaitForRequest(aStatus);	
		}
	
	
	if(KFileSizeMaxLargerThan4GBMinusOne == EFalse)
		{
		if((aPos + aDes.Length()) < K4GB)
			{
			test_KErrNone(aStatus.Int());
			}
		else
			{
			test_Equal(KErrNotSupported, aStatus.Int());
			}
		}
	else
		{
		test_KErrNone(aStatus.Int());
		}
	return(*this);	
	}

RFileTest& RFileTest::WriteU(TUint aPos, const TDesC8& aDes, TRequestStatus &aStatus)
//
// Write to the file (Asynchronous).
// Position is a TUint value
//
	{
	test.Printf(_L("%S write  %08x\n"),&iName,aPos);
	RFile64::Write(aPos,aDes,aStatus);
	User::WaitForRequest(aStatus);
	if( KErrNone == aStatus.Int())	// this is to ensure that the written data is committed and not cached.
		{
		RFile64::Flush(aStatus);
		User::WaitForRequest(aStatus);	
		}
	test_KErrNone(aStatus.Int());
	return(*this);	
	}

RFileTest& RFileTest::WriteU(TUint aPos, const TDesC8& aDes, TInt aLen, TRequestStatus &aStatus)
// 
// Write to the file. Asynchronous
// Position is a TUint value
//
	{
	test.Printf(_L("%S write  %08x-%08x\n"),&iName,aPos,aPos+aLen-1);
	RFile64::Write(aPos,aDes,aLen,aStatus);
	User::WaitForRequest(aStatus);
	if( KErrNone == aStatus.Int())	// this is to ensure that the written data is committed and not cached.
		{
		RFile64::Flush(aStatus);
		User::WaitForRequest(aStatus);	
		}

	if(aLen < 0)
		{
		test_Equal(KErrArgument, aStatus.Int());
		}
	else
		{
		test_KErrNone(aStatus.Int());
		}
	return(*this);	
	}


	
RFileTest& RFileTest::Read(TDes8& aDes) 
//
// Read from the file. Expected not to fail (Synchronous).
//
	{
	test.Printf(_L("%S read \n"),&iName);
	TInt r = RFile64::Read(aDes);
	test_KErrNone(r);
	return(*this);
	}

RFileTest& RFileTest::Read(TDes8& aDes, TRequestStatus& aStatus) 
//
// Read from the file. Expected not to fail (Asynchronous).
//
	{
	TInt64 size = 0;
	test.Printf(_L("%S read \n"),&iName);
	RFile64::Read(aDes, aStatus);
	User::WaitForRequest(aStatus);
	TInt len = aDes.Length();
	TInt r = RFile64::Size(size);
	test_KErrNone(r);
	if(KFileSizeMaxLargerThan4GBMinusOne == EFalse)
		{
		if(size < K4GB)
			{
			test_KErrNone(aStatus.Int());
			}
		else
			{
			test_KErrNone(aStatus.Int());
			test_Equal(0, len);
			}
		}
	else
		{
		test_KErrNone(aStatus.Int());
		}
	return(*this);
	}

RFileTest& RFileTest::Read(TDes8& aDes,TInt aLen,TRequestStatus& aStatus) 
//
// Read from the file. Expected not to fail (Asynchronous).
//
	{
	TInt64 size = 0;
	test.Printf(_L("%S read \n"),&iName);
	RFile64::Read(aDes,aLen,aStatus);
	User::WaitForRequest(aStatus);
	TInt len = aDes.Length();
	TInt r = RFile64::Size(size);
	test_KErrNone(r);
	if(KFileSizeMaxLargerThan4GBMinusOne == EFalse)
		{
		if(size < K4GB)
			{
			test_KErrNone(aStatus.Int());
			}
		else
			{
			test_KErrNone(aStatus.Int());
			test_Equal(0, len);
			}
		}
	else
		{
		test_KErrNone(aStatus.Int());
		}	
	return(*this);
	}
	
RFileTest& RFileTest::Read(TDes8 &aDes, TInt aLen)
//
// Read from the file. Expected not to fail (Synchronous).
//
	{
	test.Printf(_L("%S read 0x%08x bytes\n"),&iName,aLen);
	TInt r = RFile64::Read(aDes,aLen);
	if (aLen < 0)
		{
		test_Equal(KErrArgument, r);
		}
	else
		{
		test_KErrNone(r);
		}
	return(*this);
	}
	
RFileTest& RFileTest::Read(TInt64 aPos, TDes8& aDes, TInt aLen) 
//
// Read from the file. Expected not to fail (Synchronous).
//
	{
	test.Printf(_L("%S read   0x%lx-0x%lx\n"),&iName,aPos,aPos+aLen-1);
	TInt r = RFile64::Read(aPos,aDes,aLen);
	TInt len = aDes.Length();
	if (aLen < 0)
		{
		test_Equal(KErrArgument, r);
		}
	else
		{
		test_KErrNone(r);
		}
	if(KFileSizeMaxLargerThan4GBMinusOne == EFalse && aPos >= K4GB)
		{	
		test_Equal(0, len);
		}
	return(*this);
	}

RFileTest& RFileTest::ReadE(TInt64 aPos, TDes8& aDes, TInt aLen) 
//
// Reads the specified number of bytes from the file at a specified offset. Expected to fail.
//
	{
	test.Printf(_L("%S readE  0x%lx-0x%lx\n"),&iName,aPos,aPos+aLen-1);
	TInt r = RFile64::Read(aPos,aDes,aLen);
	test_Value(r, r == KErrLocked);
	return(*this);
	}

RFileTest& RFileTest::Read(TInt64 aPos, TDes8& aDes, TInt aLen, TRequestStatus& aStatus) 
//
// Reads the specified number of bytes from the file at a specified offset, Expected not to fail (Asynchronous).
//
	{
	test.Printf(_L("%S read   0x%lx-0x%lx\n"),&iName,aPos,aPos+aLen-1);
	RFile64::Read(aPos,aDes,aLen,aStatus);
	User::WaitForRequest(aStatus);
	TInt len = aDes.Length();
	if(aLen < 0)
		{
		test_Equal(KErrArgument, aStatus.Int());
		}
	else
		{
		test_KErrNone(aStatus.Int());
		}
	if(KFileSizeMaxLargerThan4GBMinusOne == EFalse && aPos >= K4GB)
		{
		test_Equal(0, len);	
		}
	return(*this);
	}

RFileTest& RFileTest::ReadP(TInt64 aPos, TDes8& aDes)
//
// Reads from the file at the specfied offset with in the file (Synchronous).
//
	{
	test.Printf(_L("%S read   0x%lx\n"),&iName,aPos);
	TInt r = RFile64::Read(aPos,aDes);
	test_KErrNone(r);
	return(*this);	
	}

RFileTest& RFileTest::ReadU(TUint aPos, TDes8& aDes)
//
// Reads from the file at the specfied offset with in the file (Synchronous).
// Offset is specified as a TUint value.
//
	{
	test.Printf(_L("%S read   0x%lx\n"),&iName,aPos);
	TInt r = RFile64::Read(aPos,aDes);
	test_KErrNone(r);
	return(*this);	
	}

RFileTest& RFileTest::ReadU(TUint aPos, TDes8& aDes,TRequestStatus& aStatus)
//
// Reads from the file at the specfied offset with in the file (Asynchronous).
// Offset is specified as a TUint value.
//
	{
	test.Printf(_L("%S read   0x%lx\n"),&iName,aPos);
	RFile64::Read(aPos,aDes,aStatus);
	User::WaitForRequest(aStatus);
	test_KErrNone(aStatus.Int());
	return(*this);
	}

RFileTest& RFileTest::ReadU(TUint aPos, TDes8& aDes, TInt aLen) 
//
// Read from the file. Expected not to fail (Synchronous).
// Offset is specified as a TUint value.
//
	{
	test.Printf(_L("%S read   0x%lx-0x%lx\n"),&iName,aPos,aPos+aLen-1);
	TInt r = RFile64::Read(aPos,aDes,aLen);
	if (aLen < 0)
		{
		test_Equal(KErrArgument, r);
		}
	else
		{
		test_KErrNone(r);
		}
	return(*this);
	}

RFileTest& RFileTest::ReadU(TUint aPos, TDes8& aDes, TInt aLen, TRequestStatus& aStatus) 
//
// Reads the specified number of bytes from the file at a specified offset, Expected not to fail (Asynchronous).
// Offset is specified as a TUint value.
//
	{
	test.Printf(_L("%S read   0x%lx-0x%lx\n"),&iName,aPos,aPos+aLen-1);
	RFile64::Read(aPos,aDes,aLen,aStatus);
	User::WaitForRequest(aStatus);
	if(aLen < 0)
		{
		test_Equal(KErrArgument, aStatus.Int());
		}
	else
		{
		test_KErrNone(aStatus.Int());
		}
	return(*this);
	}

	
RFileTest& RFileTest::Read(TInt64 aPos, TDes8& aDes, TRequestStatus& aStatus)
//
// Reads from the file at the specfied offset with in the file (Asynchronous).
//
	{
	test.Printf(_L("%S read   0x%lx\n"),&iName,aPos);
	RFile64::Read(aPos,aDes,aStatus);
	User::WaitForRequest(aStatus);
	test_KErrNone(aStatus.Int());
	return(*this);
	}
	
RFileTest& RFileTest::SetSize(TInt64 aSize)
//
// Set the size of the file. Expected not to fail.
//
	{
	test.Printf(_L("%S size: 0x%lx\n"),&iName,aSize);
	TInt r = RFile64::SetSize(aSize);
	if(KFileSizeMaxLargerThan4GBMinusOne == EFalse)
		{
		if(aSize < K4GB)
			{
			test_KErrNone(r);
			}
		else
			{
			test_Value(r, r == KErrNotSupported);
			}
		}
	else
		{
		test_KErrNone(r);
		}
	return(*this);
	}

RFileTest& RFileTest::SetSizeE(TInt64 aSize)
//
// Set the size of the file. Expected to fail.
//
	{
	test.Printf(_L("%S sizeE: 0x%lx\n"),&iName,aSize);
	TInt r = RFile64::SetSize(aSize);
	test_Value(r, r == KErrLocked);
	return(*this);
	}

RFileTest& RFileTest::Size(TInt64& aSize)
//
// Gets the current file size. Expected not to fail.
//
	{
	TInt r = RFile64::Size(aSize);
	test.Printf(_L("%S size: 0x%lx\n"),&iName,aSize);

    if(KFileSizeMaxLargerThan4GBMinusOne == EFalse)
		{
		if(aSize < K4GB)
			{
			test_KErrNone(r);
			}
		else
			{
			test_Value(r, r == KErrTooBig);
			}
		}
	else
		{
		test_KErrNone(r);
		}
	return(*this);	

	}
RFileTest& RFileTest::Seek(TSeek aMode, TInt64& aPos)
//
// Sets the current file position. Expected not to fail.
//
	{
	test.Printf(_L("Seek to pos %LD in %d Mode\n"),aPos, aMode);
	TInt r = RFile64::Seek(aMode, aPos);
	if (aPos < 0)
		{
		test_Equal(KErrArgument, r);
		}
	else
		{
		test_KErrNone(r);
		}
	return(*this);	
	}

/**
@SYMTestCaseID      PBASE-T_FILE64BIT-0756
@SYMTestPriority    High
@SYMTestRequirement REQ9531 
@SYMTestType        CIT
@SYMTestCaseDesc    Test opening a large file = 2GB in size
@SYMTestActions     
1) Gets the entry details for a file using RFs::Entry(). The original file size=2GB
2) Open a large file whose size = 2GB, with File Mode = EFileRead
3) Close the file
@SYMTestExpectedResults
1) File size = 2GB
2) KErrNone, File open successful
3) File closed successfully
@SYMTestStatus      Implemented
*/
void TestOpen2GB()
	{
	TEntry entry;
	TInt64 testSize, size = 0;
	TFileName fileName;
	fileName.Append(gDriveToTest);
	fileName.Append(KTestPath);
	fileName.Append(_L("File2GB.txt"));
	
	testSize = K2GB;
	
	test.Next(_L("Create the file using RFile64::Replace and set the size and close"));
	TestRFile1.Replace(fileName);
	TestRFile1.SetSize(testSize);
	TestRFile1.Close();
	
	
	test.Next(_L("2GB File: Open"));
	TInt r = TheFs.Entry(fileName, entry);
	test_KErrNone(r);
	test_Equal(testSize, (TUint) entry.iSize);

    TestRFile1.Open(fileName, EFileRead);
	
	
	TestRFile1.Size(size);
	test_Equal(testSize, size);
	
	TestRFile1.Close();
	r = TheFs.Delete(fileName);
	test_KErrNone(r);
	}

/**
@SYMTestCaseID      PBASE-T_FILE64BIT-0757
@SYMTestPriority    High
@SYMTestRequirement REQ9531 
@SYMTestType        CIT
@SYMTestCaseDesc    Test opening a large file = 3GB in size
@SYMTestActions     
1) Gets the entry details for a file using RFs::GetEntry(). The original file size=3GB
2) Open a large file whose size = 3GB, with File Mode = EFileRead
3) Close the file
@SYMTestExpectedResults
1) File size = 3GB
2) KErrNone, File open successful
3) File closed successfully
@SYMTestStatus      Implemented
*/
void TestOpen3GB()
	{
	TInt r;
	TEntry entry;
	TInt64 testSize, size = 0;
	TFileName fileName;
	fileName.Append(gDriveToTest);
	fileName.Append(KTestPath);
	fileName.Append(_L("File3GB.txt"));
	testSize = K3GB;
	
	test.Next(_L("Create the file using RFile64::Replace and set the size and close"));
	TestRFile1.Replace(fileName);
	TestRFile1.SetSize(testSize);
	TestRFile1.Close();
		
	test.Next(_L("3GB File: Open"));
	r = TheFs.Entry(fileName, entry);
	test_KErrNone(r);
	test_Equal(testSize, (TUint) entry.iSize);
	
	TestRFile1.Open(fileName,EFileRead);
	
	TestRFile1.Size(size);
	test_Equal(testSize, size);
	TestRFile1.Close();
	
	r = TheFs.Delete(fileName);
	test_KErrNone(r);
	}

/**
@SYMTestCaseID      PBASE-T_FILE64BIT-0758
@SYMTestPriority    High
@SYMTestRequirement REQ9531 
@SYMTestType        CIT
@SYMTestCaseDesc    Test opening a large file < 4GB in size
@SYMTestActions     
1) Gets the entry details for a file using RFs::GetEntry(). The original file size=4GB-1
2) Open a large file whose size = 4GB-1, with File Mode = EFileRead
3) Close the file
@SYMTestExpectedResults
1) File size = 4GB-1
2) KErrNone, File open successful
3) File closed successfully
@SYMTestStatus      Implemented
*/
void TestOpen4GBMinusOne()
	{
	TInt r;
	TEntry entry;
	TInt64 testSize, size = 0;
	TFileName fileName;
	fileName.Append(gDriveToTest);
	fileName.Append(KTestPath);
	fileName.Append(_L("File4GBMinusOne.txt"));
	testSize = K4GB-1;
	
	test.Next(_L("Create the file using RFile64::Replace and set the size and close"));
	TestRFile1.Replace(fileName);
	TestRFile1.SetSize(testSize);
	TestRFile1.Close();
	
	test.Next(_L("4GB-1 File: Open"));
	r = TheFs.Entry(fileName, entry);
	test_KErrNone(r);
	
	test_Equal(testSize, (TUint) entry.iSize);
	
	TestRFile1.Open(fileName, EFileRead);
		
	TestRFile1.Size(size);
		
	test_Equal(testSize, size);
	TestRFile1.Close();
	
	r = TheFs.Delete(fileName);
	test_KErrNone(r);
	}
	
/**
@SYMTestCaseID      PBASE-T_FILE64BIT-0759
@SYMTestPriority    High
@SYMTestRequirement REQ9531 
@SYMTestType        CIT
@SYMTestCaseDesc    Test opening a large file 4GB in size
@SYMTestActions     
1) Gets the entry details for a file using RFs::GetEntry(). The original file size=4GB
2) Open a large file whose size = 4GB, with File Mode = EFileRead
3) Close the file
@SYMTestExpectedResults
1) File size = 4GB
2) KErrNone, File open successful
3) File closed successfully
@SYMTestStatus      Implemented
*/
void TestOpen4GB()
	{
	TInt r;
	TEntry entry;
	TInt64 testSize, size = 0;
	TFileName fileName;
	fileName.Append(gDriveToTest);
	fileName.Append(KTestPath);
	fileName.Append(_L("File4GB.txt"));
	testSize = K4GB;

	test.Next(_L("Create the file using RFile64::Replace and set the size and close"));
	TestRFile1.Replace(fileName);
	TestRFile1.SetSize(testSize);
	TestRFile1.Close();
	
	test.Next(_L("4GB File: Open"));
	r = TheFs.Entry(fileName, entry);
	test_KErrNone(r);
	
	if ((TUint) entry.iSize == testSize)
		{
		TestRFile1.Open(fileName, EFileRead);
		TestRFile1.Size(size);
		test_Equal(testSize, size);
		TestRFile1.Close();
		}
	
	r = TheFs.Delete(fileName);
	test_KErrNone(r);
		
	}	
	
/**
@SYMTestCaseID      PBASE-T_FILE64BIT-0760
@SYMTestPriority    High
@SYMTestRequirement REQ9531 
@SYMTestType        CIT
@SYMTestCaseDesc    Tests opening a large file > 2GB in size
@SYMTestActions     
1) Create a new file named "File4GBMinusOne.txt"
2) Open the file with file mode = EFileWrite
3) Set the file size to 4GB-1
4) Write few bytes to the file and close
5) Close the file
6) Open the file "File4GBMinusOne.txt"
7) If FAT32 file system, set the file size to 4GB
8) Close the file
9) Open the file with file mode = EDeleteOnClose
@SYMTestExpectedResults
1) File creation successful with KErrNone
2) File open successful with KErrNone
3) KErrNone, Sets the file size to 4GB-1
4) KErrNone, write is successful and file closed successfully
5) File closed successfully
6) KErrNone, file open successful
7) KErrNotSupported. For next generation file system KErrNone is expected
8) File closed successfully
9) File open failed with KErrArgument
@SYMTestStatus      Implemented
*/
void TestOpenMoreThan2GB()
	{
	// constants and literals
	test.Next(_L("Test Files of size more than 2GB\n"));
	
	TInt64 			size;
	TBuf8<KBUFSIZE> readBuf;
	TFileName fileName;
	fileName.Append(gDriveToTest);
	fileName.Append(KTestPath);
	fileName.Append(_L("File4GBMinusOne.txt"));
	
	test.Start(_L("Test to create a large file > 2GB\n"));
	
	TestRFile1.Replace(fileName);
	test.Next(_L("Set the file size to 4GB-1\n"));
	
	size = K4GBMinusOne;
	TestRFile1.SetSize(size);
	
	TBuf8<10> writeBuf;
	writeBuf.Zero();
	for(TInt count = 0; count < 10; count++)
		{
		writeBuf.Append(count);
		}
		
   	test.Next(_L("Write 10 bytes to the file\n"));
	TestRFile1.Write(0, writeBuf, 10);
	test.Next(_L("Read 10 bytes from position 0\n"));
	TestRFile1.Read(0, readBuf, 10);
	test(writeBuf == readBuf);
	
	TInt64 s;
	TestRFile1.Size(s);
	if(s < K4GB)
		{
		test.Printf(_L("\nFile size is less than 4 GB !!!!\n"));	
		}

	TestRFile1.Close();
		
	test.Next(_L("Open the file File4GBMinusOne.txt\n"));
	TestRFile1.Open(fileName);
	
	if(KFileSizeMaxLargerThan4GBMinusOne == EFalse)	
		{
		test.Next (_L("Set the file size to 4GB\n"));
		size = K4GB;
		TestRFile1.SetSize(size);
		}
	TestRFile1.Close();
	
	RFile64 file64;
	TInt r = file64.Open(TheFs,fileName,EDeleteOnClose);
	test_Value(r, r == KErrArgument);
	
	r = TheFs.Delete(fileName);
	test_KErrNone(r);
	
	}

/**
@SYMTestCaseID      PBASE-T_FILE64BIT-0761
@SYMTestPriority    High
@SYMTestRequirement REQ9531 
@SYMTestType        CIT
@SYMTestCaseDesc    Tests opening a file using RFile and RFile64 in file sharing mode
@SYMTestActions     
1) Create a file using RFile::Replace()
2) Open the file using RFile::Open()  and file mode = EFileShareAny
3) Write 100 bytes to the file and close the file
4) Open the same file using RFile64::Open() and file mode = EFileShareAny
5) Set the file size to 4GB-1 using RFile64::SetSize().
6) Get the file size using RFile::Size()
7) Seek to the file position 2GB+5 using RFile::Seek()
8) Get the file size using RFile64::Size()
9) Seek to the file position 4GB-10 using RFile64::Seek()
10) Read from the file position 4GB-10 using RFile::Read() of length 5 bytes
11) Close the file.
12) Open the file using RFile::Open().
13) Open the file using RFile64::Open() and close the file.
@SYMTestExpectedResults
1) File created successful with KErrNone.
2) File opened successfully with KErrNone.
3) Write successful with KErrNone.
4) File opened successfully with KErrNone.
5) File size set successfully with KErrNone.
6) Fail with KErrNotSupported.
7) Seek operation fail with KErrArgument.
8) FileSize == 4GB-1.
9) KErrNone.
10) Read fail with KErrNotSupported.
11) File closed successfully.
12) File Open failed with KErrTooBig.
13) File open successfully with KErrNone and file closed successfully.
@SYMTestStatus      Implemented
*/
void TestOpenRFileRFile64()
	{
	RFile file;
	TInt size;
	TInt64 size64;
	TInt count;
	TFileName fileName;
	fileName.Append(gDriveToTest);
	fileName.Append(KTestPath);
	fileName.Append(_L("File4GBMinusOne.txt"));
	
	test.Start(_L("Test opening a file using RFile and RFile64 in file sharing mode\n"));
	TInt r = file.Replace(TheFs,fileName,EFileShareAny|EFileWrite);
	test_KErrNone(r);
	
	TBuf8<100> writeBuf;
	TBuf8<100> readBuf;
	writeBuf.Zero();
	for(count = 0; count < 100; count++)
		{
		writeBuf.Append(count);
		}
		
   	test.Next(_L("Write 100 bytes to the file\n"));
	r = file.Write(0, writeBuf, 100);	
	test_KErrNone(r);
	
	test.Next(_L("Read 100 bytes from position 0"));
	r = file.Read(0, readBuf, 100); 
	test_KErrNone(r);
	
	test.Next(_L("Compare the read data to the written data"));
	test(readBuf == writeBuf);

	
	test.Next(_L("Open the same file using RFile64::Open"));
	TestRFile1.Open(fileName,EFileShareAny|EFileWrite);
	
	test.Next(_L("Set the file size to 4GB-1\n"));
	TestRFile1.SetSize(K4GBMinusOne);
	
	test.Next(_L("Query the file size using Rfile::Size()\n"));
	r = file.Size(size);
	test_Value(r, r == KErrTooBig);
	
	test.Next(_L("Seek to the file position using 2GB+5 using RFile::Seek()\n"));
	TUint seekPos1 = K2GB + 5;
	TInt seekPos  = (TInt)seekPos1;
	r = file.Seek(ESeekStart,seekPos);
	test_Value(r, r == KErrArgument);
	
	test.Next(_L("Get the file size using RFile64::Size()\n"));
	TestRFile1.Size(size64);
	
	test.Next(_L("Seek to the file position 4GB-10 using RFile64::Seek()\n"));
	TInt64 seekPos64 = K4GB - 10;
	TestRFile1.Seek(ESeekStart,seekPos64);
	
	TBuf8<5> writeBuf64;
	TBuf8<5> readBuf64;
	writeBuf64.Zero();
	for(count = 0; count < 5; count++)
		{
		writeBuf64.Append(count);
		}
	
	test.Next(_L("Read from the file position 4GB-10 using RFile::Read() of length 5 bytes\n"));
	TestRFile1.Write(seekPos64,writeBuf64,5);
	TestRFile1.Seek(ESeekStart,seekPos64);
	TestRFile1.Read(seekPos64,readBuf64,5);
	test(readBuf64 == writeBuf64);
	
	TestRFile1.Close();
	file.Close();
	
	test.Next(_L("Open the file using Rfile::Open()\n"));
	r = file.Open(TheFs,fileName,EFileShareAny|EFileWrite);
	test_Value(r, r == KErrTooBig);
	
	test.Next(_L("Open the file using Rfile64::Open() and close\n"));
	TestRFile1.Open(fileName,EFileShareAny|EFileWrite);
	TestRFile1.Close();
	
	r = TheFs.Delete(fileName);
	test_KErrNone(r);
	}

/**
@SYMTestCaseID      PBASE-T_FILE64BIT-0762
@SYMTestPriority    High
@SYMTestRequirement REQ9531
@SYMTestType        CIT
@SYMTestCaseDesc    Tests the temporary file creation using RFile64::Temp()
@SYMTestActions     
1) Create a Temporary file using RFile64::Temp() in write mode and DeleteOnClose
2) Set the file size to 4GB-1
3) Write 100 bytes to the file at position 2GB+1
4) Write 1 byte to file position 4GB-2
5) Write 10 bytes to file position 0.
6) Write 1 byte to file position 4GB+1
7) Read and compare the data at position 2GB+1,4GB-2,0 and close the file
8) Delete the temporary file.
9) Create a temporary file using RFile64::Temp() in write mode and without DeleteOnClose flag
10) Close the File
11) Delete the temporary file
@SYMTestExpectedResults
1) Temporary file created successfully 
2) File size = 4GB-1
3) Write successful with KErrNone
4) Write successful with KErrNone
5) Write successful with KErrNone
6) Write fail with KErrNotSupported
7) Read data == written data
8) KErrNotFound, since the file is already deleted on close
9) File created successfully
10) File closed
11) File deleted successfully

@SYMTestStatus      Implemented
*/
void TestCreateTempFile()
	{
	TInt count;
	TFileName testDir;
	testDir.Append(gDriveToTest);
	testDir.Append(KTestPath);
	
	TInt r = TheFs.MkDir(testDir);
	test_Value(r, r == KErrNone || r == KErrAlreadyExists);
	
	TFileName fileName;
	TestRFile1.Temp(testDir, fileName, EFileWrite|EDeleteOnClose);
	
	test.Next(_L("Set the file size to 4GB-1\n"));
	TestRFile1.SetSize(K4GBMinusOne);
	
	TInt64 size = 0;
	TestRFile1.Size(size);
	test (size == K4GBMinusOne);
	
	TBuf8<0x64> writeBuf;
	TBuf8<0x64> readBuf;
	writeBuf.Zero();
	for(count = 0; count < 100; count++)
		{
		writeBuf.Append(count);
		}
	TInt64 seekPos = K2GB + 1;
	test.Next(_L("Write 100 bytes to the file at position 2GB+1\n"));
	TestRFile1.Write(seekPos, writeBuf, 100);	
	test.Next(_L("Read 100 bytes from position 2GB+1"));
	TestRFile1.Read(seekPos, readBuf, 100); 
	test(writeBuf == readBuf);
	
	test.Next(_L("Write 1 byte to the file at position 4GB-2\n"));
	TBuf8<01> writeBuf1Byte;
	TBuf8<01> readBuf1Byte;
	writeBuf1Byte.Zero();
	writeBuf1Byte.Append(0);
	seekPos = K4GBMinusTwo;
	TestRFile1.Write(seekPos, writeBuf1Byte, 1);	
	
	test.Next(_L("Read 1 byte from position 4GB-2"));
	seekPos = K4GBMinusTwo;
	TestRFile1.Read(seekPos, readBuf1Byte, 1); 
	test(writeBuf1Byte == readBuf1Byte);
	
	test.Next(_L("Write 10 bytes to the file at position 0\n"));
	TBuf8<10> writeBuf10Byte;
	TBuf8<10> readBuf10Byte;
	writeBuf10Byte.Zero();
	for(count = 0; count < 10; count++)
		{
		writeBuf10Byte.Append(count);
		}
	TestRFile1.Write(0, writeBuf10Byte, 10);	
	
	test.Next(_L("Read 10 byte from position 0"));
	TestRFile1.Read(0, readBuf10Byte, 10); 
	test(writeBuf10Byte == readBuf10Byte);
	
	test.Next(_L("Write 1 byte to the file at position 4GB+1\n"));
	seekPos = K4GB + 1;
	TestRFile1.Write(seekPos, writeBuf1Byte, 1);
	
	TestRFile1.Close();
	
	test.Next(_L("Delete the temporary file\n"));
	r = TheFs.Delete(fileName);
	test_Value(r, r == KErrNotFound);
	
	test.Next(_L("Create a temporary file using RFile64::Temp without EDeleteOnClose flag\n"));
	TestRFile1.Temp(testDir, fileName, EFileWrite);
	
	test.Next(_L("Close the file\n"));	
	TestRFile1.Close();
	
	test.Next(_L("Delete the temporary the file\n"));	
	r = TheFs.Delete(fileName);
	test_KErrNone(r);
	
	}

/**
@SYMTestCaseID      PBASE-T_FILE64BIT-0763
@SYMTestPriority    High
@SYMTestRequirement REQ9531 
@SYMTestType        CIT
@SYMTestCaseDesc    Tests the file creation using RFile64::Create()
@SYMTestActions     
1) Create a file FileLargeOne.txt in write mode.
2) Set the file size to 3GB-4KB
3) Seek the file: Mode = ESeekEnd
4) Write to a file with current position and length =4KB
5) Get the file size.
6) Write to a file at position 0 and length = 100 bytes.
7) Write to a file at position 4GB -2 and length = 1 byte
8) Write to a file at position 4GB -2 and length = 3 byte
9) Read and compare the data written at position 0, 4GB-1
10) Close the File.
11) Create the file FileLargeOne.txt in write mode.
12) Create a file with invalid path and file name.
@SYMTestExpectedResults
1) File created successfully with KErrNone 
2) File size = 3GB-4KB
3) KErrNone
4) Write successful with KErrNone
5) File size == 3GB
6) Write successful with KErrNone
7) Write successful with KErrNone
8) Write fails with KErrNotSupported.
9) Read data == written data.
10) File closed successfully.
11) File creation failed with KErrAlreadyExists
12) File Creation failed with KErrPathNotFound.
@SYMTestStatus      Implemented
*/
void TestCreateRFile64()
	{
	TInt count;
	TFileName fileName;
	fileName.Append(gDriveToTest);
	fileName.Append(KTestPath);
	fileName.Append(_L("FileLargeOne.txt"));
	
	test.Next(_L("create a file named FileLargeOne.txt\n"));
	TestRFile1.Create(fileName, EFileWrite);
	
	test.Next(_L("set the file size to 3GB - 4KB\n"));
	TestRFile1.SetSize(K3GB-K4KB);
	
	TInt64 size = 0;
	TestRFile1.Size(size);
	test (size == K3GB-K4KB);
	
	test.Next(_L("seek to the end of the file\n"));
	TInt64 seekPos = 0;
	TestRFile1.Seek(ESeekEnd,seekPos);
	test(seekPos == K3GB-K4KB);	
	
	test.Next(_L("write to the file current position and length = 4KB\n"));
	TBuf8<4096> writeBufK4KB;
	TBuf8<4096> readBufK4KB;
	for (count = 0; count < 4096; count++)
		{
		writeBufK4KB.Append(count+1);
		}
	
	TestRFile1.Write(writeBufK4KB,K4KB);
	
	test.Next(_L("read from the file from position K3GB-K4KB and length = 4KB\n"));
	seekPos = K3GB - K4KB;
	TestRFile1.Read(seekPos,readBufK4KB,K4KB);
	test(writeBufK4KB == readBufK4KB);
	
	test.Next(_L("get the size of the file\n"));
	size = 0;
	TestRFile1.Size(size);
	test(size == K3GB);
	
	test.Next(_L("write to the file at position 0 and length = 100bytes\n"));
	TBuf8<0x64> writeBuf100B;
	TBuf8<0x64> readBuf100B;
	writeBuf100B.Zero();
	for(count = 0; count < 100; count++)
		{
		writeBuf100B.Append(count);
		}
	seekPos = 0;
	TestRFile1.Write(seekPos, writeBuf100B, 100);	
	
	test.Next(_L("Read 100 bytes from position 0"));
	TestRFile1.Read(seekPos, readBuf100B, 100); 
	test(writeBuf100B == readBuf100B);
	
	test.Next(_L("Write 1 byte to the file at position 4GB-2\n"));
	TBuf8<01> writeBuf1Byte;
	TBuf8<01> readBuf1Byte;
	writeBuf1Byte.Zero();
	writeBuf1Byte.Append(0);
	seekPos = K4GBMinusTwo;
	TestRFile1.SetSize(K4GB-1);
	TestRFile1.Write(seekPos, writeBuf1Byte, 1);	
	
	test.Next(_L("Read 1 byte from position 4GB-2"));
	seekPos = K4GBMinusTwo;
	TestRFile1.Read(seekPos, readBuf1Byte, 1); 
	test(writeBuf1Byte == readBuf1Byte);
	
	test.Next(_L("Write 3 bytes to the file at position 4GB-1\n"));
	TBuf8<3> writeBuf3Byte;
	
	writeBuf3Byte.Zero();
	for(count = 0; count < 3; count++)
		{
		writeBuf3Byte.Append(count);
		}
	seekPos = K4GBMinusTwo;
	TestRFile1.Write(seekPos, writeBuf1Byte, 3);	
	
	TestRFile1.Close();
	
	test.Next(_L("create a file named FileLargeOne.txt(KErrAlreadyExists)\n"));
	TestRFile1.Create(fileName,EFileWrite);
	
	test.Next(_L("create a file with InvalidPath and fileName\n"));	
	RFile64 file64;
	TInt r = file64.Create(TheFs, _L("C:\\InvalidPathName\\FileName"),EFileWrite);
	test_Value(r, r == KErrPathNotFound);
	
	r = TheFs.Delete(fileName);
	test_KErrNone(r);
	}	
	
/**
@SYMTestCaseID      PBASE-T_FILE64BIT-0764
@SYMTestPriority    High
@SYMTestRequirement REQ9531 
@SYMTestType        CIT
@SYMTestCaseDesc    Tests the file creation using RFile64::Replace()
@SYMTestActions     
1) Replace a file FileLargeOne.txt in write mode using RFile64::Replace.
2) Set the file size to 4GB-1
3) Write to a file with position = 4GB-4KB-2 and length = 4KB
4) Get the file size
5) Seek the file: Mode = ESeekEnd,pos = 0.
6) Write to a file with current position, length = 1 byte
7) Seek the file: Mode = ESeekStart
8) Write to a file with current position and length = 4KB
9) Seek the file: Mode = ESeekEnd
10)Read from the current position and length = 1 byte and compare with written data
11)Seek the file: Mode = ESeekStart
12)Read the data from the current position and length = 4KB and compare with written data
13)Close the file
14)Replace a file FileLargeOne.txt in write mode
15)Get the file size
16)Close the file.
17)Replace a file FileLargeOne.txt with invalid path
@SYMTestExpectedResults
1) File created successfully with KErrNone 
2) File size = 4GB-1
3) Write successful with KErrNone
4) File size = 4GB-1
5) KErrNone
6) Write successful with KErrNone
7) KErrNone
8) Write successful with KErrNone
9) KErrNone
10)Written data == Read data
11)KErrNone
12)Written data == Read data
13)File Closed
14)File creatd successfully with KErrNone
15)File size = 0
16)File Closed
17)File creation failed with KErrPathNotFound.
@SYMTestStatus      Implemented
*/
void TestReplaceRFile64()
	{
	TFileName fileName;
	fileName.Append(gDriveToTest);
	fileName.Append(KTestPath);
	fileName.Append(_L("FileLargeOne.txt"));
	
	test.Next(_L("Replace a file named FileLargeOne.txt\n"));
	TestRFile1.Replace(fileName, EFileWrite);
	
	test.Next(_L("Set the size of the file to 4GB-1\n"));
	TestRFile1.SetSize(K4GB-1);

	TBuf8<4096> writeBufK4KB;
	TBuf8<4096> readBufK4KB;	
	for (TInt count = 0; count < 4096; count++)
		{
		writeBufK4KB.Append(count+1);
		}
	
	test.Next(_L("Write to a file with position = 4GB-4KB-2 and length = 4KB\n"));
	TInt64 pos = K4GB-K4KB-2;
	TestRFile1.Write(pos,writeBufK4KB,K4KB);
	
	test.Next(_L("Read from 4GB-4KB-1 and compare data\n"));	
	TestRFile1.Read(pos,readBufK4KB,K4KB);
	test(writeBufK4KB == readBufK4KB);
	
	test.Next(_L("Get the file size\n"));
	TInt64 size = 0;
	TestRFile1.Size(size);
	test (size == K4GB-1);
	
	test.Next(_L("Seek the file: Mode = ESeekEnd,pos = 0.\n"));
	TInt64 seekPos = 0;
	TestRFile1.Seek(ESeekEnd,seekPos);
	test(seekPos == K4GB-1);
		
	test.Next(_L("Write to a file with current position, length = 1 byte\n"));
	TBuf8<1> writeBuf1B(_L8("0"));
	TBuf8<1> readBuf1B;
	
    if(!KFileSizeMaxLargerThan4GBMinusOne)
        seekPos--;

    TestRFile1.Write(seekPos,writeBuf1B,1); //-- now seek pos is K4GB
	
	
	test.Next(_L("Seek the file: Mode = ESeekStart\n"));	
	seekPos = 0;
	TestRFile1.Seek(ESeekStart,seekPos);
	
	test.Next(_L("Write to a file with current position and length = 4KB\n"));	
	TestRFile1.Write(seekPos,writeBufK4KB,K4KB);
	
	test.Next(_L("Seek the file: Mode = ESeekEnd\n"));	
	seekPos = 0;
	TestRFile1.Seek(ESeekEnd,seekPos);
    
    if(KFileSizeMaxLargerThan4GBMinusOne)
    {//-- file is larger than 4G-1
        test(seekPos == K4GB);
    }
	else
    {
        test(seekPos == K4GB-1);
 	}
	
    seekPos--;
    

	test.Next(_L("Read from pos = 4GB-1 and compare data\n"));	
	TestRFile1.Read(seekPos,readBuf1B,1);
	test(writeBuf1B == readBuf1B);	
	
	test.Next(_L("Seek the file: Mode = ESeekStart\n"));	
	seekPos = 0;
	TestRFile1.Seek(ESeekStart,seekPos);
	
	test.Next(_L("Read from the file and compare written data\n"));	
	TestRFile1.Read(seekPos,readBufK4KB,K4KB);
	test (writeBufK4KB == readBufK4KB);
	
	test.Next(_L("Close the file\n"));	
	TestRFile1.Close();

	test.Next(_L("Replace a file FileLargeOne.txt in write mode\n"));
	TestRFile1.Replace(fileName, EFileWrite);
	
	test.Next(_L("Get the file size\n"));
	size = 0;
	TestRFile1.Size(size);
	test (size == 0);
	
	test.Next(_L("Close the file\n"));	
	TestRFile1.Close();
	
	test.Next(_L("Replace a file FileLargeOne.txt with invalid path\n"));	
	RFile64 file64;
	TInt r = file64.Replace(TheFs,_L("C:\\InvalidPath\\FileLargeOne.Txt"),EFileWrite);
	test_Value(r, r == KErrPathNotFound);
	
	r = TheFs.Delete(fileName);
	test_KErrNone(r);
	}

/**
@SYMTestCaseID      PBASE-T_FILE64BIT-0765
@SYMTestPriority    High
@SYMTestRequirement REQXXXX 
@SYMTestType        CIT
@SYMTestCaseDesc    Tests the file replace using RFs::Replace()
@SYMTestActions     
1) Create a file named FileLargeOne.txt using RFile64::Replace()
2) Set the file size to 3GB and get the file size
3) Write 10 bytes to location 2GB+10 and close the file
4) Replace the file named ReNameFileLargeOne.txt using RFs::Replace()
5) Open the file ReNameFileLargeOne.txt
6) Set the file size to 4GB-1
7) Write 10 bytes to the location 3GB+10
8) Read the above file from the location 3GB+10
9) Compare the read and the written data
10)Close the file
@SYMTestExpectedResults
1) File created successfully with KErrNone 
2) File size = 3GB
3) Write successful with KErrNone and file closed
4) FileLargeOne.txt is replaced with ReNameFileLargeOne.txt successfully
5) File ReNameFileLargeOne.txt is opened successfully
6) KErrNone
7) Write successful with KErrNone
8) Read is successful with KErrNone
9) Written data == Read data
10)File Closed
@SYMTestStatus      Implemented
*/
void TestReplaceRFile64RFs()
	{
	
	TFileName fileName;
	fileName.Append(gDriveToTest);
	fileName.Append(KTestPath);
	fileName.Append(_L("FileLargeOne.txt"));
	
	test.Next(_L("Replace a file named FileLargeOne.txt\n"));
	TestRFile1.Replace(fileName, EFileWrite);
	
	test.Next(_L("Set the file size to 3GB and get the file size\n"));
	TestRFile1.SetSize(K3GB);
	TInt64 size = 0;
	TestRFile1.Size(size);
	test (size == K3GB);
	
	
	test.Next(_L("Write 10 bytes to location 2GB+10 and close the file\n"));
	TBuf8<10> writeBuf;
	TBuf8<10> readBuf;	
	for (TInt count = 0; count < 10; count++)
		{
		writeBuf.Append(count+1);
		}
	TInt64 pos = K2GB+10;
	TestRFile1.Write(pos,writeBuf,10);
	TestRFile1.Read(pos,readBuf,10);
	test(readBuf == writeBuf);
	TestRFile1.Close();
	
	test.Next(_L("Replace the file named ReNameFileLargeOne.txt using RFs::Replace()\n"));
	TFileName fileNameReplace;
	fileNameReplace.Append(gDriveToTest);
	fileNameReplace.Append(KTestPath);
	fileNameReplace.Append(_L("ReNameFileLargeOne.txt\n"));
	TestRFs.Replace(fileName,fileNameReplace);
	
	test.Next(_L("Open the file ReNameFileLargeOne.txt\n"));
	TestRFile1.Open(fileNameReplace,EFileWrite);
	
	test.Next(_L("Set the file size to 4GB-1\n"));
	TestRFile1.SetSize(K4GB-1);
	size = 0;
	TestRFile1.Size(size);
	test (size == K4GB-1);
	
	test.Next(_L("Write 10 bytes to the location 3GB+10\n"));
	pos = K3GB+10;
	TestRFile1.Write(pos,_L8("ABCDEFGHIJ"),10);
	
	test.Next(_L("Read the above file from the location 3GB+10 and compare\n"));
	TBuf8<10> readBuffer;	
	TestRFile1.Read(pos,readBuffer,10);
	test(readBuffer == _L8("ABCDEFGHIJ"));
	
	test.Next(_L("Close the file and delete\n"));
	TestRFile1.Close();
	TInt r = TheFs.Delete(fileNameReplace);
	test_KErrNone(r);
	}

/**
@SYMTestCaseID      PBASE-T_FILE64BIT-0766
@SYMTestPriority    High
@SYMTestRequirement REQXXXX
@SYMTestType        CIT
@SYMTestCaseDesc    Test the file creation using RFile64::AdoptFromClient()
@SYMTestActions 
1) Connect to the File server 
2) Create a file and set the file size to 4GB-1
3) Write few bytes to the location 4GB-10, length = 9bytes
4) Transfer the file handle using TransferToServer() close the file
5) Adopt the already open file from a client using RFile64::Adopt::AdoptFromClient()
6) Read the file from position 4GB-10 and compare the data
@SYMTestExpectedResults
1) Connection successful
2) File created successfully
3) Write successful with KErrNone
4) KErrNone, Transfer to server is successful
5) successfully Allows the server to adopt an already open file from a client process
6) File read should be successful and Read Data = Test Data
@SYMTestStatus      Implemented
*/
void TestRFile64AdoptFromClient()
	{
	test.Next(_L("Tests for checking RFile64::AdoptFromClient()"));

	RProcess p;
	TInt r = p.Create(_L("FHServer64Bit.exe"), KNullDesC);
	test_KErrNone(r);
	
	
	test.Next(_L("Connect to the File server \n"));
	RFs fs;
	r = fs.Connect();
	test_KErrNone(r);

	// Check the number of open file handles
	TInt resCount = fs.ResourceCount();
	test_Equal(0, resCount);

	r = fs.ShareProtected();
	test_KErrNone(r);

	r = fs.CreatePrivatePath(gDrive);
	test_KErrNone(r);
	r = fs.SetSessionToPrivate(gDrive);
	
	test.Next(_L("Create a file and set the file size to 4GB-1\n"));
	RFile64 file1;
	r = file1.Replace(fs,KClientFileName,EFileWrite);
	test_KErrNone(r);
	r = file1.SetSize(K4GB-1);
	test_KErrNone(r);
	
	test.Next(_L("Write few bytes to the location 4GB-10, length = 9bytes\n"));
	r = file1.Write(K4GB-10,KTestData3(),9);
	test_KErrNone(r);
	file1.Close();

	r = p.SetParameter(3, gDrive);
	test_KErrNone(r);
	
	p.Resume();
	
	
	test.Next(_L("Transfer the file handle using TransferToServer() close the file\n"));
	RFileHandleSharer64Bit handsvr;
	do
		{
		r = handsvr.Connect();
		}
	while(r == KErrNotFound);
	test_KErrNone(r);

	r = handsvr.SetTestDrive(gDrive);
	test_KErrNone(r);

	r = fs.SetSessionToPrivate(gDrive);
	test_KErrNone(r);

	r = file1.Open(fs,KClientFileName,EFileRead);
	test_KErrNone(r);
	
	// pass the file handle to FHServer
	test.Next(_L("RFile::TransferToServer()"));

	TIpcArgs ipcArgs;
	r = file1.TransferToServer(ipcArgs, 0, 1);
	test_KErrNone(r);
	
	test.Next(_L("Adopt the already open file from a client using RFile64::AdoptFromClient()\n"));
	r = handsvr.PassFileHandleProcessLargeFileClient(ipcArgs);
	test_KErrNone(r);

	// verify that the original file handle's position is unchanged
	TInt64 pos = 0;
	r = file1.Seek(ESeekCurrent, pos);
	test_KErrNone(r);
	test_Equal(0, pos);
	// make sure we can still use it

	test.Next(_L("Read the file from position 4GB-10 and compare the data\n"));
	TBuf8<9> rbuf;
	r = file1.Read(K4GB-10,rbuf);
	test_KErrNone(r);
	test (rbuf == KTestData3);

	// Close the file
	file1.Close();	
	handsvr.Exit();
	handsvr.Close();
	r = fs.MkDir(_L("C:\\mdir"));
	test_Value(r, r == KErrNone || r == KErrAlreadyExists);
		
	// Check the number of open file handles
	resCount = fs.ResourceCount();
	test_Equal(0, resCount);

	r = fs.Delete(KClientFileName);
	test_KErrNone(r);
	fs.Close();
	}

/**
@SYMTestCaseID      PBASE-T_FILE64BIT-0767
@SYMTestPriority    High
@SYMTestRequirement REQXXXX
@SYMTestType        CIT
@SYMTestCaseDesc    Test the file creation using RFile64::AdoptFromCreator()
@SYMTestActions 
1) Create a process named "FHServer64Bit.exe"
2) Connect to the File server 
3) Create a file and set the file size to 4GB-1
4) Write few bytes to the location 4GB-10, length = 3 bytes
5) Transfer the file handle using TransferToProcess() close the file
6) Resume the process "FHServer64bit.exe" 
7) Adopts the already open file from a client using RFile64::AdoptFromCreator()
8) Read the file from position 4GB-10 and compare the data
@SYMTestExpectedResults
1) Process is created successfully with KErrnone
2) Connection successful
3) File created successfully
4) Write successful with KErrNone
5) KErrNone, Transfer to other process is successful
6) Server process should be resumed
7) successfully Allows the server to adopt an already open file from a client process
8) File read should be successful and Read Data = Test Data
@SYMTestStatus      Implemented
*/

void TestRFile64AdoptFromCreator()
	{
	TInt r;
	test.Next(_L("Tests for checking RFile64::AdoptFromCreator()"));
	//create test server
	test.Next(_L("Create a process named FHServer64Bit.exe\n"));
	RProcess p;
	r = p.Create(_L("FHServer64Bit.exe"), KNullDesC);
	test_KErrNone(r);
		
	test.Next(_L("Connect to the file server\n"));
	RFs fs;
	r = fs.Connect();
	test_KErrNone(r);

	// Check the number of open file handles
	TInt resCount = fs.ResourceCount();
	test_Equal(0, resCount);

	r = fs.ShareProtected();
	test_KErrNone(r);

	r = fs.CreatePrivatePath(gDrive);
	test_KErrNone(r);
	r = fs.SetSessionToPrivate(gDrive);
	
	test.Next(_L("Create a file and set the file size to 4GB-1\n"));
	RFile64 file1;
	r = file1.Replace(fs,KClientFileName,EFileWrite);
	test_KErrNone(r);
	r = file1.SetSize(K4GB-1);
	test_KErrNone(r);
	
	test.Next(_L("Write few bytes to the location 4GB-10, length = 3bytes\n"));
	r = file1.Write(K4GB-10,KTestData2(),3);
	test_KErrNone(r);
	file1.Close();

	r = file1.Open(fs, KClientFileName, EFileWrite);

	test_KErrNone(r);
	
	// NB slot 0 is reserved for the command line

	test.Next(_L("Transfer the file handle using TransferToProcess() close the file"));

	r = file1.TransferToProcess(p, 1, 2);

	r = p.SetParameter(3, gDrive);
	test_KErrNone(r);

	r = fs.SetSessionToPrivate(gDrive);
	test_KErrNone(r);

	// make sure we can still read from the file
	TBuf8<3> rbuf;
	r = file1.Read(K4GB-10,rbuf,3);
	test_KErrNone(r);
	r = rbuf.CompareF(KTestData2());
	test_KErrNone(r);
	file1.Close();

	r = fs.MkDir(_L("C:\\mdir"));
	test_Value(r, r == KErrNone || r == KErrAlreadyExists);
	
	// Check the number of open file handles - 
	// should be 1 (the one duplicated for the other process)
	resCount = fs.ResourceCount();
	test_Equal(1, resCount);

	fs.Close();

	test.Next(_L("Resume the process FHServer64bit.exe "));
// Start the server thread
	p.Resume();

// connect to the server
	RFileHandleSharer64Bit handsvr;
	do
		{
		r = handsvr.Connect();
		}
	while(r == KErrNotFound);
	test_KErrNone(r);
	r = handsvr.SetTestDrive(gDrive);
	test_KErrNone(r);

	// wait for server to read the file
	r = handsvr.PassFileHandleProcessLargeFileCreator();
	test_KErrNone(r);
	
	
	// cleanup	
	handsvr.Exit();
	handsvr.Close();
	p.Close();
	}

/**
@SYMTestCaseID      PBASE-T_FILE64BIT-0768
@SYMTestPriority    High
@SYMTestRequirement REQXXXX
@SYMTestType        CIT
@SYMTestCaseDesc    Test the file creation using RFile64::AdoptFromServer()
@SYMTestActions 
1) Connect to the File server 
2) Create a file and set the file size to 4GB-1
3) Write few bytes to the location 4GB-10, length = 9bytes
4) Adopt an already open file from a server using RFile64::AdoptFromServer()
5) Read the file from position 4GB-10 and compare the data
@SYMTestExpectedResults
1) Connection successful
2) File created successfully
3) Write successful with KErrNone
4) successfully Allows the client to adopt an already open file from a server process
5) File read should be successful and Read Data = Test Data
@SYMTestStatus      Implemented
*/

void TestRFile64AdoptFromServer()
	{
	
	test.Next(_L("Tests for checking RFile64::AdoptFromServer()"));
	TInt r;
	
	test.Next(_L("Connect to the file server\n"));
	RFs fs;
	r = fs.Connect();
	test_KErrNone(r);

	// Check the number of open file handles
	TInt resCount = fs.ResourceCount();
	test_Equal(0, resCount);

	r = fs.ShareProtected();
	test_KErrNone(r);

	r = fs.CreatePrivatePath(gDrive);
	test_KErrNone(r);
	r = fs.SetSessionToPrivate(gDrive);
	
	test.Next(_L("Create a file and set the file size to 4GB-1\n"));
	RFile64 file1;
	r = file1.Replace(fs,KClientFileName,EFileWrite);
	test_KErrNone(r);
	r = file1.SetSize(K4GB-1);
	test_KErrNone(r);
	
	
	r = file1.Write(K4GB-10,KTestData3(),9);
	test_KErrNone(r);
		
	file1.Close();
	r = fs.Delete(KClientFileName);
	test_KErrNone(r);
	
	RProcess p;
	r = p.Create(_L("FHServer64Bit.exe"), KNullDesC);
	test_KErrNone(r);
	// Request an open file (write mode) from the server
	// using RFile64::AdoptFromServer()
	
	test.Next(_L("Adopt an already open file from a server using RFile64::AdoptFromServer()\n"));
	p.Resume();
	RFileHandleSharer64Bit handsvr;
	do
		{
		r = handsvr.Connect();
		}
	while(r == KErrNotFound);
	test_KErrNone(r);

	r = handsvr.SetTestDrive(gDrive);
	test_KErrNone(r);

	TInt ssh;
	TInt fsh = handsvr.GetFileHandleLargeFile2(ssh, EFileWrite);
	test_Value(fsh, fsh >= 0);

	// Closing the handle to the server ensures the server has closed it's
	// RFs and RFile handles - this provides a means of testing whether we 
	// can still adopt the RFile even if the server has closed it's one.

	handsvr.Sync(); // make sure server has finished doing what it's doing
	handsvr.Exit();
	handsvr.Close();

	// adopt the file handle from FHServer
	test.Next(_L("RFile64::AdoptFromServer()"));

	RFile64 file;
	r = file.AdoptFromServer(fsh, ssh);
	test_KErrNone(r);

	test.Next(_L("Read the file from position 4GB-10 and compare the data\n"));
	TBuf8<9> rbuf;
	r = file.Read(K4GB-10,rbuf);
	test_KErrNone(r);
	// server should write KTestData1 ("Server!!!") to file
	test (rbuf == KTestData4);

	TFileName fileName;
	r = file.FullName(fileName);
	test_KErrNone(r);
	
	file.Close();
	//cleanup
	r = fs.Delete(fileName);
	test_KErrNone(r);
		
	TFileName sessionPath;
	r = fs.SessionPath(sessionPath);
	test_KErrNone(r);
	
	r = fs.RmDir(sessionPath);
	test_KErrNone(r);
	
	fs.Close();
	
	}

	
/**
@SYMTestCaseID      PBASE-T_FILE64BIT-0769
@SYMTestPriority    High
@SYMTestRequirement REQ9526 
@SYMTestType        CIT
@SYMTestCaseDesc    Tests for reading a big file synchronously with specified position
@SYMTestActions     
1) Big file is read synchronously in a thread, with aPos = 0;
2) Big file is read synchronously in a thread, with aPos = 2GB-1;
3) Big file is read synchronously in a thread. With aPos = 4GB -2. File size= 4GB-1.
4) Check for FAT32 file system, Read from a big file synchronously in a thread with aPos = 4GB.
@SYMTestExpectedResults 
1) KErrNone, file is read successfully
2) KErrNone, file is read successfully
3) KErrNone, file is read successfully
4) KErrNone and zero length descriptor, if NGFS is supported we should get the valid data
@SYMTestStatus      Implemented
*/
void TestOpenAndReadSyncLargeFile()
	{
	const TUint KBufSize = KKB;
	TUint 			pos;
	TBuf8<KBufSize> readBuf1;
	TBuf8<KBufSize> readBuf2;
	TUint i;
	TInt r = GenerateBigFileContents();
	test_KErrNone(r);
	
	test.Next(_L("Open & Read Synchronously Large File From Diff Offset:"));

	TFileName fileName;
	fileName.Append(gDriveToTest);
	fileName.Append(KTestPath);
	fileName.Append(_L("File4GBMinusOne.txt"));
	TestRFile1.Open(fileName,EFileRead);

	test.Next(_L("Big file is read synchronously in a thread, with aPos = 0\n"));
	// Sync read from pos = 0
	pos = 0; 
	readBuf1.Zero();
	TestRFile1.ReadP(pos, readBuf1);
	
	test.Next(_L("Compare the data read to the expected data\n"));
	for(i = pos; i< pos + (KBufSize / 4); i+=4)
		{
		TUint j =  * ((TUint*) &readBuf1[i - pos]);
		test_Equal(i, j);
		}
		
	test.Next(_L("Big file is read synchronously in a thread, with aPos = 2GB-1\n"));
	// Sync read from pos = 2GB-1
	pos = K2GB;
	readBuf2.Zero();
	TestRFile1.ReadP(pos, readBuf2);

	test.Next(_L("Compare the data read to the expected data\n"));
	for(i = pos; i< pos + (KBufSize / 4); i+=4)
		{
		TUint j =  * ((TUint*) &readBuf2[i - pos]);
		test_Equal(i, j);
		}
	
	
	test.Next(_L("Big file is read synchronously in a thread, with aPos = 4GB-2\n"));	
	TBuf8<1> readBuffer;
	pos = K4GBMinusTwo;
	TestRFile1.ReadP(pos, readBuffer);
	test_Equal(1, readBuffer.Length());

	// tests need to be repeated for calling the TUint variant of RFile64::Read()
	pos = 0;
	TestRFile1.ReadU(pos, readBuf1);
	
	test.Next(_L("Compare the data read to the expected data\n"));
	for(i = pos; i< pos + (KBufSize / 4); i+=4)
		{
		TUint j =  * ((TUint*) &readBuf1[i - pos]);
		test_Equal(i, j);
		}
		
	test.Next(_L("Big file is read synchronously in a thread, with aPos = 2GB\n"));
	// Sync read from pos = 2GB
	pos = K2GB;
	readBuf2.Zero();
	TestRFile1.ReadU(pos, readBuf2);

	test.Next(_L("Compare the data read to the expected data\n"));
	for(i = pos; i< pos + (KBufSize / 4); i+=4)
		{
		TUint j =  * ((TUint*) &readBuf2[i - pos]);
		test_Equal(i, j);
		}
	
	
	test.Next(_L("Big file is read synchronously in a thread, with aPos = 4GB-2\n"));	
	pos = K4GBMinusTwo;
	TestRFile1.ReadU(pos, readBuffer);
	test_Equal(1, readBuffer.Length());

	// tests need to be repeated for calling the current position variant of RFile64::Read()
	TInt64 seekPos = 0;
	TestRFile1.Seek(ESeekStart,seekPos);
	TestRFile1.Read(readBuf1);
	
	test.Next(_L("Compare the data read to the expected data\n"));
	for(i = (TUint)seekPos; i< seekPos + (KBufSize / 4); i+=4)
		{
		TUint j =  * ((TUint*) &readBuf1[i - (TUint)seekPos]);
		test_Equal(i, j);
		}
		
	test.Next(_L("Big file is read synchronously in a thread, with aPos = 2GB\n"));
	// Sync read from pos = 2GB
	seekPos = K2GB;
	TestRFile1.Seek(ESeekStart,seekPos);
	readBuf2.Zero();
	TestRFile1.Read(readBuf2);

	test.Next(_L("Compare the data read to the expected data\n"));
	for(i = (TUint)seekPos; i< seekPos + (KBufSize / 4); i+=4)
		{
		TUint j =  * ((TUint*) &readBuf2[i - (TUint)seekPos]);
		test_Equal(i, j);
		}
	
	
	test.Next(_L("Big file is read synchronously in a thread, with aPos = 4GB-2\n"));	
	seekPos = K4GBMinusTwo;
	TestRFile1.Seek(ESeekStart,seekPos);
	TestRFile1.Read(readBuffer);
	test_Equal(1, readBuffer.Length());

	
	if(KFileSizeMaxLargerThan4GBMinusOne == EFalse)	
		{
		TInt64 pos64 = K4GB;
		TestRFile1.ReadP(pos64, readBuf1);
		test_Equal(0, readBuf1.Length());
		}
	TestRFile1.Close();
	}

/**
@SYMTestCaseID      PBASE-T_FILE64BIT-0770
@SYMTestPriority    High
@SYMTestRequirement REQ9526 
@SYMTestType        CIT
@SYMTestCaseDesc    Tests for reading a big file asynchronously with specified position
@SYMTestActions     
1) Big file is read asynchronously in a thread, with aPos = 0;
2) Big file is read asynchronously in a thread, with aPos = 2GB-1;
3) Big file is read asynchronously in a thread. With aPos = 4GB -1.
4) Check for FAT32 file system, Read from a big file asynchronously in a thread with aPos = 4GB.
@SYMTestExpectedResults 
1) KErrNone, file is read successfully
2) KErrNone, file is read successfully
3) KErrNone, file is read successfully
4) KErrNone and zero length descriptor. If NGFS is supported we should get the valid data.
@SYMTestStatus      Implemented
*/
void TestOpenAndReadAsyncLargeFile()
	{
	const TUint KBufSize = KKB;
	TInt64  fileSize, size = 0;
	TUint pos;
	TUint i;
	TBuf8<KBufSize> readBuf;
	readBuf.SetLength(KBufSize);


	test.Next(_L("Open & Read Asynchronously Large File From Diff Offset:"));
	
	TFileName fileName;
	fileName.Append(gDriveToTest);
	fileName.Append(KTestPath);
	fileName.Append(_L("File4GBMinusOne.txt"));
	TestRFile1.Open(fileName,EFileRead);
	
	test.Next(_L("Big file is read asynchronously in a thread, with aPos = 0\n"));
	// Async read from pos = 0
	TRequestStatus status1 = KRequestPending;
	pos = 0;
	TestRFile1.Read(pos, readBuf, status1);

	test.Next(_L("Compare the data read to the expected data\n"));
	for(i = pos; i< pos + (KBufSize / 4); i+=4)
		{
		TUint j =  * ((TUint*) &readBuf[i - pos]);
		test_Equal(i, j);
		}	
	
	test.Next(_L("Big file is read asynchronously in a thread, with aPos = 2GB-1\n"));
	// Async read from pos = 2GB-1
	TRequestStatus status2 = KRequestPending;
	pos = K2GB;
	TestRFile1.Read(pos, readBuf, status2);
	
	test.Next(_L("Compare the data read to the expected data\n"));
	for(i = pos; i< pos + (KBufSize / 4); i+=4)
		{
		TUint j =  * ((TUint*) &readBuf[i - pos]);
		test_Equal(i, j);
		}
	
	test.Next(_L("Big file is read asynchronously in a thread, with aPos = 4GB-1\n"));
	TBuf8<0x1> readBuf1;
	// Async read from pos = 4GB-1
	TRequestStatus status3 = KRequestPending;
	pos = K4GBMinusTwo;
	TestRFile1.Read(pos, readBuf1, status3);
	test_Equal(1, readBuf1.Length());
			
	fileSize = K4GBMinusOne;
	TestRFile1.Size(size);
	test(size == fileSize);
	
	//tests need to be repeated for calling the TUint variant of RFile64::Read()
	pos = 0;
	TestRFile1.ReadU(pos, readBuf, status1);

	test.Next(_L("Compare the data read to the expected data\n"));
	for(i = pos; i< pos + (KBufSize / 4); i+=4)
		{
		TUint j =  * ((TUint*) &readBuf[i - pos]);
		test_Equal(i, j);
		}	
	
	test.Next(_L("Big file is read asynchronously in a thread, with aPos = 2GB-1\n"));
	// Async read from pos = 2GB-1
	status2 = KRequestPending;
	pos = K2GB;
	TestRFile1.ReadU(pos, readBuf, status2);
	
	test.Next(_L("Compare the data read to the expected data\n"));
	for(i = pos; i< pos + (KBufSize / 4); i+=4)
		{
		TUint j =  * ((TUint*) &readBuf[i - pos]);
		test_Equal(i, j);
		}
	
	test.Next(_L("Big file is read asynchronously in a thread, with aPos = 4GB-1\n"));
	// Async read from pos = 4GB-1
	status3 = KRequestPending;
	pos = K4GBMinusTwo;
	TestRFile1.ReadU(pos, readBuf1, status3);
	test_Equal(1, readBuf1.Length());
	
	// tests need to be repeated for calling the current position variant of RFile64::Read()
	TInt64 seekPos = 0;
	TestRFile1.Seek(ESeekStart,seekPos);
	TestRFile1.Read(readBuf, status1);

	test.Next(_L("Compare the data read to the expected data\n"));
	for(i = (TUint)seekPos; i< seekPos + (KBufSize / 4); i+=4)
		{
		TUint j =  * ((TUint*) &readBuf[i - (TUint)seekPos]);
		test_Equal(i, j);
		}	
	
	test.Next(_L("Big file is read asynchronously in a thread, with aPos = 2GB-1\n"));
	// Async read from pos = 2GB-1
	status2 = KRequestPending;
	seekPos = K2GB;
	TestRFile1.Seek(ESeekStart,seekPos);
	TestRFile1.Read(readBuf, status2);
	
	test.Next(_L("Compare the data read to the expected data\n"));
	for(i = (TUint)seekPos; i< seekPos + (KBufSize / 4); i+=4)
		{
		TUint j =  * ((TUint*) &readBuf[i - (TUint)seekPos]);
		test_Equal(i, j);
		}
	
	test.Next(_L("Big file is read asynchronously in a thread, with aPos = 4GB-1\n"));
	// Async read from pos = 4GB-1
	status3 = KRequestPending;
	seekPos = K4GBMinusTwo;
	TestRFile1.Seek(ESeekStart,seekPos);
	TestRFile1.Read(readBuf1, status3);
	test_Equal(1, readBuf1.Length());
	

	// Async read from pos = 4GB
	if(KFileSizeMaxLargerThan4GBMinusOne == EFalse)
		{
		TRequestStatus status5 = KRequestPending;
		TInt64 pos64;
		pos64 = K4GB;
		TestRFile1.Read(pos64, readBuf, status5);
		test_Equal(0, readBuf.Length());
		}
	// Close the file	
	TestRFile1.Close();
	}		
	
/**
@SYMTestCaseID      PBASE-T_FILE64BIT-0771
@SYMTestPriority    High
@SYMTestRequirement REQ9526 
@SYMTestType        CIT
@SYMTestCaseDesc    Tests for reading a big file synchronously with specified position and length
@SYMTestActions     
1) Big file is read synchronously in a thread, with aPos = 0 and length = 256bytes
2) Big file is read synchronously in a thread, with aPos = 2GB-1 and length = 2KB
3) Big file is read synchronously in a thread, with aPos = 4GB -1 and length = 10 bytes
4) Check for FAT32 file system. Read from a big file, synchronously in a thread with aPos = 4GB and length = 1KB
5) Big file is read synchronously in a thread, with aPos = 0 and length = -1
6) Big file is read synchronously in a thread, with aPos = 0 and length = 0 bytes
@SYMTestExpectedResults 
1) KErrNone, file is read successfully
2) KErrNone, file is read successfully
3) KErrNone, file is read successfully
4) KErrNone, with zero length descriptor. If NGFS is supported we should get the valid data
5) KErrArgument
6) KErrNone
@SYMTestStatus      Implemented
*/

void TestOpenAndReadSyncLargeFileWithLen()
	{
	TInt64 pos;
	TUint i;
	TBuf8<KMAXBUFSIZE> readBuf;
	readBuf.SetLength(KMAXBUFSIZE);

	test.Next(_L("Open & Read Synchronously Large File From Different Offset and Length:"));

	TFileName fileName;
	fileName.Append(gDriveToTest);
	fileName.Append(KTestPath);
	fileName.Append(_L("File4GBMinusOne.txt"));
	TestRFile1.Open(fileName,EFileRead);
	
	test.Next(_L("Big file is read synchronously in a thread, with aPos = 0 and length = 256bytes:"));
	// Sync read from pos = 0 and length = 256
	pos = 0;
	TestRFile1.Read(pos, readBuf, 256);
	test_Equal(256, readBuf.Length());
	
	test.Next(_L("Compare the data read to the expected data\n"));
	for(i = (TUint)pos; i< pos + (256 / 4); i+=4)
		{
		TUint j =  * ((TUint*) &readBuf[i - (TUint)pos]);
		test_Equal(i, j);
		}
	
	test.Next(_L("Big file is read synchronously in a thread, with aPos = 2GB and length = K2KB:"));
	// Sync read from pos = 2GB and length = K2KB
	pos = K2GB;
	TestRFile1.Read(pos, readBuf, K2KB);
	test_Equal(K2KB, readBuf.Length());
	
	test.Next(_L("Compare the data read to the expected data\n"));
	for(i = (TUint)pos; i< pos + (K2KB / 4); i+=4)
		{
		TUint j =  * ((TUint*) &readBuf[i - (TUint)pos]);
		test_Equal(i, j);
		}
	
	
	test.Next(_L("Big file is read synchronously in a thread, with aPos = 4GB -1 and length = 10:"));
	// Sync read from pos = 4GB-1 and length = 10
	pos = K4GBMinusTwo;
	TestRFile1.Read(pos, readBuf, 10);
	test_Equal(1, readBuf.Length());
		

	// Sync read from pos = 4GB and length = KKB

	if(KFileSizeMaxLargerThan4GBMinusOne == EFalse)	
		{
		pos = K4GB;
		TestRFile1.Read(pos, readBuf, KKB);
		test_Equal(0, readBuf.Length());
		}
		
	test.Next(_L("Big file is read synchronously in a thread, with aPos = 0 and length = -1"));
	// Sync read from pos = 0 and length = -1	
	pos = 0;
	TestRFile1.Read(pos, readBuf, -1);

	//tests need to repeated for TUint variant of RFile64::Read()

	test.Next(_L("Big file is read synchronously in a thread, with aPos = 0 and length = 256bytes:"));
	// Sync read from pos = 0 and length = 256
	pos = 0;
	TestRFile1.ReadU((TUint)pos, readBuf, 256);
	test_Equal(256, readBuf.Length());
	
	test.Next(_L("Compare the data read to the expected data\n"));
	for(i = (TUint)pos; i< pos + (256 / 4); i+=4)
		{
		TUint j =  * ((TUint*) &readBuf[i - (TUint)pos]);
		test_Equal(i, j);
		}
	
	test.Next(_L("Big file is read synchronously in a thread, with aPos = 2GB and length = K2KB:"));
	// Sync read from pos = 2GB and length = K2KB
	pos = K2GB;
	TestRFile1.ReadU((TUint)pos, readBuf, K2KB);
	test_Equal(K2KB, readBuf.Length());
	
	test.Next(_L("Compare the data read to the expected data\n"));
	for(i = (TUint)pos; i< pos + (K2KB / 4); i+=4)
		{
		TUint j =  * ((TUint*) &readBuf[i - (TUint)pos]);
		test_Equal(i, j);
		}
	
	
	test.Next(_L("Big file is read synchronously in a thread, with aPos = 4GB -1 and length = 10:"));
	// Sync read from pos = 4GB-1 and length = 10
	pos = K4GBMinusTwo;
	TestRFile1.ReadU((TUint)pos, readBuf, 10);
	test_Equal(1, readBuf.Length()); 
		
	//tests need to repeated for current position variant of RFile64::Read()
	test.Next(_L("Big file is read synchronously in a thread, with aPos = 0 and length = 256bytes:"));
	// Sync read from pos = 0 and length = 256
	TInt64 seekPos = 0;
	TestRFile1.Seek(ESeekStart,seekPos);
	TestRFile1.Read(readBuf, 256);
	test_Equal(256, readBuf.Length());
	
	test.Next(_L("Compare the data read to the expected data\n"));
	for(i = (TUint)seekPos; i< seekPos + (256 / 4); i+=4)
		{
		TUint j =  * ((TUint*) &readBuf[i - (TUint)seekPos]);
		test_Equal(i, j);
		}
	
	test.Next(_L("Big file is read synchronously in a thread, with aPos = 2GB and length = K2KB:"));
	// Sync read from pos = 2GB and length = K2KB
	seekPos = K2GB;
	TestRFile1.Seek(ESeekStart,seekPos);
	TestRFile1.Read(readBuf, K2KB);
	test_Equal(K2KB, readBuf.Length());
	
	test.Next(_L("Compare the data read to the expected data\n"));
	for(i = (TUint)seekPos; i< seekPos + (K2KB / 4); i+=4)
		{
		TUint j =  * ((TUint*) &readBuf[i - (TUint)seekPos]);
		test_Equal(i, j);
		}
	
	
	test.Next(_L("Big file is read synchronously in a thread, with aPos = 4GB -1 and length = 10:"));
	// Sync read from pos = 4GB-1 and length = 10
	seekPos = K4GBMinusTwo;
	TestRFile1.Seek(ESeekStart,seekPos);
	TestRFile1.Read(readBuf, 10);
	test_Equal(1, readBuf.Length()); 
	

	// Sync read from pos = 4GB and length = KKB

	if(KFileSizeMaxLargerThan4GBMinusOne == EFalse)	
		{
		pos = K4GB;
		TestRFile1.Read(pos, readBuf, KKB);
		test_Equal(0, readBuf.Length());
		}
		
	test.Next(_L("Big file is read synchronously in a thread, with aPos = 0 and length = -1"));
	// Sync read from pos = 0 and length = -1	
	pos = 0;
	TestRFile1.Read(pos, readBuf, -1);

	

	test.Next(_L("Big file is read synchronously in a thread, with aPos = 0 and length = 0 bytes"));
	// Sync read from pos = 0 and length = 0
	pos = 0;
	TestRFile1.Read(pos, readBuf, 0);
	test_Equal(0, readBuf.Length());

	TestRFile1.Close();
	}
/**
@SYMTestCaseID      PBASE-T_FILE64BIT-0772
@SYMTestPriority    High
@SYMTestRequirement REQ9526 
@SYMTestType        CIT
@SYMTestCaseDesc    Tests for reading a big file asynchronously with specified position and length
@SYMTestActions     
1) Big file is read asynchronously in a thread, with aPos = 0 and length = 256 bytes
2) Big file is read asynchronously in a thread, with aPos = 2GB-1 and length = 1KB
3) Big file is read asynchronously in a thread, with aPos = 4GB-1 and length = 1KB
4) Big file is read asynchronously in a thread, with aPos = 4GB and length = 256 bytes
5) Big file is read asynchronously in a thread, with aPos = 0 and length = -1
6) Big file is read asynchronously in a thread, with aPos = 0 and length = 0 bytes
@SYMTestExpectedResults 
1) KErrNone, file is read successfully
2) KErrNone, file is read successfully
3) KErrNone, file is read successfully
4) KErrNone, with zero length descriptor. If NGFS is supported KErrNone with valid data
5) KErrArgument
6) KErrNone
@SYMTestStatus      Implemented
*/
void TestOpenAndReadAsyncLargeFileWithLen()
	{
	TInt64 pos;
	TUint i ;
	TBuf8<KMAXBUFSIZE> readBuf;
	readBuf.SetLength(KMAXBUFSIZE);

	test.Next(_L("Open & Read Asynchronously Large File From Different Offset & Length:"));

	TFileName fileName;
	fileName.Append(gDriveToTest);
	fileName.Append(KTestPath);
	fileName.Append(_L("File4GBMinusOne.txt"));
	TestRFile1.Open(fileName,EFileRead);

	test.Next(_L("Big file is read asynchronously in a thread, with aPos = 0 and length = 256 bytes\n"));
	// Async read from pos = 0 and length = 256
	TRequestStatus status1 = KRequestPending;
	pos = 0;
	TestRFile1.Read(pos, readBuf, 256, status1);
	test_Equal(256, readBuf.Length());
	
	test.Next(_L("Compare the data read to the expected data\n"));
	for(i = (TUint)pos; i< pos + (256 / 4); i+=4)
		{
		TUint j =  * ((TUint*) &readBuf[i - (TUint)pos]);
		test_Equal(i, j);
		}
		
	test.Next(_L("Big file is read asynchronously in a thread, with aPos = 2GB and length = KKB bytes\n"));	
	// Async read from pos = 2GB and length = KKb
	TRequestStatus status2 = KRequestPending;
	pos = K2GB;
	TestRFile1.Read(pos, readBuf, KKB, status2);
	test_Equal(KKB, readBuf.Length());
	
	test.Next(_L("Compare the data read to the expected data\n"));
	for(i = (TUint)pos; i< pos + (KKB / 4); i+=4)
		{
		TUint j =  * ((TUint*) &readBuf[i - (TUint)pos]);
		test_Equal(i, j);
		}
	
	
	test.Next(_L("Big file is read asynchronously in a thread, with aPos = 4GB-1 and length = KKb bytes\n"));	
	// Async read from pos = 4GB-1 and length = KKb
	TRequestStatus status3 = KRequestPending;
	pos = K4GBMinusTwo;
	TestRFile1.Read(pos, readBuf, KKB, status3);
	test_Equal(1, readBuf.Length()); 
		
	// tests need to be repeated for TUint variant of RFile64::Read()
	test.Next(_L("Big file is read asynchronously in a thread, with aPos = 0 and length = 256 bytes\n"));
	// Async read from pos = 0 and length = 256
	status1 = KRequestPending;
	pos = 0;
	TestRFile1.ReadU((TUint)pos, readBuf, 256, status1);
	test_Equal(256, readBuf.Length());
	
	test.Next(_L("Compare the data read to the expected data\n"));
	for(i = (TUint)pos; i< pos + (256 / 4); i+=4)
		{
		TUint j =  * ((TUint*) &readBuf[i - (TUint)pos]);
		test_Equal(i, j);
		}
		
	test.Next(_L("Big file is read asynchronously in a thread, with aPos = 2GB and length = KKB bytes\n"));	
	// Async read from pos = 2GB and length = KKb
	status2 = KRequestPending;
	pos = K2GB;
	TestRFile1.ReadU((TUint)pos, readBuf, KKB, status2);
	test_Equal(KKB, readBuf.Length());
	
	test.Next(_L("Compare the data read to the expected data\n"));
	for(i = (TUint)pos; i< pos + (KKB / 4); i+=4)
		{
		TUint j =  * ((TUint*) &readBuf[i - (TUint)pos]);
		test_Equal(i, j);
		}
	
	
	test.Next(_L("Big file is read asynchronously in a thread, with aPos = 4GB-1 and length = KKb bytes\n"));	
	// Async read from pos = 4GB-1 and length = KKb
	status3 = KRequestPending;
	pos = K4GBMinusTwo;
	TestRFile1.ReadU((TUint)pos, readBuf, KKB, status3);
	test_Equal(1, readBuf.Length()); 
		
	// tests need to be repeated for current position variant of RFile64::Read()
	test.Next(_L("Big file is read asynchronously in a thread, with aPos = 0 and length = 256 bytes\n"));
	// Async read from pos = 0 and length = 256
	status1 = KRequestPending;
	TInt64 seekPos = 0;
	TestRFile1.Seek(ESeekStart,seekPos);
	TestRFile1.Read(readBuf, 256, status1);
	test_Equal(256, readBuf.Length());
	
	test.Next(_L("Compare the data read to the expected data\n"));
	for(i = (TUint)seekPos; i< seekPos + (256 / 4); i+=4)
		{
		TUint j =  * ((TUint*) &readBuf[i - (TUint)seekPos]);
		test_Equal(i, j);
		}
		
	test.Next(_L("Big file is read asynchronously in a thread, with aPos = 2GB and length = KKB bytes\n"));	
	// Async read from pos = 2GB and length = KKb
	status2 = KRequestPending;
	seekPos = K2GB;
	TestRFile1.Seek(ESeekStart,seekPos);
	TestRFile1.Read(readBuf, KKB, status2);
	test_Equal(KKB, readBuf.Length());
	
	test.Next(_L("Compare the data read to the expected data\n"));
	for(i = (TUint)seekPos; i< seekPos + (KKB / 4); i+=4)
		{
		TUint j =  * ((TUint*) &readBuf[i - (TUint)seekPos]);
		test_Equal(i, j);
		}
	
	
	test.Next(_L("Big file is read asynchronously in a thread, with aPos = 4GB-1 and length = KKb bytes\n"));	
	// Async read from pos = 4GB-1 and length = KKb
	status3 = KRequestPending;
	seekPos = K4GBMinusTwo;
	TestRFile1.Seek(ESeekStart,seekPos);
	TestRFile1.Read(readBuf, KKB, status3);
	test_Equal(1, readBuf.Length()); 
	
		
	test.Next(_L("Big file is read asynchronously in a thread, with aPos = 4GB and length = 256 bytes\n"));	
	// Async read from pos = 4GB and length = 256
	
	
	if(KFileSizeMaxLargerThan4GBMinusOne == EFalse)
		{
		TRequestStatus status5 = KRequestPending;
		pos = K4GB;
		TestRFile1.Read(pos, readBuf, 256, status5);
		test_Equal(0, readBuf.Length());
		}
	
	test.Next(_L("Big file is read asynchronously in a thread, with aPos = 0 and length = -1 bytes\n"));	
	// Async read from pos = 0 and length = -1	
	TRequestStatus status6 = KRequestPending;
	pos = 0;
	TestRFile1.Read(pos, readBuf, -1, status6);

	test.Next(_L("Big file is read asynchronously in a thread, with aPos = 0 and length = 0 bytes\n"));	
	// Async read from pos = 0 and length = 0
	TRequestStatus status7 = KRequestPending;
	pos = 0;
	TestRFile1.Read(pos, readBuf, 0, status7);

	TestRFile1.Close();
	
	TInt r = TheFs.Delete(fileName);
	test_KErrNone(r);
	}	

/**
@SYMTestCaseID      PBASE-T_FILE64BIT-0773
@SYMTestPriority    High
@SYMTestRequirement REQ9526 
@SYMTestType        CIT
@SYMTestCaseDesc    Tests for writing to a big file synchronously with specified position
@SYMTestActions     
1) Write to a big file synchronously in a thread, with aPos = 0 and data = 256 bytes, File size = 4GB-1
2) Write to a big file synchronously in a thread, with aPos = 2GB-1 and data = 1KB
3) Write to a big file synchronously in a thread, with aPos = 4GB-1 and data = 1 byte
4) Write a big file synchronously in a thread, with aPos = 4GB and data = 256 bytes
@SYMTestExpectedResults 
1) KErrNone, write is successful
2) KErrNone, write is successful
3) KErrNone, write is successful
4) KErrNotSupported, if NGFS is supported KErrNone and write is successful
@SYMTestStatus      Implemented
*/

void TestOpenAndWriteSyncLargeFile()
	{
	test.Next(_L("Open & Write Synchronously Large File From Different Offset:"));

	TInt count;
	TFileName fileName;
	fileName.Append(gDriveToTest);
	fileName.Append(KTestPath);
	fileName.Append(_L("File4GBMinusOne.txt"));
	TestRFile1.Replace(fileName, EFileWrite);
	TestRFile1.SetSize(K4GBMinusOne);
	
	TInt64 size;
	TestRFile1.Size(size);
	test(size == K4GBMinusOne);
	
	
	test.Next(_L("Write to a big file synchronously in a thread, with aPos = 0 and data = 256 bytes, File size = 4GB-1\n"));
	TBuf8<0x100> writeBuf100;
	TBuf8<0x100> readBuf100;
	writeBuf100.Zero();
	writeBuf100.FillZ();
	for (count = 0; count < 0x100; count++)
		{
		writeBuf100.Append((TChar)count);
		}
	TestRFile1.WriteP(0,writeBuf100);
	TestRFile1.Size(size);
	TestRFile1.ReadP(0,readBuf100);
	test(writeBuf100 == readBuf100);
	
	test.Next(_L("Write to a big file synchronously in a thread, with aPos = 2GB-1 and data = 1KB\n"));
	TBuf8<0x400> writeBuf400;
	TBuf8<0x400> readBuf400;
	writeBuf400.Zero();
	writeBuf400.FillZ();	
	for (count = 0; count < 0x400; count++)
		{
		writeBuf400.Append(count+20);
		}
	TestRFile1.WriteP(K2GBMinusOne,writeBuf400);
	TestRFile1.ReadP(K2GBMinusOne,readBuf400); // just for validation
	test(writeBuf400 == readBuf400);

	test.Next(_L("Write to a big file synchronously in a thread, with aPos = 4GB-1 and data = 1 byte\n"));
	TBuf8<1> testReadBuf;
	TestRFile1.WriteP(K4GBMinusTwo,_L8("1"));
	TestRFile1.ReadP(K4GBMinusTwo, testReadBuf); 
	test_Equal(1, testReadBuf.Length());
	
	//tests need to be repeated for TUint variant of RFile64::Write()
	readBuf100.Zero(); //to ensure that the previous data is removed
	test.Next(_L("Write to a big file synchronously in a thread, with aPos = 0 and data = 256 bytes, File size = 4GB-1\n"));
	TUint pos = 0;
	TestRFile1.WriteU(pos,writeBuf100);
	TestRFile1.ReadU(pos,readBuf100);
	test(writeBuf100 == readBuf100);
	
	test.Next(_L("Write to a big file synchronously in a thread, with aPos = 2GB-1 and data = 1KB\n"));
	readBuf400.Zero();//to ensure that the previous data is removed
	pos = K2GBMinusOne;
	TestRFile1.WriteU(pos,writeBuf400);
	TestRFile1.ReadU(pos,readBuf400); // just for validation
	test(writeBuf400 == readBuf400);


	test.Next(_L("Write to a big file synchronously in a thread, with aPos = 4GB-1 and data = 1 byte\n"));
	pos = K4GBMinusTwo;
	testReadBuf.Zero();//to ensure that the previous data is removed
	TestRFile1.WriteU(pos,_L8("1"));
	TestRFile1.ReadU(pos, testReadBuf); 
	test_Equal(1, testReadBuf.Length());
	
	//
	//tests need to be repeated for current position variant of RFile64::Write()
	//testing with only current position as 4GB-2(boundary condition)
	//
	test.Next(_L("Write to a big file synchronously in a thread, with aPos = 4GB-1 and data = 1 byte\n"));
	TInt64 seekPos = K4GBMinusTwo;
	testReadBuf.Zero();//to ensure that the previous data is removed
	TestRFile1.Seek(ESeekStart,seekPos);
	TestRFile1.Write(_L8("1"));
	TestRFile1.Seek(ESeekStart,seekPos);
	TestRFile1.Read(testReadBuf); 
	test_Equal(1, testReadBuf.Length());
	
	
	test.Next(_L("Write a big file synchronously in a thread, with aPos = 4GB and data = 256 bytes\n"));
	
	
	TBuf8<0x100> writeBuffer256;
	TBuf8<0x100> readBuffer256;
	writeBuffer256.Zero();
	writeBuffer256.FillZ();
	readBuffer256.Zero();
	readBuffer256.FillZ();	
	
	if(KFileSizeMaxLargerThan4GBMinusOne == EFalse)	
		{	
		for (TInt count = 0; count < 256; count++)
			{
			writeBuffer256.Append((TChar)count);
			}
		TestRFile1.WriteP(K4GB,writeBuffer256); 
		User::After(100000);
		// Validation for boundary condition 4GB
		TestRFile1.ReadP(K4GB,readBuffer256);
		TInt rr = readBuffer256.Length();
		test_KErrNone(rr);
		test(readBuffer256.Length() == 0);
		}
	TestRFile1.Close();
	
	TInt r = TheFs.Delete(fileName);
	test_KErrNone(r);
	}

/**
@SYMTestCaseID      PBASE-T_FILE64BIT-0774
@SYMTestPriority    High
@SYMTestRequirement REQ9526 
@SYMTestType        CIT
@SYMTestCaseDesc    Tests for writing to a big file asynchronously with specified position
@SYMTestActions     
1) Write to a big file asynchronously in a thread, with aPos = 0 and data = 256 bytes, File size = 4GB-1
2) Write to a big file asynchronously in a thread, with aPos = 2GB-1 and data = 1KB
3) Write to a big file asynchronously in a thread, with aPos = 4GB-1 and data = 1 byte
4) Check for FAT32 file system. Write to a big file asynchronously in a thread with aPos =  4GB and data length = 256 bytes
@SYMTestExpectedResults 
1) KErrNone, write is successful
2) KErrNone, write is successful
3) KErrNone, write is successful
4) KErrNotSupported, if NGFS is available KErrNone and write is successful.
@SYMTestStatus      Implemented
*/

void TestOpenAndWriteAsyncLargeFile()
	{
	test.Next(_L("Open & Write Asynchronously Large File From Different Offset:"));

	TInt count;
	TFileName fileName;
	fileName.Append(gDriveToTest);
	fileName.Append(KTestPath);
	fileName.Append(_L("File4GBMinusOne.txt"));
	TestRFile1.Replace(fileName, EFileWrite);

	TestRFile1.SetSize(K4GBMinusOne);
	TInt64 size;
	TestRFile1.Size(size);

	test.Next(_L("Write to a big file asynchronously in a thread, with aPos = 0 and data = 256 bytes, File size = 4GB-1\n"));
	TBuf8<0x100> writeBuf100;
	TBuf8<0x100> readBuf100;
	writeBuf100.Zero();
	writeBuf100.FillZ();
	for (count = 0; count < 0x100; count++)
		{
		writeBuf100.Append((TChar)count);
		}
	TRequestStatus status1 = KRequestPending;
	TestRFile1.Write(0,writeBuf100,status1);
	TestRFile1.ReadP(0,readBuf100);
	test (writeBuf100 == readBuf100);

	test.Next(_L("Write to a big file asynchronously in a thread, with aPos = 2GB-1 and data = 1KB\n"));
	TBuf8<0x400> writeBuf400;
	TBuf8<0x400> readBuf400;
	writeBuf400.Zero();
	writeBuf400.FillZ();	
	for (count = 0; count < 0x400; count++)
		{
		writeBuf400.Append(count+20);
		}
	TRequestStatus status2 = KRequestPending;
	TestRFile1.Write(K2GBMinusOne,writeBuf400,status2);
	TestRFile1.ReadP(K2GBMinusOne,readBuf400); // just for validation
	test(writeBuf400 == readBuf400);

	test.Next(_L("Write to a big file asynchronously in a thread, with aPos = 4GB-1 and data = 1 byte\n"));
	TBuf8<0x1> writeBuf;
	TBuf8<0x1> readBuf;
	writeBuf.Zero();
	writeBuf.FillZ();
	for (count = 0; count < 0x1; count++)
		{
		writeBuf.Append((TChar)(count+17));
		}
	TRequestStatus status3 = KRequestPending;	
	TestRFile1.Write(K4GBMinusTwo,writeBuf,status3); 
	TestRFile1.ReadP(K4GBMinusTwo,readBuf);
	test_Equal(1, readBuf.Length());
	
	//tests need to be repeated for TUint variant of RFile64::Write()
	test.Next(_L("Write to a big file asynchronously in a thread, with aPos = 0 and data = 256 bytes, File size = 4GB-1\n"));
	readBuf100.Zero();//to ensure that the previous data is removed
	status1 = KRequestPending;
	TUint pos = 0;
	TestRFile1.WriteU(pos,writeBuf100,status1);
	TestRFile1.ReadU(pos,readBuf100);
	test (writeBuf100 == readBuf100);

	test.Next(_L("Write to a big file asynchronously in a thread, with aPos = 2GB-1 and data = 1KB\n"));
	readBuf400.Zero();//to ensure that the previous data is removed
	status2 = KRequestPending;
	pos = K2GBMinusOne;
	TestRFile1.WriteU(pos,writeBuf400,status2);
	TestRFile1.ReadU(pos,readBuf400); // just for validation
	test(writeBuf400 == readBuf400);

	test.Next(_L("Write to a big file asynchronously in a thread, with aPos = 4GB-1 and data = 1 byte\n"));
	readBuf.Zero();//to ensure that the previous data is removed
	status3 = KRequestPending;	
	pos = K4GBMinusTwo;
	TestRFile1.WriteU(pos,writeBuf,status3); 
	TestRFile1.ReadU(pos,readBuf);
	test_Equal(1, readBuf.Length());
	
	//
	//tests need to be repeated for current position variant of RFile64::Write()
	//testing with only current position as 4GB-2(boundary condition)
	//
	test.Next(_L("Write to a big file asynchronously in a thread, with aPos = 4GB-1 and data = 1 byte\n"));
	readBuf.Zero();//to ensure that the previous data is removed
	status3 = KRequestPending;	
	TInt64 seekPos = K4GBMinusTwo;
	TestRFile1.Seek(ESeekStart,seekPos);
	TestRFile1.Write(writeBuf,status3); 
	TestRFile1.Seek(ESeekStart,seekPos);
	TestRFile1.Read(readBuf);
	test_Equal(1, readBuf.Length());
	
	
	if(KFileSizeMaxLargerThan4GBMinusOne == EFalse)
		{	
		TBuf8<0x100> writeBuf256;
		TBuf8<0x100> readBuf256;
		writeBuf256.Zero();
		writeBuf256.FillZ();
		for (TInt count = 0; count < 0x100; count++)
			{
			writeBuf256.Append((TChar)(count+7));
			}
		TRequestStatus status4 = KRequestPending;
		TestRFile1.Write(K4GB,writeBuf256,status4); 
		User::After(100000);
		// Validation for boundary condition 4GB
		TestRFile1.ReadP(K4GB,readBuf256);
		test_Equal(0, readBuf256.Length());
		}
	TestRFile1.Close();
	
	TInt r = TheFs.Delete(fileName);
	test_KErrNone(r);
	}

/**
@SYMTestCaseID      PBASE-T_FILE64BIT-0775
@SYMTestPriority    High
@SYMTestRequirement REQ9526 
@SYMTestType        CIT
@SYMTestCaseDesc    Tests for writing to a big file synchronously with specified position and length
@SYMTestActions     
1) Write to a big file synchronously in a thread with aPos = 0, data = 256 bytes and length = 255 bytes
2) Write to a big file synchronously in a thread with aPos = 2GB -1, data = 1KB and length = 200 bytes
3) Write to a big file synchronously in a thread with aPos = 4GB-10 and data = 1KB and length = 10 bytes
4) Check for FAT32 file system. Write to a big file synchronously in a thread with aPos = 4GB, data length = 256 bytes and length = 10 bytes
5) Write to a big file synchronously in a thread with aPos = 0 ,  data = 256 bytes and length =0 bytes
6) Write to a big file synchronously in a thread with aPos = 0 ,  data = 256 bytes and length = -1
@SYMTestExpectedResults 
1) KErrNone, write is successful
2) KErrNone, write is successful
3) KErrNone, write is successful
4) KErrNotSupported. If NGFS is supported and write is successful 
5) KErrNone
6) KErrArgument
@SYMTestStatus      Implemented
*/

void TestOpenAndWriteSyncLargeFileWithLen()
	{
	test.Next(_L("Open & Write Synchronously Large File From Different Offset and length:"));

	TInt count;
	TFileName fileName;
	fileName.Append(gDriveToTest);
	fileName.Append(KTestPath);
	fileName.Append(_L("File4GBMinusOne.txt"));
	TestRFile1.Replace(fileName, EFileWrite);
	
	TestRFile1.SetSize(K4GBMinusOne);
	
	test.Next(_L("Write to a big file synchronously in a thread with aPos = 0, data = 256 bytes and length = 255 bytes\n"));	
	TBuf8<0x100> writeBuf100;
	TBuf8<0x100> readBuf100;
	TBuf8<0x100> validateBuf100;
	writeBuf100.Zero();
	writeBuf100.FillZ();
	validateBuf100.Zero();
	for (count = 0; count < 0x100; count++)
		{
		writeBuf100.Append((TChar)count);
		if(count < 0xFF)
			validateBuf100.Append((TChar)count);
		}
	TestRFile1.Write(0,writeBuf100,255);
	TestRFile1.Read(0,readBuf100,255);
	test(validateBuf100 == readBuf100);
	
	test.Next(_L("Write to a big file synchronously in a thread with aPos = 2GB -1, data = 1KB and length = 200 bytes\n"));	
	TBuf8<0x400> writeBuf400;
	TBuf8<0x400> readBuf400;
	TBuf8<0x400> validateBuf400;
	writeBuf400.Zero();
	writeBuf400.FillZ();	
	for (count = 0; count < 0x400; count++)
		{
		writeBuf400.Append(count+20);
		if(count<200)
			validateBuf400.Append(count+20);
		}
	TestRFile1.Write(K2GBMinusOne,writeBuf400,200);
	TestRFile1.Read(K2GBMinusOne,readBuf400,200); 
	test(validateBuf400 == readBuf400);
	
	test.Next(_L("Write to a big file synchronously in a thread with aPos = 4GB-10 and data = 1KB and length = 10 bytes\n"));	
	TBuf8<0x400> writeBuf1024;
	TBuf8<0x400> readBuf1024;
	TBuf8<0x400> validateBuf1024;
	writeBuf1024.Zero();
	writeBuf1024.FillZ();	
	for (count = 0; count < 0x400; count++)
		{
		writeBuf1024.Append(count+3);
		if(count < 9)
			validateBuf1024.Append(count+3);
		}
	TestRFile1.Write(K4GBMinusTen,writeBuf1024,9);
	TestRFile1.Read(K4GBMinusTen,readBuf1024,9); 
	test(validateBuf1024 == readBuf1024);
	
	//tests need to be repeated for TUint variant of RFile64::Write()
	test.Next(_L("Write to a big file synchronously in a thread with aPos = 0, data = 256 bytes and length = 255 bytes\n"));	
	readBuf100.Zero();//to ensure that the previous data is removed
	TUint pos = 0;
	TestRFile1.WriteU(pos,writeBuf100,255);
	TestRFile1.ReadU(pos,readBuf100,255);
	test(validateBuf100 == readBuf100);
	
	test.Next(_L("Write to a big file synchronously in a thread with aPos = 2GB -1, data = 1KB and length = 200 bytes\n"));	
	readBuf400.Zero();//to ensure that the previous data is removed
	pos = K2GBMinusOne;
	TestRFile1.WriteU(pos,writeBuf400,200);
	TestRFile1.ReadU(pos,readBuf400,200); 
	test(validateBuf400 == readBuf400);
	
	test.Next(_L("Write to a big file synchronously in a thread with aPos = 4GB-10 and data = 1KB and length = 10 bytes\n"));	
	readBuf1024.Zero();//to ensure that the previous data is removed
	pos = K4GBMinusTen;
	TestRFile1.WriteU(pos,writeBuf1024,9);
	TestRFile1.ReadU(pos,readBuf1024,9); 
	test(validateBuf1024 == readBuf1024);
	
	//
	//tests need to be repeated for current position variant of RFile64::Write()
	//testing with only current position as 4GB-2(boundary condition)
	//
	test.Next(_L("Write to a big file synchronously in a thread with aPos = 4GB-10 and data = 1KB and length = 10 bytes\n"));	
	readBuf1024.Zero();//to ensure that the previous data is removed
	TInt64 seekPos = K4GBMinusTen;
	TestRFile1.Seek(ESeekStart,seekPos);
	TestRFile1.Write(writeBuf1024,9);
	TestRFile1.Seek(ESeekStart,seekPos);
	TestRFile1.Read(readBuf1024,9); 
	test(validateBuf1024 == readBuf1024);
	
	
		
	test.Next(_L("Check for FAT32 file system. Write to a big file synchronously in a thread with aPos = 4GB, data length = 256 bytes and length = 10 bytes\n"));	
	if(KFileSizeMaxLargerThan4GBMinusOne == EFalse)
		{	
		TBuf8<0x100> writeBuf256;
		TBuf8<0x100> readBuf256;
		writeBuf256.Zero();
		writeBuf256.FillZ();	
		for (TInt count = 0; count < 0x100; count++)
			{
			writeBuf256.Append(count+6);
			}
		TestRFile1.Write(K4GB,writeBuf256,10);
		TestRFile1.Read(K4GB,readBuf256,10); 
		test_Equal(0, readBuf256.Length());
		}
	test.Next(_L("Write to a big file synchronously in a thread with aPos = 0 ,  data = 256 bytes and length =0 bytes\n"));	
	TBuf8<0x100> wrBuf256;
	TBuf8<0x100> reBuf256;
	wrBuf256.Zero();
	wrBuf256.FillZ();	
	for (count = 0; count < 0x100; count++)
		{
		wrBuf256.Append(count+6);
		}
	TestRFile1.Write(0,wrBuf256,0);
	TestRFile1.Read(0,reBuf256,0);
	test_Equal(0, reBuf256.Length());

	test.Next(_L("Write to a big file synchronously in a thread with aPos = 0 ,  data = 256 bytes and length = -1\n"));	
	TBuf8<0x100> wBuf256;
	wBuf256.Zero();
	wBuf256.FillZ();	
	for (count = 0; count < 0x100; count++)
		{
		wBuf256.Append(count+3);
		}
	TestRFile1.Write(0,wrBuf256,-1);
	TestRFile1.Close();
	
	TInt r = TheFs.Delete(fileName);
	test_KErrNone(r);
	}
		
/**
@SYMTestCaseID      PBASE-T_FILE64BIT-0776
@SYMTestPriority    High
@SYMTestRequirement REQ9526
@SYMTestType        CIT
@SYMTestCaseDesc    Tests for writing to a big file asynchronously with specified position and length
@SYMTestActions     
1) Write to a big file asynchronously in a thread, with aPos = 0, data = 256 bytes and length = 255 bytes 
2) Write to a big file asynchronously in a thread, with aPos = 2GB-1, data = 1KB and length = 200 bytes
3) Write to a big file asynchronously in a thread, with aPos = 4GB-10, data = 10 bytes and length = 10 bytes
4) Check for FAT32 file system. Write to a big file asynchronously in a thread, with aPos = 4GB+1, data = 256 bytes and length = 10 bytes
5) Write to a big file asynchronously in a thread, with aPos = 0, data = 256 bytes and length = 0 bytes
6) Write to a big file asynchronously in a thread, with aPos = 0, data = 256 bytes and length = -1bytes 
@SYMTestExpectedResults 
1) KErrNone, write is successful
2) KErrNone, write is successful
3) KErrNone, write is successful
4) KErrNotSupported. If NGFS is supported KErrNone and write is successful
5) KErrNone
6) KErrArgument
@SYMTestStatus      Implemented
*/
	
void TestOpenAndWriteAsyncLargeFileWithLen()
	{
	test.Next(_L("Open & Write Asynchronously Large File From Different Offset and length:"));

	TInt count;
	TFileName fileName;
	fileName.Append(gDriveToTest);
	fileName.Append(KTestPath);
	fileName.Append(_L("File4GBMinusOne.txt"));
	TestRFile1.Replace(fileName, EFileWrite);
	
	TestRFile1.SetSize(K4GBMinusOne);
	
	test.Next(_L("Write to a big file asynchronously in a thread, with aPos = 0, data = 256 bytes and length = 255 bytes \n"));
	TBuf8<0x100> writeBuf100;
	TBuf8<0x100> readBuf100;
	TBuf8<0x100> validateBuf100;
	writeBuf100.Zero();
	writeBuf100.FillZ();
	validateBuf100.Zero();
	for (count = 0; count < 0x100; count++)
		{
		writeBuf100.Append((TChar)count);
		if(count < 0xFF)
			validateBuf100.Append((TChar)count);
		}
	TRequestStatus status1 = KRequestPending;
	TestRFile1.Write(0,writeBuf100,255,status1);
	TestRFile1.Read(0,readBuf100,255);
	test(validateBuf100 == readBuf100);

	test.Next(_L("Write to a big file asynchronously in a thread, with aPos = 2GB-1, data = 1KB and length = 200 bytes\n"));
	TBuf8<0x400> writeBuf400;
	TBuf8<0x400> readBuf400;
	TBuf8<0x400> validateBuf400;
	writeBuf400.Zero();
	writeBuf400.FillZ();
	validateBuf400.Zero();
	for (count = 0; count < 0x400; count++)
		{
		writeBuf400.Append(count+20);
		if(count < 200)
			validateBuf400.Append(count+20);
		}
	TRequestStatus status2 = KRequestPending;
	TestRFile1.Write(K2GBMinusOne,writeBuf400,200,status2);
	TestRFile1.Read(K2GBMinusOne,readBuf400,200); 
	test(validateBuf400 == readBuf400);

	test.Next(_L("Write to a big file asynchronously in a thread, with aPos = 4GB-10, data = 10 bytes and length = 10 bytes\n"));
	TBuf8<0x0A> writeBuf0A;
	TBuf8<0x0A> readBuf0A;
	TBuf8<0x0A> validateBuf0A;
	writeBuf0A.Zero();
	readBuf0A.FillZ();
	validateBuf0A.Zero();	
	for (count = 0; count < 0x0A; count++)
		{
		writeBuf0A.Append(count+3);
		if(count<9)
			validateBuf0A.Append(count+3);
		}
	TRequestStatus status3 = KRequestPending;
	TestRFile1.Write(K4GBMinusTen,writeBuf0A,9,status3);
	TestRFile1.Read(K4GBMinusTen,readBuf0A,9);
	test(validateBuf0A == readBuf0A); 
	
	//tests need to be repeated for TUint variant of RFile64::Write()
	
	test.Next(_L("Write to a big file asynchronously in a thread, with aPos = 0, data = 256 bytes and length = 255 bytes \n"));
	readBuf100.Zero();//to ensure that the previous data is removed
	status1 = KRequestPending;
	TUint pos = 0;
	TestRFile1.WriteU(pos,writeBuf100,255,status1);
	TestRFile1.ReadU(pos,readBuf100,255);
	test(validateBuf100 == readBuf100);

	test.Next(_L("Write to a big file asynchronously in a thread, with aPos = 2GB-1, data = 1KB and length = 200 bytes\n"));
	readBuf400.Zero();//to ensure that the previous data is removed
	status2 = KRequestPending;
	pos = K2GBMinusOne;
	TestRFile1.Write(pos,writeBuf400,200,status2);
	TestRFile1.Read(pos,readBuf400,200); 
	test(validateBuf400 == readBuf400);

	test.Next(_L("Write to a big file asynchronously in a thread, with aPos = 4GB-10, data = 10 bytes and length = 10 bytes\n"));
	readBuf0A.Zero();//to ensure that the previous data is removed
	status3 = KRequestPending;
	pos = K4GBMinusTen;
	TestRFile1.Write(pos,writeBuf0A,9,status3);
	TestRFile1.Read(pos,readBuf0A,9);
	test(validateBuf0A == readBuf0A); 
	
	//
	//tests need to be repeated for current position variant of RFile64::Write()
	//testing with only current position as 4GB-2(boundary condition)
	//
	test.Next(_L("Write to a big file asynchronously in a thread, with aPos = 4GB-10, data = 10 bytes and length = 10 bytes\n"));
	readBuf0A.Zero();//to ensure that the previous data is removed
	status3 = KRequestPending;
	TInt64 seekPos = K4GBMinusTen;
	TestRFile1.Seek(ESeekStart,seekPos);
	TestRFile1.Write(writeBuf0A,9,status3);
	TestRFile1.Seek(ESeekStart,seekPos);
	TestRFile1.Read(readBuf0A,9);
	test(validateBuf0A == readBuf0A); 
	
	
	
	test.Next(_L("Check for FAT32 file system. Write to a big file asynchronously in a thread, with aPos = 4GB+1, data = 256 bytes and length = 10 bytes\n"));
	
	if(KFileSizeMaxLargerThan4GBMinusOne == EFalse)
		{	
		TBuf8<0x100> writeBuf256;
		TBuf8<0x100> readBuf256;
		writeBuf256.Zero();
		writeBuf256.FillZ();	
		for (TInt count = 0; count < 0x100; count++)
			{
			writeBuf256.Append(count+6);
			}
		TRequestStatus status5 = KRequestPending;
		TestRFile1.Write(K4GBPlusOne,writeBuf256,10,status5);
		TestRFile1.Read(K4GBPlusOne,readBuf256,10); 
		test_Equal(0, readBuf256.Length());
		}
	
	test.Next(_L("Write to a big file asynchronously in a thread, with aPos = 0, data = 256 bytes and length = 0 bytes\n"));	
	TBuf8<0x100> wrBuf256;
	TBuf8<0x100> reBuf256;
	wrBuf256.Zero();
	wrBuf256.FillZ();	
	for (count = 0; count < 0x100; count++)
		{
		wrBuf256.Append(count+6);
		}
	TRequestStatus status6 = KRequestPending;
	TestRFile1.Write(0,wrBuf256,0,status6);
	TestRFile1.Read(0,reBuf256,0); 
	test_Equal(0, reBuf256.Length());

	test.Next(_L("Write to a big file asynchronously in a thread, with aPos = 0, data = 256 bytes and length = -1bytes \n"));	
	TBuf8<0x100> wBuf256;
	wBuf256.Zero();
	wBuf256.FillZ();	
	for (count = 0; count < 0x100; count++)
		{
		wBuf256.Append(count+3);
		}
	TRequestStatus status7 = KRequestPending;
	TestRFile1.Write(0,wrBuf256,-1,status7);
	TestRFile1.Close();
	
	TInt r = TheFs.Delete(fileName);
	test_KErrNone(r);
	}
			
/**
@SYMTestCaseID      PBASE-T_FILE64BIT-0777
@SYMTestPriority    High
@SYMTestRequirement REQ9526
@SYMTestType        CIT
@SYMTestCaseDesc    Tests for locking a large file using RFile64::Lock()
@SYMTestActions     
1) Lock a big file with aPos = 0, aLength = 2GB-1
2) Lock a big file with aPos = 0, aLength = 2GB-1 ( i.e. again Lock with same parameters). This is to test multiple locks to same region.
3) Extend the Lock with aPos = 0, aLength  = 2GB+10.This tests overlapped locks.
4) Extend the Lock with aPos = 2GB-100, aLength = 200. This also tests overlapped locks.
5) Lock with aPos = 100, aLength = 2GB-100.This tries to lock sub region of a Lock
6) Lock same file with aPos = 2GB-1 and aLength = 200. Lock same file with aPos = 2GB + 300 and aLength =200.
7) Lock a big file with aPos = 0, aLength = 4GB-1.Tests boundary condition.
8) Lock a file with aPos =4GB and aLength=10
@SYMTestExpectedResults
1) KErrNone, lock is successful
2) KErrLocked, lock is unsuccessful
3) KErrLocked, lock is unsuccessful
4) KErrLocked, lock is unsuccessful
5) KErrLocked, lock is unsuccessful
6) KErrNone, lock is successful 
7) KErrLocked, lock is successful
8) KErrNone
@SYMTestStatus      Implemented
*/
	
void TestFileLock()
	{
	test.Next(_L("Tests for locking a big file:"));

	TFileName fileName;
	fileName.Append(gDriveToTest);
	fileName.Append(KTestPath);
	fileName.Append(_L("File4GBMinusOne.txt"));
	TestRFile1.Replace(fileName, EFileRead);
	
	test.Next(_L("Lock a big file with aPos = 0, aLength = 2GB-1\n"));
	TestRFile1.Lock(0, K2GBMinusOne);
		
	
	test.Next(_L("Attempt to lock the same region again\n"));
	TestRFile1.LockE(0, K2GBMinusOne);
	
	test.Next(_L("Extend the Lock with aPos = 0, aLength  = 2GB+10\n"));	
	TestRFile1.LockE(0, K2GBPlusTen);
	
	test.Next(_L("Extend the Lock with aPos = 2GB-100, aLength = 200\n"));	
	TestRFile1.LockE(K2GBMinus100, 200);
	
	test.Next(_L("Lock with aPos = 100, aLength = 2GB-100.\n"));	
	TestRFile1.LockE(100, K2GBMinus100);
	
	test.Next(_L("Lock same file with aPos = 2GB-1 and aLength = 200\n"));		
	TestRFile1.Lock(K2GBMinusOne, 200);
	
	test.Next(_L("Lock a big file with aPos = 0, aLength = 4GB-1\n"));			
	TestRFile1.LockE(0, K4GBMinusOne);
	
	
    if(KFileSizeMaxLargerThan4GBMinusOne)
    {	    
	    test.Next(_L("Lock a file with aPos =4GB and aLength=10\n"));			
	    TestRFile1.Lock(K4GB + 2, 10);
	}

	TestRFile1.Close();
	
	TInt r = TheFs.Delete(fileName);
	test_KErrNone(r);
	}


/**
@SYMTestCaseID      PBASE-T_FILE64BIT-0778
@SYMTestPriority    High
@SYMTestRequirement REQ9526
@SYMTestType        CIT
@SYMTestCaseDesc    Tests the File unlock functionality using RFile64::UnLock()
@SYMTestActions     
1) UnLock a big file, same region which is locked with aPos = 0, aLength = 2GB-1.
2) UnLock a big file, which is not locked with aPos = 0, aLength = 2GB-1.
3) UnLock a big file twice, same region which is locked with aPos = 0, aLength = 2GB-1.
4) UnLock a big file in a region which is sub region of Lock.  aPos = 10, aLength = 2GB-100. File should have been locked with aPos = 0, aLength = 2GB-1.
5) UnLock a big file in a region which is extended region of Lock, with aPos = 0, aLength = 2GB +100. File should have been locked with aPos = 0, aLength = 2GB-1.
6) UnLock a big file to test boundary condition, with aPos = 0, aLength = 4GB-1.The file should have been locked with same arguments.
7) UnLock different regions of a file which were locked in same order with aPos = 0, aLength = 2GB+200. 
						Second Unlock   aPos = 2GB+300 and aLength =200. 
						Third UnLock aPos = 2GB+600 and aLength = 200.
8) UnLock different regions of a file which were locked in different order with aPos = 0, aLength = 2GB+100. 
						Second Unlock   aPos = 2GB+300 and aLength =200. 
						Third UnLock aPos = 2GB+600 and aLength = 200.
9) UnLock multiple locked regions with aPos = 0, aLength = 2GB-1.The file should have been locked in same region but with different locks  
10)Unlock a locked file with aPos = 4GB and aLength = 10bytes

@SYMTestExpectedResults 
1) KErrNone
2) KErrNotFound
3) KErrNotFound
4) KErrNotFound
5) KErrNotFound
6) KErrNone 
7) KErrNone
8) KErrNone
9) KErrNotFound
10)KErrNone
@SYMTestStatus      Implemented
*/
	
void TestFileUnlock()
	{
	test.Next(_L("Tests for Unlocking a big file:\n"));

	TFileName fileName;
	fileName.Append(gDriveToTest);
	fileName.Append(KTestPath);
	fileName.Append(_L("File4GBMinusOne.txt"));
	TestRFile2.Replace(fileName, EFileRead);
	
	test.Next(_L("UnLock a big file, same region which is locked with aPos = 0, aLength = 2GB-1.\n"));
	TestRFile2.Lock(0, K2GBMinusOne);
	TestRFile2.UnLock(0, K2GBMinusOne);
	TestRFile2.UnLockE(0, K2GBMinusOne);
	
	test.Next(_L("UnLock a big file, which is not locked with aPos = 0, aLength = 2GB-1\n"));	
	TestRFile2.UnLockE(0, K2GBMinusOne);
	
	test.Next(_L("UnLock a big file twice, same region which is locked with aPos = 0, aLength = 2GB-1\n"));		
	TestRFile2.Lock(0, K2GBMinusOne);
	TestRFile2.UnLockE(10, K2GBMinus100);
	TestRFile2.UnLock(0, K2GBMinusOne);
	
	test.Next(_L("UnLock a big file in a region which is sub region of Lock.  aPos = 10, aLength = 2GB-100. File should have been locked with aPos = 0, aLength = 2GB-1\n"));			
	TestRFile2.Lock(0, K2GBMinusOne);
	TestRFile2.UnLockE(10, K2GBMinus100); 
	TestRFile2.UnLock(0, K2GBMinusOne);
	
	test.Next(_L("UnLock a big file in a region which is extended region of Lock, with aPos = 0, aLength = 2GB +100. File should have been locked with aPos = 0, aLength = 2GB-1.\n"));
	TestRFile2.Lock(0, K2GBMinusOne);
	TestRFile2.UnLockE(10, K2GBPlus100);
	TestRFile2.UnLock(0, K2GBMinusOne);

	test.Next(_L("UnLock a big file to test boundary condition, with aPos = 0, aLength = 4GB-1.The file should have been locked with same arguments\n"));	
	TestRFile2.Lock(0, K4GBMinusOne);
	TestRFile2.UnLock(0, K4GBMinusOne);
	
	test.Next(_L("UnLock different regions of a file which were locked in same order with aPos = 0, aLength = 2GB+200\n"));	
	TestRFile2.Lock(0, K2GBPlus200);
	TestRFile2.Lock(K2GBPlus300, 200);
	TestRFile2.Lock(K2GBPlus600, 200);
	TestRFile2.UnLock(0, K2GBPlus200);
	TestRFile2.UnLock(K2GBPlus300, 200);
	TestRFile2.UnLock(K2GBPlus600, 200);
		
	test.Next(_L("UnLock different regions of a file which were locked in different order with aPos = 0, aLength = 2GB+100\n"));	
	TestRFile2.Lock(0, K2GBPlus100);
	TestRFile2.Lock(K2GBPlus600, 200);
	TestRFile2.Lock(K2GBPlus300, 200);                                        
	TestRFile2.UnLock(K2GBPlus600, 200);
	TestRFile2.UnLock(K2GBPlus300, 200);
	TestRFile2.UnLock(0, K2GBPlus100); 
	
	test.Next(_L("UnLock multiple locked regions with aPos = 0, aLength = 2GB-1.The file should have been locked in same region but with different locks\n"));	
	TestRFile2.Lock(0, 100);
	TestRFile2.Lock(100, K2GBMinusOne);
	TestRFile2.UnLockE(0, K2GBMinusOne);

		
    if(KFileSizeMaxLargerThan4GBMinusOne)
        {	    
		
	test.Next(_L("Unlock a locked file with aPos = 4GB and aLength = 10bytes\n"));		
	TestRFile2.Lock(K4GB, 10);	
	TestRFile2.UnLock(K4GB, 10);	
	    }

    TestRFile2.Close();
	TInt r = TheFs.Delete(fileName);
	test_KErrNone(r);
	}

/**
@SYMTestCaseID      PBASE-T_FILE64BIT-0779
@SYMTestPriority    High
@SYMTestRequirement REQ9526
@SYMTestType        CIT
@SYMTestCaseDesc    Tests for file seek operation using RFile64::Seek()
@SYMTestActions     
1) Set the file size as 20
2) Seek mode = ESeekEnd, seek position = 80, call RFile64::Seek() 
3) Check Seek position
4) Set file size = 512
5) Seek mode =ESeekStart, assign seek position =513, get the seek position using RFile64::Seek()
6) Check the seek position
7) Seek mode = ESeekEnd, get the seek position using RFile64::Seek()
8) Check the seek position
9) Seek position =-530, seek mode = ESeekEnd, Get the seek position using RFile64::Seek()
10)Check the seek position
11)Seek position =-10, seek mode = ESeekEnd, get the seek position using RFile64::Seek()
12)Seek position =-10, seek mode = ESeekStart, get the seek position using RFile64::Seek()
13)Seek position =0, seek mode = ESeekEnd, get the seek position using RFile64::Seek()
@SYMTestExpectedResults 
1) KErrNone
2) KErrNone
3) Seek position = 20
4) KErrNone
5) KErrNone
6) Seek position = 513
7) KErrNone 
8) Seek position = 512
9) KErrNone
10)Seekposition = 0
11)Seek position =502
12)KErrArgument, seek position unchanged
13)Seek position =512
@SYMTestStatus      Implemented
*/
	
void TestFileSeek()
	{
	TInt64 seekPos;
	
	TFileName fileName;
	fileName.Append(gDriveToTest);
	fileName.Append(KTestPath);
	fileName.Append(_L("seektest.txt"));
	TestRFile1.Replace(fileName);


	test.Next(_L("Set the file size as 20\n"));			
	TestRFile1.SetSize(20);
	
	test.Next(_L("Seek mode = ESeekEnd, seek position = 80, call RFile64::Seek() "));
	seekPos = 80;								
    TestRFile1.Seek(ESeekEnd, seekPos);	
    test_Equal(20, seekPos);				

	test.Next(_L("Set the file size as 512\n"));			
	TestRFile1.SetSize(512);
	
	test.Next(_L("Seek mode =ESeekStart, assign seek position =513, get the seek position using RFile64::Seek()\n"));			
	seekPos = 513;
	TestRFile1.Seek(ESeekStart, seekPos);
	test_Equal(513, seekPos);

	test.Next(_L("Seek mode = ESeekEnd, get the seek position using RFile64::Seek()\n"));			
	TestRFile1.Seek(ESeekEnd, seekPos);
	test_Equal(512, seekPos);

	test.Next(_L("Seek position =-530, seek mode = ESeekEnd, Get the seek position using RFile64::Seek()\n"));			
	seekPos = -530;
	TestRFile1.Seek(ESeekEnd, seekPos);
	test_Equal(0, seekPos);

	test.Next(_L("Seek position =-10, seek mode = ESeekEnd, get the seek position using RFile64::Seek()\n"));	
	seekPos = -10;
	TestRFile1.Seek(ESeekEnd, seekPos);
	test_Equal(502, seekPos);

	test.Next(_L("Seek position =-10, seek mode = ESeekStart, get the seek position using RFile64::Seek()\n"));	
	seekPos = -10;
	TestRFile1.Seek(ESeekStart,seekPos);
	test_Equal(-10, seekPos);

	test.Next(_L("Seek position =0, seek mode = ESeekEnd, get the seek position using RFile64::Seek()\n"));	
	seekPos = 0;
	TestRFile1.Seek(ESeekEnd,seekPos);
	test_Equal(512, seekPos);

	TestRFile1.Close();
	
	TInt r = TheFs.Delete(fileName);
	test_KErrNone(r);
	}

/**
@SYMTestCaseID      PBASE-T_FILE64BIT-0780
@SYMTestPriority    High
@SYMTestRequirement REQ9526
@SYMTestType        CIT
@SYMTestCaseDesc    Test file seek operation for large file
@SYMTestActions     
1) Set the file size as 2GB-1
2) Seek mode = ESeekEnd, seek position = 2GB+80, call RFile64::Seek()  
3) Check Seek position
4) Set file size = 4GB -1
5) Seek mode = ESeekStart, assign seek position = 4GB-1, get the seek position using RFile64::Seek()
6) Check the seek position
7) Seek mode = ESeekEnd, get the seek position using RFile64::Seek()
8) Check the seek position
9) Seek position = (4GB), seek mode = ESeekEnd, Get the seek position using RFile64::Seek()
10)Check the seek position
11)Seek position =-10, seek mode = ESeekEnd, get the seek position using RFile64::Seek()
12)Seek position =-10, seek mode = ESeekStart, get the seek position using RFile64::Seek()
13)Seek position =0, seek mode = ESeekEnd, get the seek position using RFile64::Seek()
@SYMTestExpectedResults 
1) KErrNone
2) KErrNone
3) Seek position = 2GB-1
4) KErrNone
5) KErrNone
6) Seek position = 4GB-1
7) KErrNone 
8) Seek position = 4GB-1
9) KErrNone
10)Seekposition = 0
11)Seek position =4GB-10
12)KErrArgument, seek position unchanged
13)Seek position =4GB - 1
@SYMTestStatus      Implemented
*/
	
void TestFileSeekBigFile()
	
	{
	TInt64 seekPos;
	
	
	TFileName fileName;
	fileName.Append(gDriveToTest);
	fileName.Append(KTestPath);
	fileName.Append(_L("File4GBMinusOne.txt"));
	TestRFile1.Replace(fileName, EFileRead|EFileWrite);
	
	test.Next(_L("Set the file size as 2GB-1\n"));	
	TestRFile1.SetSize(K2GBMinusOne);
	
	test.Next(_L("Seek mode = ESeekEnd, seek position = 2GB+80, call RFile64::Seek()\n"));		
	seekPos = K2GBPlus80;
    TestRFile1.Seek(ESeekEnd, seekPos);
    test(seekPos == K2GBMinusOne);
	
	test.Next(_L("Set the file size to 4GB-1\n"));	
	TestRFile1.SetSize(K4GBMinusOne);
	
	test.Next(_L("Seek mode = ESeekStart, assign seek position = 4GB-1, get the seek position using RFile64::Seek()\n"));	
	seekPos = K4GBMinusOne;
	TestRFile1.Seek(ESeekStart, seekPos);
	test(seekPos == K4GBMinusOne);

	test.Next(_L("Seek mode = ESeekEnd, get the seek position using RFile64::Seek()\n"));	
	TestRFile1.Seek(ESeekEnd, seekPos);
	test(seekPos == K4GBMinusOne);
	
	if(KFileSizeMaxLargerThan4GBMinusOne)
		{
		TestRFile1.SetSize(K4GB);
		TestRFile1.Seek(ESeekEnd, seekPos);
		test(seekPos == K4GB);
		seekPos = -10;
		TestRFile1.Seek(ESeekEnd, seekPos);
		test(seekPos == K4GB-10);
		seekPos = -10;
		TestRFile1.Seek(ESeekStart,seekPos);
		test(seekPos == -10);
		seekPos = 0;
		TestRFile1.Seek(ESeekEnd,seekPos);
		test(seekPos == K4GB);
		}
	else
		{
		TestRFile1.SetSize(K4GB);
		TestRFile1.Seek(ESeekEnd, seekPos);
		test(seekPos == K4GBMinusOne);
		seekPos = -10;
		TestRFile1.Seek(ESeekEnd, seekPos);
		test(seekPos == K4GBMinusOne-10);
		seekPos = -10;
		TestRFile1.Seek(ESeekStart,seekPos);
		test(seekPos == -10);
		seekPos = 0;
		TestRFile1.Seek(ESeekEnd,seekPos);
		test(seekPos == K4GBMinusOne);
		}
		

	TestRFile1.Close();
	
	TInt r = TheFs.Delete(fileName);
	test_KErrNone(r);
}

/**
@SYMTestCaseID      PBASE-T_FILE64BIT-0781
@SYMTestPriority    High
@SYMTestRequirement REQ9527
@SYMTestType        CIT
@SYMTestCaseDesc    Test RFile64::SetSize() and RFile64::Size() functionality
@SYMTestActions     
1) Set the file size =128KB
2) Write a test data = "ABCDEFGH", at position = 0
3) Get the file size
4) Read the data from position = 0
5) Compare the read data with written data
6) Set the file size to = 2GB-1
7) Write test data = "IJKLMnOPxY IJKLMnOPx",  length=20 bytes, at position 2GB-10
8) Get the file size
9) Read the data from the position 2GB-10
10)Compare the read data
11)Set the file size = 4GB-1
12)Write test data = "IJKLMnOPxY IJKLMnOPx",  length=10 bytes, at position 4GB-10
13)Get the file size
14)Read the data from the position 4GB-10
15)Compare the read data
@SYMTestExpectedResults 
1) KErrNone
2) KErrNone, write is successful
3) KErrNone, File Size = 128KB 
4) KErrNone, read is successful
5) Read data == Written data
6) KErrNone
7) KErrNone, write is successful
8) KErrNone File Size = 2GB+10
9) KErrNone, read is successful
10)Read data == Written data
11)KErrNone
12)KErrNone, write is successful for 10 bytes
13)KErrNone File Size == 4GB-1
14)KErrNone, read is successful
15)Read data == Written data
@SYMTestStatus      Implemented
*/
	
void TestSetsize()
	{
	test.Next(_L("Create a large file"));
	
	TFileName fileName;
	fileName.Append(gDriveToTest);
	fileName.Append(KTestPath);
	fileName.Append(_L("File4GBMinusOne.txt"));
	TestRFile1.Replace(fileName, EFileRead|EFileWrite);
	
	CheckDisk();
	
	test.Next(_L("Set the file size =128KB\n"));
	TestRFile1.SetSize(131072); // 128KB
	
	test.Next(_L("Write a test data = ABCDEFGH, at position = 0\n"));
	TBuf8<16> testData = _L8("ABCDEFGH");
	TestRFile1.WriteP(0,testData);
	TInt64 size = 0;
	
	test.Next(_L("Get the file size\n"));
	TestRFile1.Size(size);
	test_Equal(131072, size);
	
	test.Next(_L("Read and compare the data from position = 0\n"));
	TBuf8<16> testData2;
	TestRFile1.Read(0,testData2,8);
	test(testData == testData2);
	
	test.Next(_L("Set the file size =2GB - 1 \n"));
	TestRFile1.SetSize(K2GBMinusOne); // 2GB-1
	
	test.Next(_L("Write test data = IJKLMnOPxY IJKLMnOPx ,length=20 bytes, at position 2GB-10\n"));
	TBuf8<20> testData3 = _L8("IJKLMnOPxY IJKLMnOPx");
	TestRFile1.Write(K2GBMinusTen,testData3, 20);
	
	test.Next(_L("Get the file size\n"));
	TestRFile1.Size(size);
	test(size == K2GBPlusTen);
	
	test.Next(_L("Read and compare the data from position = 2GB-10\n"));
	TBuf8<10> testData4;
	TestRFile1.Read(K2GBMinusTen,testData4,10);
	test(testData4 == _L8("IJKLMnOPxY"));

	test.Next(_L("Set the file size =2GB - 1 \n"));
	TestRFile1.SetSize(K4GBMinusOne); // 4GB-1
	
	test.Next(_L("Write test data = IJKLMnOPxY IJKLMnOPx,  length=10 bytes, at position 4GB-10\n"));
	TBuf8<20> testData5 = _L8("IJKLMnOPxY IJKLMnOPx");
	TestRFile1.Write(K4GBMinusTen,testData5,9);
	
	test.Next(_L("Get the file size\n"));
	TestRFile1.Size(size);
	test(size == K4GBMinusOne);
	
	test.Next(_L("Read the data from the position 4GB-10\n"));
	TBuf8<10> testData6;
	TestRFile1.Read(K4GBMinusTen,testData6,9);
	test(testData6 == _L8("IJKLMnOPx"));
	TestRFile1.Close();
	
	TInt r = TheFs.Delete(fileName);
	test_KErrNone(r);
	}

/**
@SYMTestCaseID      PBASE-T_FILE64BIT-0782
@SYMTestPriority    High
@SYMTestRequirement REQ9528
@SYMTestType        CIT
@SYMTestCaseDesc    Tests for reading a data from a big file without opening it
@SYMTestActions     
1) Read from a big file using RFs::ReadFileSection() from position 3GB-1,52byte lengths of data
2) Open a big file in EFileShareAny | EFileRead mode and read from it using RFs::ReadFileSection() from position 3GB-1, 52byte lengths of data
3) Open a big file in EFileShareExclusive | EFileRead mode and read from it using RFs::ReadFileSection( ) from position 3GB-1, 52byte lengths of data
4) Open a big file in EFileShareExclusive | EFileWrite mode and read from it using RFs::ReadFileSection( ) from position 3GB-1, 52byte lengths of data
5) Check for FAT32 file system. Read from a big file using RFs::ReadFileSection( ) from position equal to 4GB, 52byte lengths of data
@SYMTestExpectedResults 
1) KErrNone, read is successful
2) KErrNone, open and read both are successful
3) KErrNone, open and read both are successful
4) KErrNone, open and read both are successful
5) KErrNone with zero length descriptor,if NGFS is supported we should get the valid data
@SYMTestStatus      Implemented
*/
void TestReadFilesection()
	{
	test.Next(_L("Read data from a large file using RFs::ReadFileSection\n"));
	TBuf8<52> testDes;
	TBuf8<52>  readBuf;
	
	RFile64 file;
		
	TFileName fileName;
	fileName.Append(gDriveToTest);
	fileName.Append(KTestPath);
	fileName.Append(_L("File4GBMinusOne.txt"));
	
	TInt r = file.Replace(TheFs,fileName,EFileWrite);
	test_KErrNone(r);
	r = file.SetSize(K4GBMinusOne);
	test_KErrNone(r);
	file.Close();
	
	test.Next(_L("Read from a big file using RFs::ReadFileSection() from position 3GB-1,52byte lengths of data\n"));
	TestRFs.ReadFileSection(fileName,K3GBMinusOne,testDes,52);

	test.Next(_L("Open a big file in EFileShareAny | EFileRead mode and read from it using RFs::ReadFileSection() from position 3GB-1, 52byte lengths of data\n"));
	TestRFile1.Open(fileName,EFileShareAny|EFileRead);
	TestRFs.ReadFileSection(fileName,0,testDes,52);
	test_Equal(52, testDes.Length());
	TestRFile1.Close();
	
	test.Next(_L("Open a big file in EFileShareExclusive | EFileRead mode and read from it using RFs::ReadFileSection( ) from position 3GB-1, 52byte lengths of data\n"));
	TestRFile1.Open(fileName,EFileShareExclusive|EFileRead);
	TestRFs.ReadFileSection(fileName,K3GBMinusOne,testDes,52);
	TestRFile1.Close();
	
	test.Next(_L("Open a big file in EFileShareExclusive | EFileWrite mode and read from it using RFs::ReadFileSection( ) from position 3GB-1, 52byte lengths of data\n"));
	TestRFile1.Open(fileName,EFileShareExclusive|EFileWrite);
	TestRFs.ReadFileSection(fileName,K3GBMinusOne,testDes,52);
	TestRFile1.Close();
	
	
	test.Next(_L("Check for FAT32 file system. Read from a big file using RFs::ReadFileSection( ) from position equal to 4GB, 52byte lengths of data\n"));
		
	if(KFileSizeMaxLargerThan4GBMinusOne == EFalse)
		{
		TestRFs.ReadFileSection(fileName,K4GB,readBuf,52);
		}
		
	r = TheFs.Delete(fileName);
	test_KErrNone(r);
	}
	
/**
@SYMTestCaseID      PBASE-T_FILE64BIT-0783
@SYMTestPriority    High
@SYMTestRequirement REQ9530
@SYMTestType        CIT
@SYMTestCaseDesc    Check that we can get a valid directory listing of a directory containing large files using RDir and then CDir
					TInt RFs::GetDir(const TDesC& aName,TUint anEntryAttMask,TUint anEntrySortKey,CDir*& anEntryList) const;
@SYMTestActions     
1) Get the directory listing, sort by size
2) Check the files count in the directory. Number of files in a directory is 4
3) Get the entry list & Check the files are listed in order of file sizes
@SYMTestExpectedResults 
1) KErrNone
2) 4 Files in the directory
3) File size should match and arranged in ascending order
@SYMTestStatus      Implemented
*/
void TestGetDirectory()
	{
	test.Next(_L("Read a directory containing large files using RDir"));

	TFileName dirName;
	dirName.Append(gDriveToTest);
	dirName.Append(KTestPath);
		
	TFileName file4GBMinusOne;
	file4GBMinusOne.Append(gDriveToTest);
	file4GBMinusOne.Append(KTestPath);
	file4GBMinusOne.Append(_L("File4GBMinusOne.txt"));
	TFileName file2GBMinusOne;
	file2GBMinusOne.Append(gDriveToTest);
	file2GBMinusOne.Append(KTestPath);
	file2GBMinusOne.Append(_L("File2GBMinusOne.txt"));
	TFileName file2GB;
	file2GB.Append(gDriveToTest);
	file2GB.Append(KTestPath);
	file2GB.Append(_L("File2GB.txt"));
	TFileName file3GB;
	file3GB.Append(gDriveToTest);
	file3GB.Append(KTestPath);
	file3GB.Append(_L("File3GB.txt"));

	TestRFile1.Replace(file4GBMinusOne);
	TestRFile1.SetSize(K4GBMinusOne);
	TestRFile1.Close();
	
	TestRFile1.Replace(file2GBMinusOne);
	TestRFile1.SetSize(K2GBMinusOne);
	TestRFile1.Close();
	
	TestRFile1.Replace(file2GB);
	TestRFile1.SetSize(K2GB);
	TestRFile1.Close();
	
	TestRFile1.Replace(file3GB);
	TestRFile1.SetSize(K3GB);
	TestRFile1.Close();
		
	test.Next(_L("Get the directory listing, sort by size\n"));
	RDir dir;
	TInt r = dir.Open(TheFs, dirName, KEntryAttNormal);
	test_KErrNone(r);
	
	TEntryArray entryArray;
	r = dir.Read(entryArray);
	test_Value(r, r == KErrEof);

	test.Next(_L("Check the files count in the directory. Number of files in a directory is 4\n"));
	test_Equal(gFilesInDirectory, entryArray.Count());
	
	test.Next(_L("Get the entry list & Check the files are listed in order of file sizes\n"));
	TInt n;
	for (n = 0; n<entryArray.Count(); n++)
		{
		const TEntry& entry = entryArray[n];
		if (entry.iName.MatchF(KFile2GBMinusOne()) == 0)
			test(entry.FileSize() == K2GBMinusOne);
		else if (entry.iName.MatchF(KFile2GB()) == 0)
			test(entry.FileSize() == K2GB);
		else if (entry.iName.MatchF(KFile3GB()) == 0)
			test(entry.FileSize() == K3GB);
		else if (entry.iName.MatchF(KFile4GBMinusOne()) == 0)
			test(entry.FileSize() == K4GBMinusOne);
		else
			test(EFalse);
		}

	dir.Close();

	test.Next(_L("Read a directory containing large files using CDir & sort by size"));
	CDir* dirList = NULL;
	TestRFs.GetDir(dirName, KEntryAttMaskSupported, ESortBySize, dirList);
	test_Equal(gFilesInDirectory, dirList->Count());
	for (n = 0; n<dirList->Count(); n++)
		{
		TEntry entry;
		entry = (*dirList)[n];
		if (entry.iName.MatchF(KFile2GBMinusOne()) == 0)
			test(entry.FileSize() == K2GBMinusOne);
		else if (entry.iName.MatchF(KFile2GB()) == 0)
			test(entry.FileSize() == K2GB);
		else if (entry.iName.MatchF(KFile3GB()) == 0)
			test(entry.FileSize() == K3GB);
		else if (entry.iName.MatchF(KFile4GBMinusOne()) == 0)
			test(entry.FileSize() == K4GBMinusOne);
		else
			test(EFalse);
		}
	delete dirList;
	dirList = NULL;
	}
	

/**
@SYMTestCaseID      PBASE-T_FILE64BIT-0784
@SYMTestPriority    High
@SYMTestRequirement REQ9533
@SYMTestType        CIT
@SYMTestCaseDesc    Tests functionality of TEntry
@SYMTestActions     
1) Set the File Size to 4GB-1 using RFile64::SetSize()
2) Get the entry
3) Get the file size, using TEntry::FileSize()
4) Get the file size using iSize (i.e. without type cast to TUint)
5) Check for FAT32 file system. Set the file size to 4GB
6) Get the file size, using TEntry::FileSize()
7) Compare the File size with expected size
@SYMTestExpectedResults 
1) KErrNone
2) KErrNone
3) File size = 4GB-1
4) File size = -1
5) KErrNotSupported for FAT32 and KErrNone for NGFS
6) FAT32 file size = 4GB-1 and NGFS file size = 4GB
7) File size = 4GB-1
@SYMTestStatus      Implemented
*/
void TestTEntry()	
	{
	test.Next(_L("Tests functionality for TEntry"));
	
	TFileName fileName;
	fileName.Append(gDriveToTest);
	fileName.Append(KTestPath);
	fileName.Append(_L("File4GBMinusOne.txt"));
	TestRFile1.Replace(fileName, EFileRead|EFileWrite);
	
	CDir* anEntryList;
	TEntry entry;
	TInt64 size = 0;
	
	test.Next(_L("Set the File Size to 4GB-1 using RFile64::SetSize()\n"));
	TestRFile1.SetSize(K4GBMinusOne);
	
	test.Next(_L("Get the entry\n"));
	TestRFs.GetDir(fileName, KEntryAttMaskSupported, ESortBySize, anEntryList);
	for (TInt n = 0; n<anEntryList->Count(); n++)
		{
		entry = (*anEntryList)[n];
		if (entry.iName.MatchF(KFile4GBMinusOne) == 0)
			{
			test(entry.FileSize() == K4GBMinusOne);
			}
		}
		
	test.Next(_L("Get the file size, using TEntry::FileSize()\n"));
	size = entry.FileSize();
	test(size == K4GBMinusOne);
	test_Equal(-1, entry.iSize);
	
	
	
	if(KFileSizeMaxLargerThan4GBMinusOne == EFalse)
		{
		TestRFile1.SetSize(K4GB);
		size = entry.FileSize();
		test(size == K4GBMinusOne);	
		}
	TestRFile1.Close();
	delete anEntryList;
	anEntryList = NULL;
	}
	
/**
@SYMTestCaseID      PBASE-T_FILE64BIT-0785
@SYMTestPriority    High
@SYMTestRequirement REQ9530
@SYMTestType        CIT
@SYMTestCaseDesc    Test the RDir read functionality with large file
@SYMTestActions     
1) Open the directory containing large file, using RDir open()
2) Read the directory entry using TEntryArray as parameter 
3) Check the count
4) Close using RDir 
@SYMTestExpectedResults 
1) KErrNone, open is successful
2) KErrEof
3) count = 4 files
4) Closes the directory
@SYMTestStatus      Implemented
*/
	
void TestReadDirectory()
	{
	test.Next(_L("RDir::Read()"));
	
	TFileName dirName;
	dirName.Append(gDriveToTest);
	dirName.Append(KTestPath);
	
	test.Next(_L("Open the directory containing large file, using RDir open()\n"));
	RDir dir;
	TInt r = dir.Open(TheFs, dirName, KEntryAttNormal);
	test_KErrNone(r);
	
	test.Next(_L("Read the directory entry using TEntryArray as parameter\n"));
	TEntryArray entryArray;
	r = dir.Read(entryArray);
	test_Value(r, r == KErrEof);
	
	test.Next(_L("Check the count\n"));
	test_Equal(gFilesInDirectory, entryArray.Count());
	
	test.Next(_L("Close using RDir\n"));
	dir.Close();
	}

/**
@SYMTestCaseID      PBASE-T_FILE64BIT-0786
@SYMTestPriority    High
@SYMTestRequirement REQ9530
@SYMTestType        CIT
@SYMTestCaseDesc    Test the sorting of directory entries using CDir::Sort()
@SYMTestActions     
1) Sort with number of entries =0
2) Sort the directory entries with large files, sort key = ESortBySize
3) Get the entries count
4) Check the files are arranged in increasing file size
@SYMTestExpectedResults 
1) KErrNone
2) KErrNone, sort is successful
3) count = 4
4) sequence should be in increasing order
@SYMTestStatus      Implemented
*/
	
void TestSortDirectory()
	{
	CDir* anEntryList;
	TEntry entry;
	
	
	TFileName testDir0;
	testDir0.Append(gDriveToTest);
	testDir0.Append(_L("F32-TEST"));
	
	TInt r = TheFs.MkDir(testDir0);
	test_Value(r, r == KErrNone || r == KErrAlreadyExists);
	
	test.Next(_L("Sort with number of entries =0\n"));
	TestRFs.GetDir(testDir0, KEntryAttMaskSupported, ESortBySize, anEntryList);
	test_Equal(0, anEntryList->Count());
	delete anEntryList;
	anEntryList = NULL;
	
	test.Next(_L("	Sort the directory entries with large files, sort key = ESortBySize\n"));
	TFileName testDir;
	testDir.Append(gDriveToTest);
	testDir.Append(KTestPath);
	CDir* aDirList;
	TestRFs.GetDir(testDir, KEntryAttMaskSupported, ESortBySize, aDirList);
	
	test.Next(_L("Get the entries count\n"));
	test_Equal(gFilesInDirectory, aDirList->Count());
	
	
	test.Next(_L("Check the files are arranged in increasing file size\n"));
	for (TInt n = 0; n<aDirList->Count(); n++)
		{
		entry = (*aDirList)[n];
		if (entry.iName.MatchF(KFile2GBMinusOne) == 0)
			{
			test(entry.FileSize() == K2GBMinusOne);
			test_Equal(0, n);
			}
		else if (entry.iName.MatchF(KFile2GB()) == 0)
			{
			test(entry.FileSize() == K2GB);
			test_Equal(1, n);
			}
		else if (entry.iName.MatchF(KFile3GB) == 0)
			{
			test(entry.FileSize() == K3GB);
			test_Equal(2, n);
			}
		else if (entry.iName.MatchF(KFile4GBMinusOne) == 0)
			{
			test(entry.FileSize() == K4GBMinusOne);
			test_Equal(3, n);
			}
		else
			test(EFalse);
		}
	delete aDirList;
	aDirList = NULL;
	}	

/**
@SYMTestCaseID      PBASE-T_FILE64BIT-0787
@SYMTestPriority    High
@SYMTestRequirement REQ9530
@SYMTestType        CIT
@SYMTestCaseDesc    Test cases for validating CDir::AddL()
@SYMTestActions     
1) Fill the directory entry with details of large files contained in them
2) Get the directory entry,using RFs::GetDir()
3) Compare with entry added
@SYMTestExpectedResults 
1) KErrNone
2) KErrNone
3) Added entry == retrieved entry
@SYMTestStatus      Implemented
*/
void TestAddLDirectory()
	{
	CDir* aDirList;
	TEntry entry;
	
	TFileName testDir;
	testDir.Append(gDriveToTest);
	testDir.Append(KTestPath);	
	
	test.Next(_L("Get the directory entry,using RFs::GetDir()\n"));
	TestRFs.GetDir(testDir, KEntryAttMaskSupported, ESortBySize, aDirList);
	test_Equal(gFilesInDirectory, aDirList->Count());

	test.Next(_L("Compare with entry added\n"));
	for (TInt n = 0; n<aDirList->Count(); n++)
		{
		entry = (*aDirList)[n];
		if (entry.iName.MatchF(KFile2GBMinusOne) == 0)
			{
			test(entry.FileSize() == K2GBMinusOne);
			test_Equal(0, n);
			}
		else if (entry.iName.MatchF(KFile2GB()) == 0)
			{
			test(entry.FileSize() == K2GB);
			test_Equal(1, n);
			}
		else if (entry.iName.MatchF(KFile3GB) == 0)
			{
			test(entry.FileSize() == K3GB);
			test_Equal(2, n);
			}
		else if (entry.iName.MatchF(KFile4GBMinusOne) == 0)
			{
			test(entry.FileSize() == K4GBMinusOne);
			test_Equal(3, n);
			}
		else
			test(EFalse);
		}
		delete aDirList;
		aDirList = NULL;
		
	TFileName file4GBMinusOne;
	file4GBMinusOne.Append(gDriveToTest);
	file4GBMinusOne.Append(KTestPath);
	file4GBMinusOne.Append(_L("File4GBMinusOne.txt"));
	TFileName file2GBMinusOne;
	file2GBMinusOne.Append(gDriveToTest);
	file2GBMinusOne.Append(KTestPath);
	file2GBMinusOne.Append(_L("File2GBMinusOne.txt"));
	TFileName file2GB;
	file2GB.Append(gDriveToTest);
	file2GB.Append(KTestPath);
	file2GB.Append(_L("File2GB.txt"));
	TFileName file3GB;
	file3GB.Append(gDriveToTest);
	file3GB.Append(KTestPath);
	file3GB.Append(_L("File3GB.txt"));
	
	TInt r = TheFs.Delete(file4GBMinusOne);
	test_KErrNone(r);
	r = TheFs.Delete(file2GBMinusOne);
	test_KErrNone(r);
	r = TheFs.Delete(file2GB);
	test_KErrNone(r);
	r = TheFs.Delete(file3GB);
	test_KErrNone(r);
	}
	
/**
@SYMTestCaseID      PBASE-T_FILE64BIT-0788
@SYMTestPriority    High
@SYMTestRequirement REQXXXX
@SYMTestType        CIT
@SYMTestCaseDesc    Test cases for validating TFileText changes.
@SYMTestActions     
1) Open test file and get the file size using RFile64::Size() and set the file handle to TFileText object
2) Seek to the file end using TFileText::Seek()
3) Get current file position using RFile64::Seek() and verify it is at file end.
4) Seek to location greater than 2GB-1 using RFile64::Seek
5) Write data to the file using RFile64::Write
6) Read data using TFileText::Read
7) Compare the data read in steps 6 to the data written in step 5.
8) Seek to the file end using TFileText::Seek(ESeekEnd).
9) Write known data using TFileText::Write
10) Read the data using RFile64::Read
11) Compare the source data with read data.
@SYMTestExpectedResults 
1) KErrNone
2) KErrNone
3) Current file position is file end.
4) KErrNone
5) KErrNone
6) KErrNone
7) Read data == Written data
8) KErrNone
9) KErrNone
10) KErrNone
11) Read data == Source data
@SYMTestStatus      Implemented
*/	
void TestTFileText()
	{
	TFileName fileName;
	fileName.Append(gDriveToTest);
	fileName.Append(KTestPath);
	fileName.Append(_L("test.txt"));
	TInt r;
	RFile64 file64;
	TInt64 sizeK3GB = K3GB;
	
	test.Next(_L("Open test file and get the file size using RFile64::Size() and set the file handle to TFileText object\n"));
	r = file64.Replace(TheFs,fileName,EFileRead|EFileWrite);
	test_KErrNone(r);
	r = file64.SetSize(sizeK3GB);
	test_KErrNone(r);
	TFileText fileText;
	fileText.Set(file64);
	
	test.Next(_L("Seek to the file end using TFileText::Seek()\n"));
	r = fileText.Seek(ESeekEnd);
	test_KErrNone(r);
	
	test.Next(_L("Get current file position using RFile64::Seek() and verify it is at file end.\n"));
	TInt64 pos = 0;
	r = file64.Seek(ESeekCurrent, pos);
	test_KErrNone(r);
	test(pos == sizeK3GB);
	
	test.Next(_L("Write data to the file using RFile64::Write\n"));
	HBufC* record = HBufC::NewL(10);
	record->Des().SetLength(10);
	record->Des().Fill('A');
	TPtrC8 bufPtr;
	bufPtr.Set((TUint8*)record->Ptr(),record->Size()); // Size() returns length in bytes
	r = file64.Write(pos,bufPtr);
	test_KErrNone(r);
	
	test.Next(_L("Read data using TFileText::Read\n"));
	TBuf<20> fileTextReadBuf;
	file64.Seek(ESeekStart,pos);//seek to the position where the data has been written
	r = fileText.Read(fileTextReadBuf);
	test(fileTextReadBuf == _L("AAAAAAAAAA"));
	
	test.Next(_L("Seek to the file end using TFileText::Seek(ESeekEnd)\n"));
	r = fileText.Seek(ESeekEnd);
	test_KErrNone(r);
	
	test.Next(_L("Write known data using TFileText::Write\n"));
	TBuf<20> fileTextWriteBuf(_L("AAAAAAAAAA"));
	pos = 0;
	r = file64.Seek(ESeekCurrent,pos);
	r = fileText.Write(fileTextWriteBuf);
	test_KErrNone(r);
	
	test.Next(_L("Read the data using RFile64::Read\n"));
	TBuf8<20> file64ReadBuf;
	file64ReadBuf.Zero();
	r = file64.Read(pos,file64ReadBuf);
	r = bufPtr.Compare(file64ReadBuf);
	test_KErrNone(r);
	
	file64.Close();
	
	r = TheFs.Delete(fileName);
	test_KErrNone(r);
	User::Free(record);
	}


/**
@SYMTestCaseID      PBASE-T_FILE64BIT-0789
@SYMTestPriority    High
@SYMTestRequirement REQ9526
@SYMTestType        CIT
@SYMTestCaseDesc    Test the file read and write with locking a specified region of the file.
@SYMTestActions     
1) Set the File Size to 2GB-1
2) Lock a section of large file, position =0, aLength = 2GB-1
3) Read from position = 2GB-100 and length = 99
4) Write to the File, position = 2GB-100 and length = 99
5) Use  RFs::ReadFileSection () and with position = 2GB -100 and length = 99
6) Set the file size to 4GB-1
7) Lock a section of large file, position =2GB, aLength = 4GB-1
8) Write to the File, position = 4GB-100 and length = 99
@SYMTestExpectedResults 
1) KErrNone
2) KErrNone, file lock successful
3) KErrNone, read is successful
4) KErrLocked, write is unsuccessful
5) KErrNone, read is successful
6) KErrNone
7) KErrNone, lock is successful
8) KErrLocked, write is unsuccessful
@SYMTestStatus      Implemented
*/
void TestReadWriteLock()
	{
	TBuf8<0x63> readBuf;
	TBuf8<0x63> buf;
	TFileName fileName;
	fileName.Append(gDriveToTest);
	fileName.Append(KTestPath);
	fileName.Append(_L("File4GBMinusOne.txt"));
	
	test.Start(_L("Test Lock Functionality\n"));
	TestRFile1.Replace(fileName, EFileWrite|EFileShareAny);
	TestRFile2.Open(fileName);

	test.Next(_L("Creating test pattern"));
	pattern.SetLength(pattern.MaxLength());
	for (TInt i = 0;i<pattern.MaxLength();i++)
		pattern[i] = (TText8)(i + 10);
		
	TInt64 size = 0;
	test.Next(_L("Multi file tests"));
	
	test.Next(_L("Set the File Size to 2GB-1\n"));
	TestRFile1.SetSize(K2GBMinusOne);
	TestRFile1.Size(size);
	test(size == K2GBMinusOne);
	
	test.Next(_L("Lock a section of large file, position =0, aLength = 2GB-1\n"));
	TestRFile1.Lock(0,K2GBMinusOne);
	TestRFile1.LockE(0,K2GBMinusOne);
	
	test.Next(_L("Read from position = 2GB-100 and length = 99\n"));
	TestRFile1.Read(K2GBMinus100,buf,99);
	
	test.Next(_L("Write to the File, position = 2GB-100 and length = 99\n"));
	TestRFile2.WriteE(K2GBMinus100,pattern,99);
	TestRFile1.UnLock(0,K2GBMinusOne);
	TestRFile2.Write(K2GBMinus100,pattern,99);

	test.Next(_L("Use  RFs::ReadFileSection () and with position = 2GB -100 and length = 99\n"));
	TestRFs.ReadFileSection(fileName,K2GBMinus100,readBuf,99);
	
	test.Next(_L("Set the file size to 4GB-1\n"));
	TestRFile1.SetSize(K4GBMinusOne);
	TestRFile1.Size(size);
	test(size == K4GBMinusOne);

    if(KFileSizeMaxLargerThan4GBMinusOne)
        {	    

	    test.Next(_L("Lock a section of large file, position =2GB, aLength = 4GB-1\n"));
	    TestRFile1.Lock(K2GB,K4GBMinusOne);
	    TestRFile1.LockE(K2GB,K4GBMinusOne);
	    
	    test.Next(_L("Write to the File, position = 4GB-100 and length = 99\n"));
	    TestRFile2.WriteE(K4GBMinus100,pattern,99);
	    TestRFile1.UnLock(K2GB,K4GBMinusOne);
	    }
    
    TestRFile2.Close();
	TestRFile1.Close();
	
	TInt r = TheFs.Delete(fileName);
	test_KErrNone(r);
	test.End();
	}

/**
@SYMTestCaseID      PBASE-T_FILE64BIT-0790
@SYMTestPriority    High
@SYMTestRequirement REQ9526
@SYMTestType        CIT
@SYMTestCaseDesc    Test the files unlock functionality and performs file read and write.
@SYMTestActions     
1) Set the File Size to 2GB-1
2) Lock a section of file, position =0, aLength = 2GB-1
3) UnLock a section of large file, position = 0, aLength = 2GB-1
4) Read a file with position = 2GB-100 and length = 99
5) Write to a file with position = 2GB-100 and length = 99
6) Use RFs::ReadFileSection() to read from a file, position = 0 and length = 100(Open mode = EFileShareAny)
7) Set the File Size to 4GB-1
8) Lock a section of large file, position =2GB, aLength = 4GB-1
9) UnLock a section of large file, position =2GB, aLength = 4GB-1
10)Write to the File, position = 4GB-100 and length = 99
@SYMTestExpectedResults returns 
1) KErrNone
2) KErrNone, file locked successfully
3) KErrNone, File unlocked successfully
4) KErrNone, read is successful
5) KErrNone, write is successful
6) KErrNone, read is successful
7) KErrNone
8) KErrNone, lock is successful
9) KErrNone, unlock is successful
10)KErrNone, write is successful
@SYMTestStatus      Implemented
*/
void TestUnLock()
	{
	TBuf8<0x63> buf;
	TBuf8<0x64> readBuf;
	TInt64 size = 0;
	TFileName fileName;
	fileName.Append(gDriveToTest);
	fileName.Append(KTestPath);
	fileName.Append(_L("File4GBMinusOne.txt"));
	
	test.Start(_L("Test Unlock Functionality\n"));
	TestRFile1.Replace(fileName,EFileWrite|EFileShareAny);
	TestRFile2.Open(fileName);

	test.Next(_L("Creating test pattern"));
	pattern.SetLength(pattern.MaxLength());
	for (TInt i = 0;i < pattern.MaxLength();i++)
		pattern[i] = (TText8)(i+10);

	test.Next(_L("Set the File Size to 2GB-1\n"));
	TestRFile1.SetSize(K2GBMinusOne);
	TestRFile1.Size(size);
	test(size == K2GBMinusOne);
	
	test.Next(_L("Lock a section of file, position =0, aLength = 2GB-1\n"));
	TestRFile1.Lock(0,K2GBMinusOne);
	TestRFile1.LockE(0,K2GBMinusOne);
	
	test.Next(_L("UnLock a section of large file, position = 0, aLength = 2GB-1\n"));
	TestRFile1.UnLock(0,K2GBMinusOne);
	
	test.Next(_L("Read a file with position = 2GB-100 and length = 99\n"));
	TestRFile1.Read(K2GBMinus100,buf,99);
	
	test.Next(_L("Write to a file with position = 2GB-100 and length = 99\n"));
	TestRFile2.Write(K2GBMinus100,pattern,99);
	TestRFile1.Read(K2GBMinus100,buf,99);
	test(pattern == buf); // Written data == Read data

	test.Next(_L("Use RFs::ReadFileSection() to read from a file, position = 0 and length = 100(Open mode = EFileShareAny)\n"));
	TFileName fileName1;
	fileName1.Append(gDriveToTest);
	fileName1.Append(KTestPath);
	fileName1.Append(_L("File2GB.txt"));
	RFile64 file;
	TInt r = file.Replace(TheFs, fileName1, EFileWrite);
	test_KErrNone(r);
	file.SetSize(K2GB);
	test_KErrNone(r);
	file.Close();
	TestRFs.ReadFileSection(fileName1,0,readBuf,100);
	r = TheFs.Delete(fileName1);
	test_KErrNone(r);
	test.Next(_L("Creating test pattern"));
	
	TBuf8<0x63> writeBuf63;
	TBuf8<0x63> readBuf63;
	for (TInt count = 0; count < 0x63; count++)
		{
		writeBuf63.Append((TChar)count);
		}
	
	test.Next(_L("Set the File Size to 4GB-1\n"));
	TestRFile1.SetSize(K4GBMinusOne);
	TestRFile1.Size(size);
	test(size == K4GBMinusOne);
	 
    if(KFileSizeMaxLargerThan4GBMinusOne)
        {	    
	    test.Next(_L("Lock a section of large file, position =2GB, aLength = 4GB-1\n"));
	    TestRFile1.Lock(K2GB,K4GBMinusOne);
	
	    test.Next(_L("UnLock a section of large file, position =2GB, aLength = 4GB-1\n"));
	    TestRFile1.UnLock(K2GB,K4GBMinusOne);
	
	    test.Next(_L("Write to the File, position = 4GB-100 and length = 99\n"));
	    TestRFile2.Write(K4GBMinus100,writeBuf63,99);
	    TestRFile2.Read(K4GBMinus100,readBuf63,99);
	    test(writeBuf63 == readBuf63); // Written data == Read data
        }
	
	TestRFile2.Close();
	TestRFile1.Close();
	
	r = TheFs.Delete(fileName);
	test_KErrNone(r);
	test.End();
	}

/**
@SYMTestCaseID      PBASE-T_FILE64BIT-2349
@SYMTestPriority    High
@SYMTestRequirement REQ9529
@SYMTestType        CIT
@SYMTestCaseDesc    Test RFile64::Seek(), Read() & Write() functionality for synchronous calls

					IMPORT_C TInt Read(TDes8 &aDes) const;
					IMPORT_C TInt Write(const TDesC8 &aDes);
@SYMTestActions     
1) Open the large file, Set the File Size to 2GB-1
2) File Seek with: Mode= ESeekCurrent, position = 2GB-100
3) Write to a file with current position and length = 99
4) Seek the file: Mode= ESeekStart, position = 2GB-100
5) Read a file with current position and length =99
6) Compare the read data with written data
7) Seek the file: Mode = ESeekEnd
8) Write to a file with current position and length =99
9) Set the file size to 4GB-1
10)Check for FAT32 File system. Seek the file: Mode = ESeekEnd
11)Write to a file with current position and length =99
12)If NGFS is supported,  Set the file size to 4GB+10
13)Seek the file: Mode = ESeekEnd
14)Write to a file with current position and length =99
@SYMTestExpectedResults 
1) KErrNone, open is successful
2) KErrNone, Seek Position = 2GB-100
3) KErrNone, write is successful
4) KErrNone, Seek Position = 2GB-100
5) KErrNone, read is successful
6) Read data == written data
7) KErrNone
8) KErrNone, write is successful
9) KErrNone
10)KErrNone
11)
12)KErrNone
13)KErrNone
14)KErrNone
@SYMTestStatus      Implemented
*/
void TestSeekReadWrite()
	{
	TBuf8<0x63> readBuf;
	TBuf8<0x63> writeBuf;
	TInt count;
	TInt64 seekPos = 0;
	TFileName fileName;
	fileName.Append(gDriveToTest);
	fileName.Append(KTestPath);
	fileName.Append(_L("File4GBMinusOne.txt"));
	
	test.Start(_L("Test RFile64::Seek(), RFile64::Read() & RFile64::Write() for synchronous calls\n"));
	
	test.Next(_L("Open the large file, Set the File Size to 2GB-1\n"));
	TestRFile1.Replace(fileName,EFileWrite|EFileShareAny);
	
	for (count = 0; count < 0x63; count++)
		{
		writeBuf.Append(count);
		}

	test.Next(_L("Single file tests"));
	TInt64 size2GBMinusOne = 1;
	size2GBMinusOne <<= 31;
	size2GBMinusOne -= 1;
	TestRFile1.SetSize(size2GBMinusOne);

	test.Next(_L("File Seek with: Mode= ESeekCurrent, position = 2GB-100\n"));
	seekPos = K2GBMinus100;
	TestRFile1.Seek(ESeekCurrent,seekPos);
	seekPos = 0;
	TestRFile1.Seek(ESeekCurrent,seekPos);
	test(seekPos == K2GBMinus100);
	
	test.Next(_L("Write to a file with current position and length = 99\n"));
	TestRFile1.Write(writeBuf); // Write 99 bytes of data to a file
	
	test.Next(_L("Seek the file: Mode= ESeekStart, position = 2GB-100\n"));
	seekPos = 0;
	TestRFile1.Seek(ESeekCurrent,seekPos);
	test(seekPos == K2GBMinusOne);
	
	test.Next(_L("Read a file with current position and length =99\n"));
	seekPos = K2GBMinus100;
	TestRFile1.Seek(ESeekStart,seekPos);
	TestRFile1.Read(readBuf);
	
	test.Next(_L("Compare the read data with written data\n"));
	test(writeBuf == readBuf); // Written data == Read data
	
	TBuf8<0x63> writeBuf99;
	TBuf8<0x63> readBuf99;
	TInt64 size = 0;
	
	test.Next(_L("Seek the file: Mode = ESeekEnd\n"));
	TestRFile1.Seek(ESeekEnd,seekPos); // Seek to end
	test(seekPos == K2GBMinusOne);
	
	test.Next(_L("Write to a file with current position and length =99\n"));
	for (count = 0; count < 0x63; count++)
		{
		writeBuf99.Append(count+1);
		}
	TestRFile1.Write(writeBuf99);
	seekPos = K2GBMinusOne;
	TestRFile1.Seek(ESeekStart,seekPos);
	TestRFile1.Read(readBuf99);
	test(writeBuf99 == readBuf99); // Written data == Read data
	// Query Size
	TestRFile1.Size(size);
	test(size == K2GBPlus98); // Validate the file size

	TBuf8<0x63> readBufx63;
	TBuf8<0x63> writeBufx63;
	size = 0;
	if(KFileSizeMaxLargerThan4GBMinusOne == EFalse) //File Systems supporting size of upto 4GB-1
		{
		TestRFile1.SetSize(K4GBMinusOne);
		TestRFile1.Seek(ESeekEnd,seekPos); // Seek to end
		test(seekPos == K4GBMinusOne);
		for (TInt count = 0; count < 0x63; count++)
			{
			writeBufx63.Append(count+2);
			}
		TestRFile1.Write(writeBufx63);
		TestRFile1.Size(size);
		test(size == K4GBMinusOne);
		}
	else
		{ //-- the currenc file system supports files larger than 4G-1
		TestRFile1.SetSize(K4GB+10);
		TestRFile1.Seek(ESeekEnd,seekPos); // Seek to end
		test(seekPos == K4GB+10);
		for (TInt count = 0; count < 0x63; count++)
			{
			writeBufx63.Append(count+2);
			}
		TestRFile1.Write(writeBufx63);
		TestRFile1.Size(size);
		test(size == K4GB+109);	
		TestRFile1.Read(readBufx63); // validating the content
		}

	TestRFile1.Close();
	
	TInt r = TheFs.Delete(fileName);
	test_KErrNone(r);
	test.End();	
	}	

/**
@SYMTestCaseID      PBASE-T_FILE64BIT-2350
@SYMTestPriority    High
@SYMTestRequirement REQ9529
@SYMTestType        CIT
@SYMTestCaseDesc    Test RFile64::Seek(), Read() & Write() functionality for asynchronous calls

					IMPORT_C void Read(TDes8 &aDes, TRequestStatus &aStatus) const;
					IMPORT_C void Write(const TDesC8 &aDes, TRequestStatus &aStatus);
@SYMTestActions     
1) Open the large file, Set the File Size to 2GB-1
2) File Seek with: Mode= ESeekCurrent, position = 2GB-100
3) Write to a file with current position and length = 99
4) Seek the file: Mode= ESeekStart, position = 2GB-100
5) Read a file with current position and length =99
6) Compare the read data with written data
7) Seek the file: Mode = ESeekEnd
8) Write to a file with current position and length =99
9) Set the file size to 4GB-1
10)Check for FAT32 File system. Seek the file: Mode = ESeekEnd
11)Write to a file with current position and length =99
12)If NGFS is supported,  Set the file size to 4GB+10
13)Seek the file: Mode = ESeekEnd
14)Write to a file with current position and length =99
@SYMTestExpectedResults 
1) KErrNone, open is successful
2) KErrNone, Seek Position = 2GB-100
3) KErrNone, write is successful
4) KErrNone, Seek Position = 2GB-100
5) KErrNone, read is successful
6) Read data == written data
7) KErrNone
8) KErrNone, write is successful
9) KErrNone
10)KErrNone
11)
12)KErrNone
13)KErrNone
14)KErrNone
@SYMTestStatus      Implemented
*/
void TestSeekAsyncReadWrite()
	{
	TBuf8<0x63> readBuf;
	TBuf8<0x63> writeBuf;
	TInt count;
	TFileName fileName;
	fileName.Append(gDriveToTest);
	fileName.Append(KTestPath);
	fileName.Append(_L("File4GBMinusOne.txt"));
	
	test.Start(_L("Test RFile64::Seek(), RFile64::Read() & RFile64::Write() for Asynchronous calls\n"));
	
	TestRFile1.Replace(fileName,EFileWrite|EFileShareAny);
	
	for (count = 0; count < 0x63; count++)
		{
		writeBuf.Append(count);
		}

	test.Next(_L("Single file tests"));
	TestRFile1.SetSize(K2GBMinusOne);
	TInt64 seekPos = K2GBMinus100;
	TestRFile1.Seek(ESeekCurrent,seekPos);
	seekPos = 0;
	TestRFile1.Seek(ESeekCurrent,seekPos);
	test(seekPos == K2GBMinus100);
	
	TRequestStatus status1 = KRequestPending;
	TestRFile1.Write(writeBuf,status1); // Write 99 bytes of data to a file
	seekPos = 0;
	TestRFile1.Seek(ESeekCurrent,seekPos);
	test(seekPos == K2GBMinusOne);
	seekPos = K2GBMinus100;
	TestRFile1.Seek(ESeekStart,seekPos);
	TRequestStatus status2 = KRequestPending;
	TestRFile1.Read(readBuf, status2);
	test(writeBuf == readBuf); // Written data == Read data
	
	TBuf8<0x63> writeBuf99;
	TBuf8<0x63> readBuf99;
	TInt64 size = 0;
	TestRFile1.Seek(ESeekEnd,seekPos); // Seek to end
	test(seekPos == K2GBMinusOne);
	for (count = 0; count < 0x63; count++)
		{
		writeBuf99.Append(count+1);
		}
	TRequestStatus status3 = KRequestPending;
	TestRFile1.Write(writeBuf99, status3);
	seekPos = K2GBMinusOne;
	TestRFile1.Seek(ESeekStart,seekPos);
	TRequestStatus status4 = KRequestPending;
	TestRFile1.Read(readBuf99, status4);
	test(writeBuf99 == readBuf99); // Written data == Read data
	// Query Size
	TestRFile1.Size(size);
	test(size == K2GBPlus98); // Validate the file size

	TBuf8<0x63> readBufx63;
	TBuf8<0x63> writeBufx63;
	size = 0;
	if(KFileSizeMaxLargerThan4GBMinusOne == EFalse)//File Systems supporting size of upto 4GB-1
		{
		TestRFile1.SetSize(K4GBMinusOne);
		TestRFile1.Seek(ESeekEnd,seekPos); // Seek to end
		test(seekPos == K4GBMinusOne);
		for (TInt count = 0; count < 0x63; count++)
			{
			writeBufx63.Append(count+2);
			}
		TRequestStatus status5 = KRequestPending;
		TestRFile1.Write(writeBufx63, status5);
		TestRFile1.Size(size);
		test(size == K4GBMinusOne);
		}
    else
		{
		TestRFile1.SetSize(K4GB+10);
		TestRFile1.Seek(ESeekEnd,seekPos); // Seek to end
		test(seekPos == K4GB+10);
		for (TInt count = 0; count < 0x63; count++)
			{
			writeBufx63.Append(count+2);
			}
		TRequestStatus status7 = KRequestPending;			
		TestRFile1.Write(writeBufx63, status7);
		TestRFile1.Size(size);
		test(size == K4GB+109);
		TRequestStatus status8 = KRequestPending;;	
		TestRFile1.Read(readBufx63, status8); // validating the content
		}
	TestRFile1.Close();
	
	TInt r = TheFs.Delete(fileName);
	test_KErrNone(r);
	test.End();	
	}	

/**
@SYMTestCaseID      PBASE-T_FILE64BIT-2351
@SYMTestPriority    High
@SYMTestRequirement REQ9529
@SYMTestType        CIT
@SYMTestCaseDesc    Test RFile64::Seek(), Read() & Write() functionality for synchronous calls with length

					IMPORT_C TInt Read(TDes8 &aDes, TInt aLength) const;
					IMPORT_C TInt Write(const TDesC8 &aDes, TInt aLength);
@SYMTestActions     
1) Open the large file, Set the File Size to 2GB-1
2) File Seek with: Mode= ESeekCurrent, position = 2GB-100
3) Write to a file with current position and length = 99
4) Seek the file: Mode= ESeekStart, position = 2GB-100
5) Read a file with current position and length =99
6) Compare the read data with written data
7) Seek the file: Mode = ESeekEnd
8) Write to a file with current position and length =99
9) Set the file size to 4GB-1
10)Check for FAT32 File system. Seek the file: Mode = ESeekEnd
11)Write to a file with current position and length =99
12)If NGFS is supported,  Set the file size to 4GB+10
13)Seek the file: Mode = ESeekEnd
14)Write to a file with current position and length =99
@SYMTestExpectedResults 
1) KErrNone, open is successful
2) KErrNone, Seek Position = 2GB-100
3) KErrNone, write is successful
4) KErrNone, Seek Position = 2GB-100
5) KErrNone, read is successful
6) Read data == written data
7) KErrNone
8) KErrNone, write is successful
9) KErrNone
10)KErrNone
11)
12)KErrNone
13)KErrNone
14)KErrNone
@SYMTestStatus      Implemented
*/
void TestSeekReadWriteLen()
	{
	TBuf8<0x63> readBuf;
	TBuf8<0x63> writeBuf;
	TInt count;
	TFileName fileName;
	fileName.Append(gDriveToTest);
	fileName.Append(KTestPath);
	fileName.Append(_L("File4GBMinusOne.txt"));
	
	test.Start(_L("Test RFile64::Seek(), RFile64::Read() & RFile64::Write() with Length\n"));
	
	TestRFile1.Replace(fileName,EFileWrite|EFileShareAny);
	
	for (count = 0; count < 0x63; count++)
		{
		writeBuf.Append(count);
		}

	test.Next(_L("Single file tests"));
	
	test.Next(_L("Open the large file, Set the File Size to 2GB-1\n"));
	TestRFile1.SetSize(K2GBMinusOne);
	
	test.Next(_L("File Seek with: Mode= ESeekCurrent, position = 2GB-100\n"));
	TInt64 seekPos = K2GBMinus100;
	TestRFile1.Seek(ESeekCurrent,seekPos);
	seekPos = 0;
	TestRFile1.Seek(ESeekCurrent,seekPos);
	test(seekPos == K2GBMinus100);
	
	test.Next(_L("Write to a file with current position and length = 99\n"));
	TestRFile1.Write(writeBuf, 99); // Write 99 bytes of data to a file
	seekPos = 0;
	TestRFile1.Seek(ESeekCurrent,seekPos);
	test(seekPos == K2GBMinusOne);
	
	test.Next(_L("Seek the file: Mode= ESeekStart, position = 2GB-100\n"));
	seekPos = K2GBMinus100;
	TestRFile1.Seek(ESeekStart,seekPos);
	
	test.Next(_L("Read a file with current position and length =99\n"));
	TestRFile1.Read(readBuf, 99);
	
	test.Next(_L("Compare the read data with written data\n"));
	test(writeBuf == readBuf); // Written data == Read data
	
	test.Next(_L("Seek the file: Mode = ESeekEnd\n"));
	TBuf8<0x63> writeBuf99;
	TBuf8<0x63> readBuf99;
	TInt64 size = 0;
	TestRFile1.Seek(ESeekEnd,seekPos); // Seek to end
	test(seekPos == K2GBMinusOne);
	
	test.Next(_L("Write to a file with current position and length =99\n"));
	for (count = 0; count < 0x63; count++)
		{
		writeBuf99.Append(count+1);
		}
	TestRFile1.Write(writeBuf99, 99);
	seekPos = K2GBMinusOne;
	TestRFile1.Seek(ESeekStart,seekPos);
	TestRFile1.Read(readBuf99, 99);
	test(writeBuf99 == readBuf99); // Written data == Read data
	// Query Size
	TestRFile1.Size(size);
	test(size == K2GBPlus98); // Validate the file size

	TBuf8<0x63> readBufx63;
	TBuf8<0x63> writeBufx63;
	size = 0;
	if(KFileSizeMaxLargerThan4GBMinusOne == EFalse)//File Systems supporting size of upto 4GB-1
		{
		TestRFile1.SetSize(K4GBMinusOne);
		TestRFile1.Seek(ESeekEnd,seekPos); // Seek to end
		test(seekPos == K4GBMinusOne);
		for (TInt count = 0; count < 0x63; count++)
			{
			writeBufx63.Append(count+2);
			}
		TestRFile1.Write(writeBufx63, 99);
		TestRFile1.Size(size);
		test(size == K4GBMinusOne);
		}
    else
		{
		TestRFile1.SetSize(K4GB+10);
		TestRFile1.Seek(ESeekEnd,seekPos); // Seek to end
		test(seekPos == K4GB+10);
		for (TInt count = 0; count < 0x63; count++)
			{
			writeBufx63.Append(count+2);
			}
		TestRFile1.Write(writeBufx63, 99);
		TestRFile1.Size(size);
		test(size == K4GB+109);	
		TestRFile1.Read(readBufx63, 99); // validating the content
		}

	TestRFile1.Close();
	
	TInt r = TheFs.Delete(fileName);
	test_KErrNone(r);
	test.End();	
	}	
/**
@SYMTestCaseID      PBASE-T_FILE64BIT-2352
@SYMTestPriority    High
@SYMTestRequirement REQ9529
@SYMTestType        CIT
@SYMTestCaseDesc    Test RFile64::Seek(), Read() & Write() functionality for asynchronous calls with length
					IMPORT_C void Read(TDes8 &aDes, TRequestStatus &aStatus) const;
					IMPORT_C void Write(const TDesC8 &aDes, TRequestStatus &aStatus);
@SYMTestActions     
1) Open the large file, Set the File Size to 2GB-1
2) File Seek with: Mode= ESeekCurrent, position = 2GB-100
3) Write to a file with current position and length = 99
4) Seek the file: Mode= ESeekStart, position = 2GB-100
5) Read a file with current position and length =99
6) Compare the read data with written data
7) Seek the file: Mode = ESeekEnd
8) Write to a file with current position and length =99
9) Set the file size to 4GB-1
10)Check for FAT32 File system. Seek the file: Mode = ESeekEnd
11)Write to a file with current position and length =99
12)If NGFS is supported,  Set the file size to 4GB+10
13)Seek the file: Mode = ESeekEnd
14)Write to a file with current position and length =99
@SYMTestExpectedResults 
1) KErrNone, open is successful
2) KErrNone, Seek Position = 2GB-100
3) KErrNone, write is successful
4) KErrNone, Seek Position = 2GB-100
5) KErrNone, read is successful
6) Read data == written data
7) KErrNone
8) KErrNone, write is successful
9) KErrNone
10)KErrNone
11)
12)KErrNone
13)KErrNone
14)KErrNone
@SYMTestStatus      Implemented
*/	

void TestSeekAsyncReadWriteLen()
	{
	TBuf8<0x63> readBuf;
	TBuf8<0x63> writeBuf;
	TInt count;
	TFileName fileName;
	fileName.Append(gDriveToTest);
	fileName.Append(KTestPath);
	fileName.Append(_L("File4GBMinusOne.txt"));
	
	
	test.Start(_L("Test RFile64::Seek, RFile64::Read & RFile64::Write\n"));
	
	test.Next(_L("Open the large file, Set the File Size to 2GB-1\n"));
	TestRFile1.Replace(fileName,EFileWrite|EFileShareAny);
	
	for (count = 0; count < 0x63; count++)
		{
		writeBuf.Append(count);
		}

	test.Next(_L("Single file tests"));
	TestRFile1.SetSize(K2GBMinusOne);
	
	test.Next(_L("File Seek with: Mode= ESeekCurrent, position = 2GB-100\n"));
	TInt64 seekPos = K2GBMinus100;
	TestRFile1.Seek(ESeekCurrent,seekPos);
	seekPos = 0;
	TestRFile1.Seek(ESeekCurrent,seekPos);
	test(seekPos == K2GBMinus100);

	test.Next(_L("Write to a file with current position and length = 99"));
	TRequestStatus status1 = KRequestPending;
	TestRFile1.Write(writeBuf,99,status1); // Write 99 bytes of data to a file
	
	test.Next(_L("Seek the file: Mode= ESeekStart, position = 2GB-100\n"));
	seekPos = 0;
	TestRFile1.Seek(ESeekCurrent,seekPos);
	test(seekPos == K2GBMinusOne);
	seekPos = K2GBMinus100;
	TestRFile1.Seek(ESeekStart,seekPos);
	
	test.Next(_L("Read a file with current position and length =99\n"));
	TRequestStatus status2 = KRequestPending;
	TestRFile1.Read(readBuf,99,status2);
	
	test.Next(_L("Compare the read data with written data\n"));
	test(writeBuf == readBuf); // Written data == Read data
	
	test.Next(_L("Seek the file: Mode = ESeekEnd\n"));
	TBuf8<0x63> writeBuf99;
	TBuf8<0x63> readBuf99;
	TInt64 size = 0;
	TestRFile1.Seek(ESeekEnd,seekPos); // Seek to end
	test(seekPos == K2GBMinusOne);
	for (count = 0; count < 0x63; count++)
		{
		writeBuf99.Append(count+1);
		}
		
	test.Next(_L("Write to a file with current position and length =99\n"));
	TRequestStatus status3 = KRequestPending;
	TestRFile1.Write(writeBuf99,99,status3);
	seekPos = K2GBMinusOne;
	TestRFile1.Seek(ESeekStart,seekPos);
	TRequestStatus status4 = KRequestPending;
	TestRFile1.Read(readBuf99,99,status4);
	test(writeBuf99 == readBuf99); // Written data == Read data
	// Query Size
	TestRFile1.Size(size);
	test(size == K2GBPlus98); // Validate the file size
	
	TBuf8<0x63> readBufx63;
	TBuf8<0x63> writeBufx63;
	size = 0;
    if(KFileSizeMaxLargerThan4GBMinusOne == EFalse) //File Systems supporting size of upto 4GB-1
		{
		test.Next(_L("Set the file size to 4GB-1\n"));
		TestRFile1.SetSize(K4GBMinusOne);
		TestRFile1.Seek(ESeekEnd,seekPos); // Seek to end
		test(seekPos == K4GBMinusOne);
		for (TInt count = 0; count < 0x63; count++)
			{
			writeBufx63.Append(count+2);
			}
		TRequestStatus status5 = KRequestPending;
		TestRFile1.Write(writeBufx63,99,status5);
		TestRFile1.Size(size);
		test(size == K4GBMinusOne);
		}
    else
		{
		TestRFile1.SetSize(K4GB+10);
		TestRFile1.Seek(ESeekEnd,seekPos); // Seek to end
		test(seekPos == K4GB+10);
		for (TInt count = 0; count < 0x63; count++)
			{
			writeBufx63.Append(count+2);
			}
		TRequestStatus status7 = KRequestPending;			
		TestRFile1.Write(writeBufx63,99,status7);
		TestRFile1.Size(size);
		test(size == K4GB+109);
		TRequestStatus status8 = KRequestPending;;	
		TestRFile1.Read(readBufx63,99,status8); 
		}
	TestRFile1.Close();
	TInt r = TheFs.Delete(fileName);
	test_KErrNone(r);
	test.End();	
	}	
/**
@SYMTestCaseID      PBASE-T_FILE64BIT-2353
@SYMTestPriority    High
@SYMTestRequirement REQ9529
@SYMTestType        CIT
@SYMTestCaseDesc    Tests File resizing functionality
@SYMTestActions	
1) Create a file name "test" in write mode
2) Generate a random file size
3) Set the file size using RFile64::SetSize()
4) Get the file size.
5) Seek to the file position RFile64::Size() -100 & File position >0
6) Write 99 bytes to the current file position
7) Read from the current file position.
8) Compare the file read with written data.
9) Repeat step 2 to 8 for <10> times in a loop.
10) Close the file
@SYMTestExpectedResults
1) File create successfully.
2) Ramdom file size generated.
3) File size set successfully.
4) File size = Random file size set.
5) File seek successful.
6) File write successful with KErrNone.
7) File read successful with KErrNone.
8) Read data == Written data.
9) Expect result same as step 2 to 8.
10) File closed successfully.
@SYMTestStatus      Implemented
*/
void TestFileReSize()
	{
	TInt64 seed = K4GBMinusOne;
	TBuf8<0x63> readBuf;
	TBuf8<0x63> writeBuf;
	TFileName fileName;
	fileName.Append(gDriveToTest);
	fileName.Append(KTestPath);
	fileName.Append(_L("test.txt"));
	for (TInt count = 0; count < 0x63; count++)
		{
		writeBuf.Append(count);
		}
	test.Next(_L("Create a file name test in write mode\n"));
	TestRFile1.Replace(fileName, EFileWrite);
	TInt randSize;
	TInt64 size;
	for (TInt i = 0; i<10; i++)
		{
		test.Next(_L("Generate a random file size\n"));
		randSize = Math::Rand(seed);
		
		test.Next(_L("Set the file size using RFile64::SetSize()\n"));
		TestRFile1.SetSize(randSize);
		
		test.Next(_L("Get the file size.\n"));
		TestRFile1.Size(size);
		test(randSize == size);
		
		test.Next(_L("Seek to the file position RFile64::Size() -100 & File position >0\n"));
		TInt64 seekPos = size - 100;
		if(size>100) //carry out tests for file sizes greater than 100 bytes
			{	
			TestRFile1.Seek(ESeekStart,seekPos);
			TRequestStatus status1 = KRequestPending;
			
			test.Next(_L("Write 99 bytes to the current file position\n"));
			TestRFile1.Write(writeBuf,99,status1); // Write 99 bytes of data to a file
			TestRFile1.Seek(ESeekStart,seekPos);
			
			
			test.Next(_L("Read from the current file position.\n"));
			TRequestStatus status2 = KRequestPending;
			
			test.Next(_L("Compare the file read with written data.\n"));
			TestRFile1.Read(readBuf,99,status2);
			test(writeBuf == readBuf); // Written data == Read data
			}
		}
	TestRFile1.Close();
	TInt r = TheFs.Delete(fileName);
	test_KErrNone(r);
	}
/**
@SYMTestCaseID      PBASE-T_FILE64BIT-2354
@SYMTestPriority    High
@SYMTestRequirement REQ9532
@SYMTestType        CIT
@SYMTestCaseDesc    Tests for copying a directory containing large files using CFileMan::Copy()
@SYMTestActions     
1) Create one large file in the specified test path
2) Set the file size to 3GB-4KB
3) Seek to the end of the file
4) Check the seek position
5) Write to a file with position = 3GB-4KB and length = 4KB
6) Get the file size
7) Create 3 small files in the specified test path
8) Write 10 bytes to the created files
9) Copy the files from one folder to another using CFileMan::Copy()
10)Get the directory entry and find how many files are copied
11)Set file man observer
12)Copy the files from one folder to another, source folder has 3 small files and a large file
13)Check observer for number of successful copy and failed copy
14)Get the directory entry and find how many files copied
@SYMTestExpectedResults 
1) KErrNone, file created successfully
2) KErrNone
3) KErrNone
4) Seek position = 3GB-4KB
5) KErrNone, write is successful
6) File size = 3GB
7) KErrNone, 3 small files created successfully
8) KErrNone, write is successful
9) KErrNone, copy is successful
10)4 files copied successfully
11)KErrNone
12)KErrNone, copy is successful
13)Number of success file copy = 4, Failed = 0
14)4 files copied successfully
@SYMTestStatus      Implemented
*/	

void TestCopyDirectory()
	{
	test.Next(_L("Copy a directory containing large files"));
	CFileMan* fileMan = CFileMan::NewL(TheFs);
	test(fileMan != NULL);
	
	CFileManObserver* observer = new CFileManObserver(fileMan);
	test(observer != NULL);

	TPath filePathOld;
	filePathOld.Append(gDriveToTest);
	filePathOld.Append(KTestPath);
	
		
	TPath filePathNew;
	filePathNew.Append(gDriveToTest);
	filePathNew.Append(_L(":\\TEST\\"));
	
	
	// create some small files in the source directory 
	// so that there is a combination of small files and one large files
	RDir dir;
	TEntryArray entryArray;
	
	TFileName fileLarge1;
	fileLarge1.Append(gDriveToTest);
	fileLarge1.Append(KTestPath);
	fileLarge1.Append(_L("FileLargeOne.txt"));
	
	test.Next(_L("	Create one large file in the specified test path\n"));
	TestRFile1.Replace(fileLarge1, EFileWrite | EFileShareAny);
	
	
	test.Next(_L("Set the file size to 3GB-4KB\n"));
	TestRFile1.SetSize(K3GB-K4KB);
	TInt64 size1 = 0;
	TestRFile1.Size(size1);
	
	test.Next(_L("Seek to the end of the file\n"));
	TInt64 seekPos = 0;
	TestRFile1.Seek(ESeekEnd,seekPos); 
	test(seekPos == K3GB-K4KB);
	
	test.Next(_L("Write to a file with position = 3GB-4KB and length = 4KB\n"));
	TBuf8<4096> writeBufK4KB;
	for (TInt count = 0; count < 4096; count++)
		{
		writeBufK4KB.Append(count+1);
		}
	TestRFile1.Write(seekPos, writeBufK4KB, writeBufK4KB.Length());
	TestRFile1.Size(size1);
	test.Next(_L("Get the file size\n"));
	TInt64 size = 0;
	TestRFile1.Size(size);
	test(size == K3GB);
	TestRFile1.Close();
	
	test.Next(_L("Create 3 small files in the specified test path and Write 10 bytes to the created files\n"));
	TFileName fileSmall1;
	fileSmall1.Append(gDriveToTest);
	fileSmall1.Append(KTestPath);
	fileSmall1.Append(_L("FileSmallOne.txt"));
	
	TFileName fileSmall2;
	fileSmall2.Append(gDriveToTest);
	fileSmall2.Append(KTestPath);
	fileSmall2.Append(_L("FileSmallTwo.txt"));
	
	TFileName fileSmall3;
	fileSmall3.Append(gDriveToTest);
	fileSmall3.Append(KTestPath);
	fileSmall3.Append(_L("FileSmallThree.txt"));
	
	TestRFile1.Create(fileSmall1, EFileWrite | EFileShareAny);
	TestRFile1.Write(_L8("1234567891"));
	TestRFile1.Close();

	TestRFile1.Create(fileSmall2, EFileWrite | EFileShareAny);
	TestRFile1.Write(_L8("1234567891"));
	TestRFile1.Close();

	TestRFile1.Create(fileSmall3, EFileWrite | EFileShareAny);
	TestRFile1.Write(_L8("1234567891"));
	TestRFile1.Close();

	test.Next(_L("Copy the files from one folder to another using CFileMan::Copy()\n"));
	TInt r = fileMan->Copy(filePathOld, filePathNew, CFileMan::ERecurse | CFileMan::EOverWrite);
	test_Value(r, r == KErrNone || r == KErrTooBig);

	test.Next(_L("Get the directory entry and find how many files are copied\n"));
	// check SMALL and LARGE files have been copied
	r = dir.Open(TheFs, filePathNew, KEntryAttNormal);
	test_KErrNone(r);
	r = dir.Read(entryArray);
	test_Value(r, r == KErrEof);
	test_Equal(gFilesInDirectory, entryArray.Count());
	dir.Close();
	
	// then delete the new directory
	r = fileMan->Delete(filePathNew);
	test_KErrNone(r);

	test.Next(_L("Set file man observer\n"));
	// attempt to copy to new directory again - this time with an observer
	fileMan->SetObserver(observer);
	
	test.Next(_L("Copy the files from one folder to another, source folder has 3 small files and a large file\n"));
	r = fileMan->Copy(filePathOld, filePathNew, CFileMan::ERecurse | CFileMan::EOverWrite);
	test_Value(r, r == KErrNone || r == KErrTooBig);
	
	test.Next(_L("Check observer for number of successful copy and failed copy\n"));
	// test that 3 small files and 1 large file were copied
	// (For 8 GB disk, the 4GB file is missing)
	test_Equal(gFilesInDirectory, observer->iNotifyEndedSuccesses);
	test_Equal(0, observer->iNotifyEndedFailures);

	test.Next(_L("Get the directory entry and find how many files copied\n"));
	// check SMALL files have been copied
	r = dir.Open(TheFs, filePathNew, KEntryAttNormal);
	test_KErrNone(r);
	r = dir.Read(entryArray);
	test_Value(r, r == KErrEof);

	test_Equal(gFilesInDirectory, entryArray.Count());
	dir.Close();
	
	// then delete the new directory
	r = fileMan->Delete(filePathNew);
	test_KErrNone(r);

	delete observer;
	delete fileMan;
	
	r = TheFs.Delete(fileSmall1);
	test_KErrNone(r);
	r = TheFs.Delete(fileSmall2);
	test_KErrNone(r);
	r = TheFs.Delete(fileSmall3);
	test_KErrNone(r);
	r = TheFs.Delete(fileLarge1);
	test_KErrNone(r);
	}
/**
@SYMTestCaseID      PBASE-T_FILE64BIT-2355
@SYMTestPriority    High
@SYMTestRequirement REQ9532
@SYMTestType        CIT
@SYMTestCaseDesc    Tests for moving a directory containing large files using CFileMan::Move()
@SYMTestActions     	
1) Create 3 small files and 1 large file
2) Write 10 bytes to the created files
3) Move the files from one folder to another using CFileMan Move
4) Get the directory entry and find how many files moved in a directory
5) Move the files back to the original folder 
@SYMTestExpectedResults 
1) KErrNone, files created successfully
2) KErrNone, write is successful
3) KErrNone
4) 4 files moved successfully
5) KErrNone
@SYMTestStatus      Implemented
*/
void TestMoveDirectory()
	{
	test.Next(_L("Move a directory containing large files"));
	CFileMan* fileMan = CFileMan::NewL(TheFs);
	test(fileMan != NULL);
	
	TPath filePathOld;
	filePathOld.Append(gDriveToTest);
	filePathOld.Append(KTestPath);
	
		
	TPath filePathNew;
	filePathNew.Append(gDriveToTest);
	filePathNew.Append(_L(":\\TEST\\"));
	
	test.Next(_L("Create 3 small files and 1 large file\n"));
	
	TFileName fileLarge1;
	fileLarge1.Append(gDriveToTest);
	fileLarge1.Append(KTestPath);
	fileLarge1.Append(_L("FileLargeOne.txt"));
	TestRFile1.Replace(fileLarge1, EFileWrite | EFileShareAny);
	TestRFile1.SetSize(K4GB-1);
	TestRFile1.Close();
	
	
	TFileName fileSmall1;
	fileSmall1.Append(gDriveToTest);
	fileSmall1.Append(KTestPath);
	fileSmall1.Append(_L("FileSmallOne.txt"));
	
	TFileName fileSmall2;
	fileSmall2.Append(gDriveToTest);
	fileSmall2.Append(KTestPath);
	fileSmall2.Append(_L("FileSmallTwo.txt"));
	
	TFileName fileSmall3;
	fileSmall3.Append(gDriveToTest);
	fileSmall3.Append(KTestPath);
	fileSmall3.Append(_L("FileSmallThree.txt"));
	
	TestRFile1.Create(fileSmall1, EFileWrite | EFileShareAny);
	TestRFile1.Write(_L8("1234567891"));
	TestRFile1.Close();

	TestRFile1.Create(fileSmall2, EFileWrite | EFileShareAny);
	TestRFile1.Write(_L8("1234567891"));
	TestRFile1.Close();

	TestRFile1.Create(fileSmall3, EFileWrite | EFileShareAny);
	TestRFile1.Write(_L8("1234567891"));
	TestRFile1.Close();
	

	// move to new directory
	TInt r = fileMan->Move(filePathOld, filePathNew, CFileMan::ERecurse | CFileMan::EOverWrite);
	test_Value(r, r == KErrNone || r == KErrTooBig);

	// check SMALL and LARGE files have been moved
	RDir dir;
	r = dir.Open(TheFs, filePathNew, KEntryAttNormal);
	test_KErrNone(r);
	TEntryArray entryArray;
	r = dir.Read(entryArray);
	test_Value(r, r == KErrEof);
	test_Equal(4, entryArray.Count());
	dir.Close();
	
	// then delete the new directory
	r = fileMan->Delete(filePathNew);
	test_KErrNone(r);
	delete fileMan;
	}


static void TestOpenFiles()
	{
    TestOpen2GB();
	TestOpen3GB();
	TestOpen4GBMinusOne();
	TestOpen4GB();
	TestOpenMoreThan2GB();
	TestOpenRFileRFile64();
	TestCreateTempFile();
	TestCreateRFile64();
    TestReplaceRFile64();
	TestReplaceRFile64RFs();
	}
	
static void TestAdoptFiles()
	{
	TInt r;
	RFs fs;
	r = fs.Connect();
	test_KErrNone(r);
	r = fs.ShareProtected();
	test_KErrNone(r);
	TFileName sessionp;
	fs.SessionPath(sessionp);
	r = fs.MkDirAll(sessionp);
	test_Value(r, r == KErrNone || r == KErrAlreadyExists);
	fs.Close();
	TestRFile64AdoptFromCreator();
	TestRFile64AdoptFromClient();
	TestRFile64AdoptFromServer();
	}


static void TestReadFile()
	{
//
//The order of these tests need to be preserved, since the first test creates
//a 4GB file, while the last one deletes it. 
// 
	TestOpenAndReadSyncLargeFile();
	TestOpenAndReadAsyncLargeFile();
	TestOpenAndReadSyncLargeFileWithLen(); 
    TestOpenAndReadAsyncLargeFileWithLen();
	}
	
static void TestWriteFile()
	{
	TestOpenAndWriteSyncLargeFile();
	TestOpenAndWriteAsyncLargeFile();
	TestOpenAndWriteSyncLargeFileWithLen();
	TestOpenAndWriteAsyncLargeFileWithLen();
	}
	
static void TestFileAccess()
	{
	TestFileLock();
	TestFileUnlock();
	TestFileSeek();
	TestFileSeekBigFile();
	}

static void TestLockUnLock()
	{
	TestReadWriteLock();
	TestUnLock();
	TestSeekReadWrite();
	TestSeekAsyncReadWrite(); 
  	TestSeekReadWriteLen();
    TestSeekAsyncReadWriteLen();
	}

static void TestCopyMoveDirectory()
	{
	TestCopyDirectory();
	TestMoveDirectory();
	}

TInt testLockPanic(TAny* aPtr)
	{
	TInt aPos=128;
	TInt aLen=-1;
	RFile64 * ptr = (RFile64 *)aPtr;
	TInt r=ptr->Lock(aPos, aLen);
	test_KErrNone(r);
	return KErrNone;
	}

TInt testUnLockPanic(TAny* aPtr)
	{
	TInt aPos=128;
	TInt aLen=-1;
	RFile64 * ptr = (RFile64 *)aPtr;
	TInt r=ptr->UnLock(aPos, aLen);
	test_KErrNone(r);
	return KErrNone;
	}

TInt testSetSizePanic(TAny* aPtr)
	{
	TInt aSize=-1;
	RFile64 * ptr = (RFile64 *)aPtr;
	TInt r=ptr->SetSize(aSize);
	test_KErrNone(r);
	return KErrNone;
	}

static void TestRFile64NegLen()
	{
	test.Start(_L("Test RFile64::Lock, RFile64::Unlock, RFile64::SetSize and RFile::Write with Negative Length parameter"));
	
	test_KErrNone(TheFs.ShareProtected());
	
	RFile64 aFile;
	TInt r = aFile.Create(TheFs, _L("\\testRFile64NegLen.txt"), EFileWrite);
	test_Value(r, r == KErrNone || r == KErrAlreadyExists);
	
	TRequestStatus status = KRequestPending;

	// launch call on separate thread as expected to panic
	// Test Lock with a negative length
	User::SetJustInTime(EFalse);
	RThread t;
	test(t.Create(_L("testLockPanic"), testLockPanic, KDefaultStackSize, 0x2000, 0x2000, &aFile) == KErrNone);
	t.Logon(status);
	t.Resume();
	User::WaitForRequest(status);
	User::SetJustInTime(ETrue);
	test(t.ExitType() == EExitPanic);
	test_Equal(17, t.ExitReason());
	t.Close();
	
	// Test Unlock with a negative length
	User::SetJustInTime(EFalse);
	status = KRequestPending;
	test(t.Create(_L("testUnLockPanic"), testUnLockPanic, KDefaultStackSize, 0x2000, 0x2000, &aFile) == KErrNone);
	t.Logon(status);
	t.Resume();
	User::WaitForRequest(status);
	test(t.ExitType() == EExitPanic);
	test_Equal(18, t.ExitReason());
	t.Close();
	User::SetJustInTime(ETrue);
	
	// Test SetSize with a negative length
	User::SetJustInTime(EFalse);
	status = KRequestPending;
	test(t.Create(_L("testSetSizePanic"), testSetSizePanic, KDefaultStackSize, 0x2000, 0x2000, &aFile) == KErrNone);
	t.Logon(status);
	t.Resume();
	User::WaitForRequest(status);
	test(t.ExitType() == EExitPanic);
	test_Equal(20, t.ExitReason());
	t.Close();
	User::SetJustInTime(ETrue);
	
	// Test RFile64::Write with a zero or negative length
	TInt aPos=128;
	TInt aLen=-1;
	TBuf8<0x100> gBuf=_L8("1234567891");
	gBuf.Zero();
	TRequestStatus status1=KRequestPending;
	TRequestStatus status2=KRequestPending;
	
	// If a zero length is passed into the Write function, KErrNone should be returned. 
	r=aFile.Write(aPos,gBuf);
	test_KErrNone(r);
	
	// If the length is a negative, KErrArgument should be returned. 
	r=aFile.Write(aPos,gBuf,aLen);
	test_Value(r, r == KErrArgument);
	
	// Test the asynchronous requests
	aFile.Write(aPos,gBuf,aLen,status1);
	aFile.Write(aPos,gBuf,aLen,status2);
	User::WaitForRequest(status1);
	test_Equal(KErrArgument, status1.Int());
	User::WaitForRequest(status2);
	test_Equal(KErrArgument, status2.Int());
	
	aFile.Close();
	r = TheFs.Delete(_L("\\testRFile64NegLen.txt"));
	test_KErrNone(r);
	test.End();	
	}

/*
 * Flush file to ensure that written data is committed and not cached
 */
TInt FlushFile(RFile64& aFile64)
	{
	TRequestStatus status;
	aFile64.Flush(status); 
	User::WaitForRequest(status);
	return status.Int();
	}

/*
 * Test inline RFile64 Read/Write APIs
 */
void TestInlineReadWrite()
	{
	test.Start(_L("Test Inline RFile64 Read/Write APIs"));
	TFileName fileName;
	fileName.Append(gDriveToTest);
	fileName.Append(KTestPath);
	TInt r = TheFs.MkDir(fileName);
	test_Value(r, r == KErrNone || r == KErrAlreadyExists);
	
	fileName.Append(_L("FileInline.txt"));
	RFile64 file64;
	r = file64.Replace(TheFs, fileName, EFileWrite);
	test_KErrNone(r);
	
	TBuf8<15> writeBuf(_L8("Inline"));
	TBuf8<15> readBuf;
	readBuf.Zero();
	
	test.Next(_L("RFile64::Write(const TDesC8& aDes)"));
	// Expected text in file: Inline
	r = file64.Write(writeBuf);
	test_KErrNone(r);
	// Flush to ensure that the written data is committed and not cached
	r = FlushFile(file64);
	test_KErrNone(r);
	
	test.Next(_L("RFile64::Read(TInt aPos,TDes8& aDes)"));
	TInt readPos = 0;
	r = file64.Read(readPos, readBuf);
	test_KErrNone(r);
	test(readBuf == writeBuf);
	
	
	test.Next(_L("RFile64::Write(const TDesC8& aDes,TRequestStatus& aStatus)"));
	// Expected text in file: InlineInline
	TRequestStatus status = KRequestPending;	
	file64.Write(writeBuf, status);
	User::WaitForRequest(status);
	test_KErrNone(status.Int());
	status = KRequestPending;
	r = FlushFile(file64);
	test_KErrNone(r);
	
	test.Next(_L("Read(TInt aPos,TDes8& aDes,TRequestStatus& aStatus)"));
	file64.Read(readPos, readBuf, status);
	User::WaitForRequest(status);
	test_KErrNone(status.Int());
	status = KRequestPending;
	TBuf8<15> expectedBuf(_L8("InlineInline"));
	test(readBuf == expectedBuf);
	
	test.Next(_L("Write(const TDesC8& aDes,TInt aLength)"));
	// Expected text in file: InlineInlineInl
	TInt writeLength = 3;
	r = file64.Write(writeBuf, writeLength);
	test_KErrNone(r);
	r = FlushFile(file64);
	test_KErrNone(r);
	
	test.Next(_L("Read(TInt aPos,TDes8& aDes,TInt aLength)"));
	TInt readLength = 13;
	r = file64.Read(readPos, readBuf, readLength);
	test_KErrNone(r);
	expectedBuf.Append(_L8("I"));
	test(readBuf == expectedBuf);
	
	
	test.Next(_L("Write(const TDesC8& aDes,TInt aLength,TRequestStatus& aStatus)"));
	// Expected text in file: InlineInlineIInl
	file64.Write(writeBuf, writeLength, status);
	User::WaitForRequest(status);
	test_KErrNone(status.Int());
	status = KRequestPending;
	r = FlushFile(file64);
	test_KErrNone(r);
	
	test.Next(_L("Read(TInt aPos,TDes8& aDes,TInt aLength,TRequestStatus& aStatus)"));
	readLength = 14;
	file64.Read(readPos, readBuf, readLength, status);
	User::WaitForRequest(status);
	test_KErrNone(status.Int());
	status = KRequestPending;
	expectedBuf.Append(_L8("I"));
	test(readBuf == expectedBuf);
	
	test.Next(_L("Write(TInt aPos,const TDesC8& aDes)"));
	// Expected text in file: InlInlineineIInl
	TInt writePos = 3;
	r = file64.Write(writePos, writeBuf);
	test_KErrNone(r);
	r = FlushFile(file64);
	test_KErrNone(r);
	
	// RFile64::Read(TInt aPos,TDes8& aDes) (Again)
	readPos = 1;
	r = file64.Read(readPos, readBuf);
	test_KErrNone(r);
	expectedBuf.Zero();
	expectedBuf.Append(_L8("nlInlineineIInl"));
	test(readBuf == expectedBuf);
	
	test.Next(_L("Write(TInt aPos,const TDesC8& aDes,TRequestStatus& aStatus)"));
	// Expected text in file: InlInlineInlinel
	writePos = 9;
	file64.Write(writePos, writeBuf, status);
	User::WaitForRequest(status);
	test_KErrNone(status.Int());
	status = KRequestPending;
	r = FlushFile(file64);
	test_KErrNone(r);
	
	// RFile64::Read(TInt aPos,TDes8& aDes,TRequestStatus& aStatus) (Again)
	file64.Read(readPos, readBuf, status);
	User::WaitForRequest(status);
	test_KErrNone(status.Int());
	status = KRequestPending;
	expectedBuf.Zero();
	expectedBuf.Append(_L8("nlInlineInlinel"));
	test(readBuf == expectedBuf);
	
	test.Next(_L("Write(TInt aPos,const TDesC8& aDes,TInt aLength)"));
	// Expected text in file: InlInlInlInlinel
	writePos = 6;
	r = file64.Write(writePos, writeBuf, writeLength);
	test_KErrNone(r);
	r = FlushFile(file64);
	test_KErrNone(r);
	
	// Read(TInt aPos,TDes8& aDes,TInt aLength) (Again)
	readPos = 5;
	readLength = 8;
	r = file64.Read(readPos, readBuf, readLength);
	test_KErrNone(r);
	expectedBuf.Zero();
	expectedBuf.Append(_L8("lInlInli"));
	test(readBuf == expectedBuf);
	
	
	test.Next(_L("Write(TInt aPos,const TDesC8& aDes,TInt aLength,TRequestStatus& aStatus)"));
	// Expected text in file: InlInlInlInlinel
	writePos = 12;
	file64.Write(writePos, writeBuf, writeLength, status);
	User::WaitForRequest(status);
	test_KErrNone(status.Int());
	status = KRequestPending;
	r = FlushFile(file64);
	test_KErrNone(r);

	// RFile64::Read(TInt aPos,TDes8& aDes,TInt aLength,TRequestStatus& aStatus) (Again)
	readPos = 7;
	file64.Read(readPos, readBuf, readLength, status);
	User::WaitForRequest(status);
	test_KErrNone(status.Int());
	status = KRequestPending;
	expectedBuf.Zero();
	expectedBuf.Append(_L8("nlInlInl"));
	test(readBuf == expectedBuf);
	
	
	// Test the remaining RFile64::Read APIs with the read position at the end of the file
	
	TBuf8<1> readBuf2;
	readBuf2.Zero();
	TBuf8<1> expectedBuf2(_L8("l"));
	
	test.Next(_L("Read(TDes8& aDes)"));
	r = file64.Read(readBuf2);
	test_KErrNone(r);
	test(readBuf2 == expectedBuf2);
	
	test.Next(_L("Read(TDes8& aDes,TRequestStatus& aStatus)"));
	file64.Read(readBuf2, status);
	User::WaitForRequest(status);
	test_KErrNone(status.Int());
	status = KRequestPending;
	expectedBuf2.Zero();
	test(readBuf2 == expectedBuf2);
	
	// These tests should give overflow errors
	test.Next(_L("Read(TDes8& aDes,TInt aLength)"));
	r = file64.Read(readBuf2, readLength);
	test_Equal(KErrOverflow, r);
	
	test.Next(_L("Read(TDes8& aDes,TInt aLength,TRequestStatus& aStatus)"));
	file64.Read(readBuf2, readLength, status);
	User::WaitForRequest(status);
	test_Equal(KErrOverflow, status.Int());

	
	file64.Close();
	r = TheFs.Delete(fileName);
	test_KErrNone(r);
	test.End();
	}

//-------------------------------------------------------------------------------------------------------------------

static TInt PrepareDisk(TInt aDrive)
	{
    TInt r;

    if (!Is_Win32(TheFs, aDrive))
    	{
		r = FormatDrive(TheFs, aDrive, ETrue);
		test_KErrNone(r);
    	}

	r = TheFs.Volume(gDriveVolumeInfo, aDrive);
	test_KErrNone(r);
	test.Printf(_L("Drive volume size is %LU\n"), gDriveVolumeInfo.iSize);

	// don't test if media size is less than 4 GB
	if (gDriveVolumeInfo.iFree <= K4GB)
		{
		test.Printf(_L("Skipping T_FILE64BIT: test requires disk with capacity more than 4 GB\n"));
		return KErrNotSupported;
		}
	
	TFileName fileName;
	fileName.Append(gDriveToTest);
	fileName.Append(KTestPath);
	
	MakeDir(fileName);
	
	
    //-- decide if we need to test files >= 4G. As soon as there is no appropriate API yet, this way is dodgy..
    if(Is_Fat(TheFs, aDrive))
        {
        KFileSizeMaxLargerThan4GBMinusOne = EFalse; //-- FAT doesn't support >= 4G files
        }
    else if(Is_SimulatedSystemDrive(TheFs, aDrive))
        {
		 //-- This is the emulator's windows drive or PlatSim's HVFS.
		 //-- The maximal file size depends on the Windows FS used for this drive.
         //-- If it is NTFS, files >= 4G are supported.
        r = CreateEmptyFile(TheFs, _L("\\test_file"), K4GB);
        
        KFileSizeMaxLargerThan4GBMinusOne = (r == KErrNone);
        r = TheFs.Delete(_L("\\test_file"));

        }
    else
        {//-- something else, exFAT for example
        if(Is_ExFat(TheFs, aDrive))    
            KFileSizeMaxLargerThan4GBMinusOne = ETrue; 
        }


	return KErrNone;
	}


void CallTestsL()
	{
	TInt r;
	r = RFs::CharToDrive(gDriveToTest, gDrive);
	test_KErrNone(r);

    //-- set up console output 
    F32_Test_Utils::SetConsole(test.Console()); 

    PrintDrvInfo(TheFs, gDrive);

    TestInlineReadWrite();
    
	r = PrepareDisk(gDrive);
	if(r == KErrNotSupported)
		return;


	TestRFile64NegLen();
	TestOpenFiles();
	TestAdoptFiles();
	TestReadFile(); 
	TestWriteFile(); 
	TestFileAccess(); 
	TestSetsize();
	TestReadFilesection();
	TestTFileText();

	TestLockUnLock(); 
	TestFileReSize();
	//
	// these tests require disk capacity of aaround 12GB.
	//order of these tests need to be preserved since the files are
	//created only in TestGetDirectory() and then deleted in TestAddLDirectory
	//but all the intermediate tests uses those files.
	//
	if (gDriveVolumeInfo.iFree >= K12GB)
		{
		TestGetDirectory();
		TestTEntry(); 
		TestReadDirectory();
		TestSortDirectory();
		TestAddLDirectory();
		}
	// these tests require disk capacity of aaround 9GB.
	if (gDriveVolumeInfo.iFree >= K9GB)
		{
		TestCopyMoveDirectory(); 
		}
	
	// Delete the test directory
	TFileName dirName;
	dirName.Append(gDriveToTest);
	dirName.Append(KTestPath);
	r = TheFs.RmDir(dirName);
	test_KErrNone(r);
	}







