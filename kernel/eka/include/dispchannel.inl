// Copyright (c) 2007-2009 Nokia Corporation and/or its subsidiary(-ies).
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
//

/**
 @file dispchannel.inl
 @publishedPartner
 @released
*/

#ifndef __DISPCHANNEL_INL__
#define __DISPCHANNEL_INL__

#include <dispchannel.h>

#ifndef __KERNEL_MODE__

/** Creates a connection to the DisplayChannel Driver logical device driver.
    This handle will need to be closed by calling Close()
    when the connection to the Display Driver is no longer required.

    @param aScreen  Identifies the screen this object will control
    @return KErrNone if successful, otherwise one of the system-wide error codes.
*/
inline TInt RDisplayChannel::Open(TUint aScreen)
	{
	TVersion versionAlwaysSupported(KDisplayChMajorVersionNumberAlwaysSupported,
                                    KDisplayChMinorVersionNumberAlwaysSupported,
                                    KDisplayChBuildVersionNumberAlwaysSupported);
	
    TInt r = DoCreate(Name(), versionAlwaysSupported, aScreen, NULL, NULL);
	return r;
	}

/** The connection is closed and the driver reverts back to legacy behaviour,
    i.e. Non-GCE mode where the legacy buffer is used as the screen output
*/
inline void RDisplayChannel::Close(void)
	{
  	RBusLogicalChannel::Close();
	}

/** Get the static details of the display-owned composition buffer(s).
    These are used with the address returned by GetCompositionBuffer()
    to describe the current composition buffer.

    The normal and flipped size information reflects the maximum direct access display resolution,
    rather than the current resolution. The offset between lines remains constant,
    regardless of current buffer format. See NextLineOffset() for specific buffer
    format details.

    The available rotations indicates all the possible rotations, across all
    resolutions, even if not all are supported in all cases.

    @see SetResolution
    @see SetBufferFormat
    @see NextLineOffset

    @param aInfo The static details of the composition buffer(s).
    @return KErrNone if successful, otherwise one of the system-wide error codes.
*/
inline TInt	RDisplayChannel::GetDisplayInfo(TDes8& aInfo)
	{
	return (DoControl(ECtrlGetDisplayInfo, &aInfo));
    }

/** Return the access details for a composition buffer, opening a new handle to the chunk containing
    the composition buffer.  Note that because all returned information is static, this function will
    normally only be called called once for each composition buffer when the RDisplayChannel is opened.

    @param aBufferIndex  The index of a composition buffer.
    @param aChunk A new open handle to chunk containing the composition buffer indexed by aBufferIndex.
    @param aChunkOffset The offset from the base address of aChunk to the composition buffer indexed by aBufferIndex.
    @return KErrNone if this command completed successfully, otherwise this is another of the system-wide error codes.
*/
inline TInt RDisplayChannel::GetCompositionBufferInfo(TUint aBufferIndex, RChunk& aChunk, TInt& aOffset)
	{
	TInt arg[2] = {0,0};
	(DoControl(ECtrlGetCompositionBufferInfo, &aBufferIndex, &arg));
    aChunk.SetHandle(arg[0]);
    aOffset = arg[1];
	return KErrNone;
	}

/** Request the driver to identify a driver-owned composition buffer for the user to render into.
    This may not be completed until the next display refresh if the system is implementing multi-buffering
    and there are no composition buffers available immediately.

    @param  aBufferIndex On completion of aStatus, contains the buffer index of driver-owned buffer suitable
    for client rendering.  The buffer access details can be retrieved by calling GetCompositionBufferInfo().
    The remaining buffer attributes are described by GetDisplayInfo()
    @param aStatus On completion, contains the status of the request. This is KErrNone if 
    the system is multi-buffered and an available buffer was returned.  
    This is KErrNone is the system is single-buffered and the primary buffer was returned.  
    Otherwise this is another of the system-wide error codes.
*/
inline void	RDisplayChannel::GetCompositionBuffer(TUint& aIndex, TRequestStatus& aStatus)
	{
	DoRequest(EReqGetCompositionBuffer, aStatus, (TAny*)&aIndex);
	}

