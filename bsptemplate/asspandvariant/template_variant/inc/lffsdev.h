// Copyright (c) 2004-2009 Nokia Corporation and/or its subsidiary(-ies).
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
// template\template_variant\inc\lffsdev.h
// Header file for a Logging Flash file system (LFFS) physical device driver 
// for a standard Common Flash Interface (CFI) based NOR flash chip.
// This file is part of the Template Base port
// 
//

#ifndef __LFFSDEV_H__
#define __LFFSDEV_H__

#include <drivers/flash_media.h>
#include <template_assp.h>

#define FLASH_FAULT()	Kern::Fault("LFFSDEV",__LINE__)

//
// TO DO: (mandatory)
//
// Define the bus width (typically this will be 16 or 32)
// and the number of devices spanning the bus (typically 1, 2 or 4).
//
// Possible value are :	FLASH_BUS_WIDTH	FLASH_BUS_DEVICES
//						32				1					1 x 32 bit device
//						32				2					2 x 16 bit devices
//						32				4					4 x 8  bit devices
//						16				1					1 x 16 bit device
//						16				2					2 x 8  bit devices
//
//
#define FLASH_BUS_WIDTH 32		// EXAMPLE_ONLY
#define FLASH_BUS_DEVICES 2		// EXAMPLE_ONLY



#if FLASH_BUS_WIDTH == 32
	#define TFLASHWORD TUint32
	#define FLASH_BYTES_TO_WORDS(aNumBytes) (aNumBytes >> 2)
	#define FLASH_ADDRESS_IN_BYTES(aWordAddr) (aWordAddr << 2)
	#define FLASH_ERASE_WORD_VALUE 0xFFFFFFFF
#elif FLASH_BUS_WIDTH == 16
	#define TFLASHWORD TUint16
	#define FLASH_BYTES_TO_WORDS(aNumBytes) (aNumBytes >> 1)
	#define FLASH_ADDRESS_IN_BYTES(aWordAddr) (aWordAddr << 1)
	#define FLASH_ERASE_WORD_VALUE 0xFFFF
#else	// FLASH_BUS_WIDTH == 8
	#define TFLASHWORD TUint8
	#define FLASH_BYTES_TO_WORDS(aNumBytes) aNumBytes
	#define FLASH_ADDRESS_IN_BYTES(aWordAddr) aWordAddr
	#define FLASH_ERASE_WORD_VALUE 0xFF
#endif

#define BUS_WIDTH_PER_DEVICE (FLASH_BUS_WIDTH / FLASH_BUS_DEVICES)

/********************************************
Common Flash Interface (CFI) definitions for various
combinations of FLASH_BUS_WIDTH and FLASH_BUS_DEVICES
 ********************************************/
//
// TO DO: (optional)
// Delete the definitions that are not applicable to your device
//
#if FLASH_BUS_WIDTH == 32 && FLASH_BUS_DEVICES == 1			// 1x32bit devices on 32bit bus

const TFLASHWORD KCmdReadArray				= 0x000000FF;	// Set read array command
const TFLASHWORD KCmdReadStatusRegister		= 0x00000070;	// Read status register command
const TFLASHWORD KCmdClearStatusRegister	= 0x00000050;	// Clear status register error bits command
const TFLASHWORD KCmdWriteToBuffer			= 0x000000E8;	// Write to buffer setup command
const TFLASHWORD KCmdBlockErase				= 0x00000020;	// Block erase command
const TFLASHWORD KCmdEraseSuspend			= 0x000000B0;	// Erase suspend command
const TFLASHWORD KCmdEraseResume			= 0x000000D0;	// Erase resume command (actually a confirm)
const TFLASHWORD KCmdConfirm				= 0x000000D0;	// Confirm command
const TFLASHWORD KCmdReadIdentifiers		= 0x00000090;	// Read Flash identifiers command
const TFLASHWORD KCmdReadQuery				= 0x00000098;	// Read Flash Query info command

