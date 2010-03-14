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

/**
 @file
 @publishedAll
 @released
*/

#if !defined(__F32FILE_H__)
#define __F32FILE_H__

#if !defined(__E32BASE_H__)
#include <e32base.h>
#endif

#ifndef SYMBIAN_ENABLE_PUBLIC_PLATFORM_HEADER_SPLIT
// Old implementation including platform e32svr.h (which includes the several other platform headers)...
#if !defined(__E32SVR_H__)
#include <e32svr.h>
#endif
#include <e32ldr.h>
#else
// New implementation including only the public headers needed for f32file.h...
#include <e32ldr.h>
// And the public headers previously included via e32svr.h but otherwise not needed for f32file.h...
#include <e32def.h>
#include <e32event.h>
#include <e32debug.h>
#include <e32keys.h> 
#endif


/**
@publishedAll
@released

The session default drive.
*/
const TInt KDefaultDrive=KMaxTInt;




/**
@publishedAll
@released

Indicates a drive letter which is not in use. 

This is useful when scanning a drive list to find which drives are available.
*/
const TInt KDriveAbsent=0x00;




/**
@publishedAll
@released

The default value for the number of message slots passed to RFs::Connect().

@see RFs::Connect
*/
const TInt KFileServerDefaultMessageSlots=-1;




/**
@publishedAll
@released

The size of the array of TEntry items contained in a TEntryArray object.

@see TEntryArray
@see TEntry
*/
const TInt KEntryArraySize=(0x200*sizeof(TText));




/**
@publishedAll
@released

The character used to separate directories in the path name.
*/
const TInt KPathDelimiter='\\';




/**
@publishedAll
@released

The character used to separate the drive letter from the path.
*/
const TInt KDriveDelimiter=':';




/**
@publishedAll
@released

The character used to separate the filename from the extension.
*/
const TInt KExtDelimiter='.';




/**
@publishedAll
@released

The maximum number of available drives.
*/
const TInt KMaxDrives=26;


/**
@publishedAll
@released

The maximum number of available proxy drives.
*/
const TInt KMaxProxyDrives=KMaxDrives-KMaxLocalDrives;


/**
@publishedAll
@released

Defines a modifiable buffer descriptor to contain a drive list.

The descriptor has maximum length KMaxDrives, sufficient to contain
all possible drive letters.

@see RFs::DriveList
@see KMaxDrives
*/
typedef TBuf8<KMaxDrives> TDriveList;



/**
@publishedAll
@released

The maximum length of a drivename.

Sufficient for a drive letter and colon.
*/
const TInt KMaxDriveName=0x02;




/**
@publishedAll
@released

Defines a modifiable buffer descriptor to contain a drive name.

A drive name comprises a drive letter (A through Z) and a colon.
KMaxDriveName (2 bytes) is sufficient for a drive letter and colon.

@see TDriveUnit::Name
@see KMaxDriveName
*/
typedef TBuf<KMaxDriveName> TDriveName;




/**
@publishedAll
@released

The maximum length of a file system name or file system sub type name.
32 characters is sufficient for a file system or sub type name.
*/
const TInt KMaxFSNameLength=0x0020;




/**
@publishedAll
@released

Defines a modifiable buffer descriptor to contain a file system or file system sub type name.

@see KMaxFSNameLength
*/
typedef TBuf<KMaxFSNameLength> TFSName;




/**
@publishedAll
@released

File/directory attribute: any file without the hidden or system attribute.
*/
const TUint KEntryAttNormal=0x0000;




/**
@publishedAll
@released

File/directory attribute: read-only file or directory.
*/
const TUint KEntryAttReadOnly=0x0001;




/**
@publishedAll
@released

File/directory attribute: hidden file or directory.
*/
const TUint KEntryAttHidden=0x0002;




/**
@publishedAll
@released

File/directory attribute: system file.
*/
const TUint KEntryAttSystem=0x0004;




/**
@publishedAll
@released

File/directory attribute: volume name directory.
*/
const TUint KEntryAttVolume=0x0008;




/**
@publishedAll
@released

File/directory attribute: a directory without the hidden or system attribute.
*/
const TUint KEntryAttDir=0x0010;




/**
@publishedAll
@released

File/directory attribute: an archive file.
*/
const TUint KEntryAttArchive=0x0020;




/**
@publishedAll
@released

File/directory attribute: ROM eXecute In Place file
*/
const TUint KEntryAttXIP=0x0080;




/**
@publishedAll
@released

This file attribute bit is set if the file exists only on a remote file 
system and is not locally cached.

Due to the potential high-latency of remote file systems, applications 
(or users of applications) may make use of this bit to modify their 
behaviour when working with remote files.

This is a read-only attribute, so any attempt to set this attribute will
will be ignored.
*/
const TUint KEntryAttRemote=0x0100;




/**
@publishedAll
@released

The range of entry attributes reserved for file-system specific meanings.
File systems may assign meaning to these bits, but their definition will
not be supported nor maintained by Symbian.

All other file attribute bits are reserved for use by Symbian.

The following table summarises the assignment of attribute bits:

	 0 - KEntryAttReadOnly
	 1 - KEntryAttHidden
	 2 - KEntryAttSystem
	 3 - KEntryAttVolume
	
	 4 - KEntryAttDir
	 6 - KEntryAttArchive
	 7 - KEntryAttXIP

	 8 - KEntryAttRemote
	 9 - Reserved
	10 - Reserved
	11 - Reserved
	
	12 - Reserved
	13 - Reserved
	14 - Reserved
	15 - Reserved
	
	16 - File System Specific
	17 - File System Specific
	18 - File System Specific
	19 - File System Specific
	
	20 - File System Specific
	22 - File System Specific
	22 - File System Specific
	23 - File System Specific
	
	24 - KEntryAttPacked
	25 - Reserved
	26 - Reserved
	27 - KEntryAttMatchExclude
	
	28 - KEntryAttAllowUid
	29 - Reserved
	30 - KEntryAttMatchExclusive
	31 - Reserved
*/
const TUint KEntryAttMaskFileSystemSpecific=0x00FF0000;




/**
@publishedAll
@released

Bit mask for matching file and directory entries.

This mask ensures that directories and hidden and
system files are matched.

(Note that KEntryAttNormal matches all entry types except directories, hidden
and system entries).

@see RFs::GetDir
*/
const TUint KEntryAttMatchMask=(KEntryAttHidden|KEntryAttSystem|KEntryAttDir);





/**
@publishedAll
@released

Bit mask for matching file and directory entries.

This is used when all entry types, including hidden and system files,
but excluding the volume entry are to be matched.

@see RFs::GetDir
*/
const TUint KEntryAttMaskSupported=0x3f;




/**
@publishedAll
@released

Bit mask for matching file and directory entries.

This is used for exclusive matching. When OR'ed with one or more file attribute
constants, for example, KEntryAttNormal, it ensures that only the files with
those attributes are matched.
When OR’ed with KEntryAttDir, directories only (not hidden or system) are matched.

@see KEntryAttDir
@see KEntryAttNormal
@see RFs::GetDir
*/
const TUint KEntryAttMatchExclusive=0x40000000;




/**
@publishedAll
@released

Bit mask for feature manager file entries.

It is used in order to identify each ROM feature set data file 
uniquely in the mount order of ROM sections.

*/
const TUint KEntryAttUnique=0x01000000;




/**
@publishedAll
@released

Bit mask for matching file and directory entries.

It is used to exclude files or directories with certain attributes from
directory listings. This bitmask has the opposite effect
to KEntryAttMatchExclusive. For example:

@code
KEntryAttMatchExclude|KEntryAttReadOnly
@endcode

excludes all read only entries from the directory listing.

@code
KEntryAttMatchExclusive|KEntryAttReadOnly
@endcode
lists only read only entries.

@see KEntryAttMatchExclusive
@see RFs::GetDir
*/
const TUint KEntryAttMatchExclude=0x08000000;




/**
@publishedAll
@released

Bit mask for matching file and directory entries.

Bit mask flag used when UID information should be included in the directory
entry listing.

@see RFs::GetDir
*/
const TUint KEntryAttAllowUid=0x10000000;




/**
@publishedAll
@released

Indicates that a TEntry (that is generally returned from a TEntryArray) is
stored in packed format where the iSizeHigh and iReserved fields follow the
valid characters of the name string.  Before accessing the aforementioned
members, the entry must be unpacked.

*/
const TUint KEntryAttPacked = 0x01000000;


/**
@publishedAll
@released
*/
const TUint KMaxMapsPerCall = 0x8;




enum TNotifyType
/**
@publishedAll
@released

A set of change notification flags.

These flags indicate the kind of change that should result in notification.

This is useful for programs that maintain displays of file lists that
must be dynamically updated.

@see RFs::NotifyChange
@see RFs
@see RFile
@see RRawDisk
*/
	{
	/**
	Any change, including mounting and unmounting drives.
	*/
	ENotifyAll=0x01,
	
	
	/**
	Addition or deletion of a directory entry, or changing or formatting a disk.
	*/
	ENotifyEntry=0x02,
	
	
	/**
	Change resulting from file requests:
	RFile::Create(), RFile::Replace(), RFile::Rename(), RFs::Delete(),
	RFs::Replace(), and RFs::Rename().
	*/
	ENotifyFile=0x04,
	
	
	/**
	Change resulting from directory requests:
	RFs::MkDir(), RFs::RmDir(), and RFs::Rename().
	*/
	ENotifyDir=0x08,
	
	
	/**
	Change resulting from: RFs::SetEntry(), RFile::Set(), RFile::SetAtt(),
	RFile::SetModified() and RFile::SetSize() requests.
	*/
	ENotifyAttributes=0x10,
	
	
	/**
	Change resulting from the RFile::Write() request.
	*/
	ENotifyWrite=0x20,
	
	
	/**
	Change resulting from the RRawDisk::Write() request.
	*/
	ENotifyDisk=0x40
	};

/**
    @publishedAll
    @released

    Notification modes for safe media removal notification API

    @see RFs::NotifyDismount
*/
enum TNotifyDismountMode
	{
	/** Used by a client to register for notification of pending dismount. This is the default behaviour for RFs::NotifyDismount*/
	EFsDismountRegisterClient=0x01,
	
	/** 
    Used for graceful file system dismounting with notifying clients of a pending dismount. 
    If all clients have responded by RFs::AllowDismount(), the file system will be dismounted. 
    */
	EFsDismountNotifyClients=0x02,
	
	/**  Used to forcibly dismount the file system without notifying clients. */
	EFsDismountForceDismount=0x03,
	};


enum TFileCacheFlags
/**
@publishedAll
@released

Flags used to enable file server drive-specific caching 
*/
	{
	/**
	Enable read caching - if file explicitly opened in EFileReadBuffered mode
	*/
	EFileCacheReadEnabled = 0x01,

	/**
	Enable read caching for all files, regardless of file open mode
	*/
	EFileCacheReadOn = 0x02,

	/**
	Enable read-ahead caching - if file explicitly opened in EFileReadAheadOn mode
	*/
	EFileCacheReadAheadEnabled = 0x04,	

	/**
	Enable read-ahead caching, regardless of file open mode
	*/
	EFileCacheReadAheadOn = 0x08,	

	/**
	Enable write caching, if file explicitly opened in EFileWriteBuffered mode
	*/
	EFileCacheWriteEnabled = 0x10,	

	/**
	Enable write caching for all files, regardless of file open mode
	*/
	EFileCacheWriteOn = 0x20,	
	};

/**
@publishedAll
@released

Commands to query specific volume information.

@see TVolumeIOParamInfo
*/
enum TQueryVolumeInfoExtCmd
	{
	/**
    Queries the sub type of the file system mounted on a specified volume.
    For example, FAT12, FAT16 or FAT32.
    */
    EFileSystemSubType,
	
    /**
    Queries the I/O parameters of a specificed volume.
    This includes the block size, the cluster size and the recommended read and write sizes for the media.    
    */
    EIOParamInfo,

    /** 
    This command determines whether the volume is synchronous or asynchronous.
    A boolean value is returned within the buffer defined as TPckgBuf<TBool>. 
    ETrue for Synchronous and EFalse for Asynchronous.
    */
    EIsDriveSync,

    /**
    Query if the given drive is finalised. See RFs::FinaliseDrive() 
    Not all file systems may support this query.
    A boolean value is returned within the buffer defined as TPckgBuf<TBool>. 
    ETrue value means that the drive is finalised
    */
    EIsDriveFinalised,
	};