/** Cancel the outstanding request to GetCompositionBuffer()
*/
inline void	RDisplayChannel::CancelGetCompositionBuffer(void)
	{
	DoCancel(1<<ECtrlCancelGetCompositionBuffer);
	}

/** Tells the driver to queue a request to show the composition buffer on screen at the next display refresh.

    @param aRegion The region changed by the client.  The driver may choose to optimise posting using this
    information or it may ignore it.  Up to KMaxRectangles rectangles can be specified.  If null, the whole buffer is used.
    @param aPostCount This is an identifier returned by the driver for this Post request on exiting the method
*/
inline TInt RDisplayChannel::PostCompositionBuffer(const TRegionFix<TDisplayInfo::KMaxRectangles>* aRegion, TPostCount& aCount)
	{
	return (DoControl(ECtrlPostCompositionBuffer, (TAny*)aRegion, &aCount));
	}

/** Request the driver to show the legacy buffer on screen at the next display refresh.
    @param aPostCount This is an identifier returned by the driver for this Post request on exiting the method.
    @return KErrNone if the request was queued successfully, otherwise this is another of the system-wide error codes.
*/
inline TInt	RDisplayChannel::PostLegacyBuffer(const TRegionFix<TDisplayInfo::KMaxRectangles>* aRegion, TPostCount& aCount)
	{
	return (DoControl(ECtrlPostLegacyBuffer, (TAny*)aRegion, &aCount));
	}

/** Register a user-provided buffer for use by the driver.  This enables the user to subsequently post the buffer. 
    The number of registered buffers will need to have a limit and this is set to KMaxUserBuffers.

    @param aBufferId Identifier to be used in the call to PostUserBuffer().
    @param aChunk Chunk containing memory to be used by the driver.
    @param aOffset Byte offset of the buffer from the chunk base.
    @return KErrNone if the buffer has successfully registered.  
    KErrTooBig if the number of registered buffers is at its limit when this call was made.  
    Otherwise this is another of the system-wide error codes.
*/
inline TInt RDisplayChannel::RegisterUserBuffer(TBufferId& aBufferId, const RChunk& aChunk, TInt aOffset)
	{
	TInt arg[2] = {aChunk.Handle(), aOffset};
	return (DoControl(ECtrlRegisterUserBuffer, arg, &aBufferId));
	}

/** Request the driver to show a buffer on screen at the next display refresh and notify the user when the buffer 
    is no longer being displayed or has been dropped. Note that if two successive calls are made to PostUserBuffer() 
    in between vertical sync pulses, the latest PostUserBuffer() call will supersede the previous call and will complete 
    any outstanding request status object for that call with KErrCancel

    @param aBufferId Identifier representing a buffer.  Generated by a previous call to RegisterUserBuffer().
    @param aStatus On completion the submitted buffer is no longer in use by the display hardware, unless the same 
    buffer is posted a second time before it has completed. aStatus contains the status of the request. This is KErrNone 
    if the request was executed successfully.  This is KErrCancel if this posting request was superseded by a more recent request.  
    This is KErrArgument if aBufferId is not a registered buffer. Otherwise this is another of the system-wide error codes.
    @param aRegion The region changed by the client.  The driver may choose to optimise posting using this information or it may ignore it.  
    Up to KMaxRectangles rectangles can be specified.  If null, the whole buffer is used.
    @param aPostCount This is an identifier returned by the driver for this Post request on exiting the method.
*/
inline void RDisplayChannel::PostUserBuffer(TBufferId aBufferId, TRequestStatus& aStatus, const TRegionFix<TDisplayInfo::KMaxRectangles>* aRegion, TPostCount& aCount)
	{
	TInt arg[2] = {aBufferId, reinterpret_cast<const TInt>(aRegion)};
	DoRequest(EReqPostUserBuffer, aStatus, arg, &aCount);
	}

/** Cancel the outstanding request to PostUserBuffer
*/
inline void RDisplayChannel::CancelPostUserBuffer(void)
	{
	DoCancel(1<<ECtrlCancelPostUserBuffer);
	}

