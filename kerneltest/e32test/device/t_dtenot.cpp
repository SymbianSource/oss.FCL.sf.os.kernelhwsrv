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
// e32test\device\t_dtenot.cpp
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
#define PDD_NAME _L("EUART")
#define LDD_NAME _L("ECOMM")
#endif

#define CHECK(r,v)	{if ((r)!=(v)) {test.Printf(_L("Line %d Expected %08x Got %08x\n"),__LINE__,(v),(r)); test(0);}}

//	Our own comms object with synchronous writes
class RComm : public RBusDevComm
	{
public:
	TInt WriteS(const TDesC8& aDes);
	TInt WriteS(const TDesC8& aDes,TInt aLength);
	};

LOCAL_D RTest test(_L("T_DTENOT"));

RComm* theSerialPort;
TCommCaps2 theCapsBuf;
TCommCapsV02& theCaps=theCapsBuf();

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

enum TSetClearOutSignal {ESetOutSignal,EClearOutSignal};
enum TOutcomeExpected {EExpectNotify,EExpectTimeout};
enum TSigChngNotifyType {ENotifyOnInSigOnly,ENotifyOnAnyChange};
LOCAL_C void TestNotifySignalChange(TUint anOutSig,TSetClearOutSignal aSetClr,TUint anInSig,TOutcomeExpected anExpect,TSigChngNotifyType aType)
//
// Change the state of the specified output signal and wait up to 2 seconds for
// this to trigger a change notification.
//
	{
	RTimer tim;
	tim.CreateLocal();
	TRequestStatus notifStatus;
	TRequestStatus timeStatus;

	TUint signals=0;
	if (aType==ENotifyOnAnyChange)
		theSerialPort->NotifySignalChange(notifStatus,signals);
	else
	    theSerialPort->NotifySignalChange(notifStatus,signals,anInSig);
//	CHECK(notifStatus.Int(),KRequestPending);
	const TUint KTimeOut=2000000;
	tim.After(timeStatus,KTimeOut);
	CHECK(timeStatus.Int(),KRequestPending);
	if (aSetClr==ESetOutSignal)
		theSerialPort->SetSignals(anOutSig,0); // Set Out signal
	else
		theSerialPort->SetSignals(0,anOutSig); // Clear Out signal
	User::WaitForRequest(notifStatus,timeStatus);
	if (notifStatus!=KRequestPending)
		{
		test.Printf(_L("notifStatus=%08x\n"),notifStatus.Int());
		tim.Cancel();
		User::WaitForRequest(timeStatus);
		if (anExpect==EExpectNotify)
			{
			// Got a notification as expected - but was it the correct notification?
			TUint sigmask=(anInSig*KSignalChanged);
			if (aSetClr==ESetOutSignal)
				sigmask|=anInSig;
			if (aType==ENotifyOnAnyChange)
				{CHECK((signals&sigmask),sigmask);}
			else
				{CHECK(signals,sigmask);}
			}
		else
			{
			test.Printf(_L("Spurious notify %d %08x\n"),notifStatus.Int(),signals);
			test(FALSE); // Unexpectedly got notification
			}
		}
	else
		{
		test.Printf(_L("timeStatus=%08x\n"),timeStatus.Int());
		theSerialPort->NotifySignalChangeCancel();
		User::WaitForRequest(notifStatus);
		if (anExpect==EExpectNotify)
			{
			test.Printf(_L("Timed Out!\n\r"));
			test(FALSE);
			}
		else
		    test(timeStatus==KErrNone); // Success
		}
	}

