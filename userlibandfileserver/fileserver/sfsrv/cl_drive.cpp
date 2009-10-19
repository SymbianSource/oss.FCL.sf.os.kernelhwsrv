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
// f32\sfsrv\cl_drive.cpp
// 
//

#include "cl_std.h"




EXPORT_C TDriveUnit::TDriveUnit(TInt aDrive)
/**
Constructor taking a drive number.

@param aDrive The drive number.

@panic FSCLIENT 0 if aDrive is greater than or equal to KMaxDrives or less than 0.

@see KMaxDrives
*/
	{
	__ASSERT_ALWAYS((aDrive>=0 && aDrive<KMaxDrives),Panic(EDriveUnitBadDrive));
	iDrive=aDrive;
	}




EXPORT_C TDriveUnit::TDriveUnit(const TDesC& aDriveText)
/**
Constructor taking a drive letter.

@param aDriveText A descriptor containing text whose first character is
                  the drive letter. Can be upper or lower case. Trailing text
                  is ignored.
                  
@panic FSCLIENT 1 if the drive letter is invalid, i.e. does not correspond
       to a drive number.
       
@see RFs::CharToDrive
*/
	{
	__ASSERT_ALWAYS(RFs::CharToDrive(aDriveText[0],iDrive)==0,Panic(EDriveUnitBadDriveText));
	}




EXPORT_C TDriveUnit& TDriveUnit::operator=(TInt aDrive)
/**
Assigns the drive number to the drive unit

@param aDrive The new drive number.

@return A reference to this drive unit. 

@panic FSCLIENT 0 if aDrive is greater than or equal to KMaxDrives.

@see KMaxDrives
*/
	{
	__ASSERT_ALWAYS(aDrive<KMaxDrives,Panic(EDriveUnitBadDrive));
	iDrive=aDrive;
	return *this;
	}




EXPORT_C TDriveUnit& TDriveUnit::operator=(const TDesC& aDriveText)
/**
Assigns a drive letter to the drive unit.

The letter must be between A and Z or a panic is raised. Any trailing
text within the descriptor is ignored.

@param aDriveText Descriptor containing text whose first character is
                  the drive letter. It can be upper or lower case.
                  
@return A reference to this drive unit. 

@panic FSCLIENT 1 if the drive letter is invalid, i.e. does not correspond
       to a drive number.
       
@see RFs::CharToDrive                  
*/
	{
	__ASSERT_ALWAYS(RFs::CharToDrive(aDriveText[0],iDrive)==0,Panic(EDriveUnitBadDriveText));
	return *this;
	}




EXPORT_C TDriveName TDriveUnit::Name() const
/**
Gets the drive unit as text.

The drive letter is returned with a trailing colon.

@return The drive letter and a trailing colon.

@panic FSCLIENT 0 if RFs::DriveToChar() returned an error.
*/
	{
	TChar driveLetter;
	TInt r = RFs::DriveToChar(iDrive,driveLetter);
	__ASSERT_ALWAYS(r == KErrNone, Panic(EDriveUnitBadDrive));
	TDriveName driveName;
	driveName.SetLength(2);
	driveName[0]=(TText)driveLetter;
	driveName[1]=KDriveDelimiter;
	return driveName;
	}

