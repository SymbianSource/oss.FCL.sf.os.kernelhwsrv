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
// f32\sfile\sf_notifier.cpp
// 
//

#include "sf_notifier.h"
#include "sf_file_cache.h"

CFsObjectCon* FsNotificationManager::iNotifyRequests = NULL;
RFastLock FsNotificationManager::iChainLock;
TInt FsNotificationManager::iFilterRegister[];
CFsPool<CFsNotificationBlock>* FsNotificationManager::iPool;


CFsNotificationPathFilter* CFsNotificationPathFilter::NewL(const TDesC& aPath, const TDesC& aFilename, TInt aDriveNum)
	{
	CFsNotificationPathFilter* self = new (ELeave) CFsNotificationPathFilter();
	CleanupStack::PushL(self);
	self->ConstructL(aPath,aFilename,aDriveNum);
	CleanupStack::Pop(self);
	return self;
	}

void CFsNotificationPathFilter::ConstructL(const TDesC& aPath, const TDesC& aFilename, TInt aDriveNum)
	{
	//Allocate the path and filename
	iPath = aPath.AllocL();
	iFilename = aFilename.AllocL();	
	iDriveNum = aDriveNum;
	}

CFsNotificationPathFilter::~CFsNotificationPathFilter()
	{
	if(iFilename)
		delete iFilename;
	if(iPath)
		delete iPath;
	}

CFsNotificationPathFilter::CFsNotificationPathFilter()
: iPath(NULL), iFilename(NULL)
	{
	}

CFsNotifyRequest* CFsNotifyRequest::NewL()
	{
	CFsNotifyRequest* self = new(ELeave) CFsNotifyRequest();
	CleanupStack::PushL(self);
	self->ConstructL();
	CleanupStack::Pop();
	return self;
	}

void CFsNotifyRequest::ConstructL()
	{
	User::LeaveIfError(iClientSyncLock.CreateLocal());
	User::LeaveIfError(iTailSemaphore.CreateLocal()); 
	}

CFsNotifyRequest::CFsNotifyRequest()
	{
	SetActive(EInactive);
	}

CFsNotifyRequest::~CFsNotifyRequest()
	{
	__PRINT(_L("CFsNotifyRequest::~CFsNotifyRequest()"));
	
	RemoveFilters();
	
	if(ClientMsgHandle()!=0)
		iClientMsg.Complete(KErrCancel);
	
	if(iBufferMsg.Handle()!=0)
		iBufferMsg.Complete(KErrCancel);
	
	iClientSyncLock.Close();
	iTailSemaphore.Close();
	}

/*
 * Returns the Array of TypeFilters.
 * Each TFsNotificationTypeFilter matches to a particular TFsNotification::TFsNotificationType
 * and has a CFsNotificationFilter which stores the iPath and iName associated with this filter type.
 * 
 * (These are speerated so that we can have multiple type filters for every name filter)
 */
TFsNotificationTypeArray* CFsNotifyRequest::FilterTypeList(TInt aDrive,TInt aIndex)
	{
	__ASSERT_DEBUG(aIndex < KNumRegisterableFilters,Fault(ENotificationFault));

	TFsNotificationTypeDriveArray* filters = iDrivesTypesFiltersMap.Find(aDrive);
	if(filters)
		return &((*filters)[aIndex]);
	else
		return NULL;
	}

//Sets filter's notification request status
void CFsNotifyRequest::SetActive(TNotifyRequestStatus aValue)
	{
	iNotifyRequestStatus = aValue;
	}

CFsNotifyRequest::TNotifyRequestStatus CFsNotifyRequest::ActiveStatus()
	{
	return (TNotifyRequestStatus)iNotifyRequestStatus;
	}

