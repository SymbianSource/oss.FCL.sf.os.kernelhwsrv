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
// in its implementation.
// 
//

/**
 @file The interface to an example data converter device driver which uses Shared Chunks
 @publishedPartner
 @prototype 9.1
*/

#ifndef __CONVERT1_H__
#define __CONVERT1_H__

#include <e32cmn.h>
#include <e32ver.h>
#ifndef __KERNEL_MODE__
#include <e32std.h>
#endif

/**
User interface for 'Convert1'
*/
class RConvert1 : public RBusLogicalChannel
	{
public:
	/**
	Structure for holding driver capabilities information
	*/
	class TCaps
		{
	public:
		TVersion iVersion;
		TInt iMaxChannels; /**< Maximum number of simultaneous channels supported by driver */
		};

	/**
	Structure for holding driver configuration data
	*/
	class TConfig
		{
	public:
		TInt iBufferSize;			/**< Size of convert buffer */
		TBool iCreateInputChunk;	/**< True if driver is to create an input chunk */
		TInt iSpeed;				/**< Speed of converter in bytes/second (for this example test) */
		};
	typedef TPckgBuf<TConfig> TConfigBuf;

#ifndef __KERNEL_MODE__
public:
	RConvert1();
	TInt Open();
	void Close();
	TInt GetConfig(TConfigBuf& aConfig);
	TInt SetConfig(const TConfigBuf& aConfig);
	void Convert(const TDesC8& aInput,TRequestStatus& aStatus);
	void Convert(RChunk aInputChunk,TInt aInputOffset,TInt aInputSize,TRequestStatus& aStatus);
	void Convert(TInt aSize,TRequestStatus& aStatus);
	inline RChunk OutChunk() const;
	inline RChunk InChunk();
	inline TPtr8 InBuffer();
private:
	/**
	Hide the Duplicate() method by making it private.
	The purpose of hiding the method is to prevent it's use because this object also contains
	chunk handles which would need special consideration.
	We don't want to bother supporting Duplicate() for this particular driver because
	it only supports a single client thread so normal use wouldn't require Duplicate().
	*/
	TInt Duplicate(const RThread& aSrc,TOwnerType aType=EOwnerProcess);
#endif

public:
	inline static const TDesC& Name();
	inline static TVersion VersionRequired();
private:
	/**
	Enumeration of Control messages.
	*/
	enum TControl
		{
		EGetConfig,
		ESetConfig,
		EConvertDes,
		EConvertChunk,
		EConvertInChunk,
		};

	/**
	Enumeration of Request messages.
	(None used in this example)
	*/
	enum TRequest
		{
		ENumRequests,
		EAllRequests = (1<<ENumRequests)-1
		};

	/**
	Structure used to package arguments for a EConvertChunk control message
	*/
	struct TConvertArgs
		{
		TInt iChunkHandle;
		TInt iOffset;
		TInt iSize;
		};

	/**
	Structure representing input and output buffers
	*/
	struct TBufferInfo
		{
		TInt iOutChunkHandle; /**< Handle to Shared Chunk used to hold output data */
		TInt iInChunkHandle;  /**< Handle to Shared Chunk used to hold input data */
		TInt iInBufferOffset; /**< Offset within input chunk where the input buffer actually starts */
		TUint8* iInBufferPtr; /**< Calculated address for start of input buffer within client process */
		TInt iInBufferSize;   /**< Size of input buffer in bytes */
		};

	TBufferInfo iBufferInfo;

	// Kernel side LDD channel is a friend
	friend class DConvert1Channel;
	};

/**
  The driver's name

  @return The name of the driver

  @internalComponent
*/
inline const TDesC& RConvert1::Name()
	{
	_LIT(KConvert1Name,"CONVERT1");
	return KConvert1Name;
	}

/**
  The driver's version

  @return The version number of the driver

  @internalComponent
*/
inline TVersion RConvert1::VersionRequired()
	{
	const TInt KMajorVersionNumber=1;
	const TInt KMinorVersionNumber=0;
	const TInt KBuildVersionNumber=KE32BuildVersionNumber;
	return TVersion(KMajorVersionNumber,KMinorVersionNumber,KBuildVersionNumber);
	}

/*
  NOTE: The following methods would normally be exported from a seperate client DLL
  but are included inline in this header file for convenience.
*/

#ifndef __KERNEL_MODE__

/**
  Constructor to clear member data
*/
RConvert1::RConvert1()
	{
	memclr(&iBufferInfo,sizeof(iBufferInfo));
	}

