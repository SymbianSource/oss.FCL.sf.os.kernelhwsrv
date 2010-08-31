// Copyright (c) 2002-2010 Nokia Corporation and/or its subsidiary(-ies).
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
// e32test\dma\d_dma.cpp
// 
//

#include "platform.h"
#include <kernel/kern_priv.h>
#include <drivers/dma.h>
#include "d_dma.h"

_LIT(KClientPanicCat, "D_DMA");
_LIT(KDFCThreadName,"D_DMA_DFC_THREAD");
const TInt KDFCThreadPriority=26;

//////////////////////////////////////////////////////////////////////////////

//
// Class abstracting the way DMA buffers are created and destroyed to
// allow tests to run both on WINS and hardware.
//

class TBufferMgr
	{
public:
	TInt Alloc(TInt aIdx, TInt aSize);
	void FreeAll();
	TUint8* Addr(TInt aIdx) const;
	TPhysAddr PhysAddr(TInt aIdx) const;
	TInt Size(TInt aIdx) const;
	enum { KMaxBuf = 8 };
private:
#ifdef __WINS__
	struct {TUint8* iPtr; TInt iSize;} iBufs[KMaxBuf];
#else
	struct {DPlatChunkHw* iChunk; TInt iSize;} iBufs[KMaxBuf];
#endif
	};

#ifdef __WINS__

TUint8* TBufferMgr::Addr(TInt aIdx) const
	{
 	__ASSERT_DEBUG(0 <= aIdx && aIdx < KMaxBuf, Kern::PanicCurrentThread(KClientPanicCat, __LINE__));
	__ASSERT_DEBUG(iBufs[aIdx].iPtr != NULL, Kern::PanicCurrentThread(KClientPanicCat, __LINE__));
	return iBufs[aIdx].iPtr;
	}


TInt TBufferMgr::Size(TInt aIdx) const
	{
 	__ASSERT_DEBUG(0 <= aIdx && aIdx < KMaxBuf, Kern::PanicCurrentThread(KClientPanicCat, __LINE__));
	__ASSERT_DEBUG(iBufs[aIdx].iPtr != NULL, Kern::PanicCurrentThread(KClientPanicCat, __LINE__));
	return iBufs[aIdx].iSize;
	}


TInt TBufferMgr::Alloc(TInt aIdx, TInt aSize)
	{
 	__ASSERT_DEBUG(0 <= aIdx && aIdx < KMaxBuf, Kern::PanicCurrentThread(KClientPanicCat, __LINE__));
	__ASSERT_DEBUG(iBufs[aIdx].iPtr == NULL, Kern::PanicCurrentThread(KClientPanicCat, __LINE__));
	NKern::ThreadEnterCS();
	iBufs[aIdx].iPtr = new TUint8[aSize];
	NKern::ThreadLeaveCS();
	iBufs[aIdx].iSize = aSize;
	return iBufs[aIdx].iPtr ? KErrNone : KErrNoMemory;
	}


void TBufferMgr::FreeAll()
	{
	NKern::ThreadEnterCS();
	for (TInt i=0; i<KMaxBuf; ++i)
		{
		delete iBufs[i].iPtr;
		iBufs[i].iPtr = NULL;
		}
	NKern::ThreadLeaveCS();
	}

#else

TUint8* TBufferMgr::Addr(TInt aIdx) const
	{
 	__ASSERT_DEBUG(0 <= aIdx && aIdx < KMaxBuf, Kern::PanicCurrentThread(KClientPanicCat, __LINE__));
	__ASSERT_DEBUG(iBufs[aIdx].iChunk != NULL, Kern::PanicCurrentThread(KClientPanicCat, __LINE__));
	return (TUint8*)iBufs[aIdx].iChunk->LinearAddress();
	}


TPhysAddr TBufferMgr::PhysAddr(TInt aIdx) const
	{
 	__ASSERT_DEBUG(0 <= aIdx && aIdx < KMaxBuf, Kern::PanicCurrentThread(KClientPanicCat, __LINE__));
	__ASSERT_DEBUG(iBufs[aIdx].iChunk != NULL, Kern::PanicCurrentThread(KClientPanicCat, __LINE__));
	return iBufs[aIdx].iChunk->PhysicalAddress();
	}


