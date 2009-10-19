// Copyright (c) 2006-2009 Nokia Corporation and/or its subsidiary(-ies).
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
// e32\include\drivers\soundsc.h
// Kernel side definitions for the shared chunk sound driver.
//
//

/**
 @file
 @internalTechnology
 @prototype
*/

#ifndef __SOUNDSC_H__
#define __SOUNDSC_H__

#include <d32soundsc.h>
#include <platform.h>
#include <kernel/kpower.h>
#include <e32ver.h>

/** The maximum number of client transfer requests which may be outstanding in a particular direction at any time. */
const TInt KMaxSndScRequestsPending=8;

/**
A bit-mask value for the sound configuration status variable DSoundScLdd::iSoundConfigFlags. This being set signifies
that the sound configuration has been set in hardware by the driver.
*/
const TUint KSndScSoundConfigIsSetup=0x00000001;
/**
A bit-mask value for the sound configuration status variable DSoundScLdd::iSoundConfigFlags. This being set signifies
that the record level / play volume has been set in hardware by the driver.
*/
const TUint KSndScVolumeIsSetup=0x00000002;

// Bit-mask values used when testing the driver
const TUint KSoundScTest_StartTransferError=0x01;
const TUint KSoundScTest_TransferDataError=0x02;
const TUint KSoundScTest_TransferTimeout=0x04;

// Forward declarations
class TAudioBuffer;
class DBufferManager;
class DSoundScLdd;

/**
@publishedPartner
@prototype
*/
class TSoundSharedChunkBufConfig : public TSharedChunkBufConfigBase
	{
public:
	/** The first entry of the buffer offset list. This list holds the offset from the start of the chunk
	for each buffer. This list is only valid if the flag KScFlagBufOffsetListInUse is set in
	TSharedChunkBufConfigBase::iFlags. */
	TInt iBufferOffsetListStart;
	};

/**
The sound driver power handler class.
*/
class DSoundScPowerHandler : public DPowerHandler
	{
public:
	DSoundScPowerHandler(DSoundScLdd* aChannel);
	// Inherited from DPowerHandler
	void PowerUp();
	void PowerDown(TPowerState aPowerState);
private:
	DSoundScLdd* iChannel;
	};

/**
An object encapsulating an audio data transfer - either record or playback.
*/
class TSndScTransfer
	{
public:
	enum TTfState
		{
		/** None of the data for this transfer has been queued on the device. */
		ETfNotStarted,
		/** Some of the data for this transfer has been queued on the device - but there is more that needs queuing. */
		ETfPartlyStarted,
		/** All the data for this transfer has been queued on the device - but transfer is not complete. */
		ETfFullyStarted,
		/** All the data for this transfer has been transferred. */
		ETfDone
		};
public:
	TSndScTransfer();
	void Init(TUint aId,TInt aChunkOffset,TInt aLength,TAudioBuffer* anAudioBuffer);
	inline TInt GetNotStartedLen();
	inline TInt GetStartOffset();
	inline TInt GetLengthTransferred();
	void SetStarted(TInt aLength);
	TBool SetCompleted(TInt aLength);
public:
	/** A value which uniquely identifies a particular audio data transfer. */
	TUint iId;
	/** The status of this transfer. */
	TTfState iTfState;
	/** The audio buffer associated with this transfer. */
	TAudioBuffer* iAudioBuffer;
private:
	/** An offset within the shared chunk indicating the progress of the transfer. Data between the initial offset
	and this value has either been successfully been transferred or has been queued for transfer. */
	TUint iStartedOffset;
	/** An offset within the shared chunk indicating the end of the data to be transferred. */
	TUint iEndOffset;
	/** This holds the count of the number of bytes which have been successfully transferred. */
	TInt iLengthTransferred;
	/** This holds the count of the number of transfer fragments which are currently in progress for this transfer. */
	TInt iTransfersInProgress;
	};

/**
An object encapsulating an audio request from the client - either record or playback.
*/
class TSoundScRequest : public SDblQueLink
	{
public:
	inline TSoundScRequest();
	virtual ~TSoundScRequest();
	virtual TInt Construct();
public:
	/** The thread which issued the request and which supplied the request status. */
	DThread* iOwningThread;
	/** The client request completion object
		This is the base class and may be constructed as a derived type*/
	TClientRequest* iClientRequest;
	};

