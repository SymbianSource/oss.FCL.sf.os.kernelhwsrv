// Copyright (c) 1995-2009 Nokia Corporation and/or its subsidiary(-ies).
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
// e32\kernel\scodeseg.cpp
// 
//

#include <kernel/kern_priv.h>
#include <e32uid.h>
#include "execs.h"

extern void InvalidExecHandler();

// Compare code segments by name
TInt CompareCodeSegsByName(const DCodeSeg& aA, const DCodeSeg& aB)
	{
	TLinAddr aa = (TLinAddr)&aA;
	const TDesC8* pA = (aa & 1) ? (const TDesC8*)(aa&~3) : &aA.iRootName;
	return pA->CompareF(aB.iRootName);
	}


//
// DCodeSeg::RCodeSegsByAddress
//

DCodeSeg::RCodeSegsByAddress::RCodeSegsByAddress(TInt aMinGrowBy, TInt aFactor)
	: iActive(0)
	{
	new (iArray+0) RPointerArray<DCodeSeg>(aMinGrowBy, aFactor);
	new (iArray+1) RPointerArray<DCodeSeg>(aMinGrowBy, aFactor);
	}


TInt DCodeSeg::RCodeSegsByAddress::Add(DCodeSeg* aCodeSeg)
	{
	TInt r = iArray[iActive^1].InsertInOrder(aCodeSeg, Compare);
	if(r!=KErrNone)
		return r;
	NKern::LockSystem();
	iActive^=1;
	NKern::UnlockSystem();
	r = iArray[iActive^1].InsertInOrder(aCodeSeg, Compare);
	if(r!=KErrNone)
		Remove(aCodeSeg);
	return r;
	}


TInt DCodeSeg::RCodeSegsByAddress::Remove(DCodeSeg* aCodeseg)
	{
	TInt result = KErrNotFound;
	TInt active = iActive;
	for(;;)
		{
		RPointerArray<DCodeSeg>& array = iArray[iActive^1];
		TInt first = array.SpecificFindInOrder(aCodeseg, Compare, EArrayFindMode_First);
		TInt last = array.SpecificFindInOrder(aCodeseg, Compare, EArrayFindMode_Last);
		for(; first >= 0 && first < last; ++first)
			if(array[first]==aCodeseg)
				{
				array.Remove(first);
				result = KErrNone;
				break;
				}
		if(active!=iActive)
			return result;
		NKern::LockSystem();
		iActive ^= 1;
		NKern::UnlockSystem();
		}
	}


DCodeSeg* DCodeSeg::RCodeSegsByAddress::Find(TLinAddr aAddress)
	{
	DCodeSeg* s = NULL;
	DCodeSeg* st = (DCodeSeg*)(TLinAddr(&aAddress)+1);
	RPointerArray<DCodeSeg>& array = iArray[iActive];
	TInt ix;
	array.SpecificFindInOrder(st, ix, Compare, EArrayFindMode_Last);
	if (ix)
		{
		s = array[ix-1];
		if (aAddress<s->iRunAddress || (aAddress-s->iRunAddress) >= s->iSize)
			s = NULL;
		}
	return s;
	}


TInt DCodeSeg::RCodeSegsByAddress::Compare(const DCodeSeg& aA, const DCodeSeg& aB)
	{
	TLinAddr aa = (TLinAddr)&aA;
	TLinAddr a = (aa&1) ? *(TLinAddr*)(aa&~3) : aA.iRunAddress;
	TLinAddr b = aB.iRunAddress;
	if (a > b)
		return 1;
	else if (a < b)
		return -1;
	return 0;
	}


inline TBool CheckUid(const TUid aUid, const TUid aRequestedUid)
	{
	return (aRequestedUid.iUid==0 || aRequestedUid.iUid==aUid.iUid);
	}

TBool K::CheckUids(const TUidType& aUids, const TUidType& aRequestedUids)
	{
	return CheckUid(aUids.iUid[0],aRequestedUids.iUid[0]) && 
			CheckUid(aUids.iUid[1],aRequestedUids.iUid[1]) && 
			CheckUid(aUids.iUid[2],aRequestedUids.iUid[2]);
	}

/** Enters thread critical section and acquires code segment mutex.

@pre No fast mutex can be held.
@pre Call in a thread context.
@pre Kernel must be unlocked
@pre interrupts enabled
*/
EXPORT_C void Kern::AccessCode()
	{
	CHECK_PRECONDITIONS(MASK_THREAD_STANDARD,"Kern::AccessCode");
	NKern::ThreadEnterCS();
	DCodeSeg::Wait();
	}

/** Exits thread critical section and releases code segment mutex.

@pre No fast mutex can be held.
@pre Call in a thread context.
@pre Kernel must be unlocked
@pre interrupts enabled
@pre Calling thread must be in a critical section
*/
EXPORT_C void Kern::EndAccessCode()
	{
	CHECK_PRECONDITIONS(MASK_THREAD_CRITICAL,"Kern::EndAccessCode");
	DCodeSeg::Signal();
	NKern::ThreadLeaveCS();
	}

/** Accessor function for the kernel's code segment list.

	@return Pointer to the code segment list.
*/
EXPORT_C SDblQue* Kern::CodeSegList()
	{
	return &DCodeSeg::GlobalList;
	}

/** Accessor function for the kernel's code segment list mutex.

	@return Pointer to the code segment list mutex.
*/
EXPORT_C DMutex* Kern::CodeSegLock()
	{
	return DCodeSeg::CodeSegLock;
	}

#ifdef KDLL
GLDEF_C void DumpCodeSegCreateInfo(TCodeSegCreateInfo& a)
	{
	Kern::Printf("UIDS: %08x %08x %08x",a.iUids.iUid[0],a.iUids.iUid[1],a.iUids.iUid[2]);
	Kern::Printf("Attr: %08x",a.iAttr);
	Kern::Printf("Code size: %08x",a.iCodeSize);
	Kern::Printf("Text size: %08x",a.iTextSize);
	Kern::Printf("Data size: %08x",a.iDataSize);
	Kern::Printf("Bss size: %08x",a.iBssSize);
	Kern::Printf("Total data size: %08x",a.iTotalDataSize);
	Kern::Printf("Entry Point Veneer: %08x",a.iEntryPtVeneer);
	Kern::Printf("File Entry Point: %08x",a.iFileEntryPoint);
	Kern::Printf("DepCount: %d",a.iDepCount);
	Kern::Printf("ExportDir: %08x",a.iExportDir);
	Kern::Printf("ExportDirCount: %08x",a.iExportDirCount);
	Kern::Printf("CodeLoad: %08x",a.iCodeLoadAddress);
	Kern::Printf("CodeRun: %08x",a.iCodeRunAddress);
	Kern::Printf("DataLoad: %08x",a.iDataLoadAddress);
	Kern::Printf("DataRun: %08x",a.iDataRunAddress);
	Kern::Printf("RootName off: %d len: %d",a.iRootNameOffset,a.iRootNameLength);
	Kern::Printf("Ext offset: %d",a.iExtOffset);
	Kern::Printf("Ver: %08x",a.iModuleVersion);
	Kern::Printf("SID: %08x",a.iS.iSecureId);
	Kern::Printf("Cap: %08x %08x",a.iS.iCaps[1],a.iS.iCaps[0]);
	Kern::Printf("Handle: %08x",a.iHandle);
	Kern::Printf("Client Process Handle: %08x",a.iClientProcessHandle);
	Kern::Printf("CodeRelocTable: %d bytes at %08x", a.iCodeRelocTableSize, a.iCodeRelocTable);
	Kern::Printf("ImportFixupTabl: %d bytes at %08x", a.iImportFixupTableSize, a.iImportFixupTable);
	Kern::Printf("CodeDelta: %08x", a.iCodeDelta);
	Kern::Printf("DataDelta: %08x", a.iDataDelta);
	Kern::Printf("UseCodePaging: %d", a.iUseCodePaging);
	}

GLDEF_C void DumpProcessCreateInfo(TProcessCreateInfo& a)
	{
	DumpCodeSegCreateInfo(a);
	Kern::Printf("HeapSizeMin: %08x", a.iHeapSizeMin);
	Kern::Printf("HeapSizeMax: %08x", a.iHeapSizeMax);
	Kern::Printf("StackSize: %08x", a.iStackSize);
	Kern::Printf("Client Handle: %08x",a.iClientHandle);
	Kern::Printf("Process Handle: %08x",a.iProcessHandle);
	Kern::Printf("Final Handle: %08x",a.iFinalHandle);
	Kern::Printf("OwnerType: %08x",a.iOwnerType);
	Kern::Printf("Priority: %08x",a.iPriority);
	Kern::Printf("Debug Attributes: %08x", a.iDebugAttributes);
	}
#endif

/******************************************************************************
 * DCodeSeg class
 ******************************************************************************/
void DCodeSeg::Wait()
	{
	if (CodeSegLock)
		Kern::MutexWait(*CodeSegLock);
	}

void DCodeSeg::Signal()
	{
	if (CodeSegLock)
		Kern::MutexSignal(*CodeSegLock);
	}


/**	Verifies that a code segment handle is valid.

	@param	aHandle	The code segment handle to check.
	
	@return	A pointer to the code segment if the handle is valid;
	        NULL, if the handle is invalid.

	@pre Call in a thread context.
 */
EXPORT_C DCodeSeg* DCodeSeg::VerifyHandle(TAny* aHandle)
	{
	CHECK_PRECONDITIONS(MASK_NOT_ISR|MASK_NOT_IDFC,"DCodeSeg::VerifyHandle");		
	DCodeSeg* pS=NULL;
	if (K::IsInKernelHeap(aHandle,sizeof(DCodeSeg)))
		{
		TUint32 cs_buf[sizeof(DCodeSeg)/sizeof(TUint32)];
		if (!Kern::SafeRead(aHandle,cs_buf,sizeof(DCodeSeg)))
			{
			DCodeSeg& cs=*(DCodeSeg*)cs_buf;
			TInt32 uid1=cs.iUids.iUid[0].iUid;
			if (cs.iAsyncDeleteNext==aHandle && (uid1==KDynamicLibraryUidValue || uid1==KExecutableImageUidValue))
				pS=(DCodeSeg*)aHandle;
			}
		}
	return pS;
	}

DCodeSeg* DCodeSeg::VerifyHandleP(TAny* aHandle)
	{
	DCodeSeg* pS=VerifyHandle(aHandle);
	if (!pS)
		K::PanicKernExec(EBadCodeSegHandle);
	return pS;
	}

DCodeSeg* DCodeSeg::VerifyCallerAndHandle(TAny* aHandle)
	{
	K::CheckFileServerAccess();
	return VerifyHandleP(aHandle);
	}

DCodeSeg* DCodeSeg::FindCodeSeg(const TFindCodeSeg& aFind)
	{
	if (GlobalList.IsEmpty())
		return NULL;
	DCodeSeg* st = (DCodeSeg*)(TLinAddr(&aFind.iName)+1);
	TBool newExe = (aFind.iUids.iUid[0].iUid==KExecutableImageUidValue);
	DProcess* pP = (DProcess*)aFind.iProcess;
	TInt first = CodeSegsByName.SpecificFindInOrder(st, &CompareCodeSegsByName, EArrayFindMode_First);
	TInt last = CodeSegsByName.SpecificFindInOrder(st, &CompareCodeSegsByName, EArrayFindMode_Last);
	__KTRACE_OPT(KDLL, Kern::Printf("FindCodeSeg %S first=%d last=%d", &aFind.iName, first, last));
	for (; first >= 0 && first < last; ++first)
		{
		DCodeSeg* pS = CodeSegsByName[first];
		if (!(pS->iMark&EMarkLoaded))
			continue;
	    if (((pS->iAttr ^ aFind.iAttrVal) & aFind.iAttrMask) != 0 || !K::CheckUids(pS->iUids,aFind.iUids) )
	          continue;
		if (aFind.iS.iSecureId && aFind.iS.iSecureId != pS->iS.iSecureId)
			continue;
		if (aFind.iModuleVersion!=KModuleVersionWild && aFind.iModuleVersion!=pS->iModuleVersion)
			continue;
		TUint32 c = 0;
		TInt i;
		for (i=0; i<SCapabilitySet::ENCapW; ++i)
			c |= (aFind.iS.iCaps[i] &~ pS->iS.iCaps[i]);
		if (c)
			continue;
		if ((pP||newExe) && !pS->FindCheck(pP))
			continue;
		return pS;
		}
	return NULL;
	}


