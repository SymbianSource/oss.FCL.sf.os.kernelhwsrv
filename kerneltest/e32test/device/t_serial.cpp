// Copyright (c) 1995-2009 Nokia Corporation and/or its subsidiary(-ies).
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
// e32test\device\t_serial.cpp
// 
//

//#define _DEBUG_DEVCOMM

#define __E32TEST_EXTENSION__
#include <e32base.h>
#include <e32base_private.h>
#include <e32test.h>
#include <e32cons.h>
#include <e32svr.h>
#include <e32hal.h>
#include <d32comm.h>
#include <e32uid.h>
#include <hal.h>
#include "d_lddturnaroundtimertest.h"
#include <u32hal.h>

//#define DRIVER_TRACE_ON // disables or adjusts timeout for tests affected by LDD trace

// Enable aggressive paging policy if required
#if 0
#define WDP_ENABLED // affects some tests
#define FLUSH_WDP_CACHE UserSvr::HalFunction(EHalGroupVM,EVMHalFlushCache,0,0)
#else
#define FLUSH_WDP_CACHE
#endif

#if defined (__WINS__)
#define PDD_NAME _L("ECDRV.PDD")
#define LDD_NAME _L("ECOMM.LDD")
#else
#define PDD_NAME _L("EUART")
#define LDD_NAME _L("ECOMM")
#endif

const char KSpinner[]={'|','/','-','\\',};

#include "../power/async.h"

#define CHECK(r,v)	{if ((r)!=(v)) {test.Printf(_L("Line %d Expected %d Got %d\n"),__LINE__,(v),(r)); test(0);}}

// constant expressions for elements in an array, and 1st address past the end
#define ELEMENTS(A) (sizeof(A)/sizeof(A[0]))
#define LIMIT(A) (A + ELEMENTS(A))

// Our own comms object with synchronous writes
class RComm : public RBusDevComm
	{
public:
	TInt WriteS(const TDesC8& aDes);
	TInt WriteS(const TDesC8& aDes,TInt aLength);
	// Override the read functions to flush the paging cache
	inline void Read(TRequestStatus &aStatus,TDes8 &aDes)
		{
		FLUSH_WDP_CACHE;
		RBusDevComm::Read(aStatus, aDes);
		}
	inline void Read(TRequestStatus &aStatus,TDes8 &aDes,TInt aLength)
		{
		FLUSH_WDP_CACHE;
		RBusDevComm::Read(aStatus, aDes, aLength);
		}
	inline void ReadOneOrMore(TRequestStatus &aStatus,TDes8 &aDes)
		{
		FLUSH_WDP_CACHE;
		RBusDevComm::ReadOneOrMore(aStatus, aDes);
		}
	};

LOCAL_D RTest test(_L("T_SERIAL"));
RComm* theSerialPorts[2];
TCommCaps2 theCaps1Buf;
TCommCapsV02& theCaps1=theCaps1Buf();
TCommCaps2 theCaps2Buf;
TCommCapsV02& theCaps2=theCaps2Buf();
TInt PortA;
TInt PortB;

const TInt KWriteSize=250;
const TInt KXonNumReads=0x10;
const TInt KXonReadSize=0x400;

class TSpeedAndName
	{
public:
	TUint iMask;
	TBps iSpeed;
	const TText* iName;
	};

class TSpeedAndNameV2
	{
public:
	TUint iMask;
	TBps iSpeed;
	const TText* iName;
	TUint iBps;
	};

const TSpeedAndName KSpeeds[]=
	{
//	{KCapsBps50,EBps50,_S("50")},
//	{KCapsBps75,EBps75,_S("75")},
//	{KCapsBps110,EBps110,_S("110")},
//	{KCapsBps134,EBps134,_S("134")},
//	{KCapsBps150,EBps150,_S("150")},
//	{KCapsBps300,EBps300,_S("300")},
//	{KCapsBps600,EBps600,_S("600")},
//	{KCapsBps1200,EBps1200,_S("1200")},
//	{KCapsBps1800,EBps1800,_S("1800")},
//	{KCapsBps2000,EBps2000,_S("2000")},
//	{KCapsBps2400,EBps2400,_S("2400")},
//	{KCapsBps3600,EBps3600,_S("3600")},
//	{KCapsBps4800,EBps4800,_S("4800")},
//	{KCapsBps7200,EBps7200,_S("7200")},
	{KCapsBps9600,EBps9600,_S("9600")},
	{KCapsBps19200,EBps19200,_S("19200")},
//	{KCapsBps38400,EBps38400,_S("38400")},
	{KCapsBps57600,EBps57600,_S("57600")},
	{KCapsBps115200,EBps115200,_S("115200")},
	};

// These speeds are used to test break handling
const TSpeedAndNameV2 KBreakSpeeds[]=
	{
//	{KCapsBps50,EBps50,_S("50"),50},
//	{KCapsBps75,EBps75,_S("75"),75},
//	{KCapsBps110,EBps110,_S("110"),110},
//	{KCapsBps134,EBps134,_S("134"),134},
//	{KCapsBps150,EBps150,_S("150"),150},
	{KCapsBps300,EBps300,_S("300"),300},
//	{KCapsBps600,EBps600,_S("600"),600},
	{KCapsBps1200,EBps1200,_S("1200"),1200},
//	{KCapsBps1800,EBps1800,_S("1800"),1800},
//	{KCapsBps2000,EBps2000,_S("2000"),2000},
//	{KCapsBps2400,EBps2400,_S("2400"),2400},
//	{KCapsBps3600,EBps3600,_S("3600"),3600},
	{KCapsBps4800,EBps4800,_S("4800"),4800},
//	{KCapsBps7200,EBps7200,_S("7200"),7200},
//	{KCapsBps9600,EBps9600,_S("9600"),9600},
//	{KCapsBps19200,EBps19200,_S("19200"),19200},
//	{KCapsBps38400,EBps38400,_S("38400"),38400},
	{KCapsBps57600,EBps57600,_S("57600"),57600},
	{KCapsBps115200,EBps115200,_S("115200"),115200},
	};

// Multiplying factors to give Min turnaround times in microseconds between Rx and Tx
#if defined (__WINS__)
const TUint KTurnaroundTimes[] =
{
	150,
	120,
	90,
	60
};
#else
const TUint KTurnaroundTimes[] =
{
#ifdef DRIVER_TRACE_ON
	150,
	120,
	90,
	60
#else
	15,
	12,
	9,
	6
#endif
};
#endif

class TFrameAndName
	{
public:
	TDataBits iData;
	TStopBits iStop;
	TParity iParity;
	const TText* iName;
	};

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

//	{EData6,EStop2,EParityNone,_S("6,N,2")},
//	{EData6,EStop2,EParityEven,_S("6,E,2")},
//	{EData6,EStop2,EParityOdd,_S("6,O,2")},

//	{EData6,EStop1,EParityNone,_S("6,N,1")},
//	{EData6,EStop1,EParityEven,_S("6,E,1")},
//	{EData6,EStop1,EParityOdd,_S("6,O,1")},

//	{EData5,EStop1,EParityNone,_S("5,N,1")},
//	{EData5,EStop1,EParityEven,_S("5,E,1")},
//	{EData5,EStop1,EParityOdd,_S("5,O,1")},
	};

class THandShakeAndName
	{
public:
	TUint iHandshake;
	const TText* iName;
	};

THandShakeAndName KHandshakes[]=
	{
//	{KConfigObeyDSR,_S("DSR/DTR")},		// most cables don't actually support this
	{KConfigObeyCTS,_S("CTS/RTS")},
// 	{KConfigObeyDCD,_S("DCD")},
	};

enum TSerialTestFault
	{
	EBadArg,
	};
_LIT(KLddFileName, "D_LDDTURNAROUNDTIMERTEST.LDD");
RLddTest1 ldd;

#ifdef _DEBUG_DEVCOMM
void CommDebug(RBusDevComm& aComm)
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
//	test.Printf(_L("  DFC Pending      : %10d %10d\r\n"), info.iTxDfcPend, info.iTxDfcPend);
//	test.Printf(_L("  DFC Run/Count    : %10d %10d\r\n"), info.iRunningDfc, info.iDfcCount);
//	test.Printf(_L("  DFC Req/Do/Drain : %10d %10d %10d\r\n"), info.iDfcReqSeq, info.iDfcHandlerSeq, info.iDoDrainSeq);
	}
#else
void CommDebug(RBusDevComm& /*aComm*/)
	{
	test.Printf(_L("Debug Dump not available\r\n"));
	}
#endif

TInt RComm::WriteS(const TDesC8& aDes)
//
// Syncronous write
//
	{

	return(WriteS(aDes,aDes.Length()));
	}


TInt RComm::WriteS(const TDesC8& aDes,TInt aLength)
//
// Syncronous write
//
	{

	TRequestStatus s;

	// Force there to be paging events
	FLUSH_WDP_CACHE;

	//
	Write(s,aDes,aLength);
	User::WaitForRequest(s);
	return(s.Int());
	}

void Panic(TSerialTestFault const& aFault)
//
// Panic the test code
//
	{
	User::Panic(_L("Comm Test"),aFault);
	}

void StripeMem(TDes8& aBuf,TUint aStartChar,TUint anEndChar)
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

#define COLUMN_HEADER _L("                 InBuf         |         outbuf\n\r")

bool CompareDescriptors(TDes8 &aLeft,TDes8 &aRight)
//
// Compare a couple of descriptors and dump them if they don't match
//
	{
	TInt lLen=aLeft.Length();
	TInt rLen=aRight.Length();
	TInt minLen=Min(lLen,rLen);

	aRight.SetLength(minLen);
	aLeft.SetLength(minLen);
	bool r = (aLeft.Compare(aRight)==0);
	if (!r)
		{
		RDebug::Print(_L("Compare failed - dumping descriptors\n\r"));
		TInt len=aLeft.Length();
		RDebug::Print(COLUMN_HEADER);
		TBuf8<0x100> buf;
		for (TInt i=0;i<=len/8;i++)
		{
			buf.Zero();
			buf.SetLength(0);
			buf.AppendFormat(_L8("%4d: "),i*8);

			for (TInt j=0;j<8;j++)
				{
				if ((i*8)+j<len)
					{
					TInt v=aLeft[(i*8)+j];
					buf.AppendFormat(_L8("%02x "),v);
					}
				else
					buf.AppendFormat(_L8("   "));
				}
			buf.AppendFormat(_L8(" | "));
			for (TInt k=0;k<8;k++)
				{
				if ((i*8)+k>=len)
					break;
				TInt v=aRight[(i*8)+k];
				buf.AppendFormat(_L8("%02x "),v);
				}
			buf.AppendFormat(_L8("\r\n"));
			RDebug::RawPrint(buf);	
			}
		}

	if (!r) {
		theSerialPorts[0]->Close();
		theSerialPorts[1]->Close();
		aRight.SetLength(rLen);
		aLeft.SetLength(lLen);
	}
	return r;
}

TInt CheckedWrite(TInt aBufSize)
//
// Write a buffer from one serial port to the other and vice versa.
//
	{
	TUint8* inBuf=new TUint8[aBufSize];
	test(inBuf!=NULL);
	TUint8* outBuf=new TUint8[aBufSize];
	test(outBuf!=NULL);
	TPtr8 outDes(outBuf,aBufSize,aBufSize);
	TPtr8 inDes(inBuf,aBufSize,aBufSize);

	RTimer tim;
	tim.CreateLocal();
	TRequestStatus readStatus;
	TRequestStatus timeStatus;

	StripeMem(outDes,'A','Z');
	inDes.FillZ();

	const TInt KReadFirstPort=0;
	const TInt KWriteFirstPort=1;

	// Check the driver rejects an attempt to read more data than the buffer allows
	theSerialPorts[KReadFirstPort]->Read(readStatus,inDes,aBufSize+1);
	test(readStatus==KErrGeneral);

	// Start reading for real
	theSerialPorts[KReadFirstPort]->Read(readStatus,inDes);
	test_Equal(KRequestPending, readStatus.Int());

	// Synchronous write
	TInt ret=theSerialPorts[KWriteFirstPort]->WriteS(outDes,aBufSize);
	test(ret==KErrNone);

	// Set a 6 second timer going
	const TUint KTimeOut=12000000;
	tim.After(timeStatus,KTimeOut);
	test(timeStatus==KRequestPending);

	// Wait for EITHER the read to complete, OR for the timer to timeout
	User::WaitForRequest(readStatus,timeStatus);
	if (timeStatus==KErrNone) // timer completed normally... oh dear, what happened to the read??
		{
		test.Printf(_L("RComm::Read() timed out!\n\r"));
		theSerialPorts[KReadFirstPort]->ReadCancel();
		test(EFalse);	// fail
		}
	else
		{
		tim.Cancel();
		if (readStatus!=KErrNone)
			test.Printf(_L("Read Failed! (%d)\n\r"),readStatus.Int());
		test(readStatus==KErrNone);
		test(ret==KErrNone);
		test.Printf(_L("\rRead %d of %d\n\r"),inDes.Length(),outDes.Length());
		test(CompareDescriptors(outDes,inDes));
		}

	test.Printf(_L("\t\t\tReverse\n"));
	theSerialPorts[KWriteFirstPort]->Read(readStatus,inDes,aBufSize);
	theSerialPorts[KReadFirstPort]->WriteS(outDes,aBufSize);
	User::WaitForRequest(readStatus);
	tim.After(timeStatus,KTimeOut);
	test(timeStatus==KRequestPending);
	User::WaitForRequest(readStatus,timeStatus);
	if (timeStatus==KErrNone)
		{
		test.Printf(_L("Timed Out!\n\r"));
		theSerialPorts[KWriteFirstPort]->ReadCancel();
		test(EFalse);	// fail
		}
	else
		{
		tim.Cancel();
		if (readStatus!=KErrNone)
				test.Printf(_L("Read Failed! (%d)\n\r"),readStatus.Int());
		test(readStatus==KErrNone);
		test(ret==KErrNone);
		test.Printf(_L("\rRead %d of %d\n\r"),inDes.Length(),outDes.Length());
		outDes.SetLength(inDes.Length());
		test(CompareDescriptors(outDes,inDes));
		}

	tim.Close();
	delete [] inBuf;
	delete [] outBuf;

	return inDes.Length();
	}