/**
A play request object.
*/
class TSoundScPlayRequest : public TSoundScRequest
	{
public:
	TSoundScPlayRequest();
	inline void SetFail(TInt aCompletionReason);
	inline void UpdateProgress(TInt aLength);
	TInt Construct();
public:
	/** The transfer information associated with this play request. */
	TSndScTransfer iTf;
	/** The play request flags which were supplied by the client for this request - see KSndFlagLastSample. */
	TUint iFlags;
	/** The error value to be returned when completing the request. */
	TInt iCompletionReason;
	};

/**
An object encapsulating a queue of audio requests from the client.
*/
class TSoundScRequestQueue
	{
public:
	TSoundScRequestQueue(DSoundScLdd* aLdd);
	virtual ~TSoundScRequestQueue();
	virtual TInt Create();
	TSoundScRequest* NextFree();
	void Add(TSoundScRequest* aReq);
	TSoundScRequest* Remove();
	TSoundScRequest* Remove(TSoundScRequest* aReq);
	TSoundScRequest* Find(TRequestStatus* aStatus);
	void Free(TSoundScRequest* aReq);
	inline TBool IsEmpty();
	inline TBool IsAnchor(TSoundScRequest* aReq);
	void CompleteAll(TInt aCompletionReason,NFastMutex* aMutex=NULL);
protected:
	/** The queue of pending audio requests. */
	SDblQue iPendRequestQ;
	/** The queue of unused audio requests. */
	SDblQue iUnusedRequestQ;
	/** The actual array of request objects. */
	TSoundScRequest* iRequest[KMaxSndScRequestsPending];
	/** Mutex used to protect the unused queue from corruption */
	NFastMutex iUnusedRequestQLock;
private:
	/** The owning LDD object. */
	DSoundScLdd* iLdd;
	};

/**
An object encapsulating a queue of play requests from the client.
*/
class TSoundScPlayRequestQueue : public TSoundScRequestQueue
	{
public:
	TSoundScPlayRequestQueue(DSoundScLdd* aLdd);
	virtual TInt Create();
	TSoundScPlayRequest* NextRequestForTransfer();
	TSoundScPlayRequest* Find(TUint aTransferID,TBool& aIsNextToComplete);
	};

/**
The buffer manager base class.
*/
class DBufferManager : public DBase
	{
public:
	enum TFlushOp
		{
		/** Flush before a DMA write. */
		EFlushBeforeDmaWrite,
		/** Flush before a DMA read. */
		EFlushBeforeDmaRead,
		/** Flush after a DMA read. */
		EFlushAfterDmaRead
		};
public:
	DBufferManager(DSoundScLdd* aLdd);
	~DBufferManager();
	TInt Create(TSoundSharedChunkBufConfig* aBufConfig);
	TInt Create(TSoundSharedChunkBufConfig& aBufConfig,TInt aChunkHandle,DThread* anOwningThread);
	void FlushData(TInt aChunkOffset,TInt aLength,TFlushOp aFlushOp);
protected:
	TInt ValidateBufferOffsets(TInt* aBufferOffsetList,TInt aNumBuffers,TInt aBufferSizeInBytes);
	TInt ValidateRegion(TUint aChunkOffset,TUint aLength,TAudioBuffer*& anAudioBuffer);
	TInt CreateBufferLists(TInt aNumBuffers);
	TInt CommitMemoryForBuffer(TInt aChunkOffset,TInt aSize,TBool& aIsContiguous);
	void GetChunkCreateInfo(TChunkCreateInfo& aChunkCreateInfo);
protected:
	/** The owning LDD object. */
	DSoundScLdd* iLdd;
	/** The chunk which contains the buffers. */
	DChunk* iChunk;
	/** The linear address in kernel process for the start of the chunk. */
	TLinAddr iChunkBase;
	/** MMU mapping attributes that the chunk has actually been mapped with. */
	TUint32 iChunkMapAttr;
	/** The number of buffers. */
	TInt iNumBuffers;
	/** The actual array of buffer objects. */
	TAudioBuffer* iAudioBuffers;
	/** The maximum transfer length that the owning audio channel can support in a single data transfer. */
	TInt iMaxTransferLen;
protected:
	friend class DSoundScLdd;
	friend class TAudioBuffer;
	};

