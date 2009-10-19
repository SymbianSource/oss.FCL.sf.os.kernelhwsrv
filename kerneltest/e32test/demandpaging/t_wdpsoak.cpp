// Copyright (c) 2005-2009 Nokia Corporation and/or its subsidiary(-ies).
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
// e32test\mmu\t_wdpsoak.cpp
// 
//

#define __E32TEST_EXTENSION__
#include <e32test.h>
#include <dptest.h>
#include <hal.h>
#include "../mmu/mmudetect.h"
#include "../mmu/freeram.h"

#define MAX_CHUNKS	10
#define PRINT(string) if (!gQuiet) test.Printf(string)
#define PRINT1(string,param) if (!gQuiet) test.Printf(string,param)
#define TESTNEXT(string) if (!gQuiet) test.Next(string)

//------------globals---------------------
LOCAL_D RTest test(_L("T_WDPSOAK"));
LOCAL_D TInt gPageSize = 0;
LOCAL_D TUint gChunkSize = 0;		// default chunk size
LOCAL_D RChunk gChunk[MAX_CHUNKS];
LOCAL_D TUint gNextChunk = 0;
LOCAL_D TBool gQuiet = EFalse;
LOCAL_D TUint gPeriod = 0;
LOCAL_D TUint gMin = 0;
LOCAL_D TUint gMax = 0;
LOCAL_D TUint gMemScheme = 0;

const TUint32 KFlushQuietLimit = 100000;

TUint64 SwapFree()
	{
	SVMSwapInfo swapInfo;
	test_KErrNone(UserSvr::HalFunction(EHalGroupVM, EVMHalGetSwapInfo, &swapInfo, 0));
	
	return swapInfo.iSwapFree;
	}

TUint64 SwapSize()
	{
	SVMSwapInfo swapInfo;
	test_KErrNone(UserSvr::HalFunction(EHalGroupVM, EVMHalGetSwapInfo, &swapInfo, 0));
	
	return swapInfo.iSwapSize;
	}

void CacheSize(TUint aMin, TUint aMax)
	{
	SVMCacheInfo info;
	if (UserSvr::HalFunction(EHalGroupVM,EVMHalGetCacheSize,&info,0) != KErrNone)
		{
		return;
		}
	
	if (aMin > 0 || aMax > 0)
		{
		if (aMin > 0)
			{
			info.iMinSize = aMin;
			}
		if (aMax > 0)
			{
			info.iMaxSize = aMax;
			}
		UserSvr::HalFunction(EHalGroupVM,EVMHalSetCacheSize,(TAny*)info.iMinSize,(TAny*)info.iMaxSize);
		if (UserSvr::HalFunction(EHalGroupVM,EVMHalGetCacheSize,&info,0) != KErrNone)
			{
			return;
			}
		}

	PRINT1(_L("Paging Cache min size %d"),info.iMinSize);
	PRINT1(_L("  max size %d"),info.iMaxSize);
	PRINT1(_L("  current size %d\n"),info.iCurrentSize);
	}

void ShowMemoryUse()
	{
	PRINT1(_L("RAM free 0x%08X bytes"),FreeRam());
	PRINT1(_L("  Swap free 0x%08X bytes\n"),SwapFree());

	TPckgBuf<DPTest::TEventInfo> infoBuf;
	TInt r = UserSvr::HalFunction(EHalGroupVM,EVMHalGetEventInfo,&infoBuf,0);
	if (r!=KErrNone)
		{
		return;
		}
	PRINT1(_L("Page fault count %d"),infoBuf().iPageFaultCount);
	PRINT1(_L("  Page IN count %d\n"),infoBuf().iPageInReadCount);

	return;
	}

void ShowHelp()
	{
	PRINT(_L("***************************************\n"));
	PRINT(_L("The following are immediate commands\n"));
	PRINT(_L("F       flush the paging cache\n"));
	PRINT(_L("I       show memory information\n"));
	PRINT(_L("?       show this help\n"));
	PRINT(_L("Rn      read all pages of chunk n\n"));
	PRINT(_L("Wn      write all pages of chunk n\n"));
	PRINT(_L("Mn      periodic memory scheme n\n"));
	PRINT(_L("The following require a <CR> termination\n"));
	PRINT(_L("C=nnnn  create a chunnk of size nnnn\n"));
	PRINT(_L("L=nnnn  paging cache min size nnnn\n"));
	PRINT(_L("H=nnnn  paging cache max size nnnn\n"));
	PRINT(_L("P=nnnn  periodic flush/memory scheme nnnn microseconds\n"));
	PRINT(_L("Esc     to exit\n"));
	PRINT(_L("***************************************\n"));
	}

