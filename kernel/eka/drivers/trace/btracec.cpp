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
// e32\drivers\trace\btracec.cpp
// 
//

#include <e32std.h>
#include <e32std_private.h>
#include "d32btrace.h"
#include <e32svr.h>
#include <e32atomics.h>


void Panic(TInt aPanicNum)
	{
	_LIT(KRBTracePanic,"RBTrace");
	User::Panic(KRBTracePanic,aPanicNum);
	}

EXPORT_C TInt RBTrace::Open()
	{
	_LIT(KBTraceLddName,"btracex");
	TInt r = User::LoadLogicalDevice(KBTraceLddName);
	if(r!=KErrNone && r!=KErrAlreadyExists)
		return r;
	r = DoCreate( Name(), TVersion(), KNullUnit, NULL, NULL, EOwnerThread);
	if(r==KErrNone)
		{
		r = OpenChunk();
		if(r!=KErrNone)
			Close();
		}
	return r;
	};


TInt RBTrace::OpenChunk()
	{
	TInt r = iDataChunk.SetReturnedHandle(DoControl(EOpenBuffer));
	if(r==KErrNone)
		iBuffer = (TBTraceBuffer*)iDataChunk.Base();
	iLastGetDataSize = 0;
	return r;
	}


void RBTrace::CloseChunk()
	{
	iLastGetDataSize = 0;
	iBuffer = NULL;
	iDataChunk.Close();
	}


EXPORT_C void RBTrace::Close()
	{
	CloseChunk();
	RBusLogicalChannel::Close();
	}


EXPORT_C TInt RBTrace::BufferSize()
	{
	if(!iDataChunk.Handle())
		return 0;
	return iBuffer->iEnd;
	}


EXPORT_C TInt RBTrace::ResizeBuffer(TInt aSize)
	{
	CloseChunk();
	TInt r = DoControl(EResizeBuffer,(TAny*)aSize);
	if(r==KErrNone)
		r = OpenChunk();
	return r;
	}


EXPORT_C void RBTrace::Empty()
	{
	TBTraceBuffer* buffer = iBuffer;
	TUint32 mode = __e32_atomic_swp_acq32(&buffer->iMode, 0);	/* read original mode and disable trace */
	while(__e32_atomic_load_acq32(&buffer->iGeneration) & 1)	/* wait for trace handler to complete */
		{ /* should really __chill() but not available user side */ }
	buffer->iTail = buffer->iHead;
	__e32_atomic_store_ord32(&buffer->iMode, mode);
	}


EXPORT_C TUint RBTrace::Mode()
	{
	return iBuffer->iMode;
	}


EXPORT_C void RBTrace::SetMode(TUint aMode)
	{
	iLastGetDataSize = 0;
	__e32_atomic_store_ord32(&iBuffer->iMode, aMode);
	}


EXPORT_C TInt RBTrace::SetFilter(TUint aCategory, TInt aValue)
	{
	return (TInt)DoControl(ESetFilter,(TAny*)aCategory,(TAny*)aValue);
	}


EXPORT_C TInt RBTrace::SetFilter2(TUint32 aUid, TBool aValue)
	{
	return (TInt)DoControl(ESetFilter2,(TAny*)aUid,(TAny*)aValue);
	}


EXPORT_C TInt RBTrace::SetFilter2(const TUint32* aUids, TInt aNumUids)
	{
	return (TInt)DoControl(ESetFilter2Array,(TAny*)aUids,(TAny*)aNumUids);
	}


EXPORT_C TInt RBTrace::SetFilter2(TInt aGlobalFilter)
	{
	return DoControl(ESetFilter2Global,(TAny*)aGlobalFilter);
	}