TUint CheckZeroTurnaround(TInt aBufSize, TUint aDirection)
//
// Checks that when a Turnaround of 0ms was selected the write takes place immediately
// aBufSize is selected such as it takes only slightly less than User timer granularity
// at the Baud rate selected to transmit that buffer.
// Checks that it takes less than a User side timer tick to complete a Write request
// at the selected Baud rate. Therefore proving the write is not being delayed in the driver
//
	{
	TUint8* inBuf=new TUint8[aBufSize];
	test(inBuf!=NULL);
	TUint8* outBuf=new TUint8[aBufSize];
	test(outBuf!=NULL);
	TPtr8 outDes(outBuf,aBufSize,aBufSize);
	TPtr8 inDes(inBuf,aBufSize,aBufSize);
	TInt numberRead = 0;

	TTimeIntervalMicroSeconds32 aTimeOut=0;
	UserHal::TickPeriod(aTimeOut);

	RTimer timeoutTimer;
	timeoutTimer.CreateLocal();
	TRequestStatus readStatus;
	TRequestStatus writeStatus;
	TRequestStatus timeStatus;

	StripeMem(outDes,'A','Z');
	inDes.FillZ();

	const TUint port_A = aDirection?1:0;
	const TUint port_B = 1 - port_A;

	// queue a read on port_A
	test.Printf(_L("\r\nRead %d \r\n"), port_A);
	theSerialPorts[port_A]->Read(readStatus,inDes);
	test(readStatus==KRequestPending);

	// write on port_B to complete read
	theSerialPorts[port_B]->SetMinTurnaroundTime(0);
	theSerialPorts[port_B]->Write(writeStatus,outDes,aBufSize);
	test(writeStatus==KRequestPending || writeStatus==KErrNone );

	// start the local turnaround timer
	timeoutTimer.After(timeStatus, (20*aTimeOut.Int()));		// give it a 20% margin
	test(timeStatus==KRequestPending);

	User::WaitForRequest(readStatus, timeStatus);

	if(timeStatus == KErrNone)
		{
		// if timeout first -> BAD
		test.Printf(_L("Timed out!\r\n"));
		theSerialPorts[port_A]->ReadCancel();
		test(EFalse);	// fail
		}
	else
		{
		// else read was first -> GOOD
		timeoutTimer.Cancel();

		if (readStatus!=KErrNone)
				test.Printf(_L("Read Failed! (%d)\n\r"),readStatus.Int());
		test(readStatus==KErrNone);
		test(writeStatus==KErrNone);
		test.Printf(_L("Good, read OK and took less than timeout\r\n"));
		test(CompareDescriptors(outDes,inDes));
		numberRead = inDes.Length();
		}
	timeoutTimer.Close();
	delete inBuf;
	delete outBuf;

	return numberRead;
	}


TUint TimedCheckedWrite(TInt aBufSize, TUint aTurnaround, TUint aDirection)
//
// Checks that Write requests are delayed if a Turnaround != 0 is selected.
// aTurnarund is chosen to be significantly greater than the time it takes to transmit
// a buffer of aBufSize at the Baud rate.
// Checks that for a given Turnaround time it always takes > that time to transmit
// a buffer of aBufSize.
// aDirection specifies the direction of transmission.
//
	{
	TUint8* inBuf=new TUint8[aBufSize];
	test(inBuf!=NULL);
	TUint8* outBuf=new TUint8[aBufSize];
	test(outBuf!=NULL);
	TPtr8 outDes(outBuf,aBufSize,aBufSize);
	TPtr8 inDes(inBuf,aBufSize,aBufSize);
	TInt numberRead = 0;

	TTimeIntervalMicroSeconds32 p=0;
	UserHal::TickPeriod(p);
	TInt tPeriod = p.Int();

	const TUint KTimeOut = 1500000;		// 1500 milliseconds
	RTimer timeoutTimer;
	timeoutTimer.CreateLocal();
	TRequestStatus readStatus;
	TRequestStatus writeStatus;
	TRequestStatus timeStatus;

	RTimer turnaroundTimer;
	turnaroundTimer.CreateLocal();
	TRequestStatus turnaroundTimerStatus;

	StripeMem(outDes,'A','Z');
	inDes.FillZ();

	const TUint port_A = aDirection?1:0;
	const TUint port_B = 1 - port_A;

	// set turnaround on port_A
	TInt r = theSerialPorts[port_A]->SetMinTurnaroundTime(aTurnaround+tPeriod);
	test(r== KErrNone);
	r = theSerialPorts[port_B]->SetMinTurnaroundTime(0);
	test(r== KErrNone);

	// queue a short read on port_A
	test.Printf(_L("\r\nRead %d to set turnaround %d\r\n"), port_A, aTurnaround+tPeriod);
	theSerialPorts[port_A]->Read(readStatus,inDes);
	test(readStatus==KRequestPending);

	// start the local turnaround timer
	turnaroundTimer.After(turnaroundTimerStatus, aTurnaround);
	test(turnaroundTimerStatus==KRequestPending);

	// write on port_B to complete read and start the driver's turnaround timer on A
	theSerialPorts[port_B]->Write(writeStatus,outDes,aBufSize);
	test((writeStatus==KRequestPending)||(writeStatus==KErrNone));		// may complete before coming back here as buffer size's small

	User::WaitForRequest(readStatus);
	test(readStatus==KErrNone);
	test(writeStatus==KErrNone);
	test(CompareDescriptors(outDes,inDes));
	inDes.FillZ();

	// queue a short read on port_B
	theSerialPorts[port_B]->Read(readStatus, inDes);
	test(readStatus==KRequestPending);

	// write on port_A
	theSerialPorts[port_A]->Write(writeStatus,outDes,aBufSize);
	test(writeStatus==KRequestPending);

	// wait on both the read on port_B and the local turnaround timer
	User::WaitForRequest(readStatus, turnaroundTimerStatus);

	if(turnaroundTimerStatus == KErrNone)
		{
		// if local turnaround timeout first -> GOOD
		// start big timeout and wait on either timeout or read on port_B
		timeoutTimer.After(timeStatus, KTimeOut);
		test(timeStatus==KRequestPending);

		User::WaitForRequest(readStatus, timeStatus);
		if(timeStatus == KErrNone)
			{
			// if timeout first -> BAD
			test.Printf(_L("Timed out!\r\n"));

			theSerialPorts[port_B]->ReadCancel();
			test(EFalse);	// fail
			}
		else
			{
			// else read was first -> GOOD
			timeoutTimer.Cancel();

			if (readStatus!=KErrNone)
					test.Printf(_L("Read Failed! (%d)\n\r"),readStatus.Int());
			test(readStatus==KErrNone);
			test(writeStatus==KErrNone);
			test.Printf(_L("Good, read OK, write took longer than turnaround\r\n"));
			test(CompareDescriptors(outDes,inDes));
			numberRead = inDes.Length();
			}
		}
	else if(readStatus == KErrNone)
		{
		TInt timerStatus = turnaroundTimerStatus.Int();
		// else read was first -> BAD
		turnaroundTimer.Cancel();
		test.Printf(_L("read completed before turnaround\r\n"));
		test.Printf(_L("turnaroundTImer status  %d ms!\r\n"),timerStatus);
		test(EFalse);	// fail
		}

	timeoutTimer.Close();
	turnaroundTimer.Close();
	delete inBuf;
	delete outBuf;

	return numberRead;
	}

// Checks that setting the turnaround first time before any read or write, will start the
// turnaround timer. It is make sure that first write will be delayed atleast min turnaround
// time.
void TestFirstDelayedWrite(TInt aBufSize, TUint aTurnaround, TUint aDirection)
	{
	test.Printf(_L("Loading logical device for getting kernel timer tick & count\n"));
	TInt r=User::LoadLogicalDevice(KLddFileName);
	test(r == KErrNone || r == KErrAlreadyExists);

	test.Printf(_L("Opening of logical device\n"));
	r = ldd.Open();
	test(r == KErrNone || r == KErrAlreadyExists);


	// Create input and an output buffers
	TUint8* inBuf=new TUint8[aBufSize];
	test(inBuf!=NULL);
	TUint8* outBuf=new TUint8[aBufSize];
	test(outBuf!=NULL);

	// Fill the output buffer with stuff and empty the input buffer
	TPtr8 outDes(outBuf,aBufSize,aBufSize);
	TPtr8 inDes(inBuf,aBufSize,aBufSize);
	StripeMem(outDes,'A','Z');
	inDes.FillZ();
	

	// Configure both ports to 9600bps.
    TCommConfig cBuf1;
	TCommConfigV01& c1=cBuf1();
	theSerialPorts[0]->Config(cBuf1);
	TCommConfig cBuf2;
	TCommConfigV01& c2=cBuf2();
	theSerialPorts[0]->Config(cBuf2);
	c1.iHandshake=0;
	c2.iHandshake=0;
	c2.iFifo=EFifoDisable;
	c2.iDataBits=c1.iDataBits=EData8;
	c2.iStopBits=c1.iStopBits=EStop1;
	c2.iParity=c1.iParity=EParityNone;
	c2.iRate=c1.iRate=EBps9600;
    r = theSerialPorts[0]->SetConfig(cBuf1);
    test_Equal(KErrNone, r);
	r = theSerialPorts[1]->SetConfig(cBuf2);
	test(r == KErrNone);

	// Create a timer
	RTimer timeoutTimer;
	timeoutTimer.CreateLocal();
	TRequestStatus readStatus = 0xbaadf00d;
	TRequestStatus writeStatus = 0xbaadf00d;
	//TRequestStatus timeStatus = 0xbaadf00d;

	const TUint port_A = aDirection?1:0;
	const TUint port_B = 1 - port_A;

	TUint time1 = 0, time2 = 0, time3 = 0;
	// set turnaround on port_A
	r = theSerialPorts[port_A]->SetMinTurnaroundTime(aTurnaround);
	test(r== KErrNone);

	//Capture the turnaround time
	ldd.Test_getTimerCount(time1);

	// set turnaround on port_B
	r = theSerialPorts[port_B]->SetMinTurnaroundTime(0);
	test(r== KErrNone);

	// queue a short read on port_B
	theSerialPorts[port_B]->Read(readStatus, inDes);
	/* BOGUS TEST: Zero-length reads complete immediately.
	test_Equal(KRequestPending, readStatus.Int());
	*/

	// write on port_A
	theSerialPorts[port_A]->Write(writeStatus,outDes,aBufSize);
	/* BOGUS TEST
	test(writeStatus==KRequestPending);
	*/

	/* BOGUS TEST
	The turnaround timer exists to introduce small delays between SUCCESSIVE reads & writes, 
	so as not to flummox IrDA transceivers which are slow in changing between write & read 
	modes. Setting a timer value does not have an immediate effect, it will
	apply *after* the next read/write.

	// start a local timeout with aTurnaround/3 and wait on it
	timeoutTimer.After(timeStatus, aTurnaround/3);
	test(timeStatus==KRequestPending);
	User::WaitForRequest(timeStatus);
	test(timeStatus==KErrNone);

	// check that read on port_B has not completed yet (write on port_A has been delayed in the driver)
	test_Equal(KRequestPending, readStatus.Int());
	*/

	// wait for write to complete
	User::WaitForRequest(writeStatus);
	if(writeStatus == KErrNone)
		{
		//record the time of write complete
		ldd.Test_getTimerCount(time2);
		}

	//Wait for read to complete
	User::WaitForRequest(readStatus);
	test(readStatus==KErrNone);

	//Covert turnaround time to timer ticks
	time3 = aTurnaround / 1000;
	ldd.Test_getTimerTicks(time3);

	test.Printf(_L("Time1 %d\n"), time1);
	test.Printf(_L("Time2 %d\n"), time2);
	test.Printf(_L("Time3 %d\n"), time3);
	//Write takes apporximately 250 ticks to write.
	time2 = (time2 - time1); //Includes turnaround time + write time
	time1 = time3 > time2 ? time3 - time2 : time2 - time3;
	test.Printf(_L("Time differece %d\n"), time1);
	//test(time1 == 0 || time1 == 1); <-- Apparently unreasonable on SMP hardware

	timeoutTimer.Close();
	test.Printf(_L("Closing the channel\n"));
	ldd.Close();

	test.Printf(_L("Freeing logical device\n"));
	r = User::FreeLogicalDevice(KLddFileName);;
	test(r==KErrNone);

	delete inBuf;
	delete outBuf;

	}




