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



/**
 @file
 @internalTechnology
*/

#include <e32base.h>
#include <d32usbc.h>

#include "mstypes.h"
#include "msctypes.h"
#include "usbmsclientpanic.h"
#include "botmsctypes.h"
#include "testman.h"
#include "botmscserver.h"

#include "mserverprotocol.h"
#include "mdevicetransport.h"

#include "mstypes.h"
#include "botcontrolinterface.h"
#include "debug.h"
#include "msdebug.h"
#include "cusbmassstorageserver.h"
#include "cusbmassstoragecontroller.h"
#include "drivemanager.h"
#include "cbulkonlytransport.h"

#define KInEndpoint EEndpoint1
#define KOutEndpoint EEndpoint2


//-------------------------------------
/**
Create CBulkOnlyTransport object
@param aNumDrives - The number of drives available for MS
@param aController - reference to the parent
@return pointer to newly created object
*/
CBulkOnlyTransport* CBulkOnlyTransport::NewL(TInt aNumDrives,
                                             CUsbMassStorageController& aController)
	{
    __MSFNSLOG
	if (aNumDrives <=0 || static_cast<TUint>(aNumDrives) > KUsbMsMaxDrives)
		{
		User::Leave(KErrArgument);
		}
	CBulkOnlyTransport* self = new(ELeave) CBulkOnlyTransport(aNumDrives, aController);
	CleanupStack::PushL(self);
	self->ConstructL();
	CleanupStack::Pop(self);
	return self;
	}

#ifdef MSDC_TESTMODE
CBulkOnlyTransport* CBulkOnlyTransport::NewL(TInt aNumDrives,
                                             CUsbMassStorageController& aController,
                                             TTestParser* aTestParser)
	{
    __MSFNSLOG
	if (aNumDrives <=0 || static_cast<TUint>(aNumDrives) > KUsbMsMaxDrives)
		{
		User::Leave(KErrArgument);
		}
	CBulkOnlyTransport* self = new(ELeave) CBulkOnlyTransport(aNumDrives,
                                                              aController,
                                                              aTestParser);
	CleanupStack::PushL(self);
	self->ConstructL();
	CleanupStack::Pop(self);
	return self;
	}
#endif


/**
c'tor
@param aNumDrives - The number of drives available for MS
@param aController - reference to the parent
*/
CBulkOnlyTransport::CBulkOnlyTransport(TInt aNumDrives,
                                       CUsbMassStorageController& aController):
	CActive(EPriorityStandard),
	iMaxLun(aNumDrives-1),
	iController(aController),
	iStallAllowed(ETrue)
	{
    __MSFNLOG
	}

#ifdef MSDC_TESTMODE
CBulkOnlyTransport::CBulkOnlyTransport(TInt aNumDrives,
                                       CUsbMassStorageController& aController,
                                       TTestParser* aTestParser)
:   CActive(EPriorityStandard),
	iMaxLun(aNumDrives-1),
	iController(aController),
	iStallAllowed(ETrue),
    iTestParser(aTestParser)
	{
    __MSFNLOG
	}
#endif


/**
Constructs the CBulkOnlyTranspor object
*/
void CBulkOnlyTransport::ConstructL()
	{
    __MSFNLOG
	iBotControlInterface = CBotControlInterface::NewL(*this);
	iDeviceStateNotifier = CActiveDeviceStateNotifier::NewL(*this);
	CActiveScheduler::Add(this);
	}


/**
Destructor
*/
CBulkOnlyTransport::~CBulkOnlyTransport()
	{
    __MSFNLOG
	if (iInterfaceConfigured)
		{
		Stop();
		}
	delete iBotControlInterface;
	delete iDeviceStateNotifier;
	}


/**
Set or unset configuration descriptor for USB MassStorage Bulk Only transport

@param aUnset indicate whether set or unset descriptor
@return KErrNone if operation was completed successfully, errorcode otherwise
*/
TInt CBulkOnlyTransport::SetupConfigurationDescriptor(TBool aUnset)
	{
    __MSFNLOG
	TInt ret(KErrNone);
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

	return ret;
	}


