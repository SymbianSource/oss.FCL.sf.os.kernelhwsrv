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
// e32test\device\t_term.cpp
// T_TERM.CPP - Dumb terminal
// 
//

#define VERBOSE
#define _PC_CARD_SERIAL

#include <e32test.h>
#include <e32twin.h>
#include <d32comm.h>
#include <f32file.h>

void StartLoader();

const TPtrC KCaptureFileName=_L("C:\\CAPTURE.TXT");
const TPtrC KUploadFileName=_L("Z:\\UPLOAD.TXT");
RFs TheFs;
RFile TheCaptureFile;
RFile TheUploadFile;

RChunk TheCaptureChunk;
TPtr8 TheCapturedText(NULL,0,0);

TBuf8<1024> ch;
TBuf8<1024> chw;
TBuf<1024>	buf;
TCommConfig TheConfigBuf;
TCommConfigV01 &TheConfig=TheConfigBuf();
TInt TheLastError=KErrNone;

const TInt KMaxDumpLength=0x100;

enum TTermPanic
	{
	EStraySignal,
	ELoadPhysicalDeviceErr,
	ELoadLogicalDeviceErr,
	EOpenErr,
	EConnectFsErr,
	ECaptureFileOpen,
	EOpenUploadFile,
	EChunkCreateErr,
	};

enum TRxMode
	{
	ENormal=0,
	ELoopBack=1,
	ECountChars=2,
	ECapture=128,
	};

struct SSettings
	{
	TBool iNotFinished;
	TBool iLocalEcho;
	TInt iAddLF;
	TBool iDump;
	TInt iDumpRepeat;
	TBuf8<KMaxDumpLength> iDumpData;
	TRxMode iRxMode;
	TInt iCharCount;
	TInt iMaxInOne;
	TInt iInfraRed;
	TBool iWaitAfterWrite;
	// Fifo
	// Brk
	};

LOCAL_D SSettings TheSettings;
LOCAL_D RBusDevComm TheCommPort;
LOCAL_D RConsole TheWindow;

LOCAL_C TInt CommWriteSync(RBusDevComm &aComm, const TDesC8 &aData)
	{
	TRequestStatus stat;
	aComm.Write(stat, aData);
	User::WaitForRequest(stat);
	return stat.Int();
	}

LOCAL_C TInt WaitAfterWrite(RBusDevComm& aComm)
	{
	TRequestStatus s;
	TBuf8<1> b;
	aComm.Write(s,b);
	User::WaitForRequest(s);
	return s.Int();
	}

LOCAL_C TInt RateToInt(TBps aRate)
//
//
//
	{

	switch (aRate)
		{
	case EBps230400:	return 230400;
	case EBps115200:	return 115200;
    case EBps57600:	return 57600;
    case EBps38400:	return 38400;
    case EBps19200:	return 19200;
    case EBps9600:	return 9600;
	case EBps7200:	return 7200;
    case EBps4800:	return 4800;
	case EBps3600:	return 3600;
    case EBps2400:	return 2400;
	case EBps2000:	return 2000;
	case EBps1800:	return 1800;
    case EBps1200:	return 1200;
    case EBps600:	return 600;
    case EBps300:	return 300;
    case EBps150:	return 150;
	case EBps134:	return 134;
    case EBps110:	return 110;
	case EBps75:	return 75;
	case EBps50:	return 50;
	default:	return -1;
		}
	}

LOCAL_C TBps IntToRate(TInt aVal)
//
//
//
	{

	if (aVal>=230400) return EBps230400;
	if (aVal>=115200) return EBps115200;
	if (aVal>=57600) return EBps57600;
	if (aVal>=38400) return EBps38400;
	if (aVal>=19200) return EBps19200;
	if (aVal>=9600) return EBps9600;
	if (aVal>=7200) return EBps7200;
	if (aVal>=4800) return EBps4800;
	if (aVal>=3600) return EBps3600;
	if (aVal>=2400) return EBps2400;
	if (aVal>=2000) return EBps2000;
	if (aVal>=1800) return EBps1800;
	if (aVal>=1200) return EBps1200;
	if (aVal>=600) return EBps600;
	if (aVal>=300) return EBps300;
	if (aVal>=150) return EBps150;
	if (aVal>=134) return EBps134;
	if (aVal>=110) return EBps110;
	if (aVal>=75) return EBps75;
	if (aVal>=50) return EBps50;
	return EBps50;
	}

