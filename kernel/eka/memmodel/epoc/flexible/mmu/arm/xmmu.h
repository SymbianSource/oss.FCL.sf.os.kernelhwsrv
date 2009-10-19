// Copyright (c) 2007-2009 Nokia Corporation and/or its subsidiary(-ies).
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

#include "cache_maintenance.inl"


/**
 @file
 @internalComponent
*/

#if defined(GNUC) && !defined(__MARM_ARM4__)
#define	__VOLATILE__	volatile
#else
#define __VOLATILE__
#endif

#if defined(__SMP__) && defined(__CPU_ARM11MP__) 
#define COARSE_GRAINED_TLB_MAINTENANCE
#define BROADCAST_TLB_MAINTENANCE
#endif



FORCE_INLINE void __arm_dmb()
	{
	#if defined(__CPU_ARMV6)
		// dmb instruction...
		#ifdef __GNUC__
			TInt zero = 0;
			asm __VOLATILE__ ("mcr p15, 0, %0, c7, c10, 5 " : : "r"(zero));
		#elif defined(__ARMCC__)
			TInt zero = 0;
			asm("mcr p15, 0, zero, c7, c10, 5 ");
		#elif defined(__GCCXML__)
			// empty
		#else
			#error Unknown compiler
		#endif
	#elif defined(__CPU_ARMV7)
		// deprecated CP15 version of DMB...
		#ifdef __GNUC__
			TInt zero = 0;
			asm __VOLATILE__ ("mcr p15, 0, %0, c7, c10, 5 " : : "r"(zero));
		#elif defined(__ARMCC__)
			TInt zero = 0;
			asm("mcr p15, 0, zero, c7, c10, 5 ");
		#elif defined(__GCCXML__)
			// empty
		#else
			#error Unknown compiler
		#endif
	#else
		// non inline version...
		__e32_memory_barrier();
	#endif
	}


FORCE_INLINE void __arm_dsb()
	{
	#if defined(__CPU_ARMV6)
		// drain write buffer...
		#ifdef __GNUC__
			TInt zero = 0;
			asm __VOLATILE__ ("mcr p15, 0, %0, c7, c10, 4 " : : "r"(zero));
		#elif defined(__ARMCC__)
			TInt zero = 0;
			asm("mcr p15, 0, zero, c7, c10, 4 ");
		#elif defined(__GCCXML__)
			// empty
		#else
			#error Unknown compiler
		#endif
	#elif defined(__CPU_ARMV7)
		// deprecated CP15 version of DSB...
		#ifdef __GNUC__
			TInt zero = 0;
			asm __VOLATILE__ ("mcr p15, 0, %0, c7, c10, 4 " : : "r"(zero));
		#elif defined(__ARMCC__)
			TInt zero = 0;
			asm("mcr p15, 0, zero, c7, c10, 4 ");
		#elif defined(__GCCXML__)
			// empty
		#else
			#error Unknown compiler
		#endif
	#else
		// non inline version...
		__e32_io_completion_barrier();
	#endif
	}


extern "C" void __e32_instruction_barrier();

FORCE_INLINE void __arm_isb()
	{
	#if defined(__CPU_ARMV6)
		// prefetch flush...
		#ifdef __GNUC__
			TInt zero = 0;
			asm __VOLATILE__ ("mcr p15, 0, %0, c7, c5, 4 " : : "r"(zero));
		#elif defined(__ARMCC__)
			TInt zero = 0;
			asm("mcr p15, 0, zero, c7, c5, 4 ");
		#elif defined(__GCCXML__)
			// empty
		#else
			#error Unknown compiler
		#endif
	#elif defined(__CPU_ARMV7)
		// deprecated CP15 version of ISB...
		#ifdef __GNUC__
			TInt zero = 0;
			asm __VOLATILE__ ("mcr p15, 0, %0, c7, c5, 4 " : : "r"(zero));
		#elif defined(__ARMCC__)
			TInt zero = 0;
			asm("mcr p15, 0, zero, c7, c5, 4 ");
		#elif defined(__GCCXML__)
			// empty
		#else
			#error Unknown compiler
		#endif
	#else
		// non inline version...
		__e32_instruction_barrier();
	#endif
	}


