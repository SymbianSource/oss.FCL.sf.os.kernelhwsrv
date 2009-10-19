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
// e32test\device\t_loop.cpp
// 
//

//#define _DEBUG_DEVCOMM

#include <e32base.h>
#include <e32base_private.h>
#include <e32test.h>
#include <e32svr.h>
#include <d32comm.h>
#include <e32math.h>
#include <e32uid.h>

#if defined (__WINS__)
#define __COMM_LDD _L("ECOMM")
#define __COMM_PDD1 _L("ECDRV")
#else
#define __COMM_LDD _L("ECOMM")
#define __COMM_PDD1 _L("EUART1")
#define __COMM_PDD2 _L("EUART2")
#endif

const char KSpinner[]={'|','/','-','\\',};

const TInt KKeyboardPriority	= 4;
const TInt KTimerPriority		= 3;
const TInt KWriterPriority		= 2;
const TInt KReaderPriority		= 1;

////////////////////////////////////////////////////////////////////////////////

RTest Test(_L("T_LOOP"));

#define TEST(a) __DoTest((a), __FILE__, __LINE__, 0)
#define TESTERR(a,b) __DoTest((a), __FILE__, __LINE__, b)

void __DoTest(TBool aCondition, char* aFile, TInt aLine, TInt aErr)
	{
	if (aCondition)
		return;
		
	if (aErr==0)
		Test.Printf(_L("\r\nCheckpoint Fail at %s:%d\r\n"), aFile, aLine);
	else
		Test.Printf(_L("\r\nCheckpoint Fail at %s:%d: Return code = %d (0x%x)\r\n"), aFile, aLine, aErr, aErr);
	__DEBUGGER();
	Test.Getch();
	User::Exit(aErr);
	}

////////////////////////////////////////////////////////////////////////////////

class CDevCommTestBase;

// This needs:
// inline void Read(TRequestStatus &aStatus) { iConsole->Read(aStatus); }
// inline void ReadCancel() { iConsole->ReadCancel(); }
// inline void TKeyCode KeyCode() { return iConsole->KeyCode(); }
// adding to RTest

class CKeyReader : public CActive
	{
public:
	CKeyReader(CDevCommTestBase* aTester, RTest& aTest);
	~CKeyReader();
	void Start();
protected:
	void RunL();
	void DoCancel();
public:
	RTest& iTest;
	TKeyCode iKey;
	CDevCommTestBase* iTester;
	};

class CDevCommIOBase : public CActive
	{
public:
	CDevCommIOBase(CDevCommTestBase* aTester, TInt aPriority);
	~CDevCommIOBase();
	void CreateL(TInt aBufferSize);
	void UpdateCount();
	void ResetCount();
public:
	CDevCommTestBase* iTester;
	TPtr8 iDes;
	TUint8* iBuffer;
	TInt iSize;
	TInt iCount;
	TInt iTotal;
	};

class CDevCommWriterBase : public CDevCommIOBase
	{
public:
	CDevCommWriterBase(CDevCommTestBase* aTester);
	~CDevCommWriterBase();
	void Ready();
	void Start();
protected:
	void RunL();
	void DoCancel();
	};

class CDevCommReaderBase : public CDevCommIOBase
	{
public:
	CDevCommReaderBase(CDevCommTestBase* aTester);
	~CDevCommReaderBase();
	void Ready();
	void Start();
protected:
	void RunL();
	void DoCancel();
	};

class CDevCommTimerBase : public CTimer
	{
public:
	CDevCommTimerBase(CDevCommTestBase* aTester);
	void CreateL();
protected:
	void RunL();
public:
	CDevCommTestBase* iTester;
	};

enum THandshakeMode
	{
	EHandshakeNone,
	EHandshakeHardware,
	EHandshakeSoftware
	};

class CDevCommTestBase : public CAsyncOneShot
	{
public:
	CDevCommTestBase();
	~CDevCommTestBase();
	void CreateL(TInt aBufferSize);
	TInt Open(TInt aPort);
	void Close();
	void Debug();
	TInt SetHandshaking(THandshakeMode aMode);
	TInt LineFailOn();
	TInt LineFailOff();
	TInt ZeroTerminate();
	void ShowLoopSignals(TUint aOutState, TUint aInState);
	virtual void ReadComplete(TInt aStatus);
	virtual void WriteComplete(TInt aStatus);
	virtual void TimeComplete(TInt aStatus);
	virtual void KeyComplete(TKeyCode aKey);
	void Start();
public:
	RBusDevComm iComm;
	CDevCommWriterBase* iWriter;
	CDevCommReaderBase* iReader;
	CDevCommTimerBase* iTimer;
	CKeyReader* iKeyboard;
	TInt iBufferSize;
	};


CKeyReader::CKeyReader(CDevCommTestBase* aTester, RTest& aTest)
	: CActive(KKeyboardPriority), iTest(aTest), iTester(aTester)
	{
	__DECLARE_NAME(_S("CKeyReader"));
	CActiveScheduler::Add(this);
	}

CKeyReader::~CKeyReader()
	{
	Cancel();
	}

void CKeyReader::Start()
	{
	if (IsActive())
		return;
	SetActive();
	iTest.Console()->Read(iStatus);
	}

void CKeyReader::RunL()
	{
	iKey = iTest.Console()->KeyCode();
	iTester->KeyComplete(iKey);
	Start();
	}

void CKeyReader::DoCancel()
	{
	iTest.Console()->ReadCancel();
	}



CDevCommIOBase::CDevCommIOBase(CDevCommTestBase* aTester, TInt aPriority)
	: CActive(aPriority), iTester(aTester), iDes(NULL, 0)
	{
	__DECLARE_NAME(_S("CDevCommIOBase"));
	CActiveScheduler::Add(this);
	}

CDevCommIOBase::~CDevCommIOBase()
	{
	if (iBuffer)
		User::Free(iBuffer);
	}

void CDevCommIOBase::CreateL(TInt aSize)
	{
	iSize = aSize;
	if (iSize>0)
		iBuffer = (TUint8*)User::AllocL(iSize);
	iDes.Set(iBuffer, iSize, iSize);
	}

