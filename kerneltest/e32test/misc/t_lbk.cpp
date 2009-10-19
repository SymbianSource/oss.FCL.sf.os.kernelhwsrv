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
// e32test\misc\t_lbk.cpp
// 
//

#include <e32test.h>
#include <d32comm.h>

RTest test(_L("LoopBack"));

#define TEST(c)	((void)((c)||(test.Printf(_L("Failed at line %d\n"),__LINE__),test.Getch(),test(0),0)))

const TInt KBufferSize=4096;

_LIT(KLddName,"ECOMM");
_LIT(KPddName,"EUART");

TUint8 Buffer[2*KBufferSize];

void LoadCommDrivers()
	{
	test.Printf(_L("Load LDD\n"));
	TInt r=User::LoadLogicalDevice(KLddName);
	TEST(r==KErrNone || r==KErrAlreadyExists);

	TInt i;
	TInt n=0;
	for (i=-1; i<10; ++i)
		{
		TBuf<16> pddName=KPddName();
		if (i>=0)
			pddName.Append('0'+i);
		TInt r=User::LoadPhysicalDevice(pddName);
		if (r==KErrNone || r==KErrAlreadyExists)
			{
			++n;
			test.Printf(_L("%S found\n"),&pddName);
			}
		}
	TEST(n!=0);
	}

GLDEF_C TInt E32Main()
	{
	RThread().SetPriority(EPriorityAbsoluteForeground);
	test.SetLogged(EFalse);
	test.Title();

	TBuf<256> cmd;
	User::CommandLine(cmd);
	TInt port=0;
	if (cmd.Length()!=0)
		{
		TUint8 c=(TUint8)cmd[0];
		if (c>='0' && c<='9')
			{
			port=c-'0';
			}
		}

	TInt r=KErrNone;
	LoadCommDrivers();

	RBusDevComm comm;
	r=comm.Open(port);
	test(r==KErrNone);
	TCommConfig cfg;
	TCommConfigV01& c=cfg();
	comm.Config(cfg);
	c.iRate=EBps115200;
	c.iDataBits=EData8;
	c.iStopBits=EStop1;
	c.iParity=EParityNone;
	c.iHandshake=0;
	c.iFifo=EFifoEnable;
	c.iTerminatorCount=0;
	r=comm.SetConfig(cfg);
	test(r==KErrNone);

	TRequestStatus rxs, txs;
	TRequestStatus* pS=&txs;
	User::RequestComplete(pS,0);

	TInt pos=0;
	FOREVER
		{
		TPtr8 dptr(Buffer+pos,0,KBufferSize);
		comm.ReadOneOrMore(rxs,dptr);
		User::WaitForRequest(rxs);
		if (rxs!=KErrNone)
			test.Printf(_L("RX error %d\n"),rxs.Int());
		User::WaitForRequest(txs);
		if (txs!=KErrNone)
			test.Printf(_L("TX error %d\n"),txs.Int());
		comm.Write(txs,dptr);
		if (pos)
			pos=0;
		else
			pos=KBufferSize;
		}
	}