/**	Removes a specified set of mark bits from every code segment in the system.

	@param	aMask	The mask of bits to reset.

    @pre Calling thread must be in a critical section.
    @pre No fast mutex can be held.
	@pre Call in a thread context.
	@pre Code segment lock mutex held.
	@pre Kernel must be unlocked
	@pre interrupts enabled
 */
EXPORT_C void DCodeSeg::UnmarkAll(TUint32 aMask)
	{
	CHECK_PRECONDITIONS(MASK_THREAD_CRITICAL,"DCodeSeg::UnmarkAll");		
	__ASSERT_WITH_MESSAGE_MUTEX(CodeSegLock,"Code segment lock mutex held","DCodeSeg::UnmarkAll");
	__KTRACE_OPT(KDLL,Kern::Printf("DCodeSeg::UnmarkAll %08x",aMask));
	SDblQueLink* anchor=&GlobalList.iA;
	SDblQueLink* pL=anchor->iNext;
	for (; pL!=anchor; pL=pL->iNext)
		_LOFF(pL,DCodeSeg,iLink)->iMark &= ~aMask;
	}


void DCodeSeg::UnmarkAndCloseAll(TUint32 aMask)
	{
	__KTRACE_OPT(KDLL,Kern::Printf("DCodeSeg::UnmarkAndCloseAll %08x",aMask));
	SDblQueLink* anchor=&GlobalList.iA;
	SDblQueLink* pL=anchor->iNext;
	SDblQueLink* pN=pL->iNext;
	for (; pL!=anchor; pL=pN, pN=pN->iNext)
		{
		DCodeSeg* pS=_LOFF(pL,DCodeSeg,iLink);
		if (pS->iMark & aMask)
			{
			pS->iMark &= ~aMask;
			pS->Close();
			}
		}
	}

TInt DCodeSeg::ListMarked(SDblQue& aQ, TUint32 aMask)
	{
	__KTRACE_OPT(KDLL,Kern::Printf(">DCodeSeg::ListMarked %08x",aMask));
	TInt n=0;
	SDblQueLink* anchor=&GlobalList.iA;
	SDblQueLink* pL=anchor->iNext;
	for (; pL!=anchor; pL=pL->iNext)
		{
		DCodeSeg* pS=_LOFF(pL,DCodeSeg,iLink);
		if (pS->iMark & aMask)
			{
			aQ.Add(&pS->iTempLink);
			++n;
			}
		}
	__KTRACE_OPT(KDLL,Kern::Printf("<DCodeSeg::ListMarked %d",n));
	return n;
	}


/** Removes all code segments from a queue and clear specified mark(s).

	@param aQ	Queue to iterate.  Code segments are linked together via DCodeSeg::iTempLink.
	@param aMask Bits to clear in DCodeSeg::iMark fields of iterated code segments.

	@pre DCodeSeg::CodeSegLock mutex held

	@post DCodeSeg::CodeSegLock mutex held

	@publishedPartner
	@released
 */
EXPORT_C void DCodeSeg::EmptyQueue(SDblQue& aQ, TUint32 aMask)
	{
	__ASSERT_WITH_MESSAGE_MUTEX(CodeSegLock,"Code segment lock mutex held","DCodeSeg::EmptyQueue");
	while(!aQ.IsEmpty())
		{
		DCodeSeg* pS=_LOFF(aQ.First()->Deque(), DCodeSeg, iTempLink);
		pS->iTempLink.iNext=NULL;
		pS->iMark &= ~aMask;
		}
	}

void DCodeSeg::DoKernelCleanup(TAny*)
	{
	// Clean up kernel-side code segments
	// Runs in supervisor thread
	__KTRACE_OPT(KDLL,Kern::Printf("DCodeSeg::DoKernelCleanup"));
	DCodeSeg::Wait();
	while(!KernelGarbageList.IsEmpty())
		{
		// if system has not gone idle since next codeseg was queued...
		DCodeSeg* pS = _LOFF(KernelGarbageList.First(),DCodeSeg,iGbgLink);
		TUint32 idleGenerationCount = pS->iGbgIdleGenerationCount;
		if(idleGenerationCount==NKern::IdleGenerationCount())
			{
			// then queue cleanup again for later...
			QueueKernelCleanup();
			break;
			}

		// close all codesegs with the same iGbgIdleGenerationCount...
		do
			{
			// deque and close codeseg...
			pS->iGbgLink.Deque();
			pS->iGbgLink.iNext = NULL;
			__NK_ASSERT_DEBUG(pS->iAccessCount>0);
			pS->CheckedClose();

			// get next codeseg...
			if(KernelGarbageList.IsEmpty())
				break;
			pS = _LOFF(KernelGarbageList.First(),DCodeSeg,iGbgLink);
			}
		while(pS->iGbgIdleGenerationCount==idleGenerationCount);
		}
	DCodeSeg::Signal();
	}

void DCodeSeg::EmptyGarbageList()
	{
	__KTRACE_OPT(KDLL,Kern::Printf("DCodeSeg::EmptyGarbageList"));
	while(!GarbageList.IsEmpty())
		{
		DCodeSeg* pS=_LOFF(GarbageList.First()->Deque(),DCodeSeg,iGbgLink);
		__NK_ASSERT_DEBUG(pS->iAccessCount==0);
		pS->iGbgLink.iNext=NULL;
		delete pS;
		}
	}

void DCodeSeg::DeferDeletes()
	{
	++DeleteLock;
	}

void DCodeSeg::EndDeferDeletes()
	{
	if (--DeleteLock==0)
		EmptyGarbageList();
	}

TInt DCodeSeg::WriteCallList(SDblQue& aList, TLinAddr* aEpList, TBool aInitData)
	{
	__KTRACE_OPT(KDLL,Kern::Printf(">DCodeSeg::WriteCallList"));
	SDblQueLink* anchor=&aList.iA;
	SDblQueLink* pL=aList.First();
	TAny* p=NULL;
	for (; !p && pL!=anchor; pL=pL->iNext, ++aEpList)
		{
		DCodeSeg* pS=_LOFF(pL, DCodeSeg, iTempLink);
		if (aInitData)
			pS->InitData();
		TLinAddr ep=pS->iEntryPtVeneer;
		__KTRACE_OPT(KDLL,Kern::Printf("Writing EP %08x->%08x",ep,aEpList));
		p=Kern::KUSafeWrite(aEpList, &ep, sizeof(TLinAddr));
		}
	TInt r=p?ECausedException:0;
	__KTRACE_OPT(KDLL,Kern::Printf("<DCodeSeg::WriteCallList %d",r));
	return r;
	}

/**	Finds a code segment by its entry point address.

	@param	aEntryPoint	The entry point to search for.
	
	@return	A pointer to the code segment if it exists;
	        NULL if the entry point does not match any code segment.

    @pre Calling thread must be in a critical section.
    @pre No fast mutex can be held.
	@pre Call in a thread context.
	@pre Code segment lock mutex held.
	@pre Kernel must be unlocked
	@pre interrupts enabled
 */
EXPORT_C DCodeSeg* DCodeSeg::CodeSegFromEntryPoint(TInt aEntryPoint)
	{
	CHECK_PRECONDITIONS(MASK_THREAD_CRITICAL,"DCodeSeg::CodeSegFromEntryPoint");		
	__ASSERT_WITH_MESSAGE_MUTEX(CodeSegLock,"Code segment lock mutex held","DCodeSeg::CodeSegFromEntryPoint");
	DCodeSeg* s = Kern::CodeSegFromAddress( (TLinAddr)aEntryPoint, TheCurrentThread->iOwningProcess );
	if (s && s->iFileEntryPoint == TLinAddr(aEntryPoint))
		return s;
	return NULL;
	}

DCodeSeg::DCodeSeg()
	:	iAccessCount(1), iMark(EMarkLdr)
	{
	iLink.iNext=NULL;
	iTempLink.iNext=NULL;
	iGbgLink.iNext=NULL;
	}

void DCodeSeg::Destruct()
	{
	__KTRACE_OPT(KDLL,Kern::Printf("DCodeSeg::Destruct %C", this));
#ifdef BTRACE_CODESEGS
	BTrace4(BTrace::ECodeSegs,BTrace::ECodeSegDestroyed,this);
#endif
	Wait();
	TInt first = CodeSegsByName.SpecificFindInOrder(this, &CompareCodeSegsByName, EArrayFindMode_First);
	TInt last = CodeSegsByName.SpecificFindInOrder(this, &CompareCodeSegsByName, EArrayFindMode_Last);
	for (; first >= 0 && first < last; ++first)
		{
		if (CodeSegsByName[first] == this)
			{
			CodeSegsByName.Remove(first);
			__KTRACE_OPT(KDLL,Kern::Printf("CodeSeg @%08x Removed By Name", this));
			break;
			}
		}
	if(CodeSegsByAddress.Remove(this)==KErrNone)
		{
		__KTRACE_OPT(KDLL,Kern::Printf("CodeSeg @%08x Removed By Address %08x (index %d)", this, iRunAddress, first));
		}
	if (iLink.iNext)
		iLink.Deque();
	if (iTempLink.iNext)
		iTempLink.Deque();
	Signal();
	delete iFileName;
	Kern::Free(iDeps);
	Kern::SafeClose((DObject*&)iAttachProcess,NULL);
	iAsyncDeleteNext = NULL; // from now on VerifyHandle(this) will fail 
	}

void DCodeSeg::Close()
	{
	__KTRACE_OPT(KDLL,Kern::Printf("DCodeSeg::Close %d %C", iAccessCount, this));
	__NK_ASSERT_DEBUG(iAccessCount>0);
	if (--iAccessCount==0)
		delete this;
	}

void DCodeSeg::CheckedClose()
	{
	__KTRACE_OPT(KDLL,Kern::Printf("DCodeSeg::CheckedClose %d %C", iAccessCount, this));
	__NK_ASSERT_DEBUG(iAccessCount>0);
	if (--iAccessCount==0)
		{
		if (DeleteLock)
			GarbageList.Add(&iGbgLink);
		else
			delete this;
		}
	}

void DCodeSeg::CheckedOpen()
	{
	__KTRACE_OPT(KDLL,Kern::Printf("DCodeSeg::CheckedOpen %d %C", iAccessCount, this));
	if (iGbgLink.iNext)
		{
		__NK_ASSERT_DEBUG(iAccessCount==0 || iAccessCount==1);
		iAccessCount = 1;
		iGbgLink.Deque();
		iGbgLink.iNext=NULL;
		}
	else
		++iAccessCount;
	}

void DCodeSeg::WaitCheckedOpen()
	{
	DCodeSeg::Wait();
	CheckedOpen();
	DCodeSeg::Signal();
	}