void CDevCommIOBase::UpdateCount()
	{
	iCount += iDes.Length();
	iTotal += iDes.Length();
	}

void CDevCommIOBase::ResetCount()
	{
	iCount = 0;
	}

CDevCommWriterBase::CDevCommWriterBase(CDevCommTestBase* aTester)
	: CDevCommIOBase(aTester, KWriterPriority)
	{
	__DECLARE_NAME(_S("CDevCommWriterBase"));
	}

CDevCommWriterBase::~CDevCommWriterBase()
	{
	Cancel();
	}

void CDevCommWriterBase::Start()
	{
	__ASSERT_ALWAYS(iBuffer!=NULL, User::Panic(_L("No Buffer"), 0));
	if (IsActive())
		return;
	SetActive();
	iTester->iComm.Write(iStatus, iDes);
	}

void CDevCommWriterBase::Ready()
	{
	if (IsActive())
		return;
	SetActive();
	iTester->iComm.Write(iStatus, TPtr8(NULL, 0));
	}

void CDevCommWriterBase::RunL()
	{
	UpdateCount();
	iTester->WriteComplete(iStatus.Int());
	}

void CDevCommWriterBase::DoCancel()
	{
	iTester->iComm.WriteCancel();
	}


CDevCommReaderBase::CDevCommReaderBase(CDevCommTestBase* aTester)
	: CDevCommIOBase(aTester, KReaderPriority)
	{
	__DECLARE_NAME(_S("CDevCommReaderBase"));
	}

CDevCommReaderBase::~CDevCommReaderBase()
	{
	Cancel();
	}

void CDevCommReaderBase::Start()
	{
	__ASSERT_ALWAYS(iBuffer!=NULL, User::Panic(_L("No Buffer"), 0));
	if (IsActive())
		return;
	SetActive();
	iDes.SetLength(iDes.MaxLength()-iCount);
	iTester->iComm.Read(iStatus, iDes);
	}

void CDevCommReaderBase::Ready()
	{
	if (IsActive())
		return;
	SetActive();
    TPtr8 ptr(NULL, 0);
    iTester->iComm.Read(iStatus, ptr);
	}

void CDevCommReaderBase::RunL()
	{
	UpdateCount();
	iTester->ReadComplete(iStatus.Int());
	}

void CDevCommReaderBase::DoCancel()
	{
	iTester->iComm.ReadCancel();
	}

CDevCommTimerBase::CDevCommTimerBase(CDevCommTestBase* aTester)
	: CTimer(KTimerPriority), iTester(aTester)
	{
	__DECLARE_NAME(_S("CDevCommTestTimerBase"));
	CActiveScheduler::Add(this);
	}

void CDevCommTimerBase::CreateL()
	{
	ConstructL();
	}

void CDevCommTimerBase::RunL()
	{
	iTester->TimeComplete(iStatus.Int());
	}

CDevCommTestBase::CDevCommTestBase()
	: CAsyncOneShot(-1)
	{
	__DECLARE_NAME(_S("CDevCommTestBase"));
	}

CDevCommTestBase::~CDevCommTestBase()
	{
	delete iKeyboard;
	delete iTimer;
	delete iWriter;
	delete iReader;
	iComm.Close();
	}

void CDevCommTestBase::CreateL(TInt aBufferSize)
	{
	iBufferSize = aBufferSize;
	iKeyboard = new (ELeave) CKeyReader(this, Test);
	iTimer = new (ELeave) CDevCommTimerBase(this);
	iWriter = new (ELeave) CDevCommWriterBase(this);
	iReader = new (ELeave) CDevCommReaderBase(this);
	iKeyboard->Start();
	iWriter->CreateL(iBufferSize);
	iTimer->CreateL();
	iReader->CreateL(iBufferSize/16);
	}

void CDevCommTestBase::Start()
	{
	Call();
	}

TInt CDevCommTestBase::Open(TInt aPort)
	{
	TInt err;
    if (err = iComm.Open(aPort), err!=KErrNone)
		return err;

    TCommConfig cBuf;
	TCommConfigV01 &c=cBuf();
	iComm.Config(cBuf);

    c.iStopBits = EStop1;
    c.iDataBits = EData8;
    c.iParity = EParityNone;
	c.iHandshake = 0
//		| KConfigObeyXoff
//		| KConfigSendXoff
		| KConfigObeyCTS
//		| KConfigFailCTS
		| KConfigObeyDSR
//		| KConfigFailDSR
//		| KConfigObeyDCD
//		| KConfigFailDCD
//		| KConfigFreeRTS
//		| KConfigFreeDTR
		;
    c.iRate = EBps115200;
    c.iFifo = EFifoEnable;
	c.iTerminatorCount = 0;
	c.iTerminator[0] = 0x00;

    if (err = iComm.SetConfig(cBuf), err!=KErrNone)
		{
		iComm.Close();
		return err;
		}
	return KErrNone;
	}


TInt CDevCommTestBase::ZeroTerminate()
	{
	TCommConfig cBuf;
	TCommConfigV01 &c=cBuf();
	iComm.Config(cBuf);

	c.iTerminatorCount = 1;
	c.iTerminator[0] = 0x00;

    return iComm.SetConfig(cBuf);
	}

void CDevCommTestBase::Close()
	{
	iTimer->Cancel();
	iReader->Cancel();
	iWriter->Cancel();
	iComm.Close();
	}