//Completes and frees notification request
//In case of KErrNone must be called with iChainLock already held
void CFsNotifyRequest::CompleteClientRequest(TInt aReason,TBool aIsCancel)
	{
	__PRINT(_L("CFsNotifyRequest::CompleteClientRequest()"));

	iClientSyncLock.Wait();
	
	if(aReason==KErrNone) 
		{
		__PRINT(_L("CFsNotifyRequest::CompleteClientRequest() - Complete KErrNone"));
		//Synchronising the current iServerTail to the client.
		iClientHead = iClientTail; //Client has read all previous entries
		iClientTail = iServerTail; //Client's new tail is everything the server has been writing since this function was last called
		TInt clientTail = iClientTail;
		TPckg<TInt> tailDes(clientTail);
		iClientMsg.Write(KMsgPtr0,tailDes);
		}
	else if(aIsCancel)
		{
		__PRINT(_L("CFsNotifyRequest::CompleteClientRequest() - Complete isCancel"));
		iServerTail = 0;
		iClientTail = 0;
		iClientHead = 0;
		TPckgBuf<TInt> tailDes(iClientTail);
		//Perhaps client has crashed so no point checking return:
		iClientMsg.Write(KMsgPtr0,tailDes); 
		}
	__PRINT(_L("CFsNotifyRequest::CompleteClientRequest() - Complete Request"));
	iClientMsg.Complete(aReason);
	iClientSyncLock.Signal();
	}
	
TInt CFsNotifyRequest::SynchroniseBuffer(CFsNotificationBlock& aBlock,TInt aServerTail, TInt aNotificationSize)
	{
	TPtrC8 blockDes((TText8*)aBlock.Data(),aNotificationSize);
	return iBufferMsg.Write(KMsgPtr0,blockDes,aServerTail);
	}

//Removes all filters.
//Deletes iPath, iFilename
TInt CFsNotifyRequest::RemoveFilters()
	{
	__PRINT(_L("CFsNotifyRequest::RemoveFilters()"));
		
	//For every drive with filters set...
	RHashMap<TInt,TFsNotificationTypeDriveArray>::TIter iterator(iDrivesTypesFiltersMap);
	TFsNotificationTypeDriveArray* currentDriveFilters = (TFsNotificationTypeDriveArray*)iterator.NextValue();
	while(currentDriveFilters)
		{
		//For every filter array (1 for each type of TFsNotificationType)
		for(TInt filterType = 0; filterType < KNumRegisterableFilters; filterType++)
			{
			TFsNotificationTypeArray& filterList = (*currentDriveFilters)[filterType];
			TInt filterTypeCount = filterList.Count();
			if(filterTypeCount)
				{
				//Remove this type from the filter register
				TFsNotification::TFsNotificationType type = CFsNotificationInfo::NotificationType(filterType);
				FsNotificationManager::SetFilterRegister(type,EFalse,filterTypeCount);
				}
			filterList.Reset();
			filterList.Close();
			}
		currentDriveFilters->Reset();
		currentDriveFilters->Close();
		iterator.RemoveCurrent();
		currentDriveFilters = (TFsNotificationTypeDriveArray*)iterator.NextValue();
		}
	iDrivesTypesFiltersMap.Close();
	return KErrNone;
	}

TInt CFsNotifyRequest::AddFilterL(CFsNotificationPathFilter* aFilter, TUint aMask)
	{
	__PRINT(_L("CFsNotifyRequest::AddFilterL"));

	//Get the drive number to so know which drive array to add the filter(s) to.
	TInt driveNum = aFilter->iDriveNum; 
	
	TInt notifyType = 1; 
	TInt r = KErrNone;
	//Create/Add a TypeFilter for each type in aMask
	while((notifyType & KNotificationValidFiltersMask) && (aMask & KNotificationValidFiltersMask))
		{
		//If this notifyType is present in aMask
		if(aMask & notifyType)
			{
			TFsNotificationTypeFilter typeFilter;
			typeFilter.iNotificationType = (TFsNotification::TFsNotificationType) notifyType;
			typeFilter.iPathFilter = aFilter;
			TInt index = CFsNotificationInfo::TypeToIndex(typeFilter.iNotificationType);
			
			//If the per-drive-filterLists have not
			//been set up yet then do so now.
			TFsNotificationTypeDriveArray* driveArray = iDrivesTypesFiltersMap.Find(driveNum);
			if(!driveArray)
				{
				TFsNotificationTypeDriveArray dArray;
				r = iDrivesTypesFiltersMap.Insert(driveNum,dArray);
				User::LeaveIfError(r);					
				driveArray = iDrivesTypesFiltersMap.Find(driveNum);
				
				//Create filter arrays for every type
				for(TInt i =0; i< KNumRegisterableFilters; i++)
					{
					TFsNotificationTypeArray filterArray;
					driveArray->Append(filterArray);
					}
				}
			TFsNotificationTypeArray& filterArray= (*driveArray)[index];
			filterArray.Append(typeFilter);

			//Remove this type from our mask
			//and continue
			aMask ^= notifyType;
			}
		notifyType <<= 1;
		}
	return r;
	}

