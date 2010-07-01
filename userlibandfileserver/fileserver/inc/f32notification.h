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
// fileserver/inc/f32notification.h
//

#ifndef __F32NOTIFICATION_H__
#define __F32NOTIFICATION_H__

#include <f32file.h> //For clients that include F32Notification.h first.

/*
 * This class is not intended for instantiation neither on the stack
 * nor on the heap.
 * 
 * Clients wishing to use this class should use a pointer provided by 
 * CFsNotify::NextNotification().
 * 
 * @publishedAll
 * @released
 */
class TFsNotification
	{
public:
	enum TFsNotificationType
		{
		EFileChange		= 0x0001,	// File contents have changed or/and file has changed size
		ERename 		= 0x0002,	// File/directory renamed, or file replaced using RFs::Replace()
		ECreate 		= 0x0004,	// File/directory created, or file recreated using RFile::Replace()
		EAttribute 		= 0x0008,	// File attribute(s) changed
		EDelete 		= 0x0010, 	// File/directory deleted
		EVolumeName		= 0x0020, 	// Volume name modified
		EDriveName		= 0x0040, 	// Drive name modified
		EMediaChange 	= 0x0080, 	// File system mounted/dismounted, media inserted/removed or device formatted
		EOverflow		= 0x0100,	// Sent by the file server to notify when a buffer overflow occurs
		EAllOps 		= KMaxTUint
		};

	/*
	 * Any data returned is only valid after the previous asynchronous call to RequestNotifications has
	 * completed and before the next call to RequestNotifications. 
	 * 
	 * @returns the type of notification as TFsNotificationType.
	 * @publishedAll
	 * @released
	 */
	IMPORT_C TFsNotificationType NotificationType() const;
	
	/*
	 * Returns via the aPath parameter
	 * the path of the file or directory that this notification is notifying about.
	 * 
	 * Any data returned is only valid after the previous asynchronous call to RequestNotifications has
	 * completed and before the next call to RequestNotifications. 
	 * 
	 * @returns KErrNone upon success, otherwise one of the system wide errors
	 * @param aPath - TPtrC& which is assigned to the path that was changed as notified by this notification.
	 * @publishedAll
	 * @released
	 */
	IMPORT_C TInt Path(TPtrC& aPath) const;
	
	/*
	 * Certain notification types such as Rename, can provide 2 paths.
	 * The first path will be the file that has changed, whilst the second path
	 * with be the new name. (New filename in the case of rename).
	 * 
	 * Any data returned is only valid after the previous asynchronous call to RequestNotifications has
	 * completed and before the next call to RequestNotifications. 
	 * 
	 * @returns KErrNone upon success, else one of the system wide errors
	 * @param aNewName - TPtrC& which is set to the newname as described.
	 * @publishedAll
	 * @released
	 */
	IMPORT_C TInt NewName(TPtrC& aNewName) const; 
	
	/*
	 * Any data returned is only valid after the previous asynchronous call to RequestNotifications has
	 * completed and before the next call to RequestNotifications. 
	 * 
	 * @returns KErrNone upon success, else one of the system wide error codes
	 * @param aSetAtt - In the case of the NotificationType being EAttributes, for the path that was changed
	 * 					aSetAtt will return the attributes that were set.
	 * @param aClearAtt - In the case of the NotificationType being EAttributes,
	 * 					  returns the attributes that were cleared.
	 * @publishedAll
	 * @released
	 */
	IMPORT_C TInt Attributes(TUint& aSetAtt,TUint& aClearAtt) const;
	
	/*
	 * In the case of the file size having changed, such as in calls to file write and set size,
	 * aSize returns the new size of the file.
	 * 
	 * Any data returned is only valid after the previous asynchronous call to RequestNotifications has
	 * completed and before the next call to RequestNotifications. 
	 * 
	 * @returns KErrNone upon success, else one of the system wide error codes
	 * @param aSize - The new size of the file being notified about.
	 * @publishedAll
	 * @released
	 */
	IMPORT_C TInt FileSize(TInt64& aSize) const; 

	/*
	 * @returns The Drive Number associated with the Path.
	IMPORT_C TInt DriveNumber(TInt& aDriveNumber) const;
	 */
	
	/*
	 * @returns the UID of the process that caused this change.
	IMPORT_C TInt UID(TUid& aUID) const;
	 */
private:
	//Declared private to prevent construction
	TFsNotification();
	~TFsNotification();
	//To prevent copying.  No implementation is provided.
	TFsNotification& operator=(TFsNotification& aNotification);
	
	TInt PathSize() const;
	TInt NewNameSize() const;
	TInt NotificationSize() const;

	friend class CFsNotify;
	friend class CFsNotificationList;
	};