TInt CDevCommTestBase::SetHandshaking(THandshakeMode aMode)
	{
    TCommConfig cBuf;
	TCommConfigV01 &c=cBuf();
	iComm.Config(cBuf);
	
	switch (aMode)
		{
	case EHandshakeNone:
	c.iHandshake = 0
//		| KConfigObeyXoff
//		| KConfigSendXoff
//		| KConfigObeyCTS
//		| KConfigFailCTS
//		| KConfigObeyDSR
//		| KConfigFailDSR
//		| KConfigObeyDCD
//		| KConfigFailDCD
		| KConfigFreeRTS
		| KConfigFreeDTR
		;
		break;
	case EHandshakeSoftware:
	c.iXonChar=0x11;
	c.iXoffChar=0x13;
	c.iHandshake = 0
		| KConfigObeyXoff
		| KConfigSendXoff
//		| KConfigObeyCTS
//		| KConfigFailCTS
//		| KConfigObeyDSR
//		| KConfigFailDSR
//		| KConfigObeyDCD
//		| KConfigFailDCD
		| KConfigFreeRTS
		| KConfigFreeDTR
		;
		break;
	case EHandshakeHardware:
	c.iHandshake = 0
//		| KConfigObeyXoff
//		| KConfigSendXoff
		| KConfigObeyCTS
//		| KConfigFailCTS
		| KConfigObeyDSR
//		| KConfigFailDSR
//		| KConfigObeyDCD
//		| KConfigFailDCD
//		| KConfigFreeRTS
//		| KConfigFreeDTR
		;
		break;
		}
    return iComm.SetConfig(cBuf);
	}

TInt CDevCommTestBase::LineFailOn()
	{
    TCommConfig cBuf;
	TCommConfigV01 &c=cBuf();
	iComm.Config(cBuf);
	c.iHandshake |= (KConfigFailDSR|KConfigFailDCD);
    return iComm.SetConfig(cBuf);
	}

TInt CDevCommTestBase::LineFailOff()
	{
    TCommConfig cBuf;
	TCommConfigV01 &c=cBuf();
	iComm.Config(cBuf);
	c.iHandshake &= ~(KConfigFailDSR|KConfigFailDCD);
    return iComm.SetConfig(cBuf);
	}

void CDevCommTestBase::ShowLoopSignals(TUint aOutState, TUint aInState)
	{
	TPtrC cts, dsr, dcd;
	TPtrC rts, dtr;
	rts.Set(aOutState & KSignalRTS ? _L("RTS On ") : _L("RTS Off"));
	dtr.Set(aOutState & KSignalDTR ? _L("DTR On ") : _L("DTR Off"));
	Test.Printf(_L("%S, %S : "), &rts,  &dtr);
	cts.Set(aInState & KSignalCTS ? _L("CTS On ") : _L("CTS Off"));
	dsr.Set(aInState & KSignalDSR ? _L("DSR On ") : _L("DSR Off"));
	dcd.Set(aInState & KSignalDCD ? _L("DCD On ") : _L("DCD Off"));
	Test.Printf(_L("%S, %S, %S "), &cts, &dsr, &dcd);
	rts.Set(aInState & KSignalRTS ? _L("RTS On ") : _L("RTS Off"));
	dtr.Set(aInState & KSignalDTR ? _L("DTR On ") : _L("DTR Off"));
	Test.Printf(_L("[%S, %S]\r\n"), &rts,  &dtr);
	}

#ifdef _DEBUG_DEVCOMM
void CDevCommTestBase::Debug()
	{
	TCommDebugInfoPckg infopckg;
	TCommDebugInfo& info = infopckg();
	iComm.DebugInfo(infopckg);

	Test.Printf(_L("  LDD State        :    TX         RX    \r\n"));
	Test.Printf(_L("  Busy             : %10d %10d\r\n"), info.iTxBusy, info.iRxBusy);
	Test.Printf(_L("  Held             : %10d %10d\r\n"), info.iTxHeld, info.iRxHeld);
	Test.Printf(_L("  Length           : %10d %10d\r\n"), info.iTxLength, info.iRxLength);
	Test.Printf(_L("  Offset           : %10d %10d\r\n"), info.iTxOffset, info.iRxOffset);
	Test.Printf(_L("  Int Count        : %10d %10d\r\n"), info.iTxIntCount, info.iRxIntCount);
	Test.Printf(_L("  Err Count        : %10d %10d\r\n"), info.iTxErrCount, info.iRxErrCount);
	Test.Printf(_L("  Buf Count        : %10d %10d\r\n"), info.iTxBufCount, info.iRxBufCount);
	Test.Printf(_L("  Fill/Drain       : %10d %10d\r\n"), info.iFillingTxBuf, info.iFillingTxBuf);
	Test.Printf(_L("  XON              : %10d %10d\r\n"), info.iTxXon, info.iRxXon);
	Test.Printf(_L("  XOFF             : %10d %10d\r\n"), info.iTxXoff, info.iRxXoff);
	Test.Printf(_L("  Chars            : %10d %10d\r\n"), info.iTxChars, info.iRxChars);
//	Test.Printf(_L("  DFC Pending      : %10d %10d\r\n"), info.iTxDfcPend, info.iTxDfcPend);
//	Test.Printf(_L("  DFC Run/Count    : %10d %10d\r\n"), info.iRunningDfc, info.iDfcCount);
//	Test.Printf(_L("  DFC Req/Do/Drain : %10d %10d %10d\r\n"), info.iDfcReqSeq, info.iDfcHandlerSeq, info.iDoDrainSeq);
	}
#else
void CDevCommTestBase::Debug()
	{
	Test.Printf(_L("Debug Dump not available\r\n"));
	}
#endif

void CDevCommTestBase::ReadComplete(TInt /*aStatus*/)
	{}

void CDevCommTestBase::WriteComplete(TInt /*aStatus*/)
	{}

void CDevCommTestBase::TimeComplete(TInt /*aStatus*/)
	{}

void CDevCommTestBase::KeyComplete(TKeyCode /*aKey*/)
	{}

////////////////////////////////////////////////////////////////////////////////

void StripeMem(TUint8 *aBuf, TInt aStartPos, TInt anEndPos, TUint aStartChar, TUint anEndChar, TInt aOffset=0)
//
// Mark a buffer with repeating byte pattern
//
	{
	TUint character=aStartChar+(aOffset%((anEndChar+1)-aStartChar));

	for (TInt i=aStartPos;i<anEndPos;i++)
		{
		aBuf[i]=(TText8)character;
		if(++character>anEndChar)
			character=aStartChar;
		}
	}

