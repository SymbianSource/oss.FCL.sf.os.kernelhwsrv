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
// ubootldr\flash_nor.cpp
// 
//

#define FILE_ID	0x464C5348

#include "bootldr.h"
#include "ubootldrldd.h"
#include <e32std.h>
#include <e32std_private.h>
#include <e32svr.h>
#include <e32cons.h>
#include <f32file.h>
#include <hal.h>
#include <u32hal.h>
#include "flash_nor.h"

#define KFlashRetries 1000000

#ifdef __SUPPORT_FLASH_REPRO__
_LIT(KLitThreadName,"Flash");

TLinAddr FlashImageAddr;
TUint32 FlashImageSize;
TUint32 * FlashAddress;

volatile TUint32 Available;
volatile TBool Complete;

#define addr_to_page(a) (a&~(0x1000-1))
#define addr_pageoff(a) (a&(0x1000-1))

// Memory
RUBootldrLdd LddFlash;
RChunk TheFlashChunk;

TUint FlashId = FLASH_TYPE_UNKNOWN;


#define PRINTF(x)
#define SPANSION_PRINTF(x)
#define TYAX_PRINTF(x)
#define WRITE_PRINTF(x)

// Reset prototypes
TInt cfiReset (TUint32 flashId, TUint32 address);
TInt tyaxReset(TUint32 flashId, TUint32 address);

// Erase prototypes
TInt spansionErase(TUint32 flashId, TUint32 aBase,  TUint32 anAddr, TUint32 aSize);
TInt tyaxErase    (TUint32 flashId, TUint32 aBase,  TUint32 anAddr, TUint32 aSize);

// Write prototypes
TInt spansionWrite(TUint32 flashId, TUint32 anAddr, TUint32 aSize, const TUint32* aPS);
TInt tyaxWrite    (TUint32 flashId, TUint32 anAddr, TUint32 aSize, const TUint32* aPS);

///////////////////////////////////////////////////////////////////////////////
//
// FLASH INFO
//
// This table holds all the information we have about supported flash devices
//
///////////////////////////////////////////////////////////////////////////////
const TFlashInfo flashInfo [] =
	{
//	Description                Manufacturer ID     Device ID           Reset fn   Erase fn       Write fn       Comments
	{_L(""),                   CFI_MANUF_ANY,      CFI_DEV_ANY,        cfiReset,  NULL,          NULL,          }, // This is the catch-all entry in case we aren't initialised

//	{_L("Spansion xyz"),       CFI_MANUF_SPANSION, CFI_DEV_xyz,        xyzReset,  xyzErase,      xyzWrite,      }, // Put new Spansion flash types here, before the CFI_DEV_ANY, or they won't get detected
	{_L("Spansion S29GL512N"), CFI_MANUF_SPANSION, CFI_DEV_S29GL512N,  cfiReset,  spansionErase, spansionWrite, }, // NaviEngine Rev B & C
	{_L("Spansion Generic"),   CFI_MANUF_SPANSION, CFI_DEV_ANY,        cfiReset,  spansionErase, spansionWrite, }, // Generic Spansion flash types

//	{_L("Intel xyz"),          CFI_MANUF_INTEL,    CFI_DEV_xyz,        xyzReset,  xyzErase,      xyzWrite,      }, // Put new Intel flash types here, before the CFI_DEV_ANY, or they won't get detected
	{_L("Intel Sibley"),       CFI_MANUF_INTEL,    CFI_DEV_SIBLEY,     tyaxReset, tyaxErase,     tyaxWrite,     }, // H4 with Intel Tyax flash parts
	{_L("Intel 28F256L18T"),   CFI_MANUF_INTEL,    CFI_DEV_28F256L18T, tyaxReset, tyaxErase,     tyaxWrite,     }, // H4 with Intel Tyax flash parts
	{_L("Intel Tyax"),         CFI_MANUF_INTEL,    CFI_DEV_ANY,        tyaxReset, tyaxErase,     tyaxWrite,     }, // Generic Intel Tyax flash support

	// End Of Table - no more entries after here
	{_L(""),                   0,                  0,                  NULL,          NULL,          NULL           }  // NULL entry used to mark end of table
	};