void CreateChunk(RChunk * aChunk, TUint aSize)
	{
	TESTNEXT(_L("Creating a paged chunk"));
	TChunkCreateInfo createInfo;
	PRINT1(_L("Creating chunk size 0x%08X bytes "),aSize);
	PRINT1(_L("at index %d\n"),gNextChunk);
	createInfo.SetPaging(TChunkCreateInfo::EPaged);
	createInfo.SetNormal(aSize,aSize);
	test_KErrNone(aChunk->Create(createInfo));
	}

void ReadChunk(RChunk * aChunk)
	{
	TESTNEXT(_L("Reading from each page of chunk"));
	TUint8* chunkBase = aChunk->Base();
	
	TUint8 chunkVal = 0;
	for (TInt i = 0; i < aChunk->Size(); i += gPageSize)
		{
		chunkVal = * (chunkBase + i);
		}
	
	// only needed to remove compiler warning on unused variable
	if (chunkVal)
		chunkVal = 0;
	
	return;
	}

void WriteChunk(RChunk * aChunk, TUint8 aValue = 0)
	{
	static TUint8 lastWriteValue = 1;
	TESTNEXT(_L("Writing to each page of chunk"));
	TUint8* chunkBase = aChunk->Base();
	
	lastWriteValue = (TUint8)(aValue == 0 ? lastWriteValue + 1 : aValue);
	for (TInt i = 0; i < aChunk->Size(); i += gPageSize)
		{
		* (chunkBase + i) = lastWriteValue;
		}

	return;
	}

void ParseCommandLine ()
	{
	TBuf<64> c;
	
	User::CommandLine(c);
	c.LowerCase();

	if (c != KNullDesC)
		{
		TLex lex(c);
		TPtrC token;

		while (token.Set(lex.NextToken()), token != KNullDesC)
			{
			if (token.Mid(0) == _L("quiet"))
				{
				gQuiet = ETrue;
				continue;
				}

			if (token.Mid(0) == _L("verbose"))
				{
				gQuiet = EFalse;
				continue;
				}

			if (token.Left(5) == _L("chunk"))
				{
				TInt equalPos;
				equalPos = token.Locate('=');
				if (equalPos > 0 && (equalPos+1) < token.Length())
					{
					TLex lexNum(token.Mid(equalPos+1));
					lexNum.Val(gChunkSize,EDecimal);	
					}
				continue;
				}

			if (token.Left(3) == _L("low"))
				{
				TInt equalPos;
				equalPos = token.Locate('=');
				if (equalPos > 0 && (equalPos+1) < token.Length())
					{
					TLex lexNum(token.Mid(equalPos+1));
					lexNum.Val(gMin,EDecimal);	
					}
				continue;
				}

			if (token.Left(5) == _L("high"))
				{
				TInt equalPos;
				equalPos = token.Locate('=');
				if (equalPos > 0 && (equalPos+1) < token.Length())
					{
					TLex lexNum(token.Mid(equalPos+1));
					lexNum.Val(gMax,EDecimal);	
					}
				continue;
				}

			if (token.Left(6) == _L("period"))
				{
				TInt equalPos;
				equalPos = token.Locate('=');
				if (equalPos > 0 && (equalPos+1) < token.Length())
					{
					TLex lexNum(token.Mid(equalPos+1));
					lexNum.Val(gPeriod,EDecimal);	
					}
				continue;
				}

			if (token.Left(3) == _L("mem"))
				{
				TInt equalPos;
				equalPos = token.Locate('=');
				if (equalPos > 0 && (equalPos+1) < token.Length())
					{
					TLex lexNum(token.Mid(equalPos+1));
					lexNum.Val(gMemScheme,EDecimal);	
					}
				continue;
				}

			}
		}
	}

enum TimerActions
	{
	ENoaction = 0,
	EFlush = 1,
	EFlushQuiet = 2,
	EMemScheme1 = 1 << 4,
	EMemScheme2 = 2 << 4,
	EMemScheme3 = 3 << 4,
	EMemScheme4 = 4 << 4,
	};

// CActive class to monitor KeyStrokes from User
class CActiveConsole : public CActive
	{
public:
	CActiveConsole();
	~CActiveConsole();
	void GetCharacter();
	static TInt Callback(TAny* aCtrl);

private:
	CPeriodic*	iTimer;
	TChar iCmdGetValue;
	TBool iGetHexValue;
	TBool iPrompt;
	TChar iLastChar;
	TUint iValue;
	TUint16 iActions;
	TUint32 iPeriod;
	
	// Defined as pure virtual by CActive;
	// implementation provided by this class.
	virtual void DoCancel();
	// Defined as pure virtual by CActive;
	// implementation provided by this class,
	virtual void RunL();
	void ProcessKeyPressL(TChar aChar);
	void ProcessValue();
	};

