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
//

#include <e32base.h>
#include <f32file.h>
#include <e32property.h>

#include "mstypes.h"
#include "msctypes.h"
#include "usbmsshared.h"

#include "drivemanager.h"
#include "drivepublisher.h"
#include "tscsiserverreq.h"
#include "tscsiservercmds.h"
#include "mserverprotocol.h"
#include "mdevicetransport.h"

#include "testman.h"
#include "cscsiserverprotocol.h"
#include "debug.h"
#include "msdebug.h"


TMediaWriteMan::TMediaWriteMan()
:   iActive(EFalse),
    iOffset(0),
	iMediaWriteSize(KDefaultMediaWriteSize)
    {
    }

void TMediaWriteMan::ReportHighSpeedDevice()
	{
    __MSFNLOG
	iMediaWriteSize = KHsMediaWriteSize;
	__PRINT1(_L("HS Device reported: SCSI will use %d bytes disk write size"), iMediaWriteSize);
	}


TInt64 TMediaWriteMan::Start(TUint32 aLba, TUint32 aLength, TUint32 aBlockSize)
    {
    iActive = ETrue;
    iOffset = static_cast<TInt64>(aLba) * aBlockSize;
    iBytesRemain = aLength * aBlockSize;

	TInt64 theEnd = iOffset + iBytesRemain;
    return theEnd;
    }

TUint32 TMediaWriteMan::NextPacket()
    {
    iActive = ETrue;
    return (iBytesRemain < iMediaWriteSize) ? iBytesRemain : iMediaWriteSize;
    }


void TMediaWriteMan::Reset()
    {
    iActive = EFalse;
    iOffset = 0;
    }

void TMediaWriteMan::SetOffset(const TInt64& aOffset, TUint aLength)
    {
    iOffset = aOffset + aLength;
    iBytesRemain -= aLength;
    }

TUint32 TMediaWriteMan::GetPacketLength() const
    {
    // KMaxBufSize or the MediaWriteSize, whichever is smallest.
	TUint32 thisLength = (iBytesRemain > KMaxBufSize) ? KMaxBufSize : iBytesRemain;
	thisLength = (thisLength > iMediaWriteSize) ? iMediaWriteSize : thisLength;
    return thisLength;
    }


/**
Creates the CScsiProtocol object.  Called during controller initialisation.

@param aDriveManager reference to the drive manager object
*/
CScsiServerProtocol* CScsiServerProtocol::NewL(CDriveManager& aDriveManager)
	{
    __MSFNSLOG
	CScsiServerProtocol* self = new (ELeave) CScsiServerProtocol(aDriveManager);
	CleanupStack::PushL(self);
	self->ConstructL();
	CleanupStack::Pop();
	return self;
	}

#ifdef MSDC_TESTMODE
CScsiServerProtocol* CScsiServerProtocol::NewL(CDriveManager& aDriveManager, TTestParser* aTestParser)
	{
    __MSFNSLOG
	CScsiServerProtocol* self = new (ELeave) CScsiServerProtocol(aDriveManager, aTestParser);
	CleanupStack::PushL(self);
	self->ConstructL();
	CleanupStack::Pop();
	return self;
	}
#endif


/**
c'tor

@param aDriveManager reference to the drive manager object
*/
CScsiServerProtocol::CScsiServerProtocol(CDriveManager& aDriveManager)
:   iDriveManager(aDriveManager)
	{
    __MSFNLOG

	iWriteTransferPublisher = CUsbWriteTransferPublisher::NewL(iBytesWritten);
	iReadTransferPublisher = CUsbReadTransferPublisher::NewL(iBytesRead);

	for (TUint i = 0; i < KUsbMsMaxDrives; i++)
		{
		iBytesRead[i] = 0;
		iBytesWritten[i] = 0;
		}
	}

