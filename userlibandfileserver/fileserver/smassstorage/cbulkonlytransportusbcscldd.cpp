/*
* Copyright (c) 2004-2010 Nokia Corporation and/or its subsidiary(-ies).
* All rights reserved.
* This component and the accompanying materials are made available
* under the terms of the License "Eclipse Public License v1.0"
* which accompanies this distribution, and is available
* at the URL "http://www.eclipse.org/legal/epl-v10.html".
*
* Initial Contributors:
* Nokia Corporation - initial contribution.
*
* Contributors:
*
* Description:
*
*/


/**
 @file
 @internalTechnology
*/

#include <e32std.h>
#include "mtransport.h"
#include "mprotocol.h"
#include "mldddevicestatenotification.h"

#include "drivemanager.h"
#include "cusbmassstoragecontroller.h"

#include "cbulkonlytransport.h"
#include "cbulkonlytransportusbcscldd.h"
#include "smassstorage.h"

#include "OstTraceDefinitions.h"
#ifdef OST_TRACE_COMPILER_IN_USE
#include "cbulkonlytransportusbcsclddTraces.h"
#endif



//This value defined in USB Mass Storage Bulk Only Transrt spec and not supposed to be changed
static const TInt KRequiredNumberOfEndpoints = 2; // in addition to endpoint 0.

static const TInt KUsbNumInterfacesOffset = 4;

#ifdef MSDC_MULTITHREADED
//Only used in DB. The first 2K of KMaxScBufferSize for sending CSW
static const TUint32 KCswBufferSize = 2 * 1024;
#endif

////////////////////////////////////
/**
Called by CBulkOnlyTransportUsbcScLdd to create an instance of CControlInterfaceUsbcScLdd

@param aParent reference to the CBulkOnlyTransportUsbcScLdd
*/


CControlInterfaceUsbcScLdd* CControlInterfaceUsbcScLdd::NewL(CBulkOnlyTransportUsbcScLdd& aParent)
    {
    CControlInterfaceUsbcScLdd* self = new(ELeave) CControlInterfaceUsbcScLdd(aParent);
    CleanupStack::PushL(self);
    self->ConstructL();
    CActiveScheduler::Add(self);
    CleanupStack::Pop();
    return self;
    }


void CControlInterfaceUsbcScLdd::ConstructL()
    {
    }


/**
c'tor

@param aParent reference to the CBulkOnlyTransportUsbcScLdd
*/
CControlInterfaceUsbcScLdd::CControlInterfaceUsbcScLdd(CBulkOnlyTransportUsbcScLdd& aParent)
    :CActive(EPriorityStandard),
     iParent(aParent),
     iCurrentState(ENone)
    {
    }


/**
d'tor
*/
CControlInterfaceUsbcScLdd::~CControlInterfaceUsbcScLdd()
    {
    Cancel();
    iEp0Buf.Close();
    }

TInt CControlInterfaceUsbcScLdd::OpenEp0()
    {
    TInt res = iParent.Ldd().OpenEndpoint(iEp0Buf,0);
    return res;
    }

/**
Called by CBulkOnlyTransport HwStart to start control interface
*/
TInt CControlInterfaceUsbcScLdd::Start()
    {
    TInt res = ReadEp0Data();
    return (res);
    }


/**
Called by desctructor of CBulkOnlyTransportUsbcScLdd to stop control interface
*/
void CControlInterfaceUsbcScLdd::Stop()
    {
    if (!IsActive())
        {
        return;
        }
    iCurrentState = ENone;
    Cancel();
    }


/**
Cancel outstanding request (if any)
*/
void CControlInterfaceUsbcScLdd::DoCancel()
    {
    switch(iCurrentState)
        {
        case EReadEp0Data:
            iParent.Ldd().ReadCancel(KUsbcScEndpointZero);
            break;
        case ESendMaxLun:
            iParent.Ldd().WriteCancel(KUsbcScEndpointZero);
            break;
        default:
            __ASSERT_DEBUG(EFalse, User::Panic(KUsbMsSvrPncCat, EMsControlInterfaceBadState));
        }
    }


