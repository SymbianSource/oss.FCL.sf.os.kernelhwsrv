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
// e32\include\d32camerasc.inl
// 
//

inline TVersion RDevCameraSc::VersionRequired()
	{
	const TInt KCameraMajorVersionNumber=1;
	const TInt KCameraMinorVersionNumber=0;
	const TInt KCameraBuildVersionNumber=KE32BuildVersionNumber;
	return TVersion(KCameraMajorVersionNumber,KCameraMinorVersionNumber,KCameraBuildVersionNumber);
	}

#ifndef __KERNEL_MODE__


inline RDevCameraSc::RDevCameraSc()
: RBusLogicalChannel()
	{
	iCameraCaps = NULL;
	iCapsSize = 0;
	}

inline TInt RDevCameraSc::Open(TInt aUnit)
	{
	TInt r=DoCreate(KDevCameraScName,VersionRequired(),aUnit,NULL,NULL,EOwnerThread);
	if (KErrNone == r)
		{
		// Obtain the Capability structure size then allocate memory for it on client side
		r=iCapsSize=DoControl(EControlCapsSize);		
		if (KErrNone > r)
			{
			iCapsSize = 0;
			return r;
			}

		TAny* capsBufPtr = User::Alloc(iCapsSize);
		if(NULL == capsBufPtr)
			{
			iCapsSize = 0;
			return KErrNoMemory;
			}

		TPtr8 capsPtr((TUint8*)capsBufPtr, iCapsSize, iCapsSize);
		// Fill the Capability structure
		r = DoControl(EControlCaps,(TAny*)&capsPtr);
		if (KErrNone > r)
			{
			iCapsSize = 0;
			return r;
			}
		iCameraCaps = (TCameraCapsV02*) capsPtr.Ptr();
		}

	return r;
	}

inline void RDevCameraSc::Close()
	{
	if (iCameraCaps != NULL)
		{
		User::Free(iCameraCaps);
		iCameraCaps = NULL;
		}
	iCapsSize = 0;
	RBusLogicalChannel::Close();
	}

inline TInt RDevCameraSc::Caps(TDes8& aCapsBuf)
	{
	if (aCapsBuf.MaxLength() < iCapsSize)
		{
		return KErrArgument;
		}

	TPtrC8 ptr ((TUint8*)iCameraCaps, iCapsSize);
	aCapsBuf = ptr;
	return KErrNone;
	}

inline TPtrC8 RDevCameraSc::Caps()
	{
	TPtrC8 ptr((TUint8*)iCameraCaps, iCapsSize);
	return ptr;
	}

inline TInt RDevCameraSc::SetBufConfigChunkCreate(TDevCamCaptureMode aCaptureMode, TInt aNumBuffers, RChunk& aChunk)
	{return(aChunk.SetReturnedHandle(DoControl(EControlSetBufConfigChunkCreate,(TAny*)aCaptureMode,(TAny*)aNumBuffers)));}

inline TInt RDevCameraSc::SetBufConfigChunkOpen(TDevCamCaptureMode aCaptureMode, const TDesC8& aBufferConfigBuf, RChunk& aChunk)
	{
	SSetBufConfigChunkOpenInfo info = {&aBufferConfigBuf, aChunk.Handle()};
	return(DoControl(EControlSetBufConfigChunkOpen,(TAny*)aCaptureMode,&info));
	}

inline TInt RDevCameraSc::ChunkClose(TDevCamCaptureMode aCaptureMode)
	{return(DoControl(EControlChunkClose,(TAny*)aCaptureMode));}

inline TInt RDevCameraSc::SetCamConfig(TDevCamCaptureMode aCaptureMode,const TDesC8& aConfigBuf)
	{return(DoControl(EControlSetCamConfig,(TAny*)aCaptureMode,(TAny*)&aConfigBuf));}

inline void RDevCameraSc::GetCamConfig(TDevCamCaptureMode aCaptureMode, TDes8& aConfigBuf)
	{DoControl(EControlGetCamConfig,(TAny*)aCaptureMode,(TAny*)&aConfigBuf);}

inline void RDevCameraSc::GetBufferConfig(TDevCamCaptureMode aCaptureMode, TDes8& aConfigBuf)
	{DoControl(EControlGetBufferConfig,(TAny*)aCaptureMode,(TAny*)&aConfigBuf);}

inline TInt RDevCameraSc::SetCaptureMode(TDevCamCaptureMode aCaptureMode)
	{return(DoControl(EControlSetCaptureMode,(TAny*)aCaptureMode));}

inline TInt RDevCameraSc::Start()
	{return(DoControl(EControlStart));}

inline TInt RDevCameraSc::Stop()
	{return(DoControl(EControlStop));}

inline void RDevCameraSc::NotifyNewImage(TRequestStatus& aStatus)
	{DoRequest(ERequestNotifyNewImage,aStatus);}

inline void RDevCameraSc::NotifyNewImageCancel()
	{DoCancel(1<<ERequestNotifyNewImage);}

inline void RDevCameraSc::NotifyNewImageCancel(const TRequestStatus& aStatus)
	{DoControl(EControlNotifyNewImageSpecificCancel,(TAny*)&aStatus);}

inline TInt RDevCameraSc::ReleaseBuffer(TInt aBufferId)
	{return(DoControl(EControlReleaseBuffer,(TAny*)aBufferId));}

inline TInt RDevCameraSc::BufferIdToOffset(TDevCamCaptureMode aCaptureMode, TInt aId, TInt& aOffset)
	{
	// search criteria
	TDevCamBufferModeAndIdBuf databuf;
	TDevCamBufferModeAndId &data = databuf();
	data.iCaptureMode = aCaptureMode;
	data.iId = aId;

	return (DoControl(EControlBufferIdToOffset,(TAny*)&databuf,(TAny*)&aOffset));
	}

inline TInt RDevCameraSc::CapsSize()
	{return(iCapsSize);}

inline TInt RDevCameraSc::FrameSizeCaps(TDevCamCaptureMode aCaptureMode, TUidPixelFormat aUidPixelFormat, TDes8& aFrameSizeCapsBuf)
	{
	SFrameSizeCapsInfo info = {aUidPixelFormat, aCaptureMode};
	return(DoControl(EControlFrameSizeCaps, (TAny*)&aFrameSizeCapsBuf, &info));
	}
	
inline TInt RDevCameraSc::SetDynamicAttribute(TDevCamDynamicAttributes aAttribute, TUint aValue)
	{
	TInt r = KErrNone;
	
	// Are we not supported?
	TInt misc = 0;
	switch (aAttribute)
		{
		case ECamAttributeBrightness:
			{
			misc = KCamMiscBrightness;
			break;
			}
		case ECamAttributeContrast:
			{
			misc = KCamMiscContrast;
			break;
			}
		case ECamAttributeColorEffect:
			{
			misc = KCamMiscColorEffect;
			break;
			}
		default:
			{
			r = KErrBadName;
			}
		}

	if (KErrNone != r)
		{
		return r;
		}

	if (!(iCameraCaps->iCapsMisc & misc))
		{
		r = KErrNotSupported;
		}

	else
		{
		TDynamicRange &range = iCameraCaps->iDynamicRange[aAttribute];
		// Not within range?
		if (aValue < range.iMin || aValue > range.iMax)
			{
			r = KErrArgument;
			}
		else
			{
			r = DoControl(EControlSetDynamicAttribute,(TAny*)aAttribute, (TAny*)aValue);
			}
		}

	return r;
	}

#endif	// __KERNEL_MODE__
