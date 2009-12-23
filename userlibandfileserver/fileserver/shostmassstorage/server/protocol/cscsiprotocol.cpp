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

#include "debug.h"
#include "msdebug.h"
#include "msctypes.h"
#include "shared.h"
#include "msgservice.h"

#include "mscutils.h"

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
	__MSFNSLOG
	CScsiProtocol* r = new (ELeave) CScsiProtocol(aTransport);

	CleanupStack::PushL(r);
	r->ConstructL(aLun);
	CleanupStack::Pop();
	return r;
    }

void CScsiProtocol::ConstructL(TLun aLun)
    {
	__MSFNLOG
    iFsm = CMassStorageFsm::NewL(*this);
	iState = EDisconnected;

    const TInt blockLength = 0x200;

    iHeadbuf.CreateL(blockLength);
    iTailbuf.CreateL(blockLength);
    }


CScsiProtocol::CScsiProtocol(MTransport& aTransport)
:   iSpcInterface(aTransport),
    iSbcInterface(NULL)
    {
	__MSFNLOG
    }


CScsiProtocol::~CScsiProtocol()
    {
	__MSFNLOG
    delete iFsm;
    iHeadbuf.Close();
    iTailbuf.Close();
    delete iSbcInterface;
    }


void CScsiProtocol::InitialiseUnitL()
    {
	__MSFNLOG

	// A device may take time to mount the media. If the device fails attempt to
	// retry the connection for a number of seconds
    TInt retryCounter = 20;
    do
        {
        retryCounter--;
        iFsm->ConnectLogicalUnitL();
        iState = iFsm->IsConnected() ? EConnected: EDisconnected;

        if (iState == EConnected)
            {
            break;
            }
        User::After(1000 * 200);    // 200 mS
        }
    while (retryCounter);
	}


void CScsiProtocol::UninitialiseUnitL()
    {
	__MSFNLOG
    iFsm->DisconnectLogicalUnitL();
    }

TBool CScsiProtocol::IsConnected()
    {
	__MSFNLOG
	return (iState == EConnected)? ETrue : EFalse;
    }

void CScsiProtocol::ReadL(TPos aPos,
                          TDes8& aBuf,
                          TInt aLength)
    {
	__MSFNLOG
    if(!IsConnected())
		User::Leave(KErrNotReady);
    iSbcInterface->iBlockTransfer.ReadL(*this, aPos, aLength, aBuf);
    }


void CScsiProtocol::BlockReadL(TPos aPos, TDes8& aCopybuf, TInt aLen)
    {
	__MSFNLOG
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
        __SCSIPRINT1(_L("READ(10) Err=%d"), err);
        DoCheckConditionL();
        User::LeaveIfError(KErrAbort);
        }

    // handle residue
    while (len != aLen)
        {
        __SCSIPRINT2(_L("SCSI Read Residue 0x%x bytes read (0x%x)"), len, aLen);
        __SCSIPRINT2(_L("Pos=0x%lx Len=0x%x"), aPos, aLen);

        // read next block

        // full blocks read in bytes
        TInt bytesRead = len/blockLen * blockLen;
        aPos += bytesRead;
        aLen -= bytesRead;
        len = aLen;

        __SCSIPRINT3(_L("New Pos=0x%lx Len=0x%x (bytes read = %x)"),
                     aPos, aLen, bytesRead);

        aCopybuf.SetLength(bytesRead);

        // read rest of the block
        TInt err = iSbcInterface->Read10L(aPos/blockLen, aCopybuf, len);
        if (err)
            {
            DoCheckConditionL();
            User::LeaveIfError(KErrAbort);
            }
        }
    }


void CScsiProtocol::WriteL(TPos aPosition,
                           TDesC8& aBuf,
                           TInt aLength)
    {
	__MSFNLOG
    if(!IsConnected())
		User::Leave(KErrNotReady);
    iSbcInterface->iBlockTransfer.WriteL(*this, aPosition, aLength, aBuf);
    }


