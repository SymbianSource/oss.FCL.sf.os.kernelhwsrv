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
// e32test\realtime\t_frag.cpp
// 
//

#include <e32test.h>
#include "u32std.h"
#include "../misc/prbs.h"

RTest test(_L("T_FRAG"));
const TInt KHeapSize=4096;

LOCAL_C TInt ThreadFunction(TAny* aRandomNumber)
	{
	TUint seed[2];
	seed[0]=(TUint)aRandomNumber;
	seed[1]=0;

	RChunk c;
	TInt r=c.CreateLocal(0x1000,0x01000000);	// initial 4k max 16Mb
	if (r!=KErrNone)
		User::Panic(_L("T_FRAG"),r);
	FOREVER
		{
		TUint wait=Random(seed);
		wait &= 16383;	// delay in us between 0 and 16383
		User::After(wait);
		TUint size=Random(seed);
		size &= 4194303;	// size in bytes between 0 and 4194303
		c.Adjust(size);		// adjust chunk, ignore any errors
		}
	}

GLDEF_C TInt E32Main()
	{
	test.Title();
	test.Start(_L("Testing allocation/deallocation with fragmented memory"));

	TInt r=KErrNone;

	RThread t1;
	TRequestStatus s1;
	r=t1.Create(_L("Thread1"),ThreadFunction,KDefaultStackSize,KHeapSize,KHeapSize,(TAny*)0xb504f334);
	test(r==KErrNone);
	t1.SetPriority(EPriorityLess);
	t1.Logon(s1);
	t1.Resume();

	RThread t2;
	TRequestStatus s2;
	r=t2.Create(_L("Thread2"),ThreadFunction,KDefaultStackSize,KHeapSize,KHeapSize,(TAny*)0xddb3d743);
	test(r==KErrNone);
	t2.SetPriority(EPriorityMuchLess);
	t2.Logon(s2);
	t2.Resume();

	RThread t3;
	TRequestStatus s3;
	r=t3.Create(_L("Thread3"),ThreadFunction,KDefaultStackSize,KHeapSize,KHeapSize,(TAny*)0xc90fdaa2);
	test(r==KErrNone);
	t3.SetPriority(EPriorityMuchLess);
	t3.Logon(s3);
	t3.Resume();

	User::WaitForAnyRequest();

	if (s1!=KRequestPending)
		{
		TExitCategoryName t1ExitCategory =  t1.ExitCategory();
		test.Printf(_L("Thread1 exited %d,%d,%S\n"),t1.ExitType(),t1.ExitReason(),&t1ExitCategory);
		}
	if (s2!=KRequestPending)
		{
		TExitCategoryName t2ExitCategory =  t2.ExitCategory();
		test.Printf(_L("Thread2 exited %d,%d,%S\n"),t2.ExitType(),t2.ExitReason(),&t2ExitCategory);
		}
	if (s3!=KRequestPending)
		{
		TExitCategoryName t3ExitCategory =  t3.ExitCategory();
		test.Printf(_L("Thread3 exited %d,%d,%S\n"),t3.ExitType(),t3.ExitReason(),&t3ExitCategory);
		}
	t1.Kill(0);
	t2.Kill(0);
	t3.Kill(0);

	test.End();
	return 0;
	}
