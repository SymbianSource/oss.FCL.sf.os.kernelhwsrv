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
// e32test\misc\drvread.cpp
// 
//

#include <e32test.h>
#include <e32svr.h>
#include <d32locd.h>

RTest test(_L("DriveRead"));
CConsoleBase* C;
TRequestStatus KeyStat;
TBool Changed;
TBusLocalDrive D;
TInt Drive;

void OpenDrive()
	{
	D.Close();
	TInt r=D.Connect(Drive, Changed);
	test(r==KErrNone);
	}

TInt Inkey()
	{
	if (KeyStat==KRequestPending)
		return -1;
	User::WaitForRequest(KeyStat);
	TKeyCode k=C->KeyCode();
	C->Read(KeyStat);
	return (TInt)k;
	}

void ReadDrive(TAny* aDest, const TInt64& aPos, TInt aSize)
	{
	TInt64 pos=aPos;
	TPtr8 p((TUint8*)aDest, 0, aSize);
	TInt r=D.Read(pos, aSize, p);
	test(r==KErrNone);
	}

TInt E32Main()
	{
	test.Title();
	C=test.Console();
	C->Read(KeyStat);

	TInt increment;
	TBuf<256> cmdBuf;
	User::CommandLine(cmdBuf);
	TLex cmd(cmdBuf);
	cmd.SkipSpace();
	test(cmd.Val(Drive)==KErrNone);
	cmd.SkipSpace();
	test(cmd.Val(*(TUint32*)&increment,EHex)==KErrNone);

	TInt block_size=0x10000;
	TUint8* buf=(TUint8*)User::Alloc(block_size);
	test(buf!=NULL);

	OpenDrive();

	TLocalDriveCapsV2 caps;
	TPckg<TLocalDriveCapsV2> capsPckg(caps);
	TInt r=D.Caps(capsPckg);
	test(r==KErrNone);
	TInt64 drive_size=caps.iSize;
	TInt64 pos=0;
	TUint32 fc=User::NTickCount();
	test.Printf(_L("Drive size = %x%08x\n"),I64HIGH(drive_size),I64LOW(drive_size));

	FOREVER
		{
		TInt k=Inkey();
		if (k==EKeyEscape)
			break;
		ReadDrive(buf, pos, block_size);
		pos+=TInt64(increment);
		if (pos+TInt64(block_size) > drive_size)
			pos=0;
		if ((User::NTickCount()-fc)>1000)
			{
			fc=User::NTickCount();
			test.Printf(_L("Pos=%x%08x\n"),I64HIGH(pos),I64LOW(pos));
			}
		}

	User::Free(buf);
	D.Close();

	return 0;
	}


