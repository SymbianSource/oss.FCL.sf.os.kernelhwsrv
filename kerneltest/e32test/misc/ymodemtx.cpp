// Copyright (c) 2009-2009 Nokia Corporation and/or its subsidiary(-ies).
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
// e32test\misc\ymodemtx.cpp
// 
//

#include <e32std.h>
#include <e32svr.h>
#include <f32file.h>
#include <d32comm.h>

RFs TheFs;
RFile TheFile;
TInt FileSize;
RBusDevComm TheComm;
RTimer TheTimer;
TFileName FileName;
TFileName FileName8b;
TPtr8 FileName8(0,0,0);

const TUint8 STX=0x02;
const TUint8 EOT=0x04;
const TUint8 ACK=0x06;
const TUint8 BIGC=0x43;
const TUint8 BIGG=0x47;

#define PACKET_SIZE		1024

#define MIN(a,b)		((a)<(b)?(a):(b))
#define OFFSET(p,off)	((void*)((char*)p+off))

#define RESET_COMM()	TheComm.ResetBuffers()

#define assert(x)			((void)( (x) || (__Panic(__LINE__, #x),0) ))
#define assert_KErrNone(r)	((void)( ((r)==KErrNone) || (__Panic(__LINE__, (r)),0) ))


void __Panic(TInt aLine, TInt aError)
	{
	RDebug::Printf("Line %d Expected KErrNone got %d", aLine, aError);
	User::Panic(_L("YMODEMTX"), aLine);
	}

void __Panic(TInt aLine, const char* aMessage)
	{
	RDebug::Printf("Line %d Assertion \"%s\" failed", aLine, aMessage);
	User::Panic(_L("YMODEMTX"), aLine);
	}

/*
YModem packet structure:
Byte 0 = STX
Byte 1 = sequence number (first user data packet is 1)
Byte 2 = complement of sequence number
Bytes 3-1026 = data (1K per packet)
Bytes 1027, 1028 = 16-bit CRC (big-endian)

A herald packet is sent first, with sequence number 0
The data field consists of the null-terminated file name
followed by the null-terminated file size in ASCII decimal
digits.
*/
struct SPacket
	{
	TUint8 iPTI;
	TUint8 iSeq;
	TUint8 iSeqBar;
	TUint8 iData[PACKET_SIZE];
	TUint8 iCRC1;
	TUint8 iCRC0;
	};

