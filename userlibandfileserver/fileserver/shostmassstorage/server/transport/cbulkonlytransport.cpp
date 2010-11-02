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
#include <d32usbdi_hubdriver.h>
#include <d32usbdi.h>
#include <d32otgdi.h>
#include <d32usbdescriptors.h>
#include <d32usbtransfers.h>
#ifdef MASSSTORAGE_PUBLISHER
#include <e32property.h>
#endif
#include "botmsctypes.h"
#include "msctypes.h"
#include "mscutils.h"
#include "mtransport.h"
#include "mprotocol.h"
#include "cusbifacehandler.h"
#include "cbulkonlytransport.h"

#include "OstTraceDefinitions.h"
#ifdef OST_TRACE_COMPILER_IN_USE
#include "cbulkonlytransportTraces.h"
#endif


CBulkOnlyTransport::CBulkOnlyTransport()
:   iBulkOutCbwTd(KCbwPacketSize),
    iBulkDataTd(KResponsePacketSize),
    iBulkInCswTd(KCswPacketSize)
    {
    }

CBulkOnlyTransport::~CBulkOnlyTransport()
    {
    delete iUsbInterfaceHandler;
    UnInitialiseTransport();
    iInterface.Close();
    }

/**
Create CBulkOnlyTransport object.

@param aInterfaceId The interface ID of the device

@return CBulkOnlyTransport* Pointer to the created object
*/
CBulkOnlyTransport* CBulkOnlyTransport::NewL(TUint aInterfaceId)
    {
    CBulkOnlyTransport* transport = new (ELeave) CBulkOnlyTransport();
    CleanupStack::PushL(transport);
    transport->ConstructL(aInterfaceId);
    CleanupStack::Pop(transport);
    return transport;
    }


void CBulkOnlyTransport::ConstructL(TUint aInterfaceId)
    {
    User::LeaveIfError(iInterface.Open(aInterfaceId));
    iUsbInterfaceHandler = CUsbInterfaceHandler::NewL(iInterface, iBulkPipeIn);
    InitialiseTransport();
    }


void CBulkOnlyTransport::Resume()
    {
    OstTrace0(TRACE_SHOSTMASSSTORAGE_BOT, CBULKONLYTRANSPORT_10, "BOT RESUME");
    iInterface.CancelPermitSuspend();
    }

void CBulkOnlyTransport::Suspend(TRequestStatus& aStatus)
    {
    OstTrace0(TRACE_SHOSTMASSSTORAGE_BOT, CBULKONLYTRANSPORT_11, "BOT SUSPEND");
//  iInterface.PermitRemoteWakeup(ETrue);
    iInterface.PermitSuspendAndWaitForResume(aStatus);
    }

TInt CBulkOnlyTransport::InitialiseTransport()
    {
    TInt BulkOutEpAddr;
    TInt BulkInEpAddr;
    const TUint8 KEpDirectionIn = 0x80;
    const TUint8 KEpDirectionOut = 0x00;
    const TUint8 KTransferTypeBulk = 0x02;

    GetEndpointAddress(iInterface,0,KTransferTypeBulk,KEpDirectionOut,BulkOutEpAddr);
    GetEndpointAddress(iInterface,0,KTransferTypeBulk,KEpDirectionIn,BulkInEpAddr);
    iInterface.OpenPipeForEndpoint(iBulkPipeOut, BulkOutEpAddr, EFalse);
    iInterface.OpenPipeForEndpoint(iBulkPipeIn, BulkInEpAddr, EFalse);

    if (iInterface.RegisterTransferDescriptor(iBulkOutCbwTd) != KErrNone ||
            iInterface.RegisterTransferDescriptor(iBulkDataTd) != KErrNone ||
                    iInterface.RegisterTransferDescriptor(iBulkInCswTd) != KErrNone)
        {
        return KErrGeneral;
        }

    if (iInterface.InitialiseTransferDescriptors() != KErrNone)
        {
        return KErrGeneral;
        }
    return KErrNone;
    }