/**
Branch predictor invalidate all
*/
FORCE_INLINE void __arm_bpiall()
	{
	#ifdef __GNUC__
		TInt zero = 0;
		asm __VOLATILE__ ("mcr p15, 0, %0, c7, c5, 6 " : : "r"(zero));
	#elif defined(__ARMCC__)
		TInt zero = 0;
		asm("mcr p15, 0, zero, c7, c5, 6 ");
	#elif defined(__GCCXML__)
		// empty
	#else
		#error Unknown compiler
	#endif
	}


#ifdef __SMP__

/**
Branch predictor invalidate all inner-shareable
*/
FORCE_INLINE void __arm_bpiallis()
	{
	// branch predictor invalidate all inner-shareable
	#ifdef __GNUC__
		TInt zero = 0;
		asm __VOLATILE__ ("mcr p15, 0, %0, c7, c1, 6 " : : "r"(zero));
	#elif defined(__ARMCC__)
		TInt zero = 0;
		asm("mcr p15, 0, zero, c7, c1, 6 ");
	#elif defined(__GCCXML__)
		// empty
	#else
		#error Unknown compiler
	#endif
	}

#endif


/**	
This will make sure that the change in page directory is visible by H/W Page-Table Walk.  
Call this function when a single entry in page directory is changed.
*/
FORCE_INLINE void SinglePdeUpdated(TPde* aPde)
  	{
	CacheMaintenance::SinglePdeUpdated((TLinAddr)aPde);
  	}


#ifdef BROADCAST_TLB_MAINTENANCE

/**
Signal other CPU cores to perform TLB maintenance.

@param aLinAddrAndAsid	If == 0, then InvalidateTLB;
						if < KMmuAsidCount, then InvalidateTLBForAsid;
						else InvalidateTLBForPage.
*/
extern void BroadcastInvalidateTLB(TLinAddr aLinAddrAndAsid=0);

#endif


/**
Invalidate a single I+D TLB entry on this CPU core only.
@param aLinAddrAndAsid Virtual address of a page of memory ORed with the ASID value.
*/
FORCE_INLINE void LocalInvalidateTLBForPage(TLinAddr aLinAddrAndAsid)
	{
	#ifdef __GNUC__
		#if defined(__CPU_ARM11MP__) // why?...
			asm __VOLATILE__ ("mcr p15, 0, %0, c8, c7, 3 " : : "r"(aLinAddrAndAsid));
		#else
			asm __VOLATILE__ ("mcr p15, 0, %0, c8, c7, 1 " : : "r"(aLinAddrAndAsid));
		#endif
	#elif defined(__ARMCC__)
		#if defined(__CPU_ARM11MP__) // why?...
			asm("mcr p15, 0, aLinAddrAndAsid, c8, c7, 3 ");
		#else
			asm("mcr p15, 0, aLinAddrAndAsid, c8, c7, 1 ");
		#endif
	#elif defined(__GCCXML__)
		// empty
	#else
		#error Unknown compiler
	#endif
	__arm_bpiall();
	__arm_dsb();
	__arm_isb();
	}


/**
Invalidate a single I+D TLB entry on all CPU cores.
@param aLinAddrAndAsid Virtual address of a page of memory ORed with the ASID value.
*/
FORCE_INLINE void InvalidateTLBForPage(TLinAddr aLinAddrAndAsid)
	{
	#ifdef BROADCAST_TLB_MAINTENANCE
		BroadcastInvalidateTLB(aLinAddrAndAsid);
	#elif !defined(__SMP__)
		LocalInvalidateTLBForPage(aLinAddrAndAsid);
	#else // __SMP__
		// inner-shareable invalidate...
		#ifdef __GNUC__
			asm __VOLATILE__ ("mcr p15, 0, %0, c8, c3, 1 " : : "r"(aLinAddrAndAsid));
		#elif defined(__ARMCC__)
			asm("mcr p15, 0, aLinAddrAndAsid, c8, c3, 1 ");
		#elif defined(__GCCXML__)
			// empty
		#else
			#error Unknown compiler
		#endif
		__arm_bpiallis();
		__arm_dsb();
		__arm_isb();
	#endif
	}


