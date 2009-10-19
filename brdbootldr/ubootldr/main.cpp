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
#include <e32rom.h>

#include <hal.h>
#include <d32ethernet.h>

#define FILE_ID	0x594D555D
#include "bootldr.h"
#include "ubootldrldd.h"

#include <d32comm.h>

#ifdef _DEBUG_CORELDR_
#define WAIT_TO_REBOOT  10000000
#else
#define WAIT_TO_REBOOT    100000
#endif

#define addr_to_page(a) (a&~(0x1000-1))

_LIT(KPanicText,"UBOOTLDR");
_LIT(LDD_NAME, "Enet");
_LIT(PDD_NAME, "Ethernet");

// Memory
RUBootldrLdd Ldd;
RChunk TheRamChunk;		// This is the download area

/// Global number of seconds to delay before reboot

void GetChunk()
	{
	TInt PageSize;
	UserHal::PageSizeInBytes(PageSize);

	TInt r = User::LoadLogicalDevice(KBootldrLddName);

	r = Ldd.Open();
	if (r!=KErrNone)
			{
			PrintToScreen(_L("FAULT due to LDD open\r\n"));
			BOOT_FAULT();
			}

	TUint8* kernelAddress;
	r=Ldd.CreateChunk(KRamTargetSize,(TAny**)&kernelAddress);
	if (r!=KErrNone)
			{
			PrintToScreen(_L("FAULT due to chunk create\r\n"));
			BOOT_FAULT();
			}

	r = Ldd.CommitMemory(KRamTargetSize,addr_to_page(KRamTargetAddr));
	if (r!=KErrNone)
			{
			PrintToScreen(_L("FAULT due to commit\r\n"));
			BOOT_FAULT();
			}

	r = Ldd.GetChunkHandle(TheRamChunk);
	if (r!=KErrNone)
			{
			PrintToScreen(_L("FAULT due to handle\r\n"));
			BOOT_FAULT();
			}

	TUint8* Base = TheRamChunk.Base();
	ActualDestinationAddress = (TUint32*)Base;
	}

// method of getting the date string at build time
// Taken from e32test\device\t_usbco22.cpp
#define WIDEN2(x) L ## x
#define WIDEN(x) WIDEN2(x)
#define __WDATE__ WIDEN(__DATE__)


//////////////////////////////////////////////////////////////////////////////
//
// Application entry point
//
//////////////////////////////////////////////////////////////////////////////
LOCAL_C void RunAppL()
	{
	TInt r;

	// Get the memory download area mapped into a chunk
	GetChunk();

	// Initialise our graphical screen
	InitDisplay();

#ifdef __USE_VARIANT_INIT__
	// The variant initialisation function is likely to perform bootloader
	// configuration or print debug information.
	VariantInit();
#endif

#ifdef __USE_USBMS__
	// If the bootloader is in the USB Mass Storage mode (indicated by the Variant
	// initialisation routine) then call that method first; it will either read/boot
	// an image, start the USB Mass Storage boot application or return.
	if ((LoadDevice==ELoadUSBMS) || (LoadDevice==EBootUSBMS))
		TryUSBMS();
#endif

	// Start the menu UI thread, this will cause a console to overwrite the framebuffer
	StartMenu();
	ClearScreen();

	PrintToScreen(_L("Copyright (c) 2005-2009 Nokia Corporation and/or its subsidiary(-ies).\r\n"));
	PrintToScreen(_L("BootLoader %d.%d\r\n"), BOOTLDR_VERSION_MAJOR, BOOTLDR_VERSION_MINOR);
	PrintToScreen(_L("Platform build %d\n\rBuilt: %s\r\n"), PLATFORM_BUILD, __WDATE__);

	// Load Ethernet driver:  it might need to load to set the MAC address in HAL
    TInt pddLoaded = KErrUnknown;
	TInt lddLoaded = KErrUnknown;
	TInt opened    = KErrUnknown;
	RBusDevEthernet card;

    pddLoaded = User::LoadPhysicalDevice(PDD_NAME);
	if (pddLoaded == KErrNone)
		{
		lddLoaded = User::LoadLogicalDevice(LDD_NAME);
		if (lddLoaded == KErrNone)
			{
			opened = card.Open(0);
			User::After(2000);
			}
		}

	TInt serialNum;
	r = HAL::Get(HAL::ESerialNumber, serialNum);

	if (r == KErrNone && serialNum != 0)
		{
		// we only show the serial number if it was available from HAL and set to something
		PrintToScreen(_L("MAC Address: 00-0a-%02x-%02x-%02x-%02x\r\n"),
			(serialNum & 0xFF000000) >> 24,
			(serialNum & 0x00FF0000) >> 16,
			(serialNum & 0x0000FF00) >>  8,
			(serialNum & 0x000000FF)
			);
		}


	// Free the Ethernet driver now we're done getting the MAC address
	if (opened == KErrNone)
		{
		card.Close();
		}
	if (lddLoaded == KErrNone)
		{
	    User::FreeLogicalDevice(LDD_NAME);
		}
	if (pddLoaded == KErrNone)
		{
	    User::FreePhysicalDevice(PDD_NAME);
		}

	if (LoadDevice == EBootEMMC)
		{
		if ( SearchDrivesRaw() )
			{
			DoDownload();
			}
		}
		
#ifdef __USE_LOCAL_DRIVES__
	// The next method to test is reading an image file from removable media
	if (SearchDrives())
		DoDownload();
#endif

	// The last method is to attempt to download an image over the serial port
	switch (SerialBaud)
		{
		case EBps115200:
			PrintToScreen(_L("Initiating YModem-G on port %d @ 115200 baud\r\n"), SerialDownloadPort);
			break;
		case EBps230400:
			PrintToScreen(_L("Initiating YModem-G on port %d @ 230400 baud\r\n"), SerialDownloadPort);
			break;
		default:
			PrintToScreen(_L("Initiating YModem-G download on port %d\r\n"), SerialDownloadPort);
			SerialBaud = EBps115200;
			break;
		}
	r=InitSerialDownload(SerialDownloadPort);
	if (r!=KErrNone)
		{
		RDebug::Print(_L("FAULT: YModem-G download returned %d\r\n"),r);
		BOOT_FAULT();
		}

	PrintToScreen(_L("Receiving %lS\r\nSize %d\r\n"),&FileName,FileSize);
	DoDownload();
}