void CBulkOnlyTransport::UnInitialiseTransport()
    {
    iBulkPipeOut.Close();
    iBulkPipeIn.Close();
    }


TInt CBulkOnlyTransport::GetEndpointAddress(RUsbInterface& aUsbInterface,
                                            TInt aInterfaceSetting,
                                            TUint8 aTransferType,
                                            TUint8 aDirection,
                                            TInt& aEndpointAddress)
    {
    TUsbInterfaceDescriptor alternateInterfaceDescriptor;

    if (aUsbInterface.GetAlternateInterfaceDescriptor(aInterfaceSetting, alternateInterfaceDescriptor))
        {
        OstTrace1(TRACE_SHOSTMASSSTORAGE_BOT, CBULKONLYTRANSPORT_20,
                  "GetEndpointAddress : <Error> Unable to get alternate interface (%x) descriptor", aInterfaceSetting);
        return KErrGeneral;
        }

    TUsbGenericDescriptor* descriptor = alternateInterfaceDescriptor.iFirstChild;

    while (descriptor)
        {
        TUsbEndpointDescriptor* endpoint = TUsbEndpointDescriptor::Cast(descriptor);
        if (endpoint)
            {
            if ((endpoint->Attributes() & aTransferType) == aTransferType)
                {
                // Found the endpoint address
                if ( (endpoint->EndpointAddress() & 0x80) == aDirection)
                    {
                    aEndpointAddress = endpoint->EndpointAddress();
                    OstTrace0(TRACE_SHOSTMASSSTORAGE_BOT, CBULKONLYTRANSPORT_21,
                              "GetEndpointAddress : Endpoint address found");
                    return KErrNone;
                    }
                }
            }
        descriptor = descriptor->iNextPeer;
        }

    // Unable to find the endpoint address
    OstTrace0(TRACE_SHOSTMASSSTORAGE_BOT, CBULKONLYTRANSPORT_22,
              "GetEndpointAddress : Unable to find endpoint address matching the specified attributes");
    return KErrNotFound;
    }

void CBulkOnlyTransport::GetMaxLun(TLun* aMaxLun, const RMessage2& aMessage)
    {
    iUsbInterfaceHandler->GetMaxLun(aMaxLun, aMessage);
    }

TInt CBulkOnlyTransport::Reset()
    {
    RUsbInterface::TUsbTransferRequestDetails reqDetails;
    _LIT8(KNullDesC8,"");

    reqDetails.iRequestType = 0x21;
    reqDetails.iRequest = 0xFF;
    reqDetails.iValue = 0x0000;
    reqDetails.iIndex = 0x0000;
    reqDetails.iFlags = 0x04;       // Short transfer OK

    iInterface.Ep0Transfer(reqDetails, KNullDesC8, (TDes8 &) KNullDesC8, iStatus);
    User::WaitForRequest(iStatus);
    OstTrace1(TRACE_SHOSTMASSSTORAGE_BOT, CBULKONLYTRANSPORT_30,
              "BOT RESET[%d]", iStatus.Int());

    if (iStatus.Int() != KErrNone)
        {
        return KErrGeneral;
        }

    return KErrNone;
    }

void CBulkOnlyTransport::DoResetRecovery()
    {
    OstTrace0(TRACE_SHOSTMASSSTORAGE_BOT, CBULKONLYTRANSPORT_31,
              "BOT RESET RECOVERY");
#ifdef MASSSTORAGE_PUBLISHER
    TMsPublisher publisher(TMsPublisher::KBotResetProperty);
#endif
    Reset();
    iBulkPipeIn.ClearRemoteStall();
    iBulkPipeOut.ClearRemoteStall();
    }