inline void StripeDes(TDes8 &aBuf, TInt aStartPos, TInt anEndPos, TUint aStartChar, TUint anEndChar, TInt aOffset=0)
	{
	StripeMem((TUint8 *)aBuf.Ptr(), aStartPos, anEndPos, aStartChar, anEndChar, aOffset);
	}

TBool CheckMem(TUint8 *aBuf, TInt aStartPos, TInt anEndPos, TUint aStartChar, TUint anEndChar, TInt aOffset=0)
//
// Mark a buffer with repeating byte pattern
//
	{
	TUint character=aStartChar+(aOffset%((anEndChar+1)-aStartChar));

	for (TInt i=aStartPos;i<anEndPos;i++)
		{
		if (aBuf[i]!=(TText8)character)
			return EFalse;
		if(++character>anEndChar)
			character=aStartChar;
		}
	return ETrue;
	}

inline TBool CheckDes(TDes8 &aBuf, TInt aStartPos, TInt anEndPos, TUint aStartChar, TUint anEndChar, TInt aOffset=0)
	{
	return CheckMem((TUint8 *)aBuf.Ptr(), aStartPos, anEndPos, aStartChar, anEndChar, aOffset);
	}

////////////////////////////////////////////////////////////////////////////////

void CommStart()
	{
	TInt ret;
	Test.Printf(_L("Loading Drivers\r\n"));
 	ret = User::LoadPhysicalDevice(__COMM_PDD1);
	TESTERR(ret==KErrNone || ret==KErrAlreadyExists, ret);
// 	ret = User::LoadPhysicalDevice(__COMM_PDD2);
//	TESTERR(ret==KErrNone || ret==KErrAlreadyExists, ret);
	ret = User::LoadLogicalDevice(__COMM_LDD);
	TESTERR(ret==KErrNone || ret==KErrAlreadyExists, ret);
	Test.Printf(_L("OK\r\n"));
	}

////////////////////////////////////////////////////////////////////////////////
	
class CTestRandTerm : public CDevCommTestBase
	{
public:
	enum TTestRandTermState	{ EIdle, EWaitReady, EWaitReset, EWaitIO };
	enum TTestFailType { ETestFailBoth, ETestFailRead, ETestFailWrite, ETestBadData };
public:
	static CTestRandTerm* NewL(TInt aPort);
	CTestRandTerm();
	~CTestRandTerm();
	virtual void ReadComplete(TInt aStatus);
	virtual void WriteComplete(TInt aStatus);
	virtual void TimeComplete(TInt aStatus);
	virtual void KeyComplete(TKeyCode aKey);
	void Reset();
	void Write();
	void Read();
	TBool CheckRead();
	void Halt();
	void Fail(TTestFailType aType, TInt aError);
protected:
	virtual void RunL();
public:
	TTestRandTermState iState;
	TInt64 iSeed;
	TInt iCount;
	TInt iOffset;
	TInt iRetries;
	TInt iPackets;
	TInt iSpin;
	TBool iTrace;
	};

CTestRandTerm::CTestRandTerm()
	{
	}

CTestRandTerm::~CTestRandTerm()
	{
	}

CTestRandTerm* CTestRandTerm::NewL(TInt aPort)
	{
	CTestRandTerm* tester = new (ELeave) CTestRandTerm;
	CleanupStack::PushL(tester);
	tester->CreateL(1000);
	User::LeaveIfError(tester->Open(aPort));
	CleanupStack::Pop();
	return tester;
	}

void CTestRandTerm::Reset()
	{
    Test.Printf(_L("Resetting Port\r\n"));
	iReader->Cancel();
	iWriter->Cancel();
	iTimer->Cancel();
	LineFailOff();
	iComm.ResetBuffers();
	iTimer->After(1000000);
	iState = EWaitReset;
	}

void CTestRandTerm::RunL()
	{
	iCount = 0;
	iState = EIdle;
	iSeed = 1;
    Test.Printf(_L("Waiting for Port\r\n"));
	ZeroTerminate();
	iWriter->Ready();
	iTimer->After(1000000);
	iState = EWaitReady;
	}

void CTestRandTerm::ReadComplete(TInt aStatus)
	{
	if (iTrace)
		Test.Printf(_L("CTestRandTerm::ReadComplete(%d) len = %d/%d\r\n"), aStatus, iWriter->iDes.Length(), iWriter->iDes.MaxLength());

	if (aStatus!=KErrNone)
		{
		Fail(ETestFailRead, aStatus);
		return;
		}

	switch (iState)
		{
	case EWaitIO:
		iRetries = 0;
		iTimer->Cancel();
		if (CheckRead())
			{
			iPackets++;
			if (iReader->iCount==iWriter->iCount)
				{
				iCount += iWriter->iCount;
				Test.Printf(_L("%c %6d %d\r"), KSpinner[iSpin++ & 3], iPackets, iCount);
				Write();
				Read();
				}
			else
				{
				iOffset = iReader->iCount;
				Test.Printf(_L("%c\r"), KSpinner[iSpin++ & 3]);
				Read();
				}
			}
		else
			{
			Fail(ETestBadData, KErrNone);
			}
		break;
	default:
		break;
		}
	}

void CTestRandTerm::WriteComplete(TInt aStatus)
	{
	if (iTrace)
		{
		Test.Printf(_L("CTestRandTerm::WriteComplete(%d) len = %d/%d\r\n"), aStatus, iWriter->iDes.Length(), iWriter->iDes.MaxLength());
		}

	if (aStatus!=KErrNone)
		{
		Fail(ETestFailWrite, aStatus);
		return;
		}

	switch (iState)
		{
	case EWaitReady:
		iRetries = 0;
		iTimer->Cancel();
		iState = EWaitIO;
		Test.Printf(_L("Port Ready\r\n"));
		LineFailOn();
		Write();
		Read();
		break;
	case EWaitIO:
		iRetries = 0;
		if (iReader->iCount==iWriter->iCount)
			{
			Write();
			Read();
			}
		break;
	default:
		break;
		}
	}

