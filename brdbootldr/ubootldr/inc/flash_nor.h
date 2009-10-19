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

#ifndef __FLASH_NOR_H__
#define __FLASH_NOR_H__

// If Flash parameters are not already defined, set them to Tyax values.
#ifndef FLASHERASEBLOCKSIZE
#define FLASHERASEBLOCKSIZE 0x20000 //128KB
#endif
#ifndef FLASHWRITEBUFSIZE
#define FLASHWRITEBUFSIZE 0x40 //64 bytes
#endif

///////////////////////////////////////////////////////////////////////////////
//
// CFI - flash identification
//
///////////////////////////////////////////////////////////////////////////////
enum CfiManifacturerId
	{
	CFI_MANUF_SPANSION = 0x01,
	CFI_MANUF_INTEL    = 0x89,
	CFI_MANUF_ANY      = (TUint16) -1, // some manufacturers' flash chips comform to one standard CFI command set
	};

enum CfiDeviceId
	{
	CFI_DEV_S29GL512N  = 0x227e,
	CFI_DEV_SIBLEY     = 0x88b1,          // Intel Sibley as found on 3430 SDP (H6)
	CFI_DEV_28F256L18T = 0x880d,          // Intel Tyax as found on my H4
	CFI_DEV_ANY        = (TUint16) -1,    // some manufacturers' flash chips comform to one standard CFI command set
	};

typedef struct
	{
	TPtrC   name;
	TUint16	manufacturerId;
	TUint16 deviceId;

	TInt (*reset)(TUint32 flashId, TUint32 address);
	TInt (*erase)(TUint32 flashId, TUint32 aBase,  TUint32 anAddr, TUint32 aSize);
	TInt (*write)(TUint32 flashId, TUint32 anAddr, TUint32 aSize,  const TUint32* aPS);

	TUint   blockSize;  // the physical block size of the flash
	}
TFlashInfo;

const TUint	FLASH_TYPE_UNKNOWN        = 0;

///////////////////////////////////////////////////////////////////////////////
//
// CFI - generic command processing
//
///////////////////////////////////////////////////////////////////////////////
typedef struct
	{
	TUint32 location; // where to write this command to
	TUint32	offset;   // the offset for this command
	TUint32 command;  // the command itself
	}
TCfiCommands;

enum
	{
	CFI_BASE8,   // use the base   address for this 8 bit command
	CFI_SECTOR8, // use the sector address for this 8 bit command

	CFI_END = (TUint32) -1 // used to mark the end of the command sequence
	};



const TUint32 KFlashEraseBlockSize = FLASHERASEBLOCKSIZE;
const TUint32 KFlashWriteBufSize   = FLASHWRITEBUFSIZE;
const TUint32 KRebootDelaySecs     = 5; // Delay(S) between flashing bootldr to reboot

// Flash commands
//const TUint8	KCmdWordProgram        = 0x40 ;
const TUint8	KCmdBlockErase1        = 0x20 ;
const TUint8	KCmdBlockErase2        = 0xd0 ;
//const TUint8	KCmdEraseSuspend       = 0xb0 ;
//const TUint8	KCmdEraseResume        = 0xd0 ;
const TUint8	KCmdReadStatus         = 0x70 ; 
const TUint8	KCmdClearStatus        = 0x50 ;
const TUint8	KCmdReadArrayMode      = 0xFF ; 
const TUint8	KCmdClearBlockLockBit1 = 0x60 ;
const TUint8	KCmdClearBlockLockBit2 = 0xD0 ;
//const TUint8  KCmdSetBlockLockBit1   = 0x60 ;
//const TUint8	KCmdSetBlockLockBit2   = 0x01 ;

// Flash status
const TUint8	KStatusBusy            = 0x80 ;
//const TUint8	KStatusProgramError    = 0x38 ;
//const TUint8	KStatusVoltageError    = 0x08 ;
const TUint8	KStatusCmdSeqError     = 0x30 ;
const TUint8	KStatusLockBitError    = 0x20 ;

const TUint8    KCmdWriteStatusSibley  = 0xE9; // Sibley write
const TUint8    KCmdWriteStatus        = 0xE8; // send Tyax write

GLREF_C TUint32 * GetFlashChunk(void);
GLREF_C TBool     BlankCheck   (TUint32 anAddr, TUint32 aSize);
GLREF_C TInt      Erase        (TUint32 anAddr, TUint32 aSize);
GLREF_C TInt      Write        (TUint32 anAddr, TUint32 aSize, const TUint32* aPS);

#endif
