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
// f32\sfsrv\cl_notification.cpp
// 
//
#include "cl_std.h"
#include "cl_notification.h"

#ifdef SYMBIAN_F32_ENHANCED_CHANGE_NOTIFICATION
/*
 * The order of the data in the buffer is:
 * Word1   : NotificationSize (2 bytes) , PathSize (2 bytes)
 * Word2   : NewNameSize (2 bytes) , NotificationType (2 bytes)
 * Word3   : UID - NOT YET SUPPORTED
 * Word(s) : Path (TText8) , [Any sub-class members]
 * 
 * The notification size should be located at *this
 */ 
TInt TFsNotification::NotificationSize() const
	{
	TInt word1 = *(TInt*)this;
	return (word1 >> 16);
	}
 
/*
 * The order of the data in the buffer is:
 * Word1   : NotificationSize (2 bytes) , PathSize (2 bytes)
 * Word2   : NewNameSize (2 bytes) , NotificationType (2 bytes)
 * Word3   : UID - NOT YET SUPPORTED
 * Word(s) : Path (TText8) , [Any sub-class members]
 * 
 * The notification type should be located at:
 * 	*this + sizeof(NotificationSize) + sizeof(PathSize) + sizeof(NewNameSize)
 */ 
EXPORT_C TFsNotification::TFsNotificationType TFsNotification::NotificationType() const
	{
	TUint* word2 = PtrAdd((TUint*)this, sizeof(TUint));
	TFsNotificationType ret = (TFsNotificationType)(*word2 & 0x0000FFFF);
	//Check it is a valid type
	__ASSERT_DEBUG(!((TInt)ret & ~KNotificationValidFiltersMask) || (ret == EOverflow), Panic(ENotificationPanic));
	return ret; //Returns the lower 2 bytes of Word2
	}

/*
 * The order of the data in the buffer is:
 * Word1   : NotificationSize (2 bytes) , PathSize (2 bytes)
 * Word2   : NewNameSize (2 bytes) , NotificationType (2 bytes)
 * Word3   : UID - NOT YET SUPPORTED
 * Word(s) : Path (TText8) , [Any sub-class members]
 * 
 * The path size should be located at *this + sizeof(NotificationSize)
 */ 
TInt TFsNotification::PathSize() const
	{
	//Notification of type EOverflow does not have a path associated with it
	__ASSERT_DEBUG(NotificationType() != EOverflow,Panic(ENotificationPanic));
	TUint ret = (*(TUint*)this & 0x0000FFFF); //Returns the lower 2 bytes of Word1
	return (TInt)ret;
	}

/*
 * The order of the data in the buffer is:
 * Word1   : NotificationSize (2 bytes) , PathSize (2 bytes)
 * Word2   : NewNameSize (2 bytes) , NotificationType (2 bytes)
 * Word3   : UID - NOT YET SUPPORTED
 * Word(s) : Path (TText8) , [Any sub-class members]
 * 
 * The path should be located at: *this + KNotificationHeaderSize
 */
EXPORT_C TInt TFsNotification::Path(TPtrC& aPath) const
	{
	//Notification of type EOverflow does not have a path associated with it
	if(NotificationType() == EOverflow)
		return KErrNotSupported;

	TUint16* pathPtr = PtrAdd((TUint16*)this, KNotificationHeaderSize);
	aPath.Set(pathPtr,PathSize()/2);
	return KErrNone;
	}

/*
 * The order of the data in the buffer is:
 * Word1   : NotificationSize (2 bytes) , PathSize (2 bytes)
 * Word2   : NewNameSize (2 bytes) , NotificationType (2 bytes)
 * Word3   : UID - NOT YET SUPPORTED
 * Word(s) : Path (TText8) , [Any sub-class members]
 * 
 * The new name size should be located at: *this + sizeof(NotificationSize) + sizeof(PathSize)
 */ 
TInt TFsNotification::NewNameSize() const
	{
	//The only notifications containing a new name are ERename, EVolumeName and EDriveName
	__ASSERT_DEBUG((NotificationType() == ERename ||
					NotificationType() == EVolumeName ||
					NotificationType() == EDriveName),Panic(ENotificationPanic));
	TInt* word2 = PtrAdd((TInt*)this, sizeof(TInt));
	return ((*word2) >> 16);
	}

/*
 * The order of the data in the buffer is:
 * Word1   : NotificationSize (2 bytes) , PathSize (2 bytes)
 * Word2   : NewNameSize (2 bytes) , NotificationType (2 bytes)
 * Word3   : UID - NOT YET SUPPORTED
 * Word(s) : Path (TText8) , [Any sub-class members]
 * 
 * The new name should be located at: *this + KNotificationHeaderSize + Align4(PathSize)
 */