LOCAL_C void ConfigString(TDes &aBuf, const TCommConfigV01 &aConfig, const SSettings &aSettings)
//
//	Construct a Configuaration string
//
	{

	// Config
	aBuf.Format(_L(" %d "), RateToInt(aConfig.iRate));
	switch (aConfig.iParity)
		{
	case EParityEven: aBuf.Append(_L("E")); break;
	case EParityOdd: aBuf.Append(_L("O")); break;
	case EParityNone: aBuf.Append(_L("N")); break;
    default: break;
		}
	switch (aConfig.iDataBits)
		{
	case EData5: aBuf.Append(_L("5")); break;
	case EData6: aBuf.Append(_L("6")); break;
	case EData7: aBuf.Append(_L("7")); break;
	case EData8: aBuf.Append(_L("8")); break;
    default: break;
		}
	if (aConfig.iStopBits==EStop1)
		aBuf.Append(_L("1 "));
	else
		aBuf.Append(_L("2 "));

	aBuf.Append(_L("Use:"));
	if (aConfig.iHandshake==0)
		aBuf.Append(_L("NoControl "));
	if (aConfig.iHandshake&(KConfigObeyXoff|KConfigSendXoff))
		aBuf.Append(_L("XonXoff "));
	if (aConfig.iHandshake&KConfigObeyCTS)
		aBuf.Append(_L("CTS/RTS "));
	if (aConfig.iHandshake&KConfigObeyDSR)
		aBuf.Append(_L("DSR/DTR "));
	if (aConfig.iHandshake&KConfigWriteBufferedComplete)
		aBuf.Append(_L("Early "));
	//|KConfigObeyDCD|KConfigFailDCD|))


//	if (aConfig.iBreak==TEiger::EBreakOn)
//		aBuf.Append(_L(" Brk"));
	if (aConfig.iFifo==EFifoEnable)
		aBuf.Append(_L(" Fifo"));
	
	// Settings
	if (aSettings.iLocalEcho)
		aBuf.Append(_L("LocalEcho "));
	if (aSettings.iAddLF)
		aBuf.Append(_L("AddLF "));
	if ((aSettings.iRxMode&~ECapture)==ELoopBack)
		aBuf.Append(_L("LpBk"));
	else if ((aSettings.iRxMode&~ECapture)==ECountChars)
		aBuf.Append(_L("CtCh"));
	aBuf.Append(_L(" "));
	aBuf.AppendNum((TInt)(RThread().Priority()));
	if (aSettings.iInfraRed==1)
		aBuf.Append(_L("IR1"));
	else if (aSettings.iInfraRed==2)
		aBuf.Append(_L("IR2"));
	if (aSettings.iWaitAfterWrite)
		aBuf.Append(_L("Wait"));

	aBuf.Append(_L("Last Err: "));
	if (TheLastError==KErrNone)
		aBuf.Append(_L("None "));
	else if (TheLastError==KErrCommsLineFail)
		aBuf.Append(_L("LineFail "));
	else if (TheLastError==KErrCommsFrame)
		aBuf.Append(_L("Frame "));
	else if (TheLastError==KErrCommsOverrun)
		aBuf.Append(_L("Overrun "));
	else if (TheLastError==KErrCommsParity)
		aBuf.Append(_L("Parity "));
	else if (TheLastError==KErrAbort)
		aBuf.Append(_L("Abort "));
	else if (TheLastError==KErrBadPower)
		aBuf.Append(_L("BadPower "));
	else if (TheLastError==KErrNotReady)
		aBuf.Append(_L("NotReady "));
	else
		aBuf.AppendNum(TheLastError);
	}

