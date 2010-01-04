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


#include "T_TestFSY.h"

/*@{*/
_LIT(KDefaultPath,		"C:\\");
/*@}*/

CTestFileSystem::CTestFileSystem()
//
// Constructor
//
	{
	}

CMountCB* CTestFileSystem::NewMountL() const
//
// Create a new mount control block
//
	{
	return (new(ELeave) CTestMountCB);
	}

CFileCB* CTestFileSystem::NewFileL() const
//
// Create a new file
//
	{
	return (new(ELeave) CTestFileCB);
	}

CDirCB* CTestFileSystem::NewDirL() const
//
// create a new directory lister
//
	{
	return (new(ELeave) CTestDirCB);
	}

CFormatCB* CTestFileSystem::NewFormatL() const
//
// Create a new media formatter
//
	{
	return (new(ELeave) CTestFormatCB);
	}

TInt CTestFileSystem::DefaultPath(TDes& aPath) const
//
// Return the intial default path
//
	{
	aPath=KDefaultPath;
	return KErrNone;
	}

void CTestFileSystem::DriveInfo(TDriveInfo& anInfo,TInt aDriveNumber) const
//
// Return drive info - iDriveAtt and iBatteryState are already set
//
	{
	TLocalDriveCapsV2Buf	localDriveCaps;
	DriveNumberToLocalDrive(aDriveNumber).Caps(localDriveCaps);
	anInfo.iMediaAtt=localDriveCaps().iMediaAtt;
	anInfo.iType=localDriveCaps().iType;
	anInfo.iDriveAtt=localDriveCaps().iDriveAtt;
	}

TBusLocalDrive& CTestFileSystem::DriveNumberToLocalDrive(TInt aDriveNumber) const
//
// Return the local drive associated with aDriveNumber
//
	{
	return(GetLocalDrive(aDriveNumber));
	}
