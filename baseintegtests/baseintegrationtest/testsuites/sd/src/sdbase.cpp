// Copyright (c) 2007-2009 Nokia Corporation and/or its subsidiary(-ies).
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
// sdserver.cpp
// Base class for all test steps
// 
//

#include "sdbase.h"

/** 
Initialises the D_MMCID device driver that will let us access the very first
sectors of the memory card. This driver is located in base/e32utils/pccd.

@param none

@return ETrue if OK, EFalse if not
*/

TBool CBaseTestSDBase::InitDeviceDriver()
	{
	TInt r;
	
	// Load Device Driver that will let us read the hidden sectors
	r = User::LoadLogicalDevice(_L("D_MMCIF"));
	if (r == KErrNone)
		{
		INFO_PRINTF1(_L("D_MMCIF.LDD loaded"));
		}
	else if (r == KErrAlreadyExists)
		{
		INFO_PRINTF1(_L("D_MMCIF.LDD already loaded"));
		}
	else
		{
		ERR_PRINTF2(_L("Could not load D_MMCIF.LDD. Return value: %d"), r);
		return EFalse;
		}
	
	iDriver.Close();
	r = iDriver.Open(0, iDriver.VersionRequired());
	if (r != KErrNone)
		{
		ERR_PRINTF2(_L("Could not open device driver. Return value: %d"), r);
		return EFalse;
		}
	
	TRequestStatus rs;
	iDriver.PwrUpAndInitStack(rs);
	User::WaitForRequest(rs);
	if (rs.Int() != KErrNone)
		{
		ERR_PRINTF2(_L("Could not power up SD stack. Return value: %d"), rs.Int());
		return EFalse;
		}
	
	TUint cardsPresentMask;
	r = iDriver.StackInfo(cardsPresentMask);
	if (r != KErrNone)
		{
		ERR_PRINTF2(_L("Could not determine number of present cards. Return value: %d"), r);
		return EFalse;
		}
	
	iDriver.SelectCard(0);
	
	r = iDriver.CardInfo(iCardInfo);
	if (r != KErrNone)
		{
		ERR_PRINTF2(_L("Could not get card info. Return value: %d"), r);
		return EFalse;
		}
	iCardSizeInSectors = I64LOW(iCardInfo.iCardSizeInBytes >> KSectorSizeShift);
	return ETrue;
	}

/** 
Starts a session with the File Server.

@param none

@return ETrue if OK, EFalse if not
*/

TBool CBaseTestSDBase::InitFileServer()
	{
	// Connect to the File Server
	TInt r;
	r = iFs.Connect();
	if (r != KErrNone)
		{
		ERR_PRINTF2(_L("Could not connect to the File Server. Return value: %d"), r);
		return EFalse;
		}
	return ETrue;
	}

/** 
Instantiate a CFileMan object.

@param none

@return ETrue if OK, EFalse if not
*/

TBool CBaseTestSDBase::InitFileMan()
	{
	// Instantiate a File Manager
	TInt r;
	if (iFileMan != NULL)
		{
		ERR_PRINTF1(_L("iFileMan already instantiated"));
		return EFalse;
		}
	TRAP(r, iFileMan = CFileMan::NewL(iFs));
	if (r != KErrNone)
		{
		ERR_PRINTF2(_L("Could not instantiate CFileMan. Return value: %d"), r);
		return EFalse;
		}
	return ETrue;
	}

/** 
Read drive to test from INI file

@param none

@return ETrue if OK, EFalse if not
*/
TBool CBaseTestSDBase::InitDriveLetter()
	{
	// Find out which drive is the removable media drive
	TPtrC ptrDriveLetter;
	TChar letter;
	_LIT(KDriveLetter, "DriveLetter");
	if (!GetStringFromConfig(_L("DefaultSection"), KDriveLetter, ptrDriveLetter))
		{
		ERR_PRINTF1(_L("INI File read error"));
		return EFalse;
		}
	letter = ptrDriveLetter[0];
	letter.UpperCase();
	if ((letter >= 'A') && (letter <= 'Z'))
		{
		iDrive = (TUint) letter - 'A';
		INFO_PRINTF2(_L("Drive to test: %c"), iDrive + 'A');
		}
	else
		{
		ERR_PRINTF2(_L("Invalid drive letter: %c"), ptrDriveLetter[0]);
		return EFalse;
		}
	return ETrue;
	}

TVerdict CBaseTestSDBase::doTestStepPostambleL()
	{
	if (iFileMan != NULL)
		{
		delete iFileMan;
		}
	iFs.Close();
	User::FreeLogicalDevice(_L("MmcIf"));
	return TestStepResult();
	}

/** 
Read a sector from the memory card

@param aSector is the sector number
@param aSectorBuffer 512-byte buffer the sector contents will be copied to

@return KErrNone if successful, otherwise any other system-wide error coed
*/
	
TInt CBaseTestSDBase::ReadSector(TInt aSector, TDes8& aSectorBuffer)
	{
	TRequestStatus rs;
	iDriver.ReadSector(rs, aSector, aSectorBuffer);
	User::WaitForRequest(rs);
	if (rs.Int() == KErrNone)
		{
		INFO_PRINTF3(_L("Read sector %08xh (%d)"), aSector, aSector);
		}
	else
		{
		ERR_PRINTF4(_L("Error during Read sector %08xh (%d): %d"), aSector, aSector, rs.Int());
		}
	return(rs.Int());
	}

/** 
Write a sector to the memory card

@param aSector is the sector number
@param aSectorBuffer 512-byte buffer containing the data to write on this sector

@return KErrNone if successful, otherwise any other system-wide error coed
*/

TInt CBaseTestSDBase::WriteSector(TInt aSector, const TDesC8& aSectorBuffer)
	{
	TRequestStatus rs;
	iDriver.WriteSector(rs, aSector, aSectorBuffer);
	User::WaitForRequest(rs);
	if (rs.Int() == KErrNone)
		{
		INFO_PRINTF3(_L("Write sector %08xh (%d)"), aSector, aSector);
		}
	else
		{
		ERR_PRINTF4(_L("Error during Write sector %08xh (%d): %d"), aSector, aSector, rs.Int());
		}
	return(rs.Int());
	}
