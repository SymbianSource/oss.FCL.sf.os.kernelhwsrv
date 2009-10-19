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
// e32test\realtime\t_latncy.cpp
// 
//

#include <e32test.h>
#include <e32svr.h>

LOCAL_D RTest test(_L("Latency"));
LOCAL_D CConsoleBase* console=NULL;

struct SLatencyValues
	{
	TUint iAddress;
	TUint iLatencies;
	TUint iThreadIds;
	TUint iSwi;
	};

struct SMaxLatencyValues
	{
	TInt iMaxIntLatency;
	TUint iMaxIntLatencyRetAddr;
	TInt iMaxThreadLatency;
	TUint iMaxThreadLatencyRetAddr;
	};

extern void GetLatencyValues(TInt, TInt&, SLatencyValues*);

inline TInt* NullTIntPtr()	// to allow generation of a NULL reference without the compiler complaining
	{ return 0; }

void GetMaxLatencyValues(SMaxLatencyValues& a)
	{
	GetLatencyValues(2,*NullTIntPtr(),(SLatencyValues*)&a);
	}

LOCAL_C void DumpValues(TInt aCount, SLatencyValues* aValues)
	{
	TInt i;
	for (i=0; i<aCount; i++)
		{
		SLatencyValues v=aValues[i];
		TInt addr=v.iAddress;
		TInt iticks=v.iLatencies>>16;
		TInt tticks=v.iLatencies & 0xffff;
		TInt ius=(iticks*125+124)>>6;
		TInt tus=(tticks*125+124)>>6;
//		TInt oldtid=v.iThreadIds & 0xffff;
//		TInt newtid=v.iThreadIds >> 16;
		TInt swi=v.iSwi;
		RDebug::Print(_L("%08x %4d %4d %08x %08x"),addr,ius,tus,v.iThreadIds,swi);
		}
	}

LOCAL_C void DisplayMaxLatencyValues()
	{
	SMaxLatencyValues v;
	GetMaxLatencyValues(v);
	TInt ius=(v.iMaxIntLatency*125+124)>>6;
	TInt tus=(v.iMaxThreadLatency*125+124)>>6;
	console->Printf(_L("Interrupt: %4dus return address %08x\n"),ius,v.iMaxIntLatencyRetAddr);
	console->Printf(_L("Thread:    %4dus return address %08x\n"),tus,v.iMaxThreadLatencyRetAddr);
	}

GLDEF_C TInt E32Main()
	{
	TInt c;
	SLatencyValues* pV=new SLatencyValues[28672];

	test.Title();
	console=test.Console();
	if (!pV)
		User::Panic(_L("OOM"),0);

	TBool exit=EFalse;
	while(!exit)
		{
		TKeyCode k=test.Getch();
		switch(k)
			{
			case '0':
				console->Printf(_L("Getting Latency Values\n"));
				GetLatencyValues(0,c,pV);
				DumpValues(c,pV);
				break;
			case '1':
				console->Printf(_L("Clearing Latency Values\n"));
				GetLatencyValues(1,c,pV);
				break;
			case '2':
				DisplayMaxLatencyValues();
				break;
			case 'x':
			case 'X':
				exit=ETrue;
				break;
			default:
				break;
			}
		}

	test.End();
	return 0;
	}

