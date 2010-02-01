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
// e32/include/drivers/usbc.h
// Kernel side definitions for the USB Device driver stack (PIL + LDD).
// 
//

/**
 @file usbc.h
 @internalTechnology
*/

#ifndef __USBC_H__
#define __USBC_H__

#include <kernel/kernel.h>
#include <kernel/kern_priv.h>
#include <kernel/kpower.h>
#include <platform.h>

#include <d32usbc.h>

#include <drivers/usbcshared.h>



/** LDD Major version, This should agree with the information in RDevUsbcClient::TVer.
*/
const TInt KUsbcMajorVersion = 0;

/** LDD Minor version, This should agree with the information in RDevUsbcClient::TVer.
*/
const TInt KUsbcMinorVersion = 1;

/** LDD Build version, This should agree with the information in RDevUsbcClient::TVer.
*/
const TInt KUsbcBuildVersion = KE32BuildVersionNumber;

/** Must correspond to the max enum of TRequest + 1;
	currently this is ERequestOtgFeaturesNotify = 10.
*/
const TInt KUsbcMaxRequests = 11;

//
//########################### Logical Device Driver (LDD) #############################
//

/** USB LDD factory class.
*/
class DUsbcLogDevice : public DLogicalDevice
	{
public:
	DUsbcLogDevice();
	virtual TInt Install();
	virtual void GetCaps(TDes8& aDes) const;
	virtual TInt Create(DLogicalChannelBase*& aChannel);
	};