TUint ChangeTurnaroundTimeInDelayedWrite(TInt aBufSize, TUint aTurnaround, TUint aNewTurnaround, TUint aDirection)
//
// Checks that a delayed write will go based on the new timeout value if the Turnaround time is changed
// when a write is being delayed in the driver
// aBufSize is such that transmission of a buffer of that size at the baud rate selected << aTurnaround
// Check that a Write is being delayed by a previous Read and that changing the turnaround will adjust
// the turnaround timer based on the new value and write will happend after minturnaround time has elapsed
//
	{
	test.Printf(_L("Loading logical device for getting kernel timer tick & count\n"));
	TInt r=User::LoadLogicalDevice(KLddFileName);
	test(r == KErrNone || r == KErrAlreadyExists);

	test.Printf(_L("Opening of logical device\n"));
	r = ldd.Open();
	test(r == KErrNone || r == KErrAlreadyExists);

	TUint8* inBuf=new TUint8[aBufSize];
	test(inBuf!=NULL);
	TUint8* outBuf=new TUint8[aBufSize];
	test(outBuf!=NULL);
	TPtr8 outDes(outBuf,aBufSize,aBufSize);
	TPtr8 inDes(inBuf,aBufSize,aBufSize);
	TInt numberRead = 0;

	StripeMem(outDes,'A','Z');
	inDes.FillZ();

	RTimer timeoutTimer;
	timeoutTimer.CreateLocal();
	TRequestStatus readStatus;
	TRequestStatus writeStatus;
	TRequestStatus timeStatus;

	const TUint port_A = aDirection?1:0;
	const TUint port_B = 1 - port_A;

	// set turnaround on port_A
	r = theSerialPorts[port_A]->SetMinTurnaroundTime(aTurnaround);
	test(r== KErrNone);
	// set turnaround on port_B
	r = theSerialPorts[port_B]->SetMinTurnaroundTime(0);
	test(r== KErrNone);

	// Issue a zero length read on port_A
	theSerialPorts[port_A]->Read(readStatus,inDes,0);
	User::WaitForRequest(readStatus);
	test(readStatus==KErrNone);
	//Record the start of turnaround time on port_A
	TUint time1 = 0, time2 = 0, time3 = 0;
	ldd.Test_getTimerCount(time1);

	// queue a short read on port_B
	theSerialPorts[port_B]->Read(readStatus, inDes);
	test(readStatus==KRequestPending);

	// write on port_A
	theSerialPorts[port_A]->Write(writeStatus,outDes,aBufSize);
	test(writeStatus==KRequestPending);

	// start a local timeout with aTurnaround/3 and wait on it
	timeoutTimer.After(timeStatus, aTurnaround/3);
	test(timeStatus==KRequestPending);
	User::WaitForRequest(timeStatus);
	test(timeStatus==KErrNone);

	// check that read on port_B has not completed yet (write on port_A has been delayed in the driver)
#ifndef WDP_ENABLED // lots of paging screws up timing assumptions
	test(readStatus==KRequestPending);
	test(writeStatus==KRequestPending);
#endif

	// change turnaround on port_A (should adjust turnaround time accordingly)
	r = theSerialPorts[port_A]->SetMinTurnaroundTime(aNewTurnaround);
	test(r==KErrNone);

	//Check read on port_B & write on port_A is still delayed.
#if !defined(DRIVER_TRACE_ON) && !defined(WDP_ENABLED)
	test(readStatus==KRequestPending);
	test(writeStatus==KRequestPending);
#endif
	// wait for write to complete
	User::WaitForRequest(writeStatus);
	if(writeStatus == KErrNone)
	{
		//record the time of write complete
		ldd.Test_getTimerCount(time2);
	}

	//Wait for read to complete
	User::WaitForRequest(readStatus);
	test(readStatus==KErrNone);

	//Calculate the turnaround time, write should be delayed.
	time3 = aNewTurnaround/1000;
	//Convert to timer ticks
	ldd.Test_getTimerTicks(time3);
	test.Printf(_L("aTurnaround = %d, aNewTurnaround = %d\n"), aTurnaround, aNewTurnaround);
	test.Printf(_L("Time1 = %d\n"), time1);
	test.Printf(_L("Time2 = %d\n"), time2);
	test.Printf(_L("Time3 = %d\n"), time3);
	time1 = time2 - time1;
	time1 = time3 > time1 ? (time3 - time1) : (time1 - time3);
	test.Printf(_L("Time difference %d\n"), time1);
#if !defined(DRIVER_TRACE_ON) && !defined(WDP_ENABLED)
//	test((time1 == 1) || (time1 == 0));
#endif
	test.Printf(_L("Write delayed for requested time\r\n"));
	test(CompareDescriptors(outDes,inDes));
	numberRead = inDes.Length();

	timeoutTimer.Close();
	test.Printf(_L("Closing the channel\n"));
	ldd.Close();

	test.Printf(_L("Freeing logical device\n"));
	r = User::FreeLogicalDevice(KLddFileName);;
	test(r==KErrNone);

	delete inBuf;
	delete outBuf;

	return numberRead;
	}


TUint StopInDelayedWrite(TInt aBufSize, TUint aTurnaround, TUint aDirection)
//
// Checks that when a write is being delayed and then is cancelled the driver's turnaround timer continues
// ticking and if another write is queued it will be delayed by the remaining time
// aBufSize is such that transmission of a buffer of that size at the baud rate selected << aTurnaround
// Check that a Write is being delayed by a previous Read and that changing the Turnaround will make it
// go ahead immediately
//
	{
	TUint8* inBuf=new TUint8[aBufSize];
	test(inBuf!=NULL);
	TUint8* outBuf=new TUint8[aBufSize];
	test(outBuf!=NULL);
	TPtr8 outDes(outBuf,aBufSize,aBufSize);
	TPtr8 inDes(inBuf,aBufSize,aBufSize);
	TInt numberRead = 0;

	TTimeIntervalMicroSeconds32 p=0;
	UserHal::TickPeriod(p);
	TInt tPeriod = p.Int();

	const TUint KTimeOut = 1500000;		// 150 milliseconds
	RTimer timeoutTimer;
	timeoutTimer.CreateLocal();
	TRequestStatus readStatus;
	TRequestStatus writeStatus;
	TRequestStatus timeStatus;

	RTimer turnaroundTimer;
	turnaroundTimer.CreateLocal();
	TRequestStatus turnaroundTimerStatus;

	StripeMem(outDes,'A','Z');
	inDes.FillZ();

	const TUint port_A = aDirection?1:0;
	const TUint port_B = 1 - port_A;

	// set turnaround on port_A
	TInt r = theSerialPorts[port_A]->SetMinTurnaroundTime(aTurnaround+tPeriod);
	test(r== KErrNone);
	r = theSerialPorts[port_B]->SetMinTurnaroundTime(0);
	test(r== KErrNone);


	// queue a zero length read to start the turnaround on port_A
	test.Printf(_L("\r\nRead Zero Length on %d to start turnaround %d\r\n"), port_A, aTurnaround);
	test.Printf(_L("\r\nUsing %d character buffers\r\n"),aBufSize);

	theSerialPorts[port_A]->Read(readStatus,inDes,0);
	User::WaitForRequest(readStatus);
	test(readStatus==KErrNone);

	// start the local turnaround timer
	turnaroundTimer.After(turnaroundTimerStatus, aTurnaround);
	test(turnaroundTimerStatus==KRequestPending);

	// queue a short read on port_B
	theSerialPorts[port_B]->Read(readStatus, inDes);
	test(readStatus==KRequestPending);

	// write on port_A
	theSerialPorts[port_A]->Write(writeStatus,outDes,aBufSize);
	test(writeStatus==KRequestPending);

	// start a local timeout with aTurnaround/3 and wait on it
	timeoutTimer.After(timeStatus, aTurnaround/3);
	test(timeStatus==KRequestPending);
	User::WaitForRequest(timeStatus);
	test(timeStatus==KErrNone);

	// check that read on port_B has not completed yet (write on port_A has been delayed in the driver)
	test(readStatus==KRequestPending);

	// cancel write on port_A
	theSerialPorts[port_A]->WriteCancel();
	test(writeStatus==KErrCancel);

	// ...and restart it again
	theSerialPorts[port_A]->Write(writeStatus,outDes,aBufSize);
#ifndef DRIVER_TRACE_ON
	test(writeStatus==KRequestPending);
#endif

	// wait on both the read on port_B and the local turnaround timer
	User::WaitForRequest(readStatus, turnaroundTimerStatus);

	// We are expecting this to have gone off by now...
	if(turnaroundTimerStatus == KErrNone) // this local timer is LESS than the driver turnaround
		{
		// if local turnaround timeout first -> GOOD
		// start big timeout and wait on either timeout or read on port_B
		timeoutTimer.After(timeStatus, KTimeOut);
		test(timeStatus==KRequestPending);

		User::WaitForRequest(readStatus, timeStatus);
		if(timeStatus == KErrNone)
			{
			// if timeout first -> BAD
			test.Printf(_L("Timed out!\r\n"));

			theSerialPorts[port_B]->ReadCancel();
			test(EFalse);	// fail
			}
		else
			{
			// else read was first -> GOOD
			timeoutTimer.Cancel();

			if (readStatus!=KErrNone)
					test.Printf(_L("Read Failed! (%d) - should have completed (on delayed write data)\n\r"),readStatus.Int());
			test(readStatus==KErrNone);
			test(writeStatus==KErrNone);
			test.Printf(_L("OK, write later than turnaround\r\n"));
			test(CompareDescriptors(outDes,inDes));
			numberRead = inDes.Length();
			}
		}
	// failed here => second write has gone off faster than expected...
	else if(readStatus == KErrNone)
		{
		// else read was first -> BAD
		TInt timerStatus = turnaroundTimerStatus.Int();
		turnaroundTimer.Cancel();
		test.Printf(_L("read completed before turnaround\r\n"));
		test.Printf(_L("Turnaround timer status = %d\r\n"),timerStatus);
		test(EFalse);	// fail
		}

	timeoutTimer.Close();
	turnaroundTimer.Close();
	delete inBuf;
	delete outBuf;

	return numberRead;
	}

