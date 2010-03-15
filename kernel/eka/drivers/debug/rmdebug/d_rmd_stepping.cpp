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
// This file contains stepping code refactored from rm_debug_kerneldriver.cpp/rm_debug_kerneldriver.h
//

#include <e32def.h>
#include <e32def_private.h>
#include <e32cmn.h>
#include <e32cmn_private.h>
#include <kernel/kernel.h> 
#include <kernel/kern_priv.h>
#include <nk_trace.h>
#include <arm.h>
#include <rm_debug_api.h>

#include "d_rmd_stepping.h"
#include "d_rmd_breakpoints.h"
#include "rm_debug_kerneldriver.h"	// needed to access DRM_DebugChannel
#include "rm_debug_driver.h"
#include "debug_logging.h"

using namespace Debug;

//
// DRMDStepping::DRMDStepping
//
DRMDStepping::DRMDStepping(DRM_DebugChannel* aChannel)
:
	iChannel(aChannel)
	{
	// to do
	}

//
// DRMDStepping::~DRM_DebugChannel
//
DRMDStepping::~DRMDStepping()
	{
	// to do
	}

//
// DRMDStepping::IsExecuted
//
TBool DRMDStepping::IsExecuted(TUint8 aCondition ,TUint32 aStatusRegister)
	{
	LOG_MSG("DRMDStepping::IsExecuted()");

	TBool N = ((aStatusRegister >> 28) & 0x0000000F) & 0x00000008;
	TBool Z = ((aStatusRegister >> 28) & 0x0000000F) & 0x00000004;
	TBool C = ((aStatusRegister >> 28) & 0x0000000F) & 0x00000002;
	TBool V = ((aStatusRegister >> 28) & 0x0000000F) & 0x00000001;

	switch(aCondition)
		{
		case 0:
			return Z;
		case 1:
			return !Z;
		case 2:
			return C;
		case 3:
			return !C;
		case 4:
			return N;
		case 5:
			return !N;
		case 6:
			return V;
		case 7:
			return !V;
		case 8:
			return (C && !Z);
		case 9:
			return (!C || Z);
		case 10:
			return (N == V);
		case 11:
			return (N != V);
		case 12:
			return ((N == V) && !Z);
		case 13:
			return (Z || (N != V));
		case 14:
		case 15:
			return ETrue;
		}
	
	return EFalse;
	}

//
// DRMDStepping::IsPreviousInstructionMovePCToLR
//
TBool DRMDStepping::IsPreviousInstructionMovePCToLR(DThread *aThread)
	{
	LOG_MSG("DRMDStepping::IsPreviousInstructionMovePCToLR()");

	TInt err = KErrNone;
	
	// there are several types of instructions that modify the PC that aren't
	// designated as linked or non linked branches.  the way gcc generates the
	// code can tell us whether or not these instructions are to be treated as
	// linked branches.  the main cases are bx and any type of mov or load or
	// arithmatic operation that changes the PC.  if these are really just
	// function calls that will return, gcc will generate a mov	lr, pc
	// instruction as the previous instruction.  note that this is just for arm
	// and armi
	
	// get the address of the previous instruction
	TUint32 address = 0;
	err = iChannel->ReadKernelRegisterValue(aThread, PC_REGISTER, address);
	if(err != KErrNone)
		{
		LOG_MSG2("Non-zero error code discarded: %d", err);
		}
	address -= 4;

	TBuf8<4> previousInstruction;
	err = iChannel->DoReadMemory(aThread, address, 4, previousInstruction);
	if (KErrNone != err)
		{
		LOG_MSG2("Error %d reading memory at address %x", address);
		return EFalse;
		}

	const TUint32 movePCToLRIgnoringCondition = 0x01A0E00F;

	TUint32 inst = *(TUint32 *)previousInstruction.Ptr();
	
	if ((inst & 0x0FFFFFFF) == movePCToLRIgnoringCondition)
		{
		return ETrue;
		}
		
	return EFalse;
	}

//
// DRMDStepping::DecodeDataProcessingInstruction
//
void DRMDStepping::DecodeDataProcessingInstruction(TUint8 aOpcode, TUint32 aOp1, TUint32 aOp2, TUint32 aStatusRegister, TUint32 &aBreakAddress)
	{
	LOG_MSG("DRMDStepping::DecodeDataProcessingInstruction()");

	switch(aOpcode)
		{
		case 0:
			{
			// AND
			aBreakAddress = aOp1 & aOp2;
			break;
			}
		case 1:
			{
			// EOR
			aBreakAddress = aOp1 ^ aOp2;
			break;
			}
		case 2:
			{
			// SUB
			aBreakAddress = aOp1 - aOp2;
			break;
			}
		case 3:
			{
			// RSB
			aBreakAddress = aOp2 - aOp1;
			break;
			}
		case 4:
			{
			// ADD
			aBreakAddress = aOp1 + aOp2;
			break;
			}
		case 5:
			{
			// ADC
			aBreakAddress = aOp1 + aOp2 + (aStatusRegister & arm_carry_bit()) ? 1 : 0;
			break;
			}
		case 6:
			{
			// SBC
			aBreakAddress = aOp1 - aOp2 - (aStatusRegister & arm_carry_bit()) ? 0 : 1;
			break;
			}
		case 7:
			{
			// RSC
			aBreakAddress = aOp2 - aOp1 - (aStatusRegister & arm_carry_bit()) ? 0 : 1;
			break;
			}
		case 12:
			{
			// ORR
			aBreakAddress = aOp1 | aOp2;
			break;
			}
		case 13:
			{
			// MOV
			aBreakAddress = aOp2;
			break;
			}
		case 14:
			{
			// BIC
			aBreakAddress = aOp1 & ~aOp2;
			break;
			}
		case 15:
			{
			// MVN
			aBreakAddress = ~aOp2;
			break;
			}
		}
	}

//
// DRMDStepping::CurrentInstruction
//
// Returns the current instruction bitpattern (either 32-bits or 16-bits) if possible
TInt DRMDStepping::CurrentInstruction(DThread* aThread, TUint32& aInstruction)
	{
	LOG_MSG("DRMDStepping::CurrentInstruction");

	// What is the current PC?
	TUint32 pc;	
	ReturnIfError(CurrentPC(aThread,pc));

	// Read it one byte at a time to ensure alignment doesn't matter
	TUint32 inst = 0;
	for(TInt i=3;i>=0;i--)
		{

		TBuf8<1> instruction;
		TInt err = iChannel->DoReadMemory(aThread, (pc+i), 1, instruction); 
		if (KErrNone != err)
			{
			LOG_MSG2("DRMDStepping::CurrentInstruction : Failed to read memory at current PC: return 0x%08x",pc);
			return err;
			}

		inst = (inst << 8) | (*(TUint8 *)instruction.Ptr());
		}

	aInstruction = inst;

	LOG_MSG2("DRMDStepping::CurrentInstruction 0x%08x", aInstruction);

	return KErrNone;
	}

