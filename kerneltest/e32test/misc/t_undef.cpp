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
// e32test\misc\t_undef.cpp
// 
//

#include <e32test.h>
#include "u32std.h"

LOCAL_D RTest test(_L("UNDEF"));

#ifdef __CPU_X86
# ifdef __GCC32__
LOCAL_C void Crash()
	{
	asm("int 6");
	}
# else
__declspec(naked) LOCAL_C void Crash()
	{
	_asm int 6
	}
# endif
#else 
void Crash();
#endif

GLDEF_C TInt E32Main()
	{
	test.Title();

	Crash();

	return 0;
	}