void CTestRandTerm::TimeComplete(TInt aStatus)
	{
	if (iTrace)
		Test.Printf(_L("CTestRandTerm::TimeComplete(%d)\r\n"), aStatus);

	if (aStatus!=KErrNone)
		{
		__DEBUGGER();
		return;
		}

	switch (iState)
		{
	case EWaitReset:
	    Test.Printf(_L("Waiting for Port\r\n"));
		iWriter->Ready();
		iTimer->After(1000000);
		iState = EWaitReady;
		break;
	case EWaitReady:
		if (++iRetries>10)
			{
			Test.Printf(_L("Too many retries\r\n"));
			Halt();
			}
		else
			{
			Test.Printf(_L("%c\r"), KSpinner[iSpin++ & 3]);
			iWriter->Ready();
			iTimer->After(1000000);
			}
		break;
	case EWaitIO:
		Fail(ETestFailBoth, KErrTimedOut);
		break;
	default:
		Reset();
		break;
		}
	}


void CTestRandTerm::KeyComplete(TKeyCode aKey)
	{
	if (iTrace)
		Test.Printf(_L("CTestRandTerm::KeyComplete(%d)\r\n"), aKey);

	switch ((TInt)aKey)
		{
	case EKeyEscape:
		Halt();
		break;
	case 'd':
	case 'D':
		Debug();
		break;
	case 'q':
	case 'Q':
		iTrace = 0;
		break;
	case 'v':
	case 'V':
		iTrace = 1;
		break;
    default:
        break;
        }
	}


void CTestRandTerm::Fail(TTestFailType aType, TInt aError)
	{
	switch (aType)
		{
	case ETestFailBoth:
		Test.Printf(_L("Timeout at offset %d\r\n"), iOffset);
		break;
	case ETestFailRead:
		Test.Printf(_L("Read fail (%d) at offset %d\r\n"), aError, iOffset);
		break;
	case ETestFailWrite:
		Test.Printf(_L("Write fail (%d) at offset %d\r\n"), aError, iOffset);
		break;
	case ETestBadData:
		Test.Printf(_L("Data verify failure at offset %d\r\n"), iOffset);
		break;
		}
	Debug();
	Reset();
	}


void CTestRandTerm::Write()
	{
	iOffset = 0;
	iWriter->ResetCount();
	iReader->ResetCount();

	TInt i;
	TInt j = 0;
	StripeDes(iWriter->iDes, 0, iBufferSize, '@', 'Z');
	while (j<iBufferSize)
		{
		i = Math::Rand(iSeed) % (iBufferSize/4);
		if (j+i<iBufferSize)
			iWriter->iDes[j+i] = '\0';
		j += i;
		}

	iWriter->Start();
	}


void CTestRandTerm::Read()
	{
	iReader->Start();
	iTimer->After(5000000);
	}


TBool CTestRandTerm::CheckRead()
	{
	TPtrC8 ref;
	ref.Set(iWriter->iDes.Ptr()+iOffset, iReader->iDes.Length());
	return ref.Compare(iReader->iDes)==0;
	}

void CTestRandTerm::Halt()
	{
	iReader->Cancel();
	iWriter->Cancel();
	iTimer->Cancel();
	CActiveScheduler::Stop();
	}

////////////////////////////////////////////////////////////////////////////////
	

class CTestSignals : public CDevCommTestBase
	{
public:
	enum TTestState	{ EAllOff, ERtsOn, EDtrOn, EAllOn, EMonitor };
public:
	static CTestSignals* NewL(TInt aPort);
	CTestSignals();
	~CTestSignals();
	virtual void KeyComplete(TKeyCode aKey);
	virtual void ReadComplete(TInt aStatus);
	void Halt();
	void DoSignals(TTestState aState);
protected:
	virtual void RunL();
public:
	TTestState iState;
	};

CTestSignals::CTestSignals()
	{
	}

CTestSignals::~CTestSignals()
	{
	}

CTestSignals* CTestSignals::NewL(TInt aPort)
	{
	CTestSignals* tester = new (ELeave) CTestSignals;
	CleanupStack::PushL(tester);
	tester->CreateL(0);
	User::LeaveIfError(tester->Open(aPort));
	CleanupStack::Pop();
	return tester;
	}

void CTestSignals::RunL()
	{
    TCommConfig cBuf;
	TCommConfigV01 &c=cBuf();
	iComm.Config(cBuf);
	c.iHandshake = KConfigFreeRTS | KConfigFreeDTR;
    iComm.SetConfig(cBuf);
	iReader->Ready();
	}


void CTestSignals::Halt()
	{
	Test.Printf(_L("                        \r"));
	CActiveScheduler::Stop();
	}

void CTestSignals::ReadComplete(TInt /*aStatus*/)
	{
	DoSignals(EAllOff);
	}

void CTestSignals::KeyComplete(TKeyCode aKey)
	{
	switch (aKey)
		{
	case EKeyEscape:
		Halt();
		break;
	default:
		switch (iState)
			{
		case EAllOff:
			DoSignals(ERtsOn);
			break;
		case ERtsOn:
			DoSignals(EDtrOn);
			break;
		case EDtrOn:
			DoSignals(EAllOn);
			break;
		case EAllOn:
			DoSignals(EAllOff);
			break;
		default:
			break;
			}
		}
	}

void CTestSignals::DoSignals(TTestState aState)
	{
	TUint set=0, clr=0;

	switch (aState)
		{
	case EAllOff:
		set = 0;
		clr = KSignalRTS | KSignalDTR;
		break;
	case ERtsOn:
		set = KSignalRTS;
		clr = KSignalDTR;
		break;
	case EDtrOn:
		set = KSignalDTR;
		clr = KSignalRTS;
		break;
	case EAllOn:
		set = KSignalRTS | KSignalDTR;
		clr = 0;
		break;
	default:
		set = 0;
		clr = 0;
		}
	iComm.SetSignals(set, clr);
	TUint sig = iComm.Signals();
	ShowLoopSignals(set, sig);
	iState = aState;	
	Test.Printf(_L("Press key for next state\r"));
	}

////////////////////////////////////////////////////////////////////////////////

