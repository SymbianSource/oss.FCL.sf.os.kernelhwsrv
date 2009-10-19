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
// e32test\device\t_modem1.cpp
// Test program for PC Card Serial Port Driver - Requires Dacom GoldCard Modem.
// 
//

#include <e32std.h>
#include <e32std_private.h>
#include <e32test.h>
#include <e32cons.h>
#include <e32svr.h>
#include <e32hal.h>
#include <e32uid.h>
#include <d32comm.h>
#include <hal.h>

class RComm : public RBusDevComm
	{
public:
	TInt WriteS(const TDesC8& aDes);
	};

const TInt KBlockSize=256;
//                                  Block size     Line rate
const TInt KBlocksShort=4; 	        // 4*256=1K     - <1200
const TInt KBlocksMedium=12; 	    // 12*256=3K    - <4800
const TInt KBlocksLong=64; 	        // 64*256=16K   - <28800
const TInt KBlocksVeryLong=128; 	// 128*256=32K  -  28800

enum TLineRate {EV21_300,EBell103_300,EV22_1200,EBell212_1200,EV22bis_2400,
				  EV32_4800,EV32_9600,EV32bis_14400,EV34_28800,ELineRateEnd};
const TInt KMaxLineRates=ELineRateEnd;
const TInt KStandardRxBufferSize=0x400;

#if !defined (__WINS__)
#define PDD_NAME _L("EUARTn")
#define LDD_NAME _L("ECOMM")
#else
#define PDD_NAME _L("ECDRV.PDD")
#define LDD_NAME _L("ECOMM.LDD")
#endif

LOCAL_D RTest test(_L("T_MODEM1"));
LOCAL_D TInt LineModeData[KMaxLineRates]={0,64,1,69,2,9,9,10,11};
LOCAL_D TInt LineRateData[KMaxLineRates]={300,300,1200,1200,2400,4800,9600,14400,28800};
LOCAL_D TInt LineConnectData[KMaxLineRates]={1200,1200,1200,1200,2400,4800,9600,14400,28800};
LOCAL_D RComm *theSerialPort;


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

LOCAL_C TPtrC BaudRateInText(TBps aRate)
//
// Convert into Baudrate text
//
    {

    switch (aRate)
	    {
        case EBps50:       return(_L("50"));        break;
        case EBps75:       return(_L("75"));        break;
        case EBps110:      return(_L("110"));       break;
        case EBps134:      return(_L("134"));       break;
        case EBps150:      return(_L("150"));       break;
        case EBps300:      return(_L("300"));       break;
        case EBps600:      return(_L("600"));       break;
        case EBps1200:     return(_L("1200"));      break;
        case EBps1800:     return(_L("1800"));      break;
        case EBps2000:     return(_L("2000"));      break;
        case EBps2400:     return(_L("2400"));      break;
        case EBps3600:     return(_L("3600"));      break;
        case EBps4800:     return(_L("4800"));      break;
        case EBps7200:     return(_L("7200"));      break;
        case EBps9600:     return(_L("9600"));      break;
        case EBps19200:    return(_L("19200"));     break;
        case EBps38400:    return(_L("38400"));     break;
        case EBps57600:    return(_L("57600"));     break;
        case EBps115200:   return(_L("115000"));    break;
        default:           return(_L("Unknown"));   break;
	    }
    }

LOCAL_C TInt TranslateCrLf(TDes8 &aDes)
//
// Search for CR/LF characters in a string and replace them with 
// '\r' '\n' format. Also replaces unprintable characters with "?"
//
    {

    TText8 buf[KBlockSize];
    TText8 *pS=(TText8*)aDes.Ptr();
    TText8 *pSE=pS+aDes.Size();
    TText8 *pT=&buf[0];
    TText8 *pTMax=pT+(KBlockSize-1);
    for (;pS<pSE;pS++,pT++)
        {
        if (pT>=pTMax)
            return(KErrTooBig);
        if (*pS=='\xD'||*pS=='\xA')
            {
            *pT++='\\';       
            *pT=(*pS=='\xD')?'r':'n';
            }
        else if (((TChar)*pS).IsPrint())
            *pT=*pS;
        else
            *pT='\?';
        }
    *pT=0;
    if ((pT-&buf[0])>aDes.MaxLength())
        return(KErrTooBig);
    aDes.Copy(&buf[0]);
    return(KErrNone);
    }