/**
Invalidate entire TLB on this CPU only
*/
FORCE_INLINE void LocalInvalidateTLB()
	{
	#ifdef __GNUC__
		asm __VOLATILE__ ("mcr p15, 0, r0, c8, c7, 0 ");
	#elif defined(__ARMCC__)
		TInt dummy = 0; // damned RVCT
		asm("mcr p15, 0, dummy, c8, c7, 0 ");
	#elif defined(__GCCXML__)
		// empty
	#else
		#error Unknown compiler
	#endif
	__arm_bpiall();
	__arm_dsb();
	__arm_isb();
	}


/**
Invalidate entire TLB on all CPU cores.
*/
FORCE_INLINE void InvalidateTLB()
	{
	#ifdef BROADCAST_TLB_MAINTENANCE
		BroadcastInvalidateTLB(0);
	#elif !defined(__SMP__)
		LocalInvalidateTLB();
	#else // __SMP__
		// inner-shareable invalidate...
		#ifdef __GNUC__
			TInt zero = 0;
			asm __VOLATILE__ ("mcr p15, 0, %0, c8, c3, 0 " : : "r"(zero));
		#elif defined(__ARMCC__)
			TInt zero = 0;
			asm("mcr p15, 0, zero, c8, c3, 0 ");
		#elif defined(__GCCXML__)
			// empty
		#else
			#error Unknown compiler
		#endif
		__arm_bpiallis();
		__arm_dsb();
		__arm_isb();
	#endif
	}


#if defined(__CPU_ARM1136__) && !defined(__CPU_ARM1136_ERRATUM_424067_FIXED)
#define INVALIDATE_TLB_BY_ASID_BROKEN
#endif
#if defined(__CPU_ARM1176__) && !defined(__CPU_ARM1176_ERRATUM_424692_FIXED)
#define INVALIDATE_TLB_BY_ASID_BROKEN
#endif
 

__ASSERT_COMPILE(KKernelOsAsid==0); // InvalidateTLBForAsid assumes this


/**
Invalidate all TLB entries which match the given ASID value (current CPU only)
*/
FORCE_INLINE void LocalInvalidateTLBForAsid(TUint aAsid)
	{
#ifndef INVALIDATE_TLB_BY_ASID_BROKEN
	if(aAsid&=0xff)
		{
#if defined(__CPU_CORTEX_A9__) && !defined(__CPU_ARM_A9_ERRATUM_571618_FIXED)
		__arm_dmb();	// ARM Cortex-A9 MPCore erratum 571618 workaround
						// Execute memory barrier before interruptible CP15 operations
#endif
		// invalidate all I+D TLB entries for ASID...
		#ifdef __GNUC__
			asm __VOLATILE__ ("mcr p15, 0, %0, c8, c7, 2 " : : "r"(aAsid));
		#elif defined(__ARMCC__)
			asm("mcr p15, 0, aAsid, c8, c7, 2 ");
		#elif defined(__GCCXML__)
			// empty
		#else
			#error Unknown compiler
		#endif
		}
	else
		// ASID==0 means 'kernel' memory. We have to invalidate the entire TLB here
		// as this is the only way of getting rid of global entries...
#endif
		{
		// invalidate entire TLB...
		#ifdef __GNUC__
			asm __VOLATILE__ ("mcr p15, 0, r0, c8, c7, 0 ");
		#elif defined(__ARMCC__)
			TInt dummy = 0; // damned RVCT
			asm("mcr p15, 0, dummy, c8, c7, 0 ");
		#elif defined(__GCCXML__)
			// empty
		#else
			#error Unknown compiler
		#endif
		}
	__arm_bpiall();
	__arm_dsb();
	__arm_isb();
	}


