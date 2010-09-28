// Copyright (c) 1997-2009 Nokia Corporation and/or its subsidiary(-ies).
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
// template\template_assp\template_assp.h
// Definitions for Template ASSP
// 
// WARNING: This file contains some APIs which are internal and are subject
//          to change without notice. Such APIs should therefore not be used
//          outside the Kernel and Hardware Services package.
//

#ifndef __A32TEMPLATEV1_H__
#define __A32TEMPLATEV1_H__
#include <e32const.h>
#include <platform.h>
#include <e32hal.h>
#include <assp.h>
#include <kernel/kern_priv.h>
#include <drivers/rtclock.h>

//----------------------------------------------------------------------------
// Constant conventions:
//----------------------------------------------------------------------------

// KH		Hardware definition
// KHw		4-byte word definition prefix
// KHb		Byte definition prefix
// KHt		Bit definition prefix
// KHm		Mask definition prefix
// KHs		Shift definition prefix
// KHo		Offset definition prefix
// KHwRo	Read-only register
// KHwWo	Write-only register
// KHwRw	Read/write register
// KHwBase	Base address within memory map
// _i		Input suffix
// _o		Output suffix
// _b		Input/output suffix

//----------------------------------------------------------------------------
// Memory map: physical addresses
//----------------------------------------------------------------------------
// NB: these are just examples

const TUint KHwBaseCs0			=	0x00000000;
const TUint KHwBaseCs1			=	KHwBaseCs0 + 128*KMega;
const TUint KHwBaseCs2			=	KHwBaseCs1 + 128*KMega;
const TUint KHwBaseCs3			=	KHwBaseCs2 + 128*KMega;

const TUint KHwBaseMemBank0		=	0x20000000;
const TUint KHwBaseMemBank1		=	KHwBaseMemBank0 + 256*KMega;

const TUint KHwBaseRegisters	=	0x80000000;
const TUint KHwBasePeripherals	=	KHwBaseRegisters;					// 8000.0000
const TUint KHwBasePeripheralsA	=	KHwBasePeripherals  + 256*KMega;	// 9000.0000
const TUint KHwBasePeripheralsB	=	KHwBasePeripheralsA + 256*KMega;	// A000.0000
const TUint KHwBasePeripheralsC	=	KHwBasePeripheralsB + 256*KMega;	// B000.0000

// etc...

//----------------------------------------------------------------------------
// Memory map: linear addresses
//----------------------------------------------------------------------------

#if defined (__MEMMODEL_MULTIPLE__)
const TUint KHwLinBaseRegisters = 0xc6000000;			// as mapped by bootstrap
const TUint KHwLinSeparation	= 0x1000;
#elif defined(__MEMMODEL_DIRECT__)
const TUint KHwLinBaseRegisters	= 0x10000000;			// physical address (example only)
const TUint KHwLinSeparation	= 0x01000000;			// physical offsets (example only)
#else
const TUint KHwLinBaseRegisters = 0x63000000;			// as mapped by bootstrap
const TUint KHwLinSeparation	= 0x1000;
#endif

// EXAMPLE ONLY:
const TUint KHwLinBasePeriphGroupA	=	KHwLinBaseRegisters;
const TUint KHwLinBasePeripheral1	=	KHwLinBasePeriphGroupA + 0x00*KHwLinSeparation;
const TUint KHwLinBasePeripheral2	=	KHwLinBasePeriphGroupA + 0x01*KHwLinSeparation;
const TUint KHwLinBasePeripheral3	=	KHwLinBasePeriphGroupA + 0x02*KHwLinSeparation;
const TUint KHwLinBasePeripheral4	=	KHwLinBasePeriphGroupA + 0x03*KHwLinSeparation;

const TUint KHwLinBasePeriphGroupB	=	KHwLinBaseRegisters + 0x20*KHwLinSeparation;

const TUint KHwBaseSerial1	=	KHwLinBasePeriphGroupB + 0x00*KHwLinSeparation;
const TUint KHwBaseSerial2	=	KHwLinBasePeriphGroupB + 0x01*KHwLinSeparation;
const TUint KHwBaseSerial3	=	KHwLinBasePeriphGroupB + 0x02*KHwLinSeparation;

const TUint KHwLinBasePeriphGroupC	=	KHwLinBaseRegisters + 0x30*KHwLinSeparation;