static const TUint16 crcTab[256] =
    {
	0x0000,0x1021,0x2042,0x3063,0x4084,0x50a5,0x60c6,0x70e7,0x8108,0x9129,0xa14a,
	0xb16b,0xc18c,0xd1ad,0xe1ce,0xf1ef,0x1231,0x0210,0x3273,0x2252,0x52b5,0x4294,
	0x72f7,0x62d6,0x9339,0x8318,0xb37b,0xa35a,0xd3bd,0xc39c,0xf3ff,0xe3de,0x2462,
	0x3443,0x0420,0x1401,0x64e6,0x74c7,0x44a4,0x5485,0xa56a,0xb54b,0x8528,0x9509,
	0xe5ee,0xf5cf,0xc5ac,0xd58d,0x3653,0x2672,0x1611,0x0630,0x76d7,0x66f6,0x5695,
	0x46b4,0xb75b,0xa77a,0x9719,0x8738,0xf7df,0xe7fe,0xd79d,0xc7bc,0x48c4,0x58e5,
	0x6886,0x78a7,0x0840,0x1861,0x2802,0x3823,0xc9cc,0xd9ed,0xe98e,0xf9af,0x8948,
	0x9969,0xa90a,0xb92b,0x5af5,0x4ad4,0x7ab7,0x6a96,0x1a71,0x0a50,0x3a33,0x2a12,
	0xdbfd,0xcbdc,0xfbbf,0xeb9e,0x9b79,0x8b58,0xbb3b,0xab1a,0x6ca6,0x7c87,0x4ce4,
	0x5cc5,0x2c22,0x3c03,0x0c60,0x1c41,0xedae,0xfd8f,0xcdec,0xddcd,0xad2a,0xbd0b,
	0x8d68,0x9d49,0x7e97,0x6eb6,0x5ed5,0x4ef4,0x3e13,0x2e32,0x1e51,0x0e70,0xff9f,
	0xefbe,0xdfdd,0xcffc,0xbf1b,0xaf3a,0x9f59,0x8f78,0x9188,0x81a9,0xb1ca,0xa1eb,
	0xd10c,0xc12d,0xf14e,0xe16f,0x1080,0x00a1,0x30c2,0x20e3,0x5004,0x4025,0x7046,
	0x6067,0x83b9,0x9398,0xa3fb,0xb3da,0xc33d,0xd31c,0xe37f,0xf35e,0x02b1,0x1290,
	0x22f3,0x32d2,0x4235,0x5214,0x6277,0x7256,0xb5ea,0xa5cb,0x95a8,0x8589,0xf56e,
	0xe54f,0xd52c,0xc50d,0x34e2,0x24c3,0x14a0,0x0481,0x7466,0x6447,0x5424,0x4405,
	0xa7db,0xb7fa,0x8799,0x97b8,0xe75f,0xf77e,0xc71d,0xd73c,0x26d3,0x36f2,0x0691,
	0x16b0,0x6657,0x7676,0x4615,0x5634,0xd94c,0xc96d,0xf90e,0xe92f,0x99c8,0x89e9,
	0xb98a,0xa9ab,0x5844,0x4865,0x7806,0x6827,0x18c0,0x08e1,0x3882,0x28a3,0xcb7d,
	0xdb5c,0xeb3f,0xfb1e,0x8bf9,0x9bd8,0xabbb,0xbb9a,0x4a75,0x5a54,0x6a37,0x7a16,
	0x0af1,0x1ad0,0x2ab3,0x3a92,0xfd2e,0xed0f,0xdd6c,0xcd4d,0xbdaa,0xad8b,0x9de8,
	0x8dc9,0x7c26,0x6c07,0x5c64,0x4c45,0x3ca2,0x2c83,0x1ce0,0x0cc1,0xef1f,0xff3e,
	0xcf5d,0xdf7c,0xaf9b,0xbfba,0x8fd9,0x9ff8,0x6e17,0x7e36,0x4e55,0x5e74,0x2e93,
	0x3eb2,0x0ed1,0x1ef0
    };

void UpdateCrc(const void* aPtr, TInt aLength, TUint16& aCrc)
//
// Perform a CCITT CRC checksum.
//
	{

	register const TUint8* pB = (const TUint8*)aPtr;
	register TUint16 crc=aCrc;
    while (aLength--)
		crc=TUint16((crc<<8)^crcTab[(crc>>8)^*pB++]);
	aCrc=crc;
	}

void ClearCommError()
	{
	}

TInt CommRead1(TInt aTimeout)
	{
	TBuf8<1> c;
	TRequestStatus s0, s1;
	TheComm.ReadOneOrMore(s1, c);
	TheTimer.After(s0, aTimeout*1000);
	User::WaitForRequest(s0, s1);
	if (s1 != KRequestPending)
		{
		TheTimer.Cancel();
		User::WaitForRequest(s0);
		return (s1==KErrNone) ? c[0] : s1.Int();
		}
	TheComm.ReadCancel();
	User::WaitForRequest(s1);
	return KErrTimedOut;
	}

TInt CommRead(TDes8& aBuf, TInt aTimeout)
	{
	aBuf.Zero();
	TRequestStatus s0, s1;
	TInt timeout = aTimeout * 1000;

	TUint8* b = (TUint8*)aBuf.Ptr();
	while (aBuf.Length() < aBuf.MaxLength())
		{
		TPtr8 p(b+aBuf.Length(), 0, aBuf.MaxLength()-aBuf.Length());
		TheComm.ReadOneOrMore(s1, p);
		TheTimer.After(s0, timeout);
		User::WaitForRequest(s0, s1);
		if (s1 == KErrNone)
			{
			TheTimer.Cancel();
			User::WaitForRequest(s0);
			aBuf.SetLength(aBuf.Length() + p.Length());
			timeout = 500000;
			continue;
			}
		if (s1 != KRequestPending)
			{
			TheTimer.Cancel();
			User::WaitForRequest(s0);
			break;
			}
		if (s0 != KRequestPending)
			{
			TheComm.ReadCancel();
			User::WaitForRequest(s1);
			break;
			}
		}
	if (aBuf.Length()==0)
		return KErrTimedOut;
	return aBuf.Length();
	}

