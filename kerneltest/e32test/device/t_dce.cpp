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
// e32test\device\t_dce.cpp
// 
//

// Cogent uses two port loopback, Linda doesn't
#define _TWO_PORT_LOOPBACK_ 

#include <e32base.h>
#include <e32base_private.h>
#include <e32test.h>
#include <e32cons.h>
#include <e32svr.h>
#include <e32hal.h>
#include <d32comm.h>
#include <e32uid.h>

const TInt KUnit0=0;
const TInt KUnit1=1;
const TInt KUnit2=2;

#ifdef _TWO_PORT_LOOPBACK_
#define NOTIFY_TIMEOUT 2000000 // 2 seconds
#else
#define NOTIFY_TIMEOUT 10000000 // 10 seconds
#endif

const TInt KTestPatternSize=250;

#if defined (__WINS__)
// Running this test in WINS is fairly pointless since WINS uses the DTE driver.
// The WINS version was just to get the basic test program working - i.e. to 
// speed up the debug process.
#define PDD_NAME _L("ECDRV.PDD")
#define LDD_NAME _L("ECOMM.LDD")
const TInt KTestUnitDte=KUnit0;
const TInt KTestUnitDce=KUnit1;
const TInt KChangeSigIn=KSignalCTS; // DTE driver used on WINS so read DSR rather than RTS
const TInt KOtherSigIn=KSignalDCD; // DTE driver used on WINS so read DCD rather than DTR
const TInt KChangeSigOut=KSignalRTS;
#else
#define DTEPDD_NAME _L("EUART3")
//#define DCEPDD_NAME _L("EUART1DCE") // Linda
#define DCEPDD_NAME _L("EUART1") // Cogent
#define DTELDD_NAME _L("ECOMM")
#define DCELDD_NAME _L("ECOMMDCE")
const TInt KTestUnitDte=KUnit2;
const TInt KTestUnitDce=KUnit0;
const TInt KChangeSigIn=KSignalRTS; // DCE input - Cogent
const TInt KOtherSigIn=KSignalDTR; // DCE input - Cogent
const TInt KChangeSigOut=KSignalRTS; // DTE output - Cogent
//const TInt KChangeSigIn=KSignalDTR; // DCE input - Linda
//const TInt KOtherSigIn=KSignalRTS; // DCE input - Linda
//const TInt KChangeSigOut=KSignalDTR; // DTE output - Linda
#endif

//	Our own comms object with synchronous writes
class RComm : public RBusDevComm
	{
public:
	TInt WriteS(const TDesC8& aDes);
	TInt WriteS(const TDesC8& aDes,TInt aLength);
	};

LOCAL_D RTest test(_L("T_DCE"));
RComm* TheDteSerialPort;
#if defined (__WINS__)
RBusDevComm* TheDceSerialPort;
#else
RBusDevCommDCE* TheDceSerialPort;
#endif

TCommConfig TheConfigDceBuf;
TCommConfigV01& TheConfigDce=TheConfigDceBuf();
TCommConfig TheConfigDteBuf;
TCommConfigV01& TheConfigDte=TheConfigDteBuf();
TCommCaps2 TheCapsDceBuf;
TCommCapsV02& TheCapsDce=TheCapsDceBuf();


TInt RComm::WriteS(const TDesC8& aDes)

//	Syncronous write

	{
	return(WriteS(aDes,aDes.Length()));
	}
	
TInt RComm::WriteS(const TDesC8& aDes,TInt aLength)

//	Syncronous write

	{

	TRequestStatus s;
	Write(s,aDes,aLength);
	User::WaitForRequest(s);
	return(s.Int());
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

#define COLUMN_HEADER _L("            RxBuf            |          Expected       \r\n")
LOCAL_C void DumpDescriptors(TDes8 &aLeft,TDes8 &aRight)
//
//
//
	{

	TBuf<80> b;
	test.Printf(_L("Compare failed:\r\n"));
	TInt minLen=Min(aLeft.Length(),aRight.Length());
	test.Printf(COLUMN_HEADER);
	TInt i=0;
	TInt j=0;
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
		b.Append(_L("\r\n"));
		test.Printf(b);
		i+=8;
		if ((i%64)==0)
			test.Getch();
		}
	}