void DCodeSeg::QueueKernelCleanup()
	{
	__ASSERT_MUTEX(CodeSegLock);
	if(!KernelCleanupLock && !KernelGarbageList.IsEmpty())
		KernelCleanupDfc.QueueOnIdle();
	}

void DCodeSeg::DeferKernelCleanup()
	{
	DCodeSeg::Wait();
	++KernelCleanupLock;
	DCodeSeg::Signal();
	}

void DCodeSeg::EndDeferKernelCleanup()
	{
	DCodeSeg::Wait();
	__NK_ASSERT_DEBUG(KernelCleanupLock>0);
	if(!--KernelCleanupLock)
		{
		TUint32 idleGenerationCount = NKern::IdleGenerationCount();
		while (!DeferredKernelGarbageList.IsEmpty())
			{
			DCodeSeg* pS = _LOFF(DeferredKernelGarbageList.GetFirst(), DCodeSeg, iGbgLink);
			pS->iGbgIdleGenerationCount = idleGenerationCount;
			KernelGarbageList.Add(&pS->iGbgLink);
			}
		if(!KernelGarbageList.IsEmpty())
			KernelCleanupDfc.QueueOnIdle();
		}
	DCodeSeg::Signal();
	}

void DCodeSeg::ScheduleKernelCleanup(TBool aImmed)
	{
	__KTRACE_OPT(KDLL,Kern::Printf("DCodeSeg::ScheduleKernelCleanup %d %C %d", iAccessCount, this, aImmed));
	__NK_ASSERT_DEBUG(iAccessCount && (iAttr & ECodeSegAttKernel));
	DProcess& kern = *K::TheKernelProcess;
	SDblQue cs_list;
	DCodeSeg::Wait();
	kern.RemoveCodeSeg(this, &cs_list);	// remove this code seg from the kernel's list

	// Call destructors in all codesegs...
	// Go through in reverse order since the top level code segment appears at the end
	// and we want to call destructors in highest-level-first order.
	SDblQueLink* next_link = &cs_list.iA;
	while ((next_link=next_link->iPrev) != &cs_list.iA)
		{
		DCodeSeg* pS=_LOFF(next_link, DCodeSeg, iTempLink);
		if (pS->iMark & DCodeSeg::EMarkDataInit)
			{
			TLibraryEntry f=(TLibraryEntry)pS->iEntryPtVeneer;
			__KTRACE_OPT(KDLL,Kern::Printf("EntryPoint %08x DETACH",f));
			(*f)(KModuleEntryReasonProcessDetach);
			}
		}

	// close all codesegs...
	TUint32 idleGenerationCount = NKern::IdleGenerationCount();
	while (!cs_list.IsEmpty())
		{
		DCodeSeg* pS=_LOFF(cs_list.Last()->Deque(), DCodeSeg, iTempLink);
		pS->iTempLink.iNext=NULL;
		if (aImmed)
			pS->CheckedClose();
		else
			{
			// Don't actually delete the code segment until the system goes idle.
			__NK_ASSERT_DEBUG(!pS->iGbgLink.iNext);
			if(KernelCleanupLock)
				DeferredKernelGarbageList.Add(&pS->iGbgLink);
			else
				{
				pS->iGbgIdleGenerationCount = idleGenerationCount;
				KernelGarbageList.Add(&pS->iGbgLink);
				}
			}
		}

	if (!aImmed)
		QueueKernelCleanup();

	DCodeSeg::Signal();
	}

TInt DCodeSeg::Create(TCodeSegCreateInfo& aInfo, DProcess* aProcess)
	{
	const TInt KValidAttrMask =
		ECodeSegAttKernel | ECodeSegAttGlobal | ECodeSegAttFixed | ECodeSegAttABIMask | ECodeSegAttHDll | ECodeSegAttExpVer | ECodeSegAttNmdExpData | ECodeSegAttSMPSafe;
	
	__KTRACE_OPT(KDLL,Kern::Printf("DCodeSeg::Create %08x file %S ver %08x process %O",this,&aInfo.iFileName,aInfo.iModuleVersion,aProcess));
	TInt32 uid1=aInfo.iUids.iUid[0].iUid;
	if (uid1!=KDynamicLibraryUidValue && uid1!=KExecutableImageUidValue)
		return KErrNotSupported;
	iUids=aInfo.iUids;
	iAttr=aInfo.iAttr & KValidAttrMask;
	TInt fnl=aInfo.iFileName.Length();
	if (TUint32(aInfo.iRootNameOffset)>=TUint32(fnl-1))
		return KErrArgument;
	if (TUint32(aInfo.iRootNameLength)>TUint32(fnl))
		return KErrArgument;
	if (TUint32(aInfo.iRootNameOffset+aInfo.iRootNameLength)>TUint32(fnl))
		return KErrArgument;
	if (TUint32(aInfo.iExtOffset)>TUint32(fnl) || TUint32(aInfo.iExtOffset)<TUint32(aInfo.iRootNameOffset))
		return KErrArgument;
	if (aInfo.iDepCount<0)
		return KErrArgument;
	if (!(iAttr&ECodeSegAttKernel) && !aProcess && aInfo.iTotalDataSize!=0)
		return KErrNotSupported;	// don't allow static data for non-kernel code unless client process is specified
	iFileName=HBuf::New(aInfo.iFileName);
	if (!iFileName)
		return KErrNoMemory;
#ifdef BTRACE_CODESEGS
	BTraceBig(BTrace::ECodeSegs,BTrace::ECodeSegCreated,this,iFileName->Ptr(),iFileName->Length());
#endif
	iRootName.Set(iFileName->Mid(aInfo.iRootNameOffset,aInfo.iRootNameLength));
	iExtOffset=aInfo.iExtOffset;
	iModuleVersion=aInfo.iModuleVersion;
	iDepCount=aInfo.iDepCount;
	iS=aInfo.iS;
	if (iDepCount)
		{
		iDeps=(DCodeSeg**)Kern::AllocZ(iDepCount*sizeof(DCodeSeg*));
		if (!iDeps)
			return KErrNoMemory;
		}
	TInt r=DoCreate(aInfo,aProcess);
	__KTRACE_OPT(KDLL,Kern::Printf("attach proc=%O exe code seg=%08x",iAttachProcess,iExeCodeSeg));
	if (r==KErrNone)
		{
		iFileEntryPoint=aInfo.iFileEntryPoint;
		aInfo.iEntryPtVeneer=aInfo.iFileEntryPoint;	// default - modified by loader if necessary
		iEntryPtVeneer=0;	// for now
		DCodeSeg::Wait();
		r = CodeSegsByName.InsertInOrderAllowRepeats(this, &CompareCodeSegsByName);
		if (r == KErrNone)
			{
			__KTRACE_OPT(KDLL,Kern::Printf("CodeSeg @%08x Added By Name", this));
			if(!(iAttr&ECodeSegAttAddrNotUnique))
				{
				// put codesegs which have a unique run address into a container...
				r = CodeSegsByAddress.Add(this);
				if (r == KErrNone)
					{
					__KTRACE_OPT(KDLL,Kern::Printf("CodeSeg @%08x Added By Address %08x", this, iRunAddress));
					}
				}
			}
		if (r == KErrNone)
			GlobalList.Add(&iLink);
		iAsyncDeleteNext=this;
		DCodeSeg::Signal();
#ifdef BTRACE_CODESEGS
		TModuleMemoryInfo info;
		GetMemoryInfo(info,NULL);
		BTraceN(BTrace::ECodeSegs,BTrace::ECodeSegInfo,this,iAttr,&info,sizeof(info));
#endif
		}
	return r;
	}
	
TInt DCodeSeg::Loaded(TCodeSegCreateInfo& aInfo)
	{
	iAttr &= ~ECodeSegAttSMPSafe;
	iAttr |= aInfo.iAttr & ECodeSegAttSMPSafe;
	iEntryPtVeneer = aInfo.iEntryPtVeneer;		// final value provided by loader
	FinaliseRecursiveFlags();
	return KErrNone;
	}

void DCodeSeg::SetAttachProcess(DProcess* aProcess)
	{
	__KTRACE_OPT(KDLL,Kern::Printf("DCodeSeg %C SetAttachProcess(%O)", this, aProcess));
	__ASSERT_ALWAYS(!iAttachProcess, K::Fault(K::ECodeSegAttachProcess));
	DCodeSeg* pPSeg=aProcess->CodeSeg();
	iAttachProcess=aProcess;
	__ASSERT_DEBUG(!iExeCodeSeg || iExeCodeSeg==pPSeg, K::Fault(K::ECodeSegBadExeCodeSeg));
	iExeCodeSeg=pPSeg;
	iAttachProcess->Open();
	}

void DCodeSeg::FinaliseRecursiveFlags()
	{
	if (!(iMark&EMarkRecursiveFlagsValid))
		{
		DCodeSeg::UnmarkAll(EMarkRecursiveFlagsCheck);
		CalcRecursiveFlags();
		iMark|=EMarkRecursiveFlagsValid;
		}
	}

void DCodeSeg::CalcRecursiveFlags()
	{
	__KTRACE_OPT(KDLL,Kern::Printf(">DCodeSeg::CalcRecursiveFlags %C %08x", this, iMark));
	if (!(iMark&EMarkRecursiveFlagsCheck))
		{
		TUint32 valid=EMarkRecursiveFlagsValid;
		iMark|=EMarkRecursiveFlagsCheck;
		TInt i;
		for (i=0; i<iDepCount; ++i)
			{
			DCodeSeg* pS=iDeps[i];
			if (pS)
				{
				if (!(pS->iMark & EMarkRecursiveFlagsValid))
					{
					pS->CalcRecursiveFlags();
					if (!(pS->iMark & EMarkRecursiveFlagsValid))
						valid=0;
					}
				TUint32 m=pS->iMark;
				if (m & EMarkDataPresent)
					iMark|=EMarkDataPresent;
				if (IsDll() && pS->IsDll() && (m & EMarkDataInit))
					iMark|=EMarkDataInit;
				if (!iAttachProcess && pS->iAttachProcess)
					SetAttachProcess(pS->iAttachProcess);
				if (!iExeCodeSeg && pS->iExeCodeSeg)
					iExeCodeSeg=pS->iExeCodeSeg;
				if (!(pS->iAttr & ECodeSegAttSMPSafe))
					iAttr &= ~ECodeSegAttSMPSafe;
				}
			}
		iMark|=valid;	// set valid bit if no cycles were encountered
		}
	__KTRACE_OPT(KDLL,Kern::Printf("<DCodeSeg::CalcRecursiveFlags %C %08x", this, iMark));
	}

void DCodeSeg::Info(TCodeSegCreateInfo& aInfo)
	{
	aInfo.iFileName=*iFileName;
	aInfo.iUids=iUids;
	aInfo.iAttr=iAttr;
	aInfo.iEntryPtVeneer=(TUint32)iEntryPtVeneer;
	aInfo.iFileEntryPoint=(TUint32)iFileEntryPoint;
	aInfo.iDepCount=iDepCount;
	aInfo.iRootNameOffset=iRootName.Ptr()-iFileName->Ptr();
	aInfo.iRootNameLength=iRootName.Length();
	aInfo.iExtOffset=iExtOffset;
	aInfo.iModuleVersion=iModuleVersion;
	aInfo.iS=iS;
	aInfo.iHandle=this;
	}

TInt DCodeSeg::AddDependency(DCodeSeg* aExporter)
	{
	__ASSERT_ALWAYS(iNextDep<iDepCount, K::Fault(K::ECodeDependenciesInconsistent));
	if (aExporter->IsExe())
		{
		if (iExeCodeSeg && iExeCodeSeg!=aExporter)
			return KErrNotSupported;
		iExeCodeSeg=aExporter;
		}
	iDeps[iNextDep++]=aExporter;
	return KErrNone;
	}

