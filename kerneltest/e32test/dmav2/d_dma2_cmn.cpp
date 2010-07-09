/*
* Copyright (c) 2009-2010 Nokia Corporation and/or its subsidiary(-ies).
* All rights reserved.
* This component and the accompanying materials are made available
* under the terms of "Eclipse Public License v1.0"
* which accompanies this distribution, and is available
* at the URL "http://www.eclipse.org/legal/epl-v10.html".
*
* Initial Contributors:
* Nokia Corporation - initial contribution.
*
* Contributors:
*
* Description: Implmentation of DMAv2 test code, common
* to both user and kernel side
*
*/
#ifdef __KERNEL_MODE__
#include <platform.h>

#ifdef __DMASIM__
#ifdef __WINS__
typedef TLinAddr TPhysAddr;
#endif
static inline TPhysAddr LinToPhys(TLinAddr aLin) {return aLin;}
#else
static inline TPhysAddr LinToPhys(TLinAddr aLin) {return Epoc::LinearToPhysical(aLin);}
#endif
#endif

#include "d_dma2.h"


TInt Log2(TInt aNum)
	{
	TInt res = -1;
	while(aNum)
		{
		res++;
		aNum >>= 1;
		}
	return res;
	}

TCallbackRecord::TCallbackRecord(
		TCbContext aContext,
		TInt aReq,
		TInt aReqSrc,
		TInt aReqDst,
		TInt aDes,
		TInt aDesSrc,
		TInt aDesDst,
		TInt aFrame,
		TInt aFrameSrc,
		TInt aFrameDst,
		TInt aPause,
		TInt aPauseSrc,
		TInt aPauseDst,
		TDmaResult aResult
	)
	//Default iIsrRedoRequestResult is 1 as this is an invalid error code
	:iResult(aResult), iContext(aContext), iIsrRedoRequestResult(1)
	{
	SetCount(EDmaCallbackRequestCompletion, aReq);
	SetCount(EDmaCallbackRequestCompletion_Src, aReqSrc);
	SetCount(EDmaCallbackRequestCompletion_Dst, aReqDst);
	SetCount(EDmaCallbackDescriptorCompletion, aDes);
	SetCount(EDmaCallbackDescriptorCompletion_Src, aDesSrc);
	SetCount(EDmaCallbackDescriptorCompletion_Dst, aDesDst);
	SetCount(EDmaCallbackFrameCompletion, aFrame);
	SetCount(EDmaCallbackFrameCompletion_Src, aFrameSrc);
	SetCount(EDmaCallbackFrameCompletion_Dst, aFrameDst);
	SetCount(EDmaCallbackLinkedListPaused, aPause);
	SetCount(EDmaCallbackLinkedListPaused_Src, aPauseSrc);
	SetCount(EDmaCallbackLinkedListPaused_Dst, aPauseDst);
	}

TCallbackRecord TCallbackRecord::Empty()
	{
	return TCallbackRecord(EInvalid,0,0,0,0,0,0,0,0,0,0,0,0,EDmaResultError);
	}

void TCallbackRecord::Reset()
	{
	new (this) TCallbackRecord();
	}

TBool TCallbackRecord::operator == (const TCallbackRecord aOther) const
	{
	return (memcompare((TUint8*)this, sizeof(*this), (TUint8*)&aOther, sizeof(aOther)) == 0);
	}

TInt TCallbackRecord::GetCount(TDmaCallbackType aCbType) const
	{
	const TInt index = BitToIndex(aCbType);
	return iCallbackLog[index];
	}

void TCallbackRecord::SetCount(TDmaCallbackType aCbType, TInt aCount)
	{
	const TInt index = BitToIndex(aCbType);
	iCallbackLog[index] = aCount;
	}

TInt TCallbackRecord::BitToIndex(TDmaCallbackType aCbType) const
	{
	const TInt index = Log2(aCbType);
	TEST_ASSERT(index >=0 && index < KNumberOfCallbacks);

	return index;
	}

void TCallbackRecord::ProcessCallback(TUint aCallbackMask, TDmaResult aResult)
	{
	// This function may be called several
	// times and will accumulate the number of each callback
	// received. However, it will only ever remember the last
	// result and context value,
	iResult = aResult;
	iContext = CurrentContext();
	TEST_ASSERT(iContext != EInvalid);

	for(TInt i=0; i < KNumberOfCallbacks; i++)
		{
		if(aCallbackMask & 1)
			{
			iCallbackLog[i]++;
			}
		aCallbackMask >>= 1;
		}
	// Assert that we have handled all bits
	// if not then maybe KNumberOfCallbacks is too small
	// or there is a spurious bit in aCallbackMask
	TEST_ASSERT(aCallbackMask == 0);
	}

