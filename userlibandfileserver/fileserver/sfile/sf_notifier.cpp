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


CFsNotificationPathFilter* CFsNotificationPathFilter::NewL(const TDesC& aPath, const TDesC& aFilename)
	{
	CFsNotificationPathFilter* self = new (ELeave) CFsNotificationPathFilter();
	CleanupStack::PushL(self);
	self->ConstructL(aPath,aFilename);
	CleanupStack::Pop(self);
	return self;
	}

void CFsNotificationPathFilter::ConstructL(const TDesC& aPath, const TDesC& aFilename)
	{
	//Allocate the path and filename
	iPath = aPath.AllocL();
	iFilename = aFilename.AllocL();	
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
				TFsNotification::TFsNotificationType type = FsNotificationHelper::NotificationType(filterType);
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
	iPathFilterList.ResetAndDestroy();
	iPathFilterList.Close();
	return KErrNone;
	}

TInt CFsNotifyRequest::AddFilterL(CFsNotificationPathFilter* aFilter, TUint aMask)
	{
	__PRINT(_L("CFsNotifyRequest::AddFilterL"));

	iPathFilterList.AppendL(aFilter);
	
	//Get the drive number to so know which drive array to add the filter(s) to.
	TInt driveNum = FsNotificationHelper::DriveNumber(aFilter->iPath->Des()); 
	
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
			TInt index = FsNotificationHelper::TypeToIndex(typeFilter.iNotificationType);
			
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
		iPool = CFsPool<CFsNotificationBlock>::New(KNotificationPoolSize);
		User::LeaveIfNull(iPool);
		}
	}