class RFsNotify; //incomplete decl
class CFsNotifyBody; //incomplete decl

/*
 * CFsNotify is a class which allows changes to file and directories to be monitored
 *
 * The notification framework supported by CFsNotify is able to keep track of multiple notifications,
 * whilst ensuring that notifications cannot be missed (unlike RFs::NotifyChange which can miss changes).
 *
 * CFsNotify encapsulates the client-side sub-session associated with
 * the file server notification framework.
 * 
 * In order to recieve notifications of changes to a file system, a 
 * client may use this class in order to set up notification 'filters' and
 * request notifications.
 * 
 * Filters can be set for specific types of operations on specific (and wildcard) paths.
 * 
 * The difference between using this framework and RFs::NotifyChange is that
 * this framework will provide significant performance improvements, especially so
 * when operating on a large number of files.
 * 
 * In addition, this framework will provide verbose information about each
 * change that occurs, meaning that from a user's perspective they no longer have
 * to perform expensive directory scans in order to acertain what has changed.
 *
 * Notifications can also provide additional information:
 * EFileChange provides the new file size
 * ERename provide the new name of the affected file.
 * EDriveName/EVolumeName provides the name of the drive which has changed
 * EAttribute provides the attributes which were set and cleared on the affected file. 
 * 
 * @publishedAll
 * @released
 */
