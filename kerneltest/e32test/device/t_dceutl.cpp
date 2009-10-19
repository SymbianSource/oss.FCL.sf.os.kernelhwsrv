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
// e32test\device\t_dceutl.cpp
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

const TInt KDisplayTitleY=0;
const TInt KDisplayMainY=2;
const TInt KDisplayTextY=17;
const TInt KUnit0=0;
const TInt KUnit1=1;
const TInt KTestPatternSize=250;
enum TPanic {ECreatingConsole,ELoadingPDD,ELoadingLDD,ECreatingRComm,EOpeningPort,ESettingPort,ECircBuf,EStraySignal};

#if defined (__WINS__)
#define PDD_NAME _L("ECDRV.PDD")
#define LDD_NAME _L("ECOMM.LDD")
#else
#define PDD_NAME _L("EUARTn")
#define LDD_NAME _L("ECOMM")
#endif

class RComm : public RBusDevComm
	{
public:
	TInt WriteS(const TDesC8& aDes);
	};

CConsoleBase *TheConsole;
RComm *TheSerialPort;
TCommConfig TheConfigBuf;
TCommConfigV01& TheConfig=TheConfigBuf();
TBool TheDtrState;
TBool TheRtsState;
TBuf8<KTestPatternSize> TheTestBuf;

LOCAL_C void Panic(TPanic aPanic)
//
// Panic
//
	{

	if (TheSerialPort)
		TheSerialPort->Close();
	delete TheSerialPort;
	delete TheConsole;
	User::Panic(_L("T_DCEUTL"),aPanic);
	}

LOCAL_C TPtrC BaudrateText(TBps aBaudrate)
	{
	switch(aBaudrate)
		{
		case EBps50: 		return(_L("50"));
		case EBps75: 		return(_L("75"));
		case EBps110: 		return(_L("110"));
		case EBps134: 		return(_L("134"));
		case EBps150: 		return(_L("150"));
		case EBps300: 		return(_L("300"));
		case EBps600: 		return(_L("600"));
		case EBps1200: 		return(_L("1200"));
		case EBps1800: 		return(_L("1800"));
		case EBps2000: 		return(_L("2000"));
		case EBps2400: 		return(_L("2400"));
		case EBps3600: 		return(_L("3600"));
		case EBps4800: 		return(_L("4800"));
		case EBps7200: 		return(_L("7200"));
		case EBps9600: 		return(_L("9600"));
		case EBps19200: 	return(_L("19200"));
		case EBps38400: 	return(_L("38400"));
		case EBps57600: 	return(_L("57600"));
		case EBps115200: 	return(_L("115200"));
		case EBps230400: 	return(_L("230400"));
		case EBps460800:	return(_L("460800"));
		case EBps576000:	return(_L("576000"));
		case EBps1152000: 	return(_L("1152000"));
		case EBps4000000: 	return(_L("4000000"));
		case EBpsSpecial: 	return(_L("Special"));
		default: 			return(_L("Unknown"));
		}
	}

LOCAL_C TPtrC DtrText(TBool aDtrState)
	{
	if (aDtrState)
		return(_L("ASSERTED"));
	else
		return(_L("NEGATED"));
	}

LOCAL_C TPtrC RtsText(TBool aRtsState)
	{
	if (aRtsState)
		return(_L("ASSERTED"));
	else
		return(_L("NEGATED"));
	}

TInt RComm::WriteS(const TDesC8& aDes)
//
// Syncronous write
//
	{

	TRequestStatus s;
	Write(s,aDes,aDes.Length());
	User::WaitForRequest(s);
	return(s.Int());
	}

LOCAL_C void CenteredPrintf(TInt aLine,TRefByValue<const TDesC> aFmt,...)
//
// Print centrally on specified line
//
	{
	VA_LIST list;
	VA_START(list,aFmt);
	TBuf<0x100> aBuf;
	aBuf.AppendFormatList(aFmt,list);
	TInt xPos = ((TheConsole->ScreenSize().iWidth)-aBuf.Length())/2;
	if (xPos<0)
		xPos=0;
	TheConsole->SetPos(0,aLine);
	TheConsole->ClearToEndOfLine();
	TheConsole->SetPos(xPos,aLine);
	TheConsole->Write(aBuf);
	}

