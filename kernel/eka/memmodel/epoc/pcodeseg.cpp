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
// e32\memmodel\epoc\pcodeseg.cpp
// 
//

#include "plat_priv.h"
#include <e32uid.h>

TLinAddr DCodeSeg::ExceptionDescriptor()
	{
	DEpocCodeSeg* s = (DEpocCodeSeg*)this;
	if (s->iXIP)
		return s->RomInfo().iExceptionDescriptor;
	return s->RamInfo().iExceptionDescriptor;
	}

DEpocCodeSeg::~DEpocCodeSeg()
	{
	if (!iXIP && iMemory)
		{
		iMemory->iCodeSeg = 0;
		iMemory->Close();
		iMemory = NULL;
		}
	}

void DEpocCodeSeg::Destruct()
	{
	DCodeSeg::Wait();
	
	if (iLoaderCookie!=NULL)
		{
		if ((TInt) iLoaderCookie&1) // Not set fully loaded
			{
			iLoaderCookie = (TCodeSegLoaderCookieList*) ( ((TInt) iLoaderCookie) & ~1);
			delete iLoaderCookie;
			iLoaderCookie=NULL;
			}
		else
			{
			// Notify reaper of destruction
			NKern::LockSystem();
			if (DestructNotifyRequest->IsReady())
				{
				DThread* t = DestructNotifyThread;
				DestructNotifyThread = NULL;				
				Kern::QueueRequestComplete(t, DestructNotifyRequest,KErrNone);
				}
			NKern::UnlockSystem();
	
			iLoaderCookie->iNext=DestructNotifyList;
			DestructNotifyList=iLoaderCookie;
			iLoaderCookie=NULL;
			}
		}

	DCodeSeg::Signal();
	DCodeSeg::Destruct();
	}

void DEpocCodeSeg::Info(TCodeSegCreateInfo& aInfo)
	{
	CHECK_PAGING_SAFE;
	if (iXIP)
		{
		const TRomImageHeader& rih=RomInfo();
		aInfo.iCodeSize=rih.iCodeSize;
		aInfo.iTextSize=rih.iTextSize;
		aInfo.iDataSize=rih.iDataSize;
		aInfo.iBssSize=rih.iBssSize;
		aInfo.iTotalDataSize=rih.iTotalDataSize;
		aInfo.iExportDir=rih.iExportDir;
		aInfo.iExportDirCount=rih.iExportDirCount;
		aInfo.iCodeLoadAddress=rih.iCodeAddress;
		aInfo.iCodeRunAddress=rih.iCodeAddress;
		aInfo.iDataLoadAddress=rih.iDataAddress;
		aInfo.iDataRunAddress=rih.iDataBssLinearBase;
		aInfo.iExceptionDescriptor = rih.iExceptionDescriptor;
		}
	else
		{
		const SRamCodeInfo& ri=RamInfo();
		aInfo.iCodeSize=ri.iCodeSize;
		aInfo.iTextSize=ri.iTextSize;
		aInfo.iDataSize=ri.iDataSize;
		aInfo.iBssSize=ri.iBssSize;
		aInfo.iTotalDataSize=ri.iDataSize+ri.iBssSize;
		aInfo.iExportDir=ri.iExportDir;
		aInfo.iExportDirCount=ri.iExportDirCount;
		aInfo.iCodeLoadAddress=ri.iCodeLoadAddr;
		aInfo.iCodeRunAddress=ri.iCodeRunAddr;
		aInfo.iDataLoadAddress=ri.iDataLoadAddr;
		aInfo.iDataRunAddress=ri.iDataRunAddr;
		aInfo.iExceptionDescriptor = ri.iExceptionDescriptor;
		}
	DCodeSeg::Info(aInfo);
	}

void DEpocCodeSeg::GetDataSizeAndBase(TInt& aTotalDataSizeOut, TLinAddr& aDataBaseOut)
	{
	if (iXIP)
		{
		aTotalDataSizeOut=RomInfo().iTotalDataSize;
		aDataBaseOut=RomInfo().iDataBssLinearBase;
		}
	else
		{
		aTotalDataSizeOut=RamInfo().iDataSize+RamInfo().iBssSize;
		aDataBaseOut=RamInfo().iDataRunAddr;
		}	
	}

