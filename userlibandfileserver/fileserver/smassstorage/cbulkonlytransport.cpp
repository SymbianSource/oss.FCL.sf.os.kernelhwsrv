// Copyright (c) 2004-2010 Nokia Corporation and/or its subsidiary(-ies).
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

#include <e32std.h>
#include "mtransport.h"
#include "mprotocol.h"
#include "mldddevicestatenotification.h"
#include "tbulkmm.h"

#include "drivemanager.h"
#include "cusbmassstoragecontroller.h"

#include "cbulkonlytransport.h"
#include "cbulkonlytransportusbcldd.h"
#if !defined(__WINS__) && !defined(__X86__)
#include "cbulkonlytransportusbcscldd.h"
#endif
#include "smassstorage.h"

#include "OstTraceDefinitions.h"
#ifdef OST_TRACE_COMPILER_IN_USE
#include "cbulkonlytransportTraces.h"
#endif


//CBW offsets
static const TInt KCbwSignatureOffset          = 0;
static const TInt KCbwTagOffset                = 4;
static const TInt KCbwDataTransferLengthOffset = 8;
static const TInt KCbwFlagOffset               = 12;
static const TInt KCbwLunOffset                = 13;
static const TInt KCbwCbLengthOffset           = 14;

static const TInt KMaxCbwcbLength              = 16;

// CSW offsets
static const TInt KCswSingnatureOffset         = 0;
static const TInt KCswTagOffset                = 4;
static const TInt KCswDataResidueOffset        = 8;
static const TInt KCswStatusOffset             = 12;


/**
 This function unpacks into the TUsbRequestHdr class from a descriptor with
 the alignment that would be introduced on the USB bus.

 @param aBuffer Input buffer
 @param aTarget Unpacked header.
 @return Error.
 */
TInt TUsbRequestHdr::Decode(const TDesC8& aBuffer)
    {
    if (aBuffer.Length() < static_cast<TInt>(KRequestHdrSize))
        {
        OstTrace1(TRACE_SMASSSTORAGE_USB, TUSBREQUESTHDR_DECODE_ERR,
                 "ERROR: USB DEVICE REQUEST invalid length 0x%x", aBuffer.Length());
        return KErrGeneral;
        }

    iRequestType = aBuffer[0];
    iRequest = static_cast<TEp0Request>(aBuffer[1]);
    iValue   = static_cast<TUint16>(aBuffer[2] + (aBuffer[3] << 8));
    iIndex   = static_cast<TUint16>(aBuffer[4] + (aBuffer[5] << 8));
    iLength  = static_cast<TUint16>(aBuffer[6] + (aBuffer[7] << 8));
    OstTraceExt5(TRACE_SMASSSTORAGE_USB, TUSBREQUESTHDR_DECODE,
              "USB DEVICE REQUEST type=0x%x request=0x%x value=%d index=%d length=%d",
              iRequestType, iRequest, iValue, iIndex, iLength);

    return KErrNone;
    }


/**
This function determines whether data is required by the host in response
to a message header.

@return TBool   Flag indicating whether a data response required.
*/
TBool TUsbRequestHdr::IsDataResponseRequired() const

    {
    return (iRequestType & 0x80) ? (TBool)ETrue : (TBool)EFalse;
    }

//-------------------------------------
/**
Create an object of a class derived from CBulkOnlyTransport (default to CBulkOnlyTransportUsbcLdd object)
@param aNumDrives - The number of drives available for MS
@param aController - reference to the parent
@return pointer to newly created derived class object
*/
CBulkOnlyTransport* CBulkOnlyTransport::NewL(TInt aNumDrives,CUsbMassStorageController& aController)
    {
    return NewL(aNumDrives,aController, (CUsbMassStorageController::TTransportldd) 1);
    }

