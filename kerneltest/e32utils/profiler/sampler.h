// Copyright (c) 1999-2009 Nokia Corporation and/or its subsidiary(-ies).
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
// e32utils\profiler\sampler.h
// 
//

#ifndef __SAMPLER_H__
#define __SAMPLER_H__

#include <e32cmn.h>
#ifndef __KERNEL_MODE__
#include <e32std.h>
#endif

//Uncomment this line to generate debug logging of the profiler and sampler.
//#define DEBUG_PROFILER(f) f
#define DEBUG_PROFILER(f)

/**
 * The user device driver class for controlling the sampler.
 */
class RSampler : public RBusLogicalChannel
	{
	friend class DProfile;
private:
	enum TControl
		{
		EControlGetSegments,
		EControlStartProfile,
		EControlStopProfile,
		EControlResetProfile,
		EControlResetSegments,
		EControlDrain,
		EControlGetErrors
		};
	enum TRequest
		{
		ERequestRead
		};
	static inline TInt CancelRequest(TRequest aRequest);
public:
#ifndef __KERNEL_MODE__
	/** Open a channel to the sampling device */
	inline TInt Open();
	/** Get the current non-XIP Code Segments*/
	inline void GetSegments(TDes8& aData);
	/** Start sampling */
	inline void Start(TInt aRate);
	/** Stop sampling */
	inline void Stop();
	/** Extract the sample data */
	inline void Read(TDes8& aData,TRequestStatus& aStatus);
	/** Cancel a read request */
	inline void ReadCancel();
	/** Extract any remaining sample data */
	inline void Drain(TDes8& aData);
	/** Get error report */
	inline void GetErrors(TDes8& aData);
	/** Reset the sampler for a new run */
	inline void Reset(TBool aXIPOnly);
	/** Initialise non-XIP segments for a new run */
	inline void ResetSegments();
#endif
	};

_LIT(KSamplerName,"Sampler");

#ifndef __KERNEL_MODE__
inline TInt RSampler::CancelRequest(TRequest aRequest)
	{return 1<<aRequest;}

inline TInt RSampler::Open()
	{return DoCreate(KSamplerName,TVersion(1,0,0),KNullUnit,NULL,NULL);}

/**
 * Get the existing non-XIP Code Segments.
 *
 * @param aData	A descriptor to receive data. Returns as zero-length if all data 
 * records are already transfered.
 */
inline void RSampler::GetSegments(TDes8& aData)
	{DoControl(EControlGetSegments, &aData);}

/**
 * Start sampling at the requested rate. If currently sampling, this will just
 * change the sample rate.
 *
 * @param aRate The sample rate in samples/second
 */
inline void RSampler::Start(TInt aRate)
	{DoControl(EControlStartProfile, reinterpret_cast<TAny*>(aRate));}

/**
 * Stop sampling. If currently sampling, any outstanding read request will be completed
 * with data remaining in the buffer.
 */
inline void RSampler::Stop()
	{DoControl(EControlStopProfile);}

/**
 * Reset the sampler. All sample data is discarded and history is removed, preparing the
 * sampler for a new run. The sampler must be stopped before it can be reset.
 *
 * @param aXIPOnly True if the profiler runs in XIPOnly mode.
 */
inline void RSampler::Reset(TBool aXIPOnly)
	{DoControl(EControlResetProfile,(TAny*)aXIPOnly);}

/**
* Reset non-XIP code segments, preparing the sampler for a new run
*/
inline void RSampler::ResetSegments()
	{DoControl(EControlResetSegments);}

/**
 * Extract the sample data from the device. This will complete when the device's buffer is
 * full, when the provided descriptor is full, or when the sampler is stopped.
 *
 * @param aData		A descriptor to receive the sample data
 * @param aStatus	A request status used to signal when this request is completed
 */
inline void RSampler::Read(TDes8& aData,TRequestStatus& aStatus)
	{DoRequest(ERequestRead,aStatus,&aData);}

/**
 * Cancel the outstanding read request
 */
inline void RSampler::ReadCancel()
	{DoCancel(CancelRequest(ERequestRead));}

/**
 * Extract any remaining sample data from the device buffer. When the buffer is
 * empty, the descriptor will be empty on return.
 *
 * @param aData	A descriptor to receive the sample data
 */
inline void RSampler::Drain(TDes8& aData)
	{DoControl(EControlDrain,&aData);}

/**
 * Get error report from the sampler.
 *
 * @param aData		A descriptor to receive error report
 */
inline void RSampler::GetErrors(TDes8& aData)
	{DoControl(EControlGetErrors,&aData);}

#endif

#endif
