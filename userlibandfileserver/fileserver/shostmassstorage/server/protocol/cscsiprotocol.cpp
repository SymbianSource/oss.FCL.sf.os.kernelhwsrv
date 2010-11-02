// Copyright (c) 2008-2010 Nokia Corporation and/or its subsidiary(-ies).
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

#include "OstTraceDefinitions.h"
#ifdef OST_TRACE_COMPILER_IN_USE
#include "cscsiprotocolTraces.h"
#endif

#include "msctypes.h"
#include "shared.h"
#include "msgservice.h"

#include "mtransport.h"
#include "mprotocol.h"
#include "tscsiclientreq.h"
#include "tscsiprimarycmds.h"
#include "tscsiblockcmds.h"

#include "mblocktransferprotocol.h"
#include "tblocktransfer.h"

#include "tsbcclientinterface.h"
#include "tspcclientinterface.h"
#include "cmassstoragefsm.h"
#include "cscsiprotocol.h"

#include "usbmshostpanic.h"


/**
Create the CScsiProtocol object.

@param aLun The LUN for the device represented by this object
@param aTransport The transport interface
@param aStatusPollingInterval The polling interval
@return CScsiProtocol* A reference to the
*/
CScsiProtocol* CScsiProtocol::NewL(TLun aLun, MTransport& aTransport)
    {
    CScsiProtocol* r = new (ELeave) CScsiProtocol(aTransport);

    CleanupStack::PushL(r);
    r->ConstructL(aLun);
    CleanupStack::Pop();
    return r;
    }

void CScsiProtocol::ConstructL(TLun aLun)
    {
    // iState = EEntry;
    iFsm = CMassStorageFsm::NewL(*this);

    const TInt blockLength = 0x200;

    iHeadbuf.CreateL(blockLength);
    iTailbuf.CreateL(blockLength);
    }


CScsiProtocol::CScsiProtocol(MTransport& aTransport)
:   iSpcInterface(aTransport),
    iSbcInterface(NULL)
    {
    }


CScsiProtocol::~CScsiProtocol()
    {
    delete iFsm;
    iHeadbuf.Close();
    iTailbuf.Close();
    delete iSbcInterface;
    }


void CScsiProtocol::InitialiseUnitL()
    {
    iState = EDisconnected;

    // A device may take time to mount the media. If the device fails attempt to
    // retry the connection for a number of seconds
    TInt retryCounter = 20;
    do
        {
        retryCounter--;
        TInt err = iFsm->ConnectLogicalUnitL();
        if (err == KErrNotSupported)
            {
            break;
            }
        if (iFsm->IsConnected())
            {
            iState = EConnected;
            break;
            }
        User::After(1000 * 200);    // 200 mS
        }
    while (retryCounter);
    }


void CScsiProtocol::UninitialiseUnitL()
    {
    iFsm->DisconnectLogicalUnitL();
    }

TBool CScsiProtocol::IsConnected()
    {
    return (iState == EConnected)? ETrue : EFalse;
    }

void CScsiProtocol::ReadL(TPos aPos,
                          TDes8& aBuf,
                          TInt aLength)
    {

    if (!iSbcInterface)
        User::Leave(KErrNotSupported);

    if(!IsConnected())
        User::Leave(KErrNotReady);
    iSbcInterface->iBlockTransfer.ReadL(*this, aPos, aLength, aBuf);
    }


void CScsiProtocol::BlockReadL(TPos aPos, TDes8& aCopybuf, TInt aLen)
    {
    if (!iSbcInterface)
        User::Leave(KErrNotSupported);

    __ASSERT_DEBUG(aPos % iSbcInterface->iBlockTransfer.BlockLength() == 0,
                   User::Panic(KUsbMsHostPanicCat, EBlockDevice));

    const TInt blockLen = iSbcInterface->iBlockTransfer.BlockLength();
    TInt len = aLen;

    TInt64 lba = aPos / static_cast<TInt64>(blockLen);

    if (I64HIGH(lba))
        {
        User::LeaveIfError(KErrOverflow);
        }

    TInt err = iSbcInterface->Read10L(I64LOW(lba), aCopybuf, len);
    if (err)
        {
        OstTrace1(TRACE_SHOSTMASSSTORAGE_SCSI, CSCSIPROTOCOL_10,
                  "READ(10) Err=%d", err);
        User::LeaveIfError(DoCheckConditionL());
        }

    // handle residue
    while (len != aLen)
        {
        OstTraceExt2(TRACE_SHOSTMASSSTORAGE_SCSI, CSCSIPROTOCOL_11,
                  "SCSI Read Residue 0x%x bytes read (0x%x)", len, aLen);
        OstTraceExt2(TRACE_SHOSTMASSSTORAGE_SCSI, CSCSIPROTOCOL_12,
                  "Pos=0x%lx Len=0x%x", aPos, aLen);

        // read next block

        // full blocks read in bytes
        TInt bytesRead = len/blockLen * blockLen;
        aPos += bytesRead;
        aLen -= bytesRead;
        len = aLen;

        OstTraceExt3(TRACE_SHOSTMASSSTORAGE_SCSI, CSCSIPROTOCOL_13,
                     "New Pos=0x%lx Len=0x%x (bytes read = %x)", aPos, aLen, bytesRead);

        aCopybuf.SetLength(bytesRead);

        // read rest of the block
        TInt err = iSbcInterface->Read10L(aPos/blockLen, aCopybuf, len);
        if (err)
            {
            User::LeaveIfError(DoCheckConditionL());
            }
        }
    }


