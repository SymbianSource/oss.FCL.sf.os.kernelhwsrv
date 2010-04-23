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
// e32test\mmu\t_chunk3.cpp
// 
//

#include <e32test.h>
#include "u32std.h"
#include "../misc/prbs.h"

RTest test(_L("T_CHUNK3"));

GLDEF_C TInt E32Main()
	{
	TUint seed[2];
	seed[0]=0xddb3d743;
	seed[1]=0;

	test.Title();
	test.Start(_L("Testing chunk resizing"));

	RChunk c1;
	RChunk c2;
	TInt r=c1.CreateLocal(0,0x01000000);	// initial 0 max 16Mb
	test(r==KErrNone);
	r=c2.CreateLocal(0,0x01000000);			// initial 0 max 16Mb
	test(r==KErrNone);

	FOREVER
		{
		r=c1.Adjust(0x400000);				// adjust first chunk to 4Mb
		test(r==KErrNone);
		r=c1.Adjust(0);						// now adjust back to 0
		test(r==KErrNone);
		r=c1.Adjust(0x100000);				// adjust first chunk to 1Mb
		test(r==KErrNone);
		r=c2.Adjust(0x200000);				// now adjust second chunk to 2Mb
		test(r==KErrNone);

		TUint wait=Random(seed);
		wait &= 16383;						// delay in us between 0 and 16383
		User::AfterHighRes(wait);			// wait for a bit

		r=c2.Adjust(0);						// then back to zero
		test(r==KErrNone);
		r=c1.Adjust(0);						// both chunks back to zero
		test(r==KErrNone);
		}

//	test.End();
	}