void FsNotificationManager::SetFilterRegister(TUint aFilter, TBool aAdd, TInt aCount)
	{
	__PRINT2(_L("FsNotificationManager::SetFilterRegister(aFilter=%u,aAdd=%d)"),aFilter,aAdd);
	TInt index = FsNotificationHelper::TypeToIndex((TFsNotification::TFsNotificationType)aFilter);
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
void FsNotificationHelper::NotificationType(TInt aFunction,TFsNotification::TFsNotificationType& aNotificationType)
	{
	__PRINT(_L("FsNotificationHelper::NotificationType"));
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


//=====FsNotificationManager===========================
 
//Get the path of the file, folder or drive name based on the TFsMessage function
void FsNotificationHelper::PathName(CFsClientMessageRequest& aRequest, TDes& aPath)
	{
	__PRINT(_L("FsNotificationHelper::PathName"));
	//Get the notification type
	TInt function = aRequest.Operation()->Function();
	
	//Get the filename(s)
	switch(function)
		{
		case EFsFileWrite:			//EParseSrc | EFileShare
		case EFsFileSetSize:		//EParseSrc | EFileShare
		case EFsFileSetAtt:			//EParseDst | EParseSrc, - should not use these; has share.
		case EFsFileSet:
		case EFsFileWriteDirty:		//EFileShare
			{
			CFileShare* share = NULL;
			CFileCB* file = NULL;
			GetFileFromScratch(&aRequest,share,file);	
			aPath.Append(file->DriveNumber() + 'A');
			aPath.Append(':');
			aPath.Append(file->FileName().Des());
			break;
			}
		case EFsFileCreate:			//EParseSrc
		case EFsDelete:				//EParseSrc
		case EFsSetEntry:			//EParseSrc,
		case EFsFileRename:			//EParseDst | EParseSrc,
		case EFsRename:				//EParseDst | EParseSrc,
		case EFsReplace:			//EParseDst | EParseSrc,
		case EFsFileReplace:		//EParseSrc
			{
			aPath.Copy(aRequest.Src().FullName());
			break;
			}
        case EFsRmDir:              //EParseSrc
        case EFsMkDir:              //EParseSrc
            {
            aPath.Copy(aRequest.Src().DriveAndPath());
            break;
            }
		case EFsFormatNext:			//EParseSrc
		case EFsDismountFileSystem: //0
		case EFsMountFileSystem:	//0
		case EFsSetVolume:			//0
		case EFsSetDriveName:		//ESync
		case EFsRawDiskWrite:		//EParseSrc
		case EFsMountFileSystemScan:
			{
			_LIT(KFormatDrive,"?:");
			TBuf<2> drive;
			drive.Append(KFormatDrive);
			drive[0] = TText(aRequest.Drive()->DriveNumber() + 'A');
			aPath.Copy(drive);
			break;
			}
		default:
			ASSERT(0);
			break;
		}
	}

//Get the new path of the file, folder or drive name based on the TFsMessage function
void FsNotificationHelper::NewPathName(CFsClientMessageRequest& aRequest, TPtrC& aNewPath)
	{
	__PRINT(_L("FsNotificationHelper::NewPathName"));
	//Get the notification type
	TInt function = aRequest.Operation()->Function();

	//Get the filename(s)
	switch(function)
		{
		case EFsFileRename:			//EParseDst | EParseSrc,
		case EFsRename:				//EParseDst | EParseSrc,
		case EFsReplace:			//EParseDst | EParseSrc,
			{
			aNewPath.Set(aRequest.Dest().FullName());
			break;
			}
		case EFsSetDriveName:		//ESync
			{
			TFileName name;
			aRequest.ReadL(KMsgPtr1, name);
			aNewPath.Set(name);
			break;
			}
		case EFsSetVolume:			//0
			{
			TFileName name;
			aRequest.ReadL(KMsgPtr0, name);
			aNewPath.Set(name);
			break;
			}
		default:
			{
			ASSERT(0);
			break;
			}
		}
	}

//Get the size of the notification based on its type
TInt FsNotificationHelper::NotificationSize(CFsClientMessageRequest& aRequest, TFsNotification::TFsNotificationType aNotificationType, const TDesC& aName)
	{
	__PRINT(_L("FsNotificationHelper::NotificationSize"));
	
	/*
	 * If there are no new names, the order of the data in the buffer is:
	 * Word1   : NotificationSize (2 bytes) , PathSize (2 bytes)
	 * Word2   : NotificationType (Lower 2 bytes)
	 * Word(s) : Path (TText8) , [Any sub-class members]
	 * 
	 * Else for notification types ERename, EVolumeName and EDriveName the order is:
	 * Word1   : NotificationSize (2 bytes) , PathSize (2 bytes)
	 * Word2   : NewNameSize (2 bytes) , NotificationType (2 bytes)
	 * Word(s) : Path (TText8) , NewName (TText8)
	 * 
	 * EOverflow size: KNotificationHeaderSize
	 */	
	
	TInt size = KNotificationHeaderSize + Align4(aName.Size());
	
	switch(aNotificationType)
		{
		//NewName
 		case TFsNotification::ERename:
		case TFsNotification::EVolumeName:
		case TFsNotification::EDriveName:
			{
			TPtrC dest;
			NewPathName(aRequest,dest);
			size += Align4(dest.Size()); 
			break;
			}
		case TFsNotification::EFileChange:
			{
			size += sizeof(TInt64);
			break;
			}
		case TFsNotification::EAttribute:
			{
			size += sizeof(TUint64);
			break;
			}
		case TFsNotification::ECreate: 
		case TFsNotification::EDelete:
		case TFsNotification::EMediaChange:
			{
			break;
			}
		default:
			{
			ASSERT(0);
			break;
			}
		}
	return (TUint16) size;
	}

TFsNotification::TFsNotificationType FsNotificationHelper::NotificationType(TInt& aIndex)
	{
	__PRINT(_L("FsNotificationHelper::NotificationType(TInt)"));
	__ASSERT_DEBUG(aIndex < KNumRegisterableFilters, Fault(ENotificationFault));
	
	switch(aIndex) //No break statements here on purpose
		{
		case 7 : return TFsNotification::EMediaChange;
		case 6 : return TFsNotification::EDriveName;
		case 5 : return TFsNotification::EVolumeName;
		case 4 : return TFsNotification::EDelete;
		case 3 : return TFsNotification::EAttribute;
		case 2 : return TFsNotification::ECreate;
		case 1 : return TFsNotification::ERename;
		case 0 : return TFsNotification::EFileChange;
		default: ASSERT(0); return (TFsNotification::TFsNotificationType) 0;
		}
	}

//Get the array index of the notification based on its type
TInt FsNotificationHelper::TypeToIndex(TFsNotification::TFsNotificationType aType)
	{
	__PRINT(_L("FsNotificationHelper::ArrayIndex"));

	TInt index = 0; 
	switch(aType) //No break statements here on purpose
		{
		case TFsNotification::EMediaChange: index++;
		case TFsNotification::EDriveName:	index++;
		case TFsNotification::EVolumeName:	index++;
		case TFsNotification::EDelete:	 	index++;
		case TFsNotification::EAttribute:	index++;
		case TFsNotification::ECreate:	 	index++;
		case TFsNotification::ERename:	 	index++;
		case TFsNotification::EFileChange:	// skip;
		default: break;
		}
	__ASSERT_DEBUG(index < KNumRegisterableFilters, Fault(ENotificationFault));
	return index;
	}

TInt FsNotificationHelper::DriveNumber(const TPtrC& aPath)
	{
	if(aPath.Length() >= 2 && ((TChar)aPath[1])==(TChar)':')
		{
		TChar driveChar = ((TChar)aPath[0]);
		driveChar.UpperCase();
		TInt driveNum = driveChar-(TChar)'A'; 
		return driveNum;
		}
	else
		{
		return KErrNotFound;
		}
	}

//Get the attributes set and cleared
void FsNotificationHelper::Attributes(CFsClientMessageRequest& aRequest, TUint& aSet, TUint& aClear)
	{
	__PRINT(_L("FsNotificationHelper::Attributes"));

	TInt function = aRequest.Operation()->Function();
	const RMessage2& msg = aRequest.Message();

	switch(function)
		{
		case EFsFileSet:
			{
			aSet = msg.Int1();
			aClear = msg.Int2();
			break;
			}
		case EFsFileSetAtt:
			{
			aSet = msg.Int0();
			aClear = msg.Int1();
			break;
			}
		case EFsSetEntry:
			{
			aSet = msg.Int2();
			aClear = msg.Int3();
			break;
			}
		default:
			{
			ASSERT(0);
			break;
			}
		}
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
TInt CFsNotifyRequest::NotifyChange(CFsClientMessageRequest* aRequest,const TDesC& aName, TFsNotification::TFsNotificationType aNotificationType, CFsNotificationBlock& aBlock)
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

	TInt notificationSize = FsNotificationHelper::NotificationSize(*aRequest,aNotificationType,aName);
	
	iClientSyncLock.Wait();
	iTailSemaphore.Wait();
	
	TInt tail = iServerTail;
	
	//Validation
	TBool overflow = ValidateNotification(notificationSize, tail);
		
	//Now that we know there is enough space in the buffer we can write 
	//the standard attributes that all notifications have.

	//We can store the size of the notification 
	//and the size of the name in the same word.
	
	TUint16 nameLen = 0;	//Overflow has no name
	TInt notifSize = KNotificationHeaderSize;
	if(!overflow)
		{
		nameLen = (TUint16)aName.Size();
		notifSize = notificationSize;
		}
	else 
		{
		aNotificationType = TFsNotification::EOverflow;
		}	

	iServerTail = tail + notifSize;
	iTailSemaphore.Signal();
	
	TInt writeOffset = 0;	//Where to write in the block
	
	//Store notification Size and NameSize (Word1)
	TUint sizeNameLen = (notifSize << 16) | nameLen;	
	memcpy((TText8*)aBlock.Data()+writeOffset,&sizeNameLen,sizeof(TUint));
	writeOffset+=sizeof(TUint);

	TPtrC newName;
	
	if (aNotificationType == TFsNotification::ERename ||
		aNotificationType == TFsNotification::EVolumeName ||
		aNotificationType == TFsNotification::EDriveName)
		{
		FsNotificationHelper::NewPathName(*aRequest,newName);
		//Store NewNameSize and notification Type (Word2)
		TUint typeNewNameLen = ((TUint16)newName.Size() << 16) | (TUint16)aNotificationType;
		memcpy((TText8*)aBlock.Data()+writeOffset,&typeNewNameLen,sizeof(TUint));
		}
	else
		{
		//Store notification Type (Word2)
		memcpy((TText8*)aBlock.Data()+writeOffset,&aNotificationType,sizeof(TUint));
		}
	writeOffset+=sizeof(TUint);
	
	CFileShare* share = NULL;
    CFileCB* file = NULL;
    if(aRequest) //Don't always have a request such as when called from localdrives.
        {
        GetFileFromScratch(aRequest, share, file);
        }
    
    //
    //Store UID
    /*
	TUid uid;
	uid.iUid = KErrUnknown;
	if(aRequest && aRequest->Operation()->iFunction == EFsFileWriteDirty)
	    {
	    uid = aRequest->iUID;
	    }
	else if(aRequest)
	    {
	    uid = aRequest->Message().Identity();
	    }
	memcpy((TText8*)aBlock.Data()+writeOffset,&uid.iUid,sizeof(TUint32));
	writeOffset+=sizeof(TUint32);
	*/
	
	if(!overflow)
		{
		//Store Name (Word3)
		memcpy((TText8*)aBlock.Data()+writeOffset,aName.Ptr(),aName.Size());
		writeOffset += Align4(aName.Size());
		

		switch (aNotificationType)
			{
			case TFsNotification::EFileChange:
				{
				TInt64 size = 0;
				size = file->CachedSize64();
				memcpy((TText8*)aBlock.Data()+writeOffset,&size,sizeof(TInt64));
				writeOffset += sizeof(TInt64);
				break;
				}
			case TFsNotification::ERename:
			case TFsNotification::EVolumeName:
			case TFsNotification::EDriveName:
				{
				//Store NewName
				memcpy((TText8*)aBlock.Data()+writeOffset,newName.Ptr(),newName.Size());
				writeOffset += Align4(newName.Size());
				break;
				}
			case TFsNotification::EAttribute:
				{
				TUint set=0;
				TUint clear=0;
				FsNotificationHelper::Attributes(*aRequest,set,clear);
				TUint64 att = MAKE_TUINT64(set,clear);
				memcpy((TText8*)aBlock.Data()+writeOffset,&att,sizeof(TUint64));
				writeOffset += sizeof(TUint64);
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
void FsNotificationManager::HandleChange(CFsClientMessageRequest* aRequest,const TDesC& aOperationName, TFsNotification::TFsNotificationType aType)
	{
	__PRINT2(_L("FsNotificationManager::HandleChange() aRequest=0x%x, aType=%d"),&aRequest,aType);

	Lock(); //ToDo: Read Lock (Read/Write Lock)	
	if(Count())
		{
		//Only search while there are filters of this type set up.
		TInt index = FsNotificationHelper::TypeToIndex(aType);
		TInt& filterCount = FsNotificationManager::FilterRegister(index);
		TInt seenFilter = filterCount; //Number of requests set up for this type
		
		//Iterate CFsNotifyRequests
		TInt count = iNotifyRequests->Count();
		
		if(aType == TFsNotification::EMediaChange)
			seenFilter = count;
		
		//If there aren't any requests then breakout
		if(count == 0)
			{
			Unlock();
			return;
			}
		
		TInt driveNum = FsNotificationHelper::DriveNumber(aOperationName); 

		//For every notification request.
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
			TFsNotificationTypeArray* filterList = notifyRequest->FilterTypeList(driveNum,index);
			DoHandleChange(filterList,seenFilter,aRequest,notifyRequest,aOperationName,aType);

			if(aType==TFsNotification::EMediaChange)
				continue; //next request
			
			//If there are still filters to check
			if(seenFilter)
				{
				//Check changes that are not tied to a particular drive
				filterList = notifyRequest->FilterTypeList(KErrNotFound,index);
				DoHandleChange(filterList,seenFilter,aRequest,notifyRequest,aOperationName,aType);
				}
			}
		}
	Unlock();
	}

//A change has occurred in f32 represented by this
//request object. Work out which CfsNotify’s are interested
// (if any) and call CfsNotifyRequest::NotifyChange.
void FsNotificationManager::HandleChange(CFsClientMessageRequest& aRequest, TFsNotification::TFsNotificationType aType)
	{
	__PRINT(_L("FsNotificationManager::HandleChange"));
	TFileName currentOperationsName;
	FsNotificationHelper::PathName(aRequest, currentOperationsName);
	if(currentOperationsName.Length())
		HandleChange(&aRequest,currentOperationsName,aType);
	}

//A change has occurred in f32 represented by this
//request object. Work out which CfsNotify’s are interested
// (if any) and call CfsNotifyRequest::NotifyChange.
void FsNotificationManager::HandleChange(CFsClientMessageRequest& aRequest)
	{
	if(Count() && aRequest.Message().Handle() != KLocalMessageHandle)
		{
		__PRINT(_L("FsNotificationManager::HandleChange"));
		TFsNotification::TFsNotificationType operationNotificationType;
		FsNotificationHelper::NotificationType(aRequest.FsFunction(), operationNotificationType);
		HandleChange(aRequest,operationNotificationType);
		}
	}


////
#else
////

void FsNotificationManager::HandleChange(CFsClientMessageRequest* ,const TDesC&, TFsNotification::TFsNotificationType)
	{
	return;
	}

void FsNotificationManager::HandleChange(CFsClientMessageRequest& , TFsNotification::TFsNotificationType)
	{
	return;
	}

void FsNotificationManager::HandleChange(CFsClientMessageRequest&)
	{
	return;
	}

#endif //SYMBIAN_F32_ENHANCED_CHANGE_NOTIFICATION

//Called from FsNotificationManager::DoHandleChange
FsNotificationManager::TFsNotificationFilterMatch FsNotificationManager::DoMatchFilter(CFsClientMessageRequest* aRequest, const TDesC& aOperationName,CFsNotificationPathFilter& aFilter)
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
        TInt r = PathCheck(aRequest,aOperationName.Mid(2),&KCapFsSysFileTemp,&KCapFsPriFileTemp,&KCapFsROFileTemp, __PLATSEC_DIAGNOSTIC_STRING("FsNotificationManager::DoHandleChange"));
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

// This is called on a per drive basis.
void FsNotificationManager::DoHandleChange(TFsNotificationTypeArray* aFilterTypeArray,TInt& aSeenFilter, CFsClientMessageRequest* aRequest, CFsNotifyRequest* aNotifyRequest, const TDesC& aOperationName, TFsNotification::TFsNotificationType& aType)
	{		
	__PRINT(_L("FsNotificationManager::DoHandleChange()"));
	
	if(!aFilterTypeArray)
		return;
	
	TInt numFilters = aFilterTypeArray->Count();
	
	if(aType == TFsNotification::EMediaChange)
		numFilters = 1; //Only need to notify once per drive.
		
	//For every filter in this request
	for(TInt j = 0; j < numFilters;++j)
		{
		//Is the correct notification type
		aSeenFilter--;
		
		TBool filterMatch = EDifferent;
		if(aType != TFsNotification::EMediaChange)
			{
			CFsNotificationPathFilter& filter = *(((*aFilterTypeArray)[j]).iPathFilter);
			__PRINT2(_L("FsNotificationManager::DoHandleChange() operationName=%S, filterName=%S"),&aOperationName,filter.iPath);
			
			filterMatch = DoMatchFilter(aRequest,aOperationName,filter);
			if(filterMatch == FsNotificationManager::EContinue)
			    continue; //triggers for data cages
			
			//We need to check for changes coming in to a directory when its rename
			if(aType == TFsNotification::ERename && filterMatch==FsNotificationManager::EDifferent)  
                {
                TPtrC aDestinationNamePtrC;
                FsNotificationHelper::NewPathName(*aRequest,aDestinationNamePtrC);
                __PRINT2(_L("FsNotificationManager::DoHandleChange() destinationName=%S, filterName=%S"),&aDestinationNamePtrC,filter.iPath);
                filterMatch = DoMatchFilter(aRequest,aDestinationNamePtrC,filter);
                }
			}

		if(filterMatch || (aType == TFsNotification::EMediaChange))//Match or MediaChange (report regardless of filters)
			{
			//Matching - Handle change
			
			//Get a CFsNotificationBlock to use 
			//So that we can do IPC from a single place.
			CFsNotificationBlock* block = iPool->Allocate();
				
			TInt r = aNotifyRequest->NotifyChange(aRequest,aOperationName,aType,*block);
				
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