void CBulkOnlyTransport::SendCbwL(const MClientCommandServiceReq* aReq,
                                  TBotCbw::TCbwDirection aDirection,
                                  TUint32 aTransferLength)
    {
    OstTrace1(TRACE_SHOSTMASSSTORAGE_BOT, CBULKONLYTRANSPORT_40,
              "Cbw Tag=0x%x", iCbwTag);

    iCbw.SetTag(iCbwTag++);
    iCbw.SetDataTransferLength(aTransferLength);
    iCbw.SetDataTransferDirection(aDirection);
    iCbw.SetLun(iLun);

    TPtr8 buf = iBulkOutCbwTd.WritableBuffer();
    iCbw.EncodeL(buf, aReq);

    iBulkOutCbwTd.SaveData(buf.Length());
    iBulkPipeOut.Transfer(iBulkOutCbwTd, iStatus);
    User::WaitForRequest(iStatus);

    TInt r = iStatus.Int();
    if (r != KErrNone)
        {
        if (r == KErrUsbStalled)
            {
            OstTrace0(TRACE_SHOSTMASSSTORAGE_BOT, CBULKONLYTRANSPORT_41,
                      "Cbw: BulkOut stalled");
            DoResetRecovery();
            }
        OstTrace1(TRACE_SHOSTMASSSTORAGE_BOT, CBULKONLYTRANSPORT_42,
                  "Usb transfer error %d", r);
        User::Leave(KErrGeneral);
        }
    }


void CBulkOnlyTransport::ReceiveCswL()
    {
    iBulkInCswTd.SaveData(KCswPacketSize);
    iBulkPipeIn.Transfer(iBulkInCswTd, iStatus);
    User::WaitForRequest(iStatus);

    TInt r = iStatus.Int();
    if (r != KErrNone)
        {
        if (r == KErrUsbStalled)
            {
            OstTrace0(TRACE_SHOSTMASSSTORAGE_BOT, CBULKONLYTRANSPORT_50,
                      "Csw: Clearing BulkIn stall");
            iBulkPipeIn.ClearRemoteStall();
            iBulkInCswTd.SaveData(KCswPacketSize);
            iBulkPipeIn.Transfer(iBulkInCswTd, iStatus);
            User::WaitForRequest(iStatus);
#ifdef MASSSTORAGE_PUBLISHER
            TMsPublisher publisher(TMsPublisher::KStallProperty);
#endif
            r = iStatus.Int();
            if (r == KErrUsbStalled)
                {
                OstTrace0(TRACE_SHOSTMASSSTORAGE_BOT, CBULKONLYTRANSPORT_51,
                          "Csw: BulkIn stalled");
                DoResetRecovery();
                }
            }
        // Handle other usb error and retry failures
        if (r != KErrNone)
            {
            OstTrace1(TRACE_SHOSTMASSSTORAGE_BOT, CBULKONLYTRANSPORT_52,
                      "Usb transfer error %d", r);
            User::Leave(KErrGeneral);
            }
        }

    TPtrC8 data = iBulkInCswTd.Buffer();
    r = iCsw.Decode(data);
    if (r != KErrNone)
        {
        OstTrace0(TRACE_SHOSTMASSSTORAGE_BOT, CBULKONLYTRANSPORT_53,
                  "Csw: Invalid");
        DoResetRecovery();
        User::Leave(KErrGeneral);
        }
    }


TInt CBulkOnlyTransport::SendControlCmdL(const MClientCommandServiceReq* aCommand)
    {
    SendCbwL(aCommand, TBotCbw::EDataOut, 0);
    ReceiveCswL();
    return ProcessZeroTransferL();
    }


