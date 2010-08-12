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

#include "elocal.h"
#include <emulator.h>
#include <TCHAR.h>

const TInt KMajorVersionNumber=1;
const TInt KMinorVersionNumber=0;

const TUint KInvalidSetFilePointer = 0xffffffff;	// INVALID_SET_FILE_POINTER

#pragma data_seg(".data2")
#ifdef __VC32__
#pragma bss_seg(".data2")
#endif
static TInt ReadSpeed;
static TInt WriteSpeed;
#pragma data_seg()
#ifdef __VC32__
#pragma bss_seg()
#endif

static void Panic(TPanic aPanic)
	{
	User::Panic(_L("LocalFSys"),aPanic);
	}


//
// Map aDrive to a path given by environment variables
//
TBool MapDrive(TDes& aFileName, TInt aDrive)
	{

	TDriveName root(TDriveUnit(aDrive).Name());
	TFileName path;
	TBuf<3> rootWithSlash(root);
	rootWithSlash.Append('\\');

	if (MapEmulatedFileName(path, rootWithSlash) == KErrNone)
		{
		aFileName=path.Left(3);	// drive letter, colon and backslash
		return(ETrue);
		}
	aFileName=root;  // no trailing backslash
	return(EFalse);
	}
	
TInt MapFileName(TDes& aFileName, TInt aDrive, const TDesC& aName)
	{
	TFileName n(TDriveUnit(aDrive).Name());
	n.Append(aName);
	return MapEmulatedFileName(aFileName,n);
	}

void MapFileNameL(TDes& aFileName, TInt aDrive, const TDesC& aName)
	{
	User::LeaveIfError(MapFileName(aFileName,aDrive,aName));
	}


/**
Check whether a descriptor has enough space for null-terminating and append a zero terminator if it does.
Supposed to be used for Win32 file operations, taking C-like strings as parameters.
The main purpose is to avoid panics caused by descriptors overflow.

@param  aDes descriptor to be null-terminated; a file(directory) name presumably.
@return cast to LPCTSTR value of the descriptor, supposed to be the unicode string
@leave  KErrBadName if there is no room for trailing zero 
*/
LPCTSTR StrPtrZL(TDes16& aDes)
    {
    if(aDes.MaxLength() - aDes.Length() < 1)
        User::Leave(KErrBadName);  //-- no room for terminating zero
    
    return (LPCTSTR)aDes.PtrZ();
    }



//
// Converts a TTime structure to a Windows/NT filetime.  Assumes that aTime is a
// UTC (system) time
//
static void timeToFileTimeL(const TTime& aTime,FILETIME* f)
	{

	TDateTime dateTime=aTime.DateTime();
	SYSTEMTIME t;
	#pragma warning( disable : 4244 ) // conversion from 'const int' to 'unsigned short', possible loss of data
	t.wYear=dateTime.Year();
	t.wMonth=dateTime.Month()+1;
	t.wDay=dateTime.Day()+1;
	t.wDayOfWeek=(aTime.DayNoInWeek()+1)%7;
	t.wHour=dateTime.Hour();
	t.wMinute=dateTime.Minute();
	t.wSecond=dateTime.Second();
	t.wMilliseconds=dateTime.MicroSecond()/1000;
	#pragma warning( default : 4244 ) // conversion from 'const int' to 'unsigned short', possible loss of data
	__ASSERT_ALWAYS(SystemTimeToFileTime(&t,f)==TRUE,User::Leave(KErrArgument));
	}

//
//	Convert Windows/NT file time to TTime
//	Assumes the NT file time is UTC
//
static void fileTimeToTime(FILETIME* f,TTime& aTime)
	{
	SYSTEMTIME t;
	__ASSERT_ALWAYS(FileTimeToSystemTime(f,&t)==TRUE,Panic(EFileTimeToSystemTime));
	aTime=TDateTime(t.wYear,TMonth(t.wMonth-1),t.wDay-1,t.wHour,t.wMinute,t.wSecond,t.wMilliseconds*1000);
	}

//
// Return the size and free space on a drive.
//
static TInt GetMediaSize(TInt aDriveNumber,TInt64& aSize,TInt64& aFree)
	{

	TBuf<4> driveName;
	MapDrive(driveName,aDriveNumber);
	DWORD sectorsPerCluster;
	DWORD bytesPerSector;
	DWORD freeClusters;
	DWORD sizeClusters;
	// this function should be upgraded to GetDiskFreeSpaceEx
	BOOL b=Emulator::GetDiskFreeSpace((LPCTSTR)driveName.PtrZ(),&sectorsPerCluster,&bytesPerSector,&freeClusters,&sizeClusters);
	if (!b)
		return Emulator::LastError();

	TInt64 bytesPerCluster=(TInt)(sectorsPerCluster*bytesPerSector);
	aSize=TInt64((TInt)sizeClusters)*bytesPerCluster;
	aFree=TInt64((TInt)freeClusters)*bytesPerCluster;
	return(KErrNone);
	}

//
// Return the volume name and uniqueID.
//
static TInt GetVolumeId(TInt aDriveNumber,TUint& aUniqueID)
	{

	TBuf<4> driveName;
	MapDrive(driveName,aDriveNumber);
	DWORD uniqueID,componentLength,flags;
	BOOL b=Emulator::GetVolumeInformation((LPCTSTR)driveName.PtrZ(),NULL,0,&uniqueID,&componentLength,&flags,NULL,0);
	if (!b)
		return Emulator::LastError();

	aUniqueID=uniqueID;
	return(KErrNone);
	}

//#########################################################################################################################
//##        CLocalMountCB class implementation
//#########################################################################################################################


CLocalMountCB::CLocalMountCB()
	{
	}

CLocalMountCB::~CLocalMountCB()
	{
	}


//
// Returns ETrue if the drive == EDriveZ
//
TBool CLocalMountCB::IsRomDrive() const
	{
	// WINS emulated rom drive is Z:
	return(Drive().DriveNumber()==EDriveZ);
	}

//-------------------------------------------------------------------------------------------------------------------

/**
Mount a volume. 

@param aForceMount Flag to indicate whether mount should be forced to succeed if an error occurs
@leave KErrNoMemory,KErrNotReady,KErrCorrupt,KErrUnknown.
*/
void CLocalMountCB::MountL(TBool /*aForceMount*/)
	{
	TInt64 s,f;
	const TInt driveNum=Drive().DriveNumber();
	User::LeaveIfError(GetMediaSize(driveNum,s,f));
	
    iSize=s;
    if (driveNum==EDriveZ)
		iSize=4*1048576;
	
    User::LeaveIfError(GetVolumeId(driveNum,iUniqueID));
	SetVolumeName(HBufC::NewL(0));	// all Win32 volumes are unnamed
	
    //-- assign default value, 4G-1
    iMaxFileSizeSupported = ((TUint64)4 << 30)-1;

        {
        //-- find out the maximal supported file size. For this we need to query the name of the windows filesystem that is
        //-- used for the emulated drive
        TBuf<20> bufWinDrive;
        MapDrive(bufWinDrive, Drive().DriveNumber());
        ASSERT(bufWinDrive.Length() >= 3);

        TCHAR rootName[10];
        TCHAR volName[30];
        TCHAR volFSFileName[30];
        DWORD volSerNum;
        DWORD volMaxCompLen;
        DWORD volFsFlags;
        
        memset(rootName, 0, sizeof(rootName));
        wcsncpy(rootName, bufWinDrive.Ptr(), 3); //- something like "k:\\"

        BOOL b = GetVolumeInformation(rootName, volName, sizeof(volName)/sizeof(TCHAR), &volSerNum, &volMaxCompLen, &volFsFlags, volFSFileName, sizeof(volFSFileName)/sizeof(TCHAR));
        if(b)
            {
            if(_wcsicmp(volFSFileName, _TEXT("NTFS")) == 0)
                {//-- this is NTFS
                iMaxFileSizeSupported = 0xFFFFFFF0000; //-- max. file size for NTFS
                }
             else
                {//-- theoretically other than FAT & NTFS filesystem are possible.. Figure yourself.
            }   }
        }


    }

