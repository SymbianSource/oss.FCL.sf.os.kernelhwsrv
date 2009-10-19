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
//



/**
 @file
 @internalComponent
 @released
*/

#ifndef D_RMD_STEPPING_INL
#define D_RMD_STEPPING_INL

//
// IsBitSet
//
// Returns 1 if the bit 'aNum' is set within aBitset, 0 otherwise
inline TUint32 DRMDStepping::IsBitSet(const TUint32 aBitset, const TUint8 aNum)
	{
	return (aBitset & (1 << aNum) );
	}

// 
// BitCount
//
// Count number of bits in aVal
inline TUint32 DRMDStepping::BitCount(const TUint32 aVal)
	{
	TUint32 num = 0;

	for(TInt i = 0; i < 32; i++)
		{
		if ((1 << i) & aVal)
			{
			num++;
			}
		}
	return num;
	}

//
// Thumb2 opcode decoding
//
// Special data instructions and branch and exchange.
//
// Returns Opcode as defined in ARM ARM DDI0406A, section A6.2.3
inline TUint16 DRMDStepping::t2opcode16special(const TUint16 aInst)
	{
	TUint8 aVal = (aInst & 0x03C0) >> 5;

	return aVal;
	}


// Thumb2 opcode decoding instructions
// 
// Returns Opcode as defined in ARM ARM DDI0406A, section A6.2
// 16-bit Thumb instruction encoding
inline TUint16 DRMDStepping::t2opcode16(const TUint16 aInst)
{
	TUint16 aVal = (aInst & 0xFC00) >> 9;

	return aVal;
}

// ARM opcode decoding functions
inline TUint32 DRMDStepping::arm_opcode(const TUint32 aInst)
{
// #define ARM_OPCODE(x)		(((TUint32)(x) & 0x0E000000) >> 25)

	TUint32 aVal = ((aInst) & 0x0E000000) >> 25;

	return aVal;
}

inline TUint32 DRMDStepping:: arm_rm(const TUint32 aInst)
{
//#define ARM_RM(x)				((TUint32)(x) & 0x0000000F)			// bit 0- 4

	TUint32 aVal = (aInst) & 0x0000000F;

	return aVal;
}

inline TUint32 DRMDStepping:: arm_rs(const TUint32 aInst)
{
//#define ARM_RS(x)				(((TUint32)(x) & 0x00000F00) >> 8)	// bit 8-11

	TUint32 aVal = ((aInst) & 0x00000F00) >> 8;

	return aVal;
}

inline TUint32 DRMDStepping:: arm_rd(const TUint32 aInst)
{
//#define ARM_RD(x)				(((TUint32)(x) & 0x0000F000) >> 12)	// bit 12-15

	TUint32 aVal = ((aInst) & 0x0000F000) >> 12;

	return aVal;
}

inline TUint32 DRMDStepping:: arm_rn(const TUint32 aInst)
{
//#define ARM_RN(x)				(((TUint32)(x) & 0x000F0000) >> 16)	// bit 16-19

	TUint32 aVal = ((aInst) & 0x000F0000) >> 16;

	return aVal;
}

inline TUint32 DRMDStepping::arm_load(const TUint32 aInst)
{
//#define ARM_LOAD(x)				(((TUint32)(x) & 0x00100000) >> 20)	// bit 20

	TUint32 aVal = ((aInst) & 0x00100000) >> 20;

	return aVal;
}

// Data processing instruction defines
inline TUint32 DRMDStepping::arm_data_shift(const TUint32 aInst)
{
//#define ARM_DATA_SHIFT(x)		(((TUint32)(x) & 0x00000060) >> 5) 	// bit 5- 6
	
	TUint32 aVal = ((aInst) & 0x00000060) >> 5;

	return aVal;
}

inline TUint32 DRMDStepping::arm_data_c(const TUint32 aInst)
{
//#define ARM_DATA_C(x)			(((TUint32)(x) & 0x00000F80) >> 7) 	// bit 7-11

	TUint32 aVal = ((aInst) & 0x00000F80) >> 7;

	return aVal;
}

inline TUint32 DRMDStepping::arm_data_imm(const TUint32 aInst)
{
//#define ARM_DATA_IMM(x)			((TUint32)(x) & 0x000000FF)			// bit 0-7

	TUint32 aVal = (aInst) & 0x000000FF;

	return aVal;
}

inline TUint32 DRMDStepping::arm_data_rot(const TUint32 aInst)
{
//#define ARM_DATA_ROT(x)			(((TUint32)(x) & 0x00000F00) >> 8) 	// bit 8-11

	TUint32 aVal = ((aInst) & 0x00000F00) >> 8;

	return aVal;
}

// Single date transfer instruction defines
inline TUint32 DRMDStepping::arm_single_imm(const TUint32 aInst)
{
//#define ARM_SINGLE_IMM(x)		((TUint32)(x) & 0x00000FFF)			// bit 0-11

	TUint32 aVal = (aInst) & 0x00000FFF;

	return aVal;
}