/**
Implement CControlInterfaceUsbcScLdd state machine
*/
void CControlInterfaceUsbcScLdd::RunL()
    {
    if (iStatus != KErrNone)
        {
        OstTrace1(TRACE_SMASSSTORAGE_BOT, CCONTROLINTERFACEUSBCSCLDD,
                  "ERROR %d in RunL", iStatus.Int());
        //read EP0  again
        ReadEp0Data();
        return;
        }

    ReadEp0Data();
    }

/**
Actual Read to RDevUsbcScClient BIL
*/
TInt CControlInterfaceUsbcScLdd::ReadUsbEp0()
    {
    iCurrentState = EReadEp0Data;
    iStatus = KRequestPending;
    return iEp0Buf.GetBuffer (iEp0Packet,iEp0Size,iEp0Zlp,iStatus);
    }


/**
Post a read request to EEndpoint0 to read request header
*/
TInt CControlInterfaceUsbcScLdd::ReadEp0Data()
    {
    if (IsActive())
        {
        OstTrace0(TRACE_SMASSSTORAGE_BOT, READEP0DATA_SC, "Still active");
        return KErrServerBusy;
        }

    TInt r = KErrNone;
    do
        {
        r = ReadUsbEp0();
        if (r == KErrCompletion)
            {
            iStatus = KErrNone;
            DecodeEp0Data();
            }
        else if (r == KErrNone)
            {
            SetActive();
            }
        } while (((r == KErrCompletion) || (r == TEndpointBuffer::KStateChange)) && (!IsActive()));
    return KErrNone;
    }


/**
Decode request header and do appropriate action - get max LUN info or post a reset request
*/
void CControlInterfaceUsbcScLdd::DecodeEp0Data()
    {
    if (IsActive())
        {
        __ASSERT_DEBUG(EFalse, User::Panic(KUsbMsSvrPncCat, EMsControlInterfaceStillActive));
        return;
        }

    TPtrC8 ep0ReadDataPtr((TUint8*)iEp0Packet, iEp0Size);
    TInt err = iRequestHeader.Decode(ep0ReadDataPtr);

    if(err != KErrNone)
        return;

    switch(iRequestHeader.iRequest)
        {
        //
        // GET MAX LUN (0xFE)
        //
        case TUsbRequestHdr::EReqGetMaxLun:
            {
            OstTrace1(TRACE_SMASSSTORAGE_BOT, DECODEEP0DATA_SC, ">>> EP0 GetMaxLun = %d", iParent.MaxLun());

            if (   iRequestHeader.iRequestType != 0xA1 //value from USB MS BOT spec
                || iRequestHeader.iIndex > 15
                || iRequestHeader.iValue != 0
                || iRequestHeader.iLength != 1)
                {
                OstTrace0(TRACE_SMASSSTORAGE_BOT, DECODEEP0DATA1_SC, "ERROR: GetMaxLun command packet check error");
                iParent.Ldd().EndpointZeroRequestError();
                break;
                }

            TPtr8* ep0WriteDataPtr = NULL;
            TUint ep0Length;
            iEp0Buf.GetInBufferRange(((TAny*&)ep0WriteDataPtr),ep0Length);
            ep0WriteDataPtr->SetLength(1);  //Return only 1 byte to host
            ep0WriteDataPtr->Fill(0);
            ep0WriteDataPtr->Fill(iParent.MaxLun());    // Supported Units
            TInt length = ep0WriteDataPtr->Length();
            err = iEp0Buf.WriteBuffer((TPtr8*)(ep0WriteDataPtr->Ptr()),length,ETrue,iStatus);

            iCurrentState = ESendMaxLun;
            SetActive();

            return;
            }
        //
        // RESET (0xFF)
        //
        case TUsbRequestHdr::EReqReset:
            {
            OstTrace0(TRACE_SMASSSTORAGE_BOT, DECODEEP0DATA2_SC, ">>> EP0 BulkOnlyMassStorageReset");

            if (   iRequestHeader.iRequestType != 0x21 //value from USB MS BOT spec
                || iRequestHeader.iIndex > 15
                || iRequestHeader.iValue != 0
                || iRequestHeader.iLength != 0)
                {
                OstTrace0(TRACE_SMASSSTORAGE_BOT, DECODEEP0DATA3_SC, "BulkOnlyMassStorageReset command packet check error");
                iParent.Ldd().EndpointZeroRequestError();
                break;
                }

            iParent.HwStop();
            iParent.Controller().Reset();
            iParent.HwStart(ETrue);

            err = iParent.Ldd().SendEp0StatusPacket();
            return;
            }
        //
        // Unknown?
        //
        default:
            {
            OstTrace0(TRACE_SMASSSTORAGE_BOT, DECODEEP0DATA4_SC, ">>> EP0 DecodeEp0Data : Unknown Request");
            }
        }
        ReadEp0Data();  //try to get another request
    }


