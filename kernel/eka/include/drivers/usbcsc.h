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
// e32\include\drivers\usbcsc.h
// Kernel side definitions for the USB Device driver stack (PIL + LDD).
// 
//

/**
 @file usbcsc.h
 @internalTechnology
*/

#ifndef __USBCSC_H__
#define __USBCSC_H__

#include <kernel/kernel.h>
#include <kernel/kern_priv.h>
#include <kernel/kpower.h>
#include <platform.h>

#include <d32usbcsc.h>

#include <drivers/usbcshared.h>

/** LDD Major version, This should agree with the information in RDevUsbcClient::TVer.
*/
const TInt KUsbcScMajorVersion = 0;

/** LDD Minor version, This should agree with the information in RDevUsbcClient::TVer.
*/
const TInt KUsbcScMinorVersion = 1;

/** LDD Build version, This should agree with the information in RDevUsbcClient::TVer.
*/
const TInt KUsbcScBuildVersion = KE32BuildVersionNumber;

/** Must correspond to the max enum of TRequest + 1;
	currently this is ERequestOtgFeaturesNotify = 10.
*/
const TInt KUsbcScMaxRequests = 11;

// Request queue sizes need to be power of 2.

/** The number of requests that can be queued on any IN endpoint */
const TInt KUsbcScInRequests = 4;
/** The number of requests that can be queued on any OUT endpoint */
const TInt KUsbcScOutRequests = 2;

/** In TUsbcScBuffer.iDirection, this indicates that the endpoint is an IN endpoint */
const TInt KUsbcScIn =  0;
/** In TUsbcScBuffer.iDirection, this indicates that the endpoint is an OUT endpoint */
const TInt KUsbcScOut = 1;


/** In TUsbcScBuffer.iDirection, this indicates that the endpoint is an Bidirectional endpoint
currently operating as an IN endpoint */
const TInt KUsbcScBiIn = 2;
/** In TUsbcScBuffer.iDirection, this indicates that the endpoint is an Bidirectional endpoint
currently operating as an OUT endpoint */
const TInt KUsbcScBiOut = 3;

/** The number of directions supported for endpoints, other then EP0.  Currently 2, IN and OUT. */
const TInt KUsbcScDirections = 2;

/** In TUsbcScBuffer::iDirection, this indicates that the endpoint direction is Unknown. */
const TInt KUsbcScUnknown = 4;

const TInt KPageSize = 0x1000;

/** The default buffer size requested for a endpoint, if the user app does not specify a size.*/
const TInt KUsbcScDefaultBufferSize = 0x10000; // 64k

/** The size of the unmapped region of memory between endpoint buffers.
This serves as a guard region, making memory over/under runs more obviose.*/
const TInt KGuardSize = KPageSize;

/** The size put aside for the chunk header structre.*/
const TInt KHeaderSize = KPageSize;

/** For buffers of size >= KUsbScBigBuffIs, The smallest unit of continiouse memory that will be used.
No read will be set up smaller then this, to avoid overly fragmenting the data.
*/
const TInt KUsbSc_BigBuff_MinimumRamRun = KPageSize;

/** For buffers of size < KUsbScBigBuffIs, The smallest unit of continiouse memory that will be used.
No read will be set up smaller then this, to avoid overly fragmenting the data.*/
const TInt KUsbSc_SmallBuff_MinimumRamRun = 1024;

/** The size a buffer request has to be to switch to using KUsbSc_BigBuff_MinimumRamRun.
If the requested buffer is smaller, then the smallest memory allocated to a buffer is KPageSize*/
const TInt KUsbScBigBuffIs = KPageSize*6;



// EP0 is mapped manually, unlike the other endpoints.

/** The position, within the chunk, that the EP0 IN buffer appears*/
const TInt KUsbScEP0InBufPos = 0x800;
/** The position, within the chunk, that the EP0 IN buffer ends*/
const TInt KUsbScEP0InBufEnd = KUsbScEP0InBufPos + 0x400;

// Its better for Out to go 2nd, so gaurd page after it.
/** The position, within the chunk, that the EP0 OUT buffer appears*/
const TInt KUsbScEP0OutBufPos = 0xc00;
/** The position, within the chunk, that the EP0 OUT buffer ends*/
const TInt KUsbScEP0OutBufEnd = KUsbScEP0OutBufPos + 0x400;

