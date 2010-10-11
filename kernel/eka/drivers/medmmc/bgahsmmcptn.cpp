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

protected:
	void SetPartitionEntry(TPartitionEntry* aEntry, TUint aFirstSector, TUint aNumSectors);

private:
	virtual TInt ReadPartition(TUint32 aPtOffset);
	static void SessionEndCallBack(TAny* aSelf);
	void DoSessionEndCallBack();
	virtual TInt DecodePartitionInfo();
	TInt GetPartitionSizeInSectors(TUint aPartition, TUint32& aSize);
	TInt GetPartitionOffset(TUint32& aPtOffset);
	TInt SelectNextPartition();

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
	};

DBB5PartitionInfo::DBB5PartitionInfo()
  : iSessionEndCallBack(DBB5PartitionInfo::SessionEndCallBack, this),
    iCheckTOC(EFalse)
	{
	}

DBB5PartitionInfo::~DBB5PartitionInfo()
	{
	delete iSession;
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
	
	return(KErrNone);
	}

TInt DBB5PartitionInfo::PartitionInfo(TPartitionInfo& aInfo, const TMMCCallBack& aCallBack)
	{
	iPartitionInfo = &aInfo;
	iPartitionInfo->iPartitionCount = 0;
	iCallBack = aCallBack;

	// Always check the user partition first
	iSelectedPartition = TExtendedCSD::ESelectUserArea;
	iSession->SetPartition(iSelectedPartition);

	// Preferred partition scheme is BB5, which is located in the last block of the media.
    TUint32 ptiOffset;
    TInt r;
    do
    	{
	    r = GetPartitionOffset(ptiOffset);

	    if(r == KErrNone)
	    	{
		    r = ReadPartition(ptiOffset);
		    return r;
	    	}

	    r = SelectNextPartition();
    	}
    while(r != KErrNotFound);

	return r;
	}
	
// retrieves size in terms of sectors
TInt DBB5PartitionInfo::GetPartitionSizeInSectors(TUint aPartition, TUint32& aSize)
	{
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
		
	return r;	
	}	
	
// returns KErrCompletion on success after having checked all partitions
TInt DBB5PartitionInfo::ReadPartition(TUint32 aPtOffset)
    {
	// If media driver is persistent (see EMediaDriverPersistent)
	// the card may have changed since last power down, so reset CID
	iSession->SetCard(iCard);

	iSession->SetupCIMReadBlock(aPtOffset, iIntBuf);
	
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

	TInt r = iSession->EpocErrorCode();

	
	TInt& partitionCount = iPartitionInfo->iPartitionCount;

	if (r == KErrNone)
		r = DecodePartitionInfo();

	if (iCheckTOC)
		{
		//BB5 table not found need to check for TOC in this partition before continuing
		if (r!=KErrNone)
			{
			iDriver->PartitionInfoComplete(KErrNotReady);
			}
		return;
		}


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
		else if(partitionCount == 0)
			{
			__KTRACE_OPT(KPBUSDRV, Kern::Printf("Mmc:dsc: No supported partitions found!"));
			r = KErrCorrupt;
			}
		else
			r = KErrCompletion;
		}

	// Notify medmmc that partitioninfo is complete
	iCallBack.CallBack();

	// All potential partitions checked - KErrCompletion
	// indicates that there are no more partitions to check
	r = (r == KErrCompletion) ? KErrNone : KErrNotReady;
	iDriver->PartitionInfoComplete(r);
	}


