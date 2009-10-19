// Copyright (c) 1994-2009 Nokia Corporation and/or its subsidiary(-ies).
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
// e32\memmodel\epoc\direct\mprocess.cpp
// 
//

#include <memmodel.h>
#include <kernel/cache.h>

#define iMState		iWaitLink.iSpare1

_LIT(KDollarDat,"$DAT");

/********************************************
 * Process
 ********************************************/
void DMemModelProcess::Destruct()
	{
	DProcess::Destruct();
	}

TInt DMemModelProcess::NewChunk(DChunk*& aChunk, SChunkCreateInfo& aInfo, TLinAddr& aRunAddr)
	{
	aChunk=NULL;
	DMemModelChunk* pC=new DMemModelChunk;
	if (!pC)
		return KErrNoMemory;
	pC->iChunkType=aInfo.iType;
	if (!aInfo.iGlobal && (iAttributes & DMemModelProcess::EPrivate)!=0)
		pC->iAttributes |= DMemModelChunk::EPrivate;
	pC->iOwningProcess=(aInfo.iGlobal)?NULL:this;
	TInt r=pC->Create(aInfo);
	if (r==KErrNone && (aInfo.iOperations & SChunkCreateInfo::EAdjust))
		{
		if (aInfo.iRunAddress!=0)
			pC->SetFixedAddress(aInfo.iRunAddress,aInfo.iPreallocated);
		if (aInfo.iPreallocated==0)
			{
			if (pC->iAttributes & DChunk::EDisconnected)
				{
				r=pC->Commit(aInfo.iInitialBottom,aInfo.iInitialTop-aInfo.iInitialBottom);
				}
			else if (pC->iAttributes & DChunk::EDoubleEnded)
				{
				r=pC->AdjustDoubleEnded(aInfo.iInitialBottom,aInfo.iInitialTop);
				}
			else
				{
				r=pC->Adjust(aInfo.iInitialTop);
				}
			}
		if (r==KErrNone)
			{
			aRunAddr=(TLinAddr)pC->Base();
			}
		}
	if (r==KErrNone)
		{
		pC->iDestroyedDfc = aInfo.iDestroyedDfc;
		aChunk=(DChunk*)pC;
		}
	else
		pC->Close(NULL);	// NULL since chunk can't have been added to process
	return r;
	}

TInt DMemModelProcess::DoCreate(TBool aKernelProcess, TProcessCreateInfo&)
	{
	__KTRACE_OPT(KPROC,Kern::Printf("DMemModelProcess::DoCreate %O",this))
	if (aKernelProcess)
		iAttributes=ESupervisor|EPrivate;
	else
		iAttributes=0;
	return KErrNone;
	}

TInt DMemModelProcess::CreateDataBssStackArea(TProcessCreateInfo& aInfo)
	{
	__KTRACE_OPT(KPROC,Kern::Printf("DMemModelProcess::CreateDataBssStackArea %O",this));
	TInt dataBssSize=MM::RoundToBlockSize(aInfo.iTotalDataSize);
	TInt r=KErrNone;
	DMemModelCodeSeg* cs=(DMemModelCodeSeg*)iTempCodeSeg;
	if (!cs->iXIP && dataBssSize)
		{
		// only RAM loaded processes need data chunks
		// ROM processes have their .data/.bss areas reserved by ROMBUILD
		SChunkCreateInfo cinfo;
		cinfo.iGlobal=EFalse;
		cinfo.iAtt=TChunkCreate::ENormal;
		cinfo.iForceFixed=EFalse;
		cinfo.iOperations=SChunkCreateInfo::EAdjust|SChunkCreateInfo::EAdd;
		cinfo.iType=EUserData;
		cinfo.iMaxSize=dataBssSize;
		cinfo.iInitialBottom=0;
		cinfo.iInitialTop=dataBssSize;
		cinfo.iPreallocated=0;
		cinfo.iName.Set(KDollarDat);
		cinfo.iOwner=this;
		cinfo.iRunAddress=0;
		r=NewChunk((DChunk*&)iDataBssStackChunk, cinfo, iDataBssRunAddress);
		__KTRACE_OPT(KPROC,Kern::Printf("RAM process, ret=%d, data at %08x+%x",r,iDataBssRunAddress,dataBssSize));
		}
	else if (cs->iXIP)
		{
		iDataBssRunAddress=cs->RomInfo().iDataBssLinearBase;
		__KTRACE_OPT(KPROC,Kern::Printf("ROM process, data at %08x+%x",iDataBssRunAddress,dataBssSize));
		}
	return r;
	}

TInt DMemModelProcess::AddChunk(DChunk* aChunk,TBool isReadOnly)
	{
	return KErrNone;
	}

void DMemModelProcess::FinalRelease()
	{
	}

void DMemModelProcess::RemoveDllData()
//
// Call with CodeSegLock held
//
	{
	}

TInt DMemModelProcess::MapCodeSeg(DCodeSeg*)
	{
	return KErrNone;
	}

void DMemModelProcess::UnmapCodeSeg(DCodeSeg*)
	{
	}

