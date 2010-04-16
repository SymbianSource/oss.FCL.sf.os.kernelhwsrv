// Copyright (c) 2000-2009 Nokia Corporation and/or its subsidiary(-ies).
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
// e32\include\drivers\usbcshared.inl
// Kernel side definitions for the USB Device driver stack (PIL + LDD).
// 
//

/**
 @file usbcshared.inl
 @internalTechnology
*/

#ifndef __USBCSHARED_INL__
#define __USBCSHARED_INL__

//
// --- DUsbClientController (USB PDD) ---
//

// --- Private member functions, used by controller itself ---

const DBase* DUsbClientController::PEndpoint2ClientId(TInt aRealEndpoint) const
	{
	if (iRealEndpoints[aRealEndpoint].iLEndpoint)
		return iRealEndpoints[aRealEndpoint].iLEndpoint->iInterface->iInterfaceSet->iClientId;
	else
		return NULL;
	}


TInt DUsbClientController::PEndpoint2LEndpoint(TInt aRealEndpoint) const
	{
	if (iRealEndpoints[aRealEndpoint].iLEndpoint)
		return iRealEndpoints[aRealEndpoint].iLEndpoint->iLEndpointNum;
	else
		return -1;
	}


const TUsbcConfiguration* DUsbClientController::CurrentConfig() const
	{
	return (iCurrentConfig ? iConfigs[iCurrentConfig - 1] : NULL);
	}


TUsbcConfiguration* DUsbClientController::CurrentConfig()
	{
	return (iCurrentConfig ? iConfigs[iCurrentConfig - 1] : NULL);
	}


TBool DUsbClientController::InterfaceExists(TInt aNumber) const
	{
	const TInt num_ifcsets = iConfigs[0]->iInterfaceSets.Count();
	RPointerArray<TUsbcInterfaceSet>& ifcsets = iConfigs[0]->iInterfaceSets;
	for (TInt i = 0; i < num_ifcsets; i++)
		{
		if (ifcsets[i]->iInterfaceNumber == aNumber)
			{
			return ETrue;
			}
		}
	return EFalse;
	}


TBool DUsbClientController::EndpointExists(TUint aAddress) const
	{
	// Ep0 doesn't have a "logical ep" pointer (there's no virtual endpoint zero);
	// that's why this pointer being non-NULL is not a sufficient criterion for
	// endpoint-existence. (Apart from that, ep0 always exists.)
	const TInt idx = EpAddr2Idx(aAddress);
	return ((idx < iDeviceTotalEndpoints) &&
			((iRealEndpoints[idx].iLEndpoint != NULL) ||
			 ((aAddress & KUsbEpAddress_Portmask) == 0)));
	}


void DUsbClientController::Buffer2Setup(const TAny* aBuf, TUsbcSetup& aSetup) const
	{
	// TUint8 index
	aSetup.iRequestType = static_cast<const TUint8*>(aBuf)[0];
	aSetup.iRequest		= static_cast<const TUint8*>(aBuf)[1];
	// TUint16 index from here!
	aSetup.iValue  = SWAP_BYTES_16((static_cast<const TUint16*>(aBuf))[1]);
	aSetup.iIndex  = SWAP_BYTES_16((static_cast<const TUint16*>(aBuf))[2]);
	aSetup.iLength = SWAP_BYTES_16((static_cast<const TUint16*>(aBuf))[3]);
	}


TUint DUsbClientController::EpIdx2Addr(TUint aRealEndpoint) const
	{
	return ((aRealEndpoint << 7) & 0x80) | ((aRealEndpoint >> 1) & 0x0f);
	}


TUint DUsbClientController::EpAddr2Idx(TUint aAddress) const
	{
	return ((aAddress & 0x80) >> 7) | ((aAddress & 0x0f) << 1);
	}


void DUsbClientController::SetEp0DataOutVars(const TUsbcSetup& aPacket, const DBase* aClientId)
	{
	__KTRACE_OPT(KUSB, Kern::Printf("DUsbClientController::SetEp0DataOutVars()"));
	iSetup = aPacket;
	iEp0DataReceiving = ETrue;
	iEp0DataReceived = 0;
	iEp0ClientId = aClientId;
	}


void DUsbClientController::ResetEp0DataOutVars()
	{
	__KTRACE_OPT(KUSB, Kern::Printf("DUsbClientController::ResetEp0DataOutVars()"));
	iEp0DataReceiving = EFalse;
	iEp0DataReceived = 0;
	iEp0ClientId = NULL;
	}