/**
Invalidate all TLB entries which match the given ASID value on all CPU cores.
*/
FORCE_INLINE void InvalidateTLBForAsid(TUint aAsid)	
	{
	aAsid &= 0xff;
	#ifdef BROADCAST_TLB_MAINTENANCE
		BroadcastInvalidateTLB(aAsid);
	#elif !defined(__SMP__)
		LocalInvalidateTLBForAsid(aAsid);
	#else // __SMP__
		if(aAsid!=0)
			{
#if defined(__CPU_CORTEX_A9__) && !defined(__CPU_ARM_A9_ERRATUM_571618_FIXED)
			__arm_dmb();	// ARM Cortex-A9 MPCore erratum 571618 workaround
							// Execute memory barrier before interruptible CP15 operations
#endif
			// invalidate all I+D TLB entries for ASID...
			#ifdef __GNUC__
				asm __VOLATILE__ ("mcr p15, 0, %0, c8, c3, 2 " : : "r"(aAsid));
			#elif defined(__ARMCC__)
				asm("mcr p15, 0, aAsid, c8, c3, 2 ");
			#elif defined(__GCCXML__)
				// empty
			#else
				#error Unknown compiler
			#endif
			}
		else
			{
			// ASID==0 means 'kernel' memory. We have to invalidate the entire TLB here
			// as this is the only way of getting rid of global entries...
			#ifdef __GNUC__
				asm __VOLATILE__ ("mcr p15, 0, %0, c8, c3, 0 " : : "r"(aAsid));
			#elif defined(__ARMCC__)
				asm("mcr p15, 0, aAsid, c8, c3, 0 ");
			#elif defined(__GCCXML__)
				// empty
			#else
				#error Unknown compiler
			#endif
			}
		__arm_bpiallis();
		__arm_dsb();
		__arm_isb();
	#endif
	}


/**
Return the virtual address of the page directory used for address space
\a aOsAsid. Note, the global page directory is mapped after each
address space specific page director in a way which means that it
appears to be a single contiguous page directory which maps the
entire 32bit virtual address range. I.e. the returned page directory
address can be simply indexed by any virtual address without regard
to whether it belongs to the given address space or lies in the
global region.
*/
FORCE_INLINE TPde* Mmu::PageDirectory(TInt aOsAsid)
	{
	return (TPde*)(KPageDirectoryBase+(aOsAsid<<KPageDirectoryShift));
	}


/**
Return the virtual address of the Page Directory Entry (PDE) used to map
the region containing the virtual address \a aAddress in the address space
\a aOsAsid.
*/
FORCE_INLINE TPde* Mmu::PageDirectoryEntry(TInt aOsAsid, TLinAddr aAddress)
	{
	return PageDirectory(aOsAsid) + (aAddress>>KChunkShift);
	}


/**
Return the physical address mapped by the section mapping contained
in the given Page Directory Entry \a aPde. If \a aPde is not a
section mapping, then KPhysAddrInvalid is returned.
*/
FORCE_INLINE TPhysAddr Mmu::PdePhysAddr(TPde aPde)
	{
	if((aPde&KPdePresentMask)==KArmV6PdeSection)
		return aPde&KPdeSectionAddrMask;
	return KPhysAddrInvalid;
	}


#ifdef __CPU_MEMORY_TYPE_REMAPPING

/*
Bits in a PTE which represent access permissions...

AP2 AP1 AP0		usr	wr
0	0	x		n	y
0	1	x		y	y
1	0	x		n	n
1	1	x		y	n
*/

