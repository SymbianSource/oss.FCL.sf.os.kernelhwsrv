// Copyright (c) 1998-2009 Nokia Corporation and/or its subsidiary(-ies).
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

#define _CRTIMP			// we want to use the static runtime library
#include <e32cmn.h>
#include <e32cmn_private.h>
#include <string.h>
#include <emulator.h>
#include <e32ldr.h>
#include <e32ldr_private.h>
#include <e32uid.h>

#pragma data_seg(".data2")
#ifdef __VC32__
#pragma bss_seg(".data2")
#endif
static TBool UnicodeHost;
static Emulator::SInit Data;
#pragma data_seg()
#ifdef __VC32__
#pragma bss_seg()
#endif

/**	
Initializes a module and prepares it to handle requests.

@param aInit	An object of the structure SInit.

@see SInit.
*/
EXPORT_C void Emulator::Init(const Emulator::SInit& aInit)
	{
	UnicodeHost = (GetVersion() & 0x80000000) ? FALSE : TRUE;
	Data = aInit;
	if (Data.iCodePage == 0)
		Data.iCodePage = CP_ACP;
	}

LOCAL_C DWORD UStringLength(LPCSTR lpMultiByteStr)
//
// returns length of unicode string generated - assumes null terminated string
//
	{
	return MultiByteToWideChar(Data.iCodePage,0,lpMultiByteStr,-1,0,0);
	}


LOCAL_C DWORD ConvertToUnicode(LPCSTR aNarrow,LPWSTR aUnicode,DWORD aLength,BOOL aCharsRequired)
//
// Converts narrow string to unicode string
//
	{
	DWORD uniLength=UStringLength(aNarrow);
	if(uniLength>aLength || uniLength==0)
		return aCharsRequired ? uniLength : 0;
	uniLength=MultiByteToWideChar(Data.iCodePage,0,aNarrow,-1,aUnicode,aLength);
	// return number of characters excluding the null terminator
	return uniLength ? uniLength-1 : 0;
	}


LOCAL_C DWORD ConvertToNarrow(LPCWSTR aUnicode,LPSTR aNarrow,DWORD aLength)
//
// Converts unicode string to narrow string
//
	{
	return WideCharToMultiByte(Data.iCodePage,0,aUnicode,-1,aNarrow,aLength,"@",NULL);
	}

LOCAL_C TInt GetSectionsSize(const IMAGE_NT_HEADERS32* aBase)
//
// Iterates through the section headers at the start of an image and determines the minimum amount of the
// PE file that must be read in in order to examine the sections themselves
//
	{
    // List of sections are are interested in examining
	const BYTE* sectionNames[] =
			{
			KWin32SectionName_Symbian, KWin32SectionName_Import, KWin32SectionName_EpocData, KWin32SectionName_EpocBss,
			KWin32SectionName_Text, KWin32SectionName_RData, KWin32SectionName_NmdExpData
			};

	// Get a ptr to the first section header in preparation for examining all the section headers
	DWORD maxOffset = 0;
	const IMAGE_NT_HEADERS32* ntHead = aBase;
	const IMAGE_SECTION_HEADER* imgHead = (const IMAGE_SECTION_HEADER*) ((TUint8*) &ntHead->OptionalHeader + ntHead->FileHeader.SizeOfOptionalHeader);
	const IMAGE_SECTION_HEADER* end = imgHead + ntHead->FileHeader.NumberOfSections;

	for (; imgHead < end; ++imgHead)
		{
		TBool SectionUsed = EFalse;

		// Go through each of the sections that we need to examine and see if the current section is in the list
		for (TInt index = 0; index < (sizeof(sectionNames) / sizeof(BYTE*)); ++index)
			{
			if (memcmp(imgHead->Name, sectionNames[index], IMAGE_SIZEOF_SHORT_NAME)==0)
				SectionUsed = ETrue;
			}

		// If the current section is one we are interested in, calculate its offset in the raw file and add its size;
		// this gives us the minimum amount of the file to be mapped in order to examine this section and all those
		// preceding it
		if (SectionUsed)
			{
			if ((imgHead->PointerToRawData + imgHead->SizeOfRawData) > maxOffset)
				{
				maxOffset = (imgHead->PointerToRawData + imgHead->SizeOfRawData);
				}
			}
		}

	return(maxOffset);
	}