//-------------------------------------------------------------------------------------------------------------------
/**
    Try remount this volume. Checks if the volume parameters remained the same as on original MountL() call, and
    if they are, re-initialises the mount. 
    @return KErrNone if the remount was OK
            system-wide error code otherwise
*/
TInt CLocalMountCB::ReMount()
	{

	TInt d=Drive().DriveNumber();
	TUint uniqueID;
	TInt r=GetVolumeId(d,uniqueID);
	if (r==KErrNone && uniqueID!=iUniqueID)
		r=KErrGeneral;
	return(r);
	}

//-------------------------------------------------------------------------------------------------------------------
void CLocalMountCB::Dismounted()
	{
    }

//-------------------------------------------------------------------------------------------------------------------
//
// Return the volume info.
//
void CLocalMountCB::VolumeL(TVolumeInfo& aVolume) const
	{

	TInt64 s,f;
	TInt driveNum=Drive().DriveNumber();
	User::LeaveIfError(GetMediaSize(driveNum,s,f));
	if (driveNum==EDriveZ)
		aVolume.iFree=0;
	else
		aVolume.iFree=f;
	}

//-------------------------------------------------------------------------------------------------------------------
//
// Set the volume label. Not supported on Win32 volumes
//
void CLocalMountCB::SetVolumeL(TDes&)
	{

	User::Leave(IsRomDrive() ? KErrAccessDenied : KErrNotSupported);
	}

//-------------------------------------------------------------------------------------------------------------------
//
// Return the address of the file if it is in rom
//
void CLocalMountCB::IsFileInRom(const TDesC& aName,TUint8*& aFileStart)
	{

	aFileStart=NULL;
	if (!IsRomDrive())
		return;

	TFileName n;
	if (MapFileName(n,Drive().DriveNumber(),aName)!=KErrNone)
		return;
	
	DWORD access=GENERIC_READ;
	DWORD share=FILE_SHARE_WRITE|FILE_SHARE_READ;
	DWORD create=OPEN_EXISTING;
	HANDLE h=Emulator::CreateFile((LPCTSTR)n.PtrZ(),access,share,NULL,create,FILE_FLAG_RANDOM_ACCESS,NULL);
	if (h==INVALID_HANDLE_VALUE)
		return;

	CLocalFileCB::RomAddress(aName, h, aFileStart);
	CloseHandle(h);
	}

//-------------------------------------------------------------------------------------------------------------------
/**
    Make a directory.
    @param aName full path to the directory to create. Name validity is checked by file server.
*/
void CLocalMountCB::MkDirL(const TDesC& aName)
	{

	if (IsRomDrive())
		User::Leave(KErrAccessDenied);
	TFileName n;
	MapFileNameL(n,Drive().DriveNumber(),aName);
	BOOL b=Emulator::CreateDirectory(StrPtrZL(n),NULL);
	
	if (b)
		return;
	TInt r=Emulator::LastError();
	if (r!=KErrAlreadyExists)
		User::Leave(r);
	TEntry e;
	EntryL(aName,e);

	if (e.IsDir())
		User::Leave(KErrAlreadyExists);
	else
		User::Leave(KErrAccessDenied);
	}

//-------------------------------------------------------------------------------------------------------------------
/**
    Remove a directory.
    @param aName directory name
*/
void CLocalMountCB::RmDirL(const TDesC& aName)
	{

	if (IsRomDrive())
		User::Leave(KErrAccessDenied);
	
    TFileName n;
	MapFileNameL(n,Drive().DriveNumber(),aName);
	BOOL b=Emulator::RemoveDirectory(StrPtrZL(n));
	
	if (!b)
		User::Leave(Emulator::LastError());
	}

//-------------------------------------------------------------------------------------------------------------------
//
// Delete a file.
//
void CLocalMountCB::DeleteL(const TDesC& aName)
	{

	if (IsRomDrive())
		User::Leave(KErrAccessDenied);
	
    //-- check entry attributes
    TEntry entry;
    EntryL(aName, entry);
	if (entry.IsDir() ||  entry.IsReadOnly())
	    User::Leave(KErrAccessDenied);

    TFileName n;
	MapFileNameL(n,Drive().DriveNumber(),aName);
	BOOL b=Emulator::DeleteFile(StrPtrZL(n));
	
	if (!b)
		User::Leave(Emulator::LastError());
	}

//-------------------------------------------------------------------------------------------------------------------
//
// Rename a file or directory.
//
void CLocalMountCB::RenameL(const TDesC& aOldName,const TDesC& aNewName)
	{

	if (IsRomDrive())
		User::Leave(KErrAccessDenied);
	TEntry entry;
	TRAPD(r,EntryL(aNewName,entry));
	if (r!=KErrNone && r!=KErrNotFound)
		User::Leave(r);
	TFileName old;
	MapFileNameL(old,Drive().DriveNumber(),aOldName);
	TFileName n;
	MapFileNameL(n,Drive().DriveNumber(),aNewName);
	BOOL b=Emulator::MoveFile(StrPtrZL(old),StrPtrZL(n));
	
	if (!b)
		User::Leave(Emulator::LastError());
	}

//-------------------------------------------------------------------------------------------------------------------
void CLocalMountCB::ReplaceL(const TDesC& aOldName,const TDesC& aNewName)
//
// Delete aNewName if it exists and rename anOldName.
//
	{

	if (IsRomDrive())
		User::Leave(KErrAccessDenied);
	TEntry entry;
	if(FileNamesIdentical(aOldName,aNewName))
		{
		return;
		}
	TRAPD(r,DeleteL(aNewName));
	if (r!=KErrNotFound && r!=KErrNone)
		User::Leave(r);
	TFileName old;
	MapFileNameL(old,Drive().DriveNumber(),aOldName);
	TFileName n;
	MapFileNameL(n,Drive().DriveNumber(),aNewName);
	BOOL b=Emulator::MoveFile(StrPtrZL(old),StrPtrZL(n));
	if (!b)
		User::Leave(Emulator::LastError());
	}
	
//-------------------------------------------------------------------------------------------------------------------
//
//	Set and get file pointer for windows files
//	
static DWORD SetFilePointerL(HANDLE hFile,LONG lDistanceToMove,DWORD dwMoveMethod)
	
	{
	DWORD dwRet;
	
	dwRet=SetFilePointer(hFile,lDistanceToMove,0,dwMoveMethod);
	if (dwRet==KInvalidSetFilePointer)	//	INVALID_HANDLE_VALUE
		User::Leave(Emulator::LastError());

	return (dwRet);	
	}

//-------------------------------------------------------------------------------------------------------------------
//
//	Set and get file pointer for windows files
//	
static DWORD SetFilePointer64L(HANDLE hFile, LARGE_INTEGER * lpDistanceToMove, DWORD dwMoveMethod)
	{

	DWORD dwRet;
	
	dwRet=SetFilePointer(hFile, lpDistanceToMove->LowPart, &(lpDistanceToMove->HighPart), dwMoveMethod);
	
    TInt r = Emulator::LastError();
	if ((KInvalidSetFilePointer==dwRet) && (r != NO_ERROR))	
		User::Leave(r);

	return (dwRet);	
	}