const TUint KHwBaseInterrupts		=	KHwLinBasePeriphGroupC + 0x00*KHwLinSeparation;
const TUint KHwInterruptsMaskRo		=	KHwBaseInterrupts + 0x00;
const TUint KHwInterruptsMaskSet	=	KHwBaseInterrupts + 0x04;
const TUint KHwInterruptsMaskClear	=	KHwBaseInterrupts + 0x08;
const TUint KHoInterruptsIrqPending	=	0x0C;
const TUint KHwInterruptsIrqPending	=	KHwBaseInterrupts + KHoInterruptsIrqPending;
const TUint KHoInterruptsFiqPending	=	0x10;
const TUint KHwInterruptsFiqending	=	KHwBaseInterrupts + KHoInterruptsFiqPending;


// Other device specifc constants, register offsets, bit masks, general-purpose I/O allocations,
// interrupt sources, Memory settings and geometries, etc


class TTemplate
	{
	/**
	 * Accessor functions to hardware resources managed by ASSP (ASIC). Auxiliary and information functions which
	 * are commonly used by Device Drivers or ASSP/Variant code.
	 * Some examples below. These examples assume that the hardware blocks they access (e.g. Interrupt controller
	 * RTC, Clock Control Module, UART, etc) are part of the ASSP.
	 */
public:
	/**
	 * initialisation
	 */
	static void Init3();
	/**
	 * Active waiting loop (not to be used after System Tick timer has been set up - Init3()
	 * @param aDuration A wait time in milliseconds
	 */	
	IMPORT_C static void BootWaitMilliSeconds(TInt aDuration);
	/**
	 * Read and return the Startup reason of the Hardware
	 * @return A TMachineStartupType enumerated value
	 */	
	IMPORT_C static TMachineStartupType StartupReason();
	/**
	 * Read and return the the CPU ID
	 * @return An integer containing the CPU ID string read off the hardware
	 */	
	IMPORT_C static TInt CpuVersionId();
	/**
	 * Read Linear base address of debug UART (as selected in obey file or with eshell debugport command).
	 * @return An integer containing the Linear address of debug Serial Port
	 */	
	IMPORT_C static TUint DebugPortAddr();
	/**
	 * Read CPU clock period in picoseconds
	 * @return An integer containing the CPU clock period in picoseconds
	 */	
	IMPORT_C static TUint ProcessorPeriodInPs();
	/**
	 * Set the Hardware Interrupt masks
	 * @param aValue A new interrupt mask value
	 */	
	IMPORT_C static void SetIntMask(TUint aValue);
	/**
	 * Modify the Hardware Interrupt masks
	 * @param aClearMask A mask with interrupt source bits to clear (disable)
	 * @param aSetMask A mask with interrupt source bits to set (enable)
	 */
	IMPORT_C static void ModifyIntMask(TUint aClearMask,TUint aSetMask);
	/**
	 * Read the state of pending interrupts
	 * @return A mask containing bits set for all pending interrupts
	 */	
	IMPORT_C static TUint IntsPending();
	/**
	 * Read the current time of the RTC
	 * @return A value that is the real time as given by a RTC
	 */	
	IMPORT_C static TUint RtcData();
	/**
	 * Set the RTC time 
	 * @param aValue The real time to set the RTC
	 */	
	IMPORT_C static void SetRtcData(TUint aValue);
	/**
	 * Obtain the physical start address of Video Buffer
	 * @return the physical start address of Video Buffer
	 */	
	IMPORT_C static TPhysAddr VideoRamPhys();
private:
	/**
	 * Assp-specific implementation for Kern::NanoWait function
	 */
	static void NanoWait(TUint32 aInterval);
	};

// TO DO: (optional)
//
// Enumerate here all ASSP interrupt souces. It could be a good idea to enumerate them in a way that facilitates
// operating on the corresponding interrupt controller registers (e.g using their value as a shift count)
//
// EXAMPLE ONLY
enum TTemplateAsspInterruptId
	{
	// ASSP or first-level Interrupt IDs
	EAsspIntIdA=0,
	EAsspIntIdB=1,
	EAsspIntIdC=2,
	EAsspIntIdD=3,
	EAsspIntIdE=4,
	// ...
	EAsspIntIdUsb=11,
	EAsspIntIdDma=12,
	// ...
	EAsspIntIdZ=25
	};

//
// TO DO: (optional)
//
// Define here some commonly used ASSP interrupts
//
// EXAMPLE ONLY
const TInt KIntIdExpansion=EAsspIntIdA;		// this is the ASSP interrupt which connects to second-level (Variant)
											// Interrupt controller: all 2nd level interrupts come through this interrupt
const TInt KIntIdOstMatchMsTimer=EAsspIntIdB;
const TInt KIntIdDigitiser=EAsspIntIdC;
const TInt KIntIdSound=EAsspIntIdD;
const TInt KIntIdTimer1=EAsspIntIdE;	


#endif