EXPORT_C TInt RBTrace::Filter2(TUint32*& aUids, TInt& aGlobalFilter)
	{
	TInt count = (TInt)DoControl(EGetFilter2Part1,&aUids,&aGlobalFilter);
	if(count<=0)
		{
		aUids = 0;
		return count;
		}
	aUids = (TUint32*)User::Alloc(count*sizeof(TUint32));
	if(!aUids)
		return KErrNoMemory;
	DoControl(EGetFilter2Part2,aUids,(TAny*)count);
	return count;
	}


EXPORT_C TInt RBTrace::GetData(TUint8*& aData)
	{
	TInt size = iBuffer->GetData(aData);
	iLastGetDataSize = size;
	return size;
	}

EXPORT_C void RBTrace::DataUsed()
	{
	TBTraceBuffer* buffer = iBuffer;
	if(!(buffer->iMode&RBTrace::EFreeRunning))
		{
		/* Make sure change to iTail is not observed before the trace data reads
			which preceded the call to this function. */
		__e32_memory_barrier();
		buffer->iTail += iLastGetDataSize;
		}
	iLastGetDataSize = 0;
	}


EXPORT_C void RBTrace::RequestData(TRequestStatus& aStatus, TInt aSize)
	{
	if(aSize<0)
		aSize = 0;
	aStatus = KRequestPending;
	if(aSize || iBuffer->iHead==iBuffer->iTail)
		DoControl(ERequestData,&aStatus,(TAny*)aSize);
	else
		{
		TRequestStatus* s = &aStatus;
		User::RequestComplete(s,KErrNone);
		}
	}


EXPORT_C void RBTrace::CancelRequestData()
	{
	DoControl(ECancelRequestData);
	}

EXPORT_C TBool RBTrace::SetTimestamp2Enabled(TBool aEnable)
	{
	return (TBool)DoControl(ESetTimestamp2Enabled, (TAny*)aEnable);
	}

/**
Find out how much data is available.
@param aData Set to the buffer offset where the available trace data is located.
@param aTail Set to the the original value of the iTail pointer
@return Number of bytes of trace data, or an error.
*/
TInt TBTraceBuffer::Data(TUint& aData, TUint& aTail)
	{
	TUint head, tail, wrap;
	TUint32 g0;
	TInt retries=64;
	do	{
		if (--retries<0)
			return KErrInUse;
		// sleep every 8 tries to give the write a chance
		if (retries&7==0)
			User::AfterHighRes(1);
		g0 = iGeneration;
		__e32_memory_barrier();
		head = iHead;
		wrap = iWrap;
		tail = __e32_atomic_and_rlx32(&iTail, ~TUint32(1));
		__e32_memory_barrier();
		} while(iGeneration!=g0 || (g0&1));	// repeat until we get a consistent set
	tail &= ~1;
	aTail = tail;
	TUint end = head;
	if (head<tail)
		{
		end = wrap;
		if (tail>=end)
			{
			tail = iStart;
			end = head;
			}
		}
	aData = tail;
	return end - tail;
	}


/**
Adjust trace data size so it represents a whole number of trace records.
@param aData The buffer offset where the available trace data is located.
@param aSize The size of data at aTail. Must be >= KMaxBTraceRecordSize.
@return An adjusted value for aSize.
*/
TInt TBTraceBuffer::Adjust(TUint aData, TInt aSize)
	{
	TInt adjustedSize = (aSize&~3) - KMaxBTraceRecordSize;
	if (adjustedSize<0)
		Panic(0);
	volatile TUint8* recordOffsets = (volatile TUint8*)this + iRecordOffsets;
	adjustedSize += recordOffsets[(aData+adjustedSize)>>2]<<2;
	if (adjustedSize>aSize)
		Panic(1);
	return adjustedSize;
	}


/**
Update the stored tail offset.
@param aOld The value which iTail is expected to have if no more overwrites have occurred
@param aNew The new value for iTail
@return aNew on success, 0 if the previous tail value had changed before we could update it.
*/
TUint TBTraceBuffer::UpdateTail(TUint32 aOld, TUint32 aNew)
	{
	if (__e32_atomic_cas_rel32(&iTail, &aOld, aNew))
		return aNew;	// if iTail==aOld, set iTail=aNew and return aNew
	return 0;	// else return 0
	}


