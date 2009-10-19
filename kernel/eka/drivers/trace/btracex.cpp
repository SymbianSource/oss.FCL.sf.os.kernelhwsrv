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
// e32\drivers\trace\btrace.cpp
// 
//

#include <kernel/kern_priv.h>
#include "platform.h"
#include "drivers/btrace.h"

#if defined(__EPOC32__) && defined(__CPU_X86)
#include <x86.h>
#endif

TBTraceBufferK Buffer;

TBool ChannelOpen = EFalse;

const TUint KCopyBufferMaxSize = 0x10000;

TInt TBTraceBufferK::Create(TInt aSize)
	{
	if(aSize<=0)
		return KErrArgument;
	TUint pageSize = Kern::RoundToPageSize(1);
	aSize = (aSize+pageSize-1)&-(TInt)pageSize;

	TUint recordOffsets = aSize+pageSize;
	TUint recordOffsetsSize = Kern::RoundToPageSize(aSize>>2);
	TUint copyBuffer = recordOffsets+recordOffsetsSize+pageSize;
	TUint copyBufferSize = Kern::RoundToPageSize(aSize>>2);
	if(copyBufferSize>KCopyBufferMaxSize)
		copyBufferSize = KCopyBufferMaxSize;
	TUint chunkSize = copyBuffer+copyBufferSize+pageSize;

	// Create chunk...
    TChunkCreateInfo info;
    info.iType         = TChunkCreateInfo::ESharedKernelSingle;
    info.iMaxSize      = chunkSize;
#ifdef __EPOC32__
	// we want full caching, no execute, default sharing
	new (&info.iMapAttr) TMappingAttributes2(EMemAttNormalCached, EFalse, ETrue);
#endif
    info.iOwnsMemory   = ETrue; // Use memory from system's free pool
    info.iDestroyedDfc = NULL;
	TUint32 mapAttr;
	TInt r = Kern::ChunkCreate(info, iBufferChunk, iAddress, mapAttr);
	if(r==KErrNone)
		r = Kern::ChunkCommit(iBufferChunk, 0, aSize);
	if(r==KErrNone)
		r = Kern::ChunkCommit(iBufferChunk, recordOffsets, recordOffsetsSize);
	if(r==KErrNone)
		r = Kern::ChunkCommit(iBufferChunk, copyBuffer, copyBufferSize);

	// Check errors...
	if(r!=KErrNone)
		{
		Close();
		return r;
		}

	// Initialise state...
	iStart = sizeof(TBTraceBuffer);
	iEnd = aSize;
	iRecordOffsets = (TUint8*)(iAddress+recordOffsets);

	TBTraceBuffer* userBuffer = (TBTraceBuffer*)iAddress;
	userBuffer->iRecordOffsets = recordOffsets;
	userBuffer->iCopyBuffer = copyBuffer;
	userBuffer->iCopyBufferSize = copyBufferSize;

	Reset(0);

#ifndef __SMP__
	TInt irq = NKern::DisableAllInterrupts();
#endif
	iTimestamp2Enabled = EFalse;
	BTrace::SetHandlers(TBTraceBufferK::Trace,TBTraceBufferK::ControlFunction,iOldBTraceHandler,iOldBTraceControl);
#ifndef __SMP__
	NKern::RestoreInterrupts(irq);
#endif

	return KErrNone;
	}


void TBTraceBufferK::Close()
	{
#ifdef __SMP__
	if(iOldBTraceHandler)
		{
		BTrace::THandler handler;
		BTrace::TControlFunction control;
		BTrace::SetHandlers(iOldBTraceHandler,iOldBTraceControl,handler,control);
		iOldBTraceHandler = NULL;
		iOldBTraceControl = NULL;
		}
	TSpinLock* sl = BTrace::LockPtr();
	TInt irq = sl->LockIrqSave();	// guarantees handler can't run at the same time
	DChunk* chunk = iBufferChunk;
	iBufferChunk = NULL;
	iAddress = NULL;
	sl->UnlockIrqRestore(irq);
#else
	TInt irq = NKern::DisableAllInterrupts();
	if(iOldBTraceHandler)
		{
		BTrace::THandler handler;
		BTrace::TControlFunction control;
		BTrace::SetHandlers(iOldBTraceHandler,iOldBTraceControl,handler,control);
		iOldBTraceHandler = NULL;
		iOldBTraceControl = NULL;
		}
	DChunk* chunk = iBufferChunk;
	iBufferChunk = NULL;
	iAddress = NULL;
	NKern::RestoreInterrupts(irq);
#endif
	if(chunk)
		Kern::ChunkClose(chunk);
	}




