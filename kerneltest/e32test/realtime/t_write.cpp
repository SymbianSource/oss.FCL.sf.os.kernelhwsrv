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
// e32test\realtime\t_write.cpp
// 
//

#include <e32test.h>
#include "u32std.h"
#include "../misc/prbs.h"

LOCAL_D RTest test(_L("Remote Write"));

class RMySession : public RSessionBase
	{
public:
	TInt Connect(RServer2 aSrv,TRequestStatus& aStat)
		{return CreateSession(aSrv,TVersion(),1,EIpcSession_Unsharable,0,&aStat);}
	void Test(TDes8& aDes)
		{Send(0,TIpcArgs(&aDes));}
	};

void RunTest(RMessage2& aMsg,TPtr8& aDes)
	{
	// never return or complete message, just do lots of writes...

	TUint seed[2];
	seed[0]=0xadf85458;
	seed[1]=0;

	TInt* pS=(TInt*)User::Alloc(65536);
	TInt* pD=(TInt*)User::Alloc(65536);
	TPtrC8 s((TUint8*)pS,65536);
	aDes.Set((TUint8*)pD,0,65536);
	if (!pS || !pD)
		User::Panic(_L("OOM"),0);

	RProcess().SetPriority(EPriorityLow);
	FOREVER
		{
		TInt i;
		for (i=0; i<16384; i++)
			pS[i]=(TInt)Random(seed);
		TInt r=aMsg.Write(0,s);
		if (r!=KErrNone)
			User::Panic(_L("WriteL"),r);
		for (i=0; i<16384; i++)
			{
			if (pD[i]!=pS[i])
				User::Panic(_L("ERROR"),i);
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

	TPtr8 des(0,0);
	sess.Test(des);

	srv.Receive(m);
	RunTest(m, des);

	test.End();
	return 0;
	}