void CScsiProtocol::BlockWriteL(TPos aPos, TDesC8& aCopybuf, TUint aOffset, TInt aLen)
    {
	__MSFNLOG
	__ASSERT_DEBUG(aPos % iSbcInterface->iBlockTransfer.BlockLength() == 0,
                   User::Panic(KUsbMsHostPanicCat, EBlockDevice));
    const TInt blockLen = iSbcInterface->iBlockTransfer.BlockLength();
    TInt len = aLen;
	TInt err = iSbcInterface->Write10L(aPos/blockLen, aCopybuf, aOffset, len);
    if (err)
        {
        DoCheckConditionL();
        User::LeaveIfError(KErrAbort);
        }

    while (len != aLen)
        {
        // handle residue
        __SCSIPRINT2(_L("SCSI Write Residue 0x%x bytes read (0x%x)"), len, aLen);
        __SCSIPRINT2(_L("Pos=0x%lx Len=0x%x"), aPos, aLen);

        // write next block

        // full blocks written in bytes
        TInt bytesWritten = len/blockLen * blockLen;
        aPos += bytesWritten;
        aLen -= bytesWritten;
        len = aLen;
        __SCSIPRINT2(_L("New Pos=0x%lx Len=0x%x"), aPos, aLen);

        TPtrC8 buf = aCopybuf.Mid(bytesWritten);

        // write rest of the block
        TInt err = iSbcInterface->Write10L(aPos/blockLen, buf, aOffset, len);
        if (err)
            {
            DoCheckConditionL();
            User::LeaveIfError(KErrAbort);
            }
        }
    }


void CScsiProtocol::GetCapacityL(TCapsInfo& aCapsInfo)
    {
	__MSFNLOG
    if (!IsConnected())
        {
        DoScsiReadyCheckEventL();
        }

	TLba lastLba;
	TUint32 blockLength;

	TInt err = iSbcInterface->ReadCapacity10L(lastLba, blockLength);
    if (err)
        {
        if (err == KErrCommandFailed)
            {
            // Clear sense error
            DoCheckConditionL();
            }
        User::LeaveIfError(KErrAbort);
        }

    // update iWriteProtect
    err = MsModeSense10L();
    if (err)
        {
        if (err == KErrCommandFailed)
            {
            // Clear sense error
            DoCheckConditionL();
            }

        err = MsModeSense6L();
        if (err == KErrCommandFailed)
            {
            // Clear sense error
            DoCheckConditionL();
            }           
        }

    aCapsInfo.iNumberOfBlocks = lastLba + 1;
    aCapsInfo.iBlockLength = blockLength;
    aCapsInfo.iWriteProtect = iWriteProtect;

	__SCSIPRINT3(_L("numBlock = x%x , blockLength = %x wp = %d"),
                 lastLba + 1, blockLength, iWriteProtect);
    }


/**
Perform SCSI INQUIRY command. The function leaves if the device response is not
compliant with the protocol standard.

@return TInt KErrNone if successful otherwise KErrCommandFailed to indicate a
device status error
*/
TInt CScsiProtocol::MsInquiryL()
    {
	__MSFNLOG
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
    __TESTREPORT1(_L("RMB = %d"), info.iRemovable);
    __TESTREPORT2(_L("PERIPHERAL DEVICE TYPE = %d PQ = %d"),
                 info.iPeripheralDeviceType,
                 info.iPeripheralQualifier);
    __TESTREPORT1(_L("VERSION = %d"), info.iVersion);
    __TESTREPORT1(_L("RESPONSE DATA FORMAT = %d"), info.iResponseDataFormat);
    __TESTREPORT3(_L("VENDOR ID %S PRODUCT ID %S REV %S"),
                 &info.iIdentification.iVendorId,
                 &info.iIdentification.iProductId,
                 &info.iIdentification.iProductRev);

    if (info.iPeripheralQualifier != 0 && info.iPeripheralQualifier != 1)
        {
        __HOSTPRINT(_L("Peripheral Qualifier[Unknown device type]\n"))
        return KErrNotSupported;
        }

    if (info.iPeripheralDeviceType != 0)
        {
        __HOSTPRINT(_L("Peripheral Device Type[Unsupported device type]\n"))
        return KErrNotSupported;
        }

    iRemovableMedia = info.iRemovable;

    // SCSI Block device
    iSbcInterface = new (ELeave) TSbcClientInterface(iSpcInterface.Transport());
    iSbcInterface->InitBuffers(&iHeadbuf, &iTailbuf);

    return KErrNone;
    }


/**
Perform SCSI TEST UNIT READY command. The function leaves if the device response
is not compliant with the protocol standard.

@return TInt KErrNone if successful or otherwise KErrCommandFailed to indicate a
device status error
*/
TInt CScsiProtocol::MsTestUnitReadyL()
    {
	__MSFNLOG
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
	__MSFNLOG
    // READ CAPACITY
    TUint32 blockSize;
    TUint32 lastLba;
    TInt err = iSbcInterface->ReadCapacity10L(lastLba, blockSize);

    __TESTREPORT2(_L("CAPACITY: Block Size=0x%x Last LBA=0x%x"), blockSize, lastLba);
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
	__MSFNLOG
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
	__MSFNLOG
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
	__MSFNLOG
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
	__MSFNLOG
    return iSpcInterface.PreventAllowMediumRemovalL(aPrevent);
    }