//
// DRMDStepping::CurrentArchMode
//
// Determines architecture mode from the supplied cpsr
TInt DRMDStepping::CurrentArchMode(const TUint32 aCpsr, Debug::TArchitectureMode& aMode)
	{
// Thumb2 work will depend on having a suitable cpu architecture to compile for...
#ifdef ECpuJf
	// State table as per ARM ARM DDI0406A, section A.2.5.1
	if(aCpsr & ECpuJf)
		{
		if (aCpsr & ECpuThumb)
			{
			// ThumbEE (Thumb2)
			aMode = Debug::EThumb2EEMode;
			}
		else
			{
			// Jazelle mode - not supported
			return KErrNotSupported;
			}
		}
	else
#endif
		{
		if (aCpsr & ECpuThumb)
			{
			// Thumb mode
			aMode = Debug::EThumbMode;
			}
		else
			{
			// ARM mode
			aMode = Debug::EArmMode;
			}
		}

	return KErrNone;
	}

//
// DRMDStepping::PCAfterInstructionExecutes
//
// Note, this function pretty much ignores all the arguments except for aThread.
// The arguments continue to exist so that the function has the same prototype as
// the original from Nokia. In the long term this function will be re-factored
// to remove obsolete parameters.
//
TUint32 DRMDStepping::PCAfterInstructionExecutes(DThread *aThread, TUint32 aCurrentPC, TUint32 aStatusRegister, TInt aInstSize, /*TBool aStepInto,*/ TUint32 &aNewRangeEnd, TBool &aChangingModes)
	{
	LOG_MSG("DRMDStepping::PCAfterInstructionExecutes()");

	// by default we will set the breakpoint at the next instruction
	TUint32 breakAddress = aCurrentPC + aInstSize;

	TInt err = KErrNone;

	// determine the architecture
	TUint32 cpuid;
	asm("mrc p15, 0, cpuid, c0, c0, 0 ");
	LOG_MSG2("DRMDStepping::PCAfterInstructionExecutes() - cpuid = 0x%08x\n",cpuid);

	cpuid >>= 8;
	cpuid &= 0xFF;

	// determine the architecture mode for the current instruction
	TArchitectureMode mode = EArmMode;	// Default assumption is ARM 

	// Now we must examine the CPSR to read the T and J bits. See ARM ARM DDI0406A, section B1.3.3
	TUint32 cpsr;

	ReturnIfError(CurrentCPSR(aThread,cpsr));
	LOG_MSG2("DRMDStepping::PCAfterInstructionExecutes() - cpsr = 0x%08x\n",cpsr);

	// Determine the mode
	ReturnIfError(CurrentArchMode(cpsr,mode));

	// Decode instruction based on current CPU mode
	switch(mode)
		{
		case Debug::EArmMode:
			{
			// Obtain the current instruction bit pattern
			TUint32 inst;
			ReturnIfError(CurrentInstruction(aThread,inst));
			
			LOG_MSG2("Current instruction: %x", inst);

			// check the conditions to see if this will actually get executed
			if (IsExecuted(((inst>>28) & 0x0000000F), aStatusRegister)) 
				{
				switch(arm_opcode(inst)) // bits 27-25
					{
					case 0:
						{
						switch((inst & 0x00000010) >> 4) // bit 4
							{
							case 0:
								{
								switch((inst & 0x01800000) >> 23) // bits 24-23
									{
									case 2:
										{
										// move to/from status register.  pc updates not allowed
										// or TST, TEQ, CMP, CMN which don't modify the PC
										break;
										}
									default:
										{
										// Data processing immediate shift
										if (arm_rd(inst) == PC_REGISTER)
											{
											TUint32 rn = aCurrentPC + 8;
											if (arm_rn(inst) != PC_REGISTER) // bits 19-16
												{
												err = iChannel->ReadKernelRegisterValue(aThread, arm_rn(inst), rn);
												if(err != KErrNone)
													{
													LOG_MSG2("Non-zero error code discarded: %d", err);
													}
												}

											TUint32 shifter = ShiftedRegValue(aThread, inst, aCurrentPC, aStatusRegister);

											DecodeDataProcessingInstruction(((inst & 0x01E00000) >> 21), rn, shifter, aStatusRegister, breakAddress);
											}
										break;
										}
									}
								break;
								}
							case 1:
								{
								switch((inst & 0x00000080) >> 7) // bit 7
									{
									case 0:
										{
										switch((inst & 0x01900000) >> 20) // bits 24-23 and bit 20
											{
											case 0x10:
												{
												// from figure 3-3
												switch((inst & 0x000000F0) >> 4) // bits 7-4
													{
													case 1:
														{
														if (((inst & 0x00400000) >> 22) == 0) // bit 22
															{
															// BX
															// this is a strange case.  normally this is used in the epilogue to branch the the link
															// register.  sometimes it is used to call a function, and the LR is stored in the previous
															// instruction.  since what we want to do is different for the two cases when stepping over,
															// we need to read the previous instruction to see what we should do
															err = iChannel->ReadKernelRegisterValue(aThread, (inst & 0x0000000F), breakAddress);
															if(err != KErrNone)
																{
																LOG_MSG2("Non-zero error code discarded: %d", err);
																}

															if ((breakAddress & 0x00000001) == 1)
																{
																aChangingModes = ETrue;
																}

															breakAddress &= 0xFFFFFFFE;
															}
														break;
														}
													case 3:
														{
														// BLX
															{
															err = iChannel->ReadKernelRegisterValue(aThread, (inst & 0x0000000F), breakAddress);
															if(err != KErrNone)
																{
																LOG_MSG2("Non-zero error code discarded: %d", err);
																}

															if ((breakAddress & 0x00000001) == 1)
																{
																aChangingModes = ETrue;
																}
															
															breakAddress &= 0xFFFFFFFE;
															}
														break;
														}
													default:
														{
														// either doesn't modify the PC or it is illegal to
														break;
														}
													}
												break;
												}
											default:
												{
												// Data processing register shift
												if (((inst & 0x01800000) >> 23) == 2) // bits 24-23
													{
													// TST, TEQ, CMP, CMN don't modify the PC
													}
												else if (arm_rd(inst) == PC_REGISTER)
													{
													// destination register is the PC
													TUint32 rn = aCurrentPC + 8;
													if (arm_rn(inst) != PC_REGISTER) // bits 19-16
														{
														err = iChannel->ReadKernelRegisterValue(aThread, arm_rn(inst), rn);
														if(err != KErrNone)
															{
															LOG_MSG2("Non-zero error code discarded: %d", err);
															}
														}
													
													TUint32 shifter = ShiftedRegValue(aThread, inst, aCurrentPC, aStatusRegister);
													
													DecodeDataProcessingInstruction(((inst & 0x01E00000) >> 21), rn, shifter, aStatusRegister, breakAddress);
													}
												break;
												}
											}
										break;
										}
									default:
										{
										// from figure 3-2, updates to the PC illegal
										break;
										}
									}
								break;
								}
							}
						break;
						}
					case 1:
						{
						if (((inst & 0x01800000) >> 23) == 2) // bits 24-23
							{
							// cannot modify the PC
							break;
							}
						else if (arm_rd(inst) == PC_REGISTER)
							{
							// destination register is the PC
							TUint32 rn;
							err = iChannel->ReadKernelRegisterValue(aThread, arm_rn(inst), rn); // bits 19-16
							if(err != KErrNone)
								{
								LOG_MSG2("Non-zero error code discarded: %d", err);
								}
							TUint32 shifter = ((arm_data_imm(inst) >> arm_data_rot(inst)) | (arm_data_imm(inst) << (32 - arm_data_rot(inst)))) & 0xffffffff;

							DecodeDataProcessingInstruction(((inst & 0x01E00000) >> 21), rn, shifter, aStatusRegister, breakAddress);
							}
						break;
						}
					case 2:
						{
						// load/store immediate offset
						if (arm_load(inst)) // bit 20
							{
							// loading a register from memory
							if (arm_rd(inst) == PC_REGISTER)
								{
								// loading the PC register
								TUint32 base;
								err = iChannel->ReadKernelRegisterValue(aThread, arm_rn(inst), base);
								if(err != KErrNone)
									{
									LOG_MSG2("Non-zero error code discarded: %d", err);
									}

								/* Note: At runtime the PC would be 8 further on
								 */
								if (arm_rn(inst) == PC_REGISTER)
									{
									base = aCurrentPC + 8;
									}

								TUint32 offset = 0;

								if (arm_single_pre(inst))
									{
									// Pre-indexing
									offset = arm_single_imm(inst);

									if (arm_single_u(inst))
										{
										base += offset;
										}
									else
										{
										base -= offset;
										}
									}

								TBuf8<4> destination;
								err = iChannel->DoReadMemory(aThread, base, 4, destination);
								
								if (KErrNone == err)
									{
									breakAddress = *(TUint32 *)destination.Ptr();

									if ((breakAddress & 0x00000001) == 1)
										{
										aChangingModes = ETrue;
										}
									breakAddress &= 0xFFFFFFFE;
									}
								else
									{
									LOG_MSG("Error reading memory in decoding step instruction");
									}
								}
							}
						break;
						}
					case 3:
						{
						if (((inst & 0xF0000000) != 0xF0000000) && ((inst & 0x00000010) == 0))
							{
							// load/store register offset
							if (arm_load(inst)) // bit 20
								{
								// loading a register from memory
								if (arm_rd(inst) == PC_REGISTER)
									{
									// loading the PC register
									TUint32 base = 0;
									if(arm_rn(inst) == PC_REGISTER)
										{
										base = aCurrentPC + 8;
										}
									else
										{
										err = iChannel->ReadKernelRegisterValue(aThread, arm_rn(inst), base);
										if(err != KErrNone)
											{
											LOG_MSG2("Non-zero error code discarded: %d", err);
											}
										}

									TUint32 offset = 0;

									if (arm_single_pre(inst))
										{
										offset = ShiftedRegValue(aThread, inst, aCurrentPC, aStatusRegister);

										if (arm_single_u(inst))
											{
											base += offset;
											}
										else
											{
											base -= offset;
											}
										}

									TBuf8<4> destination;
									err = iChannel->DoReadMemory(aThread, base, 4, destination);

									if (KErrNone == err)
										{
										breakAddress = *(TUint32 *)destination.Ptr();

										if ((breakAddress & 0x00000001) == 1)
											{
											aChangingModes = ETrue;
											}
										breakAddress &= 0xFFFFFFFE;
										}
									else
										{
										LOG_MSG("Error reading memory in decoding step instruction");
										}
									}
								}
							}
						break;
						}
					case 4:
						{
						if ((inst & 0xF0000000) != 0xF0000000)
							{
							// load/store multiple
							if (arm_load(inst)) // bit 20
								{
								// loading a register from memory
								if (((inst & 0x00008000) >> 15))
									{
									// loading the PC register
									TInt offset = 0;	
									if (arm_block_u(inst))
										{
										TUint32 reglist = arm_block_reglist(inst);
										offset = iChannel->Bitcount(reglist) * 4 - 4;
										if (arm_block_pre(inst))
											offset += 4;
										}
									else if (arm_block_pre(inst))
										{
										offset = -4;
										}

									TUint32 temp = 0;
									err = iChannel->ReadKernelRegisterValue(aThread, arm_rn(inst), temp);
									if(err != KErrNone)
										{
										LOG_MSG2("Non-zero error code discarded: %d", err);
										}

									temp += offset;

									TBuf8<4> destination;
									err = iChannel->DoReadMemory(aThread, temp, 4, destination);

									if (KErrNone == err)
										{
										breakAddress = *(TUint32 *)destination.Ptr();
										if ((breakAddress & 0x00000001) == 1)
											{
											aChangingModes = ETrue;
											}
										breakAddress &= 0xFFFFFFFE;
										}
									else
										{
										LOG_MSG("Error reading memory in decoding step instruction");
										}
									}
								}
							}
						break;
						}
					case 5:
						{
						if ((inst & 0xF0000000) == 0xF0000000)
							{
							// BLX
							breakAddress = (TUint32)arm_instr_b_dest(inst, aCurrentPC);

							// Unconditionally change into Thumb mode
							aChangingModes = ETrue;
							breakAddress &= 0xFFFFFFFE;
							}
						else
							{
							if ((inst & 0x01000000)) // bit 24
								{
								// BL
									breakAddress = (TUint32)arm_instr_b_dest(inst, aCurrentPC);
								}
							else
								{
								// B
								breakAddress = (TUint32)arm_instr_b_dest(inst, aCurrentPC);
								}
							}
						break;
						} // case 5
					} //switch(arm_opcode(inst)) // bits 27-25
				} // if (IsExecuted(((inst>>28) & 0x0000000F), aStatusRegister)) 
			} // case Debug::EArmMode:
		break;

		case Debug::EThumbMode:
			{
			// Thumb Mode
			//
			// Notes: This now includes the extra code
			// required to decode V6T2 instructions
			
			LOG_MSG("DRMDStepping::PCAfterInstructionExecutes: Thumb Instruction");

			TUint16 inst;

			// Obtain the current instruction bit pattern
			TUint32 inst32;
			ReturnIfError(CurrentInstruction(aThread,inst32));

			inst = static_cast<TUint16>(inst32 & 0xFFFF);

			LOG_MSG2("Current Thumb instruction: 0x%x", inst);

			// v6T2 instructions

			// Note: v6T2 decoding is only enabled for DEBUG builds or if using an
			// an ARM_V6T2 supporting build system. At the time of writing, no
			// ARM_V6T2 supporting build system exists, so the stepping code cannot
			// be said to be known to work. Hence it is not run for release builds

			TBool use_v6t2_decodings = EFalse;

#if defined(DEBUG) || defined(__ARMV6T2__)
			use_v6t2_decodings = ETrue;
#endif
			// coverity[dead_error_line]
			if (use_v6t2_decodings)
				{
				// 16-bit encodings

				// A6.2.5 Misc 16-bit instructions
				// DONE Compare and branch on zero (page A8-66)
				// If then hints

				// ARM ARM DDI0406A - section A8.6.27 CBNZ, CBZ
				//
				// Compare and branch on Nonzero and Compare and Branch on Zero.
				if ((inst & 0xF500) == 0xB100)
					{
					LOG_MSG("ARM ARM DDI0406A - section A8.6.27 CBNZ, CBZ");

					// Decoding as per ARM ARM description
					TUint32 op = (inst & 0x0800) >> 11;
					TUint32 i = (inst & 0x0200) >> 9;
					TUint32 imm5 = (inst & 0x00F8) >> 3;
					TUint32 Rn = inst & 0x0007;

					TUint32 imm32 = (i << 6) | (imm5 << 1);

					// Obtain value for register Rn
					TUint32 RnVal = 0;
					ReturnIfError(RegisterValue(aThread,Rn,RnVal));

					if (op)
						{
						// nonzero
						if (RnVal != 0x0)
							{
							// Branch
							breakAddress = aCurrentPC + imm32;
							}
						}
					else
						{
						// zero
						if (RnVal == 0x0)
							{
							// Branch
							breakAddress = aCurrentPC + imm32;
							}
						}
				}

				// ARM ARM DDI0406A - section A8.6.50 IT
				//
				// If Then instruction
				if ((inst & 0xFF00) == 0xBF00)
					{
					LOG_MSG("ARM ARM DDI0406A - section A8.6.50 IT");

					// Decoding as per ARM ARM description
					TUint32 firstcond = inst & 0x00F0 >> 4;
					TUint32 mask = inst & 0x000F;

					if (firstcond == 0xF)
						{
						// unpredictable
						LOG_MSG("ARM ARM DDI0406A - section A8.6.50 IT - Unpredictable");
						break;
						}

					if ((firstcond == 0xE) && (BitCount(mask) != 1))
						{
						// unpredictable
						LOG_MSG("ARM ARM DDI0406A - section A8.6.50 IT - Unpredictable");
						break;
						}

					// should check if 'in-it-block'
					LOG_MSG("Cannot step IT instructions.");

					// all the conds are as per Table A8-1 (i.e. the usual 16 cases)
					// no idea how to decode the it block 'after-the-fact'
					// so probably need to treat instructions in the it block
					// as 'may' be executed. So breakpoints at both possible locations
					// depending on whether the instruction is executed or not.

					// also, how do we know if we have hit a breakpoint whilst 'in' an it block?
					// can we check the status registers to find out?
					//
					// see arm arm page 390.
					//
					// seems to depend on the itstate field. this also says what the condition code
					// actually is, and how many instructions are left in the itblock.
					// perhaps we can just totally ignore this state, and always do the two-instruction
					// breakpoint thing? Not if there is any possibility that the address target
					// would be invalid for the non-taken branch address...
					}


				// 32-bit encodings.
				//

				// Load word A6-23
				// Data processing instructions a6-28
				// 

				// ARM ARM DDI0406A - section A8.6.26
				if (inst32 & 0xFFF0FFFF == 0xE3C08F00)
					{
					LOG_MSG("ARM ARM DDI0406A - section A8.6.26 - BXJ is not supported");

					// Decoding as per ARM ARM description
					// TUint32 Rm = inst32 & 0x000F0000;	// not needed yet
					}

				// return from exception... SUBS PC,LR. page b6-25
				//
				// ARM ARM DDi046A - section B6.1.13 - SUBS PC,LR
				//
				// Encoding T1
				if (inst32 & 0xFFFFFF00 == 0xF3DE8F00)
					{
					LOG_MSG("ARM ARM DDI0406A - section B6.1.13 - SUBS PC,LR Encoding T1");

					// Decoding as per ARM ARM description
					TUint32 imm8 = inst32 & 0x000000FF;
					TUint32 imm32 = imm8;

					// TUint32 register_form = EFalse;	// not needed for this decoding
					// TUint32 opcode = 0x2;	// SUB	// not needed for this decoding
					TUint32 n = 14;

					// Obtain LR
					TUint32 lrVal;
					ReturnIfError(RegisterValue(aThread,n,lrVal));

					TUint32 operand2 = imm32;	// always for Encoding T1
					
					TUint32 result = lrVal - operand2;
					
					breakAddress = result;
					}

				// ARM ARM DDI0406A - section A8.6.16 - B
				//
				// Branch Encoding T3
				if (inst32 & 0xF800D000 == 0xF0008000)
					{
					LOG_MSG("ARM ARM DDI0406A - section A8.6.16 - B Encoding T3");

					// Decoding as per ARM ARM description
					TUint32 S = inst32 & 0x04000000 >> 26;
					// TUint32 cond = inst32 & 0x03C00000 >> 22;	// not needed for this decoding
					TUint32 imm6 = inst32 & 0x003F0000 >> 16;
					TUint32 J1 = inst32 & 0x00002000 >> 13;
					TUint32 J2 = inst32 & 0x00000800 >> 11;
					TUint32 imm11 = inst32 & 0x000007FF;

					TUint32 imm32 = S ? 0xFFFFFFFF : 0 ;
					imm32 = (imm32 << 1) | J2;
					imm32 = (imm32 << 1) | J1;
					imm32 = (imm32 << 6) | imm6;
					imm32 = (imm32 << 11) | imm11;
					imm32 = (imm32 << 1) | 0;

					breakAddress = aCurrentPC + imm32;
					}

				// ARM ARM DDI0406A - section A8.6.16 - B
				//
				// Branch Encoding T4
				if (inst32 & 0xF800D000 == 0xF0009000)
					{
					LOG_MSG("ARM ARM DDI0406A - section A8.6.16 - B");

					// Decoding as per ARM ARM description
					TUint32 S = inst32 & 0x04000000 >> 26;
					TUint32 imm10 = inst32 & 0x03FF0000 >> 16;
					TUint32 J1 = inst32 & 0x00002000 >> 12;
					TUint32 J2 = inst32 & 0x00000800 >> 11;
					TUint32 imm11 = inst32 & 0x000003FF;

					TUint32 I1 = !(J1 ^ S);
					TUint32 I2 = !(J2 ^ S);

					TUint32 imm32 = S ? 0xFFFFFFFF : 0;
					imm32 = (imm32 << 1) | S;
					imm32 = (imm32 << 1) | I1;
					imm32 = (imm32 << 1) | I2;
					imm32 = (imm32 << 10) | imm10;
					imm32 = (imm32 << 11) | imm11;
					imm32 = (imm32 << 1) | 0;

					breakAddress = aCurrentPC + imm32;
					}


				// ARM ARM DDI0406A - section A8.6.225 - TBB, TBH
				//
				// Table Branch Byte, Table Branch Halfword
				if (inst32 & 0xFFF0FFE0 == 0xE8D0F000)
						{
					LOG_MSG("ARM ARM DDI0406A - section A8.6.225 TBB,TBH Encoding T1");

					// Decoding as per ARM ARM description
					TUint32 Rn = inst32 & 0x000F0000 >> 16;
					TUint32 H = inst32 & 0x00000010 >> 4;
					TUint32 Rm = inst32 & 0x0000000F;

					// Unpredictable?
					if (Rm == 13 || Rm == 15)
						{
						LOG_MSG("ARM ARM DDI0406A - section A8.6.225 TBB,TBH Encoding T1 - Unpredictable");
						break;
						}

					TUint32 halfwords;
					TUint32 address;
					ReturnIfError(RegisterValue(aThread,Rn,address));

					TUint32 offset;
					ReturnIfError(RegisterValue(aThread,Rm,offset));

					if (H)
						{
						address += offset << 1;
						}
					else
						{
						address += offset;
						}

					ReturnIfError(ReadMem32(aThread,address,halfwords));

					breakAddress = aCurrentPC + 2*halfwords;
					break;
					}

				// ARM ARM DDI0406A - section A8.6.55 - LDMDB, LDMEA
				//
				// LDMDB Encoding T1
				if (inst32 & 0xFFD02000 == 0xE9100000)
					{
					LOG_MSG("ARM ARM DDI0406 - section A8.6.55 LDMDB Encoding T1");

					// Decoding as per ARM ARM description
					// TUint32 W = inst32 & 0x00200000 >> 21;	// Not needed for this encoding
					TUint32 Rn = inst32 & 0x000F0000 >> 16;
					TUint32 P = inst32 & 0x00008000 >> 15;
					TUint32 M = inst32 & 0x00004000 >> 14;
					TUint32 registers = inst32 & 0x00001FFF;

					//TBool wback = (W == 1);	// not needed for this encoding

					// Unpredictable?
					if (Rn == 15 || BitCount(registers) < 2 || ((P == 1) && (M==1)))
						{
						LOG_MSG("ARM ARM DDI0406 - section A8.6.55 LDMDB Encoding T1 - Unpredictable");
						break;
						}

					TUint32 address;
					ReturnIfError(RegisterValue(aThread,Rn,address));

					address -= 4*BitCount(registers);

					for(TInt i=0; i<15; i++)
						{
						if (IsBitSet(registers,i))
							{
							address +=4;
							}
						}

					if (IsBitSet(registers,15))
						{
						TUint32 RnVal = 0;
						ReturnIfError(ReadMem32(aThread,address,RnVal));

						breakAddress = RnVal;
						}
					break;
					}

				// ARM ARM DDI0406A - section A8.6.121 POP
				//
				// POP.W Encoding T2
				if (inst32 & 0xFFFF2000 == 0xE8BD0000)
					{
					LOG_MSG("ARM ARM DDI0406A - section A8.6.121 POP Encoding T2");

					// Decoding as per ARM ARM description
					TUint32 registers = inst32 & 0x00001FFF;
					TUint32 P = inst32 & 0x00008000;
					TUint32 M = inst32 & 0x00004000;

					// Unpredictable?
					if ( (BitCount(registers)<2) || ((P == 1)&&(M == 1)) )
						{
						LOG_MSG("ARM ARM DDI0406A - section A8.6.121 POP Encoding T2 - Unpredictable");
						break;
						}

					TUint32 address;
					ReturnIfError(RegisterValue(aThread,13,address));
					
					for(TInt i=0; i< 15; i++)
						{
						if (IsBitSet(registers,i))
							{
							address += 4;
							}
						}

					// Is the PC written?
					if (IsBitSet(registers,15))
						{
						// Yes
						ReturnIfError(ReadMem32(aThread,address,breakAddress));
						}
					}

				// POP Encoding T3
				if (inst32 & 0xFFFF0FFFF == 0xF85D0B04)
					{
					LOG_MSG("ARM ARM DDI0406A - section A8.6.121 POP Encoding T3");

					// Decoding as per ARM ARM description
					TUint32 Rt = inst32 & 0x0000F000 >> 12;
					TUint32 registers = 1 << Rt;

					// Unpredictable?
					if (Rt == 13 || Rt == 15)
						{
						LOG_MSG("ARM ARM DDI0406A - section A8.6.121 POP Encoding T3 - Unpredictable");
						break;
						}
					
					TUint32 address;
					ReturnIfError(RegisterValue(aThread,13,address));
					
					for(TInt i=0; i< 15; i++)
						{
						if (IsBitSet(registers,i))
							{
							address += 4;
							}
						}

					// Is the PC written?
					if (IsBitSet(registers,15))
						{
						// Yes
						ReturnIfError(ReadMem32(aThread,address,breakAddress));
						}

					break;
					}

				// ARM ARM DDI0406A - section A8.6.53 LDM
				//
				// Load Multiple Encoding T2 
				if ((inst32 & 0xFFD02000) == 0xE8900000)
					{
					LOG_MSG("ARM ARM DDI0406A - section A8.6.53 LDM Encoding T2");

					// Decoding as per ARM ARM description
					TUint32 W = inst32 & 0x0020000 >> 21;
					TUint32 Rn = inst32 & 0x000F0000 >> 16;
					TUint32 P = inst32 & 0x00008000 >> 15;
					TUint32 M = inst32 & 0x00004000 >> 14;
					TUint32 registers = inst32 & 0x0000FFFF;
					TUint32 register_list = inst32 & 0x00001FFF;
				
					// POP?
					if ( (W == 1) && (Rn == 13) )
						{
						// POP instruction
						LOG_MSG("ARM ARM DDI0406A - section A8.6.53 LDM Encoding T2 - POP");
						}

					// Unpredictable?
					if (Rn == 15 || BitCount(register_list) < 2 || ((P == 1) && (M == 1)) )
						{
						LOG_MSG("ARM ARM DDI0406A - section A8.6.53 LDM Encoding T2 - Unpredictable");
						break;
						}
					
					TUint32 RnVal;
					ReturnIfError(RegisterValue(aThread,Rn,RnVal));

					TUint32 address = RnVal;

					// Calculate offset of address
					for(TInt i = 0; i < 15; i++)
						{
						if (IsBitSet(registers,i))
						{
							address += 4;
						}
						}

					// Does it load the PC?
					if (IsBitSet(registers,15))
						{
						// Obtain the value loaded into the PC
						ReturnIfError(ReadMem32(aThread,address,breakAddress));
						}
					break;

					}

				// ARM ARM DDI0406A - section B6.1.8 RFE
				//
				// Return From Exception Encoding T1 RFEDB
				if ((inst32 & 0xFFD0FFFF) == 0xE810C000)
					{
					LOG_MSG("ARM ARM DDI0406A - section B6.1.8 RFE Encoding T1");

					// Decoding as per ARM ARM description
					// TUint32 W = (inst32 & 0x00200000) >> 21;	// not needed for this encoding
					TUint32 Rn = (inst32 & 0x000F0000) >> 16;
					
					// TBool wback = (W == 1);	// not needed for this encoding
					TBool increment = EFalse;
					TBool wordhigher = EFalse;

					// Do calculation
					if (Rn == 15)
						{
						// Unpredictable 
						LOG_MSG("ARM ARM DDI0406A - section B6.1.8 RFE Encoding T1 - Unpredictable");
						break;
						}

					TUint32 RnVal = 0;
					ReturnIfError(RegisterValue(aThread,Rn,RnVal));

					TUint32 address = 0;
					ReturnIfError(ReadMem32(aThread,RnVal,address));

					if (increment)
						{
						address -= 8;
						}

					if (wordhigher)
						{
						address += 4;
						}

					breakAddress = address;
					break;
					}

				// Return From Exception Encoding T2 RFEIA
				if ((inst32 & 0xFFD0FFFF) == 0xE990C000)
					{
					LOG_MSG("ARM ARM DDI0406A - section B6.1.8 RFE Encoding T2");

					// Decoding as per ARM ARM description
					// TUint32 W = (inst32 & 0x00200000) >> 21;	// not needed for this encoding
					TUint32 Rn = (inst32 & 0x000F0000) >> 16;
					
					// TBool wback = (W == 1);	// not needed for this encoding
					TBool increment = ETrue;
					TBool wordhigher = EFalse;

					// Do calculation
					if (Rn == 15)
						{
						// Unpredictable 
						LOG_MSG("ARM ARM DDI0406A - section B6.1.8 RFE Encoding T2 - Unpredictable");
						break;
						}

					TUint32 RnVal = 0;
					ReturnIfError(RegisterValue(aThread,Rn,RnVal));

					TUint32 address = 0;
					ReturnIfError(ReadMem32(aThread,RnVal,address));

					if (increment)
						{
						address -= 8;
						}

					if (wordhigher)
						{
						address += 4;
						}

					breakAddress = RnVal;
					break;
					}

				// Return From Exception Encoding A1 RFE<amode>
				if ((inst32 & 0xFE50FFFF) == 0xF8100A00)
					{
					LOG_MSG("ARM ARM DDI0406A - section B6.1.8 RFE Encoding A1");

					// Decoding as per ARM ARM description
					TUint32 P = (inst32 & 0x01000000) >> 24;
					TUint32 U = (inst32 & 0x00800000) >> 23;
					// TUint32 W = (inst32 & 0x00200000) >> 21; // not needed for this encoding
					TUint32 Rn = (inst32 & 0x000F0000) >> 16;	
					
					// TBool wback = (W == 1);	// not needed for this encoding
					TBool increment = (U == 1);
					TBool wordhigher = (P == U);

					// Do calculation
					if (Rn == 15)
						{
						// Unpredictable 
						LOG_MSG("ARM ARM DDI0406A - section B6.1.8 RFE Encoding A1 - Unpredictable");
						break;
						}

					TUint32 RnVal = 0;
					ReturnIfError(RegisterValue(aThread,Rn,RnVal));

					TUint32 address = 0;
					ReturnIfError(ReadMem32(aThread,RnVal,address));

					if (increment)
						{
						address -= 8;
						}

					if (wordhigher)
						{
						address += 4;
						}

					breakAddress = address;
					break;
					}
				}

			// v4T/v5T/v6T instructions
			switch(thumb_opcode(inst))
				{
				case 0x08:
					{
					// Data-processing. See ARM ARM DDI0406A, section A6-8, A6.2.2.

					if ((thumb_inst_7_15(inst) == 0x08F))
						{
						// BLX(2)
						err = iChannel->ReadKernelRegisterValue(aThread, ((inst & 0x0078) >> 3), breakAddress);
						if(err != KErrNone)
							{
							LOG_MSG2("Non-zero error code discarded: %d", err);
							}

						if ((breakAddress & 0x00000001) == 0)
							{
							aChangingModes = ETrue;
							}

						breakAddress &= 0xFFFFFFFE;

						// Report how we decoded this instruction
						LOG_MSG("DRMDStepping::PCAfterInstructionExecutes: Decoded as BLX (2)");
						}
					else if (thumb_inst_7_15(inst) == 0x08E)
						{
						// BX
						err = iChannel->ReadKernelRegisterValue(aThread, ((inst & 0x0078) >> 3), breakAddress);
						if(err != KErrNone)
							{
							LOG_MSG2("Non-zero error code discarded: %d", err);
							}

						if ((breakAddress & 0x00000001) == 0)
							{
							aChangingModes = ETrue;
							}
						
						breakAddress &= 0xFFFFFFFE;

						// Report how we decoded this instruction
						LOG_MSG("DRMDStepping::PCAfterInstructionExecutes: Decoded as BX");
						}
					else if ((thumb_inst_8_15(inst) == 0x46) && ((inst & 0x87) == 0x87))
						{
						// MOV with PC as the destination
						err = iChannel->ReadKernelRegisterValue(aThread, ((inst & 0x0078) >> 3), breakAddress);
						if(err != KErrNone)
							{
							LOG_MSG2("Non-zero error code discarded: %d", err);
							}

						// Report how we decoded this instruction
						LOG_MSG("DRMDStepping::PCAfterInstructionExecutes: Decoded as MOV with PC as the destination");
						}
					else if ((thumb_inst_8_15(inst) == 0x44) && ((inst & 0x87) == 0x87))
						{
						// ADD with PC as the destination
						err = iChannel->ReadKernelRegisterValue(aThread, ((inst & 0x0078) >> 3), breakAddress);
						if(err != KErrNone)
							{
							LOG_MSG2("Non-zero error code discarded: %d", err);
							}
						breakAddress += aCurrentPC + 4; // +4 because we need to use the PC+4 according to ARM ARM DDI0406A, section A6.1.2.

						// Report how we decoded this instruction
						LOG_MSG("DRMDStepping::PCAfterInstructionExecutes: Decoded as ADD with PC as the destination");
						}
					break;
					}
				case 0x13:
					{
					// Load/Store single data item. See ARM ARM DDI0406A, section A6-10

					//This instruction doesn't modify the PC.

					//if (thumb_inst_8_15(inst) == 0x9F)
					//{
						// LDR(4) with the PC as the destination
					//	breakAddress = ReadRegister(aThread, SP_REGISTER) + (4 * (inst & 0x00FF));
					//}

					// Report how we decoded this instruction
					LOG_MSG("DRMDStepping::PCAfterInstructionExecutes: Decoded as This instruction doesn't modify the PC.");
					break;
					}
				case 0x17:
					{
					// Misc 16-bit instruction. See ARM ARM DDI0406A, section A6-11

					if (thumb_inst_8_15(inst) == 0xBD)
						{
						// POP with the PC in the list
						TUint32 regList = (inst & 0x00FF);
						TInt offset = 0;
						err = iChannel->ReadKernelRegisterValue(aThread,  SP_REGISTER, (T4ByteRegisterValue&)offset);
						if(err != KErrNone)
							{
							LOG_MSG2("Non-zero error code discarded: %d", err);
							}
						offset += (iChannel->Bitcount(regList) * 4);

						TBuf8<4> destination;
						err = iChannel->DoReadMemory(aThread, offset, 4, destination);
						
						if (KErrNone == err)
							{
							breakAddress = *(TUint32 *)destination.Ptr();

							if ((breakAddress & 0x00000001) == 0)
								{
								aChangingModes = ETrue;
								}

							breakAddress &= 0xFFFFFFFE;
							}
						else
							{
							LOG_MSG("Error reading memory in decoding step instruction");
							}

						// Report how we decoded this instruction
						LOG_MSG("DRMDStepping::PCAfterInstructionExecutes: Decoded as POP with the PC in the list");
						}
					break;
					}
				case 0x1A:
				case 0x1B:
					{
					// Conditional branch, and supervisor call. See ARM ARM DDI0406A, section A6-13

					if (thumb_inst_8_15(inst) < 0xDE)
						{
						// B(1) conditional branch
						if (IsExecuted(((inst & 0x0F00) >> 8), aStatusRegister))
							{
							TUint32 offset = ((inst & 0x000000FF) << 1);
							if (offset & 0x00000100)
								{
								offset |= 0xFFFFFF00;
								}
							
							breakAddress = aCurrentPC + 4 + offset;

							// Report how we decoded this instruction
							LOG_MSG("DRMDStepping::PCAfterInstructionExecutes: Decoded as B(1) conditional branch");
							}
						}
					break;
					}
				case 0x1C:
					{
					// Unconditional branch, See ARM ARM DDI0406A, section A8-44.

					// B(2) unconditional branch
					TUint32 offset = (inst & 0x000007FF) << 1;
					if (offset & 0x00000800)
						{
						offset |= 0xFFFFF800;
						}
					
					breakAddress = aCurrentPC + 4 + offset;

					// Report how we decoded this instruction
					LOG_MSG("DRMDStepping::PCAfterInstructionExecutes: Decoded as B(2) unconditional branch");

					break;
					}
				case 0x1D:
					{
					if (!(inst & 0x0001))
						{
						// BLX(1)
						err = iChannel->ReadKernelRegisterValue(aThread, LINK_REGISTER, breakAddress);
						if(err != KErrNone)
							{
							LOG_MSG2("Non-zero error code discarded: %d", err);
							}
						breakAddress +=  ((inst & 0x07FF) << 1);
						if ((breakAddress & 0x00000001) == 0)
							{
							aChangingModes = ETrue;
							}

						breakAddress &= 0xFFFFFFFC;

						// Report how we decoded this instruction
						LOG_MSG("DRMDStepping::PCAfterInstructionExecutes: Decoded as BLX(1)");

						}
					break;
					}
				case 0x1E:
					{
					// Check for ARMv7 CPU
					if(cpuid == 0xC0)
						{
						// BL/BLX 32-bit instruction
						aNewRangeEnd += 4;

						breakAddress = (TUint32)thumb_instr_b_dest(inst32, aCurrentPC);

						if((inst32 >> 27) == 0x1D)
							{
							// BLX(1)
							if ((breakAddress & 0x00000001) == 0)
								{
								aChangingModes = ETrue;
								}

							breakAddress &= 0xFFFFFFFC;

							// Report how we decoded this instruction
							LOG_MSG("DRMDStepping::PCAfterInstructionExecutes: Decoded as 32-bit BLX(1)");
							}
						else
							{
							// Report how we decoded this instruction
							LOG_MSG("DRMDStepping::PCAfterInstructionExecutes: 32-bit BL instruction");
							}
						LOG_MSG2(" 32-bit BL/BLX instruction: breakAddress = 0x%X", breakAddress);
						} // if(cpuid == 0xC0)
					else
						{
						// BL/BLX prefix - destination is encoded in this and the next instruction
						aNewRangeEnd += 2;

						// Report how we decoded this instruction
						LOG_MSG("DRMDStepping::PCAfterInstructionExecutes: BL/BLX prefix - destination is encoded in this and the next instruction");
						}

					break;
					}
				case 0x1F:
					{
					// BL
					err = iChannel->ReadKernelRegisterValue(aThread, LINK_REGISTER, breakAddress);
					if(err != KErrNone)
						{
						LOG_MSG2("Non-zero error code discarded: %d", err);
						}
					breakAddress += ((inst & 0x07FF) << 1);

					// Report how we decoded this instruction
					LOG_MSG("DRMDStepping::PCAfterInstructionExecutes: Decoded as BL");
					break;
					}
				default:
					{
					// Don't know any better at this point!
					LOG_MSG("DRMDStepping::PCAfterInstructionExecutes:- default to next instruction");
					}
					break;
				} // switch(thumb_opcode(inst))
			} // case Debug::EThumbMode:
		break;

		case Debug::EThumb2EEMode:
			{
			// Not yet supported
			LOG_MSG("DRMDStepping::PCAfterInstructionExecutes - Debug::EThumb2Mode is not supported");

			}
			break;

		default:
			LOG_MSG("DRMDStepping::PCAfterInstructionExecutes - Cannot determine CPU mode architecture");
		} // switch(mode)

	LOG_MSG2("DRMDStepping::PCAfterInstructionExecutes : return 0x%08x",breakAddress);
	return breakAddress;
	}