TInt DCodeSeg::ListDeps(SDblQue* aQ, TUint32 aMark)
	{
	__KTRACE_OPT(KDLL,Kern::Printf(">DCodeSeg::ListDeps %C %08x", this, aMark));
	TInt n=0;
	if (!(iMark & aMark))
		{
		iMark|=aMark;
		TInt i;
		for (i=0; i<iDepCount; ++i)
			if (iDeps[i])
				n+=iDeps[i]->ListDeps(aQ,aMark);
		if (aQ)
			aQ->Add(&iTempLink);
		++n;
		}
	__KTRACE_OPT(KDLL,Kern::Printf("<DCodeSeg::ListDeps %d",n));
	return n;
	}

TInt DCodeSeg::UnListDeps(TUint32 aMark)
	{
	__KTRACE_OPT(KDLL,Kern::Printf(">DCodeSeg::UnListDeps %C", this));
	TInt n=0;
	if (!(iMark & aMark))
		{
		iMark|=aMark;
		TInt i;
		for (i=0; i<iDepCount; ++i)
			if (iDeps[i])
				n+=iDeps[i]->UnListDeps(aMark);
		if (iTempLink.iNext)
			{
			iTempLink.Deque();
			iTempLink.iNext=NULL;
			++n;
			}
		}
	__KTRACE_OPT(KDLL,Kern::Printf("<DCodeSeg::UnListDeps %d",n));
	return n;
	}

TInt DCodeSeg::MarkAndOpenDeps(TUint32 aMark)
	{
	__KTRACE_OPT(KDLL,Kern::Printf(">DCodeSeg::MarkAndOpenDeps %C %08x", this, aMark));
	TInt n=0;
	if (!(iMark & aMark))
		{
		iMark|=aMark;
		TInt i;
		for (i=0; i<iDepCount; ++i)
			if (iDeps[i])
				n+=iDeps[i]->MarkAndOpenDeps(aMark);
		CheckedOpen();
		++n;
		}
	__KTRACE_OPT(KDLL,Kern::Printf("<DCodeSeg::MarkAndOpenDeps %d",n));
	return n;
	}

void DCodeSeg::AppendFullRootName(TDes& aDes)
	{
	if (!iFileName)
		{
		aDes.Append('@');
		aDes.AppendNumFixedWidth((TUint32)this, EHex, 8);
		return;
		}
	if (!(iAttr & ECodeSegAttExpVer))
		{
		aDes.Append(iRootName);
		return;
		}
	TInt root_offset = iRootName.Ptr() - iFileName->Ptr();
	TInt root_base_len = iExtOffset - root_offset;
	aDes.Append(iRootName.Left(root_base_len));
	AppendVerExt(aDes);
	}

void DCodeSeg::AppendFullFileName(TDes& aDes)
	{
	if (!iFileName)
		{
		aDes.Append('@');
		aDes.AppendNumFixedWidth((TUint32)this, EHex, 8);
		return;
		}
	if (!(iAttr & ECodeSegAttExpVer))
		{
		aDes.Append(*iFileName);
		return;
		}
	aDes.Append(iFileName->Left(iExtOffset));
	AppendVerExt(aDes);
	}

void DCodeSeg::AppendVerExt(TDes& aDes)
	{
	aDes.Append('{');
	aDes.AppendNumFixedWidth(iModuleVersion, EHex, 8);
	aDes.Append('}');
	aDes.Append(iFileName->Mid(iExtOffset));
	}

void DCodeSeg::TraceAppendFullName(TDes& aDes)
	{
	AppendFullFileName(aDes);
	aDes.Append(_S8(" Ver "), 5);
	aDes.AppendNum((TInt)(iModuleVersion >> 16));
	aDes.Append('.');
	aDes.AppendNum((TInt)(iModuleVersion & 0x0000ffffu));
	}

void DCodeSeg::BTracePrime(TInt aCategory)
	{
#ifdef BTRACE_CODESEGS
	if (aCategory == BTrace::ECodeSegs || aCategory == -1)
		{
		BTraceBig(BTrace::ECodeSegs,BTrace::ECodeSegCreated,this,iFileName->Ptr(),iFileName->Length());
		TModuleMemoryInfo info;
		GetMemoryInfo(info,NULL);
		BTraceN(BTrace::ECodeSegs,BTrace::ECodeSegInfo,this,iAttr,&info,sizeof(info));
		}
#endif
	}

/******************************************************************************
 * Process stuff
 ******************************************************************************/

TInt DProcess::WaitDllLock()
	{
	// Can not use Kern::MutexWait() here as DLL lock can be held without calling
	// thread being in CS.
	NKern::LockSystem();
	TInt r = iDllLock->Wait();
	NKern::UnlockSystem();
	return r;
	}

void DProcess::SignalDllLock()
	{
	// Can not use Kern::MutexSignal() here as DLL lock can be held without calling
	// thread being in CS.
	NKern::LockSystem();
	iDllLock->Signal();
	}

/** Walks through all code segments used by this process.

	Iterates through the static and dynamic code dependency graph of this
	process and marks each traversed code segment using the specified mark.

	Optionally adds the traversed code segments to a specified queue, or removes
	them from this queue.  The queue uses DCodeSeg::iTempLink.

	When the queue is not used anymore, the mark must be cleared by calling
	DCodeSeg::EmptyQueue().

	The code segment mutex must be acquired before calling this function and released
	only when the queue is not used anymore.

	@param aQ	Queue where code segment should be added/removed or NULL.
	@param aExclude	Code segment to exclude from the iteration or NULL.
	@param aMark Mark or-ed to DCodeSeg::iMark of all iterated code segments.
	@param aFlags  Bitmask storing action to perform on queue and traversal policy.
	
	@return Number of code segments traversed. 

	@pre Calling thread must be in a critical section.
	@pre DCodeSeg::CodeSegLock mutex held.
	@pre Kernel must be unlocked
	@pre interrupts enabled
	@pre No fast mutex can be held
	@pre Call in a thread context

	@post DCodeSeg::CodeSegLock mutex held.
	@post Calling thread is in a critical section.

	@see Kern::AccessCode()
	@see Kern::EndAccessCode()
	@see DCodeSeg::TMark
	@see DProcess::TTraverseFlags
	@see DCodeSeg::EmptyQueue()

	@publishedPartner
	@released
 */
EXPORT_C TInt DProcess::TraverseCodeSegs(SDblQue* aQ, DCodeSeg* aExclude, TUint32 aMark, TUint32 aFlags)
	{
	CHECK_PRECONDITIONS(MASK_THREAD_CRITICAL,"DProcess::TraverseCodeSegs");		
	__ASSERT_WITH_MESSAGE_MUTEX(DCodeSeg::CodeSegLock,"DCodeSeg::CodeSegLock mutex held","DProcess::TraverseCodeSegs");
	__KTRACE_OPT(KDLL,Kern::Printf(">DProcess %O TraverseCodeSegs exclude %08x mark %08x flg %x",this,aExclude,aMark,aFlags));
	TInt n=0;
	TInt i;
	TInt c=iDynamicCode.Count();
	for (i=-1; i<c; ++i)
		{
		DCodeSeg* pS=(i<0)?iCodeSeg:iDynamicCode[i].iSeg;
		if (pS && pS!=aExclude)
			{
			if (aFlags & ETraverseFlagRestrict)
				{
				if (i>=0)
					{
					// Codeseg is a dynamic dependency
					DLibrary* pL=iDynamicCode[i].iLib;
					if (pL && pL->iState!=DLibrary::EAttached)
						continue;
					}
				else
					{
					// Static dependencies of process
					if (!(iAttributes&EStaticCallsDone))
						continue;
					}
				}
			if (aFlags & ETraverseFlagAdd)
				n+=pS->ListDeps(aQ,aMark);
			else
				n+=pS->UnListDeps(aMark);
			}
		}
	__KTRACE_OPT(KDLL,Kern::Printf("<DProcess %O TraverseCodeSegs %d",this,n));
	return n;
	}

TInt DProcess::OpenDeps()
	{
	__KTRACE_OPT(KDLL,Kern::Printf(">DProcess %O OpenDeps",this));
	TInt r=KErrNone;
	DCodeSeg::UnmarkAll(DCodeSeg::EMarkListDeps|DCodeSeg::EMarkUnListDeps);
	SDblQue cs_list;
	iTempCodeSeg->ListDeps(&cs_list, DCodeSeg::EMarkListDeps);	// list code segments in least-first order
	SDblQueLink* anchor=&cs_list.iA;
	SDblQueLink* pL=cs_list.First();
	for(; pL!=anchor; pL=pL->iNext)
		{
		DCodeSeg* pN=_LOFF(pL,DCodeSeg,iTempLink);
		r=MapCodeSeg(pN);
		if (r!=KErrNone)
			break;
		pN->CheckedOpen();
		__DEBUG_EVENT2(EEventAddCodeSeg, pN, this);
#ifdef BTRACE_CODESEGS
		BTrace8(BTrace::ECodeSegs,BTrace::ECodeSegMapped,pN,this);
#endif
		}
	if (r!=KErrNone)
		{
		pL=pL->iPrev;	// points to last one which was actually mapped
		SDblQueLink* pLL=pL->iPrev;
		for (; pL!=anchor; pL=pLL, pLL=pLL->iPrev)
			{
			DCodeSeg* pN=_LOFF(pL,DCodeSeg,iTempLink);

			__REMOVE_CODESEG_FROM_CODEMODIFIER(pN,this);
			__DEBUG_EVENT2(EEventRemoveCodeSeg, pN, this);
			UnmapCodeSeg(pN);
#ifdef BTRACE_CODESEGS
			BTrace8(BTrace::ECodeSegs,BTrace::ECodeSegUnmapped,pN,this);
#endif
			pN->CheckedClose();
			}
		}
	else
		{
		iCodeSeg=iTempCodeSeg;
		iTempCodeSeg=NULL;
		iCodeSeg->iMark|=DCodeSeg::EMarkLoaded;	// process code segment could now be reloaded
		iCodeSeg->CheckedClose();	// balance extra access for EXE code segment
		}
	DCodeSeg::EmptyQueue(cs_list,0);
	__KTRACE_OPT(KDLL,Kern::Printf("<DProcess %O OpenDeps %d",this,r));
	return r;
	}

