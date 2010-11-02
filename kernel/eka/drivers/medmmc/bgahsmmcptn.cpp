// Copyright (c) 2010 Nokia Corporation and/or its subsidiary(-ies).
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
// Partition Management for Embedded MMC devices
//
//

#include <emmcptn.h>
#include "mmc.h"
#include "bgahsmmcptn.h"
#include "medmmc.h"
#include "toc.h"

//#define __DEBUG_PARTITIONS_
//#define __DEBUG_CHECK_PARTITION_

class DBB5PartitionInfo : public DEMMCPartitionInfo
	{
public:
	 DBB5PartitionInfo();
	~DBB5PartitionInfo();

public:
	virtual TInt Initialise(DMediaDriver* aDriver);
	virtual TInt PartitionInfo(TPartitionInfo& anInfo, const TMMCCallBack& aCallBack);
	virtual TInt PartitionCaps(TLocDrv& aDrive, TDes8& aInfo);
	TInt HalFunction(TInt aFunction, TAny* a1, TAny* a2);
	
protected:
	void SetPartitionEntry(TPartitionEntry* aEntry, TUint aFirstSector, TUint aNumSectors);

private:
	virtual TInt ReadPartition(TUint32 aPtOffset);
	virtual TInt ReadPartition(TUint32 aPtOffset, TUint aNumBlocks);
	static void SessionEndCallBack(TAny* aSelf);
	void DoSessionEndCallBack();
	TInt DecodeTOCPartitionInfo();
	TInt DecodeBB5PartitionInfo();
	TInt CheckPartitionBoundaries(TInt aStart, TInt aEnd);
	TInt GetPartitionSizeInSectors(TUint aPartition, TUint32& aSize);
	TInt GetPartitionOffset(TUint32& aPtOffset);
	TInt SelectNextPartition();
	TInt ReadTOCVersionInfo(TUint32& aOffset);
	void DecodeVersionInfo();

private:	
	enum TMediaRequest
	    {
        EIdle                    = 0x00,
        EDecodeBB5PartitionInfo  = 0x01,
        EDecodeTOCPartitionInfo  = 0x02,        
        EReadImageVersionInfo    = 0x03,	    
	    };
	
protected:
		
	DMediaDriver*	iDriver;
	TPartitionInfo* iPartitionInfo;
	TMMCCallBack	iSessionEndCallBack;
	TMMCCallBack	iCallBack;		   // Where to report the PartitionInfo completion
	DMMCSession*	iSession;
	TMMCard*		iCard;
	TUint8* 		iIntBuf;
	TUint32 		iPartitionAttributes[KMaxLocalDrives];
	TBool           iCheckTOC;
	Toc*            iTocPtr;
	TUint32			iSelectedPartition;	
	TMediaRequest   iMedReq;
	
// Version Info Stuff	
	TBool            iVersionInfoRead;    
    TUint32          iVersionInfoItems; /** Amount of version info items */
    TVersionInfoItem iVersionInfo[KMaxSectionItems]; /** Array for keep whole Version Info structures */
    TUint32          iTocCount;
	};

/*****************************************************************************/
/*                                                                           */
/* NAME                                                                      */
/*      halFunction                                                          */
/* DESCRIPTION                                                               */
/*      This function gets HAL queries via EHalGroupMedia device 1           */
/*      This function is registered in the end of DoCreate                   */
/* PARAMETERS                                                                */
/*      TAny* aPtr      pointer to DMediaDriverNand object                   */
/*      TInt  aFunction requested function:                                  */
/*            1 - NAND block size                                            */
/*            2 - Is separate erase command supported  ETrue/EFalse          */
/*      TAny* a1        address to user side variable, block size is saved   */
/*                      to this address                                      */
/*      TAny* a2        not used                                             */
/* RETURN VALUES                                                             */
/*      KErrNone                                                             */
/* NOTES                                                                     */
/*                                                                           */
/*****************************************************************************/
LOCAL_C TInt halFunction(TAny* aPtr, TInt aFunction, TAny* a1, TAny* a2)
    {
    DBB5PartitionInfo* pI = (DBB5PartitionInfo*)aPtr;
    return pI->HalFunction(aFunction,a1,a2);
    }

DBB5PartitionInfo::DBB5PartitionInfo()
  : iSessionEndCallBack(DBB5PartitionInfo::SessionEndCallBack, this),
    iCheckTOC(EFalse),iMedReq(EIdle),iVersionInfoItems(0)
	{
	}

DBB5PartitionInfo::~DBB5PartitionInfo()
	{
	delete iSession;
	if (iVersionInfoRead)
	    {
	    TInt r = Kern::RemoveHalEntry(EHalGroupNandMedia,ENandMediaDevice1);
	    __KTRACE_OPT(KPBUSDRV, Kern::Printf("~DBB5PartitionInfo: Removed HAL Entry %d",r));
	    r = r; //get rid of warning, nothing we can do with the warning
	    }
	
	if (iTocPtr)
	    {
	    delete iTocPtr;
	    }
	}

TInt DBB5PartitionInfo::Initialise(DMediaDriver* aDriver)
	{
	iDriver = aDriver;

	DMMCSocket* socket = ((DMMCSocket*)((DPBusPrimaryMedia*)(iDriver->iPrimaryMedia))->iSocket);
	if(socket == NULL)
		return(KErrNoMemory);

	DMMCStack* stack = socket->Stack(0);
	iCard = stack->CardP(((DPBusPrimaryMedia*)(iDriver->iPrimaryMedia))->iSlotNumber);

	iSession = stack->AllocSession(iSessionEndCallBack);
	if (iSession == NULL)
		return(KErrNoMemory);

	iSession->SetStack(stack);
	iSession->SetCard(iCard);

	// this gets used before any access
	TInt bufLen, minorBufLen;
	stack->BufferInfo(iIntBuf, bufLen, minorBufLen);

	iSelectedPartition = TExtendedCSD::ESelectUserArea;
	
	iTocPtr = new Toc();
	
	return(KErrNone);
	}

