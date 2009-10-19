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
// bootldr\inc\bootldr.h
// 
//

#ifndef FILE_ID
#error File ID not specified
#endif

#include "bootloader_variantconfig.h"

#define BOOTLDR_VERSION_MAJOR 1
#define BOOTLDR_VERSION_MINOR 11

#ifndef PLATFORM_BUILD
#define PLATFORM_BUILD 0
#endif

#include <e32std.h>
#include <e32svr.h>
#include <e32cons.h>
#include <f32file.h>
#include <d32comm.h>

GLREF_C void BootFault(TUint aId, TInt aLine, char aFileName[]);
#define BOOT_FAULT()	BootFault(FILE_ID,__LINE__, __FILE__ )
#define TEST(c)		((void)((c)||(BOOT_FAULT(),0)))
#define CHECK(c)	((void)(((c)==0)||(RDebug::Print(_L("Error %d at line %d\n"),(c),__LINE__),BOOT_FAULT(),0)))

//#define _DEBUG_CORELDR_  1

#ifdef _DEBUG_CORELDR_        
    #define DEBUG_PRINT(m) PrintToScreen m;	\
                           RDebug::Print m;
#else
    #define DEBUG_PRINT(m)
#endif    

#define TROM_LOADER_HEADER_SIZE 0x100

GLREF_D TInt LoadDrive;
GLREF_D TInt LoadFile;
GLREF_D TInt LoadSize;
GLREF_D TInt ImageSize;
GLREF_D TInt ImageReadProgress;
GLREF_D TBool ImageHeaderPresent;
GLREF_D TBool ImageZip;
GLREF_D TBool LoadToFlash;
GLREF_D TBool FlashBootLoader;
GLREF_D TBuf<256> FileName;
GLREF_D TUint32 RamBootPhys;
GLREF_D TUint32 * ActualDestinationAddress;
GLREF_D TInt SerialDownloadPort;
GLREF_D TBps SerialBaud;
GLREF_D RFile bootFile;
GLREF_D TInt FileSize;
GLREF_D TBool ImageDeflated;
GLREF_D TBool RomLoaderHeaderExists;
GLREF_D TBusLocalDrive LocDrv;
GLREF_D TBool LocDrvChg;
GLREF_D TInt64 LocDrvPos;

enum TLoadDevice
	{
	ELoadDrive,
	ELoadSerial,
	ELoadUSBMS,
	EBootUSBMS,
	EBootEMMC
	};

GLREF_D TLoadDevice LoadDevice;

typedef TInt (*TInputFunc)(TUint8* aDest, TInt& aLength);
GLREF_D TInputFunc InputFunction;

typedef void (*TCloseInputFunc)();
GLREF_D TCloseInputFunc CloseInputFunction;

inline TInt ReadInputData(TUint8* aDest, TInt& aLength)
	{ return (*InputFunction)(aDest,aLength); }
inline void CloseInput()
	{ (*CloseInputFunction)(); }


GLREF_C TInt LoadDriver(const TDesC& aName, TBool aPdd);

// Graphical screen methods
GLREF_C void PrintToScreen(TRefByValue<const TDesC> aFmt,...);
GLREF_C void InitDisplay();
GLREF_C void ClearScreen();
GLREF_C void PutChar(TUint aChar);
GLREF_C void PutString(const TDesC& aBuf);
GLREF_C void InitProgressBar(TInt aId, TUint aLimit, const TDesC& aTitle);
GLREF_C void UpdateProgressBar(TInt aId, TUint aProgress);

// Menu
GLREF_C void StartMenu();
GLREF_C void EnableMenu();
GLREF_C void DisableMenu();

// Fundamental download methods
GLREF_C void DoDownload();
GLREF_C TInt DoZipDownload(RFile &aBootFile);
GLREF_C TInt DoDeflateDownload();
GLREF_C void Restart(TInt aCode);

// Media download
GLREF_C TBool SearchDrives();
GLREF_C TInt ReadFromFile(TUint8* aDest, TInt& aLength);
GLREF_C void CloseFile();
GLREF_C TInt GetInnerCompression(TBool &aImageDeflated, TBool &aRomLoaderHeaderExists);

// Local Drive Raw Access
GLREF_C TBool SearchDrivesRaw();
GLREF_C TInt ReadFromLocalDrive(TUint8* aDest, TInt& aLength);
GLREF_C void CloseLocalDrive();

// Serial download
GLREF_C TInt InitSerialDownload(TInt aPort);

// NOR FLASH methods
GLREF_C TInt InitFlashWrite();
GLREF_C void NotifyDataAvailable(TInt aTotalAmount);
GLREF_C void NotifyDownloadComplete();

// USB Mass Storage download
GLREF_C void TryUSBMS();
GLREF_C TBool StartUSBMS();

// Variant supplied methods
GLREF_C void VariantInit();
GLREF_C void ReadConfig();
GLREF_C void WriteConfig();

inline TLinAddr DestinationAddress()
	{ return (TLinAddr)ActualDestinationAddress; }