///////////////////////////////////////////////////////////////////////////////
// CFI Commands
///////////////////////////////////////////////////////////////////////////////
// Query
const TCfiCommands CfiQuery [] =
	{
		{CFI_BASE8,   0xAAA,   0xAA},
		{CFI_BASE8,   0x555,   0x55},
		{CFI_BASE8,   0xAAA,   0x90},

		{CFI_END,     CFI_END, CFI_END} // Termination of command sequence - this entry is not a command
	};

// Erase
const TCfiCommands CfiErase [] =
	{
		{CFI_BASE8,   0xAAA,   0xAA},
		{CFI_BASE8,   0x555,   0x55},
		{CFI_BASE8,   0xAAA,   0x80},
		{CFI_BASE8,   0xAAA,   0xAA},
		{CFI_BASE8,   0x555,   0x55},
		{CFI_SECTOR8, 0x000,   0x30},

		{CFI_END,     CFI_END, CFI_END} // Termination of command sequence - this entry is not a command
	};

// Write
const TCfiCommands CfiWrite [] =
	{
		{CFI_BASE8,   0xAAA,   0xAA},
		{CFI_BASE8,   0x555,   0x55},
		{CFI_BASE8,   0xAAA,   0xA0},

		{CFI_END,     CFI_END, CFI_END} // Termination of command sequence - this entry is not a command
	};






///////////////////////////////////////////////////////////////////////////////
//
// CFI Command execution
//
// CFI implements a generic set of commands that can be used on all CFI flash
// parts.
//
// The commands usually write to the base address of the device + an offset,
// or to the sector/block address for some commands.
//
///////////////////////////////////////////////////////////////////////////////
TInt CfiCommand(TUint32 base, TUint32 sector, const TCfiCommands * commands)
	{
	if (commands != NULL)
		{
		const TCfiCommands * pCmd = commands;
		while (pCmd->location != CFI_END)
			{
			switch (pCmd->location)
				{
				case CFI_BASE8:
					{
					*(volatile TUint8*)(base   + pCmd->offset) = pCmd->command;
					}
					break;
				case CFI_SECTOR8:
					{
					*(volatile TUint8*)(sector + pCmd->offset) = pCmd->command;
					}
					break;
				default:
					return KErrNotSupported;
				}
			pCmd++;
			}
		}
	return KErrNone;
	}




///////////////////////////////////////////////////////////////////////////////
//
// TYAX specific routines
//
///////////////////////////////////////////////////////////////////////////////
// Clear the status register
///////////////////////////////////////////////////////////////////////////////
void tyaxClearStatus(TUint32 address)
	{
	volatile TUint16 *p = (TUint16 *)address;
	*p=KCmdClearStatus;	// clear status reg
	}

///////////////////////////////////////////////////////////////////////////////
// Wait until cmd completes
///////////////////////////////////////////////////////////////////////////////
void tyaxWaitUntilReady(TUint32 address, TUint16 cmd)
	{
	volatile TUint16 *pF = (TUint16 *)address;
	TUint16 s=0;
	TInt i=KFlashRetries;

	for (; i>0 && ((s&KStatusBusy)!=KStatusBusy); --i)	// check ready bit
		{
		*pF=cmd;
		s=*pF;
		}
	if (i==0)
		{
		PrintToScreen(_L("Write timed out"));
		BOOT_FAULT();
		}
	if (s&KStatusCmdSeqError)
		{
		PrintToScreen(_L("Write error s=%x pF=0x%x\n"), s, pF);
		}
	}

///////////////////////////////////////////////////////////////////////////////
// Unlock Flash
///////////////////////////////////////////////////////////////////////////////
void tyaxUnlock(TUint32 address)
	{
	TYAX_PRINTF(RDebug::Printf("tyaxUnlock(0x%08x)", address));
	TUint16 * pF = (TUint16*)address;
	// Unlock
	*pF=KCmdClearBlockLockBit1;
	*pF=KCmdClearBlockLockBit2;
	}