/**
Create an object of a class derived from CBulkOnlyTransport
@param aNumDrives - The number of drives available for MS
@param aController - reference to the parent
@param aTransportLddFlag - Type of usb client ldd
@return pointer to newly created derived class object
*/
CBulkOnlyTransport* CBulkOnlyTransport::NewL(TInt aNumDrives,CUsbMassStorageController& aController, CUsbMassStorageController::TTransportldd aTransportLddFlag)
    {
    if (aNumDrives <=0 || static_cast<TUint>(aNumDrives) > KUsbMsMaxDrives)
        {
        User::Leave(KErrArgument);
        }

#if !defined(__WINS__) && !defined(__X86__)
    CBulkOnlyTransportUsbcScLdd* scTransport;
#endif
    CBulkOnlyTransportUsbcLdd* nonscTransport;
    switch (aTransportLddFlag)
        {
        case 1:
            nonscTransport = new(ELeave) CBulkOnlyTransportUsbcLdd(aNumDrives, aController);
            OstTrace0(TRACE_SMASSSTORAGE_BOT, CBULKONLYTRANSPORT_NEWL1, "CBulkOnlyTransportUsbcLdd created.");
            return nonscTransport;
#if !defined(__WINS__) && !defined(__X86__)
        case 2:
            scTransport = new(ELeave) CBulkOnlyTransportUsbcScLdd(aNumDrives, aController);
            OstTrace0(TRACE_SMASSSTORAGE_BOT, CBULKONLYTRANSPORT_NEWL2, "CBulkOnlyTransportUsbcScLdd created.");
            return scTransport;
#endif
        default:
            __ASSERT_DEBUG(EFalse, User::Panic(KUsbMsSvrPncCat, EMsCBulkOnlyTransportNull));
            return NULL;

        }
    }


TInt CBulkOnlyTransport::InitialiseTransportL(TInt aTransportLddFlag)
    {
    TInt ret = KErrNone;
    MTransportBase* transport;
    iController.GetTransport(transport);
    switch (aTransportLddFlag)
        {
#if !defined(__WINS__) && !defined(__X86__)
        case 2:
                ret = ((CBulkOnlyTransportUsbcScLdd*) transport)->Ldd().Open(0);
                if (ret != KErrNone)
                    {
                    return ret;
                    }
                else
                    {
                    ((CBulkOnlyTransportUsbcScLdd*) transport)->Ldd().Close();
                    CleanupStack::PushL(transport);
                    ((CBulkOnlyTransportUsbcScLdd*) transport)->ConstructL();
                    CleanupStack::Pop(transport);
                    return ret;
                    }
#endif
        case 1:
                ret = ((CBulkOnlyTransportUsbcLdd*) transport)->Ldd().Open(0);
                if (ret != KErrNone)
                    {
                    return ret;
                    }
                else
                    {
                    ((CBulkOnlyTransportUsbcLdd*) transport)->Ldd().Close();
                    CleanupStack::PushL(transport);
                    ((CBulkOnlyTransportUsbcLdd*) transport)->ConstructL();
                    CleanupStack::Pop(transport);
                    return ret;
                    }
        default:
                return KErrNotFound;
        }
    }

/**
c'tor
@param aNumDrives - The number of drives available for MS
@param aController - reference to the parent
*/
CBulkOnlyTransport::CBulkOnlyTransport(TInt aNumDrives,CUsbMassStorageController& aController):
    CActive(EPriorityStandard),
    iMaxLun(aNumDrives-1),
    iController(aController),
    iStallAllowed(ETrue),
    iInterfaceConfigured(EFalse),
    iCommandBufPtr(NULL,0),
    iDataBufPtr(NULL,0),
    iCswBufPtr(NULL,0),
    iPaddingBufPtr(NULL,0),
    iWriteBufPtr(NULL,0),
    iReadBufPtr(NULL, 0),
    iCbwBufPtr(NULL,0)
    {
    }

/**
Destructor
*/
CBulkOnlyTransport::~CBulkOnlyTransport()
    {
    }


/**
Called by the protocol after processing the packet to indicate that more data is required.

@param aData reference to the data buffer.
*/
void CBulkOnlyTransport::SetupReadData(TUint aLength)
    {
    OstTrace1(TRACE_SMASSSTORAGE_BOTDATA, CBULKONLYTRANSPORT_SETUPREADDATA,
              "BOT SetupReadData Length = 0x%x bytes", aLength);
    iBufSize = aLength;
    iReadSetUp = ETrue;
    }


/**
Called by the protocol after processing the packet to indicate that data should be written to the host.

@param aData reference to the data buffer.
*/
void CBulkOnlyTransport::SetupWriteData(TPtrC8& aData)
    {
    OstTrace1(TRACE_SMASSSTORAGE_BOTDATA, CBULKONLYTRANSPORT_SETUPWRITEDATA,
              "BOT SetupWriteData Length = 0x%x bytes", aData.Length());
    iWriteBufPtr.Set(aData);
    iWriteSetUp = ETrue;
    }


