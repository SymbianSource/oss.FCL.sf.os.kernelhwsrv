// Copyright (c) 2009 Nokia Corporation and/or its subsidiary(-ies).
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

#include <drivers/emmcptn.h>
#include "OstTraceDefinitions.h"
#ifdef OST_TRACE_COMPILER_IN_USE
#include "locmedia_ost.h"
#ifdef __VC32__
#pragma warning(disable: 4127) // disabling warning "conditional expression is constant"
#endif
#include "emmcptnTraces.h"
#endif


const TInt KDiskSectorShift=9;

class DLegacyEMMCPartitionInfo : public DEMMCPartitionInfo
	{
public:
	 DLegacyEMMCPartitionInfo();
	~DLegacyEMMCPartitionInfo();
public:
	virtual TInt Initialise(DMediaDriver* aDriver);
	virtual TInt PartitionInfo(TPartitionInfo& anInfo, const TMMCCallBack& aCallBack);
	virtual TInt PartitionCaps(TLocDrv& aDrive, TDes8& aInfo);
	
protected:
	void SetPartitionEntry(TPartitionEntry* aEntry, TUint aFirstSector, TUint aNumSectors);
	
private:
	static void SessionEndCallBack(TAny* aSelf);
		   void DoSessionEndCallBack();
	virtual TInt DecodePartitionInfo();
	
protected:
	DMediaDriver*   iDriver;
	TPartitionInfo* iPartitionInfo;
	TMMCCallBack	iSessionEndCallBack;
	TMMCCallBack 	iCallBack;         // Where to report the PartitionInfo completion
	DMMCSession*	iSession;
	TMMCard*		iCard;
	TUint8*			iIntBuf;
	};

DLegacyEMMCPartitionInfo::DLegacyEMMCPartitionInfo()
  : iSessionEndCallBack(DLegacyEMMCPartitionInfo::SessionEndCallBack, this)
	{
	OstTraceFunctionEntry0( DLEGACYEMMCPARTITIONINFO_DLEGACYEMMCPARTITIONINFO_ENTRY );
	}

DLegacyEMMCPartitionInfo::~DLegacyEMMCPartitionInfo()
	{
	OstTraceFunctionEntry0( DLEGACYEMMCPARTITIONINFO_DESTRUCTOR_ENTRY );
	delete iSession;
	OstTraceFunctionExit0( DLEGACYEMMCPARTITIONINFO_DESTRUCTOR_EXIT );
	}

TInt DLegacyEMMCPartitionInfo::Initialise(DMediaDriver* aDriver)
	{
	OstTraceFunctionEntry1( DLEGACYEMMCPARTITIONINFO_INITIALISE_ENTRY, this );
	iDriver = aDriver;

	DMMCSocket* socket = ((DMMCSocket*)((DPBusPrimaryMedia*)(iDriver->iPrimaryMedia))->iSocket);
	if(socket == NULL)
	    {
		OstTraceFunctionExitExt( DLEGACYEMMCPARTITIONINFO_INITIALISE_EXIT1, this, KErrNoMemory );
		return KErrNoMemory;
	    }

	DMMCStack* stack = socket->Stack(0);
	iCard = stack->CardP(((DPBusPrimaryMedia*)(iDriver->iPrimaryMedia))->iSlotNumber);
	
	iSession = stack->AllocSession(iSessionEndCallBack);
	if (iSession == NULL)
		return KErrNoMemory;

	iSession->SetStack(stack);
	iSession->SetCard(iCard);

	// this gets used before any access
	TInt bufLen, minorBufLen;
	stack->BufferInfo(iIntBuf, bufLen, minorBufLen);

	OstTraceFunctionExitExt( DLEGACYEMMCPARTITIONINFO_INITIALISE_EXIT2, this, KErrNone );
	return KErrNone;
	}

