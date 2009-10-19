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
// @file
// various FAT test utilities
//
//


#define __E32TEST_EXTENSION__
#include <f32file.h>
#include <e32test.h>

#include "fs_utils.h"

extern RTest test;

// Wait until file server cleanup following session disconnect has completed
void FsBarrier()
	{
	RFs fs;
	TInt r = fs.Connect();
	test_KErrNone(r);
	TRequestStatus s;
	fs.NotifyDestruction(s);
	test_Equal(KRequestPending, s.Int());

	// This sends the disconnect message to the file server.
	fs.Close();

	// Wait for session to be destroyed. This will happen when the file server
	// completes the disconnect message.
	User::WaitForRequest(s);
	}


