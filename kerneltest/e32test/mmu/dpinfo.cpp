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
// e32test\mmu\dpinfo.cpp
// 
//

#include <e32debug.h>
#include <hal.h>
#include <d32locd.h>
#include "paging_info.h"

TInt E32Main()
	{
	TInt cmdLineLen = User::CommandLineLength();
	if (cmdLineLen == 0)
		return PagingInfo::PrintAll();
	else
		{
		RBuf cmdLine;
		TInt r = cmdLine.Create(cmdLineLen);
		if (r != KErrNone)
			return r;
		User::CommandLine(cmdLine);

		if (cmdLine == _L("-r"))
			{
			RDebug::Printf("Resetting demand paging info");
			return PagingInfo::ResetAll();
			}
		else
			{
			RDebug::Printf("usage: dpinfo [-r]");
			return KErrArgument;
			}
		}
	}