TCallbackRecord::TCbContext TCallbackRecord::CurrentContext() const
	{
#ifdef __KERNEL_MODE__
	switch(NKern::CurrentContext())
		{
	case NKern::EThread:
		return EThread;
	case NKern::EInterrupt:
		return EIsr;
	//Fall-through: If context is IDFC or the EEscaped marker occur
	//it is an error
	case NKern::EIDFC:
	case NKern::EEscaped:
	default:
		return EInvalid;
		}
#else
	//for the benefit of user-mode testing
	return EThread;
#endif
	}

void TCallbackRecord::Print() const
	{
	PRINT(GetCount(EDmaCallbackRequestCompletion));
	PRINT(GetCount(EDmaCallbackRequestCompletion_Src));
	PRINT(GetCount(EDmaCallbackRequestCompletion_Dst));
	PRINT(GetCount(EDmaCallbackDescriptorCompletion));
	PRINT(GetCount(EDmaCallbackDescriptorCompletion_Src));
	PRINT(GetCount(EDmaCallbackDescriptorCompletion_Dst));
	PRINT(GetCount(EDmaCallbackFrameCompletion));
	PRINT(GetCount(EDmaCallbackFrameCompletion_Src));
	PRINT(GetCount(EDmaCallbackFrameCompletion_Dst));
	PRINT(GetCount(EDmaCallbackLinkedListPaused));
	PRINT(GetCount(EDmaCallbackLinkedListPaused_Src));
	PRINT(GetCount(EDmaCallbackLinkedListPaused_Dst));
	PRINT(iResult);
	PRINT(iContext);
	PRINT(iIsrRedoRequestResult);
	}

TDmacTestCaps::TDmacTestCaps()
	:iPILVersion(1)
	{
	}

TDmacTestCaps::TDmacTestCaps(const SDmacCaps& aDmacCaps, TInt aVersion)
	:SDmacCaps(aDmacCaps), iPILVersion(aVersion)
	{}

TAddrRange::TAddrRange(TUint aStart, TUint aLength)
	:iStart(aStart), iLength(aLength)
	{
	}

TBool TAddrRange::Contains(TAddrRange aRange) const
	{
	return Contains(aRange.Start()) && Contains(aRange.End());
	}

TBool TAddrRange::Overlaps(const TAddrRange& aRange) const
	{
	return (aRange.Contains(iStart) || aRange.Contains(End()) ||
			Contains(aRange.Start()) || Contains(aRange.End()));
	}

TBool TAddrRange::IsFilled(TUint8 aValue) const
	{
	TUint8* buffer = reinterpret_cast<TUint8*>(iStart);
	for(TUint i = 0; i < iLength; i++)
		{
		if(buffer[i] != aValue)
			return EFalse;
		}
	return ETrue;
	}

/**
If addresses have been left as KPhysAddrInvalid or the count as 0 (ie. the
default values used for IsrRedoRequest) then substitute the values from
aTransferArgs.
*/
void TAddressParms::Substitute(const TDmaTransferArgs& aTransferArgs)
	{
	Substitute(GetAddrParms(aTransferArgs));
	}

/**
If addresses have been left as KPhysAddrInvalid or the count as 0 (ie. the
default values used for IsrRedoRequest) then substitute the values from
aTransferArgs.
*/
void TAddressParms::Substitute(const TAddressParms& aAddrParams)
	{
	if(iSrcAddr == KPhysAddrInvalidUser)
		iSrcAddr = aAddrParams.iSrcAddr;

	if(iDstAddr == KPhysAddrInvalidUser)
		iDstAddr = aAddrParams.iDstAddr;

	if(iTransferCount == 0)
		iTransferCount = aAddrParams.iTransferCount;
	}

/**
Addresses are converted into absolute,
addresses (virtual in user mode, physical in kernel)
unless they are KPhysAddrInvalid
*/
void TAddressParms::Fixup(TLinAddr aChunkBase)
	{
	if(iSrcAddr != KPhysAddrInvalidUser)
		{
		iSrcAddr += aChunkBase;

#ifdef __KERNEL_MODE__
		iSrcAddr = LinToPhys(iSrcAddr);
		TEST_ASSERT(iSrcAddr != KPhysAddrInvalid);
#endif
		}
#ifndef __KERNEL_MODE__
	else
		{
		// Substitute must be called before
		// Fixup on user side
		TEST_FAULT;
		}
#endif

	if(iDstAddr != KPhysAddrInvalidUser)
		{
		iDstAddr += aChunkBase;

#ifdef __KERNEL_MODE__
		iDstAddr = LinToPhys(iDstAddr);
		TEST_ASSERT(iDstAddr != KPhysAddrInvalid);
#endif
		}
#ifndef __KERNEL_MODE__
	else
		{
		// Substitute must be called before
		// Fixup on user side
		TEST_FAULT;
		}
#endif
	}

TBool TAddressParms::CheckRange(TLinAddr aStart, TUint aSize)
	{
	TAddrRange chunk(aStart, aSize);
	return chunk.Contains(SourceRange()) && chunk.Contains(DestRange());
	}

