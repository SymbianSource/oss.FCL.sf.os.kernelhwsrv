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
// e32test\misc\eject.cpp
// 
//

#include <e32test.h>
#include <e32svr.h>
#include <d32locd.h>

TInt E32Main()
	{
	RLocalDrive d;
	TInt drive;
	TBool changed;
	TBuf<256> cmdBuf;
	User::CommandLine(cmdBuf);
	TLex cmd(cmdBuf);
	cmd.SkipSpace();
	TInt r = cmd.Val(drive);
	if (r==KErrNone)
		r = d.Connect(drive, changed);
	if (r==KErrNone)
		d.ForceMediaChange(1);
	d.Close();
	return 0;
	}