/**
Modify a Page Table Entry (PTE) value so it restricts access to the memory it maps.
The returned PTE value is the same as \a aPte but with its access permissions set
to read-only if \a aReadOnly is true, and set to allow no access if \a aReadOnly is false.
*/
FORCE_INLINE TPte Mmu::MakePteInaccessible(TPte aPte, TBool aReadOnly)
	{
	__NK_ASSERT_DEBUG((aPte&KArmV6PteTypeMask)!=KArmV6PteLargePage);
	if(aPte&KPtePresentMask)
		{
		__NK_ASSERT_DEBUG((bool)(aPte&KArmV6PteSmallTEX1)==(bool)(aPte&KArmV6PteSmallXN)); // TEX1 should be a copy of XN
		if(aReadOnly)
			aPte |= KArmV6PteAP2; // make read only
		else
			aPte &= ~KPtePresentMask; // make inaccessible
		}
	return aPte;
	}


/**
Modify a Page Table Entry (PTE) value so it allows greater access to the memory it maps.
The returned PTE value is the same as \a aPte but with its access permissions set
to read/write if \a aWrite is true, and set to read-only if \a aWrite is false.
*/
FORCE_INLINE TPte Mmu::MakePteAccessible(TPte aPte, TBool aWrite)
	{
	__NK_ASSERT_DEBUG((aPte&KArmV6PteTypeMask)!=KArmV6PteLargePage);
	if((aPte&KPtePresentMask)==0)
		{
		// wasn't accessible, make it so...
		if(aPte&KArmV6PteSmallTEX1)
			aPte |= KArmV6PteSmallXN; // restore XN by copying from TEX1
		aPte |= KArmV6PteSmallPage;
		aPte |= KArmV6PteAP2; // make read only
		}
	if(aWrite)
		aPte &= ~KArmV6PteAP2; // make writable
	return aPte;
	}


#else // not __CPU_MEMORY_TYPE_REMAPPING

/*
Bits in a PTE which represent access permissions...

AP2 AP1 AP0		usr	wr
0	0	0
0	0	1		n	y
0	1	0
0	1	1		y	y
1	0	0
1	0	1		n	n
1	1	0		y	n
1	1	1
*/

FORCE_INLINE TPte Mmu::MakePteInaccessible(TPte aPte, TBool aReadOnly)
	{
	__NK_ASSERT_DEBUG((aPte&KArmV6PteTypeMask)!=KArmV6PteLargePage);
	if(aPte&KPtePresentMask)
		{
		if(!aReadOnly)
			{
			// copy XN to AP0...
			if(aPte&KArmV6PteSmallXN)
				aPte |= KArmV6PteAP0;
			else
				aPte &= ~KArmV6PteAP0;

			// make inaccessible...
			aPte &= ~KPtePresentMask;
			}
		else
			{
			// make read only...
			aPte |= KArmV6PteAP2; // make read only
			if(aPte&KArmV6PteAP1)
				aPte &= ~KArmV6PteAP0; // correct AP0
			}
		}
	return aPte;
	}


FORCE_INLINE TPte Mmu::MakePteAccessible(TPte aPte, TBool aWrite)
	{
	__NK_ASSERT_DEBUG((aPte&KArmV6PteTypeMask)!=KArmV6PteLargePage);
	if((aPte&KPtePresentMask)==0)
		{
		// wasn't accessible, make it so...
		if(aPte&KArmV6PteAP0)
			aPte |= KArmV6PteSmallXN; // restore XN by copying from AP0
		aPte |= KArmV6PteAP0;
		aPte |= KArmV6PteSmallPage;

		// make read only...
		aPte |= KArmV6PteAP2; // make read only
		if(aPte&KArmV6PteAP1)
			aPte &= ~KArmV6PteAP0; // correct AP0
		}
	if(aWrite)
		{
		// make writable...
		aPte &= ~KArmV6PteAP2;
		aPte |= KArmV6PteAP0; 
		}
	return aPte;
	}

#endif // __CPU_MEMORY_TYPE_REMAPPING