/**
The record buffer manager class.
*/
class DRecordBufferManager : public DBufferManager
	{
public:
	DRecordBufferManager(DSoundScLdd* aLdd);
	void Reset();
	inline TAudioBuffer* GetCurrentRecordBuffer();
	inline TAudioBuffer* GetNextRecordBuffer();
	TAudioBuffer* GetBufferForClient();
	TAudioBuffer* SetBufferFilled(TInt aBytesAdded,TInt aTransferResult);
	TAudioBuffer* ReleaseBuffer(TInt aChunkOffset);
protected:
	/** The buffer currently being filled with record data. (Not in any list). */
	TAudioBuffer* iCurrentBuffer;
	/** The next buffer to use to capture record data. (Not in any list). */
	TAudioBuffer* iNextBuffer;
	/** A queue of those buffers which are currently free. */
	SDblQue iFreeBufferQ;
	/** A queue of those buffers which currently contain record data (and which aren't being used by the client). */
	SDblQue iCompletedBufferQ;
	/** A queue of those buffers which are currently being used by the client. */
	SDblQue iInUseBufferQ;
	/** A flag set within SetBufferFilled() each time it is necessary to use a buffer from the completed list rather
	than the free list as the next buffer for capture (i.e. 'iNextBuffer'). */
	TBool iBufOverflow;
private:
	friend class DSoundScLdd;
	};

/**
The class representing a single record/play buffer.
*/
class TAudioBuffer : public SDblQueLink
	{
public:
	TAudioBuffer();
	~TAudioBuffer();
	TInt Create(DChunk* aChunk,TInt aChunkOffset,TInt aSize,TBool aIsContiguous,DBufferManager* aBufManager);
	TInt GetFragmentLength(TInt aChunkOffset,TInt aLengthRemaining,TPhysAddr& aPhysAddr);
	void Flush(DBufferManager::TFlushOp aFlushOp);
public:
	/** The owning buffer manager. */
	DBufferManager* iBufManager;
	/** The offset, in bytes, of the start of the buffer within the chunk. */
	TInt iChunkOffset;
	/** The size of the buffer in bytes. */
	TInt iSize;
	/** The physical address of the start of the buffer. KPhysAddrInvalid if the buffer is not physically contiguous. */
	TPhysAddr iPhysicalAddress;
	/** A list of physical addresses for buffer pages. 0 if the buffer is physically contiguous. */
	TPhysAddr* iPhysicalPages;
	/** Used for record only this is the number of bytes added into this buffer during recording. */
	TInt iBytesAdded;
	/** Used for record only this is the result of the transfer into this buffer. */
	TInt iResult;
	};