void turnaroundTestReadWrite()
//
// Read and write at various speeds, with various turnarounds
// Check that the data received data matches sent data
	{
	// Open both serial ports
	TInt r=theSerialPorts[0]->Open(PortA);
	test(r==KErrNone);
	r=theSerialPorts[0]->QueryReceiveBuffer();
	test(r==0);
	r=theSerialPorts[1]->Open(PortB);
	test(r==KErrNone);
	r=theSerialPorts[1]->QueryReceiveBuffer();
	test(r==0);

	// Perform the read/write test at each baudrate for 8N1, no handshaking
	TCommConfig cBuf1;
	TCommConfigV01& c1=cBuf1();
	theSerialPorts[0]->Config(cBuf1);
	TCommConfig cBuf2;
	TCommConfigV01& c2=cBuf2();
	theSerialPorts[0]->Config(cBuf2);
	c1.iHandshake=0;
	c2.iHandshake=0;
	c2.iFifo=EFifoDisable;

	c2.iDataBits=c1.iDataBits=EData8;
	c2.iStopBits=c1.iStopBits=EStop1;
	c2.iParity=c1.iParity=EParityNone;
	c2.iRate=c1.iRate=EBps9600;

	TBuf<0x40> msg;

	test.Start(_L("Read/write test with default turnaround and at 9600 Bps"));

	TTimeIntervalMicroSeconds32 p=0;
	UserHal::TickPeriod(p);
	TInt tPeriod = p.Int();
	test.Printf(_L("Tick period %d\r\n"), tPeriod);

	TUint aBufLength = 96*p.Int()/10000;
	test.Printf(_L("Need to transmit %d chars at 9600 Bps\r\n"), aBufLength); // let's try with 10*tick period (approx)

	theSerialPorts[0]->SetConfig(cBuf1);
	theSerialPorts[1]->SetConfig(cBuf2);

	// These work fine
	test(CheckZeroTurnaround(aBufLength, 0)==aBufLength);
	test(CheckZeroTurnaround(aBufLength, 1)==aBufLength);

	test.Next(_L("Read/write test at various speeds and min turnarounds"));
#if defined (__WINS__)
	const TUint KShortBufferSize=100;
#else
	const TUint KShortBufferSize=10;
#endif
	TUint direction=0;
	for(TUint i = 0; i < ELEMENTS(KSpeeds); ++i)
		{
		TInt turnaround;
		turnaround = KTurnaroundTimes[i]*p.Int();

		if (theCaps1.iRate&KSpeeds[i].iMask && theCaps2.iRate&KSpeeds[i].iMask)
			{
			msg.Format(_L("\r\nRead/write @ %s Bps with %d millisec turnaround\r\n"), KSpeeds[i].iName, turnaround/1000);
			test.Next(msg);

			c1.iRate=KSpeeds[i].iSpeed;
			TInt r=theSerialPorts[0]->SetConfig(cBuf1);
			test(r==KErrNone);
			c2.iRate=KSpeeds[i].iSpeed;
			r=theSerialPorts[1]->SetConfig(cBuf2);
			test(r==KErrNone);

			test.Printf(_L("Do TimedCheckedWrite\r\n"));
			test(TimedCheckedWrite(KShortBufferSize, turnaround, direction)==KShortBufferSize);
			}
		else
			{
			msg.Format(_L("%s Bps not supported\r\n"),KSpeeds[i].iName);
			test.Next(msg);
			}

		direction=1-direction;

		msg.Format(_L("\r\nRead turnaround time back\r\n"));
			test.Next(msg);

		TInt n = theSerialPorts[0]->MinTurnaroundTime();
		test(n==turnaround+tPeriod);

		msg.Format(_L("Value returned was %d\r\n"), n/1000);
		test.Next(msg);

		test.Printf(_L("Decrease turnaroundtime during delayed write\n"));
		test(ChangeTurnaroundTimeInDelayedWrite(KShortBufferSize, turnaround, turnaround - 10000, direction)==KShortBufferSize);

		test.Printf(_L("Increase turnaroundtime during delayed write\n"));
		test(ChangeTurnaroundTimeInDelayedWrite(KShortBufferSize, turnaround, turnaround + 30000 ,direction)==KShortBufferSize);

		direction=1-direction;

		test.Printf(_L("\r\nDo StopInDelayedWrite @ %s Bps\r\n"), KSpeeds[i].iName);
		test(StopInDelayedWrite(KShortBufferSize, turnaround, direction)==KShortBufferSize);
		}

	// return defaults for following tests

	msg.Format(_L("\r\nSet default turnaround (0) on both ports \r\n"));
	test.Next(msg);

	test(theSerialPorts[0]->SetMinTurnaroundTime(0)==KErrNone);
	test(theSerialPorts[1]->SetMinTurnaroundTime(0)==KErrNone);

	theSerialPorts[0]->Close();
	theSerialPorts[1]->Close();

	msg.Format(_L("\r\n... End of turnaround tests ...\r\n"));
	test.Next(msg);

	test.End();
	}

void testReadWrite()
//
// Read and write at various speeds
	{
	test.Start(_L("Testing read and write"));

	TInt r=theSerialPorts[0]->Open(PortA);
	test(r==KErrNone);
	r=theSerialPorts[0]->QueryReceiveBuffer();
	test(r==0);
	r=theSerialPorts[1]->Open(PortB);
	test(r==KErrNone);
	r=theSerialPorts[1]->QueryReceiveBuffer();
	test(r==0);

	TCommConfig cBuf1;
	TCommConfigV01& c1=cBuf1();
	theSerialPorts[0]->Config(cBuf1);
	TCommConfig cBuf2;
	TCommConfigV01& c2=cBuf2();
	theSerialPorts[0]->Config(cBuf2);
	c1.iHandshake=0;
	c2.iHandshake=0;
	c2.iFifo=EFifoDisable;

	c2.iDataBits=c1.iDataBits=EData8;
	c2.iStopBits=c1.iStopBits=EStop1;
	c2.iParity=c1.iParity=EParityNone;
	c2.iRate=c1.iRate=EBps9600;

	TBuf<0x40> mess;
	test.Printf(_L("Delayed first write\n"));
    TestFirstDelayedWrite(0, 2343750, 1);

	TInt numTests=sizeof(KSpeeds)/sizeof(TSpeedAndName);
	for (TInt i=0;i<numTests;i++)
		{
		if (theCaps1.iRate&KSpeeds[i].iMask && theCaps2.iRate&KSpeeds[i].iMask)
			{
			mess.Format(_L("read/write @ %s Bps"),KSpeeds[i].iName);
			test.Next(mess);
			c1.iRate=KSpeeds[i].iSpeed;
			TInt r=theSerialPorts[0]->SetConfig(cBuf1);
			test(r==KErrNone);
			c2.iRate=KSpeeds[i].iSpeed;
			r=theSerialPorts[1]->SetConfig(cBuf2);
			test(r==KErrNone);
			test.Printf(_L("DoCheckedWrite\r\n"));
			test(CheckedWrite(KWriteSize)==KWriteSize);
			}
		else
			{
			mess.Format(_L("%s Bps not supported"),KSpeeds[i].iName);
			test.Next(mess);
			}
		}

	theSerialPorts[0]->Close();
	theSerialPorts[1]->Close();

	test.End();
	}

void testTiming()
//
// Read and write at various speeds
	{

	test.Start(_L("Testing read and write speed"));
	const TInt KSamples=10;
	const TInt KNumWrites=100;
	const TInt KBufSize=2000;
	test.Printf(_L("%d sets of %d by %d characters @ 19200\n\r"),KSamples,KNumWrites,KBufSize);

	TCommConfig cBuf1;
	TCommConfigV01& c1=cBuf1();
	theSerialPorts[0]->Config(cBuf1);
	TCommConfig cBuf2;
	TCommConfigV01& c2=cBuf2();
	theSerialPorts[0]->Config(cBuf2);
	c1.iHandshake=0;
	c2.iHandshake=0;
	c2.iFifo=EFifoDisable;

	c2.iDataBits=c1.iDataBits=EData8;
	c2.iStopBits=c1.iStopBits=EStop1;
	c2.iParity=c1.iParity=EParityNone;
	c2.iRate=c1.iRate=EBps19200;

	TInt r=theSerialPorts[0]->SetConfig(cBuf1);
	test(r==KErrNone);
	r=theSerialPorts[1]->SetConfig(cBuf2);
	test(r==KErrNone);

	TUint samples[KSamples];

	for (TInt i=0;i<KSamples;i++)
		{
		test.Printf(_L("."));

		TUint8* inBuf=new TUint8[KBufSize];
		TUint8* outBuf=new TUint8[KBufSize];
		TPtr8 outDes(outBuf,KBufSize,KBufSize);
		TPtr8 inDes(inBuf,KBufSize,KBufSize);


		RTimer tim;
		tim.CreateLocal();
		TRequestStatus readStatus;
		TRequestStatus timeStatus;

		StripeMem(outDes,'A','Z');
		inDes.FillZ();

		TTime startTime;
		startTime.HomeTime();
		for (TInt l=0;l<KNumWrites;l++)
			{
			inDes.SetLength(KBufSize/3);
			theSerialPorts[0]->Read(readStatus,inDes);

			TInt ret=theSerialPorts[1]->WriteS(outDes,KBufSize);
			const TUint KTimeOut=6000000;
			tim.After(timeStatus,KTimeOut);

			User::WaitForRequest(readStatus,timeStatus);


			if (timeStatus==KErrNone)
				{
				test.Printf(_L("Timed Out!\n\r"));
				theSerialPorts[0]->ReadCancel();
				}
			else
				{
				tim.Cancel();
				if (readStatus!=KErrNone)
						test.Printf(_L("Read Failed! (%d)\n\r"),readStatus.Int());
				test(readStatus==KErrNone);
				test(ret==KErrNone);
				test(inDes.Length()==inDes.MaxLength());
				test(inDes.Length()==KBufSize);
				test(CompareDescriptors(outDes,inDes));
				}

			}
		TTime endTime;
		endTime.HomeTime();

		TInt64 delta=endTime.MicroSecondsFrom(startTime).Int64();
		delta=delta/1000;
		TInt delta32=I64INT(delta);
		samples[i]=delta32;
		test.Printf(_L("Read/Write %d time = %d ms\n\r"),KNumWrites*KBufSize,delta32);
		}

	TInt avg=0;
	for (TInt j=0;j<KSamples;j++)
		{
		avg=avg+samples[j];
		}
	avg/=KSamples;
	test.Printf(_L("      Average time = %d ms\n\r"),avg);
	test.Printf(_L("Press a key\n\r"));
	test.Getch();

	test.End();
	}

void testBreak()
///
/// Tests serial breaks
///
 	{
	TBuf<256> msg;
	test.Next(_L("Testing breaks"));

	TCommConfig cBuf0;
	TCommConfigV01& c0=cBuf0();
	TCommConfig cBuf1;
	TCommConfigV01& c1=cBuf1();

	TRequestStatus breakStatus;
	TRequestStatus readStatus;
	TRequestStatus writeStatus;
	TRequestStatus timerStatus;

	TInt r=theSerialPorts[0]->Open(PortA);
	test(r==KErrNone);
	r=theSerialPorts[0]->QueryReceiveBuffer();
	test(r==0);
	r=theSerialPorts[1]->Open(PortB);
	test(r==KErrNone);
	r=theSerialPorts[1]->QueryReceiveBuffer();
	test(r==0);

	theSerialPorts[0]->Config(cBuf0);
	theSerialPorts[1]->Config(cBuf1);
	c0.iRate=c1.iRate=EBps110;
	c0.iParityError=c1.iParityError=0;
	c0.iHandshake=c1.iHandshake=0;

	c0.iDataBits=c1.iDataBits=EData8;
	c0.iStopBits=c1.iStopBits=EStop1;
	c0.iParity=c1.iParity=EParityNone;


	r=theSerialPorts[0]->SetConfig(cBuf0);
	test(r==KErrNone);
	r=theSerialPorts[1]->SetConfig(cBuf1);
	test(r==KErrNone);

for(TUint i = 0; i < ELEMENTS(KBreakSpeeds) ; ++i)
		{
		if (theCaps1.iRate&KBreakSpeeds[i].iMask && theCaps2.iRate&KBreakSpeeds[i].iMask)
			{
			msg.Format(_L("Break tests @ %s Bps"), KBreakSpeeds[i].iName);
			test.Start(msg);

			c0.iRate=KBreakSpeeds[i].iSpeed;
			TInt r=theSerialPorts[0]->SetConfig(cBuf0);
			test(r==KErrNone);
			c1.iRate=KBreakSpeeds[i].iSpeed;
			r=theSerialPorts[1]->SetConfig(cBuf1);
			test(r==KErrNone);


			// should take more than 1.5s

			HBufC8* bigReadBuffer=HBufC8::NewL(KWriteSize);
			HBufC8* bigWriteBuffer=HBufC8::NewMaxL(KWriteSize);
			TPtr8 bigReadBufferPtr(bigReadBuffer->Des());
			TPtr8 bigWriteBufferPtr(bigWriteBuffer->Des());

			StripeMem(bigWriteBufferPtr, 'A', 'Z');
			bigReadBufferPtr.FillZ();


			const TUint KWriteSize=1024 + KBreakSpeeds[i].iBps/4;
			const TInt KTimerTime=1500000;
			const TInt KBreakTime=3000000;
			const TInt KMinTurnaroundTime=150000;

			RTimer timer;
			test(timer.CreateLocal()==KErrNone);


// Test 1
			test.Start(_L("Break after write"));
		//- start a user timer which will expire just after the TX would complete with no break
			timer.After(timerStatus, KTimerTime);

		//- request TX (and RX) and request a break
			theSerialPorts[0]->Write(writeStatus, *bigWriteBuffer, KWriteSize);
			theSerialPorts[0]->Break(breakStatus, KBreakTime);
			theSerialPorts[1]->Read(readStatus, bigReadBufferPtr, KWriteSize);

		// Make sure the timer completes first
			User::WaitForRequest(writeStatus, readStatus);
			User::WaitForRequest(breakStatus);

			test(readStatus!=KErrNone && readStatus!=KRequestPending);
			test(breakStatus==KErrNone);
			test(writeStatus==KErrNone || writeStatus==KRequestPending);	// Can be still pending, since if the read is completed with an error then the write won't complete since the buffers may fill up
			test(timerStatus==KErrNone);

			if (writeStatus==KRequestPending)
				theSerialPorts[0]->WriteCancel();

// Test 2
			test.Next(_L("Write after break"));
		//- start a user timer which will expire just after the TX would complete with no break
			timer.After(timerStatus, KTimerTime);

		//- request TX (and RX) and request a break
			theSerialPorts[1]->Read(readStatus, bigReadBufferPtr, KWriteSize);
			theSerialPorts[0]->Break(breakStatus, KBreakTime);
			theSerialPorts[0]->Write(writeStatus, *bigWriteBuffer, KWriteSize);

		// Make sure the timer completes first
			User::WaitForRequest(breakStatus);
			User::WaitForRequest(writeStatus, readStatus);

			test(readStatus!=KErrNone && readStatus!=KRequestPending);
			test(breakStatus==KErrNone);
			test(writeStatus==KErrNone || writeStatus==KRequestPending);	// write may not be able to cmoplete due to no remaining pending read
			test(timerStatus==KErrNone);

			if (writeStatus==KRequestPending)
				theSerialPorts[0]->WriteCancel();

// Test 3
			test.Next(_L("Cancellation of break"));
		//- Check cancellation of breaks

		//- request TX (and RX) and request a break
			theSerialPorts[0]->Break(breakStatus, KBreakTime);
			theSerialPorts[1]->Read(readStatus, bigReadBufferPtr, KWriteSize);
			theSerialPorts[0]->Write(writeStatus, *bigWriteBuffer, KWriteSize);

		//- cancel break
			theSerialPorts[0]->BreakCancel();

			User::WaitForRequest(breakStatus);
			test(breakStatus.Int()==KErrCancel);

			User::WaitForRequest(readStatus);

			if (writeStatus==KRequestPending)
				theSerialPorts[0]->WriteCancel();

// Test 4

			test.Next(_L("Break during turnaround"));
		//- Check break still works during turnaround
			test (KErrNone==theSerialPorts[0]->SetMinTurnaroundTime(KMinTurnaroundTime));

			theSerialPorts[0]->Read(readStatus, bigReadBufferPtr, 1);
			theSerialPorts[1]->Write(writeStatus, *bigWriteBuffer, 1);
			User::WaitForRequest(readStatus);
			User::WaitForRequest(writeStatus);

		//- start a user timer which will expire just after the TX would complete with no break
			timer.After(timerStatus, KTimerTime);

		//- request TX (and RX) and request a break
			theSerialPorts[0]->Break(breakStatus, KBreakTime);
			theSerialPorts[1]->Read(readStatus, bigReadBufferPtr, KWriteSize);
			theSerialPorts[0]->Write(writeStatus, *bigWriteBuffer, KWriteSize);

		// Make sure the timer completes first
			User::WaitForRequest(writeStatus, readStatus);
			User::WaitForRequest(breakStatus);

			test(readStatus!=KErrNone && readStatus!=KRequestPending);
			test(breakStatus==KErrNone);
			test(writeStatus==KErrNone || writeStatus==KRequestPending);
			test(timerStatus==KErrNone);

			if (writeStatus==KRequestPending)
				theSerialPorts[0]->WriteCancel();

			test (KErrNone==theSerialPorts[0]->SetMinTurnaroundTime(0));

// End tests
			timer.Close();
			test.End();
			test.End();
			}
		else
			{
			msg.Format(_L("%s Bps not supported"),KBreakSpeeds[i].iName);
			test.Next(msg);
			}
		}	// end rate loop

	theSerialPorts[0]->Close();
	theSerialPorts[1]->Close();
	}