/**
Set up interface descriptor

@return KErrNone if operation was completed successfully, errorcode otherwise
*/
TInt CBulkOnlyTransport::SetupInterfaceDescriptors()
	{
    __MSFNLOG
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
	TUsbcInterfaceInfoBuf ifc;
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
			// KInEndpoint is going to be our TX (IN, write) endpoint
			ifc().iEndpointData[0].iType = KUsbEpTypeBulk;
			if((d_caps().iFeatureWord1 & KUsbDevCapsFeatureWord1_EndpointResourceAllocV2) == KUsbDevCapsFeatureWord1_EndpointResourceAllocV2)
				ifc().iEndpointData[0].iFeatureWord1  = KUsbcEndpointInfoFeatureWord1_DMA|KUsbcEndpointInfoFeatureWord1_DoubleBuffering;
			ifc().iEndpointData[0].iDir  = KUsbEpDirIn;
			ifc().iEndpointData[0].iSize = maxPacketSize;
			ifc().iEndpointData[0].iInterval_Hs = 0;
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
			// KOutEndpoint is going to be our RX (OUT, read) endpoint
			ifc().iEndpointData[1].iType = KUsbEpTypeBulk;
			if((d_caps().iFeatureWord1 & KUsbDevCapsFeatureWord1_EndpointResourceAllocV2) == KUsbDevCapsFeatureWord1_EndpointResourceAllocV2)
				ifc().iEndpointData[1].iFeatureWord1  = KUsbcEndpointInfoFeatureWord1_DMA|KUsbcEndpointInfoFeatureWord1_DoubleBuffering;
			ifc().iEndpointData[1].iDir  = KUsbEpDirOut;
			ifc().iEndpointData[1].iSize = maxPacketSize;
			ifc().iEndpointData[1].iInterval_Hs = 0;
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
	ifc().iClass.iClassNum    = 0x08;	// Mass Storage
	ifc().iClass.iSubClassNum = 0x06;	// SCSI Transparent Command Set
	ifc().iClass.iProtocolNum = 0x50;	// Bulk Only Transport

	TUint bandwidth_priority = (EUsbcBandwidthOUTDefault | EUsbcBandwidthINDefault);
	if (d_caps().iHighSpeed)
		{
		// If this device supports USB High-speed, then we request 64KB buffers
		// (otherwise the default 4KB ones will do).
		bandwidth_priority = (EUsbcBandwidthOUTPlus2 | EUsbcBandwidthINPlus2);
		// Also, tell the Protocol about it, because it might want to do some
		// optimizing too.
		iProtocol->ReportHighSpeedDevice();
		}
	ret = iLdd.SetInterface(0, ifc, bandwidth_priority);
	return ret;
	}


/**
Called by the protocol after processing the packet to indicate that more data is required.

@param aData reference to the data buffer.
*/
void CBulkOnlyTransport::SetupDataOut(TPtr8& aData)
    {
    __MSFNLOG
    iDataTransferMan.SetModeDataOut(aData);
    }

/**
Called by the protocol after processing the packet to indicate that data should be written to the host.

@param aData reference to the data buffer.
*/
void CBulkOnlyTransport::SetupDataIn(TPtrC8& aData)
	{
    iDataTransferMan.SetModeDataIn(aData);
	}


TInt CBulkOnlyTransport::Start()
	{
    __MSFNLOG
	TInt err = KErrNone;

	if (!iProtocol)
		{
		return KErrBadHandle;   //protocol should be set up before start
		}

	if (IsActive())
		{
		__PRINT(_L("CBulkOnlyTransport::Start  - active before start!\n"));
		return KErrInUse;
		}

	if ((err = iLdd.Open(0))					!= KErrNone ||
		(err = SetupConfigurationDescriptor()) 	!= KErrNone ||
		(err = SetupInterfaceDescriptors())		!= KErrNone )
		{
		__PRINT(_L("CBulkOnlyTransport::Start  - Error during descriptors setup!\n"));
		return err;
		}

	iDeviceStateNotifier->Activate();  // activate notifier wich will wait until USB became configured
	TUsbcDeviceState deviceStatus = EUsbcDeviceStateDefault;
	err = iLdd.DeviceStatus(deviceStatus);
	__PRINT1(_L("CBulkOnlyTransport::Start - Device status = %d\n"), deviceStatus);
	if (err == KErrNone && deviceStatus == EUsbcDeviceStateConfigured)
		{
		__PRINT(_L("CBulkOnlyTransport::Start  - Starting bulk only transport\n"));
		err = HwStart();
		}
	iInterfaceConfigured = ETrue;
	return err;
	}