class CFsNotify : public CBase
	{
	public:
		
		/*
		 * Factory function. Creates a new CFsNotify and returns a pointer to it.
		 *
		 * CFsNotify stores notifications in a buffer. 
		 * Clients of CFsNotify must specify how large this buffer should be.
		 * 
		 * As a guideline: Notification objects in the buffer typically have a 8byte header,
		 * followed by a word aligned string containing the fullname of the file that has changed.
		 * In the case of a rename notification both the original and the new fullnames are stored.
		 *
		 * However, clients must not assume to know the exact size of notifications when determining the size of their buffer,
		 * as it is not possible to know how often a client will be able to read the notifications from the buffer.
		 * In addition, if further notification types are added, then the header size or maximum data could increase.
		 * 
		 * Thus, clients must ensure that their notification handling code appropriately deals with an overflow notification, whereby the
		 * buffer was not large enough to store all of the notifications.
		 *
		 * If aBufferSize is greater than (KMaxTInt/2) then it will return KErrArgument.
		 * If aBufferSize is less than KMinNotificationBufferSize (which is an internal constant but is approximately equal to 1KB) then aBufferSize will be 
		 * set to KMinNotificationBufferSize.
		 * 
		 * @param aFs - RFs session. Must be connected.
		 * @param aBufferSize  - Size of buffer in which to store notification objects.
		 * @return CFsNotify* - Pointer to newly created CFsNotify.
		 * @publishedAll
		 * @released
		 */
		IMPORT_C static CFsNotify* NewL(RFs& aFs, TInt aBufferSize);
		
		/*
		 * CFsNotify Destructor.
		 * @publishedAll
		 * @released
		 */
		IMPORT_C virtual ~CFsNotify();

		/*
		 * Clients are able to set-up notifications for changes based on Notification filters.
		 * 
		 * Filters can be set according to the type of notification required and the path to monitor.
		 * Notification types are set up as a bitmask of TFsNotification::TNotificationType.
		 * 		e.g.  (TUint) TFsNotification::ECreate | TFsNotification::EFileChange
		 * 
		 * So long as clients have filters set for a particular drive, they will automatically
		 * receive TFsNotification::EMediaChange notifications regardless 
		 * of whether they are registered for EMediaChange or not.
		 * This notification can be caused by media card removal, the file system being dismounted or 
		 * the media being formatted.
		 *
		 * If clients set the EOverflow notification type then AddNotification will return an error.
		 * The exception to this is when using EAllOps which is used as a convenience notification type
		 * which matches all of the notification types.
		 * 
		 * aPath specifies which path should be monitored for changes.
		 * aFilename specifies which filename should be monitored for changes.
		 * 
		 * Either aPath or aFilename may be zero-length strings:
		 * 	If you want to match a specific filename in every directory, you can leave aPath blank.
		 *	Leaving aFilename blank results in matching against aPath only (i.e. a directory).
		 * 
		 * '*' and '?' wild cards are allowed in the path and filename.
		 * 
		 * To match against:
		 * Specified directory only : (This is the specified directly only, not its contents.)
		 * 		- aPath 	== drive:\\[path\\]directory\\
		 * 		  aFilename == 					(zero length descriptor)
		 *
		 * Specified file only : 
		 * 		- aPath		== drive:\\[path\\]directory\\
		 * 		  aFilename == file.txt (or *, *.*, or *.txt)
		 *
		 * To enable matching against directories recursively:
		 * 		- aPath		== drive:\\[path\\]directory\\*    (NB: trailing * denotes directory recursion)
		 * 		- aFilename == file.txt (or *, *.*, or *.txt)
		 *
		 * 
		 * @param aNotificationType - A Bitmask of the type of change to be monitored for based on the values of TFsNotification::TNotifyType
		 * @param aPath - The directory to monitor
		 * @param aFilename - The filename to monitor
		 * @return KErrNone upon success, else one of the system wide error codes
		 * @publishedAll
		 * @released 
		 */
		IMPORT_C TInt AddNotification(TUint aNotificationType, const TDesC& aPath, const TDesC& aFilename);
		
		/*
		 * Remove Notifications will remove all of the filters that have been 
		 * set by calls to AddNotification.
		 * 
		 * This does not cancel any ongoing requests, but any on going requests will not complete until
		 * filters have been added via calls to AddNotification
		 * @return KErrNone or a system-wide error code.
		 * @publishedAll
		 * @released
		 */
		IMPORT_C TInt RemoveNotifications();
		
		/*
		 * This is an Asynchronous function.
		 * 
		 * RequestNotification will issue a notification request asynchronously.
		 * When this request has completed, aStatus's value will be set something other than KRequestPending.
		 * 
		 * Completion means that at least 1 notification has been put in the notification buffer.
		 * Clients should call NextNotification() until it returns NULL.
		 * 
		 * When all the notifications have been read, the client should call RequestNotifications to set up another request.
		 * 
		 * Notifications will continue to be stored by the file server 
		 * until CancelNotifications is called, meaning that notifications that occur 
		 * whilst the client is processing the current notifications are not lost.
		 * @param aStatus - A TRequestStatus that shall be completed when the Asynchronous request is completed.
		 * @return KErrNone or a system-wide error code.
		 * @publishedAll
		 * @released
		 */
		IMPORT_C TInt RequestNotifications(TRequestStatus& aStatus);
		
		/*
		 * Cancels the outstanding Asynchronous notification request that was set up
		 * in the call to RequestNotifications.
		 * @param aStatus - The TRequestStatus that was used in the call to RequestNotification
		 * @return KErrNone or a system-wide error code.
		 * @publishedAll
		 * @released
		 */
		IMPORT_C TInt CancelNotifications(TRequestStatus& aStatus);
		
		/*
		 * Once a notification request has completed, the client should call
		 * NextNotification in order to access the notifications that have been stored in a notification buffer.
		 * 
		 * NextNotification returns a pointer to a TFsNotification. Each TFsNotification
		 * represents a single notification.
		 * 
		 * When NULL is returned, there are no more Notifications to read and RequestNotification must be called.
		 * 
		 * @return const TFsNotification* - A pointer to a notification
		 * @publishedAll
		 * @released
		 */
		IMPORT_C const TFsNotification * NextNotification();
		
	private:
		// Allocates buffer.
		void ConstructL(RFs& aFs,TInt aBufferSize);
		CFsNotify();

		//Hide internals
		CFsNotifyBody* iBody;
		
		friend class RFsNotify;
		};

#endif //__F32NOTIFICATION_H__