TBool DUsbClientController::IsInTheRequestList(const TUsbcRequestCallback& aCallback)
	{
    	const TInt irq = __SPIN_LOCK_IRQSAVE(iUsbLock);
	TSglQueIter<TUsbcRequestCallback> iter(iEp0ReadRequestCallbacks);
	TUsbcRequestCallback* p;
	while ((p = iter++) != NULL)
		{
		if (p == &aCallback)
			{
			__SPIN_UNLOCK_IRQRESTORE(iUsbLock, irq);
			return ETrue;
			}
		}
    	__SPIN_UNLOCK_IRQRESTORE(iUsbLock, irq);
	return EFalse;
	}


TBool DUsbClientController::IsInTheStatusList(const TUsbcStatusCallback& aCallback)
	{
    	const TInt irq = __SPIN_LOCK_IRQSAVE(iUsbLock);
	TSglQueIter<TUsbcStatusCallback> iter(iStatusCallbacks);
	TUsbcStatusCallback* p;
	while ((p = iter++) != NULL)
		{
		if (p == &aCallback)
			{
            		__SPIN_UNLOCK_IRQRESTORE(iUsbLock, irq);
			return ETrue;
			}
		}
    	__SPIN_UNLOCK_IRQRESTORE(iUsbLock, irq);
	return EFalse;
	}


TBool DUsbClientController::IsInTheEpStatusList(const TUsbcEndpointStatusCallback& aCallback)
	{
    	const TInt irq = __SPIN_LOCK_IRQSAVE(iUsbLock);
	TSglQueIter<TUsbcEndpointStatusCallback> iter(iEpStatusCallbacks);
	TUsbcEndpointStatusCallback* p;
	while ((p = iter++) != NULL)
		{
		if (p == &aCallback)
			{
		    	__SPIN_UNLOCK_IRQRESTORE(iUsbLock, irq);
			return ETrue;
			}
		}
    	__SPIN_UNLOCK_IRQRESTORE(iUsbLock, irq);
	return EFalse;
	}


TBool DUsbClientController::IsInTheOtgFeatureList(const TUsbcOtgFeatureCallback& aCallback)
	{
    	const TInt irq = __SPIN_LOCK_IRQSAVE(iUsbLock);
	TSglQueIter<TUsbcOtgFeatureCallback> iter(iOtgCallbacks);
	TUsbcOtgFeatureCallback* p;
	while ((p = iter++) != NULL)
		{
		if (p == &aCallback)
			{
		    	__SPIN_UNLOCK_IRQRESTORE(iUsbLock, irq);
			return ETrue;
			}
		}
    	__SPIN_UNLOCK_IRQRESTORE(iUsbLock, irq);
	return EFalse;
	}

//
// --- Misc classes ---
//

// --- TUsbcClientCallback

/** Constructor.
 */
TUsbcClientCallback::TUsbcClientCallback(DBase* aOwner, TDfcFn aCallback, TInt aPriority)
	: iOwner(aOwner),
	  iDfc(aCallback, aOwner, aPriority)
	{}


/** Returns a pointer to the owner of this request.

	@return A pointer to the owner of this request.
*/
DBase* TUsbcClientCallback::Owner() const
	{
	return iOwner;
	}


/** Executes the callback function set by the owner of this request.

	@return KErrNone.
*/
TInt TUsbcClientCallback::DoCallback()
	{
	__ASSERT_DEBUG((NKern::CurrentContext() == EThread), Kern::Fault(KUsbPILPanicCat, __LINE__));
	iDfc.Enque();
	return KErrNone;
	}


/** Cancels the callback function set by the owner of this request.
 */
void TUsbcClientCallback::Cancel()
	{
	iDfc.Cancel();
	}


/** Sets the DFC queue used by the callback function.
	@param aDfcQ DFC queue to be set
 */
void TUsbcClientCallback::SetDfcQ(TDfcQue* aDfcQ)
	{
	iDfc.SetDfcQ(aDfcQ);
	}


// --- TUsbcEndpointStatusCallback

/** Constructor.
 */
TUsbcEndpointStatusCallback::TUsbcEndpointStatusCallback(DBase* aOwner, TDfcFn aCallback,
														 TInt aPriority)
	: iOwner(aOwner),
	  iDfc(aCallback, aOwner, aPriority),
	  iState(0)
	{}


/** Sets the state of this request to aState.

	@param aState The new state to be set.
*/
void TUsbcEndpointStatusCallback::SetState(TUint aState)
	{
	iState = aState;
	}


/** Returns the state value of this request.

	@return The state value of this request.
*/
TUint TUsbcEndpointStatusCallback::State() const
	{
	return iState;
	}