TInt CFsNotifyRequest::SetClientMessage(const RMessage2& aClientMsg)
	{
	__PRINT(_L("CFsNotifyRequest::SetClientMessage"));
	iClientMsg = aClientMsg;
	return KErrNone;
	}

TInt CFsNotifyRequest::ClientMsgHandle()
	{
	return iClientMsg.Handle();
	}

const RMessage2& CFsNotifyRequest::BufferMessage()
    {
    return iBufferMsg;
    }

void CFsNotifyRequest::CloseNotification()
	{
	__PRINT(_L("CFsNotifyRequest::CloseNotification()"));
	iBufferMsg.Complete(KErrNone);
	if(ClientMsgHandle()!=0)
		CompleteClientRequest(KErrCancel,EFalse);
	}

//New notification request from client
void FsNotificationManager::AddNotificationRequestL(CFsNotifyRequest* aNotificationRequest)
	{
	__PRINT(_L("FsNotificationManager::AddNotificationRequestL"));
	Lock();
	iNotifyRequests->AddL(aNotificationRequest,ETrue);
	Unlock();
	}

//Notification request cancelled
//Must be called with iChainLock held
void FsNotificationManager::RemoveNotificationRequest(CFsNotifyRequest* aNotificationRequest)
	{
	__PRINT(_L("FsNotificationManager::RemoveNotificationRequest"));
	iNotifyRequests->Remove(aNotificationRequest,ETrue);
	}

void FsNotificationManager::RemoveNotificationRequest(CSessionFs* aSession)
	{
	__PRINT(_L("FsNotificationManager::RemoveNotificationRequest(CSessionFs*)"));
	
	TInt count = Count();
	if(count)
		{
		Lock();
		count = Count(); //check again just incase it's changed before we got the lock
		if(count)
			{
			for(TInt i=0; i < count; i++)
				{
				//Remove all notification requests associated with this session.
				CFsNotifyRequest* notify = (CFsNotifyRequest*)(*iNotifyRequests)[i];
				if(notify->iSession == aSession)
					{
					RemoveNotificationRequest(notify);
					delete notify;
					}
				}
			if(!Count())
				{
				__PRINT(_L("FsNotificationManager::RemoveNotificationRequest(CSessionFs*) - Closing Manager"));
				Close();
				}
			}
		Unlock();
		}
	}

TBool FsNotificationManager::IsInitialised()
	{
	__PRINT(_L("FsNotificationManager::IsInitialised()"));
	return (TBool)iNotifyRequests;
	}

void FsNotificationManager::OpenL()
	{
	__PRINT(_L("FsNotificationManager::InitialiseL()"));
	if(!iNotifyRequests)
		{
		if(iChainLock.Handle() == 0)
			{
			User::LeaveIfError(iChainLock.CreateLocal());	
			}
		iNotifyRequests = TheContainer->CreateL();
		iPool = CFsPool<CFsNotificationBlock>::New(KNotificationPoolSize,CFsNotificationBlock::New);
		User::LeaveIfNull(iPool);
		}
	}

void FsNotificationManager::SetFilterRegister(TUint aFilter, TBool aAdd, TInt aCount)
	{
	__PRINT2(_L("FsNotificationManager::SetFilterRegister(aFilter=%u,aAdd=%d)"),aFilter,aAdd);
	TInt index = CFsNotificationInfo::TypeToIndex((TFsNotification::TFsNotificationType)aFilter);
	TInt& fr = FsNotificationManager::FilterRegister(index);
	__ASSERT_DEBUG((aAdd) ? fr >= 0 : fr > 0,Fault(ENotificationFault));
	fr+= aAdd ? aCount : -aCount; 
	}

void FsNotificationManager::SetFilterRegisterMask(TUint aMask,TBool aAdd)
	{
	__PRINT(_L("FsNotificationManager::RegisterFilterMask()"));
	TInt notifyType = 1; 

	while(notifyType & KNotificationValidFiltersMask && aMask & KNotificationValidFiltersMask)
		{
		if(aMask & notifyType)
			{
			SetFilterRegister(notifyType,aAdd);
			aMask ^= notifyType;
			}
		notifyType <<= 1;
		}
	}

TInt& FsNotificationManager::FilterRegister(TInt aIndex)
	{
	__PRINT(_L("FsNotificationManager::FilterRegister()"));
	__ASSERT_DEBUG(aIndex < KNumRegisterableFilters,Fault(ENotificationFault));
	return iFilterRegister[aIndex];
	}

