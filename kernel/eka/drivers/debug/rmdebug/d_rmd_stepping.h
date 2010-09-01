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

#ifndef D_RMD_STEPPING_H
#define D_RMD_STEPPING_H

// fwd declaration of DRM_DebugChannel
class DRM_DebugChannel;

// extracted from rm_debug_kerneldriver.h
// Register definitions
#define SP_REGISTER			13
#define LINK_REGISTER		14
#define PC_REGISTER			15
#define STATUS_REGISTER		16

class DRMDStepping : public DBase
{
public:
	// ctor
	DRMDStepping(DRM_DebugChannel* aChannel);

	// dtor
	~DRMDStepping();

	// extracted from rm_debug_kerneldriver.cpp
	TBool IsExecuted(TUint8 aCondition, TUint32 aStatusRegister);
	TBool IsPreviousInstructionMovePCToLR(DThread *aThread);
	void DecodeDataProcessingInstruction(TUint8 aOpcode, TUint32 aOp1, TUint32 aOp2, TUint32 aStatusRegister, TUint32 &aBreakAddress);
	TUint32 PCAfterInstructionExecutes(DThread *aThread, TUint32 aCurrentPC, TUint32 aStatusRegister, TInt aInstSize, TUint32 &aNewRangeEnd, TBool &aChangingModes);
	TUint32 ShiftedRegValue(DThread *aThread, TUint32 aInstruction, TUint32 aCurrentPC, TUint32 aStatusRegister);
	TInt ModifyBreaksForStep(DThread *aThread, TUint32 aRangeStart, TUint32 aRangeEnd,TBool aResumeOnceOutOfRange, TBool aCheckForStubs, const TUint32 aNumSteps);

private:

	// Needed to access private data until re-structuring work is complete.
	friend class DRM_DebugChannel;

	DRM_DebugChannel* iChannel;	// temporary reference back to DRM_DebugChannel to help with refactoring

	// Set of inline functions for decoding instructions. Formerly these were all macros

	// ARM instruction bitmasks
	inline TUint32 arm_opcode(const TUint32 aInst);

	// Generic instruction defines
	inline TUint32 arm_rm(const TUint32 aInst);
	inline TUint32 arm_rs(const TUint32 aInst);
	inline TUint32 arm_rd(const TUint32 aInst);
	inline TUint32 arm_rn(const TUint32 aInst);
	inline TUint32 arm_load(const TUint32 aInst);

	// Data processing instruction defines
	inline TUint32 arm_data_shift(const TUint32 aInst);
	inline TUint32 arm_data_c(const TUint32 aInst);
	inline TUint32 arm_data_imm(const TUint32 aInst);
	inline TUint32 arm_data_rot(const TUint32 aInst);

	// Single date transfer instruction defines
	inline TUint32 arm_single_imm(const TUint32 aInst);
	inline TUint32 arm_single_byte(const TUint32 aInst);
	inline TUint32 arm_single_u(const TUint32 aInst);
	inline TUint32 arm_single_pre(const TUint32 aInst);

	// Block data transfer instruction defines
	inline TUint32 arm_block_reglist(const TUint32 aInst);
	inline TUint32 arm_block_u(const TUint32 aInst);
	inline TUint32 arm_block_pre(const TUint32 aInst);

	// Branch instruction defines
	inline TUint32 arm_b_addr(const TUint32 aInst);
	inline TUint32 arm_instr_b_dest(const TUint32 aInst, TUint32& aAddr);
	inline TUint32 thumb_b_addr(const TUint32 aInst);
	inline TUint32 thumb_instr_b_dest(const TUint32 aInst, TUint32& aAddr);
	inline TUint32 arm_carry_bit(void);
	

	// Thumb instruction bitmasks
	inline TUint16 thumb_opcode(const TUint16 aInst);
	inline TUint16 thumb_inst_7_15(const TUint16 aInst);
	inline TUint16 thumb_inst_8_15(const TUint16 aInst);

	// Thumb2 decode support functions
	inline TUint16 t2opcode16(const TUint16 aInst);

	inline TUint16 t2opcode16special(const TUint16 aInst);

	// Helper functions
	TInt CurrentPC(DThread* aThread, TUint32& aPC);

	TInt CurrentCPSR(DThread* aThread, TUint32& aCPSR);	

	TInt CurrentInstruction(DThread* aThread, TUint32& aInstruction);

	TInt CurrentArchMode(const TUint32 cpsr, Debug::TArchitectureMode& mode);

	TInt RegisterValue(DThread *aThread, const TUint32 aKernelRegisterId, TUint32 &aValue);

	TInt ReadMem32(DThread* aThread, const TUint32 aAddress, TUint32& aValue);

	TInt ReadMem16(DThread* aThread, const TUint32 aAddress, TUint16& aValue);

	TInt ReadMem8(DThread* aThread, const TUint32 aAddress, TUint8& aValue);

	inline TUint32 BitCount(const TUint32 aVal);

	inline TUint32 IsBitSet(const TUint32 aBitset, const TUint8 aBitNum);
};

#include "d_rmd_stepping.inl"

#endif	// D_RMD_STEPPPING_H

// End of file - d-rmd-stepping.h
