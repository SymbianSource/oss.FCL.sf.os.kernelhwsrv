// Copyright (c) 1998-2009 Nokia Corporation and/or its subsidiary(-ies).
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
// e32test\misc\t_unzip.cpp
// 
//

#include <e32test.h>
#include <f32file.h>
#include "unzip.h"

RTest test(_L("T_UNZIP"));

RFile TheInputFile;
TUint8* OutPtr;

GLDEF_C void AcceptUnzippedBlock(TZipInfo& /*aInfo*/, TUint8*& aOutPtr, TInt aError)
	{
	if (aError==KErrNone)
		OutPtr=aOutPtr;
	}

GLDEF_C TInt ReadInputData(TUint8* aDest, TInt& aLength)
	{
	TPtr8 ptr(aDest,0,aLength);
	return TheInputFile.Read(ptr);
	}

GLDEF_C TInt UnzipComplete(TZipInfo& /*a*/, TUint8* /*aOutPtr*/, TInt /*aError*/)
	{
	return KErrNone;
	}

_LIT(KLitThreadName,"Unzip");
TInt Initialise(TZipInfo& a)
	{
	TInt r=InitInfo(a);
	if (r!=KErrNone)
		return r;
	a.iFileBufSize=4*a.iInBufSize;
	TAny* pFileBuf=MALLOC(a.iFileBufSize);
	if (!pFileBuf)
		return KErrNoMemory;
	a.iFileBuf=(TUint8*)pFileBuf;
	RThread t;
	r=t.Create(KLitThreadName,UnzipThread,0x2000,NULL,&a);
	if (r!=KErrNone)
		{
		FREE(pFileBuf);
		a.iFileBuf=NULL;
		return r;
		}
	t.SetPriority(EPriorityLess);
	t.Logon(a.iThreadStatus);
	t.Resume();
	a.iThreadHandle=t.Handle();
	return KErrNone;
	}

void ProcessHeader(TZipInfo& a)
	{
	test.Printf(_L("Flags=%d\n"),a.iFlags);
	test.Printf(_L("Method=%d\n"),a.iMethod);
	test.Printf(_L("Crc=%d\n"),a.iCrc);
	test.Printf(_L("Compressed size=%d\n"),a.iCompressedSize);
	test.Printf(_L("Uncompressed size=%d\n"),a.iUncompressedSize);
	test.Printf(_L("File name %S\n"),&a.iName);
	test.Printf(_L("Data offset %d\n\n"),a.iDataOffset);

	test.Next(_L("Allocate memory for unzipped file"));
	a.iOutBuf=(TUint8*)User::Alloc(a.iUncompressedSize);
	test(a.iOutBuf!=NULL);
	test.Next(_L("Begin unzipping"));
	a.iHeaderDone=2;
	TRequestStatus* pS=&a.iProcessedHeader;
	RThread t;
	t.SetHandle(a.iThreadHandle);
	t.RequestComplete(pS,0);
	}

void Cleanup(TZipInfo& a)
	{
	delete a.iFileBuf;
	a.iFileBuf=NULL;
	delete a.iOutBuf;
	a.iOutBuf=NULL;
	RThread& t=*(RThread*)&a.iThreadHandle;
	t.Close();
	}

GLDEF_C TInt E32Main()
	{
	test.Title();
	TFileName inputFileName;
	User::CommandLine(inputFileName);
	test.Start(_L("Connect to file server"));
	RFs fs;
	TInt r=fs.Connect();
	test(r==KErrNone);
	test.Printf(_L("Open file %S\n"),&inputFileName);
	r=TheInputFile.Open(fs,inputFileName,EFileRead);
	test(r==KErrNone);
	TZipInfo z;
	r=TheInputFile.Size(z.iRemain);
	test(r==KErrNone);
	test.Printf(_L("File size %d\n"),z.iRemain);

	test.Next(_L("Initialise"));
	r=Initialise(z);
	test(r==KErrNone);

	test.Next(_L("Read header"));
	TUint32 c=0;
	RThread t;
	t.SetHandle(z.iThreadHandle);
	while (z.iRemain && z.iThreadStatus==KRequestPending)
		{
		TRequestStatus dummy;
		TRequestStatus* pS=&dummy;
		r=ReadBlockToBuffer(z);
		test(r==KErrNone);
		t.RequestComplete(pS,0);		// same process
//		test.Printf(_L("."));
		while(z.iHeaderDone==0 && z.iThreadStatus==KRequestPending)
			DELAY(20000);
		if (z.iHeaderDone==1 && z.iThreadStatus==KRequestPending)
			{
			// after reading first block, process the header
			ProcessHeader(z);
			c=User::NTickCount();
			}
		}

	test.Next(_L("\nWait for thread to exit"));
	User::WaitForRequest(z.iThreadStatus);
	if (z.iRemain || t.ExitReason()!=KErrNone)
		{
		test.Printf(_L("Error %d\n"),t.ExitReason());
		test(0);
		}
	TUint c2=User::NTickCount();
	test.Printf(_L("Took %dms\n"),c2-c);
	TheInputFile.Close();
	test.Getch();

	TInt unc_size=OutPtr-z.iOutBuf;
	test.Printf(_L("Recovered size %d\n"),unc_size);
	test.Printf(_L("Writing to file\n"));
	RFile file;
	r=file.Replace(fs,z.iName,EFileWrite);
	test(r==KErrNone);
	TPtrC8 ptr(z.iOutBuf,unc_size);
	r=file.Write(ptr);
	file.Close();

	fs.Close();
	Cleanup(z);

	return KErrNone;
	}