void CommWrite(const TDesC8& aBuf)
	{
	TRequestStatus s;
	TheComm.Write(s, aBuf);
	User::WaitForRequest(s);
	}

void CommWriteC(TUint aChar)
	{
	TBuf8<1> b;
	b.SetLength(1);
	b[0] = (TUint8)aChar;
	CommWrite(b);
	}

void CommWriteS(const char* aString)
	{
	CommWrite(TPtrC8((const TUint8*)aString));
	}

TInt PreparePacket(SPacket& pkt, TInt aSeq)
	{
	TInt r = KErrNone;
	TUint16 crc = 0;

	pkt.iPTI = STX;
	pkt.iSeq = (TUint8)(aSeq>=0 ? aSeq : 0);
	pkt.iSeqBar = (TUint8)~pkt.iSeq;
	if (aSeq>0)
		{
		TInt l;
		TInt fpos = (aSeq-1)*PACKET_SIZE;	// file position of packet
		if ( fpos >= FileSize )
			return KErrEof;
		l = MIN(PACKET_SIZE, FileSize-fpos);
		TPtr8 d(pkt.iData, 0, l);
		assert_KErrNone(TheFile.Read(d));
		if (l<PACKET_SIZE)
			memset(pkt.iData+l, 0, PACKET_SIZE-l);
		}
	else
		{
		memset(pkt.iData, 0, PACKET_SIZE);
		if (aSeq==0)
			{
			TInt pos = FileName8.LocateReverse('\\');
			TPtrC8 fn8(FileName8.Mid(pos+1));
			memcpy(pkt.iData, fn8.Ptr(), fn8.Length());
			TPtr8 d2(pkt.iData+fn8.Length()+1, 0, PACKET_SIZE-fn8.Length()-1);
			d2.Num(FileSize);
			}
		}
	UpdateCrc(pkt.iData, PACKET_SIZE, crc);
	pkt.iCRC1 = (TUint8)(crc>>8);
	pkt.iCRC0 = (TUint8)crc;
	return r;
	}

TInt SendPacket(TInt& aSeq, TBool aStream)
	{
	TInt c;
	SPacket pkt;
	TInt retries = 10;
	TInt tmout = (aSeq>=0) ? 2000 : 500;
	TInt r = PreparePacket(pkt, aSeq);
	if (r!=KErrNone)
		return r;
	for(;;)
		{
		RESET_COMM();
		CommWrite(TPtrC8( (const TUint8*)&pkt, sizeof(pkt) ));
		if (aStream)
			break;
		c = CommRead1(tmout);
		if (c==KErrTimedOut && aSeq<0)
			return KErrNone;
		if (c>=0)
			{
			if (c==ACK)
				break;
			}
		if (--retries==0)
			return KErrTimedOut;
		}
	if (aSeq==0)
		{
		c = CommRead1(100);
		if (c==KErrTimedOut)
			{
			++aSeq;
			return KErrNone;
			}
		if (aStream && c!=BIGG)
			return KErrTimedOut;
		else if (!aStream && c!=BIGC)
			return KErrTimedOut;
		}
	++aSeq;
	return r;
	}

TInt SendImageFile()
	{
	TInt r = 0;
	TInt b1;
	TBool stream;
	TInt seq = 0;

	RESET_COMM();
	r = CommRead1(20000);
	if (r<0)
		return r;
	if (r!=BIGG && r!=BIGC)
		return KErrTimedOut;
	stream = (r==BIGG);
	seq = 0;
	r = KErrNone;
	while (r==KErrNone)
		{
		r = SendPacket(seq, stream);
		}
	if (r!=KErrEof)
		return r;
	r=KErrNone;
	RESET_COMM();
	CommWriteC(EOT);
	b1 = CommRead1(500);
	if (b1==KErrTimedOut)
		return KErrNone;
	if (b1!=ACK)
		return KErrNone;
	b1 = CommRead1(1000);
	if (b1==KErrTimedOut)
		return KErrNone;
	if (stream && b1!=BIGG)
		return KErrNone;
	else if (!stream && b1!=BIGC)
		return KErrNone;
	seq = -1;
	r = SendPacket(seq, stream);
	return r;
	}


#define	ALLOWBPS(x)	case x : bps = EBps##x; break

