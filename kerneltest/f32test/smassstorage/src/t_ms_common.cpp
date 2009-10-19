// Copyright (c) 2004-2009 Nokia Corporation and/or its subsidiary(-ies).
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
// Common test code for Mass Storage
// 
//

/**
 @file
 @internalTechnology
*/

#include <e32std.h>
#include <e32std_private.h>
#include <f32file.h>
#include "t_ms_main.h"

GLDEF_C TBool isDriveRemovable(TInt aDriveNumber)
	{
	RFs fs;
	test(KErrNone == fs.Connect());
	TDriveInfo info;
	fs.Drive(info, aDriveNumber);
	fs.Close();
	if (info.iDriveAtt & KDriveAttRemovable)
		{
		return ETrue;
		}
	return EFalse;
	}