/** Returns a pointer to the owner of this request.

	@return A pointer to the owner of this request.
*/
DBase* TUsbcEndpointStatusCallback::Owner() const
	{
	return iOwner;
	}


/** Executes the callback function set by the owner of this request.

	@return KErrNone.
*/
TInt TUsbcEndpointStatusCallback::DoCallback()
	{
	if (NKern::CurrentContext() == EThread)
		iDfc.Enque();
	else
		iDfc.Add();
	return KErrNone;
	}


/** Cancels the callback function set by the owner of this request.
*/
void TUsbcEndpointStatusCallback::Cancel()
	{
	iDfc.Cancel();
	}


/** Sets the DFC queue used by the callback function.
*/
void TUsbcEndpointStatusCallback::SetDfcQ(TDfcQue* aDfcQ)
	{
	iDfc.SetDfcQ(aDfcQ);
	}


// --- TUsbcStatusCallback

/** Constructor.
 */
TUsbcStatusCallback::TUsbcStatusCallback(DBase* aOwner, TDfcFn aCallback, TInt aPriority)
	: iOwner(aOwner),
	  iDfc(aCallback, aOwner, aPriority)
	{
 	ResetState();
	}


/** Sets the state of this request to aState (at the first available position
	in the state value array).

	@param aState The new state to be set.
*/
void TUsbcStatusCallback::SetState(TUsbcDeviceState aState)
	{
	for (TInt i = 0; i < KUsbcDeviceStateRequests; i++)
		{
		if (iState[i] == EUsbcNoState)
			{
			iState[i] = aState;
			return;
			}
		}
	__KTRACE_OPT(KPANIC, Kern::Printf("  Error: KUsbcDeviceStateRequests too small (%d)!",
									  KUsbcDeviceStateRequests));
	}


/** Returns the state value of this request at a certain index.

	@param aIndex The index to be used for referencing the state array.

	@return The state value of this request at aIndex.
*/
TUsbcDeviceState TUsbcStatusCallback::State(TInt aIndex) const
	{
	if (aIndex >= 0 && aIndex < KUsbcDeviceStateRequests)
		{
		return iState[aIndex];
		}
	else
		{
		__KTRACE_OPT(KPANIC, Kern::Printf("  Error: aIndex too large (%d)!", aIndex));
		return EUsbcNoState;
		}
	}


/** Resets the entire state value array of this request.
*/
void TUsbcStatusCallback::ResetState()
	{
	for (TInt i = 0; i < KUsbcDeviceStateRequests; ++i)
		{
		iState[i] = EUsbcNoState;
		}
	}


/** Returns a pointer to the owner of this request.

	@return A pointer to the owner of this request.
*/
DBase* TUsbcStatusCallback::Owner() const
	{
	return iOwner;
	}


/** Executes the callback function set by the owner of this request.

	@return KErrNone.
*/
TInt TUsbcStatusCallback::DoCallback()
	{
	if (NKern::CurrentContext() == EThread)
		iDfc.Enque();
	else
		iDfc.Add();
	return KErrNone;
	}


/** Cancels the callback function set by the owner of this request.
*/
void TUsbcStatusCallback::Cancel()
	{
	iDfc.Cancel();
	}


/** Sets the DFC queue used by the callback function.
*/
void TUsbcStatusCallback::SetDfcQ(TDfcQue* aDfcQ)
	{
	iDfc.SetDfcQ(aDfcQ);
	}

// --- TUsbcRequestCallback

/** Constructor.
 */
TUsbcRequestCallback::TUsbcRequestCallback(const DBase* aOwner, TInt aEndpointNum, TDfcFn aDfcFunc,
										   TAny* aEndpoint, TDfcQue* aDfcQ, TInt aPriority)
	: iEndpointNum(aEndpointNum),
	  iRealEpNum(-1),
	  iOwner(aOwner),
	  iDfc(aDfcFunc, aEndpoint, aDfcQ, aPriority),
	  iTransferDir(EControllerNone),
	  iBufferStart(NULL),
	  iPacketIndex(NULL),							  // actually TUint16 (*)[]
	  iPacketSize(NULL),							  // actually TUint16 (*)[]
	  iLength(0),
	  iZlpReqd(EFalse),
	  iTxBytes(0),
	  iRxPackets(0),
	  iError(KErrNone)
	{
	}


/** Destructor.
 */
TUsbcRequestCallback::~TUsbcRequestCallback()
	{
	__KTRACE_OPT(KUSB, Kern::Printf("TUsbcRequestCallback::~TUsbcRequestCallback()"));
	iDfc.Cancel();
	}


