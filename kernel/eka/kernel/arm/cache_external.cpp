// Copyright (c) 2005-2009 Nokia Corporation and/or its subsidiary(-ies).
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
// eka\kernel\arm\cachel2.cpp
//
// Support for the following external cache controllers:
//	- L210
//	- L220
//	- PL310
//

/* Errata status:
 *
 * General note that applies to all errata:
 * 	External cache configuration in bootstrap is implemented in baseport (usually HardwareInitialise
 *	procedure). Baseport should ensure errata fixes are properly implemented in bootstrap.
 *
 * L210 Errata status:
 *
 * 	- Incorrect IDLE assertion on BUSY to IDLE transaction transition on master ports.
 * 		There is no Software work-around for this erratum.
 * 
 *	- BWABT event only asserted if abort response is received on the last access of buffered write
 *	  or eviction transaction.
 *		There is no Software work-around for this erratum.
 * 
 *	- IDLE can be active while a transaction starts on a master port (HTRANSMx = NSEQ).
 *		There is no Software work-around for this erratum.
 * 
 *	- Abort response can be lost on a nonbufferable write access with master port in 32-bit configuration.
 *		There is no Software work-around for this erratum.
 * 
 *	- Evictions can cause write data from write buffer to be overwritten.
 *		The work-around for this erratum is to use the L210 as a write-through cache only, which
 *		is enforced in baseport.
 * 
 *	- Incorrect eviction buffer request deassertion can cause WAW hazard or outof-date data read.
 *		The work-around for this erratum is to use the L210 as a write-through cache only, which
 *		is enforced in baseport.
 * 
 *	- Read-after-write hazards can be incorrectly handled between write allocation linefills and
 *	  dirty data evictions in three master ports configuration.
 *		The software work-around for this erratum is to change memory regions attributes so that
 *		allocate on read miss only policy is used for all outer write-back regions.
 * 		Note:
 *			Write allocate is supported only for systems using ARMv6 extensions only
 *		Status:
 *			Fix not implemented.
 * 
 *	- Starting a background maintenance operation while a line is sitting in the writeallocate buffer
 *	  can prevent that line from being allocated
 *		The software work-around is to perform an explicit Cache Sync operation before initiating a
 *		background clean or clean-and-invalidate cache maintenance operation.
 *		Status:
 *			Fix implemented unconditionally because Kernel is using background maintenance
 *			operation only during soft restart and performance issue is minimal/nonexistant.
 * 
 *	- The Cache Sync operation does not guarantee that the Eviction Buffer is empty
 * 		From ARM Support:
 *			To insure the error does not occur, you can do a Cache Sync operation followed by a SWP
 *			to non-cacheable memory.
 *		Status:
 *			Fix implemented in Kernel when 
 *				__ARM_L210_ERRATA_CACHESYNC_DOESNT_FLUSH_EVICTION_BUFFER_FIXED
 *			is not defined in variant.mmh
 *
 * L220 Errata status:
 *
 *	- 484863: The Cache Sync operation does not guarantee that the Eviction Buffer is empty.
 *		The software workaround is to do a dummy STR to Non-cacheable Normal Memory before launching
 *		the Cache Sync operation. Baseport (in bootstrap) MUST ensure that memory mapping of
 *		BTP_Uncached entry in BootTable is either:
 * 			- MEMORY_UNCACHED (memory type remapping on) or,
 * 			- CACHE_BUFC	  (memory type remapping off)
 * 		Status:
 *			Fix implemented in Kernel when 
 *				__ARM_L220_ERRATUM_484863_FIXED
 *			is not defined in variant.mmh
 *
 * PL310 Errata status:
 *
 *	- 504326: Prefetching can be done on wrong address or wrong memory area.
 * 		Status:
 * 			No need for the fix in Kernel as it doesn't do prefetching.
 * 			When PL310 is configured in bootstrap, baseport should keep the Instruction
 * 			and Data Prefetch features disabled, as they are by default.
 * 
 * 	- 539766: Non Secure Invalidate by Way maintenance operation does not work properly.
 * 		Description:
 * 			Invalidate by Way in non-secure world may corrupt secure cache lines.
 * 		Status:
 *			Unconditionaly fixed. Kernal maintains only a single way in one go. 
 * 			When intialising PL310, baseport should make sure to invalide only a single
 * 			way in one go.
 * 
 *	- 540921: Parity must be disabled in exclusive cache configuration.
 * 		Status:
 * 			No need for the fix in Kernel as PL310 is initialised by baseport.
 * 			When intialising PL310, baseport should make sure not to enable both 
 * 			parity and exclusive mode.
 * 
 *  - 588369: Clean&Invalidate maintenance operations do not invalidate clean lines.
 * 		Status:
 *			Fix implemented in Kernel when 
 *				__ARM_PL310_ERRATUM_588369_FIXED
 *			is not defined in variant.mmh
 * 			CleanAndInvalidateByPA is replaced by CleanByPA and InvalidateByPA.
 * 			CleanAndInvalidateByIndexWay is made sure never to happen.
 * 			CleanAndInvalidateByWay is made sure never to happen.
 * 			Coherancy problem mentioned in the workaround is not relevant. 
 * 
 *	- 588371: Potential data corruption when an AXI locked access is received while a
 *	  		  prefetch is treated.
 * 		Status:
 * 			No need for the fix in Kernel as PL310 SWP instruction is depricated since ARMv6.
 * 
 *	- 501023: Address Filtering register content cannot be read.
 * 		Status:
 * 			No need for the fix in Kernel it doesn't access access filtering registers.
 * 
 *	- 502117: Address Filtering registers may not have appropriate values out of reset.
 * 		Status:
 * 			No need for the fix in Kernel it doesn't access access filtering registers.
 * 
 *	- 588375: Potential deadlock when evictions or store buffer writes are issued by
 * 			  different master ports.
 * 		Status:
 * 			Not fixed in Kernel as there is no software workaround for this erratum.
 * 
 *  - 727915: Background Clean & Invalidate by Way operation can cause data corruption
 *      Status:
 *          There was no need to fix anything as PL310 cache maintenance doesn't use any 
 *          _Maintain_ByWay operation. (It is only used in ExternalCache::AtomicSync on
 *          L210 & L220.)
 */