/**
@publishedAll
@released

Volume IO parameter information.

This class is used to return IO parameter information for a specified volume.

The volume parameter information holds recommended buffer sizes for the creation of efficient buffers for
reading and writing.

@see RFs::VolumeIOParam()
*/
class TVolumeIOParamInfo
	{
public:
	/**
	The size of a block in bytes.
	
	Reads and writes that are aligned on block boundaries are up to twice as fast as when 
	mis-aligned.	
	
	Read and write operations on certain underlying media is done in blocks.
	A write operation that modifies only part of a block is less efficient, in general, than
	one that modifies an entire block. Data throughput degrades linearly for reads and writes in smaller
	sized units. 
	*/
	TInt iBlockSize;
	/**
	The size in bytes of a single disk cluster.
	
	Read and write operations that are aligned on cluster boundaries are more efficient.
	
	The file system organises and allocates the file data on the disk in clusters where each cluster is
	one or more blocks. Files that are not zero length occupy at least one cluster of the disk, 
	so large numbers of very small files use up more disk space than expected. 
	*/
	TInt iClusterSize;
	/**
	The recommended buffer size for optimised reading performance. 
	
	The given buffer size is based on sensible benchmark testing results produced by the mobile device vendor.
	The buffer size is then added to the estart.txt file
	
	The figure is included in the estart.txt file along with the drive number and the variable name. 
	The example below shows the required format:

	[DriveC]
	RecReadBufSize 8192

	When no value is provided, value KErrNotSupported is returned.
	*/
	TInt iRecReadBufSize;
	/**
	The recommended buffer size for optimised writing performance. 
	
	The given buffer size is based on sensible benchmark testing results produced by the mobile device vendor.
	The buffer size is then added to the estart.txt file
	
	The figure is included in the estart.txt file along with the drive number and the variable name. 
	The example below shows the required format:

	[DriveC]
	RecWriteBufSize 16384
	
	When no value is provided, value KErrNotSupported is returned.
	*/
	TInt iRecWriteBufSize;


    /** 
    The maximum file size that is supported by the file system mounted on this volume. 
    Not all file system may provide this parameter;  The value KMaxTUint64 (0xffffffffffffffff) means that this particular file system hasn't 
    provided this information.
    */
    TUint64 iMaxSupportedFileSize;

private:
	/*
	Reserved space for future use
	*/
	TInt iReserved[2];
	};

enum TDriveNumber
/**
@publishedAll
@released

The drive number enumeration.
*/
	{
	EDriveA,   EDriveB,   EDriveC,   EDriveD,   EDriveE,
	EDriveF,   EDriveG,   EDriveH,   EDriveI,   EDriveJ,
	EDriveK,   EDriveL,   EDriveM,   EDriveN,   EDriveO, 
	EDriveP,   EDriveQ,   EDriveR,   EDriveS,   EDriveT,
	EDriveU,   EDriveV,   EDriveW,   EDriveX,   EDriveY,
	EDriveZ
	};




enum TEntryKey
/**
@publishedAll
@released

Flags indicating the order in which directory entries are to be sorted.

@see RFs::GetDir
@see CDirScan::SetScanDataL
@see CDir::Sort
*/
	{
	/**
	The default; no sorting takes place
	*/
	ESortNone=0,


	/**
	Sort according to alphabetic order of file and directory name.

    This setting is mutually exclusive with ESortByExt, ESortBySize,
    ESortByDate and ESortByUid.
	*/
	ESortByName,
	
	
	/**
	Sort according to alphabetic order of file extension.
	
	Files without an extension take precedence over files with an extension.
	For files with the same extension or without an extension, the default is
	to sort by name.

    This setting is mutually exclusive with ESortByName, ESortBySize,
    ESortByDate and ESortByUid.
	*/
	ESortByExt,
	
	
	/**
	Sort according to file size.

    This setting is mutually exclusive with ESortByName, ESortByExt,
    ESortByDate and ESortByUid.
	*/
	ESortBySize,
	
	
	/**
	Sort according to files' last modified time and date.
	
	By default, most recent last.

    This setting is mutually exclusive with ESortByName, ESortByExt,
    ESortBySize and ESortByUid.
	*/
	ESortByDate,
	
	
	/**
	Sort according to file UID.
 
    This setting is mutually exclusive with ESortByName, ESortByExt,
    ESortBySize and ESortByDate.
	*/
	ESortByUid,
	
	
	/**
	Qualifies the sort order; if set, directories are listed in the order in
	which they occur. 
	
	This is the default.

    This flag is mutually exclusive with EDirsFirst and EDirslast.
	*/
	EDirsAnyOrder=0,
	
	
	/**
	Qualifies the sort order; if set, directories come before files in sort order.

    This flag is mutually exclusive with EDirsAnyOrder and EDirsLast.
	*/
	EDirsFirst=0x100,
	
	
	/**
	Qualifies the sort order; if set, files come before directories in sort order.
 
    This flag is mutually exclusive with EDirsAnyOrder and EDirsFirst.
	*/
	EDirsLast=0x200,
	
	
	/**
	Qualifies the sort order; files are sorted in ascending order, i.e. from A to Z.
	This is the default behaviour.

    This flag is mutually exclusive with EDescending and EDirDescending.
	*/
	EAscending=0,
	
	
	/**
	Qualifies the sort order; files are sorted in descending order, i.e. from Z to A.
    
    This flag is mutually exclusive with EAscending and EDirDescending.
	*/
	EDescending=0x400,
	
	
	/**
	Qualifies the sort order; directories are sorted in descending order, i.e. from Z to A.
    
    This flag shall be used in combination with either EDirsFirst or EDirsLast.
    This flag is mutually exclusive with EAscending and EDescending.
	*/
	EDirDescending=0x800
	};




enum TFileMode
/**
@publishedAll
@released

Access and share modes available when opening a file.

The access mode indicates whether the file is opened just for reading or
for writing.

The share mode indicates whether other RFile objects can access the
open file, and whether this access is read only.

Use EFileShareReadersOrWriters if a client does not care whether the file has
been previously opened for ReadOnly or Read/Write access.

If EFileShareReadersOrWriters is not used, then a client needs to cooperate with 
other clients in order to open the file with the correct share mode, either
EFileShareReadersOnly or EFileShareAny, depending on the share mode used when
the file was originally opened.

To open a file for reading and writing with read and write shared access, 
use:

@code
_LIT(KFilename, "filename.ext");
RFile file;
file.Open(theFs, KFilename, EFileShareAny|EFileWrite);
@endcode

If another instance of RFile tries to open this file in EFileShareExclusive
or EFileShareReadersOnly mode, access is denied. However, it can be opened
in EFileShareAny mode or EFileShareReadersOrWriters mode.

If a file is opened with EFileShareReadersOrWriters, and the file is opened for
sharing by another client, then the file share mode is promoted to the new share 
mode. When the file handle is closed then the share mode is demoted back to 
EFileShareReadersOrWriters.

@code

Table of FileShare promotion rules
----------------------------------

Client A					Client B							Resultant Share Mode 
--------					--------							--------------------
ReadersOnly					ReadersOnly						ReadersOnly
ReadersOnly					ReadersOrWriters|EFileRead			ReadersOnly
ReadersOnly					ReadersOrWriters|EFileWrite		INCOMPATIBLE
ReadersOnly					Any								INCOMPATIBLE

ReadersOrWriters|EFileRead	ReadersOnly						ReadersOnly
ReadersOrWriters|EFileRead	ReadersOrWriters|EFileRead		ReadersOrWriters
ReadersOrWriters|EFileRead	ReadersOrWriters|EFileWrite		ReadersOrWriters
ReadersOrWriters|EFileRead	Any								Any

ReadersOrWriters|EFileWrite	ReadersOnly						INCOMPATIBLE
ReadersOrWriters|EFileWrite	ReadersOrWriters|EFileRead			ReadersOrWriters
ReadersOrWriters|EFileWrite	ReadersOrWriters|EFileWrite		ReadersOrWriters
ReadersOrWriters|EFileWrite	Any								Any

Any							ReadersOnly						INCOMPATIBLE
Any							ReadersOrWriters|EFileRead			Any
Any							ReadersOrWriters|EFileWrite		Any
Any							Any								Any
@endcode

Use the following guidance notes for selecting FileShare mode with shared RFile objects:

EFileShareAny
- Use this mode to request both read and write access when another client needs
to write to the file and respective client access to the file is coordinated.
- To open a file for non-exclusive write, use EFileShareAny | EFileWrite. 
- It is recommended that either EFileShareAny or EFileShareAny | EFileRead are
not used. These combinations will block users attempting to use the 
EFileShareReadersOnly mode even if all the EFileShareAny handles do not have
the EFileWrite bit set as the EFileRead and EFileWrite bits have no affect on 
sharing. Use either EFileShareReadersOnly or EFileShareReadersOrWriters.

EFileShareReadersOrWriters 
- Use this mode when it does not matter if another file writes to the file and
file access can not be coordinated as other clients are unknown.
- To open a file for shared read access whilst permitting writers, use 
EFileShareReadersOrWriters | EFileRead.

- For write access with unrestricted share mode, 
EFileShareReadersOrWriters | EFileWrite may be used however 
EFilesShareAny | EFileWrite is preferred.

EFileShareReadersOnly
- Use this mode to get read access to the file and deny write access for any 
other handles on this file. 
- To open a file for shared read access whilst disallowing writers use 
EFileShareReadersOnly.


Files may be opened in text or binary mode. Native Symbian OS application 
files are nearly all binary, (so they will usually be opened in binary mode).
However, they can be opened in text mode (to support testing, and to make them
compatible with text formats on remote systems).
Symbian OS native text format uses CR-LF (ASCII 0x0d, 0x0a) to denote the end of
a line. When reading, however, any combination of CR, LF, LF-CR or CR-LF is
recognised as the end of a line. Where a remote file system uses a different
format, it is the responsibility of the installable file system to present
an interface for text files which conforms with this format.

The share mode may be OR’ed with either EFileStream or EFileStreamText.

Additionally, it may be OR’ed with either EFileRead or EFileWrite.
*/
	{
	/**
	Exclusive access for the program opening the file.
	
	No other program can access the file until it is closed.
    If another program is already accessing the file in any share mode, then
    an attempt to open it with an EFileShareExclusive will fail.
	*/
	EFileShareExclusive,


	/**
	Read-only sharing.
	
	This means that the file may only be accessed for reading.
    A file cannot be opened using a share mode of EFileShareReadersOnly with
    an EFileWrite flag.
	*/
	EFileShareReadersOnly,


	/**
	Shared access for reading and writing.
	
	This means that other programs may share access to the file for reading
	and writing with the program which opened the file.

	When using this mode, the program is expecting another program to be able
	to write to the file, so is not compatible with EFileShareReadersOnly.
	*/
	EFileShareAny,


	/**
	Shared access for reading and writing.
	
	This means that other programs may share access to the file for reading
	and writing with the program which opened the file.  
	
	When using this mode, the program does not care if another program has
	the file open for read or write access.
	*/
	EFileShareReadersOrWriters,


	/**
	For files to be opened in binary mode.
	*/
	EFileStream=0,


	/**
	For files to be opened in text mode.
	*/
	EFileStreamText=0x100,


	/**
	The file may be read from but not written to.
	*/
	EFileRead=0,


	/**
	The file may be read from and written to
	
	Cannot be combined with a share mode of EFileShareReadersOnly.
	*/
	EFileWrite=0x200,

	/**
	Specifies that an asynchronous read request should not be completed 
	until all requested data becomes available.
	
	Cannot be combined with the EFileShareExclusive or EFileShareReadersOnly
	share modes as this will prohibit a writer from updating the file.
	*/
	EFileReadAsyncAll=0x400,

	/**
	Enables write buffering
	*/
	EFileWriteBuffered	=0x00000800,

	/**
	Disables write buffering
	*/
	EFileWriteDirectIO	=0x00001000,

	/**
	Enables read buffering
	*/
	EFileReadBuffered	=0x00002000,

	/**
	Disables read buffering
	*/
	EFileReadDirectIO	=0x00004000,

	/**
	Enables read ahead. 
	*/
	EFileReadAheadOn	=0x00008000,

	/**
	Disables read ahead.
	*/
	EFileReadAheadOff	=0x00010000,

	/**
	Enable delete on close
	*/
	EDeleteOnClose		=0x00020000,
	
	/**
	Enables operations on large files.
	
	@internalTechnology
	
	*/
	EFileBigFile        =0x00040000

	};