EXPORT_C TInt TFsNotification::NewName(TPtrC& aNewName) const
	{
	//Only ERename, EVolumeName and EDriveName have second paths
	//Notification of type EOverflow does not have a path associated with it
	TFsNotificationType notificationType = NotificationType();
	if((notificationType != ERename &&
		notificationType != EVolumeName &&
		notificationType != EDriveName) ||
		notificationType == EOverflow)
		{
		return KErrNotSupported;
		}

	TUint16* pathPtr = PtrAdd((TUint16*)this, KNotificationHeaderSize + Align4(PathSize()));
	aNewName.Set(pathPtr,NewNameSize()/2);
	return KErrNone;
	}

/*
 * The order of the data in the buffer is:
 * Word1   : NotificationSize (2 bytes) , PathSize (2 bytes)
 * Word2   : NewNameSize (2 bytes) , NotificationType (2 bytes)
 * Word3   : UID - NOT YET SUPPORTED
 * Word(s) : Path (TText8) , [Any sub-class members]
 * 
 * The attribute should be located at: *this + KNotificationHeaderSize + Align4(PathSize)
 */
EXPORT_C TInt TFsNotification::Attributes(TUint& aSetAtt, TUint& aClearAtt) const
	{
	if(NotificationType() != EAttribute)
		return KErrNotSupported;

	TUint* clearAttptr = PtrAdd((TUint*)this, KNotificationHeaderSize + Align4(PathSize()));
	aClearAtt = *clearAttptr;
	aSetAtt = *PtrAdd((TUint*)clearAttptr, sizeof(TUint));
	return KErrNone;
	}

/*
 * The order of the data in the buffer is:
 * Word1   : NotificationSize (2 bytes) , PathSize (2 bytes)
 * Word2   : NewNameSize (2 bytes) , NotificationType (2 bytes)
 * Word3   : UID - NOT YET SUPPORTED
 * Word(s) : Path (TText8) , [Any sub-class members]
 * 
 * The size should be located at: *this + KNotificationHeaderSize + Align4(PathSize)
 */
EXPORT_C TInt TFsNotification::FileSize(TInt64& aSize) const
	{
	if(NotificationType() != EFileChange)
		return KErrNotSupported;

	aSize = *PtrAdd((TInt64*)this, KNotificationHeaderSize + Align4(PathSize()));
	return KErrNone;
	}

/*
 * The order of the data in the buffer is:
 * Word1   : NotificationSize (2 bytes) , PathSize (2 bytes)
 * Word2   : NewNameSize (2 bytes) , NotificationType (2 bytes)
 * Word3   : UID
 * Word(s) : Path (TText8) , [Any sub-class members]
 */
EXPORT_C TInt TFsNotification::DriveNumber(TInt& aDriveNumber) const
	{
	TPtrC path(NULL,0);
	TInt r = Path(path);
	if(r == KErrNone)
		{
		if(path.Length() >= 2 && ((TChar)path[1]==(TChar)':'))
			{
			r = RFs::CharToDrive(path[0],aDriveNumber);
			}
		}
	return r;
	}
	

/*
 * The order of the data in the buffer is:
 * Word1   : NotificationSize (2 bytes) , PathSize (2 bytes)
 * Word2   : NewNameSize (2 bytes) , NotificationType (2 bytes)
 * Word3   : UID
 * Word(s) : Path (TText8) , [Any sub-class members]
 */
EXPORT_C TInt TFsNotification::UID(TUid& aUID) const
    {
    TUint* word3 = PtrAdd((TUint*)this, sizeof(TUint)*2);
    aUID.iUid = *word3;
    return KErrNone; 
    }
	


CFsNotificationList* CFsNotificationList::NewL(TInt aBufferSize)
	{
	CFsNotificationList* self = new(ELeave) CFsNotificationList;
	CleanupStack::PushL(self);
	self->iBuf = HBufC8::NewL(aBufferSize);
	self->iBufferPtr.Set((TUint8*)self->iBuf->Ptr(),0,self->iBuf->Des().MaxSize());
	CleanupStack::Pop(self);
	return self;
	}

CFsNotificationList::CFsNotificationList()
: iTailPckg(iTail), iBufferPtr(NULL,0)
	{
	}

CFsNotificationList::~CFsNotificationList()
	{
	delete iBuf;
	}

TInt CFsNotificationList::BufferSize() const 
	{
	return iBuf->Size();
	}