void ParseCommandLine()
	{
	TInt r = 0;
	TInt cmdLineLen = User::CommandLineLength();
	HBufC* cmdLine = HBufC::New(cmdLineLen);
	assert(cmdLine!=0);
	TPtr cmdLineW(cmdLine->Des());
	User::CommandLine(cmdLineW);
	TLex16 cmdLineLex(*cmdLine);
	TInt baud = 115200;
	TInt port = 0;
	while (!cmdLineLex.Eos())
		{
		TPtrC token(cmdLineLex.NextToken());
		cmdLineLex.SkipSpace();
		if (token[0]=='-')
			{
			switch (token[1])
				{
				case 'p':
					{
					r = cmdLineLex.Val(port);
					assert_KErrNone(r);
					break;
					}
				case 'b':
					{
					r = cmdLineLex.Val(baud);
					assert_KErrNone(r);
					break;
					}
				default:
					assert(0);
					break;
				}
			continue;
			}
		FileName = token;
		assert(cmdLineLex.Eos());
		}
	assert(FileName.Length()>0);

	TBps bps = EBpsSpecial;
	switch (baud)
		{
		ALLOWBPS(50);
		ALLOWBPS(75);
		ALLOWBPS(110);
		ALLOWBPS(134);
		ALLOWBPS(150);
		ALLOWBPS(300);
		ALLOWBPS(600);
		ALLOWBPS(1200);
		ALLOWBPS(1800);
		ALLOWBPS(2000);
		ALLOWBPS(2400);
		ALLOWBPS(3600);
		ALLOWBPS(4800);
		ALLOWBPS(7200);
		ALLOWBPS(9600);
		ALLOWBPS(19200);
		ALLOWBPS(38400);
		ALLOWBPS(57600);
		ALLOWBPS(115200);
		ALLOWBPS(230400);
		ALLOWBPS(460800);
		ALLOWBPS(576000);
		ALLOWBPS(1152000);
		ALLOWBPS(4000000);
		ALLOWBPS(921600);
		}
	assert(bps != EBpsSpecial);

	assert_KErrNone(TheFs.Connect());
	assert_KErrNone(TheFile.Open(TheFs, FileName, EFileShareReadersOnly|EFileRead));
	assert_KErrNone(TheFile.Size(FileSize));
	FileName8b = FileName;
	new (&FileName8) TPtr8(FileName8b.Collapse());
	assert(TheComm.Open(port)==KErrNone);
	TCommConfig cfgBuf;
	TCommConfigV01& cfg = cfgBuf();
	cfg.iRate = bps;
	cfg.iDataBits = EData8;
	cfg.iStopBits = EStop1;
	cfg.iParity = EParityNone;
	cfg.iHandshake = 0;
	cfg.iParityError = 0;
	cfg.iFifo = EFifoEnable;
	cfg.iSpecialRate = 0;
	cfg.iTerminatorCount = 0;
	cfg.iSIREnable = ESIRDisable;
	cfg.iSIRSettings = KConfigSIRShutDown;
	assert_KErrNone(TheComm.SetConfig(cfgBuf));
	CommWrite(_L8("Sending "));
	CommWrite(FileName8);
	CommWrite(_L8("\r\nWaiting for YModem or YModem-G receive...\r\n"));
	}

_LIT(KLddName,"ECOMM");
_LIT(KPddName,"EUART");

void LoadCommDrivers()
	{
	TInt r=User::LoadLogicalDevice(KLddName);
	if (r!=KErrAlreadyExists)
		assert_KErrNone(r);

	TInt i;
	TInt n=0;
	for (i=-1; i<10; ++i)
		{
		TBuf<16> pddName=KPddName();
		if (i>=0)
			pddName.Append('0'+i);
		r=User::LoadPhysicalDevice(pddName);
		if (r==KErrNone || r==KErrAlreadyExists)
			++n;
		}
	assert(n!=0);
	}

TInt E32Main()
	{
	TInt r;

	LoadCommDrivers();
	ParseCommandLine();

	r = TheTimer.CreateLocal();
	assert_KErrNone(r);

	r = SendImageFile();
	assert_KErrNone(r);

	TheTimer.Close();
	TheComm.Close();
	TheFile.Close();
	TheFs.Close();

	return KErrNone;
	}