/**
@publishedAll
@released

Bit mask provided for retrieving a file's share mode.

@see TFileMode
*/
const TUint KFileShareMask=0xff;




enum TFormatMode 
/**
@publishedAll
@released

The format method.
*/
	{
	/**
	Indicates a high density floppy disk to be formatted.
	
	Obsolete.
	
	Can be ORed with EFullFormat or EQuickFormat, but does not
	have any effect.
	*/
	EHighDensity,
    
    
    /**
    Indicates a low density floppy disk to be formatted.
	
	Obsolete. 
	
	Can be ORed with EFullFormat or EQuickFormat, but does not
	have any effect.
	*/
	ELowDensity,


	/**
	Performs a full format, erasing whole media content and creating 
	new file system layout.
	
	This is the default mode.
	*/
	EFullFormat=0,


    /**
    Performs a quick media format, erasing only required minimum media content. 
	For example, for FAT file system it resets FAT and root directory content. 
	Also preserves bad sectors if there are some on the volume.
	*/
	EQuickFormat=0x100,
	
	
	/**
	Indicates a custom formatting mode. In this mode some optional file system 
	specific parameters may be passed to RFormat::Open().
	
	@see TLDFormatInfo
	@see TInt RFormat::Open(RFs &aFs, const TDesC &aName, TUint aFormatMode, TInt &aCount, const TDesC8 &anInfo);
	*/
	ESpecialFormat=0x200,

	/**
	Forced erase of locked media
	*/
	EForceErase=0x400,

    /** 
    This flag enables formatting the volume even if it has files or directories opened on it.
    If this flag is specified, the volume will be forcedly dismounted before performing media formatting.
 
    Even with this flag the RFormat::Open() can fail with KErrInUse in following cases:
        1. if there are clamped files on the volume.
        2. there are opened "disk access" objects, like RFormat or RRawDisk on the volume.
    */
    EForceFormat = 0x800,

	};




enum TSeek
/**
@publishedAll
@released

Flags indicating the destination of a seek operation.

File locations are specified as a 32-bit signed integer,
allowing offsets of ?GB from the origin of the seek.

@see RFile::Seek
*/
	{
	/**
	This can only be used for file systems with execute-in-place facilities,
	such as the ROM file system: the offset specifies the absolute address of
	the data.
	*/
	ESeekAddress,


	/**
	Destination is the start of file.
	*/
	ESeekStart,


	/**
	Destination is the current position in file.
	*/
	ESeekCurrent,


	/**
	Destination is the end of file.
	*/
	ESeekEnd
	};

/**
@publishedAll
@released

*/
class TBlockMapEntry : public TBlockMapEntryBase
	{
public:
	IMPORT_C TBlockMapEntry();
	IMPORT_C void SetNumberOfBlocks( TUint aNumberOfBlocks );
	IMPORT_C void SetStartBlock( TUint aStartBlock );
	};

/**
@publishedAll
@released

*/
typedef TBuf8<KMaxMapsPerCall*sizeof(TBlockMapEntry)> TBlockArrayDes;

struct SBlockMapInfo : public SBlockMapInfoBase
	{
	TBlockArrayDes iMap;
	};

/**
@publishedAll
@released
*/
enum TBlockMapUsage
	{
	/* */
	EBlockMapUsagePaging,
	/* */
	ETestDebug
	};

class TEntry
/**
@publishedAll
@released

Encapsulates an entry in a directory, which can be another (nested) directory,
a file or a volume label.

Each directory entry has a name which is relative to its owning directory
and a type, which is indicated by its unique identifier (UID).

An entry can be interrogated for the following properties:

1. the kind of entry: stored in the entry UIDs, stored in iType

2. the entry attributes, stored in iAtt

3. the size of entry

4. the time the entry was last modified.

@see RDir
@see RFs::Entry
@see RFs::SetEntry
@see CfileBase::CurrentEntry
*/
	{
public:
	IMPORT_C TEntry();
	IMPORT_C TEntry(const TEntry& aEntry);
	IMPORT_C TEntry& operator=(const TEntry& aEntry);
	IMPORT_C TBool IsReadOnly() const;
	IMPORT_C TBool IsHidden() const;
	IMPORT_C TBool IsSystem() const;
	IMPORT_C TBool IsDir() const;
	IMPORT_C TBool IsArchive() const;
	inline const TUid& operator[](TInt anIndex) const;
	inline TBool IsUidPresent(TUid aUid) const;
	inline TBool IsTypeValid() const;
	inline TUid MostDerivedUid() const;
	IMPORT_C TInt64 FileSize() const;
	inline void SetFileSize(TInt64 aFileSize);

private:
	inline void Copy(const TEntry& aEntry);
	inline void Unpack();

public:
    /**
    The individual bits within this byte indicate which attributes
    have been set.

    @see KEntryAttNormal
	@see KEntryAttReadOnly
	@see KEntryAttHidden
    @see KEntryAttSystem
    */
	TUint iAtt;


    /**
    The size of the file in bytes.
    For files larger that 2G it must be cast to TUint in order to avoid looking like negative signed.
    */
	TInt iSize;
	
	
	/**
	The local time of last modification.
	*/
	TTime iModified;
	
	
	/**
	The file's UIDtype
	*/
	TUidType iType;
	
	
	/**
	The name of the file relative to the owning directory,
	with a maximum of KMaxFileName characters.
	
	@see KMaxFileName
	*/
	TBufC<KMaxFileName> iName;
	
private:	
	TUint32 iSizeHigh; 
	/**
	Reserved for future expansion
	*/
	TUint32 iReserved;
	};




class RDir;
class TEntryArray
/**
@publishedAll
@released

Array of directory entries.

It contains the results of a call to RDir::Read(): it will contain all
the TEntry items in the directory. Thus, a directory can be read in
a single call, minimising client/server communication overheads.

@see TEntry
@see RDir::Read
*/
	{
public:
	IMPORT_C TEntryArray();
	IMPORT_C TInt Count() const;
	IMPORT_C const TEntry& operator[](TInt anIndex) const;
private:
	TInt iCount;
	TInt iIndex;
	const TEntry* iPos;
	TBuf8<KEntryArraySize> iBuf;
	friend class RDir;
	friend class RFs;
	friend class TRawEntryArray;
	};




class TDriveInfo
/**
@publishedAll
@released

Contains drive information.

@see RFs::Drive
*/
	{
public:
    /**
    The type of media mounted on the drive.
    */
	TMediaType iType;


    /**
    Indicates whether the drive supports a battery, and if so, its state.
    */
    TBatteryState iBattery;


    /**
    The drive attributes.
    
    @see KDriveAttLocal
    @see KDriveAttRom
    @see KDriveAttRedirected
    @see KDriveAttSubsted
    @see KDriveAttInternal
    @see KDriveAttRemovable 
    @see KDriveAttExternal 
    */
	TUint iDriveAtt;
 
 
    /**
    The attributes of the media mounted on the drive.
    
    @see KMediaAttVariableSize
    @see KMediaAttDualDensity
    @see KMediaAttFormattable
    @see KMediaAttWriteProtected
    @see KMediaAttLockable
    @see KMediaAttLocked
    */
	TUint iMediaAtt;


	/**
	The type of interface used to connect to the media.
	*/
	TConnectionBusType iConnectionBusType;
	};




class TVolumeInfo
/**
@publishedAll
@released

Contains information about a volume mounted on a drive. Use RFs::Drive() if only 
the drive information is required. 

If a drive supports removable media it may contain different volumes over time.

Volume information is made up of information concerning the drive on which it is mounted, 
which can also be accessed through RFs::Drive(), and the volume information, this is made 
up of the size of the volume, the free space, its unique identifying number and a name.

TVolumeInfo is initialised by RFs::Volume().

@see RFs::Volume()
@see RFs::Drive()
*/
	{
public:
	IMPORT_C TVolumeInfo();
	
	/**
	Information about the drive on which the volume is mounted.
	
	@see TDriveInfo
	*/
	TDriveInfo iDrive;


	/**
	The volume’s unique identifying number.
	*/
	TUint iUniqueID;


	/**
	The maximum size of the volume in bytes. The current amount of memory
	in use plus the amount of free memory.
	*/
	TInt64 iSize;


	/**
	The amount of free space on the volume in bytes.
	*/
	TInt64 iFree;


	/**
	Name of the volume, with a maximum of KMaxFileName characters.
	
	This field is optional.

    @see KMaxFileName
	*/
	TBufC<KMaxFileName> iName;
	
	/** 
	Flags which define the default file-caching behaviour for this volume

	@see TFileCacheFlags
	*/
	TFileCacheFlags iFileCacheFlags;
    
    /**
    @prototype
    @internalTechnology
    Internal flag, used in the case of non-blocking getting volume information. 
    @see RFs::Volume(TVolumeInfo& aVol,TInt aDrive, TRequestStatus& aStat)

    If this flag is set, it means that the volume information will be obtained
    asynchronously. More specific, on return iFree will reflect the _current_ amount of free space on volume at the moment of 
    RFs::Volume() call, not the exact final value. This is because in this case getting volume information will be asynchronous, 
    and the client will not be suspended until the mount finish calculating free space. At present appicable to FAT32 file system only.
    */
    TUint8 iVolSizeAsync : 1;

private:	
	/**
	Reserved for future expansion
	*/
    TUint8  i8Reserved1;	
    TUint16 i16Reserved1;	
    TUint32 i32Reserved1;	
    TUint32 i32Reserved2;	
	};




class TDriveUnit
/**
@publishedAll
@released

Drive numbers and letters.

A drive may be represented by either an integer between zero and twenty five
inclusive, or by a buffer descriptor containing a character between "A" and "Z"
inclusive, followed by a colon.
This class encapsulates both representations.
An instance of this class is constructed specifying either the drive number
or the drive letter and may be converted between the two representations.
*/
	{
public:
	inline TDriveUnit() {};
	IMPORT_C TDriveUnit(TInt aDrive);
	IMPORT_C TDriveUnit(const TDesC& aDrive);
	IMPORT_C TDriveUnit& operator=(TInt aDrive);
	IMPORT_C TDriveUnit& operator=(const TDesC& aDrive);
	inline operator TInt() const;
	IMPORT_C TDriveName Name() const;
private:
	TInt iDrive;
	};