LOCAL_C TInt CheckedWrite(TInt aBufSize)
// 
// Write/Read loopback test - requires either a loopback connector or a
// connected device looping back the data (running T_DCEUTL).
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

	TheDceSerialPort->Read(readStatus,inDes,aBufSize+1);
	test(readStatus==KErrGeneral);

	TheDceSerialPort->Read(readStatus,inDes);
	test(readStatus==KRequestPending);

    TInt ret;
#if defined (_TWO_PORT_LOOPBACK_)
	ret=TheDteSerialPort->WriteS(outDes,aBufSize);
#else
	TRequestStatus ws;
	TheDceSerialPort->Write(ws,outDes,aBufSize);
	User::WaitForRequest(ws);
	ret=ws.Int();
#endif
	test(ret==KErrNone);
	const TUint KTimeOut=6000000;
	tim.After(timeStatus,KTimeOut);
	test(timeStatus==KRequestPending);
	User::WaitForRequest(readStatus,timeStatus);
	if (timeStatus==KErrNone)
		{
		TheDceSerialPort->ReadCancel();
		User::WaitForRequest(readStatus);
		test.Printf(_L("Timed Out!\n\r"));
		test.Getch();
		test(FALSE);
		}
	else
		{
		tim.Cancel();
		User::WaitForRequest(timeStatus);
		if (readStatus!=KErrNone)
			{
			test.Printf(_L("Read Failed! (%d)\n\r"),readStatus.Int());
			test.Getch();
			test(FALSE);
			}
		test(readStatus==KErrNone);
		test.Printf(_L("Read %d of %d\n\r"),inDes.Length(),outDes.Length());
		ret=outDes.Compare(inDes);
		if (ret!=0)
			DumpDescriptors(inDes,outDes);
  		test(ret==0);
		}

	tim.Close();
	delete inBuf;
	delete outBuf;

	return inDes.Length();
	}