TInt DBB5PartitionInfo::DecodePartitionInfo()
//
// Decode partition info that was read into internal buffer
//
	{
	__KTRACE_OPT(KPBUSDRV, Kern::Printf(">Mmc:PartitionInfo()"));
	TInt& partitionCount = iPartitionInfo->iPartitionCount;
    TInt r = KErrNone;

	if (iCheckTOC)
	    {
        // Try utilising the TOC (Table Of Contents) partitioning scheme 
        const TText8* KRofsNames[KNoOfROFSPartitions] = { KTocRofs1Generic,
                                                          KTocRofs2Generic,
                                                          KTocRofs3Generic,
                                                          KTocRofs4Generic,
                                                          KTocRofs5Generic,                                                            
                                                          KTocRofs6Generic,                                                            
                                                          };
                                        
        STocItem item;
        iTocPtr = reinterpret_cast<Toc*>(&iIntBuf[0]);
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
        
		r = KErrNone;
        iCheckTOC = EFalse;
	    }
	else
	    {
        // Try utilising the BB5 partitioning scheme	
        BGAHSMMCPTN_PI_STR *partitionTable = (BGAHSMMCPTN_PI_STR*)(&iIntBuf[0]);
    
        // Verify that this is the Nokia partition table
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
                        iPartitionInfo->iEntry[partitionCount].iPartitionType	  = partitionType;
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
                        }
                    }
                } 
            }
        else
            {
            __KTRACE_OPT(KPBUSDRV, Kern::Printf("BGAHSMMC signature not found - try TOC layout"));
            iCheckTOC = ETrue;
            
            TInt r = ReadPartition(KTocStartSector);
            return r;
            }
	    }
	
	
	// Validate partition address boundaries
	if(partitionCount == 0)
		{
		__KTRACE_OPT(KPBUSDRV, Kern::Printf("Mmc: No supported partitions found!"));
		// No Supported partitions found on this physical partition
		return KErrNone;
		}
		
#ifdef __DEBUG_CHECK_PARTITION_			
	// Validate partition address boundaries
	TUint32 eMmcPartitionSizeInSectors = 0;
	if(r == KErrNone)
		{
		// At least one entry for a supported partition found
		r = GetPartitionSizeInSectors(iSelectedPartition, eMmcPartitionSizeInSectors);
		
		if(r != KErrNone)
			{
			__KTRACE_OPT(KPBUSDRV, Kern::Printf("Mmc: Could not retrieve size for eMMC partition 0x%02X", iSelectedPartition));
			r = KErrCorrupt;
			}
		}
		
	if(r == KErrNone)
		{
		TUint64 eMmcPartitionSize = eMmcPartitionSizeInSectors * KSectorSize;
		
		TPartitionEntry& part = iPartitionInfo->iEntry[partitionCount - 1];
	
		// Check that the eMmcPartition address space boundary is not exceeded by the last partition
		if(part.iPartitionBaseAddr + part.iPartitionLen > eMmcPartitionSize)
			{
			__KTRACE_OPT(KPBUSDRV, Kern::Printf("Mmc: Partition #%d exceeds eMmc address space", partitionCount));
			r = KErrCorrupt;
			}
		}
		
	if(r == KErrNone)
		{
		// Go through all BB5 partition entries on this eMMC partition and check boundaries
		for(TInt i = partitionCount - 1; i > iPartitionInfo->iPartitionCount; i--)
			{
			const TPartitionEntry& curr = iPartitionInfo->iEntry[i];
			TPartitionEntry& prev = iPartitionInfo->iEntry[i-1];

			// Check if partitions overlap
			if(curr.iPartitionBaseAddr < (prev.iPartitionBaseAddr + prev.iPartitionLen))
				{
				__KTRACE_OPT(KPBUSDRV, Kern::Printf("Mmc: Overlapping partitions - check #%d", i));
				r = KErrCorrupt;
				}
			}
		}
#endif // _DEBUG_CHECK_PARTITION_
		
	if(r == KErrNone)
		{
		iPartitionInfo->iPartitionCount = partitionCount;
		iPartitionInfo->iMediaSizeInBytes = iCard->DeviceSize64();
		}

	__KTRACE_OPT(KPBUSDRV, Kern::Printf("<Mmc:PartitionInfo (C:%d)", partitionCount));
	return r;
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

