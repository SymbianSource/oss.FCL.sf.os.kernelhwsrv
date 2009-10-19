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
// e32\include\kernel\arm\arm_types.h
// 
// WARNING: This file contains some APIs which are internal and are subject
//          to change without notice. Such APIs should therefore not be used
//          outside the Kernel and Hardware Services package.
//

/**
 @file
 @publishedPartner
 @released
*/

#ifndef __ARM_TYPES_H__
#define __ARM_TYPES_H__

	
/** Symbolic names for ARM CPU modes.

	@publishedPartner
	@released
*/
enum TArmModes
	{
	EUserMode=0x10,
	EFiqMode=0x11,
	EIrqMode=0x12,
	ESvcMode=0x13,
	EMonitorMode=0x16,
	EAbortMode=0x17,
	EUndefMode=0x1b,
	ESystemMode=0x1f,
	EMaskMode=0x1f
	};


/**	Symbolic names for ARM registers.
	
	The order is guaranteed to be the same as in TArmRegSet.

	@see TArmRegSet
	@publishedPartner
	@released
*/
enum TArmRegisters
	{
	EArmR0  = 0,
	EArmR1  = 1,
	EArmR2  = 2,
	EArmR3  = 3,
	EArmR4  = 4,
	EArmR5  = 5,
	EArmR6  = 6,
	EArmR7  = 7,
	EArmR8  = 8,
	EArmR9  = 9,
	EArmR10 = 10,
	EArmR11 = 11,
	EArmR12 = 12,
	EArmSp  = 13,
	EArmLr  = 14,
	EArmPc  = 15,
	EArmFlags = 16,
	EArmDacr  = 17,
	
	KArmRegisterCount = 18
	};

typedef TUint32 TArmReg;

/**	Structure storing values of all ARM registers.

	The order is guaranteed to be the same as in TArmRegisters so the latter
	can be used as index:

	@code
	TArmRegSet set;
	[...]
	TArmReg* p = static_cast<TArmReg*>(&set);
	p[EArmR13] = ...;
	@endcode
	
	@see TArmRegSet
	@publishedPartner
	@released
 */
class TArmRegSet
	{
public:
	TArmReg iR0;
	TArmReg iR1;
	TArmReg iR2;
	TArmReg iR3;
	TArmReg iR4;
	TArmReg iR5;
	TArmReg iR6;
	TArmReg iR7;
	TArmReg iR8;
	TArmReg iR9;
	TArmReg iR10;
	TArmReg iR11;
	TArmReg iR12;
	TArmReg iR13;
	TArmReg iR14;
	TArmReg	iR15;
	TArmReg iFlags;
	TArmReg iDacr;
	};

/**	Structure storing where a given register is saved on the supervisor stack.
	@see UserContextTables
	@publishedPartner
	@released
 */
class TArmContextElement
	{
public:
	enum TType
		{
		EUndefined,				/**< register is not available */
		EOffsetFromSp,			/**< iValue is offset from stack pointer */
		EOffsetFromStackTop,	/**< iValue is offset from stack top */
		ESpPlusOffset,			/**< value = SP + offset */
		EOffsetFromSpBic3,		/**< iValue is offset from stack pointer, value read is ANDed with ~3 */
		EOffsetFromSpBic3_1,	/**< iValue is offset from stack pointer, value read is ANDed with ~3 and iValue2<<2 is added */
		EOffsetFromSpBic3_2,	/**< iValue is offset from stack pointer, value read is ANDed with ~3,
													iValue2<<2 is added and a second indirection performed */
		};
public:
	TUint8 iType;
	TUint8 iValue;
#ifdef __SMP__
	TUint8 iValue2;
	TUint8 iValue3;
#endif
	};