/**
Helper functions for encoding pseudo- floating point values recoverable by:

	int exponent = (signed char)(encoded_val >> 24);
	int mantissa = encoded_val & 0xFFFFFF;
	double val = mantissa * pow(2, exponent);
*/
TUint EncodeFloatesque(TUint64 val64, TInt exponent)
	{
	// Lose precision until it fits in 24 bits
	TInt round_up = 0;
	while (val64>=0x1000000) 
		{
		round_up = (TInt)(val64&1);
		val64 >>= 1;
		exponent++;
		}
	if (round_up) 
		{
		val64++;
		if (val64>=0x1000000) 
			{
			val64 >>= 1;
			exponent++;
			}
		}

	// Return 8-bit exponent and 24-bit mantissa
	return (TUint)(val64 | (((unsigned char)exponent)<<24));
	}

TUint EncodeReciprocal(TUint val)
	{
	if (val==0) return val;

	// Get reciprocal * 2^64
	TUint64 val64 = val;
	TUint64 div = 0;
	div--;
	val64 = div / val64;
	
	return EncodeFloatesque(val64, -64);
	}

TUint EncodePostDiv(TUint val, TUint divisor)
	{
	TUint64 val64 = val;
	val64 <<= 32;
	val64 = val64 / divisor;
	return EncodeFloatesque(val64, -32);
	}

void BTracePrimeMetatrace()
	{
#ifdef __SMP__
	TUint period1 = EncodeReciprocal(NKern::TimestampFrequency());
	TUint period2 = period1 + (32u<<24);	// timestamp2 period is 2^32 * timestamp1 period
	BTrace12(BTrace::EMetaTrace, BTrace::EMetaTraceTimestampsInfo, period1, period2, 1);
#else
	TUint period1 = EncodeReciprocal(NKern::FastCounterFrequency());
	TUint period2 = EncodePostDiv(NKern::TickPeriod(), 1000000);
	BTrace12(BTrace::EMetaTrace, BTrace::EMetaTraceTimestampsInfo, period1, period2, 0);
#endif
	}

void TBTraceBufferK::Reset(TUint aMode)
	{
#ifdef __SMP__
	TSpinLock* sl = BTrace::LockPtr();
#endif
	TInt irq = __SPIN_LOCK_IRQSAVE(*sl);	// guarantees handler can't run at the same time
	iHead = iStart;
	TBTraceBuffer* userBuffer = (TBTraceBuffer*)iAddress;
	userBuffer->iStart = iStart;
	userBuffer->iEnd = iEnd;
	userBuffer->iHead = iHead;
	userBuffer->iTail = iHead;
	userBuffer->iGeneration = 0;
	userBuffer->iMode = aMode;
	__SPIN_UNLOCK_IRQRESTORE(*sl,irq);
	if(aMode)
		{
		if (BTrace::CheckFilter(BTrace::EMetaTrace))
			BTracePrimeMetatrace();
		BTrace::Prime();
		}
	}


TInt TBTraceBufferK::RequestData(TInt aSize, TDfc* aDfc)
	{
	if(aSize<=0)
		aSize = 1;
#ifdef __SMP__
	TSpinLock* sl = BTrace::LockPtr();
#endif
	TInt irq = __SPIN_LOCK_IRQSAVE(*sl);	// guarantees handler can't run
	TBTraceBuffer* userBuffer = (TBTraceBuffer*)iAddress;
	if(!userBuffer)
		{
		__SPIN_UNLOCK_IRQRESTORE(*sl,irq);
		return KErrNotReady;
		}
	TInt dif = userBuffer->iTail-iHead;
	if(dif>0)
		aSize = 0; // we need no more bytes because all bytes to end of buffer are available
	else
		aSize += dif; // number of bytes extra we need
	if(aSize>0)
		{
		iRequestDataSize = aSize;
		iWaitingDfc = aDfc;
		}
	__SPIN_UNLOCK_IRQRESTORE(*sl,irq);
	if(aSize<=0)
		return KErrCompletion;
	return KErrNone;
	}


