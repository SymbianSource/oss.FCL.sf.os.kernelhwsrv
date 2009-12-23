// Copyright (c) 1997-2009 Nokia Corporation and/or its subsidiary(-ies).
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
// e32test\misc\d_ipccpy.cpp
// LDD for testing IPC copy functions
// 
//

#include "platform.h"
#include <kernel/kern_priv.h>
#include "d_ipccpy.h"

const TInt KMajorVersionNumber=0;
const TInt KMinorVersionNumber=1;
const TInt KBuildVersionNumber=1;

const TInt KBigBufferSize = 65536;

_LIT(KDIpcCpyPanicCategory,"DIpcCpy");

class DIpcCpyFactory : public DLogicalDevice
//
// IPC copy LDD factory
//
	{
public:
	DIpcCpyFactory();
	~DIpcCpyFactory();
	virtual TInt Install();						//overriding pure virtual
	virtual void GetCaps(TDes8& aDes) const;	//overriding pure virtual
	virtual TInt Create(DLogicalChannelBase*& aChannel);	//overriding pure virtual

private:
	TDynamicDfcQue* iDfcQ;
	};

class DIpcCpy : public DLogicalChannel
//
// Millisecond timer LDD channel
//
	{
public:
	DIpcCpy(TDfcQue* aDfcQ);
	virtual ~DIpcCpy();
protected:
	virtual TInt DoCreate(TInt aUnit, const TDesC8* anInfo, const TVersion& aVer);
	virtual TInt Request(TInt aFunc, TAny* a1, TAny* a2);
	virtual void HandleMsg(TMessageBase* aMsg);
public:
	void TimerExpired();
	TInt CreateHardwareChunks(TPtr8& aUserDes);

	// Panic reasons
	enum TPanic
		{
		ERequestAlreadyPending = 1
		};
public:
	DThread* iThread;
	TClientRequest* iAsyncRequest;
	TAny* iDest;
	TInt iSeqNum;
	NTimer iTimer;
	TDfc iDfc;
	TUint8 iBuffer[260];
	TUint8* iBigBuffer;
#ifdef __EPOC32__
	DPlatChunkHw* iHwChunks[RIpcCpy::ENumHwChunkTypes];
#endif
	TLinAddr iHwChunkLinAddrs[RIpcCpy::ENumHwChunkTypes];
	};

DECLARE_STANDARD_LDD()
	{
    return new DIpcCpyFactory;
    }

DIpcCpyFactory::DIpcCpyFactory()
//
// Constructor
//
    {
    iVersion=TVersion(KMajorVersionNumber,KMinorVersionNumber,KBuildVersionNumber);
    //iParseMask=0;//No units, no info, no PDD
    //iUnitsMask=0;//Only one thing
    }

DIpcCpyFactory::~DIpcCpyFactory()
//
// Destructor
//
	{
	if (iDfcQ)
		iDfcQ->Destroy();
	}

TInt DIpcCpyFactory::Create(DLogicalChannelBase*& aChannel)
//
// Create a new DIpcCpy on this logical device
//
    {
	aChannel=new DIpcCpy(iDfcQ);
    return aChannel?KErrNone:KErrNoMemory;
    }

const TInt KIpcCpyThreadPriority = 27;
_LIT(KIpcCpyThread,"IpcCpyThread");

TInt DIpcCpyFactory::Install()
//
// Install the LDD - overriding pure virtual
//
    {
	// Allocate a kernel thread to run the DFC 
	TInt r = Kern::DynamicDfcQCreate(iDfcQ, KIpcCpyThreadPriority, KIpcCpyThread);

#ifdef CPU_AFFINITY_ANY
	NKern::ThreadSetCpuAffinity((NThread*)(iDfcQ->iThread), KCpuAffinityAny);			
#endif

	if (r != KErrNone)
		return r; 	

    return SetName(&KIpcCpyLddName);
    }

