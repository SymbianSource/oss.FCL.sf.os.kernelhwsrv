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
// e32\memmodel\emul\win32\mprocess.cpp
// 
//

#include "memmodel.h"
#include <property.h>
#include <emulator.h>
#include <wchar.h>

#define iMState		iWaitLink.iSpare1

extern const char* JustInTime;
extern TBool DisableWSDWarning;
const TInt KMaxWsdDllsPerProcess = 256;

/********************************************
 * Process
 ********************************************/
DWin32Process::DWin32Process()
: iDllData(KMaxWsdDllsPerProcess, _FOFF(SProcessDllDataBlock,iCodeSeg))
	{
	// Set process JustInTime flag from the emulator property.  This is not set
	// for the process containing supervisor thread.
	if (JustInTime && !_stricmp(JustInTime, "none"))
			{
			iFlags &= !KProcessFlagJustInTime;
			}
	}

DWin32Process::~DWin32Process()
	{
	__KTRACE_OPT(KMMU,Kern::Printf("DWin32Process destruct"));
	Destruct();
	RemoveDllData();
	iDllData.Close();
	}

TInt DWin32Process::NewChunk(DChunk*& aChunk, SChunkCreateInfo& aInfo, TLinAddr& aRunAddr)
	{
	aChunk=NULL;
	DWin32Chunk* pC=new DWin32Chunk;
	if (!pC)
		return KErrNoMemory;
	pC->iChunkType=aInfo.iType;
	if (!aInfo.iGlobal && (iAttributes & DWin32Process::EPrivate)!=0)
		pC->iAttributes |= DWin32Chunk::EPrivate;
	pC->iOwningProcess=(aInfo.iGlobal)?NULL:this;
	TInt r=pC->Create(aInfo);
	if (r==KErrNone && (aInfo.iOperations & SChunkCreateInfo::EAdjust))
		{
		__ASSERT_ALWAYS(aInfo.iRunAddress==0,MM::Panic(MM::EInvalidChunkCreate));
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

TInt DWin32Process::DoCreate(TBool aKernelProcess, TProcessCreateInfo& /*aInfo*/)
	{
	__KTRACE_OPT(KPROC,Kern::Printf("DWin32Process::DoCreate %O",this))
	iAttributes=aKernelProcess ? ESupervisor|EPrivate : 0;
	// force iDllData to reserve KMaxWsdDllsPerProcess. The Append will
	// create the space, the Remove will not free it.
	SProcessDllDataBlock data = {0,0,0};
	TInt err = iDllData.Append(data);
	if(err==KErrNone) 
		{
			__ASSERT_ALWAYS(iDllData.Count()==1,MM::Panic(MM::EWsdBadReserve));
			iDllData.Remove(0);
		}
	return err;
	}

TInt DWin32Process::CreateDataBssStackArea(TProcessCreateInfo& /*aInfo*/)
//
// This is managed for us by win32
//
	{
	__KTRACE_OPT(KPROC,Kern::Printf("DWin32Process::CreateDataBssStackArea %O",this));
	return KErrNone;
	}

TInt DWin32Process::AddChunk(DChunk* /*aChunk*/,TBool /*isReadOnly*/)
	{
	return KErrNone;
	}


TInt DWin32Process::NewShPool(DShPool*& aPool, TShPoolCreateInfo& aInfo)
	{
	aPool = NULL;
	DWin32ShPool* pC = NULL;

	if (aInfo.iInfo.iFlags & TShPoolCreateInfo::EPageAlignedBuffer)
		{
		pC = new DWin32AlignedShPool();
		}
	else
		{
		pC = new DWin32NonAlignedShPool();
		}

	if (pC == NULL)
		{
		return KErrNoMemory;
		}

	TInt r = pC->Create(this, aInfo);

	if (r == KErrNone)
		{
		aPool = pC;
		}
	else
		{
		pC->Close(NULL);
		}

	return r;
	} // DWin32Process::NewShPool


void DWin32Process::Release()
	{
	CallRuntimeHook(EWin32RuntimeProcessDetach);
	DProcess::Release();
	}

void DWin32Process::FinalRelease()
	{
	}

TInt DWin32Process::MapCodeSeg(DCodeSeg* aSeg)
	{
	
	__KTRACE_OPT(KDLL,Kern::Printf("Process %O MapCodeSeg(%C)", this, aSeg));
	if (!aSeg || !aSeg->IsDll())
		return KErrNone;
	
	DWin32CodeSeg* seg = (DWin32CodeSeg*)aSeg;
	if (seg->iRealDataSize == 0 && seg->iRealBssSize == 0)
		return KErrNone;
	
	if (!DisableWSDWarning && this!=K::TheKernelProcess) 
        Kern::Printf("WARNING!!: WSD Lib Loaded- Process %O, has loaded Lib %C"
                     " which has %d bytes of WSD",this,aSeg,
                     seg->iRealDataSize + seg->iRealBssSize);
	
	// remove any existing copy of this code seg (should never happen)
	UnmapCodeSeg(aSeg);
	
	SProcessDllDataBlock data;
	data.iCodeSeg = seg;
	
	TInt count=0;
	TInt err = KErrNone;

	// Scheduling must be disabled while we are allocating memory from Windows' heap
	NKern::Lock();
	data.iDataCopy = GlobalAlloc(GMEM_FIXED, seg->iRealDataSize);
	data.iBssCopy = GlobalAlloc(GMEM_FIXED, seg->iRealBssSize);
	NKern::Unlock();
	if (!data.iDataCopy || !data.iBssCopy)
		{
		err = KErrNoMemory;
		goto failed;
		}

	memcpy(data.iDataCopy, seg->iDataCopy, seg->iRealDataSize);	// start with init data
	memclr(data.iBssCopy, seg->iRealBssSize);					// initialized to zeros

	NKern::Lock();
	count = iDllData.Count();
	if (count == KMaxWsdDllsPerProcess)
		err = KErrOverflow;
	if (!err)
		err = iDllData.InsertInUnsignedKeyOrder(data);
	NKern::Unlock();
	if (err)
		goto failed;
			
	return KErrNone;	

failed:
	// Scheduling must be disabled while we are freeing memory from Windows' heap
	NKern::Lock();
	GlobalFree(data.iDataCopy);
	GlobalFree(data.iBssCopy);
	NKern::Unlock();

	return err;
	}

void DWin32Process::UnmapCodeSeg(DCodeSeg* aSeg)
	{
	if (!aSeg || !aSeg->IsDll())
		return;
	
	DWin32CodeSeg* seg = (DWin32CodeSeg*)aSeg;
	if (seg->iRealDataSize == 0 && seg->iRealBssSize == 0)
		return;
		
	SProcessDllDataBlock data;
	data.iCodeSeg = seg;
	NKern::Lock();
	if (seg->iLiveProcess == this)
		seg->iLiveProcess = NULL;
	TInt ix = iDllData.FindInUnsignedKeyOrder(data);
	if (ix >= 0)
		{
		data = iDllData[ix];
		iDllData.Remove(ix);
		}
	NKern::Unlock();
	
	if (ix < 0)
		return;

	// Scheduling must be disabled while we are freeing memory from Windows' heap
	NKern::Lock();
	GlobalFree(data.iDataCopy);
	GlobalFree(data.iBssCopy);
	NKern::Unlock();

	__KTRACE_OPT(KDLL,Kern::Printf("Process %O UnmapCodeSeg(%C)", this, aSeg));
	}

void DWin32Process::RemoveDllData()
	{
	// unmap all DLL data with kernel locked
	TInt count = iDllData.Count();
	for (TInt ii=count-1; ii>=0; ii--)
		{
		SProcessDllDataBlock data = iDllData[ii];
		NKern::Lock();
		if (data.iCodeSeg->iLiveProcess == this)
			data.iCodeSeg->iLiveProcess = NULL;
		iDllData.Remove(ii);
		GlobalFree(data.iDataCopy);
		GlobalFree(data.iBssCopy);
		NKern::Unlock();
		}
	}

TInt DWin32Process::AttachExistingCodeSeg(TProcessCreateInfo& /*aInfo*/)
	{
	return KErrNotSupported;	// never allowed
	}

void DWin32Process::CallRuntimeHook(TWin32RuntimeReason aReason)
	{
	if (iWin32RuntimeHook)
		{
		SchedulerLock();
		TBool ok = iWin32RuntimeHook(aReason);
		SchedulerUnlock();
		if (!ok && aReason != EWin32RuntimeProcessDetach)
			Kern::PanicCurrentThread(_L("MemModel"), MM::EWin32RuntimeError);
		}
	}


void DThread::IpcExcHandler(TExcTrap* aTrap, DThread* aThread, TAny* aContext)
	{
	aThread->iIpcClient = 0;
	TIpcExcTrap& xt=*(TIpcExcTrap*)aTrap;
	TWin32ExcInfo& info=*(TWin32ExcInfo*)aContext;
	TLinAddr va=(TLinAddr)info.iExcDataAddress;
	if (va>=xt.iRemoteBase && (va-xt.iRemoteBase)<xt.iSize)
		xt.Exception(KErrBadDescriptor);	// problem accessing remote address - 'leave' so an error code will be returned
	if (xt.iLocalBase && va>=xt.iLocalBase && (va-xt.iLocalBase)<xt.iSize)
		NKern::UnlockSystem();		// problem accessing local address - return and panic current thread as usual
	// otherwise return and fault kernel
	}

TInt DThread::RawRead(const TAny* aSrc, TAny* aDest, TInt aLength, TInt /*aFlags*/, TIpcExcTrap* /*aExcTrap*/)
//
// Read from the thread's process.
// aSrc is run address of memory to read
// aDest is current address of destination
// Enter and leave with system locked
//
	{
	if (iMState==EDead)
		return KErrDied;
	const TUint8* pS=(const TUint8*)aSrc;
	TUint8* pD=(TUint8*)aDest;
	TBool kernelLocked = EFalse;
	const TUint8* pC=(const TUint8*)MM::CurrentAddress(this,pS,aLength,EFalse,kernelLocked);
	if (kernelLocked)
		{
		// kernel locked because of DLL WSD IPC, do it all in one big block
		TInt r = KErrNone;
		__KTRACE_OPT(KTHREAD2,Kern::Printf("DThread::RawRead %08x",pC));
		if (!pC)
			r = KErrBadDescriptor;
		else
			memcpy(pD,pC,aLength);
		NKern::Unlock();
		return r;
		}
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

TInt DThread::RawWrite(const TAny* aDest, const TAny* aSrc, TInt aLength, TInt /*aFlags*/, DThread* /*anOriginatingThread*/, TIpcExcTrap* /*aExcTrap*/)
//
// Write to the thread's process.
// aDest is run address of memory to write
// aSrc is current address of destination
// anOriginatingThread is the thread on behalf of which this operation is performed (eg client of device driver).
// Enter and leave with system locked
//
	{
	if (iMState==EDead)
		return KErrDied;
	TUint8* pD=(TUint8*)aDest;
	const TUint8* pS=(const TUint8*)aSrc;
	TBool kernelLocked = EFalse;
	TUint8* pC=(TUint8*)MM::CurrentAddress(this,pD,aLength,ETrue,kernelLocked);
	if (kernelLocked)
		{
		// kernel locked because of DLL WSD IPC, do it all in one big block
		TInt r = KErrNone;
		__KTRACE_OPT(KTHREAD2,Kern::Printf("DThread::RawWrite %08x",pC));
		if (!pC)
			r = KErrBadDescriptor;
		else
			memcpy(pC,pS,aLength);
		NKern::Unlock();
		return r;
		}
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

TInt DThread::ReadAndParseDesHeader(const TAny* aSrc, TDesHeader& aDest)
//
// Read and parse the header of a remote descriptor.
// Enter and leave with system locked.
//
	{
	__ASSERT_SYSTEM_LOCK;
	static const TUint8 LengthLookup[16]={4,8,12,8,12,0,0,0,0,0,0,0,0,0,0,0};
	if (iMState == EDead)
		return KErrDied;
	TBool kernelLocked = EFalse;
	const TUint32* pS=(const TUint32*)MM::CurrentAddress(this,aSrc,sizeof(TDesC8),EFalse,kernelLocked);
	if (!pS || (TInt(pS)&3)!=0)
		{
		if (kernelLocked)
			NKern::Unlock();
		return KErrBadDescriptor;
		}
	TInt type=0;
	XTRAPD(r, XT_DEFAULT,					\
		type=*pS>>KShiftDesType8;			\
		TInt l=LengthLookup[type];			\
		if (l==0)							\
			r=KErrBadDescriptor;			\
		else								\
			wordmove(&aDest,pS,l);			\
		);
	if (kernelLocked)
		NKern::Unlock();
	if (r!=KErrNone)
		return r;
	return K::ParseDesHeader(aSrc, (TRawDesHeader&)aDest, aDest);
	}

DChunk* DThread::OpenSharedChunk(const TAny* aAddress, TBool /*aWrite*/, TInt& aOffset)
	{
	DWin32Chunk* chunk=0;
	DObjectCon& chunks=*K::Containers[EChunk];
	chunks.Wait();
	TInt count=chunks.Count();
	TInt i;
	TUint offset=0;
	for(i=0;i<count;i++)
		{
		DWin32Chunk* pC=(DWin32Chunk*)chunks[i];
		offset = (TUint)aAddress-(TUint)pC->Base();
		if(offset<TUint(pC->iMaxSize))
			{
			chunk = pC;
			break;
			}
		}
	chunks.Signal();

	if(!chunk)
		return 0;

	if((chunk->iChunkType!=ESharedKernelSingle && chunk->iChunkType!=ESharedKernelMultiple))
		return 0;
	if(chunk->Open()!=KErrNone)
		return 0;
	aOffset = offset;
	return chunk;
	}

TInt DThread::PrepareMemoryForDMA(const TAny* /*aLinAddr*/, TInt /*aSize*/, TPhysAddr* /*aPhysicalPageList*/)
	{
	return KErrNotSupported;
	}

TInt DThread::ReleaseMemoryFromDMA(const TAny* /*aLinAddr*/, TInt /*aSize*/, TPhysAddr* /*aPhysicalPageList*/)
	{
	return KErrNotSupported;
	}

