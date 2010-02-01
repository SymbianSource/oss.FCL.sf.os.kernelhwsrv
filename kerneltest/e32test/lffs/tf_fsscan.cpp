// Copyright (c) 2001-2009 Nokia Corporation and/or its subsidiary(-ies).
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
// this isn't actually a test of the media driver.It scans all loaded media
// drivers a filesystems and prints information about them
// 
//

#include <e32std.h>
#include <e32std_private.h>
#include <e32svr.h>
#include <e32test.h>
#include <f32file.h>



LOCAL_C void ScanPhysicalDeviceDrivers()
	/**
	 * Displays list of loaded PDDs
	 */
	{
	RDebug::Print( _L("Scanning loaded media drivers...") );

	_LIT( KSearchName, "Media.*" );
	TFindPhysicalDevice findHb;
	findHb.Find(KSearchName);
	TFullName name;
	while (findHb.Next(name)==KErrNone)
		{
		_LIT( KFormatStr, "    %S" );
		RDebug::Print( KFormatStr, &name );
		}

	RDebug::Print( _L("\r\n") );
	}


LOCAL_C void ShowDriveCaps( const TLocalDriveCaps& aCaps )
	{
	RDebug::Print( _L("    size=%ld"), aCaps.iSize );
	RDebug::Print( _L("    media type=%d"), aCaps.iType );
	RDebug::Print( _L("    connection bus=%d"), aCaps.iConnectionBusType );
	RDebug::Print( _L("    drive attributes=0x%x"), aCaps.iDriveAtt );
	RDebug::Print( _L("    media attributes=0x%x"), aCaps.iMediaAtt );
    RDebug::Print( _L("    base address=0x%x"), aCaps.iBaseAddress );

	_LIT( KFsysUnknown, "Unknown" );
	_LIT( KFsysRom, "ROM" );
	_LIT( KFsysFat, "FAT" );
	_LIT( KFsysLffs, "LFFS" );
	const TDesC* fsType;
	switch( aCaps.iFileSystemId )
		{
		case KDriveFileSysFAT:
			fsType = &KFsysFat;
			break;
		case KDriveFileSysROM:
			fsType = &KFsysRom;
			break;
		case KDriveFileSysLFFS:
			fsType = &KFsysLffs;
			break;
		default:
			fsType = &KFsysUnknown;
			break;
		}

	RDebug::Print( _L("    filesystem id=%S (%d)\r\n"), fsType, aCaps.iFileSystemId );
	}

LOCAL_C void ShowVariantDriveInfo()
	/**
	 * Display drive mapping info from variant/ASSP layers
	 */
	{
	RDebug::Print( _L("Variant drive info...") );


	// Drive info
	TDriveInfoV1Buf driveInfo;
	TInt r=UserHal::DriveInfo(driveInfo);
	if( KErrNone == r )
		{
		RDebug::Print( _L("Total supported drives = %d"), driveInfo().iTotalSupportedDrives );
		}
	else
		{
		RDebug::Print( _L("!! Failed to get drive info (e=%d)"), r );
		}

	// Attempt to open local drives
	for( TInt i = 0; i < KMaxLocalDrives; i++ )
		{
		TBusLocalDrive drive;
		TBool changedFlag = EFalse;
		TInt r = drive.Connect( i, changedFlag );
		if( KErrNone == r )
			{
			RDebug::Print( _L("LocDrive %d: connected"), i );
			TLocalDriveCapsV2Buf caps;
			TInt rv = drive.Caps( caps );
			if( KErrNone == rv )
				{
				ShowDriveCaps( caps() );
				}
			else
				{
				RDebug::Print( _L("    failed to get caps(%d)"), rv );
				}
			}
		else
			{
			RDebug::Print( _L("LocDrive %d: not available(%d)"), r );
			}
		drive.Disconnect();
		}

	RDebug::Print( _L("\r\n") );
	}



LOCAL_C void ShowVolumeInfo( const TVolumeInfo& aInfo )
	{
	
	RDebug::Print( _L("    media type=%d"), aInfo.iDrive.iType );
	RDebug::Print( _L("    drive attributes=0x%x"), aInfo.iDrive.iDriveAtt );
	RDebug::Print( _L("    media attributes=0x%x"), aInfo.iDrive.iMediaAtt );
	RDebug::Print( _L("    connection bus=%d"), aInfo.iDrive.iConnectionBusType );

	RDebug::Print( _L("    UID=0x%x"), aInfo.iUniqueID );
	RDebug::Print( _L("    size=0x%lx"), aInfo.iSize );
	RDebug::Print( _L("    free=0x%lx"), aInfo.iFree );
	RDebug::Print( _L("    name=%S"), &aInfo.iName );
	}


LOCAL_C void ShowDriveMountInfo()
	/**
	 * Show mounted filesystems
	 */
	{
	RDebug::Print( _L("Scanning drives") );

	RFs fs;
	TInt rv = fs.Connect();
	if( KErrNone != rv )
		{
		RDebug::Print( _L("!! Failed to connect to F32(%d)"), rv );
		return;
		}

	for( TInt i = EDriveA; i <= EDriveZ; i++ )
		{
		RDebug::Print( _L("Drive %c:"), i+'A' );

		TFullName name;
		rv = fs.FileSystemName(name, i);
		if( KErrNone != rv && KErrNotFound != rv)
			{
			RDebug::Print( _L("    !! failed to read filesystem name(%d)"), rv );
			}
		else 
			{
			if (name.Length() != 0)
				{
				RDebug::Print( _L("    FS=%S"), &name );
    			}
			else
				{
				RDebug::Print( _L("    no filesystem") );
				}

			TVolumeInfo volInfo;
			rv = fs.Volume( volInfo, i );
			if( KErrNone == rv )
				{
				ShowVolumeInfo( volInfo );
				}
			else
				{
				RDebug::Print( _L("    !! failed to get volume info(%d)"), rv );
				}
			RDebug::Print( _L("\r\n") );
			}


		}

	fs.Close();
	}



void E32Main()
	{
	RDebug::Print( _L("TF_FSSCAN") );
	
	ScanPhysicalDeviceDrivers();
	ShowVariantDriveInfo();
	ShowDriveMountInfo();

	RDebug::Print( _L("TF_FSSCAN done") );
	}

