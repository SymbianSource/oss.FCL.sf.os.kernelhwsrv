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
// \f32test\loader\tld_helper.cpp
// 
//

#include "t_loader_delete.h"
#include <e32debug.h>
#include <e32ldr.h>
#include <e32ldr_private.h>

TInt E32Main()
	{
#ifdef __WINS__
	// don't use JIT if running on emulator because don't want to halt
	// t_loader_delete test run.
	User::SetJustInTime(EFalse);
#endif

	TInt r;
	RProcess p;

	RLoader l;
	r = l.Connect();
	if (r != KErrNone)
		p.Panic(_L("tldh-noconn"), 0);

	
	TFileName fn;
	User::CommandLine(fn);

	TBuf16<512> aBuf;
	aBuf.Copy(fn);

	if (aBuf.FindF(KBadDescriptor) >= 0)
		{
		const TDesC16 *pBuf = &aBuf;

		((int*)pBuf)[0] = 0x7554444f; //malformed descriptor

		RDebug::Print(_L("Calling RLoader::Delete passing as an argument a malformed descriptor %S\n"),&fn );
		
		r = l.Delete(*pBuf);
		
		// panic with the reason from RLoader::Delete so t_loader_delete
		// can check for the right error code.
		p.Panic(KTldPanicCat, r);

		}
	
	// panic with the reason from RLoader::Delete so t_loader_delete
	// can check for the right error code.
	r = l.Delete(fn);
	p.Panic(KTldPanicCat, r);

	// unused return value - present to prevent compiler warning
	return KErrNone;
	}
