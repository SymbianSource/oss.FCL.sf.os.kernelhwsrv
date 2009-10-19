// Copyright (c) 1996-2009 Nokia Corporation and/or its subsidiary(-ies).
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
// e32test\misc\t_alive.cpp
// 
//

#include <e32test.h>
#include <e32base.h>
#include <e32base_private.h>
#include <e32svr.h>

RTest test(_L("T_ALIVE"));

class CTim : public CActive
	{
public:
	static CTim* New();
	CTim() : CActive(EPriorityStandard) {}
	void RunL();
	void DoCancel() {}
public:
	RTimer iTimer;
	};

class CTest : public CActive
	{
public:
	CTest() : CActive(EPriorityUserInput) {}
	static CTest* New();
	void RunL();
	void DoCancel() {}
	void Start();
	};

LOCAL_C void DisplayProcesses()
	{
	TFindProcess fp(_L("*"));
	TFullName fn;
	while(fp.Next(fn)==KErrNone)
		{
		RDebug::Print(_L("%S"),&fn);
		}
	}


CTim* CTim::New()
	{
	CTim* pC=new CTim;
	if (pC)
		{
		TInt r=pC->iTimer.CreateLocal();
		if (r!=KErrNone)
			{
			delete pC;
			pC=NULL;
			}
		}
	return pC;
	}

void CTim::RunL()
	{
	DisplayProcesses();
	iTimer.HighRes(iStatus,2100000);
	SetActive();
	}

CTest* CTest::New()
	{
	return new CTest;
	}

void CTest::Start()
	{
	test.Console()->Read(iStatus);
	SetActive();
	}

void CTest::RunL()
	{
	TKeyCode k=test.Console()->KeyCode();
	TChar c=(TUint)k;
	TBuf<1> b;
	b.SetLength(1);
	b[0]=(TText)c;
	RDebug::Print(_L("%S"),&b);
	if (c!='0')
		Start();
	else
		CActiveScheduler::Stop();
	}

GLDEF_C TInt E32Main()
	{
	test.Title();
	CActiveScheduler* pS=new CActiveScheduler;
	if (!pS)
		User::Panic(_L("SCHED"),0);
	CActiveScheduler::Install(pS);
	CTim* pT2=CTim::New();
	if (!pT2)
		User::Panic(_L("TIM2"),0);
	CActiveScheduler::Add(pT2);
	CTest* pTest=CTest::New();
	if (!pTest)
		User::Panic(_L("TEST"),0);
	CActiveScheduler::Add(pTest);
	pT2->RunL();
	pTest->Start();
	CActiveScheduler::Start();
	return 0;
	}