void CScsiProtocol::WriteL(TPos aPosition,
                           TDesC8& aBuf,
                           TInt aLength)
    {
    if (!iSbcInterface)
        User::Leave(KErrNotSupported);

    if(!IsConnected())
        User::Leave(KErrNotReady);
    iSbcInterface->iBlockTransfer.WriteL(*this, aPosition, aLength, aBuf);
    }


void CScsiProtocol::BlockWriteL(TPos aPos, TDesC8& aCopybuf, TUint aOffset, TInt aLen)
    {
    if (!iSbcInterface)
        User::Leave(KErrNotSupported);

    __ASSERT_DEBUG(aPos % iSbcInterface->iBlockTransfer.BlockLength() == 0,
                   User::Panic(KUsbMsHostPanicCat, EBlockDevice));
    const TInt blockLen = iSbcInterface->iBlockTransfer.BlockLength();
    TInt len = aLen;
    TInt err = iSbcInterface->Write10L(aPos/blockLen, aCopybuf, aOffset, len);
    if (err)
        {
        User::LeaveIfError(DoCheckConditionL());
        }

    while (len != aLen)
        {
        // handle residue
        OstTraceExt2(TRACE_SHOSTMASSSTORAGE_SCSI, CSCSIPROTOCOL_20,
                     "SCSI Write Residue 0x%x bytes read (0x%x)", len, aLen);
        OstTraceExt2(TRACE_SHOSTMASSSTORAGE_SCSI, CSCSIPROTOCOL_21,
                     "Pos=0x%lx Len=0x%x", aPos, aLen);

        // write next block

        // full blocks written in bytes
        TInt bytesWritten = len/blockLen * blockLen;
        aPos += bytesWritten;
        aLen -= bytesWritten;
        len = aLen;
        OstTraceExt2(TRACE_SHOSTMASSSTORAGE_SCSI, CSCSIPROTOCOL_22,
                     "New Pos=0x%lx Len=0x%x", aPos, aLen);

        TPtrC8 buf = aCopybuf.Mid(bytesWritten);

        // write rest of the block
        TInt err = iSbcInterface->Write10L(aPos/blockLen, buf, aOffset, len);
        if (err)
            {
            User::LeaveIfError(DoCheckConditionL());
            }
        }
    }


void CScsiProtocol::GetCapacityL(TCapsInfo& aCapsInfo)
    {
    if (!IsConnected())
        {
        if (!DoScsiReadyCheckEventL())
            return;
        }

    if (!iSbcInterface)
        {
        aCapsInfo.iMediaType = EMediaCdRom;
        aCapsInfo.iNumberOfBlocks = 0;
        aCapsInfo.iBlockLength = 0;
        aCapsInfo.iWriteProtect = ETrue;
        return;
        }

    aCapsInfo.iMediaType = EMediaHardDisk;

    TLba lastLba;
    TUint32 blockLength;

    // Retry ReadCapacity10L if stalled
    TInt stallCounter = 4;
    TInt err = KErrNone;
    do
        {
        err = iSbcInterface->ReadCapacity10L(lastLba, blockLength);
        } while (err == KErrCommandStalled && stallCounter-- > 0);

    if (err)
        {
        // DoCheckConditionL clears sense error
        // Media not present will return KErrNotReady so leave here
        User::LeaveIfError(DoCheckConditionL());
        }

    // update iWriteProtect
    err = MsModeSense10L();
    if (err)
        {
        if (err == KErrCommandFailed)
            {
            // Clear sense error
            err = DoCheckConditionL();
            // ignore error if unsupported
            if (err != KErrUnknown)
                {
                User::LeaveIfError(err);
                }
            }

        err = MsModeSense6L();
        if (err == KErrCommandFailed)
            {
            // Clear sense error
            err = DoCheckConditionL();
            // ignore error if unsupported
            if (err != KErrUnknown)
                {
                User::LeaveIfError(err);
                }
            }
        }

    aCapsInfo.iNumberOfBlocks = lastLba + 1;
    aCapsInfo.iBlockLength = blockLength;
    aCapsInfo.iWriteProtect = iWriteProtect;

    OstTraceExt3(TRACE_SHOSTMASSSTORAGE_SCSI, CSCSIPROTOCOL_30,
                 "numBlock=x%x blockLength=x%x wp=%d",
                 (TUint32)(lastLba + 1), blockLength, (TInt32)iWriteProtect);
    }