LOCAL_C void Heading(TRefByValue<const TDesC> aFmt,...)
//
// Print a title
//
	{
	VA_LIST list;
	VA_START(list,aFmt);
	TBuf<0x100> aBuf;
	aBuf.AppendFormatList(aFmt,list);
	CenteredPrintf(KDisplayTitleY,aBuf);
	}

LOCAL_C void Instructions(TBool topLine,TRefByValue<const TDesC> aFmt,...)
//
// Print instructions (dont use top line with hex display).
//
	{
	VA_LIST list;
	VA_START(list,aFmt);
	TBuf<0x100> aBuf;
	aBuf.AppendFormatList(aFmt,list);
	CenteredPrintf((topLine)?KDisplayTextY-1:KDisplayTextY,aBuf);
	}

LOCAL_C void StripeMem(TDes8& aBuf,TUint aStartChar,TUint anEndChar)
//
// Mark a buffer with repeating byte pattern
//
	{

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

#define COLUMN_HEADER _L("            RxBuf            |          Expected       ")
LOCAL_C void DumpDescriptors(TDes8 &aLeft,TDes8 &aRight)
//
//
//
	{

	TBuf<80> b;
	CenteredPrintf(KDisplayMainY+2,_L("Compare failed:"));
	TInt minLen=Min(aLeft.Length(),aRight.Length());
	CenteredPrintf(KDisplayMainY+3,COLUMN_HEADER);
	TInt i=0;
	TInt j=0;
	TInt pos=KDisplayMainY+4;
	while (i<=minLen)
		{
		b.Format(_L("%02x: "),i);
		for (j=0;j<8;j++)
			{
			if ((i+j)<minLen)
				{
				TInt v=aLeft[i+j];
				b.AppendFormat(_L("%02x "),v);
				}
			else
				b.Append(_L("   "));
			}
		b.Append(_L(" | "));
		for (j=0;j<8;j++)
			{
			if ((i+j)<minLen)
				{
				TInt v=aRight[i+j];
				b.AppendFormat(_L("%02x "),v);
				}
			else
				b.Append(_L("   "));
			}
		CenteredPrintf(pos++,b);
		i+=8;
		if ((i%64)==0)
			{
			pos=KDisplayMainY+4;
			TheConsole->Getch();
			}
		}
	}

LOCAL_C TInt ChangeBaudrate()
//
// Change baudrate
//
	{

	CenteredPrintf(KDisplayMainY,_L("Select Baudrate:-"));
	CenteredPrintf(KDisplayMainY+1,_L("A - 4800  "));
	CenteredPrintf(KDisplayMainY+2,_L("B - 9600  "));
	CenteredPrintf(KDisplayMainY+3,_L("C - 19200 "));
	CenteredPrintf(KDisplayMainY+4,_L("D - 38400 "));
	CenteredPrintf(KDisplayMainY+5,_L("E - 57600 "));
	CenteredPrintf(KDisplayMainY+6,_L("F - 115200"));
	TChar c;
	do
		{
		c=(TUint)TheConsole->Getch();
		c.UpperCase();
		}
	while(c<'A' && c>'F');
	
	switch (c)
		{
		case 'A': TheConfig.iRate=EBps4800; break;
		case 'B': TheConfig.iRate=EBps9600; break;
		case 'C': TheConfig.iRate=EBps19200; break;
		case 'D': TheConfig.iRate=EBps38400; break;
		case 'E': TheConfig.iRate=EBps57600; break;
		case 'F': TheConfig.iRate=EBps115200; break;
		case 0x1b: return(KErrNone);
		}
	TInt r=TheSerialPort->SetConfig(TheConfigBuf);
	if (r!=KErrNone)
		{
		CenteredPrintf(KDisplayMainY+9,_L("Error (%d) changing baudrate"),r);
		TheConsole->Getch();
		}

	return(KErrNone);
	}

LOCAL_C TInt SendHayesCommand()
//
// Send short hayes command
//
	{

	TInt r=TheSerialPort->WriteS(_L8("AT&f\r"));
	if (r!=KErrNone)
		{
		CenteredPrintf(KDisplayMainY+1,_L("Error (%d) sending data"),r);
		TheConsole->Getch();
		}
	return(KErrNone);
	}

LOCAL_C TInt SendLongHayesCommand()
//
// Send Long hayes command
//
	{

	TInt r=TheSerialPort->WriteS(_L8("AT&f&f&f&f&f&f&f\r"));
	if (r!=KErrNone)
		{
		CenteredPrintf(KDisplayMainY+1,_L("Error (%d) sending data"),r);
		TheConsole->Getch();
		}
	return(KErrNone);
	}

const TInt KBufSize=0x100;
LOCAL_C TInt Loopback()
//
// Loopback data from Rx to Tx
//
	{

	CenteredPrintf(KDisplayMainY,_L("Loopback mode"));
	CenteredPrintf(KDisplayMainY+5,_L("Hit a key abort"));
	TheSerialPort->ResetBuffers();

	CCirBuffer* cbufPtr=new CCirBuffer;
	__ASSERT_ALWAYS(cbufPtr!=NULL,Panic(ECircBuf));
	TRAPD(r,cbufPtr->SetLengthL(KBufSize));
	__ASSERT_ALWAYS(r==KErrNone,Panic(ECircBuf));
	TRequestStatus kStat,rStat,tStat = 0;

	TBool TxActive=EFalse;
	TInt TxCount=0;
	TUint8 txChar;
	TPtr8 txPtr(&txChar,1);

	TUint8 rxChar;
	TPtr8 rxPtr(&rxChar,1);
	TheSerialPort->Read(rStat,rxPtr,1);

	TheConsole->Read(kStat);
	FOREVER
		{
		User::WaitForAnyRequest();
		if (rStat!=KRequestPending)
			{
			if (rStat.Int()!=KErrNone)
				{ // Rx error
				CenteredPrintf(KDisplayMainY+5,_L("Rx error(%d)"),rStat.Int());
				TheConsole->ReadCancel();
				User::WaitForRequest(kStat);
				goto LoopEnd;
				}
			cbufPtr->Put((TInt)rxChar);
			TheSerialPort->Read(rStat,rxPtr,1);
			if (!TxActive)
				{
				txChar=(TUint8)cbufPtr->Get();
				TheSerialPort->Write(tStat,txPtr,1);
				TxActive=ETrue;
				}
			}
		else if (TxActive && tStat!=KRequestPending)
			{
			if (tStat.Int()!=KErrNone)
				{ // Tx error
				CenteredPrintf(KDisplayMainY+5,_L("Tx error(%d)"),tStat.Int());
				TheSerialPort->ReadCancel();
				User::WaitForRequest(rStat);
				TheConsole->ReadCancel();
				User::WaitForRequest(kStat);
				TxActive=EFalse;
				goto LoopEnd;
				}
			TxCount++;
			TInt t=cbufPtr->Get();
			if (t==KErrGeneral)
				TxActive=EFalse;
			else
				{
				txChar=(TUint8)t;
				TheSerialPort->Write(tStat,txPtr,1);
				}
			}
		else if (kStat!=KRequestPending)
			{
			CenteredPrintf(KDisplayMainY+5,_L("Tx count (%d) - Hit another key"),TxCount);
			TheSerialPort->ReadCancel();
			User::WaitForRequest(rStat);
LoopEnd:
			if (TxActive)
				{
				TheSerialPort->WriteCancel();
				User::WaitForRequest(tStat);
				}
			delete cbufPtr;
			TheConsole->Getch();
			break;
			}
		else
			Panic(EStraySignal);
		}
	return(KErrNone);
	}

LOCAL_C TInt ToggleDtr()
//
// Toggle state of DTR signal
//
	{

	if (TheDtrState)
		{
		TheSerialPort->SetSignals(0,KSignalDTR); // Negate DTR
		TheDtrState=EFalse;
		}
	else
		{
		TheSerialPort->SetSignals(KSignalDTR,0); // Assert DTR
		TheDtrState=ETrue;
		}
	return(KErrNone);
	}

LOCAL_C TInt ToggleRts()
//
// Toggle state of RTS signal
//
	{

	if (TheRtsState)
		{
		TheSerialPort->SetSignals(0,KSignalRTS); // Negate RTS
		TheRtsState=EFalse;
		}
	else
		{
		TheSerialPort->SetSignals(KSignalRTS,0); // Assert RTS
		TheRtsState=ETrue;
		}
	return(KErrNone);
	}

LOCAL_C TInt SendXoff()
//
// Send XOFF
//
	{

	TInt r=TheSerialPort->WriteS(_L8("\x13"));
	if (r!=KErrNone)
		{
		CenteredPrintf(KDisplayMainY+1,_L("Error (%d) sending XOFF"),r);
		TheConsole->Getch();
		}
	return(KErrNone);
	}

LOCAL_C TInt ReceiveBlock()
//
// Receive a block
//
	{

	CenteredPrintf(KDisplayMainY,_L("Waiting to recieve a block. Hit a key to abort"));
	TheSerialPort->ResetBuffers();
	TRequestStatus kStat,rStat;
	TBuf8<KTestPatternSize> rdBuf(KTestPatternSize);
	TheSerialPort->Read(rStat,rdBuf);
	TheConsole->Read(kStat);
	User::WaitForRequest(kStat,rStat);
	if (rStat!=KRequestPending)
		{
		TheConsole->ReadCancel();
		User::WaitForRequest(kStat);
		if (rStat.Int()!=KErrNone)
			{
			CenteredPrintf(KDisplayMainY+5,_L("Rx error(%d)"),rStat.Int());
			TheConsole->Getch();
			}
		else if (rdBuf.Compare(TheTestBuf)!=0)
			DumpDescriptors(rdBuf,TheTestBuf);
		else
			{
			CenteredPrintf(KDisplayMainY+5,_L("Success"));
			TheConsole->Getch();
			}
		}
	else
		{
		TheSerialPort->ReadCancel();
		User::WaitForRequest(rStat);
		}
	return(KErrNone);
	}

LOCAL_C TInt TransmitBlock()
//
// Transmit a block
//
	{

	TInt r;
	CenteredPrintf(KDisplayMainY,_L("Hit a key to transmit a block"));
	while ((TUint)TheConsole->Getch()!=0x1b)
		{
		r=TheSerialPort->WriteS(TheTestBuf);
		if (r!=KErrNone)
			{
			CenteredPrintf(KDisplayMainY+1,_L("Error (%d) transmitting block"),r);
			TheConsole->Getch();
			}
		}
	return(KErrNone);
	}

LOCAL_C TInt SendXon()
//
// Send XON
//
	{

	TInt r=TheSerialPort->WriteS(_L8("\x11"));
	if (r!=KErrNone)
		{
		CenteredPrintf(KDisplayMainY+1,_L("Error (%d) sending XON"),r);
		TheConsole->Getch();
		}
	return(KErrNone);
	}

LOCAL_C void DceUtil()
//
// DCE Serial Driver test utilities
//
	{
	TBuf<20> b(_L("BDHLOQRSTXY\x1b"));

	FOREVER
		{
		TheConsole->ClearScreen();
		TPtrC br=BaudrateText(TheConfig.iRate);
		TPtrC dt=DtrText(TheDtrState);
		TPtrC rt=RtsText(TheRtsState);
		Heading(_L("T_DCEUTL 1.01 (Baudrate: %S DTR:%S RTS:%S)"),&br,&dt,&rt);
		Instructions(ETrue,_L("Change(B)aud Toggle(D)TR Send(H)ayes (L)oopBack Send X(O)FF"));
		Instructions(EFalse,_L("(Q)uit (R)xBlock Toggle RT(S) (T)xBlock Send(X)ON LongHayes(Y)?"));
		TChar c;
		do
			{
			c=(TUint)TheConsole->Getch();
			c.UpperCase();
			}
		while(b.Locate(c)==KErrNotFound);
		
		switch (c)
			{
			case 'B': 	// Change baudrate
				ChangeBaudrate();
				break;
			case 'D':   // Toggle state of DTR signal
				ToggleDtr();
				break;
			case 'H':  	// Send short hayes command
				SendHayesCommand();
				break;
			case 'L':  	// Loopback data from Rx to Tx
				Loopback();
				break;
			case 'O':   // Send XOFF
				SendXoff();
				break;
			case 'Q': case 0x1b: // Quit
				return;
			case 'R': 	// Receive a block
				ReceiveBlock();
				break;
			case 'S':   // Toggle state of RTS signal
				ToggleRts();
				break;
			case 'T': 	// Transmit a block
				TransmitBlock();
				break;
			case 'X':	// Send XON
				SendXon();
				break;
			case 'Y':  	// Send long hayes command
				SendLongHayesCommand();
				break;
			}
		}
	}

GLDEF_C TInt E32Main()
	{

	// Create console
	TRAPD(r,TheConsole=Console::NewL(_L("T_DCEUTL"),TSize(KConsFullScreen,KConsFullScreen)))
	__ASSERT_ALWAYS(r==KErrNone,Panic(ECreatingConsole));
	TheTestBuf.SetLength(KTestPatternSize);
	StripeMem(TheTestBuf,'A','Z');

	TBuf <0x100> cmd;
	User::CommandLine(cmd);
	TInt port=0;
	if ((cmd.Length()>0) && (cmd[0]>='1' && cmd[0]<='4'))
		port=(TInt)(cmd[0]-'0');

	// Load Device Drivers
	TheConsole->Printf(_L("Load PDD\n\r"));
    TBuf<9> pddName=PDD_NAME;
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
	__ASSERT_ALWAYS(r==KErrNone||r==KErrAlreadyExists,Panic(ELoadingPDD));
	TheConsole->Printf(_L("Load LDD\n\r"));
	r=User::LoadLogicalDevice(LDD_NAME);
	__ASSERT_ALWAYS(r==KErrNone||r==KErrAlreadyExists,Panic(ELoadingLDD));

	// Create RComm object
	TheConsole->Printf(_L("Create RComm object\n\r"));
	TheSerialPort=new RComm;
	__ASSERT_ALWAYS(TheSerialPort!=NULL,Panic(ECreatingRComm));

	// Open Serial Port
	TheConsole->Printf(_L("Open Serial Port (%d)\n\r"),port);
	r=TheSerialPort->Open(port);
	__ASSERT_ALWAYS(r==KErrNone,Panic(EOpeningPort));

	// Setup serial port
	TheConsole->Printf(_L("Setup serial port\n\r"));
	TheSerialPort->Config(TheConfigBuf);
	TheConfig.iRate=EBps9600;
	TheConfig.iDataBits=EData8;
	TheConfig.iStopBits=EStop1;
	TheConfig.iParity=EParityNone;
	TheConfig.iHandshake=(KConfigFreeRTS|KConfigFreeDTR); // So we can control them ourselves
	r=TheSerialPort->SetConfig(TheConfigBuf);
	__ASSERT_ALWAYS((r==KErrNone||r==KErrNotSupported),Panic(ESettingPort));
	if (r==KErrNotSupported)
		{
		// Port may not support the handshake settings
		TheConfig.iHandshake=0; 
		r=TheSerialPort->SetConfig(TheConfigBuf);
		__ASSERT_ALWAYS(r==KErrNone,Panic(ESettingPort));
		}

	// Assert DTR signal
	TheSerialPort->SetSignals(KSignalDTR,0); // Assert DTR
	TheDtrState=ETrue;
	// Assert RTS signal
	TheSerialPort->SetSignals(KSignalRTS,0); // Assert RTS
	TheRtsState=ETrue;

	DceUtil();

	TheSerialPort->Close();
	delete TheSerialPort;
	delete TheConsole;
	return(KErrNone);
	}