TLibraryFunction DEpocCodeSeg::Lookup(TInt aOrdinal)
	{
	CHECK_PAGING_SAFE;
	// return NULL for -ve ordinal or 0th ordinal when not stddll/stdexe
	if (aOrdinal<0 || (aOrdinal==0 && !(iAttr&ECodeSegAttNmdExpData)))
		return NULL;
	TInt xdc;
	TLinAddr* xd;	// points to the 0th ordinal of export table
	if (iXIP)
		{
		const TRomImageHeader& rih=RomInfo();
		xdc=rih.iExportDirCount;
		xd=(TLinAddr*)rih.iExportDir - 1;
		}
	else
		{
		SRamCodeInfo& ri=RamInfo();
		xdc=ri.iExportDirCount;
		xd=(TLinAddr*)ri.iExportDir - 1;
		}
	if (aOrdinal <= xdc)
		{
		UNLOCK_USER_MEMORY();
		TLibraryFunction f=(TLibraryFunction)xd[aOrdinal];
		LOCK_USER_MEMORY();
		if ((TLinAddr)f == iFileEntryPoint)
			f = NULL;
		__KTRACE_OPT(KDLL,Kern::Printf("Lookup(%d)->%08x",aOrdinal,f));
		return f;
		}
	__KTRACE_OPT(KDLL,Kern::Printf("Lookup(%d)->NULL",aOrdinal));
	return NULL;
	}

TInt DEpocCodeSeg::GetMemoryInfo(TModuleMemoryInfo& aInfo, DProcess*)
	{
	CHECK_PAGING_SAFE;
	if (iXIP)
		{
		const TRomImageHeader& rih		= RomInfo();
		aInfo.iCodeBase					= rih.iCodeAddress;
		aInfo.iCodeSize					= rih.iTextSize;
		aInfo.iConstDataSize			= rih.iCodeSize - aInfo.iCodeSize;
		aInfo.iConstDataBase			= (aInfo.iConstDataSize > 0) ? (aInfo.iCodeBase + aInfo.iCodeSize) : 0;
		aInfo.iInitialisedDataBase		= rih.iDataBssLinearBase;
		aInfo.iInitialisedDataSize		= rih.iDataSize;
		aInfo.iUninitialisedDataSize	= rih.iBssSize;
		aInfo.iUninitialisedDataBase	= aInfo.iInitialisedDataBase+aInfo.iInitialisedDataSize;
		}
	else
		{
		const SRamCodeInfo& ri			= RamInfo();
		aInfo.iCodeBase					= ri.iCodeRunAddr;
		aInfo.iCodeSize					= ri.iTextSize;
		aInfo.iConstDataSize			= ri.iCodeSize - aInfo.iCodeSize;
		aInfo.iConstDataBase			= (aInfo.iConstDataSize > 0) ? (aInfo.iCodeBase + aInfo.iCodeSize) : 0;
		aInfo.iInitialisedDataBase		= ri.iDataRunAddr;
		aInfo.iInitialisedDataSize		= ri.iDataSize;
		aInfo.iUninitialisedDataSize	= ri.iBssSize;
		aInfo.iUninitialisedDataBase	= aInfo.iInitialisedDataBase + aInfo.iInitialisedDataSize;
		}
	return KErrNone;
	}

