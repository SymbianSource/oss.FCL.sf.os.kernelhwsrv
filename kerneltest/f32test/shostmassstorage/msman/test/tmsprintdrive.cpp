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
//


#include <e32def.h>
#include <e32cmn.h>
#include <f32file.h>
#include <e32test.h>

#include "tmsprintdrive.h"

extern RTest test;
extern RFs fsSession;

void TMsPrintDrive::FormatDriveInfo(TDes& aBuffer, const TDriveInfo& aDriveInfo)
    {
    // Append battery, media and drive information to aBuffer
    // Define descriptor constants using the _LIT macro
    _LIT(KFormatString,"Type=0x%02x,Connection Bus=0x%02x,DriveAtt=0x%02x,MediaAtt=0x%02x\r\n");
    _LIT(KConnectionBusInternal,"Connection Bus is Internal\r\n");
    _LIT(KConnectionBusUsb,"Connection Bus is USB\r\n");
    _LIT(KConnectionBusUnknown,"Connection Bus is Unknown\r\n");
    _LIT(KNotPresent,"No media present\r\n");
    _LIT(KFloppy,"Media is floppy disk\r\n");
    _LIT(KHard,"Media is hard disk\r\n");
    _LIT(KCDROM,"Media is CD-ROM\r\n");
    _LIT(KRam,"Media is RAM\r\n");
    _LIT(KFlash,"Media is flash\r\n");
    _LIT(KRom,"Media is ROM\r\n");
    _LIT(KRemote,"Media is remote\r\n");
    _LIT(KNANDFlash,"Media is NAND flash\r\n");
    _LIT(KUnknown,"Media unknown\r\n");
    _LIT(KDriveAtts,"Drive attributes:");
    _LIT(KLocal," local");
    _LIT(KROMDrive," ROM");
    _LIT(KRedirected," redirected");
    _LIT(KSubstituted," substituted");
    _LIT(KInternal," internal");
    _LIT(KRemovable," removable");
    _LIT(KExternal," external");
    _LIT(KMediaAtts,"\r\nMedia attributes:");
    _LIT(KDynamic," dynamic");
    _LIT(KDual," dual-density");
    _LIT(KFormattable," formattable");
    _LIT(KLockable," lockable");
    _LIT(KLocked," locked");
    _LIT(KHasPassword," has password");
    _LIT(KWriteProtected," write-protected");
    _LIT(KNewLine,"\r\n");

    aBuffer.AppendFormat(KFormatString, TInt(aDriveInfo.iType),
                         TInt(aDriveInfo.iConnectionBusType),
                         TInt(aDriveInfo.iDriveAtt),
                         TInt(aDriveInfo.iMediaAtt));

    switch (aDriveInfo.iConnectionBusType)
        {
        case EConnectionBusInternal:
            aBuffer.Append(KConnectionBusInternal);
            break;
        case EConnectionBusUsb:
            aBuffer.Append(KConnectionBusUsb);
            break;
        default:
            aBuffer.Append(KConnectionBusUnknown);
        }

    switch (aDriveInfo.iType)
            {
        case EMediaNotPresent:
            aBuffer.Append(KNotPresent);
            break;
        case EMediaFloppy:
            aBuffer.Append(KFloppy);
            break;
        case EMediaHardDisk:
            aBuffer.Append(KHard);
            break;
        case EMediaCdRom:
            aBuffer.Append(KCDROM);
            break;
        case EMediaRam:
            aBuffer.Append(KRam);
            break;
        case EMediaFlash:
            aBuffer.Append(KFlash);
            break;
        case EMediaRom:
            aBuffer.Append(KRom);
            break;
        case EMediaRemote:
            aBuffer.Append(KRemote);
            break;
        case EMediaNANDFlash:
            aBuffer.Append(KNANDFlash);
            break;
        default:
            aBuffer.Append(KUnknown);

        }
        aBuffer.Append(KDriveAtts);
        if (aDriveInfo.iDriveAtt & KDriveAttLocal)
           aBuffer.Append(KLocal);
        if (aDriveInfo.iDriveAtt & KDriveAttRom)
            aBuffer.Append(KROMDrive);
        if (aDriveInfo.iDriveAtt & KDriveAttRedirected)
           aBuffer.Append(KRedirected);
        if (aDriveInfo.iDriveAtt & KDriveAttSubsted)
           aBuffer.Append(KSubstituted);
        if (aDriveInfo.iDriveAtt & KDriveAttInternal)
           aBuffer.Append(KInternal);
        if (aDriveInfo.iDriveAtt & KDriveAttRemovable)
           aBuffer.Append(KRemovable);
        if (aDriveInfo.iDriveAtt & KDriveAttExternal)
           aBuffer.Append(KExternal);
        aBuffer.Append(KMediaAtts);
        if (aDriveInfo.iMediaAtt & KMediaAttVariableSize)
            aBuffer.Append(KDynamic);
        if (aDriveInfo.iMediaAtt & KMediaAttDualDensity)
            aBuffer.Append(KDual);
        if (aDriveInfo.iMediaAtt & KMediaAttFormattable)
            aBuffer.Append(KFormattable);
        if (aDriveInfo.iMediaAtt & KMediaAttWriteProtected)
            aBuffer.Append(KWriteProtected);
        if (aDriveInfo.iMediaAtt & KMediaAttLockable)
            aBuffer.Append(KLockable);
        if (aDriveInfo.iMediaAtt & KMediaAttLocked)
            aBuffer.Append(KLocked);
        if (aDriveInfo.iMediaAtt & KMediaAttHasPassword)
            aBuffer.Append(KHasPassword);
        aBuffer.Append(KNewLine);
    }