TInt DBB5PartitionInfo::PartitionInfo(TPartitionInfo& aInfo, const TMMCCallBack& aCallBack)
	{
    __KTRACE_OPT(KPBUSDRV,Kern::Printf(">DBB5PartitionInfo:PartitionInfo"));
	iPartitionInfo = &aInfo;
	iPartitionInfo->iPartitionCount = 0;
	iCallBack = aCallBack;
	iVersionInfoRead = EFalse;

	// Always check the user partition first
	iSelectedPartition = TExtendedCSD::ESelectUserArea;
	iSession->SetPartition(iSelectedPartition);

	// Preferred partition scheme is BB5, which is located in the last block of the media.
    TUint32 ptiOffset = 0;
    TInt r = KErrNone;
    do
    	{
	    r = GetPartitionOffset(ptiOffset);

	    if(r == KErrNone)
	    	{
		    r = ReadPartition(ptiOffset);
		    iMedReq = (KErrNone==r) ? EDecodeBB5PartitionInfo : EIdle;
		    break;
	    	}

	    r = SelectNextPartition();
    	}
    while(r != KErrNotFound);    
    
    __KTRACE_OPT(KPBUSDRV,Kern::Printf("<DBB5PartitionInfo::PartitionInfo(%d)",r));
	return r;
	}
	
// retrieves size in terms of sectors
TInt DBB5PartitionInfo::GetPartitionSizeInSectors(TUint aPartition, TUint32& aSize)
	{
    __KTRACE_OPT(KPBUSDRV,Kern::Printf(">DBB5PartitionInfo: GetPartitionSizeInSectors(%d)",aPartition));
    
	TInt r = KErrNone;
	TUint32 size = 0;
	const TExtendedCSD& extCsd = iCard->ExtendedCSD();
	
	switch(aPartition)
		{
		case TExtendedCSD::ESelectUserArea:
			{
			size = (TUint32)(iCard->DeviceSize64() / KSectorSize);
			
			if(extCsd.ExtendedCSDRev() >= TExtendedCSD::EExtendedCSDRev1_5)
				{
				TUint32 otherPartitionsSize = 
					extCsd.GeneralPurposePartition1SizeInSectors()
					+ extCsd.GeneralPurposePartition2SizeInSectors()
					+ extCsd.GeneralPurposePartition3SizeInSectors()
					+ extCsd.GeneralPurposePartition4SizeInSectors();
					
					__ASSERT_DEBUG(size >= otherPartitionsSize, Kern::Fault("DBB5PartitionInfo size mismatch", __LINE__));
					size -= otherPartitionsSize;
				}
			}
			break;
		case TExtendedCSD::ESelectBootPartition1:
		case TExtendedCSD::ESelectBootPartition2:
			size = (extCsd.ExtendedCSDRev() < TExtendedCSD::EExtendedCSDRev1_3) ? 
				0 : extCsd.BootSizeInSectors();
			break;
		case TExtendedCSD::ESelectRPMB:
			size = (extCsd.ExtendedCSDRev() < TExtendedCSD::EExtendedCSDRev1_5) ? 
				0 : extCsd.RpmbSizeInSectors();
			break;
		case TExtendedCSD::ESelectGPAPartition1:
			size = (extCsd.ExtendedCSDRev() < TExtendedCSD::EExtendedCSDRev1_5) ? 
				0 : extCsd.GeneralPurposePartition1SizeInSectors();
			break;
		case TExtendedCSD::ESelectGPAPartition2:
			size = (extCsd.ExtendedCSDRev() < TExtendedCSD::EExtendedCSDRev1_5) ? 
				0 : extCsd.GeneralPurposePartition2SizeInSectors();
			break;
		case TExtendedCSD::ESelectGPAPartition3:
			size = (extCsd.ExtendedCSDRev() < TExtendedCSD::EExtendedCSDRev1_5) ? 
				0 : extCsd.GeneralPurposePartition3SizeInSectors();
			break;
		case TExtendedCSD::ESelectGPAPartition4:
			size = (extCsd.ExtendedCSDRev() < TExtendedCSD::EExtendedCSDRev1_5) ? 
				0 : extCsd.GeneralPurposePartition4SizeInSectors();
			break;
		default:
			// unknown partition
			size = 0;
			r = KErrNotSupported;
			break;
		}

	__KTRACE_OPT(KPBUSDRV,Kern::Printf("<DBB5PartitionInfo: GetPartitionSizeInSectors(size %d, r %d)",size,r));
	aSize = size;	
	return r;
	}
	
TInt DBB5PartitionInfo::GetPartitionOffset(TUint32& aPtiOffset)
	{
	TInt r = GetPartitionSizeInSectors(iSelectedPartition, aPtiOffset);
		
	if((r != KErrNone) || (aPtiOffset == 0))
		{
		// error reading or partition not supported, skip
		r = KErrNotSupported;
		}
	else
		{			
		// need to determine correct end of the partition
		aPtiOffset -= KPIOffsetFromMediaEnd;
		}
		
	return r;
	}
	