/**
  Open a logical channel to the driver.
  The opened channel may only be used by the calling trhead.

  @return One of the system wide error codes.
*/
TInt RConvert1::Open()
	{
	return DoCreate(Name(),VersionRequired(),KNullUnit,NULL,NULL,EOwnerThread);
	}

/**
  Close a logical channel to the driver
*/
void RConvert1::Close()
	{
	OutChunk().Close();
	InChunk().Close();
	RBusLogicalChannel::Close();
	}

/**
  Get the current configuration settings.

  @param aConfig A structure which will be filled with the configuration settings.

  @return KErrNone
*/
TInt RConvert1::GetConfig(TConfigBuf& aConfig)
	{
	return DoControl(EGetConfig,(TAny*)&aConfig);
	}

/**
  Set the current configuration settings.

  @param aConfig The new configuration settings to be used.

  @return KErrInUse if data convertion is already in progress
          KErrArgument if any configuration values are invalid.
		  KErrNone otherwise

  @post On success, new memory buffers will have been created and mapped into client process.
*/
TInt RConvert1::SetConfig(const TConfigBuf& aConfig)
	{
	OutChunk().Close();
	InChunk().Close();
	TInt r = DoControl(ESetConfig,(TAny*)&aConfig,&iBufferInfo);
	if(r!=KErrNone)
		return r;
	// Sett address of input
	if(InChunk().Handle())
		iBufferInfo.iInBufferPtr = InChunk().Base()+iBufferInfo.iInBufferOffset;
	return r;
	}

/**
  Convert data in the specified descriptor.

  @param aInput A descriptor containing the data to be converted
  @param aStatus The request status signaled when convertion is complete (or on error).
                 The result value is the offset within OutChunk() where the coverted output
				 data resides; or set to one of the system wide error codes when an error
				 occurs:

  @pre The driver must have been previousely initialised by a call to SetConfig()
*/
void RConvert1::Convert(const TDesC8& aInput,TRequestStatus& aStatus)
	{
	DoControl(EConvertDes,(TAny*)&aInput,&aStatus);
	}

/**
  Convert data in the specified chunk.

  @param aInputChunk  The chunk containing the data to be converted
  @param aInputOffset Offset from start of chunk for the start of data to be converted.
  @param aInputSize Number of bytes of data to be converted.
  @param aStatus The request status signaled when convertion is complete (or on error).
                 The result value is the offset within OutChunk() where the coverted output
				 data resides; or set to one of the system wide error codes when an error
				 occurs:

  @pre The driver must have been previousely initialised by a call to SetConfig()
*/
void RConvert1::Convert(RChunk aInputChunk,TInt aInputOffset,TInt aInputSize,TRequestStatus& aStatus)
	{
	TConvertArgs args;
	args.iChunkHandle = aInputChunk.Handle();
	args.iOffset = aInputOffset;
	args.iSize = aInputSize;
	DoControl(EConvertChunk,(TAny*)&args,(TAny*)&aStatus);
	}

/**
  Convert data in the input chunk. I.e. placed in InBuffer().

  @param aSize   Number of bytes of data to be converted.
  @param aStatus The request status signaled when convertion is complete (or on error).
                 The result value is the offset within OutChunk() where the coverted output
				 data resides; or set to one of the system wide error codes when an error
				 occurs:

  @pre The driver must have been previousely initialised by a call to SetConfig()
*/
void RConvert1::Convert(TInt aSize,TRequestStatus& aStatus)
	{
	DoControl(EConvertInChunk,(TAny*)aSize,(TAny*)&aStatus);
	}

/**
  Obtain the chunk into which converted data will be placed.
  This chunk may change after calls to SetConfig().

  @return The chunk

  @pre The driver must have been configured using SetConfig()
       with TConfig::iCreateInputChunk set true.
*/
inline RChunk RConvert1::InChunk()
	{
	return RChunk((RChunk&)iBufferInfo.iInChunkHandle);
	}

/**
  Obtain the chunk into which converted data will be placed.
  This chunk may change after calls to SetConfig().

  @return The chunk

  @pre The driver must have been configured using SetConfig()
*/
inline RChunk RConvert1::OutChunk() const
	{
	return RChunk((RChunk&)iBufferInfo.iOutChunkHandle);
	}

/**
  Get a pointer descriptor for the input buffer.

  @return A pointer descriptor to the input buffer memory.

  @pre The driver must have been configured using SetConfig()
       with TConfig::iCreateInputChunk set true.
*/
inline TPtr8 RConvert1::InBuffer()
	{
	return TPtr8(iBufferInfo.iInBufferPtr,iBufferInfo.iInBufferSize);
	}

#endif  // !__KERNEL_MODE__

#endif