/**
@return ETrue if the source or destination range of this object
overlaps with aRange
*/
TBool TAddressParms::Overlaps(const TAddrRange aRange) const
	{
	return SourceRange().Overlaps(aRange) || DestRange().Overlaps(aRange);
	}

/**
@return ETrue if either the source or dest range of this
overlap with either of those of aParm
*/
TBool TAddressParms::Overlaps(const TAddressParms aParm) const
	{
	return Overlaps(aParm.SourceRange()) || Overlaps(aParm.DestRange());
	}

TBool TAddressParms::operator==(const TAddressParms& aOther) const
	{
	return iSrcAddr == aOther.iSrcAddr &&
		iDstAddr == aOther.iDstAddr &&
		iTransferCount == aOther.iTransferCount;
	}

TAddressParms GetAddrParms(const TDmaTransferArgs& aArgs)
	{
	return TAddressParms(aArgs);
	}

TAddrRange TAddressParms::SourceRange() const
	{
	return TAddrRange(iSrcAddr, iTransferCount);
	}

TAddrRange TAddressParms::DestRange() const
	{
	return TAddrRange(iDstAddr, iTransferCount);
	}

void TAddressParms::MakePhysical()
	{
#ifdef __KERNEL_MODE__
	iSrcAddr = LinToPhys(iSrcAddr);
	TEST_ASSERT(iSrcAddr != KPhysAddrInvalid);
	iDstAddr = LinToPhys(iDstAddr);
	TEST_ASSERT(iDstAddr != KPhysAddrInvalid);
#else
	TEST_FAULT;
#endif
	}

void SetAddrParms(TDmaTransferArgs& aTransferArgs, const TAddressParms& aAddrParams)
	{
	aTransferArgs.iSrcConfig.iAddr = aAddrParams.iSrcAddr;
	aTransferArgs.iDstConfig.iAddr = aAddrParams.iDstAddr;
	aTransferArgs.iTransferCount = aAddrParams.iTransferCount;
	}

TIsrRequeArgs TIsrRequeArgsSet::GetArgs()
	{
	TEST_ASSERT(!IsEmpty());
	const TIsrRequeArgs args(iRequeArgs[iIndex]);
	iIndex++;
	iCount--;
	return args;
	}

void TIsrRequeArgsSet::Substitute(const TDmaTransferArgs& aTransferArgs)
	{
	TAddressParms initial(aTransferArgs);

	//if on user side it is assumed that aTransferArgs addresses will be offset
	//based (from a virtual address). In kernel mode it is expected that address
	//will be absolute virtual addresses, and must therefore be made physical
#ifdef __KERNEL_MODE__
	initial.MakePhysical();
#endif

	const TAddressParms* previous = &initial;

	for(TInt i=0; i<iCount; i++)
		{
		TAddressParms& current = iRequeArgs[i];
		current.Substitute(*previous);
		previous = &current;
		}
	}

void TIsrRequeArgsSet::Fixup(TLinAddr aChunkBase)
	{
	for(TInt i=0; i<iCount; i++)
		{
		iRequeArgs[i].Fixup(aChunkBase);
		}
	}

/** Check that both source and destination of ISR reque args will lie within the
range specified by aStart and aSize.

@param aStart The linear base address of the region
@param aSize The size of the region
*/
TBool TIsrRequeArgs::CheckRange(TLinAddr aStart, TUint aSize) const
	{
	TUint chunkStart = 0;
#ifdef __KERNEL_MODE__
	chunkStart = LinToPhys(aStart);
	TEST_ASSERT(chunkStart != KPhysAddrInvalid);
#else
	chunkStart = aStart;
#endif

	// If an address is still KPhysAddrInvalid it means the arguments haven't
	// yet been substituted
	TAddrRange chunk(chunkStart, aSize);
	TBool sourceOk = (iSrcAddr != KPhysAddrInvalid) && chunk.Contains(SourceRange());

	TBool destOk = (iDstAddr != KPhysAddrInvalid) && chunk.Contains(DestRange());

	TBool ok = sourceOk && destOk;
	if(!ok)
		{
		PRINTF(("Error, re-queue args: "));
		TBuf<128> buf;
		AppendString(buf);
		PRINTF(("%S", &buf));
		PRINTF(("overflow buffer base=0x%08x, size=0x%08x", chunkStart, aSize));
		}
	return ok;
	}

TBool TIsrRequeArgsSet::CheckRange(TLinAddr aAddr, TUint aSize) const
	{
	for(TInt i=0; i<iCount; i++)
		{
		if(!iRequeArgs[i].CheckRange(aAddr, aSize))
			return EFalse;
		}
	return ETrue;
	}

TBool TIsrRequeArgsSet::CheckRange(TLinAddr aAddr, TUint aSize, const TDmaTransferArgs& aInitialParms) const
	{
	// apply substitution, without modifying the original
	TIsrRequeArgsSet copy(*this);
	copy.Substitute(aInitialParms);

	return copy.CheckRange(aAddr, aSize);
	}