void testFraming()
//
// Test framing
//
	{
	test.Start(_L("Testing framing"));

	TInt r=theSerialPorts[0]->Open(PortA);
	test(r==KErrNone);
	r=theSerialPorts[0]->QueryReceiveBuffer();
	test(r==0);
	r=theSerialPorts[1]->Open(PortB);
	test(r==KErrNone);
	r=theSerialPorts[1]->QueryReceiveBuffer();
	test(r==0);

	TCommConfig cBuf0;
	TCommConfigV01& c0=cBuf0();
	TCommConfig cBuf1;
	TCommConfigV01& c1=cBuf1();
	TBuf<0x40> mess;

	theSerialPorts[0]->Config(cBuf0);
	c0.iRate=EBps9600;
	c0.iHandshake=0;
	theSerialPorts[1]->Config(cBuf1);
	c1.iRate=EBps9600;
	c1.iHandshake=0;

	TInt numTests=sizeof(KFrameTypes)/sizeof(TFrameAndName);
	for (TInt i=0;i<numTests;i++)
		{
		c0.iDataBits=KFrameTypes[i].iData;
		c0.iStopBits=KFrameTypes[i].iStop;
		c0.iParity=KFrameTypes[i].iParity;
		TInt r=theSerialPorts[0]->SetConfig(cBuf0);
		if (r==KErrNone)
			{

			c1.iDataBits=KFrameTypes[i].iData;
			c1.iStopBits=KFrameTypes[i].iStop;
			c1.iParity=KFrameTypes[i].iParity;
			r=theSerialPorts[1]->SetConfig(cBuf1);
			if(r==KErrNone)
				{
				mess.Format(_L("read/write using %s "),KFrameTypes[i].iName);
				test.Next(mess);
				test(CheckedWrite(KWriteSize)==KWriteSize);
				}
			}

		if (r!=KErrNone)
			test.Printf(_L("%s not supported\n\r"),KFrameTypes[i].iName);
		}

	theSerialPorts[0]->Close();
	theSerialPorts[1]->Close();
	test.End();
	}
//
void testTerminators()
//
// Test termination masks - assumes that Checked write stripes memory starting with 'A'
//
	{

	test.Next(_L("Testing termination masks"));

	TCommConfig cBuf;
	TCommConfigV01& c=cBuf();

	theSerialPorts[0]->Close();
	theSerialPorts[1]->Close();

	TInt r=theSerialPorts[0]->Open(PortA);
	test(r==KErrNone);
	r=theSerialPorts[0]->QueryReceiveBuffer();
	test(r==0);
	r=theSerialPorts[1]->Open(PortB);
	test(r==KErrNone);
	r=theSerialPorts[1]->QueryReceiveBuffer();
	test(r==0);
	theSerialPorts[0]->Config(cBuf);
	c.iTerminator[0]='C';
	c.iTerminatorCount=1;
	c.iHandshake=0;

	c.iDataBits=EData8;
	c.iStopBits=EStop1;
	c.iParity=EParityNone;

	r=theSerialPorts[0]->SetConfig(cBuf);
	test(r==KErrNone);

	TCommConfig cBuf1;
	TCommConfigV01& c1=cBuf1();
	theSerialPorts[1]->Config(cBuf1);

	c1.iDataBits=EData8;
	c1.iStopBits=EStop1;
	c1.iParity=EParityNone;

	c1.iTerminator[0]='C';
	c1.iTerminatorCount=1;
	c1.iHandshake=0;

	r=theSerialPorts[1]->SetConfig(cBuf1);
	test(r==KErrNone);

	User::After(100000);
	theSerialPorts[0]->ResetBuffers();
	theSerialPorts[1]->ResetBuffers();

	test(CheckedWrite(KWriteSize)==3);

	// Clear the ldd buffers
	theSerialPorts[0]->ResetBuffers();
	theSerialPorts[1]->ResetBuffers();

	c.iTerminator[0]='Z';
	c.iTerminator[1]='X';
	c.iTerminator[2]='Y';
	c.iTerminator[3]='D';

	c1.iTerminator[0]='Z';
	c1.iTerminator[1]='X';
	c1.iTerminator[2]='Y';
	c1.iTerminator[3]='D';

/* 	Not yet - we have too much buffering in the driver & device.
	Unfortunately the ResetBuffers() above doesn't (and really can't) go
	deep enough. Under WINS NT buffers up some data and the following read
	(inside checked write) completes before the WriteS (and infact, after
	reading a semi random number of characters)

    c.iTerminatorCount=4;
    c1.iTerminatorCount=4;
	r=theSerialPorts[0]->SetConfig(cBuf);
	test(r==KErrNone);
	r=theSerialPorts[1]->SetConfig(cBuf1);
	test(r==KErrNone);

	test(CheckedWrite(KWriteSize)==4);
	theSerialPorts[0]->Config(cBuf);
*/
	// Reset termination mask.
	c.iTerminatorCount=0;
	c1.iTerminatorCount=0;
	r=theSerialPorts[0]->SetConfig(cBuf);
	test(r==KErrNone);
	r=theSerialPorts[1]->SetConfig(cBuf1);
	test(r==KErrNone);

	theSerialPorts[0]->Close();
	theSerialPorts[1]->Close();
	}

void testXonXoff()
//
// tests XonXoff
//
	{
	test.Next(_L("Testing xon xoff"));
	test.Start(_L("Setup"));

	TInt r=theSerialPorts[0]->Open(PortA);
	test(r==KErrNone);
	r=theSerialPorts[0]->QueryReceiveBuffer();
	test(r==0);
	r=theSerialPorts[1]->Open(PortB);
	test(r==KErrNone);
	r=theSerialPorts[1]->QueryReceiveBuffer();
	test(r==0);

	TCommConfig cBuf;
	TCommConfigV01& c=cBuf();

	theSerialPorts[0]->Config(cBuf);
	c.iHandshake=KConfigObeyXoff|KConfigSendXoff;
	c.iStopBits=EStop1;
	c.iParity=EParityNone;
	c.iDataBits=EData8;
	c.iRate=EBps19200;
	c.iXonChar=0x11;
	c.iXoffChar=0x13;
	c.iParityError=KConfigXonXoffDebug;
	c.iTerminatorCount=0;
	test(theSerialPorts[0]->SetConfig(cBuf)==KErrNone);

	theSerialPorts[1]->Config(cBuf);
	c.iHandshake=KConfigObeyXoff|KConfigSendXoff;
	c.iStopBits=EStop1;
	c.iParity=EParityNone;
	c.iDataBits=EData8;
	c.iRate=EBps19200;
	c.iXonChar=0x11;
	c.iXoffChar=0x13;
	c.iParityError=KConfigXonXoffDebug;
	c.iTerminatorCount=0;
	test(theSerialPorts[1]->SetConfig(cBuf)==KErrNone);

	theSerialPorts[0]->SetReceiveBufferLength(0x400);
	theSerialPorts[1]->SetReceiveBufferLength(0x400);

	const TInt KXonWriteSize=KXonNumReads*KXonReadSize;

	TUint8* inBuf=new TUint8[KXonReadSize];
	TUint8* outBuf=new TUint8[KXonWriteSize];
	TPtr8 outDes(outBuf,KXonWriteSize,KXonWriteSize);
	TPtr8 inDes(inBuf,KXonReadSize,KXonReadSize);

	TRequestStatus readStatus;
	TRequestStatus writeStatus;
	TRequestStatus timeStatus;
	RTimer timer;
	timer.CreateLocal();
	TInt writePos=0;

	StripeMem(outDes,'A','Z');
	inDes.FillZ();

	test.Next(_L("Write bytes to com1"));
	test.Printf(_L("Reading after delay (1 of %d) avail = %d\n\r"),KXonNumReads, theSerialPorts[0]->QueryReceiveBuffer());
	theSerialPorts[0]->Read(readStatus,inDes,KXonReadSize);
	theSerialPorts[1]->Write(writeStatus,outDes,KXonWriteSize);
	timer.After(timeStatus,1000000);
	User::WaitForRequest(readStatus,timeStatus);
	test(readStatus==KErrNone);
	test(timeStatus==KRequestPending);
	TPtrC8 aOutDes = outDes.Mid(writePos,KXonReadSize);
	test(CompareDescriptors(inDes,(TDes8&)aOutDes));

	writePos+=KXonReadSize;

	if (timeStatus==KRequestPending)
		User::WaitForRequest(timeStatus);

	TInt i;
	for (i=0;i<KXonNumReads-1;++i)
		{
		inDes.FillZ();
		timer.After(timeStatus,450000);
		User::WaitForRequest(timeStatus);
		test(timeStatus==KErrNone);

		test.Printf(_L("Reading after delay (%d of %d) avail = %d\n\r"),i+2,KXonNumReads, theSerialPorts[0]->QueryReceiveBuffer());
		theSerialPorts[0]->Read(readStatus,inDes,KXonReadSize);
		timer.After(timeStatus,1000000);
		User::WaitForRequest(readStatus,timeStatus);
		if (readStatus!=KErrNone)
			test.Printf(_L("Read error %d\n\r"),readStatus.Int());
		test(readStatus==KErrNone);
		test(timeStatus==KRequestPending);
		TPtrC8 aOutDes = outDes.Mid(writePos,KXonReadSize);
		test(CompareDescriptors(inDes,(TDes8&)aOutDes));
		timer.Cancel();
		writePos+=KXonReadSize;
		}

	test.Next(_L("2nd Large Write complete"));
	test(writeStatus==KErrNone);

	delete [] inBuf;
	delete [] outBuf;

	theSerialPorts[0]->Close();
	theSerialPorts[1]->Close();

	test.End();
	}

