// Copyright (c) 2009 Nokia Corporation and/or its subsidiary(-ies).
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
// f32test\server\t_notifier_caps.cpp
// 
//

#include <f32file.h>
#include <e32test.h>
#include <e32hal.h>
#include <e32math.h>
#include <f32dbg.h>

const TInt KNotificationHeaderSize = (sizeof(TUint16)*2)+(sizeof(TUint));
const TInt KMinNotificationBufferSize = 2*KNotificationHeaderSize + 2*KMaxFileName;

TInt E32Main()
	{
	TInt r = KErrNone;
	
	CTrapCleanup* cleanup;
	cleanup = CTrapCleanup::New();
		
	RFs fs;
	fs.Connect();
	
	CFsNotify* notify = NULL;
	TRAP(r, notify = CFsNotify::NewL(fs,KMinNotificationBufferSize));
	User::LeaveIfError(r);
	
	TChar systemChar = fs.GetSystemDriveChar();
	TBuf<40> path;
	path.Append(systemChar);
	path.Append(_L(":\\PRIVATE\\01234567\\"));
	
	TBuf<8> filename;
	filename.Append(_L("file.txt"));
	
	r = notify->AddNotification((TUint)TFsNotification::ECreate,path,filename);

	delete notify;
	delete cleanup;
	fs.Close();
	return r;
	}