TInt CBulkOnlyTransport::HwStart(TBool aDiscard)
	{
    __MSFNLOG

    TInt lun = MaxLun();
    do
        {
        Controller().DriveManager().Connect(lun);
        }
    while(--lun >= 0);

	TInt res = iBotControlInterface->Start() ;

	iCurrentState = ENone;
    iDataTransferMan.Init();
	iStarted = ETrue;

	TUsbDeviceCaps d_caps;
	TInt ret = iLdd.DeviceCaps(d_caps);
	if (ret == KErrNone)
		{
		if((d_caps().iFeatureWord1 & KUsbDevCapsFeatureWord1_EndpointResourceAllocV2) != KUsbDevCapsFeatureWord1_EndpointResourceAllocV2)
			{
			// Set up DMA if possible (errors are non-critical)
			TInt err = iLdd.AllocateEndpointResource(KOutEndpoint, EUsbcEndpointResourceDMA);
			if (err != KErrNone)
				{
				__PRINT1(_L("Set DMA on OUT endpoint failed with error code: %d"), err);
				}
			err = iLdd.AllocateEndpointResource(KInEndpoint, EUsbcEndpointResourceDMA);
			if (err != KErrNone)
				{
				__PRINT1(_L("Set DMA on IN endpoint failed with error code: %d"), err);
				}

			// Set up Double Buffering if possible (errors are non-critical)
			err = iLdd.AllocateEndpointResource(KOutEndpoint, EUsbcEndpointResourceDoubleBuffering);
			if (err != KErrNone)
				{
				__PRINT1(_L("Set Double Buffering on OUT endpoint failed with error code: %d"), err);
				}
			err = iLdd.AllocateEndpointResource(KInEndpoint, EUsbcEndpointResourceDoubleBuffering);
			if (err != KErrNone)
				{
				__PRINT1(_L("Set Double Buffering on IN endpoint failed with error code: %d"), err);
				}
			}
		}

    if (aDiscard)
		{
		TInt bytes;
		const TInt err = iLdd.QueryReceiveBuffer(KOutEndpoint, bytes);
		if (err != KErrNone || bytes <= 0)
			{
			__PRINT1(_L("Error: err=%d bytes=%d"), bytes);
			}
		else
			{
			__PRINT1(_L("RxBuffer has %d bytes"), bytes);
			FlushDataOut(bytes);
			}
		}

	ClientReadCbw();
	return res;
	}


void CBulkOnlyTransport::HwStop()
	{
    __MSFNLOG
	if (iStarted)
		{
        iController.DriveManager().Disconnect();
		Cancel();
		iBotControlInterface->Cancel();
		iProtocol->Cancel();
		iStarted = EFalse;
		}
	}


void CBulkOnlyTransport::HwSuspend()
	{
    __MSFNLOG
    iController.DriveManager().Disconnect();
	}


void CBulkOnlyTransport::HwResume()
	{
    __MSFNLOG
    iController.DriveManager().Connect();
	}


/**
Stops the Bulk Only Transport
*/
TInt CBulkOnlyTransport::Stop()
	{
    __MSFNLOG
	iBotControlInterface->Cancel();
	iDeviceStateNotifier->Cancel();
	Cancel();
	if  (iInterfaceConfigured)
		{
		iLdd.ReleaseInterface(0);
		SetupConfigurationDescriptor(ETrue);
		iLdd.Close();
		}
	iCurrentState = ENone;
	iInterfaceConfigured = EFalse;

	return KErrNone;
	}


/**
Read aLength bytes of data from the host into the read buffer.
@param aLength The number of bytes to read from the host.
*/
void CBulkOnlyTransport::ClientReadCbw()
	{
    __MSFNLOG
	if (IsActive())
		{
		__PRINT(_L("Still active\n"));
		__ASSERT_DEBUG(EFalse, User::Panic(KUsbMsClientPanicCat, EMsBulkOnlyStillActive));
		return;
		}

    ReadCbw();
    }


void CBulkOnlyTransport::ReadCbw()
	{
    __MSFNLOG
	iCbwBuf.SetMax();
	iLdd.ReadUntilShort(iStatus, KOutEndpoint, iCbwBuf, iCbwBuf.Length());
	iCurrentState = EWaitForCBW;
	SetActive();
	}


void CBulkOnlyTransport::DoCancel()
	{
    __MSFNLOG
	iLdd.WriteCancel(KInEndpoint);
	iLdd.ReadCancel(KOutEndpoint);
	}


void CBulkOnlyTransport::Activate(TInt aReason)
    {
    __MSFNLOG
    SetActive();
    TRequestStatus* r = &iStatus;
    User::RequestComplete(r, aReason);
    }