TInt CBulkOnlyTransport::SendControlCmdL(const MClientCommandServiceReq* aCommand,
                                         MClientCommandServiceResp* aResp)
    {
    SendCbwL(aCommand, TBotCbw::EDataIn, aResp->DataLength());

    if (aResp->DataLength() > KResponsePacketSize)
        {
        OstTrace0(TRACE_SHOSTMASSSTORAGE_BOT, CBULKONLYTRANSPORT_60,
                  "Control command response length not supported");
        User::Leave(KErrGeneral);
        }

    iBulkDataTd.SaveData(KResponsePacketSize);
    iBulkPipeIn.Transfer(iBulkDataTd, iStatus);
    User::WaitForRequest(iStatus);
    TInt r = iStatus.Int();
    if (r != KErrNone)
        {
        if (r != KErrUsbStalled)
            {
            OstTrace1(TRACE_SHOSTMASSSTORAGE_BOT, CBULKONLYTRANSPORT_61,
                      "Usb transfer error %d",r);
            User::Leave(KErrGeneral);
            }
        OstTrace0(TRACE_SHOSTMASSSTORAGE_BOT, CBULKONLYTRANSPORT_62,
                  "SendControlCmdL ClearRemoteStall");
        iBulkPipeIn.ClearRemoteStall();
        ReceiveCswL();
        return KErrCommandStalled;
        }
    TPtrC8 data = iBulkDataTd.Buffer();

    ReceiveCswL();

    TUint32 dataReceived = 0;
    r = ProcessInTransferL(dataReceived);
    if (!r)
        {
        TRAP(r, aResp->DecodeL(data));
        if (dataReceived == 0)
            {

            // Some devices are found not to support DataResidue and return zero
            // bytes received. This state is NOT treated as an error condition
            // and we assume that all bytes are valid. For SCSI command set
            // applicable to Mass Storage a device must always return data so
            // the zero byte condition is not possible with compliant devices.
            //
            // List of known non-compliant devices:
            // 1 Transcend JetFlashV30 VendorID=JetFlash ProductID=TS#GJFV30
            // (# is device size in G)

            OstTrace0(TRACE_SHOSTMASSSTORAGE_BOT, CBULKONLYTRANSPORT_63,
                      "Warning: No data received");
            }
        }
    return r;
    }


TInt CBulkOnlyTransport::SendDataRxCmdL(const MClientCommandServiceReq* aCommand,
                                        TDes8& aCopyBuf,
                                        TInt& aLen)
    {
    TInt r = KErrNone;
    SendCbwL(aCommand, TBotCbw::EDataIn, aLen);

    // store initial length as data is appended to the buffer
    TInt startPos = aCopyBuf.Length();

    TInt len = aLen;

    while (len)
        {
        if(len > KResponsePacketSize)
            iBulkDataTd.SaveData(KResponsePacketSize);
        else
            iBulkDataTd.SaveData(len);
        iBulkPipeIn.Transfer(iBulkDataTd, iStatus);
        User::WaitForRequest(iStatus);

        r = iStatus.Int();
        if (r != KErrNone)
            {
            if (r == KErrUsbStalled)
                {
                OstTrace0(TRACE_SHOSTMASSSTORAGE_BOT, CBULKONLYTRANSPORT_70,
                          "SendDataRxCmdL ClearRemoteStall");
                iBulkPipeIn.ClearRemoteStall();
#ifdef MASSSTORAGE_PUBLISHER
                TMsPublisher publisher(TMsPublisher::KStallProperty);
#endif
                break;
                }
            DoResetRecovery();
            OstTrace1(TRACE_SHOSTMASSSTORAGE_BOT, CBULKONLYTRANSPORT_71,
                      "Usb transfer error %d", r);
            User::Leave(KErrGeneral);
            }

        TPtrC8 data = iBulkDataTd.Buffer();
        aCopyBuf.Append(data.Ptr(), data.Length());
        if(len > KResponsePacketSize)
            len -= KResponsePacketSize;
        else
            len = 0;
        }

    ReceiveCswL();
    TUint32 lenReceived = 0;

    r = ProcessInTransferL(lenReceived);
    aLen = lenReceived;
    aCopyBuf.SetLength(startPos + lenReceived);

    return r;
    }