//-------------------------------------------------------------------------------------------------------------------
/**
    Read file section without opening this file on a file server side.
    
    @param  aName       file name; all trailing dots from the name will be removed
    @param  aFilePos    start read position within a file
    @param  aLength     how many bytes to read; on return will be how many bytes actually read
    @param  aDes        local buffer desctriptor
    @param  aMessage    from file server, used to write data to the buffer in different address space.

    @leave on media read error
*/
void CLocalMountCB::ReadSectionL(const TDesC& aName,TInt aPos,TAny* aTrg,TInt aLength,const RMessagePtr2& aMessage)
	{
	
	TFileName n;
	MapFileNameL(n,Drive().DriveNumber(),aName);
	
	WIN32_FIND_DATA d;
	HANDLE hFile=Emulator::FindFirstFile(StrPtrZL(n),&d);
	if (hFile==INVALID_HANDLE_VALUE)
		User::Leave(Emulator::LastError());
	FOREVER
		{
		TPtrC fileName((TText*)(&d.cFileName[0]));
		if (fileName!=_L(".") && fileName!=_L(".."))
			break;
		if (!Emulator::FindNextFile(hFile,&d))
			{
			TInt r = Emulator::LastError();
			User::Leave(r == KErrEof ? KErrNotFound : r);
			}
		}
	
	FindClose(hFile);
		
	hFile=Emulator::CreateFile(StrPtrZL(n),GENERIC_READ,FILE_SHARE_READ|FILE_SHARE_WRITE,NULL,OPEN_EXISTING,0,NULL);
	if (hFile==INVALID_HANDLE_VALUE)
		return;

	DWORD dwSizeLow, dwSizeHigh;
	dwSizeLow=GetFileSize(hFile,&dwSizeHigh);
	TInt r = Emulator::LastError();
	if((NO_ERROR != r) && (INVALID_FILE_SIZE == dwSizeLow))
		User::Leave(r);
	
	// ReadSectionL can support only upto 2G as aPos is TInt!
	const TInt64 fileSize = MAKE_TINT64(dwSizeHigh, dwSizeHigh);
	if(fileSize > KMaxTInt)
		{
		if (!CloseHandle(hFile))
			User::Leave(Emulator::LastError());

		User::Leave(KErrTooBig);
		}
	
//	Check that reading from aPos for aLength lies within the file
//	if aPos is within the file, and aLength is too long, read up to EOF
//	If aPos is beyond the file, return a zero length descriptor

	if ((TInt)dwSizeLow>=(aPos+aLength))	//	Can read entire length requested from aPos	
		SetFilePointerL(hFile,aPos,FILE_BEGIN);			
	
	else if ((TInt)dwSizeLow>aPos)		//	Can read from aPos but not entire length requested
		{
		SetFilePointerL(hFile,aPos,FILE_BEGIN);
		aLength=dwSizeLow-aPos;
		}	
	else						//	Cannot read from aPos because it lies outside file
		{						//	Close file and leave with KErrEof
		if (!CloseHandle(hFile))
			User::Leave(Emulator::LastError());

		User::Leave(KErrEof);
		}

	TBuf8<0x1000> buf;
	TInt pos=0;

	if (aMessage.Handle() == KLocalMessageHandle)
		((TPtr8* )aTrg)->SetLength(0);
	
	while (aLength)
		{
		TInt readTotal=Min(aLength,buf.MaxLength());
		DWORD ret;
		BOOL b=ReadFile(hFile,(TAny*)buf.Ptr(),readTotal,&ret,NULL);
		if (!b || ((TInt)ret!=readTotal))	
			User::Leave(Emulator::LastError());
		buf.SetLength(ret);
		
		if(aMessage.Handle() == KLocalMessageHandle)
			((TPtr8* )aTrg)->Append(buf);
		else
			aMessage.WriteL(0,buf,pos);
	
		pos+=ret;
		if (((TInt)ret)<readTotal)
			break;
		aLength-=readTotal;
		}
			
	if (!CloseHandle(hFile))
		User::Leave(Emulator::LastError());
	}


//-------------------------------------------------------------------------------------------------------------------
//
// Read the entry uid if present
//
void CLocalMountCB::ReadUidL(const TDesC& aName,TEntry& anEntry) const
	{

//  First check to see if the first sixteen bytes form a valid UID
	TBuf<KMaxFileName + 1> fileName=aName;
	HANDLE hFile=Emulator::CreateFile(StrPtrZL(fileName),GENERIC_READ,FILE_SHARE_READ|FILE_SHARE_WRITE,NULL,OPEN_EXISTING,0,NULL);
	if (hFile==INVALID_HANDLE_VALUE)
		return;
	DWORD ret;
	TBuf8<sizeof(TCheckedUid)> checkedUidBuf;
	checkedUidBuf.SetLength(sizeof(TCheckedUid));
	ReadFile(hFile,&checkedUidBuf[0],sizeof(TCheckedUid),&ret,NULL);
	if (ret!=sizeof(TCheckedUid))
		goto close;
	{
	TCheckedUid checkedUid(checkedUidBuf);
	if(checkedUid.UidType()!=TUidType(TUid::Null(),TUid::Null(),TUid::Null()))
		{
		anEntry.iType=checkedUid.UidType();
		goto close;
		}
	}

//Look at PE file for UID section
		{
		const TInt KPeHeaderAddrAddr=0x3c;
		const TInt KPeHeaderAddrSize=0x01;
		const TInt KNumberOfSectionsOffset=0x06;
		const TInt KNumberOfSectionsSize=0x02;
		const TInt KSectionTableOffset=0xf8;
		const TInt KSectionHeaderSize=0x28;
		const TInt KSectionNameLength=0x08;
		const TInt KPtrToRawDataOffset=0x14;
		const TInt KPtrToRawDataSize=0x04;
		const TText8 peText[4]={'P','E',0,0};
		const TText8 uidText[8]={'.','S','Y','M','B','I','A','N'};
		
	//Read address of start of PE header
		if (SetFilePointer(hFile,KPeHeaderAddrAddr,0,FILE_BEGIN)==KInvalidSetFilePointer)
			goto close;
		TInt peAddr=0;
		ReadFile(hFile,&peAddr,KPeHeaderAddrSize,&ret,NULL);
		if (ret!=KPeHeaderAddrSize)
			goto close;
		
	//Check it really is the start of PE header
		if (SetFilePointer(hFile,peAddr,0,FILE_BEGIN)==KInvalidSetFilePointer)
			goto close;
		TText8 text[4];
		ReadFile(hFile,text,4,&ret,NULL);
		if (*(TInt32*)text!=*(TInt32*)peText)
			goto close;
		
	//Read number of sections
		if (SetFilePointer(hFile,peAddr+KNumberOfSectionsOffset,0,FILE_BEGIN)==KInvalidSetFilePointer)
			goto close;
		TInt sections=0;
		ReadFile(hFile,&sections,KNumberOfSectionsSize,&ret,NULL);
		if (ret!=KNumberOfSectionsSize)
			goto close;

	//Go through section headers looking for UID section
		if (SetFilePointer(hFile,peAddr+KSectionTableOffset,0,FILE_BEGIN)==KInvalidSetFilePointer)
			goto close;
		TInt i=0;
		for(;i<sections;i++)
			{
			TText8 name[KSectionNameLength];
			ReadFile(hFile,name,KSectionNameLength,&ret,NULL);
			if (ret!=KSectionNameLength)
				goto close;
			if (*(TInt64*)name==*(TInt64*)uidText)
				break;
			if (SetFilePointer(hFile,KSectionHeaderSize-KSectionNameLength,0,FILE_CURRENT)==KInvalidSetFilePointer)
				goto close;
			}
		if (i==sections)
			goto close;

	//Read RVA/Offset
		if (SetFilePointer(hFile,KPtrToRawDataOffset-KSectionNameLength,0,FILE_CURRENT)==KInvalidSetFilePointer)
			goto close;
		TInt uidOffset;
		ReadFile(hFile,&uidOffset,KPtrToRawDataSize,&ret,NULL);
		if (ret!=KPtrToRawDataSize)
			goto close;

	//Read UIDs!
		if (SetFilePointer(hFile,uidOffset,0,FILE_BEGIN)==KInvalidSetFilePointer)
			User::Leave(KErrGeneral);

		TEmulatorImageHeader header;
		ReadFile(hFile,&header,sizeof(header),&ret,NULL);
		if (ret==sizeof(header))
			anEntry.iType=*(TUidType*)&header;
		}
//Close file
close:
	if (!CloseHandle(hFile))
		User::Leave(Emulator::LastError());
	}