void CBulkOnlyTransport::RunL()
	{
    __MSFNLOG
	if (iStatus != KErrNone)
		{
		__PRINT1(_L("Error %d in RunL, halt endpoints \n"), iStatus.Int());
		SetPermError(); //halt endpoints for reset recovery
		return;
		}

	switch (iCurrentState)
		{
        case EWaitForCBW:
            __PRINT(_L("EWaitForCBW"));
            TRAPD(err, DecodeCbwL());
            if (err)
                {
                // CBW not valid or meaningful
                // Specification says: "If the CBW is not valid, the device
                // shall STALL the Bulk-In pipe. Also, the device shall either
                // STALL the Bulk-Out pipe, or the device shall accept and
                // discard any Bulk-Out data. The device shall maintain this
                // state until a Reset Recovery." Here we keep bulk-in ep
                // stalled and ignore bulk-out ep.
                SetPermError();
                return;
                }
			break;

        case EHandleDataIn:
            {
            __PRINT(_L("EHandleDataIn"));
            iDataTransferMan.SetModeNoData();

			if (iDataTransferMan.iDataResidue && iStallAllowed)
				{
				StallEndpointAndWaitForClear(KInEndpoint);
				}
#ifdef MSDC_TESTMODE
            if (iTestParser && iTestParser->Enabled())
                {
                if (iTestParser->TestCase() == TTestParser::ETestCaseDiStallCsw)
                    {
                    iTestParser->ClrTestCase();
                    __TESTMODEPRINT1("CBW Tag=0x%x: Stalling CSW Data-In", iCbwTag);
                    StallEndpointAndWaitForClear(KInEndpoint);
                    }
                }
#endif
			SendCsw(iCbwTag, iDataTransferMan.iDataResidue, iCmdStatus);
            }
			break;

		case EHandleDataOut:
			{
            TUint bytesWritten;
            TInt ret = iProtocol->MediaWritePacket(bytesWritten);
            iDataTransferMan.iDataResidue -= bytesWritten;

			switch(ret)
                {
            case KErrCompletion:
                {
                // The protocol has indicated with KErrCompletion that
                // sufficient data is available in the buffer to process the
                // transfer immediately.

                TInt deviceDataLength = iDataTransferMan.iReadBuf.Length();
                iLdd.Read(iStatus, KOutEndpoint, iDataTransferMan.iReadBuf, deviceDataLength);
                SetActive();
                }
                break;
            case KErrNotReady:
                {
                // The protocol has indicated with KErrNotReady that
                // insufficient data is available in the buffer, so should wait
                // for it to arrive.
                TInt deviceDataLength = iDataTransferMan.iReadBuf.Length();
                iLdd.Read(iStatus, KOutEndpoint, iDataTransferMan.iReadBuf, deviceDataLength);
                SetActive();
                }
                break;
            case KErrAbort:
                {
                iDataTransferMan.SetModeNoData();
                iCmdStatus = TBotCsw::ECommandFailed;
                if (iDataTransferMan.iDataResidue)
                    {
                    __PRINT(_L("Discarding residue"));
                    FlushDataOut(iDataTransferMan.iTransportResidue);
                    }
                SendCsw(iCbwTag, iDataTransferMan.iDataResidue, iCmdStatus);
                }
                break;

            case KErrNone:
            default:
                {
                // The protocol has indicated that transfer is complete, so
                // send the CSW response to the host.
                iDataTransferMan.SetModeNoData();
#ifdef MSDC_TESTMODE
                if (iDataTransferMan.iDataResidue)
                    {
                    TBool done = EFalse;
                    if (iTestParser && iTestParser->Enabled())
                        {
                        TInt testCase = iTestParser->TestCase();
                        if (testCase == TTestParser::ETestCaseDoStallCsw)
                            {
                            iTestParser->ClrTestCase();
                            __TESTMODEPRINT1("CBW Tag=0x%x: Stalling CSW Data-Out", iCbwTag);
                            StallEndpointAndWaitForClear(KInEndpoint);
                            done = ETrue;
                            }
                        }
                    if (!done)
                        {
                        __PRINT(_L("Discarding residue"));
                        // we have to read as much data as available that host PC
                        // sends; otherwise, bulk-out endpoint will need to keep
                        // sending NAK back.
                        FlushDataOut(iDataTransferMan.iDataResidue);
                        __TESTMODEPRINT2("CBW Tag=0x%x: Residue = x%x",
                                         iCbwTag, iDataTransferMan.iDataResidue);
                        }
                    }
                else
                    {
                    if (iTestParser && iTestParser->Enabled())
                        {
                        TInt testCase = iTestParser->TestCase();
                        if (testCase == TTestParser::ETestCaseDoStallCsw)
                            {
                            iTestParser->ClrTestCase();
                            __TESTMODEPRINT1("CBW Tag=0x%x: Stalling CSW Data-Out", iCbwTag);
                            StallEndpointAndWaitForClear(KInEndpoint);
                            }
                        }
                    }

#else
                    if (iDataTransferMan.iDataResidue)
                        {
                        __PRINT(_L("Discarding residue"));
                        // we have to read as much data as available that host PC
                        // sends; otherwise, bulk-out endpoint will need to keep
                        // sending NAK back.
                        FlushDataOut(iDataTransferMan.iDataResidue);
                        }
#endif
                SendCsw(iCbwTag, iDataTransferMan.iDataResidue, iCmdStatus);
                }
                break;  // KErrNone
                } // switch
            }
			break;  // EReadingData

		case ESendingCSW:
			__PRINT(_L("ESendingCSW"));
			ReadCbw();
			break;

        case EPermErr:
			__PRINT(_L("EPermErr"));
			StallEndpointAndWaitForClear(KInEndpoint);
            break;

        default:
			SetPermError();		// unexpected state
		}
	}