TInt DBB5PartitionInfo::SelectNextPartition()
	{
    __KTRACE_OPT(KPBUSDRV,Kern::Printf(">DBB5PartitionInfo: SelectNextPartition (%d)",iSelectedPartition));
	TExtendedCSD extCsd = iCard->ExtendedCSD();
	TUint maxPartition = TExtendedCSD::ESelectUserArea;
	
	if(extCsd.ExtendedCSDRev() >= TExtendedCSD::EExtendedCSDRev1_5)
		{
		// v4.4 supports UDA, 2x BOOT, RPMB and 4x GPAP partitions
		maxPartition = TExtendedCSD::ESelectGPAPartition4;
		}
#ifdef EMMC_BOOT_PARTITION_ACCESS_ENABLED
	else if(extCsd.ExtendedCSDRev() >= TExtendedCSD::EExtendedCSDRev1_3)
		{
		// v4.3 supports up to two BOOT partitions
		maxPartition = TExtendedCSD::ESelectBootPartition2;
		}
#endif // EMMC_BOOT_PARTITION_ACCESS_ENABLED
	
	++iSelectedPartition;
	
	// skip through to GPAP1 if either the currently selected partition is RPMB or
	// if it is one of the BOOT partitions and boot partition access is not enabled
	if((iSelectedPartition == TExtendedCSD::ESelectRPMB)
#ifndef EMMC_BOOT_PARTITION_ACCESS_ENABLED 
		|| (iSelectedPartition == TExtendedCSD::ESelectBootPartition1)
		|| (iSelectedPartition == TExtendedCSD::ESelectBootPartition2)
#endif	   
		)
		{
		iSelectedPartition = TExtendedCSD::ESelectGPAPartition1;
		}

	TInt r = KErrNone;
	if(iSelectedPartition > maxPartition)
		{
		r = KErrNotFound; // no more partitions to be checked
		}
	else
		{
		iSession->SetPartition(iSelectedPartition);
		}

	__KTRACE_OPT(KPBUSDRV,Kern::Printf("<DBB5PartitionInfo: SelectNextPartition (%d) err (%d)",iSelectedPartition, r));
	        
	return r;	
	}	

TInt DBB5PartitionInfo::ReadPartition(TUint32 aPtOffset)
    {
    return ReadPartition(aPtOffset,1);
    }

// returns KErrCompletion on success after having checked all partitions
TInt DBB5PartitionInfo::ReadPartition(TUint32 aPtOffset, TUint aNumBlocks)
    {
    __ASSERT_ALWAYS(aNumBlocks > 0,Kern::Fault("DBB5PartitionInfo:RP aNumBlocks = 0", __LINE__));
	// If media driver is persistent (see EMediaDriverPersistent)
	// the card may have changed since last power down, so reset CID
	iSession->SetCard(iCard);

	if (aNumBlocks == 1)
	    {
	    iSession->SetupCIMReadBlock(aPtOffset, iIntBuf);
	    }
	else
	    {
	    iSession->SetupCIMReadBlock(aPtOffset, iIntBuf,aNumBlocks);	    
	    }
	
	TInt r = iDriver->InCritical();
	if (r == KErrNone)
		r = iSession->Engage();

	if(r != KErrNone)
		iDriver->EndInCritical();

	return(r);
	}

TInt DBB5PartitionInfo::PartitionCaps(TLocDrv& aDrive, TDes8& aInfo)
	{
	TLocalDriveCapsV6Buf& Info = static_cast< TLocalDriveCapsV6Buf&> (aInfo);

	if (aDrive.iPartitionType == KPartitionTypePagedData)
		{
		Info().iFileSystemId = KDriveFileNone;
		Info().iDriveAtt |= KDriveAttHidden;
		}
	else if (aDrive.iPartitionType == KPartitionTypeRofs)
		{
		Info().iFileSystemId = KDriveFileSysROFS;
		Info().iMediaAtt &= ~KMediaAttFormattable;
		Info().iMediaAtt |= KMediaAttWriteProtected;
		}
	else if ((aDrive.iPartitionType == KPartitionTypeROM) ||
			 (aDrive.iPartitionType == KPartitionTypeEmpty))
		{
		Info().iFileSystemId = KDriveFileNone;
		Info().iMediaAtt &= ~KMediaAttFormattable;
		Info().iMediaAtt |= KMediaAttWriteProtected;
		}
    else if ((aDrive.iPartitionType == KPartitionTypePartitionMagic) || //CPS/PMM
             (aDrive.iPartitionType == KPartitionTypeSymbianCrashLog))
        {
        Info().iFileSystemId = KDriveFileNone;
        Info().iMediaAtt |= KMediaAttFormattable;
        }
	else if ( PartitionIsFAT(aDrive.iPartitionType) || PartitionIsFAT32(aDrive.iPartitionType)	)
		{		
		Info().iDriveAtt |= iPartitionAttributes[aDrive.iPartitionNumber];
		}

	return KErrNone;
	}

void DBB5PartitionInfo::SessionEndCallBack(TAny* aSelf)
	{
	DBB5PartitionInfo& self = *static_cast<DBB5PartitionInfo*>(aSelf);
	self.DoSessionEndCallBack();
	}

