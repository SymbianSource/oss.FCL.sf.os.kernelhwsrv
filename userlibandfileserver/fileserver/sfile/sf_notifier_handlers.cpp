// Copyright (c) 2008-2009 Nokia Corporation and/or its subsidiary(-ies).
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
// f32\sfile\sf_notifier_handlers.cpp
// 
//
#include "sf_std.h"
#include "sf_notifier.h"

#ifdef SYMBIAN_F32_ENHANCED_CHANGE_NOTIFICATION

TInt TFsNotificationOpen::Initialise(CFsRequest* /*aRequest*/)
	{
	return KErrNone;
	}

TInt TFsNotificationOpen::DoRequestL(CFsRequest* aRequest)
	{
	__PRINT(_L("TFsNotificationOpen::DoRequestL()"));
	//Check whether we've got a notification manager 
	//If not, create it and call OpenL
	if(!FsNotificationManager::IsInitialised())
		{
		FsNotificationManager::OpenL();
		}
	
	//Create a new CFsNotifyRequest and add it to the manager
	CFsNotifyRequest* notifyRequest = CFsNotifyRequest::NewL();

	//Get handle and add request to manager and Session->Handles
	TInt handle = 0;
	TBool addedToManager = EFalse;
	TRAPD(ret,HandleRequestL(aRequest, notifyRequest, handle,addedToManager));	
	if (ret!=KErrNone)
		{
		//Remove request from Session->Handles if it was already added
		if (handle!=0)
			aRequest->Session()->Handles().Remove(handle,ETrue);
		
		//Remove request from manager
		if(addedToManager)
			{
			FsNotificationManager::Lock();
			FsNotificationManager::RemoveNotificationRequest(notifyRequest);
			FsNotificationManager::Unlock();
			}
		delete notifyRequest;
		return(ret);
		}
	
	notifyRequest->iSession = aRequest->Session();
	aRequest->Session()->IncResourceCount();
	return ret;
	}

//Get handle and add request to Session->Handles
void TFsNotificationOpen::HandleRequestL(CFsRequest* aRequest, CFsNotifyRequest* aNotifyRequest, TInt& aHandle,TBool& aAddedToManager)
	{
	aAddedToManager = EFalse;
	FsNotificationManager::AddNotificationRequestL(aNotifyRequest);
	aAddedToManager = ETrue;
	aHandle = aRequest->Session()->Handles().AddL(aNotifyRequest,ETrue);
	TPtrC8 pH((TUint8*)&aHandle, sizeof(TInt));
	aRequest->WriteL(KMsgPtr3,pH);
	}

TInt TFsNotificationBuffer::Initialise(CFsRequest* /*aRequest*/)
	{
	return KErrNone;
	}

TInt TFsNotificationBuffer::DoRequestL(CFsRequest* aRequest)
	{
	__PRINT(_L("TFsNotificationBuffer::DoRequestL()"));
	HBufC8* buffer = (HBufC8*)(aRequest->Message().Ptr0());
	TInt handle = aRequest->Message().Int3();
	CFsNotifyRequest* notifyRequest = (CFsNotifyRequest*)SessionObjectFromHandle(handle,0,aRequest->Session());
	if(!notifyRequest)
		return KErrBadHandle;
	if(!buffer)
		return KErrArgument;

	notifyRequest->iClientBufferSize = aRequest->Message().Int1();
	notifyRequest->iBufferMsg = aRequest->Message();
	return KErrNone;
	}

TInt TFsNotificationRequest::Initialise(CFsRequest* aRequest)
	{
	TInt handle = aRequest->Message().Int3();	
	CFsNotifyRequest* notifyRequest = (CFsNotifyRequest*)SessionObjectFromHandle(handle,0,aRequest->Session());
	if(!notifyRequest)
		return KErrBadHandle;
	
	//Check the tail's validity
	TInt tail;
	TPckg<TInt> tailPkg(tail);
	TInt r = aRequest->Read(KMsgPtr0,tailPkg);
	
	if(r!=KErrNone || tail < 0 || tail > notifyRequest->iClientBufferSize)
		return KErrArgument;
		
	return KErrNone;
	}

