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
// f32\sfsrv\cl_fraw.cpp
// 
//

#include "cl_std.h"




/**
Opens a direct access channel to the disk.

Other resources are disabled from accessing the disk until Close() is called.

Note that if any resources are currently open on the disk, an error
is returned.

@param aFs    The file server session.
@param aDrive The drive containing the disk to be accessed. Specify a drive
              in the range EDriveA to EDriveZ for drives A to Z.

@return KErrNone, if successful;
        KErrInUse is returned if any resources are currently open on the disk;
        otherwise one of the other system-wide error codes.

@capability TCB

*/
EXPORT_C TInt RRawDisk::Open(RFs& aFs,TInt aDrive)
	{
	if (!RFs::IsValidDrive(aDrive))
		return(KErrArgument);
	iDrive=aDrive;
	return(CreateSubSession(aFs,EFsRawDiskOpen,TIpcArgs(aDrive)));
	}




/**
Closes the direct access channel to the disk, and allows other resources
to access the disk.
*/
EXPORT_C void RRawDisk::Close()
	{
	CloseSubSession(EFsRawSubClose);
	}




/**
Reads directly from the disk.

The function reads a number of bytes into the specified descriptor from
the disk, beginning at the specified position.

@param aPos The position on the disk at which to begin reading.
@param aDes The descriptor into which data is to be read. 
            On return aDes contains the data read.

@panic User In debug builds, the media driver panics if aPos is larger than 
            the size of the physical media or if the end address, given by 
            aPos + the maximum length of the descriptor, is greater than 
            the size of the physical media.

@return KErrNone, if successful, otherwise one of the other system-wide error
        codes.

@capability TCB

*/
EXPORT_C TInt RRawDisk::Read(TInt64 aPos,TDes8& aDes)
	{
	TInt maxLength = aDes.MaxLength();
	if (maxLength==0)
		return(KErrNone);
	TPtrC8 tBuf((TUint8*)&aPos,sizeof(TInt64));
	return(SendReceive(EFsRawDiskRead,TIpcArgs(&aDes,maxLength,&tBuf)));
	}




/**
Writes directly to the disk.

The function writes the contents of the specified descriptor to the 
disk at position aPos.

@param aPos The position at which to begin writing.
@param aDes The descriptor containing the data to be written to the disk.

@panic User In debug builds, the media driver panics if aPos is larger than 
            the size of the physical media or if the end address, given by 
            aPos + the maximum length of the descriptor, is greater than 
            the size of the physical media.

@return KErrNone, if successful, otherwise one of the other system-wide error
        codes.

@capability TCB

*/
EXPORT_C TInt RRawDisk::Write(TInt64 aPos,TDesC8& aDes)
	{
	TInt length = aDes.Length();
	if (length==0)
		return(KErrNone);
	TPtrC8 tBuf((TUint8*)&aPos,sizeof(TInt64));
	return(SendReceive(EFsRawDiskWrite,TIpcArgs(&aDes,length,&tBuf)));
	}




//
// Old read / write methods left in to be BC
//
class _RRawDisk : public RRawDisk
	{
public:
	IMPORT_C TInt Read(TInt aPos,TDes8& aDes);
	IMPORT_C TInt Write(TInt aPos,TDesC8& aDes);
	};




/**
Reads directly from the disk.

The function reads a number of bytes into the specified descriptor from
the disk beginning at the specified position.

@param aPos The position on the disk at which to begin reading.
@param aDes The descriptor into which data is to be read. On return, contains the
            data read. The number of bytes read is the smaller of:
            a) the total number of bytes on the disk minus aPos;
            b) the maximum length of the descriptor.

@return KErrNone, if successful, otherwise one of the other system-wide error
        codes.

@capability TCB
@deprecated
*/
EXPORT_C TInt _RRawDisk::Read(TInt aPos,TDes8& aDes)
	{
	TInt maxLength = aDes.MaxLength();
	if (maxLength==0)
		return(KErrNone);
	TInt64 pos = MAKE_TINT64(0,aPos);
	TPtrC8 tBuf((TUint8*)&pos,sizeof(TInt64));
	return(SendReceive(EFsRawDiskRead,TIpcArgs(&aDes,maxLength,&tBuf)));
	}




/**
Writes directly to the disk.

The function writes the contents of the specified descriptor to the 
disk at position aPos.

@param aPos The position at which to begin writing.
@param aDes The descriptor containing the data to be written to the disk.

@return KErrNone, if successful, otherwise one of the other system-wide error
        codes.

@capability TCB
@deprecated
*/
EXPORT_C TInt _RRawDisk::Write(TInt aPos,TDesC8& aDes)
	{
	TInt length = aDes.Length();
	if (length==0)
		return(KErrNone);
	TInt64 pos = MAKE_TINT64(0,aPos);
	TPtrC8 tBuf((TUint8*)&pos,sizeof(TInt64));
	return(SendReceive(EFsRawDiskWrite,TIpcArgs(&aDes,length,&tBuf)));
	}