//const TUint32 KRomImageFlagsAlwaysLoaded=KRomImageFlagPrimary|KRomImageFlagVariant|KRomImageFlagExtension;
const TUint32 KRomImageDataFlags=(KRomImageFlagData|KRomImageFlagDataInit|KRomImageFlagDataPresent);
TInt DEpocCodeSeg::DoCreate(TCodeSegCreateInfo& aInfo, DProcess* aProcess)
	{
	__KTRACE_OPT(KDLL,Kern::Printf(">DEpocCodeSeg::DoCreate code_addr=%08x",aInfo.iCodeLoadAddress));
	CHECK_PAGING_SAFE;
	TInt r=KErrNone;
	TBool exeSeg=(iExeCodeSeg==this);
	TBool kp=exeSeg && (aProcess->iAttributes&DProcess::ESupervisor);
	TBool user_proc=exeSeg && !kp;
	if (aInfo.iCodeLoadAddress)
		{
		iXIP=ETrue;
		iInfo=(const TAny*)aInfo.iCodeLoadAddress;
		}
	if (user_proc)
		{
		r=aProcess->CreateDataBssStackArea((TProcessCreateInfo&)aInfo);
		if (r!=KErrNone)
			return r;
		}
	if (iXIP)
		{
		const TRomImageHeader& rih=RomInfo();
		iXIP=ETrue;
		if ( (rih.iFlags & (KRomImageFlagExeInTree|KRomImageFlagDll)) == (KRomImageFlagExeInTree|KRomImageFlagDll))
			{
			const TRomImageHeader* exeRIH=rih.iDllRefTable->iEntry[0];	// EXE is always first entry
			if (aProcess)
				{
				DEpocCodeSeg* pS=(DEpocCodeSeg*)aProcess->CodeSeg();
				if (!pS->iXIP || &pS->RomInfo()!=exeRIH)
					return KErrNotSupported;
				iExeCodeSeg=pS;
				}
			}

		iMark |= (rih.iFlags & KRomImageDataFlags);
		iMark |= EMarkRecursiveFlagsValid;
		__KTRACE_OPT(KDLL,Kern::Printf("ROM Code Seg: mark %08x attr=%02x",iMark,iAttr));
		aInfo.iFileEntryPoint=rih.iEntryPoint;
		aInfo.iExportDir=rih.iExportDir;
		aInfo.iCodeLoadAddress=rih.iCodeAddress;
		aInfo.iCodeRunAddress=rih.iCodeAddress;
		iRunAddress = rih.iCodeAddress;
		iSize = rih.iCodeSize;
		aInfo.iDataLoadAddress=rih.iDataAddress;
		if (aInfo.iTotalDataSize && user_proc && aProcess->iDataBssRunAddress!=rih.iDataBssLinearBase)
			K::Fault(K::EProcessDataAddressInvalid);
		aInfo.iDataRunAddress=rih.iDataBssLinearBase;
		aInfo.iExceptionDescriptor = rih.iExceptionDescriptor;
		r=DoCreateXIP(aProcess);
		return r;
		}
	iMemory = DEpocCodeSegMemory::New(this);
	if (!iMemory)
		return KErrNoMemory;
	iInfo = &iMemory->iRamInfo;

	if (aInfo.iUseCodePaging)
		{
		if ((aInfo.iCodeBlockMapEntries==NULL) || (aInfo.iCodeBlockMapCommon.iLocalDriveNumber<0) ||
		(((TInt64) (aInfo.iFileClamp.iCookie[0] | aInfo.iFileClamp.iCookie[1]))==0))
				return KErrArgument;
			
		iLoaderCookie = new TCodeSegLoaderCookieList();
		if (!iLoaderCookie)
			return KErrNoMemory;
				
		SBlockMapInfoBase* cbm = &aInfo.iCodeBlockMapCommon;

		TUint startBlock;
		kumemget32(&startBlock, &aInfo.iCodeBlockMapEntries->iStartBlock, sizeof(TUint));

		iLoaderCookie->iCookie.iDriveNumber = cbm->iLocalDriveNumber;
		iLoaderCookie->iCookie.iStartAddress =	cbm->iStartBlockAddress +
												startBlock *
												cbm->iBlockGranularity +
												cbm->iBlockStartOffset;
		iLoaderCookie->iCookie.iFileClamp = aInfo.iFileClamp;
		// Mark cookie not to be closed yet
		iLoaderCookie = (TCodeSegLoaderCookieList*) ( ((TInt) iLoaderCookie) | 1);
		}
		
	SRamCodeInfo& ri=RamInfo();
	ri.iCodeSize=aInfo.iCodeSize;
	ri.iTextSize=aInfo.iTextSize;
	ri.iDataSize=aInfo.iDataSize;
	ri.iBssSize=aInfo.iBssSize;
	TInt total_data_size=ri.iDataSize+ri.iBssSize;
	ri.iExportDirCount=aInfo.iExportDirCount;
	if (total_data_size!=0)
		{
		iMark|=(EMarkData|EMarkDataPresent);
		if (!exeSeg)
			iMark|=EMarkDataInit;
		}
	// if this DLL/EXE doesn't have data, don't set valid bit since we don't yet know about its dependencies

	if (exeSeg)
		ri.iDataRunAddr=aProcess->iDataBssRunAddress;
	r=DoCreateRam(aInfo, aProcess);
	if (r==KErrNone)
		{
		aInfo.iFileEntryPoint+=ri.iCodeRunAddr;
		aInfo.iExportDir+=ri.iCodeRunAddr;
		ri.iExportDir=aInfo.iExportDir;
		aInfo.iCodeLoadAddress=ri.iCodeLoadAddr;
		aInfo.iCodeRunAddress=ri.iCodeRunAddr;
		aInfo.iDataLoadAddress=ri.iDataLoadAddr;
		aInfo.iDataRunAddress=ri.iDataRunAddr;
		TUint32 xd = aInfo.iExceptionDescriptor;
		ri.iExceptionDescriptor = xd ? (xd + ri.iCodeRunAddr) : 0;
		aInfo.iExceptionDescriptor = ri.iExceptionDescriptor;
		iRunAddress = ri.iCodeRunAddr;
		}
	__KTRACE_OPT(KDLL,Kern::Printf("RAM Code Seg: mark %08x attr=%02x xd=%08x",iMark,iAttr,ri.iExceptionDescriptor));
	__KTRACE_OPT(KDLL,Kern::Printf("<DEpocCodeSeg::DoCreate %d",r));
	return r;
	}