const TFsNotification * CFsNotificationList::NextNotification()
	{
	TFsNotification* notification;

	if(iHead == iTail)
		{
		return NULL;
		}
	TUint* startptr = (TUint*)iBuf->Ptr();
	TUint* nptr = PtrAdd(startptr, iHead);
	TInt bufferSize = iBuf->Des().MaxSize();
	TUint* endOfBuffer = PtrAdd(startptr, bufferSize);

	if(*nptr == KNotificationBufferFiller || nptr == endOfBuffer)
		{
		iHead = 0;
		notification = (TFsNotification*)startptr;
		}
	else
		{
		notification = (TFsNotification*)nptr;
		}
	iHead += notification->NotificationSize();
	if(iHead  == bufferSize)
		iHead = 0;
	
	return notification;
	}


EXPORT_C CFsNotify* CFsNotify::NewL(RFs& aFs, TInt aBufferSize)
	{
	CFsNotify* self=new(ELeave) CFsNotify;
	CleanupStack::PushL(self);

	//Making sure buffer size is at least minimally large and not too big
	if(aBufferSize > (KMaxTInt/2))
		{
		User::Leave(KErrArgument);
		}
	else if(aBufferSize < KMinNotificationBufferSize)
		{
		aBufferSize = KMinNotificationBufferSize;
		}
	
	self->ConstructL(aFs, Align4(aBufferSize));
	CleanupStack::Pop(self);
	return self;
	}

void CFsNotify::ConstructL(RFs& aFs,TInt aBufferSize)
	{
	iBody = new(ELeave) CFsNotifyBody();
	iBody->iBuffer = CFsNotificationList::NewL(aBufferSize);
	User::LeaveIfError(iBody->iFsNotify.Open(aFs,iBody->iBuffer,iBody->iBufferStatus));
	}

CFsNotify::CFsNotify()
	{
	}

EXPORT_C CFsNotify::~CFsNotify()
	{
	if(iBody)
		{
		if(iBody->iBuffer)
			{
			iBody->iBuffer->iTail = 0;
			iBody->iBuffer->iHead = 0;
			iBody->iFsNotify.Close();
			delete iBody->iBuffer;
			}
		}
	delete iBody;
	}

CFsNotifyBody::CFsNotifyBody()
	{
	}

CFsNotifyBody::~CFsNotifyBody()
	{
	}

EXPORT_C TInt CFsNotify::AddNotification(TUint aNotificationType, const TDesC& aPath, const TDesC& aFilename)
    {
    TInt pathLength = aPath.Length();
    if(aNotificationType == 0 || (pathLength <= 0 && aFilename.Length() <= 0))
        return KErrArgument;

    //Validate path.
    //Paths must be fully formed. i.e. with a valid or wild drive letter.
    if(pathLength)
        {
        if(aPath[0] != '*' && aPath[0] != '?')
            {
            TInt drive = KErrNotFound;
            TInt r = RFs::CharToDrive(aPath[0],drive);

            if(r!=KErrNone)
                return KErrPathNotFound;
            if(drive < EDriveA || drive > EDriveZ)
                return KErrPathNotFound;
            if(pathLength < 2)
                return KErrPathNotFound;
            else if(aPath[1] != ':')
                return KErrPathNotFound;
            }
        }


    return iBody->iFsNotify.AddNotification(aNotificationType, aPath, aFilename);
    }

//Removes notification request, does not close session
EXPORT_C TInt CFsNotify::RemoveNotifications()
	{
	return iBody->iFsNotify.RemoveNotifications();
	}

EXPORT_C TInt CFsNotify::RequestNotifications(TRequestStatus& aStatus)
	{
	if(aStatus == KRequestPending || ((iBody->iClientStatus != NULL) && (*iBody->iClientStatus == KRequestPending)))
		return KErrInUse;
	
	iBody->iClientStatus = &aStatus;
	//Read the new notifications which will start at tail.
	//(Also this forbids user access outside permitted range)
	iBody->iBuffer->iHead = iBody->iBuffer->iTail; 
	iBody->iFsNotify.RequestNotifications(aStatus, iBody->iBuffer->iTailPckg);
	return KErrNone;
	}

//Cancels notification request, does not close session
EXPORT_C TInt CFsNotify::CancelNotifications(TRequestStatus& aStatus)
	{
	if(aStatus != KRequestPending || &aStatus != iBody->iClientStatus)
		return KErrInUse;
	
	TInt r = iBody->iFsNotify.CancelNotifications();
	aStatus = !KRequestPending;
	iBody->iBuffer->iHead = 0;
	iBody->iBuffer->iTail = 0;
	return r;
	}

EXPORT_C const TFsNotification *  CFsNotify::NextNotification()
	{
	return iBody->iBuffer->NextNotification();
	}