template <TUint S>
struct Buf8
	{
public:
	Buf8(LPCWSTR aUnicode);
	inline operator LPCSTR() const
		{return iPtr;}
private:
	const char* iPtr;
	char iBuf[S];
	};

template <TUint S>
Buf8<S>::Buf8(LPCWSTR aUnicode)
	{
	if (aUnicode)
		{
		iPtr = iBuf;
		ConvertToNarrow(aUnicode,iBuf,S);
		}
	else
		{
		iPtr = NULL;
		}
	}



/**	
Acquires the global lock for host interaction.
*/
EXPORT_C void Emulator::Lock()
	{
	Data.iLock();
	}

/**
Releases the global lock for host interaction.
This may overwrite the error code for this thread and that is not the behaviour expected, 
so save and restore it.
*/
EXPORT_C void Emulator::Unlock()
	{
	DWORD error=GetLastError();
	Data.iUnlock();
	SetLastError(error);
	}

/**
Takes the current thread out of the emulator scheduling model.
*/
EXPORT_C void Emulator::Escape()
	{
	Data.iEscape();
	}

/**
Returns the calling thread into the emulator scheduling model.
This may overwrite the error code for this thread and that is not the behaviour expected, 
so save and restore it.
*/
EXPORT_C void Emulator::Reenter()
	{
	DWORD error=GetLastError();
	Data.iReenter();
	SetLastError(error);
	}


/**
Jumps to the NThread Win32 SEH exception handler.

@param aException	Describes an exception.
@param aContext		Describes the context.
	
@return   A handler to handle the exception occurred
*/
EXPORT_C DWORD Emulator::Win32SEHException(EXCEPTION_RECORD* aException, CONTEXT* aContext)
	{
	return Data.iException(aException, aContext);
	}


/**
This function wraps the Win32 CreateDirectory API (see http://msdn2.microsoft.com/en-us/library/aa363855.aspx).
*/  
EXPORT_C BOOL Emulator::CreateDirectory(LPCWSTR lpPathName,LPSECURITY_ATTRIBUTES lpSecurityAttributes)
	{
	__LOCK_HOST;

	if (UnicodeHost)
		return ::CreateDirectoryW(lpPathName, lpSecurityAttributes);

	return ::CreateDirectoryA(Buf8<MAX_PATH>(lpPathName),lpSecurityAttributes);
	}

#ifdef __VC32__
//disable unreachable code warning in VC++
#pragma warning (disable : 4702)
#endif


/**
Recursively ensures that the full directory exists.
	
@param aPathName  	Provides the path name.

@return  TRUE, if the function succeeds
		 FALSE, if the function fails
*/
EXPORT_C BOOL Emulator::CreateAllDirectories(LPCSTR aPathName)
	{
	__LOCK_HOST;

	char path[MAX_PATH];
	strcpy(path, aPathName);
	char* p = path;
	for (;;)
		{
		p = strchr(p, '\\');
		char temp=0;
		if (p)
			{
			temp = *++p;
			*p = '\0';
			}
		DWORD att = ::GetFileAttributesA(path);
		if ((att == -1 || !(att&FILE_ATTRIBUTE_DIRECTORY)) && !::CreateDirectoryA(path, NULL))
			return EFalse;
		if (!p)
			return ETrue;
		*p = temp;
		}
	}
#ifdef __VC32__
//enable unreachable code warning in VC++
#pragma warning (default : 4702)
#endif

#ifndef INVALID_FILE_ATTRIBUTES	
#define INVALID_FILE_ATTRIBUTES	 ((DWORD)-1)
#endif


