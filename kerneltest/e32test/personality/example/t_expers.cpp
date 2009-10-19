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
// e32test\personality\example\t_expers.cpp
// 
//

#include <e32base.h>
#include <e32base_private.h>
#include <e32test.h>
#include "ifcldd.h"
#include "../../misc/prbs.h"

RTest test(_L("EXAMPLE PERSONALITY"));

_LIT(KIfcLddName,"expers.ldd");

class CRxData : public CActive
	{
public:
	CRxData();
	static void New(RRtosIfc aIfc);
	void Start();
	void RunL();
	void DoCancel();
public:
	RRtosIfc iIfc;
	TUint8 iBuf[512];
	};

void CRxData::New(RRtosIfc aIfc)
	{
	CRxData* p = new CRxData;
	test(p!=NULL);
	p->iIfc = aIfc;
	CActiveScheduler::Add(p);
	p->Start();
	}

CRxData::CRxData()
	:	CActive(0)
	{
	}

void CRxData::DoCancel()
	{
	iIfc.Cancel(RRtosIfc::ECancelReceive);
	}

void CRxData::Start()
	{
	iIfc.Receive(iStatus, *(SRxData*)iBuf);
	SetActive();
	}

void CRxData::RunL()
	{
	SRxData& rxd = *(SRxData*)iBuf;
	TInt l = rxd.iLength;
	TUint cs = 0;
	TInt i;
	for (i=0; i<l; ++i)
		cs += rxd.iData[i];
	test( (cs & 0xff) == rxd.iChecksum);
	test.Printf(_L("RXD: l=%02x cs=%02x\n"), l, rxd.iChecksum);
	Start();
	}

class CReport : public CActive
	{
public:
	CReport();
	static void New(RRtosIfc aIfc);
	void Start();
	void RunL();
	void DoCancel();
	void SendData();
public:
	RRtosIfc iIfc;
	TUint iSeed[2];
	SReport iRpt;
	};

void CReport::New(RRtosIfc aIfc)
	{
	CReport* p = new CReport;
	test(p!=NULL);
	p->iIfc = aIfc;
	CActiveScheduler::Add(p);
	p->Start();
	}

CReport::CReport()
	:	CActive(0)
	{
	iSeed[0] = 0xb504f333;
	}

void CReport::DoCancel()
	{
	iIfc.Cancel(RRtosIfc::ECancelReport);
	}

void CReport::Start()
	{
	iIfc.Report(iStatus, iRpt);
	SetActive();
	}

void CReport::RunL()
	{
	switch (iRpt.iType)
		{
		case SReport::ESem:
			test.Printf(_L("SEM: C:%10d OK:%10d BAD:%10d\n"), iRpt.iCount, iRpt.iOkCount, iRpt.iBadCount);
			if (iRpt.iOkCount>1000000)
				CActiveScheduler::Stop();
			break;
		case SReport::ERcv:
			test.Printf(_L("RCV: C:%10d OK:%10d BAD:%10d\n"), iRpt.iCount, iRpt.iOkCount, iRpt.iBadCount);
			break;
		case SReport::ETm:
			test.Printf(_L("TM: C:%10d\n"), iRpt.iCount);
#ifndef __EPOC32__
			if (iRpt.iCount > 100000)
				CActiveScheduler::Stop();
#endif
			TUint x = Random(iSeed) & 3;
			if (x==3)
				SendData();
			if (x)
				SendData();
			else
				iIfc.FlushData();
			break;
		}
	Start();
	}

void CReport::SendData()
	{
	TInt l = (Random(iSeed)&127)+128;
	TUint buf[64];
	TInt i;
	for (i=0; i<64; ++i)
		buf[i] = Random(iSeed);
	TPtrC8 ptr((const TUint8*)buf, l);
	TInt r = iIfc.SendData(ptr);
	test(r==KErrNone);
	}


GLDEF_C TInt E32Main()
	{
	test.Title();
	test.Start(_L("Load interface LDD"));

	TInt r = User::LoadLogicalDevice(KIfcLddName);
	test(r==KErrNone || r==KErrAlreadyExists);

	test.Next(_L("Open channel"));
	RRtosIfc ifc;
	RThread().SetPriority(EPriorityAbsoluteHigh);
	r = ifc.Open();
	test(r==KErrNone);

	test.Next(_L("Start personality tests"));
	ifc.Init();

	TRequestStatus s;
	ifc.WaitInitialTests(s);
	User::WaitForRequest(s);
	test(s==KErrNone);

	CActiveScheduler* as = new CActiveScheduler;
	test(as!=NULL);
	CActiveScheduler::Install(as);
	CRxData::New(ifc);
	CReport::New(ifc);

	CActiveScheduler::Start();

	ifc.Finish();
	ifc.Close();

	test.End();
	return 0;
	}