/**
Perform SCSI INQUIRY command. The function leaves if the device response is not
compliant with the protocol standard.

@return TInt KErrNone if successful otherwise KErrCommandFailed to indicate a
device status error
*/
TInt CScsiProtocol::MsInquiryL()
    {
    ResetSbc();

   /**
    INQUIRY
   */

   /**
    SPC states
    - the INQUIRY data should be returned even though the device server is not
      ready for other commands.

    - If the standard INQUIRY data changes for any reason, the device server
      shall generate a unit attention condition
   */
    TPeripheralInfo info;
    TInt err = iSpcInterface.InquiryL(info);
    if (err)
        {
        // KErrCommandFailed
        return err;
        }

    // print reponse
    OstTrace1(TRACE_SHOSTMASSSTORAGE_DEVICEREPORT, CSCSIPROTOCOL_40,
              "RMB = %d", info.iRemovable);
    OstTraceExt2(TRACE_SHOSTMASSSTORAGE_DEVICEREPORT, CSCSIPROTOCOL_41,
              "PERIPHERAL DEVICE TYPE = %d PQ = %d",
              info.iPeripheralDeviceType,
              info.iPeripheralQualifier);
    OstTrace1(TRACE_SHOSTMASSSTORAGE_DEVICEREPORT, CSCSIPROTOCOL_42,
              "VERSION = %d", info.iVersion);
    OstTrace1(TRACE_SHOSTMASSSTORAGE_DEVICEREPORT, CSCSIPROTOCOL_43,
              "RESPONSE DATA FORMAT = %d",
              info.iResponseDataFormat);

    OstTraceData(TRACE_SHOSTMASSSTORAGE_DEVICEREPORT, CSCSIPROTOCOL_440,
              "VENDOR ID %s",
              info.iIdentification.iVendorId.Ptr(), info.iIdentification.iVendorId.Size() * 2);
    OstTraceData(TRACE_SHOSTMASSSTORAGE_DEVICEREPORT, CSCSIPROTOCOL_441,
              "PRODUCT ID %s",
              info.iIdentification.iProductId.Ptr(), info.iIdentification.iProductId.Size()  * 2);
    OstTraceData(TRACE_SHOSTMASSSTORAGE_DEVICEREPORT, CSCSIPROTOCOL_442,
              "REV %s",
              info.iIdentification.iProductRev.Ptr(), info.iIdentification.iProductRev.Size() * 2);

    if (info.iPeripheralQualifier != 0 && info.iPeripheralQualifier != 1)
        {
        OstTrace0(TRACE_SHOSTMASSSTORAGE_HOST, CSCSIPROTOCOL_45,
                  "Peripheral Qualifier[Unknown device type]");
        err = KErrUnknown;
        }
    else if (info.iPeripheralDeviceType == 0)
        {
        // SCSI SBC Direct access device
        iRemovableMedia = info.iRemovable;

        // SCSI Block device
        iSbcInterface = new (ELeave) TSbcClientInterface(iSpcInterface.Transport());
        iSbcInterface->InitBuffers(&iHeadbuf, &iTailbuf);
        err = KErrNone;
        }
    else if (info.iPeripheralDeviceType == 5)
        {
        // SCSI MMC-2 CD-ROM device
        OstTrace0(TRACE_SHOSTMASSSTORAGE_HOST, CSCSIPROTOCOL_46,
                  "Peripheral Device Type[CD-ROM]");
        iRemovableMedia = info.iRemovable;

        // MMC-2 is not supported. A SCSI interface call will return
        // KErrNotSupported. If SCSI support is extended in future then
        // TSbcInterface class should be replaced with a proper interface class.
        iSbcInterface = NULL;
        err = KErrNone;
        }
    else
        {
        OstTrace0(TRACE_SHOSTMASSSTORAGE_HOST, CSCSIPROTOCOL_47,
                  "Peripheral Device Type[Unsupported device type]");
        err = KErrUnknown;
        }

    return err;
    }


