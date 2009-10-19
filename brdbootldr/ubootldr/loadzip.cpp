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
// ubootldr\loadzip.cpp
// 
//

#define FILE_ID	0x4C5A4950

#include <e32std.h>
#include <e32std_private.h>
#include "bootldr.h"
#include "unzip.h"

TUint8 Buffer[1024];

GLDEF_C void AcceptUnzippedBlock(TZipInfo& aInfo, TUint8*& aOutPtr, TInt aError)
	{
	if (aError!=KErrNone)
		{
		PrintToScreen(_L("Error!\r\n"));
		BOOT_FAULT();
		}
#ifdef __SUPPORT_FLASH_REPRO__
	if (LoadToFlash)
		{
		// signal flash programming thread
		TInt got=(TInt)((TLinAddr)aOutPtr-(TLinAddr)DestinationAddress());
		NotifyDataAvailable(got);
		}
#endif
	}

GLDEF_C TInt UnzipComplete(TZipInfo& a, TUint8* aOutPtr, TInt aError)
	{
#ifdef __SUPPORT_FLASH_REPRO__
	if (LoadToFlash)
		NotifyDownloadComplete();
#endif
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
		delete pFileBuf;
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
	FileName=a.iName;
	LoadSize=a.iUncompressedSize;
	TInt size_mod_4k=LoadSize & 0xfff;
	if (size_mod_4k==0)
		ImageHeaderPresent=EFalse;
	else if (size_mod_4k==256)
		ImageHeaderPresent=ETrue;
	else
		{
		PrintToScreen(_L("\r\n\r\nInvalid size\r\n"));
		BOOT_FAULT();
		}
	ImageSize=ImageHeaderPresent ? LoadSize-256 : LoadSize;

	PrintToScreen(_L("Unzip %lS, size %d\r\n"),&FileName,LoadSize);

#ifdef __SUPPORT_FLASH_REPRO__
	if (LoadToFlash)
		{
		TInt r=InitFlashWrite();
		if (r!=KErrNone)
			{
			PrintToScreen(_L("InitFlashWrite returned %d\r\n"),r);
			BOOT_FAULT();
			}
		}
#endif

	a.iOutBuf=(TUint8*)DestinationAddress();

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
	a.iOutBuf=NULL;
	RThread& t=*(RThread*)&a.iThreadHandle;
	t.Close();
	}

TInt DoZipDownload(RFile &aBootFile)
	{
	TZipInfo z;
	z.iRemain=FileSize;
	InitProgressBar(0,(TUint)FileSize,_L("LOAD"));
	TInt r=Initialise(z);
	CHECK(r);
	RThread t;
	t.SetHandle(z.iThreadHandle);

	while (z.iRemain && z.iThreadStatus==KRequestPending)
		{
		TRequestStatus dummy;
		TRequestStatus* pS=&dummy;

		r=ReadBlockToBuffer(z, aBootFile);
		if (r != KErrNone)
			{
			PrintToScreen(_L("FAULT: Unzip Error %d\r\n"),r);
			if (z.iFileBufW-z.iFileBufR==z.iFileBufSize)
				{
				PrintToScreen(_L("Check there is only one image\n\rin the zip file.\r\n"));
				}
			CHECK(r);
			}

		UpdateProgressBar(0,(TUint)(FileSize-z.iRemain));
		t.RequestComplete(pS,0);		// same process
		while(z.iHeaderDone==0 && z.iThreadStatus==KRequestPending)
			{
			DELAY(20000);
			}
		if (z.iHeaderDone==1 && z.iThreadStatus==KRequestPending)
			{
			// after reading first block, process the header
			ProcessHeader(z);
			}
		}	// while

	User::WaitForRequest(z.iThreadStatus);

	TInt exitType=t.ExitType();
	TInt exitReason=t.ExitReason();
	if (z.iRemain || exitType!=EExitKill || exitReason!=KErrNone)
		{
		TBuf<32> exitCat=t.ExitCategory();
		PrintToScreen(_L("Exit code %d,%d,%S\n"),exitType,exitReason,&exitCat);
		TEST(0);
		}

	PrintToScreen(_L("Unzip complete\r\n"));
	
	TUint8* pD=Buffer;
	TInt len=1024;

	r=ReadInputData(pD,len);
	TEST(r==KErrEof);


	DELAY(20000);

	Cleanup(z);
	return KErrNone;
	}

