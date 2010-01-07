// Copyright (c) 2008-2009 Nokia Corporation and/or its subsidiary(-ies).
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

/**
 @file
 @internalTechnology
*/

#include <e32base.h>

#include "msdebug.h"
#include "debug.h"
#include "msctypes.h"

#include "mtransport.h"
#include "mprotocol.h"
#include "tscsiclientreq.h"

#include "tblocktransfer.h"
#include "mscutils.h"
#include "usbmshostpanic.h"
#include "tscsiblockcmds.h"
#include "tsbcclientinterface.h"

/**
Constructor.

@param aTransport The Transport interface to be used
*/
TSbcClientInterface::TSbcClientInterface(MTransport& aTransport)
:   iTransport(aTransport)
    {
    __MSFNLOG
    }


TSbcClientInterface::~TSbcClientInterface()
    {
    __MSFNLOG
    }


/**
Constructor to create and send SCSI MODE SENSE (6) request to obtain the
medium's Write Protect status. The function leaves if the device response is not
compliant with the protocol standard.

@param aPageCode The SCSI PAGE CODE value
@param aWriteProtected The SCSI WP value

@return TInt KErrNone if successful, KErrCommandFailed to indicate a
device status error, KErrCommandStalled to indicate device stall
*/
TInt TSbcClientInterface::ModeSense6L(TUint aPageCode, TBool& aWriteProtected)
    {
    __MSFNLOG
    TScsiClientModeSense6Req modeSense6Req(TScsiClientModeSense6Req::ECurrentValues,
                                           aPageCode);
    TScsiClientModeSense6Resp modeSense6Resp;
    TInt err = iTransport.SendControlCmdL(&modeSense6Req, &modeSense6Resp);
    if (!err)
        {
        __SCSIPRINT1(_L("SCSI MODE SENSE (6) INFO WrProtect=%d"),
                     modeSense6Resp.iWriteProtected);
        aWriteProtected = modeSense6Resp.iWriteProtected;
        }
    else
        {
        aWriteProtected = EFalse;
        }
	return err;
    }


/**
Create and send SCSI MODE SENSE (10) request to obtain the mediums Write Protect
status. The function leaves if the device response is not compliant with the
protocol standard.

@param aPageCode The SCSI PAGE CODE value
@param aWriteProtected The SCSI WP value

@return TInt KErrNone if successful, KErrCommandFailed to indicate a
device status error, KErrCommandStalled to indicate a device stall
*/
TInt TSbcClientInterface::ModeSense10L(TUint aPageCode, TBool& aWriteProtected)
    {
    __MSFNLOG
    TScsiClientModeSense10Req modeSense10Req(TScsiClientModeSense10Req::ECurrentValues,
                                             aPageCode);
    TScsiClientModeSense10Resp modeSense10Resp;
    TInt err = iTransport.SendControlCmdL(&modeSense10Req, &modeSense10Resp);

    if (!err)
        {
        __SCSIPRINT1(_L("SCSI MODE SENSE (10) INFO WrProtect=%d"),
                     modeSense10Resp.iWriteProtected);
        aWriteProtected = modeSense10Resp.iWriteProtected;
        }
    else
        {
        aWriteProtected = EFalse;
        }
	return err;
    }


/**
Constructor to create SCSI MODE SENSE (10) request.

@param aPageControl The SCSI PAGE CODE value
@param aPageCode The SCSI WP value
@param aSubPageCode The SCSI SUB PAGE CODE value
*/
TScsiClientModeSense10Req::TScsiClientModeSense10Req(TPageControl aPageControl,
                                                     TUint aPageCode,
                                                     TUint aSubPageCode)
    :
    TScsiClientReq(EModeSense10),
    iPageControl(aPageControl),
    iPageCode(aPageCode),
    iSubPageCode(aSubPageCode),
    iAllocationLength(KResponseLength)
    {
    __MSFNLOG
    }


TInt TScsiClientModeSense10Req::EncodeRequestL(TDes8& aBuffer) const
    {
    __MSFNSLOG
    __SCSIPRINT(_L("<-- SCSI MODE SENSE (10)"));
    TInt length = TScsiClientReq::EncodeRequestL(aBuffer);

    // PC
    aBuffer[2] = iPageCode;
    aBuffer[2] |= iPageControl << 6;
    aBuffer[3] = iSubPageCode;

    BigEndian::Put16(&aBuffer[7], iAllocationLength);
    return length;
    }