//-------------------------------------------------------------------------------------------------------------------
/**
    Try to find a directory entry by the given name and path. 
    This method _must_ leave if the entry is not found. See the caller.

    @param  aName   path to the directory object. all trailing dots from the name will be removed.
    @param  anEntry on return will contain the entry data
    
    @leave  KErrPathNotFound if there is no path to the aName
            KErrNotFound     if the entry corresponding to the aName is not found
            system-wide erorr code of media read failure.
*/
void CLocalMountCB::EntryL(const TDesC& aName,TEntry& anEntry) const
	{

	TFileName n;
	MapFileNameL(n,Drive().DriveNumber(),aName);
	WIN32_FIND_DATA d;
	HANDLE h=Emulator::FindFirstFile(StrPtrZL(n),&d);
	if (h==INVALID_HANDLE_VALUE)
		User::Leave(Emulator::LastError());
	FOREVER
		{
		TPtrC fileName((TText*)(&d.cFileName[0]));
		if (fileName!=_L(".") && fileName!=_L(".."))
			break;
		if (!Emulator::FindNextFile(h,&d))
			{
			TInt r = Emulator::LastError();
			User::Leave(r == KErrEof ? KErrNotFound : r);
			}
		}
	FindClose(h);
	anEntry.iName.Des()=(TText*)(&d.cFileName[0]);
	anEntry.iAtt=d.dwFileAttributes&KEntryAttMaskSupported;
	if (IsRomDrive())
		anEntry.iAtt|=KEntryAttReadOnly;

	anEntry.SetFileSize(MAKE_TINT64(d.nFileSizeHigh,d.nFileSizeLow));

	fileTimeToTime(&d.ftLastWriteTime,anEntry.iModified);
	ReadUidL(n,anEntry);
	}

//-------------------------------------------------------------------------------------------------------------------
/**
    Set directory entry details.
    @param  aName           entry name; all trailing dots from the name will be removed
    @param  aTime           entry modification time (and last access as well)
    @param  aSetAttMask     entry attributes OR mask
    @param  aClearAttMask   entry attributes AND mask

*/
void CLocalMountCB::SetEntryL(const TDesC& aName,const TTime& aTime,TUint aSetAttMask,TUint aClearAttMask)
	{

	if (IsRomDrive())
		User::Leave(KErrAccessDenied);
	TFileName n;
	MapFileNameL(n,Drive().DriveNumber(),aName);
	TUint setAttMask=aSetAttMask&KEntryAttMaskSupported;
	DWORD att=Emulator::GetFileAttributes(StrPtrZL(n));
	if (att==0xffffffffu)
		User::Leave(Emulator::LastError());
	
    if (setAttMask|aClearAttMask)
		{
		att|=setAttMask;
		att&=(~aClearAttMask);
		if (!Emulator::SetFileAttributes((LPCTSTR)n.Ptr(),att))
			User::Leave(Emulator::LastError());
		}
	
    if (aSetAttMask&KEntryAttModified)
		{
		FILETIME f;
		timeToFileTimeL(aTime,&f);

		if (att&KEntryAttReadOnly)
			{
			DWORD writeableAtt=att&(~KEntryAttReadOnly);
			if (!Emulator::SetFileAttributes((LPCTSTR)n.Ptr(),writeableAtt))
				User::Leave(Emulator::LastError());
			}

		HANDLE h;
		if (att&KEntryAttDir)
			{
			h=Emulator::CreateFile((LPCTSTR)n.Ptr(),GENERIC_READ|GENERIC_WRITE,0,NULL,OPEN_EXISTING,FILE_ATTRIBUTE_DIRECTORY|FILE_FLAG_BACKUP_SEMANTICS, NULL);
			if (h==INVALID_HANDLE_VALUE)
				User::Leave(Emulator::LastError());
			}
		else
			{
			h=Emulator::CreateFile((LPCTSTR)n.Ptr(),GENERIC_READ|GENERIC_WRITE,0,NULL,OPEN_EXISTING,0,NULL);
			if (h==INVALID_HANDLE_VALUE)
				User::Leave(Emulator::LastError());
			}

		if (!SetFileTime(h,NULL,&f,&f))
			{
			TInt error = Emulator::LastError(); 
			CloseHandle(h);
			User::Leave(error);
			}
		
        if (!CloseHandle(h))
			User::Leave(Emulator::LastError());
		
        if ((att&KEntryAttReadOnly) && !Emulator::SetFileAttributes((LPCTSTR)n.Ptr(),att))
			User::Leave(Emulator::LastError());
		}
	}

//-------------------------------------------------------------------------------------------------------------------
/**
    Open/Create/Replace a file on the current mount.
    
    @param  aName   file name; all trailing dots from the name will be removed
    @param  aMode   File open mode, See TFileMode
    @param  anOpen  specifies action: open, create or replace the file
    @param  aFile   pointer to the CFileCB object to populate

*/
void CLocalMountCB::FileOpenL(const TDesC& aName,TUint aMode,TFileOpen anOpen,CFileCB* aFile)
	{

	if (IsRomDrive() && (anOpen!=EFileOpen || (aMode&EFileWrite)))
		User::Leave(KErrAccessDenied);
	TFileName n;
	MapFileNameL(n,Drive().DriveNumber(),aName);
	
	DWORD access=GENERIC_READ|GENERIC_WRITE;
	DWORD share=FILE_SHARE_WRITE|FILE_SHARE_READ;
	DWORD create=0;
	switch (anOpen)
		{
	    case EFileOpen: create=OPEN_EXISTING; break;
	    case EFileCreate: create=CREATE_NEW; break;
	    case EFileReplace: create=CREATE_ALWAYS; break;
		}

	HANDLE h=Emulator::CreateFile(StrPtrZL(n),access,share,NULL,create,FILE_FLAG_RANDOM_ACCESS,NULL);
	
	if((h==INVALID_HANDLE_VALUE) && !(aMode&EFileWrite))
	{
	// If windows will not allow write access and it was not requested then open for read only
	access=GENERIC_READ;
	h=Emulator::CreateFile(StrPtrZL(n),access,share,NULL,create,FILE_FLAG_RANDOM_ACCESS,NULL);
	}

	if (h==INVALID_HANDLE_VALUE)
		User::Leave(Emulator::LastError());
	CLocalFileCB& file=(*((CLocalFileCB*)aFile));
	file.SetHandle(h);
	
    BY_HANDLE_FILE_INFORMATION info;
	if (!GetFileInformationByHandle(h,&info))
		User::Leave(Emulator::LastError());
	
    const TUint64 fileSize = MAKE_TUINT64(info.nFileSizeHigh, info.nFileSizeLow);
	
    // Check on file size
	if(MaxFileSizeSupported() < fileSize)
		User::Leave(KErrTooBig);
	
    file.SetMaxSupportedSize(MaxFileSizeSupported());
    file.SetSize64(fileSize, EFalse);
	file.SetAtt(info.dwFileAttributes&KEntryAttMaskSupported);

	if (IsRomDrive())
		file.SetAtt(file.Att() | KEntryAttReadOnly);
	TTime tempTime=file.Modified();
	fileTimeToTime(&info.ftLastWriteTime,tempTime);
	file.SetModified(tempTime);
	}