///////////////////////////////////////////////////////////////////////////////
//
// GENERIC - implementations of the generic routines
//
// - reset
// - erase
// - write
//
///////////////////////////////////////////////////////////////////////////////
// Reset Flash
///////////////////////////////////////////////////////////////////////////////
TInt cfiReset(TUint32 flashId, TUint32 address)
	{
	SPANSION_PRINTF(RDebug::Printf("cfiReset(0x%08x)", address));

	volatile TUint8 * p = (TUint8*)address;
	*(p)=0xF0;			// reset spansion flash
	return KErrNone;
	}

///////////////////////////////////////////////////////////////////////////////
// Reset Flash
///////////////////////////////////////////////////////////////////////////////
TInt tyaxReset(TUint32 flashId, TUint32 address)
	{
	TYAX_PRINTF(RDebug::Printf("tyaxReset(0x%08x)", address));

	TUint16 * p = (TUint16*)address;

	// clear the status register
	tyaxClearStatus((TUint32)address);

	// write to linear base and set strataflash into readarray mode
	*p=KCmdReadArrayMode;
	return KErrNone;
	}




///////////////////////////////////////////////////////////////////////////////
// Erase a block of flash
///////////////////////////////////////////////////////////////////////////////
TInt spansionErase(TUint32 flashId, TUint32 aBase, TUint32 anAddr, TUint32 aSize)
	{
	SPANSION_PRINTF(RDebug::Printf("spansionErase 0x%08x", anAddr));

	volatile TUint32 base=anAddr&~(KFlashEraseBlockSize-1);	// round base address down to block
	volatile TUint32 end=anAddr+aSize;
	end=(end+KFlashEraseBlockSize-1)&~(KFlashEraseBlockSize-1);	// round end address up to block
	TUint32 size=end-base;
	volatile TUint8* p=(volatile TUint8*)base;

	SPANSION_PRINTF(RDebug::Printf("Erase anAddr=0x%08x, aSize=0x%08x, base=0x%08x, end=0x%08x, size=0x%08x, p=0x%08x", anAddr, aSize, base, end, size, p));

	cfiReset(flashId, aBase);
	for (; size; size-=KFlashEraseBlockSize, p+=(KFlashEraseBlockSize>>1))
		{
		CfiCommand(aBase, base, CfiErase);

		TUint retries = KFlashRetries;
		while ((*(volatile TUint8*)anAddr != 0xFF) && (retries != 0))
			{
			retries--;
			}
		if (retries==0)
			{
			RDebug::Printf("Erase Failed anAddr=0x%08x, aSize=0x%08x, base=0x%08x, end=0x%08x, size=0x%08x, p=0x%08x", anAddr, aSize, base, end, size, p);
			}
		cfiReset(flashId, aBase);
		}
	return 0;
	}


///////////////////////////////////////////////////////////////////////////////
// Erase a block of flash
///////////////////////////////////////////////////////////////////////////////
TInt tyaxErase(TUint32 flashId, TUint32 aBase, TUint32 anAddr, TUint32 aSize)
	{
	TUint32 base=anAddr&~(KFlashEraseBlockSize-1);	// round base address down to block
	TUint32 end=anAddr+aSize;
	end=(end+KFlashEraseBlockSize-1)&~(KFlashEraseBlockSize-1);	// round end address up to block
	TUint32 size=end-base;
	volatile TUint16* p=(volatile TUint16*)base;

	// write to linear base and set strataflash into readarray mode
	*p=KCmdReadArrayMode;
	// clear the status register
	*p=KCmdClearStatus;
	for (; size; size-=KFlashEraseBlockSize, p+=(KFlashEraseBlockSize>>1))
		{
		// Unlock
		*p=KCmdClearBlockLockBit1;
		*p=KCmdClearBlockLockBit2;
		// Erase
		*p=KCmdBlockErase1;	// block erase
		*p=KCmdBlockErase2;	// block erase confirm

		// wait for the erase to finish
		while ((*p & KStatusBusy)!=KStatusBusy);

		// put the flash block back to normal
		TUint32 s=*p;
		*p=KCmdClearStatus;	// clear status reg
		*p=KCmdReadArrayMode;
		
		if (s & KStatusLockBitError)
			{
			// error
			RDebug::Printf("Erase Failed: addr:0x%x status: 0x%x", p, s);
			return (TUint32)p-anAddr+1;
			}
		}
	return 0;
	}