#include <arm.h>
#include "cache_maintenance.h"
#include <nkern.h>

#ifdef __HAS_EXTERNAL_CACHE__

//L2 Cache globals
SCacheInfo ExternalCache::Info;
TLinAddr   ExternalCache::Base;
#if defined(__ARM_PL310_CACHE__)
#if defined(__SMP__)
TSpinLock   ExternalCache::iLock(TSpinLock::EOrderCacheMaintenance);
#else
// Any order will do here
TSpinLock   ExternalCache::iLock(TSpinLock::EOrderGenericIrqLow0);
#endif
#endif // defined(__ARM_PL310_CACHE__)


#if defined (__ARM_L220_CACHE__)
// These two macros that deal with disabling/restoring interrupts
// when L220 accesses controll register. They are defined here to
// improve readability of the code below.

#define L220_COMMAND_PREAMBLE                       \
    TInt irq = NKern::DisableAllInterrupts();       \
    while (*ctrlReg & 1) __chill();

#define L220_COMMAND_POSTAMBLE                      \
    NKern::RestoreInterrupts(irq);

#else  // defined (__ARM_L220_CACHE__)
#define L220_COMMAND_PREAMBLE
#define L220_COMMAND_POSTAMBLE
#endif // else defined (__ARM_L220_CACHE__)



#if defined(__ARM_PL310_CACHE__)
// These three macros that deal with pl310 spin lock are
// defined here to improve readability of the code below.
#define PL310_SPIN_LOCK                             \
    TInt lockCounter = KMaxCacheLinesPerSpinLock;   \
    TInt spinLockIrq = Lock();

#define PL310_SPIN_FLASH                            \
    if (!--lockCounter)                             \
        {                                           \
        lockCounter = KMaxCacheLinesPerSpinLock;    \
        FlashLock(spinLockIrq);                     \
        }

#define PL310_SPIN_UNLOCK                           \
    Unlock(spinLockIrq);
    
#else //defined(__ARM_PL310_CACHE__)
#define PL310_SPIN_LOCK
#define PL310_SPIN_FLASH
#define PL310_SPIN_UNLOCK
#endif // else defined(__ARM_PL310_CACHE__)



#define CACHEL2FAULT()	ExternalCacheFault(__LINE__)
void ExternalCacheFault(TInt aLine)
	{
	Kern::Fault("ExternalCacheFault",aLine);
	}