LOCAL_C void GetRate(TBps &aRate, const TDesC &aDes)
//
//	Set Baud rate
//
	{

	TInt32 i;
	if (TLex(aDes).Val(i)==KErrNone)
		aRate=IntToRate(i);
	}

LOCAL_C void GetParity(TParity &aParity, const TDesC &aDes)
//
//
//
	{

	if (aDes.FindF(_L("O"))>=0)
		aParity=EParityOdd;
  	if (aDes.FindF(_L("E"))>=0)
		aParity=EParityEven;
	if (aDes.FindF(_L("N"))>=0)
		aParity=EParityNone;
	}

LOCAL_C void GetHandshake(TUint &aHandshake, const TDesC &aDes)
//
//
//
	{

	if (aDes.FindF(_L("N"))>=0)
		aHandshake=0;
	if (aDes.FindF(_L("X"))>=0)
		aHandshake=KConfigObeyXoff|KConfigSendXoff;
	if (aDes.FindF(_L("C"))>=0)
		aHandshake=KConfigObeyCTS;
	if (aDes.FindF(_L("D"))>=0)
		aHandshake=KConfigObeyDSR|KConfigFreeRTS;
	if (aDes.FindF(_L("E"))>=0)
		aHandshake|=KConfigWriteBufferedComplete;
	}

LOCAL_C void GetStopBit(TStopBits &aStop, const TDesC &aDes)
	{

	TInt32 in;
	if (TLex(aDes).Val(in)==KErrNone)
		{
		if (in==1)
			aStop=EStop1;
		if (in==2)
			aStop=EStop2;
		}
	else
		{
		if (aStop==EStop1)
			aStop=EStop2;
		else
			aStop=EStop1;
		}
	}

LOCAL_C void GetLength(TDataBits &aData, const TDesC &aDes)
	{

	TInt32 in;
	if (TLex(aDes).Val(in)==KErrNone)
		{
		switch (in)
			{
		case 5: aData=EData5; break;
		case 6: aData=EData6; break;
		case 7: aData=EData7; break;
		case 8: aData=EData8; break;
		default: break;
			}
		}
	}

LOCAL_C void GetInfraRedMode(TInt &aInfraRed, const TDesC &aDes)
	{

	if (aDes.FindF(_L("0"))>=0)
		aInfraRed=0;
	else if (aDes.FindF(_L("1"))>=0)
		aInfraRed=1;
	else if (aDes.FindF(_L("2"))>=0)
		aInfraRed=2;
	}

LOCAL_C void GetWaitMode(TBool &aWait, const TDesC &aDes)
	{

	if (aDes.FindF(_L("0"))>=0)
		aWait=EFalse;
	else if (aDes.FindF(_L("1"))>=0)
		aWait=ETrue;
	}

/*LOCAL_C void GetBreak(const TDesC &aDes)
	{

	if (aDes==_L(""))
		{
		if (data.iBreak==TEiger::EBreakOn)
			data.iBreak=TEiger::EBreakOff;
		else
			data.iBreak=TEiger::EBreakOn;
		}
	if (aDes.FindF(_L("N"))>=0)
		data.iBreak=TEiger::EBreakOn;
	if (aDes.FindF(_L("F"))>=0)
		data.iBreak=TEiger::EBreakOff;
	SetConfig();
	}
*/
LOCAL_C void GetFifo(TUint& aFifo, const TDesC &aDes)
	{

	if (aDes==_L(""))
		{
		if (aFifo==EFifoEnable)
			aFifo=EFifoDisable;
		else
			aFifo=EFifoEnable;
		}
	if (aDes.FindF(_L("N"))>=0)
		aFifo=EFifoEnable;
	if (aDes.FindF(_L("F"))>=0)
		aFifo=EFifoDisable;
	}

LOCAL_C void GetEcho(TBool &aEcho, const TDesC &aDes)
	{

	if (aDes==_L(""))
		{
		if (aEcho)
			aEcho=EFalse;
		else
			aEcho=ETrue;
		}
	if (aDes.FindF(_L("N"))>=0)
		aEcho=ETrue;
	if (aDes.FindF(_L("F"))>=0)
		aEcho=EFalse;
	}