/**
@internalComponent
*/
struct SBankedRegs
	{
	TArmReg iSCTLR;		// mrc p15, 0, r, c1, c0, 0
	TArmReg iACTLR;		// mrc p15, 0, r, c1, c0, 1
	TArmReg iTTBR0;		// mrc p15, 0, r, c2, c0, 0
	TArmReg iTTBR1;		// mrc p15, 0, r, c2, c0, 1
	TArmReg iTTBCR;		// mrc p15, 0, r, c2, c0, 2
	TArmReg iDACR;		// mrc p15, 0, r, c3, c0, 0
	TArmReg iPRRR;		// mrc p15, 0, r, c10, c2, 0
	TArmReg iNMRR;		// mrc p15, 0, r, c10, c2, 1
	TArmReg iVBAR;		// mrc p15, 0, r, c12, c0, 0
	TArmReg	iFCSEIDR;	// mrc p15, 0, r, c13, c0, 0
	TArmReg	iCTXIDR;	// mrc p15, 0, r, c13, c0, 1
	TArmReg	iRWRWTID;	// mrc p15, 0, r, c13, c0, 2
	TArmReg	iRWROTID;	// mrc p15, 0, r, c13, c0, 3
	TArmReg	iRWNOTID;	// mrc p15, 0, r, c13, c0, 4
	TArmReg	iDFSR;		// mrc p15, 0, r, c5, c0, 0
	TArmReg	iIFSR;		// mrc p15, 0, r, c5, c0, 1
	TArmReg	iADFSR;		// mrc p15, 0, r, c5, c1, 0
	TArmReg	iAIFSR;		// mrc p15, 0, r, c5, c1, 1
	TArmReg	iDFAR;		// mrc p15, 0, r, c6, c0, 0
	TArmReg	iIFAR;		// mrc p15, 0, r, c6, c0, 2
	TArmReg i_Spare[12];
	};

__ASSERT_COMPILE(sizeof(SBankedRegs)==128);

/**
@internalComponent
*/
struct SAuxiliaryRegs
	{
	TArmReg	iTEEHBR;	// mrc p14, 6, r, c1, c0, 0
	TArmReg iCPACR;		// mrc p15, 0, r, c1, c0, 2
	TArmReg iSCR;		// mrc p15, 0, r, c1, c1, 0
	TArmReg iSDER;		// mrc p15, 0, r, c1, c1, 1
	TArmReg iNSACR;		// mrc p15, 0, r, c1, c1, 2
	TArmReg iPMCR;		// **mrc p15, 0, r, c2, c0, 0
	TArmReg iMVBAR;		// mrc p15, 0, r, c12, c0, 1
	TArmReg i_Spare[9];
	};

__ASSERT_COMPILE(sizeof(SAuxiliaryRegs)==64);

/**
@internalComponent
*/
struct SNormalRegs
	{
	TArmReg iR0;		// user registers
	TArmReg iR1;
	TArmReg iR2;
	TArmReg iR3;
	TArmReg iR4;
	TArmReg iR5;
	TArmReg iR6;
	TArmReg iR7;
	TArmReg iR8;
	TArmReg iR9;
	TArmReg iR10;
	TArmReg iR11;
	TArmReg iR12;
	TArmReg iR13;
	TArmReg iR14;
	TArmReg	iR15;
	TArmReg iFlags;
	TArmReg iR13Svc;
	TArmReg iR14Svc;
	TArmReg iSpsrSvc;
	TArmReg iR13Abt;
	TArmReg iR14Abt;
	TArmReg iSpsrAbt;
	TArmReg iR13Und;
	TArmReg iR14Und;
	TArmReg iSpsrUnd;
	TArmReg iR13Irq;
	TArmReg iR14Irq;
	TArmReg iSpsrIrq;
	TArmReg iR8Fiq;
	TArmReg iR9Fiq;
	TArmReg iR10Fiq;
	TArmReg iR11Fiq;
	TArmReg iR12Fiq;
	TArmReg iR13Fiq;
	TArmReg iR14Fiq;
	TArmReg iSpsrFiq;
	TArmReg iR13Mon;
	TArmReg iR14Mon;
	TArmReg iSpsrMon;
	TArmReg i_Spare[8];
	};

__ASSERT_COMPILE(sizeof(SNormalRegs)==192);

/**
@internalComponent
*/
struct SFullArmRegSet
	{
	SNormalRegs		iN;
	SAuxiliaryRegs	iA;
	SBankedRegs		iB[2];
	TUint64			iMore[62];	// store VFP registers here
	TInt			iExcCode;	// 0=prefetch, 1=data, 2=undef, -1=not valid, KMaxTInt=non-exception crash
	TUint32			iCrashArgs[3];
	};

__ASSERT_COMPILE(sizeof(SFullArmRegSet)==1024);


#endif
