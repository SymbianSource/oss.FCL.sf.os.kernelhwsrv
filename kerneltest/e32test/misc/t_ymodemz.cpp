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
// e32test\misc\t_ymodemz.cpp
// 
//

#include <e32test.h>
#include "ymodemu.h"
#include <f32file.h>
#include "unzip.h"

RTest test(_L("YModemZ"));

#define TEST(c)		((void)((c)||(test.Printf(_L("Failed at line %d\n"),__LINE__),test.Getch(),test(0),0)))
#define CHECK(c)	((void)(((c)==0)||(test.Printf(_L("Error %d at line %d\n"),(c),__LINE__),test.Getch(),test(0),0)))

const TInt KBufferSize=4096;

_LIT(KLddName,"ECOMM");
_LIT(KPddName,"EUART");

RFs TheFs;
RFile TheOutputFile;
YModemU* TheYModem;

void LoadCommDrivers()
	{
	test.Printf(_L("Load LDD\n"));
	TInt r=User::LoadLogicalDevice(KLddName);
	TEST(r==KErrNone || r==KErrAlreadyExists);

	TInt i;
	TInt n=0;
	for (i=-1; i<10; ++i)
		{
		TBuf<16> pddName=KPddName();
		if (i>=0)
			pddName.Append('0'+i);
		TInt r=User::LoadPhysicalDevice(pddName);
		if (r==KErrNone || r==KErrAlreadyExists)
			{
			++n;
			test.Printf(_L("%S found\n"),&pddName);
			}
		}
	TEST(n!=0);
	}

GLDEF_C void AcceptUnzippedBlock(TZipInfo& aInfo, TUint8*& aOutPtr, TInt aError)
	{
	if (aError==KErrNone)
		{
		TInt avail=aOutPtr-aInfo.iOutBuf;
		if (avail>=KZipWindowSize+0x1000)
			{
			TInt len=avail-KZipWindowSize;
			TPtrC8 ptr(aInfo.iOutBuf,len);
			TInt r=TheOutputFile.Write(ptr);
			CHECK(r);
			Mem::Copy(aInfo.iOutBuf,aInfo.iOutBuf+len,KZipWindowSize);
			aOutPtr=aInfo.iOutBuf+KZipWindowSize;
			}
		}
	}

GLDEF_C TInt ReadInputData(TUint8* aDest, TInt& aLength)
	{
	TUint8* pD=aDest;
//	test.Printf(_L("@%dms\n"),User::NTickCount());
	TInt r=TheYModem->ReadPackets(pD,aLength);
//	test.Printf(_L("ReadIP %d\n"),r);
	aLength=pD-aDest;
	return r;
	}

