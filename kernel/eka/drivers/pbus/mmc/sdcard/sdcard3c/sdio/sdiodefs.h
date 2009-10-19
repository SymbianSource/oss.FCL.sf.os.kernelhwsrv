// Copyright (c) 2003-2009 Nokia Corporation and/or its subsidiary(-ies).
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
// General register definitions for the SDIO controller
// 
//

/**
 @file sdiodefs.h
 @publishedPartner
 @released
*/

#ifndef __SDIODEFS_H__
#define __SDIODEFS_H__

//// Constants for Read/Write[Direct/Extended]

const TUint32 KSdioCmdDataMask		= 0x000000FF;	// CMD52/CMD53 Direction
const TUint32 KSdioCmdDirMask		= 0x80000000;	// CMD52/CMD53 Direction
const TUint32 KSdioCmdRead			= 0x00000000;	// CMD52/CMD53 Direction : Read
const TUint32 KSdioCmdWrite			= 0x80000000;	// CMD52/CMD53 Direction : Write
const TUint32 KSdioCmdRAW			= 0x08000000;	// CMD52 Read-After-Write
const TUint32 KSdioCmdByteMode		= 0x00000000;	// CMD53 Byte Mode
const TUint32 KSdioCmdBlockMode		= 0x08000000;	// CMD53 Block Mode
const TUint32 KSdioCmdAutoInc		= 0x04000000;	// CMD53 Incrementing Address
const TUint32 KSdioCmdFIFO			= 0x00000000;	// CMD53 FIFO Mode

const TUint32 KSdioCmdFunctionMask  = 0x03;
const TUint32 KSdioCmdFunctionShift = 28;

const TUint32 KSdioCmdAddressMask		 = 0x1FFFF;
const TUint32 KSdioCmdAddressShift		 = 9;
const TUint32 KSdioCmdAddressMaskShifted = KSdioCmdAddressMask << KSdioCmdAddressShift;
const TUint32 KSdioCmdAddressAIncVal	 = 1 << KSdioCmdAddressShift;

const TUint32 KSdioCmdCountMask		= 0x1FF;

//// R4 Response Bitfields

const TUint32 KSDIOMemoryPresent		= KBit27;					// 32bit SDIO R4 Response
const TUint32 KSDIOFunctionCountMask	= KBit28 | KBit29 | KBit30;	// 32bit SDIO R4 Response
const TUint32 KSDIOFunctionCountShift	= 28;						// 32bit SDIO R4 Response
const TUint32 KSDIOReady				= KBit31;					// 32bit SDIO R4 Response
const TUint32 KSDIOOCRMask				= 0x00FFFFFF;				// 32bit SDIO R4 Response

//// Card Common Control Registers (CCCR)

const TUint32 KCCCRRegSdioRevision		  = 0x00;
const TUint32 KCCCRRegSdSpec			  = 0x01;
const TUint32 KCCCRRegIoEnable			  = 0x02;
const TUint32 KCCCRRegIoReady			  = 0x03;
const TUint32 KCCCRRegIntEnable			  = 0x04;
const TUint32 KCCCRRegIntPending		  = 0x05;
const TUint32 KCCCRRegIoAbort			  = 0x06;
const TUint32 KCCCRRegBusInterfaceControl = 0x07;
const TUint32 KCCCRRegCardCapability	  = 0x08;
const TUint32 KCCCRRegCisPtrLo			  = 0x09;
const TUint32 KCCCRRegCisPtrMid			  = 0x0a;
const TUint32 KCCCRRegCisPtrHi			  = 0x0b;
const TUint32 KCCCRRegBusSuspend		  = 0x0c;
const TUint32 KCCCRRegFunctionSelect	  = 0x0d;
const TUint32 KCCCRRegExecFlags			  = 0x0e;
const TUint32 KCCCRRegReadyFlags		  = 0x0f;
const TUint32 KCCCRRegFN0BlockSizeLo	  = 0x10;
const TUint32 KCCCRRegFN0BlockSizeHi	  = 0x11;
const TUint32 KCCCRRegPowerControl		  = 0x12;
const TUint32 KCCCRRegHighSpeed			  = 0x13;


