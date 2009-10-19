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
// e32test\misc\flash.cpp
// 
//

#include <e32test.h>
#include <f32file.h>
#include "flash.h"

RTest test(_L("FLASH"));
TInt Mode;
TFileName FileName;
RFs Fs;
RFile File;
TUint32 StartAddr;
TUint32 Size;

const TUint KBufferSize=4096;
TUint8 Buffer[KBufferSize];
TUint8 Buffer2[KBufferSize];

enum TMode
	{
	EDump,
	EBlankCheck,
	EErase,
	EWrite,
	EVerify,
	};

void Usage()
	{
	test.Printf(_L("Usage:\n"));
	test.Printf(_L(" flash d <addr> <size> <file> - dump flash to file\n"));
	test.Printf(_L(" flash b <addr> <size>        - blank check flash\n"));
	test.Printf(_L(" flash e <addr> <size>        - erase flash\n"));
	test.Printf(_L(" flash w <addr> <file>        - write file to flash\n"));
	test.Printf(_L(" flash v <addr> <file>        - verify flash against file\n"));
	}

TInt ParseCommandLine()
	{
	TBuf<256> cmd;
	User::CommandLine(cmd);
	cmd.Trim();
	if (cmd.Length()==0)
		return KErrArgument;
	TLex lex(cmd);
	TPtrC mode_str=lex.NextToken();
	switch (mode_str[0])
		{
		case 'D':
		case 'd':
			Mode=EDump; break;
		case 'B':
		case 'b':
			Mode=EBlankCheck; break;
		case 'E':
		case 'e':
			Mode=EErase; break;
		case 'W':
		case 'w':
			Mode=EWrite; break;
		case 'V':
		case 'v':
			Mode=EVerify; break;
		default:
			return KErrArgument;
		}
	lex.SkipSpace();
	TInt r=lex.Val(StartAddr,EHex);
	if (r!=KErrNone)
		return r;
	if (Mode==EDump || Mode==EBlankCheck || Mode==EErase)
		{
		lex.SkipSpace();
		r=lex.Val(Size,EHex);
		if (r!=KErrNone)
			return r;
		}
	if (Mode==EDump || Mode==EWrite || Mode==EVerify)
		{
		lex.SkipSpace();
		FileName=lex.NextToken();
		if (FileName.Length()==0)
			return KErrArgument;
		if (Mode==EDump)
			{
			r=File.Replace(Fs,FileName,EFileWrite);
			if (r!=KErrNone)
				{
				test.Printf(_L("\nCould not open file %S for write\n"),&FileName);
				return KErrGeneral;
				}
			}
		else
			{
			r=File.Open(Fs,FileName,EFileRead);
			if (r!=KErrNone)
				{
				test.Printf(_L("\nCould not open file %S for read\n"),&FileName);
				return KErrGeneral;
				}
			else
				{
				r=File.Size((TInt&)Size);
				if (r!=KErrNone)
					{
					test.Printf(_L("\nError %d accessing file %S\n"),r,&FileName);
					File.Close();
					return KErrGeneral;
					}
				test.Printf(_L("File size %x\n"),Size);
				}
			}
		}
	return KErrNone;
	}

void DoCommand()
	{
	Flash* pF=Flash::New(StartAddr);
	if (!pF)
		{
		test.Printf(_L("Problem with flash device\n"));
		return;
		}
	TInt r=KErrNotSupported;
	switch (Mode)
		{
		case EDump:
			{
			while (Size)
				{
				TUint size=Size;
				if (size>KBufferSize)
					size=KBufferSize;
				r=pF->Read(StartAddr,size,Buffer);
				if (r!=KErrNone)
					break;
				r=File.Write(TPtrC8(Buffer,size));
				if (r!=KErrNone)
					break;
				StartAddr+=size;
				Size-=size;
				}
			break;
			}
		case EBlankCheck:
			{
			r=pF->BlankCheck(StartAddr,Size);
			if (r!=KErrNone)
				{
				test.Printf(_L("Error at address %08x\n"),StartAddr+r-1);
				}
			break;
			}
		case EErase:
			r=pF->Erase(StartAddr,Size);
			if (r!=KErrNone)
				{
				test.Printf(_L("Error at address %08x\n"),StartAddr+r-1);
				}
			break;
		case EWrite:
			{
			while (Size)
				{
				test.Printf(_L("%08x\n"),StartAddr);
				TUint size=Size;
				if (size>KBufferSize)
					size=KBufferSize;
				TPtr8 p8(Buffer,0,size);
				r=File.Read(p8);
				if (r!=KErrNone)
					break;
				r=pF->Write(StartAddr,size,Buffer);
				if (r!=KErrNone)
					break;
				StartAddr+=size;
				Size-=size;
				}
			break;
			}
		case EVerify:
			{
			while (Size)
				{
				TUint size=Size;
				if (size>KBufferSize)
					size=KBufferSize;
				r=pF->Read(StartAddr,size,Buffer);
				if (r!=KErrNone)
					break;
				TPtr8 p8(Buffer2,0,size);
				r=File.Read(p8);
				if (r!=KErrNone)
					break;
				TUint i;
				for (i=0; i<size; ++i)
					{
					if (Buffer2[i]!=Buffer[i])
						{
						test.Printf(_L("VERIFY ERROR: Flash addr %08x, flash %02x, file %02x\n"),
									StartAddr+i,Buffer[i],Buffer2[i]);
						return;
						}
					}
				StartAddr+=size;
				Size-=size;
				}
			break;
			}
		}
	if (r!=KErrNone)
		test.Printf(_L("Error %d occurred\n"),r);
	delete pF;
	}

TInt E32Main()
	{
	test.Title();
	test.Start(_L("Connect to F32"));
	TInt r=Fs.Connect();
	test(r==KErrNone);

	r=ParseCommandLine();
	if (r==KErrNone)
		DoCommand();
	else
		Usage();

	Fs.Close();
	test.End();
	return 0;
	}