/** Deregister a previously registered buffer.

    @param aBufferId Identifier for a previously registered buffer, generated by a call to RegisterUserBuffer().
    @return KErrNone if successful, KErrArgument if the token is not recognised, KErrInUse if the buffer is still in use,
*/
inline TInt RDisplayChannel::DeregisterUserBuffer(TBufferId aBufferId)
	{
	return (DoControl(ECtrlDeregisterUserBuffer, (TAny*)&aBufferId));
	}

/** This function allows the caller to be notified when a specified post either gets displayed or dropped.
    Completes when a specified post request either gets displayed or dropped.  
    Used to determine timing information for when a particular frame was displayed.

    @param aPostCount An identifier representing a particular post request obtained from PostUserBuffer().
    @param aStatus On completion, contains the status of the request. This is KErrNone if the post request was 
    valid and was either displayed or dropped.  If aPostCount is less than the current count then this will complete 
    immediately with KErrNone. Otherwise this is another of the system-wide error codes.
*/
inline void RDisplayChannel::WaitForPost(TPostCount aPostCount, TRequestStatus& aStatus)
	{
    DoRequest(EReqWaitForPost, aStatus, &aPostCount);
	}

/** Cancel the outstanding request to WaitForPost().
*/
inline void RDisplayChannel::CancelWaitForPost(void)
	{
	DoCancel(1<<ECtrlCancelWaitForPost);
	}

/** Ensures that the next post request will be displayed using the requested rotation.

    @param aRotation A specific rotation value.
    @param aDisplayConfigChanged Returns ETrue if the composition buffer is now using an alternative 
    (either Normal or Flipped) orientation following this call.
    @return KErrNone if rotation can be achieved.  KErrNotSupported if the requested rotation is not supported. 
    Otherwise this is another of the system-wide error codes.
*/
inline TInt RDisplayChannel::SetRotation(TDisplayRotation aRotation, TBool& aDisplayConfigChanged)
	{
	aDisplayConfigChanged = EFalse;
	return (DoControl(ECtrlSetRotation, &aRotation, &aDisplayConfigChanged));
	}

/** Return the current rotation setting of the display.
*/
inline RDisplayChannel::TDisplayRotation RDisplayChannel::CurrentRotation(void)
	{
    TDisplayRotation rotation = ERotationNormal;
    DoControl(ECtrlCurrentRotation, &rotation);
    return rotation;
	}

/** Asynchronous request for notification of when a display change occurs.
    The request is completed when the connectedness of the display changes, or when
    the set of available resolutions or pixel formats changes.
    
    If a failure occurs, it is passed back in the aStatus.

    @panic DISPCHAN KNotificationAlreadySet if a notification request is pending.
    @param aStatus    Completed on display change, or cancellation.
*/
inline void RDisplayChannel::NotifyOnDisplayChange(TRequestStatus& aStatus)
	{
	DoRequest(EReqWaitForDisplayConnect, aStatus);
	}

/** Cancels a pending request to notify on display change.

    If there is a pending notification request, the status value will be set to
    KErrCancelled and it will be completed. If there is no pending request for
    notification, the call will simply return.
*/
inline void RDisplayChannel::NotifyOnDisplayChangeCancel()
	{
	DoCancel(1<<ECtrlCancelWaitForDisplayConnect);
	}

/** Returns the number of display resolutions currently available.

    This is the maximum number of resolutions that can currently be retrieved using
    GetResolutions() and set using a combination of SetResolution() and
    SetRotation().
    When no display is connected, there shall be a single entry.
    
    @return The number of TResolution elements that can be retrieved using
    GetResolutions().
*/
inline TInt RDisplayChannel::NumberOfResolutions()  
	{
	return (DoControl(ECtrlNumberOfResolutions));
	}