const TFLASHWORD KStsReady					= 0x00000080;	// Ready bit
const TFLASHWORD KStsSuspended				= 0x00000040;	// Suspend bit
const TFLASHWORD KStsEraseError				= 0x00000020;	// Erase error bit
const TFLASHWORD KStsWriteError				= 0x00000010;	// Write error bit
const TFLASHWORD KStsVppLow					= 0x00000008;	// Vpp low bit
const TFLASHWORD KStsReserved				= 0x00000004;	// Reserved bit (not used)
const TFLASHWORD KStsLocked					= 0x00000002;	// Locked bit
const TFLASHWORD KStsReserved2				= 0x00000001;	// Reserved bit

#elif FLASH_BUS_WIDTH == 32 && FLASH_BUS_DEVICES == 2		// 2x16bit devices on 32bit bus

const TFLASHWORD KCmdReadArray				= 0x00FF00FF;	// Set read array command
const TFLASHWORD KCmdReadStatusRegister		= 0x00700070;	// Read status register command
const TFLASHWORD KCmdClearStatusRegister	= 0x00500050;	// Clear status register error bits command
const TFLASHWORD KCmdWriteToBuffer			= 0x00E800E8;	// Write to buffer setup command
const TFLASHWORD KCmdBlockErase				= 0x00200020;	// Block erase command
const TFLASHWORD KCmdEraseSuspend			= 0x00B000B0;	// Erase suspend command
const TFLASHWORD KCmdEraseResume			= 0x00D000D0;	// Erase resume command (actually a confirm)
const TFLASHWORD KCmdConfirm				= 0x00D000D0;	// Confirm command
const TFLASHWORD KCmdReadIdentifiers		= 0x00900090;	// Read Flash identifiers command
const TFLASHWORD KCmdReadQuery				= 0x00980098;	// Read Flash Query info command

const TFLASHWORD KStsReady					= 0x00800080;	// Ready bit
const TFLASHWORD KStsSuspended				= 0x00400040;	// Suspend bit
const TFLASHWORD KStsEraseError				= 0x00200020;	// Erase error bit
const TFLASHWORD KStsWriteError				= 0x00100010;	// Write error bit
const TFLASHWORD KStsVppLow					= 0x00080008;	// Vpp low bit
const TFLASHWORD KStsReserved				= 0x00040004;	// Reserved bit (not used)
const TFLASHWORD KStsLocked					= 0x00020002;	// Locked bit
const TFLASHWORD KStsReserved2				= 0x00010001;	// Reserved bit

#elif FLASH_BUS_WIDTH == 32 && FLASH_BUS_DEVICES == 4		// 4x8bit devices on 32bit bus

const TFLASHWORD KCmdReadArray				= 0xFFFFFFFF;	// Set read array command
const TFLASHWORD KCmdReadStatusRegister		= 0x70707070;	// Read status register command
const TFLASHWORD KCmdClearStatusRegister	= 0x50505050;	// Clear status register error bits command
const TFLASHWORD KCmdWriteToBuffer			= 0xE8E8E8E8;	// Write to buffer setup command
const TFLASHWORD KCmdBlockErase				= 0x20202020;	// Block erase command
const TFLASHWORD KCmdEraseSuspend			= 0xB0B0B0B0;	// Erase suspend command
const TFLASHWORD KCmdEraseResume			= 0xD0D0D0D0;	// Erase resume command (actually a confirm)
const TFLASHWORD KCmdConfirm				= 0xD0D0D0D0;	// Confirm command
const TFLASHWORD KCmdReadIdentifiers		= 0x90909090;	// Read Flash identifiers command
const TFLASHWORD KCmdReadQuery				= 0x98989898;	// Read Flash Query info command

const TFLASHWORD KStsReady					= 0x80808080;	// Ready bit
const TFLASHWORD KStsSuspended				= 0x40404040;	// Suspend bit
const TFLASHWORD KStsEraseError				= 0x20202020;	// Erase error bit
const TFLASHWORD KStsWriteError				= 0x10101010;	// Write error bit
const TFLASHWORD KStsVppLow					= 0x08080808;	// Vpp low bit
const TFLASHWORD KStsReserved				= 0x04040404;	// Reserved bit (not used)
const TFLASHWORD KStsLocked					= 0x02020202;	// Locked bit
const TFLASHWORD KStsReserved2				= 0x01010101;	// Reserved bit