//
// --- class CBulkOnlyTransportUsbcScLdd ---------------------------------------------------------
//

CBulkOnlyTransportUsbcScLdd::CBulkOnlyTransportUsbcScLdd(TInt aNumDrives,CUsbMassStorageController& aController)
    :CBulkOnlyTransport(aNumDrives, aController)
    {
    }

/**
Constructs the CBulkOnlyTransportUsbcScLdd object
*/
void CBulkOnlyTransportUsbcScLdd::ConstructL()
    {
    iControlInterface = CControlInterfaceUsbcScLdd::NewL(*this);
    iDeviceStateNotifier = CActiveDeviceStateNotifierBase::NewL(*this, *this);
    iChunk = new RChunk();
    CActiveScheduler::Add(this);
    }


CBulkOnlyTransportUsbcScLdd::~CBulkOnlyTransportUsbcScLdd()
    {
    if (iInterfaceConfigured)
        {
        Stop();
        TInt err = iSCReadEndpointBuf.Close();
        err = iSCWriteEndpointBuf.Close();
        delete iControlInterface ;
        delete iDeviceStateNotifier;
        delete iChunk;
        }
    }

RDevUsbcScClient& CBulkOnlyTransportUsbcScLdd::Ldd()
    {
    return iLdd;
    }

/**
Set or unset configuration descriptor for USB MassStorage Bulk Only transport

@param aUnset indicate whether set or unset descriptor
@return KErrNone if operation was completed successfully, errorcode otherwise
*/
TInt CBulkOnlyTransportUsbcScLdd::SetupConfigurationDescriptor(TBool aUnset)
    {
    TInt ret(KErrNone);

    if ((ret = iLdd.Open(0)) != KErrNone)
        return ret;

    TInt configDescriptorSize(0);
    iLdd.GetConfigurationDescriptorSize(configDescriptorSize);
    if (static_cast<TUint>(configDescriptorSize) != KUsbDescSize_Config)
        {
        return KErrCorrupt;
        }

    TBuf8<KUsbDescSize_Config> configDescriptor;
    ret = iLdd.GetConfigurationDescriptor(configDescriptor);
    if (ret != KErrNone)
        {
        return ret;
        }

    // I beleive that other fields setted up during LDD initialisation
    if (aUnset)
        {
        --configDescriptor[KUsbNumInterfacesOffset];
        }
    else
        {
        ++configDescriptor[KUsbNumInterfacesOffset];
        }
    ret = iLdd.SetConfigurationDescriptor(configDescriptor);

    if (aUnset)
        {
        iLdd.Close();
        }

    return ret;
    }