/** OUT buffering is a collection of flat buffers. Each is either fillable or drainable.
	When one buffer becomes full (notified by the PIL) it is marked as not-fillable and the next
	fillable buffer is used. When the buffer has finished draining it is marked as fillable.
*/
class TDmaBuf
	{
public:
	TDmaBuf();
 	TDmaBuf(TUsbcEndpointInfo* aEndpointInfo, TInt aBandwidthPriority);
	~TDmaBuf();
	TInt Construct(TUsbcEndpointInfo* aEndpointInfo);
	TInt BufferTotalSize() const;
	TInt BufferSize() const;
	TInt SetBufferAddr(TInt aBufInd, TUint8* aBufAddr);
	TInt BufferNumber() const;
	void SetMaxPacketSize(TInt aSize);
	void Flush();
	// Rx (OUT) variants
	void RxSetActive();
	void RxSetInActive();
	TBool RxIsActive();
	TBool IsReaderEmpty();
	void ReadXferComplete(TInt aNoBytesRx, TInt aNoPacketsRx, TInt aErrorCode);
	TInt RxCopyDataToClient(DThread* aThread, TClientBuffer *aTcb, TInt aLength, TUint32& aDestOffset,
							TBool aRUS, TBool& aCompleteNow);
	TInt RxCopyPacketToClient(DThread* aThread,TClientBuffer *aTcb, TInt aLength);
	TInt RxGetNextXfer(TUint8*& aBufferAddr, TUsbcPacketArray*& aIndexArray, TUsbcPacketArray*& aSizeArray,
					   TInt& aLength, TPhysAddr& aBufferPhys);
	TBool RxIsEnoughSpace(TInt aSize);
	inline TInt RxBytesAvailable() const;
	inline void IncrementBufferIndex(TInt& aIndex);
	inline TInt NoRxPackets() const;
	TInt SetDrainable(TInt aBufferNum);
	// Tx (IN) variants
	void TxSetActive();
	void TxSetInActive();
	TBool TxIsActive();
	TInt TxStoreData(DThread* aThread,TClientBuffer *aTcb, TInt aTxLength, TUint32 aBufferOffset);
	TInt TxGetNextXfer(TUint8*& aBufferAddr, TInt& aTxLength, TPhysAddr& aBufferPhys);
	TBool ShortPacketExists();

#if defined(USBC_LDD_BUFFER_TRACE)
	TInt NoRxPacketsAlt() const;
	TInt NoRxBytesAlt() const;
#endif

private:
	TBool AdvancePacket();
	inline TInt GetCurrentError();
	TBool NextDrainableBuffer();
	TBool NextFillableBuffer();
	void FreeDrainedBuffers();
	TInt PeekNextPacketSize();
	TInt PeekNextDrainableBuffer();
	void ModifyTotalRxBytesAvail(TInt aVal);
	void ModifyTotalRxPacketsAvail(TInt aVal);
	void AddToDrainQueue(TInt aBufferIndex);
	inline TInt CopyToUser(DThread* aThread, const TUint8* aSourceAddr, TInt aLength,
						   TClientBuffer *aTcb, TUint32& aDestOffset);
private:
	TInt iExtractOffset;									// offset into current packet for data read
	TInt iMaxPacketSize;
	TInt iNumberofBuffers;
	TInt iBufSz;
	TBool iRxActive;
	TBool iTxActive;
	TInt iTotalRxBytesAvail;
	TInt iTotalRxPacketsAvail;
	//
	TUint8* iBufBasePtr;
	TUint8* iCurrentDrainingBuffer;
	TInt iCurrentDrainingBufferIndex;
	TInt iCurrentFillingBufferIndex;
	TUint iCurrentPacket;
	TUsbcPacketArray* iCurrentPacketIndexArray;
	TUsbcPacketArray* iCurrentPacketSizeArray;
	TUint8* iBuffers[KUsbcDmaBufNumMax];
	TBool iDrainable[KUsbcDmaBufNumMax];
	TUsbcPacketArray iPacketInfoStorage[KUsbcDmaBufNumMax * KUsbcDmaBufNumArrays * KUsbcDmaBufMaxPkts];
	TUsbcPacketArray* iPacketIndex[KUsbcDmaBufNumMax];
	TUsbcPacketArray* iPacketSize[KUsbcDmaBufNumMax];
	TUint iNumberofBytesRx[KUsbcDmaBufNumMax];
	TUint iNumberofPacketsRx[KUsbcDmaBufNumMax];
	TInt iError[KUsbcDmaBufNumMax];
	TPhysAddr iBufferPhys[KUsbcDmaBufNumMax];
	TBool iCanBeFreed[KUsbcDmaBufNumMax];
	TInt iDrainQueue[KUsbcDmaBufNumMax + 1];
	TInt iDrainQueueIndex;
	TUint iEndpointType;

#if defined(USBC_LDD_BUFFER_TRACE)
	TInt iFillingOrder;
	TInt iFillingOrderArray[KUsbcDmaBufNumMax];
	TInt iDrainingOrder;
 	TUint iNumberofBytesRxRemain[KUsbcDmaBufNumMax];
 	TUint iNumberofPacketsRxRemain[KUsbcDmaBufNumMax];
#endif
	};


class DLddUsbcChannel;

/** Endpoint tracking for the LDD buffering etc.
*/
class TUsbcEndpoint
	{
public:
	TUsbcEndpoint();
	TUsbcEndpoint(DLddUsbcChannel* aLDD, DUsbClientController* aController,
				  const TUsbcEndpointInfo* aEndpointInfo, TInt aEndpointNum,
				  TInt aBandwidthPriority);
	~TUsbcEndpoint();
	TInt Construct();
	TInt TryToStartRead(TBool aReEntrant);
	TInt TryToStartWrite(TEndpointTransferInfo* pTfr);
	TInt CopyToClient(DThread* aThread, TClientBuffer *aTcb);
	TInt CopyToClient(DThread* aClient, TBool& aCompleteNow, TClientBuffer *aTcb);
	TInt ContinueWrite();
	void SetMaxPacketSize(TInt aSize);
	void CancelTransfer(DThread* aThread, TClientBuffer *aTcb);
	void AbortTransfer();
	inline TUsbcEndpointInfo* EndpointInfo();
	inline TInt RxBytesAvailable() const;

	inline TInt BufferSize() const;
	inline TInt SetBufferAddr( TInt aBufInd, TUint8* aAddr);
	inline TInt BufferNumber() const;

	inline void SetTransferInfo(TEndpointTransferInfo* aTransferInfo);
	inline void ResetTransferInfo();
	inline void SetClientReadPending(TBool aVal);
	inline void SetClientWritePending(TBool aVal);
	inline TBool ClientWritePending();
	inline TBool ClientReadPending();
	inline void SetRealEpNumber(TInt aRealEpNumber);
	inline TInt RealEpNumber() const;

public:
	TDmaBuf* iDmaBuffers;

private:
	static void RequestCallback(TAny* aTUsbcEndpoint);
	void TxComplete();
	TInt RxComplete(TBool aReEntrant);
	void RxCompleteNow();
	TInt EndpointComplete();

private:
	DUsbClientController* iController;
	TUsbcEndpointInfo iEndpointInfo;
	TEndpointTransferInfo iTransferInfo;
	TBool iClientReadPending;
	TBool iClientWritePending;
	TInt iEndpointNumber;
	TInt iRealEpNumber;
	DLddUsbcChannel* iLdd;
	TInt iError;
	TUsbcRequestCallback* iRequestCallbackInfo;
	TUint32 iBytesTransferred;
	TInt iBandwidthPriority;
	};