void DBB5PartitionInfo::DoSessionEndCallBack()
	{
	iDriver->EndInCritical();

	__KTRACE_OPT(KPBUSDRV,Kern::Printf(">DBB5PartitionInfo:DoSessionEndCallBack (%d)",iMedReq));
	
	TInt r = iSession->EpocErrorCode();
	
	if (KErrNone == r)
	    {
	    switch (iMedReq)
	        {
	        case EDecodeBB5PartitionInfo   : 
	            {
                r = DecodeBB5PartitionInfo();
                if (r == KErrNotFound)
                    {
                    // Check for TOC in this partition instead
                    TUint32 size = 0;
                    r = GetPartitionSizeInSectors(iSelectedPartition,size);
                    if ((KErrNone == r) && (size >= KTocStartSector+1))
                        {
                        //Check partition size to see if TOC could be read (i.e. might be too small)
                        r = ReadPartition(KTocStartSector);
                        iMedReq = (KErrNone==r) ? EDecodeTOCPartitionInfo : EIdle;
                        return;
                        }                    
                    }
                // else - found BB5 check if next phys partition has anything to read...
                break;
	            }
	        case EDecodeTOCPartitionInfo   :
	            {
	            r = DecodeTOCPartitionInfo();
	            if (r == KErrNone)
	                {
	                //Found a TOC, read version info if available
	                TUint32 versOffset = 0;
	                iTocCount = 0; //Reset Toc entry counter
	                r = ReadTOCVersionInfo(versOffset);	                
	                if (r == KErrNotFound)
	                    {
	                    // No valid partitions present
	                    iMedReq = EIdle;
                        r = KErrNone;
	                    }
	                else
	                    {
	                    // Found something to read...
	                    r = ReadPartition(versOffset, 2);
	                    iMedReq = (KErrNone==r) ? EReadImageVersionInfo : EIdle;
	                    return;
	                    }
	                }
	            else
	                {
	                // TOC not found move on to next phys partition
	                iMedReq = EIdle;
	                r = KErrNone;
	                }
	            break;
	            }
	        case EReadImageVersionInfo     :
	            {	            
                DecodeVersionInfo();
                TUint32 versOffset = 0;
                iTocCount++;
                r = ReadTOCVersionInfo(versOffset);
                if (r == KErrNotFound)
                    {
                    //Finished parsing the current TOC                                 
                    //Mark version info as being read in order to report when finished later
                    iVersionInfoRead = ETrue;
                    iMedReq = EIdle;
                    r=KErrNone;
                    }
                else if (r==KErrNone)
                    {
                    // more partitions to read
                    r = ReadPartition(versOffset, 2);
                    iMedReq = (KErrNone==r) ? EReadImageVersionInfo : EIdle;
                    return;
                    }
                break;
	            }
	            
	        default : Kern::Fault("DBB5PartitionInfo unknown request type", __LINE__);
	        }
	    }
	
	// Next physical partition will be search for if
	// BB5 entry was found, no TOC entry was found or no Version Information was found
	if(r == KErrNone)
		{
		// check next partition(s) for BB5
		TUint32 ptiOffset = 0;
	
		r = SelectNextPartition();
		while(r != KErrNotFound)
			{
			if(r == KErrNone)
				r = GetPartitionOffset(ptiOffset);
				
			if(r == KErrNone)
				{
				r = ReadPartition(ptiOffset);
				iMedReq = (KErrNone==r) ? EDecodeBB5PartitionInfo : EIdle;
				if(r != KErrNone)
					break;

				return;
				}

			r = SelectNextPartition();
			}

		
		// end of partitions - reinterpret error code
		if(r != KErrNotFound)
			{
			__KTRACE_OPT(KPBUSDRV, Kern::Printf("Mmc:dsc: ReadPartition() failed r=%d!", r));
			r = KErrCorrupt;
			}
		else if(iPartitionInfo->iPartitionCount == 0)
			{
			__KTRACE_OPT(KPBUSDRV, Kern::Printf("Mmc:dsc: No supported partitions found!"));
			r = KErrCorrupt;
			}
		else
			r = KErrCompletion;
		}
	
	
	if (iVersionInfoRead)
	    {
	    __KTRACE_OPT(KPBUSDRV, Kern::Printf("%d Version Headers - Register HAL entry", iVersionInfoItems ));
	    //finished parsing the TOC register HAL function            
	    r = Kern::AddHalEntry(EHalGroupNandMedia, halFunction, this, ENandMediaDevice1);
	    if (r==KErrNone)
	        {	        
	        __KTRACE_OPT(KPBUSDRV,Kern::Printf("DBB5PartitionInfo:Added HAL Entry %d",r));
	        r = KErrCompletion; //re-mark as completed
	        }
	    }

	// Notify medmmc that partitioninfo is complete
	iCallBack.CallBack();

	// All potential partitions checked - KErrCompletion
	// indicates that there are no more partitions to check
	r = (r == KErrCompletion) ? KErrNone : KErrNotReady;
	__KTRACE_OPT(KPBUSDRV,Kern::Printf("<DBB5PartitionInfo:PartitionInfo Complete %d",r));
	iDriver->PartitionInfoComplete(r);
	}

TInt DBB5PartitionInfo::CheckPartitionBoundaries(TInt aStart, TInt aEnd)
    {
    __ASSERT_ALWAYS(aStart >= 0,Kern::Fault("DBB5PartitionInfo:CPB aStart <0", __LINE__));
    __ASSERT_ALWAYS(aEnd > 0,Kern::Fault("DBB5PartitionInfo:CPB aEnd <=0", __LINE__));
    
    // Validate partition address boundaries
    TUint32 eMmcPartitionSizeInSectors = 0;
    TInt r = KErrNone;
    
    if (aStart != aEnd)
        {
        // At least one entry for a supported partition found
        r = GetPartitionSizeInSectors(iSelectedPartition, eMmcPartitionSizeInSectors);
        
        if(r != KErrNone)
            {
            __KTRACE_OPT(KPBUSDRV, Kern::Printf("DBB5PartitionInfo:CPB: Could not retrieve size for eMMC partition 0x%02X", iSelectedPartition));
            r = KErrCorrupt;
            }
        
    
        // Check that the eMmcPartition address space boundary is not exceeded by the last partition
        if(r == KErrNone)
            {
            TUint64 eMmcPartitionSize = eMmcPartitionSizeInSectors * KSectorSize;
            
            TPartitionEntry& part = iPartitionInfo->iEntry[aEnd - 1];
                
            if(((TUint64)(part.iPartitionBaseAddr + part.iPartitionLen)) > eMmcPartitionSize)
                {
                __KTRACE_OPT(KPBUSDRV, Kern::Printf("DBB5PartitionInfo:CPB: Partition #%d exceeds eMmc address space", aEnd));
                r = KErrCorrupt;
                }
            }
            
        if(r == KErrNone)
            {
            // Go through all BB5 partition entries on this eMMC partition and check boundaries
            for(TInt i = aEnd - 1; i > aStart; i--)
                {
                const TPartitionEntry& curr = iPartitionInfo->iEntry[i];
                TPartitionEntry& prev = iPartitionInfo->iEntry[i-1];
    
                // Check if partitions overlap
                if(curr.iPartitionBaseAddr < (prev.iPartitionBaseAddr + prev.iPartitionLen))
                    {
                    __KTRACE_OPT(KPBUSDRV, Kern::Printf("DBB5PartitionInfo:CPB: Overlapping partitions - check #%d", i));
                    r = KErrCorrupt;
                    }
                }
            }
        }
    
    return r;
    }