void DIpcCpyFactory::GetCaps(TDes8& aDes) const
//
// Get capabilities - overriding pure virtual
//
    {
    TCapsIpcCpyV01 b;
    b.iVersion=TVersion(KMajorVersionNumber,KMinorVersionNumber,KBuildVersionNumber);
    Kern::InfoCopy(aDes,(TUint8*)&b,sizeof(b));
    }

void timerExpired(TAny* aPtr)
	{
	DIpcCpy* p=(DIpcCpy*)aPtr;
	p->iDfc.Add();
	}

void dfcFn(TAny* aPtr)
	{
	DIpcCpy* p=(DIpcCpy*)aPtr;
	p->TimerExpired();
	}

DIpcCpy::DIpcCpy(TDfcQue* aDfcQ)
//
// Constructor
//
	:	iTimer(timerExpired,this),
		iDfc(dfcFn,this,aDfcQ,1)
    {
	iThread=&Kern::CurrentThread();
	iThread->Open();
//	iSeqNum=0;
//	iDest=NULL;
	SetDfcQ(aDfcQ);
    }

DIpcCpy::~DIpcCpy()
	{
	if (iAsyncRequest)
		{
		Kern::QueueRequestComplete(iThread, iAsyncRequest, KErrCancel);	// does nothing if request not pending
		Kern::DestroyClientRequest(iAsyncRequest);
		}
	Kern::Free(iBigBuffer);
	Kern::SafeClose((DObject*&)iThread, NULL);

#ifdef __EPOC32__
	for(TInt i=0; i<RIpcCpy::ENumHwChunkTypes; i++)
		Kern::SafeClose((DObject*&)iHwChunks[i], NULL);
#endif
	}

TInt DIpcCpy::DoCreate(TInt /*aUnit*/, const TDesC8* /*anInfo*/, const TVersion& aVer)
//
// Create channel
//
    {

    if (!Kern::QueryVersionSupported(TVersion(KMajorVersionNumber,KMinorVersionNumber,KBuildVersionNumber),aVer))
    	return KErrNotSupported;
	TInt r = Kern::CreateClientRequest(iAsyncRequest);
	if (r!=KErrNone)
		return r;
	iBigBuffer = (TUint8*)Kern::Alloc(KBigBufferSize);
	if (!iBigBuffer)
		return KErrNoMemory;
	iMsgQ.Receive();
	return KErrNone;
	}


TInt DIpcCpy::CreateHardwareChunks(TPtr8& aUserDes)
	{
#ifndef __EPOC32__
	(void)aUserDes;
	return KErrNone;
#else
	NKern::ThreadEnterCS();
	TInt r;
	TUint32 size=Kern::RoundToPageSize(1);
#ifdef __X86__
	const TUint attrs[] = { EMapAttrSupRw, EMapAttrUserRw, EMapAttrReadUser }; // X86 does support EMapAttrUserRo, so use EMapAttrReadUser
#else
	const TUint attrs[] = { EMapAttrSupRw, EMapAttrUserRw, EMapAttrUserRo };
#endif
	for(TInt i=0; i<RIpcCpy::ENumHwChunkTypes; i++)
		{
		TPhysAddr phys;
		r = Epoc::AllocPhysicalRam(size,phys);
		if(r!=KErrNone)
			{
			NKern::ThreadLeaveCS();
			return r;
			}

		TChunkCreateInfo info;
		info.iType = TChunkCreateInfo::ESharedKernelMultiple;
		info.iMaxSize = size;
		info.iMapAttr = 0;
		info.iOwnsMemory = EFalse;
		DChunk* chunk;
		TLinAddr base;
		TUint32 attr;
		r = Kern::ChunkCreate(info,chunk,base,attr);
		if(r==KErrNone)
			{
			r=Kern::ChunkCommitPhysical(chunk, 0, size, phys);
			if(r==KErrNone)
				{
				memcpy((TAny*)base,&aUserDes,sizeof(TPtr8));
				}
			Kern::ChunkClose(chunk);
			if(r==KErrNone)
				r = DPlatChunkHw::New(iHwChunks[i], phys, size, attrs[i]);
			}

		if(r==KErrNone)
			{
			iHwChunkLinAddrs[i] = iHwChunks[i]->LinearAddress();
			}
		else if (r==KErrNotSupported) //ARMv6K && ARMv7 do not support EMapAttrUserRo
			{
			iHwChunkLinAddrs[i] = 0;
			r = KErrNone;			
			}
		else
			{
			Epoc::FreePhysicalRam(phys,size);
			NKern::ThreadLeaveCS();
			return r;
			}

		}
	NKern::ThreadLeaveCS();
	return r;
#endif
	}