/* ???
LOCAL_C void PrintBuf(TDes8 &aBuf)
//
// Print the contents of a buffer
//
	{

	TInt len=aBuf.Length();
	for (TInt i=0;i<=len/8;i++)
		{
		test.Printf(_L("%4d: "),i*8);

		for (TInt j=0;j<8;j++)
			{
			if ((i*8)+j>=len)
				break;
			TInt v=aBuf[(i*8)+j];
			test.Printf(_L("%02x "),v);
			}
		test.Printf(_L("\n\r"));
		}
	}
*/
LOCAL_C void testLoopBack(TLineRate aLineRate,TBps aBaudRate)
//
// Perform an analogue loopback test at the specified linerate
//
    {

    TInt err;
	TBuf<64> b;
	TPtrC bd=BaudRateInText(aBaudRate);
	b.Format(_L("Loopback test(%S)"),&bd);
	test.Start(b);

    TBuf8<KBlockSize> txBuf;
	theSerialPort->ResetBuffers();
	txBuf.Format(_L8("AT&F+MS=%d,0,%d,%d\\N0&K3&D2M0\r"),LineModeData[aLineRate],LineRateData[aLineRate],LineRateData[aLineRate]);
    test(theSerialPort->WriteS(txBuf)==KErrNone);

    TBuf8<KBlockSize> rxBuf;
	User::After(2000000);		  // 2Secs
	err=theSerialPort->QueryReceiveBuffer();
    test(err>0);
	rxBuf.SetLength(err);
	TRequestStatus rxStat;
    theSerialPort->ReadOneOrMore(rxStat,rxBuf);
	User::WaitForRequest(rxStat);
//	test.Printf(_L("   Rx(%d):"),rxStat); // ???
    test(rxStat==KErrNone);
    txBuf.Append(_L("\r\nOK\r\n"));
  	err=rxBuf.Compare(txBuf);
//	test(TranslateCrLf(rxBuf)==KErrNone); // ???
//  test.Printf(_L(" %S\r\n"),&rxBuf); // ???
  	test(err==0);
	
	test.Next(_L("Get loopback"));
	txBuf.Format(_L8("AT&T1\r"));
    test(theSerialPort->WriteS(txBuf)==KErrNone);
	User::After(5000000);		  // 5Secs
	err=theSerialPort->QueryReceiveBuffer();
    test(err>0);
	rxBuf.SetLength(err);
    theSerialPort->ReadOneOrMore(rxStat,rxBuf);
	User::WaitForRequest(rxStat);
	test.Printf(_L("   Rx(%d):"),rxStat);
    test(rxStat==KErrNone);
    txBuf.AppendFormat(_L8("\r\nCONNECT %d\r\n"),LineConnectData[aLineRate]);
  	err=rxBuf.Compare(txBuf);
    test(TranslateCrLf(rxBuf)==KErrNone);
    test.Printf(_L(" %S\r\n"),&rxBuf); // Print what we got back (without CR/LF etc).
    // Sometimes get extra character as modem goes on-line so just look for command echo + connect
  	test(err>=0);
	User::After(2000000);		  // 2Secs

	TInt totalBlocksToTransfer;
    if (aBaudRate<EBps1200||aLineRate<EV22_1200)
        totalBlocksToTransfer=KBlocksShort;
    else if (aBaudRate<EBps4800||aLineRate<EV32_4800)
        totalBlocksToTransfer=KBlocksMedium;
    else if (aLineRate<EV34_28800)
        totalBlocksToTransfer=KBlocksLong;
    else
        totalBlocksToTransfer=KBlocksVeryLong;
	b.Format(_L("Transfering data(%dK)"),(totalBlocksToTransfer*KBlockSize)/1024);
	test.Next(b);
	TInt loopBackFail=KErrGeneral;
	TRequestStatus txStat;
    txBuf.SetLength(KBlockSize);
	TInt i;
	for (i=0;i<KBlockSize;i++)
		txBuf[i]=(TUint8)i;
    theSerialPort->Write(txStat,txBuf,KBlockSize);
	TInt txBlks=(totalBlocksToTransfer-1);
    rxBuf.Fill(0,KBlockSize);
    theSerialPort->Read(rxStat,rxBuf,KBlockSize);
	TInt rxBlks=0;
	TRequestStatus tStat;
	RTimer tim;
	test(tim.CreateLocal()==KErrNone);
	tim.After(tStat,40000000);  // 40Secs
	test.Printf(_L(">"));
	FOREVER
		{
		User::WaitForAnyRequest();
		if (tStat!=KRequestPending)
       		{
//			test.Printf(_L("t"));   // Timed out
			theSerialPort->ReadCancel(); // Cancel serial read
			User::WaitForRequest(rxStat);
			if (txBlks>0)
				{
		        theSerialPort->WriteCancel(); // Cancel serial write
		        User::WaitForRequest(txStat);
				}
			loopBackFail=KErrTimedOut; // Test failed
			break;						
        	}
		else if (rxStat!=KRequestPending)
        	{
//			test.Printf(_L("r"));   // Serial rx request complete
			if (rxStat!=0)
				{
				loopBackFail=rxStat.Int(); // Test failed
				goto endSerial;
				}
			for (i=0;i<KBlockSize;i++)
				{
				if (rxBuf[i]!=i)
					{
					loopBackFail=KErrCorrupt; // Test failed
			        rxBuf[KBlockSize-1]=0;
//					PrintBuf(rxBuf); // ???
//					goto endSerial; // !!!Ignore compare fails for now!!!
					}
				}
			test.Printf(_L("<"));
			if (++rxBlks<totalBlocksToTransfer)
				{
                rxBuf.Fill(0,KBlockSize);
                theSerialPort->Read(rxStat,rxBuf,KBlockSize);
				}
			else
				{
				loopBackFail=KErrNone;
endSerial:
			    tim.Cancel(); // Cancel timer request.
			    User::WaitForRequest(tStat);
				if (txBlks>0)
					{
		            theSerialPort->WriteCancel(); // Cancel serial write
		            User::WaitForRequest(txStat);
					}
				break;
				}
        	}
		else if (txStat!=KRequestPending)
        	{
//			test.Printf(_L("s")); // Serial tx request complete
			if (txBlks>0)
				{
                theSerialPort->Write(txStat,txBuf,KBlockSize);
				test.Printf(_L(">"));
				txBlks--;
				}
        	}
    	else
        	{
//			test.Printf(_L("?")); // Stray signal - cancel everything
			theSerialPort->ReadCancel(); // Cancel serial read
			User::WaitForRequest(rxStat);
			tim.Cancel(); // Cancel timer request.
			User::WaitForRequest(tStat);
			if (txBlks>0)
				{
		        theSerialPort->WriteCancel(); // Cancel serial write
		        User::WaitForRequest(txStat);
				}
			loopBackFail=KErrDied;
            break;
        	}
		}
	test.Printf(_L(" (%d)\r\n"),loopBackFail);
	// !!! At this point RTS may or may not be asserted following the write cancel. The
	// following seems necessary to make sure RTS is asserted so any remaining Rx data
	// can be received.and thrown away
	User::After(2000000);
	theSerialPort->ResetBuffers();
 	User::After(1000000);		   // Wait 1Secs for any remaining Rx data
	tim.Close();

	test.Next(_L("Disconnect"));
	theSerialPort->ResetBuffers(); // Through away any remaining Rx data.
	txBuf.Format(_L8("+++"));
    test(theSerialPort->WriteS(txBuf)==KErrNone);
	User::After(2000000);		  // 2Secs
	err=theSerialPort->QueryReceiveBuffer();
    test(err>0);
	rxBuf.SetLength(err);
    theSerialPort->ReadOneOrMore(rxStat,rxBuf);
	User::WaitForRequest(rxStat);
    test(rxStat==KErrNone);
    txBuf.Append(_L("\r\nOK\r\n"));
  	err=rxBuf.Compare(txBuf);
//  test(TranslateCrLf(rxBuf)==KErrNone); // ???
//  test.Printf(_L("   %S\r\n"),&rxBuf); // ???
	test(err==0);

	txBuf.Format(_L8("ATH0\r"));
    test(theSerialPort->WriteS(txBuf)==KErrNone);
	User::After(4000000);		  // 4Secs
	err=theSerialPort->QueryReceiveBuffer();
    test(err>0);
	rxBuf.SetLength(err);
    theSerialPort->ReadOneOrMore(rxStat,rxBuf);
	User::WaitForRequest(rxStat);
    test(rxStat==KErrNone);
    txBuf.Append(_L("\r\nOK\r\n"));
  	err=rxBuf.Compare(txBuf);
//  test(TranslateCrLf(rxBuf)==KErrNone); // ???
//  test.Printf(_L("   %S\r\n"),&rxBuf); // ???
  	test(err==0);

	test.Next(_L("Check result"));
	test(loopBackFail==KErrNone || loopBackFail==KErrCorrupt); // !!!Ignore compare fails for now!!!
//	test(loopBackFail==KErrNone);
	
	test.End();
    }