// Obtain a 32-bit memory value with minimum fuss
TInt DRMDStepping::ReadMem32(DThread* aThread, const TUint32 aAddress, TUint32& aValue)
	{
	TBuf8<4> valBuf;
	TInt err = iChannel->DoReadMemory(aThread, aAddress, 4, valBuf);
	if (err != KErrNone)
		{
		LOG_MSG2("DRMDStepping::ReadMem32 failed to read memory at 0x%08x", aAddress);
		return err;
		}

	aValue = *(TUint32 *)valBuf.Ptr();

	return KErrNone;
	}

// Obtain a 16-bit memory value with minimum fuss
TInt DRMDStepping::ReadMem16(DThread* aThread, const TUint32 aAddress, TUint16& aValue)
	{
	TBuf8<2> valBuf;
	TInt err = iChannel->DoReadMemory(aThread, aAddress, 2, valBuf);
	if (err != KErrNone)
		{
		LOG_MSG2("DRMDStepping::ReadMem16 failed to read memory at 0x%08x", aAddress);
		return err;
		}

	aValue = *(TUint16 *)valBuf.Ptr();

	return KErrNone;
	}

// Obtain a 16-bit memory value with minimum fuss
TInt DRMDStepping::ReadMem8(DThread* aThread, const TUint32 aAddress, TUint8& aValue)
	{
	TBuf8<1> valBuf;
	TInt err = iChannel->DoReadMemory(aThread, aAddress, 1, valBuf);
	if (err != KErrNone)
		{
		LOG_MSG2("DRMDStepping::ReadMem8 failed to read memory at 0x%08x", aAddress);
		return err;
		}

	aValue = *(TUint8 *)valBuf.Ptr();

	return KErrNone;
	}