/**
Return true if a Page Table Entry (PTE) only allows read-only access to memory.
*/
FORCE_INLINE TBool Mmu::IsPteReadOnly(TPte aPte)
	{
	__NK_ASSERT_DEBUG(aPte&KPtePresentMask); // read-only state is ambiguous if pte not present
	return aPte&KArmV6PteAP2;
	}


/**
Return true if a Page Table Entry (PTE) doesn't allow any access to the memory.
*/
FORCE_INLINE TBool Mmu::IsPteInaccessible(TPte aPte)
	{
	return !(aPte&KPtePresentMask);
	}

/**
Return true if the Page Table Entry \a aNewPte allows greater access to
memory that \a aOldPte. Only the permissions read/write, read-only and no-access
are considered, not any execute or privileged access.
*/
FORCE_INLINE TBool Mmu::IsPteMoreAccessible(TPte aNewPte, TPte aOldPte)
	{
	if(aNewPte&aOldPte&KPtePresentMask)			// if ptes both present
		return (aOldPte&~aNewPte)&KArmV6PteAP2;	//   check for more writable
	else										// else
		return aNewPte&KPtePresentMask;			//   check for new pte being present
	}


/**
Bit flag values representing the memory mapping differences governed by
MMU Page Directory Entries (PDEs). Memory which differs in #TPdeType can
not be mapped using the same Page Table, as they would share the same PDE
entry.
*/
enum TPdeType
	{
	/**
	Legacy (and little-used/unused?) ARM attribute.
	This could potentially be removed (see DMemoryMapping::PdeType()).
	*/
	EPdeTypeECC				= 1<<0,

	/**
	Total number of combinations of #TPdeType values.
	*/
	ENumPdeTypes			= 2
	};


/**
Bit flag values representing the memory mapping differences governed by
MMU Page Table Entries (PTEs).
*/
enum TPteType
	{
	/**
	PTE grants user mode access to memory.
	*/
	EPteTypeUserAccess		= EUser,

	/**
	PTE grants write access to memory.
	*/
	EPteTypeWritable		= EReadWrite,

	/**
	PTE grants execute  access to memory.
	*/
	EPteTypeExecutable		= EExecute,

	/**
	PTE is 'global'. I.e. the memory it maps is intended to be accessible
	in all process contexts, i.e. for mappings at virtual address >= KGlobalMemoryBase.
	The MMU uses this to tag TLB entries as valid for all ASIDs.
	*/
	EPteTypeGlobal			= 1<<3,

	/**
	Total number of combinations of #TPteType values.
	*/
	ENumPteTypes			= 16
	};

__ASSERT_COMPILE(EPteTypeUserAccess==(1<<0));
__ASSERT_COMPILE(EPteTypeWritable==(1<<1));
__ASSERT_COMPILE(EPteTypeExecutable==(1<<2));


#define MMU_SUPPORTS_EXECUTE_NEVER


/**
Return the #TPdeType for memory with the given attributes value.
*/
FORCE_INLINE TUint Mmu::PdeType(TMemoryAttributes aAttributes)
	{
	return aAttributes&EMemoryAttributeUseECC ? EPdeTypeECC : 0;
	}


/**
Return the #TPteType to use for memory mappings requiring the given access permissions
and Global attribute. The global flag is true if #EPteTypeGlobal is to be set.
*/
FORCE_INLINE TUint Mmu::PteType(TMappingPermissions aPermissions, TBool aGlobal)
	{
	__NK_ASSERT_DEBUG(aPermissions&EUser || aGlobal); // can't have supervisor local memory

	TUint pteType =	(aPermissions&(EUser|EReadWrite|EExecute));
	if(aGlobal)
		pteType |= EPteTypeGlobal;

	__NK_ASSERT_DEBUG(pteType<ENumPteTypes);

	return pteType;
	}