#elif FLASH_BUS_WIDTH == 16 && FLASH_BUS_DEVICES == 1	// 1x16bit devices on 16bit bus

const TFLASHWORD KCmdReadArray				= 0x00FF;	// Set read array command
const TFLASHWORD KCmdReadStatusRegister		= 0x0070;	// Read status register command
const TFLASHWORD KCmdClearStatusRegister	= 0x0050;	// Clear status register error bits command
const TFLASHWORD KCmdWriteToBuffer			= 0x00E8;	// Write to buffer setup command
const TFLASHWORD KCmdBlockErase				= 0x0020;	// Block erase command
const TFLASHWORD KCmdEraseSuspend			= 0x00B0;	// Erase suspend command
const TFLASHWORD KCmdEraseResume			= 0x00D0;	// Erase resume command (actually a confirm)
const TFLASHWORD KCmdConfirm				= 0x00D0;	// Confirm command
const TFLASHWORD KCmdReadIdentifiers		= 0x0090;	// Read Flash identifiers command
const TFLASHWORD KCmdReadQuery				= 0x0098;	// Read Flash Query info command

const TFLASHWORD KStsReady					= 0x0080;	// Ready bit
const TFLASHWORD KStsSuspended				= 0x0040;	// Suspend bit
const TFLASHWORD KStsEraseError				= 0x0020;	// Erase error bit
const TFLASHWORD KStsWriteError				= 0x0010;	// Write error bit
const TFLASHWORD KStsVppLow					= 0x0008;	// Vpp low bit
const TFLASHWORD KStsReserved				= 0x0004;	// Reserved bit (not used)
const TFLASHWORD KStsLocked					= 0x0002;	// Locked bit
const TFLASHWORD KStsReserved2				= 0x0001;	// Reserved bit

#elif FLASH_BUS_WIDTH == 16 && FLASH_BUS_DEVICES == 2	// 2x8bit devices on 16bit bus
const TFLASHWORD KCmdReadArray				= 0xFFFF;	// Set read array command
const TFLASHWORD KCmdReadStatusRegister		= 0x7070;	// Read status register command
const TFLASHWORD KCmdClearStatusRegister	= 0x5050;	// Clear status register error bits command
const TFLASHWORD KCmdWriteToBuffer			= 0xE8E8;	// Write to buffer setup command
const TFLASHWORD KCmdBlockErase				= 0x2020;	// Block erase command
const TFLASHWORD KCmdEraseSuspend			= 0xB0B0;	// Erase suspend command
const TFLASHWORD KCmdEraseResume			= 0xD0D0;	// Erase resume command (actually a confirm)
const TFLASHWORD KCmdConfirm				= 0xD0D0;	// Confirm command
const TFLASHWORD KCmdReadIdentifiers		= 0x9090;	// Read Flash identifiers command
const TFLASHWORD KCmdReadQuery				= 0x9898;	// Read Flash Query info command

const TFLASHWORD KStsReady					= 0x8080;	// Ready bit
const TFLASHWORD KStsSuspended				= 0x4040;	// Suspend bit
const TFLASHWORD KStsEraseError				= 0x2020;	// Erase error bit
const TFLASHWORD KStsWriteError				= 0x1010;	// Write error bit
const TFLASHWORD KStsVppLow					= 0x0808;	// Vpp low bit
const TFLASHWORD KStsReserved				= 0x0404;	// Reserved bit (not used)
const TFLASHWORD KStsLocked					= 0x0202;	// Locked bit
const TFLASHWORD KStsReserved2				= 0x0101;	// Reserved bit

#endif

// address at which to issue the Query command
const TUint32 KCmdReadQueryOffset				= 0x55;	