TInt CBulkOnlyTransport::Start()
    {
    OstTrace0(TRACE_SMASSSTORAGE_BOT, CBULKONLYTRANSPORT_START, "Start...");
    if (!iProtocol)
        {
        return KErrBadHandle;   //protocol should be set up before start
        }

    if (IsActive())
        {
        OstTrace0(TRACE_SMASSSTORAGE_BOT, CBULKONLYTRANSPORT_START1, "Error: active before start!");
        return KErrInUse;
        }

    TInt err = KErrNone;
    if ((err = SetupConfigurationDescriptor())  != KErrNone ||
        (err = SetupInterfaceDescriptors())     != KErrNone )
        {
        OstTrace1(TRACE_SMASSSTORAGE_BOT, CBULKONLYTRANSPORT_START2, "Error: descriptors setup failed %d", err);
        return err;
        }

    AllocateEndpointResources();
    ActivateDeviceStateNotifier();  // activate notifier wich will wait until USB became configured
    TUsbcDeviceState deviceStatus = EUsbcDeviceStateDefault;
    err = GetDeviceStatus(deviceStatus);
    if (err == KErrNone && deviceStatus == EUsbcDeviceStateConfigured)
        {
        err = HwStart();
        }

#ifdef MSDC_MULTITHREADED
    TPtr8 aDes1(NULL,0);
    TPtr8 aDes2(NULL,0);
    GetBufferPointers(aDes1, aDes2);
    iProtocol->InitializeBufferPointers(aDes1, aDes2); // have to pass pointer to memory not offsets to initialise TPtr, and lengths
#endif

    iInterfaceConfigured = ETrue;
    return err;
    }

TInt CBulkOnlyTransport::HwStart(TBool aDiscard)
    {
    TInt lun = MaxLun();
    OstTrace1(TRACE_SMASSSTORAGE_BOT, CBULKONLYTRANSPORT_HWSTART, "HwStart MAX LUN=%d", lun);
    do
        {
        Controller().DriveManager().Connect(lun);
        }
    while(--lun >= 0);

    TInt res = StartControlInterface();

    iCurrentState = ENone;
    iWriteSetUp=EFalse;
    iReadSetUp=EFalse;
    iStarted = ETrue;

    if (aDiscard)
        {
        FlushData();
        }

    ReadCBW();
    return res;
    }


TInt CBulkOnlyTransport::HwStop()
    {
    OstTrace0(TRACE_SMASSSTORAGE_BOT, CBULKONLYTRANSPORT_HWSTOP, "HwStop");
    if (iStarted)
        {
        StopBulkOnlyEndpoint();
        CancelControlInterface();
        iStarted = EFalse;
        }
    return KErrNone;
    }


void CBulkOnlyTransport::StopBulkOnlyEndpoint()
    {
    TInt lun = MaxLun();
    do
        {
        Controller().DriveManager().Disconnect(lun);
        }
    while(--lun >= 0);
    Cancel();
    iProtocol->Cancel();
    }


TInt CBulkOnlyTransport::HwSuspend()
    {
    OstTrace0(TRACE_SMASSSTORAGE_BOT, CBULKONLYTRANSPORT_HWSUSPEND, "HwSuspend");
    TInt lun = MaxLun();
    do
        {
        Controller().DriveManager().Disconnect(lun);
        }
    while(--lun >= 0);

    return KErrNone;
    }


TInt CBulkOnlyTransport::HwResume()
    {
    OstTrace0(TRACE_SMASSSTORAGE_BOT, CBULKONLYTRANSPORT_HWRESUME, "HwResume");
    TInt lun = MaxLun();
    do
        {
        Controller().DriveManager().Connect(lun);
        }
    while(--lun >= 0);

    return KErrNone;
    }

/**
Stops the Bulk Only Transport
*/
TInt CBulkOnlyTransport::Stop()
    {
    OstTrace0(TRACE_SMASSSTORAGE_BOT, CBULKONLYTRANSPORT_STOP, "Stop");
    CancelControlInterface();
    CancelDeviceStateNotifier();
    Cancel();
    if  (iInterfaceConfigured)
        {
        ReleaseInterface();
        SetupConfigurationDescriptor(ETrue);
        }
    iCurrentState = ENone;
    iInterfaceConfigured = EFalse;

    return KErrNone;
    }



