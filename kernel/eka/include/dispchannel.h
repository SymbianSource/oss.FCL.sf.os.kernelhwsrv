// Copyright (c) 2008-2009 Nokia Corporation and/or its subsidiary(-ies).
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
// User side class definitions for GCE mode support.
// This file contains declarations for the generic
// screen display channel and APIs.
// 
//

/**
 @file dispchannel.h
 @publishedPartner
 @released
*/

#ifndef __DISPLAY_CHANNEL_H__
#define __DISPLAY_CHANNEL_H__ 

#include <e32cmn.h>
#include <e32ver.h>
#ifndef __KERNEL_MODE__
#include <e32std.h>
#endif

_LIT(KDisplayDriverName,"displaychannel");

// the defined versions to be used for macros, condititional complilation, etc. 

// The major version number. MUST be a number in the range 0 to 127
#define K_DISPLAY_CH_MAJOR_VERSION_NUMBER 1
// The minor version number. MUST be a number in the range 0 to 99
#define K_DISPLAY_CH_MINOR_VERSION_NUMBER 2
// The build number. MUST be a number in the range 0 to 32,767
#define K_DISPLAY_CH_BUILD_VERSION_NUMBER 1

// legacy set of version constants
const TInt KDisplayChMajorVersionNumber = K_DISPLAY_CH_MAJOR_VERSION_NUMBER;
const TInt KDisplayChMinorVersionNumber = K_DISPLAY_CH_MINOR_VERSION_NUMBER;
const TInt KDisplayChBuildVersionNumber = K_DISPLAY_CH_BUILD_VERSION_NUMBER;

// the base version supported across all implementation
const TInt KDisplayChMajorVersionNumberAlwaysSupported = 1;
const TInt KDisplayChMinorVersionNumberAlwaysSupported = 0;
const TInt KDisplayChBuildVersionNumberAlwaysSupported = 1;