class CTestPerf : public CDevCommTestBase
	{
public:
	enum TTestRandTermState	{ EIdle, EWaitReady, EWaitReset, EWaitIO };
	enum TTestFailType { ETestFailBoth, ETestFailRead, ETestFailWrite, ETestBadData };
public:
	static CTestPerf* NewL(TInt aPort);
	CTestPerf();
	~CTestPerf();
	virtual void ReadComplete(TInt aStatus);
	virtual void WriteComplete(TInt aStatus);
	virtual void TimeComplete(TInt aStatus);
	virtual void KeyComplete(TKeyCode aKey);
	void Reset();
	void Write();
	void Read();
	TBool CheckRead();
	void Halt();
	void Fail(TTestFailType aType, TInt aError);
protected:
	virtual void RunL();
public:
	TTestRandTermState iState;
	TInt64 iSeed;
	TInt iCount;
	TInt iOffset;
	TInt iRetries;
	TInt iPackets;
	TInt iSpin;
	TBool iTrace;
	TTime iStartTime;
	TInt iRate;
	TInt iSpeed;
	};

CTestPerf::CTestPerf()
	{
	}

CTestPerf::~CTestPerf()
	{
	}

CTestPerf* CTestPerf::NewL(TInt aPort)
	{
	CTestPerf* tester = new (ELeave) CTestPerf;
	CleanupStack::PushL(tester);
	tester->CreateL(250);
	User::LeaveIfError(tester->Open(aPort));
	CleanupStack::Pop();
	StripeDes(tester->iWriter->iDes, 0, tester->iBufferSize, '@', 'Z');
	return tester;
	}

void CTestPerf::Reset()
	{
    Test.Printf(_L("Resetting Port\r\n"));
	iReader->Cancel();
	iWriter->Cancel();
	iTimer->Cancel();
	LineFailOff();
	iComm.ResetBuffers();
	iTimer->After(1000000);
	iState = EWaitReset;
	}

void CTestPerf::RunL()
	{
	iCount = 0;
	iState = EIdle;
	iSeed = 1;
    Test.Printf(_L("Waiting for Port\r\n"));
	ZeroTerminate();
	iWriter->Ready();
	iTimer->After(1000000);
	iState = EWaitReady;
	}

void CTestPerf::ReadComplete(TInt aStatus)
	{
	if (iTrace)
		Test.Printf(_L("CTestPerf::ReadComplete(%d) len = %d/%d\r\n"), aStatus, iWriter->iDes.Length(), iWriter->iDes.MaxLength());

	if (aStatus!=KErrNone)
		{
		Fail(ETestFailRead, aStatus);
		return;
		}

	switch (iState)
		{
	case EWaitIO:
		iRetries = 0;
		iTimer->Cancel();
		iCount += iReader->iCount;
		iPackets++;
		{
		TTime end;
		end.UniversalTime();
		TInt64 difftime = (end.MicroSecondsFrom(iStartTime).Int64()+TInt64(500000))/TInt64(1000000);
		if (difftime==0)
			difftime = 1;
		TInt64 cps = MAKE_TINT64(0,iCount)/difftime;
		TInt rate = (I64INT(cps)*10000)/11520;

		iRate += rate;
		iSpeed += I64INT(cps);
		
		Test.Printf(_L("%c %6d %d (%dbps=%d.%02d%%)\r"), KSpinner[iSpin++ & 3], iPackets, iCount, iSpeed/iPackets, (iRate/iPackets)/100, (iRate/iPackets)%100);
		}
		Read();
		break;
	default:
		break;
		}
	}

void CTestPerf::WriteComplete(TInt aStatus)
	{
	if (iTrace)
		{
		Test.Printf(_L("CTestPerf::WriteComplete(%d) len = %d/%d\r\n"), aStatus, iWriter->iDes.Length(), iWriter->iDes.MaxLength());
		}

	if (aStatus!=KErrNone)
		{
		Fail(ETestFailWrite, aStatus);
		return;
		}

	switch (iState)
		{
	case EWaitReady:
		iRetries = 0;
		iTimer->Cancel();
		iState = EWaitIO;
		Test.Printf(_L("Port Ready\r\n"));
		LineFailOn();
		iStartTime.UniversalTime();;
		Write();
		Read();
		break;
	case EWaitIO:
		iRetries = 0;
		Write();
		break;
	default:
		break;
		}
	}

void CTestPerf::TimeComplete(TInt aStatus)
	{
	if (iTrace)
		Test.Printf(_L("CTestPerf::TimeComplete(%d)\r\n"), aStatus);

	if (aStatus!=KErrNone)
		{
		__DEBUGGER();
		return;
		}

	switch (iState)
		{
	case EWaitReset:
	    Test.Printf(_L("Waiting for Port\r\n"));
		iWriter->Ready();
		iTimer->After(1000000);
		iState = EWaitReady;
		break;
	case EWaitReady:
		if (++iRetries>10)
			{
			Test.Printf(_L("Too many retries\r\n"));
			Halt();
			}
		else
			{
			Test.Printf(_L("%c\r"), KSpinner[iSpin++ & 3]);
			iWriter->Ready();
			iTimer->After(1000000);
			}
		break;
	case EWaitIO:
		Fail(ETestFailBoth, KErrTimedOut);
		break;
	default:
		Reset();
		break;
		}
	}


void CTestPerf::KeyComplete(TKeyCode aKey)
	{
	if (iTrace)
		Test.Printf(_L("CTestPerf::KeyComplete(%d)\r\n"), aKey);

	switch ((TInt)aKey)
		{
	case EKeyEscape:
		Halt();
		break;
	case 'd':
	case 'D':
		Test.Printf(_L("\r\n"));
		Debug();
		break;
	case 'q':
	case 'Q':
		iTrace = 0;
		break;
	case 'v':
	case 'V':
		iTrace = 1;
		break;
	case 's':
	case 'S':
		Test.Printf(_L("\r\n"));
		Test.Printf(_L("Keyboard : %08x, %d\r\n"), iKeyboard->iStatus.Int(), iKeyboard->IsActive());
		Test.Printf(_L("Timer    : %08x, %d\r\n"), iTimer->iStatus.Int(), iTimer->IsActive());
		Test.Printf(_L("Reader   : %08x, %d\r\n"), iReader->iStatus.Int(), iReader->IsActive());
		Test.Printf(_L("Writer   : %08x, %d\r\n"), iWriter->iStatus.Int(), iWriter->IsActive());
		break;
    default:
        break;
        }
	}


