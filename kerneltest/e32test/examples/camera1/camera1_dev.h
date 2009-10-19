// Copyright (c) 2005-2009 Nokia Corporation and/or its subsidiary(-ies).
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
// its implementation.
// 
//

/**
 @file Kernel side interfaces to example camera device driver which uses Shared Chunks in
 @publishedPartner
 @prototype 9.1
*/

#ifndef __CAMERA1_DEV_H__
#define __CAMERA1_DEV_H__

/**
  Logical Device (factory class) for 'Camera1'
*/
class DCamera1Factory : public DLogicalDevice
	{
public:
	DCamera1Factory();
	~DCamera1Factory();
	//	Inherited from DLogicalDevice
	virtual TInt Install();
	virtual void GetCaps(TDes8& aDes) const;
	virtual TInt Create(DLogicalChannelBase*& aChannel);
	};

/**
  Class representing a single image buffer
*/
class DImageBuffer
	{
public:
	DImageBuffer();
	~DImageBuffer();
	TInt Create(DChunk* aChunk, TInt aOffset, TInt aSize);
public:
	TInt iChunkOffset;			/**< Offset, in bytes, of buffer start within the chunk */
	TInt iSize;					/**< Size of buffer n bytes */
	TPhysAddr iPhysicalAddress;	/**< Physical address of buffer. KPhysAddrInvalid if buffer not physically contiguous */
	TPhysAddr* iPhysicalPages;	/**< List of physical addresses for buffer pages. 0 if buffer is physically contiguous */
	};

/**
  Class representing all of the image buffers
*/
class DCaptureBuffers : public DBase
	{
public:
	static DCaptureBuffers* New(TInt aNumBuffers,TInt aBufferSize);
	void Open();
	void Close();
	void Reset();
	void Purge(DImageBuffer* aBuffer);
	DImageBuffer* ImageForClient();
	DImageBuffer* ImageCaptured();
	DImageBuffer* ImageRelease(TInt aChunkOffset);
	DImageBuffer* InUseImage(TInt aChunkOffset);
private:
	DCaptureBuffers();
	~DCaptureBuffers();
	TInt Create(TInt aNumBuffers,TInt aBufferSize);
	static DImageBuffer* Remove(DImageBuffer** aList);
	static DImageBuffer* Add(DImageBuffer** aList,DImageBuffer* aBuffer);
public:
	DChunk* iChunk;				/**< The chunk which contains the buffers */
	TLinAddr iChunkBase;		/**< Linear address in kernel process for the start of the chunk  */
	TUint32 iChunkMapAttr;		/**< MMU mapping attributes for chunk */
	TInt iNumBuffers;			/**< Number of buffers */
	DImageBuffer* iImageBuffer;	/**< Array of iNumBuffers buffer objects */
	//
	DImageBuffer* iCurrentBuffer;		/**< The buffer currently being filled by image capture */
	DImageBuffer* iNextBuffer;			/**< The buffer to use for next image capture */
	DImageBuffer** iFreeBuffers;		/**< NULL terminated list of free buffers */
	DImageBuffer** iCompletedBuffers;	/**< NULL terminated list of buffers containing captured images */
	DImageBuffer** iInUseBuffers;		/**< NULL terminated list of buffers currently being used by client */
private:
	TInt iAccessCount;			/**< Access count for this object */
	TAny* iBufferLists;			/**< Memory holding lists iFreeBuffers, iCompletedBuffers and iInUseBuffers */
	};

/**
  Logical Channel class for 'Camera1'
*/
class DCamera1Channel : public DLogicalChannelBase
	{
public:
	DCamera1Channel();
	virtual ~DCamera1Channel();
	// Inherited from DLogicalChannelBase
	virtual TInt DoCreate(TInt aUnit, const TDesC8* anInfo, const TVersion& aVer);
	virtual TInt Request(TInt aReqNo, TAny* a1, TAny* a2);
private:
	// Implementation for the differnt kinds of messages sent through RBusLogicalChannel
	TInt DoControl(TInt aFunction, TAny* a1, TAny* a2);
	TInt DoRequest(TInt aNotReqNo, TAny* a1, TAny* a2);
	TInt DoCancel(TUint aMask);
	// Methods for configuration
	TInt GetConfig(TDes8* aConfigBuf);
	TInt SetConfig(const TDesC8* aConfigBuf);
	// Methods for capturing images
	TInt StartCapture();
	TInt EndCapture();
	void CaptureImage(TRequestStatus* aRequestStatus,TInt aReleaseImage);
	void CaptureImageCancel();
	TInt ImageRelease(TInt aChunkOffset);
	static void CaptureDfcTrampoline(TAny* aSelf);
	void CaptureDfc();
	void StateChange(TBool aNewState);
	static void StateChangeDfcTrampoline(TAny* aSelf);
	void StateChangeDfc();
	// Methods which program the camera hardware
	void DoStartCapture();
	void DoEndCapture();
	void DoNextCapture();
private:
	NFastMutex iCaptureMutex;	/**< Mutex to protect access to driver state */

	DCaptureBuffers* iCaptureBuffers;		/**< The image buffers */

	TRequestStatus* iCaptureRequestStatus;	/**< The request status for client CaptureImage request */
	DThread* iCaptureRequestThread;			/**< The client thread which issued a CaptureImage request */

	TDfcQue* iDfcQ;				/**< The DFC queue used for driver functions */
	TDfc iStateChangeDfc;		/**< DFC queued when Start/EndCapture requests are performed */
	TBool iNewState;			/**< True if state change DFC should start image capture, false to stop capture */
	DMutex* iStateChangeMutex;	/**< Mutex which protect Start/EndCapture requests from reenty */
	NFastSemaphore iStateChangeSemaphore;	/**< Semaphore signaled when state change DFC completes */

	RCamera1::TConfig iConfig;	/**< The driver configuration information */
	TBool iCapturing;			/**< Flag which is True when image capture is in progress */

	TInt iCaptureCounter;		/**< Frame counter incremented on each image captured */
	NTimer iCaptureTimer;		/**< Timer used to emulate image capture hardware */
	TInt iCaptureRateTicks;		/**< Number of timer ticks for iCaptureTimer */
	};

#endif