//Must be called with the iChainLock
void FsNotificationManager::Close()
	{
	__PRINT(_L("FsNotificationManager::Stop()"));
	CFsObjectCon*& request = iNotifyRequests;
	if(request)
		{
		TheContainer->Delete(request);
		delete iPool;
		iPool = NULL;
		}
	request = NULL;
	}

TInt FsNotificationManager::Count()
	{
	__PRINT(_L("FsNotificationManager::Count()"));
	if(IsInitialised())
		return iNotifyRequests->Count();
	return 0;
	}

void FsNotificationManager::Lock()
	{
	__PRINT(_L("--->FsNotificationManager::Lock()"));
	iChainLock.Wait();
	}

void FsNotificationManager::Unlock()
	{
	__PRINT(_L("<---FsNotificationManager::Unlock()"));
	iChainLock.Signal();
	}

//Get the notification type based on the TFsMessage function
void CFsNotificationInfo::NotificationType(TInt aFunction,TNotificationType& aNotificationType)
	{
	__PRINT(_L("CFsNotificationInfo::NotificationType"));
	switch(aFunction)
		{
		case EFsFileWrite:
		case EFsFileWriteDirty:
		case EFsFileSetSize:
			{
			aNotificationType = TFsNotification::EFileChange;
			break;
			}
		case EFsRename:
		case EFsFileRename:
		case EFsReplace:
			{
			aNotificationType = TFsNotification::ERename;
			break;
			}
		case EFsMkDir:
		case EFsFileCreate:
		case EFsFileReplace:
			{
			aNotificationType = TFsNotification::ECreate;
			break;
			}
		case EFsFileSetAtt:
		case EFsFileSet:
		case EFsSetEntry:
			{
			aNotificationType = TFsNotification::EAttribute;
			break;
			}
		case EFsDelete:
		case EFsRmDir:
			{
			aNotificationType = TFsNotification::EDelete;
			break;
			}
		case EFsSetVolume:
			{
			aNotificationType = TFsNotification::EVolumeName;
			break;
			}
		case EFsSetDriveName:
			{
			aNotificationType = TFsNotification::EDriveName;
			break;
			}
		case EFsDismountFileSystem:
		case EFsMountFileSystem:
		case EFsFormatNext:
		case EFsRawDiskWrite:
		case EFsMountFileSystemScan:
			{
			aNotificationType = TFsNotification::EMediaChange;
			break;
			}
		default:
			{
			aNotificationType = (TFsNotification::TFsNotificationType)0;
			break;
			}
		}
	}

//=====CFsNotificationBlock============================
// Uses CFsPool

CFsNotificationBlock* CFsNotificationBlock::New()
	{
	return new CFsNotificationBlock();
	}
CFsNotificationBlock::CFsNotificationBlock()
	{
	}
CFsNotificationBlock::~CFsNotificationBlock()
	{
	//Nothing to do here
	}
TAny* CFsNotificationBlock::Data()
	{
	return (TAny*)&iData;
	}


 


