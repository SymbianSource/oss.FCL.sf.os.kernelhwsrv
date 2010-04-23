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
// e32test\device\t_commsk.cpp
// 
//

#include <e32base.h>
#include <e32base_private.h>
#include <e32test.h>
#include <e32cons.h>
#include <e32svr.h>
#include <e32hal.h>
#include <d32comm.h>
#include <e32uid.h>
#include <hal.h>

#if defined (__WINS__)
#define PDD_NAME _L("ECDRV.PDD")
#define LDD_NAME _L("ECOMM.LDD")
#else
#define PDD_NAME _L("EUARTn")
#define LDD_NAME _L("ECOMM")
#endif

//	Our own comms object with synchronous writes
class RComm : public RBusDevComm
	{
public:
	TInt WriteS(const TDesC8& aDes);
	TInt WriteS(const TDesC8& aDes,TInt aLength);
	TInt WriteSE(const TDes8& Entered, TInt aLength);
	};

LOCAL_D RTest test(_L("T_COMMSK"));
RComm* theSerialPort;
TCommCaps theCaps1Buf;
TCommCapsV01& theCaps1=theCaps1Buf();
TBool MediaChangeTestingEnabled;


class TResult
	{
public:
	enum TResTest {ETestPower,ESimpleWriting};
	TResult();
	void Display(CConsoleBase *aConsole,TInt aCycles);
	void Add(TResTest aTst,TInt anErr);
	inline TInt SetFreeMem(TInt aVal)
		{return(iFreeMem=aVal);}
private:
	TInt iFreeMem;	
	TInt iTestPowerFails;
	TInt iWriteFails;
	};

class TSpeedAndName
	{
public:
	TUint iMask; 
	TBps iSpeed;
	const TText* iName;
	};

/*
const TSpeedAndName KSpeeds[]=
	{
	{KCapsBps50,EBps50,_S("50")}, 
	{KCapsBps2400,EBps2400,_S("2400")},
	{KCapsBps9600,EBps9600,_S("9600")},
	{KCapsBps19200,EBps19200,_S("19200")},
	{KCapsBps57600,EBps57600,_S("57600")},
	{KCapsBps115200,EBps115200,_S("115200")},
	};*/
class TFrameAndName
	{
public:
	TDataBits iData;
	TStopBits iStop;
	TParity iParity;
	const TText* iName;
	};
/*
const TFrameAndName KFrameTypes[]=
	{
	{EData8,EStop1,EParityNone,_S("8,N,1")},
	{EData8,EStop1,EParityEven,_S("8,E,1")},
	{EData8,EStop1,EParityOdd,_S("8,O,1")},

	{EData8,EStop2,EParityNone,_S("8,N,2")},
	{EData8,EStop2,EParityEven,_S("8,E,2")},
	{EData8,EStop2,EParityOdd,_S("8,O,2")},

	{EData7,EStop2,EParityNone,_S("7,N,2")},
	{EData7,EStop2,EParityEven,_S("7,E,2")},
	{EData7,EStop2,EParityOdd,_S("7,O,2")},

	{EData7,EStop1,EParityNone,_S("7,N,1")},
	{EData7,EStop1,EParityEven,_S("7,E,1")},
	{EData7,EStop1,EParityOdd,_S("7,O,1")},
	};*/

class THandShakeAndName
	{
public:
	TUint iHandshake;
	const TText* iName;
	};

THandShakeAndName KHandshakes[]=
	{
	{KConfigObeyDSR,_S("DSR/DTR")},
	{KConfigObeyCTS,_S("CTS/RTS")},
	};

enum TSerialTestFault {EBadArg,EUnknownSignal};

TInt RComm::WriteS(const TDesC8& aDes)

//	Syncronous write

	{
	return(WriteS(aDes,aDes.Length()));
	}

TInt RComm::WriteS(const TDesC8& aDes,TInt aLength)
//
//	Syncronous write
//
	{

	TRequestStatus s;
	Write(s,aDes,aLength);
	User::WaitForRequest(s);
	return(s.Int());
	}

TResult::TResult()
	{

	iTestPowerFails=0;
	iFreeMem=0;
	iWriteFails=0;
	}

void TResult::Display(CConsoleBase *aConsole,TInt aCycles)
//
//	Display test results:
//
	{

	test.Console()->ClearScreen();
	TInt xStartPos=0;
	TInt yStartPos=2;

	aConsole->SetPos(xStartPos,yStartPos);
	test.Printf(_L("Total cycles         : %d"),aCycles);

	//	Display results of Power Down/Up followed by a simple write/read to the Serial Port
	aConsole->SetPos(xStartPos,yStartPos+2);
	if (MediaChangeTestingEnabled)
		test.Printf(_L("Media Change failures: %d\n"),iTestPowerFails);
	else
		test.Printf(_L("Power test failures  : %d\n"),iTestPowerFails);
	aConsole->SetPos(xStartPos,yStartPos+3);
	test.Printf(_L("Serial test failures : %d\n"),iWriteFails);

	aConsole->SetPos(xStartPos,yStartPos+5);
	test.Printf(_L("Free mem (in bytes)  : %d"),iFreeMem);
	}