TInt TFsNotificationRequest::DoRequestL(CFsRequest* aRequest)
	{
	__PRINT(_L("TFsNotificationRequest::DoRequestL()"));
	//We need to check whether there is anything in the buffer
	//If so we should complete straight away.
	FsNotificationManager::Lock();
	
	//Get notification request
	TInt handle = aRequest->Message().Int3();	
	CFsNotifyRequest* notifyRequest = (CFsNotifyRequest*)SessionObjectFromHandle(handle,0,aRequest->Session());
	if(!notifyRequest)
		{
		FsNotificationManager::Unlock();
		return KErrBadHandle;
		}
	
	TInt r = notifyRequest->SetClientMessage(aRequest->Message());
	if(r != KErrNone)
		{
		FsNotificationManager::Unlock();
		return r;
		}

	CFsNotifyRequest::TNotifyRequestStatus status = notifyRequest->ActiveStatus();
	if(status==CFsNotifyRequest::EOutstanding ||
		status==CFsNotifyRequest::EOutstandingOverflow)
		{
		notifyRequest->CompleteClientRequest(KErrNone);
		}

	//Update Status
	if(status!=CFsNotifyRequest::EOutstandingOverflow)
		{
		notifyRequest->SetActive(CFsNotifyRequest::EActive);
		// RDebug::Print(_L("TFsNotificationRequest::DoRequestL Not-OutOver- iClientHead==%d, iClientTail==%d"),notifyRequest->iClientHead,notifyRequest->iClientTail);
		}
	else
		{
		notifyRequest->SetActive(CFsNotifyRequest::EInactive);
		
		// RDebug::Print(_L("TFsNotificationRequest::DoRequestL OutOver- iClientHead==%d, iClientTail==%d"),notifyRequest->iClientHead,notifyRequest->iClientTail);
		
		// If the user is in OutstandingOverflow notification state, 
		// then we can set iClientHead to be equal to iServerTail now.
		// That way if the client requests again and the state will go 
		// back to active, the server will see that buffer as empty 
		// rather than full/overflow.
		
		notifyRequest->iClientHead = notifyRequest->iClientTail;
		}
	FsNotificationManager::Unlock();
	return r;
	}

TInt TFsNotificationCancel::Initialise(CFsRequest* /*aRequest*/)
	{
	return KErrNone;
	}

TInt TFsNotificationCancel::DoRequestL(CFsRequest* aRequest)
	{
	__PRINT(_L("TFsNotificationCancel::DoRequestL()"));
	FsNotificationManager::Lock();
	
	//Get notification request and deactivate filter
	TInt handle = aRequest->Message().Int3();	
	CFsNotifyRequest* notifyRequest = (CFsNotifyRequest*)SessionObjectFromHandle(handle,0,aRequest->Session());
	if(!notifyRequest)
		{
		FsNotificationManager::Unlock();
		return KErrBadHandle;
		}
	
	if(notifyRequest->ClientMsgHandle()!=0)
		{	
		notifyRequest->SetActive(CFsNotifyRequest::EInactive);
		notifyRequest->CompleteClientRequest(KErrCancel,ETrue);
		}
	FsNotificationManager::Unlock();
	return KErrNone;
	}

TInt TFsNotificationSubClose::Initialise(CFsRequest* /*aRequest*/)
	{
	return KErrNone;
	}

TInt TFsNotificationSubClose::DoRequestL(CFsRequest* aRequest)
	{
	__PRINT(_L("TFsNotificationSubClose::DoRequestL()"));
	FsNotificationManager::Lock();
	
	//We need to complete the buffer request here as this type of request is not
	//completed in the normal way as it is kept open in order that we can access the buffer
	//in the client-side for the lifetime of this subsession.
	TInt handle = aRequest->Message().Int3();
	CFsNotifyRequest* notifyRequest = (CFsNotifyRequest*) SessionObjectFromHandle(handle,0,aRequest->Session());
	if(!notifyRequest)
		{
		FsNotificationManager::Unlock();
		return KErrBadHandle;
		}
	
	notifyRequest->RemoveFilters();
	notifyRequest->CloseNotification(); //Completes the buffer and the client requests.
	
	TInt count = FsNotificationManager::Count();
	
	//Also deletes notifyRequest
	aRequest->Session()->Handles().Remove(handle,ETrue);
	if(count==1)
		{
		//If this is the last request then we need to remove the manager
		FsNotificationManager::Close();
		}
	
	FsNotificationManager::Unlock();
	aRequest->Session()->DecResourceCount();
	notifyRequest = NULL;

	return(KErrNone);
	}

TInt TFsNotificationAdd::Initialise(CFsRequest* aRequest)
	{
	__PRINT(_L("TFsNotificationAdd::Initialise()"));
	TUint filter = (TUint) aRequest->Message().Int0();
	//If it's AllOps then it's ok
	if (filter!=TFsNotification::EAllOps)
		{

		TInt invalid = filter & ~KNotificationValidFiltersMask;
		//Check: Non-valid values ARE NOT set
		//		 and valid values ARE set.
		if(invalid || !filter)
			{
			return KErrArgument;
			}
		}
	
	TFileName path;
	TInt r = aRequest->Read(KMsgPtr1,path);
	if(r != KErrNone)
		{
		return r;
		}
	
	if(path.Length() >= 2)
		r=PathCheck(aRequest,path.Mid(2),&KCapFsSysFileTemp,&KCapFsPriFileTemp,&KCapFsROFileTemp, __PLATSEC_DIAGNOSTIC_STRING("Notification Add Filter"));
	return r;
	}