/**
Perform SCSI TEST UNIT READY command. The function leaves if the device response
is not compliant with the protocol standard.

@return TInt KErrNone if successful or otherwise KErrCommandFailed to indicate a
device status error
*/
TInt CScsiProtocol::MsTestUnitReadyL()
    {
    /* TestUnitReady */
    return iSpcInterface.TestUnitReadyL();
    }


/**
Perform SCSI READ CAPACITY (10) command. The function leaves if the device
response is not compliant with the protocol standard.

Before a block device can be read or written the media's capacity (LAST LBA and
BLOCK SIZE) must be obtained. This function is used to initialise TBlockTransfer
with the capacity parameters via TSbcInterface::ReadCapcaityL().

@return TInt KErrNone if successful, KErrCommandFailed to indicate a
device status error, KErrCommandStalled to indicate a device stall
*/
TInt CScsiProtocol::MsReadCapacityL()
    {
    if (!iSbcInterface)
        {
        User::Leave(KErrNotSupported);
        }

    // READ CAPACITY
    TUint32 blockSize;
    TUint32 lastLba;
    TInt err = iSbcInterface->ReadCapacity10L(lastLba, blockSize);

    OstTraceExt2(TRACE_SHOSTMASSSTORAGE_DEVICEREPORT, CSCSIPROTOCOL_50,
                 "CAPACITY: Block Size=0x%x Last LBA=0x%x", blockSize, lastLba);
    return err;
    }


/**
Perform MODE SENSE (10) command. The function leaves if the device response is
not compliant with the protocol standard.

@return TInt KErrNone if successful, KErrCommandFailed to indicate a
device status error, KErrCommandStalled to indicate a device stall
*/
TInt CScsiProtocol::MsModeSense10L()
    {
    if (!iSbcInterface)
        User::Leave(KErrNotSupported);

    TBool writeProtected;
    TInt err = iSbcInterface->ModeSense10L(TSbcClientInterface::EReturnAllModePages, writeProtected);

    if (!err)
        {
        iWriteProtect = writeProtected;
        }
    return err;
    }


/**
Perform SCSI MODE SENSE (6) command. The function leaves if the device response
is not compliant with the protocol standard.

@return TInt KErrNone if successful, KErrCommandFailed to indicate a
device status error, KErrCommandStalled to indicate a device stall
*/
TInt CScsiProtocol::MsModeSense6L()
    {
    if (!iSbcInterface)
        User::Leave(KErrNotSupported);

    TBool writeProtected;
    TInt err = iSbcInterface->ModeSense6L(TSbcClientInterface::EReturnAllModePages, writeProtected);

    if (!err)
        {
        iWriteProtect = writeProtected;
        }
    return err;
    }


/**
Perform SCSI START STOP UNIT command. The function leaves if the device response
is not compliant with the protocol standard.

@return TInt KErrNone if successful otherwise KErrCommandFailed to indicate a
device status error
*/
TInt CScsiProtocol::MsStartStopUnitL(TBool aStart)
    {
    if (!iSbcInterface)
        User::Leave(KErrNotSupported);

    return iSbcInterface->StartStopUnitL(aStart);
    }


/**
Perform SCSI PREVENT ALLOW MEDIA REMOVAL command. The function leaves if the
device response is not compliant with the protocol standard.

@return TInt KErrNone if successful otherwise KErrCommandFailed to indicate a
device status error
*/
TInt CScsiProtocol::MsPreventAllowMediaRemovalL(TBool aPrevent)
    {
    return iSpcInterface.PreventAllowMediumRemovalL(aPrevent);
    }


