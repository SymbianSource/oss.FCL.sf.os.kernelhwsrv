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
// e32test\misc\t_ymodem.cpp
// 
//

#include <e32test.h>
#include "ymodemu.h"
#include <f32file.h>

RTest test(_L("YModem"));

#define TEST(c)	((void)((c)||(test.Printf(_L("Failed at line %d\n"),__LINE__),test.Getch(),test(0),0)))

const TInt KBufferSize=4096;

_LIT(KLddName,"ECOMM");
_LIT(KPddName,"EUART");

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

GLDEF_C TInt E32Main()
	{
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

	test.Next(_L("Connect to file server"));
	RFs fs;
	r=fs.Connect();
	test(r==KErrNone);

	test.Next(_L("Create YModem object"));
	YModemU* pY=NULL;
	TRAP(r,pY=YModemU::NewL(port,ETrue));
	test(r==KErrNone && pY!=NULL);

	test.Next(_L("Create buffer"));
	TUint8* buffer=(TUint8*)User::Alloc(KBufferSize);
	test(buffer!=NULL);

	test.Start(_L("Receive..."));

	TBool mode=1;
	FOREVER
		{
		TInt total_size=0;
		TInt size=-1;
		TBuf<256> name;
		r=pY->StartDownload(mode, size, name);
		if (r!=KErrNone)
			break;
		test.Printf(_L("r=%d, size=%d, name %S\n"),r,size,&name);
		if (r==KErrNone)
			{
			test.Printf(_L("Opening file for write\n"));
			RFile file;
			r=file.Replace(fs,name,EFileWrite);
			if (r!=KErrNone)
				{
				test.Printf(_L("RFile::Replace returns %d\n"),r);
				test.Getch();
				test(0);
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
						test.Printf(_L("RFile::Write returns %d\n"),s);
						test.Getch();
						test(0);
						}
					}
				}
			file.Close();
			test.Printf(_L("rx size=%d\n"),total_size);
			}
		}
	delete buffer;
	delete pY;
	fs.Close();
	test.Printf(_L("r=%d\n"),r);
	test.Getch();

	return KErrNone;
	}
