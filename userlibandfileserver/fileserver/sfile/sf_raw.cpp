// Copyright (c) 1996-2009 Nokia Corporation and/or its subsidiary(-ies).
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
// f32\sfile\sf_raw.cpp
// 
//

#include "sf_std.h"
#include "sf_file_cache.h"

#ifdef OST_TRACE_COMPILER_IN_USE
#include "sf_rawTraces.h"
#endif

LOCAL_C CRawDiskCB* GetRawDiskFromHandle(TInt aHandle, CSessionFs* aSession)
//
// Get the rawdisk control block from aHandle
//
	{
	return((CRawDiskCB*)(SessionObjectFromHandle(aHandle,RawDisks->UniqueID(),aSession)));
	}

LOCAL_C void DoFsRawDiskOpenL(TInt& aHandle,CRawDiskCB*& aRawDisk, CFsRequest* aRequest)
//
// Do the bits that can leave
//
	{

	aRawDisk=new(ELeave) CRawDiskCB;
	RawDisks->AddL(aRawDisk,ETrue);
	aRawDisk->InitL(&aRequest->Drive()->CurrentMount(),aRequest->Drive()->IsWriteProtected());
	// modify resource counter after initialisation to ensure correct cleanup in object destruction
	AddDiskAccess(aRequest->Drive()->CurrentMount());
	aHandle=aRequest->Session()->Handles().AddL(aRawDisk,ETrue);
	}

TInt TFsRawDiskOpen::DoRequestL(CFsRequest* aRequest)
//
// Open direct disk access channel
//
	{
	__PRINT(_L("TFsRawDiskOpen::DoRequestL(CFsRequest* aRequest)"));
		
	TInt r=aRequest->Drive()->CheckMount();
	if (r==KErrNone && aRequest->Drive()->CurrentMount().LockStatus()<0)
		return(KErrInUse);
	if (r!=KErrNone)
		return(r);

	__ASSERT_DEBUG( aRequest->Drive()->CurrentMount().LockStatus()==0,Fault(ESvrRawDiskOpenFailed));

	TInt handle;
	CRawDiskCB* rawDisk=NULL;
	TRAPD(ret,DoFsRawDiskOpenL(handle,rawDisk,aRequest));
	if (ret!=KErrNone)
		{
		if(rawDisk)
			rawDisk->Close();
		return(ret);
		}

	r = rawDisk->Drive().FlushCachedFileInfo();
	if (r != KErrNone)
		return(r);

	// Empty the closed file container
	TClosedFileUtils::Remove();

	TPtrC8 pH((TUint8*)&handle,sizeof(TInt));
	aRequest->WriteL(KMsgPtr3,pH);
	aRequest->Session()->IncResourceCount();
	return(KErrNone);
	}

TInt TFsRawDiskOpen::Initialise(CFsRequest* aRequest)
//	
//
	{
	if (!KCapFsRawDiskOpen.CheckPolicy(aRequest->Message(), __PLATSEC_DIAGNOSTIC_STRING("Raw Disk Open")))
		return KErrPermissionDenied;
	TInt driveNo=aRequest->Message().Int0();
	return ValidateDriveDoSubst(driveNo,aRequest);
	}


TInt TFsRawDiskRead::DoRequestL(CFsRequest* aRequest)
//
// Do Raw Read
//
	{
	__PRINT(_L("TFsRawDiskRead::DoRequestL(CFsRequest* aRequest)"));
	
	CRawDiskCB* rawDisk=(CRawDiskCB*)aRequest->ScratchValue();

	TInt r=rawDisk->Drive().CheckMount();
	if (r!=KErrNone && r!=KErrInUse)
		return(r);
	if (&rawDisk->Mount()!=&rawDisk->Drive().CurrentMount())
		return(KErrDisMounted);
	TInt64 pos;
	TPtr8 tBuf((TUint8*)&pos,sizeof(TInt64));
	aRequest->ReadL(KMsgPtr2,tBuf);
	if ( pos < 0 )
		return(KErrArgument);
	TInt length=aRequest->Message().Int1();
	const TAny* pDes=aRequest->Message().Ptr0();

	TRACETHREADID(aRequest->Message());
	OstTraceExt5(TRACE_FILESYSTEM, FSYS_ECMOUNTCBRAWREADL, "drive %d clientThreadId %x aPos %x:%x aLength %d", (TUint) aRequest->DriveNumber(), (TUint) threadId, (TUint) I64HIGH(pos), (TUint) I64LOW(pos), (TUint) length);

	TRAP(r,rawDisk->Mount().RawReadL(pos,length,pDes,0,aRequest->Message()));

	OstTrace1(TRACE_FILESYSTEM, FSYS_ECMOUNTCBRAWREADLRET, "r %d", r);

	return(r);
	}