/**
This function wraps the Win32 CreateFile API (see http://msdn2.microsoft.com/en-us/library/aa363858.aspx).
It also modifies file attributes depending on whether the file is hidden or not.
*/
EXPORT_C HANDLE Emulator::CreateFile(LPCWSTR lpFileName,DWORD dwDesiredAccess,DWORD dwShareMode,
						   LPSECURITY_ATTRIBUTES lpSecurityAttributes,DWORD dwCreationDistribution,
						   DWORD dwFlagsAndAttributes,HANDLE hTemplateFile)
	{
	__LOCK_HOST;
	HANDLE h;
	BOOL hidden = 0; 
	DWORD att = INVALID_FILE_ATTRIBUTES;
	if(dwCreationDistribution==CREATE_ALWAYS)
		{
		att = ::GetFileAttributes(lpFileName);
		if(att!=INVALID_FILE_ATTRIBUTES && (att&FILE_ATTRIBUTE_HIDDEN))
			{
			hidden = ::SetFileAttributes(lpFileName, (att&~FILE_ATTRIBUTE_HIDDEN));
			}
		}
		
	if (UnicodeHost)
		h = ::CreateFileW(lpFileName,dwDesiredAccess,dwShareMode,lpSecurityAttributes,dwCreationDistribution,
							dwFlagsAndAttributes,hTemplateFile);
	else
		h = ::CreateFileA(Buf8<MAX_PATH>(lpFileName),dwDesiredAccess,dwShareMode,lpSecurityAttributes,dwCreationDistribution,
						dwFlagsAndAttributes,hTemplateFile);
	
	if(hidden && h!=INVALID_HANDLE_VALUE)
		{
		::SetFileAttributes(lpFileName, att);
		}
	return h;
	}


/**
This function wraps the Win32 DeleteFile API (see http://msdn2.microsoft.com/en-us/library/aa363915.aspx).
*/
EXPORT_C BOOL Emulator::DeleteFile(LPCWSTR lpFileName)
	{
	__LOCK_HOST;

	if (UnicodeHost)
		return ::DeleteFileW(lpFileName);

	return ::DeleteFileA(Buf8<MAX_PATH>(lpFileName));
	}

LOCAL_C TBool MapFindFileData(LPWIN32_FIND_DATAW lpFindFileData, LPWIN32_FIND_DATAA lpFindNarrow)
	{
	lpFindFileData->dwFileAttributes=lpFindNarrow->dwFileAttributes;
	lpFindFileData->ftCreationTime=lpFindNarrow->ftCreationTime;
	lpFindFileData->ftLastAccessTime=lpFindNarrow->ftLastAccessTime;
	lpFindFileData->ftLastWriteTime=lpFindNarrow->ftLastWriteTime;
	lpFindFileData->nFileSizeHigh=lpFindNarrow->nFileSizeHigh;
	lpFindFileData->nFileSizeLow=lpFindNarrow->nFileSizeLow;
	
	if(!MultiByteToWideChar(Data.iCodePage,0,lpFindNarrow->cFileName,-1,lpFindFileData->cFileName,MAX_PATH))
		return FALSE;

	if(lpFindNarrow->cAlternateFileName!=NULL)
		{
		// magic number 14 comes from MS documentation
		if(!MultiByteToWideChar(Data.iCodePage,0,lpFindNarrow->cAlternateFileName,-1,lpFindFileData->cAlternateFileName,14))
			return FALSE;
		}
	return TRUE;
	}


/**
This function wraps the Win32 FindFirstFile API (see http://msdn2.microsoft.com/en-us/library/aa364418.aspx).	
*/
EXPORT_C HANDLE Emulator::FindFirstFile(LPCWSTR lpFileName,LPWIN32_FIND_DATAW lpFindFileData)
	{
	__LOCK_HOST;

	if (UnicodeHost)
		return ::FindFirstFileW(lpFileName, lpFindFileData);

	WIN32_FIND_DATAA lpFindNarrow;
	HANDLE h=::FindFirstFileA(Buf8<MAX_PATH>(lpFileName),&lpFindNarrow);
	if(h==INVALID_HANDLE_VALUE)
		return h;

	if (!MapFindFileData(lpFindFileData, &lpFindNarrow))
		{
		FindClose(h);
		return INVALID_HANDLE_VALUE;
		}

	return h;
	}


/**
This function wraps the Win32 FindNextFile API (see http://msdn2.microsoft.com/en-us/library/aa364428.aspx).	
*/
EXPORT_C BOOL Emulator::FindNextFile(HANDLE hFindFile,LPWIN32_FIND_DATAW lpFindFileData)
	{
	__LOCK_HOST;

	if (UnicodeHost)
		return ::FindNextFileW(hFindFile, lpFindFileData);

	WIN32_FIND_DATAA lpFindNarrow;
	if(!::FindNextFileA(hFindFile,&lpFindNarrow))
		return FALSE;

	return MapFindFileData(lpFindFileData, &lpFindNarrow);
	}