/**
Set up interface descriptor

@return KErrNone if operation was completed successfully, errorcode otherwise
*/
TInt CBulkOnlyTransportUsbcScLdd::SetupInterfaceDescriptors()
    {
    // Device caps
    TUsbDeviceCaps d_caps;
    TInt ret = iLdd.DeviceCaps(d_caps);
    if (ret != KErrNone)
        {
        return ret;
        }
    TInt totalEndpoints = d_caps().iTotalEndpoints;
    if (totalEndpoints  < KRequiredNumberOfEndpoints)
        {
        return KErrHardwareNotAvailable;
        }

    // Endpoint caps
    TUsbcEndpointData data[KUsbcMaxEndpoints];
    TPtr8 dataptr(reinterpret_cast<TUint8*>(data), sizeof(data), sizeof(data));
    ret = iLdd.EndpointCaps(dataptr);
    if (ret != KErrNone)
        {
        return ret;
        }

    // Set the active interface
    TUsbcScInterfaceInfoBuf ifc;
    TInt ep_found = 0;
    TBool foundBulkIN = EFalse;
    TBool foundBulkOUT = EFalse;

    for (TInt i = 0; i < totalEndpoints ; i++)
        {
        const TUsbcEndpointCaps* caps = &data[i].iCaps;
        const TInt maxPacketSize = caps->MaxPacketSize();
        if (!foundBulkIN &&
            (caps->iTypesAndDir & (KUsbEpTypeBulk | KUsbEpDirIn)) == (KUsbEpTypeBulk | KUsbEpDirIn))
            {
            // InEndpoint is going to be our TX (IN, write) endpoint
            ifc().iEndpointData[0].iType = KUsbEpTypeBulk;
            if((d_caps().iFeatureWord1 & KUsbDevCapsFeatureWord1_EndpointResourceAllocV2) == KUsbDevCapsFeatureWord1_EndpointResourceAllocV2)
                ifc().iEndpointData[0].iFeatureWord1  = KUsbcEndpointInfoFeatureWord1_DMA|KUsbcEndpointInfoFeatureWord1_DoubleBuffering;
            ifc().iEndpointData[0].iDir  = KUsbEpDirIn;
            ifc().iEndpointData[0].iSize = maxPacketSize;
            ifc().iEndpointData[0].iInterval_Hs = 0;
            ifc().iEndpointData[0].iBufferSize = KMaxScBufferSize;
            foundBulkIN = ETrue;
            if (++ep_found == KRequiredNumberOfEndpoints)
                {
                break;
                }
            continue;
            }
        if (!foundBulkOUT &&
            (caps->iTypesAndDir & (KUsbEpTypeBulk | KUsbEpDirOut)) == (KUsbEpTypeBulk | KUsbEpDirOut))
            {
            // OutEndpoint is going to be our RX (OUT, read) endpoint
            ifc().iEndpointData[1].iType = KUsbEpTypeBulk;
            if((d_caps().iFeatureWord1 & KUsbDevCapsFeatureWord1_EndpointResourceAllocV2) == KUsbDevCapsFeatureWord1_EndpointResourceAllocV2)
                ifc().iEndpointData[1].iFeatureWord1  = KUsbcEndpointInfoFeatureWord1_DMA|KUsbcEndpointInfoFeatureWord1_DoubleBuffering;
            ifc().iEndpointData[1].iDir  = KUsbEpDirOut;
            ifc().iEndpointData[1].iSize = maxPacketSize;
            ifc().iEndpointData[1].iInterval_Hs = 0;
            ifc().iEndpointData[1].iBufferSize = KMaxScBufferSize;
            ifc().iEndpointData[1].iReadSize = KMaxScReadSize;

            foundBulkOUT = ETrue;
            if (++ep_found == KRequiredNumberOfEndpoints)
                {
                break;
                }
            continue;
            }
        }
    if (ep_found != KRequiredNumberOfEndpoints)
        {
        return KErrHardwareNotAvailable;
        }

    _LIT16(string, "USB Mass Storage Interface");
    ifc().iString = const_cast<TDesC16*>(&string);
    ifc().iTotalEndpointsUsed = KRequiredNumberOfEndpoints;
    ifc().iClass.iClassNum    = 0x08;   // Mass Storage
    ifc().iClass.iSubClassNum = 0x06;   // SCSI Transparent Command Set
    ifc().iClass.iProtocolNum = 0x50;   // Bulk Only Transport

    if (d_caps().iHighSpeed)
        {
        // Tell the Protocol about it, because it might want to do some
        // optimizing too.
        iProtocol->ReportHighSpeedDevice();
        }

    if ((ret = iLdd.SetInterface(0, ifc)) == KErrNone)
        {
        return (iLdd.FinalizeInterface(iChunk));
        }
    return ret;
    }

