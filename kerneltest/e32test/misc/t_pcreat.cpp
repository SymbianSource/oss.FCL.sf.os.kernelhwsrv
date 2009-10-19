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
// e32test\misc\t_pcreat.cpp
// 
//

#include <e32std.h>
#include <e32std_private.h>
#include <e32svr.h>

TInt E32Main()
	{
	TUint id = (TUint)RProcess().Id();
	RDebug::Print(_L("Process %d start"),id);

	TAny* p = User::Alloc(8192);
	memset(p, id, 8192);

	const TDesC& name = RProcess().FileName();
	RProcess next;
	TInt r = next.Create(name, KNullDesC);
	if (r != KErrNone)
		User::Panic(_L("xx"),r);
	next.Resume();
	return 0;
	}