void TResult::Add(TResTest aTst,TInt anErr)
//	
//	Increment the corresponding variable if test fails
//	
	{

	if (anErr!=KErrNone)
		{
		switch (aTst)
			{
			case ETestPower:
				if (anErr!=KErrNone)
					++iTestPowerFails;
				break;
			case ESimpleWriting:
				if (anErr!=KErrNone)
					++iWriteFails;
				break;
			default:
				iTestPowerFails=0;
				break;
			}
		}
	}

#ifdef _DEBUG_DEVCOMM
LOCAL_C void CommDebug(RBusDevComm& aComm)
	{
	TCommDebugInfoPckg infopckg;
	TCommDebugInfo& info = infopckg();
	aComm.DebugInfo(infopckg);

	test.Printf(_L("  LDD State        :    TX         RX    \r\n"));
	test.Printf(_L("  Busy             : %10d %10d\r\n"), info.iTxBusy, info.iRxBusy);
	test.Printf(_L("  Held             : %10d %10d\r\n"), info.iTxHeld, info.iRxHeld);
	test.Printf(_L("  Length           : %10d %10d\r\n"), info.iTxLength, info.iRxLength);
	test.Printf(_L("  Offset           : %10d %10d\r\n"), info.iTxOffset, info.iRxOffset);
	test.Printf(_L("  Int Count        : %10d %10d\r\n"), info.iTxIntCount, info.iRxIntCount);
	test.Printf(_L("  Err Count        : %10d %10d\r\n"), info.iTxErrCount, info.iRxErrCount);
	test.Printf(_L("  Buf Count        : %10d %10d\r\n"), info.iTxBufCount, info.iRxBufCount);
	test.Printf(_L("  Fill/Drain       : %10d %10d\r\n"), info.iFillingTxBuf, info.iFillingTxBuf);
	test.Printf(_L("  XON              : %10d %10d\r\n"), info.iTxXon, info.iRxXon);
	test.Printf(_L("  XOFF             : %10d %10d\r\n"), info.iTxXoff, info.iRxXoff);
	test.Printf(_L("  Chars            : %10d %10d\r\n"), info.iTxChars, info.iRxChars);
	}
#else
void CommDebug(RBusDevComm& /*aComm*/)
	{
	test.Printf(_L("Debug Dump not available\r\n"));
	}
#endif

LOCAL_C void Panic(TSerialTestFault const& aFault)
//
// Panic the test code
//
	{
	User::Panic(_L("Comm Test"),aFault);
	}

LOCAL_C void StripeMem(TDes8& aBuf,TUint aStartChar,TUint anEndChar)
//
// Mark a buffer with repeating byte pattern
//
	{

	__ASSERT_ALWAYS(aStartChar<=anEndChar,Panic(EBadArg));
	if (aStartChar==anEndChar)
		{
		aBuf.Fill(aStartChar);
		return;
		}

	TUint character=aStartChar;
	for (TInt i=0;i<aBuf.Length();i++)
		{
		aBuf[i]=(TText8)character;
		if(++character>anEndChar)
			character=aStartChar;
		}
	}

LOCAL_C TInt TestSimpleWriting()
	{	
	
	const TInt KBufSize=100;
	TUint8* inBuf=new TUint8[KBufSize]; 
	TUint8* outBuf=new TUint8[KBufSize];
	TPtr8 outDes(outBuf,KBufSize,KBufSize);
	TPtr8 inDes(inBuf,KBufSize,KBufSize);
	StripeMem(outDes,'A','Z');
	inDes.FillZ();
	inDes.SetLength(1);
	
	TRequestStatus readStatus;
	TRequestStatus timeStatus;
			
	theSerialPort->WriteS(outDes,KBufSize);
	theSerialPort->Read(readStatus,inDes);
	
	RTimer tim;
	test(tim.CreateLocal()==KErrNone);
	tim.After(timeStatus,2000000);  // Async. timer request - 2Secs

			
	User::WaitForRequest(timeStatus,readStatus);
	
	if (readStatus!=KRequestPending)
        {
		//	Serial request is complete - cancel timer
		tim.Cancel();
		User::WaitForRequest(timeStatus);
		if (readStatus==KErrNone)
			{
			test(inDes.Length()==inDes.MaxLength());
			test(inDes.Length()==KBufSize);
			outDes.SetLength(inDes.Length());
			return((inDes.Compare(outDes)==0)?KErrNone:KErrGeneral);
			}
		else 
			return KErrGeneral;	
		}
	
	else if (timeStatus!=KRequestPending)
       	{
		//	Timed out before Serial test completed
		theSerialPort->ReadCancel(); // Cancel serial read
		User::WaitForRequest(readStatus);
		return KErrTimedOut;
		}
	else
		Panic(EUnknownSignal);
	return(KErrNone); // Never gets here
	}