/**
Copy data out of the main trace buffer into the 'copy buffer'.
This may fail if the data is overwritten before it hase been successfully copied.
@param aData The buffer offset where the available trace data is located.
@param aTail The value which iTail is expected to have if no more overwrites have occurred
@param aSize The size of data at aTail
@return The number of bytes successfully copied.
@post iBuffer.iTail has been updated to point to the trace record following those copied.
*/
TInt TBTraceBuffer::CopyData(TUint aData, TUint aTail, TInt aSize)
	{
	// clip size to copy buffer
	TInt maxSize = iCopyBufferSize;
	if (aSize>maxSize)
		aSize = Adjust(aData,maxSize);

	if (iTail & 1)
		return 0; // give up if data we're about to copy has been overwritten

	memcpy((TUint8*)this+iCopyBuffer, (TUint8*)this+aData, aSize);

	if (!UpdateTail(aTail, aData+aSize))
		return 0;

	return aSize;
	}


/**
Get pointer to as much contiguous trace data as is available.
@param aData Pointer to the first byte of trace data.
@return The number of bytes of trace data available at aData.
*/
TInt TBTraceBuffer::GetData(TUint8*& aData)
	{
	TInt retries = 4;

	// get availabe data...
	TUint data, tail;
	TInt size = Data(data, tail);
	if (!size)
		return size; // no data available

	if (!(iMode & RBTrace::EFreeRunning))
		{
		// if we got an error from Data but aren't in free-running mode, something has
		// been interrupting the writing thread for some time while it has interrupts
		// turned off. give up.
		if (size<0)
			return 0;
		// were not in free-running mode, so we can leave the data where it is...
		aData = (TUint8*)this + data;
		iTail = data;	// OK since iTail never updated by kernel in this mode
		return size;
		}

	// if we couldn't get a consistent snapshot of the pointers, we need to disable
	// free running, otherwise we will stall for a very long time.
	if (size==KErrInUse)
		goto giveup;

	// copy data to the copy buffer...
	aData = (TUint8*)this + iCopyBuffer;
	size = CopyData(data, tail, size);
	if (size)
		return size; // success

	// data copy failed because new data was added during copy; this happens when the buffer
	// is full, so now we'll attempt to discard data to give us some breathing space...
	while(retries)
		{
		// see what data is available...
		size = Data(data, tail);
		if (!size)
			return size; // no data in buffer (shouldn't happen because buffer was full to start with)
		if (size==KErrInUse)
			goto giveup;

		// discard a proportion of the data...
		TInt discard = iCopyBufferSize>>retries;
		if (discard>=size)
			discard = size;
		else
			discard = Adjust(data, discard); // make sure we only discard a whole number of trace records
		size -= discard;
		data = UpdateTail(tail, data+discard);
		if (!data)
			continue;	// tail was updated before we could discard, so try again
		if (!size)
			continue;	// we discarded everything - make sure and then exit

		// try and copy remaining data...
		size = CopyData(data, data, size);
		if (size)
			break; // success!

		// loop around for another go, discard more this time
		--retries;
		}

	if (!size)
		{
giveup:
		// we haven't managed to copy data, so give up and do it with free-running mode off...
		TUint32 mode = __e32_atomic_and_acq32(&iMode, ~(TUint32)RBTrace::EFreeRunning);	/* read original mode and disable free running */
		size = Data(data, tail);
		// as above: if we get an error here then something has been interrupting the writer
		// for an unreasonable time, give up.
		if (size<0)
			return 0;
		size = CopyData(data, tail, size);
		__e32_atomic_store_ord32(&iMode, mode);	/* restore original mode */
		}

	// we discarded some data, so mark first trace record to indicate that some records are missing...
	aData[BTrace::EFlagsIndex] |= BTrace::EMissingRecord;

	return size;
	}


