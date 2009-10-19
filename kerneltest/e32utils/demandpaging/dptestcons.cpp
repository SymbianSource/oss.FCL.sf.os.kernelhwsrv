// Copyright (c) 2007-2009 Nokia Corporation and/or its subsidiary(-ies).
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
// e32utils\demandpaging\dptestcons.cpp
// 
//

#include <e32cons.h>
#include <dptest.h>

CConsoleBase* TheConsole;


void doHelp()
	{
	TheConsole->Printf(_L("Usage: dptestcons [-h] [-o] [-f] [-sMIN,MAX]\n"));
	TheConsole->Printf(_L("\n"));
	TheConsole->Printf(_L("-h    Prints this help information\n"));
	TheConsole->Printf(_L("-o    Prints all DPTest information\n"));
	TheConsole->Printf(_L("-f    Flushes the Demand Paging cache\n"));
	TheConsole->Printf(_L("-s    Sets the Demand Paging cache size (MIN and MAX in bytes)\n"));
	}

void doDPTestInformation()
	{
	TheConsole->Printf(_L("DPTest information\n==================\n"));
	TUint32 attribs = DPTest::Attributes();
	_LIT(KOn, "ON");
	_LIT(KOff, "OFF");
	TheConsole->Printf(_L("ROM Paging:  %S\n"), (attribs&DPTest::ERomPaging) ? &KOn : &KOff);
	TheConsole->Printf(_L("Code Paging: %S\n"), (attribs&DPTest::ECodePaging) ? &KOn : &KOff);
	TheConsole->Printf(_L("Data Paging: %S\n"), (attribs&DPTest::EDataPaging) ? &KOn : &KOff);
	TUint min;
	TUint max;
	TUint current;
	TInt err = DPTest::CacheSize(min, max, current);
	if (err == KErrNone)
		{
		TheConsole->Printf(_L("CacheSize: Min=%u, Max=%u, Current=%u\n"), min, max, current);
		}
	else
		{
		TheConsole->Printf(_L("CacheSize failed with err=%d\n"), err);
		}
	TPckgBuf<DPTest::TEventInfo> eventPckg;
	err = DPTest::EventInfo(eventPckg);
	if (err == KErrNone)
		{
		TheConsole->Printf(_L("EventInfo: PageFaultCount=%Lu, PageInCount=%Lu\n"), eventPckg().iPageFaultCount, eventPckg().iPageInReadCount);
		}
	else
		{
		TheConsole->Printf(_L("EventInfo failed with err=%d\n"), err);
		}
	TheConsole->Printf(_L("==================\n"));
	}

void doFlushCache()
	{
	TInt err = DPTest::FlushCache();
	TheConsole->Printf(_L("FlushCache completed with err=%d\n"), err);
	}

void doSetCacheSize(const TDesC& aCacheSizes)
	{
	TLex lex(aCacheSizes);
	TChar c = lex.Peek();
	while (c != 0 && c != ',')
		{
		lex.Inc();
		c = lex.Peek();
		}
	TPtrC valDes = lex.MarkedToken();
	TUint min;
	if (TLex(valDes).Val(min) != KErrNone)
		{
		TheConsole->Printf(_L("Bad minimum value provided for SetCacheSize: %S\n"), &valDes);
		return;
		}
	if (c != 0)
		{
		lex.Inc();
		}
	valDes.Set(lex.Remainder());
	TUint max;
	if (TLex(valDes).Val(max) != KErrNone)
		{
		TheConsole->Printf(_L("Bad maximum value provided for SetCacheSize: %S\n"), &valDes);
		return;
		}
	TInt err = DPTest::SetCacheSize(min, max);
	TheConsole->Printf(_L("SetCacheSize (Min=%u, Max=%u) completed with err=%d\n"), min, max, err);
	}

void processCommands(RArray<TPtrC> aArgArray)
	{
	const TInt count = aArgArray.Count();
	if (count == 0)
		{
		doHelp();
		return;
		}
	for (TInt ii=0; ii<count; ii++)
		{
		TPtrC current = aArgArray[ii];
		if (current.Length() < 2 || current[0] != '-')
			{
			TheConsole->Printf(_L("Unknown command: %S\n"), &current);
			}
		switch (current[1])
			{
		case 'h':
			doHelp();
			break;
		case 'o':
			doDPTestInformation();
			break;
		case 'f':
			doFlushCache();
			break;
		case 's':
			doSetCacheSize(current.Mid(2));
			break;
		default:
			TheConsole->Printf(_L("Unknown command: %S\n"), &current);
			break;
			}
		}
	}

TInt E32Main()
	{
	// create console...
	TFileName exeFile = RProcess().FileName();
	TRAPD(err, TheConsole = Console::NewL(exeFile,TSize(KConsFullScreen,KConsFullScreen)));
	TheConsole->Printf(_L("---DPTESTCONS---\n"));
	if (err != KErrNone)
		{
		return err;
		}
	// get command-line...
	RBuf clDes;
	if (clDes.Create(User::CommandLineLength()+1) != KErrNone)
		{
		return err;
		}
	User::CommandLine(clDes);

	// split up args...
	RArray<TPtrC> argArray;
	TLex lex(clDes);
	while (!lex.Eos())
		{
		err = argArray.Append(lex.NextToken());
		if (err != KErrNone)
			{
			return err;
			}
		}
	processCommands(argArray);

	TheConsole->Printf(_L("Press any key to continue.\n"));
	TheConsole->Getch();
	return 0;
	}