void CBulkOnlyTransportUsbcScLdd::ReleaseInterface()
    {
    iLdd.ReleaseInterface(0);
    }


TInt CBulkOnlyTransportUsbcScLdd::StartControlInterface()
    {
    return iControlInterface->Start();
    }

void CBulkOnlyTransportUsbcScLdd::CancelControlInterface()
    {
    iControlInterface->Cancel();
    }

void CBulkOnlyTransportUsbcScLdd::ActivateDeviceStateNotifier()
    {
    iDeviceStateNotifier->Activate();
    }

void CBulkOnlyTransportUsbcScLdd::CancelDeviceStateNotifier()
    {
    iDeviceStateNotifier->Cancel();
    }


void CBulkOnlyTransportUsbcScLdd::CancelReadWriteRequests()
    {
    TUsbcScChunkHeader chunkHeader(*iChunk);
    for (TInt i = 0; i < chunkHeader.iAltSettings->iNumOfAltSettings; i++)
        {
        TInt8* endpoint = (TInt8*) (chunkHeader.iAltSettings->iAltTableOffset[i] + (TInt) iChunk->Base());
        TInt numOfEps = chunkHeader.GetNumberOfEndpoints(i);
        __ASSERT_DEBUG(numOfEps >= 0, User::Invariant());
        for (TInt j = 1; j <= numOfEps; j++)
            {
            TUsbcScHdrEndpointRecord* endpointInf = (TUsbcScHdrEndpointRecord*) &(endpoint[j * chunkHeader.iAltSettings->iEpRecordSize]);
            if (endpointInf->Direction() == KUsbScHdrEpDirectionOut)
                {
                iLdd.ReadCancel(endpointInf->iBufferNo);
                }
            if (endpointInf->Direction() == KUsbScHdrEpDirectionIn)
                {
                iLdd.WriteCancel(endpointInf->iBufferNo);
                }
            }
        }
    }

void CBulkOnlyTransportUsbcScLdd::AllocateEndpointResources()
    {
    TUsbcScChunkHeader chunkHeader(*iChunk);
    for (TInt i = 0; i < chunkHeader.iAltSettings->iNumOfAltSettings; i++)
        {
        TInt8* endpoint = (TInt8*) (chunkHeader.iAltSettings->iAltTableOffset[i] + (TInt) iChunk->Base());

        for (TInt j = 1; j <= chunkHeader.GetNumberOfEndpoints(i); j++)
            {
            TUsbcScHdrEndpointRecord* endpointInf = (TUsbcScHdrEndpointRecord*) &(endpoint[j * chunkHeader.iAltSettings->iEpRecordSize]);
            if (endpointInf->Direction() == KUsbScHdrEpDirectionOut)
                {
                iOutEndpoint = j;
                }
            if (endpointInf->Direction() == KUsbScHdrEpDirectionIn)
                {
                iInEndpoint = j;
                }
            }
        }

    TUsbDeviceCaps d_caps;
    TInt err;
    TInt ret = iLdd.DeviceCaps(d_caps);
    if (ret == KErrNone)
        {
        if((d_caps().iFeatureWord1 & KUsbDevCapsFeatureWord1_EndpointResourceAllocV2) != KUsbDevCapsFeatureWord1_EndpointResourceAllocV2)
            {
            // Set up DMA if possible (errors are non-critical)
            err = iLdd.AllocateEndpointResource(iOutEndpoint, EUsbcEndpointResourceDMA);
            if (err != KErrNone)
                {
                OstTrace1(TRACE_SMASSSTORAGE_BOT, A1_SC, "Set DMA on OUT endpoint failed with error code: %d", err);
                }
            err = iLdd.AllocateEndpointResource(iInEndpoint, EUsbcEndpointResourceDMA);
            if (err != KErrNone)
                {
                OstTrace1(TRACE_SMASSSTORAGE_BOT, A2_SC, "Set DMA on IN endpoint failed with error code: %d", err);
                }

                // Set up Double Buffering if possible (errors are non-critical)
            err = iLdd.AllocateEndpointResource(iOutEndpoint, EUsbcEndpointResourceDoubleBuffering);
            if (err != KErrNone)
                {
                OstTrace1(TRACE_SMASSSTORAGE_BOT, A3_SC, "Set Double Buffering on OUT endpoint failed with error code: %d", err);
                }
            err = iLdd.AllocateEndpointResource(iInEndpoint, EUsbcEndpointResourceDoubleBuffering);
                if (err != KErrNone)
                {
                OstTrace1(TRACE_SMASSSTORAGE_BOT, A4_SC, "Set Double Buffering on IN endpoint failed with error code: %d", err);
                }
            }
        }

    err = OpenEndpoints();
    }