TInt DBB5PartitionInfo::DecodeBB5PartitionInfo()
//
// Decode partition info that was read into internal buffer
//
    {
    __KTRACE_OPT(KPBUSDRV, Kern::Printf(">DBB5PartitionInfo: DecodeBB5PartitionInfo(%d)",iPartitionInfo->iPartitionCount));
    TInt partitionCount = iPartitionInfo->iPartitionCount;
    TInt r = KErrNotFound;
        
#ifdef __PRINT_RAW_ENTRIES
    Kern::Printf("BB5 Entry");
    for (TUint i = 0; i < 512; i+=8)
        {
        Kern::Printf("%02x %02x %02x %02x   %02x %02x %02x %02x", iIntBuf[i],iIntBuf[i+1],iIntBuf[i+2],iIntBuf[i+3],iIntBuf[i+4],iIntBuf[i+5],iIntBuf[i+6],iIntBuf[i+7]);
        }
#endif    
    
    // Try utilising the BB5 partitioning scheme    
    BGAHSMMCPTN_PI_STR *partitionTable = (BGAHSMMCPTN_PI_STR*)(&iIntBuf[0]);

    // Verify that this is the BB5 partition table
    if( memcompare( (TUint8*)&(partitionTable->iId[0]), sizeof(BGAHSMMCPTN_PI_ID), (TUint8*)BGAHSMMCPTN_PI_ID, sizeof(BGAHSMMCPTN_PI_ID)) == 0 )
        {
        __KTRACE_OPT(KPBUSDRV, Kern::Printf("Nokia partition structure found"));
        __KTRACE_OPT(KPBUSDRV, Kern::Printf("partitionTable->id..............: %s", partitionTable->iId ));
        __KTRACE_OPT(KPBUSDRV, Kern::Printf("partitionTable->sector_size.....: %d = 0x%x", partitionTable->iSector_size, partitionTable->iSector_size));
        __KTRACE_OPT(KPBUSDRV, Kern::Printf("partitionTable->major_ver.......: %d", partitionTable->iMajor_ver));
        __KTRACE_OPT(KPBUSDRV, Kern::Printf("partitionTable->minor_ver.......: %d", partitionTable->iMinor_ver));
        __KTRACE_OPT(KPBUSDRV, Kern::Printf("partitionTable->partition_amount: %d", partitionTable->iPartition_amount));
        
        TUint8 partitionType = 0;       
        // Check Supported Version is present
        if (partitionTable->iMajor_ver <= BGAHSMMCPTN_PI_VER_MAJOR)
            {
            for( TUint8 index = 0; (index < partitionTable->iPartition_amount) && (index < BGAHSMMCPTN_LAST_DRIVE); index++ )
                {
                if (partitionTable->iMinor_ver >= BGAHSMMCPTN_PART_TYPE_SUPP_VER_MINOR)
                    partitionType = partitionTable->iPartitions[index].iPartition_type;
                else                    
                    partitionType = partitionTable->iPartitions[index].iPartition_id;
            
                // FAT/PMM/CPS/SWAP/CORE/ROFS/CRASH
                if( (partitionTable->iPartitions[index].iSize > 0) &&
                    ( PartitionIsFAT(partitionType) ||
                      PartitionIsFAT32(partitionType) ||
                     (KPartitionTypeSymbianCrashLog == partitionType) ||
                     (KPartitionTypePartitionMagic == partitionType) || //CPS/PMM
                     (KPartitionTypeRofs == partitionType) || 
                     (KPartitionTypeEmpty == partitionType) ||
                     (KPartitionTypeROM == partitionType) ||
                     (KPartitionTypePagedData == partitionType) ) )
                    {                   
                    iPartitionInfo->iEntry[partitionCount].iPartitionType     = partitionType;
                    iPartitionAttributes[partitionCount]                      = partitionTable->iPartitions[index].iPartition_attributes;
                    static_cast<DMmcMediaDriverFlash *>(iDriver)->SetEMmcPartitionMapping(partitionCount, iSelectedPartition);
                    // ROM/ROFS partitions have a BB5 checksum header that must be offset for the Symbian OS.
                    const TUint32 KStartOffset = ((KPartitionTypeROM == partitionType) || (KPartitionTypeRofs == partitionType) || (KPartitionTypeEmpty == partitionType)) ? KBB5HeaderSizeInSectors : 0;
                    
                    iPartitionInfo->iEntry[partitionCount].iPartitionBaseAddr = ((Int64) partitionTable->iPartitions[index].iStart_sector + KStartOffset) << KDiskSectorShift;
                    iPartitionInfo->iEntry[partitionCount].iPartitionLen      = ((Int64) partitionTable->iPartitions[index].iSize - KStartOffset) << KDiskSectorShift;
    
                    __KTRACE_OPT(KPBUSDRV, Kern::Printf("Registering partition #%d:", partitionCount));
                    __KTRACE_OPT(KPBUSDRV, Kern::Printf("partitionCount....: %d", partitionCount));
                    __KTRACE_OPT(KPBUSDRV, Kern::Printf("startSector.......: 0x%x", partitionTable->iPartitions[index].iStart_sector ));
                    __KTRACE_OPT(KPBUSDRV, Kern::Printf("iPartitionBaseAddr: 0x%lx (sectors: %d)", iPartitionInfo->iEntry[partitionCount].iPartitionBaseAddr, (TUint32)(iPartitionInfo->iEntry[partitionCount].iPartitionBaseAddr >> KDiskSectorShift)));
                    __KTRACE_OPT(KPBUSDRV, Kern::Printf("size..............: 0x%lx", partitionTable->iPartitions[index].iSize ));
                    __KTRACE_OPT(KPBUSDRV, Kern::Printf("iPartitionLen.....: 0x%lx (sectors: %d)", iPartitionInfo->iEntry[partitionCount].iPartitionLen, iPartitionInfo->iEntry[partitionCount].iPartitionLen >> KDiskSectorShift));
                    __KTRACE_OPT(KPBUSDRV, Kern::Printf("iPartitionType....: %d", iPartitionInfo->iEntry[partitionCount].iPartitionType));
                    __KTRACE_OPT(KPBUSDRV, Kern::Printf("iPartitionAttribs.: 0x%x", iPartitionAttributes[partitionCount]));
                    __KTRACE_OPT(KPBUSDRV, Kern::Printf("iPartitionMapping.: 0x%x", static_cast<DMmcMediaDriverFlash *>(iDriver)->GetEMmcPartitionMapping(partitionCount)));
                    __KTRACE_OPT(KPBUSDRV, Kern::Printf(" "));
    
                    partitionCount++;
                    r = KErrNone;
                    }
                }
            }
        
        if(partitionCount > iPartitionInfo->iPartitionCount)
            {
            __KTRACE_OPT(KPBUSDRV, Kern::Printf("DBB5PartitionInfo: New BB5 partitions found"));
#ifdef __DEBUG_CHECK_PARTITION_         
            r = CheckPartitionBoundaries(iPartitionInfo->iPartitionCount,partitionCount);        
            if (r == KErrNone)
#endif // __DEBUG_CHECK_PARTITION_             
                {
                //Update master partition count
                iPartitionInfo->iPartitionCount = partitionCount;
                iPartitionInfo->iMediaSizeInBytes = iCard->DeviceSize64();
                }                
            }
        }
    
    __KTRACE_OPT(KPBUSDRV, Kern::Printf("< DBB5PartitionInfo: DecodeBB5PartitionInfo(%d)",r));
    return r;
    }


