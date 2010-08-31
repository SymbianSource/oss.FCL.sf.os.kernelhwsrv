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
#include "bgahsmmcptn.h"
#include "toc.h"
//#define __DEBUG_PARTITIONS_
//#define __DEBUG_CHECK_PARTITION_
const TInt    KDiskSectorShift          = 9;

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

	return(KErrNone);
	}

TInt DBB5PartitionInfo::PartitionInfo(TPartitionInfo& aInfo, const TMMCCallBack& aCallBack)
	{
	iPartitionInfo = &aInfo;
	iCallBack = aCallBack;

	// Preferred partition scheme is BB5, which is located in the last block of the media.
    const TUint32 ptiOffset = (I64LOW(iCard->DeviceSize64() >> KDiskSectorShift)) - KPIOffsetFromMediaEnd;
	return ReadPartition(ptiOffset);
	}
	
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

	if (r == KErrNone)
		r = DecodePartitionInfo();

	if (!iCheckTOC)
	    {        
	    iDriver->PartitionInfoComplete(r == KErrNone ? r : KErrNotReady);
	    }
	}

TInt DBB5PartitionInfo::DecodePartitionInfo()
//
// Decode partition info that was read into internal buffer
//
	{
	__KTRACE_OPT(KPBUSDRV, Kern::Printf(">Mmc:PartitionInfo()"));
	TUint partitionCount = iPartitionInfo->iPartitionCount = 0;

	
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
        TInt r = KErrNone;

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
                        
                        // ROM/ROFS partitions have a BB5 checksum header that must be offset for the Symbian OS.
                        const TUint32 KstartOffset = ((KPartitionTypeROM == partitionType) || (KPartitionTypeRofs == partitionType) || (KPartitionTypeEmpty == partitionType)) ? KBB5HeaderSizeInSectors : 0;
                        
                        iPartitionInfo->iEntry[partitionCount].iPartitionBaseAddr = ((Int64) partitionTable->iPartitions[index].iStart_sector + KstartOffset) << KDiskSectorShift;
                        iPartitionInfo->iEntry[partitionCount].iPartitionLen      = ((Int64) partitionTable->iPartitions[index].iSize - KstartOffset) << KDiskSectorShift;
        
                    	__KTRACE_OPT(KPBUSDRV, Kern::Printf("Registering partition #%d:", partitionCount));
                    	__KTRACE_OPT(KPBUSDRV, Kern::Printf("partitionCount....: %d", partitionCount));
                    	__KTRACE_OPT(KPBUSDRV, Kern::Printf("startSector.......: 0x%x", partitionTable->iPartitions[index].iStart_sector ));
                    	__KTRACE_OPT(KPBUSDRV, Kern::Printf("iPartitionBaseAddr: 0x%lx (sectors: %d)", iPartitionInfo->iEntry[partitionCount].iPartitionBaseAddr, (TUint32)(iPartitionInfo->iEntry[partitionCount].iPartitionBaseAddr >> KDiskSectorShift)));
                    	__KTRACE_OPT(KPBUSDRV, Kern::Printf("size..............: 0x%lx", partitionTable->iPartitions[index].iSize ));
                    	__KTRACE_OPT(KPBUSDRV, Kern::Printf("iPartitionLen.....: 0x%lx (sectors: %d)", iPartitionInfo->iEntry[partitionCount].iPartitionLen, iPartitionInfo->iEntry[partitionCount].iPartitionLen >> KDiskSectorShift));
                    	__KTRACE_OPT(KPBUSDRV, Kern::Printf("iPartitionType....: %d", iPartitionInfo->iEntry[partitionCount].iPartitionType));
                    	__KTRACE_OPT(KPBUSDRV, Kern::Printf("iPartitionAttribs.: 0x%x", iPartitionAttributes[partitionCount]));
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
		return KErrCorrupt;
		}
#ifdef __DEBUG_CHECK_PARTITION_	
	else
		{
		// at least one entry for a supported partition found
		const TInt64 deviceSize = iCard->DeviceSize64();
		TPartitionEntry& part = iPartitionInfo->iEntry[partitionCount - 1];

		// Check that the card address space boundary is not exceeded by the last partition
		if(part.iPartitionBaseAddr + part.iPartitionLen > deviceSize)
			{
			__KTRACE_OPT(KPBUSDRV, Kern::Printf("Mmc: MBR partition exceeds card memory space"));
			return KErrCorrupt;
			}

		// Go through all partition entries and check boundaries
		for(TInt i = partitionCount - 1; i > 0; i--)
			{
			const TPartitionEntry& curr = iPartitionInfo->iEntry[i];
			TPartitionEntry& prev = iPartitionInfo->iEntry[i-1];

			// Check if partitions overlap
			if(curr.iPartitionBaseAddr < (prev.iPartitionBaseAddr + prev.iPartitionLen))
				{
				__KTRACE_OPT(KPBUSDRV, Kern::Printf("Mmc: Overlapping partitions - check #%d", i));
				return KErrCorrupt;
				}
			}
		}
#endif // _DEBUG_CHECK_PARTITION_

	iPartitionInfo->iPartitionCount = partitionCount;
	iPartitionInfo->iMediaSizeInBytes = iCard->DeviceSize64();

	//Notify medmmc that partitioninfo is complete.
	iCallBack.CallBack();

	__KTRACE_OPT(KPBUSDRV, Kern::Printf("<Mmc:PartitionInfo (C:%d)", partitionCount));
	return KErrNone;
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

