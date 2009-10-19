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
// e32test\misc\t_cmpres.cpp
// 
//

#include <e32test.h>

/**
Overview:
	Test heap compression with all offsets.

API Information:
	RHeap.

Details:
	- Allocate some cells with specified sizes from the current thread's 
	  heap, free a few of them, compress all the chunks containing heaps 
	  and check the validity of the current thread's heap.

Platforms/Drives/Compatibility:
	All 

Assumptions/Requirement/Pre-requisites:
	
Failures and causes:
	
Base Port information:
*/

LOCAL_D RTest test(_L("T_CMPRES"));

GLDEF_C TInt E32Main()
	{

	test.Title();
	test.Start(_L("Testing heap compression with all offsets"));

	TInt n;
	for (n=0x4000; n<=0x5000; n+=4)
		{
		TAny* p1=User::Alloc(0x10000);
		test(p1!=NULL);
		User::Free(p1);
		TAny* p2=User::Alloc(n);
		test(p2!=NULL);
		TAny* p3=User::Alloc(0x4000);
		test(p3!=NULL);
		User::Free(p3);
		User::CompressAllHeaps();
		User::Allocator().Check();
		User::Free(p2);
		}

	test.End();
	return 0;
	}