TInt TBufferMgr::Size(TInt aIdx) const
	{
 	__ASSERT_DEBUG(0 <= aIdx && aIdx < KMaxBuf, Kern::PanicCurrentThread(KClientPanicCat, __LINE__));
	__ASSERT_DEBUG(iBufs[aIdx].iChunk != NULL, Kern::PanicCurrentThread(KClientPanicCat, __LINE__));
	return iBufs[aIdx].iSize;
	}


TInt TBufferMgr::Alloc(TInt aIdx, TInt aSize)
	{
 	__ASSERT_DEBUG(0 <= aIdx && aIdx < KMaxBuf, Kern::PanicCurrentThread(KClientPanicCat, __LINE__));
	__ASSERT_DEBUG(iBufs[aIdx].iChunk == NULL, Kern::PanicCurrentThread(KClientPanicCat, __LINE__));
	NKern::ThreadEnterCS();
	TPhysAddr phys;
	TInt r = Epoc::AllocPhysicalRam(aSize, phys);
	if (r == KErrNone)
		{
		r = DPlatChunkHw::New(iBufs[aIdx].iChunk, phys, aSize, EMapAttrSupRw | EMapAttrFullyBlocking);
		if (r != KErrNone)
			Epoc::FreePhysicalRam(phys, aSize);
		iBufs[aIdx].iSize = aSize;
		
		__KTRACE_OPT(KDMA, Kern::Printf("TBufferMgr::Alloc buffer %d linAddr=0x%08x, physAddr=0x%08x, size=%d",
					aIdx, Addr(aIdx), PhysAddr(aIdx), Size(aIdx)));
		}
	NKern::ThreadLeaveCS();
	return r;
	}


void TBufferMgr::FreeAll()
	{
	for (TInt i=0; i<KMaxBuf; ++i)
		{
		if (iBufs[i].iChunk)
			{
			TPhysAddr base = iBufs[i].iChunk->PhysicalAddress();
			TInt size = iBufs[i].iSize;
			__ASSERT_DEBUG(iBufs[i].iChunk->AccessCount() == 1,
						   Kern::PanicCurrentThread(KClientPanicCat, __LINE__));
			NKern::ThreadEnterCS();
			iBufs[i].iChunk->Close(NULL);
			iBufs[i].iChunk = NULL;
			Epoc::FreePhysicalRam(base, size);
			NKern::ThreadLeaveCS();
			}
		}
	}

#endif


#ifndef DMA_APIV2
static TInt FragmentCount(DDmaRequest* aRequest)
	{
	TInt count = 0;
	for (SDmaDesHdr* pH = aRequest->iFirstHdr; pH != NULL; pH = pH->iNext)
		count++;
	return count;
	}
#endif


//////////////////////////////////////////////////////////////////////////////

class DDmaTestChannel : public DLogicalChannelBase
	{
public:
	virtual ~DDmaTestChannel();
protected:
	// from DLogicalChannelBase
	virtual TInt DoCreate(TInt aUnit, const TDesC8* anInfo, const TVersion& aVer);
	virtual TInt Request(TInt aFunction, TAny* a1, TAny* a2);
	virtual TInt RequestUserHandle(DThread* aThread, TOwnerType aType);
private:
	TInt Execute(const TDesC8& aDes);
	static void Dfc(DDmaRequest::TResult aResult, TAny* aArg);
	TInt DoGetInfo(TAny* aInfo);
	void FreeDmaRequests();
	void FreeClientRequests();
private:
	TUint32 iCookie;
	TBufferMgr iBufMgr;
	TDmaChannel* iChannel;
	enum { KMaxRequests = 8 };
	DDmaRequest* iRequests[KMaxRequests];
	TClientRequest* iClientRequests[KMaxRequests];
	DDmaTestChannel* iMap[KMaxRequests];
	TBool iCloseInCb[KMaxRequests];
	TUint32 iMemMemPslInfo;
	DThread* iClient;
	TDynamicDfcQue* iDfcQ;
	};