TInt TFsRawDiskRead::Initialise(CFsRequest* aRequest)
//
//
//
	{
	if (!KCapFsRawDiskRead.CheckPolicy(aRequest->Message(), __PLATSEC_DIAGNOSTIC_STRING("Raw Disk Read")))
		return KErrPermissionDenied;
	CRawDiskCB* rawDisk=GetRawDiskFromHandle(aRequest->Message().Int3(),aRequest->Session());
	if(!rawDisk)
		return(KErrBadHandle);
	aRequest->SetDrive(&rawDisk->Drive());
	aRequest->SetScratchValue((TUint)rawDisk);
	return KErrNone;
	}


TInt TFsRawDiskWrite::DoRequestL(CFsRequest* aRequest)
//
// Do Raw write
//
	{
	__PRINT(_L("TFsRawDiskWrite::DoRequestL(CFsRequest* aRequest)"));
	
	CRawDiskCB* rawDisk=(CRawDiskCB*)aRequest->ScratchValue();
	
	TInt r=rawDisk->Drive().CheckMount();
	if (r!=KErrNone && r!=KErrInUse)
		return(r);
	if (&rawDisk->Mount()!=&rawDisk->Drive().CurrentMount())
		return(KErrDisMounted);
	TInt64 pos;
	TPtr8 tBuf((TUint8*)&pos,sizeof(TInt64));
	aRequest->ReadL(KMsgPtr2,tBuf);
	if ( pos < 0 )
		return(KErrArgument);
	TInt length=aRequest->Message().Int1();
	const TAny* pDes=aRequest->Message().Ptr0();

	TRACETHREADID(aRequest->Message());
	OstTraceExt5(TRACE_FILESYSTEM, FSYS_ECMOUNTCBRAWWRITEL, "drive %d clientThreadId %x aPos %x:%x aLength %d", (TUint) aRequest->DriveNumber(), (TUint) threadId, (TUint) I64HIGH(pos), (TUint) I64LOW(pos), (TUint) length);

	TRAP(r,rawDisk->Mount().RawWriteL(pos,length,pDes,0,aRequest->Message()));

	OstTrace1(TRACE_FILESYSTEM, FSYS_ECMOUNTCBRAWWRITELRET, "r %d", r);

	rawDisk->SetChanged();
	return(r);
	}

TInt TFsRawDiskWrite::Initialise(CFsRequest* aRequest)
//
//
//
	{
	if (!KCapFsRawDiskWrite.CheckPolicy(aRequest->Message(), __PLATSEC_DIAGNOSTIC_STRING("Raw Disk Write")))
		return KErrPermissionDenied;
	CRawDiskCB* rawDisk=GetRawDiskFromHandle(aRequest->Message().Int3(),aRequest->Session());
	if(!rawDisk)
		return(KErrBadHandle);
	if(rawDisk->IsWriteProtected())
		return(KErrAccessDenied);
	aRequest->SetDrive(&rawDisk->Drive());
	aRequest->SetScratchValue((TUint)rawDisk);
	return KErrNone;
	}

CRawDiskCB::CRawDiskCB()
//
// Constructor
//
	{
	}

CRawDiskCB::~CRawDiskCB()
//
// Destructor
//
	{
	if(iMount)
		{
		__ASSERT_DEBUG(iMount->LockStatus()>0,Fault(ERawDiskBadAccessCount));
		RemoveDiskAccess(*iMount);
		if(IsChanged())
			iMount->Drive().SetChanged(ETrue);
		iMount->Close();
		}
	}

void CRawDiskCB::InitL(CMountCB* aMount,TBool aIsWriteProtected)
//
// Initialise
//
	{
	DoInitL(aMount->Drive().DriveNumber());
	iMount=aMount;
	if(aIsWriteProtected)
		SetWriteProtected();
	User::LeaveIfError(iMount->Open());
	}