#ifndef BTRACE_DRIVER_MACHINE_CODED

TBool TBTraceBufferK::Trace_Impl(TUint32 aHeader,TUint32 aHeader2,const TUint32 aContext,const TUint32 a1,const TUint32 a2,const TUint32 a3,const TUint32 aExtra, const TUint32 aPc, TBool aIncTimestamp2)
	{
#ifndef __SMP__
	TInt irq = NKern::DisableAllInterrupts();
#endif


#ifdef __SMP__
	// Header 2 always present and contains CPU number
	// If Header2 not originally there, add 4 to size
	if (!(aHeader&(BTrace::EHeader2Present<<BTrace::EFlagsIndex*8)))
		aHeader += (4<<BTrace::ESizeIndex*8) + (BTrace::EHeader2Present<<BTrace::EFlagsIndex*8), aHeader2=0;
	aHeader2 = (aHeader2 &~ BTrace::ECpuIdMask) | (NKern::CurrentCpu()<<20);
#endif
#ifdef BTRACE_INCLUDE_TIMESTAMPS
	// Add timestamp to trace...
#if defined(__SMP__)
	aHeader += 8<<BTrace::ESizeIndex*8;
	aHeader |= BTrace::ETimestampPresent<<BTrace::EFlagsIndex*8 | BTrace::ETimestamp2Present<<BTrace::EFlagsIndex*8;
	TUint64 timeStamp = NKern::Timestamp();
#elif defined(__EPOC32__) && defined(__CPU_X86)
	aHeader += 8<<BTrace::ESizeIndex*8;
	aHeader |= BTrace::ETimestampPresent<<BTrace::EFlagsIndex*8 | BTrace::ETimestamp2Present<<BTrace::EFlagsIndex*8;
	TUint64 timeStamp = X86::Timestamp();
#else
	TUint32 timeStamp = NKern::FastCounter();
	TUint32 timeStamp2 = 0;
	if (aIncTimestamp2)
		{
		timeStamp2 = NKern::TickCount();
		aHeader += 8<<BTrace::ESizeIndex*8;
		aHeader |= (BTrace::ETimestampPresent | BTrace::ETimestamp2Present) << BTrace::EFlagsIndex*8;
		}
	else
		{
		aHeader += 4<<BTrace::ESizeIndex*8;
		aHeader |= BTrace::ETimestampPresent<<BTrace::EFlagsIndex*8;
		}
#endif
#endif
	TUint size = (aHeader+3)&0xfc;

	TBTraceBufferK& buffer = Buffer;
	TLinAddr address = buffer.iAddress;
	TBTraceBuffer& user_buffer = *(TBTraceBuffer*)address;
	++user_buffer.iGeneration;	// atomic not required since only driver modifies iGeneration
#ifdef __SMP__
	__e32_memory_barrier();
#endif
	TUint start = buffer.iStart;
	TUint end = buffer.iEnd;
	TUint orig_head = buffer.iHead;
	TInt requestDataSize = buffer.iRequestDataSize;
	TUint8* recordOffsets = buffer.iRecordOffsets;
	TUint32 orig_tail = user_buffer.iTail;
	TUint32 newHead, head, tail;

	if(!(user_buffer.iMode&RBTrace::EEnable))
		goto trace_off;

retry:
	head = orig_head;
	tail = orig_tail &~ 1;
	newHead = head+size;
	if(newHead>end)
		{
		requestDataSize = 0; 
		newHead = start+size;
		if(head<tail || tail<newHead+1)
			{
			if(!(user_buffer.iMode&RBTrace::EFreeRunning))
				goto trace_dropped;
			user_buffer.iWrap = head;
			head = start;
			tail = newHead+(recordOffsets[newHead>>2]<<2);
			goto overwrite;
			}
		user_buffer.iWrap = head;
		head = start;
		}
	else if(head<tail && tail<=newHead)
		{
		{
		requestDataSize = 0; 
		TUint wrap = user_buffer.iWrap;
		if(!(user_buffer.iMode&RBTrace::EFreeRunning))
			goto trace_dropped;
		if(newHead<end && newHead<wrap)
			{
			tail = newHead+(recordOffsets[newHead>>2]<<2);
			if(tail>=end || tail>=wrap)
				tail = start;
			}
		else
			tail = start;
		}
overwrite:
		*(TUint32*)(address+tail) |= BTrace::EMissingRecord<<(BTrace::EFlagsIndex*8);
		if (!__e32_atomic_cas_ord32(&user_buffer.iTail, &orig_tail, tail|1))
			goto retry;	// go round again if user side has already updated the tail pointer
		}

	buffer.iRequestDataSize = requestDataSize-size;

	{
	recordOffsets += head>>2;
	TUint32* src;
	TUint32* dst = (TUint32*)((TUint)address+head);
	size >>= 2; // we are now counting words, not bytes

	// store first word of trace...
	TUint w = aHeader;
	if(buffer.iDropped)
		{
		buffer.iDropped = 0;
		w |= BTrace::EMissingRecord<<(BTrace::EFlagsIndex*8); 
		}
	*recordOffsets++ = (TUint8)size;
	--size;
	*dst++ = w;

#ifndef __SMP__
	if(aHeader&(BTrace::EHeader2Present<<(BTrace::EFlagsIndex*8)))
#endif
		{
		w = aHeader2;
		*recordOffsets++ = (TUint8)size;
		--size;
		*dst++ = w;
		}

#ifdef BTRACE_INCLUDE_TIMESTAMPS
	// store timestamp...
#if defined(__SMP__) || (defined(__EPOC32__) && defined(__CPU_X86))
	*recordOffsets++ = (TUint8)size;
	--size;
	*dst++ = TUint32(timeStamp);
	*recordOffsets++ = (TUint8)size;
	--size;
	*dst++ = TUint32(timeStamp>>32);
#else
	*recordOffsets++ = (TUint8)size;
	--size;
	*dst++ = timeStamp;
	if (aIncTimestamp2)
		{
		*recordOffsets++ = (TUint8)size;
		--size;
		*dst++ = timeStamp2;
		}
#endif
#endif

	if(aHeader&(BTrace::EContextIdPresent<<(BTrace::EFlagsIndex*8)))
		{
		w = aContext;
		*recordOffsets++ = (TUint8)size;
		--size;
		*dst++ = w;
		}

	if(aHeader&(BTrace::EPcPresent<<(BTrace::EFlagsIndex*8)))
		{
		w = aPc;
		*recordOffsets++ = (TUint8)size;
		--size;
		*dst++ = w;
		}

	if(aHeader&(BTrace::EExtraPresent<<(BTrace::EFlagsIndex*8)))
		{
		w = aExtra;
		*recordOffsets++ = (TUint8)size;
		--size;
		*dst++ = w;
		}

	// store remainding words of trace...
	if(size)
		{
		w = a1;
		*recordOffsets++ = (TUint8)size;
		--size;
		*dst++ = w;
		if(size)
			{
			w = a2;
			*recordOffsets++ = (TUint8)size;
			--size;
			*dst++ = w;
			if(size)
				{
				if(size==1)
					{
					w = a3;
					*recordOffsets++ = (TUint8)size;
					*dst++ = w;
					}
				else
					{
					src = (TUint32*)a3;
					do
						{
						w = *src++;
						*recordOffsets++ = (TUint8)size;
						--size;
						*dst++ = w;
						}
					while(size);
					}
				}
			}
		}
	}
	buffer.iHead = newHead;
#ifdef __SMP__
	__e32_memory_barrier();	// make sure written data is observed before head pointer update
#endif
	user_buffer.iHead = newHead;

	{
	TDfc* dfc = (TDfc*)buffer.iWaitingDfc;
	if(dfc && buffer.iRequestDataSize<=0)
		{
		buffer.iWaitingDfc = NULL;
		dfc->RawAdd();
		}
	}

#ifdef __SMP__
	__e32_memory_barrier();
#endif
	++user_buffer.iGeneration;	// atomic not required since only driver modifies iGeneration
#ifndef __SMP__
	NKern::RestoreInterrupts(irq);
#endif
	return ETrue;


trace_dropped:
	buffer.iRequestDataSize = 0; 
	buffer.iDropped = ETrue;
#ifdef __SMP__
	__e32_memory_barrier();
#endif
	++user_buffer.iGeneration;	// atomic not required since only driver modifies iGeneration
#ifndef __SMP__
	NKern::RestoreInterrupts(irq);
#endif
	return ETrue;

trace_off:
#ifdef __SMP__
	__e32_memory_barrier();
#endif
	++user_buffer.iGeneration;	// atomic not required since only driver modifies iGeneration
#ifndef __SMP__
	NKern::RestoreInterrupts(irq);
#endif
	return EFalse;
	}