TInt DDmaTestChannel::RequestUserHandle(DThread* aThread, TOwnerType aType)
	{
	if (aType!=EOwnerThread || aThread!=iClient)
		return KErrAccessDenied;
	return KErrNone;
	}

TInt DDmaTestChannel::DoGetInfo(TAny* aInfo)
	{
	RTestDma::TInfo uinfo;
	const TDmaTestInfo& kinfo = DmaTestInfo();
	uinfo.iMaxTransferSize = kinfo.iMaxTransferSize;
	uinfo.iMemAlignMask = kinfo.iMemAlignMask;
	uinfo.iMaxSbChannels = kinfo.iMaxSbChannels;
	memcpy(&(uinfo.iSbChannels), kinfo.iSbChannels, 4 * kinfo.iMaxSbChannels);
	uinfo.iMaxDbChannels = kinfo.iMaxDbChannels;
	memcpy(&(uinfo.iDbChannels), kinfo.iDbChannels, 4 * kinfo.iMaxDbChannels);
	uinfo.iMaxSgChannels = kinfo.iMaxSgChannels;
	memcpy(&(uinfo.iSgChannels), kinfo.iSgChannels, 4 * kinfo.iMaxSgChannels);

	XTRAPD(r, XT_DEFAULT, kumemput(aInfo, &uinfo, sizeof(RTestDma::TInfo)));
	return r == KErrNone ? KErrDied : KErrGeneral;
	}


// called in thread critical section
TInt DDmaTestChannel::DoCreate(TInt /*aUnit*/, const TDesC8* aInfo, const TVersion& /*aVer*/)
	{
	TPckgBuf<RTestDma::TOpenInfo> infoBuf;

	TInt r=Kern::ThreadDesRead(&Kern::CurrentThread(), aInfo, infoBuf, 0, KChunkShiftBy0);
	if (r != KErrNone)
		return r;

	if (infoBuf().iWhat == RTestDma::TOpenInfo::EGetInfo)
		return DoGetInfo(infoBuf().U.iInfo);
	else
		{
		if (!iDfcQ)
 			{
 			r = Kern::DynamicDfcQCreate(iDfcQ, KDFCThreadPriority, KDFCThreadName);
			if (r != KErrNone)
 				return r;
#ifdef CPU_AFFINITY_ANY
			NKern::ThreadSetCpuAffinity((NThread*)(iDfcQ->iThread), KCpuAffinityAny);			
#endif
 			}	

		iMemMemPslInfo = DmaTestInfo().iMemMemPslInfo;
		iCookie = infoBuf().U.iOpen.iId;
		TDmaChannel::SCreateInfo info;
		info.iCookie = iCookie;
		info.iDfcQ = iDfcQ;
		info.iDfcPriority = 3;
		info.iDesCount = infoBuf().U.iOpen.iDesCount;
		r = TDmaChannel::Open(info, iChannel);
		if (r!= KErrNone)
			return r;

		// ---> Code coverage of rarely called functions
		const TDmac* const c = iChannel->Controller();
		if (!c)
			return KErrGeneral;
		const TInt mts = iChannel->MaxTransferSize(0, iCookie);
		if (mts == 0)
			return KErrGeneral;
		const TUint mam = iChannel->MemAlignMask(0, iCookie);
		if (~mam == 0)
			return KErrGeneral;
		// <--- Code coverage of rarely called functions

		iClient = &Kern::CurrentThread();
		for (TInt i=0; i<KMaxRequests; ++i)
			{
			r = Kern::CreateClientRequest(iClientRequests[i]);
			if (r!=KErrNone)
				return r;
			iMap[i] = this;
			TInt max = infoBuf().U.iOpen.iMaxTransferSize;
			if (max)
				{
				// Exercise request with custom limit
				iRequests[i] = new DDmaRequest(*iChannel, Dfc, iMap+i, max);
				}
			else
				{
				// Exercise request with default limit
				iRequests[i] = new DDmaRequest(*iChannel, Dfc, iMap+i);
				}
			if (! iRequests[i])
				return KErrNoMemory;

			iCloseInCb[i] = EFalse;
			}
		return KErrNone;
		}
	}