void CBulkOnlyTransport::DoCancel()
    {
    CancelReadWriteRequests();
    }


void CBulkOnlyTransport::Activate(TInt aReason)
    {
    SetActive();
    TRequestStatus* r = &iStatus;
    User::RequestComplete(r, aReason);
    }


void CBulkOnlyTransport::RunL()
    {
    if (iStatus != KErrNone)
        {
        OstTrace1(TRACE_SMASSSTORAGE_BOT, CBULKONLYTRANSPORT_RUNL, "Error %d in RunL, halt endpoints", iStatus.Int());
        SetPermError(); //halt endpoints for reset recovery
        return;
        }
    switch (iCurrentState)
        {
        case EWaitForCBW:
            OstTrace0(TRACE_SMASSSTORAGE_BOT1, CBULKONLYTRANSPORT_RUNL1, "EWaitForCBW");
            ProcessCbwEvent();
            break;

        case EWritingData:
            OstTrace0(TRACE_SMASSSTORAGE_BOT1, CBULKONLYTRANSPORT_RUNL2, "EWritingData");
            iWriteSetUp = EFalse;  //the buffer was used

            if (iDataResidue && iStallAllowed)
                {
                StallEndpointAndWaitForClear();
                }

            SendCSW(iCbwTag, iDataResidue, iCmdStatus);
            break;

        case EReadingData:
            {
            OstTrace0(TRACE_SMASSSTORAGE_BOT1, CBULKONLYTRANSPORT_RUNL3, "EReadingData");
            ProcessReadingDataEvent();
            }
            break;

        case ESendingCSW:
            OstTrace0(TRACE_SMASSSTORAGE_BOT1, CBULKONLYTRANSPORT_RUNL4, "ESendingCSW");
            ReadCBW();
            break;

        case EPermErr:
            OstTrace0(TRACE_SMASSSTORAGE_BOT1, CBULKONLYTRANSPORT_RUNL5, "EPermErr");
            StallEndpointAndWaitForClear();
            break;

        default:
            SetPermError();     // unexpected state
        }
    }


