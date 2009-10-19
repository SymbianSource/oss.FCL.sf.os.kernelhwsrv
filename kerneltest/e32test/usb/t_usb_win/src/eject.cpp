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
// e32test\usb\t_usb_win\src\eject.cpp
// eject functions
//

#include "stdafx.h"
#include <windows.h>
#include <winioctl.h>
#include <tchar.h>
#include <stdio.h>
#include "global.h"

// Prototypes
extern void PrintOut(BOOL screenFlag, BOOL logFlag, BOOL timeFlag, const char *format, ...);

HANDLE OpenVolume(char driveLetter)
{
    HANDLE hVolume;
    UINT uDriveType;
    TCHAR szVolumeName[8];
    TCHAR szRootName[5];
 	TCHAR cDriveLetter = driveLetter;
	
	if (driveLetter == ' ')
	{
		return INVALID_HANDLE_VALUE;
	}

    wsprintf(szRootName, "%c:\\", cDriveLetter);

    uDriveType = GetDriveType(szRootName);
    if (uDriveType != DRIVE_REMOVABLE)
	{
        return INVALID_HANDLE_VALUE;
    }

    wsprintf(szVolumeName, "\\\\.\\%c:", cDriveLetter);

    hVolume = CreateFile(   szVolumeName,
                            GENERIC_READ | GENERIC_WRITE,
                            FILE_SHARE_READ | FILE_SHARE_WRITE,
                            NULL,
                            OPEN_EXISTING,
                            0,
                            NULL );
    
    return hVolume;
}

#define LOCK_RETRY_DELAY	500       // half a second
#define LOCK_RETRY_MAX		20

BOOL LockAndDismout(HANDLE hVolume)
{
   DWORD dwBytesReturned;

   // Retry a number of times
   for (int i = 0; i < LOCK_RETRY_MAX; i++)
   {
       if (DeviceIoControl(hVolume, FSCTL_LOCK_VOLUME, NULL, 0, NULL, 0, &dwBytesReturned, NULL))
			{
			return DeviceIoControl( hVolume, FSCTL_DISMOUNT_VOLUME, NULL, 0, NULL, 0, &dwBytesReturned, NULL);
			}
       Sleep (LOCK_RETRY_MAX);
   }

   return FALSE;
}

BOOL AllowRemovalAndEject (HANDLE hVolume)
{
    DWORD dwBytesReturned;
    PREVENT_MEDIA_REMOVAL PMRBuffer;

    PMRBuffer.PreventMediaRemoval = FALSE;

    if (DeviceIoControl( hVolume, IOCTL_STORAGE_MEDIA_REMOVAL, &PMRBuffer, sizeof(PREVENT_MEDIA_REMOVAL),
                            NULL, 0, &dwBytesReturned, NULL))
	    return DeviceIoControl( hVolume, IOCTL_STORAGE_EJECT_MEDIA, NULL, 0, NULL, 0, &dwBytesReturned, NULL);

	return FALSE;   
}


BOOL EjectVolume(char driveLetter)
{
    HANDLE hVolume = INVALID_HANDLE_VALUE;


	hVolume = OpenVolume(driveLetter);

	if (hVolume == INVALID_HANDLE_VALUE)
		return FALSE;

    // Lock and dismount the volume.
    if (LockAndDismout(hVolume))
		{
        // Set prevent removal to false and eject the volume.
        AllowRemovalAndEject(hVolume);
		}

    // Close the volume so other processes can use the drive.
    if (!CloseHandle(hVolume))
        return FALSE;

    return TRUE;
}

#define WAIT_RETRIES 500
#define WAIT_TIME 250
#define WAIT_DELAY 1500

BOOL WaitDriveReady (char driveLetter)
{
    UINT uDriveType = DRIVE_UNKNOWN ;
    TCHAR szRootName[5];
	UINT waitTime = 0;

    wsprintf(szRootName, "%c:\\", driveLetter);

	for (int i = 0; i < WAIT_RETRIES && (uDriveType != DRIVE_REMOVABLE); i++)
		{
		uDriveType = GetDriveType(szRootName);
		if (uDriveType != DRIVE_REMOVABLE)
			{
			Sleep (WAIT_TIME);
			waitTime += WAIT_TIME;
			}
		else
			{
			Sleep (WAIT_DELAY);
			waitTime += WAIT_DELAY;
			}
		}
	

    PRINT_ALWAYS "Drive %c ready after %d milliseconds.\n",driveLetter,waitTime);
    return uDriveType == DRIVE_REMOVABLE;
}