LOCAL_C TInt TestingNotifySignalChange(TUint aChangeSignal)
//
// Test NotifySignalChange()
//
	{

	RTimer tim;
	tim.CreateLocal();
	TRequestStatus notifStatus;
	TRequestStatus timeStatus;

	if (!(TheCapsDce.iNotificationCaps & KNotifySignalsChangeSupported))
        test.Printf(_L("Signal change notification not supported on this platform"));
    else
        {
		test.Next(_L("Testing NotifySignalChange() with no mask set"));
#if defined (_TWO_PORT_LOOPBACK_)
	    TheDteSerialPort->SetSignals(0,KChangeSigOut); // Clear
#else
		test.Printf(_L("Make sure DTR negated(??D) - hit a key to start\n\r"));
		test.Getch();
		test.Printf(_L("10 seconds to assert DTR(D)\n\r"));
#endif
	
	    TUint signals=0;
	    TheDceSerialPort->NotifySignalChange(notifStatus,signals);
	    test(notifStatus==KRequestPending);
	    const TUint KTimeOut=NOTIFY_TIMEOUT;
	    tim.After(timeStatus,KTimeOut);
	    test(timeStatus==KRequestPending);
#if defined (_TWO_PORT_LOOPBACK_)
	    TheDteSerialPort->SetSignals(KChangeSigOut,0); // Set
#endif
 	    User::WaitForRequest(notifStatus,timeStatus);
	    if (notifStatus==KErrNone)
		    {
		    tim.Cancel();
			User::WaitForRequest(timeStatus);
#ifndef __WINS__
			TUint sigmask=((aChangeSignal*KSignalChanged)|aChangeSignal);
		    test((signals&sigmask)==sigmask);
#endif
		    }
	    else
		    {
		    TheDceSerialPort->NotifySignalChangeCancel();
			User::WaitForRequest(notifStatus);
		    test.Printf(_L("Timed Out!\n\r"));
			test.Getch();
		    test(FALSE);
		    }
	    TUint rdSignals=TheDceSerialPort->Signals();
		test(rdSignals&aChangeSignal);

		test.Next(_L("Testing NotifySignalChange() when not expected"));
        // Test notification doesn't happen with mask set to some other signal
	    signals=0;
	    TheDceSerialPort->NotifySignalChange(notifStatus,signals,KOtherSigIn);
	    test(notifStatus==KRequestPending);
	    tim.After(timeStatus,KTimeOut);
	    test(timeStatus==KRequestPending);
#if defined (_TWO_PORT_LOOPBACK_)
	    TheDteSerialPort->SetSignals(0,KChangeSigOut); // Clear
#else
		test.Printf(_L("10 seconds to negate DTR(D)\n\r"));
#endif
	    User::WaitForRequest(notifStatus,timeStatus);
	    if (notifStatus==KErrNone)
		    {
		    tim.Cancel();
			User::WaitForRequest(timeStatus);
		    test(FALSE);
		    }
	    else
			{
		    test(timeStatus==KErrNone); // Success
		    TheDceSerialPort->NotifySignalChangeCancel(); // Tests cancel 
			User::WaitForRequest(notifStatus);
			}
#ifndef __WINS__
	    rdSignals=TheDceSerialPort->Signals();
		test(!(rdSignals&aChangeSignal));
#endif

		test.Next(_L("Testing NotifySignalChange() with mask set"));
        // Test notification happens with mask set to this signal
	    signals=0;
	    TheDceSerialPort->NotifySignalChange(notifStatus,signals,aChangeSignal);
	    test(notifStatus==KRequestPending);
	    tim.After(timeStatus,KTimeOut);
	    test(timeStatus==KRequestPending);
#if defined (_TWO_PORT_LOOPBACK_)
	    TheDteSerialPort->SetSignals(KChangeSigOut,0); // Set
#else
		test.Printf(_L("10 seconds to assert DTR(D)\n\r"));
#endif
	    User::WaitForRequest(notifStatus,timeStatus);
	    if (notifStatus==KErrNone)
		    {
		    tim.Cancel();
			User::WaitForRequest(timeStatus);
		    test(signals==((aChangeSignal*KSignalChanged)|aChangeSignal));
		    }
	    else
		    {
		    TheDceSerialPort->NotifySignalChangeCancel();
			User::WaitForRequest(notifStatus);
		    test.Printf(_L("Timed Out!\n\r"));
			test.Getch();
		    test(FALSE);
		    }
	    rdSignals=TheDceSerialPort->Signals();
		test(rdSignals&aChangeSignal);
        }

	return(KErrNone);
	}