// Obtain a core register value with minimum fuss
TInt DRMDStepping::RegisterValue(DThread *aThread, const TUint32 aKernelRegisterId, TUint32 &aValue)
	{
	TInt err = iChannel->ReadKernelRegisterValue(aThread, aKernelRegisterId, aValue);
	if(err != KErrNone)
		{
		LOG_MSG3("DRMDStepping::RegisterValue failed to read register %d err = %d", aKernelRegisterId, err);
		}
		return err;
	}


// Encodings from ARM ARM DDI0406A, section 9.2.1
enum TThumb2EEOpcode
	{
	EThumb2HDP,		// Handler Branch with Parameter
	EThumb2UNDEF,	// UNDEFINED
	EThumb2HB,		// Handler Branch, Handler Branch with Link
	EThumb2HBLP,	// Handle Branch with Link and Parameter
	EThumb2LDRF,	// Load Register from a frame
	EThumb2CHKA,	// Check Array
	EThumb2LDRL,	// Load Register from a literal pool
	EThumb2LDRA,	// Load Register (array operations)
	EThumb2STR		// Store Register to a frame
	};

//
// DRMDStepping::ShiftedRegValue
//
TUint32 DRMDStepping::ShiftedRegValue(DThread *aThread, TUint32 aInstruction, TUint32 aCurrentPC, TUint32 aStatusRegister)
	{
	LOG_MSG("DRMDStepping::ShiftedRegValue()");

	TUint32 shift = 0;
	if (aInstruction & 0x10)	// bit 4
		{
		shift = (arm_rs(aInstruction) == PC_REGISTER ? aCurrentPC + 8 : aStatusRegister) & 0xFF;
		}
	else
		{
		shift = arm_data_c(aInstruction);
		}
	
	TInt rm = arm_rm(aInstruction);
	
	TUint32 res = 0;
	if(rm == PC_REGISTER)
		{
		res = aCurrentPC + ((aInstruction & 0x10) ? 12 : 8);
		}
	else
		{
		TInt err = iChannel->ReadKernelRegisterValue(aThread, rm, res);
		if(err != KErrNone)
			{
			LOG_MSG2("DRMDStepping::ShiftedRegValue - Non-zero error code discarded: %d", err);
			}
		}

	switch(arm_data_shift(aInstruction))
		{
		case 0:			// LSL
			{
			res = shift >= 32 ? 0 : res << shift;
			break;
			}
		case 1:			// LSR
			{
			res = shift >= 32 ? 0 : res >> shift;
			break;
			}
		case 2:			// ASR
			{
			if (shift >= 32)
			shift = 31;
			res = ((res & 0x80000000L) ? ~((~res) >> shift) : res >> shift);
			break;
			}
		case 3:			// ROR/RRX
			{
			shift &= 31;
			if (shift == 0)
				{
				res = (res >> 1) | ((aStatusRegister & arm_carry_bit()) ? 0x80000000L : 0);
				}
			else
				{
				res = (res >> shift) | (res << (32 - shift));
				}
			break;
			}
		}

	return res & 0xFFFFFFFF;
}