TInt DBB5PartitionInfo::DecodeTOCPartitionInfo()
//
// Decode partition info that was read into internal buffer
//
    {
    __KTRACE_OPT(KPBUSDRV, Kern::Printf(">DBB5PartitionInfo: DecodeTOCPartitionInfo(%d)",iPartitionInfo->iPartitionCount));
    TInt partitionCount = iPartitionInfo->iPartitionCount;
    TInt r = KErrNone;

    // Try utilising the TOC (Table Of Contents) partitioning scheme 
    const TText8* KRofsNames[KNoOfROFSPartitions] = { KTocRofs1Generic,
                                                      KTocRofs2Generic,
                                                      KTocRofs3Generic,
                                                      KTocRofs4Generic,
                                                      KTocRofs5Generic,                                                            
                                                      KTocRofs6Generic,                                                            
                                                      };
                                    
    STocItem item;    
    memcpy( iTocPtr, &iIntBuf[0], sizeof(Toc));

#ifdef __PRINT_RAW_ENTRIES
    Kern::Printf("TOC Entry");
    for (TUint i = 0; i < 512; i+=8)
        {
        Kern::Printf("%02x %02x %02x %02x   %02x %02x %02x %02x", iIntBuf[i],iIntBuf[i+1],iIntBuf[i+2],iIntBuf[i+3],iIntBuf[i+4],iIntBuf[i+5],iIntBuf[i+6],iIntBuf[i+7]);
        }
#endif   
        
    iTocPtr->iTocStartSector = KTocStartSector;

// USER Drive - Only 1        
    r = iTocPtr->GetItemByName(KTocUserName, item); 
    if (KErrNone == r)
        {
        __KTRACE_OPT(KPBUSDRV, Kern::Printf("[MD  :   ] (%11s) in TOC found : Start addr = 0x%X  Size = 0x%X", item.iFileName, item.iStart, item.iSize));
        iPartitionInfo->iEntry[partitionCount].iPartitionType     = KPartitionTypeFAT16;           
        iPartitionInfo->iEntry[partitionCount].iPartitionBaseAddr = (Int64)item.iStart;                         
        iPartitionInfo->iEntry[partitionCount].iPartitionLen      = (Int64)item.iSize;
        iPartitionAttributes[partitionCount] = 0; // No Additional Attributes required.           
        partitionCount++;
        }   
    
// ROM Drive        
    r = iTocPtr->GetItemByName(KTocRomGeneric, item); 
    if (KErrNone == r)
        {
        __KTRACE_OPT(KPBUSDRV, Kern::Printf("[MD  :   ] (%11s) in TOC found : Start addr = 0x%x  Size = 0x%x", item.iFileName, item.iStart, item.iSize));
        iPartitionInfo->iEntry[partitionCount].iPartitionType     = KPartitionTypeROM;           
        iPartitionInfo->iEntry[partitionCount].iPartitionBaseAddr = (Int64) item.iStart + (KBB5HeaderSizeInSectors << KDiskSectorShift);                         
        iPartitionInfo->iEntry[partitionCount].iPartitionLen      = (Int64) item.iSize - (KBB5HeaderSizeInSectors << KDiskSectorShift);          
        partitionCount++;
        }
    
// ROFS            
    for (TUint i = 0; i < KNoOfROFSPartitions; i++)
        {
        /* Search ROFSn item */            
        r = iTocPtr->GetItemByName(KRofsNames[i], item);
        if (r == KErrNone)
            {
            __KTRACE_OPT(KPBUSDRV, Kern::Printf("[MD  :   ] (%11s) in TOC found : Start addr = 0x%X  Size = 0x%X", item.iFileName, item.iStart, item.iSize));
            iPartitionInfo->iEntry[partitionCount].iPartitionType     = KPartitionTypeRofs;           
            iPartitionInfo->iEntry[partitionCount].iPartitionBaseAddr = (Int64) item.iStart + (KBB5HeaderSizeInSectors << KDiskSectorShift);                         
            iPartitionInfo->iEntry[partitionCount].iPartitionLen      = (Int64) item.iSize - (KBB5HeaderSizeInSectors << KDiskSectorShift);
            partitionCount++;
            }
        }         

// CPS Drive - Only 1        
    r = iTocPtr->GetItemByName(KTocCps, item); 
    if (KErrNone == r)
        {
        __KTRACE_OPT(KPBUSDRV, Kern::Printf("[MD  :   ] (%11s) in TOC found : Start addr = 0x%X  Size = 0x%X", item.iFileName, item.iStart, item.iSize));
        iPartitionInfo->iEntry[partitionCount].iPartitionType     = KPartitionTypePartitionMagic;           
        iPartitionInfo->iEntry[partitionCount].iPartitionBaseAddr = (Int64) item.iStart;                         
        iPartitionInfo->iEntry[partitionCount].iPartitionLen      = (Int64) item.iSize;
        partitionCount++;
        }
    
// CRASH Drive - Only 1        
    r = iTocPtr->GetItemByName(KTocCrashLog, item); 
    if (KErrNone == r)
        {
        __KTRACE_OPT(KPBUSDRV, Kern::Printf("[MD  :   ] (%11s) in TOC found : Start addr = 0x%X  Size = 0x%X", item.iFileName, item.iStart, item.iSize));
        iPartitionInfo->iEntry[partitionCount].iPartitionType     = KPartitionTypeSymbianCrashLog;           
        iPartitionInfo->iEntry[partitionCount].iPartitionBaseAddr = (Int64) item.iStart;                         
        iPartitionInfo->iEntry[partitionCount].iPartitionLen      = (Int64) item.iSize;
        partitionCount++;
        }
    
// SWAP Partition - Only 1        
    r = iTocPtr->GetItemByName(KTocSwap, item);
    if (KErrNone == r)
        {
        __KTRACE_OPT(KPBUSDRV, Kern::Printf("[MD  :   ] (%11s) in TOC found : Start addr = 0x%X  Size = 0x%X", item.iFileName, item.iStart, item.iSize));
        iPartitionInfo->iEntry[partitionCount].iPartitionType     = KPartitionTypePagedData;           
        iPartitionInfo->iEntry[partitionCount].iPartitionBaseAddr = (Int64) item.iStart;                         
        iPartitionInfo->iEntry[partitionCount].iPartitionLen      = (Int64) item.iSize;
        partitionCount++;
        }                

#ifdef __DEBUG_PARTITIONS_
    for (TInt i = 0; i<partitionCount; i++)
        {
        Kern::Printf("iPartitionType....: %d", iPartitionInfo->iEntry[i].iPartitionType);                
        Kern::Printf("iPartitionBaseAddr: 0x%lx (sectors: %d)", iPartitionInfo->iEntry[i].iPartitionBaseAddr, (TUint32)(iPartitionInfo->iEntry[i].iPartitionBaseAddr >> KDiskSectorShift));                
        Kern::Printf("iPartitionLen.....: 0x%lx (sectors: %d)", iPartitionInfo->iEntry[i].iPartitionLen, iPartitionInfo->iEntry[i].iPartitionLen >> KDiskSectorShift);
        Kern::Printf("iPartitionAttribs.: 0x%x", iPartitionAttributes[i]);
        Kern::Printf(" ");
        }
#endif //__DEBUG_PARTITIONS_
    
    if(partitionCount > iPartitionInfo->iPartitionCount)
        {
        __KTRACE_OPT(KPBUSDRV, Kern::Printf("Mmc: New supported partitions found!"));
#ifdef __DEBUG_CHECK_PARTITION_         
        r = CheckPartitionBoundaries(iPartitionInfo->iPartitionCount,partitionCount);        
        if (r == KErrNone)
#endif // __DEBUG_CHECK_PARTITION_             
            {
            //Update master partition count
            iPartitionInfo->iPartitionCount = partitionCount;
            iPartitionInfo->iMediaSizeInBytes = iCard->DeviceSize64();
            r = KErrNone;
            }                
        }
    else
        {
        __KTRACE_OPT(KPBUSDRV, Kern::Printf("<DBB5PartitionInfo: No New Partitions found - TOC"));
        r = KErrNotFound;
        }
    
    __KTRACE_OPT(KPBUSDRV, Kern::Printf("<DBB5PartitionInfo: DecodeTOCPartitionInfo(%d)",r));
    return r;
    }


