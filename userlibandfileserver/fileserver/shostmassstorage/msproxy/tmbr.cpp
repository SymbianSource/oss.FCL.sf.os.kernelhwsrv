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
//
//

/** @file
@internalTechnology
*/

#include <e32cmn.h>
#include <e32des8.h>
#include <partitions.h>

#include "mscutils.h"
#include "tmbr.h"

#include "OstTraceDefinitions.h"
#ifdef OST_TRACE_COMPILER_IN_USE
#include "tmbrTraces.h"
#endif


TInt TMbr::GetPartition(TDes8& aMbrData, TMBRPartitionEntry& aPartitionEntry)
    {
    TUint8* buffer = const_cast<TUint8*>(aMbrData.Ptr());

    // check first sector for a Master Boot Record
    if (LittleEndian::Get16(&buffer[KMBRSignatureOffset]) != KMBRSignature)
        {
        OstTrace0(TRACE_SHOSTMASSSTORAGE_MBR, TMBR_11,
                  "MBR not present");
        return KErrNotFound;
        }

    memcpy(&buffer[0],&buffer[KMBRFirstPartitionOffset],(sizeof(TMBRPartitionEntry)<<2));
    TMBRPartitionEntry* pe = reinterpret_cast<TMBRPartitionEntry*>(&buffer[0]);

    TInt firstValidPartitionCount = -1;
    TInt defaultPartitionNumber = -1;
    TInt partitionCount = 0;
    for (TInt i = 0; i < KMBRMaxPrimaryPartitions; i++, pe++)
        {
        if (pe->IsValidDosPartition() || pe->IsValidFAT32Partition() || pe->IsValidExFATPartition())
            {
            OstTrace1(TRACE_SHOSTMASSSTORAGE_MBR, TMBR_12,
                      "Partition %d is recognized", i);
            partitionCount++;

            if (firstValidPartitionCount < 0)
                firstValidPartitionCount = i;

            if (pe->iX86BootIndicator == KBootIndicatorBootable)
                {
                OstTrace1(TRACE_SHOSTMASSSTORAGE_MBR, TMBR_13,
                          "Partition %d is bootable", i);
                defaultPartitionNumber = i;
                break;
                }
            }
        else
            {
            OstTrace1(TRACE_SHOSTMASSSTORAGE_MBR, TMBR_14,
                      "Partition %d is invalid", i);
            }
        }

    if (partitionCount > 0)
        {
        pe = reinterpret_cast<TMBRPartitionEntry*>(&buffer[0]);
        TInt partitionIndex = firstValidPartitionCount;
        if (defaultPartitionNumber > 0)
            {
            partitionIndex = defaultPartitionNumber;
            }

        OstTrace1(TRACE_SHOSTMASSSTORAGE_MBR, TMBR_15,
                  "Using Partition %d", partitionIndex);

        TMBRPartitionEntry& partitionEntry = pe[partitionIndex];
        aPartitionEntry = partitionEntry;

        OstTraceExt2(TRACE_SHOSTMASSSTORAGE_MBR, TMBR_16,
                     "partitioncount = %d defaultpartition = %d",
                     partitionCount, partitionIndex);
        OstTraceExt2(TRACE_SHOSTMASSSTORAGE_MBR, TMBR_17,
                     "iFirstSector = x%x iNumSectors = x%x",
                     partitionEntry.iFirstSector,
                     partitionEntry.iNumSectors);
        }

    return partitionCount;
    }

