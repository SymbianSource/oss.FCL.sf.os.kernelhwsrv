// Copyright (c) 2007-2009 Nokia Corporation and/or its subsidiary(-ies).
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
// e32test\emul\t_emul_slave.cpp
// 
//

#include <e32std.h>
#include <e32std_private.h>
#include "t_emul.h"

void TrapExceptionInExe()
	{
	TRAP_IGNORE(User::Leave(KErrGeneral));
	}

TInt LoadLibraryAndCallFunc()
	{
	RLibrary l;
	TInt r = l.Load(KTEmulDll2Name);
	if (r != KErrNone)
		return r;
	TTrapExceptionInDllFunc func =
		(TTrapExceptionInDllFunc)l.Lookup(KTrapExceptionInDllOrdinal);
	if (func != NULL)
		func();
	else
		return KErrNotFound;
	l.Close();
	return KErrNone;
	}

GLDEF_C TInt E32Main()
	{
	TBuf<8> arg;
	User::CommandLine(arg);
	TLex lex(arg);
	TInt val;
	TInt r = lex.Val(val);
	if (r != KErrNone)
		return r;

	r = KErrNone;
	switch(val)
		{
		case ESlaveDoNothing:
			break;

		case ESlaveTrapExceptionInExe:
			TrapExceptionInExe();
			break;

		case ESlaveTrapExceptionInLinkedDll:
			TrapExceptionInDll();
			break;
			
		case ESlaveTrapExceptionInLoadedDll:
			r = LoadLibraryAndCallFunc();
			break;
		
		default:
			r = KErrNotSupported;
			break;
		}

	return r;
	}