/**
Decode the CBW received from the host via OutEndpoint

- If the header is valid, the data content is passed to the parser.
- Depending on the command, more data may be transmitted/received.
- ...or the CSW is sent (if not a data command).

*/
void CBulkOnlyTransport::DecodeCBW()
    {
    SetCbwPtr();

    if (!CheckCBW())  //check if CBW valid and meaningful
        {
        // CBW not valid or meaningful
        // Specification says: "If the CBW is not valid, the device shall STALL
        // the Bulk-In pipe. Also, the device shall either STALL the Bulk-Out pipe,
        // or the device shall accept and discard any Bulk-Out data. The device
        // shall maintain this state until a Reset Recovery."
        // Here we keep bulk-in ep stalled and ignore bulk-out ep.
        SetPermError();
        ExpireData((TAny*) (iCbwBufPtr.Ptr()));
        return;
        }

    TPtrC8 aData;
    aData.Set(&iCbwBufPtr[KCbwCbLengthOffset], KMaxCbwcbLength+1);    //prepare data for protocol starting form Length
    TUint lun = static_cast<TUint8>(iCbwBufPtr[13] & 0x0f);

    iCbwTag  =  static_cast<TUint32>(iCbwBufPtr[KCbwTagOffset])     |
                static_cast<TUint32>(iCbwBufPtr[KCbwTagOffset+1])   <<8 |
                static_cast<TUint32>(iCbwBufPtr[KCbwTagOffset+2])   <<16|
                static_cast<TUint32>(iCbwBufPtr[KCbwTagOffset+3])   <<24;

    TInt i = KCbwDataTransferLengthOffset;
    TUint32 hostDataLength =  static_cast<TUint32>(iCbwBufPtr[i  ])       |
                            static_cast<TUint32>(iCbwBufPtr[i+1]) <<8   |
                            static_cast<TUint32>(iCbwBufPtr[i+2]) <<16  |
                            static_cast<TUint32>(iCbwBufPtr[i+3]) <<24;

    TBool dataToHost = iCbwBufPtr[KCbwFlagOffset] & 0x80;

    OstTrace1(TRACE_SMASSSTORAGE_BOT, CBULKONLYTRANSPORT_DECODECBW,
              "CBW LUN=%d", lun);
    OstTraceExt3(TRACE_SMASSSTORAGE_BOT, CBULKONLYTRANSPORT_DECODECBW0,
              "CBW hostDataLength=0x%x, CBWtag = 0x%x, dataToHost=%d",
              hostDataLength, iCbwTag, (TUint32)dataToHost);
    //////////////////////////////////////////////
    TBool ret = iProtocol->DecodePacket(aData, lun);
    //////////////////////////////////////////////
    ExpireData((TAny*) (iCbwBufPtr.Ptr()));

    iStallAllowed = ETrue;

    if (!ret)
        {
        OstTrace1(TRACE_SMASSSTORAGE_BOT, CBULKONLYTRANSPORT_DECODECBW1, "ERROR: DecodePacket err=%d", ret);
        iCmdStatus = ECommandFailed;
        }
    else
        {
        OstTrace0(TRACE_SMASSSTORAGE_BOT1, CBULKONLYTRANSPORT_DECODECBW2, "ERROR: DecodePacket OK");
        iCmdStatus = ECommandPassed;
        }

    if (hostDataLength)    // Host  expected data transfer
        {
        if (dataToHost)  // send data to host
            {
            if (!iWriteSetUp) //write buffer was not set up
                {
                OstTrace0(TRACE_SMASSSTORAGE_BOT1, CBULKONLYTRANSPORT_DECODECBW3, "Write buffer was not setup");
                iDataResidue =hostDataLength;
                OstTrace1(TRACE_SMASSSTORAGE_BOT1, CBULKONLYTRANSPORT_DECODECBW4, "DataResidue (write to host)=%d", iDataResidue);

//------------------------------------
                if (hostDataLength <= KBOTMaxBufSize)
                    {
                    OstTrace0(TRACE_SMASSSTORAGE_BOT1, CBULKONLYTRANSPORT_DECODECBW5, "Case 4 or 8");
                    SetPaddingBufPtr(hostDataLength);
                    iPaddingBufPtr.FillZ(hostDataLength);
                    TPtrC8 ptr(NULL, 0);
                    ptr.Set((TUint8*)iPaddingBufPtr.Ptr(), hostDataLength);
                    WriteData(iStatus, ptr, hostDataLength, EFalse);
                    iStallAllowed = EFalse;
                    if (iReadSetUp)  //read buffer WAS set up - case (8)
                        {
                        OstTrace0(TRACE_SMASSSTORAGE_BOT1, CBULKONLYTRANSPORT_DECODECBW6, "It is Case 8");
                        iCmdStatus = EPhaseError;
                        }
                    return;
                    }
                else
//------------------------------------
//                  Use next block instead of StallEndpointAndWaitForClear(InEndpoint);
                    {
                    SetPaddingBufPtr(hostDataLength);
                    iPaddingBufPtr.FillZ(KBOTMaxBufSize);
                    TUint c =0;
                    TRequestStatus status;
                    while (c<hostDataLength)
                        {
                        TInt len;
                        if (hostDataLength - c >  KBOTMaxBufSize)
                            {
                            len = KBOTMaxBufSize;
                            }
                        else
                            {
                            len = hostDataLength - c;
                            }

                            TPtrC8 ptr(NULL, 0);
                            ptr.Set((TUint8*)iPaddingBufPtr.Ptr(), len);
                            WriteUsb(status, ptr, len);
                            User::WaitForRequest(status);
                            c +=  KBOTMaxBufSize;
                        }
                    }

                if (iReadSetUp)  //read buffer WAS set up - case (8)
                    {
                    OstTrace0(TRACE_SMASSSTORAGE_BOT1, CBULKONLYTRANSPORT_DECODECBW7, "Case 8");
                    SendCSW(iCbwTag, hostDataLength, EPhaseError);
                      //don't care to reset any flag - should get reset recovery
                    }
                else   // case (4)
                    {
                    OstTrace0(TRACE_SMASSSTORAGE_BOT1, CBULKONLYTRANSPORT_DECODECBW8, "Case 4");
                    SendCSW(iCbwTag, hostDataLength, iCmdStatus);
                    }
                return;
                }   // if (!iWriteSetUp)

//==================
            TUint32 deviceDataLength = iWriteBufPtr.Length();
            iDataResidue =hostDataLength - deviceDataLength ;
            OstTraceExt2(TRACE_SMASSSTORAGE_BOT1, CBULKONLYTRANSPORT_DECODECBW9,
                         "Device data length = 0x%x, DataResidue (write to host)=0x%x", deviceDataLength, iDataResidue);

            if (deviceDataLength < hostDataLength  &&
                hostDataLength < KBOTMaxBufSize )
                    {
                    OstTrace0(TRACE_SMASSSTORAGE_BOT1, CBULKONLYTRANSPORT_DECODECBW10, "Case 5 (padding)");
                    SetPaddingBufPtr(hostDataLength);
                    iPaddingBufPtr.Zero();
                    iPaddingBufPtr.Append(iWriteBufPtr);
                    iStallAllowed = EFalse;
                    OstTrace1(TRACE_SMASSSTORAGE_BOT1, CBULKONLYTRANSPORT_DECODECBW11,
                              "iPaddingBufPtr.Length = 0x%x",
                              iPaddingBufPtr.Length());
                    TPtrC8 ptr(NULL, 0);
                    ptr.Set((TUint8*)iPaddingBufPtr.Ptr(), hostDataLength);
                    WriteData(iStatus, ptr, hostDataLength, EFalse);
                    return;
                    }

//===================

            if (deviceDataLength == hostDataLength)     //case (6)[==]
                {
                OstTrace0(TRACE_SMASSSTORAGE_BOT1, CBULKONLYTRANSPORT_DECODECBW12, "Case 6");
                WriteData(iStatus, iWriteBufPtr, deviceDataLength);
                return;
                }
            else if (deviceDataLength < hostDataLength) //case (5)[<]
                {
                OstTrace0(TRACE_SMASSSTORAGE_BOT1, CBULKONLYTRANSPORT_DECODECBW13, "Case 5");
                WriteData(iStatus, iWriteBufPtr, deviceDataLength, ETrue);      // Send ZLP
                return;
                }
            else                                        // deviceDataLength > hostDataLength - case (7)
                {
                OstTrace0(TRACE_SMASSSTORAGE_BOT1, CBULKONLYTRANSPORT_DECODECBW14, "Case 7");
                iCmdStatus = EPhaseError;
                iDataResidue = 0;
                WriteData(iStatus, iWriteBufPtr, hostDataLength);
                return;
                }
            }
        else  //read data from host
            {
            if (!iReadSetUp)
                {
                iDataResidue = hostDataLength;
                OstTrace0(TRACE_SMASSSTORAGE_BOT, CBULKONLYTRANSPORT_DECODECBW15, "Read buffer was not setup");
//              Use next block instead of StallEndpointAndWaitForClear(OutEndpoint);
                DiscardData(hostDataLength);

                if (iWriteSetUp) //case (10)
                    {
                    OstTrace0(TRACE_SMASSSTORAGE_BOT1, CBULKONLYTRANSPORT_DECODECBW16, "case 10");
                    SendCSW(iCbwTag, hostDataLength, EPhaseError);
                    }
                else // case (9)
                    {
                    OstTrace0(TRACE_SMASSSTORAGE_BOT1, CBULKONLYTRANSPORT_DECODECBW17, "Case 9");
                    SendCSW(iCbwTag, hostDataLength, iCmdStatus);
                    }

                return;
                }

            TUint32 deviceDataLength = iBufSize;
            iDataResidue = hostDataLength;  // calculate residue later

            OstTraceExt2(TRACE_SMASSSTORAGE_BOT1, CBULKONLYTRANSPORT_DECODECBW18,
                      "deviceDataLength = iBufSize = 0x%x, DataResidue = HDL for now (read from host) =0x%x",
                      deviceDataLength, iDataResidue);

            if (deviceDataLength <= hostDataLength)  // case (11) and (12)
                {
                OstTrace0(TRACE_SMASSSTORAGE_BOT1, CBULKONLYTRANSPORT_DECODECBW19, "Case 11 or 12");
                ReadData(deviceDataLength);
                return;
                }
            if (deviceDataLength > hostDataLength) // case  (13)
                {
                OstTrace0(TRACE_SMASSSTORAGE_BOT1, CBULKONLYTRANSPORT_DECODECBW20, "Case 13");
                /**
                 * Comment following line in order to pass compliant test.
                 * As spec said in case 13:"The device may receive data up to a
                 * total of dCBWDataTransferLength."
                 * Here we choose to ignore incoming data.
                 */
                //StallEndpointAndWaitForClear(OutEndpoint); //Stall Out endpoint
                if (iReadSetUp)
                    {
                    WriteToClient(hostDataLength);
                    iReadSetUp = EFalse;
                    }
                SendCSW(iCbwTag, hostDataLength, EPhaseError);
                return;
                }
            }
        }
    else  // Host expected no data transfer
        {
        OstTrace0(TRACE_SMASSSTORAGE_BOT1, CBULKONLYTRANSPORT_DECODECBW21, "No data transfer expected");
        iDataResidue = 0;
        if (iWriteSetUp || iReadSetUp)   // case (2) and (3)
            {
            OstTrace0(TRACE_SMASSSTORAGE_BOT1, CBULKONLYTRANSPORT_DECODECBW22, "Case 2 or 3");
            SendCSW(iCbwTag, 0, EPhaseError);
            }
        else
            {
            OstTrace0(TRACE_SMASSSTORAGE_BOT1, CBULKONLYTRANSPORT_DECODECBW23, "Case 1");
            SendCSW(iCbwTag, 0, iCmdStatus);  //case (1)
            }
        }
    }