void DEpocCodeSeg::InitData()
	{
	__KTRACE_OPT(KDLL,Kern::Printf("DEpocCodeSeg::InitData %C", this));
	CHECK_PAGING_SAFE;
	TInt bss_size=0;
	TInt data_size=0;
	TUint8* data_run_ptr=0;
	const TUint8* data_load_ptr=0;
	if (iXIP)
		{
		const TRomImageHeader& rih=RomInfo();
		bss_size=rih.iBssSize;
		data_size=rih.iDataSize;
		data_run_ptr=(TUint8*)rih.iDataBssLinearBase;
		data_load_ptr=(const TUint8*)rih.iDataAddress;
		}
	else
		{
		SRamCodeInfo& ri=RamInfo();
		bss_size=ri.iBssSize;
		data_size=ri.iDataSize;
		data_run_ptr=(TUint8*)ri.iDataRunAddr;
		data_load_ptr=(const TUint8*)ri.iDataLoadAddr;
		}
	UNLOCK_USER_MEMORY();
	if (data_size)
		memcpy(data_run_ptr, data_load_ptr, data_size);
	if (bss_size)
		memclr(data_run_ptr+data_size, bss_size);
	LOCK_USER_MEMORY();
	}

DCodeSeg* DCodeSeg::FindRomCode(const TAny* aRomImgHdr)
	{
	CHECK_PAGING_SAFE;
	const TRomImageHeader& rih = *(const TRomImageHeader*)aRomImgHdr;
	TLinAddr ca = rih.iCodeAddress;
	return CodeSegsByAddress.Find(ca);
	}

void P::NormalizeExecutableFileName(TDes& /*aFileName*/)
	{
	}
	
TInt DEpocCodeSeg::Loaded(TCodeSegCreateInfo& aInfo)
	{
	iLoaderCookie = (TCodeSegLoaderCookieList*) ( ((TInt) iLoaderCookie) & ~1);
	return DCodeSeg::Loaded(aInfo);
	}


//
// DEpocCodeSegMemory
//

DEpocCodeSegMemory::DEpocCodeSegMemory(DEpocCodeSeg* aCodeSeg)
	: iAccessCount(1), iCodeSeg(aCodeSeg)
	{
	}


TInt DEpocCodeSegMemory::Open()
	{
	return __e32_atomic_tas_ord32(&iAccessCount, 1, 1, 0) ? KErrNone : KErrGeneral;
	}


TInt DEpocCodeSegMemory::Close()
	{
	if (__e32_atomic_tas_ord32(&iAccessCount, 1, -1, 0) == 1)
		{
		AsyncDelete();
		return DObject::EObjectDeleted;
		}
	return 0;
	}