void ExternalCache::Init1()
	{
	TInt waySize = 0; // Number of bytes in a way

	Base = TheSuperPage().iArmL2CacheBase;
	TInt auxCtrl = *(TInt*)(Base+ARML2C_AuxiliaryControl);
	__KTRACE_OPT(KBOOT,Kern::Printf("ExternalCache::Init1 L2CacheBase=%x AuxCtrl reg=%x", Base, auxCtrl));

	//Calculate the number of ways
	switch((auxCtrl & ARML2C_WaySize_Mask) >> ARML2C_WaySize_Shift)
		{
		case 0:		waySize = 0x4000;  break;
		case 1:		waySize = 0x4000;  break;
		case 2:		waySize = 0x8000;  break;
		case 3:		waySize = 0x10000; break;
		case 4:		waySize = 0x20000; break;
#if defined (__ARM_L210_CACHE__) || defined (__ARM_L220_CACHE__)
		default:	waySize = 0x40000; break;
#elif defined(__ARM_PL310_CACHE__)
		case 5:		waySize = 0x40000; break;
		case 6:		waySize = 0x80000; break;
		default:	waySize = 0x80000; break;
#endif		
		}

#if defined (__ARM_L210_CACHE__) || defined (__ARM_L220_CACHE__)
	Info.iAssoc = (auxCtrl & ARML2C_Assoc_Mask)>> ARML2C_Assoc_Shift;
	if (Info.iAssoc > 8)
		Info.iAssoc = 8;
	
#elif defined(__ARM_PL310_CACHE__)
	Info.iAssoc = auxCtrl & ARML2C_Assoc_Mask ? 16 : 8;
#endif
		
	Info.iSize = (waySize * Info.iAssoc) >> 5;	// >>5 as iSize is counted in lines
	Info.iLineLength=32;						// It is always 32
	Info.iLineLenLog2=5;
	Info.iInvalidateThreshold=Info.iSize*8; //if invalidate region >=8*CacheSize, will flush entire cache
	Info.iCleanThreshold=Info.iSize*4; 		//if clean region >=4*CacheSize, will clean entire cache
	Info.iCleanAndInvalidateThreshold=Info.iSize*4; //if flush region >=4*CacheSize, will flush entire cache

	#ifdef _DEBUG
	SCacheInfo& c=Info;
	__KTRACE_OPT(KBOOT,Kern::Printf("External Cache:"));
	__KTRACE_OPT(KBOOT,Kern::Printf("Size %04x LineLength %04x Assoc %04x",c.iSize,c.iLineLength,c.iAssoc));
	__KTRACE_OPT(KBOOT,Kern::Printf("InvalidateThreshold %04x CleanThreshold %04x CleanAndInvalidateThreshold %04x",c.iInvalidateThreshold,c.iCleanThreshold,c.iCleanAndInvalidateThreshold));
	__KTRACE_OPT(KBOOT,Kern::Printf("LineLenLog2 %02x PreemptBlock %02x",c.iLineLenLog2,c.iPreemptBlock));
	#endif
	}

void ExternalCache::Clean(TLinAddr aBase, TUint aSize)
	{
	__KTRACE_OPT(KMMU, Kern::Printf("ExternalCache::Clean base=%xH, size=%xH", aBase, aSize));
	if (aSize>=Info.CleanThresholdBytes())
		Maintain_All((TInt*)(Base+ARML2C_CleanByIndexWay));
	else
		Maintain_Region(aBase, aSize, (TInt*)(Base+ARML2C_CleanLineByPA));

	DrainBuffers();
	}

void ExternalCache::Invalidate(TLinAddr aBase, TUint aSize)
	{
	__KTRACE_OPT(KMMU,Kern::Printf("ExternalCache::Purge base=%xH, size=%xH", aBase, aSize));

#if defined(__ARM_PL310_CACHE__) && !defined(__ARM_PL310_ERRATUM_588369_FIXED)
		//Cannot Clean&Invalidate all cache, so do not bother checking the threshold
		Maintain_Region(aBase, aSize, (TInt*)(Base+ARML2C_InvalidateLineByPA));
#else // defined(__ARM_PL310_CACHE__) && !defined(__ARM_PL310_ERRATUM_588369_FIXED)	
	
	if (aSize>=Info.InvalidateThresholdBytes())
		{
		Maintain_All((TInt*)(Base+ARML2C_CleanInvalidateByIndexWay));
		DrainBuffers();
		}
	else
		{
		Maintain_Region(aBase, aSize, (TInt*)(Base+ARML2C_InvalidateLineByPA));
		}
#endif //else defined(__ARM_PL310_CACHE__) && !defined(__ARM_PL310_ERRATUM_588369_FIXED)
	}

