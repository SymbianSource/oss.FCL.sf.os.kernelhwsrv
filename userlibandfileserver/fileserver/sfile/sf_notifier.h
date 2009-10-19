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
// f32\sfile\sf_notifier.h
// 
//

#include "sf_std.h"
#include "sf_pool.h"
#include "e32hashtab.h" 
#include "cl_notification.h"
#include "f32notification.h"

#ifndef SF_NOTIFIER_H
#define SF_NOTIFIER_H

/**
 * Number of notifications in TFsNotificationType that the client can set
 * @internalTechnology
 */
const TInt KNumRegisterableFilters = 8; 

/*
 * This determines the size of the CFsPool.
 * Until we have read/write locks there is no point in this being more than 1.
 * 
 * @internalTechnology
 */
const TInt KNotificationPoolSize = 1;


/**
 * A CFsNotificationPathFilter is a simple class containing two HBufCs of the target to be notified about:
 * 1 for the drive and path
 * 1 for the filename
 *  
 *  A CFsNotificationPathFilter has a 1 to Many relationship with TFsNotificationTypeFilter
 *  
 * @internalTechnology
 */
class CFsNotificationPathFilter
	{
public:
	static CFsNotificationPathFilter* NewL(const TDesC& aPath, const TDesC& aFilename);
	~CFsNotificationPathFilter();
private:
	void ConstructL(const TDesC& aPath, const TDesC& aFilename);
	CFsNotificationPathFilter();
public:
	HBufC* iPath;
	HBufC* iFilename;
	};

/**
 * A TFsNotificationTypeFilter is a class which contains a pointer to the associated path to monitor
 * and the type of notification.
 * 
 * @internalTechnology
 */
class TFsNotificationTypeFilter
	{
public:
	CFsNotificationPathFilter* iPathFilter;
	TFsNotification::TFsNotificationType iNotificationType; 
	//As we are storing filters in filter-specific
	//arrays then iNotificationType is potentially redundant.
	};

//These typedefs are to appease the compiler as it does not like
//nested templated arguments.
 /**
  *  @internalTechnology
  */
typedef RArray<TFsNotificationTypeFilter> TFsNotificationTypeArray;
/**
 * @internalTechnology
 */
typedef RArray<TFsNotificationTypeArray> TFsNotificationTypeDriveArray;

class CFsNotificationBlock; //forward decl.

/**
 * CFsNotifyRequest is a file-server side object representation of an RFsNotify sub-session.   
 * 
 * @internalTechnology
 */
