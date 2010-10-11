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
// /os/kernelhwsrv/kernel/eka/drivers/rpmb/rpmbdevice.cpp
// Kernel extension entry point for RPMB driver.
// 
//

/**
 @file
 @internalTechnology
*/

#include "OstTraceDefinitions.h"
#ifdef OST_TRACE_COMPILER_IN_USE
#include "../../include/drivers/locmedia_ost.h"
#ifdef __VC32__
#pragma warning(disable: 4127) // disabling warning "conditional expression is constant"
#endif
#include "rpmbdeviceTraces.h"
#endif

#include <kernel/kernel.h>
#include <drivers/rpmbdevice.h>
#include <drivers/sdcard.h>
#include <drivers/sdio/sdio.h>
#include <drivers/mmc.h>

DRpmbDevice * DRpmbDevice::DRpmbDevicePtrs[KMaxPBusSockets*4] = {
	NULL, NULL, NULL, NULL,
	NULL, NULL, NULL, NULL,
	NULL, NULL, NULL, NULL,
	NULL, NULL, NULL, NULL};

EXPORT_C DRpmbDevice::DRpmbDevice():
	iSessionEndCallBack(DRpmbDevice::SessionEndCallBack, this),
	iDeviceIndex(KIndexNotAssigned)
    {
	}

EXPORT_C DRpmbDevice::~DRpmbDevice()
    {
    Close();
	}

void DRpmbDevice::SessionEndCallBack(TAny* aSelf)
    {
    DRpmbDevice& self = *static_cast<DRpmbDevice*>(aSelf);
    self.DoSessionEndCallBack();
    }

void DRpmbDevice::DoSessionEndCallBack()
    {   
    iSocket->EndInCritical();
    Kern::SemaphoreSignal(*iRequestSemaphore);
    }

void DRpmbDevice::BusCallBack(TAny* aPtr, TInt aReason, TAny* a1, TAny* a2)
{
    DRpmbDevice* device = (DRpmbDevice*)aPtr;
    TPBusState busState = (TPBusState) (TInt) a1;
	TInt busError = (TInt) a2;

    if(aReason == TPBusCallBack::EPBusStateChange 
		&& busState == EPBusOn && busError == KErrNone)
		{
		Kern::SemaphoreSignal(*(device->iPowerUpSemaphore));
        }
}

TInt DRpmbDevice::PowerUpStack()
    {
    //
    // Power up the socket - This ensures that the socket is powered up 
    // and the functions are re-enumerated.
    //  
    iBusCallBack.iFunction = BusCallBack;
    iBusCallBack.iPtr=this;
    iBusCallBack.SetSocket(iSocket->iSocketNumber);
    iBusCallBack.Add();
	NKern::ThreadEnterCS();
    TInt r = Kern::SemaphoreCreate(iPowerUpSemaphore,_L("RPMBPowerUpSem"), 0);
    if(r == KErrNone)
        {
		TInt r = iSocket->PowerUp();
        if(r==KErrNone)
            {
            Kern::SemaphoreWait(*iPowerUpSemaphore);
            }
        }
	NKern::ThreadLeaveCS();
    return r;
    }

void DRpmbDevice::SetSynchronisationParms(TUint8 aDeviceIndex)
	{
	// Mark this instance as being associated with the requested index 
	iDeviceIndex = aDeviceIndex;
	// Mark the requested index as being associated with this instance
	// Atomic operation ensures store is flushed from cache and committed 
	// to global memory
	__e32_atomic_store_ord_ptr(&(DRpmbDevicePtrs[iDeviceIndex]),this);
	}


