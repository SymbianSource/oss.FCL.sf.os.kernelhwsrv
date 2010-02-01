// Copyright (c) 2004-2009 Nokia Corporation and/or its subsidiary(-ies).
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
// e32\include\drivers\camerasc.h
// 
//

/**
 @file
 @internalAll
 @prototype
*/

#ifndef __CAMERASC_H__
#define __CAMERASC_H__

#include <d32camerasc.h>
#include <platform.h>
#include <kernel/kpower.h>
#include <e32ver.h>

/** The default number of buffers available to the client. */
const TInt KDefaultNumClientBuffers=6;

/** The maximum number of client capture requests which may be outstanding at any time. */
const TInt KMaxCamScRequestsPending=8;

/**
@internalAll
@prototype
*/
struct SBufSpecList
	{
	/** The first entry of the buffer offset list. This list holds the offset from the start of the chunk
	for each buffer. This list is only valid if the flag KScFlagBufOffsetListInUse is set in
	TSharedChunkBufConfigBase::iFlags. */
	TInt iBufferOffset;
	TInt iBufferId;
	};

/**
@internalAll
@prototype
*/
class TCameraSharedChunkBufConfig : public TSharedChunkBufConfigBase
	{
public:
 	struct SBufSpecList iSpec;
	};

// Forward declarations
class TImageBuffer;
class DCameraScLdd;
class DBufferManager;

/**
The physical device driver (PDD) base class for the camera driver.
@internalAll
@prototype
*/
class DCameraScPdd : public DBase
	{
public:
	/**
    Return the DFC queue to be used by this device.
    @param aUnit The unit number for which to get the DFC queue.
    @return The DFC queue to use.
    */
	virtual TDfcQue* DfcQ(TInt aUnit)=0;

	/**
	Return the capabilities of this camera device.
	@param aCapsBuf A packaged TCameraCapsV02 object to be filled with the capabilities of the
		device. This descriptor is in kernel memory and can be accessed directly.
	@see TCameraCapsV02.
	*/
	virtual void Caps(TDes8& aCapsBuf) const=0;

	/**
	Return data format information for a custom camera data format setting. Only required where support is
	required for a data format that isn't supported by the LDD. Platforms which don't require support
	for custom data settings need not implement this method.
	@param aConfigBuf A packaged TCameraConfigV02 object containing the current camera driver configuration
		(including an identifier for the custom setting required). This configuration object should be
		updated by the PDD with the appropriate settings for the data format concerned. This descriptor
		is in kernel memory and can be accessed directly.
	@return KErrNone if successful, otherwise one of the other system wide error codes.
	*/
	virtual TInt SpecifyCustomConfig(TDes8& aConfigBuf);

	/**
	Return the shared chunk create information to be used by this device.
	@param aChunkCreateInfo A chunk create info. object to be to be filled with the settings
							required for this device.
	*/
	virtual void GetChunkCreateInfo(TChunkCreateInfo& aChunkCreateInfo)=0;

	/**
	Configure or reconfigure the device using the the configuration supplied.
	@param aConfigBuf A packaged TCameraConfigV02 object which contains the new configuration settings.
		This descriptor is in kernel memory and can be accessed directly.
	@return KErrNone if successful, otherwise one of the other system wide error codes.
	@see TCameraConfigV02.
	*/
	virtual TInt SetConfig(const TDesC8& aConfigBuf)=0;

	/**
    Start the camera - start pixel sourcing.
    @param aCaptureMode The capture mode to start. @see TDevCamCaptureMode.
    @param aLinAddr The linear address of the start of the first buffer to use for image capture.
	@param aPhysAddr The physical address that corresponds to the linear address: aLinAddr.
    @return KErrNone if successful;
    		otherwise one of the other system wide error codes.
    */
	virtual TInt Start(TDevCamCaptureMode aCaptureMode,TLinAddr aLinAddr,TPhysAddr aPhysAddr)=0;

	/**
	Sets the address of the buffer into which the next image will be captured.
    @param aLinAddr The linear address of the start of the buffer to use to capture the image frame.
	@param aPhysAddr The physical address that corresponds to the linear address: aLinAddr.
    @return KErrNone if the capture has been initiated successfully;
  			KErrNotReady if the device is unable to accept the request for the moment;
		  	otherwise one of the other system-wide error codes.
    */
	virtual TInt CaptureNextImage(TLinAddr aLinAddr,TPhysAddr aPhysAddr)=0;

	/**
    Stop the camera - stop pixel sourcing.
    @return KErrNone if successful, otherwise one of the other system wide error codes.
    */
	virtual TInt Stop()=0;

	/**
	Power down the camera.
	*/
	virtual void PowerDown()=0;

	/**
	Queries the driver for the size of the structure to be passed to RDevCameraSc::Caps().
	*/
	virtual TInt CapsSize()=0;

	/**
	Returns the supported frame sizes that correspond to the desired capture mode and pixel format passed in.
	@param aCaptureMode The capture mode for which to obtain the information.
	@param aUidPixelFormat The pixel format for which to obtain the information.
	@param aFrameSizeCapsBuf An appropriately sized buffer to be filled with the supported frame sizes.
	@return KErrNone, if successful,
			KErrArgument, if an invalid capture mode or pixel format is specified, or if aFrameSizeCapsBuf is too small;
			otherwise one of the other system-wide error codes.
	@see SDevCamFrameSize
	*/
	virtual TInt FrameSizeCaps(TDevCamCaptureMode aCaptureMode, TUidPixelFormat aUidPixelFormat, TDes8& aFrameSizeCapsBuf)=0;

	/**
	Sets the sensor brightness to the desired setting.

	@param aValue A verified brightness setting.
	@return KErrNone if successful, KErrNotSupported if not supported.
	*/
	virtual TInt SetBrightness(TUint aValue) = 0;

	// SYM_BRANCH: Add support for setting of Dynamic Attributes. Contrast.
	/**
	Sets the sensor contrast to the desired setting.

	@param aValue A verified contrast setting.
	@return KErrNone if successful, KErrNotSupported if not supported.
	*/
	virtual TInt SetContrast(TUint aValue) = 0;

	// SYM_BRANCH: Add support for setting of Dynamic Attributes. Colour Effect.
	/**
	Sets the sensor color effect to the desired setting.

	@param aValue A verified color effect setting.
	@return KErrNone if successful, KErrNotSupported if not supported.
	*/
	virtual TInt SetColorEffect(TUint aValue) = 0;

public:
	DCameraScLdd* iLdd;
	};