/**
This function wraps the Win32 GetDiskFreeSpace API (see http://msdn2.microsoft.com/en-us/library/aa364935.aspx).
*/
EXPORT_C BOOL Emulator::GetDiskFreeSpace(LPCWSTR lpRootPathName,LPDWORD lpSectorsPerCluster,\
							   LPDWORD lpBytesPerSector,LPDWORD lpNumberOfFreeClusters,\
							   LPDWORD lpTotalNumberOfClusters)
	{
	__LOCK_HOST;

	if (UnicodeHost)
		return ::GetDiskFreeSpaceW(lpRootPathName,lpSectorsPerCluster,lpBytesPerSector,lpNumberOfFreeClusters,lpTotalNumberOfClusters);

	return ::GetDiskFreeSpaceA(Buf8<MAX_PATH>(lpRootPathName),lpSectorsPerCluster,lpBytesPerSector,lpNumberOfFreeClusters,lpTotalNumberOfClusters);

	}


/**
This function wraps the Win32 GetFileAttributes API (see http://msdn2.microsoft.com/en-us/library/aa364944.aspx).
*/
EXPORT_C DWORD Emulator::GetFileAttributes(LPCWSTR lpFileName)
	{
	__LOCK_HOST;

	if (UnicodeHost)
		return ::GetFileAttributesW(lpFileName);

	return ::GetFileAttributesA(Buf8<MAX_PATH>(lpFileName));
	}


/**
This function wraps the Win32 GetModuleHandle API (see http://msdn2.microsoft.com/en-us/library/ms683199.aspx).
*/
EXPORT_C HMODULE Emulator::GetModuleHandle(LPCWSTR lpModuleName)
	{
	__LOCK_HOST;

	if (UnicodeHost)
		return ::GetModuleHandleW(lpModuleName);

	return ::GetModuleHandleA(Buf8<MAX_PATH>(lpModuleName));
	}


/**
This function wraps the Win32 GetModuleFileName API (see http://msdn2.microsoft.com/en-us/library/ms683197.aspx).
*/
EXPORT_C DWORD Emulator::GetModuleFileName(HMODULE hModule, LPWSTR lpFilename)
	{
	__LOCK_HOST;
	if (UnicodeHost)
		return ::GetModuleFileNameW(hModule, lpFilename, MAX_PATH);
	char fn[MAX_PATH];
	DWORD r=::GetModuleFileNameA(hModule, fn, MAX_PATH);
	if (r>MAX_PATH||r==0)
		return 0;
	return ConvertToUnicode(fn, lpFilename, MAX_PATH, TRUE);
	}


/**
This function wraps the Win32 GetTempPath API (see http://msdn2.microsoft.com/en-us/library/Aa364992.aspx).
*/
EXPORT_C DWORD Emulator::GetTempPath(DWORD nBufferLength,LPWSTR lpBuff)
	{
	__LOCK_HOST;

	if (UnicodeHost)
		return ::GetTempPathW(nBufferLength,lpBuff);

	char path[MAX_PATH];
	DWORD r=::GetTempPathA(MAX_PATH,path);
	if(r>MAX_PATH||r==0)
		return 0;
	return ConvertToUnicode(path,lpBuff,nBufferLength,TRUE);
	}


/**
This function wraps the Win32 GetCurrentDirectory API (see http://msdn2.microsoft.com/en-us/library/aa364934.aspx).
*/
EXPORT_C DWORD Emulator::GetCurrentDirectory(DWORD nBufferLength,LPWSTR lpBuff)
	{
	__LOCK_HOST;

	if (UnicodeHost)
		return ::GetCurrentDirectoryW(nBufferLength,lpBuff);

	char path[MAX_PATH];
	DWORD r=::GetCurrentDirectoryA(MAX_PATH,path);
	if(r>MAX_PATH||r==0)
		return 0;
	return ConvertToUnicode(path,lpBuff,nBufferLength,TRUE);
	}