/** The number of the entry within the chunk BufferRecord table, for the OUT ep0 buffer.*/
const TInt KUsbcScEp0OutBuff = 0;
/** The number of the entry within the chunk BufferRecord table, for the IN ep0 buffer.*/
const TInt KUsbcScEp0InBuff = 1;


//
//########################### Logical Device Driver (LDD) #############################
//

/** USB LDD factory class.
*/
class DUsbcScLogDevice : public DLogicalDevice
	{
public:
	DUsbcScLogDevice();
	virtual TInt Install();
	virtual void GetCaps(TDes8& aDes) const;
	virtual TInt Create(DLogicalChannelBase*& aChannel);
	};


class DLddUsbcScChannel;
class TUsbcScBuffer;
/** Endpoint tracking for the LDD buffering etc.
*/
class TUsbcScEndpoint
	{
public:
	TUsbcScEndpoint(DLddUsbcScChannel* aLDD, DUsbClientController* aController,
				  const TUsbcScEndpointInfo* aEndpointInfo, TInt aEndpointNum
				  );
	~TUsbcScEndpoint();
	TInt Construct();
	void CancelTransfer(DThread* aThread);
	void AbortTransfer();
	inline TUsbcScEndpointInfo* EndpointInfo();
	inline TInt RxBytesAvailable() const;
	inline void ResetTransferInfo();
	inline void SetClientReadPending(TBool aVal);
	inline void SetClientWritePending(TBool aVal);
	inline TBool ClientWritePending();
	inline TBool ClientReadPending();
	inline void SetRealEpNumber(TInt aRealEpNumber);
	inline TInt RealEpNumber() const;
	inline TInt EpNumber() const;
	inline void StartBuffer();
	inline void SetBuffer(TUsbcScBuffer* aBuffer);
	inline TUsbcScBuffer* GetBuffer();


private:
	static void RequestCallback(TAny* aTUsbcScEndpoint);
	void TxComplete();
	TInt RxComplete(TBool aReEntrant);
	void RxCompleteNow();



public:
	TUsbcRequestCallback* iRequestCallbackInfo;

private:
	DUsbClientController* iController;
	TUsbcScEndpointInfo iEndpointInfo;
	TBool iClientReadPending;
	TBool iClientWritePending;
	TInt iEndpointNumber;
	TInt iRealEpNumber;
	DLddUsbcScChannel* iLdd;
	TInt iError;
	TUint32 iBytesTransferred;
	TInt iBandwidthPriority;
	TUsbcScBuffer* iBuffer;

	};


/** Linked list of 'alternate setting' info for use by the LDD.
*/
class TUsbcScAlternateSetting
	{
public:
	TUsbcScAlternateSetting();
	~TUsbcScAlternateSetting();

public:
	TUsbcScAlternateSetting* iNext;
	TUsbcScAlternateSetting* iPrevious;
	TInt iNumberOfEndpoints;
	TUint iSetting;
	TUsbcScEndpoint* iEndpoint[KMaxEndpointsPerClient + 1];
	};

class TUsbcScAlternateSettingList
	{
public:
	TUsbcScAlternateSettingList();
	~TUsbcScAlternateSettingList();

public:
	TUsbcScAlternateSetting* iHead;
	TUsbcScAlternateSetting* iTail;
	};

class TUsbcScChunkInfo
	{
public:
	TUsbcScChunkInfo(DLogicalDevice* aLdd);
	TInt CreateChunk(TInt aTotalSize);
	void Close();
	TInt ChunkAlloc(TInt aOffset, TInt aSize);
	void ChunkCleanup();
	TInt GetPhysical(TInt aOffset, TPhysAddr* aPhysical);
	static TInt New(TUsbcScChunkInfo*& aChunk, TInt aSize, DLogicalDevice* aLdd);
private:
	TUint* iPhysicalMap;
public:
	DChunk* iChunk;
	TDfc iCleanup;

	TInt8 iPageNtz; // Number of trailing zeros for a page. (Eg 4k page has 12 t.z.)
	TInt iAllocatedSize;
	TInt8* iChunkMem;
	TUint32 iChunkMapAttr;
	DLogicalDevice* iLdd;
	};


// Used to represent a matrix of endpoints with a column of sizes.
// Used by TRealizeInfo

class TEndpointSortBufs
	{
	public:
		TUsbcScEndpoint** iEp;
		TInt* iSizes;
		TInt iEps;
	};