TInt CScsiProtocol::DoCheckConditionL()
    {
    User::LeaveIfError(MsRequestSenseL());

    TInt err;

    // Check if init is needed
    if (iSenseInfo.iSenseCode == TSenseInfo::ENotReady &&
        iSenseInfo.iAdditional == TSenseInfo::EAscLogicalUnitNotReady &&
        iSenseInfo.iQualifier == TSenseInfo::EAscqInitializingCommandRequired)
        {
        if (iSbcInterface)
            {
            // start unit
            err = iSbcInterface->StartStopUnitL(ETrue);
            if (err)
                {
                User::LeaveIfError(MsRequestSenseL());
                }
            }
        }

    err = GetSystemWideSenseError(iSenseInfo);

    TScsiState nextState = iState;
    if (err == KErrDisconnected)
        {
        nextState = EDisconnected;
        }
    else if (err == KErrNotReady)
        {
        nextState = EMediaNotPresent;
        }
    else
        {
        // no state change;
        }

    if (nextState != iState)
        {
        iMediaChangeNotifier.DoNotifyL();
        iState = nextState;
        }

    return err;
    }


/**
Map SCSI sense error to a system wide error code
KErrNotReady could happen due to any of the following reasons:
    1. Lun is in the process of becoming ready
    2. Initialising command is required
    3. Lun is not ready to process the command - meaning it is still handling
    the previous command

KErrUnknown could happen due to any of the following reasons:
    1. Mass storage device does not respond to the selected logical unit, other
    than the locial unit not ready scenario 2. The command sent was not
    recognized or contains a invalid code 3. Invialid field in the command block
    4. The requested logical unit is not supported
    5. The mass storage device cnosists of insufficient resource
    6. Hardware error
    7. Blank check
    8. Vendor specific error
    9. Any illegal request (we assume the commands sent by MSC should be
    supported by the device, if not then the illegal request is said to be of
    unknown system wide error for Symbian. 10. Miscompare - we do not support
    the compare operation/command so this error should not happen

KErrAccessDenied could happen due to any of the following reasons:
    1. Data protection error happened
    2. Media was write protected
    3. Media was not present

KErrOverflow could happen due to any of the following reasons:
    1. Data over flow occured
    2. The requested LBA is out of range

KErrAbort could happen due to any of the following reasons:
    1. A copy operation is aborted.
    2. The command is aborted

KErrCorrupt could happen due to any of the following reasons:
    1. The underlying media was having errors

KErrDisconnected could happen due to any of the following reasons:
    1. The media was changed/removed - While this error is happening the file
    extension will be notified setting the iChanged flag
*/
TInt CScsiProtocol::GetSystemWideSenseError(const TSenseInfo& aSenseInfo)
    {
    TInt ret = KErrNone;
    TInt additionalError = KErrNone;

    switch(aSenseInfo.iSenseCode)
        {
        case TSenseInfo::ENoSense:
        case TSenseInfo::ERecoveredError:
            ret = KErrNone;
            break;
        case TSenseInfo::ENotReady:
            ret = KErrNotReady;
            additionalError = ProcessAsCodes(aSenseInfo);
            if (additionalError != KErrNone)
                {
                ret = additionalError;
                }
            break;
        case TSenseInfo::EMediumError:
            ret = KErrCorrupt;
            additionalError = ProcessAsCodes(aSenseInfo);
            if (additionalError != KErrNone)
                {
                ret = additionalError;
                }
            break;
        case TSenseInfo::EUnitAttention:
            ret = KErrDisconnected;
            break;
        case TSenseInfo::EDataProtection:
            ret = KErrAccessDenied;
            break;
        case TSenseInfo::EIllegalRequest:
        case TSenseInfo::EHardwareError:
        case TSenseInfo::EBlankCheck:
        case TSenseInfo::EVendorSpecific:
        case TSenseInfo::EMisCompare:
            ret = KErrUnknown;
            break;
        case TSenseInfo::ECopyAborted:
        case TSenseInfo::EAbortedCommand:
            ret = KErrAbort;
            break;
        case TSenseInfo::EDataOverflow:
            ret = KErrOverflow;
            break;
        default:
            ret = KErrUnknown;
            break;
        }

    return ret;
    }


TInt CScsiProtocol::ProcessAsCodes(const TSenseInfo& aSenseInfo)
    {
    TInt ret = KErrNone;

    switch(aSenseInfo.iAdditional)
        {
        case TSenseInfo::EAscLogicalUnitNotReady:
        case TSenseInfo::EMediaNotPresent:
            ret = KErrNotReady;
            break;

        case TSenseInfo::ELbaOutOfRange:
            ret = KErrOverflow;
            break;

        case TSenseInfo::EWriteProtected:
            ret = KErrAccessDenied;
            break;

        case TSenseInfo::ENotReadyToReadyChange:
            ret = KErrNone;
            break;

        case TSenseInfo::EAscLogicalUnitDoesNotRespondToSelection:
        case TSenseInfo::EInvalidCmdCode:
        case TSenseInfo::EInvalidFieldInCdb:
        case TSenseInfo::ELuNotSupported:
        case TSenseInfo::EInsufficientRes:
            ret = KErrUnknown;
            break;
        default:
            ret = KErrNone;
            break;
        }
    return ret;
    }