/** Retrieves the resolutions that are currently available.

    If the given buffer is large enough to hold them all, the set of available
    resolutions shall be written to it as a contiguous sequence of TResolution
    elements. aCount shall be set to the number of TResolution elements written and
    the length of the buffer shall be set to the total number of bytes written.

    If a display can be disabled or become disconnected, the list shall include the
    size (0,0). If the display is connected but disabled, the list shall also
    include the other supported resolutions. If a display cannot be disconnected or
    disabled, the list shall not include (0,0).

    @return KErrNone on success, KErrOverflow if the buffer is not large enough, or
    KErrNotSupported if not supported by driver..
    @param aResolutions    Buffer to receive resolutions, as a contiguous sequence
    of TResolution elements.
    @param aCount    The number of TResolution elements written to the buffer.
*/
inline TInt RDisplayChannel::GetResolutions(TDes8& aResolutions, TInt& aCount)  
	{
	return (DoControl(ECtrlGetResolutions,&aResolutions,&aCount));
	}

/** Sets the display's new pixel resolution.

    The resolution shall be in terms of no rotation (ERotationNormal), but the
    actual display output will depend on the current Rotation() value.

    A successful call to this function may change any of the other attributes of
    the display channel. The change to the attributes happens immediately, but the
    change to the display output will only take place on the next buffer posting.

    It is recommended that the implementation tries to minimize changes to other
    attributes, if possible. For example, if the current rotation is set to
    ERotation90CW and the new resolution supports that rotation, it should remain
    as the current rotation. On the other hand, if the new resolution does not
    support that rotation, it must be changed to a supported one.

    If (0,0) is in the resolution list, passing a zero width and/or zero height
    resolution shall select it, and shall have the effect of disabling output.

    If either dimension of the given resolution is negative, KErrArgument shall be
    returned. If the parameter is valid, but not currently available,
    KErrNotSupported shall be returned.

    @see GetResolutions
    @capability WriteDeviceData Used to prevent arbitrary changes to the display
    resolution.
    @param aRes    The new display resolution.
    @return KErrNone on success. KErrNotSupported, KErrOutOfMemory or KErrArgument
    on failure.
*/
inline TInt RDisplayChannel::SetResolution(const TSize& aRes)
	{
	return (DoControl(ECtrlSetResolution,const_cast<TSize*>(&aRes)));
	}

/** Returns the current resolution.

    This is always in terms of the ERotationNormal rotation regardless of current
    and supported rotations. When the display is disconnected or disabled, this
    returns (0,0).

    If the current rotation is ERotation90CW or ERotation270CW, the width and
    height values must be swapped by the caller to get the apparent width and
    height.

    @param aSize    Receives the current resolution.
    @return KErrNone on success, KErrNotSupported if not supported by driver.
*/
inline TInt RDisplayChannel::GetResolution(TSize& aSize)  
	{
	return (DoControl(ECtrlGetResolution,&aSize));
	}

inline TInt RDisplayChannel::GetTwips(TSize& aTwips)
	{
	return (DoControl(ECtrlGetTwips,&aTwips));
	}

/** Returns the number of different buffer pixel formats that can be retrieved
    using GetPixelFormats().

    @return The number of pixel formats supported.
*/
inline TInt RDisplayChannel::NumberOfPixelFormats()  
	{
	return (DoControl(ECtrlNumberOfPixelFormats));
	}

/** Retrieves the buffer pixel formats that are supported.

    If aFormatsBuf is large enough to hold them all, the set of available pixel
    formats shall be written to it as a contiguous sequence of TPixelFormat
    elements. aCount shall be set to the number of TPixelFormat elements written
    and the length of the buffer shall be set to the total number of bytes written.

    Not all pixel formats may be valid in all circumstances.

    @see SetBufferFormat

    @param aFormatsBuf    Buffer to receive pixel formats, as a contiguous sequence
    of TUid elements.
    @param aCount    Receives the number of TUid elements written to the buffer.
    @return KErrNone on success, KErrOverflow if the buffer is too small to hold
    all the elements, or KErrNotSupported if not supported by driver.
*/
inline TInt RDisplayChannel::GetPixelFormats(TDes8& aFormatsBuf, TInt& aCount)  
	{
	return (DoControl(ECtrlGetPixelFormats,&aFormatsBuf,&aCount));
	}