/**
Decode the CBW received from the host via KOutEndpoint

- If the header is valid, the data content is passed to the parser.
- Depending on the command, more data may be transmitted/received.
- ...or the CSW is sent (if not a data command).

*/
void CBulkOnlyTransport::DecodeCbwL()
	{
    __MSFNLOG
    TBotServerReq req;
    req.DecodeL(iCbwBuf);

    if (!(req.IsValidCbw() && req.IsMeaningfulCbw(iMaxLun)))
        {
        User::Leave(KErrGeneral);
        }

    TPtrC8 aData(&iCbwBuf[TBotCbw::KCbwCbOffset], TBotCbw::KMaxCbwcbLength);
	//aData.Set(&iCbwBuf[TBotCbw::KCbwCbOffset], TBotCbw::KMaxCbwcbLength);
    TUint8 lun = req.Lun();

    iCbwTag  = 	req.Tag();
    TUint32 hostDataLength = req.DataTransferLength();
    iDataTransferMan.SetHostDataLen(hostDataLength);

    TBotServerReq::TCbwDirection dataToHost = req.Direction();

    __PRINT4(_L("lun =%d, hostDataLength=%d, CBWtag = 0x%X\n, dataToHost=%d\n"),
             lun, hostDataLength, iCbwTag, dataToHost);

	//////////////////////////////////////////////
	TBool ret = iProtocol->DecodePacket(aData, lun);
	//////////////////////////////////////////////

	iStallAllowed = ETrue;

	if (!ret)
		{
		__PRINT(_L("Command Failed\n"));
		iCmdStatus = TBotCsw::ECommandFailed;
		}
	else
		{
		__PRINT(_L("Command Passed\n"));
		iCmdStatus = TBotCsw::ECommandPassed;
		}

	if (hostDataLength)    // Host expected data transfer
		{
		if (dataToHost == TBotServerReq::EDataIn)  // send data to host
			{
            if (!iDataTransferMan.IsModeDataIn())
				{
				__PRINT(_L("Write buffer was not setup\n"));

				if (hostDataLength <= KBOTMaxBufSize)
					{
					// Case 4 or 8"
					iBuf.FillZ(hostDataLength);
					iLdd.Write(iStatus, KInEndpoint, iBuf, hostDataLength);
					SetActive();
					iCurrentState = EHandleDataIn;
					iStallAllowed = EFalse;
					if (iDataTransferMan.IsModeDataOut())
						{
                        //read buffer WAS set up - case (8)
						iCmdStatus = TBotCsw::EPhaseError;
						}
					return;
					}
				else
					{
                    // Use next block instead of StallEndpointAndWaitForClear(KInEndpoint);
                    FlushDataIn(hostDataLength);
					}

				if (iDataTransferMan.IsModeDataOut())
					{
                    //read buffer WAS set up - case (8)
					SendCsw(iCbwTag, hostDataLength, TBotCsw::EPhaseError);
					//don't care to reset any flag - should get reset recovery
					}
				else
					{
                    // case (4)
					SendCsw(iCbwTag, hostDataLength, iCmdStatus);
					}
				return;
				}

//==================
			TInt deviceDataLength = iDataTransferMan.iWriteBuf.Length();
            iDataTransferMan.iDataResidue = hostDataLength - deviceDataLength;

#ifdef MSDC_TESTMODE
            if (iTestParser && iTestParser->Enabled())
                {
                TInt testCase = iTestParser->TestCase();
                if (testCase == TTestParser::ETestCaseDiResidue)
                    {
                    __TESTMODEPRINT2("DataIN (Data Residue) %x %x", iTestParser->TestCounter(), hostDataLength);

                    const TInt KTransferLength = 0x100;
                    const TInt KResidueA = KTransferLength/4 -10;
                    const TInt KResidueB = KTransferLength/4 - 10;
                    if (hostDataLength > KTransferLength)
                        {
                        TInt residue = 0;
                        TInt i = iTestParser->TestCounter();
                        switch (i)
                            {
                            case 0:
                                iTestParser->ClrTestCase();
                                break;
                            case 1:
                                residue = KResidueA;
                                break;
                            case 2:
                                residue = KResidueB;
                                break;

                            default:
                                break;
                            }

                        //deviceDataLength -= residue;
                        iDataTransferMan.iDataResidue = residue;
                        iStallAllowed = EFalse;
                        iTestParser->DecTestCounter();

                        __TESTMODEPRINT4("CBW Tag=0x%x: Setting Residue (Data-In) hdl=%x ddl=%x residue=%x",
                                         iCbwTag, hostDataLength, deviceDataLength, residue);
                        }
                    }
                }
#endif
			if (deviceDataLength < hostDataLength && hostDataLength <= KBOTMaxBufSize)
				{
                // do not pad
                // iStallAllowed = ETrue;
                // __PRINT1(_L("iBuf.Length=%d\n"),iBuf.Length());
                // iLdd.Write(iStatus, KInEndpoint, iBuf, deviceDataLength);
                // SetActive();
                //iCurrentState = EWritingData;

                // pad
                iBuf.Zero();
                iBuf.Append(iDataTransferMan.iWriteBuf);
                iBuf.SetLength(hostDataLength);
                iStallAllowed = EFalse;
                __PRINT1(_L("iBuf.Length=%d\n"),iBuf.Length());
                iLdd.Write(iStatus, KInEndpoint, iBuf, hostDataLength);
                SetActive();
                iCurrentState = EHandleDataIn;

				return;
				}

			if (deviceDataLength == hostDataLength)
				{
                //case (6)[==]
#ifdef MSDC_TESTMODE
                if (iTestParser && iTestParser->Enabled())
                    {
                    if (iTestParser->TestCase() == TTestParser::ETestCaseDiStallData)
                        {
                        iTestParser->ClrTestCase();
                        __TESTMODEPRINT1("CBW Tag=0x%x: Stalling CSW Data-In", iCbwTag);
                        StallEndpointAndWaitForClear(KInEndpoint);
                        }
                    }
#endif

				DataInWriteRequest(deviceDataLength);
				return;
				}
			else if (deviceDataLength < hostDataLength)
				{
                //case (5)[<]
				DataInWriteRequest(deviceDataLength, ETrue);		// Send ZLP
				return;
				}
			else
				{
                // deviceDataLength > hostDataLength - case (7)
				iCmdStatus = TBotCsw::EPhaseError;
				iDataTransferMan.iDataResidue = 0;
				DataInWriteRequest(hostDataLength);
				return;
				}
			}
		else  // read data from host
			{
			if (!iDataTransferMan.IsModeDataOut())
				{
				__PRINT(_L("Read buffer was not setup\n"));

                // Use next block instead of StallEndpointAndWaitForClear(KOutEndpoint);
                FlushDataOut(hostDataLength);
				if (iDataTransferMan.IsModeDataIn())
					{
                    //case (10)
					SendCsw(iCbwTag, hostDataLength, TBotCsw::EPhaseError);
					}
				else
					{
                    // case (9)
					SendCsw(iCbwTag, hostDataLength, iCmdStatus);
					}
				return;
				}

			TInt deviceDataLength = iDataTransferMan.iReadBuf.Length();

			if (deviceDataLength <= hostDataLength)
				{
                // case (11) and (12)
#ifdef MSDC_TESTMODE
                TBool done = EFalse;
                if (iTestParser && iTestParser->Enabled())
                    {
                    TInt testCase = iTestParser->TestCase();

                    if (testCase == TTestParser::ETestCaseDoStallData)
                        {
                        iTestParser->ClrTestCase();
                        // Stall Data Out
                        __TESTMODEPRINT2("CBW Tag=0x%x:  Stalling Data (Data-Out) Residue = x%x",
                                         iCbwTag, iDataTransferMan.iDataResidue);
                        StallEndpointAndWaitForClear(KOutEndpoint);
                        TInt bytes;
                        const TInt err = iLdd.QueryReceiveBuffer(KOutEndpoint, bytes);
                        __PRINT1(_L("Error: err=%d bytes=%d"), bytes);
                        FlushDataOut(bytes);
                        }
                    }
                if (!done)
                    {
                    DataOutReadRequest(deviceDataLength);
                    }
#else
				DataOutReadRequest(deviceDataLength);
#endif
				return;
				}
			else
				{
                // case  (13)
                /**
                 * Comment following line in order to pass compliant test.
                 * As spec said in case 13:"The device may receive data up to a
                 * total of dCBWDataTransferLength."
                 * Here we choose to ignore incoming data.
                 */
				//StallEndpointAndWaitForClear(KOutEndpoint); //Stall Out endpoint
                if (iDataTransferMan.IsModeDataOut())
                    {
                    iDataTransferMan.SetModeNoData();
                    iLdd.Read(iStatus, KOutEndpoint, iDataTransferMan.iReadBuf, hostDataLength);
                    User::WaitForRequest(iStatus);
                    iProtocol->MediaWriteAbort();
                    }
                SendCsw(iCbwTag, hostDataLength, TBotCsw::EPhaseError);
				return;
				}
			}
		}
	else  // Host expected no data transfer
		{
		__PRINT(_L("No data transfer expected\n"));
		iDataTransferMan.iDataResidue = 0;
		if (!iDataTransferMan.IsModeNoData())
			{
            // case (2) and (3)
			SendCsw(iCbwTag, 0, TBotCsw::EPhaseError);
			}
		else
			{
            //case (1)
#ifdef MSDC_TESTMODE
            if (iTestParser && iTestParser->Enabled())
                {
                TInt testCase = iTestParser->TestCase();
                if (testCase == TTestParser::ETestCaseNoDataStallCsw)
                    {
                    __TESTMODEPRINT1("CBW Tag=0x%x: Stalling CSW for Case 1 (Hn=Dn)", iCbwTag);
                    iTestParser->ClrTestCase();
                    StallEndpointAndWaitForClear(KInEndpoint);
                    }

                if (testCase == TTestParser::ETestCaseNoDataPhaseError)
                    {
                    __TESTMODEPRINT1("CBW Tag=0x%x: Enabling phase error (no data)", iCbwTag);
                    iTestParser->SetPhaseError();
                    }
                }
#endif
			SendCsw(iCbwTag, 0, iCmdStatus);
			}
		}
	}