TInt CBulkOnlyTransport::SendDataTxCmdL(const MClientCommandServiceReq* aCommand,
                                        TDesC8& aData,
                                        TUint aPos,
                                        TInt& aLen)
    {
    TInt r = KErrNone;

    SendCbwL(aCommand, TBotCbw::EDataOut, aLen);

    TInt len = aLen;
    TInt length = 0;
    iBulkDataTd.SetZlpStatus(RUsbTransferDescriptor::ESuppressZlp);
    while (len)
        {
        TPtr8 senddata = iBulkDataTd.WritableBuffer();
        senddata.Append(aData.Ptr() + length + aPos, len > KResponsePacketSize? KResponsePacketSize : len);

        iBulkDataTd.SaveData(senddata.Length());
        iBulkPipeOut.Transfer(iBulkDataTd, iStatus);
        User::WaitForRequest(iStatus);

        if (iStatus.Int() != KErrNone)
            {
            if (iStatus.Int() == KErrUsbStalled)
                {
                OstTrace0(TRACE_SHOSTMASSSTORAGE_BOT, CBULKONLYTRANSPORT_80,
                          "SendDataTxCmdL ClearRemoteStall");
                iBulkPipeOut.ClearRemoteStall();
#ifdef MASSSTORAGE_PUBLISHER
                TMsPublisher publisher(TMsPublisher::KStallProperty);
#endif
                break;
                }
            DoResetRecovery();
            OstTrace1(TRACE_SHOSTMASSSTORAGE_BOT, CBULKONLYTRANSPORT_81,
                      "Usb transfer error %d", r);
            User::Leave(KErrGeneral);
            }

        if(len > KResponsePacketSize)
            {
            len -= KResponsePacketSize;
            length += KResponsePacketSize;
            }
        else
            {
            length += len;
            len = 0;
            }
        }

    ReceiveCswL();

    TUint32 lenSent = 0;
    r = ProcessOutTransferL(lenSent);
    aLen = lenSent;

    return r;
    }


TInt CBulkOnlyTransport::ProcessZeroTransferL()
    {
    // process 13 cases

    __ASSERT_DEBUG(iCbw.iDirection == TBotCbw::EDataOut, User::Invariant());
    __ASSERT_DEBUG(iCbw.iDataTransferLength == 0, User::Invariant());

    // Hn - Host expects no data transfers
    if (!iCsw.IsValidCsw(iCbw.iTag))
        {
        OstTrace0(TRACE_SHOSTMASSSTORAGE_BOT, CBULKONLYTRANSPORT_90,
                  "BOT CSW Invalid");
        DoResetRecovery();
        User::Leave(KErrGeneral);
        }
    if (!iCsw.IsMeaningfulCsw(iCbw.iDataTransferLength))
        {
        OstTrace0(TRACE_SHOSTMASSSTORAGE_BOT, CBULKONLYTRANSPORT_91,
                  "BOT CSW not meaningful");
        DoResetRecovery();
        User::Leave(KErrGeneral);
        }

    if (iCsw.iStatus == TBotCsw::EPhaseError)
        {
        // Case (2) or (3)
        OstTrace0(TRACE_SHOSTMASSSTORAGE_BOT, CBULKONLYTRANSPORT_92,
                  "BOT Phase Error");
        // Reset Recovery
        DoResetRecovery();
        User::Leave(KErrGeneral);
        }

    if (iCsw.iDataResidue != 0)
        {
        // should not happen
        OstTrace0(TRACE_SHOSTMASSSTORAGE_BOT, CBULKONLYTRANSPORT_93,
                  "BOT Residue is invalid!");
        // Reset Recovery
        DoResetRecovery();
        User::Leave(KErrGeneral);
        }

    // all ok if here
    return (iCsw.iStatus == TBotCsw::ECommandPassed) ? KErrNone : KErrCommandFailed;
    }