/**
The physical device driver (PDD) base class for the sound driver - for either playback or record.
@publishedPartner
@prototype
*/
class DSoundScPdd : public DBase
	{
public:
	/**
	Return the DFC queue to be used by this device.
	@param	aUnit	The record or playback unit for which to return the DFC queue.  This is for use by
			SMP optimised drivers to return different DFC queues for different units that can
			then run on separate CPU cores.
	@return The DFC queue to use.
	*/
	virtual TDfcQue* DfcQ(TInt aUnit)=0;

	/**
	Return the shared chunk create information to be used by this device.
	@param aChunkCreateInfo A chunk create info. object to be to be filled with the settings
							required for this device.
	*/
	virtual void GetChunkCreateInfo(TChunkCreateInfo& aChunkCreateInfo)=0;

	/**
	Return the capabilities of this audio device.
	This includes information on the direction of the device (i.e. play or record), the number of audio channels
	supported (mono, stereo etc), the encoding formats and sample rates supported and so on.
	@param aCapsBuf A packaged TSoundFormatsSupportedV02 object to be filled with the capabilities
		of this device. This descriptor is in kernel memory and can be accessed directly.
	@see TSoundFormatsSupportedV02.
	*/
	virtual void Caps(TDes8& aCapsBuf) const=0;

	/**
	Return the maximum transfer length in bytes that this device can support in a single data transfer.
	(If the device is using the Symbian DMA framework to handle data transfers then the framework handles data
	transfers which exceed the maximum transfer length for the platform. However, some PDD implementations
	may not use the DMA framework).
	@return The maximum transfer length in bytes.
	*/
	virtual TInt MaxTransferLen() const=0;

	/**
	Configure or reconfigure the device using the configuration supplied.
	@param aConfigBuf A packaged TCurrentSoundFormatV02 object which contains the new configuration settings.
		This descriptor is in kernel memory and can be accessed directly.
	@return KErrNone if successful, otherwise one of the other system wide error codes.
	@see TCurrentSoundFormatV02.
	*/
	virtual TInt SetConfig(const TDesC8& aConfigBuf)=0;

	/**
	Set the volume or record level.
	@param aVolume The play volume or record level to be set - a value in the range 0 to 255. The value 255
		equates to the maximum volume and each value below this equates to a 0.5dB step below it.
	@return KErrNone if successful, otherwise one of the other system wide error codes.
	*/
	virtual TInt SetVolume(TInt aVolume)=0;

	/**
	Prepare the audio device for data transfer.
	This may be preparing it either for record or playback - depending on the direction of the channel.
	@return KErrNone if successful, otherwise one of the other system wide error codes.
	*/
	virtual TInt StartTransfer()=0;

	/**
	Initiate the transfer of a portion of data from/to the audio device.
	This may be to either record or playback the data - depending on the direction of the channel. When the transfer is
	complete, the PDD should signal this event using the LDD function PlayCallback() or RecordCallback() as appropriate.
	@param aTransferID A value assigned by the LDD to allow it to uniquely identify a particular transfer fragment.
	@param aLinAddr A linear address within the shared chunk. For playback this is the address of the start of the data
		to be transferred. For record, this is the start address for storing the recorded data.
	@param aPhysAddr The physical address within the shared chunk that corresponds to the linear address: aLinAddr.
	@param aNumBytes The number of bytes to be transferred.
	@return KErrNone if the transfer has been initiated successfully;
  			KErrNotReady if the device is unable to accept the transfer for the moment;
		  	otherwise one of the other system-wide error codes.
	*/
	virtual TInt TransferData(TUint aTransferID,TLinAddr aLinAddr,TPhysAddr aPhysAddr,TInt aNumBytes)=0;

	/**
	Terminate the transfer of a data to/from the device and to release any resources necessary for transfer.
	In the case of playback, this is called soon after the last pending play request from the client has been completed.
	In the case of record, the LDD will leave the audio device capturing record data even when there are no record
	requests pending from the client. Transfer will only be terminated when the client either issues
	RSoundSc::CancelRecordData() or closes the channel. Once this function had been called, the LDD will not issue
	any further TransferData() commands without first issueing a StartTransfer() command.
	*/
	virtual void StopTransfer()=0;

	/**
	Halt the transfer of data to/from the sound device but don't release any resources necessary for transfer.
	In the case of playback, if possible, any active transfer should be suspended in such a way that it can be
	resumed later - starting from next sample following the one last played.
	In the case of record, any active transfer should be aborted. When recording is halted the PDD should signal this event
	with a single call of the LDD function RecordCallback() - reporting back any partial data already received. In this
	case, if transfer is resumed later, the LDD will issue a new TransferData() request to re-commence data transfer.
	@return KErrNone if successful, otherwise one of the other system wide error codes.
	*/
	virtual TInt PauseTransfer()=0;

	/**
	Resume the transfer of data to/from the sound device following a request to halt the transfer.
	In the case of playback, if possible, any transfer which was active when the device was halted should be resumed -
	starting from next sample following the one last played. Once complete, it should be reported using PlayCallback()
	as normal.
	In the case of record, any active transfer would have been aborted when the device was halted so its just a case
	of re-creating the same setup achieved following StartTransfer().
	@return KErrNone if successful, otherwise one of the other system wide error codes.
	*/
	virtual TInt ResumeTransfer()=0;

	/**
	Power up the sound device. This is called when the channel is first opened and if ever the phone is brought out
	of standby mode.
	@return KErrNone if successful, otherwise one of the other system wide error codes.
	*/
	virtual TInt PowerUp()=0;

	/**
	Power down the sound device. This is called when the channel is closed and just before the phone powers down when
	being turned off or going into standby.
	*/
	virtual void PowerDown()=0;

	/**
	Handle a custom configuration request.
	@param aFunction A number identifying the request.
	@param aParam A 32-bit value passed to the driver. Its meaning depends on the request.
	@return KErrNone if successful, otherwise one of the other system wide error codes.
	*/
	virtual TInt CustomConfig(TInt aFunction,TAny* aParam)=0;

	/**
	Calculates and returns the number of microseconds of data transferred since the start of transfer.
	@param aTime	A reference to a variable into which to place the number of microseconds played or recorded.
	@param aState	The current state of the transfer (from TSoundScLdd::TState).  Included so that paused
					and Active transfers can be treated differently.
	@return			KErrNone if successful, otherwise one of the other system wide error codes.
	*/
	virtual TInt TimeTransferred(TInt64& aTime, TInt aState);
protected:
	inline DSoundScLdd* Ldd();
private:
	DSoundScLdd* iLdd;
	friend class DSoundScLdd;
	};