LOCAL_C TInt TestingNotifyReceiveDataAvailable()
//
// Test NotifyReceiveDataAvailable()
//
	{

	RTimer tim;
	tim.CreateLocal();
	TRequestStatus notifStatus;
	TRequestStatus timeStatus;

	if (!(TheCapsDce.iNotificationCaps & KNotifyDataAvailableSupported))
        test.Printf(_L("Data available notification not supported on this platform"));
    else
        {
		test.Next(_L("Testing NotifyReceiveDataAvailable()"));
#if !defined (_TWO_PORT_LOOPBACK_)
		test.Printf(_L("Hit a key to start\n\r"));
		test.Getch();
#endif
		TPtrC8 buf1(_S8("AT&f\r"));
	    TBuf8<0x10> buf2(0x10);

        TheDceSerialPort->ResetBuffers();
	    TheDceSerialPort->NotifyReceiveDataAvailable(notifStatus);
	    test(notifStatus==KRequestPending);
	    const TUint KTimeOut=NOTIFY_TIMEOUT;
	    tim.After(timeStatus,KTimeOut);
	    test(timeStatus==KRequestPending);
#if defined (_TWO_PORT_LOOPBACK_)
        test(TheDteSerialPort->WriteS(buf1)==KErrNone);
#else
		test.Printf(_L("10 seconds to send hayes command(H)\n\r"));
#endif
	    User::WaitForRequest(notifStatus,timeStatus);
	    if (notifStatus==KErrNone)
		    {
		    tim.Cancel();
			User::WaitForRequest(timeStatus);
		    }
	    else
		    {
	        TheDceSerialPort->NotifyReceiveDataAvailableCancel();
		    test.Printf(_L("Timed Out!\n\r"));
			test.Getch();
		    test(FALSE);
		    }
		User::After(500000);
		TInt len=TheDceSerialPort->QueryReceiveBuffer();
//        test(len==buf1.Length());
	    buf2.FillZ();
	    TheDceSerialPort->Read(notifStatus,buf2,len);
		User::WaitForRequest(notifStatus);
		TInt ret=buf2.Compare(buf1);
  	    if (ret!=0)
			{
			test.Printf(_L("Compare error\r\n"));
			test.Getch();
			}
		test(ret==0);
        }
	return(KErrNone);
	}

