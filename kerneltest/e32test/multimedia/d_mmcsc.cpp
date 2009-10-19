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
// e32test\multimedia\d_mmcsc.cpp
// The interface to a device driver which creates a shared chunk - for use with the multimedia driver tests.
// This driver uses only generic kernel APIs and so need not be built from each variant.
// 
//

/**
 @file
 @internalComponent
 @test
*/

#include <kernel/kern_priv.h>
#include "d_mmcsc.h"

//#define __KTRACE_MMD(s) s;
#define __KTRACE_MMD(s)

class DMmCreateScFactory : public DLogicalDevice
	{
public:
	DMmCreateScFactory();
	virtual TInt Install();
	virtual void GetCaps(TDes8 &aDes) const;
	virtual TInt Create(DLogicalChannelBase*& aChannel);
	};

class DMmCreateSc : public DLogicalChannel
	{
public:
	DMmCreateSc();
	~DMmCreateSc();
protected:
	virtual TInt DoCreate(TInt aUnit, const TDesC8* anInfo, const TVersion& aVer);
	virtual void HandleMsg(TMessageBase* aMsg);
	TInt DoControl(TInt aId, TAny* a1, TAny* a2);
private:
	TInt CreateChunk(TInt aNumBuffers,TInt aBufferSize);
	void CloseChunk();
	TInt MakeChunkHandle(DThread* aThread);
private:
	DThread* iClient;
	TMmSharedChunkBufConfig iBufferConfig;
	DChunk* iChunk;
	};

DECLARE_STANDARD_LDD()
	{
	return new DMmCreateScFactory;
	}

DMmCreateScFactory::DMmCreateScFactory()
	{
	// Set version number for this device
	iVersion=TVersion(0,1,1);

	// Indicate we do support units or a PDD
	iParseMask=0;
	}

TInt DMmCreateScFactory::Install()
	{
	return(SetName(&KDevMmCScName));
	}

void DMmCreateScFactory::GetCaps(TDes8& aDes) const
	{
	// Create a capabilities object
	TCapsMmCScV01 caps;
	caps.iVersion=iVersion;

	// Write it back to user memory
	Kern::InfoCopy(aDes,(TUint8*)&caps,sizeof(caps));
	}

TInt DMmCreateScFactory::Create(DLogicalChannelBase*& aChannel)
	{
	aChannel=new DMmCreateSc;
	if (!aChannel)
		return(KErrNoMemory);
	return(KErrNone);
	}

DMmCreateSc::DMmCreateSc()
	{
//	iChunk=NULL;
	iClient=&Kern::CurrentThread();
	((DObject*)iClient)->Open();
	}

DMmCreateSc::~DMmCreateSc()
	{
	CloseChunk();
	Kern::SafeClose((DObject*&)iClient,NULL);
	}

TInt DMmCreateSc::DoCreate(TInt /*aUnit*/, const TDesC8* /*anInfo*/, const TVersion& /*aVer*/)
	{

	__KTRACE_MMD(Kern::Printf("DMmCreateSc:DoCreate"));

	SetDfcQ(Kern::DfcQue0());
	iMsgQ.Receive();

	TInt r;
	r=CreateChunk(KMmCScNumBuffers,KMmCScBufSize);	// Contains two 150K buffers
	return(r);
	}

void DMmCreateSc::HandleMsg(TMessageBase* aMsg)
	{

	TThreadMessage& m=*(TThreadMessage*)aMsg;
	TInt id=m.iValue;
	if (id==(TInt)ECloseMsg)
		{
		m.Complete(KErrNone, EFalse);
		return;
		}
	else if (id==KMaxTInt)
		{
		m.Complete(KErrNone,ETrue);	// DoCancel
		return;
		}

	if (id<0)
		{
		TRequestStatus* pS=(TRequestStatus*)m.Ptr0();	// DoRequest
		Kern::RequestComplete(iClient,pS,KErrNotSupported);
		m.Complete(KErrNone,ETrue);
		}
	else
		{
		TInt r=DoControl(id,m.Ptr0(),m.Ptr1());	// DoControl
		m.Complete(r,ETrue);
		}
	}

