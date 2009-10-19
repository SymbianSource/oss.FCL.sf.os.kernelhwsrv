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
// e32test\device\t_ampv.cpp
// Approval tests for the Pc Card Adapter.
// 
//

#include <e32std.h>
#include <e32std_private.h>
#include <e32cons.h>
#include <e32svr.h>
#include <e32hal.h>
#include <e32uid.h>
#include <d32comm.h>

const TInt KRxBufSize=20;
const TInt KAmpVTimeout=2000000;
const TInt KUnit0=0;

#if defined (__WINS__)
#define PDD_NAME _L("ECDRV.PDD")
#define LDD_NAME _L("ECOMM.LDD")
#else
#define PDD_NAME _L("EUART1.PDD")
#define LDD_NAME _L("ECOMM.LDD")
#endif

class RComm : public RBusDevComm
	{
public:
	TInt WriteS(const TDesC8& aDes);
	};


GLDEF_D CConsoleBase *theConsole;
GLDEF_D RComm *theSerialPort;

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

LOCAL_C void AmpVTest()
//
// Perform the test
//
	{

    TInt errCount=0;
    TInt okStatus=0;

	TRequestStatus tStat,sStat,kStat;
	TUint8 rxBuf[KRxBufSize];
	TPtr8 rxDes(&rxBuf[0],KRxBufSize,KRxBufSize);
	theSerialPort->Read(sStat,rxDes,0);	// Raise DTR and wake up the PCA
	User::WaitForRequest(sStat);
	User::After(4000000);				// 4Secs while PC card starts up

	theSerialPort->WriteS(_L8("AT\r"));
	theSerialPort->WriteS(_L8("AT&f\r"));
	User::After(500000);		// 0.5S
	theSerialPort->ResetBuffers();
	theConsole->Printf(_L("Sending AT&V\n\r"));
	theSerialPort->WriteS(_L8("AT&V\r"));

	RTimer tim;
	tim.CreateLocal();
	tim.After(tStat,KAmpVTimeout);
	theConsole->Read(kStat);
	theSerialPort->Read(sStat,rxDes,1);

    FOREVER
        {
		User::WaitForAnyRequest();
		if (sStat!=KRequestPending)
			{
            // got another character
            if (sStat!=KErrNone)
                {
                // Bad character - initiate another try. 
                errCount++;
				tim.Cancel();
				User::WaitForRequest(tStat);
				User::After(200000);	
				theSerialPort->ResetBuffers();
				theConsole->Printf(_L("Errors:%d (Bad char-%d)\r\n"),errCount,sStat.Int());
				theConsole->Printf(_L("Sending AT&V\n\r"));
				theSerialPort->WriteS(_L8("AT&V\r"));
				tim.After(tStat,KAmpVTimeout);
                okStatus=0;
                }
            else
                {
                // Check if its CR/LF/OK/CR/LF
                switch(okStatus)
                    {
                    case 0:
                        okStatus=(rxBuf[0]=='\x0D')?(okStatus+1):0;
                        break;
                    case 1:
                        if (rxBuf[0]=='\x0A')
                            okStatus++;
                        else
                            okStatus=(rxBuf[0]=='\x0D')?1:0;
                        break;
                    case 2:
                        if (rxBuf[0]=='O')
                            okStatus++;
                        else
                            okStatus=(rxBuf[0]=='\x0D')?1:0;
                        break;
                    case 3:
                        if (rxBuf[0]=='K')
                            okStatus++;
                        else
                            okStatus=(rxBuf[0]=='\x0D')?1:0;
                        break;
                    case 4:
                        if (rxBuf[0]=='\x0D')
                            okStatus++;
                        else
                            okStatus=(rxBuf[0]=='\x0D')?1:0;
                        break;
                    case 5:
                        if (rxBuf[0]=='\x0A')
                            {
                            // Success - initiate another try. 
							tim.Cancel();
							User::WaitForRequest(tStat);
							User::After(200000);	
							theSerialPort->ResetBuffers(); 
							theConsole->Printf(_L("Errors:%d\r\n"),errCount);
							theConsole->Printf(_L("Sending AT&V\n\r"));
							theSerialPort->WriteS(_L8("AT&V\r"));
							tim.After(tStat,KAmpVTimeout);
                			okStatus=0;
                            }
                        else
                            okStatus=(rxBuf[0]=='\x0D')?1:0;
                        break;
                    default:
                        okStatus=0;
                    }
                }
            // Queue another serial read
			theSerialPort->Read(sStat,rxDes,1);
			}
		else if (tStat!=KRequestPending)
			{
            // Error - didn't get OK
			theSerialPort->ReadCancel();
			User::WaitForRequest(sStat);
            errCount++;
            // Initiate another try
			User::After(200000);	
			theSerialPort->ResetBuffers();
			theConsole->Printf(_L("Errors:%d (Timeout)\r\n"),errCount);
			theConsole->Printf(_L("Sending AT&V\n\r"));
			theSerialPort->WriteS(_L8("AT&V\r"));
			tim.After(tStat,KAmpVTimeout);
			theSerialPort->Read(sStat,rxDes,1);
            okStatus=0;
			}
        else if (kStat!=KRequestPending)
            {
			theSerialPort->ReadCancel();
			User::WaitForRequest(sStat);
			tim.Cancel();
			User::WaitForRequest(tStat);
            return;
            }
		else
			{
			theConsole->Printf(_L("ERROR - stray signal\r\n"));
			theConsole->ReadCancel();
			User::WaitForRequest(kStat);
			theSerialPort->ReadCancel();
			User::WaitForRequest(sStat);
			tim.Cancel();
			User::WaitForRequest(tStat);
			theConsole->Getch();
            return;
			}
        }
	}

GLDEF_C TInt E32Main()
	{
	TCommConfig cBuf;
	TCommConfigV01 &c=cBuf();

	// Create console
	theConsole=Console::NewL(_L("T_AMPV"),TSize(KDefaultConsWidth,KDefaultConsHeight));

	// Load Device Drivers
	theConsole->Printf(_L("Load PDD\n\r"));
	TInt r;
	r=User::LoadPhysicalDevice(PDD_NAME);
	if (r!=KErrNone&&r!=KErrAlreadyExists)
		goto AmpvEnd;
	theConsole->Printf(_L("Load LDD\n\r"));
	r=User::LoadLogicalDevice(LDD_NAME);
	if (r!=KErrNone&&r!=KErrAlreadyExists)
		goto AmpvEnd;

	// Create RComm object
	theConsole->Printf(_L("Create RComm object\n\r"));
	theSerialPort=new RComm;
	if (theSerialPort==NULL)
		goto AmpvEnd;

	// Open Serial Port
	theConsole->Printf(_L("Open Serial Port\n\r"));
	r=theSerialPort->Open(KUnit0);
	if (r!=KErrNone)
		goto AmpvEnd;

	// Setup serial port
	theConsole->Printf(_L("Setup serial port\n\r"));
	theSerialPort->Config(cBuf);
	c.iRate=EBps57600;
	c.iDataBits=EData8;
	c.iStopBits=EStop1;
	c.iParity=EParityNone;
//	c.iHandshake=KConfigObeyCTS;
	c.iHandshake=0;
	r=theSerialPort->SetConfig(cBuf);
	if (r!=KErrNone)
		goto AmpvEnd;

	AmpVTest();

AmpvEnd:
	if (theSerialPort)
		theSerialPort->Close();
	delete theSerialPort;
	delete theConsole;
	return(KErrNone);
	}


