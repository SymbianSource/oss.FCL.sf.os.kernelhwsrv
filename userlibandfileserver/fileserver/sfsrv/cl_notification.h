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
// fileserver/sfsrv/clnotification.h
//

/**
 * @file
 * @internalTechnology
 * @released
 */

#ifndef __CLNOTIFICATION_H__
#define __CLNOTIFICATION_H__

#include "f32notification.h"
	
/* 
 * This comment is:
 * @internalTechnology
 * 
 * In general, the getter functions for TFsNotification extract data from a given buffer in the order:
 *  <Notification Size> <Path Length> <Type> <Path> [any sub-class members]
 * Notification of type EOverflow doesn't have a path associated with it
 * If a new name exists (ERename, EVolumeName and EDriveName) then the order is:
 *  <Notification Size> <Path Length> <New Name Length> <Type> <Path> <New Name>
 * For EAttribute the order is:
 *  <Notification Size> <Path Length> <Type> <Path> <Attribute>
 * For EFileChange the order is:
 *  <Notification Size> <Path Length> <Type> <Path> <File Size>
 */

/**
 * A Mask of all the valid filters that a client can set
 * @internalTechnology
 */
const TInt KNotificationValidFiltersMask = (TUint)(
		TFsNotification::EAttribute |
		TFsNotification::ECreate |
		TFsNotification::EDelete |
		TFsNotification::EDriveName | 
		TFsNotification::EFileChange |
		TFsNotification::EMediaChange |
		TFsNotification::ERename |
		TFsNotification::EVolumeName
		);

/*
 * Different notification types have different data associated with them.
 *  
 * All types have the following data and are aligned in the buffer like so:
 * Word1 : Size (TUint16 - upper 2 bytes) , NameLength (TUint16 - lower 2 bytes),
 * Word2 : Type (TUint - 4 bytes)
 * Word3 : UID (TUint32 - 4 Bytes) 
 * 
 @internalTechnology
 @released
 */
const TInt KNotificationHeaderSize = (sizeof(TUint16)*2)+sizeof(TUint)+sizeof(TUint32);

/*
 * This is the minimum allowed size of the buffer set by the client that is
 * equal to the size of a notification of maximum length and an overflow
 *
@internalTechnology
@released
*/
const TInt KMinNotificationBufferSize = 2*KNotificationHeaderSize + 2*KMaxFileName;



/**
 * Fill any dead space at the end of the buffer with this
 * If there is any dead space it should always be at least 1 word in size
 * @internalTechnology
 */
const TUint KNotificationBufferFiller = 0xFFFFFFFF;

class TFsNotification; //forward ref

/* This class is used as the buffer in the file server notifier framework.
 * 
 * The buffer itself is simply a descriptor.
 * 
 * This class is internal because we may wish to change the 
 * buffer mechanism in the future without BC breaks etc.
 * 
 * @internalTechnology
 */
class CFsNotificationList : public CBase
	{
public:
	static CFsNotificationList* NewL(TInt aBufferSize);
	virtual ~CFsNotificationList();
		
private:
	CFsNotificationList();
	const TFsNotification * NextNotification();
	TInt BufferSize() const;

	HBufC8* iBuf;			//Heap based to allow buffer size spec at runtime
	TInt iHead; 			//offset to the head of the readable data.
	TInt iTail;				//offset to end of the readable data.

	TPckg<TInt> iTailPckg;	//stores iTail, used in Asyc IPC
	TPtr8 iBufferPtr;		//stores iBuf->Ptr, used in Asyc IPC
	friend class RFsNotify;
	friend class CFsNotify;
	}; 

/**
 * @internalTechnology
 */
class RFsNotify : public RSubSessionBase
	{
	public:
		TInt Open(RFs& aFs,CFsNotificationList* aBuffer, TRequestStatus& aBufferStatus);
		void Close();

		//[Re]Issues notification request
		void RequestNotifications(TRequestStatus& aStatus, TPckg<TInt>& aTailPckg);
		TInt CancelNotifications();
		TInt AddNotification(TUint aNotificationType, const TDesC& aPath,  const TDesC& aFilename);
		TInt RemoveNotifications();
	}; 

/**
 * @internalTechnology
 *
 * This class is the iBody of CFsNotify. It is being used to shield the client from the internals which are being stored here.
 *
 * @see CFsNotify
 */
class CFsNotifyBody : public CBase
	{
public:
	virtual ~CFsNotifyBody();
private:
	CFsNotifyBody();
	TRequestStatus iBufferStatus;	//for server->buffer updates
	TRequestStatus* iClientStatus;	//Client's TRS:for receiving notification
	CFsNotificationList* iBuffer;	//Buffer into which the server puts the notifications
	RFsNotify iFsNotify;
	
	friend class CFsNotify;
	};
#endif //__CLNOTIFICATION_H__

