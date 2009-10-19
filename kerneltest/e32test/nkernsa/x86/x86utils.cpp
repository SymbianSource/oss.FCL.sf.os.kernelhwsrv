// Copyright (c) 2006-2009 Nokia Corporation and/or its subsidiary(-ies).
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
// e32test\nkernsa\x86\x86utils.cpp
// 
//

#include <x86.h>
#include <nktest/nkutils.h>

extern "C" {

void __fastcall thread_request_signal(NThread* aThread);

TUint64 fcf;
TUint32 nfcf;
TUint32 nfcfs;


TUint32 round_to_page(TUint32 x)
	{
	return (x + KPageSize - 1) &~ (KPageSize - 1);
	}


TLinAddr __initial_stack_base()
	{
	return __stack_pointer() & ~0xfff;
	}

TInt __initial_stack_size()
	{
	return 0x1000;
	}

TUint64 fast_counter_freq()
	{
	return fcf;
	}

TUint32 norm_fast_counter_freq()
	{
	return nfcf;
	}

void init_fast_counter()
	{
	NKern::Sleep(2);
	TUint64 initial = fast_counter();
	NKern::Sleep(1000);
	TUint64 final = fast_counter();
	fcf = final - initial;
	TUint64 f = fcf;
	nfcfs = 0;
	while (f > 2000000)
		f>>=1, ++nfcfs;
	nfcf = (TUint32)(fcf >> nfcfs);

	DEBUGPRINT("fcf=%lx",fcf);
	DEBUGPRINT("nfcf=%d",nfcf);
	}

TInt __microseconds_to_fast_counter(TInt us)
	{
	TUint64 x = TUint64(TUint32(us));
	x *= fcf;
	x += TUint64(500000);
	x /= TUint64(1000000);

	return (TInt)x;
	}

TInt __microseconds_to_norm_fast_counter(TInt us)
	{
	TUint64 x = TUint64(TUint32(us));
	x *= nfcf;
	x += TUint64(500000);
	x /= TUint64(1000000);

	return (TInt)x;
	}

extern TUint32 __eflags();
void CheckPoint()
	{
	KPrintf("EFLAGS=%08x", __eflags());
	}

void __fastcall thread_request_signal(NThread* aThread)
	{
	NKern::ThreadRequestSignal(aThread);
	}
}