DDmaTestChannel::~DDmaTestChannel()
	{
	if (iChannel)
		{
		iChannel->CancelAll();
		FreeDmaRequests();
		iChannel->Close();
		}
	FreeClientRequests();
	if (iDfcQ)
		{
		iDfcQ->Destroy();
		}
	iBufMgr.FreeAll();
	}

void DDmaTestChannel::FreeDmaRequests()
	{
	for (TInt i=0; i<KMaxRequests; ++i)
		{
		delete iRequests[i];
		iRequests[i] = NULL;
		}
	}

void DDmaTestChannel::FreeClientRequests()
	{
	for (TInt i=0; i<KMaxRequests; ++i)
		{
		Kern::DestroyClientRequest(iClientRequests[i]);
		}
	}


TInt DDmaTestChannel::Request(TInt aFunction, TAny* a1, TAny* a2)
	{
	switch (aFunction)
		{
	case RTestDma::EAllocBuffer:
		return iBufMgr.Alloc((TInt)a1, (TInt)a2);
	case RTestDma::EFreeAllBuffers:
		iBufMgr.FreeAll();
		return KErrNone;
	case RTestDma::EFillBuffer:
		{
		TInt i = (TInt)a1;
		TUint8 val = (TUint8)(TUint)a2;
		memset(iBufMgr.Addr(i), val, iBufMgr.Size(i));
		return KErrNone;
		}
	case RTestDma::ECheckBuffer:
		{
		TInt i = (TInt)a1;
		TUint8 val = (TUint8)(TUint)a2;
		TUint8* p = iBufMgr.Addr(i);
		TUint8* end = p + iBufMgr.Size(i);
		while (p < end)
			if (*p++ != val)
				{
#ifdef _DEBUG
				const TUint8 prevValue = *(p-1);
#endif
				__KTRACE_OPT(KDMA, Kern::Printf("Check DMA buffer number %d failed at offset: %d value: %d(%c)",
												i, p-iBufMgr.Addr(i)-1, prevValue, prevValue));
				return EFalse;
				}
		return ETrue;
		}
	case RTestDma::EFragment:
		{
		RTestDma::TFragmentInfo info;
		kumemget(&info, a1, sizeof info);
		__ASSERT_DEBUG(iBufMgr.Size(info.iSrcBufIdx) == iBufMgr.Size(info.iDestBufIdx),
					   Kern::PanicCurrentThread(KClientPanicCat, __LINE__));
		__ASSERT_DEBUG(info.iSize <= iBufMgr.Size(info.iSrcBufIdx),
					   Kern::PanicCurrentThread(KClientPanicCat, __LINE__));
		__ASSERT_DEBUG(0 <= info.iRequestIdx && info.iRequestIdx < KMaxRequests,
					   Kern::PanicCurrentThread(KClientPanicCat, __LINE__));
#ifdef __DMASIM__
		// DMASIM doesn't use physical addresses
  		TUint32 src = (TUint32)iBufMgr.Addr(info.iSrcBufIdx);
  		TUint32 dest = (TUint32)iBufMgr.Addr(info.iDestBufIdx);
  		TUint KFlags = KDmaMemSrc | KDmaIncSrc | KDmaMemDest | KDmaIncDest;
#else
		TUint32 src = iBufMgr.PhysAddr(info.iSrcBufIdx);
		TUint32 dest = iBufMgr.PhysAddr(info.iDestBufIdx);
		TUint KFlags = KDmaMemSrc | KDmaIncSrc | KDmaPhysAddrSrc |
			KDmaMemDest | KDmaIncDest | KDmaPhysAddrDest | KDmaAltTransferLen;
#endif
		TInt r = iRequests[info.iRequestIdx]->Fragment(src, dest, info.iSize, KFlags, iMemMemPslInfo);
		if (r == KErrNone && info.iRs)
			r = iClientRequests[info.iRequestIdx]->SetStatus(info.iRs);
		return r;
		}
	case RTestDma::EExecute:
		return Execute(*(TDesC8*)a1);
	case RTestDma::EFailNext:
		return iChannel->FailNext((TInt)a1);
	case RTestDma::EFragmentCount:
		{
		TInt reqIdx = (TInt)a1;
		__ASSERT_DEBUG(0 <= reqIdx && reqIdx < KMaxRequests, Kern::PanicCurrentThread(KClientPanicCat, __LINE__));
#ifdef DMA_APIV2
		return iRequests[reqIdx]->FragmentCount();
#else
		return FragmentCount(iRequests[reqIdx]);
#endif
		}
	case RTestDma::EMissInterrupts:
		return iChannel->MissNextInterrupts((TInt)a1);
	default:
		Kern::PanicCurrentThread(KClientPanicCat, __LINE__);
		return KErrNone; // work-around spurious warning
		}
	}