/** Linked list of 'alternate setting' info for use by the LDD.
*/
class TUsbcAlternateSettingList
	{
public:
	TUsbcAlternateSettingList();
	~TUsbcAlternateSettingList();

public:
	TUsbcAlternateSettingList* iNext;
	TInt iNumberOfEndpoints;
	TUint iSetting;
	TInt iEpNumDeOrderedByBufSize[KMaxEndpointsPerClient + 1];
	TUsbcEndpoint* iEndpoint[KMaxEndpointsPerClient + 1];
	};


struct TClientAsynchNotify
	{
		TClientBufferRequest *iBufferRequest;
		TClientBuffer *iClientBuffer;
		void Reset();
	};
/** The channel class - the actual USB LDD.
*/
class DLddUsbcChannel : public DLogicalChannel
	{
public:
	DLddUsbcChannel();
	~DLddUsbcChannel();
	virtual TInt SendMsg(TMessageBase * aMsg);
	TInt PreSendRequest(TMessageBase * aMsg,TInt aReqNo, TRequestStatus* aStatus, TAny* a1, TAny* a2);
	TInt SendControl(TMessageBase* aMsg);
	virtual void HandleMsg(TMessageBase* aMsg);
	virtual TInt DoCreate(TInt aUnit, const TDesC8* aInfo, const TVersion& aVer);
	virtual TInt RequestUserHandle(DThread* aThread, TOwnerType aType);
	TInt DoRxComplete(TUsbcEndpoint* aTUsbcEndpoint, TInt aEndpoint, TBool aReentrant);
	void DoRxCompleteNow(TUsbcEndpoint* aTUsbcEndpoint, TInt aEndpoint);
	void DoTxComplete(TUsbcEndpoint* aTUsbcEndpoint, TInt aEndpoint, TInt aError);
	inline DThread* Client() const {return iClient;}
	inline TBool ChannelClosing() const {return iChannelClosing;}
	inline TUint AlternateSetting() const {return iAlternateSetting;}
	TClientBuffer *GetClientBuffer(TInt aEndpoint);

private:
	TInt DoCancel(TInt aReqNo);
	void DoRequest(TInt aReqNo, TRequestStatus* aStatus, TAny* a1, TAny* a2);
	TInt DoControl(TInt aFunction, TAny* a1, TAny* a2);
	TInt DoTransferAsyncReq(TInt aEndpointNum, TAny* a1, TAny* a2, TBool& aNeedsCompletion);
	TInt DoOtherAsyncReq(TInt aReqNo, TAny* a1, TAny* a2, TBool& aNeedsCompletion);
	TBool AlternateDeviceStateTestComplete();
	TInt SetInterface(TInt aInterfaceNum, TUsbcIfcInfo* aUserInterfaceInfoBuf);
	void StartEpReads();
	void DestroyAllInterfaces();
	void DestroyInterface(TUint aInterface);
	void DestroyEp0();
	inline TBool ValidEndpoint(TInt aEndpoint);
	TInt DoEmergencyComplete();
	void ReadDes8(const TAny* aPtr, TDes8& aDes);
	TInt SetupEp0();
	DPlatChunkHw* ReAllocate(TInt aBuffersize, DPlatChunkHw* aHwChunk, TUint32 aCacheAttribs);
	DPlatChunkHw* Allocate(TInt aBuffersize, TUint32 aCacheAttribs);
	void ClosePhysicalChunk(DPlatChunkHw* &aHwChunk);
	void CancelNotifyEndpointStatus();
	void CancelNotifyOtgFeatures();
	static void StatusChangeCallback(TAny* aDLddUsbcChannel);
	static void EndpointStatusChangeCallback(TAny* aDLddUsbcChannel);
	static void OtgFeatureChangeCallback(TAny* aDLddUsbcChannel);
	static void EmergencyCompleteDfc(TAny* aDLddUsbcChannel);
	void DeConfigure(TInt aErrorCode);
	TInt SelectAlternateSetting(TUint aAlternateSetting);
	TInt EpFromAlternateSetting(TUint aAlternateSetting, TInt aEndpoint);
	TInt ProcessAlternateSetting(TUint aAlternateSetting);
	TInt ProcessDeviceState(TUsbcDeviceState aDeviceState);
	void ResetInterface(TInt aErrorCode);
	void AbortInterface();
	// Set buffer address of the interface
	void ReSetInterfaceMemory(TUsbcAlternateSettingList* aAlternateSettingListRec,
	        RArray<DPlatChunkHw*> &aHwChunks );
	void UpdateEndpointSizes();
	// Check and alloc memory for the interface
	TInt SetupInterfaceMemory(RArray<DPlatChunkHw*> &aHwChunks, 
	        TUint32 aCacheAttribs );
	void PanicClientThread(TInt aReason);
	TInt PinMemory(TDesC8 *aDes, TVirtualPinObject *iPinObj); //Descriptor pinning helper.
	void CompleteBufferRequest(DThread* aThread, TInt aReqNo, TInt aReason);
private:
	DUsbClientController* iController;
	DThread* iClient;
	TBool iValidInterface;
	TUsbcAlternateSettingList* iAlternateSettingList;
	TUsbcEndpoint* iEndpoint[KMaxEndpointsPerClient + 1];	// include ep0
	TRequestStatus* iRequestStatus[KUsbcMaxRequests];
	TClientAsynchNotify* iClientAsynchNotify[KUsbcMaxRequests];
	TUsbcClientCallback iCompleteAllCallbackInfo;
	TAny* iStatusChangePtr;
	TUsbcStatusCallback iStatusCallbackInfo;
	TAny* iEndpointStatusChangePtr;
	TUsbcEndpointStatusCallback iEndpointStatusCallbackInfo;
	TAny* iOtgFeatureChangePtr;
	TUsbcOtgFeatureCallback iOtgFeatureCallbackInfo;
	TInt iNumberOfEndpoints;
    RArray<DPlatChunkHw*> iHwChunksEp0;
    RArray<DPlatChunkHw*> iHwChunks;

	TUsbcDeviceState iDeviceState;
	TUsbcDeviceState iOldDeviceState;
	TBool iOwnsDeviceControl;
	TUint iAlternateSetting;
	TBool iDeviceStatusNeeded;
	TUsbcDeviceStatusQueue* iStatusFifo;
	TBool iChannelClosing;
	TVirtualPinObject *iPinObj1;
	TVirtualPinObject *iPinObj2;
	TVirtualPinObject *iPinObj3;
	TClientDataRequest<TUint> *iStatusChangeReq;
	TClientDataRequest<TUint> *iEndpointStatusChangeReq;
	TClientDataRequest<TUint> *iOtgFeatureChangeReq;
	TEndpointTransferInfo iTfrInfo;
	};


#include <drivers/usbc.inl>

#endif	// __USBC_H__