void AppendAsteriskL(TDes& aDes)
	{
	if (aDes.Length()==aDes.MaxLength())
		User::Leave(KErrBadName);
	aDes.Append('*');
	}

//-------------------------------------------------------------------------------------------------------------------	
/**
    Open a directory on the current mount.
    
    @param  aName   path to the object in the directory we want to open; all trailing dots from the name will be removed
    @param  aDir    dir. CB to be filled in.
    
    If there is no such a path, this method must leave with KErrPathNotFound

    @leave  KErrPathNotFound if thereis no such path
    @leave  error code on media read fault
*/
void CLocalMountCB::DirOpenL(const TDesC& aName,CDirCB* aDir)
	{

	TFileName n;
	TParse parse;
	MapFileNameL(n,Drive().DriveNumber(),aName);
	parse.Set(n,NULL,NULL);
	n=parse.DriveAndPath();
	AppendAsteriskL(n);
	WIN32_FIND_DATA info;
	HANDLE h=Emulator::FindFirstFile(StrPtrZL(n),&info);
	if (h==INVALID_HANDLE_VALUE)
		{
		TInt error=Emulator::LastError();
		TParse parser;
		TInt r=parser.Set(n,NULL,NULL);
		if (r!=KErrNone)
			User::Leave(r);
		if (!parser.IsRoot() || Drive().DriveNumber()!=0 || error!=KErrNotFound)
			User::Leave(error);
		h=NULL;
		}
	CLocalDirCB& dir=(*((CLocalDirCB*)aDir));
	dir.SetHandle(h);
	dir.SetPending(ETrue);
	dir.iEntry.iName.Des()=(TText*)(&info.cFileName[0]);
	dir.iEntry.iAtt=info.dwFileAttributes&KEntryAttMaskSupported;

	const TInt64 fileSize = MAKE_TINT64(info.nFileSizeHigh,info.nFileSizeLow);
	dir.iEntry.SetFileSize(fileSize);

	n=parse.FullName();
	if (parse.NameAndExt().Length()==0)
		AppendAsteriskL(n);
	dir.SetFullName(n);
	fileTimeToTime(&info.ftLastWriteTime,dir.iEntry.iModified);
	}

//-------------------------------------------------------------------------------------------------------------------	
//
// Read directly from disk
//
void CLocalMountCB::RawReadL(TInt64 /*aPos*/,TInt /*aLength*/,const TAny* /*aDes*/,TInt /*anOffset*/,const RMessagePtr2& /*aMessage*/) const
	{
	User::Leave(KErrNotSupported);
	}

//-------------------------------------------------------------------------------------------------------------------	
//
// Write directly to disk
//
void CLocalMountCB::RawWriteL(TInt64 /*aPos*/,TInt /*aLength*/,const TAny* /*aDes*/ ,TInt /*anOffset*/,const RMessagePtr2& /*aMessage*/)
	{
	User::Leave(KErrNotSupported);
	}

//-------------------------------------------------------------------------------------------------------------------	
//
// Get the short name associated with aLongName
//
void CLocalMountCB::GetShortNameL(const TDesC& aLongName,TDes& aShortName)
	{

	if (IsRomDrive())
		User::Leave(KErrNotSupported);

	TFileName n;
	MapFileNameL(n,Drive().DriveNumber(),aLongName);
	WIN32_FIND_DATA d;
	HANDLE h=Emulator::FindFirstFile(StrPtrZL(n),&d);
	if (h==INVALID_HANDLE_VALUE)
		User::Leave(Emulator::LastError());
	FindClose(h);
    if (d.cAlternateFileName[0])	// we have a dos name too
        aShortName=(TText*)(&d.cAlternateFileName[0]);
	else
		aShortName=(TText*)(&d.cFileName[0]);
	}

//-------------------------------------------------------------------------------------------------------------------	
//
// Get the short name associated with aLongName
//
void CLocalMountCB::GetLongNameL(const TDesC& aShortName,TDes& aLongName)
	{

	if (IsRomDrive())
		User::Leave(KErrNotSupported);

	TFileName n;
	MapFileNameL(n,Drive().DriveNumber(),aShortName);
	WIN32_FIND_DATA d;
	HANDLE h=Emulator::FindFirstFile(StrPtrZL(n),&d);
	if (h==INVALID_HANDLE_VALUE)
		User::Leave(Emulator::LastError());
	FindClose(h);
	aLongName=(TText*)(&d.cFileName[0]);
	}

//-------------------------------------------------------------------------------------------------------------------	
/**
Reports whether the specified interface is supported - if it is,
the supplied interface object is modified to it

@param aInterfaceId     The interface of interest
@param aInterface       The interface object
@return                 KErrNone if the interface is supported, otherwise KErrNotFound 

@see CMountCB::GetInterface()
*/
TInt CLocalMountCB::GetInterface(TInt aInterfaceId,TAny*& aInterface,TAny* aInput)
    {
	switch(aInterfaceId)
		{
		case EFileExtendedInterface:
			((CMountCB::MFileExtendedInterface*&) aInterface) = this;
			return KErrNone;

    	case ELocalBufferSupport:
    		// CLocalMountCB doesn't ever use any extensions?
	    	// 	- seems to not have any iProxyDrive or LocalDrive() or similar, 
    		// so we'll just return KErrNone here.
   			return KErrNone;

		default:
		    return CMountCB::GetInterface(aInterfaceId,aInterface,aInput);
		}
    }

//-------------------------------------------------------------------------------------------------------------------	
TInt CLocalMountCB::LocalBufferSupport()
	{
	TAny* dummyInterface = NULL;
	TAny* dummyInput = NULL;
	return GetInterface(ELocalBufferSupport,dummyInterface,dummyInput);
	}