TBool CFsNotifyRequest::ValidateNotification(TInt aNotificationSize, TInt& aServerTail)
	{
	__PRINT(_L("CFsNotifyRequest::ValidateNotification"));
	//RDebug::Print(_L("CFsNotifyRequest::ValidateNotification - iServerTail=%d, aServerTail=%d, iClientTail=%d,iClientHead=%d, aNotificationSize=%d"),iServerTail,aServerTail,iClientTail,iClientHead,aNotificationSize);
	//
	//Start Validation
	//
	TBool overflow = EFalse;
	
	//Check that we have not filled the buffer
    if (aServerTail == iClientHead)
        {
        // Buffer is empty when Client Tail = Client Head
        if (iClientHead != iClientTail)
        	{
			overflow = ETrue;
            return overflow;            
			}
        }

	//Work out remaining size taking account of whether the end position is
	//before or after the overflow position.
	TInt remainingSize = (iClientHead > aServerTail)
			? iClientHead - aServerTail 
			: iClientBufferSize - (aServerTail - iClientHead);

    TInt reservedSize = aNotificationSize;
    // + Save additional space for OVERFLOW
    reservedSize += KNotificationHeaderSize;

	//
    // Have we wrapped around already?
    //
    if (iClientHead > aServerTail)
        {
		// Yes,
		// Buffer looks something like this:
		//
        //            |CH             
        // [5678------1234]
        //     |ST		

		//
		//  Check if we can insert in the middle section:
		//
		if (remainingSize < reservedSize)
			{
			overflow = ETrue;
			}	
		//else:
		// 	{
		// 	We add new notification to middle 
        //	[5678***---1234]
        // 	}
		//
		return overflow;
        }


	//
    // We have not wrapped around yet..
    //
    // Buffer looks something like this:
    //
    //    |CH      
    // [--123456789--]
    //            |ST
    //


	//
    // Check up-front whether its possible for overflow to go at the beginning.
    // If there is not enough space at the start for overflow then we need to
    // check that's there's space for overflow at the end and must not rollover.
    //
    TBool canRollOver = ETrue;
    
    if (iClientHead < KNotificationHeaderSize)
        {
		//
        //  |CH      
        // [123456789----]
        //          |ST
        //
        // No space for overflow at the beginning of buffer.
        //
        canRollOver = EFalse; 
        }

	//
    // IF: Cannot rollover
    //
    if (!canRollOver)
        {
        //IF (notification + overflow) does not fit at the end overflow now.
        if ((iClientBufferSize - aServerTail) < reservedSize)
            {
            overflow = ETrue;
            }        
        //Else
        //	{
		//	Add notification (**) to end [---12345678**---]
		//	}

        }
    else 
	// Can rollover  
	// - need to check that notification fits at the end
	//   or that notification+overflow fits at the beginning.
        {
        // If not enough space at end, rollover
        if ((iClientBufferSize - aServerTail) < aNotificationSize)
            {
			//
			// Add notification to start and fill end with Filler char 
            // [----0123456789#]
            //
            
            // IF we are not at the very end of the buffer,
			// insert a KNotificationBufferFiller at iServerTail.
			// When the client reads this, it sets iHead to 0 and reads from there.
			if(iServerTail != iClientBufferSize)
				{
				//If there is space it will always be at least 1 word big
				TPtrC8 fillerDes((TText8*) &KNotificationBufferFiller, sizeof(TUint));
				iBufferMsg.Write(KMsgPtr0, fillerDes, aServerTail);
				}

            // Now that we have rolled over we need to check whether there is
            // space at the beginning for notification + overflow
			// We already know that overflow fits.
            if (reservedSize > iClientHead)
                {
                //  [ov--0123456789-]
                overflow = ETrue;
                }
			//
			// Add notification/overflow to the beginning
			//  	[**--0123456789(#)]
			//
			aServerTail = 0;
			}
		//
		// else - notification fits at the end so there is nothing to do here.
		//
		//
        }
    //
    //End Validation
    //
    return overflow;
    }