void CTestPerf::Fail(TTestFailType aType, TInt aError)
	{	
	switch (aType)
		{
	case ETestFailBoth:
		Test.Printf(_L("\r\nTimeout at offset %d\r\n"), iOffset);
		break;
	case ETestFailRead:
		Test.Printf(_L("\r\nRead fail (%d) at offset %d\r\n"), aError, iOffset);
		break;
	case ETestFailWrite:
		Test.Printf(_L("\r\nWrite fail (%d) at offset %d\r\n"), aError, iOffset);
		break;
	case ETestBadData:
		Test.Printf(_L("\r\nData verify failure at offset %d\r\n"), iOffset);
		break;
		}
	Debug();
	Reset();
	}


void CTestPerf::Write()
	{
	iOffset = 0;
	iWriter->ResetCount();
	iWriter->Start();
	}

void CTestPerf::Read()
	{
	iReader->ResetCount();
	iReader->Start();
	iTimer->After(5000000);
	}

TBool CTestPerf::CheckRead()
	{
	TPtrC8 ref;
	ref.Set(iWriter->iDes.Ptr()+iOffset, iReader->iDes.Length());
	return ref.Compare(iReader->iDes)==0;
	}

void CTestPerf::Halt()
	{
	iReader->Cancel();
	iWriter->Cancel();
	iTimer->Cancel();
	CActiveScheduler::Stop();
	}

////////////////////////////////////////////////////////////////////////////////

class CTestXonXoff : public CDevCommTestBase
	{
public:
	enum TTestRandTermState	{ EIdle, EWaitReady, EWaitReset, EWaitIO };
	enum TTestFailType { ETestFailBoth, ETestFailRead, ETestFailWrite, ETestBadData };
public:
	static CTestXonXoff* NewL(TInt aPort);
	CTestXonXoff();
	~CTestXonXoff();
	virtual void ReadComplete(TInt aStatus);
	virtual void WriteComplete(TInt aStatus);
	virtual void TimeComplete(TInt aStatus);
	virtual void KeyComplete(TKeyCode aKey);
	void Reset();
	void Write();
	void Read();
	TBool CheckRead();
	void Halt();
	void Fail(TTestFailType aType, TInt aError);
protected:
	virtual void RunL();
public:
	TTestRandTermState iState;
	TInt64 iSeed;
	TInt iCount;
	TInt iOffset;
	TInt iRetries;
	TInt iPackets;
	TInt iSpin;
	TBool iTrace;
	TTime iStartTime;
	TInt iRate;
	TInt iSpeed;
	};

CTestXonXoff::CTestXonXoff()
	{
	}

CTestXonXoff::~CTestXonXoff()
	{
	}

CTestXonXoff* CTestXonXoff::NewL(TInt aPort)
	{
	CTestXonXoff* tester = new (ELeave) CTestXonXoff;
	CleanupStack::PushL(tester);
	tester->CreateL(16384);
	User::LeaveIfError(tester->Open(aPort));
	User::LeaveIfError(tester->SetHandshaking(EHandshakeSoftware));
	CleanupStack::Pop();
	StripeDes(tester->iWriter->iDes, 0, tester->iBufferSize, '@', 'Z');
	return tester;
	}

void CTestXonXoff::Reset()
	{
    Test.Printf(_L("Resetting Port\r\n"));
	iReader->Cancel();
	iWriter->Cancel();
	iTimer->Cancel();
	LineFailOff();
	iComm.ResetBuffers();
	iTimer->After(1000000);
	iState = EWaitReset;
	}

void CTestXonXoff::RunL()
	{
	iCount = 0;
	iState = EIdle;
	iSeed = 1;
    Test.Printf(_L("Waiting for Port\r\n"));
	ZeroTerminate();

	iWriter->Ready();
	iTimer->After(1000000);
	
//	iState = EWaitReady;
//	WriteComplete(0);
	}

void CTestXonXoff::ReadComplete(TInt aStatus)
	{
	if (iTrace)
		Test.Printf(_L("CTestXonXoff::ReadComplete(%d) len = %d/%d (%d)\r\n"), aStatus, iReader->iDes.Length(), iReader->iDes.MaxLength(), iReader->iTotal);

	if (aStatus!=KErrNone)
		{
		Fail(ETestFailRead, aStatus);
		return;
		}

	switch (iState)
		{
	case EWaitIO:
		iRetries = 0;
		iTimer->Cancel();
		if (!CheckDes(iReader->iDes, 0, iReader->iDes.Length(), '@', 'Z', iCount & 0x3fff))
			{
			Fail(ETestBadData, aStatus);
			return;
			}
		iCount += iReader->iCount;
		iPackets++;
		{
		TTime end;
		end.UniversalTime();
		TInt64 difftime = (end.MicroSecondsFrom(iStartTime).Int64()+TInt64(500000))/TInt64(1000000);
		if (difftime==0)
			difftime = 1;
		TInt64 cps = MAKE_TINT64(0,iCount)/difftime;
		TInt rate = (I64INT(cps)*10000)/11520;
		iRate += rate;
		iSpeed += I64INT(cps);
		Test.Printf(_L("%c %6d %d (%dbps=%d.%02d%%)\r"), KSpinner[iSpin++ & 3], iPackets, iCount, iSpeed/iPackets, (iRate/iPackets)/100, (iRate/iPackets)%100);
		}
		Read();
		break;
	default:
		break;
		}
	}