class RFs;
//
class TParseBase
/**
@publishedAll
@released

Base class for file name parsing.

You first need to set up the path to be parsed using either a TParse, TParsePtr
or TParsePtrC object.

The interrogation and extraction functions in this class allow you to test
whether a component has been specified in the pathname, and if so,
to extract it. If a component is not present in the pathname,
the extraction function returns an empty string.

This class also allows directories to be added to, and popped from the path.

Notes:

1. the filename modification functions cannot be used by the TParsePtrC class.

2. navigation using .. and . is not supported.

@see TParse
@see TParsePtr
@see TParsePtrC
*/
	{
private:
	struct SField {TUint8 pos;TUint8 len;TUint8 present;TUint8 filler;};
	enum TField {EDrive,EPath,EName,EExt,EMaxFields};
	enum TWild {EWildName=0x01,EWildExt=0x02,EWildEither=0x04,EIsRoot=0x08,EWildIsKMatchOne=0x10,EWildIsKMatchAny=0x20};
public:
	IMPORT_C TParseBase();
	IMPORT_C TInt PopDir();
	IMPORT_C TInt AddDir(const TDesC& aName);
	IMPORT_C const TDesC& FullName() const;
	IMPORT_C TPtrC Drive() const;
	IMPORT_C TPtrC Path() const;
	IMPORT_C TPtrC DriveAndPath() const;
	IMPORT_C TPtrC Name() const;
	IMPORT_C TPtrC Ext() const;
	IMPORT_C TPtrC NameAndExt() const;
	IMPORT_C TBool DrivePresent() const;
	IMPORT_C TBool PathPresent() const;
	IMPORT_C TBool NamePresent() const;
	IMPORT_C TBool ExtPresent() const;
	IMPORT_C TBool NameOrExtPresent() const;
	IMPORT_C TBool IsRoot() const;
	IMPORT_C TBool IsWild() const;
	IMPORT_C TBool IsKMatchOne() const;
	IMPORT_C TBool IsKMatchAny() const;
	IMPORT_C TBool IsNameWild() const;
	IMPORT_C TBool IsExtWild() const;
protected:
	virtual TDes& NameBuf() = 0;                // Reference to derived class descriptor containing the filename.
	virtual const TDesC& NameBufC() const = 0;  // const reference to derived class descriptor containing the filename.
	TInt Set(const TDesC* aName,const TDesC* aRelated,const TDesC* aDefault,TBool allowWild);
private:
	TInt ParseDrive(TLex& aName,TBool& aDone);
	TInt ParsePath(TLex& aName,TBool& aDone);
	TInt ParseName(TLex& aName,TBool& aDone);
	TInt ParseExt(TLex& aName,TBool& aDone);
protected:
	TInt16 iMod;        // Non-zero indicates File name modification functionality is enabled.
private:
	TInt16 iWild;
	SField iField[EMaxFields];
	};




class TParsePtr : public TParseBase
/**
@publishedAll
@released

Parses filenames using less space on the stack than TParse.

Stores a reference to a filename, unlike TParse, which uses
a 512 byte TFileName object as an internal buffer to store
a copy of the filename.
The filename's components (drive, path, etc.) can be retrieved using
the functions provided by the base class, TParseBase.
This class should be used in preference to TParse when minimising stack
usage is a priority.

@see TParse
@see TFileName
*/
	{
public:
	IMPORT_C TParsePtr(TDes& aName);
protected:
	IMPORT_C TDes& NameBuf();
	IMPORT_C const TDesC& NameBufC() const;
private:
	TPtr iNameBuf;
	};




class TParsePtrC : public TParseBase
/**
@publishedAll
@released

Parses, but cannot modify, filenames using less space on the stack than TParse.

Stores a reference to a filename, unlike TParse, which uses
a 512 byte TFileName object as an internal buffer to store
a copy of the filename.
The filename's components (drive, path, etc.) can be retrieved using
the functions provided by the base class, TParseBase. 
Note that because the filename cannot be modified through this class,
the base class functions PopDir() and AddDir() cannot be called, because
a panic will be raised.

@see TParse
@see TFileName
*/
	{
public:
	IMPORT_C TParsePtrC(const TDesC& aName);
protected:
	IMPORT_C TDes& NameBuf();
	IMPORT_C const TDesC& NameBufC() const;
private:
	TPtrC iNameBuf;
	};




class TParse : public TParseBase
/**
@publishedAll
@released

Parses filenames.

The class uses the full filename structure supported by Symbian OS.

TParse works by using the Set() function to set up the filename to be parsed.
Then, various getter functions defined in the base class, TParseBase, such as:
FullName(), Drive(), Path(), DriveAndPath(), Name(), Ext() and NameAndExt()
may be used to retrieve path components.

There are a number of restrictions to valid path components, which are
described in guide documentation.
*/
	{
public:
	IMPORT_C TParse();
	IMPORT_C TInt Set(const TDesC& aName,const TDesC* aRelated,const TDesC* aDefault);
	IMPORT_C TInt SetNoWild(const TDesC& aName,const TDesC* aRelated,const TDesC* aDefault);
protected:
	IMPORT_C TDes& NameBuf();
	IMPORT_C const TDesC& NameBufC() const;
private:
	TFileName iNameBuf;
	};
	
	
	

class CDir : public CBase
/**
@publishedAll
@released

Array of directory entries that has been read into memory from the file system.

It can be read and sorted by user programs, but cannot be created by them. 
*/
	{
public:
	IMPORT_C virtual ~CDir();
	IMPORT_C TInt Count() const;
	IMPORT_C const TEntry& operator[](TInt anIndex) const;
	IMPORT_C TInt Sort(TUint aEntrySortKey);
protected:
	IMPORT_C CDir();
	IMPORT_C static CDir* NewL();
	IMPORT_C void AddL(const TEntry& anEntry);
	IMPORT_C void ExtractL(TBool aRemove,CDir*& aDir);
	IMPORT_C void Compress();
protected:
	CArrayPakFlat<TEntry>* iArray;
	friend class RFs;
	friend class TOpenFileScan;
	};
	
	
#if defined SYMBIAN_PRIVATE_EFSRV
	#define EFSRV_EXPORT_C
	#define EFSRV_IMPORT_C 
#else
	#define EFSRV_EXPORT_C EXPORT_C
	#define EFSRV_IMPORT_C IMPORT_C
#endif
	
// forward declarations from e32ldr_private.h
class RFileClamp;