TUint Check(const TUint32* aPtr, TInt aSize)
	{
	TUint sum=0;
	aSize/=4;
	while (aSize-->0)
		sum+=*aPtr++;
	return sum;
	}

TUint CheckSumRom()
	{
#if 0
	//
	// This code was obtained from t_romchk in the e32tests, it was useful for
	// checking that roms were not getting corrupted during the download
	// process.
	//
	// However, it is not a universal checksum and is not suitable for all
	// types of images it is therefore being disabled until we need it again.
	//

	PrintToScreen(_L("Checking ROM contents...\r\n"));

	const TRomHeader* romHdr = (const TRomHeader*)DestinationAddress();
	TInt size = romHdr->iUnpagedUncompressedSize;
	const TUint32* addr = (TUint32*)romHdr;
	PrintToScreen(_L("ROM at %x, size %x\r\n"),addr,size);

	TUint checkSum = Check(addr,size);

	// hack the checksum because ROMBUILD is broken
	checkSum -= (romHdr->iRomSize-size)/4; // adjust for missing 0xffffffff
	checkSum -= romHdr->iCompressionType;
	checkSum -= romHdr->iUnpagedCompressedSize;
	checkSum -= romHdr->iUnpagedUncompressedSize;

	TUint expectedChecksum = 0x12345678;
	if (checkSum != expectedChecksum)
		{
		PrintToScreen(_L("Fail: Checksum = %8x, expected %8x\r\n"),checkSum,expectedChecksum);
		RDebug::Print(_L("Fail: Checksum = %8x, expected %8x\r\n"),checkSum,expectedChecksum);
		BOOT_FAULT();
		}
	else
		{
		PrintToScreen(_L("Pass: Checksum = %8x, expected %8x\r\n"),checkSum,expectedChecksum);
		RDebug::Print(_L("Pass: Checksum = %8x, expected %8x\r\n"),checkSum,expectedChecksum);
		}
#endif
	return 0;
	}