///////////////////////////////////////////////////////////////////////////////
// Write a block of flash
///////////////////////////////////////////////////////////////////////////////
TInt spansionWrite(TUint32 flashId, TUint32 anAddr, TUint32 aSize, const TUint32* aPS)
// Assume aSize <= KFlashWriteBufSize
	{
	SPANSION_PRINTF(WRITE_PRINTF(RDebug::Printf("spansionWrite anAddr=0x%08x, aSize=0x%08x", anAddr, aSize)));

	volatile TUint8  * base  = (TUint8 *)FlashAddress;
	volatile TUint16 * pDest = (TUint16*)anAddr;
	volatile TUint16 * pSrc  = (TUint16*)aPS;
	volatile TUint16 * pEnd  = (TUint16*)(anAddr+aSize);

	for (; pDest < pEnd; pDest++, pSrc++)
		{
		CfiCommand((TUint32)base, (TUint32)base, CfiWrite);
		*pDest = *pSrc;

		TUint retries = KFlashRetries;
		while ((*pDest != *pSrc) && (retries != 0))
			{
			retries--;
			}

		if (*pDest != *pSrc)
			{
			RDebug::Printf("Write failed 0x%x=0x%x == 0x%x", pDest, *pSrc, *pDest);
			return 1;
			}
		}
	return 0;
	}

///////////////////////////////////////////////////////////////////////////////
// Write a block of flash
///////////////////////////////////////////////////////////////////////////////
// Assume aSize <= KFlashWriteBufSize
TInt tyaxWrite(TUint32 flashId, TUint32 anAddr, TUint32 aSize, const TUint32* aPS)
	{
	TYAX_PRINTF(WRITE_PRINTF(RDebug::Printf("tyaxWrite anAddr=0x%08x, aSize=0x%08x", anAddr, aSize)));

	volatile TUint16* pF=(volatile TUint16*)anAddr;

	tyaxUnlock(anAddr);
	tyaxClearStatus(anAddr);

	if (flashInfo[flashId].deviceId == CFI_DEV_SIBLEY)
		{
		tyaxWaitUntilReady(anAddr, KCmdWriteStatusSibley);
		}
		else
		{
		tyaxWaitUntilReady(anAddr, KCmdWriteStatus);
		}

	// convert to words - 1
	TInt16 l=(aSize>>1)-1;
	*pF=l;										// Write no of words
	const TUint16* pS=(const TUint16*)aPS;
	for (;l>=0;l--)
		{
		*pF++=*pS++;
		}
	pF=(volatile TUint16*)anAddr;
	*pF=0xD0;									// Confirm
		
	tyaxWaitUntilReady(anAddr, KCmdReadStatus);
	tyaxReset(flashId, anAddr);

	return 0;
	}












///////////////////////////////////////////////////////////////////////////////
//
// WRAPPERS
//
// A top level routine to prevent each function checking the flash type
//
///////////////////////////////////////////////////////////////////////////////
TInt flashReset(TUint32 flashId, TUint32 address)
	{
	PRINTF(RDebug::Printf("flashReset()"));

	TInt retVal = KErrNotSupported;

	if (flashInfo[flashId].reset != NULL)
		{
		retVal = flashInfo[flashId].reset(flashId, address);
		}

	return retVal;
	}

TInt flashErase(TUint32 flashId, TUint32 base, TUint32 address, TUint32 size)
	{
	PRINTF(RDebug::Printf("flashErase()"));

	TInt retVal = KErrNone;

	if (flashInfo[flashId].erase != NULL)
		{
		retVal = flashInfo[flashId].erase(flashId, base, address, size);
		}

	return retVal;
	}

TInt flashWrite(TUint32 flashId, TUint32 anAddr, TUint32 aSize, const TUint32* aPS)
	{
	WRITE_PRINTF(RDebug::Printf("flashWrite()"));

	TInt retVal = KErrNone;

	if (flashInfo[flashId].write != NULL)
		{
		retVal = flashInfo[flashId].write(flashId, anAddr, aSize, aPS);
		}
	
	return retVal;
	}