/** Sets the buffer format to be used when the next buffer is posted.

    The width and height used in the buffer format correspond to the current
    rotation in effect. The size in the buffer format must be at least as big as
    the buffer mapping size, or the function shall fail with KErrNotSupported.

    @see SetBufferMapping
    @see PostCompositionBuffer
    @see PostLegacyBuffer
    @see PostUserBuffer

    @capability WriteDeviceData Used to prevent arbitrary changes to the buffer
    format.
    @param aBufferFormat    The buffer format to be used.
    @return KErrNone on success, KErrArgument if the buffer format is not valid,
    KErrNotSupported if the format is valid but not supported, or KErrOutOfMemory
    on memory allocation failure.
*/
inline TInt RDisplayChannel::SetBufferFormat(const TBufferFormat& aBufferFormat)
	{
	return (DoControl(ECtrlSetBufferFormat,const_cast<TBufferFormat*>(&aBufferFormat)));

	}

/** Retrieves the buffer format that will be used on the next buffer posting.

    Initially, this will match the information returned by GetDisplayInfo() for the
    current rotation.
    When a new resolution, rotation or mapping is chosen, the buffer format may
    change.

    @param aBufferFormat    Receives the buffer format.
    @return KErrNone on success, KErrNotSupported if not supported by driver.
*/
inline TInt RDisplayChannel::GetBufferFormat(TBufferFormat& aBufferFormat)  
	{
	return (DoControl(ECtrlGetBufferFormat,&aBufferFormat));
	}

/** Returns the offset in bytes from the start of a plane to the next one, for the
    given buffer format.

    This allows for additional bytes at the end of a plane before the start of the
    next one, to allow for alignment, for example.

    For packed pixel formats and interleaved planes in semi-planar formats, the
    return value is zero.
    
    The current display channel resolution and the rotation is used in computing the
    next plane offset.

    @param aBufferFormat    A buffer width, in pixels.
    @param aPlane    The plane number, starting at zero, for planar formats.
    @return The next plane offset, in bytes, or zero if the parameters are invalid,
    not recognised or not supported.
*/
inline TInt RDisplayChannel::NextPlaneOffset(const TBufferFormat& aBufferFormat, TInt aPlane) 
	{
	return (DoControl(ECtrlNextPlaneOffset,const_cast<TBufferFormat*>(&aBufferFormat),&aPlane));
	}

/** Returns the offset in bytes between pixels in adjacent lines, for the given
    plane if relevant.

    The value returned may allow for additional bytes at the end of each line, for
    the purposes of alignment, for example. This is also known as the "stride" of
    the line.

    For packed pixel formats, aPlane is ignored. The offset returned shall be at
    least the width in pixels multiplied by the bytes per pixel.

    The current display channel resolution and the rotation is used in computing the
    next line offset.
    
    For planar and semi-planar formats, aPlane dictates which offset is returned.
    It must be at least the width in pixels multiplied by the (possibly fractional)
    number of bytes per pixel for the plane.

    @param aBufferFormat    The buffer format.
    @param aPlane    The plane number, starting at zero.
    @return The stride for a given combination of width in pixels and pixel format,
    or zero if the parameters are invalid, not recognised or not supported.
*/
inline TInt RDisplayChannel::NextLineOffset(const TBufferFormat& aBufferFormat, TInt aPlane) 
	{
	return (DoControl(ECtrlNextLineOffset,const_cast<TBufferFormat*>(&aBufferFormat),&aPlane));
	}

/** Returns the offset in bytes from the start of a plane to the next one, for the
    given parameters.

    This allows for additional bytes at the end of a plane before the start of the
    next one, to allow for alignment, for example.

    For packed pixel formats and interleaved planes in semi-planar formats, the
    return value is zero.

    For planar and semi-planar formats, aPlane dictates which offset is returned.
    It must be at least the width in pixels multiplied by the (possibly fractional)
    number of bytes per pixel for the plane.

    @param aBufferFormat    The buffer format.
    @param aResolution	The resolution to be taken in consideration
    @param aRotation    The rotation to be taken in consideration
    @param aPlane    The plane number, starting at zero.
    @return The stride for a given combination of width in pixels and pixel format,
    or zero if the parameters are invalid, not recognised or not supported.
*/
inline TInt RDisplayChannel::NextPlaneOffset(const TBufferFormat& aBufferFormat, const TResolution& aResolution, TDisplayRotation aRotation, TInt aPlane)
	{
	TBufferFormatContext context(aResolution, aRotation, aPlane);
	return (DoControl(ECtrlNextPlaneOffsetExtended, const_cast<TBufferFormat*>(&aBufferFormat), &context));
	}