/**
Initiate stalling of bulk IN endpoint.
Used when protocol wants to force host to initiate a reset recovery.
*/
void CBulkOnlyTransport::SetPermError()
	{
    __MSFNLOG
    iCurrentState = EPermErr;
    Activate(KErrNone);
	}


/**
Send data provided by protocol to the host

@param aLength amount of data (in bytes) to be send to host
*/
void CBulkOnlyTransport::DataInWriteRequest(TUint aLength, TBool aZlpRequired)
	{
    __MSFNLOG
	iLdd.Write(iStatus, KInEndpoint, iDataTransferMan.iWriteBuf, aLength, aZlpRequired);
    iDataTransferMan.iTransportResidue -= aLength;
	iCurrentState = EHandleDataIn;
	SetActive();
	}


/**
Request data form the host for the protocol

@param aLength amount of data (in bytes) to be received from the host
*/
void CBulkOnlyTransport::DataOutReadRequest(TUint aLength)
	{
    __MSFNLOG
	iLdd.Read(iStatus, KOutEndpoint, iDataTransferMan.iReadBuf, aLength);
	iDataTransferMan.iTransportResidue -= aLength;
	iCurrentState = EHandleDataOut;
    SetActive();
	}


/**
Send Command Status Wrapper to the host

@param aTag Echo of Command Block Tag sent by the host.
@param aDataResidue the difference between the amount of data expected by the
       host, and the actual amount of data processed by the device.
@param aStatus indicates the success or failure of the command.
*/
void CBulkOnlyTransport::SendCsw(TUint aTag, TUint32 aDataResidue, TBotCsw::TCswStatus aStatus)
	{
    __MSFNLOG
	__PRINT2(_L("DataResidue = %d, Status = %d \n"), aDataResidue, aStatus);
    iCsw.SetLength(TBotCsw::KCswLength);
    TBotServerResp resp(iCbwTag, aDataResidue, aStatus);
#ifdef MSDC_TESTMODE
    resp.EncodeL(iCsw, iTestParser);
#else
    resp.EncodeL(iCsw);
#endif
	iLdd.Write(iStatus, KInEndpoint, iCsw, TBotCsw::KCswLength);
	iCurrentState = ESendingCSW;
	SetActive();
	}