void ExternalCache::CleanAndInvalidate(TLinAddr aBase, TUint aSize)
	{
	__KTRACE_OPT(KMMU,Kern::Printf("ExternalCache::CleanAndInvalidate base=%xH, size=%xH", aBase, aSize));
#if defined(__ARM_PL310_CACHE__) && !defined(__ARM_PL310_ERRATUM_588369_FIXED)
    //Cannot Clean&Invalidate all cache, so do not bother checking the threshold
    Maintain_Region(aBase, aSize, (TInt*)(Base+ARML2C_CleanInvalidateLineByPA));
#else //defined(__ARM_PL310_CACHE__) && !defined(__ARM_PL310_ERRATUM_588369_FIXED)

	if (aSize>=Info.CleanAndInvalidateThresholdBytes())
		Maintain_All((TInt*)(Base+ARML2C_CleanInvalidateByIndexWay));
	else
		Maintain_Region(aBase, aSize, (TInt*)(Base+ARML2C_CleanInvalidateLineByPA));
#endif
	DrainBuffers();
	}

void ExternalCache::AtomicSync()
	{
	// This methos is called during reboot or power down sequence and therefore is not allowed
	// to do Kernel calls that may hold spin lock - such as Kern::Print or precondition checkings.
	// CHECK_PRECONDITIONS(MASK_INTERRUPTS_DISABLED,"ExternalCache::AtomicSync");
	// __KTRACE_OPT(KMMU,Kern::Printf("ExternalCache::AtomicSync"));
#if defined(__ARM_PL310_CACHE__)
    // Do not use maintain-by-way operations on PL310 (due to erratum 727915 for example)
    // Also, do not use ARML2C_CleanInvalidateByIndexWay (erratum 588369)
    // Do not hold spin lock as it is assumed this is the only running CPU.
    TInt indexNo = Info.iSize>>3; // This is the number of cache lines in each way. Assoc is always 8 in this cache
    volatile TInt* ctrlReg = (volatile TInt*)(Base+ARML2C_CleanByIndexWay);
    TInt way,index;
    for (way = 0 ; way <Info.iAssoc ; way++)
        {
        for (index = 0 ; index <indexNo ; index++)
            {
            *ctrlReg = (way<<ARML2C_WayShift) | (index<<ARML2C_IndexShift); //This will maintain a single cache line
            }
        }
#else //defined(__ARM_PL310_CACHE__)

	#if defined (__ARM_L210_CACHE__)
	// Flush buffers before starting background clean+invalidate.
	// This is the fix for an erratum (see the top of the file).
	DrainBuffers();
	#endif

#if defined (__ARM_L220_CACHE__)
	{
	// Loop if there is ongoing background operation.
	volatile TInt* cacheSync = (volatile TInt*)(Base+ARML2C_CacheSync);
	while (*cacheSync & 1) __chill();
	}
#endif	
	
	int wayShift;
	// Clean and invalidate cache way-after-the-way. Maintenance of multiple ways at
	// the same time is the subject of numerous errata.
	for (wayShift=1; wayShift < 0x100; wayShift<<=1)//8 loops (for 8 ways), with wayShift values 1,2,4,...,0x80
		{
		volatile TInt* ctrlReg = (volatile TInt*)(Base+ARML2C_CleanInvalidateByWay);
		*ctrlReg = wayShift;
		while(*ctrlReg) __chill();
		}

	DrainBuffers();
#endif
	return;
	}

void ExternalCache::InvalidatePhysicalMemory(TPhysAddr aAddr, TUint aSize)
	{
	if (aSize == 0)
		return;
	
	volatile TInt* ctrlReg = (volatile TInt*)(Base+ARML2C_InvalidateLineByPA);
	TUint32 endAddr = aAddr+aSize; 	// (Exclusive)
	if (endAddr<aAddr)
	    endAddr = 0xffffffff; 

	aAddr &= ~(Info.iLineLength-1); // Align starting address to line length.

    PL310_SPIN_LOCK;
lineLoop:
    L220_COMMAND_PREAMBLE;
	*ctrlReg = aAddr;	//This will invalidate the line from the external cache
	L220_COMMAND_POSTAMBLE;
	PL310_SPIN_FLASH;
	
	aAddr+=Info.iLineLength;
	if (aAddr<endAddr) goto lineLoop;

	PL310_SPIN_UNLOCK;
	}