// Class CActiveConsole
CActiveConsole::CActiveConsole()
	: CActive(EPriorityHigh)
	{
	CActiveScheduler::Add(this);

    iTimer = CPeriodic::NewL(EPriorityNormal);
    iActions = ENoaction;
    iPrompt = ETrue;
    iPeriod = 0;
 	if (gPeriod > 0)
		{
		if (gMemScheme > 0)
			{
			iActions = (TUint16)(gMemScheme << 4);			
			}
		else
			{
			iActions = (TUint16)(gPeriod < KFlushQuietLimit ? EFlushQuiet : EFlush);
			}
		iPeriod = gPeriod;
	    iTimer->Start(0,gPeriod,TCallBack(Callback,this));
		}
	}

CActiveConsole::~CActiveConsole()
	{
    iTimer->Cancel();
    delete iTimer;

    Cancel();
	}

// Callback function for timer expiry
TInt CActiveConsole::Callback(TAny* aControl)
	{
	switch (((CActiveConsole*)aControl)->iActions & 0x0F)
		{
		case ENoaction :
			break;

		case EFlush :
			PRINT(_L("Flush\n"));
			// drop through to quiet 
			
		case EFlushQuiet :
			test_KErrNone(DPTest::FlushCache());
			break;
			
		default	:
			break;
		}

	switch (((CActiveConsole*)aControl)->iActions & 0xF0)
		{
		TUint i;
		case EMemScheme1 :
			for (i = 0; i < gNextChunk; i++)
				ReadChunk (&gChunk[i]);						
			break;

		case EMemScheme2 :
			for (i = 0; i < gNextChunk; i++)
				WriteChunk (&gChunk[i]);						
			break;

		default	:
			break;
		}
	
	return KErrNone;
	}

void CActiveConsole::GetCharacter()
	{
	if (iPrompt)
		{
		PRINT(_L("***Command (F,I,Q,V,?,Rn,Wn,Mn,C=nnnnn,H=nnnn,L=nnnn,P=nnnn) or Esc to exit ***\n"));
		iPrompt = EFalse;
		}
	test.Console()->Read(iStatus);
	SetActive();
	}

void CActiveConsole::DoCancel()
	{
	PRINT(_L("CActiveConsole::DoCancel\n"));
	test.Console()->ReadCancel();
	}

void CActiveConsole::ProcessKeyPressL(TChar aChar)
	{
	if (aChar == EKeyEscape)
		{
		PRINT(_L("CActiveConsole: ESC key pressed -> stopping active scheduler...\n"));
		CActiveScheduler::Stop();
		return;
		}
	aChar.UpperCase();
	if (iCmdGetValue != 0 && aChar == '\r')
		{
		if (iLastChar == 'K')
			{
			iValue *= iGetHexValue ? 0x400 : 1000;
			}
		if (iLastChar == 'M')
			{
			iValue *= iGetHexValue ? 0x10000 : 1000000;
			}
		PRINT1(_L("CActiveConsole: Value %d\n"),iValue);
		ProcessValue();
		}
	if (iCmdGetValue != 0 )
		{
		if (iGetHexValue)
			{
			if (aChar.IsDigit())
				{
				iValue = iValue * 16 + aChar.GetNumericValue();
			}
			else
				{
				if (aChar.IsHexDigit())
					{
					iValue = iValue * 16 + (TUint)aChar - 'A' + 10;
					}
				else
					{
						if (aChar != 'K' && aChar != 'M')
						{
						PRINT(_L("Illegal hexadecimal character - Enter command\n"));
						iCmdGetValue = 0;
						}
					}
				}
			}
		else
			{
			if (aChar.IsDigit())
				{
				iValue = iValue * 10 + aChar.GetNumericValue();
				}
			else
				{
				if ((aChar == 'X') && (iLastChar == '0') && (iValue == 0))
					iGetHexValue = ETrue;
				else
					{
					if (aChar != 'K' && aChar != 'M')
						{
						test.Printf(_L("Illegal decimal character - Enter command\n"));
						iCmdGetValue = 0;							
						}
					}
				}
			}
		}
	else
		{
		switch (aChar)
			{
			case 'F' :
				TESTNEXT(_L("Flushing Cache"));
				test_KErrNone(DPTest::FlushCache());
				ShowMemoryUse();
				iPrompt = ETrue;
				break;
				
			case 'I' :
				CacheSize(0,0);
				ShowMemoryUse();
				iPrompt = ETrue;
				break;
	
			case 'Q' :
				gQuiet = ETrue;
				iPrompt = ETrue;
				break;

			case 'V' :
				gQuiet = EFalse;
				iPrompt = ETrue;
				break;
				
			case '?' :
				ShowHelp();
				break;

			case '=' :
				iCmdGetValue = iLastChar;
				iGetHexValue = EFalse;
				iValue = 0;
				break;
						
			default :
				if (aChar.IsDigit())
					{
					if (iLastChar == 'R')
						{
						if (aChar.GetNumericValue() < (TInt)gNextChunk)
							{
							ReadChunk (&gChunk[aChar.GetNumericValue()]);			
							}
						else
							{
							for (TUint i = 0; i < gNextChunk; i++)
								ReadChunk (&gChunk[i]);			
							}
						iPrompt = ETrue;
						}				
					if (iLastChar == 'W')
						{
						if (aChar.GetNumericValue() < (TInt)gNextChunk)
							{
							WriteChunk (&gChunk[aChar.GetNumericValue()]);			
							}
						else
							{
							for (TUint i = 0; i < gNextChunk; i++)
								WriteChunk (&gChunk[i]);			
							}
						iPrompt = ETrue;
						}
					if (iLastChar == 'M')
						{
						if (aChar.GetNumericValue() == 0)
							{
							iActions = (TUint16)(iPeriod < KFlushQuietLimit ? EFlushQuiet : EFlush);							
							}
						else
							{
							iActions = (TUint16)(aChar.GetNumericValue() << 4);							
							}
						iPrompt = ETrue;
						}
					}
				break;
			}
		}
	iLastChar = aChar;
	GetCharacter();
	return;
	}