NONSHARABLE_CLASS(CFsNotifyRequest) : public CFsObject
	{
public:
	
	/*
	 * Active means that the client is waiting for the first notification to be sent
	 * The status then changes to Outstanding when further notifications are sent to the buffer
	 * If the server overflows when in state EOutstanding then we move to state EOutstandingOverflow.
	 * From EOutstandingOverflow after the next call to RequestNotifications from the client, we must move to state EInactive.
	 * If we do not overflow then when RequestNotifications is called we can move back to state EActive.
	 * EInactive is when there are no notifications outstanding and request notifications hasn't been called. 
	 */
	enum TNotifyRequestStatus
		{
		EActive,				//Server waiting for a notification (filters added, RequestNotifications called)
		EOutstanding,			//Server waiting for client to call RequestNotifications, has notification(s) to send.
		EOutstandingOverflow,	//Server waiting for client to call RequestNotifications, has notification(s) to send, buffer has overflowed.
		EInactive				//Server waiting for RequestNotification, no notifications outstanding 
		};
	
	static CFsNotifyRequest* NewL();
	virtual ~CFsNotifyRequest();
	
	/*
	 * Returns the RArray<TFsNotificationFilter> for an index
	 * as returned from FsNotificationHelper::TypeToIndex()
	 */
	TFsNotificationTypeArray* FilterTypeList(TInt aDrive,TInt aIndex);
	
	//Removes all filters from iFilterList
	TInt RemoveFilters();

	//Add single filter to this request
	TInt AddFilterL(CFsNotificationPathFilter* aFilter, TUint aMask);

	//Sets filter as active/inactive
	void SetActive(TNotifyRequestStatus aValue);
	
	/**
	 *Get the status of this request.
	 *@See TNotifyRequestStatus 
	 */ 
	TNotifyRequestStatus ActiveStatus();
	
	/*
	 * Completes and frees notification request
	 * 
	 * @param aIsCancel is used to determine whether 
	 * to write back to the client or not when aReason != KErrNone.
	 * 
	 * In the case of closing the subsession you shouldn't write back to the client.
	 */
	void CompleteClientRequest(TInt aReason,TBool aIsCancel=EFalse);

	//RfsNotify::RequestNotifications has been called
	TInt SetClientMessage(const RMessage2& aClientMsg);

	/* 
	 * Called from FsNotificationManager::HandleChange(),
	 * this function packages the data in to a CFsNotificationBlock in preperation for 
	 * notification of this operation to the client.
	 * 
	 * Calling of this function means that we are notifying about this operation. (all checks passed)
	 * 
	 * aRequest can be NULL when the request doesn't come from the file server 
	 * (such as when a media card is removed)
	 */
	TInt NotifyChange(CFsClientMessageRequest* aRequest, const TDesC& aName, TFsNotification::TFsNotificationType aNotificationType, CFsNotificationBlock& aBlock);
		
	/*
	 * This function performs the IPC to the client's buffer.
	 */
	TInt SynchroniseBuffer(CFsNotificationBlock& aBlock,TInt aServerTail, TInt aNotificationSize);
	
	//Closing this notification
	void CloseNotification();

	//Simple getter
	TInt ClientMsgHandle();
	
private:
	CFsNotifyRequest();
	void ConstructL();
	
	//Check whether there is room for a new notification in the client's buffer
	TBool ValidateNotification(TInt aNotificationSize, TInt& aServerTail);
	
	/*
	 * The iTailSemaphore is used so that many CFsNotificationBlocks can
	 * be processed concurrently.
	 * This lock ensures that the iServerTail is safe.
	 */
	 //ToDo:  This should be a ReadWriteLock
	RFastLock iTailSemaphore;
	
	/*
	 * The iClientSyncLock is a Read/Write style lock whereby it is 
	 * set up with the value of KNotificationPoolSize.
	 * When a block is allocated it calls wait on this lock.
	 * 
	 * This lock is to ensure that all of the currently processing blocks are
	 * written before the client is updated.
	 * 
	 * i.e. if two blocks are being processed concurrently and the second 
	 * block is written we need to wait until the first one is also written
	 * before the client receives the updated tail.
	 */
	//ToDo: This should be a ReadWriteLock
	RFastLock iClientSyncLock;
	

	/*
	 * HashMap<DriveNumber, TFsNotificationTypeDriveArray>
	 * HashMap<DriveNumber, RArray<TFsNotificationTypeArray>>
	 * HashMap<DriveNumber, RArray<RArray<TFsNotificationTypeFilter>>>
	 * 
	 * Each value of iDrivesTypesFiltersMap is of type TFsNotificationTypeDriveArray 
	 * associated with a particular drive.
	 * 
	 * Each index of the TFsNotificationTypeDriveArray is a TFsNotificationTypeArray
	 */
	RHashMap<TInt,TFsNotificationTypeDriveArray> iDrivesTypesFiltersMap;
	
	/*
	 * The iPathFilterList is an RPointerArray of CFsNotificationPathFilters.
	 * 
	 * These are normally only accessed via a TFsNotificationTypeFilter (via iDrivesTypesFiltersMap),
	 * not via this array directly.
	 */
	RPointerArray<CFsNotificationPathFilter> iPathFilterList;
	
	RMessage2 iBufferMsg; //To update buffer
	RMessage2 iClientMsg; //client notification request
	
	CSessionFs* iSession; //Session associated with this request (TFsSessionDisconnect::DoRequestL)
	
	TNotifyRequestStatus iNotifyRequestStatus;	//Current status of this request
	
	//The following 3 variables must be aligned when modified.
	TInt iClientHead;	//Offset where the client should start reading from.
						//If the server writes past this offset we must overflow the client.
	
	TInt iClientTail;	//The end of the client's accessible range.
	
	TInt iServerTail;	//The end of the server's accessible range.
						//Overflow occurs if iServerTail becomes more than iClientHead.
	
	TInt iClientBufferSize;		//Buffer size is word-aligned.
	
	friend class TFsNotificationBuffer;	//For access to iClientBufferSize and iBufferMsg
	friend class TFsNotificationRequest;//For access to iClientBufferSize
	friend class FsNotificationManager; //For access to iSession
	friend class TFsNotificationOpen; //For access to iSession
	friend class TFsNotificationRemove; //For access to iDrivesTypesFiltersMap
	};

/**
 * A CFsNotificationBlock is a chunk of memory which is used to represent a notification
 * such that a single IPC can be performed from server to client.
 * 
 * CFsNotificationBlocks are stored in a CFsPool<CFsNotificationBlock>. 
 * 
 *@internalTechnology
 */
class CFsNotificationBlock
	{
public:
	static CFsNotificationBlock* New();
	~CFsNotificationBlock();
	TAny* Data();
private:
	CFsNotificationBlock();
	TText8 iData[KMinNotificationBufferSize];
	};

/**
 * Helper class to get certain attributes from or about a particular operation to used in a notification
 * 
 * @internalTechnology
 */
class FsNotificationHelper
	{
public:
	static void NotificationType(TInt aFunction,TFsNotification::TFsNotificationType& aNotificationType);
	static void PathName(CFsClientMessageRequest& aRequest, TDes& aName);
	static void NewPathName(CFsClientMessageRequest& aRequest, TPtrC& aName);
	static TInt NotificationSize(CFsClientMessageRequest& aRequest, TFsNotification::TFsNotificationType aNotificationType, const TDesC& aName);
	static TInt TypeToIndex(TFsNotification::TFsNotificationType aType);
	static TFsNotification::TFsNotificationType NotificationType(TInt& aIndex);
	static TInt DriveNumber(const TPtrC& aPath);
	static void Attributes(CFsClientMessageRequest& aRequest, TUint& aSet, TUint& aClear);
	};