// Called from FsNotificationManager::HandleChange().
// Sends notifications into the client's buffer.
// If there is a iClientMsg then this is the first time this
// has been called since the client called RequestNotifications.
// In this situation we complete the client request.
TInt CFsNotifyRequest::NotifyChange(CFsNotificationInfo* aRequest, CFsNotificationBlock& aBlock)
	{
	/*
	 * Different notification types have different data associated with them.
	 * 
	 * All types EXCEPT for ERename, EVolumeName and EDriveName have the following data 
	 * and are aligned in the buffer like so:
	 * Word1   : NotificationSize (2 bytes) , PathSize (2 bytes)
	 * Word2   : NotificationType (Lower 2 bytes)
	 * Word(s) : Path (TText8) , [Any sub-class members]
	 * 
	 * Else for notification types ERename, EVolumeName and EDriveName the order is:
	 * Word1   : NotificationSize (2 bytes) , PathSize (2 bytes)
	 * Word2   : NewNameSize (2 bytes) , NotificationType (2 bytes)
	 * Word(s) : Path (TText8) , NewName (TText8)
	 * 
	 * Overflow notification type doesn't have a name, so its namesize is 0
	 * and there will be no Word3.
	 */	
	
	__PRINT(_L("CFsNotifyRequest::NotifyChange()"));

    TInt notificationSize = CFsNotificationInfo::NotificationSize(*aRequest);
    
	iClientSyncLock.Wait();
	iTailSemaphore.Wait();
	
	TInt tail = iServerTail;
	
	//Validation
	TBool overflow = ValidateNotification(notificationSize, tail);
		
	//Now that we know there is enough space in the buffer we can write 
	//the standard attributes that all notifications have.

	//We can store the size of the notification 
	//and the size of the name in the same word.
	
	TBuf<2> driveBuf;
	driveBuf.SetLength(2);
    TChar driveLetter = 'A';
    RFs::DriveToChar(aRequest->DriveNumber(),driveLetter);
    driveBuf[0] = (TText)driveLetter;
    driveBuf[1] = (TText)':';
	
	TUint16 nameLen = 0;	//Overflow has no name
	TInt notifSize = KNotificationHeaderSize;
	if(!overflow)
		{
        nameLen = (TUint16)aRequest->SourceSize();
		notifSize = notificationSize;
		}
	else 
		{
        aRequest->NotificationType() = TFsNotification::EOverflow;
		}	

	iServerTail = tail + notifSize;
	iTailSemaphore.Signal();
	
	TInt writeOffset = 0;	//Where to write in the block
	
	//Store notification Size and NameSize (Word1)
	TUint sizeNameLen = (notifSize << 16) | nameLen;	
	memcpy((TText8*)aBlock.Data()+writeOffset,&sizeNameLen,sizeof(TUint));
	writeOffset+=sizeof(TUint);

    if (aRequest->NotificationType() == TFsNotification::ERename ||
        aRequest->NotificationType() == TFsNotification::EVolumeName ||
        aRequest->NotificationType() == TFsNotification::EDriveName)
        {
        //Store NewNameSize and notification Type (Word2)
        TUint typeNewNameLen = ((TUint16)aRequest->NewNameSize() << 16) | (TUint16)aRequest->NotificationType();
		memcpy((TText8*)aBlock.Data()+writeOffset,&typeNewNameLen,sizeof(TUint));
		}
	else
		{
		//Store notification Type (Word2)
        memcpy((TText8*)aBlock.Data()+writeOffset,&aRequest->NotificationType(),sizeof(TUint));
		}
	writeOffset+=sizeof(TUint);
	
    //
    //Store UID
	memcpy((TText8*)aBlock.Data()+writeOffset,&aRequest->Uid().iUid,sizeof(TUint32));
	writeOffset+=sizeof(TUint32);
	
	
	if(!overflow)
		{
		//Store Name (Word3)
	    {
	    //Store driveColon
	    if(aRequest->NotificationType()!=TFsNotification::EMediaChange)
	        {
	        memcpy((TText8*)aBlock.Data()+writeOffset,driveBuf.Ptr(),driveBuf.Size());
	        writeOffset += driveBuf.Size(); //NB: Not Align4'd deliberately.
	        }
	    memcpy((TText8*)aBlock.Data()+writeOffset,aRequest->Source().FullName().Ptr(),aRequest->Source().FullName().Size());
	    writeOffset += Align4(aRequest->Source().FullName().Size());
	    }

        switch (aRequest->NotificationType())
			{
			case TFsNotification::EFileChange:
			case TFsNotification::EAttribute:
				{
                memcpy((TText8*)aBlock.Data()+writeOffset,aRequest->Data(),sizeof(TInt64));
				writeOffset += sizeof(TInt64);
				break;
				}
			case TFsNotification::ERename:
			case TFsNotification::EVolumeName:
			case TFsNotification::EDriveName:
				{
				//Store NewName
				
				if(!aRequest->DestDriveIsSet())
				    {
				    //This means that the notification has come from a Mount rather than from FileServer
				    //It also means that the new name will have the same drive letter as the source.
				    memcpy((TText8*)aBlock.Data()+writeOffset,driveBuf.Ptr(),driveBuf.Size());
				    writeOffset += driveBuf.Size(); //NB: Not Align4'd deliberately.
				    }
                memcpy((TText8*)aBlock.Data()+writeOffset,aRequest->NewName().FullName().Ptr(),aRequest->NewName().FullName().Size());
                writeOffset += Align4(aRequest->NewName().FullName().Size());
				break;
				}

			default:
				{
				break;
				}
			}
		}
	
	//Write to client buffer
	TInt r = SynchroniseBuffer(aBlock,tail,notifSize);
	
	//Signal the iClientSyncLock. 
	//When all locks on this are signaled then CompleteClientRequest can be called.
	//This signal moves when we have a cache system
	iClientSyncLock.Signal();
	
	//We need to complete if this was the first 
	//write to the client's buffer
    if (r == KErrNone)
        {
		//We need to complete if this was the first 
		//write to the client's buffer
        if(ClientMsgHandle()!=0)
            {
			//RDebug::Print(_L("CFsNotifyRequest::NotifyChange iClientHead(%d) iClientTail(%d) iServerTail(%d) iClientBufferSize(%d)"),iClientHead,iClientTail,iServerTail,iClientBufferSize);
            __PRINT4(_L("CFsNotifyRequest::NotifyChange iClientHead(%d) iClientTail(%d) iServerTail(%d) iClientBufferSize(%d)"),iClientHead,iClientTail,iServerTail,iClientBufferSize);
            CompleteClientRequest(KErrNone);
            }
        else if(!overflow)
            {
		SetActive(CFsNotifyRequest::EOutstanding);
            }
        else //Overflow
            {
		SetActive(CFsNotifyRequest::EOutstandingOverflow);
            }
        }
	else // r!=KErrNone
		{
		//RDebug::Print(_L("sf_notifier.cpp line %d function = %d, r = %d"),__LINE__, aRequest->FsFunction(),r);
		//RDebug::Print(_L("iServerTail=%d, tail=%d, iClientBufferSize=%d, overflow=%d"),iServerTail,tail,iClientBufferSize,overflow);
		}
	return r;
	}