//// Bit definitions for KCCCRRegIntEnable

const TUint8 KSDIOCardIntEnMaster	= KBit0;	// Master Interrupt Enable bit

//// Bit definitions for KCCCRRegIntEnable

const TUint8 KSDIOCardIntPendMask	= 0xfe;		// Pending interrupt mask

//// Bit definitions for KCCCRRegCardCapability

const TUint8 KSDIOCardCapsBit4BLS	= KBit7;	// 4-Bit mode supported for Low Speed devices
const TUint8 KSDIOCardCapsBitLSC	= KBit6;	// Card is a Low Speed device
const TUint8 KSDIOCardCapsBitE4MI	= KBit5;	// Enable interrupt between blocks in 4-bit mode
const TUint8 KSDIOCardCapsBitS4MI	= KBit4;	// Supports interrups between blocks in 4-bit mode
const TUint8 KSDIOCardCapsBitSBS	= KBit3;	// Supports Suspend/Resume
const TUint8 KSDIOCardCapsBitSRW	= KBit2;	// Supports Read/Wait
const TUint8 KSDIOCardCapsBitSMB	= KBit1;	// Supports Multi-Block
const TUint8 KSDIOCardCapsBitSDC	= KBit0;	// Supports Direct Commands during Mult-Byte transfer

//// Bit definitions for KCCCRRegIoAbort

const TUint8 KSDIOCardIoAbortReset = KBit3;		// IO Reset

//// Bit definitions for KCCCRRegBusInterfaceControl

const TUint8 KSDIOCardBicBitBusWidth4 = KBit1;
const TUint8 KSDIOCardBicMaskBusWidth = KBit0 | KBit1;
const TUint8 KSDIOCardBicBitCdDisable = KBit7;

//// Bit definitions for KCCCRRegHighSpeed

const TUint8 KSDIOCardHighSpeedSHS 	= KBit0;
const TUint8 KSDIOCardHighSpeedEHS 	= KBit1;

//// FBR Offsets

const TUint32 KFBRFunctionOffset = 0x100;

//// Function Basic Registers (FBR)

const TUint8 KFBRRegInterfaceCode = 0x00;
const TUint8 KFBRRegExtendedCode  = 0x01;
const TUint8 KFBRRegPowerFlags	  = 0x02;
const TUint8 KFBRRegCisPtrLo	  = 0x09;
const TUint8 KFBRRegCisPtrMid	  = 0x0a;
const TUint8 KFBRRegCisPtrHi	  = 0x0b;
const TUint8 KFBRRegCsaPtrLo	  = 0x0c;
const TUint8 KFBRRegCsaPtrMid	  = 0x0d;
const TUint8 KFBRRegCsaPtrHi	  = 0x0e;
const TUint8 KFBRRegCsaWindow	  = 0x0f;
const TUint8 KFBRRegIoBlockSizeLo = 0x10;
const TUint8 KFBRRegIoBlockSizeHi = 0x11;

//// FBR Register Bit Definitions

const TUint8 KFBRRegInterfaceCodeMask = KBit0 | KBit1 | KBit2 | KBit3;
const TUint8 KFBRRegSupportsCSA		  = KBit6;
const TUint8 KFBRRegEnableCSA		  = KBit7;

const TUint8 KFBRRegEnableHighPower		= KBit2;
const TUint8 KFBRRegPowerSupportMask	= KBit0 | KBit1;

const TUint8 KFBRRegStandardPower		= 0x00;
const TUint8 KFBRRegHighPowerSupported	= KBit1;
const TUint8 KFBRRegHighPowerRequired	= KBit0 | KBit1;

////

const TUint32 KSDIOCccrLength = 0x14;
// FBR only reads upto the CSA Data Pointer, it should never include the CSA Data Window
const TUint32 KSDIOFbrLength  = 0x0e;

////
	
const TUint32 KCommandSessionBlocked = KBit0;
const TUint32 KDataSessionBlocked    = KBit1;		

////
	
const TUint32 KSDIOBusWidth1=0x00;	
const TUint32 KSDIOBusWidth4=0x02;
const TUint32 KSDIOStatusBlockLength = 0x40;

////

#endif	// #ifndef __SDIODEFS_H__
