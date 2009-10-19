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
// e32test\misc\ymodemu.cpp
// 
//

#include <e32std.h>
#include <e32std_private.h>
#include <d32comm.h>
#include "ymodemu.h"

//#include <e32test.h>
//GLREF_C RTest test;

YModemU::YModemU(TBool aG)
	: YModem(aG)
	{
	}

YModemU::~YModemU()
	{
	iComm.Close();
	iTimer.Close();
	}

TInt YModemU::Create(TInt aPort)
	{
	TInt r=iComm.Open(aPort);
	if (r!=KErrNone)
		return r;
	TCommConfig cfg;
	TCommConfigV01& c=cfg();
	iComm.Config(cfg);
	c.iRate=EBps115200;
	c.iDataBits=EData8;
	c.iStopBits=EStop1;
	c.iParity=EParityNone;
//	c.iHandshake=KConfigFreeRTS|KConfigFreeDTR;
	c.iHandshake=0;
	c.iFifo=EFifoEnable;
	c.iTerminatorCount=0;
	r=iComm.SetConfig(cfg);
	if (r!=KErrNone)
		return r;
	r=iComm.SetReceiveBufferLength(8192);
	if (r!=KErrNone)
		return r;
	return iTimer.CreateLocal();
	}

YModemU* YModemU::NewL(TInt aPort, TBool aG)
	{
	YModemU* p=new YModemU(aG);
	TInt r=p->Create(aPort);
	if (r!=KErrNone)
		{
		delete p;
		User::Leave(r);
		}
	return p;
	}

void YModemU::WriteC(TUint aChar)
	{
	TBuf8<1> b;
	b.SetLength(1);
	b[0]=(TUint8)aChar;
	TRequestStatus s;
	iComm.Write(s,b);
	User::WaitForRequest(s);
	}

TInt YModemU::ReadBlock(TDes8& aDest)
	{
	aDest.Zero();
	TRequestStatus s1, s2;
	iTimer.After(s1,iTimeout);
//	test.Printf(_L("@%dms %d\n"),User::NTickCount(),iComm.QueryReceiveBuffer());
	iComm.Read(s2,aDest);
	User::WaitForRequest(s1,s2);
	if (s2!=KRequestPending)
		{
		iTimer.Cancel();
		User::WaitForRequest(s1);
		return s2.Int();
		}
	iComm.ReadCancel();
	User::WaitForRequest(s2);
	return KErrTimedOut;
	}