TInt DDmaTestChannel::Execute(const TDesC8& aDes)
	{
	TBuf8<64> cmd;
	Kern::KUDesGet(cmd, aDes);
	__KTRACE_OPT(KDMA, Kern::Printf("DDmaTestChannel::Execute cmd=%S", &cmd));

	const TText8* p = cmd.Ptr();
	const TText8* pEnd = p + cmd.Length();
	while (p<pEnd)
		{
		TText8 opcode = *p++;
		switch (opcode)
			{
		case 'Q':
			{
			__ASSERT_DEBUG(p < pEnd, Kern::PanicCurrentThread(KClientPanicCat, __LINE__));
			TInt nextChar = *p++;
			TInt channel = -1;

			if(nextChar == 'X')
				{
				// Channel should be closed in callback
				__ASSERT_DEBUG(p < pEnd, Kern::PanicCurrentThread(KClientPanicCat, __LINE__));
				channel = *p++ - '0';
				iCloseInCb[channel] = ETrue;
				}
			else
				{
				channel = nextChar - '0';
				}
			__ASSERT_DEBUG(0 <= channel && channel < KMaxRequests, Kern::PanicCurrentThread(KClientPanicCat, __LINE__));
			iRequests[channel]->Queue();
			break;
			}
		case 'C':
			{
			iChannel->CancelAll();
			for(TInt i=0; i<KMaxRequests; ++i)
				{
				if(iClientRequests[i]->IsReady())
					iClientRequests[i]->Reset();
				}
			break;
			}
		default:
			Kern::PanicCurrentThread(KClientPanicCat, __LINE__);
			}
		}
	return KErrNone;
	}


void DDmaTestChannel::Dfc(DDmaRequest::TResult aResult, TAny* aArg)
	{
	DDmaTestChannel** ppC = (DDmaTestChannel**)aArg;
	DDmaTestChannel* pC = *ppC;
	TInt i = ppC - pC->iMap;
	TClientRequest* req = pC->iClientRequests[i];
	TInt r = (aResult==DDmaRequest::EOk) ? KErrNone : KErrGeneral;

	if(pC->iCloseInCb[i])
		{
		pC->iCloseInCb[i] = EFalse;
		__KTRACE_OPT(KDMA, Kern::Printf("Close channel in callback"));

		pC->FreeDmaRequests();
		pC->iChannel->Close();
		pC->iChannel = NULL;
		}

	if (req->IsReady())
		Kern::QueueRequestComplete(pC->iClient, req, r);
	}

//////////////////////////////////////////////////////////////////////////////

class DDmaTestFactory : public DLogicalDevice
	{
public:
	DDmaTestFactory();
	// from DLogicalDevice
	virtual TInt Install();
	virtual void GetCaps(TDes8& aDes) const;
	virtual TInt Create(DLogicalChannelBase*& aChannel);
	};


DDmaTestFactory::DDmaTestFactory()
    {
    iVersion = TestDmaLddVersion();
    iParseMask = KDeviceAllowUnit;							// no info, no PDD
    // iUnitsMask = 0;										// Only one thing
    }


TInt DDmaTestFactory::Create(DLogicalChannelBase*& aChannel)
    {
	aChannel=new DDmaTestChannel;
	return aChannel ? KErrNone : KErrNoMemory;
    }


TInt DDmaTestFactory::Install()
    {
    return SetName(&KTestDmaLddName);
    }


void DDmaTestFactory::GetCaps(TDes8& /*aDes*/) const
    {
    }

//////////////////////////////////////////////////////////////////////////////

DECLARE_STANDARD_LDD()
	{
    return new DDmaTestFactory;
	}
