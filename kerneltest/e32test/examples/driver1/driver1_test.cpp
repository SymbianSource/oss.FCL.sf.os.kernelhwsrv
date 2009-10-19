// Copyright (c) 2003-2009 Nokia Corporation and/or its subsidiary(-ies).
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
//

/**
 @file Test code for example Device Driver
 @publishedPartner
 @released
*/

#include <e32test.h>
#include <e32def.h>
#include <e32def_private.h>
#include "driver1.h"

LOCAL_D RTest test(_L("DRIVER1_TEST"));

_LIT(KDriver1LddFileName,"DRIVER1_LDD");
_LIT(KDriver1PddFileName,"DRIVER1_PDD");

_LIT8(KTestSendData,"abcdefghijklmnopqrstuvwxyz");

GLDEF_C TInt E32Main()
    {
	test.Title();

	TInt r;

	test.Start(_L("Load Physical Device"));
	r=User::LoadPhysicalDevice(KDriver1PddFileName);
	test(r==KErrNone || r==KErrAlreadyExists);

	test.Next(_L("Load Logical Device"));
	r=User::LoadLogicalDevice(KDriver1LddFileName);
	test(r==KErrNone || r==KErrAlreadyExists);

	__KHEAP_MARK;

	test.Next(_L("Open Device"));
	RDevice device;
	r=device.Open(RDriver1::Name());
	test(r==KErrNone);

	test.Next(_L("Get Device Capabilities"));
	RDriver1::TCaps caps;
	TPckg<RDriver1::TCaps>capsPckg(caps);
	capsPckg.FillZ(); // Zero 'caps' so we can tell if GetCaps has really filled it
	device.GetCaps(capsPckg);
	TVersion expectedVer(RDriver1::VersionRequired());
	test(caps.iVersion.iMajor==expectedVer.iMajor);
	test(caps.iVersion.iMinor==expectedVer.iMinor);
	test(caps.iVersion.iBuild==expectedVer.iBuild);

	test.Next(_L("Close Device"));
	device.Close();

	test.Next(_L("Open Logical Channel"));
	RDriver1 ldd;
	r=ldd.Open();
	test(r==KErrNone);

	test.Next(_L("GetConfig"));
	RDriver1::TConfigBuf configBuf;
	configBuf.FillZ();   // Zero 'config' so we can tell if GetConfig has really filled it
	r=ldd.GetConfig(configBuf);
	test(r==KErrNone);

	RDriver1::TConfig& config=configBuf();
	test(config.iPddBufferSize!=0);
	test(config.iMaxSendDataSize!=0);
	test(config.iMaxReceiveDataSize!=0);

	test.Next(_L("SetConfig"));
	TInt speed = configBuf().iSpeed+1;
	configBuf().iSpeed = speed;
	r=ldd.SetConfig(configBuf); // Use SetConfig to change speed
	test(r==KErrNone);

	configBuf.FillZ();
	r=ldd.GetConfig(configBuf);
	test(r==KErrNone);
	test(configBuf().iSpeed==speed);

	test.Next(_L("Check access by wrong client"));
	RDriver1 ldd2=ldd;
	r=ldd2.Duplicate(RThread(),EOwnerProcess);
	test(r==KErrAccessDenied);

	test.Next(_L("Check handle duplication"));
	ldd2=ldd;
	r=ldd2.Duplicate(RThread(),EOwnerThread);
	test(r==KErrNone);
	ldd2.Close();

	test.Next(_L("SendData"));
	TRequestStatus status;
	ldd.SendData(status,KTestSendData);

	test.Next(_L("SendDataCancel"));
	ldd.SendDataCancel();
	User::WaitForRequest(status);
	r=status.Int();
	test(r==KErrCancel);

	test.Next(_L("SendData"));
	ldd.SendData(status,KTestSendData);
	User::WaitForRequest(status);
	r=status.Int();
	test(r==KErrNone);

	test.Next(_L("ReceiveData"));
	TBuf8<256> buffer;
	ldd.ReceiveData(status,buffer);

	test.Next(_L("ReceiveDataCancel"));
	ldd.ReceiveDataCancel();
	User::WaitForRequest(status);
	r=status.Int();
	test(r==KErrCancel);

	test.Next(_L("ReceiveData"));
	buffer.FillZ(buffer.MaxLength());
	buffer.Zero();
	ldd.ReceiveData(status,buffer);
	User::WaitForRequest(status);
	r=status.Int();
	test(r==KErrNone);

	TInt expectedSize = config.iPddBufferSize;
	if(expectedSize>(&KTestSendData)->Size())
		expectedSize = (&KTestSendData)->Size();
	test(buffer.Size()==expectedSize);
	test(buffer==(&KTestSendData)->Right(expectedSize));

	test.Next(_L("Close Logical Channel"));
	ldd.Close();

	__KHEAP_MARKEND;

	test.Next(_L("Unload Logical Device"));
	r=User::FreeLogicalDevice(RDriver1::Name());
	test(r==KErrNone);

	test.Next(_L("Unload Physical Device"));
	TName pddName(RDriver1::Name());
	_LIT(KVariantExtension,".template");
	pddName.Append(KVariantExtension);
	r=User::FreePhysicalDevice(pddName);
	test(r==KErrNone);

	test.End();

	return(0);
    }