#if !defined (__WINS__)
LOCAL_C TInt TestingFlowControlChange()
//
// Test NotifyFlowControlChange()
//
	{

	RTimer tim;
	tim.CreateLocal();
	TRequestStatus notifStatus;
	TRequestStatus timeStatus;

	if (!(TheCapsDce.iNotificationCaps & KNotifyFlowControlChangeSupported))
        test.Printf(_L("Flow Control change notification not supported on this platform"));
    else
        {
#if !defined (_TWO_PORT_LOOPBACK_)
		test.Printf(_L("Hit a key to start\n\r"));
		test.Getch();
#endif

		test.Next(_L("Testing GetFlowControlStatus()"));
		TheConfigDce.iHandshake=KConfigObeyXoff;
		test(TheDceSerialPort->SetConfig(TheConfigDceBuf)==KErrNone);
		TFlowControl fc;
		TheDceSerialPort->GetFlowControlStatus(fc);
		test(fc==EFlowControlOff);

		test.Next(_L("Testing NotifyFlowControlChange() with s/w flow control"));
	    TheDceSerialPort->NotifyFlowControlChange(notifStatus);
	    test(notifStatus==KRequestPending);
	    const TUint KTimeOut=NOTIFY_TIMEOUT;
	    tim.After(timeStatus,KTimeOut);
	    test(timeStatus==KRequestPending);
#if defined (_TWO_PORT_LOOPBACK_)
        test(TheDteSerialPort->WriteS(_L8("\x13"))==KErrNone); // XOFF
#else
		test.Printf(_L("10 seconds to send XOFF(O)\n\r"));
#endif
	    User::WaitForRequest(notifStatus,timeStatus);
	    if (notifStatus==KErrNone)
		    {
		    tim.Cancel();
			User::WaitForRequest(timeStatus);
		    }
	    else
		    {
	        TheDceSerialPort->NotifyFlowControlChangeCancel();
		    test.Printf(_L("Timed Out!\n\r"));
			test.Getch();
		    test(FALSE);
		    }

		test.Next(_L("Testing GetFlowControlStatus() again"));
		TheDceSerialPort->GetFlowControlStatus(fc);
		test(fc==EFlowControlOn);

#if defined (_TWO_PORT_LOOPBACK_)
        test(TheDteSerialPort->WriteS(_L8("\x11"))==KErrNone); // XON
#else
		test.Printf(_L("Send XON(X)\n\r"));
#endif
		User::After(1000000);		  // 1Sec
		TheDceSerialPort->GetFlowControlStatus(fc);
		test(fc==EFlowControlOff);
		TheConfigDce.iHandshake=0;
		test(TheDceSerialPort->SetConfig(TheConfigDceBuf)==KErrNone);

#if defined (_TWO_PORT_LOOPBACK_)
		test.Next(_L("Testing NotifyFlowControlChange() with h/w flow control"));
		TheConfigDce.iHandshake=KConfigObeyRTS;
		test(TheDceSerialPort->SetConfig(TheConfigDceBuf)==KErrNone);
		TheDceSerialPort->GetFlowControlStatus(fc);
		test(fc==EFlowControlOff);

	    TheDceSerialPort->NotifyFlowControlChange(notifStatus);
	    test(notifStatus==KRequestPending);
	    tim.After(timeStatus,KTimeOut);
	    test(timeStatus==KRequestPending);
		TheDteSerialPort->SetSignals(0,KChangeSigOut); // Clear
	    User::WaitForRequest(notifStatus,timeStatus);
	    if (notifStatus==KErrNone)
		    {
		    tim.Cancel();
			User::WaitForRequest(timeStatus);
		    }
	    else
		    {
	        TheDceSerialPort->NotifyFlowControlChangeCancel();
		    test.Printf(_L("Timed Out!\n\r"));
			test.Getch();
		    test(FALSE);
		    }

		TheDceSerialPort->GetFlowControlStatus(fc);
		test(fc==EFlowControlOn);

		TheDteSerialPort->SetSignals(KChangeSigOut,0); // Set
		User::After(1000000);		  // 1Sec
		TheDceSerialPort->GetFlowControlStatus(fc);
		test(fc==EFlowControlOff);

		test.Next(_L("Testing NotifyFlowControlChange() when not expected"));
        // Test notification doesn't happen when s/w flow control happens
	    TheDceSerialPort->NotifyFlowControlChange(notifStatus);
	    test(notifStatus==KRequestPending);
	    tim.After(timeStatus,KTimeOut);
	    test(timeStatus==KRequestPending);
        test(TheDteSerialPort->WriteS(_L8("\x13"))==KErrNone); // XOFF
	    User::WaitForRequest(notifStatus,timeStatus);
	    if (notifStatus==KErrNone)
		    {
		    tim.Cancel();
			User::WaitForRequest(timeStatus);
		    test(FALSE);
		    }
	    else
		    {
		    test(timeStatus==KErrNone); // Success
	        TheDceSerialPort->NotifyFlowControlChangeCancel(); // Tests cancel
			User::WaitForRequest(notifStatus);
		    }
        test(TheDteSerialPort->WriteS(_L8("\x11"))==KErrNone); // XON
		TheDceSerialPort->GetFlowControlStatus(fc);
		test(fc==EFlowControlOff);

		TheConfigDce.iHandshake=0;
		test(TheDceSerialPort->SetConfig(TheConfigDceBuf)==KErrNone);
#endif
        }
	return(KErrNone);
	}

