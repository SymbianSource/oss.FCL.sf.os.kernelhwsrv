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

const TInt	  KDiskSectorShift		= 9;
const TUint32 KPIOffsetFromMediaEnd = 1;

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
	};

DBB5PartitionInfo::DBB5PartitionInfo()
  : iSessionEndCallBack(DBB5PartitionInfo::SessionEndCallBack, this)
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

	// If media driver is persistent (see EMediaDriverPersistent)
	// the card may have changed since last power down, so reset CID
	iSession->SetCard(iCard);

	const TUint32 ptiOffset = (I64LOW(iCard->DeviceSize64() >> KDiskSectorShift)) - KPIOffsetFromMediaEnd;
	iSession->SetupCIMReadBlock(ptiOffset, iIntBuf);

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

	iDriver->PartitionInfoComplete(r == KErrNone ? r : KErrNotReady);
	}

TInt DBB5PartitionInfo::DecodePartitionInfo()
//
// decode partition info that was read into internal buffer
//
	{
	__KTRACE_OPT(KPBUSDRV, Kern::Printf(">Mmc:PartitionInfo()"));
	TUint partitionCount = iPartitionInfo->iPartitionCount = 0;

	// For internal devices it is only valid to report up to 1 SWAP partition
	TBool foundSwap = EFalse;

	BGAHSMMCPTN_PI_STR *partitionTable = (BGAHSMMCPTN_PI_STR*)(&iIntBuf[0]);

	// Verify that this is the Nokia partition table
	if( memcompare( (TUint8*)&(partitionTable->id[0]), sizeof(BGAHSMMCPTN_PI_ID), (TUint8*)BGAHSMMCPTN_PI_ID, sizeof(BGAHSMMCPTN_PI_ID)) == 0 )
		{
		__KTRACE_OPT(KPBUSDRV, Kern::Printf("Nokia partition structure found"));
		__KTRACE_OPT(KPBUSDRV, Kern::Printf("partitionTable->id..............: %s", partitionTable->id ));
		__KTRACE_OPT(KPBUSDRV, Kern::Printf("partitionTable->sector_size.....: %d = 0x%x", partitionTable->sector_size, partitionTable->sector_size));
		__KTRACE_OPT(KPBUSDRV, Kern::Printf("partitionTable->major_ver.......: %d", partitionTable->major_ver));
		__KTRACE_OPT(KPBUSDRV, Kern::Printf("partitionTable->minor_ver.......: %d", partitionTable->minor_ver));
		__KTRACE_OPT(KPBUSDRV, Kern::Printf("partitionTable->partition_amount: %d", partitionTable->partition_amount));
		
		for( TUint8 index = 0; (index < partitionTable->partition_amount) && (index < BGAHSMMCPTN_LAST_DRIVE); index++ )
			{
			if( (partitionTable->partitions[index].size > 0) &&
				( PartitionIsFAT(partitionTable->partitions[index].partition_id) ||
				  PartitionIsFAT32(partitionTable->partitions[index].partition_id) ||
				  (KPartitionTypePagedData == partitionTable->partitions[index].partition_id && !foundSwap) ) )
				{
				iPartitionInfo->iEntry[partitionCount].iPartitionType	  = partitionTable->partitions[index].partition_id;
				iPartitionInfo->iEntry[partitionCount].iPartitionBaseAddr = (Int64) partitionTable->partitions[index].start_sector << KDiskSectorShift;
				iPartitionInfo->iEntry[partitionCount].iPartitionLen	  = (Int64) partitionTable->partitions[index].size << KDiskSectorShift;
				iPartitionAttributes[partitionCount]					  = partitionTable->partitions[index].partition_attributes;

				__KTRACE_OPT(KPBUSDRV, Kern::Printf("Registering partition #%d:", partitionCount));
				__KTRACE_OPT(KPBUSDRV, Kern::Printf("partitionCount....: %d", partitionCount));
				__KTRACE_OPT(KPBUSDRV, Kern::Printf("startSector.......: 0x%x", partitionTable->partitions[index].start_sector ));
				__KTRACE_OPT(KPBUSDRV, Kern::Printf("iPartitionBaseAddr: 0x%lx (sectors: %d)", iPartitionInfo->iEntry[partitionCount].iPartitionBaseAddr, (TUint32)(iPartitionInfo->iEntry[partitionCount].iPartitionBaseAddr >> KDiskSectorShift)));
				__KTRACE_OPT(KPBUSDRV, Kern::Printf("size..............: 0x%lx", partitionTable->partitions[index].size ));
				__KTRACE_OPT(KPBUSDRV, Kern::Printf("iPartitionLen.....: 0x%lx (sectors: %d)", iPartitionInfo->iEntry[partitionCount].iPartitionLen, iPartitionInfo->iEntry[partitionCount].iPartitionLen >> KDiskSectorShift));
				__KTRACE_OPT(KPBUSDRV, Kern::Printf("iPartitionType....: %d", iPartitionInfo->iEntry[partitionCount].iPartitionType));
				__KTRACE_OPT(KPBUSDRV, Kern::Printf("iPartitionAttribs.: 0x%x", iPartitionAttributes[partitionCount]));
				__KTRACE_OPT(KPBUSDRV, Kern::Printf(" "));

				if(KPartitionTypePagedData == partitionTable->partitions[index].partition_id)
					{
					foundSwap = ETrue;
					}

				partitionCount++;
				}
			}
		}

	// Validate partition address boundaries
	if(partitionCount == 0)
		{
		__KTRACE_OPT(KPBUSDRV, Kern::Printf("Mmc: No supported partitions found!"));
		return KErrCorrupt;
		}
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