///////////////////////////////////////////////////////////////////////////////
//
// Flash ID
//
// Identify the flash part at the given address
// returns an index into the flashInfo structure
///////////////////////////////////////////////////////////////////////////////
TInt flashId(TUint32 address)
	{
	TUint deviceIndex = FLASH_TYPE_UNKNOWN;

	volatile TUint16* p16=(volatile TUint16*)address; // used for 16 bit read/write to the flash

	// Put flash into CFI query mode using 8 bit writes
	CfiCommand(address, address, CfiQuery);

	// Read ID codes using 16 bit reads
	// if we ever need to support 8 bit devices, we may need to change this to 2 x 8 bit reads per attribute
	TUint16 manufacturerId = *(p16  );
	TUint16 deviceId       = *(p16+1);

	for (TUint32 i=0; flashInfo[i].manufacturerId !=0; i++)
		{
		PRINTF(RDebug::Printf("Check device: M 0x%04x D 0x%04x", flashInfo[i].manufacturerId, flashInfo[i].deviceId));

		if (  (  flashInfo[i].manufacturerId == manufacturerId)
		   && ( (flashInfo[i].deviceId       == CFI_DEV_ANY   ) // support generic flash devices
		      ||(flashInfo[i].deviceId       == deviceId      )
			  )
		   )
			{
			PRINTF(RDebug::Print(_L("Found device: %s (Manufacturer=%x Device=%x)"), flashInfo[i].name.Ptr(), flashInfo[i].manufacturerId, flashInfo[i].deviceId));
			deviceIndex = i;
			break;
			}
		}
	if (deviceIndex == FLASH_TYPE_UNKNOWN)
		{
		RDebug::Printf("Flash type unknown: Manufacturer ID = %04x, Device ID = %04x", manufacturerId, deviceId );
		}
	flashReset(deviceIndex, (TUint32)FlashAddress);
	return deviceIndex;
	}


///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////





GLDEF_C TUint32 * GetFlashChunk()
	{
	// return if already initialised
	if (FlashAddress != NULL)
		return FlashAddress;

	TInt r = User::LoadLogicalDevice(KBootldrLddName);

	r = LddFlash.Open();
	if (r!=KErrNone)
			{
			PrintToScreen(_L("FAULT due to LddFlash open\r\n"));
			BOOT_FAULT();
			}

	TUint8* kernelAddress;
	r=LddFlash.CreateChunk(KNORFlashTargetSize,(TAny**)&kernelAddress);
	if (r!=KErrNone)
			{
			PrintToScreen(_L("FAULT due to chunk create\r\n"));
			BOOT_FAULT();
			}

	// If we're running from RAM flash will be in a different place...
	r = LddFlash.CommitMemory(KNORFlashTargetSize,addr_to_page(KNORFlashTargetAddr));
	if (r!=KErrNone)
			{
			PrintToScreen(_L("FAULT due to commit\r\n"));
			BOOT_FAULT();
			}

	r = LddFlash.GetChunkHandle(TheFlashChunk);
	if (r!=KErrNone)
			{
			PrintToScreen(_L("FAULT due to handle\r\n"));
			BOOT_FAULT();
			}

	TUint8* Base = TheFlashChunk.Base();
	FlashAddress = (TUint32*)Base;
	FlashId      = flashId((TUint32)FlashAddress);

	return FlashAddress;
	}

GLDEF_C void NotifyDataAvailable(TInt aTotalAmount)
	{
	Available=(TUint32)aTotalAmount;
	}

GLDEF_C void NotifyDownloadComplete()
	{
	Complete=ETrue;
	}

GLDEF_C TBool BlankCheck(TUint32 anAddr, TUint32 aSize)
	{
	const TUint16* p=(const TUint16*)anAddr;
	const TUint16* pE=p+(aSize>>1);
	TBool rv=ETrue;

	while(p<pE)
		{
		if (*p!=0xffff)
			{
			PRINTF(RDebug::Printf("BlankCheck %x is not blank! anAddr=0x%08x, aSize=0x%08x, p=0x%08x, *p=0x%08x", anAddr, anAddr, aSize, (TUint32)p, (TUint32)*p));
			rv=EFalse;
			break;
			}
		p++;
		}
	if (rv)
		{
		PRINTF(RDebug::Printf("BlankCheck: %x is blank", anAddr));
		}
	return rv;
	}