TInt DProcess::AddCodeSeg(DCodeSeg* aSeg, DLibrary* aLib, SDblQue& aQ)
	{
	__KTRACE_OPT(KDLL,Kern::Printf(">DProcess %O AddCodeSeg %O",this,aLib));
	DCodeSeg* pS=aSeg;
	SCodeSegEntry e;
	e.iSeg=pS;
	e.iLib=aLib;
	DCodeSeg::UnmarkAll(DCodeSeg::EMarkListDeps|DCodeSeg::EMarkUnListDeps);
	TInt n=pS->ListDeps(&aQ, DCodeSeg::EMarkListDeps);			// list code segments in least-first order
	n-=TraverseCodeSegs(&aQ, NULL, DCodeSeg::EMarkUnListDeps, 0);		// remove already present ones
	TInt r=KErrNone;
	SDblQueLink* anchor=&aQ.iA;
	SDblQueLink* pL=aQ.First();
	for(; pL!=anchor; pL=pL->iNext)
		{
		DCodeSeg* pN=_LOFF(pL,DCodeSeg,iTempLink);
		r=MapCodeSeg(pN);
		if (r!=KErrNone)
			break;
		__DEBUG_EVENT2(EEventAddCodeSeg, pN, this);
#ifdef BTRACE_CODESEGS
		BTrace8(BTrace::ECodeSegs,BTrace::ECodeSegMapped,pN,this);
#endif
		pN->CheckedOpen();
		__KTRACE_OPT(KDLL,Kern::Printf("pN=%08x aLib=%08x iMark=%08x", pN, aLib, pN->iMark));
		if (!aLib && pN->iMark&DCodeSeg::EMarkDataInit)
			{
			// kernel library - initialise data now
			pN->InitData();
			TLibraryEntry f=(TLibraryEntry)pN->iEntryPtVeneer;
			__KTRACE_OPT(KDLL,Kern::Printf("EntryPoint %08x ATTACH",f));
			(*f)(KModuleEntryReasonProcessAttach);
			}
		}
	if (r==KErrNone && !(pS->iAttr & ECodeSegAttSMPSafe))
		{
		if (__e32_atomic_add_ord32(&iSMPUnsafeCount, 1) == 0)
			{
#ifdef __SMP__
			r = UpdateSMPSafe();
#endif
			}
		}
	if (r==KErrNone)
		r=iDynamicCode.InsertInUnsignedKeyOrder(e);
	if (r!=KErrNone && !(pS->iAttr & ECodeSegAttSMPSafe))
		{
		if (__e32_atomic_add_ord32(&iSMPUnsafeCount, (TUint)-1) == 1)
			{
#ifdef __SMP__
			UpdateSMPSafe();
#endif
			}
		}
	if (r!=KErrNone && n!=0)	// n=0 if code seg already mapped owing to an implicit linkage
		{
		pL=pL->iPrev;	// points to last one which was actually mapped
		SDblQueLink* pLL=pL->iPrev;
		for (; pL!=anchor; pL=pLL, pLL=pLL->iPrev)
			{
			DCodeSeg* pN=_LOFF(pL,DCodeSeg,iTempLink);
			if (!aLib && pN->iMark&DCodeSeg::EMarkDataInit)
				{
				TLibraryEntry f=(TLibraryEntry)pN->iEntryPtVeneer;
				__KTRACE_OPT(KDLL,Kern::Printf("EntryPoint %08x DETACH",f));
				(*f)(KModuleEntryReasonProcessDetach);
				}
			
			__REMOVE_CODESEG_FROM_CODEMODIFIER(pN,this);
			__DEBUG_EVENT2(EEventRemoveCodeSeg, pN, this);
			UnmapCodeSeg(pN);
#ifdef BTRACE_CODESEGS
			BTrace8(BTrace::ECodeSegs,BTrace::ECodeSegUnmapped,pN,this);
#endif
			pN->CheckedClose();
			}
		}
	__KTRACE_OPT(KDLL,Kern::Printf("<DProcess %O AddCodeSeg %d",this,r));
	return r;
	}

void DProcess::RemoveCodeSeg(DCodeSeg* aSeg, SDblQue* aQ)
	{
	__KTRACE_OPT(KDLL,Kern::Printf(">DProcess %O RemoveCodeSeg %08x(%C) Q:%08x", this, aSeg, aSeg, aQ));
	SCodeSegEntry e;
	e.iSeg=aSeg;
	e.iLib=NULL;
	TInt i;
	if (aSeg!=iCodeSeg)
		{
		TInt r=iDynamicCode.FindInUnsignedKeyOrder(e,i);
		__ASSERT_ALWAYS(r==KErrNone, K::Fault(K::ECodeSegRemoveAbsent));
		iDynamicCode.Remove(i);
		if (!(aSeg->iAttr & ECodeSegAttSMPSafe))
			{
			if (__e32_atomic_add_ord32(&iSMPUnsafeCount, (TUint)-1) == 1)
				{
#ifdef __SMP__
				UpdateSMPSafe();
#endif
				}
			}
		}
	SDblQue cs_list;
	SDblQue* q = aQ ? aQ : &cs_list;
	DCodeSeg::UnmarkAll(DCodeSeg::EMarkListDeps|DCodeSeg::EMarkUnListDeps);
	aSeg->ListDeps(q, DCodeSeg::EMarkListDeps);	// list all dependents of this code seg
	TraverseCodeSegs(q, aSeg, DCodeSeg::EMarkUnListDeps, 0);	// remove those still needed
	NKern::LockSystem();
	if (aSeg==iCodeSeg)
		iCodeSeg=NULL;
	NKern::UnlockSystem();
	if (!aQ)
		{
		while(!cs_list.IsEmpty())
			{
			DCodeSeg* pS=_LOFF(cs_list.First()->Deque(), DCodeSeg, iTempLink);
			pS->iTempLink.iNext=NULL;
			__REMOVE_CODESEG_FROM_CODEMODIFIER(pS,this);
			__DEBUG_EVENT2(EEventRemoveCodeSeg, pS, this);
			UnmapCodeSeg(pS);
#ifdef BTRACE_CODESEGS
			BTrace8(BTrace::ECodeSegs,BTrace::ECodeSegUnmapped,pS,this);
#endif
			pS->CheckedClose();
			}
		}
	__KTRACE_OPT(KDLL,Kern::Printf("<DProcess %O RemoveCodeSeg",this));
	}

TInt DProcess::CallList(SDblQue& aQ, DCodeSeg* aSeg, TUint32 aFlag)
	{
	__KTRACE_OPT(KDLL,Kern::Printf("DProcess %O CallList seg=%08x flg=%x",this,aSeg,aFlag));
	DCodeSeg::UnmarkAll(DCodeSeg::EMarkListDeps|DCodeSeg::EMarkUnListDeps);
	TInt n=aSeg->ListDeps(&aQ, DCodeSeg::EMarkListDeps);
	n-=TraverseCodeSegs(&aQ, aSeg, DCodeSeg::EMarkUnListDeps, aFlag&~ETraverseFlagAdd);
	return n;
	}

/******************************************************************************
 * Library
 ******************************************************************************/
DLibrary::DLibrary()
	{
	iThreadLink.iNext=NULL;
	}

DLibrary::~DLibrary()
	{
	__ASSERT_ALWAYS(iMapCount==0, K::Fault(K::ELibDestructBadMapCount));
	}

// If aPtr!=NULL a user handle is being closed
TInt DLibrary::Close(TAny* aPtr)
	{
	DProcess* pP=(DProcess*)aPtr;
	DThread& t=*TheCurrentThread;
	TInt m=1;
	if (pP)
		{
		DCodeSeg::Wait();
		m=--iMapCount;
		// debugger will be notified later if library is being unloaded
		__COND_DEBUG_EVENT(m>0, EEventRemoveLibrary, this);
		if (m==0)
			{
			TBool first_close=t.iClosingLibs.IsEmpty();
			if (t.iOwningProcess==pP && iState==EAttached && (!first_close || iCodeSeg->iMark&DCodeSeg::EMarkDataInit))
				{
				iState=EDetachPending;
				t.iClosingLibs.Add(&iThreadLink);	// need to run destructors
				if (!first_close)
					{
					// this is a close within a close
					// destructors only run after those from first Close
					__KTRACE_OPT(KDLL,Kern::Printf("DLibrary %O CWC",this));
					DCodeSeg::Signal();
					return 0;
					}
				}
			else
				{
				// thread exiting/constructors never run/constructors not needed
				__DEBUG_EVENT(EEventRemoveLibrary, this);
				iState=ELoaded;
				RemoveFromProcess();
				m=-1;
				}
			}
		__KTRACE_OPT(KDLL,Kern::Printf("DLibrary %O Close m=%d",this,m));
		TInt ret = EObjectUnmapped;
		// close the object before signal to prevent an open race condition.
		if (m)		// don't close yet if destructors needed
			ret = DObject::Close(aPtr);
		// THIS OBJECT MAY NOW BE DEAD, only Signal and return should follow.
		DCodeSeg::Signal();
		return ret;	// signal that destructors are required
		}
	else
		return DObject::Close(aPtr);
	}

void DLibrary::RemoveFromProcess()
	{
	DProcess* pP=(DProcess*)Owner();
	if (pP->iThreadsLeaving)
		{
		// Put library on process garbage list
		Open();
		pP->iGarbageList.Add(&iGbgLink);
		}
	else
		ReallyRemoveFromProcess();
	}

void DLibrary::ReallyRemoveFromProcess()
	{
	DProcess* pP=(DProcess*)Owner();
	NKern::FMWait(&DObject::Lock);
	DCodeSeg* pS=iCodeSeg;
	iCodeSeg=NULL;
	NKern::FMSignal(&DObject::Lock);
	NKern::LockSystem();
	NKern::UnlockSystem();
	pP->RemoveCodeSeg(pS, NULL);
	}

// Called when a handle is opened to this library
TInt DLibrary::AddToProcess(DProcess* /*aProcess*/)
	{
	DCodeSeg::Wait();
	++iMapCount;
	DCodeSeg::Signal();
	return KErrNone;
	}

void DLibrary::DoAppendName(TDes& aDes)
	{
	DObject::DoAppendName(aDes);
	TUint32 uid=iCodeSeg?iCodeSeg->iUids.iUid[2].iUid:0;
	TUint32 ver=iCodeSeg?iCodeSeg->iModuleVersion:0;
	aDes.Append('{');
	aDes.AppendNumFixedWidth(ver,EHex,8);
	aDes.Append('}');
	aDes.Append('[');
	aDes.AppendNumFixedWidth(uid,EHex,8);
	aDes.Append(']');
	}

TInt DLibrary::New(DLibrary*& aLib, DProcess* aProcess, DCodeSeg* aSeg)
	{
	aLib=new DLibrary;
	if (!aLib)
		return KErrNoMemory;
	TInt r=aLib->SetOwner(aProcess);
	if (r==KErrNone)
		{
		if (aSeg->iRootName.Length() > KMaxKernelName-KMaxUidName-KMaxVersionName)
			r = KErrBadName;
		else
			r=aLib->SetName(&aSeg->iRootName);
		}
	if (r==KErrNone)
		r=K::AddObject(aLib,ELibrary);
	if (r!=KErrNone)
		{
		aLib->Close(NULL);
		aLib=NULL;
		return r;
		}
	aLib->iCodeSeg=aSeg;
	return KErrNone;
	}


/******************************************************************************
 * Thread cleanup
 ******************************************************************************/
void DThread::RemoveClosingLibs()
	{
	__KTRACE_OPT(KDLL,Kern::Printf(">DThread::RemoveClosingLibs"));
	DCodeSeg::Wait();
	while(!iClosingLibs.IsEmpty())
		{
		DLibrary* pL=_LOFF(iClosingLibs.First()->Deque(), DLibrary, iThreadLink);
		pL->iThreadLink.iNext=NULL;
		__DEBUG_EVENT(EEventRemoveLibrary, pL);
		pL->RemoveFromProcess();
		pL->Close(NULL);
		}
	DCodeSeg::Signal();
	__KTRACE_OPT(KDLL,Kern::Printf("<DThread::RemoveClosingLibs"));
	}

/******************************************************************************
 * Executive Handlers - Loader
 ******************************************************************************/
DProcess* OpenProcess(TInt aHandle, TBool aAllowNull)
	{
	DProcess* pP=NULL;
	if (aHandle)
		{
		NKern::LockSystem();
		pP=(DProcess*)K::ThreadEnterCS(aHandle, EProcess);
		}
	else if (!aAllowNull)
		K::PanicKernExec(EBadHandle);
	else
		NKern::ThreadEnterCS();
	return pP;
	}