#ifdef SYMBIAN_F32_ENHANCED_CHANGE_NOTIFICATION

//A change has occurred in f32 represented by this
//request object. Work out which CfsNotify’s are interested
// (if any) and call CfsNotifyRequest::NotifyChange.
void FsNotificationManager::HandleChange(CFsNotificationInfo& aRequest)
    {
    __PRINT2(_L("FsNotificationManager::HandleChange() aNotificationInfo=0x%x,NotificationType=%d"),&aRequest,aRequest.NotificationType());
    if(Count())
        {
        Lock(); //ToDo: Read Lock (Read/Write Lock) 
        if(Count())
            {
            //Only search while there are filters of this type set up.
            TInt index = CFsNotificationInfo::TypeToIndex(aRequest.NotificationType());
            TInt& filterCount = FsNotificationManager::FilterRegister(index);
            TInt seenFilter = filterCount; //Number of requests set up for this type
            
            //Iterate CFsNotifyRequests
            TInt count = iNotifyRequests->Count();
            
            if(aRequest.NotificationType() == TFsNotification::EMediaChange)
                seenFilter = count;
            
            //If there aren't any requests then breakout
            if(count == 0)
                {
                Unlock();
                return;
                }
            
            //For every notification request(i.e. every CFsNotify client-side).
            for(TInt i=0; i<count && seenFilter; ++i)
                {
                CFsNotifyRequest* notifyRequest = (CFsNotifyRequest*)(*iNotifyRequests)[i];
                CFsNotifyRequest::TNotifyRequestStatus status = notifyRequest->ActiveStatus();
                if(! (status==CFsNotifyRequest::EActive || 
                      status==CFsNotifyRequest::EOutstanding))
                    {
                    //Not active; check next notification request
                    continue;
                    }
                
                //Check whether we are interested in this change.
                //Get the filters associated with this operation on this drive
                TFsNotificationTypeArray* filterList = notifyRequest->FilterTypeList(aRequest.DriveNumber(),index);
                DoHandleChange(filterList,seenFilter,aRequest,notifyRequest);

                if(aRequest.NotificationType()==TFsNotification::EMediaChange)
                    continue; //next request
                
                //If there are still filters to check
                if(seenFilter)
                    {
                    //Check changes that are not tied to a particular drive
                    filterList = notifyRequest->FilterTypeList(KErrNotFound,index);
                    DoHandleChange(filterList,seenFilter,aRequest,notifyRequest);
                    }
                }
            }
        Unlock();
        }
    }


////
#else
////

void FsNotificationManager::HandleChange(CFsNotificationInfo&)
	{
	return;
	}

#endif //SYMBIAN_F32_ENHANCED_CHANGE_NOTIFICATION

