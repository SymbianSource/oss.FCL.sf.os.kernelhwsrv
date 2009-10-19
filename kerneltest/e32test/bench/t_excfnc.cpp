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
// e32test\bench\t_excfnc.cpp
// 
//

#include <e32std.h>
#include <e32std_private.h>
#include <e32base.h>
#include <e32base_private.h>

GLREF_D TInt64 count;

GLDEF_C TInt FastExec(TAny*)
	{

	FOREVER
		{
		User::NTickCount();
		User::NTickCount();
		User::NTickCount();
		User::NTickCount();
		User::NTickCount();
		User::NTickCount();
		User::NTickCount();
		User::NTickCount();
		User::NTickCount();
		User::NTickCount();
		User::NTickCount();
		User::NTickCount();
		User::NTickCount();
		User::NTickCount();
		User::NTickCount();
		User::NTickCount();
		User::NTickCount();
		User::NTickCount();
		User::NTickCount();
		User::NTickCount();
		++count;
		}
	}

GLDEF_C TInt SlowExec(TAny*)
	{
    RThread x;
	FOREVER
		{
        x.Id();
        x.Id();
        x.Id();
        x.Id();
        x.Id();
        x.Id();
        x.Id();
        x.Id();
        x.Id();
        x.Id();
        x.Id();
        x.Id();
        x.Id();
        x.Id();
        x.Id();
        x.Id();
        x.Id();
        x.Id();
        x.Id();
        x.Id();
		++count;
		}
	}
