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
// f32\sfile\sf_dir.cpp
// 
//

#include "sf_std.h"

#ifdef OST_TRACE_COMPILER_IN_USE
#include "sf_dirTraces.h"
#endif

LOCAL_C CDirCB* GetDirFromHandle(TInt aHandle,CSessionFs* aSession)
//
// Get the dir control block from its handle.
//
	{
	return((CDirCB*)(SessionObjectFromHandle(aHandle,Dirs->UniqueID(),aSession)));
	}

LOCAL_C TInt DoInitialise(CFsRequest* aRequest)
//
//	Determine asynchronicity from dir control block
//
	{
	CDirCB* dir;
	dir=GetDirFromHandle(aRequest->Message().Int3(),aRequest->Session());
	if(!dir)
		return(KErrBadHandle);
	aRequest->SetDrive(&dir->Drive());
	aRequest->SetScratchValue((TUint)dir);
	return KErrNone;
	}

#ifndef __ARMCC__
LOCAL_C 
#endif
void fsDirReadPacked(TEntry* pE,TEntry* pEnd,volatile TInt& aLen,CDirCB& aDir)
//
// Read packed directory entries.
//
	{

	FOREVER
		{
		TEntry e;

		OstTrace1(TRACE_FILESYSTEM, FSYS_ECDIRCBREADL2, "fsDirReadPacked this %x", &aDir);
		aDir.ReadL(e);
		OstTraceExt5(TRACE_FILESYSTEM, FSYS_ECDIRCBREADL2RET, "fsDirReadPacked r %d att %x modified %x:%x  size %d", (TUint) KErrNone, (TUint) e.iAtt, (TUint) I64HIGH(e.iModified.Int64()), (TUint) I64LOW(e.iModified.Int64()), (TUint) e.iSize);
		TInt len=EntrySize(e, EFalse);
		TInt rLen=EntrySize(e, ETrue);
		TEntry* pX=PtrAdd(pE,rLen);
		if (pX>pEnd)
			{

			OstTrace1(TRACE_FILESYSTEM, FSYS_ECDIRCBSTORELONGENTRYNAMEL, "fsDirReadPacked this %x", &aDir);
			aDir.StoreLongEntryNameL(e.iName);
			OstTrace1(TRACE_FILESYSTEM, FSYS_ECDIRCBSTORELONGENTRYNAMELRET, "fsDirReadPacked r %d", KErrNone);

			aDir.SetPending(ETrue);
			break;
			}
		aLen+=rLen;
		Mem::Copy(pE,&e,len);

		/**
		 * Flag the entry with KEntryAttPacked so we can unpack
		 * these fields as required at a later date...
		 */
		pE->iAtt |= KEntryAttPacked;

		/**
		 * ...and pack the iSizeHigh and iReserved fields to the end of the name string
		 */
		TUint32* pSizeHighSrc = PtrAdd((TUint32*)&e, sizeof(TEntry) - 2*sizeof(TInt));
		TUint32* pSizeHighDst = PtrAdd((TUint32*)pE, EntrySize(*pE, EFalse));

		*pSizeHighDst++ = *pSizeHighSrc++;	// Copy length
		*pSizeHighDst   = *pSizeHighSrc;	// Copy reserved

		pE=pX;
		}
	}

TInt TFsDirOpen::DoRequestL(CFsRequest* aRequest)
//
// Open a directory.
//
	{

	__PRINT(_L("TFsDirOpen::DoRequestL(CFsRequest* aRequest)"));
	TInt h;
	TUidType uidType;
	TPckgBuf<TUidType> pckgUid;
	aRequest->ReadL(KMsgPtr2,pckgUid);
	uidType=pckgUid();
	TInt r=aRequest->Drive()->DirOpen(aRequest->Session(),h,aRequest->Src().FullName().Mid(2),aRequest->Message().Int1(),uidType);
	if (r!=KErrNone)
		return(r);

	
	//DirRead does not have a filename / src stored, so if there are plugins installed which
	//wish to intercept dirread1/packed then store the name in CDirCB::iName now.
	CFsPlugin* plugin = NULL;
	//Get the next plugin which is mounted on this drive (IsMounted called in NextPlugin)
	//Do not check whether we're registered for current operation (in case not registered for EFsDirOpen)
	FsPluginManager::ReadLockChain();                                      //!Check operation
	while(FsPluginManager::NextPlugin(plugin,(CFsMessageRequest*)aRequest,(TBool)EFalse)==KErrNone && plugin)
		{
		if(plugin->IsRegistered(EFsDirReadOne) ||
			plugin->IsRegistered(EFsDirReadPacked) ||
			plugin->IsRegistered(EFsDirSubClose))
			{
			CDirCB* dir = GetDirFromHandle(h,aRequest->Session());
			TPtrC name = aRequest->Src().FullName();
			r = dir->SetName(&name);
			CheckForLeaveAfterOpenL(r, aRequest, h);
			break;
			}
		}
	FsPluginManager::UnlockChain();
	
	TPtrC8 pH((TUint8*)&h,sizeof(TInt));
	TRAP(r,aRequest->WriteL(KMsgPtr3,pH))
	CheckForLeaveAfterOpenL(r, aRequest, h);
	aRequest->Session()->IncResourceCount();
	return(KErrNone);
	}