LOCAL_C TInt TestingNotifyConfigChange()
//
// Test NotifyConfigChange()
//
	{

	RTimer tim;
	tim.CreateLocal();
	TRequestStatus notifStatus;
	TRequestStatus timeStatus;

	if (!(TheCapsDce.iNotificationCaps & KNotifyRateChangeSupported))
        test.Printf(_L("Rate change notification not supported on this platform"));
    else
        {
		test.Next(_L("Testing NotifyConfigChange()"));
#if defined (_TWO_PORT_LOOPBACK_)
	    TheConfigDte.iRate=EBps9600;
	    test(TheDteSerialPort->SetConfig(TheConfigDteBuf)==KErrNone);
#else
		test.Printf(_L("Change baudrate to 9600(BB)\n\r"));
		test.Printf(_L("Hit a key to start\n\r"));
		test.Getch();
#endif
		TCommNotificationPckg cmBuf;
		TCommNotificationV01& cm=cmBuf();

		// Test requesting before autobauding is enabled
	    TheDceSerialPort->NotifyConfigChange(notifStatus,cmBuf);
	    User::WaitForRequest(notifStatus);
	    test(notifStatus==KErrGeneral);

		// Enable autobauding
		TheConfigDce.iRate=EBpsAutobaud;  
		test(TheDceSerialPort->SetConfig(TheConfigDceBuf)==KErrNone);

		cm.iChangedMembers=0;
		cm.iRate=EBps50;
	    TheDceSerialPort->NotifyConfigChange(notifStatus,cmBuf);
	    test(notifStatus==KRequestPending);
	    const TUint KTimeOut=NOTIFY_TIMEOUT;
	    tim.After(timeStatus,KTimeOut);
	    test(timeStatus==KRequestPending);
#if defined (_TWO_PORT_LOOPBACK_)
        test(TheDteSerialPort->WriteS(_L8("AT&f\r"))==KErrNone);
#else
		test.Printf(_L("10 seconds to send hayes command(H)\n\r"));
#endif
	    User::WaitForRequest(notifStatus,timeStatus);
	    if (notifStatus==KErrNone)
		    {
		    tim.Cancel();
			User::WaitForRequest(timeStatus);
			test(cm.iChangedMembers==KRateChanged);
			test(cm.iRate==EBps9600);
		    }
	    else
		    {
	        TheDceSerialPort->NotifyConfigChangeCancel();
		    test.Printf(_L("Timed Out!\n\r"));
			test.Getch();
		    test(FALSE);
		    }
        }
	return(KErrNone);
	}
#endif