/**
Check if CBW Valid and Meaningful.

@return ETrue if CBW is Valid and Meaningful, EFalse otherwise
*/
TBool CBulkOnlyTransport::CheckCBW()
    {
    //
    // Check valid
    //

    // Check length
    if ((TUint) (iCbwBufPtr.Length()) != KCbwLength)
        {
        OstTrace1(TRACE_SMASSSTORAGE_BOT, CBULKONLYTRANSPORT_CHECKCBW, "ERROR Bad length: 0x%x != KCbwLength", iCbwBufPtr.Length());
        return EFalse;
        }

    // Check signature
    TInt i = KCbwSignatureOffset;
    if (iCbwBufPtr[i  ] != 0x55 ||         // CBW Singature from USB Bulk-Only Transport spec
        iCbwBufPtr[i+1] != 0x53 ||
        iCbwBufPtr[i+2] != 0x42 ||
        iCbwBufPtr[i+3] != 0x43)
        {
        OstTraceData(TRACE_SMASSSTORAGE_BOT, CBULKONLYTRANSPORT_CHECKCBW1,
                     "CBW ERROR: Bad signature %s", &iCbwBufPtr[i], 4);
        return EFalse;
        }

    //
    // Check meaningful
    //

    // Check reserved bits ( must be zero )
    if ((iCbwBufPtr[KCbwLunOffset] & 0xF0) || (iCbwBufPtr[KCbwCbLengthOffset] & 0xE0))
        {
        OstTrace0(TRACE_SMASSSTORAGE_BOT, CBULKONLYTRANSPORT_CHECKCBW2, "CBW ERROR: Reserved bits not zero");
        return EFalse;
        }

    // check command block length
    TInt cbwcbLength = iCbwBufPtr[KCbwCbLengthOffset] & 0x1F;
    if (cbwcbLength >KMaxCbwcbLength)
        {
        OstTrace0(TRACE_SMASSSTORAGE_BOT, CBULKONLYTRANSPORT_CHECKCBW3, "CBW ERROR: Incorrect block length");
        return EFalse;
        }

    //check LUN
    TInt8 lun = static_cast<TUint8>(iCbwBufPtr[KCbwLunOffset] & 0x0f);
    if (iMaxLun < lun)
        {
        OstTrace1(TRACE_SMASSSTORAGE_BOT, CBULKONLYTRANSPORT_CHECKCBW4, "CBW ERROR: bad lun: %d", lun);
        return EFalse;
        }

    return ETrue;
    }


