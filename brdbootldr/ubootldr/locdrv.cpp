// Copyright (c) 2005-2009 Nokia Corporation and/or its subsidiary(-ies).
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
// improvised boot loader mechanism
// 
//

/**
 @file
*/

#include <e32const.h>
#include <e32const_private.h>
#include <e32std.h>
#include <e32std_private.h>
#include <e32svr.h>
#include <e32cons.h>
#include <f32file.h>
#include <hal.h>
#include <u32hal.h>
#include "bootloader_variantconfig.h"
#include <nkern/nk_trace.h>
#include <e32twin.h>

#define FILE_ID	0x594D555D
#include "bootldr.h"

void CloseAndDeleteFile();
RFs TheFs;

// Extra stuff to determine inner compression from the ROM header
#include <e32rom.h>
extern TInt memcmp1(const TUint8* aTrg, const TUint8* aSrc, TInt aLength);

// XXX FIXED DRIVE PATH
const TPtrC filePath=_L("d:\\");

#if defined(__SUPPORT_UNZIP__)
const TBool ZipIsSupported = ETrue;
#else
const TBool ZipIsSupported = EFalse;
#endif

#if defined(__SUPPORT_FLASH_REPRO__)
const TBool FlashIsSupported = ETrue;
#else
const TBool FlashIsSupported = EFalse;
#endif

typedef struct 
	{
	TPtrC filename;
	TBool Zip;        // set to ETrue if this file is a ZIP file
	TBool Flash;      // set to ETrue if this file is to be written to flash
	TBool BootLoader; // set to ETrue if this file is to be flashed to the BootLoader address
	TBool Delete;     // set to ETrue if this file is to be deleted after loading into RAM
	}
	TFileTypes;


TFileTypes supportedFileTypes [] =
	{
		// Name              Zip     Flash   BootLoader  Delete
		{_L("FLASHLDR.ZIP"), ETrue,  ETrue,  ETrue,      ETrue   },
		{_L("FLASHLDR.BIN"), EFalse, ETrue,  ETrue,      ETrue   },

		{VARIANT_ZIP,        ETrue,  EFalse, EFalse,     EFalse  },
		{VARIANT_BIN,        EFalse, EFalse, EFalse,     EFalse  },

		{_L("SYS$ROM.ZIP"),  ETrue,  EFalse, EFalse,     EFalse  },
		{_L("SYS$ROM.BIN"),  EFalse, EFalse, EFalse,     EFalse  },

        {_L("FLASHIMG.ZIP"), ETrue,  ETrue,  EFalse,     EFalse  },
		{_L("FLASHIMG.BIN"), EFalse, ETrue,  EFalse,     EFalse  },
        {_L("BOOTLDR.ZIP"),  ETrue,  ETrue,  ETrue,      EFalse  },
		{_L("BOOTLDR.BIN"),  EFalse, ETrue,  ETrue,      EFalse  },

		{_L("COREIMG.BIN"),  EFalse, EFalse, EFalse,     ETrue   },

		{_L(""),0} // Last Entry - this empty row is used in code to detect table end
	};


GLDEF_C TBool SearchDrivesRaw()
	{
	// Scan local drives directly (i.e. via TLocalDrv rather than RFS)
	
	PrintToScreen(_L("Checking local drives directly.\r\n"));
	
	TDriveInfoV1Buf diBuf;
	UserHal::DriveInfo(diBuf);
	TDriveInfoV1 &di=diBuf();
	
	
	LocDrvChg = EFalse;
	LocDrvPos = 0;
	TInt LocalDriveNum = KErrNotFound;
	TInt r = KErrNone;
	TInt n=0;
	for ( ; n<KMaxLocalDrives && LocalDriveNum == KErrNotFound; n++)
		{
		r = LocDrv.Connect(n, LocDrvChg);

		if(r != KErrNone)
			{
			RDebug::Print(_L("\nDrive %d: TBusLocalDrive::Connect() failed %d"), n, r);
			continue;
			}	

	    TLocalDriveCapsV5Buf capsBuf;
	    TLocalDriveCapsV5& caps = capsBuf();
		r = LocDrv.Caps(capsBuf);
		if(r != KErrNone)
			{
			RDebug::Print(_L("\nDrive %d: TBusLocalDrive::Caps() failed %d"), n, r);
			continue;
			}
		else
			{
			PrintToScreen(_L("Found Drive %d OK\r\n"),n);
			RDebug::Print(_L("\nDrive %d: %S"), n, &di.iDriveName[n]);
			RDebug::Print(_L("PartitionType %X"), caps.iPartitionType);
			RDebug::Print(_L("PartitionSize %ld"), caps.iSize);
			
			// Check that drive is labelled as MMC or SDIO, 
			// note that this is a platform specific label...
			if ((di.iDriveName[n].MatchF(_L("MultiMediaCard0")) == KErrNone) ||
			    (di.iDriveName[n].MatchF(_L("SDIOCard0")) == KErrNone))
				{
				if (caps.iPartitionType == KPartitionTypeROM)
					{
					RDebug::Print(_L("- ROM Partition"));
					LocalDriveNum = n;
					FileSize = caps.iSize;
					break;
					}
				}
			
			CloseLocalDrive();
			}
		}
	
	if (LocalDriveNum == KErrNotFound)
		{
		RDebug::Print(_L("No ROM Partitions found"));
		return EFalse;
		}
	else
		{
		RDebug::Print(_L("Query ROM in drive %d"), LocalDriveNum);
		}
	
	InputFunction=ReadFromLocalDrive;
	CloseInputFunction=CloseLocalDrive;
	
	RDebug::Print(_L("Determine the compression"));
	
    r = GetInnerCompression(ImageDeflated, RomLoaderHeaderExists);
    
    if(KErrNone != r)
        {
        PrintToScreen(_L("Unable to determine the compression!\r\n"));
	    BOOT_FAULT();    
        }
    else
    	{
    	// Put position back to start
    	LocDrvPos = 0;
    	}
	
	return ETrue;
	}