LOCAL_C void GetRxMode(TRxMode &aMode, const TDesC &aDes)
	{

	if (aDes.FindF(_L("N"))>=0)
		aMode=ENormal;
	if (aDes.FindF(_L("L"))>=0)
		aMode=ELoopBack;
	if (aDes.FindF(_L("C"))>=0)
		aMode=ECountChars;
	if (aDes.FindF(_L("S"))>=0)
		{
		aMode=TRxMode(TInt(aMode)|ECapture);
//		TInt r=TheCaptureFile.Create(TheFs,KCaptureFileName,EFileWrite);
//		if (r!=KErrNone)
//			User::Panic(_L("T_TERM CAP"),r);
		}
	if (aDes.FindF(_L("Z"))>=0)
		{
		aMode=TRxMode(TInt(aMode)&~ECapture);
//		TheCaptureFile.Close();
		}
	if (aDes.FindF(_L("0"))>=0)
		RThread().SetPriority(EPriorityNormal);
	if (aDes.FindF(_L("1"))>=0)
		RThread().SetPriority(EPriorityAbsoluteHigh);
	}

LOCAL_C void GetDump(SSettings &aSettings, const TDesC &aDes)
	{
	
	TInt32 in;
	if (TLex(aDes).Val(in)==KErrNone)
		{
		aSettings.iDump=ETrue;
		aSettings.iDumpRepeat=in;
		return;
		}
	if (aDes.Length()!=0)
		{
		TBuf8<16> b=_L8("0123456789ABCDEF");
		aSettings.iDumpData.Zero();
		TInt i;
		for (i=0; i<16; i++)
			aSettings.iDumpData+=b;
		return;
		}
	RConsole dialog;
	TInt r=dialog.Init(_L("Type data to dump to comm.  Escape to finish"),TSize(KConsFullScreen,KConsFullScreen));
	r=dialog.Control(_L("+Maximize +NewLine"));
	aSettings.iDumpData=_L8("");
	TConsoleKey k;
	do 
		{
		dialog.Read(k);
		if (k.Code()==EKeyEscape)
			break;
		TText a=(TText)k.Code();
		TPtrC s(&a,1);
		dialog.Write(s);
		aSettings.iDumpData.Append(k.Code());
		//if (a=='\r')
		//	dialog.Write(_L("\n"));
		} while (aSettings.iDumpData.Length()<KMaxDumpLength);

	dialog.Destroy();
	dialog.Close();
	}



