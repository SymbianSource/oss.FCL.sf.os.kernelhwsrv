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
// eka\include\kernel\kernboot.h
// Interface between bootstrap and kernel
// This header file should contain only POD structures with members having
// only the following types:
// TInt8, TUint8, TInt16, TUint16, TInt32, TUint32, TInt64, TUint64
// TInt, TUint, TLinAddr, T* where T is an alphanumeric string
// If TInt64/TUint64 are used, manual padding should be used to guarantee
// that these start on an 8 byte boundary.
// 
// WARNING: This file contains some APIs which are internal and are subject
//          to change without notice. Such APIs should therefore not be used
//          outside the Kernel and Hardware Services package.
//

#ifndef __KERNBOOT_H__
#define __KERNBOOT_H__

/**	
The part of SSuperPageBase class.
Holds the stack base address and the stack size of IRQ, FIQ, Undefined & Abort cpu modes.
@see SSuperPageBase
@publishedPartner
@released
*/
struct TStackInfo
	{	
	void* iIrqStackBase;	/**< Stack base address of IRQ CPU mode*/	
	TInt  iIrqStackSize;	/**< Stack size of IRQ CPU mode*/	
	void* iFiqStackBase;	/**< Stack base address of FIQ CPU mode*/	
	TInt  iFiqStackSize;	/**< Stack size of FIQ CPU mode*/	
	void* iUndStackBase;	/**< Stack base address of Undefined CPU mode*/	
	TInt  iUndStackSize;	/**< Stack size of Undefined CPU mode*/	
	void* iAbtStackBase;	/**< Stack base address of Abort CPU mode*/	
	TInt  iAbtStackSize;	/**< Stack size of Abort CPU mode*/
	};

/**	The part of the super page used by the bootstrap.

Optional fields in this structure may be set to 
KSuperPageAddressFieldUndefined (-1) if not used during boot time.
Current optional fields are:	iSmrData

@publishedPartner
@released
*/
struct SSuperPageBase
	{
	TInt		iRamDriveSize;				/**< @internalComponent */	//   current size of RAM drive
	TInt		iTotalRomSize;											/**< total size of ROM */
	TInt		iTotalRamSize;											/**< total size of RAM, excluding simulated ROM*/
	TLinAddr	iPageDir;					/**< @internalComponent */	//   address of page directory
	TInt		iKernelFault;											/**< flag to indicate reboot due to fault - cleared by bootstrap on hardware reset*/
	TLinAddr	iRamBootData;				/**< @internalComponent */	//   pointer to RAM description
	TLinAddr	iRomBootData;				/**< @internalComponent */	//   pointer to ROM description
	TLinAddr	iBootTable;					/**< @internalComponent */	//   used temporarily by bootstrap
	TLinAddr	iCodeBase;					/**< @internalComponent */	//   used temporarily by bootstrap
	TUint32		iBootFlags;					/**< @internalComponent */	//   used temporarily by bootstrap
	TLinAddr	iBootAlloc;					/**< @internalComponent */	//   used temporarily by bootstrap
	TLinAddr	iMachineData;											/**< pointer to variant specific information*/
	TUint		iActiveVariant;											/**< Active Hardware Variant Descriptor*/
	TLinAddr	iPrimaryEntry;											/**< pointer to TRomEntry for active kernel image*/
	TInt		iInitialHeapSize;										/**< initial size of kernel stack/heap chunk*/
	TInt		iDebugPort;												/**< debug port number*/
	TUint		iCpuId;													/**< CPU identifier*/
	TLinAddr	iUncachedAddress;										/**< Virtual address mapped to uncached memory if required*/
	TLinAddr	iRootDirList;											/**< pointer to ROM root directory list (for extension ROMs)*/
	TInt		iHwStartupReason;										/**< startup reason code (hardware dependent)*/
	TLinAddr	iKernelLimit;				/**< @internalComponent */	//   linear address of top of kernel region (direct model only)
	TLinAddr	iRamBase;					/**< @internalComponent */	//   linear address base of RAM (direct model only)
	TLinAddr	iDCacheFlushArea;										/**< linear address of DCache flush area (direct model only)*/
	TLinAddr	iDCacheFlushWrap;										/**< size of DCache flush area (direct model only)*/
	TLinAddr	iAltDCacheFlushArea;									/**< linear address of alternate DCache flush area (direct model only)*/
	TLinAddr	iAltDCacheFlushWrap;									/**< size of alternate DCache flush area (direct model only)*/
	TStackInfo	iStackInfo;												/**< base address & size of Irq & Fiq CPU mode's stacks*/
	TLinAddr	iArmL2CacheBase;										/**< base address of the external cache controller (e.g. L210, L220,...)*/
	TUint32		iRomHeaderPhys;				/**< @internalComponent */
	TUint32		iAPBootPagePhys;			/**< @internalComponent */
	TUint32		iAPBootPageLin;				/**< @internalComponent */
	TUint32		iAPBootPageDirPhys;			/**< @internalComponent */
	TUint32		iSmrData;                                               /**< address of shadow memory region information block (SMRIB), optional field set by bootstrap PSL */
	TUint32     iPlatformSpecificMappings;
	TInt		iReserved[25];											/**< reserved for the future use*/
	};


