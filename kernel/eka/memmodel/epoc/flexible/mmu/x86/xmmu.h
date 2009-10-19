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
// This will make sure that the changes in page tables are visible by H/W Page-Table Walk.  
// Call this function when two and more consecutive entries in page table are changed.
// 
//

/**
FORCE_INLINE macro in GCC works differently from in VC and armcc.
*/
#ifdef _MSC_VER
#define REALLY_INLINE __forceinline
#else
#ifdef __GNUC__
#if __GNUC__ < 3
#define REALLY_INLINE inline
#else
#define REALLY_INLINE inline __attribute__ ((always_inline))
#endif
#else
#define REALLY_INLINE inline
#endif
#endif

/**	
This will make sure that the change in page directory is visible by H/W Page-Table Walk.  
Call this function when a single entry in page directory is changed.
*/
extern void SinglePdeUpdated(TPde* aPde);


extern void __fastcall DoInvalidateTLBForPage(TLinAddr aLinAddrAndAsid);
extern void DoInvalidateTLB();


/**
Invalidate a single I+D TLB entry on this CPU core only.
@param aLinAddrAndAsid Virtual address of a page of memory ORed with the ASID value.
*/
REALLY_INLINE void __fastcall LocalInvalidateTLBForPage(TLinAddr aLinAddrAndAsid)
	{
	DoInvalidateTLBForPage(aLinAddrAndAsid);
	}


#ifndef __SMP__

/**
Invalidate a single I+D TLB entry
@param aLinAddrAndAsid Virtual address of a page of memory ORed with the ASID value.
*/
REALLY_INLINE void __fastcall InvalidateTLBForPage(TLinAddr aLinAddrAndAsid)
	{
	DoInvalidateTLBForPage(aLinAddrAndAsid);
	}


/**
Invalidate entire TLB
*/
REALLY_INLINE void InvalidateTLB()
	{
	DoInvalidateTLB();
	}


#else // __SMP__


#define COARSE_GRAINED_TLB_MAINTENANCE

/**
Invalidate a single I+D TLB entry.
@param aLinAddrAndAsid Virtual address of a page of memory ORed with the ASID value.
*/
extern void InvalidateTLBForPage(TLinAddr aLinAddrAndAsid);


/**
Invalidate entire TLB
*/
extern void InvalidateTLB();


#endif // __SMP__


/**
Invalidate all TLB entries which match the given ASID value
*/
extern void InvalidateTLBForAsid(TUint aAsid);


FORCE_INLINE TPde* Mmu::PageDirectory(TInt aOsAsid)
	{
	return (TPde*)(KPageDirectoryBase+(aOsAsid<<KPageDirectoryShift));
	}


FORCE_INLINE TPde* Mmu::PageDirectoryEntry(TInt aOsAsid, TLinAddr aAddress)
	{
	return PageDirectory(aOsAsid) + (aAddress>>KChunkShift);
	}


FORCE_INLINE TPhysAddr Mmu::PdePhysAddr(TPde aPde)
	{
	if ((aPde&(KPdePtePresent|KPdeLargePage)) == (KPdePtePresent|KPdeLargePage))
		return aPde & KPdeLargePagePhysAddrMask;
	return KPhysAddrInvalid;
	}


FORCE_INLINE TPte Mmu::MakePteInaccessible(TPte aPte, TBool aReadOnly)
	{
	if(aReadOnly)
		return aPte&~KPdePteWrite;
	else
		return aPte&~KPdePtePresent;
	}


FORCE_INLINE TPte Mmu::MakePteAccessible(TPte aPte, TBool aWrite)
	{
	if((aPte&KPdePtePresent)==0)
		{
		aPte |= KPdePtePresent;
		aPte &= ~KPdePteWrite;
		}
	if(aWrite)
		aPte |= KPdePteWrite;
	return aPte;
	}


FORCE_INLINE TBool Mmu::IsPteReadOnly(TPte aPte)
	{
	__NK_ASSERT_DEBUG(aPte&KPdePtePresent); // read-only state is ambiguous if pte not present
	return !(aPte&KPdePteWrite);
	}


FORCE_INLINE TBool Mmu::IsPteInaccessible(TPte aPte)
	{
	return !(aPte&KPdePtePresent);
	}


FORCE_INLINE TBool Mmu::IsPteMoreAccessible(TPte aNewPte, TPte aOldPte)
	{
	if(aNewPte&aOldPte&KPdePtePresent)			// if ptes both present
		return (aNewPte&~aOldPte)&KPdePteWrite;	//   check for more writable
	else										// else
		return aNewPte&KPdePtePresent;			//   check for new pte being present
	}


enum TPdeType
	{
	ENumPdeTypes			= 1
	};


enum TPteType
	{
	EPteTypeUserAccess		= EUser,
	EPteTypeWritable		= EReadWrite,
	EPteTypeGlobal			= 1<<2,
	ENumPteTypes			= 8
	};

__ASSERT_COMPILE(EPteTypeUserAccess==(1<<0));
__ASSERT_COMPILE(EPteTypeWritable==(1<<1));


FORCE_INLINE TUint Mmu::PdeType(TMemoryAttributes /*aAttributes*/)
	{
	return 0;
	}


FORCE_INLINE TUint Mmu::PteType(TMappingPermissions aPermissions, TBool aGlobal)
	{
	__NK_ASSERT_DEBUG(aPermissions&EUser || aGlobal); // can't have supervisor local memory

	TUint pteType =	(aPermissions&(EUser|EReadWrite));
	if(aGlobal)
		pteType |= EPteTypeGlobal;

	__NK_ASSERT_DEBUG(pteType<ENumPteTypes);

	return pteType;
	}


FORCE_INLINE TBool Mmu::CheckPteTypePermissions(TUint aPteType, TUint aAccessPermissions)
	{
	aAccessPermissions &= EUser|EReadWrite;
	return (aPteType&aAccessPermissions)==aAccessPermissions;
	}


FORCE_INLINE TMappingPermissions Mmu::PermissionsFromPteType(TUint aPteType)
	{
	return (TMappingPermissions)(aPteType&(EPteTypeUserAccess|EPteTypeWritable));
	}

extern void __fastcall UserWriteFault(TLinAddr aAddr);
extern void __fastcall UserReadFault(TLinAddr aAddr);


/**
Indicate whether a PDE entry maps a page table.

@param aPde The PDE entry in question.
*/
FORCE_INLINE TBool Mmu::PdeMapsPageTable(TPde aPde)
	{
	return (aPde & KPdeLargePage) == 0;
	}


/**
Indicate whether a PDE entry maps a section.

@param aPde The PDE entry in question.
*/
FORCE_INLINE TBool Mmu::PdeMapsSection(TPde aPde)
	{
	return (aPde & KPdeLargePage) != 0;
	}