TInt ReadFromLocalDrive(TUint8* aDest, TInt& aLength)
	{
	// construct as TPtr8(TUint8 *aBuf, TInt aMaxLength);
	// .. because TBusLocalDrive.Read only understands descriptors
	TPtr8 d(aDest, aLength);
	
	TInt r = LocDrv.Read(LocDrvPos,aLength,d);
	LocDrvPos+=aLength;
	
	if (LocDrvPos >= FileSize)
		{
		// ROM read completely
		r = KErrEof;
		}
	
	return r;
	}


void CloseLocalDrive()
	{
	LocDrv.Disconnect();
	}


GLDEF_C TBool SearchDrives()
	{
	// set up the list of files to look for -- this code will search through
	// all available drives until it finds one of these files in a root
	// directory

	PrintToScreen(_L("Checking local drives.\r\n"));

	// search for files
	TInt r = TheFs.Connect();
	if (r != KErrNone)
		{
		RDebug::Print(_L("FAULT: Connecting RFs returned %d\r\n"),r);
		BOOT_FAULT();
		}

	TFindFile finder(TheFs);

	// for each file in the list
	TInt NrChecked = 0;
	r = KErrNotFound;

	while ( (*(supportedFileTypes[NrChecked].filename.Ptr()) != 0) && (r == KErrNotFound))
		{
		if (  (!ZipIsSupported && (supportedFileTypes[NrChecked].Zip))
		   || (!FlashIsSupported && (supportedFileTypes[NrChecked].Flash))
		   )
			{
			//skip ZIP/FLASH file types if they aren't supported
			}
		else
			{
			TPtrC thisFile = supportedFileTypes[NrChecked].filename;
			r = finder.FindByDir(thisFile, filePath);

			if (r == KErrNone)
				PrintToScreen(_L("Found %s\r\n"), thisFile.Ptr());
			}
		NrChecked++;
		}

	// if found
	if (r == KErrNone)
		{
		// setup some flags 
		LoadFile=--NrChecked; // predecrement since this was incremented at the end of the last loop
		LoadDevice=ELoadDrive;

		const TPtrC bootFileName = finder.File();

		PrintToScreen(_L("Opening: %s\r\n"), supportedFileTypes[NrChecked].filename.Ptr());

		r = bootFile.Open(TheFs, bootFileName, EFileRead);

		if (r != KErrNone)
			{
			PrintToScreen(_L("Bootfile failed to open - err %d.\r\n"),r);

			BOOT_FAULT();
			}

		ImageZip         = supportedFileTypes[NrChecked].Zip;
		LoadToFlash      = supportedFileTypes[NrChecked].Flash;
		FlashBootLoader  = supportedFileTypes[NrChecked].BootLoader;
		
		InputFunction=ReadFromFile;
		CloseInputFunction = supportedFileTypes[NrChecked].Delete ? CloseAndDeleteFile : CloseFile;

        if( !ImageZip)
            {
            r = GetInnerCompression(ImageDeflated, RomLoaderHeaderExists);
            if(KErrNone != r)
                {
                PrintToScreen(_L("Unable to determine the compression!\r\n"));
			    BOOT_FAULT();    
                }
            else
            	{
            	// Move file pos back to the beginning
            	TInt pos = 0;
                r = bootFile.Seek(ESeekStart, pos);
            	}
            }

		r = bootFile.Size(FileSize);

		if (r == KErrNone)
			{
			PrintToScreen(_L("Opened, size: %d bytes.\r\n"), FileSize);
			if(ImageDeflated)
    			{
    			PrintToScreen(_L("ROM Image is deflated.\r\n"));    
    			}
			}
		else
			{
			PrintToScreen(_L("Unable to read file size\r\n"));
			BOOT_FAULT();
			}

		// Found image - return true
		return ETrue;
		}

	// else NOT FOUND
	return EFalse;
	}