TInt DLegacyEMMCPartitionInfo::PartitionInfo(TPartitionInfo& anInfo, const TMMCCallBack& aCallBack)
	{
	OstTraceFunctionEntry1( DLEGACYEMMCPARTITIONINFO_PARTITIONINFO_ENTRY, this );
	iPartitionInfo = &anInfo;
	iCallBack = aCallBack;
	// If media driver is persistent (see EMediaDriverPersistent), 
	// the card may have changed since last power down, so reset CID
	iSession->SetCard(iCard);
	
	iSession->SetupCIMReadBlock(0, iIntBuf);

	TInt r = iDriver->InCritical();
	if (r == KErrNone)
		r = iSession->Engage();

	if(r != KErrNone)
		iDriver->EndInCritical();
	
	OstTraceFunctionExitExt( DLEGACYEMMCPARTITIONINFO_PARTITIONINFO_EXIT, this, r );
	return r;
	}

TInt DLegacyEMMCPartitionInfo::PartitionCaps(TLocDrv& aDrive, TDes8& aInfo)
	{
	 OstTraceFunctionEntry1( DLEGACYEMMCPARTITIONINFO_PARTITIONCAPS_ENTRY, this );
	 TLocalDriveCapsV6Buf& Info = static_cast< TLocalDriveCapsV6Buf&> (aInfo);
	
	// is this query for the swap partition ?
	if (aDrive.iPartitionType == KPartitionTypePagedData)
		{
		Info().iFileSystemId = KDriveFileNone;
		Info().iDriveAtt|= KDriveAttHidden;
		}

	// is this query for the ROFS partition ?
	if (aDrive.iPartitionType == KPartitionTypeRofs)
		{
		Info().iFileSystemId = KDriveFileSysROFS;
		Info().iMediaAtt&= ~KMediaAttFormattable;
		Info().iMediaAtt|= KMediaAttWriteProtected;
		}
	
	// is this query for the ROM partition ?
	if (aDrive.iPartitionType == KPartitionTypeROM)
		{
		Info().iFileSystemId = KDriveFileNone;
		Info().iMediaAtt&= ~KMediaAttFormattable;
		Info().iMediaAtt|= KMediaAttWriteProtected;
		}
	
	OstTraceFunctionExitExt( DLEGACYEMMCPARTITIONINFO_PARTITIONCAPS_EXIT, this, KErrNone );
	return KErrNone;
	}

void DLegacyEMMCPartitionInfo::SessionEndCallBack(TAny* aSelf)
	{
	OstTraceFunctionEntry0( DLEGACYEMMCPARTITIONINFO_SESSIONENDCALLBACK_ENTRY );
	DLegacyEMMCPartitionInfo& self = *static_cast<DLegacyEMMCPartitionInfo*>(aSelf);
	self.DoSessionEndCallBack();
	OstTraceFunctionExit0( DLEGACYEMMCPARTITIONINFO_SESSIONENDCALLBACK_EXIT );
	}

void DLegacyEMMCPartitionInfo::DoSessionEndCallBack()
	{
	OstTraceFunctionEntry1( DLEGACYEMMCPARTITIONINFO_DOSESSIONENDCALLBACK_ENTRY, this );
	iDriver->EndInCritical();

	TInt r = iSession->EpocErrorCode();

	if (r == KErrNone)
		r = DecodePartitionInfo();

	iDriver->PartitionInfoComplete(r == KErrNone ? r : KErrNotReady);
	OstTraceFunctionExit1( DLEGACYEMMCPARTITIONINFO_DOSESSIONENDCALLBACK_EXIT, this );
	}

