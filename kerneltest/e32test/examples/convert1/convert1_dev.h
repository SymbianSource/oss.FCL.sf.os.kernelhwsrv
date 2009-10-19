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
// Shared Chunks in its implementation.
// 
//

/**
 @file Kernel side interfaces to example data converter device driver which uses
 @publishedPartner
 @prototype 9.1
*/

#ifndef __CONVERT1_DEV_H__
#define __CONVERT1_DEV_H__

/**
  Logical Device (factory class) for 'Convert1'
*/
class DConvert1Factory : public DLogicalDevice
	{
public:
	DConvert1Factory();
	~DConvert1Factory();
	//	Inherited from DLogicalDevice
	virtual TInt Install();
	virtual void GetCaps(TDes8& aDes) const;
	virtual TInt Create(DLogicalChannelBase*& aChannel);
	// Resource handling methods
	TInt ClaimResource(TInt& aResourceId);
	void ReleaseResource(TInt aResourceId);
private:
	NFastMutex iResourceMutex;	/**< Mutex to protect access to iResourceFlags */
	TUint iResourceFlags;		/**< Bitfield of flags representing device resources available for use.
								     I.e. iResourceFlags&(1<<resourceId) is true if resource 'resourceId' is free. */
	};

/**
  Class representing a buffer of data
*/
class DChunkBuffer
	{
public:
	DChunkBuffer();
	~DChunkBuffer();
	TInt Create(TInt aSize);
	void Destroy();
	TInt SetMaxSize(TInt aMaxSize);
	TInt Open(TAny* aAddress, TInt aSize, TBool aWrite=EFalse);
	TInt Open(TInt aChunkHandle, TInt aOffset, TInt aSize, TBool aWrite=EFalse);
	void Close();
	TInt Copy(TAny* aAddress, TInt aSize);
private:
	TInt SetPhysicalAddresses(TInt aSize);
public:
	DChunk* iChunk;				/**< The chunk which contains the buffer */
	TInt iChunkOffset;			/**< Offset, in bytes, of buffer start within the chunk */
	TInt iMaxSize;				/**< Maximum size of buffer n bytes */
	TLinAddr iChunkBase;		/**< Linear address in kernel process for the start of the chunk  */
	TUint32 iChunkMapAttr;		/**< MMU mapping attributes for chunk */
	TPhysAddr iPhysicalAddress;	/**< Physical address of buffer. KPhysAddrInvalid if buffer not physically contiguous */
	TPhysAddr* iPhysicalPages;	/**< List of physical addresses for buffer pages. 0 if buffer is physically contiguous */
	};

/**
  Logical Channel class for 'Convert1'
*/
class DConvert1Channel : public DLogicalChannelBase
	{
public:
	DConvert1Channel(DConvert1Factory* aFactory);
	virtual ~DConvert1Channel();
	//	Inherited from DObject
	virtual TInt RequestUserHandle(DThread* aThread, TOwnerType aType);
	// Inherited from DLogicalChannelBase
	virtual TInt DoCreate(TInt aUnit, const TDesC8* anInfo, const TVersion& aVer);
	virtual TInt Request(TInt aReqNo, TAny* a1, TAny* a2);
private:
	// Panic reasons
	enum TPanic
		{
		ERequestFromWrongThread=1,
		ERequestAlreadyPending
		};
	// Implementation for the differnt kinds of messages sent through RBusLogicalChannel
	TInt DoControl(TInt aFunction, TAny* a1, TAny* a2);
	TInt DoRequest(TInt aNotReqNo, TAny* a1, TAny* a2);
	TInt DoCancel(TUint aMask);
	// Methods for configuration
	TInt GetConfig(TDes8* aConfigBuf);
	TInt SetConfig(const TDesC8* aConfigBuf,RConvert1::TBufferInfo* aBufferInfo);
	// Methods for capturing images
	void ConvertDes(const TDesC8* aSrc,TRequestStatus* aRequestStatus);
	void ConvertChunk(const RConvert1::TConvertArgs* aSrcArgs,TRequestStatus* aRequestStatus);
	void ConvertInChunk(TInt aSize,TRequestStatus* aRequestStatus);
	void ConvertCancel();
	void ConvertComplete(TInt aResult);
	static void ConvertDfcTrampoline(TAny* aSelf);
	void ConvertDfc();
	// Methods which program the convert hardware
	void DoConvertStart(TInt aOffset,TInt aSize);
	void DoConvertCancel();
private:
	DConvert1Factory* iFactory;	/**< Pointer to device driver factory object */
	TInt iResourceId;			/**< The id of the device hardware resource owned by this channel */

	NFastMutex iConvertMutex;	/**< Mutex to protect access to driver state */

	DChunkBuffer* iSource;		/**< Buffer containing the converter's input data */
	DChunkBuffer iInBuffer;		/**< Buffer into which client supplied data can be copied */
	DChunkBuffer iOutBuffer;	/**< Buffer containing the converter's output data */
	DChunkBuffer iClientBuffer;	/**< Buffer representing client supplied chunk data */

	DThread* iClient;			/**< The single client thread for this channel */
	TRequestStatus* iConvertRequestStatus;	/**< The request status for client ConvertImage request */

	RConvert1::TConfig iConfig;	/**< The driver configuration information */

	NTimer iConvertTimer;		/**< Timer used to emulate image capture hardware */
	};

#endif