#ifdef MSDC_TESTMODE
CScsiServerProtocol::CScsiServerProtocol(CDriveManager& aDriveManager, TTestParser* aTestParser)
:   iDriveManager(aDriveManager),
    iTestParser(aTestParser)
	{
    __MSFNLOG

	iWriteTransferPublisher = CUsbWriteTransferPublisher::NewL(iBytesWritten);
	iReadTransferPublisher = CUsbReadTransferPublisher::NewL(iBytesRead);

	for (TUint i = 0; i < KUsbMsMaxDrives; i++)
		{
		iBytesRead[i] = 0;
		iBytesWritten[i] = 0;
		}
	}
#endif


CScsiServerProtocol::~CScsiServerProtocol()
	{
    __MSFNLOG
    iDataBuf.Close();
	delete iWriteTransferPublisher;
	delete iReadTransferPublisher;
	}


void CScsiServerProtocol::ConstructL()
	{
    __MSFNLOG
	}


/**
Associates the transport with the protocol. Called during initialisation of the controller.

@param aTransport pointer to the transport object
*/
void CScsiServerProtocol::RegisterTransport(MDeviceTransport* aTransport)
	{
    __MSFNLOG
	iTransport = aTransport;
	}


/**
Called by the Transport when it detects that the USB device is either running
at High Speed or is at least capable of HS operation. The Protocol can use this
information (for instance) to select the optimal write block size to use.

This function is preferably called before actual MS data transfer operation
starts, and usually only once.

*/
void CScsiServerProtocol::ReportHighSpeedDevice()
	{
    __MSFNLOG
    iMediaWriteMan.ReportHighSpeedDevice();
	}


void CScsiServerProtocol::SetParameters(const TMassStorageConfig& aConfig)
	{
    __MSFNLOG
	iConfig = aConfig;
	}


/**
Called by the transport layer when a packet is available for decoding.
If an error occurs, the sense code is updated and EFalse is returned.

@param aData

@return  ETrue if command was decoded and executed successfully
*/
TBool CScsiServerProtocol::DecodePacket(TPtrC8& aData, TUint8 aLun)
	{
    __MSFNLOG
    TScsiServerReq* cdb = NULL;
    TRAPD(err, cdb = cdb->CreateL(static_cast<TScsiServerReq::TOperationCode>(aData[0]), aData));

    TBool decodeGood = EFalse;
    if (err == KErrNotSupported)
    	iSenseInfo.SetSense(TSenseInfo::EIllegalRequest, TSenseInfo::EInvalidCmdCode);
	else if (err != KErrNone)
		iSenseInfo.SetSense(TSenseInfo::EAbortedCommand, TSenseInfo::EInsufficientRes);
    else if (cdb->iNaca) // Check the CONTROL byte
		iSenseInfo.SetSense(TSenseInfo::EIllegalRequest, TSenseInfo::EInvalidFieldInCdb);
    else if (cdb->iLink)
        iSenseInfo.SetSense(TSenseInfo::EIllegalRequest, TSenseInfo::EInvalidFieldInCdb);
    else
        {
        TScsiServerReq::TOperationCode operationCode = cdb->iOperationCode;
        if (aLun > iDriveManager.MaxLun())
            {
            __PRINT(_L("No drive available\n"));
            iSenseInfo.SetSense(TSenseInfo::EIllegalRequest, TSenseInfo::ELuNotSupported);
            }
        else
            {
            iLun = aLun;
            if (operationCode != TScsiServerReq::ERequestSense)
                {
                iSenseInfo.SetSense(TSenseInfo::ENoSense);
                }

            switch (operationCode)
                {
            case TScsiServerReq::ETestUnitReady:
                HandleUnitReady();
                break;

            case TScsiServerReq::ERequestSense:
                HandleRequestSense(*cdb);
                break;

            case TScsiServerReq::EInquiry:
                HandleInquiry(*cdb);
                break;

            case TScsiServerReq::EModeSense6:
                HandleModeSense6(*cdb);
                break;

            case TScsiServerReq::EStartStopUnit:
                HandleStartStopUnit(*cdb);
                break;

            case TScsiServerReq::EPreventMediaRemoval:
                HandlePreventMediaRemoval(*cdb);
                break;

            case TScsiServerReq::EReadCapacity10:
                HandleReadCapacity10(*cdb);
                break;

            case TScsiServerReq::ERead10:
                HandleRead10(*cdb);
                break;

            case TScsiServerReq::EWrite10:
                HandleWrite10(*cdb);
                break;

            case TScsiServerReq::EReadFormatCapacities:
                HandleReadFormatCapacities(*cdb);
                break;

            default:
                iSenseInfo.SetSense(TSenseInfo::EIllegalRequest, TSenseInfo::EInvalidCmdCode);
                break;
                }
            }
        __PRINT1(_L("DecodePacket result = %d"), iSenseInfo.SenseOk());
    	decodeGood = iSenseInfo.SenseOk();
        }

    delete cdb;
    return decodeGood;
	}