TInt DLegacyEMMCPartitionInfo::DecodePartitionInfo()
//
// decode partition info that was read into internal buffer 
//
	{
	OstTraceFunctionEntry1( DLEGACYEMMCPARTITIONINFO_DECODEPARTITIONINFO_ENTRY, this );
	TUint partitionCount=iPartitionInfo->iPartitionCount=0;
	TInt defaultPartitionNumber=-1;
	TMBRPartitionEntry* pe;
	const TUint KMBRFirstPartitionOffsetAligned = KMBRFirstPartitionOffset & ~3;
	TInt i;

	// Read of the first sector successful so check for a Master Boot Record
	if (*(TUint16*)(&iIntBuf[KMBRSignatureOffset])!=0xAA55)
	    {
		// If no valid signature give up now, No way to re-format an internal drive correctly
		OstTraceFunctionExitExt( DLEGACYEMMCPARTITIONINFO_DECODEPARTITIONINFO_EXIT1, this, KErrCorrupt );
		return KErrCorrupt;
	    }

	__ASSERT_COMPILE(KMBRFirstPartitionOffsetAligned + KMBRMaxPrimaryPartitions * sizeof(TMBRPartitionEntry) <= KMBRSignatureOffset);

	memmove(&iIntBuf[0], &iIntBuf[2],
		KMBRFirstPartitionOffsetAligned + KMBRMaxPrimaryPartitions * sizeof(TMBRPartitionEntry)); 


	for (i=0, pe = (TMBRPartitionEntry*)(&iIntBuf[KMBRFirstPartitionOffsetAligned]);
		pe->iPartitionType != 0 && i < KMaxPartitionEntries; i++,pe--)
		{
		if (pe->IsDefaultBootPartition())
			{
			SetPartitionEntry(&iPartitionInfo->iEntry[0],pe->iFirstSector,pe->iNumSectors);
			defaultPartitionNumber=i;
			partitionCount++;
			break;
			}
		}

	// Now add any other partitions
	for (i=0, pe = (TMBRPartitionEntry*)(&iIntBuf[KMBRFirstPartitionOffsetAligned]);
		pe->iPartitionType != 0 && i < KMaxPartitionEntries; i++,pe--)
		{
		if (defaultPartitionNumber==i)
			{
			// Already sorted
			}

		// FAT partition ?
		else if (pe->IsValidDosPartition() || pe->IsValidFAT32Partition() || pe->IsValidExFATPartition())
			{
			SetPartitionEntry(&iPartitionInfo->iEntry[partitionCount],pe->iFirstSector,pe->iNumSectors);
			__KTRACE_OPT(KLOCDPAGING, Kern::Printf("Mmc: FAT partition found at sector #%u", pe->iFirstSector));
			OstTrace1(TRACE_INTERNALS, DLEGACYEMMCPARTITIONINFO_DECODEPARTITIONINFO_FAT, "FAT partition found at sector #%x", pe->iFirstSector);
			partitionCount++;
			}

		else if (pe->iPartitionType == KPartitionTypeROM)
			{
			TPartitionEntry& partitionEntry = iPartitionInfo->iEntry[partitionCount];
			SetPartitionEntry(&iPartitionInfo->iEntry[partitionCount],pe->iFirstSector,pe->iNumSectors);
			partitionEntry.iPartitionType = pe->iPartitionType;
			partitionCount++;				 

			__KTRACE_OPT(KLOCDPAGING, Kern::Printf("Mmc: KPartitionTypeROM found at sector #%u", pe->iFirstSector));
			OstTrace1(TRACE_INTERNALS, DLEGACYEMMCPARTITIONINFO_DECODEPARTITIONINFO_ROM, "KPartitionTypeROM found at sector #%x", pe->iFirstSector);
			}

		// ROFS partition ?
		else if (pe->iPartitionType == KPartitionTypeRofs)
			{
			
// Don't expose this for normal operation only boot?			
			TPartitionEntry& partitionEntry = iPartitionInfo->iEntry[partitionCount];
			SetPartitionEntry(&iPartitionInfo->iEntry[partitionCount],pe->iFirstSector,pe->iNumSectors);
			partitionEntry.iPartitionType = pe->iPartitionType;
			__KTRACE_OPT(KLOCDPAGING, Kern::Printf("Mmc: KPartitionTypeRofs found at sector #%u", pe->iFirstSector));
			OstTrace1(TRACE_INTERNALS, DLEGACYEMMCPARTITIONINFO_DECODEPARTITIONINFO_ROFS, "KPartitionTypeRofs found at sector #%x", pe->iFirstSector);
			partitionCount++;
			}
 
		// Swap partition ?
		else if (pe->iPartitionType == KPartitionTypePagedData)
			{
			__KTRACE_OPT(KLOCDPAGING, Kern::Printf("Mmc: KPartitionTypePagedData found at sector #%u", pe->iFirstSector));
			OstTrace1(TRACE_INTERNALS, DLEGACYEMMCPARTITIONINFO_DECODEPARTITIONINFO_PAGED, "KPartitionTypeRofs found at sector #%x", pe->iFirstSector);
			TPartitionEntry& partitionEntry = iPartitionInfo->iEntry[partitionCount];
			SetPartitionEntry(&iPartitionInfo->iEntry[partitionCount],pe->iFirstSector,pe->iNumSectors);
			partitionEntry.iPartitionType = pe->iPartitionType;
			partitionCount++;
			}
		}

	// Check the validity of the partition address boundaries
	// If there is any MBR errors
	if(partitionCount > 0)
		{
		const TInt64 deviceSize = iCard->DeviceSize64();
		TPartitionEntry& part = iPartitionInfo->iEntry[partitionCount - 1];
		// Check that the card address space boundary is not exceeded by the last partition
		if(part.iPartitionBaseAddr + part.iPartitionLen > deviceSize)
			{
			__KTRACE_OPT(KPBUSDRV, Kern::Printf("Mmc: MBR partition exceeds card memory space"));
			OstTraceFunctionExitExt( DLEGACYEMMCPARTITIONINFO_DECODEPARTITIONINFO_EXIT2, this, KErrCorrupt );
			return KErrCorrupt;
			}
		
		// More than one partition. Go through all of them
		if (partitionCount > 0)
			{
			for(i=partitionCount-1; i>0; i--)
				{
				const TPartitionEntry& curr = iPartitionInfo->iEntry[i];
				TPartitionEntry& prev = iPartitionInfo->iEntry[i-1];
				// Check if partitions overlap
				if(curr.iPartitionBaseAddr < (prev.iPartitionBaseAddr + prev.iPartitionLen))
					{
					__KTRACE_OPT(KPBUSDRV, Kern::Printf("Mmc: Overlapping partitions"));
					OstTraceFunctionExitExt( DLEGACYEMMCPARTITIONINFO_DECODEPARTITIONINFO_EXIT3, this, KErrCorrupt );
					return KErrCorrupt;
					}
				}
			}
		}

	if (defaultPartitionNumber==(-1) && partitionCount==0)
		{
		__KTRACE_OPT(KPBUSDRV, Kern::Printf("No Valid Partitions Found!"));
		OstTraceFunctionExitExt( DLEGACYEMMCPARTITIONINFO_DECODEPARTITIONINFO_EXIT4, this, KErrCorrupt );
		return KErrCorrupt;
		}

	iPartitionInfo->iPartitionCount=partitionCount;
	iPartitionInfo->iMediaSizeInBytes=iCard->DeviceSize64();

#ifdef _DEBUG
	__KTRACE_OPT(KPBUSDRV, Kern::Printf("<Mmc:PartitionInfo (C:%d)",partitionCount));
	for (TUint x=0; x<partitionCount; x++)
		__KTRACE_OPT(KPBUSDRV, Kern::Printf("     Partition%d (B:%xH L:%xH)",x,I64LOW(iPartitionInfo->iEntry[x].iPartitionBaseAddr),I64LOW(iPartitionInfo->iEntry[x].iPartitionLen)));
#endif

	//Notify medmmc that partitioninfo is complete.
	iCallBack.CallBack();
	
	OstTraceFunctionExitExt( DLEGACYEMMCPARTITIONINFO_DECODEPARTITIONINFO_EXIT5, this, KErrNone );
	return KErrNone;
	}


void DLegacyEMMCPartitionInfo::SetPartitionEntry(TPartitionEntry* aEntry, TUint aFirstSector, TUint aNumSectors)
//
// auxiliary static function to record partition information in TPartitionEntry object
//
	{
	OstTraceFunctionEntryExt( DLEGACYEMMCPARTITIONINFO_SETPARTITIONENTRY_ENTRY, this );
	aEntry->iPartitionBaseAddr=aFirstSector;
	aEntry->iPartitionBaseAddr<<=KDiskSectorShift;
	aEntry->iPartitionLen=aNumSectors;
	aEntry->iPartitionLen<<=KDiskSectorShift;
	aEntry->iPartitionType=KPartitionTypeFAT12;
	OstTraceFunctionExit1( DLEGACYEMMCPARTITIONINFO_SETPARTITIONENTRY_EXIT, this );
	}

// End - DLegacyEMMCPartitionInfo


EXPORT_C DEMMCPartitionInfo* CreateEmmcPartitionInfo()
	{
	OstTraceFunctionEntry0( _CREATEEMMCPARTITIONINFO_ENTRY );
	return new DLegacyEMMCPartitionInfo;
	}

DECLARE_STANDARD_EXTENSION()
	{
	return KErrNone;
	}