//
void testHWHandshaking()
//
// test hardware hand shaking
//
	{

#if defined (__WINS__)
const TInt KHWReadSize=0x2000;
#else
const TInt KHWReadSize=0x400;
#endif

	test.Start(_L("Testing hardware handshaking"));

	TInt r=theSerialPorts[0]->Open(PortA);
	test(r==KErrNone);
	r=theSerialPorts[0]->QueryReceiveBuffer();
	test(r==0);
	r=theSerialPorts[1]->Open(PortB);
	test(r==KErrNone);
	r=theSerialPorts[1]->QueryReceiveBuffer();
	test(r==0);

	TCommConfig cBuf0;
	TCommConfigV01& c0=cBuf0();
	TBuf<0x40> mess;
	theSerialPorts[0]->Config(cBuf0);
	c0.iRate=EBps115200;
	c0.iParityError=0;
	c0.iHandshake=0;

	TCommConfig cBuf1;
	TCommConfigV01& c1=cBuf1();
	test(theSerialPorts[0]->SetConfig(cBuf0)==KErrNone);

	theSerialPorts[1]->Config(cBuf1);
	c1.iRate=EBps115200;
	c1.iParityError=0;
	c1.iHandshake=0;
	test(theSerialPorts[1]->SetConfig(cBuf1)==KErrNone);

	const TInt KXonWriteSize=KXonNumReads*KHWReadSize;

	TUint8* inBuf=new TUint8[KHWReadSize];
	TUint8* outBuf=new TUint8[KXonWriteSize];
	TPtr8 outDes(outBuf,KXonWriteSize,KXonWriteSize);
	TPtr8 inDes(inBuf,KHWReadSize,KHWReadSize);

//TUint8* inBuf2=new TUint8[KXonWriteSize];
//TPtr8 inDes2(inBuf2,KXonWriteSize,KXonWriteSize);

#if defined (__WINS__)
	theSerialPorts[0]->SetReceiveBufferLength(0x50);
	theSerialPorts[1]->SetReceiveBufferLength(0x50);
#else
	theSerialPorts[0]->SetReceiveBufferLength(0x400);
	theSerialPorts[1]->SetReceiveBufferLength(0x400);
#endif

	TInt numTests=sizeof(KHandshakes)/sizeof(THandShakeAndName);
	for(TInt j=0;j<numTests;j++)
		{
		mess.Format(_L("read/write using %s "),KHandshakes[j].iName);
		test.Next(mess);
		c0.iHandshake=c1.iHandshake=KHandshakes[j].iHandshake;

		if((theCaps1.iHandshake & KHandshakes[j].iHandshake)
			&& (theCaps2.iHandshake & KHandshakes[j].iHandshake))
			{
			test(theSerialPorts[0]->SetConfig(cBuf0)==KErrNone);
			test(theSerialPorts[1]->SetConfig(cBuf1)==KErrNone);
			TRequestStatus readStatus;
			TRequestStatus writeStatus;

			StripeMem(outDes,'A','Z');
			inDes.FillZ();

			theSerialPorts[1]->Write(writeStatus,outDes,KXonWriteSize);

//TRequestStatus writeStatus2;
//theSerialPorts[0]->Write(writeStatus2,outDes,KXonWriteSize);

			TInt i;
			for (i=0;i<KXonNumReads;i++)
				{
				inDes.FillZ();
#if defined (__WINS__)
				User::After(600000);
#else
				User::After(300000);
#endif
				test.Printf(_L("Reading %d after delay (%d of %d) avail = %d\r\n"),KHWReadSize, i+1,KXonNumReads, theSerialPorts[0]->QueryReceiveBuffer());
				theSerialPorts[0]->Read(readStatus,inDes,KHWReadSize);
				User::WaitForRequest(readStatus);
				test(readStatus==KErrNone);
				TPtrC8 aOutDes = outDes.Mid(KHWReadSize*i,KHWReadSize);
				test(CompareDescriptors(inDes,(TDes8&)aOutDes));
				test(inDes.Length()==KHWReadSize);
				}

			test.Next(_L("2nd Large Write complete"));
			User::WaitForRequest(writeStatus);
			test(writeStatus==KErrNone);

//theSerialPorts[1]->Read(readStatus,inDes2,KXonWriteSize);
//User::WaitForRequest(writeStatus2);
//test(writeStatus2==KErrNone);

//User::WaitForRequest(readStatus);
//test(readStatus==KErrNone);

			}
		else
			{
			test.Printf(_L("Config not supported\r\n"));
			}
		}
	delete [] inBuf;
	delete [] outBuf;

	theSerialPorts[0]->Close();
	theSerialPorts[1]->Close();

	test.End();
	}

void testWriteZero()
//
// Test a write of 0 bytes is still blocked by CTS flow control. 
// Test does a flow controlled Write(0) which is blocked by the remote 
// port state being closed (hence remote RTS disasserted, hence writer's 
// CTS likewise). Then it opens the remote port and asserts RTS - this 
// unblocks the original Write(0). 
// 
	{
	TInt r=theSerialPorts[0]->Open(PortA);
	test(r==KErrNone);
	r=theSerialPorts[0]->QueryReceiveBuffer();
	test(r==0);

	test.Next(_L("Testing Write 0"));

	TCommConfig cBuf1;
	TCommConfigV01& c1=cBuf1();
	theSerialPorts[0]->Config(cBuf1);
	c1.iRate=EBps19200;
	c1.iParityError=0;
	c1.iHandshake=KConfigObeyCTS;

	r=theSerialPorts[0]->SetConfig(cBuf1);
	test(r==KErrNone);

	test.Start(_L("Test Write(0) with remote RTS disasserted blocks"));
	TRequestStatus writeStat;
	theSerialPorts[0]->Write(writeStat,TPtr8(NULL,0),0);

	RTimer timer;
	timer.CreateLocal();
	TRequestStatus timeStatus;
	timer.After(timeStatus,1000000);
	User::WaitForRequest(timeStatus,writeStat);

	test(timeStatus==KErrNone);
	test(writeStat==KRequestPending);

	r=theSerialPorts[1]->Open(PortB);
	test(r==KErrNone);
	r=theSerialPorts[1]->QueryReceiveBuffer();
	test(r==0);

	TCommConfig cBuf2;
	TCommConfigV01& c2=cBuf2();
	theSerialPorts[1]->Config(cBuf2);
	c2.iRate=EBps19200;
	c2.iParityError=0;
	c2.iHandshake |= KConfigFreeRTS;
	r=theSerialPorts[1]->SetConfig(cBuf2);
	test(r==KErrNone);

	test.Next(_L("Test Write(0) with remote RTS asserted completes"));
	timer.After(timeStatus,10000000);
	theSerialPorts[1]->SetSignals(KSignalRTS,0);

	User::WaitForRequest(timeStatus,writeStat);
	if (writeStat==KRequestPending)
		test.Printf(_L("     Timed out!\n"));
	User::After(2000000);

	test(writeStat==KErrNone);
	test(timeStatus==KRequestPending);

	timer.Cancel();

	theSerialPorts[0]->Close();
	theSerialPorts[1]->Close();

	test.End();
	}


void testSingleCharacterReads()
//
// Test reading one character at a time.
//
	{
	const TInt KWriteSize=100;
	test.Start(_L("Test partial reads"));

	TInt r=theSerialPorts[0]->Open(PortA);
	test(r==KErrNone);
	r=theSerialPorts[0]->QueryReceiveBuffer();
	test(r==0);
	r=theSerialPorts[1]->Open(PortB);
	test(r==KErrNone);
	r=theSerialPorts[1]->QueryReceiveBuffer();
	test(r==0);

	TCommConfig cBuf0;
	TCommConfigV01& c0=cBuf0();
	theSerialPorts[0]->Config(cBuf0);

	TCommConfig cBuf1;
	TCommConfigV01& c1=cBuf1();
	theSerialPorts[1]->Config(cBuf1);

	c0.iRate=c1.iRate=EBps9600;
	c0.iParityError=c1.iParityError=0;
	c0.iHandshake=c1.iHandshake=KConfigObeyCTS;

	c0.iDataBits=c1.iDataBits=EData8;
	c0.iStopBits=c1.iStopBits=EStop1;
	c0.iParity=c1.iParity=EParityNone;

	r=theSerialPorts[0]->SetConfig(cBuf0);
	test(r==KErrNone);
	r=theSerialPorts[1]->SetConfig(cBuf1);
	test(r==KErrNone);
	test.Printf(_L("Setconfig OK\r\n"));

	TInt bufSiz=KWriteSize+3+(KWriteSize/2);

	r=theSerialPorts[0]->SetReceiveBufferLength(bufSiz);
	if (r!=KErrNone)
		test.Printf(_L("Setting buffers to %d bytes for com0 failed %d\n\r"),bufSiz,r);
	r=theSerialPorts[1]->SetReceiveBufferLength(bufSiz+1);
	if (r!=KErrNone)
		test.Printf(_L("Setting buffers to %d bytes for com1 failed %d\n\r"),bufSiz,r);

	TUint8* singleCharReadBuf=new TUint8[1];
	test(singleCharReadBuf!=NULL);
	TPtr8 singleCharReadDes(singleCharReadBuf,1,1);
	TUint8* multiCharWriteBuf=new TUint8[KWriteSize];
	test(multiCharWriteBuf!=NULL);
	TPtr8 multiCharWriteDes(multiCharWriteBuf,KWriteSize,KWriteSize);
	multiCharWriteDes.Fill('m');

	RTimer tim;
	tim.CreateLocal();

	for (TInt j=0;j<2;j++)
		{
		TInt readPort=0;
		TInt writePort=0;
		readPort=1-j;
		writePort=j;

		TBuf<256> message;
		message.Format(_L("Reading single chars from port %d, writing %d to port %d"),readPort,multiCharWriteDes.Length(),writePort);
		test.Next(message);

		TRequestStatus readZeroStat;
		theSerialPorts[readPort]->Read(readZeroStat,singleCharReadDes,0);//a zero length read completes immediately and
		User::WaitForRequest(readZeroStat);								 //will wake up the receiver
		test.Printf(_L("Have done a read zero: %d\n\r"),readZeroStat.Int());
		User::After(1000000);

		TRequestStatus multiWriteStat;
		theSerialPorts[writePort]->Write(multiWriteStat,multiCharWriteDes);
//		User::WaitForRequest(multiWriteStat);
//		test.Printf(_L("Have done a write: %d\n\r"),multiWriteStat.Int());

		TRequestStatus timStat;
		TInt spin=0;
		for (TInt i=0;i<KWriteSize;i++)
			{
			tim.After(timStat,10000000);
			TRequestStatus readStat;
			singleCharReadDes.SetLength(0);
			theSerialPorts[readPort]->Read(readStat,singleCharReadDes);
			User::WaitForRequest(readStat,timStat);

			test.Printf(_L("r"));
			if (i%32==0)
				test.Printf(_L("\r%c"),KSpinner[spin++%4]);

			if (readStat!=KErrNone)
				{
				TBuf<256> message;
				if (readStat==KRequestPending)
					{
					message.Format(_L("\n\rRead timed out after %d chars (of %d)\n\r"),i,KWriteSize);
					/*if (multiWriteStat==KErrNone)
						{
						User::WaitForRequest(multiWriteStat);
						theSerialPorts[readPort]->ReadCancel();
						theSerialPorts[writePort]->Write(multiWriteStat,multiCharWriteDes);
						}*/
					}
				else
					if (readStat!=KErrOverflow && readStat!=KErrCommsOverrun)
						message.Format(_L("\n\rRead Failed %d after %d chars (of %d)\n\r"),readStat.Int(),i,KWriteSize);

				test.Printf(message);
				User::After(2000000);
				test(EFalse);
				}

			tim.Cancel();
			if (singleCharReadDes[0]!='m')
				{
				test.Printf(_L("Received character: 0x%02x\n"),singleCharReadDes[0]);
				test(EFalse);
				}
			}

		test.Printf(_L("Done\n\r"));

		tim.After(timStat,1000000);
		User::WaitForRequest(timStat,multiWriteStat);
		if (timStat.Int()==KErrNone)
			{
			test.Printf(_L("Lost at least one char!\n\r"));
			theSerialPorts[writePort]->WriteCancel();
			test(EFalse);
			}
		else
			{
			tim.Cancel();
			}
		}

	TUint8* singleCharWriteBuf=new TUint8[1];
	test(singleCharWriteBuf!=NULL);
	TPtr8 singleCharWriteDes(singleCharWriteBuf,1,1);
	singleCharWriteDes.Fill('s');
	TUint8* multiCharReadBuf=new TUint8[KWriteSize];
	test(multiCharReadBuf!=NULL);
	TPtr8 multiCharReadDes(multiCharReadBuf,KWriteSize,KWriteSize);

	for (TInt k=0;k<2;k++)
		{
		TInt readPort=0;
		TInt writePort=0;

		readPort=k;
		writePort=1-k;

		TRequestStatus multiReadStat;
		theSerialPorts[readPort]->Read(multiReadStat,multiCharReadDes);

		TBuf<256> message;
		message.Format(_L("Writing single chars to port %d"),readPort);
		test.Next(message);

		TRequestStatus timStat;
		TInt spin=0;
		for (TInt i=0;i<KWriteSize;i++)
			{
			TRequestStatus writeStat;
			tim.After(timStat,5000000);
			theSerialPorts[writePort]->Write(writeStat,singleCharWriteDes);
			User::WaitForRequest(writeStat,timStat);

			if ((i%32)==0)
				test.Printf(_L("\r%c"),KSpinner[spin++%4]);

			if (writeStat!=KErrNone)
				{
				TBuf<256> message;
				if (writeStat==KRequestPending)
					message.Format(_L("\n\rWrite timed out after %d chars (of %d)\n\r"),i,KWriteSize);
				else
					message.Format(_L("\n\rWrite Failed %d after %d chars (of %d)\n\r"),writeStat.Int(),i,KWriteSize);

				test.Printf(message);
				}
			test(writeStat==KErrNone);
			tim.Cancel();
			}
		test.Printf(_L("Done\n\r"));

		tim.After(timStat,1000000);
		User::WaitForRequest(timStat,multiReadStat);
		if (timStat.Int()==KErrNone)
			{
			test.Printf(_L("Lost at least one char!\n\r"));
			theSerialPorts[readPort]->ReadCancel();
			test(EFalse);
			}
		else
			{
			tim.Cancel();
			test(multiReadStat==KErrNone);
			test(multiCharWriteDes.Length()==multiCharWriteDes.MaxLength());
			}
		}

	test.End();
	tim.Close();

	delete [] multiCharWriteBuf;
	delete [] singleCharReadBuf;
	delete [] singleCharWriteBuf;
	delete [] multiCharReadBuf;

	theSerialPorts[0]->Close();
	theSerialPorts[1]->Close();
	}