TBool TBTraceBufferK::TraceWithTimestamp2(TUint32 aHeader,TUint32 aHeader2,const TUint32 aContext,const TUint32 a1,const TUint32 a2,const TUint32 a3,const TUint32 aExtra,const TUint32 aPc)
	{
	return Trace_Impl(aHeader, aHeader2, aContext, a1, a2, a3, aExtra, aPc, ETrue);
	}

TBool TBTraceBufferK::Trace(TUint32 aHeader,TUint32 aHeader2,const TUint32 aContext,const TUint32 a1,const TUint32 a2,const TUint32 a3,const TUint32 aExtra,const TUint32 aPc)
	{
	return Trace_Impl(aHeader, aHeader2, aContext, a1, a2, a3, aExtra, aPc, EFalse);
	}

#endif // BTRACE_DRIVER_MACHINE_CODED


TInt TBTraceBufferK::ControlFunction(BTrace::TControl aFunction, TAny* aArg1, TAny* aArg2)
	{
	switch(aFunction)
		{
	case BTrace::ECtrlSystemCrashed:
		if(Buffer.iAddress)
			((TBTraceBuffer*)Buffer.iAddress)->iMode = 0; // turn off trace
		return KErrNone;
		
	case BTrace::ECtrlCrashReadFirst:
		Buffer.iCrashReadPart = 0;
		// fall through...
	case BTrace::ECtrlCrashReadNext:
		Buffer.CrashRead(*(TUint8**)aArg1,*(TUint*)aArg2);
		++Buffer.iCrashReadPart;
		return KErrNone;
				
	default:
		return KErrNotSupported;
		}
	}