void CScsiProtocol::DoCheckConditionL()
    {
	__MSFNLOG
    User::LeaveIfError(MsRequestSenseL());

    // Check if init is needed
    if (iSenseInfo.iSenseCode == TSenseInfo::ENotReady &&
        iSenseInfo.iAdditional == TSenseInfo::EAscLogicalUnitNotReady &&
        iSenseInfo.iQualifier == TSenseInfo::EAscqInitializingCommandRequired)
        {
        // start unit
        TInt err = iSbcInterface->StartStopUnitL(ETrue);

        if (err)
            {
            User::LeaveIfError(MsRequestSenseL());
            }

        }

    TInt r = GetSystemWideSenseError(iSenseInfo);

    if (((r == KErrNotReady) && (iState == EConnected)) ||
        r == KErrDisconnected)
	    {
        CompleteNotifyChangeL();
        }
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
	__MSFNLOG
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
	__MSFNLOG
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
	__MSFNLOG
    return iSpcInterface.RequestSenseL(iSenseInfo) ? KErrCommandFailed : KErrNone;
	}


void CScsiProtocol::CreateSbcInterfaceL(TUint32 aBlockLen, TUint32 aLastLba)
    {
	__MSFNLOG
    // SCSI Block device
    ASSERT(iSbcInterface == NULL);
    iSbcInterface = new (ELeave) TSbcClientInterface(iSpcInterface.Transport());
    iSbcInterface->InitBuffers(&iHeadbuf, &iTailbuf);
    iSbcInterface->SetCapacityL(aBlockLen, aLastLba);
    }


void CScsiProtocol::ResetSbc()
    {
	__MSFNLOG
    if (iSbcInterface)
        {
        delete iSbcInterface;
        iSbcInterface = NULL;
        }
    }


void CScsiProtocol::NotifyChange(const RMessage2& aMessage)
	{
    __MSFNLOG
    iMediaChangeNotifier.Register(aMessage);
	}


void CScsiProtocol::ForceCompleteNotifyChangeL()
	{
    __MSFNLOG
    iMediaChangeNotifier.DoNotifyL();
	}


void CScsiProtocol::CancelChangeNotifierL()
	{
    __MSFNLOG
    iMediaChangeNotifier.DoCancelL();
	}


void CScsiProtocol::SuspendL()
	{
    __MSFNLOG
    if (iFsm->StartStopUnitRequired())
        {
        iSbcInterface->StartStopUnitL(EFalse);
        }
	}

void CScsiProtocol::ResumeL()
	{
    __MSFNLOG
    if (iFsm->StartStopUnitRequired())
        {
        iSbcInterface->StartStopUnitL(ETrue);
        }
	}


void CScsiProtocol::DoScsiReadyCheckEventL()
	{
    __MSFNLOG
	TInt err = KErrNone;

	if(iFsm->IsRemovableMedia() || iState == EDisconnected)
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
            __SCSIPRINT(_L("** Disconnected Notification **"));
            iMediaChangeNotifier.DoNotifyL();
			}
        }
	else
        {
		if (err == KErrNone)
			{
			iState = EConnected;
            __SCSIPRINT(_L("** Connected Notification **"));
            iMediaChangeNotifier.DoNotifyL();
			}
        }
	}

void CScsiProtocol::CompleteNotifyChangeL()
	{
    __MSFNLOG
    if (!iFsm->IsStatusCheck())
		{
		if (iState == EConnected)
			{
			iState = EDisconnected;
            iMediaChangeNotifier.DoNotifyL();
			}
		}
	}

RMediaChangeNotifier::RMediaChangeNotifier()
:   iRegistered(EFalse)
    {
    __MSFNSLOG
    }


RMediaChangeNotifier::~RMediaChangeNotifier()
    {
    __MSFNSLOG
    if (iRegistered)
        iNotifier.Complete(KErrDisconnected);
    }

/**
Initialise notifier to enable media change notfications.

@param aMessage The message to commplete the notification
*/
void RMediaChangeNotifier::Register(const RMessage2& aMessage)
    {
    __MSFNLOG
	iRegistered = ETrue;
	iNotifier = aMessage;
    }


void RMediaChangeNotifier::DoNotifyL()
    {
	__MSFNLOG
	CompleteNotifierL(KErrNone);
    }

void RMediaChangeNotifier::DoCancelL()
    {
	__MSFNLOG
	CompleteNotifierL(KErrCancel);
    }

void RMediaChangeNotifier::CompleteNotifierL(TInt aReason)
	{
    __MSFNLOG
	if (iRegistered)
        {
		TBool mediaChanged = ETrue;
		TPtrC8 pStatus((TUint8*)&mediaChanged,sizeof(TBool));
		iNotifier.WriteL(0,pStatus);
		iNotifier.Complete(aReason);
		iRegistered = EFalse;
        }
	}
