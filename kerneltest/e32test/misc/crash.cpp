// Copyright (c) 1998-2009 Nokia Corporation and/or its subsidiary(-ies).
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
// e32test\misc\crash.cpp
// 
//

#include <e32std.h>
#include <e32std_private.h>

GLDEF_C TInt E32Main()
	{
	TBuf<256> cmd;
	User::CommandLine(cmd);
	User::SetCritical(User::ESystemCritical);
	cmd.Trim();
	if (cmd.Length() && TChar(cmd[0]).IsDigit())
		{
		TLex lex(cmd);
		TInt time=0;
		TInt r=lex.Val(time);
		if (r==KErrNone && time>0)
			User::AfterHighRes(time*1000000);
		}
	*((TInt*)0x80000001)=0;
	return 0;
	}