TInt CBulkOnlyTransport::ProcessInTransferL(TUint32& aDataReceived)
    {
    aDataReceived = 0;
    // process 13 cases

    __ASSERT_DEBUG(iCbw.iDirection == TBotCbw::EDataIn, User::Invariant());

    // Hi - Host expects to receive data from the device
    if (!iCsw.IsValidCsw(iCbw.iTag))
        {
        OstTrace0(TRACE_SHOSTMASSSTORAGE_BOT, CBULKONLYTRANSPORT_100,
                  "BOT CSW Invalid");
        DoResetRecovery();
        User::Leave(KErrGeneral);
        }
    if (!iCsw.IsMeaningfulCsw(iCbw.iDataTransferLength))
        {
        OstTrace0(TRACE_SHOSTMASSSTORAGE_BOT, CBULKONLYTRANSPORT_101,
                  "BOT CSW not meaningful");
        DoResetRecovery();
        User::Leave(KErrGeneral);
        }


    if (iCsw.iStatus == TBotCsw::EPhaseError)
        {
        OstTrace0(TRACE_SHOSTMASSSTORAGE_BOT, CBULKONLYTRANSPORT_102,
                  "BOT Phase Error");
        // Case (7) or (8)
        // Reset Recovery
        DoResetRecovery();
        User::Leave(KErrGeneral);
        }

    // all ok if here
    // Case (4), (5) or (6)
    aDataReceived = iCbw.iDataTransferLength - iCsw.iDataResidue;
    return (iCsw.iStatus == TBotCsw::ECommandPassed) ? KErrNone : KErrCommandFailed;
    }


TInt CBulkOnlyTransport::ProcessOutTransferL(TUint32& aDataReceived)
    {
    aDataReceived = 0;
    // process 13 cases

    __ASSERT_DEBUG(iCbw.iDirection == TBotCbw::EDataOut, User::Invariant());

    // Ho - Host expects to send data to the device
    if (!iCsw.IsValidCsw(iCbw.iTag))
        {
        OstTrace0(TRACE_SHOSTMASSSTORAGE_BOT, CBULKONLYTRANSPORT_110,
                  "BOT CSW Invalid");
        DoResetRecovery();
        User::Leave(KErrGeneral);
        }
    if (!iCsw.IsMeaningfulCsw(iCbw.iDataTransferLength))
        {
        OstTrace0(TRACE_SHOSTMASSSTORAGE_BOT, CBULKONLYTRANSPORT_111,
                  "BOT CSW not meaningful");
        DoResetRecovery();
        User::Leave(KErrGeneral);
        }

    if (iCsw.iStatus == TBotCsw::EPhaseError)
        {
        OstTrace0(TRACE_SHOSTMASSSTORAGE_BOT, CBULKONLYTRANSPORT_112,
                  "BOT Phase Error");
        // Case (10) or (13)
        // Reset Recovery
        DoResetRecovery();
        User::Leave(KErrGeneral);
        }

    // all ok if here
    // Case (9), (11) or (12)
    aDataReceived = iCbw.iDataTransferLength - iCsw.iDataResidue;
    return (iCsw.iStatus == TBotCsw::ECommandPassed) ? KErrNone : KErrCommandFailed;
    }


void CBulkOnlyTransport::SetLun(TLun aLun)
    {
    iLun = aLun;
    OstTrace1(TRACE_SHOSTMASSSTORAGE_BOT, CBULKONLYTRANSPORT_120,
              "CBulkOnlyTransport::SetLun(%d)", aLun);
    }