/**
Associates the transport with the protocol.  Called during initialization of the controller.

@param aProtocol reference to the protocol
*/
void CBulkOnlyTransport::RegisterProtocol(MServerProtocol& aProtocol)
	{
    __MSFNLOG
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
    __MSFNLOG
	return iMaxLun;
	}


/**
Used by CControlInterface
@return reference to USB logical driver
*/
RDevUsbcClient& CBulkOnlyTransport::Ldd()
	{
	return iLdd;
	}


void CBulkOnlyTransport::StallEndpointAndWaitForClear(TEndpointNumber aEndpoint)
	{
    __MSFNLOG
	__ASSERT_DEBUG(aEndpoint != EEndpoint0, User::Panic(KUsbMsClientPanicCat, EMsWrongEndpoint));

	// Now stall this endpoint
	__PRINT1(_L("Stalling endpoint %d"), aEndpoint);
	__TESTMODEPRINT2("CBW Tag=0x%x: Stalling endpoint %d", iCbwTag, aEndpoint);
	TInt r = iLdd.HaltEndpoint(aEndpoint);
	if (r != KErrNone)
		{
		__PRINT2(_L("Error: stalling ep %d failed: %d"), aEndpoint, r);
		}
	TEndpointState ep_state;
	TInt i = 0;
	do
		{
		// Wait for 10ms before checking the ep status
		User::After(10000);

        if (aEndpoint == KInEndpoint)
            {
            // Discard BULK-OUT data
            TInt bytes;
            const TInt err = iLdd.QueryReceiveBuffer(KOutEndpoint, bytes);
            if (err != KErrNone || bytes <= 0)
                {
                __PRINT1(_L("Error: err=%d bytes=%d"), bytes);
                }
            else
                {
                __PRINT1(_L("RxBuffer has %d bytes"), bytes);
                FlushDataOut(bytes);
                }
            }

		iLdd.EndpointStatus(aEndpoint, ep_state);
		if (++i >= 550)
			{
			// 5.5 secs should be enough (see 9.2.6.1 Request Processing Timing)
			__PRINT1(_L("Error: Checked for ep %d de-stall for 5.5s - giving up now"), aEndpoint);
            __TESTMODEPRINT1("Error: Checked for ep %d de-stall for 5.5s - giving up now", aEndpoint);
			// We can now only hope for a Reset Recovery
			return;
			}
		} while ((ep_state == EEndpointStateStalled) && iStarted);
	__PRINT2(_L("Checked for ep %d de-stall: %d time(s)"), aEndpoint, i);
    __TESTMODEPRINT2("Checked for ep %d de-stall: %d time(s)", aEndpoint, i);
	}