/**
This function wraps the Win32 GetVolumeInformation API (see http://msdn2.microsoft.com/en-us/library/aa364993.aspx).
*/
EXPORT_C BOOL Emulator::GetVolumeInformation(LPCWSTR lpRootPathName,LPWSTR lpVolumeNameBuffer,DWORD nVolumeNameSize,
								   LPDWORD lpVolumeSerialNumber,LPDWORD lpMaximumComponentLength,
								   LPDWORD lpFileSystemFlags,LPWSTR,DWORD)
	{
	__LOCK_HOST;

	// lpfileSystemNameBuffer always NULL so no need to convert
	if (UnicodeHost)
		return ::GetVolumeInformationW(lpRootPathName,lpVolumeNameBuffer,nVolumeNameSize,lpVolumeSerialNumber,
										lpMaximumComponentLength,lpFileSystemFlags,NULL,0);

	char volName[MAX_PATH];
	BOOL res=::GetVolumeInformationA(Buf8<MAX_PATH>(lpRootPathName),volName,MAX_PATH,lpVolumeSerialNumber,
									lpMaximumComponentLength,lpFileSystemFlags,NULL,0);

	if(res && lpVolumeNameBuffer)
		ConvertToUnicode(volName,lpVolumeNameBuffer,nVolumeNameSize,FALSE);
	return res;
	}


/**
This function wraps the Win32 LoadLibrary API (see http://msdn2.microsoft.com/en-us/library/ms684175.aspx).
*/
EXPORT_C HMODULE Emulator::LoadLibrary(LPCWSTR lpLibFileName)
	{
	__LOCK_HOST;

	if (UnicodeHost)
		return ::LoadLibraryW(lpLibFileName);

	return ::LoadLibraryA(Buf8<MAX_PATH>(lpLibFileName));
	}


/**
This function wraps the Win32 FreeLibrary API (see http://msdn2.microsoft.com/en-us/library/ms683152.aspx).
*/
EXPORT_C BOOL Emulator::FreeLibrary(HMODULE hLibModule)
	{
	__LOCK_HOST;
	return ::FreeLibrary(hLibModule);
	}



/**
This function wraps the Win32 MoveFile API (see http://msdn2.microsoft.com/en-us/library/aa365239.aspx).
*/
EXPORT_C BOOL Emulator::MoveFile(LPCWSTR lpExistingFileName,LPCWSTR lpNewFileName)
	{
	__LOCK_HOST;

	if (UnicodeHost)
		return ::MoveFileW(lpExistingFileName,lpNewFileName);

	return ::MoveFileA(Buf8<MAX_PATH>(lpExistingFileName),Buf8<MAX_PATH>(lpNewFileName));
	}


/**
This function wraps the Win32 CopyFile API (see http://msdn2.microsoft.com/en-us/library/aa363851.aspx).
*/
EXPORT_C BOOL Emulator::CopyFile(LPCWSTR lpExistingFileName,LPCWSTR lpNewFileName,BOOL aFailIfExists)
	{
	__LOCK_HOST;

	if (UnicodeHost)
		return ::CopyFileW(lpExistingFileName,lpNewFileName,aFailIfExists);

	return ::CopyFileA(Buf8<MAX_PATH>(lpExistingFileName),Buf8<MAX_PATH>(lpNewFileName),aFailIfExists);
	}


/**
This function wraps the Win32 OutputDebugString API (see http://msdn2.microsoft.com/en-us/library/aa363362.aspx).
*/
EXPORT_C VOID Emulator::OutputDebugString(LPCWSTR lpOutputString)
	{
	if (UnicodeHost)
		::OutputDebugStringW(lpOutputString);
	else
		::OutputDebugStringA(Buf8<1024>(lpOutputString));
	}



/**
This function wraps the Win32 RemoveDirectory API (see http://msdn2.microsoft.com/en-us/library/aa365488.aspx).
*/
EXPORT_C BOOL Emulator::RemoveDirectory(LPCWSTR lpPathName)
	{
	__LOCK_HOST;

	if (UnicodeHost)
		return ::RemoveDirectoryW(lpPathName);

	return ::RemoveDirectoryA(Buf8<MAX_PATH>(lpPathName));
	}


