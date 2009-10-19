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
// e32test\misc\drvdump.cpp
// 
//

#include <e32test.h>
#include <e32svr.h>
#include <d32locd.h>

RTest test(_L("DriveDump"));
CConsoleBase* C;
TBool Changed;
TBusLocalDrive D;
TInt Drive;
TBool SerialOut=ETrue;

void OpenDrive()
	{
	D.Close();
	TInt r=D.Connect(Drive, Changed);
	test(r==KErrNone);
	}

TInt Getch()
	{
	return (TInt)C->Getch();
	}

void Print(const TDesC& aDes)
	{
	C->Write(aDes);
	if (SerialOut)
		RDebug::RawPrint(aDes);
	}

void putc(TUint aChar)
	{
	TBuf<1> b;
	b.SetLength(1);
	b[0]=(TText)aChar;
	Print(b);
	}

void NewLine()
	{
	Print(_L("\r\n"));
	}

void PrintLine(const TDesC& aDes)
	{
	Print(aDes);
	NewLine();
	}

void Dump(const TAny* aStart, TInt aLength, const TInt64& aAddr)
	{
	TBuf<80> out;
	TBuf<20> ascii;
	TUint a=(TUint)aStart;
	TInt64 da=aAddr;
	do
		{
		out.Zero();
		ascii.Zero();
		out.AppendNum(I64HIGH(da),EHex);
		out.AppendNumFixedWidth(I64LOW(da), EHex,8);
		out.Append(_L(": "));
		TUint b;
		for (b=0; b<16; b++)
			{
			TUint8 c=*(TUint8*)(a+b);
			out.AppendNumFixedWidth(c,EHex,2);
			out.Append(' ');
			if (b==7)
				out.Append(_L("| "));
			if (c<0x20 || c>=0x7f)
				c=0x2e;
			ascii.Append(TChar(c));
			}
		out.Append(ascii);
		PrintLine(out);
		a+=16;
		aLength-=16;
		da+=16;
		} while(aLength>0);
	}

void ReadDrive(TAny* aDest, const TInt64& aPos, TInt aSize)
	{
	TInt64 pos=aPos;
	TPtr8 p((TUint8*)aDest, 0, aSize);
	TInt r=D.Read(pos, aSize, p);
	if (r!=KErrNone)
		{
		test.Printf(_L("r=%d\n"), r);
		test(r==KErrNone);
		}
	}

TInt E32Main()
	{
	test.Title();
	C=test.Console();

	TInt64 pos;
	TUint size;
	TBuf<256> cmdBuf;
	User::CommandLine(cmdBuf);
	TLex cmd(cmdBuf);
	cmd.SkipSpace();
	test(cmd.Val(Drive)==KErrNone);
	cmd.SkipSpace();
	test(cmd.Val(pos,EHex)==KErrNone);
	cmd.SkipSpace();
	test(cmd.Val(size,EHex)==KErrNone);

	TUint8* buf=(TUint8*)User::Alloc(size);

	OpenDrive();

	TBuf<80> out;
	out.Format(_L("Drive %d"),Drive);
	PrintLine(out);

	ReadDrive(buf, pos, size);
	Dump(buf, size, pos);

	User::Free(buf);
	D.Close();

	return 0;
	}


