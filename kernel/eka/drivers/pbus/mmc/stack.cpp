// Copyright (c) 1999-2010 Nokia Corporation and/or its subsidiary(-ies).
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
// \e32\drivers\pbus\mmc\stack.cpp
// 
//

#include <drivers/mmc.h>
#include <kernel/kern_priv.h>
#include <drivers/locmedia.h>
#include "stackbody.h"

#include "OstTraceDefinitions.h"
#ifdef OST_TRACE_COMPILER_IN_USE
#include "../../../include/drivers/locmedia_ost.h"
#ifdef __VC32__
#pragma warning(disable: 4127) // disabling warning "conditional expression is constant"
#endif
#include "stackTraces.h"
#endif

#ifdef __SMP__
TSpinLock MMCLock(TSpinLock::EOrderGenericIrqHigh0);
#endif

#define ASSERT_NOT_ISR_CONTEXT	__ASSERT_DEBUG(NKern::CurrentContext()!=NKern::EInterrupt,DMMCSocket::Panic(DMMCSocket::EMMCUnblockingInWrongContext));

#if !defined(__WINS__)
#define DISABLEPREEMPTION TUint irq = __SPIN_LOCK_IRQSAVE(MMCLock);
#define RESTOREPREEMPTION __SPIN_UNLOCK_IRQRESTORE(MMCLock,irq);
#else
#define DISABLEPREEMPTION
#define RESTOREPREEMPTION										   
#endif

//#define ENABLE_DETAILED_SD_COMMAND_TRACE

// default length of minor buffer - must have at least enough space for one sector
const TInt KMinMinorBufSize = 512;

//	MultiMedia Card Controller - Generic level code for controller, intermediate
//	level code for media change and power supply handling

EXPORT_C TUint TCSD::CSDField(const TUint& aTopBit, const TUint& aBottomBit) const
/**
 * Extract bitfield from CSD
 */
	{
	const TUint indexT=KMMCCSDLength-1-aTopBit/8;
	const TUint indexB=KMMCCSDLength-1-aBottomBit/8;
	return(((indexT==indexB ? iData[indexT]
			: (indexT+1)==indexB ? ((iData[indexT]<<8) | iData[indexT+1])
			: ((iData[indexT]<<16) | (iData[indexT+1]<<8) | iData[indexT+2])
			) >> (aBottomBit&7)) & ((1<<(aTopBit-aBottomBit+1))-1));
	}


//	--------  class TCSD  --------
// Raw field accessor functions are defined in mmc.inl.  These functions return
// values that require extra computation, such as memory capacity.


EXPORT_C TUint TCSD::DeviceSize() const
/**
 *
 * Calculate device capacity from CSD
 * 
 * Section 5.3, MMCA Spec 2.2 (Jan 2000)
 *
 * memory capacity = BLOCKNR * BLOCK_LEN
 * where
 *	BLOCKNR = (C_SIZE + 1) * MULT; MULT = 2 ** (C_MULT_SIZE + 2);
 *	BLOCK_LEN = 2 ** (READ_BL_LEN)
 *
 * memory capacity	= (C_SIZE + 1) * (2 ** (C_MULT_SIZE + 2)) * (2 ** READ_BL_LEN)
 *					= (C_SIZE + 1) * (2 ** (C_MULT_SIZE + 2 + READ_BL_LEN))
 *
 * @return Device Capacity
 */
	{
	__KTRACE_OPT(KPBUS1, Kern::Printf("csd:ds:0x%x,0x%x,0x%x", ReadBlLen(), CSize(), CSizeMult()));

	const TUint blockLog = ReadBlLen();
	if( blockLog > 11 )
		return( 0 );

	const TUint size = (CSize() + 1) << (2 + CSizeMult() + blockLog);

	if( size == 0 )
		return( 0xFFF00000 );

	return( size );
	}

EXPORT_C TMMCMediaTypeEnum TCSD::MediaType() const
/**
 * This function makes a rough approximation if media type based on supported
 * command classes (CCC).
 *
 * @return TMMCMediaTypeEnum describing the type of media.
 */
	{
	struct mediaTableEntry
		{
		TUint iMask;
		TUint iValue;
		TMMCMediaTypeEnum iMedia;
		};

	const TUint testMask = (KMMCCmdClassBlockRead|KMMCCmdClassBlockWrite|KMMCCmdClassErase|KMMCCmdClassIOMode);
	static const mediaTableEntry mediaTable[] = 
		{
		{KMMCCmdClassBasic, 0,																  EMultiMediaNotSupported},
		{testMask,			(KMMCCmdClassBlockRead|KMMCCmdClassBlockWrite|KMMCCmdClassErase), EMultiMediaFlash},
		{testMask,			KMMCCmdClassBlockRead|KMMCCmdClassBlockWrite,					  EMultiMediaFlash},
		{testMask,			KMMCCmdClassBlockRead|KMMCCmdClassErase,						  EMultiMediaROM},
		{testMask,			KMMCCmdClassBlockRead,											  EMultiMediaROM},
		{KMMCCmdClassIOMode,KMMCCmdClassIOMode,												  EMultiMediaIO},
		{0,					0,																  EMultiMediaOther}
		};

	const TUint ccc = CCC();
	const mediaTableEntry* ptr = mediaTable;

	while( (ccc & ptr->iMask) != (ptr->iValue) )
		ptr++;

    if (ptr->iMedia == EMultiMediaFlash)
        {
        // Further check PERM_WRITE_PROTECT and TMP_WRITE_PROTECT bits
        if (PermWriteProtect() || TmpWriteProtect())
            return EMultiMediaROM;
        }

	return( ptr->iMedia );
	}

EXPORT_C TUint TCSD::ReadBlockLength() const
/**
 * Calculates the read block length from the CSD.
 * READ_BL_LEN is encoded as a logarithm.
 *
 * @return The read block length 
 */
	{
	const TUint blockLog = ReadBlLen();
	
	//SD version 2.0 or less the range is 0-11
	//MMC version 4.1 or less the range is 0-11
	//MMC version 4.2 the range is 0-14 (15 is reserved for future use)
	//But we cannot differentiate among 4.x
	//Hence , 0-14 is supported for 4.x
	if (SpecVers() < 4)
		{
		if( blockLog > 11 )
			return( 0 );
		}
	else
		{
		if(blockLog > 14)
			return ( 0 );
		}

	return( 1 << blockLog );
	}

EXPORT_C TUint TCSD::WriteBlockLength() const
/**
 * Calculates the write block length from the CSD.
 * WRITE_BL_LEN is encoded as a logarithm.
 *
 * @return The write block length 
 */
	{
	const TUint blockLog = WriteBlLen();
	if( blockLog > 11 )
		return( 0 );

	return( 1 << blockLog );
	}

EXPORT_C TUint TCSD::EraseSectorSize() const
/**
 * Calculates the erase sector size from the CSD.
 * SECTOR_SIZE is a 5 bit value, which is one less than the number of write
 *
 * @return The erase sector size
 */
	{
	if (SpecVers() < 3)
		{
		// V2.2 and earlier supports erase sectors. Read sector size from CSD(46:42) - confusingly now reassigned as
		// erase group size. 
		return( (EraseGrpSize()+1) * WriteBlockLength() );
		}
	else
		{
		// Support for erase sectors removed from V3.1 onwards
		return(0);	
		}

	}

EXPORT_C TUint TCSD::EraseGroupSize() const
/**
 * Calculates the erase group size from the CSD.
 * ERASE_GRP_SIZE is a 5 bit value, which is one less than the number of erase
 * sectors in an erase group.
 *
 * @return The erase group size
 */
	{
	if (SpecVers() < 3)
		{
		// For V2.2 and earlier, the erase group size is held in CSD(41:37)  - confusingly now reassigned as the erase
		// group multiplier. The units for this are erase sectors, so need to convert to write blocks and then bytes.
		TUint erSecSizeInBytes=(EraseGrpSize()+1) * WriteBlockLength();
		return( (EraseGrpMult()+1) * erSecSizeInBytes );
		}
	else
		{
		// For V3.1 onwards, the erase group size is determined by multiplying the erase group size - CSD(41:37) by the
		// erase group multiplier - CSD(46:42)). The units for this are write blocks, so need to convert to bytes.  
		TUint erGrpSizeInWrBlk = (EraseGrpSize()+1) * (EraseGrpMult()+1);
		return(erGrpSizeInWrBlk * WriteBlockLength());
		}	
	}

EXPORT_C TUint TCSD::MinReadCurrentInMilliamps() const
/**
 * Calculates the minimum read current from the CSD.
 * VDD_R_CURR_MIN is a three bit value which is mapped to a number of mA.
 * 0 actually maps to 0.5mA, but has been rounded up.
 *
 * @return The minimum read current, in Milliamps
 */
	{
	static const TUint8 minConsumptionTable[] = {1,1,5,10,25,35,60,100};
	return( minConsumptionTable[VDDRCurrMin()] );
	}

EXPORT_C TUint TCSD::MinWriteCurrentInMilliamps() const
/**
 * Calculates the minimum write current from the CSD.
 * VDD_W_CURR_MIN is a three bit value which is mapped to a number of mA.
 *
 * @return The minimum write current, in Milliamps
 */
	{
	static const TUint8 minConsumptionTable[] = {1,1,5,10,25,35,60,100};
	return( minConsumptionTable[VDDWCurrMin()] );
	}

EXPORT_C TUint TCSD::MaxReadCurrentInMilliamps() const
/**
 * Calculates the maximum read current from the CSD.
 * VDD_R_CURR_MAX is a three bit value which is mapped to a number of mA.
 * 0 actually maps to 0.5mA, but has been rounded up.
 *
 * @return The maximum read current, in Milliamps
 */
	{
	static const TUint8 maxConsumptionTable[] = {1,5,10,25,35,45,80,200};
	return( maxConsumptionTable[VDDRCurrMax()] );
	}

EXPORT_C TUint TCSD::MaxWriteCurrentInMilliamps() const
/**
 * Calculates the maximum write current from the CSD.
 * VDD_W_CURR_MAX is a three bit value which is mapped to a number of mA.
 *
 * @return The maximum write current, in Milliamps
 */
	{
	static const TUint8 maxConsumptionTable[] = {1,5,10,25,35,45,80,200};
	return( maxConsumptionTable[VDDWCurrMax()] );
	}

EXPORT_C TUint TCSD::MaxTranSpeedInKilohertz() const
/**
 * TRAN_SPEED is an eight bit value which encodes three fields.
 * Section 5.3, MMCA Spec 2.2 (Jan 2000)
 *
 *	2:0	transfer rate unit	values 4 to 7 are reserved.
 *	6:3	time value
 *
 * @return Speed, in Kilohertz
 */
	{
	// tranRateUnits entries are all divided by ten so tranRateValues can be integers
	static const TUint tranRateUnits[8] = {10,100,1000,10000,10,10,10,10};
	static const TUint8 tranRateValues[16] = {10,10,12,13,15,20,25,30,35,40,45,50,55,60,70,80};
	const TUint ts = TranSpeed();
	return( tranRateUnits[ts&7] * tranRateValues[(ts>>3)&0xF] );
	}

//	--------  class TMMCard  --------

TMMCard::TMMCard()
: iIndex(0), iUsingSessionP(0), iFlags(0), iBusWidth(1)
	{
	// empty.
	}

EXPORT_C TBool TMMCard::IsReady() const
/**
 * Predicate for if card is mounted and in standby/transfer/sleep state.
 *
 * @return ETrue if ready, EFalse otherwise.
 */
	{
	const TUint state = iStatus.State();
	__KTRACE_OPT(KPBUS1, Kern::Printf("=mcc:ir:%d,0x%08x", IsPresent(), state));
	OstTraceExt2( TRACE_INTERNALS, TMMCARD_ISREADY, "IsPresent=%d; state=0x%08x", IsPresent(), state );
	
	return IsPresent() && (state == ECardStateStby || state == ECardStateTran || state == ECardStateSlp);
	}

EXPORT_C TBool TMMCard::IsLocked() const
/**
 * Predicate for if card is locked
 * 
 * It would be useful to check if the CSD supports the password protection
 * feature.  Password protection was introduced in c3.1, 05/99 and SPEC_VERS
 * is encoded 0 |-> 1.0 - 1.2, 1 |-> 1.4, 3 |-> 2.2.  Some cards support
 * password locking but their CSD reports SPEC_VERS == 1.
 *
 * @return ETrue if locked, EFalse otherwise.
 */
	{
	OstTraceFunctionEntry1( TMMCARD_ISLOCKED_ENTRY, this );
	if ( !IsPresent() ) 
		return( EFalse );

	return( (TUint32(iStatus) & KMMCStatCardIsLocked) != 0 );
	}

TInt64 TMMCard::DeviceSize64() const
/**
 * Returns the size of the MMC card in bytes
 * @return The size of the MMC card in bytes.
 */
	{
	OstTraceFunctionEntry1( TMMCARD_DEVICESIZE64_ENTRY, this );
	const TBool highCapacity = IsHighCapacity();
	const TUint32 sectorCount = ExtendedCSD().SectorCount();
	
	return ((highCapacity && sectorCount) ? (((TInt64)ExtendedCSD().SectorCount()) * 512) : (TInt64)CSD().DeviceSize());
	}

TUint32 TMMCard::PreferredWriteGroupLength() const
/**
 * Returns the write group length.  Provided by the variant.
 * Default implementation returns a multiple of the write block length, as indicated by the CSD.
 * @return The preferred write group length.
 */
	{
	OstTraceFunctionEntry1( TMMCARD_PREFERREDWRITEGROUPLENGTH_ENTRY, this );
	return(CSD().WriteBlockLength() << 5);	// 16K for a standard 512byte block length
	}

TInt TMMCard::GetFormatInfo(TLDFormatInfo& /*aFormatInfo*/) const
/**
 * Returns the preferred format parametersm for the partition.
 * Implemented at the Variant layer.
 * @return Standard Symbian OS error code.
 */
	{
	return KErrNotSupported;
	}

TUint32 TMMCard::MinEraseSectorSize() const
/**
 * Returns the minimum erase sector size.  Provided by the variant.
 * Default implementation returns the erase sector size, as indicated by the CSD.
 * @return The minimum erase sector size.
 */
	{
	return CSD().EraseSectorSize();
	}

TUint32 TMMCard::EraseSectorSize() const
/**
 * Returns the recommended erase sector size.  Provided by the variant.
 * Default implementation returns the erase sector size, as indicated by the CSD.
 * @return The recommended erase sector size.
 */
	{
	return CSD().EraseSectorSize();
	}

LOCAL_C TBool IsPowerOfTwo(TInt aNum)
//
// Returns ETrue if aNum is a power of two
//
	{
	return (aNum != 0 && (aNum & -aNum) == aNum);
	}
	
TInt TMMCard::GetEraseInfo(TMMCEraseInfo& aEraseInfo) const
/**
 * Return info. on erase services for this card
 * @param aEraseInfo A reference to the TMMCEraseInfo to be filled in with the erase information.
 * @return Symbian OS error code.
 */
	{
	OstTraceFunctionEntry1( TMMCARD_GETERASEINFO_ENTRY, this );
	
	// Check whether this card supports Erase Class Commands. Also, validate the erase group size
	if ((CSD().CCC() & KMMCCmdClassErase) && IsPowerOfTwo(CSD().EraseGroupSize()))
		{
		// This card supports erase cmds. Also, all versions of MMC cards support Erase Group commands (i.e. CMD35, CMD36).
		OstTrace0( TRACE_INTERNALS, TMMCARD_GETERASEINFO, "Card supports erase class commands" );
		aEraseInfo.iEraseFlags=(KMMCEraseClassCmdsSupported|KMMCEraseGroupCmdsSupported); 
		
		// Return the preferred size to be used as the unit for format operations. We need to return a sensible
		// multiple of the erase group size - as calculated by the CSD. A value around 1/32th of the total disk
		// size generally results in an appropriate number of individual format calls.
		const TInt64 devSizeDividedBy32=(DeviceSize64()>>5);
		aEraseInfo.iPreferredEraseUnitSize=CSD().EraseGroupSize();
		while (aEraseInfo.iPreferredEraseUnitSize < devSizeDividedBy32)
			aEraseInfo.iPreferredEraseUnitSize<<=1;
	
		// Return the smallest size that can be used as the unit for erase operations. For erase group commands, this
		// is the erase group size. 
		aEraseInfo.iMinEraseSectorSize=CSD().EraseGroupSize();
		} 
	else	
		aEraseInfo.iEraseFlags=0;
	
	OstTraceFunctionExitExt( TMMCARD_GETERASEINFO_EXIT, this, KErrNone );
	return KErrNone;
	}

TUint TMMCard::MaxTranSpeedInKilohertz() const
/**
 * Returns the maximum supported clock rate for the card, in Kilohertz.
 * @return Speed, in Kilohertz
 */
	{
	OstTraceFunctionEntry1( TMMCARD_MAXTRANSPEEDINKILOHERTZ_ENTRY, this );
	// Default implementation obtains the transaction speed from the CSD
	TUint32 highSpeedClock = HighSpeedClock();
	return(highSpeedClock ? highSpeedClock : iCSD.MaxTranSpeedInKilohertz());
	}


TInt TMMCard::MaxReadBlLen() const
/**
 * Returns the maximum read block length supported by the card encoded as a logarithm
 * Normally this is the same as the READ_BL_LEN field in the CSD register,
 * but for high capacity cards (>- 2GB) this is set to a maximum of 512 bytes,
 * if possible, to try to avoid compatibility issues.
 */
	{
	OstTraceFunctionEntry1( TMMCARD_MAXREADBLLEN_ENTRY, this );
	const TInt KDefaultReadBlockLen = 9;	// 2^9 = 512 bytes
	const TCSD& csd = CSD();

	TInt blkLenLog2 = csd.ReadBlLen();

	if (blkLenLog2 > KDefaultReadBlockLen)
		{
		__KTRACE_OPT(KPBUS1, Kern::Printf("=mmc:mrbl %d", blkLenLog2));
		OstTrace1( TRACE_INTERNALS, TMMCARD_MAXREADBLLEN1, "Block length 1=%d", blkLenLog2 );
		
		
		if (csd.ReadBlPartial() || CSD().SpecVers() >= 4)
			{
			//
			// MMC System Spec 4.2 states that 512 bytes blocks are always supported, 
			// regardless of the state of READ_BL_PARTIAL 
			//
			blkLenLog2 = KDefaultReadBlockLen;	
			__KTRACE_OPT(KPBUS1, Kern::Printf("=mmc:mrbl -> %d", blkLenLog2));
			OstTrace1( TRACE_INTERNALS, TMMCARD_MAXREADBLLEN2, "Block length 2=%d", blkLenLog2 );
			}
		}

	OstTraceFunctionExitExt( TMMCARD_MAXREADBLLEN_EXIT, this, blkLenLog2 );
	return blkLenLog2;

	}

TInt TMMCard::MaxWriteBlLen() const
/**
 * Returns the maximum write block length supported by the card encoded as a logarithm
 * Normally this is the same as the WRITE_BL_LEN field in the CSD register,
 * but for high capacity cards (>- 2GB) this is set to a maximum of 512 bytes,
 * if possible, to try to avoid compatibility issues.
 */
	{
	OstTraceFunctionEntry1( TMMCARD_MAXWRITEBLLEN_ENTRY, this );
	const TInt KDefaultWriteBlockLen = 9;	// 2^9 = 512 bytes
	const TCSD& csd = CSD();

	TInt blkLenLog2 = csd.WriteBlLen();

	if (blkLenLog2 > KDefaultWriteBlockLen)
		{
		__KTRACE_OPT(KPBUS1, Kern::Printf("=mmc:mrbl %d", blkLenLog2));
		OstTrace1( TRACE_INTERNALS, TMMCARD_MAXWRITEBLLEN1, "Block length 1=%d", blkLenLog2 );
		if (csd.WriteBlPartial() || CSD().SpecVers() >= 4)
			{
			//
			// MMC System Spec 4.2 states that 512 bytes blocks are always supported, 
			// regardless of the state of READ_BL_PARTIAL 
			//
			blkLenLog2 = KDefaultWriteBlockLen;	
			__KTRACE_OPT(KPBUS1, Kern::Printf("=mmc:mrbl -> %d", blkLenLog2));
			OstTrace1( TRACE_INTERNALS, TMMCARD_MAXWRITEBLLEN2, "Block length 1=%d", blkLenLog2 );
			}
		}

	OstTraceFunctionExitExt( TMMCARD_MAXWRITEBLLEN_EXIT, this, blkLenLog2 );
	return blkLenLog2;

	}

//	--------  class TMMCardArray  --------

EXPORT_C TInt TMMCardArray::AllocCards()
/** 
 * Allocate TMMCard objects for iCards and iNewCardsArray.
 * This function is called at bootup as part of stack allocation so there
 * is no cleanup if it fails.
 *
 * @return KErrNone if successful, Standard Symbian OS error code otherwise.
 */
	{
	OstTraceFunctionEntry1( TMMCARDARRAY_ALLOCCARDS_ENTRY, this );
	for (TUint i = 0; i < KMaxMMCardsPerStack; ++i)
		{
		// zeroing the card data used to be implicit because embedded in
		// CBase-derived DMMCStack.
		if ((iCards[i] = new TMMCard) == 0)
		    {
			OstTraceFunctionExitExt( TMMCARDARRAY_ALLOCCARDS_EXIT1, this, KErrNoMemory );
			return KErrNoMemory;
		    }
		iCards[i]->iUsingSessionP = 0;
		if ((iNewCards[i] = new TMMCard) == 0)
		    {
			OstTraceFunctionExitExt( TMMCARDARRAY_ALLOCCARDS_EXIT2, this, KErrNoMemory );
			return KErrNoMemory;
		    }
		iNewCards[i]->iUsingSessionP = 0;
		}

	OstTraceFunctionExitExt( TMMCARDARRAY_ALLOCCARDS_EXIT3, this, KErrNone );
	return KErrNone;
	}

void TMMCardArray::InitNewCardScan()
/**
 * Prepare card array for new scan.
 */
	{
	OstTraceFunctionEntry1( TMMCARDARRAY_INITNEWCARDSCAN_ENTRY, this );
	iNewCardsCount=0;
	OstTraceFunctionExit1( TMMCARDARRAY_INITNEWCARDSCAN_EXIT, this );
	}

void TMMCardArray::MoveCardAndLockRCA(TMMCard& aSrcCard,TMMCard& aDestCard,TInt aDestIndex)
/**
 * Copy card object and lock RCA.
 */
	{
	OstTraceExt2(TRACE_FLOW, TMMCARDARRAY_MOVECARDANDLOCKRCA_ENTRY, "TMMCardArray::MoveCardAndLockRCA;aDestIndex=%d;this=%x", aDestIndex, (TUint) this);
	__KTRACE_OPT(KPBUS1, Kern::Printf("=mca:mclr:%d", aDestIndex));

	aDestCard.iCID=aSrcCard.iCID;
	aDestCard.iRCA=aSrcCard.iRCA;
	aDestCard.iCSD=aSrcCard.iCSD;
	aDestCard.iIndex=aDestIndex;		// Mark card as being present
	aDestCard.iFlags=aSrcCard.iFlags;
	aDestCard.iBusWidth=aSrcCard.iBusWidth;
	aDestCard.iHighSpeedClock = aSrcCard.iHighSpeedClock;

	iOwningStack->iRCAPool.LockRCA(aDestCard.iRCA);

	// Now that we have transferred ownership, reset the source card
	aSrcCard.iRCA = aSrcCard.iIndex = aSrcCard.iFlags = 0;
	aSrcCard.iBusWidth = 1;
	aSrcCard.iHighSpeedClock = 0;

	aSrcCard.iUsingSessionP = NULL;
	OstTraceFunctionExit1( TMMCARDARRAY_MOVECARDANDLOCKRCA_EXIT, this );
	}

EXPORT_C void TMMCardArray::AddNewCard(const TUint8* aCID,TRCA* aNewRCA)
/**
 * Found a new card to add to the array. Add it to a separate array for now
 * since we need to know all the cards present before we start replacing old
 * entries.
 */
	{
	OstTraceFunctionEntryExt( TMMCARDARRAY_ADDNEWCARD_ENTRY, this );
	// Store the CID in the next free slot
	NewCard(iNewCardsCount).iCID = aCID;

	*aNewRCA=0;

	// Now let's look if we've seen this card before
	for ( TUint i=0 ; i<iOwningStack->iMaxCardsInStack ; i++ )
		{
		if ( Card(i).iCID==NewCard(iNewCardsCount).iCID )
			{
			*aNewRCA=Card(i).iRCA;
			NewCard(iNewCardsCount).iIndex=(i+1);
			break;
			}
		}

	if ( *aNewRCA==0 )
		{
		// Not seen this one before so get a new RCA for the card
		NewCard(iNewCardsCount).iIndex=0;
		__ASSERT_ALWAYS( (*aNewRCA=iOwningStack->iRCAPool.GetFreeRCA())!=0,DMMCSocket::Panic(DMMCSocket::EMMCNoFreeRCA) );
		}

	__KTRACE_OPT(KPBUS1, Kern::Printf("mca:adn: assigning new card %d rca 0x%04x", iNewCardsCount, TUint16(*aNewRCA) ));
	OstTraceExt2( TRACE_INTERNALS, TMMCARDARRAY_ADDNEWCARD, "iNewCardsCount=%d; RCA=0x%04x", iNewCardsCount, (TUint) *aNewRCA );
	
	NewCard(iNewCardsCount).iRCA=*aNewRCA;
	iNewCardsCount++;
	OstTraceFunctionExit1( TMMCARDARRAY_ADDNEWCARD_EXIT, this );
	}

TInt TMMCardArray::MergeCards(TBool aFirstPass)
/**
 * This function places newly acquired cards from the new card array into free
 * slots of the main card array.
 * Returns KErrNotFound if not able to successfully place all the new cards.
 */
	{
	OstTraceFunctionEntryExt( TMMCARDARRAY_MERGECARDS_ENTRY, this );

	
	__KTRACE_OPT(KPBUS1,Kern::Printf("=mca:mc:%d,%d", aFirstPass, iNewCardsCount));
	OstTrace1( TRACE_INTERNALS, TMMCARDARRAY_MERGECARDS1, "iNewCardsCount=%d", iNewCardsCount );
	
	TUint i;	// New card index
	TUint j;	// Main card index

	// Only do this on first pass. Setup any new cards which were already there
	if (aFirstPass)
		{
		for ( i=0 ; i<iNewCardsCount ; i++ )
			{
			__KTRACE_OPT(KPBUS1, Kern::Printf("-mca:fp,i=%d,idx=0x%x", i, NewCard(i).iIndex));
			OstTraceExt2( TRACE_INTERNALS, TMMCARDARRAY_MERGECARDS2, "i=%d; Index=0x%x", i, NewCard(i).iIndex );
			if( NewCard(i).iIndex != 0 ) // Signifies card was here before (iIndex has old slot number +1)
				{
				// Put it in the same slot as before
				j=(NewCard(i).iIndex-1);
				MoveCardAndLockRCA(NewCard(i),Card(j),(j+1));
				}
			}
		}

	for ( i=0,j=0 ; i<iNewCardsCount ; i++ )
		{
		__KTRACE_OPT(KPBUS1, Kern::Printf("-mca:i=%d,j=%d,rca=0x%4x", i, j, TUint16(NewCard(i).iRCA) ));
		if ( NewCard(i).iRCA != 0 )
			{
			// Find a spare slot in main array for this new card
			while ( Card(j).IsPresent() )
				if ( ++j==iOwningStack->iMaxCardsInStack )
				    {
					OstTraceFunctionExitExt( TMMCARDARRAY_MERGECARDS_EXIT1, this, KErrNotFound );
					return KErrNotFound;
				    }

			// Found a free slot; move the card info there
			__KTRACE_OPT(KPBUS1, Kern::Printf("-mca:freej=%d,rca=0x%04x", j, TUint16(Card(j).iRCA) ));
			OstTraceExt2( TRACE_INTERNALS, TMMCARDARRAY_MERGECARDS3, "j=%d; RCA=0x%04x", j, (TUint) (Card(j).iRCA) );
			if ( Card(j).iRCA != 0 )
				iOwningStack->iRCAPool.UnlockRCA(Card(j).iRCA);

			__KTRACE_OPT(KPBUS1, Kern::Printf("merging new card %d to card %d dest index %d", i, j, j+1));
			OstTraceExt3( TRACE_INTERNALS, TMMCARDARRAY_MERGECARDS4, "Merging new card %d to card %d; Destination index=%d", (TInt) i, (TInt) j, (TInt) j+1 );
			MoveCardAndLockRCA(NewCard(i),Card(j),(j+1));
			}
		}
	OstTraceFunctionExitExt( TMMCARDARRAY_MERGECARDS_EXIT2, this, KErrNone );
	return KErrNone;
	}

void TMMCardArray::UpdateAcquisitions(TUint* aMaxClock)
/**
 * Called when we have successfully stored a new set of cards in the card array.
 * This performs final initialisation of the card entries and determines the
 * maximum bus clock that can be employed - by checking the CSD of each card.
 */
	{
	OstTraceFunctionEntryExt( TMMCARDARRAY_UPDATEACQUISITIONS_ENTRY, this );
	
	__KTRACE_OPT(KPBUS1,Kern::Printf("=mca:uda"));
	iCardsPresent=0;
	TUint maxClk = iOwningStack->iMultiplexedBus ? 1 : 800000; // ???
	for ( TUint i=0 ; i < (iOwningStack->iMaxCardsInStack) ; i++ )
		{
		if ( Card(i).IsPresent() )
			{
			// General initialisation
			iCardsPresent++;
			Card(i).iSetBlockLen=0;
			Card(i).iLastCommand=ECmdSendStatus;

			// Check each card present to determine appropriate bus clock
			TUint maxTS = iOwningStack->MaxTranSpeedInKilohertz(Card(i));
			if(iOwningStack->iMultiplexedBus)
				{
				if ( maxTS > maxClk )
					maxClk = maxTS;
				}
			else
				{
				if ( maxTS < maxClk )
					maxClk = maxTS;
				}
			}
		}
	// ??? Should also calculate here and return the data timeout and busy timeout 
	// instead of relying on ASSP defaults.

	*aMaxClock=maxClk;
	OstTraceFunctionExit1( TMMCARDARRAY_UPDATEACQUISITIONS_EXIT, this );
	}

EXPORT_C void TMMCardArray::DeclareCardAsGone(TUint aCardNumber)
/**
 * Clears up a card info object in the main card array
 */
	{
	OstTraceFunctionEntryExt( TMMCARDARRAY_DECLARECARDASGONE_ENTRY, this );
	
	__KTRACE_OPT(KPBUS1,Kern::Printf("=mca:dcag"));
	// If we thought this one was present then mark it as not present
	TMMCard& card = Card(aCardNumber);
	if (card.IsPresent())
		{
		card.iIndex=0; // Mark card as not present
		iCardsPresent--;
		}

	// If this card is in use by a session then flag that card has now gone
	if( card.iUsingSessionP != NULL )
		card.iUsingSessionP->iState |= KMMCSessStateCardIsGone;

	card.iUsingSessionP=NULL;
	card.iSetBlockLen=0;
	card.iFlags=0; 		// Reset 'has password' and 'write protected' bit fields
	card.iHighSpeedClock=0;
	card.iBusWidth=1;
	OstTraceFunctionExit1( TMMCARDARRAY_DECLARECARDASGONE_EXIT, this );
	}

// return this card's index in the array or KErrNotFound if not found
TInt TMMCardArray::CardIndex(const TMMCard* aCard)
	{
	OstTraceFunctionEntryExt( TMMCARDARRAY_CARDINDEX_ENTRY, this );
	TInt i;
	for (i = KMaxMMCardsPerStack-1; i>= 0; i--)
		{
		if (iCards[i] == aCard)
			break;
		}
	OstTraceFunctionExitExt( TMMCARDARRAY_CARDINDEX_EXIT, this, i );
	return i;
	}

//	--------  class TMMCCommandDesc  --------

EXPORT_C TInt TMMCCommandDesc::Direction() const
/**
 * returns -1, 0 or +1 for DT directions read, none or write respectively
 */
	{
	OstTraceFunctionEntry1( TMMCCOMMANDDESC_DIRECTION_ENTRY, this );
	TUint dir = iSpec.iDirection;
	TInt result = dir;
	TInt ret;

	if( dir == 0 )
	    {
	    ret = 0;
		OstTraceFunctionExitExt( TMMCCOMMANDDESC_DIRECTION_EXIT1, this, ret );
		return ret;
	    }

	if( dir & KMMCCmdDirWBitArgument )
		result = TUint(iArgument) >> (dir & KMMCCmdDirIndBitPosition);

	if( dir & KMMCCmdDirNegate )
		result = ~result;

	ret = ((result&1)-1)|1;

	OstTraceFunctionExitExt( TMMCCOMMANDDESC_DIRECTION_EXIT2, this, ret );
	return ret;
	}


TBool TMMCCommandDesc::AdjustForBlockOrByteAccess(const DMMCSession& aSession)
	{
	OstTraceExt2(TRACE_FLOW, TMMCCOMMANDDESC_ADJUSTFORBLOCKORBYTEACCESS_ENTRY, "TMMCCommandDesc::AdjustForBlockOrByteAccess;Session ID=%d;this=%x", (TInt) aSession.SessionID(), (TUint) this);
/**
 * The MMC session provides both block and byte based IO methods, all of which can
 * be used on both block and byte based MMC cards.  This method adjusts the command
 * arguments so that they match the underlying cards access mode.
 *
 * @return ETrue if the address is valid or successfully converted, EFalse otherwise
 */
	TUint32 blockLength = BlockLength();

	if(iTotalLength == 0	||
	   blockLength == 0	||
	   iTotalLength % KMMCardHighCapBlockSize != 0	||	// always aligned on 512 bytes
	   blockLength % KMMCardHighCapBlockSize != 0)
		{
		OstTraceFunctionExitExt( TMMCCOMMANDDESC_ADJUSTFORBLOCKORBYTEACCESS_EXIT1, this, (TUint) EFalse );
		return EFalse;
		}

	if(aSession.CardP()->IsHighCapacity())
		{	
		// high capacity (block-based) card
		if((iFlags & KMMCCmdFlagBlockAddress) == 0)
			{	
			// The command arguments are using byte based addressing
			//  - adjust to block-based addressing
			if(iArgument % KMMCardHighCapBlockSize != 0)
				{
				// Block based media does not support misaligned access
				OstTraceFunctionExitExt( TMMCCOMMANDDESC_ADJUSTFORBLOCKORBYTEACCESS_EXIT2, this, (TUint) EFalse );
				return EFalse;
				}

			// adjust for block based access
			iArgument = iArgument >> KMMCardHighCapBlockSizeLog2;
			iFlags |= KMMCCmdFlagBlockAddress;
			}
		}
	else
		{
		// standard (byte based) card
		if((iFlags & KMMCCmdFlagBlockAddress) != 0)
			{
			// The command arguments are using block based addressing
			//  - adjust to byte-based addressing
			const TUint32 maxBlocks = 4 * 1024 * ((1024 * 1024) >> KMMCardHighCapBlockSizeLog2);
			
			if(iArgument > maxBlocks)
				{
				// The address is out of range (>2G) - cannot convert
				OstTraceFunctionExitExt( TMMCCOMMANDDESC_ADJUSTFORBLOCKORBYTEACCESS_EXIT3, this, (TUint) EFalse );
				return EFalse;
				}

			// adjust for byte-based access
			iArgument = iArgument << KMMCardHighCapBlockSizeLog2;
			iFlags &= ~KMMCCmdFlagBlockAddress;
			}
		else if(iArgument % KMMCardHighCapBlockSize != 0)
			{
			// byte addressing, unaligned address
			OstTraceFunctionExitExt( TMMCCOMMANDDESC_ADJUSTFORBLOCKORBYTEACCESS_EXIT4, this, (TUint) EFalse );
			return EFalse;
			}
		}

	OstTraceFunctionExitExt( TMMCCOMMANDDESC_ADJUSTFORBLOCKORBYTEACCESS_EXIT5, this, (TUint) ETrue );
	return ETrue;
	}

void TMMCCommandDesc::Dump(TUint8* aResponseP, TMMCErr aErr)
	{

	Kern::Printf("------------------------------------------------------------------");
	Kern::Printf("CMD %02d(0x%08x) - ",TUint(iCommand),TUint(iArgument));

	switch(iCommand)
		{
		case 0  : Kern::Printf("                   | GO_IDLE_STATE");			break;
		case 1  : Kern::Printf("                   | SEND_OP_COND");			break;
		case 2  : Kern::Printf("                   | ALL_SEND_CID");			break;
		case 3  : Kern::Printf("                   | SET_RELATIVE_ADDR");		break;
		case 4  : Kern::Printf("                   | SET_DSR");					break;
		case 5  : Kern::Printf("                   | SLEEP/AWAKE");				break;
		case 6  : Kern::Printf("                   | SWITCH");					break;
		case 7  : Kern::Printf("                   | SELECT/DESELECT_CARD");	break;
		case 8  : Kern::Printf("                   | SEND_EXT_CSD");			break;
		case 9  : Kern::Printf("                   | SEND_CSD");				break;
		case 10 : Kern::Printf("                   | SEND_CID");				break;
		case 11 : Kern::Printf("                   | READ_DAT_UNTIL_STOP");		break;
		case 12 : Kern::Printf("                   | STOP_TRANSMISSION");		break;
		case 13 : Kern::Printf("                   | SEND_STATUS");				break;
		case 14 : Kern::Printf("                   | BUSTEST_R");				break;
		case 15 : Kern::Printf("                   | GO_INACTIVE_STATE");		break;
		case 16 : Kern::Printf("                   | SET_BLOCKLEN");			break;
		case 17 : Kern::Printf("                   | READ_SINGLE_BLOCK");		break;
		case 18 : Kern::Printf("                   | READ_MULTIPLE_BLOCK");		break;
		case 19 : Kern::Printf("                   | BUSTEST_W");				break;
		case 20 : Kern::Printf("                   | WRITE_DAT_UNTIL_STOP");	break;
		case 23 : Kern::Printf("                   | SET_BLOCK_COUNT");			break;
		case 24 : Kern::Printf("                   | WRITE_BLOCK");				break;
		case 25 : Kern::Printf("                   | WRITE_MULTIPLE_BLOCK");	break;
		case 26 : Kern::Printf("                   | PROGRAM_CID");				break;
		case 27 : Kern::Printf("                   | PROGRAM_CSD");				break;
		case 28 : Kern::Printf("                   | SET_WRITE_PROT");			break;
		case 29 : Kern::Printf("                   | CLR_WRITE_PROT");			break;
		case 30 : Kern::Printf("                   | SEND_WRITE_PROT");			break;
		case 32 : Kern::Printf("                   | ERASE_WR_BLK_START");		break;	// SD
		case 33 : Kern::Printf("                   | ERASE_WR_BLK_END");		break;	// SD
		case 35 : Kern::Printf("                   | ERASE_GROUP_START");		break;
		case 36 : Kern::Printf("                   | ERASE_GROUP_END");			break;
		case 38 : Kern::Printf("                   | ERASE");					break;
		case 39 : Kern::Printf("                   | FAST_IO");					break;
		case 40 : Kern::Printf("                   | GO_IRQ_STATE");			break;
		case 42 : Kern::Printf("                   | LOCK_UNLOCK");				break;
		case 55 : Kern::Printf("                   | APP_CMD");					break;
		case 56 : Kern::Printf("                   | GEN_CMD");					break;
		default : Kern::Printf("                   | *** UNKNOWN COMMAND ***"); break;
		}

	switch(iSpec.iResponseType)
		{
		case ERespTypeNone:		Kern::Printf("               RSP - NONE");		break;
		case ERespTypeUnknown:	Kern::Printf("               RSP - UNKNOWN");	break;
		case ERespTypeR1:		Kern::Printf("               RSP - R1");		break;
		case ERespTypeR1B:		Kern::Printf("               RSP - R1b");		break;
		case ERespTypeR2:		Kern::Printf("               RSP - R2");		break;
		case ERespTypeR3:		Kern::Printf("               RSP - R3");		break;
		case ERespTypeR4:		Kern::Printf("               RSP - R4");		break;
		case ERespTypeR5:		Kern::Printf("               RSP - R5");		break;
		case ERespTypeR6:		Kern::Printf("               RSP - R6");		break;
		default :				Kern::Printf("               RSP - *** UNKNOWN RESPONSE ***"); break;
		}

	switch(iSpec.iResponseLength)
		{
		case 0  :																				break;
		case 4  : Kern::Printf("                   | 0x%08x", TMMC::BigEndian32(aResponseP));	break;
		case 16 : Kern::Printf("                   | 0x%08x 0x%08x 0x%08x 0x%08x", ((TUint32*)aResponseP)[0], ((TUint32*)aResponseP)[1], ((TUint32*)aResponseP)[2], ((TUint32*)aResponseP)[3]); break;
		default : Kern::Printf("                   | *** RESPONSE NOT PARSED ***");				break;
		}
									  Kern::Printf("               ERR - 0x%08x", aErr);
	if(aErr & KMMCErrResponseTimeOut) Kern::Printf("                   | KMMCErrResponseTimeOut");
	if(aErr & KMMCErrDataTimeOut)	  Kern::Printf("                   | KMMCErrDataTimeOut");
	if(aErr & KMMCErrBusyTimeOut)	  Kern::Printf("                   | KMMCErrBusyTimeOut");
	if(aErr & KMMCErrBusTimeOut)	  Kern::Printf("                   | KMMCErrBusTimeOut");
	if(aErr & KMMCErrTooManyCards)	  Kern::Printf("                   | KMMCErrTooManyCards");
	if(aErr & KMMCErrResponseCRC)	  Kern::Printf("                   | KMMCErrResponseCRC");
	if(aErr & KMMCErrDataCRC)		  Kern::Printf("                   | KMMCErrDataCRC");
	if(aErr & KMMCErrCommandCRC)	  Kern::Printf("                   | KMMCErrCommandCRC");
	if(aErr & KMMCErrStatus)		  Kern::Printf("                   | KMMCErrStatus");
	if(aErr & KMMCErrNoCard)		  Kern::Printf("                   | KMMCErrNoCard");
	if(aErr & KMMCErrBrokenLock)	  Kern::Printf("                   | KMMCErrBrokenLock");
	if(aErr & KMMCErrPowerDown)		  Kern::Printf("                   | KMMCErrPowerDown");
	if(aErr & KMMCErrAbort)			  Kern::Printf("                   | KMMCErrAbort");
	if(aErr & KMMCErrStackNotReady)	  Kern::Printf("                   | KMMCErrStackNotReady");
	if(aErr & KMMCErrNotSupported)	  Kern::Printf("                   | KMMCErrNotSupported");
	if(aErr & KMMCErrHardware)		  Kern::Printf("                   | KMMCErrHardware");
	if(aErr & KMMCErrBusInconsistent) Kern::Printf("                   | KMMCErrBusInconsistent");
	if(aErr & KMMCErrBypass)		  Kern::Printf("                   | KMMCErrBypass");
	if(aErr & KMMCErrInitContext)	  Kern::Printf("                   | KMMCErrInitContext");
	if(aErr & KMMCErrArgument)		  Kern::Printf("                   | KMMCErrArgument");
	if(aErr & KMMCErrSingleBlock)	  Kern::Printf("                   | KMMCErrSingleBlock");
	if(aErr & KMMCErrUpdPswd)		  Kern::Printf("                   | KMMCErrUpdPswd");
	if(aErr & KMMCErrLocked)		  Kern::Printf("                   | KMMCErrLocked");
	if(aErr & KMMCErrNotFound)		  Kern::Printf("                   | KMMCErrNotFound");
	if(aErr & KMMCErrAlreadyExists)	  Kern::Printf("                   | KMMCErrAlreadyExists");
	if(aErr & KMMCErrGeneral)		  Kern::Printf("                   | KMMCErrGeneral");


	if(iSpec.iResponseType == ERespTypeR1 || iSpec.iResponseType == ERespTypeR1B)
		{
		const TUint32 stat = TMMC::BigEndian32(aResponseP);
		
											 Kern::Printf("              STAT - 0x%08x", stat);
		if(stat & KMMCStatAppCmd)			 Kern::Printf("                   | KMMCStatAppCmd");
		if(stat & KMMCStatSwitchError)		 Kern::Printf("                   | KMMCStatSwitchError");
		if(stat & KMMCStatReadyForData)		 Kern::Printf("                   | KMMCStatReadyForData");
		if(stat & KMMCStatCurrentStateMask){ Kern::Printf("                   | KMMCStatCurrentStateMask");
											 const TMMCardStateEnum cardState = (TMMCardStateEnum)(stat & KMMCStatCurrentStateMask);
											 switch (cardState){
												 case ECardStateIdle  : Kern::Printf("                     | ECardStateIdle"); break;
												 case ECardStateReady : Kern::Printf("                     | ECardStateReady"); break;
												 case ECardStateIdent : Kern::Printf("                     | ECardStateIdent"); break;
												 case ECardStateStby  : Kern::Printf("                     | ECardStateStby"); break;
												 case ECardStateTran  : Kern::Printf("                     | ECardStateTran"); break;
												 case ECardStateData  : Kern::Printf("                     | ECardStateData"); break;
												 case ECardStateRcv   : Kern::Printf("                     | ECardStateRcv"); break;
												 case ECardStatePrg   : Kern::Printf("                     | ECardStatePrg"); break;
												 case ECardStateDis   : Kern::Printf("                     | ECardStateDis"); break;
												 case ECardStateBtst  : Kern::Printf("                     | ECardStateBtst"); break;
												 case ECardStateSlp   : Kern::Printf("                     | ECardStateSlp"); break;
												 default   	 		  : Kern::Printf("                     | ECardStateUnknown"); break;
											 }
										   }
		if(stat & KMMCStatEraseReset)		 Kern::Printf("                   | KMMCStatEraseReset");
		if(stat & KMMCStatCardECCDisabled)	 Kern::Printf("                   | KMMCStatCardECCDisabled");
		if(stat & KMMCStatWPEraseSkip)		 Kern::Printf("                   | KMMCStatWPEraseSkip");
		if(stat & KMMCStatErrCSDOverwrite)	 Kern::Printf("                   | KMMCStatErrCSDOverwrite");
		if(stat & KMMCStatErrOverrun)		 Kern::Printf("                   | KMMCStatErrOverrun");
		if(stat & KMMCStatErrUnderrun)		 Kern::Printf("                   | KMMCStatErrUnderrun");
		if(stat & KMMCStatErrUnknown)		 Kern::Printf("                   | KMMCStatErrUnknown");
		if(stat & KMMCStatErrCCError)		 Kern::Printf("                   | KMMCStatErrCCError");
		if(stat & KMMCStatErrCardECCFailed)  Kern::Printf("                   | KMMCStatErrCardECCFailed");
		if(stat & KMMCStatErrIllegalCommand) Kern::Printf("                   | KMMCStatErrIllegalCommand");
		if(stat & KMMCStatErrComCRCError)	 Kern::Printf("                   | KMMCStatErrComCRCError");
		if(stat & KMMCStatErrLockUnlock)	 Kern::Printf("                   | KMMCStatErrLockUnlock");
		if(stat & KMMCStatCardIsLocked)		 Kern::Printf("                   | KMMCStatCardIsLocked");
		if(stat & KMMCStatErrWPViolation)	 Kern::Printf("                   | KMMCStatErrWPViolation");
		if(stat & KMMCStatErrEraseParam)	 Kern::Printf("                   | KMMCStatErrEraseParam");
		if(stat & KMMCStatErrEraseSeqError)  Kern::Printf("                   | KMMCStatErrEraseSeqError");
		if(stat & KMMCStatErrBlockLenError)  Kern::Printf("                   | KMMCStatErrBlockLenError");
		if(stat & KMMCStatErrAddressError)	 Kern::Printf("                   | KMMCStatErrAddressError");
		if(stat & KMMCStatErrOutOfRange)	 Kern::Printf("                   | KMMCStatErrOutOfRange");
		}

	Kern::Printf("                   -----------------------------------------------");
	}

//	--------  class TMMCRCAPool  --------

TRCA TMMCRCAPool::GetFreeRCA()
/**
 * Returns a free RCA number from the pool or zero if none is available
 */
	{
	OstTraceFunctionEntry1( TMMCRCAPOOL_GETFREERCA_ENTRY, this );
	TUint32 seekm = (iPool | iLocked) + 1;
	iPool |= (seekm & ~iLocked);
	TUint16 ret;
	
	if( (seekm & 0xFFFFFFFF) == 0 )
	    {
	    ret = 0;
		OstTraceFunctionExitExt( TMMCRCAPOOL_GETFREERCA_EXIT1, this, (TUint) ret);
		return ret;
	    }

	TUint16 pos = 1;

	if ((seekm & 0xFFFF) == 0)	{ seekm >>= 16;	pos = 17; }
	if ((seekm & 0xFF) == 0)	{ seekm >>= 8;	pos += 8; }
	if ((seekm & 0xF) == 0)		{ seekm >>= 4;	pos += 4; }
	if ((seekm & 0x3) == 0)		{ seekm >>= 2;	pos += 2; }
	if ((seekm & 0x1) == 0)		pos++;

	// Multiply return value by 257 so that 1 is never returned.  (0x0001 is the default RCA value.)
	// The RCA integer value is divided by 257 in LockRCA() and UnlockRCA() to compensate
	// for this adjustment.  These functions are only ever called in this file with the iRCA
	// field of a TMMCard object, and not with arbitrary values.
	// The iRCA field itself is only assigned values from iNewCards[] or zero.  iNewCards
	// in turn is fed values from this function, in DMMCStack::CIMUpdateAcqSM() / EStSendCIDIssued.

	ret = TUint16(pos << 8 | pos);
	OstTraceFunctionExitExt( TMMCRCAPOOL_GETFREERCA_EXIT2, this, (TUint) ret);
	return ret;
	}



//	--------  class TMMCSessRing  --------

TMMCSessRing::TMMCSessRing()
/**
 * Constructor
 */
	: iPMark(NULL),iPoint(NULL),iPrevP(NULL),iSize(0)
	{OstTraceFunctionEntry1( TMMCSESSRING_TMMCSESSRING_ENTRY, this );}


void TMMCSessRing::Erase()
/**
 * Erases all the ring content
 */
	{
	OstTraceFunctionEntry1( TMMCSESSRING_ERASE_ENTRY, this );
	iPMark = iPoint = iPrevP = NULL; iSize = 0;
	OstTraceFunctionExit1( TMMCSESSRING_ERASE_EXIT, this );
	}


DMMCSession* TMMCSessRing::operator++(TInt)
/**
 * Post increment of Point
 */
	{
	if( iPoint == NULL )
		return( NULL );

	if( (iPrevP=iPoint) == iPMark )
		iPoint = NULL;
	else
		iPoint = iPoint->iLinkP;

	return( iPrevP );
	}


TBool TMMCSessRing::Point(DMMCSession* aSessP)
/**
 * Finds aSessP and sets Point to that position
 */
	{
	OstTraceFunctionEntryExt( TMMCSESSRING_POINT_ENTRY, this );
	Point();

	while( iPoint != NULL )
		if( iPoint == aSessP )
		    {
			OstTraceFunctionExitExt( TMMCSESSRING_POINT_EXIT1, this, (TUint) ETrue );
			return ETrue;
		    }
		else
			this->operator++(0);

	OstTraceFunctionExitExt( TMMCSESSRING_POINT_EXIT2, this, (TUint) EFalse );
	return EFalse;
	}

void TMMCSessRing::Add(DMMCSession* aSessP)
/**
 * Inserts aSessP before Marker. Point is moved into the Marker position.
 */
	{
	OstTraceFunctionEntryExt( TMMCSESSRING_ADD1_ENTRY, this );
	if( iSize == 0 )
		{
		iPMark = iPrevP = iPoint = aSessP;
		aSessP->iLinkP = aSessP;
		iSize = 1;
		OstTraceFunctionExit1( TMMCSESSRING_ADD1_EXIT1, this );
		return;
		}

	iPoint = iPMark->iLinkP;
	iPMark->iLinkP = aSessP;
	aSessP->iLinkP = iPoint;
	iPMark = iPrevP = aSessP;
	iSize++;
	OstTraceFunctionExit1( TMMCSESSRING_ADD1_EXIT2, this );
	}


void TMMCSessRing::Add(TMMCSessRing& aRing)
/**
 * Inserts aRing before Marker. Point is moved into the Marker position.
 * aRing Marker becomes the fisrt inserted element.
 * Erases aRing.
 */
	{
	OstTraceFunctionEntry1( TMMCSESSRING_ADD2_ENTRY, this );
	Point();

	if( aRing.iSize == 0 )
	    {
		OstTraceFunctionExit1( TMMCSESSRING_ADD2_EXIT1, this );
		return;
	    }

	if( iSize == 0 )
		{
		iPrevP = iPMark = aRing.iPMark;
		iPoint = iPrevP->iLinkP;
		iSize = aRing.iSize;
		}
	else
		{
		iPrevP->iLinkP = aRing.iPMark->iLinkP;
		iPMark = iPrevP = aRing.iPMark;
		iPrevP->iLinkP = iPoint;
		iSize += aRing.iSize;
		}

	aRing.Erase();
	OstTraceFunctionExit1( TMMCSESSRING_ADD2_EXIT2, this );
	}

DMMCSession* TMMCSessRing::Remove()
/**
 * Removes an element pointed to by Point.
 * Point (and possibly Marker) move forward as in operator++
 */
	{
	OstTraceFunctionEntry1( TMMCSESSRING_REMOVE1_ENTRY, this );
	DMMCSession* remS = iPrevP;

	if( iSize < 2 )
		Erase();
	else
		{
		remS = remS->iLinkP;
		iPrevP->iLinkP = remS->iLinkP;
		iSize--;

		if( iPoint != NULL )
			iPoint = iPrevP->iLinkP;

		if( iPMark == remS )
			{
			iPMark = iPrevP;
			iPoint = NULL;
			}
		}

	OstTraceFunctionExitExt( TMMCSESSRING_REMOVE1_EXIT, this, ( TUint )( remS ) );
	return remS;
	}


void TMMCSessRing::Remove(DMMCSession* aSessP)
/**
 * Removes a specified session from the ring
 */
	{
	OstTraceFunctionEntryExt( TMMCSESSRING_REMOVE2_ENTRY, this );
	if( Point(aSessP) )
		Remove();
	else
		DMMCSocket::Panic(DMMCSocket::EMMCSessRingNoSession);
	OstTraceFunctionExit1( TMMCSESSRING_REMOVE2_EXIT, this );
	}



//	--------  class TMMCStateMachine  --------


/**
Removes all state from the state machine.

It also resets the stack and the exit code.
*/
EXPORT_C void TMMCStateMachine::Reset()
	{
	OstTraceFunctionEntry1( TMMCSTATEMACHINE_RESET_ENTRY, this );
	iAbort = EFalse;
	iSP = 0; iExitCode = 0;
	iStack[0].iState = 0; iStack[0].iTrapMask = 0;
	OstTraceFunctionExit1( TMMCSTATEMACHINE_RESET_EXIT, this );
	}




/**
The state machine dispatcher.

@return The MultiMediaCard error code. 
*/
EXPORT_C TMMCErr TMMCStateMachine::Dispatch()
	{
	OstTraceFunctionEntry1( TMMCSTATEMACHINE_DISPATCH_ENTRY, this );
	
	// If a state machine returns non-zero, i.e. a non-empty error set, then the second
	// inner while loop is broken.  The errors are thrown like an exception where the
	// stack is unravelled until it reaches a state machine which can handle at least
	// one of the error codes, else this function returns with the exit code or'd with
	// KMMCErrBypass.  If the state machine returns zero, then this function returns
	// zero if iSuspend is set, i.e., if the stack is waiting on an asynchronous event.
	// If suspend is not set, then the next state machine is called.  This may be the
	// same as the current state machine, or its caller if the current state machine
	// ended called Pop() before exiting, e.g., via SMF_END.

	while( iSP >= 0 && !iAbort )
		{
		// If there is an un-trapped error, wind back down the stack, either
		// to the end of the stack or until the error becomes trapped.
		while( iSP >= 0 && (iExitCode & ~iStack[iSP].iTrapMask) != 0 )
			iSP--;

		iExitCode &= ~KMMCErrBypass;

		if ( iExitCode )
			{
			__KTRACE_OPT(KPBUS1,Kern::Printf(">MMC:Err %x",iExitCode));
			OstTrace1( TRACE_INTERNALS, TMMCSTATEMACHINE_DISPATCH, "iExitCode=0x%x", iExitCode );
			}

		while( iSP >= 0 && !iAbort )
			{
			__KTRACE_OPT(KPBUS1,Kern::Printf("-msm:dsp:%02x:%08x.%02x",iSP, TUint32(iStack[iSP].iFunction), State()));
			OstTraceExt3( TRACE_INTERNALS, TMMCSTATEMACHINE_DISPATCH2, "iSP=%d; iStack[iSP].iFunction=0x%08x; State=0x%02x", (TInt) iSP, (TUint) iStack[iSP].iFunction, (TUint) State() );

			iSuspend = ETrue;
			const TMMCErr signal = iStack[iSP].iFunction(iContextP);

			if (signal)
				{
				iExitCode = signal;
				break;
				}

			if( iSuspend )
				{
				__KTRACE_OPT(KPBUS1,Kern::Printf("<msm:dsp:exitslp"));
				OstTraceFunctionExit1( TMMCSTATEMACHINE_DISPATCH_EXIT1, this );
				return(0);
				}
			}
		}

	__KTRACE_OPT(KPBUS1,Kern::Printf("<msm:dsp:exit%08x", iExitCode));
	OstTraceFunctionExit1( TMMCSTATEMACHINE_DISPATCH_EXIT2, this );
	return( KMMCErrBypass | iExitCode );
	}




/**
Pushes another state machine entry onto the stack.

Typically, this is invoked using one of the macros:
SMF_CALL, SMF_CALLWAIT, SMF_INVOKES, SMF_INVOKEWAITS 

@param anEntry  The state machine function to be run; this will start at
                the initial state (EStBegin), with no exception handling defined.
@param aSuspend Indicates whether the state machine is to block on return to the dispatcher;
                Specify ETrue to block; EFalse not to block.
                EFalse is the default, if not explicitly stated.

@return KMMCErrNone

@panic PBUS-MMC 0 if the maximum depth of nested state machine entries is being exeeded.

@see SMF_CALL
@see SMF_CALLWAIT
@see SMF_INVOKES
@see SMF_INVOKEWAITS 
*/
EXPORT_C TMMCErr TMMCStateMachine::Push(TMMCErr (*anEntry)(TAny*), TBool aSuspend)
	{
	OstTraceFunctionEntry1( TMMCSTATEMACHINE_PUSH_ENTRY, this );
	iSP++;
	__ASSERT_ALWAYS(TUint(iSP)<KMaxMMCMachineStackDepth,
		DMMCSocket::Panic(DMMCSocket::EMMCMachineStack));
	iStack[iSP].iFunction = anEntry;
	iStack[iSP].iState = 0;
	iStack[iSP].iTrapMask = 0;
	if( !aSuspend )
		iSuspend = EFalse;
	OstTraceFunctionExit1( TMMCSTATEMACHINE_PUSH_EXIT, this );
	return 0;
	}




/**
Jumps to the specified state machine function in the current state machine entry.

@param anEntry  The state machine function to be run; this will start at
                the initial state (EStBegin), with no exception handling defined.
@param aSuspend Indicates whether the state machine is to block on return to the dispatcher;
                Specify ETrue to block; EFalse not to block.
                EFalse is the default, if not explicitly stated.

@return KMMCErrNone
*/
EXPORT_C TMMCErr TMMCStateMachine::Jump(TMMCErr (*anEntry)(TAny*), TBool aSuspend)
	{
	OstTraceFunctionEntry1( TMMCSTATEMACHINE_JUMP_ENTRY, this );
	iStack[iSP].iFunction = anEntry;
	iStack[iSP].iState = 0;
	iStack[iSP].iTrapMask = 0;
	if( !aSuspend )
		iSuspend = EFalse;
	OstTraceFunctionExit1( TMMCSTATEMACHINE_JUMP_EXIT, this );
	return 0;
	}




//	--------  class DMMCStack  --------

#pragma warning( disable : 4355 )	// this used in initializer list
EXPORT_C DMMCStack::DMMCStack(TInt /*aBus*/, DMMCSocket* aSocket)
/**
 * Constructs a DMMCStack object
 * @param aBus Unused
 * @param aSocket A pointer to the associated socket.
 */
	: iWorkSet(),
	iReadyQueue(),
	iEntryQueue(),
	iStackDFC(DMMCStack::StackDFC, this, 1),
	iSelectedCard(TUint16(~0)),
	iSocket(aSocket),
	iStackSession(NULL),
	iAutoUnlockSession(TMMCCallBack(AutoUnlockCBST, this)),
	iInitState(EISPending),
	iInitialise(ETrue),
	iCurrentDSR(),
	iConfig(),
	iRCAPool(),
	iMasterConfig()
	{
//	iStackState(0),
//	iLockingSessionP(NULL),
//	iAttention(EFalse),
//	iAbortReq(EFalse),
//	iCompReq(EFalse),
//	iDoorOpened(EFalse),
//	iPoweredUp(EFalse),
//	iDFCRunning(EFalse),
//	iAbortAll(EFalse),
//	iAllExitCode(0),
//	iSessionP(NULL),
//	iCurrentOpRange(0),
//	iCardsPresent(0),
//	iMaxCardsInStack(0)
	}
#pragma warning( default : 4355 )

EXPORT_C TInt DMMCStack::Init()
/**
 * Initialises the generic MMC stack.
 * @return KErrNone if successful, standard error code otherwise.
 */
	{
	OstTraceFunctionEntry1( DMMCSTACK_INIT_ENTRY, this );
	// allocate and initialize session object
	if ((iStackSession = AllocSession(TMMCCallBack(StackSessionCBST, this))) == 0)
	    {
		OstTraceFunctionExitExt( DMMCSTACK_INIT_EXIT1, this, KErrNoMemory );
		return KErrNoMemory;
	    }

	// create helper class
	if ((iBody = new DBody(*this)) == NULL)
	    {
		OstTraceFunctionExitExt( DMMCSTACK_INIT_EXIT2, this, KErrNoMemory );
		return KErrNoMemory;
	    }

	iStackSession->SetStack(this);

	iStackDFC.SetDfcQ(&iSocket->iDfcQ);

	// Get the maximal number of cards from ASSP layer
	iMaxCardsInStack = iSocket->TotalSupportedCards();
	if ( iMaxCardsInStack > KMaxMMCardsPerStack )
		iMaxCardsInStack=KMaxMMCardsPerStack;

	TInt r = iCardArray->AllocCards();

	OstTraceFunctionExitExt( DMMCSTACK_INIT_EXIT3, this, r );
	return r;
	}

EXPORT_C void DMMCStack::PowerUpStack()
/**
 * Enforce stack power-up and initialisation.
 * This is an asynchronous operation, which calls DMMCSocket::PowerUpSequenceComplete upon completion.
 */
	{
	OstTraceFunctionEntry1( DMMCSTACK_POWERUPSTACK_ENTRY, this );
	__KTRACE_OPT(KPBUS1,Kern::Printf("=mst:pus"));

	if (iPSLBuf == NULL)
		{
		GetBufferInfo(&iPSLBuf, &iPSLBufLen);
		iMinorBufLen = KMinMinorBufSize;
		}

	ReportPowerDown();							// ensure power will be switch on regardless

	Scheduler( iInitialise );
	OstTraceFunctionExit1( DMMCSTACK_POWERUPSTACK_EXIT, this );
	}

void DMMCStack::QSleepStack()
/**
 * Schedules a session to place media in Sleep State
 */
	{
	OstTraceFunctionEntry1( DMMCSTACK_QSLEEPSTACK_ENTRY, this );
	__KTRACE_OPT(KPBUS1,Kern::Printf("=mst:qsleep"));

	Scheduler( iSleep );
	OstTraceFunctionExit1( DMMCSTACK_QSLEEPSTACK_EXIT, this );
	}

EXPORT_C void DMMCStack::PowerDownStack()
/**
 * Enforce stack power down.
 * Clients generally shouldn't need to concern themselves with powering down a stack 
 * unless they specifically need to perform a power reset of a card.  If a driver fails to 
 * open then normal practise is for that driver to leave the card powered so that any subsequent 
 * driver which may attempt to open immediately after this failed attempt won't have to re-power the card. 
 * If no driver successfully opens on the card then the Controllers inactivity/not in use 
 * timeout system can be left to power it down.
 */
	{
	OstTraceFunctionEntry1( DMMCSTACK_POWERDOWNSTACK_ENTRY, this );
	__KTRACE_OPT(KPBUS1,Kern::Printf("=mst:pds"));

	ReportPowerDown();
	iInitState = EISPending;
	DoPowerDown();

	TBool cardRemoved = (iStackState & KMMCStackStateCardRemoved);
	for (TUint i=0;i<iMaxCardsInStack;i++)
		{
		TMMCard& card = iCardArray->Card(i);
		card.SetBusWidth(1);
		card.SetHighSpeedClock(0);
		if (cardRemoved)
			{
		    iCardArray->DeclareCardAsGone(i);
			}
		else
			{
			// set the locked bit if the card has a password - need to do this 
			// now that RLocalDrive::Caps() no longer powers up the stack
			if (card.HasPassword())
				{
				TMapping* pmp = iSocket->iPasswordStore->FindMappingInStore(card.CID());
				if (!pmp || pmp->iState != TMapping::EStValid)
					{
					*((TUint32*) &card.iStatus) |= KMMCStatCardIsLocked;
					}
				}
			
			// Remove card state flags, after a power cycle all cards are in idle state
			*((TUint32*) &card.iStatus) &= ~KMMCStatCurrentStateMask;
			}
		}
	if (cardRemoved)
	    iStackState &= ~KMMCStackStateCardRemoved;


	iSocket->iVcc->SetState(EPsuOff);
	if (iSocket->iVccCore)
		iSocket->iVccCore->SetState(EPsuOff);

	// Cancel timers, reset ASSP, cancel stack DFC & remove session from workset 
	// to ensure stack doesn't wake up again & attempt to dereference iSessionP
 	if (iSessionP)
		Abort(iSessionP);

	iStackDFC.Cancel();

	// The stack may have powered down while attempting to power up (e.g. because a card has not responded), 
	// so ensure stack doesn't attempt to initialize itself again until next PowerUpStack()
	iInitialise = EFalse;
	iStackState &= ~(KMMCStackStateInitInProgress | KMMCStackStateInitPending | KMMCStackStateBusInconsistent | KMMCStackStateWaitingDFC);
	iSessionP = NULL;
	OstTraceFunctionExit1( DMMCSTACK_POWERDOWNSTACK_EXIT, this );
	}

//
// DMMCStack:: --- Stack Scheduler and its supplementary functions ---
//
DMMCStack::TMMCStackSchedStateEnum DMMCStack::SchedGetOnDFC()
/**
 * Initiates stack DFC. Returns either Continue or Loop.
 */
	{
	OstTraceFunctionEntry1( DMMCSTACK_SCHEDGETONDFC_ENTRY, this );
	__KTRACE_OPT(KPBUS1,Kern::Printf("=mst:sgd"));

	if( iDFCRunning )
	    {
		OstTraceFunctionExitExt( DMMCSTACK_SCHEDGETONDFC_EXIT1, this, (TInt) ESchedContinue);
		return ESchedContinue;
	    }

	if( (iStackState & KMMCStackStateWaitingDFC) == 0 )
		{
		__KTRACE_OPT(KPBUS1,Kern::Printf("=mst:sgd:q"));
		iStackState |= KMMCStackStateWaitingDFC;
		if (NKern::CurrentContext()==NKern::EInterrupt)
			iStackDFC.Add();
		else
			iStackDFC.Enque();
		}

	OstTraceFunctionExitExt( DMMCSTACK_SCHEDGETONDFC_EXIT2, this, (TInt) ESchedLoop);
	return ESchedLoop;
	}

void DMMCStack::SchedSetContext(DMMCSession* aSessP)
/**
 * Sets up the specified session as the current session.
 * Invoked by JobChooser and Initialiser.
 * @param aSessP A pointer to the session.
 */
	{
	OstTraceFunctionEntry1( DMMCSTACK_SCHEDSETCONTEXT_ENTRY, this );
	__KTRACE_OPT(KPBUS1,Kern::Printf("=mst:ssc"));

	if( (iStackState & (KMMCStackStateInitPending|KMMCStackStateBusInconsistent)) != 0 &&
		aSessP->iSessionID != ECIMInitStack )
		{
		iInitialise = ETrue;
		OstTraceFunctionExit1( DMMCSTACK_SCHEDSETCONTEXT_EXIT1, this );
		return;
		}

	if( iSessionP != aSessP )
		{
		iStackState |= KMMCStackStateReScheduled;
		MergeConfig( aSessP );

		if( aSessP->iSessionID == ECIMInitStack )
			iInitialise = ETrue;
		else
			if( InitStackInProgress() )
				MarkComplete( aSessP, KMMCErrStackNotReady );
			else
				if( aSessP->iBrokenLock )
					MarkComplete( aSessP, KMMCErrBrokenLock );

		iSessionP = aSessP;
		}

	iSessionP->iState &= ~KMMCSessStateDoReSchedule;
	OstTraceFunctionExit1( DMMCSTACK_SCHEDSETCONTEXT_EXIT2, this );
	}

void DMMCStack::SchedDoAbort(DMMCSession* aSessP)
/**
 * Aborts asynchronous activities of a session aSessP
 * @param aSessP A pointer to the session to be aborted.
 */
	{
	OstTraceFunctionEntryExt( DMMCSTACK_SCHEDDOABORT_ENTRY, this );
	__KTRACE_OPT(KPBUS1,Kern::Printf("=mst:sda"));

#ifdef __EPOC32__
	if( aSessP->iBlockOn & KMMCBlockOnPollTimer )
		aSessP->iPollTimer.Cancel();

	if( aSessP->iBlockOn & KMMCBlockOnRetryTimer )
		aSessP->iRetryTimer.Cancel();

	if( aSessP->iBlockOn & KMMCBlockOnPgmTimer )
		aSessP->iProgramTimer.Cancel();
#endif	// #ifdef __EPOC32__

	if( aSessP->iBlockOn & KMMCBlockOnWaitToLock )
		iStackState &= ~KMMCStackStateWaitingToLock;

	if( aSessP->iBlockOn & (KMMCBlockOnASSPFunction | KMMCBlockOnInterrupt | KMMCBlockOnDataTransfer) )
		ASSPReset();

	if( (aSessP->iState & (KMMCSessStateInProgress|KMMCSessStateCritical)) ==
		 (KMMCSessStateInProgress|KMMCSessStateCritical) )
		iStackState |= KMMCStackStateInitPending;
	
	
	(void)__e32_atomic_and_ord32(&aSessP->iBlockOn, ~(KMMCBlockOnPollTimer | KMMCBlockOnRetryTimer |
						  							  KMMCBlockOnWaitToLock | KMMCBlockOnASSPFunction | 
						  							  KMMCBlockOnInterrupt | KMMCBlockOnDataTransfer) );
	OstTraceFunctionExit1( DMMCSTACK_SCHEDDOABORT_EXIT, this );
	}

DMMCStack::TMMCStackSchedStateEnum DMMCStack::SchedResolveStatBlocks(DMMCSession* aSessP)
/**
 * Checks static blocking conditions and removes them as necessary
 * @param aSessP A pointer to the session.
 * @return EschedContinue or ESchedLoop (if scheduler is to be restarted)
 */
	{
	OstTraceFunctionEntryExt( DMMCSTACK_SCHEDRESOLVESTATBLOCKS_ENTRY, this );

	__KTRACE_OPT(KPBUS1,Kern::Printf("=mst:srsb"));

	if( (aSessP->iBlockOn & KMMCBlockOnCardInUse) && aSessP->iCardP->iUsingSessionP == NULL )
		aSessP->SynchUnBlock( KMMCBlockOnCardInUse );

	if( (aSessP->iBlockOn & KMMCBlockOnWaitToLock) && iWorkSet.Size() == 1 )
		{
		// ECIMLockStack processed here
		iLockingSessionP = aSessP;					// in this order
		iStackState &= ~KMMCStackStateWaitingToLock;
		aSessP->SynchUnBlock( KMMCBlockOnWaitToLock );
		MarkComplete( aSessP, KMMCErrNone );
		OstTraceFunctionExitExt( DMMCSTACK_SCHEDRESOLVESTATBLOCKS_EXIT1, this, (TInt) ESchedLoop );
		return ESchedLoop;
		}

	OstTraceFunctionExitExt( DMMCSTACK_SCHEDRESOLVESTATBLOCKS_EXIT2, this, (TInt) ESchedContinue );
	return ESchedContinue;
	}

DMMCStack::TMMCStackSchedStateEnum DMMCStack::SchedGroundDown(DMMCSession* aSessP, TMMCErr aReason)
/**
 * Aborts all asynchronous activities of session aSessP with
 * iExitCode = aReason. This function conserns itself with asynchronous
 * activities only; session static state (eg Critical) is not taken into
 * account. Session dynamic state and action flags (i.e. SafeInGaps,
 * DoReSchedule and DoDFC) are cleared.
 * @param aSessP A pointer to the session.
 * @param aReason The reason for aborting.
 * @return EschedContinue if everything's done OK.
 * @return ESchedLoop if the session can not be safely grounded (eg
 * iStackSession) and should therefore be aborted and/or completed by a
 * separate scheduler pass.
 */
	{
	OstTraceExt3(TRACE_FLOW, DMMCSTACK_SCHEDGROUNDDOWN_ENTRY, "DMMCStack::SchedGroundDown;aSessionP=%x;aReason=%d;this=%x", (TUint) aSessP, (TInt) aReason, (TUint) this);
	__KTRACE_OPT(KPBUS1,Kern::Printf("=mst:sgdn"));

	if( (aSessP == iStackSession) || InitStackInProgress() )
	    {
		OstTraceFunctionExitExt( DMMCSTACK_SCHEDGROUNDDOWN_EXIT1, this, (TInt) ESchedLoop );
		return ESchedLoop;
	    }
	
	if( aSessP->iState & KMMCSessStateInProgress )
		{
		SchedDoAbort( aSessP );
		//coverity[check_return]
		//return value is not saved or checked because there is no further uses.
		aSessP->iMachine.SetExitCode( aReason );
		aSessP->iState &= ~(KMMCSessStateSafeInGaps | KMMCSessStateDoReSchedule |
							KMMCSessStateDoDFC);
		}

	OstTraceFunctionExitExt( DMMCSTACK_SCHEDGROUNDDOWN_EXIT2, this, (TInt) ESchedContinue );
	return ESchedContinue;
	}

DMMCStack::TMMCStackSchedStateEnum DMMCStack::SchedEnqueStackSession(TMMCSessionTypeEnum aSessID)
/**
 * Prepare internal session for InitStack and enque it into WorkSet.
 * @return EschedContinue or ESchedLoop
 */
	{
	OstTraceExt2(TRACE_FLOW, DMMCSTACK_SCHEDENQUESTACKSESSION_ENTRY ,"DMMCStack::SchedEnqueStackSession;aSessID=%d;this=%x", (TInt) aSessID, (TUint) this);
	__KTRACE_OPT(KPBUS1,Kern::Printf("=mst:sess"));

		if( iStackSession->IsEngaged() )
			{
			MarkComplete( iStackSession, KMMCErrAbort );
			OstTraceFunctionExitExt( DMMCSTACK_SCHEDENQUESTACKSESSION_EXIT1, this, (TInt) ESchedLoop );
			return ESchedLoop;
			}

		iStackSession->SetupCIMControl( aSessID );
		iWorkSet.Add( iStackSession );
		iStackSession->iState |= KMMCSessStateEngaged;
		OstTraceFunctionExitExt( DMMCSTACK_SCHEDENQUESTACKSESSION_EXIT2, this, (TInt) ESchedContinue );
		return ESchedContinue;
	}

void DMMCStack::SchedGrabEntries()
/**
 * Merges Entry queue into Ready queue. Invoked at the scheduler entry and
 * after the completion pass
 */
	{
	OstTraceFunctionEntry1( DMMCSTACK_SCHEDGRABENTRIES_ENTRY, this );
	
	__KTRACE_OPT(KPBUS1,Kern::Printf("=mst:sge"));

	iAttention = EFalse;		// Strictly in this order
	if( !iEntryQueue.IsEmpty() )
		{
		DISABLEPREEMPTION
		iReadyQueue.Add( iEntryQueue );
		RESTOREPREEMPTION
		}
	OstTraceFunctionExit1( DMMCSTACK_SCHEDGRABENTRIES_EXIT, this );
	}

void DMMCStack::SchedDisengage()
/**
 * This function is called by AbortPass() and CompletionPass() to remove the session
 * at WorkSet Point, to abort its asynchronous activities (if any) and
 * clear up the dependent resources
 */
	{
	OstTraceFunctionEntry1( DMMCSTACK_SCHEDDISENGAGE_ENTRY, this );
	__KTRACE_OPT(KPBUS1,Kern::Printf("=mst:sd"));

	DMMCSession* sessP = iWorkSet.Remove();

	SchedDoAbort( sessP );

	if( sessP == iSessionP )
		iSessionP = NULL;

	if( sessP->iCardP != NULL && sessP->iCardP->iUsingSessionP == sessP )
		sessP->iCardP->iUsingSessionP = NULL;

	// Some sessions may attach to more than once card, so need to iterate 
	// through all cards and clear their session pointers if they match sessP
	if (sessP == &iAutoUnlockSession || 
		sessP->iSessionID == ECIMLockUnlock || 
		sessP->iSessionID == ECIMInitStackAfterUnlock)
		{
		for (TUint i = 0; i < iMaxCardsInStack; i++)
			{
			TMMCard& cd = *(iCardArray->CardP(i));
			if (cd.iUsingSessionP == sessP)
				cd.iUsingSessionP = NULL;
			}
		}

	if( sessP->iState & KMMCSessStateASSPEngaged )
		ASSPDisengage();

	sessP->iState = 0;
	OstTraceFunctionExit1( DMMCSTACK_SCHEDDISENGAGE_EXIT, this );
	}

inline DMMCStack::TMMCStackSchedStateEnum DMMCStack::SchedAbortPass()
/**
 * DMMCStack Scheduler private inline functions. These functions were separated as inline functions
 * only for the sake of Scheduler() clarity.
 */
	{
	OstTraceFunctionEntry1( DMMCSTACK_SCHEDABORTPASS_ENTRY, this );
	__KTRACE_OPT(KPBUS1,Kern::Printf("=mst:sap"));

	iAbortReq = EFalse;
	SchedGrabEntries();
	DMMCSession* sessP;

	iWorkSet.Point();

	while( (sessP = iWorkSet) != NULL )
		if( iAbortAll || sessP->iDoAbort )
			SchedDisengage();
		else
			iWorkSet++;

	iReadyQueue.Point();

	while( (sessP = iReadyQueue) != NULL )
		if( iAbortAll || sessP->iDoAbort )
			{
			iReadyQueue.Remove();
			sessP->iState = 0;
			}
		else
			iReadyQueue++;

	if( iAbortReq )
	    {
		OstTraceFunctionExitExt( DMMCSTACK_SCHEDABORTPASS_EXIT1, this, (TInt) ESchedLoop );
		return ESchedLoop;
	    }

	// Clearing iAbortAll here is a bit dodgy. It wouldn't work if somebody interrupted us
	// at this point, enqued a session and then immediately called Reset() - that session
	// would not be discarded. However, the correct solution (enque Reset() requests
	// and process them in the Scheduler main loop) seems to be too expensive just to avoid
	// this particular effect.
	iAbortAll = EFalse;
	OstTraceFunctionExitExt( DMMCSTACK_SCHEDABORTPASS_EXIT2, this, (TInt) ESchedContinue  );
	return ESchedContinue;
	}

inline DMMCStack::TMMCStackSchedStateEnum DMMCStack::SchedCompletionPass()
/**
 * This function calls back all the sessions waiting to be completed
 * Returns either Continue or Loop.
 */
	{
	OstTraceFunctionEntry1( DMMCSTACK_SCHEDCOMPLETIONPASS_ENTRY, this );
	__KTRACE_OPT(KPBUS1,Kern::Printf("=mst:scp"));

	iCompReq = EFalse;
	DMMCSession* sessP;

	if( iCompleteAllExitCode )
		{
		SchedGrabEntries();
		iWorkSet.Add( iReadyQueue );
		}

	iWorkSet.Point();

	while( (sessP = iWorkSet) != NULL )
		if( iCompleteAllExitCode || sessP->iDoComplete )
			{
			if( (EffectiveModes(sessP->iConfig) & KMMCModeCompleteInStackDFC) != 0 &&
				 SchedGetOnDFC() )
				{
				// DFC has been queued so return back to main loop.  Next time
				// SchedGetOnDfc() will return EFalse, and the callback will be called.
				iCompReq = ETrue;
				OstTraceFunctionExitExt( DMMCSTACK_SCHEDCOMPLETIONPASS_EXIT1, this, (TInt) ESchedLoop );
				return ESchedLoop;
				}

			SchedDisengage();					// calls iWorkSet.Remove
			sessP->iMMCExitCode |= iCompleteAllExitCode;
			// Update the controller store if a password operation was in progress.
			TBool doCallback = ETrue;
			if (sessP->iSessionID == ECIMLockUnlock)
				{
				iSocket->PasswordControlEnd(sessP, sessP->EpocErrorCode());
				
				if(sessP->EpocErrorCode() == KErrNone)
					{
					sessP->SetupCIMInitStackAfterUnlock();
					if(sessP->Engage() == KErrNone)
						{
						doCallback = EFalse;
						}
					}
				}

			if(sessP->iSessionID == ECIMInitStackAfterUnlock)
				{
				// After unlocking the stack, cards may have switched into HS mode
				// (HS switch commands are only valid when the card is unlocked).
				//
				// Therefore, we need to re-negotiate the maximum bus clock again.
				//
				// The PSL will use this to set the master config (limiting the clock if
				// appropriate).
				//
				// Note that the clock may change when a specific card is selected.
				//
				TUint maxClk;
				iCardArray->UpdateAcquisitions(&maxClk);
				SetBusConfigDefaults( iMasterConfig.iBusConfig, maxClk );
				DoSetClock(maxClk);
				}

			if(doCallback)
				{
				// Restore the callers card pointer as some state machines 
				// (e.g. ECIMLockUnlock, ECIMInitStackAfterUnlock) can change it
				sessP->RestoreCard();


				// call media driver completion routine or StackSessionCBST().
				sessP->iCallBack.CallBack();
				}
			}
		else
			iWorkSet++;

	if( iCompReq )
	    {
		OstTraceFunctionExitExt( DMMCSTACK_SCHEDCOMPLETIONPASS_EXIT2, this, (TInt) ESchedLoop );
		return ESchedLoop;
	    }

	iCompleteAllExitCode = 0;

	OstTraceFunctionExitExt( DMMCSTACK_SCHEDCOMPLETIONPASS_EXIT3, this, ( TInt) ESchedContinue );
	return ESchedContinue;
	}

inline DMMCStack::TMMCStackSchedStateEnum DMMCStack::SchedInitStack()
/**
 * "Immediate" InitStack initiator. Returns either Continue or Loop.
 */
	{
	OstTraceFunctionEntry1( DMMCSTACK_SCHEDINITSTACK_ENTRY, this );
	__KTRACE_OPT(KPBUS1,Kern::Printf("=mst:sis"));

	if( SchedGetOnDFC() )
	    {
		OstTraceFunctionExitExt( DMMCSTACK_SCHEDINITSTACK_EXIT1, this, (TInt) ESchedLoop );
		return ESchedLoop;
	    }

	if( iSessionP != NULL && (iStackState & KMMCStackStateJobChooser) == 0 )
		{
		if( (iSessionP->iState & KMMCSessStateInProgress) )
			{
			if( SchedGroundDown(iSessionP, KMMCErrPowerDown) )
				{
				MarkComplete( iSessionP, KMMCErrPowerDown );
				OstTraceFunctionExitExt( DMMCSTACK_SCHEDINITSTACK_EXIT2, this, (TInt) ESchedLoop );
				return ESchedLoop;
				}
			}
		else
			iSessionP->iMachine.Reset();
		}

	// NB if the current session was InitStack InProgress, JobChooser can not be active;
	// so we are not going to continue another InitStack as if nothing happened.

	iStackState &= ~(KMMCStackStateInitInProgress|KMMCStackStateInitPending);

	// If there is no current session (e.g. called from PowerUpStack()) or the current
	// session isn't specifically ECIMInitStack (which it rarely will be) then we have to use
	// the stack session to perform the stack init.
	if( iSessionP == NULL || iSessionP->iSessionID != ECIMInitStack )
		{
		if( SchedEnqueStackSession(ECIMInitStack) )
		    {
			OstTraceFunctionExitExt( DMMCSTACK_SCHEDINITSTACK_EXIT3, this, (TInt) ESchedLoop );
			return ESchedLoop;
		    }

		SchedSetContext( iStackSession );	// make the internal session to be current job
		}

	// Neither client nor internal session could be blocked here, not even on "BrokenLock"
	__ASSERT_ALWAYS( (iSessionP->iBlockOn)==0,
	DMMCSocket::Panic(DMMCSocket::EMMCInitStackBlocked) );

	iStackState |= KMMCStackStateInitInProgress;
	// nothing can stop this session now; it's safe to clear iInitialise here.
	iInitialise = EFalse;
	OstTraceFunctionExitExt( DMMCSTACK_SCHEDINITSTACK_EXIT4, this, (TInt) ESchedContinue );
	return ESchedContinue;
	}

inline DMMCStack::TMMCStackSchedStateEnum DMMCStack::SchedSleepStack()
/**
 * "Immediate" Stack sleep mode. Returns either Continue or Loop.
 */
	{
	OstTraceFunctionEntry1( DMMCSTACK_SCHEDSLEEPSTACK_ENTRY, this );
	__KTRACE_OPT(KPBUS1,Kern::Printf("=mst:SchdSlp!"));

	// Make sure Stack DFC is Running!
	if( SchedGetOnDFC() )
		{
		__KTRACE_OPT(KPBUS1, Kern::Printf("mst:SchdSlp - DFC not running"));
		OstTraceFunctionExitExt( DMMCSTACK_SCHEDSLEEPSTACK_EXIT1, this, (TInt) ESchedLoop );
		return ESchedLoop;
		}

	if( iSessionP != NULL && (iStackState & KMMCStackStateJobChooser) == 0 )
		{
		if( (iSessionP->iState & KMMCSessStateInProgress) )
			{
			// A session has been queued before sleep, 
			// cancel sleep and loop for next session
			iSleep = EFalse;
			OstTraceFunctionExitExt( DMMCSTACK_SCHEDSLEEPSTACK_EXIT2, this, (TInt) ESchedLoop );
			return ESchedLoop;
			}
		}
	
	// Use the stack session to perform the stack sleep.
	if( SchedEnqueStackSession(ECIMSleep) )
		{
		__KTRACE_OPT(KPBUS1,Kern::Printf("SchdSlp: already Enqued"));
		// Stack already busy cancel sleep
		iSleep = EFalse;
		OstTraceFunctionExitExt( DMMCSTACK_SCHEDSLEEPSTACK_EXIT3, this, (TInt) ESchedLoop );
		return ESchedLoop;
		}

	SchedSetContext( iStackSession );	// make the internal session to be current job

	// Sleep has now been queued
	iSleep = EFalse;
	iStackState |= KMMCStackStateSleepinProgress;
	__KTRACE_OPT(KPBUS1, Kern::Printf("<mst:SchdSlp"));
	
	OstTraceFunctionExitExt( DMMCSTACK_SCHEDSLEEPSTACK_EXIT4, this, (TInt) ESchedLoop );
	return ESchedLoop;
	}


inline TBool DMMCStack::SchedPreemptable()
/**
 * Checks if the current session can be preempted
 */
	{	// strictly in the following order
	OstTraceFunctionEntry1( DMMCSTACK_SCHEDPREEMPTABLE_ENTRY, this );
	__KTRACE_OPT(KPBUS1,Kern::Printf("=mst:spe"));
	
	if( (iStackState & KMMCStackStateJobChooser) ||
		(iSessionP->iState & KMMCSessStateDoReSchedule) )
	    {
		OstTraceFunctionExitExt( DMMCSTACK_SCHEDPREEMPTABLE_EXIT1, this, (TUint) ETrue );
		return ETrue;
	    }

	if( (iSessionP->iBlockOn & KMMCBlockOnASSPFunction) )
	    {
		OstTraceFunctionExitExt( DMMCSTACK_SCHEDPREEMPTABLE_EXIT2, this, (TUint) EFalse );
		return EFalse;
	    }

	TBool preemptDC = EFalse;	

	if (iSessionP->iBlockOn & KMMCBlockOnYielding)
		{
		// Added to support yielding the stack for a specific command.
		preemptDC = ETrue;		
		}
	else if( (iSessionP->iBlockOn & KMMCBlockOnDataTransfer) )
		{
		// Added for SDIO Read/Wait and SDC support.  This condition
		// is set at the variant, and determines whether commands may be
		// issued during the data transfer period.
		if(!(iSessionP->iState & KMMCSessStateAllowDirectCommands))
		    {
			OstTraceFunctionExitExt( DMMCSTACK_SCHEDPREEMPTABLE_EXIT3, this, (TUint) EFalse );
			return EFalse;
		    }
		
		// We must consider the remaining blocking conditions
		// before being sure that we can enable pre-emtion of this session
		preemptDC = ETrue;
		}

	if( (iSessionP->iBlockOn & (KMMCBlockOnCardInUse | KMMCBlockOnNoRun)) )
	    {
		OstTraceFunctionExitExt( DMMCSTACK_SCHEDPREEMPTABLE_EXIT4, this, (TUint) ETrue );
		return ETrue;
	    }
	
	if( (iConfig.iModes & KMMCModeEnablePreemption) == 0 )
	    {
		OstTraceFunctionExitExt( DMMCSTACK_SCHEDPREEMPTABLE_EXIT5, this, (TUint) EFalse );
		return EFalse;
	    }

	if( (iSessionP->iBlockOn & KMMCBlockOnGapTimersMask) &&
		(iConfig.iModes & KMMCModePreemptInGaps) &&
		(iSessionP->iState & KMMCSessStateSafeInGaps) )
	    {
		OstTraceFunctionExitExt( DMMCSTACK_SCHEDPREEMPTABLE_EXIT6, this, (TUint) ETrue );
		return ETrue;
	    }

	if( iSessionP->iBlockOn & KMMCBlockOnInterrupt )
	    {
		OstTraceFunctionExitExt( DMMCSTACK_SCHEDPREEMPTABLE_EXIT7, this, (TUint) ETrue );
		return ETrue;
	    }

	if(preemptDC)
	    {
		OstTraceFunctionExitExt( DDMMCSTACK_SCHEDPREEMPTABLE_EXIT8, this, (TUint) ETrue );
		return ETrue;
	    }
		
	OstTraceFunctionExitExt( DMMCSTACK_SCHEDPREEMPTABLE_EXIT9, this, (TUint) EFalse );
	return EFalse;
	}

inline DMMCStack::TMMCStackSchedStateEnum DMMCStack::SchedSession()
/**
 * Current context analyser. Returns Exit, Loop or ChooseJob.
 */
	{
	OstTraceFunctionEntry1( DMMCSTACK_SCHEDSESSION_ENTRY, this );

	__KTRACE_OPT(KPBUS1,Kern::Printf("=mst:ss"));

	// If no current session selected then we need to choose one
	if (iSessionP == NULL)
	    {
		OstTraceFunctionExitExt( DMMCSTACK_SCHEDSESSION_EXIT1, this, (TInt) ESchedChooseJob );
		return ESchedChooseJob;
	    }

	// Check any static blocking conditions on the current session and remove if possible
	if (SchedResolveStatBlocks(iSessionP)==ESchedLoop)
	    {
		OstTraceFunctionExitExt( DMMCSTACK_SCHEDSESSION_EXIT2, this, (TInt) ESchedLoop );
		return ESchedLoop;
	    }

	// If current session is still blocked, see if we could pre-empt the session
	if (iSessionP->iBlockOn)
		{
		if( SchedPreemptable() )
		    {
		    OstTraceFunctionExitExt( DMMCSTACK_SCHEDSESSION_EXIT3, this, (TInt) ESchedChooseJob );
			return ESchedChooseJob;
		    }

		OstTraceFunctionExitExt( DMMCSTACK_SCHEDSESSION_EXIT4, this, (TInt) ESchedExit );
		return ESchedExit;	// No preemption possible
		}

	// If the current session has been marked to be 'un-scheduled' then we
	// need to choose another session if ones available
	if ( (iSessionP->iState & KMMCSessStateDoReSchedule) )
	    {
		OstTraceFunctionExitExt( DMMCSTACK_SCHEDSESSION_EXIT5, this, (TInt) ESchedChooseJob );
		return ESchedChooseJob;
	    }

	// Check if this session requires to be run in DFC context - loop if necessary
	if ( (iSessionP->iState & KMMCSessStateDoDFC) )
		{
		iSessionP->iState &= ~KMMCSessStateDoDFC;
		if( SchedGetOnDFC()==ESchedLoop )
		    {
			OstTraceFunctionExitExt( DMMCSTACK_SCHEDSESSION_EXIT6, this, (TInt) ESchedLoop );
			return ESchedLoop;
		    }
		}

	// Now we actually execute the current session
	if( iLockingSessionP != NULL )
		{
		if( (iStackState & KMMCStackStateLocked) )
			{
			if( iSessionP != iLockingSessionP )
				{
				iLockingSessionP->iBrokenLock = ETrue;
				iLockingSessionP = NULL;
				DeselectsToIssue(KMMCIdleCommandsAtRestart); // use it for the number of deselects as well
				}
			}
		else
			if( iSessionP == iLockingSessionP )
				iStackState |= KMMCStackStateLocked;
		}

	if( iSessionP->iInitContext != iInitContext )
		{
		// If the current session's init_stack pass number is set but isn't the same as the current
		// pass number, it indicates this session is being resumed having tried to recover from
		// a bus inconsitency by re-initialising the stack. Set the exit code to a special
		// value so this session can un-wind from where the initial error occured, back to the start.
		if( iSessionP->iInitContext != 0 )
			//coverity[check_return]
			//return value is not saved or checked because there is no further uses.
			iSessionP->iMachine.SetExitCode(KMMCErrInitContext | iSessionP->iMachine.ExitCode());

		iSessionP->iInitContext = iInitContext;
		}

	iStackState &= ~KMMCStackStateJobChooser;
	iSessionP->iState &= ~KMMCSessStateSafeInGaps;

	// Execute the session state machine until it completes, is blocked or is aborted.
	TMMCErr exitCode = iSessionP->iMachine.Dispatch();

	iStackState &= ~KMMCStackStateReScheduled;

	if( exitCode )
		MarkComplete( iSessionP, (exitCode & ~KMMCErrBypass) );

	OstTraceFunctionExitExt( DMMCSTACK_SCHEDSESSION_EXIT7, this, (TInt) ESchedLoop );
	return ESchedLoop;
	}

TBool DMMCStack::SchedYielding(DMMCSession* aSessP)
/**
 * Check whether the scheduler should yield to another command
 */
	{
	OstTraceFunctionEntryExt( DMMCSTACK_SCHEDYIELDING_ENTRY, this );
	// Test whether a full loop through the sessions has occurred during a yield
	if ((aSessP->iBlockOn & KMMCBlockOnYielding) && (iStackState & KMMCStackStateYielding))
		{
		// We've looped, now stop yielding
		aSessP->iBlockOn &= ~KMMCBlockOnYielding;
		iStackState &= ~KMMCStackStateYielding;
		}
	TBool ret = (iStackState & KMMCStackStateYielding) != 0;
	OstTraceFunctionExitExt( DMMCSTACK_SCHEDYIELDING_EXIT, this, ret );
	return ret;
	}

TBool DMMCStack::SchedAllowDirectCommands(DMMCSession* aSessP)
/**
 * Check whether direct only commands can be run.
 */
	{
	OstTraceFunctionEntryExt( DMMCSTACK_SCHEDALLOWDIRECTCOMMANDS_ENTRY, this );
	TBool allowDirectCommands = EFalse;

	// Test the remaining sessions to see if they have a DMA data transfer blockage which allow direct commands only
	DMMCSession* testSessP = aSessP;
	do
		{
		if ((testSessP->iBlockOn & KMMCBlockOnDataTransfer) && (testSessP->iState & KMMCSessStateAllowDirectCommands))
			allowDirectCommands = ETrue;
		testSessP = testSessP->iLinkP;
		}			
	while((aSessP != testSessP) && (testSessP != NULL));

	OstTraceFunctionExitExt( DMMCSTACK_SCHEDALLOWDIRECTCOMMANDS_EXIT, this, allowDirectCommands );
	return allowDirectCommands;
	}

inline DMMCStack::TMMCStackSchedStateEnum DMMCStack::SchedChooseJob()
/**
 * Find an unblocked job to run. Returns Exit or Loop.
 */
	{
	OstTraceFunctionEntry1( DMMCSTACK_SCHEDCHOOSEJOB_ENTRY, this );
	__KTRACE_OPT(KPBUS1,Kern::Printf("=mst:scj"));

	iStackState |= KMMCStackStateJobChooser;
	SchedGrabEntries();
	DMMCSession* sessP = NULL;

	if( iLockingSessionP != NULL )		// if stack is already locked we accept only locking session
		{
		if( iWorkSet.IsEmpty() && iReadyQueue.Point(iLockingSessionP) )
			sessP = iReadyQueue.Remove();
		}
	else								// otherwise we might add a fresh session from reserve
		{
		iStackState &= ~KMMCStackStateLocked;
		if( iWorkSet.Size() < KMMCMaxJobsInStackWorkSet &&		// if work set is not too big
			!iReadyQueue.IsEmpty() &&							// and there are ready sessions
			(iStackState & KMMCStackStateWaitingToLock) == 0 )	// and nobody waits to lock us
			{
			iReadyQueue.Point();								// at marker to preserve FIFO
			sessP = iReadyQueue.Remove();
			}
		}

	if( sessP != NULL )
		{
		iWorkSet.Add( sessP );

		if( sessP->iSessionID == ECIMLockStack )
			{
			sessP->SynchBlock( KMMCBlockOnWaitToLock | KMMCBlockOnNoRun );
			sessP->iBrokenLock = EFalse;
			iStackState |= KMMCStackStateWaitingToLock;
			}
		}
	
	if( iSessionP  != NULL )
		iWorkSet.AdvanceMarker();		// move current session to the end of the queue

	iWorkSet.Point();
	
	while( (sessP = iWorkSet) != NULL )
		{
		// first, remove all static blocking conditions
		if( SchedResolveStatBlocks(sessP) )
		    {
			OstTraceFunctionExitExt( DMMCSTACK_SCHEDCHOOSEJOB_EXIT1, this, (TInt) ESchedLoop );
			return ESchedLoop;
		    }

		TBool scheduleSession = ETrue;
		// Test whether we are yielding 
		if (SchedYielding(sessP) && (sessP->Command().iSpec.iCommandType != iYieldCommandType))
			scheduleSession = EFalse;
		// Test whether this session is blocked
		else if (sessP->iBlockOn)
			scheduleSession = EFalse;
		// Test whether we can only handle direct commands
		else if (SchedAllowDirectCommands(sessP) && (sessP->Command().iSpec.iCommandType != ECmdTypeADC))
			scheduleSession = EFalse;
		
		if (scheduleSession)
			{
			iWorkSet.SetMarker();
			SchedSetContext( sessP );
			OstTraceFunctionExitExt( DMMCSTACK_SCHEDCHOOSEJOB_EXIT2, this, (TInt) ESchedLoop );
			return ESchedLoop;
			}
			
		iWorkSet++;
		}
	
	OstTraceFunctionExitExt( DMMCSTACK_SCHEDCHOOSEJOB_EXIT3, this, (TInt) ESchedExit );
	return ESchedExit;		
	}

void DMMCStack::StackDFC(TAny* aStackP)
/**
 * This DFC is used to startup Stack Scheduler from the background.
 */
	{
	OstTraceFunctionEntry0( DMMCSTACK_STACKDFC_ENTRY );
	__KTRACE_OPT(KPBUS1,Kern::Printf("=mst:sdf"));

	DMMCStack* const stackP = static_cast<DMMCStack*>(aStackP);
	stackP->Scheduler( stackP->iDFCRunning );
	OstTraceFunctionExit0( DMMCSTACK_STACKDFC_EXIT );
	}

void DMMCStack::Scheduler(volatile TBool& aFlag)
/**
 * This is the main function which controls, monitors and synchronises session execution.
 * It's divided into the entry function Scheduler() and the scheduling mechanism itself,
 * DoSchedule()
 */
	{
	OstTraceFunctionEntry0( DMMCSTACK_SCHEDULER_ENTRY );
	__KTRACE_OPT(KPBUS1,Kern::Printf("=mst:sch"));

	DISABLEPREEMPTION
	aFlag = ETrue;

	if( iStackState & KMMCStackStateRunning )
		{
		RESTOREPREEMPTION
		return;
		}

	iStackState |= KMMCStackStateRunning;
	RESTOREPREEMPTION
	DoSchedule();
	OstTraceFunctionExit0( DMMCSTACK_SCHEDULER_EXIT );
	}

void DMMCStack::DoSchedule()
	{
	OstTraceFunctionEntry1( DMMCSTACK_DOSCHEDULE_ENTRY, this );
	__KTRACE_OPT(KPBUS1,Kern::Printf(">mst:dos"));

	for(;;)
		{
		for(;;)
			{
			if( iAbortReq && SchedAbortPass() )
				continue;

			if( iDFCRunning )
				iStackState &= ~KMMCStackStateWaitingDFC;
			else
				if( iStackState & KMMCStackStateWaitingDFC )
					break;

			if( iCompReq && SchedCompletionPass() )
				continue;

			if( iInitialise && SchedInitStack() )
				continue;
						
			if( iSleep && SchedSleepStack() )
				continue;

			iAttention = EFalse;

			DMMCStack::TMMCStackSchedStateEnum toDo = SchedSession();

			if( toDo == ESchedLoop )
				continue;

			if( toDo == ESchedExit )
				break;

			if( SchedChooseJob() == ESchedExit )
				break;
			}

		DISABLEPREEMPTION

		if( !iAbortReq &&
			((iStackState & KMMCStackStateWaitingDFC) ||
			 (iCompReq | iInitialise | iAttention)==0) ||
			 ((iSessionP) && (iSessionP->iState & KMMCSessStateAllowDirectCommands)))
			{
			// Clear DFC flag here in case somebody was running scheduler in the background
			// when DFC turned up. This should never really happen, but with EPOC who knows
			iStackState &= ~KMMCStackStateRunning;
			iDFCRunning = EFalse;
			
			RESTOREPREEMPTION
			__KTRACE_OPT(KPBUS1,Kern::Printf("<mst:dos"));
			OstTraceFunctionExit1( DMMCSTACK_DOSCHEDULE_EXIT1, this );
			return;
			}

		RESTOREPREEMPTION
		}
	}

//
// DMMCStack:: --- Session service ---
//
void DMMCStack::Add(DMMCSession* aSessP)
/**
 * Adds session aSessP to the EntryQueue (asynchronous function)
 */
	{
	OstTraceFunctionEntryExt( DMMCSTACK_ADD_ENTRY, this );
	ASSERT_NOT_ISR_CONTEXT
	__KTRACE_OPT(KPBUS1,Kern::Printf(">MMC:Add %d",TUint(aSessP->iSessionID)));

	DISABLEPREEMPTION
	iEntryQueue.Add( aSessP );
	aSessP->iState |= KMMCSessStateEngaged;
	RESTOREPREEMPTION
	Scheduler( iAttention );
	OstTraceFunctionExit1( DMMCSTACK_ADD_EXIT, this );
	}

void DMMCStack::Abort(DMMCSession* aSessP)
/**
 * Aborts a session
 */
	{
	OstTraceFunctionEntryExt( DMMCSTACK_ABORT_ENTRY, this );
	ASSERT_NOT_ISR_CONTEXT
	__KTRACE_OPT(KPBUS1,Kern::Printf("=mst:abt"));

	if( !aSessP->IsEngaged() )
	    {
		OstTraceFunctionExit1( DMMCSTACK_ABORT_EXIT1, this );
		return;
	    }

	aSessP->iDoAbort = ETrue;
	aSessP->iMachine.Abort();

	Scheduler( iAbortReq );
	OstTraceFunctionExit1( DMMCSTACK_ABORT_EXIT2, this );
	}

void DMMCStack::Stop(DMMCSession* aSessP)
/**
 * Signals session to stop
 */
	{
	OstTraceFunctionEntryExt( DMMCSTACK_STOP1_ENTRY, this );
	ASSERT_NOT_ISR_CONTEXT
	__KTRACE_OPT(KPBUS1,Kern::Printf("=mst:stp"));

	if( !aSessP->IsEngaged() )
	    {
		OstTraceFunctionExit1( DMMCSTACK_STOP1_EXIT1, this );
		return;
	    }

	aSessP->iDoStop = ETrue;
	OstTraceFunctionExit1( DMMCSTACK_STOP1_EXIT2, this );
	}

EXPORT_C void DMMCStack::Block(DMMCSession* aSessP, TUint32 aFlag)
	{
	OstTraceFunctionEntryExt( DMMCSTACK_BLOCK_ENTRY, this );
	ASSERT_NOT_ISR_CONTEXT
	__KTRACE_OPT(KPBUS1,Kern::Printf("=mst:blk"));

	if( !aSessP->IsEngaged() )
	    {
		OstTraceFunctionExit1( DMMCSTACK_BLOCK_EXIT1, this );
		return;
	    }

	__KTRACE_OPT(KPBUS1,Kern::Printf("=mst:blk:[aFlag=%08x, iBlockOn=%08x]", aFlag, aSessP->iBlockOn));
	OstTraceExt2( TRACE_INTERNALS, DMMCSTACK_BLOCK, "aFlag=0x%08x; iBlockOn=0x%08x", aFlag, aSessP->iBlockOn );
	

	(void)__e32_atomic_ior_ord32(&aSessP->iBlockOn, aFlag);
	OstTraceFunctionExit1( DMMCSTACK_BLOCK_EXIT2, this );
	}

EXPORT_C void DMMCStack::UnBlock(DMMCSession* aSessP, TUint32 aFlag, TMMCErr anExitCode)
/**
 * aFlag is a bitset of KMMCBlockOnXXX events that have occured.  If the stack's
 * session is waiting on all of these events, then it is scheduled.
 */
	{
	OstTraceExt4(TRACE_FLOW, DMMCSTACK_UNBLOCK_ENTRY , "DMMCStack::UnBlock;aSessP=%x;aFlag=%x;anExitCode=%d;this=%x", (TUint) aSessP, (TUint) aFlag, (TInt) anExitCode, (TUint) this);
	__KTRACE_OPT(KPBUS1,Kern::Printf("=mst:ubl"));

	if (aSessP != NULL)
		{
		__KTRACE_OPT(KPBUS1,Kern::Printf("=mst:ubl:[aFlag=%08x, iBlockOn=%08x", aFlag, aSessP->iBlockOn));
		OstTraceExt2( TRACE_INTERNALS, DMMCSTACK_UNBLOCK, "aFlag=0x%08x; iBlockOn=0x%08x", aFlag, aSessP->iBlockOn );
		

		if( (aSessP->iBlockOn & aFlag) == 0 )
		    {
			OstTraceFunctionExit1( DMMCSTACK_UNBLOCK_EXIT1, this );
			return;
		    }

		// Must be either in a DFC or have the KMMCSessStateDoDFC flag set
		__ASSERT_DEBUG( 
			(aSessP->iState & KMMCSessStateDoDFC) != 0 || 
			NKern::CurrentContext() != NKern::EInterrupt,
			DMMCSocket::Panic(DMMCSocket::EMMCUnblockingInWrongContext));

		(void)__e32_atomic_and_ord32(&aSessP->iBlockOn, ~aFlag);
		aSessP->iMachine.SetExitCode( anExitCode );

		if( aSessP->iBlockOn == 0 )
			Scheduler( iAttention );
		}
	OstTraceFunctionExit1( DMMCSTACK_UNBLOCK_EXIT2, this );
	}

void DMMCStack::UnlockStack(DMMCSession* aSessP)
/**
 * Removes stack lock. Asynchronous function.
 */
	{
	OstTraceFunctionEntryExt( DMMCSTACK_UNLOCKSTACK_ENTRY, this );
	ASSERT_NOT_ISR_CONTEXT
	__KTRACE_OPT(KPBUS1,Kern::Printf("=mst:ust"));

	aSessP->iBrokenLock = EFalse;

	if( aSessP == iLockingSessionP )
		{
		iLockingSessionP = NULL;
		Scheduler( iAttention );
		}
	OstTraceFunctionExit1( DMMCSTACK_UNLOCKSTACK_EXIT1, this );
	}

EXPORT_C TInt DMMCStack::Stop(TMMCard* aCardP)
/**
 * Completes all sessions operating with a specified card with KMMCErrAbort.
 * Returns either KErrNone or KErrServerBusy.
 */
	{
	OstTraceFunctionEntryExt( DMMCSTACK_STOP2_ENTRY, this );
	ASSERT_NOT_ISR_CONTEXT
	__KTRACE_OPT(KPBUS1,Kern::Printf("=mst:stp"));

	DISABLEPREEMPTION

	if( iStackState & KMMCStackStateRunning )
		{
		RESTOREPREEMPTION
		return KErrServerBusy;	// can not operate in foreground
		}

	iStackState |= KMMCStackStateRunning;
	RESTOREPREEMPTION

	DMMCSession* sessP;
	SchedGrabEntries();

	iWorkSet.Point();

	while( (sessP = iWorkSet++) != NULL )
		if( sessP->iCardP == aCardP )
			MarkComplete( sessP, KMMCErrAbort );

	iReadyQueue.Point();

	while( (sessP = iReadyQueue) != NULL )
		if( sessP->iCardP == aCardP )
			{
			MarkComplete( sessP, KMMCErrAbort );
			iReadyQueue.Remove();
			iWorkSet.Add( sessP );
			}
		else
			iReadyQueue++;

	SchedGetOnDFC();
	DoSchedule();
	OstTraceFunctionExitExt( DMMCSTACK_STOP2_EXIT, this, KErrNone );
	return KErrNone;
	}

void DMMCStack::MarkComplete(DMMCSession* aSessP, TMMCErr anExitCode)
/**
 * Marks session to be completed on the next scheduler pass.
 */
	{
	OstTraceExt3(TRACE_FLOW, DMMCSTACK_MARKCOMPLETE_ENTRY ,"DMMCStack::MarkComplete;aSessP=%x;anExitCode=%d;this=%x", (TUint) aSessP, (TInt) anExitCode, (TUint) this);
	ASSERT_NOT_ISR_CONTEXT
	__KTRACE_OPT(KPBUS1,Kern::Printf("=mst:mcp"));

	aSessP->SynchBlock( KMMCBlockOnNoRun );
	aSessP->iMMCExitCode = anExitCode;
	aSessP->iDoComplete = ETrue;
	iCompReq = ETrue;
	OstTraceFunctionExit1( DMMCSTACK_MARKCOMPLETE_EXIT, this );
	}

//
// DMMCStack:: --- Miscellaneous ---
//
EXPORT_C TUint32 DMMCStack::EffectiveModes(const TMMCStackConfig& aClientConfig)
/**
 * Calculates effective client modes as real client modes merged with iMasterConfig modes
 */
	{
	OstTraceFunctionEntry1( DMMCSTACK_EFFECTIVEMODES_ENTRY, this );
	__KTRACE_OPT(KPBUS1,Kern::Printf("=mst:em"));

	const TUint32 masterMode = (iMasterConfig.iModes & iMasterConfig.iUpdateMask) |
								(KMMCModeDefault & ~iMasterConfig.iUpdateMask);

	const TUint32 c = aClientConfig.iClientMask;
	const TUint32 u = aClientConfig.iUpdateMask;
	const TUint32 m = aClientConfig.iModes;
	const TUint32 userMode = (c & ((m & u) | ~u)) | (m & KMMCModeMask);
	const TUint32 userMask = (u | KMMCModeClientMask) &
							((masterMode & KMMCModeMasterOverrides) | ~KMMCModeMasterOverrides);

	const TUint32 effectiveMode = (userMode & userMask) | (masterMode & ~userMask);

	if( effectiveMode & KMMCModeEnableClientConfig )
	    {
		OstTraceFunctionExitExt( DMMCSTACK_EFFECTIVEMODES_EXIT1, this, ( TUint )( effectiveMode ) );
		return effectiveMode;
	    }
	else
	    {
	    
		TUint32 ret = (effectiveMode & KMMCModeClientOverrides) |
				(masterMode & ~(KMMCModeClientOverrides | KMMCModeClientMask));
		OstTraceFunctionExitExt( DMMCSTACK_EFFECTIVEMODES_EXIT2, this, ( TUint )( ret ) );
		return ret;
	    }
	}

void DMMCStack::MergeConfig(DMMCSession* aSessP)
/**
 * Merges client and master configuration into iConfig
 */
	{
	OstTraceFunctionEntryExt( DMMCSTACK_MERGECONFIG_ENTRY, this );
	__KTRACE_OPT(KPBUS1,Kern::Printf("=mst:mc"));

	TMMCStackConfig& cC = aSessP->iConfig;
	TMMCStackConfig& mC = iMasterConfig;
	const TUint32 modes = EffectiveModes( cC );
	const TUint32 mastM = mC.iClientMask;

	iConfig.iModes = modes;

	iConfig.iPollAttempts =
			(modes & KMMCModeClientPollAttempts)
		?	cC.iPollAttempts
		:	((mastM & KMMCModeClientPollAttempts) ? mC.iPollAttempts : KMMCMaxPollAttempts);

	iConfig.iTimeOutRetries =
			(modes & KMMCModeClientTimeOutRetries)
		?	cC.iTimeOutRetries
		:	((mastM & KMMCModeClientTimeOutRetries) ? mC.iTimeOutRetries : KMMCMaxTimeOutRetries);

	iConfig.iCRCRetries =
			(modes & KMMCModeClientCRCRetries)
		?	cC.iCRCRetries
		:	((mastM & KMMCModeClientCRCRetries) ? mC.iCRCRetries : KMMCMaxCRCRetries);

	iConfig.iUnlockRetries =
			(modes & KMMCModeClientUnlockRetries)
		?	cC.iUnlockRetries
		:	((mastM & KMMCModeClientUnlockRetries) ? mC.iUnlockRetries : KMMCMaxUnlockRetries);
		
	iConfig.iOpCondBusyTimeout =
			(modes & KMMCModeClientiOpCondBusyTimeout)
		?	cC.iOpCondBusyTimeout
		:	((mastM & KMMCModeClientiOpCondBusyTimeout) ? mC.iOpCondBusyTimeout : KMMCMaxOpCondBusyTimeout);	

	// There are no default constants defining BusConfig parameters.
	// iMasterConfig.iBusConfig must be initialised by ASSP layer

	// _?_? The code below can be modified later for a card controlled session
	// to include CSD analisys and calculate time-out and clock parameters on that basis.
	// As it written now, the defaults for all cards will be the same.

	if( modes & KMMCModeClientBusClock )
		{
		TUint clock = cC.iBusConfig.iBusClock;
		if( clock > mC.iBusConfig.iBusClock )
			clock = mC.iBusConfig.iBusClock;
		if( clock < KMMCBusClockFOD )
			clock = KMMCBusClockFOD;
		DoSetClock(clock);
		}
	else if( modes & KMMCModeCardControlled && aSessP->CardP() )
		{
		TUint clock = MaxTranSpeedInKilohertz(*aSessP->CardP());
		if( clock > mC.iBusConfig.iBusClock )
			clock = mC.iBusConfig.iBusClock;
		if( clock < KMMCBusClockFOD )
			clock = KMMCBusClockFOD;
		DoSetClock(clock);
		}
	else
		DoSetClock(mC.iBusConfig.iBusClock);

	iConfig.iBusConfig.iClockIn = (modes & KMMCModeClientClockIn)
							? cC.iBusConfig.iClockIn
							: mC.iBusConfig.iClockIn;

	iConfig.iBusConfig.iClockOut = (modes & KMMCModeClientClockOut)
							? cC.iBusConfig.iClockOut
							: mC.iBusConfig.iClockOut;

	iConfig.iBusConfig.iResponseTimeOut = (modes & KMMCModeClientResponseTimeOut)
							? cC.iBusConfig.iResponseTimeOut
							: mC.iBusConfig.iResponseTimeOut;

	iConfig.iBusConfig.iDataTimeOut = (modes & KMMCModeClientDataTimeOut)
							? cC.iBusConfig.iDataTimeOut
							: mC.iBusConfig.iDataTimeOut;

	iConfig.iBusConfig.iBusyTimeOut = (modes & KMMCModeClientBusyTimeOut)
							? cC.iBusConfig.iBusyTimeOut
							: mC.iBusConfig.iBusyTimeOut;
	OstTraceFunctionExit1( DMMCSTACK_MERGECONFIG_EXIT, this );
	}

TBool DMMCStack::StaticBlocks()
/**
 * This function realises the potential blocking conditions of the current session.
 * Returns ETrue if the session has to be stopped right now
 */
	{
	OstTraceFunctionEntry1( DMMCSTACK_STATICBLOCKS_ENTRY, this );
	__KTRACE_OPT(KPBUS1,Kern::Printf("=mst:stb"));

	if( iSessionP->iDoStop )
		{
		MarkComplete( iSessionP, KMMCErrAbort );
		OstTraceFunctionExitExt( DMMCSTACK_STATICBLOCKS_EXIT1, this, (TUint) ETrue );
		return ETrue;
		}

	if( !iDFCRunning && (iSessionP->iState & KMMCSessStateDoDFC) )
	    {
		OstTraceFunctionExitExt( DMMCSTACK_STATICBLOCKS_EXIT2, this, (TUint) ETrue );
		return ETrue;
	    }

	TBool ret = (iSessionP->iState & KMMCSessStateDoReSchedule) != 0; 
	OstTraceFunctionExitExt( DMMCSTACK_STATICBLOCKS_EXIT3, this, ret );
	return ret;
	}


EXPORT_C TBool DMMCStack::CardDetect(TUint /*aCardNumber*/)
/**
 * Returns ETrue when a card is present in the card socket 'aCardNumber'.
 * Default implementation when not provided by ASSP layer.
 */
	{
	return(ETrue);
	}

EXPORT_C TBool DMMCStack::WriteProtected(TUint /*aCardNumber*/)
/**
 * Returns ETrue when the card in socket 'aCardNumber' is mechanically write
 * protected.
 * Default implementation when not provided by ASSP layer.
 */
	{
	return(EFalse);
	}

//	--------  DMMCStack State Machine functions  --------
//

// Auxiliary SM function service

void DMMCStack::StackSessionCBST(TAny* aStackP)
/**
 * Stack Session completion routine.
 */
	{
	__KTRACE_OPT(KPBUS1,Kern::Printf("=mst:sscbs"));
	static_cast<DMMCStack *>(aStackP)->StackSessionCB();
	}


TInt DMMCStack::StackSessionCB()
	{
	OstTraceFunctionEntry1( DMMCSTACK_STACKSESSIONCB_ENTRY, this );
	__KTRACE_OPT(KPBUS1,Kern::Printf("=mst:sscb "));

	if (iStackState & KMMCStackStateSleepinProgress)
		{
		// Sleep completed update stack state
		iStackState &= ~KMMCStackStateSleepinProgress;
		OstTraceFunctionExit1( DMMCSTACK_STACKSESSIONCB_EXIT1, this );
		return 0;
		}
	
	TMMCErr mmcError = iStackSession->MMCExitCode();
	iStackState &= ~KMMCStackStateInitInProgress;

	TInt  errCode   = KErrNone;
	TBool anyLocked = EFalse;

	if (mmcError != KMMCErrNone)
		{
		//
		// StackSessionCB is the completion callback for the internal initialisation/power-up
		// session, so we never expect a callback while initialisation is still in progress.
		// - unless a card has failed to respond and the controller has not detected the error (!)
		//
		errCode = KErrTimedOut;	// this error code is not sticky, so should allow stack to be powered up again
		iInitialise = EFalse;
		iStackState &= ~(KMMCStackStateInitInProgress | KMMCStackStateInitPending | KMMCStackStateBusInconsistent);
		}
	else
		{
		if (! iCardArray->CardsPresent())
			{
			errCode = KErrNotReady;
			}
		else
			{
			// Stack initialized ok, so complete request or start auto-unlock

			iInitState = EISDone;

			// remove bindings from password store for cards that do not have passwords
			TUint i;
			for (i = 0; i < iMaxCardsInStack; ++i)
				{
				TMMCard& cd = *(iCardArray->CardP(i));
				if (cd.IsPresent())
					{
					if (cd.HasPassword())
						anyLocked = ETrue;
					else
						{
						TMapping* pmp = iSocket->iPasswordStore->FindMappingInStore(cd.CID());
						if (pmp)
							pmp->iState = TMapping::EStInvalid;
						}
					}	// if (cd.IsPresent())
				}	// for (i = 0; i < iMaxCardsInStack; ++i)

			// if any cards are locked then launch auto-unlock mechanism
			if (anyLocked)
				{
				//
				// During power up (stack session context), we use the iAutoUnlockSession
				// to perform auto-unlock of the cards.  Upon completion of the AutoUnlock
				// state machine, the local media subsystem is notified via ::PowerUpSequenceComplete
				//
				iAutoUnlockSession.SetStack(this);
				iAutoUnlockSession.SetupCIMAutoUnlock();

				errCode = iAutoUnlockSession.Engage();
				if(errCode == KErrNone)
					{
					// don't complete power up request yet
					//  - This will be done in DMMCStack::AutoUnlockCB()
					OstTraceFunctionExit1( DMMCSTACK_STACKSESSIONCB_EXIT2, this );
					return 0;
					}
				}
			}	// else ( !iCardArray->CardsPresent() )
		}

	if(errCode == KErrNone)
		{
		//
		// No cards are locked (otherwise we will have engaged iAutoUnlockSession) and we
		// have encountered no error, so can now continue with the second-stage initialisation
		// phase (InitStackAfterUnlock).  This performs initialisation that can only be
		// performed when a card is unlocked (such as setting bus width, speed class etc..)
		//
		// iAutoUnlockSession::AutoUnlockCB will complete power-up by calling ::PowerUpSequenceComplete
		//
		iAutoUnlockSession.SetStack(this);
		iAutoUnlockSession.iCardP = NULL;
		iAutoUnlockSession.SetupCIMInitStackAfterUnlock();
		errCode = iAutoUnlockSession.Engage();
		}

	if(errCode != KErrNone)
		{
		//
		// We have encountered an error during power up initialisation
		//  - Complete the request and notify the local media subsystem.
		//

		// Calling PowerUpSequenceComplete() with an error may result in the media driver being closed which will delete
		// the media driver's session, so the stack must be made re-entrant here to allow all references to any engaged 
		// sessions to be removed from the stack immediately to prevent the stack from referencing a deleted object
		__ASSERT_ALWAYS(iStackState & KMMCStackStateRunning, DMMCSocket::Panic(DMMCSocket::EMMCNotInDfcContext));
		iStackState &= ~KMMCStackStateRunning;
		iSocket->PowerUpSequenceComplete(errCode);
		iStackState |= KMMCStackStateRunning;

		}

	OstTraceFunctionExit1( DMMCSTACK_STACKSESSIONCB_EXIT3, this );
	return 0;
	}

void DMMCStack::AutoUnlockCBST(TAny *aStackP)
	{
	__KTRACE_OPT(KPBUS1,Kern::Printf("=mst:aucbs"));

	static_cast<DMMCStack *>(aStackP)->AutoUnlockCB();
	}


TInt DMMCStack::AutoUnlockCB()
	{
	OstTraceFunctionEntry1( DMMCSTACK_AUTOUNLOCKCB_ENTRY, this );
	__KTRACE_OPT(KPBUS1,Kern::Printf("=mst:aucb"));

	// This is the session end callback for iAutoUnlockSession,
	// called at the end of the power up and initialisation process.

	TInt epocErr = iAutoUnlockSession.EpocErrorCode();

	// Calling PowerUpSequenceComplete() with an error may result in the media driver being closed which will delete
	// the media driver's session, so the stack must be made re-entrant here to allow all references to any engaged 
	// sessions to be removed from the stack immediately to prevent the stack from referencing a deleted object
	__ASSERT_ALWAYS(iStackState & KMMCStackStateRunning, DMMCSocket::Panic(DMMCSocket::EMMCNotInDfcContext));
	if (epocErr != KErrNone)
		iStackState &= ~KMMCStackStateRunning;
	iSocket->PowerUpSequenceComplete(epocErr);
	iStackState |= KMMCStackStateRunning;

	OstTraceFunctionExit1( DMMCSTACK_AUTOUNLOCKCB_EXIT, this );
	return 0;
	}


inline TMMCErr DMMCStack::AttachCardSM()
/**
 * This SM function must be invoked by every session which is CardControlled.
 *
 * Some commands require that the data held by the stack for a given card is up to date. 
 *
 * These are card controlled commands. Before such commands are issued, this function should 
 * first be invoked which performs the SEND_STATUS (CMD13) command. 
 *
 * @return MMC error code
 */
	{
	__KTRACE_OPT(KPBUS1,Kern::Printf("=mst:ac"));

		enum states
			{
			EStBegin=0,
			EStAttStatus,
			EStEnd
			};

		DMMCSession& s=Session();
		OstTrace1( TRACE_INTERNALS, DMMCSTACK_ATTACHCARDSM1, "Current session=0x%x", &s );

	SMF_BEGIN

		OstTrace0( TRACE_INTERNALS, DMMCSTACK_ATTACHCARDSM2, "EStBegin" );
		if( s.iCardP == NULL )
		    {
			OstTraceFunctionExitExt( DMMCSTACK_ATTACHCARDSM_EXIT1, this, (TInt) KMMCErrNoCard );
			return KMMCErrNoCard;
		    }

		if( s.iCardP->iUsingSessionP != NULL && s.iCardP->iUsingSessionP != &s )
			{
			s.SynchBlock( KMMCBlockOnCardInUse );
			SMF_WAIT
			}

		if( s.iCardP->IsPresent() && s.iCardP->iCID == s.iCID )
			s.iCardP->iUsingSessionP = &s;
		else
		    {
			OstTraceFunctionExitExt( DMMCSTACK_ATTACHCARDSM_EXIT2, this, (TInt) KMMCErrNoCard );
			return KMMCErrNoCard;
		    }

		s.iConfig.SetMode( KMMCModeCardControlled );	// for future context switching
		iConfig.SetMode( KMMCModeCardControlled );		// for this context

		// read card status if there are sticky bits in it
		if( (TUint32(s.iCardP->iStatus) & KMMCStatClearByReadMask) == 0 ||
			s.iCardP->iLastCommand == ECmdSendStatus ||
			s.iSessionID == ECIMNakedSession )
			SMF_EXIT

		CurrentSessPushCmdStack();
		s.FillCommandDesc( ECmdSendStatus, 0 );
		m.SetTraps( KMMCErrBasic );		// to restore command stack position to its original level
		SMF_INVOKES( ExecCommandSMST, EStAttStatus )

	SMF_STATE(EStAttStatus)

		OstTrace0( TRACE_INTERNALS, DMMCSTACK_ATTACHCARDSM3, "EStAttStatus" );
		CurrentSessPopCmdStack();
		OstTraceFunctionExitExt( DMMCSTACK_ATTACHCARDSM_EXIT3, this, (TInt) err );
		SMF_RETURN( err )

	SMF_END
	}

inline TMMCErr DMMCStack::CIMInitStackSM()
/**
 * Performs the Perform the CIM_INIT_STACK macro.
 * @return MMC error code
 */
	{
		enum states
			{
			EStBegin=0,
			EStInitDone,
			EStEnd
			};

		__KTRACE_OPT(KPBUS1,Kern::Printf(">MMC:InitStackSM"));

		DMMCSession& s=Session();
		OstTrace1( TRACE_INTERNALS, DMMCSTACK_CIMINITSTACKSM1, "Current session=0x%x", &s );

	SMF_BEGIN

		OstTrace0( TRACE_INTERNALS, DMMCSTACK_CIMINITSTACKSM2, "EStBegin" );
		m.SetTraps( KMMCErrAll );	// to prevent this macro from infinite restarts via iInitialise

		SMF_INVOKES( CIMUpdateAcqSMST, EStInitDone )

	SMF_STATE(EStInitDone)

		OstTrace0( TRACE_INTERNALS, DMMCSTACK_CIMINITSTACKSM3, "EStInitDone" );
		s.iState &= ~KMMCSessStateInProgress;	// now we won't be restarted
		SchedGetOnDFC();						// StackSessionCB must be on DFC
		OstTraceFunctionExitExt( DMMCSTACK_CIMINITSTACKSM_EXIT, this, (TInt) err );
		SMF_RETURN( err )						// _?_ power cycles can be performed here if error

	SMF_END
	}

TMMCErr DMMCStack::CIMUpdateAcqSM()
/**
 * Performs an identification of a card stack. New cards are always
 * initialised but if KMMCStackStateInitInProgress is FALSE then existing
 * cards keep their configuration.
 * After successful execution of this function, all cards will be in standby
 * state.
 * If iPoweredUp is FALSE then the stack is powered up and a full INIT_STACK 
 * is performed (i.e all cards set to idle and then initialized). 
 * @return MMC error code
 */
	{
		enum states
			{
			EStBegin=0,
			EStPoweredUp,
			EStClockOn,
			EStStartInterrogation,
			EStCheckStack,
            EStCardCap,
			EStIssueDSR,
			EStFinishUp,
			EStEnd
			};

		__KTRACE_OPT(KPBUS1,Kern::Printf(">MMC:UpdAcqSM"));

		DMMCSession& s=Session();
		DMMCPsu* psu=(DMMCPsu*)iSocket->iVcc;
		OstTrace1( TRACE_INTERNALS, DMMCSTACK_CIMUPDATEACQSM1, "Current session=0x%x", &s );

	SMF_BEGIN

		OstTrace0( TRACE_INTERNALS, DMMCSTACK_CIMUPDATEACQSM2, "EStBegin" );
		// This macro works naked and must not be preempted
		iConfig.RemoveMode( KMMCModeEnablePreemption | KMMCModeCardControlled );
		// Ensure DFC is running before and after powering up
		if( SchedGetOnDFC() )	// Such a direct synchronisation with Scheduler() can only
			SMF_WAIT			// be used in this macro

		s.iState |= (KMMCSessStateInProgress | KMMCSessStateCritical);

		if (iPoweredUp)
			SMF_GOTOS( EStPoweredUp )

		// The bus is not powered so all cards need initialising - enforce INIT_STACK.
		iStackState |= KMMCStackStateInitInProgress;

		// Need to turn on the PSU at it's default voltage. Let the ASSP layer choose
		// this voltage by calling SetVoltage() with the full range the ASSP supports. 
		iCurrentOpRange=(psu->VoltageSupported() & ~KMMCAdjustableOpVoltage);
		psu->SetVoltage(iCurrentOpRange);
		SMF_INVOKES( DoPowerUpSMST, EStPoweredUp )

	SMF_STATE(EStPoweredUp)

		OstTrace0( TRACE_INTERNALS, DMMCSTACK_CIMUPDATEACQSM3, "EStPoweredUp" );
		// Switch on the bus clock in identification mode
		SetBusConfigDefaults(iMasterConfig.iBusConfig, KMMCBusClockFOD);
		DoSetClock(KMMCBusClockFOD);

		// Make sure controller is in 1-bit bus width mode
		DoSetBusWidth(EBusWidth1);

		MergeConfig(&s);	// This might take some time, but we are running in DFC here
		// Reinstate config bits after the merge
		iConfig.RemoveMode( KMMCModeEnablePreemption | KMMCModeCardControlled );
		SMF_INVOKES( InitClockOnSMST, EStClockOn )	// Feed init clock to the bus

	SMF_STATE(EStClockOn)

		OstTrace0( TRACE_INTERNALS, DMMCSTACK_CIMUPDATEACQSM4, "EStClockOn" );
		// Check if there are any cards present in the stack
		if (!HasCardsPresent())
			SMF_GOTOS( EStCheckStack )

		if( !InitStackInProgress() )
			SMF_GOTOS( EStStartInterrogation )

		// Increment the stack's initialiser pass number. Set the current session's pass
		// number to the new value. Pass number may be used later on to detect sessions
		// which have been re-initialized due to problems on the bus. 
		if ((++iInitContext) == 0)		
			iInitContext++;				// Pass number must never be zero
		s.iInitContext = iInitContext;	// this session is always in a proper context

	SMF_STATE(EStStartInterrogation)

		OstTrace0( TRACE_INTERNALS, DMMCSTACK_CIMUPDATEACQSM5, "EStStartInterrogation" );
		// NB: RCAs are not unlocked here. They will be unlocked one by one during the update of card info array.
		SMF_INVOKES( AcquireStackSMST, EStCheckStack )

	SMF_STATE(EStCheckStack)

		OstTrace0( TRACE_INTERNALS, DMMCSTACK_CIMUPDATEACQSM6, "EStCheckStack" );
		// Check that all known cards are still present by issuing select/deselect
		SMF_INVOKES( CheckStackSMST, EStCardCap )

	SMF_STATE(EStCardCap)

		OstTrace0( TRACE_INTERNALS, DMMCSTACK_CIMUPDATEACQSM7, "EStCardCap" );
		// Call a licencee-specific state machine to allow card capabilities to be modified.
		SMF_INVOKES( ModifyCardCapabilitySMST, EStIssueDSR )

	SMF_STATE(EStIssueDSR)

		// Now that we have updated the card entries, do any final initialisation
		// of the card entries and determine the maximum bus clock that can be employed.
		//
		// If the bus is not multiplexed (ie - MMC stack), then the max clock is set to
		// the lowest common denominator of all cards in the stack.  Otherwise (in the case
		// of a multiplexed bus such as SD), the highest clock is returned and the clock
		// rate is changed when a new card is selected.
		//
		OstTrace0( TRACE_INTERNALS, DMMCSTACK_CIMUPDATEACQSM8, "EStIssueDSR" );
		TUint maxClk;
		iCardArray->UpdateAcquisitions(&maxClk);
		SetBusConfigDefaults( iMasterConfig.iBusConfig, maxClk );
		DoSetClock(maxClk);

		// merge clock from iMasterConfig.iBusConfig.iBusClock to 
		// iConfig.iBusConfig.iBusClock - which the PSL should use to configure it's clock
		MergeConfig(&s);	

		// switch to normal iConfig clock mode
		InitClockOff();
		
	SMF_STATE(EStFinishUp)

		OstTrace0( TRACE_INTERNALS, DMMCSTACK_CIMUPDATEACQSM9, "EStFinishUp" );
		s.iState &= ~(KMMCSessStateInProgress | KMMCSessStateCritical);

		// Update/Init stack has been completed. 

	SMF_END
	}


#define MHZ_TO_KHZ(valInMhz) ((valInMhz) * 1000)

EXPORT_C TMMCErr DMMCStack::InitStackAfterUnlockSM()
/**
 * Perform last-stage initialisation of the MMC card.
 * This implements initialiation that must occur only when the card
 * is unlocked (ie - immediately after unlocking, or during initialisation
 * if the card is already unlocked)
 */
	{
		enum states
			{
			EStBegin=0,
			EStTestNextCard,
			EStGetExtendedCSD,
			EStGotExtendedCSD,
			EStGotModifiedExtendedCSD,
			EStEraseGroupDefSet,
			EStDetermineBusWidthAndClock,
			EStGotBusWidthAndClock,	
			EStNoMoreCards,
			EStExit,
			EStEnd
			};

	DMMCSession& s = Session();
	TBool initSingleCard = (s.CardP() == NULL) ? (TBool)EFalse : (TBool)ETrue;
	OstTrace1( TRACE_INTERNALS, DMMCSTACK_INITSTACKAFTERUNLOCKSM1, "Current session=0x%x", &s );

	SMF_BEGIN

		OstTrace0( TRACE_INTERNALS, DMMCSTACK_INITSTACKAFTERUNLOCKSM2, "EStBegin" );
		if(initSingleCard)
			{
			iSelectedCardIndex = iCxCardCount;
			TMMCard* cardP = iCardArray->CardP(iSelectedCardIndex);

			// if card not present or is locked, exit
			if ((!cardP->IsPresent()) || (cardP->IsLocked()))
				SMF_GOTOS(EStExit);

			s.SetCard(cardP);

			// If a card is currently indexed for initialisation, then only configure this one.  
			// We assume that this has been called from the SD stack, so only one MMC card will be present on the bus

			SMF_GOTOS(EStGetExtendedCSD);
			}

		// Initialising the entire MMC stack - start with the first card
		iSelectedCardIndex = -1;
		
		// ...fall through...

	SMF_STATE(EStTestNextCard)

		OstTrace0( TRACE_INTERNALS, DMMCSTACK_INITSTACKAFTERUNLOCKSM3, "EStTestNextCard" );

		// any more cards ?
		if (++iSelectedCardIndex >= iCxCardCount)
			SMF_GOTOS(EStNoMoreCards);

		// if no card in this slot, try next one
		if (!iCardArray->CardP(iSelectedCardIndex)->IsPresent())
			SMF_GOTOS(EStTestNextCard);

		TMMCard* cardP = iCardArray->CardP(iSelectedCardIndex);
		s.SetCard(cardP);

		// if card is locked, try the next one
		if(cardP->IsLocked())
			SMF_GOTOS(EStTestNextCard);

	SMF_STATE(EStGetExtendedCSD)

		OstTrace0( TRACE_INTERNALS, DMMCSTACK_INITSTACKAFTERUNLOCKSM4, "EStGetExtendedCSD" );
	
		// Get the Extended CSD if this is an MMC version 4 card

		__KTRACE_OPT(KPBUS1, Kern::Printf(">ConfigureHighSpeed(), SpecVers() %u", s.CardP()->CSD().SpecVers()));
		OstTrace1( TRACE_INTERNALS, DMMCSTACK_INITSTACKAFTERUNLOCKSM5, "SpecVers()=%u", s.CardP()->CSD().SpecVers() );
		

		// clear the Extended CSD contents in case this is a pre-version 4 card or the read fails.
		memset(s.CardP()->iExtendedCSD.Ptr(), 0, KMMCExtendedCSDLength);

		if (s.CardP()->CSD().SpecVers() < 4) 
			SMF_GOTOS(EStTestNextCard);

		m.SetTraps(KMMCErrResponseTimeOut | KMMCErrStatus | KMMCErrDataCRC | KMMCErrBypass);	// KMMCErrDataCRC will pick up if the card is not in 1-bit mode

		CurrentSessFillCmdDesc(ECmdSendExtendedCSD);
		CurrentSessFillCmdArgs(0, KMMCExtendedCSDLength, iPSLBuf, KMMCExtendedCSDLength);

		__KTRACE_OPT(KPBUS1, Kern::Printf(">ConfigureHighSpeed(), Sending ECmdSendExtendedCSD"));
		SMF_INVOKES(CIMReadWriteBlocksSMST, EStGotExtendedCSD)

	SMF_STATE(EStGotExtendedCSD)
		
		OstTrace0( TRACE_INTERNALS, DMMCSTACK_INITSTACKAFTERUNLOCKSM6, "EStGotExtendedCSD" );
		if (err != KMMCErrNone)
			{
			SMF_GOTOS(EStExit);
			}

		memcpy(s.CardP()->iExtendedCSD.Ptr(), iPSLBuf, KMMCExtendedCSDLength);

		// Call a licencee-specific state machine to allow the Extended CSD register to be modified.
		SMF_INVOKES( ModifyCardCapabilitySMST, EStGotModifiedExtendedCSD )

	SMF_STATE(EStGotModifiedExtendedCSD)

		OstTrace0( TRACE_INTERNALS, DMMCSTACK_INITSTACKAFTERUNLOCKSM7, "EStGotExtendedCSD" );
	
		__KTRACE_OPT(KPBUS1, Kern::Printf("Extended CSD"));
		__KTRACE_OPT(KPBUS1, Kern::Printf("CSDStructureVer:            %u", s.CardP()->ExtendedCSD().CSDStructureVer()));
		__KTRACE_OPT(KPBUS1, Kern::Printf("ExtendedCSDRev:             %u", s.CardP()->ExtendedCSD().ExtendedCSDRev()));
		__KTRACE_OPT(KPBUS1, Kern::Printf("-------------------------------"));
		__KTRACE_OPT(KPBUS1, Kern::Printf("SupportedCmdSet:            %u", s.CardP()->ExtendedCSD().SupportedCmdSet()));
		__KTRACE_OPT(KPBUS1, Kern::Printf("PowerClass26Mhz360V:        0x%02X", s.CardP()->ExtendedCSD().PowerClass26Mhz360V()));
		__KTRACE_OPT(KPBUS1, Kern::Printf("PowerClass52Mhz360V:        0x%02X", s.CardP()->ExtendedCSD().PowerClass52Mhz360V()));
		__KTRACE_OPT(KPBUS1, Kern::Printf("PowerClass26Mhz195V:        0x%02X", s.CardP()->ExtendedCSD().PowerClass26Mhz195V()));
		__KTRACE_OPT(KPBUS1, Kern::Printf("PowerClass52Mhz195V:        0x%02X", s.CardP()->ExtendedCSD().PowerClass52Mhz195V()));
		__KTRACE_OPT(KPBUS1, Kern::Printf("CardType:                   %u", s.CardP()->ExtendedCSD().CardType()));
		__KTRACE_OPT(KPBUS1, Kern::Printf("CmdSet:                     %u", s.CardP()->ExtendedCSD().CmdSet()));
		__KTRACE_OPT(KPBUS1, Kern::Printf("CmdSetRev:                  %u", s.CardP()->ExtendedCSD().CmdSetRev()));
		__KTRACE_OPT(KPBUS1, Kern::Printf("PowerClass:                 %u", s.CardP()->ExtendedCSD().PowerClass()));
		__KTRACE_OPT(KPBUS1, Kern::Printf("HighSpeedTiming:            %u", s.CardP()->ExtendedCSD().HighSpeedTiming()));
		__KTRACE_OPT(KPBUS1, Kern::Printf("HighCapacityEraseGroupSize: %u", s.CardP()->ExtendedCSD().HighCapacityEraseGroupSize()));
		__KTRACE_OPT(KPBUS1, Kern::Printf("AccessSize:                 %u", s.CardP()->ExtendedCSD().AccessSize()));
		__KTRACE_OPT(KPBUS1, Kern::Printf("BootInfo:                   %u", s.CardP()->ExtendedCSD().BootInfo() ));
		__KTRACE_OPT(KPBUS1, Kern::Printf("BootSizeMultiple:           %u", s.CardP()->ExtendedCSD().BootSizeMultiple() ));
		__KTRACE_OPT(KPBUS1, Kern::Printf("EraseTimeoutMultiple:       %u", s.CardP()->ExtendedCSD().EraseTimeoutMultiple() ));
		__KTRACE_OPT(KPBUS1, Kern::Printf("ReliableWriteSector:        %u", s.CardP()->ExtendedCSD().ReliableWriteSector() ));
		__KTRACE_OPT(KPBUS1, Kern::Printf("HighCapWriteProtGroupSize:  %u", s.CardP()->ExtendedCSD().HighCapacityWriteProtectGroupSize() ));
		__KTRACE_OPT(KPBUS1, Kern::Printf("SleepCurrentVcc:            %u", s.CardP()->ExtendedCSD().SleepCurrentVcc() ));
		__KTRACE_OPT(KPBUS1, Kern::Printf("SleepCurrentVccQ:           %u", s.CardP()->ExtendedCSD().SleepCurrentVccQ()));
		__KTRACE_OPT(KPBUS1, Kern::Printf("SleepAwakeTimeout:          %u", s.CardP()->ExtendedCSD().SleepAwakeTimeout()));
		__KTRACE_OPT(KPBUS1, Kern::Printf("BootConfig:                 %u", s.CardP()->ExtendedCSD().BootConfig()));
		__KTRACE_OPT(KPBUS1, Kern::Printf("BootBusWidth:               %u", s.CardP()->ExtendedCSD().BootBusWidth()));
		__KTRACE_OPT(KPBUS1, Kern::Printf("EraseGroupDef:              %u", s.CardP()->ExtendedCSD().EraseGroupDef()));
		
		OstTraceDefExt3( OST_TRACE_CATEGORY_RND, TRACE_MMCDEBUG, DMMCSTACK_INITSTACKAFTERUNLOCKSM8, "CSDStructureVer=%u; ExtendedCSDRev=%u; SupportedCmdSet=%u", s.CardP()->ExtendedCSD().CSDStructureVer(), s.CardP()->ExtendedCSD().ExtendedCSDRev(), s.CardP()->ExtendedCSD().SupportedCmdSet() );
		OstTraceDefExt4( OST_TRACE_CATEGORY_RND, TRACE_MMCDEBUG, DMMCSTACK_INITSTACKAFTERUNLOCKSM9, "PowerClass26Mhz360V=0x%02x; PowerClass52Mhz360V=0x%02x; PowerClass26Mhz195V=0x%02x; PowerClass52Mhz195V=0x%02x", s.CardP()->ExtendedCSD().PowerClass26Mhz360V(), s.CardP()->ExtendedCSD().PowerClass52Mhz360V(), s.CardP()->ExtendedCSD().PowerClass26Mhz195V(), s.CardP()->ExtendedCSD().PowerClass52Mhz195V() );
		OstTraceDefExt5( OST_TRACE_CATEGORY_RND, TRACE_MMCDEBUG, DMMCSTACK_INITSTACKAFTERUNLOCKSM10, "CardType=%u; CmdSet=%u; CmdSetRev=%u; PowerClass=%u; HighSpeedTiming=%u", s.CardP()->ExtendedCSD().CardType(), s.CardP()->ExtendedCSD().CmdSet(), s.CardP()->ExtendedCSD().CmdSetRev(), s.CardP()->ExtendedCSD().PowerClass(), s.CardP()->ExtendedCSD().HighSpeedTiming() );
		OstTraceDefExt5( OST_TRACE_CATEGORY_RND, TRACE_MMCDEBUG, DMMCSTACK_INITSTACKAFTERUNLOCKSM11, "HighCapacityEraseGroupSize=%u; AccessSize=%u; BootInfo=%u; BootSizeMultiple=%u; EraseTimeoutMultiple=%u", s.CardP()->ExtendedCSD().HighCapacityEraseGroupSize(), s.CardP()->ExtendedCSD().AccessSize(), s.CardP()->ExtendedCSD().BootInfo(), s.CardP()->ExtendedCSD().BootSizeMultiple(), s.CardP()->ExtendedCSD().EraseTimeoutMultiple() );
		OstTraceDefExt5( OST_TRACE_CATEGORY_RND, TRACE_MMCDEBUG, DMMCSTACK_INITSTACKAFTERUNLOCKSM12, "ReliableWriteSector=%u; HighCapWriteProtGroupSize=%u; SleepCurrentVcc=%u; SleepCurrentVccQ=%u; SleepAwakeTimeout=%u", s.CardP()->ExtendedCSD().ReliableWriteSector(), s.CardP()->ExtendedCSD().HighCapacityWriteProtectGroupSize(), s.CardP()->ExtendedCSD().SleepCurrentVcc(), s.CardP()->ExtendedCSD().SleepCurrentVccQ(), s.CardP()->ExtendedCSD().SleepAwakeTimeout() );
		OstTraceDefExt3( OST_TRACE_CATEGORY_RND, TRACE_MMCDEBUG, DMMCSTACK_INITSTACKAFTERUNLOCKSM13, "BootConfig=%u; BootBusWidth=%u; EraseGroupDef=%u", s.CardP()->ExtendedCSD().BootConfig(), s.CardP()->ExtendedCSD().BootBusWidth(), s.CardP()->ExtendedCSD().EraseGroupDef() );
		
		if (s.CardP()->ExtendedCSD().ExtendedCSDRev() >= 3)
			{
			if (!(s.CardP()->ExtendedCSD().EraseGroupDef()) && s.CardP()->ExtendedCSD().HighCapacityEraseGroupSize())
				{
				// Need to ensure that media is using correct erase group sizes.
				TMMCArgument arg = TExtendedCSD::GetWriteArg(
					TExtendedCSD::EWriteByte,
					TExtendedCSD::EEraseGroupDefIndex,
					TExtendedCSD::EEraseGrpDefEnableHighCapSizes,
					0);
	
				__KTRACE_OPT(KPBUS1, Kern::Printf(">Writing to EXT_CSD (EEraseGroupDefIndex), arg %08X", (TUint32) arg));
				OstTrace1( TRACE_INTERNALS, DMMCSTACK_INITSTACKAFTERUNLOCKSM14, "Writing to EXT_CSD (EEraseGroupDefIndex); arg=0x%08x", (TUint32) arg );
				
				s.FillCommandDesc(ECmdSwitch, arg);
				
				SMF_INVOKES(ExecSwitchCommandST, EStEraseGroupDefSet)
				}
			}
		
		SMF_GOTOS(EStDetermineBusWidthAndClock)
		
	SMF_STATE(EStEraseGroupDefSet)
	
		OstTrace0( TRACE_INTERNALS, DMMCSTACK_INITSTACKAFTERUNLOCKSM15, "EStEraseGroupDefSet" );
	
		if (err == KMMCErrNone)
			{
			// EEraseGroupDef has been updated succussfully, 
			// update the Extended CSD to reflect this			
			memset( s.CardP()->iExtendedCSD.Ptr()+TExtendedCSD::EEraseGroupDefIndex, TExtendedCSD::EEraseGrpDefEnableHighCapSizes, 1);
			}

	// Fall through to the next state
	SMF_STATE(EStDetermineBusWidthAndClock)
	
		OstTrace0( TRACE_INTERNALS, DMMCSTACK_INITSTACKAFTERUNLOCKSM16, "EStDetermineBusWidthAndClock" );
		SMF_INVOKES( DetermineBusWidthAndClockSMST, EStGotBusWidthAndClock )

    SMF_STATE(EStGotBusWidthAndClock)

		OstTrace0( TRACE_INTERNALS, DMMCSTACK_INITSTACKAFTERUNLOCKSM17, "EStGotBusWidthAndClock" );
		SMF_NEXTS(initSingleCard ? EStExit : EStTestNextCard)

		if(iMultiplexedBus || iCardArray->CardsPresent() == 1)
			{
			SMF_CALL( ConfigureHighSpeedSMST )
			}
		
		SMF_GOTONEXTS

	SMF_STATE(EStNoMoreCards)

		OstTrace0( TRACE_INTERNALS, DMMCSTACK_INITSTACKAFTERUNLOCKSM18, "EStNoMoreCards" );

	// Fall through to the next state
	SMF_STATE(EStExit)
	
		OstTrace0( TRACE_INTERNALS, DMMCSTACK_INITSTACKAFTERUNLOCKSM19, "EStExit" );
		m.ResetTraps();

	SMF_END
	}



/**
DetermineBusWidthAndClockSM()

Reads the extended CSD register for all MMCV4 cards in the stack.
If there is only one MMCV4 card, then an attempt is made to switch
both the card and the controller to a higher clock rate (either 26MHz of 52MHz) 
and to a wider bus width (4 or 8 bits). 
The clock speed & bus width chosen depend on :

- whether the card supports it
- whether the controller supports it
- whether the controller has the ability to supply enough current (the current used 
  by the card can be read from the Extended CSD register)

*/
TMMCErr DMMCStack::DetermineBusWidthAndClockSM()
	{
		enum states
			{
			EStBegin=0,
			EStWritePowerClass,
			EStStartBusTest,
			EStExit,
			EStEnd
			};

	DMMCSession& s = Session();
	TMMCard* cardP = iCardArray->CardP(iSelectedCardIndex);
	OstTrace1( TRACE_INTERNALS, DMMCSTACK_DETERMINEBUSWIDTHANDCLOCKSM1, "Current session=0x%x", &s );

	SMF_BEGIN
	
		OstTrace0( TRACE_INTERNALS, DMMCSTACK_DETERMINEBUSWIDTHANDCLOCKSM2, "EStBegin" );
		// Trap Switch errors & no-response errors
		m.SetTraps(KMMCErrResponseTimeOut | KMMCErrStatus);

		__KTRACE_OPT(KPBUS1, Kern::Printf(">ConfigureHighSpeed(), iCxCardCount %u", iCxCardCount));


	SMF_STATE(EStWritePowerClass)

		OstTrace0( TRACE_INTERNALS, DMMCSTACK_DETERMINEBUSWIDTHANDCLOCKSM3, "EStWritePowerClass" );
	
		// Check the card type is valid
		if (!(cardP->iExtendedCSD.IsSupportedCardType()))
			{            
			__KTRACE_OPT(KPBUS1, Kern::Printf("Unsupported card type %u", cardP->iExtendedCSD.CardType()));
			OstTrace1( TRACE_INTERNALS, DMMCSTACK_DETERMINEBUSWIDTHANDCLOCKSM4, "Unsupported card type=%u", cardP->iExtendedCSD.CardType() );
			
			SMF_GOTOS(EStExit);
			}

		// determine the optimum bus width & clock speed which match the power constraints
		TUint powerClass;
		DetermineBusWidthAndClock(
			*cardP, 
			(iCurrentOpRange == KMMCOCRLowVoltage), 
			powerClass, 
			iBusWidthAndClock);

		// If the power class for the chosen width is different from the default,
		// send SWITCH cmd and write the POWER_CLASS byte of the EXT_CSD register
		if (powerClass > 0)
			{
			TMMCArgument arg = TExtendedCSD::GetWriteArg(
				TExtendedCSD::EWriteByte,
				TExtendedCSD::EPowerClassIndex,
				powerClass,
				0);

			__KTRACE_OPT(KPBUS1, Kern::Printf(">ConfigureHighSpeed(), Writing to EXT_CSD (EPowerClass), arg %08X", (TUint32) arg));
			OstTrace1( TRACE_INTERNALS, DMMCSTACK_DETERMINEBUSWIDTHANDCLOCKSM5, "Writing to EXT_CSD (EPowerClass); arg=0x%08x", (TUint32) arg );
			s.FillCommandDesc(ECmdSwitch, arg);
			SMF_INVOKES(ExecSwitchCommandST, EStStartBusTest)
			}

	SMF_STATE(EStStartBusTest)
		
        OstTrace0( TRACE_INTERNALS, DMMCSTACK_DETERMINEBUSWIDTHANDCLOCKSM6, "EStStartBusTest" );
        	
		if (err != KMMCErrNone)
			{
			SMF_GOTOS(EStExit);
			}

		// We have determined the capabilities of the host and card.
		//  - Before switching to the required bus width, perform the BUSTEST sequence
		SMF_INVOKES(ExecBusTestSMST, EStExit);

	SMF_STATE(EStExit)
	
		OstTrace0( TRACE_INTERNALS, DMMCSTACK_DETERMINEBUSWIDTHANDCLOCKSM7, "EStExit" );
		m.ResetTraps();

	SMF_END
	}


/**
ConfigureHighSpeedSM()

Reads the extended CSD register for all MMCV4 cards in the stack.
If there is only one MMCV4 card, then an attempt is made to switch
both the card and the controller to a higher clock rate (either 26MHz of 52MHz) 
and to a wider bus width (4 or 8 bits). 
The clock speed & bus width chosen depend on :

- whether the card supports it
- whether the controller supports it
- whether the controller has the ability to supply enough current (the current used 
  by the card can be read from the Extended CSD register)

*/
TMMCErr DMMCStack::ConfigureHighSpeedSM()
	{
		enum states
			{
			EStBegin=0,
			EStConfigureBusWidth,
			EStWriteHsTiming,
			EStConfigureClock,
			EStExit,
			EStEnd
			};

	DMMCSession& s = Session();
	TMMCard* cardP = iCardArray->CardP(iSelectedCardIndex);
	OstTrace1( TRACE_INTERNALS, DMMCSTACK_CONFIGUREHIGHSPEEDSM1, "Current session=0x%x", &s );

	SMF_BEGIN

        OstTrace0( TRACE_INTERNALS, DMMCSTACK_CONFIGUREHIGHSPEEDSM2, "EStBegin" );
        
		// Trap Switch errors & no-response errors
		m.SetTraps(KMMCErrResponseTimeOut | KMMCErrStatus);

		__KTRACE_OPT(KPBUS1, Kern::Printf(">ConfigureHighSpeed(), iCxCardCount %u", iCxCardCount));
		OstTrace1( TRACE_INTERNALS, DMMCSTACK_CONFIGUREHIGHSPEEDSM3, "iCxCardCount=%d", iCxCardCount );

		cardP->SetHighSpeedClock(0);

		// Check the card type is valid
        if (!(cardP->iExtendedCSD.IsSupportedCardType()))
            {            
			__KTRACE_OPT(KPBUS1, Kern::Printf("Unsupported card type %u", cardP->iExtendedCSD.CardType()));
			OstTrace1( TRACE_INTERNALS, DMMCSTACK_CONFIGUREHIGHSPEEDSM4, "Unsupported card type=%u", cardP->iExtendedCSD.CardType() );
			SMF_GOTOS(EStExit);
			}

		// If the bus width is 4 or 8, send SWITCH cmd and write the BUS_WIDTH byte of the EXT_CSD register

		if (iBusWidthAndClock != E1Bit20Mhz)
			{
			TMMCArgument arg = TExtendedCSD::GetWriteArg(
				TExtendedCSD::EWriteByte,
				TExtendedCSD::EBusWidthModeIndex,
				(iBusWidthAndClock & E4BitMask) ? TExtendedCSD::EExtCsdBusWidth4 : TExtendedCSD::EExtCsdBusWidth8,
				0);

			__KTRACE_OPT(KPBUS1, Kern::Printf(">ConfigureHighSpeed(), Writing to EXT_CSD (EBusWidthMode), arg %08X", (TUint32) arg));
			OstTrace1( TRACE_INTERNALS, DMMCSTACK_CONFIGUREHIGHSPEEDSM5, "Writing to EXT_CSD (EBusWidthMode); arg=0x%x", (TUint32) arg );
			s.FillCommandDesc(ECmdSwitch, arg);
			SMF_INVOKES(ExecSwitchCommandST, EStConfigureBusWidth)
			}

	SMF_STATE(EStConfigureBusWidth)

        OstTrace0( TRACE_INTERNALS, DMMCSTACK_CONFIGUREHIGHSPEEDSM6, "EStConfigureBusWidth" );
        
		if (err != KMMCErrNone)
			{
			SMF_GOTOS(EStExit);
			}

		// Ensure that the controller is configured for an 4 or 8 bit bus
		//  - BUSTEST should have already done this
		if (iBusWidthAndClock & E4BitMask)
			{
			DoSetBusWidth(EBusWidth4);
			}
		else if (iBusWidthAndClock & E8BitMask)
			{
			DoSetBusWidth(EBusWidth8);
			}
		// fall through to next state

	SMF_STATE(EStWriteHsTiming)
	
		OstTrace0( TRACE_INTERNALS, DMMCSTACK_CONFIGUREHIGHSPEEDSM7, "EStWriteHsTiming" );
	
		if (iBusWidthAndClock == E1Bit20Mhz)
			SMF_GOTOS(EStExit);

		TMMCArgument arg = TExtendedCSD::GetWriteArg(
			TExtendedCSD::EWriteByte,
			TExtendedCSD::EHighSpeedInterfaceTimingIndex,
			1,	// turn on high speed (26 or 52 Mhz, depending on the card type)
			0);

		__KTRACE_OPT(KPBUS1, Kern::Printf(">ConfigureHighSpeed(), Writing to EXT_CSD (EHighSpeedInterfaceTiming), arg %08X", (TUint32) arg));
		OstTrace1( TRACE_INTERNALS, DMMCSTACK_CONFIGUREHIGHSPEEDSM8, "Writing to EXT_CSD (EHighSpeedInterfaceTiming); arg=0x%x", (TUint32) arg );
		s.FillCommandDesc(ECmdSwitch, arg);
		SMF_INVOKES(ExecSwitchCommandST, EStConfigureClock)


	SMF_STATE(EStConfigureClock)

		OstTrace0( TRACE_INTERNALS, DMMCSTACK_CONFIGUREHIGHSPEEDSM9, "EStConfigureClock" );
	
		if (err != KMMCErrNone)
			{
			DoSetBusWidth(EBusWidth1);
			SMF_GOTOS(EStExit);
			}

		cardP->SetHighSpeedClock(
			MHZ_TO_KHZ(((iBusWidthAndClock & E52MhzMask) ? 
			            TMMCMachineInfoV4::EClockSpeed52Mhz:
			            TMMCMachineInfoV4::EClockSpeed26Mhz)));

	SMF_STATE(EStExit)
	
		OstTrace0( TRACE_INTERNALS, DMMCSTACK_CONFIGUREHIGHSPEEDSM10, "EStExit" );
	
		m.ResetTraps();

	SMF_END
	}

// Issue a switch command and then wait while he card is in prg state
//
TMMCErr DMMCStack::ExecSwitchCommand()
	{
		enum states
			{
			EStBegin=0,
			EStSendStatus,
			EStGetStatus,
			EStEnd
			};

		DMMCSession& s=Session();
		OstTrace1( TRACE_INTERNALS, DMMCSTACK_EXECSWITCHCOMMAND1, "Current session=0x%x", &s );

	SMF_BEGIN
    	OstTrace0( TRACE_INTERNALS, DMMCSTACK_EXECSWITCHCOMMAND2, "EStBegin" );
    	SMF_INVOKES(ExecCommandSMST, EStSendStatus)

	SMF_STATE(EStSendStatus)
		OstTrace0( TRACE_INTERNALS, DMMCSTACK_EXECSWITCHCOMMAND3, "EStSendStatus" );
		s.FillCommandDesc(ECmdSendStatus, 0);
		SMF_INVOKES(ExecCommandSMST, EStGetStatus)

	SMF_STATE(EStGetStatus)
		OstTrace0( TRACE_INTERNALS, DMMCSTACK_EXECSWITCHCOMMAND4, "EStGetStatus" );
		const TMMCStatus st(s.ResponseP());

		const TMMCardStateEnum st1 = st.State();
		if (st1 == ECardStatePrg)
			{
			SMF_INVOKES(ProgramTimerSMST, EStSendStatus);
			}
		
	// Fall through if CURRENT_STATE is not PGM

	SMF_END
	}

// Issue CMD5 to change device status to Sleep mode
//
TMMCErr DMMCStack::ExecSleepCommandSM()
	{
		enum states
			{
			EStBegin=0,
			EStIndexNxtCard,
			EStIssueSleepAwake,
			EStSleepAwakeIssued,
			EStUpdateStackState,
			EStDone,
			EStEnd
			};

		DMMCSession& s=Session();
		OstTrace1( TRACE_INTERNALS, DMMCSTACK_EXECSLEEPCOMMANDSM1, "Current session=0x%x", &s );

	SMF_BEGIN
	
        OstTrace0( TRACE_INTERNALS, DMMCSTACK_EXECSLEEPCOMMANDSM2, "EStBegin" );
        
		__KTRACE_OPT(KPBUS1, Kern::Printf(">ExecSleepCommandSM()"));
		
		iAutoUnlockIndex = -1;
		// drop through....
		
	SMF_STATE(EStIndexNxtCard)
	
		OstTrace0( TRACE_INTERNALS, DMMCSTACK_EXECSLEEPCOMMANDSM3, "EStIndexNxtCard" );
		__KTRACE_OPT(KPBUS1, Kern::Printf(">EStIndexNxtCard"));
		// the cycle is finished when iAutoUnlockIndex == KMaxMultiMediaCardsPerStack
		if(iAutoUnlockIndex >= TInt(KMaxMMCardsPerStack))
			{
			SMF_GOTOS(EStUpdateStackState);
			}

		// Potentionaly more than one eMMC device attached to Controller
		// need to select each device and determine if Sleep can be issued
		TBool useIndex = EFalse;
		for (++iAutoUnlockIndex; iAutoUnlockIndex < TInt(KMaxMMCardsPerStack); ++iAutoUnlockIndex)
			{
			// card must be present and a valid 4.3 device
			TMMCard* cardP = iCardArray->CardP(iAutoUnlockIndex);
			useIndex = ( (cardP->IsPresent()) &&
					     (cardP->ExtendedCSD().ExtendedCSDRev() >= 3) &&
						 (cardP->iStatus != ECardStateSlp) );

			// don't increment iAutoUnlockIndex in continuation loop
			if (useIndex)
				{
				__KTRACE_OPT(KPBUS1, Kern::Printf(">Card[%d]: is v4.3 device",iAutoUnlockIndex));
				OstTrace1( TRACE_INTERNALS, DMMCSTACK_EXECSLEEPCOMMANDSM4, "Card[%d]: is v4.3+ device", iAutoUnlockIndex );
				break;
				}
			}
		
		if (!useIndex)
			{
			SMF_GOTOS(EStUpdateStackState);
			}
		
		TMMCard &cd = *(iCardArray->CardP(iAutoUnlockIndex));
		s.SetCard(&cd);
		
		CurrentSessPushCmdStack();		
		s.FillCommandDesc(ECmdSleepAwake, KBit15);
		
		// CMD5 is an AC command, ExecCommandSMST will automatically issue a deselect
		SMF_INVOKES(ExecCommandSMST, EStSleepAwakeIssued)
		
	SMF_STATE(EStSleepAwakeIssued)
		 
		OstTrace0( TRACE_INTERNALS, DMMCSTACK_EXECSLEEPCOMMANDSM5, "EStSleepAwakeIssued" );
		__KTRACE_OPT(KPBUS1, Kern::Printf(">EStSleepAwakeIssued!"));
		
		const TMMCStatus status(s.ResponseP());
		
		CurrentSessPopCmdStack();

		if(status.State() == ECardStateStby || status.State() == ECardStateSlp)
			{			
			// R1b is issued before Sleep state is achieved and 
			// will therefore return the previous state which was Standby
			__KTRACE_OPT(KPBUS1, Kern::Printf(">Card[%d]: SLEEP",iAutoUnlockIndex));
			OstTrace1( TRACE_INTERNALS, DMMCSTACK_EXECSLEEPCOMMANDSM6, "Card[%d]: SLEEP", iAutoUnlockIndex );
			
			// Ensure card status is ECardStateSlp
			s.CardP()->iStatus.UpdateState(ECardStateSlp);
			
			// Update Stack state to indicate media is sleep mode
			iStackState |= KMMCStackStateSleep;
			}
		else
			{ 
			__KTRACE_OPT(KPBUS1, Kern::Printf(">Card[%d]: UNKNOWN",iAutoUnlockIndex));
			OstTrace1( TRACE_INTERNALS, DMMCSTACK_EXECSLEEPCOMMANDSM7, "Card[%d]: UNKNOWN", iAutoUnlockIndex );
			
			return (KMMCErrStatus);
			}
		
		SMF_GOTOS(EStIndexNxtCard)

	SMF_STATE(EStUpdateStackState)

		OstTrace0( TRACE_INTERNALS, DMMCSTACK_EXECSLEEPCOMMANDSM8, "EStUpdateStackState" );
		if (iStackState & KMMCStackStateSleep)
			{ 
			// Media has been transitioned to sleep state
			iStackState &= ~KMMCStackStateSleep;
			
			// VccCore may now be switched off
			iSocket->iVccCore->SetState(EPsuOff);
			}
//		else
			// No media transitioned to sleep state or media was already in sleep state,
			// nothing to do...
			
	SMF_STATE(EStDone)
		
		OstTrace0( TRACE_INTERNALS, DMMCSTACK_EXECSLEEPCOMMANDSM9, "EStDone" );
		__KTRACE_OPT(KPBUS1, Kern::Printf("<ExecSleepCommandSM()"));
		
	SMF_END
	}


//Issue CMD5 to change devices into STANDBY state
TMMCErr DMMCStack::ExecAwakeCommandSM()
	{
		enum states
			{
			EStBegin=0,
			EStPoweredUp,
			EStAwakeIssued,
			EStDone,
			EStEnd
			};

		DMMCSession& s=Session();
		OstTrace1( TRACE_INTERNALS, DMMCSTACK_EXECAWAKECOMMANDSM1, "Current session=0x%x", &s );

	SMF_BEGIN
	
		OstTrace0( TRACE_INTERNALS, DMMCSTACK_EXECAWAKECOMMANDSM2, "EStBegin" );
		__KTRACE_OPT(KPBUS1, Kern::Printf(">ExecAwakeCommandSM()"));
		
		// Call PSL to ensure VccQ is powered up before continuing
		SMF_INVOKES( DoWakeUpSMST, EStPoweredUp )
		
	SMF_STATE(EStPoweredUp)	

		OstTrace0( TRACE_INTERNALS, DMMCSTACK_EXECAWAKECOMMANDSM3, "EStPoweredUp" );
		__KTRACE_OPT(KPBUS1, Kern::Printf("VccQ Powered Up"));
		
		//Issue CMD5 to awaken media
		CurrentSessPushCmdStack();		
		s.FillCommandDesc(ECmdSleepAwake);
		s.Command().iArgument.SetRCA(s.CardP()->RCA());
		
		SMF_INVOKES(IssueCommandCheckResponseSMST, EStAwakeIssued)
		
	SMF_STATE(EStAwakeIssued)
		 
		OstTrace0( TRACE_INTERNALS, DMMCSTACK_EXECAWAKECOMMANDSM4, "EStAwakeIssued" );
		__KTRACE_OPT(KPBUS1, Kern::Printf(">>EStAwakeIssued!"));
		
		TMMCStatus status(s.ResponseP());

		if(status.State() == ECardStateStby || status.State() == ECardStateSlp)
			{			
			// R1b is issued before Standby state is achieved and 
			// will therefore return the previous state which was Sleep
			__KTRACE_OPT(KPBUS1, Kern::Printf(">Card[%d]: STANDBY",iAutoUnlockIndex));
			OstTrace1( TRACE_INTERNALS, DMMCSTACK_EXECAWAKECOMMANDSM5, "Card[%d]: STANDBY", iAutoUnlockIndex );
			s.CardP()->iStatus.UpdateState(ECardStateStby);
			}
		else
			{ 
			__KTRACE_OPT(KPBUS1, Kern::Printf(">Card[%d]: UNKNOWN",iAutoUnlockIndex));
			OstTrace1( TRACE_INTERNALS, DMMCSTACK_EXECAWAKECOMMANDSM6, "Card[%d]: UNKNOWN", iAutoUnlockIndex );
			OstTraceFunctionExitExt( DMMCSTACK_EXECAWAKECOMMANDSM_EXIT, this, (TInt) KMMCErrStatus );
			return KMMCErrStatus;
			}

		CurrentSessPopCmdStack();	
	
	// Fall through to the next state
	SMF_STATE(EStDone)
	
		OstTrace0( TRACE_INTERNALS, DMMCSTACK_EXECAWAKECOMMANDSM7, "EStDone" );
		__KTRACE_OPT(KPBUS1, Kern::Printf("<ExecAwakeCommandSM()"));
	
	SMF_END
	}


// determine the maximum bus width and clock speed supported by both the controller
// and the card which fits the power constraints.
void DMMCStack::DetermineBusWidthAndClock(
	const TMMCard& aCard, 
	TBool aLowVoltage,
	TUint& aPowerClass,
	TBusWidthAndClock& aBusWidthAndClock)
	{
	OstTraceExt2(TRACE_FLOW, DMMCSTACK_DETERMINEBUSWIDTHANDCLOCK_ENTRY, "DMMCStack::DetermineBusWidthAndClock;aLowVoltage=%d;this=%x", (TInt) aLowVoltage, (TUint) this);

	// Set default return values - in case power constraints aren't matched
	aPowerClass = 0;
	aBusWidthAndClock = E1Bit20Mhz;

	// Get the bus widths & clocks supported by the controller
	// NB If the PSL doesn not support TMMCMachineInfoV4, return
	TMMCMachineInfoV4 machineInfo;
	TMMCMachineInfoV4Pckg machineInfoPckg(machineInfo);
	MachineInfo(machineInfoPckg);
	if (machineInfo.iVersion < TMMCMachineInfoV4::EVersion4)
	    {
		OstTraceFunctionExit1( DMMCSTACK_DETERMINEBUSWIDTHANDCLOCK_EXIT1, this );
		return;
	    }

	TBusWidth maxBusWidth = machineInfo.iMaxBusWidth;
	TInt maxClockSpeedInMhz = machineInfo.iMaxClockSpeedInMhz;

	TUint32 controllerWidthAndClock = E1Bit20Mhz;

	if (maxClockSpeedInMhz >= TMMCMachineInfoV4::EClockSpeed26Mhz && maxBusWidth >= EBusWidth4)
		controllerWidthAndClock|= E4Bits26Mhz;
	if (maxClockSpeedInMhz >= TMMCMachineInfoV4::EClockSpeed52Mhz && maxBusWidth >= EBusWidth4)
		controllerWidthAndClock|= E4Bits52Mhz;
		
	if (maxClockSpeedInMhz >= TMMCMachineInfoV4::EClockSpeed26Mhz && maxBusWidth >= EBusWidth8)
		controllerWidthAndClock|= E8Bits26Mhz;
	if (maxClockSpeedInMhz >= TMMCMachineInfoV4::EClockSpeed52Mhz && maxBusWidth >= EBusWidth8)
		controllerWidthAndClock|= E8Bits52Mhz;
		
	// Get the bus widths & clocks supported by the card
	TUint32 cardWidthAndClock = E1Bit20Mhz;

	if (aCard.ExtendedCSD().CardType() & TExtendedCSD::EHighSpeedCard26Mhz)
		cardWidthAndClock|= E4Bits26Mhz | E8Bits26Mhz;
	if (aCard.ExtendedCSD().CardType() & TExtendedCSD::EHighSpeedCard52Mhz)
		cardWidthAndClock|= E4Bits52Mhz | E8Bits52Mhz;


	// Get the bus widths & clocks supported by both the controller & card
	// by AND-ing them together,
	TUint32 supportedWidthAndClock = controllerWidthAndClock & cardWidthAndClock;

	// Iterate through all the modes (starting at the fastest) until we find one
	// that is supported by both card & controller and fits the power constraints
	TUint powerClass = 0;
	for (TUint targetWidthAndClock = E8Bits52Mhz; targetWidthAndClock != 0; targetWidthAndClock>>= 1)
		{
		if ((supportedWidthAndClock & targetWidthAndClock) == 0)
			continue;

		powerClass = GetPowerClass(aCard, TBusWidthAndClock(targetWidthAndClock), aLowVoltage);

		// can the controller support this power class ?
		if (powerClass > (aLowVoltage ? machineInfo.iLowVoltagePowerClass : machineInfo.iHighVoltagePowerClass ))
			continue;

		aPowerClass = powerClass;
		aBusWidthAndClock = TBusWidthAndClock(targetWidthAndClock);
		break;
		}

	__KTRACE_OPT(KPBUS1, Kern::Printf("aPowerClass %u, targetWidthAndClock = %08X", aPowerClass, aBusWidthAndClock));
	OstTraceExt2( TRACE_INTERNALS, DMMCSTACK_DETERMINEBUSWIDTHANDCLOCK, "aPowerClass=%u; targetWidthAndClock=0x%08x", aPowerClass, (TUint) aBusWidthAndClock );
	OstTraceFunctionExit1( DMMCSTACK_DETERMINEBUSWIDTHANDCLOCK_EXIT2, this );
	}

TUint DMMCStack::GetPowerClass(const TMMCard& aCard, TBusWidthAndClock aWidthAndClock, TBool aLowVoltage)
	{
	OstTraceExt3(TRACE_FLOW, DMMCSTACK_GETPOWERCLASS_ENTRY, "DMMCStack::GetPowerClass;aWidthAndClock=%d;aLowVoltage=%d;this=%x", (TInt) aWidthAndClock, (TInt) aLowVoltage, (TUint) this);
	// The power class for 4 bit bus configurations is in the low nibble,
	// The power class for 8 bit bus configurations is in the high nibble,
	#define LO_NIBBLE(val) (val & 0x0F)
	#define HI_NIBBLE(val) ((val >> 4) & 0x0F)

	const TExtendedCSD& extendedCSD = aCard.ExtendedCSD();

	TUint powerClass = 0;

	if (aLowVoltage)
		{
		switch(	aWidthAndClock)
			{
			case E4Bits26Mhz:
				powerClass = LO_NIBBLE(extendedCSD.PowerClass26Mhz195V());
				break;
			case E4Bits52Mhz:
				powerClass = LO_NIBBLE(extendedCSD.PowerClass52Mhz195V());
				break;
			case E8Bits26Mhz:
				powerClass = HI_NIBBLE(extendedCSD.PowerClass26Mhz195V());
				break;
			case E8Bits52Mhz:
				powerClass = HI_NIBBLE(extendedCSD.PowerClass52Mhz195V());
				break;
			case E1Bit20Mhz:
				powerClass = 0;
				break;
			}
		}
	else
		{
		switch(	aWidthAndClock)
			{
			case E4Bits26Mhz:
				powerClass = LO_NIBBLE(extendedCSD.PowerClass26Mhz360V());
				break;
			case E4Bits52Mhz:
				powerClass = LO_NIBBLE(extendedCSD.PowerClass52Mhz360V());
				break;
			case E8Bits26Mhz:
				powerClass = HI_NIBBLE(extendedCSD.PowerClass26Mhz360V());
				break;
			case E8Bits52Mhz:
				powerClass = HI_NIBBLE(extendedCSD.PowerClass52Mhz360V());
				break;
			case E1Bit20Mhz:
				powerClass = 0;
				break;
			}
		}

		OstTraceFunctionExitExt( DMMCSTACK_GETPOWERCLASS_EXIT, this, powerClass );
		return powerClass;
	}

//
// Execute the BUSTEST procedure for a given bus width
//
TMMCErr DMMCStack::ExecBusTestSM()
	{
	enum states
		{
		EStBegin=0,
		EstSendBusTest_W,
		EstSendBusTest_R,
		EstGotBusTest_R,
		EStExit,
		EStEnd
		};

	DMMCSession& s = Session();
	OstTrace1( TRACE_INTERNALS, DMMCSTACK_EXECBUSTESTSM1, "Current session=0x%x", &s );

	SMF_BEGIN
		//
		// Start the BUSTEST sequence at the maximum supported by the PSL
		//  - iSpare[0] keeps track of the current bus width
		//
		OstTrace0( TRACE_INTERNALS, DMMCSTACK_EXECBUSTESTSM2, "EStBegin" );
		if (iBusWidthAndClock & E8BitMask)
			{
			iSpare[0] = EBusWidth8;
			__KTRACE_OPT(KPBUS1, Kern::Printf("...Hardware supports 8-bit bus"));
			OstTrace0( TRACE_INTERNALS, DMMCSTACK_EXECBUSTESTSM3, "Hardware supports 8-bit bus" );
			}
		else if(iBusWidthAndClock & E4BitMask)
			{
			iSpare[0] = EBusWidth4;
			__KTRACE_OPT(KPBUS1, Kern::Printf("...Hardware supports 4-bit bus"));
			OstTrace0( TRACE_INTERNALS, DMMCSTACK_EXECBUSTESTSM4, "Hardware supports 4-bit bus" );
			}
		else
			{
			__KTRACE_OPT(KPBUS1, Kern::Printf("...Hardware supports 1-bit bus"));
			OstTrace0( TRACE_INTERNALS, DMMCSTACK_EXECBUSTESTSM5, "Hardware supports 1-bit bus" );
			iSpare[0] = EBusWidth1;
			}

		// remove KMMCModeCardControlled so that IssueCommandCheckResponseSMST doesn't try to 
		// override the bus width & clock rate using the card settings
		iConfig.RemoveMode( KMMCModeCardControlled );

	SMF_STATE(EstSendBusTest_W)
		//
		// Issue the BUSTEST_W command
		//
		OstTrace0( TRACE_INTERNALS, DMMCSTACK_EXECBUSTESTSM6, "EStSendBusTest_W" );
		TInt length = 2;
		switch(iSpare[0])
			{
			case EBusWidth8:
				// Set the host to 8-bit mode
				__KTRACE_OPT(KPBUS1, Kern::Printf("BUSTEST : EBusWidth8"));
				OstTrace0( TRACE_INTERNALS, DMMCSTACK_EXECBUSTESTSM7, "BUSTEST : EBusWidth8" );
				DoSetBusWidth(EBusWidth8);
				iPSLBuf[0] = 0x55;
				iPSLBuf[1] = 0xaa;
				break;

			case EBusWidth4:
				// Set the host to 4-bit mode
				__KTRACE_OPT(KPBUS1, Kern::Printf("BUSTEST : EBusWidth4"));
				OstTrace0( TRACE_INTERNALS, DMMCSTACK_EXECBUSTESTSM8, "BUSTEST : EBusWidth4" );
				DoSetBusWidth(EBusWidth4);
				iPSLBuf[0] = 0x5a;
				iPSLBuf[1] = 0x00;
				break;

			case EBusWidth1:
			default:
				// Set the host to 1-bit mode
				__KTRACE_OPT(KPBUS1, Kern::Printf("BUSTEST : EBusWidth1"));
				OstTrace0( TRACE_INTERNALS, DMMCSTACK_EXECBUSTESTSM9, "BUSTEST : EBusWidth1" );
				DoSetBusWidth(EBusWidth1);
				iPSLBuf[0] = 0x40;
				iPSLBuf[1] = 0x00;
				break;
			}	

		// Issue BUSTEST_W

		__KTRACE_OPT(KPBUS1, Kern::Printf("...Issue BUSTEST_W [%02x:%02x]", iPSLBuf[1], iPSLBuf[0]));
		OstTraceExt2( TRACE_INTERNALS, DMMCSTACK_EXECBUSTESTSM10, "Issue BUSTEST_W [%02x:%02x]", (TUint) iPSLBuf[1], (TUint) iPSLBuf[0] );

		m.SetTraps(KMMCErrDataCRC); // CRC check is optional for BUSTEST

		s.FillCommandDesc(ECmdBustest_W);
		s.FillCommandArgs(0, length, &iPSLBuf[0], length);
		SMF_INVOKES(IssueCommandCheckResponseSMST, EstSendBusTest_R)

	SMF_STATE(EstSendBusTest_R)
		//
		// Issue the BUSTEST_R command
		//
		OstTrace0( TRACE_INTERNALS, DMMCSTACK_EXECBUSTESTSM11, "EStSendBusTest_R" );
		__KTRACE_OPT(KPBUS1, Kern::Printf("...got BUSTEST_W response : %02x", err));
		OstTrace1( TRACE_INTERNALS, DMMCSTACK_EXECBUSTESTSM12, "Got BUSTEST_W response=0x%02x", err );

		if(err == KMMCErrNone || err == KMMCErrDataCRC)
			{
			__KTRACE_OPT(KPBUS1, Kern::Printf("...sending BUSTEST_R"));
			OstTrace0( TRACE_INTERNALS, DMMCSTACK_EXECBUSTESTSM13, "Sending BUSTEST_R" );

			iPSLBuf[0] = 0;
			iPSLBuf[1] = 0;

			s.FillCommandDesc(ECmdBustest_R);
			s.FillCommandArgs(0, 2, &iPSLBuf[0], 2);
			SMF_INVOKES(IssueCommandCheckResponseSMST, EstGotBusTest_R)
			}
		else
			{
			OstTraceFunctionExitExt( DMMCSTACK_EXECBUSTESTSM_EXIT, this, (TInt) KMMCErrNotSupported );
			SMF_RETURN(KMMCErrNotSupported);
			}

	SMF_STATE(EstGotBusTest_R)
		//
		// Validate the BUSTEST_R data with that issued by BUSTEST_W
		//
		OstTrace0( TRACE_INTERNALS, DMMCSTACK_EXECBUSTESTSM14, "EStGotBusTest_R" );
		__KTRACE_OPT(KPBUS1, Kern::Printf("...got BUSTEST_R response [%02x:%02x] : err(%02x)", iPSLBuf[1], iPSLBuf[0], err));
		OstTraceExt3( TRACE_INTERNALS, DMMCSTACK_EXECBUSTESTSM15, "Got BUSTEST_R response [%02x:%02x]; err(%x)", (TUint) iPSLBuf[1], (TUint) iPSLBuf[0], (TUint) err );

		TBool retry = EFalse;
		TBool is52MHzSupported = (iBusWidthAndClock & E52MhzMask) ? (TBool)ETrue : (TBool)EFalse;

		switch(iSpare[0])
			{
			case EBusWidth8:
				{
				if(iPSLBuf[0] == 0xAA && iPSLBuf[1] == 0x55)
					{
					// 8-Bit bus supported
					iBusWidthAndClock = is52MHzSupported ? E8Bits52Mhz : E8Bits26Mhz;
					}
				else
					{
					// 8-Bit bus not supported - retry with 4-Bit
					retry = ETrue;
					iSpare[0] = EBusWidth4;
					}
				break;
				}

			case EBusWidth4:
				{
				if(iPSLBuf[0] == 0xA5)
					{
					// 4-Bit Bus Supported
					iBusWidthAndClock = is52MHzSupported ? E4Bits52Mhz : E4Bits26Mhz;
					}
				else
					{
					// 4-Bit bus not supported - retry with 1-Bit
					retry = ETrue;
					iSpare[0] = EBusWidth1;
					}
				break;
				}

			case EBusWidth1:
				{
				if((iPSLBuf[0] & 0xC0) == 0x80)
					{
					// 1-Bit Bus Supported
					iBusWidthAndClock = E1Bit20Mhz;
					}

				// Failed to perform BUSTEST with 1-Bit bus.
				//  - We can't recover from this, but let's continue using low-speed 1-Bit mode
				iBusWidthAndClock = E1Bit20Mhz;
				break;
				}

			default:
				DMMCSocket::Panic(DMMCSocket::EMMCBadBusWidth);
				break;
			}	

		if(retry)
			{
			__KTRACE_OPT(KPBUS1, Kern::Printf("...BUSTEST Failed : Retry"));
			OstTrace0( TRACE_INTERNALS, DMMCSTACK_EXECBUSTESTSM16, "BUSTEST Failed : Retry" );
			SMF_GOTOS(EstSendBusTest_W);
			}
		
		switch(iBusWidthAndClock)
			{
			case E1Bit20Mhz:
				iCardArray->CardP(iSelectedCardIndex)->SetBusWidth(1);
				break;

			case E4Bits26Mhz:
			case E4Bits52Mhz:
				iCardArray->CardP(iSelectedCardIndex)->SetBusWidth(4);
				break;

			case E8Bits26Mhz:
			case E8Bits52Mhz:
				iCardArray->CardP(iSelectedCardIndex)->SetBusWidth(8);
				break;
			}

		__KTRACE_OPT(KPBUS1, Kern::Printf("...BUSTEST OK"));
		OstTrace0( TRACE_INTERNALS, DMMCSTACK_EXECBUSTESTSM17, "BUSTEST OK" );

		DoSetBusWidth(EBusWidth1);

	SMF_STATE(EStExit)
		OstTrace0( TRACE_INTERNALS, DMMCSTACK_EXECBUSTESTSM18, "EStExit" );
		iConfig.SetMode( KMMCModeCardControlled );

	SMF_END
	}


/**
 * PSL-supplied method to retrieve an interface.
 * The caller should set aInterfacePtr to NULL before calling
 * the PSL should only modify aInterfacePtr if it supports the interface
 * The default implementation here does nothing
 * Replaces Dummy2()
*/
EXPORT_C void DMMCStack::GetInterface(TInterfaceId aInterfaceId, MInterface*& aInterfacePtr)
	{
	OstTraceFunctionEntry1( DMMCSTACK_GETINTERFACE_ENTRY, this );
	if (aInterfaceId == KInterfaceCancelSession)
		{
		DMMCSession* session = (DMMCSession*&) aInterfacePtr;
		Abort(session);
		UnlockStack(session);
		}

	OstTraceFunctionExit1( DMMCSTACK_GETINTERFACE_EXIT, this );
	}


TMMCErr DMMCStack::GoIdleSM()
/**
 * Issues GO_IDLE_STATE twice with a RetryGap between them. 
 * After that the bus context ought to be considered as "known".
 * @return MMC error code
 */
	{
		enum states
			{
			EStBegin=0,
			EStIdleLoop,
			EStIdleEndCheck,
			EStEnd
			};

		DMMCSession& s=Session();
		OstTrace1( TRACE_INTERNALS, DMMCSTACK_GOIDLESM1, "Current session=0x%x", &s );

	SMF_BEGIN

		OstTrace0( TRACE_INTERNALS, DMMCSTACK_GOIDLESM2, "EStBegin" );
		s.FillCommandDesc( ECmdGoIdleState, 0 );
		iCxPollRetryCount = KMMCIdleCommandsAtRestart;

	SMF_STATE(EStIdleLoop)
		OstTrace0( TRACE_INTERNALS, DMMCSTACK_GOIDLESM3, "EStIdleLoop" );
		SMF_INVOKES( ExecCommandSMST, EStIdleEndCheck )

	SMF_STATE(EStIdleEndCheck)

		OstTrace0( TRACE_INTERNALS, DMMCSTACK_GOIDLESM4, "EStIdleEndCheck" );
		if( --iCxPollRetryCount > 0 )
			SMF_INVOKES( RetryGapTimerSMST, EStIdleLoop )

		iStackState &= ~(KMMCStackStateDoDeselect|KMMCStackStateBusInconsistent);
		iSelectedCard = 0;

		// According to the spec, the default bus width after power up or GO_IDLE is 1 bit bus width
		DoSetBusWidth(EBusWidth1);

	SMF_END
	}

EXPORT_C TMMCErr DMMCStack::AcquireStackSM()
/**
 * This macro acquires new cards in an MMC - bus topology stack.
 * It starts with the Controller reading the operating conditions of the 
 * cards in the stack (SEND_OP_COND - CMD1). Then, any new cards in the stack
 * are identified (ALL_SEND_CID - CMD2) and each one is assigned a relative
 * card address (SET_RCA - CMD3). This is done by systematically broadcasting
 * CMD2 to all cards on the bus until all uninitialized cards have responded.
 * Finally the card specific data (SEND_CSD - CMD9) is read from each card.
 * @return MMC error code
 */
	{
		enum states
			{
			EStBegin=0,
			EStIdle,
			EStFullRangeDone,
			EStSetRangeLoop,
			EStSetRangeBusyCheck,
			EStCIDLoop,
			EStSendCIDIssued,
			EStCIDsDone,
			EStCSDLoop,
			EStSendCSDDone,
			EStMergeCards,
			EStReMergeCards,
			EStEnd
			};

		DMMCSession& s=Session();
		DMMCPsu* psu=(DMMCPsu*)iSocket->iVcc;
		OstTrace1( TRACE_INTERNALS, DMMCSTACK_ACQUIRESTACKSM1, "Current session=0x%x", &s );

	SMF_BEGIN

		OstTrace0( TRACE_INTERNALS, DMMCSTACK_ACQUIRESTACKSM2, "EStBegin" );
		iRCAPool.ReleaseUnlocked();
		iCxPollRetryCount = 0; // Reset max number of poll attempts on card busy

		SMF_INVOKES( GoIdleSMST, EStIdle )

	SMF_STATE(EStIdle)

		OstTrace0( TRACE_INTERNALS, DMMCSTACK_ACQUIRESTACKSM3, "EStIdle" );
		// If this platform doesn't support an adjustable voltage PSU then there is
		// no point in interogating the card(s) present for their supported range
		if ( !(psu->VoltageSupported()&KMMCAdjustableOpVoltage) )
			{
			// if the PSU isn't adjustable then it can't support low voltage mode
			iCurrentOpRange&= ~KMMCOCRLowVoltage;
			s.FillCommandDesc(ECmdSendOpCond, (iCurrentOpRange | KMMCOCRAccessModeHCS | KMMCOCRBusy));	// Range supported + Busy bit (iArgument==KBit31)
			SMF_GOTOS( EStSetRangeLoop )
			}

		// Interrogate card(s) present - issue CMD1 with omitted voltage range
		s.FillCommandDesc( ECmdSendOpCond, KMMCOCRAccessModeHCS | KMMCOCRBusy);	// Full range + Sector Access + Busy bit (iArgument==KBit31)
		m.SetTraps( KMMCErrResponseTimeOut );

		SMF_INVOKES( ExecCommandSMST, EStFullRangeDone )

	SMF_STATE(EStFullRangeDone)

		OstTrace0( TRACE_INTERNALS, DMMCSTACK_ACQUIRESTACKSM4, "EStFullRangeDone" );
		if( err )												// If timeout
			{
			iConfig.RemoveMode( KMMCModeEnableTimeOutRetry );	// There is no point to do it second time
			}
		else
			{
			// Cards responded with Op range - evaluate the common subset with the current setting
			// Dont worry aboout the busy bit for now, we'll check that when we repeat the command
			TUint32 newrange = (TMMC::BigEndian32(s.ResponseP()) & ~KMMCOCRBusy);
			newrange &= iCurrentOpRange;

			if (newrange==0)
				{
				// One or more card is incompatible with our h/w
				if (iMaxCardsInStack<=1)
				    {
					OstTraceFunctionExitExt( DMMCSTACK_ACQUIRESTACKSM_EXIT1, this, (TInt) KMMCErrNotSupported );
					return KMMCErrNotSupported; // There can only be one card - we don't support it.
				    }
				else
					// Force the default range
					iCurrentOpRange=(psu->VoltageSupported() & ~KMMCAdjustableOpVoltage);  
				}
			else
				iCurrentOpRange=newrange;		// OK, new cards are compatible
			}

		// If platform and the card both support low voltage mode (1.65 - 1.95v), switch
		if (iCurrentOpRange & KMMCOCRLowVoltage)
			{
			iCurrentOpRange = KMMCOCRLowVoltage;
			SMF_INVOKES( SwitchToLowVoltageSMST, EStSetRangeLoop )
			}

	SMF_STATE(EStSetRangeLoop)

		OstTrace0( TRACE_INTERNALS, DMMCSTACK_ACQUIRESTACKSM5, "EStSetRangeLoop" );
		// Repeat CMD1 this time setting Current Op Range
		s.Command().iArgument = iCurrentOpRange | KMMCOCRAccessModeHCS | KMMCOCRBusy;

		m.SetTraps( KMMCErrResponseTimeOut );

		SMF_INVOKES( ExecCommandSMST, EStSetRangeBusyCheck )

	SMF_STATE(EStSetRangeBusyCheck)

		OstTrace0( TRACE_INTERNALS, DMMCSTACK_ACQUIRESTACKSM6, "EStSetRangeLoop" );
		if( !err )
			{
			// Bit31 of the OCR response is low if the cards are still powering up.
			const TUint32 ocrResponse = TMMC::BigEndian32(s.ResponseP());

			const TBool isBusy = ((ocrResponse & KMMCOCRBusy) == 0);
			__KTRACE_OPT(KPBUS1,Kern::Printf("-mmc:upd:bsy%d", isBusy));
			OstTrace1( TRACE_INTERNALS, DMMCSTACK_ACQUIRESTACKSM7, "MMC busy status=%d", isBusy );

			if (isBusy)	
				{
				// Some cards are still busy powering up. Check if we should timeout
				if ( ++iCxPollRetryCount > iConfig.OpCondBusyTimeout() )
				    {
					OstTrace0( TRACE_INTERNALS, DMMCSTACK_ACQUIRESTACKSM8, "Peripheral bus timeout" );
					OstTraceFunctionExitExt( DMMCSTACK_ACQUIRESTACKSM_EXIT2, this, (TInt) KMMCErrBusTimeOut );
					return KMMCErrBusTimeOut;
				    }
				m.ResetTraps();
				SMF_INVOKES( RetryGapTimerSMST, EStSetRangeLoop )
				}

			iSpare[0] = 0;

			if((ocrResponse & KMMCOCRAccessModeMask) == KMMCOCRAccessModeHCS)
				{
				__KTRACE_OPT(KPBUS1, Kern::Printf("Found large MMC card."));
				OstTrace0( TRACE_INTERNALS, DMMCSTACK_ACQUIRESTACKSM9, "Found large MMC card" );
				iSpare[0] = KMMCardIsHighCapacity;
				}
			}

		iConfig.SetMode( EffectiveModes(s.iConfig) & KMMCModeEnableTimeOutRetry );	// Restore original setting

		// All cards are now ready and notified of the voltage range - ask ASSP to set it up
		psu->SetVoltage(iCurrentOpRange);
		if (psu->SetState(EPsuOnFull) != KErrNone)
		    {
			OstTraceFunctionExitExt( DMMCSTACK_ACQUIRESTACKSM_EXIT3, this, (TInt) KMMCErrHardware );
			return KMMCErrHardware;
		    }

		iCardArray->InitNewCardScan(); // Collect new cards, one by one

	SMF_STATE(EStCIDLoop)

		OstTrace0( TRACE_INTERNALS, DMMCSTACK_ACQUIRESTACKSM10, "EStCIDLoop" );
		if ( iCardArray->NewCardCount() >= iMaxCardsInStack )
			SMF_GOTOS( EStCIDsDone )

		s.FillCommandDesc( ECmdAllSendCID, 0 );
		m.SetTraps( KMMCErrResponseTimeOut );

		SMF_INVOKES( ExecCommandSMST, EStSendCIDIssued )

	SMF_STATE(EStSendCIDIssued)

		OstTrace0( TRACE_INTERNALS, DMMCSTACK_ACQUIRESTACKSM11, "EStSendCIDIssued" );
		if( !err )
			{
			// A card responded with a CID. Create a new card entry in the card array 
			// and initialise this entry with the CID. The card array allocates it an
			// RCA, either the old RCA if we have seen this card before, or a new one.
			TRCA rca;
			iCardArray->AddNewCard(s.ResponseP(),&rca); // Response is CID

			// Now assign the new RCA to the card
			s.FillCommandDesc( ECmdSetRelativeAddr, TMMCArgument(rca) );
			m.ResetTraps();
			SMF_INVOKES( ExecCommandSMST, EStCIDLoop )
			}

	SMF_STATE(EStCIDsDone)

		OstTrace0( TRACE_INTERNALS, DMMCSTACK_ACQUIRESTACKSM12, "EStCIDsDone" );
		// All cards are initialised; get all their CSDs
		m.ResetTraps();				// We are no longer processing any errors

		if( iCardArray->NewCardCount()==0 )
			SMF_EXIT						// No new cards acquired

		iCxCardCount=0;							// New cards index
		s.FillCommandDesc( ECmdSendCSD );

	SMF_STATE(EStCSDLoop)

		OstTrace0( TRACE_INTERNALS, DMMCSTACK_ACQUIRESTACKSM13, "EStCSDLoop" );
		s.Command().iArgument = TMMCArgument(iCardArray->NewCardP(iCxCardCount)->iRCA);
		SMF_INVOKES( ExecCommandSMST, EStSendCSDDone )

	SMF_STATE(EStSendCSDDone)

		OstTrace0( TRACE_INTERNALS, DMMCSTACK_ACQUIRESTACKSM14, "EStSendCSDDone" );
		// Store the CSD in the new card entry
		TMMCard* cardP = iCardArray->NewCardP(iCxCardCount);
		cardP->iCSD = s.ResponseP();

		// Perform MMC Specific parsing of the CSD structure
		TUint specVers = cardP->CSD().SpecVers();	// 1 => 1.4, 2 => 2.0 - 2.2, 3 => 3.1

		if ((specVers >= 2) && (cardP->CSD().CCC() & KMMCCmdClassLockCard))
			{
			cardP->iFlags |= KMMCardIsLockable;
			}

		if(iSpare[0] == KMMCardIsHighCapacity)
			{
			cardP->iFlags |= KMMCardIsHighCapacity;
			}

		if( ++iCxCardCount < (TInt)iCardArray->NewCardCount() )
			SMF_GOTOS( EStCSDLoop )

		SMF_NEXTS(EStMergeCards)

	SMF_STATE(EStMergeCards)

		OstTrace0( TRACE_INTERNALS, DMMCSTACK_ACQUIRESTACKSM15, "EStMergeCards" );
		// Merging the old card info with newly acquired cards (we will ask each card for status
		// to determine whether it's really present later).
		if( SchedGetOnDFC() )
			SMF_WAIT
		if ( iCardArray->MergeCards(ETrue)==KErrNone )
			SMF_EXIT						// Completed successfully

		SMF_INVOKES( CheckStackSMST, EStReMergeCards ) // No space so check if any cards have gone

	SMF_STATE(EStReMergeCards)

		OstTrace0( TRACE_INTERNALS, DMMCSTACK_ACQUIRESTACKSM16, "EStReMergeCards" );
		if( SchedGetOnDFC() )
			SMF_WAIT
		if ( iCardArray->MergeCards(EFalse)!=KErrNone ) // There are more cards in the stack than we can handle
		    {
			OstTraceFunctionExitExt( DMMCSTACK_ACQUIRESTACKSM_EXIT4, this, (TInt) KMMCErrTooManyCards );
			return KMMCErrTooManyCards;
		    }

	SMF_END
	}


/**
 * Power down the bus and power it up again in low-voltage mode
 * 
 * @return MMC error code.
 */
TMMCErr DMMCStack::SwitchToLowVoltageSM()
	{
	enum states
		{
		EStBegin=0,
		EStPoweredUp,
		EStClockOn,
		EStEnd
		};

	__KTRACE_OPT(KPBUS1,Kern::Printf(">MMC:SwLowVolt"));

	DMMCPsu* psu=(DMMCPsu*)iSocket->iVcc;
	OstTrace1( TRACE_INTERNALS, DMMCSTACK_SWITCHTOLOWVOLTAGESM1, "Current PSU=0x%x", psu );

	SMF_BEGIN
		OstTrace0( TRACE_INTERNALS, DMMCSTACK_SWITCHTOLOWVOLTAGESM2, "EStBegin" );
		// turn power off
		DoPowerDown();
		psu->SetState(EPsuOff);

		// turn power back on in low voltage mode
		psu->SetVoltage(iCurrentOpRange);
		if (psu->SetState(EPsuOnFull) != KErrNone)
		    {
			OstTraceFunctionExitExt( DMMCSTACK_SWITCHTOLOWVOLTAGESM_EXIT, this, (TInt) KMMCErrHardware );
			return KMMCErrHardware;
		    }

		SMF_INVOKES( DoPowerUpSMST, EStPoweredUp )

	SMF_STATE(EStPoweredUp)
		OstTrace0( TRACE_INTERNALS, DMMCSTACK_SWITCHTOLOWVOLTAGESM3, "EStPoweredUp" );
		// turn the clock back on
		SMF_INVOKES( InitClockOnSMST, EStClockOn )	// Feed init clock to the bus

	SMF_STATE(EStClockOn)
		OstTrace0( TRACE_INTERNALS, DMMCSTACK_SWITCHTOLOWVOLTAGESM4, "EStClockOn" );
		// wait for 1ms and then 74 clock cycles
		// 74 clock cylces @ 400 Khz = 74 / 400,000 = 0.000185 secs = 0.185 ms
		// so total wait = 1.185 ms
		SMF_INVOKES(LowVoltagePowerupTimerSMST, EStEnd);

	SMF_END

	}

inline TMMCErr DMMCStack::CIMCheckStackSM()
/**
 * Performs the CIM_CHECK_STACK macro (with pre-emption disabled).
 * @return MMC error code
 */
	{
		enum states
			{
			EStBegin=0,
			EStFinish,
			EStEnd
			};

		DMMCSession& s=Session();
		OstTrace1( TRACE_INTERNALS, DMMCSTACK_CIMCHECKSTACKSM1, "Current session=0x%x", &s );

	SMF_BEGIN

		OstTrace0( TRACE_INTERNALS, DMMCSTACK_CIMCHECKSTACKSM2, "EStBegin" );
		// This macro works naked and must not be preempted
		iConfig.RemoveMode( KMMCModeEnablePreemption | KMMCModeCardControlled );
		s.iState |= KMMCSessStateInProgress;

		SMF_INVOKES( CheckStackSMST, EStFinish )

	SMF_STATE(EStFinish)

		OstTrace0( TRACE_INTERNALS, DMMCSTACK_CIMCHECKSTACKSM3, "EStFinish" );
		s.iState &= ~KMMCSessStateInProgress;

	SMF_END
	}

inline TMMCErr DMMCStack::CheckStackSM()
/**
 * For each card in iCards[], sends CMD13 to see if still there.
 * If not, calls DeclareCardAsGone().  Frees up space for new cards.
 * @return MMC error code
 */
	{
		enum states
			{
			EStBegin=0,
			EStLoop,
			EStCardSelectedGotStatus,
			EStCardDeselected,
			EStEnd
			};

		DMMCSession& s=Session();
		OstTrace1( TRACE_INTERNALS, DMMCSTACK_CHECKSTACKSM1, "Current session=0x%x", &s );

	SMF_BEGIN

		OstTrace0( TRACE_INTERNALS, DMMCSTACK_CHECKSTACKSM2, "EStBegin" );
		iCxCardCount=-1;
		m.SetTraps( KMMCErrResponseTimeOut );

	SMF_STATE(EStLoop)

		OstTrace0( TRACE_INTERNALS, DMMCSTACK_CHECKSTACKSM3, "EStLoop" );
		if ( ++iCxCardCount == (TInt)iMaxCardsInStack )
			SMF_EXIT

		if ( !iCardArray->CardP(iCxCardCount)->IsPresent() )
			SMF_GOTOS( EStLoop )					// card's not present

		TUint32 arg = TUint32(iCardArray->CardP(iCxCardCount)->RCA()) << 16;
		s.FillCommandDesc(ECmdSelectCard, arg);
		SMF_INVOKES(ExecCommandSMST, EStCardSelectedGotStatus)

	SMF_STATE(EStCardSelectedGotStatus)

		OstTrace0( TRACE_INTERNALS, DMMCSTACK_CHECKSTACKSM4, "EStCardSelectedGotStatus" );
		__KTRACE_OPT(KPBUS1, Kern::Printf("-mst:cssm:err%08x", err));
		OstTrace1( TRACE_INTERNALS, DMMCSTACK_CHECKSTACKSM5, "err=0x%08x", err );

		if(err)
			{
			// Timeout - the card is no longer present so remove from the card array
			iCardArray->DeclareCardAsGone(iCxCardCount);
			SMF_GOTOS( EStLoop )
			}

		TMMCard& card=*(iCardArray->CardP(iCxCardCount));
		card.iStatus=s.ResponseP();

		// This function is only called as part of the power up sequence, so
		// take the opportunity to record if it has a password
		if((card.iStatus & KMMCStatCardIsLocked) != 0)
			{
			card.iFlags|=KMMCardHasPassword;
			}

		s.FillCommandDesc(ECmdSelectCard, 0);
		SMF_INVOKES(ExecCommandSMST, EStCardDeselected)
			
	SMF_STATE(EStCardDeselected)
		OstTrace0( TRACE_INTERNALS, DMMCSTACK_CHECKSTACKSM6, "EStCardDeselected" );
		SMF_GOTOS( EStLoop )

	SMF_END
	}

inline TMMCErr DMMCStack::CheckLockStatusSM()
/*
 * Called as part of the power-up sequence, this determined if the card is locked or has a password.
 *
 * If the card reports itself as being unlocked and there is a mapping in the password store, 
 * then the stored password is used to attempt to lock the card. The overall aim of this is
 * to ensure that if a card always powers up in the locked state if it contains a known password.
 *
 * This ensures that cards that are still unlocked after a power down/power up sequence do not
 * end up having their passwords removed from the store, which can happen in environments where
 * the PSU voltage level is not monitored - in such systems, we cannot guarantee that a card will 
 * be fully reset and power up locked, hence the need to attempt to lock the card.
 *
 * @return MMC error code
 */
	{
		enum states
			{
			EStBegin=0,
			EStLoop,
			EStCardSelectedGotStatus,
			EStCheckLockStatus,
			EStCardDeselected,
			EStEnd
			};

		DMMCSession& s=Session();
		OstTrace1( TRACE_INTERNALS, DMMCSTACK_CHECKLOCKSTATUSSM1, "Current session=0x%x", &s );

	SMF_BEGIN

		OstTrace0( TRACE_INTERNALS, DMMCSTACK_CHECKLOCKSTATUSSM2, "EStBegin" );
		iCxCardCount=-1;
		m.SetTraps( KMMCErrResponseTimeOut );
		iMinorBufLen = KMinMinorBufSize;

	SMF_STATE(EStLoop)

		if ( ++iCxCardCount == (TInt)iMaxCardsInStack )
			SMF_EXIT

		if ( !iCardArray->CardP(iCxCardCount)->IsPresent() )
			SMF_GOTOS( EStLoop )					// card's not present

		TUint32 arg = TUint32(iCardArray->CardP(iCxCardCount)->RCA()) << 16;
		s.FillCommandDesc(ECmdSelectCard, arg);
		SMF_INVOKES(ExecCommandSMST, EStCardSelectedGotStatus)

	SMF_STATE(EStCardSelectedGotStatus)

		OstTrace0( TRACE_INTERNALS, DMMCSTACK_CHECKLOCKSTATUSSM3, "EStCardSelectedGotStatus" );
		__KTRACE_OPT(KPBUS1, Kern::Printf("-mst:cssm:err%08x", err));
		OstTrace1( TRACE_INTERNALS, DMMCSTACK_CHECKLOCKSTATUSSM4, "err=0x%08x", err );
		if ( !err )
			{
			TMMCard& card=*(iCardArray->CardP(iCxCardCount));
			card.iStatus=s.ResponseP(); // Got the response

			iMinorBufLen = Max(iMinorBufLen, 1 << card.MaxReadBlLen());

			// this function is only called as part of the power up sequence, so
			// take the opportunity to record if it has a password
			if((card.iStatus & KMMCStatCardIsLocked) != 0)
				{
				card.iFlags |= KMMCardHasPassword;
				}
				
			// If the status suggests that the card is unlocked, we test
			// for the presence of a password by attempting to lock the card
			// (if we have a password in the store).  This handles conditions
			// where a card has not been fully powered down before reapplying power.
			if(!(card.iFlags & KMMCardHasPassword))
				{
				TMapping* pmp = iSocket->iPasswordStore->FindMappingInStore(card.CID());
				if(pmp)
					{
					if(pmp->iState == TMapping::EStValid)
						{
						const TInt kPWD_LEN = pmp->iPWD.Length();
						iPSLBuf[0] = KMMCLockUnlockLockUnlock;	// LOCK_UNLOCK = 1, SET_PWD = 0, CLR_PWD = 0
						iPSLBuf[1] = static_cast<TUint8>(kPWD_LEN);
						TPtr8 pwd(&iPSLBuf[2], kPWD_LEN);
						pwd.Copy(pmp->iPWD);

						const TInt kBlockLen = 1 + 1 + kPWD_LEN;

						// Need to use CIMReadWriteBlocksSMST to ensure that the
						// card is connected and the block length is set correctly
						s.SetCard(iCardArray->CardP(iCxCardCount));
						m.SetTraps(KMMCErrStatus | KMMCErrUpdPswd);
						s.FillCommandDesc(ECmdLockUnlock);
						s.FillCommandArgs(0, kBlockLen, iPSLBuf, kBlockLen);

						TMMCCommandDesc& cmd = s.Command();
						cmd.iUnlockRetries = 0;
						
						SMF_INVOKES(CIMReadWriteBlocksSMST,EStCheckLockStatus)
						}
					}
				}
			}

		SMF_GOTOS( EStLoop )

	SMF_STATE(EStCheckLockStatus)

		OstTrace0( TRACE_INTERNALS, DMMCSTACK_CHECKLOCKSTATUSSM5, "EStCheckLockStatus" );
		__KTRACE_OPT(KPBUS1, Kern::Printf("-mst:cssm:err%08x", err));
		OstTrace1( TRACE_INTERNALS, DMMCSTACK_CHECKLOCKSTATUSSM6, "err=0x%08x", err );

		if ((err & KMMCErrUpdPswd) || 
		   ((err & KMMCErrStatus) && (s.LastStatus().Error() == KMMCStatErrLockUnlock)))
			{
			// ECMDLockUnlock (with LockUnlockLockUnlock param) succeeded.
			// (either locked successfully, or we have attempted to lock a locked card)
			// Now determine if the card really is locked by checking the lock status.
			TMMCard& card=*(iCardArray->CardP(iCxCardCount));
			card.iStatus=s.LastStatus();
			if((card.iStatus & KMMCStatCardIsLocked) != 0)
				{
				card.iFlags |= KMMCardHasPassword;
				}
			}

		s.FillCommandDesc(ECmdSelectCard, 0);
		SMF_INVOKES(ExecCommandSMST, EStCardDeselected)
			
	SMF_STATE(EStCardDeselected)
		OstTrace0( TRACE_INTERNALS, DMMCSTACK_CHECKLOCKSTATUSSM7, "EStCardDeselected" );
		SMF_GOTOS( EStLoop )

	SMF_END
	}

EXPORT_C TMMCErr DMMCStack::ModifyCardCapabilitySM()
//
// This function provides a chance to modify the capability of paticular cards.
// Licensee may overide this function to modify certain card's capability as needed.
// A state machine is needed in derived function and function of base class should be
// called in order to act more generic behaviour.
//
    {
		enum states
			{
			EStBegin=0,
			EStEnd
			};

    SMF_BEGIN

    SMF_END
    }

//
// Timers
//

inline TMMCErr DMMCStack::PollGapTimerSM()
/**
 * Starts the poll timer.
 *
 * This may be used when executing CIM_UPDATE_ACQ when handling cards which are 
 * slow to power-up/reset and return busy following the issuing of CMD1.
 *
 * @return MMC error code.
 */
	{
		enum states
			{
			EStBegin=0,
			EStEnd
			};
#ifdef __EPOC32__
		DMMCSession& s=Session();
		OstTrace1( TRACE_INTERNALS, DMMCSTACK_POLLGAPTIMERSM1, "Current session=0x%x", &s );
#endif

	SMF_BEGIN
		OstTrace0( TRACE_INTERNALS, DMMCSTACK_POLLGAPTIMERSM2, "EStBegin" );
#ifdef __EPOC32__
		s.SynchBlock( KMMCBlockOnPollTimer );
		s.iPollTimer.OneShot(KMMCPollGapInMilliseconds,EFalse);

		SMF_EXITWAIT
#endif

	SMF_END
	}

inline TMMCErr DMMCStack::RetryGapTimerSM()
/**
 * Starts the retry timer. 
 *
 * This may be used when executing CIM_UPDATE_ACQ. When initialising the stack,
 * CMD0 is issued twice to get the bus in a known state and this timer is used 
 * to time the gap between issuing the two CMD0 commands.
 *
 * @return MMC error code.
 */
	{
		enum states
			{
			EStBegin=0,
			EStEnd
			};
#ifdef __EPOC32__
		DMMCSession& s=Session();
		OstTrace1( TRACE_INTERNALS, DMMCSTACK_RETRYGAPTIMERSM1, "Current session=0x%x", &s );
#endif

	SMF_BEGIN
		OstTrace0( TRACE_INTERNALS, DMMCSTACK_RETRYGAPTIMERSM2, "EStBegin" );
#ifdef __EPOC32__
		s.SynchBlock( KMMCBlockOnRetryTimer );
		s.iRetryTimer.OneShot(KMMCRetryGapInMilliseconds,EFalse);

		SMF_EXITWAIT
#endif

	SMF_END
	}

inline TMMCErr DMMCStack::ProgramTimerSM()
/**
 * Starts the program timer.
 *
 * This is used during write operartions to a card to sleep for an PSL-dependent period 
 * between issuing send status commands (CMD13). This is required in order to check when 
 * the card has finished writing its data to payload memory.
 *
 * @return MMC error code.
 */
	{
	enum states
		{
		EStBegin = 0,
		EStEnd
		};

#ifdef __EPOC32__
	DMMCSession &s = Session();
	OstTrace1( TRACE_INTERNALS, DMMCSTACK_PROGRAMTIMERSM1, "Current session=0x%x", &s );
#endif

	SMF_BEGIN
		OstTrace0( TRACE_INTERNALS, DMMCSTACK_PROGRAMTIMERSM2, "EStBegin" );
#ifdef __EPOC32__
		s.SynchBlock(KMMCBlockOnPgmTimer);
		s.iProgramTimer.Cancel();
		s.iProgramTimer.OneShot(ProgramPeriodInMilliSeconds(),EFalse);

		SMF_EXITWAIT
#endif
	SMF_END
	}

TMMCErr DMMCStack::LowVoltagePowerupTimerSM()
/**
 * Starts the low voltage power-up timer
 * NB Re-uses the retry gap timer.
 *
 * This is used after powering the bus off and then on after discovering that 
 * both the controller and card support low voltage operation.
 *
 * @return MMC error code.
 */
	{
	enum states
		{
		EStBegin = 0,
		EStEnd
		};

#ifdef __EPOC32__
	DMMCSession &s = Session();
	OstTrace1( TRACE_INTERNALS, DMMCSTACK_LOWVOLTAGEPOWERUPTIMERSM1, "Current session=0x%x", &s );
#endif

	SMF_BEGIN
		OstTrace0( TRACE_INTERNALS, DMMCSTACK_LOWVOLTAGEPOWERUPTIMERSM2, "EStBegin" );
#ifdef __EPOC32__
		s.SynchBlock(KMMCBlockOnRetryTimer);
		s.iRetryTimer.OneShot(KMMCLowVoltagePowerUpTimeoutInMilliseconds,EFalse);

		SMF_EXITWAIT
#endif
	SMF_END
	}


inline TMMCErr DMMCStack::ExecCommandSM()
/**
 * The main command executor. 
 * Depending on the main command being issued, this macro may result in the issuing of whole sequence 
 * of commands as it prepares the bus for the command in question. 
 *
 * In certain circumstances, this first issues one or more de-select commands (CMD7 + reserved RCA) 
 * to get the bus in a known state. It then analyses the main command and if necessary, selects the 
 * card in question (CMD7 + target RCA). 
 *
 * For block transfer commands, it will set the block length on the card concerned (CMD16) if this has 
 * not been done already. Likewise, for SD Cards, if the bus width has not yet been set-up, it will issue 
 * the appropriate bus width command (ACMD6). 
 * 
 * Finally it issues the main command requested before performing any error recovery that may be necessary 
 * following this. 
 *
 * In all cases it calls the generic layer child function IssueCommandCheckResponseSM() to execute each command.
 *
 * @return MMC error code
 */
	{
		enum states
			{
			EStBegin=0,
			EStExecCmd,
			EStRetry,
			EStDeselectLoop,
			EStDeselectEndCheck,
			EStAnalyseCommand,
			EStSelectDone,
			EStBlockCountCmdIssued,
			EStTestAppCommand,
			EStIssueAppCommandDone,
			EStIssueCommand,
			EStCommandIssued,
			EStErrRecover,
			EStEnd
			};

		DMMCSession& s=Session();
		OstTrace1( TRACE_INTERNALS, DMMCSTACK_EXECCOMMANDSM1, "Current session=0x%x", &s );
		
	SMF_BEGIN

		OstTrace0( TRACE_INTERNALS, DMMCSTACK_EXECCOMMANDSM2, "EStBegin" );
		if ( ( CurrentSessCardRCA() != 0 ) && ( (s.CardP()->iStatus.State()) == ECardStateSlp) )
			{
			// Currently selected media is asleep, so it must be awoken
			SMF_INVOKES(ExecAwakeCommandSMST,EStExecCmd)
			}
	
	SMF_STATE(EStExecCmd)
	
		OstTrace0( TRACE_INTERNALS, DMMCSTACK_EXECCOMMANDSM3, "EStExecCmd" );
		TMMCCommandDesc& cmd = s.Command();
		// clearup some internally used flags
		cmd.iFlags &= ~(KMMCCmdFlagExecTopBusy|KMMCCmdFlagExecSelBusy);
		cmd.iPollAttempts = cmd.iTimeOutRetries = cmd.iCRCRetries = 0;

	SMF_STATE(EStRetry)

		OstTrace0( TRACE_INTERNALS, DMMCSTACK_EXECCOMMANDSM4, "EStRetry" );
		TMMCCommandDesc& cmd = s.Command();
		m.SetTraps( KMMCErrBasic & ~Command().iExecNotHandle);	// Processing all trappable errors

		if (iMultiplexedBus)
			{
			if(cmd.iCommand == ECmdSelectCard)
				{
				DeselectsToIssue(1);
				}

			if (iConfig.iModes & KMMCModeCardControlled)
				{
				DoSetBusWidth(BusWidthEncoding(s.CardP()->BusWidth()));
				DoSetClock(MaxTranSpeedInKilohertz(*s.CardP()));

				// Check if this card is already in the appropriate selected/deselected
				// state for the forthcoming command.
				if (CurrentSessCardRCA() != iSelectedCard)
					{
					DeselectsToIssue(1);
					}
				}
			}

		// If bus context is unknown, issue DESELECT a few times with a RetryGap between them.
		if ( (iStackState & KMMCStackStateDoDeselect) == 0 )
			SMF_GOTOS( EStAnalyseCommand )

		// Save the top-level command while we issue de-selects
		CurrentSessPushCmdStack();
		CurrentSessFillCmdDesc( ECmdSelectCard, 0 );      // Deselect - RCA=0
		iCxDeselectCount=iDeselectsToIssue;

	// Fall through to the next state
	SMF_STATE(EStDeselectLoop)
		OstTrace0( TRACE_INTERNALS, DMMCSTACK_EXECCOMMANDSM5, "EStDeselectLoop" );
		SMF_INVOKES(IssueCommandCheckResponseSMST,EStDeselectEndCheck)	

	SMF_STATE(EStDeselectEndCheck)

		OstTrace0( TRACE_INTERNALS, DMMCSTACK_EXECCOMMANDSM6, "EStDeselectEndCheck" );
		// If we got an error and this is the last de-select then give up
		if (err && iCxDeselectCount == 1)
			{
		    CurrentSessPopCmdStack();
			OstTraceFunctionExitExt( DMMCSTACK_EXECCOMMANDSM_EXIT1, this, (TInt) err );
			SMF_RETURN(err)
			}

		if (--iCxDeselectCount > 0)
			SMF_INVOKES(RetryGapTimerSMST,EStDeselectLoop)

		CurrentSessPopCmdStack();
		iStackState &= ~KMMCStackStateDoDeselect;

	// Fall through to the next state
	SMF_STATE(EStAnalyseCommand)

		OstTrace0( TRACE_INTERNALS, DMMCSTACK_EXECCOMMANDSM7, "EStAnalyseCommand" );
		TMMCCommandDesc& cmd = s.Command();
		// Check if its un-important whether the card is in transfer state (i.e
		// selected) or not for the command we are about to execute. Such
		// commands are CMD0, CMD7 and CMD13.
		
		// This state machine should never send CMD55
		if (cmd.iCommand == ECmdAppCmd)
		    {
		    OstTraceFunctionExitExt( DMMCSTACK_EXECCOMMANDSM_EXIT2, this, (TInt) KMMCErrNotSupported );
			SMF_RETURN (KMMCErrNotSupported)
		    }

		SMF_NEXTS( EStTestAppCommand )
		if (cmd.iCommand == ECmdGoIdleState || cmd.iCommand == ECmdSelectCard || cmd.iCommand == ECmdSendStatus)
			{
			SMF_GOTONEXTS
			}

		// See if we need to select (or deselect) this card
		TRCA targetRCA=0;
		switch( cmd.iSpec.iCommandType )
			{
			case ECmdTypeBC: case ECmdTypeBCR: case ECmdTypeAC:
			// Command which don't require the card to be selected
				break;
			case ECmdTypeACS: case ECmdTypeADTCS: case ECmdTypeADC:
			// Commands which do require the card to be selected
				{
				if ( (iConfig.iModes & KMMCModeCardControlled) == 0 )
					SMF_GOTONEXTS
				// Get the RCA of the card
				if ( (targetRCA = s.CardRCA()) == 0 )
				    {
				    OstTraceFunctionExitExt( DMMCSTACK_EXECCOMMANDSM_EXIT3, this, (TInt) KMMCErrNoCard );
					SMF_RETURN( KMMCErrNoCard )
				    }
				break;
				}
			default:
				SMF_GOTONEXTS
			}

		// Check if this card is already in the appropriate selected/deselected
		// state for the forthcoming command.
		if (targetRCA==iSelectedCard)
			SMF_GOTONEXTS

		// Need to select (or deselect by using RCA(0)) the card so push the
		// top-level command onto the command stack while we issue the select command.
		CurrentSessPushCmdStack();
		s.FillCommandDesc(ECmdSelectCard,targetRCA);
		SMF_INVOKES(IssueCommandCheckResponseSMST,EStSelectDone)	

	SMF_STATE(EStSelectDone)

		OstTrace0( TRACE_INTERNALS, DMMCSTACK_EXECCOMMANDSM8, "EStSelectDone" );
		TMMCCommandDesc& cmd = s.Command();
		
		if ( err )
			{
			cmd.iFlags &= ~(KMMCCmdFlagASSPFlags|KMMCCmdFlagExecSelBusy);

			if (err == KMMCErrBusyTimeOut)
				cmd.iFlags |= KMMCCmdFlagExecSelBusy;

			CurrentSessPopCmdStack();
			SMF_NEXTS(EStErrRecover)
			OstTraceFunctionExitExt( DMMCSTACK_EXECCOMMANDSM_EXIT4, this, (TInt) err );
			return err;		// re-enter the next state with that error
			}

		// Are we trying to recover from a top-level command returning busy (by de-selecting and re-selecting)
		if ( cmd.iFlags & KMMCCmdFlagExecTopBusy )
			{
			cmd.iFlags &= ~KMMCCmdFlagExecTopBusy;

			TUint32 blockLength = cmd.BlockLength();

			if ( !(cmd.iSpec.iMultipleBlocks) || cmd.iTotalLength <= blockLength )
				SMF_EXIT		// operation is completed

			cmd.iTotalLength    -= blockLength;
			cmd.iArgument        = TUint(cmd.iArgument) + blockLength;
			cmd.iDataMemoryP    += blockLength;
			s.iBytesTransferred += blockLength;
			cmd.iPollAttempts = 0;
			}

		CurrentSessPopCmdStack();
		
		cmd = s.Command();
		if (!cmd.iSpec.iUseStopTransmission && cmd.iSpec.iMultipleBlocks)
			{
			// Multi-block command using SET_BLOCK_COUNT
			// This is a re-try of the data transfer, normally select (CMD7) is performed along with the issuing of CMD23, 
			// therefore need to re-issue SET_BLOCK_COUNT.....
	  		const TUint blocks = cmd.NumBlocks();
	
			CurrentSessPushCmdStack();
			s.FillCommandDesc( ECmdSetBlockCount, blocks );
			SMF_INVOKES( IssueCommandCheckResponseSMST, EStBlockCountCmdIssued )
			}
		else
			{
			SMF_GOTOS( EStTestAppCommand )
			}

	SMF_STATE(EStBlockCountCmdIssued)
		
		OstTrace0( TRACE_INTERNALS, DMMCSTACK_EXECCOMMANDSM9, "EStBlockCountCmdIssued" );
		const TMMCStatus status(s.ResponseP());
		CurrentSessPopCmdStack();
		if (status.Error())
		    {
		    OstTraceFunctionExitExt( DMMCSTACK_EXECCOMMANDSM_EXIT5, this, (TInt) KMMCErrStatus );
			SMF_RETURN(KMMCErrStatus)
		    }

		if(err & KMMCErrNotSupported)
			{
			// Not supported by the PSL, so use standard Stop Transmission mode
			s.Command().iSpec.iUseStopTransmission = ETrue;
			}
		// Fall through...		

	SMF_STATE(EStTestAppCommand)
		OstTrace0( TRACE_INTERNALS, DMMCSTACK_EXECCOMMANDSM10, "EStTestAppCommand" );
		TMMCCommandDesc& cmd = s.Command();
		if (cmd.iSpec.iCommandClass != KMMCCmdClassApplication)
			SMF_GOTOS( EStIssueCommand )

		CurrentSessPushCmdStack();
		s.FillCommandDesc(ECmdAppCmd, s.CardRCA()); // Send APP_CMD (CMD55)	
		SMF_INVOKES(IssueCommandCheckResponseSMST,EStIssueAppCommandDone)
		
	SMF_STATE(EStIssueAppCommandDone)
		OstTrace0( TRACE_INTERNALS, DMMCSTACK_EXECCOMMANDSM11, "EStIssueAppCommandDone" );
		CurrentSessPopCmdStack();
		if ( err )
			{
			SMF_NEXTS(EStErrRecover)
            OstTraceFunctionExitExt( DMMCSTACK_EXECCOMMANDSM_EXIT6, this, (TInt) err );
			return err;		// re-enter the next state with that error
			}


	SMF_STATE(EStIssueCommand)

		OstTrace0( TRACE_INTERNALS, DMMCSTACK_EXECCOMMANDSM12, "EStIssueCommand" );
		TMMCCommandDesc& cmd = s.Command();
		// If its an addressed command (rather than a selected command), then
		// setup the argument with the RCA. (Commands requiring card to be
		// selected - ACS don't matter since selected cards don't need an RCA now).
		if ((iConfig.iModes & KMMCModeCardControlled) && cmd.iSpec.iCommandType==ECmdTypeAC )
			{
			const TRCA targetRCA = s.CardRCA();
			if ( targetRCA == 0 )
			    {
			    OstTraceFunctionExitExt( DMMCSTACK_EXECCOMMANDSM_EXIT7, this, (TInt) KMMCErrNoCard );
				SMF_RETURN( KMMCErrNoCard )
			    }
			cmd.iArgument.SetRCA(targetRCA);
			}

		// Issue the top-level command
		SMF_INVOKES(IssueCommandCheckResponseSMST,EStCommandIssued)

	SMF_STATE(EStCommandIssued)

		OstTrace0( TRACE_INTERNALS, DMMCSTACK_EXECCOMMANDSM13, "EStCommandIssued" );
		// If command was succesful then we've finished.
		if (!err)
			SMF_EXIT

	SMF_STATE(EStErrRecover)

		OstTrace0( TRACE_INTERNALS, DMMCSTACK_EXECCOMMANDSM14, "EStErrRecover" );
		TMMCCommandDesc& cmd = s.Command();
		const TUint32 modes=iConfig.iModes;
		SMF_NEXTS(EStRetry)

		m.ResetTraps();							// no more re-entries via return()

		if (cmd.iCommand == ECmdSelectCard)
			DeselectsToIssue( 1 );	// in case stby/tran synch is lost

		if (cmd.iSpec.iMultipleBlocks && (cmd.iFlags & KMMCCmdFlagBytesValid))
			{
			cmd.iTotalLength -= cmd.iBytesDone;
			cmd.iArgument = TUint(cmd.iArgument) + cmd.iBytesDone;
			cmd.iDataMemoryP += cmd.iBytesDone;
			s.iBytesTransferred += cmd.iBytesDone;

			if (cmd.iTotalLength < cmd.BlockLength())
				{
				DeselectsToIssue(1);
				OstTraceFunctionExitExt( DMMCSTACK_EXECCOMMANDSM_EXIT8, this, (TInt) err );
				return err;
				}
			}

		if ((modes & KMMCModeEnableRetries) == 0)
		    {
			OstTraceFunctionExitExt( DMMCSTACK_EXECCOMMANDSM_EXIT9, this, (TInt) err );
			return err;
		    }

		const TUint32 toMask = (KMMCErrResponseTimeOut|KMMCErrDataTimeOut);
		const TUint32 crcMask = (KMMCErrResponseCRC|KMMCErrDataCRC|KMMCErrCommandCRC);

		if( (err & ~(toMask|crcMask)) == KMMCErrNone ) // time-outs/CRC errors
			{
			if( cmd.iSpec.iCommandType == ECmdTypeADTCS )	// for data transfer commands
				{
				DeselectsToIssue( 1 );						// enforce deselect before any retries

				if( (modes & KMMCModeCardControlled) == 0 )
				    {
					OstTraceFunctionExitExt( DMMCSTACK_EXECCOMMANDSM_EXIT10, this, (TInt) err );
					return err;		// we wouldn't know what to select - no retries
				    }
				}

			TUint32 gapEnabled = 0;

			if( err & toMask )
				{
				cmd.iTimeOutRetries++;
				gapEnabled |= KMMCModeTimeOutRetryGap;

				if( (modes & KMMCModeEnableTimeOutRetry) == 0 ||
					cmd.iTimeOutRetries > iConfig.iTimeOutRetries )
				    {
					OstTraceFunctionExitExt( DMMCSTACK_EXECCOMMANDSM_EXIT11, this, (TInt) err );
					return err;
				    }
				}

			if( err & crcMask )
				{
				cmd.iCRCRetries++;
				gapEnabled |= KMMCModeCRCRetryGap;

				if( (modes & KMMCModeEnableCRCRetry) == 0	||
					cmd.iCRCRetries > iConfig.iCRCRetries		||
					((err & KMMCErrDataCRC) != 0 && (modes & KMMCModeDataCRCRetry) == 0) )
				    {
					OstTraceFunctionExitExt( DMMCSTACK_EXECCOMMANDSM_EXIT12, this, (TInt) err );
					return err;
				    }
				}

			if( (modes & gapEnabled) == gapEnabled )
				{
				if( modes & KMMCModeCardControlled )
					s.iState |= KMMCSessStateSafeInGaps;	// preemption allowed

				SMF_CALL( RetryGapTimerSMST )
				}

			if( (modes & (KMMCModeEnablePreemption|KMMCModePreemptOnRetry|KMMCModeCardControlled)) ==
				(KMMCModeEnablePreemption|KMMCModePreemptOnRetry|KMMCModeCardControlled) )
				{
				s.SwapMe();
				SMF_WAIT		// let the others take over the bus before retry
				}

			// No preemption, just repeat the command
			SMF_GOTONEXTS
			}

		if( err & KMMCErrBusInconsistent )
			{
			// ASSP layer reported that we must re-initialise the stack to recover.
			// Here we'll allow stack initialiser to take over. The control will then be transferred
			// to whoever processes KMMCErrInitContext (must be a top-level SM function)

//			ReportInconsistentBusState();	// ASSP layer should have it done
			s.iGlobalRetries++;

			if( s.iGlobalRetries > KMMCMaxGlobalRetries )
			    {
				OstTraceFunctionExitExt( DMMCSTACK_EXECCOMMANDSM_EXIT13, this, (TInt) err );
				return err;
			    }

			s.SwapMe();		// To prevent the initialiser from aborting this session
			SMF_WAIT		// Initialiser will take over here
			}

		if( err == KMMCErrBusyTimeOut )
			{
			if ((cmd.iFlags & KMMCCmdFlagExecSelBusy) == 0)	// check if that was a response
				cmd.iFlags |= KMMCCmdFlagExecTopBusy;		// to a top level command

			DeselectsToIssue( 1 );	// force deselect as the next bus operation
			cmd.iPollAttempts++;

			if( (modes & KMMCModeEnableBusyPoll) == 0 ||
				((modes & KMMCModeCardControlled) == 0 && cmd.iCommand != ECmdSelectCard) ||
				cmd.iPollAttempts > iConfig.iPollAttempts )
			    {
				OstTraceFunctionExitExt( DMMCSTACK_EXECCOMMANDSM_EXIT14, this, (TInt) err );
				return err;
			    }

			if( modes & KMMCModeBusyPollGap )
				{
				s.iState |= KMMCSessStateSafeInGaps;	// preemption allowed
				SMF_CALL( PollGapTimerSMST )
				}

			if( (modes & (KMMCModeEnablePreemption|KMMCModePreemptOnBusy)) ==
				(KMMCModeEnablePreemption|KMMCModePreemptOnBusy) )
				{
				s.SwapMe();
				SMF_WAIT		// let the others take over the bus before retry
				}

			// No preemption, just repeat the Deselect/Select sequence
			SMF_GOTONEXTS
			}

		OstTraceFunctionExitExt( DMMCSTACK_EXECCOMMANDSM_EXIT15, this, (TInt) err );
		return err;

	SMF_END
	}

TMMCErr DMMCStack::IssueCommandCheckResponseSM()
/**
 * Issue a single command to the card and check the response 
 *
 * This generic layer child function in turn calls IssueMMCCommandSM().
 *
 * @return MMC error code.
 */
	{
	enum states
		{
		EStBegin=0,
		EStIssueCommand,
		EStCommandIssued,
		EStEnd
		};

		DMMCSession& s=Session();
		TMMCCommandDesc& cmd = Command();
		OstTrace1( TRACE_INTERNALS, DMMCSTACK_ISSUECOMMANDCHECKRESPONSESM1, "Current session=0x%x", &s );

	SMF_BEGIN

		OstTrace0( TRACE_INTERNALS, DMMCSTACK_ISSUECOMMANDCHECKRESPONSESM2, "EStBegin" );
		__KTRACE_OPT(KPBUS1,Kern::Printf(">MMC:Issue %d %x",TUint(cmd.iCommand),TUint(cmd.iArgument)));
		OstTraceExt2( TRACE_INTERNALS, DMMCSTACK_ISSUECOMMANDCHECKRESPONSESM3, "CMD%02d(0x%08x)", TUint(cmd.iCommand), TUint(cmd.iArgument) );
		
		// Stop the Controller from powering down the card due to bus inactivity
		iSocket->ResetInactivity(0);

	SMF_STATE(EStIssueCommand)

		OstTrace0( TRACE_INTERNALS, DMMCSTACK_ISSUECOMMANDCHECKRESPONSESM4, "EStIssueCommand" );
		// If command is directed at a specific card then save this command in card object
		if (iConfig.iModes & KMMCModeCardControlled)
			{
			s.iCardP->iLastCommand = cmd.iCommand;

			if(iMultiplexedBus)
				{
				DoSetBusWidth(BusWidthEncoding(s.CardP()->BusWidth()));
				DoSetClock(MaxTranSpeedInKilohertz(*s.CardP()));
				}
			}

		if (cmd.iCommand==ECmdSelectCard)
			iSelectedCard = TUint16(~0);

		// Pass the command to ASSP layer
		cmd.iFlags &= ~(KMMCCmdFlagASSPFlags|KMMCCmdFlagExecSelBusy);
		cmd.iBytesDone=0;
		m.SetTraps(KMMCErrAll);
		SMF_INVOKES(IssueMMCCommandSMST,EStCommandIssued)

	SMF_STATE(EStCommandIssued)

		OstTrace0( TRACE_INTERNALS, DMMCSTACK_ISSUECOMMANDCHECKRESPONSESM5, "EStCommandIssued" );
#ifdef ENABLE_DETAILED_SD_COMMAND_TRACE
		cmd.Dump(s.ResponseP(), err);
#endif

		OstTraceDefExt2( OST_TRACE_CATEGORY_RND, TRACE_MMCDEBUG, DMMCSTACK_ISSUECOMMANDCHECKRESPONSESM6, "MMC Protocol: CMD%02d(0x%08x)", (TInt) cmd.iCommand, (TUint) cmd.iArgument );
		OstTraceDefExt4( OST_TRACE_CATEGORY_RND, TRACE_MMCDEBUG, DMMCSTACK_ISSUECOMMANDCHECKRESPONSESM7, "MMC Protocol: RSP%d - LEN 0x%08x - ERR 0x%08x - STAT 0x%08x", (TUint) cmd.iSpec.iResponseType, (TUint) cmd.iSpec.iResponseLength, (TUint) err, (TUint) TMMC::BigEndian32(s.ResponseP()) );
		
		TMMCErr exitCode=err;
		// If we have just de-selected all cards in the stack, RCA(0) then ignore response timeout.
		if ( cmd.iCommand==ECmdSelectCard && TRCA(cmd.iArgument)==0 )
			exitCode &= ~KMMCErrResponseTimeOut;
		else
			{
			// If commands returns card status and there we no command errors
			// (or the status contains errors) then save the status info.
			if ( (cmd.iFlags & KMMCCmdFlagStatusReceived) ||
				 ((exitCode==KMMCErrNone || (exitCode & KMMCErrStatus)) &&
				  (cmd.iSpec.iResponseType==ERespTypeR1 || cmd.iSpec.iResponseType==ERespTypeR1B)) )
				{
				TMMCStatus status=s.ResponseP();
				s.iLastStatus=status;
				__KTRACE_OPT(KPBUS1, Kern::Printf("mmc:ec:st=%08x", TUint32(status)));
				OstTrace1( TRACE_INTERNALS, DMMCSTACK_ISSUECOMMANDCHECKRESPONSESM8, "status=0x%08x", TUint32(status) );

				if (iConfig.iModes & KMMCModeCardControlled)
					s.iCardP->iStatus=status;

				// Update exit code if card status is reporting an error (in case not done already)
				if (status.Error() != 0)
					exitCode |= KMMCErrStatus;
				}
			}

		// If we've just selected a card and the command was succesful then
		// remember which one so we don't need to do it twice.
		if (cmd.iCommand==ECmdSelectCard && exitCode==KMMCErrNone)
			iSelectedCard=TRCA(cmd.iArgument);

		OstTraceFunctionExitExt( DMMCSTACK_ISSUECOMMANDCHECKRESPONSESM_EXIT, this, ( TInt ) exitCode );
		SMF_RETURN(exitCode)

	SMF_END
	}

//
// General Client Service CIMs/Sessions; top level functions
//
inline TMMCErr DMMCStack::NakedSessionSM()
/**
 * Executes an individual MMC command (as opposed to a macro).
 *
 * If the command is 'card controlled' it first invokes AttachCardSM() 
 * before calling ExecCommandSM() to excecute the requested command.
 *
 * @return MMC error code.
 */
	{
		enum states
			{
			EStBegin=0,
			EStAttached,
			EStFinish,
			EStEnd
			};

		DMMCSession& s=Session();
		OstTrace1( TRACE_INTERNALS, DMMCSTACK_NAKEDSESSIONSM1, "Current session=0x%x", &s );

	SMF_BEGIN
		OstTrace0( TRACE_INTERNALS, DMMCSTACK_NAKEDSESSIONSM2, "EStBegin" );
		s.iState |= KMMCSessStateInProgress;

		if( (iConfig.iModes & KMMCModeCardControlled) != 0 )
			SMF_INVOKES( AttachCardSMST, EStAttached )

	SMF_BPOINT(EStAttached)

		OstTrace0( TRACE_INTERNALS, DMMCSTACK_NAKEDSESSIONSM3, "EStAttached" );
		SMF_INVOKES( ExecCommandSMST, EStFinish )

	SMF_STATE(EStFinish)

		OstTrace0( TRACE_INTERNALS, DMMCSTACK_NAKEDSESSIONSM4, "EStFinish" );
		s.iState &= ~KMMCSessStateInProgress;
	SMF_END
	}

inline TMMCErr DMMCStack::CIMSetupCardSM()
/**
 * Executes the CIM_SETUP_CARD macro.
 *
 * @return MMC error code.
 */
	{
		enum states
			{
			EStBegin=0,
			EStAttached,
			EStSelected,
			EStGotCSD,
			EStEnd
			};

		DMMCSession& s=Session();
		OstTraceExt2( TRACE_INTERNALS, DMMCSTACK_CIMSETUPCARDSM1, "Current session=0x%x; Last status=0x%x", (TUint) &s, (TUint) s.iLastStatus );

		__KTRACE_OPT(KPBUS1,Kern::Printf(">MMC:SetupCardSM %x",TUint(s.iLastStatus)));

	SMF_BEGIN
	
		OstTrace0( TRACE_INTERNALS, DMMCSTACK_CIMSETUPCARDSM2, "EStBegin" );
		s.iState |= KMMCSessStateInProgress;

		SMF_INVOKES( AttachCardSMST, EStAttached )	// attachment is mandatory here

	SMF_BPOINT(EStAttached)

		OstTrace0( TRACE_INTERNALS, DMMCSTACK_CIMSETUPCARDSM3, "EStAttached" );
		s.FillCommandDesc( ECmdSelectCard, 0 );
		SMF_INVOKES( ExecCommandSMST, EStSelected )

	SMF_STATE(EStSelected)

		OstTrace0( TRACE_INTERNALS, DMMCSTACK_CIMSETUPCARDSM4, "EStSelected" );
		if( !s.iCardP->IsReady() )
		    {
			OstTraceFunctionExitExt( DMMCSTACK_CIMSETUPCARDSM_EXIT, this, (TInt) KMMCErrNoCard );
			return KMMCErrNoCard;
		    }

		s.FillCommandDesc( ECmdSendCSD, Command().iArgument ); // NB: the card will be deselected to execute this command
		SMF_INVOKES( ExecCommandSMST, EStGotCSD )

	SMF_STATE(EStGotCSD)

		OstTrace0( TRACE_INTERNALS, DMMCSTACK_CIMSETUPCARDSM5, "EStGotCSD" );
		s.iCardP->iCSD = s.ResponseP();

		s.iState &= ~KMMCSessStateInProgress;
	SMF_END
	}

EXPORT_C TMMCErr DMMCStack::CIMReadWriteBlocksSM()
/**
 * This macro performs single/multiple block reads and writes.
 *
 * For normal read/write block operations, this function determines the appropriate
 * MMC command to send and fills the command descriptor accordingly based on 
 * the value of the session ID set. However, it is necessary to have set the
 * command arguments (with DMMCSession::FillCommandArgs()) before this function
 * is called.
 *
 * For special block read/write operations, e.g. lock/unlock, it is required to
 * have already filled the command descriptor (with DMMCSession::FillCommandDesc())
 * for the special command required - in addition to have setup the command arguments.
 *
 * @return MMC error code.
 */
	{
		enum states
			{
			EStBegin=0,
			EStRestart,
			EStAttached,
			EStLength1,
			EStLengthSet,
			EStIssueBlockCount,
			EStAppCmdIssued,
			EStBlockCountCmdIssued,
			EStBpoint1,
			EStIssued,
			EStWaitFinish,
			EStWaitFinish1,
			EStRWFinish,
			EStEnd
			};

		DMMCSession& s=Session();
		OstTraceExt2( TRACE_INTERNALS, DMMCSTACK_CIMREADWRITEBLOCKSSM1, "Current session=0x%x; Last status=0x%x", (TUint) &s, (TUint) s.iLastStatus );

		__KTRACE_OPT(KPBUS1,Kern::Printf(">MMC:RWBlocksSM %x",TUint(s.iLastStatus)));

	SMF_BEGIN

		OstTrace0( TRACE_INTERNALS, DMMCSTACK_CIMREADWRITEBLOCKSSM2, "EStBegin" );
		if(s.SessionID() == ECIMWriteBlock || s.SessionID() == ECIMWriteMBlock)
			{
			// Check that the card supports class 4 (Write) commands
			const TUint ccc = s.iCardP->CSD().CCC();
			if(!(ccc & KMMCCmdClassBlockWrite))
			    {
				OstTraceFunctionExitExt( DMMCSTACK_CIMREADWRITEBLOCKSSM_EXIT1, this, (TInt) KMMCErrNotSupported );
				return KMMCErrNotSupported;
			    }
			}

		s.iState |= KMMCSessStateInProgress;
		m.SetTraps(KMMCErrInitContext);

	SMF_STATE(EStRestart)		// NB: ErrBypass is not processed here

		OstTrace0( TRACE_INTERNALS, DMMCSTACK_CIMREADWRITEBLOCKSSM3, "EStRestart" );
		SMF_CALLMEWR(EStRestart) // Create a recursive call entry to recover from the errors trapped
		m.SetTraps(KMMCErrStatus);
		if (s.Command().iSpec.iCommandClass!=KMMCCmdClassApplication || s.Command().iCommand==ECmdAppCmd)
			{
			s.ResetCommandStack();
			SMF_INVOKES( AttachCardSMST, EStAttached )	// attachment is mandatory here
			}

	SMF_BPOINT(EStAttached)

		OstTrace0( TRACE_INTERNALS, DMMCSTACK_CIMREADWRITEBLOCKSSM4, "EStAttached" );
		TMMCCommandDesc& cmd = s.Command();

		const TUint32 blockLength = cmd.BlockLength();
		if(blockLength == 0)
		    {
			OstTraceFunctionExitExt( DMMCSTACK_CIMREADWRITEBLOCKSSM_EXIT2, this, (TInt) KMMCErrArgument );
			return KMMCErrArgument;
		    }

		if(s.iSessionID == ECIMReadBlock	||
		   s.iSessionID == ECIMWriteBlock	||
		   s.iSessionID == ECIMReadMBlock	||
		   s.iSessionID == ECIMWriteMBlock)
			{	
			// read/write operation
			if(!cmd.AdjustForBlockOrByteAccess(s))
				{
				// unable to convert command arguments to suit the underlying block/byte access mode
				OstTraceFunctionExitExt( DMMCSTACK_CIMREADWRITEBLOCKSSM_EXIT3, this, (TInt) KMMCErrArgument );
				return KMMCErrArgument;
				}
			}

		// Set the block length if it has changed. Always set for ECIMLockUnlock.
		if ((blockLength == s.iCardP->iSetBlockLen) && (s.iSessionID != ECIMLockUnlock))
			{
			SMF_GOTOS( EStLengthSet )
			}

		s.iCardP->iSetBlockLen = 0;
		CurrentSessPushCmdStack();
		s.FillCommandDesc( ECmdSetBlockLen, blockLength );
		SMF_INVOKES( ExecCommandSMST, EStLength1 )

	SMF_STATE(EStLength1)

		OstTrace0( TRACE_INTERNALS, DMMCSTACK_CIMREADWRITEBLOCKSSM5, "EStAttached" );
		const TMMCStatus status(s.ResponseP());
		CurrentSessPopCmdStack();
		if (status.Error())
		    {
		    OstTraceFunctionExitExt( DMMCSTACK_CIMREADWRITEBLOCKSSM_EXIT4, this, (TInt) KMMCErrStatus );
			SMF_RETURN(KMMCErrStatus)
		    }
		s.iCardP->iSetBlockLen = s.Command().BlockLength();

	SMF_STATE(EStLengthSet)

		OstTrace0( TRACE_INTERNALS, DMMCSTACK_CIMREADWRITEBLOCKSSM6, "EStLengthSet" );
		TMMCCommandDesc& cmd = s.Command();
		TUint opType = 0;
		const TUint kTypeWrite =	KBit0;
		const TUint kTypeMultiple =	KBit1;
		const TUint kTypeSpecial =	KBit2;
		static const TMMCCommandEnum cmdCodes[4] =
			{ECmdReadSingleBlock, ECmdWriteBlock, ECmdReadMultipleBlock, ECmdWriteMultipleBlock};

		switch( s.iSessionID )
			{
			case ECIMReadBlock:
				break;
			case ECIMWriteBlock:
				opType=kTypeWrite;
				break;
			case ECIMReadMBlock:
				opType=kTypeMultiple;
				break;
			case ECIMWriteMBlock:
				opType=kTypeWrite|kTypeMultiple;
				break;
			case ECIMLockUnlock:
			default:
				opType=kTypeSpecial;
				break;
			}

  		const TUint blocks = cmd.NumBlocks();

		SMF_NEXTS(EStBpoint1)
		if ( !(opType & kTypeSpecial) )	// A special session has already set its command descriptor
			{
			
			if (cmd.iFlags & KMMCCmdFlagReliableWrite)
				//ensure multiple block commands are used for reliable writing
				opType |= kTypeMultiple;
			
			if ( (blocks==1) && !(cmd.iFlags & KMMCCmdFlagReliableWrite) )
				{
				// Reliable Write requires that Multi-Block command is used.
				opType &= ~kTypeMultiple;
				}

			TUint32 oldFlags = cmd.iFlags;	// Maintain old flags which would be overwritten by FillCommandDesc
			cmd.iCommand = cmdCodes[opType];
			s.FillCommandDesc();
			cmd.iFlags = oldFlags;			// ...and restore

			if((opType & kTypeMultiple) == kTypeMultiple)
				{
				//
				// This is a Multiple-Block DT command.  Check the version of the card, as
				// MMC Version 3.1 onwards supports the SET_BLOCK_COUNT mode for DT.
				//
				// Note that if the PSL does not support pre-determined block count of
				// data transmission, then it may return KMMCErrNotSupported to force
				// the 'Stop Transmission' mode to be used instead.
				//
				if(s.iCardP->CSD().SpecVers() >= 3)
					{
					OstTrace0( TRACE_INTERNALS, DMMCSTACK_CIMREADWRITEBLOCKSSM7, "CMD12 (STOP_TRANSMISSION) not used" );
					
					cmd.iSpec.iUseStopTransmission = EFalse;
					SMF_NEXTS(EStIssueBlockCount)
					}
				else
					{
					cmd.iSpec.iUseStopTransmission = ETrue;
					}
				}
			}

		SMF_GOTONEXTS

	SMF_STATE(EStIssueBlockCount)
		OstTrace0( TRACE_INTERNALS, DMMCSTACK_CIMREADWRITEBLOCKSSM8, "EStIssueBlockCount" );
		//
		// Issues SET_BLOCK_COUNT (CMD23) for MB R/W data transfers.
		// This is only issued if MMC version >= 3.1 and the PSL
		// supports this mode of operation.
		//
		TMMCCommandDesc& cmd = s.Command();
  		TUint32 args = (TUint16)cmd.NumBlocks();
	
		m.SetTraps(KMMCErrStatus | KMMCErrNotSupported);

		if (cmd.iFlags & KMMCCmdFlagReliableWrite)
			{
			// Command marked as Reliable Write 
			// set Bit31 in CMD23 argument
			args |= KMMCCmdReliableWrite;
			}
		
		CurrentSessPushCmdStack();
		s.FillCommandDesc( ECmdSetBlockCount, args );
		SMF_INVOKES( ExecCommandSMST, EStBlockCountCmdIssued )

	SMF_STATE(EStBlockCountCmdIssued)
		
		OstTrace0( TRACE_INTERNALS, DMMCSTACK_CIMREADWRITEBLOCKSSM9, "EStBlockCountCmdIssued" );
		const TMMCStatus status(s.ResponseP());
		CurrentSessPopCmdStack();
		if (status.Error())
		    {
		    OstTraceFunctionExitExt( DMMCSTACK_CIMREADWRITEBLOCKSSM_EXIT5, this, (TInt) KMMCErrStatus );
			SMF_RETURN(KMMCErrStatus)
		    }

		if(err & KMMCErrNotSupported)
			{
			// Not supported by the PSL, so use standard Stop Transmission mode
			s.Command().iSpec.iUseStopTransmission = ETrue;
			}

		SMF_GOTOS(EStBpoint1)

	SMF_STATE(EStAppCmdIssued)

		OstTrace0( TRACE_INTERNALS, DMMCSTACK_CIMREADWRITEBLOCKSSM10, "EStAppCmdIssued" );
		const TMMCStatus status(s.ResponseP());
		CurrentSessPopCmdStack();
		if (status.Error())
		    {
		    OstTraceFunctionExitExt( DMMCSTACK_CIMREADWRITEBLOCKSSM_EXIT6, this, (TInt) KMMCErrStatus );
			SMF_RETURN(KMMCErrStatus)
		    }

	SMF_BPOINT(EStBpoint1)

		OstTrace0( TRACE_INTERNALS, DMMCSTACK_CIMREADWRITEBLOCKSSM11, "EStBpoint1" );
		// NB We need to trap KMMCErrStatus errors, because if one occurs, 
		// we still need to wait to exit PRG/RCV/DATA state 
		m.SetTraps(KMMCErrStatus);

		SMF_INVOKES( ExecCommandSMST, EStIssued )

	SMF_STATE(EStIssued)

		OstTrace0( TRACE_INTERNALS, DMMCSTACK_CIMREADWRITEBLOCKSSM12, "EStIssued" );
		// check state of card after data transfer with CMD13.

		if (s.Command().Direction() != 0)
			{
			SMF_GOTOS(EStWaitFinish)
			}

		SMF_GOTOS(EStRWFinish);

	SMF_STATE(EStWaitFinish)

		OstTrace0( TRACE_INTERNALS, DMMCSTACK_CIMREADWRITEBLOCKSSM13, "EStWaitFinish" );
		// Save the status and examine it after issuing CMD13...
		// NB We don't know where in the command stack the last response is stored (e.g. there may 
		// have bee a Deselect/Select issued), but we do know last response is stored in iLastStatus
		TMMC::BigEndian4Bytes(s.ResponseP(), s.iLastStatus);

		CurrentSessPushCmdStack();
		s.FillCommandDesc(ECmdSendStatus, 0);
		SMF_INVOKES(ExecCommandSMST, EStWaitFinish1)

	SMF_STATE(EStWaitFinish1)
		OstTrace0( TRACE_INTERNALS, DMMCSTACK_CIMREADWRITEBLOCKSSM14, "EStWaitFinish1" );
		const TMMCStatus status(s.ResponseP());
		CurrentSessPopCmdStack();

#ifdef __WINS__
		SMF_GOTOS(EStRWFinish);
#else
		const TMMCardStateEnum st1 = status.State();
		if (st1 == ECardStatePrg || st1 == ECardStateRcv || st1 == ECardStateData)
			{
			SMF_INVOKES(ProgramTimerSMST, EStWaitFinish);
			}
		if (status.Error())
		    {
		    OstTraceFunctionExitExt( DMMCSTACK_CIMREADWRITEBLOCKSSM_EXIT7, this, (TInt) KMMCErrStatus );
			SMF_RETURN(KMMCErrStatus)
		    }
#endif
		
		// Fall through if CURRENT_STATE is not PGM or DATA
	SMF_STATE(EStRWFinish)

		OstTrace0( TRACE_INTERNALS, DMMCSTACK_CIMREADWRITEBLOCKSSM15, "EStRWFinish" );
		if (TMMCStatus(s.ResponseP()).Error() != 0)
		    {
		    OstTraceFunctionExitExt( DMMCSTACK_CIMREADWRITEBLOCKSSM_EXIT8, this, (TInt) KMMCErrStatus );
			SMF_RETURN(KMMCErrStatus);
		    }

		s.iState &= ~KMMCSessStateInProgress;

		// skip over recursive entry or throw error and catch in CIMLockUnlockSM()
		TMMCErr ret = (s.Command().iCommand == ECmdLockUnlock) ? KMMCErrUpdPswd : KMMCErrBypass; 
		OstTraceFunctionExitExt( DMMCSTACK_CIMREADWRITEBLOCKSSM_EXIT9, this, (TInt) ret );
		return ret;

	SMF_END
	}

inline TMMCErr DMMCStack::CIMEraseSM()
/**
 * This macro performs sector/group erase of a continuous area
 *
 * @return MMC error code.
 */
	{
		enum states
			{
			EStBegin=0,
			EStRestart,
			EStAttached,
			EStStartTagged,
			EStEndTagged,
			EStErased,
			EStWaitFinish,
			EStWaitFinish1,
			EStEraseFinish,
			EStEnd
			};

		DMMCSession& s=Session();
		OstTraceExt2( TRACE_INTERNALS, DMMCSTACK_CIMERASESM1, "Current session=0x%x; Last status=0x%x", (TUint) &s, (TUint) s.iLastStatus );
		

		__KTRACE_OPT(KPBUS1,Kern::Printf(">MMC:EraseSM %x",TUint(s.iLastStatus)));

	SMF_BEGIN

		OstTrace0( TRACE_INTERNALS, DMMCSTACK_CIMERASESM2, "EStBegin" );
		// Check that the card supports class 4 (Write) commands
		const TUint ccc = s.iCardP->CSD().CCC();
		if(!(ccc & KMMCCmdClassErase))
		    {
			OstTraceFunctionExitExt( DMMCSTACK_CIMERASESM_EXIT1, this, (TInt) KMMCErrNotSupported );
			return KMMCErrNotSupported;
		    }

		DoAddressCard(s.iCardP->iIndex-1);

		s.iState |= KMMCSessStateInProgress;
		m.SetTraps( KMMCErrInitContext );

	SMF_STATE(EStRestart)

		OstTrace0( TRACE_INTERNALS, DMMCSTACK_CIMERASESM3, "EStRestart" );
		SMF_CALLMEWR(EStRestart) // Create a recursive call entry to recover from Init

		s.ResetCommandStack();
		SMF_INVOKES( AttachCardSMST, EStAttached )		// attachment is mandatory

	SMF_BPOINT(EStAttached)

		OstTrace0( TRACE_INTERNALS, DMMCSTACK_CIMERASESM4, "EStAttached" );
		TMMCCommandDesc& cmd = s.Command();

		if(cmd.iTotalLength == 0)
		    {
			OstTraceFunctionExitExt( DMMCSTACK_CIMERASESM_EXIT2, this, (TInt) KMMCErrArgument );
			return KMMCErrArgument;
		    }

		switch( s.iSessionID )
			{
			case ECIMEraseSector:
				OstTrace0( TRACE_INTERNALS, DMMCSTACK_CIMERASESM5, "ECIMEraseSector" );
				TMMCEraseInfo eraseInfo;
				s.iCardP->GetEraseInfo(eraseInfo);
				cmd.iBlockLength = eraseInfo.iMinEraseSectorSize;
				cmd.iCommand = ECmdTagSectorStart;
				break;
			case ECIMEraseGroup:
				OstTrace0( TRACE_INTERNALS, DMMCSTACK_CIMERASESM6, "ECIMEraseGroup" );
				cmd.iBlockLength = s.iCardP->iCSD.EraseGroupSize();
				if(cmd.iBlockLength == 0 || cmd.iTotalLength % cmd.iBlockLength != 0)
				    {
					OstTraceFunctionExitExt( DMMCSTACK_CIMERASESM_EXIT3, this, (TInt) KMMCErrArgument );
					return KMMCErrArgument;
				    }
				cmd.iCommand = ECmdTagEraseGroupStart;
				break;
			default:
				DMMCSocket::Panic(DMMCSocket::EMMCEraseSessionID);
			}

		if(!cmd.AdjustForBlockOrByteAccess(s))
		    {
			OstTraceFunctionExitExt( DMMCSTACK_CIMERASESM_EXIT4, this, (TInt) KMMCErrArgument );
			return KMMCErrArgument;
		    }

		iConfig.RemoveMode( KMMCModeEnablePreemption );	// erase sequence must not be pre-empted

		const TUint flags = cmd.iFlags;
		s.FillCommandDesc();
		cmd.iFlags = flags;
		SMF_INVOKES( ExecCommandSMST, EStStartTagged )

	SMF_STATE(EStStartTagged)

		OstTrace0( TRACE_INTERNALS, DMMCSTACK_CIMERASESM7, "EStStartTagged" );
		const TMMCCommandDesc& cmd = s.Command();

		TMMCCommandEnum command;
		TUint endAddr = cmd.iArgument;
		if(s.iSessionID == ECIMEraseSector)
			{
			endAddr += cmd.IsBlockCmd() ? (cmd.iTotalLength / cmd.iBlockLength - 1) : (cmd.iTotalLength - cmd.iBlockLength);
			command = ECmdTagSectorEnd;
			}
		else
			{
			if(cmd.IsBlockCmd())
				{
				endAddr += (cmd.iTotalLength - cmd.iBlockLength) >> KMMCardHighCapBlockSizeLog2;
				}
			else
				{
				endAddr += cmd.iTotalLength - cmd.iBlockLength;
				}
	
			command = ECmdTagEraseGroupEnd;
			}

		CurrentSessPushCmdStack();
		s.FillCommandDesc( command, endAddr );
		SMF_INVOKES( ExecCommandSMST, EStEndTagged )

	SMF_STATE(EStEndTagged)

		OstTrace0( TRACE_INTERNALS, DMMCSTACK_CIMERASESM8, "EStEndTagged" );
		// Increase the inactivity timeout as an erase operation can potentially take a long time
		// At the moment this is a somewhat arbitrary 30 seconds. This could be calculated more accurately
		// using TAAC,NSAC, R2W_FACTOR etc. but that seems to yield very large values (?)
		const TInt KMaxEraseTimeoutInSeconds = 30;
		iBody->SetInactivityTimeout(KMaxEraseTimeoutInSeconds);
		m.SetTraps(KMMCErrAll);
		s.FillCommandDesc( ECmdErase, 0 );
		SMF_INVOKES( ExecCommandSMST, EStErased )

	SMF_STATE(EStErased)
		OstTrace0( TRACE_INTERNALS, DMMCSTACK_CIMERASESM9, "EStErased" );
		m.SetTraps( KMMCErrInitContext );
		iBody->RestoreInactivityTimeout();
		if (err != KMMCErrNone)
		    {
		    OstTraceFunctionExitExt( DMMCSTACK_CIMERASESM_EXIT5, this, (TInt) err );
			SMF_RETURN(err);
		    }


	SMF_STATE(EStWaitFinish)

		OstTrace0( TRACE_INTERNALS, DMMCSTACK_CIMERASESM10, "EStWaitFinish" );
		CurrentSessPushCmdStack();
		s.FillCommandDesc(ECmdSendStatus, 0);
		SMF_INVOKES(ExecCommandSMST, EStWaitFinish1)

	SMF_STATE(EStWaitFinish1)

		OstTrace0( TRACE_INTERNALS, DMMCSTACK_CIMERASESM11, "EStWaitFinish1" );
		const TMMCStatus st(s.ResponseP());
		CurrentSessPopCmdStack();

#ifdef __WINS__
		SMF_GOTOS(EStEraseFinish);
#else
		const TMMCardStateEnum st1 = st.State();
		if (st1 == ECardStatePrg || st1 == ECardStateRcv || st1 == ECardStateData)
			{
			SMF_INVOKES(ProgramTimerSMST, EStWaitFinish);
			}
#endif
		
		// Fall through if CURRENT_STATE is not PGM or DATA
	SMF_STATE(EStEraseFinish)

		OstTrace0( TRACE_INTERNALS, DMMCSTACK_CIMERASESM12, "EStEraseFinish" );
		s.iState &= ~KMMCSessStateInProgress;
		OstTraceFunctionExitExt( DMMCSTACK_CIMERASESM_EXIT6, this, (TInt) KMMCErrBypass );
		return KMMCErrBypass;		// to skip over the recursive entry

	SMF_END
	}

inline TMMCErr DMMCStack::CIMReadWriteIOSM()
/**
 * This macro reads/writes a stream of bytes from/to an I/O register. 
 * This is a generalised form of FAST_IO (CMD39) MMC command.
 *
 * @return MMC error code.
 */
	{
		enum states
			{
			EStBegin=0,
			EStReadIO,
			EStIOLoop,
			EStEnd
			};

		DMMCSession& s=Session();
		TMMCCommandDesc& cmd = s.Command();
		OstTraceExt2( TRACE_INTERNALS, DMMCSTACK_CIMREADWRITEIOSM1, "Current session=0x%x; Last status=0x%x", (TUint) &s, (TUint) s.iLastStatus );
		

		__KTRACE_OPT(KPBUS1,Kern::Printf(">MMC:IOSM %x",TUint(s.iLastStatus)));

	SMF_BEGIN
		OstTrace0( TRACE_INTERNALS, DMMCSTACK_CIMREADWRITEIOSM2, "EStBegin" );
		s.iState |= KMMCSessStateInProgress;
		TUint argument = (TUint(cmd.iArgument)&0x7F) << 8; // shift reg addr into a proper position

		switch( s.iSessionID )
			{
			case ECIMReadIO:
				break;
			case ECIMWriteIO:
				argument |= 0x8000;
				break;
			default:
				DMMCSocket::Panic(DMMCSocket::EMMCIOSessionID);
			}

		s.FillCommandDesc( ECmdFastIO, argument );
		s.iBytesTransferred = ~0UL;

		SMF_INVOKES( AttachCardSMST, EStIOLoop )	// attachment's mandatory

	SMF_STATE(EStReadIO)

		OstTrace0( TRACE_INTERNALS, DMMCSTACK_CIMREADWRITEIOSM3, "EStReadIO" );
		*(cmd.iDataMemoryP)++ = s.ResponseP()[3];
		cmd.iTotalLength--;

	SMF_BPOINT(EStIOLoop)

		OstTrace0( TRACE_INTERNALS, DMMCSTACK_CIMREADWRITEIOSM4, "EStIOLoop" );
		s.iBytesTransferred++;

		if( cmd.iTotalLength == 0 )
			{
			s.iState &= ~KMMCSessStateInProgress;
			SMF_EXIT
			}

		if( s.iSessionID == ECIMWriteIO )
			{
			TUint8 byte = *(cmd.iDataMemoryP)++;
			cmd.iTotalLength--;
			cmd.iArgument = (TUint(cmd.iArgument)&0xFF00) | byte;
			}
		else
			SMF_NEXTS(EStReadIO)

		iConfig.RemoveMode( KMMCModeEnableRetries );	// no retries on I/O registers!

		SMF_CALL( ExecCommandSMST )

	SMF_END
	}

inline TMMCErr DMMCStack::CIMLockUnlockSM()
/**
 * Locking and unlocking a card involves writing a data block to the card.
 * CIMReadWriteBlocksSM() could be used directly, but, in practive, a card must
 * sometimes be sent the data block twice.
 *
 * @return MMC error code.
 */
	{
		enum states
			{
			EStBegin,
			EStRetry,
			EStTestR1,
			EStEnd
			};

		DMMCSession& s=Session();
		TMMCCommandDesc& cmd = Command();
		OstTrace1( TRACE_INTERNALS, DMMCSTACK_CIMLOCKUNLOCKSM1, "Current session=0x%x", &s );

		__KTRACE_OPT(KPBUS1, Kern::Printf("mmc:clusm"));

	SMF_BEGIN
        
		OstTrace0( TRACE_INTERNALS, DMMCSTACK_CIMLOCKUNLOCKSM2, "EStBegin" );
		m.SetTraps(KMMCErrStatus | KMMCErrUpdPswd);
		cmd.iUnlockRetries = 0;					// attempt counter
		iCMD42CmdByte = cmd.iDataMemoryP[0];

	SMF_STATE(EStRetry)
		OstTrace0( TRACE_INTERNALS, DMMCSTACK_CIMLOCKUNLOCKSM3, "EStRetry" );
		__KTRACE_OPT(KPBUS1, Kern::Printf("mmc:clusm:%x/%x", cmd.iUnlockRetries, (iSessionP == &iAutoUnlockSession) ? KMMCMaxAutoUnlockRetries : iConfig.iUnlockRetries));

	if (iCMD42CmdByte == KMMCLockUnlockErase)
		{
		// Section 4.6.2 of version 4.2 of the the MMC specification states that 
		// the maximum time for a force erase operation should be 3 minutes 
		const TInt KMaxForceEraseTimeoutInSeconds = 3 * 60;
		iBody->SetInactivityTimeout(KMaxForceEraseTimeoutInSeconds);
		m.SetTraps(KMMCErrAll);
		}


		SMF_INVOKES(CIMReadWriteBlocksSMST, EStTestR1);
	
	SMF_STATE(EStTestR1)
		OstTrace0( TRACE_INTERNALS, DMMCSTACK_CIMLOCKUNLOCKSM4, "EStTestR1" );
		if (iCMD42CmdByte == KMMCLockUnlockErase)
			{
			m.SetTraps(KMMCErrStatus | KMMCErrUpdPswd);
			iBody->RestoreInactivityTimeout();
			}

		if (err & KMMCErrStatus)
			{
			const TMMCStatus st = s.LastStatus();	// set in ExecCommandSM() / EStCommandIssued
			TMMCCommandDesc& cmd0 = Command();
			
			__KTRACE_OPT(KPBUS1, Kern::Printf("mmc:clusm:EStTestR1 [err: %08x, st:%08x] : RETRY [%d]", 
											  err, (TInt)s.LastStatus(), cmd0.iUnlockRetries));
			OstTraceExt3( TRACE_INTERNALS, DMMCSTACK_CIMLOCKUNLOCKSM5, "err=%08x; Last status=%d; Unlock retries=%d", (TUint) err, (TInt) s.LastStatus(), (TUint) cmd0.iUnlockRetries );

			const TInt KMaxRetries = (iSessionP == &iAutoUnlockSession) ? KMMCMaxAutoUnlockRetries : iConfig.iUnlockRetries;
			
			// retry if LOCK_UNLOCK_FAIL only error bit
			if (!(	iCMD42CmdByte == 0	// LOCK_UNLOCK = 0; SET_PWD = 0
				&&	err == KMMCErrStatus && st.Error() == KMMCStatErrLockUnlock
				&&	(iConfig.iModes & KMMCModeEnableUnlockRetry) != 0
				&&	++cmd0.iUnlockRetries < KMaxRetries ))
				{
				__KTRACE_OPT(KPBUS1, Kern::Printf("mmc:clusm:abt"));
                OstTrace0( TRACE_INTERNALS, DMMCSTACK_CIMLOCKUNLOCKSM6, "LockUnlock abort" );
                OstTraceFunctionExitExt( DMMCSTACK_CIMLOCKUNLOCKSM_EXIT, this, (TInt) err );
				SMF_RETURN(err);
				}

			__KTRACE_OPT(KPBUS1, Kern::Printf("mmc:clusm:retry"));
			OstTrace0( TRACE_INTERNALS, DMMCSTACK_CIMLOCKUNLOCKSM7, "LockUnlock retry" );

#ifdef __EPOC32__
			s.SynchBlock(KMMCBlockOnRetryTimer);
			s.iRetryTimer.OneShot(KMMCUnlockRetryGapInMilliseconds,EFalse);
			SMF_WAITS(EStRetry)
#else
			SMF_GOTOS(EStRetry);
#endif
			}
		else if (err & KMMCErrUpdPswd)
			{
			// CMD42 executed successfully, so update 'Has Password' flag
			if ((iCMD42CmdByte & (KMMCLockUnlockClrPwd | KMMCLockUnlockErase)) != 0)
				{
				s.CardP()->iFlags&=(~KMMCardHasPassword);
				}
			else if ((iCMD42CmdByte & KMMCLockUnlockSetPwd) != 0)
				{
				s.CardP()->iFlags|=KMMCardHasPassword;
				}
			
			SMF_EXIT;
			}
		else if (err != KMMCErrNone)
			{
			OstTraceFunctionExitExt( DMMCSTACK_CIMLOCKUNLOCKSM_EXIT2, this, (TInt) err );
			SMF_RETURN(err);
			}


	SMF_END
	}

inline TMMCErr DMMCStack::CIMAutoUnlockSM()
/**
 * Performs auto-unlocking of the card stack
 *
 * @return MMC error code
 */
	{
		enum states
			{
			EStBegin=0,
			EStNextIndex,
			EStSendStatus,
			EStGetStatus,
			EStUnlock,
			EStInitStackAfterUnlock,
			EStIssuedLockUnlock,
			EStDone,
			EStEnd
			};

		__KTRACE_OPT(KPBUS1,Kern::Printf(">MMC:CIMAutoUnlockSM"));

		DMMCSession& s=Session();
		OstTrace1( TRACE_INTERNALS, DMMCSTACK_CIMAUTOUNLOCKSM1, "Current session=0x%x", &s );

	SMF_BEGIN

		OstTrace0( TRACE_INTERNALS, DMMCSTACK_CIMAUTOUNLOCKSM2, "EStBegin" );
		iAutoUnlockIndex = -1;

		m.SetTraps(KMMCErrAll);	// Trap (and ignore) all errors during auto-unlock

	SMF_STATE(EStNextIndex)

		OstTrace0( TRACE_INTERNALS, DMMCSTACK_CIMAUTOUNLOCKSM3, "EStNextIndex" );
		if(err)
			{
			iSocket->PasswordControlEnd(&Session(), err);
			}

		// the cycle is finished when iAutoUnlockIndex == KMaxMultiMediaCardsPerStack
		
		if(iAutoUnlockIndex >= TInt(KMaxMMCardsPerStack))
			{
			SMF_GOTOS(EStInitStackAfterUnlock);
			}

		// find next available card with password in the controller store

		const TMapping *mp = NULL;

		TBool useIndex = EFalse;
		for (++iAutoUnlockIndex; iAutoUnlockIndex < TInt(KMaxMMCardsPerStack); ++iAutoUnlockIndex)
			{
			useIndex =							// card must be present with valid mapping
					iCardArray->CardP(iAutoUnlockIndex)->IsPresent()
				&&	(mp = iSocket->iPasswordStore->FindMappingInStore(iCardArray->CardP(iAutoUnlockIndex)->CID())) != NULL
				&&	mp->iState == TMapping::EStValid;

			// don't increment iAutoUnlockIndex in continuation loop
			if (useIndex)
				break;
			}

		if (! useIndex)
			{
			// if no usable index, complete with no error code
			SMF_GOTOS(EStInitStackAfterUnlock);
			}

		//
		// We've found a locked card with a password in the password store,
		// so attempt to unlock using the CIMLockUnlockSMST state machine.
		//
		// Upon completion, test the next card before performing further initialisation.
		//
		
		DoAddressCard(iAutoUnlockIndex); 	// Address the card
		TMMCard* cd = iCardArray->CardP(iAutoUnlockIndex);
		s.SetCard(cd);
	
	// Fall through to the next state
	SMF_STATE(EStSendStatus)
		        
		s.FillCommandDesc(ECmdSendStatus, 0);
		                        
		SMF_INVOKES(ExecCommandSMST,EStGetStatus)
		                        
	SMF_STATE(EStGetStatus)
		                        
		const TMMCStatus st = s.LastStatus();
		if((st & KMMCStatCardIsLocked) == 0)
			{
			SMF_GOTOS(EStNextIndex);
		    }

	// Fall through to the next state	                        
	SMF_STATE(EStUnlock)
		                        
		const TMapping *mp = NULL;
		mp = iSocket->iPasswordStore->FindMappingInStore(iCardArray->CardP(iAutoUnlockIndex)->CID());
		__ASSERT_DEBUG(mp, DMMCSocket::Panic(DMMCSocket::EMMCMachineState));

		OstTrace1( TRACE_INTERNALS, DMMCSTACK_CIMAUTOUNLOCKSM4, "Attempting to unlock card %d", iCardArray->CardP(iAutoUnlockIndex)->Number() );
		
		const TInt kPWD_LEN = mp->iPWD.Length();
		iPSLBuf[0] = 0;				// LOCK_UNLOCK = 0; unlock
		iPSLBuf[1] = static_cast<TUint8>(kPWD_LEN);
		TPtr8 pwd(&iPSLBuf[2], kPWD_LEN);
		pwd.Copy(mp->iPWD);

		const TInt kBlockLen = 1 + 1 + kPWD_LEN;

		s.FillCommandDesc(ECmdLockUnlock);
		s.FillCommandArgs(0, kBlockLen, iPSLBuf, kBlockLen);

		SMF_INVOKES( CIMLockUnlockSMST, EStNextIndex )

	SMF_STATE(EStInitStackAfterUnlock)

		OstTrace0( TRACE_INTERNALS, DMMCSTACK_CIMAUTOUNLOCKSM5, "EStInitStackAfterUnlock" );
		//
		// We've attempted to unlock all cards (successfully or not)
		//  - Now perform second-stage initialisation that can only be performed
		//    on unlocked cards (such as setting bus width, high speed etc..)
		//

		m.ResetTraps();

		SMF_INVOKES( InitStackAfterUnlockSMST, EStDone )

	SMF_STATE(EStDone)
		OstTrace0( TRACE_INTERNALS, DMMCSTACK_CIMAUTOUNLOCKSM6, "EStDone" );

	SMF_END
	}

inline TMMCErr DMMCStack::NoSessionSM()
/**
 * A null state machine function. Just returns KMMCErrNotSupported.
 *
 * @return KMMCErrNotSupported
 */
	{
		enum states
			{
			EStBegin=0,
			EStEnd
			};

	SMF_BEGIN

		return( KMMCErrNotSupported );

	SMF_END
	}

//
// Static adapter functions to top-level state machines.
//

TMMCErr DMMCStack::NakedSessionSMST(TAny* aStackP)			// ECIMNakedSession
	{ return( static_cast<DMMCStack *>(aStackP)->NakedSessionSM() ); }

TMMCErr DMMCStack::CIMUpdateAcqSMST( TAny* aStackP )		// ECIMUpdateAcq
	{ return( static_cast<DMMCStack *>(aStackP)->CIMUpdateAcqSM() ); }

TMMCErr DMMCStack::CIMInitStackSMST( TAny* aStackP )		// ECIMInitStack
	{ return( static_cast<DMMCStack *>(aStackP)->CIMInitStackSM() ); }

TMMCErr DMMCStack::CIMCheckStackSMST( TAny* aStackP )		// ECIMCheckStack
	{ return( static_cast<DMMCStack *>(aStackP)->CIMCheckStackSM() ); }

TMMCErr DMMCStack::CIMSetupCardSMST(TAny* aStackP)			// ECIMSetupCard
	{ return( static_cast<DMMCStack *>(aStackP)->CIMSetupCardSM() ); }

EXPORT_C TMMCErr DMMCStack::CIMReadWriteBlocksSMST(TAny* aStackP)	// ECIMReadBlock, ECIMWriteBlock, ECIMReadMBlock, ECIMWriteMBlock
	{ return( static_cast<DMMCStack *>(aStackP)->CIMReadWriteBlocksSM() ); }

TMMCErr DMMCStack::CIMEraseSMST(TAny* aStackP)				// ECIMEraseSector, ECIMEraseGroup
	{ return( static_cast<DMMCStack *>(aStackP)->CIMEraseSM() ); }

TMMCErr DMMCStack::CIMReadWriteIOSMST(TAny* aStackP)		// ECIMReadIO, ECIMWriteIO
	{ return( static_cast<DMMCStack *>(aStackP)->CIMReadWriteIOSM() ); }

TMMCErr DMMCStack::CIMLockUnlockSMST(TAny* aStackP)			// ECIMLockUnlock
	{ return( static_cast<DMMCStack *>(aStackP)->CIMLockUnlockSM() ); }

TMMCErr DMMCStack::NoSessionSMST(TAny* aStackP)				// ECIMLockStack
	{ return( static_cast<DMMCStack *>(aStackP)->NoSessionSM() ); }

TMMCErr DMMCStack::InitStackAfterUnlockSMST( TAny* aStackP )		
	{ return( static_cast<DMMCStack *>(aStackP)->InitStackAfterUnlockSM() ); }

TMMCErr DMMCStack::AcquireStackSMST( TAny* aStackP )
	{ return( static_cast<DMMCStack *>(aStackP)->AcquireStackSM() ); }

TMMCErr DMMCStack::CheckStackSMST( TAny* aStackP )
	{ return( static_cast<DMMCStack *>(aStackP)->CheckStackSM() ); }

TMMCErr DMMCStack::ModifyCardCapabilitySMST( TAny* aStackP )
	{ return( static_cast<DMMCStack *>(aStackP)->ModifyCardCapabilitySM() ); }

TMMCErr DMMCStack::AttachCardSMST( TAny* aStackP )
	{ return( static_cast<DMMCStack *>(aStackP)->AttachCardSM() ); }

TMMCErr DMMCStack::ExecCommandSMST( TAny* aStackP )
	{ return( static_cast<DMMCStack *>(aStackP)->ExecCommandSM() ); }

TMMCErr DMMCStack::IssueCommandCheckResponseSMST(TAny* aStackP)
	{ return( static_cast<DMMCStack *>(aStackP)->IssueCommandCheckResponseSM() ); }

TMMCErr DMMCStack::PollGapTimerSMST( TAny* aStackP )
	{ return( static_cast<DMMCStack *>(aStackP)->PollGapTimerSM() ); }

TMMCErr DMMCStack::RetryGapTimerSMST( TAny* aStackP )
	{ return( static_cast<DMMCStack *>(aStackP)->RetryGapTimerSM() ); }

TMMCErr DMMCStack::ProgramTimerSMST(TAny *aStackP)
	{ return static_cast<DMMCStack *>(aStackP)->ProgramTimerSM(); }

TMMCErr DMMCStack::GoIdleSMST( TAny* aStackP )
	{ return( static_cast<DMMCStack *>(aStackP)->GoIdleSM() ); }

TMMCErr DMMCStack::CheckLockStatusSMST( TAny* aStackP )
	{ return( static_cast<DMMCStack *>(aStackP)->CheckLockStatusSM() ); }

TMMCErr DMMCStack::CIMAutoUnlockSMST( TAny* aStackP )
	{ return( static_cast<DMMCStack *>(aStackP)->CIMAutoUnlockSM() ); }

TMMCErr DMMCStack::ExecSleepCommandSMST( TAny* aStackP )
	{ return( static_cast<DMMCStack *>(aStackP)->ExecSleepCommandSM() ); }

TMMCErr DMMCStack::ExecAwakeCommandSMST( TAny* aStackP )
	{ return( static_cast<DMMCStack *>(aStackP)->ExecAwakeCommandSM() ); }
//
// Static interfaces to ASSP layer SM functions
//
TMMCErr DMMCStack::DoPowerUpSMST( TAny* aStackP )
	{ return( static_cast<DMMCStack *>(aStackP)->DoPowerUpSM() ); }

TMMCErr DMMCStack::InitClockOnSMST( TAny* aStackP )
	{ return( static_cast<DMMCStack *>(aStackP)->InitClockOnSM() ); }

EXPORT_C TMMCErr DMMCStack::IssueMMCCommandSMST( TAny* aStackP )
	{ return( static_cast<DMMCStack *>(aStackP)->IssueMMCCommandSM() ); }

TMMCErr DMMCStack::DetermineBusWidthAndClockSMST( TAny* aStackP )
	{ return( static_cast<DMMCStack *>(aStackP)->DetermineBusWidthAndClockSM() ); }

TMMCErr DMMCStack::ConfigureHighSpeedSMST( TAny* aStackP )
	{ return( static_cast<DMMCStack *>(aStackP)->ConfigureHighSpeedSM() ); }

TMMCErr DMMCStack::ExecSwitchCommandST( TAny* aStackP )
	{ return( static_cast<DMMCStack *>(aStackP)->ExecSwitchCommand() ); }

TMMCErr DMMCStack::SwitchToLowVoltageSMST( TAny* aStackP )
	{ return( static_cast<DMMCStack *>(aStackP)->SwitchToLowVoltageSM() ); }

TMMCErr DMMCStack::LowVoltagePowerupTimerSMST(TAny *aStackP)
	{ return static_cast<DMMCStack *>(aStackP)->LowVoltagePowerupTimerSM(); }

TMMCErr DMMCStack::ExecBusTestSMST( TAny* aStackP )
	{ return( static_cast<DMMCStack *>(aStackP)->ExecBusTestSM() ); }

TMMCErr DMMCStack::DoWakeUpSMST( TAny* aStackP )
	{ 	
	MDoWakeUp* dowakeup = NULL;
	static_cast<DMMCStack *>(aStackP)->GetInterface(KInterfaceDoWakeUpSM, (MInterface*&) dowakeup);
	if (dowakeup)
		{
		return dowakeup->DoWakeUpSM();
		}
	else
		{
		// Interface not supported at PSL Level
		return KMMCErrNotSupported;
		}
	}


EXPORT_C DMMCSession* DMMCStack::AllocSession(const TMMCCallBack& aCallBack) const
/**
* Factory function to create DMMCSession derived object.  Non-generic MMC
* controllers can override this to generate more specific objects.
* @param aCallBack Callback function to notify the client that a session has completed
* @return A pointer to the new session
*/
	{
	OstTraceFunctionEntry1( DMMCSTACK_ALLOCSESSION_ENTRY, this );
	return new DMMCSession(aCallBack);
	}

EXPORT_C void DMMCStack::Dummy1() {}

/**
 * Calls the PSL-implemented function SetBusWidth() if the bus width has changed
 *
 */
void DMMCStack::DoSetBusWidth(TUint32 aBusWidth)
	{
	OstTraceFunctionEntryExt( DMMCSTACK_DOSETBUSWIDTH_ENTRY, this );
	if (iBody->iCurrentSelectedBusWidth != aBusWidth)
		{
		iBody->iCurrentSelectedBusWidth = aBusWidth;
		SetBusWidth(aBusWidth);
		}
	OstTraceFunctionExit1( DMMCSTACK_DOSETBUSWIDTH_EXIT, this );
	}

/**
 * Sets iConfig.iBusConfig.iBusClock - which the PSL SHOULD use to set the clock before every command.
 *
 * Some PSLs, however, may only change the clock when SetBusConfigDefaults() is called, 
 * so if the clock has changed, SetBusConfigDefaults() is called
 *
 * @param aClock The requested clock frequency in Kilohertz
 */
void DMMCStack::DoSetClock(TUint32 aClock)
	{
	OstTraceFunctionEntryExt( DMMCSTACK_DOSETCLOCK_ENTRY, this );
	iConfig.iBusConfig.iBusClock = aClock;

	if (iPoweredUp&&(iBody->iCurrentSelectedClock != aClock))
		{
		iBody->iCurrentSelectedClock = aClock;
		SetBusConfigDefaults(iConfig.iBusConfig, aClock);
		}
	OstTraceFunctionExit1( DMMCSTACK_DOSETCLOCK_EXIT, this );
	}


TUint DMMCStack::MaxTranSpeedInKilohertz(const TMMCard& aCard) const
	{
	OstTraceFunctionEntry1( DMMCSTACK_MAXTRANSPEEDINKILOHERTZ_ENTRY, this );
	TUint32 highSpeedClock = aCard.HighSpeedClock();
	TUint ret = highSpeedClock ? highSpeedClock : aCard.MaxTranSpeedInKilohertz(); 
	OstTraceFunctionExitExt( DMMCSTACK_MAXTRANSPEEDINKILOHERTZ_EXIT, this, ret );
	return ret;
	}



EXPORT_C void DMMCStack::SetBusWidth(TUint32 /*aBusWidth*/)
	{
	}

EXPORT_C void DMMCStack::MachineInfo(TDes8& /*aMachineInfo*/)
	{
	}

TBusWidth DMMCStack::BusWidthEncoding(TInt aBusWidth) const
/**
 * Returns the bus width as a TBusWidth given a card's bus width 
 * expressed as an integer (1,4 or 8)
 * @return the bus width encoded as a TBusWidth
 */
	{
	OstTraceFunctionEntryExt( DMMCSTACK_BUSWIDTHENCODING_ENTRY, this );
	TBusWidth busWidth = EBusWidth1;

	switch(aBusWidth)
		{
		case 8: 
			busWidth =  EBusWidth8; 
			break;
		case 4: 
			busWidth =  EBusWidth4; 
			break;
		case 1: 
		case 0: 
			busWidth =  EBusWidth1; 
			break;
		default:
			DMMCSocket::Panic(DMMCSocket::EMMCBadBusWidth);

		}
	OstTraceFunctionExitExt( DMMCSTACK_BUSWIDTHENCODING_EXIT, this, ( TUint )&( busWidth ) );
	return busWidth;
	}

void DMMCStack::DoAddressCard(TInt aCardNumber)
	{
	MAddressCard* addressCardInterface = NULL;
	GetInterface(KInterfaceAddressCard, (MInterface*&) addressCardInterface);
	if (addressCardInterface)
		addressCardInterface->AddressCard(aCardNumber);
	else
		{
		// if the interface isn't supported on a multiplexed bus, then panic if the card number > 0 - 
		// one cause of this panic is if the PSL 's implementation of GetInterface() does not call the 
		// base class's implementation of GetInterface()
		__ASSERT_ALWAYS((!iMultiplexedBus) || (aCardNumber <= 0), DMMCSocket::Panic(DMMCSocket::EMMCAddressCardNotSupported));
		}
	}

/**
 * class DMMCSocket 
 */
EXPORT_C DMMCSocket::DMMCSocket(TInt aSocketNumber, TMMCPasswordStore* aPasswordStore)
/**
 * Constructs a DMMCSocket class
 * @param aSocketNumber the socket ID
 * @param aPasswordStore pointer to the password store
 */
	:DPBusSocket(aSocketNumber),
	iPasswordStore(aPasswordStore)
	{
	OstTraceFunctionEntryExt( DMMCSOCKET_DMMCSOCKET_ENTRY, this );
	}

TInt DMMCSocket::TotalSupportedCards()
/**
 * Returns the total number of MMC slots supported by the socket.
 * @return The number of MMC slots supported by the socket
 */
	{
	OstTraceFunctionEntry1( DMMCSOCKET_TOTALSUPPORTEDCARDS_ENTRY, this );
	OstTraceFunctionExitExt( DMMCSOCKET_TOTALSUPPORTEDCARDS_EXIT, this, iMachineInfo.iTotalSockets );
	return iMachineInfo.iTotalSockets;
	}


// -------- Password store management --------

//
// The persistent file is a contiguous sequence of entries.
// An entry format is [CID@16 | PWD_LEN@4 | PWD@PWD_LEN].
// CID and PWD_LEN are both stored in big endian format.
//

TInt DMMCSocket::PrepareStore(TInt aBus, TInt aFunc, TLocalDrivePasswordData &aData)
/**
 * Called from media driver before CMD42 session engaged, in kernel server context
 * so that mappings can be allocated or deallocated.
 * 
 * Using zero-length passwords for MMC operations is disallowed by this function.
 * Locking with and clearing a null password is failed with KErrAccessDenied.
 * If the drive is already mounted, then TBusLocalDrive::Unlock() will fail with
 * KErrAlreadyExists.  Otherwise, this function will fail with KErrLocked, which
 * is translated to KErrAccessDenied in Unlock(), in the same way as unlocking
 * a locked card with the wrong password
 * 
 * @param aBus The card to be unlocked.
 * @param aFunc The operation to perform (EPasswordLock, EPasswordUnlock, EPasswordClear).
 * @param aData TLocalDrivePasswordData reference containing the password
 * @return KErrAccessDenied An attempt to lock or clear was made with a NULL password.
 * @return KErrLocked An an attempt to unlock was made with a NULL password.
 * @return KErrNone on success
 */
	{
	OstTraceExt3(TRACE_FLOW, DMMCSOCKET_PREPARESTORE_ENTRY, "DMMCSocket::PrepareStore;aBus=%d;aFunc=%d;this=%x", aBus, aFunc, (TUint) this);
	TInt r = 0;

	TMMCard *card=iStack->CardP(aBus);
	__ASSERT_ALWAYS(card, Panic(EMMCSessionNoPswdCard));
	const TCID &cid = card->CID();

	switch (aFunc)
		{
	case DLocalDrive::EPasswordLock:
		{
		TMediaPassword newPwd = *aData.iNewPasswd;

		if (newPwd.Length() == 0)
			r = KErrAccessDenied;
		else
			r = PasswordControlStart(cid, aData.iStorePasswd ? &newPwd : NULL);
		}
		break;

	case DLocalDrive::EPasswordUnlock:
		{
		TMediaPassword curPwd = *aData.iOldPasswd;

		if (curPwd.Length() == 0)
			r = KErrLocked;
		else
			r = PasswordControlStart(cid, aData.iStorePasswd ? &curPwd : NULL);
		}
		break;

	case DLocalDrive::EPasswordClear:
		{
		TMediaPassword curPwd = *aData.iOldPasswd;

		if (curPwd.Length() == 0)
			r = KErrAccessDenied;
		else
			r = PasswordControlStart(cid, aData.iStorePasswd ? &curPwd : NULL);
		}
		break;

	default:
		Panic(EMMCSessionPswdCmd);
		break;
		}

	OstTraceFunctionExitExt( DMMCSOCKET_PREPARESTORE_EXIT, this, r );
	return r;
	}


TInt DMMCSocket::PasswordControlStart(const TCID &aCID, const TMediaPassword *aPWD)
/**
 * Remove any non-validated mappings from the store, and allocate a binding for
 * the card's CID if necessary.
 * 
 * s = source (current) password stored; t = target (new) password should be stored
 * f = failure
 * 
 * t is equivalent to iMPTgt.Length() > 0, which is used by PasswordControlEnd().
 * 
 * The target password is not stored in the store at this point, but in the stack.
 * This leaves any existing mapping which can be used for recovery if the operation
 * fails.  This means the user does not have to re-enter the right password after
 * trying to unlock a card with the wrong password.
 * 
 * See PasswordControlEnd() for recovery policy.
 */
	{
	OstTraceFunctionEntry1( DMMCSOCKET_PASSWORDCONTROLSTART_ENTRY, this );
	TInt r = KErrNone;							// error code

	TBool changed = EFalse;						// compress store if changed

	TBuf8<KMMCCIDLength> cid;					// convert to TBuf8<> for comparison
	cid.SetLength(KMMCCIDLength);
	aCID.Copy(&cid[0]);

	TBool s = EFalse;							// source password (current mapping)
	TBool t = aPWD != NULL;						// target pasword (new value for mapping)

	// remove any bindings which were not validated.  This is all non EStValid
	// bindings - the previous operation could have failed before CMD42 was sent,
	// in which case its state would be EStPending, not EStInvalid.

	// an inefficiency exists where an invalid binding for the target CID exists.
	// This could be reused instead of being deallocated and reallocated.  This
	// situation would occur if the user inserted a card whose password was not
	// known to the machine, unlocked it with the wrong password and tried again.
	// The case is rare and the cost is run-time speed, which is not noticeable,
	// The run-time memory usage is equivalent, so it is probably not worth the
	// extra rom bytes and logic.

	for (TInt i = 0; i < iPasswordStore->iStore->Count(); )
		{
		if ((*iPasswordStore->iStore)[i].iState != TMapping::EStValid)
			{
			iPasswordStore->iStore->Remove(i);	// i becomes index for next item
			changed = ETrue;
			}
		else
			{
			if ((*iPasswordStore->iStore)[i].iCID == cid)
				s = ETrue;
			++i;
			}
		}

	if (! t)
		iStack->iMPTgt.Zero();
	else
		{
		iStack->iMPTgt = *aPWD;

		if (!s)
			{
			TMediaPassword mp;					// empty, to indicate !s
			if ((r = iPasswordStore->InsertMapping(aCID, mp, TMapping::EStPending)) != KErrNone)
			    {
				OstTraceFunctionExitExt( DMMCSOCKET_PASSWORDCONTROLSTART_EXIT1, this, r );
				return r;
			    }

			changed = ETrue;
			}
		}

	if (changed)
		iPasswordStore->iStore->Compress();

	OstTraceFunctionExitExt( DMMCSOCKET_PASSWORDCONTROLSTART_EXIT2, this, r );
	return r;
	}



void DMMCSocket::PasswordControlEnd(DMMCSession *aSessP, TInt aResult)
/**
 * called by DMMCStack::SchedCompletionPass() after CMD42 has completed to
 * update internal store.  This function does not run in ks context and so
 * can only invalidate bindings for later removal in PasswordControlStart().
 * 
 * s = source (current) password stored; t = target (new) password should be stored
 * f = failure
 * 
 * If the operation fails, then a recovery policy is used so the user does
 * not lose the good current binding and have to re-enter the password.
 * '
 * f = 0					f = 1
 * 				T						T
 * 			0		1				0		1
 * 	S	0	N		V		S	0	N		I
 * 		1	W		V			1	R		R
 * 
 * 	N	nothing		V	validate	W	wipe
 * 	I	invalidate	R	restore
 * '
 * See PasswordControlStart() for details of how store set up.
 */
	{
	OstTraceFunctionEntryExt( DMMCSOCKET_PASSWORDCONTROLEND_ENTRY, this );
	// autounlock is a special case because the PasswordControlStart() will
	// not have been called (the CID is not known in ks context.)  The mapping
	// for this specific card is removed on failure, because it is the current
	// mapping that is definitely wrong.

	TBuf8<KMMCCIDLength> cid;					// convert to TBuf8<> for comparison
	cid.SetLength(KMMCCIDLength);
	aSessP->CardP()->CID().Copy(&cid[0]);

	if (aSessP == &iStack->iAutoUnlockSession)
		{
		TBool changed = EFalse;						// compress store if changed

		for (TInt j = 0; j < iPasswordStore->iStore->Count(); )
			{
			TMapping &mp = (*iPasswordStore->iStore)[j];
			if (mp.iCID == cid)
				{
				mp.iState = (aResult == KErrNone ? TMapping::EStValid : TMapping::EStInvalid);
				if(mp.iState == TMapping::EStInvalid)
					{
					iPasswordStore->iStore->Remove(j);
					changed = ETrue;
					}
				else
					{
					j++;
					}
				}
			else
				{
				j++;
				}
			}

		if (changed)
			iPasswordStore->iStore->Compress();
		}
	else
		{
		const TMediaPassword &mpTgt = iStack->iMPTgt;
		TBool s = EFalse;						// default value in case no mapping
		TBool t = mpTgt.Length() > 0;
		TBool f = (aResult != KErrNone);

		TMapping mp, *pmp;						// get mapping to mutate
		mp.iCID = cid;
		TInt psn = iPasswordStore->iStore->Find(mp, iPasswordStore->iIdentityRelation);
		if (psn == KErrNotFound)
			{
			OstTraceFunctionExit1( DMMCSOCKET_PASSWORDCONTROLEND_EXIT1, this );
			return;
			}
		else
			{
			pmp = &(*iPasswordStore->iStore)[psn];
			s = pmp->iPWD.Length() > 0;
			}

		if (f)
			{
			if (s)		// s & ~f
				pmp->iState = TMapping::EStValid;		// restore
			else
				{
				if (t)	// ~s & t & f 
					pmp->iState = TMapping::EStInvalid;	// invalidate
				}
			}
		else
			{
			if (t)		// t & ~f
				{
				pmp->iState = TMapping::EStValid;		// validate
				pmp->iPWD = mpTgt;
				}
			else
				{
				if (s)	// s & ~t & ~f
					pmp->iState = TMapping::EStInvalid;	// wipe
				}
			}	// else (f)
		}	// else if (aSessP == &iStack->iAutoUnlockSession)
	OstTraceFunctionExit1( DMMCSOCKET_PASSWORDCONTROLEND_EXIT2, this );
	}


TMMCPasswordStore::TMMCPasswordStore()
/**
 * Contructor
 */
	: iIdentityRelation(TMMCPasswordStore::CompareCID)
	{
	OstTraceFunctionEntry1( TMMCPASSWORDSTORE_TMMCPASSWORDSTORE_ENTRY, this );
	}

TInt TMMCPasswordStore::Init()
/**
 * Initialises the password store and allocates resources.
 * @return KErrNone if successful, standard error code otherwise.
 */
	{
	OstTraceFunctionEntry1( TMMCPASSWORDSTORE_INIT_ENTRY, this );
	// We don't have a destructor yet as this object lasts forever
	iStore = new RArray<TMapping>(4, _FOFF(TMapping, iCID));
	if(!iStore)
	    {
		OstTraceFunctionExitExt( TMMCPASSWORDSTORE_INIT_EXIT1, this, KErrNoMemory );
		return KErrNoMemory;
	    }
	OstTraceFunctionExitExt( TMMCPASSWORDSTORE_INIT_EXIT2, this, KErrNone );
	return KErrNone;
	}

EXPORT_C TBool TMMCPasswordStore::IsMappingIncorrect(const TCID& aCID, const TMediaPassword& aPWD)
/**
 * Returns true if the password is definitely incorrect, i.e. if a valid entry with a
 * different password exists.  Returns false if correct (because the mapping matches,)
 * or if cannot tell (because no valid mapping.)
 */
	{
	OstTraceFunctionEntry1( TMMCPASSWORDSTORE_ISMAPPINGINCORRECT_ENTRY, this );
	TMapping* pmp = FindMappingInStore(aCID);
	TBool ret = pmp != 0 && pmp->iState == TMapping::EStValid && pmp->iPWD.Compare(aPWD) != 0;
	OstTraceFunctionExitExt( TMMCPASSWORDSTORE_ISMAPPINGINCORRECT_EXIT, this, ret );
	return ret;
	}

TMapping *TMMCPasswordStore::FindMappingInStore(const TCID &aCID)
/**
 * return pointer to aCID mapping in store or NULL if not found
 */
	{
	OstTraceFunctionEntry1( TMMCPASSWORDSTORE_FINDMAPPINGINSTORE_ENTRY, this );
	TMapping *pmp = NULL;
	TMapping mp;								// 8 + 16 + 8 + 16 + 4 bytes
	mp.iCID.SetLength(KMMCCIDLength);
	aCID.Copy(&mp.iCID[0]);

	TInt psn=iStore->Find(mp, iIdentityRelation);
	if(psn!=KErrNotFound)
		{
		pmp = &(*iStore)[psn];
		}
	OstTraceFunctionExitExt( TMMCPASSWORDSTORE_FINDMAPPINGINSTORE_EXIT, this, ( TUint )( pmp ) );
	return pmp;
	}

TInt TMMCPasswordStore::InsertMapping(const TCID &aCID, const TMediaPassword &aPWD, TMapping::TState aState)
/**
 * Ensures that a mapping from aCID to aPWD exists in the store.  If an
 * existing entry does not exist to mutate, then insert a new one.  This
 * may cause an allocation, depending on the granularity and count.
 * 
 * If the CID is already bound to something in the store, then this operation
 * is a binary search, otherwise it may involve kernel heap allocation.
 */
	{
	OstTraceFunctionEntry1( TMMCPASSWORDSTORE_INSERTMAPPING_ENTRY, this );
	TInt r = KErrNone;
	TMapping mpN;
	mpN.iCID.SetLength(KMMCCIDLength);
	aCID.Copy(&mpN.iCID[0]);					// copies from aCID into buffer.

	TInt psn = iStore->Find(mpN, iIdentityRelation);
	if(psn == KErrNotFound)
		{
		mpN.iPWD.Copy(aPWD);
		mpN.iState = aState;
		r=iStore->Insert(mpN, iStore->Count());
		}
	else
		{
		TMapping &mpE = (*iStore)[psn];
		mpE.iPWD.Copy(aPWD);
		mpE.iState = aState;
		r = KErrNone;
		}

	OstTraceFunctionExitExt( TMMCPASSWORDSTORE_INSERTMAPPING_EXIT, this, r );
	return r;
	}

TInt TMMCPasswordStore::PasswordStoreLengthInBytes()
/**
 * virtual from DPeriphBusController, kern exec
 * return number of bytes needed for persistent file representation
 * of the password store
 */
	{
	OstTraceFunctionEntry1( TMMCPASSWORDSTORE_PASSWORDSTORELENGTHINBYTES_ENTRY, this );
	TInt sz = 0;

	for (TInt i = 0; i < iStore->Count(); ++i)
		{
		const TMapping &mp = (*iStore)[i];
		if (mp.iState == TMapping::EStValid)
			sz += KMMCCIDLength + sizeof(TInt32) + mp.iPWD.Length();
		}

	OstTraceFunctionExitExt( TMMCPASSWORDSTORE_PASSWORDSTORELENGTHINBYTES_EXIT, this, sz );
	return sz;
	}

TBool TMMCPasswordStore::ReadPasswordData(TDes8 &aBuf)
/**
 * virtual from DPeriphBusController, kern exec
 * fills descriptor with persistent representation of password store
 * data.  aBuf is resized to contain exactly the password data from
 * the store.  If its maximum length is not enough then KErrOverflow
 * is returned and aBuf is not mutated.
 */
	{
	OstTraceFunctionEntry1( TMMCPASSWORDSTORE_READPASSWORDDATA_ENTRY, this );
	TInt r=KErrNone;										// error code

	if (PasswordStoreLengthInBytes() > aBuf.MaxLength())
		r = KErrOverflow;
	else
		{
		aBuf.Zero();
		for (TInt i = 0; i < iStore->Count(); ++i)
			{
			const TMapping &mp = (*iStore)[i];

			if (mp.iState == TMapping::EStValid)
				{
				aBuf.Append(mp.iCID);

				TUint8 lenBuf[sizeof(TInt32)];		// length, big-endian
				TMMC::BigEndian4Bytes(lenBuf, TInt32(mp.iPWD.Length()));
				aBuf.Append(&lenBuf[0], sizeof(TInt32));

				aBuf.Append(mp.iPWD);
				}
			}

		r = KErrNone;
		}

	OstTraceFunctionExitExt( TMMCPASSWORDSTORE_READPASSWORDDATA_EXIT, this, r );
	return r;
	}


TInt TMMCPasswordStore::WritePasswordData(TDesC8 &aBuf)
/**
 * virtual from DPeriphBusController, kern server
 * replace current store with data from persistent representation in aBuf.
 */
	{
	OstTraceFunctionEntry1( TMMCPASSWORDSTORE_WRITEPASSWORDDATA_ENTRY, this );
	// should only be called at boot up, but remove chance of duplicate entries
	iStore->Reset();

	TInt iBIdx;									// buffer index

	// check buffer integrity
	
	TBool corrupt = EFalse;						// abort flag
	for (iBIdx = 0; iBIdx < aBuf.Length(); )
		{
		// enough raw data for CID, PWD_LEN and 1 byte of PWD
		corrupt = TUint(aBuf.Length() - iBIdx) < KMMCCIDLength + sizeof(TUint32) + 1;
		if (corrupt)
			break;
		
		// PWD_LEN is valid and enough raw data left for PWD
		iBIdx += KMMCCIDLength;
		const TInt32 pwd_len(TMMC::BigEndian32(&aBuf[iBIdx]));
		corrupt = !(
				(pwd_len <= TInt32(KMaxMediaPassword))
			&&	aBuf.Length() - iBIdx >= TInt(sizeof(TUint32)) + pwd_len );
		if (corrupt)
			break;
		
		// skip over PWD_LEN and PWD to next entry
		iBIdx += sizeof(TInt32) + pwd_len;
		}

	if (corrupt)
	    {
		OstTraceFunctionExitExt( TMMCPASSWORDSTORE_WRITEPASSWORDDATA_EXIT1, this, KErrCorrupt );
		return KErrCorrupt;
	    }

	// Build the store from the entries in the buffer.
	TInt r = KErrNone;							// error code
	for (iBIdx = 0; r == KErrNone && iBIdx < aBuf.Length(); )
		{
		TPtrC8 pCID(&aBuf[iBIdx], KMMCCIDLength);	// CID
		const TCID cid(pCID.Ptr());

		const TInt32 pwd_len(TMMC::BigEndian32(&aBuf[iBIdx + KMMCCIDLength]));
		TMediaPassword pwd;
		pwd.Copy(&aBuf[iBIdx + KMMCCIDLength + sizeof(TInt32)], pwd_len);

		iBIdx += KMMCCIDLength + sizeof(TInt32) + pwd_len;
		r = InsertMapping(cid, pwd, TMapping::EStValid);
		}

	// it may be acceptable to use a partially created store, providing the
	// sections that do exist are valid.  Alternatively, the operation should
	// atomic from the startup thread's point of view.

	if (r != KErrNone)
		iStore->Reset();

	OstTraceFunctionExitExt( TMMCPASSWORDSTORE_WRITEPASSWORDDATA_EXIT2, this, r );
	return r;
	}

TInt TMMCPasswordStore::CompareCID(const TMapping& aLeft, const TMapping& aRight)
/**
 * CID Comparason Functions for RArray::Find
 */
	{
	OstTraceFunctionEntry0( TMMCPASSWORDSTORE_COMPARECID_ENTRY );
	return(aLeft.iCID == aRight.iCID);
	}

void DMMCSocket::InitiatePowerUpSequence()
/**
 * Initiates a power up sequence on the stack
 */
	{
	OstTraceFunctionEntry1( DMMCSOCKET_INITIATEPOWERUPSEQUENCE_ENTRY, this );
	iStack->PowerUpStack();
	OstTraceFunctionExit1( DMMCSOCKET_INITIATEPOWERUPSEQUENCE_EXIT, this );
	}

TBool DMMCSocket::CardIsPresent()
/**
 * Indicates the presence of a card.
 * @return ETrue if a card is present, EFalse otherwise
 */
	{
	OstTraceFunctionEntry1( DMMCSOCKET_CARDISPRESENT_ENTRY, this );
	TInt r = iStack->HasCardsPresent();
	OstTraceFunctionExitExt( DMMCSOCKET_CARDISPRESENT_EXIT, this, r );
	return r;
	}

void DMMCSocket::AdjustPartialRead(const TMMCard* aCard, TUint32 aStart, TUint32 aEnd, TUint32* aPhysStart, TUint32* aPhysEnd) const
/**
 * Calculates the minimum range that must be read off a card, an optimisation that takes advantage
 * of the partial read feature found on some cards.  It takes the logical range that the media driver
 * wants to read from the card, and increases it to take into account factors such as FIFO width and
 * minimum DMA transfer size.
 * @param aCard A pointer to the MMC Card
 * @param aStart The required start position
 * @param aEnd The required end position
 * @param aPhysStart The adjusted start position
 * @param aPhysEnd The adjusted end position
 */
	{
	OstTraceFunctionEntryExt( DMMCSOCKET_ADJUSTPARTIALREAD_ENTRY, this );
	iStack->AdjustPartialRead(aCard, aStart, aEnd, aPhysStart, aPhysEnd);
	OstTraceFunctionExit1( DMMCSOCKET_ADJUSTPARTIALREAD_EXIT, this );
	}

void DMMCSocket::GetBufferInfo(TUint8** aMDBuf, TInt* aMDBufLen)
/**
 * Returns the details of the buffer allocated by the socket for data transfer operations.  The buffer
 * is allocated and configured at the variant layer to allow , for example, contiguous pages to be
 * allocated for DMA transfers.
 * @param aMDBuf A pointer to the allocated buffer
 * @param aMDBufLen The length of the allocated buffer
 */
	{
	OstTraceFunctionEntryExt( DMMCSOCKET_GETBUFFERINFO_ENTRY, this );
	iStack->GetBufferInfo(aMDBuf, aMDBufLen);
	OstTraceFunctionExit1( DMMCSOCKET_GETBUFFERINFO_EXIT, this );
	}

void DMMCSocket::Reset1()
/**
 * Resets the socket by powering down the stack.
 * If there are operations in progress (inCritical), this call will be deferred
 * until the operation is complete.  In the case of an emergency power down,
 * this will occur immediately.
 */
	{
	OstTraceFunctionEntry1( DMMCSOCKET_RESET1_ENTRY, this );
	if (iState == EPBusCardAbsent)
	    {
	    // Reset is result of card eject!
	    iStack->iStackState |= KMMCStackStateCardRemoved;
	    }
	
	
	iStack->PowerDownStack();
	OstTraceFunctionExit1( DMMCSOCKET_RESET1_EXIT, this );
	}

void DMMCSocket::Reset2()
/**
 * Resets the socket in response to a PSU fault or media change.
 * Called after Reset1, gives the opportunity to free upp allocated resources
 */
	{
	// No need to do anything here, as the only thing to do is power down the
	// stack, which is performed in ::Reset1
	}

TInt DMMCSocket::Init()
/**
 * Allocates resources and initialises the MMC socket and associated stack object.
 * @return KErrNotReady if no stack has been allocated, standard error code otherwise
 */
	{
	OstTraceFunctionEntry1( DMMCSOCKET_INIT_ENTRY, this );
	__KTRACE_OPT(KPBUS1,Kern::Printf(">MMC:Init"));

	// We need to make sure the stack is initialised,
	// as DPBusSocket::Init() will initiate a power up sequence
	if(iStack == NULL)
	    {
		OstTraceFunctionExitExt( DMMCSOCKET_INIT_EXIT1, this, KErrNotReady );
		return KErrNotReady;
	    }

	GetMachineInfo();

	TInt r = iStack->Init();
	if (r!=KErrNone)
	    {
		OstTraceFunctionExitExt( DMMCSOCKET_INIT_EXIT2, this, r );
		return r;
	    }

	r = DPBusSocket::Init();
	if (r!=KErrNone)
	    {
		OstTraceFunctionExitExt( DMMCSOCKET_INIT_EXIT3, this, r );
		return r;
	    }
	
	OstTraceFunctionExitExt( DMMCSOCKET_INIT_EXIT4, this, KErrNone );
	return KErrNone;
	}

void DMMCSocket::GetMachineInfo()
/**
 * Gets the platform specific configuration information.
 * @see TMMCMachineInfo
 */
	{
	OstTraceFunctionEntry1( DMMCSOCKET_GETMACHINEINFO_ENTRY, this );
	// Get machine info from the stack
	iStack->MachineInfo(iMachineInfo);

	__KTRACE_OPT(KPBUS1, Kern::Printf(">GetMI : iTotalSockets %u", iMachineInfo.iTotalSockets));
	__KTRACE_OPT(KPBUS1, Kern::Printf(">GetMI : iTotalMediaChanges %u", iMachineInfo.iTotalMediaChanges));
	__KTRACE_OPT(KPBUS1, Kern::Printf(">GetMI : iTotalPrimarySupplies %u", iMachineInfo.iTotalPrimarySupplies));
	__KTRACE_OPT(KPBUS1, Kern::Printf(">GetMI : iSPIMode %u", iMachineInfo.iSPIMode));
	__KTRACE_OPT(KPBUS1, Kern::Printf(">GetMI : iBaseBusNumber %u", iMachineInfo.iBaseBusNumber));
	OstTraceDefExt5( OST_TRACE_CATEGORY_RND, TRACE_MMCDEBUG, DMMCSOCKET_GETMACHINEINFO, "iTotalSockets=%d; iTotalMediaChanges=%d; iTotalPrimarySupplies=%d; iSPIMode=%d; iBaseBusNumber=%d", iMachineInfo.iTotalSockets, iMachineInfo.iTotalMediaChanges, iMachineInfo.iTotalPrimarySupplies, iMachineInfo.iSPIMode, iMachineInfo.iBaseBusNumber );
	
	
	OstTraceFunctionExit1( DMMCSOCKET_GETMACHINEINFO_EXIT, this );
	}


// MMC specific functions

EXPORT_C void DMMCSocket::Panic(TMMCPanic aPanic)
/**
 * Panic the MMC Controller
 * @param aPanic The panic code
 */
	{
	OstTraceFunctionEntry0( DMMCSOCKET_PANIC_ENTRY );
	_LIT(KPncNm,"PBUS-MMC");
	Kern::PanicCurrentThread(KPncNm,aPanic);
	}

EXPORT_C DMMCPsu::DMMCPsu(TInt aPsuNum, TInt aMediaChangedNum)
/**
 * Constructor for a DMMCPsu object
 * @param aPsuNum The power supply number
 * @param aMediaChangedNum The associated media change number
 */
	: DPBusPsuBase(aPsuNum, aMediaChangedNum)
	{
	OstTraceFunctionEntryExt( DMMCPSU_DMMCPSU_ENTRY, this );
	
	iVoltageSetting=0x00ffc000; // Default voltage range - 2.6V to 3.6V (OCR reg. format).
	OstTraceFunctionExit1( DMMCPSU_DMMCPSU_EXIT, this );
	}

EXPORT_C TInt DMMCPsu::DoCreate()
/**
 * Create a DMMCPsu object.
 * This should be overridden at the variant layer to allow interrupts and
 * other variant-specific parameters to be initialised.  The default 
 * implementation does nothing.
 * @return Standard Symbian OS error code.
 */
	{
	return KErrNone;
	}


void DMMCPsu::SleepCheck(TAny* aPtr)
/**
 * Checks if media can be placed in Sleep state 
 * and therefore if VccQ supply can be turned off.
 * 
 * @Param aPtr reference to DMMCPsu Object to be acted upon.
 */
	{	
	OstTraceFunctionEntry0( DMMCPSU_SLEEPCHECK_ENTRY );
	DMMCPsu& self = *static_cast<DMMCPsu*>(aPtr);
	
	if (
		(self.iNotLockedTimeout&&!self.IsLocked()&&++self.iNotLockedCount>self.iNotLockedTimeout) ||
		(self.iInactivityTimeout&&++self.iInactivityCount>self.iInactivityTimeout)
	   )
		{
		DMMCSocket* socket = static_cast<DMMCSocket*>(self.iSocket);
		socket->iStack->QSleepStack();
		}
	OstTraceFunctionExit0( DMMCPSU_SLEEPCHECK_EXIT );
	}

EXPORT_C DMMCMediaChange::DMMCMediaChange(TInt aMediaChangeNum)
/**
 * Constructor for a DMMCMediaChange object
 * @param aMediaChangeNum The media change number
 */
	: DMediaChangeBase(aMediaChangeNum)
	{
	OstTraceFunctionEntryExt( DMMCMEDIACHANGE_DMMCMEDIACHANGE_ENTRY, this );
	}

EXPORT_C TInt DMMCMediaChange::Create()
/**
 * Create a DMMCMediaChange object.
 * This should be overridden at the variant layer to allow interrupts and
 * other variant-specific parameters to be initialised.  The base class implementation
 * should be called prior to any variant-specific initialisation.
 * @return Standard Symbian OS error code.
 */
	{
	OstTraceFunctionEntry1( DMMCMEDIACHANGE_CREATE_ENTRY, this );
	TInt r = DMediaChangeBase::Create();
	OstTraceFunctionExitExt( DMMCMEDIACHANGE_CREATE_EXIT, this, r );
	return r;
	}