//-------------------------------------------------------------------------------------------------------------------	
/**
    Read file section without opening this file on a file server side.
    
    @param  aName       file name; all trailing dots from the name will be removed
    @param  aFilePos    start read position within a file
    @param  aLength     how many bytes to read; on return will be how many bytes actually read
    @param  aDes        local buffer desctriptor
    @param  aMessage    from file server, used to write data to the buffer in different address space.

    @leave on media read error
*/
void CLocalMountCB::ReadSection64L(const TDesC& aName, TInt64 aPos, TAny* aTrg, TInt aLength, const RMessagePtr2& aMessage)
	{
	TFileName n;
	MapFileNameL(n,Drive().DriveNumber(),aName);
	
	WIN32_FIND_DATA d;
	HANDLE hFile=Emulator::FindFirstFile(StrPtrZL(n),&d);
	if (hFile==INVALID_HANDLE_VALUE)
		User::Leave(Emulator::LastError());
	
	FOREVER
		{
		TPtrC fileName((TText*)(&d.cFileName[0]));
		if (fileName!=_L(".") && fileName!=_L(".."))
			break;
		if (!Emulator::FindNextFile(hFile,&d))
			{
			TInt r = Emulator::LastError();
			User::Leave(r == KErrEof ? KErrNotFound : r);
			}
		}
	
	FindClose(hFile);
	
	hFile=Emulator::CreateFile(StrPtrZL(n),GENERIC_READ,FILE_SHARE_READ|FILE_SHARE_WRITE,NULL,OPEN_EXISTING,0,NULL);
	if (hFile==INVALID_HANDLE_VALUE)
		return;

	DWORD dwSizeLow, dwSizeHigh;
	dwSizeLow=GetFileSize(hFile,&dwSizeHigh);
	TInt r = Emulator::LastError();
	if((NO_ERROR != r) && (INVALID_FILE_SIZE == dwSizeLow))
		User::Leave(r);
	
	// Check on file size 
	const TInt64 fileSize = MAKE_TINT64(dwSizeHigh, dwSizeLow);
	if(MaxFileSizeSupported() < (TUint64)fileSize)
		{
		if (!CloseHandle(hFile))
			User::Leave(Emulator::LastError());
		
        User::Leave(KErrTooBig);
		}
	
//	Check that reading from aPos for aLength lies within the file
//	if aPos is within the file, and aLength is too long, read up to EOF
//	If aPos is beyond the file, return a zero length descriptor

	if (fileSize>=aPos+aLength)	//	Can read entire length requested from aPos	
		SetFilePointer64L(hFile,(LARGE_INTEGER *)&aPos,FILE_BEGIN);
	
	else if (fileSize>aPos)		//	Can read from aPos but not entire length requested
		{
		SetFilePointer64L(hFile,(LARGE_INTEGER *)&aPos,FILE_BEGIN);
		aLength=(TInt)(fileSize-aPos);
		}	
	else						//	Cannot read from aPos because it lies outside file
		{						//	Close file and leave with KErrEof
		if (!CloseHandle(hFile))
			User::Leave(Emulator::LastError());

		User::Leave(KErrEof);
		}

	TBuf8<0x1000> buf;
	TInt pos=0;

	if (aMessage.Handle() == KLocalMessageHandle)
		((TPtr8* )aTrg)->SetLength(0);
	
	while (aLength)
		{
		TInt readTotal=Min(aLength,buf.MaxLength());
		DWORD ret;
		BOOL b=ReadFile(hFile,(TAny*)buf.Ptr(),readTotal,&ret,NULL);
		if (!b || ((TInt)ret!=readTotal))	
			User::Leave(Emulator::LastError());
		buf.SetLength(ret);
		
		if(aMessage.Handle() == KLocalMessageHandle)
			((TPtr8* )aTrg)->Append(buf);
		else
			aMessage.WriteL(0,buf,pos);
		
		pos+=ret;
		if (((TInt)ret)<readTotal)
			break;
		aLength-=readTotal;
		}
	
	if (!CloseHandle(hFile))
		User::Leave(Emulator::LastError());
	}

//-------------------------------------------------------------------------------------------------------------------

/**
    CLocalMountCB control method.
    @param  aLevel  specifies the operation to perfrom on the mount
    @param  aOption specific option for the given operation
    @param  aParam  pointer to generic parameter, its meaning depends on aLevel and aOption

    @return standard error code.
*/

TInt CLocalMountCB::MountControl(TInt aLevel, TInt aOption, TAny* aParam)
    {
    //-- File System - specific queries 
    if(aLevel == EMountFsParamQuery && aOption == ESQ_GetMaxSupportedFileSize)
        {//-- this is a query to provide the max. supported file size; aParam is a pointer to TUint64 to return the value
        *(TUint64*)aParam = MaxFileSizeSupported();    
        return KErrNone;
        }

    return KErrNotSupported; 
    }

//#########################################################################################################################
//##        CLocalFileCB class implementation
//#########################################################################################################################


CLocalFileCB::CLocalFileCB()
	{
	}

CLocalFileCB::~CLocalFileCB()
	{

	if (iAtt&KEntryAttModified)
		{
		TRAPD(ret,FlushDataL());
//		if (ret!=KErrNone) // Can fail if floppy disk is removed
//			Panic(EFileClose); // Ignore error
		}
	if (iWinHandle!=NULL && !CloseHandle(iWinHandle))
		Panic(EFileClose);
	}

//-------------------------------------------------------------------------------------------------------------------	
//
// Returns ETrue if the drive number == EDriveZ
//
TBool CLocalFileCB::IsRomDrive() const
	{

	// WINS emulated rom drive is Z:
	return(((CLocalFileCB*)this)->Mount().Drive().DriveNumber()==EDriveZ);
	}

//-------------------------------------------------------------------------------------------------------------------	
//
//	Check that the file pointer iCurrentPos is positioned correctly
//	in relation to the Win32 file pointer
//
void CLocalFileCB::CheckPosL(TInt64 aPos)
	{
//	Get the current Win32 file pointer position	
	LARGE_INTEGER pos;
	pos.QuadPart = 0;
	DWORD position=SetFilePointer(iWinHandle,pos.LowPart,&pos.HighPart,FILE_CURRENT);
	TInt r = Emulator::LastError();
	if ((KInvalidSetFilePointer == position) && (r != NO_ERROR))
		User::Leave(r);
//	Set iCurrentPos and Win32 file pointers to aPos if they are different to each
//	other or different to aPos
	if ((pos.QuadPart!=iCurrentPos) || (iCurrentPos!=aPos))
		{
		iCurrentPos=(-1);
		pos.QuadPart = aPos;
		position = SetFilePointer(iWinHandle,pos.LowPart,&pos.HighPart,FILE_BEGIN);
		r = Emulator::LastError();
		if ((KInvalidSetFilePointer == position) && (r != NO_ERROR))
			User::Leave(r);
		iCurrentPos=aPos;
		}
	}

//-------------------------------------------------------------------------------------------------------------------	
void CLocalFileCB::ReadL(TInt aPos,TInt& aLength,const TAny* aDes,const RMessagePtr2& aMessage)
	{
	ReadL((TInt64)aPos, aLength, (TDes8*)aDes, aMessage, 0);
	}

//-------------------------------------------------------------------------------------------------------------------	
void CLocalFileCB::WriteL(TInt aPos,TInt& aLength,const TAny* aDes,const RMessagePtr2& aMessage)
	{
	WriteL((TInt64)aPos, aLength, (TDesC8*)aDes, aMessage, 0);
	}

struct SRomMap
	{
	HBufC* iName;
	TUint8* iAddr;
	};
//-------------------------------------------------------------------------------------------------------------------	

TInt CLocalFileCB::RomAddress(const TDesC& aName, HANDLE aFile, TUint8*& aAddr)
	{
	static CArrayFixSeg<SRomMap>* gRomMap = new CArrayFixSeg<SRomMap>(64);
	for (TInt ii=0; ii<gRomMap->Count(); ii++)
		{
		if (*gRomMap->At(ii).iName == aName)
			{
			aAddr = gRomMap->At(ii).iAddr;
			return KErrNone;
			}
		}

	HANDLE fileMapping=CreateFileMappingA(aFile,NULL,PAGE_READONLY,0,0,NULL);
	if (fileMapping==0)
		return Emulator::LastError();
	aAddr=(TUint8*)MapViewOfFile(fileMapping,FILE_MAP_READ,0,0,0);
	SRomMap entry;
	entry.iAddr = aAddr;
	entry.iName = aName.Alloc();
	if (entry.iName)
		{
		TRAPD(ignore, gRomMap->AppendL(entry));
		}
	return KErrNone;
	}

//-------------------------------------------------------------------------------------------------------------------	
//
// If ROM file, do a memory map and return the address
//
TInt CLocalFileCB::Address(TInt& aPos) const
	{

	TBool isRomFile=IsRomDrive();
	if (!isRomFile)
		return(KErrNotSupported);
	
	if (aPos>Size64())
		return(KErrEof);
	if (iFilePtr==NULL)
		{
		CLocalFileCB* This=(CLocalFileCB*)this;
		TInt err = RomAddress(*iFileName, iWinHandle, This->iFilePtr);
		if (err)
			return err;
		}
	aPos=(TInt)((TUint8*)iFilePtr+aPos);
	return(KErrNone);
	}