void TBTraceBufferK::CrashRead(TUint8*& aData, TUint& aSize)
	{
	// start by assuming no data...
	aData = 0;
	aSize = 0;
	
	TBTraceBuffer* userBuffer = (TBTraceBuffer*)iAddress;
	if(!userBuffer)
		return; // no trace buffer, so end...

	TUint head = iHead;
	TUint tail = userBuffer->iTail;
	TUint8* data = (TUint8*)userBuffer;
	
	if(head>tail)
		{
		// data is in one part...
		if(iCrashReadPart==0)
			{
			aData = data+tail;
			aSize = head-tail;
			}
		// else no more parts
		}
	else if(head<tail)
		{
		// data is in two parts...
		if(iCrashReadPart==0)
			{
			// first part...
			aData = data+tail;
			aSize = userBuffer->iWrap-tail;
			}
		else if(iCrashReadPart==1)
			{
			// second part...
			aData = data+iStart;
			aSize = head-iStart;
			}
		// else no more parts
		}
	}


//
// LDD
//

class DBTraceFactory : public DLogicalDevice
	{
public:
	virtual TInt Install();
	virtual void GetCaps(TDes8& aDes) const;
	virtual TInt Create(DLogicalChannelBase*& aChannel);
	};


class DBTraceChannel : public DLogicalChannelBase
	{
public:
	DBTraceChannel();
	virtual ~DBTraceChannel();
	//	Inherited from DObject
	virtual TInt RequestUserHandle(DThread* aThread, TOwnerType aType);
	// Inherited from DLogicalChannelBase
	virtual TInt DoCreate(TInt aUnit, const TDesC8* anInfo, const TVersion& aVer);
	virtual TInt Request(TInt aReqNo, TAny* a1, TAny* a2);
	//
	static void WaitCallback(TAny* aSelf);
private:
	DThread* iClient;
	TClientRequest*	iWaitRequest;
	TDfc iWaitDfc;
	TBool iOpened;
	TInt iFilter2Count;
	TUint32* iFilter2;
	TUint32* iFilter2Set;
	TBool iTimestamp2Enabled;
	};