TInt TFsNotificationAdd::DoRequestL(CFsRequest* aRequest)
	{
	__PRINT(_L("TFsNotificationAdd::DoRequestL()"));
	TInt handle = aRequest->Message().Int3();
	CFsNotifyRequest* notifyRequest = (CFsNotifyRequest*) SessionObjectFromHandle(handle,0,aRequest->Session());
	if(!notifyRequest)
		return KErrBadHandle;
	
	TFileName path;
	aRequest->Read(KMsgPtr1,path);
	
	TFileName filename;
	TInt r = aRequest->Read(KMsgPtr2,filename);
	if(r!= KErrNone)
		return r;
	
	__PRINT2(_L("TFsNotificationAdd::AddNotification() path=%S, filename=%S"),&path,&filename);
	
	//If this is a path starting with 'drive-letter:'
	TInt driveNum = FsNotificationHelper::DriveNumber(path);
	if(path.Length() >= 2 && (driveNum < 0 || driveNum > 25) && ((TChar)driveNum)!=((TChar)'?') && ((TChar)path[1])==(TChar)':')
		{
		return KErrPathNotFound;
		}

	CleanupStack::PushL(notifyRequest);
	CFsNotificationPathFilter* filter = CFsNotificationPathFilter::NewL(path,filename);

	//Bitmask of filter types
	TUint filterMask = (TUint) aRequest->Message().Int0();
	
	r = notifyRequest->AddFilterL(filter,filterMask);
	CleanupStack::Pop(notifyRequest);

	if(r == KErrNone)
		{
		FsNotificationManager::Lock();
		//Increment global filter register
		FsNotificationManager::SetFilterRegisterMask(filterMask,(TBool)ETrue);
		FsNotificationManager::Unlock();
		}
	return r;
	}

TInt TFsNotificationRemove::Initialise(CFsRequest* /*aRequest*/)
	{
	__PRINT(_L("TFsNotificationRemove::Initialise()"));
	return KErrNone;
	}

TInt TFsNotificationRemove::DoRequestL(CFsRequest* aRequest)
	{
	__PRINT(_L("TFsNotificationRemove::DoRequestL()"));
	FsNotificationManager::Lock();
	
	TInt handle = aRequest->Message().Int3();
	CFsNotifyRequest* notifyRequest = (CFsNotifyRequest*) SessionObjectFromHandle(handle,0,aRequest->Session());
	if(!notifyRequest)
		{
		FsNotificationManager::Unlock();
		return KErrBadHandle;
		}
	
	TInt r = notifyRequest->RemoveFilters();
	FsNotificationManager::Unlock();
	return r;
	}

#else //SYMBIAN_F32_ENHANCED_CHANGE_NOTIFICATION

CFsObjectCon* FsNotificationManager::iNotifyRequests = NULL;

CFsNotifyRequest::CFsNotifyRequest()
	{
	}

CFsNotifyRequest::~CFsNotifyRequest()
	{
	}

TInt TFsNotificationOpen::Initialise(CFsRequest* /*aRequest*/)
	{
	return KErrNotSupported;
	}

TInt TFsNotificationOpen::DoRequestL(CFsRequest* /*aRequest*/)
	{
	return KErrNotSupported;
	}

void TFsNotificationOpen::HandleRequestL(CFsRequest* /*aRequest*/, CFsNotifyRequest* /*aNotifyRequest*/, TInt& /*aHandle*/,TBool& /*aAddedToManager*/)
	{
	User::Leave(KErrNotSupported);
	}

TInt TFsNotificationBuffer::Initialise(CFsRequest* /*aRequest*/)
	{
	return KErrNotSupported;
	}

TInt TFsNotificationBuffer::DoRequestL(CFsRequest* /*aRequest*/)
	{
	return KErrNotSupported;
	}

TInt TFsNotificationRequest::Initialise(CFsRequest* /*aRequest*/)
	{
	return KErrNotSupported;
	}

TInt TFsNotificationRequest::DoRequestL(CFsRequest* /*aRequest*/)
	{
	return KErrNotSupported;
	}

TInt TFsNotificationCancel::Initialise(CFsRequest* /*aRequest*/)
	{
	return KErrNotSupported;
	}

TInt TFsNotificationCancel::DoRequestL(CFsRequest* /*aRequest*/)
	{
	return KErrNotSupported;
	}

TInt TFsNotificationSubClose::Initialise(CFsRequest* /*aRequest*/)
	{
	return KErrNotSupported;
	}

TInt TFsNotificationSubClose::DoRequestL(CFsRequest* /*aRequest*/)
	{
	return KErrNotSupported;
	}

TInt TFsNotificationAdd::Initialise(CFsRequest* /*aRequest*/)
	{
	return KErrNotSupported;
	}

TInt TFsNotificationAdd::DoRequestL(CFsRequest* /*aRequest*/)
	{
	return KErrNotSupported;
	}

TInt TFsNotificationRemove::Initialise(CFsRequest* /*aRequest*/)
	{
	return KErrNotSupported;
	}

TInt TFsNotificationRemove::DoRequestL(CFsRequest* /*aRequest*/)
	{
	return KErrNotSupported;
	}

#endif //SYMBIAN_F32_ENHANCED_CHANGE_NOTIFICATION