void testBiDirectionalSingleCharacterReads()
//
// Test reading and writing one character at a time.
//
	{

	test.Start(_L("Test concurrent partial reads and writes"));

	TInt r=theSerialPorts[0]->Open(PortA);
	test(r==KErrNone);
	r=theSerialPorts[0]->QueryReceiveBuffer();
	test(r==0);
	r=theSerialPorts[1]->Open(PortB);
	test(r==KErrNone);
	r=theSerialPorts[1]->QueryReceiveBuffer();
	test(r==0);

	TCommConfig cBuf0;
	TCommConfigV01& c0=cBuf0();
	theSerialPorts[0]->Config(cBuf0);

	TCommConfig cBuf1;
	TCommConfigV01& c1=cBuf1();
	theSerialPorts[1]->Config(cBuf1);

	c0.iRate=c1.iRate=EBps9600;
	c0.iParityError=c1.iParityError=0;
	c0.iHandshake=c1.iHandshake=KConfigObeyCTS;

	c0.iDataBits=c1.iDataBits=EData8;
	c0.iStopBits=c1.iStopBits=EStop1;
	c0.iParity=c1.iParity=EParityNone;

	r=theSerialPorts[0]->SetConfig(cBuf0);
	test(r==KErrNone);
	r=theSerialPorts[1]->SetConfig(cBuf1);
	test(r==KErrNone);

	const TInt KWriteSize=4000;
	TUint8* singleCharReadBuf=new TUint8[1];
	test(singleCharReadBuf!=NULL);
	TPtr8 singleCharReadDes(singleCharReadBuf,1,1);
	TUint8* multiCharWriteBuf=new TUint8[KWriteSize];
	test(multiCharWriteBuf!=NULL);
	TPtr8 multiCharWriteDes(multiCharWriteBuf,KWriteSize,KWriteSize);
	multiCharWriteDes.Fill('m');
	TUint8* singleCharWriteBuf=new TUint8[1];
	test(singleCharWriteBuf!=NULL);
	TPtr8 singleCharWriteDes(singleCharWriteBuf,1,1);
	singleCharWriteDes.Fill('s');
	TUint8* multiCharReadBuf=new TUint8[KWriteSize];
	test(multiCharReadBuf!=NULL);
	TPtr8 multiCharReadDes(multiCharReadBuf,KWriteSize,KWriteSize);

	TRequestStatus multiWriteStat;
	TRequestStatus multiReadStat;
	theSerialPorts[0]->Write(multiWriteStat,multiCharWriteDes);
	theSerialPorts[0]->Read(multiReadStat,multiCharReadDes);

	TInt spin=0;
	for (TInt i=0;i<KWriteSize;i++)
		{
		if (i%32==0)
			test.Printf(_L("\r%c"),KSpinner[spin++%4]);

		TRequestStatus readStat;
		TRequestStatus writeStat;
		theSerialPorts[1]->Read(readStat,singleCharReadDes);
		theSerialPorts[1]->Write(writeStat,singleCharWriteDes);
		User::WaitForRequest(readStat);
		User::WaitForRequest(writeStat);

		if (readStat!=KErrNone)
			{
			test.Printf(_L("Read Failed %d after %d chars\n\r"),readStat.Int(),i);
			test(EFalse);
			}
		if (writeStat!=KErrNone)
			{
			test.Printf(_L("Write Failed %d after %d chars\n\r"),writeStat.Int(),i);
			test(EFalse);
			}
		}

	test.Printf(_L("\n\r"));

	RTimer tim;
	tim.CreateLocal();
	TRequestStatus timStat;
	tim.After(timStat,3000000);
	User::WaitForRequest(multiWriteStat,timStat);
	test(timStat==KRequestPending);
	tim.Cancel();
	User::WaitForRequest(timStat);
	test(timStat==KErrCancel);
	test(multiWriteStat==KErrNone);
	tim.After(timStat,3000000);
	User::WaitForRequest(multiReadStat,timStat);
	test(timStat==KRequestPending);
	tim.Cancel();
	tim.Close();
	User::WaitForRequest(timStat);
	test(timStat==KErrCancel);
	test(multiReadStat==KErrNone);
	test(multiCharWriteDes.Length()==multiCharWriteDes.MaxLength());

	test.End();

	delete [] multiCharWriteBuf;
	delete [] singleCharReadBuf;
	delete [] singleCharWriteBuf;
	delete [] multiCharReadBuf;

	theSerialPorts[0]->Close();
	theSerialPorts[1]->Close();
	}

void testMultiTerminatorCompletion()
//
// Test multiple terminator completions
//
	{
	test.Next(_L("Test partial reads with terminators"));

	TInt r=theSerialPorts[0]->Open(PortA);
	test(r==KErrNone);
	r=theSerialPorts[0]->QueryReceiveBuffer();
	test(r==0);
	r=theSerialPorts[1]->Open(PortB);
	test(r==KErrNone);
	r=theSerialPorts[1]->QueryReceiveBuffer();
	test(r==0);

	TCommConfig cBuf0;
	TCommConfigV01& c0=cBuf0();
	theSerialPorts[0]->Config(cBuf0);
	TCommConfig cBuf1;
	TCommConfigV01& c1=cBuf1();
	theSerialPorts[1]->Config(cBuf1);

	c0.iRate=c1.iRate=EBps9600;
	c0.iParityError=c1.iParityError=0;

	c0.iHandshake=c1.iHandshake=KConfigObeyCTS;

	c0.iDataBits=c1.iDataBits=EData8;
	c0.iStopBits=c1.iStopBits=EStop1;
	c0.iParity=c1.iParity=EParityNone;

	r=theSerialPorts[0]->SetConfig(cBuf0);
	test(r==KErrNone);
	c1.iTerminator[0]='a';
	c1.iTerminatorCount=1;
	r=theSerialPorts[1]->SetConfig(cBuf1);
	test(r==KErrNone);
	const TInt KWriteSize=4000;
	TUint8* writeBuf=new TUint8[KWriteSize];
	test(writeBuf!=NULL);
	TPtr8 writeDes(writeBuf,KWriteSize,KWriteSize);
	writeDes.Fill('a');
	TUint8* readBuf=new TUint8[KWriteSize];
	test(readBuf!=NULL);
	TPtr8 readDes(readBuf,KWriteSize,KWriteSize);
	TRequestStatus writeStat;
	theSerialPorts[0]->Write(writeStat,writeDes);
	test(writeStat==KRequestPending);
	TInt spin=0;
	for (TInt i=0;i<KWriteSize;i++)
		{
		if (i%32==0)
			test.Printf(_L("\r%c"),KSpinner[spin++%4]);
		TRequestStatus readStat;
		readDes.SetLength(KWriteSize/2);
		theSerialPorts[1]->Read(readStat,readDes);
		User::WaitForRequest(readStat);
		test(readStat==KErrNone);
		test(readDes.Length()==1);
		}
	test.Printf(_L("\n\r"));
	User::WaitForRequest(writeStat);

	delete [] readBuf;
	delete [] writeBuf;

	theSerialPorts[0]->Close();
	theSerialPorts[1]->Close();
	}

void TestSimpleWriting()
	{
	test.Next(_L("Test we can still write 0->1"));
	const TPtrC8 string1=_L8("If you strike me down, I shall become more powerful than you can possibly imagine.");
	TBuf8<100> inBuf;
	TRequestStatus stat;
	theSerialPorts[1]->Read(stat,inBuf,string1.Length());
	test(stat==KRequestPending);
	TInt r=theSerialPorts[0]->WriteS(string1,string1.Length());
	test(r==KErrNone);
	User::WaitForRequest(stat);
	test(stat==KErrNone);
	test(inBuf==string1);

	test.Next(_L("Test we can still write 1->0"));
	const TPtrC8 string2=_L8("Who's the more foolish... the fool or the fool who follows him?");
	theSerialPorts[0]->Read(stat,inBuf,string2.Length());
	test(stat==KRequestPending);
	r=theSerialPorts[1]->WriteS(string2,string2.Length());
	test(r==KErrNone);
	User::WaitForRequest(stat);
	test(stat==KErrNone);
	test(inBuf==string2);
	}

