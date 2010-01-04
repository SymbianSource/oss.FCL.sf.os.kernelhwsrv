/*
* Copyright (c) 2005-2009 Nokia Corporation and/or its subsidiary(-ies).
* All rights reserved.
* This component and the accompanying materials are made available
* under the terms of "Eclipse Public License v1.0"
* which accompanies this distribution, and is available
* at the URL "http://www.eclipse.org/legal/epl-v10.html".
*
* Initial Contributors:
* Nokia Corporation - initial contribution.
*
* Contributors:
*
* Description: 
*
*/


#include "T_TestFSY1.h"

//	EPOC includes
#include <f32ver.h>

/*@{*/
_LIT(KFileSystemName,	"TestFileSystem1");

const TInt	KMajorVersionNumber=1;
const TInt	KMinorVersionNumber=0;
/*@}*/

CFileSystem* CTestFileSystem1::NewL()
//
// Return File System
//
	{
	return (new (ELeave) CTestFileSystem1);
	}

CTestFileSystem1::CTestFileSystem1()
//
// Constructor
//
	{
	}

TInt CTestFileSystem1::Install()
//
// Install the file system
//
	{
	iVersion=TVersion(KMajorVersionNumber, KMinorVersionNumber, KF32BuildVersionNumber);
	return(SetName(&KFileSystemName));
	}
TBool CTestFileSystem1::IsExtensionSupported() const
//
//	Return false to disallow mount extension on this file system
//
	{
	return EFalse;
	}

extern "C" {

EXPORT_C CFileSystem* CreateFileSystem()
//
// Create a new file system
//
	{
	return(CTestFileSystem1::NewL());
	}
}