LOCAL_C void CommandWindow(TCommConfigV01 &aConfig, SSettings &aSettings)
//
//	Display some words of wisdom and get a command from the user
//
	{

	TBuf<32> b;
	b.Num(aSettings.iCharCount);
	b+=_L(" ");
	b.AppendNum(aSettings.iMaxInOne);
	b+=_L("\n");
	RConsole dialog;
	TInt r=dialog.Init(_L("."),TSize(KConsFullScreen,KConsFullScreen));
	r=dialog.Control(_L("+Maximize +NewLine"));
	dialog.Write(_L("B<n> Set Bps to n               P[Odd|Even|None] Set Parity\n"));
	dialog.Write(_L("S[1|2] Set/Toggle stop bits     L<n> Set Data Length (5<=n<=8)\n"));
	dialog.Write(_L("K[On|Off] Set/Toggle BRK        F[On|Off] Set/Toggle Fifo\n"));
	dialog.Write(_L("H[None|X|CtsRts|DsrDtr] Handshaking\n"));
	dialog.Write(_L("D[<n>] Set data or Dump data n times\n"));
 	dialog.Write(_L("J Toggle Add Line Feed          E Toggle local Echo\n"));
	dialog.Write(_L("U [NLC] Set Rx Mode\n"));
	dialog.Write(b);
	dialog.Write(_L("Q Quit\n"));
	dialog.Write(_L("\n:"));

	//	Get a command
	TBuf<0x80> des=_L("");
	TConsoleKey k;
	dialog.Read(k);
	while ((k.Code()!='\r') && (k.Code()!=EKeyEscape))
		{
		TText a=(TText)k.Code();
		TPtrC s(&a,1);
		dialog.Write(s);
		des.Append(k.Code());
		dialog.Read(k);
		}

	if (k.Code()!=EKeyEscape && des.Length()>0)
		{
		des.UpperCase();
		TBuf<0x80> right(des.Right(des.Length()-1));
		if (des[0]=='B')
			GetRate(aConfig.iRate, right);
		if (des[0]=='P')
			GetParity(aConfig.iParity, right);
		if (des[0]=='S')
			GetStopBit(aConfig.iStopBits, right);
		if (des[0]=='L')
			GetLength(aConfig.iDataBits, right);
//		if (des[0]=='K')
//			GetBreak(aSettings.iBreak, right);
		if (des[0]=='F')
			GetFifo(aConfig.iFifo, right);
		if (des[0]=='I')
			GetInfraRedMode(aSettings.iInfraRed, right);
		if (aSettings.iInfraRed==1)
			{
			aConfig.iSIREnable=ESIREnable;
			aConfig.iSIRSettings=KConfigSIRPulseWidthMinimum;
			}
		else if (aSettings.iInfraRed==2)
			{
			aConfig.iSIREnable=ESIREnable;
			aConfig.iSIRSettings=KConfigSIRPulseWidthMaximum;
			}
		else
			{
			aConfig.iSIREnable=ESIRDisable;
			aConfig.iSIRSettings=0;
			}
		if (des[0]=='H')
			GetHandshake(aConfig.iHandshake, right);
		if (des[0]=='E')
			GetEcho(aSettings.iLocalEcho, right);
		if (des[0]=='D')
			GetDump(aSettings, right);
		if (des[0]=='J')
			aSettings.iAddLF=!aSettings.iAddLF;
		if (des[0]=='U')
			{
			GetRxMode(aSettings.iRxMode, right);
			aSettings.iCharCount=0;
			aSettings.iMaxInOne=0;
			}
		if (des[0]=='Q')
			aSettings.iNotFinished=EFalse;
		if (des[0]=='W')
			GetWaitMode(aSettings.iWaitAfterWrite, right);
		}

	dialog.Destroy();
	dialog.Close();
	}

// The following decl is a hack for the Eiger build.
// Without it T_TERM.EXE has no .data or .bss section.  This means the data offset
// field in the file header is zero.  When the kernel comes to copy the data sections
// from rom into ram it reads the data size field (which is the size of all data
// sections) from the header and tries to copy data from 0x00000000 (the data offset),
// causing a data abort.
TInt dummy=10;	 
#if defined (__WINS__)
#define PDD_NAME _L("ECDRV")
#define LDD_NAME _L("ECOMM")
#else
#define PDD_NAME _L("EUART")
#define LDD_NAME _L("ECOMM")
#endif

LOCAL_C void ProcessError(TInt anError)
	{
	TBuf<80> buf;
	if (anError!=KErrNone)
		{
		TheLastError=anError;
		ConfigString(buf, TheConfig, TheSettings);
		TheWindow.SetTitle(buf);
		}
	}

LOCAL_C void HandleRx(TRequestStatus& aStatus, TBool /*aFinish*/)
	{
	chw.Copy(ch);
	switch(TheSettings.iRxMode & ~ECapture)
		{
		case ENormal:
			{
			buf.Copy(chw);
			TheWindow.Write(buf);
			break;
			}
		case ELoopBack:
			{
			ProcessError(CommWriteSync(TheCommPort,chw));
			if (TheSettings.iWaitAfterWrite)
				ProcessError(WaitAfterWrite(TheCommPort));
			break;
			}
		case ECountChars:
			{
			TInt l=chw.Length();
			TheSettings.iCharCount+=l;
			if (l>TheSettings.iMaxInOne)
				TheSettings.iMaxInOne=l;
			break;
			}
		}
	if ((TheSettings.iRxMode & ECapture)!=0 && TheCaptureChunk.Handle()!=0)
		{
//		TheCaptureFile.Write(chw);
		TInt newLen=TheCapturedText.Length()+chw.Length();
		TheCaptureChunk.Adjust(newLen);
		TheCapturedText.Append(chw);
		}
//	if ((TheSettings.iRxMode & ~ECapture)==ELoopBack && !aFinish)
//		TheCommPort.Read(aStatus, ch);
//	else
		TheCommPort.ReadOneOrMore(aStatus, ch);
	}