TInt DMemModelProcess::NewShPool(DShPool*& /* aPool */, TShPoolCreateInfo& /* aInfo */)
	{
	return KErrNotSupported;
	}

TInt DThread::RawRead(const TAny* aSrc, TAny* aDest, TInt aLength, TInt aFlags, TIpcExcTrap* /*aExcTrap*/)
//
// Read from the thread's process.
// aSrc is run address of memory to read
// aDest is current address of destination
// Enter and leave with system locked
//
	{
	const TUint8* pS=(const TUint8*)aSrc;
	TUint8* pD=(TUint8*)aDest;
	TBool check=ETrue;
	while (aLength)
		{
		if (check)
			{
			if (iMState==EDead)
				return KErrDied;
			__KTRACE_OPT(KTHREAD2,Kern::Printf("DThread::Read %08x",pS));
			}
		TInt l=Min(aLength,K::MaxMemCopyInOneGo);
		memcpy(pD,pS,l);
		pD+=l;
		pS+=l;
		aLength-=l;
		if (aLength)
			check=NKern::FlashSystem();
		}
	return KErrNone;
	}

TInt DThread::RawWrite(const TAny* aDest, const TAny* aSrc, TInt aLength, TInt aFlags, DThread* anOriginatingThread, TIpcExcTrap* /*aExcTrap*/)
//
// Write to the thread's process.
// aDest is run address of memory to write
// aSrc is current address of destination
// anOriginatingThread is the thread on behalf of which this operation is performed (eg client of device driver).
// Enter and leave with system locked
//
	{
	TUint8* pD=(TUint8*)aDest;
	const TUint8* pS=(const TUint8*)aSrc;
	TBool check=ETrue;
	while (aLength)
		{
		if (check)
			{
			if (iMState==EDead)
				return KErrDied;
			__KTRACE_OPT(KTHREAD2,Kern::Printf("DThread::Write %08x",pD));
			}
		TInt l=Min(aLength,K::MaxMemCopyInOneGo);
		memcpy(pD,pS,l);
		pD+=l;
		pS+=l;
		aLength-=l;
		if (aLength)
			check=NKern::FlashSystem();
		}
	return KErrNone;
	}

#ifdef __DEBUGGER_SUPPORT__
TInt CodeModifier::SafeWriteCode(DProcess* /*aProcess*/, TLinAddr /*aAddress*/, TInt /*aSize*/, TUint /*aValue*/, void* /*aOldValue*/)
	{
	return KErrNotSupported;
	}
TInt CodeModifier::WriteCode(TLinAddr /*aAddress*/, TInt /*aSize*/, TUint /*aValue*/, void* /*aOldValue*/)
	{
	return KErrNotSupported;
	}
#endif //__DEBUGGER_SUPPORT__

TInt DThread::ReadAndParseDesHeader(const TAny* aSrc, TDesHeader& aDest)
//
// Read and parse the header of a remote descriptor.
// Enter and leave with system locked.
//
	{
	TRawDesHeader& header = (TRawDesHeader&)aDest;
	static const TUint8 LengthLookup[16]={4,8,12,8,12,0,0,0,0,0,0,0,0,0,0,0};
	const TUint32* pS=(const TUint32*)aSrc;
	if (!pS || (TInt(pS)&3)!=0)
		return KErrBadDescriptor;
	if (Kern::SafeRead(pS,&header,sizeof(TUint32)))
		return KErrBadDescriptor;
	TInt type=header[0]>>KShiftDesType8;
	TInt l=LengthLookup[type];
	if (l==0)
		return KErrBadDescriptor;
	if (l>(TInt)sizeof(TUint32) && Kern::SafeRead(pS+1,&header[1],l-sizeof(TUint32)))
		return KErrBadDescriptor;
	return K::ParseDesHeader(aSrc, header, aDest);
	}

DChunk* DThread::OpenSharedChunk(const TAny* aAddress, TBool /*aWrite*/, TInt& aOffset)
	{
	DMemModelChunk* chunk=0;
	DObjectCon& chunks=*K::Containers[EChunk];
	NKern::LockSystem();
	chunks.Lock()->Wait();
	TInt count=chunks.Count();
	TInt i;
	TUint offset=0;
	for(i=0;i<count;i++)
		{
		DMemModelChunk* pC=(DMemModelChunk*)chunks[i];
		offset = (TUint)aAddress-(TUint)pC->Base();
		if(offset<TUint(pC->iMaxSize))
			{
			chunk = pC;
			break;
			}
		}
	chunks.Lock()->Signal();

	if(!chunk)
		return 0;

	if((chunk->iChunkType!=ESharedKernelSingle && chunk->iChunkType!=ESharedKernelMultiple))
		return 0;
	if(chunk->Open()!=KErrNone)
		return 0;
	aOffset = offset;
	return chunk;
	}

TInt DThread::PrepareMemoryForDMA(const TAny* aLinAddr, TInt aSize, TPhysAddr* aPhysicalPageList)
	{
	return KErrNotSupported;
	}

TInt DThread::ReleaseMemoryFromDMA(const TAny* aLinAddr, TInt aSize, TPhysAddr* aPhysicalPageList)
	{
	return KErrNotSupported;
	}