TInt DMmCreateSc::DoControl(TInt aId, TAny* a1, TAny* /*a2*/)
	{

	__KTRACE_MMD(Kern::Printf("DMmCreateSc:DoControl %x",aId));
	TInt r=KErrNone;

	switch (aId)
		{
		case RMmCreateSc::EGetChunkHandle:
			{
			r=MakeChunkHandle(iClient);
			break;
			}
		case RMmCreateSc::EGetBufInfo:
			{
			TPtrC8 ptr((const TUint8*)&iBufferConfig,sizeof(iBufferConfig));
			r=Kern::ThreadDesWrite(iClient,a1,ptr,0,KTruncateToMaxLength,NULL);
			break;
			}
		default:
			r=KErrNotSupported;
		}
	return(r);
	}

TInt DMmCreateSc::CreateChunk(TInt aNumBuffers,TInt aBufferSize)
	{

	TInt bufferSize=Kern::RoundToPageSize(aBufferSize);
	TInt chunkSz=(aNumBuffers*bufferSize);
	__KTRACE_MMD(Kern::Printf("DMmCreateSc:CreateChunk %d",chunkSz));

	// Thread must be in critical section to create chunks
	NKern::ThreadEnterCS();

	// Create the shared chunk.
	TChunkCreateInfo info;
	info.iType = TChunkCreateInfo::ESharedKernelMultiple;
	info.iMaxSize = chunkSz;
#if !defined(__WINS__)
	info.iMapAttr=EMapAttrFullyBlocking;	// No caching
#else
	info.iMapAttr=0;
#endif
	info.iOwnsMemory = ETrue; 				// Using RAM pages
	info.iDestroyedDfc = NULL;				// No chunk destroy DFC

	DChunk* chunk;
	TLinAddr chunkKernelAddr;
	TUint32 mapAttr;
	TInt r = Kern::ChunkCreate(info,chunk,chunkKernelAddr,mapAttr);
	if (r!=KErrNone)
		{
		NKern::ThreadLeaveCS();
		return(r);
		}

	// Map the three buffers into the chunk - each containing	physically contiguous RAM pages.
	TInt bufOffset=0;
	TUint32 pa;
	r=Kern::ChunkCommitContiguous(chunk,bufOffset,bufferSize,pa);
	if (r==KErrNone)
		{
		bufOffset+=bufferSize;
		r=Kern::ChunkCommitContiguous(chunk,bufOffset,bufferSize,pa);
		}
	if (r==KErrNone)
		{
		bufOffset+=bufferSize;
		r=Kern::ChunkCommitContiguous(chunk,bufOffset,bufferSize,pa);
		}
	if (r!=KErrNone)
		Kern::ChunkClose(chunk); // Commit failed - tidy-up.
	else
		iChunk=chunk;

	NKern::ThreadLeaveCS();

	// Update our buffer info. structure to reflect the chunk we have just created.
	iBufferConfig.iNumBuffers=aNumBuffers;
	iBufferConfig.iBufferSizeInBytes=aBufferSize;
	iBufferConfig.iFlags|=KScFlagBufOffsetListInUse;
	iBufferConfig.iSpec[0].iBufferOffset=0;
	iBufferConfig.iSpec[0].iBufferId=0;
	iBufferConfig.iSpec[1].iBufferOffset=bufferSize;
	iBufferConfig.iSpec[1].iBufferId=1;

	__KTRACE_MMD(Kern::Printf("ChunkCommit %d",r));
	return(r);
	}

void DMmCreateSc::CloseChunk()
	{

	__KTRACE_MMD(Kern::Printf("DMmCreateSc:CloseChunk"));

	// Thread must be in critical section to close a chunk
	NKern::ThreadEnterCS();

	// Close chunk
	if (iChunk)
		Kern::ChunkClose(iChunk);

	// Can leave critical section now
	NKern::ThreadLeaveCS();
	}

TInt DMmCreateSc::MakeChunkHandle(DThread* aThread)
	{

	TInt r;
	// Thread must be in critical section to make a handle
	NKern::ThreadEnterCS();
	if (iChunk)
		r=Kern::MakeHandleAndOpen(aThread,iChunk);
	else
		r=KErrNotFound;
	NKern::ThreadLeaveCS();
	__KTRACE_MMD(Kern::Printf("DMmCreateSc:MakeChunkHandle %x %d",aThread,r));
	return(r);
	}