/**
Encode CBW into the supplied buffer. The command is also encoded using the
supplied encoding method of MClientCommandServiceReq.

@param aBuffer The buffer to copy the encoded stream in to
@param aCommand The command to be encoded into the Command Block field
*/
void TBotCbw::EncodeL(TPtr8 &aBuffer, const MClientCommandServiceReq* aCommand) const
    {
    aBuffer.SetLength(KCbwLength);

    TPtr8 commandBlock = aBuffer.MidTPtr(TBotCbw::KCbwCbOffset);

    aBuffer.FillZ();

    TInt cbLength = aCommand->EncodeRequestL(commandBlock);

    TUint8* ptr = (TUint8 *) aBuffer.Ptr();
    LittleEndian::Put32(&ptr[KCbwSignatureOffset], 0x43425355);
    LittleEndian::Put32(&ptr[KCbwTagOffset], iTag);
    LittleEndian::Put32(&ptr[KCbwDataTransferLengthOffset], iDataTransferLength);
    aBuffer[KCbwFlagOffset] = (iDirection == EDataOut) ? 0x00 : 0x80;
    aBuffer[KCbwLunOffset] = iLun;
    aBuffer[KCbwCbLengthOffset] = cbLength;
    OstTrace1(TRACE_SHOSTMASSSTORAGE_BOT, CBULKONLYTRANSPORT_121,
              "BOT TBotCbw::Encode Lun=%d", iLun);
    }


/**
Decode stream into TBotCsw.

@param aPtr A buffer containing CSW stream
*/
TInt TBotCsw::Decode(const TDesC8& aPtr)
    {
    if (aPtr.Length() != KCswLength)
        {
        OstTrace1(TRACE_SHOSTMASSSTORAGE_BOT, CBULKONLYTRANSPORT_130,
                  "Invalid CSW length %d", aPtr.Length());
        return KErrGeneral;
        }

    iSignature = LittleEndian::Get32(&aPtr[KCswSignatureOffset]);
    iTag = LittleEndian::Get32(&aPtr[KCswTagOffset]);
    iDataResidue = LittleEndian::Get32(&aPtr[KCswDataResidueOffset]);

    TInt status = aPtr[KCswStatusOffset];
    iStatus = static_cast<TCswStatus>(status);
    OstTrace1(TRACE_SHOSTMASSSTORAGE_BOT, CBULKONLYTRANSPORT_131,
              "BOT CSW Status = %d", iStatus);
    return KErrNone;
    }


/**
Checks that CSW contents are valid, USB Mass Storage Class Bulk-Only
Transport 6.3.

@param aTag The tag value received from the device

@return TBool Return ETrue if valid else EFalse
*/
TBool TBotCsw::IsValidCsw(TUint32 aTag) const
    {
    if (iSignature != 0x53425355)
        {
        return EFalse;
        }
    if (iTag != aTag)
        {
        return EFalse;
        }
    return ETrue;
    }


/**
Checks that CSW contents are meaningful, USB Mass Storage Class Bulk-Only
Transport 6.3.

@param aTransferLength Number of bytes transferred

@return TBool Return ETrue if meaningful else EFalse
*/
TBool TBotCsw::IsMeaningfulCsw(TUint32 aTransferLength) const
    {
    if (iStatus != EPhaseError)
        {
        if (iDataResidue > aTransferLength)
            {
            return EFalse;
            }
        }
    return ETrue;
    }

#ifdef MASSSTORAGE_PUBLISHER
TMsPublisher::TMsPublisher(TPropertyKeys aKey)
    {
    RProperty prop;

    const TUid KUidHostMassStorageCategory = {KMyUid};

    TInt ret = prop.Attach(KUidHostMassStorageCategory, aKey);
    OstTraceExt2(TRACE_SHOSTMASSSTORAGE_BOT, CBULKONLYTRANSPORT_140,
                 "Property Key[%d] attach ret=%d", aKey, ret);
    if (ret == KErrNone)
        {
        TInt flag;
        ret = prop.Get(KUidHostMassStorageCategory, aKey, flag);
        if (ret == KErrNone)
            {
            flag++;
            ret = prop.Set(KUidHostMassStorageCategory, aKey, flag);
            }
        OstTraceExt3(TRACE_SHOSTMASSSTORAGE_BOT, CBULKONLYTRANSPORT_141,
                     "Property Set[%d] ret=%d Counter=%d",
                     aKey, ret, flag);
        prop.Close();
        }
    }

#endif