LOCAL_C TInt SetUp()
//
//	Set up the serial port
//
	{
	
	theSerialPort->QueryReceiveBuffer();

	TCommConfig cBuf;
	TCommConfigV01& c=cBuf();
	theSerialPort->Config(cBuf);
	
	c.iFifo=EFifoEnable;
	c.iDataBits=EData8;
	c.iStopBits=EStop1;
	c.iParity=EParityNone;
	c.iRate=EBps115200;
	c.iHandshake=0;

	TInt r=theSerialPort->SetConfig(cBuf);
	return (r);
	}

LOCAL_C TInt TestPower()	
	{	
	
	User::After(1000000);	// Allow 1 second before power down
	RTimer timer;
	TRequestStatus done;
	timer.CreateLocal();
	TTime wakeup;
	wakeup.HomeTime();
	wakeup+=TTimeIntervalSeconds(10);
	timer.At(done,wakeup);
	UserHal::SwitchOff();
	User::WaitForRequest(done);
	return (done.Int());
	}
	
GLDEF_C TInt E32Main()
    {

//	test.SetLogged(EFalse); 	// Turn off serial port debugging!

	test.Title();
	test.Start(_L("Comms Soak Test"));

	TBuf <0x100> cmd;
	User::CommandLine(cmd);
	TInt port=0;
	if ((cmd.Length()>0) && (cmd[0]>='1' && cmd[0]<='4'))
		port=(TInt)(cmd[0]-'0');

	test.Next(_L("Load Ldd/Pdd"));
	TInt r;
    TBuf<10> pddName=PDD_NAME;
#if !defined (__WINS__)
	pddName[5]=(TText)('1'+port);
	TInt muid=0;
	if (HAL::Get(HAL::EMachineUid, muid)==KErrNone)
		{
		// Brutus uses EUART4 for both COM3 and COM4
		if (muid==HAL::EMachineUid_Brutus && port==4)
			pddName[5]=(TText)'4';
		}
#endif
	r=User::LoadPhysicalDevice(pddName);
	test.Printf(_L("Load %S returned %d\n\r"),&pddName,r);
	test(r==KErrNone || r==KErrAlreadyExists);

	r=User::LoadLogicalDevice(LDD_NAME);
	test.Printf(_L("Load LDD returned %d\n\r"),r);
	test(r==KErrNone || r==KErrAlreadyExists);

	theSerialPort=new RComm;
	test(theSerialPort!=NULL);

	//	Set up the serial port
	theSerialPort->Open(port);
	r=SetUp();
	test(r==KErrNone);
	
	MediaChangeTestingEnabled=EFalse;
	test.Printf(_L("\r\nThis test requires a loopback conector on the Serial Port\r\n"));
	test.Printf(_L("<<Hit M for media change testing>>\r\n"));
	test.Printf(_L("<<Any other key for power testing>>\r\n"));
	TChar c=(TUint)test.Getch();
	c.UpperCase();
	if (c=='M')
		MediaChangeTestingEnabled=ETrue;

	//	Continuous Test
	TInt cycles=0;
	TResult results;
	
	TRequestStatus stat;
	test.Console()->Read(stat);
	
	while (stat==KRequestPending)
		{
		// Calculate the amount of free memory
		TMemoryInfoV1Buf membuf;
    	UserHal::MemoryInfo(membuf);
    	TMemoryInfoV1 &memoryInfo=membuf();
		results.SetFreeMem(memoryInfo.iFreeRamInBytes);
	
		if (MediaChangeTestingEnabled)
			{
			// Media Change Test (if enabled) then SetUp Serial Port and test we can still read/write
//			UserSvr::ForceRemountMedia(ERemovableMedia0); // Generate media change
//			User::After(1000000);	// Allow 1 second after power down
			}
		else
			{
			// Power Down then test we can still read/write
			r=TestPower();			
			results.Add(TResult::ETestPower,r);
			}
		r=TestSimpleWriting();
		results.Add(TResult::ESimpleWriting,r);

		//	Display the results of the test cycle
		cycles++;
		results.Display(test.Console(),cycles);
		}  

	User::WaitForRequest(stat);
	theSerialPort->Close();
	test.End();

	return(KErrNone);
	}