void CActiveConsole::ProcessValue()
	{
	switch (iCmdGetValue)
		{
		case 'C' :
			if (iValue > 0 && gNextChunk < MAX_CHUNKS)
				{
				CreateChunk (&gChunk[gNextChunk], iValue);
				ReadChunk (&gChunk[gNextChunk]);
				ShowMemoryUse();				
				gNextChunk++;
				}
			break;

		case 'H' :
			CacheSize (0,iValue);
			break;
			
		case 'L' :
			CacheSize (iValue,0);
			break;

		case 'P' :
			iPeriod = iValue;
			iActions = (TUint16)(iValue < KFlushQuietLimit ? EFlushQuiet : EFlush);
			iTimer->Cancel();
			if (iValue > 0)
				{
				iTimer->Start(0,iValue,TCallBack(Callback,this));
				}
			break;	

		default :
			break;
		}
	iCmdGetValue = 0;
	iPrompt = ETrue;
	}

void CActiveConsole::RunL()
	{
	ProcessKeyPressL(static_cast<TChar>(test.Console()->KeyCode()));
	}

TInt E32Main()
	{
	test.Title();
	test.Start(_L("Writable Data Paging Soak Test"));

	ParseCommandLine();
	
	if (DPTest::Attributes() & DPTest::ERomPaging)
		test.Printf(_L("Rom paging supported\n"));
	if (DPTest::Attributes() & DPTest::ECodePaging)
		test.Printf(_L("Code paging supported\n"));
	if (DPTest::Attributes() & DPTest::EDataPaging)
		test.Printf(_L("Data paging supported\n"));

	TInt totalRamSize;
	HAL::Get(HAL::EMemoryRAM,totalRamSize);
	HAL::Get(HAL::EMemoryPageSize,gPageSize);
	test.Printf(_L("Total RAM size 0x%08X bytes"),totalRamSize);
	test.Printf(_L("  Swap size 0x%08X bytes"),SwapSize());
	test.Printf(_L("  Page size 0x%08X bytes\n"),gPageSize);
	CacheSize(gMin,gMax);

	if ((DPTest::Attributes() & DPTest::EDataPaging) == 0)
		{
		test.Printf(_L("Writable Demand Paging not supported\n"));
		test.End();
		return 0;
		}

	ShowMemoryUse();

	//User::SetDebugMask(0x00000008);		//KMMU
	//User::SetDebugMask(0x00000080);		//KEXEC
	//User::SetDebugMask(0x90000000);		//KPANIC KMMU2
	//User::SetDebugMask(0x40000000, 1);	//KPAGING

	if (gChunkSize)
		{
		CreateChunk (&gChunk[gNextChunk], gChunkSize);
		ReadChunk (&gChunk[gNextChunk]);
		ShowMemoryUse();				
		gNextChunk++;
		}
	
	CActiveScheduler* myScheduler = new (ELeave) CActiveScheduler();
	CActiveScheduler::Install(myScheduler);

	CActiveConsole* myActiveConsole = new CActiveConsole();
	myActiveConsole->GetCharacter();

	CActiveScheduler::Start();
	
	test.End();

	return 0;
	}