class RFs : public RSessionBase
/**
@publishedAll
@released

A handle to a file server session.

A program or thread may have arbitrarily many sessions open simultaneously.

Use this class for all file system manipulation, including:

1. adding, removing, moving and renaming files and directories

2. inspecting and changing file attributes and directory entry details.
   These include the time and date when the file or directory was last
   written to, its size and various attribute flags such as read-only,
   hidden, archive or system.                                     

3. finding a file’s real name; if the file system on which it is stored
   has to "mangle" the name into a shorter format

4. getting directory listings

5. maintaining a default path; unlike some other systems, there is a single
   system default path, rather than one for each drive: the default path
   consists of a drive and a path specification.

6. performing context-sensitive parses using TParse objects, and
   the session path

7. obtaining information on drives and volumes

8. formatting and labelling volumes

9. obtaining a list of valid drives

10. emulating the DOS subst command, which allows any directory to appear
    as if it were a separate drive

11. requesting notification of when significant change occurs.
    This can be used for programs which maintain file lists, but must
    update those lists when change occurs.

12. finding the version number of the file server

13. resource counting to ensure that all resources are closed when
    the session terminates.

This class is not intended for user derivation.

The following restrictions apply when a path is specified:

1. its total length must not exceed 256 characters

2. wildcards cannot be used in the drive or in any directory name,
   although they may be allowed in the filename and extension.

3. double backslashes are not allowed in the path. 

4. the following characters must not be included anywhere in the path: < > " / |

5. a colon may only be included between the drive and path

6. no directory name or filename plus extension may consist solely
   of space characters, or of a single or double dot.

7. spaces between the drive, if specified, and the first directory in
   the path are illegal, although there may be spaces between other
   path components, for instance between directories.
*/
	{
public:
	EFSRV_IMPORT_C TInt Connect(TInt aMessageSlots=KFileServerDefaultMessageSlots);
	EFSRV_IMPORT_C void Close();
	EFSRV_IMPORT_C TVersion Version() const;
	EFSRV_IMPORT_C TInt AddFileSystem(const TDesC& aFileName) const;
	EFSRV_IMPORT_C TInt MountFileSystem(const TDesC& aFileSystemName,TInt aDrive) const;
	EFSRV_IMPORT_C TInt MountFileSystem(const TDesC& aFileSystemName,TInt aDrive, TBool aIsSync) const;
	EFSRV_IMPORT_C TInt MountFileSystemAndScan(const TDesC& aFileSystemName,TInt aDrive,TBool& aIsMountSuccess) const;
	EFSRV_IMPORT_C TInt MountFileSystem(const TDesC& aFileSystemName,const TDesC& aExtensionName,TInt aDrive);
	EFSRV_IMPORT_C TInt MountFileSystem(const TDesC& aFileSystemName,const TDesC& aExtensionName,TInt aDrive, TBool aIsSync);
	EFSRV_IMPORT_C TInt MountFileSystemAndScan(const TDesC& aFileSystemName,const TDesC& aExtensionName,TInt aDrive,TBool& aIsMountSuccess) const;
	EFSRV_IMPORT_C TInt DismountFileSystem(const TDesC& aFileSystemName,TInt aDrive) const;
	EFSRV_IMPORT_C TInt RemoveFileSystem(const TDesC& aFileSystemName) const;
	EFSRV_IMPORT_C TInt FileSystemName(TDes& aName,TInt aDrive) const;
	EFSRV_IMPORT_C TInt AddExtension(const TDesC& aFileName);
	EFSRV_IMPORT_C TInt MountExtension(const TDesC& aExtensionName,TInt aDrive);
	EFSRV_IMPORT_C TInt DismountExtension(const TDesC& aExtensionName,TInt aDrive);
	EFSRV_IMPORT_C TInt RemoveExtension(const TDesC& aExtensionName);
	EFSRV_IMPORT_C TInt ExtensionName(TDes& aExtensionName,TInt aDrive,TInt aPos);
	EFSRV_IMPORT_C TInt RemountDrive(TInt aDrive,const TDesC8* aMountInfo=NULL,TUint aFlags=0);
	EFSRV_IMPORT_C void NotifyChange(TNotifyType aType,TRequestStatus& aStat);
	EFSRV_IMPORT_C void NotifyChange(TNotifyType aType,TRequestStatus& aStat,const TDesC& aPathName);
	EFSRV_IMPORT_C void NotifyChangeCancel();
	EFSRV_IMPORT_C void NotifyChangeCancel(TRequestStatus& aStat);
	EFSRV_IMPORT_C void NotifyDiskSpace(TInt64 aThreshold,TInt aDrive,TRequestStatus& aStat);
	EFSRV_IMPORT_C void NotifyDiskSpaceCancel(TRequestStatus& aStat);
	EFSRV_IMPORT_C void NotifyDiskSpaceCancel();
	EFSRV_IMPORT_C TInt DriveList(TDriveList& aList) const;
	EFSRV_IMPORT_C TInt DriveList(TDriveList& aList, TUint aFlags) const;
	EFSRV_IMPORT_C TInt Drive(TDriveInfo& anInfo,TInt aDrive=KDefaultDrive) const;
    EFSRV_IMPORT_C TInt Volume(TVolumeInfo& aVol,TInt aDrive=KDefaultDrive) const;
    EFSRV_IMPORT_C void Volume(TVolumeInfo& aVol,TInt aDrive, TRequestStatus& aStat) const;
	EFSRV_IMPORT_C TInt SetVolumeLabel(const TDesC& aName,TInt aDrive=KDefaultDrive);
	EFSRV_IMPORT_C TInt Subst(TDes& aPath,TInt aDrive=KDefaultDrive) const;
	EFSRV_IMPORT_C TInt SetSubst(const TDesC& aPath,TInt aDrive=KDefaultDrive);
	EFSRV_IMPORT_C TInt RealName(const TDesC& aName,TDes& aResult) const;
    EFSRV_IMPORT_C TInt GetMediaSerialNumber(TMediaSerialNumber& aSerialNum, TInt aDrive);
	EFSRV_IMPORT_C TInt SessionPath(TDes& aPath) const;
	EFSRV_IMPORT_C TInt SetSessionPath(const TDesC& aPath);
	EFSRV_IMPORT_C TInt Parse(const TDesC& aName,TParse& aParse) const;
	EFSRV_IMPORT_C TInt Parse(const TDesC& aName,const TDesC& aRelated,TParse& aParse) const;
	EFSRV_IMPORT_C TInt MkDir(const TDesC& aPath);
	EFSRV_IMPORT_C TInt MkDirAll(const TDesC& aPath);
	EFSRV_IMPORT_C TInt RmDir(const TDesC& aPath);
	EFSRV_IMPORT_C TInt GetDir(const TDesC& aName,TUint anEntryAttMask,TUint anEntrySortKey,CDir*& anEntryList) const;
	EFSRV_IMPORT_C TInt GetDir(const TDesC& aName,TUint anEntryAttMask,TUint anEntrySortKey,CDir*& anEntryList,CDir*& aDirList) const;
	EFSRV_IMPORT_C TInt GetDir(const TDesC& aName,const TUidType& anEntryUid,TUint anEntrySortKey,CDir*& aFileList) const;
	EFSRV_IMPORT_C TInt Delete(const TDesC& aName);
	EFSRV_IMPORT_C TInt Rename(const TDesC& anOldName,const TDesC& aNewName);
	EFSRV_IMPORT_C TInt Replace(const TDesC& anOldName,const TDesC& aNewName);
	EFSRV_IMPORT_C TInt Att(const TDesC& aName,TUint& aAttValue) const;
	EFSRV_IMPORT_C TInt SetAtt(const TDesC& aName,TUint aSetAttMask,TUint aClearAttMask);
	EFSRV_IMPORT_C TInt Modified(const TDesC& aName,TTime& aTime) const;
	EFSRV_IMPORT_C TInt SetModified(const TDesC& aName,const TTime& aTime);
	EFSRV_IMPORT_C TInt Entry(const TDesC& aName,TEntry& anEntry) const;
	EFSRV_IMPORT_C TInt SetEntry(const TDesC& aName,const TTime& aTime,TUint aSetAttMask,TUint aClearAttMask);
private:
	EFSRV_IMPORT_C TInt ReadFileSection_RESERVED(const TDesC& aName,TInt aPos,TDes8& aDes,TInt aLength) const;
public:
	EFSRV_IMPORT_C static TBool IsValidDrive(TInt aDrive);
	EFSRV_IMPORT_C static TInt CharToDrive(TChar aChar,TInt& aDrive);
	EFSRV_IMPORT_C static TInt DriveToChar(TInt aDrive,TChar& aChar);
	EFSRV_IMPORT_C static TBool IsRomAddress(TAny* aAny);
	EFSRV_IMPORT_C static TDriveNumber GetSystemDrive();
	EFSRV_IMPORT_C static TChar GetSystemDriveChar();
	EFSRV_IMPORT_C TInt SetSystemDrive(TDriveNumber aSystemDrive);
	EFSRV_IMPORT_C void ResourceCountMarkStart() const;
	EFSRV_IMPORT_C void ResourceCountMarkEnd() const;
	EFSRV_IMPORT_C TInt ResourceCount() const;
	EFSRV_IMPORT_C TInt IsFileOpen(const TDesC& aFile,TBool& anAnswer) const;
	EFSRV_IMPORT_C TInt CheckDisk(const TDesC& aDrive) const;
	EFSRV_IMPORT_C TInt ScanDrive(const TDesC& aDrive) const;
	EFSRV_IMPORT_C TInt GetShortName(const TDesC& aLongName,TDes& aShortName) const;
	EFSRV_IMPORT_C TInt GetLongName(const TDesC& aShortName,TDes& aLongName) const;
	EFSRV_IMPORT_C TBool GetNotifyUser();
	EFSRV_IMPORT_C void SetNotifyUser(TBool aValue);
	EFSRV_IMPORT_C TUint8* IsFileInRom(const TDesC& aFileName) const;
	EFSRV_IMPORT_C TBool IsValidName(const TDesC& anEntryName) const;
	EFSRV_IMPORT_C TBool IsValidName(const TDesC& aFileName,TText& aBadChar) const;
	EFSRV_IMPORT_C TInt GetDriveName(TInt aDrive,TDes& aDriveName) const;
	EFSRV_IMPORT_C TInt SetDriveName(TInt aDrive,const TDesC& aDriveName);
	EFSRV_IMPORT_C TInt LoaderHeapFunction(TInt aFunction, TAny *aArg1=NULL, TAny *aArg2=NULL);
	IMPORT_C TInt SetErrorCondition(TInt anError,TInt aCount=0);
	EFSRV_IMPORT_C TInt SetDebugRegister(TInt aVal);
	EFSRV_IMPORT_C TInt SetAllocFailure(TInt aAllocNum);
	EFSRV_IMPORT_C void DebugNotify(TInt aDrive,TUint aNotifyType,TRequestStatus& aStat);
	EFSRV_IMPORT_C TInt ControlIo(TInt aDrive,TInt aCommand);
	EFSRV_IMPORT_C TInt ControlIo(TInt aDrive,TInt aCommand,TDes8& aParam1);
	EFSRV_IMPORT_C TInt ControlIo(TInt aDrive,TInt aCommand,TDes8& aParam1,TDes8& aParam2);
	EFSRV_IMPORT_C TInt ControlIo(TInt aDrive,TInt aCommand,TAny* aParam1,TAny* aParam2);
	EFSRV_IMPORT_C TInt LockDrive(TInt aDrv, const TMediaPassword &aOld, const TMediaPassword &aNew, TBool aStr);
	EFSRV_IMPORT_C TInt UnlockDrive(TInt aDrv, const TMediaPassword &Pswd, TBool aStr);
	EFSRV_IMPORT_C TInt ClearPassword(TInt aDrv, const TMediaPassword &aPswd);
	EFSRV_IMPORT_C TInt ErasePassword(TInt aDrv);
	EFSRV_IMPORT_C TInt SetSessionToPrivate(TInt aDrive);
	EFSRV_IMPORT_C TInt PrivatePath(TDes& aPath);
	EFSRV_IMPORT_C TInt CreatePrivatePath(TInt aDrive);	
	EFSRV_IMPORT_C void StartupInitComplete(TRequestStatus& aStat);
	EFSRV_IMPORT_C TInt SetLocalDriveMapping(const TDesC8& aMapping);

	EFSRV_IMPORT_C TInt FinaliseDrives();
    
    /** specifies drive finalisation modes */
    enum TFinaliseDrvMode
        {
        EFinal_RW,      ///< after successful finalisation the drive remains writable and will become "not finalised" after the first write operation.
        EFinal_RO,      ///< after successful finalisation the drive becomes read-only
        EForceUnfinalise///< @internalComponent  mark the drive as "not finalised" can result in KErrAbort if the dive is in inconsistent state.
        };

    EFSRV_IMPORT_C TInt FinaliseDrive(TInt aDriveNo, TFinaliseDrvMode aMode) const;

	EFSRV_IMPORT_C TInt SwapFileSystem(const TDesC& aOldFileSystemName,const TDesC& aNewFileSystemName,TInt aDrive) const;
	EFSRV_IMPORT_C TInt ReserveDriveSpace(TInt aDriveNo, TInt aSpace);
	EFSRV_IMPORT_C TInt GetReserveAccess(TInt aDriveNo);
	EFSRV_IMPORT_C TInt ReleaseReserveAccess(TInt aDriveNo);

	EFSRV_IMPORT_C TInt AddPlugin(const TDesC& aFileName) const;
	EFSRV_IMPORT_C TInt RemovePlugin(const TDesC& aPluginName) const;
	EFSRV_IMPORT_C TInt PluginName(TDes& aPluginName,TInt aDrive,TInt aPos);

	EFSRV_IMPORT_C TInt MountPlugin(const TDesC& aPluginName) const;
	EFSRV_IMPORT_C TInt MountPlugin(const TDesC& aPluginName,TInt aDrive) const;
	EFSRV_IMPORT_C TInt MountPlugin(const TDesC& aPluginName,TInt aDrive, TInt aPos) const;
	
	EFSRV_IMPORT_C TInt DismountPlugin(const TDesC& aPluginName) const;
	EFSRV_IMPORT_C TInt DismountPlugin(const TDesC& aPluginName,TInt aDrive) const;
	EFSRV_IMPORT_C TInt DismountPlugin(const TDesC& aPluginName,TInt aDrive,TInt aPos) const;

	EFSRV_IMPORT_C void NotifyDismount(TInt aDrive, TRequestStatus& aStat, TNotifyDismountMode aMode=EFsDismountRegisterClient) const;
	EFSRV_IMPORT_C void NotifyDismountCancel(TRequestStatus& aStat) const;
	EFSRV_IMPORT_C void NotifyDismountCancel() const;
	EFSRV_IMPORT_C TInt AllowDismount(TInt aDrive) const;
    EFSRV_IMPORT_C TInt SetStartupConfiguration(TInt aCommand,TAny* aParam1,TAny* aParam2) const;
	EFSRV_IMPORT_C TInt AddCompositeMount(const TDesC& aFileSystemName,TInt aLocalDriveToMount,TInt aCompositeDrive, TBool aSync) const;
	EFSRV_IMPORT_C TInt SetNotifyChange(TBool aNotifyChange);
	EFSRV_IMPORT_C TInt QueryVolumeInfoExt(TInt aDrive, TQueryVolumeInfoExtCmd aCommand, TDes8& aInfo) const;
	EFSRV_IMPORT_C TInt VolumeIOParam(TInt aDriveNo, TVolumeIOParamInfo& aParamInfo) const;
	EFSRV_IMPORT_C TInt FileSystemSubType(TInt aDriveNo, TDes& aName) const;
	EFSRV_IMPORT_C TInt InitialisePropertiesFile(const TPtrC8& aPtr) const;
	
	IMPORT_C TInt AddProxyDrive(const TDesC& aFileName);
	IMPORT_C TInt RemoveProxyDrive(const TDesC& aDriveName);
	
	template <class T0,class T1> inline TInt MountProxyDrive(const TUint aDrive, const TDesC& aName, T0 a0, T1 a1)
		{ return(DoMountProxyDrive(TIpcArgs(aDrive, &aName, a0, a1))); };
	IMPORT_C TInt DismountProxyDrive(const TUint aDrive);
	
	TInt Unclamp(const RFileClamp& aHandle);
	
	EFSRV_IMPORT_C TInt ReadFileSection(const TDesC& aName,TInt64 aPos,TDes8& aDes,TInt aLength) const;
	
    /**
	This class is used to for returning meaningful error code values to users of RFs::IsValidName(const TDesC& ,TNameValidParam& ) 
	@see TError
	*/
	class TNameValidParam
		{
		public:
			/** Initialises the members of the class. By default iUseSessionPath is set to EFalse, however one could set it to ETrue.*/
  			inline TNameValidParam(TBool aUseSessionPath = EFalse);
  				
 		/** possible error codes */
 		enum TError
 			{
			ErrNone,            ///< no error.
 			ErrBadCharacter,    ///< aName contains a bad character; and its position is in iInvalidCharPos.
 			ErrBadName,         ///< aName isn't a valid file or directory name.
 			ErrNameTooLong      ///< aName length or aName + session path length (see iUseSessionPath) is longer than 256 characters.
 			};
		
		inline TError ErrorCode() const;
 		inline void   UseSessionPath(TBool aUseSessionPath);
 		inline TUint  InvalidCharPos() const;
 		friend class TFsIsValidName;
		private:
 			TError iError;          ///< the reason why aName is invalid, see TError
 			TBool  iUseSessionPath; ///< if ETrue, and if aName isn't fully specified, missing parts will be taken from the session path
 			TUint  iInvalidCharPos; ///< may contain invalid character position if error is ErrBadCharacter,else 0.
 		};
	EFSRV_IMPORT_C TBool IsValidName(const TDesC& aName, TNameValidParam& aParam );

    /** Special enumerator values for the SupportedFileSystemName() API */
    enum 
        {
        KRootFileSystem  = 0x00800000,  ///< specifies "root" file system. The result will be the same as for FileSystemName() API call
        KFirstChildFileSystem = 0       ///< specifies the first child file system number, the second will be KFirstChildFileSystem+1 etc.
        };

    EFSRV_IMPORT_C TInt SupportedFileSystemName(TDes& aName, TInt aDrive, TInt aFsEnumerator) const;

protected:
	TInt SendReceive(TInt aFunction,const TIpcArgs& aArgs) const;

private:
	void GetDirL(const TDesC& aMatchName,TUint anEntryAttMask,TUint anEntrySortKey,CDir*& anEntryList,CDir*& aDirList,RDir& aDir) const;
	void GetDirL(const TDesC& aMatchName,TUint anEntryAttMask,TUint anEntrySortKey,CDir*& anEntryList,RDir& aDir) const;
	void GetDirL(const TDesC& aMatchName,const TUidType& aUidType,TUint anEntrySortKey,CDir*& anEntryList,RDir& aDir) const;
	void DoGetDirL(TUint anEntrySortKey,CDir*& anEntryList,RDir& aDir) const;
	TInt GetOpenFileList(TInt& aSessionNum,TInt& aLocalPos,TThreadId& aThreadId,TEntryArray& anArray) const;
	
	IMPORT_C TInt DoMountProxyDrive(const TIpcArgs& ipcArgs);
	
	friend class TOpenFileScan;
	friend class RFsPlugin;
	};