void DRpmbDevice::ClearSynchronisationParms()
	{
	// Serialise access to global pointer array and it's local index
	NKern::FMWait(&iSynchronisationParmsMutex);
	if (iDeviceIndex < KMaxPBusSockets*4)
		{
		// Atomic operation for load from global memory and not from cache
		DRpmbDevice * ptrTableEntry = 
			(DRpmbDevice *)__e32_atomic_load_acq_ptr(&(DRpmbDevicePtrs[iDeviceIndex]));
		// This instance of DRpmbDevice is associated with an index
		// The associated index MUST be associated with this instance
		__ASSERT_ALWAYS((ptrTableEntry == this), Kern::Fault(__FILE__, __LINE__));
		// Disassociate index and instance
		// Atomic operation ensures store is flushed from cache and committed 
		// to global memory
		__e32_atomic_store_ord_ptr(&(DRpmbDevicePtrs[iDeviceIndex]),NULL);
		iDeviceIndex = KIndexNotAssigned;
		}
	// Serialise access to global pointer array and it's local index
	NKern::FMSignal(&iSynchronisationParmsMutex);
	}

EXPORT_C TInt DRpmbDevice::Open(TUint aDeviceIndex)
    {
    //
    //eMMC4.4+ devices have RPMB partitions and each MMC device may be configured as having an RPMB 
	//partition in the baseport
	//This function creates an MMC stack session for device aDeviceIndex
	//This is used to access the RPMB partition on that device
	//Extensions that use this interface during system startup should be located AFTER the RPMB and MMC
	//extesnions in the system ROM and should not call this interface synchronously from a system
	//startup initialisation routine
	//aDeviceIndex the index of the device supporting the RPMB partition
	//Returns KerrNone if successful
	//Returns KErrNotReady if the baseport configuration hasn't been read yet
    //Returns KErrNotSupported if the baseport configuration does not have a valid RPMB partition 
	//or RPMB is not supported by the media device
	//Otherwise retruns a systemwide error code
    //
    OstTrace0(TRACE_FLOW, DRPMBDEVICE_OPEN_1, "RPMB: >DrpmbDevice::Open");
    __KTRACE_OPT(KPBUS1, Kern::Printf("RPMB: >DrpmbDevice::Open"));
	
	TRpmbDeviceParms params;
	params.iCardNumber = 0;
	params.iSocketPtr = NULL;	
    MRpmbInfo* rpmbInterface = NULL;
	
	TInt r = MMCGetExtInterface(KInterfaceRpmb, (MMCMExtInterface*&) rpmbInterface);
	// MMCGetExtInterface currently returns KErrNotReady with rpmbInterface == NULL if the RPMB parameters 
	// haven't yet been populated
	// proveided any calling extension is located AFTER the RPMB and MMC extesnions in the system ROM and 
	// does not call this interface synchronously from a system initialisation routine the following 
	// shouldn't be asserted 
	OstTrace1(TRACE_FLOW, DRPMBDEVICE_OPEN_2, "RPMB: DrpmbDevice Get Interface err = %d", r);
	__KTRACE_OPT(KPBUS1, Kern::Printf("RPMB: DrpmbDevice Get Interface err = %d", r));
	__ASSERT_ALWAYS(r == KErrNone, Kern::Fault(__FILE__, __LINE__));
		
	if (rpmbInterface == NULL)
		{
		// unexpected error since MMCGetExtInterface didn't return an error
	    OstTrace0(TRACE_FLOW, DRPMBDEVICE_OPEN_3, "RPMB: DrpmbDevice Null rpmbInterface");
	    __KTRACE_OPT(KPBUS1, Kern::Printf("RPMB: DrpmbDevice Null rpmbInterface"));
		return KErrGeneral;
		}
		
	// Interface currently supports a single device, device index = 0   
	r = rpmbInterface->RpmbInfo(aDeviceIndex, params);
    if(r != KErrNone)
        {
		// requested index non zero or baseport not configured with RPMB capable MMC device
        OstTrace1(TRACE_FLOW, DRPMBDEVICE_OPEN_4, "RPMB: DrpmbDevice requested index non zero or baseport not configured, err = %d", r);
        __KTRACE_OPT(KPBUS1, Kern::Printf("RPMB: DrpmbDevice requested index non zero or baseport not configured, err = %d", r));
		return r;
		}
   
    iSocket = params.iSocketPtr;
	// iSocket cannot be NULL 
	// TMMCardControllerInterface::RegisterMediaDevices ensures that the assigned value is not NULL
	__ASSERT_ALWAYS((iSocket!=NULL), Kern::Fault(__FILE__, __LINE__));
    
	// Serialise access to global pointer array and it's local index
	NKern::FMWait(&iSynchronisationParmsMutex);

	if (iDeviceIndex != KIndexNotAssigned)
		{
		// This instance of DRpmbDevice is already open
		if (iDeviceIndex == aDeviceIndex)
			{
			// Serialise access to global pointer array and it's local index
			NKern::FMSignal(&iSynchronisationParmsMutex);
			// Already open with requested index
		    OstTrace0(TRACE_FLOW, DRPMBDEVICE_OPEN_5, "RPMB: DrpmbDevice already open with requested index");
		    __KTRACE_OPT(KPBUS1, Kern::Printf("RPMB: DrpmbDevice already open with requested index"));
			return KErrNone;
			}
		else
			{
			// Serialise access to global pointer array and it's local index
			NKern::FMSignal(&iSynchronisationParmsMutex);
			// Already open with other index
	        OstTrace0(TRACE_FLOW, DRPMBDEVICE_OPEN_6, "RPMB: DrpmbDevice already open with other index");
	        __KTRACE_OPT(KPBUS1, Kern::Printf("RPMB: DrpmbDevice already open with other index"));
			return KErrInUse;
			}
		}
	else
		{
		// This instance of DRpmbDevice is not open

		// Atomic operation for load from global memory and not from cache
		DRpmbDevice * ptrTableEntry = 
			(DRpmbDevice *)__e32_atomic_load_acq_ptr(&(DRpmbDevicePtrs[aDeviceIndex]));

		if (ptrTableEntry == NULL)
			{
			SetSynchronisationParms((TUint8)(aDeviceIndex));
			}
		else
			{
			// Requested index cannot be associated with this instance of DRpmbdevice  
			__ASSERT_ALWAYS(ptrTableEntry != this, Kern::Fault(__FILE__, __LINE__));
			// Serialise access to global pointer array and it's local index
			NKern::FMSignal(&iSynchronisationParmsMutex);
			// Requested index already associated with a different instance of DRpmbDevice
	        OstTrace0(TRACE_FLOW, DRPMBDEVICE_OPEN_7, "RPMB: DrpmbDevice requested index already associated with a different instance of DrpmbDevice");
	        __KTRACE_OPT(KPBUS1, Kern::Printf("RPMB: DrpmbDevice requested index already associated with a different instance of DrpmbDevice"));
			return KErrInUse;
			}
		}
	NKern::FMSignal(&iSynchronisationParmsMutex);

	r = PowerUpStack();
    if (r != KErrNone && r != KErrCompletion)
        {
        // Stack wasn't already powered up and failed to power up
		ClearSynchronisationParms();
	    OstTrace1(TRACE_FLOW, DRPMBDEVICE_OPEN_8, "RPMB: DrpmbDevice wasn't already powered up and failed with err = %d", r);
	    __KTRACE_OPT(KPBUS1, Kern::Printf("RPMB: DrpmbDevice wasn't already powered up and failed with err = %d", r));
		return r;
        }
    
    DMMCStack* stack = iSocket->Stack(KBusNumber);
    if(stack == NULL)
        {
		// KBusNumber = 0 so iSocket->Stack() returns iSocket->iStack 
		// Expect this to have been pre-assigned
		// Expect a socket to be bound to a stack
        ClearSynchronisationParms();
        OstTrace0(TRACE_FLOW, DRPMBDEVICE_OPEN_9, "RPMB: stack is NULL");
        __KTRACE_OPT(KPBUS1, Kern::Printf("RPMB: stack is NULL"));
        return KErrNoMemory;
        }
    
    iCard = stack->CardP(params.iCardNumber);
    if(iCard == NULL) 
        {
		// stack->CardP() returns stack->iCardArray->CardP(params.iCardNumber)
		// Expect this to have been pre-assigned
		// Expect card array to point to a card
        ClearSynchronisationParms();
        OstTrace0(TRACE_FLOW, DRPMBDEVICE_OPEN_10, "RPMB: card pointer is NULL");
        __KTRACE_OPT(KPBUS1, Kern::Printf("RPMB: card pointer is NULL"));
        return KErrNoMemory;
        }
    
	NKern::ThreadEnterCS(); 
    iSession = stack->AllocSession(iSessionEndCallBack);
	NKern::ThreadLeaveCS(); 
    if (iSession == NULL)
        {
		// DMMCStack::AllocSession() failed to create a new instance off DMMCSession
        OstTrace0(TRACE_FLOW, DRPMBDEVICE_OPEN_11, "RPMB: failed to create a new instance of DMMCSession");
        __KTRACE_OPT(KPBUS1, Kern::Printf("RPMB: failed to create a new instance of DMMCSession"));
        return KErrNoMemory; 
        }

    iSession->SetStack(stack);
    iSession->SetCard(iCard);
    
    TInt bufLen, minorBufLen;
    stack->BufferInfo(iIntBuf, bufLen, minorBufLen);
	// mmc media driver reserved the first KRpmbOneFramePacketLength bytes of the 
	// PSL buffer to be used for RPMB requests / responses
	iIntBuf += minorBufLen;
    
    if(iCard->ExtendedCSD().ExtendedCSDRev() < 5 || iCard->ExtendedCSD().RpmbSize() == 0)
        {
		// RPMB is not supported on selected hardware
        ClearSynchronisationParms();
        OstTrace0(TRACE_FLOW, DRPMBDEVICE_OPEN_12, "RPMB: feature is not supported on selected hardware");
        __KTRACE_OPT(KPBUS1, Kern::Printf("RPMB: feature is not supported on selected hardware"));
        return KErrNotSupported;
        }
    OstTrace0(TRACE_FLOW, DRPMBDEVICE_OPEN_13, "RPMB: <DrpmbDevice::Open");
    __KTRACE_OPT(KPBUS1, Kern::Printf("RPMB: <DrpmbDevice::Open"));
    return KErrNone;
    }