///////////////////////////////////////////////////////////////////////////////
//
// Erase
//
// This function is used by the variant code.  The variant code shouldn't care
// about the Flash ID, so I've left this function here as a wrapper for the
// internal flashErase function, passing in a nasty global variable containing
// the Flash ID.
//
///////////////////////////////////////////////////////////////////////////////
GLDEF_C TInt Erase(TUint32 anAddr, TUint32 aSize)
	{
	flashErase(FlashId, (TUint32)FlashAddress, anAddr, aSize);
	return 0;
	}


///////////////////////////////////////////////////////////////////////////////
//
// Write
//
// This function is used by the variant code.  As well as the Flash ID comment
// from above (see Erase), the variant shouldn't have to care about internal
// buffer sizes, etc.
//
///////////////////////////////////////////////////////////////////////////////
GLDEF_C TInt Write(TUint32 anAddr, TUint32 aSize, const TUint32* aPS)
	{
	TInt rv=0;
	do
		{
		if ((rv=flashWrite(FlashId, anAddr, KFlashWriteBufSize, aPS))!=0)
			{
			break;
			}
		anAddr+=KFlashWriteBufSize;
		aPS+=KFlashWriteBufSize>>2;
		aSize-=KFlashWriteBufSize;
		} while(aSize);
	return rv;
	}