/**
This function wraps the Win32 SetFileAttributes API (see http://msdn2.microsoft.com/en-us/library/aa365535.aspx).
*/

EXPORT_C BOOL Emulator::SetFileAttributes(LPCWSTR lpFileName,DWORD dwFileAttributes)
	{
	__LOCK_HOST;

	if (UnicodeHost)
		return ::SetFileAttributesW(lpFileName,dwFileAttributes);

	return ::SetFileAttributesA(Buf8<MAX_PATH>(lpFileName),dwFileAttributes);
	}



/**
This function wraps the Win32 SetVolumeLabel API (see http://msdn2.microsoft.com/en-us/library/aa365560.aspx).
*/
EXPORT_C BOOL Emulator::SetVolumeLabel(LPCWSTR lpRootPathName,LPCWSTR lpVolumeName)
	{
	__LOCK_HOST;

	if (UnicodeHost)
		return ::SetVolumeLabelW(lpRootPathName,lpVolumeName);

	return ::SetVolumeLabelA(Buf8<MAX_PATH>(lpRootPathName),Buf8<MAX_PATH>(lpVolumeName));
	}


/**
Maps an NT error to an Epoc32 error.
	
@return	 Last-error code of the calling thread.
*/
EXPORT_C TInt Emulator::LastError()

// For other error codes look at MSDN "Numerical List of Error Codes"
	{
	switch (GetLastError())
		{
	case ERROR_SUCCESS:				return KErrNone;
	case ERROR_INVALID_DRIVE:		return KErrNotReady;
	case ERROR_INVALID_NAME:
	case ERROR_FILENAME_EXCED_RANGE:
	case ERROR_OPEN_FAILED:			return KErrBadName; 
	case ERROR_INVALID_HANDLE:		return KErrBadHandle;
	case ERROR_NOT_SUPPORTED:
	case ERROR_INVALID_FUNCTION:	return KErrNotSupported;
	case ERROR_SHARING_VIOLATION: 
	case ERROR_ACCESS_DENIED: 
	case ERROR_WRITE_PROTECT:		return KErrAccessDenied;
	case ERROR_LOCK_VIOLATION:		return KErrLocked;
	case ERROR_FILE_NOT_FOUND:
	case ERROR_MOD_NOT_FOUND:		return KErrNotFound;
	case ERROR_DIRECTORY: 
	case ERROR_BAD_PATHNAME:
	case ERROR_PATH_NOT_FOUND:		return KErrPathNotFound; 
	case ERROR_ALREADY_EXISTS:
	case ERROR_FILE_EXISTS:			return KErrAlreadyExists;
	case ERROR_NOT_READY:			return KErrNotReady; 
	case ERROR_UNRECOGNIZED_VOLUME:
	case ERROR_NOT_DOS_DISK:		return KErrUnknown;
	case ERROR_UNRECOGNIZED_MEDIA:
	case ERROR_BAD_EXE_FORMAT:		return KErrCorrupt;
	case ERROR_NO_MORE_FILES:		return KErrEof; 
	case ERROR_DIR_NOT_EMPTY:		return KErrInUse;
	case ERROR_INVALID_USER_BUFFER:
	case ERROR_NOT_ENOUGH_MEMORY:
	case ERROR_INSUFFICIENT_BUFFER:
	case ERROR_OUTOFMEMORY:			return KErrNoMemory;
	case ERROR_DISK_FULL:			return KErrDiskFull;
	case ERROR_INVALID_DATA:
	case ERROR_INVALID_PARAMETER:	return KErrArgument;
	case ERROR_OPERATION_ABORTED:	return KErrCancel;

    default:						return KErrGeneral;
		}
	}

/**
This function wraps the Win32 GetProcAddress API (see http://msdn2.microsoft.com/en-us/library/ms683212.aspx).
*/
FARPROC Emulator::GetProcAddress(HMODULE hModule, LPCSTR lpProcName)
	{
	__LOCK_HOST;
	return ::GetProcAddress(hModule, lpProcName);
	}



// image file support

// loaded modules