TInt ExecHandler::CodeSegCreate(TCodeSegCreateInfo& aInfo)
	{
	TCodeSegCreateInfo info;
	kumemget32(&info, &aInfo, sizeof(info));
	__KTRACE_OPT2(KEXEC,KDLL,Kern::Printf("Exec::CodeSegCreate %S ver %08x", &info.iFileName, info.iModuleVersion));
	__KTRACE_OPT(KDLL,DumpCodeSegCreateInfo(info));
	K::CheckFileServerAccess();			// only F32 can use this exec function
	info.iHandle=NULL;
	DProcess* pP=OpenProcess(info.iClientProcessHandle, ETrue);
	TInt r=KErrNoMemory;
	DCodeSeg* pS=M::NewCodeSeg(info);
	if (pS)
		{
		r=pS->Create(info, pP);
		if (r==KErrNone)
			info.iHandle=pS;
		else
			pS->Close();
		}
	if (pP)
		pP->Close(NULL);
	NKern::ThreadLeaveCS();
	kumemput32(&aInfo.iUids, &info.iUids, sizeof(info)-sizeof(info.iFileName));
	__KTRACE_OPT(KDLL,DumpCodeSegCreateInfo(info));
	__KTRACE_OPT2(KEXEC,KDLL,Kern::Printf("Exec::CodeSegCreate returns %d",r));
	return r;
	}

TInt ExecHandler::CodeSegLoaded(TCodeSegCreateInfo& aInfo)
	{
	TCodeSegCreateInfo info;
	kumemget32(&info, &aInfo, sizeof(info));
	__KTRACE_OPT2(KEXEC,KDLL,Kern::Printf("Exec::CodeSegLoaded %S ver %08x", &info.iFileName, info.iModuleVersion));
	__KTRACE_OPT(KDLL,DumpCodeSegCreateInfo(info));
	DCodeSeg* pS=DCodeSeg::VerifyCallerAndHandle(info.iHandle);
	Kern::AccessCode();
	TInt r=pS->Loaded(info);
	if (r==KErrNone)
		pS->iMark|=DCodeSeg::EMarkLoaded;
	Kern::EndAccessCode();
	kumemput32(&aInfo.iUids, &info.iUids, sizeof(info)-sizeof(info.iFileName));
	__KTRACE_OPT(KDLL,DumpCodeSegCreateInfo(info));
	__KTRACE_OPT2(KEXEC,KDLL,Kern::Printf("Exec::CodeSegLoaded returns %d",r));
	return r;
	}

TInt ExecHandler::LibraryCreate(TLibraryCreateInfo& aInfo)
	{
	__KTRACE_OPT2(KEXEC,KDLL,Kern::Printf("Exec::LibraryCreate"));
	TLibraryCreateInfo info;
	kumemget32(&info, &aInfo, sizeof(info));
	DCodeSeg* pS=DCodeSeg::VerifyCallerAndHandle(info.iCodeSegHandle);
	NKern::ThreadEnterCS();
	NKern::LockSystem();
	DThread* pT=(DThread*)K::ObjectFromHandle(info.iClientHandle,EThread);
	if (!pT || pT->Open())
		{
		NKern::ThreadLeaveCS();
		K::PanicCurrentThread(EBadHandle);
		}
	NKern::UnlockSystem();
	DProcess* pP=pT->iOwningProcess;
	DCodeSeg::Wait();
	DLibrary* pL=NULL;
	SCodeSegEntry find;
	find.iSeg=pS;
	find.iLib=NULL;
	TInt i;
	TInt r=pP->iDynamicCode.FindInUnsignedKeyOrder(find,i);
	if (r==KErrNone)
		{
		// this code segment is already explicitly in use by the process
		pL=pP->iDynamicCode[i].iLib;
		__KTRACE_OPT2(KEXEC,KDLL,Kern::Printf("Already loaded into process, lib=%O@%08x",pL,pL));
		r=pT->MakeHandleAndOpen(info.iOwnerType, pL, info.iLibraryHandle);
		if (r==KErrNone)
			{
			if (pL->iState==DLibrary::EDetachPending)
				{
				// load got in between Close and destructors running
				pL->iState=DLibrary::EAttached;
				}
			if (pL->iThreadLink.iNext)
				{
				pL->iThreadLink.Deque();
				pL->iThreadLink.iNext=NULL;
				pL->Close(NULL);
				}
			if (pL->iGbgLink.iNext)
				{
				pL->iGbgLink.Deque();
				pL->iGbgLink.iNext=NULL;
				pL->DObject::Close(NULL);
				}
			__DEBUG_EVENT2(EEventAddLibrary, pL, pT);
			}
		DCodeSeg::Signal();
		pT->Close(NULL);
		NKern::ThreadLeaveCS();
		kumemput32(&aInfo, &info, sizeof(info));
		__KTRACE_OPT2(KEXEC,KDLL,Kern::Printf("Exec::LibraryCreate returns %d",r));
		return r;
		}
	DCodeSeg::Signal();
	r=DLibrary::New(pL,pP,pS);
	if (r==KErrNone)
		{
		DCodeSeg::Wait();
		SDblQue cs_list;
		r = (pT->iExitType==EExitPending) ? pP->AddCodeSeg(pS,pL,cs_list) : KErrDied;
		if (r==KErrNone)
			{
			r=pT->MakeHandle(info.iOwnerType, pL, info.iLibraryHandle);
			if (r==KErrNone)
				{
				if (pS->iMark&DCodeSeg::EMarkDataInit)
					pL->iState=DLibrary::ELoaded;
				else
					pL->iState=DLibrary::EAttached;
				}
			else
				{
				DCodeSeg::EmptyQueue(cs_list, 0);	// unlink mapped DLLs, leave cs_list empty
				pL->RemoveFromProcess();
				}
			}
		DCodeSeg::EmptyQueue(cs_list, 0);	// unlink mapped DLLs, leave cs_list empty
		if (r!=KErrNone)
			{
			pL->iCodeSeg=NULL;
			pL->Close(NULL);
			}
		else
			__DEBUG_EVENT2(EEventAddLibrary, pL, pT);
		DCodeSeg::Signal();
		}
	pT->Close(NULL);
	NKern::ThreadLeaveCS();
	kumemput32(&aInfo, &info, sizeof(info));
	__KTRACE_OPT2(KEXEC,KDLL,Kern::Printf("Exec::LibraryCreate returns %d",r));
	return r;
	}

TInt ExecHandler::CodeSegOpen(TAny* aHandle, TInt aClientProcessHandle)
	{
	__KTRACE_OPT2(KEXEC,KDLL,Kern::Printf("Exec::CodeSegOpen"));
	DCodeSeg* pS=DCodeSeg::VerifyCallerAndHandle(aHandle);
	DProcess* pP=OpenProcess(aClientProcessHandle, ETrue);
	DCodeSeg::Wait();
	TInt r=KErrNone;
	if (pP && !pS->OpenCheck(pP))
		r=KErrNotSupported;
	if (r==KErrNone)
		pS->MarkAndOpenDeps(DCodeSeg::EMarkLdr);
	DCodeSeg::Signal();
	if (pP)
		pP->Close(NULL);
	NKern::ThreadLeaveCS();
	return r;
	}

void ExecHandler::CodeSegClose(TAny* aHandle)
	{
	__KTRACE_OPT2(KEXEC,KDLL,Kern::Printf("Exec::CodeSegClose %08x",aHandle));
	K::CheckFileServerAccess();			// only F32 can use this exec function
	DCodeSeg* pS=NULL;
	if (aHandle)
		pS=DCodeSeg::VerifyHandleP(aHandle);
	Kern::AccessCode();
	if (pS)
		pS->Close();
	else
		DCodeSeg::UnmarkAndCloseAll(DCodeSeg::EMarkLdr);
	Kern::EndAccessCode();
	}

void ExecHandler::CodeSegNext(TAny*& aHandle, const TFindCodeSeg& aFind)
	{
	__KTRACE_OPT2(KEXEC,KDLL,Kern::Printf(">Exec::CodeSegNext"));
	TFindCodeSeg find;
	kumemget32(&find, &aFind, sizeof(find));
	K::CheckFileServerAccess();			// only F32 can use this exec function
	DCodeSeg* pS;
	if (find.iRomImgHdr)
		pS=DCodeSeg::FindRomCode(find.iRomImgHdr);
	else
		{
		DProcess* pP=OpenProcess(find.iProcess, ETrue);
		DThread& t=*TheCurrentThread;
		t.iTempObj=pP;
		find.iProcess=(TInt)pP;
		NKern::ThreadLeaveCS();
		pS=DCodeSeg::FindCodeSeg(find);
		if (pP)
			{
			NKern::ThreadEnterCS();
			t.iTempObj=NULL;
			pP->Close(NULL);
			NKern::ThreadLeaveCS();
			}
		}
	kumemput32(&aHandle, &pS, sizeof(pS));
	__KTRACE_OPT2(KEXEC,KDLL,Kern::Printf("<Exec::CodeSegNext %08x",pS));
	}

void ExecHandler::CodeSegInfo(TAny* aHandle, TCodeSegCreateInfo& aInfo)
	{
	__KTRACE_OPT2(KEXEC,KDLL,Kern::Printf("Exec::CodeSegInfo %08x",aHandle));
	TCodeSegCreateInfo info;
	DCodeSeg* pS=DCodeSeg::VerifyCallerAndHandle(aHandle);
	Kern::AccessCode();
	pS->Info(info);
	info.iHandle = pS;
	Kern::EndAccessCode();
	kumemput32(&aInfo, &info, _FOFF(TCodeSegCreateInfo,iClientProcessHandle));
	__KTRACE_OPT(KDLL,DumpCodeSegCreateInfo(info));
	}

TInt ExecHandler::CodeSegAddDependency(TAny* aImporter, TAny* aExporter)
	{
	__KTRACE_OPT2(KEXEC,KDLL,Kern::Printf("Exec::CodeSegAddDependency %08x on %08x",aImporter,aExporter));
	DCodeSeg* pI=DCodeSeg::VerifyCallerAndHandle(aImporter);
	DCodeSeg* pE=DCodeSeg::VerifyHandleP(aExporter);
	Kern::AccessCode();
	TInt r=pI->AddDependency(pE);
	Kern::EndAccessCode();
	return r;
	}

void ExecHandler::CodeSegDeferDeletes()
	{
	__KTRACE_OPT2(KEXEC,KDLL,Kern::Printf("Exec::CodeSegDeferDeletes"));
	K::CheckFileServerAccess();			// only F32 can use this exec function
	Kern::AccessCode();
	DCodeSeg::DeferDeletes();
	Kern::EndAccessCode();
	}

void ExecHandler::CodeSegEndDeferDeletes()
	{
	__KTRACE_OPT2(KEXEC,KDLL,Kern::Printf("Exec::CodeSegEndDeferDeletes"));
	K::CheckFileServerAccess();			// only F32 can use this exec function
	Kern::AccessCode();
	DCodeSeg::EndDeferDeletes();
	Kern::EndAccessCode();
	}