TInt CBulkOnlyTransportUsbcScLdd::OpenEndpoints()
    {
    TInt res = iLdd.OpenEndpoint(iSCReadEndpointBuf, iOutEndpoint);
    if (res == KErrNone)
        {
        res = iLdd.OpenEndpoint(iSCWriteEndpointBuf, iInEndpoint);
        if (res == KErrNone)
            {
            iSCWriteEndpointBuf.GetInBufferRange((TAny*&)iDataPtr, iInBufferLength);
            res = iControlInterface->OpenEp0();
            }
        return res;
        }

    return res;
    }


TInt CBulkOnlyTransportUsbcScLdd::GetDeviceStatus(TUsbcDeviceState& deviceStatus)
    {
    return iLdd.DeviceStatus(deviceStatus);
    }

void CBulkOnlyTransportUsbcScLdd::FlushData()
    {
    // Intentionally Left Blank Do Nothing
    }

/**
 * Read out rest data from OutEndpoint and discard them
 */
void CBulkOnlyTransportUsbcScLdd::ReadAndDiscardData(TInt /*aBytes*/)
    {
    TRequestStatus status;
    TInt r = KErrNone; // Lets assume there is no data
    do
        {
        iSCReadSize = 0;
        status = KErrNone;
        r = iSCReadEndpointBuf.TakeBuffer(iSCReadData,iSCReadSize,iReadZlp,status);
        iSCReadEndpointBuf.Expire();
        }
    while (r == KErrCompletion);

    if (r == KErrNone)
        {
        TRequestStatus* stat = &status;
        User::RequestComplete(stat, KErrNone);
        }
    }

/**
Called by the protocol to determine how many bytes of data are available in the read buffer.

@return The number of bytes available in the read buffer
*/
TInt CBulkOnlyTransportUsbcScLdd::BytesAvailable()
    {
    // Intentionally Left Blank Do Nothing
    return 0;
    }

void CBulkOnlyTransportUsbcScLdd::StallEndpointAndWaitForClear()
    {
    // Now stall this endpoint
    OstTrace1(TRACE_SMASSSTORAGE_BOT, D1_SC, "Stalling endpoint %d", iInEndpoint);
    TInt r = iLdd.HaltEndpoint(iInEndpoint);
    if (r != KErrNone)
        {
        OstTraceExt2(TRACE_SMASSSTORAGE_BOT, D2_SC, "Error: stalling ep %d failed: %d", iInEndpoint, r);
        }
    TEndpointState ep_state;
    TInt i = 0;
    do
        {
        // Wait for 10ms before checking the ep status
        User::After(10000);
        iLdd.EndpointStatus(iInEndpoint, ep_state);
        if (++i >= 550)
            {
            // 5.5 secs should be enough (see 9.2.6.1 Request Processing Timing)
            OstTrace1(TRACE_SMASSSTORAGE_BOT, D3_SC, "Error: Checked for ep %d de-stall for 5.5s - giving up now", iInEndpoint);
            // We can now only hope for a Reset Recovery
            return;
            }
        } while ((ep_state == EEndpointStateStalled) && iStarted);
    OstTraceExt2(TRACE_SMASSSTORAGE_BOT, D4_SC, "Checked for ep %d de-stall: %d time(s)", iInEndpoint, i);
    }

