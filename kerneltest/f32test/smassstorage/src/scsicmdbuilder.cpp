// Copyright (c) 2004-2009 Nokia Corporation and/or its subsidiary(-ies).
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
// Provides utilities to build SCSI commands
// 
//

/**
 @file
 @internalTechnology
*/

#include <e32std.h>
#include <e32std_private.h>
#include <testusbc.h>
#include "scsicmdbuilder.h"

#define SCSI_READ_WRITE_LEN       10
#define SCSI_UNIT_READY_LEN        6
#define SCSI_MED_RMVL_LEN          6
#define SCSI_START_STOP_LEN        6

GLDEF_D TBuf8<KScsiCmdMaxLen> scsiCmdBuf;
GLREF_D RDevTestUsbcClient usbcClient;

void BuildReadWrite(TInt aFlag, TInt aLogicalBlkAddr, TInt aTotalBlks)
    // 
    // Builds SCSI read(10) / write(10) command
    //
    {
    // Zero out the whole buffer
    scsiCmdBuf.FillZ(SCSI_READ_WRITE_LEN);
    scsiCmdBuf[0] = TUint8(aFlag);     // opcode for read (0x28)/write (0x2A)

    // Fill in logical block address. Big endian
    scsiCmdBuf[2] = TUint8((aLogicalBlkAddr >> 24) & 0xFF);
    scsiCmdBuf[3] = TUint8((aLogicalBlkAddr >> 16) & 0xFF);
    scsiCmdBuf[4] = TUint8((aLogicalBlkAddr >> 8) & 0xFF);
    scsiCmdBuf[5] = TUint8(aLogicalBlkAddr & 0xFF);
    
    // Data transfer length (# of sectors). Big endian
    scsiCmdBuf[7] = TUint8((aTotalBlks >> 8) & 0xFF);
    scsiCmdBuf[8] = TUint8((aTotalBlks & 0xFF));

    scsiCmdBuf.SetLength(SCSI_READ_WRITE_LEN);
    }

void BuildMediumRemoval(TInt aFlag)
    //
    // Builds prevent/allow medium removal command
    // 
    {
    // Zero out the buf
    scsiCmdBuf.SetLength(SCSI_MED_RMVL_LEN);
    scsiCmdBuf.FillZ(SCSI_MED_RMVL_LEN);
    scsiCmdBuf[0] = 0x1E;             // opcode for medium removal
    scsiCmdBuf[4] = TUint8(aFlag);    // prevent(1)/allow(0) medium removal 
    }

void BuildTestUnitReady()
    //
    // Builds test unit ready command
    // 
    {
    // Zero out the buf
    scsiCmdBuf.FillZ(SCSI_UNIT_READY_LEN);
    scsiCmdBuf[0] = 0;                // opcode for test unit ready

    scsiCmdBuf.SetLength(SCSI_UNIT_READY_LEN);
    }

void BuildStartStopUnit(TInt aFlag)
    //
    // Builds start/stop unit command
    // 
    {
    const TUint8 KStartMask = 0x01;
    const TUint8 KLoejMask = 0x02;
    // Zero out the buf
    scsiCmdBuf.FillZ(SCSI_START_STOP_LEN);
    scsiCmdBuf[0] = 0x1B;             // opcode for test unit ready
    scsiCmdBuf[1] = 0x01;			  // set immed bit to true
    scsiCmdBuf[4] = TUint8((aFlag == 1) ? KLoejMask & KStartMask : KLoejMask);  // LOEJ + START or STOP

    scsiCmdBuf.SetLength(SCSI_START_STOP_LEN);
    }

void fillInt(TUint8* dest, TInt source)
    // 
    // Copy an int. Little endian
    //
    {
    for (TInt i = 0; i < 4; i++)
        {
        *dest++ = TUint8((source >> i*8) & 0xFF);
        }
    }

TInt extractInt(const TUint8* aBuf)
    //
    // Extract an integer from a buffer. Assume little endian
    //
    {
    return aBuf[0] + (aBuf[1] << 8) + (aBuf[2] << 16) + (aBuf[3] << 24);
    }

void createCBW(TDes8& aCbw, TInt aDCBWTag, TInt aDataTransferLen, TUint8 aInOutFlag, TDes8& aCBWCB, TUint8 aTestLun)
    // 
    // aCbw:            stores CBW
    // aDCBWTag:        a command block tag sent by the host. Used to associates a CSW
    //                  with corresponding CBW
    // aDataTranferLen: the number of bytes the host expects to transfer
    // aInOutFlag:      value for bmCBWFlags field, indicating the direction of transfer
    // aCBWCB			the actual command to be wrapped
    // aTestLun			local unit number
    {
    // Zero out aCbw
    aCbw.SetLength(KCbwLength);
    aCbw.FillZ();

    // dCBWSignature field, the value comes from spec
    TInt dCBWSignature = 0x43425355;
    fillInt(&aCbw[0], dCBWSignature);
    // dCBWTag field
    fillInt(&aCbw[4], aDCBWTag);
    // dCBWDataTransferLength field
    fillInt(&aCbw[8], aDataTransferLen);
    // bmCBWFlags field
    aCbw[12] = aInOutFlag;
	aCbw[13] = aTestLun;
    // bCBWCBLength field
    aCbw[14] = TUint8(aCBWCB.Length());          
	
    // CBWCB field
    for (TInt i = 0; i < aCbw[14]; ++i)
    	{
    	aCbw[15 + i] = aCBWCB[i];
    	}
    }