GLDEF_C TInt E32Main()
//
// Test DTE serial driver change notification
//
    {
	test.SetLogged(EFalse); 	// Turn off serial port debugging!

	test.Title();
	test.Start(_L("Check loopback connector"));

	TBuf <0x100> cmd;
	User::CommandLine(cmd);
	TInt port=0;
	if ((cmd.Length()>0) && (cmd[0]>='0' && cmd[0]<='9'))
		port=(TInt)(cmd[0]-'0');

	// Read machine name to determine handshake options required
	TInt mid;
	TInt r=HAL::Get(HAL::EMachineUid,mid);
	test(r==KErrNone);

	TUint handshake=(KConfigFreeRTS|KConfigFreeDTR); // So we can control them ourselves
	if (mid==HAL::EMachineUid_Brutus && port<3)
		handshake=0;	// Brutus can't support these option on ports 0-2

	test.Printf(_L("\r\nThis test requires a loopback conector.\r\n"));
	test.Printf(_L("<<Hit a key to continue>>\r\n"));
	test.Getch();
	
	TInt muid=0;
	test(HAL::Get(HAL::EMachineUid, muid)==KErrNone);
	TBool isAssabet=(muid==HAL::EMachineUid_Assabet);

	// Load Device Drivers
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
			pddName[pddName.Length()-1]=TText('0'+i);
		r=User::LoadPhysicalDevice(pddName);
		if (r==KErrNone || r==KErrAlreadyExists)
			test.Printf(_L("PDD %S loaded\n"),&pddName);
		}

	test.Next(_L("Load LDD"));
	r=User::LoadLogicalDevice(LDD_NAME);
	test.Printf(_L("Load LDD Return %d\n\r"),r);

	test.Next(_L("Create RComm object"));
	theSerialPort=new RComm;
	test(theSerialPort!=NULL);
