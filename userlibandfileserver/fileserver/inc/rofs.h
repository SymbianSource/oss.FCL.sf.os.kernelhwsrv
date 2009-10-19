// Copyright (c) 1995-2009 Nokia Corporation and/or its subsidiary(-ies).
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
// WARNING: This file contains some APIs which are internal and are subject
//          to change without notice. Such APIs should therefore not be used
//          outside the Kernel and Hardware Services package.
//

/**
 @file
 @internalAll
*/

#ifndef ROFS_H
#define ROFS_H

/**
@internalAll
@released
*/
const TUint16 KRofsFormatVersion = 0x200;

class TRofsEntry
/**
@internalAll
@released
*/
	{
public:
	TUint16 iStructSize;	// Total size of entry, header + name + any padding
	TUint8	iUids[sizeof(TCheckedUid)];		// A copy of all the UID info
	TUint8	iNameOffset;	// offset of iName from start of entry
	TUint8	iAtt;			// standard file attributes
	TUint32 iFileSize;		// real size of file in bytes (may be different from size in image)
							// for subdirectories this is the total size of the directory
							// block entry excluding padding
	TUint32 iFileAddress;	// address in image of file start
	TUint8	iAttExtra;		// extra ROFS attributes (these are inverted so 0 = enabled)
	TUint8	iNameLength;	// length of iName
	TUint16 iName[1];
	};

/**
@internalAll
@released
*/
const TUint KRofsEntryNameOffset = _FOFF( TRofsEntry, iName );
/**
@internalAll
@released
*/
const TUint KRofsEntryHeaderSize = KRofsEntryNameOffset;

class TRofsDir
/**
@internalAll
@released
*/
	{
	public:
	TUint16	iStructSize;		// Total size of this directory block including padding
	TUint8	padding;
	TUint8	iFirstEntryOffset;	// offset to first entry
	TUint32 iFileBlockAddress;	// address of associated file block
	TUint32	iFileBlockSize;		// size of associated file block
	TRofsEntry	iSubDir;		// first subdir entry (not present if no subdirs)
	};

/**
@internalAll
@released
*/
const TUint KRofsDirFirstEntryOffset = _FOFF( TRofsDir, iSubDir );
/**
@internalAll
@released
*/
const TUint KRofsDirHeaderSize = KRofsDirFirstEntryOffset;

class TRofsHeader
/**
@internalAll
@released
*/
	{
	public:
	TUint8		iIdentifier[4];		// ROFS identifier
	TUint8		iHeaderSize;
	TUint8		iReserved;
	TUint16		iRofsFormatVersion;
	TUint		iDirTreeOffset;	// offset to start of directory structure
	TUint		iDirTreeSize;		// size in bytes of directory
	TUint		iDirFileEntriesOffset;	// offset to start of file entries
	TUint		iDirFileEntriesSize;	// size in bytes of file entry block
	TInt64		iTime;
	TVersion	iImageVersion;		// licensee image version
	TUint32		iImageSize;
	TUint		iCheckSum;
	TUint32		iMaxImageSize;
	};
/**
@internalAll
@released
*/

class TExtensionRofsHeader
	{
	public:    
	TUint8		iIdentifier[4];		// ROFS Extension identifier
	TUint8		iHeaderSize;
	TUint8		iReserved;
	TUint16		iRofsFormatVersion;
	TUint		iDirTreeOffset;	        // offset to start of directory structure
	TUint		iDirTreeSize;		// size in bytes of directory
	TUint		iDirFileEntriesOffset;	// offset to start of file entries
	TUint		iDirFileEntriesSize;	// size in bytes of file entry block
	TInt64		iTime;
	TVersion	iImageVersion;		// licensee image version
	TUint32		iImageSize;
	TUint		iCheckSum;
	TUint32		iMaxImageSize;
	};

const TUint KRofsHeaderSize = sizeof(TRofsHeader);
const TUint KExtensionRofsHeaderSize = sizeof(TExtensionRofsHeader);

/**
@internalAll
@released
*/
const TInt16 KEarliestSupportedFormatVersion = KRofsFormatVersion;
/**
@internalAll
@released
*/
const TInt16 KLatestSupportedFormatVersion = KRofsFormatVersion;

const TUint KRofsMangleNameLength = 7;

#endif