void TestPower()
	{
	test.Next(_L("Power up and down"));

	TInt r=theSerialPorts[0]->Open(PortA);
	test(r==KErrNone);
	r=theSerialPorts[0]->QueryReceiveBuffer();
	test(r==0);
	r=theSerialPorts[1]->Open(PortB);
	test(r==KErrNone);
	r=theSerialPorts[1]->QueryReceiveBuffer();
	test(r==0);

	test.Start(_L("Power down while writing 0->1"));
	TCommConfig cBuf1;
	TCommConfigV01& c1=cBuf1();
	theSerialPorts[0]->Config(cBuf1);
	TCommConfig cBuf2;
	TCommConfigV01& c2=cBuf2();
	theSerialPorts[1]->Config(cBuf2);
	c1.iFifo=EFifoEnable;

	c2.iDataBits=c1.iDataBits=EData8;
	c2.iStopBits=c1.iStopBits=EStop1;
	c2.iParity=c1.iParity=EParityEven;
	c1.iRate=c2.iRate=EBps19200;
	c1.iHandshake=c2.iHandshake=0;

	r=theSerialPorts[0]->SetConfig(cBuf1);
	test(r==KErrNone);
	r=theSerialPorts[1]->SetConfig(cBuf2);
	test(r==KErrNone);

	RTimer timer;
	test(timer.CreateLocal()==KErrNone);
	TTime wakeup;
	wakeup.HomeTime();
	wakeup+=TTimeIntervalSeconds(10);
	TRequestStatus done;
	timer.At(done,wakeup);
	test(done==KRequestPending);
	RAsyncSwitchOff async;
	r=async.Start(2000000);
	test(r==KErrNone);

//	test(PowerCheckedWrite(KWriteSize*200)==KErrNone);

	const TUint bigWriteSize=KWriteSize*200;
	TUint8* inBuf=new TUint8[bigWriteSize];
	test(inBuf!=NULL);
	TUint8* outBuf=new TUint8[bigWriteSize];
	test(outBuf!=NULL);
	TPtr8 outDes(outBuf,bigWriteSize,bigWriteSize);
	TPtr8 inDes(inBuf,bigWriteSize,bigWriteSize);

	RTimer tim;
	tim.CreateLocal();
	TRequestStatus readStatus;
	TRequestStatus timeStatus;

	StripeMem(outDes,'A','Z');
	inDes.FillZ();

	theSerialPorts[0]->Read(readStatus,inDes,bigWriteSize);
	test(readStatus==KRequestPending);

	test.Printf(_L("Write........."));
	r=theSerialPorts[1]->WriteS(outDes,bigWriteSize);
	test(r==KErrAbort);
	test.Printf(_L("Aborted by power down\n"));
	r=async.Wait();
	test(r==KErrNone);
	r=async.Start(2000000);
	test(r==KErrNone);
	const TUint KTimeOut=6000000;
	tim.After(timeStatus,KTimeOut);
	User::WaitForRequest(readStatus,timeStatus);
	if (timeStatus==KErrNone)
		{
		test.Printf(_L("Timed Out!\n\r"));
		theSerialPorts[0]->ReadCancel();
		test(EFalse);
		}
	tim.Cancel();
	test(readStatus==KErrAbort);
	r=async.Wait();
	test(r==KErrNone);

	User::WaitForRequest(done);
	test(done==KErrNone);

	test.Next(_L("Reset config"));
	TestSimpleWriting();
	test.Next(_L("Close and reopen"));
	theSerialPorts[0]->Close();
	theSerialPorts[1]->Close();

	r=theSerialPorts[0]->Open(PortA);
	test(r==KErrNone);
	r=theSerialPorts[1]->Open(PortB);
	test(r==KErrNone);

	theSerialPorts[0]->Config(cBuf1);
	theSerialPorts[1]->Config(cBuf2);
	c1.iFifo=EFifoEnable;
	c2.iDataBits=c1.iDataBits=EData8;
	c2.iStopBits=c1.iStopBits=EStop1;
	c2.iParity=c1.iParity=EParityNone;
	c1.iRate=c2.iRate=EBps19200;
	c1.iHandshake=c2.iHandshake=0;

	r=theSerialPorts[0]->SetConfig(cBuf1);
	test(r==KErrNone);
	r=theSerialPorts[1]->SetConfig(cBuf2);
	test(r==KErrNone);

	TestSimpleWriting();

	test.Next(_L("Power down while writing 1->0"));
	theSerialPorts[0]->Config(cBuf1);
	theSerialPorts[1]->Config(cBuf2);
	c1.iFifo=EFifoEnable;

	c2.iDataBits=c1.iDataBits=EData8;
	c2.iStopBits=c1.iStopBits=EStop1;
	c2.iParity=c1.iParity=EParityEven;
	c1.iRate=c2.iRate=EBps9600;
	c1.iHandshake=c2.iHandshake=0;

	r=theSerialPorts[0]->SetConfig(cBuf1);
	test(r==KErrNone);
	r=theSerialPorts[1]->SetConfig(cBuf2);
	test(r==KErrNone);

	wakeup.HomeTime();
	wakeup+=TTimeIntervalSeconds(10);
	timer.At(done,wakeup);
	test(done==KRequestPending);
	r=async.Start(2000000);
	test(r==KErrNone);

//	test(PowerCheckedWrite(KWriteSize*200)==KErrNone);
	StripeMem(outDes,'A','Z');
	inDes.FillZ();

	theSerialPorts[1]->Read(readStatus,inDes,bigWriteSize);
	test(readStatus==KRequestPending);

	test.Printf(_L("Write........."));
	r=theSerialPorts[0]->WriteS(outDes,bigWriteSize);
	test(r==KErrAbort);
	test.Printf(_L("Aborted by power down\n"));
	tim.After(timeStatus,KTimeOut);
	User::WaitForRequest(readStatus,timeStatus);
	if (timeStatus==KErrNone)
		{
		test.Printf(_L("Timed Out!\n\r"));
		theSerialPorts[1]->ReadCancel();
		test(EFalse);
		}
	tim.Cancel();
	CHECK(readStatus.Int(),KErrAbort);
	r=async.Wait();
	test(r==KErrNone);

	User::WaitForRequest(done);
	test(done==KErrNone);

	test.Next(_L("Reset config"));
	TestSimpleWriting();
	test.Next(_L("Close and reopen"));
	theSerialPorts[0]->Close();
	theSerialPorts[1]->Close();

	r=theSerialPorts[0]->Open(PortA);
	test(r==KErrNone);
	r=theSerialPorts[0]->QueryReceiveBuffer();
	test(r==0);
	r=theSerialPorts[1]->Open(PortB);
	test(r==KErrNone);
	r=theSerialPorts[1]->QueryReceiveBuffer();
	test(r==0);

	theSerialPorts[0]->Config(cBuf1);
	theSerialPorts[1]->Config(cBuf2);
	c1.iFifo=EFifoEnable;
	c2.iDataBits=c1.iDataBits=EData8;
	c2.iStopBits=c1.iStopBits=EStop1;
	c2.iParity=c1.iParity=EParityNone;
	c1.iRate=c2.iRate=EBps19200;
	c1.iHandshake=c2.iHandshake=0;

	r=theSerialPorts[0]->SetConfig(cBuf1);
	test(r==KErrNone);
	r=theSerialPorts[1]->SetConfig(cBuf2);
	test(r==KErrNone);

	TestSimpleWriting();

	theSerialPorts[0]->Close();
	theSerialPorts[1]->Close();

	test.Next(_L("Test signals are preserved"));

	r=theSerialPorts[0]->Open(PortA);
	test(r==KErrNone);
	r=theSerialPorts[0]->QueryReceiveBuffer();
	test(r==0);
	r=theSerialPorts[1]->Open(PortB);
	test(r==KErrNone);
	r=theSerialPorts[1]->QueryReceiveBuffer();
	test(r==0);

	if((theCaps1.iHandshake & KCapsFreeRTSSupported) && (theCaps2.iHandshake & KCapsFreeRTSSupported))
		{//should also check for KConfigFreeDTR
		theSerialPorts[0]->Config(cBuf1);
		theSerialPorts[1]->Config(cBuf2);

		c1.iHandshake=KConfigFreeRTS|KConfigFreeDTR;
		c2.iHandshake=KConfigFreeRTS|KConfigFreeDTR;
		r=theSerialPorts[0]->SetConfig(cBuf1);
		CHECK(r,KErrNone);
		r=theSerialPorts[1]->SetConfig(cBuf2);
		CHECK(r,KErrNone);

		theSerialPorts[0]->SetSignals(KSignalRTS,KSignalDTR);
		theSerialPorts[1]->SetSignals(KSignalDTR,KSignalRTS);

		TUint signals=theSerialPorts[0]->Signals();
		//test(signals==(KSignalRTS|KSignalDSR));//something weird happens here under WINS - the CD line is set(?)
		CHECK((signals&(KSignalRTS|KSignalDSR)) , (KSignalRTS|KSignalDSR));
		signals=theSerialPorts[1]->Signals();
		CHECK(signals,(KSignalDTR|KSignalCTS));

		wakeup.HomeTime();
		wakeup+=TTimeIntervalSeconds(10);
		timer.At(done,wakeup);
		r=async.Start(5000000);
		CHECK(r,KErrNone);
		test(done==KRequestPending);
		User::WaitForRequest(done);
		test(done==KErrNone);
		r=async.Wait();
		CHECK(r,KErrNone);

		User::After(100000);	// wait for both ports to power back up
		signals=theSerialPorts[0]->Signals();
		//test(signals==(KSignalRTS|KSignalDSR));
		CHECK((signals&(KSignalRTS|KSignalDSR)) , (KSignalRTS|KSignalDSR));
		signals=theSerialPorts[1]->Signals();
		CHECK(signals,(KSignalDTR|KSignalCTS));
		}

	c1.iHandshake=0;
	c2.iHandshake=0;
	r=theSerialPorts[0]->SetConfig(cBuf1);
	test(r==KErrNone);
	r=theSerialPorts[1]->SetConfig(cBuf2);
	test(r==KErrNone);

	theSerialPorts[0]->Close();
	theSerialPorts[1]->Close();

	test.End();
	}

void testSwitchIrDA()
	{
	test.Next(_L("Switch to IrDA"));
//Open the serial port channel.

	TInt r=theSerialPorts[0]->Open(PortA);
	test(r==KErrNone);
	r=theSerialPorts[0]->QueryReceiveBuffer();
	test(r==0);

	theSerialPorts[0]->Caps(theCaps1Buf);
	if (!(theCaps1.iSIR&KCapsSIR115kbps))
		{
		theSerialPorts[0]->Close();
		test.Printf(_L("\t\tIrDA not supported\n"));
		return;
		}

	r=theSerialPorts[0]->QueryReceiveBuffer();
	test(r==0);


//Configure the channel for IrDA at 115.2k baud.
	TCommConfig cBuf1;
	TCommConfigV01& c1=cBuf1();
	theSerialPorts[0]->Config(cBuf1);
	c1.iSIREnable=ESIREnable;
	c1.iRate=EBps115200;
	c1.iDataBits=EData8;
	c1.iParity=EParityNone;
	c1.iStopBits=EStop1;
	c1.iHandshake=0;
	c1.iHandshake|=KConfigFreeDTR;
	c1.iHandshake|=KConfigFreeRTS;
	r=theSerialPorts[0]->SetConfig(cBuf1);
	test(r==KErrNone);
	r=theSerialPorts[0]->QueryReceiveBuffer();
	test(r==0);
	const TUint8 KData[1] ={0x00};
	const TPtrC8 KDataPtr(KData,1);
	TRequestStatus stat;
	theSerialPorts[0]->Write(stat,KDataPtr);
	User::WaitForRequest(stat);
	r=theSerialPorts[0]->QueryReceiveBuffer();
	test.Printf(_L("ReceiveBuf = %d\n"),r);
//	test(r==0);

	theSerialPorts[0]->Write(stat,KDataPtr);
	User::WaitForRequest(stat);
	User::After(1000000);
	r=theSerialPorts[0]->QueryReceiveBuffer();
	test.Printf(_L("ReceiveBuf = %d\n"),r);
	while (theSerialPorts[0]->QueryReceiveBuffer())
		{
		TBuf8<1> buf;
		theSerialPorts[0]->Read(stat,buf,1);
		test.Printf(_L("Data = "),&buf);
		User::WaitForRequest(stat);
		test.Printf(_L("%d\n"),buf[0]);
		}
	r=theSerialPorts[0]->QueryReceiveBuffer();
	test.Printf(_L("ReceiveBuf = %d\n"),r);
	theSerialPorts[0]->Write(stat,KDataPtr);
	User::WaitForRequest(stat);
//Check for any received data pending (the answer is 1! Which is incorrect as nothing has sent me any IrDA data)
	r=theSerialPorts[0]->QueryReceiveBuffer();
	test.Printf(_L("ReceiveBuf = %d\n"),r);
//	test(r==0);
	theSerialPorts[0]->Write(stat,KDataPtr);
	User::WaitForRequest(stat);
	User::After(1000000);
	r=theSerialPorts[0]->QueryReceiveBuffer();

	theSerialPorts[0]->Close();
	}

GLDEF_C TInt E32Main()
//
//
//
    {

#if defined (__WINS__)
	test.SetLogged(ETrue);	// log to $TEMP/EPOCWIND.OUT
#else
	test.SetLogged(EFalse);	//turn off serial port debugging!
#endif


	test.Title();
	test.Start(_L("Serial loopback test"));

	TInt muid=0;
	test(HAL::Get(HAL::EMachineUid, muid)==KErrNone);
//CF
	TBool isAssabet=(muid==HAL::EMachineUid_Assabet);

	PortA=0;
	PortB=3; // used to be 1 but it apparently doesn't exist
	TBuf <0x100> cmd;
	User::CommandLine(cmd);

	TBool stress = EFalse;
	if (cmd.Length()>0)
		{
		if (cmd.Length() == 1)
			{
			if ((cmd[0] == 'S') || (cmd[0] == 's'))
				stress = ETrue;
			}
		else
			{
			if (cmd[0]>='0' && cmd[0]<='9')
				PortA=(TInt)(cmd[0]-'0');
			if (cmd[2]>='0' && cmd[2]<='9')
				PortB=(TInt)(cmd[2]-'0');
			if ((cmd[cmd.Length()-1] == 'S') || (cmd[cmd.Length()-1] == 's'))
				stress = ETrue;
			}
		}


	test.Printf(_L("Primary Port:%d Secondary Port:%d\n\r"),PortA,PortB);


	TInt r;
    TBuf<10> pddName=PDD_NAME;
	test.Next(_L("Load PDDs"));
#ifdef __WINS__
	const TInt KMaxPdds=0;
#else
	const TInt KMaxPdds=10;
#endif
	TInt i;
	for (i=-1; i<KMaxPdds; ++i)
		{
		if (i==0)
			pddName.Append(TChar('0'));
		else if (i>0)
			pddName[pddName.Length()-1] = (TText)('0'+i);
		r=User::LoadPhysicalDevice(pddName);
		if (r==KErrNone || r==KErrAlreadyExists)
			test.Printf(_L("PDD %S loaded\n"),&pddName);
		}

	test.Next(_L("Load LDD"));
	r=User::LoadLogicalDevice(LDD_NAME);
	test.Printf(_L("Load LDD Return %d\n\r"),r);

	test.Next(_L("Create RComm objects"));
	theSerialPorts[0]=new RComm;
	theSerialPorts[1]=new RComm;
	test(theSerialPorts[0]!=NULL);
	test(theSerialPorts[1]!=NULL);
//

	do
		{		

		test.Next(_L("Open:"));
		r=theSerialPorts[0]->Open(PortA);
		test.Printf(_L("Open(Unit0)=%d\n\r"),r);
		test(r==KErrNone);
		r=theSerialPorts[1]->Open(PortB);
		test.Printf(_L("Open(Unit1)=%d\n\r"),r);
		test(r==KErrNone);

		test.Next(_L("Get caps"));
		theSerialPorts[0]->Caps(theCaps1Buf);
		test(r==KErrNone);
		theSerialPorts[1]->Caps(theCaps2Buf);
		test(r==KErrNone);

		theSerialPorts[0]->Close();
		theSerialPorts[1]->Close();

		testReadWrite();

		// testTiming();

		turnaroundTestReadWrite();

		testTerminators();
		testHWHandshaking();

		if((theCaps1.iHandshake & KCapsObeyXoffSupported) && (theCaps2.iHandshake & KCapsObeyXoffSupported))
			testXonXoff();

		if((theCaps1.iHandshake & KCapsObeyCTSSupported) && (theCaps2.iHandshake & KCapsObeyCTSSupported))
			{
			testSingleCharacterReads();
			testWriteZero();
	//CF - see description of problem with testTerminators()
			if (!isAssabet) testMultiTerminatorCompletion();
			testBiDirectionalSingleCharacterReads();
			}

		testFraming();
		testBreak();
		testSwitchIrDA();
		} while (stress);

	User::After(3000000);
	test.End();
	return(KErrNone);
	}