//-------------------------------------------------------------------------------------------------------------------	
//
// Set the file size.
//
void CLocalFileCB::SetSizeL(TInt aSize)
	{
	SetSizeL(aSize);
	}

//-------------------------------------------------------------------------------------------------------------------	
//
// Set the entry's attributes and modified time.
//
void CLocalFileCB::SetEntryL(const TTime& aTime,TUint aSetAttMask,TUint aClearAttMask)
	{

	if (IsRomDrive())
		User::Leave(KErrAccessDenied);
	TUint setAttMask=aSetAttMask&KEntryAttMaskSupported;
	if (setAttMask|aClearAttMask)
		{
		iAtt|=setAttMask;
		iAtt&=(~aClearAttMask);
		iAtt|=KEntryAttModified;
		}
	if (aSetAttMask&KEntryAttModified)
		iModified=aTime;
	iAtt|=KEntryAttModified;
	}

//-------------------------------------------------------------------------------------------------------------------	
//
// Commit any buffered date to the media.
//
void CLocalFileCB::FlushAllL()
	{
	FlushDataL();
	}

//-------------------------------------------------------------------------------------------------------------------	
//
// Commit any buffered date to the media.
//
void CLocalFileCB::FlushDataL()
	{

	if (IsRomDrive())
		return;

	TFileName n;
	TInt driveNumber=Mount().Drive().DriveNumber();
	MapFileNameL(n,driveNumber,FileName());
	
    if(!Emulator::SetFileAttributes(StrPtrZL(n),iAtt&KEntryAttMaskSupported))
		User::Leave(Emulator::LastError()); //	Panic(EFileCloseSetAttributes);
	FILETIME f;
	timeToFileTimeL(iModified,&f);
	if (!SetFileTime(iWinHandle,&f,&f,&f))
		User::Leave(Emulator::LastError());

	iAtt&=(~KEntryAttModified);
	}

//-------------------------------------------------------------------------------------------------------------------	
//
// Rename the file while open
//
void CLocalFileCB::RenameL(const TDesC& aNewName)
	{

	TInt driveNumber=Mount().Drive().DriveNumber();

	TFileName n1;
	MapFileNameL(n1,driveNumber,FileName());
	TFileName n2;
	MapFileNameL(n2,driveNumber,aNewName);

	CloseHandle(iWinHandle);
	TInt ret=KErrNone;
	if (!Emulator::MoveFile(StrPtrZL(n1),StrPtrZL(n2)))
		{
		ret=Emulator::LastError();
		n2=n1;
		}
	DWORD access=GENERIC_READ|GENERIC_WRITE;
	DWORD share=FILE_SHARE_WRITE|FILE_SHARE_READ;
	DWORD create=OPEN_EXISTING;
	iWinHandle=Emulator::CreateFile(StrPtrZL(n2),access,share,NULL,create,FILE_FLAG_RANDOM_ACCESS,NULL);
	if (iWinHandle==INVALID_HANDLE_VALUE)
		User::Leave(Emulator::LastError());
	
	LARGE_INTEGER pos;
	pos.QuadPart = iCurrentPos;
	DWORD position = SetFilePointer(iWinHandle,pos.LowPart,&pos.HighPart,FILE_BEGIN);
	TInt r = Emulator::LastError();
	if ((KInvalidSetFilePointer == position) && (r != NO_ERROR))
		User::Leave(r);	
	
	User::LeaveIfError(ret);
	AllocBufferL(iFileName,aNewName);
	}

//-------------------------------------------------------------------------------------------------------------------	
TInt CLocalFileCB::GetInterface(TInt aInterfaceId,TAny*& aInterface,TAny* aInput)
	{
	switch(aInterfaceId)
		{
		case EExtendedFileInterface:
			((CFileCB::MExtendedFileInterface*&) aInterface) = this;
			return KErrNone;

		default:
			return CFileCB::GetInterface(aInterfaceId,aInterface,aInput);
		}
	}

//-------------------------------------------------------------------------------------------------------------------	
/**
    Read data from the file.
    
    @param  aFilePos    start read position within a file
    @param  aLength     how many bytes to read; on return will be how many bytes actually read
    @param  aDes        local buffer desctriptor
    @param  aMessage    from file server, used to write data to the buffer in different address space.
    @param  aDesOffset  offset within data descriptor where the data will be copied

    @leave on media read error

*/
void CLocalFileCB::ReadL(TInt64 aPos,TInt& aLength,TDes8* aDes,const RMessagePtr2& aMessage, TInt aOffset)
	{

	const TUint64 KMaxFilePosition = LocalMount().MaxFileSizeSupported()-1;
    

    if(KMaxFilePosition < (TUint64)aPos)
		User::Leave(KErrNotSupported);
	
	CheckPosL(aPos);
	TInt pos=0;
	TInt len=aLength;
	TBuf8<65536> buf;

	if (aMessage.Handle() == KLocalMessageHandle)
		((TPtr8* )aDes)->SetLength(0);

	while (len)
		{
		TInt s=Min(len,buf.MaxLength());
		DWORD res;
		BOOL b=ReadFile(iWinHandle,(TAny*)buf.Ptr(),s,&res,NULL);
		if(!b)
			User::Leave(Emulator::LastError());

		buf.SetLength(res);

	if (aMessage.Handle() == KLocalMessageHandle)
		((TPtr8* )aDes)->Append(buf);
	else
		aMessage.WriteL(0,buf,pos + aOffset);
	
		pos+=res;
		if (((TInt)res)<s)
			break;
		len-=s;
		}
	TInt delay = (ReadSpeed * aLength) >> 10;
	if (delay)
		User::AfterHighRes(delay);
	aLength=pos;
	iCurrentPos=aPos+pos;
	}

//-------------------------------------------------------------------------------------------------------------------	
/**
    Write data to the file.
    
    @param  aFilePos    start write position within a file
    @param  aLength     how many bytes to write; on return contain amount of data actually written
    @param  aDes        local buffer desctriptor
    @param  aMessage    from file server, used to write data to the media from different address space.
    @param  aDesOffset  offset within data descriptor 

    @leave on media read error

*/
void CLocalFileCB::WriteL(TInt64 aPos,TInt& aLength,const TDesC8* aDes,const RMessagePtr2& aMessage, TInt aOffset)
	{
	if (IsRomDrive())
		User::Leave(KErrAccessDenied);
	
	
    const TUint64 KMaxFileSize = LocalMount().MaxFileSizeSupported();
    const TUint64 KMaxFilePosition = KMaxFileSize - 1;

	if( KMaxFilePosition < (TUint64)aPos || KMaxFileSize < (TUint64)(aPos + aLength) )
		User::Leave(KErrNotSupported);
	
	CheckPosL(aPos);
	TInt pos=0;
	TInt len=aLength;
	TBuf8<65536> buf;

	while (len)
		{
		TInt s=Min(len,buf.MaxLength());

		if (aMessage.Handle() == KLocalMessageHandle)
			buf.Copy( ((TPtr8* )aDes)->MidTPtr(pos, s) );
		else
			aMessage.ReadL(0,buf,pos + aOffset);

		DWORD res;
		BOOL b=WriteFile(iWinHandle,buf.Ptr(),s,&res,NULL);
		
        if (!b)
			User::Leave(Emulator::LastError());

		if (((TInt)res)<s)
			User::Leave(KErrCorrupt);
		
        len-=s;
		pos+=s;
		}
	TInt delay = (WriteSpeed * aLength) >> 10;
	if (delay)
		User::AfterHighRes(delay);
	aLength=pos;
	iCurrentPos=aPos+pos;
	}