/**
Checks if drive ready

@param aLun Logic unit number
@return pointer to drive correspondent to LUN if drive mounted and ready, NULL otherwise
*/
CMassStorageDrive* CScsiServerProtocol::GetCheckDrive()
	{
    __MSFNLOG
#ifdef MSDC_TESTMODE
    if (iTestParser && iTestParser->SenseError() != TTestParser::ETestSenseErrorNoSense)
        {
        switch (iTestParser->SenseError())
            {
            case TTestParser::ETestSenseErrorMediaNotPresent:
                __TESTMODEPRINT("Set SENSE ERROR(ENotReady, EMediaNotPresent)");
                iSenseInfo.SetSense(TSenseInfo::ENotReady,
                                    TSenseInfo::EMediaNotPresent);
                break;


            case TTestParser::ETestSenseErrorUnitAttention:
            default:
                __TESTMODEPRINT("Set SENSE ERROR(EUnitAttention, ENotReadyToReadyChange)");
                iSenseInfo.SetSense(TSenseInfo::EUnitAttention,
                                    TSenseInfo::ENotReadyToReadyChange);
                break;
            }
        iTestParser->ClrSenseError();
        return NULL;
        }
#endif

	CMassStorageDrive* drive = iDriveManager.Drive(iLun);
	CMassStorageDrive::TMountState mountState = drive->MountState();

	if (mountState == CMassStorageDrive::EDisconnected || mountState == CMassStorageDrive::EConnecting)
		{
		__PRINT(_L("Drive disconnected\n"));
		iSenseInfo.SetSense(TSenseInfo::ENotReady,
							TSenseInfo::EMediaNotPresent);
		return NULL;
		}

	TLocalDriveRef::TDriveState state = drive->CheckDriveState();
	if (state == TLocalDriveRef::EMediaNotPresent || state == TLocalDriveRef::ELocked)
		{
		__PRINT1(_L("Media not present or locked. (state =0x%X)\n"),state);
		iSenseInfo.SetSense(TSenseInfo::ENotReady, TSenseInfo::EMediaNotPresent);
		return NULL;
		}

	if (drive->IsMediaChanged(ETrue))  //reset "media changed" status
		{
		__PRINT(_L("Media was changed\n"));
		// SAM-2 Section 5.9.5 Unit Attention Condition
		iSenseInfo.SetSense(TSenseInfo::EUnitAttention, TSenseInfo::ENotReadyToReadyChange);
		iDriveManager.Connect(iLun);   //publish event to USB app
		return NULL;
		}

	if (mountState == CMassStorageDrive::EDisconnecting)
		{
		__PRINT(_L("Drive disconnecting\n"));
		iSenseInfo.SetSense(TSenseInfo::ENotReady,
							TSenseInfo::EMediaNotPresent);
		return NULL;
		}

	return drive;
	}


/**
Command Parser for the UNIT READY command (0x00)

@param aLun Logic unit number
@return ETrue if successful,
*/
TBool CScsiServerProtocol::HandleUnitReady()
	{
    __MSFNLOG
	return GetCheckDrive() ? ETrue : EFalse;
	}