inline TUint32 DRMDStepping::arm_single_byte(const TUint32 aInst)
{
//#define ARM_SINGLE_BYTE(x)		(((TUint32)(x) & 0x00400000) >> 22)	// bit 22

	TUint32 aVal = ((aInst) & 0x00400000) >> 22;

	return aVal;
}

inline TUint32 DRMDStepping::arm_single_u(const TUint32 aInst)
{
//#define ARM_SINGLE_U(x)			(((TUint32)(x) & 0x00800000) >> 23)	// bit 23

	TUint32 aVal = ((aInst) & 0x00800000) >> 23;

	return aVal;
}

inline TUint32 DRMDStepping::arm_single_pre(const TUint32 aInst)
{
//#define ARM_SINGLE_PRE(x)		(((TUint32)(x) & 0x01000000) >> 24)	// bit 24

	TUint32 aVal = ((aInst) & 0x01000000) >> 24;

	return aVal;
}

// Block data transfer instruction defines
inline TUint32 DRMDStepping::arm_block_reglist(const TUint32 aInst)
{
//#define ARM_BLOCK_REGLIST(x)	((TUint32)(x) & 0x0000FFFF)		// bit 0-15

	TUint32 aVal = (aInst) & 0x0000FFFF;

	return aVal;
}

inline TUint32 DRMDStepping::arm_block_u(const TUint32 aInst)
{
//#define ARM_BLOCK_U(x)			(((TUint32)(x) & 0x00800000) >> 23)	// bit 23

	TUint32 aVal = ((aInst) & 0x00800000) >> 23;

	return aVal;
}

inline TUint32 DRMDStepping::arm_block_pre(const TUint32 aInst)
{
//#define ARM_BLOCK_PRE(x)		(((TUint32)(x) & 0x01000000) >> 24)	// bit 24

	TUint32 aVal = ((aInst) & 0x01000000) >> 24;

	return aVal;
}

// Branch instruction defines
inline TUint32 DRMDStepping::arm_b_addr(const TUint32 aInst)
{
//#define ARM_B_ADDR(x)			((x & 0x00800000) ? ((TUint32)(x) & 0x00FFFFFF | 0xFF000000) : (TUint32)(x) & 0x00FFFFFF)

	TUint32 aVal = ((aInst & 0x00800000) ? ((TUint32)(aInst) & 0x00FFFFFF | 0xFF000000) : (TUint32)(aInst) & 0x00FFFFFF);

	return aVal;
}

inline TUint32 DRMDStepping::arm_instr_b_dest(const TUint32 aInst, TUint32& aAddress)
{
//#define ARM_INSTR_B_DEST(x,a)	(ARM_B_ADDR(x) << 2) + ((TUint32)(a) + 8)

	TUint32 aVal = (arm_b_addr(aInst) << 2) + ((TUint32)(aAddress) + 8);

	return aVal;
}

inline TUint32 DRMDStepping::thumb_b_addr(const TUint32 aInst)
{
//#define THUMB_B_ADDR(x) ((x & 0x0400) ? ((((TUint32)(x) & 0x07FF)<<11) | (((TUint32)(x) & 0x07FF0000)>>16) | 0xFFC00000) :\
                                            ((TUint32)(x) & 0x07FF)<<11) | (((TUint32)(x) & 0x07FF0000)>>16)

	TUint32 aVal = ((((TUint32)(aInst) & 0x07FF)<<11) | ((TUint32)(aInst) & 0x07FF0000)>>16);

	return ((aInst & 0x0400) ? (aVal | 0xFFC00000) : aVal);
}

inline TUint32 DRMDStepping::thumb_instr_b_dest(const TUint32 aInst, TUint32& aAddress)
{
//#define THUMB_INSTR_B_DEST(x,a)	(THUMB_B_ADDR(x) << 1) + ((TUint32)(a) + 4)

	TUint32 aVal = (thumb_b_addr(aInst) << 1) + ((TUint32)(aAddress) + 4);

	return aVal;
}

inline TUint32 DRMDStepping::arm_carry_bit(void)
{
//#define ARM_CARRY_BIT			0x20000000	// bit 30

	TUint32 aVal = 0x20000000;

	return aVal;
}

// Thumb instruction bitmasks
inline TUint16 DRMDStepping::thumb_opcode(const TUint16 aInst)
{
//	#define THUMB_OPCODE(x)		(((TUint16)(x) & 0xF800) >> 11)

	TUint16 aVal = ((aInst) & 0xF800) >> 11;

	return aVal;
}

inline TUint16 DRMDStepping::thumb_inst_7_15(const TUint16 aInst)
{
//	#define THUMB_INST_7_15(x)	(((TUint16)(x) & 0xFF80) >> 7)

	TUint16 aVal = ((aInst) & 0xFF80) >> 7;

	return aVal;
}

inline TUint16 DRMDStepping::thumb_inst_8_15(const TUint16 aInst)
{
//	#define THUMB_INST_8_15(x)	(((TUint16)(x) & 0xFF00) >> 8)

	TUint16 aVal = ((aInst) & 0xFF00) >> 8;

	return aVal;
}

#endif	// D_RMD_STEPPPING_INL

// End of file - d-rmd-stepping.inl