/**
The logical device (factory class) for the camera driver.
*/
class DCameraScLddFactory : public DLogicalDevice
	{
public:
	DCameraScLddFactory();
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
The class representing a single image buffer.
*/
class TImageBuffer : public SDblQueLink
	{
public:
	TImageBuffer();
	~TImageBuffer();
	TInt Create(DChunk* aChunk,TInt aOffset,TInt aSize,TInt aId,TBool aIsContiguous);
	void SyncMemoryBeforeDmaRead();
	void SyncMemoryAfterDmaRead();
public:
	/** The buffer id */
	TInt iId;
	/** The chunk used for this buffer */
	DChunk* iChunk;
	/** The offset, in bytes, of the start of the buffer within the chunk. */
	TInt iChunkOffset;
	/** The size of the buffer in bytes. */
	TInt iSize;
	/** The virtual address of buffer. */
	TLinAddr iLinearAddress;
	/** The physical address of buffer. KPhysAddrInvalid if the buffer is not physically contiguous. */
	TPhysAddr iPhysicalAddress;
	/** A list of physical addresses for buffer pages. 0 if the buffer is physically contiguous. */
	TPhysAddr* iPhysicalPages;
	/** This is the result of the transfer into this buffer. */
	TInt iResult;
	};

/**
An object encapsulating an image capture request from the client.
*/
class TCameraScRequest : public SDblQueLink
	{
public:
	inline TCameraScRequest()
		{}
public:
	/** The request status associated with the request - used to signal completion of the request and pass back a
	completion code. */
	TRequestStatus* iStatus;
	};

/**
An object encapsulating a queue of image capture requests from the client.
*/
class TCameraScRequestQueue
	{
public:
	TCameraScRequestQueue(NFastMutex* aMutexPtr);
	~TCameraScRequestQueue();
	TInt Create(DThread* anOwningThread);
	TInt Add(TRequestStatus* aStatus);
	TRequestStatus* Remove();
	void Cancel(TRequestStatus* aStatus);
	void CancelAll();
	inline TBool IsEmpty();
private:
	/** The queue of pending capture requests. */
	SDblQue iPendRequestQ;
	/** The queue of unused capture requests. */
	SDblQue iUnusedRequestQ;
	/** The actual array of request objects. */
	TCameraScRequest* iRequest[KMaxCamScRequestsPending];
	NFastMutex* iMutexPtr;
	DThread* iOwningThread;
	};

/**
The buffer manager base class.
*/
class DBufferManager : public DBase
	{
public:
	DBufferManager(DCameraScLdd* aLdd);
	~DBufferManager();
	TInt Create(TInt aNumBuffers,TInt aBufferSize);
	TInt Create(TCameraSharedChunkBufConfig& aBufConfig,TInt aChunkHandle,DThread* anOwningThread);
	void GetBufConfig(TCameraSharedChunkBufConfig& aBufConfig);
	void Reset();
	void Purge(TImageBuffer* aBuffer);
	TImageBuffer* GetImageForClient(TBool aRemoveLast);
	TImageBuffer* SetImageCaptured(TInt aResult);
	TInt ReleaseImage(TInt aChunkOffset);
	TImageBuffer* NextAvailableForCapture();
	TImageBuffer* FindInUseImage(TInt aChunkOffset);
protected:
	TInt CreateBufferLists(TInt aNumBuffers);
	TInt CommitMemoryForBuffer(TInt aChunkOffset,TInt aSize,TBool& aIsContiguous);
protected:
	/** The owning LDD object. */
	DCameraScLdd* iLdd;
	/** The chunk which contains the buffers. */
	DChunk* iChunk;
	/** The linear address in kernel process for the start of the chunk. */
	TLinAddr iChunkBase;
	/**< MMU mapping attributes that the chunk has actually been mapped with. */
	TUint32 iChunkMapAttr;
	/** The number of buffers. */
	TInt iNumBuffers;
	/** The actual array of buffer objects. */
	TImageBuffer* iImageBuffer;
	/** The buffer currently being filled by image capture. (Not in any list). */
	TImageBuffer* iCurrentBuffer;
	/** The next buffer to use for image capture. (Not in any list). */
	TImageBuffer* iNextBuffer;
	/** A queue of those buffers which are currently free. */
	SDblQue iFreeBufferQ;
	/** A queue of those buffers which currently contain captured images (and which aren't being used by the client). */
	SDblQue iCompletedBufferQ;
	/** A queue of those buffers which are currently being used by the client. */
	SDblQue iInUseBufferQ;
private:
	friend class DCameraScLdd;
	};

/**
The configuration class that is specific for each capture mode. This allows the driver to maintain different configurations,
one for each capture mode, and make switching between capture modes faster.
*/
class TCaptureModeConfig
	{
	private:
		/** The handle to the chunk that is returned to the user side code. */
		TInt iChunkHandle;
		/** The current configuration of the capture mode */
		TCameraConfigV02 iCamConfig;
		/** The current configuration of the chunk. */
		TCameraSharedChunkBufConfig* iBufConfig;
		/** The size in bytes of the chunk configuration info. structure. */
		TInt iBufConfigSize;
		/** The current frame height. */
		TInt iFrameHeight;
		/** The current frame width. */
		TInt iFrameWidth;
		/** The buffer manager. */
		DBufferManager* iBufManager;
	private:
		friend class DCameraScLdd;
	};

/**
The camera driver power handler class.
*/
class DCameraScPowerHandler : public DPowerHandler
	{
public:
	DCameraScPowerHandler(DCameraScLdd* aChannel);
	// Inherited from DPowerHandler
	void PowerUp();
	void PowerDown(TPowerState aPowerState);
private:
	DCameraScLdd* iChannel;
	};

/**
The logical channel class for the camera driver.
*/
class DCameraScLdd : public DLogicalChannel
	{
public:
	enum TState
		{
		/** Channel open - but not configured. */
		EOpen,
		/** Channel configured - but inactive. */
		EConfigured,
		/** Channel is active - capturing images. */
		ECapturing
		};
public:
	DCameraScLdd();
	virtual ~DCameraScLdd();
	// Inherited from DLogicalChannel
	virtual TInt DoCreate(TInt aUnit, const TDesC8* anInfo, const TVersion& aVer);
	virtual TInt Request(TInt aReqNo, TAny* a1, TAny* a2);
	virtual void HandleMsg(TMessageBase* aMsg);
	virtual TInt RequestUserHandle(DThread* aThread, TOwnerType aType);
	inline DThread* OwningThread();
	inline TInt CurrentFrameHeight();
	inline TInt CurrentFrameWidth();
	void Shutdown();
	virtual TInt ImageCaptureCallback(TDevCamCaptureMode aCaptureMode,TInt aResult,TLinAddr* aLinAddr,TPhysAddr* aPhysAddr);
	virtual void PanicClientThread(TInt aReason);
private:
	TInt DoControl(TInt aFunction, TAny* a1, TAny* a2);
	TInt DoRequest(TInt aFunction, TRequestStatus* aStatus, TAny* a1, TAny* a2);
	TInt DoCancel(TUint aMask);
	TInt SetCaptureMode(TInt aCaptureMode);
	TInt SetCamConfig(TInt aCaptureMode, const TDesC8* aCamConfigBuf);
	TInt SetBufConfig(TInt aCaptureMode, const TDesC8* aBufferConfigBuf,TInt aChunkHandle);
	TInt SetBufConfig(TInt aCaptureMode, TInt aNumBuffers);
	TInt ChunkClose(TInt aCaptureMode);
	TInt Start();
	TInt NotifyNewImage(TRequestStatus* aStatus);
	TInt ReleaseBuffer(TInt aChunkOffset);
	TInt DoSetConfig(TInt aCaptureMode, const TDesC8* aCamConfigBuf);
	TInt ValidateConfig(TInt aCaptureMode, TCameraConfigV02 &aConfig);
	TInt DoValidateConfig(TCameraCapsV02* aCamCaps, TInt &aCaptureMode, TCameraConfigV02 &aConfig);
	TInt DoStart();
	TInt ReAllocBufferConfigInfo(TInt aCaptureMode, TInt aNumBuffers);
	TInt ReAllocBufferConfigInfo(TInt aNumBuffers);
	TInt GetSensorCaps(TAny* a1);
	TInt GetFrameSizeCaps(TAny* a1, TAny* a2);
	TInt GetDynamicAttribute(TInt aAttribute, TUint& aValue);
	TInt SetDynamicAttribute(TInt aAttribute, TUint aValue);
	inline DCameraScPdd* Pdd();
	static void RestartDfc(TAny* aChannel);
	static void PowerUpDfc(TAny* aPtr);
	static void PowerDownDfc(TAny* aPtr);
private:
	/** An array of configurations for each capture mode. */
	TCaptureModeConfig* iCaptureModeConfig;
	/** The unit number of this channel. */
	TInt iUnit;
	/** The operating state of the channel. */
	TState iState;
	/** A pointer to the owning client thread. */
	DThread* iOwningThread;
	/** The current capture mode of the camera. */
	TDevCamCaptureMode iCaptureMode;
	/** The pending request queue. */
	TCameraScRequestQueue iRequestQueue;
	/** A mutex to protect access to the buffer lists and the pending request list. */
	NFastMutex iMutex;
	/** The camera driver power handler. */
	DCameraScPowerHandler* iPowerHandler;
	/** DFC used to re-start the PDD following a data capture error. */
	TDfc iRestartDfc;
	/** DFC used to handle power down requests from the power manager before a transition into system shutdown/standby. */
	TDfc iPowerDownDfc;
	/** DFC used to handle power up requests from the power manager following a transition out of system standby. */
	TDfc iPowerUpDfc;
	
	// Used as a cache for values successfully set by SetDynamicAttribute().
	TUint iBrightnessValue;
	TUint iContrastValue;
	TUint iColorEffectValue;
	
	friend class DCameraScPowerHandler;
	friend class DBufferManager;
	};

#include <drivers/camerasc.inl>

#endif	// __CAMERASC_H__