/**
Command Parser for the REQUEST SENSE command (0x03)

@return ETrue if successful,
*/
TBool CScsiServerProtocol::HandleRequestSense(const TScsiServerReq& aRequest)
	{
    __MSFNLOG
    const TScsiServerRequestSenseReq request = static_cast<const TScsiServerRequestSenseReq&>(aRequest);
	__PRINT1(_L("length = %d\n"), request.iAllocationLength);

    TScsiServerRequestSenseResp requestSense;
    requestSense.iAllocationLength = request.iAllocationLength;

    requestSense.SetResponseCode(TScsiServerRequestSenseResp::ECurrentErrors);
    requestSense.iSensePtr = &iSenseInfo;
    requestSense.Encode(iCommandBuf);

	__PRINT4(_L("Response=0x%x Sense=0x%x, Additional=0x%x, Qualifier=0x%x\n"),
				iCommandBuf[0], iCommandBuf[02], iCommandBuf[12], iCommandBuf[13]);

	TPtrC8 writeBuf = iCommandBuf.Left(request.iAllocationLength);
	iTransport->SetupDataIn(writeBuf);

	// clear the sense info
	iSenseInfo.SetSense(TSenseInfo::ENoSense);
	return ETrue;
	}


/**
Command Parser for the INQUIRY command (0x12)

@param aLun Logic unit number
@return ETrue if successful,
*/
TBool CScsiServerProtocol::HandleInquiry(const TScsiServerReq& aRequest)
	{
    __MSFNLOG
    const TScsiServerInquiryReq request = static_cast<const TScsiServerInquiryReq&>(aRequest);

	if (request.iCmdDt || request.iEvpd || request.iPage || iLun >= KUsbMsMaxDrives)
		{
		iSenseInfo.SetSense(TSenseInfo::EIllegalRequest, TSenseInfo::EInvalidFieldInCdb);
		return EFalse;
		}

    TScsiServerInquiryResp inquiry(iConfig);

    inquiry.SetAllocationLength(request.iAllocationLength);

#ifdef MSDC_TESTMODE
    if (iTestParser && !iTestParser->Removable())
        {
        __TESTMODEPRINT("RMB Cleared");
        inquiry.SetNotRemovable();
        }
#endif

    inquiry.Encode(iCommandBuf);

    TUint length = inquiry.Length();

	TPtrC8 writeBuf = iCommandBuf.Left(length);
	iTransport->SetupDataIn(writeBuf);

	iSenseInfo.SetSense(TSenseInfo::ENoSense);
	return ETrue;
	}


/**
 Command Parser for the START STOP UNIT command (0x1B)

 @param aData command data (started form position 1)
 @param aLun Logic unit number
 @return ETrue if successful, TFalse otherwise
 */
TBool CScsiServerProtocol::HandleStartStopUnit(const TScsiServerReq& aRequest)
	{
    __MSFNLOG

	const TScsiServerStartStopUnitReq request = static_cast<const TScsiServerStartStopUnitReq&>(aRequest);

	if (request.iLoej)
		{
		if(request.iStart)	//Start unit
			{
			iDriveManager.Connect(iLun);
			__PRINT(_L("Load media\n"));

            // rd/wr publisher
			iBytesRead[iLun] = 0;
			iBytesWritten[iLun] = 0;

			// publish the initial values
			iWriteTransferPublisher->DoPublishDataTransferredEvent();
			iReadTransferPublisher->DoPublishDataTransferredEvent();
			}
		else		//Stop unit
			{
			iDriveManager.SetCritical(iLun, EFalse);
			iDriveManager.Disconnect(iLun);
			__PRINT(_L("Unload media\n"));
			}
		}

	if (request.iImmed)
		{
		return ETrue;
		}

	CMassStorageDrive* drive = iDriveManager.Drive(iLun);

	TInt  timeLeft (20);   // 1 sec timeout
	CMassStorageDrive::TMountState mountState;

	do
		{
		User::After(1000 * 50);		// 50 mSec
		--timeLeft;
		mountState = drive->MountState();

		if ((!request.iStart && mountState != CMassStorageDrive::EConnected)
			 ||
			 (request.iStart &&
				(mountState == CMassStorageDrive::EDisconnecting ||
               mountState == CMassStorageDrive::EConnected)))
			{
			return ETrue;
			}
		} while (timeLeft>0);

	//timeout happend
	iSenseInfo.SetSense(TSenseInfo::ENotReady,
						TSenseInfo::EAscLogicalUnitDoesNotRespondToSelection);
	return EFalse;
	}