//
// DRMDStepping::CurrentPC
//
// 
//
TInt DRMDStepping::CurrentPC(DThread* aThread, TUint32& aPC)
	{
	LOG_MSG("DRMDStepping::CurrentPC");

	TInt err = iChannel->ReadKernelRegisterValue(aThread, PC_REGISTER, aPC);
	if(err != KErrNone)
		{
		// We don't know the current PC for this thread!
		LOG_MSG("DRMDStepping::CurrentPC - Failed to read the current PC");
		
		return KErrGeneral;
		}

	LOG_MSG2("DRMDStepping::CurrentPC 0x%08x", aPC);

	return KErrNone;
	}

//
// DRMDStepping::CurrentCPSR
//
// 
//
TInt DRMDStepping::CurrentCPSR(DThread* aThread, TUint32& aCPSR)
	{
	LOG_MSG("DRMDStepping::CurrentCPSR");

	TInt err = iChannel->ReadKernelRegisterValue(aThread, STATUS_REGISTER, aCPSR);
	if(err != KErrNone)
		{
		// We don't know the current PC for this thread!
		LOG_MSG("DRMDStepping::CurrentPC - Failed to read the current CPSR");
		
		return KErrGeneral;
		}

	LOG_MSG2("DRMDStepping::CurrentCPSR 0x%08x", aCPSR);
	
	return KErrNone;
	}