/*
    Generic display driver interface
*/
class RDisplayChannel : public RBusLogicalChannel
	{
public:

	typedef TInt  TPixelFormat;
	typedef TUint TBufferId;
	typedef TUint TPostCount;

    enum TDisplayRotation
         /** Used to communicate available rotations as a bit field and used to set a 
             particular rotation as a single value. Rotations are specified relative 
             to the normal physical orientation of the device.
         */
        {
            ERotationNormal    = 1,
            ERotation90CW      = 2,
            ERotation180       = 4,
            ERotation270CW     = 8,
            // v 1.1
            ERotationAll       = 0xF
        };

    // control messages
	enum TControl
		{
		// v 1.0
		ECtrlGetCaps = 0,
		ECtrlIsSingleBuffered,
		ECtrlGetDisplayInfo,
		ECtrlOpen,
		ECtrlClose,
		ECtrlPostCompositionBuffer,
		ECtrlPostLegacyBuffer,
		ECtrlRegisterUserBuffer,
		ECtrlDeregisterUserBuffer,
		ECtrlPostCount,
		ECtrlSetRotation,
		ECtrlCurrentRotation,
		ECtrlGetCompositionBufferInfo,
		// intermediate version
		ECtrlVersion,
		// v 1.1
		ECtrlNumberOfResolutions,
		ECtrlGetResolutions,
		ECtrlSetResolution,
		ECtrlGetResolution,
		ECtrlGetTwips,
		ECtrlNumberOfPixelFormats,
		ECtrlGetPixelFormats,
		ECtrlSetBufferFormat,
		ECtrlGetBufferFormat,
		ECtrlNextPlaneOffset,
		ECtrlNextLineOffset,
		// v 1.2
		ECtrlNextPlaneOffsetExtended,
		ECtrlNextLineOffsetExtended,
		ECtrlCreateUserBuffer
		};

    // request messages
	enum TRequest
		{
		EReqGetCompositionBuffer = 0,
		EReqPostUserBuffer,
		EReqWaitForPost,
		// v 1.1
		EReqWaitForDisplayConnect
		};

    // corresponding cancel
    enum TCancel
    	{
           ECtrlCancelGetCompositionBuffer = 0,
	       ECtrlCancelPostUserBuffer,
	       ECtrlCancelWaitForPost,
			// v 1.1
	       ECtrlCancelWaitForDisplayConnect
	       
    	};

    enum TPanic
    	{
            EWrongRequest,      // wrong request number from the user side
    		EReqAlreadyPending, // user asynchronous reques is already pending
    		EDriverNotReady,    // wrong operation mode
			// v 1.1
    		EInvalidResolution,	// Negative resolution passed
    		ENullArgument,		// NULL argument passed
    	};

public:		//structures used
	
    // display channel configuration information per orientation
    class TOrientationSpecificInfo
    /** Used to communicate the display configuration for a particular orientation.
    */
        {
    public:
        TUint iWidth;               //the width of the display in pixels.
        TUint iHeight;              // the height of the display in pixels.
        TInt iOffsetBetweenLines;
        TUint32 reserved_0;        // reserved fields for future extension
        TUint32 reserved_1;
        TUint32 reserved_2;
        TUint32 reserved_3;
        TUint32 reserved_4;
        };

    // display channel info
    class TDisplayInfo
        {
    public:
		enum { KMaxUserBuffers = 8 };
		enum { KMaxRectangles = 4 };
        TUint iBitsPerPixel;
        TUint iRefreshRateHz;
        TUint iAvailableRotations;
        TPixelFormat iPixelFormat;
        TOrientationSpecificInfo iNormal;       // rotation of 0 or 180 degrees
        TOrientationSpecificInfo iFlipped;      // rotation of 90 or 270 degrees
        TUint iNumCompositionBuffers;
        TUint32 reserved_0;                     // reserved fields for future extension
        TUint32 reserved_1;
        TUint32 reserved_2;
        TUint32 reserved_3;
        TUint32 reserved_4;
        };
    
	/** Defines a resolution setting, a combination of a size in pixels and the
		rotations of that size that can be used. For consistency, the size is always
		given in terms of ERotationNormal, regardless of whether that rotation is
		supported or not.
	 */
	class TResolution
		{	//Kernel mode makes use of agregate constructors
	public:
#ifndef __KERNEL_MODE__
		inline TResolution(TSize aPixelSize,TSize aTwipsSize, TUint32 aFlags = ERotationNormal);
#endif

	public:
		/** The physical display size in pixels with no rotation (i.e. ERotationNormal).
		    See RDisplayChannel::SetResolution().
		*/
		TSize iPixelSize;
		/** The physical display size in twips with no rotation (i.e. ERotationNormal).
		    See RDisplayChannel::SetResolution().
		*/
		TSize iTwipsSize;
		/** A bitwise combination of RDisplayChannel::TDisplayRotation values.
		*/
		TUint32 iFlags;
#ifndef __KERNEL_MODE__
	private:
#endif
		/** Reserved for extension and alignment.
		*/
		TUint32 reserved_0;
		};

	/**
	  Defines the format of a buffer to be posted using PostCompositionBuffer(),
	  PostLegacyBuffer() or PostUserBuffer().
	 */
	class TBufferFormat
		{	//Kernel mode makes use of agregate constructors
	public:
#ifndef __KERNEL_MODE__
		inline TBufferFormat(TSize aSize, TPixelFormat aPixelFormat);
#endif
	public:
		/** The pixel dimensions of the buffer.
		*/
		TSize iSize;
		/** The pixel format of the buffer.
		*/
		TPixelFormat iPixelFormat;
#ifndef __KERNEL_MODE__
	private:
#endif
		/** Reserved for extension and alignment
		*/
		TUint32 reserved_0;
		};
	
	struct TBufferFormatContext
		{
#ifndef __KERNEL_MODE__
		inline TBufferFormatContext(TResolution aResolution, TDisplayRotation aRotation, TInt aPlane);
#endif
		/** The resolution to be used when the buffer format is processed
		*/
		TResolution iResolution;
		
		/** The rotation to be used when the buffer format is processed
		*/
		TDisplayRotation iRotation;
		
		/** The plane number, starting at zero
		*/
		TInt iPlane;
		};

	inline static const TDesC& Name();
	inline static TVersion VersionRequired(void);
#ifndef __KERNEL_MODE__
	// v1.0 methods
	inline TInt Open(TUint aScreen);
	inline void Close(void);
	inline TInt	GetDisplayInfo(TDes8& aInfo);
	inline TDisplayRotation CurrentRotation(void);
	inline TInt GetCompositionBufferInfo(TUint aBufferIndex, RChunk& aChunk, TInt& aOffset);
	inline void GetCompositionBuffer(TUint& aBufferIndex, TRequestStatus& aStatus);
	inline void CancelGetCompositionBuffer(void);
	inline TInt PostCompositionBuffer(const TRegionFix<TDisplayInfo::KMaxRectangles>* aRegion, TPostCount& aCount );
	inline TInt	PostLegacyBuffer(const TRegionFix<TDisplayInfo::KMaxRectangles>* aRegion, TPostCount& aCount);
	inline TInt RegisterUserBuffer(TBufferId& aBufferId, const RChunk& aChunk, TInt aOffset);
    inline void PostUserBuffer(TBufferId aBufferId, TRequestStatus& aStatus, const TRegionFix<TDisplayInfo::KMaxRectangles>* aRegion, TPostCount& aCount );
    inline void CancelPostUserBuffer(void);
    inline TInt DeregisterUserBuffer(TBufferId aBufferId);
    inline void WaitForPost(TPostCount aTPostCount, TRequestStatus& aStatus);
    inline void CancelWaitForPost(void);
    inline TInt SetRotation(TDisplayRotation aRotation, TBool& aDisplayConfigChanged);
    // added to v1.0
    inline TInt Version(TVersion& aVersion); 
    // v1.1 methods
	inline void NotifyOnDisplayChange(TRequestStatus& aStatus);
	inline void NotifyOnDisplayChangeCancel();
	inline TInt NumberOfResolutions();
	inline TInt GetResolutions(TDes8& aResolutions, TInt& aCount);
	inline TInt SetResolution(const TSize& aRes);
	inline TInt GetResolution(TSize& aSize);
	inline TInt GetTwips(TSize& aTwips);
	inline TInt NumberOfPixelFormats();
	inline TInt GetPixelFormats(TDes8& aFormatsBuf, TInt& aCount);
	inline TInt SetBufferFormat(const TBufferFormat& aBufferFormat);
	inline TInt GetBufferFormat(TBufferFormat& aBufferFormat);
	inline TInt NextPlaneOffset(const TBufferFormat& aBufferFormat, TInt aPlane);
	inline TInt NextLineOffset(const TBufferFormat& aBufferFormat, TInt aPlane);
    // v1.2 methods
	inline TInt NextPlaneOffset(const TBufferFormat& aBufferFormat, const TResolution& aResolution, TDisplayRotation aRotation, TInt aPlane);
	inline TInt NextLineOffset(const TBufferFormat& aBufferFormat, const TResolution& aResolution, TDisplayRotation aRotation, TInt aPlane);
#ifdef _DEBUG
	inline TInt CreateUserBuffer(TBufferFormat& aBufferFormat, RChunk& aChunk);
#endif // _DEBUG
#endif
	};

#include <dispchannel.inl>

#endif /* __DISPLAY_CHANNEL__ */

// EOF
