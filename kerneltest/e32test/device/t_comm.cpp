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
// e32test\device\t_comm.cpp
// 
//

#include <e32base.h>
#include <e32base_private.h>
#include <e32test.h>
#include <e32svr.h>
#include <d32comm.h>
#include <e32uid.h>

LOCAL_D RTest test(_L("T_COMM"));

const TInt KUnit0=0;
const char* KLongString="1 3456789012345678\n\r2 3456789012345678\n\r3 3456789012345678\n\r4 3456789012345678\n\r5 3456789012345678\n\r6 3456789012345678\n\r7 3456789012345678\n\r8 3456789012345678\n\r9 3456789012345678\n\r0 3456789012345678\n\r";

class RComm : public RBusDevComm
	{
public:
	TInt WriteS(const TDesC8& aDes);
	TInt WriteS(const TDesC8& aDes,TInt aLength);
	};


LOCAL_C void testRDevice(const RBusDevComm& aDev,const TFindLogicalDevice& aFind)
//
// Test the RDevice class
//
	{

	test.Start(_L("Testing RDevice with find handle"));
	RDevice d;
	TInt r=d.Open(aFind);
	test(r==KErrNone);
	d.Close();
//
	test.Next(_L("Testing RDevice open by name"));
	r=d.Open(_L("Comm"));
	test(r==KErrNone);
//
	test.Next(_L("Query version supported"));
	TBool b=d.QueryVersionSupported(aDev.VersionRequired());
	test(b);
//
	test.Next(_L("Query is available"));
	b=d.IsAvailable(KDefaultUnit,NULL,NULL);
	test(b);
//
	test.Next(_L("Query device capabilities"));
	TPckgBuf<TCapsDevCommV01> c;
	d.GetCaps(c);
 	TVersionName aName = c().version.Name();
 	test.Printf(_L("  Version = %S\n"),&aName);
//
	d.Close();
	test.End();
	}

TInt RComm::WriteS(const TDesC8& aDes)
//
// Synchronous write
//
	{

	return(WriteS(aDes,aDes.Length()));
	}

TInt RComm::WriteS(const TDesC8& aDes,TInt aLength)
//
// Synchronous write
//
	{

	TRequestStatus s;
	Write(s,aDes,aLength);
	User::WaitForRequest(s);
	return(s.Int());
	}

LOCAL_C void testComm()
//
// Test the comms device driver
//
	{

	test.Start(_L("Testing comm device"));
//
	RComm com;
	TInt r=com.Open(KUnit0);
	test(r==KErrNone);
//
	test.Next(_L("Set config to 9600,8,N,1"));
	TCommConfig cBuf;
	TCommConfigV01& c=cBuf();
	c.iRate=EBps9600;
	c.iDataBits=EData8;
	c.iStopBits=EStop1;
	c.iParity=EParityNone;
	c.iHandshake=KConfigObeyXoff|KConfigSendXoff;
	c.iParityError=KConfigParityErrorFail;
	c.iTerminatorCount=0;
	c.iXonChar=0x11; // XON
	c.iXonChar=0x13; // XOFF
	c.iSpecialRate=0;
	c.iParityErrorChar=0;
	r=com.SetConfig(cBuf);
	test(r==KErrNone);
//
	test.Next(_L("Write hello world"));
	com.WriteS(_L8("\r\nHello world\r\n"));
	test.Next(_L("Write long string"));
	com.WriteS(_L8(KLongString));
//
	test.Next(_L("Set config to 4800,7,N,1"));
	c.iRate=EBps4800;
	c.iDataBits=EData7;
	c.iStopBits=EStop1;
	c.iParity=EParityNone;
	c.iHandshake=KConfigObeyXoff|KConfigSendXoff;
	c.iParityError=KConfigParityErrorFail;
	c.iTerminatorCount=0;
	c.iXonChar=0x11; // XON
	c.iXonChar=0x13; // XOFF
	c.iSpecialRate=0;
	c.iParityErrorChar=0;
	r=com.SetConfig(cBuf);
	test(r==KErrNone);

	test.Next(_L("Close comm device"));
	com.Close();
//
	test.End();
	}

GLDEF_C TInt E32Main()
//
// Test the various kernel types.
//
    {

	test.Title();
//
	test.Start(_L("Load PDD"));
	TInt r=User::LoadPhysicalDevice(_L("ECDRV"));
	test(r==KErrNone);
//
	test.Next(_L("Load LDD"));
	r=User::LoadLogicalDevice(_L("ECOMM"));
	test(r==KErrNone);
//
	test.Next(_L("Find libs"));
	TFindLibrary fL;
	TFullName n;
	while (fL.Next(n)==KErrNone)
		test.Printf(_L("  %S\n"),&n);
//
	test.Next(_L("Find devices"));
	TFindLogicalDevice fD;
	while (fD.Next(n)==KErrNone)
		test.Printf(_L("  %S\n"),&n);
//
	RBusDevComm com;
	fD.Find(_L("Comm"));
	r=fD.Next(n);
	test(r==KErrNone);
	testRDevice(com,fD);
//
	test.Next(_L("Open device"));
	r=com.Open(KUnit0);
	test(r==KErrNone);
//
//
	test.Next(_L("Free device/In use"));
	r=User::FreeLogicalDevice(_L("Comm"));
	test(r==KErrInUse);
//
	test.Next(_L("Close device"));
	com.Close();
//
//
	test.Next(_L("Testing comms device"));
	testComm();
//
	TFindPhysicalDevice fDr;
 	test.Next(_L("Free driver/Ok"));
	TFullName drivName;
	fDr.Find(_L("Comm.*"));
	r=fDr.Next(drivName);
	test(r==KErrNone);
	r=User::FreePhysicalDevice(drivName);
	test(r==KErrNone);
//
	test.Next(_L("Free device/Ok"));
	r=User::FreeLogicalDevice(_L("Comm"));
	test(r==KErrNone);
//
	test.Next(_L("Find libs none"));
	fL.Find(_L("*.*"));
	while (fL.Next(n)==KErrNone)
		test.Printf(_L("  %S\n"),&n);
//
	test.Next(_L("Find devices none"));
	fD.Find(_L("*.*"));
	while (fD.Next(n)==KErrNone)
		test.Printf(_L("  %S\n"),&n);
//
	test.Next(_L("Find drivers none"));
	while (fDr.Next(n)==KErrNone)
		test.Printf(_L("  %S\n"),&n);

	test.End();

	return(0);
    }