// Version Info Support
TInt DBB5PartitionInfo::HalFunction(TInt aFunction, TAny* a1, TAny* /*a2*/)
    {
    TInt ret = KErrGeneral;  

    __KTRACE_OPT(KPBUSDRV, Kern::Printf(">DBB5PartitionInfo::HalFunction"));
    
    if (aFunction == EGetVersionInfoItems)
        {
        __KTRACE_OPT(KPBUSDRV, Kern::Printf(">DBB5PartitionInfo::EGetVersionInfoItems %d Version Headers",iVersionInfoItems ));
        kumemput32(a1, &iVersionInfoItems, sizeof(TUint32));
        ret = KErrNone;
        }
    else if (aFunction == EGetVersionInfo)
        {
        __KTRACE_OPT(KPBUSDRV, Kern::Printf(">DBB5PartitionInfo::EGetVersionInfo"));
        kumemput32(a1, &iVersionInfo[0], sizeof(TVersionInfoItem)*iVersionInfoItems);
        ret = KErrNone;
        }

    __KTRACE_OPT(KPBUSDRV, Kern::Printf("<DBB5PartitionInfo::HalFunction"));
    
    return ret;
    }

// returns KErrNotfound if reached the end of the toc
TInt DBB5PartitionInfo::ReadTOCVersionInfo(TUint32& aOffset)
    {
    TInt r = KErrNotFound;
    for (; iTocCount < KMaxNbrOfTocItems; iTocCount++ )
        {
        __KTRACE_OPT(KPBUSDRV, Kern::Printf("ReadTOCVersionInfo: Find next usable TOC entry (%d)",iTocCount));
        // Iterate through the partition info table looking for valid partition headers
        if ( iTocPtr->iTOC[iTocCount].iStart == KEndOfToc ) 
            {
            __KTRACE_OPT(KPBUSDRV, Kern::Printf("ReadTOCVersionInfo: End of the TOC"));
            break;
            }    
        else if ( iTocPtr->iTOC[iTocCount].iStart & 0x80000000 )             
            {
            __KTRACE_OPT(KPBUSDRV, Kern::Printf("ReadTOCVersionInfo: Image has negative offset"));
            continue;
            }    
        else if ( iTocPtr->iTOC[iTocCount].iSize <= 1024 ) 
            {
            __KTRACE_OPT(KPBUSDRV, Kern::Printf("ReadTOCVersionInfo: entry is less than 2 sectors in size"));
            continue;
            }
        
        r = KErrNone;
        break;
        } 
    
    if (r == KErrNone)
        {
        aOffset = ((iTocPtr->iTOC[iTocCount].iStart) >> KSectorShift) + (iTocPtr->iTocStartSector);
        }
    
    return r;
    }