const TUint32 KQueryOffsetQRY					= 0x10;
const TUint32 KQueryOffsetSizePower				= 0x27;
const TUint32 KQueryOffsetWriteBufferSizePower	= 0x2A;
const TUint32 KQueryOffsetErasePartitions		= 0x2C;
const TUint32 KQueryOffsetEraseBlockCount		= 0x2D;
const TUint32 KQueryOffsetEraseBlockSize		= 0x2F;

/********************************************
 * Driver definitions
 ********************************************/
const TInt KMaxWriteSetupAttempts		= 8;
const TInt KMaxEraseResumeAttempts		= 32;

const TInt KDataBufSize=1024;


// TO DO: (mandatory)
// Define the following timeouts in terms of timer ticks
// This is only example code... you may need to modify it for your hardware
// The examples given here assume a timer clock frequency of 3.6864MHz
const TUint32 KFlashWriteTimerPeriod =		1500;	// 1500 ticks @ 3.6864MHz = 406us
const TUint32 KFlashWriteTimerRetries =		3;		// total 1.2ms
const TUint32 KFlashSuspendTimerPeriod =	1500;	// 1500 ticks @ 3.6864MHz = 406us
const TUint32 KFlashSuspendTimerRetries =	3;		// total 1.2ms
const TUint32 KFlashEraseTimerPeriod =		100000;	// 100000 ticks @ 3.6864MHz = 27ms
const TUint32 KFlashEraseTimerRetries =		100;	// total 2.7sec
const TInt	  KEraseSuspendHoldOffTime =	130;	// 130ms

/********************************************
 * Media driver class
 ********************************************/
//
// TO DO: (optional)
//
// Add any private functions and data you require
//
class DMediaDriverFlashTemplate : public DMediaDriverFlash
	{
public:
	enum TWriteState {EWriteIdle=0,EWriting=1};
	enum TEraseState {EEraseIdle=0,EErase=1,EEraseNoSuspend=2,ESuspendPending=3,ESuspending=4,ESuspended=5};
	enum TEvent {EPollTimer=1,ETimeout=2,EHoldOffEnd=4};

	DMediaDriverFlashTemplate(TInt aMediaId);

	// replacing pure virtual - FLASH device specific stuff
	virtual TInt Initialise();
	virtual TUint32 EraseBlockSize();
	virtual TUint32 TotalSize();
	virtual TInt DoRead();
	virtual TInt DoWrite();
	virtual TInt DoErase();

private:
	void IPostEvents(TUint32 aEvents);
	void HandleEvents(TUint32 aEvents);
	void ClearEvents(TUint32 aEvents);
	void StartPollTimer(TUint32 aPeriod, TUint32 aRetries);
	void StartPollTimer();
	void StartHoldOffTimer();
	void CancelHoldOffTimer();
	void StartErase();
	void SuspendErase();
	void WriteStep();
	void DoFlashReady(TUint32 aStatus);
	void DoFlashTimeout();
	void StartPendingRW();
	TUint32 ReadQueryData8(TUint32 aOffset);
	TUint32 ReadQueryData16(TUint32 aOffset);
	void ReadFlashParameters();
	static void Isr(TAny* aPtr);
	static void HoldOffTimerFn(TAny* aPtr);
	static void EventDfc(TAny* aPtr);

private:
	TWriteState iWriteState;
	TEraseState iEraseState;
	TLinAddr iBase;
	TUint32 iEraseBlockSize;
	TUint32 iTotalSize;
	TUint32 iWriteBufferSize;
	TUint32 iWritePos;
	TInt iDataBufPos;
	TInt iDataBufRemain;
	TUint8* iData;		// data being written
	TUint32 iErasePos;
	NTimer iHoldOffTimer;
	TUint32 iEvents;
	TDfc iEventDfc;
	TUint32 iPollPeriod;
	TUint32 iPollRetries;
	TUint32 iEraseError;
	TInt iWriteError;
	TInt iEraseAttempt;
	DPlatChunkHw* iFlashChunk;
	};

#endif