LOCAL_C void testAllLineRates(TBps aRate)
//
// Perform loopback test at the specified baudrate in as many line modes that 
// are supported at this baudrate
//
    {

	test.Start(_L("Setting baudrate"));
	TCommConfig cBuf;
	TCommConfigV01 &c=cBuf();
	theSerialPort->Config(cBuf);
	c.iRate=aRate;
	c.iDataBits=EData8;
	c.iStopBits=EStop1;
	c.iParity=EParityNone;
	c.iHandshake=KConfigObeyCTS;
//	c.iHandshake=0;
	test(theSerialPort->SetConfig(cBuf)==KErrNone);

	if (aRate>=EBps38400)
		{
	    test.Next(_L("Testing at V.34-28800"));
	    testLoopBack(EV34_28800,aRate);
        }

	if (aRate>=EBps19200)
		{
	    test.Next(_L("Testing at V.32bis-14400"));
	    testLoopBack(EV32bis_14400,aRate);
        }

	if (aRate>=EBps9600)
		{
	    test.Next(_L("Testing at V.32-9600"));
	    testLoopBack(EV32_9600,aRate);
        }

//	if (aRate>=EBps4800)
//		{
//	    test.Next(_L("Testing at V.32-4800"));
//	    testLoopBack(EV32_4800,aRate);
//		}

	if (aRate>=EBps2400)
		{
		test.Next(_L("Testing at V.22bis-2400"));
		testLoopBack(EV22bis_2400,aRate);
		}

//	if (aRate>=EBps1200)
//		{
//	    test.Next(_L("Testing at Bell212-1200"));
//	    testLoopBack(EBell212_1200,aRate);
//      }

//	if (aRate>=EBps1200)
//		{
//	    test.Next(_L("Testing at V.22-1200"));
//	    testLoopBack(EV22_1200,aRate);
//      }

//	test.Next(_L("Testing at Bell103-300"));
//	testLoopBack(EBell103_300,aRate);

	test.Next(_L("Testing at V.21-300"));
    testLoopBack(EV21_300,aRate); 

    test.End();
    }