// This is used to calculate the layout of the shared chunk
// based on a list of alternative settings / endpoints provided.
 
class TRealizeInfo
	{
	public:
		void Init(TUsbcScAlternateSettingList* aAlternateSettingList);
		TInt CopyAndSortEndpoints();
		void CalcBuffSizes();
		void Free();

		void LayoutChunkHeader(TUsbcScChunkInfo* aChunkInfo);
	public:
		TInt iMaxEndpoints;
		TInt iTotalSize;
		TInt iTotalBuffers;
		TInt iAltSettings;
		TEndpointSortBufs iBufs[KUsbcScDirections];
		TUsbcScAlternateSettingList* iAlternateSettingList;

		// Chunk layout info.
		TUsbcScChunkBuffersHeader* iChunkStuct;
		TUsbcScChunkAltSettingHeader* iAltSettingsTbl;
	};



/** The channel class - the actual USB LDD.
*/
class DLddUsbcScChannel : public DLogicalChannel
	{
public:
	DLddUsbcScChannel();
	~DLddUsbcScChannel();
	virtual void HandleMsg(TMessageBase* aMsg);
	virtual TInt DoCreate(TInt aUnit, const TDesC8* aInfo, const TVersion& aVer);
	virtual TInt RequestUserHandle(DThread* aThread, TOwnerType aType);
	inline DThread* Client() const {return iClient;}
	inline TBool ChannelClosing() const {return iChannelClosing;}
	inline TUint AlternateSetting() const {return iAlternateSetting;}
	
	static void RequestCallbackEp0(TAny* aTUsbcScChannel);

private:

	TInt DoCancel(TInt aReqNo, TUint aMask, TUint a1);
	TInt DoRequest(TInt aReqNo, TRequestStatus* aStatus, TAny* a1, TAny* a2);
	TInt DoControl(TInt aFunction, TAny* a1, TAny* a2);
	TInt DoReadDataNotify(TRequestStatus* aStatus, TInt aBufferNum, TInt aLength);
	void StartDataRead(TInt aBufferNum);
	TInt DoWriteData(TRequestStatus* aStatus,TInt aBufferNum, TUint aStart, TUint aLength, TUint aFlags);
	TBool AlternateDeviceStateTestComplete();
	TInt SetInterface(TInt aInterfaceNum, TUsbcScIfcInfo* aUserInterfaceInfoBuf);
	void StartEpReads();
	void DestroyAllInterfaces();
	void DestroyInterface(TUint aInterface);
	void DestroyEp0();
	inline TBool ValidEndpoint(TInt aEndpoint);
	TInt GetRealEpForEpResource(TInt aEndpoint, TInt& aRealEp);
	inline TBool Configured();
	TInt DoEmergencyComplete();
	void ReadDes8(const TAny* aPtr, TDes8& aDes);
	TInt SetupEp0();
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
	TInt32 StartNextInAlternateSetting();
	TInt ProcessDeviceState(TUsbcDeviceState aDeviceState);
	void ResetInterface(TInt aErrorCode);
	void PanicClientThread(TInt aReason);

	TInt RealizeInterface(void);

private:
	DUsbClientController* iController;
	DThread* iClient;
	TBool iValidInterface;
	
	TUsbcScAlternateSettingList* iAlternateSettingList;
	TUsbcScEndpoint** iEndpoint;  // Pointer to the current endpoint set.

	static const TInt KUsbcMaxRequests = RDevUsbcScClient::ERequestMaxRequests;
	TRequestStatus* iRequestStatus[KUsbcMaxRequests];
	TUsbcClientCallback iCompleteAllCallbackInfo;
	TAny* iStatusChangePtr;
	TUsbcStatusCallback iStatusCallbackInfo;
	TAny* iEndpointStatusChangePtr;
	TUsbcEndpointStatusCallback iEndpointStatusCallbackInfo;
	TAny* iOtgFeatureChangePtr;
	TUsbcOtgFeatureCallback iOtgFeatureCallbackInfo;
	TUint8* iBufferBaseEp0;
	TInt iBufferSizeEp0;
	TInt iNumberOfEndpoints;
	TUsbcDeviceState iDeviceState;
	TUsbcDeviceState iOldDeviceState;
	TBool iOwnsDeviceControl;
	TUint16 iAlternateSetting;
	TUint16 iAsSeq;

	TUsbcDeviceStatusQueue* iStatusFifo;
	TBool iUserKnowsAltSetting;
	TBool iDeviceStatusNeeded;
	TBool iChannelClosing;
	TBool iRealizeCalled;

	TUsbcScChunkInfo* iChunkInfo;
	TInt iNumBuffers;
	TUsbcScBuffer *iBuffers;

	TUsbcScEndpoint* iEp0Endpoint;
	TInt iEP0InBuff;
	TInt iEP0OutBuff;

	friend class TUsbcScBuffer;
	friend void TUsbcScEndpoint::AbortTransfer();
	};

