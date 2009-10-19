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
//
// WARNING: This file contains some APIs which are internal and are subject
//          to change without notice. Such APIs should therefore not be used
//          outside the Kernel and Hardware Services package.
//

#ifndef D32BTRACE_H
#define D32BTRACE_H

#include <e32cmn.h>
#include <e32ver.h>
#include <e32btrace.h>

#ifndef __KERNEL_MODE__
#include <e32std.h>
#endif


class TBTraceBuffer;

/**
Interface to the fast-trace memory buffer.

@publishedPartner
@released
*/
class RBTrace : public RBusLogicalChannel
	{
public:
	/**
	Bit flags indicating the operational mode of the fast-trace buffer.
	*/
	enum TMode
		{
		/**
		Flag set to enable trace to be stored in the buffer.
		If this is not set, all trace records are discarded.
		*/
		EEnable			= 1<<0,

		/**
		This flag dictates the behaviour which occurs when the trace buffer is too full to
		accomodate a new record.
		When this flag is set, new trace records will overwrite the oldest ones.
		When this flag is not set, new trace records are discard.
		(This latter mode provides the best performance.)
		*/
		EFreeRunning	= 1<<1,
		};

#ifndef __KERNEL_MODE__
	/**
	Open channel to fast-trace driver.
	Must be called before any other methods are used.
	@return KErrNone or standard error code.
	*/
	IMPORT_C TInt Open();

	/**
	Close channel to fast-trace driver.
	*/
	IMPORT_C void Close();

	/**
	Get the current size of trace buffer.
	@return Buffer size.
	*/
	IMPORT_C TInt BufferSize();

	/**
	Change size of trace buffer.
	This causes all current data in the trace buffer to be lost.
	If this method fails then the trace buffer may no longer exist.
	@param aSize The size in bytes for the trace buffer.
	@return KErrNone or standard error code.
	*/
	IMPORT_C TInt ResizeBuffer(TInt aSize);

	/**
	Discard all trace data.
	*/
	IMPORT_C void Empty();

	/**
	The chunk in which trace data returned by GetData() resides.
	@return The chunk.
	*/
	inline RChunk DataChunk();

	/**
	The operational mode for fast-trace.
	@return The current operational mode. This is bitmask of values from TMode.
	*/
	IMPORT_C TUint Mode();

	/**
	Set the operational mode for fast-trace.
	Calling this method, invalidates the trace data returned by the last call to GetData().
	@param aMode A bitmask of values from TMode.
	*/
	IMPORT_C void SetMode(TUint aMode);

	/**
	Set the trace filter bit for the specified category.
	@param aCategory A category value from BTrace::TCategories.
	@param aValue The new filter value for the category.
				  1 means traces of this category are output, 0 means they are suppressed.
				  Any other value will be ignored, and this method will just return the current
				  value of the filter.
	@return The previous value of the filter for the category, 0 or 1.
			Or KErrNotSupported if this category is not supported by this build of the kernel.
	*/
	IMPORT_C TInt SetFilter(TUint aCategory, TInt aValue);

	/**
	Get the trace filter bit for the specified category.
	@param aCategory A category value from enum BTrace::TCategory,	
	@return The value of the filter for the category, 0 or 1,
			or KErrNotSupported if this category is not supported by this build of the kernel.
			(1 means traces of this category are output, 0 means they are suppressed.)
	*/
	inline TInt Filter(TUint aCategory);

	/**
	Set the trace secondary trace filter for the specified UID.

	This method can not be used to disable a UID key if SetFilter2(TInt aGlobalFilter)
	has been used to set the filter to pass all traces. Such attempts result in a return
	code of KErrNotSupported.

	@param aUid   The UID to filter.
	@param aValue The new filter value for the UID.
				  1 means traces with this UID are output, 0 means they are suppressed.
				  Other values must not be used.

	@return The previous value of the filter for the UID, 0 or 1, if operation is successful.
			Otherwise, a negative number representing a system wide error code.
			(E.g. KErrNoMemory.)
	*/
	IMPORT_C TInt SetFilter2(TUint32 aUid, TBool aValue);

	/**
	Set the secondary trace filter to include only the specified UIDs.
	
	@param aUids    Pointer to array of UIDs.
	@param aNumUids Number of UID values pointer to by \a aUid.

	@return KErrNone on success.
			Otherwise, a negative number representing a system wide error code.
			(E.g. KErrNoMemory.)
	*/
	IMPORT_C TInt SetFilter2(const TUint32* aUids, TInt aNumUids);

	/**
	Set the secondary trace filter to pass or reject every trace.
	
	@param aGlobalFilter If 0, the secondary filter will reject
						 all traces; if 1, all traces are passed
						 by the filter.
						 Other values have no effect.

	@return The previous value of the global filter.
	*/
	IMPORT_C TInt SetFilter2(TInt aGlobalFilter);

	/**
	Get the contents of the secondary trace filter.
	
	@param [out] aUids   Pointer to array of UIDs contained in the secondary filter.
						Ownership of this array is passed to the caller of this
						function, which is then responsible for deleting it.
						If filter is empty, \a aUid equals zero.
	@param [out] aGlobalFilter	Set to 1 if the secondary filter passes all traces.
								Set to 0 if the secondary filter rejects all traces.
								Set to -1 if the secondary filter operates by UIDs contained in traces.

	@return Number of UIDs in returned array, if operation is successful.
			Otherwise, a negative number representing a system wide error code.
	*/
	IMPORT_C TInt Filter2(TUint32*& aUids, TInt& aGlobalFilter);

	/**
	Get pointer to as much contiguous trace data as is available.
	This data resides in the shared chunk DataChunk().
	The data returned will always be a whole number of trace records, i.e. trace records
	will not be split between successive calls to this method.
	Once the data is no loger required, DataUsed() must be called.
	This method can be called repeatedly to get more trace data. E.g.
	@code
		RBTrace trace;
		TInt error = trace.Open();
		if(error!=KErrNone)
			return error;
		const TInt KTraceDataBlockSize = 65536; // size of data we ideally want to process in one go
		TUint8* data;
		TInt size;
		do
			{
			while((size=trace.GetData(data))!=0)
				{
				ProcessTheTraceData(data,size);
				trace.DataUsed();
				}
			TRequestStatus waitStatus;
			trace.RequestData(waitStatus,KTraceDataBlockSize);
			User::WaitForRequest(waitStatus);
			error = waitStatus.Int();
			}
		while(error==KErrNone);
		trace.Close();
		return error;
	@endcode

	@param aData Pointer to the first byte of trace data.
	@return The number of bytes of trace data available at aData.
	@see DataChunk();
	*/
	IMPORT_C TInt GetData(TUint8*& aData);

	/**
	Remove from the trace buffer all of the data returned by the last call to GetData().
	*/
	IMPORT_C void DataUsed();

	/**
	Request notification when trace data becomes available.
	Only one outstanding request may be present at any one time.
	@param aStatus Request status to be complete with KErrNone once data becomes available.
	@param aSize The minimum number of bytes of trace data required.
				 This is intended to improve performance by only signalling the client once
				 a suitably large amount of trace data is available. However, this request
				 may complete before this amount of trace data is available, therefore a
				 client must be able to handle this situation.
				 If aSize is zero or negative, then the request completes as soon as any trace
				 data is available.
	@panic BTRACE 0 if a previous request is still pending
	*/
	IMPORT_C void RequestData(TRequestStatus& aStatus, TInt aSize);

	/**
	Cancel any previous RequestData(), completing it with KErrCancel.
	*/
	IMPORT_C void CancelRequestData();

	/**
	@internalTechnology
	*/
	TBool SetSerialPortOutput(TBool aEnable);

	/**
	Controls whether Timestamp2 field is added to trace record headers.
	*/
	IMPORT_C TBool SetTimestamp2Enabled(TBool aEnable);

#endif

	/**
	Enumeration of panic reasons for category 'BTRACE'.
	*/
	enum TPanic
		{
		ERequestAlreadyPending, /**< A call to RequestData() was made whist a request was already pending. */
		};
private:
	TInt OpenChunk();
	void CloseChunk();
	inline static const TDesC& Name();

	enum TControl
		{
		EOpenBuffer,
		EResizeBuffer,
		ESetFilter,
		ESetFilter2,
		ESetFilter2Array,
		ESetFilter2Global,
		EGetFilter2Part1,
		EGetFilter2Part2,
		ERequestData,
		ECancelRequestData,
		ESetSerialPortOutput,
		ESetTimestamp2Enabled,
		};
#ifndef __KERNEL_MODE__
	RChunk iDataChunk;
	TBTraceBuffer* iBuffer;
	TInt iLastGetDataSize;
	TUint32 iSpare[4];
#endif
	friend class DBTraceChannel;
	friend class DBTraceFactory;
	friend class RBTraceAdapter;
	};