//Called from FsNotificationManager::DoHandleChange
FsNotificationManager::TFsNotificationFilterMatch FsNotificationManager::DoMatchFilter(const RMessage2& aMessage, const TDesC& aOperationName,CFsNotificationPathFilter& aFilter)
    {
    TFsNotificationFilterMatch filterMatch = EDifferent;
    TParsePtrC parseOp(aOperationName);
    
    TPtrC pathOpDes = parseOp.DriveAndPath();
    TPtrC nameOpDes = parseOp.NameAndExt();
    TInt pathLength = aFilter.iPath->Des().Length();
    TInt nameLength = aFilter.iFilename->Des().Length();
    TInt paths = -1;
    TInt names = -1;
    if(pathLength != 0)
        {
        paths = pathOpDes.MatchF(aFilter.iPath->Des());
        }
    else //if no path filter was set up
        // then we need to ensure we don't notify on data-caged areas which we shouldn't
        {
        TInt r = PathCheck(aMessage,aOperationName.Mid(2),&KCapFsSysFileTemp,&KCapFsPriFileTemp,&KCapFsROFileTemp, __PLATSEC_DIAGNOSTIC_STRING("FsNotificationManager::DoHandleChange"));
        if(r != KErrNone)
            return EContinue; //next filter
        }
    
    if(nameLength != 0)
        {
        names = nameOpDes.MatchF(aFilter.iFilename->Des());  
        }
    //Check: Path & Names Match
    if((paths == 0 || pathLength==0) &&                             //  paths match && 
        (names >= 0 || (nameLength==0 && nameOpDes.Length()==0))) // names match OR there are no names (i.e. operation is a dir and no filename filter)
        {
         filterMatch = EMatch;
        }
    return filterMatch;
    }

// The aFilterTypeArray is an array for the filters that target the current drive (only).
// This is called on a per client (CFsNotify) basis.
void FsNotificationManager::DoHandleChange(TFsNotificationTypeArray* aFilterTypeArray,TInt& aSeenFilter, CFsNotificationInfo& aNotificationInfo, CFsNotifyRequest* aNotifyRequest)
	{		
	__PRINT(_L("FsNotificationManager::DoHandleChange()"));
	
	if(!aFilterTypeArray)
		return;
	
	TInt numFilters = aFilterTypeArray->Count();
	
    if(aNotificationInfo.NotificationType() == TFsNotification::EMediaChange)
        numFilters = 1; //Only need to notify once per client for EMediaChange.
        
    //For every filter in this request (CFsNotify)
	for(TInt j = 0; j < numFilters;++j)
		{
		//Is the correct notification type
		aSeenFilter--;
		
		TBool filterMatch = EDifferent;
        if(aNotificationInfo.NotificationType()  != TFsNotification::EMediaChange)
			{
			CFsNotificationPathFilter& filter = *(((*aFilterTypeArray)[j]).iPathFilter);
            __PRINT2(_L("FsNotificationManager::DoHandleChange() operationName=%S, filterName=%S"),&aNotificationInfo.Source().FullName(),filter.iPath);
            
			//buferMsg here is the message of the client *recieving* the notification
            const RMessage2& bufferMsg = aNotifyRequest->BufferMessage();
            filterMatch = DoMatchFilter(bufferMsg,aNotificationInfo.Source().FullName(),filter);
			if(filterMatch == FsNotificationManager::EContinue)
			    continue; //triggers for data cages
			
			//We need to check for changes coming in to a directory when its rename
            if(aNotificationInfo.NotificationType() == TFsNotification::ERename && filterMatch==FsNotificationManager::EDifferent)  
                {
                __PRINT2(_L("FsNotificationManager::DoHandleChange() destinationName=%S, filterName=%S"),&aNotificationInfo.NewName().FullName(),filter.iPath);
                if(aNotificationInfo.DestDriveIsSet())
                    filterMatch = DoMatchFilter(bufferMsg,aNotificationInfo.NewName().FullName().Mid(2),filter);
                else
                    filterMatch = DoMatchFilter(bufferMsg,aNotificationInfo.NewName().FullName(),filter);
                }
			}

        if(filterMatch || (aNotificationInfo.NotificationType() == TFsNotification::EMediaChange))//Match or MediaChange (report regardless of filters)
			{
			//Matching - Handle change
			
			//Get a CFsNotificationBlock to use 
			//So that we can do IPC from a single place.
			CFsNotificationBlock* block = iPool->Allocate();
				
            TInt r = aNotifyRequest->NotifyChange(&aNotificationInfo,*block);
				
			//Free block
			iPool->Free(block);
				
			if(r != KErrNone)
				{
				//Something went wrong writing to the client's buffer
				aNotifyRequest->SetActive(CFsNotifyRequest::EInactive);
				if(aNotifyRequest->ClientMsgHandle()!=0)
					aNotifyRequest->CompleteClientRequest(r,EFalse);
				break; //Go to outer for (i.e. next request in HandleChange)
				}
			}	
		continue; //next filter
		}
	}