TInt TFsDirOpen::Initialise(CFsRequest* aRequest)
//
//
//	
	{
	
	TInt r=ParseSubstPtr0(aRequest,aRequest->Src());
	if (r!=KErrNone)
		return(r);
	r=PathCheck(aRequest->Message(),aRequest->Src().FullName().Mid(2),&KCapFsSysDirOpen,&KCapFsPriDirOpen, __PLATSEC_DIAGNOSTIC_STRING("Dir Open"));
	if(r != KErrNone)
		return r;
	return KErrNone;
	}


TInt TFsDirReadOne::DoRequestL(CFsRequest* aRequest)
//
// Read one directory entry.
//
	{
	__PRINT(_L("TFsDirReadOne::DoRequestL(CFsRequest* aRequest)"));
	CDirCB* dir=(CDirCB*)aRequest->ScratchValue();
	TInt r=dir->CheckMount();
	if (r!=KErrNone)
		return(r);
	TEntry e;

	OstTrace1(TRACE_FILESYSTEM, FSYS_ECDIRCBREADL1, "this %x", &dir);
	TRAP(r,dir->ReadL(e))
	OstTraceExt5(TRACE_FILESYSTEM, FSYS_ECDIRCBREADL1RET, "r %d att %x modified %x:%x  size %d", (TUint) KErrNone, (TUint) e.iAtt, (TUint) I64HIGH(e.iModified.Int64()), (TUint) I64LOW(e.iModified.Int64()), (TUint) e.iSize);
		


	if (r==KErrNone)
		{
		TPckgC<TEntry> pE(e);
		aRequest->WriteL(KMsgPtr0,pE);
		}
	return(r);
	}

TInt TFsDirReadOne::Initialise(CFsRequest* aRequest)
//
//
//	
	{
	return(DoInitialise(aRequest));
	}

TInt TFsDirReadPacked::DoRequestL(CFsRequest* aRequest)
//
// Read packed directory entries.
//
	{

	__PRINT(_L("TFsDirReadPacked::DoRequestL(CFsRequest* aRequest)"));
	CDirCB* dir=(CDirCB*)aRequest->ScratchValue();
	TInt r=dir->CheckMount();
	if (r!=KErrNone)
		return(r);

	TBuf8<KEntryArraySize> buf;
	TEntry* pE=(TEntry*)buf.Ptr();
	TEntry* pEnd=PtrAdd(pE,KEntryArraySize);
	volatile TInt len=0;
	TRAP(r,fsDirReadPacked(pE,pEnd,len,*(dir)));
	buf.SetLength(len);
	aRequest->WriteL(KMsgPtr0,buf);
	return(r);
	}

TInt TFsDirReadPacked::Initialise(CFsRequest* aRequest)
//
//	Call GetDirFromHandle to determine asynchronicity ***
//	
	{
	return(DoInitialise(aRequest));
	}




/**
Default cosntructor.
*/
EXPORT_C CDirCB::CDirCB()
	{

//	iPending=EFalse;
//	iDrive=NULL;
//	iMount=NULL;
	}




/**
Destructor.

Frees resources before destruction of the object.
*/
EXPORT_C CDirCB::~CDirCB()
	{
	if(iMount)
		{
		RemoveResource(*iMount);
		iMount->Close();
		}
	}

TInt CDirCB::CheckMount()
//
// Check that the media is still mounted.
//
	{

	TDrive& d=Drive();
	TInt r=d.CheckMount();
	if (r!=KErrNone)
		return(r);
	if (&Mount()!=&d.CurrentMount())
		return(KErrDisMounted);
	return(KErrNone);
	}

EXPORT_C void CDirCB::InitL(TDrive* aDrive)
//
// Initialise
//
	{
	DoInitL(aDrive->DriveNumber());
	iDrive=aDrive;
	iMount=&aDrive->CurrentMount();
	User::LeaveIfError(iMount->Open());
	}




/**
Stores a long full entry name, a TEntry::iName value, into a buffer,
re-allocating the buffer first, if necessary, to make it large enough.

The function should be implemented by a derived class; this implementation
is empty.

This function is called by a file server reading successive entries,
when the file server reads an entry for which the full file name is longer than
the maximum buffer size. Once this function has been called, the iPending flag
is set to true by the file server and, on the next call to ReadL(),
the buffer created in this function is used to store the long entry name.

The function should leave with an appropriate error code on error detection.

@param aName The name to be set to a newly read entry.
*/
EXPORT_C void CDirCB::StoreLongEntryNameL(const TDesC& /*aName*/)
	{}

	
EXPORT_C TInt CDirCB::GetInterface(TInt /*aInterfaceId*/,TAny*& /*aInterface*/,TAny* /*aInput*/)
	{
	return(KErrNotSupported);
	}
	