/**
Command Parser for the PREVENT/ALLOW MEDIA REMOVAL command (0x1E)

@param aData command data (started form position 1)
@param aLun Logic unit number
@return ETrue if successful.
*/
TBool CScsiServerProtocol::HandlePreventMediaRemoval(const TScsiServerReq& aRequest)
	{
    __MSFNLOG
	const TScsiServerPreventMediaRemovalReq& request = static_cast<const TScsiServerPreventMediaRemovalReq&>(aRequest);
	__FNLOG("CScsiProtocol::HandlePreventMediaRemoval");
	CMassStorageDrive* drive = GetCheckDrive();

	if (drive == NULL)
		{
		return EFalse;
		}
	iDriveManager.SetCritical(iLun, request.iPrevent);
	return ETrue;
	}


/** Cancel active state, Invoked by transnport when it stops */
TInt CScsiServerProtocol::Cancel()
	{
    __MSFNLOG
	iDriveManager.SetCritical(CDriveManager::KAllLuns, EFalse);
	return KErrNone;
	}


TBool CScsiServerProtocol::HandleReadFormatCapacities(const TScsiServerReq& aRequest)
/**
 * Command Parser for the READ FORMAT CAPACITIES command (0x23)
 *
 * @return ETrue if successful, else a standard Symbian OS error code.
 */
	{
    __MSFNLOG
	const TScsiServerReadFormatCapacitiesReq& request = static_cast<const TScsiServerReadFormatCapacitiesReq&>(aRequest);

    CMassStorageDrive* drive = NULL;
    for (TInt i = 0; i < 10; i++)
        {
        drive = GetCheckDrive();
        if (drive)
            {
            break;
            }
        User::After(100000);
        }

	if (!drive)
		{
        return EFalse;
		}

	TUint32 numBlocks = I64LOW(drive->MediaParams().NumBlocks());

    TScsiServerReadFormatCapacitiesResp response(request.AllocationLength());
    response.SetNumberBlocks(numBlocks);

    response.Encode(iCommandBuf);
	TPtrC8 writeBuf = iCommandBuf;
	iTransport->SetupDataIn(writeBuf);
	return ETrue;
	}


/**
Command Parser for the READ CAPACITY(10) command (0x25)

@param aData command data (started form position 1)
@param aLun Logic unit number
@return ETrue if successful.
*/
TBool CScsiServerProtocol::HandleReadCapacity10(const TScsiServerReq& aRequest)
	{
    __MSFNLOG
	const TScsiServerReadCapacity10Req& request = static_cast<const TScsiServerReadCapacity10Req&>(aRequest);
	CMassStorageDrive* drive = GetCheckDrive();
	if (drive == NULL)
		{
		return EFalse;
		}

	if (request.iPmi || request.iLogicalBlockAddress)   //do not support partial medium indicator
		{
		iSenseInfo.SetSense(TSenseInfo::EIllegalRequest, TSenseInfo::EInvalidFieldInCdb);
		return EFalse;
		}

    TScsiServerReadCapacity10Resp response;
    response.Set(drive->MediaParams().BlockSize(), drive->MediaParams().NumBlocks());
    response.Encode(iCommandBuf);

	TPtrC8 writeBuf = iCommandBuf;
	iTransport->SetupDataIn(writeBuf);

	return KErrNone;
	}


