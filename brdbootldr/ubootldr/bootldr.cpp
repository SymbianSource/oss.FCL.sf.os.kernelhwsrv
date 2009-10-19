/*
* Copyright (c) 2009 Nokia Corporation and/or its subsidiary(-ies).
* All rights reserved.
* This component and the accompanying materials are made available
* under the terms of the License "Eclipse Public License v1.0"
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

#define FILE_ID	0x594D777D
#include "bootldr.h"

GLDEF_D TInt LoadDrive;
GLDEF_D TInt LoadFile;
GLDEF_D TInt LoadSize;
GLDEF_D TInt ImageSize;
GLDEF_D TInt ImageReadProgress;
GLDEF_D TBool ImageHeaderPresent=EFalse;
GLDEF_D TBool ImageZip=EFalse;
GLDEF_D TBool LoadToFlash=EFalse;
GLDEF_D TBool FlashBootLoader=EFalse;
GLDEF_D TInt FileSize;
GLDEF_D TLoadDevice LoadDevice;
GLDEF_D TInputFunc InputFunction;
GLDEF_D TCloseInputFunc CloseInputFunction;
GLDEF_D TBuf<256> FileName;
GLDEF_D TUint32 RamBootPhys;
GLDEF_D TUint32 * ActualDestinationAddress;
GLDEF_D TInt SerialDownloadPort;
GLDEF_D TBps SerialBaud;
GLDEF_D RFile bootFile;
GLDEF_D TBool ImageDeflated=EFalse;
GLDEF_D TBool RomLoaderHeaderExists=ETrue;
GLDEF_D TBusLocalDrive LocDrv;
GLDEF_D TBool LocDrvChg;
GLDEF_D TInt64 LocDrvPos;

GLDEF_C void BootFault(TUint aId, TInt aLine, char aFileName[])
	{
	PrintToScreen(_L("BOOTFAULT: 0x%X in file %s @ line %d"), aId, aFileName, aLine);
	User::After(1000);	// delay to let the LCD draw the message
	RDebug::Print(_L("BOOTFAULT: 0x%X in file %s @ line %d"), aId, aFileName, aLine);
	User::LeaveIfError(KErrUnknown);
//	Kern::Fault((const char*)buf.Ptr(),aLine);
	}