LOCAL_C TInt LoadDeviceDrivers()
//
// Load ECOMM.LDD and all PDDs with name EUART?.PDD
//
	{
	TInt c=0;
	TInt r;
	TInt i;
	TFileName n=PDD_NAME;
	r=User::LoadPhysicalDevice(n);
	if (r==KErrNone || r==KErrAlreadyExists)
		c++;
	n+=_L("0");
	TInt p=n.Length()-1;
	for (i=0; i<10; i++)
		{
		n[p]=TText('0'+i);
		r=User::LoadPhysicalDevice(n);
		if (r==KErrNone || r==KErrAlreadyExists)
			c++;
		}
	r=User::LoadLogicalDevice(LDD_NAME);
	if (r==KErrNone || r==KErrAlreadyExists)
		c++;
	return c;
	}

GLDEF_C TInt E32Main()
//
// Term
//
    {

	// Open the window asap
	TheWindow.Init(_L("TERM"),TSize(KConsFullScreen,KConsFullScreen));

	// Initialisation
	TInt r=TheFs.Connect();
	if (r!=KErrNone)
		User::Panic(_L("T_TERM"), EConnectFsErr);

	TBuf<256> cmd;
	User::CommandLine(cmd);
	TInt port=0;
	if (cmd.Length()>0 && cmd[0]>='0' && cmd[0]<='9')
		port=cmd[0]-'0';

	// Load Device Drivers
	TConsoleKey keystroke;
	TInt nDeviceDrivers=LoadDeviceDrivers();
	if (nDeviceDrivers<2)
		{
#if defined (VERBOSE)
		TBuf<32> outBuf;
		outBuf.AppendFormat(_L("Failed(0) 0x%X\n\r"),r);
		TheWindow.Write(outBuf);
		TheWindow.Read(keystroke);
#endif
		User::Panic(_L("T_TERM"), ELoadPhysicalDeviceErr);
		}

	r=TheCaptureChunk.CreateLocal(0x1000,0x1000000);		// 16Mb
	if (r!=KErrNone)
		r=TheCaptureChunk.CreateLocal(0x1000,0x100000);		// 1Mb
	if (r!=KErrNone)
		TheCaptureChunk.SetHandle(0);
	else
		TheCapturedText.Set(TheCaptureChunk.Base(),0,0x1000000);

	TheSettings.iNotFinished=ETrue;
	TheSettings.iLocalEcho=ETrue;
	TheSettings.iAddLF=FALSE;
	TheSettings.iDump=EFalse;
	TheSettings.iDumpRepeat=1;
	TheSettings.iDumpData=_L8("Some Text\r");
	TheSettings.iRxMode=ENormal;
	TheSettings.iCharCount=0;
	TheSettings.iMaxInOne=0;
	TheSettings.iInfraRed=0;
	TheSettings.iWaitAfterWrite=EFalse;
	
	// Comms Config
	r=TheCommPort.Open(port); // Comm port
	if (r!=KErrNone)
		User::Panic(_L("T_TERM"), EOpenErr);

	TheCommPort.Config(TheConfigBuf);	// get config
	TheConfig.iHandshake=0; //KConfigObeyXoff|KConfigSendXoff;
	TheCommPort.SetConfig(TheConfigBuf);
	TheCommPort.SetReceiveBufferLength(8192);

	//	Set up a console window
	TheWindow.Control(_L("+Maximize +Newline"));
	TheWindow.Write(_L("= for command\n"));
	TBuf<0x80> buf;
	ConfigString(buf, TheConfig, TheSettings);
	TheWindow.SetTitle(buf);

	TConsoleKey k;
	TRequestStatus readStat, keyStat;

	// main loop
	TheWindow.Read(k, keyStat);
	TheCommPort.ReadOneOrMore(readStat, ch);
	do
		{
		User::WaitForRequest(readStat, keyStat);
		if (keyStat!=KRequestPending)
			{
			TKeyCode c=k.Code();
			if (c<256 && c!='=' && c!='\x15' && c!='\x3' && c!='\x12' && c!='\x18')
				{
				TText8 a8=(TText8)c;
				TText a=(TText)c;
				TPtrC8 s8(&a8,1);
				TPtrC s(&a,1);
				ProcessError(CommWriteSync(TheCommPort, s8));
				if (TheSettings.iWaitAfterWrite)
					ProcessError(WaitAfterWrite(TheCommPort));
				if (TheSettings.iLocalEcho)
					{
					TheWindow.Write(s);
					if (c=='\r' && TheSettings.iAddLF) TheWindow.Write(_L("\n"));
					}
				}
			if (c=='\x3')
				{
				TheCommPort.ReadCancel();
				HandleRx(readStat,ETrue);
				}
			else if (c=='=')
				{
				CommandWindow(TheConfig, TheSettings);
				TheCommPort.ReadCancel();
				TheCommPort.SetConfig(TheConfigBuf);
				TheCommPort.ReadOneOrMore(readStat, ch);
				ConfigString(buf, TheConfig, TheSettings);
				TheWindow.SetTitle(buf);
				}
			else if (c=='\x15')
				{
				TInt r=TheUploadFile.Open(TheFs,KUploadFileName,EFileRead);
				if (r!=KErrNone)
					User::Panic(_L("T_TERM"),EOpenUploadFile);
				TBuf8<0x100> buf;
				do	{
					TheUploadFile.Read(buf);
					ProcessError(CommWriteSync(TheCommPort,buf));
					if (TheSettings.iWaitAfterWrite)
						ProcessError(WaitAfterWrite(TheCommPort));
					} while(buf.Length()!=0);
				TheUploadFile.Close();
				}
			else if (c=='\x12')
				{
//				TInt i=0;
//				TInt len=TheCapturedText.Length();
//				while(i<len)
//					{
//					TInt l=Min(0x100,len-i);
//					TPtrC8 p(TheCapturedText.Ptr()+i,l);
//					ProcessError(CommWriteSync(TheCommPort,p));
//					if (TheSettings.iWaitAfterWrite)
//						ProcessError(WaitAfterWrite(TheCommPort));
//					i+=l;
//					}
				if (TheCaptureChunk.Handle())
					{
					ProcessError(CommWriteSync(TheCommPort,TheCapturedText));
					if (TheSettings.iWaitAfterWrite)
						ProcessError(WaitAfterWrite(TheCommPort));
					}
				}
			else if (c=='\x18')
				{
				if (TheCaptureChunk.Handle())
					TheCapturedText.Zero();
				}
			TheWindow.Read(k, keyStat);
			}
		else if (readStat!=KRequestPending)
			{
			ProcessError(readStat.Int());
			if (readStat!=KErrAbort && readStat!=KErrBadPower && readStat!=KErrNotReady)
				HandleRx(readStat,EFalse);
			else
				TheCommPort.ReadOneOrMore(readStat, ch);
			}
		else
			{
			User::Panic(_L("T_TERM"), EStraySignal);
			}

		if (TheSettings.iDump)
			{
			TheSettings.iDump=EFalse;
			TInt i;
			for (i=0; i<TheSettings.iDumpRepeat; i++)
				{
				ProcessError(CommWriteSync(TheCommPort, TheSettings.iDumpData));
				if (TheSettings.iWaitAfterWrite)
					ProcessError(WaitAfterWrite(TheCommPort));
				}
			}
		} while(TheSettings.iNotFinished);

	TheWindow.Destroy();
	TheWindow.Close();
	TheCommPort.Close();
	return(KErrNone);
    }