//
	test.Next(_L("Open:"));
	r=theSerialPort->Open(port);
	test.Printf(_L("Open(%d)=%d\n\r"),port,r);
	test(r==KErrNone);

	test.Next(_L("Get caps and configure port"));
	theSerialPort->Caps(theCapsBuf);
	CHECK(r,KErrNone);
	test.Printf(_L("Signals(DTR-RTS-RI-DCD-DSR-CTS) %x\n\r"),theCaps.iSignals);
	test.Printf(_L("Notifications %x\n\r"),theCaps.iNotificationCaps);

	TCommConfig cBuf;
	TCommConfigV01& c=cBuf();
	theSerialPort->Config(cBuf);
	c.iHandshake=handshake;
	c.iDataBits=EData8;
	c.iStopBits=EStop1;
	c.iParity=EParityNone;
	c.iRate=EBps9600;
	r=theSerialPort->SetConfig(cBuf);
	CHECK(r,KErrNone);

	RTimer tim;
	tim.CreateLocal();
	TRequestStatus notifStatus;
	TRequestStatus timeStatus;

	test.Next(_L("Testing NotifySignalChange()"));
	if (!(theCaps.iNotificationCaps & KNotifySignalsChangeSupported))
        test.Printf(_L("WARNING - Signal change notification not supported on this platform\r\n"));
    else
        {
		if ((theCaps.iSignals&(KCapsSignalCTSSupported|KCapsSignalRTSSupported))!=(KCapsSignalCTSSupported|KCapsSignalRTSSupported))
        	test.Printf(_L("WARNING - RTS/CTS not supported on this platform\r\n"));
		else
			{
#ifndef __WINS__
			TUint signals=0xffffffff;
	    	theSerialPort->SetSignals(0,KSignalRTS); // Clear RTS
	    	User::After(100000);
			test.Next(_L("Initial notifier"));
			theSerialPort->NotifySignalChange(notifStatus,signals);
			User::WaitForRequest(notifStatus);
			test(notifStatus==KErrNone);	// goes off immediately the first time
			test.Printf(_L("Signals %08x\n"),signals);
			test((signals&(KSignalRTS|KSignalCTS))==0);
#endif
			// Test with no signal mask set

			isAssabet=0;
			if (isAssabet)
				{
//			test.Next(_L("   CTS(set) notify without mask set"));
//			TestNotifySignalChange(KSignalRTS,ESetOutSignal,KSignalCTS,EExpectNotify,ENotifyOnAnyChange);
//	(CF)	Note: This test presents some problems: we specify notification on any signal but the test only   
//				  passes if the input signal specified (CTS) has changed. The test code makes a request for
//				  notification on any signal (iSignalMask=0x3f) and THEN changes an output signal (RTS) that's 
//				  wired to the input signal specified (CTS). But changing an output signal launches the DFC
//				  to complete the notification request reporting a change on the output signal NOT the input
//				  signal. The only reason most platforms pass this test is because, on them, input signal changes 
//				  trigger interrupts: due to the loopback between output and input signals, when the output
//				  signal changes so does the input signal and that triggers an interrupt which also launches 
//				  a DFC to complete the notification request reporting a change on the input signal.
//				  The interrupt is serviced before the output signal notification DFC is scheduled and
//				  the notification request is completed with the input signal change.
//				  On Assabet Modem control signals are polled instead of generating interrupts (Intel's design
//				  flaw) and therefore the output signal change will complete the notification (in error) before
//				  the input signal change is notified.
//
				test.Next(_L("   RTS(set) notify without mask set"));
				TestNotifySignalChange(KSignalRTS,ESetOutSignal,KSignalRTS,EExpectNotify,ENotifyOnAnyChange);
				test.Next(_L("   RTS(clear) notify without mask set"));
				TestNotifySignalChange(KSignalRTS,EClearOutSignal,KSignalRTS,EExpectNotify,ENotifyOnAnyChange);
				}
			else
				{
				test.Next(_L("   CTS(set) notify without mask set"));
				TestNotifySignalChange(KSignalRTS,ESetOutSignal,KSignalCTS,EExpectNotify,ENotifyOnAnyChange);
				test.Next(_L("   CTS(clear) notify without mask set"));
				TestNotifySignalChange(KSignalRTS,EClearOutSignal,KSignalCTS,EExpectNotify,ENotifyOnAnyChange);
				}

        	// Test notification doesn't happen with signal mask set to some other signal
			test.Next(_L("   No CTS(set) notify with mask set to other signal"));
			TestNotifySignalChange(KSignalRTS,ESetOutSignal,KSignalDSR,EExpectTimeout,ENotifyOnInSigOnly);
			TestNotifySignalChange(KSignalRTS,ESetOutSignal,KSignalCTS,EExpectNotify,ENotifyOnInSigOnly);

        	// Test notification happens with mask set to this signal
			test.Next(_L("   CTS(clear) notify with mask set"));
			TestNotifySignalChange(KSignalRTS,EClearOutSignal,KSignalCTS,EExpectNotify,ENotifyOnInSigOnly);
			test.Next(_L("   CTS(set) notify with mask set"));
			TestNotifySignalChange(KSignalRTS,ESetOutSignal,KSignalCTS,EExpectNotify,ENotifyOnInSigOnly);
			}

		if ((theCaps.iSignals&(KCapsSignalDSRSupported|KCapsSignalDTRSupported))!=(KCapsSignalDSRSupported|KCapsSignalDTRSupported))
        	test.Printf(_L("WARNING - DTR/DSR not supported on this platform\r\n"));
		else
			{
	    	theSerialPort->SetSignals(0,KSignalDTR); // Clear DTR
	    	User::After(100000);

        	// Test with no signal mask set

			if (isAssabet)
				{
// (CF) See note above
				test.Next(_L("   DTR(set) notify without mask set"));
				TestNotifySignalChange(KSignalDTR,ESetOutSignal,KSignalDTR,EExpectNotify,ENotifyOnAnyChange);
				test.Next(_L("   DTR(clear) notify without mask set"));
				TestNotifySignalChange(KSignalDTR,EClearOutSignal,KSignalDTR,EExpectNotify,ENotifyOnAnyChange);
				}
			else
				{
				test.Next(_L("   DSR(set) notify without mask set"));
				TestNotifySignalChange(KSignalDTR,ESetOutSignal,KSignalDSR,EExpectNotify,ENotifyOnAnyChange);
				test.Next(_L("   DSR(clear) notify without mask set"));
				TestNotifySignalChange(KSignalDTR,EClearOutSignal,KSignalDSR,EExpectNotify,ENotifyOnAnyChange);
				}

        	// Test notification happens with mask set to this signal
			test.Next(_L("   DSR(set) notify with mask set"));
			TestNotifySignalChange(KSignalDTR,ESetOutSignal,KSignalDSR,EExpectNotify,ENotifyOnInSigOnly);
			test.Next(_L("   DSR(clear) notify with mask set"));
			TestNotifySignalChange(KSignalDTR,EClearOutSignal,KSignalDSR,EExpectNotify,ENotifyOnInSigOnly);
			}

		if (mid==HAL::EMachineUid_Series5mx ||
			(theCaps.iSignals&(KCapsSignalDCDSupported|KCapsSignalDTRSupported))!=(KCapsSignalDCDSupported|KCapsSignalDTRSupported))
        	test.Printf(_L("WARNING - DTR/DCD not supported on this platform\r\n"));
		else
			{
        	// Test with no signal mask set

			if (isAssabet)
				{
// (CF) See note above
				test.Next(_L("   DTR(set) notify without mask set"));
				TestNotifySignalChange(KSignalDTR,ESetOutSignal,KSignalDTR,EExpectNotify,ENotifyOnAnyChange);			
				test.Next(_L("   DTR(clear) notify without mask set"));
				TestNotifySignalChange(KSignalDTR,EClearOutSignal,KSignalDTR,EExpectNotify,ENotifyOnAnyChange);
				}
			else
				{
				test.Next(_L("   DCD(set) notify without mask set"));
				TestNotifySignalChange(KSignalDTR,ESetOutSignal,KSignalDCD,EExpectNotify,ENotifyOnAnyChange);
				test.Next(_L("   DCD(clear) notify without mask set"));
				TestNotifySignalChange(KSignalDTR,EClearOutSignal,KSignalDCD,EExpectNotify,ENotifyOnAnyChange);
				}


        	// Test notification happens with mask set to this signal
			test.Next(_L("   DCD(set) notify with mask set"));
			TestNotifySignalChange(KSignalDTR,ESetOutSignal,KSignalDCD,EExpectNotify,ENotifyOnInSigOnly);
			test.Next(_L("   DCD(clear) notify with mask set"));
			TestNotifySignalChange(KSignalDTR,EClearOutSignal,KSignalDCD,EExpectNotify,ENotifyOnInSigOnly);
			}
        }

	test.Next(_L("Testing NotifyReceiveDataAvailable()"));
	if (!(theCaps.iNotificationCaps & KNotifyDataAvailableSupported))
        test.Printf(_L("Data available notification not supported on this platform\r\n"));
    else
        {
	    TBuf8<0x10> buf1(0x10), buf2(0x10);
        for (TInt i=0;i<0x10;i++) buf1[i]=(TUint8)i;

        theSerialPort->ResetBuffers();
	    theSerialPort->NotifyReceiveDataAvailable(notifStatus);
	    CHECK(notifStatus.Int(),KRequestPending);
	    const TUint KTimeOut=2000000;
	    tim.After(timeStatus,KTimeOut);
	    CHECK(timeStatus.Int(),KRequestPending);
	    theSerialPort->WriteS(buf1);
	    User::WaitForRequest(notifStatus,timeStatus);
	    if (notifStatus==KErrNone)
		    {
		    tim.Cancel();
		    CHECK(notifStatus.Int(),KErrNone);
		    }
	    else
		    {
		    test.Printf(_L("Timed Out!\n\r"));
	        theSerialPort->NotifyReceiveDataAvailableCancel();
		    test(FALSE);
		    }
		User::After(500000);
		CHECK(theSerialPort->QueryReceiveBuffer(),0x10);
	    buf2.FillZ();
	    theSerialPort->Read(notifStatus,buf2,0x10);
		User::WaitForRequest(notifStatus);
		test(buf1.Compare(buf2)==0);
        }

	theSerialPort->Close();
	test.Printf(_L("<<Hit a key to end>>\r\n"));
	test.Getch();
	test.End();
	return(KErrNone);
	}


