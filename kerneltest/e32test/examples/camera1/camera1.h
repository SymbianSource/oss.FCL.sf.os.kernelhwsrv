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
 @file The interface to an example camera device driver which uses Shared Chunks
 @publishedPartner
 @prototype 9.1
*/

#ifndef __CAMERA1_H__
#define __CAMERA1_H__

#include <e32cmn.h>
#include <e32ver.h>
#ifndef __KERNEL_MODE__
#include <e32std.h>
#endif

/**
User interface for 'Camera1'
*/
class RCamera1 : public RBusLogicalChannel
	{
public:
	/**
	Structure for holding driver capabilities information
	(Just a version number in this example.)
	*/
	class TCaps
		{
	public:
		TVersion iVersion;
		};

	/**
	Structure for holding driver configuration data
	*/
	class TConfig
		{
	public:
		TSize iImageSize;			/**< Size of image in pixels */
		TInt iImageBytesPerPixel;	/**< Number of bytes used to represent a single pixel */
		TInt iFrameRate;			/**< Speed to capture images at in frames per second*/
		TInt iNumImageBuffers;		/**< Number of simultanious images the client wishes to process */
		};
	typedef TPckgBuf<TConfig> TConfigBuf;

#ifndef __KERNEL_MODE__
public:
	TInt Open();
	void Close();
	TInt GetConfig(TConfigBuf& aConfig);
	TInt SetConfig(const TConfigBuf& aConfig);
	TInt StartCapture();
	TInt EndCapture();
	void CaptureImage(TRequestStatus& aStatus, TInt aReleaseImage=-1);
	void CaptureImageCancel();
	TInt ReleaseImage(TInt aReleaseImage);
	TInt Duplicate(const RThread& aSrc,TOwnerType aType=EOwnerProcess);
	inline RChunk ImageChunk() const;
private:
	RChunk iChunk;   /**< The chunk into which captured images will be placed */
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
		EStartCapture,
		EEndCapture,
		ECaptureImage,
		EReleaseImage
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

	// Kernel side LDD channel is a friend
	friend class DCamera1Channel;
	};

/**
  The driver's name

  @return The name of the driver

  @internalComponent
*/
inline const TDesC& RCamera1::Name()
	{
	_LIT(KCamera1Name,"CAMERA1");
	return KCamera1Name;
	}

/**
  The driver's version

  @return The version number of the driver

  @internalComponent
*/
inline TVersion RCamera1::VersionRequired()
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
  Open a logical channel to the driver

  @return One of the system wide error codes.
*/
TInt RCamera1::Open()
	{
	return DoCreate(Name(),VersionRequired(),KNullUnit,NULL,NULL,EOwnerProcess);
	}

/**
  Close a logical channel to the driver
*/
void RCamera1::Close()
	{
	iChunk.Close();
	RBusLogicalChannel::Close();
	}

/**
  Get the current configuration settings.

  @param aConfig A structure which will be filled with the configuration settings.

  @return KErrNone
*/
TInt RCamera1::GetConfig(TConfigBuf& aConfig)
	{
	return DoControl(EGetConfig,(TAny*)&aConfig);
	}

/**
  Set the current configuration settings.

  @param aConfig The new configuration settings to be used.

  @return KErrInUse if image capturing is already in progress
          KErrArgument if any configuration values are invalid.
		  KErrNone otherwise

  @post On success, iChunk will contain the handle of the chunk used to
        contain captured images.
*/
TInt RCamera1::SetConfig(const TConfigBuf& aConfig)
	{
	iChunk.Close(); // The following call will give us a new handle
	return iChunk.SetReturnedHandle(DoControl(ESetConfig,(TAny*)&aConfig));
	}

/**
  Start the image capture process.

  @return KErrNotReady if SetConfig() has not been previously called.
          KErrNone otherwise.

  @pre The driver must have been previousely initialised by a call to SetConfig()
*/
TInt RCamera1::StartCapture()
	{
	return DoControl(EStartCapture);
	}

/**
  End the image capturing process.
  Also performs CaptureImageCancel()
*/
TInt RCamera1::EndCapture()
	{
	return DoControl(EEndCapture);
	}

/**
  Get the next available image and optionally release an already captured image.
  Only one request may be pending at any time.

  @param aStatus The request status signaled when an image is available (or on error).
                 The result value is the offset within iChunk where the capture image resides;
		         or set to one of the system wide error codes when an error occurs:
				 KErrNotReady if StartCapture() hasn't been previousely called,
				 KErrInUse if there is already a pending CaptureImage() request,
				 KErrOverflow if the client already has all the images buffers.

  @param aReleaseImage The chunk offset of an image which the client has finished processing.
                       Set to -1 to indicate 'no image'

  @pre Image capturing must have been started with StartCapture()
*/
void RCamera1::CaptureImage(TRequestStatus& aStatus,TInt aReleaseImage)
	{
	aStatus=KRequestPending;
	DoControl(ECaptureImage,(TAny*)&aStatus,(TAny*)aReleaseImage);
	}

/**
  Cancel a previous CaptureImage() request
*/
void RCamera1::CaptureImageCancel()
	{
	DoCancel(1<<ECaptureImage);
	}

/**
  Release an already captured image.

  This makes the images buffer available again for the driver to capture images into.

  @param aReleaseImage The chunk offset of the image to be released.
                       This is a value returned by a CaptureImage() request.
*/
TInt RCamera1::ReleaseImage(TInt aReleaseImage)
	{
	return DoControl(EReleaseImage,(TAny*)aReleaseImage);
	}

/**
  Override of RHandleBase::Duplicate.
  This takes care of also duplicating other resources owned by this object.
  @param aSrc  A reference to the thread containing the handle which is to be 
               duplicated for this thread.
  @param aType An enumeration whose enumerators define the ownership of this 
               handle. If not explicitly specified, EOwnerProcess is taken
               as default.
  @return KErrNone, if successful; otherwise, one of the other system wide error 
          codes.
  @see RHandleBase::Duplicate
*/
TInt RCamera1::Duplicate(const RThread& aSrc,TOwnerType aType)
	{
	// Duplicate handle to channel
	TInt r=RHandleBase::Duplicate(aSrc,aType);
	if(r==KErrNone && iChunk.Handle()!=KNullHandle)
		{
		// Duplicate handle to chunk
		r = iChunk.Duplicate(aSrc,aType);
		if(r!=KErrNone)
			RHandleBase::Close(); // Undo channel open
		}
	if(r!=KErrNone)
		iChunk.SetHandle(KNullHandle); // On error, clear chunk handle
	return r;
	}

/**
  Obtain the chunk into which captured images will be placed.
  This chunk may change after calls to SetConfig().

  @return The chunk

  @pre The driver must have been configured using SetConfig()
*/
inline RChunk RCamera1::ImageChunk() const
	{
	return iChunk;
	}

#endif  // !__KERNEL_MODE__

#endif