GLDEF_C TInt UnzipComplete(TZipInfo& a, TUint8* aOutPtr, TInt aError)
	{
	TInt r=aError;
	if (r==KErrNone && aOutPtr>a.iOutBuf)
		r=TheOutputFile.Write(TPtrC8(a.iOutBuf,aOutPtr-a.iOutBuf));
	CHECK(r);
	return r;
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

	TInt r=TheOutputFile.Replace(TheFs,a.iName,EFileWrite);
	CHECK(r);
	test.Printf(_L("Allocate memory for unzipped file\n"));
	a.iOutBuf=(TUint8*)User::Alloc(262144);
	TEST(a.iOutBuf!=NULL);
	test.Printf(_L("Begin unzipping\n"));
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
//	RThread().SetSystem(ETrue);
	RThread().SetPriority(EPriorityAbsoluteForeground);
	test.SetLogged(EFalse);
	test.Title();

	TBuf<256> cmd;
	User::CommandLine(cmd);
	TInt port=0;
	if (cmd.Length()!=0)
		{
		TUint8 c=(TUint8)cmd[0];
		if (c>='0' && c<='9')
			{
			port=c-'0';
			}
		}

	TInt r=KErrNone;
	LoadCommDrivers();

	test.Printf(_L("Connect to file server\n"));
	r=TheFs.Connect();
	CHECK(r);
	r=TheFs.ShareAuto();
	CHECK(r);

	test.Printf(_L("Create YModem object\n"));
	YModemU* pY=NULL;
	TRAP(r,pY=YModemU::NewL(port,ETrue));
	TEST(r==KErrNone && pY!=NULL);
	TheYModem=pY;

	test.Printf(_L("Create buffer\n"));
	TUint8* buffer=(TUint8*)User::Alloc(KBufferSize);
	TEST(buffer!=NULL);

	test.Printf(_L("Receive...\n"));

	TBool mode=1;
	FOREVER
		{
		TInt total_size=0;
		TInt size=-1;
		TBuf<256> name;
		r=pY->StartDownload(mode, size, name);
//		test.Printf(_L("@%dms"),User::NTickCount());
		if (r!=KErrNone)
			break;
		test.Printf(_L("r=%d, size=%d, name %S\n"),r,size,&name);
		if (r==KErrNone && name.Right(4).CompareF(_L(".zip"))==0 && size!=0)
			{
			test.Printf(_L("Initialising unzip...\n"));
			TZipInfo z;
			z.iRemain=size;
			r=Initialise(z);
			CHECK(r);
			test.Printf(_L("Read header\n"));
			RThread t;
			t.SetHandle(z.iThreadHandle);
			while (z.iRemain && z.iThreadStatus==KRequestPending)
				{
				TRequestStatus dummy;
				TRequestStatus* pS=&dummy;
//				test.Printf(_L("remain=%d\n"),z.iRemain);
				r=ReadBlockToBuffer(z);
				CHECK(r);
				t.RequestComplete(pS,0);		// same process
				while(z.iHeaderDone==0 && z.iThreadStatus==KRequestPending)
					DELAY(20000);
				if (z.iHeaderDone==1 && z.iThreadStatus==KRequestPending)
					{
					// after reading first block, process the header
					ProcessHeader(z);
					}
				}
			test.Printf(_L("\nWait for thread to exit\n"));
			User::WaitForRequest(z.iThreadStatus);
			TInt exitType=t.ExitType();
			TInt exitReason=t.ExitReason();
			if (z.iRemain || exitType!=EExitKill || exitReason!=KErrNone)
				{
				TBuf<32> exitCat=t.ExitCategory();
				test.Printf(_L("Exit code %d,%d,%S\n"),exitType,exitReason,&exitCat); test.Getch(); test(0);
				}
			TUint8* pD=buffer;
			r=pY->ReadPackets(pD,KBufferSize);	// should get EOF response
			TEST(r==KErrEof);
			Cleanup(z);
			TheOutputFile.Close();
			}
		else if (r==KErrNone)
			{
			test.Printf(_L("Opening file for write\n"));
			RFile file;
			r=file.Replace(TheFs,name,EFileWrite);
			if (r!=KErrNone)
				{
				test.Printf(_L("RFile::Replace returns %d\n"),r); test.Getch();	test(0);
				}
			while (r==KErrNone)
				{
				TUint8* pD=buffer;
				r=pY->ReadPackets(pD,KBufferSize);
				if (r==KErrNone || r==KErrEof)
					{
					TInt blen=pD-buffer;
					if (size>0)				// size was transmitted
						{
						if (blen>size-total_size)
							blen=size-total_size;
						}
					total_size+=blen;
					TPtrC8 fptr(buffer,blen);
					TInt s=file.Write(fptr);
					if (s!=KErrNone)
						{
						test.Printf(_L("RFile::Write returns %d\n"),s); test.Getch(); test(0);
						}
					}
				}
			file.Close();
			test.Printf(_L("rx size=%d\n"),total_size);
			}
		}
	delete buffer;
	delete pY;
	TheFs.Close();
	test.Printf(_L("r=%d\n"),r);
	test.Getch();

	return KErrNone;
	}