/**
The logical device (factory class) for the sound driver.
*/
class DSoundScLddFactory : public DLogicalDevice
	{
public:
	DSoundScLddFactory();
	virtual TInt Install();
	virtual void GetCaps(TDes8 &aDes) const;
	virtual TInt Create(DLogicalChannelBase*& aChannel);
	TBool IsUnitOpen(TInt aUnit);
	TInt SetUnitOpen(TInt aUnit,TBool aIsOpenSetting);
private:
	/** Mask to keep track of which units have a channel open on them. */
	TUint iUnitsOpenMask;
	/** A mutex to protect access to the unit info. mask. */
	NFastMutex iUnitInfoMutex;
	};

/**
The logical channel class for the sound driver.
*/
class DSoundScLdd : public DLogicalChannel
	{
public:
	enum TState
		{
		/** Channel is open - but not configured. */
		EOpen,
		/** Channel is configured - but inactive. */
		EConfigured,
		/** Channel is active - recording or playing data. */
		EActive,
		/** Channel is paused - recording or playing has been suspended. */
		EPaused
		};
public:
	DSoundScLdd();
	virtual ~DSoundScLdd();
	// Inherited from DLogicalChannel
	virtual TInt DoCreate(TInt aUnit, const TDesC8* anInfo, const TVersion& aVer);
	virtual TInt Request(TInt aReqNo, TAny* a1, TAny* a2);
 	virtual TInt SendMsg(TMessageBase* aMsg);
	virtual void HandleMsg(TMessageBase* aMsg);
	void Shutdown();
	// Functions used by the PDD
	virtual void PlayCallback(TUint aTransferID,TInt aTransferResult,TInt aBytesPlayed);
	virtual void RecordCallback(TUint aTransferID,TInt aTransferResult,TInt aBytesRecorded);
	virtual void NotifyChangeOfHwConfigCallback(TBool aHeadsetPresent);
	virtual TSoundSharedChunkBufConfig* BufConfig();
	virtual TLinAddr ChunkBase();
private:
	// Implementations for the different kinds of requests
	TInt DoControl(TInt aFunction, TAny* a1, TAny* a2,DThread* aThread);
	TInt DoRequest(TInt aFunction, TRequestStatus* aStatus, TAny* a1, TAny* a2,DThread* aThread);
	TInt DoCancel(TUint aMask);
	TInt SetBufferConfig(DThread* aThread);
	TInt SetBufferConfig(TInt aChunkHandle,DThread* aThread);
	TInt SetSoundConfig();
	TInt DoSetSoundConfig(const TCurrentSoundFormatV02& aSoundConfig);
	TInt SetVolume(TInt aVolume);
	TInt PlayData(TRequestStatus* aStatus,TSoundScPlayRequest* aRequest,DThread* aThread);
	TInt RecordData(TRequestStatus* aStatus,TInt* aLengthPtr,DThread* aThread);
	TInt StartRecord();
	TInt ReleaseBuffer(TInt aChunkOffset);
	TInt CustomConfig(TInt aFunction,TAny* aParam);
	TInt ValidateConfig(const TCurrentSoundFormatV02& aConfig);
	TInt ReAllocBufferConfigInfo(TInt aNumBuffers);
	void StartNextPlayTransfers();
	inline void CompletePlayRequest(TSoundScPlayRequest* aReq,TInt aResult);
	void DoCompletePlayRequest(TSoundScPlayRequest* aReq);
	void CompleteAllDonePlayRequests(TSoundScPlayRequest* aReq);
	void HandleCurrentRecordBufferDone(TInt aTransferResult);
	void CompleteRequest(DThread* aThread,TRequestStatus* aStatus,TInt aReason,TClientRequest* aClientRequest=NULL);
	TInt StartNextRecordTransfers();
	void StartPlayEofTimer();
	void CancelPlayEofTimer();
	inline DSoundScPdd* Pdd();
	static void PlayEofTimerExpired(TAny* aChannel);
	static void PlayEofTimerDfc(TAny* aChannel);
	static void PowerUpDfc(TAny* aChannel);
	static void PowerDownDfc(TAny* aChannel);
	TInt PrePlay(TMessageBase* aMsg);
	TInt PreSetBufferChunkCreateOrOpen(TMessageBase* aMsg);
	TInt PreSetSoundConfig(TMessageBase* aMsg);
private:
	/** The handle to the chunk that is returned to the user side code. */
	TInt iChunkHandle;
	/** The handle of the thread that created the chunk referenced by iChunkHandle */
	DThread* iChunkHandleThread;
	/** The unit number of this channel. */
	TInt iUnit;
	/** The data transfer direction for this unit: play or record. */
	TSoundDirection iDirection;
	/** The operating state of the channel. */
	TState iState;
	/** Spare. */
	TInt iSpare;
	/** The buffer manager - managing the shared chunk and the record/play buffers within this. */
	DBufferManager* iBufManager;
	/** A mutex to protect access to the buffer lists and the pending request list. */
	NFastMutex iMutex;
	/** The transfer status of the record buffer currently being filled. */
	TSndScTransfer iCurrentRecBufTf;
	/** The transfer status of the record buffer which is next to be filled. */
	TSndScTransfer iNextRecBufTf;
	/** The current buffer configuration in the play/record chunk. */
	TSoundSharedChunkBufConfig* iBufConfig;
	/** The size in bytes of the play/record buffer configuration info. structure. */
	TInt iBufConfigSize;
	/** The sound driver power handler. */
	DSoundScPowerHandler* iPowerHandler;
	/** DFC used to handle power down requests from the power manager before a transition into system shutdown/standby. */
	TDfc iPowerDownDfc;
	/** DFC used to handle power up requests from the power manager following a transition out of system standby. */
	TDfc iPowerUpDfc;
	/** The capabilities of this device. */
	TSoundFormatsSupportedV02 iCaps;
	/** The current audio configuration of the driver. */
	TCurrentSoundFormatV02 iSoundConfig;
	/** The requested audio configuration of the driver before validation. */
	TCurrentSoundFormatV02 iTempSoundConfig;
	/** A bitmask holding sound configuration status information. */
	TUint32 iSoundConfigFlags;
	/** The current setting for the record level / play volume. */
	TInt iVolume;
	/** The queue of pending play or record requests. */
	TSoundScRequestQueue* iReqQueue;
	/** Used to complete the change of hardware notification. */
	TClientDataRequest<TBool>* iNotifyChangeOfHwClientRequest;
	/** The thread which has registered for the hardware configuration change notifier. */
	DThread* iChangeOfHwConfigThread;
	/** A pointer to the headset present boolean variable in client memory which is updated on a notification. */
	TBool* iHeadsetPresentStatPtr;
	/** To keep track of the number of bytes transferred. */
	TInt iBytesTransferred;
	/** Count of the number of times the PDD completes a record transfer fragment just after being paused. */
	TInt iCompletesWhilePausedCount;
	/** A timer used to delay exiting play transfer mode at EOF. */
	NTimer iEofTimer;
	/** DFC used to delay exiting play transfer mode at EOF. */
	TDfc iPlayEofDfc;
	/** Used for testing and debugging. */
	TUint iTestSettings;
	/** A flag to indicate whether the play EOF timer is active. */
	TBool iPlayEofTimerActive;
	/** Used in debug builds to track that all calls to DThread::Open() are balanced with a close before the driver closes. */
	TInt iThreadOpenCount;
	/** Used to complete requests in the DFC thread. */
	TClientRequest** iClientRequests;

	friend class DBufferManager;
	friend class DSoundScPowerHandler;
	friend class TSoundScRequestQueue;
	};

#include <drivers/soundsc.inl>
#endif	// __SOUNDSC_H__