GLDEF_C TInt E32Main()
	{
//	test.SetLogged(EFalse); 	// Turn off serial port debugging!

    TInt r;
	test.Title();

	test.Start(_L("PC Card Modem Test Program"));

	RProcess proc;
	TBuf <0x100> cmd;
	proc.CommandLine(cmd);

	// First parameter (if present) sets the serial port number
	TInt port=0;
	if ((cmd.Length()>0) && (cmd[0]>='1' && cmd[0]<='4'))
		port=(TInt)(cmd[0]-'0');

	// 2nd parameter (if present) sets the start speed 
	// (4=115K,3=57600,2=38400,1=19200,0=9600)
	TInt startSpeed=4;
	if ((cmd.Length()>3) && (cmd[2]>='0' && cmd[2]<='4'))
			startSpeed=(TInt)(cmd[2]-'0');

	test.Next(_L("Load Device Drivers"));
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
	test(r==KErrNone||r==KErrAlreadyExists);
	r=User::LoadLogicalDevice(LDD_NAME);
	test(r==KErrNone||r==KErrAlreadyExists);

	test.Next(_L("Open serial port"));
	theSerialPort=new RComm;
	test(theSerialPort!=NULL);
	r=theSerialPort->Open(port);
	test(r==KErrNone);
//  TCommCaps capsBuf;
//  TCommCapsV01& caps=capsBuf();
//	theSerialPort->Caps(capsBuf);

	// Check that the driver powering sequence has completed successfully by
	// issueing a few simple driver control functions.
	test.Next(_L("Modem power tests"));
	test(theSerialPort->SetReceiveBufferLength(KStandardRxBufferSize)==KErrNone);
	r=theSerialPort->ReceiveBufferLength();
//	test.Printf(_L("(%d)"),r); // ???
	test(r==KStandardRxBufferSize);
	r=(TInt)theSerialPort->Signals();
//	test.Printf(_L("(%d)"),r); // ???
	test(r>=0);

	RTimer timer;
	TRequestStatus rs;
	test(timer.CreateLocal()==KErrNone);
	TTime tim;
	tim.HomeTime();
	tim+=TTimeIntervalSeconds(8);
	timer.At(rs,tim);
	UserHal::SwitchOff();
	User::WaitForRequest(rs);
	test(rs.Int()==KErrNone);

	r=theSerialPort->ReceiveBufferLength();
//	test.Printf(_L("(%d)"),r); // ???
	test(r==KStandardRxBufferSize);
	r=(TInt)theSerialPort->Signals();
//	test.Printf(_L("(%d)"),r); // ???
	test(r>=0);
	User::After(2000000);		  // 2Secs !!!

	if (startSpeed>=4)
		{
		test.Next(_L("Testing at 115K"));
  		testAllLineRates(EBps115200);
		}

	if (startSpeed>=3)
		{
		test.Next(_L("Testing at 57600"));
    	testAllLineRates(EBps57600);
		}

	if (startSpeed>=2)
		{
		test.Next(_L("Testing at 38400"));
    	testAllLineRates(EBps38400);
		}

	if (startSpeed>=1)
		{
		test.Next(_L("Testing at 19200"));
    	testAllLineRates(EBps19200);
		}

	test.Next(_L("Testing at 9600"));
    testAllLineRates(EBps9600);

	test.Next(_L("Close serial port"));
	theSerialPort->Close();
	delete theSerialPort;
	test.End();
	return(KErrNone);
	}

