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
// e32test\misc\t_proc3a.cpp
// 
//

#include <e32std.h>
#include <e32std_private.h>
#include "u32std.h"
#include "../misc/prbs.h"

TUint Seed[2];

#define PANIC(r) Panic(__LINE__,r)
void Panic(TInt aLine, TInt aCode)
	{
	_LIT(KLitTPROC3A,"T_PROC3A-");
	TBuf<16> b=KLitTPROC3A();
	b.AppendNum(aLine);
	RProcess().Panic(b,aCode);
	}

void StartThread(TInt);

TInt ThreadFunction(TAny* aParam)
	{
	TInt p=(TInt)aParam;
	FOREVER
		{
		TProcessPriority pp=(p&1)?EPriorityForeground:EPriorityBackground;
		RProcess().SetPriority(pp);
		++p;
		StartThread(p);
		User::AfterHighRes(1);
		}
	}

void StartThread(TInt aParam)
	{
	RThread t;
	TInt r=t.Create(KNullDesC(),ThreadFunction,0x1000,NULL,(TAny*)aParam);
	if (r!=KErrNone)
		PANIC(r);
	t.Resume();
	t.Close();
	}

GLDEF_C TInt E32Main()
	{
	TBuf<256> cmd;
	User::CommandLine(cmd);
	TLex lex(cmd);
	TUint param1;
	TInt r=lex.Val(param1,EHex);
	if (r!=KErrNone)
		PANIC(r);
	Seed[0]=param1;
	Seed[1]=0;
	StartThread(0);
	TUint x=Random(Seed);
	x&=63;
	x+=2;
	User::AfterHighRes(x);
	RProcess().Kill(param1);
	return 0;
	}