/**
Gets the header format of the file system.

@return		Returns the header format of the file system.	
*/
EXPORT_C const IMAGE_NT_HEADERS32* Emulator::TModule::NtHeader() const
	{
	if (!IsValid())
		return 0;
	const IMAGE_DOS_HEADER* dhead = (const IMAGE_DOS_HEADER*)Translate(0);
	if ( IMAGE_DOS_SIGNATURE != dhead->e_magic )
		return 0;
	if (dhead->e_lfarlc < sizeof(IMAGE_DOS_HEADER))
		return 0;
	const IMAGE_NT_HEADERS32* ntHead = (const IMAGE_NT_HEADERS32*)Translate(dhead->e_lfanew);
    if ( ntHead->Signature != IMAGE_NT_SIGNATURE )
		return 0;
	return ntHead;
	}


/**
Constructor which sets the handles of loaded module to the specified module.

@param aModuleName		Holds the name of the module to be loaded.
*/
EXPORT_C Emulator::TModule::TModule(LPCSTR aModuleName)
	: iModule(GetModuleHandleA(aModuleName)), iBase(iModule)
	{
	if (!NtHeader())
		{
		iModule = 0;
		iBase = 0;
		}
	}


/**
Gets the pointer of the section header if the header name matches with the buffer aSection[].
		
@param aSection[]	A buffer of type BYTE.		

@return 	Returns the pointer to section header of the file system.
*/
EXPORT_C const IMAGE_SECTION_HEADER* Emulator::TModule::SectionHeader(const BYTE aSection[]) const
	{
	const IMAGE_NT_HEADERS32* ntHead = NtHeader();
	if (!ntHead)
		return 0;

	const IMAGE_SECTION_HEADER* imgHead = (const IMAGE_SECTION_HEADER*)((TUint8*)&ntHead->OptionalHeader + ntHead->FileHeader.SizeOfOptionalHeader);
	const IMAGE_SECTION_HEADER* end = imgHead + ntHead->FileHeader.NumberOfSections;
	for (; imgHead < end; ++imgHead)
		{
		if (memcmp(imgHead->Name, aSection, IMAGE_SIZEOF_SHORT_NAME)==0)
			return imgHead;
		}
	return 0;
	}


/**
Points to the first byte or first page of the loaded module or mapped file image.
		
@param aSection[]	A buffer of type BYTE.
		
@return TAny		Returns first byte of the loaded module or first page of the mapped file image.
*/
EXPORT_C const TAny* Emulator::TModule::Section(const BYTE aSection[]) const
	{
	const IMAGE_SECTION_HEADER* imgHead = SectionHeader(aSection);
	if (imgHead)
		return Translate(IsLoaded() ? imgHead->VirtualAddress : imgHead->PointerToRawData);
	return 0;
	}


/**
Go through the section headers looking for UID section.
	
@param aType	Contains a UID type.
*/
EXPORT_C void Emulator::TModule::GetUids(TUidType& aType) const
	{
	const TEmulatorImageHeader* hdr = (const TEmulatorImageHeader*)Section(KWin32SectionName_Symbian);
	if (hdr)
		aType = *(const TUidType*)&hdr->iUids[0];
	}


/**
Goes through section headers and gets the information of file system.

@param aInfo	Contains the information of the file system.
*/
EXPORT_C void Emulator::TModule::GetInfo(TProcessCreateInfo& aInfo) const
	{
	aInfo.iExceptionDescriptor = 0;
	const IMAGE_NT_HEADERS32* ntHead = NtHeader();
	if (ntHead)
		{
		const TEmulatorImageHeader* hdr = (const TEmulatorImageHeader*)Section(KWin32SectionName_Symbian);
		if (hdr)
			{
			aInfo.iUids = *(const TUidType*)&hdr->iUids[0];
			TBool isExe = (hdr->iUids[0].iUid==KExecutableImageUidValue);
			TBool data_section_present = (Section(KWin32SectionName_EpocData)!=NULL);
			TBool data = hdr->iFlags & KEmulatorImageFlagAllowDllData;
			if (data_section_present && isExe)
				data = ETrue;
			aInfo.iBssSize=data?1:0;
			aInfo.iDataSize=0;
			aInfo.iTotalDataSize=aInfo.iBssSize;
			aInfo.iDepCount = 0;
			aInfo.iHeapSizeMin = ntHead->OptionalHeader.SizeOfHeapCommit;
			aInfo.iHeapSizeMax = ntHead->OptionalHeader.SizeOfHeapReserve;
			aInfo.iStackSize = 0x1000;
			aInfo.iPriority = hdr->iPriority;
			aInfo.iHandle = NULL;
			aInfo.iS = hdr->iS;
			aInfo.iModuleVersion = hdr->iModuleVersion;
			if (ntHead->FileHeader.Characteristics & IMAGE_FILE_DLL)
				aInfo.iAttr |= ECodeSegAttHDll;
			}
		}
	GetUids(aInfo.iUids);
	}