/**
This class is used by TUsbcScStatusList to form a queue of status requests.
These requests are on a buffer basis, so that all buffers can have at least two requests
pending, at the same time. (i.e. buffer 1 could have two requests outstanding, as well as 2 on buffer 2.)
*/

class TUsbcScStatusElement
{
public:
	TRequestStatus* iStatus;
	TInt iLength;
	TUint iStart;
	TUint iFlags;	
};

enum TUsbcScStatusState
{
	ENotRunning,
	EInProgress,
	EReadingAhead,
	EFramgementInProgress
};

class TUsbcScStatusList
{
public:
	TInt Construct(TInt aSize, DThread* aThread);
	void Destroy();

	TUsbcScStatusElement* Next();
	void Pop();
	TInt Add(TRequestStatus* aStatus, TInt aLength, TUint aStart, TUint aFlags);
	void CancelQueued(TInt aErrorCode=KErrCancel);
	TInt Complete(TInt aError);
	void Complete();
public:
	TUsbcScStatusState iState;

private:
	DThread* iClient;
	TInt iHead;   // The element at the head of the queue, ie, the earliest added, and next to be removed.
	TInt iLength; // Length of queue, ie number of elements within
	TInt iSize;   // size of array, ie, max # of requests at a time.
	TUsbcScStatusElement* iElements;

};



/**
This class holds the kernel's copy of all the details related to a shared endpoint buffer,
and provides methods for the LDD to manipulate it.
*/
class TUsbcScBuffer
{
public:
	static const TInt8 KNoEpAssigned=0;
	static const TInt8 KEpIsEnding=1;
	static const TInt8 KEpIsStarting=2;

public:
	TInt Construct(TInt aDirection, DLddUsbcScChannel* aLdd, TInt aBufferOffset, TInt aBufferEndOffset, TInt aMinReadSize, TInt aMaxPacketSize, TInt aMaxReadSize);
	void CreateChunkBufferHeader();
	void StartEndpoint(TUsbcRequestCallback* iRequestInfo, TUint iFlags);

	void Destroy();

	TInt StartDataRead();
	void CompleteRead(TBool aStartNextRead=ETrue);
	void PopStall();
	void StartDataWrite();
	void CompleteWrite();
	void Cancel(TInt aErrorCode);

	void UpdateBufferList(TInt aByteCount,TUint aFlags, TBool aStartNextRead=ETrue);
	void Ep0CancelLddRead();
	void SendEp0StatusPacket(TInt aState);

public:
	
	TInt8 iDirection;
	TInt8 iMode;
	TInt8 iNeedsPacket;
	TInt8 iReserved;
	DLddUsbcScChannel* iLdd;
	TLinAddr iChunkAddr;
	SUsbcScBufferHeader* iBufferStart;
	TUint iBufferEnd; // One word on from the last word in the buffer.
	TUint iAlignMask;
	TUsbcScStatusList iStatusList;
	TUsbcRequestCallback* iCallback;
	union 
	{
		TInt iHead; // Out endpoints only;
		TUint iSent; // In endpoints only
	};
	TUsbcScChunkInfo* iChunkInfo;
	TInt iMinReadSize;
	TInt iMaxReadSize;
	TInt iMaxPacketSize;  // 0 indicates unconfiured.
	TInt iFirstPacket;
	TInt iStalled;

	// needed for backwards compatibility
	TUsbcPacketArray iIndexArray[KUsbcDmaBufNumMax]; // Has 2 elements
	TUsbcPacketArray iSizeArray[KUsbcDmaBufNumMax];  // Has 2 elements
#ifdef _DEBUG
	TUint iSequence;
#endif

};



#include <drivers/usbcsc.inl>

#endif	// __USBCSC_H__