/** Returns the offset in bytes between pixels in adjacent lines, for the given
    plane if relevant.

    The value returned may allow for additional bytes at the end of each line, for
    the purposes of alignment, for example. This is also known as the "stride" of
    the line.

    For packed pixel formats, aPlane is ignored. The offset returned shall be at
    least the width in pixels multiplied by the bytes per pixel.

    For planar and semi-planar formats, aPlane dictates which offset is returned.
    It must be at least the width in pixels multiplied by the (possibly fractional)
    number of bytes per pixel for the plane.

    @param aBufferFormat    The buffer format.
    @param aResolution	The resolution to be taken in consideration
    @param aRotation    The rotation to be taken in consideration
    @param aPlane    The plane number, starting at zero.
    @return The stride for a given combination of width in pixels and pixel format,
    or zero if the parameters are invalid, not recognised or not supported.
*/
inline TInt RDisplayChannel::NextLineOffset(const TBufferFormat& aBufferFormat, const TResolution& aResolution, TDisplayRotation aRotation, TInt aPlane)
	{
	TBufferFormatContext context(aResolution, aRotation, aPlane);
	return (DoControl(ECtrlNextLineOffsetExtended, const_cast<TBufferFormat*>(&aBufferFormat), &context));
	}

/** Returns the current version of the driver.
*/
inline TInt RDisplayChannel::Version(TVersion& aVersion) 
	{
	return (DoControl(ECtrlVersion, &aVersion));
	}

#ifdef _DEBUG
/** Debug only function to allocate a shared chunk user buffer for testing.
*/
inline TInt RDisplayChannel::CreateUserBuffer(TBufferFormat& aBufferFormat, RChunk& aChunk)
	{
	return (aChunk.SetReturnedHandle(DoControl(ECtrlCreateUserBuffer, &aBufferFormat)));
	}
#endif // _DEBUG

/** Constructs a resolution setting.

    @param aSize    The resolution size in pixels, in ERotationNormal rotation.
    @param aRotations    A bitwise combination of one or more TDisplayRotation
    values.
*/
inline RDisplayChannel::TResolution::TResolution(TSize aPixelSize, TSize aTwipsSize, TUint32 aFlags):
	iPixelSize(aPixelSize),iTwipsSize(aTwipsSize),iFlags(aFlags),reserved_0(0)
	{	}

/** Constructs a buffer format.

    @param aSize    The size in pixels.
    @param aPixelFormat    The pixel format.
*/
inline RDisplayChannel::TBufferFormat::TBufferFormat(TSize aSize, TPixelFormat aPixelFormat):
	iSize(aSize),iPixelFormat(aPixelFormat),reserved_0(0)
	{
	}


/** Constructs a buffer context.

    @param aResolution    The display resolution.
    @param aRotation    The display rotation.
    @param aPlane
*/
inline RDisplayChannel::TBufferFormatContext::TBufferFormatContext(TResolution aResolution, TDisplayRotation aRotation, TInt aPlane):
	iResolution(aResolution), iRotation(aRotation), iPlane(aPlane)
	{
	}


#endif

/**
*/
inline const TDesC& RDisplayChannel::Name()
{
 return (KDisplayDriverName);
}

/**
*/
inline TVersion RDisplayChannel::VersionRequired(void)
{
	return TVersion(KDisplayChMajorVersionNumber,
				    KDisplayChMinorVersionNumber,
			        KDisplayChBuildVersionNumber);
}

#endif // __DISPCHANNEL_INL__