TInt ReadFromFile(TUint8* aDest, TInt& aLength)
	{
	// construct as TPtr8(TUint8 *aBuf, TInt aMaxLength);
	// .. because RFile.Read only understands descriptors
	TPtr8 d(aDest, aLength);

	TInt r = bootFile.Read(d);

	if (d.Length() < aLength) // may happen at the end of a file
		{
		aLength = d.Length();
		}

	if (d.Length() == 0)	// indicates end of file
		{
		if (FileSize-aLength == ImageReadProgress)
			return KErrEof;
		else
			// this will drop through to the error code below and will fault
			return KErrGeneral;
		}

	return r;
	}

void CloseFile()
	{
	bootFile.Close();
	}

void CloseAndDeleteFile()
	{
	TFileName fileName;
	TInt r = bootFile.FullName(fileName);
	if (r != KErrNone)
		PrintToScreen(_L("CloseAndDeleteFile() RFile::FullName returned %d"), r);

	bootFile.Close();

	r = TheFs.Delete(fileName);
	PrintToScreen(_L("Deleted file fileName %S"), &fileName);
	}

//#define TROM_LOADER_HEADER_SIZE 0x100
#define BUFFER_SIZE         (TROM_LOADER_HEADER_SIZE + sizeof(TRomHeader))

TInt GetInnerCompression(TBool &aImageDeflated, TBool &aRomLoaderHeaderExists )
    {
    TInt r = KErrNone;
    
    TUint8 buffer[BUFFER_SIZE];
    TInt   bufferSize = BUFFER_SIZE;
    
    const TUint8 * romLoaderSignature1 = (const TUint8*)"EPOC";
    const TUint8 * romLoaderSignature2 = (const TUint8*)"ROM";
    
    r = ReadInputData((TUint8*)&buffer, bufferSize);
	if( KErrNone!=r)
		{
		PrintToScreen(_L("Unable to read loader headers... (size:%d)\r\n"), bufferSize);
		BOOT_FAULT();    
		}
	else
    	{
    	// Check headers
    	TRomHeader* romHeader= (TRomHeader *) &buffer;
    	aRomLoaderHeaderExists = EFalse;
    	
    	if( !memcmp1(buffer, romLoaderSignature1, 4) && !memcmp1(&buffer[8], romLoaderSignature2, 3) )
    	    {
            // We have TRomLoaderHeader skip it
            romHeader = (TRomHeader *) (&buffer[TROM_LOADER_HEADER_SIZE]);
            aRomLoaderHeaderExists = ETrue;
    	    }
    	
        if(romHeader->iCompressionType == 0 )
            {
             RDebug::Print(_L("Image is NOT Compressed"));
             aImageDeflated = EFalse;
             FileSize = romHeader->iUncompressedSize;
             PrintToScreen(_L("ROMSIZE:%d\r\n"), romHeader->iUncompressedSize);
             RDebug::Print(_L("ROMSIZE:%d"), romHeader->iUncompressedSize);
             
         	if (romHeader->iPageableRomStart > 0)
         		{
         		PrintToScreen(_L("Paged ROM FOUND\r\n"));
         		RDebug::Print(_L("Paged ROM FOUND"));
         		FileSize = romHeader->iPageableRomStart;
         		RDebug::Print(_L("Unpaged ROMSIZE:%d"), romHeader->iPageableRomStart);
         		}          
            }
        else if (romHeader->iCompressionType == KUidCompressionDeflate )
            {
            RDebug::Print(_L("Image is Compressed\r\n"));
            aImageDeflated = ETrue;
            FileSize = romHeader->iUnpagedUncompressedSize;
            RDebug::Print(_L("Compressed ROMSIZE:%d\r\n"), romHeader->iUnpagedUncompressedSize);       
            }
        else
            {
            RDebug::Print(_L("Not supported compression method:0x%08x\r\n"), romHeader->iCompressionType);
            PrintToScreen(_L("Not supported compression method:0x%08x\r\n"), romHeader->iCompressionType);
            r = KErrNotSupported;
            }
     	}
	return r;        
    }
    
