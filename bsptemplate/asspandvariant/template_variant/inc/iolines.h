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
// template\template_variant\inc\iolines.h
// Variant layer header for Template Platform
// 
// WARNING: This file contains some APIs which are internal and are subject
//          to change without notice. Such APIs should therefore not be used
//          outside the Kernel and Hardware Services package.
//

#ifndef __V32TEMPLATEV1_H__
#define __V32TEMPLATEV1_H__
#include <e32cmn.h>
#include <kernel/kpower.h>

//----------------------------------------------------------------------------
// Variant-specific constants: use #define if constant dependencies are not
// declared within this file (this breaks the dependency on other header files)
//----------------------------------------------------------------------------

// Examples of what goes in here include:
//
// - General-purpose I/O allocation such as 
//	 #define KtVariantGpio32KHzClkOut		KHtGpioPort1
//	 #define KtVariantGpioRClkOut			KHtGpioPort0
//
//	 #define KmVariantPinDirectionIn Sleep	0
//
// - Memory constants (type, geometry, wait states, etc) such as:
//	 #define KwVariantRom0Type				TTemplate::ERomTypeBurst4Rom
//	 #define KwVariantRom0Width				TTemplate::ERomWidth32
//	 const TUint KwVariantRom0WaitNs		= 150;
//	 const TUint KwVariantRom0PageNs		= 30;
//	 const TUint KwVariantRom0RecoverNs		= 55;
//
// - Specific Peripherals (Keyboard, LCD, CODECS, Serial Ports) such as 
//	 const TUint KwVariantKeyColLshift		= 7;
//	 #define KwVariantLcdBpp				TTemplate::ELcd8BitsPerPixel
//   const TUint KwVariantLcdMaxColors		= 4096;
//   const TUint KwVariantCodecMaxVolume	= 0;
//
// - Off-chip hardware control blocks (addresses, register make-up)
//
// - Interrupts (second-level Interrupt controller base address, register make-up):
//   (EXAMPLE ONLY:)
const TUint32 KHwVariantPhysBase		=	0x40000000;
const TUint32 KHoVariantRegSpacing		=	0x200;

const TUint32 KHoBaseIntCont			=	0x0B*KHoVariantRegSpacing;

const TUint32 KHoIntContEnable			=	0x00;		// offsets from KHwVariantPhysBase+KHoBaseIntCont
const TUint32 KHoIntContPending			=	0x04;
// other Variant and external blocks Base adrress offsets to KHwVariantPhysBase


// TO DO: (optional)
//
// Enumerate here all Variant (2nd level)  interrupt sources. It could be a good idea to enumerate them in a way that 
// facilitates operating on the corresponding interrupt controller registers (e.g using their value as a shift count)
//
//   (EXAMPLE ONLY:)
enum TTemplateInterruptId
	{
	// the top-level bit is set to distinguish from first level (ASSP) Interrupts
	EXIntIdA=0x80000000,
	EXIntIdB=0x80000001,
	// ...
	EXIntIdZ=0x80000019,

	ENumXInts=0x1A
	};

//
// TO DO: (optional)
//
// Define here some commonly used Variant (2nd level) interrupts
//
//   (EXAMPLE ONLY:)
const TInt KIntIdKeyboard=EXIntIdB;

class Variant
    {
	// below is a selection of functions usually implemented  at this level. This do not constitute a mandatory
	// set and it might not be relevant for your hardware...
public:
	/**
	 * initialisation
	 */
	static void Init3();
	/**
	 * Returns the Linear Base address of the Variant Hardware
	 */
	IMPORT_C static TUint BaseLinAddress();
	/**
	 * When invoked, turns off all power supplies
	 */
	IMPORT_C static void PowerReset();
	/**
	 * When invoked, it marks the Serial port used for outputting debug strings as requiring re-initialisation
	 * As in, for example, the serial port was used by a device driver or the system is coming back from Standby
	 */
	IMPORT_C static void MarkDebugPortOff();
	/**
	 * When invoked, initialises the Serial Port hardware for the serial port used to output Debug strings
	 * Called by Template::DebugInit()
	 */
	IMPORT_C static void UartInit();
	/**
	 * When invoked, read the state of on-board switches
	 * @return A bitmask with the state of on-board switches
	 */
	IMPORT_C static TUint Switches();
	// other functions to access hardware not covered by specific device-drivres, which may be called from drivers
	// or platform-specifc code
	// ...
public:
	static TUint32 iBaseAddress;
	// (optional): May need to have a follower variable to store the value of a read only register initialised at boot time
	// static TUint aFollower;
    };

#endif