void TMsPrintDrive::FormatVolumeInfo(TDes& aBuffer, const TVolumeInfo& aVolumeInfo)
    {
    // Append volume information to aBuffer
    _LIT(KUID,"Unique ID: %08x\r\n");
    _LIT(KSize,"Size: 0x%Lx bytes\r\n");
    _LIT(KFree,"Free space: 0x%Lx bytes\r\n");
    _LIT(KVolName,"Volume name: %S\r\n");
    aBuffer.AppendFormat(KUID, aVolumeInfo.iUniqueID);
    aBuffer.AppendFormat(KSize, aVolumeInfo.iSize);
    aBuffer.AppendFormat(KFree, aVolumeInfo.iFree);
    aBuffer.AppendFormat(KVolName, &aVolumeInfo.iName);
    }


void TMsPrintDrive::VolInfoL(TInt aDriveNumber)
    {
    _LIT(KMessage,"Drive Info\r\n");
    _LIT(KValidDriveMsg,"Valid drives as characters (and as numbers) are:");
    _LIT(KDriveChar,"%c");
    _LIT(KDriveNum,"(%d) ");
    _LIT(KNewLine,"\r\n");
    _LIT(KAvailDriveMsg,"Using DriveList(), available drives are: ");
    _LIT(KDriveAtts,"%c: %02x ");
    _LIT(KDriveInfo,"Drive information for %c: drive is:\r\n%S\r\n");
    _LIT(KVolInfo,"Volume information for %c: is:\r\n%S\r\n");

    test.Printf(KMessage);
    test.Printf(KValidDriveMsg);

    TChar driveLetter;

    if (fsSession.IsValidDrive(aDriveNumber))
        {
        fsSession.DriveToChar(aDriveNumber,driveLetter);
        test.Printf(KDriveChar,TUint(driveLetter));
        fsSession.CharToDrive(driveLetter, aDriveNumber);
        test.Printf(KDriveNum, aDriveNumber);
        }

    test.Printf(KNewLine);

    TDriveList drivelist;
    User::LeaveIfError(fsSession.DriveList(drivelist));
    // A TDriveList (the list of available drives), is an array of
    // 26 bytes. Each byte with a non zero value signifies that the
    // corresponding drive is available.

    test.Printf(KAvailDriveMsg);

    if (drivelist[aDriveNumber]) // if drive-list entry non-zero, drive is available
        {
        User::LeaveIfError(fsSession.DriveToChar(aDriveNumber,driveLetter));
        // The following line prints the drive letter followed by the hex value
        // of the integer indicating that drive's attributes
        test.Printf(KDriveAtts,TUint(driveLetter), drivelist[aDriveNumber]);
        }

    test.Printf(KNewLine);

    // Print information about available drives
    TBuf<200> buffer;
    TDriveInfo driveInfo;

    fsSession.Drive(driveInfo, aDriveNumber);
    if (driveInfo.iDriveAtt == KDriveAbsent)
        {
        }
    else
        {
        FormatDriveInfo(buffer, driveInfo);
        User::LeaveIfError(fsSession.DriveToChar(aDriveNumber, driveLetter));
        test.Printf(KDriveInfo, TUint(driveLetter), &buffer);
        buffer.Zero();
        }

    // Print volume information for all available drives. TVolumeInfo
    // provides drive information, and additional information about
    // the volume. Just print out the volume information.

    TVolumeInfo volumeInfo;

    TInt err=fsSession.Volume(volumeInfo, aDriveNumber);
    if (err!=KErrNotReady)
        // Volume() returns KErrNotReady if no volume present.
        // In this case, check next drive number
        {
        buffer.Zero();
        FormatVolumeInfo(buffer, volumeInfo);
        User::LeaveIfError(fsSession.DriveToChar(aDriveNumber,driveLetter));
        test.Printf(KVolInfo, (TUint)driveLetter, &buffer);
        }
    }