EXPORT_C void DRpmbDevice::Close()
    {
	//
    //Closes an interaction with an RPMB configured device 
    //
    OstTrace0(TRACE_FLOW, DRPMBDEVICE_CLOSE_1, "RPMB: >DrpmbDevice::Close");
    __KTRACE_OPT(KPBUS1, Kern::Printf("RPMB: >DrpmbDevice::Close"));
 	
	ClearSynchronisationParms();
    
	iBusCallBack.Remove();

	NKern::ThreadEnterCS();

    if(iPowerUpSemaphore)
		{
        iPowerUpSemaphore->Close(NULL);
		iPowerUpSemaphore = NULL;
		}

	if (iSession)
		{
		delete iSession;
		iSession = NULL;
		}

	if(iRequestSemaphore)
		{
        iRequestSemaphore->Close(NULL);
		iRequestSemaphore = NULL;
		}
   
	NKern::ThreadLeaveCS();
	OstTrace0(TRACE_FLOW, DRPMBDEVICE_CLOSE_2, "RPMB: <DrpmbDevice::Close");
	__KTRACE_OPT(KPBUS1, Kern::Printf("RPMB: <DrpmbDevice::Close"));
    }

EXPORT_C TInt DRpmbDevice::SendAccessRequest(TDes8& aRpmbRequest, TDes8& aRpmbResponse)
    {
    //
    //sends a request to be handled by the RPMB partition
    //aRpmbRequest - the physically contiguous region of memory containg the request to be handled
    //aRpmbResponse - the physically contiguous region memory to where the response from the hardware will be written
	//returns KErrNone if successful
	//returns system wide epoc error code if set on call back from stack
	//returns KErrArgument for invalid descriptor argument length
	//returns KErrNotReady if not opened
	//retruns KErrNotready if MMC stack is processing a posponed power down or a postponed media change event
	//otherwise returns system wide error code 
	//
    
    OstTrace0(TRACE_FLOW, DRPMBDEVICE_SENDACCESSREQUEST_1, "RPMB: >DrpmbDevice::SendAccessRequest");
    __KTRACE_OPT(KPBUS1, Kern::Printf("RPMB: >DrpmbDevice::SendAccessRequest"));

	if ((aRpmbRequest.Length() != KRpmbOneFramePacketLength) || (aRpmbResponse.Length() != KRpmbOneFramePacketLength))
		{
		// insist on single frame access as mutiple read and multiple write (reliable write) notb yet supported
		return KErrArgument;
		}

	if (iSession == NULL)
        {
		// DRpmbDevice::Open not called at all
		// or not called after DRpmbDevice::Close
	    OstTrace0(TRACE_FLOW, DRPMBDEVICE_SENDACCESSREQUEST_2, "RPMB: DMMCSession is NULL");
	    __KTRACE_OPT(KPBUS1, Kern::Printf("RPMB: DMMCSession is NULL"));
        return KErrNotReady;
        }

	if (iRequestSemaphore == NULL)
		{
		// iRequestSemaphore zero filled prior to contruction
		// create semaphore for waiting on stack
		NKern::ThreadEnterCS();
		TInt r = Kern::SemaphoreCreate(iRequestSemaphore,_L("RPMBRequestSemaphore"), 0);
		NKern::ThreadLeaveCS();
		if (r != KErrNone)
			{
			// failed to create semaphore 
		    OstTrace1(TRACE_FLOW, DRPMBDEVICE_SENDACCESSREQUEST_3, "RPMB: failed to create semaphore err = %d", r);
		    __KTRACE_OPT(KPBUS1, Kern::Printf("RPMB: failed to create semaphore err = %d", r));
			return r;
			}
		}

    memcpy(iIntBuf, aRpmbRequest.Ptr(), KRpmbOneFramePacketLength);
     
    TInt r = iSocket->InCritical();
	// may be KErrNotready if MMC stack is processing a posponed power down event or a postponed media change event
	if (r == KErrNone)
       {
    iSession->SetPartition(TExtendedCSD::ESelectRPMB);            //Set RPMB Partition
    iSession->ResetCommandStack();
    iSession->FillCommandArgs(KDeviceAddress, KRpmbOneFramePacketLength, iIntBuf, KRpmbOneFramePacketLength);
    iSession->iSessionID = ECIMRpmbAccess;

        r = iSession->Engage();
        if(r == KErrNone)
            {
			Kern::SemaphoreWait(*iRequestSemaphore);
			memcpy((TUint8 *)aRpmbResponse.Ptr(), iIntBuf, KRpmbOneFramePacketLength);
		    OstTrace1(TRACE_FLOW, DRPMBDEVICE_SENDACCESSREQUEST_4, "RPMB: request complete with %d", iSession->EpocErrorCode());
		    __KTRACE_OPT(KPBUS1, Kern::Printf("RPMB: request complete with %d", iSession->EpocErrorCode()));
			return iSession->EpocErrorCode();  
            }
        }
 
    iSocket->EndInCritical();
    
    OstTrace1(TRACE_FLOW, DRPMBDEVICE_SENDACCESSREQUEST_5, "RPMB: <DrpmbDevice::SendAccessRequest complete %d", r);
    __KTRACE_OPT(KPBUS1, Kern::Printf("RPMB: <DrpmbDevice::SendAccessRequest complete %d", r));
    
    return r;
    }

DECLARE_STANDARD_EXTENSION()
    {
    __KTRACE_OPT(KBOOT,Kern::Printf("RPMBEXT.DLL DECLARE_STANDARD_EXTENSION entry point"));      
	return KErrNone;
    }