void CTestXonXoff::WriteComplete(TInt aStatus)
	{
	if (iTrace)
		{
		Test.Printf(_L("CTestXonXoff::WriteComplete(%d) len = %d/%d (%d)\r\n"), aStatus, iWriter->iDes.Length(), iWriter->iDes.MaxLength(), iWriter->iTotal);
		}

	if (aStatus!=KErrNone)
		{
		Fail(ETestFailWrite, aStatus);
		return;
		}

	switch (iState)
		{
	case EWaitReady:
		iRetries = 0;
		iTimer->Cancel();
		iState = EWaitIO;
		Test.Printf(_L("Port Ready\r\n"));
		LineFailOn();
		iStartTime.UniversalTime();;
		Write();
		Read();
		break;
	case EWaitIO:
		iRetries = 0;
		Write();
		break;
	default:
		break;
		}
	}

void CTestXonXoff::TimeComplete(TInt aStatus)
	{
	if (iTrace)
		Test.Printf(_L("CTestXonXoff::TimeComplete(%d)\r\n"), aStatus);

	if (aStatus!=KErrNone)
		{
		__DEBUGGER();
		return;
		}

	switch (iState)
		{
	case EWaitReset:
	    Test.Printf(_L("Waiting for Port\r\n"));
		iWriter->Ready();
		iTimer->After(1000000);
		iState = EWaitReady;
		break;
	case EWaitReady:
		if (++iRetries>10)
			{
			Test.Printf(_L("Too many retries\r\n"));
			Halt();
			}
		else
			{
			Test.Printf(_L("%c\r"), KSpinner[iSpin++ & 3]);
			iWriter->Ready();
			iTimer->After(1000000);
			}
		break;
	case EWaitIO:
		Fail(ETestFailBoth, KErrTimedOut);
		break;
	default:
		Reset();
		break;
		}
	}


void CTestXonXoff::KeyComplete(TKeyCode aKey)
	{
	if (iTrace)
		Test.Printf(_L("CTestXonXoff::KeyComplete(%d)\r\n"), aKey);

	switch ((TInt)aKey)
		{
	case EKeyEscape:
		Halt();
		break;
	case 'd':
	case 'D':
		Test.Printf(_L("\r\n"));
		Debug();
		break;
	case 'q':
	case 'Q':
		iTrace = 0;
		break;
	case 'v':
	case 'V':
		iTrace = 1;
		break;
	case 's':
	case 'S':
		Test.Printf(_L("\r\n"));
		Test.Printf(_L("Keyboard : %08x, %d\r\n"), iKeyboard->iStatus.Int(), iKeyboard->IsActive());
		Test.Printf(_L("Timer    : %08x, %d\r\n"), iTimer->iStatus.Int(), iTimer->IsActive());
		Test.Printf(_L("Reader   : %08x, %d\r\n"), iReader->iStatus.Int(), iReader->IsActive());
		Test.Printf(_L("Writer   : %08x, %d\r\n"), iWriter->iStatus.Int(), iWriter->IsActive());
		break;
    default:
        break;
		}
	}


void CTestXonXoff::Fail(TTestFailType aType, TInt aError)
	{
	switch (aType)
		{
	case ETestFailBoth:
		Test.Printf(_L("\r\nTimeout at offset %d\r\n"), iOffset);
		break;
	case ETestFailRead:
		Test.Printf(_L("\r\nRead fail (%d) at offset %d\r\n"), aError, iOffset);
		break;
	case ETestFailWrite:
		Test.Printf(_L("\r\nWrite fail (%d) at offset %d\r\n"), aError, iOffset);
		break;
	case ETestBadData:
		Test.Printf(_L("\r\nData verify failure at offset %d\r\n"), iOffset);
		break;
		}
	Debug();
	Reset();
	}


void CTestXonXoff::Write()
	{
	iOffset = 0;
	iWriter->ResetCount();
	StripeDes(iWriter->iDes, 0, iWriter->iDes.Length(), '@', 'Z');
	iWriter->Start();
	}

void CTestXonXoff::Read()
	{
	User::After(1000000);
	iReader->ResetCount();
	iReader->Start();
	iTimer->After(5000000);
	}

TBool CTestXonXoff::CheckRead()
	{
	TPtrC8 ref;
	ref.Set(iWriter->iDes.Ptr()+iOffset, iReader->iDes.Length());
	return ref.Compare(iReader->iDes)==0;
	}

void CTestXonXoff::Halt()
	{
	iReader->Cancel();
	iWriter->Cancel();
	iTimer->Cancel();
	CActiveScheduler::Stop();
	}


////////////////////////////////////////////////////////////////////////////////

TInt E32Main()
	{
	TInt err;
	
	Test.Start(_L("Comm Driver Tests"));
	CommStart();
    Test.Printf(_L("Insert plug in then press a key\r\n"));
	Test.Getch();

	TEST(CTrapCleanup::New()!=NULL);
	CActiveScheduler* Scheduler = new CActiveScheduler;
	TEST(Scheduler!=NULL);
	CActiveScheduler::Install(Scheduler);
/*
	CTestSignals* testsignals = NULL;
	TRAP(err, testsignals = CTestSignals::NewL(0));
	TEST(err==KErrNone);
	testsignals->Start();
	Scheduler->Start();
	delete testsignals;
	
	CTestRandTerm* testrandterm = NULL;
	TRAP(err, testrandterm = CTestRandTerm::NewL(0));
	TEST(err==KErrNone);
	testrandterm->Start();
	Scheduler->Start();
	delete testrandterm;

	CTestPerf* testperf = NULL;
	TRAP(err, testperf = CTestPerf::NewL(0));
	TEST(err==KErrNone);
	testperf->Start();
	Scheduler->Start();
	delete testperf;
*/

	CTestXonXoff* testx = NULL;
	TRAP(err, testx = CTestXonXoff::NewL(0));
	TEST(err==KErrNone);
	testx->Start();
	Scheduler->Start();
	delete testx;

/*
	CTestXonXoff* testx1 = NULL;
	TRAP(err, testx1 = CTestXonXoff::NewL(0));
	TEST(err==KErrNone);
	testx1->Start();

	CTestXonXoff* testx2 = NULL;
	TRAP(err, testx2 = CTestXonXoff::NewL(1));
	TEST(err==KErrNone);
	testx2->Start();

	Scheduler->Start();

	delete testx1;
	delete testx2;
*/
	Test.End();
	return KErrNone;
	}