GLDEF_C void DoDownload()
/*
 The DoDownload method is ultimatly called by all the download routines, it is
 responsible for unzipping images as well as stripping off EPOC headers and
 starting the flash writing methods.
 */
	{
	TInt r;

	// If the image is zipped start the unzip thread
	if (ImageZip)
		{
		// initialise unzip
		PrintToScreen(_L("Loading zip ..\r\n"));
		r=DoZipDownload(bootFile);
		if (r!=KErrNone)
			{
			PrintToScreen(_L("\r\nFAULT due to zip %d\r\n"), r);
			BOOT_FAULT();
			}
		PrintToScreen(_L("\r\nZip download complete.\r\n"));
		}
	else
		{
		LoadSize=FileSize;
				
		if( ImageDeflated )
    		{
    		PrintToScreen(_L("\r\n\r\nLoading deflated image...\r\n"));
    	    ImageHeaderPresent=EFalse;
    	    DoDeflateDownload();
    	    PrintToScreen(_L("Deflated image download complete.\r\n"));	  
    		}
		else
    		{
    		// If it isn't a zipped or deflated image then the filesize is the image size -
    		// check that it is valid for a Symbian image and determine whether to
    		// remove any header that may be present.
    		TInt size_mod_4k=FileSize & 0xfff;
    		if (size_mod_4k==0)
    			{
    			ImageHeaderPresent=EFalse;
    			PrintToScreen(_L("Image header not present.\r\n"));
    			}
    		else if (size_mod_4k==256)
    			{
    			ImageHeaderPresent=ETrue;
    			PrintToScreen(_L("Image header present.\r\n"));
    			}
    		else
    			{
    		    PrintToScreen(_L("\r\n\r\nInvalid size\r\n"));
    			BOOT_FAULT();	
    			}
    		ImageSize=ImageHeaderPresent ? LoadSize-256 : LoadSize;
    		TUint8* pD=(TUint8*)DestinationAddress();
    		TInt block_size;
    		if (FileSize==0)
    			block_size=0x1000;		// YModem download with unknown size
    		else
    			block_size=Max(0x1000,FileSize>>8);
    		block_size=(block_size+0xfff)&~0xfff;
    		if (FileSize>0)
    			InitProgressBar(0,(TUint)FileSize,_L("LOAD"));
    		r=KErrNone;

// If target location is flash start the flasher thread
#ifdef __SUPPORT_FLASH_REPRO__
    		if (LoadToFlash)
    			{
    			r=InitFlashWrite();
    			if (r!=KErrNone)
    				{
    				PrintToScreen(_L("FAULT due to InitFlashWrite return %d\r\n"), r);
    				BOOT_FAULT();
    				}
    			}
#endif

    		while (r==KErrNone)
    			{
    			TInt len=block_size;
    			r=ReadInputData(pD,len);

    			if (r!=KErrNone && r!=KErrEof)
    				break;
    			pD+=len;
    			ImageReadProgress+=len;
    			if (FileSize>0)
    				{
    				UpdateProgressBar(0,(TUint)ImageReadProgress);
				}
#ifdef __SUPPORT_FLASH_REPRO__
    			if (LoadToFlash)
    				NotifyDataAvailable(ImageReadProgress);
#endif
    			}

    		if (r!=KErrEof)	
    			{
    			PrintToScreen(_L("FAULT due to EOF. %d bytes read.\r\n"), ImageReadProgress);
    			BOOT_FAULT();
    			}
    		else
    			PrintToScreen(_L("Loaded %d bytes.\r\n"), ImageReadProgress);

    		if (ImageReadProgress < LoadSize)
    			ImageSize=ImageHeaderPresent ? ImageReadProgress-256 : ImageReadProgress;
#ifdef __SUPPORT_FLASH_REPRO__
    		if (LoadToFlash)
    			NotifyDownloadComplete();
#endif
    		}

		}
	if (CloseInputFunction)
		CloseInput();

	if (LoadToFlash)
		{
		// In the load to flash scenario it will boot the image.
		while(1)
			User::After(10000000);
		}

	// Booting from Ram - remove the header
	if (ImageHeaderPresent)
		{
		TUint8* pI=(TUint8*)DestinationAddress();
		Mem::Move(pI,pI+256,ImageSize);
		}

	CheckSumRom();

	PrintToScreen(_L("Booting Image...\r\n"));
	// Restart board with "boot image + located in ram"

	User::After(WAIT_TO_REBOOT);
		

	HAL::Set(HAL::EDisplayState, 0);
	Restart(KtRestartReasonBootRestart | KtRestartReasonRAMImage);

	//  *** NOTREACHED ***
	}

GLDEF_C TInt E32Main()
	{
	CTrapCleanup* cleanup=CTrapCleanup::New();

	// Make bootloader panics fault the system. This means that this process
	// will NEVER END
	User::SetCritical(User::ESystemPermanent);

	TRAPD(error,RunAppL());
	__ASSERT_ALWAYS(!error, User::Panic(KPanicText, error));
	
	delete cleanup;

	PrintToScreen(_L("Boot loader ended abnormally")); 

	Ldd.Close();
	return 0;
	}