//-------------------------------------------------------------------------------------------------------------------

/** 

    Base class for volume formatting parameters. This class package buffer, TVolFormatParamBuf or
    packaged buffer of the derived class can be passed to the RFormat::Open() in order to provide
    file system-specific formatting parameters.
    Each file system that supports such formatting parameters shall have this class specialisation (e.g. TVolFormatParam_FAT)
    All classes, derived from this one must have the same size as the base class.  
    In order to use formatting parameters the format mode shall have ESpecialFormat bit flag set.


    @see    RFormat::Open(RFs&,const TDesC&,TUint,TInt& ,const TDesC8& anInfo);
    @see    TVolFormatParamBuf

    @publishedAll
    @released
*/ 
class TVolFormatParam
    {
public:
    inline TVolFormatParam();
    inline void Init();
    
    inline void SetFileSystemName(const TDesC& aFsName);

    static inline TUint32 CalcFSNameHash(const TDesC& aFsName);
    inline TUint32 FSNameHash() const;

    inline TBool SomeParamsSet() const;


protected:
   
    
    enum {KMaxDataSlots = 64}; ///< the size of data array iData

    inline void SetVal(TUint aIndex, TUint32 aVal);
    inline TUint32 GetVal(TUint aIndex) const;


public:
    enum {KUId = 0x820116A2}; ///< this value shell be in iUid field to identify this class object

    /** 
    This class tree UID. Used to distinguish the object of this class from TLDFormatInfo and other possible data structures.
    For this and derived classes this field must be KUId. This field offset in the class must be 0
    */
    const TUint32 iUId;    
                            
private:
    TUint32 iFSysNameHash;          ///< Up-cased file system name hash (crc32) used to designate the file system. 0 means "not set"
    TBool   iParamsSet : 1;         ///< ETrue if any parameter was set (SetVal() called). Gets reset to EFalse by Init()    
    TUint32 iData[KMaxDataSlots];   ///< used as a pool for various data. The derived classes are free to use it by SetVal()/GetVal()
    }; 

__ASSERT_COMPILE(_FOFF(TVolFormatParam, iUId) == 0);


/** package buffer for the objects of class TVolFormatParamBuf */
typedef TPckgBuf<TVolFormatParam> TVolFormatParamBuf;




#ifndef SYMBIAN_ENABLE_64_BIT_FILE_SERVER_API
/**
@publishedAll
@released

Creates and opens a file, and performs all operations on a single open file.

These include:

- reading from and writing to the file

- seeking to a position within the file

- locking and unlocking within the file

- setting file attributes

Before using any of these services, a connection to a file server session must
have been made, and the file must be open.

Opening Files:

-  use Open() to open an existing file for reading or writing; an error is
   returned if it does not already exist.
   To open an existing file for reading only, use Open() with an access mode of
   EFileRead, and a share mode of EFileShareReadersOnly.

-  use Create() to create and open a new file for writing; an error is returned
   if it already exists.

-  use Replace() to open a file for writing, replacing any existing file of
   the same name if one exists, or creating a new file if one does not exist.
   Note that if a file exists, its length is reset to zero.

-  use Temp() to create and open a temporary file with a unique name,
   for writing and reading.

When opening a file, you must specify the file server session to use for
operations with that file. If you do not close the file explicitly, it is
closed when the server session associated with it is closed.

Reading and Writing:

There are several variants of both Read() and Write().
The basic Read(TDes8& aDes) and Write(const TDesC8& aDes) are supplemented
by variants allowing the descriptor length to be overridden, or the seek
position of the first byte to be specified, or asynchronous completion,
or any combination.

Reading transfers data from a file to a descriptor, and writing transfers
data from a descriptor to a file. In all cases, the file data is treated
as binary and byte descriptors are used (TDes8, TDesC8).

@see TDes8
@see TDesC8
*/
#else
/**
@publishedAll
@released

Creates and opens a file, and performs all operations on a single open file.

These include:

- reading from and writing to the file

- seeking to a position within the file

- locking and unlocking within the file

- setting file attributes

Before using any of these services, a connection to a file server session must
have been made, and the file must be open.

Opening Files:

-  use Open() to open an existing file for reading or writing; an error is
   returned if it does not already exist.
   To open an existing file for reading only, use Open() with an access mode of
   EFileRead, and a share mode of EFileShareReadersOnly.

-  use Create() to create and open a new file for writing; an error is returned
   if it already exists.

-  use Replace() to open a file for writing, replacing any existing file of
   the same name if one exists, or creating a new file if one does not exist.
   Note that if a file exists, its length is reset to zero.

-  use Temp() to create and open a temporary file with a unique name,
   for writing and reading.

When opening a file, you must specify the file server session to use for
operations with that file. If you do not close the file explicitly, it is
closed when the server session associated with it is closed.

Reading and Writing:

There are several variants of both Read() and Write().
The basic Read(TDes8& aDes) and Write(const TDesC8& aDes) are supplemented
by variants allowing the descriptor length to be overridden, or the seek
position of the first byte to be specified, or asynchronous completion,
or any combination.

Reading transfers data from a file to a descriptor, and writing transfers
data from a descriptor to a file. In all cases, the file data is treated
as binary and byte descriptors are used (TDes8, TDesC8).

RFile class supports operations on files of size less than or equal to 2GB - 1.
If the file size is greater than 2GB - 1 (large file), use of class RFile64 is 
recommended for following operations:
	1. Opening a large file
	2. Creating a file which can grow beyond 2GB - 1 by size
	3. Creating a temporary file which can grow beyond 2GB - 1 by size
	4. Replacing an existing file after which it can grow beyond 2GB - 1 by size
	5. Adopting a large file handle from client
	6. Adopting a large file handle from server
	7. Adopting a large file handle from another process
	8. Read from a position greater than 2GB - 1
	9. Writing to a file by which the size can grow beyond 2GB - 1
	10. Seek to a position greater than 2GB - 1
	11. Setting a size greater than 2GB - 1
	12. Querying the file size (greater than 2GB - 1)
	13. Locking and unlocking a position and length that is beyond 2GB - 1

@see TDes8
@see TDesC8

@see RFile64

*/

#endif


class RFile : public RSubSessionBase
	{
public:
	EFSRV_IMPORT_C TInt Open(RFs& aFs,const TDesC& aName,TUint aFileMode);
	EFSRV_IMPORT_C void Close();
	EFSRV_IMPORT_C TInt Create(RFs& aFs,const TDesC& aName,TUint aFileMode);
	EFSRV_IMPORT_C TInt Replace(RFs& aFs,const TDesC& aName,TUint aFileMode);
	EFSRV_IMPORT_C TInt Temp(RFs& aFs,const TDesC& aPath,TFileName& aName,TUint aFileMode);
	EFSRV_IMPORT_C TInt Read(TDes8& aDes) const;
	EFSRV_IMPORT_C void Read(TDes8& aDes,TRequestStatus& aStatus) const;
	EFSRV_IMPORT_C TInt Read(TDes8& aDes,TInt aLength) const;
	EFSRV_IMPORT_C void Read(TDes8& aDes,TInt aLength,TRequestStatus& aStatus) const;
	EFSRV_IMPORT_C TInt Read(TInt aPos,TDes8& aDes) const;
	EFSRV_IMPORT_C void Read(TInt aPos,TDes8& aDes,TRequestStatus& aStatus) const;
	EFSRV_IMPORT_C TInt Read(TInt aPos,TDes8& aDes,TInt aLength) const;
	EFSRV_IMPORT_C void Read(TInt aPos,TDes8& aDes,TInt aLength,TRequestStatus& aStatus) const;
	EFSRV_IMPORT_C void ReadCancel(TRequestStatus& aStatus) const;
	EFSRV_IMPORT_C void ReadCancel() const;
	EFSRV_IMPORT_C TInt Write(const TDesC8& aDes);
	EFSRV_IMPORT_C void Write(const TDesC8& aDes,TRequestStatus& aStatus);
	EFSRV_IMPORT_C TInt Write(const TDesC8& aDes,TInt aLength);
	EFSRV_IMPORT_C void Write(const TDesC8& aDes,TInt aLength,TRequestStatus& aStatus);
	EFSRV_IMPORT_C TInt Write(TInt aPos,const TDesC8& aDes);
	EFSRV_IMPORT_C void Write(TInt aPos,const TDesC8& aDes,TRequestStatus& aStatus);
	EFSRV_IMPORT_C TInt Write(TInt aPos,const TDesC8& aDes,TInt aLength);
	EFSRV_IMPORT_C void Write(TInt aPos,const TDesC8& aDes,TInt aLength,TRequestStatus& aStatus);
	EFSRV_IMPORT_C TInt Lock(TInt aPos,TInt aLength) const;
	EFSRV_IMPORT_C TInt UnLock(TInt aPos,TInt aLength) const;
	EFSRV_IMPORT_C TInt Seek(TSeek aMode,TInt& aPos) const;
	EFSRV_IMPORT_C TInt Flush();
	EFSRV_IMPORT_C void Flush(TRequestStatus& aStatus);
	EFSRV_IMPORT_C TInt Size(TInt& aSize) const;
	EFSRV_IMPORT_C TInt SetSize(TInt aSize);
	EFSRV_IMPORT_C TInt Att(TUint& aAttValue) const;
	EFSRV_IMPORT_C TInt SetAtt(TUint aSetAttMask,TUint aClearAttMask);
	EFSRV_IMPORT_C TInt Modified(TTime& aTime) const;
	EFSRV_IMPORT_C TInt SetModified(const TTime& aTime);
	EFSRV_IMPORT_C TInt Set(const TTime& aTime,TUint aSetAttMask,TUint aClearAttMask);
	EFSRV_IMPORT_C TInt ChangeMode(TFileMode aNewMode);
	EFSRV_IMPORT_C TInt Rename(const TDesC& aNewName);
	EFSRV_IMPORT_C TInt Drive(TInt &aDriveNumber, TDriveInfo &aDriveInfo) const;
	EFSRV_IMPORT_C TInt Adopt(RFs& aFs, TInt aHandle);
	EFSRV_IMPORT_C TInt AdoptFromClient(const RMessage2& aMsg, TInt aFsHandleIndex, TInt aFileHandleIndex);
	EFSRV_IMPORT_C TInt AdoptFromServer(TInt aFsHandle, TInt aFileHandle);
	EFSRV_IMPORT_C TInt AdoptFromCreator(TInt aFsIndex, TInt aFileHandleIndex);
	EFSRV_IMPORT_C TInt Name(TDes& aName) const;
	EFSRV_IMPORT_C TInt TransferToServer(TIpcArgs& aIpcArgs, TInt aFsHandleIndex, TInt aFileHandleIndex) const;
	EFSRV_IMPORT_C TInt TransferToClient(const RMessage2& aMsg, TInt aFileHandleIndex) const;
	EFSRV_IMPORT_C TInt TransferToProcess(RProcess& aProcess, TInt aFsHandleIndex, TInt aFileHandleIndex) const;
	EFSRV_IMPORT_C TInt Duplicate(const RFile& aFile, TOwnerType aType=EOwnerProcess);
	EFSRV_IMPORT_C TInt FullName(TDes& aName) const;
	EFSRV_IMPORT_C TInt BlockMap(SBlockMapInfo& aInfo, TInt64& aStartPos, TInt64 aEndPos=-1, TInt aBlockMapusage=EBlockMapUsagePaging) const;
	TInt Clamp(RFileClamp& aHandle);

protected:
	// RSubSessionBase overrides
	TInt CreateSubSession(const RSessionBase& aSession,TInt aFunction,const TIpcArgs& aArgs);
	void CloseSubSession(TInt aFunction);
	TInt SendReceive(TInt aFunction,const TIpcArgs& aArgs) const;

	TInt DuplicateHandle(TInt& aSubSessionHandle) const;

	friend class RFilePlugin;
	};

