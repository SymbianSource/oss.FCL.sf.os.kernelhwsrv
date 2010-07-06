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
// e32\include\emulator.h
// 
//

/**
 @file
 @publishedPartner
 @released
*/

#ifndef __EMULATOR_H__
#define __EMULATOR_H__

#include <e32def.h>
#define WIN32_LEAN_AND_MEAN
#pragma warning( disable : 4201 ) // nonstandard extension used : nameless struct/union
#include <windows.h>
#pragma warning( default : 4201 ) // nonstandard extension used : nameless struct/union

class TUidType;
class TProcessCreateInfo;

// Names of sections that we are interested in looking at.  These are used by the GetSectionsSize()
// routine to determine the minimum number of bytes of a DLL to map into memory in order to examine
// its sections.  If a new section is added here then it must also be added to the list of sections
// held in the sectionNames array in GetSectionsSize()
static const BYTE KWin32SectionName_Symbian[IMAGE_SIZEOF_SHORT_NAME]	= {'.','S','Y','M','B','I','A','N'};
static const BYTE KWin32SectionName_Import[IMAGE_SIZEOF_SHORT_NAME]		= {'.','i','d','a','t','a','\0','\0'};
static const BYTE KWin32SectionName_EpocData[IMAGE_SIZEOF_SHORT_NAME]	= {'.','d','a','t','a','\0','\0','\0'};
static const BYTE KWin32SectionName_EpocBss[IMAGE_SIZEOF_SHORT_NAME]	= {'.','b','s','s','\0','\0','\0','\0'};
static const BYTE KWin32SectionName_Text[IMAGE_SIZEOF_SHORT_NAME]		= {'.','t','e','x','t','\0','\0','\0'};
static const BYTE KWin32SectionName_RData[IMAGE_SIZEOF_SHORT_NAME]		= {'.','r','d','a','t','a','\0','\0'};
static const BYTE KWin32SectionName_NmdExpData[IMAGE_SIZEOF_SHORT_NAME]	= {'.','e','x','p','d','a','t','a'};