/**	Descriptor for a single contiguous block of RAM

@publishedPartner
@released
*/
struct SRamBank
	{
	TUint32	iBase;						/**< physical base address of block*/
	TUint32 iSize;						/**< size of block in bytes*/
	};

/** Maximum number of RAM zones.
@see SRamZone
@publishedPartner
@released
*/
const TUint KMaxRamZones = 64;

/** Flag to indicate that no further fixed pages can be allocated into a
particular RAM zone.
This flag still allows device drivers to allocate pages into a RAM zone
using Epoc::ZoneAllocPhysicalRam().

@publishedPartner
@released
*/
const TUint KRamZoneFlagNoFixed = 0x01;

/** Flag to indicate that no further movable pages can be allocated into a
particular RAM zone.

@publishedPartner
@released
*/
const TUint KRamZoneFlagNoMovable = 0x02;

/** Flag to indicate that no further discardable pages can be allocated into a
particular RAM zone.

@publishedPartner
@released
*/
const TUint KRamZoneFlagNoDiscard = 0x04;

/** RAM zone flag that ensures no further pages are allocated into 
a RAM zone.

@publishedPartner
@released
*/
const TUint KRamZoneFlagNoAlloc = 0x80;

/** RAM zone flag mask that ensures only discardable pages are allocated into 
a RAM zone.
This flag mask still allows device drivers to allocate pages into a RAM zone
using Epoc::ZoneAllocPhysicalRam().

@publishedPartner
@released
*/
const TUint KRamZoneFlagDiscardOnly = KRamZoneFlagNoFixed | KRamZoneFlagNoMovable;

/** RAM zone flag to indicate that a RAM zone should be temporarily blocked 
from further allocations.

@internalComponent
*/
const TUint KRamZoneFlagTmpBlockAlloc = 0x08000000;

/** RAM zone flag to indicate that a RAM zone has been inspected by a general defrag.

@internalComponent
*/
const TUint KRamZoneFlagGenDefrag = 0x20000000;


/** RAM zone flag to indicate that a RAM zone should not be allocated
into by a general defrag operation.

@internalComponent
*/
const TUint KRamZoneFlagGenDefragBlock = 0x10000000;

/** RAM zone flag to indicate that a RAM zone is being claimed and therefore shouldn't
be allocated into.

@internalComponent
*/
const TUint KRamZoneFlagClaiming = 0x40000000;

/** RAM zone flag that is used to detect RAM zone activity.

@internalComponent
*/
const TUint KRamZoneFlagMark = 0x80000000;


/**
A mask of RAM zone flag bits that can not be set by device driver or base port code.

@internalComponent
*/
const TUint KRamZoneFlagInvalid = ~(KRamZoneFlagNoAlloc | KRamZoneFlagNoFixed | KRamZoneFlagNoMovable | KRamZoneFlagNoDiscard);

/** Value that cannot be used for a RAM zone ID.
@publishedPartner
@released
*/
const TUint KRamZoneInvalidId = 0xffffffff;