//-------------------------------------------------------------------------------------------------------------------	
/**
    Set file size.
    @param aSize new file size.
*/
void CLocalFileCB::SetSizeL(TInt64 aSize)
	{
    const TUint64 KMaxFileSize = LocalMount().MaxFileSizeSupported();

	if(KMaxFileSize < (TUint64)aSize)
		User::Leave(KErrNotSupported);
	
	CheckPosL(aSize);
	if(!SetEndOfFile(iWinHandle))
		{
		iCurrentPos= -1;
		User::Leave(Emulator::LastError());
		}

	SetSize64(aSize, EFalse);
	}

//#########################################################################################################################
//##        CLocalDirCB class implementation
//#########################################################################################################################

CLocalDirCB::CLocalDirCB()
	        :iEntry()
	{
	}

CLocalDirCB::~CLocalDirCB()
	{

	if (iWinHandle!=NULL && !FindClose(iWinHandle))
		Panic(EDirClose);
	}

//-------------------------------------------------------------------------------------------------------------------	
TBool CLocalDirCB::MatchUid()
	{

	if (iUidType[0]!=TUid::Null() || iUidType[1]!=TUid::Null() || iUidType[2]!=TUid::Null())
		return(ETrue);
	
    return(EFalse);
	}

//-------------------------------------------------------------------------------------------------------------------	
/** @return  ETrue if the aUidTrg matches aUidSuitor */
static TBool CompareUid(const TUidType& aUidTrg, const TUidType& aUidSuitor)
	{
	
	if (aUidTrg[0]!=TUid::Null() && aUidTrg[0]!=aUidSuitor[0])
		return(EFalse);
	if (aUidTrg[1]!=TUid::Null() && aUidTrg[1]!=aUidSuitor[1])
		return(EFalse);
	if (aUidTrg[2]!=TUid::Null() && aUidTrg[2]!=aUidSuitor[2])
		return(EFalse);
	return(ETrue);
	}

//-------------------------------------------------------------------------------------------------------------------	
/**
    Read current entry from the directory and move to the next one.
    This function must leave KErrEof when the end of directory is reached

    @param anEntry extracted directory entry
    @leave KErrEof when there are no more entries in the directory
           system-wide error code on media read fault.

*/
void CLocalDirCB::ReadL(TEntry& anEntry)
	{

	if (iWinHandle==NULL)
		User::Leave(KErrEof);

	FOREVER
		{
		if (!iPending)
			{
			WIN32_FIND_DATA info;
			if (!Emulator::FindNextFile(iWinHandle,&info))
				User::Leave(Emulator::LastError());

			iEntry.iName.Des()=(TText*)(&info.cFileName[0]);
			iEntry.iAtt=info.dwFileAttributes&KEntryAttMaskSupported;
			iEntry.SetFileSize(MAKE_TINT64(info.nFileSizeHigh,info.nFileSizeLow));
			fileTimeToTime(&info.ftLastWriteTime,iEntry.iModified);
			}
		iPending=EFalse;
		anEntry=iEntry;
		if (anEntry.iName==_L(".") || anEntry.iName==_L(".."))
			continue;
		if ((iFullName.NameAndExt()==_L("*.*") || iFullName.NameAndExt()==_L("*") || anEntry.iName.MatchF(iFullName.NameAndExt())!=KErrNotFound) && Mount().MatchEntryAtt(anEntry.iAtt&KEntryAttMaskSupported,iAtt))
			{
			if (MatchUid())
				{
				TParse fileName;
				TBuf<KMaxFileName> driveAndPath=iFullName.DriveAndPath();
				fileName.Set(anEntry.iName,&driveAndPath,NULL);
				(*(CLocalMountCB*)&Mount()).ReadUidL(fileName.FullName(),anEntry);
				if (CompareUid(iUidType,anEntry.iType))
					break;
				}
			else
				break;
			}
		}
	if ((iAtt&KEntryAttAllowUid)==0 || anEntry.iAtt&KEntryAttDir || MatchUid())
		return;
	TParse fileName;
	TBuf<KMaxFileName> driveAndPath=iFullName.DriveAndPath();
	fileName.Set(anEntry.iName,&driveAndPath,NULL);
	(*(CLocalMountCB*)&Mount()).ReadUidL(fileName.FullName(),anEntry);
	}

//#########################################################################################################################
//##        CLocalFormatCB class implementation
//#########################################################################################################################

CLocalFormatCB::CLocalFormatCB()
	{
	}

CLocalFormatCB::~CLocalFormatCB()
	{
	}

void CLocalFormatCB::DoFormatStepL()
	{
	iCurrentStep=0;
	User::Leave(KErrNotSupported);
	}


//#########################################################################################################################
//##        CLocal File System class implementation
//#########################################################################################################################

extern "C" 
{
//
// Create a new file system
//
EXPORT_C CFileSystem* CreateFileSystem()
	{
	return(new CLocal);
	}
}

CLocal::CLocal()
	{
	}

TInt CLocal::Install()
	{

	SetErrorMode(SEM_FAILCRITICALERRORS);
	EmulatorDiskSpeed(ReadSpeed, WriteSpeed);
	iVersion=TVersion(KMajorVersionNumber,KMinorVersionNumber,KF32BuildVersionNumber);
	_LIT(KWin32Name,"Win32");
	return(SetName(&KWin32Name));
	}

CMountCB* CLocal::NewMountL() const
//
// Create a new mount control block.
//
	{

	return(new(ELeave) CLocalMountCB);
	}

CFileCB* CLocal::NewFileL() const
//
// Create a new file.
//
	{

	return(new(ELeave) CLocalFileCB);
	}

CDirCB* CLocal::NewDirL() const
//
// Create a new directory lister.
//
	{
	return(new(ELeave) CLocalDirCB);
	}

CFormatCB* CLocal::NewFormatL() const
//
// Create a new media formatter.
//
	{
	return(new(ELeave) CLocalFormatCB);
	}

TInt CLocal::DefaultPath(TDes& aPath) const
//
// Return the initial default path.
//
	{
	aPath=_L("?:\\");
	aPath[0] = (TUint8) RFs::GetSystemDriveChar();
	return(KErrNone);
	}

void CLocal::DriveInfo(TDriveInfo& anInfo,TInt aDriveNumber) const
//
// Return the drive info.
//
	{

	anInfo.iMediaAtt=0;

// Get Fake drive info.
	if (aDriveNumber==EDriveZ)
		{
		anInfo.iType=EMediaRom;
		anInfo.iMediaAtt=KMediaAttWriteProtected;
		anInfo.iDriveAtt=KDriveAttRom|KDriveAttInternal;
		anInfo.iConnectionBusType=EConnectionBusInternal;
		return;
		}
	if (aDriveNumber==EDriveC)
		{
		anInfo.iType=EMediaHardDisk;
		anInfo.iMediaAtt=KMediaAttVariableSize;
		anInfo.iDriveAtt=KDriveAttLocal|KDriveAttInternal;
		anInfo.iConnectionBusType=EConnectionBusInternal;
		return;
		}
	TFileName envValue;
	if (MapDrive(envValue,aDriveNumber))
		{
		anInfo.iType=EMediaHardDisk;
		anInfo.iDriveAtt=KDriveAttLocal|KDriveAttInternal;
		anInfo.iConnectionBusType=EConnectionBusInternal;
		return;		
		}
	anInfo.iType=EMediaNotPresent;
	anInfo.iDriveAtt=0;
	}















