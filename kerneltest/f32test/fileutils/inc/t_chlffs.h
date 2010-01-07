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
// f32test\server\t_chlffs.h
// 
//

#if !defined(__T_CHLFFS_H__)
#define __T_CHLFFS_H__

#include <f32file.h>
#include <d32locd.h>

#if defined(__WINS__)
// #define __TEST_LFFS_ONLY__
#endif

GLREF_D RTest test;

GLREF_C TInt CheckLFFSDriveForPlatform();
GLREF_C TBool CheckMountLFFS(RFs &anFsSession,TChar aDriveLetter);
GLREF_C TBool IsTestingLFFS();
GLREF_C void TestingLFFS(TBool aSetting);
GLREF_C TInt GetDriveLFFS();
GLREF_C TBool IsDefaultDriveLFFS();
GLDEF_C TBool IsSessionDriveLFFS(RFs& aFs,TChar& aDriveLetter);
GLREF_C TBool IsNamedDriveLFFS(RFs &anFsSession,TText aDrv);
GLREF_C TBool IsFileSystemLFFS(RFs &aFsSession,TInt aDrive);
GLREF_C TInt GetLFFSControlModeSize();

#endif
