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
// e32test\misc\t_abt.cpp
// 
//

#include <e32test.h>

LOCAL_D RTest test(_L("ABT"));
LOCAL_D TInt junk=0x11;

class RMySession : public RSessionBase
	{
public:
	TInt Connect(RServer2 aSrv,TRequestStatus& aStat)
		{return CreateSession(aSrv,TVersion(),1,EIpcSession_Unsharable,0,&aStat);}
	void Test(TDesC8& aDes)
		{Send(0,TIpcArgs(&aDes));}
	};

void RunTest(RMessage2& aMsg,TPtrC8& aDes)
	{
	RProcess().SetPriority(EPriorityLow);
	TUint8* pD=(TUint8*)User::Alloc(2048);
	TPtr8 d(pD,2048);
	TInt* pJ=&junk;
	*pJ++=0x2233;
	TUint32 data_addr=(TUint32)pJ;
	data_addr=(data_addr+4095)&~4095;
	data_addr-=2044;
	const TUint8* p=(const TUint8*)data_addr;
	aDes.Set(p,2048);
	FOREVER
		{
		TInt r=aMsg.Read(0,d);
		if (r!=KErrBadDescriptor)
			{
			test.Printf(_L("Return code %d\n"),r);
			test(0);
			}
		}
	}

GLDEF_C TInt E32Main()
	{
	test.Title();

	RServer2 srv;
	srv.CreateGlobal(KNullDesC);

	RMySession sess;
	TRequestStatus stat;
	sess.Connect(srv,stat);

	RMessage2 m;
	srv.Receive(m);
	m.Complete(KErrNone);	// connect message

	User::WaitForRequest(stat);	// connected

	TPtrC8 des;
	sess.Test(des);

	srv.Receive(m);
	RunTest(m, des);

	test.End();
	return 0;
	}