void ExternalCache::CleanPhysicalMemory(TPhysAddr aAddr, TUint aSize)
	{
	if (aSize == 0)
		return;
	
	volatile TInt* ctrlReg = (volatile TInt*)(Base+ARML2C_CleanLineByPA);
	TUint32 endAddr = aAddr+aSize; 	// (Exclusive)
    if (endAddr<aAddr)
        endAddr = 0xffffffff; 
	aAddr &= ~(Info.iLineLength-1); // Align starting address to line length.

    PL310_SPIN_LOCK;
lineLoop:
    L220_COMMAND_PREAMBLE;
	*ctrlReg = aAddr;	//This will clean the line from the external cache
    L220_COMMAND_POSTAMBLE;
    PL310_SPIN_FLASH;
	
	aAddr+=Info.iLineLength;
	if (aAddr<endAddr) goto lineLoop;
	
    PL310_SPIN_UNLOCK;
	DrainBuffers();
	}

void ExternalCache::CleanAndInvalidatePhysicalMemory(TPhysAddr aAddr, TUint aSize)
	{
	if (aSize == 0)
		return;
	
	TUint32 endAddr = aAddr+aSize; 	// (Exclusive)
    if (endAddr<aAddr)
        endAddr = 0xffffffff; 
	aAddr &= ~(Info.iLineLength-1); // Align starting address to line length.

    PL310_SPIN_LOCK;

#if defined(__ARM_PL310_CACHE__) && !defined(__ARM_PL310_ERRATUM_588369_FIXED)
    volatile TInt* cleanReg = (volatile TInt*)(Base+ARML2C_CleanLineByPA);
	volatile TInt* invalidateReg = (volatile TInt*)(Base+ARML2C_InvalidateLineByPA);

lineLoop:
    TInt ret = NKern::DisableAllInterrupts();
	*cleanReg = aAddr;
	*invalidateReg = aAddr;
    NKern::RestoreInterrupts(ret);

#else // #idefined(__ARM_PL310_CACHE__) && !defined(__ARM_PL310_ERRATUM_588369_FIXED)

	volatile TInt* ctrlReg = (volatile TInt*)(Base+ARML2C_CleanInvalidateLineByPA);
	
lineLoop:
    L220_COMMAND_PREAMBLE;
	*ctrlReg = aAddr;	//This will clean&invalidate the line from the external cache
    L220_COMMAND_POSTAMBLE;
#endif //else defined(__ARM_PL310_CACHE__) && !defined(__ARM_PL310_ERRATUM_588369_FIXED)

    PL310_SPIN_FLASH;
	
	aAddr+=Info.iLineLength;
	if (aAddr<endAddr) goto lineLoop;
	
    PL310_SPIN_UNLOCK;
	DrainBuffers();
	}