TInt RFsNotify::Open(RFs& aFs, CFsNotificationList* aBuffer, TRequestStatus& aBufferStatus)
	{
	if(aBuffer == NULL || aBuffer->iBuf == NULL || &aFs == NULL || &aBufferStatus==NULL)
		return KErrArgument;
	
	TInt err = CreateSubSession(aFs,EFsNotificationOpen);
	if (err == KErrNone)
		{
		aBufferStatus = KRequestPending;
		//memclr((TUint8*)aBuffer->iBuf->Ptr(),aBuffer->iBuf->Des().MaxSize());
		SendReceive(EFsNotificationBuffer, TIpcArgs(&aBuffer->iBufferPtr,aBuffer->iBuf->Des().MaxSize()), aBufferStatus);
		}
	return err;
	}

void RFsNotify::Close()
	{
	CloseSubSession(EFsNotificationSubClose);
	}

/*
[Re]Issues notification request
Updates buffer, if supplied.

@return - last readable index of buffer.
*/
void RFsNotify::RequestNotifications(TRequestStatus& aStatus, TPckg<TInt>& aTailPckg)
	{
	aStatus = KRequestPending;
	SendReceive(EFsNotificationRequest,TIpcArgs(&aTailPckg),aStatus);
	}

TInt RFsNotify::CancelNotifications()
	{
	//there can only be one outstanding notification request at a time
	return (SendReceive(EFsNotificationCancel));
	}

//Adds notification filter
TInt RFsNotify::AddNotification(TUint aNotificationType, const TDesC& aPath, const TDesC& aFilename)
	{
	if(aNotificationType == 0 || (aPath.Length() <= 0 && aFilename.Length() <= 0))
		return KErrArgument;
	
	return (SendReceive(EFsNotificationAdd,TIpcArgs(aNotificationType,&aPath,&aFilename)));
	}

//Removes request, does not close session
TInt RFsNotify::RemoveNotifications()
	{
	return(SendReceive(EFsNotificationRemove));
	}
#else //SYMBIAN_F32_ENHANCED_CHANGE_NOTIFICATION

EXPORT_C TFsNotification::TFsNotificationType TFsNotification::NotificationType() const
	{
	Panic(ENotificationPanic);
	return (TFsNotification::TFsNotificationType)0;
	}

EXPORT_C TInt TFsNotification::Path(TPtrC&) const
	{
	Panic(ENotificationPanic);
	return KErrNotSupported;
	}

EXPORT_C TInt TFsNotification::NewName(TPtrC&) const
	{
	Panic(ENotificationPanic);
	return KErrNotSupported;
	}

EXPORT_C TInt TFsNotification::Attributes(TUint&,TUint&) const
	{
	Panic(ENotificationPanic);
	return KErrNotSupported;
	}

EXPORT_C TInt TFsNotification::FileSize(TInt64&) const
	{
	Panic(ENotificationPanic);
	return KErrNotSupported;
	}

EXPORT_C TInt TFsNotification::DriveNumber(TInt&) const
	{
	Panic(ENotificationPanic);
	return KErrNotSupported;
	}
	
EXPORT_C TInt TFsNotification::UID(TUid&) const
    {
 	Panic(ENotificationPanic);
	return KErrNotSupported;
    }

EXPORT_C CFsNotify* CFsNotify::NewL(RFs& , TInt)
	{
	Panic(ENotificationPanic);
	User::Leave(KErrNotSupported);
	return NULL;
	}

EXPORT_C CFsNotify::~CFsNotify()
	{
	Panic(ENotificationPanic);
	}

EXPORT_C TInt CFsNotify::AddNotification(TUint, const TDesC&, const TDesC&)
	{
	Panic(ENotificationPanic);
	return KErrNotSupported;
	}

EXPORT_C TInt CFsNotify::RemoveNotifications()
	{
	Panic(ENotificationPanic);
	return KErrNotSupported;
	}

EXPORT_C TInt CFsNotify::RequestNotifications(TRequestStatus&)
	{
	Panic(ENotificationPanic);
	return KErrNotSupported;
	}

EXPORT_C TInt CFsNotify::CancelNotifications(TRequestStatus&)
	{
	Panic(ENotificationPanic);
	return KErrNotSupported;
	}

EXPORT_C const TFsNotification *  CFsNotify::NextNotification()
	{
	Panic(ENotificationPanic);
	return NULL;
	}

CFsNotificationList::~CFsNotificationList()
	{
	Panic(ENotificationPanic);
	}

CFsNotifyBody::CFsNotifyBody()
	{
	Panic(ENotificationPanic);
	}

CFsNotifyBody::~CFsNotifyBody()
	{
	Panic(ENotificationPanic);
	}

#endif //SYMBIAN_F32_ENHANCED_CHANGE_NOTIFICATION