/** Sets some data members of this request for a read request.

	@param aBufferStart The start of the data buffer to be filled.
	@param aBufferAddr The physical address of the buffer (used for DMA).
	@param aPacketIndex A pointer to the packet index values array.
	@param aPacketSize A pointer to the packet size values array.
	@param aLength The number of bytes to be received.
*/
void TUsbcRequestCallback::SetRxBufferInfo(TUint8* aBufferStart, TPhysAddr aBufferAddr,
										   TUsbcPacketArray* aPacketIndex,
										   TUsbcPacketArray* aPacketSize,
										   TInt aLength)
	{
	iTransferDir = EControllerRead;
	iBufferStart = aBufferStart;
	iBufferAddr = aBufferAddr;
	iPacketIndex = aPacketIndex;
	iPacketSize = aPacketSize;
	iLength = aLength;
	}


/** Sets some data members of this request for a write request.

	@param aBufferStart The start of the buffer that contains the data to be sent.
	@param aBufferAddr The physical address of the buffer (used for DMA).
	@param aLength The number of bytes to be transmitted.
*/
void TUsbcRequestCallback::SetTxBufferInfo(TUint8* aBufferStart, TPhysAddr aBufferAddr, TInt aLength)
	{
	iTransferDir = EControllerWrite;
	iBufferStart = aBufferStart;
	iBufferAddr = aBufferAddr;
	iLength = aLength;
	}


/** Sets the transfer direction for this request.

	@param aTransferDir The new transfer direction.
*/
void TUsbcRequestCallback::SetTransferDirection(TTransferDirection aTransferDir)
	{
	iTransferDir = aTransferDir;
	}


/** Returns a pointer to the owner of this request.

	@return A pointer to the owner of this request.
*/
const DBase* TUsbcRequestCallback::Owner() const
	{
	return iOwner;
	}


/** Executes the callback function set by the owner of this request.

	@return KErrNone.
*/
TInt TUsbcRequestCallback::DoCallback()
	{
	if (NKern::CurrentContext() == NKern::EThread)
		iDfc.Enque();
	else
		iDfc.Add();
	return KErrNone;
	}


/** Cancels the callback function set by the owner of this request.
*/
void TUsbcRequestCallback::Cancel()
	{
	iDfc.Cancel();
	}

// --- TUsbcOtgFeatureCallback

/** Constructor.
 */
TUsbcOtgFeatureCallback::TUsbcOtgFeatureCallback(DBase* aOwner, TDfcFn aCallback,
												 TInt aPriority)
	: iOwner(aOwner),
	  iDfc(aCallback, aOwner, aPriority),
	  iValue(0)
	{}


/** Returns a pointer to the owner of this request.
	@return A pointer to the owner of this request.
*/
DBase* TUsbcOtgFeatureCallback::Owner() const
	{
	return iOwner;
	}


/** Set feature value which is to be notified to client.
	@param OTG feature value to be set
*/
void TUsbcOtgFeatureCallback::SetFeatures(TUint8 aFeatures)
	{
	iValue = aFeatures;
	}


/** Set feature value which is to be notified to client.
	@return Value of OTG features
*/
TUint8 TUsbcOtgFeatureCallback::Features() const
	{
	return iValue;
	}


/** Set DFC queue.
	@param aDfcQ  DFC queue to be set
*/ 
void TUsbcOtgFeatureCallback::SetDfcQ(TDfcQue* aDfcQ)
	{
	iDfc.SetDfcQ(aDfcQ);
	}


/** Executes the callback function set by the owner of this request.
	@return KErrNone.
*/
TInt TUsbcOtgFeatureCallback::DoCallback()
	{
	if (NKern::CurrentContext() == EThread)
		iDfc.Enque();
	else
		iDfc.Add();
	return KErrNone;
	}


/** Cancels the callback function set by the owner of this request.
 */
void TUsbcOtgFeatureCallback::Cancel()
	{
	iDfc.Cancel();
	}


/** Returns a pointer to the currently selected (active) setting of this interface.

	@return A pointer to the currently selected (active) setting of this interface.
*/
const TUsbcInterface* TUsbcInterfaceSet::CurrentInterface() const
	{
	return iInterfaces[iCurrentInterface];
	}


/** Returns a pointer to the currently selected (active) setting of this interface.

	@return A pointer to the currently selected (active) setting of this interface.
*/

TUsbcInterface* TUsbcInterfaceSet::CurrentInterface()
	{
	return iInterfaces[iCurrentInterface];
	}


#endif // __USBCSHARED_INL__