/**
Called by the protocol to determine how many bytes of data are available in the read buffer.

@return The number of bytes available in the read buffer
*/
TInt CBulkOnlyTransport::BytesAvailable()
	{
    __MSFNLOG
	TInt bytes = 0;
	TInt err = iLdd.QueryReceiveBuffer(KOutEndpoint, bytes);
	if (err != KErrNone)
		bytes = 0;
	return bytes;
	}


/**
 * Read out rest data from KOutEndpoint and discard them
 */
void CBulkOnlyTransport::FlushDataOut(TInt aLength)
	{
    __MSFNLOG
	iBuf.SetMax();
	TRequestStatus status;
	while (aLength > 0)
		{
		iLdd.ReadOneOrMore(status, KOutEndpoint, iBuf, iBuf.Length());
		User::WaitForRequest(status);
		TInt err = status.Int();
		if (err != KErrNone)
			{
			// Bad.
			break;
			}
		aLength -= iBuf.Length();
		}
	}


void CBulkOnlyTransport::FlushDataIn(TInt aLength)
    {
	iBuf.SetMax();
	TInt c = 0;
    TInt len;
	TRequestStatus status;
	while (c < aLength)
		{
        len = aLength - c;
		if (len >  KBOTMaxBufSize)
			{
			len = KBOTMaxBufSize;
			}
		iLdd.Write(status, KInEndpoint, iBuf, len);
		User::WaitForRequest(status);
		c +=  KBOTMaxBufSize;
		}
    }

//
// --- class CActiveDeviceStateNotifier ---------------------------------------------------------
//
CActiveDeviceStateNotifier::CActiveDeviceStateNotifier(CBulkOnlyTransport& aParent)
:   CActive(EPriorityStandard),
	iParent(aParent),
	iDeviceState(EUsbcNoState),
	iOldDeviceState(EUsbcNoState)
	{
    __MSFNLOG
	}


CActiveDeviceStateNotifier* CActiveDeviceStateNotifier::NewL(CBulkOnlyTransport& aParent)
	{
    __MSFNSLOG
	CActiveDeviceStateNotifier* self = new (ELeave) CActiveDeviceStateNotifier(aParent);
	CleanupStack::PushL(self);
	self->ConstructL();
	CActiveScheduler::Add(self);
	CleanupStack::Pop();									// self
	return (self);
	}


void CActiveDeviceStateNotifier::ConstructL()
	{
    __MSFNLOG
	}


CActiveDeviceStateNotifier::~CActiveDeviceStateNotifier()
	{
    __MSFNLOG
	Cancel();												// base class
	}


void CActiveDeviceStateNotifier::DoCancel()
/**
 *
 */
	{
    __MSFNLOG
	iParent.Ldd().AlternateDeviceStatusNotifyCancel();
	}


void CActiveDeviceStateNotifier::RunL()
/**
 *
 */
	{
    __MSFNLOG
	// This displays the device state.
	// In a real world program, the user could take here appropriate action (cancel a
	// transfer request or whatever).
	if (!(iDeviceState & KUsbAlternateSetting))
		{
		switch (iDeviceState)
			{
		case EUsbcDeviceStateUndefined:
			__PRINT(_L("Device State notifier: Undefined\n"));
			iParent.HwStop();
			break;
		case EUsbcDeviceStateAttached:
			__PRINT(_L("Device State notifier: Attached\n"));
			iParent.HwStop();
			break;
		case EUsbcDeviceStatePowered:
			__PRINT(_L("Device State notifier: Powered\n"));
			iParent.HwStop();
			break;
		case EUsbcDeviceStateDefault:
			__PRINT(_L("Device State notifier: Default\n"));
			iParent.HwStop();
			break;
		case EUsbcDeviceStateAddress:
			__PRINT(_L("Device State notifier: Address\n"));
			iParent.HwStop();
			break;
		case EUsbcDeviceStateConfigured:
			__PRINT(_L("Device State notifier: Configured\n"));
			if (iOldDeviceState == EUsbcDeviceStateSuspended)
				{
				iParent.HwResume();
				}
			else
				{
				iParent.HwStart();
				}
			break;
		case EUsbcDeviceStateSuspended:
			__PRINT(_L("Device State notifier: Suspended\n"));
			if (iOldDeviceState == EUsbcDeviceStateConfigured)
				{
				iParent.HwSuspend();
				}
			break;
		default:
			__PRINT(_L("Device State notifier: ***BAD***\n"));
			iParent.HwStop();
			break;
			}
		iOldDeviceState = iDeviceState;
		}
	else if (iDeviceState & KUsbAlternateSetting)
		{
		__PRINT1(_L("Device State notifier: Alternate interface setting has changed: now %d\n"), iDeviceState & ~KUsbAlternateSetting);
		}
	Activate();
	}


void CActiveDeviceStateNotifier::Activate()
/**
 *
 */
	{
    __MSFNLOG
	if (IsActive())
		{
		__PRINT(_L("Still active\n"));
		return;
		}
	iParent.Ldd().AlternateDeviceStatusNotify(iStatus, iDeviceState);
	SetActive();
	}