TInt ExecHandler::ProcessCreate(TProcessCreateInfo& aInfo, const TDesC* aCommandLine)
	{
	TProcessCreateInfo info;
	kumemget32(&info, &aInfo, sizeof(info));
	__KTRACE_OPT2(KEXEC,KDLL,Kern::Printf("Exec::ProcessCreate %S ver %08x", &info.iFileName, info.iModuleVersion));
	__KTRACE_OPT(KDLL,DumpProcessCreateInfo(info));
	K::CheckFileServerAccess();			// only F32 can use this exec function
	if (info.iHandle)
		DCodeSeg::VerifyHandleP(info.iHandle);
	info.iProcessHandle=0;
	DThread& t=*TheCurrentThread;
	TInt cmdlen=0;
	if (aCommandLine)
		{
		TInt maxLen;
		Kern::KUDesInfo(*aCommandLine,cmdlen,maxLen);
		}
	NKern::ThreadEnterCS();
	HBuf* pCmd=NULL;
	if (cmdlen)
		pCmd=HBuf::New(cmdlen);
	t.iTempAlloc=pCmd;
	NKern::ThreadLeaveCS();				// we can die here but HBuf will be freed
	if (cmdlen)
		{
		if (!pCmd)
			return KErrNoMemory;
		Kern::KUDesGet(*pCmd,*aCommandLine);
		}
	NKern::ThreadEnterCS();
	t.iTempAlloc=NULL;					// no more panics allowed
	TMiscNotifier* n = 0;
	SMiscNotifierQ* q = 0;
	TInt r = KErrNone;
	if (info.iDestructStat)
		{
		n = new TMiscNotifier;
		q = new SMiscNotifierQ;
		if (!n || !q)
			r = KErrNoMemory;
		}
	DProcess* pP=NULL;
	if (r == KErrNone)
		r = Kern::ProcessCreate(pP, info, pCmd, &info.iProcessHandle);
	if (r == KErrNone && info.iDestructStat)
		{
		r = K::TheMiscNotifierMgr.NewMiscNotifier(info.iDestructStat, TRUE, pP, n, q);
		__NK_ASSERT_DEBUG(r >= 0); // can't fail
		r = KErrNone;
		}
	delete pCmd;						// in case P::NewProcess() failed
	delete q;
	if (n)
		n->Close();
	NKern::ThreadLeaveCS();
	kumemput32(&aInfo.iUids, &info.iUids, sizeof(info)-sizeof(info.iFileName));
	__KTRACE_OPT(KDLL,DumpProcessCreateInfo(info));
	__KTRACE_OPT2(KEXEC,KDLL,Kern::Printf("Exec::ProcessCreate returns %d",r));
	return r;
	}


/** Creates a user execute-in-place process.

	Creates and constructs a new DProcess object and optionally creates and
	opens a handle to it.  The caller must call DProcess::Loaded() to complete
	process initialisation.

	This function is only documented because it is used by the EXSTART kernel
	extension to start the secondary process (i.e. file server).  In general
	processes should be created by user-side code because this function
	bypasses the file server/loader.
	
	@param	aProcess	On successful return, contains a pointer to the newly
						created process.
	@param	aInfo		Information required to create the process.
	@param	aCommand	Pointer to heap-based descriptor containing the command
						line to pass to the process.  On return, set to NULL if
						and only if	ownership has been transferred to the newly
						created process. In other words, The caller must delete
						the descriptor if the call fails and the pointer is not
						set to NULL.
	@param	aHandle		Pointer to handle or NULL if no handle is to be created.
						If non NULL contains on successful return a handle to
						the newly created process.

	@return Standard error code.

	@pre Calling thread must be in a critical section.
	@pre Kernel must be unlocked
	@pre interrupts enabled
	@pre No fast mutex can be held
	@pre Call in a thread context
	
	@post Calling thread is in a critical section.

	@internalTechnology

	@See TProcessCreateInfo
 */
EXPORT_C TInt Kern::ProcessCreate(DProcess*& aProcess, TProcessCreateInfo& aInfo, HBuf*& aCommand, TInt* aHandle)
	{
	CHECK_PRECONDITIONS(MASK_THREAD_CRITICAL,"Kern::ProcessCreate");			
	TInt r=KErrNoMemory;
	aProcess=P::NewProcess();
	if (aProcess)
		{
		r=aProcess->Create(EFalse,aInfo,aCommand);
		__KTRACE_OPT(KPROC,Kern::Printf("DProcess::Create returns %d",r));
		aCommand=NULL;						// will be deleted by process destruct if an error occurs
		if (r==KErrNone && aHandle)
			r=K::MakeHandleAndOpen(EOwnerProcess,aProcess,*aHandle);
		if (r!=KErrNone)
			{
			if (!aProcess->iThreadQ.IsEmpty())
				{
				// must remove thread
				DThread* pT=_LOFF(aProcess->iThreadQ.First()->Deque(),DThread,iProcessLink);
				pT->iProcessLink.iNext=NULL;
				pT->Stillborn();
				}
			aProcess->Release();
			}
		}
	return r;
	}

TInt ExecHandler::ProcessLoaded(TProcessCreateInfo& aInfo)
	{
	TProcessCreateInfo info;
	kumemget32(&info, &aInfo, sizeof(info));
	__KTRACE_OPT2(KEXEC,KDLL,Kern::Printf("Exec::ProcessLoaded %S ver %08x", &info.iFileName, info.iModuleVersion));
	__KTRACE_OPT(KDLL,DumpProcessCreateInfo(info));
	K::CheckFileServerAccess();			// only F32 can use this exec function
	NKern::LockSystem();
	DProcess* pP=(DProcess*)K::ThreadEnterCS(info.iProcessHandle, EProcess);
	TInt r=pP->Loaded(info);
	info.iFinalHandle=0;
	if (r==KErrNone)
		{
		NKern::LockSystem();
		DThread* pT=(DThread*)K::ObjectFromHandle(info.iClientHandle,EThread);
		if (!pT || pT->Open())
			{
			NKern::ThreadLeaveCS();
			K::PanicCurrentThread(EBadHandle);
			}
		NKern::UnlockSystem();
		r=pT->MakeHandle(info.iOwnerType, pP, info.iFinalHandle);
		if (r==KErrNone)
			{
			DProcess* creator = pT->iOwningProcess;
			pP->iCreatorId = creator->iId;
			pP->iCreatorInfo = creator->iS;
			}
		pT->Close(NULL);
		}
	if (r!=KErrNone)
		pP->Close(NULL);
	NKern::ThreadLeaveCS();
	kumemput32(&aInfo.iUids, &info.iUids, sizeof(info)-sizeof(info.iFileName));
	__KTRACE_OPT(KDLL,DumpProcessCreateInfo(info));
	__KTRACE_OPT2(KEXEC,KDLL,Kern::Printf("Exec::ProcessLoaded returns %d",r));
	return r;
	}

TInt ExecHandler::CheckLoaderClientState(DThread* aClient)
	{
	__KTRACE_OPT2(KEXEC,KDLL,Kern::Printf("Exec::CheckLoaderClientState %O", aClient));
	DMutex* pM = aClient->iOwningProcess->iDllLock;
	if (pM && pM->iCleanup.iThread==aClient && aClient->iClosingLibs.IsEmpty())
		return KErrNone;
	return KErrGeneral;
	}

TAny* ExecHandler::ThreadProcessCodeSeg(DThread* aThread)
	{
	__KTRACE_OPT2(KEXEC,KDLL,Kern::Printf("Exec::ThreadProcessCodeSeg %O",aThread));
	DProcess& p=*aThread->iOwningProcess;
	TAny* h=p.CodeSeg();
	__KTRACE_OPT2(KEXEC,KDLL,Kern::Printf("Exec::ThreadProcessCodeSeg returns %08x",h));
	return h;
	}

void ExecHandler::CodeSegReadExportDir(TAny* aHandle, TUint32* aDest)
	{
	DCodeSeg* pS=DCodeSeg::VerifyCallerAndHandle(aHandle);
	__KTRACE_OPT2(KEXEC,KDLL,Kern::Printf("Exec::CodeSegReadExportDir %C", pS));
	pS->ReadExportDir(aDest);
	}

TLinAddr ExecHandler::ExceptionDescriptor(TLinAddr aCodeAddress)
	{
	__KTRACE_OPT2(KEXEC,KDLL,Kern::Printf(">Exec::ExceptionDescriptor %08x", aCodeAddress));
	TLinAddr xd = 0;
	Kern::AccessCode();
	DCodeSeg* s = Kern::CodeSegFromAddress(aCodeAddress, TheCurrentThread->iOwningProcess);
	if (s)
		xd = s->ExceptionDescriptor();
	Kern::EndAccessCode();
	__KTRACE_OPT2(KEXEC,KDLL,Kern::Printf("<Exec::ExceptionDescriptor %08x", xd));
	return xd;
	}

/**	Finds the code segment which contains a specified instruction address in a
	specified process address space.

	@param	aAddr		The address to search for.
	@param	aProcess	The process in which aAddr resides.
	
	@return	A pointer to the code segment if it exists;
	        NULL if no corresponding code segment could be found.

    @pre Calling thread must be in a critical section.
    @pre No fast mutex can be held.
	@pre Call in a thread context.
	@pre Code segment lock mutex held.
	@pre Kernel must be unlocked
	@pre interrupts enabled
	@pre Call in a thread context
 */
EXPORT_C DCodeSeg* Kern::CodeSegFromAddress(TLinAddr aAddr, DProcess* aProcess)
	{
	CHECK_PRECONDITIONS(MASK_THREAD_CRITICAL,"Kern::CodeSegFromAddress");
	__ASSERT_WITH_MESSAGE_MUTEX(DCodeSeg::CodeSegLock,"Code segment lock mutex held","Kern::CodeSegFromAddress");
	__KTRACE_OPT(KDLL,Kern::Printf(">Kern::CodeSegFromAddress %08x", aAddr));
	DCodeSeg* s = aProcess->iCodeSeg;
	if(!s || aAddr<s->iRunAddress || (aAddr-s->iRunAddress) >= s->iSize)
		{
		// if not in EXE's codeseg, then find in list...
		s = DCodeSeg::CodeSegsByAddress.Find(aAddr);
		}
	__KTRACE_OPT(KDLL,Kern::Printf("<Kern::CodeSegFromAddress %C", s));
	return s;
	}

/* Requests to be notified when something is added to the CodeSegLoaderCookie list.

   Called with the system locked.
*/

TInt ExecHandler::NotifyIfCodeSegDestroyed(TRequestStatus& aStatus)
	{
	K::CheckFileServerAccess();		// only F32 can use this exec function
	TInt r = DCodeSeg::DestructNotifyRequest->SetStatus(&aStatus);
	if (r == KErrNone)
		DCodeSeg::DestructNotifyThread = TheCurrentThread;
	return r;
	}



/*	Removes one CodeSeg from the cleanup list and returns it's cookie.
	The CodeSeg is then destroyed.
*/

TInt ExecHandler::GetDestroyedCodeSegInfo(TCodeSegLoaderCookie& aCookie)
	{
	TCodeSegLoaderCookieList* cookie;
	K::CheckFileServerAccess();			// only F32 can use this exec function
	NKern::ThreadEnterCS();
	DCodeSeg::Wait();
	if (DCodeSeg::DestructNotifyList==NULL)
		{
		DCodeSeg::Signal();
		NKern::ThreadLeaveCS();
		return KErrNotFound;
		}
		
	cookie=DCodeSeg::DestructNotifyList;
	DCodeSeg::DestructNotifyList=DCodeSeg::DestructNotifyList->iNext;
	DCodeSeg::Signal();
	kumemput(&aCookie,&cookie->iCookie, sizeof(TCodeSegLoaderCookie));
	delete  cookie;
	NKern::ThreadLeaveCS();
	return KErrNone;
	}
	

/******************************************************************************
 * Executive Handlers - Client
 ******************************************************************************/

// Acquire the process DLL lock just before a load
TInt ExecHandler::WaitDllLock()
	{
	__KTRACE_OPT2(KEXEC,KDLL,Kern::Printf("Exec::WaitDllLock"));
	DThread& t=*TheCurrentThread;
	DProcess& p=*t.iOwningProcess;
	__ASSERT_ALWAYS(t.iClosingLibs.IsEmpty(), K::PanicKernExec(EWaitDllLockInvalid));
	return p.WaitDllLock();
	}