TInt FlashThread(TAny*)
	{
	// If this thread crashes we want it to take the system down
	User::SetCritical(User::ESystemPermanent);

	GetFlashChunk();
	if (FlashBootLoader)
		{
		PrintToScreen(_L("*** Reflashing bootloader ***\r\n"));
		FlashImageAddr=(TLinAddr)FlashAddress;
		// sanity check...
		if ((TUint32)ImageSize > KNORFlashMaxBootloaderSize)
			{
			PrintToScreen(_L("Image is larger than the flash area (%d > %d) bytes.\r\n"), ImageSize, KNORFlashMaxBootloaderSize);
			return KErrNotSupported;
			}
		}
	else
		{
		PrintToScreen(_L("*** Writing to NOR Flash ***\r\n"));
		FlashImageAddr=(TLinAddr)FlashAddress+KNORFlashMaxBootloaderSize;

		// sanity check...
		if ((TUint32)ImageSize > KNORFlashMaxImageSize)
			{
			PrintToScreen(_L("Image is larger than the flash area (%d > %d) bytes.\r\n"), ImageSize, KNORFlashMaxImageSize);
			return KErrNotSupported;
			}
		}

	FlashImageSize=(TUint32)ImageSize;
	Complete=EFalse;

	TUint32 imgSzMb=(FlashImageSize+0xfffff)&~0xfffff;	// round image size up to 1Mb

	InitProgressBar(1,imgSzMb,_L("ERASE"));
	TUint32 base=FlashImageAddr;
	TUint32 end=base+imgSzMb;
	TInt r=KErrNone;
	while(base<end)
		{
		if (!BlankCheck(base,KFlashEraseBlockSize))
			{
			r=Erase(base, KFlashEraseBlockSize);
			if (r!=KErrNone)
				{
				PrintToScreen(_L("Erase failed 0x%x\r\n"), r);
				RDebug::Printf("Erase failed 0x%x", r);
				// make this a rdebug
				BOOT_FAULT();
				}
			}
		if (!BlankCheck(base,KFlashEraseBlockSize))
			{
			PrintToScreen(_L("BlankCheck failed 0x%x\r\n"),base);
			RDebug::Printf("BlankCheck failed at adress 0x%08x with error code 0x%x",base,r);
			//BOOT_FAULT();	// why crash at this point, retry is better, surely?
			}
		else
			{
			// only move to next block and update progress if the block erase passed
			base+=KFlashEraseBlockSize;
			UpdateProgressBar(1,base-FlashImageAddr);
			}
		}

	base=FlashImageAddr;
	while(base<end)
		{

		if (!BlankCheck(base,KFlashEraseBlockSize))
			{
			PrintToScreen(_L("BlankCheck 2 failed 0x%x\r\n"),base);
			RDebug::Printf("BlankCheck 2 failed at adress 0x%08x with error code 0x%x",base,r);
			BOOT_FAULT();
			}
		base+=KFlashEraseBlockSize;
		}

	InitProgressBar(1,FlashImageSize,_L("WRITE"));
	TUint32 source=DestinationAddress();		// start of image in RAM
	if (ImageHeaderPresent)
		source+=256;							// skip header if present
	TUint32 target=FlashImageAddr;						// target in flash
	TBool complete=EFalse;
	TUint32 used_bytes=0;

	// while the image hasn't been written fully
	while ((target-FlashImageAddr) < FlashImageSize)
		{
		used_bytes=source-DestinationAddress();

		complete=Complete;					// must check Complete before Available

		// if there isn't anything ready, go back to the top
		if (Available<(used_bytes+256) && !complete)
			{
			continue;									// wait for 256 bytes more data
			}
		TUint32 write_block_size=Available-used_bytes;	// how much is ready
		write_block_size &= ~(KFlashWriteBufSize-1);	// only write whole buffers

		while (write_block_size)
			{
			TUint32 write_size=Min(write_block_size,(TUint32)0x400);	// update progress after each 1K
			r=Write(target,write_size,(const TUint32*)source);
			if (r!=KErrNone)
				{
				PrintToScreen(_L("Write failed 0x%x"),r);
				BOOT_FAULT();
				}

			target+=write_size;
			source+=write_size;
			write_block_size-=write_size;
			UpdateProgressBar(1,target-FlashImageAddr);
			}
		}

	PrintToScreen(_L("Verifying image...\r\n"));

	source=DestinationAddress();				// start of image in RAM
	if (ImageHeaderPresent)
		source+=256;							// skip header if present
	base=FlashImageAddr;
	volatile TUint16* pRam=(volatile TUint16*)source;
	volatile TUint16* pFlash=(volatile TUint16*)base;
	volatile TUint16* pFlashEnd=pFlash+(FlashImageSize>>1);

	InitProgressBar(1, FlashImageSize, _L("VERIFY"));
	while(pFlash<pFlashEnd)
		{
		if (*pFlash++ != *pRam++)
			{
			PrintToScreen(_L("Verify error at byte %d (0x%x != 0x%x)\r\n"),
				((pFlash-1) - (volatile TUint16*)base) * 2, (*(pFlash-1)), (*(pRam-1)));

			PrintToScreen(_L("VERIFY %d"),(TInt)(pFlash-1));
			BOOT_FAULT();
			}

		if (!((TUint32)pFlash % 0x400))
			UpdateProgressBar(1,(TUint32)pFlash-(TUint32)FlashImageAddr);
		}

	PrintToScreen(_L("Verify complete\r\n"));

	if (FlashBootLoader)
		{
		PrintToScreen(_L("Rebooting in %d Seconds...\r\n"), KRebootDelaySecs);

		InitProgressBar(1, KRebootDelaySecs, _L("DELAY "));
		for (TUint i=0 ; i<KRebootDelaySecs ; ++i)
			{
			User::After(1000000);	// Sleep in millisecs
			UpdateProgressBar(1, i);
			}
		UpdateProgressBar(1, KRebootDelaySecs);	// let it get to the end
		PrintToScreen(_L("Rebooting...\r\n"));
		User::After(10000);
		Restart(KtRestartReasonHardRestart);
		}

	PrintToScreen(_L("Booting Image...\r\n"));
	Restart(KtRestartReasonBootRestart | KtRestartReasonNORImage);

	// NOTREACHED
	return 0;
	}

GLDEF_C TInt InitFlashWrite()
	{
	// start thread
	RThread t;
	TInt r=t.Create(KLitThreadName,FlashThread,0x2000,NULL,NULL);
	if (r!=KErrNone)
		{
		return r;
		}
	t.SetPriority(EPriorityLess);
	t.Resume();
	return KErrNone;
	}
#endif	//__SUPPORT_FLASH_REPRO__