void ExternalCache::Maintain_Region(TLinAddr aBase, TUint aSize, TInt* aCtrlReg)
	{
	__KTRACE_OPT(KMMU,Kern::Printf("ExternalCache::Maintain_Region %08xH+%08xH, reg=%xH", aBase, aSize, aCtrlReg));

	if (aSize == 0)
		return;
	
	volatile TInt* ctrlReg = aCtrlReg;
	TInt lineLength = Info.iLineLength;
	//Round down aBase to line length and recalculate aSize accordingly.
	TInt diff = aBase & (lineLength-1);
	aBase -= diff;
	//Ensure size doesn't overflow
	if ( ((TInt)(aSize)<0) && (((TInt)(aSize)+diff)>0) )
		aSize = 0xffffffff;
	else
		aSize += diff;

	// Find the starting physical address. If VA is invalid, try the next page.
findStartingPA:	
	TPhysAddr physAddress = Epoc::LinearToPhysical(aBase);
	if (physAddress == KPhysAddrInvalid)
		{
		__KTRACE_OPT(KMMU,Kern::Printf("ExternalCache::Maintain_Region invalid VA:%xH", aBase));
		diff = KPageSize - (aBase&KPageMask);
		aBase+=diff;
		if ((TInt)aSize>0 && ((TInt)aSize<=diff))
		    return;
		aSize-=diff;
		goto findStartingPA;
		}

	__KTRACE_OPT(KMMU,Kern::Printf("ExternalCache::Maintain_Region VA:%xH > PA:%xH", aBase, physAddress));
	aBase &= ~KPageMask; //The rest of the function expects aBase to be page aligned.

    PL310_SPIN_LOCK;
lineLoop:

#if defined(__ARM_PL310_CACHE__) && !defined(__ARM_PL310_ERRATUM_588369_FIXED)
    if((TInt)aCtrlReg == Base+ARML2C_CleanInvalidateLineByPA)
        {
        // CleanInvalidateLineByPA is broken
        volatile TInt* cleanReg = (volatile TInt*)(Base+ARML2C_CleanLineByPA);
        volatile TInt* invalidateReg = (volatile TInt*)(Base+ARML2C_InvalidateLineByPA);
        TInt ret = NKern::DisableAllInterrupts();
        *cleanReg = physAddress;
        *invalidateReg = physAddress;
        NKern::RestoreInterrupts(ret);
        }
    else
        *ctrlReg = physAddress; //This will clean, purge or flush the line
        
#else
    L220_COMMAND_PREAMBLE;
    *ctrlReg = physAddress;	//This will clean, purge or flush the line
    L220_COMMAND_POSTAMBLE;
#endif
    PL310_SPIN_FLASH;
	
	if ((TInt)aSize>0 && ((TInt)aSize<=lineLength))
		goto endOfRegion;

	aSize=aSize-lineLength;
	physAddress+=lineLength;

	if ((physAddress & KPageMask))
		goto lineLoop;
	
	//Drop here when the end of the page is reached.
endOfPage:	
	aBase += KPageSize;
	physAddress = Epoc::LinearToPhysical(aBase);
	if (physAddress == KPhysAddrInvalid)
		{
		__KTRACE_OPT(KMMU,Kern::Printf("ExternalCache::Maintain_Region Invalid VA %xH", aBase));
		if ((TInt)aSize>0 && ((TInt)aSize<=KPageSize))
			goto endOfRegion;
		aSize-=KPageSize;
		goto endOfPage;
		}		
	__KTRACE_OPT(KMMU,Kern::Printf("ExternalCache::Maintain_Region VA:%xH > PA:%xH", aBase, physAddress));
	goto lineLoop;

endOfRegion:
    PL310_SPIN_UNLOCK;
	return;
	}

void ExternalCache::Maintain_All(TInt* aCtrlReg)
	{
	
	__KTRACE_OPT(KMMU,Kern::Printf("ExternalCache::Maintain_All %xH", aCtrlReg));

#if defined(__ARM_PL310_CACHE__) && !defined(__ARM_PL310_ERRATUM_588369_FIXED)
	//CleanAndInvalidateByIndexWay is broken
	__ASSERT_DEBUG((TInt)aCtrlReg != Base+ARML2C_CleanInvalidateByIndexWay, CACHEL2FAULT());
#endif
	
	TInt indexNo = Info.iSize>>3; // This is the number of cache lines in each way. Assoc is always 8 in this cache
	volatile TInt* ctrlReg = aCtrlReg;

    PL310_SPIN_LOCK;
	TInt way,index;
	for (way = 0 ; way <Info.iAssoc ; way++)
		{
		for (index = 0 ; index <indexNo ; index++)
			{
		    L220_COMMAND_PREAMBLE;
		    *ctrlReg = (way<<ARML2C_WayShift) | (index<<ARML2C_IndexShift); //this will maintain a single cache line
            L220_COMMAND_POSTAMBLE;
			PL310_SPIN_FLASH;
			}
		}
    PL310_SPIN_UNLOCK;
	}

#if defined(__ARM_PL310_CACHE__)
TInt ExternalCache::Lock()
	{
	return __SPIN_LOCK_IRQSAVE(iLock);
	}

void ExternalCache::FlashLock(TInt aIrq)
	{
	__SPIN_FLASH_IRQRESTORE(iLock, aIrq);
	}
	
void ExternalCache::Unlock(TInt aIrq)
	{
	__SPIN_UNLOCK_IRQRESTORE(iLock, aIrq);
	}
#endif // defined(__ARM_PL310_CACHE__)

#endif // defined (__HAS_EXTERNAL_CACHE__)