/**
Read CBW data from the host and decode it.
*/
void CBulkOnlyTransportUsbcScLdd::ReadCBW()
    {
    if (IsActive())
        {
        __ASSERT_DEBUG(EFalse, User::Panic(KUsbMsSvrPncCat, EMsBulkOnlyStillActive));
        return;
        }
    TInt r = KErrNone;
    do
        {
        r = ReadUsb();
        if ((r == KErrCompletion) && (iSCReadSize == KCbwLength))
            {
            iCurrentState = EWaitForCBW;
            iStatus = KErrNone;
            DecodeCBW();
            iSCReadSize = 0;
            }
        else if (r == KErrNone)
            {
            iCurrentState = EWaitForCBW;
            SetActive();
            }
        else if ((r == KErrCompletion) && (iSCReadSize != KCbwLength))
            {
            ExpireData(iSCReadData);
            }
        }while ((r == KErrCompletion) && (!IsActive()));
    }

void CBulkOnlyTransportUsbcScLdd::ExpireData(TAny* aAddress)
    {
    if (aAddress)
        {
        iSCReadEndpointBuf.Expire(aAddress);
        }
    else
        {
        iSCReadEndpointBuf.Expire();
        }
    }

void CBulkOnlyTransportUsbcScLdd::ProcessCbwEvent()
    {
    ReadCBW();
    }

/**
Request data form the host for the protocol

@param aLength amount of data (in bytes) to be received from the host
*/
void CBulkOnlyTransportUsbcScLdd::ReadData(TUint /*aLength*/)
    {
    if (IsActive())
        {
        __ASSERT_DEBUG(EFalse, User::Panic(KUsbMsSvrPncCat, EMsBulkOnlyStillActive));
        return;
        }
    TInt r = KErrNone;
    do
        {
        r = ReadUsb();
        iCurrentState = EReadingData;
        if (r == KErrCompletion)
            {
            iReadBufPtr.Set((TUint8*) iSCReadData, iSCReadSize, iSCReadSize);
            iStatus = KErrNone;
            ProcessDataFromHost();
            iSCReadSize = 0;
            }
        else if (r == KErrNone)
            {
            iSCReadSize = 0;
            SetActive();
            }

        }while ((r == KErrCompletion) && (!IsActive()));
    }

void CBulkOnlyTransportUsbcScLdd::ProcessDataFromHost()
    {
    TInt ret = KErrNone;
        {
        if (iReadSetUp)
            {
            ret = iProtocol->ReadComplete(KErrNone);
            }
#ifndef MSDC_MULTITHREADED
        ExpireData(iSCReadData);
#endif
        TUint deviceDataLength = iSCReadSize; // What was written to the disk // static_cast<TUint>(iReadBuf.Length());
        if(ret == KErrCompletion)
            {
            // The protocol has indicated with KErrCompletion that sufficient
            // data is available in the buffer to process the transfer immediately.

            //iDataResidue is initially set to host data length as we do not know how much data is there in the 'LDD transfer'.
            //After a TakeBuffer call, iSCReadSize in updated and set to deviceDataLength
            iDataResidue -= deviceDataLength;
            }
        else
            {
            iDataResidue -= deviceDataLength;

            // The protocol has indicated that transfer is
            // complete, so send the CSW response to the host.
            iReadSetUp = EFalse;

            if (ret != KErrNone)
                {
                iCmdStatus = ECommandFailed;
                }

            if (iDataResidue)
                {
                OstTrace0(TRACE_SMASSSTORAGE_BOT, E2_SC, "Discarding residue");
                // we have to read as much data as available that host PC sends;
                // otherwise, bulk-out endpoint will need to keep sending NAK back.
                ReadAndDiscardData(iDataResidue);
                }

            SendCSW(iCbwTag, iDataResidue, iCmdStatus);
            }
        }
    }

void CBulkOnlyTransportUsbcScLdd::WriteUsb(TRequestStatus& aStatus, TPtrC8& aDes, TUint aLength, TBool aZlpRequired)
    {
    TUint aOffset = (TUint) aDes.Ptr() - (TUint) iChunk->Base();
    iSCWriteEndpointBuf.WriteBuffer(aOffset, aLength, aZlpRequired, aStatus);
    }