void DBB5PartitionInfo::DecodeVersionInfo()
    {    
    __KTRACE_OPT(KPBUSDRV,Kern::Printf(">Decode Version Info (%d)",iVersionInfoItems));
    
#ifdef __PRINT_RAW_ENTRIES
    Kern::Printf("Version Info:");
    for (TUint i = 0; i < 1024; i+=8)
        {
        Kern::Printf("%02x %02x %02x %02x   %02x %02x %02x %02x", iIntBuf[i],iIntBuf[i+1],iIntBuf[i+2],iIntBuf[i+3],iIntBuf[i+4],iIntBuf[i+5],iIntBuf[i+6],iIntBuf[i+7]);
        }
#endif
    
    // check BB5 common header and image header
    if ( *(TUint32*)&iIntBuf[0]                  == KBB5_CommonHeaderMagic || 
         *(TUint32*)&iIntBuf[KImageHeaderOffset] == KImageHeaderMagic )       
        {
        // pick up required info from TOC
        TVersionInfoItem *vinfo = (TVersionInfoItem *)&iVersionInfo[iVersionInfoItems];
        vinfo->iSectionMaxSize = iTocPtr->iTOC[iTocCount].iSize;
        memcpy( &vinfo->iSectionName[0], &iTocPtr->iTOC[iTocCount].iFileName[0], KMaxSectionNameLen);
        
        // pick up required info from image header
        TImageHeader *himage = (TImageHeader *)&iIntBuf[KImageHeaderOffset];
        vinfo->iImageCompressedSize = himage->iImageCompressedSize;
        vinfo->iImageSize = himage->iImageSize;
        memcpy( &vinfo->iVersion[0], &himage->iVersion[0], KMaxVersionInfoLen);
        
        
        __KTRACE_OPT(KPBUSDRV,Kern::Printf("Section Name (%11s)",vinfo->iSectionName));
        __KTRACE_OPT(KPBUSDRV,Kern::Printf("Section size (%d)",vinfo->iSectionMaxSize));
        __KTRACE_OPT(KPBUSDRV,Kern::Printf("Compressed Size %d", vinfo->iImageCompressedSize));
        __KTRACE_OPT(KPBUSDRV,Kern::Printf("Image Size %d", vinfo->iImageSize));
        __KTRACE_OPT(KPBUSDRV,Kern::Printf("VersionInfo (%11s)",vinfo->iVersion));
        
        iVersionInfoItems++;
        }
    
    __KTRACE_OPT(KPBUSDRV,Kern::Printf("<Decode Version Info (%d)",iVersionInfoItems));
    return;
    }


// End - DBB5PartitionInfo


EXPORT_C DEMMCPartitionInfo* CreateEmmcPartitionInfo()
	{
	return new DBB5PartitionInfo;
	}

DECLARE_STANDARD_EXTENSION()
	{
	return KErrNone;
	}

//	End of File