/**
Command Parser for the READ10 command (0x28)

@param aData command data (started form position 1)
@param aLun Logic unit number
@return ETrue if successful.
*/
TBool CScsiServerProtocol::HandleRead10(const TScsiServerReq& aRequest)
	{
    __MSFNLOG
	const TScsiServerRead10Req& request = static_cast<const TScsiServerRead10Req&>(aRequest);
	CMassStorageDrive* drive = GetCheckDrive();
	if (drive == NULL)
		{
		return EFalse;
		}

	if (request.iProtect)
		{
		iSenseInfo.SetSense(TSenseInfo::EIllegalRequest, TSenseInfo::EInvalidFieldInCdb);
		return EFalse;
		}

	if (!request.iTransferLength)
		{
		return ETrue; // do nothing - this is not an error
		}

    TUint32 blockSize = drive->MediaParams().BlockSize();

	const TInt64 bOffset = static_cast<TInt64>(request.iLogicalBlockAddress) * blockSize;
	const TInt bLength = request.iTransferLength * blockSize;
	const TInt64 theEnd = bOffset + bLength;

	if (theEnd > drive->MediaParams().Size())  //check if media big enough for this request
		{
		__PRINT(_L("err - Request ends out of media\n"));
		iSenseInfo.SetSense(TSenseInfo::EIllegalRequest, TSenseInfo::ELbaOutOfRange);
		return EFalse;
		}

	// check if our buffer can hold requested data
	if (iDataBuf.MaxLength() < bLength)
		{
        TRAPD(err,iDataBuf.ReAllocL(bLength));
        if (err)
            {
            __PRINT(_L("err - Buffer too small\n"));
            iSenseInfo.SetSense(TSenseInfo::EIllegalRequest, TSenseInfo::EInvalidFieldInCdb);
            return EFalse;
            }
		}

    iDataBuf.SetLength(bLength);
	TInt err = drive->Read(bOffset, bLength, iDataBuf);
	if (err != KErrNone)
		{
		__PRINT1(_L("Read failed, err=%d\n"), err);
		iSenseInfo.SetSense(TSenseInfo::ENotReady, TSenseInfo::EMediaNotPresent);
		return EFalse;
		}

	TPtrC8 writeBuf = iDataBuf;

    // rd publisher
	iBytesRead[iLun] += writeBuf.Length();
	iReadTransferPublisher->StartTimer();

	// Set up data write to the host
#ifdef MSDC_TESTMODE
    if (iTestParser)
        {
        TBool test = iTestParser->DInSearch(writeBuf);
        }
#endif
	iTransport->SetupDataIn(writeBuf);
	return ETrue;
	}


/**
Command Parser for the WRITE(10) command (0x2A)

@param aData command data (started form position 1)
@param aLun Logic unit number
@return ETrue if successful.
*/
TBool CScsiServerProtocol::HandleWrite10(const TScsiServerReq& aRequest)
	{
    __MSFNLOG
	const TScsiServerWrite10Req& request = static_cast<const TScsiServerWrite10Req&>(aRequest);
	CMassStorageDrive* drive = GetCheckDrive();
	if (drive == NULL)
		{
		return EFalse;
		}
	if (request.iProtect)
		{
		iSenseInfo.SetSense(TSenseInfo::EIllegalRequest, TSenseInfo::EInvalidFieldInCdb);
		return EFalse;
		}

	if (!request.iTransferLength)
		{
		return ETrue; // do nothing - this is not an error
		}

    const TMediaParams& params = drive->MediaParams();

	if (params.IsWriteProtected() ||
		params.IsLocked())
		{
		iSenseInfo.SetSense(TSenseInfo::EDataProtection, TSenseInfo::EWriteProtected);
		return EFalse;
		}

    TInt64 theEnd = iMediaWriteMan.Start(request.iLogicalBlockAddress, request.iTransferLength, params.BlockSize());
	if (theEnd > params.Size())  //check if media big enough for this request
		{
		__PRINT(_L("err - Request ends out of media\n"));
		iSenseInfo.SetSense(TSenseInfo::EIllegalRequest, TSenseInfo::ELbaOutOfRange);
		return EFalse;
		}

    TUint32 thisLength = iMediaWriteMan.GetPacketLength();

    // check if our buffer can hold requested data
    if (iDataBuf.MaxLength() < thisLength)
        {
        TRAPD(err,iDataBuf.ReAllocL(thisLength));
        if (err)
            {
            __PRINT(_L("err - Buffer too small\n"));
            iSenseInfo.SetSense(TSenseInfo::EIllegalRequest, TSenseInfo::EInvalidFieldInCdb);
            return EFalse;
            }
        }

	iDataBuf.SetLength(thisLength);
	TPtr8 readBuf = iDataBuf.LeftTPtr(iDataBuf.Length());

    // wr publisher
	iBytesWritten[iLun] += readBuf.Length();
	iWriteTransferPublisher->StartTimer();
	iTransport->SetupDataOut(readBuf);
	return ETrue;
	}