GLDEF_C TInt E32Main()
//
// Test DCE serial driver
//
    {

	test.Title();
	test.Start(_L("Turn off logging"));
	test.SetLogged(EFalse);//turn off serial port debugging!

	TInt r;
	test.Next(_L("Load drivers"));
#if defined (__WINS__)
	r=User::LoadPhysicalDevice(PDD_NAME);
	test.Printf(_L("Load EUART Return %d\n\r"),r);
	r=User::LoadLogicalDevice(LDD_NAME);
	test.Printf(_L("Load ECOMM Return %d\n\r"),r);
	TheDceSerialPort=new RBusDevComm;
#else
	r=User::LoadPhysicalDevice(DTEPDD_NAME);
	test.Printf(_L("Load DTE EUART Return %d\n\r"),r);
	r=User::LoadPhysicalDevice(DCEPDD_NAME);
	test.Printf(_L("Load DCE EUART Return %d\n\r"),r);
	r=User::LoadLogicalDevice(DTELDD_NAME);
	test.Printf(_L("Load DTE ECOMM Return %d\n\r"),r);
	r=User::LoadLogicalDevice(DCELDD_NAME);
	test.Printf(_L("Load DCE ECOMM Return %d\n\r"),r);
	TheDceSerialPort=new RBusDevCommDCE;
#endif
	test(TheDceSerialPort!=NULL);
	TheDteSerialPort=new RComm;
	test(TheDteSerialPort!=NULL);
//
	test.Next(_L("Open:"));
	r=TheDceSerialPort->Open(KTestUnitDce);
	test.Printf(_L("Open(DCE)=%d\n\r"),r);
	test(r==KErrNone);
	r=TheDteSerialPort->Open(KTestUnitDte);
	test.Printf(_L("Open(DTE)=%d\n\r"),r);
	test(r==KErrNone);

	// Setup serial ports
	test.Next(_L("Setup serial port"));
	TheDceSerialPort->Config(TheConfigDceBuf);
	TheConfigDce.iRate=EBps9600;
	TheConfigDce.iDataBits=EData8;
	TheConfigDce.iStopBits=EStop1;
	TheConfigDce.iParity=EParityNone;
	TheConfigDce.iHandshake=0;
	r=TheDceSerialPort->SetConfig(TheConfigDceBuf);
	test(r==KErrNone);

	TheDteSerialPort->Config(TheConfigDteBuf);
	TheConfigDte.iRate=EBps9600;
	TheConfigDte.iDataBits=EData8;
	TheConfigDte.iStopBits=EStop1;
	TheConfigDte.iParity=EParityNone;
	TheConfigDte.iHandshake=0;
	r=TheDteSerialPort->SetConfig(TheConfigDteBuf);
	test(r==KErrNone);

	test.Next(_L("Get DCE caps"));
	TheDceSerialPort->Caps(TheCapsDceBuf);
	test(r==KErrNone);

#ifndef __WINS__
	test.Next(_L("I/p signals"));
#if defined (_TWO_PORT_LOOPBACK_)
	TheDteSerialPort->SetSignals(0,KChangeSigOut); // Clear
#else
	test.Printf(_L("Negate DTR(D) - hit a key\n\r"));
	test.Getch();
#endif
	TUint sig=TheDceSerialPort->Signals();
	test.Printf(_L("Check (%x) is negated: %x\n\r"),KChangeSigIn,sig);
	test(!(sig&KChangeSigIn));
#if defined (_TWO_PORT_LOOPBACK_)
	TheDteSerialPort->SetSignals(KChangeSigOut,0); // Set
#else
	test.Printf(_L("Assert DTR(D) - hit a key\n\r"));
	test.Getch();
#endif
	sig=TheDceSerialPort->Signals();
	test.Printf(_L("Check (%x) is asserted: %x\n\r"),KChangeSigIn,sig);
	test(sig&KChangeSigIn);

#if defined (_TWO_PORT_LOOPBACK_)
	test.Next(_L("O/p signals"));
	TheDceSerialPort->SetSignals(0,KSignalCTS); // Clear
	sig=TheDteSerialPort->Signals();
	test.Printf(_L("Check (%x) is negated: %x\n\r"),KSignalCTS,sig);
	test(!(sig&KSignalCTS));
	TheDceSerialPort->SetSignals(KSignalCTS,0); // Set
	sig=TheDteSerialPort->Signals();
	test.Printf(_L("Check (%x) is asserted: %x\n\r"),KSignalCTS,sig);
	test(sig&KSignalCTS);
#endif
#endif
	test.Next(_L("Loopback test at 9600"));
#if !defined (_TWO_PORT_LOOPBACK_)
	test.Printf(_L("Start loopback at 9600(L) - hit a key when ready\n\r"));
	test.Getch();
#endif
	test(CheckedWrite(KTestPatternSize)==KTestPatternSize);

	test.Next(_L("Loopback test at 115200"));
	TheConfigDce.iRate=EBps115200;
	r=TheDceSerialPort->SetConfig(TheConfigDceBuf);
	test(r==KErrNone);
#if defined (_TWO_PORT_LOOPBACK_)
	TheConfigDte.iRate=EBps115200;
	r=TheDteSerialPort->SetConfig(TheConfigDteBuf);
	test(r==KErrNone);
#else
	test.Printf(_L("Start loopback at 115200(??BFL) - hit a key when ready\n\r"));
	test.Getch();
#endif
	test(CheckedWrite(KTestPatternSize)==KTestPatternSize);

	test.Next(_L("Test signal change notification"));
	TestingNotifySignalChange(KChangeSigIn);

	test.Next(_L("Test receive data available notification"));
	TestingNotifyReceiveDataAvailable();

#if !defined (__WINS__)
	test.Next(_L("Test flow control change"));
	TestingFlowControlChange();

	test.Next(_L("Test config change notification"));
	TestingNotifyConfigChange();
#endif

	TheDceSerialPort->Close();
	TheDteSerialPort->Close();
	test.Printf(_L("Hit a key"));
	test.Getch();
	test.End();
	return(KErrNone);
	}