//
// DRMDStepping::ModifyBreaksForStep
//
// Set a temporary breakpoint at the next instruction to be executed after the one at the current PC
// Disable the breakpoint at the current PC if one exists
//
TInt DRMDStepping::ModifyBreaksForStep(DThread *aThread, TUint32 aRangeStart, TUint32 aRangeEnd, /*TBool aStepInto,*/ TBool aResumeOnceOutOfRange, TBool aCheckForStubs, const TUint32 aNumSteps)
	{
	LOG_MSG2("DRMDStepping::ModifyBreaksForStep() Numsteps 0x%d",aNumSteps);

	// Validate arguments
	if (!aThread)
		{
		LOG_MSG("DRMDStepping::ModifyBreaksForStep() - No aThread specified to step");
		return KErrArgument;
		}

	// Current PC
	TUint32 currentPC;

	ReturnIfError(CurrentPC(aThread,currentPC));
	LOG_MSG2("Current PC: 0x%x", currentPC);

	// disable breakpoint at the current PC if necessary
	ReturnIfError(iChannel->iBreakManager->DisableBreakAtAddress(currentPC));

	// Current CPSR
	TUint32 statusRegister;

	ReturnIfError(CurrentCPSR(aThread,statusRegister));
	LOG_MSG2("Current CPSR: %x", statusRegister);

	TBool thumbMode = (statusRegister & ECpuThumb);
	if (thumbMode)
		LOG_MSG("Thumb Mode");

	TInt instSize = thumbMode ? 2 : 4;

	TBool changingModes = EFalse;

	TUint32 breakAddress = 0;

	TUint32 newRangeEnd = aRangeEnd;

	breakAddress = PCAfterInstructionExecutes(aThread, currentPC, statusRegister, instSize, /* aStepInto, */ newRangeEnd, changingModes);

	/*
	If there is already a user breakpoint at this address, we do not need to set a temp breakpoint. The program
	should simply stop at that address.	
	*/
	TBreakEntry* breakEntry = NULL;
	do
		{
		breakEntry = iChannel->iBreakManager->GetNextBreak(breakEntry);
		if(breakEntry && !iChannel->iBreakManager->IsTemporaryBreak(*breakEntry))
			{
			if ((breakEntry->iAddress == breakAddress) && ((breakEntry->iThreadSpecific && breakEntry->iId == aThread->iId) || (!breakEntry->iThreadSpecific && breakEntry->iId == aThread->iOwningProcess->iId)))
				{
				LOG_MSG("DRMDStepping::ModifyBreaksForStep - Breakpoint already exists at the step target address\n");

				// note also that if this is the case, we will not keep stepping if we hit a real breakpoint, so may as well set
				// the step count = 0.
				breakEntry->iNumSteps = 0;

				return KErrNone;
				}
			}
		} while(breakEntry);

	breakEntry = NULL;
	do
		{
		breakEntry = iChannel->iBreakManager->GetNextBreak(breakEntry);
		if(breakEntry && iChannel->iBreakManager->IsTemporaryBreak(*breakEntry))
			{
			if (breakEntry->iAddress == 0)
				{
				breakEntry->iId = aThread->iId;
				breakEntry->iAddress = breakAddress;
				breakEntry->iThreadSpecific = ETrue;

				TBool realThumbMode = (thumbMode && !changingModes) || (!thumbMode && changingModes);

				// Need to set the correct type of breakpoint for the mode we are in
				// and the the one we are changing into
				if(realThumbMode)
					{
					// We are remaining in Thumb mode
					breakEntry->iMode = EThumbMode;
					}
				else
					{
					// We are switching to ARM mode
					breakEntry->iMode = EArmMode;
					}

				breakEntry->iResumeOnceOutOfRange = aResumeOnceOutOfRange;
				breakEntry->iSteppingInto = ETrue /* aStepInto */;
				breakEntry->iRangeStart = 0;	// no longer used
				breakEntry->iRangeEnd = 0;		// no longer used

				LOG_MSG2("Adding temp breakpoint with id: %d", breakEntry->iBreakId);
				LOG_MSG2("Adding temp breakpoint with thread id: %d", aThread->iId);

				// Record how many more steps to go after we hit this one
				breakEntry->iNumSteps = aNumSteps;

				LOG_MSG3("Setting temp breakpoint id %d with %d steps to go\n", breakEntry->iBreakId, aNumSteps);

				return iChannel->iBreakManager->DoEnableBreak(*breakEntry, ETrue);			
				}
			}
		} while(breakEntry);
	LOG_MSG("ModifyBreaksForStep : Failed to set suitable breakpoint for stepping");
	return KErrNoMemory;	// should never get here
}

// End of file - d-rmd-stepping.cpp