/**
Initiate stalling of bulk IN endpoint.
Used when protocol wants to force host to initiate a reset recovery.
*/
void CBulkOnlyTransport::SetPermError()
    {
    iCurrentState = EPermErr;
    Activate(KErrNone);
    }


/**
Send data provided by protocol to the host

@param aLength amount of data (in bytes) to be send to host
*/
void CBulkOnlyTransport::WriteData(TRequestStatus& aStatus, TPtrC8& aDes, TUint aLength, TBool aZlpRequired)
    {
    if (IsActive())
        {
        __ASSERT_DEBUG(EFalse, User::Panic(KUsbMsSvrPncCat, EMsBulkOnlyStillActive));
        return;
        }
    WriteUsb(aStatus, aDes, aLength, aZlpRequired);
    iCurrentState = EWritingData;
    SetActive();
    }


/**
Send Command Status Wrapper to the host

@param aTag Echo of Command Block Tag sent by the host.
@param aDataResidue the difference between the amount of data expected by the
       host, and the actual amount of data processed by the device.
@param aStatus indicates the success or failure of the command.
*/
void CBulkOnlyTransport::SendCSW(TUint aTag, TUint aDataResidue, TCswStatus aStatus)
    {
    OstTraceExt2(TRACE_SMASSSTORAGE_BOT1, CBULKONLYTRANSPORT_SENDCSW,
              "CSW DataResidue = 0x%x, Status = %d", aDataResidue, aStatus);

    if (IsActive())
        {
        __ASSERT_DEBUG(EFalse, User::Panic(KUsbMsSvrPncCat, EMsBulkOnlyStillActive));
        return;
        }

    SetCswBufPtr(KCswLength);
    TInt i = KCswSingnatureOffset;
    iCswBufPtr[i  ] = 0x55;   // CSW Singature from USB Bulk-Only Transport spec
    iCswBufPtr[i+1] = 0x53;
    iCswBufPtr[i+2] = 0x42;
    iCswBufPtr[i+3] = 0x53;

    i = KCswTagOffset;

    iCswBufPtr[i  ] = static_cast<TUint8>((aTag & 0x000000FF));
    iCswBufPtr[i+1] = static_cast<TUint8>((aTag & 0x0000FF00) >> 8);
    iCswBufPtr[i+2] = static_cast<TUint8>((aTag & 0x00FF0000) >> 16);
    iCswBufPtr[i+3] = static_cast<TUint8>((aTag & 0xFF000000) >> 24);

    i = KCswDataResidueOffset;
    iCswBufPtr[i  ] = static_cast<TUint8>((aDataResidue & 0x000000FF));
    iCswBufPtr[i+1] = static_cast<TUint8>((aDataResidue & 0x0000FF00) >> 8);
    iCswBufPtr[i+2] = static_cast<TUint8>((aDataResidue & 0x00FF0000) >> 16);
    iCswBufPtr[i+3] = static_cast<TUint8>((aDataResidue & 0xFF000000) >> 24);

    iCswBufPtr[KCswStatusOffset] = static_cast<TUint8>(aStatus);

    TPtrC8 ptr(NULL, 0);
    ptr.Set((const TUint8*)iCswBufPtr.Ptr(), KCswLength);

    WriteUsb(iStatus, ptr, KCswLength);

    iCurrentState = ESendingCSW;

    SetActive();
    }