/** Specifies a single RAM zone and its properties.
@publishedPartner
@released
*/
struct SRamZone
	{
	TUint32 iBase;		/**< Physical base address of the RAM zone*/
	TUint32 iSize;		/**< Size of the RAM zone in bytes, the end of the SRamZone array is indicated by a RAM zone size of zero*/
	TUint iId;			/**< ID number of the RAM zone*/
	TUint iFlags;		/**< Zone flags - specify whether the RAM zone should be reserved for contiguous buffer or h/w use etc*/
	TUint8 iPref;		/**< Preference value for this RAM zone, lower preference RAM zones are used first*/
	TUint8 iReserved[3];/**< @internalComponent*/
	};

/** Defines an SRamZone struct object.
@see SRamZone
@publishedPartner
@released
*/
#define __SRAM_ZONE(aBase, aSize, aId, aFlags, aPref) \
{aBase, aSize, aId, aFlags, aPref, {0,0,0,},}

/** Ends the RAM zone declarations by defining the iSize of a RAM zone as zero.
@see SRamZone
@publishedPartner
@released
*/
#define __SRAM_ZONE_END {0,0,0,0,0,{0,0,0,},}

/** Descriptor for a single contiguous block of ROM

@publishedPartner
@released
*/
class SRomBank
	{
public:
	TUint32	iBase;						/**< physical base address of block*/
	TUint32 iSize;						/**< size of block*/
	TUint8	iWidth;						/**< log2(bus width in bits)*/
	TUint8	iType;						/**< device type*/
	TUint8	iSpeedR;					/**< random access speed*/
	TUint8	iSpeedS;					/**< sequential access speed*/
	TUint32	iLinBase;					/**< linear base address of block*/
	};


/* Enumeration for type of ROM

@publishedPartner
@released
*/
enum TRomType
	{
	ERomTypeXIPFlash = 0x01,
	ERomTypeXIP = 0x02,
	ERomTypeRamAsRom = 0x04
	};


/* CPU ID field

@publishedPartner
@released
*/
const TUint KCpuIdISS = 0x80000000u;	// Instruction Set Simulator


/* Boot flags field

@publishedPartner
@released
*/
const TUint32 KBootFlagRunFromRAM = 0x80000000u;


/** This cosnstant is used in optional fields in the SSuperPageBase struct
(e.g. iSmrData) to indicate the field is not used/set during the boot process.

This maybe because the baseport does not support this field/feature or that 
the feature is not enabled/available as determined by bootstrap PSL in 
InitialiseHardware() at boot time.

@publishedPartner
@prototype
*/
const TUint32 KSuperPageAddressFieldUndefined = 0xffffffff; 


/**	Descriptor for a single contiguous block of RAM initialised with a SMR 
image. Typically these are initialised by the pre-OS loader. 
The structure represents an entry in the Shadow Memory Region Information 
Block (SMRIB). SMRIB is pointed to by the SuperPage iSmrData member.
SMRIB ends with zero sized SSmrBank entry. The iSize is in bytes and must be a 
multiple of 4Kb.

@publishedPartner
@prototype
*/
struct SSmrBank
	{
	TUint32	iBase;				/**< physical base address of block */
	TUint32 iSize;				/**< size of block in bytes, multiple of 4Kb */
	TUint32 iPayloadUID;		/**< SMR Payload UID */
	TUint32 iPayloadFlags;		/**< SMR Payload flags */
	};


/** Structure defining the SMR partition image header format.
The SMR Rom Header has a fixed size of 256 bytes.

@publishedPartner
@prototype
*/
struct SSmrRomHeader
	{
	TUint32	iFingerPrint[2];	/**< "SMR_PART" [0]=0x5F524D53[1]=0x54524150 */
	TUint32 iReserved0;			/**< Reserved for future use, always 0 */
	
	TUint32 iImageVersion;		/**< Version number of image format */
	TUint32 iImageSize;			/**< Size in bytes of the image (hdr+payload)*/
	TUint32 iImageTimestamp;	/**< Image creation time in seconds since midnight Jan 1st 1970 */
	TUint32 iReserved1; 		/**< Reserved for future use, always 0 */
	
	TUint32 iPayloadChecksum;	/**< Calculated checksum at build time */
	TUint32 iPayloadUID;		/**< Alloc'd UID identifying payload content */
	TUint32 iPayloadFlags;		/**< Payload specific flag word */
	TUint32 iReserved[54];		/**< Reserved for future use, always 0 */
	};


#endif