/**
Create READ (10) request and send to the transport layer. This performs a
logical block read of the device server. The received data is appended into the
copy buffer. The function leaves if the device response is not compliant with
the protocol standard.
Note that TBlockTransfer must be initialised beforehand.

@param aLba The Logical Block address to read from
@param aBuffer The buffer to copy data to
@param aLen The number of bytes to be read (IN) and returns the number of bytes
actually read (OUT)

@return TInt KErrNone if successful otherwise KErrCommandFailed to indicate a
device status error or KErrArgument to indicate that aLen is too large for the
protocol.
*/
TInt TSbcClientInterface::Read10L(TLba aLba, TDes8& aBuffer, TInt& aLen)
    {
    __MSFNLOG
	__ASSERT_DEBUG(iBlockTransfer.BlockLength(), User::Panic(KUsbMsHostPanicCat, EBlockLengthNotSet));
    __ASSERT_DEBUG(aLen % iBlockTransfer.BlockLength() == 0, User::Panic(KUsbMsHostPanicCat, EBlockDevice));

    TScsiClientRead10Req read10Req;

    read10Req.iLogicalBlockAddress = aLba;

    TInt blockTransferLength = aLen / iBlockTransfer.BlockLength();
    if (blockTransferLength > static_cast<TInt>(KMaxTUint16))
        {
        User::Leave(KErrArgument);
        }
    read10Req.iBlockTransferLength = static_cast<TUint16>(blockTransferLength);
    TInt err = iTransport.SendDataRxCmdL(&read10Req, aBuffer, aLen);
	return err;
    }

/**
Create READ CAPACITY (10) request and send to the transport layer. The request
returns the device servers capacity information. The device server's response
values are also used here to initialise the TBlockTransfer values. The function
leaves if the device response is not compliant with the protocol standard.

@param aLba The Logical Block Address returned by the LU
@param aBlockSize The Block Size returned by the LU

@return TInt KErrNone if successful, KErrCommandFailed to indicate a
device status error, KErrCommandStalled to indicate a device stall
*/
TInt TSbcClientInterface::ReadCapacity10L(TLba& aLba, TUint32& aBlockSize)
    {
    __MSFNLOG
    TScsiClientReadCapacity10Req capacity10Req;
    TScsiClientReadCapacity10Resp capacity10Resp;

    TInt err = iTransport.SendControlCmdL(&capacity10Req, &capacity10Resp);
    if (!err)
        {
        aLba = capacity10Resp.iLba;
        aBlockSize = capacity10Resp.iBlockSize;

        __SCSIPRINT2(_L("Capacity LBA=0x%08x SIZE=0x%08x"),
                     aLba, aBlockSize);

        iBlockTransfer.SetCapacityL(aBlockSize, aLba);
        }
	return err;
    }


/**
Create START STOP UNIT request. The function leaves if the device response is
not compliant with the protocol standard.

@param aStart SCSI START value

@return TInt KErrNone if successful otherwise KErrCommandFailed to indicate a
device status error
*/
TInt TSbcClientInterface::StartStopUnitL(TBool aStart)
    {
    __MSFNLOG
    TScsiClientStartStopUnitReq startStopUnitReq;

    startStopUnitReq.iImmed = ETrue;
    startStopUnitReq.iLoej = EFalse;
    startStopUnitReq.iStart = aStart;

    TInt err = iTransport.SendControlCmdL(&startStopUnitReq);

	return err;
    }


/**
Create WRITE (10) request and send to the transport layer. This performs a
logical block write of the device server. Note that TBlockTransfer must be
initialised beforehand.  The function leaves if the device response is not
compliant with the protocol standard.

@param aLba Logical Block Address to write the data to
@param aBuffer Buffer containing the data
@param aPos Offset into the buffer to the data
@param aLen The number of bytes to be written (IN) and returns the bytes
actually transferred (OUT)

@return TInt KErrNone if successful otherwise KErrCommandFailed to indicate a
device status error or KErrArgument to indicate that aLen is too large for the
protocol.
*/
TInt TSbcClientInterface::Write10L(TLba aLba, TDesC8& aBuffer, TUint aPos, TInt& aLen)
    {
    __MSFNLOG
	__ASSERT_DEBUG(iBlockTransfer.BlockLength(), User::Panic(KUsbMsHostPanicCat, EBlockLengthNotSet));
    __ASSERT_DEBUG(aLen % iBlockTransfer.BlockLength() == 0, User::Panic(KUsbMsHostPanicCat, EBlockDevice));

    // check that buffer size is large enough
	if (aBuffer.Length() < (aPos + aLen))
		{
        User::Leave(KErrArgument);
		}

    TScsiClientWrite10Req write10Req;
    write10Req.iLogicalBlockAddress = aLba;

    TInt blockTransferLength = aLen / iBlockTransfer.BlockLength();
    if (blockTransferLength > static_cast<TInt>(KMaxTUint16))
        {
        User::Leave(KErrArgument);
        }
    write10Req.iBlockTransferLength = static_cast<TUint16>(blockTransferLength);

    TInt err = iTransport.SendDataTxCmdL(&write10Req, aBuffer, aPos, aLen);
	return err;
    }