/**
Emulator utility functions. These APIs should only be used for extensions to emulator 
functionality and are unsupported on non-emulator platforms.

@publishedPartner
@released
*/
struct Emulator
	{
public:

	/**
	Declares a function pointer to TLock.
	
	@see TLock.
	*/
	typedef void (*TLockFunc)();
	
	/**
	Declares a function pointer to Win32SEHException.
	
	@see Win32SEHException.		
	*/
	typedef DWORD (*TExceptionFunc)(EXCEPTION_RECORD* aException, CONTEXT* aContext);
	
	/**
	A structure which packs parameters to be inintialized. 
	*/
	struct SInit
		{
		TUint iCodePage;
		TLockFunc iLock;
		TLockFunc iUnlock;
		TLockFunc iEscape;
		TLockFunc iReenter;
		TExceptionFunc iException;
		};
	
	/**
	A structure which handles the global lock for host interaction.
	*/
	struct TLock
		{
		
		/**
		Constructor which calls the Emulator::Lock() function.
		
		@see Emulator::Lock().
		*/
		inline TLock()
			{Emulator::Lock();}
		
		/**
		Destructor which calls the Emulator::Unlock() function.
		
		@see Emulator::Unlock().
		*/
		inline ~TLock()
			{Emulator::Unlock();}
		};
		
	/**
	A structure which handles current thread by taking in or out of the emulator scheduling model.
	*/
	struct TEscape
		{
		
		/**
		Constructor which calls the Emulator::Escape() function.
		
		@see Emulator::Escape().
		*/
		inline TEscape()
			{Emulator::Escape();}
		
		/**
		Destructor which calls the Emulator::Reenter() function.
		
		@see Emulator::Reenter().
		*/
		inline ~TEscape()
			{Emulator::Reenter();}
		};
#define __LOCK_HOST Emulator::TLock __lock
#define __ESCAPE_HOST Emulator::TEscape __escape
public:

	IMPORT_C static void Init(const SInit& aInit);
//
	IMPORT_C static void Lock();
	IMPORT_C static void Unlock();
	IMPORT_C static void Escape();
	IMPORT_C static void Reenter();
//
	IMPORT_C static DWORD Win32SEHException(EXCEPTION_RECORD* aException, CONTEXT* aContext);
//
	IMPORT_C static TInt LastError();
	IMPORT_C static BOOL CreateDirectory(LPCTSTR, LPSECURITY_ATTRIBUTES);
	IMPORT_C static BOOL CreateAllDirectories(LPCSTR);
	IMPORT_C static HANDLE CreateFile(LPCTSTR ,DWORD ,DWORD ,LPSECURITY_ATTRIBUTES ,DWORD ,DWORD ,HANDLE);
	IMPORT_C static BOOL DeleteFile(LPCTSTR);
	IMPORT_C static HANDLE FindFirstFile(LPCTSTR ,LPWIN32_FIND_DATA);	
	IMPORT_C static BOOL FindNextFile(HANDLE ,LPWIN32_FIND_DATA);
	IMPORT_C static BOOL GetDiskFreeSpace(LPCTSTR ,LPDWORD ,LPDWORD ,LPDWORD ,LPDWORD);
	IMPORT_C static DWORD GetFileAttributes(LPCTSTR);
	IMPORT_C static HMODULE GetModuleHandle(LPCTSTR);
	IMPORT_C static DWORD GetCurrentDirectory(DWORD ,LPTSTR);
	IMPORT_C static DWORD GetTempPath(DWORD ,LPTSTR);
	IMPORT_C static BOOL GetVolumeInformation(LPCTSTR ,LPTSTR ,DWORD ,LPDWORD ,LPDWORD ,LPDWORD ,LPTSTR,DWORD);
	IMPORT_C static HMODULE LoadLibrary(LPCTSTR);	
	IMPORT_C static BOOL FreeLibrary(HMODULE);	 
	IMPORT_C static BOOL MoveFile(LPCTSTR ,LPCTSTR);
	IMPORT_C static BOOL CopyFile(LPCTSTR ,LPCTSTR, BOOL);
	IMPORT_C static VOID OutputDebugString(LPCTSTR);	
	IMPORT_C static BOOL RemoveDirectory(LPCTSTR);
	IMPORT_C static BOOL SetFileAttributes(LPCTSTR ,DWORD);	
	IMPORT_C static BOOL SetVolumeLabel(LPCTSTR ,LPCTSTR);	
	IMPORT_C static FARPROC GetProcAddress(HMODULE, LPCSTR);
	IMPORT_C static DWORD GetModuleFileName(HMODULE hModule, LPWSTR lpFilename);
public:

	/**
	A class which holds a loaded module of the file system. 
	*/
	class TModule
		{
	public:
	
		/**
		Constructor which sets the handles of loaded module	to current instance of the application.
		
		@param aModule		A handle to the current instance of the application.
		*/
		inline TModule(HINSTANCE aModule)
			:iModule(aModule), iBase(aModule)
			{}
			
		IMPORT_C TModule(LPCSTR aModuleName);
		
		/**
		Checks the validity of the loaded module.
		
		@return		TRUE, if the function succeeds
					FALSE, if the function fails		 
		*/
		inline TBool IsValid() const
			{return iBase!=NULL;}
			
		/**
		Relocates the address of the loaded module by appending aRawVirtualAddress to the handle of loaded module.
		
		@param aRawVirtualAddress	Contains the address of the first byte of the section when loaded into 
									memory, relative to the image base. For object files, this is the 
									address of the first byte before relocation is applied.
		
		@return 	Returns the actual address of the handle.
		*/
		inline const TAny* Translate(TInt32 aRawVirtualAddress) const
			{return (const TUint8*)iBase + aRawVirtualAddress;}
	
		
		IMPORT_C void GetUids(TUidType& aType) const;
		IMPORT_C void GetInfo(TProcessCreateInfo& aInfo) const;
	public:
	
		/**
		Default constructor which sets the handles of loaded module or mapped file image to the default handle.	
		*/
		inline TModule()
			:iModule(0), iBase(0)
			{}
	
		/**
		Checks whether the current module is a loaded module, or just a mapped file image.
		
		@return		TRUE, if the current module is a loaded module 
					FALSE, if the current module is a mapped image 
		*/
		TBool IsLoaded() const		// Is this a loaded module, or just a mapped file image.
			{return iModule != 0;}
			
		IMPORT_C const IMAGE_NT_HEADERS32* NtHeader() const;
		
		/**
		Passes the handle of a loaded module to the calling module.
		
		@return 	Returns a handle of a loaded module.
	    */
		const TAny* Base() const
			{return iBase;}

		IMPORT_C const IMAGE_SECTION_HEADER* SectionHeader(const BYTE aSection[]) const;
		IMPORT_C const TAny* Section(const BYTE aSection[]) const;	
		IMPORT_C const IMAGE_IMPORT_DESCRIPTOR* Imports() const;
	public:
	
		/**
		A handle to a loaded module.
		*/
		HMODULE iModule;
		
		/**
		A handle to a loaded module for the mapped view.
		*/
		const TAny* iBase;
		};
		
	/**
	A class which holds a mapped image of the file system.
	*/
	class RImageFile : public TModule
		{
	public:
	
		/**
		Constructor which sets the handle of the mapped image file to default handle.	
		*/
		inline RImageFile()
			: iMapping(0)
			{}
			
		IMPORT_C TInt Open(LPCTSTR aImageFile);
		IMPORT_C void Close();
	private:
	
		/**
		A handle to the mapped image file.
		*/
		HANDLE iMapping;
		};
	};


#endif