void CScsiServerProtocol::MediaWriteAbort()
    {
    __MSFNLOG
    iMediaWriteMan.Reset();
    iSenseInfo.SetSense(TSenseInfo::EAbortedCommand);
    }

/**
Called by the transport when the requested data has been read or an error has
occurred during the read.

@param aError Indicate if an error occurs during reading data by transport.
@return KErrAbort if command processing is complete but has failed,
        KErrCompletion if sufficient data is available in the buffer to process
        the transfer immediately, KErrNotReady if insufficient data is
        available in the buffer so the transport should wait for it to arrive,
        KErrNone if command processing is complete and was successful.
*/
TInt CScsiServerProtocol::MediaWritePacket(TUint& aBytesWritten)
	{
    __MSFNLOG
    aBytesWritten = 0;
    if (iMediaWriteMan.Active() == EFalse)
        {
        iSenseInfo.SetSense(TSenseInfo::EAbortedCommand);
        return KErrAbort;
        }

    CMassStorageDrive* drive = GetCheckDrive();
    if (drive == NULL)
        {
        return KErrAbort;
        }

#ifdef MSDC_TESTMODE
    if (iTestParser && iTestParser->Enabled())
        {
        TInt testCase = iTestParser->TestCase();
        if (testCase == TTestParser::ETestCaseDoStallData)
            {
            iTestParser->DecTestCounter();

            TInt testCounter = iTestParser->TestCounter();
            if (testCounter == 1)
                {
                __TESTMODEPRINT1("Aborting MediaWritePacket (Data Stall) x%x", iMediaWriteMan.BytesRemain());
                __TESTMODEPRINT2("Offset=0x%lx Length=%x",
                                 iMediaWriteMan.Offset(), iDataBuf.Length());
                return KErrNone;
                }
            else if (testCounter == 0)
                {
                // Display the next write and clear the test
                iTestParser->ClrTestCase();
                __TESTMODEPRINT2("Offset=0x%lx Length=%x",
                                 iMediaWriteMan.Offset(), iDataBuf.Length());
                }
            }
        else if (testCase == TTestParser::ETestCaseDoResidue)
            {
            iTestParser->DecTestCounter();
            TInt testCounter = iTestParser->TestCounter();

            if (testCounter == 1)
                {
                // abort write and leave residue
                __TESTMODEPRINT1("Aborting MediaWritePacket (Data Residue) x%x", iMediaWriteMan.BytesRemain());
                __TESTMODEPRINT2("Offset=0x%lx Length=0x%x",
                                 iMediaWriteMan.Offset(), iDataBuf.Length());
                aBytesWritten = 0;
                return KErrAbort;
                }
            else if (testCounter == 0)
                {
                // Display the next write and clear the test
                iTestParser->ClrTestCase();
                __TESTMODEPRINT2("MediaWritePacket Offset=0x%lx Length=0x%x",
                                 iMediaWriteMan.Offset(), iDataBuf.Length());
                }
            else
                {
                __TESTMODEPRINT3("MediaWritePacket[%x] Offset=0x%lx Length=0x%x",
                                 testCounter, iMediaWriteMan.Offset(), iDataBuf.Length());
                }

            }
        }
#endif

	const TInt64 bOffset = iMediaWriteMan.Offset();
    iMediaWriteMan.Reset();

   	__PRINT1(_L("SCSI: writing %d bytes\n"), iDataBuf.Length());

    TInt err = KErrNone;
#ifdef MSDC_TESTMODE
    if (iTestParser)
        {
        TBool test = iTestParser->DoutSearch(iDataBuf);
        if (test)
            {
            // Do not write test control blocks to media
            }
        else
            {
            // ********* Write data to the drive ********
            err = drive->Write(bOffset, iDataBuf);
            }
        }
#else
    // ********* Write data to the drive ********
   	err = drive->Write(bOffset, iDataBuf);
#endif
   	if (err != KErrNone)
   		{
   		__PRINT1(_L("Error after write = 0x%X \n"), err);
   		iSenseInfo.SetSense(TSenseInfo::EAbortedCommand);
   		return KErrAbort;
   		}

   	TUint thisLength = iDataBuf.Length();
    aBytesWritten = thisLength;

    iMediaWriteMan.SetOffset(bOffset, thisLength);

   	if (iMediaWriteMan.BytesRemain() == 0)
        {
        return iSenseInfo.SenseOk() ? KErrNone : KErrAbort;
        }

    // More data is expected - set up another request to read from the host
    const TUint32 nextPacketLength = iMediaWriteMan.NextPacket();
 	TUint bytesAvail = iTransport->BytesAvailable() & ~(drive->MediaParams().BlockSize()-1);

 	TBool wait = EFalse;

    thisLength = nextPacketLength;
    if (bytesAvail)
        {
        if (bytesAvail < nextPacketLength)
            {
            // Not enough data is available at the transport to satisfy the
            // request, so return KErrNotReady to indicate that the transport
            // should wait.
            thisLength = nextPacketLength;
            wait = ETrue;
            }
        }

 	thisLength = (thisLength > KMaxBufSize) ? KMaxBufSize : thisLength;

   	iDataBuf.SetLength(thisLength);
   	TPtr8 readBuf = iDataBuf.LeftTPtr(iDataBuf.Length());
    iTransport->SetupDataOut(readBuf);
    return wait ? KErrNotReady : KErrCompletion;
	}