/**
Perform SCSI REQUEST SENSE command.  The function leaves if the device response
is not compliant with the protocol standard.

@return TInt KErrNone if successful otherwise KErrCommandFailed to indicate a
device status error
*/
TInt CScsiProtocol::MsRequestSenseL()
    {
    return iSpcInterface.RequestSenseL(iSenseInfo) ? KErrCommandFailed : KErrNone;
    }


void CScsiProtocol::CreateSbcInterfaceL(TUint32 aBlockLen, TUint32 aLastLba)
    {
    // SCSI Block device
    ASSERT(iSbcInterface == NULL);
    iSbcInterface = new (ELeave) TSbcClientInterface(iSpcInterface.Transport());
    iSbcInterface->InitBuffers(&iHeadbuf, &iTailbuf);
    iSbcInterface->SetCapacityL(aBlockLen, aLastLba);
    }


void CScsiProtocol::ResetSbc()
    {
    if (iSbcInterface)
        {
        delete iSbcInterface;
        iSbcInterface = NULL;
        }
    }


void CScsiProtocol::NotifyChange(const RMessage2& aMessage)
    {
    iMediaChangeNotifier.Register(aMessage);
    }


void CScsiProtocol::ForceCompleteNotifyChangeL()
    {
    iMediaChangeNotifier.DoNotifyL();
    }


void CScsiProtocol::CancelChangeNotifierL()
    {
    iMediaChangeNotifier.DoCancelL();
    }


void CScsiProtocol::SuspendL()
    {
    if (iFsm->StartStopUnitRequired())
        {
        iSbcInterface->StartStopUnitL(EFalse);
        }
    }

void CScsiProtocol::ResumeL()
    {
    if (iFsm->StartStopUnitRequired())
        {
        iSbcInterface->StartStopUnitL(ETrue);
        }
    }


TBool CScsiProtocol::DoScsiReadyCheckEventL()
    {
    TInt err = KErrNone;

    if(iRemovableMedia || iState != EConnected)
        {
        iFsm->SetStatusCheck();
        TRAP(err, iFsm->ConnectLogicalUnitL());
        iFsm->ClearStatusCheck();

        User::LeaveIfError(err);
        err = iFsm->IsConnected() ? KErrNone : KErrNotReady;
        }

    if (iState == EConnected)
        {
        if (err != KErrNone)
            {
            iState = EDisconnected;
            OstTrace0(TRACE_SHOSTMASSSTORAGE_SCSI, CSCSIPROTOCOL_60,
                      "** Disconnected Notification **");
            iMediaChangeNotifier.DoNotifyL();
            }
        }
    else
        {
        if (err == KErrNone)
            {
            iState = EConnected;
            OstTrace0(TRACE_SHOSTMASSSTORAGE_SCSI, CSCSIPROTOCOL_61,
                      "** Connected Notification **");
            iMediaChangeNotifier.DoNotifyL();
            }
        }
    return err = KErrNone ? ETrue : EFalse;
    }


RMediaChangeNotifier::RMediaChangeNotifier()
:   iRegistered(EFalse)
    {
    }


RMediaChangeNotifier::~RMediaChangeNotifier()
    {
    if (iRegistered)
        iNotifier.Complete(KErrDisconnected);
    }

/**
Initialise notifier to enable media change notfications.

@param aMessage The message to commplete the notification
*/
void RMediaChangeNotifier::Register(const RMessage2& aMessage)
    {
    iRegistered = ETrue;
    iNotifier = aMessage;
    }


void RMediaChangeNotifier::DoNotifyL()
    {
    CompleteNotifierL(KErrNone);
    }

void RMediaChangeNotifier::DoCancelL()
    {
    CompleteNotifierL(KErrCancel);
    }

void RMediaChangeNotifier::CompleteNotifierL(TInt aReason)
    {
    if (iRegistered)
        {
        TBool mediaChanged = ETrue;
        TPtrC8 pStatus((TUint8*)&mediaChanged,sizeof(TBool));
        iNotifier.WriteL(0,pStatus);
        iNotifier.Complete(aReason);
        iRegistered = EFalse;
        }
    }