#ifdef SYMBIAN_ENABLE_64_BIT_FILE_SERVER_API
#include <f32file64.h>
#endif

class RDir : public RSubSessionBase
/**
@publishedAll
@released

Reads the entries contained in a directory.

You must first open the directory, specifying an attribute mask which is used
by Read() calls to filter the entry types required. Then, use one of
the Read() functions to read the filtered entries. When the operation
is complete, the directory should be closed using Close()

There are two types of Read(): one works with a single entry at a time,
requiring programs to iterate through the entries explicitly.
The other works with an entire TEntryArray, allowing multiple entries to be
read in one call.
As well as making application program logic somewhat simpler, this type
uses fewer calls to the server, and is more efficient.

Each type of Read() can be performed either synchronously or asynchronously.

It may be more convenient to use RFs::GetDir() than the Read() calls supported
by this class.
RFs::GetDir() has the advantage that it allows a directory’s entries to be
sorted in various ways.
However, it does not provide asynchronous as well as synchronous variants
and does not allow entries to be read individually.

@see RFs
*/
	{
public:
	EFSRV_IMPORT_C TInt Open(RFs& aFs,const TDesC& aMatchName,const TUidType& aUidType);
	EFSRV_IMPORT_C TInt Open(RFs& aFs,const TDesC& aMatchName,TUint anAttMask);
	EFSRV_IMPORT_C void Close();
	EFSRV_IMPORT_C TInt Read(TEntryArray& anArray) const;
	EFSRV_IMPORT_C void Read(TEntryArray& anArray,TRequestStatus& aStatus) const;
	EFSRV_IMPORT_C TInt Read(TEntry& anEntry) const;
	EFSRV_IMPORT_C void Read(TPckg<TEntry>& anEntry,TRequestStatus& aStatus) const;

private:
	// RSubSessionBase overrides
	TInt SendReceive(TInt aFunction,const TIpcArgs& aArgs) const;

	friend class RDirPlugin;
	};


class RFormat : public RSubSessionBase
/**
@publishedAll
@released

Formats a device, one step at a time. 

RFormat must first be opened on a device before formatting each
track using Next().

There is also an asynchronous version of Next() which, if encapsulated into
a suitable active object, can be used to implement a
user-interruptible formatting process.
*/
	{
public:
	IMPORT_C TInt Open(RFs& aFs,const TDesC& aName,TUint aFormatMode,TInt& aCount);
	IMPORT_C TInt Open(RFs& aFs,const TDesC& aName,TUint aFormatMode,TInt& aCount,const TDesC8& anInfo);
	IMPORT_C void Close();
	IMPORT_C TInt Next(TInt& aStep);
	IMPORT_C void Next(TPckgBuf<TInt>& aStep,TRequestStatus& aStatus);
	};




class RRawDisk : public RSubSessionBase
/**
@publishedAll
@released

Enables direct disk access.

No other resources can access the disk while direct access to it is in effect.

This class is not intended for user derivation.
*/
	{
public:
	IMPORT_C TInt Open(RFs& aFs,TInt aDrive);
	IMPORT_C void Close();
	IMPORT_C TInt Read(TInt64 aPos,TDes8& aDes);
	IMPORT_C TInt Write(TInt64 aPos,TDesC8& aDes);
private:
	TInt iDrive;
	};




class CDirStack;
NONSHARABLE_CLASS(CDirScan) : public CBase
/**
@publishedAll
@released

Scans a directory structure.

The scan moves from directory to directory through the hierarchy, returning
a list of the entries contained in each. The order in which the directories
are scanned is determined by a sort key which is specified when setting up
the scan. The base directory to be scanned and the entry types of interest
must also be specified before performing the scan.

This class is not intended for user derivation
*/
    {
public:
    /**
    Defines the scan direction.
    */
	enum TScanDirection
		{
		/**
		Scan upwards from the lowest level directory in the hierarchy to
		the top level directory.
		*/
		EScanUpTree,
		
		/**
		Scan downwards from the top level directory in the hierarchy to
		the bottom level directory.
		*/
		EScanDownTree
		};
public:
	IMPORT_C static CDirScan* NewL(RFs& aFs);
	IMPORT_C static CDirScan* NewLC(RFs& aFs);
	IMPORT_C ~CDirScan();
	IMPORT_C void SetScanDataL(const TDesC& aMatchName,TUint anEntryAttMask,TUint anEntrySortMask,TScanDirection aScanDir=EScanDownTree);
	IMPORT_C void NextL(CDir*& aDirEntries);
	IMPORT_C TPtrC AbbreviatedPath();
	IMPORT_C TPtrC FullPath();
protected:
	CDirScan(RFs& aFs);
private:
	inline RFs& Fs();
	void UpdateAbbreviatedPath();
	void ScanUpTreeL(CDir*& aDirEntries);
	void ScanDownTreeL(CDir*& aDirEntries);
	void GetDirEntriesL(CDir*& aDirEntries);
private:
	RFs* const iFs;
	TParse iFullPath;
	TPtrC iAbbreviatedPath;
	TInt iAbbreviatedPathPos;
	TUint iEntryAttMask;
	TUint iEntrySortMask;
	TBool iScanning;
	TScanDirection iScanDir;
	CDirStack* iStack;
	};




enum TFileManError
/**
@publishedAll
@released

A list of CFileMan error codes.

@see CFileMan
*/
	{
	/**
	No additional error information is available, either because
	the latest CFileMan operation did not return an error, or if it did,
	the error was not one for which additional information is available.
	*/
	ENoExtraInformation,


	/**
	A leave occurred while setting up the initial scan.
	
	This indicates that the operation did not begin.
	
	@see CDirScan.
    */
	EInitializationFailed,


	/**
	A leave occurred while scanning the next directory in the course of a file
	management function.
	
	This indicates that the operation did begin.
	
	@see CDirScan.
	*/
	EScanNextDirectoryFailed,


	/**
	Error occurred when attempting to open the source file for a file copy
	or move.
	*/
	ESrcOpenFailed,


	/**
	Error occurred while attempting to create, or, if overwriting is in effect,
	replace the target file for a file copy or move.
	*/
	ETrgOpenFailed,


	/**
	The operation completed without processing any files because no matching
	files were found.
	*/
	ENoFilesProcessed
	};




class MFileManObserver
/**
@publishedAll
@released

Provides notification of the progress of synchronous or asynchronous
file management operations.

It should be inherited by classes which implement this protocol.

The enquiry functions provided by CFileBase and CFileMan may be used by
the observer to display information about the progress of the operation
such as error messages, the names of the target and destination files,
and the number of bytes transferred during a copy operation.
Notification may take place before or after an entry has been processed,
or during a file copy or move.
Each notification function returns a value which can be used to enable
the user to control the progress of the operation, for example to cancel
a long-running multiple file copy.
To use this class, pass a pointer to an instance of the class to
the CFileMan constructor, or use SetObserver(), defined in CFileBase.
*/
	{
public:
    /**
    Control for the current CFileMan operation.
    */
	enum TControl
		{
		/**
		Proceed with the current or the next entry.
		*/
		EContinue,


		/**
		Retry processing the previous entry.
		*/
		ERetry,


		/**
		Abort operation, causes function to return KErrCancel.
		*/
		EAbort,
		

		/**
		Cancel processing the current entry.
		*/
		ECancel
		};
public:
	IMPORT_C virtual TControl NotifyFileManStarted();
	IMPORT_C virtual TControl NotifyFileManOperation();
	IMPORT_C virtual TControl NotifyFileManEnded();
	};




class CFileBase : public CBase
/**
@publishedAll
@released

Abstract base class for file management.

It provides functions to set an observer for the derived class
CFileMan, and to get information about the entry being processed.

@see CFileMan
*/
	{
public:
	IMPORT_C void SetObserver(MFileManObserver* anObserver);
public:
	IMPORT_C const TEntry& CurrentEntry();
	IMPORT_C TPtrC AbbreviatedPath();
	IMPORT_C TPtrC FullPath();
	IMPORT_C TInt GetLastError();
	IMPORT_C TFileManError GetMoreInfoAboutError();
protected:
	IMPORT_C CFileBase(RFs& anFs);
	IMPORT_C void ConstructL();
	IMPORT_C ~CFileBase();
	IMPORT_C void RunL();
	IMPORT_C void RunInSeparateThreadL(TThreadFunction aThreadFunction);
// virtual
    /**
    Called from RunL to perform tidy up after an operation.
    
    @see CFileMan
    @see CFileBase::RunL
    */
	virtual void CompleteOperationL() {};
	
//pure virtual
	/**
	Called from RunL to perform the requested operation.
	
	@see CFileMan
	@see CFileBase::RunL
	*/
	virtual void DoOperationL() = 0;
protected:
	RFs iFs;
	RFs iFsOld;
	RThread iFManThread;
	RSemaphore iSynchronizer;
	CDirScan* iScanner;
	CDir* iDirList;
	TInt iCurrentEntry;
	TUint iMatchEntry;
	TUint iSwitches;
	TParse iSrcFile;
	MFileManObserver* iObserver;
	TInt iLastError;
	TFileManError iErrorInfo;
	TRequestStatus* iStatus;
	HBufC* iSessionPath;
	TInt iNumberOfFilesProcessed;




friend void DoFManBaseOperationL(TAny* aPtr);
friend TInt FManBaseThreadFunction(TAny* aPtr);
	};