/**
Finds the import section from the data directory. This relies on the module being loaded.
		
@return 	Returns the imported executable.
*/
EXPORT_C const IMAGE_IMPORT_DESCRIPTOR* Emulator::TModule::Imports() const
	{
	if (!IsLoaded())
		return NULL;
	const IMAGE_NT_HEADERS32* ntHead = NtHeader();
	if (!ntHead)
		return NULL;
	DWORD va = ntHead->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT].VirtualAddress;
	if (!va)
		return NULL;
	return (const IMAGE_IMPORT_DESCRIPTOR*)Translate(va);
	}

// modules in the file system
EXPORT_C TInt Emulator::RImageFile::Open(LPCTSTR aImageFile)
	{
	Buf8<MAX_PATH>   nameBuf(aImageFile);
	char *pName = strrchr(LPCSTR(nameBuf), '\\');
	pName ? ++pName : pName = (char *)LPCSTR(nameBuf);

	__LOCK_HOST;
	iMapping = OpenFileMapping(FILE_MAP_READ, FALSE, (LPCTSTR)pName);
	if (!iMapping)
		{
		if (pName == (char *)LPCSTR(nameBuf))
			iModule = Emulator::GetModuleHandle(aImageFile);
		if (iModule)
			iBase = iModule;
		else
			{
			// need to map the file instead
			HANDLE file = Emulator::CreateFile(aImageFile, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, 0, NULL);
			if (file == INVALID_HANDLE_VALUE)
				return LastError();
			iMapping = CreateFileMappingA(file, NULL, PAGE_READONLY, 0, 0, pName);
			CloseHandle(file);
			}
		}
	if (!iModule)
		{
		if (!iMapping)
			return LastError();

		// First we need to read in the PE file's image headers, which consist of a 4 byte signature, the file header
		// containing general information and up to 96 section headers.  Map this amount of the file into memory so
		// that we can examine it and calculate the minimum size of the sections themselves that need to be mapped into
		// memory in order to examine the actual sections
		SIZE_T headersSize = (sizeof(DWORD) + sizeof(IMAGE_FILE_HEADER) + (96 * sizeof(IMAGE_OPTIONAL_HEADER)));
		iBase = MapViewOfFile(iMapping, FILE_MAP_READ, 0, 0, headersSize);

		if (iBase)
			{
			// Scan through the section headers and determine the minimum amount of the file to be mapped into memory
			// in order for us to safely examine the sections, and map the file into memory.  We do this rather than
			// map the entire PE file into memory because with full debug information, DLLs can be anything up to 100 MB!!!
			TInt sectionsSize = GetSectionsSize(NtHeader());

			// Before remapping the file with its new size, unmap it.  While one would think that it is safe to
			// increase the size of the file's mapping it is not, and doing so triggers behaviour in Windows that
			// results in quickly using up all available virtual memory address space!
			if (UnmapViewOfFile(iBase))
				{
				iBase = MapViewOfFile(iMapping, FILE_MAP_READ, 0, 0, sectionsSize);
				}
			else
				{
				iBase = NULL;
				}
			}

		if (!iBase)
			{
			CloseHandle(iMapping);
			iMapping = 0;
			return LastError();
			}
		}

	if (!NtHeader())
		{
		Close();
		return KErrNotSupported;
		}
	return KErrNone;
	}

EXPORT_C void Emulator::RImageFile::Close()
	{
	if (iMapping)
		{
		UnmapViewOfFile(iBase);
		CloseHandle(iMapping);
		iMapping = 0;
		}
	iBase = 0;
	iModule = 0;
	}