TInt DIpcCpy::Request(TInt aFunc, TAny* a1, TAny* a2)
	{
	if (aFunc == RIpcCpy::EControlBigRead)
		{
		TUint size = (TUint)a2;
		if (size > (TUint)KBigBufferSize)
			return KErrOverflow;
		kumemput(a1, iBigBuffer, size);
		return KErrNone;
		}
	else if (aFunc == RIpcCpy::EControlBigWrite)
		{
		TUint size = (TUint)a2;
		if (size > (TUint)KBigBufferSize)
			return KErrOverflow;
		kumemget(iBigBuffer, a1, size);
		return KErrNone;
		}
	else if (aFunc == RIpcCpy::EControlHardwareChunks)
		{
		TPtr8 des(0,0,0);
		kumemget(&des,a2,sizeof(TPtr8));
		TInt r=CreateHardwareChunks(des);
		if(r==KErrNone)
			kumemput(a1, iHwChunkLinAddrs, sizeof(iHwChunkLinAddrs));
		return r;
		}
	return DLogicalChannel::Request(aFunc, a1, a2);
	}

void DIpcCpy::HandleMsg(TMessageBase* aMsg)
	{
	TInt r=KErrNone;
	TThreadMessage& m=*(TThreadMessage*)aMsg;
	TInt id=m.iValue;
	if (id==(TInt)ECloseMsg)
		{
		iTimer.Cancel();
		iDfc.Cancel();
		m.Complete(KErrNone,EFalse);
		iMsgQ.CompleteAll(KErrServerTerminated);
		return;
		}
	else if (id<0)
		{
		TRequestStatus* pS=(TRequestStatus*)m.Ptr0();
		if (iAsyncRequest->SetStatus(pS) != KErrNone)
			Kern::ThreadKill(iThread,EExitPanic,ERequestAlreadyPending,KDIpcCpyPanicCategory);

		if (id==~RIpcCpy::ERequestIpcCpy)
			{
			iDest=m.Ptr1();
			iTimer.OneShot(1);
			}
		else
			{
			r=KErrNotSupported;
			}

		if(r!=KErrNone)
			{
			Kern::QueueRequestComplete(iThread, iAsyncRequest, r);
			r = KErrNone;
			}
		}
	else
		{
		r=KErrNotSupported;
		}

	m.Complete(r,ETrue);
	}

void DIpcCpy::TimerExpired()
	{
	TInt src_offset=iSeqNum&3;
	TInt dest_offset=(iSeqNum>>2)&3;
	TInt length=(iSeqNum>>4)+1;
	TInt i;
	for (i=src_offset; i<length+src_offset; ++i)
		iBuffer[i]=(TUint8)(i+1);
	TPtrC8 ptr(iBuffer+src_offset, length);
	TInt r=Kern::ThreadDesWrite(iThread, iDest, ptr, dest_offset, KChunkShiftBy0, NULL);
	if (r==KErrNone)
		{
		r=iSeqNum;
		if (++iSeqNum==4096)
			iSeqNum=0;
		}
	Kern::QueueRequestComplete(iThread, iAsyncRequest, r);
	}