/**
Associates the transport with the protocol.  Called during initialization of the controller.

@param aProtocol reference to the protocol
*/
void CBulkOnlyTransport::RegisterProtocol(MProtocolBase& aProtocol)
    {
    iProtocol = &aProtocol;
    }


/**
Used by CControlInterface

@return reference to the controller which instantiate the CBulkOnlyTransport
*/
CUsbMassStorageController& CBulkOnlyTransport::Controller()
    {
    return iController;
    }


/**
@return the number of logical units supported by the device.
Logical Unit Numbers on the device shall be numbered contiguously starting from LUN
0 to a maximum LUN of 15 (Fh).
*/
TInt CBulkOnlyTransport::MaxLun()
    {
    return iMaxLun;
    }


void CBulkOnlyTransport::GetCommandBufPtr(TPtr8& aDes, TUint aLength) // Set pointer to buffer of specified aLength for command
    {
    aDes.Set(SetCommandBufPtr(aLength));
    }

void CBulkOnlyTransport::GetReadDataBufPtr(TPtr8& aDes) // Set pointer to buffer into which data is to be read from drive (Read10)
    {
    aDes.Set(SetDataBufPtr());
    }


void CBulkOnlyTransport::GetWriteDataBufPtr(TPtrC8& aDes) // Set pointer to buffer from which data is to be written to drive (Write10)
    {
    aDes.Set(iReadBufPtr);
    }

#ifdef MSDC_MULTITHREADED
void CBulkOnlyTransport::ProcessReadData(TAny* aAddress)
    {
    ExpireData(aAddress);
    }
#endif