// Release the process DLL lock if a load fails
TInt ExecHandler::ReleaseDllLock()
	{
	__KTRACE_OPT2(KEXEC,KDLL,Kern::Printf("Exec::ReleaseDllLock"));
	DThread& t=*TheCurrentThread;
	DProcess& p=*t.iOwningProcess;
	__ASSERT_ALWAYS(t.iClosingLibs.IsEmpty(), K::PanicKernExec(EReleaseDllLockInvalid));
	p.SignalDllLock();
	return KErrNone;
	}

// Accept a DLibrary just after a load and extract a list of entry points
TInt ExecHandler::LibraryAttach(TInt aHandle, TInt& aNumEps, TLinAddr* aEpList)
	{
	__KTRACE_OPT2(KEXEC,KDLL,Kern::Printf(">Exec::LibraryAttach"));
	DThread& t=*TheCurrentThread;
	DProcess& p=*t.iOwningProcess;
	TInt maxEps;
	TInt numEps=0;
	kumemget32(&maxEps, &aNumEps, sizeof(TInt));
	__ASSERT_ALWAYS(	p.iDllLock->iCleanup.iThread==&t && 
						t.iClosingLibs.IsEmpty() &&
						maxEps>0,
					K::PanicKernExec(ELibraryAttachInvalid));
	Kern::AccessCode();
	NKern::LockSystem();
	DLibrary* pL=(DLibrary*)t.ObjectFromHandle(aHandle,ELibrary);
	NKern::UnlockSystem();
	if (!pL)
		{
		Kern::EndAccessCode();
		K::PanicKernExec(EBadHandle);
		}
	DCodeSeg* pS=pL->iCodeSeg;
	TInt r=KErrNone;
	if (pL->iState!=DLibrary::EAttached)	// already loaded or no entry points required
		{
		SDblQue cs_list;
		numEps=p.CallList(cs_list, pS, DProcess::ETraverseFlagRestrict);
		if (numEps>maxEps)
			r=ETooManyEntryPoints;
		else if (numEps>0)
			{
			r=DCodeSeg::WriteCallList(cs_list, aEpList, ETrue);
			DCodeSeg::EmptyQueue(cs_list, 0);
			if (r==KErrNone)
				pL->iState=DLibrary::EAttaching;
			}
		else
			pL->iState=DLibrary::EAttached;
		}
	Kern::EndAccessCode();
	if (r>0 || numEps==0)
		p.SignalDllLock();
	if (r>0)
		K::PanicKernExec(r);
	kumemput32(&aNumEps,&numEps,sizeof(TInt));
	__KTRACE_OPT2(KEXEC,KDLL,Kern::Printf("<Exec::LibraryAttach numEps=%d r=%d",numEps,r));
	return r;
	}

// Signal that entry points have been completed and release DLL lock
TInt ExecHandler::LibraryAttached(TInt aHandle)
	{
	__KTRACE_OPT2(KEXEC,KDLL,Kern::Printf("Exec::LibraryAttached"));
	DThread& t=*TheCurrentThread;
	DProcess& p=*t.iOwningProcess;
	__ASSERT_ALWAYS(	p.iDllLock->iCleanup.iThread==&t && 
						t.iClosingLibs.IsEmpty(),
					K::PanicKernExec(ELibraryAttachedInvalid));
	TInt s;
	Kern::AccessCode();
	NKern::LockSystem();
	DLibrary* pL=(DLibrary*)t.ObjectFromHandle(aHandle,ELibrary);
	NKern::UnlockSystem();
	if (!pL)
		{
		Kern::EndAccessCode();
		K::PanicKernExec(EBadHandle);
		}
	s=pL->iState;
	if (s==DLibrary::EAttaching)
		pL->iState=DLibrary::EAttached;
	Kern::EndAccessCode();
	__ASSERT_ALWAYS(s==DLibrary::EAttaching, K::PanicKernExec(ELibraryAttachedInvalid));
	p.SignalDllLock();
	return KErrNone;
	}

// Extract a list of implicit DLL entry points for the current process
TInt ExecHandler::StaticCallList(TInt& aNumEps, TLinAddr* aEpList)
	{
	__KTRACE_OPT2(KEXEC,KDLL,Kern::Printf(">Exec::StaticCallList"));
	DThread& t=*TheCurrentThread;
	DProcess& p=*t.iOwningProcess;
	NKern::LockSystem();
	TBool initData=t.iFlags&KThreadFlagOriginal;
	if (initData)
		t.iFlags&=~KThreadFlagOriginal;
	NKern::UnlockSystem();
	TInt maxEps;
	TInt numEps=0;
	kumemget32(&maxEps, &aNumEps, sizeof(TInt));
	Kern::AccessCode();
	DCodeSeg* pS=p.iCodeSeg;
	SDblQue cs_list;
	DCodeSeg::UnmarkAll(DCodeSeg::EMarkListDeps|DCodeSeg::EMarkUnListDeps);
	numEps=pS->ListDeps(&cs_list, DCodeSeg::EMarkListDeps);
	TInt r=KErrNone;
	if (numEps>maxEps)
		r=ETooManyEntryPoints;
	else
		r=DCodeSeg::WriteCallList(cs_list, aEpList, initData);
	DCodeSeg::EmptyQueue(cs_list,0);
	Kern::EndAccessCode();
	if (r>0)
		K::PanicKernExec(r);
	kumemput32(&aNumEps,&numEps,sizeof(TInt));
	__KTRACE_OPT2(KEXEC,KDLL,Kern::Printf("<Exec::StaticCallList numEps=%d",numEps));
	return KErrNone;
	}

// Mark the process after implicit DLL entry points have been called
void ExecHandler::StaticCallsDone()
	{
	__KTRACE_OPT2(KEXEC,KDLL,Kern::Printf(">Exec::StaticCallsDone"));
	DThread& t=*TheCurrentThread;
	DProcess& p=*t.iOwningProcess;
	NKern::LockSystem();
	p.iAttributes|=DProcess::EStaticCallsDone;
	NKern::UnlockSystem();
	__KTRACE_OPT2(KEXEC,KDLL,Kern::Printf("<Exec::StaticCallsDone"));
	}

// Extract a list of DLL entry points for a closing DLL
TInt ExecHandler::LibraryDetach(TInt& aNumEps, TLinAddr* aEpList)
	{
	__KTRACE_OPT2(KEXEC,KDLL,Kern::Printf(">Exec::LibraryDetach"));
	DThread& t=*TheCurrentThread;
	DProcess& p=*t.iOwningProcess;
	TInt maxEps;
	TInt numEps=0;
	kumemget32(&maxEps, &aNumEps, sizeof(TInt));
	__ASSERT_ALWAYS(	p.iDllLock->iCleanup.iThread==&t && 
						maxEps>0,
					K::PanicKernExec(ELibraryDetachInvalid));
	Kern::AccessCode();
	TInt r=KErrEof;
	if (!t.iClosingLibs.IsEmpty())	// could be empty if a DLL is reloaded by another thread
		{
		DLibrary* pL=_LOFF(t.iClosingLibs.First(), DLibrary, iThreadLink);
		DCodeSeg* pS=pL->iCodeSeg;
		SDblQue cs_list;
		numEps=p.CallList(cs_list, pS, DProcess::ETraverseFlagRestrict);
		if (numEps>maxEps)
			r=ETooManyEntryPoints;
		else if (numEps>0)
			{
			r=DCodeSeg::WriteCallList(cs_list, aEpList, EFalse);
			DCodeSeg::EmptyQueue(cs_list, 0);
			pL->iState=DLibrary::EDetaching;
			}
		else
			{
			r=KErrNone;
			pL->iState=DLibrary::ELoaded;
			}
		DCodeSeg::EmptyQueue(cs_list,0);
		}
	Kern::EndAccessCode();
	if (r>0)
		K::PanicKernExec(r);
	if (r==KErrEof)
		p.SignalDllLock();
	kumemput32(&aNumEps,&numEps,sizeof(TInt));
	__KTRACE_OPT2(KEXEC,KDLL,Kern::Printf("<Exec::LibraryDetach numEps=%d r=%d",numEps,r));
	return r;
	}

// Signal that entry points have been completed and release DLL
TInt ExecHandler::LibraryDetached()
	{
	__KTRACE_OPT2(KEXEC,KDLL,Kern::Printf(">Exec::LibraryDetached"));
	DThread& t=*TheCurrentThread;
	DProcess& p=*t.iOwningProcess;
	__ASSERT_ALWAYS(	p.iDllLock->iCleanup.iThread==&t &&
						!t.iClosingLibs.IsEmpty(),
						K::PanicKernExec(ELibraryDetachedInvalid));
	TInt r=KErrNone;
	Kern::AccessCode();
	DLibrary* pL=_LOFF(t.iClosingLibs.First()->Deque(), DLibrary, iThreadLink);
	__DEBUG_EVENT(EEventRemoveLibrary, pL);
	pL->iThreadLink.iNext=NULL;
	pL->iState=DLibrary::ELoaded;
	pL->RemoveFromProcess();
	pL->Close(NULL);
	if (t.iClosingLibs.IsEmpty())
		r=KErrEof;
	Kern::EndAccessCode();
	if (r==KErrEof)
		p.SignalDllLock();
	__KTRACE_OPT2(KEXEC,KDLL,Kern::Printf("<Exec::LibraryDetached r=%d",r));
	return r;
	}

// Signal that User::Leave() is about to throw a C++ exception
// that will unwind the stack, potentially touching code that
// could be unloaded by code on the cleanup stack.  Therefore,
// delay unmapping code from this process whilst there are
// threads leaving.
TTrapHandler* ExecHandler::LeaveStart()
	{
	__KTRACE_OPT(KEXEC, Kern::Printf(">Exec::LeaveStart"));

#ifndef __LEAVE_EQUALS_THROW__

	InvalidExecHandler();
	__KTRACE_OPT(KEXEC, Kern::Printf("<Exec::LeaveStart"));
	return NULL;

#else //__LEAVE_EQUALS_THROW__

	DThread& thread = *TheCurrentThread;

	NKern::ThreadEnterCS();
	if (++thread.iLeaveDepth == 1)
		{
		__e32_atomic_add_ord32(&thread.iOwningProcess->iThreadsLeaving, 1);
		}
	NKern::ThreadLeaveCS();

	__KTRACE_OPT(KEXEC, Kern::Printf("<Exec::LeaveStart"));
	return thread.iTrapHandler;

#endif // !__LEAVE_EQUALS_THROW__

	}

// Signal that the C++ exception thrown by User::Leave() has been
// caught and that therefore this thread's stack has been unwound.
// Any code that is pending being unloaded and which could have
// been accessed whilst the stack was unwound may therefore be
// safely unloaded if all threads have completed leaving and this
// was not a recursive call to User::Leave().
void ExecHandler::LeaveEnd()
	{
	__KTRACE_OPT(KEXEC, Kern::Printf(">Exec::LeaveEnd"));

#ifndef __LEAVE_EQUALS_THROW__

	InvalidExecHandler();

#else //__LEAVE_EQUALS_THROW__

	DThread& thread = *TheCurrentThread;

	__ASSERT_ALWAYS(thread.iLeaveDepth > 0, InvalidExecHandler());

	NKern::ThreadEnterCS();
	thread.CleanupLeave();
	NKern::ThreadLeaveCS();

#endif // !__LEAVE_EQUALS_THROW__

	__KTRACE_OPT(KEXEC, Kern::Printf("<Exec::LeaveEnd"));
	}