/**
Test if a memory access is allowed by a given mapping type.

@param aPteType				#TPteType used for a mapping. E.g. TMemoryMappingBase::PteType()
@param aAccessPermissions	Flags from #TMappingPermissions indicating the memory access
							required.

@return True if a memory access requested with permissions \a aAccessPermissions
		is allowed on a mapping of the specified #TPteType.
		False if the access is not allowed.
*/
FORCE_INLINE TBool Mmu::CheckPteTypePermissions(TUint aPteType, TUint aAccessPermissions)
	{
	aAccessPermissions &= EUser|EReadWrite|EExecute;
	return (aPteType&aAccessPermissions)==aAccessPermissions;
	}


/**
Extract the #TMappingPermissions corresponding to a given #TPteType.
*/
FORCE_INLINE TMappingPermissions Mmu::PermissionsFromPteType(TUint aPteType)
	{
	return (TMappingPermissions)(aPteType&(EPteTypeUserAccess|EPteTypeWritable|EPteTypeExecutable));
	}

extern void UserWriteFault(TLinAddr aAddr);
extern void UserReadFault(TLinAddr aAddr);


//
// TODO: Move these to NKern
//

FORCE_INLINE void inline_DisableAllInterrupts()
	{
#ifdef __GNUC__
	#ifdef __CPU_ARM_HAS_CPS
		CPSIDIF;
	#else
		TInt reg;
		asm __VOLATILE__ ("mrs %0, cpsr" : "=r"(reg));
		asm __VOLATILE__ ("orr %0, %0, #0xc0" : : "r"(reg));
		asm __VOLATILE__ ("msr cpsr_c, %0" : : "r"(reg));
	#endif
/*
#elif defined(__ARMCC__)
	#if defined(__CPU_ARM_HAS_CPS) && __ARMCC_VERSION>=300000
		asm("cpsid if");
	#else
		TInt reg;
		asm("mrs reg, cpsr");
		asm("orr reg, reg, #0xc0");
		asm("msr cpsr_c, reg");
	#endif
*/
#else
	NKern::DisableAllInterrupts();
#endif
	}

FORCE_INLINE void inline_EnableAllInterrupts()
	{
#ifdef __GNUC__
	#ifdef __CPU_ARM_HAS_CPS
		CPSIEIF;
	#else
		TInt reg;
		asm __VOLATILE__ ("mrs %0, cpsr" : "=r"(reg));
		asm __VOLATILE__ ("bic %0, %0, #0xc0" : : "r"(reg));
		asm __VOLATILE__ ("msr cpsr_c, %0" : : "r"(reg));
	#endif
/*
#elif defined(__ARMCC__)
	#if defined(__CPU_ARM_HAS_CPS) && __ARMCC_VERSION>=300000
		asm("cpsie if");
	#else
		TInt reg;
		asm("mrs reg, cpsr");
		asm("bic reg, reg, #0xc0");
		asm("msr cpsr_c, reg");
	#endif
*/
#else
	NKern::EnableAllInterrupts();
#endif
	}


#ifndef	__SMP__
#undef __SPIN_LOCK_IRQ
#define __SPIN_LOCK_IRQ(lock)					(inline_DisableAllInterrupts())
#undef __SPIN_UNLOCK_IRQ
#define __SPIN_UNLOCK_IRQ(lock)					(inline_EnableAllInterrupts())
#undef __SPIN_FLASH_IRQ
#define __SPIN_FLASH_IRQ(lock)					(inline_EnableAllInterrupts(),inline_DisableAllInterrupts(),((TBool)TRUE))
#endif


/**
Indicate whether a PDE entry maps a page table.

@param aPde The PDE entry in question.
*/
FORCE_INLINE TBool Mmu::PdeMapsPageTable(TPde aPde)
	{
	return (aPde & KPdeTypeMask) == KArmV6PdePageTable;
	}


/**
Indicate whether a PDE entry maps a section.

@param aPde The PDE entry in question.
*/
FORCE_INLINE TBool Mmu::PdeMapsSection(TPde aPde)
	{
	return (aPde & KPdeTypeMask) == KArmV6PdeSection;
	}