/**
Command Parser for the MODE SENSE(06) command (0x1A)

@return ETrue if successful.
*/
TBool CScsiServerProtocol::HandleModeSense6(const TScsiServerReq& aRequest)
	{
    __MSFNLOG
	const TScsiServerModeSense6Req& request = static_cast<const TScsiServerModeSense6Req&>(aRequest);

	TScsiServerModeSense6Resp response;
    response.SetAllocationLength(request.iAllocationLength);

	if (request.iPageCode != TScsiServerModeSense6Req::KAllPages ||
        request.iPageControl == TScsiServerModeSense6Req::EChangeableValues)
		{
		__PRINT(_L("TSenseInfo::EIllegalRequest,TSenseInfo::EInvalidFieldInCdb"));
		iSenseInfo.SetSense(TSenseInfo::EIllegalRequest,TSenseInfo::EInvalidFieldInCdb);
		return EFalse;
		}
	if (request.iPageControl != TScsiServerModeSense6Req::EDefaultValues)
		{
		//check if drive write protected
		CMassStorageDrive* drive = GetCheckDrive();
		if (drive == NULL)
			{
			__PRINT(_L("drive == null"));
			return EFalse;
			}

#ifdef MSDC_TESTMODE
    if (iTestParser)
        {
        response.SetWp(iTestParser->WriteProtect());
        }
#else
        response.SetWp(drive->MediaParams().IsWriteProtected());
#endif
        }

    response.Encode(iCommandBuf);

	TPtrC8 writeBuf = iCommandBuf;
	iTransport->SetupDataIn(writeBuf);
	return iSenseInfo.SenseOk();
	}