//
// DBTraceFactory
//

TInt DBTraceFactory::Install()
	{
	return SetName(&RBTrace::Name());
	}

void DBTraceFactory::GetCaps(TDes8& aDes) const
	{
	Kern::InfoCopy(aDes,0,0);
	}

TInt DBTraceFactory::Create(DLogicalChannelBase*& aChannel)
	{
	aChannel=new DBTraceChannel();
	if(!aChannel)
		return KErrNoMemory;
	return KErrNone;
	}

void syncDfcFn(TAny* aPtr)
	{
	NKern::FSSignal((NFastSemaphore*)aPtr);
	}

void Sync(TDfcQue* aDfcQ)
	{
	NFastSemaphore s(0);
	TDfc dfc(&syncDfcFn, &s, aDfcQ, 0);
	dfc.Enque();
	NKern::FSWait(&s);
	}

//
// DBTraceChannel
//

DBTraceChannel::DBTraceChannel()
	: iWaitDfc(WaitCallback,this,Kern::DfcQue1(),7)
	{
	}

DBTraceChannel::~DBTraceChannel()
	{
	delete iFilter2Set;
	Buffer.iWaitingDfc = NULL;
	iWaitDfc.Cancel();
	Sync(Kern::DfcQue1());
	if (iWaitRequest)
		{
		Kern::QueueRequestComplete(iClient, iWaitRequest, KErrCancel);	// does nothing if request not pending
		Kern::DestroyClientRequest(iWaitRequest);
		}
	if (iOpened)
		__e32_atomic_swp_ord32(&ChannelOpen, 0);
	}

TInt DBTraceChannel::DoCreate(TInt /*aUnit*/, const TDesC8* /*aInfo*/, const TVersion& /*aVer*/)
	{
//	_LIT_SECURITY_POLICY_C2(KSecurityPolicy,ECapabilityReadDeviceData,ECapabilityWriteDeviceData);
//	if(!KSecurityPolicy().CheckPolicy(&Kern::CurrentThread(),__PLATSEC_DIAGNOSTIC_STRING("Checked by BTRACE")))
//		return KErrPermissionDenied;
	iClient = &Kern::CurrentThread();
	TInt r = Kern::CreateClientRequest(iWaitRequest);
	if (r!=KErrNone)
		return r;
	if (__e32_atomic_swp_ord32(&ChannelOpen, 1))
		return KErrInUse;
	iOpened = ETrue;
	return KErrNone;
	}


TInt DBTraceChannel::RequestUserHandle(DThread* aThread, TOwnerType aType)
	{
	if (aType!=EOwnerThread || aThread!=iClient)
		return KErrAccessDenied;
	return KErrNone;
	}

void DBTraceChannel::WaitCallback(TAny* aSelf)
	{
	DBTraceChannel& c = *(DBTraceChannel*)aSelf;
	Kern::QueueRequestComplete(c.iClient, c.iWaitRequest, KErrNone);
	}