#ifndef __KERNEL_MODE__

inline RChunk RBTrace::DataChunk()
	{
	return iDataChunk;
	}

inline TInt RBTrace::Filter(TUint aCategory)
	{
	return SetFilter(aCategory,-1);
	}

#endif // !__KERNEL_MODE__

inline const TDesC& RBTrace::Name()
	{
	_LIT(KBTraceName,"btrace");
	return KBTraceName;
	}


/**
@internalComponent
*/
class TBTraceBuffer
	{
private:
	TUint				iStart;
	TUint				iEnd;
	volatile TUint		iHead;
	volatile TUint		iTail;
	volatile TUint		iWrap;
	volatile TUint		iGeneration;
	volatile TUint		iMode;
	TUint				iRecordOffsets;
	TUint				iCopyBuffer;
	TUint				iCopyBufferSize;
private:
	TInt	Data(TUint& aData, TUint& aTail);
	TInt	Adjust(TUint aTail, TInt aSize);
	TInt	CopyData(TUint aData, TUint aTail, TInt aSize);
	TUint	UpdateTail(TUint32 aOld, TUint32 aNew);
	TInt	GetData(TUint8*& aData);

	friend class RBTrace;
	friend class TBTraceBufferK;
	friend class RBTraceAdapter;
	};



#endif
