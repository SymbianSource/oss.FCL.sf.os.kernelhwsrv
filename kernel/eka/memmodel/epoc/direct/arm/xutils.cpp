// Copyright (c) 1994-2009 Nokia Corporation and/or its subsidiary(-ies).
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
// e32\memmodel\epoc\direct\arm\xutils.cpp
// 
//

//#define __DBG_MON_FAULT__
//#define __RAM_LOADED_CODE__
//#define __EARLY_DEBUG__

#include "arm_mem.h"

/**
	@pre	Call in a thread context.
	@pre	Interrupts must be enabled.
	@pre	Kernel must be unlocked.
 */
EXPORT_C TPhysAddr Epoc::LinearToPhysical(TLinAddr aLinAddr)
	{
	CHECK_PRECONDITIONS(MASK_KERNEL_UNLOCKED|MASK_INTERRUPTS_ENABLED|MASK_NOT_ISR|MASK_NOT_IDFC,"Epoc::LinearToPhysical");
	__KTRACE_OPT(KMMU,Kern::Printf("Epoc::LinearToPhysical(%08x)", aLinAddr));
#ifdef __CPU_HAS_MMU
	// Deal with faking no MMU on hardware with an MMU
	if (!Arm::MmuActive())
		return aLinAddr;
	TUint32 phys=0xffffffffu;
	TUint32 pdphys = Arm::MmuTTBR0();
	TUint32 pdlin = TheSuperPage().iPageDir;
	TUint32 offset = pdlin - pdphys;	// offset from phys to lin for page directory/tables
	const TUint32* pageDir = (const TUint32*)pdlin;
	TUint32 pde = pageDir[aLinAddr>>20];
	__KTRACE_OPT(KMMU,Kern::Printf("PDE %08x", pde));
	if ((pde&3)==2)
		{
		// section
		phys = (pde&0xfff00000u) | (aLinAddr&0x000fffffu);
		}
	else if ((pde&3)==1)
		{
		// page table
		TUint32 ptphys = pde & 0xfffffc00u;
		const TUint32* pageTable = (const TUint32*)(ptphys + offset);
		TUint32 pte = pageTable[(aLinAddr>>12)&0xff];
		__KTRACE_OPT(KMMU,Kern::Printf("PTE %08x", pte));
		if ((pte&3)==1)
			{
			// 64K page
			phys = (pte&0xffff0000u) | (aLinAddr&0x0000ffffu);
			}
		else if (pte&3)
			{
			// 4K page
			phys = (pte&0xfffff000u) | (aLinAddr&0x00000fffu);
			}
		else
			{
			FAULT();
			}
		}
	else
		{
		FAULT();
		}
	__KTRACE_OPT(KMMU,Kern::Printf("Physical address = %08x",phys));
	return phys;
#else
	// real non-MMU hardware
	return aLinAddr;
#endif
	}