/**
 * The FsNotificationManager is a static object
 * 
 *@internalTechnology
 */
class FsNotificationManager
	{
public:
	//New notification request from client
	static void AddNotificationRequestL(CFsNotifyRequest* aNotificationRequest);

	//Notification request cancel
	static void RemoveNotificationRequest(CFsNotifyRequest* aNotificationRequest);
	//Notification request cancel (session closed)
	static void RemoveNotificationRequest(CSessionFs* aSession);

	/* A change has occurred represented by this request.
	 * Work out which CFsNotifyRequests are interested
	 * (if any) and call CFsNotifyRequest::NotifyChange.
	 */
	static void HandleChange(CFsClientMessageRequest& aRequest);
	
	/* A change has occurred represented by this request.
	 * Work out which CFsNotifyRequests are interested
	 * (if any) and call CFsNotifyRequest::NotifyChange.
	 * 
	 * This override is used directly when we want to force a particular notification type
	 */
	static void HandleChange(CFsClientMessageRequest& aRequest, TFsNotification::TFsNotificationType aType);
	
	/* 
	 * This override is used directly when we want to specify the current operation's name (src) and notification type.
	 * 
	 * aRequest can be NULL when the request doesn't come from the file server 
	 * such as when a media card is removed, see LocalDrives::CompleteDriveNotifications
	 * 
	 * @See LocalDrives::CompleteDriveNotifications(TInt aDrive)
	 */
	static void HandleChange(CFsClientMessageRequest* aRequest, const TDesC& aOperationName, TFsNotification::TFsNotificationType aType);

	//Initialise iNotifyRequests and iStaticNotification
	static void OpenL();
	static TBool IsInitialised();
	
	/*
	 * On CFsNotifyRequest closing, Close is called if this is the last request being removed.
	 * This removes all of the managers private data.
	 */
	static void Close();

	/*
	 * Calls SetFilterRegister for every valid notification set in aMask.
	 */
	static void SetFilterRegisterMask(TUint aMask,TBool aAdd);
	
	/*
	 * Adds or Removes to the count of filters set up for a particular type
	 * This is a global count such that if there are no fiters for a particular type 
	 * HandleChange doesn't need to do any iteration for that type.
	 */
	static void SetFilterRegister(TUint aFilter, TBool aAdd, TInt aCount = 1);
	/*
	 * Get the number of registers filters set up on a particular type.
	 * @param aIndex the TFsNotificationType's index as determined from FsNotificationHelper::TypeToIndex
	 */
	static TInt& FilterRegister(TInt aIndex);

	/*
	 * Returns the number of CFsNotifyRequests set up
	 */
	static TInt Count();
	
	/*
	 * Lock the iChainLock (currently not a ReadWriteLock)
	 */
	static void Lock();
	
	/*
	 * Unlock iChainLock
	 */
	static void Unlock();

private:
  
    /*
     * @internalTechnology
	 * Used by DoMatchFilter and DoHandleChange to control the flow of 
	 * loop execution.
     */
    enum TFsNotificationFilterMatch
        {
        EDifferent  = 0x00, //Operation and Filters do not match.
        EMatch      = 0x01, //Operation and Filters do match.
        EContinue   = 0x02  //Data caged directory - Do not notify.
        };
    
    /*
     * Checks whether aOperation matches the filter name and/or path set in aFilter. 
     */
    static TFsNotificationFilterMatch DoMatchFilter(CFsClientMessageRequest* aRequest, const TDesC& aOperationName,CFsNotificationPathFilter& aFilter);
    
	/*
	 * Iterates filters for a particular drive.
	 * Called from HandleChange
	 */
	static void DoHandleChange(TFsNotificationTypeArray* aFilterTypeArray, TInt& aSeenFilter, CFsClientMessageRequest* aRequest, CFsNotifyRequest* aNotifyRequest, const TDesC& aOperationName, TFsNotification::TFsNotificationType& aType);
	
	/*
	 * Stores the CFsNotifyRequests
	 */
	static CFsObjectCon* iNotifyRequests;
	
	//As we are doing notifications 'in-place' which is multi-threaded
	//we need to have locking to protect iNotifyRequests.
	//ToDo: ReadWriteLock
	static RFastLock iChainLock;
	
	/*
	 * Global register per filter type. 
	 * Keeps a count of the number of filters set up for a particular type
	 * (NB: EMediaChange is reported regardless of filters set)
	 */
	static TInt iFilterRegister[KNumRegisterableFilters];
	
	/*
	 * This is a pool of blocks which are server-side versions of TFsNotification.
	 * They are used so that we can have a single IPC from server to client.
	 * 
	 * it will also be used for coalescing changes.
	 */
	static CFsPool<CFsNotificationBlock>* iPool;
	
	friend class CFsNotifyRequest;
	friend class RequestAllocator;
	friend class TFsNotificationSubClose;
	};


#endif /* SF_NOTIFIER_H */