void CBulkOnlyTransportUsbcScLdd::SetCbwPtr()
    {
    iCbwBufPtr.Set((TUint8*)iSCReadData, iSCReadSize);
    }

TPtr8& CBulkOnlyTransportUsbcScLdd::SetCommandBufPtr(TUint aLength)
    {
    TPtr8 writeBuf((TUint8*) iDataPtr, aLength);
    iCommandBufPtr.Set(writeBuf);
    return iCommandBufPtr;
    }

void CBulkOnlyTransportUsbcScLdd::SetReadDataBufPtr(TUint /*aLength*/)
    {
    // Do nothing for now
    }

TPtr8& CBulkOnlyTransportUsbcScLdd::SetDataBufPtr()
    {
    TPtr8 writeBuf((TUint8*) iDataPtr, iInBufferLength);
    iDataBufPtr.Set(writeBuf);
    return iDataBufPtr;
    }

void CBulkOnlyTransportUsbcScLdd::SetPaddingBufPtr(TUint aLength)
    {
    TPtr8 writeBuf((TUint8*) iDataPtr, aLength, aLength);
    iPaddingBufPtr.Set(writeBuf);
    }

void CBulkOnlyTransportUsbcScLdd::SetCswBufPtr(TUint aLength)
    {
    TPtr8 writeBuf((TUint8*) iDataPtr, aLength, aLength);
    iCswBufPtr.Set(writeBuf);
    }

void CBulkOnlyTransportUsbcScLdd::ProcessReadingDataEvent()
    {
    ReadData();
    }

TInt CBulkOnlyTransportUsbcScLdd::ReadUsb(TUint /*aLength*/)
    {
    return iSCReadEndpointBuf.TakeBuffer(iSCReadData,iSCReadSize,iReadZlp,iStatus);
    }

void CBulkOnlyTransportUsbcScLdd::DiscardData(TUint aLength)
    {
    TUint c = 0;
    TRequestStatus status;
    TInt r = KErrNone;
    while (c < aLength)
        {
        iSCReadSize = 0;
        status = KErrNone;
        r = iSCReadEndpointBuf.TakeBuffer(iSCReadData,iSCReadSize,iReadZlp,status);
        c += iSCReadSize;
        if (r == KErrNone)
            User::WaitForRequest(status);
        if (r == KErrCompletion)
            {
            iSCReadEndpointBuf.Expire();
            }
        iSCReadSize = 0;
        }
    }

void CBulkOnlyTransportUsbcScLdd::WriteToClient(TUint aLength)
    {
    TUint c = 0;
    TRequestStatus status;
    TInt r = KErrNone;
    while (c < aLength)
        {
        iSCReadSize = 0;
        status = KErrNone;
        r = iSCReadEndpointBuf.TakeBuffer(iSCReadData,iSCReadSize,iReadZlp,status);
        c += iSCReadSize;
        iProtocol->ReadComplete(KErrGeneral);
        if (r == KErrNone)
            User::WaitForRequest(status);
        if (r == KErrCompletion)
            {
            iSCReadEndpointBuf.Expire();
            }
        iSCReadSize = 0;
        }
    }

#ifdef MSDC_MULTITHREADED
void CBulkOnlyTransportUsbcScLdd::GetBufferPointers(TPtr8& aDes1, TPtr8& aDes2)
    {
    //for DB
    TUint length = (TUint) (iInBufferLength - KCswBufferSize) / 2;

    TUint8* start = (TUint8*) (iDataPtr) + KCswBufferSize ; // 'first' buffer
    aDes1.Set(start, length, length);

    start = (TUint8*) (iDataPtr) + KCswBufferSize + length; // 'second' buffer
    aDes2.Set(start, length, length);
    }
#endif

void CBulkOnlyTransportUsbcScLdd::Activate(TRequestStatus& aStatus, TUint& aDeviceState)
    {
    iLdd.AlternateDeviceStatusNotify(aStatus, aDeviceState);
    }


void CBulkOnlyTransportUsbcScLdd::Cancel()
    {
    iLdd.AlternateDeviceStatusNotifyCancel();
    }