NONSHARABLE_CLASS(CFileMan) : public CFileBase
/**
@publishedAll
@released

Offers file management services which accept the use of wildcards;
synchronous and asynchronous.

It also provides enquiry functions, which, like those provided by
the base class CFileBase, may be used by an observer class object
to provide the user with information about the progress of the operation.

All of the file management functions provided by this class accept the use of
wildcards, and may operate either synchronously or asynchronously.
When CFileMan is operating asynchronously, the operation takes place in
a separate thread from the calling code.

A file notification observer (an instance of a class deriving
from MFileManObserver) may optionally be used by CFileMan when operating
synchronously or asynchronously. If provided, the appropriate notification
function is called before or after each entry has been processed,
or during a file copy or move.
This notification can be used to provide information about the state of
the operation, such as the number of bytes transferred during a
large-scale file copy. It can also be used to allow the user to cancel,
retry or continue processing an entry, or to abort the whole operation.
If such notification is required, specify an object deriving from
MFileManObserver class in the constructor, or call SetObserver(),
defined in the base class, CFileBase.

All of the file manipulation functions except Rename() may operate recursively,
and all can operate non-recursively. When operating recursively,
these functions will act on all matching files located throughout
the source directory’s hierarchy. When operating non-recursively,
these functions act upon files contained in the single top level source
directory only. Recursion is set or unset using the switch parameter to
these functions.

This class is not intended for user derivation.

Note: 

To support wildcard, CFileMan needs to store the entire directory entry 
information. Therefore, in a extreme condition, if a directory contains 
a huge number of files (e.g. more than 15000 files with 10 characters' long file 
names), user may encounter KErrNoMemory errors. Developers who have a need to handle 
this rare case should increase the heap size limitation of their applications. 

For more information about heap size configuration, please refer following 
section in Symbian Developer Library:
Symbian OS build guide >> Build Tools Reference >> MMP file syntax >> epocheapsize   

@see MFileManObserver
*/
	{
public:
    /**
    An enumeration that identifies CFileMan tasks. This enumeration is used
    by CurrentAction() to identify which task currently being carried out.

	@see CFileMan::CurrentAction
    */
	enum TAction
		{
		/**
		Inactive
		*/
		ENone,


		/**
		Setting attributes
		*/
		EAttribs,
		
		
		/**
		Copying files
		*/
		ECopy,
		
		
		/**
		Deleting files
		*/
		EDelete,
		
		
		/**
		Moving files
		*/
		EMove,
		
		
		/**
		Renaming files
		*/
		ERename,
		
		
		/**
		Deleting a directory and all contents
		*/
		ERmDir,
		
		
		/**
		Renaming component to VFAT short name (guaranteed to be unique)
		*/
		ERenameInvalidEntry,

		/**
		Copying file from open file handle
		*/
		ECopyFromHandle,
		};
		
		
	/**
	Overwriting and recursion switch.
	
	Used in CFileMan functions to set whether operations are applied to
	the specified directory and all directories below it, or
	the specified directory only. 
	*/	
	enum TSwitch
		{
		/**
		Any files in the destination directory that have the same name as
		the source files in a rename, move or copy operation, will
		be overwritten.
		*/
		EOverWrite=1,
		
		
		/**
		Recursive operation.
		*/
		ERecurse=2
		};
public:
	IMPORT_C static CFileMan* NewL(RFs& aFs);
	IMPORT_C static CFileMan* NewL(RFs& aFs,MFileManObserver* anObserver);
	~CFileMan();
	IMPORT_C TAction CurrentAction();
	IMPORT_C void GetCurrentTarget(TFileName& aFile);
	IMPORT_C void GetCurrentSource(TFileName& aFile);
	IMPORT_C TInt BytesTransferredByCopyStep();
public:
	IMPORT_C TInt Attribs(const TDesC& aName,TUint aSetMask,TUint aClearMask,const TTime& aTime,TUint aSwitch=0);
	IMPORT_C TInt Attribs(const TDesC& aName,TUint aSetMask,TUint aClearMask,const TTime& aTime,TUint aSwitch,TRequestStatus& aStatus);
	IMPORT_C TInt Copy(const TDesC& anOld,const TDesC& aNew,TUint aSwitch=EOverWrite);
	IMPORT_C TInt Copy(const TDesC& anOld,const TDesC& aNew,TUint aSwitch,TRequestStatus& aStatus);
	IMPORT_C TInt Delete(const TDesC& aName,TUint aSwitch=0);
	IMPORT_C TInt Delete(const TDesC& aName,TUint aSwitch,TRequestStatus& aStatus);
	IMPORT_C TInt Move(const TDesC& anOld,const TDesC& aNew,TUint aSwitch=EOverWrite);
	IMPORT_C TInt Move(const TDesC& anOld,const TDesC& aNew,TUint aSwitch,TRequestStatus& aStatus);
	IMPORT_C TInt Rename(const TDesC& anOld,const TDesC& aNew,TUint aSwitch=EOverWrite);
	IMPORT_C TInt Rename(const TDesC& anOld,const TDesC& aNew,TUint aSwitch,TRequestStatus& aStatus);
	IMPORT_C TInt RmDir(const TDesC& aDirName);
	IMPORT_C TInt RmDir(const TDesC& aDirName,TRequestStatus& aStatus);
	IMPORT_C TInt Copy(const RFile& anOld,const TDesC& aNew,TUint aSwitches=EOverWrite);
	IMPORT_C TInt Copy(const RFile& anOld,const TDesC& aNew,TUint aSwitches,TRequestStatus& aStatus);
protected:
	CFileMan(RFs& aFs);
	TInt RenameInvalidEntry(const TDesC& anOld,const TDesC& aNew,TUint aSwitch=EOverWrite);
private:
	/**
	This is an internal enumeration for CFileMan implementation. 
	THis enumeration is mapped into TAction when user wants to identify the current
	task of CFileMan by CurrentAction().

	@see CFileMan::TAction
	@see CFileMan::CurrentAction
    */	
	enum TInternalAction
		{
		/**
		Internal indicator for None operation.
		This is mapped to CFileMan::ENone.
		*/
		EInternalNone,
		
		/**
		Internal indicator for Attribs() operation.
		This is mapped to CFileMan::EAttribs.
		*/
		EInternalAttribs,
		
		/**
		Internal indicator for Copy() operation.
		This is mapped to CFileMan::ECopy.
		*/
		EInternalCopy,
		
		/**
		Internal indicator for Delete() operation.
		This is mapped to CFileMan::EDelete.
		*/
		EInternalDelete,
		
		/**
		Internal indicator for Move() operation on different drives.
		This is mapped to CFileMan::Move.
		*/
		EInternalCopyForMove,
		
		/**
		Internal indicator for Move() operation on the same drive.
		This is mapped to CFileMan::Rename.
		Note for compatibility reasons, it is not mapped to CFileMan::Move.
		*/
		EInternalRenameForMove,
		
		/**
		Internal indicator for Rename() operation.
		This is mapped to CFileMan::ERename.
		*/
		EInternalRename,
		
		/**
		Internal indicator for RmDir() operation.
		This is mapped to CFileMan::ERmDir.
		*/
		EInternalRmDir,
		
		/**
		Internal indicator for RenameInvalidEntry() operation.
		This is mapped to CFileMan::ERenameInvalidEntry.
		*/
		EInternalRenameInvalidEntry,
		
		/**
		Internal indicator for CopyFromHandle() operation.
		This is mapped to CFileMan::ECopyFromHandle.
		*/
		EInternalCopyFromHandle,
		};

	void CompleteOperationL();
	void DoOperationL();
	void CheckForDirectory();
	void SetFlags(TBool aOverWrite,TBool aRecurse,TBool aScanDirection,TBool aMoveRename);
	void GetSrcAndTrg(TParse& aSrcName,TFileName& aTrgName);
	void DoSynchronize(TInt aRetVal);
	TInt CheckRenameAllowed(const TDesC& aSrcName,const TDesC& aTrgName);
	TInt SetupMoveOnSameDrive(TUint aSwitches, TBool& aComplete);
	TInt SetupMoveAcrossDrives(TUint aSwitches);
	TInt SetupTargetDirectory(TBool aOverWrite, TBool& aComplete);
	TBool SrcTrgDrivesIdentical();
	TInt SetupDirectoryForMove(TBool& aSrcIsDir);
private:
	void DoAttribsL();
	void DoCopyOrMoveL();
	void DoDeleteL();
	void DoRenameL();
	void DoRmDirL();
	void DoCopyFromHandleL();
#ifndef SYMBIAN_ENABLE_64_BIT_FILE_SERVER_API
	TInt DoCopy(const RFile& aSrcFile, RFile& aDstFile, TInt& aRet);
#else
	TInt DoCopy(const RFile64& aSrcFile, RFile64& aDstFile, TInt& aRet);
#endif
private:
	TParse iTrgFile;
	TInternalAction iAction;
	TUint iSetMask;
	TUint iClearMask;
	TTime iTime;
	TInt iBytesTransferred;
#ifndef SYMBIAN_ENABLE_64_BIT_FILE_SERVER_API
	RFile iSrcFileHandle;
#else
	RFile64 iSrcFileHandle;
#endif
	TBool iMovingContents;
	TEntry iTmpEntry;
	TParse iTmpParse;
	TFileName iTmpName1;
	TFileName iTmpName2;

friend void RenameInvalidEntryL(RFs& aFs,TParse& aSrcFile);
	};




class TFindFile
/**
@publishedAll
@released

Searches for files and directories.

Each function has a variant which searches for multiple files/directories,
using one or more wildcard characters in the filename.
If an initial search is successful, further searches can be carried out
using Find() or FindWild().
You can also retrieve the fully qualified file specification,
and manipulate and interrogate it using the TParse class (or related classes).

Note that when specifying the path of a directory to search, the path should
always end with a backslash character.When trailing backslash is not present 
then it is considered as file. And path will be taken till last backslash.
The client must have appropriate capabilities for the directory to be searched. 
For example without ALL FILES Capability, it is not possible to successfully 
find any files under \sys\bin directory.

By default if the file is not found in the current drive the rest of the drives,
excluding the remote ones, will be searched. Using function SetFindMask it is 
possible to specify a combination of drive attributes(aMask) that the drives to 
be searched must match.  
 
*/
	{
public:
	IMPORT_C TFindFile(RFs& aFs);
	IMPORT_C TInt FindByPath(const TDesC& aFileName,const TDesC* aPathList);
	IMPORT_C TInt FindByDir(const TDesC& aFileName,const TDesC& aDirPath);
	IMPORT_C TInt Find();
	IMPORT_C TInt FindWildByPath(const TDesC& aFileName,const TDesC* aPathList,CDir*& aDirList);
	IMPORT_C TInt FindWildByDir(const TDesC& aFileName,const TDesC& aDirPath,CDir*& aDir);
	IMPORT_C TInt FindWild(CDir*& aDirList);
	IMPORT_C TInt SetFindMask(TUint aMask);
	inline const TDesC& File() const;
private:
	TInt DoFind();
	TInt DoFindByPath(const TDesC& aFileName,const TDesC* aPathList);
	TInt DoFindByDir(const TDesC& aFileName,const TDesC& aDir);
	TInt DoFindInDir();
	TInt DoFindNextInPath();
	TInt DoFindNextInDriveList();
private:
	RFs* const iFs;
	TParse iFile;
	TInt iPathPos;
	TInt iCurrentDrive;
	TInt iMode;
	const TDesC* iPath;
	TDriveList iDrvList;
	CDir** iDir;
	TUint32 iMatchMask;	
	};




/**
@publishedAll
@released

Contains a list of entries for the files which were opened in
a file server session.

@see CDir
*/
typedef CDir CFileList;




class TOpenFileScan
/**
@publishedAll
@released

Scans open files to get a list of the entries for all files which are currently
open in a particular file server session.

NextL() creates a list of the files opened by the session.
The ID of the thread which opened the files listed may be obtained by calling ThreadId().
If multiple sessions are in use, repeatedly calling NextL() will return a list
of open files in each session.
*/
	{
public:
	IMPORT_C TOpenFileScan(RFs& aFs);
	IMPORT_C void NextL(CFileList*& aFileList);
	IMPORT_C TThreadId ThreadId() const;
private:
	RFs* iFs;
	TThreadId iThreadId;
	TInt iScanPos;
	TInt iEntryListPos;
	};




class TFileText
/**
@publishedAll
@released

Reads and writes single lines of text to or from a Unicode file.
*/
	{
private:
	enum TFileState
		{
		EStartOfFile,
		ENormal,
		EReverse
		};
public:
	IMPORT_C TFileText();
	IMPORT_C void Set(RFile& aFile);
	IMPORT_C TInt Read(TDes& aDes);
	IMPORT_C TInt Write(const TDesC& aDes);
	IMPORT_C TInt Seek(TSeek aMode);
private:
	void NextRecord();
	TInt CheckForTerminator(TBool& anAnswer);
	TInt FillBuffer();
private:
	const TText* iNext;
	const TText* iEnd;
	TFileState iState;
#ifndef SYMBIAN_ENABLE_64_BIT_FILE_SERVER_API
	RFile iFile;
#else
	RFile64 iFile;
#endif
	TBuf8<0x100> iReadBuf; 
	};




/**
@publishedAll
@released
*/
IMPORT_C TBool FileNamesIdentical(const TDesC& aFileName1,const TDesC& aFileName2);

/**
@publishedAll
@released

The UID of the File Server process
*/
const TInt KFileServerUidValue = 0x100039e3;


#include <f32file.inl>
#ifdef SYMBIAN_ENABLE_64_BIT_FILE_SERVER_API
#include <f32file64.inl>
#endif

#ifdef SYMBIAN_F32_ENHANCED_CHANGE_NOTIFICATION	
#include <f32notification.h>
#endif

#ifndef SYMBIAN_ENABLE_SPLIT_HEADERS
#include <f32file_private.h>
#endif

#endif