TInt DBTraceChannel::Request(TInt aReqNo, TAny* a1, TAny* a2)
	{
	TInt r;
	TBTraceBufferK& buffer = Buffer;

	switch(aReqNo)
		{
	case RBTrace::EOpenBuffer:
		NKern::ThreadEnterCS();
		if(!Buffer.iBufferChunk)
			r = buffer.Create(0x100000);
		else
			r = KErrNone;
		if(r==KErrNone)
			r = Kern::MakeHandleAndOpen(NULL, buffer.iBufferChunk);
		NKern::ThreadLeaveCS();
		return r;

	case RBTrace::EResizeBuffer:
		NKern::ThreadEnterCS();
		buffer.Close();
		r = buffer.Create((TInt)a1);
		NKern::ThreadLeaveCS();
		return r;

	case RBTrace::ESetFilter:
		{
		TInt old = BTrace::SetFilter((BTrace::TCategory)(TInt)a1,(TInt)a2);
		if((TInt)a2==1 && old==0) // filter turned on?
			{
			if ((TInt)a1==BTrace::EMetaTrace) 
				BTracePrimeMetatrace();
			BTrace::Prime((TInt)a1); // prime this trace category
			}
		return old;
		}

	case RBTrace::ESetFilter2:
		return BTrace::SetFilter2((TUint32)a1,(TBool)a2);

	case RBTrace::ESetFilter2Array:
		{
		NKern::ThreadEnterCS();
		delete iFilter2Set;
		TInt size = (TInt)a2*sizeof(TUint32);
		TUint32* buffer = (TUint32*)Kern::Alloc(size);
		iFilter2Set = buffer;
		NKern::ThreadLeaveCS();
		if(!buffer)
			return KErrNoMemory;
		kumemget32(buffer,a1,size);
		r = BTrace::SetFilter2(buffer,(TInt)a2);
		NKern::ThreadEnterCS();
		delete iFilter2Set;
		iFilter2Set = 0;
		NKern::ThreadLeaveCS();
		return r;
		}

	case RBTrace::ESetFilter2Global:
		BTrace::SetFilter2((TBool)a1);
		return KErrNone;

	case RBTrace::EGetFilter2Part1:
		{
		NKern::ThreadEnterCS();
		delete iFilter2;
		iFilter2 = 0;
		iFilter2Count = 0;
		TInt globalFilter = 0;
		iFilter2Count = BTrace::Filter2(iFilter2,globalFilter);
		NKern::ThreadLeaveCS();
		kumemput32(a2,&globalFilter,sizeof(TBool));
		return iFilter2Count;
		}

	case RBTrace::EGetFilter2Part2:
		if((TInt)a2!=iFilter2Count)
			return KErrArgument;
		if(iFilter2Count>0)
			kumemput32(a1,iFilter2,iFilter2Count*sizeof(TUint32));
		NKern::ThreadEnterCS();
		delete iFilter2;
		iFilter2 = 0;
		iFilter2Count = 0;
		NKern::ThreadLeaveCS();
		return KErrNone;

	case RBTrace::ERequestData:
		if (iWaitRequest->SetStatus((TRequestStatus*)a1) != KErrNone)
			Kern::PanicCurrentThread(RBTrace::Name(),RBTrace::ERequestAlreadyPending);
		r = buffer.RequestData((TInt)a2,&iWaitDfc);
		if (r!=KErrNone)
			{
			iWaitRequest->Reset();
			TRequestStatus* s = (TRequestStatus*)a1;
			if (r==KErrCompletion)
				r = KErrNone;
			Kern::RequestComplete(s, r);
			}
		return r;

	case RBTrace::ECancelRequestData:
		buffer.iWaitingDfc = NULL;
		iWaitDfc.Cancel();
		Kern::QueueRequestComplete(iClient, iWaitRequest, KErrCancel);
		return KErrNone;

	case RBTrace::ESetSerialPortOutput:
		{
		TUint mode = Kern::ESerialOutNever+(TUint)a1;
		mode = Kern::SetTextTraceMode(mode,Kern::ESerialOutMask);
		mode &= Kern::ESerialOutMask;
		return mode-Kern::ESerialOutNever;
		}

	case RBTrace::ESetTimestamp2Enabled:
		{
		TBool old = iTimestamp2Enabled;
		iTimestamp2Enabled = (TBool)a1;
		BTrace::TControlFunction oldControl;
		BTrace::THandler oldHandler;
		BTrace::THandler handler = iTimestamp2Enabled ? TBTraceBufferK::TraceWithTimestamp2 : TBTraceBufferK::Trace;
		BTrace::SetHandlers(handler,TBTraceBufferK::ControlFunction,oldHandler,oldControl);
		return old;
		}

	default:
		break;
		}
	return KErrNotSupported;
	}


DECLARE_EXTENSION_LDD()
	{
	return new DBTraceFactory;
	}

#ifdef __WINS__
DECLARE_STANDARD_EXTENSION()
#else
DECLARE_EXTENSION_WITH_PRIORITY(KExtensionMaximumPriority)
#endif
	{
	TSuperPage& superPage = Kern::SuperPage();
	TInt bufferSize = superPage.iInitialBTraceBuffer;
	if(!bufferSize)
		bufferSize = 0x10000;
	TInt r=Buffer.Create(bufferSize);
	if(r==KErrNone)
		Buffer.Reset(superPage.iInitialBTraceMode);
	return r;
	}

